# `why-branch` — the control-flow solver (v1)

For the project-wide inventory and controlled reductions, see the
[goto reconstruction audit](goto-audit.md). Its helper and nested-loop controls
show why a matching assembly join alone does not establish an original goto.
The follow-up also recovers dialog exit flags and switch-arm copies: shared
source tails can inhibit the compiler merge that ordinary per-arm statements
produce. Serializer helper boundaries can remove both gotos and old inliner
pins by preserving each caller's cleanup scope. The Windows message pump
also disproves a blanket goto-loop rule: an outer event loop with `continue`
is exact, while a nested `while (PeekMessageA(...))` rotates/hoists the import
and lowers both retained and expanded bodies. Creature-spell error handling
requires the combined positive outer scope, direct target rejections and
switch breaks; testing only the direct returns misses the exact form.
Map stamping similarly needs the complete DC local/reference model and its
canonical map accessor: together they remove the cover-search jump, remove an
unattested release-VERIFY and reach 100%. The artifact slot search is exact
when exhaustion returns inside the loop and explicit-slot validation occupies
an `else`; a loop-only rewrite had left that coupled scope unresolved.

The ordinary player-position helper gives another caller/retained-body control.
A selected-result flag in its reverse fallback scan preserves the exact helper
and both exact callers, while direct scan returns lower first-player setup.
Restoring that canonical call eliminates a separate forced-inline copy. Always
measure shared-header consumers when removing the obsolete declaration; even
an unused declaration can affect VC6 register allocation elsewhere.

The drawbridge example reaches the same conclusion from an exact starting
body: restoring ordinary `DoorCanBeLowered` and const `hexcell::hasArmy`
allows a positive gate guard and `else` to remove two jumps at 100%.
`isWinner` restores its `army::is` calls and first-scan result, eliminating
three jumps while retaining all 188 bytes. A failed direct-return probe alone
had missed both forms.

A common failure action can also have an ordinary breakable scope.
`NewfullMap::load` keeps 56.7217% with two `break`s from one `do/while(0)`
loading scope; direct returns at either site score 53.5994%. A successful
partial switch rewrite need not remove every remaining exit at once: default
and bonus arms can become structured while neutral cases still preserve a
separate compressed-table destination. Record that narrower limit explicitly.

A positive release scope resolves another apparent return-merging limit.
`border::main` becomes exact and loses all three gotos when the mouse-hit and
selected bodies are positive scopes and inactive handling retains its negative
arm. `iconWidget::main` uses the corresponding positive scopes and canonical
ordinary `setPalette`, removes five gotos, and reaches 99.9639%; only two
scratch-register operands remain. Direct-return probes against the old negative
release guard had missed both results. Keep the guard scopes in the family,
not just the spelling of their exits.

Switch labels also require independent semantic verification. The spell-work
audit found wrongly grouped immunity destinations and hero resistance confined
to one creature arm. Raw retail selector tables and DC calls establish the
correct routes; behavioral negative controls catch those mistakes independently
of the score. Restoring those routes and the original `IsMindSpell` header
accessor removes seven jumps and raises 88.5071% to 96.6018%.

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

## Body equality does not establish return width or branch destinations

The RMG gap predicates at `0x5b6320` and `0x5b6430` produce the same complete
263/262-byte bodies with either `int` or `unsigned char` return declarations.
Their retail callers test `al`; an `int` declaration instead makes those
callers test `eax`. Verify callers before treating an exact retained body as
proof of its return type.

Likewise, `rmgTerrainPainter::hasSeparatedNeighbours` (`0x5b6810`, prior role
`HasSeparatedNeighbours`) agreed in every line of address-masked assembly at
99.7458% while differing in three short-branch operands. Both versions contain
two identical false-return epilogues, but the branches at `+0x20`, `+0x52`,
and `+0x72` selected `+0x64` where retail selects `+0x44`. Native bool and
true/false literals did not change those destinations.

A shared `noSeparation` return inside the first empty-run scan closes all 132
raw bytes, including the mask-builder call. The initial full-ring failure and
the later wrap checks explicitly enter this block. Declare the unsigned
`direction` local without initialization before the initial scan, then assign
it before use; this makes the first jump legal without bypassing an initialized
declaration. VC6 still duplicates the zero epilogue at `+0x64` for the later
loop's fallthrough exit. The fix therefore preserves the two return blocks
while recovering their incoming edges. `paintPoint` stays at 99.5570% and both
worklist destructors remain exact. Compare resolved targets: equal instruction
and return counts do not establish this control-flow match.

## A byte-returning accessor can preserve bitfield extraction

