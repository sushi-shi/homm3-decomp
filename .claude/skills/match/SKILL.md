---
name: match
description: "Byte-match HoMM3 functions/TUs against retail HEROES3.EXE - the whole in-tree loop: locate via the DC-roster order-map, claim, reconstruct C++ for VC6 SP3 (/O2 /Ob2 /Oy- /Op /ML /Gr /GX), iterate with homm3 sema diff, ratchet with homm3 build, document residual classes, and judge TU closure. Use when matching a function, reconstructing/mapping/closing a TU, chasing a plateau, or promoting located addresses to claims. Sibling doctrine adapted from homm2/gruntz matcher agents."
---

# match — reconstruct byte-matching HoMM3 TUs (VC6 SP3 /O2)

You write C++ that, compiled with VC6 SP3 `CL.EXE` under wine (per-TU flags in
`config/units.toml`; default game profile `/O2 /Ob2 /Oy- /Op /ML /Gr /GX /D_WINDOWS` - /Op is engine-wide, byte-proven by the AI TUs),
produces code byte-identical to retail `HEROES3.EXE`, verified by objdiff through
`homm3 build`. External sources remain hypotheses until retail-byte evidence proves
them; substantive outcomes are recorded in the port plan's §5 decision log.

## The governing ledger: per-function MAX fuzzy (ratchet)

`config/match_baseline.tsv` records each function's best-observed fuzzy; a drop
below MAX fails the build. **MAX is the only ledger** (gruntz doctrine): current-%
dips from correct changes are acceptable — the user tracks MAX, not simultaneous
exactness. A deliberate lower is a hand edit of the baseline row with a dated
comment (precedent: GetArmyMorale). Promoting a carcass fn renames its row —
DELETE the superseded flat-name row in the same change or the gate reports it
MISSING forever.

**A PEAK CAN BE LOST WITHOUT `hist` RECORDING IT.** If an edit regresses and
is re-banked at the lower value before anyone notices, BOTH `max` and `hist`
sit at the lower number and nothing in the ledger remembers. `game::Load`'s
five `type_point emptyPoint;` declarations had drifted back INSIDE their pins;
`max` and `hist` both read 90.98, and the only surviving record was its own
residual note, which quoted 91.8624 — exactly where re-hoisting them landed.
**So a note quoting a number higher than the row's own `max` is itself a
lost-peak report** — but read it, do not grep it. A mechanical scan over 1900
quoted percentages produced 144 hits and the top ones were all false: notes
routinely cite OTHER functions' scores ("costs SetupHeroView 99.53"), and a
note sitting between two claims attaches to the row that FOLLOWS it. The
precise check is `hist > max`, which `homm3 vc6 queue` already reports.

**CHECK `hist` AGAINST `max` — THE RATCHET CANNOT SEE A LOST PEAK.** It
compares against `max`, so once a max has been accepted downward the row sits
below a value it once reached and the build stays green forever. `hist` is the
only record. `homm3 vc6 queue` now reports every such row.

This is not hypothetical, and it happened TWICE from the same cause:
- A retired view gate left a header invariant unenforced — the `#if` went with
  the audit, the prose survived the merge — and two `town` rows sat 20.7 and
  11.5 points under their peaks with the ratchet clean. Both recovered, one
  back to EXACT.
- The same audit exposed a **default constructor**: `type_artifact`'s
  `: artifactId(-1), extra(-1) {}` had been gated so only `game.cpp` saw it.
  With the gate gone `seerhut.cpp` saw it too, and its loop gained two dead
  `-1` stores plus a lost CSE — `GetAIValue` 100 -> 70. Constructing through
  the two-argument ctor restored the exact bytes with no header change.

**When a gate or a declaration is removed, re-read what its comment claimed to
guarantee — and check SPECIAL MEMBERS, not only function definitions.**

**A MAX IS ONLY COMPARABLE WITHIN A DELINK GENERATION** (proven 2026-08-20).
New claims in ANY TU rename symbols in the synth PDB, vostok delinks every
unit against that PDB, so the TARGET half of the comparison moves for units
you never touched. `recruit::Update` fell 90.8376 -> 88.2360 at an
integration with recruit.cpp and all its headers byte-identical; reverting
only mapcell.cpp's two new VA claims and re-delinking returned it to exactly
90.83756, and restoring them returned it to 88.23604. So:
- a cross-unit "regression" after a merge that added claims MAY be this
  rather than a code change — check whether the unit's source and headers
  actually moved before hunting a spelling. But do not assume it: of the
  FIVE observed instances it is two, two and one - and the fifth has the
  BEST fix of the three. A lane put three channel-mask externs in
  `bitmap16.h`, which 21 TUs include, and it cost `recruitUnit::Update`
  90.84 -> 88.24; MOVING them to `spells.h`, which 2 TUs include, made the
  regression disappear entirely. So before gating and before accepting, ask
  whether the declaration is simply in too wide a header. Narrowing the
  header is not a workaround - a declaration belongs with its consumers, and
  it costs nothing. Of the other four, Two were real ungated `game.h`
  additions (a `type_point` member that made a class non-POD, and a nested
  enum), both restored by gating; two were the delink generation, each PROVEN
  by a revert control — stash the lane's source and headers, re-delink, and
  see the row return to its old value exactly. Rule out the source FIRST
  because it is cheap, then run the revert control rather than assuming;
- the newer, more-claimed reference is the more ACCURATE one, so the lower
  number is generally the honest one. Accept it with the cause recorded, do
  not chase it;
- conversely, do not bank a max measured against a stale delink. Re-run
  build -> delink -> build before trusting a number you are about to record.

## The loop

1. **Locate** (if unclaimed). The DC CodeView roster is the spine:
   `awk -F',' '$5=="<tu>.obj"' evidence/dreamcast/functions.csv` — the `file`
   column separates real TU rows from header/template attributions. Order-map DC
   line order onto carve rows (`config/retail-functions.tsv`) inside the TU's
   link-order bracket (`evidence/link-order/{units,gaps,functions}.tsv` — STALE
   between regens; recompute with `python -m homm3.analysis.link_order`).
   Corroborate EVERY pairing with body evidence: strings, imports
   (`__imp__X@N` = N arg bytes), claimed-callee names, address-takes (hardest
   proof), vtable stores, size plausibility (SH4→x86 0.3-2.5x). Exhaustive
   order-maps over a segment count as proof (kb.cpp PollSound..oldmain
   precedent). cinit-pattern rows (guard byte 0x6abaa0 / atexit / ~95B
   ten-iteration initializers) and STL COMDAT tails are excluded class - never
   claim them. DC size columns NEVER transfer to claims; sizes come from the
   carve only. **`DC_ONLY(off, cb)` = "evidenced only in the Dreamcast build"
   (va.h:31), NOT "no retail body exists".** Many DC_ONLY rows have real retail
   bodies merely not located yet; promoting one to `VA` on body evidence is
   ORDINARY locate work needing no special approval. The reverse also holds: a
   claim found sitting on an excluded class is WITHDRAWN back to DC_ONLY.
   Arity (`ret N` vs DC parameter count) is the highest-yield screen there is -
   eight consecutive lanes found misattributed claims, most of them this way.
**CLAIMING IS BULK WORK; RECONSTRUCTING IS NOT** (measured 2026-08-20, and
this CORRECTS an entry written earlier the same day that said never to
bulk-promote a roster).

A `VA()` claim may sit INSIDE the `#if 0 // @carcass` block with a `// @stub`
body. The va-claims gate is a line regex, so a carcass claim is still
order-, size- and class-checked without ever being compiled. 54 such claims
already existed across 13 files (ai_tactical 14, advmgr 7, town 7,
victorylossconditions 7) before anyone wrote this down. So once an order-map
is verified, promote the whole roster in one pass - verify every RVA against
`config/retail-functions.tsv` offline first - and let the gate check it.
spells went from 2 claims to 40 in a single commit that way.

The compile clash only bites when you pull a row OUT of the carcass to
reconstruct it, and then it is per-function work: the carcass signature and
the hand-modelled header declaration frequently DISAGREE, and VC6 reports the
clash as a cascade of C2065 `undeclared identifier` on the PARAMETER NAMES -
which reads like a missing include, not a signature mismatch, and the first
error can land 300 lines from the row that caused it.
`combatManager::ComputeSpellDamage`'s carcass row says
`int (SpellID, int, int, const hero*, const hero*, const army*, unsigned
char)` while `cmbtmgr.h:1409` declares `long (SpellID, long, long, ...)`.
Reconcile per function, preferring the header - it is modelled from bytes.

