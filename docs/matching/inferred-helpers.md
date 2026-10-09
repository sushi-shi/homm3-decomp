# Project-inferred helper audit (2026-10-09)

`config/source/win_only.tsv` carried 105 "project-inferred" rows: helpers
that the #121/#123 helper recovery introduced without a Dreamcast procedure,
mostly measured MAX-neutral when they landed. An invented helper is not free:
it adds an /Ob2 candidate site to every caller (see
[the inliner](../vc6/inliner.md)), and an out-of-line copy adds a call retail
may lack. Each helper was checked against:

- **Dreamcast** line tables at every caller (`homm3 dreamcast show`). The
  older SH build expands explicit inline helpers with rows from the helper's
  own file or line range (`cursor.obj` shows `hero.h`/`struct.h` rows, for
  example), so a helper body written on consecutive caller lines with no
  foreign or back-jump rows is a flat statement in that source.
- **Mac** retail call sites (`homm3 mac calls --json` over every pair). The
  CodeWarrior candidate keeps calls to ordinary `.cpp` helpers; where it
  calls the helper and Mac retail has no call at that site, Mac retail has
  the operation in place.
- **Loki** `h3maped` dynamic symbols and disassembly
  (`orig/editors/loki-1.0/h3maped.dynsym.tsv`).
- **Windows retail** call sets (`homm3.vc6.inline_model._function_calls`
  over the normalized base/target objects): no helper in the list kept a call
  retail lacks except through the callers' inlining decisions.

Classes: **proven** (native evidence names or retains the helper), **contradicted**
(native evidence writes the operation in place; folded back), **unproven**
(no native counterpart; folded when scores held, kept when the fold
dropped a function) and **dead** (no remaining caller; removed).

| class | count |
| --- | ---: |
| proven, kept | 5 |
| contradicted, folded or replaced | 74 |
| unproven, folded (scores held) | 4 |
| unproven, kept (fold drops a function) | 2 |
| dead, removed | 20 |

## Proven

| helper | evidence |
| --- | --- |
| `TObjectType::getSlotCategory` | Loki `0x8363ad4`, called from the slot-traits `contains` bodies |
| `TObjectType::getTriggerLoc` | Loki `0x8363b48`; editor `TGameObject::getTriggerLoc` forwards to it |
| `TObjectImageNameTable::getCount` / `getName` | Loki `TObjectType::getImageName` calls `TUniqueSet<std::string>::numItems` and `get` on `{anonymous}::getImageNameSet()`; replaced by `TUniqueSet<std::string>` (include/UniqueSet.h) |
| `TSeerReward::getRewardType` | Mac body `0x16a4c4`; Mac `TSeerHut::doSeerEvent` calls it on the reward member |

## Unproven

