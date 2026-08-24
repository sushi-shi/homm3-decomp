<!-- # generator: homm3.vc6.report | # date: 2026-08-24 | # ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - regenerate, never hand-edit | plateaus in [50.0, 99.999%); base-vs-delinked-target diagnosis, no recompiles -->
# vc6 plateau diagnosis (read-only; solvers propose, never land)

412 function(s). why-reg = register-homing knobs; why-branch = control-flow knobs; predict-inline = out-of-line CALL multiset divergence (a callee inlined on one side only - dominated by STL basic_string/vector ops + small dtors retail inlines and we do not). CALIBRATION 2026-08-19: this column USED to be dominated by a NAME artifact - retail's side names an unclaimed callee with a synth working label our compiled side can never emit, so one call booked as both an under- and an over-inline and the inliner route (which sits upstream of registers and blocks) buried the true diagnosis. inline_model.divergence now pairs those off by count: on the tree of that date the inliner class fell from 135 rows to 46 of 211, and register-homing (108) overtook it as the dominant plateau class. MECHANISM (RE'd, docs/vc6/inliner.md): /Ob2 budget = clamp(2*caller_cb,1000,35000) spent sequentially; our leaner reconstructions sit at the 1000 floor and STARVE, so retail inlines what we call. FIX = finish the caller's body (budget follows statement mass, byte-inert counts) - do NOT chase _Tidy/vector spellings or pragmas. So on LOW-% rows inline divergence largely self-resolves as reconstruction completes; it is the pure wall only on high-% rows. Mixed walls list both distances.

## Wall-class summary

- **198** register-homing (why-reg)
- **104** inliner (predict-inline)
- **82** control-flow (why-branch)
- **28** unclassified

