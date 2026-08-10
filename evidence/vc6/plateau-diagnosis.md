<!-- # generator: homm3.vc6.report | # date: 2026-08-10 | # ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - regenerate, never hand-edit | plateaus in [50.0, 99.999%); base-vs-delinked-target diagnosis, no recompiles -->
# vc6 plateau diagnosis (read-only; solvers propose, never land)

125 function(s). why-reg = register-homing knobs; why-branch = control-flow knobs; predict-inline = out-of-line CALL multiset divergence (a callee inlined on one side only - dominated by STL basic_string/vector ops + small dtors retail inlines and we do not). MECHANISM (RE'd, docs/vc6/inliner.md): /Ob2 budget = clamp(2*caller_cb,1000,35000) spent sequentially; our leaner reconstructions sit at the 1000 floor and STARVE, so retail inlines what we call. FIX = finish the caller's body (budget follows statement mass, byte-inert counts) - do NOT chase _Tidy/vector spellings or pragmas. So on LOW-% rows inline divergence largely self-resolves as reconstruction completes; it is the pure wall only on high-% rows. Mixed walls list both distances.

## Wall-class summary

- **81** inliner (predict-inline)
- **35** register-homing (why-reg)
- **6** unclassified
- **3** control-flow (why-branch)

| fuzzy | unit | function | wall class | reg-dist | flow-dist | knob to try |
|---|---|---|---|---|---|---|
| 50.31 | game | `?Load@game@@QAEHPAVTAbstractFile@@@Z` | inliner (predict-inline) | 1496 | 211 | callee expanded on one side only (A8/A9/A12): 83 under-inline, 71 over-inline |
| 54.95 | advmgr | `?get_army_help_text@@YI?AV?$basic_string@DU?..` | inliner (predict-inline) | 245 | 65 | callee expanded on one side only (A8/A9/A12): 28 under-inline, 17 over-inline |
| 55.81 | advmgr | `?QuickInfo@advManager@@QAEXHHH@Z` | inliner (predict-inline) | 2275 | 557 | callee expanded on one side only (A8/A9/A12): 29 under-inline, 54 over-inline |
| 58.84 | cmbtmgr | `?GenerateMap@combatManager@@QAEXXZ` | register-homing (why-reg) | 96 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 67.56 | armygrp | `?get_morale_description@armyGroup@@QBE?AV?$b..` | inliner (predict-inline) | 721 | 202 | callee expanded on one side only (A8/A9/A12): 56 under-inline, 49 over-inline |
| 68.37 | exec | `?CallManager@executive@@QAEXPAVbaseManager@@..` | inliner (predict-inline) | 65 | 3 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 3 over-inline |
| 72.01 | iconwdgt | `?NextRandomFrame@iconWidget@@QAEXXZ` | control-flow (why-branch) | 172 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 72.18 | misc | `??0TPickANumber@@QAE@HH@Z` | inliner (predict-inline) | 37 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 74.49 | hero | `?remove_artifact@hero@@QAEXJ@Z` | inliner (predict-inline) | 91 | 6 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 74.78 | advmgr | `?ProcessHover@advManager@@QAEHHH@Z` | inliner (predict-inline) | 555 | 115 | callee expanded on one side only (A8/A9/A12): 14 under-inline, 13 over-inline |
| 74.79 | armygrp | `?get_luck_description@armyGroup@@QBE?AV?$bas..` | inliner (predict-inline) | 185 | 40 | callee expanded on one side only (A8/A9/A12): 26 under-inline, 22 over-inline |
| 74.90 | advmgr | `?ProcessWaitingHover@advManager@@QAEHHH@Z` | inliner (predict-inline) | 241 | 10 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 75.01 | cmbtmgr | `?CalculateGainedExperience@combatManager@@QA..` | register-homing (why-reg) | 88 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 77.49 | game | `?ClaimShipyard@game@@QAEXUtype_point@@H@Z` | inliner (predict-inline) | 218 | 16 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 5 over-inline |
| 78.26 | font | `?DrawCharacter@font@@QAEXHPAVBitmap16Bit@@HH..` | register-homing (why-reg) | 62 | 1 | spill to dead-parameter slot (B4) |
| 79.54 | armygrp | `?Merge@armyGroup@@QAEEPAV1@@Z` | control-flow (why-branch) | 135 | 27 | loop-form / merged-return placement / case order (D1-D9) |
| 79.89 | ai_player | `?make_gift@type_AI_player@@QAEXJ@Z` | inliner (predict-inline) | 333 | 46 | callee expanded on one side only (A8/A9/A12): 23 under-inline, 21 over-inline |
| 80.57 | ai | `?place_shooter@combatManager@@QAEXPBVarmy@@@Z` | inliner (predict-inline) | 46 | 12 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 80.76 | advmgr | `?DrawUnderlay@advManager@@QAEXHHHHH@Z` | inliner (predict-inline) | 380 | 3 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 3 over-inline |
| 81.04 | path | `?ValidAttack@army@@QAEHHHHHPAH@Z` | inliner (predict-inline) | 70 | 59 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 81.19 | town | `?can_build@town@@QBEEF@Z` | register-homing (why-reg) | 76 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 81.21 | town | `?get_buildable_mask@town@@QBE_JXZ` | register-homing (why-reg) | 56 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.49 | town | `?get_build_cost@town@@QBEFW4type_building_id..` | register-homing (why-reg) | 35 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.73 | town | `?get_legion_bonus@town@@QAEJJ@Z` | inliner (predict-inline) | 21 | 3 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 82.01 | advmgr | `?FindAdjacentMonster@advManager@@QAEEUtype_p..` | inliner (predict-inline) | 216 | 2 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 82.93 | mousemgr | `?SetPointer@mouseManager@@QAEXHW4EPointerSet..` | inliner (predict-inline) | 25 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 83.21 | advmgr | `?DrawAdvObjShadow@advManager@@QAEXHHHHH@Z` | inliner (predict-inline) | 501 | 0 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 3 over-inline |
| 84.38 | ai_tactical | `?get_hex_attack_value@type_AI_attack_hex_cho..` | inliner (predict-inline) | 94 | 1 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline |
| 84.41 | game | `?ClaimGarrison@game@@QAEXHH@Z` | inliner (predict-inline) | 10 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 85.43 | ai_combat | `?cast_spell@type_AI_combat_data@@QAEXAAV1@W4..` | inliner (predict-inline) | 323 | 26 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 85.58 | ai_tactical | `?should_attack_now@type_AI_spellcaster@@QAEE..` | inliner (predict-inline) | 68 | 0 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 3 over-inline |
| 85.70 | hero | `?can_summon_boat@hero@@QAEEXZ` | inliner (predict-inline) | 53 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 86.00 | recruit | `?GetMonsterCost@@YIXHPAH@Z` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 86.38 | ai_player | `?calculate_demand@type_AI_player@@QAEXXZ` | inliner (predict-inline) | 387 | 34 | callee expanded on one side only (A8/A9/A12): 10 under-inline, 9 over-inline |
| 86.38 | advmgr | `?DrawAdvObj@advManager@@QAEXHHHHH@Z` | inliner (predict-inline) | 863 | 2 | callee expanded on one side only (A8/A9/A12): 11 under-inline, 11 over-inline |
| 86.41 | iconwdgt | `?NextRandomSiegeEngineFrame@iconWidget@@QAEX..` | register-homing (why-reg) | 59 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 86.74 | town | `?initialize_spells@town@@QAEXPBVTownExtra@@@Z` | inliner (predict-inline) | 148 | 38 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 6 over-inline |
| 87.19 | smackmgr | `?VideoPlay@@YIHHHHHH@Z` | inliner (predict-inline) | 143 | 0 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 5 over-inline |
| 87.28 | advmgr | `?get_creature_bank_help_text@@YIXPADPAVNewma..` | inliner (predict-inline) | 69 | 3 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 87.29 | cmbtmgr | `?RemoveObstacle@combatManager@@QAEXH@Z` | register-homing (why-reg) | 23 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.73 | ai_tactical | `?find_enemy_attacks@type_AI_spellcaster@@QAE..` | inliner (predict-inline) | 128 | 4 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline |
| 88.02 | findpath | `?GetTerrainCost@@YIHPAVhero@@Utype_point@@HH..` | register-homing (why-reg) | 87 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 88.10 | button | `?Main@button@@UAEHPAVmessage@@@Z` | inliner (predict-inline) | 245 | 48 | callee expanded on one side only (A8/A9/A12): 18 under-inline, 14 over-inline |
| 88.17 | game | `?calculate_production@game@@QAEXXZ` | inliner (predict-inline) | 158 | 33 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 88.44 | button | `??0textButton@@QAE@HHHHHPBD00HHEHHH@Z` | inliner (predict-inline) | 55 | 2 | callee expanded on one side only (A8/A9/A12): 8 under-inline, 10 over-inline |
| 88.51 | armygrp | `?get_spell_work_chance@@YIMHW4TCreatureType@..` | control-flow (why-branch) | 236 | 141 | loop-form / merged-return placement / case order (D1-D9) |
| 88.91 | hero | `?GetManaFrame@hero@@QAEHXZ` | register-homing (why-reg) | 16 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.93 | smackmgr | `?VideoRealignBuffers@@YIXXZ` | unclassified | 6 | 0 | run why-reg / why-branch for the full search |
| 89.37 | cmbtmgr | `?LearnSpellFromEagleEye@combatManager@@QAEXH..` | inliner (predict-inline) | 8 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 89.46 | ai_combat | `?get_enchantment_value@type_AI_combat_data@@..` | inliner (predict-inline) | 107 | 2 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline |
| 89.50 | advmgr | `?GetSoundId@advManager@@QAE?AW4e_looping_sou..` | inliner (predict-inline) | 507 | 42 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 89.53 | ai_player | `?end_turn@type_AI_player@@QAEXXZ` | inliner (predict-inline) | 102 | 3 | callee expanded on one side only (A8/A9/A12): 11 under-inline, 11 over-inline |
| 89.63 | smackmgr | `?VideoDrawRects@@YIXXZ` | inliner (predict-inline) | 149 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 89.71 | advmgr | `?SetRolloverText@advManager@@QAEXPAVNewmapCe..` | inliner (predict-inline) | 803 | 560 | callee expanded on one side only (A8/A9/A12): 10 under-inline, 20 over-inline |
| 89.82 | findpath | `?SeedCombatPosition@searchArray@@QAEXPBVarmy..` | inliner (predict-inline) | 38 | 2 | callee expanded on one side only (A8/A9/A12): 6 under-inline, 6 over-inline |
| 90.42 | hero | `?GetMobilityFrame@hero@@QAEHXZ` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.83 | recruit | `?Update@recruitUnit@@QAEXEJ@Z` | register-homing (why-reg) | 144 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.93 | ai_combat | `?choose_melee@type_AI_combat_data@@QBE_NABV1..` | inliner (predict-inline) | 283 | 2 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 5 over-inline |
| 91.01 | ai_combat | `?initialize_creatures@type_AI_combat_data@@Q..` | inliner (predict-inline) | 334 | 2 | callee expanded on one side only (A8/A9/A12): 7 under-inline, 7 over-inline |
| 91.16 | advmgr | `?DrawGround@advManager@@QAEXHHHHH@Z` | inliner (predict-inline) | 140 | 0 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 2 over-inline |
| 91.22 | misc | `?Pick@TPickANumber@@QAEHXZ` | register-homing (why-reg) | 5 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.25 | advmgr | `?DrawShroud@advManager@@QAEXHHHHH@Z` | inliner (predict-inline) | 74 | 0 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 3 over-inline |
| 91.31 | advmgr | `?ProcessSearch@advManager@@QAEHHHH@Z` | inliner (predict-inline) | 253 | 0 | callee expanded on one side only (A8/A9/A12): 17 under-inline, 17 over-inline |
| 91.74 | font | `?LineLength@font@@QAEHPBDH@Z` | register-homing (why-reg) | 33 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.88 | window | `?CenterWindow@heroWindow@@QAEXHH@Z` | inliner (predict-inline) | 79 | 0 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 3 over-inline |
| 93.22 | cmbtmgr | `?RaiseSkeletons@combatManager@@QAEXH@Z` | inliner (predict-inline) | 4 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 93.37 | strip | `?DrawOwner@strip@@IAEXH@Z` | unclassified | 12 | 0 | run why-reg / why-branch for the full search |
| 93.87 | initialize | `?create_included_mask@@YIXPBHPA_J@Z` | unclassified | 6 | 0 | run why-reg / why-branch for the full search |
| 93.92 | button | `?Draw@textButton@@UAEXXZ` | register-homing (why-reg) | 7 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.07 | initialize | `?initialize_game_data@@YIXXZ` | register-homing (why-reg) | 120 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.17 | mousemgr | `?Update@mouseManager@@QAEXE@Z` | inliner (predict-inline) | 375 | 0 | callee expanded on one side only (A8/A9/A12): 9 under-inline, 9 over-inline |
| 94.76 | ai_combat | `?do_general_melee@type_AI_combat_data@@QAEXA..` | inliner (predict-inline) | 39 | 20 | callee expanded on one side only (A8/A9/A12): 1 under-inline |
| 95.03 | town | `?GiveSpells@town@@QAEXPAVhero@@@Z` | inliner (predict-inline) | 18 | 35 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 95.28 | ai_tactical | `?set_melee_enemies@type_AI_spellcaster@@QAEX..` | inliner (predict-inline) | 52 | 1 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline |
| 95.46 | cmbtmgr | `?InLineOfSight@combatManager@@QAEEHH@Z` | register-homing (why-reg) | 24 | 1 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 95.49 | ai_tactical | `?get_cure_value@type_AI_spellcaster@@QAEJPBV..` | inliner (predict-inline) | 30 | 0 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 5 over-inline |
| 95.74 | path | `?ValidPath@army@@QAEEHE@Z` | inliner (predict-inline) | 4 | 0 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 3 over-inline |
| 95.85 | game | `?GetName@playerData@@QAEPADXZ` | register-homing (why-reg) | 6 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.92 | smackmgr | `?VideoClose@@YIXXZ` | inliner (predict-inline) | 2 | 20 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 3 over-inline |
| 95.93 | misc | `?ReadPrefsFromRegistry@@YIXXZ` | inliner (predict-inline) | 151 | 2 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 5 over-inline |
| 96.35 | ai_tactical | `?get_hypnotize_value@type_AI_spellcaster@@QA..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.35 | ai | `?get_attack_change@combatManager@@QAEJPBVarm..` | inliner (predict-inline) | 50 | 0 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline |
| 96.43 | game | `??0SavedGameHeader@@QAE@XZ` | inliner (predict-inline) | 13 | 0 | callee expanded on one side only (A8/A9/A12): 10 under-inline, 10 over-inline |
| 96.46 | ai_player | `?calculate_reserve@type_AI_player@@QAEXXZ` | inliner (predict-inline) | 61 | 0 | callee expanded on one side only (A8/A9/A12): 8 under-inline, 8 over-inline |
| 96.46 | town | `?destroy_extra_capitol@town@@QAEXXZ` | inliner (predict-inline) | 34 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 96.53 | hero | `?DestroySiegeWeaponArtifact@hero@@QAEXH@Z` | register-homing (why-reg) | 17 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 96.56 | armygrp | `?GetArmyMorale@armyGroup@@QAEHHPBVhero@@PBVt..` | register-homing (why-reg) | 75 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.63 | font | `??1font@@UAE@XZ` | inliner (predict-inline) | 5 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 96.67 | recruit | `??0recruitUnit@@QAE@PAVhero@@W4TCreatureType..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.81 | font | `?DrawBoundedString@font@@QAEXPBDPAVBitmap16B..` | inliner (predict-inline) | 71 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 97.32 | ai_combat | `?do_aftermath@type_AI_combat_data@@QAEXPAV1@..` | inliner (predict-inline) | 4 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 97.97 | hero | `?TransferArtifacts@hero@@QAEXPAV1@@Z` | register-homing (why-reg) | 14 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 97.97 | recruit | `??0recruitUnit@@QAE@PAVarmyGroup@@EW4TCreatu..` | register-homing (why-reg) | 24 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.01 | advmgr | `?set_witch_hut_help_text@@YIXPADPAVhero@@PAV..` | register-homing (why-reg) | 19 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.10 | hero | `?GetExperienceIncrement@hero@@SIHH@Z` | register-homing (why-reg) | 23 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.17 | advmgr | `?DrawHeroPart@advManager@@QAEXHAAUTDrawParts..` | inliner (predict-inline) | 15 | 0 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 5 over-inline |
| 98.18 | advmgr | `?DrawHeroPartShadow@advManager@@QAEXHAAUTDra..` | inliner (predict-inline) | 15 | 0 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 5 over-inline |
| 98.38 | hero | `?GetHighestSchool@hero@@QAE?AW4TSpellSchool@..` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 98.40 | armygrp | `??0TSplitWindow@@QAE@HHH@Z` | inliner (predict-inline) | 19 | 0 | callee expanded on one side only (A8/A9/A12): 43 under-inline, 44 over-inline |
| 98.47 | ai | `?DoCompAI@combatManager@@QAEXH@Z` | inliner (predict-inline) | 1 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 98.50 | misc | `?CheckConfigFile@@YIXXZ` | register-homing (why-reg) | 47 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.57 | armygrp | `?GetMorale@armyGroup@@QAEHPBVhero@@PBVtown@@..` | inliner (predict-inline) | 13 | 0 | callee expanded on one side only (A8/A9/A12): 12 under-inline, 12 over-inline |
| 98.68 | hero | `?IsInIdentifyRange@hero@@QAEEPBUtype_point@@..` | register-homing (why-reg) | 2 | 0 | register-homing knob (B-family) |
| 98.75 | ai_player | `?can_take_town@@YIEPBVhero@@PBVtown@@@Z` | inliner (predict-inline) | 4 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 98.98 | game | `?GetNewHeroId@game@@QAEHHW4THeroClass@@E0@Z` | inliner (predict-inline) | 6 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 98.98 | ai_tactical | `?get_frenzy_value@type_AI_spellcaster@@QAEJP..` | inliner (predict-inline) | 6 | 0 | callee expanded on one side only (A8/A9/A12): 12 under-inline, 12 over-inline |
| 99.11 | ai_tactical | `?get_simple_attack_effect@type_AI_combat_par..` | inliner (predict-inline) | 32 | 0 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 3 over-inline |
| 99.31 | mousemgr | `?LoadFrame@mouseManager@@QAEXH@Z` | inliner (predict-inline) | 32 | 0 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline |
| 99.36 | advmgr | `?SetShrineHelpText@@YIXPADPAVhero@@PAVNewmap..` | unclassified | 3 | 1 | run why-reg / why-branch for the full search |
| 99.47 | recruit | `?siege_artifact_to_creature@@YI?AW4TCreature..` | register-homing (why-reg) | 12 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.53 | game | `?GetPlayerName@game@@QAEPADH@Z` | register-homing (why-reg) | 12 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.58 | hero | `?get_primary_skill_total@hero@@QAEFXZ` | register-homing (why-reg) | 2 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.64 | game | `?LoadBoatPool@game@@QAEHPAVTAbstractFile@@@Z` | inliner (predict-inline) | 14 | 0 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline |
| 99.70 | game | `?SaveRumours@game@@AAEHPAVTAbstractFile@@@Z` | inliner (predict-inline) | 4 | 0 | callee expanded on one side only (A8/A9/A12): 10 under-inline, 10 over-inline |
| 99.74 | advmgr | `?SetTreeHelpText@@YIXPADPAVhero@@PAVNewmapCe..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.78 | town | `?initialize_hordes@town@@SIXXZ` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.78 | soundmgr | `?MemorySample@soundManager@@QAEPAVds_memsamp..` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.85 | town | `?TownFn_005BF900@town@@QAEJJ@Z` | unclassified | 14 | 0 | run why-reg / why-branch for the full search |
| 99.85 | ai_tactical | `?get_attack_skill_value@type_AI_spellcaster@..` | inliner (predict-inline) | 52 | 0 | callee expanded on one side only (A8/A9/A12): 10 under-inline, 10 over-inline |
| 99.87 | ai_tactical | `?get_curse_value@type_AI_spellcaster@@QAEJPB..` | inliner (predict-inline) | 34 | 0 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline |
| 99.92 | armygrp | `?WindowHandler@TSplitWindow@@UAEHPAVmessage@..` | inliner (predict-inline) | 8 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline |
| 99.94 | game | `?LoadMinePool@game@@QAEHPAVTAbstractFile@@H@Z` | inliner (predict-inline) | 4 | 0 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline |
| 99.96 | ai_combat | `?AI_value_of_combat@@YIJPBVhero@@0ABVarmyGro..` | inliner (predict-inline) | 8 | 0 | callee expanded on one side only (A8/A9/A12): 6 under-inline, 6 over-inline |
| 99.97 | game | `?ClaimGenerator@game@@QAEXHH@Z` | inliner (predict-inline) | 4 | 0 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline |
| 99.97 | game | `?CreateBoat@game@@QAEHHHHHEC@Z` | inliner (predict-inline) | 4 | 0 | callee expanded on one side only (A8/A9/A12): 4 under-inline, 4 over-inline |
