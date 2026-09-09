# Goto reconstruction audit

The current inventory is **67 goto statements in 37 functions across 21 files**.
That is **249 fewer than the initial 316 (78.8%)**, across 101 functions;
86 functions and 34 files no longer contain gotos. The latest source-scope and
helper pass removes **31 statements from the 98-statement checkpoint**, improves
four current scores and lowers none among all 4,752 scored functions. Its
border handler is newly exact, and the spell-immunity audit corrects several
retail switch-routing mistakes.

The isolated audit removed **198 statements (62.7%)**, across 83 functions,
without lowering any current score. That checkpoint contained 118 statements
in 53 functions across 29 files. Its measurements below describe that source
context; the integration and subsequent reductions are recorded separately.

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

The initial inventory contained 275 forward and 41 backward jumps. The isolated result
contains 106 forward and 12 backward jumps. Eighteen of those statements
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
`build/goto-more/`, `build/goto-residuals/`, `build/goto-integration/`,
`build/goto-after-merge/`, `build/goto-next/` and `build/source-families/`. These are scratch artifacts; source-specific
conclusions live beside the affected functions. The isolated audit added no new inline keywords,
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

## Isolated full checkpoint

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

## Integration with the helper cleanup

The combined checkpoint includes destination `e4650642` and audit `dd2b4c97`.
The C++ sources merge without conflicts. The generated status files are
regenerated from the combined sources; both branches' historical peaks remain
available through the build's Git-history recovery.

Relative to the destination, all 4,752 row identities remain: **18 current
scores improve and none decrease**, with exact functions rising from 4,057
to 4,061. The full build passes, at 96.38% fuzzy and 96.12% executable matched.
All 118 remaining gotos and the destination's 236 remaining inlining pins are
preserved. No new helper forks or inlining controls resolve the merge.

The destination's canonical map accessor, string I/O members, saved-header
reset and popup ownership changes are retained. Against the isolated audit,
13 current scores rise and 16 fall; those are separate from the destination
comparison. The movement includes `doAdvCommand` at 80.8250% (destination
77.1728%, audit 84.9679%), `game::load` at 78.2645% (destination 64.2804%, audit
76.9004%) and `army::doAttack` at 99.9040% (destination 98.8868%, audit 99.9424%).
The earlier isolated percentages describe their own compiler context.

One integration-specific loss was recovered: `CEnterNameEdit::onKeyPress`
initially scored 99.8868%. A new 12-state family (six distinct objects)
reproduces 100% with the canonical `getText()` call passed directly to
`onNameChange`. The separate text local that was exact in the isolated tree
now changes two stack-slot operands. Every sibling keeps its score; retail
CFG, instructions and the eight named calls agree in the adopted result.

## Further structured reductions

Starting from merged checkpoint `2f737f67`, twelve new finite source families
exhaust **217 successfully scored states and 146 distinct objects**. Each
family checks the owning TU's complete score vector and reproduces its
retained objects. Together with the isolated audit and integration family,
the search comprises 116 families, 1,028 scored states and 869 distinct objects
summed within their separate contexts. These counts include parent controls.

The follow-up removes **20 more gotos in eleven functions**, from 118 to 98.
Seven additional functions and two additional files are cleared. The remaining
inventory has 86 forward jumps, twelve backward jumps and eleven jumps whose
label syntactically begins a terminal return.

| Function or group | Removed | Retained source form |
| --- | ---: | --- |
| Army-group luck and terrain-description helpers | 6 | Default `break`, per-arm adjustments and returns; preserve out-of-range town behavior |
| `combatManager::isWinner` | 3 | Canonical `army::is` calls, first-scan result, guarded opponent scan |
| `combatManager::hexIsBlocked` | 2 | Ordinary `doorCanBeLowered`, const `hexcell::hasArmy`, positive bridge guard and outer `else` |
| `combatManager::automateCatapult` | 1 | Selected-target result before fallback selection |
| `advManager::doTreasureDialog` | 2 | Choice result with the gold arm first and the separate explicit-gold return |
| `oldmain` | 2 | Scored-campaign result and picker cancellation `break` |
| `NewfullMap::load` | 2 | One breakable failure scope around seer and event loading |
| `ResourceManager::getBitmap816` | 1 | Positive allocation result before cache insertion and return |
| `fillTreasureGroup` | 1 | Clear the failed selection after the final deletion, then use its existing loop exit |
| **Total** | **20** | **11 functions** |

