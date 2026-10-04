use super::*;
use crate::{behavior::Behavior, placement_rules::PlacementRules, prototype::PrototypeSource};
use std::fmt::Write;

fn creatures(first_ai: i32) -> CreatureCatalog {
    let mut text = String::new();
    for row in 0..185 {
        for column in 0..24 {
            if column > 0 {
                text.push('\t');
            }
            let value = if column == 10 {
                if row == 2 {
                    first_ai
                } else {
                    500
                }
            } else {
                0
            };
            write!(text, "{value}").unwrap();
        }
        text.push_str("\r\n");
    }
    CreatureCatalog::parse(text.as_bytes()).unwrap()
}
fn empty_source() -> PrototypeSource<'static> {
    PrototypeSource::parse(b"0\r\n", |_| Ok::<_, ()>(None)).unwrap()
}
fn rules() -> PlacementRules {
    PlacementRules::parse(b"header\r\nheader\r\nheader\r\n", Behavior::Hotfix).unwrap()
}

#[test]
fn creature_count_keeps_signed_division_and_strict_rounding_boundaries() {
    let id = CreatureId::parse(0).unwrap();
    for (ai, expected) in [
        (10_000, 0),
        (1000, 5),
        (833, 6),
        (714, 8),
        (416, 12),
        (384, 15),
        (100, 50),
        (98, 50),
        (90, 60),
        (-100, -50),
    ] {
        assert_eq!(
            CreatureReward::new(id, &creatures(ai)).unwrap().count(),
            expected,
            "AI {ai}"
        );
    }
}

#[test]
fn absent_art_does_not_hide_eager_constructor_faults() {
    let source = empty_source();
    let rules = rules();
    let prototypes = source
        .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
        .unwrap();
    let mut workspace = TreasureWorkspace::default();
    assert!(matches!(workspace.prepare(&prototypes, &creatures(0)),
        Err(TreasureError::CreatureCount(id)) if id.index() == 0));
    // A failed preparation can be retried with corrected input.
    let creatures = creatures(500);
    assert!(workspace.prepare(&prototypes, &creatures).is_ok());
}

#[test]
fn absent_art_retains_definitions_and_repeated_preparation_reuses_storage() {
    let source = empty_source();
    let rules = rules();
    let creatures = creatures(500);
    let mut workspace = TreasureWorkspace::default();
    for (version, last_creature, last_dwelling) in [
        (MapVersion::Restoration, 117, 57),
        (MapVersion::ArmageddonsBlade, 144, 79),
        (MapVersion::ShadowOfDeath, 144, 79),
    ] {
        let prototypes = source.prepare(&rules, version, Behavior::Hotfix).unwrap();
        let catalog = workspace.prepare(&prototypes, &creatures).unwrap();
        let definitions = catalog.definitions();
        assert!(prototypes.entries().is_empty());
        let TreasureReward::Creature(first) = definitions[2].reward() else {
            panic!("first creature block");
        };
        assert_eq!(first.creature().index(), last_creature);
        let dwellings: Vec<_> = definitions
            .iter()
            .filter(|def| matches!(def.reward(), TreasureReward::Dwelling(_)))
            .map(|def| def.subtype())
            .collect();
        assert_eq!(dwellings.first(), Some(&last_dwelling));
        assert_eq!(dwellings.last(), Some(&0));
        assert!(!definitions
            .iter()
            .any(|def| def.reward().terrain_dependent()));
        let address = definitions.as_ptr();
        let count = definitions.len();
        let capacity = workspace.definitions.capacity();
        let repeated = workspace.prepare(&prototypes, &creatures).unwrap();
        assert_eq!(repeated.definitions().as_ptr(), address);
        assert_eq!(repeated.definitions().len(), count);
        assert_eq!(workspace.definitions.capacity(), capacity);
    }
}

#[test]
fn tent_and_seer_ordinals_are_independent_of_stored_subtypes() {
    let behavior = Behavior::Retail(crate::behavior::RetailProfile::default());
    let rules = PlacementRules::parse(b"header\r\nheader\r\nheader\r\n", behavior).unwrap();
    let mut bytes = String::from("5\r\n");
    for (kind, subtype) in [
        (raw::BORDER_TENT, 4),
        (raw::SEER, 7),
        (raw::BORDER_TENT, 2),
        (raw::SEER, 3),
        (raw::MONSTER, 0),
    ] {
        writeln!(
            bytes,
            "art.def {:048b} {:048b} {:09b} {:09b} {kind} {subtype} 0 0\r",
            0_u64,
            1_u64 << 47,
            1,
            1
        )
        .unwrap();
    }
    let mut mask = [0; 14];
    mask[0] = 1;
    mask[1] = 1;
    let source = PrototypeSource::parse(bytes.as_bytes(), |_| {
        Ok::<_, ()>(Some(crate::prototype::ImageMask::parse(&mask).unwrap()))
    })
    .unwrap();
    let prototypes = source
        .prepare(&rules, MapVersion::ShadowOfDeath, behavior)
        .unwrap();
    let creatures = creatures(500);
    let mut workspace = TreasureWorkspace::default();
    let catalog = workspace.prepare(&prototypes, &creatures).unwrap();
    let tents: Vec<_> = catalog
        .definitions()
        .iter()
        .filter_map(|def| match def.reward() {
            TreasureReward::KeyTent { ordinal, value } => Some((ordinal.value(), value)),
            _ => None,
        })
        .collect();
    assert_eq!(
        tents,
        [
            (1, 5000),
            (1, 7500),
            (1, 10000),
            (1, 15000),
            (1, 20000),
            (0, 5000),
            (0, 7500),
            (0, 10000),
            (0, 15000),
            (0, 20000)
        ]
    );
    let quests: Vec<_> = catalog
        .definitions()
        .iter()
        .filter(|def| matches!(def.reward(), TreasureReward::QuestCreature { .. }))
        .collect();
    assert_eq!(quests.first().unwrap().subtype(), 0);
    assert_eq!(quests.last().unwrap().subtype(), 1);
    assert!(quests.iter().all(|def| def.density().get() == 3));
}

#[test]
fn definition_ids_cannot_cross_catalogs_or_survive_repreparation() {
    let source = empty_source();
    let rules = rules();
    let prototypes = source
        .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
        .unwrap();
    let creatures = creatures(500);
    let mut first = TreasureWorkspace::default();
    let mut second = TreasureWorkspace::default();
    let first_catalog = first.prepare(&prototypes, &creatures).unwrap();
    let first_id = first_catalog.ids().next().unwrap();
    assert!(first_catalog.get(first_id).is_some());
    let second_catalog = second.prepare(&prototypes, &creatures).unwrap();
    assert!(second_catalog.get(first_id).is_none());
    let second_id = second_catalog.ids().next().unwrap();
    assert_eq!(first_id.index(), second_id.index());
    assert!(first_catalog.get(second_id).is_none());
    let replacement = first.prepare(&prototypes, &creatures).unwrap();
    assert!(replacement.get(first_id).is_none());
}
