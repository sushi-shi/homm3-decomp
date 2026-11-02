# PR #202: isolated Mac preservation check

Reviewed head: [5304445fa9623127eea6e330bf9d2a55067bc135](https://github.com/sushi-shi/homm3-decomp/commit/5304445fa9623127eea6e330bf9d2a55067bc135), [PR #202](https://github.com/sushi-shi/homm3-decomp/pull/202).

Fresh baseline and candidate `homm3 mac build adventuremapwindow` both completed successfully in `/tmp/homm3-cleanup-names-20261102` at cleanup commit `d413ed6b6`. The candidate is the identical complete source replacement already tested by the two-state Windows source-family run `eb047bc851e60e3bc1f8`; no new source variants, profiles, TOC manifests or waivers were introduced. This checks the PR delta on the cleanup context, not a complete historical PR-head checkout.

| Measurement | Baseline | PR candidate |
| --- | ---: | ---: |
| Available scored pairs in adventuremapwindow | 20 | 20 |
| Exact scored pairs | 17 | 17 |
| Emitted pairs unavailable | 16 | 16 |
| Windows-VA claims without full-TU bodies | 0 | 0 |
| Scored-pair regressions | — | 0 |
| Lost previously available comparisons | — | 0 |

Every available pair retains precisely its baseline score. The three non-exact pairs remain `CGameChatEdit::onKeyPress` 99.1379%, `TAdventureMapWindow::processRightSelect` 32.5581%, and `updateQuestLogButton` 26.5988%. The same sixteen emitted pairs remain unavailable. Counts exclude the older smackmgr result that the report retains from another unit's earlier invocation.

**The edited constructor itself is unavailable in both states.** Windows VA `0x00401510`, `TAdventureMapWindow::TAdventureMapWindow`, cannot be scored because anonymous TOC literal `@1845` has zero reviewed payload matches. Thus this establishes preservation of the available siblings, not Mac exactness or preservation of the changed constructor. Its independent Windows result remains 98.4879% MAX → 100%; available Mac results neither contradict that source hypothesis nor fully verify it. No new TOC/data manifest was authored to manufacture coverage.

The comparison scope is full-TU function code and reviewed jump tables; exception metadata is not scored. Unavailable comparisons are explicit coverage gaps, not zero differences or passes. No Mac ledger was banked, no source adopted, and no commit created. Authored `src/adventuremapwindow.cpp` was restored byte-for-byte from the saved pre-experiment copy; `git status --short` is empty. The generated Mac object/report still describes the candidate until a future build refreshes it; preserved reports below identify each measured state.

Artifacts under the isolated checkout's `build/pr-review-202/`:

- `mac-baseline.log`, `mac-baseline.json`: fresh unchanged-source measurement.
- `mac-candidate.log`, `mac-candidate.json`: exact PR-source candidate measurement.
- `mac-original.cpp`: byte-for-byte source restoration copy.
- `manifest.json`: the pre-existing Windows two-state candidate definition.
