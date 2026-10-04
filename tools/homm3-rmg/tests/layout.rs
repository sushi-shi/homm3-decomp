//! Compare against checkpoints captured from the real VC6 generator.

use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    boundaries::{BoundaryMap, BoundaryWorkspace, TerrainCoverage},
    domain::Level,
    layout::LayoutWorkspace,
    request::{default_record, Levels, MapSize, Request},
    rng::RetailRng,
    selection::{resolve_water, SelectedTemplate},
    template::TemplateSource,
};
use std::fmt::Write;

#[test]
#[ignore = "requires HOMM3_RMG_TEMPLATE and HOMM3_RMG_ORACLE layout checkpoint directory"]
fn zone_layout_matches_vc6_checkpoints() {
    let bytes = std::fs::read(std::env::var_os("HOMM3_RMG_TEMPLATE").unwrap()).unwrap();
    let root = std::path::PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    let source = TemplateSource::parse(&bytes).unwrap();
    let mut workspace = LayoutWorkspace::default();
    let mut boundaries = BoundaryWorkspace::default();
    for (mode, behavior) in [
        ("retail", Behavior::Retail(RetailProfile::default())),
        ("hotfix", Behavior::Hotfix),
    ] {
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
            let candidates = source.prepare(&request, water).unwrap();
            let selected = SelectedTemplate::select(&candidates, &request, &mut rng).unwrap();
            let zones = workspace
                .generate(&selected, &request, water, &mut rng)
                .unwrap();
            assert_layout(
                &selected,
                zones,
                &rng,
                &root.join(format!("{mode}-layout/case-{case}-candidate/layout.txt")),
            );
            let boundary_map = boundaries
                .generate(selected.template(), zones, &request, water, &mut rng)
                .unwrap();
            for level in [Level::Surface, Level::Underground]
                .into_iter()
                .take(levels.count() as usize)
            {
                assert_boundaries(
                    &boundary_map,
                    size,
                    level,
                    &root.join(format!(
                        "{mode}-layout/case-{case}-candidate/boundary-{}.txt",
                        level.index()
                    )),
                );
            }
            assert_graph(
                &boundary_map,
                &root.join(format!("{mode}-layout/case-{case}-candidate/graph.txt")),
            );
            let coverage = boundary_map.prepare_terrain(&mut rng).unwrap();
            assert_coverage(
                &coverage,
                &root.join(format!("{mode}-layout/case-{case}-candidate/coverage.txt")),
            );
        }
    }
}

fn assert_graph(map: &BoundaryMap<'_>, expected_path: &std::path::Path) {
    let mut checkpoint = "connections\n".to_owned();
    for zone in map.zones() {
        for link in map
            .connections()
            .iter()
            .filter(|link| link.source == zone.id())
        {
            writeln!(
                checkpoint,
                "{} {} {} {} {} {}",
                link.source.index(),
                link.destination.index(),
                link.value,
                u8::from(link.unguarded),
                u8::from(link.border_guard),
                u8::from(link.connected)
            )
            .unwrap();
        }
    }
    checkpoint.push_str("distances\n");
    for zone in map.zones() {
        write!(checkpoint, "{}", zone.id().index()).unwrap();
        for distance in map.distances(zone.id()).unwrap() {
            write!(checkpoint, " {distance}").unwrap();
        }
        checkpoint.push('\n');
    }
    let expected = std::fs::read_to_string(expected_path).unwrap();
    assert_eq!(
        checkpoint,
        expected.replace("\r\n", "\n"),
        "{}",
        expected_path.display()
    );
}