`ScoreObjectPlacement` (`0x536bc0`) tests bit 27 with `shr edx,27` followed
by `test dl,1`, while a later condition on bit 25 tests the containing dword
directly. Reading the unsigned one-bit field in the first condition folds
to `test dword ptr [item+0x28],0x08000000` in the candidate. Returning that
field through an ordinary `unsigned char` member accessor preserves the
shift and byte test after expansion (84.2027% to 84.6788%).

This supports a narrow accessor boundary as a source hypothesis. It does
not establish the original method name or prove that every shifted flag
test came from a helper. Keep the canonical bitfield and verify the caller;
do not replace the record with a raw-word alias to force the extraction.

## Reused scalar variables can retain addressable homes across phases

The RMG placement-rule reader (`0x536560`) passes object type, subtype and
terrain locals by reference to vector insertions, then reuses their same
three stack homes in a later prototype-binding pass. Declaring new locals
for that pass lets VC6 strength-reduce the object-type stride and keep the
terrain/subtype values in registers. Reusing the parsing variables restores
the retail address calculations and loads/stores (89.9878% to 97.5804%).

This is a lifetime hypothesis supported by both the earlier address-taking
and the later home reuse. Merely seeing two values share a stack offset is
insufficient: unrelated locals can also share a slot. Keep the actual source
operations and references; do not add dummy address escapes or volatile.

## Integral conversion can separate a loop index from a vector argument

The Pandora's Box spell factory (`0x534520`) scans spell traits with a
136-byte induction stride and copies the integer spell ID to a temporary
before the expanded `vector<int>::push_back`. Passing an `int` loop variable
directly exposes its address to the insertion and leaves a multiply-derived
trait address in each iteration (85.8423%). A `long` loop variable converts
to the vector's `int` element type, restoring both the copied argument and
retail's induction stride; the entire 615-byte function matches exactly.

A separate `int spellId = spell` passed to `push_back` is also exact. Writing
`int(spell)` with an already-`int` index is byte-identical to the direct
reference form under VC6 and does not create the required boundary. Inspect
the actual caller's argument copy and loop addressing before choosing a
conversion or a separately scoped payload; neither form proves the original
source spelling by itself.

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

The Complete-only RMG repair method (`0x5b5440`) confirms this distinction
with a nested circular scan and an exit from both loops. Retail `+0x4de`
advances at one header. An assignment-condition outer `while` duplicates
that test; an explicit `while (1)` advance/test/goto restores the entire
71-block CFG, whereas equivalent `for (;;)`, `do (1)` and labelled-header
forms still rotate. A word-width diagonal local also recovers the full-register
copy at `+0x511`. The combined source change raises 90.0961% to 91.3390%
without collateral. The 60 outer-loop/receiver/width candidates and 60 top-ten
inner-loop refinements in `generate-rmg-gap-scan-hypotheses.py` all compile;
the latter add no gain. The native oracle exhausts all nonempty ring masks.
This extends the measured D2 behaviour to a multi-loop exit; it does not make
equivalent source loop forms interchangeable in the VC6 oracle.

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
- ~~**Constructors are unreachable.**~~ **CLOSED 2026-08-14** — see
  "The body locator" below. Both solvers now run on constructors,
  destructors, operators and qualified member definitions.
- The locator finds the *definition*; the mutation library still only
  rewrites the BODY. A member-initialiser list is never mutated, so
  base-class-argument and member-init-order levers are not searched.

## The body locator (shared with `why-reg`)

`scripts/homm3/vc6/_source.py`. Both solvers must answer one question
before they can search — "where in this .cpp is the body of the function
the .obj calls F?" — and until 2026-08-14 they answered it with a raw-text
`re.finditer(rf"\b{fn}\s*\(")`. Two defects, both measured on
`src/bottomviewsubwindow.cpp`, both now covered by the `locator` gate of
`homm3 vc6 check`:

1. **Name.** `why-branch` searched the MANGLED symbol verbatim — a string
   that never appears in source — so its guided search could only run on an
   already-demangled `--fn`. `why-reg`'s decoder handled only
   `?name@Class@@…`, so every `??`-special name decoded to garbage
   (`??0TBottomViewKingdom@@QAE@PAVheroWindow@@@Z` → `QAE::?0TBottom…`).
   **The tree's entire constructor, destructor and operator population was
   outside both solvers** — 29 of the 373 plateaued rows are constructors
   and 5 are destructors, and solver reach on them was exactly 0.
