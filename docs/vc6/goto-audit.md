# Goto reconstruction audit

The audit removes **198 of 316 goto statements (62.7%)**, across 83 functions,
without lowering any current score among the 4,752 scored functions.
The remaining inventory is **118 statements in 53 functions across 29 files**,
down from 123 functions in 55 files. Seventy functions and twenty-six files
no longer contain gotos.

The isolated worktree follow-ups from `bcb7f5ed` remove **123 statements
(241 to 118)**. The final search removes **25 (143 to 118)** in fourteen
functions, including map stamping, elemental placement, artifact equipping,
recruitment and player selection. Each pass was measured against its own
full-build baseline; final match and collateral results are below.

These are source counts, not a claim that all 316 were originally structured
C++. Some jumps have positive Dreamcast source evidence; others model compiler
joins. Matching a jump instruction does not distinguish these origins.

## Scope and reproduction

Run the read-only inventory from the repository root:

```sh
python scripts/experiments/audit-gotos.py
python scripts/experiments/audit-gotos.py --revision HEAD
python scripts/experiments/audit-gotos.py --json /tmp/homm3-gotos.json
```

The script reads tracked C/C++ files under `src/` and `include/`, excludes
comments, literals and `#if 0` carcasses, and resolves each label within its
own function, including the three unannotated helpers in the initial inventory.
It excludes vendor code and untracked build copies. This is a lexical inventory,
not a complete preprocessor. There are no counted gotos in `include/`.

The recorded before tree is commit
`231248aaac65c6a630d22f3bed3d26b1f5e8c133`; pass that to `--revision` to
reproduce the 316-statement inventory after these changes are committed.

The initial inventory contained 275 forward and 41 backward jumps. The result
contains 106 forward and 12 backward jumps. Eighteen remaining statements
syntactically target a terminal return; that alone does not make a direct return
byte-equivalent.

Dreamcast `show`, `asm --blocks`, `inline-clues` and retail `sema diff`
summary/structure/source outputs were collected for all 120 retail-addressed
functions in the initial inventory, plus the local-player selection helper.
The targeted rewrites use the recovered source calls, scopes and retail CFGs.
Missing inline-clue attribution was never treated as evidence against expansion.

The VC6 source-family search completed **103 controlled families**, scoring
**799 states and 717 distinct objects**, summed within their separate contexts.
The final search and collateral recovery account for 30 families, 173 scored
states and 156 distinct objects. These totals include unchanged parents, not
independent fixes. Families use unchanged-source and opposite-corner
reproduction controls, score every sibling in their selected TUs, and reproduce
retained candidates.

The victory-helper and serializer families each explored 60 of 64 combinations.
The broad startup/playback/dialog family explored 60 of 324 combinations;
a subsequent 12-state family exhausted the startup/dialog recombination and
reproduced its adopted winner. Other finite families were exhausted. One initial
creature-type return probe failed object-identity reproduction and was excluded.
Two artifact-resistance states failed compilation because a surviving
jump skipped a declaration initializer; they are excluded from successful
counts. A legal declaration-scope correction compiled but lost 5.3464 points
and was rejected. Only the separately reproduced, neutral Dispel return was
adopted in that function.

Local manifests, candidate results and evidence outputs are in
`build/goto-audit/` (initial workspace), `build/goto-followup/`,
`build/goto-more/`, `build/goto-residuals/` and
`build/source-families/`. These are scratch artifacts; source-specific
conclusions live beside the affected functions. No new inline keywords,
forced-inlining directives, inlining pins, dummy operations or volatile
surrogates were introduced. Canonical helpers keep their declarations and
inlining status; newly reconstructed helpers are ordinary members or free
overloads. The unattested forced-inline player-selector copy is removed. The serializer restoration also removes five existing inline-depth
pins; none were added or moved into the restored helpers.

## Adopted forms

The first two passes removed 124 statements:

