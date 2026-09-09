# Goto reconstruction audit

The audit removes **75 of 316 goto statements (23.7%)**, across 38 functions,
without lowering any of the 4,752 scored functions. The remaining inventory is
**241 statements in 92 functions across 48 files**, down from 123 functions in
55 files. Thirty-one functions and seven files no longer contain gotos.

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
own function. It includes the three unannotated helpers that contain gotos;
it excludes vendor code and untracked build copies. This is a lexical inventory,
not a complete preprocessor. There are no counted gotos in `include/`.

The recorded before tree is commit
`231248aaac65c6a630d22f3bed3d26b1f5e8c133`; pass that to `--revision` to
reproduce the 316-statement inventory after these changes are committed.

The initial inventory contained 275 forward and 41 backward jumps. The result
contains 213 forward and 28 backward jumps. Forty-five remaining statements
syntactically target a terminal return; that alone does not make a direct return
byte-equivalent.

Dreamcast `show`, `asm --blocks`, `inline-clues` and retail `sema diff`
summary/structure/source outputs were collected for all 120 retail-addressed
functions in the initial inventory, plus the local-player selection helper.
The targeted rewrites use the recovered source calls, scopes and retail CFGs.
Missing inline-clue attribution was never treated as evidence against expansion.

The VC6 source-family search completed 39 controlled families, scoring 278
states and 223 distinct objects **summed within their separate contexts**.
Those totals include unchanged parent states; they are not 278 independent
fixes. Families used unchanged-source and opposite-corner reproduction controls,
scored every sibling in their selected TUs, and reproduced retained candidates.
The largest family explored 60 of 64 combinations of six independent victory
helper restorations. Other finite families were exhausted. One creature-type
return probe failed object-identity reproduction and was excluded from these
totals and from adoption.

Local manifests, candidate results and evidence outputs are in
`build/goto-audit/` and `build/source-families/`. They are scratch artifacts;
source-specific conclusions live beside the affected functions. No new inline
keywords, forced-inlining directives, inlining pins, dummy operations or volatile
surrogates were introduced. Existing helper declarations and inlining status
were retained; newly reconstructed helpers are ordinary members.

## Adopted forms

| Replacement | Removed | Functions and source evidence |
| --- | ---: | --- |
| Canonical helper calls | 21 | `ValidHex`/`FindPath` in path queries; `IsHumanTeam` in six victory checks; `GetLocalPlayerGamePos`; `CanJoin`/`Dismiss` in army merging; `get_attack_direction`; `LoadObeliskPool`/`SaveObeliskPool`; nullary `IsComputerAction`; `is_last_action`; `spells_not_required`; `IsIncapacitated` with the shooter rejection guard |
| Structured loops and `continue`/`break` | 26 | Two reverse corpse scans (12), obstacle placement (6), nested current-pass retries in `nextArmy` (5), Capitol/human-player scans (2), looping-sound trim (1) |
| Direct returns | 15 | Campaign handler consume exits (6), text-widget zero exits (6), DirectDraw blit exits (3) |
| Conditional scopes and switch-arm placement | 7 | Luck/morale calculation guards (4), bottom-view branch (1), world-shroud branch (1), Clover bonus in its switch arm (1) |
| Existing playback completion flags | 6 | Lost-game, Smacker and Bink loops (one each), campaign prologue playback (3) |
| **Total** | **75** | **38 functions** |

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

## Measured forms retained

These are limits of the tested source families, not proof that the remaining
gotos existed in the original C++. Helper/lifetime recovery may supersede them.

| Function | Structured probe | Kept score | Probe score |
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
  Recover an actual source helper before introducing a new abstraction.
- Six failure joins in `game::load`/`save`. Dreamcast names black-market, town
  and object-vector serializers; their container inlining and caller cleanup
  interactions need separate controlled families. Only the obelisk pair was
  reconstructed in this pass.
- Multi-level search exits in combat pathfinding, artifact placement, random
  map generation and spell placement. Preserve exhaustion behavior and object
  lifetimes when changing their loops. `mirrorImage` already has positive DC
  evidence for a jump out of its search, so its mere presence is not a defect.
- Shared return/selector blocks with the measured costs above. Search for
  missing source boundaries and lifetimes rather than assuming a compiler
  generation limitation from a flattened body.

## Full checkpoint

`homm3 build` rebuilds affected units, regenerates labels/retail targets and
runs the gates. The checkpoint includes collateral from both edited headers,
across all 149 linked units. All 4,752 baseline row identities are unchanged;
4,748 current scores are unchanged and four improve:

| Function | Before | After |
| --- | ---: | ---: |
| `army::validPath` | 95.7377% | **100%** |
| `type_AI_spellcaster::shouldAttackNow` | 86.6781% | **94.2671%** |
| `game::load` | 60.5547% | **63.4030%** |
| `game::save` | 76.9208% | **80.2448%** |

Linked exact functions increase from **4,059 to 4,060**. Linked fuzzy match
increases from **96.28% to 96.29%**; executable matched bytes increase from
**96.02% to 96.03%**. The larger historical serializer scores in older source
comments belong to earlier implementations, not this audit's before state.

The evidence establishes source simplification without score regressions.
Non-exact functions retain their documented residuals; this is not a claim
that all executable behavior has been validated through gameplay.