fn assert_coverage(coverage: &TerrainCoverage<'_>, expected_path: &std::path::Path) {
    let map = coverage.map();
    let mut checkpoint = format!("{}\n", coverage.rng().state);
    for zone in map.zones() {
        let position = zone.position();
        write!(
            checkpoint,
            "{} {} {} {}",
            zone.id().index(),
            position.point.x,
            position.point.y,
            position.level.index()
        )
        .unwrap();
        if let Some(bounds) = zone.bounds() {
            writeln!(
                checkpoint,
                " {} {} {} {}",
                bounds.minimum().x,
                bounds.minimum().y,
                bounds.maximum().x,
                bounds.maximum().y
            )
            .unwrap();
        } else {
            checkpoint.push_str(" empty\n");
        }
    }
    checkpoint.push_str("cells\n");
    for row in map.raster().cells().chunks(map.raster().dimension()) {
        for cell in row {
            write!(
                checkpoint,
                "{},{} ",
                cell.zone
                    .map_or(-1, |zone| i32::try_from(zone.index()).unwrap()),
                u8::from(cell.paint_terrain)
            )
            .unwrap();
        }
        checkpoint.push('\n');
    }
    let expected = std::fs::read_to_string(expected_path)
        .unwrap()
        .replace("\r\n", "\n");
    for (line, (actual, expected)) in checkpoint.lines().zip(expected.lines()).enumerate() {
        assert_eq!(actual, expected, "{} line {line}", expected_path.display());
    }
    assert_eq!(checkpoint.lines().count(), expected.lines().count());
}

fn assert_boundaries(
    map: &BoundaryMap<'_>,
    size: MapSize,
    level: Level,
    expected_path: &std::path::Path,
) {
    let mut checkpoint = format!("{}\n", map.level_rng(level).unwrap().state);
    for zone in map
        .zones()
        .iter()
        .filter(|zone| zone.position().level == level)
    {
        write!(checkpoint, "{}", zone.id().index()).unwrap();
        for vertex in map.polygon(zone.id()) {
            write!(checkpoint, " {},{}", vertex.x, vertex.y).unwrap();
        }
        checkpoint.push('\n');
    }
    checkpoint.push_str("cells\n");
    let dimension = size.dimension() as usize;
    let plane = &map.raster().cells()
        [level.index() * dimension * dimension..(level.index() + 1) * dimension * dimension];
    for row in plane.chunks(dimension) {
        for cell in row {
            write!(
                checkpoint,
                "{},{} ",
                cell.zone
                    .map_or(-1, |zone| i32::try_from(zone.index()).unwrap()),
                u8::from(cell.paint_terrain)
            )
            .unwrap();
        }
        checkpoint.push('\n');
    }
    let expected = std::fs::read_to_string(expected_path).unwrap();
    let expected = expected.replace("\r\n", "\n");
    // Keep diagnostics readable: large cell dumps are available as artifacts.
    for (line, (actual, expected)) in checkpoint.lines().zip(expected.lines()).enumerate() {
        assert_eq!(actual, expected, "{} line {line}", expected_path.display());
    }
    assert_eq!(checkpoint.lines().count(), expected.lines().count());
}

fn assert_layout(
    selected: &SelectedTemplate<'_>,
    zones: &[homm3_rmg::layout::ZoneLayout],
    rng: &RetailRng,
    expected_path: &std::path::Path,
) {
    let mut checkpoint = String::new();
    writeln!(
        checkpoint,
        "{}\n{}",
        std::str::from_utf8(selected.template().name()).unwrap(),
        rng.state()
    )
    .unwrap();
    for player in selected.players() {
        write!(
            checkpoint,
            "{} ",
            player.map_or(-1, |p| i32::try_from(p.index()).unwrap())
        )
        .unwrap();
    }
    checkpoint.push('\n');
    for zone in zones {
        let position = zone.position();
        writeln!(
            checkpoint,
            "{} {} {} {} {} {} {} {}",
            zone.id().index(),
            position.point.x,
            position.point.y,
            position.level.index(),
            zone.scaled_size(),
            zone.alignment()
                .map_or(-1, |town| i32::try_from(town.index()).unwrap()),
            zone.terrain().index(),
            zone.creature_town()
                .map_or(-1, |town| i32::try_from(town.index()).unwrap())
        )
        .unwrap();
    }
    let expected = std::fs::read_to_string(expected_path).unwrap();
    assert_eq!(
        checkpoint,
        expected.replace("\r\n", "\n"),
        "{}",
        expected_path.display()
    );
}