| Replacement | Removed | Functions and source evidence |
| --- | ---: | --- |
| Canonical helper calls | 32 | Path queries; six victory checks; local-player selection; army merging and attack direction; obelisk, black-market, town and generator serialization; computer-action policy; tactical last-action, spell-necessity and ranged-attack checks; shooter rejection; sacrifice-window artifact return |
| Structured loops and switch `continue`/`break` | 36 | Reverse corpse scans (12), obstacle placement (6), current-pass retries (5), Capitol/human scans (2), sound trim (1), slider dispatch exits (5), sell-creature rollover arms (5) |
| Direct returns | 17 | Campaign consume exits (6), text-widget zero exits (6), DirectDraw blit exits (3), binary-diff search exits (2) |
| Conditional scopes and switch-arm placement | 19 | Luck/morale guards (4), bottom/world views (2), Clover bonus (1), slider decrement/increment arms (2), artifact quantity wording (1), adventure options (3), game-type help (2), main-menu help (1), system-options outer dispatch (3) |
| Completion, exit and redraw flags | 20 | Playback loops (6), dimension-door/scuttle dialogs (5), combat-options dispatch (9) |
| **Total** | **124** | **52 functions** |

The dispatcher and playback pass adds these 49 removals:

| Functions | Removed | Adopted source form |
| --- | ---: | --- |
| `doAdvCommand` | 9 | Guarded movement loop, inner event-loop break and landing scope; canonical `hideRoute`, flight and water-walking helpers |
| `showCreatureSpellError` | 11 | Positive initial scope, direct target-rejection returns and spell-switch breaks |
| `updateGrid`, `drawWallAt` | 2 | Canonical computer/controlling-side helpers and ordinary wall `if`/`else` |
| `CDPlayLobby::connect` | 1 | Existing ordinary `getConnectionSettings(0, 0)` |
| Scholar and town events | 5 | DC-named `ScholarAwards` fallback state; direct end-game call/return |
| `getSpellWorkChance` | 1 | Direct Dispel success return |
| Mouse input and turn-duration chat | 2 | Position-handled flag and positive display scope |
| `nextBinkFrame` | 2 | Positive video/frame readiness guard |
| Windows message pump | 2 | Outer event loop with peek-dispatch `continue` and iconic pump |
| `earlySetup` | 4 | Two explicit head-tested loops |
| `eventWindowHandler` | 3 | Per-answer stores and forward returns |
| `displayVCWinLoss` | 5 | Campaign artifact `if`/`else` formatting and shared announcement tail |
| RMG treasure policy and terrain repair | 2 | Policy `if`/`else` and outer gap-loop `break` |
| **Total** | **49** | **17 functions** |

The final search adds these 25 removals:

| Functions | Removed | Adopted source form |
| --- | ---: | --- |
| `splitArmies` | 6 | Scoped calculation with early breaks and existing arrangement calls |
| `damageMessage`, combat-results handler | 2 | Wiped-out result and existing dialog exit flag |
| Dragon City event | 1 | Guard the AI appraisal before the shared creature-bank event |
| `LODFile::pointAt` | 2 | Restored ordinary `getDataPtr` helper |
| `summonElemental` | 4 | Nested picker loops with explicit exhaustion and chosen-hex propagation; canonical `getHexIndex` |
| `summonBoat` | 1 | Water-search result flag |
| Multiplayer widget handler | 2 | Original modem/direct failure cleanup and exit stores |
| Monster quick view | 1 | Detail-view result flag preserving construction scopes |
| `stampObject` | 1 | DC `mustCover`, references, coordinate/height locals and canonical map accessor |
| Artifact capacity and equipping | 3 | Component-fit result and head-tested slot search with separate explicit-slot validation |
| Recruitment handler | 1 | Shared unsigned-char abort result |
| First-player selection | 1 | Canonical ordinary `getLocalPlayerGamePos`; removed forced-inline copy |
| **Total** | **25** | **14 functions** |

The most useful distinction was between replacing a copied helper's exit and
restoring the helper itself. `getLocalPlayer` with a direct caller return scores
84.5588%; calling the existing ordinary `getLocalPlayerGamePos` preserves 100%.
VC6 expands that helper even though its definition follows the caller.

Likewise, `shouldAttackNow` had treated its first scan as caller source and
attributed the residual to register allocation. Dreamcast line 864 calls
`is_last_action`. Restoring that call raises 86.6781% to 94.2671% and removes
the scan's success label. The helper's existing inline status is unchanged.

`castSpell` calls the restored ordinary, const `spellsNotRequired` member.
Its early returns expand into the same shared assignment block that the copied
body had represented with `healing_only_done`; the caller remains exact.

