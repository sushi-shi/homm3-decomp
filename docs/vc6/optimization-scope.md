# VC6 C2 optimization scope — how far effects reach, and what we can predict

Answers three recurring questions: (1) is the inline→everything cascade written
down; (2) can we predict whether something inlined; (3) how *non-local* are the
optimizations across a function. Grounded in what the `vc6` area reverse-
engineered (inliner, register allocator) plus the Ghidra atlas's module map
(`evidence/vc6/c2-tu-map.tsv`); passes we located but did not model are marked
**located-only**.

## 1. The C2 pass pipeline (image base 0x10700000; anchor RVAs from the atlas)

C1XX (front end) parses the TU and emits IL — symbol handles in parse order
(`docs/vc6/handle-order.md`), each callee's size estimate `cb`, candidacy flags.
C2 then runs, roughly:

| order | pass | module (anchor RVA) | modeled? |
|---|---|---|---|
| 1 | read IL | `reader.c` 0x84d9b | handle/cb extraction (il-format.md) |
| 2 | **inline expansion** | `inline.c` 0x94521 | **yes — `predict()`, 9/9** |
| 3 | flow graph | `fg.c` 0x86c8a | substrate (CFG) |
| 3 | loop graph | `lg.c` 0x91d26 | substrate (loops) |
| 4 | global dataflow (CSE, copy-prop, DCE) | `globdf.c` 0x8f5e3, `globopt.c` 0x90643 | located-only |
| 4 | single-def/single-use | `sdsu.c` 0x87418 | located-only |
| 4 | local DAG (block CSE/order) | `dag.c` 0x91bbf | located-only |
| 5 | **register allocation** | `color.c` 0x8e474, `regasg.c` 0x8b906 | **yes — first-fit preference** |
| 6 | scheduling | `schedmd.c` 0x7a7de | located-only |
| 7 | emit | `code.c`/`emit.c` | — |

**The one load-bearing ordering fact we proved:** inlining (step 2) runs *before*
register allocation (step 5), and its cost model uses the front-end `cb`, not the
allocated result. So the cascade is **one-way**: inline decisions feed regalloc /
CFG / scheduling; those never feed back into inlining.

## 2. How non-local each optimization is

The heavy passes reason over the **whole function** (or a whole loop), not per
statement — which is why a single local change is rarely local in effect.

- **Register allocation — WHOLE FUNCTION.** Live ranges span all basic blocks;
  the allocator walks pseudos in creation order and first-fits over
  `{EAX,ECX,EDX,ESI,EDI,EBX,EBP}`. One added value (or one call vs inline) shifts
  the ordering and **renames registers everywhere downstream** — the modal "B1
  swap" is a whole-function decision, not a local one (`docs/vc6/regalloc.md`,
  catalog B7).
- **Global dataflow — WHOLE FUNCTION, cross-block** (`globdf.c`/`globopt.c` over
  `fg.c`): common-subexpression elimination reuses a value across blocks; copy
  propagation (the `this` alias we measured); dead-store / dead-code elimination
  (a store in one block killed by a def three blocks later). located-only — we
  *observe* these via `sema diff`, we do not predict them.
- **Loop optimizations — WHOLE LOOP NEST** (`lg.c`): LICM hoists loop-invariant
  computation out (VideoClose, iconwdgt); strength reduction / induction-variable
  rewriting spans the loop. located-only.
- **CFG optimizations — WHOLE FUNCTION**: jump threading, tail-merging,
  cross-jumping operate over all of the function's exits (the merged-return /
  DUP-EXIT family, catalog D4–D7).
- **Inline budget — PER CALLER (whole function), sequential**: charged in tuple
  order, so an early inline starves a later site (A9); nested expansion divides
  `budget ÷ sites-remaining`.
- **Scheduling — mostly WITHIN A BLOCK** (`schedmd.c`/`dag.c`), but the schedule
  interacts with the global allocation, so it isn't cleanly isolated.

What *is* genuinely local: instruction selection per tuple, immediate/SIB
encoding (the B18 base/index tie-break), and window peepholes (`fppeeps`). These
are the residuals no whole-function reasoning reaches — and the ones the tools
correctly report as not source-addressable.

**Consequence for matching:** because 2, 4, 5, and the CFG passes are
whole-function, and handle numbering is whole-*TU*, a mismatch propagates. This is
the "butterfly": a declaration-count change moves a distant function (handle
order, TU-global), a call-vs-inline renames every register after it (allocation,
function-global). It is *why* the doctrine is **inline → control-flow → register**
(fix the upstream, function-global cause first) and why one wrong inline can drop
a function to 46%.

## 3. Can we predict whether something inlined?

**Yes — deterministically, not just probabilistically — for a source we control.**
`inline_model.predict(caller_cb, sites)` implements the RE'd rule and is validated
9/9 against the real compiler. Given the inputs it says expand-or-call exactly:

- **caller_cb / callee cb**: measurable with `inline_model --measure-cb` (titrates
  a callee's front-end estimate against the real compiler) or readable from the
  IL (`il handles` / the `sy` stream). With these, prediction is exact.
- **No-compile / probabilistic mode**: estimate `cb ≈ 14 × simple-statements`
  (`CB_PER_STMT`, measured) and candidacy from the ~13-statement front-end
  save-gate. That predicts expand/call from source structure alone, with the
  uncertainty of the estimate — good enough to answer "will this inline?" and to
  quantify the budget gap (`inline_model --gap`: "grow the caller ~N statements").

Two honest limits: (a) for **retail** we don't predict, we **read** — the
`predict-inline` diagnoser reads retail's actual out-of-line calls from the
delinked object, so prediction is for *our* reconstruction and for reasoning about
gaps; (b) the model omits the rarely-triggered post-substitution veto (inliner.md
§6). Register *bindings* are likewise predictable (first-fit in creation order,
why-reg v2) **when the divergence is handle ORDER**; when it is handle STATE
(values differ, order matches) it is C2-internal and not predictable from order
alone (`docs/vc6/handle-order.md`).

## 4. Is the cascade documented?

Now yes — here, plus the pieces it ties together: the inliner rule
(`inliner.md`), the allocator (`regalloc.md`), catalog **B7** ("register renaming
cascade after a call-vs-inline change") and the smackmgr worked example, and the
`diagnose` routing that encodes inline→flow→register. The taxonomy has also been
applied outside HOMM3 (`gruntz-wall-identifier-field-note.md`, MSVC 5.0), so the
scope model is not HOMM3-specific.
