# `why-branch` — the control-flow solver (v1)

`homm3 vc6 why-branch <src> --fn F (--against UNIT:FN | --against-src FILE)
[--json]` — the control-flow twin of `why-reg`. It diagnoses a **CFG /
branch-shape** residual (not a register binding) and runs a guided oracle
search over control-flow source spellings for the one that reproduces the
reference's jumps. Same contract as `why-reg` throughout: rc 0 = zero
divergence reached (already, or by a mutation, which prints as a diff for
reviewed application — the tool proposes, never lands); rc 1 =
improved-but-not-exact or nothing helped; rc 2 = error. Scratch:
`build/vc6/whybranch/{base,ref,mut}/`.

**v1 contains no reverse-engineered flow-graph model.** The real pinned
compiler is the oracle: every candidate spelling is compiled with the game
profile (`/O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS` + `/FAs` via
`cc_wrap`), F is sliced with `sema/_asm`'s llvm-objdump path, and the
branch-shape distance is the verdict. Mutants that fail to compile are
discarded by the oracle, never reasoned about. The boundary with the later
RE track is drawn at the bottom of this page.

Code: `scripts/homm3/vc6/flow_model.py` (solver + mutation library) and
`scripts/homm3/vc6/_flow.py` (metric + diagnosis), layered exactly like
`reg_model.py` / `_align.py`.

## The branch-shape distance

Register-BLIND on purpose — the complement of `_align`'s register-visible
metric. Three structural components (`_flow.distance`):

| component | what it is | reused machinery |
|---|---|---|
| kinds | per-block branch-kind skeleton (jcc/jmp/ret/fall/end + direction, block-index space) | `_align.flow_kinds` over `_asm.cfg` / `_asm.branch_kind` |
| tokens | ordered conditional-branch sequence, each `mnemonic#symbolic-target` (index of the first branch at or after the target — the `sema diff --branches` convention: uniform displacement shifts compare EQUAL) | `_asm.parse_ins` + the diff --branches token idea |
| rets | ret-count delta (the DUP-EXIT / merged-return signature) | — |

`distance = unpaired(kinds) + unpaired(tokens) + |ret delta|` (difflib
longest-match pairing, autojunk off). **0 = the branch shapes agree** —
whatever residual remains is register allocation / instruction selection,
i.e. `why-reg`'s domain, and the tool says so ("hand off to why-reg").
Streams clip at the first impossible mnemonic ((bad)/jecxz/loop* — inline
jump-table bytes); a clipped profile is flagged `partial` and diagnosed as
covering a prefix.

## Diagnosis taxonomy (D-classes → branch signatures)

Emitted by `_flow.diagnose`; catalog IDs are
`docs/vc6/behavior-catalog.md`'s. Heuristic mapping — the compile, not the
table, is the verdict on any fix.

| signature | finding | catalog |
|---|---|---|
| conditional branch counts differ | **structural** — a block not reconstructed, a folded if, an inlining decision (the `sema diff --branches` "COUNTS DIFFER" signal); loop-form and exit-merge knobs legitimately add/remove a compare site, so the search still runs | STRUCT |
| one side's back edge is a jcc, the other's a jmp to a top test | **loop-rotation** — rotated (duplicated guard) vs top-tested-unrotated; names which side is which and the `while (1) { if (!(c)) break; }` family | D1/D2 |
| base rets > reference rets | **exit-merge, DUP-EXIT** — we duplicate an exit the reference merges; respell as nested single-return (D5) or goto-into-shared-block (D4). The check_shipyard "8 branches, 2 rets" asymmetry | D4/D5 |
| reference rets > base rets | **exit-merge, reverse** — retail tail-duplicates; give each arm its own return | D6 |
| same counts + mnemonics, different symbolic targets | **topology** — jump-threading / cross-jumping territory; often NOT source-addressable (D3), sometimes the two-break `for(;;)` routing (D12) | D3/D7/D12 |
| indirect jmp on one side only | **dispatch** — switch (jump table) vs if-chain; case bodies emit in SOURCE order | D9 |
| setcc/sbb heavier on one side, branches heavier on the other | **branchless** — ternary selector / bool-flag spelling folds a branch (or the reverse: the TPickANumber clamp branches where a ternary would not) | D8/D13 |
| paired-position mnemonic flips | **flip** — signedness twins (jl/jb: a real source-type bug) vs polarity (opposite sense: arm order / fall-through path) | D8/D13 (type note) |

