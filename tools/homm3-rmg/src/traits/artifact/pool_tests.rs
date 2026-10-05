//! Per-map artifact rules, with explicit traits to isolate native rule interactions.
use super::*;
use crate::{
    artifact::{ArtifactPool, ArtifactPoolError},
    request::Water,
    rng::RetailRng,
    template::{Availability, MapOptions},
};

fn catalog(rules: Ruleset) -> ArtifactCatalog {
    ArtifactCatalog {
        entries: vec![
            ArtifactTraits {
                class: ArtifactClass::Special,
                disabled: false,
                combination: None,
                component_of: None,
            };
            rules.artifact_count()
        ]
        .into_boxed_slice(),
        rules,
    }
}
fn options(artifacts: Option<&[u8]>, combinations: Option<&[u8]>) -> MapOptions {
    MapOptions {
        artifacts: artifacts.map(<[u8]>::to_vec),
        combination_artifacts: combinations.map(<[u8]>::to_vec),
        ..MapOptions::default()
    }
}

#[test]
fn constructor_water_bans_and_template_overrides_keep_trait_gates() {
    let mut catalog = catalog(Ruleset::HotA181);
    for id in [8, 9, 71, 83, 129, 165] {
        catalog.entries[id].class = ArtifactClass::Treasure;
    }
    catalog.entries[9].disabled = true;
    catalog.entries[129].combination = Some(CombinationId(0));
    for water in [Water::None, Water::Normal, Water::Islands] {
        let mut pool = ArtifactPool::new(&catalog, water);
        assert_eq!(
            pool.is_excluded(catalog.id(71).unwrap()),
            water == Water::None
        );
        assert!(pool.is_excluded(catalog.id(129).unwrap()));
        assert!(pool.combination_bans()[1]);
        pool.apply_template(&options(
            Some(b"+0 +9 +71 +83 +129 +165 -8"),
            Some(b"-0 +1"),
        ))
        .unwrap();
        for id in [71, 83, 129, 165] {
            assert!(!pool.is_excluded(catalog.id(id).unwrap()));
        }
        assert!(pool.is_excluded(catalog.id(8).unwrap()));
        assert_eq!(pool.overrides()[0], Availability::Inherit); // Non-random class.
        assert_eq!(pool.overrides()[9], Availability::Inherit); // Disabled trait.
        assert_eq!(pool.overrides()[129], Availability::Enabled);
        assert!(pool.combination_bans()[0]);
        assert!(!pool.combination_bans()[1]);
        let mut rng = RetailRng::new(1);
        let selection = pool.select_quest(&mut rng);
        assert_eq!(selection.pool, 4);
        assert_eq!(selection.candidates, 3); // Assembled artifact still cannot serve a quest.
    }
}

#[test]
fn low_pool_counts_permitted_components_before_the_candidate_filter() {
    let mut catalog = catalog(Ruleset::HotA181);
    for row in &mut catalog.entries[8..28] {
        row.class = ArtifactClass::Treasure;
        row.component_of = Some(CombinationId(0));
    }
    catalog.entries[152].class = ArtifactClass::Treasure;
    let mut pool = ArtifactPool::new(&catalog, Water::None);
    let mut rng = RetailRng::new(1);
    let chosen = pool.select_quest(&mut rng);
    assert_eq!((chosen.pool, chosen.candidates), (21, 1));
    assert!(!chosen.pool_low());
    assert_eq!(chosen.artifact, catalog.id(152));
    assert_eq!(rng.draws(), 1); // Unlike prison selection, a singleton draws.
    assert!(!pool.is_excluded(chosen.artifact.unwrap())); // Selection is not a successful placement.
    assert!(pool.exclude(chosen.artifact.unwrap()));
    assert!(!pool.exclude(chosen.artifact.unwrap()));
    let checkpoint = rng.checkpoint();
    let empty = pool.select_quest(&mut rng);
    assert_eq!((empty.pool, empty.candidates), (20, 0));
    assert!(!empty.pool_low());
    assert!(empty.artifact.is_none());
    assert_eq!(rng.checkpoint(), checkpoint);
    pool.apply_template(&options(None, Some(b"-0"))).unwrap();
    let unlocked = pool.select_quest(&mut rng);
    assert_eq!((unlocked.pool, unlocked.candidates), (20, 20));
    pool.exclude(unlocked.artifact.unwrap());
    assert!(pool.select_quest(&mut rng).pool_low());
}

#[test]
fn no_artifacts_fallback_changes_only_the_override_and_invalid_lists_are_atomic() {
    let mut catalog = catalog(Ruleset::HotA181);
    for id in [8, 71] {
        catalog.entries[id].class = ArtifactClass::Treasure;
    }
    let mut pool = ArtifactPool::new(&catalog, Water::None);
    pool.apply_template(&options(Some(b"-8 -71"), None))
        .unwrap();
    assert_eq!(pool.overrides()[8], Availability::Inherit);
    assert_eq!(pool.overrides()[71], Availability::Disabled);
    assert!(pool.is_excluded(catalog.id(8).unwrap()));
    let snapshot = pool.clone();
    assert_eq!(
        pool.apply_template(&options(Some(b"+71"), Some(b"+2147483648"))),
        Err(ArtifactPoolError::IdOverflow("combination artifacts"))
    );
    assert_eq!(pool.exclusions(), snapshot.exclusions());
    assert_eq!(pool.overrides(), snapshot.overrides());
    assert_eq!(pool.combination_bans(), snapshot.combination_bans());
    assert_eq!(
        pool.combination_overrides(),
        snapshot.combination_overrides()
    );
}

#[test]
fn complete_keeps_combination_components_and_claims_do_not_cross_generations() {
    let mut catalog = catalog(Ruleset::Complete);
    for id in [8, 40, 129] {
        catalog.entries[id].class = ArtifactClass::Treasure;
    }
    catalog.entries[8].component_of = Some(CombinationId(0));
    catalog.entries[129].combination = Some(CombinationId(0));
    let mut first = ArtifactPool::new(&catalog, Water::None);
    let second = ArtifactPool::new(&catalog, Water::Islands);
    let mut rng = RetailRng::new(1);
    let selected = first.select_quest(&mut rng);
    assert_eq!(selected.artifact, catalog.id(129)); // 41 % 3 = 2, ascending.
    assert_eq!((selected.pool, selected.candidates), (3, 3));
    first.exclude(selected.artifact.unwrap());
    assert!(!second.is_excluded(selected.artifact.unwrap()));
    first.exclude(catalog.id(40).unwrap());
    assert_eq!(first.select_quest(&mut rng).artifact, catalog.id(8));
    assert_eq!(rng.draws(), 2);
    first.exclude(catalog.id(8).unwrap());
    let checkpoint = rng.checkpoint();
    assert!(first.select_quest(&mut rng).artifact.is_none());
    assert_eq!(rng.checkpoint(), checkpoint);
    assert_eq!(
        first.apply_template(&MapOptions::default()),
        Err(ArtifactPoolError::UnsupportedRuleset(Ruleset::Complete))
    );
}