`nextArmy` provides the complementary loop result. Five back edges resume the
same pass without advancing its counter. A nested `while (1)` with `continue`
reproduces all 674 retail bytes, as does a nested `for (;;)`. The former comment
claiming that a structured loop necessarily rotates was not supported by the
compiler controls and has been corrected.

The ranged-attack calculation is a second strong helper control. Its old note
claimed that two jumps into a shared quotient return were needed for exactness.
Dreamcast instead calls `getSimpleAttackEffect`, `IsIncapacitated` and
`cannot_attack`. All eight combinations of restoring those existing boundaries
preserve 100%, including the fully structured caller.

The six black-market, town and generator serializers remove all remaining
failure labels in `game::load` and `game::save`. Canonical `clear`, `resize`,
indexing and element I/O now live in the ordinary helpers evidenced by
Dreamcast. Their early returns preserve one caller cleanup boundary each;
there are no copied container bodies or new inlining controls. The combined
family improves Load from 63.4030% to 76.9004% and Save from 80.2448% to 96.5761%.

A switch may need **more ordinary source statements** to produce fewer artificial
joins. `TSellCreatureWindow::setRolloverText` gives each arm its original
`strcpy`/`sprintf` and ends with `break`; VC6 then merges the appropriate copies.
This removes five gotos and restores 100% from 90.2612%, while retaining
`getArmyName`. Similarly, Dreamcast's dialog exit flags preserve exactness in
both dimension-door handlers where duplicated direct exits lose 16–18 points.


`showCreatureSpellError` shows why coupled scopes matter. Replacing all eleven
jumps with false returns scored only 68.3889%. A positive initial scope, direct
returns from target rejection and ordinary switch breaks together reproduce
100%, up from 92.3889%. Its CFG, instructions and named 14-call stream agree.

The old `process1WindowsMessage` comment claimed that its top-tested peek
back edge proved source goto. An outer event loop with `continue` reproduces
all 170 bytes and its import decisions, including the exact expansions in
`GameTime::delay` and `delayTil`. A nested `while (PeekMessageA(...))` does rotate
and hoist the import, losing 16.7692 points in the pump and 19–23 in its callers.
This is evidence against that nested form, not against structured loops.

The Complete campaign artifact messages also admit ordinary `if`/`else` source.
Giving each special case its own `strcpy`, followed by the existing victory
announcement/dialog tail, removes five jumps and raises 94.9521% to 97.3784%.
A text-ID selector reaches 95.1482%; a pointer plus explicit found flag is neutral.
No nullable string pointer is used as an inferred success flag.


`LODFile::pointAt` is another exact helper recovery. DC line 431 calls
`getDataPtr`; reconstructing that ordinary member and its PC seek behavior
replaces two copied failure labels while preserving all 171 caller bytes.

`stampObject` requires the complete source model. The DC `mustCover` result,
object reference, coordinate and height locals, two object-type lookups and
canonical `NewfullMap::cell` call reproduce 100%, up from 90.4313%. The old
unattested release-VERIFY budget carrier is removed. The unused intersection
result and coordinates are explicit DC calculations, not invented padding.

`equipArtifact` reaches 100% from 93.2051% by keeping exhaustion's return
inside a head-tested loop and putting explicit-slot validation in an `else`.
Changing only the loop had missed that coupled scope. Its component-capacity
predicate keeps 92.9185% with a separate fit result and outer `continue`.

First-player setup calls `getLocalPlayerGamePos` at DC line 4495. Restoring
that ordinary helper removes an unattested forced-inline copy, its declaration
and its success goto. The helper's shared selected-result scope preserves the
exact retained body and both exact expanded callers. Using its earlier direct
returns instead loses 5.25 points in setup; a break-and-clamp form also lowers
`getLocalPlayer` and the retained helper. This is why helper source, retained
body and each expansion must be measured together.

## Measured limits and negative controls

These are limits of particular source families, not proof that the remaining
gotos existed in the original C++. Some negative controls are now superseded
by the successful helper or scope restorations above; they remain useful
evidence against replacing every goto mechanically.

