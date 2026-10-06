# Rust RMG hotfix coverage

Source audit of every explicit `HOMM3_RMG_HOTFIX` conditional, first performed
by two reviewers at implementation commit `668cb01fa` and since re-anchored to
the current sources: 51 conditionals in `src/rmg.cpp`, eight in `include/rmg.h`,
and the declaration gate in `include/rmg_request.h`. No missing or mismatched
implementation was identified. The growth from the original 45 and five
conditionals is source movement, not new policy: shared helpers were expanded
into their callers, and hotfix-only helper declarations gained their own gates.
This establishes source coverage, not executed behavioral parity. See the
[architecture reference](rust-rmg.md) for the compatibility contract and limits.

Rust paths below are relative to `tools/homm3-rmg/src`. C++ anchors name the
function, class or file-scope helper enclosing each conditional, with the
retail VA from its `VA(...)` annotation where the function has one; they stay
valid when source lines move. A row may cover several conditionals in one
function.

| C++ anchor | Policy or operation | Rust implementation |
| --- | --- | --- |
| `rmg.cpp` includes | Include ownership support | Rust ownership; no runtime branch |
| `TRmgKeyTentObject::completePlacement` (0x5338E0), `TRmgQuestArtifactObject::completePlacement` (0x533A50) | Delete removed tent/quest parents, preserve active parents | `placement/treasure_completion.rs`, `ObjectArena::retire` |
| `isUsableRmgPrototype`; its filter in `TRmgGeneratorBase::loadObjectPrototypes` (0x536200) | Admit prototype dimensions, bottom footprint and creature subtype; hole exemption | `prototype.rs`, `Prototype::hotfix_admission` and catalog filtering |
| `TRmgGeneratorBase::loadObjectPrototypes` (0x536200), monster guard | Empty monster-family sorting | `PrototypeSource::prepare` |
| `countRmgPlacementRuleRows`; row count and end test in `TRmgGeneratorBase::readObjectPlacementRules` (0x536560) | Count placement rows through the first blank | `PlacementRules::parse` |
| `TRmgGeneratorBase::readObjectPlacementRules` (0x536560), row admission | Skip incomplete and invalid placement rows | `PlacementRules::parse` |
| `TRmgGeneratorBase::readObjectPlacementRules` (0x536560), score columns | Keep original row indexes and all score columns | `PlacementRule::source_row`, score lookup |
| `TRmgGenerator::TRmgGenerator` (0x537B10) | Initialize tent cursor to light blue | `placement/registration.rs`, `prepare_catalog` |
| `TRmgTemplateZone::isUsable`, `TRmgTemplate::isUsable` and helpers | Admit zone sizes, ownership, densities and player slots | `template.rs`, zone/template admission |
| `TRmgGenerator::loadTemplates` (0x537FF0), `readRmgTemplateZones` (0x538480) | Skip absent maximum-size and zone-index fields | `TemplateSource::prepare` |
| `TRmgGenerator::loadTemplates` (0x537FF0), template acceptance | Count distinct usable player slots | Template admission |
| `TRmgZonePlacementFailure`; `TRmgGenerator::positionZone` (0x53B970) | Reject empty layout candidates before drawing | `layout.rs`, position selection |
| `TRmgGenerator::initializeZones` (0x53BCB0) | Own pending zones until successful placement | Local `PositionedZone`, appended after success |
| `TRmgGenerator::drawIrregularZoneBoundary` (0x53BFF0), `drawIslandBoundary` (0x53CD30), `connectJunctionEntrance` (0x5443A0) | Clamp subdivision range to one, retain draw | `raster.rs`, `boundary_midpoint` |
| `TRmgGenerator::buildZoneBoundaries` (0x53E050), `testSlot` probe | Boundary probe has no towns or unused town draw | `boundaries.rs`, radial-site setup |
| `TRmgGenerator::buildZoneBoundaries` (0x53E050), appended water zone | Water zones have cleared policies and neutral creatures | `boundaries.rs`, `CreaturePreference::water`; template-only placement rules |
| `TRmgGenerator::buildZoneConnectionPaths` (0x5405D0) | Choose fresh eligible path seeds | `placement/connection_paths.rs` |
| `TRmgGenerator::createGuard` (0x540B20) | Return no guard when no rank matches | `prototype/guard.rs`, `select_guard` |
| `TRmgGenerator::placeGuard` (Mac 0x243498) | Skip off-map or unassigned guard positions | `placement/guards.rs` |
| `TRmgGenerator::tryPlacePrimaryTown` (0x545250) | Resolve initially neutral town alignment | `BoundaryMap::record_primary_town` |
| `TRmgGenerator::createRoads` (0x548290) | Handle empty road targets after the initial draw | `placement/roads.rs`, `create_roads` |
| `TRmgGenerator::createRiverToJoin` (0x548500), `createRiverToOutlet` (0x548DF0) | Require reached river endpoints | Shared endpoint check in `placement/rivers.rs` |
| `TRmgGenerator::markRiverCoastTarget` (0x548A40) | Exclude right-edge coast aliases | `placement/rivers.rs`, `coast_cell` |
| `TRmgGenerator::hasPlayerTowns`, `TRmgObject::getEntrance`; their call in `TRmgGenerator::generate` (0x549930) | Require owned starting towns | `placement/towns.rs`, `has_player_towns` |
| `TRmgGenerator::generate` (0x549930), `initializeZones` call | Stop generation after layout failure | `layout.rs` and orchestration error propagation |
| `TRmgGenerator::writeMapHeader` (0x549CB0), description buffer | Cap template name and enlarge description buffer | `output/header.rs`, `description` |
| `TRmgGenerator::writeMapHeader` (0x549CB0), team masks | Disjoint human/computer team masks | `output/header.rs`, player admission |
| `TRmgGenerator::removeObject` (0x54BC50), active-object search | Do not erase an absent active object | `placement/registration.rs`, `unregister_object` |
| `TRmgGenerator::removeObject` (0x54BC50), cell-membership search | Do not erase an absent cell membership | `placement/mutation.rs`, `erase_footprint` |
| `TRandomMapRequest::isSupported`; its gate in `TRandomMapRequest::generateToFile` (0x54BF60) | Supported request and entry gate | `Request::parse`; generation requires the parsed request |
| `TRmgGenerator::hasRequiredPrototypes` and helpers; its gate in `TRandomMapRequest::generateToFile` (0x54BF60) | Required prototypes and entry gate | `prototype/readiness.rs`; orchestration Admission stage |
| `rmg.h`: `TRmgTreasureDef` virtual destructor | Treasure-definition destruction | Enum-owned definitions; no virtual destructor needed |
| `rmg.h`: `TRmgTemplateZone` and `TRmgTemplate` admission declarations | Zone/template admission helpers | `template.rs` |
| `rmg.h`: `TRmgGenerator::getSerializedMapVersion` | Unknown format fallback | Unknown formats cannot reach the writer after request admission |
| `rmg.h`: `TRmgObject::getEntrance`, `TRmgGenerator::hasRequiredPrototypes`/`hasPlayerTowns` declarations | Prototype and starting-town checks | Readiness and town checks above |
| `rmg_request.h`: `TRandomMapRequest::isSupported` declaration | Supported-request declaration | `Request::parse` |

The fallback for an unknown format in the native internal writer is unreachable
through the hotfix public entry point. Rust rejects unknown request versions
before generation and uses an exhaustive `MapVersion` match during output.

The source review alone did not resolve executable differences. The corrected
treasure geometry comparison now passes, and nonperturbing retail captures
establish zero-versus-nonzero water-zone town flags. Full rainbow-table parity remains
unproven; the [architecture reference](rust-rmg.md) defines verification requirements.
