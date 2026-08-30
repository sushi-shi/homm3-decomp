<!-- # generator: homm3.vc6.report | # date: 2026-08-30 | # ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - regenerate, never hand-edit | plateaus in [50.0, 99.999%); base-vs-delinked-target diagnosis, no recompiles -->
# vc6 plateau diagnosis (read-only; solvers propose, never land)

31 function(s). why-reg = register-homing knobs; why-branch = control-flow knobs; predict-inline = out-of-line CALL multiset divergence (a callee inlined on one side only - dominated by STL basic_string/vector ops + small dtors retail inlines and we do not). CALIBRATION 2026-08-19: this column USED to be dominated by a NAME artifact - retail's side names an unclaimed callee with a synth working label our compiled side can never emit, so one call booked as both an under- and an over-inline and the inliner route (which sits upstream of registers and blocks) buried the true diagnosis. inline_model.divergence now pairs those off by count: on the tree of that date the inliner class fell from 135 rows to 46 of 211, and register-homing (108) overtook it as the dominant plateau class. MECHANISM (RE'd, docs/vc6/inliner.md): /Ob2 budget = clamp(2*caller_cb,1000,35000) spent sequentially; our leaner reconstructions sit at the 1000 floor and STARVE, so retail inlines what we call. FIX = finish the caller's body (budget follows statement mass, byte-inert counts) - do NOT chase _Tidy/vector spellings or pragmas. So on LOW-% rows inline divergence largely self-resolves as reconstruction completes; it is the pure wall only on high-% rows. Mixed walls list both distances.

## Wall-class summary

- **11** register-homing (why-reg)
- **10** control-flow (why-branch)
- **10** inliner (predict-inline)

