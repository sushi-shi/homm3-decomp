# Disposable TU-state sweep

VC6 code generation can move when parser-visible declarations before a
function change, even when the function's own source hash does not. The batch
runner searches that state without retaining synthetic code:

```sh
homm3 vc6 state-sweep --trials 30 --jobs 8 --bank
```

The runner discovers every numeric `CUR < HIST` row and groups them by
translation unit. In each trial it inserts a uniquely named copy of the forest
beside every affected function, then compiles that combined source once. This
preserves Gruntz's target-local placement while one TU compile tests all of its
affected functions. All compiled functions in that TU are scored, so an
unrelated improvement is also eligible for banking. Generated source copies,
objects, reports, and resumable trial records live only under
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

The initial seed-20260906 run on 2026-09-07 covered 128 affected functions in
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

The corrected target-local grouped rerun completed another 1,470 candidate
compiles over the same 49 TUs. It found no additional reproducible MAX gains.