2. **Claim.** `VA(0x004xxxxx, 0xSIZE)  // <evidence-tag>, dc 0x<off>` above the
   declarator; absolute VAs; sizes carve-exact; strictly increasing per file
   (the ORDER gate); keep `// @stub` bodies for located-not-reconstructed.
   Compiler-generated scalar deleting dtors:
   `VA_COMPGEN(0xADDR, 0xSIZE, SCALAR_DELETING_DTOR, <class>)` — the base obj
   already emits `??_G`, the claim alone pairs it. Data: `DATA(0xADDR)` on
   definitions in the owning TU (or nearest consumer with an ownership note);
   string literals/float-pool via `DATA_COMPGEN(0xADDR, name, "value")` — read
   exact bytes from the hash-verified image, never guess. Unclaimed data externs
   still pair (reloc NAMES don't gate the verdict — DoDialog precedent); claims
   are hygiene + future data-phase truth.
3. **Reconstruct.** Decode with `homm3 sema disasm 0x<va>` (`--verbose` bytes+
   relocs, `--blocks` CFG, `--base` your side). Conventions: /Gr = free
   functions fastcall (ecx, edx, stack); members thiscall; WINAPI stdcall.
   Model real types in the owner header (`include/<tu>.h`), pads sliced only
   where bytes prove a field; SIZE() asserts are clang-arm only — VC6 does NOT
   check them, so re-verify stride arithmetic by hand (the 187*0x70 incident).
**NEVER CHAIN THE BOOTSTRAP WITH `&&`.** `homm3 build && homm3 delink &&
homm3 build` SHORT-CIRCUITS: if the first build exits 1 - which it does
whenever inherited rows are sitting under flat carve names - delink never
runs, the second build never runs, and the numbers you read are a lie about
a stale tree. Use `;` between them. A lane lost its whole baseline to this
and diagnosed twelve phantom regressions before noticing.

4. **Build + score.** `homm3 build --fast` for the inner loop. After ANY new
   claim or DATA lands, run `homm3 delink` ONCE (the target side must relearn
   names), then `--fast` again. Scores: filter `build/objdiff/report.json` by
   unit (report addresses are obj-local — count identity, not RVAs).
5. **Iterate.** `homm3 sema diff 0x<va> --asm` (masked; reloc-name-only rows on
   data are cosmetic), `--branches` for the structural signal the masking hides.
   For real deltas compare UNMASKED (`disasm --base` vs `disasm`) — masking
   hides immediates (the IDC_ARROW and 0x54cc bugs were invisible masked).
   Structural versions first (homm2 doctrine): pick the retail-compatible CFG
   family before micro-spelling.
6. **Finish.** Full `homm3 build` exit 0 "ratchet clean" — read the exit code
   UNPIPED (`homm3 build; echo $?`); `build 2>&1 | tail; echo $?` reports the
   pipe's exit and stderr buffering displaces fatal-gate lines to look early. Residual left? Write a
   house-style comment: `// Residual (NN.N%): <delta> - tried and rejected:
   <spellings>.` Never record scores-as-claims in §5 without the ratchet
   agreeing.

**A CARCASS STUB IS A GOOD ENOUGH CALLEE — RE-TRY ANYTHING PARKED ON THIS**
(2026-08-20). objdiff scores a relocation whose target is a working label as
MATCHING, so a caller can reach byte-exact while its callees are still
`// @stub`. `type_AI_spellcaster::get_enchantment_function` closed at
100.0000 with two of its 37 address-taken callees unreconstructed. The same
thing from the other side: renaming `SpellCastWorks` to
`ValidSpellTargetArmy` moved 7 relocations and changed no score at all.
So "its callees are not emitted yet" is NOT a reason to park a function, and
any row parked on that reasoning should be re-tried.

**AN INCLUDE-SET MEASUREMENT DOES NOT SURVIVE A MERGE** (2026-08-20). The
wall counts DECLARATORS, not edits, so two header additions that are each
individually ratchet-clean are not jointly clean. Two lanes added declarators
to `cmbtmgr.h`; each measured its own as neutral on its own branch, and the
merged count regressed `command::GetCommand` 92.5714 -> 92.5357 exactly as
one of them had predicted for a single bare declarator. Re-measure the
include-set-sensitive rows after every merge that touches a shared header.

**THE WALL SATURATES** (measured 2026-08-20, and it CORRECTS two headers that
recorded the opposite as fact). `armygrp.h`'s SSpellTraits note and `army.h`'s
EArmyCreatureId note both said ANY enumerator costs `initialize_game_data`
100.0000 -> 96.0880 and that "the wall fires on the FIRST one and no count
restores it". Measured cleanly: two enumerators cost exactly what one did, and
flattening the rest of the `ESpellId` roster afterwards - roughly twenty more
enumerators visible tree-wide - left the row AT 96.0880, moved exactly one
other row, and GAINED an exact function elsewhere. So the gates were buying
nothing after the first one. Do not price a header addition as if each
declarator costs again.

**THE WALL IS A STEP FUNCTION, NOT A SUM** (measured across the view audit's
seven commits, 2026-08-20). Removing 89 view macros moved rows
NON-MONOTONICALLY: `initialize_game_data` crossed 100 -> 96.09 -> 94.07 ->
100 -> 94.07, while `command::GetCommand` and `events::monsters_sell_out`
dipped and RECOVERED to max, and `hero::GiveArtifact` round-tripped. So the
include-set class responds to the closure's total declarator POPULATION
crossing a threshold, not to a per-declarator penalty that accumulates. Two
consequences: a lane must not assume its own edit is what moved a row - the
population may have crossed on someone else's - and must not assume a dip is
permanent, because adding further declarators can carry it back over.

**AND THE GATES WERE HIDING REAL MODELLING ERRORS.** Emptying them surfaced
`ExtraInfoUnion` and `CCombatTypeMsg` each DEFINED TWICE with contradictory
member sets, `town::get_army` declared with two different return types (the
DC mangled name `QAAAAV` settles it: a reference), `SPELL_SACRIFICE` in two
`ESpellId` arms, and four enumerators duplicated between `EArmySpellRowId`
and `ESpellId`. A per-TU view lets two incompatible models of the same type
coexist indefinitely, because no TU ever sees both.

**BUT DO NOT OVER-GATE FOR IT.** The ledger tracks `cur`, `max` AND `hist`,
and the ratchet compares against `max`: a perturbation lowers `cur` while
`max` and `hist` keep the peak, and even `--accept-regressions` leaves `hist`
intact (`status.py:321` says exactly that when it reports one). So an
include-set perturbation costs a RE-MEASURE by a later lane, not the result.
Gate a declarator when it is genuinely compile-required — a name collision, a
type not every TU can see — not merely because it moves a score. 91 view
macros guarding 238 gate sites accumulated on the unstated assumption that
any perturbation is damage. It is not.

**DECLARE-BUT-DO-NOT-DEFINE TO FORCE A SPECIAL MEMBER OUT OF LINE** (2026-08-20).
The first lever that actually works against the OVER-inline family. If retail
CALLS a compiler-generated copy-assign / copy-ctor that our compile expands
inline, declare it in the class and give it no definition:
`hero& operator=(const hero&);`. The implicit member disappears, the call goes
out of line, and the linker is happy because retail's own `??4hero` COMDAT is
the real definition. `CAdvMgrNetMsgHandler::HandleNetMsg` went **28.43 ->
95.76 on that one line** (two inlined 0x492-byte copies collapsed), with zero
tree-wide fallout.

**predict-inline's CALL-MULTISET DIVERGENCE IS NOT ALWAYS THE INLINER.**
Three misreads proven in one lane, each with a different real cause:
- **tail duplication** — VC6 duplicated a shared join call into both arms of
  an if/else, so the count differs with no inlining involved. `SetPointer` is
  cross-TU and can never be an /Ob2 candidate; a ternary merged the sites.
- **cross-jumping** — `ShowRoute`'s "missing" CompleteDraw/UpdateScreen calls
  were merged tails, not expansions.
- **constant-folded flag arguments** — per-copy block differences that look
  like divergent expansions. VC6 cannot propagate a tested global across an
  opaque call, so if the diagnosis requires that, it is wrong.
Before acting on an inliner route, check whether the callee is even a
candidate (cross-TU ones never are).

**VC6 INITIALIZES MEMBERS IN DECLARATION ORDER, NOT INIT-LIST ORDER**
(byte-inert A/B). Reordering the init list changes nothing. But a member
assigned as the FIRST BODY STATEMENT schedules after the later members'
default construction - worth +7.3 on `advManager::advManager`.

**THE SWITCH/IF-CHAIN TELL IS THE LOWERING, NOT THE COMPARE ORDER**
(2026-08-20, measured both directions in one lane). VC6 lowers a `switch`
with `dec/je` and SUNK arm bodies; an if-chain with `cmp/jne` and INLINE arms.
`DoVictory` needed an if-chain where a switch was written; `ViewArmy` needed a
switch where an if-chain was written. Read the lowering, do not guess from how
the conditions look.

**DECLARATION ORDER BEATS SCHEDULING.** When retail's loads sit in FRONT of a
constructor call, that is declaration order, not a scheduler decision - no
scheduler may move a load across an opaque call. Moving a loop iterator's
declaration AND initialiser above the string/vector locals took
`show_looted_artifacts` 97.41 -> 100.

**SMALL SPELLINGS THAT ARE NOT INTERCHANGEABLE** (same lane):
- `std::_cpp_min`, NOT `std::min` — windows.h's macro breaks the latter.
- A const-ref argument needs a REAL LOCAL: `_cpp_min(member, cast)` binds the
  member lvalue directly where retail copies to a temp. Landing the result in
  a third `int` local rather than assigning into a `short` stopped a
  re-narrowed load (+3.5).
- `= {0}`, NOT `= ""`, for a zeroed char buffer — `= ""` makes VC6 LOAD the
  literal's first byte.
- A dead store retail keeps needs `volatile`; a plain local is dead-stored
  away (+0.8). Flag it in-source as a codegen claim, not a proven token.
- **A wider load than the store means an int-parameter inline between them.**

**A TERNARY ARGUMENT RETAIL BRANCHES OVER MUST BE AN IF/ELSE OVER TWO CALLS**
(2026-08-20). `DoBolt` 84.83 -> **98.22** in one edit - the whole register
allocation fell into line behind it. `why-branch` named it exactly (D8/D13)
from a one-branch CFG-count difference, so run it on any residual that reduces
to a single branch.

**A SHARED STORE AT THE FOOT OF AN IF/ELSE MUST BE MERGED, NOT DUPLICATED**
into an early-return arm. Rewriting that way closed `LoadSpellEffect`
(89.68 -> 100) AND lifted an unrelated caller 87.03 -> 88.12.

**`(r) | (g) | (b)` EMITS RIGHT-TO-LEFT** - which is what fixes the channel
assignment of a mask triple (`DrawBolt` 71.63 -> 76.90).

**LOCALS ONLY READ ON A LATER ITERATION ARE UNINITIALISED IN RETAIL**
(`ChainLightning` 95.30 -> 96.19).

**A MEMBER FUNCTION NEVER DECLARED IN ITS CLASS** produces a cascade of C2109
"subscript requires array or pointer type" and C2228 on every `this`-relative
member, **at a line number that MOVES as you edit unrelated code**. It reads
exactly like a VC6 capacity limit and is not one. Check the class declaration
first; a 20-line probe TU isolates it in one compile.

**STATEMENT-SCOPED `#pragma inline_depth(0)` IS THE LEVER FOR THE
"MODEL-REFUTING" CLASS** (2026-08-20, found INDEPENDENTLY by two lanes in one
round). Where retail refuses an expansion the RE'd /Ob2 budget rule must
accept, you do not need to explain the refusal - you can impose it. The pragma
is STATEMENT-granular, so pin the SITE.
`advManager::Open` 52.72 -> **97.80** (two pins), `hero::HeroFn_004D8B30`
55.74 -> **94.67** (one line), `CancelIndividualSpell` 21.50 -> **96.27**,
`InitClean` 0 -> **92.63**. Route it with `predict-inline`'s OVER-inline
column.

Everything previously recorded as unreachable in this class had aimed at
moving the BUDGET - spellings, pragma-on-instantiation, 100/200/300-statement
dose titration, an RTM-vs-SP3 compiler A/B. Those measurements were right and
their conclusion was wrong.

**Bounds, all measured:**
- **Only N=0 bites.** Mid-function `inline_depth(1|2|3|4)` is inert - measured
  twice in one function and four times in another, and again at FILE scope
  around a whole function (townmgr `BuyBuild`, `inline_depth(1)` byte-flat
  with the control at the same placement moving it 68.0799 -> 50.8477). So
  "inline the parent, call the child" is NOT spellable at any placement, and
  the several tree rows whose residual is exactly that shape - retail expands
  `vector::insert` and CALLS `_Ucopy`/`_Ufill`/`_Construct` while we expand
  both levels - are unreachable while the callee lives in a vendored header
  you may not pin inside. When the container is HAND-MODELLED the pins go in
  its own body and it IS reachable (see cmbtmgr's insert below).
- **A statement pin inside a shared inline function is a per-CALLEE knob, not
  a per-call-site one.** Pinning `getCellData` inside findpath's `mark_enemy`
  carried into every expansion: `FindCombatPath` 46.4584 -> 51.5746 and
  `mark_teleport` 100.0000 -> 82.7826, a net loss. Only sites written in the
  caller's own body are per-site.
- Consequently, anything retail keeps INLINE must be hoisted out of the pinned
  statement first - subscripts, `begin()`/`end()`, the call result into a
  named local.
- **It backfires when retail keeps only a nested CHILD out of line**, and
  hard: `BVResMsg` 93.33 -> 26.42, `BVMessage` 92.68 -> 21.28,
  `hero::initialize` 82.86 -> 62.31, `hero::hero()` 70.99 -> 53.99. Use it
  only where retail keeps the WHOLE callee out of line.
- The pin DOES reach a member-initialiser expansion (contra the eh-cleanup
  caveat), and pinning a later site does NOT shrink the
  `budget / sites-remaining` divisor for an earlier one — **but that does not
  run backwards: pinning an EARLY site ENLARGES the divisor for later ones**,
  so an early pin can produce retail's call exactly and still lose points to a
  knock-on over-inline (measured −6.6 on `~NewfullMap`).
- **On a `return` statement the pin reaches the local's scope-exit
  destructor** — `game::Load` +1.85, `game::Save` +2.56, matching retail's
  census exactly (3 out-of-line `~SavedGameHeader`, 1 expansion). **Only for a
  FUNCTION-scoped local**: on block-scoped ones the identical `predict-inline`
  signal loses hard (−6.4 and −5.9 on two others).
- A pin can retire a `#pragma inline_depth(0)` SCAFFOLD elsewhere: pinning the
  two rejected sites in `spell_is_valid_on_target` let
  `get_total_combat_value` drop its scaffold and go 46.91 -> 94.74 with four
  neighbouring rows holding.
- **THE PIN REACHES A `return` STATEMENT'S SCOPE-EXIT DESTRUCTOR** (game lane,
  2026-08-20). `#pragma inline_depth(0)` on `return 0;` alone forces the
  FUNCTION-SCOPED local's destructor out of line at that exit and leaves the
  early-return exits expanded - which is exactly the split retail has when it
  calls a big `~T()` at the normal exit and expands it elsewhere. `game::Load`
  83.30 -> 85.15 on the one line, and the same lever on `game::Save`'s two
  tail exits took it 80.09 -> 82.65 and matched retail's census exactly (3
  out-of-line `~SavedGameHeader` calls, 1 expansion).
  **BUT ONLY FOR A FUNCTION-SCOPED LOCAL.** On a BLOCK-scoped one the same
  pin reaches a different cleanup and loses hard - `readMonsterData`
  96.47 -> 90.04, `TTimedEvent::Read` 72.92 -> 67.03 - even though
  predict-inline's `_Tidy` census names the identical `base x0 vs retail x1`
  shape in all four cases. Check where the local is declared first.