## Mutation library (v1)

Regex-guided site discovery over F's body only (same stance as `why-reg`;
libclang was rejected there for VC6-era-C++ fragility and the verdict
carries over). Capped at 48 candidates, deduped.

| class | rewrite family | catalog |
|---|---|---|
| loop-form | `while (c) {..}` → `while (1) { if (!(c)) break; ..}` and `if ((c) == 0)` twin / `for (;;) {..}` / `do {..} while (c)` / explicit goto-loop; and the REVERSE (top-tested → rotatable `while (inv)` / do-while) | D1/D2 |
| for-induction | index type short/int/long; descending `i-- > 0` for `i = 0; i < N; ++i` | D10 |
| merge-return | adjacent same-value guard returns → goto-INTO-the-shared-block (the path.cpp layout) and the `||` merge | D4 |
| nest-exits | flat chain of ≥2 same-value guard returns + final return → nested ifs with ONE textual fail return | D5 |
| ternary | `if (c) return A; [else] return B;` ↔ `return (c) ? A : B;`; `if (c) x=A; else x=B;` ↔ `x = (c) ? A : B;` | D8 |
| bool-flag | comparison-initialized flag declaration `unsigned char` ↔ `int`/`long` | D13 |
| case-order | adjacent case-block swaps + full reversal; only switches where every case block is break/return/goto/continue-terminated (fallthrough and stacked labels are never reordered) | D9 |

Two rewrites are not semantics-preserving in general and their labels say
"retail arbitrates": `do..while` (zero-trip) and descending induction
(iteration order). They stay in the menu deliberately — retail bytes are
the semantic ground truth in this domain, and every winner is only a
proposal until the matching agent verifies and applies it.

Not covered in v1 (documented non-coverage): nested-if → flat-chain (the
reverse of nest-exits needs brace-tree parsing), goto-into-block →
split-ifs reverse, loop bodies without braces, D6 retail-side
tail-duplication spellings (duplicate-the-mutation-per-arm), D12's
two-break `for(;;)` routing, D3 (provably not source-addressable — five
measured spellings collapse identically, per the catalog).

## Self-test evidence (real compiler, 2026-08-09)

Hermetic pairs under `build/vc6/whybranch/selftest/`; every run through
the pinned SP3 CL under Wine.

**D2 loop form — recovered, rc 0.**
```
$ homm3 vc6 why-branch build/vc6/whybranch/selftest/whybranch_rotated.cpp \
    --fn pump --against-src build/vc6/whybranch/selftest/whybranch_toptest.cpp
[distance]  5 ...   [diagnosis] STRUCT + D1/D2 loop-rotation (base ROTATED)
  unrotate loop: `while (1) { if (!(gRun)) break; .. }`    D2   -5   0  EXACT
  unrotate loop: `while (1) { if ((gRun) == 0) break; .. } D2   -5   0  EXACT
  unrotate loop: `for (;;) { if (!(gRun)) break; .. }`     D2   +0   5  no change
rc 0
```
The search independently reproduces the catalog's D2 claim: only the
`while (1)+break` forms unrotate; `for (;;)+break`, do-while and the goto
transcription still rotate/duplicate.

**D4 merged return — recovered, rc 0.**
```
$ homm3 vc6 why-branch .../whybranch_splitret.cpp --fn fetch \
    --against-src .../whybranch_mergedret.cpp
[distance]  4 ...   [diagnosis] flip (jge->jl) + D4/D5 DUP-EXIT (3 vs 2 rets)
  merge guard returns via goto-into-block                  D4   -4   0  EXACT
  merge guard returns via `||`                             D4   -2   2  improved
  nest 2 flat guard returns into one `return -1` exit      D5   -2   2  improved
rc 0
```
Ranking mirrors the catalog: goto-into-block exact; `||` and nesting
re-thread to the sunk form and only improve.

**D5 DUP-EXIT — recovered, rc 0.**
```
$ homm3 vc6 why-branch .../whybranch_flatchain.cpp --fn gate \
    --against-src .../whybranch_nestedchain.cpp
