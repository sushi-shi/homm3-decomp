//! Installed-resource checks; copyrighted game data stays outside the repository.

use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    raw,
    request::{default_record, Levels, MapSize, Request, Water},
    template::{TemplateCandidate, TemplateSource},
};

#[test]
#[ignore = "requires HOMM3_RMG_TEMPLATE pointing to an extracted installed rmg.txt"]
fn installed_templates_cover_supported_requests() {
    let path = std::env::var_os("HOMM3_RMG_TEMPLATE").expect("set HOMM3_RMG_TEMPLATE");
    let bytes = std::fs::read(path).unwrap();
    let source = TemplateSource::parse(&bytes).unwrap();
    for behavior in [Behavior::Retail(RetailProfile::default()), Behavior::Hotfix] {
        let mut cases = 0;
        let mut accepted = 0;
        let mut deferred_faults = 0;
        for size in [
            MapSize::Small,
            MapSize::Medium,
            MapSize::Large,
            MapSize::ExtraLarge,
        ] {
            for levels in [Levels::Surface, Levels::Underground] {
                for version in [
                    raw::RMG_MAP_RESTORATION_OF_ERATHIA,
                    raw::RMG_MAP_ARMAGEDDONS_BLADE,
                    raw::RMG_MAP_SHADOW_OF_DEATH,
                ] {
                    for (humans, computers) in [(0, 2), (1, 0), (1, 1), (2, 0), (4, 4), (8, 0)] {
                        let mut record = default_record(size, levels);
                        record.m_mapVersion = version;
                        record.m_humanPlayerCount = humans;
                        record.m_computerPlayerCount = computers;
                        let request = Request::parse(record, behavior).unwrap();
                        for water in [Water::None, Water::Normal, Water::Islands] {
                            let templates = source.prepare(&request, water).unwrap();
                            for candidate in &templates {
                                let TemplateCandidate::Ready(template) = candidate else {
                                    assert!(!behavior.is_hotfix());
                                    deferred_faults += 1;
                                    continue;
                                };
                                for zone in template.zones() {
                                    for connection in zone.connections() {
                                        assert!(
                                            connection.destination().index()
                                                < template.zones().len()
                                        );
                                    }
                                }
                            }
                            accepted += templates.len();
                            cases += 1;
                        }
                    }
                }
            }
        }
        assert!(
            accepted > 0,
            "no installed templates usable in {behavior:?}"
        );
        eprintln!("{behavior:?}: {cases} requests, {accepted} candidates, {deferred_faults} deferred faults");
    }
}
