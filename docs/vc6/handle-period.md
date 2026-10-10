# The period-64 handle effect: how declaration counts reach the register allocator

Adding *k* handle-consuming declarations above a function can change its
code. For any fixed edit point the effect repeats every 64 handles, so
`initializeGameData` has six assemblies over k = 0..63, and the same six
recur up to k = 260 and beyond. This page explains where C2 turns handle
numbers into that period, which handles matter, which source edits move
them, how often it happens, and how to use it for matching.

The subject is C2.DLL 12.00.8447 (image base `0x10700000`; RVAs below).
**Measured** means observed in pinned-compiler IL replays through a scratch
trace shim (§7). **Static** means read from the disassembly. **Hypothesis** is
marked as such.

## 1. Mechanism

The period comes from a 64-bucket hash table that the register allocator's
setup pass builds for rematerializable constants. Handle values enter only
there. They do not enter through the 1024-bucket table at `0x9d88c`, which
is keyed by live-range number (a different number from the IL handle) and
is only the downstream consumer. They do not enter through the operand-rank
hash `hashOperand` (`0xd25d`) either.

<!-- c2-role: function 0x2cac2 buildLiveRanges -->
<!-- c2-role: function 0x2abbf collectConstantOperands -->
<!-- c2-role: function 0x2c6ce findOrAddConstantPseudo -->
<!-- c2-role: global 0x9d750 constantPseudoBuckets -->
<!-- c2-role: function 0x2cdf4 releaseUnusedPseudos -->
<!-- c2-role: function 0x211c3 getLiveRangeRecord -->
<!-- c2-role: global 0x9d74c nextLiveRangeKey -->
<!-- c2-role: global 0xae038 freeLiveRangeRecords -->
<!-- c2-role: function 0x213c6 releaseLiveRangeRecord -->
<!-- c2-role: global 0x9d88c liveRangeTable -->
<!-- c2-role: function 0x2d3ef allocateRegisters -->
<!-- c2-role: function 0x2df43 substituteSingleUseTemps -->

The chain, static unless noted otherwise:

1. **The pseudo table.** `optimizeFunction` calls `buildLiveRanges`
   (`0x2cac2`) just before register allocation. It resets the live-range key
   counter `0x9d74c` to 1 and clears the 64 buckets at `.bssbe 0x9d750`
   (`rep stosd`, ecx=0x40).
   * `0x2abbf` walks the tuples, filtered by `0x2c600`, and passes each
     operand of kind 7 (integer constant) and each of kind 3/4 (constant
     address) to `0x2c6ce`.
   * `0x2c6ce` is unoptimized compiler code (it uses a frame pointer and
     `div` by 0x40). It hashes the operand into one of the 64 buckets:
     * a constant: `value % 64` (`0x2c6f4`);
     * a symbol address: `sym+0x28 % 64` (`0x2c7de`, `0x2c7f8`).
       `sym+0x28` is the symbol's **IL handle**: the logged keys equal the
       `il handles` capture values, and under the replay shift knob they
       move by exactly k.
   * It looks up or creates one pseudo per distinct constant (a 0x54-byte
     record of kind 0xd), inserts it at the head of its bucket's chain, and
     gives it a live-range record from `getLiveRangeRecord` (`0x211c3`).
2. **Release in bucket order.** `releaseUnusedPseudos` (`0x2cdf4`) walks
   buckets 0..63, newest entry first within a bucket. It releases the record
   of each pseudo whose count field (`use+0x14`) is below 1
   (`releaseLiveRangeRecord`, `0x213c6`). In `initializeGameData` every
   pseudo is released.
   * Released records go onto a LIFO free list, `0xae038`.
   * They keep their numeric key: `+0x1c` is restored after the record is
     zeroed.
3. **Keys are recycled.** The same function then walks the symbol arenas
   (`0x9bc50`) and creates a live-range record for each ordinary variable
   and temporary. `getLiveRangeRecord` takes the free-list head first
   (`0x2125a`) and skips the key assignment, so each **variable's
   live-range number is a released pseudo's number, in reverse bucket
   order**. Measured on `initializeGameData`:
   * at k=0 the first reused key is `0x22`, released from bucket 63;
   * at k=30 it is `0x2f`;
   * variables and temporaries have `sym+0x28 = 0` and are never hashed
     themselves.
