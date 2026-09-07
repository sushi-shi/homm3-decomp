# Manual HIST recovery

The batch state sweep deliberately stops at reproducible compiler-state gains.
Rows left with `MAX < HIST` need a manual evidence pass: first determine whether
HIST came from faithful source, an older source-false workaround, or a compiler
state that the bounded sweep did not visit. A higher fuzzy score alone does not
authorize a source change.

This work is stacked on the CUR/MAX/HIST tooling PR. The initial manual tranche
on 2026-09-07 used target-local disposable candidates and retained no synthetic
declarations:

| Target | Manual campaign | Result |
|---|---|---|
| `type_sacrifice_window::createArtifactWidgets` | 240 trials, 12 Gruntz state families | no state above 99.9983% MAX |
| `recruitUnit::main` | 240 trials, 12 Gruntz state families | all states remained 99.0924% |
| `army::doAttack(army*, int)` | 240 trials, 12 Gruntz state families | no state above 98.9251% MAX |
| `townManager::townManager` | exhaustive 511-step typedef-handle phase | all states remained 99.0800% |

For `townManager::townManager`, all 24 orders of the four adjacent authentic
mask/count/type assignments were compiled as a source-shape matrix. The best
score was 99.32%, but it moved the scalar stores ahead of both 64-bit mask
assignments and contradicted the Dreamcast statement order. It was rejected.
The Dreamcast public name `castleOpen` also proves a future semantic rename of
`g_unnamed6aa9d8` to `g_castleOpen`; a disposable rename was byte-flat for the
constructor, so it was not misreported as a HIST recovery.

The next pass should work from retail/Dreamcast structure rather than repeat
these state or ordering probes. In particular, the constructor's only residual
is the scheduling of two upper-dword zero stores, while the sacrifice function's
only residual remains its already-documented shared constructor-store order.
