# Source-hypothesis batches

`homm3 hypotheses` (also available as `homm3 vc6 hypotheses`) adapts King's Field's `kf hypotheses` manifest and
Cartesian source renderer to the configured HoMM3 VC6 profiles and normalized
objdiff scoring. Each option describes a reviewed, exact source substitution.
Independent axes combine; five binary axes produce 32 candidates.

```json
{
  "schema": 1,
  "unit": "bitmap16",
  "function": "?reference@Bitmap16Bit@@QAEXHHHPAG@Z",
  "axes": [{
    "name": "extent_product",
    "find": "    m_imageSize = w * h * 2;",
    "options": [
      {"name": "baseline"},
      {"name": "pixel_size", "replace": "    m_imageSize = w * h * sizeof(unsigned short);"}
    ]
  }]
}
```

```sh
homm3 hypotheses /tmp/reference.json -j 8 --keep-top 10
```

Functions accept exact scored symbols or unique symbol substrings within the
unit. Each `find` must occur exactly once. An option may have atomic
`extra_edits`, each containing `find` and `replace`; axes must not overlap.
Duplicate sources compile once, and `--limit` bounds the Cartesian product
before rendering (default 256).

The runner compiles a canonical baseline, then candidates in parallel, directly
through the TU's normal compiler wrapper. Each compile has its own source and
object directory. All scored functions in that TU are recorded to expose
collateral. Ranked results, input fingerprints, top sources and their objects
live under `build/hypotheses/`, or the new directory specified by `--output`.
`results.json` contains the canonical `baseline`, ranked `results`, source/target
hashes and the shared-input fingerprint. Raw and normalized objects are retained
for the top candidates. Source, ledger, headers, compiler and scoring inputs must stay unchanged during
a batch. Exit 0 means at least one target scores exactly 100%; 1 means no exact
candidate. Malformed manifests and invalidated observations are errors.

Run the required retail/Dreamcast evidence pass before designing hypotheses.
Scores do not authorize changing proven source facts. The runner never applies
or banks a winner; inspect the source and assembly, apply supported changes,
then run the normal focused build and full checkpoint gates. Vendored sources
remain pristine. Compiler-state padding and artificial ODR uses are not source
hypotheses.
