//! Source-ordered payload factories and explicit pending-object cleanup.
use super::{
    ObjectArena, ObjectId, ObjectPayload, PandoraReward, PlacementError, PrisonPayload,
    QuestArtifactPayload, SeerPayload, SeerReward, TreasuresReady,
};
use crate::{
    domain::Terrain,
    geometry::ZoneId,
    hero::{HeroId, HeroPool},
    identity::OwnerId,
    object::ObjectKind,
    prototype::{PrototypeId, PrototypeRef},
    raw,
    rng::RetailRng,
    traits::{ArtifactCatalog, SpellCatalog, SpellId},
    treasure::{DefinitionId, SpellReward, TreasureReward},
};
use std::{error::Error, fmt};

/// A factory cannot resolve its inputs or perform a required native operation.
#[derive(Debug)]
pub enum TreasureGenerationError {
    /// Definition came from another prepared catalog.
    Definition(DefinitionId),
    /// Identity, context, arithmetic, or allocation fault.
    Placement(PlacementError),
    /// A lazy treasure valuation failed.
    Value(super::TreasureValueError),
    /// Guard selection failed.
    Guard(super::GuardPlacementError),
    /// Assembly cannot reset a group that still owns objects.
    GroupNotEmpty,
    /// Replacement reads an anchor with no owning map zone.
    UnassignedReplacement(crate::domain::WorldPosition),
    /// A scroll's eligible list is empty; the native draw has already happened.
    EmptyScroll,
    /// Quest artifact art was absent after its native selection point.
    QuestArtifactPrototype,
    /// A selected quest artifact has no loaded art.
    MissingArtifact(crate::traits::ArtifactId),
}
impl fmt::Display for TreasureGenerationError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Definition(id) => write!(f, "foreign treasure definition {}", id.index()),
            Self::Placement(error) => error.fmt(f),
            Self::Value(error) => error.fmt(f),
            Self::Guard(error) => error.fmt(f),
            Self::GroupNotEmpty => {
                f.write_str("discard or commit the previous treasure group first")
            }
            Self::UnassignedReplacement(position) => {
                write!(f, "replacement has no zone at {position:?}")
            }
            Self::EmptyScroll => f.write_str("scroll selection divides by zero after its draw"),
            Self::MissingArtifact(id) => {
                write!(f, "quest artifact {} has no prototype", id.index())
            }
            Self::QuestArtifactPrototype => f.write_str("missing quest artifact prototype"),
        }
    }
}
impl Error for TreasureGenerationError {}
impl From<PlacementError> for TreasureGenerationError {
    fn from(error: PlacementError) -> Self {
        Self::Placement(error)
    }
}

impl From<super::TreasureValueError> for TreasureGenerationError {
    fn from(error: super::TreasureValueError) -> Self {
        Self::Value(error)
    }
}
impl From<super::GuardPlacementError> for TreasureGenerationError {
    fn from(error: super::GuardPlacementError) -> Self {
        Self::Guard(error)
    }
}

/// A definition and its selected art, kept together through later roulette choice.
#[derive(Clone, Copy, Debug)]
pub struct SelectedTreasure {
    definition: DefinitionId,
    prototype: PrototypeId,
}
impl SelectedTreasure {
    /// Definition whose factory will run without choosing art again.
    #[must_use]
    pub const fn definition(self) -> DefinitionId {
        self.definition
    }
    /// Selected prototype, including its original source row identity.
    #[must_use]
    pub const fn prototype(self) -> PrototypeId {
        self.prototype
    }
}
/// Ownership of a generated, unplaced treasure and any pending child.
/// Group insertion will consume this token; failed attempts must explicitly discard it.
#[must_use = "place this pending treasure or explicitly discard it through its generation"]
#[derive(Debug)]
pub struct PendingTreasure {
    object: ObjectId,
    definition: DefinitionId,
}
impl PendingTreasure {
    /// Arena identity for inspecting geometry and payload without transferring ownership.
    #[must_use]
    pub const fn object(&self) -> ObjectId {
        self.object
    }
}

