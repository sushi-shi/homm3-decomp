# Supplemental Mac review of open PR candidates

This supplements the Windows/source review snapshot of 2026-11-02. Mac is evidence for Windows source reconstruction, not a second game target. Results below distinguish scored pairs from unresolved/unavailable comparisons; an unavailable changed function is not a preservation pass. The independent Windows 100% results do not establish Mac preservation. All comparisons use ordinary source and existing per-TU CodeWarrior profiles, with no new data manifests, platform forks or abstraction waivers.

Workers tested each downloaded PR delta in their own SSD cleanup context, not complete historical PR-head checkouts or the final combined integration tree. They refreshed a targeted unchanged-source baseline, applied the tested candidate, compared all available pairs, then restored authored files. They did not bank Mac scores or commit candidates. Full-TU function code and reviewed jump tables are scored; exception metadata is not scored. Final combined integration gates are separate.

**New preservation findings:** reject #192; hold #206 and #209; #205 requires a source-backed Mac-preservation decision before adoption. All four have edited-target Mac losses despite Windows 100%. Unlike the optional local rewrites, #205 restores the supported `game::getAlignment` helper: preserve that recovery while resolving its loss, rather than flattening it for a score. No waiver is recorded or implied. Other unavailable targets remain explicitly unverified.

## Decisions and coverage

