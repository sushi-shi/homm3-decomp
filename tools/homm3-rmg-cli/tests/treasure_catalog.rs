//! Compare prepared definitions against native constructor checkpoints.
use homm3_rmg::{
    behavior::{Behavior, RetailProfile},
    placement_rules::PlacementRules,
    prototype::PrototypeSource,
    request::MapVersion,
    traits::CreatureCatalog,
    treasure::{TreasureDefinition, TreasureReward, TreasureWorkspace},
};
use homm3_rmg_cli::resources::Installation;
use std::{fmt::Write, path::PathBuf};

fn snapshot(definitions: &[TreasureDefinition]) -> String {
    use TreasureReward as T;
    let mut text = String::new();
    for &def in definitions {
        let (tag, args) = match def.reward() {
            T::Plain(_) => ("plain", [0; 3]),
            T::Artifact(_) => ("artifact", [0; 3]),
            T::Creature(reward) | T::QuestCreature { reward, .. } => (
                if matches!(def.reward(), T::Creature(_)) {
                    "creature"
                } else {
                    "quest-creature"
                },
                [
                    i32::try_from(reward.creature().index()).unwrap(),
                    reward.count(),
                    0,
                ],
            ),
            T::Experience { amount, .. } => ("experience", [amount, 0, 0]),
            T::Gold { amount, .. } => ("gold", [amount, 0, 0]),
            T::Spells { spells, .. } => (
                "spells",
                [
                    spells.levels().0,
                    spells.levels().1,
                    i32::try_from(spells.schools().bits()).unwrap(),
                ],
            ),
            T::KeyTent { .. } => ("tent", [0; 3]),
            T::Dwelling => ("dwelling", [0; 3]),
            T::Prison { experience, .. } => ("prison", [experience, 0, 0]),
            T::Resource(_) => ("resource", [0; 3]),
            T::Scholar => ("scholar", [0; 3]),
            T::Shrine(_) => ("shrine", [0; 3]),
            T::QuestExperience { amount, .. } => ("quest-experience", [amount, 0, 0]),
            T::QuestGold { amount, .. } => ("quest-gold", [amount, 0, 0]),
            T::Scroll { level, .. } => ("scroll", [i32::try_from(level.get()).unwrap(), 0, 0]),
            T::WitchHut => ("witch", [0; 3]),
        };
        writeln!(
            text,
            "{tag} {} {} {} {} {} {} {}",
            def.kind().index(),
            def.subtype(),
            def.reward().fixed_value().unwrap_or(-1),
            def.density(),
            args[0],
            args[1],
            args[2]
        )
        .unwrap();
    }
    text
}

#[test]
#[ignore = "requires HOMM3_RMG_DATA and HOMM3_RMG_ORACLE constructor captures"]
fn native_treasure_definitions() {
    let directory = PathBuf::from(std::env::var_os("HOMM3_RMG_DATA").unwrap());
    let oracle = PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    let mut installation = Installation::open(&directory).unwrap();
    let mut objects = Vec::new();
    installation.text("objects.txt", &mut objects).unwrap();
    let source = PrototypeSource::parse(&objects, |name| installation.mask(name)).unwrap();
    let mut bytes = Vec::new();
    installation.text("crtraits.txt", &mut bytes).unwrap();
    let creatures = CreatureCatalog::parse(&bytes).unwrap();
    installation.text("rand_trn.txt", &mut bytes).unwrap();
    let mut workspace = TreasureWorkspace::default();
    for (mode, behavior) in [
        ("retail", Behavior::Retail(RetailProfile::default())),
        ("hotfix", Behavior::Hotfix),
    ] {
        let rules = PlacementRules::parse(&bytes, behavior).unwrap();
        for (case, version) in [
            MapVersion::ShadowOfDeath,
            MapVersion::ArmageddonsBlade,
            MapVersion::Restoration,
            MapVersion::ShadowOfDeath,
        ]
        .into_iter()
        .enumerate()
        {
            let prototypes = source.prepare(&rules, version, behavior).unwrap();
            let catalog = workspace.prepare(&prototypes, &creatures).unwrap();
            let actual = snapshot(catalog.definitions());
            for suffix in ["candidate", "repeat"] {
                let expected = std::fs::read_to_string(oracle.join(format!(
                    "{mode}-layout/case-{case}-{suffix}/treasure-definitions.txt"
                )))
                .unwrap();
                assert_eq!(
                    actual.lines().count(),
                    expected.lines().count(),
                    "{mode} {case} count"
                );
                for (line, (actual, expected)) in actual.lines().zip(expected.lines()).enumerate() {
                    assert_eq!(actual, expected, "{mode} {case} {suffix} line {line}");
                }
            }
            eprintln!(
                "{mode} {version:?}: {} definitions match candidate and repeat",
                catalog.definitions().len()
            );
        }
    }
}
