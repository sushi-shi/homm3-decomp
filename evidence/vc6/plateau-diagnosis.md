<!-- # generator: homm3.vc6.report | # date: 2026-09-02 | # ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - regenerate, never hand-edit | plateaus in [50.0, 99.999%); base-vs-delinked-target diagnosis, no recompiles -->
# vc6 plateau diagnosis (read-only; solvers propose, never land)

535 function(s). why-reg = register-homing knobs; why-branch = control-flow knobs; predict-inline = out-of-line CALL multiset divergence (a callee inlined on one side only - dominated by STL basic_string/vector ops + small dtors retail inlines and we do not). CALIBRATION 2026-08-19: this column USED to be dominated by a NAME artifact - retail's side names an unclaimed callee with a synth working label our compiled side can never emit, so one call booked as both an under- and an over-inline and the inliner route (which sits upstream of registers and blocks) buried the true diagnosis. inline_model.divergence now pairs those off by count: on the tree of that date the inliner class fell from 135 rows to 46 of 211, and register-homing (108) overtook it as the dominant plateau class. MECHANISM (RE'd, docs/vc6/inliner.md): /Ob2 budget = clamp(2*caller_cb,1000,35000) spent sequentially; our leaner reconstructions sit at the 1000 floor and STARVE, so retail inlines what we call. FIX = finish the caller's body (budget follows statement mass, byte-inert counts) - do NOT chase _Tidy/vector spellings or pragmas. So on LOW-% rows inline divergence largely self-resolves as reconstruction completes; it is the pure wall only on high-% rows. Mixed walls list both distances.

## Wall-class summary

- **229** register-homing (why-reg)
- **183** inliner (predict-inline)
- **85** control-flow (why-branch)
- **34** unclassified

