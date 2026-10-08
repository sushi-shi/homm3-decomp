# C2/C1XX state that unrelated edits disturb

A function's bytes can change when nothing it contains, and nothing it
calls, has changed. The ledger shows this as CUR below MAX. This page lists
the compiler state that such unrelated edits move, and the tools that
read it, set it under IL replay, and compile a unit 1-to-M: once, into
every assembly each of its functions can take.

"Unrelated" means the edit touches neither the function nor its callees.
Examples: a declaration added to a header, another function inserted or
removed, or a definition moved.

Three inputs move. Each entry below gives the unrelated edit, the compiler
state it changes, and the effect.

## 1. Phase flag (C2 `.bssbe` 0x9f120)

**Edit → state.** Changing what C2 compiles before the function changes
the phase flag it inherits.

**State.** The per-function driver `0x13615` writes the flag. Its one
caller is `0x683c7`, and the driver returns to `0x683cc`.

* The only read is at `0x5b11` in `0x5739`, before the next function's
  driver starts.
* The bss initial value is 0.
* Across 1,105 functions in six units (drawing, mapcell, army, initialize,
  philai, singleselectionwindow), every driver run left 1.
* So a function reads 0 only if it is the first function in its unit
  that goes through the global optimizer. That includes out-of-line
  definitions, `_$E` dynamic initializers where their object is defined,
  and generated destructors. Inline-only helpers, unreferenced statics and
  `#pragma optimize("g", off)` bodies do not count.
