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

Mac function addresses live in source, beside the Windows claim:
`VA(0x004d8720, 0x568) MAC_ADDRESS(0x0f3fe4, 0x568)`, or on its own line above
a definition with no Windows VA. Offsets are relative to the PEF code section;
the macro claims functions only. Mirroring `retail/`, [`mac/functions.tsv`](mac/functions.tsv)
holds every verified code span, [`mac/runtime-map.tsv`](mac/runtime-map.tsv)
and [`mac/runtime-aliases.tsv`](mac/runtime-aliases.tsv) label library code,
and [`mac/dispositions.tsv`](mac/dispositions.tsv) records evidenced reasons a
source function has no Mac body. Every source claim and runtime label must be
one `functions.tsv` row. `homm3 mac migrate` copies reviewed TOML spans into
these forms; `homm3 mac parity` validates them and reports every source
function as located, unlocated or disposed (`build/gen/mac/parity.tsv`).

Mac candidates reuse the owning source file's include prefix and read ordinary
project headers with the native CodeWarrior library. Duplicate declaration
headers and the body-extraction manifest have been removed. The remaining
[`source/header-fragments.toml`](source/header-fragments.toml) records ordinary
shared headers that still need include-site ownership.
