//! Native cell mutations: ordered overlaps, clipped footprints and border reservations.
use homm3_rmg::{
    behavior::{Behavior, RetailProfile, TownMask},
    boundaries::BoundaryWorkspace,
    domain::{Level, WorldPosition},
    geometry::Point,
    layout::LayoutWorkspace,
    object::ObjectKind,
    placement::{
        BorderColor, Neighborhood, ObjectArena, ObjectId, PathReservation, PlacementError,
        PlacementMap, PlacementWorkspace,
    },
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
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE placement-mutation checkpoints"]
fn native_cell_mutations_preserve_flags_and_membership_order() {
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
            let actual = snapshot(&mut map, &catalog);
            checked += actual.lines().count();
            let expected = std::fs::read_to_string(oracle.join(format!(
                "{mode}-layout/case-{case}-candidate/placement-mutation.txt"
            )))
            .unwrap();
            assert_eq!(actual, expected.replace("\r\n", "\n"), "{mode} case {case}");
        }
    }
    eprintln!("checked {checked} native mutation checkpoint lines");
}

fn position(x: i32, y: i32) -> WorldPosition {
    WorldPosition {
        point: Point::new(x, y),
        level: Level::Surface,
    }
}
fn initialize(map: &mut PlacementMap<'_, '_, '_>) {
    for y in 0..6 {
        for x in 0..6 {
            let point = position(x, y);
            assert!(map.objects_at(point).unwrap().next().is_none());
            map.clear_border(point).unwrap();
            match (x + y) % 3 {
                0 => map.mark_obstacle(point).unwrap(),
                1 => map.release_path(point).unwrap(),
                _ => (),
            }
        }
    }
    let border = position(5, 5);
    map.mark_border(border, BorderColor::parse(15).unwrap())
        .unwrap();
    map.open_path(border).unwrap();
    map.clear_obstacle(border).unwrap();
    map.release_path(border).unwrap();
}
fn snapshot(map: &mut PlacementMap<'_, '_, '_>, catalog: &PrototypeCatalog<'_>) -> String {
    let mut actual = String::new();
    let mut objects = ObjectArena::default();
    let monster = ObjectKind::parse(i32::try_from(raw::MONSTER).unwrap()).unwrap();
    let blocker_prototype = catalog.at(monster, 0).unwrap().id();
    for family in 0..raw::ADVENTURE_OBJECT_TRAIT_COUNT {
        let kind = ObjectKind::parse(i32::try_from(family).unwrap()).unwrap();
        let Some(prototype) = catalog.at(kind, 0).map(PrototypeRef::id) else {
            continue;
        };
        let entry = catalog.get(prototype).unwrap();
        if entry.prototype().kind().index() == raw::BORDER_GUARD as usize
            || entry.image_mask().size().is_err()
        {
            continue;
        }
        initialize(map);
        let a = objects.create(catalog, prototype).unwrap();
        let b = objects.create(catalog, prototype).unwrap();
        let blocker = objects.create(catalog, blocker_prototype).unwrap();
        let ids = [a, b, blocker];
        assert!(objects.get(a).unwrap().position().is_none());
        let anchor = position(5, 5);
        let mut ever_occupied = false;
        for phase in 0..20 {
            apply_phase(map, &mut objects, catalog, ids, phase);
            write!(actual, "{} {phase}", entry.prototype().source_row()).unwrap();
            if phase == 11 {
                let anchor = objects.get(a).unwrap().position().unwrap();
                write!(actual, " {} {}", anchor.point.x, anchor.point.y).unwrap();
            }
            actual.push('\n');
            write_cells(&mut actual, map, ids);
            if phase == 2 {
                ever_occupied = (0..6).any(|y| {
                    (0..6).any(|x| map.objects_at(position(x, y)).unwrap().any(|id| id == a))
                });
            }
            if phase == 8 {
                let removed = map.erase_footprint(&objects, catalog, a);
                if !map.coverage().map().behavior().is_hotfix() && ever_occupied {
                    assert!(matches!(removed, Err(PlacementError::MissingMembership(_))));
                } else {
                    removed.unwrap();
                }
            }
            if (2..=10).contains(&phase) {
                assert_eq!(objects.get(a).unwrap().position(), Some(anchor));
            }
        }
    }
    actual
}
fn write_cells(actual: &mut String, map: &PlacementMap<'_, '_, '_>, ids: [ObjectId; 3]) {
    for y in 0..6 {
        for x in 0..6 {
            let point = position(x, y);
            let cell = map.cell(point).unwrap().state();
            write!(
                actual,
                "{} {} {} {} {} {}",
                u8::from(cell.passable()),
                cell.entrance()
                    .map_or(-1, |kind| i32::try_from(kind.index()).unwrap()),
                u8::from(cell.reservation() == PathReservation::Obstacle),
                u8::from(cell.reservation() == PathReservation::Open),
                cell.border().map_or(-1, |color| i32::from(color.value())),
                cell.object_distance()
            )
            .unwrap();
            for object in map.objects_at(point).unwrap() {
                write!(
                    actual,
                    " {}",
                    ids.iter().position(|&id| id == object).unwrap()
                )
                .unwrap();
            }
            actual.push('\n');
        }
    }
}

fn apply_phase(
    map: &mut PlacementMap<'_, '_, '_>,
    objects: &mut ObjectArena,
    catalog: &PrototypeCatalog<'_>,
    [a, b, blocker]: [ObjectId; 3],
    phase: usize,
) {
    let anchor = position(5, 5);
    match phase {
        1 => map
            .insert_object(objects, catalog, blocker, anchor)
            .unwrap(),
        2 | 3 => map.insert_object(objects, catalog, a, anchor).unwrap(),
        4 => map
            .insert_object(objects, catalog, b, position(0, 0))
            .unwrap(),
        5 | 6 => map.erase_footprint(objects, catalog, a).unwrap(),
        7 => map.erase_footprint(objects, catalog, blocker).unwrap(),
        8 => map.erase_footprint(objects, catalog, b).unwrap(),
        9 => {
            map.clear_border(anchor).unwrap();
            map.mark_border(anchor, BorderColor::parse(3).unwrap())
                .unwrap();
            map.mark_border(anchor, BorderColor::parse(7).unwrap())
                .unwrap();
        }
        10 => map.clear_border(anchor).unwrap(),
        11 => map
            .insert_object(objects, catalog, a, position(i32::MIN, -1))
            .unwrap(),
        12 => map
            .insert_object(objects, catalog, blocker, position(3, 3))
            .unwrap(),
        13 => map.open_path_patch(position(0, 0)).unwrap(),
        14 => {
            map.mark_border(position(4, 4), BorderColor::parse(7).unwrap())
                .unwrap();
            map.mark_obstacle_patch(position(3, 3)).unwrap();
        }
        15 => map
            .release_neighborhood_path(position(3, 3), Neighborhood::ThreeByThree)
            .unwrap(),
        16 => map.open_path_patch(position(4, 4)).unwrap(),
        17 => map
            .release_neighborhood_path(position(2, 2), Neighborhood::FiveByFive)
            .unwrap(),
        18 => {
            assert_eq!(
                map.open_entrance_approach(position(4, 3)).unwrap(),
                position(4, 4)
            );
        }
        19 => {
            map.erase_footprint(objects, catalog, blocker).unwrap();
            map.clear_border(position(4, 4)).unwrap();
        }
        _ => (),
    }
}
