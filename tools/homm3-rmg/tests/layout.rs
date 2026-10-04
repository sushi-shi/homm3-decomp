//! Compare against checkpoints captured from the real VC6 generator.

use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
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
            let expected = std::fs::read_to_string(
                root.join(format!("{mode}-layout/case-{case}-candidate/layout.txt")),
            )
            .unwrap();
            assert_eq!(
                checkpoint,
                expected.replace("\r\n", "\n"),
                "{mode} case {case}"
            );
        }
    }
}
