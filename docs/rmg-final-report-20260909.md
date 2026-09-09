# RMG integration report — 9 September 2026

## Result

The RMG source-family work and matching-skill update are integrated into
the primary `decomp-complete-4.0` branch. The combined tree
preserves newer main-branch RMG helper recoveries and control-flow cleanups.
This is a finalized work checkpoint, **not a claim that RMG is finished**.

## Verified matching status

Figures come from a full VC6 SP3 build of the combined tree against the pinned
English GOG Complete retail target, using `build/objdiff/report.json` and
`config/match_baseline.tsv`. Counts include tracked template and generated
helper bodies, not only handwritten game methods.

| Unit | Size-weighted fuzzy | Currently exact | Tracked |
| --- | ---: | ---: | ---: |
| `rmg` | 90.55% | 180 | 267 |
| `rmg_support` | 85.26% | 27 | 31 |
| `rmg_terrain` | 90.34% | 49 | 68 |
| **Combined** | **90.34%** | **256 (69.95%)** | **366** |

The combined fuzzy figure is weighted by each unit's retail code size, not
an average of its function percentages. Of 142,416 tracked code bytes,
44,839 (31.48%) belong to currently exact functions; the remaining, generally
larger functions still match partially. This does not contradict 90.34%
fuzzy similarity: one differing instruction makes a whole function non-exact.

259 functions have a verified 100% MAX; three of those are not currently
exact in this combined compiler state. MAX is historical evidence for the
current implementation, not a claim about the bytes emitted now.

### Remaining work

110 tracked functions are currently non-exact:

| Current fuzzy range | Functions |
| --- | ---: |
| 99% to below 100% | 13 |
| 95% to below 99% | 16 |
| 90% to below 95% | 19 |
| 80% to below 90% | 24 |
| Below 80% | 38 |

Large remaining contributors include map-header writing (77.90%), river
creation (79.56%), Voronoi site insertion (45.42%), subterranean-gate
construction (82.10%), branching-path carving (73.02%), terrain transitions
(84.06%), ground connections (78.07%) and road-cost construction (83.72%).
Thus the remainder is substantial algorithm, ownership, inlining and compiler
reconstruction work, not just the last few register choices. Neither the
9.66-point fuzzy gap nor the function count is a reliable time estimate.
Tracked-function statistics also do not establish exhaustive retail inventory
or whole-TU closure.

## Results retained from this work

Six previously non-exact tracked bodies now reach 100% compared with the
RMG branch's initial checkpoint: `connectJunctionEntrance`,
`createWaterZoneIsland`, `TRandomMapRequest::generateToFile`,
`loadObjectPrototypes`, `writeRmgObjectPrototype`, and `TRmgVoronoi::locate`.
Integration also retains main's exact rule-vector helpers and signed-short
vector resize recovery.

Other substantial retained gains include connection-cost flooding
(62.48% → 83.68%), water-zone distance flooding (65.65% → 88.72%), underground
decoration (96.10% → 99.80%) and the generation coordinator
(96.90% → 99.92%). The coordinator's progress constant was corrected from
7000 to the retail value 6900.

The final function inspected was `buildZoneConnectionPaths` (0x5405d0).
Its seven-form terrain-extraction experiment did not improve 99.3582%.
The unchanged reconstruction was retained; the signed extraction and
loop-exit load-order differences are explicitly documented beside it.
`generate()` remains at 99.9228%, with selected-index register and commuted
player-count load differences. Neither function is reported as exact.

## Semantic confidence and limits

**It is not established that everything is semantically correct.**

The native contract tests exercise actual extracted C++ bodies with reduced
owners and scripted helpers. They check such properties as selection and
player mapping, live container bounds, ordered pipeline callbacks, ownership,
serialization, movement and placement decisions. Deliberately incorrect
variants test whether these checks detect relevant faults. The generation
coordinator's extended check covers 101 source forms, 18,432 scenarios per
form plus empty-template exits, and seven rejected negative controls.

These are bounded tests, not a proof over every map, seed, template, allocation
failure or game interaction. No end-to-end deterministic map-output parity
test between a fully linked reconstruction and retail was established here.
Retail normalized instruction matches are strong local evidence, but fuzzy
scores can hide important operand differences and do not quantify semantic
correctness. No source placeholders were found by the final targeted scan of
the three RMG source files; absence of placeholder markers is not proof of
source completeness.

## Verification

The combined full build passes the match/status, retained-RVA, claim,
single-view and cleanliness gates without lowering MAX for an edited source.
Generated matching summaries and admission/remaining-work inventories are
regenerated from the combined state.

- 35 experimental RMG tests pass on the integrated tree.
- The 135-test RMG regression run completes with 131 passes and four stale
  source-recognition errors. These concern main's verified outer terrain-loop
  break, retry-exhaustion break, and simplified entrance policy. The three
  generators now recognize those exact refinements while retaining their
  historical controls and rejecting altered behavior. All four affected
  checks pass on rerun, together with the adopted terrain-loop behavioral
  oracle: five checks pass in that focused rerun. The entire 135-test suite
  was not rerun after these fixture-only fixes.
- The standalone coordinator check passes all 101 forms and seven negative
  controls after integration.
- `git diff --check` passes.

The skill update is commit `b77473dd`; the main RMG work checkpoint is
`e7995d9b`. Merge commits preserve both these changes and main's intervening
work. The unrelated untracked `src/build/` directory in the primary worktree
is not part of this integration and is left untouched. No remote push is
performed.