2. **Carcass.** Because the search ran over raw text it returned the FIRST
   definition, and this tree fences unreconstructed rows in
   `#if 0  // @carcass` blocks near the top of the file, each a
   `{ /* @stub */ }`. `sum_mobility` located to line 155 (the stub) instead
   of line 1269; `animate` to line 100 instead of 1299. Mutating a stub is
   a no-op, so the search reported "nothing helped" — **a CAPPED verdict it
   had never measured**. 102 of the tree's source files carry a carcass
   block; 25 rows resolved to the wrong body and 60 more resolved to a
   fenced stub that has no real body at all.

The locator now decodes the MSVC special names it needs (`??0`/`??1`,
the `?2`…`?_V` operator codes, and the `??_G`/`??_E`/`??_7` family, which
it reports as *compiler-generated — no source body*, which is an answer
rather than a failure), searches a length-preserving MASKED view of the TU
(comments, string/char literals, every directive line and every dead
`#if 0` / `#if 1`-`#else` region blanked to spaces), and walks the real
definition grammar:

```
NAME ( params ) [const|volatile|throw(..)|__decl…] [: a(x), b(y)] {
```

so qualified member definitions, member-initialiser lists, destructors,
operators and cv/exception/calling-convention suffixes all resolve, while
declarations and call sites (`new Foo(...)`) are still refused. When there
is no body, `explain_miss` says WHICH of the three cases it is
(compiler-generated / fenced carcass / defined elsewhere) instead of a bare
"cannot locate", and `why-branch` reports it as a diagnosis-only rc 1
rather than dying at rc 2.

Reach on the 373 plateaued rows, measured before/after:

| symbol kind | rows | v1 located | now |
|---|---|---|---|
| constructor (`??0`) | 29 | 0 | 17 |
| destructor (`??1`) | 5 | 0 | 3 |
| ordinary member/free | 155 | 154 | 141 |
| flat / C name | 183 | 9 | 2 |
| compiler-generated | 1 | 0 | 0 (correctly) |

The two drops are the fix, not a regression: all 20 are carcass stubs the
v1 locator accepted as real bodies. Every one of the 60 tree-wide rows the
new locator "loses" was verified to be a `{ /* @stub */ }` inside an
inactive region.

Remaining v1 limits, documented not silent: template-id scopes
(`?$vector@…`) are declined rather than guessed (those rows are
header-defined, so "not located" is the right answer), and `#if FOO` /
`#ifdef FOO` are treated as ACTIVE — only the literal constant idiom is
evaluated, because guessing a macro's value would hide a real definition
and the failure mode being fixed is the opposite one.

### Calibration on six real constructors (2026-08-14)

Not synthetic pairs — six recorded plateaus, run through both solvers
against their delinked retail objects. All six previously died at
"cannot locate the body".

| row | fuzzy | `why-branch` | `why-reg` |
|---|---|---|---|
| `TBottomViewKingdom::TBottomViewKingdom` | 94.06 | 65, STRUCT + **D6** (retail 2 rets vs our 1), 0 sites | 201 → **99** `which->active` as a long local (B13) |
| `TBottomViewHero::TBottomViewHero` | 96.52 | 3, D8/D13 flip → **2** short induction (D10) | 93 → **65** `who->stats[i]` as a char local (B15) |
| `TBottomViewTown::TBottomViewTown` | 94.31 | 2, D8/D13 flip — **CAPPED**, 5 sites, none help | 139 — **CAPPED**, 13 sites, none help |
| `TBottomViewResourceMessage::…` | 91.30 | — | 40 → 39 (`volatile`, cosmetic) |
| `type_combat_sub_window::type_combat_sub_window` | 89.4751 | **0 — branch shapes agree** | 403 — **CAPPED**, 1 site, worse |
| `TCombatControlSubWindow::TCombatControlSubWindow` | 94.3558 | **0 — branch shapes agree** | 28, B10/B14 scratch preference — **CAPPED** |

The two combat rows are the point of the exercise: `why-branch` returning
**0** turns "the residual is C2 processing-order state on a
non-source-nameable value" from an assertion into a measurement. The
control flow provably agrees with retail; `why-reg` then finds no source
knob that moves the binding; and the surviving `why-reg` diagnosis on
`type_combat_sub_window` is a frame-size delta (`sub esp,0x10` vs `0xc`)
plus a callee-saved role swap — a value promoted on one side only. Nothing
in the source spells that. The prior verdict stands, now measured.

**Every candidate the solvers proposed was rejected by objdiff**, and that
is itself the calibrating result: the masked distances improve while the
bytes do not.

