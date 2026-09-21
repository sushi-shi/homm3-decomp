# Manual HIST recovery

The batch state sweep deliberately stops at reproducible compiler-state gains.
Rows left with `MAX < HIST` need a manual evidence pass: first determine whether
HIST came from faithful source, an older source-false workaround, or a compiler
state that the bounded sweep did not visit. A higher fuzzy score alone does not
authorize a source change.

For `townManager::townManager`, all 24 orders of the four adjacent authentic
mask/count/type assignments were compiled as a source-shape matrix. The best
score was 99.32%, but it moved the scalar stores ahead of both 64-bit mask
assignments and contradicted the Dreamcast statement order. It was rejected.
The Dreamcast public name `castleOpen` also proves a future semantic rename of
`g_unnamed6aa9d8` to `g_castleOpen`; a disposable rename was byte-flat for the
constructor, so it was not misreported as a HIST recovery.

A recovered example is `setElevationToggleImage`. Complete's
retail body stores `g_elevationToggleLevel` before loading the indexed icon
pointer, whereas the prior source assignment forced the opposite schedule.
Moving that assignment below the store makes all three blocks exact while
preserving the existing `message` local and every call.

The adjacent `setSleepImage` row exposed why historical score alone is not
source evidence.  Its four-byte same-class Dreamcast stub says nothing about
the Complete body, but the older real `TAdvMenu::SetSleepImage` body explicitly
calls `button::clear_hotkeys` and `button::set_hotkey` on consecutive source
lines.  The reconstruction had flattened the former into direct vector access.
Restoring the one-call header wrapper is byte-flat in that experiment's compiler
state, while `predict-inline` still identifies one extra out-of-line
`vector<int>::size` in the candidate.  Twelve target-local state families did
not recover the 100% HIST, so the helper is retained and the nested inliner difference needs fresh measurement before another probe.
