---
name: wall-identifier
description: Identify and route a HoMM3 matching WALL using the vc6 compiler-model tooling. When a reconstructed function plateaus below 100% and no obvious spelling closes it, run `homm3 vc6 diagnose <unit:fn>` to classify the residual (inliner / control-flow / register / masked-cosmetic / include-set) and get the exact solver command + fix lever. Use when a function is stuck, when triaging which plateaus are worth effort, when asked "why won't this match", "what kind of wall is this", "which solver", or when reasoning about register swaps, inline divergence, control-flow shape, or include-set sensitivity. Complements the `match` skill (which reconstructs); this one DIAGNOSES the wall.
---

# wall-identifier — classify a matching wall with the vc6 tooling

The pinned VC6 compiler is a deterministic function: `bytes = f(preprocessed TU,
flags)`. Retail came out of that same `f` from real source, so **every function
is reducible in principle** — the job is finding which *input* differs. The `vc6`
area reverse-engineered `f`'s inliner and register allocator (from `C2.DLL`) and
its front-end IL (from `C1XX.DLL`), so a plateau's residual can be *named* — which
compiler decision diverged, and which source/TU input controls it — instead of
guessed at with a blind spelling sweep.

**Mental model that decides everything below:** the unit of reproduction is the
**whole TU**, not the function. Some residuals are controlled by *TU-global /
front-end state* (symbol-handle order, inline budget) that a local body edit
CANNOT reach — but matching the TU context can. The tools tell you which case
you're in.

## Start here: `homm3 vc6 diagnose <unit:fn>`

One command from a red `sema diff` to a routed answer. It reads the same
base-vs-delinked-target pair the ratchet scores (no recompile), classifies the
residual, routes in doctrine order **inline → control-flow → register**, and
prints the exact solver command(s) to run.

```
homm3 vc6 diagnose 'ai_combat:?do_general_melee@type_AI_combat_data@@QAEXAAV1@@Z'
```

`<unit:fn>` is `unit:mangled-name` (the mangled name from the objdiff report /
`homm3 status`).

For a whole-tree triage instead of one function, two sweeps, and they answer
different questions:

- **`homm3 vc6 queue`** → `evidence/wall-census.tsv`. Every unmatched function
  in the tree, routed exactly as `diagnose` routes it, ranked by **recoverable
  bytes** = `size * (1 - fuzzy/100)`. Use this to decide WHAT TO WORK ON.
- **`homm3 vc6 report`** → `evidence/vc6/plateau-diagnosis.md`. A per-function
  markdown table of plateaus at ≥ `--lo` (default 50%), with reg/flow distances
  and the knob. Use this to READ a unit's plateaus in detail. It reports the
  raw classifier, not the routed answer, and by default cannot see stubs or
  anything under 50% at all.
- **`python3 -m homm3.analysis.ordermap <unit>`** → the DC-roster-to-x86
  alignment for claiming work, with a VERDICT from arity agreement and span
  coverage. Verdicts as of 2026-08-20, after both anchor fixes below:
  **spells 100%, advmgr 98%, army 98%, town 96%, townmgr 95%, cmbtmgr 94%,
  hero 92% — all USABLE**; game 76%, mapcell 70% — MIXED; ai_tactical 51%,
  sacrifice_window 64%, tradpost 55% — DO NOT CLAIM; seerhut THIN.
  ai_tactical's 51% is real, not an artifact: it holds 82 anchors and is
  already fully reconstructed, so what is left genuinely does not align.

**A TOOL VALIDATED ON ONE UNIT IS NOT VALIDATED** (learned the hard way,
2026-08-20). `ordermap`'s anchor detection tokenized the mangled label and
demanded exactly one DC-name match. Every `army::X` label contains the token
`army`, which the roster's own `army::army` constructor also answers to — so
almost every anchor in almost every unit was silently discarded. It was
calibrated on `spells`, which is the ONE unit that cannot reveal the bug,
because its claims are mostly free functions whose labels never carry a class
token. On that evidence a triage table was published calling `spells` the only
usable unit and `army` MIXED at 79%. After the fix army is 94% USABLE, hero
93%, advmgr 97%, cmbtmgr 94%, town 96%, and game moved 47% -> 75%. Anchors
went 2 -> 73 on army, 9 -> 96 on hero, 15 -> 95 on advmgr.
Two lessons: **a low-anchor verdict may be an artifact of the anchor finder,
not a property of the unit** — check the anchor count before believing a
verdict; and validate a heuristic on the unit most likely to BREAK it, not the
one it was written against.

