# Mac verification of PRs #203, #208 and #209

All three exact existing source-family candidates were compiled separately in the isolated SSD tree `/tmp/homm3-cleanup-flow-20261102`, after an unchanged-source Mac control build of all three units. Every source file was restored byte for byte against its manifest control. No new source variant, banking, commit, main-tree edit or publication was performed.

| PR | Unit | Scored pairs, control → candidate | Exact pairs | Unavailable emitted pairs | Availability losses | Result |
| --- | --- | --- | --- | --- | --- | --- |
| #203 | advmgr | 58 → 58 | 32 → 32 | 56 → 56 | 0 | All available scores unchanged; edited target unavailable |
| #208 | systemoptionswindow | 0 → 0 | 0 → 0 | 4 → 4 | 0 | Entire unit unavailable for linked byte comparison |
| #209 | tradpost | 8 → 8 | 2 → 2 | 32 → 32 | 0 | Edited target regresses; seven other scores unchanged |

All selected claims emitted full-TU bodies; no selected claim was unavailable because its body was missing. Unavailable rows are linking/reference coverage gaps, not successful comparisons.

## Target-specific verdicts

- **#203 `advManager::processHover`, Windows VA `0x0040e360`: limited Mac verification.** Both control and candidate are unavailable because `.processHover__10advManagerFii` has an unresolved `HUNK_XREF_24BIT` relocation to `.isValid__10type_pointCFv` at `+0xfc`. The 58 other available comparisons are unchanged. Earlier Windows verification improved the target 99.7070% → 100%, but the Mac result does not establish target preservation.
- **#208 `TSystemOptionsWindow::TSystemOptionsWindow`, Windows VA `0x005b1790`: Mac target unverified.** The control lacks a reviewed payload match for anonymous TOC literal `@2216`; the candidate has the same coverage gap under generated label `@2221`. The remaining three unit functions also remain unavailable: destructor vtable payload, `doModal`'s `writePrefs` relocation, and `windowHandler`'s button click-sample data payload. Earlier Windows verification improved 98.3461% → 100%; no Mac score claim can be made.
- **#209 `TGiveResourceWindow::update`, Windows VA `0x005eaf50`: hold for Mac regression.** The candidate changes the score from **35.7731958763% to 35.7216494845%**, a loss of **0.0515463918 percentage points**. This is the edited function's own source change, not unchanged-source collateral. Its seven other available unit comparisons are identical and no availability is lost. Earlier Windows verification improved 99.9819% → 100%. The temporary quantity alias adds no recovered helper or independently proven original source fact that justifies waiving preservation; do not adopt it solely for Windows exactness.

## Evidence and commands

Control: `homm3 mac build advmgr systemoptionswindow tradpost`.
Candidates: `homm3 mac build <unit>` after installing only the corresponding manifest's existing candidate replacement; a `finally` block restored its original source before proceeding to the next PR. Each Mac command completed successfully. Unit-selected builds report measurements without checkpointing; their success is not itself a global Mac-preservation verdict.

Artifacts in the isolated tree's `build/cleanup-flow/pr-mac/`:

- `baseline.log`, `baseline-report.json`: unchanged-source control.
- `pr203.log`, `pr208.log`, `pr209.log` and corresponding `*-report.json`: candidate results.
- `summary.json`: per-unit score/availability differences and restoration checks.
- `pr203-original.cpp`, `pr208-original.cpp`, `pr209-original.cpp`: byte-for-byte restoration copies.
- `check-candidates.py`, `summarize.py`: disposable reproduction orchestration under ignored `build/`.

These conclusions supplement `/tmp/homm3-pr-review-20261102/flow-review.md`; they supersede any implication there that Windows-only verification establishes cross-platform preservation. No manifest additions or Mac-linking inventory work was attempted to conceal the explicitly reported gaps.
