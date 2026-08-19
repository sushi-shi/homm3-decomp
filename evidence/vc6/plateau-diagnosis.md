<!-- # generator: homm3.vc6.report | # date: 2026-08-19 | # ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - regenerate, never hand-edit | plateaus in [50.0, 99.999%); base-vs-delinked-target diagnosis, no recompiles -->
# vc6 plateau diagnosis (read-only; solvers propose, never land)

211 function(s). why-reg = register-homing knobs; why-branch = control-flow knobs; predict-inline = out-of-line CALL multiset divergence (a callee inlined on one side only - dominated by STL basic_string/vector ops + small dtors retail inlines and we do not). CALIBRATION 2026-08-19: this column USED to be dominated by a NAME artifact - retail's side names an unclaimed callee with a synth working label our compiled side can never emit, so one call booked as both an under- and an over-inline and the inliner route (which sits upstream of registers and blocks) buried the true diagnosis. 77 of the 135 rows that reported divergence were that and nothing else; inline_model.divergence now pairs those off by count, and register-homing overtook the inliner as the dominant plateau class. MECHANISM (RE'd, docs/vc6/inliner.md): /Ob2 budget = clamp(2*caller_cb,1000,35000) spent sequentially; our leaner reconstructions sit at the 1000 floor and STARVE, so retail inlines what we call. FIX = finish the caller's body (budget follows statement mass, byte-inert counts) - do NOT chase _Tidy/vector spellings or pragmas. So on LOW-% rows inline divergence largely self-resolves as reconstruction completes; it is the pure wall only on high-% rows. Mixed walls list both distances.

## Wall-class summary

- **108** register-homing (why-reg)
- **46** inliner (predict-inline)
- **37** control-flow (why-branch)
- **20** unclassified

