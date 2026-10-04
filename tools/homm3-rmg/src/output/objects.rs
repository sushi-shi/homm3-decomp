//! Serialize by payload class, independently of the object's prototype family.
use super::{expansion, OutputFault, Writer};
use crate::{
    generation::GeneratedMap,
    placement::{Fort, ObjectPayload, PandoraReward, PositionedObject, SeerReward},
    raw,
    request::MapVersion,
    treasure::CreatureReward,
};
use std::io::Write;
#[expect(
    clippy::too_many_lines,
    reason = "one exhaustive payload dispatch makes every wire layout visible without allocation or extra state"
)]
pub(super) fn write(
    map: &GeneratedMap<'_>,
    object: PositionedObject<'_>,
    slot: u32,
    out: &mut Writer<'_, impl Write>,
) -> Result<(), OutputFault> {
    let payload = *object.payload();
    let version = map.report().request().version();
    out.position(object.position())?;
    out.u32(slot)?;
    out.zero(5)?;
    match payload {
        ObjectPayload::Base | ObjectPayload::KeyTent(_) => Ok(()),
        ObjectPayload::Artifact | ObjectPayload::QuestArtifact(_) => out.byte(0),
        ObjectPayload::Ownable | ObjectPayload::Shrine => out.bytes(&[255, 0, 0, 0]),
        ObjectPayload::Resource => out.zero(9),
        ObjectPayload::Scholar => {
            out.byte(255)?;
            out.zero(7)
        }
        ObjectPayload::Scroll(spell) => {
            out.bytes(&[0, spell.index() as u8])?;
            out.zero(3)
        }
        ObjectPayload::WitchHut => {
            if expansion(version) {
                out.u32(raw::RMG_WITCH_HUT_ALLOWED_SKILLS)?;
            }
            Ok(())
        }
        ObjectPayload::Monster(monster) => {
            if expansion(version) {
                out.i32(monster.id().value())?;
            }
            out.u16(monster.count() as u16)?;
            out.byte(monster.disposition() as u8)?;
            out.zero(5)
        }
        ObjectPayload::Town(town) => {
            if expansion(version) {
                out.i32(town.id().value())?;
            }
            out.byte(town.owner().map_or(255, |p| p.index() as u8))?;
            out.zero(4)?;
            out.byte(u8::from(town.fort() == Fort::Present))?;
            let spells = (raw::HERO_SPELL_COUNT as usize).div_ceil(8);
            if expansion(version) {
                out.zero(spells)?;
            }
            out.zero(spells)?;
            out.u32(0)?;
            if version == MapVersion::ShadowOfDeath {
                out.byte(255)?;
            }
            out.zero(3)
        }
        ObjectPayload::Prison(hero) => {
            if expansion(version) {
                out.i32(hero.id().value())?;
            }
            out.bytes(&[255, hero.hero().index() as u8, 0])?;
            if version == MapVersion::ShadowOfDeath {
                out.byte(u8::from(hero.experience() != 0))?;
            }
            if version != MapVersion::ShadowOfDeath || hero.experience() != 0 {
                out.i32(hero.experience())?;
            }
            out.zero(5)?;
            out.byte(255)?;
            if expansion(version) {
                out.bytes(&[0, 255])?;
                if version == MapVersion::ShadowOfDeath {
                    out.zero(2)?;
                } else {
                    out.byte(254)?;
                }
            }
            out.zero(16)
        }
        ObjectPayload::Seer(seer) => {
            let artifact = seer
                .artifact()
                .map_or(-1, |a| i32::try_from(a.index()).unwrap());
            if expansion(version) {
                out.bytes(&[raw::QUEST_ARTIFACTS as u8, 1])?;
                out.u16(artifact as u16)?;
                out.i32(-1)?;
                out.zero(12)?;
            } else {
                out.byte(artifact as u8)?;
            }
            match seer.reward() {
                SeerReward::Experience(xp) if xp > 0 => {
                    out.byte(raw::eRewardExperience as u8)?;
                    out.i32(xp)?;
                }
                SeerReward::Creatures(stack) => {
                    out.byte(raw::eRewardCreature as u8)?;
                    creature(out, version, stack)?;
                }
                reward => {
                    out.bytes(&[raw::eRewardResource as u8, raw::GOLD as u8])?;
                    out.i32(if let SeerReward::Gold(gold) = reward {
                        gold
                    } else {
                        0
                    })?;
                }
            }
            out.zero(2)
        }
        ObjectPayload::Pandora(reward) => {
            out.byte(0)?;
            out.i32(if let PandoraReward::Experience(xp) = reward {
                xp
            } else {
                0
            })?;
            out.zero(6)?; // mana, morale, luck
            for resource in 0..raw::NUM_RESOURCES {
                let gold = if let PandoraReward::Gold(gold) = reward {
                    gold
                } else {
                    0
                };
                out.i32(if resource == raw::GOLD as u32 {
                    gold
                } else {
                    0
                })?;
            }
            out.zero(6)?; // primary skills, secondary count, artifact count
            if let PandoraReward::Spells(spells) = reward {
                let count = map.treasures().pandora_spells(spells).count();
                out.byte(u8::try_from(count).map_err(|_| OutputFault::Count)?)?;
                for spell in map.treasures().pandora_spells(spells) {
                    out.byte(spell.index() as u8)?;
                }
            } else {
                out.byte(0)?;
            }
            if let PandoraReward::Creatures(stack) = reward {
                out.byte(1)?;
                creature(out, version, stack)?;
            } else {
                out.byte(0)?;
            }
            out.zero(8)
        }
    }
}
fn creature(
    out: &mut Writer<'_, impl Write>,
    version: MapVersion,
    stack: CreatureReward,
) -> Result<(), OutputFault> {
    if expansion(version) {
        out.u16(stack.creature().index() as u16)?;
    } else {
        out.byte(stack.creature().index() as u8)?;
    }
    out.u16(stack.count() as u16)
}
