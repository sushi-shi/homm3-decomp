//! Full native town-placement state and RNG checkpoints.
use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    boundaries::BoundaryWorkspace,
    domain::{Level, WorldPosition},
    geometry::Point,
    layout::LayoutWorkspace,
    object::ObjectKind,
    placement::{Fort, ObjectArena, PathReservation, PlacementWorkspace, TownsPlaced},
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
use std::{fmt::Write, path::PathBuf};

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE towns checkpoints"]
fn native_primary_and_additional_towns_preserve_payloads_cells_and_rng() {
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
            let map = placement.begin(painted).unwrap();
            assert_eq!(map.terrain().tiles().as_ptr(), tile_address);
            let catalog = source.prepare(&rules, request.version(), behavior).unwrap();
            let mut objects = ObjectArena::default();
            let towns = map.place_towns(&mut objects, &catalog, &mut rng).unwrap();
            let actual = snapshot(&towns, &objects, &catalog);
            eprintln!(
                "{mode} case {case}: {} towns, RNG {}",
                towns.towns().len(),
                towns.rng().state
            );
            checked += actual.lines().count();
            let expected = std::fs::read_to_string(
                oracle.join(format!("{mode}-layout/case-{case}-candidate/towns.txt")),
            )
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
    eprintln!("checked {checked} native town checkpoint lines");
}

fn position(x: i32, y: i32, level: Level) -> WorldPosition {
    WorldPosition {
        point: Point::new(x, y),
        level,
    }
}
fn snapshot(
    towns: &TownsPlaced<'_, '_, '_>,
    objects: &ObjectArena,
    catalog: &PrototypeCatalog<'_>,
) -> String {
    let mut actual = String::new();
    let map = towns.map();
    writeln!(
        actual,
        "{} {} {}",
        towns.rng().state,
        map.next_object_id(),
        if map.coverage().map().behavior().is_hotfix() {
            1
        } else {
            -1
        }
    )
    .unwrap();
    writeln!(actual, "{}", map.coverage().map().zones().len()).unwrap();
    for zone in map.coverage().map().zones() {
        let entrance = zone
            .primary_town()
            .unwrap_or(position(0, 0, Level::Surface));
        writeln!(
            actual,
            "{} {} {} {} {}",
            zone.alignment()
                .map_or(-1, |town| i32::try_from(town.index()).unwrap()),
            u8::from(zone.primary_town().is_some()),
            entrance.point.x,
            entrance.point.y,
            entrance.level.index()
        )
        .unwrap();
    }
    writeln!(actual, "{}", towns.towns().len()).unwrap();
    for town in towns.towns() {
        let geometry = objects.get(town.object()).unwrap();
        let anchor = geometry.position().unwrap();
        let entrance = town.entrance();
        writeln!(
            actual,
            "{} {} {} {} {} {} {} {} {} {}",
            catalog
                .get(geometry.prototype())
                .unwrap()
                .prototype()
                .source_row(),
            town.id().value(),
            town.owner()
                .map_or(-1, |owner| i32::try_from(owner.index()).unwrap()),
            u8::from(town.fort() == Fort::Present),
            anchor.point.x,
            anchor.point.y,
            anchor.level.index(),
            entrance.point.x,
            entrance.point.y,
            entrance.level.index()
        )
        .unwrap();
    }
    writeln!(actual, "{}", towns.road_targets().len()).unwrap();
    for entrance in towns.road_targets() {
        writeln!(
            actual,
            "{} {} {}",
            entrance.point.x,
            entrance.point.y,
            entrance.level.index()
        )
        .unwrap();
    }
    write_counts(&mut actual, towns);
    write_cells(&mut actual, towns);
    actual
}
fn write_counts(actual: &mut String, towns: &TownsPlaced<'_, '_, '_>) {
    let map = towns.map();
    let kind = |k| ObjectKind::parse(i32::try_from(k).unwrap()).unwrap();
    for k in 0..raw::ADVENTURE_OBJECT_TRAIT_COUNT {
        write!(actual, "{} ", map.object_count(kind(k))).unwrap();
    }
    actual.push('\n');
    for zone in map.coverage().map().zones() {
        for k in 0..raw::ADVENTURE_OBJECT_TRAIT_COUNT {
            write!(
                actual,
                "{} ",
                map.zone_object_count(zone.id(), kind(k)).unwrap()
            )
            .unwrap();
        }
        actual.push('\n');
    }
}
fn write_cells(actual: &mut String, towns: &TownsPlaced<'_, '_, '_>) {
    let map = towns.map();
    let side = i32::try_from(map.coverage().map().raster().dimension()).unwrap();
    for level in [Level::Surface, Level::Underground]
        .into_iter()
        .take(map.coverage().map().request().levels().count() as usize)
    {
        for y in 0..side {
            for x in 0..side {
                let at = position(x, y, level);
                let cell = map.cell(at).unwrap();
                write!(
                    actual,
                    "{} {} {} {} {}",
                    u8::from(cell.passable()),
                    cell.entrance()
                        .map_or(-1, |kind| i32::try_from(kind.index()).unwrap()),
                    u8::from(cell.reservation() == PathReservation::Obstacle),
                    u8::from(cell.reservation() == PathReservation::Open),
                    cell.object_distance()
                )
                .unwrap();
                for object in map.objects_at(at).unwrap() {
                    write!(
                        actual,
                        " {}",
                        towns
                            .towns()
                            .iter()
                            .find(|town| town.object() == object)
                            .unwrap()
                            .id()
                            .value()
                    )
                    .unwrap();
                }
                actual.push('\n');
            }
        }
    }
}