| fuzzy | unit | function | wall class | reg-dist | flow-dist | knob to try |
|---|---|---|---|---|---|---|
| 50.46 | game | `?Load@game@@QAEHPAVTAbstractFile@@@Z` | inliner (predict-inline) | 1497 | 211 | callee expanded on one side only (A8/A9/A12): 14 under-inline, 2 over-inline (30 name-unresolvable pair(s) discounted) |
| 57.47 | findpath | `?PushPoint@searchArray@@QAEXPBUpathCell@@PAU..` | inliner (predict-inline) | 715 | 91 | callee expanded on one side only (A8/A9/A12): 9 under-inline (19 name-unresolvable pair(s) discounted) |
| 58.84 | cmbtmgr | `?GenerateMap@combatManager@@QAEXXZ` | register-homing (why-reg) | 96 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 59.36 | advmgr | `?QuickInfo@advManager@@QAEXHHH@Z` | inliner (predict-inline) | 1951 | 515 | callee expanded on one side only (A8/A9/A12): 26 over-inline (11 name-unresolvable pair(s) discounted) |
| 64.73 | advmgr | `?get_army_help_text@@YI?AV?$basic_string@DU?..` | inliner (predict-inline) | 289 | 62 | callee expanded on one side only (A8/A9/A12): 8 under-inline (11 name-unresolvable pair(s) discounted) |
| 68.08 | townmgr | `?BuyBuild@townManager@@QAEHHHH@Z` | inliner (predict-inline) | 895 | 83 | callee expanded on one side only (A8/A9/A12): 4 over-inline (19 name-unresolvable pair(s) discounted) |
| 70.69 | game | `?get_underground_gate_exit@game@@QAE?AUtype_..` | control-flow (why-branch) | 38 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 72.01 | iconwdgt | `?NextRandomFrame@iconWidget@@QAEXXZ` | control-flow (why-branch) | 172 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 74.09 | lodfile | `?Find@LODFile@@AAEXIIPBD@Z` | control-flow (why-branch) | 150 | 14 | loop-form / merged-return placement / case order (D1-D9) |
| 74.48 | armygrp | `?get_morale_description@armyGroup@@QBE?AV?$b..` | inliner (predict-inline) | 631 | 134 | callee expanded on one side only (A8/A9/A12): 7 under-inline, 2 over-inline (35 name-unresolvable pair(s) discounted) |
| 74.49 | hero | `?remove_artifact@hero@@QAEXJ@Z` | control-flow (why-branch) | 91 | 6 | loop-form / merged-return placement / case order (D1-D9) |
| 75.01 | cmbtmgr | `?CalculateGainedExperience@combatManager@@QA..` | register-homing (why-reg) | 88 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 75.74 | findpath | `?PushCombatPoint@searchArray@@QAEXHHHHH@Z` | inliner (predict-inline) | 177 | 42 | callee expanded on one side only (A8/A9/A12): 2 over-inline (9 name-unresolvable pair(s) discounted) |
| 76.87 | artifact | `?InitializeArtifactTraitsTable@@YIEXZ` | inliner (predict-inline) | 332 | 41 | callee expanded on one side only (A8/A9/A12): 1 under-inline (6 name-unresolvable pair(s) discounted) |
| 77.49 | game | `?ClaimShipyard@game@@QAEXUtype_point@@H@Z` | control-flow (why-branch) | 218 | 16 | loop-form / merged-return placement / case order (D1-D9) |
| 77.73 | levelupwindow | `?WindowHandler@TLevelUpWindow@@UAEHPAVmessag..` | inliner (predict-inline) | 104 | 36 | callee expanded on one side only (A8/A9/A12): 3 over-inline (3 name-unresolvable pair(s) discounted) |
| 78.12 | remote | `??_GCNetMsgHandler@@UAEPAXI@Z` | register-homing (why-reg) | 6 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 78.26 | font | `?DrawCharacter@font@@QAEXHPAVBitmap16Bit@@HH..` | register-homing (why-reg) | 62 | 1 | spill to dead-parameter slot (B4) |
| 78.80 | advmgr | `?ProcessHover@advManager@@QAEHHH@Z` | inliner (predict-inline) | 527 | 120 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (5 name-unresolvable pair(s) discounted) |
| 79.09 | mapcell | `?get_special_terrain@NewmapCell@@QBE?AW4TAdv..` | control-flow (why-branch) | 59 | 52 | loop-form / merged-return placement / case order (D1-D9) |
| 79.54 | armygrp | `?Merge@armyGroup@@QAEEPAV1@@Z` | control-flow (why-branch) | 135 | 27 | loop-form / merged-return placement / case order (D1-D9) |
| 79.70 | hillfortwindow | `?Recalculate@THillFortWindow@@QAEXE@Z` | inliner (predict-inline) | 288 | 10 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 80.30 | ai | `?choose_melee_target@combatManager@@QAEEPBVa..` | control-flow (why-branch) | 807 | 15 | loop-form / merged-return placement / case order (D1-D9) |
| 80.52 | ai_player | `?make_gift@type_AI_player@@QAEXJ@Z` | inliner (predict-inline) | 317 | 46 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 1 over-inline (14 name-unresolvable pair(s) discounted) |
| 80.94 | advmgr | `?ProcessWaitingHover@advManager@@QAEHHH@Z` | register-homing (why-reg) | 224 | 2 | spill to dead-parameter slot (B4) |
| 81.04 | path | `?ValidAttack@army@@QBEHHHHHPAH@Z` | control-flow (why-branch) | 70 | 59 | loop-form / merged-return placement / case order (D1-D9) |
| 81.21 | town | `?get_buildable_mask@town@@QBE_JXZ` | register-homing (why-reg) | 56 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.37 | command | `?is_computer_action@combatManager@@QAEEPBVar..` | inliner (predict-inline) | 44 | 52 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 81.49 | town | `?get_build_cost@town@@QBEFW4type_building_id..` | register-homing (why-reg) | 35 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.64 | campaignwindow | `?CampaignWindowHandler@@YIHAAVmessage@@@Z` | register-homing (why-reg) | 60 | 7 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.73 | town | `?get_legion_bonus@town@@QAEJJ@Z` | control-flow (why-branch) | 21 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 82.01 | advmgr | `?FindAdjacentMonster@advManager@@QAEEUtype_p..` | register-homing (why-reg) | 216 | 2 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 82.49 | ai | `?place_shooter@combatManager@@QAEXPBVarmy@@@Z` | control-flow (why-branch) | 79 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 82.54 | campaignwindow | `??0TCampaignWindow@@QAE@EH@Z` | inliner (predict-inline) | 259 | 89 | callee expanded on one side only (A8/A9/A12): 9 over-inline (19 name-unresolvable pair(s) discounted) |
| 82.57 | armygrp | `?get_luck_description@armyGroup@@QBE?AV?$bas..` | inliner (predict-inline) | 121 | 40 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 1 over-inline (14 name-unresolvable pair(s) discounted) |
| 82.86 | hero | `?initialize@hero@@QAEXF@Z` | inliner (predict-inline) | 99 | 17 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 82.93 | mousemgr | `?SetPointer@mouseManager@@QAEXHW4EPointerSet..` | register-homing (why-reg) | 25 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 83.21 | advmgr | `?DrawAdvObjShadow@advManager@@QAEXHHHHH@Z` | register-homing (why-reg) | 501 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 83.36 | combatoptionswindow | `?CombatOptionsWindowHandler@@YIHAAVmessage@@..` | inliner (predict-inline) | 156 | 22 | callee expanded on one side only (A8/A9/A12): 4 over-inline (3 name-unresolvable pair(s) discounted) |
| 83.95 | diff | `?MakeDiff@CDiffMaker@@QAEPAVCDiffFile@@AAK@Z` | register-homing (why-reg) | 214 | 1 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 84.17 | diff | `?FindNextSame@CDiffMaker@@IAE_NHHAAH0@Z` | register-homing (why-reg) | 14 | 0 | register-homing knob (B-family) |
| 84.38 | ai_tactical | `?get_hex_attack_value@type_AI_attack_hex_cho..` | register-homing (why-reg) | 94 | 1 | spill to dead-parameter slot (B4) |
| 84.41 | game | `?ClaimGarrison@game@@QAEXHH@Z` | register-homing (why-reg) | 10 | 0 | register-homing knob (B-family) |
| 84.72 | cspriteframe | `?SetPixelFormat@CSpriteFrame@@SIXIII@Z` | register-homing (why-reg) | 44 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 85.43 | ai_combat | `?cast_spell@type_AI_combat_data@@QAEXAAV1@W4..` | inliner (predict-inline) | 323 | 26 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 85.58 | ai_tactical | `?should_attack_now@type_AI_spellcaster@@QAEE..` | register-homing (why-reg) | 68 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 85.70 | hero | `?can_summon_boat@hero@@QAEEXZ` | register-homing (why-reg) | 53 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 85.91 | fly | `?Fly@army@@QAEHH@Z` | register-homing (why-reg) | 311 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 85.99 | townmgr | `??0THallWindow@@QAE@H@Z` | inliner (predict-inline) | 2701 | 150 | callee expanded on one side only (A8/A9/A12): 13 under-inline, 12 over-inline (223 name-unresolvable pair(s) discounted) |
| 86.01 | army | `?GoBerserk@army@@QAEXXZ` | control-flow (why-branch) | 16 | 10 | loop-form / merged-return placement / case order (D1-D9) |
| 86.33 | search | `?BuildPath@searchArray@@QAEHPBVhero@@J@Z` | inliner (predict-inline) | 142 | 17 | callee expanded on one side only (A8/A9/A12): 1 over-inline (5 name-unresolvable pair(s) discounted) |
| 86.38 | ai_player | `?calculate_demand@type_AI_player@@QAEXXZ` | inliner (predict-inline) | 387 | 34 | callee expanded on one side only (A8/A9/A12): 1 under-inline (7 name-unresolvable pair(s) discounted) |
| 86.38 | advmgr | `?DrawAdvObj@advManager@@QAEXHHHHH@Z` | register-homing (why-reg) | 863 | 2 | spill to dead-parameter slot (B4) |
| 86.41 | iconwdgt | `?NextRandomSiegeEngineFrame@iconWidget@@QAEX..` | register-homing (why-reg) | 59 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 86.63 | resourcemanager | `?AddToCache@ResourceManager@@YIXPAVresource@..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.11 | ai | `?find_attack_hexes@@YIXPBVarmy@@JJJJPBVsearc..` | inliner (predict-inline) | 110 | 29 | callee expanded on one side only (A8/A9/A12): 1 over-inline (8 name-unresolvable pair(s) discounted) |
| 87.13 | advmgr | `?get_creature_bank_help_text@@YIXPADPAVNewma..` | control-flow (why-branch) | 71 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 87.19 | smackmgr | `?VideoPlay@@YIHHHHHH@Z` | register-homing (why-reg) | 143 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 87.29 | cmbtmgr | `?RemoveObstacle@combatManager@@QAEXH@Z` | register-homing (why-reg) | 23 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.73 | ai_tactical | `?find_enemy_attacks@type_AI_spellcaster@@QAE..` | register-homing (why-reg) | 128 | 4 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.02 | findpath | `?GetTerrainCost@@YIHPAVhero@@Utype_point@@HH..` | register-homing (why-reg) | 87 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 88.10 | button | `?Main@button@@UAEHPAVmessage@@@Z` | inliner (predict-inline) | 245 | 48 | callee expanded on one side only (A8/A9/A12): 4 under-inline (7 name-unresolvable pair(s) discounted) |
| 88.14 | winmgr | `?FadeFromBlack@heroWindowManager@@QAEXH@Z` | register-homing (why-reg) | 108 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.17 | game | `?calculate_production@game@@QAEXXZ` | control-flow (why-branch) | 158 | 33 | loop-form / merged-return placement / case order (D1-D9) |
| 88.44 | button | `??0textButton@@QAE@HHHHHPBD00HHEHHH@Z` | inliner (predict-inline) | 53 | 2 | callee expanded on one side only (A8/A9/A12): 2 over-inline (3 name-unresolvable pair(s) discounted) |
| 88.44 | slider | `?SetKnob@slider@@IAEXH@Z` | control-flow (why-branch) | 20 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 88.51 | armygrp | `?get_spell_work_chance@@YIMHW4TCreatureType@..` | control-flow (why-branch) | 236 | 141 | loop-form / merged-return placement / case order (D1-D9) |
| 88.51 | winmgr | `?FadeToBlack@heroWindowManager@@QAEXHE@Z` | register-homing (why-reg) | 128 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.70 | viewarmywindow | `??0TViewArmyWindow@@QAE@PAVarmyGroup@@HPBVhe..` | inliner (predict-inline) | 144 | 32 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 1 over-inline (22 name-unresolvable pair(s) discounted) |
| 88.93 | smackmgr | `?VideoRealignBuffers@@YIXXZ` | unclassified | 6 | 0 | run why-reg / why-branch for the full search |
| 89.04 | findpath | `?TestPossibleDirections@searchArray@@QAEXPAV..` | control-flow (why-branch) | 592 | 48 | loop-form / merged-return placement / case order (D1-D9) |
| 89.14 | fly | `?ValidFlight@army@@QBEEHE@Z` | control-flow (why-branch) | 46 | 24 | loop-form / merged-return placement / case order (D1-D9) |
| 89.46 | ai_combat | `?get_enchantment_value@type_AI_combat_data@@..` | register-homing (why-reg) | 107 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.50 | advmgr | `?GetSoundId@advManager@@QAE?AW4e_looping_sou..` | control-flow (why-branch) | 507 | 42 | loop-form / merged-return placement / case order (D1-D9) |
| 89.53 | ai_player | `?end_turn@type_AI_player@@QAEXXZ` | control-flow (why-branch) | 102 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 89.60 | town | `?can_build@town@@QBEEF@Z` | register-homing (why-reg) | 87 | 0 | spill to dead-parameter slot (B4) |
| 89.63 | smackmgr | `?VideoDrawRects@@YIXXZ` | register-homing (why-reg) | 149 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.77 | townmgr | `?DoUniversity@townManager@@QAEXXZ` | inliner (predict-inline) | 74 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (6 name-unresolvable pair(s) discounted) |
| 89.82 | findpath | `?SeedCombatPosition@searchArray@@QAEXPBVarmy..` | register-homing (why-reg) | 38 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.82 | advmgr | `?SetRolloverText@advManager@@QAEXPAVNewmapCe..` | inliner (predict-inline) | 852 | 404 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 7 over-inline (3 name-unresolvable pair(s) discounted) |
| 89.86 | town | `?initialize_spells@town@@QAEXPBVTownExtra@@@Z` | control-flow (why-branch) | 80 | 11 | loop-form / merged-return placement / case order (D1-D9) |
| 89.88 | viewarmywindow | `?create_dismiss_widget@TViewArmyWindow@@QAEX..` | control-flow (why-branch) | 31 | 20 | loop-form / merged-return placement / case order (D1-D9) |
| 89.88 | viewarmywindow | `?create_upgrade_widget@TViewArmyWindow@@QAEX..` | control-flow (why-branch) | 31 | 20 | loop-form / merged-return placement / case order (D1-D9) |
| 89.99 | viewarmywindow | `??0TViewArmyWindow@@QAE@PBVarmy@@HHE@Z` | inliner (predict-inline) | 199 | 46 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 3 over-inline (26 name-unresolvable pair(s) discounted) |
| 90.07 | mainmenu | `?MainMenuHandler@@YIHAAVmessage@@@Z` | inliner (predict-inline) | 201 | 8 | callee expanded on one side only (A8/A9/A12): 1 under-inline (8 name-unresolvable pair(s) discounted) |
| 90.75 | townmgr | `?DoBlacksmith@@YIXHH@Z` | register-homing (why-reg) | 34 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.78 | quickherowindow | `??0TQuickHeroWindow@@QAE@PAVhero@@W4TViewLev..` | inliner (predict-inline) | 239 | 75 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (40 name-unresolvable pair(s) discounted) |
| 90.84 | recruit | `?Update@recruitUnit@@QAEXEJ@Z` | register-homing (why-reg) | 140 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.93 | townmgr | `?UpdateTownLocator@TTownScreenWindow@@QAEXH@Z` | register-homing (why-reg) | 11 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.93 | ai_combat | `?choose_melee@type_AI_combat_data@@QBE_NABV1..` | inliner (predict-inline) | 283 | 2 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (2 name-unresolvable pair(s) discounted) |
| 91.01 | ai_combat | `?initialize_creatures@type_AI_combat_data@@Q..` | register-homing (why-reg) | 334 | 2 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 91.11 | remote | `??1CNetMsgHandlerPause@@UAE@XZ` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.25 | advmgr | `?DrawShroud@advManager@@QAEXHHHHH@Z` | register-homing (why-reg) | 74 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.30 | bottomviewsubwindow | `??0TBottomViewResourceMessage@@QAE@PAVheroWi..` | register-homing (why-reg) | 40 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.51 | campaignbrief | `??1NewSMapHeader@@QAE@XZ` | unclassified | 12 | 0 | run why-reg / why-branch for the full search |
| 91.71 | army | `?Turn@army@@QAEXE@Z` | register-homing (why-reg) | 16 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.92 | systemoptionswindow | `?WindowHandler@TSystemOptionsWindow@@UAEHPAV..` | control-flow (why-branch) | 119 | 72 | loop-form / merged-return placement / case order (D1-D9) |
| 91.94 | hillfortwindow | `?HillFortWindowHandler@@YIHAAVmessage@@@Z` | control-flow (why-branch) | 60 | 23 | loop-form / merged-return placement / case order (D1-D9) |
| 91.98 | border | `?Main@border@@UAEHPAVmessage@@@Z` | control-flow (why-branch) | 37 | 28 | loop-form / merged-return placement / case order (D1-D9) |
| 92.00 | army | `?can_shoot@army@@QBEEPBV1@@Z` | control-flow (why-branch) | 13 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 92.19 | advmgr | `?ProcessSearch@advManager@@QAEHHHH@Z` | register-homing (why-reg) | 214 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 92.22 | advmgr | `?DrawUnderlay@advManager@@QAEXHHHHH@Z` | register-homing (why-reg) | 166 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.52 | army | `?get_berserk_targets@army@@QBEXAAV?$vector@P..` | register-homing (why-reg) | 53 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.57 | victorylossconditions | `?IsGrailTarget@VictoryConditionStruct@@QAEEP..` | register-homing (why-reg) | 22 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.57 | command | `?GetCommand@combatManager@@QAEHH@Z` | register-homing (why-reg) | 104 | 1 | spill to dead-parameter slot (B4) |
| 92.68 | font | `?LineLength@font@@QAEHPBDH@Z` | register-homing (why-reg) | 17 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.68 | advmgr | `?BVMessage@advManager@@QAEXPBD@Z` | inliner (predict-inline) | 13 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 92.88 | window | `?CenterWindow@heroWindow@@QAEXHH@Z` | register-homing (why-reg) | 79 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.22 | cmbtmgr | `?RaiseSkeletons@combatManager@@QAEXH@Z` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 93.33 | advmgr | `?BVResMsg@advManager@@QAEXPBDHH@Z` | inliner (predict-inline) | 13 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 93.37 | strip | `?DrawOwner@strip@@IAEXH@Z` | unclassified | 12 | 0 | run why-reg / why-branch for the full search |
| 93.48 | mapcell | `?get_map_object@NewmapCell@@QAE?AW4TAdventur..` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.61 | resourcemanager | `?GetSpreadsheet@ResourceManager@@YIPAVTSprea..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.66 | mapcell | `?CalculateCellExtra@NewfullMap@@QAEXPAVNewma..` | control-flow (why-branch) | 6 | 21 | loop-form / merged-return placement / case order (D1-D9) |
| 93.87 | initialize | `?create_included_mask@@YIXPBHPA_J@Z` | unclassified | 6 | 0 | run why-reg / why-branch for the full search |
| 93.88 | townmgr | `??0type_monster_join_window@@QAE@PAVhero@@PA..` | control-flow (why-branch) | 43 | 24 | loop-form / merged-return placement / case order (D1-D9) |
| 94.08 | army | `?attack_hex@army@@QAEEHE@Z` | control-flow (why-branch) | 37 | 7 | loop-form / merged-return placement / case order (D1-D9) |
| 94.17 | mousemgr | `?Update@mouseManager@@QAEXE@Z` | register-homing (why-reg) | 375 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 94.25 | hero | `?Fly@hero@@QAEXH@Z` | inliner (predict-inline) | 31 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 94.54 | ai | `?mark_multiheaded_enemy@combatManager@@QAEXP..` | inliner (predict-inline) | 64 | 42 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 94.73 | game | `?GetRandomArtifactId@game@@QAE?AW4TArtifact@..` | register-homing (why-reg) | 27 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.79 | ai_combat | `?do_general_melee@type_AI_combat_data@@QAEXA..` | inliner (predict-inline) | 35 | 20 | callee expanded on one side only (A8/A9/A12): 1 under-inline |
| 94.88 | advmgr | `?DrawGround@advManager@@QAEXHHHHH@Z` | inliner (predict-inline) | 170 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline (2 name-unresolvable pair(s) discounted) |
| 95.03 | town | `?GiveSpells@town@@QAEXPAVhero@@@Z` | control-flow (why-branch) | 18 | 35 | loop-form / merged-return placement / case order (D1-D9) |
| 95.19 | slider | `?Main@slider@@UAEHPAVmessage@@@Z` | control-flow (why-branch) | 57 | 48 | loop-form / merged-return placement / case order (D1-D9) |
| 95.27 | iconwdgt | `?Main@iconWidget@@UAEHPAVmessage@@@Z` | control-flow (why-branch) | 48 | 30 | loop-form / merged-return placement / case order (D1-D9) |
| 95.28 | ai_tactical | `?set_melee_enemies@type_AI_spellcaster@@QAEX..` | register-homing (why-reg) | 52 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.46 | cmbtmgr | `?InLineOfSight@combatManager@@QBEEHH@Z` | register-homing (why-reg) | 24 | 1 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 95.49 | ai_tactical | `?get_cure_value@type_AI_spellcaster@@QAEJPBV..` | register-homing (why-reg) | 30 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.59 | townmgr | `??0TTownScreenWindow@@QAE@XZ` | inliner (predict-inline) | 823 | 190 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 6 over-inline (248 name-unresolvable pair(s) discounted) |
| 95.67 | seerhut | `?GetRequirementText@type_skill_quest@@UAE?AV..` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.76 | campaignbrief | `??1TCampaignBrief@@UAE@XZ` | register-homing (why-reg) | 52 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.78 | ai | `?choose_resurrect_action@combatManager@@QAEE..` | register-homing (why-reg) | 54 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.89 | ai | `?choose_creature_spell@combatManager@@QAEEPB..` | register-homing (why-reg) | 21 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.92 | smackmgr | `?VideoClose@@YIXXZ` | control-flow (why-branch) | 2 | 20 | loop-form / merged-return placement / case order (D1-D9) |
| 95.93 | misc | `?ReadPrefsFromRegistry@@YIXXZ` | inliner (predict-inline) | 151 | 2 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 96.16 | mapcell | `?is_diggable@NewmapCell@@QAEEXZ` | control-flow (why-branch) | 3 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 96.35 | ai_tactical | `?get_hypnotize_value@type_AI_spellcaster@@QA..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.35 | ai | `?get_attack_change@combatManager@@QAEJPBVarm..` | register-homing (why-reg) | 50 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.38 | combatresultswindow | `??0TCombatResultsWindow@@QAE@PBVhero@@0HHEH@Z` | inliner (predict-inline) | 320 | 77 | callee expanded on one side only (A8/A9/A12): 2 over-inline (126 name-unresolvable pair(s) discounted) |
| 96.40 | combatcontrolsubwindow | `??0TCombatControlSubWindow@@QAE@PAVheroWindo..` | register-homing (why-reg) | 16 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.43 | townmgr | `?Recruit@TCastleWindow@@QAEXH@Z` | register-homing (why-reg) | 36 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.43 | viewarmywindow | `?create_damage_widget@TViewArmyWindow@@QAEXP..` | inliner (predict-inline) | 29 | 22 | callee expanded on one side only (A8/A9/A12): 1 over-inline (14 name-unresolvable pair(s) discounted) |
| 96.43 | game | `??0SavedGameHeader@@QAE@XZ` | unclassified | 13 | 0 | run why-reg / why-branch for the full search |
| 96.46 | ai_player | `?calculate_reserve@type_AI_player@@QAEXXZ` | register-homing (why-reg) | 61 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.46 | town | `?destroy_extra_capitol@town@@QAEXXZ` | register-homing (why-reg) | 34 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.53 | hero | `?DestroySiegeWeaponArtifact@hero@@QAEXH@Z` | register-homing (why-reg) | 17 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 96.56 | armygrp | `?GetArmyMorale@armyGroup@@QAEHHPBVhero@@PBVt..` | register-homing (why-reg) | 75 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.63 | remote | `?KillOldChat@CChatManager@@QAEXXZ` | register-homing (why-reg) | 28 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.67 | recruit | `??0recruitUnit@@QAE@PAVhero@@W4TCreatureType..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.01 | army | `?SetLuck@army@@QAEXPBVhero@@PBVarmyGroup@@PB..` | unclassified | 16 | 0 | run why-reg / why-branch for the full search |
| 97.09 | remote | `?CheckForWarning@CTurnDuration@@QAEXXZ` | register-homing (why-reg) | 38 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.32 | ai_combat | `?do_aftermath@type_AI_combat_data@@QAEXPAV1@..` | register-homing (why-reg) | 4 | 0 | register-homing knob (B-family) |
| 97.52 | army | `?SetMorale@army@@QAEXPBVhero@@PBVarmyGroup@@..` | register-homing (why-reg) | 39 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.77 | bottomviewsubwindow | `??0TBottomViewHero@@QAE@PAVheroWindow@@@Z` | register-homing (why-reg) | 86 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.77 | townmgr | `?SetRolloverText@TCastleWindow@@QAEXPAVmessa..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.97 | hero | `?TransferArtifacts@hero@@QAEXPAV1@@Z` | register-homing (why-reg) | 14 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 97.97 | recruit | `??0recruitUnit@@QAE@PAVarmyGroup@@EW4TCreatu..` | register-homing (why-reg) | 24 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.01 | advmgr | `?set_witch_hut_help_text@@YIXPADPAVhero@@PAV..` | register-homing (why-reg) | 19 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.02 | townmgr | `?WindowHandler@TBuyBuildWindow@@UAEHPAVmessa..` | unclassified | 3 | 0 | run why-reg / why-branch for the full search |
| 98.09 | townmgr | `?SetupTown@townManager@@QAEXE@Z` | unclassified | 20 | 0 | run why-reg / why-branch for the full search |
| 98.10 | hero | `?GetExperienceIncrement@hero@@SIHH@Z` | register-homing (why-reg) | 23 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.13 | systemoptionswindow | `??0TSystemOptionsWindow@@QAE@XZ` | inliner (predict-inline) | 145 | 105 | callee expanded on one side only (A8/A9/A12): 2 under-inline (133 name-unresolvable pair(s) discounted) |
| 98.17 | advmgr | `?DrawHeroPart@advManager@@QAEXHAAUTDrawParts..` | unclassified | 15 | 0 | run why-reg / why-branch for the full search |
| 98.18 | advmgr | `?DrawHeroPartShadow@advManager@@QAEXHAAUTDra..` | unclassified | 15 | 0 | run why-reg / why-branch for the full search |
| 98.29 | townmgr | `??0type_garrison_base_window@@QAE@PAVhero@@H..` | inliner (predict-inline) | 169 | 44 | callee expanded on one side only (A8/A9/A12): 4 over-inline (203 name-unresolvable pair(s) discounted) |
| 98.46 | font | `?DrawStringExecute@font@@QAEXPBDHPAVBitmap16..` | register-homing (why-reg) | 72 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.47 | ai | `?DoCompAI@combatManager@@QAEXH@Z` | unclassified | 1 | 0 | run why-reg / why-branch for the full search |
| 98.50 | misc | `?CheckConfigFile@@YIXXZ` | register-homing (why-reg) | 47 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.52 | bottomviewsubwindow | `??0TBottomViewKingdom@@QAE@PAVheroWindow@@@Z` | register-homing (why-reg) | 24 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.57 | armygrp | `?GetMorale@armyGroup@@QAEHPBVhero@@PBVtown@@..` | register-homing (why-reg) | 13 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.58 | townmgr | `?SetupWell@townManager@@QAEXPAVTCastleWindow..` | register-homing (why-reg) | 48 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.58 | adventuremapwindow | `?UpdateHeroLocator@TAdventureMapWindow@@QAEX..` | control-flow (why-branch) | 9 | 14 | loop-form / merged-return placement / case order (D1-D9) |
| 98.61 | townmgr | `?DoTownGate@townManager@@QAEXXZ` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.68 | hero | `?IsInIdentifyRange@hero@@QAEEPBUtype_point@@..` | register-homing (why-reg) | 2 | 0 | register-homing knob (B-family) |
| 98.75 | bottomviewsubwindow | `??0TBottomViewTown@@QAE@PAVheroWindow@@@Z` | register-homing (why-reg) | 81 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 98.79 | font | `?DrawBoundedString@font@@QAEXPBDPAVBitmap16B..` | register-homing (why-reg) | 9 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.82 | levelupwindow | `??0TLevelUpWindow@@QAE@PAVhero@@HHH@Z` | inliner (predict-inline) | 72 | 30 | callee expanded on one side only (A8/A9/A12): 1 under-inline (85 name-unresolvable pair(s) discounted) |
| 98.84 | townmgr | `?BuildObj@townManager@@QAEXH@Z` | unclassified | 13 | 1 | run why-reg / why-branch for the full search |
| 98.84 | quicktownwindow | `??0TQuickTownWindow@@QAE@PBVtown@@W4TViewLev..` | register-homing (why-reg) | 16 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 98.84 | puzzlewindow | `??0type_AI_puzzle_tile@@QAE@PAVNewmapCell@@U..` | register-homing (why-reg) | 9 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.98 | game | `?GetNewHeroId@game@@QAEHHW4THeroClass@@E0@Z` | register-homing (why-reg) | 6 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.98 | ai_tactical | `?get_frenzy_value@type_AI_spellcaster@@QAEJP..` | unclassified | 6 | 0 | run why-reg / why-branch for the full search |
| 99.10 | ai | `?choose_defense_hex@combatManager@@QAEEPBVar..` | register-homing (why-reg) | 30 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.11 | ai_tactical | `?get_simple_attack_effect@type_AI_combat_par..` | register-homing (why-reg) | 32 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.31 | mousemgr | `?LoadFrame@mouseManager@@QAEXH@Z` | register-homing (why-reg) | 32 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.36 | advmgr | `?SetShrineHelpText@@YIXPADPAVhero@@PAVNewmap..` | unclassified | 3 | 1 | run why-reg / why-branch for the full search |
| 99.39 | resourcedisplay | `??0TResourceDisplay@@QAE@PAVheroWindow@@E@Z` | register-homing (why-reg) | 52 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.58 | hero | `?get_primary_skill_total@hero@@QAEFXZ` | register-homing (why-reg) | 2 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.64 | diff | `?Apply@CDiffFile@@QAEPAXPAEH@Z` | register-homing (why-reg) | 6 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.64 | game | `?LoadBoatPool@game@@QAEHPAVTAbstractFile@@@Z` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.70 | game | `?SaveRumours@game@@AAEHPAVTAbstractFile@@@Z` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 99.72 | townmgr | `??0TCastleWindow@@QAE@XZ` | inliner (predict-inline) | 96 | 0 | callee expanded on one side only (A8/A9/A12): 93 under-inline, 93 over-inline (399 name-unresolvable pair(s) discounted) |
| 99.74 | advmgr | `?SetTreeHelpText@@YIXPADPAVhero@@PAVNewmapCe..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.78 | town | `?initialize_hordes@town@@SIXXZ` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.78 | soundmgr | `?MemorySample@soundManager@@QAEPAVds_memsamp..` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.84 | victorylossconditions | `?CheckForDefeatedTownLoss@LossConditionStruc..` | register-homing (why-reg) | 22 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.85 | town | `?TownFn_005BF900@town@@QAEJJ@Z` | unclassified | 14 | 0 | run why-reg / why-branch for the full search |
| 99.85 | ai_tactical | `?get_attack_skill_value@type_AI_spellcaster@..` | register-homing (why-reg) | 52 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.87 | ai_tactical | `?get_curse_value@type_AI_spellcaster@@QAEJPB..` | unclassified | 34 | 0 | run why-reg / why-branch for the full search |
| 99.92 | armygrp | `?WindowHandler@TSplitWindow@@UAEHPAVmessage@..` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.93 | townmgr | `?SetupMage@townManager@@QAEXPAVheroWindow@@@Z` | register-homing (why-reg) | 2 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.94 | army | `?do_multi_head_attack@army@@QAEXIPAH0PAJ@Z` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.94 | adventureoptionswindow | `?WindowHandler@TAdventureOptionsWindow@@UAEH..` | control-flow (why-branch) | 34 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 99.94 | game | `?LoadMinePool@game@@QAEHPAVTAbstractFile@@H@Z` | register-homing (why-reg) | 4 | 0 | spill to dead-parameter slot (B4) |
| 99.95 | events | `?monsters_sell_out@advManager@@QAE_NPAVhero@..` | register-homing (why-reg) | 12 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.95 | hero | `?PlaceInMap@hero@@QAEXHUtype_point@@E@Z` | register-homing (why-reg) | 2 | 0 | register-homing knob (B-family) |
| 99.96 | ai_combat | `?AI_value_of_combat@@YIJPBVhero@@0ABVarmyGro..` | unclassified | 8 | 0 | run why-reg / why-branch for the full search |
| 99.98 | townmgr | `?WindowHandler@TShipWindow@@UAEHPAVmessage@@..` | register-homing (why-reg) | 4 | 0 | register-homing knob (B-family) |
| 99.99 | armygrp | `??0TSplitWindow@@QAE@HHH@Z` | inliner (predict-inline) | 2 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (32 name-unresolvable pair(s) discounted) |
| 99.99 | townmgr | `?DoHall@townManager@@QAEXXZ` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
