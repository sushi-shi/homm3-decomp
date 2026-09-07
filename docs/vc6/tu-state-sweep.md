# Disposable TU-state sweep

VC6 code generation can move when parser-visible declarations before a
function change, even when the function's own source hash does not. The batch
runner searches that state without retaining synthetic code:

```sh
homm3 vc6 state-sweep --trials 30 --jobs 8 --bank
```

The runner discovers every numeric `CUR < HIST` row, groups them by translation
unit, and compiles each forest once per TU. All compiled functions in that TU
are scored, so an unrelated improvement is also eligible for banking. Generated
source copies, objects, reports, and resumable trial records live only under
`build/tu-state-sweep/`.

Banking fails closed:

- each prospective improvement is recompiled and must reproduce to four score
  decimals;
- authored source files and `config/match_baseline.tsv` must not change during
  the sweep;
- a row with a function hash must still have that exact live hash;
- CUR remains the clean canonical-build score;
- only MAX is raised, with HIST raised too if the observation is a new all-time
  peak;
- no synthetic declaration is copied into authored source.

The seed-20260906 run on 2026-09-07 covered 128 affected functions in 49 TUs:
30 forests per TU, 1,470 candidate compiles. Two new observations reproduced:

| TU | Function | Previous MAX | Reproduced MAX | Trial |
|---|---|---:|---:|---:|
| `army` | `army::doAttack(army*, int)` | 98.8868% | 98.9251% | 29 |
| `recruit` | `recruitUnit::update(unsigned char, long)` | 94.1574% | 96.5558% | 24 |

`CEnterNameEdit::onKillFocus`, banked immediately before the batch run, is the
known exact control: the same generator's trial 2 reproduces 100% while its
clean CUR remains 99.8710%.
