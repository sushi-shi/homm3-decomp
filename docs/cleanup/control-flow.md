# Control-flow cleanup

This pass follows the HoMM1 `docs/reconstruction-debt.md` goto review and
HoMM2 commit `96352ca0cd444903742e4f2184d4a7668c828578` (move assignments
before their conditions). Counts are review inputs, not evidence of source
defects. Preserve operations, helper calls, lifetime boundaries and native
interfaces; verify the authored change with its configured retail compiler.

The starting revision is `eb0115861`. The earlier H3
[goto audit](../reconstruction/goto-audit.md) records prior probes, including
`CSaveGameEdit::ignoreKey` direct-return and PCX shared-copy variants. Those
results belong to their stated compiler contexts, and are not overwritten
by this review. The worktree and generated experiments
are on the NVMe filesystem at `/tmp/homm3-cleanup-flow-20261102`.

## Reviewed candidates

The final cleanup retains four assignments moved before their conditions in
`hasMultipleAdjSimilarSpans`, which stays MAX 100%. The worker's game-only
comparison also kept `_setTriggerMask` at MAX 100% after replacing its
nested-search goto with a direct return. Integration rejected that replacement:
the complete `h3maped` comparison lowered the function's source-owned MAX
from 100% to 98.72%. The original goto is restored before publication.

The rejected forms supplied no new native source fact that justified changing
the existing code shape merely to lower a source-construct count. The editor
loss is an authored-source change, not unchanged-source CUR noise. No helper,
declaration, ABI, scan order, or sentinel changes are retained.

| Trial, not adopted | Starting MAX | Trial MAX | Disposition |
| --- | ---: | ---: | --- |
| `TObjectType::_setTriggerMask`: direct return from the nested search | 100% (`h3maped`) | 98.72% (`h3maped`); game remains 100% | Restored at integration; the game-only worker result did not cover the editor. |
| `CSaveGameEdit::ignoreKey`: direct return instead of shared `ignore` label, keeping `GetKeyState` | 100% | 97.54% | Restored; reproduces the historical failed form in the current context. |
| `loadpcx`: give the nibble case its own `memcpy` and `break` | 100% | 84.17% | Restored; the single shared row-copy body remains. |
| `victorReadPcxPalette`: select the header read with `colors != victorPcxExtendedColors` | 100% | 98.12% | Restored; preserve the accepted extended-palette transfer. |
| `TOverviewWindow` constructor: assignment followed by a separate field test | 99.56% | 93.19% | Restored; the new test adds another vector-index operation. |
| Same constructor: Windows unconditional assignment/add, Mac-only comparison and fallback | 99.56% | 95.73% | Restored; source operation simplification changes compiler expansion context. The documented Windows assignment remains, not an equality test. |

Percentages are rounded measured/projected MAX values in the named contexts.
The trigger-return trial was initially banked in the worker's game report;
the integration editor result supersedes that acceptance. The other rejected
forms were restored before the worker checkpoint. `markAIPuzzle` was inspected
but not edited; its guarded alignment read remains a separate lead.

| Owner | Evidence and intended cleanup |
| --- | --- |
| `TObjectType::_setTriggerMask`, Windows `0x00514a60` | The first matching bit assigns `_m_triggerLoc`, then the existing `found` label only reaches `return *this`. A direct return preserves the game match but loses the editor match; retain the existing join and sentinel. |
| `TRmgTerrainPainter::hasMultipleAdjSimilarSpans`, Windows `0x005b6810` | The ring traversal updates a direction modulo `TILE_DIR_COUNT` before comparing with the start. Put each update before its condition; preserve wrap and final-run order. |
| `victorReadPcxPalette`, Windows `0x006043d0` | An accepted extended palette bypasses the header read. Preserve marker validation, the 16-color fallback, positive-handle condition, close/free behavior and conversion ordering when replacing the shared-copy jump. Retail-only library reconstruction. |
| `loadpcx`, Windows `0x00603e00` | The nibble decode jumps to the indexed row copy; complete RGB planes also reach that copy while partial planes continue the enclosing loop. Preserve all three paths and the top-of-function C local declarations. |
| `CSaveGameEdit::ignoreKey`, Windows `0x0057cfe0` | Period returns ignored in both shift states, but the existing `GetKeyState` call must remain. A source simplification must not silently remove that external call. |
| `markAIPuzzle`, DC `0x115838` | The assignment occurs only for a nonnegative player. Any separation must preserve that short circuit and the `-1` to zero fallback. This helper is expanded by Windows callers. |
| `overview.cpp` generator listing | Windows assigns `CREATURE_GENERATOR_1` before the test; Mac compares. This is a documented platform difference, not permission to fix the Windows behavior by replacing `=` with `==`. |