- **PINNING AN EARLY SITE ENLARGES THE BUDGET FOR THE LATER ONES.** The rule
  recorded above - that pinning a later site does not shrink an earlier one's
  `budget / sites-remaining` divisor - DOES NOT RUN BACKWARDS. `~NewfullMap`:
  `inline_depth(0)` on the leading `delete[]` produces retail's
  `call ??_ENewmapCell` exactly and still LOSES 6.6 points (95.88 -> 89.29),
  because the freed budget is then spent over-inlining `~vector<BlackBoxData>`
  further down, which retail calls. After a pin, re-read the WHOLE call
  multiset, not just the site you aimed at.

**A REJECTED PIN IS ONLY REJECTED FOR THE INLINE STRUCTURE IT WAS MEASURED
IN** (2026-08-20, cmbtmgr). `#pragma inline_depth(0)` on
`SetupAndLoadObstacles`'s PlaceObstacle call was measured, recorded and
rejected at 64.8103 -> 58.9216. After an EARLIER decision in the same body
changed - `TObstacleVector::insert` started expanding - the identical pin
GAINED 10.9 (78.9588 -> 89.8722). Re-try every rejected pin in a function
whose inline structure has since moved, and never carry a pin verdict
across such a change. Two corollaries measured in the same body:
- **Hoist what retail keeps inline OUT of the pinned statement, always.**
  The same pin also de-inlined the `obstacles.size() - 1` sitting in its
  statement; landing that in a named local first is worth **+6.50** on
  `place_obstacle` (67.1937 -> 73.6911) and +0.54 on findpath's
  `PushPoint`. This is the single most repeated cheap win of the lever.
- **A `return` statement is a pinnable site.** Retail destroyed
  `place_obstacle`'s picker OUT OF LINE on the failure path and inlined
  the delete on the success path; `inline_depth(0)` on the `return 0;`
  alone reproduces the split (+1.15).

**`inline` ON AN OUT-OF-CLASS DEFINITION IS LOAD-BEARING FOR /Ob2**
(2026-08-20, and it bounds the "definition order is not the lever" note).
VC6's /Ob2 auto-inliner does NOT take a large body from a plain
out-of-class definition: `combatManager::TObstacleVector::insert` (740 B)
stayed a CALL at both of its sites and every score held identical to the
digit, whether the definition sat before or after its callers. The same
compile DID auto-inline the 59 B and 49 B helpers beside it. Marking the
definition `inline` is what made it expand, and it is the whole
difference between "no movement at all" and `SetupAndLoadObstacles`
64.8103 -> 91.5113. So when retail expands a hand-modelled container
member in one caller and CALLS it in another, the recipe is: define it
`inline` in the .cpp (not the header - only that TU needs a body), pin
the caller retail calls it from, and pin the helper sites inside it that
retail keeps out of line. A defined-but-unclaimed symbol costs nothing:
objdiff adds no report row for a base symbol with no delinked twin.