| Function | Structured probe | Control score | Probe score |
| --- | --- | ---: | ---: |
| Adventure options handler | Direct consume returns | 100% | 87.4684% |
| `advManager::getSoundId` | Direct invalid returns | 96.9031% | 94.5074% |
| `getRangedAttackValue` | Direct quotient returns | 100% | 88.1132% |
| `border::main` | Direct zero returns | 94.9505% | 91.7822% |
| Combat options handler | Direct consume returns | 83.3598% | 81.7140% |
| `combatManager::isWinner` | Direct winning returns | 100% | 81.9178% |
| `combatManager::hexIsBlocked` | Direct zero returns | 100% | 75.6098% |
| `showCreatureSpellError` | Direct false returns | 92.3889% | 68.3889% |
| `iconWidget::main` | Direct zero returns | 95.7040% | 89.5668% |
| `NewfullMap::load` | Direct failure returns | 56.7217% | 53.5994% |
| `ResourceManager::getBitmap816` | Direct cached-result return | 94.5542% | 91.6626% |
| `hasSeparatedNeighbours` | Direct false returns | 100% | 99.7458% |
| `slider::main` | Direct base calls/returns | 100% | 89.6667% |
| System options handler | Direct consume returns | 94.3957% | 92.3189% |
| `CSaveGameEdit::ignoreKey` | Direct return, retaining `GetKeyState` | 100% | 97.5397% |
| `armyGroup::getArmyLuck` | Break from all terrain-switch arms | 100% | 78.6737% |
| `advManager::drawShroud` | Combined lookup guard and `else` | 100% | 47.7198% |
| `creditsWait` | Switch completion flag | 100% | 99.0393% |
| Intro playback | Combined event condition and break | 100% | 84.6629% |
| `congratsWait` | Combined event condition and break | 100% | 73.4633% |
| `placeLargeObstacle` | Break, then negative-ID guard | 100% | 83.0899% |
| `placeLargeObstacle` | Negative-ID guard inside retry loop | 100% | 93.7079% |
| `powEffect` | Inverted frame-count continue guard | 96.2927% | 96.2378% |
| Multiplayer widget handler | Duplicate exit-flag/return tail only | 97.4932% | 96.4714% |
| `getBuildingInfo` | Combined custom-building guard | 100% | 97.7551% |
| `townManager::main` | Eleven per-arm popup bodies | 90.2738% | 73.3704% |
| Resource-trade update | Decimal format in both conditional arms | 88.5391% | 86.8941% |
| Campaign handler | Shared exit flag | 84.4576% | 83.7571% |
| System options handler | Full exit/redraw-flag rewrite | 94.3957% | 85.1339% |
| Main menu | Set update flag before quit confirmation | 93.1746% | 92.2451% |
| `slider::main` | Direct handled-message return after break recovery | 100% | 99.9877% |
| `checkSetMouseDirection` | Direct rejection return | 97.6434% | 93.7273% |
| `doAdvCommand` | Guarded decrementing do/while | 83.3621% | 80.0299% |
| `earlySetup` | Conventional for scans or found flag | 95.3801% | 94.2515% |
| `getSpellWorkChance` | Legal artifact selector plus resistance returns | 88.5071% | 83.1607% |
| Victory/loss validation | Town-validation result flag | 90.0315% | 89.2426% |
| RMG zone constructor | Bounded selected-town break | 100% | 98.5294% |
| Adventure-map hover | Generic-help result flag | 100% | 94.4944% |
| `townManager::main` | Shared building-popup flag | 90.2738% | 87.0077% |
| Swap manager | Combined skill cases, right-first selection | 84.0502% | 83.4984% |
| Recruitment handler | Duplicated abort routing tail | 99.0924% | 95.0161% |

Several costs have a specific compiler explanation. Neutral/default terrain
arms collapse VC6's compressed selector table when written as plain breaks.
Direct invalid returns in `getSoundId` enable a branchless constant selection.
The terrain-ring predicate differs only in which duplicate return block receives
three branches. A tiny score loss can therefore represent a real branch-target
difference. None of these findings justifies converting every goto mechanically.

## Remaining work classes

The inventory is comprehensive; the source-family exploration is bounded, not
a proof of the maximum possible reduction. Remaining leads include:

- Shared dialog actions and dispatch tails, notably the eleven building-popup
  jumps in `townManager::main` and the multiplayer connection/error joins.
  The popup arms also branch to one common body in Dreamcast. The tested
  per-arm copies and full multiplayer cleanup combinations lose score. Two
  modem/direct failure exits now use their original stores; seven other joins
  remain. No new helper was invented to hide those joins.