## Validation

Commands and trial output live under ignored `build/cleanup-flow/`:
`first-compare.log`, `second-compare.log`, the overview native evidence pass,
and the source inventory. The source report remains separate from historic
[goto-audit](../reconstruction/goto-audit.md) counts. The worker's six-unit
game comparison reported no MAX movement: its two edited functions remained
100%, and the four restored trial owners retained their starting MAX.
`homm3 status update --write-readme` required a fresh all-unit fast report
after bootstrap; that game report was banked and README regenerated. These
are historical worker results, not complete cross-image validation.

Both changed shared TUs compiled under the pinned `h3maped` profile, but the
worker's partial editor delink failed with `overlapping data manifest
definitions` and missing unrelated header carriers. The coordinator's complete
editor build resolved that coverage gap and exposed `_setTriggerMask`'s
100% to 98.72% source-owned MAX drop (`build/cleanup-land.log`). Landing was
stopped before push, and the original goto restored. The four terrain-scan
condition cleanups remain. No manifest or gate was weakened.

During the worker trial, `homm3 mac build objecttype rmg_terrain` completed.
Seven of its nine available scored pairs are exact; 21 emitted pairs have unresolved references
and 13 claims lack full-TU bodies. Neither edited function received an
available score, so this is a compiler check, not a claim of their Mac byte
identity. No Mac manifest was added or changed.

The final goto total is **31**, unchanged from this pass's starting tree.
The worker's intermediate inventory of 30 was scoped to its direct-return
trial and is superseded by the integration veto. Four assignment expressions
were removed from conditions in the terrain scan.

## Remaining game goto leads

These are explicit follow-up leads, not claims that every retained jump is
necessary. Editor sources are reviewed by the separate editor lane.

| Source | Remaining operation requiring evidence before restructuring |
| --- | --- |
| `adventuremapwindow.cpp` | Hero locator controls and switch cases share hero/town rollover tails; default and out-of-range controls share generic help. Preserve each control range and the final rollover broadcast. |
| `objecttype.cpp` | The terminal first-trigger search join stays: its direct-return replacement was game-exact but lowered the editor to 98.72%. |
| `command.cpp` | Catapult target selection shares order emission. Remote combat action dispatch crosses the `CMessageKill` lifetime before common processing; preserve its destructor point. Existing local comments record prior structured probes. |
| `kb.cpp` | Campaign transitions re-enter `runGame` after setting game-over/start-event state. Recover the enclosing restart loop without repeating preceding campaign UI. |
| `events.cpp` | Auto-combat restores copied hero/army objects before joining `combatFinished`; the alternative path performs normal combat setup. Preserve both ownership and music/draw state. |
| `spells.cpp` | Lightning completion exits nested drawing loops before split generation and reaches shared cleanup. Preserve the cleanup and frame updates. |
| `swapmgr.cpp` | Left/right skill cases set side and index, then share the native skill-description/dialog scope. Keep that common helper path and its lifetime. |
| `townmgr.cpp` | Building-specific switch arms join one popup tail. Review case fallthrough and which cases bypass the popup before moving common work. |
| `soundmgr.cpp` | Driver-open retries, emulated-device handling and sample-rate fallback converge on a driver-result join. The exact current function has documented earlier structured alternatives; do not discard those findings. |
| `rmg_terrain.cpp` | A nested terrain-gap scan exits both loops when direction wraps. Preserve completed gap accounting and the later gap-selection phase. |

No artificial booleans, dummy operations, inlining controls, or source spelling
changes solely to steer compiler state are introduced by this pass.