**predict-inline's OVER-inline bucket cannot tell a mangled-name divergence
from an inlining decision.** ICF makes this worse than the entry below
records: `vector<widget*>::push_back` and `vector<int>::push_back` fold to
one COMDAT, so the delinked target names the OTHER instantiation and the
tool reports `base x0 vs retail x3` for a pair that is already identical.
Check element size before believing a template-callee divergence. `TownQuickView` reported `get_army base x0 vs
retail x4` because retail calls the CONST overload - a different function at a
different address. Check the overload before reaching for a knob.
**And the const-overload case has a source cause worth knowing**: three game
rows wrote `const_cast<armyGroup&>(town->get_army())` on a NON-const `town*`,
so the call resolved to the non-const overload and the const_cast was a no-op.
`static_cast<const town*>` on the RECEIVER selects retail's overload; the
bytes do not move (/OPT:ICF folded the two onto one row) but the divergence
row disappears, so the lead stops costing builds.

**A `_Tidy` COUNT IS A COUNT OF GUARDED `return`s, NOT AN INLINING DECISION**
(game lane, 2026-08-20, and it CORRECTS a diagnosis banked as fact). A
function holding one `std::string` local emits, at every guarded early
return, `push 1 / lea ecx,<string> / mov [ebp-4],state / call _Tidy / jmp
shared-tail` - one cleanup site per return, each with its own unwind-state
pair. So `_Tidy base x37 vs retail x39` did NOT mean two inlining decisions;
it meant TWO MISSING STATEMENTS. Segment both call sequences by the anchors
that resolve identically on both sides, count `_Tidy` per span, and the
missing code localises to a span you can then read straight out of retail.
`game::Load` 74.77 -> 86.75 that way, on four statements the census pointed
at (a version if/else that duplicates a guarded read, a separately-read byte,
a read-and-discard dword, and a whole gMapExtra plane).
Two consequences: a span where OUR count is HIGHER is retail sharing one
cleanup site between two conditions - the merged-return class, and an `||`
spelling does NOT reproduce it (measured, -2.5); and the mirror serializer is
the best oracle there is for what the missing statement says.

**A CROWD OF `basic_string` INTERNALS IN THE UNMATCHED COLUMN IS ONE INLINED
STRING MEMBER.** `readHeroData` showed `_Grow x4`, `_Split x2`, `_Eos`,
`memmove`, `char_traits::assign`, the free `std::_Xran` and the
`exception`/`logic_error` constructors - fifteen unmatched calls against
retail's eight - and the standing note read them as a `bitset<70>::_Xran`
throw path and concluded the wall was unreachable because "auto_inline cannot
reach a library template this TU never defines". They were the guts of ONE
`basic_string::assign(const basic_string&, size_t, size_t)` that retail calls
and we expanded. A statement pin on the assign took it 71.91 -> 83.77 on one
line. `auto_inline` genuinely cannot reach such a callee; `inline_depth(0)`
does not need to, because it suppresses expansion at the CALL SITE.

**WIDEN A PARAMETER FAMILY, NOT ONE END** (2026-08-20 — a method correction
that had produced a wrong recorded identification). A callee's parameter width
is invisible to width-blind callers, so widening the CALLEE ALONE can cost
several exact functions and look refuted, while widening the WHOLE family
leaves every one at 100 and gains the right compare.
`SpellCastWorkChance`'s slot 6 was recorded as `unsigned char` on exactly that
mistake; the body reads it `cmp dword ptr [ebp+0x1c], 1`. Six declarators
widened together, six rows migrated, nothing else moved.

**`army::Is(n) & 1` IS THE ATTRIBUTE-TEST SPELLING, AND IT CLOSES REGISTER
WALLS.** The header inline truncates the shifted word to a byte before the
caller's mask, and that truncation is what stops VC6 folding the test back
into `test dword ptr [mem], imm` on the member. `SpellCastWorkChance`
97.10 -> **99.78**: the member stays live in EDX across a 40-arm switch, the
other value falls back to its memory home, and every downstream scratch rename
disappears. The lever is the TRUNCATION — a hand-written
`unsigned a = x; (a >> N) & 1` scores 97.10, and naming the shifted value is
byte-identical.

**GOTO LOOPS ARE WORTH TWELVE POINTS** where retail's back edge is
`cmp / jle <exit> / jmp <head>` and a `for(;;)`+`break` gives one inverted
`jg <head>` (`SummonElemental` 81.26 -> 93.38). A do/while scored 85.69. The
same edit also splits an inlined destructor into retail's two copies.

**A RESIDUAL NOTE IS A HYPOTHESIS, NOT A FACT — AND THEY DECAY.** Four
separate times in one day a recorded note sent a lane the wrong way, and the
notes were written in good faith by lanes that had measured something real:

- **A conclusion can be right about the mechanism and wrong about the
  verdict.** `Open`/`ShowRoute` were "model-refuting, no known lever" — true
  of everything tried, all of which aimed at the BUDGET. The pin imposes the
  refusal instead: 52.72 -> 97.80.
- **A "TRIED AND REJECTED" measurement is CONTEXT-DEPENDENT.** The
  PlaceObstacle pin was banked as "LOSES 5.89"; once an earlier inline
  decision in the same body moved, the identical pin GAINED 10.9. **Re-try
  rejected knobs after anything upstream in that body changes.**
- **A note can name its own answer and walk past it.** `readHeroData`'s note
  blamed `bitset<70>::_Xran` and declared the wall unreachable while naming
  `basic_string::assign` in the same paragraph: +11.86 on one line.
- **A note can be wrong about WHICH sites are involved.** `readObject`'s said
  the wall was the FACTION arm "whose two siblings already match"; retail
  calls `insert` at ALL THREE dwelling arms, so every attempt that pinned
  FACTION alone lost because arm two was still expanding. **That misreading
  cost three lanes** before someone re-counted. 45.10 -> 96.98 once all three
  were pinned.

Read the note for its EVIDENCE and re-derive its conclusion. Cite the bytes,
not the prose.

**A SUNK JOIN BLOCK — TWO SHAPES, AND THEY WANT OPPOSITE THINGS.** VC6 sinks a
multi-predecessor join to the end of the function, but whether source order
can move it depends on how the predecessors reach it.

- **Both predecessors are JUMPS -> write the block TWICE.** Four single-copy
  spellings, including the exact idiom another window uses, produced the
  identical object. Duplicating leaves the first copy with ONE predecessor, so
  nothing sinks it, and the cross-jumper then merges the shared tail behind a
  `goto` — retail's one-tail shape.
  `TViewArmyWindow::WindowHandler` **35.53 -> 73.63**.
- **One predecessor FALLS THROUGH -> a `goto` into the surviving arm merges it
  in place**, and SOURCE ORDER DOES MOVE IT. Which arm carries the label is
  readable from the bytes: retail's teardown as the fall-through successor of
  the second test (`jb <teardown>` … `jae <skip>`) means the label goes in the
  SECOND guard and the first `goto`s forward into it; two jump predecessors
  mean the label goes in the count-write's own `if` and the loop `goto`s
  backward. Writing the loop case the other way round (loop inside an `if`,
  `return -1` in a trailing `else`) merges the site but SINKS the block —
  0.02 lower and the wrong layout.

**Count the `[ebp-4]` cleanup sites to tell whether a merge exists at all.**
Retail emits 37 numbered sites in `game::Save` against our 39, and 39 in
`game::Load` against our 43; every surplus is one `return -1` reached by two
conditions. That census is what turns "not a spelling" into an edit.

**And these knobs rank NON-MONOTONICALLY in combination** — goto+narrow 84.31,
`||`+wide 84.34, goto+wide 84.20, `||`+narrow 82.96. Measure the PAIR, not
each knob. A banked "`||`-with-comma MEASURED AND REJECTED, −2.5" was
rejecting the wrong thing: the merge is real, the `||` just lowers it as a
two-jump join that sinks.

**A STATEMENT PIN IS A DROP-IN REPLACEMENT FOR A VIEW GATE** that existed only
to keep a body out of a TU, and it costs no declarator anywhere:
`town::BuildBuilding` 78.58 -> 99.30 (7 sites), `get_growth_rate` -> exact
(1 site).

**THE DC XREF CENSUS READS BODIES THE LINE TABLE CANNOT.** Where
`*** SRCLINES ***` is a different revision and cannot price a body, the xref
census at the same dc offset still names its missing constructs — that found a
longhand undead scan worth 74.48 -> 79.18. **But the site count is NOT
monotone**: two other census-agreeing edits cost 2.4 and 2.9. Use it to find
candidates, then measure each.

**"DO NOT CACHE WHAT RETAIL RELOADS" CAN BE MASKED BY A TYPE ERROR.** One
function needed the re-reads AND two `unsigned short` truncations together;
either alone is a LOSS (the casts alone measured below baseline), both give
+4.12. If a known-good lever measures negative, check the types before
discarding it.

**WHERE RETAIL COMPUTES BOTH OPERAND ADDRESSES AHEAD OF A SHORT-CIRCUIT, NAME
THEM AS `const T&` LOCALS.** `a.x && b.x` written as two subscripts makes VC6
fold the offset into each load and defer the second address chain past the
branch; retail forms both with `lea` first. Six rows moved on one edit —
`combatManager::Open` **+16.52** and `damage_message` **+9.20**.

**A COORDINATE RETAIL DERIVES FROM A JUST-STORED MEMBER MUST BE SPELLED AS A
READ-BACK OF THAT MEMBER.** Neither the folded expression (VC6 never
re-associates it) nor a named local affine in the loop counter works — the
latter gets STRENGTH-REDUCED onto its own induction variable, which retail
does not do. 58.84 -> 54.49 (named local) -> **60.30** (read-back).