The missing drawbridge helper illustrates why helper recovery precedes exit
rewrites. DC line 4711 calls `DoorCanBeLowered`; its const-this record and
lines 4675/4680/4686 prove the current-side check and two `HasArmy` calls.
The latter's own const-this record corrects the existing accessor declaration.
Restoring these canonical boundaries is neutral, and the structured caller
reproduces all 128 retail bytes without a new inline declaration or pin.
Likewise, `isWinner` restores five original `army::is` calls and the first
scan's failure result. Its fully structured source reproduces all 188 bytes.

Partial switch recovery also matters. The neutral terrain cases still need a
separate compiler destination in this context, but that does not require every
other arm to retain a goto. Moving the actual bonus into its arm and allowing
default to break removes three luck jumps without changing either caller.
The morale helper admits three removals and a small improvement. Keeping one
additional good-town join reaches 93.9282%; this independently reproduced
alternative is preserved in MAX/HIST, while the greater reduction has CUR
93.1409%. The caller's own source hash is unchanged. Its final CFG still differs
from retail; neither candidate is a claim of function closure.

The earlier direct-return failures in map loading did not prove the failure
join needed a goto. A `do { ... } while (0)` scope preserves the short read,
seer lifetime, event calls and one failure return. Both this form and an
exhaustive `for (;;)` scope retain 56.7217%; either individual direct return
falls to 53.5994%. In the bitmap loader, the positive allocation guard is
neutral while moving both cache insertions into one shared action loses the
retained pair constructor.

All 4,752 score identities and historical peaks survive the full checkpoint.
Relative to `2f737f67`, 4,748 current scores are unchanged and four improve:

| Function | Before | Current |
| --- | ---: | ---: |
| `army::doAttack` | 99.9040% | **99.9424%** |
| `armyGroup::getMoraleDescription` | 93.0566% | **93.1409%** |
| `advManager::doTreasureDialog` | 83.0357% | **94.4643%** |
| `oldmain` | 77.4155% | **78.4499%** |

The attack improvement is shared-header compiler collateral from the restored
const/accessor declarations. Exact functions remain 4,061; the full build
passes at 96.38% fuzzy and 96.12% executable matched. The 236 inlining pins
are unchanged. Existing historical gaps remain explicit: editing `oldmain`
resets MAX from 78.9421% to 78.4499%, and editing `getBitmap816` resets MAX
from 97.6325% to its unchanged CUR 94.5542%; HIST preserves both earlier peaks.
Those resets are not current-score losses caused by these rewrites.

Final retail review proves exact instructions, CFG and call streams for
`isWinner` and `hexIsBlocked`. Catapult selection retains its 59-block CFG,
four named calls and six scratch-register instruction differences. Treasure
choice now retains the seven-block CFG and all nine named calls; register
homes remain different. Map loading, bitmap loading and `oldmain` retain
existing inlining and frame residuals and are not described as closed. The
RMG fill's only named-call mismatch is its existing pointer-vector insertion
alias (`type_object*` versus the retail `widget*` label); its source type is
kept and all eight call positions remain. The insertion body's 521 bytes
match retail 0x54d120 after masking its two call operands; both operator
new/delete targets also agree, confirming the folded template alias.