**ANCHOR ON THE `dc` TAGS, NOT ON NAMES.** Claims are written
`VA(0x004xxxxx, 0xSIZE)  // <evidence>, dc 0x<off>` — that tag IS a
DC-to-retail pairing, already reviewed by whoever landed the claim, and 1579
of them exist across 108 TUs. `ordermap` now reads them as its primary anchor
source and falls back to name matching only for claims with no tag. That is
what a lane did by hand to get a 53-anchor map when the tool was offering it
2. With tags in play `spells` reaches **100% agreement over 100% of its
span**, army 94 -> 98%, townmgr 86 -> 95%. When you land a claim, WRITE THE
`dc` TAG — it is not decoration, it is the next lane's anchor.

**RANK ON BYTES, NOT ON COUNTS OR PERCENTAGES.** Both other rankings mislead,
in opposite directions, and the census measured by how much (2026-08-20, 347
unmatched functions, 71.6 KB recoverable):

```
    34.4 KB    47 fn   unclaimed (no source binding)
    25.4 KB    84 fn   inliner (predict-inline)
     6.2 KB    58 fn   control-flow (why-branch)
     5.3 KB   136 fn   register-homing (why-reg)
```

The first row is the one to internalise: **nearly half of everything left is
in functions no source claim owns**, sitting at 0.00% under a flat carve name.
The solvers cannot see them at all — there is no symbol in the base object to
compare against — so any triage built only on what `diagnose` can classify
silently omits half the campaign. `queue` counts them; `report` cannot see
them at any `--lo`.

Counting functions says register-homing is the dominant wall — it is the most
common and the least valuable, 136 functions holding 40 bytes each on average.
The inliner is a third as many functions and five times the mass. Percentage
misleads the same way per function: advmgr's `QuickInfo` sits at 90.20% and is
the single largest prize in its unit at 944 recoverable bytes, while
`SetEnvironmentOrigin` at 63.94% is worth 210. Work the low-percentage ones
because one structural error is often behind them, not because the number
looks worse.

## The wall taxonomy (what `diagnose` reports, and the lever for each)

| class | signal | tool | lever — and is it LOCALLY editable? |
|---|---|---|---|
| **eh-cleanup** | the `[ebp-4]` EH state transcript diverges (COUNT / ORDER) | read the funclets | **object lifetimes** — a statement or temporary is missing, or a callee is nothrow on one side; see below |
| **inliner** | out-of-line CALL multiset diverges (a callee inlined on one side only; A8/A9/A12) | `predict-inline` | **budget starvation** — usually NOT a local edit; *finish the caller's body* |
| **control-flow** | CFG/branch shape diverges — block count, ret count, loop rotation, merged-return (D-family) | `why-branch` | loop form / merged-return placement / case order / ternary / bool spelling — often local |
| **register-homing** | schedule aligned, register bindings permuted (ESI/EDI swap), memory-homing, spill (B-family) | `why-reg --model` | pseudo-creation order — local ONLY when the swapped value is a named local; else C1 handle-state |
| **masked-equal** | reg-dist 0 AND flow-dist 0 but objdiff < 100 | — | displacement/reloc only (C9/D21) — **not a real wall**, don't grind |
| **include-set** | a header/type edit moved an *unrelated* function's score | `il-diff` | front-end symbol-handle renumbering — match retail's include closure (TU-global) |

