<!-- # generator: homm3.vc6.report | # date: 2026-08-22 | # ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - regenerate, never hand-edit | plateaus in [50.0, 99.999%); base-vs-delinked-target diagnosis, no recompiles -->
# vc6 plateau diagnosis (read-only; solvers propose, never land)

38 function(s). why-reg = register-homing knobs; why-branch = control-flow knobs; predict-inline = out-of-line CALL multiset divergence (a callee inlined on one side only - dominated by STL basic_string/vector ops + small dtors retail inlines and we do not). CALIBRATION 2026-08-19: this column USED to be dominated by a NAME artifact - retail's side names an unclaimed callee with a synth working label our compiled side can never emit, so one call booked as both an under- and an over-inline and the inliner route (which sits upstream of registers and blocks) buried the true diagnosis. inline_model.divergence now pairs those off by count: on the tree of that date the inliner class fell from 135 rows to 46 of 211, and register-homing (108) overtook it as the dominant plateau class. MECHANISM (RE'd, docs/vc6/inliner.md): /Ob2 budget = clamp(2*caller_cb,1000,35000) spent sequentially; our leaner reconstructions sit at the 1000 floor and STARVE, so retail inlines what we call. FIX = finish the caller's body (budget follows statement mass, byte-inert counts) - do NOT chase _Tidy/vector spellings or pragmas. So on LOW-% rows inline divergence largely self-resolves as reconstruction completes; it is the pure wall only on high-% rows. Mixed walls list both distances.

## Wall-class summary

- **15** inliner (predict-inline)
- **14** control-flow (why-branch)
- **9** register-homing (why-reg)

