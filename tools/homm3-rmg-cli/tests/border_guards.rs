//! Native border-guard rows, key-tent cursors and protected crossing neighborhoods.
use homm3_rmg::{
    behavior::{Behavior, RetailProfile, TownMask},
    boundaries::BoundaryWorkspace,
    domain::{Level, WorldPosition},
    geometry::Point,
    layout::LayoutWorkspace,
    placement::{BorderGuardCount, KeyTentCursor, ObjectArena, PlacementMap, PlacementWorkspace},
    placement_rules::PlacementRules,
    prototype::{PrototypeCatalog, PrototypeSource},
    request::{default_record, Levels, MapSize, Request},
    rng::RetailRng,
    selection::{resolve_water, SelectedTemplate},
    template::TemplateSource,
    terrain::TerrainWorkspace,
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
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE border-guards checkpoints"]
fn native_border_guards_preserve_tents_cursors_cells_and_rng() {
    let directory = PathBuf::from(std::env::var_os("HOMM3_RMG_DATA").unwrap());
    let oracle = PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    let mut installation = Installation::open(&directory).unwrap();
    let mut objects = Vec::new();
    installation.text("objects.txt", &mut objects).unwrap();
    let source = PrototypeSource::parse(&objects, |name| installation.mask(name)).unwrap();
    let mut bytes = Vec::new();
    installation.text("rand_trn.txt", &mut bytes).unwrap();
    let mut templates = Vec::new();
    installation.text("rmg.txt", &mut templates).unwrap();
    let templates = TemplateSource::parse(&templates).unwrap();
    let mut layout = LayoutWorkspace::default();
    let mut boundaries = BoundaryWorkspace::default();
    let mut terrain = TerrainWorkspace::default();
    let mut placement = PlacementWorkspace::default();
    let mut checked = 0;
    for (mode, behavior) in [
        (
            "retail",
            Behavior::Retail(RetailProfile {
                // These snapshots use authored C++ all-town water flags, not retail heap residue.
                water_zone_towns: Some(TownMask::ALL),
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
            let mut objects = ObjectArena::default();
            let actual = snapshot(&mut map, &mut objects, &catalog);
            checked += compare(&oracle, mode, case, &actual);
        }
    }
    eprintln!("checked {checked} native border-guard checkpoint lines");
}

fn snapshot(
    map: &mut PlacementMap<'_>,
    objects: &mut ObjectArena,
    catalog: &PrototypeCatalog<'_>,
) -> String {
    let mut actual = String::new();
    for index in 0..map.coverage().map().zones().len() {
        let zone = map.coverage().map().zones()[index];
        map.flood_connection_costs(
            zone.position(),
            zone.terrain() == homm3_rmg::domain::Terrain::Water,
        )
        .unwrap();
    }
    for index in 0..map.coverage().map().zones().len() {
        let zone = map.coverage().map().zones()[index];
        for (attempt, count) in [BorderGuardCount::Single, BorderGuardCount::Shipyard]
            .into_iter()
            .enumerate()
        {
            let mut at = zone.position();
            at.point.x += i32::try_from(attempt).unwrap();
            let mut rng = RetailRng::new(42);
            let result = map
                .place_border_guard(at, count, zone.id(), objects, catalog, &mut rng)
                .unwrap();
            if let Some(color) = result.reported_color() {
                map.mark_border_connection_area(at, color).unwrap();
            }
            let KeyTentCursor::Value(cursor) = map.next_key_tent(catalog).unwrap() else {
                panic!("explicit replay input supplied")
            };
            writeln!(
                actual,
                "{index} {attempt} {} {} {cursor} {} {} {}",
                count as u32,
                result.reported_color().unwrap_or(-1),
                rng.state(),
                map.next_object_id(),
                map.active_objects().len()
            )
            .unwrap();
        }
    }
    write_registered_objects(&mut actual, map, objects, catalog);
    write_counts(&mut actual, map);
    write_cells(&mut actual, map, |object| {
        i32::try_from(
            map.active_objects()
                .iter()
                .position(|&id| id == object)
                .unwrap(),
        )
        .unwrap()
    });
    write_borders_and_movement(&mut actual, map);
    actual
}
fn write_borders_and_movement(actual: &mut String, map: &PlacementMap<'_>) {
    let side = i32::try_from(map.coverage().map().raster().dimension()).unwrap();
    for level in [Level::Surface, Level::Underground]
        .into_iter()
        .take(map.coverage().map().request().levels().count() as usize)
    {
        for y in 0..side {
            for x in 0..side {
                let cell = map
                    .cell(WorldPosition {
                        point: Point::new(x, y),
                        level,
                    })
                    .unwrap();
                let cell = cell.state();
                let previous = cell.movement().previous().map_or([-1; 3], |p| {
                    [
                        p.point.x,
                        p.point.y,
                        i32::try_from(p.level.index()).unwrap(),
                    ]
                });
                writeln!(
                    actual,
                    "{} {} {} {} {} {}",
                    u8::from(cell.border().is_some()),
                    cell.border().map_or(-1, |c| i32::from(c.value())),
                    cell.movement().cost(),
                    previous[0],
                    previous[1],
                    previous[2]
                )
                .unwrap();
            }
        }
    }
}
fn write_registered_objects(
    actual: &mut String,
    map: &PlacementMap<'_>,
    objects: &ObjectArena,
    catalog: &PrototypeCatalog<'_>,
) {
    writeln!(actual, "{}", map.active_objects().len()).unwrap();
    for &object in map.active_objects() {
        let geometry = objects.get(object).unwrap();
        let at = geometry.position().unwrap();
        writeln!(
            actual,
            "{} {} {} {}",
            catalog
                .get(geometry.prototype())
                .unwrap()
                .prototype()
                .source_row(),
            at.point.x,
            at.point.y,
            at.level.index()
        )
        .unwrap();
    }
}
fn compare(oracle: &Path, mode: &str, case: usize, actual: &str) -> usize {
    let expected = std::fs::read_to_string(oracle.join(format!(
        "{mode}-layout/case-{case}-candidate/border-guards.txt"
    )))
    .unwrap();
    assert_eq!(
        actual.split_whitespace().count(),
        expected.split_whitespace().count(),
        "{mode} case{case} field count"
    );
    for (index, (a, b)) in actual
        .split_whitespace()
        .zip(expected.split_whitespace())
        .enumerate()
    {
        assert_eq!(a, b, "{mode} case{case} field {index}");
    }
    eprintln!("{mode} case{case}: {} lines", actual.lines().count());
    actual.lines().count()
}

// Tiny object catalogs isolate cursor/art edge cases while the map still comes
// from the normal template, layout and terrain stages.
fn with_palette(
    behavior: Behavior,
    tents: &[i32],
    guards: &[i32],
    check: impl FnOnce(&mut PlacementMap<'_>, &mut ObjectArena, &PrototypeCatalog<'_>),
) {
    use homm3_rmg::{prototype::ImageMask, raw};
    use std::convert::Infallible;
    let directory = PathBuf::from(std::env::var_os("HOMM3_RMG_DATA").unwrap());
    let mut installation = Installation::open(&directory).unwrap();
    let mut bytes = Vec::new();
    installation.text("rmg.txt", &mut bytes).unwrap();
    let templates = TemplateSource::parse(&bytes).unwrap();
    let mut rows = format!("{}\r\n", tents.len() + guards.len() + 1);
    for (kind, subtype) in tents
        .iter()
        .map(|&s| (raw::BORDER_TENT, s))
        .chain(guards.iter().map(|&s| (raw::BORDER_GUARD, s)))
        .chain([(raw::MONSTER, 0)])
    {
        write!(
            rows,
            "test.def 0 {:048b} 011111111 011111111 {kind} {subtype} 0 0\r\n",
            1_u64 << 47
        )
        .unwrap();
    }
    let mut mask = [0; 14];
    mask[0] = 1;
    mask[1] = 1;
    mask[7] = 128;
    let mask = ImageMask::parse(&mask).unwrap();
    let source =
        PrototypeSource::parse(rows.as_bytes(), |_| Ok::<_, Infallible>(Some(mask))).unwrap();
    let rules = PlacementRules::parse(b"header\r\nheader\r\nheader\r\n", behavior).unwrap();
    let mut record = default_record(MapSize::Small, Levels::Surface);
    record.m_waterContent = 0;
    let request = Request::parse(record, behavior).unwrap();
    let catalog = source.prepare(&rules, request.version(), behavior).unwrap();
    let mut rng = RetailRng::new(1);
    let water = resolve_water(request.water(), &mut rng);
    let candidates = templates.prepare(&request, water).unwrap();
    let selected = SelectedTemplate::select(&candidates, &request, &mut rng).unwrap();
    let mut layout = LayoutWorkspace::default();
    let mut boundaries = BoundaryWorkspace::default();
    let mut terrain = TerrainWorkspace::default();
    let mut placement = PlacementWorkspace::default();
    let zones = layout
        .generate(&selected, &request, water, &mut rng)
        .unwrap();
    let map = boundaries.generate(zones, &mut rng).unwrap();
    let coverage = map.prepare_terrain(&mut rng).unwrap();
    let painted = terrain.paint(coverage, &mut rng).unwrap();
    let mut map = placement.begin(painted).unwrap();
    check(&mut map, &mut ObjectArena::default(), &catalog);
}
fn place_one(
    map: &mut PlacementMap<'_>,
    objects: &mut ObjectArena,
    catalog: &PrototypeCatalog<'_>,
    rng: &mut RetailRng,
) -> Result<homm3_rmg::placement::BorderGuardPlacement, homm3_rmg::placement::PlacementError> {
    let zone = map.coverage().map().zones()[0];
    map.place_border_guard(
        zone.position(),
        BorderGuardCount::Single,
        zone.id(),
        objects,
        catalog,
        rng,
    )
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA templates for cursor/art edge cases"]
fn raw_cursor_lookup_precedes_availability_admission_and_keeps_partial_placements() {
    use homm3_rmg::placement::{BorderGuardPlacement, PlacementError};
    let retail = |color| {
        Behavior::Retail(RetailProfile {
            initial_key_tent_color: color,
            ..RetailProfile::default()
        })
    };
    with_palette(retail(None), &[0], &[0], |map, objects, catalog| {
        let mut rng = RetailRng::new(42);
        assert!(matches!(
            place_one(map, objects, catalog, &mut rng),
            Err(PlacementError::KeyTentReplayRequired)
        ));
        assert!(map.active_objects().is_empty());
        assert_eq!(rng.state(), 42);
    });
    for color in [-1, i32::MAX] {
        with_palette(retail(Some(color)), &[0], &[0], |map, objects, catalog| {
            let mut rng = RetailRng::new(42);
            assert_eq!(
                place_one(map, objects, catalog, &mut rng).unwrap(),
                BorderGuardPlacement::NotPlaced
            );
            assert!(map.active_objects().is_empty());
            assert_eq!(rng.state(), 42);
            assert_eq!(
                map.next_key_tent(catalog).unwrap(),
                KeyTentCursor::Value(color)
            );
        });
    }
    with_palette(Behavior::Hotfix, &[], &[0], |map, objects, catalog| {
        let mut rng = RetailRng::new(42);
        assert_eq!(map.next_key_tent(catalog).unwrap(), KeyTentCursor::Value(0));
        assert_eq!(
            place_one(map, objects, catalog, &mut rng).unwrap(),
            BorderGuardPlacement::NotPlaced
        );
        assert_eq!(rng.state(), 42);
    });
    with_palette(retail(Some(7)), &[7], &[], |map, objects, catalog| {
        let mut rng = RetailRng::new(42);
        assert_eq!(
            place_one(map, objects, catalog, &mut rng).unwrap(),
            BorderGuardPlacement::MissingGuardPrototype
        );
        assert!(map.active_objects().is_empty());
        assert_eq!(rng.state(), 42);
        assert_eq!(map.next_key_tent(catalog).unwrap(), KeyTentCursor::Value(7));
    });
    for guards in [&[][..], &[1][..]] {
        with_palette(Behavior::Hotfix, &[1], guards, |map, objects, catalog| {
            let index_zero = map.key_tent_color(catalog, 0).unwrap();
            map.set_key_tent_disabled(catalog, index_zero, true)
                .unwrap();
            assert_eq!(map.next_key_tent(catalog).unwrap(), KeyTentCursor::Value(1));
            let mut rng = RetailRng::new(42);
            let result = place_one(map, objects, catalog, &mut rng);
            if guards.is_empty() {
                assert_eq!(result.unwrap(), BorderGuardPlacement::MissingGuardPrototype);
                assert!(map.active_objects().is_empty());
                assert_eq!(rng.state(), 42);
            } else {
                assert!(matches!(result, Err(PlacementError::KeyTentSubtype(1))));
                assert_eq!(map.active_objects().len(), 2);
                let mut expected = RetailRng::new(42);
                expected.draw();
                assert_eq!(rng.state(), expected.state());
            }
            assert_eq!(map.next_key_tent(catalog).unwrap(), KeyTentCursor::Value(1));
        });
    }
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA templates for unusual and duplicate subtype cases"]
fn full_color_reservation_and_first_matching_art_survive_bitfield_truncation() {
    use homm3_rmg::{object::ObjectKind, placement::BorderGuardPlacement, raw};
    let behavior = Behavior::Retail(RetailProfile {
        initial_key_tent_color: Some(16),
        ..RetailProfile::default()
    });
    let colors: Vec<_> = (0..=16).collect();
    with_palette(behavior, &colors, &[16, 16], |map, objects, catalog| {
        let mut rng = RetailRng::new(42);
        let result = place_one(map, objects, catalog, &mut rng).unwrap();
        assert!(matches!(result, BorderGuardPlacement::Placed(color) if color.index() == 16));
        assert_eq!(result.reported_color(), Some(16));
        assert_eq!(map.next_key_tent(catalog).unwrap(), KeyTentCursor::Value(0));
        let guard_kind = ObjectKind::parse(i32::try_from(raw::BORDER_GUARD).unwrap()).unwrap();
        assert_eq!(
            objects.get(map.active_objects()[1]).unwrap().prototype(),
            catalog.at(guard_kind, 0).unwrap().id()
        );
        let position = map.coverage().map().zones()[0].position();
        map.mark_border_connection_area(position, result.reported_color().unwrap())
            .unwrap();
        let mut marked = 0;
        for y in position.point.y - 1..=position.point.y + 1 {
            for x in position.point.x - 1..=position.point.x + 1 {
                if let Ok(cell) = map.cell(WorldPosition {
                    point: Point::new(x, y),
                    level: position.level,
                }) {
                    if let Some(color) = cell.state().border() {
                        assert_eq!(color.value(), 0);
                        marked += 1;
                    }
                }
            }
        }
        assert!(marked > 0);
        // Keep color16 disabled while rescanning every preceding index.
        for index in 0..16 {
            let color = map.key_tent_color(catalog, index).unwrap();
            map.set_key_tent_disabled(catalog, color, true).unwrap();
        }
        assert_eq!(
            map.next_key_tent(catalog).unwrap(),
            KeyTentCursor::Value(17)
        );
    });
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA templates for failed tent placement"]
fn failing_to_fit_a_tent_releases_its_slot_without_rng_or_reservation() {
    use homm3_rmg::{object::ObjectKind, placement::BorderGuardPlacement, raw};
    with_palette(Behavior::Hotfix, &[0], &[0], |map, objects, catalog| {
        let guard_kind = ObjectKind::parse(i32::try_from(raw::BORDER_GUARD).unwrap()).unwrap();
        let prototype = catalog.at(guard_kind, 0).unwrap().id();
        let blocker = objects.create(catalog, prototype).unwrap();
        let side = i32::try_from(map.coverage().map().raster().dimension()).unwrap();
        // A trigger in every cell leaves no connected outline or southern approach.
        for y in 0..side {
            for x in 0..side {
                map.insert_object(
                    objects,
                    catalog,
                    blocker,
                    WorldPosition {
                        point: Point::new(x, y),
                        level: Level::Surface,
                    },
                )
                .unwrap();
            }
        }
        let unused = objects.create(catalog, prototype).unwrap();
        objects.discard_unplaced(unused).unwrap();
        let mut rng = RetailRng::new(42);
        for _ in 0..3 {
            assert_eq!(
                place_one(map, objects, catalog, &mut rng).unwrap(),
                BorderGuardPlacement::NotPlaced
            );
            assert_eq!(rng.state(), 42);
            assert!(map.active_objects().is_empty());
            assert_eq!(map.next_key_tent(catalog).unwrap(), KeyTentCursor::Value(0));
            let reused = objects.create(catalog, prototype).unwrap();
            assert_eq!(reused.index(), unused.index());
            assert!(objects.get(unused).is_none());
            objects.discard_unplaced(reused).unwrap();
        }
    });
}
