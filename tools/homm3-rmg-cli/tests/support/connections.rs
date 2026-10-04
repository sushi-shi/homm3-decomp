//! Native ground-crossing attempts over the shared preconnection map.
use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    boundaries::BoundaryWorkspace,
    domain::Terrain,
    layout::LayoutWorkspace,
    placement::{ConnectingZones, ConnectionError, ObjectArena, ObjectPayload, PlacementWorkspace},
    placement_rules::PlacementRules,
    prototype::PrototypeSource,
    request::{default_record, Levels, MapSize, Request},
    rng::RetailRng,
    selection::{resolve_water, SelectedTemplate},
    template::TemplateSource,
    terrain::TerrainWorkspace,
    traits::CreatureCatalog,
};
use homm3_rmg_cli::resources::Installation;
use std::{
    fmt::Write,
    path::{Path, PathBuf},
};
#[path = "placement.rs"]
mod placement_snapshot;
#[path = "treasure_factories.rs"]
mod treasure_factories;
#[path = "treasure_groups.rs"]
mod treasure_groups;
use placement_snapshot::{write_cells, write_counts};

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Attempts {
    Ground,
    GroundAndShipyard,
    Direct,
    BothPasses,
    Junctions,
    Mines,
    TreasurePaths,
    TreasureValues,
    TreasureFactories,
    TreasureGroups,
}
impl Attempts {
    fn includes_junctions(self) -> bool {
        matches!(
            self,
            Self::Junctions
                | Self::Mines
                | Self::TreasurePaths
                | Self::TreasureValues
                | Self::TreasureFactories
                | Self::TreasureGroups
        )
    }

    fn filename(self) -> &'static str {
        match self {
            Self::Ground => "ground-connections.txt",
            Self::GroundAndShipyard => "shipyard-connections.txt",
            Self::Direct => "direct-connections.txt",
            Self::BothPasses => "zone-connections.txt",
            Self::Junctions => "junctions.txt",
            Self::Mines => "mines.txt",
            Self::TreasurePaths => "treasure-paths.txt",
            Self::TreasureValues => "treasure-values.txt",
            Self::TreasureFactories => "treasure-factories.txt",
            Self::TreasureGroups => "group-geometry.txt",
        }
    }
}
const CASES: [(u32, MapSize, Levels, u32, u32); 4] = [
    (1, MapSize::Small, Levels::Surface, 2, 0),
    (42, MapSize::Medium, Levels::Underground, 1, 1),
    (100, MapSize::Large, Levels::Surface, 0, 2),
    (17, MapSize::ExtraLarge, Levels::Underground, 2, 3),
];

