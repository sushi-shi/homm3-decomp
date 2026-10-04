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
#[path = "support/placement.rs"]
mod placement_snapshot;
use placement_snapshot::{write_cells, write_counts};

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE ground-connections checkpoints"]
fn native_ground_crossings_preserve_guards_entrances_cells_and_rng() {
    let directory = PathBuf::from(std::env::var_os("HOMM3_RMG_DATA").unwrap());
    let oracle = PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    let mut installation = Installation::open(&directory).unwrap();
    let mut objects = Vec::new();
    installation.text("objects.txt", &mut objects).unwrap();
    let source = PrototypeSource::parse(&objects, |name| installation.mask(name)).unwrap();
    let mut bytes = Vec::new();
    installation.text("rand_trn.txt", &mut bytes).unwrap();
    let mut creature_bytes = Vec::new();
    installation
        .text("crtraits.txt", &mut creature_bytes)
        .unwrap();
    let creatures = CreatureCatalog::parse(&creature_bytes).unwrap();
    let templates = read_templates(&mut installation);
    let templates = TemplateSource::parse(&templates).unwrap();
    let mut layout = LayoutWorkspace::default();
    let mut boundaries = BoundaryWorkspace::default();
    let mut terrain = TerrainWorkspace::default();
    let mut placement = PlacementWorkspace::default();
    let mut checked = 0;
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
        for (case, (seed, size, levels, version, water)) in [
            (1, MapSize::Small, Levels::Surface, 2, 0),
            (42, MapSize::Medium, Levels::Underground, 1, 1),
            (100, MapSize::Large, Levels::Surface, 0, 2),
            (17, MapSize::ExtraLarge, Levels::Underground, 2, 3),
        ]
        .into_iter()
        .enumerate()
        {
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
            let actual = snapshot(
                &mut connecting,
                &mut objects,
                &catalog,
                &creatures,
                &mut rng,
                &colors,
            );
            checked += compare(&oracle, mode, case, &actual);
        }
    }
    eprintln!("checked {checked} native ground-crossing checkpoint lines");
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
fn snapshot(
    connecting: &mut ConnectingZones<'_, '_, '_>,
    objects: &mut ObjectArena,
    catalog: &homm3_rmg::prototype::PrototypeCatalog<'_>,
    creatures: &CreatureCatalog,
    rng: &mut RetailRng,
    colors: &[homm3_rmg::placement::KeyTentColor],
) -> String {
    let mut actual = String::new();
    for zone in connecting.map().coverage().map().zones() {
        assert!(connecting
            .map()
            .zone_entrances(zone.id())
            .unwrap()
            .is_empty());
    }
    for zone_index in 0..connecting.map().coverage().map().zones().len() {
        let zone = connecting.map().coverage().map().zones()[zone_index];
        if zone.terrain() == Terrain::Water {
            continue;
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
            let result = connecting
                .try_ground_connection(id, objects, catalog, creatures, rng)
                .unwrap();
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
    write_tents(&mut actual, connecting.map(), colors);
    write_objects(&mut actual, connecting.map(), objects, catalog);
    let map = connecting.map();
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
    write_counts(&mut actual, map);
    write_cells(&mut actual, map, |id| {
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
        writeln!(
            actual,
            "{} {} {} {} {}",
            cell.movement().cost(),
            previous[0],
            previous[1],
            previous[2],
            cell.border().map_or(-1, |c| i32::from(c.value()))
        )
        .unwrap();
    }
    actual
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
fn compare(oracle: &Path, mode: &str, case: usize, actual: &str) -> usize {
    let expected = std::fs::read_to_string(oracle.join(format!(
        "{mode}-layout/case-{case}-candidate/ground-connections.txt"
    )))
    .unwrap();
    assert_eq!(
        actual.split_whitespace().count(),
        expected.split_whitespace().count(),
        "{mode} case{case} field count"
    );
    for (index, (a, b)) in actual.lines().zip(expected.lines()).enumerate() {
        assert_eq!(
            a.split_whitespace().collect::<Vec<_>>(),
            b.split_whitespace().collect::<Vec<_>>(),
            "{mode} case{case} line{index}"
        );
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