4. **Key order steers allocation.** `getLiveRangeRecord` also files every
   record in `liveRangeTable` (`0x9d88c`, bucket `key & 0x3ff`; keys are
   below 1024 here, so this is key order). `allocateRegisters` (`0x2d3ef`,
   called at `0x6858b`; it contains the global coloring `0x245c3`) walks
   that table in key order in three places:
   * `0x2db8a`;
   * **`substituteSingleUseTemps`** (`0x2df43`, which checks with `0x2fda4`
     whether a single-use temporary's definition can be moved to its use);
   * `0x2450d`, ahead of the global coloring `0x245c3`
     ([regalloc.md](regalloc.md) §3b).

   These walks are order-sensitive: each accepted substitution changes the
   kill ranges for the temporaries visited after it.

**Measured localisation.** For `initializeGameData` I dumped the IL in a
form that does not depend on handle values at each of the 42 phase
checkpoints of `optimizeFunction`. The dumps for k=0, k=30 and k=1024 are
identical through checkpoint 23. They first differ after `allocateRegisters`:
* at k=0, `substituteSingleUseTemps` moves 6 temporaries;
* at k=30 it moves 9, because a different set of records is disqualified at
  `0x2e05c`.

This set is what decides whether `&g_hierarchyMask[i]` stays in a register
or is folded into each access.

**Counterfactuals.** These patch the compiler and are not matching output.
* Making the hash divisor 1 (`0x2c6f5`, `0x2c7df`, `0x2c7f9`) leaves every
  function of `initialize` constant over k = 0..63, and identical to the
  unpatched k=0 output.
* Disabling record reuse instead (`0x211d2`: esi=0, so every record gets a
  fresh key) also removes all variation in `initialize` and in army's
  `doAttack`.
* Neutralizing `hashOperand` changes nothing for `initializeGameData`.

The patched census in §4 extends the first test to every game unit.

## 2. Which handles matter

Only the **IL handles of data symbols whose addresses are constant operands
of the function after optimization** matter: global or static variables,
class static data members, and string literals (`??_C@…`). The bucket of an
integer constant depends on its value, which no declaration edit changes.
None of the following is hashed: the function's own symbol, its parameters,
locals, temporaries (all `sym+0x28 = 0`), callees, or types.

Measured on `initializeGameData`, whose address constants are 22 data
symbols:
* `town::s_includedBuildings` `0x2864`;
* `g_townEligibleBuildMask` `0x28a4`;
* `g_hierarchyMask` `0x28a5`;
* the file statics `g_town0..8Buildings`, `g_commonIncludeList` and
  `g_town0..8IncludeList`, `0x28b2`…`0x28c4`.

Its integer constants are 0, 4 and 0x56. A replay hook that shifts only
chosen handles gives:

| shift | result |
| --- | --- |
| only those 22 handles, by 65566 (≡30) / 65538 (≡2) / 61 | the full-shift class for k=30 / 2 / 61 |
| every **other** handle by 30 or by 2 | class of k=0 (no effect) |
| full shifts by 1024, 2048, 4096 (+0/+30) | class of k mod 64 |

The 22 handles are therefore necessary and sufficient. Shifting only them by
a small amount such as +30 or +12 gave an unrelated new assembly. The
presumed cause is collision with neighbouring handles, so the selective
tests use 65536+k.

army's `doAttack` has three address constants: `g_text` (bucket 25), and two
string literals in buckets 12 and 40. It also has about 25 constant buckets.
Its alternate class appears at k = 7, 20..23 and 48..51. Those are the
offsets that put the bucket-40 or the bucket-12 literal into buckets 60..63
(an observation, not a derived rule).

A function is exposed only when all of the following hold:
* it has address constants;
* their pseudos are released together with constant pseudos in an order
  that a shift changes;
* the reordered keys change a decision in the allocator walks.

Only the cyclic position of each relevant handle relative to the fixed
constant buckets and to the 64 wrap matters. That is why a shift by any
multiple of 64 is invisible.

## 3. Which source edits move them

A relevant handle moves when a handle-consuming item is added or removed
**before the point where that handle is created** in the TU. The costs below
follow [handle-order.md](handle-order.md); probe results are measured with
`python3 -m homm3.vc6.il handles`.

* **Moves them:**
  * a `typedef` (+1);
  * an enum (1 + one per enumerator);
  * a prototype (1 + its parameters);
  * a variable (+1);
  * a struct (9 + one per member…);
  * a method declaration in an earlier class;
  * a new `#include`, which costs the sum of its declarations;
  * an implicit template instantiation: `V<int> g_v;` cost +12 in the probe;
  * reordering definitions across the creation point;
  * locals, nested `{}` blocks and parameters in **earlier function
    bodies**, inline functions included. Three locals in an earlier
    function gave +3 to a later static; one block plus one local gave +2.
* **String literals** get a handle at their first use in the TU. A literal
  used in an earlier function body is created there, and any edit before
  that use moves it.
* **Does not move them:**
  * anything after the creation point, including bodies of later functions
    and declarations between the last relevant creation point and the
    function;
  * comments, blank lines and macros;
  * redeclarations (free);
  * any edit totalling a multiple of 64.
* **Partial shifts.** The relevant handles are often created at different
  places: externs in headers, file statics at their definitions, literals at
  first use. An edit between those points moves only the later group and
  changes their relative buckets as well. For `initializeGameData` the
  creation points are:
  * `town.h:496`, the class static;
  * `town.h:600`/`606`, the two externs;
  * `initialize.cpp:21-180`, the file statics after the
    `initializeGameData` prototype at line 9.

  The other lane's top-of-file and before-function sweeps both land before
  all 22 handles, which is why they give the same six classes.

**Real recompiles agree.** These were ordinary compiles, without the shim
knob, of `initialize.cpp` with one item added before its includes:

| added | handles | `initializeGameData` |
| --- | --- | --- |
| `enum { 12 enumerators };` | +13 | retail-exact |
| `struct { int a, b, c, d, e; };` | +13 | retail-exact |
| 13 typedefs | +13 | retail-exact |
| `struct { int a, b, c, d; };` | +12 | not retail |
| 27 typedefs | +27 | not retail |

The function's ledger CUR has moved between 90.1% and 100% at commits that
changed neither `initialize.cpp` nor `town.h`: 462e36412, 0dbc798f8,
a5372f736, 2e87c6efe and 340ed83c0. These are the declaration-count
changes in earlier headers that §3 describes.

## 4. States and effects across the corpus

Measured over all 135 game-profile units: 9,368 functions, 64 offsets
each, applied to every handle as a top-of-TU insert would. The census
includes `rmg`.

* **Rare.** Only **18 functions vary** (0.19%); `rmg` has 3 of them.
  * 16 have 2 assemblies, `kb` `oldmain` has 5, and `initializeGameData`
    has 6.
  * Each class is one or a few contiguous k-windows, as §2 predicts. For
    example, `getCommand` uses class 1 at k = 14..34 and 36.
* **One mechanism.** With the hash divisor patched to 1, all 18 have
  exactly one assembly over the 64 offsets, and no other function starts to
  vary. In this corpus every period-64 effect is the pseudo-table key
  recycling of §1.
* **What changes.** Each non-baseline class was compared with the k=0
  class over its instruction stream:

  | kind of difference | class pairs | example |
  | --- | ---: | --- |
  | same instructions, different order (scheduling only) | 7 | `getCommand`, `doAttack`, `transmitSaveGame`, `giveArtifact` |
  | different order and different registers, same length | 10 | `aiEnterTown`, `onKeyPress`, `onKillFocus`, `initializeSpells`, `oldmain` |
  | different instruction count (substitution, hoisting, addressing) | 8 | all five of `initializeGameData`'s, `doCombat`, `update@recruitUnit`, `createArtifactWidgets` |

  None of the differences was stack-displacement-only, inlining or call
  structure. Everything happens inside register allocation:
  * which single-use temporaries are substituted;
  * which address stays in a register;
  * coloring ties and the resulting schedule.
* **Retail and the classes.**
  * In 6 functions the k=0 class is retail and the others are not. These
    are exact today but **fragile**: they break at the margins below.

    | function | breaks at |
    | --- | --- |
    | `CEnterNameEdit::onKillFocus` | +3 handles |
    | `getCommand` | +14 |
    | `CEnterNameEdit::onKeyPress` | ±15 |
    | `monstersSellOut` | +26 / -13 |
    | `town::initializeSpells` | +26 / -29 |
    | `TRmgQuestCreatureDef::generate` | -11 |

  * In 1 function, `initializeGameData`, retail is a **non-zero** class (§5).
  * In 10 functions no class is retail; their residue lies elsewhere. Some
    of them have a class closer to retail than k=0 (instruction-stream
    similarity):
    * `game::transmitSaveGame`: 98.61 at k=5 against 98.50 at k=0. Its
      ledger has CUR 97.4042 below MAX 97.4264.
    * army's `doAttack`: 99.23 at k=7 against 99.04.

## 5. Matching use

**`initializeGameData` (0x4eb730) is offset-reachable.**
* Retail is the class at k ≡ 13..26 (mod 64), measured with a 64-offset
  replay and confirmed by real recompiles (§3). It is 100% today with
  +13..+26 handles placed before `town.h:496`, the creation point of the
  first of its 22 relevant handles. Equivalently, −38..−51.
* This explains its ledger history: HIST 100, then CUR moving with header
  churn.
* The smallest real changes in that window are:
  * an enum of 12–25 enumerators;
  * a struct of 5–18 data members;
  * a class gaining methods totalling 13–26 handles (1 + parameters each);
  * the equivalent removal of 38–51 handles.

  These must sit in the include chain before `town::s_includedBuildings`,
  or inside `class town` before line 496.
* Which declaration the original had is not decided by this evidence. The
  next step is to compare the DC/Mac declarations of the headers included
  before that point (`town.h` and its include set) with ours, and look for
  missing or extra declarations whose cost lands in the window. Do not add
  padding.
* 2026-10-08: removing the invented `message::setWidgetCommand` (3 handles)
  moved the unit to a 90.14% class; retail was then 11..24 handles ahead.
  The Dreamcast enumerators our chain enums lacked supply 12: TTerrainType's
  beach/magic-plains/cursed-ground, TTownType's `kNumTownTypes`, SpellID's
  six range enumerators and TCreatureType's two siege-weapon counts. The
  function is exact again. The same shift moves `doCombat` 96.22 -> 95.78
  and `aiEnterTown` 99.96 -> 99.93 (MAX held). Every later edit to these
  enums (the remaining DC artifact, creature and building enumerators)
  moves the unit again; recheck with `compile-m initialize`.
* 2026-10-09: folding the invented `message` dialog/input helpers removed six
  handles (initializeGameData 100 -> 94.07; retail then needed +6..+19).
  Completing `type_building_id` with its 200 remaining Dreamcast
  enumerators (+200, i.e. +8) restores it. The same edits left
  `monstersSellOut` (retail k = 3..25 or 32..63) and `transmitSaveGame`
  (its better class at 3, 7, 26..28, 30..39, 48..59) at k = 0; the
  Dreamcast `e_looping_sound_id` names in advmgr.h (+50) restore both.
* 2026-10-09: completing TCreatureType (the 57 ids the editors' town
  generator tables and the game's narrow ai.h/advmgr_objects.h rosters
  named; those rosters fold into it) alone took initializeGameData to
  94.07. Measured window above that +57: k = +5..+18 is retail, +3 gives
  97.04, +20..+26 96.06. The 16 artifact ids the game passes to its
  artifact predicates (+16, artifact_type.h) land inside it. The same
  edit moves `aiEnterTown` 99.9565 -> 99.9304 (MAX held): in philai's
  frame its retail-closer class needs at least 4 more handles, which
  initializeGameData's window excludes.
* 2026-10-09: Loki's `TObjectType::getImageNum` (objecttype.h, the map
  editor's object sprite table) moves `monstersSellOut` 100 -> 99.9517
  (MAX held). Measured after it: retail is k = 2..25 or 32..62, the
  current state k = 0 (the edit cost the two handles from k = 62).
  `onKillFocus` and initializeGameData stay exact.
* 2026-10-10 (re-audit under the MAX rule): remote.h loses the nine
  duplicate prototypes 6da218611 had kept only for their handles. CUR-only,
  MAX held: `onKillFocus` 99.87, `onKeyPress` 99.89, heroWindowManager's
  doDialog/doDialogDraw/doQuickView 99.96..99.97, string `_Copy` 99.95 in
  adventuremapwindow, receiveHeroTownData 99.98, oldmain -0.002. Under the
  rule these dips are noise; do not re-add declarations to steer them back.

**For any function** whose CUR moves between commits that did not touch its
TU, check with `homm3 vc6 compile-m` whether its assemblies are period-64
offset classes. Then use §2 to name the relevant handles and their creation
points:
* the data symbols whose addresses the function uses;
* string literals at their first use.

The needed residue fixes how many handles the original had **before those
creation points**, modulo 64. It does not fix which declaration made them.

**Fragile exact functions.** The six in §4 stay exact only while edits
above their relevant handles keep the net count within their margins
(`onKillFocus`: +3). A header refactor that changes the count by that much
breaks them, and so does a new local, block or literal in an earlier body.

**Walls.** The other lane's 242 stable walls contain no function that needs
a declaration offset ([unstable-state.md](unstable-state.md)). Among the
period-64 functions, `initializeGameData` is the only regression that
recovers. `transmitSaveGame` (CUR 97.4042 < MAX 97.4264) and army's
`doAttack` have a better non-zero class, but not retail.
* `transmitSaveGame`'s relevant handles include 13 string literals, most
  of them first used inside the function itself.
* Its better class needs k ≡ 5..18 and other windows. Locals, blocks or
  literals added earlier in the TU, or **earlier in its own body before
  those literals**, move them.

## 6. Relationship to other instability sources

* The phase flag ([phase-flag.md](phase-flag.md)) is independent. A shift by
  k never changes it.
* Inliner cost records (`sym+0x6d`) are not handle-valued.
* The 1024-bucket `liveRangeTable` is keyed by live-range number, not by IL
  handle. Its order is affected only through the recycling in §1.
* The operand-rank hash in regalloc.md §6n uses `sym+0x1c` of C2-created
  expression symbols. It did not participate in `initializeGameData`.

## 7. Method

The method follows [phase-flag.md](phase-flag.md) §7. The unit is captured
once (`/d1il`) and replayed through a scratch copy of the trace shim, so no
compiler output used here comes from a patched source. The scratch shim
adds these hooks:

* `HOMM3_VC6_HANDLE_SET="h,…:delta[:invert]"`: a selective version of the
  other lane's decode-site shift (`0x1c92e`);
* `HOMM3_VC6_TUPLE_DUMP`: a canonical IL listing at every
  `phaseCheckpoint` (`0x9ab4`). Symbols are numbered by first appearance,
  so the listing does not depend on handle values;
* `HOMM3_VC6_PK_LOG`: logs of the pseudo hash (`0x2c6fe`), the release
  walk (`0x2ce69`) and record reuse (`0x2125a`);
* `HOMM3_VC6_WALK_LOG`: the `substituteSingleUseTemps` walk and its
  disqualification sites;
* `HOMM3_VC6_PATCH`: raw byte counterfactuals.

These hooks are not committed: the shim belongs to the unstable-state lane.
The unforced replays reproduce the normal objects.

## 8. Open questions

* The exact semantics of the `use+0x14 < 1` release test in
  `releaseUnusedPseudos`, and of the `0x2c600` operand filter. Kind 4
  operands were never observed.
* Why only 0.19% of functions respond. The permuted live-range keys
  presumably feed decisions only when the allocator walks meet an
  order-sensitive substitution or a coloring tie; this was not measured
  per function.
* Mixed partial shifts were measured only through the selective-set test on
  `initializeGameData`, not swept over the corpus.
* Interaction with the callee compile-order state found by the
  unstable-state lane (callee record `+0x14 & 0x800`). Reordering
  definitions moves handles **and** compile order (that state and the phase
  flag) at once. The channels are independent in C2: none of them reads
  the pseudo table. A single real edit can still trigger more than one.
