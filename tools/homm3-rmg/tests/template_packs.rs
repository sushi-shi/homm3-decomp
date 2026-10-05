//! Checks against installed packs; no copyrighted template bytes are committed.

use homm3_rmg::{
    behavior::Behavior,
    request::{default_record, Levels, MapSize, Request, Water},
    rules::Ruleset,
    template::{ConnectionKind, TemplateCandidate, TemplateFormat, TemplateSource, ZoneRole},
};

fn request(size: MapSize, levels: Levels, humans: i32, computers: i32) -> Request {
    let mut record = default_record(size, levels);
    record.m_humanPlayerCount = humans;
    record.m_computerPlayerCount = computers;
    Request::parse(record, Behavior::Hotfix).unwrap()
}

#[test]
#[ignore = "requires HOMM3_RMG_PACKS pointing to the pinned HotA_RMGTemplates directory"]
fn installed_packs_normalize_all_zone_and_connection_records() {
    let root = std::env::var_os("HOMM3_RMG_PACKS").expect("set HOMM3_RMG_PACKS");
    let mut paths: Vec<_> = std::fs::read_dir(root)
        .unwrap()
        .map(|p| p.unwrap().path())
        .filter(|p| p.extension().is_some_and(|e| e.eq_ignore_ascii_case("h3t")))
        .collect();
    paths.sort();
    assert!(!paths.is_empty());
    let mut candidates = 0;
    let mut deferred = 0;
    let mut mirrors = 0;
    for path in &paths {
        let bytes = std::fs::read(path).unwrap();
        let source = TemplateSource::parse(&bytes).unwrap();
        let settings = source.pack_settings(Ruleset::HotA181).unwrap();
        mirrors += usize::from(settings.mirror);
        for size in [
            MapSize::Small,
            MapSize::Medium,
            MapSize::Large,
            MapSize::ExtraLarge,
        ] {
            for levels in [Levels::Surface, Levels::Underground] {
                for (humans, computers) in [(2, 0), (1, 3), (2, 4), (8, 0)] {
                    let request = request(size, levels, humans, computers);
                    for water in [Water::None, Water::Normal, Water::Islands] {
                        for template in source
                            .prepare_for(&request, water, Ruleset::HotA181)
                            .unwrap_or_else(|e| panic!("{}: {e}", path.display()))
                        {
                            let TemplateCandidate::Ready(template) = template else {
                                deferred += 1;
                                continue;
                            };
                            candidates += 1;
                            for zone in template.zones() {
                                assert_eq!(zone.allowed_towns().len(), 12);
                                assert_eq!(zone.allowed_terrain().len(), 12);
                                assert_eq!(zone.allowed_monsters().len(), 13);
                                for connection in zone.connections() {
                                    if let Some(destination) = connection.destination() {
                                        assert!(destination.index() < template.zones().len());
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    assert!(candidates > 0);
    assert!(mirrors > 0);
    eprintln!("{} installed packs, {mirrors} mirror packs, {candidates} prepared candidates, {deferred} deferred native faults",paths.len());
}

#[test]
#[ignore = "requires HOMM3_RMG_PACKS pointing to the pinned HotA_RMGTemplates directory"]
fn installed_jebus_cross_retains_extended_policies_and_source_graph() {
    let root =
        std::path::PathBuf::from(std::env::var_os("HOMM3_RMG_PACKS").expect("set HOMM3_RMG_PACKS"));
    let bytes = std::fs::read(root.join("Jebus Cross.h3t")).unwrap();
    let source = TemplateSource::parse(&bytes).unwrap();
    assert_eq!(source.format(), TemplateFormat::Pack);
    let settings = source.pack_settings(Ruleset::HotA181).unwrap();
    assert_eq!(settings.name.as_deref(), Some(b"Jebus Cross".as_slice()));
    assert_eq!(settings.max_battle_rounds, Some(100));
    assert!(!settings.mirror);
    let templates = source
        .prepare_for(
            &request(MapSize::Large, Levels::Surface, 1, 3),
            Water::None,
            Ruleset::HotA181,
        )
        .unwrap();
    assert_eq!(templates.len(), 1);
    let TemplateCandidate::Ready(template) = &templates[0] else {
        panic!("valid Jebus Cross")
    };
    assert_eq!(template.zones().len(), 5);
    assert!(template.options().spell_research);
    assert!(template.options().special_weeks_disabled);
    assert!(template
        .options()
        .objects
        .as_ref()
        .is_some_and(|s| !s.is_empty()));
    let center = &template.zones()[0];
    assert!(matches!(center.role(), ZoneRole::Treasure(None)));
    assert_eq!(center.size().get(), 40);
    assert_eq!(center.allowed_terrain().iter().position(|&b| b), Some(1)); // Sand.
    assert_eq!(center.connections().len(), 4);
    assert_eq!(
        (center.treasure()[0].minimum, center.treasure()[0].maximum),
        (35_000, 55_000)
    );
    for (index, zone) in template.zones().iter().enumerate().skip(1) {
        assert!(matches!(zone.role(), ZoneRole::Human(_)));
        assert!(zone.use_native_terrain());
        assert!(zone.allowed_towns().iter().all(|&b| b));
        assert_eq!(zone.connections().len(), 1);
        assert_eq!(zone.connections()[0].destination().unwrap().index(), 0);
        assert_eq!(
            center.connections()[index - 1]
                .destination()
                .unwrap()
                .index(),
            index
        );
        assert_eq!(zone.connections()[0].value(), 45_000);
        assert_eq!(
            zone.connections()[0].options().kind,
            ConnectionKind::Automatic
        );
        assert!(zone.options().zone_repulsion);
    }
}