Remaining limits are specific, not a proof of a minimum goto count. All eight
town-popup result/lifetime alternatives reproduce the same 87.0077% result,
below 90.2738%. Witch-hut refusal flags and text selection lower its match.
The explicit campaign replay-loop variants fall to 59.9903--60.0568%, so the
two backward restarts remain. RMG entrance policies and individual fit returns
also remain lower. These rejected forms are recorded beside their functions;
none justifies hiding the remaining joins in invented helpers or macros.


## Source scopes and spell routing

Starting from `2359d5a4`, twenty finite families exhaust **221 successfully
scored source states and 125 distinct objects**, counted once per family
context. The multiplayer family covers all 72 combinations over two batches;
its 24 objects are distinct across both batches. Together with the preceding
audit, the recorded search covers 136 families, 1,249 states and 994 objects.
These counts include reproduced parent controls, not independent fixes.

The pass removes 31 jumps in twelve functions. Nine functions and six files
are cleared; the remaining 67 statements comprise 61 forward and six backward
jumps, with three labels syntactically beginning a terminal return.

| Function or group | Removed | Retained source form |
| --- | ---: | --- |
| `getSpellWorkChance` | 7 | Separate canonical artifact calls, corrected switch routing and shared hero resistance; header `isMindSpell` |
| `border::main` | 3 | Negative inactive-widget arm with dispatch in `else`, positive mouse-hit and selected scopes |
| `iconWidget::main` | 5 | Positive mouse-hit and selected scopes, direct zero exits and ordinary `setPalette` |
| `searchArray::findCombatPath` | 3 | Direction index propagates the successful scan through ordinary breaks |
| `downgradedCreatureType` | 3 | One breakable lookup failure scope |
| `combatManager::castSpell` | 2 | Guarded redraw loops and separate exhaustion checks for Quicksand/Land Mine |
| Multiplayer widget handler | 2 | Connection-failure result with original cleanup order |
| System-options handler | 2 | Preference-change result and positive accepted-command scope |
| Main-menu handler | 1 | Confirmation result preserving the existing update assignments |
| `TTradeResourceWindow::update` | 1 | Ordinary `computeTradeRatios` and its three recovered output locals |
| `checkSetMouseDirection` | 1 | One failure scope containing the separate quick/computer guards |
| `validateVictoryLossConditions` | 1 | Direct final return for a valid loss-condition town |
| **Total** | **31** | **12 functions** |

The widget results refute the earlier claimed return-merging limit. In
`border::main`, merely replacing each goto with a return had scored 91.7822%.
The positive selected scope first restores 100%; the negative inactive-widget
scope then removes the last goto without changing any of the 461 retail bytes.
Reversing the outer polarity instead gives 76.7079%. The icon handler reaches
99.9639% with all five jumps removed and its original palette-helper call.
Its 756-byte body differs only in the EDX-versus-ECX operand used to copy the
widget id at `+0x113` and `+0x117`; all relocation references agree. It is not
reported as byte-exact.

The trading-post boundary also resolves a misleading earlier local analysis.
Dreamcast line 1208 calls `ComputeTradeRatios`, and lines 1210..1213 read its
ratio and denomination outputs. Only the maximum output is unused in this
caller. Restoring all three output locals and the complete ordinary helper
removes the formatting join at unchanged 88.5391%; all five header consumers
were measured. Keeping only the copied arithmetic had hidden that boundary.

The spell-work result required correcting behavior before interpreting scores.
Retail's compressed spell tables at `+0x418/+0x45c` and creature tables at
`+0x494/+0x4b8` establish each destination. Resurrection rejects undead without
the damage-high test; Bless falls into the shared Fortune/Misfortune/Slayer
damage test; Precision shares Forgetfulness's shooter test. Stone uses the
Troglodyte pair, while Poison uses the living trait and Gargoyle pair. The
arrow-tower rejection is independent of the siege trait. Green/Red/Azure
Dragons reject levels through three, Gold through four, and Black Dragons and
Magic Elementals reject all levels. Diamond Golems take the normal path.

