//! Native generic zone placement, including failed attempts and reused object slots.
use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    boundaries::BoundaryWorkspace,
    layout::LayoutWorkspace,
    object::ObjectKind,
    placement::{ObjectArena, PlacementError, PlacementMap, PlacementWorkspace},
    placement_rules::PlacementRules,
    prototype::{PrototypeCatalog, PrototypeSource},
    raw,
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
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE zone-objects checkpoints"]
fn native_zone_placement_preserves_candidates_registration_and_rng() {
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
            let actual = snapshot(&mut map, &mut objects, &catalog);
            checked += compare(&oracle, mode, case, &actual);
        }
    }
    eprintln!("checked {checked} native zone-placement checkpoint lines");
}

fn snapshot(
    map: &mut PlacementMap<'_, '_, '_>,
    objects: &mut ObjectArena,
    catalog: &PrototypeCatalog<'_>,
) -> String {
    let mut actual = String::new();
    let mut attempts = 0;
    let mut failures = 0;
    for index in 0..map.coverage().map().zones().len() {
        let zone = map.coverage().map().zones()[index].id();
        for family in [
            raw::BORDER_TENT,
            raw::LITH_TWOWAY,
            raw::UNDERGROUND_GATE,
            raw::SHIPYARD,
        ] {
            let kind = ObjectKind::parse(i32::try_from(family).unwrap()).unwrap();
            let Some(prototype) = catalog.at(kind, 0) else {
                continue;
            };
            for attempt in 0..2 {
                let object = objects.create(catalog, prototype).unwrap();
                let mut rng = RetailRng::new(42);
                let placed = map
                    .place_object_in_zone(object, zone, objects, catalog, &mut rng)
                    .unwrap();
                let geometry = objects.get(object).unwrap();
                let coordinates = geometry.position().map_or([-1; 3], |p| {
                    [
                        p.point.x,
                        p.point.y,
                        i32::try_from(p.level.index()).unwrap(),
                    ]
                });
                writeln!(
                    actual,
                    "{index} {family} {attempt} {} {} {} {} {} {} {} {}",
                    catalog.get(prototype).unwrap().prototype().source_row(),
                    i32::from(placed),
                    coordinates[0],
                    coordinates[1],
                    coordinates[2],
                    rng.state(),
                    map.next_object_id(),
                    map.active_objects().len()
                )
                .unwrap();
                attempts += 1;
                if placed {
                    assert!(matches!(
                        objects.discard_unplaced(object),
                        Err(PlacementError::PreviouslyPlaced(_))
                    ));
                } else {
                    failures += 1;
                    assert_eq!(rng.state(), 42);
                    objects.discard_unplaced(object).unwrap();
                    // Reusing the exact slot must not let this handle place its replacement.
                    let replacement = objects.create(catalog, prototype).unwrap();
                    assert_eq!(replacement.index(), object.index());
                    assert!(matches!(
                        map.place_object_in_zone(object, zone, objects, catalog, &mut rng),
                        Err(PlacementError::UnknownObject(_))
                    ));
                    assert_eq!(rng.state(), 42);
                    assert!(objects.get(replacement).unwrap().position().is_none());
                    objects.discard_unplaced(replacement).unwrap();
                }
            }
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
    if let Some(&object) = map.active_objects().first() {
        map.unregister_object(objects, catalog, object).unwrap();
        assert!(matches!(
            objects.discard_unplaced(object),
            Err(PlacementError::PreviouslyPlaced(_))
        ));
    }
    eprintln!("zone placement: {attempts} attempts, {failures} failures");
    actual
}
fn write_registered_objects(
    actual: &mut String,
    map: &PlacementMap<'_, '_, '_>,
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
        "{mode}-layout/case-{case}-candidate/zone-objects.txt"
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
