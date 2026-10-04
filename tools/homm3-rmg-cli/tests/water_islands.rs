//! Native water-zone island terrain, movement metadata and RNG checkpoints.
use homm3_rmg::{
    behavior::{Behavior, RetailProfile, TownMask},
    boundaries::BoundaryWorkspace,
    layout::LayoutWorkspace,
    placement::{ObjectArena, PlacementWorkspace},
    placement_rules::PlacementRules,
    prototype::PrototypeSource,
    request::{default_record, Levels, MapSize, Request},
    rng::RetailRng,
    selection::{resolve_water, SelectedTemplate},
    template::TemplateSource,
    terrain::TerrainWorkspace,
};
use homm3_rmg_cli::resources::Installation;
use std::path::PathBuf;
mod support;
use support::towns::snapshot;
#[path = "support/terrain.rs"]
mod terrain_snapshot;
use terrain_snapshot::write_terrain_and_distances;

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE water-islands checkpoints"]
fn native_water_islands_preserve_cells_and_rng() {
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
            let borders = towns.reserve_connection_borders(&mut rng).unwrap();
            let islands = borders.place_water_islands(&mut rng).unwrap();
            assert_eq!(islands.map().terrain().tiles().as_ptr(), tile_address);
            let mut actual = snapshot(islands.towns(), &objects, &catalog, islands.rng());
            write_terrain_and_distances(&mut actual, islands.map());
            eprintln!(
                "{mode} case {case}: {} towns, RNG {}",
                islands.towns().towns(&objects).unwrap().count(),
                islands.rng().state
            );
            checked += actual.lines().count();
            let expected = std::fs::read_to_string(oracle.join(format!(
                "{mode}-layout/case-{case}-candidate/water-islands.txt"
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
    eprintln!("checked {checked} native water-island checkpoint lines");
}