pub fn compare_native(attempts: Attempts) {
    let directory = PathBuf::from(std::env::var_os("HOMM3_RMG_DATA").unwrap());
    let oracle = PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    let mut installation = Installation::open(&directory).unwrap();
    let objects = read_prototypes(&mut installation);
    let source = PrototypeSource::parse(&objects, |name| installation.mask(name)).unwrap();
    let mut bytes = Vec::new();
    installation.text("rand_trn.txt", &mut bytes).unwrap();
    let creatures = read_creatures(&mut installation);
    let templates = read_templates(&mut installation);
    let templates = TemplateSource::parse(&templates).unwrap();
    let mut layout = LayoutWorkspace::default();
    let mut boundaries = BoundaryWorkspace::default();
    let mut terrain = TerrainWorkspace::default();
    let mut placement = PlacementWorkspace::default();
    let mut checked = 0;
    let mut junction_count = 0;
    let mut previous_id = None;
    for (mode, behavior) in [
        (
            "retail",
            Behavior::Retail(RetailProfile {
                initial_key_tent_color: Some(0),
                ..RetailProfile::default()
            }),
        ),
        ("hotfix", Behavior::Hotfix),
    ] {
        let rules = PlacementRules::parse(&bytes, behavior).unwrap();
        for (case, (seed, size, levels, version, water)) in CASES.into_iter().enumerate() {
            let mut record = default_record(size, levels);
            record.m_mapVersion = version;
            record.m_waterContent = water;
            let request = Request::parse(record, behavior).unwrap();
            let mut rng = RetailRng::new(seed);
            let water = resolve_water(request.water(), &mut rng);
            let candidates = templates.prepare(&request, water).unwrap();
            let selected = SelectedTemplate::select(&candidates, &request, &mut rng).unwrap();
            let zones = layout
                .generate(&selected, &request, water, &mut rng)
                .unwrap();
            let boundary_map = boundaries.generate(zones, &mut rng).unwrap();
            let coverage = boundary_map.prepare_terrain(&mut rng).unwrap();
            let painted = terrain.paint(coverage, &mut rng).unwrap();
            let tile_address = painted.tiles().as_ptr();
            let mut map = placement.begin(painted).unwrap();
            assert_eq!(map.terrain().tiles().as_ptr(), tile_address);
            let catalog = source.prepare(&rules, request.version(), behavior).unwrap();
            let colors = tent_colors(&mut map, &catalog);
            let mut objects = ObjectArena::default();
            let towns = map.place_towns(&mut objects, &catalog, &mut rng).unwrap();
            let borders = towns.reserve_connection_borders(&mut rng).unwrap();
            let islands = borders.place_water_islands(&mut rng).unwrap();
            assert_eq!(islands.map().terrain().tiles().as_ptr(), tile_address);
            let paths = islands
                .build_connection_paths(&mut objects, &catalog, &mut rng)
                .unwrap();
            let repaired = paths.repair_water_borders(&mut rng).unwrap();
            let mut connecting = repaired.begin_connections(&objects, &catalog).unwrap();
            assert_eq!(connecting.map().terrain().tiles().as_ptr(), tile_address);
            if let Some(id) = previous_id {
                let checkpoint = rng.checkpoint();
                assert!(matches!(
                    connecting.try_ground_connection(
                        id,
                        &mut objects,
                        &catalog,
                        &creatures,
                        &mut rng
                    ),
                    Err(ConnectionError::UnknownConnection(_))
                ));
                assert_eq!(rng.checkpoint(), checkpoint);
            }
            previous_id = connecting.connection_ids().next();
            junction_count += dry_junction_count(connecting.map());
            let actual = snapshot(
                connecting,
                &mut objects,
                &catalog,
                &creatures,
                &mut rng,
                &colors,
                attempts,
            );
            checked += compare(&oracle, mode, case, &actual, attempts.filename());
        }
    }
    eprintln!("checked {checked} native connection checkpoint lines");
    if attempts.includes_junctions() {
        eprintln!("prepared {junction_count} dry junction zones");
        assert!(
            junction_count > 0,
            "fixture did not exercise dry junction zones"
        );
    }
}

fn read_creatures(installation: &mut Installation) -> CreatureCatalog {
    let mut bytes = Vec::new();
    installation.text("crtraits.txt", &mut bytes).unwrap();
    CreatureCatalog::parse(&bytes).unwrap()
}

fn dry_junction_count(map: &homm3_rmg::placement::PlacementMap<'_, '_, '_>) -> usize {
    let map = map.coverage().map();
    map.zones()
        .iter()
        .filter(|zone| {
            let homm3_rmg::boundaries::ZoneOrigin::Template(id) = zone.origin() else {
                return false;
            };
            zone.terrain() != Terrain::Water
                && matches!(
                    map.template().zones()[id.index()].role(),
                    homm3_rmg::template::ZoneRole::Junction(_)
                )
        })
        .count()
}

