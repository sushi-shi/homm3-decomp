<!-- # generator: homm3.vc6.report | # date: 2026-09-02 | # ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - regenerate, never hand-edit | plateaus in [50.0, 99.999%); base-vs-delinked-target diagnosis, no recompiles -->
# vc6 plateau diagnosis (read-only; solvers propose, never land)

31 function(s). why-reg = register-homing knobs; why-branch = control-flow knobs; predict-inline = out-of-line CALL multiset divergence (a callee inlined on one side only - dominated by STL basic_string/vector ops + small dtors retail inlines and we do not). CALIBRATION 2026-08-19: this column USED to be dominated by a NAME artifact - retail's side names an unclaimed callee with a synth working label our compiled side can never emit, so one call booked as both an under- and an over-inline and the inliner route (which sits upstream of registers and blocks) buried the true diagnosis. inline_model.divergence now pairs those off by count: on the tree of that date the inliner class fell from 135 rows to 46 of 211, and register-homing (108) overtook it as the dominant plateau class. MECHANISM (RE'd, docs/vc6/inliner.md): /Ob2 budget = clamp(2*caller_cb,1000,35000) spent sequentially; our leaner reconstructions sit at the 1000 floor and STARVE, so retail inlines what we call. FIX = finish the caller's body (budget follows statement mass, byte-inert counts) - do NOT chase _Tidy/vector spellings or pragmas. So on LOW-% rows inline divergence largely self-resolves as reconstruction completes; it is the pure wall only on high-% rows. Mixed walls list both distances.

## Wall-class summary

- **15** register-homing (why-reg)
- **8** inliner (predict-inline)
- **5** control-flow (why-branch)
- **2** unclassified

| fuzzy | unit | function | wall class | reg-dist | flow-dist | knob to try |
|---|---|---|---|---|---|---|
| 72.22 | ai_player | `?buy_creatures@type_AI_player@@QAEXPAVhero@@..` | inliner (predict-inline) | 324 | 8 | callee expanded on one side only (A8/A9/A12): 2 under-inline (4 name-unresolvable pair(s) discounted) |
| 75.38 | ai_player | `?consider_hiring@@YI_NJPAVhero@@@Z` | inliner (predict-inline) | 276 | 40 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 76.87 | ai_player | `?AI_choose_destination@@YIHPAVhero@@JAAUHero..` | inliner (predict-inline) | 884 | 132 | callee expanded on one side only (A8/A9/A12): 2 over-inline (5 name-unresolvable pair(s) discounted) |
| 77.61 | ai_player | `?mark_destinations@@YIJPAVhero@@JPAVsearchAr..` | register-homing (why-reg) | 288 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 81.03 | ai_player | `?can_trade_resources@type_AI_player@@QAE_NPB..` | control-flow (why-branch) | 455 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 82.56 | ai_player | `?get_value@type_necromancy_artifact@@UBEJPBV..` | register-homing (why-reg) | 19 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 83.62 | ai_player | `?split_armies@@YIXPAVhero@@PBV1@PBVarmyGroup..` | control-flow (why-branch) | 127 | 30 | loop-form / merged-return placement / case order (D1-D9) |
| 84.59 | ai_player | `?make_gift@type_AI_player@@QAEXJ@Z` | register-homing (why-reg) | 279 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 84.83 | ai_player | `?AI_AttemptMove@@YIXPAVhero@@AAUHeroDestinat..` | inliner (predict-inline) | 176 | 42 | callee expanded on one side only (A8/A9/A12): 1 over-inline (5 name-unresolvable pair(s) discounted) |
| 85.35 | ai_player | `?net_value_of_location@@YIHPAVhero@@PAUHeroD..` | register-homing (why-reg) | 190 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 85.83 | ai_player | `?check_trade_supply@type_AI_player@@QAE_NPBH..` | register-homing (why-reg) | 73 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 88.34 | ai_player | `?choose_weakest_army@type_AI_creature_swappe..` | register-homing (why-reg) | 119 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.09 | ai_player | `?value_of_adding_army@type_AI_creature_swapp..` | register-homing (why-reg) | 155 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 89.53 | ai_player | `?end_turn@type_AI_player@@QAEXXZ` | inliner (predict-inline) | 102 | 3 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline (2 name-unresolvable pair(s) discounted) |
| 90.12 | ai_player | `?trade_resources@type_AI_player@@QAEXPBHJ@Z` | inliner (predict-inline) | 53 | 1 | callee expanded on one side only (A8/A9/A12): 1 over-inline |
| 90.68 | ai_player | `?get_value@type_antiluck_artifact@@UBEJPBVhe..` | control-flow (why-branch) | 6 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 90.68 | ai_player | `?get_value@type_antimorale_artifact@@UBEJPBV..` | control-flow (why-branch) | 6 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 91.26 | ai_player | `?calculate_reserve@type_AI_player@@QAEXXZ` | register-homing (why-reg) | 78 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.51 | ai_player | `?get_swap_value@type_AI_creature_swapper@@QA..` | register-homing (why-reg) | 16 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.76 | ai_player | `?AI_get_value_of_artifact@@YIJUtype_artifact..` | register-homing (why-reg) | 132 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.52 | ai_player | `?value_of_castle_upgrade@@YIHPAVtown@@PAH@Z` | unclassified | 14 | 0 | run why-reg / why-branch for the full search |
| 96.19 | ai_player | `?purchase_building@type_AI_player@@QAEEPAE@Z` | control-flow (why-branch) | 270 | 48 | loop-form / merged-return placement / case order (D1-D9) |
| 96.36 | ai_player | `?find_all_destinations@@YIJPAVhero@@PAVsearc..` | inliner (predict-inline) | 91 | 3 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 3 over-inline (10 name-unresolvable pair(s) discounted) |
| 96.78 | ai_player | `?get_value@type_undead_king_cloak_artifact@@..` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.27 | ai_player | `?do_best_purchase@type_AI_creature_purchaser..` | register-homing (why-reg) | 70 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.33 | ai_player | `?get_purchase_value@type_AI_creature_purchas..` | register-homing (why-reg) | 12 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.43 | ai_player | `?calculate_demand@type_AI_player@@QAEXXZ` | register-homing (why-reg) | 257 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 97.79 | ai_player | `?get_value@type_school_artifact@@UBEJPBVhero..` | unclassified | 9 | 0 | run why-reg / why-branch for the full search |
| 99.95 | ai_player | `?value_of_hiring@@YIJPAVtown@@PAVhero@@PAVse..` | inliner (predict-inline) | 48 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 99.97 | ai_player | `?attempt_teleport@@YIEPAVhero@@AAV?$vector@U..` | (diag error: no shared public text symbol in built objects) | - | - | - |
| 99.97 | ai_player | `?fill_prohibited_array@@YIXPAVplayerData@@PA..` | register-homing (why-reg) | 2 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