| fuzzy | unit | function | wall class | reg-dist | flow-dist | knob to try |
|---|---|---|---|---|---|---|
| 56.51 | seerhut | `?DoProposalDialog@type_skill_quest@@UAEXPAVh..` | inliner (predict-inline) | 247 | 33 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 2 over-inline (11 name-unresolvable pair(s) discounted) |
| 60.30 | cmbtmgr | `?GenerateMap@combatManager@@QAEXXZ` | register-homing (why-reg) | 85 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 63.94 | advmgr | `?SetEnvironmentOrigin@advManager@@QAEXUtype_..` | register-homing (why-reg) | 231 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 70.54 | resourcemanager | `@game_sprite_1599e0@12` | inliner (predict-inline) | 298 | 32 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 3 over-inline (30 name-unresolvable pair(s) discounted) |
| 72.01 | iconwdgt | `?NextRandomFrame@iconWidget@@QAEXXZ` | control-flow (why-branch) | 172 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 74.09 | lodfile | `?Find@LODFile@@AAEXIIPBD@Z` | control-flow (why-branch) | 150 | 14 | loop-form / merged-return placement / case order (D1-D9) |
| 74.41 | viewarmywindow | `?WindowHandler@TViewArmyWindow@@UAEHPAVmessa..` | inliner (predict-inline) | 664 | 167 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 2 over-inline (16 name-unresolvable pair(s) discounted) |
| 74.62 | resourcemanager | `?Open@ResourceManager@@YI_N_N0PAH@Z` | inliner (predict-inline) | 224 | 24 | callee expanded on one side only (A8/A9/A12): 1 over-inline (8 name-unresolvable pair(s) discounted) |
| 75.01 | cmbtmgr | `?CalculateGainedExperience@combatManager@@QA..` | register-homing (why-reg) | 88 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 75.10 | hero | `?GiveArtifact@hero@@QAEEPBUtype_artifact@@EE..` | inliner (predict-inline) | 140 | 45 | callee expanded on one side only (A8/A9/A12): 3 under-inline (8 name-unresolvable pair(s) discounted) |
| 75.24 | seerhut | `?DoProgressDialog@type_artifact_quest@@UAEXXZ` | register-homing (why-reg) | 64 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 75.41 | hero | `?WindowHandler@THeroScreenWindow@@UAEHPAVmes..` | inliner (predict-inline) | 1104 | 342 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 13 over-inline (16 name-unresolvable pair(s) discounted) |
| 75.55 | cmbtmgr | `?place_obstacle@combatManager@@QAEEH@Z` | inliner (predict-inline) | 169 | 47 | callee expanded on one side only (A8/A9/A12): 1 under-inline (2 name-unresolvable pair(s) discounted) |
| 75.84 | seerhut | `?DoProgressDialog@type_skill_quest@@UAEXXZ` | control-flow (why-branch) | 88 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 75.86 | victorylossconditions | `?CheckForDefeatedHeroLoss@LossConditionStruc..` | control-flow (why-branch) | 143 | 162 | loop-form / merged-return placement / case order (D1-D9) |
| 76.91 | mapcell | `?readResourceData@NewfullMap@@QAEHPAVTAbstra..` | control-flow (why-branch) | 78 | 18 | loop-form / merged-return placement / case order (D1-D9) |
| 77.39 | resourcemanager | `@game_null_159510@12` | inliner (predict-inline) | 286 | 32 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 3 over-inline (31 name-unresolvable pair(s) discounted) |
| 77.49 | game | `?ClaimShipyard@game@@QAEXUtype_point@@H@Z` | control-flow (why-branch) | 218 | 16 | loop-form / merged-return placement / case order (D1-D9) |
| 77.73 | levelupwindow | `?WindowHandler@TLevelUpWindow@@UAEHPAVmessag..` | inliner (predict-inline) | 104 | 36 | callee expanded on one side only (A8/A9/A12): 3 over-inline (3 name-unresolvable pair(s) discounted) |
| 78.12 | remote | `??_GCNetMsgHandler@@UAEPAXI@Z` | register-homing (why-reg) | 6 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 78.26 | font | `?DrawCharacter@font@@QAEXHPAVBitmap16Bit@@HH..` | register-homing (why-reg) | 62 | 1 | spill to dead-parameter slot (B4) |
| 78.84 | game | `??1game@@QAE@XZ` | inliner (predict-inline) | 178 | 0 | callee expanded on one side only (A8/A9/A12): 2 over-inline (15 name-unresolvable pair(s) discounted) |
| 78.86 | hero | `?HeroFn_004E2840@hero@@QAEEJJ@Z` | inliner (predict-inline) | 86 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 78.90 | spells | `?ModifySpellDamage@combatManager@@QAEJJHPBVh..` | control-flow (why-branch) | 153 | 7 | loop-form / merged-return placement / case order (D1-D9) |
| 79.86 | mapcell | `?get_special_terrain@NewmapCell@@QBE?AW4TAdv..` | control-flow (why-branch) | 66 | 48 | loop-form / merged-return placement / case order (D1-D9) |
| 79.94 | ai | `?mark_moat@combatManager@@QAEXPBVarmy@@PAJPA..` | register-homing (why-reg) | 63 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 80.21 | ai_tactical | `?get_protection_value@type_AI_spellcaster@@Q..` | register-homing (why-reg) | 132 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 80.29 | university_window | `?purchase_click@type_university_window@@SIHP..` | inliner (predict-inline) | 80 | 30 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 80.40 | advmgr | `?ProcessKeyPress@advManager@@QAEHPBVmessage@..` | inliner (predict-inline) | 644 | 117 | callee expanded on one side only (A8/A9/A12): 7 over-inline (9 name-unresolvable pair(s) discounted) |
| 80.51 | artifact | `?InitializeArtifactTraitsTable@@YIEXZ` | control-flow (why-branch) | 289 | 38 | loop-form / merged-return placement / case order (D1-D9) |
| 80.93 | hero | `?update_spell_list@hero@@QAEXXZ` | register-homing (why-reg) | 95 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 81.04 | path | `?ValidAttack@army@@QBEHHHHHPAH@Z` | control-flow (why-branch) | 70 | 59 | loop-form / merged-return placement / case order (D1-D9) |
| 81.37 | command | `?is_computer_action@combatManager@@QAEEPBVar..` | inliner (predict-inline) | 44 | 52 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 81.41 | game | `?get_underground_gate_exit@game@@QAE?AUtype_..` | register-homing (why-reg) | 67 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.41 | townmgr | `?SetRolloverText@TTavernWindow@@QAEXH@Z` | register-homing (why-reg) | 65 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.49 | town | `?get_build_cost@town@@QBEFW4type_building_id..` | register-homing (why-reg) | 35 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.64 | campaignwindow | `?CampaignWindowHandler@@YIHAAVmessage@@@Z` | register-homing (why-reg) | 60 | 7 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 81.70 | hero | `?update_slot@THeroScreenWindow@@QAEXJ@Z` | control-flow (why-branch) | 110 | 10 | loop-form / merged-return placement / case order (D1-D9) |
| 81.73 | town | `?get_legion_bonus@town@@QAEJJ@Z` | control-flow (why-branch) | 21 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 81.76 | hero | `?GetMobility@hero@@QAEHE@Z` | control-flow (why-branch) | 212 | 14 | loop-form / merged-return placement / case order (D1-D9) |
| 82.29 | hillfortwindow | `?Recalculate@THillFortWindow@@QAEXE@Z` | inliner (predict-inline) | 240 | 2 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 82.34 | spells | `?MirrorImage@combatManager@@QAEXHH@Z` | control-flow (why-branch) | 220 | 71 | loop-form / merged-return placement / case order (D1-D9) |
| 82.49 | ai | `?place_shooter@combatManager@@QAEXPBVarmy@@@Z` | control-flow (why-branch) | 79 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 82.86 | hero | `?initialize@hero@@QAEXF@Z` | inliner (predict-inline) | 99 | 17 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 82.93 | mousemgr | `?SetPointer@mouseManager@@QAEXHW4EPointerSet..` | register-homing (why-reg) | 25 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 83.02 | advmgr | `?ShowRoute@advManager@@QAEXHHH@Z` | register-homing (why-reg) | 253 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 83.36 | combatoptionswindow | `?CombatOptionsWindowHandler@@YIHAAVmessage@@..` | inliner (predict-inline) | 156 | 22 | callee expanded on one side only (A8/A9/A12): 4 over-inline (3 name-unresolvable pair(s) discounted) |
| 83.43 | hero | `?CheckLevel@hero@@QAEXXZ` | inliner (predict-inline) | 110 | 41 | callee expanded on one side only (A8/A9/A12): 3 over-inline |
| 83.50 | advmgr | `?ProcessHover@advManager@@QAEHHH@Z` | inliner (predict-inline) | 492 | 105 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 83.95 | diff | `?MakeDiff@CDiffMaker@@QAEPAVCDiffFile@@AAK@Z` | register-homing (why-reg) | 214 | 1 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 84.00 | mapcell | `?insert@?$vector@VTreasureData@@V?$allocator..` | control-flow (why-branch) | 242 | 6 | loop-form / merged-return placement / case order (D1-D9) |
| 84.02 | remote | `?WaitForReadyToPlayMsg@@YIXXZ` | inliner (predict-inline) | 43 | 20 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline |
| 84.17 | diff | `?FindNextSame@CDiffMaker@@IAE_NHHAAH0@Z` | register-homing (why-reg) | 14 | 0 | register-homing knob (B-family) |
| 84.38 | ai_tactical | `?get_hex_attack_value@type_AI_attack_hex_cho..` | register-homing (why-reg) | 94 | 1 | spill to dead-parameter slot (B4) |
| 84.41 | game | `?ClaimGarrison@game@@QAEXHH@Z` | register-homing (why-reg) | 10 | 0 | register-homing knob (B-family) |
| 84.53 | soundmgr | `?Open@soundManager@@UAEHH@Z` | control-flow (why-branch) | 122 | 10 | loop-form / merged-return placement / case order (D1-D9) |
| 84.59 | ai_player | `?make_gift@type_AI_player@@QAEXJ@Z` | register-homing (why-reg) | 279 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 84.72 | cspriteframe | `?SetPixelFormat@CSpriteFrame@@SIXIII@Z` | register-homing (why-reg) | 44 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 84.83 | armygrp | `?Merge@armyGroup@@QAEEPAV1@@Z` | control-flow (why-branch) | 95 | 26 | loop-form / merged-return placement / case order (D1-D9) |
| 84.96 | spells | `?Earthquake@combatManager@@QAEXH@Z` | register-homing (why-reg) | 283 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 85.23 | advmgr | `?DrawAdvObjShadow@advManager@@QAEXHHHHH@Z` | register-homing (why-reg) | 415 | 0 | spill to dead-parameter slot (B4) |
| 85.63 | advmgr | `?ViewPuzzle@advManager@@QAEXXZ` | inliner (predict-inline) | 69 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 85.91 | fly | `?Fly@army@@QAEHH@Z` | register-homing (why-reg) | 311 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 85.98 | town | `?create_building@town@@QAE?AW4type_building_..` | inliner (predict-inline) | 270 | 1 | callee expanded on one side only (A8/A9/A12): 4 under-inline |
| 86.01 | army | `?GoBerserk@army@@QAEXXZ` | control-flow (why-branch) | 16 | 10 | loop-form / merged-return placement / case order (D1-D9) |
| 86.09 | hero | `?UpdateStats@hero@@QAEXXZ` | control-flow (why-branch) | 28 | 8 | loop-form / merged-return placement / case order (D1-D9) |
| 86.33 | search | `?BuildPath@searchArray@@QAEHPBVhero@@J@Z` | inliner (predict-inline) | 142 | 17 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (4 name-unresolvable pair(s) discounted) |
| 86.37 | advmgr | `?Close@advManager@@UAEXXZ` | inliner (predict-inline) | 151 | 4 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 86.41 | iconwdgt | `?NextRandomSiegeEngineFrame@iconWidget@@QAEX..` | register-homing (why-reg) | 59 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 86.66 | viewarmywindow | `?create_spell_influence_widgets@TViewArmyWin..` | control-flow (why-branch) | 165 | 14 | loop-form / merged-return placement / case order (D1-D9) |
| 86.68 | ai_tactical | `?should_attack_now@type_AI_spellcaster@@QAEE..` | register-homing (why-reg) | 58 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 86.74 | resourcemanager | `?LoadFontData@ResourceManager@@YIPAVfont@@PB..` | inliner (predict-inline) | 60 | 6 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 86.79 | game | `?GetRandomMonster@game@@QAE?AW4TCreatureType..` | inliner (predict-inline) | 95 | 24 | callee expanded on one side only (A8/A9/A12): 1 over-inline (26 name-unresolvable pair(s) discounted) |
| 86.93 | game | `?RandomizeEvents@game@@QAEXXZ` | inliner (predict-inline) | 1037 | 67 | callee expanded on one side only (A8/A9/A12): 3 over-inline (45 name-unresolvable pair(s) discounted) |
| 86.98 | remote | `?SendIt@CDPlayHeroes@@QAE_NPAVCNetMsg@@K_N@Z` | control-flow (why-branch) | 32 | 12 | loop-form / merged-return placement / case order (D1-D9) |
| 86.99 | hero | `?can_summon_boat@hero@@QAEEXZ` | register-homing (why-reg) | 41 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.04 | ai_combat | `?cast_spell@type_AI_combat_data@@QAEXAAV1@W4..` | register-homing (why-reg) | 305 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.10 | seerhut | `?DoProposalDialog@type_resource_quest@@UAEXP..` | register-homing (why-reg) | 106 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.11 | ai | `?find_attack_hexes@@YIXPBVarmy@@JJJJPBVsearc..` | inliner (predict-inline) | 110 | 29 | callee expanded on one side only (A8/A9/A12): 1 over-inline (8 name-unresolvable pair(s) discounted) |
| 87.19 | smackmgr | `?VideoPlay@@YIHHHHHH@Z` | register-homing (why-reg) | 143 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 87.27 | hero | `?HeroFn_004DC100@hero@@QAEXJ@Z` | inliner (predict-inline) | 70 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline (5 name-unresolvable pair(s) discounted) |
| 87.55 | findpath | `?PushPoint@searchArray@@QAEXPBUpathCell@@PAU..` | inliner (predict-inline) | 320 | 116 | callee expanded on one side only (A8/A9/A12): 2 under-inline (19 name-unresolvable pair(s) discounted) |
| 87.59 | advmgr | `?DrawAdvObj@advManager@@QAEXHHHHH@Z` | register-homing (why-reg) | 734 | 2 | spill to dead-parameter slot (B4) |
| 87.65 | findpath | `?FindCombatPath@searchArray@@QAEEPBVarmy@@JJ..` | inliner (predict-inline) | 340 | 22 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (7 name-unresolvable pair(s) discounted) |
| 87.73 | ai_tactical | `?find_enemy_attacks@type_AI_spellcaster@@QAE..` | register-homing (why-reg) | 128 | 4 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.74 | hero | `?GetLuck@hero@@QAEHPBV1@EE@Z` | control-flow (why-branch) | 52 | 23 | loop-form / merged-return placement / case order (D1-D9) |
| 87.77 | command | `?GetControl@combatManager@@QAEXXZ` | control-flow (why-branch) | 70 | 50 | loop-form / merged-return placement / case order (D1-D9) |
| 87.85 | game | `??0game@@QAE@XZ` | inliner (predict-inline) | 65 | 3 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (5 name-unresolvable pair(s) discounted) |
| 87.94 | hero | `?load@hero@@QAEHPAVTAbstractFile@@H@Z` | inliner (predict-inline) | 155 | 21 | callee expanded on one side only (A8/A9/A12): 4 under-inline (5 name-unresolvable pair(s) discounted) |
| 88.04 | mapcell | `?erase@?$vector@VTreasureData@@V?$allocator@..` | register-homing (why-reg) | 11 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.10 | button | `?Main@button@@UAEHPAVmessage@@@Z` | inliner (predict-inline) | 245 | 48 | callee expanded on one side only (A8/A9/A12): 4 under-inline (4 name-unresolvable pair(s) discounted) |
| 88.14 | winmgr | `?FadeFromBlack@heroWindowManager@@QAEXH@Z` | register-homing (why-reg) | 108 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.34 | ai_player | `?choose_weakest_army@type_AI_creature_swappe..` | register-homing (why-reg) | 119 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.40 | hero | `?get_description@type_artifact@@QAE?AV?$basi..` | control-flow (why-branch) | 100 | 7 | loop-form / merged-return placement / case order (D1-D9) |
| 88.44 | button | `??0textButton@@QAE@HHHHHPBD00HHEHHH@Z` | inliner (predict-inline) | 53 | 2 | callee expanded on one side only (A8/A9/A12): 2 over-inline (1 name-unresolvable pair(s) discounted) |
| 88.44 | slider | `?SetKnob@slider@@IAEXH@Z` | control-flow (why-branch) | 20 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 88.49 | spells | `?ValidSpellTarget@combatManager@@QAEEHJJJEJ@Z` | control-flow (why-branch) | 138 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 88.51 | armygrp | `?get_spell_work_chance@@YIMHW4TCreatureType@..` | control-flow (why-branch) | 236 | 141 | loop-form / merged-return placement / case order (D1-D9) |
| 88.51 | winmgr | `?FadeToBlack@heroWindowManager@@QAEXHE@Z` | register-homing (why-reg) | 128 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.55 | advmgr | `?DoAdvCommand@advManager@@QAEPAVNewmapCell@@..` | control-flow (why-branch) | 435 | 11 | loop-form / merged-return placement / case order (D1-D9) |
| 88.74 | seerhut | `?DoProposalDialog@type_artifact_quest@@UAEXP..` | inliner (predict-inline) | 67 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 3 over-inline (12 name-unresolvable pair(s) discounted) |
| 88.86 | resourcemanager | `?GetSprite@ResourceManager@@YIPAVCSprite@@PB..` | register-homing (why-reg) | 443 | 1 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 88.93 | smackmgr | `?VideoRealignBuffers@@YIXXZ` | unclassified | 6 | 0 | run why-reg / why-branch for the full search |
| 89.04 | game | `?ValidateVictoryLossConditions@game@@QAEXE@Z` | control-flow (why-branch) | 110 | 282 | loop-form / merged-return placement / case order (D1-D9) |
| 89.09 | ai_player | `?value_of_adding_army@type_AI_creature_swapp..` | register-homing (why-reg) | 155 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.09 | ai_tactical | `?consider_sacrifice@type_AI_spellcaster@@QBE..` | control-flow (why-branch) | 118 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 89.14 | fly | `?ValidFlight@army@@QBEEHE@Z` | control-flow (why-branch) | 46 | 24 | loop-form / merged-return placement / case order (D1-D9) |
| 89.28 | hero | `??0hero@@QAE@XZ` | register-homing (why-reg) | 53 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.34 | cmbtmgr | `?ShootBallisticMissile@combatManager@@QAEXHH..` | register-homing (why-reg) | 102 | 1 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 89.46 | ai_combat | `?get_enchantment_value@type_AI_combat_data@@..` | register-homing (why-reg) | 107 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.53 | ai_player | `?end_turn@type_AI_player@@QAEXXZ` | control-flow (why-branch) | 102 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 89.59 | town | `?get_buildable_mask@town@@QBE_JXZ` | register-homing (why-reg) | 24 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.60 | town | `?can_build@town@@QBEEF@Z` | register-homing (why-reg) | 87 | 0 | spill to dead-parameter slot (B4) |
| 89.63 | smackmgr | `?VideoDrawRects@@YIXXZ` | register-homing (why-reg) | 149 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.69 | ai | `?choose_melee_target@combatManager@@QAEEPBVa..` | inliner (predict-inline) | 337 | 28 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 89.72 | townmgr | `?WindowHandler@TTavernWindow@@UAEHPAVmessage..` | register-homing (why-reg) | 80 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.82 | findpath | `?SeedCombatPosition@searchArray@@QAEXPBVarmy..` | register-homing (why-reg) | 38 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.88 | viewarmywindow | `?create_dismiss_widget@TViewArmyWindow@@QAEX..` | control-flow (why-branch) | 31 | 20 | loop-form / merged-return placement / case order (D1-D9) |
| 89.88 | viewarmywindow | `?create_upgrade_widget@TViewArmyWindow@@QAEX..` | control-flow (why-branch) | 31 | 20 | loop-form / merged-return placement / case order (D1-D9) |
| 90.08 | advmgr | `?get_creature_bank_help_text@@YIXPADPAVNewma..` | control-flow (why-branch) | 50 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 90.19 | armygrp | `?get_luck_description@armyGroup@@QBE?AV?$bas..` | register-homing (why-reg) | 105 | 1 | spill to dead-parameter slot (B4) |
| 90.20 | hero | `?remove_artifact@hero@@QAEXJ@Z` | register-homing (why-reg) | 35 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.26 | findpath | `?PushCombatPoint@searchArray@@QAEXHHHHH@Z` | control-flow (why-branch) | 85 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 90.30 | advmgr | `??0advManager@@QAE@XZ` | register-homing (why-reg) | 25 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.43 | mapcell | `?StampObject@NewfullMap@@QAEXPAVNewmapCell@@..` | control-flow (why-branch) | 164 | 48 | loop-form / merged-return placement / case order (D1-D9) |
| 90.47 | viewarmywindow | `??0TViewArmyWindow@@QAE@PAVarmyGroup@@HPBVhe..` | inliner (predict-inline) | 147 | 32 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (21 name-unresolvable pair(s) discounted) |
| 90.49 | mapcell | `?NewfullMapFn_004FD950@NewfullMap@@QAEXPAVTA..` | register-homing (why-reg) | 99 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.58 | spells | `?Resurrect@combatManager@@QAEXPAVarmy@@JE@Z` | register-homing (why-reg) | 62 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.65 | advmgr | `?UpdateRadar@advManager@@QAEXUtype_point@@EE..` | control-flow (why-branch) | 532 | 13 | loop-form / merged-return placement / case order (D1-D9) |
| 90.70 | hiscore | `??0THighScoreWindow@@QAE@XZ` | register-homing (why-reg) | 170 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.72 | mapcell | `?erase@?$vector@VMonsterData@@V?$allocator@V..` | inliner (predict-inline) | 13 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 90.75 | townmgr | `?DoBlacksmith@@YIXHH@Z` | register-homing (why-reg) | 34 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.81 | viewarmywindow | `??0TViewArmyWindow@@QAE@PBVarmy@@HHE@Z` | inliner (predict-inline) | 174 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (25 name-unresolvable pair(s) discounted) |
| 90.84 | recruit | `?Update@recruitUnit@@QAEXEJ@Z` | register-homing (why-reg) | 140 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.88 | mapcell | `?get_trigger_cell@NewmapCell@@QAEPAV1@XZ` | register-homing (why-reg) | 37 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 90.90 | town | `?give_event_reward@town@@QAEXPBVTTownEvent@@..` | inliner (predict-inline) | 128 | 5 | callee expanded on one side only (A8/A9/A12): 1 under-inline (2 name-unresolvable pair(s) discounted) |
| 90.93 | townmgr | `?UpdateTownLocator@TTownScreenWindow@@QAEXH@Z` | register-homing (why-reg) | 11 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.93 | ai_combat | `?choose_melee@type_AI_combat_data@@QBE_NABV1..` | inliner (predict-inline) | 283 | 2 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 2 over-inline (2 name-unresolvable pair(s) discounted) |
| 91.01 | ai_combat | `?initialize_creatures@type_AI_combat_data@@Q..` | register-homing (why-reg) | 334 | 2 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 91.10 | cmbtmgr | `?Unnamed464d40@combatManager@@QAEEPAVarmy@@@Z` | register-homing (why-reg) | 47 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.10 | advmgr | `?ProcessMapSelect@advManager@@QAEXPBVmessage..` | inliner (predict-inline) | 150 | 57 | callee expanded on one side only (A8/A9/A12): 1 under-inline |
| 91.10 | spells | `?GetNextChainLightningTarget@combatManager@@..` | register-homing (why-reg) | 58 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.11 | remote | `??1CNetMsgHandlerPause@@UAE@XZ` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.19 | mapcell | `?erase@?$vector@VCObjectType@@V?$allocator@V..` | inliner (predict-inline) | 57 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 91.30 | bottomviewsubwindow | `??0TBottomViewResourceMessage@@QAE@PAVheroWi..` | register-homing (why-reg) | 40 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.46 | army | `?attack_wall@army@@QAEXW4TWallTargetId@@J@Z` | inliner (predict-inline) | 155 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 91.60 | victorylossconditions | `?CheckForArtifactWin@VictoryConditionStruct@..` | register-homing (why-reg) | 75 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.61 | mapcell | `?readHeroData@NewfullMap@@QAEHPAVTAbstractFi..` | register-homing (why-reg) | 393 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 91.65 | hero | `?get_morale_description@hero@@QBE?AV?$basic_..` | inliner (predict-inline) | 104 | 76 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 1 over-inline (28 name-unresolvable pair(s) discounted) |
| 91.71 | army | `?Turn@army@@QAEXE@Z` | register-homing (why-reg) | 16 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 91.89 | mapcell | `?Save@NewfullMap@@QAEHPAVTAbstractFile@@HE@Z` | inliner (predict-inline) | 116 | 63 | callee expanded on one side only (A8/A9/A12): 2 over-inline |
| 91.94 | hillfortwindow | `?HillFortWindowHandler@@YIHAAVmessage@@@Z` | control-flow (why-branch) | 60 | 23 | loop-form / merged-return placement / case order (D1-D9) |
| 91.98 | border | `?Main@border@@UAEHPAVmessage@@@Z` | control-flow (why-branch) | 37 | 28 | loop-form / merged-return placement / case order (D1-D9) |
| 92.00 | soundmgr | `?WaitEndSampleThread@@YAXPAX@Z` | register-homing (why-reg) | 28 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.15 | quickherowindow | `??0TQuickHeroWindow@@QAE@PAVhero@@W4TViewLev..` | inliner (predict-inline) | 259 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (35 name-unresolvable pair(s) discounted) |
| 92.16 | army | `?get_unit_combat_value@army@@QBENJJEPBV1@@Z` | register-homing (why-reg) | 112 | 1 | spill to dead-parameter slot (B4) |
| 92.20 | mapcell | `?Load@NewfullMap@@QAEHPAVTAbstractFile@@HEH@Z` | inliner (predict-inline) | 53 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (12 name-unresolvable pair(s) discounted) |
| 92.21 | army | `?compute_attacker_bonus@army@@QBEHHEPAV1@EJ@Z` | inliner (predict-inline) | 172 | 58 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 92.41 | game | `?Load@game@@QAEHPAVTAbstractFile@@@Z` | inliner (predict-inline) | 539 | 62 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 2 over-inline (24 name-unresolvable pair(s) discounted) |
| 92.50 | systemoptionswindow | `?WindowHandler@TSystemOptionsWindow@@UAEHPAV..` | control-flow (why-branch) | 115 | 72 | loop-form / merged-return placement / case order (D1-D9) |
| 92.52 | army | `?get_berserk_targets@army@@QBEXAAV?$vector@P..` | register-homing (why-reg) | 53 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.57 | victorylossconditions | `?IsGrailTarget@VictoryConditionStruct@@QAEEP..` | register-homing (why-reg) | 22 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.57 | command | `?GetCommand@combatManager@@QAEHH@Z` | register-homing (why-reg) | 104 | 1 | spill to dead-parameter slot (B4) |
| 92.63 | army | `?InitClean@army@@QAEXXZ` | register-homing (why-reg) | 62 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.68 | font | `?LineLength@font@@QAEHPBDH@Z` | register-homing (why-reg) | 17 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.68 | advmgr | `?BVMessage@advManager@@QAEXPBD@Z` | inliner (predict-inline) | 13 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 92.70 | army | `?spell_is_valid_on_target@@YIEHPBVarmy@@@Z` | inliner (predict-inline) | 79 | 15 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 92.70 | advmgr | `?DrawUnderlay@advManager@@QAEXHHHHH@Z` | register-homing (why-reg) | 223 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.88 | window | `?CenterWindow@heroWindow@@QAEXHH@Z` | register-homing (why-reg) | 79 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.92 | hero | `?HeroFn_004E2550@hero@@QAEEJJ@Z` | control-flow (why-branch) | 65 | 7 | loop-form / merged-return placement / case order (D1-D9) |
| 92.94 | advmgr | `?TownQuickView@advManager@@QAEXHHHE@Z` | inliner (predict-inline) | 141 | 0 | callee expanded on one side only (A8/A9/A12): 2 over-inline (17 name-unresolvable pair(s) discounted) |
| 93.01 | mapcell | `?readBlackBox@NewfullMap@@QAEHPAVTAbstractFi..` | control-flow (why-branch) | 304 | 28 | loop-form / merged-return placement / case order (D1-D9) |
| 93.06 | armygrp | `?get_morale_description@armyGroup@@QBE?AV?$b..` | inliner (predict-inline) | 362 | 94 | callee expanded on one side only (A8/A9/A12): 1 under-inline (26 name-unresolvable pair(s) discounted) |
| 93.16 | mainmenu | `?MainMenuHandler@@YIHAAVmessage@@@Z` | inliner (predict-inline) | 95 | 9 | callee expanded on one side only (A8/A9/A12): 1 under-inline (6 name-unresolvable pair(s) discounted) |
| 93.17 | army | `?animate_missile@army@@QAEXPAV1@@Z` | register-homing (why-reg) | 208 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.18 | ai_tactical | `??0type_AI_spellcaster@@QAE@PAVcombatManager..` | register-homing (why-reg) | 67 | 0 | spill to dead-parameter slot (B4) |
| 93.21 | hero | `?equip_artifact@hero@@QAEEPBUtype_artifact@@..` | inliner (predict-inline) | 16 | 7 | callee expanded on one side only (A8/A9/A12): 1 under-inline (1 name-unresolvable pair(s) discounted) |
| 93.21 | advmgr | `?get_army_help_text@@YI?AV?$basic_string@DU?..` | control-flow (why-branch) | 29 | 44 | loop-form / merged-return placement / case order (D1-D9) |
| 93.22 | cmbtmgr | `?RaiseSkeletons@combatManager@@QAEXH@Z` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 93.32 | army | `?WalkTo@army@@QAEEHE@Z` | register-homing (why-reg) | 150 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.32 | mapcell | `?readSpellScrollData@NewfullMap@@QAEHPAVTAbs..` | register-homing (why-reg) | 60 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.33 | advmgr | `?BVResMsg@advManager@@QAEXPBDHH@Z` | inliner (predict-inline) | 13 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 93.37 | strip | `?DrawOwner@strip@@IAEXH@Z` | unclassified | 12 | 0 | run why-reg / why-branch for the full search |
| 93.44 | spells | `?SummonElemental@combatManager@@QAEXHW4TCrea..` | inliner (predict-inline) | 17 | 29 | callee expanded on one side only (A8/A9/A12): 1 under-inline (3 name-unresolvable pair(s) discounted) |
| 93.48 | mapcell | `?get_map_object@NewmapCell@@QAE?AW4TAdventur..` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.51 | cmbtmgr | `?ShootAnimatedMissile@combatManager@@QAEXHHH..` | register-homing (why-reg) | 97 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.55 | hero | `?HeroFn_004D97F0@hero@@QAEXXZ` | register-homing (why-reg) | 49 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.66 | mapcell | `?CalculateCellExtra@NewfullMap@@QAEXPAVNewma..` | control-flow (why-branch) | 6 | 21 | loop-form / merged-return placement / case order (D1-D9) |
| 93.70 | townmgr | `?DoUniversity@townManager@@QAEXXZ` | register-homing (why-reg) | 31 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.71 | hero | `?get_luck_description@hero@@QBE?AV?$basic_st..` | inliner (predict-inline) | 84 | 66 | callee expanded on one side only (A8/A9/A12): 2 under-inline (26 name-unresolvable pair(s) discounted) |
| 93.77 | cspriteframe | `?DrawCreatureImpl@CSpriteFrame@@QBEXHHHHPAGH..` | control-flow (why-branch) | 260 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 93.81 | advmgr | `?SetTownContext@advManager@@QAEXHEE@Z` | register-homing (why-reg) | 52 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.81 | game | `?SaveGame@game@@QAEEPBDEEEE@Z` | register-homing (why-reg) | 92 | 0 | spill to dead-parameter slot (B4) |
| 93.87 | initialize | `?create_included_mask@@YIXPBHPA_J@Z` | unclassified | 6 | 0 | run why-reg / why-branch for the full search |
| 93.88 | townmgr | `??0type_monster_join_window@@QAE@PAVhero@@PA..` | control-flow (why-branch) | 43 | 24 | loop-form / merged-return placement / case order (D1-D9) |
| 93.90 | spells | `?Armageddon@combatManager@@QAEXHH@Z` | inliner (predict-inline) | 255 | 1 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 93.96 | hero | `?mark_spells@@YI?AV?$bitset@$0EG@@std@@H@Z` | inliner (predict-inline) | 65 | 28 | callee expanded on one side only (A8/A9/A12): 1 over-inline (11 name-unresolvable pair(s) discounted) |
| 93.96 | advmgr | `?SetRolloverText@advManager@@QAEXPAVNewmapCe..` | inliner (predict-inline) | 705 | 350 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 93.98 | cmbtmgr | `?ShootMissile@combatManager@@QAEXHHHHPBMPBVC..` | register-homing (why-reg) | 76 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.06 | mapcell | `??4TTownEvent@@QAEAAV0@ABV0@@Z` | control-flow (why-branch) | 17 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 94.08 | army | `?attack_hex@army@@QAEEHE@Z` | control-flow (why-branch) | 37 | 7 | loop-form / merged-return placement / case order (D1-D9) |
| 94.17 | mousemgr | `?Update@mouseManager@@QAEXE@Z` | register-homing (why-reg) | 375 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 94.17 | hero | `?HeroFn_004D9CC0@hero@@QAEHH@Z` | register-homing (why-reg) | 16 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.25 | hero | `?Fly@hero@@QAEXH@Z` | register-homing (why-reg) | 31 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 94.35 | cspriteframe | `?Draw@CSpriteFrame@@QBEXHHHHPAGHHHHHAAVTPale..` | register-homing (why-reg) | 169 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 94.36 | mapcell | `??4MonsterData@@QAEAAV0@ABV0@@Z` | control-flow (why-branch) | 19 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 94.41 | game | `?calculate_production@game@@QAEXXZ` | control-flow (why-branch) | 111 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 94.43 | resourcemanager | `?GetBitmap816@ResourceManager@@YIPAVBitmap81..` | inliner (predict-inline) | 185 | 23 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (11 name-unresolvable pair(s) discounted) |
| 94.43 | command | `?CheckGetAIMove@combatManager@@QAEXXZ` | register-homing (why-reg) | 110 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 94.51 | advmgr | `?GetSoundId@advManager@@QAE?AW4e_looping_sou..` | control-flow (why-branch) | 468 | 32 | loop-form / merged-return placement / case order (D1-D9) |
| 94.52 | ai | `?SOD_choose_faerie_dragon_spell@combatManage..` | register-homing (why-reg) | 28 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.54 | ai | `?mark_multiheaded_enemy@combatManager@@QAEXP..` | inliner (predict-inline) | 64 | 42 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 94.67 | hero | `?HeroFn_004D8B30@hero@@QAEXPBVHeroExtra@@@Z` | register-homing (why-reg) | 54 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.68 | advmgr | `?ProcessSearch@advManager@@QAEHHHH@Z` | register-homing (why-reg) | 88 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 94.70 | resourcemanager | `?GetPalette24@ResourceManager@@YIPAVTPalette..` | inliner (predict-inline) | 104 | 17 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (6 name-unresolvable pair(s) discounted) |
| 94.73 | game | `?GetRandomArtifactId@game@@QAE?AW4TArtifact@..` | register-homing (why-reg) | 27 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 94.79 | ai_combat | `?do_general_melee@type_AI_combat_data@@QAEXA..` | inliner (predict-inline) | 35 | 20 | callee expanded on one side only (A8/A9/A12): 1 under-inline |
| 94.80 | hero | `?GetMorale@hero@@QAEHPBV1@EE@Z` | control-flow (why-branch) | 30 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 94.86 | mapcell | `?readMapObjects@NewfullMap@@QAEHPAVTAbstract..` | inliner (predict-inline) | 190 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (29 name-unresolvable pair(s) discounted) |
| 94.87 | advmgr | `?QuickInfo@advManager@@QAEXHHH@Z` | control-flow (why-branch) | 715 | 398 | loop-form / merged-return placement / case order (D1-D9) |
| 94.91 | spells | `?DrawBolt@combatManager@@QAEXPAUSBolt@@H@Z` | register-homing (why-reg) | 125 | 1 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 94.95 | army | `?DrawToBuffer@army@@QAEXHHH@Z` | register-homing (why-reg) | 191 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 95.00 | mapcell | `?loadBlackBox@NewfullMap@@QAEHPAVTAbstractFi..` | control-flow (why-branch) | 84 | 9 | loop-form / merged-return placement / case order (D1-D9) |
| 95.03 | cmbtmgr | `?KeepAttack@combatManager@@QAEXH@Z` | register-homing (why-reg) | 64 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.04 | mapcell | `?erase@?$vector@UTScenarioTown@@V?$allocator..` | register-homing (why-reg) | 45 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.12 | kb | `?GetNextHumanPlayer@@YIHH@Z` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.12 | townmgr | `?BuyBuild@townManager@@QAEHHHH@Z` | control-flow (why-branch) | 343 | 56 | loop-form / merged-return placement / case order (D1-D9) |
| 95.16 | mapcell | `?readTownData@NewfullMap@@QAEHPAVTAbstractFi..` | control-flow (why-branch) | 457 | 26 | loop-form / merged-return placement / case order (D1-D9) |
| 95.19 | slider | `?Main@slider@@UAEHPAVmessage@@@Z` | control-flow (why-branch) | 57 | 48 | loop-form / merged-return placement / case order (D1-D9) |
| 95.27 | iconwdgt | `?Main@iconWidget@@UAEHPAVmessage@@@Z` | control-flow (why-branch) | 48 | 30 | loop-form / merged-return placement / case order (D1-D9) |
| 95.28 | ai_tactical | `?set_melee_enemies@type_AI_spellcaster@@QAEX..` | register-homing (why-reg) | 52 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.30 | remote | `?OnPlayerDropUpdateMsg@@YIXK@Z` | inliner (predict-inline) | 38 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (5 name-unresolvable pair(s) discounted) |
| 95.44 | town | `?GiveSpells@town@@QAEXPAVhero@@@Z` | control-flow (why-branch) | 12 | 35 | loop-form / merged-return placement / case order (D1-D9) |
| 95.47 | mapcell | `?readMapLayer@NewfullMap@@QAEHPAVTAbstractFi..` | register-homing (why-reg) | 46 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.49 | ai_tactical | `?get_cure_value@type_AI_spellcaster@@QAEJPBV..` | register-homing (why-reg) | 30 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.51 | drawing | `?DrawBackground@combatManager@@QAEXXZ` | inliner (predict-inline) | 30 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 95.52 | army | `?do_attack@army@@QAEEPAV1@H@Z` | control-flow (why-branch) | 76 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 95.55 | ai_tactical | `?consider_teleport@type_AI_spellcaster@@QAEX..` | control-flow (why-branch) | 25 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 95.63 | resourcemanager | `?LoadFont@ResourceManager@@YIPAVfont@@PBD@Z` | inliner (predict-inline) | 43 | 17 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 95.66 | townmgr | `?WindowHandler@type_garrison_base_window@@UA..` | register-homing (why-reg) | 106 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 95.69 | townmgr | `?SetCommandAndText@type_garrison_base_window..` | register-homing (why-reg) | 62 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.69 | resourcemanager | `?LoadPalette@ResourceManager@@YIPAVTPalette1..` | inliner (predict-inline) | 75 | 17 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (8 name-unresolvable pair(s) discounted) |
| 95.70 | mapcell | `?readScholarData@NewfullMap@@QAEHPAVTAbstrac..` | register-homing (why-reg) | 43 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.70 | army | `?range_attack@army@@QAEXPAV1@@Z` | register-homing (why-reg) | 105 | 1 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 95.72 | cmbtmgr | `?InitNonVisualVars@combatManager@@QAEXXZ` | control-flow (why-branch) | 65 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 95.78 | ai | `?choose_resurrect_action@combatManager@@QAEE..` | inliner (predict-inline) | 54 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline (3 name-unresolvable pair(s) discounted) |
| 95.88 | mapcell | `??1NewfullMap@@QAE@XZ` | inliner (predict-inline) | 16 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (8 name-unresolvable pair(s) discounted) |
| 95.89 | ai | `?choose_creature_spell@combatManager@@QAEEPB..` | register-homing (why-reg) | 21 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.92 | smackmgr | `?VideoClose@@YIXXZ` | control-flow (why-branch) | 2 | 20 | loop-form / merged-return placement / case order (D1-D9) |
| 95.93 | misc | `?ReadPrefsFromRegistry@@YIXXZ` | inliner (predict-inline) | 151 | 2 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 96.06 | ai_tactical | `?get_fortune_value@type_AI_spellcaster@@QAEJ..` | register-homing (why-reg) | 166 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 96.15 | advmgr | `?ProcessWaitingHover@advManager@@QAEHHH@Z` | register-homing (why-reg) | 23 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.16 | mapcell | `?is_diggable@NewmapCell@@QAEEXZ` | control-flow (why-branch) | 3 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 96.19 | spells | `?ChainLightning@combatManager@@QAEXHHH@Z` | control-flow (why-branch) | 39 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 96.22 | cmbtmgr | `?PowEffect@combatManager@@QAEXHH@Z` | register-homing (why-reg) | 207 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.25 | army | `?initialize@army@@QAEXHJPBVhero@@JJJ@Z` | register-homing (why-reg) | 28 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.26 | mapcell | `??4TTimedEvent@@QAEAAV0@ABV0@@Z` | control-flow (why-branch) | 13 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 96.26 | mapcell | `?upgrade_cell_extra_info@@YIXPAVNewmapCell@@..` | control-flow (why-branch) | 158 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 96.27 | army | `?CancelIndividualSpell@army@@QAEXH@Z` | inliner (predict-inline) | 66 | 32 | callee expanded on one side only (A8/A9/A12): 1 over-inline (7 name-unresolvable pair(s) discounted) |
| 96.30 | seerhut | `?read@TQuestGuard@@QAEHPAVTAbstractFile@@@Z` | unclassified | 1 | 0 | run why-reg / why-branch for the full search |
| 96.35 | ai_tactical | `?get_hypnotize_value@type_AI_spellcaster@@QA..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.35 | ai | `?get_attack_change@combatManager@@QAEJPBVarm..` | register-homing (why-reg) | 50 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.37 | spells | `?ShowMassSpell@combatManager@@QAEXPAY0BE@$$C..` | register-homing (why-reg) | 74 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.40 | combatcontrolsubwindow | `??0TCombatControlSubWindow@@QAE@PAVheroWindo..` | register-homing (why-reg) | 16 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.40 | game | `?Save@game@@QAEHPAVTAbstractFile@@@Z` | inliner (predict-inline) | 262 | 2 | callee expanded on one side only (A8/A9/A12): 1 under-inline (13 name-unresolvable pair(s) discounted) |
| 96.43 | townmgr | `?Recruit@TCastleWindow@@QAEXH@Z` | register-homing (why-reg) | 36 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.45 | cmbtmgr | `?SetupCombat@combatManager@@QAEXUtype_point@..` | register-homing (why-reg) | 26 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.46 | ai_player | `?calculate_reserve@type_AI_player@@QAEXXZ` | register-homing (why-reg) | 61 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.46 | town | `?destroy_extra_capitol@town@@QAEXXZ` | register-homing (why-reg) | 34 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.47 | mapcell | `?readMonsterData@NewfullMap@@QAEHPAVTAbstrac..` | inliner (predict-inline) | 122 | 1 | callee expanded on one side only (A8/A9/A12): 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 96.50 | mapcell | `?copy@std@@YIPAVTTimedEvent@@PAV2@00@Z` | control-flow (why-branch) | 13 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 96.53 | hero | `?DestroySiegeWeaponArtifact@hero@@QAEXH@Z` | register-homing (why-reg) | 17 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 96.53 | seerhut | `?SetDefaultText@type_skill_quest@@UAEXXZ` | register-homing (why-reg) | 21 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.56 | armygrp | `?GetArmyMorale@armyGroup@@QAEHHPBVhero@@PBVt..` | register-homing (why-reg) | 75 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.63 | remote | `?KillOldChat@CChatManager@@QAEXXZ` | register-homing (why-reg) | 28 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.63 | remote | `?AddChat@@YAXPAVCChatManager@@PBDZZ` | register-homing (why-reg) | 9 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.64 | advmgr | `?garrison_quick_view@advManager@@QAEXHHH@Z` | control-flow (why-branch) | 8 | 8 | loop-form / merged-return placement / case order (D1-D9) |
| 96.67 | recruit | `??0recruitUnit@@QAE@PAVhero@@W4TCreatureType..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.68 | army | `?do_post_attack@army@@QAEXPAV1@HHH@Z` | control-flow (why-branch) | 192 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 96.71 | mapcell | `??4TScenarioTown@@QAEAAU0@ABU0@@Z` | control-flow (why-branch) | 16 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 96.75 | mapcell | `?GenerateHeightMap@NewfullMap@@QAEXPBVCObjec..` | register-homing (why-reg) | 30 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 96.77 | resourcemanager | `?GetBitmapResourceSize@ResourceManager@@YIHP..` | unclassified | 7 | 0 | run why-reg / why-branch for the full search |
| 96.83 | cmbtmgr | `?MakeCreaturesVanish@combatManager@@QAEXXZ` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.86 | mapcell | `?insert@?$vector@VTTimedEvent@@V?$allocator@..` | inliner (predict-inline) | 88 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 96.86 | ai_tactical | `?get_muck_and_mire_value@type_AI_spellcaster..` | register-homing (why-reg) | 43 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.96 | game | `?ViewArmy@game@@QAEXAAVarmyGroup@@HPBVhero@@..` | inliner (predict-inline) | 28 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 96.98 | mapcell | `?readObject@NewfullMap@@QAEHPAVTAbstractFile..` | inliner (predict-inline) | 306 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 97.09 | remote | `?CheckForWarning@CTurnDuration@@QAEXXZ` | register-homing (why-reg) | 38 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.10 | ai_tactical | `?consider_resurrect@type_AI_spellcaster@@QAE..` | register-homing (why-reg) | 45 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.16 | puzzlewindow | `?AI_attempt_puzzle_guess@@YI?AUtype_point@@J..` | register-homing (why-reg) | 130 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 97.24 | game | `?ConvertObject@game@@QAEXPAVNewmapCell@@@Z` | inliner (predict-inline) | 133 | 2 | callee expanded on one side only (A8/A9/A12): 1 over-inline (4 name-unresolvable pair(s) discounted) |
| 97.27 | ai_player | `?do_best_purchase@type_AI_creature_purchaser..` | register-homing (why-reg) | 70 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.29 | spells | `?ResetBoltAngle@combatManager@@QAEXPAUSBolt@..` | unclassified | 30 | 0 | run why-reg / why-branch for the full search |
| 97.32 | ai_combat | `?do_aftermath@type_AI_combat_data@@QAEXPAV1@..` | register-homing (why-reg) | 4 | 0 | register-homing knob (B-family) |
| 97.33 | ai_player | `?get_purchase_value@type_AI_creature_purchas..` | register-homing (why-reg) | 12 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.39 | townmgr | `?WindowHandler@TMageGuildWindow@@UAEHPAVmess..` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.43 | ai_player | `?calculate_demand@type_AI_player@@QAEXXZ` | register-homing (why-reg) | 257 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 97.47 | seerhut | `?DoProposalDialog@type_creature_quest@@UAEXP..` | register-homing (why-reg) | 19 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.48 | command | `?ResetRound@combatManager@@QAEXXZ` | inliner (predict-inline) | 26 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline (3 name-unresolvable pair(s) discounted) |
| 97.49 | townmgr | `?handle_mage_guild_click@townManager@@QAEXXZ` | control-flow (why-branch) | 9 | 28 | loop-form / merged-return placement / case order (D1-D9) |
| 97.53 | ai_player | `?do_purchase@type_AI_creature_purchaser@@QAE..` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.61 | mapcell | `?erase@?$vector@VTTownEvent@@V?$allocator@VT..` | inliner (predict-inline) | 43 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 97.73 | cmbtmgr | `?CheckApplyBadMorale@combatManager@@QAEHHH@Z` | control-flow (why-branch) | 26 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 97.74 | town | `?initialize_spells@town@@QAEXPBVTownExtra@@@Z` | register-homing (why-reg) | 56 | 2 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.77 | bottomviewsubwindow | `??0TBottomViewHero@@QAE@PAVheroWindow@@@Z` | register-homing (why-reg) | 86 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.77 | townmgr | `?SetRolloverText@TCastleWindow@@QAEXPAVmessa..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.80 | advmgr | `?Open@advManager@@UAEHH@Z` | inliner (predict-inline) | 76 | 2 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (20 name-unresolvable pair(s) discounted) |
| 97.82 | advmgr | `?ScreenScroll@advManager@@QAEXHH@Z` | register-homing (why-reg) | 47 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 97.92 | game | `?ClaimTown@game@@QAEXHHEE@Z` | control-flow (why-branch) | 22 | 8 | loop-form / merged-return placement / case order (D1-D9) |
| 97.97 | hero | `?TransferArtifacts@hero@@QAEXPAV1@@Z` | register-homing (why-reg) | 14 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 97.97 | recruit | `??0recruitUnit@@QAE@PAVarmyGroup@@EW4TCreatu..` | register-homing (why-reg) | 24 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.02 | townmgr | `?WindowHandler@TBuyBuildWindow@@UAEHPAVmessa..` | unclassified | 3 | 0 | run why-reg / why-branch for the full search |
| 98.09 | townmgr | `?SetupTown@townManager@@QAEXE@Z` | unclassified | 20 | 0 | run why-reg / why-branch for the full search |
| 98.10 | hero | `?GetExperienceIncrement@hero@@SIHH@Z` | register-homing (why-reg) | 23 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.13 | systemoptionswindow | `??0TSystemOptionsWindow@@QAE@XZ` | inliner (predict-inline) | 145 | 105 | callee expanded on one side only (A8/A9/A12): 2 under-inline (133 name-unresolvable pair(s) discounted) |
| 98.14 | cmbtmgr | `?SetupAndLoadObstacles@combatManager@@QAEXXZ` | register-homing (why-reg) | 34 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.17 | advmgr | `?DrawHeroPart@advManager@@QAEXHAAUTDrawParts..` | unclassified | 15 | 0 | run why-reg / why-branch for the full search |
| 98.18 | advmgr | `?DrawHeroPartShadow@advManager@@QAEXHAAUTDra..` | unclassified | 15 | 0 | run why-reg / why-branch for the full search |
| 98.19 | ai_tactical | `?consider_spell@type_AI_spellcaster@@QAEXPAU..` | register-homing (why-reg) | 72 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.21 | campaignbrief | `??1TCampaignBrief@@UAE@XZ` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.29 | townmgr | `??0type_garrison_base_window@@QAE@PAVhero@@H..` | inliner (predict-inline) | 169 | 44 | callee expanded on one side only (A8/A9/A12): 4 over-inline (203 name-unresolvable pair(s) discounted) |
| 98.32 | remote | `?HandleMPlayerLaunch@@YIEXZ` | unclassified | 20 | 0 | run why-reg / why-branch for the full search |
| 98.34 | remote | `?HandlePlayerDead@@YIXHE@Z` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.34 | seerhut | `?SetDefaultText@type_monster_quest@@UAEXXZ` | inliner (predict-inline) | 25 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (18 name-unresolvable pair(s) discounted) |
| 98.35 | game | `?match_underground_gates@game@@QAEXXZ` | register-homing (why-reg) | 18 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.38 | drawing | `?DrawFrame@combatManager@@QAEXEEEHEE@Z` | inliner (predict-inline) | 34 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (5 name-unresolvable pair(s) discounted) |
| 98.46 | font | `?DrawStringExecute@font@@QAEXPBDHPAVBitmap16..` | register-homing (why-reg) | 72 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.47 | ai | `?DoCompAI@combatManager@@QAEXH@Z` | unclassified | 1 | 0 | run why-reg / why-branch for the full search |
| 98.47 | campaignwindow | `??0TCampaignWindow@@QAE@EH@Z` | inliner (predict-inline) | 189 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (25 name-unresolvable pair(s) discounted) |
| 98.49 | textwdgt | `?Main@textWidget@@UAEHPAVmessage@@@Z` | register-homing (why-reg) | 68 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.50 | misc | `?CheckConfigFile@@YIXXZ` | register-homing (why-reg) | 47 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.52 | bottomviewsubwindow | `??0TBottomViewKingdom@@QAE@PAVheroWindow@@@Z` | register-homing (why-reg) | 24 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.52 | army | `?get_estimated_damage@army@@QBEJPBV1@JEJ@Z` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.57 | armygrp | `?GetMorale@armyGroup@@QAEHPBVhero@@PBVtown@@..` | register-homing (why-reg) | 13 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.57 | mapcell | `??4BlackBoxData@@QAEAAV0@ABV0@@Z` | inliner (predict-inline) | 27 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (25 name-unresolvable pair(s) discounted) |
| 98.58 | mapcell | `?erase@?$vector@VBlackBoxData@@V?$allocator@..` | inliner (predict-inline) | 27 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline (2 name-unresolvable pair(s) discounted) |
| 98.58 | townmgr | `?SetupWell@townManager@@QAEXPAVTCastleWindow..` | register-homing (why-reg) | 48 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.58 | adventuremapwindow | `?UpdateHeroLocator@TAdventureMapWindow@@QAEX..` | control-flow (why-branch) | 9 | 14 | loop-form / merged-return placement / case order (D1-D9) |
| 98.59 | mapcell | `?erase@?$vector@VTTimedEvent@@V?$allocator@V..` | inliner (predict-inline) | 3 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 98.60 | findpath | `?GetTerrainCost@@YIHPAVhero@@Utype_point@@HH..` | unclassified | 3 | 0 | run why-reg / why-branch for the full search |
| 98.61 | townmgr | `?DoTownGate@townManager@@QAEXXZ` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.61 | game | `?CreateTownHeroes@game@@QAEXPAH@Z` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.62 | army | `?LoadResources@army@@QAEXXZ` | inliner (predict-inline) | 117 | 0 | callee expanded on one side only (A8/A9/A12): 8 under-inline, 8 over-inline |
| 98.68 | hero | `?IsInIdentifyRange@hero@@QAEEPBUtype_point@@..` | register-homing (why-reg) | 2 | 0 | register-homing knob (B-family) |
| 98.75 | bottomviewsubwindow | `??0TBottomViewTown@@QAE@PAVheroWindow@@@Z` | inliner (predict-inline) | 81 | 0 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 5 over-inline (37 name-unresolvable pair(s) discounted) |
| 98.79 | font | `?DrawBoundedString@font@@QAEXPBDPAVBitmap16B..` | register-homing (why-reg) | 9 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.81 | hero | `?get_skill_award@@YI?AW4TSecondarySkill@@PBV..` | control-flow (why-branch) | 8 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 98.84 | townmgr | `?BuildObj@townManager@@QAEXH@Z` | unclassified | 13 | 1 | run why-reg / why-branch for the full search |
| 98.84 | quicktownwindow | `??0TQuickTownWindow@@QAE@PBVtown@@W4TViewLev..` | register-homing (why-reg) | 16 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 98.84 | cmbtmgr | `?damage_message@combatManager@@QAEXPBDJJPBVa..` | unclassified | 9 | 0 | run why-reg / why-branch for the full search |
| 98.84 | puzzlewindow | `??0type_AI_puzzle_tile@@QAE@PAVNewmapCell@@U..` | register-homing (why-reg) | 9 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.85 | mapcell | `?insert@?$vector@VMonsterData@@V?$allocator@..` | inliner (predict-inline) | 36 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 98.88 | game | `??0SavedGameHeader@@QAE@XZ` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 98.95 | townmgr | `??0TTownScreenWindow@@QAE@XZ` | inliner (predict-inline) | 259 | 1 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 6 over-inline (248 name-unresolvable pair(s) discounted) |
| 98.96 | game | `?SetupPuzzlePieces@game@@QAEHHH@Z` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 98.98 | game | `?GetNewHeroId@game@@QAEHHW4THeroClass@@E0@Z` | register-homing (why-reg) | 6 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.98 | ai_tactical | `?get_frenzy_value@type_AI_spellcaster@@QAEJP..` | unclassified | 6 | 0 | run why-reg / why-branch for the full search |
| 98.99 | game | `?add_garrison_hero@playerData@@QAEEPAVtown@@..` | register-homing (why-reg) | 2 | 0 | register-homing knob (B-family) |
| 99.06 | town | `?show_creature_rewards@@YIXPBVtown@@PAV?$vec..` | unclassified | 4 | 0 | run why-reg / why-branch for the full search |
| 99.10 | ai | `?choose_defense_hex@combatManager@@QAEEPBVar..` | register-homing (why-reg) | 30 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.11 | ai_tactical | `?get_simple_attack_effect@type_AI_combat_par..` | register-homing (why-reg) | 32 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.13 | levelupwindow | `??0TLevelUpWindow@@QAE@PAVhero@@HHH@Z` | inliner (predict-inline) | 61 | 0 | callee expanded on one side only (A8/A9/A12): 1 over-inline (84 name-unresolvable pair(s) discounted) |
| 99.15 | advmgr | `?MoreTreesNear@advManager@@QAEHUtype_point@@..` | control-flow (why-branch) | 117 | 4 | loop-form / merged-return placement / case order (D1-D9) |
| 99.20 | drawing | `?DrawWallAt@combatManager@@QAEXHH@Z` | control-flow (why-branch) | 6 | 32 | loop-form / merged-return placement / case order (D1-D9) |
| 99.20 | findpath | `?TestPossibleDirections@searchArray@@QAEXPAV..` | register-homing (why-reg) | 46 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.27 | victorylossconditions | `?CheckForGrailBuildingWin@VictoryConditionSt..` | register-homing (why-reg) | 13 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.27 | advmgr | `?SetHeroContext@advManager@@QAEXHHEE@Z` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.30 | town | `?BuildBuilding@town@@QAE?AW4type_building_id..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.31 | mousemgr | `?LoadFrame@mouseManager@@QAEXH@Z` | register-homing (why-reg) | 32 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.35 | cmbtmgr | `?LoadArmies@combatManager@@QAEXE@Z` | unclassified | 2 | 0 | run why-reg / why-branch for the full search |
| 99.39 | resourcedisplay | `??0TResourceDisplay@@QAE@PAVheroWindow@@E@Z` | register-homing (why-reg) | 52 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.40 | game | `?NewMap@game@@QAEEPBD0PAHH@Z` | unclassified | 6 | 0 | run why-reg / why-branch for the full search |
| 99.47 | mapcell | `?Read@TTimedEvent@@QAEHPAVTAbstractFile@@H@Z` | inliner (predict-inline) | 3 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 99.53 | hero | `?SetupHeroView@THeroScreenWindow@@QAEXXZ` | control-flow (why-branch) | 7 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 99.55 | army | `?SetSpellInfluence@army@@QAEXHHHPBVhero@@@Z` | inliner (predict-inline) | 125 | 7 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (13 name-unresolvable pair(s) discounted) |
| 99.57 | ai_tactical | `?consider_single_enchantment@type_AI_spellca..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.57 | seerhut | `?DoProgressDialog@type_creature_quest@@UAEXXZ` | register-homing (why-reg) | 5 | 0 | register-homing knob (B-family) |
| 99.58 | hero | `?get_primary_skill_total@hero@@QAEFXZ` | register-homing (why-reg) | 2 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.64 | diff | `?Apply@CDiffFile@@QAEPAXPAEH@Z` | register-homing (why-reg) | 6 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.64 | game | `?LoadBoatPool@game@@QAEHPAVTAbstractFile@@@Z` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.65 | cmbtmgr | `?SetNextArmy@combatManager@@QAEXHH@Z` | inliner (predict-inline) | 39 | 0 | callee expanded on one side only (A8/A9/A12): 6 under-inline, 6 over-inline (9 name-unresolvable pair(s) discounted) |
| 99.66 | townmgr | `??0THallWindow@@QAE@H@Z` | inliner (predict-inline) | 42 | 2 | callee expanded on one side only (A8/A9/A12): 12 under-inline, 12 over-inline (222 name-unresolvable pair(s) discounted) |
| 99.71 | army | `?can_cast_spell@army@@QBEEJ@Z` | register-homing (why-reg) | 33 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.72 | resourcemanager | `?insert_wrapper@TCacheMap@ResourceManager@@Q..` | unclassified | 2 | 0 | run why-reg / why-branch for the full search |
| 99.75 | game | `?randomize_university@game@@QAEXPAVNewmapCel..` | inliner (predict-inline) | 12 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 99.78 | town | `?initialize_hordes@town@@SIXXZ` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.78 | soundmgr | `?MemorySample@soundManager@@QAEPAVds_memsamp..` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.85 | combatresultswindow | `??0TCombatResultsWindow@@QAE@PBVhero@@0HHEH@Z` | register-homing (why-reg) | 83 | 1 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.85 | town | `?TownFn_005BF900@town@@QAEJJ@Z` | unclassified | 14 | 0 | run why-reg / why-branch for the full search |
| 99.85 | ai_tactical | `?get_attack_skill_value@type_AI_spellcaster@..` | register-homing (why-reg) | 52 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.86 | spells | `?AreaEffect@combatManager@@QAEXJHJJ@Z` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.87 | ai_tactical | `?get_curse_value@type_AI_spellcaster@@QAEJPB..` | unclassified | 34 | 0 | run why-reg / why-branch for the full search |
| 99.88 | game | `?get_random_lith@game@@QAEEPBV?$vector@Utype..` | register-homing (why-reg) | 48 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.88 | hero | `?HeroFn_004DBF30@hero@@QAEEHJ@Z` | register-homing (why-reg) | 24 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.89 | mapcell | `?NewfullMapFn_005042C0@NewfullMap@@QAEXXZ` | register-homing (why-reg) | 2 | 0 | register-homing knob (B-family) |
| 99.90 | ai_player | `?fill_prohibited_array@@YIXPAVplayerData@@PA..` | register-homing (why-reg) | 6 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.91 | remote | `?HandlePlayerLost@@YIXPAVCNetMsg@@@Z` | register-homing (why-reg) | 2 | 0 | register-homing knob (B-family) |
| 99.92 | armygrp | `?WindowHandler@TSplitWindow@@UAEHPAVmessage@..` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.93 | seerhut | `?LoadFromMap@type_creature_quest@@UAEXPAVTAb..` | register-homing (why-reg) | 28 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.93 | seerhut | `?Load@type_creature_quest@@UAEXPAVTAbstractF..` | register-homing (why-reg) | 28 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.94 | army | `?do_multi_head_attack@army@@QAEXIPAH0PAJ@Z` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.94 | adventureoptionswindow | `?WindowHandler@TAdventureOptionsWindow@@UAEH..` | control-flow (why-branch) | 34 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 99.94 | game | `?LoadMinePool@game@@QAEHPAVTAbstractFile@@H@Z` | register-homing (why-reg) | 4 | 0 | spill to dead-parameter slot (B4) |
| 99.94 | remote | `?HandlePlayerWon@@YIXPAVCNetMsg@@@Z` | register-homing (why-reg) | 2 | 0 | register-homing knob (B-family) |
| 99.95 | seerhut | `?Load@type_artifact_quest@@UAEXPAVTAbstractF..` | register-homing (why-reg) | 20 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.95 | seerhut | `?LoadFromMap@type_artifact_quest@@UAEXPAVTAb..` | register-homing (why-reg) | 20 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.95 | game | `?RandomizeHolyGrail@game@@QAEXXZ` | unclassified | 18 | 0 | run why-reg / why-branch for the full search |
| 99.96 | ai_player | `?mark_towns@type_town_threat_checker@@IAEXPA..` | unclassified | 2 | 0 | run why-reg / why-branch for the full search |
| 99.96 | game | `?save@playerData@@QAEHPAVTAbstractFile@@@Z` | register-homing (why-reg) | 34 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.96 | ai_combat | `?AI_value_of_combat@@YIJPBVhero@@0ABVarmyGro..` | unclassified | 8 | 0 | run why-reg / why-branch for the full search |
| 99.98 | seerhut | `?getValue@TSeerReward@@QAEHPBVhero@@@Z` | register-homing (why-reg) | 43 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 99.99 | armygrp | `??0TSplitWindow@@QAE@HHH@Z` | inliner (predict-inline) | 2 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (32 name-unresolvable pair(s) discounted) |
