<!-- # generator: homm3.vc6.report | # date: 2026-08-28 | # ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - regenerate, never hand-edit | plateaus in [50.0, 99.999%); base-vs-delinked-target diagnosis, no recompiles -->
# vc6 plateau diagnosis (read-only; solvers propose, never land)

5 function(s). why-reg = register-homing knobs; why-branch = control-flow knobs; predict-inline = out-of-line CALL multiset divergence (a callee inlined on one side only - dominated by STL basic_string/vector ops + small dtors retail inlines and we do not). CALIBRATION 2026-08-19: this column USED to be dominated by a NAME artifact - retail's side names an unclaimed callee with a synth working label our compiled side can never emit, so one call booked as both an under- and an over-inline and the inliner route (which sits upstream of registers and blocks) buried the true diagnosis. inline_model.divergence now pairs those off by count: on the tree of that date the inliner class fell from 135 rows to 46 of 211, and register-homing (108) overtook it as the dominant plateau class. MECHANISM (RE'd, docs/vc6/inliner.md): /Ob2 budget = clamp(2*caller_cb,1000,35000) spent sequentially; our leaner reconstructions sit at the 1000 floor and STARVE, so retail inlines what we call. FIX = finish the caller's body (budget follows statement mass, byte-inert counts) - do NOT chase _Tidy/vector spellings or pragmas. So on LOW-% rows inline divergence largely self-resolves as reconstruction completes; it is the pure wall only on high-% rows. Mixed walls list both distances.

## Wall-class summary

- **3** register-homing (why-reg)
- **2** control-flow (why-branch)

| fuzzy | unit | function | wall class | reg-dist | flow-dist | knob to try |
|---|---|---|---|---|---|---|
| 52.23 | cspriteframe | `?DrawTile@CSpriteFrame@@QBEXHHHHPAGHHHHHAAVT..` | control-flow (why-branch) | 1469 | 141 | loop-form / merged-return placement / case order (D1-D9) |
| 74.86 | cspriteframe | `?DrawAdvObjImpl@CSpriteFrame@@QBEXHHHHPAGHHH..` | register-homing (why-reg) | 508 | 0 | spill to dead-parameter slot (B4) |
| 84.72 | cspriteframe | `?SetPixelFormat@CSpriteFrame@@SIXIII@Z` | register-homing (why-reg) | 44 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
| 93.77 | cspriteframe | `?DrawCreatureImpl@CSpriteFrame@@QBEXHHHHPAGH..` | control-flow (why-branch) | 261 | 5 | loop-form / merged-return placement / case order (D1-D9) |
| 94.35 | cspriteframe | `?Draw@CSpriteFrame@@QBEXHHHHPAGHHHHHAAVTPale..` | register-homing (why-reg) | 169 | 0 | cache-vs-reload a member/local (B13) / homing (B2/B3) |