// Retype two-way rows as unused boat art in both implementations. Keep source
// row indices and every other field, so routing must select one-way pairs.
fn read_prototypes(installation: &mut Installation) -> Vec<u8> {
    let mut bytes = Vec::new();
    installation.text("objects.txt", &mut bytes).unwrap();
    if std::env::var_os("HOMM3_RMG_FORCE_ONE_WAY").is_none() {
        return bytes;
    }
    let mut result = Vec::new();
    let two_way = homm3_rmg::raw::LITH_TWOWAY.to_string();
    let boat = homm3_rmg::raw::BOAT.to_string();
    for (row, line) in bytes.split(|&b| b == b'\n').enumerate() {
        for (column, field) in line
            .split(u8::is_ascii_whitespace)
            .filter(|field| !field.is_empty())
            .enumerate()
        {
            if column != 0 {
                result.push(b' ');
            }
            if row != 0 && column == 5 && field == two_way.as_bytes() {
                result.extend_from_slice(boat.as_bytes());
            } else {
                result.extend_from_slice(field);
            }
        }
        result.extend_from_slice(b"\r\n");
    }
    result
}
fn read_templates(installation: &mut Installation) -> Vec<u8> {
    let mut templates = Vec::new();
    installation.text("rmg.txt", &mut templates).unwrap();
    if std::env::var_os("HOMM3_RMG_FORCE_BORDER_GUARDS").is_some() {
        templates = with_border_guards(&templates);
    }
    templates
}
fn write_tents(
    actual: &mut String,
    map: &homm3_rmg::placement::PlacementMap<'_, '_, '_>,
    colors: &[homm3_rmg::placement::KeyTentColor],
) {
    actual.push_str("tents");
    for &color in colors {
        write!(
            actual,
            " {}",
            u8::from(map.key_tent_disabled(color).unwrap())
        )
        .unwrap();
    }
    actual.push('\n');
}
fn tent_colors(
    map: &mut homm3_rmg::placement::PlacementMap<'_, '_, '_>,
    catalog: &homm3_rmg::prototype::PrototypeCatalog<'_>,
) -> Vec<homm3_rmg::placement::KeyTentColor> {
    let family =
        homm3_rmg::object::ObjectKind::parse(i32::try_from(homm3_rmg::raw::BORDER_TENT).unwrap())
            .unwrap();
    (0..catalog.family(family).len())
        .map(|index| {
            map.key_tent_color(catalog, i32::try_from(index).unwrap())
                .unwrap()
        })
        .collect()
}
fn run_attempts(
    actual: &mut String,
    connecting: &mut ConnectingZones<'_, '_, '_>,
    objects: &mut ObjectArena,
    catalog: &homm3_rmg::prototype::PrototypeCatalog<'_>,
    creatures: &CreatureCatalog,
    rng: &mut RetailRng,
    attempts: Attempts,
) {
    for zone_index in 0..connecting.map().coverage().map().zones().len() {
        let zone = connecting.map().coverage().map().zones()[zone_index];
        if zone.terrain() == Terrain::Water {
            continue;
        }
        if attempts == Attempts::GroundAndShipyard {
            connecting.clear_connection_visits(zone.id()).unwrap();
        }
        let mut link_index = 0;
        for id in connecting.connection_ids() {
            let edge = connecting.connection(id).unwrap();
            if edge.source != zone.id() {
                continue;
            }
            let current = link_index;
            link_index += 1;
            if edge.connected {
                continue;
            }
            let mut result = connecting
                .try_ground_connection(id, objects, catalog, creatures, rng)
                .unwrap();
            if !result && attempts == Attempts::GroundAndShipyard {
                result = connecting
                    .try_shipyard_connection(id, objects, catalog, creatures, rng)
                    .unwrap();
            }
            writeln!(
                actual,
                "{zone_index} {current} {} {} {} {}",
                u8::from(result),
                rng.state(),
                connecting.map().next_object_id(),
                connecting.map().active_objects().len()
            )
            .unwrap();
        }
    }
}
fn snapshot(
    mut connecting: ConnectingZones<'_, '_, '_>,
    objects: &mut ObjectArena,
    catalog: &homm3_rmg::prototype::PrototypeCatalog<'_>,
    creatures: &CreatureCatalog,
    rng: &mut RetailRng,
    colors: &[homm3_rmg::placement::KeyTentColor],
    attempts: Attempts,
) -> String {
    let mut actual = String::new();
    for direction in [
        homm3_rmg::placement::PortalDirection::OneWay,
        homm3_rmg::placement::PortalDirection::TwoWay,
    ] {
        assert!(connecting.map().portals(direction).is_empty());
    }
    for zone in connecting.map().coverage().map().zones() {
        assert!(connecting
            .map()
            .zone_entrances(zone.id())
            .unwrap()
            .is_empty());
    }
    if matches!(attempts, Attempts::Ground | Attempts::GroundAndShipyard) {
        run_attempts(
            &mut actual,
            &mut connecting,
            objects,
            catalog,
            creatures,
            rng,
            attempts,
        );
        if attempts == Attempts::GroundAndShipyard {
            write_water_state(&mut actual, connecting.map());
        }
        write_snapshot(
            &mut actual,
            connecting.map(),
            objects,
            catalog,
            colors,
            false,
        );
        return actual;
    }
    let direct = connecting
        .connect_direct_zones(objects, catalog, creatures, rng)
        .unwrap();
    assert_eq!(direct.rng(), rng.checkpoint());
    if attempts == Attempts::Direct {
        return completed_snapshot(direct.map(), objects, catalog, rng, colors, attempts);
    }
    let placed = direct
        .connect_remaining_zones(objects, catalog, creatures, rng)
        .unwrap();
    assert_eq!(placed.rng(), rng.checkpoint());
    if attempts == Attempts::BothPasses {
        return completed_snapshot(placed.map(), objects, catalog, rng, colors, attempts);
    }
    let junctions = placed.prepare_junctions(rng).unwrap();
    assert_eq!(junctions.rng(), rng.checkpoint());
    if attempts == Attempts::Junctions {
        return completed_snapshot(junctions.map(), objects, catalog, rng, colors, attempts);
    }
    let mines = junctions
        .place_mines(objects, catalog, creatures, rng)
        .unwrap();
    assert_eq!(mines.rng(), rng.checkpoint());
    if attempts == Attempts::Mines {
        return completed_snapshot(mines.map(), objects, catalog, rng, colors, attempts);
    }
    treasure_paths_snapshot(mines, objects, catalog, rng, colors, creatures, attempts)
}

