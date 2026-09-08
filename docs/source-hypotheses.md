# Source-hypothesis batches

`homm3 hypotheses` adapts King's Field's `kf hypotheses` manifest and Cartesian
product runner to the pinned VC6 profiles and HoMM3's object normalization.
Use it after the Dreamcast and retail evidence pass required by `AGENTS.md`.

```sh
homm3 build --fast customcampaign
homm3 hypotheses build/parked-batches/crossover-icon.json -j 8 --keep-top 4
```

A schema-1 manifest names a configured `unit`, the exact mangled `function`
from `config/match_baseline.tsv`, and independent substitution `axes`:

```json
{
  "schema": 1,
  "unit": "customcampaign",
  "function": "?getIconDefName@TCampaignStartCrossoverOption@@UBEPBDPAXH@Z",
  "axes": [{
    "name": "first_hero",
    "find": "hero* first = pool.size() != 0 ? pool.begin() : 0;",
    "options": [
      {"name": "baseline"},
      {"name": "indexed", "replace": "hero* first = pool.size() != 0 ? &pool[0] : 0;"}
    ]
  }]
}
```

Each `find` must occur exactly once in the original source. An option without
`replace` leaves its span unchanged. Optional `extra_edits` are exact
`{"find": "...", "replace": "..."}` substitutions in the same source that
travel atomically with that option. Edits in different axes cannot overlap.
The default bound is 256 combinations; `--limit` changes it. Identical generated
sources are compiled only once. Author meaningful source hypotheses; the runner
does not infer semantics or justify arbitrary code-generation tricks.

Each state compiles a disposable source with the unit's configured flags in an
isolated directory, then scores all currently scored functions in that TU using
the same normalization and objdiff report machinery as the existing state sweep.
It preserves objects, reports, per-state scores, the input manifest, ranked
`results.json`, and `--keep-top` candidate sources under `build/hypotheses/`.
Changes to source, headers, compiler/scoring inputs, target or ledger invalidate
the run. It never edits or banks production source.

Inspect the winning source and collateral scores, apply supported changes,
reproduce with `homm3 build --fast TU`, then run the full `homm3 build` checkpoint.
The example records the successful crossover-icon probe; its old `find` span
intentionally stops matching after the winning source is applied.
