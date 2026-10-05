//! Typed treasure payloads stored beside their arena geometry.
use super::{MapObjectId, ObjectId};
use crate::{
    hero::HeroId,
    traits::ArtifactId,
    treasure::{CreatureReward, DefinitionId, SpellReward},
};

/// The one reward assigned by a Pandora definition; other native fields stay zero.
#[derive(Clone, Copy, Debug)]
pub enum PandoraReward {
    /// Experience points.
    Experience(i32),
    /// Gold, with every other resource zero.
    Gold(i32),
    /// One creature stack, retaining the native signed count.
    Creatures(CreatureReward),
    /// Level and school criteria; output enumerates by descending level, then spell ID.
    Spells(SpellReward),
}
/// The one reward assigned to a pending seer hut.
#[derive(Clone, Copy, Debug)]
pub enum SeerReward {
    /// Experience points.
    Experience(i32),
    /// Gold resource reward.
    Gold(i32),
    /// One creature stack.
    Creatures(CreatureReward),
}
/// A prison hero reservation and native serialized payload.
#[derive(Clone, Copy, Debug)]
pub struct PrisonPayload {
    pub(super) id: MapObjectId,
    pub(super) hero: HeroId,
    pub(super) experience: i32,
}
impl PrisonPayload {
    /// Counter shared with towns and monsters; discard never rewinds it.
    #[must_use]
    pub const fn id(self) -> MapObjectId {
        self.id
    }
    /// Hero claimed during generation.
    #[must_use]
    pub const fn hero(self) -> HeroId {
        self.hero
    }
    /// Source experience assigned to the hero.
    #[must_use]
    pub const fn experience(self) -> i32 {
        self.experience
    }
}
/// Seer reward and quest artifact, assigned only during placement completion.
#[derive(Clone, Copy, Debug)]
pub struct SeerPayload {
    pub(super) reward: SeerReward,
    pub(super) artifact: Option<ArtifactId>,
}
impl SeerPayload {
    /// Reward assigned by the definition.
    #[must_use]
    pub const fn reward(self) -> SeerReward {
        self.reward
    }
    /// Selected quest item; pending huts have none.
    #[must_use]
    pub const fn artifact(self) -> Option<ArtifactId> {
        self.artifact
    }
}
/// Quest artifact wrapper owning its pending hut until completion transfers it.
///
/// ```text
/// PendingTreasure --> QuestArtifact --owns--> unplaced Seer
///                           |
///                   completion transfers the Seer to the world,
///                   or cleanup destroys the pending child
/// ```
#[derive(Clone, Copy, Debug)]
pub struct QuestArtifactPayload {
    pub(super) definition: DefinitionId,
    pub(super) seer: Option<ObjectId>,
}
impl QuestArtifactPayload {
    /// Definition reevaluated if completion replaces this object.
    #[must_use]
    pub const fn definition(self) -> DefinitionId {
        self.definition
    }
    /// Child owned by this wrapper; absent after completion resolves ownership.
    #[must_use]
    pub const fn pending_seer(self) -> Option<ObjectId> {
        self.seer
    }
}
