# Mac CodeWarrior optimization profile

The retail PEF was not built with one optimization level. Each unit's level in
`config/mac/units.toml` is the one whose full-TU object reproduces the most
exact Mac pairs; that count is the retail-backed control. Where several
levels tie, the unit keeps the shared `-O4`, the level with the most exact
pairs over the whole executable. The other flags (`-proc 750 -nomapcr`) are
unchanged.

The sweep compiled every unit at `-O1`..`-O4` with the same source
(2026-10-09, after the 927f51c6f re-bank) and scored it with
`homm3 mac build --fast`:

| level for every unit | exact pairs |
| --- | ---: |
| `-O1` | 329 / 1,532 |
| `-O2` | 426 / 1,532 |
| `-O3` (former shared profile) | 607 / 1,534 |
| `-O4` | 659 / 1,534 |

Six units reproduce more retail bodies at a lower level. With those
overrides the profile scores 705 exact pairs. Two pairs that were exact at
`-O3` are not under their unit's best level: THeroScreenWindow::exitDialog
(hero, 50% at `-O1`) and town::buildBuilding (99.72% at `-O4`). The
checkpoint that adopted the profile banked 58 lower rows of unchanged source
as profile exceptions. History agrees with two of
them: hero was first calibrated at `-O1` on its leaf functions and patrol
body, and mapcell at `-O4` on its unrolled 58-iteration class loop, before
db6df82dc set one shared `-O3` as a working assumption.

Source decisions that a Mac comparison supports hold only under the unit's
own level. Under the old `-O3`, hero::getMobility looked better with the
masked flag (50 against 16); at hero's `-O1` the original
`(m_flags & 0x40000) != 0` source is exact.

To repeat the sweep, set the shared `flags` level in a scratch copy of
`config/mac/units.toml` (no unit overrides), run `homm3 configure`,
`ninja mac-objects` and `homm3 mac build --fast`, and count exact pairs per
unit in `build/mac/report.json`.

Exact pairs per unit and level:

