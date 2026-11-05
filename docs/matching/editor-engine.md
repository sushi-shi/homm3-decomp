# Reviewed engine identities in the editors

Review date: 2026-09-24. Addresses refer to the [pinned GOG executables](editors.md#pinned-inputs), at image base `0x00400000`.

The review rechecks all **108 original map-editor source candidates** and extends them to **157 source-owned function identities**: **146 RMG functions and 11 other helpers**. The 49 extensions follow named references, class vtable slots, or complete matching bodies between verified order anchors. **131 of the 157 have identical non-relocation bytes in a source control or named game-retail body; 26 have identity evidence with differing bodies.** Five additional addresses from the earlier manual review are retained separately below, making **162 distinct map-editor addresses across these two reviews**. Compiler-generated destructor aliases are not counted as new source functions.

The campaign editor now has **two supported engine-source identities**, both artifact-mask builders. The earlier zero applied to the `/O1` source-fingerprint pass; comparing the named game-retail bodies finds them. This remains an incomplete reuse survey.

## Where the evidence lives

This document preserves the reviewed address tables and evidence in the repository's documentation. [Editor matching](editors.md) is the index for input provenance, library fingerprints, Victor, and the common-editor comparison. Existing source annotations and owning comments provide the names; provisional reconstructed names retain their existing uncertainty.

Full generated records, relocation sites, control-object hashes and experiments are local ignored artifacts:

| Location | Contents |
| --- | --- |
| `build/editor-engine/review.json` | This review's individual matches, source owners, named references, vtables, and executable/object hashes |
| `build/editor-engine/order-probes.json` | Nine uniquely bounded body matches and their two independent order anchors |
| `build/editor-engine/checks.json` | Reference accounting, mutation controls, campaign argument checks, and frozen-input checks |
| `build/editor-common/common-identities.json` | 1,309 editor-to-editor pairs, including 127 unnamed pairs, code ranges, switches and reference qualifications |
| `build/editor-common/engine-reuse.json` | Original 108-candidate source scan; retained as the earlier discovery result |
| `build/editor-order/` | Victor and the earlier shared-helper/vtable review |
| `build/editor-fid/` | Source, MFC, CRT and zlib fingerprint corpus |

The documentation is an evidence report, not a second build symbol ledger. Generated JSON is disposable and is not a tracked inventory. **All TSVs remain unchanged.** Future reviewed contiguous extents belong in the target-specific inventories under `config/retail/h3maped/` and `config/retail/h3ccmped/`; names remain source-owned. No editor scores or new carve lengths are admitted here.

## Review method and limits

Each original anchor was rechecked over its complete matched COFF span, including alignment or embedded data. Every byte outside explicit COFF relocation slots agrees. The 390 anchor relocations comprise **62 references to independently located original anchors**, **152 supported by the wider source/library corpus**, **31 exception-chain references to `fs:[0]`**, and **145 other external references recorded for further review**. Repeated named bindings are consistent. Masked-byte equality alone does not establish every external object's identity or contents.

Exact caller relocations locate additional source callees even when those callees compile differently. Constructor/destructor references anchor vtables; their emitted slot names locate virtual methods, preserving folded aliases. Nine more bodies have a unique credible entry between two independently mapped game-order anchors, with no competing source name in the checked interval. Their known external bindings are consistent with the rest of the map.

RMG's changing function lengths require individual address bindings. Game order corroborates the long runs, with explicit exceptions: `TRmgMapItem::setTerrain` is emitted in a different position; the terrain painter's `paintRectangle` and `getPackedCell` exchange order; gzip's allocation helper and destructor/read helper also move. Current reconstructed TU boundaries do not establish original object boundaries. No whole-TU constant address delta is assumed.

In the tables, **B** means all non-relocation bytes match an available complete control span; **I** means the identity is supported by a named call or anchored vtable but the available bodies differ. Neither is a reconstruction score. COFF spans, decoded instruction ranges and contiguous retail function lengths remain distinct. Reference offsets identify relocation fields inside the indicated body; vtable slots are numbered from zero. Names such as `unknownOperation` and the earlier `countPlacedZoneConnections` remain provisional source names.

## Map-editor function map

### artifact: 2 identities

Names and game ownership: [`src/artifact.cpp`](../../src/artifact.cpp).

| Source identity | Game VA | Map-editor VA | Evidence |
| --- | --- | --- | --- |
| `makeArtifactSlotMask` | `0x0044c720` | `0x00403245` | B; original anchor |
| `makeArtifactComponentMask` | `0x0044c830` | `0x0040329d` | B; original anchor |

### objecttype: 2 identities

Names and game ownership: [`src/objecttype.cpp`](../../src/objecttype.cpp).

| Source identity | Game VA | Map-editor VA | Evidence |
| --- | --- | --- | --- |
| `TNativeTerrainObjectFilter::accepts` | `0x005141b0` | `0x0049007e` | B; original anchor |
| `TAnyTerrainObjectFilter::accepts` | `0x00514220` | `0x004900ca` | B; original anchor |

### gzinflatebuf: 6 identities

Names and game ownership: [`src/gzinflatebuf.cpp`](../../src/gzinflatebuf.cpp).

| Source identity | Game VA | Map-editor VA | Evidence |
| --- | --- | --- | --- |
| `TAllocationFailure::TAllocationFailure` | `0x004d6b80` | `0x0040d513` | B; call/reference `0x00458755`+`0x27` |
| `TGzInflateBuf::getByte` | `0x004d5fd0` | `0x0044960f` | B; original anchor |
| `TGzInflateBuf::TDataError::TDataError` | `0x004d65e0` | `0x00449911` | B; call/reference `0x004499bf`+`0x14` |
| `TGzInflateBuf::readByte` | `0x004d6ba0` | `0x004499bf` | B; original anchor |
| `TGzInflateBuf::~TGzInflateBuf` | `0x004d6820` | `0x00449a1b` | B; original anchor |
| `TGzInflateBuf::underflow` | `0x004d6920` | `0x00449ab6` | I; vtable `0x00536d04`, slot 4 |

### dxplay: 1 identity

Names and game ownership: [`src/dxplay.cpp`](../../src/dxplay.cpp).

| Source identity | Game VA | Map-editor VA | Evidence |
| --- | --- | --- | --- |
| `TRuntimeError::TRuntimeError` | `0x0049a0c0` | `0x00418c07` | I; call/reference `0x004bcff5`+`0x58` |

### rmg: 111 identities

Names and game ownership: [`src/rmg.cpp`](../../src/rmg.cpp).

| Source identity | Game VA | Map-editor VA | Evidence |
| --- | --- | --- | --- |
| `TRmgMapItem::TRmgMapItem` | `0x00530e90` | `0x00499e65` | B; call/reference `0x00499f60`+`0x5d` |
| `TRmgMapItem::clear` | `0x00530f10` | `0x00499ea3` | B; original anchor |
| `type_random_map::type_random_map` | `0x00530fb0` | `0x00499f60` | B; original anchor |
| `type_random_map::~type_random_map` | `0x005310b0` | `0x0049a030` | B; original anchor |
| `type_random_map::clear` | `0x00531140` | `0x0049a072` | B; original anchor |
| `type_random_map::markCoastalTiles` | `0x00531310` | `0x0049a1ef` | B; original anchor |
| `type_random_map::addObject` | `0x00531ea0` | `0x0049abd6` | I; call/reference `0x0049e6cd`+`0x19` |
| `type_random_map::setTile` | `0x00532190` | `0x0049acc5` | I; vtable `0x00540a14`, slot 1 |
| `TRmgMapItem::setTerrain` | `0x00546940` | `0x0049acf6` | B; original anchor |
| `type_random_map::setOverlay` | `0x00532200` | `0x0049ad3e` | B; original anchor |
| `type_random_map::getSize` | `0x00532240` | `0x0049ad71` | B; vtable `0x00540a14`, slot 3 |
| `type_random_map::getTile` | `0x00532260` | `0x0049ad83` | B; original anchor |
| `type_random_map::getLand` | `0x005322c0` | `0x0049adde` | B; original anchor |
| `type_random_map::getOverlay` | `0x005322f0` | `0x0049ae01` | B; original anchor |
| `TRmgRoadMapAdapter::setOverlay` | `0x005323d0` | `0x0049aec5` | B; original anchor |
| `TRmgRoadMapAdapter::getTile` | `0x00532410` | `0x0049af1d` | B; original anchor |
| `TRmgRoadMapAdapter::getLand` | `0x00532480` | `0x0049af7b` | B; original anchor |
| `TRmgMapAdapter::setOverlay` | `0x00532730` | `0x0049b1bc` | B; original anchor |
| `TRmgMapAdapter::getTile` | `0x005327c0` | `0x0049b20c` | B; original anchor |
| `TRmgMapAdapter::getLand` | `0x00532830` | `0x0049b26a` | B; original anchor |
| `TRmgMapAdapter::getOverlay` | `0x00532860` | `0x0049b290` | B; between `0x0049b26a`, `0x0049b2b6` |
| `TRmgMapItem::write` | `0x00532890` | `0x0049b2b6` | B; original anchor |
| `TRmgZone::chooseTerrain` | `0x00532ab0` | `0x0049b53d` | B; original anchor |
| `TRmgZone::~TRmgZone` | `0x00532b50` | `0x0049b5cd` | B; original anchor |
| `TRmgTemplate::~TRmgTemplate` | `0x00532fe0` | `0x0049b965` | B; original anchor |
| `TRmgTemplate::findZone` | `0x005330a0` | `0x0049b9e6` | B; original anchor |
| `type_object::type_object` | `0x005330e0` | `0x0049ba89` | B; original anchor |
| `type_object::clearPlacementMarks` | `0x00533150` | `0x0049bae3` | B; call/reference `0x0049ba89`+`0x20` |
| `type_object::write` | `0x00533170` | `0x0049baf8` | B; original anchor |
| `rmgMonsterObject::write` | `0x005331f0` | `0x0049bb92` | B; original anchor |
| `rmgTownObject::write` | `0x005332f0` | `0x0049bc2f` | B; original anchor |
| `rmgOwnableObject::write` | `0x00533460` | `0x0049bd3e` | B; original anchor |
| `rmgArtifactObject::write` | `0x00533500` | `0x0049bd81` | B; original anchor |
| `rmgResourceObject::write` | `0x005335c0` | `0x0049bdb4` | B; original anchor |
| `rmgBlackBoxObject::write` | `0x005336f0` | `0x0049be93` | B; original anchor |
| `rmgKeyTentObject::isWritable` | `0x005338e0` | `0x0049c019` | I; vtable `0x00540b00`, slot 2 |
| `rmgQuestArtifactObject::~rmgQuestArtifactObject` | `0x005339f0` | `0x0049c065` | B; original anchor |
| `rmgQuestArtifactObject::isWritable` | `0x00533a50` | `0x0049c0a6` | B; original anchor |
| `rmgSeerHutObject::write` | `0x00533a90` | `0x0049c105` | B; original anchor |
| `rmgHeroObject::unknownOperation` | `0x00533c70` | `0x0049c264` | B; vtable `0x00540b3c`, slot 1 |
| `rmgHeroObject::write` | `0x00533c80` | `0x0049c273` | B; original anchor |
| `rmgScholarObject::write` | `0x00533e70` | `0x0049c3f4` | B; original anchor |
| `rmgShrineObject::write` | `0x00533f40` | `0x0049c44d` | B; original anchor |
| `rmgSpellScrollObject::write` | `0x00533ff0` | `0x0049c495` | B; original anchor |
| `rmgWitchHutObject::write` | `0x005340c0` | `0x0049c4f4` | B; original anchor |
| `type_treasure_def::type_treasure_def` | `0x00534160` | `0x0049c523` | B; original anchor |
| `type_treasure_def::getValue` | `0x00534190` | `0x0049c54d` | B; vtable `0x00540c50`, slot 1 |
| `type_treasure_def::generate` | `0x005341a0` | `0x0049c553` | B; original anchor |
| `type_artifact_def::generate` | `0x005341f0` | `0x0049c58a` | B; between `0x0049c553`, `0x0049c5cd` |
| `type_black_box_creature_def::type_black_box_creature_def` | `0x00534250` | `0x0049c5cd` | B; original anchor |
| `type_black_box_creature_def::getValue` | `0x00534310` | `0x0049c64b` | B; original anchor |
| `type_black_box_creature_def::generate` | `0x00534380` | `0x0049c69b` | B; original anchor |
| `type_black_box_experience_def::generate` | `0x00534410` | `0x0049c6e2` | B; original anchor |
| `type_black_box_gold_def::generate` | `0x00534490` | `0x0049c723` | B; original anchor |
| `type_black_box_spells_def::generate` | `0x00534520` | `0x0049c764` | B; original anchor |
| `type_dwelling_def::generate` | `0x00534790` | `0x0049c806` | B; between `0x0049c764`, `0x0049c849` |
| `type_map_dwelling_def::getValue` | `0x005347f0` | `0x0049c849` | B; original anchor |
| `type_resource_lump_def::generate` | `0x00534870` | `0x0049c8b0` | B; between `0x0049c849`, `0x0049c8f3` |
| `type_prison_def::generate` | `0x005348d0` | `0x0049c8f3` | B; original anchor |
| `type_scholar_def::generate` | `0x00534970` | `0x0049c97c` | B; between `0x0049c8f3`, `0x0049c9bf` |
| `type_shrine_def::type_shrine_def` | `0x005349d0` | `0x0049c9bf` | B; original anchor |
| `type_shrine_def::generate` | `0x00534a00` | `0x0049c9e3` | B; vtable `0x00540c40`, slot 0 |
| `type_witch_hut_def::type_witch_hut_def` | `0x00534a60` | `0x0049ca26` | B; original anchor |
| `type_witch_hut_def::generate` | `0x00534a90` | `0x0049ca48` | B; vtable `0x00540c50`, slot 0 |
| `type_quest_creature_def::getValue` | `0x00534af0` | `0x0049ca8b` | B; original anchor |
| `type_quest_creature_def::generate` | `0x00534b90` | `0x0049cac2` | B; original anchor |
| `type_quest_experience_def::getValue` | `0x00534c80` | `0x0049cb60` | B; between `0x0049cac2`, `0x0049cb83` |
| `type_quest_experience_def::generate` | `0x00534cc0` | `0x0049cb83` | B; original anchor |
| `type_quest_gold_def::generate` | `0x00534db0` | `0x0049cc22` | B; original anchor |
| `type_spell_scroll_def::type_spell_scroll_def` | `0x00534ea0` | `0x0049ccc1` | B; original anchor |
| `type_spell_scroll_def::generate` | `0x00534ed0` | `0x0049ccec` | I; vtable `0x00540c90`, slot 0 |
| `type_key_tent_def::getValue` | `0x00534fa0` | `0x0049cd97` | B; original anchor |
| `type_key_tent_def::generate` | `0x00534fd0` | `0x0049cdb1` | B; original anchor |
| `TRmgTreasureGroup::reset` | `0x00535040` | `0x0049ce64` | B; original anchor |
| `TRmgMapPosition::TRmgMapPosition` | `0x005355c0` | `0x0049d2c7` | B; original anchor |
| `TRmgTreasureGroup::updateBounds` | `0x00535df0` | `0x0049d6e0` | B; original anchor |
| `TRmgGeneratorBase::TRmgGeneratorBase` | `0x00536070` | `0x0049d914` | B; original anchor |
| `TRmgGeneratorBase::loadObjectPrototypes` | `0x00536200` | `0x0049da08` | I; call/reference `0x0049d914`+`0xc2` |
| `TRmgGeneratorBase::~TRmgGeneratorBase` | `0x005363b0` | `0x0049dba8` | B; original anchor |
| `TRmgGeneratorBase::addObject` | `0x005371c0` | `0x0049e6cd` | B; original anchor |
| `type_random_map::getMapItem` | `0x005378e0` | `0x0049eb6d` | B; original anchor |
| `type_random_map_generator::type_random_map_generator` | `0x00537b10` | `0x0049ecf2` | B; original anchor |
| `type_random_map_generator::~type_random_map_generator` | `0x00537df0` | `0x0049ef78` | B; original anchor |
| `type_random_map_generator::loadTemplates` | `0x00537ff0` | `0x0049f0cd` | I; call/reference `0x0049ecf2`+`0x145` |
| `type_random_map_generator::initializeObjectGenerators` | `0x00538b10` | `0x0049f95a` | I; call/reference `0x0049ecf2`+`0x1ae` |
| `type_random_map_generator::getInitialZoneBounds` | `0x0053b1f0` | `0x004a19ed` | B; original anchor |
| `type_random_map_generator::filterZonePositions` | `0x0053b2f0` | `0x004a1ad8` | B; original anchor |
| `type_random_map_generator::drawIrregularZoneBoundary` | `0x0053bff0` | `0x004a2413` | B; original anchor |
| `type_random_map_generator::drawIslandBoundary` | `0x0053cd30` | `0x004a2cdc` | B; original anchor |
| `type_random_map_generator::addObject` | `0x005402a0` | `0x004a54a7` | I; vtable `0x00540cbc`, slot 1 |
| `type_random_map_generator::placeBorderObject` | `0x00540d60` | `0x004a5e73` | B; original anchor |
| `type_random_map_generator::placeObjectInZone` | `0x00542930` | `0x004a7312` | I; call/reference `0x004a5e73`+`0xfd` |
| `type_random_map_generator::placePrimaryTown` | `0x00544a50` | `0x004a8d2c` | B; original anchor |
| `type_random_map_generator::tryPlacePrimaryTown` | `0x00545250` | `0x004a93a2` | I; call/reference `0x004a8d2c`+`0x2f` |
| `type_random_map_generator::tryPlaceMine` | `0x00545990` | `0x004a9911` | I; call/reference `0x004a9d6a`+`0x85` |
| `type_random_map_generator::placeExtraMines` | `0x00545e60` | `0x004a9c7c` | I; call/reference `0x004a9d6a`+`0xaf` |
| `type_random_map_generator::placeMines` | `0x00545f60` | `0x004a9d6a` | B; original anchor |
| `type_random_map_generator::selectObjectPrototype` | `0x00546040` | `0x004a9e40` | B; original anchor |
| `type_random_map_generator::buildRoadCostMap` | `0x00547880` | `0x004aae7b` | I; call/reference `0x004ab52a`+`0x6c` |
| `type_random_map_generator::paintRoad` | `0x00548040` | `0x004ab37f` | I; call/reference `0x004ab52a`+`0xf7` |
| `type_random_map_generator::createRoads` | `0x00548290` | `0x004ab52a` | B; original anchor |
| `type_random_map_generator::createRiverToObject` | `0x00548500` | `0x004ab6ac` | I; call/reference `0x004ac4ae`+`0x69` |
| `type_random_map_generator::markRiverTargets` | `0x00548c70` | `0x004abc20` | I; call/reference `0x004ac4ae`+`0x13` |
| `type_random_map_generator::createRiver` | `0x00548df0` | `0x004abd5f` | I; call/reference `0x004ac4ae`+`0x7f` |
| `type_random_map_generator::markRiverObjectTargets` | `0x005497a0` | `0x004ac3df` | B; original anchor |
| `type_random_map_generator::createRivers` | `0x00549870` | `0x004ac4ae` | B; original anchor |
| `type_random_map_generator::generate` | `0x00549930` | `0x004ac552` | I; call/reference `0x004adfe1`+`0x9d` |
| `type_random_map_generator::writeMap` | `0x0054abf0` | `0x004ad1e3` | I; call/reference `0x004adfe1`+`0xb5` |
| `type_random_map_generator::selectPrisonHero` | `0x0054b100` | `0x004ad640` | B; original anchor |
| `type_random_map_generator::placeQuestArtifact` | `0x0054b490` | `0x004ad947` | I; call/reference `0x0049c0a6`+`0x8` |
| `TRandomMapRequest::generateToFile` | `0x0054bf60` | `0x004adfe1` | B; original anchor |

### rmg_support: 9 identities

Names and game ownership: [`src/rmg_support.cpp`](../../src/rmg_support.cpp).

| Source identity | Game VA | Map-editor VA | Evidence |
| --- | --- | --- | --- |
| `TRmgLinePatternTable::TRmgLinePatternTable` | `0x004f9be0` | `0x00458755` | B; original anchor |
| `TRmgVector::length` | `0x005fceb0` | `0x004cc5ad` | I; call/reference `0x004a2413`+`0xd0` |
| `TRmgBoundaryVertex::TRmgBoundaryVertex` | `0x005fcef0` | `0x004cc5db` | B; original anchor |
| `TRmgBoundaryVertex::splice` | `0x005fcf60` | `0x004cc643` | B; original anchor |
| `TRmgBoundaryVertex::detach` | `0x005fcfa0` | `0x004cc670` | B; call/reference `0x004cc9cc`+`0xc` |
| `TRmgVoronoi::~TRmgVoronoi` | `0x005fd330` | `0x004cc91e` | B; original anchor |
| `TRmgVoronoi::createEdge` | `0x005fd390` | `0x004cc955` | B; original anchor |
| `TRmgVoronoi::removeEdge` | `0x005fd5b0` | `0x004cc9cc` | B; original anchor |
| `TRmgVoronoi::locate` | `0x005fd6b0` | `0x004cca55` | B; original anchor |

### rmg_terrain: 26 identities

Names and game ownership: [`src/rmg_terrain.cpp`](../../src/rmg_terrain.cpp).

| Source identity | Game VA | Map-editor VA | Evidence |
| --- | --- | --- | --- |
| `TRmgLineWalker::TRmgLineWalker` | `0x004fa280` | `0x00458d37` | B; original anchor |
| `TRmgLineWalker::drawTo` | `0x004fa2b0` | `0x00458d66` | B; original anchor |
| `TRmgLineWalker::paintPoint` | `0x004fa3c0` | `0x00458e61` | I; call/reference `0x00458d66`+`0x88` |
| `TRmgPatternTerrainRule::TRmgPatternTerrainRule` | `0x005b3780` | `0x004ba868` | B; original anchor |
| `TRmgPatternTerrainRule::hasEntries` | `0x005b3840` | `0x004ba8f7` | B; vtable `0x00543780`, slot 1 |
| `TRmgPatternTerrainRule::isSpecialFrame` | `0x005b3860` | `0x004ba91d` | B; vtable `0x00543780`, slot 2 |
| `TRmgPatternTerrainRule::getEntry` | `0x005b3880` | `0x004ba92b` | B; vtable `0x00543780`, slot 3 |
| `TRmgPatternTerrainRule::selectBaseFrame` | `0x005b3890` | `0x004ba938` | B; original anchor |
| `TRmgPatternTerrainRule::selectTransitionFrame` | `0x005b38f0` | `0x004ba989` | I; vtable `0x00543780`, slot 5 |
| `TRmgTableTerrainRule::TRmgTableTerrainRule` | `0x005b3a20` | `0x004baa66` | B; between `0x004ba989`, `0x004baa94` |
| `TRmgTableTerrainRule::hasEntries` | `0x005b3a40` | `0x004baa77` | B; vtable `0x0054379c`, slot 1 |
| `TRmgTableTerrainRule::getEntry` | `0x005b3a80` | `0x004baa86` | B; between `0x004ba989`, `0x004baa94` |
| `TRmgTableTerrainRule::selectBaseFrame` | `0x005b3aa0` | `0x004baa94` | B; original anchor |
| `TRmgTableTerrainRule::selectTransitionFrame` | `0x005b3ae0` | `0x004baabf` | B; original anchor |
| `rmgTerrainPainter::initializePackedCell` | `0x005b3dd0` | `0x004bad97` | B; original anchor |
| `rmgTerrainPainter::rmgTerrainPainter` | `0x005b45f0` | `0x004bb5ce` | B; original anchor |
| `rmgTerrainPainter::paintRectangle` | `0x005b4960` | `0x004bb681` | I; call/reference `0x004bd099`+`0x14` |
| `rmgTerrainPainter::getPackedCell` | `0x005b48d0` | `0x004bb71b` | B; original anchor |
| `rmgTerrainPainter::queueOtherTerrainNeighbours` | `0x005b50f0` | `0x004bba59` | B; original anchor |
| `rmgTerrainPainter::isHorizontalGap` | `0x005b6320` | `0x004bc674` | B; original anchor |
| `rmgTerrainPainter::isVerticalGap` | `0x005b6430` | `0x004bc6e0` | B; original anchor |
| `rmgTerrainPainter::buildMatchingNeighbourMask` | `0x005b6540` | `0x004bc74c` | B; original anchor |
| `TRmgTerrainBrush::TRmgTerrainBrush` | `0x005b7250` | `0x004bcff5` | B; original anchor |
| `TRmgTerrainBrush::changeTerrain` | `0x005b7520` | `0x004bd07c` | B; original anchor |
| `TRmgTerrainBrush::paintRectangle` | `0x005b7690` | `0x004bd099` | B; between `0x004bd07c`, `0x004bd0d1` |
| `rmgTerrainPainter::~rmgTerrainPainter` | `0x005b76f0` | `0x004bd0d1` | B; original anchor |

## Earlier reviewed addresses outside that table

These five were established by the earlier manual call/vtable review. The other ten addresses in that review already occur above and are not added again. The two deleting-destructor rows are compiler-generated roles.

| Source identity or role | Map-editor VA | Evidence |
| --- | --- | --- |
| `TGzInflateBuf::TGzInflateBuf` | `0x00449663` | Immediately after exact getByte; installs anchored vtable; gzip magic and matched getByte/readByte/TDataError/crc32/inflateInit2_ calls |
| `type_random_map_generator::countPlacedZoneConnections` | `0x004a1967` | Both calls from exact filterZonePositions; directly before exact getInitialZoneBounds; reviewed connection-vector count loop and canConnect call |
| `TGzInflateBuf deleting destructor` | `0x004499ff` | Vtable anchored by known constructor or matched slot; method order and field/rand semantics reviewed |
| `TRmgPatternTerrainRule / TRmgTableTerrainRule deleting destructor (folded)` | `0x004ba901` | Vtable anchored by known constructor or matched slot; method order and field/rand semantics reviewed |
| `TRmgTableTerrainRule::isSpecialFrame` | `0x004baa81` | Vtable anchored by known constructor or matched slot; method order and field/rand semantics reviewed |

The four-byte little-endian trailer reader at `0x00449c1e` remains an unnamed role: it calls `readByte` four times, combines shifts 8/16/24, and is called twice by `underflow` after `inflateEnd`. It is not included in the named-function totals.

## Campaign-editor artifact helpers

| Source identity | Game VA | Map-editor VA | Campaign-editor VA | Campaign evidence |
| --- | --- | --- | --- | --- |
| `makeArtifactSlotMask` | `0x0044c720` | `0x00403245` | `0x00401ee0` | All non-relocation bytes of the 267-byte game body; 14 calls with the same slot arguments |
| `makeArtifactComponentMask` | `0x0044c830` | `0x0040329d` | `0x00402360` | All non-relocation bytes of the 290-byte game body; 12 calls with the same recipe arguments |

The direct decoder independently agrees on all instructions, branch structure and referenced string contents for both game-to-campaign pairs. Both campaign entries have decoded caller evidence. Their larger retail bodies were missed by the `/O1` source-only comparison.

The caller at `0x00402770` builds the artifact slot-mask table, including the same fourteen nonempty masks; `0x00402490` builds the twelve combination-artifact recipes. All 26 ordered immediate argument lists agree with their respective game initializers at `0x0044cc00` and `0x0044c960`, including the component IDs and counts. The initializer bodies themselves differ and are not counted as extra byte matches or ordinary source functions. This establishes concrete artifact-table support reused by the campaign editor; it does not measure total engine reuse.

## Validation and local reproduction

The review uses existing pinned VC6 control objects and named delinked game bodies. Each game-transfer template is checked against the game's actual retail bytes before searching either editor. No Ghidra boundaries or sizes are used.

In-memory mutation controls reject a changed fixed instruction byte, a redirected named callee despite an unchanged masked body, and an anchored vtable slot that contradicts an independently mapped function. All 108 original anchors reproduce, all nine order-bracket additions retain their evidence, and both campaign argument-sequence checks pass. The 21 used object hashes, pinned images, source/header/profile baseline and all twenty tracked TSVs were checked. Original executable files are untouched.

With the earlier scratch corpora still present:

```sh
export HOMM3_DIR="$PWD"
export PYTHONPATH="$PWD/scripts"
python3 build/editor-engine/review.py
python3 build/editor-engine/order-probes.py
python3 build/editor-engine/checks.py --with-order
python3 build/editor-engine/report.py
```

These scripts are one-off review artifacts under ignored `build/`. The address tables and conclusions above survive their removal. Source, headers, compiler profiles, inventories and scores were not edited; no game build was needed for this evidence/documentation-only review.
