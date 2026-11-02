# PR197 Mac evidence review

Compared the unchanged baseline and the exact previously Windows-verified PR197 source/header candidate in `/tmp/homm3-cleanup-editors-20261102`. The main/integration tree was untouched. Both targeted commands completed successfully:

```sh
homm3 mac build questlogwindow advmgr hero swapmgr
```

No Mac ledger was banked, no commit was created, and both authored candidate files were restored byte-for-byte in a `finally` block. The worktree is checked separately for tracked changes.

## Coverage and preservation

| Unit | Baseline scored | Candidate scored | Baseline exact | Candidate exact | Baseline unavailable | Candidate unavailable |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| questlogwindow | 2 | 2 | 0 | 0 | 4 | 4 |
| advmgr | 58 | 58 | 32 | 32 | 56 | 56 |
| hero | 71 | 71 | 41 | 41 | 57 | 57 |
| swapmgr | 10 | 10 | 7 | 7 | 18 | 18 |

Total: 141 → 141 scored pairs, 80 → 80 exact; 0 availability losses, 0 newly available pairs, and 0 score changes. All previously scored pairs were compared; no score loss was found.

Both runs report zero selected Windows-VA claims without a full-TU body. The 135 emitted-but-unavailable pairs are missing resolved-reference evidence, not zero-score results. Unavailability reasons are unchanged between runs.

## doQuestLog limitation and direct emitted-code check

`doQuestLog` (Windows `0x0052e430`, Mac `0:0x14a280`) is unavailable in both retail Mac comparisons because of:

> unreviewed UDATA symbol `g_questLogWindow`

The emitted `.doQuestLog__Fi` hunk is 1048 bytes in both builds; its bytes and all symbolic cross-references are identical: **False**. The candidate changes this emitted function. Because both versions have the unresolved global binding, the linked retail score cannot establish whether this change improves or worsens the Mac comparison.

Across the four full-TU listings, **one emitted code hunk changed**: `doQuestLog`. Exactly two bytes differ, at function offsets `+0x2ff` and `+0x357`; all 29 symbolic cross-references are identical. The two changed instructions are the matching temporary store/load: baseline `stw r0,0xac(r1)` / `lwz r6,0xac(r1)` become candidate `stw r0,0xa4(r1)` / `lwz r6,0xa4(r1)`. All other emitted code hunks retain their bytes and cross-references. No new data/TOC/function-descriptor admissions or compiler-profile changes were made.

## Verdict

PR197 preserves all available Mac scores and availability. The target function itself remains unavailable for a linked retail score verdict and its emitted code changes. Therefore this run finds no regression among the 141 scored pairs but does **not** establish Mac preservation of doQuestLog. The controlled Windows 100% result remains verified; the target-specific Mac gap must accompany any integration decision.

## Evidence files

- `build/pr197-mac-baseline.log` and `build/pr197-mac-candidate.log`.
- `build/pr-review-197/mac/baseline.json` and `candidate.json`.
- `build/pr-review-197/mac/{baseline,candidate}-<unit>.{o,dis.txt,hunks.json}`.

## Unavailable pairs