### eh-cleanup walls — read this BEFORE any spelling (new 2026-08-14)
`diagnose` prints an `eh signal` line whenever the two sides' `[ebp-4]` state
stores disagree. That transcript is a **record of object lifetimes**, and it is
the one signal `predict-inline` / `why-branch` / `why-reg` all miss.
**The rule: VC6 emits a cleanup chain only for a region that can THROW.** So a
COUNT divergence names a *missing statement or a missing throwing call*, not a
mis-spelling — and it is often a TU-input fact rather than a body fact: VC6's
`<new>` declares `operator delete(void*) _THROW0()` while the implicit
declaration (and `NEW.H`, which has only the placement forms) leaves it
throwing. Do not infer the map — **retail publishes it as data**: the funclet
group under `<Class>_<fn>_unwindNN` in `build/gen/symbol_names.csv`, each entry
an `add ecx,<off>; jmp ~T` naming the subobject it cleans. Byte-proven closure:
`Bitmap816::~Bitmap816` (`docs/vc6/eh-cleanup.md`). Caveats already measured:
a `[N,0,N+1,0,…]`-vs-`[N,N+1,…]` shape is an *inliner* witness, and the body's
site count does **not** reach a member-initialiser expansion.

### inliner walls — the dominant class (~65% of plateaus)
`homm3 vc6 predict-inline <src> --fn <caller> --against <unit:caller>` lists which
callees retail inlines but we call (UNDER) or vice-versa (OVER). The `/Ob2` rule
is RE'd and validated (`docs/vc6/inliner.md`): **budget = clamp(2×caller_cb, 1000,
35000)**, spent sequentially per call site (cost = the callee's front-end size
estimate; ≤40 is free), nested expansions get `budget ÷ sites-remaining`. The huge
`basic_string::_Tidy` / `vector` / `new` / `delete` under-inlines are **budget
starvation**: our leaner reconstruction sits at the 1000 floor while retail's
fuller body earns a bigger budget and inlines everything. **FIX: finish the
caller's body — the budget follows statement mass (byte-inert statements count).
Do NOT chase `_Tidy`/`vector` spellings, `#pragma auto_inline`, or headers.** On
low-% functions this class largely self-resolves as reconstruction completes; it
is the pure remaining wall only on high-% rows.

### control-flow walls
`homm3 vc6 why-branch <src> --fn F --against <unit:F>` — guided oracle search over
the D-knobs (goto ↔ while ↔ `for(;;)+break`, split-if vs `||`/`&&` vs
merged-return block placement, switch case-emission order, ternary-vs-if, bool
materialization). `homm3 sema diff <addr> --branches` is the raw structural signal.

### THE SOLVERS RANK ON DISTANCE, THE LEDGER RANKS ON FUZZY — RE-MEASURE
Every solver here scores candidates on its own distance metric (register-slot
disagreements, branch-shape disagreements). That metric is a PROXY and it can
rank OPPOSITE to the number the ratchet banks. Measured 2026-08-20: why-reg's
top-ranked pick on cmbtmgr's `ShootAnimatedMissile` improved register distance
by 20 slots and LOST 0.46 fuzzy. A why-branch pick on the same lane's tree
improved branch distance and moved fuzzy not at all.

So treat a solver's "improved" column as a HYPOTHESIS ORDERING, never as a
result: apply the edit, run the real build, and keep it only if FUZZY rose.
The solvers are still worth running first — a why-branch mutation closed
readGarrisonData outright — but the verdict is always the ratchet's.

### register walls — use the model (v2)
`homm3 vc6 why-reg <src> --fn F --against <unit:F> --model`. The allocator is RE'd
(`docs/vc6/regalloc.md`): one preference order **EAX ECX EDX ESI EDI EBX EBP**,
first-fit per pseudo in **creation order**; call-crossing values get ESI/EDI/EBX
in order (the B1 swap), byte-sized values lose ESI/EDI/EBP (B15 char homing). v2
predicts the exact assignment, names the transposed value, and proposes the ONE
creation-order edit — then confirms in ONE compile. Crucially it also **proves
when the residual is unreachable**: if the value that must move first is
`this`/a parameter/a call result, no local spelling reaches it — it's front-end
**handle-state (C1)**, and the answer is to converge the TU, not edit the body.
`--sweep` falls back to the v1 blind mutation search.

