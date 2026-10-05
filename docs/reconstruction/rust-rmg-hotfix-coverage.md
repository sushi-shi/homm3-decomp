# Rust RMG hotfix coverage

Source audit of implementation commit `668cb01fa`, against its merged C++ base.
Two reviewers examined every explicit `HOMM3_RMG_HOTFIX` conditional: 45 in
`src/rmg.cpp`, five in `include/rmg.h`, and the declaration gate in
`include/rmg_request.h`. No missing or mismatched implementation was identified.
This establishes source coverage, not executed behavioral parity. See the
[architecture reference](rust-rmg.md) for the compatibility contract and limits.

Rust paths below are relative to `tools/homm3-rmg/src`. C++ line numbers identify
the audited commit; operation names remain useful after source movement.

| C++ branch | Policy or operation | Rust implementation |
| --- | --- | --- |
| `rmg.cpp:15` | Include ownership support | Rust ownership; no runtime branch |
| `1521,1550` | Delete removed tent/quest parents, preserve active parents | `placement/treasure_completion.rs`, `ObjectArena::retire` |
| `2408,2447` | Admit prototype dimensions, bottom footprint and creature subtype; hole exemption | `prototype.rs`, `Prototype::hotfix_admission` and catalog filtering |
| `2459` | Empty monster-family sorting | `PrototypeSource::prepare` |
| `2495,2571,2578` | Count placement rows through the first blank | `PlacementRules::parse` |
| `2540,2550` | Skip incomplete and invalid placement rows | `PlacementRules::parse` |
| `2606` | Keep original row indexes and all score columns | `PlacementRule::source_row`, score lookup |
| `2969` | Initialize tent cursor to light blue | `placement/registration.rs`, `prepare_catalog` |
| `3138` | Admit zone sizes, ownership, densities and player slots | `template.rs`, zone/template admission |
| `3219,3272` | Skip absent maximum-size and zone-index fields | `TemplateSource::prepare` |
| `3245` | Count distinct usable player slots | Template admission |
| `3650,3686` | Reject empty layout candidates before drawing | `layout.rs`, position selection |
| `3734,3747` | Own pending zones until successful placement | Local `PositionedZone`, appended after success |
| `3807` | Clamp subdivision range to one, retain draw | `raster.rs`, midpoint subdivision |
| `4430` | Boundary probe has no towns or unused town draw | `boundaries.rs`, radial-site setup |
| `4462,4489` | Water zones have cleared policies and neutral creatures | `boundaries.rs`, `CreaturePreference::water`; template-only placement rules |
| `5048,5059,5070` | Choose fresh eligible path seeds | `placement/connection_paths.rs` |
| `5203` | Return no guard when no rank matches | `prototype/guard.rs`, `select_guard` |
| `5318` | Skip off-map or unassigned guard positions | `placement/guards.rs` |
| `6633` | Resolve initially neutral town alignment | `BoundaryMap::record_primary_town` |
| `7548` | Handle empty road targets after the initial draw | `placement/roads.rs`, `create_roads` |
| `7648,7831` | Require reached river endpoints | Shared endpoint check in `placement/rivers.rs` |
| `7680` | Exclude right-edge coast aliases | `placement/rivers.rs`, outlet admission |
| `7954,8060` | Require owned starting towns | `placement/towns.rs`, starting-town check |
| `8044` | Stop generation after layout failure | `layout.rs` and orchestration error propagation |
| `8157` | Cap template name and enlarge description buffer | `output/header.rs`, `description` |
| `8277` | Disjoint human/computer team masks | `output/header.rs`, player admission |
| `8775` | Do not erase an absent active object | `placement/registration.rs`, `unregister_object` |
| `8805` | Do not erase an absent cell membership | `placement/mutation.rs`, `erase_footprint` |
| `8846,8964` | Supported request and entry gate | `Request::parse`; generation requires the parsed request |
| `8846,8981` | Required prototypes and entry gate | `prototype/readiness.rs`; orchestration Admission stage |
| `rmg.h:67` | Treasure-definition destruction | Enum-owned definitions; no virtual destructor needed |
| `491,514` | Zone/template admission helpers | `template.rs` |
| `1829` | Unknown format fallback | Unknown formats cannot reach the writer after request admission |
| `1924` | Prototype and starting-town checks | Readiness and town checks above |
| `rmg_request.h:72` | Supported-request declaration | `Request::parse` |

The fallback for an unknown format in the native internal writer is unreachable
through the hotfix public entry point. Rust rejects unknown request versions
before generation and uses an exhaustive `MapVersion` match during output.

The source review alone did not resolve executable differences. The corrected
treasure geometry comparison now passes, and nonperturbing retail captures
establish zero-versus-nonzero water-zone town flags. Full rainbow-table parity remains
unproven; the [architecture reference](rust-rmg.md) defines verification requirements.
