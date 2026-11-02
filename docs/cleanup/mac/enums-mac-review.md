# Mac preservation review: PRs 198, 199, 210

Baseline: clean cleanup commit `6e71ce804`, SSD worktree
`/tmp/homm3-cleanup-enums-20261102`. This reviews the exact downloaded PR
deltas on that integration context, not independent historical-head checkouts.
Pinned ordinary full-TU CodeWarrior profiles and existing reference inventories
were used throughout. No platform forks, waivers, linking manifests, source
adoptions, Windows reruns, or score checkpoints were introduced.

## Results

| PR | Checked units | Scored / exact | Emitted unavailable | Claims without full-TU body | Score losses | Lost scored pairs | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 198 | mapcell, game | 108 / 48 | 127 | 5 | 0 | 0 | Target improves; all other scored pairs preserved |
| 199 | game | 79 / 38 | 80 | 4 | 0 | 0 | Scored collateral preserved; changed targets unavailable |
| 210 | rmg | 67 / 13 | 115 | 16 | 0 | 0 | Scored collateral preserved; changed targets unavailable |

All table counts are unchanged from the corresponding baseline selection.
No previously scored pair disappeared. There were no newly scored pairs.
The only score movement is the improvement below.

### #198

`upgradeCellExtraInfo` (`0x004fe6c0`, Mac section 0 offset `0x1203e8`)
improves **33.3791% → 77.0604%**. The other 107 scored mapcell/game pairs
are unchanged. The two units retain 48 exact pairs, 127 emitted unavailable
pairs, and five claims without a full-TU body. The explicit `__fastcall`
header spelling is neutralized by the existing CodeWarrior prefix; the
game TU was checked as a shared-header consumer. This is a preservation
and improvement result, not a claim that the Mac function is exact.

### #199

All 79 scored game pairs are unchanged (38 exact). The changed filename
`game::newMap` target at `0x004c0520` is **unavailable in both baseline and
candidate**: `anonymous TOC literal '@7863' has 0 reviewed payload matches`.
The other `game::newMap` overload at `0x004bfe70` is available, but its score
is not evidence for this changed overload. The filename overload's Mac
preservation remains unverified; an unchanged set of other scores does not
establish it. Existing Mac scoring also excludes exception metadata.

### #210

All 67 scored RMG pairs are unchanged (13 exact). The changed helper and
both Windows-improved target functions are **unavailable in both states**:

| Windows VA | Function | Existing comparison limitation |
| --- | --- | --- |
| `0x00535040` | `TRmgTreasureGroup::reset` | Unresolved `HUNK_XREF_24BIT` to `.clear__Q23std58__vector_deleter<9TRmgPoint,Q23std21allocator<9TRmgPoint>>Fv` at `0x24` |
| `0x0053bcb0` | `type_random_map_generator::initializeZones` | Unresolved `HUNK_XREF_24BIT` to `.__vc__Q23std38__vector_pod<Ul,Q23std13allocator<Ul>>FUl` at `0x334` |
| `0x00547360` | `type_random_map_generator::placeZoneTreasures` | Anonymous TOC literal `@1039` has zero reviewed payload matches |

`assembleTreasureGroup` (`0x005466e0`) remains available at 100%, but that
does not substitute for the unavailable `placeZoneTreasures` comparison.
The affected functions' own Mac preservation remains unverified.

## Baseline and unavailable inventory

| Unit | Scored | Exact | Emitted unavailable | No full-TU body |
| --- | --- | --- | --- | --- |
| mapcell | 29 | 10 | 47 | 1 |
| game | 79 | 38 | 80 | 4 |
| rmg | 67 | 13 | 115 | 16 |

Total baseline: **175 scored, 61 exact, 242 emitted unavailable, 21 claims
without a full-TU body**. Unavailable rows are coverage gaps, not zero-score
matches or regression waivers. The complete named baseline and per-candidate
unavailable/no-body roster, with the tool's reason for every pair, is in
[`enums-mac-unavailable.tsv`](enums-mac-unavailable.tsv).

## Method and restoration

1. `homm3 mac build mapcell game rmg` established the fresh current-tree baseline.
2. Each already-Windows-tested PR diff was copied into this owned worktree
   in turn. #198 checked `mapcell game`; #199 checked `game`; #210 checked
   `rmg`. Only unit-selected Mac builds were used, so no ledger was banked.
3. A `finally` block restored the exact original source bytes after every
   candidate. No candidate was combined with another PR.
4. A final `homm3 mac build mapcell game rmg` refreshed the restored source.
   It reproduces all 175 original scored-pair values, 61 exact pairs, and
   the original availability set: zero score changes, newly scored pairs,
   or lost scored pairs. `git status --short` is empty.

Experiment artifacts remain under `build/pr-mac-review/`: `baseline.log`
and `.json`, `candidate-198/199/210.log` and `.json`, `summary.json`,
`restored.log`, `restored.json`, and `restored-comparison.json`.
The ordinary source and cleanup commit remain unchanged.