### include-set walls (the C1 flagship)
When adding an unused struct/type or a header edit non-monotonically moves an
*unrelated* function, it is C1XX front-end symbol-handle renumbering delivered
through the IL (proven: `docs/vc6/il-format.md`). Confirm with
`homm3 vc6 il-diff <A.cpp> <B.cpp>` — `IL DIFFERS` ⇒ front-end (match the include
closure); `IL IDENTICAL` ⇒ look at C2 state. There is no local body knob.

### body location (fixed 2026-08-14 — re-test old CAPPED verdicts)
Both solvers locate F's body with `homm3.vc6._source`, which now handles
constructors **with** member-initialiser lists, destructors, operators,
qualified `Class::method` definitions and `const`/`throw()` suffixes — the whole
constructor and member-function population was outside both solvers before this.
It also masks `#if 0  // @carcass` regions, which the old locator did not: it
returned the fenced `{ /* @stub */ }` first, so every mutation was a no-op and
the run reported a CAPPED verdict **it had never measured**. 102 source files
carry a carcass block. **Any CAPPED verdict recorded before 2026-08-14 on a
constructor, destructor, operator, or on a function whose file has a carcass
block, is unmeasured — re-run it.** A row with no body now says which case it is
(compiler-generated `??_G`, fenced carcass, defined elsewhere) instead of
"cannot locate". Gate: `homm3 vc6 check --locator`.

## Doctrine (do not violate)

- **The tools PROPOSE, never apply.** Every solver prints a diff for the matching
  agent to review against retail evidence. Record an applied result in the §5
  decision log; the solver itself must never auto-edit source.
- **In-unit fidelity is built in** — solvers diagnose off the real
  `build/objdiff/base/<unit>.obj` and compile mutants with the unit's exact
  `units.toml` flags, so their distances match the ratchet. Trust them; do not
  re-add `/I<src>` or a hand-rolled profile.
- **The VC6 compile is the sole verdict.** A model prediction is a hypothesis a
  compile settles; why-reg v2 always confirms with one real compile.
- **Don't grind a bounded wall.** `masked-equal`, encoder tie-breaks (SIB
  base/index, B18), and proven handle-state residuals are not source-local — the
  tool saying so in one compile is the *answer*, not a failure. Record it and move
  on (`match` skill: document, don't grind past 3–4 hypotheses).
- **rc convention:** 0 = agrees / exact, 1 = diverges / improved-not-exact,
  2 = error. Every invocation logs to `build/homm3_vc6.log`.

## Supporting verbs

- `homm3 vc6 argv --flags "<cl flags>"` — exact per-pass (C1XX/C2) argv for a
  command line (proves `/Ob2` is front-end-only, `/d2` reaches C2 verbatim).
- `homm3 vc6 oracle all --all` — the behavior-catalog probe corpus (23 probes);
  the model's regression gate.
- `homm3 vc6 check` — the census: behavioral gate (with a negative control) +
  distance-vs-objdiff consistency (informational).
- `homm3 vc6 ab run --fn <mangled>` — RTM-vs-SP3 C2 A/B (Track R). Only for a
  function whose sole residual is a `jb`/`jl`-style generation twin — Track R
  proved this is a tiny tail; the C2 generation explains none of the current
  walls.
- `homm3 vc6 atlas --regen` — regenerate the C2 TU/globals map (rarely needed).

## Reference (read the doc for the class you're in)

`docs/vc6/behavior-catalog.md` (the ~90 byte-verified behaviors, A/B/C/D IDs —
the shared vocabulary) · `eh-cleanup.md` (the cleanup-count rule + the
tree-wide transcript table) · `inliner.md` (the `/Ob2` rule + address ledger) ·
`regalloc.md` (the preference table + first-fit order) · `il-format.md` (IL
capture + the include-set verdict) · `driver-passes.md` (the CL spec table) ·
`c2-atlas.md` (C2 module RVAs) · `rtm-generation.md` (Track R). The full picture:
`~/.claude/plans/good-now-onto-the-lexical-allen.md`.
