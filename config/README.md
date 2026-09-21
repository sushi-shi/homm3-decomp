# Configuration

- [`project.toml`](project.toml): target executable, toolchain and build settings.
- [`units.toml`](units.toml): source units, modules and compiler profiles.
- [`match_baseline.tsv`](match_baseline.tsv): generated per-function CUR/MAX/HIST ledger.

| Directory | Contents |
| --- | --- |
| [`retail/`](retail/) | Reviewed function boundaries, vtables, funclets, initialization thunks, relocations, runtime and zlib identities, and anonymous-namespace paths. |
| [`source/`](source/) | Exact Dreamcast/Windows source exclusions, Dreamcast audit suppressions and accepted address-claim exceptions. |
| [`cleanliness/`](cleanliness/) | Source-cleanliness baseline. |
| [`matching/`](matching/) | Reviewed banked-match withdrawals and parked matching candidates. |

For source review, start with [`source/dc_only.tsv`](source/dc_only.tsv) and
[`source/win_only.tsv`](source/win_only.tsv). Compiler/library entries and whole
Windows-only modules remain in their separate companion tables.

Table headers describe ownership and update rules. Retail inventories are
reviewed inputs; generated analysis and scratch output belong in `build/`.