| fuzzy | unit | function | wall class | reg-dist | flow-dist | knob to try |
|---|---|---|---|---|---|---|
| 64.28 | mapcell | `?Load@NewfullMap@@QAEHPAVTAbstractFile@@HEH@Z` | inliner (predict-inline) | 201 | 21 | callee expanded on one side only (A8/A9/A12): 6 under-inline, 1 over-inline (14 name-unresolvable pair(s) discounted) |
| 76.91 | mapcell | `?readResourceData@NewfullMap@@QAEHPAVTAbstra..` | control-flow (why-branch) | 78 | 18 | loop-form / merged-return placement / case order (D1-D9) |
| 79.86 | mapcell | `?get_special_terrain@NewmapCell@@QBE?AW4TAdv..` | control-flow (why-branch) | 66 | 48 | loop-form / merged-return placement / case order (D1-D9) |
| 84.00 | mapcell | `?insert@?$vector@VTreasureData@@V?$allocator..` | control-flow (why-branch) | 242 | 6 | loop-form / merged-return placement / case order (D1-D9) |
| 88.04 | mapcell | `?erase@?$vector@VTreasureData@@V?$allocator@..` | register-homing (why-reg) | 11 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 90.43 | mapcell | `?StampObject@NewfullMap@@QAEXPAVNewmapCell@@..` | control-flow (why-branch) | 164 | 48 | loop-form / merged-return placement / case order (D1-D9) |
| 90.72 | mapcell | `?erase@?$vector@VMonsterData@@V?$allocator@V..` | inliner (predict-inline) | 13 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 90.88 | mapcell | `?get_trigger_cell@NewmapCell@@QAEPAV1@XZ` | register-homing (why-reg) | 37 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 91.19 | mapcell | `?erase@?$vector@VCObjectType@@V?$allocator@V..` | inliner (predict-inline) | 57 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 91.61 | mapcell | `?readHeroData@NewfullMap@@QAEHPAVTAbstractFi..` | register-homing (why-reg) | 393 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 91.89 | mapcell | `?Save@NewfullMap@@QAEHPAVTAbstractFile@@HE@Z` | inliner (predict-inline) | 116 | 63 | callee expanded on one side only (A8/A9/A12): 2 over-inline (1 name-unresolvable pair(s) discounted) |
| 93.01 | mapcell | `?readBlackBox@NewfullMap@@QAEHPAVTAbstractFi..` | control-flow (why-branch) | 304 | 28 | loop-form / merged-return placement / case order (D1-D9) |
| 93.32 | mapcell | `?readSpellScrollData@NewfullMap@@QAEHPAVTAbs..` | register-homing (why-reg) | 60 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.48 | mapcell | `?get_map_object@NewmapCell@@QAE?AW4TAdventur..` | register-homing (why-reg) | 4 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 93.66 | mapcell | `?CalculateCellExtra@NewfullMap@@QAEXPAVNewma..` | control-flow (why-branch) | 6 | 21 | loop-form / merged-return placement / case order (D1-D9) |
| 94.06 | mapcell | `??4TTownEvent@@QAEAAV0@ABV0@@Z` | control-flow (why-branch) | 17 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 94.36 | mapcell | `??4MonsterData@@QAEAAV0@ABV0@@Z` | control-flow (why-branch) | 19 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 94.86 | mapcell | `?readMapObjects@NewfullMap@@QAEHPAVTAbstract..` | inliner (predict-inline) | 190 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (31 name-unresolvable pair(s) discounted) |
| 95.00 | mapcell | `?loadBlackBox@NewfullMap@@QAEHPAVTAbstractFi..` | control-flow (why-branch) | 84 | 9 | loop-form / merged-return placement / case order (D1-D9) |
| 95.04 | mapcell | `?erase@?$vector@UTScenarioTown@@V?$allocator..` | register-homing (why-reg) | 45 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.16 | mapcell | `?readTownData@NewfullMap@@QAEHPAVTAbstractFi..` | control-flow (why-branch) | 457 | 26 | loop-form / merged-return placement / case order (D1-D9) |
| 95.47 | mapcell | `?readMapLayer@NewfullMap@@QAEHPAVTAbstractFi..` | register-homing (why-reg) | 46 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.70 | mapcell | `?readScholarData@NewfullMap@@QAEHPAVTAbstrac..` | register-homing (why-reg) | 43 | 0 | name a value to steer pseudo order->EAX (B14) / decl order (B6) |
| 95.88 | mapcell | `??1NewfullMap@@QAE@XZ` | inliner (predict-inline) | 16 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 1 over-inline (8 name-unresolvable pair(s) discounted) |
| 96.16 | mapcell | `?is_diggable@NewmapCell@@QAEEXZ` | control-flow (why-branch) | 3 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 96.26 | mapcell | `??4TTimedEvent@@QAEAAV0@ABV0@@Z` | control-flow (why-branch) | 13 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 96.47 | mapcell | `?readMonsterData@NewfullMap@@QAEHPAVTAbstrac..` | inliner (predict-inline) | 122 | 1 | callee expanded on one side only (A8/A9/A12): 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 96.50 | mapcell | `?copy@std@@YIPAVTTimedEvent@@PAV2@00@Z` | control-flow (why-branch) | 13 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 96.71 | mapcell | `??4TScenarioTown@@QAEAAU0@ABU0@@Z` | control-flow (why-branch) | 16 | 2 | loop-form / merged-return placement / case order (D1-D9) |
| 96.75 | mapcell | `?GenerateHeightMap@NewfullMap@@QAEXPBVCObjec..` | register-homing (why-reg) | 30 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 96.86 | mapcell | `?insert@?$vector@VTTimedEvent@@V?$allocator@..` | inliner (predict-inline) | 88 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
| 96.98 | mapcell | `?readObject@NewfullMap@@QAEHPAVTAbstractFile..` | inliner (predict-inline) | 306 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 97.61 | mapcell | `?erase@?$vector@VTTownEvent@@V?$allocator@VT..` | inliner (predict-inline) | 43 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 98.57 | mapcell | `??4BlackBoxData@@QAEAAV0@ABV0@@Z` | inliner (predict-inline) | 27 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (25 name-unresolvable pair(s) discounted) |
| 98.58 | mapcell | `?erase@?$vector@VBlackBoxData@@V?$allocator@..` | inliner (predict-inline) | 27 | 0 | callee expanded on one side only (A8/A9/A12): 2 under-inline, 2 over-inline (2 name-unresolvable pair(s) discounted) |
| 98.59 | mapcell | `?erase@?$vector@VTTimedEvent@@V?$allocator@V..` | inliner (predict-inline) | 3 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (1 name-unresolvable pair(s) discounted) |
| 98.85 | mapcell | `?insert@?$vector@VMonsterData@@V?$allocator@..` | inliner (predict-inline) | 36 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (3 name-unresolvable pair(s) discounted) |
| 99.47 | mapcell | `?Read@TTimedEvent@@QAEHPAVTAbstractFile@@H@Z` | inliner (predict-inline) | 3 | 0 | callee expanded on one side only (A8/A9/A12): 1 under-inline, 1 over-inline (2 name-unresolvable pair(s) discounted) |