| unit | pairs | -O1 | -O2 | -O3 | -O4 | level | rule |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| adventuremapwindow | 20 | 11 | 16 | 17 | 17 | -O4 | tie: shared default |
| adventureoptionswindow | 2 | 2 | 2 | 2 | 2 | -O4 | tie: shared default |
| advmgr | 57 | 23 | 26 | 29 | 31 | -O4 | most exact |
| advspells | 2 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| ai | 30 | 2 | 2 | 6 | 6 | -O4 | tie: shared default |
| ai_combat | 20 | 2 | 4 | 8 | 11 | -O4 | most exact |
| ai_player | 67 | 6 | 13 | 24 | 26 | -O4 | most exact |
| ai_tactical | 59 | 8 | 14 | 19 | 19 | -O4 | tie: shared default |
| army | 56 | 9 | 13 | 19 | 19 | -O4 | tie: shared default |
| armygrp | 25 | 3 | 5 | 12 | 13 | -O4 | most exact |
| bitmap16 | 8 | 2 | 2 | 2 | 2 | -O4 | tie: shared default |
| bitmap24 | 3 | 1 | 1 | 2 | 2 | -O4 | tie: shared default |
| bitmap816 | 8 | 3 | 3 | 5 | 5 | -O4 | tie: shared default |
| border | 14 | 1 | 3 | 5 | 7 | -O4 | most exact |
| bottomviewsubwindow | 2 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| button | 6 | 1 | 1 | 2 | 2 | -O4 | tie: shared default |
| campaign | 5 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| campaignbrief | 12 | 5 | 7 | 7 | 7 | -O4 | tie: shared default |
| castle | 1 | 0 | 0 | 0 | 1 | -O4 | most exact |
| cmbtmgr | 39 | 4 | 4 | 10 | 10 | -O4 | tie: shared default |
| combatcontrolsubwindow | 7 | 2 | 2 | 6 | 6 | -O4 | tie: shared default |
| combatresultswindow | 2 | 1 | 2 | 1 | 1 | -O2 | most exact |
| combatwindow | 8 | 4 | 4 | 6 | 6 | -O4 | tie: shared default |
| command | 24 | 2 | 7 | 11 | 11 | -O4 | tie: shared default |
| creature_bank | 1 | 0 | 1 | 1 | 1 | -O4 | tie: shared default |
| creaturetype | 2 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| csequence | 3 | 0 | 1 | 2 | 2 | -O4 | tie: shared default |
| cspriteframe | 2 | 1 | 1 | 1 | 1 | -O4 | tie: shared default |
| cursor | 6 | 3 | 3 | 4 | 4 | -O4 | tie: shared default |
| customcampaign | 69 | 17 | 19 | 33 | 34 | -O4 | most exact |
| customcampaignwindow | 6 | 1 | 3 | 3 | 4 | -O4 | most exact |
| dialogbox | 4 | 0 | 2 | 3 | 3 | -O4 | tie: shared default |
| diff | 4 | 1 | 1 | 2 | 2 | -O4 | tie: shared default |
| dimensiondoorwindow | 4 | 2 | 2 | 2 | 2 | -O4 | tie: shared default |
| drawing | 14 | 0 | 1 | 5 | 8 | -O4 | most exact |
| event_record | 56 | 13 | 14 | 24 | 30 | -O4 | most exact |
| events | 15 | 0 | 2 | 3 | 4 | -O4 | most exact |
| exec | 6 | 0 | 3 | 3 | 3 | -O4 | tie: shared default |
| findpath | 7 | 0 | 2 | 3 | 3 | -O4 | tie: shared default |
| fly | 4 | 0 | 1 | 1 | 1 | -O4 | tie: shared default |
| font | 12 | 4 | 4 | 6 | 6 | -O4 | tie: shared default |
| game | 77 | 12 | 15 | 34 | 36 | -O4 | most exact |
| gzinflatebuf | 1 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| hero | 71 | 39 | 18 | 9 | 8 | -O1 | most exact |
| hexcell | 3 | 0 | 3 | 3 | 3 | -O4 | tie: shared default |
| hillfortwindow | 1 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| hiscore | 2 | 2 | 2 | 2 | 2 | -O4 | tie: shared default |
| iconwdgt | 5 | 3 | 3 | 5 | 5 | -O4 | tie: shared default |
| initialize | 3 | 1 | 2 | 2 | 2 | -O4 | tie: shared default |
| inputmgr | 7 | 1 | 3 | 3 | 3 | -O4 | tie: shared default |
| kb | 17 | 1 | 2 | 5 | 7 | -O4 | most exact |
| kbwin | 2 | 1 | 1 | 2 | 2 | -O4 | tie: shared default |
| lodfile | 2 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| mapcell | 29 | 4 | 4 | 9 | 10 | -O4 | most exact |
| misc | 5 | 0 | 1 | 3 | 3 | -O4 | tie: shared default |
| mousemgr | 4 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| multiplayerwindow | 8 | 3 | 5 | 6 | 6 | -O4 | tie: shared default |
| netmsg | 2 | 0 | 1 | 1 | 1 | -O4 | tie: shared default |
| newgame | 5 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| objecttype | 1 | 0 | 0 | 1 | 1 | -O4 | tie: shared default |
| overview | 2 | 0 | 0 | 2 | 2 | -O4 | tie: shared default |
| palette | 5 | 2 | 2 | 2 | 2 | -O4 | tie: shared default |
| path | 8 | 3 | 4 | 4 | 4 | -O4 | tie: shared default |
| philai | 49 | 7 | 10 | 12 | 12 | -O4 | tie: shared default |
| puzzlewindow | 4 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| questlogwindow | 2 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| quicktownwindow | 2 | 1 | 1 | 2 | 2 | -O4 | tie: shared default |
| recruit | 8 | 1 | 4 | 6 | 6 | -O4 | tie: shared default |
| remote | 53 | 23 | 27 | 31 | 33 | -O4 | most exact |
| resourcemanager | 13 | 11 | 11 | 12 | 12 | -O4 | tie: shared default |
| rmg | 55 | 4 | 7 | 5 | 5 | -O2 | most exact |
| rmg_support | 1 | 1 | 1 | 1 | 1 | -O4 | tie: shared default |
| rmg_terrain | 8 | 5 | 5 | 6 | 6 | -O4 | tie: shared default |
| rmg_voronoi | 5 | 1 | 1 | 4 | 5 | -O4 | most exact |
| sacrifice_window | 16 | 4 | 7 | 10 | 12 | -O4 | most exact |
| sample | 1 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| scenarioinfo | 3 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| search | 6 | 0 | 0 | 0 | 1 | -O4 | most exact |
| seerhut | 96 | 15 | 20 | 43 | 56 | -O4 | most exact |
| seerhuttext | 1 | 0 | 0 | 0 | 1 | -O4 | most exact |
| singleselectionpopups | 7 | 1 | 1 | 1 | 1 | -O4 | tie: shared default |
| singleselectionwindow | 41 | 2 | 14 | 8 | 8 | -O2 | most exact |
| slider | 9 | 4 | 5 | 5 | 5 | -O4 | tie: shared default |
| smackmgr | 1 | 1 | 1 | 1 | 1 | -O4 | tie: shared default |
| soundmgr | 3 | 0 | 0 | 0 | 0 | -O4 | tie: shared default |
| spellbookwindow | 2 | 2 | 2 | 2 | 2 | -O4 | tie: shared default |
| spells | 21 | 2 | 4 | 7 | 7 | -O4 | tie: shared default |
| strip | 4 | 1 | 1 | 2 | 2 | -O4 | tie: shared default |
| subwindow | 5 | 1 | 2 | 3 | 4 | -O4 | most exact |
| swapmgr | 4 | 2 | 2 | 2 | 2 | -O4 | tie: shared default |
| textntry | 7 | 2 | 2 | 3 | 3 | -O4 | tie: shared default |
| textresource | 2 | 2 | 2 | 2 | 2 | -O4 | tie: shared default |
| textscroller | 4 | 1 | 1 | 1 | 1 | -O4 | tie: shared default |
| textwdgt | 5 | 2 | 2 | 2 | 3 | -O4 | most exact |
| town | 37 | 2 | 4 | 12 | 14 | -O4 | most exact |
| towngatewindow | 3 | 0 | 0 | 2 | 3 | -O4 | most exact |
| townmgr | 24 | 8 | 5 | 4 | 4 | -O1 | most exact |
| tradpost | 8 | 2 | 1 | 1 | 1 | -O1 | most exact |
| university_window | 4 | 2 | 3 | 3 | 3 | -O4 | tie: shared default |
| victorylossconditions | 19 | 0 | 1 | 2 | 2 | -O4 | tie: shared default |
| viewarmywindow | 3 | 1 | 2 | 2 | 2 | -O4 | tie: shared default |
| widget | 7 | 2 | 6 | 6 | 6 | -O4 | tie: shared default |
| window | 25 | 8 | 11 | 16 | 16 | -O4 | tie: shared default |
| winmgr | 14 | 2 | 3 | 6 | 6 | -O4 | tie: shared default |