Mind immunity is the creature trait **or** the hero's Badge of Courage, and
hero resistance runs after every non-rejecting creature arm, including default.
The old body had inverted the trait and limited resistance to Dwarves. These
facts are established by retail control flow and corroborated by DC 505..507
and 564..565. Separate original artifact calls plus corrected scopes raise
88.5071% to 95.8839%; the header-defined `IsMindSpell` call raises that to
96.6018%. Its unsigned-byte signature and header boundary come from DC
`SpellDefs.h:345..346`; the recovered header definition uses `inline`, while
the palette and trade-ratio helpers remain ordinary functions. All 95
spell-header consumers were checked; no new inlining pin is introduced.

The native spell fixture imports the actual function, accessor and enum
values. It covers all 150 creature ids with a resistance-bearing hero, all 81
spell ids, pendant checks, undead/living/shooter restrictions, dragon levels,
Orb precedence, clamping and beneficial spells. Three deliberately wrong
variants must fail: inverted mind immunity, Dwarf-only resistance and the
Black-Dragon/Diamond-Golem substitution. This checks recovered behavior;
VC6 under Wine remains the matching verdict.

Two parser issues had obscured the controls. VC6 `.bss` records an allocated
size without file payload; `creaturetype.obj` has a 19,204-byte `.bss` in a
7,563-byte object. The CodeView reader now skips only uninitialized-section
payload bounds, while truncated initialized data still fails. The family
identity reader now handles anonymous namespaces defined in `.h` as well as
`.cpp`, stripping only the path/nonce and preserving basename, type and full
signature. This fixes the `TAutoStrPtr` reproduction control without changing
byte scoring. The failed earlier controls are excluded. The focused parser,
source-label, family and spell tests pass (34 tests).

The full build preserves all 4,752 identities, historical peaks and
`CUR <= MAX <= HIST`. Relative to `2359d5a4`, 4,748 current scores are unchanged:

| Function | Before | Current |
| --- | ---: | ---: |
| `getSpellWorkChance` | 88.5071% | **96.6018%** |
| `border::main` | 94.9505% | **100%** |
| `searchArray::findCombatPath` | 90.2669% | **92.2920%** |
| `iconWidget::main` | 95.7040% | **99.9639%** |

Exact functions increase from 4,063 to 4,064. The full build passes at 96.40%
fuzzy and 96.14% executable matched. The baseline's 221 inlining pins remain
unchanged. The recovered helpers and source scopes do not imply closure of
the remaining non-exact callers.

Final contribution checks compare the actual extents and named relocations:
all 461 border bytes agree after relocation masking, and the icon differs only
at `+0x115` and `+0x118`, the register operands described above. The border's
linear assembly view also decodes embedded selector-table data as instructions;
its apparent two-row discrepancy is absent from the byte and CFG checks.
All three cursor calls and all 54 trading-post calls agree. The spell-work,
multiplayer and system-options call views flag only shifted self-references
to their switch tables; their actual helper call sequences agree.

Pathfinding's six differently named references are independently confirmed
folded template bodies: its pointer copy is the same 37-byte body as the
retail int copy, both empty vector destructors match the three-byte
`type_artifact` body, and both pointer insertions match the 521-byte
`widget*` body. Code and callee references agree in each comparison. The
main menu retains its existing extra string-cleanup delete call; the large
spell dispatcher retains earlier inlining/frame differences. Neither is
reported as closed.

The remaining limits are recorded at the affected functions. Mirror-image
result flags reach 99.9696%: only two initial ValidHex failure branches choose
the final return instead of retail's earlier return. Positive placement and
else scopes do not recover those destinations. Guarded large-obstacle picking
with positive placement reaches 97.7528%, below the exact join. Full town-loss
failure scopes fall to 87.4352..87.7111%, while its single final success return
is neutral. Multiplayer menu-result combinations reach at most 97.5668%, and
replacing the system-options outer widget-code switch falls to 85.1339%.
These measured alternatives do not establish a minimum possible goto count.
