# Disposable TU-state sweep

VC6 code generation can move when the translation unit's include population
changes, even when a function's own source hash does not. The batch runner
searches that state without changing authored source:

```sh
homm3 vc6 state-sweep --trials 30 --jobs 8 --bank
```

The runner discovers every numeric `MAX < HIST` row and groups them by
translation unit. In each trial it chooses five to ten project headers that the
TU does not include directly or transitively, shuffles their order, inserts
that include block once after the TU's initial include block, and compiles the
TU once. Every configured function in the candidate object is scored. Per-trial
records keep the selected header order and every function score; the summary
records every function whose score moved, including drops, rather than
reporting only MAX gains. Incompatible header combinations are recorded as
failed trials without aborting other TUs. Generated source copies, objects,
reports, and resumable trial records live only under `build/tu-state-sweep/`.

Banking fails closed:

- each prospective improvement is recompiled and must reproduce to four score
  decimals;
- authored source files and `config/match_baseline.tsv` must not change during
  the sweep;
- headers, compiler/profile, retail targets and scoring code must also stay
  unchanged; their content fingerprints invalidate cached trials after changes;
- a row with a function hash must still have that exact live hash;
- CUR remains the clean canonical-build score;
- only MAX is raised, with HIST raised too if the observation is a new all-time
  peak;
- no experimental include is copied into authored source.

The retired declaration-forest implementation's initial seed-20260906 run on
2026-09-07 covered 128 affected functions in
49 TUs: 30 forests per TU, 1,470 candidate compiles. It inserted only above the
earliest affected function and is retained here as an implementation-history
result; the target-local grouped rerun supersedes it. Two observations from the
initial run reproduced:

| TU | Function | Previous MAX | Reproduced MAX | Trial |
|---|---|---:|---:|---:|
| `army` | `army::doAttack(army*, int)` | 98.8868% | 98.9251% | 29 |
| `recruit` | `recruitUnit::update(unsigned char, long)` | 94.1574% | 96.5558% | 24 |

`CEnterNameEdit::onKillFocus`, banked immediately before the batch run, is the
known exact control: the same generator's trial 2 reproduces 100% while its
clean CUR remains 99.8710%.

Its corrected target-local grouped rerun completed another 1,470 candidate
compiles over the same 49 TUs. It found no additional reproducible MAX gains.
The current include-set implementation supersedes that search strategy.

Before integration onto `31446de4`, the generator-v5 seed-20260906 include-set
census on 2026-09-07 ran 100 states for each of
the same 49 TUs. All 4,900 candidate TUs compiled and produced 307,500
function-score observations. Each of the original 128 recovery targets has 100
samples. Thirteen functions moved in at least one state; the full extrema are:

| TU / function | CUR | Low | High | MAX | HIST |
|---|---:|---:|---:|---:|---:|
| `army::doAttack` | 98.8868 | 98.8868 | 98.9251 | 98.9251 | 99.9962 |
| `CampaignHeaderStruct::load` | 53.9868 | 53.9715 | 53.9868 | 53.9868 | 80.9474 |
| `advManager::monstersSellOut` | 100.0000 | 99.9517 | 100.0000 | 100.0000 | 100.0000 |
| `hero::giveArtifact` | 76.3360 | 73.5951 | 76.3360 | 76.3360 | 76.3360 |
| `recruitUnit::update` | 94.1574 | 94.1574 | 96.5558 | 96.5558 | 96.5558 |
| `type_random_map_generator::createGroundConnection` | 79.1717 | 77.4437 | 79.1717 | 79.1717 | 80.4347 |
| `type_random_map_generator::filterZonePositions` | 94.4493 | 59.2669 | 94.4493 | 94.4493 | 94.4493 |
| `type_random_map_generator::repairWaterZoneBorders` | 100.0000 | 77.8633 | 100.0000 | 100.0000 | 100.0000 |
| `type_random_map_generator::scoreObjectPlacement` | 84.6788 | 76.1434 | 84.6788 | 84.6788 | 84.6788 |
| `vector<TRmgZoneConnection>::size` | 100.0000 | 0.0000 | 100.0000 | 100.0000 | 100.0000 |
| `type_random_map_generator::writeMapHeader` | 93.2864 | 93.2786 | 93.2864 | 93.2864 | 95.7066 |
| `type_sacrifice_window::createArtifactWidgets` | 99.9983 | 99.5918 | 99.9983 | 99.9983 | 100.0000 |
| `CEnterNameEdit::onKillFocus` | 100.0000 | 99.8710 | 100.0000 | 100.0000 | 100.0000 |

Only `army::doAttack` and `recruitUnit::update` rose above CUR, and both merely
reproduced their already-banked MAX. No observation exceeded MAX, so this run
made no ledger change. The runner's JSON summary retains the exact mangled
identities, trial numbers, and ordered header sets for both extrema.