fn treasure_paths_snapshot(
    mines: homm3_rmg::placement::MinesPlaced<'_, '_, '_>,
    objects: &mut ObjectArena,
    catalog: &homm3_rmg::prototype::PrototypeCatalog<'_>,
    rng: &mut RetailRng,
    colors: &[homm3_rmg::placement::KeyTentColor],
    creatures: &CreatureCatalog,
    attempts: Attempts,
) -> String {
    let paths = mines.prepare_treasure_paths(objects, catalog, rng).unwrap();
    assert_eq!(paths.rng(), rng.checkpoint());
    let mut actual = format!("town-zones {}", paths.town_zones().total());
    for count in paths.town_zones().by_alignment() {
        write!(actual, " {count}").unwrap();
    }
    actual.push('\n');
    if matches!(
        attempts,
        Attempts::TreasureValues | Attempts::TreasureFactories | Attempts::TreasureGroups
    ) {
        let mut workspace = homm3_rmg::treasure::TreasureWorkspace::default();
        let definitions = workspace.prepare(catalog, creatures).unwrap();
        let ready = paths.begin_treasures(definitions).unwrap();
        if matches!(
            attempts,
            Attempts::TreasureFactories | Attempts::TreasureGroups
        ) {
            let unchanged =
                completed_snapshot(ready.map(), objects, catalog, rng, colors, attempts);
            actual.push_str(&if attempts == Attempts::TreasureGroups {
                treasure_groups::snapshot(ready, objects, rng, colors)
            } else {
                treasure_factories::snapshot(ready, objects, rng)
            });
            actual.push_str(&unchanged);
            return actual;
        }

        writeln!(
            actual,
            "values {} {}",
            ready.map().coverage().map().zones().len(),
            ready.catalog().definitions().len()
        )
        .unwrap();
        for zone in ready.map().coverage().map().zones() {
            for id in ready.catalog().ids() {
                writeln!(actual, "{}", ready.value(id, zone.id()).unwrap()).unwrap();
            }
        }
        actual.push_str(&completed_snapshot(
            ready.map(),
            objects,
            catalog,
            rng,
            colors,
            Attempts::TreasureValues,
        ));
        return actual;
    }
    actual.push_str(&completed_snapshot(
        paths.map(),
        objects,
        catalog,
        rng,
        colors,
        Attempts::TreasurePaths,
    ));
    actual
}

