# Mac preservation review: PRs 196, 201, 205, 206, 207

**PRs #205 and #206 regress an available Mac comparison.** Their previously
verified Windows exact matches do not establish Mac preservation. PRs #196,
#201 and #207 leave every currently scored pair unchanged, but their changed
Windows targets have the coverage limitations described below.

The baseline is the current source at clean cleanup commit
`ebb23b1a84590ea4aa9b54db7a8e586f45a380f3`, compiled in the SSD worktree
`/tmp/homm3-cleanup-types-20261102`. Ran
`homm3 mac build soundmgr army ai_player customcampaign singleselectionwindow`,
then applied each already Windows-verified PR candidate separately to its owning
source and ran `homm3 mac build <unit>`. Restored each source byte-for-byte before
the next candidate. All five commands returned 0; selected-unit builds do not
checkpoint or enforce the full preservation gate, so return code 0 alone is
not a preservation verdict. Compared every available row against this fresh
baseline, not merely against the banked ledger.

## Coverage and results

Each arrow is baseline → isolated PR candidate. “Unavailable” means an emitted
pair could not be scored due to an unresolved reference. “Unscored” means a
Windows-VA claim lacked a full-TU body. Loss counts count baseline scored rows
that ceased to be scored; no such losses occurred.

| PR | Unit | Scored | Exact | Unavailable | Unscored | Scored rows lost | Score drops |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 196 | soundmgr | 3 → 3 | 0 → 0 | 20 → 20 | 0 → 0 | 0 | 0 |
| 201 | army | 56 → 56 | 20 → 20 | 36 → 36 | 0 → 0 | 0 | 0 |
| 205 | ai_player | 67 → 67 | 26 → 26 | 37 → 37 | 0 → 0 | 0 | 1 |
| 206 | customcampaign | 69 → 69 | 34 → 34 | 37 → 37 | 2 → 2 | 0 | 1 |
| 207 | singleselectionwindow | 49 → 49 | 20 → 20 | 83 → 83 | 0 → 0 | 0 | 0 |

Exact counts alone would conceal both regressions. All scored rows other than
the two drops below retain exactly the baseline scores.

## #205: available target regression

- `type_AI_creature_swapper::valueOfAddingArmy`, Windows `0x0042c830`,
  Mac `0:0x302b0`: **23.3805668% → 18.9484127%**. Candidate code size grows
  from 988 to 1008 bytes.
- `type_AI_creature_swapper::chooseWeakestArmy`, Windows `0x0042c690`:
  46.0784314% → 46.0784314%.
- The Mac-only `normalizeAlignment` helper is not a scored row in this
  selected-unit report. Its preservation is not independently established
  by these aggregate counts.

The complete PR candidate fails available Mac score preservation. No waiver,
abstraction annotation, split-hunk adoption, or score banking was performed.
The regression is observed for the full tested PR; this review does not claim
which individual hunk is responsible without a separate isolated experiment.

## #206: available target regression

- `SCampaign::completeCurrentMap`, Windows `0x00489820`, Mac `0:0x98484`:
  **68.2189542% → 67.8104575%**. Candidate size stays 1224 bytes.
- `game::rehomeCampaignHeroSetup`, Windows `0x00486110`:
  30.7352941% → 30.7352941%.

The complete PR candidate fails available Mac score preservation. The two
source edits are independent, but this run does not adopt a partial PR or
claim that a split candidate has been compiled. No waiver was added.

## #196: no changed-target Mac claim

The three scored soundmgr comparisons are unchanged. The edited
`waitEndSampleThread` at Windows `0x0059a6b0` has no source Mac claim and is
absent from the pair report. Its Windows exact match was independently
verified earlier; this Mac run supplies sibling preservation, not a Mac
comparison of that worker function.

## #201: changed target unavailable

All 56 scored army comparisons are unchanged. The edited two-argument
`army::doAttack` at Windows `0x00441610` cannot be scored: the baseline report
says `anonymous TOC literal '@2861' has 0 reviewed payload matches`.
It remains unavailable in the candidate. Thus the PR has no observed scored
Mac regression, but the edited target itself is not Mac-validated.

## #207: changed targets unavailable

All 49 scored selection-window comparisons are unchanged. The three Windows
exact targets remain unavailable:

| Target | Windows VA | Baseline coverage limitation |
| --- | --- | --- |
| `CNewPlayerUpdateProc::finish` | `0x005795a0` | unreviewed UDATA symbol `g_singleSelectionWindow` |
| `TSingleSelectionWindow::getHeaders` | `0x00582b40` | TOC symbol `g_fileSpec` needs one nonrelocatable emitted data payload |
| `TSingleSelectionWindow::setNewPlayerSlot` | `0x0058e700` | TOC symbol `g_gameContextFeatures` needs one nonrelocatable emitted data payload |

The additionally improved Windows `onMapFileNameMsg` (`0x00589710`) also
remains unavailable, with an unresolved `SGameSetupOptions::operator=`
relocation. The Mac-only `getHeaderDirectory` helper is not a scored row in
this report. Native disassembly previously supported its guarded-return
structure, but that evidence and the unchanged available sibling scores do
not replace a scored comparison of these targets.

The same comment-only patch-context adaptation documented in the Windows
review was used for #207; all original code hunks were present.

## Artifacts and final state

All logs, baseline/candidate report snapshots, recovery copies of the original
source, and machine-readable comparisons are outside the authored repository:

- `/tmp/homm3-pr-review-20261102/types-mac/baseline.log`
- `/tmp/homm3-pr-review-20261102/types-mac/baseline-report.json`
- `/tmp/homm3-pr-review-20261102/types-mac/pr-<PR>.log`
- `/tmp/homm3-pr-review-20261102/types-mac/pr-<PR>-report.json`
- `/tmp/homm3-pr-review-20261102/types-mac/results.json`

Each source was restored and the committed Mac ledger hash remained unchanged.
No root worktree was edited, no PR was adopted, and no commit or waiver was
created. Candidate Mac objects/reports are disposable build outputs; restored
source receipts require normal refresh before future use. Windows verification
was not repeated.