| Unit | Windows VA | Reason (same in both runs) |
| --- | --- | --- |
| advmgr | 0x00405d20 | anonymous TOC literal '@1562' has 0 reviewed payload matches |
| advmgr | 0x00405d80 | anonymous TOC literal '@1583' has 0 reviewed payload matches |
| advmgr | 0x00405e30 | TOC symbol 'g_playerTurn' needs one nonrelocatable emitted data payload |
| advmgr | 0x00406df0 | TOC symbol '__vt__10advManager' needs one nonrelocatable emitted data payload |
| advmgr | 0x00406fd0 | external TOC symbol 'g_completeDrawEnabled' must have an IL reference and no emitted initializer |
| advmgr | 0x004077e0 | TOC symbol 'g_townViewActive' needs one nonrelocatable emitted data payload |
| advmgr | 0x00407b80 | unreviewed UDATA symbol 'g_lowMemory' |
| advmgr | 0x004087b0 | unreviewed UDATA symbol 'g_forceSwitchMusic' |
| advmgr | 0x00408c40 | TOC symbol 'g_normalDirTable' needs one nonrelocatable emitted data payload |
| advmgr | 0x00409a70 | unreviewed UDATA symbol 'g_lowMemory' |
| advmgr | 0x0040a0c0 | anonymous TOC literal '@3502' has 0 reviewed payload matches |
| advmgr | 0x0040a5d0 | TOC symbol 'g_curWatchPlayer' needs one nonrelocatable emitted data payload |
| advmgr | 0x0040abe0 | anonymous TOC literal '@3944' has 0 reviewed payload matches |
| advmgr | 0x0040b150 | TOC symbol 'g_ownedByColor' needs one nonrelocatable emitted data payload |
| advmgr | 0x0040d3f0 | TOC symbol 'g_constCreatureBankTraits' needs one nonrelocatable emitted data payload |
| advmgr | 0x0040d670 | TOC symbol 'g_ownedByColor' needs one nonrelocatable emitted data payload |
| advmgr | 0x0040d8d0 | unreviewed UDATA symbol 'g_globalInfoFlagNames' |
| advmgr | 0x0040db00 | TOC symbol 'g_constWiseTreePriceText' needs one nonrelocatable emitted data payload |
| advmgr | 0x0040dcc0 | unreviewed UDATA symbol 'g_globalInfoFlagNames' |
| advmgr | 0x0040deb0 | .processWaitingHover__10advManagerFii: unresolved relocation HUNK_XREF_24BIT '.isValid__10type_pointCFv' at 0xc8 |
| advmgr | 0x0040e360 | .processHover__10advManagerFii: unresolved relocation HUNK_XREF_24BIT '.isValid__10type_pointCFv' at 0xfc |
| advmgr | 0x0040ec90 | anonymous TOC literal '@5859' has 0 reviewed payload matches |
| advmgr | 0x0040f3f0 | unreviewed UDATA symbol 'g_completeDrawFpsText' |
| advmgr | 0x0040f8c0 | external TOC symbol 'g_mapVisibilityBit' must have an IL reference and no emitted initializer |
| advmgr | 0x0040fb30 | unreviewed UDATA symbol 'g_drawingPuzzle' |
| advmgr | 0x00410c00 | unreviewed UDATA symbol 'g_drawingPuzzle' |
| advmgr | 0x00411590 | unreviewed UDATA symbol 'g_drawingPuzzle' |
| advmgr | 0x00412220 | unreviewed UDATA symbol 'g_completeDrawAllCells' |
| advmgr | 0x00412bd0 | .getCell__10advManagerF10type_point: unresolved relocation HUNK_XREF_24BIT '.isValid__10type_pointCFv' at 0x1c |
| advmgr | 0x00412c40 | external TOC symbol 'g_goSolo' must have an IL reference and no emitted initializer |
| advmgr | 0x004137c0 | anonymous TOC literal '@9329' has 0 reviewed payload matches |
| advmgr | 0x00415de0 | TOC symbol '@9397' needs one nonrelocatable emitted data payload |
| advmgr | 0x004167a0 | anonymous TOC literal '@9804' has 0 reviewed payload matches |
| advmgr | 0x00416f80 | TOC symbol 'g_curWatchPlayer' needs one nonrelocatable emitted data payload |
| advmgr | 0x00417150 | anonymous TOC literal '@3507' has 0 reviewed payload matches |
| advmgr | 0x00417420 | anonymous TOC literal '@9924' has 0 reviewed payload matches |
| advmgr | 0x00417680 | unreviewed UDATA symbol 'g_inViewWorld' |
| advmgr | 0x00417b20 | external TOC symbol 'g_completeDrawMessageBypass' must have an IL reference and no emitted initializer |
| advmgr | 0x00418160 | TOC symbol 'g_saveGameName' needs one nonrelocatable emitted data payload |
| advmgr | 0x004183d0 | unreviewed named TOC data 'g_soundVolumes' |
| advmgr | 0x00418620 | TOC symbol '@10510' needs one nonrelocatable emitted data payload |
| advmgr | 0x00418c10 | unreviewed named TOC data 'g_soundVolumes' |
| advmgr | 0x00418dd0 | unreviewed named TOC data 'g_routeArrowFrames' |
| advmgr | 0x00419300 | external TOC symbol 'g_debugLevel' must have an IL reference and no emitted initializer |
| advmgr | 0x004195c0 | unreviewed UDATA symbol 'g_lastMapScrollTime' |
| advmgr | 0x00419820 | unreviewed UDATA symbol 'g_lastMapScrollTime' |
| advmgr | 0x00419c80 | TOC symbol 'g_hwndApp' needs one nonrelocatable emitted data payload |
| advmgr | 0x00419e00 | unreviewed UDATA symbol 'g_lastDebugState' |
| advmgr | 0x0041a100 | TOC symbol 'g_monthTypeExtra' needs one nonrelocatable emitted data payload |
| advmgr | 0x0041a1e0 | TOC symbol 'g_highMemBuffer' needs one nonrelocatable emitted data payload |
| advmgr | 0x0041a660 | unreviewed UDATA symbol 'g_thisNetGotAdventureControl' |
| advmgr | 0x0041a7f0 | anonymous TOC literal '@11239' has 0 reviewed payload matches |
| advmgr | 0x0041ac00 | unreviewed UDATA symbol 'g_gameCommand' |
| advmgr | 0x0041adb0 | TOC symbol '@11396' needs one nonrelocatable emitted data payload |
| advmgr | 0x0041b040 | TOC symbol '__vt__9CAdvPopup' needs one nonrelocatable emitted data payload |
| advmgr | 0x0041b120 | TOC symbol '__vt__9CAdvPopup' needs one nonrelocatable emitted data payload |
| hero | 0x004d71a0 | TOC symbol 'g_heroSpecificAbilitiesImp' has a truncated MWLink data listing; payload unavailable |
| hero | 0x004d7220 | external TOC symbol 'g_heroSpecificAbilities' must have an IL reference and no emitted initializer |
| hero | 0x004d7240 | unreviewed UDATA symbol 'g_ballisticsTraits' |
| hero | 0x004d7a20 | .load__4heroFP13TAbstractFilei: unresolved relocation HUNK_XREF_24BIT '.__ct__Q33std10bitset<48>9referenceFRQ23std10bitset<48>Ul' at 0x93c |
| hero | 0x004d80c0 | .save__4heroFP13TAbstractFile: unresolved relocation HUNK_XREF_24BIT '.writeValue<c>__FP13TAbstractFilec' at 0x40 |
| hero | 0x004d85f0 | TOC symbol '__defctor__13type_artifactFv' needs one nonrelocatable emitted data payload |
| hero | 0x004d8720 | site-bounded Mac literal lacks one anonymous TOC reference |
| hero | 0x004d8b30 | anonymous TOC literal '@2039' has 0 reviewed payload matches |
| hero | 0x004d8fb0 | TOC symbol 'g_heroBio' needs one nonrelocatable emitted data payload |
| hero | 0x004d9110 | TOC symbol '@2226' needs one nonrelocatable emitted data payload |
| hero | 0x004d9160 | TOC symbol '@2238' needs one nonrelocatable emitted data payload |
| hero | 0x004d97f0 | external TOC symbol 'g_heroScreenArmyStripLive' must have an IL reference and no emitted initializer |
| hero | 0x004d9990 | TOC symbol 'g_statDesc' needs one nonrelocatable emitted data payload |
| hero | 0x004d9b30 | anonymous TOC literal '@2649' has 0 reviewed payload matches |
| hero | 0x004d9cc0 | anonymous TOC literal '@2649' has 0 reviewed payload matches |
| hero | 0x004d9ec0 | TOC symbol 'g_combatSurrendered' needs one nonrelocatable emitted data payload |
| hero | 0x004da720 | TOC symbol 'g_dialogDeadline' needs one nonrelocatable emitted data payload |
| hero | 0x004dad00 | external TOC symbol 'g_campaignDisabledSkills' must have an IL reference and no emitted initializer |
| hero | 0x004daf90 | unreviewed UDATA symbol 'g_currentHero' |
| hero | 0x004db1f0 | unreviewed UDATA symbol 'g_currentHero' |
| hero | 0x004db660 | unreviewed UDATA symbol 'g_heroScreenDraggedArtifact' |
| hero | 0x004dbe80 | .hasCombinationArtifactComponents__4heroFi: unresolved relocation HUNK_XREF_24BIT '.set__Q23std11bitset<144>FUlb' at 0x90 |
| hero | 0x004dbf30 | .assembleCombinationArtifact__4heroFil: unresolved relocation HUNK_XREF_24BIT '.set__Q23std11bitset<144>FUlb' at 0x98 |
| hero | 0x004dc320 | .getMoraleDescription__4heroCFv: unresolved relocation HUNK_XREF_24BIT '.__vc__Q23std40__vector_imp<Pc,Q23std13allocator<Pc>,1>CFUl' at 0x58 |
| hero | 0x004dcac0 | site-bounded Mac literal lacks one anonymous TOC reference |
| hero | 0x004dd2d0 | unreviewed UDATA symbol 'g_heroScreenHeroPosition' |
| hero | 0x004de710 | anonymous TOC literal '@6482' has 0 reviewed payload matches |
| hero | 0x004e1550 | unreviewed UDATA symbol 'g_heroScreenDraggedArtifact' |
| hero | 0x004e1600 | unreviewed UDATA symbol 'g_heroScreenHeroPosition' |
| hero | 0x004e16d0 | unreviewed UDATA symbol 'g_currentHero' |
| hero | 0x004e1800 | unreviewed UDATA symbol 'g_heroScreenNoDismiss' |
| hero | 0x004e1a50 | unreviewed UDATA symbol 'g_currentHero' |
| hero | 0x004e2550 | TOC symbol 'g_artifactSlotMasks' needs one nonrelocatable emitted data payload |
| hero | 0x004e2840 | TOC symbol 'g_artifactSlotMasks' needs one nonrelocatable emitted data payload |
| hero | 0x004e36c0 | unreviewed named TOC data 'g_luckBonuses' |
| hero | 0x004e39b0 | unreviewed named TOC data 'g_leadershipBonuses' |
| hero | 0x004e3cd0 | unreviewed named TOC data 'g_necromancyFactors' |
| hero | 0x004e3f40 | unreviewed named TOC data 'g_mysticismBonuses' |
| hero | 0x004e4070 | unreviewed named TOC data 'g_scoutingVisibility' |
| hero | 0x004e4160 | unreviewed named TOC data 'g_archeryFactors' |
| hero | 0x004e42b0 | unreviewed named TOC data 'g_offenseFactors' |
| hero | 0x004e4310 | unreviewed named TOC data 'g_defenseFactors' |
| hero | 0x004e4390 | unreviewed named TOC data 'g_estatesGold' |
| hero | 0x004e4420 | unreviewed named TOC data 'g_eagleEyeFactors' |
| hero | 0x004e4580 | unreviewed named TOC data 'g_diplomacyFactors' |
| hero | 0x004e46e0 | unreviewed named TOC data 'g_magicResistanceFactors' |
| hero | 0x004e4840 | unreviewed named TOC data 'g_learningFactors' |
| hero | 0x004e48b0 | unreviewed named TOC data 'g_intelligenceFactors' |
| hero | 0x004e4920 | unreviewed named TOC data 'g_firstAidFactors' |
| hero | 0x004e4990 | external TOC symbol 'g_moveConstants' must have an IL reference and no emitted initializer |
| hero | 0x004e5400 | anonymous TOC literal '@8004' has 0 reviewed payload matches |
| hero | 0x004e5550 | .canSummonBoat__4heroCFv: unresolved relocation HUNK_XREF_24BIT '.__ct__10type_pointFsss' at 0x58 |
| hero | 0x004e5760 | anonymous TOC literal '@8123' has 0 reviewed payload matches |
| hero | 0x004e59a0 | .fly__4heroFi: unresolved relocation HUNK_XREF_24BIT '.__ct__10type_pointFsss' at 0x30 |
| hero | 0x004e5aa0 | external TOC symbol 'g_heroSpecificAbilities' must have an IL reference and no emitted initializer |
| hero | 0x004e5ff0 | external TOC symbol 'g_heroSpecificAbilities' must have an IL reference and no emitted initializer |
| hero | 0x004e6120 | external TOC symbol 'g_heroSpecificAbilities' must have an IL reference and no emitted initializer |
| questlogwindow | 0x0052d8c0 | anonymous TOC literal '@1448' has 0 reviewed payload matches |
| questlogwindow | 0x0052e170 | unreviewed UDATA symbol 'g_questLogWindow' |
| questlogwindow | 0x0052e1e0 | TOC symbol '__vt__15TQuestLogWindow' needs one nonrelocatable emitted data payload |
| questlogwindow | 0x0052e430 | unreviewed UDATA symbol 'g_questLogWindow' |
| swapmgr | 0x005aaa80 | anonymous TOC literal '@4595' has 0 reviewed payload matches |
| swapmgr | 0x005ae3c0 | TOC symbol '__vt__11TSwapWindow' needs one nonrelocatable emitted data payload |
| swapmgr | 0x005ae430 | unreviewed UDATA symbol 'g_swapManager' |
| swapmgr | 0x005ae500 | TOC symbol '__vt__11swapManager' needs one nonrelocatable emitted data payload |
| swapmgr | 0x005ae5b0 | unreviewed UDATA symbol 'g_splitArmyMode' |
| swapmgr | 0x005ae750 | TOC symbol 'g_heroScreenDraggedArtifact' needs one nonrelocatable emitted data payload |
| swapmgr | 0x005aeb00 | unreviewed UDATA symbol 'g_swapManager' |
| swapmgr | 0x005aed20 | TOC symbol 'g_heroScreenDraggedArtifact' needs one nonrelocatable emitted data payload |
| swapmgr | 0x005aedc0 | unreviewed UDATA symbol 'g_splitArmyMode' |
| swapmgr | 0x005aef00 | TOC symbol 'akArtifactSlotTraits' needs one nonrelocatable emitted data payload |
| swapmgr | 0x005af2b0 | unreviewed UDATA symbol 'g_splitArmyMode' |
| swapmgr | 0x005af590 | TOC symbol 'g_heroScreenDraggedArtifact' needs one nonrelocatable emitted data payload |
| swapmgr | 0x005af990 | TOC symbol 'g_heroScreenDraggedArtifact' needs one nonrelocatable emitted data payload |
| swapmgr | 0x005afbf0 | .sendHeroUpdate__11swapManagerFv: unresolved relocation HUNK_XREF_24BIT '.__ct__14CHeroUpdateMsgFP4heroP4hero' at 0x7c |
| swapmgr | 0x005afdf0 | TOC symbol 'g_chatMan' needs one nonrelocatable emitted data payload |
| swapmgr | 0x005b08b0 | anonymous TOC literal '@4579' has 0 reviewed payload matches |
| swapmgr | 0x005b0ef0 | anonymous TOC literal '@4797' has 0 reviewed payload matches |
| swapmgr | 0x005b10d0 | unreviewed UDATA symbol 'g_splitArmyMode' |
