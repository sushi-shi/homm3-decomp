//! Independent initialized-retail-table comparison for native trait resources.

use homm3_rmg::{
    hero::{HeroId, HeroPool},
    raw,
    request::MapVersion,
    rng::RetailRng,
    traits::{ArtifactCatalog, CreatureCatalog, SpellCatalog},
};
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

#[test]
#[ignore = "requires HOMM3_RMG_ARTIFACTS and HOMM3_RMG_ORACLE trait/pool checkpoints for three formats"]
fn installed_artifacts_and_hero_selection_match_native() {
    let artifacts = ArtifactCatalog::parse(
        &std::fs::read(std::env::var_os("HOMM3_RMG_ARTIFACTS").unwrap()).unwrap(),
    )
    .unwrap();
    let root = PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    for mode in ["retail", "hotfix"] {
        for (case, version) in [
            MapVersion::ShadowOfDeath,
            MapVersion::ArmageddonsBlade,
            MapVersion::Restoration,
        ]
        .into_iter()
        .enumerate()
        {
            let directory = root.join(format!("{mode}-layout/case-{case}-candidate"));
            let mut actual = String::new();
            for (id, traits) in artifacts.entries().iter().enumerate() {
                writeln!(
                    actual,
                    "{id} {} {} {}",
                    traits.class() as u32,
                    u8::from(traits.disabled()),
                    traits
                        .combination()
                        .map_or(-1, |id| i32::try_from(id.index()).unwrap())
                )
                .unwrap();
            }
            assert_eq!(
                actual,
                std::fs::read_to_string(directory.join("artifact-traits.txt"))
                    .unwrap()
                    .replace("\r\n", "\n")
            );
            let hero_traits = std::fs::read_to_string(directory.join("hero-traits.txt")).unwrap();
            assert_eq!(hero_traits.lines().count(), raw::RMG_HERO_COUNT as usize);
            for line in hero_traits.lines() {
                let fields: Vec<i32> = line
                    .split_whitespace()
                    .map(|n| n.parse().unwrap())
                    .collect();
                let hero = HeroId::parse(fields[0]).unwrap();
                let available = fields[3] == 0
                    && fields[if version == MapVersion::Restoration {
                        1
                    } else {
                        2
                    }] != 0;
                assert_eq!(hero.available(version), available);
            }
            let mut pool = HeroPool::new(version);
            actual.clear();
            for id in 0..i32::try_from(raw::RMG_HERO_COUNT).unwrap() {
                write!(
                    actual,
                    "{} ",
                    u8::from(pool.is_disabled(HeroId::parse(id).unwrap()))
                )
                .unwrap();
            }
            actual.push('\n');
            let mut rng = RetailRng::new(1);
            while let Some(hero) = pool.select_prison(&mut rng) {
                write!(actual, "{} ", hero.index()).unwrap();
            }
            writeln!(actual, "-1 \n{}", rng.state()).unwrap();
            assert_eq!(
                actual,
                std::fs::read_to_string(directory.join("hero-pool.txt"))
                    .unwrap()
                    .replace("\r\n", "\n"),
                "{mode} {version:?}"
            );
        }
    }
}
