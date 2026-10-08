# Configuration

This branch's target is the Loki Linux `h3maped` image:

- [`loki/toolchain.toml`](loki/toolchain.toml): the pinned GCC 2.95.2 compile toolchain and the link media.
- [`loki/units.toml`](loki/units.toml): the 103 project units, their sources and flags.
- [`loki/match_baseline.tsv`](loki/match_baseline.tsv): the generated Loki CUR/MAX/HIST ledger.
- [`loki/patches/`](loki/patches/): source patches the link recipe applies, reconstructed from the
  image (zlib's 1.0.4 copyright line).
- [`retail/h3maped-loki/`](retail/h3maped-loki/): the image's object and function census,
  anonymous-namespace names, the X library symbols the stub link libraries are generated from
  (`x-imports.tsv`) and the stated link differences (`link-differences.toml`, none today).
- [`project.toml`](project.toml): pinned executable inputs. The shared input tooling
  still reads the Windows, Dreamcast and Mac pins; their data lives on
  `decomp-complete-4.0`.

The remaining Windows game tables (`units.toml`, `retail/*.tsv`, `source/`,
`cleanliness/`, `matching/`) are kept for the shared tooling and its tests.
Table headers describe ownership and update rules. Retail inventories are
reviewed inputs; generated analysis and scratch output belong in `build/`.
