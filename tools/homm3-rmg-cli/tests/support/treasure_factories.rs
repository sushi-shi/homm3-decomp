//! Factory checkpoints use actual payloads, then explicit ordered cleanup.
use homm3_rmg::{
    hero::HeroId,
    placement::{
        ObjectArena, ObjectId, ObjectPayload, PandoraReward, PlacementError, SeerReward,
        TreasureGeneration, TreasuresReady,
    },
    raw,
    rng::RetailRng,
    traits::{ArtifactCatalog, SpellCatalog},
    treasure::TreasureReward,
};
use homm3_rmg_cli::resources::Installation;
use std::{fmt::Write, path::PathBuf};

pub fn snapshot(
    ready: TreasuresReady<'_, '_, '_, '_, '_, '_>,
    objects: &mut ObjectArena,
    rng: &RetailRng,
) -> String {
    let mut installation =
        Installation::open(&PathBuf::from(std::env::var_os("HOMM3_RMG_DATA").unwrap())).unwrap();
    let mut bytes = Vec::new();
    installation.text("sptraits.txt", &mut bytes).unwrap();
    let spells = SpellCatalog::parse(&bytes).unwrap();
    installation.text("artraits.txt", &mut bytes).unwrap();
    let artifacts = ArtifactCatalog::parse(&bytes).unwrap();
    let mut generation = ready
        .begin_generation(objects, &spells, &artifacts)
        .unwrap();
    let mut rng = rng.clone();
    let zone = generation.ready().map().coverage().map().zones()[0].id();
    let mut text = format!(
        "factories {}\n",
        generation.ready().catalog().definitions().len()
    );
    let definitions: Vec<_> = generation.ready().catalog().ids().collect();
    let mut coverage = [0; 17];
    for id in definitions {
        let tag = tag(generation.ready().catalog().get(id).unwrap().reward());
        let selected = generation.choose_prototype(id, zone, &mut rng).unwrap();
        let row = selected.map_or(-1, |selected| row(&generation, selected.prototype()));
        writeln!(
            text,
            "select {} {tag} {row} {} {}",
            id.index(),
            rng.state(),
            generation.ready().map().next_object_id()
        )
        .unwrap();
        let Some(selected) = selected else {
            continue;
        };
        let checkpoint = rng.checkpoint();
        assert!(matches!(
            generation.generate(selected, &mut ObjectArena::default(), &mut rng),
            Err(homm3_rmg::placement::TreasureGenerationError::Placement(
                PlacementError::ArenaContext
            ))
        ));
        assert_eq!(rng.checkpoint(), checkpoint);
        let generated = generation.generate(selected, objects, &mut rng).unwrap();
        let mut child = None;
        if let Some(pending) = &generated {
            coverage[tag] += 1;
            child = write_payload(&mut text, &generation, objects, pending.object());
            if matches!(
                objects.payload(pending.object()),
                Some(ObjectPayload::Prison(_) | ObjectPayload::QuestArtifact(_))
            ) {
                assert!(matches!(
                    objects.discard_unplaced(pending.object()),
                    Err(PlacementError::TreasureCleanupRequired(_))
                ));
            }
            if let Some(child) = child {
                assert!(matches!(
                    objects.discard_unplaced(child),
                    Err(PlacementError::TreasureCleanupRequired(_))
                ));
            }
        } else {
            text.push_str("null\n");
        }
        state(&mut text, "state", &generation, &rng);
        text.push('\n');
        if let Some(pending) = generated {
            let object = pending.object();
            generation.discard(pending, objects).unwrap();
            assert!(objects.get(object).is_none());
            assert!(matches!(
                generation.pandora_spells(objects, object),
                Err(homm3_rmg::placement::TreasureGenerationError::Placement(
                    PlacementError::UnknownObject(_)
                ))
            ));
            assert!(child.is_none_or(|child| objects.get(child).is_none()));
        }
        state(&mut text, "cleanup", &generation, &rng);
        text.push_str(" 0\n");
    }
    check_prison_exhaustion(&mut generation, objects, &mut rng);
    eprintln!("generated per native factory policy: {coverage:?}");
    assert!(coverage.iter().all(|&count| count > 0));
    text
}
fn row(
    generation: &TreasureGeneration<'_, '_, '_, '_, '_, '_, '_>,
    prototype: homm3_rmg::prototype::PrototypeId,
) -> i32 {
    i32::try_from(
        generation
            .ready()
            .catalog()
            .prototypes()
            .get(prototype)
            .unwrap()
            .prototype()
            .source_row(),
    )
    .unwrap()
}
fn state(
    text: &mut String,
    label: &str,
    generation: &TreasureGeneration<'_, '_, '_, '_, '_, '_, '_>,
    rng: &RetailRng,
) {
    write!(
        text,
        "{label} {} {} ",
        rng.state(),
        generation.ready().map().next_object_id()
    )
    .unwrap();
    for hero in 0..i32::try_from(raw::RMG_HERO_COUNT).unwrap() {
        text.push(if generation.hero_disabled(HeroId::parse(hero).unwrap()) {
            '1'
        } else {
            '0'
        });
    }
}
fn write_payload(
    text: &mut String,
    generation: &TreasureGeneration<'_, '_, '_, '_, '_, '_, '_>,
    objects: &ObjectArena,
    id: ObjectId,
) -> Option<ObjectId> {
    let geometry = objects.get(id).unwrap();
    assert!(geometry.position().is_none());
    writeln!(
        text,
        "object {} -1 -1 -1",
        row(generation, geometry.prototype())
    )
    .unwrap();
    match *objects.payload(id).unwrap() {
        ObjectPayload::Pandora(reward) => {
            let (experience, gold, creature, count) = match reward {
                PandoraReward::Experience(value) => (value, 0, -1, 0),
                PandoraReward::Gold(value) => (0, value, -1, 0),
                PandoraReward::Creatures(reward) => (
                    0,
                    0,
                    i32::try_from(reward.creature().index()).unwrap(),
                    reward.count(),
                ),
                PandoraReward::Spells(_) => (0, 0, -1, 0),
            };
            write!(text, "pandora {experience} {creature} {count}").unwrap();
            for resource in homm3_rmg::domain::Resource::ALL {
                write!(
                    text,
                    " {}",
                    if resource == homm3_rmg::domain::Resource::Gold {
                        gold
                    } else {
                        0
                    }
                )
                .unwrap();
            }
            if matches!(reward, PandoraReward::Spells(_)) {
                write!(
                    text,
                    " {}",
                    generation.pandora_spells(objects, id).unwrap().count()
                )
                .unwrap();
                for spell in generation.pandora_spells(objects, id).unwrap() {
                    write!(text, " {}", spell.index()).unwrap();
                }
            } else {
                text.push_str(" 0");
            }
            text.push('\n');
        }
        ObjectPayload::KeyTent(value) => writeln!(text, "tent {value}").unwrap(),
        ObjectPayload::Prison(payload) => writeln!(
            text,
            "prison {} {} {}",
            payload.id().value(),
            payload.hero().index(),
            payload.experience()
        )
        .unwrap(),
        ObjectPayload::QuestArtifact(payload) => {
            let child = payload.pending_seer().unwrap();
            let ObjectPayload::Seer(seer) = *objects.payload(child).unwrap() else {
                panic!("quest child");
            };
            assert_eq!(seer.artifact(), None);
            assert_eq!(objects.get(child).unwrap().position(), None);
            let (experience, gold, creature, count) = match seer.reward() {
                SeerReward::Experience(value) => (value, 0, -1, 0),
                SeerReward::Gold(value) => (0, value, -1, 0),
                SeerReward::Creatures(reward) => (
                    0,
                    0,
                    i32::try_from(reward.creature().index()).unwrap(),
                    reward.count(),
                ),
            };
            writeln!(
                text,
                "quest {} {} -1 {experience} {} {gold} {creature} {count}",
                payload.definition().index(),
                row(generation, objects.get(child).unwrap().prototype()),
                raw::GOLD
            )
            .unwrap();
            return Some(child);
        }
        ObjectPayload::Scroll(spell) => writeln!(text, "scroll {}", spell.index()).unwrap(),
        ObjectPayload::Base
        | ObjectPayload::Artifact
        | ObjectPayload::Ownable
        | ObjectPayload::Resource
        | ObjectPayload::Scholar
        | ObjectPayload::Shrine
        | ObjectPayload::WitchHut => {}
        other => panic!("unexpected factory payload {other:?}"),
    }
    None
}
fn tag(reward: TreasureReward) -> usize {
    match reward {
        TreasureReward::Plain(_) => 0,
        TreasureReward::Artifact(_) => 1,
        TreasureReward::Creature(_) => 2,
        TreasureReward::Experience { .. } => 3,
        TreasureReward::Gold { .. } => 4,
        TreasureReward::Spells { .. } => 5,
        TreasureReward::KeyTent { .. } => 6,
        TreasureReward::Dwelling(_) => 7,
        TreasureReward::Prison { .. } => 8,
        TreasureReward::Resource(_) => 9,
        TreasureReward::Scholar => 10,
        TreasureReward::Shrine(_) => 11,
        TreasureReward::QuestCreature { .. } => 12,
        TreasureReward::QuestExperience { .. } => 13,
        TreasureReward::QuestGold { .. } => 14,
        TreasureReward::Scroll { .. } => 15,
        TreasureReward::WitchHut => 16,
    }
}