| fuzzy | unit | function | wall class | reg-dist | flow-dist | knob to try |
|---|---|---|---|---|---|---|
| 50.39 | events | `?DoEventArtifact@advManager@@QAEXPAVhero@@PA..` | inliner (predict-inline) | 166 | 30 | callee expanded on one side only (A8/A9/A12): 2 under-inline (4 name-unresolvable pair(s) discounted) |
| 54.05 | ai | `?choose_resurrect_action@combatManager@@QAEE..` | inliner (predict-inline) | 184 | 7 | callee expanded on one side only (A8/A9/A12): 12 under-inline, 2 over-inline (3 name-unresolvable pair(s) discounted) |
| 60.55 | game | `?Load@game@@QAEHPAVTAbstractFile@@@Z` | inliner (predict-inline) | 1229 | 138 | callee expanded on one side only (A8/A9/A12): 66 under-inline, 1 over-inline (5 name-unresolvable pair(s) discounted) |
| 60.93 | singleselectionwindow | `?CreateFilterWidgets@TSingleSelectionWindow@..` | inliner (predict-inline) | 2237 | 495 | callee expanded on one side only (A8/A9/A12): 51 under-inline (128 name-unresolvable pair(s) discounted) |
| 63.94 | advmgr | `?SetEnvironmentOrigin@advManager@@QAEXUtype_..` | register-homing (why-reg) | 231 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 64.31 | philai | `??0type_spellvalue@@QAE@PBVhero@@@Z` | control-flow (why-branch) | 199 | 41 | loop-form / merged-return placement / case order (D1-D9) |
| 65.70 | singleselectionwindow | `?GetHeroFace@TSingleSelectionWindow@@QAEXHPA..` | control-flow (why-branch) | 161 | 29 | loop-form / merged-return placement / case order (D1-D9) |
| 66.81 | townmgr | `?Main@townManager@@UAEHAAVmessage@@@Z` | inliner (predict-inline) | 1418 | 441 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 5 over-inline (28 name-unresolvable pair(s) discounted) |
| 67.07 | spellbookwindow | `?WindowHandler@TSpellbookWindow@@UAEHPAVmess..` | inliner (predict-inline) | 540 | 234 | callee expanded on one side only (A8/A9/A12): 7 over-inline (10 name-unresolvable pair(s) discounted) |
| 67.91 | singleselectionwindow | `?UpdateGameVars@TSingleSelectionWindow@@QAEX..` | inliner (predict-inline) | 161 | 11 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 3 over-inline (6 name-unresolvable pair(s) discounted) |
| 68.05 | singleselectionwindow | `?ExitDialog@TSingleSelectionWindow@@UAEHPAVm..` | control-flow (why-branch) | 32 | 17 | loop-form / merged-return placement / case order (D1-D9) |
| 68.50 | ai_tactical | `?get_attack_skill_value@type_AI_spellcaster@..` | inliner (predict-inline) | 111 | 10 | callee expanded on one side only (A8/A9/A12): 5 under-inline (1 name-unresolvable pair(s) discounted) |
| 68.91 | hero | `?GiveArtifact@hero@@QAEEPBUtype_artifact@@EE..` | inliner (predict-inline) | 163 | 48 | callee expanded on one side only (A8/A9/A12): 6 under-inline, 2 over-inline (3 name-unresolvable pair(s) discounted) |
| 70.09 | dxplay | `?FlushReceiveQueue@CDPlay@@UAEEXZ` | control-flow (why-branch) | 96 | 19 | loop-form / merged-return placement / case order (D1-D9) |
| 70.46 | singleselectionwindow | `?UpdatePlayerPositions@TSingleSelectionWindo..` | control-flow (why-branch) | 145 | 46 | loop-form / merged-return placement / case order (D1-D9) |
| 70.65 | game | `?LoadMap@game@@QAE_NPAVTAbstractFile@@@Z` | inliner (predict-inline) | 662 | 74 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 9 over-inline (13 name-unresolvable pair(s) discounted) |
| 71.82 | spells | `?SummonElemental@combatManager@@QAEXHW4TCrea..` | inliner (predict-inline) | 114 | 33 | callee expanded on one side only (A8/A9/A12): 7 under-inline (2 name-unresolvable pair(s) discounted) |
| 71.90 | singleselectionwindow | `?Tick@t_map_list_update@@UAEXXZ` | control-flow (why-branch) | 151 | 8 | loop-form / merged-return placement / case order (D1-D9) |
| 72.22 | ai_player | `?buy_creatures@type_AI_player@@QAEXPAVhero@@..` | inliner (predict-inline) | 324 | 8 | callee expanded on one side only (A8/A9/A12): 2 under-inline (4 name-unresolvable pair(s) discounted) |
| 72.85 | game | `?TransmitSaveGame@game@@QAEHHHEE@Z` | inliner (predict-inline) | 1116 | 67 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 7 over-inline (22 name-unresolvable pair(s) discounted) |
| 73.33 | inputmgr | `??0inputManager@@QAE@XZ` | unclassified | 8 | 0 | run why-reg / why-branch for the full search |
| 73.58 | multiplayerwindow | `?OnTCP@TMultiPlayerWindow@@QAEEXZ` | control-flow (why-branch) | 56 | 6 | loop-form / merged-return placement / case order (D1-D9) |
| 74.22 | hero | `?WindowHandler@THeroScreenWindow@@UAEHPAVmes..` | inliner (predict-inline) | 1308 | 369 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 11 over-inline (14 name-unresolvable pair(s) discounted) |
| 74.41 | viewarmywindow | `?WindowHandler@TViewArmyWindow@@UAEHPAVmessa..` | inliner (predict-inline) | 664 | 167 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 2 over-inline (15 name-unresolvable pair(s) discounted) |
| 75.00 | singleselectionwindow | `??0TSingleSelectionWindow@@QAE@H@Z` | inliner (predict-inline) | 2820 | 283 | callee expanded on one side only (A8/A9/A12): 23 under-inline, 45 over-inline (213 name-unresolvable pair(s) discounted) |
| 75.01 | cmbtmgr | `?CalculateGainedExperience@combatManager@@QA..` | register-homing (why-reg) | 88 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 75.24 | seerhut | `?DoProgressDialog@type_artifact_quest@@UAEXXZ` | register-homing (why-reg) | 64 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 75.24 | philai | `?get_value_of_well@@YIJPBVhero@@G@Z` | inliner (predict-inline) | 48 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline |
| 75.38 | ai_player | `?consider_hiring@@YI_NJPAVhero@@@Z` | inliner (predict-inline) | 276 | 40 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 75.43 | seerhut | `?DoProposalDialog@type_skill_quest@@UAEXPAVh..` | inliner (predict-inline) | 162 | 0 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 3 over-inline (11 name-unresolvable pair(s) discounted) |
| 75.55 | cmbtmgr | `?place_obstacle@combatManager@@QAEEH@Z` | inliner (predict-inline) | 169 | 47 | callee expanded on one side only (A8/A9/A12): 1 under-inline (2 name-unresolvable pair(s) discounted) |
| 75.84 | seerhut | `?DoProgressDialog@type_skill_quest@@UAEXXZ` | control-flow (why-branch) | 88 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 75.86 | victorylossconditions | `?CheckForDefeatedHeroLoss@LossConditionStruc..` | control-flow (why-branch) | 143 | 162 | loop-form / merged-return placement / case order (D1-D9) |
| 76.16 | singleselectionwindow | `?SetHumanSlot@TSingleSelectionWindow@@QAEXXZ` | control-flow (why-branch) | 133 | 126 | loop-form / merged-return placement / case order (D1-D9) |
| 76.61 | philai | `?get_value_of_spring@@YIJPBVhero@@PBVNewmapC..` | inliner (predict-inline) | 52 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline |
| 76.87 | ai_player | `?AI_choose_destination@@YIHPAVhero@@JAAUHero..` | inliner (predict-inline) | 884 | 132 | callee expanded on one side only (A8/A9/A12): 2 over-inline (5 name-unresolvable pair(s) discounted) |
| 76.88 | spells | `?ModifySpellDamage@combatManager@@QAEJJHPBVh..` | inliner (predict-inline) | 80 | 41 | callee expanded on one side only (A8/A9/A12): 2 under-inline (4 name-unresolvable pair(s) discounted) |
| 76.91 | mapcell | `?readResourceData@NewfullMap@@QAEHPAVTAbstra..` | control-flow (why-branch) | 78 | 18 | loop-form / merged-return placement / case order (D1-D9) |
| 76.92 | game | `?Save@game@@QAEHPAVTAbstractFile@@@Z` | inliner (predict-inline) | 879 | 20 | callee expanded on one side only (A8/A9/A12): 27 under-inline (5 name-unresolvable pair(s) discounted) |
| 77.01 | singleselectionwindow | `?GetDisplayFace@TSingleSelectionWindow@@QAEH..` | inliner (predict-inline) | 47 | 5 | callee expanded on one side only (A8/A9/A12): 2 over-inline (1 name-unresolvable pair(s) discounted) |
| 77.49 | game | `?ClaimShipyard@game@@QAEXUtype_point@@H@Z` | control-flow (why-branch) | 218 | 16 | loop-form / merged-return placement / case order (D1-D9) |
| 77.61 | ai_player | `?mark_destinations@@YIJPAVhero@@JPAVsearchAr..` | register-homing (why-reg) | 288 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 77.73 | levelupwindow | `?WindowHandler@TLevelUpWindow@@UAEHPAVmessag..` | inliner (predict-inline) | 104 | 36 | callee expanded on one side only (A8/A9/A12): 3 over-inline (2 name-unresolvable pair(s) discounted) |
| 78.12 | remote | `??_GCNetMsgHandler@@UAEPAXI@Z` | register-homing (why-reg) | 6 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 78.20 | philai | `?get_skill_value@@YIJPBVhero@@W4TSecondarySk..` | control-flow (why-branch) | 269 | 60 | loop-form / merged-return placement / case order (D1-D9) |
| 78.86 | hero | `?HeroFn_004E2840@hero@@QAEEJJ@Z` | inliner (predict-inline) | 86 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 79.26 | singleselectionwindow | `??0GameSelectionHeadersStruct@@QAE@XZ` | inliner (predict-inline) | 128 | 5 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (4 name-unresolvable pair(s) discounted) |
| 79.28 | winmgr | `?DoDialog@heroWindowManager@@QAEHPAVheroWind..` | register-homing (why-reg) | 123 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 79.66 | events | `?GiveBlackBoxReward@advManager@@QAE_NPBDPAVh..` | inliner (predict-inline) | 1022 | 170 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 6 over-inline (59 name-unresolvable pair(s) discounted) |
| 79.75 | remote | `??1CWaitForReadyPlayersDlg@@UAE@XZ` | inliner (predict-inline) | 35 | 3 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 79.94 | ai | `?mark_moat@combatManager@@QAEXPBVarmy@@PAJPA..` | register-homing (why-reg) | 63 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 80.21 | ai_tactical | `?get_protection_value@type_AI_spellcaster@@Q..` | register-homing (why-reg) | 132 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 80.29 | university_window | `?purchase_click@type_university_window@@SIHP..` | inliner (predict-inline) | 80 | 30 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 80.39 | game | `?ProcessOnMapTowns@game@@QAEXXZ` | inliner (predict-inline) | 132 | 49 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 5 over-inline (2 name-unresolvable pair(s) discounted) |
| 80.40 | advmgr | `?ProcessKeyPress@advManager@@QAEHPBVmessage@..` | inliner (predict-inline) | 644 | 116 | callee expanded on one side only (A8/A9/A12): 7 over-inline (6 name-unresolvable pair(s) discounted) |
| 80.51 | artifact | `?InitializeArtifactTraitsTable@@YIEXZ` | control-flow (why-branch) | 289 | 38 | loop-form / merged-return placement / case order (D1-D9) |
| 80.54 | singleselectionwindow | `?GetHeroName@TSingleSelectionWindow@@QAEPBDH..` | control-flow (why-branch) | 81 | 50 | loop-form / merged-return placement / case order (D1-D9) |
| 80.59 | multiplayerwindow | `??0TMultiPlayerWindow@@QAE@XZ` | inliner (predict-inline) | 695 | 87 | callee expanded on one side only (A8/A9/A12): 16 over-inline (62 name-unresolvable pair(s) discounted) |
| 80.93 | hero | `?update_spell_list@hero@@QAEXXZ` | register-homing (why-reg) | 95 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 81.03 | ai_player | `?can_trade_resources@type_AI_player@@QAE_NPB..` | control-flow (why-branch) | 455 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 81.04 | path | `?ValidAttack@army@@QBEHHHHHPAH@Z` | control-flow (why-branch) | 70 | 59 | loop-form / merged-return placement / case order (D1-D9) |
| 81.04 | iconwdgt | `?NextRandomFrame@iconWidget@@QAEXXZ` | register-homing (why-reg) | 130 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 81.19 | town | `?can_build@town@@QBEEF@Z` | register-homing (why-reg) | 76 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 81.30 | singleselectionwindow | `?OnGameHeaderInfoInitMsg@TSingleSelectionWin..` | inliner (predict-inline) | 221 | 10 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 2 over-inline (17 name-unresolvable pair(s) discounted) |
| 81.33 | game | `?NextPlayer@game@@QAEXXZ` | inliner (predict-inline) | 398 | 133 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 81.37 | command | `?is_computer_action@combatManager@@QAEEPBVar..` | inliner (predict-inline) | 44 | 50 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 81.41 | game | `?get_underground_gate_exit@game@@QAE?AUtype_..` | register-homing (why-reg) | 67 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.41 | townmgr | `?SetRolloverText@TTavernWindow@@QAEXH@Z` | register-homing (why-reg) | 65 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.42 | cspriteframe | `?DrawTile@CSpriteFrame@@QBEXHHHHPAGHHHHHAAVT..` | register-homing (why-reg) | 1547 | 3 | spill to dead-parameter slot (B4) |
| 81.49 | town | `?get_build_cost@town@@QBEFW4type_building_id..` | register-homing (why-reg) | 35 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.64 | campaignwindow | `?CampaignWindowHandler@@YIHAAVmessage@@@Z` | register-homing (why-reg) | 60 | 7 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.67 | cmbtmgr | `??1TCombatEagleEyeSide@@QAE@XZ` | inliner (predict-inline) | 8 | 0 | callee expanded on one side only (A8/A9/A12): 2 over-inline (1 name-unresolvable pair(s) discounted) |
| 81.70 | hero | `?update_slot@THeroScreenWindow@@QAEXJ@Z` | control-flow (why-branch) | 110 | 10 | loop-form / merged-return placement / case order (D1-D9) |
| 81.73 | town | `?get_legion_bonus@town@@QAEJJ@Z` | control-flow (why-branch) | 21 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 81.76 | hero | `?GetMobility@hero@@QAEHE@Z` | control-flow (why-branch) | 212 | 14 | loop-form / merged-return placement / case order (D1-D9) |
| 82.29 | hillfortwindow | `?Recalculate@THillFortWindow@@QAEXE@Z` | inliner (predict-inline) | 240 | 2 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 82.34 | spells | `?MirrorImage@combatManager@@QAEXHH@Z` | control-flow (why-branch) | 220 | 71 | loop-form / merged-return placement / case order (D1-D9) |
| 82.49 | ai | `?place_shooter@combatManager@@QAEXPBVarmy@@@Z` | control-flow (why-branch) | 79 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 82.55 | search | `?BuildPath@searchArray@@QAEHPBVhero@@J@Z` | inliner (predict-inline) | 151 | 17 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (4 name-unresolvable pair(s) discounted) |
| 82.56 | ai_player | `?get_value@type_necromancy_artifact@@UBEJPBV..` | register-homing (why-reg) | 19 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 82.86 | hero | `?initialize@hero@@QAEXF@Z` | inliner (predict-inline) | 99 | 17 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 82.93 | mousemgr | `?SetPointer@mouseManager@@QAEXHW4EPointerSet..` | register-homing (why-reg) | 25 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 82.96 | mapcell | `?readHeroData@NewfullMap@@QAEHPAVTAbstractFi..` | inliner (predict-inline) | 591 | 30 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 2 over-inline (4 name-unresolvable pair(s) discounted) |
| 83.02 | advmgr | `?ShowRoute@advManager@@QAEXHHH@Z` | register-homing (why-reg) | 253 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 83.04 | events | `?DoTreasureDialog@advManager@@QAEXPAVhero@@H..` | inliner (predict-inline) | 39 | 8 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (1 name-unresolvable pair(s) discounted) |
| 83.36 | combatoptionswindow | `?CombatOptionsWindowHandler@@YIHAAVmessage@@..` | inliner (predict-inline) | 156 | 22 | callee expanded on one side only (A8/A9/A12): 4 over-inline (3 name-unresolvable pair(s) discounted) |
| 83.43 | hero | `?CheckLevel@hero@@QAEXXZ` | inliner (predict-inline) | 110 | 41 | callee expanded on one side only (A8/A9/A12): 3 over-inline |
| 83.50 | advmgr | `?ProcessHover@advManager@@QAEHHH@Z` | inliner (predict-inline) | 492 | 105 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 83.62 | ai_player | `?split_armies@@YIXPAVhero@@PBV1@PBVarmyGroup..` | control-flow (why-branch) | 127 | 30 | loop-form / merged-return placement / case order (D1-D9) |
| 83.87 | game | `?ViewArmy@game@@QAEXAAVarmyGroup@@HPBVhero@@..` | inliner (predict-inline) | 128 | 2 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline (1 name-unresolvable pair(s) discounted) |
| 83.92 | diff | `?MakeDiff@CDiffMaker@@QAEPAVCDiffFile@@AAK@Z` | register-homing (why-reg) | 111 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 84.03 | event_record | `?load@type_record_shroud@@UAEEPAVTAbstractFi..` | inliner (predict-inline) | 74 | 11 | callee expanded on one side only (A8/A9/A12): 1 under-inline (2 name-unresolvable pair(s) discounted) |
| 84.05 | swapmgr | `?Main@swapManager@@UAEHAAVmessage@@@Z` | inliner (predict-inline) | 427 | 10 | callee expanded on one side only (A8/A9/A12): 1 under-inline (5 name-unresolvable pair(s) discounted) |
| 84.11 | resourcemanager | `?Open@ResourceManager@@YI_N_N0PAH@Z` | inliner (predict-inline) | 164 | 24 | callee expanded on one side only (A8/A9/A12): 1 over-inline (6 name-unresolvable pair(s) discounted) |
| 84.17 | diff | `?FindNextSame@CDiffMaker@@IAE_NHHAAH0@Z` | register-homing (why-reg) | 14 | 0 | register-homing knob (B-family) |
| 84.38 | ai_tactical | `?get_hex_attack_value@type_AI_attack_hex_cho..` | register-homing (why-reg) | 94 | 1 | spill to dead-parameter slot (B4) |
| 84.41 | game | `?ClaimGarrison@game@@QAEXHH@Z` | register-homing (why-reg) | 10 | 0 | register-homing knob (B-family) |
| 84.47 | remote | `?WaitForReadyToPlayMsg@@YIXXZ` | inliner (predict-inline) | 41 | 20 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 84.53 | soundmgr | `?Open@soundManager@@UAEHH@Z` | control-flow (why-branch) | 122 | 10 | loop-form / merged-return placement / case order (D1-D9) |
| 84.59 | ai_player | `?make_gift@type_AI_player@@QAEXJ@Z` | register-homing (why-reg) | 279 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 84.78 | event_record | `?ResetVisibility@game@@QAEXHHHHH@Z` | register-homing (why-reg) | 200 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 84.83 | ai_player | `?AI_AttemptMove@@YIXPAVhero@@AAUHeroDestinat..` | inliner (predict-inline) | 176 | 42 | callee expanded on one side only (A8/A9/A12): 1 over-inline (5 name-unresolvable pair(s) discounted) |
| 84.85 | armygrp | `?Merge@armyGroup@@QAEEPAV1@@Z` | control-flow (why-branch) | 95 | 26 | loop-form / merged-return placement / case order (D1-D9) |
| 84.96 | spells | `?Earthquake@combatManager@@QAEXH@Z` | register-homing (why-reg) | 283 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 85.04 | singleselectionwindow | `??4GameSelectionHeadersStruct@@QAEAAU0@ABU0@..` | inliner (predict-inline) | 68 | 16 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (8 name-unresolvable pair(s) discounted) |
| 85.23 | advmgr | `?DrawAdvObjShadow@advManager@@QAEXHHHHH@Z` | register-homing (why-reg) | 415 | 0 | spill to dead-parameter slot (B4) |
| 85.35 | ai_player | `?net_value_of_location@@YIHPAVhero@@PAUHeroD..` | register-homing (why-reg) | 190 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 85.41 | town | `?destroy_extra_capitol@town@@QAEXXZ` | control-flow (why-branch) | 51 | 15 | loop-form / merged-return placement / case order (D1-D9) |
| 85.63 | advmgr | `?ViewPuzzle@advManager@@QAEXXZ` | inliner (predict-inline) | 69 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 85.65 | event_record | `?SetVisibility@game@@QAEXHHHHHE@Z` | register-homing (why-reg) | 190 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 85.83 | ai_player | `?check_trade_supply@type_AI_player@@QAE_NPBH..` | register-homing (why-reg) | 73 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 85.90 | command | `?CheckSetMouseDirection@combatManager@@QAEEH..` | control-flow (why-branch) | 69 | 8 | loop-form / merged-return placement / case order (D1-D9) |
| 85.98 | town | `?create_building@town@@QAE?AW4type_building_..` | inliner (predict-inline) | 270 | 1 | callee expanded on one side only (A8/A9/A12): 4 under-inline |
| 86.03 | game | `?read_map_hero_setups@game@@QAEXPAVTAbstract..` | inliner (predict-inline) | 64 | 3 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 3 over-inline (2 name-unresolvable pair(s) discounted) |
| 86.06 | singleselectionwindow | `?OnPlayerPosClick@TSingleSelectionWindow@@QA..` | inliner (predict-inline) | 93 | 40 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 86.09 | hero | `?UpdateStats@hero@@QAEXXZ` | control-flow (why-branch) | 28 | 8 | loop-form / merged-return placement / case order (D1-D9) |
| 86.13 | sacrifice_window | `?backpack_click@type_sacrifice_window@@QAEXJ..` | inliner (predict-inline) | 69 | 16 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 86.13 | resourcemanager | `@game_sprite_1599e0@12` | inliner (predict-inline) | 194 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (15 name-unresolvable pair(s) discounted) |
| 86.37 | advmgr | `?Close@advManager@@UAEXXZ` | inliner (predict-inline) | 151 | 4 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 86.37 | army | `?spell_is_valid_on_target@@YIEHPBVarmy@@@Z` | inliner (predict-inline) | 98 | 37 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 86.64 | philai | `?MoveHero@@YIXPAVhero@@PAJEPAE@Z` | register-homing (why-reg) | 231 | 4 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 86.66 | viewarmywindow | `?create_spell_influence_widgets@TViewArmyWin..` | control-flow (why-branch) | 165 | 14 | loop-form / merged-return placement / case order (D1-D9) |
| 86.67 | adventuremapwindow | `?SetSleepImage@TAdventureMapWindow@@QAEXH@Z` | inliner (predict-inline) | 145 | 24 | callee expanded on one side only (A8/A9/A12): 1 under-inline (9 name-unresolvable pair(s) discounted) |
| 86.67 | singleselectionwindow | `?Tick@CNewPlayerUpdateProc@@UAEXXZ` | inliner (predict-inline) | 108 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline (7 name-unresolvable pair(s) discounted) |
| 86.68 | ai_tactical | `?should_attack_now@type_AI_spellcaster@@QAEE..` | register-homing (why-reg) | 58 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 86.78 | sacrifice_window | `?all_artifacts@type_sacrifice_window@@CIHAAV..` | inliner (predict-inline) | 65 | 8 | callee expanded on one side only (A8/A9/A12): 4 over-inline (2 name-unresolvable pair(s) discounted) |
| 86.87 | tradpost | `?Update@TTradeResourceWindow@@QAEXE@Z` | control-flow (why-branch) | 259 | 9 | loop-form / merged-return placement / case order (D1-D9) |
| 86.93 | game | `?RandomizeEvents@game@@QAEXXZ` | inliner (predict-inline) | 1037 | 67 | callee expanded on one side only (A8/A9/A12): 12 under-inline, 15 over-inline (17 name-unresolvable pair(s) discounted) |
| 86.98 | remote | `?SendIt@CDPlayHeroes@@QAE_NPAVCNetMsg@@K_N@Z` | control-flow (why-branch) | 32 | 12 | loop-form / merged-return placement / case order (D1-D9) |
| 86.99 | hero | `?can_summon_boat@hero@@QAEEXZ` | register-homing (why-reg) | 41 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.00 | game | `?Save@NewSMapHeader@@QAEHPAVTAbstractFile@@@Z` | inliner (predict-inline) | 396 | 88 | callee expanded on one side only (A8/A9/A12): 5 over-inline (6 name-unresolvable pair(s) discounted) |
| 87.02 | singleselectionwindow | `?Finish@CNewPlayerUpdateProc@@UAEXXZ` | register-homing (why-reg) | 22 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.04 | ai_combat | `?cast_spell@type_AI_combat_data@@QAEXAAV1@W4..` | register-homing (why-reg) | 305 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.06 | townmgr | `?SetupThievesGuild@TThievesGuildWindow@@QAEX..` | inliner (predict-inline) | 1014 | 120 | callee expanded on one side only (A8/A9/A12): 1 over-inline (57 name-unresolvable pair(s) discounted) |
| 87.07 | singleselectionwindow | `?GetHeaders@TSingleSelectionWindow@@QAEXPAV?..` | inliner (predict-inline) | 88 | 21 | callee expanded on one side only (A8/A9/A12): 3 under-inline (11 name-unresolvable pair(s) discounted) |
| 87.11 | ai | `?find_attack_hexes@@YIXPBVarmy@@JJJJPBVsearc..` | inliner (predict-inline) | 110 | 29 | callee expanded on one side only (A8/A9/A12): 1 over-inline (8 name-unresolvable pair(s) discounted) |
| 87.19 | smackmgr | `?VideoPlay@@YIHHHHHH@Z` | register-homing (why-reg) | 143 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 87.27 | hero | `?HeroFn_004DC100@hero@@QAEXJ@Z` | inliner (predict-inline) | 70 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 87.28 | singleselectionpopups | `?CreateWin@CTownDlg@@QAEEPAVCSprite@@HW4TTow..` | register-homing (why-reg) | 273 | 1 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 87.34 | singleselectionwindow | `?MakeHeroFilter@TSingleSelectionWindow@@QAEX..` | inliner (predict-inline) | 150 | 0 | callee expanded on one side only (A8/A9/A12): 2 over-inline (2 name-unresolvable pair(s) discounted) |
| 87.42 | townmgr | `?set_prerequisite_text@TBuyBuildWindow@@QAEX..` | register-homing (why-reg) | 68 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.55 | findpath | `?PushPoint@searchArray@@QAEXPBUpathCell@@PAU..` | inliner (predict-inline) | 320 | 116 | callee expanded on one side only (A8/A9/A12): 2 under-inline (19 name-unresolvable pair(s) discounted) |
| 87.59 | advmgr | `?DrawAdvObj@advManager@@QAEXHHHHH@Z` | register-homing (why-reg) | 734 | 2 | spill to dead-parameter slot (B4) |
| 87.65 | findpath | `?FindCombatPath@searchArray@@QAEEPBVarmy@@JJ..` | inliner (predict-inline) | 340 | 22 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (7 name-unresolvable pair(s) discounted) |
| 87.73 | ai_tactical | `?find_enemy_attacks@type_AI_spellcaster@@QAE..` | register-homing (why-reg) | 128 | 4 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.74 | hero | `?GetLuck@hero@@QAEHPBV1@EE@Z` | control-flow (why-branch) | 52 | 23 | loop-form / merged-return placement / case order (D1-D9) |
| 87.77 | command | `?GetControl@combatManager@@QAEXXZ` | control-flow (why-branch) | 70 | 50 | loop-form / merged-return placement / case order (D1-D9) |
| 87.85 | game | `??0game@@QAE@XZ` | inliner (predict-inline) | 65 | 3 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (3 name-unresolvable pair(s) discounted) |
| 87.93 | townmgr | `?set_bonus_display@TTownScreenWindow@@QAEXPA..` | control-flow (why-branch) | 298 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 87.94 | hero | `?load@hero@@QAEHPAVTAbstractFile@@H@Z` | inliner (predict-inline) | 155 | 21 | callee expanded on one side only (A8/A9/A12): 4 under-inline (5 name-unresolvable pair(s) discounted) |
| 88.09 | army | `?do_post_attack@army@@AAEXPAV1@HHH@Z` | inliner (predict-inline) | 279 | 137 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 1 over-inline (7 name-unresolvable pair(s) discounted) |
| 88.10 | button | `?Main@button@@UAEHPAVmessage@@@Z` | inliner (predict-inline) | 245 | 48 | callee expanded on one side only (A8/A9/A12): 4 under-inline (4 name-unresolvable pair(s) discounted) |
| 88.14 | winmgr | `?FadeFromBlack@heroWindowManager@@QAEXH@Z` | register-homing (why-reg) | 108 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.34 | ai_player | `?choose_weakest_army@type_AI_creature_swappe..` | register-homing (why-reg) | 119 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.40 | hero | `?get_description@type_artifact@@QBE?AV?$basi..` | control-flow (why-branch) | 100 | 7 | loop-form / merged-return placement / case order (D1-D9) |
| 88.42 | dxplay | `?TestLobbied@CDPlayLobby@@QAEEXZ` | control-flow (why-branch) | 8 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 88.44 | button | `??0textButton@@QAE@HHHHHPBD00HHEHHH@Z` | inliner (predict-inline) | 53 | 2 | callee expanded on one side only (A8/A9/A12): 2 over-inline (1 name-unresolvable pair(s) discounted) |
| 88.44 | slider | `?SetKnob@slider@@IAEXH@Z` | control-flow (why-branch) | 20 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 88.45 | swapmgr | `?OnWidgetDeselect@swapManager@@QAEXAAVmessag..` | register-homing (why-reg) | 43 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 88.49 | spells | `?ValidSpellTarget@combatManager@@QAEEHJJJEJ@Z` | control-flow (why-branch) | 138 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 88.51 | armygrp | `?get_spell_work_chance@@YIMHW4TCreatureType@..` | control-flow (why-branch) | 236 | 138 | loop-form / merged-return placement / case order (D1-D9) |
| 88.51 | winmgr | `?FadeToBlack@heroWindowManager@@QAEXHE@Z` | register-homing (why-reg) | 128 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.55 | advmgr | `?DoAdvCommand@advManager@@QAEPAVNewmapCell@@..` | control-flow (why-branch) | 435 | 11 | loop-form / merged-return placement / case order (D1-D9) |
| 88.56 | events | `?monsters_give_reward@advManager@@QAEXPAVher..` | control-flow (why-branch) | 52 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 88.62 | singleselectionwindow | `?HeaderRequested@CNewPlayerUpdateMan@@QAEXKE..` | control-flow (why-branch) | 55 | 16 | loop-form / merged-return placement / case order (D1-D9) |
| 88.65 | singleselectionwindow | `?ProcessRightSelect@TSingleSelectionWindow@@..` | inliner (predict-inline) | 458 | 42 | callee expanded on one side only (A8/A9/A12): 7 under-inline, 7 over-inline |
| 88.66 | tradpost | `?Update@TBuyArtifactWindow@@QAEXE@Z` | register-homing (why-reg) | 302 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.70 | event_record | `?record_erase_object@game@@QAEXPAVNewmapCell..` | inliner (predict-inline) | 81 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (9 name-unresolvable pair(s) discounted) |
| 88.86 | resourcemanager | `?GetSprite@ResourceManager@@YIPAVCSprite@@PB..` | register-homing (why-reg) | 443 | 1 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 88.87 | spellbookwindow | `?GotoPage@TSpellbookWindow@@QAEXH@Z` | inliner (predict-inline) | 152 | 52 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (8 name-unresolvable pair(s) discounted) |
| 88.93 | event_record | `?record_hide_boat@game@@QAEXPAVboat@@EH@Z` | inliner (predict-inline) | 73 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline (9 name-unresolvable pair(s) discounted) |
| 88.93 | smackmgr | `?VideoRealignBuffers@@YIXXZ` | unclassified | 6 | 0 | run why-reg / why-branch for the full search |
| 89.04 | game | `?ValidateVictoryLossConditions@game@@QAEXE@Z` | control-flow (why-branch) | 110 | 282 | loop-form / merged-return placement / case order (D1-D9) |
| 89.09 | ai_player | `?value_of_adding_army@type_AI_creature_swapp..` | register-homing (why-reg) | 155 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.09 | ai_tactical | `?consider_sacrifice@type_AI_spellcaster@@QBE..` | control-flow (why-branch) | 118 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 89.10 | seerhut | `?DoProposalDialog@type_artifact_quest@@UAEXP..` | inliner (predict-inline) | 58 | 1 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline (12 name-unresolvable pair(s) discounted) |
| 89.28 | hero | `??0hero@@QAE@XZ` | register-homing (why-reg) | 53 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.32 | spells | `?DrawBolt@combatManager@@QAEXPAUSBolt@@H@Z` | register-homing (why-reg) | 164 | 3 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 89.34 | events | `?CreatureBankEvent@advManager@@QAEHPAVhero@@..` | inliner (predict-inline) | 355 | 49 | callee expanded on one side only (A8/A9/A12): 3 over-inline (15 name-unresolvable pair(s) discounted) |
| 89.34 | cmbtmgr | `?ShootBallisticMissile@combatManager@@QAEXHH..` | register-homing (why-reg) | 102 | 1 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 89.35 | swapmgr | `?Update@swapManager@@QAEXXZ` | register-homing (why-reg) | 97 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.39 | resourcemanager | `@game_null_159510@12` | inliner (predict-inline) | 183 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (15 name-unresolvable pair(s) discounted) |
| 89.46 | ai_combat | `?get_enchantment_value@type_AI_combat_data@@..` | register-homing (why-reg) | 107 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.47 | iconwdgt | `?NextRandomSiegeEngineFrame@iconWidget@@QAEX..` | register-homing (why-reg) | 33 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 89.47 | combatwindow | `?CheckCombatCheatCode@@YIXAAV?$basic_string@..` | inliner (predict-inline) | 88 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 89.51 | singleselectionwindow | `?HandleNetMsg@TSingleSelectionWindow@@QAEEPA..` | inliner (predict-inline) | 346 | 43 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 2 over-inline (15 name-unresolvable pair(s) discounted) |
| 89.51 | cursor | `?MoveHero@advManager@@QAEPAVNewmapCell@@HEPA..` | inliner (predict-inline) | 688 | 132 | callee expanded on one side only (A8/A9/A12): 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 89.53 | ai_player | `?end_turn@type_AI_player@@QAEXXZ` | inliner (predict-inline) | 102 | 3 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline (2 name-unresolvable pair(s) discounted) |
| 89.59 | town | `?get_buildable_mask@town@@QBE_JXZ` | register-homing (why-reg) | 24 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.63 | smackmgr | `?VideoDrawRects@@YIXXZ` | register-homing (why-reg) | 149 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.67 | events | `?DoEventShrine@advManager@@QAEXPAVhero@@PAVN..` | inliner (predict-inline) | 126 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (7 name-unresolvable pair(s) discounted) |
| 89.69 | ai | `?choose_melee_target@combatManager@@QAEEPBVa..` | inliner (predict-inline) | 337 | 28 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 89.72 | townmgr | `?WindowHandler@TTavernWindow@@UAEHPAVmessage..` | register-homing (why-reg) | 80 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.73 | tradpost | `?WindowHandler@TSellArtifactWindow@@UAEHPAVm..` | inliner (predict-inline) | 211 | 44 | callee expanded on one side only (A8/A9/A12): 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 89.77 | multiplayerwindow | `?OnSearch@TMultiPlayerWindow@@QAEEXZ` | inliner (predict-inline) | 181 | 12 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 4 over-inline (27 name-unresolvable pair(s) discounted) |
| 89.82 | findpath | `?SeedCombatPosition@searchArray@@QAEXPBVarmy..` | register-homing (why-reg) | 38 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.08 | advmgr | `?get_creature_bank_help_text@@YIXPADPAVNewma..` | control-flow (why-branch) | 50 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 90.12 | ai_player | `?trade_resources@type_AI_player@@QAEXPBHJ@Z` | inliner (predict-inline) | 53 | 1 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 90.16 | initialize | `?initialize_game_data@@YIXXZ` | register-homing (why-reg) | 139 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.16 | viewarmywindow | `??0TViewArmyWindow@@QAE@PBVarmy@@HHE@Z` | inliner (predict-inline) | 174 | 46 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (25 name-unresolvable pair(s) discounted) |
| 90.20 | hero | `?remove_artifact@hero@@QAEXJ@Z` | register-homing (why-reg) | 35 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.22 | singleselectionwindow | `?OnMapFileNameMsg@TSingleSelectionWindow@@QA..` | inliner (predict-inline) | 119 | 3 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (7 name-unresolvable pair(s) discounted) |
| 90.25 | philai | `?DoAI@philAI@@QAEXH@Z` | inliner (predict-inline) | 123 | 32 | callee expanded on one side only (A8/A9/A12): 2 over-inline (3 name-unresolvable pair(s) discounted) |
| 90.26 | findpath | `?PushCombatPoint@searchArray@@QAEXHHHHH@Z` | control-flow (why-branch) | 85 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 90.26 | tradpost | `?SetRolloverText@TSellCreatureWindow@@QAEXH@Z` | register-homing (why-reg) | 20 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.27 | spells | `?CastSpell@combatManager@@QAEXHHHHHJ@Z` | inliner (predict-inline) | 1682 | 248 | callee expanded on one side only (A8/A9/A12): 2 over-inline (18 name-unresolvable pair(s) discounted) |
| 90.30 | advmgr | `??0advManager@@QAE@XZ` | register-homing (why-reg) | 25 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.35 | dxplay | `?GetGroupName@CDPlay@@UAEEKPADH0H@Z` | register-homing (why-reg) | 42 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.35 | dxplay | `?GetPlayerName@CDPlay@@UAEEKPADH0H@Z` | register-homing (why-reg) | 42 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.43 | mapcell | `?StampObject@NewfullMap@@QAEXPAVNewmapCell@@..` | control-flow (why-branch) | 164 | 48 | loop-form / merged-return placement / case order (D1-D9) |
| 90.47 | viewarmywindow | `??0TViewArmyWindow@@QAE@PAVarmyGroup@@HPBVhe..` | inliner (predict-inline) | 147 | 32 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (21 name-unresolvable pair(s) discounted) |
| 90.49 | mapcell | `?NewfullMapFn_004FD950@NewfullMap@@QAEXPAVTA..` | register-homing (why-reg) | 99 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.56 | singleselectionwindow | `?DrawHeroAdvancedOption@TSingleSelectionWind..` | control-flow (why-branch) | 361 | 10 | loop-form / merged-return placement / case order (D1-D9) |
| 90.57 | singleselectionwindow | `??1CNewMapHeaderInfoMsg@@QAE@XZ` | inliner (predict-inline) | 17 | 0 | callee expanded on one side only (A8/A9/A12): 2 over-inline (1 name-unresolvable pair(s) discounted) |
| 90.57 | overview | `?SetupDynamicStuff@game@@QAEXHH@Z` | control-flow (why-branch) | 1475 | 67 | loop-form / merged-return placement / case order (D1-D9) |
| 90.58 | spells | `?Resurrect@combatManager@@QAEXPAVarmy@@JE@Z` | register-homing (why-reg) | 62 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.59 | adventuremapwindow | `?CheckAdvCheatCode@@YIXAAV?$basic_string@DU?..` | inliner (predict-inline) | 96 | 26 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline (1 name-unresolvable pair(s) discounted) |
| 90.65 | advmgr | `?UpdateRadar@advManager@@QAEXUtype_point@@EE..` | control-flow (why-branch) | 532 | 9 | loop-form / merged-return placement / case order (D1-D9) |
| 90.68 | ai_player | `?get_value@type_antiluck_artifact@@UBEJPBVhe..` | control-flow (why-branch) | 6 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 90.68 | ai_player | `?get_value@type_antimorale_artifact@@UBEJPBV..` | control-flow (why-branch) | 6 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 90.70 | hiscore | `??0THighScoreWindow@@QAE@XZ` | register-homing (why-reg) | 170 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.75 | townmgr | `?DoBlacksmith@@YIXHH@Z` | register-homing (why-reg) | 34 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.88 | command | `?SetCombatDirections@combatManager@@QAEXH@Z` | register-homing (why-reg) | 219 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.90 | town | `?give_event_reward@town@@QAEXPBVTTownEvent@@..` | inliner (predict-inline) | 128 | 5 | callee expanded on one side only (A8/A9/A12): 1 under-inline (2 name-unresolvable pair(s) discounted) |
| 90.93 | ai_combat | `?choose_melee@type_AI_combat_data@@QBE_NABV1..` | inliner (predict-inline) | 283 | 2 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (2 name-unresolvable pair(s) discounted) |
| 91.01 | ai_combat | `?initialize_creatures@type_AI_combat_data@@Q..` | register-homing (why-reg) | 334 | 2 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 91.08 | townmgr | `?BuyBuild@townManager@@QAEHHHH@Z` | inliner (predict-inline) | 703 | 61 | callee expanded on one side only (A8/A9/A12): 1 over-inline (21 name-unresolvable pair(s) discounted) |
| 91.10 | cmbtmgr | `?Unnamed464d40@combatManager@@QAEEPAVarmy@@@Z` | register-homing (why-reg) | 47 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.10 | advmgr | `?ProcessMapSelect@advManager@@QAEXPBVmessage..` | inliner (predict-inline) | 150 | 57 | callee expanded on one side only (A8/A9/A12): 1 under-inline |
| 91.10 | spells | `?GetNextChainLightningTarget@combatManager@@..` | register-homing (why-reg) | 58 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.11 | remote | `??1CNetMsgHandlerPause@@UAE@XZ` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.22 | singleselectionwindow | `?OnWidgetDeselect@TSingleSelectionWindow@@QA..` | inliner (predict-inline) | 635 | 200 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 8 over-inline (8 name-unresolvable pair(s) discounted) |
| 91.26 | ai_player | `?calculate_reserve@type_AI_player@@QAEXXZ` | register-homing (why-reg) | 78 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.30 | bottomviewsubwindow | `??0TBottomViewResourceMessage@@QAE@PAVheroWi..` | register-homing (why-reg) | 40 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.46 | army | `?attack_wall@army@@AAEXW4TWallTargetId@@J@Z` | inliner (predict-inline) | 155 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 91.54 | winmgr | `?DoDialogDraw@heroWindowManager@@QAEHPAVhero..` | register-homing (why-reg) | 56 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.56 | game | `?Read@NewSMapHeader@@QAEHPAVTAbstractFile@@H..` | inliner (predict-inline) | 322 | 50 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 5 over-inline (15 name-unresolvable pair(s) discounted) |
| 91.65 | hero | `?get_morale_description@hero@@QBE?AV?$basic_..` | inliner (predict-inline) | 104 | 76 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 1 over-inline (27 name-unresolvable pair(s) discounted) |
| 91.66 | swapmgr | `?Open@swapManager@@UAEHH@Z` | register-homing (why-reg) | 106 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.67 | singleselectionwindow | `?SortMaps@TSingleSelectionWindow@@QAEXHEE@Z` | register-homing (why-reg) | 193 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 91.71 | army | `?Turn@army@@QAEXE@Z` | register-homing (why-reg) | 16 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.74 | font | `?LineLength@font@@QAEHPBDH@Z` | register-homing (why-reg) | 33 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.89 | mapcell | `?Save@NewfullMap@@QAEHPAVTAbstractFile@@HE@Z` | inliner (predict-inline) | 116 | 63 | callee expanded on one side only (A8/A9/A12): 2 over-inline |
| 91.93 | events | `?DoEventShipyard@advManager@@QAEXPAVNewmapCe..` | register-homing (why-reg) | 77 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.94 | hillfortwindow | `?HillFortWindowHandler@@YIHAAVmessage@@@Z` | control-flow (why-branch) | 60 | 23 | loop-form / merged-return placement / case order (D1-D9) |
| 91.95 | ai_player | `?initialize_artifact_effects@@YIXXZ` | (diag error: no shared public text symbol in built objects) | - | - | - |
| 92.00 | soundmgr | `?WaitEndSampleThread@@YAXPAX@Z` | register-homing (why-reg) | 28 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.05 | townmgr | `?SetCommandAndText@townManager@@QAEXPAVmessa..` | control-flow (why-branch) | 449 | 66 | loop-form / merged-return placement / case order (D1-D9) |
| 92.20 | mapcell | `?Load@NewfullMap@@QAEHPAVTAbstractFile@@HEH@Z` | inliner (predict-inline) | 53 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (7 name-unresolvable pair(s) discounted) |
| 92.20 | philai | `?value_of_bank@@YIJPBVhero@@PAVNewmapCell@@@Z` | inliner (predict-inline) | 12 | 6 | callee expanded on one side only (A8/A9/A12): 1 under-inline |
| 92.23 | game | `?GetRandomMonster@game@@QAE?AW4TCreatureType..` | inliner (predict-inline) | 38 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 3 over-inline (5 name-unresolvable pair(s) discounted) |
| 92.36 | armygrp | `?get_luck_description@armyGroup@@QBE?AV?$bas..` | register-homing (why-reg) | 70 | 2 | spill to dead-parameter slot (B4) |
| 92.44 | hero | `?equip_artifact@hero@@QAEEPBUtype_artifact@@..` | inliner (predict-inline) | 18 | 7 | callee expanded on one side only (A8/A9/A12): 1 under-inline (1 name-unresolvable pair(s) discounted) |
| 92.51 | ai_player | `?get_swap_value@type_AI_creature_swapper@@QA..` | register-homing (why-reg) | 16 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.51 | game | `?Load@NewSMapHeader@@QAEHPAVTAbstractFile@@H..` | inliner (predict-inline) | 275 | 9 | callee expanded on one side only (A8/A9/A12): 2 under-inline (5 name-unresolvable pair(s) discounted) |
| 92.52 | army | `?get_berserk_targets@army@@QBEXAAV?$vector@P..` | register-homing (why-reg) | 53 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.56 | town | `?BuildBuilding@town@@QAE?AW4type_building_id..` | inliner (predict-inline) | 85 | 0 | callee expanded on one side only (A8/A9/A12): 2 over-inline (1 name-unresolvable pair(s) discounted) |
| 92.57 | quickherowindow | `??0TQuickHeroWindow@@QAE@PAVhero@@W4TViewLev..` | inliner (predict-inline) | 253 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (31 name-unresolvable pair(s) discounted) |
| 92.57 | command | `?GetCommand@combatManager@@QAEHH@Z` | register-homing (why-reg) | 104 | 1 | spill to dead-parameter slot (B4) |
| 92.63 | army | `?InitClean@army@@QAEXXZ` | register-homing (why-reg) | 62 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.68 | advmgr | `?BVMessage@advManager@@QAEXPBD@Z` | inliner (predict-inline) | 13 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 92.70 | advmgr | `?DrawUnderlay@advManager@@QAEXHHHHH@Z` | register-homing (why-reg) | 223 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.76 | ai_player | `?AI_get_value_of_artifact@@YIJUtype_artifact..` | register-homing (why-reg) | 132 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.88 | window | `?CenterWindow@heroWindow@@QAEXHH@Z` | register-homing (why-reg) | 79 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.92 | hero | `?HeroFn_004E2550@hero@@QAEEJJ@Z` | control-flow (why-branch) | 65 | 7 | loop-form / merged-return placement / case order (D1-D9) |
| 92.94 | advmgr | `?TownQuickView@advManager@@QAEXHHHE@Z` | inliner (predict-inline) | 141 | 0 | callee expanded on one side only (A8/A9/A12): 2 over-inline (17 name-unresolvable pair(s) discounted) |
| 93.01 | mapcell | `?readBlackBox@NewfullMap@@QAEHPAVTAbstractFi..` | control-flow (why-branch) | 304 | 28 | loop-form / merged-return placement / case order (D1-D9) |
| 93.06 | armygrp | `?get_morale_description@armyGroup@@QBE?AV?$b..` | inliner (predict-inline) | 362 | 93 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 4 over-inline (20 name-unresolvable pair(s) discounted) |
| 93.10 | command | `?ProcessCombatMsg@combatManager@@QAEHAAVmess..` | inliner (predict-inline) | 293 | 256 | callee expanded on one side only (A8/A9/A12): 3 under-inline (29 name-unresolvable pair(s) discounted) |
| 93.17 | army | `?animate_missile@army@@AAEXPAV1@@Z` | register-homing (why-reg) | 208 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.17 | mainmenu | `?MainMenuHandler@@YIHAAVmessage@@@Z` | inliner (predict-inline) | 95 | 9 | callee expanded on one side only (A8/A9/A12): 1 under-inline (6 name-unresolvable pair(s) discounted) |
| 93.18 | strip | `?DrawOwner@strip@@IAEXH@Z` | unclassified | 15 | 0 | run why-reg / why-branch for the full search |
| 93.18 | ai_tactical | `??0type_AI_spellcaster@@QAE@PAVcombatManager..` | register-homing (why-reg) | 67 | 0 | spill to dead-parameter slot (B4) |
| 93.21 | advmgr | `?get_army_help_text@@YI?AV?$basic_string@DU?..` | inliner (predict-inline) | 29 | 44 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (10 name-unresolvable pair(s) discounted) |
| 93.22 | cmbtmgr | `?RaiseSkeletons@combatManager@@QAEXH@Z` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 93.32 | army | `?WalkTo@army@@QAEEHE@Z` | register-homing (why-reg) | 150 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.32 | mapcell | `?readSpellScrollData@NewfullMap@@QAEHPAVTAbs..` | register-homing (why-reg) | 60 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.33 | advmgr | `?BVResMsg@advManager@@QAEXPBDHH@Z` | inliner (predict-inline) | 13 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 93.35 | combatwindow | `?SendChat@CCombatChatEdit@@UAEXPBDH@Z` | inliner (predict-inline) | 8 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 93.42 | swapmgr | `?handle_artifact_click@swapManager@@QAEXJJE@Z` | register-homing (why-reg) | 22 | 6 | register-homing knob (B-family) |
| 93.51 | cmbtmgr | `?ShootAnimatedMissile@combatManager@@QAEXHHH..` | register-homing (why-reg) | 97 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.63 | recruit | `?add_creature_widgets@TRecruitWindow@@QAEXJJ..` | inliner (predict-inline) | 26 | 20 | callee expanded on one side only (A8/A9/A12): 1 over-inline (13 name-unresolvable pair(s) discounted) |
| 93.68 | singleselectionwindow | `?CanChooseHero@TSingleSelectionWindow@@QAEEH..` | inliner (predict-inline) | 23 | 8 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 93.70 | townmgr | `?DoUniversity@townManager@@QAEXXZ` | register-homing (why-reg) | 31 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.71 | hero | `?get_luck_description@hero@@QBE?AV?$basic_st..` | inliner (predict-inline) | 84 | 66 | callee expanded on one side only (A8/A9/A12): 2 under-inline (25 name-unresolvable pair(s) discounted) |
| 93.81 | advmgr | `?SetTownContext@advManager@@QAEXHEE@Z` | register-homing (why-reg) | 52 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.87 | initialize | `?create_included_mask@@YIXPBHPA_J@Z` | (diag error: no shared public text symbol in built objects) | - | - | - |
| 93.88 | townmgr | `??0type_monster_join_window@@QAE@PAVhero@@PA..` | control-flow (why-branch) | 43 | 24 | loop-form / merged-return placement / case order (D1-D9) |
| 93.88 | townmgr | `??0type_garrison_base_window@@QAE@PAVhero@@H..` | inliner (predict-inline) | 289 | 175 | callee expanded on one side only (A8/A9/A12): 15 over-inline (193 name-unresolvable pair(s) discounted) |
| 93.90 | spells | `?Armageddon@combatManager@@QAEXHH@Z` | inliner (predict-inline) | 255 | 1 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 93.93 | singleselectionpopups | `?CreateWin@CHeroDlg@@QAEEPAVBitmap816@@PBDPA..` | register-homing (why-reg) | 174 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.96 | hero | `?mark_spells@@YI?AV?$bitset@$0EG@@std@@H@Z` | inliner (predict-inline) | 65 | 28 | callee expanded on one side only (A8/A9/A12): 6 under-inline, 7 over-inline |
| 93.96 | advmgr | `?SetRolloverText@advManager@@QAEXPAVNewmapCe..` | inliner (predict-inline) | 705 | 345 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 93.98 | cmbtmgr | `?ShootMissile@combatManager@@QAEXHHHHPBMPBVC..` | register-homing (why-reg) | 76 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.01 | bottomviewsubwindow | `??0TBottomViewTown@@QAE@PAVheroWindow@@@Z` | inliner (predict-inline) | 200 | 54 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 5 over-inline (35 name-unresolvable pair(s) discounted) |
| 94.16 | recruit | `?Update@recruitUnit@@QAEXEJ@Z` | register-homing (why-reg) | 107 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.17 | mousemgr | `?Update@mouseManager@@QAEXE@Z` | register-homing (why-reg) | 375 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 94.17 | hero | `?HeroFn_004D9CC0@hero@@QAEHH@Z` | register-homing (why-reg) | 16 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.18 | townmgr | `?SetRolloverText@TThievesGuildWindow@@QAEXH@Z` | register-homing (why-reg) | 136 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.28 | singleselectionpopups | `?Main@CHotspotWidget@@UAEHPAVmessage@@@Z` | control-flow (why-branch) | 29 | 22 | loop-form / merged-return placement / case order (D1-D9) |
| 94.40 | systemoptionswindow | `?WindowHandler@TSystemOptionsWindow@@UAEHPAV..` | control-flow (why-branch) | 93 | 46 | loop-form / merged-return placement / case order (D1-D9) |
| 94.41 | game | `?calculate_production@game@@QAEXXZ` | control-flow (why-branch) | 111 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 94.43 | command | `?CheckGetAIMove@combatManager@@QAEXXZ` | register-homing (why-reg) | 110 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 94.47 | army | `?range_attack@army@@QAEXPAV1@@Z` | inliner (predict-inline) | 138 | 1 | callee expanded on one side only (A8/A9/A12): 1 under-inline |
| 94.51 | advmgr | `?GetSoundId@advManager@@QAE?AW4e_looping_sou..` | control-flow (why-branch) | 468 | 30 | loop-form / merged-return placement / case order (D1-D9) |
| 94.52 | ai | `?SOD_choose_faerie_dragon_spell@combatManage..` | register-homing (why-reg) | 28 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.54 | ai | `?mark_multiheaded_enemy@combatManager@@QAEXP..` | inliner (predict-inline) | 64 | 42 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 94.58 | event_record | `?replay@type_record_move_hero@@UAEXE@Z` | control-flow (why-branch) | 4 | 6 | loop-form / merged-return placement / case order (D1-D9) |
| 94.61 | tradpost | `?DoMarket@@YIXXZ` | control-flow (why-branch) | 58 | 29 | loop-form / merged-return placement / case order (D1-D9) |
| 94.67 | hero | `?HeroFn_004D8B30@hero@@QAEXPBVHeroExtra@@@Z` | register-homing (why-reg) | 54 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.68 | advmgr | `?ProcessSearch@advManager@@QAEHHHH@Z` | register-homing (why-reg) | 88 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 94.73 | game | `?GetRandomArtifactId@game@@QAE?AW4TArtifact@..` | register-homing (why-reg) | 27 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.79 | ai_combat | `?do_general_melee@type_AI_combat_data@@QAEXA..` | inliner (predict-inline) | 35 | 20 | callee expanded on one side only (A8/A9/A12): 1 under-inline |
| 94.80 | hero | `?GetMorale@hero@@QAEHPBV1@EE@Z` | control-flow (why-branch) | 30 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 94.87 | advmgr | `?QuickInfo@advManager@@QAEXHHH@Z` | control-flow (why-branch) | 715 | 392 | loop-form / merged-return placement / case order (D1-D9) |
| 94.95 | border | `?Main@border@@UAEHPAVmessage@@@Z` | control-flow (why-branch) | 31 | 26 | loop-form / merged-return placement / case order (D1-D9) |
| 95.00 | mapcell | `?loadBlackBox@NewfullMap@@QAEHPAVTAbstractFi..` | control-flow (why-branch) | 84 | 9 | loop-form / merged-return placement / case order (D1-D9) |
| 95.03 | cmbtmgr | `?KeepAttack@combatManager@@QAEXH@Z` | register-homing (why-reg) | 64 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.05 | resourcedisplay | `??0TResourceDisplay@@QAE@PAVheroWindow@@E@Z` | register-homing (why-reg) | 61 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.08 | singleselectionwindow | `?UpdateFilterWidgets@TSingleSelectionWindow@..` | control-flow (why-branch) | 73 | 56 | loop-form / merged-return placement / case order (D1-D9) |
| 95.12 | kb | `?GetNextHumanPlayer@@YIHH@Z` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.16 | mapcell | `?readTownData@NewfullMap@@QAEHPAVTAbstractFi..` | control-flow (why-branch) | 457 | 26 | loop-form / merged-return placement / case order (D1-D9) |
| 95.28 | ai_tactical | `?set_melee_enemies@type_AI_spellcaster@@QAEX..` | register-homing (why-reg) | 52 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.47 | mapcell | `?readMapLayer@NewfullMap@@QAEHPAVTAbstractFi..` | register-homing (why-reg) | 46 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.51 | singleselectionpopups | `?CreateWin@CBonusDlg@@QAEEPBDPAVBitmap816@@0..` | register-homing (why-reg) | 112 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.52 | ai_player | `?value_of_castle_upgrade@@YIHPAVtown@@PAH@Z` | unclassified | 14 | 0 | run why-reg / why-branch for the full search |
| 95.52 | singleselectionwindow | `?DrawBasicMapInfo@TSingleSelectionWindow@@QA..` | register-homing (why-reg) | 44 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.55 | ai_tactical | `?consider_teleport@type_AI_spellcaster@@QAEX..` | control-flow (why-branch) | 25 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 95.66 | townmgr | `?WindowHandler@type_garrison_base_window@@UA..` | register-homing (why-reg) | 106 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 95.68 | cmbtmgr | `?InitNonVisualVars@combatManager@@QAEXXZ` | register-homing (why-reg) | 67 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 95.69 | townmgr | `?SetCommandAndText@type_garrison_base_window..` | register-homing (why-reg) | 62 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.69 | resourcemanager | `?LoadPalette@ResourceManager@@YIPAVTPalette1..` | inliner (predict-inline) | 75 | 17 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (6 name-unresolvable pair(s) discounted) |
| 95.70 | mapcell | `?readScholarData@NewfullMap@@QAEHPAVTAbstrac..` | register-homing (why-reg) | 43 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.70 | game | `?readMapPlayerSlot@TPlayerSlotAttributes@CMa..` | inliner (predict-inline) | 128 | 1 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (6 name-unresolvable pair(s) discounted) |
| 95.70 | iconwdgt | `?Main@iconWidget@@UAEHPAVmessage@@@Z` | control-flow (why-branch) | 46 | 30 | loop-form / merged-return placement / case order (D1-D9) |
| 95.74 | path | `?ValidPath@army@@QAEEHE@Z` | register-homing (why-reg) | 4 | 0 | spill to dead-parameter slot (B4) |
| 95.78 | scenarioinfo | `??0CScenarioInfoDlg@@QAE@XZ` | inliner (predict-inline) | 1223 | 119 | callee expanded on one side only (A8/A9/A12): 1 over-inline (216 name-unresolvable pair(s) discounted) |
| 95.88 | mapcell | `??1NewfullMap@@QAE@XZ` | inliner (predict-inline) | 16 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (8 name-unresolvable pair(s) discounted) |
| 95.89 | ai | `?choose_creature_spell@combatManager@@QAEEPB..` | register-homing (why-reg) | 21 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.90 | cspriteframe | `?DrawCreatureImpl@CSpriteFrame@@QBEXHHHHPAGH..` | control-flow (why-branch) | 224 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 95.92 | smackmgr | `?VideoClose@@YIXXZ` | control-flow (why-branch) | 2 | 20 | loop-form / merged-return placement / case order (D1-D9) |
| 95.93 | misc | `?ReadPrefsFromRegistry@@YIXXZ` | inliner (predict-inline) | 151 | 2 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 96.06 | ai_tactical | `?get_fortune_value@type_AI_spellcaster@@QAEJ..` | register-homing (why-reg) | 166 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 96.14 | army | `?DrawToBuffer@army@@QAEXHHH@Z` | register-homing (why-reg) | 180 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 96.15 | drawing | `?DrawFrame@combatManager@@QAEXEEEHEE@Z` | register-homing (why-reg) | 155 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 96.15 | advmgr | `?ProcessWaitingHover@advManager@@QAEHHH@Z` | register-homing (why-reg) | 23 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.19 | spells | `?ChainLightning@combatManager@@QAEXHHH@Z` | control-flow (why-branch) | 39 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 96.19 | ai_player | `?purchase_building@type_AI_player@@QAEEPAE@Z` | control-flow (why-branch) | 270 | 48 | loop-form / merged-return placement / case order (D1-D9) |
| 96.22 | cmbtmgr | `?PowEffect@combatManager@@QAEXHH@Z` | register-homing (why-reg) | 207 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.23 | command | `?Main@combatManager@@UAEHAAVmessage@@@Z` | control-flow (why-branch) | 49 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 96.26 | mapcell | `?upgrade_cell_extra_info@@YIXPAVNewmapCell@@..` | register-homing (why-reg) | 158 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.32 | singleselectionwindow | `?OnNameClick@TSingleSelectionWindow@@QAEXH@Z` | register-homing (why-reg) | 11 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.34 | systemoptionswindow | `??0TSystemOptionsWindow@@QAE@XZ` | inliner (predict-inline) | 502 | 158 | callee expanded on one side only (A8/A9/A12): 5 under-inline (133 name-unresolvable pair(s) discounted) |
| 96.35 | ai_tactical | `?get_hypnotize_value@type_AI_spellcaster@@QA..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.35 | ai | `?get_attack_change@combatManager@@QAEJPBVarm..` | register-homing (why-reg) | 50 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.36 | ai_player | `?find_all_destinations@@YIJPAVhero@@PAVsearc..` | inliner (predict-inline) | 91 | 3 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 3 over-inline (10 name-unresolvable pair(s) discounted) |
| 96.37 | spells | `?ShowMassSpell@combatManager@@QAEXPAY0BE@$$C..` | register-homing (why-reg) | 74 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.40 | combatcontrolsubwindow | `??0TCombatControlSubWindow@@QAE@PAVheroWindo..` | register-homing (why-reg) | 12 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.43 | townmgr | `?Recruit@TCastleWindow@@QAEXH@Z` | register-homing (why-reg) | 36 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.44 | tradpost | `?WindowHandler@TBuyArtifactWindow@@UAEHPAVme..` | register-homing (why-reg) | 64 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.46 | overview | `?SetupNewOverviewType@game@@QAEXHE@Z` | unclassified | 12 | 0 | run why-reg / why-branch for the full search |
| 96.47 | seerhut | `?GetAIValue@type_artifact_quest@@UAEHH@Z` | inliner (predict-inline) | 2 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 96.47 | mapcell | `?readMonsterData@NewfullMap@@QAEHPAVTAbstrac..` | inliner (predict-inline) | 122 | 1 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 96.48 | townmgr | `?DoTavern@@YIEXZ` | control-flow (why-branch) | 29 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 96.52 | philai | `?value_of_war_factory@@YIJPBVhero@@W4TArtifa..` | (diag error: no shared public text symbol in built objects) | - | - | - |
| 96.53 | hero | `?DestroySiegeWeaponArtifact@hero@@QAEXH@Z` | register-homing (why-reg) | 17 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 96.53 | seerhut | `?SetDefaultText@type_skill_quest@@UAEXXZ` | register-homing (why-reg) | 21 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.55 | questlogwindow | `?DoQuestLog@@YIXH@Z` | register-homing (why-reg) | 36 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.56 | armygrp | `?GetArmyMorale@armyGroup@@QBEHHPBVhero@@PBVt..` | register-homing (why-reg) | 75 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.57 | overview | `?UpdateBackpack@@YIXH@Z` | register-homing (why-reg) | 5 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.59 | army | `?get_unit_combat_value@army@@QBENJJEPBV1@@Z` | register-homing (why-reg) | 40 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.60 | philai | `?GetTurnAIVars@philAI@@QAEXH@Z` | register-homing (why-reg) | 4 | 0 | register-homing knob (B-family) |
| 96.62 | sacrifice_window | `?sacrifice@type_sacrifice_window@@CIHAAVmess..` | control-flow (why-branch) | 6 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 96.63 | remote | `?KillOldChat@CChatManager@@QAEXXZ` | register-homing (why-reg) | 28 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.63 | remote | `?AddChat@@YAXPAVCChatManager@@PBDZZ` | register-homing (why-reg) | 9 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.64 | advmgr | `?garrison_quick_view@advManager@@QAEXHHH@Z` | control-flow (why-branch) | 8 | 8 | loop-form / merged-return placement / case order (D1-D9) |
| 96.67 | recruit | `??0recruitUnit@@QAE@PAVhero@@W4TCreatureType..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.72 | hero | `?UpdateArmies@hero@@QAEXXZ` | register-homing (why-reg) | 6 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.74 | hero | `?HeroFn_004DBF30@hero@@QAEEHJ@Z` | register-homing (why-reg) | 28 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 96.75 | mapcell | `?GenerateHeightMap@NewfullMap@@QAEXPBVCObjec..` | register-homing (why-reg) | 30 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 96.77 | townmgr | `??0TShipWindow@@QAE@H@Z` | control-flow (why-branch) | 29 | 18 | loop-form / merged-return placement / case order (D1-D9) |
| 96.77 | resourcemanager | `?GetBitmapResourceSize@ResourceManager@@YIHP..` | unclassified | 7 | 0 | run why-reg / why-branch for the full search |
| 96.78 | ai_player | `?get_value@type_undead_king_cloak_artifact@@..` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.80 | spellbookwindow | `?get_spell_description@TSpellbookWindow@@AAE..` | inliner (predict-inline) | 23 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (9 name-unresolvable pair(s) discounted) |
| 96.81 | font | `?DrawBoundedString@font@@QAEXPBDPAVBitmap16B..` | register-homing (why-reg) | 71 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.86 | ai_tactical | `?get_muck_and_mire_value@type_AI_spellcaster..` | register-homing (why-reg) | 43 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.95 | adventuremapwindow | `?SetElevationToggleImage@TAdventureMapWindow..` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.98 | army | `?SetSpellInfluence@army@@QAEXHHHPBVhero@@@Z` | inliner (predict-inline) | 235 | 1 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (13 name-unresolvable pair(s) discounted) |
| 97.01 | philai | `?buy_artifacts@@YIXPAVhero@@PAW4TArtifact@@J..` | inliner (predict-inline) | 4 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 97.06 | singleselectionwindow | `?OnNewPlayerMsg@TSingleSelectionWindow@@QAEE..` | inliner (predict-inline) | 19 | 5 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 97.07 | philai | `?get_artifact_purchase_value@@YIJW4TArtifact..` | inliner (predict-inline) | 2 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 97.09 | remote | `?CheckForWarning@CTurnDuration@@QAEXXZ` | register-homing (why-reg) | 38 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.10 | ai_tactical | `?consider_resurrect@type_AI_spellcaster@@QAE..` | register-homing (why-reg) | 45 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.16 | puzzlewindow | `?AI_attempt_puzzle_guess@@YI?AUtype_point@@J..` | register-homing (why-reg) | 130 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 97.27 | ai_player | `?do_best_purchase@type_AI_creature_purchaser..` | register-homing (why-reg) | 70 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.32 | spells | `?ResetBoltAngle@combatManager@@QAEXPAUSBolt@..` | unclassified | 22 | 0 | run why-reg / why-branch for the full search |
| 97.33 | ai_player | `?get_purchase_value@type_AI_creature_purchas..` | register-homing (why-reg) | 12 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.34 | advmgr | `?AdvmgrFn_0040D670@@YIXPADPAVNewmapCell@@JPB..` | register-homing (why-reg) | 12 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.36 | army | `?CancelIndividualSpell@army@@QAEXH@Z` | inliner (predict-inline) | 54 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (7 name-unresolvable pair(s) discounted) |
| 97.37 | cmbtmgr | `?MakeCreaturesVanish@combatManager@@QAEXXZ` | register-homing (why-reg) | 26 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.39 | townmgr | `?WindowHandler@TMageGuildWindow@@UAEHPAVmess..` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.40 | mapcell | `?readMapObjects@NewfullMap@@QAEHPAVTAbstract..` | inliner (predict-inline) | 136 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (28 name-unresolvable pair(s) discounted) |
| 97.43 | ai_player | `?calculate_demand@type_AI_player@@QAEXXZ` | register-homing (why-reg) | 257 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 97.44 | mapcell | `?readObject@NewfullMap@@QAEHPAVTAbstractFile..` | inliner (predict-inline) | 279 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 97.45 | events | `?DoCustomSpellScroll@advManager@@QAEXPAVhero..` | register-homing (why-reg) | 12 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.45 | hero | `?HeroFn_004DC070@hero@@QAEXJ@Z` | register-homing (why-reg) | 2 | 0 | register-homing knob (B-family) |
| 97.47 | rmg | `?InitializeObjectGenerators@type_random_map_..` | inliner (predict-inline) | 729 | 0 | callee expanded on one side only (A8/A9/A12): 29 under-inline, 28 over-inline (226 name-unresolvable pair(s) discounted) |
| 97.48 | command | `?ResetRound@combatManager@@QAEXXZ` | inliner (predict-inline) | 26 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline |
| 97.49 | multiplayerwindow | `?OnWidgetDeselect@TMultiPlayerWindow@@UAEHHP..` | register-homing (why-reg) | 165 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.51 | game | `?ConvertObject@game@@QAEXPAVNewmapCell@@@Z` | inliner (predict-inline) | 74 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 97.54 | singleselectionwindow | `?OnNewHostMsg@TSingleSelectionWindow@@QAEXPA..` | register-homing (why-reg) | 4 | 0 | register-homing knob (B-family) |
| 97.60 | tradpost | `?ComputeTradeRatios@TSellCreatureWindow@@QAE..` | register-homing (why-reg) | 20 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.63 | resourcemanager | `?GetBitmap816@ResourceManager@@YIPAVBitmap81..` | register-homing (why-reg) | 145 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 97.65 | resourcemanager | `?LoadFontData@ResourceManager@@YIPAVfont@@PB..` | control-flow (why-branch) | 16 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 97.69 | singleselectionwindow | `?WindowHandler@TSingleSelectionWindow@@UAEHP..` | inliner (predict-inline) | 41 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 97.72 | cursor | `?animate_move@advManager@@QAEXPAVhero@@HHH@Z` | register-homing (why-reg) | 31 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 97.73 | cmbtmgr | `?CheckApplyBadMorale@combatManager@@QAEHHH@Z` | control-flow (why-branch) | 26 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 97.74 | town | `?initialize_spells@town@@QAEXPBVTownExtra@@@Z` | register-homing (why-reg) | 56 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.74 | philai | `?AI_value_of_event@@YIJPBVhero@@Utype_point@..` | inliner (predict-inline) | 400 | 5 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (5 name-unresolvable pair(s) discounted) |
| 97.75 | resourcemanager | `?RemapGraphics@ResourceManager@@YIXXZ` | inliner (predict-inline) | 44 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 97.77 | bottomviewsubwindow | `??0TBottomViewHero@@QAE@PAVheroWindow@@@Z` | register-homing (why-reg) | 86 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.77 | townmgr | `?SetRolloverText@TCastleWindow@@QAEXPAVmessa..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.77 | town | `?SwapHeroes@town@@QAEXXZ` | register-homing (why-reg) | 16 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.79 | ai_player | `?get_value@type_school_artifact@@UBEJPBVhero..` | unclassified | 9 | 0 | run why-reg / why-branch for the full search |
| 97.80 | advmgr | `?Open@advManager@@UAEHH@Z` | inliner (predict-inline) | 76 | 2 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (17 name-unresolvable pair(s) discounted) |
| 97.82 | advmgr | `?ScreenScroll@advManager@@QAEXHH@Z` | register-homing (why-reg) | 47 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 97.87 | resourcemanager | `?SaturateGraphics@ResourceManager@@YIXXZ` | inliner (predict-inline) | 45 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 97.97 | recruit | `??0recruitUnit@@QAE@PAVarmyGroup@@EW4TCreatu..` | register-homing (why-reg) | 24 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.00 | singleselectionpopups | `?CreateWin@CBonusDlg@@QAEEPBDPAVCSprite@@H00..` | register-homing (why-reg) | 94 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.02 | townmgr | `?WindowHandler@TBuyBuildWindow@@UAEHPAVmessa..` | unclassified | 3 | 0 | run why-reg / why-branch for the full search |
| 98.05 | command | `?automate_first_aid_tent@combatManager@@QAEE..` | register-homing (why-reg) | 20 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.09 | townmgr | `?SetupTown@townManager@@QAEXE@Z` | register-homing (why-reg) | 20 | 0 | register-homing knob (B-family) |
| 98.10 | hero | `?GetExperienceIncrement@hero@@SIHH@Z` | register-homing (why-reg) | 23 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.10 | event_record | `?play_recorded_events@game@@QAEXXZ` | register-homing (why-reg) | 67 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.14 | cmbtmgr | `?SetupAndLoadObstacles@combatManager@@QAEXXZ` | register-homing (why-reg) | 34 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.17 | advmgr | `?DrawHeroPart@advManager@@QAEXHAAUTDrawParts..` | unclassified | 15 | 0 | run why-reg / why-branch for the full search |
| 98.18 | advmgr | `?DrawHeroPartShadow@advManager@@QAEXHAAUTDra..` | unclassified | 15 | 0 | run why-reg / why-branch for the full search |
| 98.19 | ai_tactical | `?consider_spell@type_AI_spellcaster@@QAEXPAU..` | register-homing (why-reg) | 72 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.21 | victorylossconditions | `?CheckForArtifactWin@VictoryConditionStruct@..` | inliner (predict-inline) | 15 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (13 name-unresolvable pair(s) discounted) |
| 98.24 | events | `?DoCustomArtifact@advManager@@QAEXPAVhero@@P..` | register-homing (why-reg) | 34 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.32 | remote | `?HandleMPlayerLaunch@@YIEXZ` | unclassified | 20 | 0 | run why-reg / why-branch for the full search |
| 98.34 | seerhut | `?SetDefaultText@type_monster_quest@@UAEXXZ` | inliner (predict-inline) | 25 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (10 name-unresolvable pair(s) discounted) |
| 98.35 | game | `?match_underground_gates@game@@QAEXXZ` | register-homing (why-reg) | 18 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.35 | sacrifice_window | `??0type_skeleton_window@@QAE@PAVarmyGroup@@@Z` | inliner (predict-inline) | 48 | 24 | callee expanded on one side only (A8/A9/A12): 1 under-inline (66 name-unresolvable pair(s) discounted) |
| 98.38 | hero | `?GetHighestSchool@hero@@QBE?AW4TSpellSchool@..` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 98.47 | ai | `?DoCompAI@combatManager@@QAEXH@Z` | unclassified | 1 | 0 | run why-reg / why-branch for the full search |
| 98.47 | campaignwindow | `??0TCampaignWindow@@QAE@EH@Z` | inliner (predict-inline) | 189 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (25 name-unresolvable pair(s) discounted) |
| 98.49 | textwdgt | `?Main@textWidget@@UAEHPAVmessage@@@Z` | register-homing (why-reg) | 68 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.50 | misc | `?CheckConfigFile@@YIXXZ` | register-homing (why-reg) | 47 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.52 | bottomviewsubwindow | `??0TBottomViewKingdom@@QAE@PAVheroWindow@@@Z` | register-homing (why-reg) | 24 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.52 | army | `?get_estimated_damage@army@@QBEJPBV1@JEJ@Z` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.53 | command | `?ProcessNextAction@combatManager@@QAEHAAVmes..` | inliner (predict-inline) | 52 | 107 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (12 name-unresolvable pair(s) discounted) |
| 98.57 | armygrp | `?GetMorale@armyGroup@@QBEHPBVhero@@PBVtown@@..` | inliner (predict-inline) | 13 | 0 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline (7 name-unresolvable pair(s) discounted) |
| 98.57 | events | `?DoCombat@advManager@@QAEHUtype_point@@PAVhe..` | control-flow (why-branch) | 75 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 98.58 | townmgr | `?SetupWell@townManager@@QAEXPAVTCastleWindow..` | register-homing (why-reg) | 48 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.61 | townmgr | `?DoTownGate@townManager@@QAEXXZ` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.61 | game | `?CreateTownHeroes@game@@QAEXPAH@Z` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.62 | army | `?LoadResources@army@@QAEXXZ` | inliner (predict-inline) | 117 | 0 | callee expanded on one side only (A8/A9/A12): 8 under-inline, 8 over-inline |
| 98.68 | hero | `?IsInIdentifyRange@hero@@QAEEPBUtype_point@@..` | register-homing (why-reg) | 2 | 0 | register-homing knob (B-family) |
| 98.71 | remote | `?OnPlayerDropUpdateMsg@@YIXK@Z` | inliner (predict-inline) | 17 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 98.72 | philai | `?buy_siege_engine@@YIXPAVhero@@PAVtown@@W4ty..` | register-homing (why-reg) | 4 | 0 | register-homing knob (B-family) |
| 98.75 | tradpost | `?Update@TSellCreatureWindow@@QAEX_N@Z` | register-homing (why-reg) | 37 | 0 | register-homing knob (B-family) |
| 98.76 | spells | `?ShowSpellMessage@combatManager@@QAEXHHPAVar..` | register-homing (why-reg) | 68 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.80 | singleselectionwindow | `??0CGameHeaderInfoMsg@@QAE@EHPAUGameSelectio..` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 98.81 | hero | `?get_skill_award@@YI?AW4TSecondarySkill@@PBV..` | control-flow (why-branch) | 8 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 98.84 | townmgr | `?BuildObj@townManager@@QAEXH@Z` | register-homing (why-reg) | 13 | 1 | register-homing knob (B-family) |
| 98.84 | quicktownwindow | `??0TQuickTownWindow@@QAE@PBVtown@@W4TViewLev..` | register-homing (why-reg) | 16 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 98.87 | game | `?LoadMinePool@game@@QAEHPAVTAbstractFile@@H@Z` | unclassified | 5 | 0 | run why-reg / why-branch for the full search |
| 98.88 | game | `??0SavedGameHeader@@QAE@XZ` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 98.89 | army | `?do_attack@army@@QAEEPAV1@H@Z` | register-homing (why-reg) | 20 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.89 | game | `?PerWeek@game@@QAEXXZ` | register-homing (why-reg) | 93 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.93 | recruit | `?Main@recruitUnit@@UAEHAAVmessage@@@Z` | inliner (predict-inline) | 115 | 0 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline (3 name-unresolvable pair(s) discounted) |
| 98.95 | townmgr | `??0TTownScreenWindow@@QAE@XZ` | inliner (predict-inline) | 259 | 1 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 6 over-inline (248 name-unresolvable pair(s) discounted) |
| 98.96 | game | `?SetupPuzzlePieces@game@@QAEHHH@Z` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 98.98 | singleselectionwindow | `?Update@TSingleSelectionWindow@@QAEHXZ` | register-homing (why-reg) | 60 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.98 | game | `?GetNewHeroId@game@@QAEHHW4THeroClass@@E0@Z` | register-homing (why-reg) | 6 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.98 | ai_tactical | `?get_frenzy_value@type_AI_spellcaster@@QAEJP..` | unclassified | 6 | 0 | run why-reg / why-branch for the full search |
| 99.01 | recruit | `??0TRecruitWindow@@QAE@HHHPAVrecruitUnit@@@Z` | register-homing (why-reg) | 115 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.06 | town | `?show_creature_rewards@@YIXPBVtown@@PAV?$vec..` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 99.09 | events | `?DoEventCreatureGenerator@advManager@@QAEXPA..` | control-flow (why-branch) | 11 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 99.10 | ai | `?choose_defense_hex@combatManager@@QAEEPBVar..` | register-homing (why-reg) | 30 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.11 | ai_tactical | `?get_simple_attack_effect@type_AI_combat_par..` | register-homing (why-reg) | 32 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.11 | singleselectionpopups | `?CreateWin@CTeamAlignmentDlg@@QAEEXZ` | register-homing (why-reg) | 15 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.13 | combatcontrolsubwindow | `??0TCombatCreatureSubWindow@@QAE@HHHHPAVhero..` | inliner (predict-inline) | 34 | 50 | callee expanded on one side only (A8/A9/A12): 1 under-inline (86 name-unresolvable pair(s) discounted) |
| 99.13 | levelupwindow | `??0TLevelUpWindow@@QAE@PAVhero@@HHH@Z` | inliner (predict-inline) | 61 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (84 name-unresolvable pair(s) discounted) |
| 99.17 | hero | `?HeroFn_004D9B30@hero@@QAEHH@Z` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 99.18 | philai | `?AI_enter_town@@YIXPAVhero@@PAVtown@@@Z` | register-homing (why-reg) | 43 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.20 | findpath | `?TestPossibleDirections@searchArray@@QAEXPAV..` | register-homing (why-reg) | 46 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.21 | multiplayerwindow | `?Update@TMultiPlayerWindow@@QAEXXZ` | register-homing (why-reg) | 40 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.22 | game | `?ReceiveSaveGame@game@@QAEHHHHEE@Z` | inliner (predict-inline) | 50 | 1 | callee expanded on one side only (A8/A9/A12): 2 over-inline (38 name-unresolvable pair(s) discounted) |
| 99.24 | philai | `?buy_special_building@@YIXPAVhero@@PAVtown@@..` | inliner (predict-inline) | 2 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 99.27 | advmgr | `?SetHeroContext@advManager@@QAEXHHEE@Z` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.28 | victorylossconditions | `?CheckForGrailBuildingWin@VictoryConditionSt..` | register-homing (why-reg) | 9 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.30 | events | `?CombatMonsterEvent@advManager@@QAEHPAVhero@..` | register-homing (why-reg) | 64 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.34 | dxplay | `?CreateSerialConnection@CDPlayLobby@@QAEPAVC..` | unclassified | 3 | 0 | run why-reg / why-branch for the full search |
| 99.35 | cmbtmgr | `?LoadArmies@combatManager@@QAEXE@Z` | unclassified | 2 | 0 | run why-reg / why-branch for the full search |
| 99.37 | events | `?DispatchEvent@advManager@@QAEXPAVhero@@PAVN..` | inliner (predict-inline) | 373 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline (40 name-unresolvable pair(s) discounted) |
| 99.38 | singleselectionwindow | `?SetupAdvancedOptions@TSingleSelectionWindow..` | register-homing (why-reg) | 98 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.40 | game | `?NewMap@game@@QAEEPBD0PAHH@Z` | unclassified | 6 | 0 | run why-reg / why-branch for the full search |
| 99.44 | dxplay | `?CreateModemConnection@CDPlayLobby@@QAEPAVCD..` | unclassified | 3 | 0 | run why-reg / why-branch for the full search |
| 99.46 | townmgr | `?handle_mage_guild_click@townManager@@QAEXXZ` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 99.47 | mapcell | `?Read@TTimedEvent@@QAEHPAVTAbstractFile@@H@Z` | inliner (predict-inline) | 3 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 99.50 | game | `?NewMap@game@@QAEXPAVTAbstractFile@@PAHPAVNe..` | unclassified | 30 | 0 | run why-reg / why-branch for the full search |
| 99.57 | seerhut | `?DoProgressDialog@type_creature_quest@@UAEXXZ` | register-homing (why-reg) | 5 | 0 | register-homing knob (B-family) |
| 99.58 | hero | `?get_primary_skill_total@hero@@QAEFXZ` | register-homing (why-reg) | 2 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.64 | diff | `?Apply@CDiffFile@@QAEPAXPAEH@Z` | register-homing (why-reg) | 6 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.64 | game | `?LoadBoatPool@game@@QAEHPAVTAbstractFile@@@Z` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.65 | cmbtmgr | `?SetNextArmy@combatManager@@QAEXHH@Z` | inliner (predict-inline) | 39 | 0 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 5 over-inline (2 name-unresolvable pair(s) discounted) |
| 99.66 | dxplay | `?CreateIPXConnection@CDPlayLobby@@QAEPAVCDPl..` | unclassified | 3 | 0 | run why-reg / why-branch for the full search |
| 99.66 | townmgr | `??0THallWindow@@QAE@H@Z` | inliner (predict-inline) | 42 | 0 | callee expanded on one side only (A8/A9/A12): 12 under-inline, 12 over-inline (222 name-unresolvable pair(s) discounted) |
| 99.69 | dxplay | `?CreateTCPIPConnection@CDPlayLobby@@QAEPAVCD..` | unclassified | 3 | 0 | run why-reg / why-branch for the full search |
| 99.75 | game | `?randomize_university@game@@QAEXPAVNewmapCel..` | inliner (predict-inline) | 12 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 99.84 | game | `?loadVictoryCondition@NewSMapHeader@@QAEHDPA..` | register-homing (why-reg) | 67 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.85 | town | `?TownFn_005BF900@town@@QAEJJ@Z` | unclassified | 14 | 0 | run why-reg / why-branch for the full search |
| 99.86 | spells | `?AreaEffect@combatManager@@QAEXJHJJ@Z` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.86 | combatresultswindow | `??0TCombatResultsWindow@@QAE@PBVhero@@0HHEH@Z` | unclassified | 160 | 0 | run why-reg / why-branch for the full search |
| 99.87 | singleselectionwindow | `?OnKillFocus@CEnterNameEdit@@UAEXXZ` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.87 | philai | `?AI_visit_war_factory@@YIXPAVhero@@@Z` | register-homing (why-reg) | 6 | 0 | register-homing knob (B-family) |
| 99.88 | command | `?automate_catapult@combatManager@@QAEEXZ` | register-homing (why-reg) | 6 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.89 | mapcell | `?NewfullMapFn_005042C0@NewfullMap@@QAEXXZ` | register-homing (why-reg) | 2 | 0 | register-homing knob (B-family) |
| 99.90 | swapmgr | `?handle_backpack_click@swapManager@@QAEXJJE@Z` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.92 | town | `?GiveSpells@town@@QAEXPAVhero@@@Z` | register-homing (why-reg) | 2 | 0 | register-homing knob (B-family) |
| 99.93 | resourcemanager | `?GetPalette24@ResourceManager@@YIPAVTPalette..` | register-homing (why-reg) | 54 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.94 | philai | `?value_of_obelisk@@YIHPAVNewmapCell@@J@Z` | inliner (predict-inline) | 2 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 99.95 | hero | `?initialize_ballistics_table@@YIEXZ` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.95 | ai_player | `?value_of_hiring@@YIJPAVtown@@PAVhero@@PAVse..` | inliner (predict-inline) | 48 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 99.96 | game | `?save@playerData@@QAEHPAVTAbstractFile@@@Z` | register-homing (why-reg) | 34 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.96 | philai | `?value_of_enemy_town@@YIJPBVhero@@PBVtown@@F..` | register-homing (why-reg) | 2 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.97 | ai_player | `?attempt_teleport@@YIEPAVhero@@AAV?$vector@U..` | (diag error: no shared public text symbol in built objects) | - | - | - |
| 99.97 | ai_player | `?fill_prohibited_array@@YIXPAVplayerData@@PA..` | register-homing (why-reg) | 2 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.97 | sacrifice_window | `?update_creature_offering@type_sacrifice_win..` | unclassified | 18 | 0 | run why-reg / why-branch for the full search |
| 99.98 | swapmgr | `?SetRolloverText@swapManager@@QAEXH@Z` | register-homing (why-reg) | 72 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.99 | singleselectionwindow | `?SetCurrentMap@TSingleSelectionWindow@@QAEXH..` | register-homing (why-reg) | 141 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.99 | philai | `?value_of_town@@YIJPBVhero@@HHHF@Z` | unclassified | 20 | 0 | run why-reg / why-branch for the full search |
| 100.00 | sacrifice_window | `?create_artifact_widgets@type_sacrifice_wind..` | inliner (predict-inline) | 4 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (72 name-unresolvable pair(s) discounted) |