fn completed_snapshot(
    map: &homm3_rmg::placement::PlacementMap<'_, '_, '_>,
    objects: &ObjectArena,
    catalog: &homm3_rmg::prototype::PrototypeCatalog<'_>,
    rng: &RetailRng,
    colors: &[homm3_rmg::placement::KeyTentColor],
    attempts: Attempts,
) -> String {
    let mut actual = format!(
        "rng {} {} {}\n",
        rng.state(),
        map.next_object_id(),
        map.active_objects().len()
    );
    write_water_state(&mut actual, map);
    if matches!(
        attempts,
        Attempts::BothPasses
            | Attempts::Junctions
            | Attempts::Mines
            | Attempts::TreasurePaths
            | Attempts::TreasureValues
            | Attempts::TreasureFactories
            | Attempts::TreasureGroups
    ) {
        use homm3_rmg::placement::PortalDirection;
        for (index, direction) in [PortalDirection::OneWay, PortalDirection::TwoWay]
            .into_iter()
            .enumerate()
        {
            write!(actual, "portals {index}").unwrap();
            for &portal in map.portals(direction) {
                write!(
                    actual,
                    " {}",
                    map.active_objects()
                        .iter()
                        .position(|&id| id == portal)
                        .unwrap()
                )
                .unwrap();
            }
            actual.push('\n');
        }
    }
    write_snapshot(
        &mut actual,
        map,
        objects,
        catalog,
        colors,
        matches!(
            attempts,
            Attempts::BothPasses
                | Attempts::Junctions
                | Attempts::Mines
                | Attempts::TreasurePaths
                | Attempts::TreasureValues
                | Attempts::TreasureFactories
                | Attempts::TreasureGroups
        ),
    );
    actual
}
fn write_snapshot(
    actual: &mut String,
    map: &homm3_rmg::placement::PlacementMap<'_, '_, '_>,
    objects: &ObjectArena,
    catalog: &homm3_rmg::prototype::PrototypeCatalog<'_>,
    colors: &[homm3_rmg::placement::KeyTentColor],
    include_zone_distance: bool,
) {
    write_tents(actual, map, colors);
    write_objects(actual, map, objects, catalog);
    for zone in map.coverage().map().zones() {
        let entrances = map.zone_entrances(zone.id()).unwrap();
        write!(actual, "{}", entrances.len()).unwrap();
        for point in entrances {
            write!(actual, " {} {}", point.x, point.y).unwrap();
        }
        actual.push('\n');
        for edge in map
            .coverage()
            .map()
            .connections()
            .iter()
            .filter(|edge| edge.source == zone.id())
        {
            write!(actual, "{} ", u8::from(edge.connected)).unwrap();
        }
        actual.push('\n');
    }
    write_counts(actual, map);
    write_cells(actual, map, |id| {
        i32::try_from(map.active_objects().iter().position(|&x| x == id).unwrap()).unwrap()
    });
    let side = map.coverage().map().raster().dimension();
    for index in 0..map.terrain().tiles().len() {
        let at = homm3_rmg::domain::WorldPosition {
            point: homm3_rmg::geometry::Point::new(
                i32::try_from(index % side).unwrap(),
                i32::try_from(index / side % side).unwrap(),
            ),
            level: if index < side * side {
                homm3_rmg::domain::Level::Surface
            } else {
                homm3_rmg::domain::Level::Underground
            },
        };
        let cell = map.cell(at).unwrap();
        let previous = cell.movement().previous().map_or([-1; 3], |p| {
            [
                p.point.x,
                p.point.y,
                i32::try_from(p.level.index()).unwrap(),
            ]
        });
        write!(
            actual,
            "{} {} {} {} {}",
            cell.movement().cost(),
            previous[0],
            previous[1],
            previous[2],
            cell.border().map_or(-1, |c| i32::from(c.value()))
        )
        .unwrap();
        if include_zone_distance {
            let distance = cell.zone_distance();
            write!(
                actual,
                " {} {} {}",
                distance.cost(),
                distance
                    .connection()
                    .map_or(-1, |(zone, _)| i32::try_from(zone.index()).unwrap()),
                distance.direction().index()
            )
            .unwrap();
        }
        actual.push('\n');
    }
}

