<!-- # generator: homm3.vc6.report | # date: 2026-08-27 | # ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - regenerate, never hand-edit | plateaus in [60.0, 99.999%); base-vs-delinked-target diagnosis, no recompiles -->
# vc6 plateau diagnosis (read-only; solvers propose, never land)

29 function(s). why-reg = register-homing knobs; why-branch = control-flow knobs; predict-inline = out-of-line CALL multiset divergence (a callee inlined on one side only - dominated by STL basic_string/vector ops + small dtors retail inlines and we do not). CALIBRATION 2026-08-19: this column USED to be dominated by a NAME artifact - retail's side names an unclaimed callee with a synth working label our compiled side can never emit, so one call booked as both an under- and an over-inline and the inliner route (which sits upstream of registers and blocks) buried the true diagnosis. inline_model.divergence now pairs those off by count: on the tree of that date the inliner class fell from 135 rows to 46 of 211, and register-homing (108) overtook it as the dominant plateau class. MECHANISM (RE'd, docs/vc6/inliner.md): /Ob2 budget = clamp(2*caller_cb,1000,35000) spent sequentially; our leaner reconstructions sit at the 1000 floor and STARVE, so retail inlines what we call. FIX = finish the caller's body (budget follows statement mass, byte-inert counts) - do NOT chase _Tidy/vector spellings or pragmas. So on LOW-% rows inline divergence largely self-resolves as reconstruction completes; it is the pure wall only on high-% rows. Mixed walls list both distances.

## Wall-class summary

- **14** register-homing (why-reg)
- **7** control-flow (why-branch)
- **5** inliner (predict-inline)
- **3** unclassified

| fuzzy | unit | function | wall class | reg-dist | flow-dist | knob to try |
|---|---|---|---|---|---|---|
| 66.81 | townmgr | `?Main@townManager@@UAEHAAVmessage@@@Z` | inliner (predict-inline) | 1418 | 445 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 5 over-inline (34 name-unresolvable pair(s) discounted) |
| 81.41 | townmgr | `?SetRolloverText@TTavernWindow@@QAEXH@Z` | register-homing (why-reg) | 65 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.33 | townmgr | `?SetupThievesGuild@TThievesGuildWindow@@QAEX..` | inliner (predict-inline) | 1014 | 175 | callee expanded on one side only (A8/A9/A12): 2 over-inline (56 name-unresolvable pair(s) discounted) |
| 87.42 | townmgr | `?set_prerequisite_text@TBuyBuildWindow@@QAEX..` | register-homing (why-reg) | 68 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.93 | townmgr | `?set_bonus_display@TTownScreenWindow@@QAEXPA..` | control-flow (why-branch) | 298 | 11 | loop-form / merged-return placement / case order (D1-D9) |
| 89.72 | townmgr | `?WindowHandler@TTavernWindow@@UAEHPAVmessage..` | register-homing (why-reg) | 80 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.75 | townmgr | `?DoBlacksmith@@YIXHH@Z` | register-homing (why-reg) | 34 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.93 | townmgr | `?UpdateTownLocator@TTownScreenWindow@@QAEXH@Z` | register-homing (why-reg) | 11 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 92.05 | townmgr | `?SetCommandAndText@townManager@@QAEXPAVmessa..` | control-flow (why-branch) | 449 | 70 | loop-form / merged-return placement / case order (D1-D9) |
| 93.70 | townmgr | `?DoUniversity@townManager@@QAEXXZ` | register-homing (why-reg) | 31 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.88 | townmgr | `??0type_monster_join_window@@QAE@PAVhero@@PA..` | control-flow (why-branch) | 43 | 24 | loop-form / merged-return placement / case order (D1-D9) |
| 94.18 | townmgr | `?SetRolloverText@TThievesGuildWindow@@QAEXH@Z` | control-flow (why-branch) | 136 | 3 | loop-form / merged-return placement / case order (D1-D9) |
| 95.12 | townmgr | `?BuyBuild@townManager@@QAEHHHH@Z` | control-flow (why-branch) | 343 | 56 | loop-form / merged-return placement / case order (D1-D9) |
| 95.66 | townmgr | `?WindowHandler@type_garrison_base_window@@UA..` | register-homing (why-reg) | 106 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 95.69 | townmgr | `?SetCommandAndText@type_garrison_base_window..` | register-homing (why-reg) | 62 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.43 | townmgr | `?Recruit@TCastleWindow@@QAEXH@Z` | register-homing (why-reg) | 36 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.48 | townmgr | `?DoTavern@@YIEXZ` | control-flow (why-branch) | 29 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 97.39 | townmgr | `?WindowHandler@TMageGuildWindow@@UAEHPAVmess..` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 97.49 | townmgr | `?handle_mage_guild_click@townManager@@QAEXXZ` | control-flow (why-branch) | 9 | 28 | loop-form / merged-return placement / case order (D1-D9) |
| 97.77 | townmgr | `?SetRolloverText@TCastleWindow@@QAEXPAVmessa..` | register-homing (why-reg) | 14 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.02 | townmgr | `?WindowHandler@TBuyBuildWindow@@UAEHPAVmessa..` | unclassified | 3 | 0 | run why-reg / why-branch for the full search |
| 98.09 | townmgr | `?SetupTown@townManager@@QAEXE@Z` | unclassified | 20 | 0 | run why-reg / why-branch for the full search |
| 98.29 | townmgr | `??0type_garrison_base_window@@QAE@PAVhero@@H..` | inliner (predict-inline) | 169 | 44 | callee expanded on one side only (A8/A9/A12): 4 over-inline (203 name-unresolvable pair(s) discounted) |
| 98.58 | townmgr | `?SetupWell@townManager@@QAEXPAVTCastleWindow..` | register-homing (why-reg) | 48 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.61 | townmgr | `?DoTownGate@townManager@@QAEXXZ` | register-homing (why-reg) | 10 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 98.84 | townmgr | `?BuildObj@townManager@@QAEXH@Z` | unclassified | 13 | 1 | run why-reg / why-branch for the full search |
| 98.95 | townmgr | `??0TTownScreenWindow@@QAE@XZ` | inliner (predict-inline) | 259 | 1 | callee expanded on one side only (A8/A9/A12): 5 under-inline, 6 over-inline (248 name-unresolvable pair(s) discounted) |
| 99.66 | townmgr | `??0THallWindow@@QAE@H@Z` | inliner (predict-inline) | 42 | 2 | callee expanded on one side only (A8/A9/A12): 12 under-inline, 12 over-inline (222 name-unresolvable pair(s) discounted) |
| 99.96 | townmgr | `?GetBuildingInfo@@YIPADPBVtown@@HEE@Z` | register-homing (why-reg) | 24 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
