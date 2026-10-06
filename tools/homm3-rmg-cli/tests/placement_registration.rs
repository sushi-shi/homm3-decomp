//! Native generator registration, counters, tent release and full distance grids.
use homm3_rmg::placement::ObjectId;
use homm3_rmg::{
    behavior::{Behavior, RetailProfile, TownMask},
    boundaries::BoundaryWorkspace,
    domain::{Level, WorldPosition},
    geometry::Point,
    layout::LayoutWorkspace,
    object::ObjectKind,
    placement::{KeyTentCursor, ObjectArena, PlacementError, PlacementMap, PlacementWorkspace},
    placement_rules::PlacementRules,
    prototype::{PrototypeCatalog, PrototypeRef, PrototypeSource},
    raw,
    request::{default_record, Levels, MapSize, Request},
    rng::RetailRng,
    selection::{resolve_water, SelectedTemplate},
    template::TemplateSource,
    terrain::TerrainWorkspace,
};
use homm3_rmg_cli::resources::Installation;
use std::{fmt::Write, path::PathBuf};

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE placement-registration checkpoints"]
fn native_registration_preserves_counts_distances_and_removal_quirks() {
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
            map.next_key_tent(&catalog).unwrap();
            let foreign_catalog = source.prepare(&rules, request.version(), behavior).unwrap();
            let mut foreign_objects = ObjectArena::default();
            let foreign_id = foreign_objects
                .create(
                    &foreign_catalog,
                    foreign_catalog.at(kind(raw::MONSTER), 0).unwrap().id(),
                )
                .unwrap();
            assert!(matches!(
                map.insert_object(
                    &mut foreign_objects,
                    &foreign_catalog,
                    foreign_id,
                    position(16, 16, Level::Surface)
                ),
                Err(PlacementError::CatalogContext)
            ));
            assert!(matches!(
                map.erase_footprint(&foreign_objects, &foreign_catalog, foreign_id),
                Err(PlacementError::CatalogContext)
            ));
            let actual = snapshot(&mut map, &catalog);
            checked += actual.lines().count();
            let expected = std::fs::read_to_string(oracle.join(format!(
                "{mode}-layout/case-{case}-candidate/placement-registration.txt"
            )))
            .unwrap();
            assert_eq!(
                actual.split_whitespace().count(),
                expected.split_whitespace().count(),
                "{mode} case {case} field count"
            );
            for (index, (a, b)) in actual
                .split_whitespace()
                .zip(expected.split_whitespace())
                .enumerate()
            {
                assert_eq!(a, b, "{mode} case {case} field {index}");
            }
        }
    }
    eprintln!("checked {checked} native registration checkpoint lines");
}

fn position(x: i32, y: i32, level: Level) -> WorldPosition {
    WorldPosition {
        point: Point::new(x, y),
        level,
    }
}
fn kind(value: u32) -> ObjectKind {
    ObjectKind::parse(i32::try_from(value).unwrap()).unwrap()
}
fn check_virgin_guard_release(
    map: &mut PlacementMap<'_>,
    catalog: &PrototypeCatalog<'_>,
    objects: &mut ObjectArena,
) -> (ObjectId, homm3_rmg::placement::KeyTentColor) {
    let guard = catalog.at(kind(raw::BORDER_GUARD), 0).unwrap().id();
    let virgin = objects.create(catalog, guard).unwrap();
    let color = map
        .key_tent_color(catalog, catalog.get(guard).unwrap().prototype().subtype())
        .unwrap();
    let choice = map.next_key_tent(catalog).unwrap();
    if map.coverage().map().behavior().is_hotfix() {
        assert!(matches!(choice, KeyTentCursor::Value(0)));
    } else {
        assert_eq!(choice, KeyTentCursor::ReplayRequired);
    }
    map.set_key_tent_disabled(catalog, color, true).unwrap();
    map.unregister_object(objects, catalog, virgin).unwrap();
    assert!(matches!(
        map.next_key_tent(catalog).unwrap(),
        KeyTentCursor::Value(0)
    ));

    (virgin, color)
}
fn snapshot(map: &mut PlacementMap<'_>, catalog: &PrototypeCatalog<'_>) -> String {
    let mut actual = String::new();
    let mut objects = ObjectArena::default();
    let (virgin, color) = check_virgin_guard_release(map, catalog, &mut objects);

    check_arena_binding(map, catalog, &mut objects);

    for family in 0..raw::ADVENTURE_OBJECT_TRAIT_COUNT {
        if family % 17 != 0
            && ![
                raw::MONSTER,
                raw::RESOURCE,
                raw::TOWN,
                raw::BORDER_GUARD,
                raw::BORDER_TENT,
            ]
            .contains(&family)
        {
            continue;
        }
        let Some(prototype) = catalog.at(kind(family), 0).map(PrototypeRef::id) else {
            continue;
        };
        let entry = catalog.get(prototype).unwrap();
        if entry.image_mask().size().is_err() {
            continue;
        }
        let a = objects.create(catalog, prototype).unwrap();
        let b = objects.create(catalog, prototype).unwrap();
        let level = if map.terrain().tiles().len()
            > map.coverage().map().raster().dimension().pow(2)
            && family % 2 == 1
        {
            Level::Underground
        } else {
            Level::Surface
        };
        let anchor = if entry.prototype().entrance().is_some() {
            position(16, 16, level)
        } else {
            position(5, 10, level)
        };
        let other = position(18, 17, level);
        if entry.prototype().kind() == kind(raw::BORDER_GUARD) {
            let color = map
                .key_tent_color(catalog, entry.prototype().subtype())
                .unwrap();
            map.set_key_tent_disabled(catalog, color, true).unwrap();
        }
        for phase in 0..9 {
            match phase {
                1 | 2 | 5 => map
                    .register_object(&mut objects, catalog, a, anchor)
                    .unwrap(),
                3 => map
                    .register_object(&mut objects, catalog, b, other)
                    .unwrap(),
                4 | 7 | 8 => map.unregister_object(&objects, catalog, a).unwrap(),
                6 => map.unregister_object(&objects, catalog, b).unwrap(),
                _ => (),
            }
            write_snapshot(
                &mut actual,
                map,
                catalog,
                [a, b],
                entry.prototype().source_row(),
                phase,
            );
        }
    }
    // Now the global vector has allocated storage: retail faults before releasing
    // an unplaced guard's color, whereas hotfix still releases it and succeeds.
    map.set_key_tent_disabled(catalog, color, true).unwrap();
    let removed = map.unregister_object(&objects, catalog, virgin);
    if map.coverage().map().behavior().is_hotfix() {
        removed.unwrap();
        assert!(matches!(
            map.next_key_tent(catalog).unwrap(),
            KeyTentCursor::Value(0)
        ));
    } else {
        assert!(matches!(removed, Err(PlacementError::NotRegistered(_))));
        assert!(
            matches!(map.next_key_tent(catalog).unwrap(), KeyTentCursor::Value(color) if color != 0)
        );
    }
    if map.coverage().map().raster().dimension() == 36 {
        check_extreme_entrance(map, catalog, &mut objects);
    }
    actual
}