**INLINE DEPTH IS NOT A SUBSTITUTE FOR BUDGET.** For an under-inline at depth
3, `#pragma inline_depth(255)` set at the site before expansion begins is
BYTE-FLAT — 85.9895 to the digit. The `budget/(n-k)` quotient is the limiter,
not the depth.

**THE CALLER'S SIZE IS THE OTHER HALF OF THE INLINER, AND ITS DIRECTION IS
READABLE.** `budget = clamp(2*caller_cb, 1000, 35000)`, so the caller's mass
decides what its callees do — and `predict-inline`'s column tells you which way
to push:

- **OVER-inline (we expand, retail calls) -> SHRINK THE CALLER.** Split a
  helper out and the nested STL calls a pin cannot reach come back out of
  line. `FindCombatPath` +5.43, `PushPoint` +9.32 (two doses),
  `PushCombatPoint` +11.66, `readHeroData` +2.96.
- **UNDER-inline (we call, retail expands) -> GROW THE CALLER.** That is
  `THallWindow`'s direction and `StampObject`'s (the shrink lever costs
  StampObject 11.4).

**Check the DC roster for the helper's name before inventing one** — several
are already stubbed in the carcass (`build_combat_path` was). **The second
dose is usually the plain FIELD STORES, not the loop** (+11.66, +2.56). **And
the dose is a PEAK, not a ramp**: a third helper cost 4.70 and 0.29.

**A SHARED INLINE HELPER SERVING TWO CALLERS WITH DIFFERENT RETAIL DECISIONS
MUST BE DUPLICATED.** This retires the "a pin inside a shared inline is a
per-callee knob and therefore a dead end" bound: duplicating the helper turned
a −42 B trade into **+9.50** with the sibling row back at 100.0000.

**A BITSET STORE'S DEPTH DECIDES WHETHER `_Xran` IS CALLED OR EXPANDED** —
`set(i,v)` gives a call, `[i]=v` gives an expansion. `readTownData` **+12.31**,
against a note predicting −2.10 from a "split the range check" fix that was
the wrong mechanism entirely.

**A FN-LEVEL `_Xlen` / `_Xran` RELOC CENSUS COUNTS *EXPANDED* WRAPPERS, NOT
CALLED ONES.** When the append is EXPANDED only its throw survives as a call,
so a surplus of those relocs on our side means WE over-expanded — the opposite
of how it reads. A note that had it backwards left 564 B sitting recoverable;
reading it correctly closed two twins for **+13.9 combined**.

**A FRAME TOO LARGE PLUS AN ENTRY-TIME CONSTANT THE SOURCE NEVER NAMES THERE
= A DEFAULTING DEFAULT CTOR.** The lever is the LOCAL'S DECLARATION FORM, not
the header: declare it with the real ctor at first use instead of default-then-
assign. `ProcessSearch` +0.75, and the frame became retail's exactly.

**A DWORD SELF-STORE IS A STRUCT COPY THE ALLOCATOR COALESCED** — spell the
copy (`type_point step = heroPos;`). `ShowRoute` +2.56.

**ADJACENT `jne` <-> `jb` PAIRS SWAPPING ACROSS SITES = `||` OPERAND ORDER**,
with the bounds-checked operand moved sides. `update_spell_list` +1.46.

**A GUARDED `do/while` SPLITS THE TOP-GUARD SIGNEDNESS FROM THE BACK-EDGE
FORM**, so each end can match retail independently —
`if (n > 0) do { … } while (--k != 0);` matched a `jle` guard AND a `jne` back
edge where a plain `k > 0` loop could only do one.

**AN IMPOSED CALL THAT IS BYTE-RIGHT AT ITS OWN SITE CAN STILL LOSE THE
FUNCTION.** The pin re-prices every LATER site and SHRINKS THE A9 DENOMINATOR,
pushing the quotient past a throw's cost at an earlier one (−7.76 pinned,
−15.05 unpinned, −9.84 on a third). That is the pin's second side effect and
it is why a locally-correct pin can measure negative.

