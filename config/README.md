# Configuration

- [`project.toml`](project.toml): pinned executable inputs, toolchain and build settings.
- [`units.toml`](units.toml): source units, modules and compiler profiles.
- [`match_baseline.tsv`](match_baseline.tsv): generated per-function CUR/MAX/HIST ledger.
- [`retail/data-extents.tsv`](retail/data-extents.tsv): reviewed retail data extents and boundary evidence; source annotations continue to own names.

| Directory | Contents |
| --- | --- |
| [`retail/`](retail/) | Reviewed function boundaries, vtables, funclets, initialization thunks, relocations, runtime and zlib identities, and anonymous-namespace paths. |
| [`mac/`](mac/) | Reviewed Windows-VA/DATA to Mac PEF pairs, verified runtime destinations and constants, pinned CodeWarrior profile, campaign deferrals and separate Mac CUR/MAX/HIST ledger. |
| [`source/`](source/) | Exact Dreamcast/Windows source exclusions, Dreamcast audit suppressions and accepted address-claim exceptions. |
| [`cleanliness/`](cleanliness/) | Source-cleanliness baseline. |
| [`matching/`](matching/) | Reviewed banked-match withdrawals and parked matching candidates. |

For source review, start with [`source/dc_only.tsv`](source/dc_only.tsv) and
[`source/win_only.tsv`](source/win_only.tsv). Compiler/library entries and whole
Windows-only modules remain in their separate companion tables.

Table headers describe ownership and update rules. Retail inventories are
reviewed inputs; generated analysis and scratch output belong in `build/`.

Mac additions use `mac/units/<TU>.toml` for a shared compilation profile and
`mac/functions/<TU>.toml` for reviewed spans. Existing aggregate files remain
supported, but duplicate unit profiles, symbols, VAs and overlapping spans
are rejected. See the [Mac extension workflow](../docs/tooling/mac-matching-roadmap.md#extending-a-unit).

`mac/references/<TU>.toml` records reviewed source-owned callees that resolve
calls without adding scored targets. `mac/data/<TU>.toml` lets workers extend
source-owned external storage bindings and compiler literal pools independently.
Identical shared references coalesce; conflicting identities or spans fail.
Literal `units` scopes distinguish equal payloads in different linked pools.

[`source/header-fragments.toml`](source/header-fragments.toml) records canonical
header fragments shared by both compiler views. Ownership validation requires
one literal include in the original header and checks source order at that
position; physical definition identity and duplicate checks remain intact.
