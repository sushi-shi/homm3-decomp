//! Native placement queries over completed terrain, before objects are added.
use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    boundaries::BoundaryWorkspace,
    domain::{Level, WorldPosition},
    geometry::Point,
    layout::LayoutWorkspace,
    placement::{ObstacleEntrances, PlacementError, PlacementMap, PlacementWorkspace},
    placement_rules::PlacementRules,
    prototype::{OutlineWorkspace, PrototypeCatalog, PrototypeSource},
    request::{default_record, Levels, MapSize, Request},
    rng::RetailRng,
    selection::{resolve_water, SelectedTemplate},
    template::TemplateSource,
    terrain::TerrainWorkspace,
};
use homm3_rmg_cli::resources::Installation;
use std::{fmt::Write, path::PathBuf};

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE placement-fit checkpoints"]
fn native_footprint_and_complete_fit_queries_match_on_generated_maps() {
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
    let mut outline = OutlineWorkspace::default();
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
            let actual = snapshot(&map, &catalog, &mut outline);
            checked += actual.lines().count();
            let expected = std::fs::read_to_string(oracle.join(format!(
                "{mode}-layout/case-{case}-candidate/placement-fit.txt"
            )))
            .unwrap();
            assert_eq!(actual, expected.replace("\r\n", "\n"), "{mode} case {case}");
        }
    }
    eprintln!("checked {checked} native footprint and complete-fit pairs");
}

fn snapshot(
    map: &PlacementMap<'_, '_, '_>,
    catalog: &PrototypeCatalog<'_>,
    outline: &mut OutlineWorkspace,
) -> String {
    let mut actual = String::new();
    let last = i32::try_from(map.coverage().map().raster().dimension()).unwrap() - 1;
    for entry in catalog.entries() {
        if entry.image_mask().size().is_err() {
            continue;
        }
        for zone in map.coverage().map().zones() {
            for point in [
                zone.position().point,
                Point::new(0, 0),
                Point::new(last, last),
            ] {
                let anchor = WorldPosition {
                    point,
                    level: zone.position().level,
                };
                let blocked = map
                    .footprint_blocked(entry, anchor, Some(zone.id()), ObstacleEntrances::Allow)
                    .unwrap();
                let fits = match map.can_place(entry, anchor, zone.id(), outline) {
                    Ok(fits) => i32::from(fits),
                    Err(PlacementError::EmptyOutline) => -1,
                    Err(error) => panic!("row {}: {error}", entry.prototype().source_row()),
                };
                writeln!(
                    actual,
                    "{} {} {} {} {} {} {fits}",
                    entry.prototype().source_row(),
                    zone.id().index(),
                    point.x,
                    point.y,
                    usize::from(anchor.level == Level::Underground),
                    i32::from(blocked)
                )
                .unwrap();
            }
        }
    }
    actual
}
