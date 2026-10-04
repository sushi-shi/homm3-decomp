//! Native guard construction and placement, including payload IDs and cell state.
use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    boundaries::BoundaryWorkspace,
    domain::{Level, WorldPosition},
    geometry::Point,
    layout::LayoutWorkspace,
    placement::{
        MonsterPayload, ObjectArena, ObjectId, ObjectPayload, PlacementMap, PlacementWorkspace,
    },
    placement_rules::PlacementRules,
    prototype::{PrototypeCatalog, PrototypeSource},
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
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE guard-objects checkpoints"]
fn native_guard_objects_preserve_payloads_ids_cells_and_rng() {
    let directory = PathBuf::from(std::env::var_os("HOMM3_RMG_DATA").unwrap());
    let oracle = PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    let mut installation = Installation::open(&directory).unwrap();
    let mut objects = Vec::new();
    installation.text("objects.txt", &mut objects).unwrap();
    let source = PrototypeSource::parse(&objects, |name| installation.mask(name)).unwrap();
    let mut creature_bytes = Vec::new();
    installation
        .text("crtraits.txt", &mut creature_bytes)
        .unwrap();
    let creatures = CreatureCatalog::parse(&creature_bytes).unwrap();
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
        ("retail", Behavior::Retail(RetailProfile::default())),
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
            let actual = snapshot(&mut map, &mut objects, &catalog, &creatures);
            checked += compare(&oracle, mode, case, &actual);
        }
    }
    eprintln!("checked {checked} native guard-object checkpoint lines");
}

fn monster(objects: &ObjectArena, object: ObjectId) -> &MonsterPayload {
    let ObjectPayload::Monster(payload) = objects.payload(object).unwrap() else {
        panic!("expected guard payload")
    };
    payload
}
fn position(index: usize, side: usize) -> WorldPosition {
    WorldPosition {
        point: Point::new(
            i32::try_from(index % side).unwrap(),
            i32::try_from(index / side % side).unwrap(),
        ),
        level: if index < side * side {
            Level::Surface
        } else {
            Level::Underground
        },
    }
}
fn snapshot(
    map: &mut PlacementMap<'_, '_, '_>,
    objects: &mut ObjectArena,
    catalog: &PrototypeCatalog<'_>,
    creatures: &CreatureCatalog,
) -> String {
    let mut actual = String::new();
    for index in 0..map.coverage().map().zones().len() {
        let zone = map.coverage().map().zones()[index].id();
        for value in [0, 2000, 10000, 50000] {
            for seed in [1, 42] {
                let mut rng = RetailRng::new(seed);
                let guard = map
                    .create_guard(value, zone, objects, catalog, creatures, &mut rng)
                    .unwrap();
                let data = guard.map_or([-1; 4], |object| {
                    let guard = monster(objects, object);
                    assert!(objects.get(object).unwrap().position().is_none());
                    assert!(objects.entrance(catalog, object).unwrap().is_none());
                    [
                        i64::from(guard.id().value()),
                        i64::from(guard.count()),
                        i64::from(guard.disposition()),
                        i64::try_from(
                            catalog
                                .get(objects.get(object).unwrap().prototype())
                                .unwrap()
                                .prototype()
                                .source_row(),
                        )
                        .unwrap(),
                    ]
                });
                writeln!(
                    actual,
                    "{index} {value} {seed} {} {} {} {} {} {}",
                    data[0],
                    data[1],
                    data[2],
                    data[3],
                    rng.state(),
                    map.next_object_id()
                )
                .unwrap();
            }
        }
    }
    assert!(map.active_objects().is_empty());
    let side = map.coverage().map().raster().dimension();
    for index in 0..map.coverage().map().zones().len() {
        let zone = map.coverage().map().zones()[index].id();
        let Some(cell) = map
            .coverage()
            .map()
            .raster()
            .cells()
            .iter()
            .position(|cell| cell.zone == Some(zone))
        else {
            continue;
        };
        let mut rng = RetailRng::new(42);
        let at = position(cell, side);
        map.place_guard(10000, at, objects, catalog, creatures, &mut rng)
            .unwrap();
        map.place_guard(10000, at, objects, catalog, creatures, &mut rng)
            .unwrap();
        writeln!(
            actual,
            "{index} {cell} {} {} {}",
            rng.state(),
            map.next_object_id(),
            map.active_objects().len()
        )
        .unwrap();
    }
    writeln!(actual, "{}", map.active_objects().len()).unwrap();
    for &object in map.active_objects() {
        let guard = monster(objects, object);
        let geometry = objects.get(object).unwrap();
        let at = geometry.position().unwrap();
        writeln!(
            actual,
            "{} {} {} {} {} {} {}",
            guard.id().value(),
            guard.count(),
            guard.disposition(),
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
    write_counts(&mut actual, map);
    write_cells(&mut actual, map, |object| {
        monster(objects, object).id().value()
    });
    actual
}
fn compare(oracle: &Path, mode: &str, case: usize, actual: &str) -> usize {
    let expected = std::fs::read_to_string(oracle.join(format!(
        "{mode}-layout/case-{case}-candidate/guard-objects.txt"
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