/// Treasure factories bound to one map, arena, and immutable reward-resource context.
pub struct TreasureGeneration<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards> {
    pub(super) ready: TreasuresReady<'state, 'zones, 'tiles, 'defs, 'assets, 'source>,
    spells: &'rewards SpellCatalog,
    artifacts: &'rewards ArtifactCatalog,
    heroes: HeroPool,
    pub(super) arena: Option<OwnerId>,
    pub(super) offers: Vec<SelectedTreasure>,
    pub(super) nested_groups: Vec<Box<super::TreasureGroupWorkspace>>,
}
impl<'state, 'zones, 'tiles, 'defs, 'assets, 'source>
    TreasuresReady<'state, 'zones, 'tiles, 'defs, 'assets, 'source>
{
    /// Admit payload resources and the current map's arena once.
    /// No RNG is consumed; hero availability starts at the native constructor state.
    ///
    /// # Errors
    /// Rejects a foreign arena or prototype context before creating reservations.
    pub fn begin_generation<'rewards>(
        mut self,
        objects: &ObjectArena,
        spells: &'rewards SpellCatalog,
        artifacts: &'rewards ArtifactCatalog,
    ) -> Result<
        TreasureGeneration<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>,
        PlacementError,
    > {
        self.paths
            .map_mut()
            .prepare_object_context(objects, self.catalog.prototypes())?;
        let heroes = HeroPool::new(self.map().coverage().map().version());
        Ok(TreasureGeneration {
            ready: self,
            spells,
            artifacts,
            heroes,
            arena: objects.owner(),
            offers: Vec::new(),
            nested_groups: Vec::new(),
        })
    }
}
impl<'state, 'zones, 'tiles> TreasureGeneration<'state, 'zones, 'tiles, '_, '_, '_, '_> {
    pub(super) fn map_mut(&mut self) -> &mut super::PlacementMap<'state, 'zones, 'tiles> {
        self.ready.paths.map_mut()
    }
}
impl TreasureGeneration<'_, '_, '_, '_, '_, '_, '_> {
    // Treasure placement borrows the outer workspace's buffers and returns them
    // on both success and failure; completed maps need neither buffer.
    pub(super) fn exchange_scratch(
        &mut self,
        offers: &mut Vec<SelectedTreasure>,
        groups: &mut Vec<Box<super::TreasureGroupWorkspace>>,
    ) {
        std::mem::swap(&mut self.offers, offers);
        std::mem::swap(&mut self.nested_groups, groups);
    }
    /// The admitted map, definitions and current offer policies.
    #[must_use]
    pub const fn ready(&self) -> &TreasuresReady<'_, '_, '_, '_, '_, '_> {
        &self.ready
    }
    /// Immutable artifact traits retained for later quest completion.
    #[must_use]
    pub const fn artifacts(&self) -> &ArtifactCatalog {
        self.artifacts
    }
    /// Current hero availability, including pending prison reservations.
    #[must_use]
    pub const fn hero_disabled(&self, hero: HeroId) -> bool {
        self.heroes.is_disabled(hero)
    }

    /// Choose art in the map zone's terrain, without evaluating value or consuming
    /// a factory reservation. Even a singleton art family consumes its native draw.
    ///
    /// # Errors
    /// Rejects a foreign definition or unknown map zone before drawing.
    pub fn choose_prototype(
        &self,
        definition: DefinitionId,
        zone: ZoneId,
        rng: &mut RetailRng,
    ) -> Result<Option<SelectedTreasure>, TreasureGenerationError> {
        let def = self
            .ready
            .catalog()
            .get(definition)
            .ok_or(TreasureGenerationError::Definition(definition))?;
        let terrain = self.ready.map().zone(zone)?.terrain();
        Ok(self
            .ready
            .catalog()
            .prototypes()
            .choose(def.kind(), def.subtype(), terrain, rng)
            .map(|prototype| SelectedTreasure {
                definition,
                prototype: prototype.id(),
            }))
    }
    pub(super) fn require_arena(&self, objects: &ObjectArena) -> Result<(), PlacementError> {
        if self.arena.is_some() && self.arena != objects.owner() {
            return Err(PlacementError::ArenaContext);
        }
        if !self.ready.map().memberships.accepts_arena(objects) {
            return Err(PlacementError::ArenaContext);
        }
        Ok(())
    }
    fn create(
        &mut self,
        objects: &mut ObjectArena,
        prototype: PrototypeId,
        payload: ObjectPayload,
    ) -> Result<ObjectId, PlacementError> {
        let result = objects.create_record(self.ready.catalog().prototypes(), prototype, payload);
        self.arena = objects.owner();
        result
    }

    /// Run the chosen factory, retaining its native RNG and reservation order.
    /// Returns no object only when a prison has no available hero.
    ///
    /// # Errors
    /// Reports foreign contexts before side effects; later allocation, missing
    /// quest-art or empty-scroll faults preserve the draws/reservations already made.
    pub fn generate(
        &mut self,
        selected: SelectedTreasure,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<Option<PendingTreasure>, TreasureGenerationError> {
        use TreasureReward as R;
        self.require_arena(objects)?;
        let definition = *self
            .ready
            .catalog()
            .get(selected.definition)
            .ok_or(TreasureGenerationError::Definition(selected.definition))?;
        let reward = definition.reward();
        let payload = match reward {
            R::Plain(_) => ObjectPayload::Base,
            R::Artifact(_) => ObjectPayload::Artifact,
            R::Creature(reward) => ObjectPayload::Pandora(PandoraReward::Creatures(reward)),
            R::Experience { amount, .. } => {
                ObjectPayload::Pandora(PandoraReward::Experience(amount))
            }
            R::Gold { amount, .. } => ObjectPayload::Pandora(PandoraReward::Gold(amount)),
            R::Spells { spells, .. } => ObjectPayload::Pandora(PandoraReward::Spells(spells)),
            R::KeyTent { value, .. } => ObjectPayload::KeyTent(value),
            R::Dwelling(_) => ObjectPayload::Ownable,
            R::Resource(_) => ObjectPayload::Resource,
            R::Scholar => ObjectPayload::Scholar,
            R::Shrine(_) => ObjectPayload::Shrine,
            R::WitchHut => ObjectPayload::WitchHut,
            R::Scroll { level, .. } => {
                ObjectPayload::Scroll(select_scroll(self.spells, level.get(), rng)?)
            }
            R::Prison { experience, .. } => {
                let Some(hero) = self.heroes.select_prison(rng) else {
                    return Ok(None);
                };
                objects.reserve_record()?;
                let id = self.ready.paths.map_mut().claim_object_id()?;
                ObjectPayload::Prison(PrisonPayload {
                    id,
                    hero,
                    experience,
                })
            }
            R::QuestCreature { reward, .. } => {
                return self
                    .quest(selected, SeerReward::Creatures(reward), objects, rng)
                    .map(Some)
            }
            R::QuestExperience { amount, .. } => {
                return self
                    .quest(selected, SeerReward::Experience(amount), objects, rng)
                    .map(Some)
            }
            R::QuestGold { amount, .. } => {
                return self
                    .quest(selected, SeerReward::Gold(amount), objects, rng)
                    .map(Some)
            }
        };
        let object = self.create(objects, selected.prototype, payload)?;
        Ok(Some(PendingTreasure {
            object,
            definition: selected.definition,
        }))
    }
    fn quest(
        &mut self,
        selected: SelectedTreasure,
        reward: SeerReward,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<PendingTreasure, TreasureGenerationError> {
        let seer = self.create(
            objects,
            selected.prototype,
            ObjectPayload::Seer(SeerPayload {
                reward,
                artifact: None,
            }),
        )?;
        let kind = ObjectKind::RANDOM_ARTIFACT;
        let prototype = self
            .ready
            .catalog()
            .prototypes()
            .choose(kind, 0, Terrain::Dirt, rng)
            .map(PrototypeRef::id);
        let result = match prototype {
            Some(prototype) => self
                .create(
                    objects,
                    prototype,
                    ObjectPayload::QuestArtifact(QuestArtifactPayload {
                        definition: selected.definition,
                        seer: Some(seer),
                    }),
                )
                .map_err(Into::into),
            None => Err(TreasureGenerationError::QuestArtifactPrototype),
        };
        match result {
            Ok(object) => Ok(PendingTreasure {
                object,
                definition: selected.definition,
            }),
            Err(error) => {
                objects.recycle_unplaced(seer)?;
                Err(error)
            }
        }
    }
    /// Release a rejected unplaced treasure, including its hero or pending hut.
    /// Neither RNG nor the serialized object counter is rolled back.
    ///
    /// # Errors
    /// Rejects foreign/stale tokens and positioned objects before cleanup.
    #[expect(
        clippy::needless_pass_by_value,
        reason = "consuming this ownership token prevents a second discard or placement"
    )]
    pub fn discard(
        &mut self,
        pending: PendingTreasure,
        objects: &mut ObjectArena,
    ) -> Result<(), TreasureGenerationError> {
        self.require_arena(objects)?;
        self.require_definition(&pending)?;
        let geometry = objects
            .get(pending.object)
            .ok_or(PlacementError::UnknownObject(pending.object))?;
        if geometry.position().is_some() {
            return Err(PlacementError::PreviouslyPlaced(pending.object).into());
        }
        Self::check_pending_child(pending.object, objects)?;
        self.release_payload(pending.object, objects)?;
        objects.recycle_unplaced(pending.object)?;
        Ok(())
    }
    pub(super) fn require_definition(
        &self,
        pending: &PendingTreasure,
    ) -> Result<(), TreasureGenerationError> {
        if self.ready.catalog().get(pending.definition).is_none() {
            return Err(TreasureGenerationError::Definition(pending.definition));
        }
        Ok(())
    }
    pub(super) fn check_pending_child(
        object: ObjectId,
        objects: &ObjectArena,
    ) -> Result<(), PlacementError> {
        if let Some(ObjectPayload::QuestArtifact(quest)) = objects.payload(object) {
            if let Some(seer) = quest.pending_seer() {
                let geometry = objects
                    .get(seer)
                    .ok_or(PlacementError::UnknownObject(seer))?;
                if geometry.position().is_some() {
                    return Err(PlacementError::PreviouslyPlaced(seer));
                }
            }
        }
        Ok(())
    }
    pub(super) fn release_payload(
        &mut self,
        object: ObjectId,
        objects: &mut ObjectArena,
    ) -> Result<(), PlacementError> {
        match *objects
            .payload(object)
            .ok_or(PlacementError::UnknownObject(object))?
        {
            ObjectPayload::Prison(prison) => self.heroes.release_prison(prison.hero()),
            ObjectPayload::QuestArtifact(quest) => {
                if let Some(seer) = quest.pending_seer() {
                    objects.recycle_unplaced(seer)?;
                }
            }
            _ => {}
        }
        Ok(())
    }
    /// Expand a Pandora spell payload through the generation's exact immutable
    /// catalog. Native order is descending level, then ascending spell ID.
    ///
    /// The reward already carries admitted level bounds and school bits.
    pub fn pandora_spells(&self, reward: SpellReward) -> impl Iterator<Item = SpellId> + '_ {
        pandora_spells(self.spells, reward)
    }
}

