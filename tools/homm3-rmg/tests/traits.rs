//! Independent initialized-retail-table comparison for native trait resources.

use homm3_rmg::traits::{CreatureCatalog, SpellCatalog};
use std::{fmt::Write, path::PathBuf};

#[test]
#[ignore = "requires HOMM3_RMG_CREATURES, HOMM3_RMG_SPELLS and HOMM3_RMG_ORACLE checkpoints"]
fn installed_traits_match_the_native_loaders() {
    let creatures = CreatureCatalog::parse(
        &std::fs::read(std::env::var_os("HOMM3_RMG_CREATURES").unwrap()).unwrap(),
    )
    .unwrap();
    let spells =
        SpellCatalog::parse(&std::fs::read(std::env::var_os("HOMM3_RMG_SPELLS").unwrap()).unwrap())
            .unwrap();
    let root = PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    for mode in ["retail", "hotfix"] {
        let directory = root.join(format!("{mode}-layout/case-0-candidate"));
        let mut actual = String::new();
        for (id, traits) in creatures.entries().iter().enumerate() {
            let town = traits
                .town()
                .map_or(-1, |town| i32::try_from(town.index()).unwrap());
            let tier = traits
                .tier()
                .map_or(-1, |tier| i32::try_from(tier.index()).unwrap());
            let (low, high) = traits.wandering_counts();
            writeln!(
                actual,
                "{id} {town} {tier} {} {} {low} {high}",
                traits.ai_value().map_or(0, std::num::NonZeroI32::get),
                traits.growth()
            )
            .unwrap();
        }
        let expected = std::fs::read_to_string(directory.join("creature-traits.txt"))
            .unwrap()
            .replace("\r\n", "\n");
        assert_eq!(actual, expected, "{mode} creature traits");
        actual.clear();
        for (id, traits) in spells.entries().iter().enumerate() {
            writeln!(
                actual,
                "{id} {} {} {}",
                traits.level(),
                traits.schools().bits(),
                u8::from(traits.disabled_by_default())
            )
            .unwrap();
        }
        let expected = std::fs::read_to_string(directory.join("spell-traits.txt"))
            .unwrap()
            .replace("\r\n", "\n");
        assert_eq!(actual, expected, "{mode} spell traits");
    }
}
