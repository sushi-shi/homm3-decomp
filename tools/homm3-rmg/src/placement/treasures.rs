//! Catalog-bound lazy treasure offers and map-owned completion state.
use super::{KeyTentCursor, PlacementError, PlacementMap, TownZoneCounts, TreasurePaths};
use crate::{
    boundaries::CreaturePreference,
    geometry::ZoneId,
    raw,
    traits::{ArtifactId, CreatureCatalog, CreatureId},
    treasure::{CreatureReward, DefinitionId, FamilyOrdinal, TreasureCatalog, TreasureReward},
};
use std::{error::Error, fmt};

/// A lazy value query cannot resolve its identities or perform native arithmetic.
#[derive(Debug)]
pub enum TreasureValueError {
    /// The definition belongs to another preparation.
    Definition(DefinitionId),
    /// Invalid map context, zone, replay state, or arithmetic.
    Placement(PlacementError),
}
impl fmt::Display for TreasureValueError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Definition(id) => write!(f, "foreign treasure definition {}", id.index()),
            Self::Placement(error) => error.fmt(f),
        }
    }
}
impl Error for TreasureValueError {}
impl From<PlacementError> for TreasureValueError {
    fn from(error: PlacementError) -> Self {
        Self::Placement(error)
    }
}

// These are native constructor states, not a precomputed artifact-eligibility
// cache. Completion checks availability and sets the low flag at that time.
pub(super) struct QuestState {
    pub(super) next_seer: i32,
    pub(super) pool_low: bool,
    pub(super) used: [bool; raw::ARTIFACT_COUNT as usize],
}
impl Default for QuestState {
    fn default() -> Self {
        Self {
            next_seer: 0,
            pool_low: false,
            used: [false; raw::ARTIFACT_COUNT as usize],
        }
    }
}
impl QuestState {
    fn offers(&self, ordinal: FamilyOrdinal) -> bool {
        self.next_seer == ordinal.value() && !self.pool_low
    }
}

/// Treasure placement state bound once to the catalog used for this generation.
/// Definitions keep their original creature traits; cursors and reservations
/// belong to this map and cannot be supplied by callers during value queries.
pub struct TreasuresReady<'state, 'zones, 'tiles, 'defs, 'assets, 'source> {
    pub(super) paths: TreasurePaths<'state, 'zones, 'tiles>,
    pub(super) catalog: TreasureCatalog<'defs, 'assets, 'source>,
    pub(super) quests: QuestState,
}
impl<'state, 'zones, 'tiles> TreasurePaths<'state, 'zones, 'tiles> {
    /// Bind pre-generation definitions to the completed post-mine map.
    /// No RNG is consumed and existing tent reservations are retained.
    ///
    /// # Errors
    /// Rejects a catalog built from another map's prototype context.
    pub fn begin_treasures<'defs, 'assets, 'source>(
        self,
        catalog: TreasureCatalog<'defs, 'assets, 'source>,
    ) -> Result<TreasuresReady<'state, 'zones, 'tiles, 'defs, 'assets, 'source>, PlacementError>
    {
        self.map()
            .registration
            .require_catalog(catalog.prototypes())?;
        Ok(TreasuresReady {
            paths: self,
            catalog,
            quests: QuestState::default(),
        })
    }
}
impl TreasuresReady<'_, '_, '_, '_, '_, '_> {
    /// Current generation map, retaining its existing geometry and registrations.
    #[must_use]
    pub const fn map(&self) -> &PlacementMap<'_, '_, '_> {
        self.paths.map()
    }
    /// Prepared definitions bound to this generation.
    #[must_use]
    pub const fn catalog(&self) -> &TreasureCatalog<'_, '_, '_> {
        &self.catalog
    }
    /// Frozen primary-town counts used for lazy faction adjustments.
    #[must_use]
    pub const fn town_zones(&self) -> &TownZoneCounts {
        self.paths.town_zones()
    }
    /// Native seer-family cursor, initially zero even if the family is absent.
    #[must_use]
    pub const fn next_seer_ordinal(&self) -> i32 {
        self.quests.next_seer
    }
    /// Sticky flag set only when quest completion observes a low artifact pool.
    #[must_use]
    pub const fn quest_pool_low(&self) -> bool {
        self.quests.pool_low
    }
    /// Whether successful quest completion has claimed this artifact.
    #[must_use]
    pub const fn quest_artifact_used(&self, artifact: ArtifactId) -> bool {
        self.quests.used[artifact.index()]
    }

    /// Evaluate one definition at its native lazy query point, without RNG.
    /// Callers must apply group/type-limit gates before this operation. Signed
    /// results are preserved: selection rejects every negative value, while
    /// replacement callbacks receive the exact result, including quest transforms.
    ///
    /// # Errors
    /// Reports foreign definition/map identities, a required retail tent replay
    /// value, or signed overflow at the operation the source actually evaluates.
    pub fn value(&self, definition: DefinitionId, zone: ZoneId) -> Result<i32, TreasureValueError> {
        let definition = self
            .catalog
            .get(definition)
            .ok_or(TreasureValueError::Definition(definition))?;
        let zone = self.map().zone(zone)?;
        let value = reward_value(
            definition.reward(),
            zone.creatures(),
            self.catalog.creatures(),
            self.town_zones(),
            self.map().registration.tent_cursor(),
            &self.quests,
        )?;
        Ok(value)
    }
}