fn pandora_spells(
    spells: &SpellCatalog,
    reward: SpellReward,
) -> impl Iterator<Item = SpellId> + '_ {
    let (minimum, maximum) = reward.levels();
    (minimum..=maximum).rev().flat_map(move |level| {
        spells
            .iter()
            .take(raw::HERO_SPELL_COUNT as usize)
            .filter(move |(_, traits)| {
                !traits.disabled_by_default()
                    && traits.level() == level
                    && traits.schools().intersects(reward.schools())
            })
            .map(|(id, _)| id)
    })
}
fn select_scroll(
    spells: &SpellCatalog,
    level: u32,
    rng: &mut RetailRng,
) -> Result<SpellId, TreasureGenerationError> {
    let eligible = || {
        spells
            .iter()
            .take(raw::HERO_SPELL_COUNT as usize)
            .filter(|(_, traits)| {
                !traits.disabled_by_default()
                    && traits.schools().bits() != 0
                    && i64::from(traits.level()) == i64::from(level)
            })
    };
    let count = eligible().count();
    let draw = rng.draw();
    if count == 0 {
        return Err(TreasureGenerationError::EmptyScroll);
    }
    let (id, _) = eligible()
        .nth(draw as usize % count)
        .expect("eligible count and selection use identical immutable traits");
    Ok(id)
}

#[cfg(test)]
mod tests;