- Multi-level search exits in combat pathfinding, random map generation and
  spell placement. Preserve exhaustion behavior and object lifetimes when
  changing their loops. `mirrorImage` already has positive DC evidence for a jump out of its search, so its mere presence is not a defect.
- Shared return/selector blocks with the measured costs above. Search for
  missing source boundaries and lifetimes rather than assuming a compiler
  generation limitation from a flattened body.

## Full checkpoint

`homm3 build` rebuilds affected units, regenerates labels/retail targets and
runs the gates. The checkpoint includes shared-header collateral across all
149 linked units. All 4,752 baseline row identities are unchanged; 4,732
current scores are unchanged and twenty improve from the original audit baseline:

| Function | Before | After |
| --- | ---: | ---: |
| `army::validPath` | 95.7377% | **100%** |
| `type_AI_spellcaster::shouldAttackNow` | 86.6781% | **94.2671%** |
| `game::load` | 60.5547% | **76.9004%** |
| `game::save` | 76.9208% | **96.5761%** |
| `army::doAttack` | 98.8868% | **99.9424%** |
| `recruitUnit::update` | 94.1574% | **99.9898%** |
| Combat options handler | 83.3598% | **85.9186%** |
| Sell-creature rollover | 90.2612% | **100%** |
| Artifact-purchase update | 88.6580% | **89.2899%** |
| `doAdvCommand` | 83.3621% | **84.9679%** |
| `showCreatureSpellError` | 92.3889% | **100%** |
| `earlySetup` | 95.3801% | **99.5994%** |
| `eventWindowHandler` | 91.7323% | **99.3701%** |
| `displayVCWinLoss` | 94.9521% | **97.3784%** |
| `splitArmies` | 97.7143% | **98.8238%** |
| `summonBoat` | 91.1018% | **91.5769%** |
| `summonElemental` | 93.4375% | **99.0144%** |
| Multiplayer widget handler | 97.4932% | **98.1199%** |
| `stampObject` | 90.4313% | **100%** |
| `hero::equipArtifact` | 93.2051% | **100%** |

The final search raises eight current scores and lowers none. Across the
isolated worktree follow-ups, eighteen improve. One existing historical gap
needs explicit separation from current-score changes:
`doAdvCommand` starts with CUR 83.3621% and MAX/HIST 94.3953%. Editing its own
source resets MAX; the final CUR/MAX is 84.9679%, while HIST retains 94.3953%.
This pass improves the current implementation but does not recover that older peak.

Removing the obsolete selector declaration temporarily lowered three shared-
header consumers. These were real VC6 code-generation shifts, not changed
retail targets or an accepted loss of the canonical helper:

- Recruitment's DC lines 538/542 contain a `sprintf` in each availability arm.
  Restoring both calls raises `update` from the collateral 94.1574% to 99.9898%.
- Initializing `doAttack`'s secondary target before the reset call, as retail
  does, and retaining an explicit final `else` raises 98.8868% to 99.9424%.
- Name-entry's DC line 1824 calls ordinary `onNameChange`. That missing member
  owns the player lookup, focus reset, preference stores and widget refresh.
  Capturing the text before loading the window receiver restores `onKeyPress`
  to 100% while retaining `hide`, `getText` and `onNameChange`. Passing the
  accessor directly remains at 99.8868%; all four header consumers were scored.

Linked exact functions increase from **4,059 to 4,064**. Linked fuzzy match
increases from **96.28% to 96.38%**; executable matched bytes increase from
**96.02% to 96.11%**. Relative to the final search's baseline, these changes
are 4,062 to 4,064 exact, 96.37% to 96.38% linked fuzzy, and 96.10% to 96.11%
executable matched. Older serializer scores quoted in source comments belong
to earlier implementations, not the recorded audit baseline.

The score report compares with `function_reloc_diffs=none`; exactness claims
here use that project's metric. Strict named-call review also checks the actual
helpers. `stampObject`'s six differently named calls resolve to folded POD
`_Construct`/`_Ucopy`/`_Ufill` bodies, verified byte-for-byte against retail
(9/47/38 bytes). `equipArtifact`'s unlabeled call is its raw x86 self-call.
These label differences do not justify changing the recovered source types.

The evidence establishes source simplification without current-score regressions.
Non-exact functions retain documented residuals; this is not a claim that all
executable behavior has been validated through gameplay.
