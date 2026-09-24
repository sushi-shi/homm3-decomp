# Configuration

- [`project.toml`](project.toml): target executable, toolchain and build settings.
- [`units.toml`](units.toml): source units, modules and compiler profiles.
- [`match_baseline.tsv`](match_baseline.tsv): generated per-function CUR/MAX/HIST ledger.

| Directory | Contents |
| --- | --- |
| [`retail/`](retail/) | Reviewed function boundaries, vtables, funclets, initialization thunks, relocations, runtime and zlib identities, and anonymous-namespace paths. |
| [`retail/h3maped/`](retail/h3maped/) / [`retail/h3ccmped/`](retail/h3ccmped/) | Editor seed function spans and reviewed CRT identities; partial inventories separate from game data. |
| [`source/`](source/) | Accepted address-claim exceptions. |
| [`cleanliness/`](cleanliness/) | Source-cleanliness baseline and reviewed source-local classes. |
| [`matching/`](matching/) | Reviewed banked-match withdrawals and parked matching candidates. |

Source annotations own function identities. The [ownership gate](../docs/matching/source-ownership.md)
checks active definitions and extracted retail claims. Source-local class
exceptions are reviewed in [`cleanliness/local_classes.tsv`](cleanliness/local_classes.tsv).

Table headers describe ownership and update rules. Retail inventories are
reviewed inputs; generated analysis and scratch output belong in `build/`.
