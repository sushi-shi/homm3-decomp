//! Checks against installed packs; no copyrighted template bytes are committed.

use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    layout::{hints::ZoneSolution, LayoutWorkspace},
    request::{default_record, Levels, MapSize, Request, RequestOptions, Water},
    rng::RetailRng,
    rules::Ruleset,
    selection::SelectedTemplate,
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
    let mut hint_diagnostics = 0;
    let mut fatal_hints = 0;
    let mut layout_workspace = LayoutWorkspace::default();
    let mut layouts = 0;
    let mut terrain_counts = [0_usize; 12];
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
            MapSize::Huge,
            MapSize::ExtraHuge,
            MapSize::Giant,
        ] {
            for levels in [Levels::Surface, Levels::Underground] {
                for (humans, computers) in [(2, 0), (1, 3), (2, 4), (8, 0)] {
                    let mut record = default_record(size, levels);
                    record.m_humanPlayerCount = humans;
                    record.m_computerPlayerCount = computers;
                    let request = Request::parse_with_options(
                        record,
                        Behavior::Retail(RetailProfile::default()),
                        RequestOptions {
                            ruleset: Ruleset::HotA181,
                            mirror: settings.mirror,
                            ..RequestOptions::default()
                        },
                    )
                    .unwrap();
                    for water in [Water::None, Water::Normal, Water::Islands] {
                        for template in source
                            .prepare(&request, water)
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
                            let one = [TemplateCandidate::Ready(template)];
                            let mut rng = RetailRng::new(42);
                            let selected =
                                SelectedTemplate::select(&one, &request, &mut rng).unwrap();
                            let before = rng.checkpoint();
                            let mut hints =
                                ZoneSolution::from_selected(&selected, &request, 42, |read| {
                                    Some(if read.steps == 0 { 0 } else { 100_000_001 })
                                })
                                .unwrap_or_else(|e| panic!("{}: {e}", path.display()));
                            hint_diagnostics += hints.diagnostics().len();
                            fatal_hints += usize::from(hints.diagnostics().iter().any(|d| d.fatal));
                            if !hints.diagnostics().iter().any(|d| d.fatal) {
                                for zone in selected.template().zones() {
                                    let initial = zone.towns()[2].initial_count
                                        + zone.towns()[3].initial_count;
                                    if zone.towns()[0].initial_count + zone.towns()[1].initial_count
                                        > 0
                                    {
                                        assert!(hints
                                            .town(zone.source_number(), -1)
                                            .unwrap()
                                            .is_some());
                                    }
                                    for instance in 0..initial {
                                        assert!(hints
                                            .town(zone.source_number(), instance)
                                            .unwrap()
                                            .is_some());
                                    }
                                    if zone.towns()[3].density.is_some() {
                                        hints.town(zone.source_number(), initial.max(0)).unwrap();
                                    }
                                }
                            }
                            assert!(
                                hints.diagnostics().is_empty(),
                                "{}: {:?}",
                                path.display(),
                                hints.diagnostics()
                            );
                            assert_eq!(rng.checkpoint(), before);
                            let layout = layout_workspace.generate_with_hints(
                                &selected, &request, water, 42, &mut rng,
                                |read| Some(if read.steps == 0 { 0 } else { 100_000_001 }),
                            ).unwrap_or_else(|e| panic!("{} {size:?} {levels:?} {humans}/{computers} {water:?}: {e}", path.display()));
                            assert_eq!(layout.zones().len(), selected.template().zones().len());
                            assert!(layout.hints().unwrap().diagnostics().is_empty());
                            for zone in layout.zones() {
                                terrain_counts[zone.terrain().index()] += 1;
                                assert!(
                                    zone.position().level.index()
                                        < request.constructor_parameters().levels.count() as usize
                                );
                            }
                            layouts += 1;
                        }
                    }
                }
            }
        }
    }
    assert_eq!(layouts, candidates);
    assert!(terrain_counts[10] > 0 && terrain_counts[11] > 0);
    eprintln!("{layouts} completed layouts, terrain counts {terrain_counts:?}");
    assert!(candidates > 0);
    assert!(mirrors > 0);
    eprintln!("{} installed packs, {mirrors} mirror packs, {candidates} prepared candidates, {deferred} deferred native faults",paths.len());
    eprintln!("hint solves: {hint_diagnostics} diagnostics, {fatal_hints} fatal results");
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
