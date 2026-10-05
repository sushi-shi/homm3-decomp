//! Real installation loading through disk-backed archive indexes.

use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    placement_rules::PlacementRules,
    prototype::PrototypeSource,
    request::{Levels, MapVersion},
    template::TemplateSource,
    traits::{ArtifactCatalog, CreatureCatalog, SpellCatalog},
};
use homm3_rmg_cli::resources::Installation;
use std::path::PathBuf;

#[path = "../../homm3-rmg/tests/support/prototype_snapshot.rs"]
mod prototype_snapshot;

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE native checkpoints for all formats"]
fn installation_resources_produce_native_catalogs_without_loading_archive_images() {
    let directory = PathBuf::from(std::env::var_os("HOMM3_RMG_DATA").unwrap());
    let oracle = PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    let mut installation = Installation::open(&directory).unwrap();
    let mut bytes = Vec::new();
    installation.text("crtraits.txt", &mut bytes).unwrap();
    assert_eq!(CreatureCatalog::parse(&bytes).unwrap().entries().len(), 150);
    installation.text("sptraits.txt", &mut bytes).unwrap();
    assert_eq!(SpellCatalog::parse(&bytes).unwrap().entries().len(), 81);
    installation.text("artraits.txt", &mut bytes).unwrap();
    let artifacts = ArtifactCatalog::parse(&bytes).unwrap();
    assert_eq!(artifacts.entries().len(), 144);
    installation.text("rmg.txt", &mut bytes).unwrap();
    TemplateSource::parse(&bytes).unwrap();
    let mut objects = Vec::new();
    installation.text("objects.txt", &mut objects).unwrap();
    let source = PrototypeSource::parse(&objects, |name| installation.mask(name)).unwrap();
    assert_eq!(source.rows().len(), 1326);
    assert_eq!(source.image_count(), 1305);
    installation.text("rand_trn.txt", &mut bytes).unwrap();
    for (mode, behavior) in [
        ("retail", Behavior::Retail(RetailProfile::default())),
        ("hotfix", Behavior::Hotfix),
    ] {
        let rules = PlacementRules::parse(&bytes, behavior).unwrap();
        for (case, version) in [
            MapVersion::ShadowOfDeath,
            MapVersion::ArmageddonsBlade,
            MapVersion::Restoration,
        ]
        .into_iter()
        .enumerate()
        {
            let catalog = source.prepare(&rules, version, behavior).unwrap();
            let actual = prototype_snapshot::snapshot(&catalog, &rules);
            let expected = std::fs::read_to_string(oracle.join(format!(
                "{mode}-layout/case-{case}-candidate/prototypes.txt"
            )))
            .unwrap();
            assert_eq!(actual, expected.replace("\r\n", "\n"), "{mode} {version:?}");
            let ready = catalog
                .into_generation(Levels::Underground, &artifacts)
                .unwrap();
            assert_eq!(ready.version(), version);
            assert_eq!(ready.behavior(), behavior);
            assert_eq!(ready.levels(), Levels::Underground);
        }
    }
}