**SWEEP `sub esp` IN BOTH DIRECTIONS, WHOLE-UNIT, BEFORE ANY SOLVER.**
`llvm-objdump` over `build/objdiff/normalized/{base/U.obj,target/U.c.obj}`
gives every frame delta at once, and the SIGN names the missing fact:
- **Frame too LARGE** — a mis-sized array (`char[12]` modelled as `char[8]`
  reads as a ~15% score hole), a value that could not reach a dead parameter
  home (**+8.57**), or a DEFAULTING DEFAULT CTOR whose `{-1,-1}` spills at
  entry (**+0.75**; the lever is the local's declaration form, not the header).
- **Frame too SMALL** — a MISSING NAMED LOCAL. `StampObject` 0x78 against
  retail's 0x7c wanted one reference local,
  `std::vector<TObjectCell>& objectList = thisCell->objects;` — **+5.48** and
  the frame exact. Retail proved it by biasing `[ebp+8]` once and reading
  `end()`, `begin()` and the insert's `_Last`/`_End` through that pointer.

Companion sweep: **positive `[ebp+N]` slots retail writes and we never do** are
the recycled-parameter sites, found mechanically. Validate the base
instruction count first — llvm-objdump truncates EH-bearing base dumps.

**A CALLER-SHRINK DOSE THAT IS TOO SMALL IS BYTE-FLAT, NOT FRACTIONAL, AND
DOSES ARE NOT INDEPENDENT.** The /Ob2 threshold is a STEP, so a null result
means "too small", not "wrong idea".
- `SetSpellInfluence`: three lifts, **each byte-flat alone**, worth **+4.39
  together** — and removing the one that crosses the threshold puts the other
  two back at *exactly* the untouched baseline.
- `FindCombatPath`: four doses, the third worth +0.17 and the fourth **+6.52**.
  A note reading "the two levers do not add" was two doses short, not wrong.
- **When a dose OVERSHOOTS, slice the same block thinner.** `PushPoint`'s
  whole magic_forbidden block measured −4.7; only the cell lookup inside it
  measured **+3.76**. A banked negative can be measuring the dose SIZE rather
  than the block.
So never conclude from a single flat measurement: titrate combinations, and
re-slice a block that overshoots.

**`DON'T CACHE WHAT RETAIL RELOADS` CAN BE ASYMMETRIC INSIDE ONE STATEMENT
PAIR.** Retail named one value in a temp slot and recomputed its sibling with
full addressing at both sites; dropping only the one local paid **+6.10**.

**THE REVERSE-SCAN TELL.** Retail keeping `size` AND `size-1`
(`mov edx,esi / dec esi`) with a back edge testing the PRE-decrement index
means `for (i = size()-1; i >= 0; --i)` over `[i]`, not `size(); i > 0` over
`[i-1]`. The wrong form lets VC6 fold `size()==0` into the guard and collapse
the inlined `vector::size()` null-arm join — ten instructions on a 278 B body.

**AN UNTESTED WARNING IS WORSE THAN NO NOTE.** `army::consider_attack` carried
"it inlines GetSpeed four times and can_shoot once, and would destabilise four
banked rows". Three lanes were told to leave it alone on that basis. There was
no matrix: it closed at **100.0000 on the first spelling**, 623 B, and nothing
else moved. If a note warns you off without showing a measurement, treat it as
unexplored, not as closed.

**A CARCASS CTOR/DTOR OF A WELL-MODELLED CLASS IS NEARLY FREE.** The
compiler-generated member construction and teardown IS the body: `game::game()
{}` scored **74.65** written empty, and `game::~game() { clear_event_records(); }`
scored **78.84** as one line. Read the class layout before writing anything —
on a well-modelled class most of the work is already done for you.

**FOLD N GUARDS INTO NESTED IFS OVER ONE SHARED TAIL** — worth more than the
guards themselves: **63.76 -> 81.63** on one edit, merging nine duplicated
epilogues, and it flipped a float compare to retail's `<=` polarity.

**NAMED `short` COPIES OF A BITFIELD READ ARE COMPILER TEMPS, NOT SOURCE
VARIABLES** — retail stores them as WORDS and RELOADS them each iteration.
Naming four cost four stack slots and 16 frame bytes; reading through gave the
hoist for free. And **if retail hoists a field read out of a loop that stores
to that slot, the store targets a DIFFERENT variable** — the hoist is only
legal then.

**UNDER `/Op`, A QUOTIENT FEEDING A COMPARISON HAS A HOME.** Naming the double
reproduces retail's `fstp t / fld table / fcomp t`; left anonymous it folds to
`fcomp table[8*eax]` with the operands swapped.

**WHICH ARM FALLS THROUGH IS A SOURCE FACT** — retail's `js` to the distance
block means the OTHER arm is the `if` (95.66 -> 97.16).

**A "WANTS A LEANER SPELLING" VERDICT CAN BE A LOCAL MAX.** `BuyBuild`'s own
note predicted the leaner direction; hoisting as it suggested costs **4.01**.
Both directions are now measured worse — 68.08 is a local maximum. Measure
both before acting on a directional verdict.

**THE FRAME IS A SIGNAL, NOT AN OBJECTIVE.** Reaching `StampObject`'s exact
0x7c frame by hoisting a local one scope further out costs **12.6 points**.
Use the frame delta to name the missing fact, then find the spelling that
produces it — do not chase the number.

**AN ADDRESS-TAKEN LOOP COUNTER BLOCKS STRENGTH REDUCTION.** `readMapObjects`'
`i` must be `int`, not `unsigned`, so `vector<int>::push_back` binds
`const int&` to `i` ITSELF — that takes its address, forbids induction-variable
rewriting, and makes retail rebuild every subscript from `i` across four loops.
One word, **+2.99**. **The same mechanism from the other side:** a member
re-read inside a loop blocks it too, so naming it once per iteration gives
retail's `lea/dec/movsx` walk (**+7.22**).

**BLOCK SCOPE IS WHAT PUTS A LOCAL IN A DEAD PARAMETER HOME.** Function-scoped,
`int int_buffer` gets its own slot; block-scoped, VC6 puts it in retail's own
`[ebp+8]` with the flag byte at `[ebp+0xb]`.

**BIND A BY-VALUE STRING RETURN BY `const&`** — VC6 does not elide the copy
into a named `std::string`. **+1.67** and 16 frame bytes.

**A BRANCHED TERNARY ARGUMENT WANTS `x = a; if (c) x = b;`**, not a symmetric
if/else — retail loads the default unconditionally (93.33 vs **95.70**).

**DECLARATION ORDER IS OBSERVABLE WHEN THE TWO LOADS COME FROM DIFFERENT
SOURCES** — reading a memory operand before one the guard already left live in
a register. **+2.08 / +2.32** in two loops of one function.

**WHAT CARRIES CALLER-cb, MEASURED THREE WAYS** (2026-08-20, on an 11.5 KB
body where the whole search turned on it):
- **Folded constants carry ZERO cb.** A 300-node `0|0|…|0` dead store is
  byte-flat to the digit. That kills the folded-constant respelling family
  everywhere — it is not a dose, it is nothing.
- **Statements under `if (0)` carry FULL cb, byte-inertly.** Sixty wrapped
  probes reproduced the naked plateau exactly. This is the clean carrier —
  **and therefore a MEASURING INSTRUMENT. Titrate with it BEFORE writing any
  real edit.** On `readMapObjects` the curve showed a narrow 5-to-9-statement
  plateau (87.28 → 94.29); knowing the target was that SMALL is what pointed
  at a variable's scope rather than a lifted block, and the real spelling then
  BEAT the synthetic carrier (**+7.58** vs +7.01). It also settled three
  functions' directions in one build each, before anything was spent:
  two monotone-down (shrink), one up (grow). Measure the dose, then go
  looking for a real construct of that size.
- **…UNLESS the `if (0)` block contains candidate CALL SITES**, which inverts
  it (84.46 and 79.91 against a 99.66 plateau) — dead sites still enter the
  collector.

**PIN A LOOP CONDITION ALONE.** Pragma state is per-site at collection, so
`#pragma inline_depth(0)` before a `while (i < v.size())` with
`#pragma inline_depth()` as the first BODY line confines the pin to the
condition: `size()` goes out of line while the body's subscript stays inline.
`game::Save` **+3.68**.

**FRAME TOO SMALL: PROMOTE BLOCK-SCOPED BUFFERS. FRAME TOO BIG: DELETE A
CACHE.** Both measured on the same pair of functions, both frames now exact —
`game::Save` gained two dwords by promoting five serialization buffers to
function scope; `game::Load`'s 8-byte surplus WAS a `saveVersion` local, and
retail re-reads `saved.version` at all eighteen uses.

**RETAIL `neg` RUNS ARE INLINE SUBTRACTION STRENGTH-REDUCED ONTO A NEGATED
INDUCTION** — spell `LIMIT - i` inline; a named decrementing counter never
produces them (`Armageddon` +2.89).

**A MAP SUBSCRIPT WRITTEN LONGHAND IS A HAND-EXPANDED ACCESSOR.** Retail
reloading `gpGame` three times and re-reading `worldMap.Size` through each
fresh copy is not a register wall — it is exactly what `game.h`'s own
`NewfullMap::cell(x,y,z)` emits per expansion. Writing the accessor instead of
`worldMap.cellData[(z*Size + y)*Size + x]` paid **+10.58** and **+5.62**, and
an EBX/EDI transposition three lanes had recorded as a register wall went with
it. **DIRECTION-DEPENDENT**: on an UNDER-inlining body the accessor COSTS 6.6.
Check `predict-inline`'s column first.

**THE /Ob2 BUDGET HAS A NUMERATOR, AND IT IS EASY TO MISS.**
`budget = clamp(2*caller_cb, 1000, 35000)`, and every recorded attempt on one
function had moved the DIVISOR (candidate sites: +6.9, +4.7, −2.42, −2.89,
−2.96) before concluding the site count is not monotone. True, and not the
whole knob — lifting two blocks into a single-call-site static drops
`caller_cb`, the budget falls, and the over-inlined Dinkumware goes back out
of line: **+12.20 with no statement changed**, and +1.94 on its twin. The dose
is a PEAK — three neighbouring lifts measured −12.6, −9.2, −2.1.

**A `_Tidy` CENSUS SAYS HOW MANY SITES DISAGREE; ONLY THE ORDER SAYS WHICH.**
`TTimedEvent::Read` carried "MEASURED AND REJECTED: `inline_depth(0)` on this
return, 72.92 → 67.03". The census was exact (retail calls at three sites, we
at two); the PLACEMENT was not. Reading the sites in order showed retail calls
at the first two and expands at the third while we matched the first and
third. One pragma on the right guard: **+26.55**. `inline_depth(1)` at the
same site is byte-flat — a clean confirmation of "only N=0 bites" on a body
where N=0 moves 26 points.

**DIFF CALL STREAMS IN ORDER, NOT AS A CENSUS.** Every win in one lane's round
came from reading the two sides' call sequences positionally, and FOUR of them
were invisible to a count — equal totals, or pairs the tool discounts as
name-unresolvable. A census says how many sites disagree; only the order says
which.

**CONSTRUCTOR CALL STREAMS ARE THE CHEAPEST LAYOUT ORACLE** — which class owns
a member is decided by which ctor constructs it. `heroPlayerSetups` was
modelled on `NewSMapHeader`; `CMapHeaderData`'s own ctor writes the `_Tree`
triple and buys the head node while `game::game` has no `_Tree` call at all.
Moving it: **+2.46** and one more exact function tree-wide.

**A CONSTANT-STORE FILL LOOP REACHES `rep stosd` ONLY WITH THE VALUE IN A
NAMED LOOP-INVARIANT LOCAL.** `bits[i].set()` stores the immediate and C2's
store idiom declines the fill (31.40); `bits[i] = allPlayers` with the value
named fires it (87.85).

**THE CONDITION-ONLY PIN GENERALISES FROM LOOP CONDITIONS TO `if`
CONDITIONS** — close the pragma between the condition and its statement, and
no local is needed. `game::Save` **+4.38**, against a banked "narrowing
measured worse" that had been measuring a different narrowing (one that added
a local).

**THE ACCESSOR TELL DOES NOT APPLY WHERE THE ACCESSOR IS COMPILED OUT OF
LINE.** `advmgr.cpp` defines `HOMM3_NEWFULLMAP_CELL_OUTOFLINE` — one of the
three surviving compile-required gates — so writing `cell(x,y,z)` there emits
a CALL rather than the reload pattern. Check whether the TU opts out before
converting a longhand subscript.

**A `_Tidy` / `operator delete` CENSUS NAMES THE DIRECTION, AND "THE DOSE WAS
TOO BIG" IS NOT "THE LEVER IS WRONG".** `do_post_attack`'s note read
`_Tidy 5 vs 6, delete 4 vs 3` and called it a hard class — that census IS the
over-inline signal, and two shrink doses closed the multiset (**+3.43**).
`readHeroData`'s banked "artifacts-block caller-shrink LOSES 1.96" was the
wrong dose SIZE: the four-iteration primary-skills loop pays **+0.82** and
brings `logic_error(const string&)` out of line at both throw sites.

**A LOOP COUNTER RETAIL STORES IN THE PROLOGUE BELONGS IN THE PROLOGUE.** A
banked note read "budget must be declared next to its use" (78.30 → 78.54);
the bytes say retail stores `mov [ebp-0x30], 0x7f` at fn+0x2d and reuses that
slot for a later loop counter. It only pays PAIRED with moving the `memset`
below the initialiser run — retail emits six zero-stores then `rep stosd` —
and the memset move alone is −0.35. Two knobs, neither of which works alone.

**A "LOCAL MAXIMUM" VERDICT EXPIRES WHEN A NEW LEVER LANDS.** `BuyBuild` was
banked as one — both directions measured worse, including a +20-statement
probe — and I repeated that in three briefs. The note was sound and simply
PREDATED the numerator lever: two single-call-site lifts took it
**68.08 → 95.12** (+7.33 and +19.71) and the call multisets now agree 43 = 43.
**When a new lever lands, re-open every row whose verdict was reached without
it** — a directional measurement only bounds the knobs that existed when it
was taken.

**THE NUMERATOR HAS A TITRATABLE THRESHOLD.** `calculate_demand` flips at
exactly SIX inert statements — 5 gives 93.40, 6 gives **97.43**. And
self-assignments of a spent local are the only measured-inert mass carrier
(four counter-measurements banked). Caller-shrink moves up to ~20 points per
dose and is titratable against the `predict-inline` census.