| fuzzy | unit | function | wall class | reg-dist | flow-dist | knob to try |
|---|---|---|---|---|---|---|
| 65.70 | singleselectionwindow | `?GetHeroFace@TSingleSelectionWindow@@QAEXHPA..` | control-flow (why-branch) | 161 | 29 | loop-form / merged-return placement / case order (D1-D9) |
| 70.46 | singleselectionwindow | `?UpdatePlayerPositions@TSingleSelectionWindo..` | control-flow (why-branch) | 145 | 46 | loop-form / merged-return placement / case order (D1-D9) |
| 71.90 | singleselectionwindow | `?Tick@t_map_list_update@@UAEXXZ` | control-flow (why-branch) | 151 | 8 | loop-form / merged-return placement / case order (D1-D9) |
| 77.01 | singleselectionwindow | `?GetDisplayFace@TSingleSelectionWindow@@QAEH..` | inliner (predict-inline) | 47 | 5 | callee expanded on one side only (A8/A9/A12): 2 over-inline (1 name-unresolvable pair(s) discounted) |
| 79.94 | singleselectionwindow | `?HandleRequests@CNewPlayerUpdateProc@@QAEXXZ` | inliner (predict-inline) | 106 | 16 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 9 over-inline (6 name-unresolvable pair(s) discounted) |
| 80.54 | singleselectionwindow | `?GetHeroName@TSingleSelectionWindow@@QAEPBDH..` | control-flow (why-branch) | 81 | 50 | loop-form / merged-return placement / case order (D1-D9) |
| 81.30 | singleselectionwindow | `?OnGameHeaderInfoInitMsg@TSingleSelectionWin..` | inliner (predict-inline) | 221 | 10 | callee expanded on one side only (A8/A9/A12): 3 under-inline, 2 over-inline (17 name-unresolvable pair(s) discounted) |
| 86.67 | singleselectionwindow | `?Tick@CNewPlayerUpdateProc@@UAEXXZ` | register-homing (why-reg) | 108 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 87.02 | singleselectionwindow | `?Finish@CNewPlayerUpdateProc@@UAEXXZ` | register-homing (why-reg) | 22 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 87.34 | singleselectionwindow | `?MakeHeroFilter@TSingleSelectionWindow@@QAEX..` | inliner (predict-inline) | 150 | 0 | callee expanded on one side only (A8/A9/A12): 2 over-inline (2 name-unresolvable pair(s) discounted) |
| 88.62 | singleselectionwindow | `?HeaderRequested@CNewPlayerUpdateMan@@QAEXKE..` | control-flow (why-branch) | 55 | 16 | loop-form / merged-return placement / case order (D1-D9) |
| 88.84 | singleselectionwindow | `?OnPingResponseMsg@TSingleSelectionWindow@@Q..` | register-homing (why-reg) | 27 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 88.99 | singleselectionwindow | `?SetupScenarioOptions@TSingleSelectionWindow..` | inliner (predict-inline) | 125 | 108 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 90.16 | singleselectionwindow | `?HandleNetMsg@TSingleSelectionWindow@@QAEEPA..` | inliner (predict-inline) | 318 | 43 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 4 over-inline (15 name-unresolvable pair(s) discounted) |
| 90.22 | singleselectionwindow | `?OnMapFileNameMsg@TSingleSelectionWindow@@QA..` | inliner (predict-inline) | 119 | 3 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (9 name-unresolvable pair(s) discounted) |
| 90.28 | singleselectionwindow | `?OnGameHeaderInfoMsg@TSingleSelectionWindow@..` | inliner (predict-inline) | 29 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 5 over-inline (7 name-unresolvable pair(s) discounted) |
| 90.56 | singleselectionwindow | `?DrawHeroAdvancedOption@TSingleSelectionWind..` | control-flow (why-branch) | 361 | 10 | loop-form / merged-return placement / case order (D1-D9) |
| 91.67 | singleselectionwindow | `?SortMaps@TSingleSelectionWindow@@QAEXHEE@Z` | register-homing (why-reg) | 193 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 95.08 | singleselectionwindow | `?UpdateFilterWidgets@TSingleSelectionWindow@..` | control-flow (why-branch) | 73 | 58 | loop-form / merged-return placement / case order (D1-D9) |
| 95.52 | singleselectionwindow | `?DrawBasicMapInfo@TSingleSelectionWindow@@QA..` | register-homing (why-reg) | 44 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 96.24 | singleselectionwindow | `?SetNextPlayer@CNetPlayerHandler@@QAEEH@Z` | control-flow (why-branch) | 6 | 8 | loop-form / merged-return placement / case order (D1-D9) |
| 97.06 | singleselectionwindow | `?OnNewPlayerMsg@TSingleSelectionWindow@@QAEE..` | inliner (predict-inline) | 19 | 5 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 97.20 | singleselectionwindow | `?CanChooseTown@TSingleSelectionWindow@@QAEEH..` | control-flow (why-branch) | 4 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 97.54 | singleselectionwindow | `?OnNewHostMsg@TSingleSelectionWindow@@QAEXPA..` | register-homing (why-reg) | 4 | 0 | register-homing knob (B-family) |
| 97.69 | singleselectionwindow | `?WindowHandler@TSingleSelectionWindow@@UAEHP..` | inliner (predict-inline) | 41 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (4 name-unresolvable pair(s) discounted) |
| 97.87 | singleselectionwindow | `?CanChooseHero@TSingleSelectionWindow@@QAEEH..` | control-flow (why-branch) | 4 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 98.98 | singleselectionwindow | `?Update@TSingleSelectionWindow@@QAEHXZ` | register-homing (why-reg) | 60 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.38 | singleselectionwindow | `?SetupAdvancedOptions@TSingleSelectionWindow..` | register-homing (why-reg) | 98 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.87 | singleselectionwindow | `?OnKillFocus@CEnterNameEdit@@UAEXXZ` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.89 | singleselectionwindow | `?OnKeyPress@CEnterNameEdit@@UAEHPAVmessage@@..` | register-homing (why-reg) | 8 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 99.99 | singleselectionwindow | `?SetCurrentMap@TSingleSelectionWindow@@QAEXH..` | register-homing (why-reg) | 141 | 1 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