fn write_objects(
    actual: &mut String,
    map: &homm3_rmg::placement::PlacementMap<'_, '_, '_>,
    objects: &ObjectArena,
    catalog: &homm3_rmg::prototype::PrototypeCatalog<'_>,
) {
    writeln!(actual, "objects {}", map.active_objects().len()).unwrap();
    for &id in map.active_objects() {
        let geometry = objects.get(id).unwrap();
        let position = geometry.position().unwrap();
        let payload = match objects.payload(id).unwrap() {
            ObjectPayload::Ownable => [3, -1, -1, -1],
            ObjectPayload::Resource => [4, -1, 0, -1],
            ObjectPayload::Base => [0, -1, -1, -1],
            ObjectPayload::Town(town) => [
                1,
                town.id().value(),
                town.owner()
                    .map_or(-1, |p| i32::try_from(p.index()).unwrap()),
                i32::from(town.fort() == homm3_rmg::placement::Fort::Present),
            ],
            ObjectPayload::Monster(monster) => [
                2,
                monster.id().value(),
                monster.count(),
                i32::try_from(monster.disposition()).unwrap(),
            ],
            other => panic!("unexpected pre-treasure payload: {other:?}"),
        };
        writeln!(
            actual,
            "{} {} {} {} {} {} {} {}",
            catalog
                .get(geometry.prototype())
                .unwrap()
                .prototype()
                .source_row(),
            position.point.x,
            position.point.y,
            position.level.index(),
            payload[0],
            payload[1],
            payload[2],
            payload[3]
        )
        .unwrap();
    }
}
fn compare(oracle: &Path, mode: &str, case: usize, actual: &str, filename: &str) -> usize {
    let expected = std::fs::read_to_string(
        oracle.join(format!("{mode}-layout/case-{case}-candidate/{filename}")),
    )
    .unwrap();
    // Compare content before lengths so an omitted marker does not hide the
    // first useful difference. Keep Rust output beside the capture on failure.
    let mut actual_lines = actual.lines();
    let mut expected_lines = expected.lines();
    let mut index = 0;
    loop {
        let a = actual_lines.next();
        let b = expected_lines.next();
        match (a, b) {
            (None, None) => break,
            (Some(a), Some(b)) if a.split_whitespace().eq(b.split_whitespace()) => {}
            _ => {
                let artifact = oracle.join(format!(
                    "{mode}-layout/case-{case}-candidate/{filename}.rust-actual"
                ));
                if let Err(error) = std::fs::write(&artifact, actual) {
                    eprintln!("could not save {}: {error}", artifact.display());
                }
                panic!("{mode} case{case} line{index}:\nactual: {a:?}\nexpected: {b:?}\nRust snapshot: {}", artifact.display());
            }
        }
        index += 1;
    }
    eprintln!("{mode} case{case}: {} lines", actual.lines().count());
    actual.lines().count()
}

// The matching native capture sets every parsed template connection's flag.
// Keep all other bytes, rows, columns and defaults in their source order.
fn with_border_guards(bytes: &[u8]) -> Vec<u8> {
    let mut result = Vec::new();
    for line in bytes.split_inclusive(|&b| b == b'\n') {
        let line = line
            .strip_suffix(b"\r\n")
            .or_else(|| line.strip_suffix(b"\n"))
            .unwrap_or(line);
        for (index, field) in line.split(|&b| b == b'\t').enumerate() {
            if index > 0 {
                result.push(b'\t');
            }
            if index == homm3_rmg::raw::RMG_TEMPLATE_COLUMN_CONNECTION_BORDER_GUARD as usize {
                result.push(b'x');
            } else {
                result.extend_from_slice(field);
            }
        }
        result.extend_from_slice(b"\r\n");
    }
    result
}

fn write_water_state(actual: &mut String, map: &homm3_rmg::placement::PlacementMap<'_, '_, '_>) {
    write!(actual, "roads").unwrap();
    for position in map.road_targets() {
        write!(
            actual,
            " {} {} {}",
            position.point.x,
            position.point.y,
            position.level.index()
        )
        .unwrap();
    }
    actual.push('\n');
    write!(actual, "visits").unwrap();
    let side = map.coverage().map().raster().dimension();
    for index in 0..map.terrain().tiles().len() {
        let at = homm3_rmg::domain::WorldPosition {
            point: homm3_rmg::geometry::Point::new(
                (index % side).try_into().unwrap(),
                (index / side % side).try_into().unwrap(),
            ),
            level: if index < side * side {
                homm3_rmg::domain::Level::Surface
            } else {
                homm3_rmg::domain::Level::Underground
            },
        };
        write!(
            actual,
            " {}",
            u8::from(map.cell(at).unwrap().connection_visited())
        )
        .unwrap();
    }
    actual.push('\n');
}
