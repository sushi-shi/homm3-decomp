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

A sweep can reproduce an existing peak without finding a new one. For rows
still below HIST, verify whether the old result used faithful source before
trying to recover it. See [historical match evidence](../reconstruction/historical-matches.md).