**SPLIT THE STATEMENT TO SPLIT THE PIN.** A statement-granular
`inline_depth(0)` cannot separate two uses of the same accessor in ONE
statement — but two statements can: unpinned
`tail = (count < size() ? size() : count)` then pinned `grown = size() + tail`
reproduced retail's one CSE'd expansion plus one call (**+3.79**, census
exact). A note claiming the pragma made a call count "the floor" was reading a
statement boundary as a compiler limit.

**IN-LOOP `return` vs `break`** — a `return` inside a loop is a second
scope-exit destructor path the cross-jumper cannot fully merge; `break` to a
single exit collapses two cross-jumped `operator delete` heads into retail's
one block (**+1.00**).

**MIRROR REAL DINKUMWARE HELPER SIGNATURES** — by-value bounds parameters on
`fill`/`copy_backward` are byte-load-bearing (**+0.87**), and a named
`shifted = end - count` reproduces the spilled CSE (**+0.47**).

**A BY-VALUE RECORD COPY IN A LOOP IS A SOURCE FACT.** A three-dword copy
reads the discriminant once and strength-reduces the walks into a count-down;
`const&` re-reads it per iteration and pins an indexed up-count (**+7.03**).

## The proven levers (all byte-verified in this tree — try in this order)

- **Adjacent early-out guards**: retail merges `if (a<0) return E; if (a>=N)
  return E;` into one inline block. Our CL: split ifs = closest (duplicated
  inline); `||`/goto spellings get re-threaded into a SUNK shared block —
  strictly worse. Known residual class when retail's merge is unreachable
  (path.obj) — see "compiler-generation" below.
- **goto loops**: a top-tested loop with ONE call site and no import hoist =
  retail source used `goto`; while/for(;;)+break get rotated (duplicated
  condition) AND win the import an LICM hoist
  (kbwin::Process1WindowsMessage). VC6 does not rotate or LICM goto flow.
- **DO NOT CACHE WHAT RETAIL RELOADS — this is worth more than it sounds**
  (mapcell GenerateHeightMap, 2026-08-20: caching a `CObjectType*` across the
  loop measured **68.17 against 96.75 for re-subscripting every iteration**,
  a 29-point penalty, and hoisting the whole subscript above the `memset`
  went further backwards to 64.89). Retail frequently keeps only a byte
  OFFSET live and reloads the container's `_First` each pass. The instinct to
  hoist an invariant load is usually wrong here: read what retail keeps live
  across the back edge and spell that, even when it looks redundant. **The
  loop BOUND obeys the same rule**: `game::GetNumThievesGuilds` caps at 91.89
  with `numTowns` hoisted into a local and reaches EXACT with
  `i < players[iWhichPlayer].numTowns` left in the condition, re-read every
  iteration. Related smaller instances: `get_trigger_cell` loses 5 points assigning `z` before
  `y`; `calc_cell_extra` needs `is_trigger = 1` AFTER `type_value`, not
  before; `PlaceObject` needs `(col & 0xf) | (row << 4)` and not the operands
  reversed — with `|`, operand ORDER decides which half accumulates (96.80
  vs 100.0).
- **A sibling function is the cleanest control for "source or codegen?"**
  readBlackBox's three lists each need an explicit `if (count == 0) clear();
  else { resize(count); <loop> }` — plain `resize(count)` scores 54.65 and
  leaves the CFG one branch short, the guarded form 93.01 with branches
  agreeing 55/55. What PROVED it is source rather than a codegen quirk:
  retail's own `loadBlackBox` reaches the same resize with NO zero test, and
  our plain `resize(count)` reproduces that shape exactly. When two retail
  functions differ, the difference is in their sources.
- **...AND SOMETIMES IT IS NOT A SWITCH AT ALL.** If retail's compares appear
  in an order that is NOT ascending by case value, the source wrote an
  IF-CHAIN, not a `switch` — VC6 sorts a switch's tests ascending and will
  relocate whole arms to do it. advManager::ProcessMapSelect went **77.76 ->
  90.90** on that change alone: retail compares HERO(34), TOWN(98),
  SHIPYARD(87) in that order, which no `switch` can produce. Read the compare
  ORDER before assuming the construct.
- **SCOPED LOCALS SHRINK THE FRAME** (SetHeroContext 90.56 -> 99.27). Retail
  writing three values through ONE stack slot does not mean one variable — it
  can be three separate BLOCK-SCOPED locals whose lifetimes do not overlap.
  A single function-scope local keeps the slot live across an inlined region,
  so that region's own values need fresh slots and the spill cascades. When
  the frame is bigger than retail's, look for values whose scope should end
  early. **The inverse is also real, so measure both directions**: REMOVING
  the brace scope around a shared block's locals took ProcessKeyPress 71.36
  -> 75.65 through register allocation alone, with block layout unchanged.
- **SWITCH ARM ORDER IS SOURCE ORDER — BUT ONLY FOR A JUMP-TABLE SWITCH.**
  Byte-proven 2026-08-20 on advManager::ProcessSelect (84.02 -> 100.0 on the
  edit alone): its four arms were each already byte-correct and it still
  plateaued, because retail lays them out hero/town/map/radar, the reverse of
  widget-id order. Reordering the cases and changing nothing inside them
  closed it. **QUALIFIED the same day, with a control**: for a SPARSE switch
  that compiles to a compare chain rather than a jump table, source order is
  NOT a lever — a control over three exact compare-chain switches
  (`army::AttackWall`, `bitmapBorder::Main`, `kbwin::AppCommand`) shows arm
  bodies emerge in REVERSE of the ascending dispatch order regardless of what
  the source wrote. So: check first whether retail dispatches through a jump
  table (reorder is a lever) or a compare chain (it is not, and the layout
  tells you nothing about source order).
- **A SEPARATE ZERO TEST BEFORE THE SWITCH.** advManager::Main went **64.64
  -> 93.23** and its frame 0x18 -> retail's 0x14 on one restructure: written
  as a single switch over {0, 1, 4, 0x200} VC6 pivots the list, whereas
  retail is `if (msg.id != MESSAGE_NONE) { switch } else { idle }` — a plain
  ascending three-case chain behind an explicit zero test. When a dispatch
  includes a "none" value, try lifting it out of the switch.
- **ZERO-INIT MUST ENUMERATE EVERY MEMBER** (97.42 -> 100.0). For a struct
  local, `= {0}` compiles to one store plus `rep stosd`, and `memset` to a
  bare `rep stosd`. Only the fully written-out `= {0,0,0,0,0,0,0,0}` gives
  retail's individual per-member stores — and only that form keeps the zero
  in a REGISTER, which is what lets it unify with a later `push` feeding
  other call sites. Count retail's stores and match the initializer's arity.
- **A pointer relational compare is UNSIGNED and can never produce `jl`**
  (byte-proven 2026-08-20, closed three hero.obj functions). VC6
  strength-reduces a *signed* `for (int i = 0; i < N; ++i)` walk into pointer
  form while KEEPING the original compare's signedness, so retail ending a
  loop on `cmp ebx,<End> / jl` was written as an INT INDEX, not a pointer
  walk — a `p < end` spelling emits `jb` and cannot match. Change BOTH loops
  of a nested pair in the same edit: converting only the inner one measured
  WORSE (equip_artifact 85.25 -> 75.08) because VC6 re-derived the outer
  bound as an unrelated offset while the outer walk stayed pointer-form.
- **The /Ob2 budget cuts BOTH ways - a local win can cost exact functions
  elsewhere.** Factoring duplicated arms into a shared helper made
  soundmgr's ConvertVolume score better IN ISOLATION but shrank it under
  VC6's inline budget, so it got inlined into MemorySample and ModifySample
  where retail CALLS it - knocking ModifySample off exact (100% -> 57.3%).
  Writing both arms longhand, as retail does, pushed it back over budget and
  restored both callers. `--fast` does not surface this; only the per-function
  scores of the CALLERS do. After any size-changing edit to a callee, check
  its callers' scores, not just its own. The prettier spelling is often wrong.
- **OVER-INLINE: pin the callee retail CALLS. The single highest-yield
  repeated lever found so far** (three functions, two lanes, 2026-08-20).
  When `predict-inline` reports `<callee> base x0 vs retail x1` — we expand
  it, retail calls it — put `#pragma auto_inline(off)` around that callee (or
  a statement-scoped `#pragma inline_depth(0)` at the call). Measured:
  `auto_inline(off)` on three advManager members our CL inlined
  (UpdateScreen, CompleteDraw(uchar), CheckDimNextHeroBut) paid **+7.90 on
  DoAdvCommand and +13.24 on ShowRoute**; `inline_depth(0)` on a 191-byte
  callee took cmbtmgr's place_obstacle **49.09 -> 67.19**. In all three the
  standing residual had blamed registers, and the register story was
  downstream — ROUTE THE INLINER FIRST on any low-scoring function.
  Definition order is NOT the lever: VC6 inlines functions defined LATER in
  the TU, so moving a definition does not control expansion (measured, and it
  refutes a note previously banked in advmgr.cpp).
  **The same wall has a second fix, and on big bodies it is the better one:
  SHRINK THE CALLER.** Our pre-inline caller being larger than retail's is
  what pushes the budget past retail's decisions, so splitting the caller into
  helpers restores them without pinning anything. The hero lane closed four
  plateaus this way in one round — mark_spells 84.29 -> 92.71,
  HeroFn_004E6120 93.41 -> 99.97, WindowHandler 56.54 -> 71.10,
  HeroFn_004D8B30 49.03 -> 55.74 — and for WindowHandler the tree's own
  carcass already named the three helpers to split out
  (`handle_artifact_click`, `handle_backpack_click`, `show_skills`). Check
  the carcass for helper names before inventing your own.
  **BUT THE PIN CAN RANK BACKWARDS, and the mechanism is worth knowing:
  `auto_inline(off)` / `inline_depth(0)` de-inlines EVERYTHING in the
  statements it covers, not just the callee you diagnosed.** Measured on
  cmbtmgr's SetupAndLoadObstacles: `PlaceObstacle base x0 vs retail x1` is
  the identical diagnosis that paid +18.10 on place_obstacle, and the
  identical pragma LOST 5.89 there (64.81 -> 58.92) because it also
  de-inlined the `obstacles.size()` in the same statement; hoisting `size()`
  out first recovers part and is still worse (61.59). **THE MECHANISM IS NOW
  KNOWN AND IT IS A FEATURE: `#pragma inline_depth(0)` IS STATEMENT-GRANULAR
  IN VC6, not function-granular** (game lane, 2026-08-20). So you can place
  it per call site, and that is how the two biggest serializers in the tree
  were broken open: retail keeps every `vector::resize` OUT of line while
  inlining insert/erase INTO it, and we did the exact inverse — pinning the
  eight `resize` sites took `game::Load` **50.46 -> 71.71**, and the same
  lever twice on `game::Save` (`bitset::test`, whose inlined `_Xran` throw
  path drags a `std::out_of_range` onto the frame, plus seven `save_vector`
  sites) took it **42.23 -> 73.80**. Pin the SITE, not the function.
