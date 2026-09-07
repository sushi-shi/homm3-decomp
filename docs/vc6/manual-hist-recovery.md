# Manual HIST recovery

The batch state sweep deliberately stops at reproducible compiler-state gains.
Rows left with `MAX < HIST` need a manual evidence pass: first determine whether
HIST came from faithful source, an older source-false workaround, or a compiler
state that the bounded sweep did not visit. A higher fuzzy score alone does not
authorize a source change.

This work is stacked on the CUR/MAX/HIST tooling PR. The initial manual tranche
on 2026-09-07 used target-local disposable candidates and retained no synthetic
declarations:

The preceding random-include sweep covered all 128 original recovery targets
with 100 TU-state samples each. It reproduced two already-banked MAX values but
found no new peak, leaving 125 `MAX < HIST` rows for evidence-led manual work.
The complete movement census is in [tu-state-sweep.md](tu-state-sweep.md).

| Target | Manual campaign | Result |
|---|---|---|
| `type_sacrifice_window::createArtifactWidgets` | 240 trials, 12 Gruntz state families | no state above 99.9983% MAX |
| `recruitUnit::main` | 240 trials, 12 Gruntz state families | all states remained 99.0924% |
| `army::doAttack(army*, int)` | 240 trials, 12 Gruntz state families | no state above 98.9251% MAX |
| `townManager::townManager` | exhaustive 511-step typedef-handle phase | all states remained 99.0800% |
| `TAdventureMapWindow::setElevationToggleImage` | retail-ordered source A/B | 96.9512% -> exact |
| `TAdventureMapWindow::setSleepImage` | helper-boundary repair plus 240 trials, 12 state families | source repaired; all states remained 86.6667% |

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

The first retained manual recovery is `setElevationToggleImage`. Complete's
retail body stores `g_elevationToggleLevel` before loading the indexed icon
pointer, whereas the prior source assignment forced the opposite schedule.
Moving that assignment below the store makes all three blocks exact while
preserving the existing `message` local and every call.

The adjacent `setSleepImage` row exposed why historical score alone is not
source evidence.  Its four-byte same-class Dreamcast stub says nothing about
the Complete body, but the older real `TAdvMenu::SetSleepImage` body explicitly
calls `button::clear_hotkeys` and `button::set_hotkey` on consecutive source
lines.  The reconstruction had flattened the former into direct vector access.
Restoring the one-call header wrapper is byte-flat at the current compiler
state, while `predict-inline` still identifies one extra out-of-line
`vector<int>::size` in the candidate.  Twelve target-local state families did
not recover the 100% HIST, so the helper is retained and the remaining nested
inliner residual stays open.