fn check_extreme_entrance(
    map: &mut PlacementMap<'_>,
    catalog: &PrototypeCatalog<'_>,
    objects: &mut ObjectArena,
) {
    for family in 0..raw::ADVENTURE_OBJECT_TRAIT_COUNT {
        for (index, entry) in catalog.family(kind(family)).iter().enumerate() {
            let Some(trigger) = entry.prototype().entrance() else {
                continue;
            };
            if trigger.x() != 0 || entry.image_mask().size().is_err() {
                continue;
            }
            let object = objects
                .create(catalog, catalog.at(kind(family), index).unwrap().id())
                .unwrap();
            // Flat indexing aliases cell19; the following east step overflows.
            let anchor = position(
                i32::MAX,
                -59_652_323 + i32::from(trigger.y()),
                Level::Surface,
            );
            assert!(matches!(
                map.register_object(objects, catalog, object, anchor),
                Err(PlacementError::CoordinateOverflow)
            ));
            return;
        }
    }
    panic!("fixture needs a trigger in mask column zero");
}

fn check_arena_binding(
    map: &mut PlacementMap<'_>,
    catalog: &PrototypeCatalog<'_>,
    objects: &mut ObjectArena,
) {
    let guard = catalog.at(kind(raw::BORDER_GUARD), 0).unwrap().id();
    // A wholly clipped raw insertion still binds the same arena used by registration.
    let clipped = objects.create(catalog, guard).unwrap();
    map.insert_object(
        objects,
        catalog,
        clipped,
        position(i32::MIN, -1, Level::Surface),
    )
    .unwrap();
    let mut foreign = ObjectArena::default();
    let alien = foreign.create(catalog, guard).unwrap();
    assert!(matches!(
        map.insert_object(
            &mut foreign,
            catalog,
            alien,
            position(16, 16, Level::Surface)
        ),
        Err(PlacementError::UnknownObject(_))
    ));
    assert!(matches!(
        map.register_object(
            &mut foreign,
            catalog,
            alien,
            position(16, 16, Level::Surface)
        ),
        Err(PlacementError::UnknownObject(_))
    ));
}

fn write_snapshot(
    actual: &mut String,
    map: &mut PlacementMap<'_>,
    catalog: &PrototypeCatalog<'_>,
    [a, b]: [ObjectId; 2],
    source_row: usize,
    phase: usize,
) {
    let choice = match map.next_key_tent(catalog).unwrap() {
        KeyTentCursor::Value(color) => color,
        KeyTentCursor::ReplayRequired => panic!("guard release recalculates the cursor"),
    };
    write!(actual, "{source_row} {phase} {choice}").unwrap();
    for &object in map.active_objects() {
        write!(
            actual,
            " {}",
            [a, b].iter().position(|&id| id == object).unwrap()
        )
        .unwrap();
    }
    actual.push('\n');
    for value in 0..raw::ADVENTURE_OBJECT_TRAIT_COUNT {
        write!(actual, "{} ", map.object_count(kind(value))).unwrap();
    }
    actual.push('\n');
    for zone in map.coverage().map().zones() {
        for value in 0..raw::ADVENTURE_OBJECT_TRAIT_COUNT {
            write!(
                actual,
                "{} ",
                map.zone_object_count(zone.id(), kind(value)).unwrap()
            )
            .unwrap();
        }
        actual.push('\n');
    }
    let side = i32::try_from(map.coverage().map().raster().dimension()).unwrap();
    let levels = map.terrain().tiles().len() / map.coverage().map().raster().dimension().pow(2);
    for level in [Level::Surface, Level::Underground]
        .into_iter()
        .take(levels)
    {
        for y in 0..side {
            for x in 0..side {
                write!(
                    actual,
                    "{} ",
                    map.cell(position(x, y, level))
                        .unwrap()
                        .state()
                        .object_distance()
                )
                .unwrap();
            }
        }
    }
    actual.push('\n');
}