fn check_prison_exhaustion(
    generation: &mut TreasureGeneration<'_, '_, '_, '_, '_, '_, '_>,
    objects: &mut ObjectArena,
    rng: &mut RetailRng,
) {
    let definition = generation
        .ready()
        .catalog()
        .ids()
        .find(|&id| {
            matches!(
                generation.ready().catalog().get(id).unwrap().reward(),
                TreasureReward::Prison { .. }
            )
        })
        .unwrap();
    let zone = generation.ready().map().coverage().map().zones()[0].id();
    let selected = generation
        .choose_prototype(definition, zone, rng)
        .unwrap()
        .unwrap();
    let first_id = generation.ready().map().next_object_id();
    let mut held = Vec::new();
    loop {
        let before = rng.checkpoint();
        let id = generation.ready().map().next_object_id();
        let Some(pending) = generation.generate(selected, objects, rng).unwrap() else {
            assert_eq!(rng.checkpoint(), before);
            assert_eq!(generation.ready().map().next_object_id(), id);
            break;
        };
        let ObjectPayload::Prison(payload) = *objects.payload(pending.object()).unwrap() else {
            panic!("prison payload");
        };
        assert!(generation.hero_disabled(payload.hero()));
        assert_eq!(
            payload.id().value(),
            first_id + i32::try_from(held.len()).unwrap()
        );
        held.push((pending, payload.hero()));
        assert!(held.len() <= raw::RMG_HERO_COUNT as usize);
    }
    assert!(!held.is_empty());
    let after = rng.checkpoint();
    let next_id = generation.ready().map().next_object_id();
    for (pending, hero) in held {
        let id = pending.object();
        generation.discard(pending, objects).unwrap();
        assert!(!generation.hero_disabled(hero));
        assert!(objects.get(id).is_none());
    }
    assert_eq!(rng.checkpoint(), after);
    assert_eq!(generation.ready().map().next_object_id(), next_id);
}