| PR | Baseline → candidate available pairs | Emitted unavailable pairs | Exact pairs | Scored regressions / availability losses | Changed-target result and disposition |
| --- | --- | --- | --- | --- | --- |
| [#192](https://github.com/sushi-shi/homm3-decomp/pull/192) | Target available; aggregate not reported in focused review | Not enumerated | Not enumerated | One demonstrated target regression; aggregate availability not audited here | Reject optional copy: attemptTeleport 79.3785 → 78.4819%; direct-expression baseline retained. |
| [#196](https://github.com/sushi-shi/homm3-decomp/pull/196) | 3 → 3 | 20 → 20 | 0 → 0 | 0 / 0 | Scored siblings preserved; edited worker has no Mac claim. |
| [#197](https://github.com/sushi-shi/homm3-decomp/pull/197) | 141 → 141 (four units) | 135 → 135 | 80 → 80 | 0 / 0 | Available pairs preserved; doQuestLog unavailable due g_questLogWindow. |
| [#198](https://github.com/sushi-shi/homm3-decomp/pull/198) | 108 → 108 (two units) | 127 → 127 | 48 → 48 | 0 / 0 | upgradeCellExtraInfo improves 33.3791 → 77.0604%; other available pairs unchanged. |
| [#199](https://github.com/sushi-shi/homm3-decomp/pull/199) | 79 → 79 | 80 → 80 | 38 → 38 | 0 / 0 | Available siblings preserved; edited newMap unavailable (TOC literal). |
| [#201](https://github.com/sushi-shi/homm3-decomp/pull/201) | 56 → 56 | 36 → 36 | 20 → 20 | 0 / 0 | Available siblings preserved; edited doAttack unavailable (TOC literal). |
| [#202](https://github.com/sushi-shi/homm3-decomp/pull/202) | 20 → 20 | 16 → 16 | 17 → 17 | 0 / 0 | Available siblings preserved; edited constructor unavailable in both states, so changed-target Mac preservation remains unverified. |
| [#203](https://github.com/sushi-shi/homm3-decomp/pull/203) | 58 → 58 | 56 → 56 | 32 → 32 | 0 / 0 | Available siblings preserved; edited processHover unavailable in both states. |
| [#205](https://github.com/sushi-shi/homm3-decomp/pull/205) | 67 → 67 | 37 → 37 | 26 → 26 | 1 / 0 | valueOfAddingArmy 23.3805668 → 18.9484127%; supported helper recovery needs a separate preservation decision. |
| [#206](https://github.com/sushi-shi/homm3-decomp/pull/206) | 69 → 69 | 37 → 37 | 34 → 34 | 1 / 0 | Hold: completeCurrentMap 68.2189542 → 67.8104575%; rehome target unchanged. |
| [#207](https://github.com/sushi-shi/homm3-decomp/pull/207) | 49 → 49 | 83 → 83 | 20 → 20 | 0 / 0 | Available siblings preserved; three Windows targets and collateral onMapFileNameMsg unavailable. |
| [#208](https://github.com/sushi-shi/homm3-decomp/pull/208) | 0 → 0 | 4 → 4 | 0 → 0 | 0 / 0 among available rows | No available linked comparison, including edited constructor; Mac preservation unverified. |
| [#209](https://github.com/sushi-shi/homm3-decomp/pull/209) | 8 → 8 | 32 → 32 | 2 → 2 | 1 / 0 | Hold: edited update 35.7731958763 → 35.7216494845%; seven other scores unchanged. |
| [#210](https://github.com/sushi-shi/homm3-decomp/pull/210) | 67 → 67 | 115 → 115 | 13 → 13 | 0 / 0 | Available siblings preserved; reset/initializeZones/placeZoneTreasures unavailable. |

All five supplemental worker reports are complete and authored source restoration is confirmed. Counts are per experiment and overlap between PRs; do not sum them into a unique-function coverage total. Claims without full-TU bodies are separate from emitted-but-unavailable pairs: #198 has five, #199 four, #210 sixteen, #206 two; the other enumerated candidate checks have zero. These counts are unchanged between their baseline and candidate. The focused #192 rejection report does not enumerate aggregate coverage.

## #192: rejection supersedes earlier isolated Windows acceptance

Head `3b6ad1004a5e2a187586cbc2ae965c36b1634641`. The direct final path-point argument has a freshly reproduced Mac score of 79.3785%, while the PR's extra assignment to `start_point` scores 78.4819% in the coordinator's full Mac gate despite reaching Windows 100%. A separately tested assignment-inside-argument spelling also reaches Windows 100% but drops Mac to 77.4373%; it was restored and not adopted.

Native Mac loads the final path point directly into the argument register, without a final write to the earlier `start_point` home. DC also stages the final argument at distinct stack offsets. Optimized code does not uniquely prove C++ spelling, but these observations support the retained direct expression; the optional copy restores no helper or independently established source fact that warrants its regression. No platform fork or abstraction waiver is justified. The final direct-source restoration reproduces 79.3785% again.

Detailed evidence and artifacts: [pr192-mac-review.md](/tmp/homm3-pr-review-20261102/pr192-mac-review.md), including native disassembly, DC source layout and both experimental/restored compiler logs under `/tmp/homm3-cleanup-flow-20261102/build/cleanup-flow/`. Aggregate TU coverage was not enumerated in this focused rejection report and is not invented here.

## #202: preserved available siblings, constructor coverage gap

Head `5304445fa9623127eea6e330bf9d2a55067bc135`. The identical candidate from the Windows two-state verification was checked at cleanup commit `d413ed6b6`. All twenty available adventuremapwindow scores remain identical (seventeen exact); all sixteen unavailable pair identities remain unavailable; zero claims lack emitted full-TU bodies. The changed constructor, Windows VA `0x00401510`, remains unavailable because anonymous TOC literal `@1845` has zero reviewed payload matches. This does not independently verify the changed constructor's Mac behavior or score.

Detailed evidence: [naming-mac-review.md](/tmp/homm3-pr-review-20261102/naming-mac-review.md). Baseline and candidate logs/JSON are under `/tmp/homm3-cleanup-names-20261102/build/pr-review-202/`. The retained report also contains an older unrelated smackmgr row, excluded from the counts above. Authored source was restored byte-for-byte and the checkout is clean; generated object/report artifacts retain the last candidate until refreshed.


## #203/#208/#209: further coverage gaps and an edited-target regression

Reviewed heads: #203 `ca740ddc858977cdbd5c1ab397e32655d469e70c`; #208 `51bc31aca836fffe7fe797c42d6f1c5917b8dd3c`; #209 `a4747241e2ee6a41f69552d230384ce1cca49d27`.

For #203, `advManager::processHover` (`0x0040e360`) cannot resolve its `type_point::isValid` call relocation at `+0xfc`. All 58 available siblings are unchanged, but the edited target is not verified. For #208, all four unit bodies remain unavailable: constructor anonymous TOC payload (`@2216` baseline/`@2221` candidate), destructor vtable, `doModal`'s `writePrefs` call, and handler click-sample data. Zero scored rows cannot establish preservation. All selected claims emitted full-TU bodies in these three units; missing scores are linking/reference gaps.

For #209, `TGiveResourceWindow::update` (`0x005eaf50`) loses 0.0515463918 percentage points on Mac, 35.7731958763 → 35.7216494845. This is the changed function itself, not unchanged-source collateral. Its quantity alias restores no canonical helper or independently proven original source fact. Hold this candidate rather than waive preservation solely for its Windows gain. No new correction variant was tested in this review.

Detailed report: [flow-mac-review.md](/tmp/homm3-pr-review-20261102/flow-mac-review.md). Baseline/candidate reports, logs and byte-for-byte restoration checks are in `/tmp/homm3-cleanup-flow-20261102/build/cleanup-flow/pr-mac/`. All three sources were restored; no banking/adoption/commit occurred.


## #205/#206: measured losses require source-aware decisions

Reviewed heads: #205 `f79ca22a26db43077d6ddb28082cc654717d0629`; #206 `6fd55e1fc74e19df8f54ccfc7026fb43c031532c`.

#205's `valueOfAddingArmy` (`0x0042c830`) decreases 23.3805668016 → 18.9484126984%; emitted size grows 988 → 1008 bytes. `chooseWeakestArmy` remains 46.0784313725%; every other available comparison is unchanged. The PR restores an existing canonical `game::getAlignment` operation, including its version-0 elemental exception. That supported helper recovery must not be discarded merely to regain a score. Nevertheless, the candidate is not a clean Mac-preservation pass: separate the canonical abstraction from other local/statement changes, preserve the helper, and resolve/document the regression through the normal source-evidence process before adoption. This report neither grants an abstraction waiver nor proposes a platform-specific branch.

#206's `SCampaign::completeCurrentMap` (`0x00489820`) decreases 68.2189542484 → 67.8104575163%, with unchanged 1224-byte emitted size. `game::rehomeCampaignHeroSetup` remains 30.7352941176%. This is a changed-function regression, not a decrease explained by losing available comparisons. Hold the full candidate pending correction or source evidence; the natural assignment-result-reference change can be considered independently. No correction variants were tested here.


## #196/#201/#207: preserved available scores with target gaps

Reviewed heads: #196 `626f09a97e2b8701a02de09df11e41b38cdc9e31`; #201 `fc20da881727bab5a12716363840058cf3e5cf5f`; #207 `d1c7d737633bdf41a06d94ed9925292742e48752`. These and #205/#206 were tested at cleanup commit `ebb23b1a84590ea4aa9b54db7a8e586f45a380f3`.

#196's `waitEndSampleThread` (`0x0059a6b0`) has no Mac claim and is absent from the report; its three available soundmgr siblings are preserved. #201's two-argument `army::doAttack` (`0x00441610`) remains unavailable because anonymous TOC literal `@2861` lacks a reviewed payload match.

#207's three Windows-exact targets remain unavailable: `finish` (`0x005795a0`, unreviewed `g_singleSelectionWindow`), `getHeaders` (`0x00582b40`, unresolved `g_fileSpec` payload), and `setNewPlayerSlot` (`0x0058e700`, unresolved `g_gameContextFeatures` payload). The collateral Windows gain in `onMapFileNameMsg` (`0x00589710`) also lacks a Mac comparison due an unresolved setup-options assignment relocation. Its Mac-only `getHeaderDirectory` helper is not a scored row. The same comment-only patch-context adaptation used in the Windows run retained all original code hunks.

Likewise #205's Mac-only `normalizeAlignment` helper is not a scored row. Its prior native body evidence remains useful but is not an independently measured helper preservation result. The full #205 candidate, rather than a split hunk, produced the reported regression; its cause has not been isolated here.

Detailed report: [types-mac-review.md](/tmp/homm3-pr-review-20261102/types-mac-review.md); machine-readable comparisons and baseline/candidate snapshots are in `types-mac/results.json` and `types-mac/*-report.json` under `/tmp/homm3-pr-review-20261102/`. All five sources were restored byte-for-byte, the tracked tree is clean, and the committed Mac ledger hash is unchanged.

## #197: available callers preserved, changed target still unresolved

Reviewed head `07d26ae16a6b8ca04e17ce803bcfb208307116b8`. Units `questlogwindow`, `advmgr`, `hero`, and `swapmgr` preserve respectively 2, 58, 71 and 10 scored pairs, with 0, 32, 41 and 7 exact. Their emitted-unavailable counts remain 4, 56, 57 and 18; none loses a scored pair or full-TU body.

The edited `doQuestLog` (`0x0052e430`, Mac `0:0x14a280`) is unavailable in both states because `g_questLogWindow` lacks reviewed UDATA binding. Direct emitted-code comparison does **not** show an identical target: its 1048-byte body changes two bytes at `+0x2ff` and `+0x357`, corresponding to a matching temporary store/load moving from stack offset `0xac` to `0xa4`. All 29 symbolic cross-references remain identical. This is the sole changed code hunk across the four units; linked retail scoring cannot determine whether its change improves or worsens the target while that binding remains unresolved. Do not turn the 141 preserved available pairs into a target-preservation claim.

Detailed report: [editor-mac-review.md](/tmp/homm3-pr-review-20261102/editor-mac-review.md). Baseline/candidate logs, reports and emitted hunk snapshots are under `/tmp/homm3-cleanup-editors-20261102/build/pr-review-197/mac/` and `build/pr197-mac-*.log`. Both source/header files were restored byte-for-byte; the worktree is clean and no ledger was banked.

## #198/#199/#210: one target improvement, other target gaps

Reviewed heads: #198 `8d6f0e14708568d4f4ddff31b118e67d24aa9a25`; #199 `2b6cbbba84c8d56ce7764c02df6d33d0c0a22535`; #210 `82ab38093585f3f67ed81319701ddaee430b1e1c`. All were tested separately at cleanup commit `6e71ce804`.

#198's `upgradeCellExtraInfo` (`0x004fe6c0`, Mac `0:0x1203e8`) improves 33.3791 → 77.0604%, while the other 107 scored mapcell/game pairs remain unchanged. It is improved, not Mac-exact. The explicit calling-convention spelling is already neutralized by the CodeWarrior prefix; the game TU was included as a header consumer.

#199's filename `game::newMap` (`0x004c0520`) remains unavailable due anonymous TOC literal `@7863`. The other overload's available score does not verify this changed overload, and EH metadata is outside the Mac scorer's coverage.

#210's `TRmgTreasureGroup::reset` (`0x00535040`) cannot resolve its vector-deleter `clear` call at `+0x24`; `initializeZones` (`0x0053bcb0`) cannot resolve a vector indexing call at `+0x334`; `placeZoneTreasures` (`0x00547360`) lacks a reviewed payload for anonymous literal `@1039`. The exact available `assembleTreasureGroup` is a different function and does not substitute for those targets.

Detailed report: [enums-mac-review.md](/tmp/homm3-pr-review-20261102/enums-mac-review.md), with the full named unavailable/no-body roster in `enums-mac-unavailable.tsv` beside it. Baseline/candidate/restored snapshots are in `/tmp/homm3-cleanup-enums-20261102/build/pr-mac-review/`. A final restored-source build reproduces all 175 baseline scored values (61 exact), with the original 242 emitted-unavailable and 21 no-body claims. Sources are restored, checkout clean, and no scores banked.

## Scope of this supplement

This consolidates all five supplemental worker reports plus the dedicated #192 rejection review: fourteen PRs total (#192 and the thirteen remaining independently Windows-tested candidates #196–#210, excluding blocked #200/#204). It does not re-audit rejected source models or certify the older feature drafts. The coordinator's locally integrated #191/#193/#194/#195/#215 and final all-image gate results are separate from this table; this supplement makes no publication claim. Successful unit-selected build exit codes alone do not enforce the full Mac preservation gate. Every scored row was compared, including non-exact rows: unchanged exact counts would otherwise conceal the #205/#206/#209 losses.