- **/Ob2 single-call-site inlining**: statics AND extern functions with one
  call site inline REGARDLESS OF SIZE (AppInit→WinMain ~300B; AppCommand→
  AppWndProc is our uncracked over-inline residual). Unconditional out-of-line
  emission holds for EXTERN linkage ONLY — inlined single-call STATICS vanish
  (initialize.obj: two creators have no retail bodies). Same-TU helpers:
  write the call, let VC6 inline; inline-cost is calibrated by the callee's
  locals (initialize's cim precedent).
- **Signedness CSE**: -1 stores share one register (`or ecx,-1`, byte stores
  from cl) ONLY when the byte fields are signed/plain char; unsigned char
  splits a separately-materialized 0xff (hexcell ctor — the codegen itself is
  signedness evidence).
- **Bool materialization**: `unsigned char f = expr != 0;` → `setne`+byte;
  comparing normalized ints → `neg/sbb/neg`. Retail mixing both in one compare
  = one side was a byte local (AppWndProc fMinimized/fActive). **The RETURN
  case is the same rule and closes functions** (game's `save_vector` 96.60 ->
  100.0): `return a >= b;` emits `sbb`/`inc`, while assigning to an
  `unsigned char` local first and returning that emits retail's `setae al`.
  If retail sets a byte, the source landed the result in a byte.
- **Import call forms**: `call [__imp__X]` needs a dllimport declaration
  (windows.h family); `call _X@N` thunk form needs a plain declaration. The
  form is PER-TU (timeGetTime: kbwin IAT vs button/misc/mousemgr thunk) — a
  plain redeclaration downgrades dllimport and dllimport-after-plain loses;
  declare file-locally in the TUs that need the thunk form.
- **Ternaries** on small ints → `sete/sbb` idioms; `(dir+3)%6` signed → cdq/idiv.
- **Dead args / stray calls are real**: retail keeps discarded GetSpeed()
  calls, unused params, dead sprintf args — transcribe faithfully.
- **Chained assignment order**: `a = b = 0` stores b first (window rect
  left/top ordering).
- **Statement order from homm2**: when a buka twin exists
  (evidence/homm2-overlap/), adopt its statement ORDER as the starting shape —
  proven closer on the whole basewin family. Preserve its provenance in the
  §5 record.

## Known residual classes (document, don't grind past 3-4 real hypotheses)

- **Merged-return blocks / stale-CL-generation** (path.obj, kbwin AppWndProc):
  retail's tail-merge behavior differs from our SP3 CL in both directions;
  suspected earlier-generation objects in the retail link. OPEN — needs a
  compiler-generation probe. Keep the closest spelling.
- **Register-homing family** (smackmgr VideoPlay/DrawRects, widget
  send_message/enable): retail memory-homes a value our CL promotes (or vice
  versa). Order sweeps plateau; document.
- **EH-bearing functions** — **NOT a wall; this entry was wrong** (corrected
  2026-08-20). It used to read "fs:[0] frames can't close until the synth-PDB
  EH scope lands (P2.2); claim + `// EH-bearing`, body only if cheap", and
  lanes were skipping bodies on that basis. Counter-evidence, byte-proven:
  `THeroScreenWindow::THeroScreenWindow` (0x4de710, 11,346 B) is EH-bearing
  AND exact, and the whole EH-bearing mapcell reader family
  (readArtifactData, readGarrisonData, TTimedEvent::Read, readTimedEventList,
  loadTimedEventList) is exact or near it. Treat an fs:[0] frame as ordinary
  work. Local string/temporary cleanup DOES duplicate per early return in
  retail while our CL sometimes shares one epilogue — that is the
  merged-return class above, not an EH limitation.
- **Include-set sensitivity** (initialize_game_data precedent, byte-proven
  2026-08-08): a function's exactness can depend on the COUNT OF
  USER-DEFINED TYPE DEFINITIONS visible in its TU, with no semantic change
  anywhere. One unused `struct probe_t { int a; };` in a header took
  initialize_game_data 100.0 -> 96.09; a 0..8-struct sweep gave
  100.0/96.09/96.09/26.18/97.04/94.07/100.0/100.0/96.09 - NON-MONOTONIC, so
  it is VC6 optimizer state (type/symbol-table population perturbing CSE and
  addressing-mode choice), not a modelling error. The delta is real codegen:
  retail addressed a row directly (`mov [8*eax + tbl+0x160], ebx`) where our
  CL hoisted the base into a register. Blank lines, comments and typedefs do
  NOT move it. **CORRECTED 2026-08-20 — the trigger is the DECLARATOR COUNT
  C1XX numbers member handles from, and `extern int` DOES move it.** The
  cmbtmgr lane bisected `command.obj`'s GetCommand one edit at a time
  (92.5714 -> 92.5357, three times, restored by gating each): retyping a pad
  IN PLACE is free — three such renames landed un-gated at no cost — while
  adding a declarator, an enumerator, *or a single file-scope `extern int`*
  each costs 0.036. So the old "a 200-extern probe cleared the hypothesis"
  reasoning does not generalize past the header it was run on; count
  declarators, not type definitions, and gate anything you add. NO local
  spelling change can fix this; the match returns when the TU's include
  closure matches retail's (breaking one include edge restored it). When a
  header change moves an unrelated TU's exact function, suspect this before
  hunting a spelling, and re-measure after any include-graph edit.
- **STLport surface** — CLOSED 2026-08-07: retail links Dinkumware (VC6's
  shipped STL), NOT STLport. `#include <bitset>`/`<vector>` from the pinned
  toolchain is all that is needed where real call sites exist. Bodies parked
  on the old premise are reachable; re-read any "STLport-blocked" note.

## TU closure (functions-only, per the standing data-scope decision)

A TU may be declared CLOSED in §5 (zlib/hexcell precedent) only with: every
carved target fn in the freshly-recomputed span claimed AND exact; flanking
gaps attributed (cinit/excluded classes identified, neighbors' rosters
corroborate); the DC roster exhausted — each row located, proven inlined-away
(/Ob2 emission rules), proven retail-dropped (no slot fits), or proven
DC-port-only. Absent methods are documented, never forced. The claims-only
scoreboard is NOT closure evidence (the border illusion).

## Running in a worktree lane

`HOMM3_DIR` is baked into the nix devshell pointing at the MAIN repo, and the
`homm3` CLI resolves every path from it - **cwd is irrelevant**. In a worktree
lane, prefix every invocation with `export HOMM3_DIR=<your worktree>` or you
will silently build, delink and ratchet the main repo (and race whoever owns
it). Re-establish your baseline after fixing it; numbers measured before are
void. Pipeline order is **build -> delink -> build**: delinking against stale
objects makes the target side fall back to flat carve names, after which the
ratchet reports every promoted function MISSING and invents 0.0000 flat rows.

## House rules

- The wine VC6 build is the only verdict; clang diagnostics are editor noise.
- One reviewable unit ≈ one commit; never commit unasked.
- Claim-source-only files (not in units.toml) hold free-standing @stubs; in
  COMPILED units stubs must sit inside `#if 0 // @carcass`.
- Evidence tags on claims: anchor-global / anchor-bracket / anchor-callee /
  anchor-import / anchor-vtable / linkorder / dc-bracket forced /
  corroborates — say which, plus `dc 0x<off>` (or `retail-only`).
- Name lineage is explicit: DC > homm2(buka) > NH3API/IDA, provisional names
  marked; NH3API addresses are NEVER location evidence (wrong address space).
- Pipeline extensions (labels joins, new claim kinds) are contract changes:
  smallest possible diff + §5 entry in the same change.