[distance] 12 ...   [diagnosis] STRUCT + D4/D5 DUP-EXIT (4 vs 2 rets)
                    + branchless (base's folded neg/sbb final gate)
  merge guard returns via `||` (gates c/d)                 D4  -12   0  EXACT
  nest 4 flat guard returns into one `return 0` exit       D5  -12   0  EXACT
rc 0
```
Two spellings reach branch-shape 0; `why-reg`'s finer grade arbitrates
between them.

**Honest no-help — rc 1, no crash.**
```
$ homm3 vc6 why-branch .../whybranch_nohelp_base.cpp --fn probe_gate \
    --against-src .../whybranch_nohelp_ref.cpp
[distance]  6 ...   [diagnosis] STRUCT (1 vs 2 branches - a guard the base lacks)
  collapse returns to ternary                              D8   +0   6  no change
[why-branch] no mutation moved the branch shape toward the reference ...
rc 1
```

**Hand-off path.** `whybranch_ifret.cpp` vs `whybranch_ternary.cpp` (the
d08 get_total pair) compile to the SAME branch shape — the ternary's
delta is register-visible only (`sub edx` vs `sub eax`, the d08 probe's
observable). why-branch reports distance 0 and hands off to why-reg
(rc 0). This is the designed division of labor, not a miss.

**Orbit-local negative (E11), measured.** A 3-case switch pair differing
only in case order (`whybranch_caseorder_*.cpp`, bodies with internal
flow) compiles to identical branch shapes — the listings differ by a pure
label permutation: in the COMPARE-CHAIN lowering VC6 lays the bodies out
itself, so the D9 source-order lever bites only in the jump-table lowering
(≥4 dense cases, exactly probe d09's setup). Corollary: case-order
permutations of shape-identical bodies are below this metric's resolution
by construction (register-blind); they surface at why-reg's grade.

`homm3 vc6 oracle flow --all` after landing: 10/10 d-probes PASS —
nothing existing was disturbed.

## Boundary with the later fg.c/lg.c RE model

v1 is deliberately model-free: it searches KNOWN explained-lever spellings
and lets the compiler judge. The later RE track (C2's flow-graph and
layout passes — the `fg.c`/`lg.c` source-path family visible in the
binary's embedded path strings) is the opposite bet: predict rotation,
tail-merge, cross-jump and layout decisions from the IL without compiling.
When that model lands, why-branch's diagnosis becomes a prediction
("retail's extra guard is the LICM-legality duplicate, D3 — no spelling
reaches it, stop searching") instead of a signature table, and the open
classes this tool cannot fix (D3 threading, D6 retail-side duplication,
A17 cross-jumped expansions) get an explanation instead of an honest rc 1.
Until then: this tool for reachable spellings, the census gates for rot,
and the catalog's "document, don't grind" doctrine for the rest.

## v1 limitations (honest list)

- Site discovery is regex-level: brace-less loop bodies, nested-if
  flattening, multi-line conditions and macro-heavy code are invisible.
- One mutation per candidate — no composition search (the catalog warns
  effects do not add; C1 state can move flow non-locally regardless).
- The distance treats kinds/tokens/rets equally; no weighting.
- Case-order detection needs the jump-table lowering AND shape-distinct
  bodies (see the measured negative above).
- `--against` an image-producer reference (capstone, no delinked object)
  clips at inline jump tables; branch kinds are producer-robust but the
  clipped tail is uncompared and flagged `partial`.
- Loop-form coverage is per-loop (first 3 `while` sites); nested-loop
  interactions are not modelled.
- **Constructors are unreachable.** The v1 body locator (shared with
  `why-reg`) demands a plain `fn(...) { ... }` definition, so any
  constructor carrying a member-initialiser list dies with "cannot
  locate the body of ...". Measured 2026-08-14 on the bottom-view
  family, where every plateaued row is a constructor and neither solver
  could be run at all. Until the locator accepts `T::T(...) : base(x) {`
  the whole constructor population of this tree is outside the solvers.