| proposal | reg/flow distance | objdiff |
|---|---|---|
| Hero: `who->GetPrimarySkill(i)` (the DC-attested inline accessor) | B15 −28 | 96.5197 → 96.5145 |
| Hero: `signed char stat = who->stats[i];` clamp | B15 −28 | 96.5197 → 96.5145 |
| Hero: `short i` induction | D10 −1 | 96.5197 → **96.2632** |
| Kingdom: `__int64 active = which->active;` | — | 94.0575 → **83.7959** |
| Kingdom: `long active = (long)which->active;` | B13 −102 | 94.0575 → 93.8980 |

Read that as the standing rule made concrete: the solvers' distances are
proxies and **retail's bytes are the arbiter**. On these rows they
corroborate the previous lane — the bottom-view wall is upstream of both
solvers (inline-candidate site count), which is exactly what
`why-branch`'s STRUCT/D6 finding on Kingdom says independently.

### First sweep of the unlocked population (2026-08-14)

The fix makes **20 plateaued rows** newly solver-reachable (17
constructors, 3 destructors), spread over 17 units — from
`bitmap816::~Bitmap816` at 99.97 down to
`TQuickHeroWindow::TQuickHeroWindow` at 86.84. `homm3 vc6 diagnose`
(no compile) routes twelve of them:

- **11 of 12 → inliner (`predict-inline`).** The site-count wall the
  previous lane proved on `TBottomViewKingdom` is a **population-level**
  property of this tree's constructors, not a bottom-view quirk. That is
  where the campaign's remaining constructor mass actually sits, and it
  is not a `why-reg`/`why-branch` problem.
- `palette::TPalette24` (87.07) → register-homing. `why-reg --model`
  settles it in **0 mutation compiles**: base binds `#0:esi@5 #1:edi@12`,
  retail `#0:edi@5 #1:esi@11`, same slots and same order, and the
  transposed value is **`this`** — not source-nameable, so it is C1
  handle-state, **CAPPED**. A 31-slot divergence bounded without a single
  compile is the model doing exactly its job.
- `button::textButton` (88.44) → a live lead: `#0 jbe->jae`, *different
  condition computed* at the FIRST branch. Neither an arm order nor a
  rotation — an inverted comparison or swapped operands in the guard.
  Source-addressable; the v1 mutation library has no site for it.

So the routing answer for the unlocked population is: **do not point
`why-reg` at them**. Point `predict-inline` at them, and treat the
`textButton` guard as the one spelling lead the sweep turned up.

## The FLOW bucket is two classes, and one of them is not a flow problem (2026-09-06)