* The mechanism (driver stages, the reader's decision) is documented in
  [phase-flag.md](phase-flag.md).

**Effect.** With the flag at 1, stage-0 tuple simplification in `0x5739`
folds conditional branches whose compare outcome is known (see
[phase-flag.md](phase-flag.md)), typically from inlined helpers with
constant arguments. A first-optimized function keeps those branches, which
changes instruction selection, order or registers. Inlining decisions are
never affected.

**Census** (`homm3 vc6 phase-census`): all 150 non-RMG units, 8,769
functions, each replayed with the flag at 0 and at 1.

| | functions |
| --- | ---: |
| phase-sensitive (bytes differ between 0 and 1) | 281 (3.20%) |
| … `_$E` dynamic initializers | 153 |
| … `std` library bodies | 32 |
| … game functions | 96 |
| … … currently non-exact | 27 |

What the flag changes in the 96 game functions:

| change | functions |
| --- | ---: |
| instructions differ | 53 |
| operands only (registers or stack slots) | 26 |
| operands and order | 14 |
| order only | 3 |
| call count (inlining) | 0 |

**What retail needs.** Each phase-sensitive game function was compared
with its delinked retail copy:

| | functions |
| --- | ---: |
| retail = the phase-1 assembly | 62 |
| retail = the phase-0 assembly | 1 |
| retail is neither (other source differences) | 28 |
| not in the retail target | 5 |

Our current order agrees with retail for 60 of these and disagrees for 3.

**Source-order clues.** Phase 1 means something was compiled before the
function in its retail unit. Phase 0 means nothing was, unless some
function left 0.

| function | retail needs | ours | clue |
| --- | ---: | ---: | --- |
| `combatManager::showCreatureSpellError` (drawing, first in the unit) | 1 | 0 | Retail's drawing range opens with the ten `terrain.h` mask initializers (see below), as Dreamcast's drawing.obj does. Including `terrain.h` gives 100. |
| `loadSeerHutTextColumn` (seerhuttext, 99.96%) | 1 | 0 | Retail opens the unit with the initializer pairs of the two column tables and the name vector, and the generated constructors and destructors, all before `0x56c120`. Defining the tables above the loader reproduces those sizes and that order, and gives 100. |
| `aiEnterTown` (philai, 99.96%) | 0 | 1 | Retail has it 14th by address, after `considerGarrisoning` (`0x525200`), yet retail needs it to be the unit's first globally optimized function. Either the 13 retail functions before it did not go through the global optimizer (for example, they sat in a `#pragma optimize("g", off)` region), or retail's address order is not its compile order here. Open. |

**Terrain-mask runs open the following TU.** Retail has the run of ten
`terrain.h` mask initializers (sizes 89, 96, 97, 95 × 7) 89 times, almost
always right after a 32-byte `ctype<wchar_t>::id` guard. The run is the
head of the TU that follows, not the tail of the one before:

* our objects put the ten first and the guard last (initialize, drawing);
* 58 units have a run directly before their first retail function, and 51
  of those carry the `terrain.h:70-79` initializers in the Dreamcast corpus;
* of the 7 units the old tail reading chose, 6 have no such initializers in
  Dreamcast;
* drawing's run stores to `0x6969xx`, next to drawing's own data
  (`g_combatGridPosted` at `0x6969d4`).

So in every unit whose retail range opens with the run, the first own
function receives phase 1. Including `terrain.h` in those units (branch
`match/state-walls`) brought `showCreatureSpellError` to 100 and reproduces
each object's head.

Retail's own address order is a weak guide to the first compiled function.
Our first compiled function equals retail's first by address in 70 of 150
units. `_$E` initializers and COMDAT bodies are compiled first but linked
elsewhere.

## 2. Declaration offset: the symbol-handle numbering (C1XX → C2)

**Edit → state.** Each declaration takes handles from C1XX's counter, which
[handle-order.md](handle-order.md) models (a `typedef` takes 1, a
one-member struct 9). Every symbol created after an edited point is
renumbered.

**State.** C2 decodes handles in the IL reader `0x1c92e` (a u16, or 31
bits when bit 15 is set). 18 call sites carry handles. `0x1cedf`, `0x1d193`
and `0x1d253` carry other numbering and never shift. The sites were measured
by decoding real captures with and without leading declarations.

**Effect.** Periodic in the offset, with period 64.

* `initializeGameData` takes 6 assemblies over offsets 0..63, and the same
  6 recur up to offset 260.
* The offset decides whether `&g_hierarchyMask[i]` is hoisted into a
  register or folded into each access.

**Where C2 consumes it** is not yet named. Consumers found and ruled out:

* the scoped symbol table `0x14bf4` (1024 buckets, key at `+0x28`);
* the per-function hash `0x9d88c` (keys are per-function temporaries);
* heap displacement.

Observed effects:

* the decode order changes at `0x67d47`/`0x67f7a`;
* the first execution divergence is in the bucket walk `0x2df43` →
  `0x2fda4`.

## 3. Callee compile order (C2 record `+0x14` bit `0x800`)

**Edit → state.** Changing which inline callees C2 has already compiled
when it compiles the function changes this bit. Examples: moving the
function, or adding an earlier user of an implicitly generated member,
which C1XX emits after its first user.

**State.** A callee record's `+0x14` bit `0x800` becomes set once C2 has
compiled that callee's body.

**Effect.** An inlined body expands differently before and after. For
example, `NewfullMap::readBlackBoxData` inlines the implicit
`~BlackBoxData` with per-member EH state stores while that destructor is
uncompiled, and without them afterwards. Setting only that bit at the
expansion reproduces the real reordered compile exactly.

**Reachable values.** For a function F whose inline callees with emitted
bodies are c1..cm in compile order, F at any position sees exactly a prefix
{c1..cj} as compiled, for j = 0..m.

## Tools

```sh
homm3 vc6 state <unit|VA>       # read: emission order, phase received, handle base
homm3 vc6 compile-m <unit> [--function VA] [--against OBJ]
homm3 vc6 fuzz-verify <unit> [--function VA] --edits N [--seed S] [--reuse]
homm3 vc6 phase-census [units...]
homm3 vc6 compile-m-walls walls.tsv [--reuse]
```

**Set (IL replay through the trace shim).**

* `HOMM3_VC6_PHASE=0|1`: the value left at every driver return, and
  initially.
* `HOMM3_VC6_HANDLE_SHIFT=h0:k` (hex `h0`, decimal `k`): every decoded
  handle ≥ h0 is raised by k. `compile-m` uses h0=1 and raises the `gl`
  high-water (offset 7) by k.
* `HOMM3_VC6_COMPILED_SPEC=<file>`: lines `root<TAB>callee…`. While a
  listed root is compiled, the listed callees count as compiled and its
  other callees do not.

`unstable_state.State` and `Unit.replay` wrap all three.

**compile-m.** The unit's front end runs once, and its IL is replayed:

* once with no state set, which must reproduce the plain replay for every
  function;
* for phase ∈ {0,1} × offset 0..63;
* for each callee-prefix round j = 0..max m, where every function gets
  prefix min(j, m).

`build/vc6/unstable-state/<unit>/compile-m.json` lists, for every
function, its M distinct assemblies. Each variant carries its
relocation-masked bytes (hex), a SHA-1 prefix and the states that give it.
Each function also lists its inline callees in compile order.

**fuzz-verify.** Applies N seeded random unrelated edits to scratch copies
under `build/`. Each edit is one of:

* 1..4 declarations or definitions inserted at the top or before an
  annotated definition;
* a swap of two adjacent definitions;
* a definition moved to the front;
* a declaration in a shadow copy of a directly included header.

Each copy is compiled with the plain compiler. The harness asserts that
every original function lands in its predicted set. Functions whose inline
callees the edit moved are skipped and counted, since moving a callee
touches the callee. Escapes are logged with the edit, and the coverage of
predicted variants is reported.

## Validation

* **Replay knobs against real compiles.**
  * Handle shift: real compiles with k leading `typedef`s equal the shift
    replays for every function of initialize, army, philai and advmgr at
    k = 2, 13, 27, 48 (487 functions, 0 mismatches). `initializeGameData`
    matches at all 64 offsets at both placements.
  * Phase: the setter reproduces the real reorder for
    `showCreatureSpellError`.
  * Callee bit: the setter reproduces the real reorder for
    `readBlackBoxData` and `readEventData`.
* **Ground truth.** Of the 21 CUR≠MAX functions with unchanged source:
  * Only drawing's has neither its own IL nor an inlined callee's IL
    changed semantically. Its MAX bytes are in the predicted set, at
    phase 1.
  * `initializeGameData`'s MAX is also in the set, at decl-offset 27.
  * The other 19 changed through dependency source and are correctly
    absent.
* **Fuzz.** Totals over three passes on the 21 ground-truth units:

  | pass | edits | units | checks | escapes |
  | --- | ---: | ---: | ---: | ---: |
  | 1: inserts | 1,500 | 15 | 177,100 | 0 |
  | 2: + swaps, headers | 2,722 | 14 | 416,157 | 4 |
  | 3: + front moves | 3,239 | 17 | 425,731 | 6 |

  * The pass-2 escapes were all std-sort instantiations in
    singleselectionwindow, after a swap that moved their inlined comparator
    (`TSortMapsByVictory::operator()`). The harness now treats a moved
    inline callee as a touched callee: a rerun with that seed's edits had 0
    escapes, skipping 185 checks.
  * The 6 pass-3 escapes were all in mapcell, after moving a caller ahead
    of the first user of `~BlackBoxData`. They led to state 3; the same
    seed now has 0 escapes.
  * Coverage of predicted variants is partial: for example 26 of 79 in
    mapcell. Random edits rarely reach every phase/offset/prefix
    combination; an unhit variant is not shown to be unreachable.

## Stable walls under this state (2026-10-07)

`compile-m-walls` ran `compile-m` for each of the 242 walls (rmg/zlib
excluded) and checked whether retail's masked bytes are among the M
assemblies. Output: `build/vc6/unstable-state/walls.{jsonl,md}`.

| verdict | walls |
| --- | ---: |
| retail not among the M assemblies | 226 |
| retail in the captured state (the residue is outside the function's bytes) | 7 |
| not compared (generated `__h3cg` helpers, or no retail body) | 7 |
| retail reachable by unrelated-edit state | 2 |

The two reachable walls are both phase cases:

* `aiEnterTown` (0x5253d0) at phase 0;
* `loadSeerHutTextColumn` (0x56c120) at phase 1.

No wall needs a declaration offset or a callee prefix. The 226 are source
differences: unrelated edits cannot reach retail for them.

## State scan of the non-exact functions (2026-10-08)

`homm3 vc6 state-scan --offset-units FILE` compiled the 63 in-scope units
that have a function below 100 (CUR or MAX), with `terrain.h` in its
evidenced units. Each axis was swept alone from the captured state: phase
0/1, offsets 1..63 only in the 13 units the handle census found sensitive,
and callee-prefix rounds up to the longest prefix among the non-exact
functions. Every assembly was scored with the build's paired normalization
and objdiff, so the scores are the numbers the ledger banks.

Of 516 state-sensitive functions, 100 are not exact. 15 have an assembly
that scores higher than the current one:

| function | current | best | state | verdict |
| --- | ---: | ---: | --- | --- |
| `initializeGameData` | 94.07 | 100 | offset 13..26 | real handles not yet identified |
| `aiEnterTown` | 99.96 | 100 | phase 0 | no arrangement gives 0 (see below) |
| `transmitSaveGame` | 97.40 | 97.43 (= MAX) | offset 5, 8, 9.. | lead only |
| `aiCheckRetreat`, `purchaseBuilding`, `hero::initialize`, `type_dialog_icon::set`, `TCampaignBrief` ctor, `setupDynamicStuff` | | +0.003..+0.47 | phase 0 | not first in their TU |
| `TSingleSelectionWindow` ctor, `getHeaders` | 96.41, 94.69 | 96.90, 95.79 | callee prefix | reachability unconfirmed |
| `aiAttemptMove`, `giveArtifact`, `type_quest::loadFromMap`, `type_skill_quest::doProgressDialog` | | +0.02..+1.03 | callee prefix with template members | reachability unconfirmed |

**Phase 0 mid-TU.** Seven functions are closer to retail with phase 0, and
none of them is the first function in its TU. `aiEnterTown` is exact
there. Under the model, every function compiled above them leaves 1. All
seven have a predecessor with a loop, but so do 21 of the 78 functions that
need 1, so loops do not explain it. This is open for the phase-flag
mechanism.

**The callee-prefix axis over-generates.** A captured site dump in
singleselectionwindow shows bit `0x800` still clear at `getHeaders` for
implicit members that the object compiles earlier: `~NewSMapHeader`,
`??0GameSelectionHeadersStruct`, `~SavedGameHeader`. Meanwhile template
members such as `~basic_string` do carry it in mapcell. "Compiled earlier"
therefore does not decide the bit. The rounds remain a superset (the fuzz
runs found no escapes), but a variant reached only through a callee prefix
needs a real edit to show that it can be reached.

## Limits

* `compile-m` sweeps the three axes separately. Phase × offset is a full
  product; callee prefixes are swept at phase/offset as captured.
* A global offset stands in for "declarations before any point". Mixed
  offsets were probed only on `initializeGameData`'s 16×16 grid.
* The phase sweep gives every function both values. In practice 0 needs the
  function compiled first (or a predecessor that leaves 0, which no
  function in the census did), so phase-0 variants are reachable but rare.
* The handle/non-handle classification of decoder call sites was measured
  on two units. The whole-unit equality checks above are the guard.
