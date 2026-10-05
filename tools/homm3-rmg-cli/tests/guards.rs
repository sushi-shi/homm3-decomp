//! Native guard choices from installed assets, across faction and value inputs.

use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    placement_rules::PlacementRules,
    prototype::{GuardFactions, PrototypeSource},
    request::{MapVersion, Town},
    rng::RetailRng,
    template::{AllowedGuards, GuardAffinity},
    traits::CreatureCatalog,
};
use homm3_rmg_cli::resources::Installation;
use std::path::PathBuf;

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE guard checkpoints"]
fn guard_choices_counts_and_rng_match_cpp() {
    let directory = PathBuf::from(std::env::var_os("HOMM3_RMG_DATA").unwrap());
    let oracle = PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    let mut installation = Installation::open(&directory).unwrap();
    let mut bytes = Vec::new();
    installation.text("crtraits.txt", &mut bytes).unwrap();
    let creatures = CreatureCatalog::parse(&bytes).unwrap();
    installation.text("rand_trn.txt", &mut bytes).unwrap();
    let mut objects = Vec::new();
    installation.text("objects.txt", &mut objects).unwrap();
    let source = PrototypeSource::parse(&objects, |name| installation.mask(name)).unwrap();
    let mut checked = 0;
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
            let expected = std::fs::read_to_string(
                oracle.join(format!("{mode}-layout/case-{case}-candidate/guards.txt")),
            )
            .unwrap();
            assert_eq!(expected.lines().count(), 576);
            for line in expected.lines() {
                let mut fields = line
                    .split_whitespace()
                    .map(|field| field.parse::<i64>().unwrap());
                let values: [i64; 7] = std::array::from_fn(|_| fields.next().unwrap());
                assert!(fields.next().is_none());
                let policy = usize::try_from(values[0]).unwrap();
                // Policy 0 allows every affinity; 1..=10 allows only the
                // affinity in that column; 11 matches Rampart alignment.
                let factions = match policy {
                    0 => GuardFactions::Allowed(AllowedGuards::ALL),
                    11 => GuardFactions::Matching(Town::RAMPART),
                    _ => {
                        let mut allowed = AllowedGuards::NONE;
                        if let Some(&affinity) = GuardAffinity::ALL.get(policy - 1) {
                            allowed.insert(affinity);
                        }
                        GuardFactions::Allowed(allowed)
                    }
                };
                let mut rng = RetailRng::new(u32::try_from(values[2]).unwrap());
                let guard = catalog
                    .select_guard(
                        i32::try_from(values[1]).unwrap(),
                        factions,
                        &creatures,
                        &mut rng,
                    )
                    .unwrap();
                let actual = guard.map_or([-1; 3], |guard| {
                    [
                        i64::try_from(guard.creature().index()).unwrap(),
                        i64::from(guard.count()),
                        i64::try_from(
                            catalog
                                .get(guard.prototype())
                                .unwrap()
                                .prototype()
                                .source_row(),
                        )
                        .unwrap(),
                    ]
                });
                assert_eq!(actual, values[3..6], "{mode} {version:?}: {line}");
                assert_eq!(
                    i64::from(rng.state()),
                    values[6],
                    "{mode} {version:?}: {line}"
                );
                checked += 1;
            }
        }
    }
    assert_eq!(checked, 3456);
}
