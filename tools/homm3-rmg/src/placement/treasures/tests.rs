use super::*;
use crate::{
    behavior::{Behavior, RetailProfile},
    placement_rules::PlacementRules,
    prototype::{ImageMask, PrototypeSource},
    request::{MapVersion, Town},
    treasure::TreasureWorkspace,
};
use std::fmt::Write;

fn creatures(growth: i32) -> CreatureCatalog {
    let mut bytes = String::new();
    for _ in 0..185 {
        for column in 0..24 {
            if column != 0 {
                bytes.push('\t');
            }
            let value = match column {
                10 => 500,
                11 => growth,
                _ => 0,
            };
            write!(bytes, "{value}").unwrap();
        }
        bytes.push_str("\r\n");
    }
    CreatureCatalog::parse(bytes.as_bytes()).unwrap()
}
fn policies(creatures: &CreatureCatalog) -> (TreasureReward, TreasureReward, TreasureReward) {
    let mut bytes = String::from("3\r\n");
    for kind in [raw::BORDER_TENT, raw::SEER, raw::MONSTER] {
        writeln!(
            bytes,
            "art.def {:048b} {:048b} {:09b} {:09b} {kind} 0 0 0\r",
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
        Ok::<_, ()>(Some(ImageMask::parse(&mask).unwrap()))
    })
    .unwrap();
    let behavior = Behavior::Retail(RetailProfile::default());
    let rules = PlacementRules::parse(b"h\r\nh\r\nh\r\n", behavior).unwrap();
    let prototypes = source
        .prepare(&rules, MapVersion::ShadowOfDeath, behavior)
        .unwrap();
    let mut workspace = TreasureWorkspace::default();
    let catalog = workspace.prepare(&prototypes, creatures).unwrap();
    let rewards = || catalog.definitions().iter().map(|def| def.reward());
    (
        rewards().find(|reward| matches!(reward, TreasureReward::Creature(reward) if reward.creature().index() == 0)).unwrap(),
        rewards().find(|reward| matches!(reward, TreasureReward::QuestCreature { reward, .. } if reward.creature().index() == 0)).unwrap(),
        rewards().find(|reward| matches!(reward, TreasureReward::KeyTent { .. })).unwrap(),
    )
}

#[test]
fn quest_gates_precede_faction_value_and_keep_the_signed_base_transform() {
    let creatures = creatures(1);
    let (creature, quest, _) = policies(&creatures);
    let towns = TownZoneCounts::default();
    let mut state = QuestState::default();
    let value = |reward, faction, state: &QuestState| {
        reward_value(
            reward,
            faction,
            &creatures,
            &towns,
            KeyTentCursor::ReplayRequired,
            state,
        )
        .unwrap()
    };
    let castle = CreaturePreference::Faction(Town::parse(raw::TOWN_CASTLE).unwrap());
    assert_eq!(value(creature, castle, &state), 5000);
    assert_eq!(value(quest, castle, &state), 2000);
    assert_eq!(value(creature, CreaturePreference::Neutral, &state), -1);
    assert_eq!(value(quest, CreaturePreference::Neutral, &state), -1334);
    state.pool_low = true;
    assert_eq!(value(quest, castle, &state), -1);
    state.pool_low = false;
    state.next_seer = 1;
    assert_eq!(value(quest, CreaturePreference::Neutral, &state), -1);
}

#[test]
fn only_a_tent_value_needs_the_raw_tent_cursor() {
    let creatures = creatures(1);
    let (_, _, tent) = policies(&creatures);
    let towns = TownZoneCounts::default();
    let state = QuestState::default();
    let value = |reward, cursor| {
        reward_value(
            reward,
            CreaturePreference::UnmatchedRetail,
            &creatures,
            &towns,
            cursor,
            &state,
        )
    };
    assert_eq!(
        value(TreasureReward::Plain(7), KeyTentCursor::ReplayRequired).unwrap(),
        7
    );
    assert!(matches!(
        value(tent, KeyTentCursor::ReplayRequired),
        Err(PlacementError::KeyTentReplayRequired)
    ));
    assert_eq!(value(tent, KeyTentCursor::Value(0)).unwrap(), 5000);
    for cursor in [-1, 1, i32::MAX] {
        assert_eq!(value(tent, KeyTentCursor::Value(cursor)).unwrap(), -1);
    }
}

#[test]
fn dwelling_faction_gate_skips_overflow_and_signed_growth_is_retained() {
    let id = CreatureId::parse(0).unwrap();
    let towns = TownZoneCounts::default();
    let castle = CreaturePreference::Faction(Town::parse(raw::TOWN_CASTLE).unwrap());
    let overflowing = creatures(i32::MAX);
    assert_eq!(
        dwelling_value(
            id,
            CreaturePreference::UnmatchedRetail,
            &overflowing,
            &towns
        )
        .unwrap(),
        -1
    );
    assert!(matches!(
        dwelling_value(id, castle, &overflowing, &towns),
        Err(PlacementError::Arithmetic)
    ));
    assert_eq!(
        dwelling_value(id, castle, &creatures(-3), &towns).unwrap(),
        -1500
    );
}