| helper | result |
| --- | --- |
| `SCampaign::isCurrentScenario` | kept: folding the `hero::initialize(const HeroExtra*)` site drops it 100 -> 75.31 (its depth-1 candidate site) |
| `game::getGameVersion` | kept: folding drops `type_sacrifice_window::backpackClick` 100 -> 89.03 (`updateAllSlots`'s cost stays at the `putDownArtifact` threshold only with the accessor site) |
| `TCampaignBrief::CampaignHeaderStruct::getScenario` | folded to `m_scenarios[i]`; held |
| `SCampaign::getScenarioInfo` (both) | folded to `m_mapScores[i]`; held |
| `mouseManager::waitUntilIdle` | folded to the pre-helper `while (m_busy);` spin; DC `robAppBlit` is a different body; held |

## Contradicted

| helper(s) | evidence of the in-place operation |
| --- | --- |
| `army::setAttackTarget`, `clearAttackTarget` | DC command.cpp:485, 617/618 store groupToAttack/indexToAttack directly |
| `army::setOwningSide` | DC spells.cpp:4712 stores the side directly |
| `baseManager::getStatus` | DC exec.cpp:150 reads `status`; DC retains `SetStatus` but has no getter |
| `game::getMineCount` | DC victorylossconditions.cpp:417 calls `vector<mine>::size` |
| `hero::getTargetX/Y/Z`, `clearTarget` | DC events.cpp:6120/6121 and cursor.cpp:983 (whose object shows `hero.h` inline rows elsewhere) |
| `TObjectType::getRecommendedTerrainCount` | Loki has `getRecommendedTerrainMask` (`0x8363ac0`, returns the member) and calls `bitset::count`/`operator[]` in the slot traits; replaced by that accessor |
| `CNetPlayerHandlerPlayer::resetTownAndHero`, `getPlayerPos`, `setPlayerPos`, `getHeroIndex`; `CNetPlayerHandler::beginPlayerCycle` | DC singleselectionwindow.cpp:1007..1010, 1031..1070, 1081, 1182..1184 |
| `TAdventureMapWindow::drawTownLocatorHighlight`, `showButtonImage` | Mac retail has no call at either caller (DC stubs) |
| `type_AI_combat_parameters::getRestorationPriorityValue`, `type_AI_spellcaster::shouldRestoreNow` | DC ai_tactical.cpp:2665/2666, 2674 |
| `type_spell_choice::initializeSelection` | DC ai_tactical.cpp:759..762 and 769..772 |
| `BinkManager` copy/decode/draw/close track helpers | Mac retail has the SDK calls in place (DC stubs) |
| `initializeDirectPlayName`, `CDPlay::releaseDirectPlay` | DC dxplay.cpp:297..300, 93..97, 1454..1457 |
| `hero::hasArenaVisit` | DC hero.cpp:6156, one statement; Mac flat |
| `hexcell::clearArmy`, `resetArmy`, `resetObstacle` | DC hexcell.cpp:27..34, one store per line; Mac flat |
| `message::setNoInput`, `inputManager::commitBufferedEvent`, `resetQueueIndices`, `readBufferedEvent` | DC inputmgr.cpp:831/833, 859, 864..908, 1154..1159 |
| `mouseManager::finishPointerWithoutRedraw`, `setLocalMouseRect` | DC mousemgr.cpp:479..483, 491..493, 805, 849 |
| `clearOverviewWidget` | DC overview.cpp:241..259 |
| palette `selectHSVChannels`, `adjustPaletteHue`, `adjustPaletteComponent`, `TPalette16::convert24to16WithMasks` | DC palette.cpp:166..176, 335..354, 387..395, 477..480 |
| `recruitUnit::initializeNonTownSource`, `initializeCreatureChoices`, `prepareInitialCost` | DC recruit.cpp:1121..1141, 1159..1179, 1189..1209 |
| `adjustLoadedResourceSaturation`, `readPaletteRecord`, `getResourceFileSize` | DC resourcemanager.cpp:998/999, 1094/1101, 1106/1107; Mac flat loaders |
| `type_sacrifice_window::updateUnselectedCreatureOffering`, `clearCurrentCreature`, `type_skeleton_window::clearCreatureSelection` | DC sacrifice_window.cpp:1445..1450, 2060/2061 |
| `soundManager` playback/driver gates and `endAllSamples` | Mac retail has the tests and sample loop in place (DC uses a different engine) |
| `includeBoltInUpdateBounds`, `combatManager::showResurrectionMessage` | DC spells.cpp:4025..4037, 4871..4877 |
| `canCheckCurrentPlayerVictory`, `VictoryConditionStruct::allowsCurrentPlayerVictory`, `recordWin`, `LossConditionStruct::recordLoss` | DC victorylossconditions.cpp:70/71, 90/91, 510/511 |
| `widget::initializeLinks`, `initializeHelpText`, `releaseHelpText`, `copyHelpText` | DC widget.cpp:45..55, 512..541 |
| `heroWindow::deleteWidgetObjects` | DC window.cpp:943..945 |
| `File::seekFrom` | DC winfile.cpp:213/214, 233/234, 258/259 |
| `clipScreenEffectRect`, `blendScreenPixel`, `darkenScreenPixelPair` | DC winmgr.cpp:1325..1345, 1814..1820; Mac flat |
| `message::setDialogEndCodes`, `heroWindowManager::finishDialog` | DC sacrifice_window.cpp:2294..2296 (the skeleton exit) |
| `heroWindowManager::invalidateHover` | DC winmgr.cpp:66..89, constructor stores |
| `CSprite::replacePalette` | DC CSprite.h:263/265, inside `SetPalette(TPalette16&)` |

Folding `recordLoss` moves `checkForDefeatedTownLoss` 100 -> 99.84: the
target point and the `getLocation()` temporary exchange stack homes. Named
locals, a returned `true`, the store order, `const`, a braced guard and an
explicit cast all keep 99.84; the store order costs more (97.97).

## Dead

`decodeMapBits`, `soundNode::reset`, both `TAutoStrPtr::copyText`,
`subtractResourceCost` and both `playerData::payResourceCost`, the three
`TSubWindow` collection helpers, `heroWindow::addOwnedWidget`, both
`townManager` hover helpers, `heroWindowManager::updateHover`, both
`resetForType`, both `message::setDialogEnd` and both `hero::setTarget`.

## Header declarations and the period-64 functions

Removing declarations shifts handle numbers in every TU that includes the
header ([handle period](../vc6/handle-period.md)). The four `message`
methods moved `initialize.cpp` out of retail's class (initializeGameData 100
-> 94.07), and the combined removals left `monstersSellOut` and
`transmitSaveGame` one class off. Two complete Dreamcast enums restore all
three without padding: `type_building_id` gains its 200 remaining
enumerators (generic and per-town aliases), and `e_looping_sound_id`
replaces its 70 ordinal placeholders with the 122 Dreamcast names
(`getSoundId` now returns the named sounds).

The CNetPlayerHandler accessors had made `onPlayerPosClick` exact by
raising `isFaceTaken`'s cost. Dreamcast's own `isFaceTaken` shape
(`continue` on the excluded seat, a nested face test, a function-scope
`pPlayer`) supplies that cost and keeps it exact without them.