#[expect(
    clippy::cast_possible_wrap,
    reason = "source reward defaults are asserted to fit i32 in treasure"
)]
fn reward_value(
    reward: TreasureReward,
    faction: CreaturePreference,
    creatures: &CreatureCatalog,
    towns: &TownZoneCounts,
    tent: KeyTentCursor,
    quests: &QuestState,
) -> Result<i32, PlacementError> {
    match reward {
        TreasureReward::Creature(reward) => creature_value(reward, faction, creatures, towns),
        TreasureReward::Dwelling(creature) => dwelling_value(creature, faction, creatures, towns),
        TreasureReward::QuestCreature { ordinal, reward } => {
            if !quests.offers(ordinal) {
                return Ok(-1);
            }
            let value = creature_value(reward, faction, creatures, towns)?;
            // A mismatching creature faction returns -1 from the base policy,
            // but this eligible quest still transforms it to -1334.
            value
                .checked_mul(2)
                .and_then(|value| value.checked_sub(4000))
                .map(|value| value / 3)
                .ok_or(PlacementError::Arithmetic)
        }
        TreasureReward::QuestExperience { ordinal, value, .. }
        | TreasureReward::QuestGold { ordinal, value, .. } => {
            Ok(if quests.offers(ordinal) { value } else { -1 })
        }
        TreasureReward::KeyTent { ordinal, value } => match tent {
            KeyTentCursor::ReplayRequired => Err(PlacementError::KeyTentReplayRequired),
            KeyTentCursor::Value(cursor) => Ok(if cursor == ordinal.value() { value } else { -1 }),
        },
        TreasureReward::Plain(value)
        | TreasureReward::Artifact(value)
        | TreasureReward::Resource(value)
        | TreasureReward::Shrine(value)
        | TreasureReward::Experience { value, .. }
        | TreasureReward::Gold { value, .. }
        | TreasureReward::Spells { value, .. }
        | TreasureReward::Prison { value, .. }
        | TreasureReward::Scroll { value, .. } => Ok(value),
        TreasureReward::Scholar => Ok(raw::RMG_SCHOLAR_REWARD_VALUE as i32),
        TreasureReward::WitchHut => Ok(raw::RMG_WITCH_HUT_REWARD_VALUE as i32),
    }
}
fn matches_faction(faction: CreaturePreference, town: Option<crate::request::Town>) -> bool {
    match (faction, town) {
        (CreaturePreference::Faction(expected), Some(actual)) => expected == actual,
        (CreaturePreference::Neutral, None) => true,
        _ => false,
    }
}
fn creature_value(
    reward: CreatureReward,
    faction: CreaturePreference,
    creatures: &CreatureCatalog,
    towns: &TownZoneCounts,
) -> Result<i32, PlacementError> {
    let creature = creatures.get(reward.creature());
    if !matches_faction(faction, creature.town()) {
        return Ok(-1);
    }
    let value = creature
        .ai_value()
        .map_or(0, std::num::NonZeroI32::get)
        .checked_mul(reward.count())
        .ok_or(PlacementError::Arithmetic)?;
    towns.adjust(value, creature.town())
}
fn dwelling_value(
    id: CreatureId,
    faction: CreaturePreference,
    creatures: &CreatureCatalog,
    towns: &TownZoneCounts,
) -> Result<i32, PlacementError> {
    let creature = creatures.get(id);
    if !matches_faction(faction, creature.town()) {
        return Ok(-1);
    }
    let ai = creature.ai_value().map_or(0, std::num::NonZeroI32::get);
    let value = creature
        .growth()
        .checked_mul(ai)
        .ok_or(PlacementError::Arithmetic)?;
    let value = towns.adjust(value, creature.town())?;
    let extra = ai
        .checked_mul(towns.aligned(creature.town()))
        .ok_or(PlacementError::Arithmetic)?
        / 2;
    value.checked_add(extra).ok_or(PlacementError::Arithmetic)
}

#[cfg(test)]
mod tests;