Polish lane 23's first-divergence classifier split 523 sub-100 rows by the
KIND of the first divergent byte; 56 landed in FLOW ("same instructions,
different block terminator"). Running `sema diff --summary` over all 56 and
reading the **branch/ret counts** rather than the terminator splits them
cleanly, and the split says which tool to point at them:

- **ret-count delta (14 of 56)** — retail has MORE `ret`s than we do
  (`is_computer_action` 6 vs 10, `iconWidget::Main` 17 vs 16,
  `border::Main` 8 vs 7, `CHotspotWidget::Main` 5 vs 4, `GetMobility` 3 vs
  2, `ProcessMapSelect` 13 vs 12, `GetSoundId` 73 vs 71, …). This is the
  **tail-merge family** (it was called the "tail-merge GENERATION family"
  until 2026-09-06 - see below): our SP3 cross-jumper merges epilogues
  retail's object left duplicated, and the merged copy then SINKS,
  which is what turns a short backward `jcc` in retail into a long forward
  one here. Every row in this group already carries a residual note with
  three to five rejected exit shapes. `why-branch` reports D6 on them and
  finds no catalog mutation. **Do not spend a lane on this group.**

  **The name was wrong and the generation is now excluded (2026-09-06).**
  Track R A/B'd this whole roster against VC6 RTM in both passes - the
  back end alone (C2 12.00.8168) and then the front end too (C1XX
  12.00.8168 + C2 12.00.8168, `genab run --gen rtm-fe`, all 146 units).
  `border::Main` 31+26, `CHotspotWidget::Main` 29+22, `hero::GetMobility`
  210+14, `ProcessMapSelect` 150+57, `GetSoundId` 440+8, `CheckEndGame`
  169+2, `handle_artifact_click` 22+6, `OnTCP` 52+6,
  `HighScoreWindowHandler` 126+28, `CombatOptionsWindowHandler` 156+22,
  `DisplayVCWinLoss` 110+64, `TTavernWindow::SetRolloverText` 65+0,
  `advManager::MoveHero` 678+150, `iconWidget::Main` 46+30,
  `is_computer_action` 44+50, plus `ValidAttack` 70+59, `AppWndProc` 29+0,
  `VideoClose` 2+20 - **identical scores and identical bytes on both
  sides, `sp3_vs_rtm == 0` for every one**. Retail's duplicated epilogues
  are not an older compiler's output; they are a cross-jumper decision our
  model does not reproduce (docs/vc6/rtm-generation.md §6).
- **branch-count delta (19 of 56)** — the terminator differs because one
  side has WHOLE BLOCKS the other lacks, and reading `--calls` positionally
  always names an /Ob2 site, never a statement. Retail expands
  `GetArmyName` at `CombatMessage`'s FLY arm and calls it at the WALK arm
  (5 branches); retail calls `basic_string::append` at four
  `get_morale_description` rungs and expands three; retail leaves
  `vector<long>::size()` out of line in `find_attack_hexes`' reallocate
  path. **This group is `predict-inline`'s, not `why-branch`'s** — and
  under the current cleanliness floor (inline-depth pins 355, falling
  only) it is closed, because the only levers that move it are a statement
  pin or a caller-size change.
- **`clean` / `flips POLARITY` with both counts equal (23 of 56)** —
  register-homing or block layout; `why-reg` territory.

The corollary for the OPCODE bucket (37 rows) is the same shape. A scan of
all 523 first divergences for the mnemonic pairs that name a TYPE
(`jb`/`jl`, `ja`/`jg`, `movzx`/`movsx`, `sar`/`shr`, `div`/`idiv`,
`mov`/`movzx`) returns exactly **one** hit tree-wide
(`readMapLayer`, `mov` vs `movsx`). The OPCODE bucket is therefore NOT a
signedness bucket; its members are `lea`-vs-`add` register permutations,
`setcc`-vs-branch bool materialisation, and rep-stos-vs-loop expansions.

**Where the type facts actually are: the const-reference ARGUMENT, not the
compare.** `_cpp_clamp` is declared `const int&(const int&, const int&,
const int&)`, so a `long` lvalue needs a conversion and therefore a THIRD
stack temporary, while an `int` lvalue binds its own home and needs only
two. Counting retail's temps at such a call site reads the local's declared
type straight off the bytes: `hero::GetLuck`'s `luck` is `long`
(87.7439 -> 88.0662, twelve bytes of tail recovered and two dead
write-backs removed). Count the temps before assuming a plain `int`.

## An expanded comparison helper can determine arm placement

`DrawIrregularZoneBoundary` (0x53bff0) placed terminal marking before
subdivision when the point equality checks were written as scalar
comparisons. Reversing the predicate, reversing source arms, and using
nested comparisons with `continue` did not restore retail's placement.
Writing the comparisons through `TPoint::operator==` changed that layout:
38.3069% became 92.8168%, with all branch targets agreeing. Defining
`operator!=` through `!(*this == other)` and using the negative guard was
byte-neutral at that checkpoint. Subsequent type and lifetime corrections
closed all 555 raw bytes without an inline directive.

The helper boundary therefore matters even when its final instructions
are ordinary coordinate comparisons. This is compiler evidence for the
candidate surface, not proof of the original RMG declaration: the DC
corpus has no identified counterpart, and its retained `type_point`
comparison methods concern a different packed coordinate type.

## A separate case can preserve the switch table despite an equal default

`ReadRmgTemplateZones` (0x538480) dispatches on `tolower` of a template
field. Retail's table covers `'a'`, `'n'`, `'s'`, and `'w'`; `'a'` and the
default both assign strength 3. Grouping `case 'a': default:` in source
removed `'a'` from C1's switch and produced a three-test comparison chain.
Giving `'a'` its own assignment and `break` preserves the four-case table;
the emitted table still shares the identical destination with default.

The combined switch/town-flag correction raised 86.8319% to 92.9708%.
The second correction wrote the flag's 0/1 assignments inside the two arms,
matching retail's immediate byte stores instead of materializing a boolean
register for a common store. A positive row-validation scope then raised
the reader to 96.9468%; its remaining deltas lie in vector insertion and
cleanup. These are retail/VC6 controls, with no DC RMG source counterpart.

## Returning a byte predicate directly can preserve its caller's comparison

`TRmgTerrainPainter::NeedsTerrainRepair` ends by consulting
`HasSeparatedNeighbours`, whose retained body returns only 0 or 1. Returning
that byte result directly after the other guards makes both painter cleanup
expansions use retail's `cmp al,bl`, with BL already zero. Returning it through
`&&`, explicitly comparing it with zero, or writing a final conditional 1/0
return instead produces `test al,al`. Changing the wrapper's return type to
native bool alone is neutral. The direct byte return closes all 549 bytes of
the brush destructor (0x5b72f0) and all 521 bytes of the painter destructor
(0x5b76f0), with their calls and branch destinations independently resolved.

These expressions agree because the retained nested predicate has a proven
0/1 range. This control does not justify removing normalization from an
arbitrary byte-valued function. The helper boundaries remain ordinary and
shared; no inline directive is involved.

The same checkpoint corrected the plane-view map's shared painting interface
and constructor statement order. Using the canonical `GetMapItem(0, 0,
level)` for the plane offset leaves `CreateRiver` at 39.066925%, below its
39.6178% peak. Its MAX/history remain intact for later caller-specific work.
`RepairWaterZoneBorders`'s separate bounds-aggregate finding is recorded in
[regalloc.md](regalloc.md#6g-a-bounds-aggregate-preserves-the-retail-stack-frame).

## A guarded do loop can preserve a forward exhaustion exit

The first land search in `RepairWaterZoneBorders` (0x53fcb0) has two exits:
finding a usable tile and exhausting the row. A conventional `for` or
`while (x < limit)` emits a backward `jl` followed by a forward `jmp` at
the exhaustion check. Retail instead uses a forward `jge` at 0x53fe49
and a backward `jmp` at 0x53fe4b, with the same preceding increment,
comparison, and coordinate store.

An entry guard followed by this form preserves that routing:

```cpp
x = first;
if (x < limit) {
    do {
        if (usable(x)) {
            found = 1;
            break;
        }
        ++x;
        if (x >= limit)
            break;
    } while (1);
}
```

The real candidate retains the map accessor and full eligibility predicate;
`usable` above only abbreviates the example. This recovered the two branch
destinations without changing the 0x84 frame or local homes, raising the
function from 96.921875% to 97.04883%. All CFG edges now agree; clamp and
map-view register differences remain. The RMG source has no DC counterpart,
so the spelling is supported by retail and the VC6 control, not a line table.

An explicit top exit inside `for (;;)` was byte-neutral. A top-tested
`while (1)` also changed the surrounding loop arrangement and induction
(89.92383%), so that control does not invalidate the guarded bottom exit.

## A folded search flag can determine fallback placement

`TraceZoneBoundary` (0x53c390) searches a cyclic edge list before choosing
between a rectangle fallback and the ordinary boundary walk. A return inside
the search sinks the rectangle after the ordinary walk. A `bool found = false`
with a success assignment and break, followed by `if (!found)` after the
`do/while`, reproduces retail's backward search branch and rectangle
fall-through. VC6 removes the flag's stores and test entirely. With identical
point-copy expressions, this changes 60.27% to 86.10%; the remaining mismatch
is mostly inside later vector allocation paths. A direct goto was byte-flat
in the earlier control, and nesting the success body falls to 29.83%.

Retail proves the branch topology, while the flag is a reconstruction
hypothesis that produces it naturally. An absent flag in optimized assembly
does not rule out a source flag. Test the complete search and post-search
control relationship before attributing fallback placement to a compiler
generation or an unavoidable layout decision.

## An early return can select the shared epilogue's position

`THeroScreenWindow::windowHandler` (0x4dd2d0) kept its shared consume-return
at the end of the dispatcher, although retail places it immediately after
mouse movement at +0x79. Restoring Dreamcast's deferred `exitFlag` alone did
not fix this. The decisive additional source fact is the unchanged-hover
**early return** before the status update (DC hero.cpp:3507..3508). Replacing
the inverted conditional around the update with that guard places the shared
return at +0x79 and restores all fifteen direct `normalDialog` calls. The
six help arms had previously jumped to a shared dialog-call tail.

The corrected caller also rereads the selected army slot after `updateArmies`
and branches between two explicit status broadcasts. Guarding the later
status-bar refresh with `!rightMouse`, proved by both DC 3822..3824 and retail,
restores the single shared status-update call. The real hero-only build banks
78.6458% versus the old 75.4051% MAX. All four army-refresh calls now survive.
The 0x144 versus 0x14c frame and artifact-click path merging remain unresolved;
the repaired early return does not imply that the whole function is exact.

## Recover indexed expressions before preserving strength-reduced counters

`advManager::setEnvironmentOrigin` (0x4183d0) reached 100% from 75.4080%
by restoring two Dreamcast source facts. The sound-priority assignment occurs
in both arms of the reset conditional (advmgr.cpp:9800 and 9803), and the
ring scan computes each coordinate from the original point, priority, and
shared index (9822..9829). The previous source manually maintained four
boundary coordinates and four edge cursors to imitate retail's increments.

VC6 derives those running counters itself from the indexed expressions.
Restoring only the expressions gives 97.7264%; restoring only the separate
priority stores gives 77.6667%. Together they reproduce all 27 retail blocks
and the 581-byte function. C2 merges the two priority stores while holding
0x7f in EBX and spilling the first loop's count. A named sentinel constant
on the flattened source had been byte-neutral, so that earlier result did
not establish an unreachable register assignment.

## Check a byte return before duplicating the surrounding tail

`LossConditionStruct::checkForDefeatedHeroLoss` (0x5f2a40) used a duplicated
ordinary-loss tail to compensate for poor block placement. Deleting just the
duplicate measured 10.6676%, which had been taken as a reason to keep it.
Retail instead rejects other loss types and returns the hero-id comparison
through `sete al`. An early rejection followed by a named `unsigned char`
result restores that return lowering and reaches 81.8182% with one tail.

Binding the artifact components by reference before their loop also restores
retail's stable table address, bringing the active TU build to 82.0170% from
75.8636%. The byte-result control with positive type-test nesting measures
11.2784%; an early rejection with a direct bool return measures 80.9943%.
The result type and the guard both matter. Local-scope controls are byte-flat.
The remaining shared-return and block-placement differences are unresolved;
the simplified tail is not a claim of an exact function.

## An expanded helper retains source structure that flattening loses

`TMultiPlayerWindow::onTCP` (0x5113f0) reached 100% from 75.9952% by calling
its existing ordinary `initRemote(MP_TCP, 0, 0)` helper, as Dreamcast line
1885 proves. The previous caller copied the helper's protocol assignment,
two initialization guards, capabilities query, and timeout assignments into
its own body. Its instructions matched individually, but the connection
failure dialog and return were sunk to the function's end.

VC6 expands the real helper and places that failure block between the
`textWidget` constructor's join jump and null-allocation arm, reproducing
retail. Restoring the existing `widget::show` calls and Dreamcast's nested
address-query/widget-existence checks is byte-neutral. Identical emitted
operations do not make a flattened helper equivalent for compiler layout;
restore the proven call before blaming the compiler generation.

## A container scope can free the loop-counter register

`type_skill_quest::doProgressDialog` (0x56dd60) held zero in EBX and spilled
its four-iteration counter. Retail uses EBX for the counter and deletes the
resource vector without clearing its three pointers. An inner vector scope,
ending before the lifetime-extended dialog string, removes those stores and
restores the 0x30 frame and register counter: 76.2135% becomes 94.2360%.
Declaring the skill cursor before the vector restores the initialization
schedule and reaches 96.6180% in the active TU build.

The same scope repair had improved the artifact-quest sibling. Neither
changes destruction order. An indexed loop within the skill dialog's new
scope measures 82.5169%; a named string is byte-identical to the reference.
Returned-string access and cleanup registers remain different from retail.

## Distinct return widths can keep an early exit ahead of register saves

`TNativeTerrainObjectFilter::accepts` (0x5141b0) reached 100% from 77.3913%
with an unsigned-char return and a direct logical tail. Retail's initial
slot-category rejection clears only AL and returns before saving ESI/EDI.
The later bitset-test/count conjunction materializes a full-width logical
result. The previous int declaration and explicit `if (...) return 1;
return 0;` let C2 merge the false returns, hoist a register save, and split
the bitset test's memory operand around an early pop.

Changing only the return type measures 77.0652%; the direct conjunction
with the byte return reproduces all 110 retail bytes after relocation
normalization. A byte-return function can contain `mov eax, 1` and
`xor eax, eax` for a logical expression as well as `xor al, al` for a literal
early return. The early path provides the discriminating ABI evidence.

## An explicit zero contribution can preserve a separate return path

`town::getLegionBonus` (0x5bf810) reached 100% from 81.7262% by calculating
fortification growth as `growth`, `growth / 2`, or an explicit zero before
adding base growth and halving the result. VC6 keeps the contribution in
EAX and base growth in EDI, and duplicates the common tail into each arm.
The no-building path retains a dead `xor eax, eax` from its zero assignment.

Omitting that final zero arm scores 78.63095%, even though the earlier
initialization makes it semantically redundant. Seeding the accumulator
with base growth before testing the buildings scores 81.7262%. A dead zero
and duplicated exit can therefore preserve an explicit source alternative;
neither alone establishes an uncontrollable register-allocation limitation.

## A retained reference can perturb layout after helper restoration

`hero::getMobility` (0x4e4990) reached 100% from 81.7677% by restoring the
Dreamcast-proven ordinary Navigation and Logistics helpers, followed by the
two `towns[t]` lookups in its Lighthouse condition. The helper boundaries
alone recover retail's backward land-to-AI join and two returns, reaching
91.07742%. The retained `town&` still changes register allocation throughout
the function; the separate indexed expressions produce the exact result.

With the same declarations, Navigation alone scores 92.03226% and Logistics
alone 83.97419%. Their combined lower score with the town reference is not
evidence against either proven helper. The earlier duplicated-tail and goto
probes operated on flattened helpers and could not recover the join. Land
movement is loaded before the Logistics call, as Dreamcast lines 5893/5895
show; the helper returns the complete factor including its additive one.


## Check loop entry and real operands before diagnosing register allocation

`game::transmitSaveGame` (0x4cafd0) had matching branch counts but used a
`do/while` where DC game.cpp:10382 tests `done` before entering the loop.
Restoring `while (!done)` repairs the retail branch directions and cleanup
placement. The combined null-message/timeout predicate at line 10391 is
byte-flat, but retains the evidenced source boundary.

Two semantic errors had survived the earlier register-allocation diagnosis:
the status guard used suspended (2) instead of active (1), and compression
failure called `File::deleteFile` instead of `fileError`. The wrong error calls
score identically when relocation differences are ignored. Read the named
call sequence and unmasked operands, not just the percentage. Retail and DC
also both compute the unusual `totalBlocks % fileSize`; substituting the
usual file-size remainder is not a reconstruction.

The corrected loop, operands and earlier `done` initialization bank 85.0958%
through normalized production objects, from 82.2198%. Retaining DC's complete
transfer-local initialization order gives a lower current score, with that
peak preserved. Its remaining 12-byte frame excess and register spills are
still open; equal branch counts had not proved that source control flow was
already correct.


## A decreasing counter can represent a different direction

`combatManager::mirrorImage` (0x5a6c70) reached 100% from 82.3404% after
restoring two expressions visible in the Dreamcast source mapping. Its search
selects `dirCount` when the source army faces 1, and `5 - dirCount` otherwise
(spells.cpp:4604..4607). Retail carries both induction values and chooses one
before the exclusions. Treating the decreasing counter as compiler-generated
loop bookkeeping had hidden a behavior bug: the reconstruction always searched
forward. Restoring direction selection reaches 86.1216%.

The animation computes `delta * (16 - frame) / 16` at DC lines 4685/4686.
Writing explicit running offsets instead kept additional values alive; allowing
VC6 to derive its own steps reproduces all 57 retail blocks and reaches 100%.
The source also jumps out of the search loops to a separate placement block.
Restoring that boundary and the canonical validity/column/front-offset helpers
is byte-flat, but preserves the positive source evidence. No direction reversal
is acceptable without establishing which facing selects it.


## Explicit early returns can restore register lifetimes

`combatManager::placeShooter` (0x422060) reaches 100% from 82.4884% by
restoring DC's combined entry condition and its two explicit action-8 returns
(ai.cpp:2191/2193 and 2255/2257). A shared `goto wait` produced the right broad
behavior and branch count, but kept `this` in ESI and spilled the neighbor
counter. The source returns let VC6 allocate `this` in EBX, reuse that register
for the count, and save ESI only when entering the search.

The combined condition alone scores 82.2093%; the returns are needed with it.
Canonical `getHex`/`Is` calls, occupied-neighbor-first source order, and the
Dreamcast best-hex assignment order are retained; each is byte-flat in the
corresponding controls. Matching aggregate branch counts had not established
that the source control-flow form was already correct.

`mouseManager::setPointer` (0x50cca0) independently demonstrates the same
source-exit effect. DC mousemgr.cpp:449-453 returns separately for three entry
guards, then 479-493 returns after clearing busy state for a negative or
unchanged frame. Restoring all five returns raises 82.9333% to 100%: VC6 merges
the lock destruction and busy cleanup, but now shares zero in EBX throughout
the 224-byte body. Restoring only the entry guards gives 83.1333%, whether they
are separate or combined. The prior named-zero experiment was 78.87%.

The DC `Enable`/`Disable` header bodies only read `DisableCount` in this build.
Their canonical calls are preserved; the discarded results compile away and
leave the 100% bytes unchanged. Read these tiny bodies before assuming their
names imply mutations. The PC sprite disposal remains independently proven
by retail.
