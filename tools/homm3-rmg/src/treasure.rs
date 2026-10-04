//! Source-ordered treasure definitions prepared before generation consumes RNG.
use crate::{
    identity::OwnerId,
    object::ObjectKind,
    prototype::PrototypeCatalog,
    raw,
    request::MapVersion,
    traits::{CreatureCatalog, CreatureId, SpellSchools},
};
use std::{collections::TryReserveError, error::Error, fmt, num::NonZeroU32};

/// Definition construction cannot perform the native arithmetic or reserve storage.
#[derive(Debug)]
pub enum TreasureError {
    /// Canonical recipe arguments do not describe an admitted definition.
    Recipe,
    /// Process-local catalog ownership tags are exhausted.
    IdentityExhausted,
    /// Eager creature reward construction divides by zero or overflows.
    CreatureCount(CreatureId),
    /// A family length or scalar cannot fit its native signed representation.
    Arithmetic,
    /// Definition storage could not grow.
    Allocation(TryReserveError),
}
impl fmt::Display for TreasureError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::IdentityExhausted => f.write_str("treasure catalog identity exhausted"),
            Self::Recipe => f.write_str("unsupported canonical treasure recipe"),
            Self::CreatureCount(id) => write!(
                f,
                "invalid treasure count arithmetic for creature {}",
                id.index()
            ),
            Self::Arithmetic => f.write_str("treasure definition arithmetic overflow"),
            Self::Allocation(error) => error.fmt(f),
        }
    }
}
impl Error for TreasureError {}
impl From<TryReserveError> for TreasureError {
    fn from(error: TryReserveError) -> Self {
        Self::Allocation(error)
    }
}

/// A definition's native family ordinal, distinct from a prototype's stored subtype.
/// Prepared from the loaded tent/seer family length, without selecting its art.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct FamilyOrdinal(i32);
impl FamilyOrdinal {
    fn new(index: usize) -> Result<Self, TreasureError> {
        Ok(Self(
            i32::try_from(index).map_err(|_| TreasureError::Arithmetic)?,
        ))
    }
    /// Native definition subtype.
    #[must_use]
    pub const fn value(self) -> i32 {
        self.0
    }
}

/// Eagerly computed creature reward; signed custom counts retain native semantics.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct CreatureReward {
    creature: CreatureId,
    count: i32,
}
impl CreatureReward {
    fn new(creature: CreatureId, traits: &CreatureCatalog) -> Result<Self, TreasureError> {
        let entry = traits.get(creature);
        let tier = entry.tier().ok_or(TreasureError::Recipe)?;
        let count = raw::CREATURE_REWARD_VALUES[tier.index()]
            .checked_div(
                entry
                    .ai_value()
                    .ok_or(TreasureError::CreatureCount(creature))?
                    .get(),
            )
            .ok_or(TreasureError::CreatureCount(creature))?;
        let step = if count > 50 {
            10
        } else if count > 12 {
            5
        } else if count > 5 {
            2
        } else {
            1
        };
        let count = count
            .checked_add(step / 2)
            .and_then(|value| (value / step).checked_mul(step))
            .ok_or(TreasureError::CreatureCount(creature))?;
        Ok(Self { creature, count })
    }
    /// Admitted creature table entry.
    #[must_use]
    pub const fn creature(self) -> CreatureId {
        self.creature
    }
    /// Signed constructor count, before faction-dependent valuation.
    #[must_use]
    pub const fn count(self) -> i32 {
        self.count
    }
}

/// A nonempty inclusive spell-level interval with admitted school bits.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct SpellReward {
    minimum: i32,
    maximum: i32,
    schools: SpellSchools,
}
impl SpellReward {
    fn new(minimum: i32, maximum: i32, schools: i32) -> Result<Self, TreasureError> {
        if minimum <= 0 || minimum > maximum {
            return Err(TreasureError::Recipe);
        }
        let schools = u32::try_from(schools)
            .ok()
            .and_then(SpellSchools::parse)
            .ok_or(TreasureError::Recipe)?;
        Ok(Self {
            minimum,
            maximum,
            schools,
        })
    }
    /// Inclusive source level bounds; generation visits levels from high to low.
    #[must_use]
    pub const fn levels(self) -> (i32, i32) {
        (self.minimum, self.maximum)
    }
    /// Schools considered during later spell generation.
    #[must_use]
    pub const fn schools(self) -> SpellSchools {
        self.schools
    }
}

/// Payload and value policy, without an unrelated sentinel field for dynamic values.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TreasureReward {
    /// Plain object with a fixed value.
    Plain(i32),
    /// Artifact with a fixed value.
    Artifact(i32),
    /// Creature box with a faction-dependent value.
    Creature(CreatureReward),
    /// Experience box.
    Experience {
        /// Fixed native selection value.
        value: i32,
        /// Reward quantity granted by the object.
        amount: i32,
    },
    /// Gold box.
    Gold {
        /// Fixed native selection value.
        value: i32,
        /// Reward quantity granted by the object.
        amount: i32,
    },
    /// Spell box; spell eligibility remains a generation-time operation.
    Spells {
        /// Fixed native selection value.
        value: i32,
        /// Admitted spell level and school selection.
        spells: SpellReward,
    },
    /// Key tent, offered only when its ordinal matches the current cursor.
    KeyTent {
        /// Native family ordinal used for the definition subtype.
        ordinal: FamilyOrdinal,
        /// Fixed native selection value.
        value: i32,
    },
    /// Dwelling with a creature/faction-dependent value.
    Dwelling(CreatureId),
    /// Prison hero and experience, reserved only during generation.
    Prison {
        /// Fixed native selection value.
        value: i32,
        /// Experience assigned to the imprisoned hero.
        experience: i32,
    },
    /// Resource pile with native default amount.
    Resource(i32),
    /// Scholar with source-default value.
    Scholar,
    /// Shrine with fixed value.
    Shrine(i32),
    /// Seer reward inheriting the creature box's value policy and density.
    QuestCreature {
        /// Native family ordinal used for the definition subtype.
        ordinal: FamilyOrdinal,
        /// Eagerly constructed creature reward.
        reward: CreatureReward,
    },
    /// Seer experience reward.
    QuestExperience {
        /// Native family ordinal used for the definition subtype.
        ordinal: FamilyOrdinal,
        /// Fixed native selection value.
        value: i32,
        /// Reward quantity granted by the object.
        amount: i32,
    },
    /// Seer gold reward.
    QuestGold {
        /// Native family ordinal used for the definition subtype.
        ordinal: FamilyOrdinal,
        /// Fixed native selection value.
        value: i32,
        /// Reward quantity granted by the object.
        amount: i32,
    },
    /// Scroll of the given level; eligible spells are queried during generation.
    Scroll {
        /// Positive spell level used for the later eligibility query.
        level: NonZeroU32,
        /// Fixed native selection value.
        value: i32,
    },
    /// Witch hut with source-default value.
    WitchHut,
}
const _: () = {
    assert!(raw::RMG_SCHOLAR_REWARD_VALUE <= i32::MAX as u32);
    assert!(raw::RMG_WITCH_HUT_REWARD_VALUE <= i32::MAX as u32);
};
impl TreasureReward {
    /// Constructor's fixed value, or no fixed value for creature/dwelling policies.
    #[must_use]
    #[expect(
        clippy::cast_possible_wrap,
        reason = "source defaults are asserted to fit i32"
    )]
    pub const fn fixed_value(self) -> Option<i32> {
        match self {
            Self::Plain(value)
            | Self::Artifact(value)
            | Self::Resource(value)
            | Self::Shrine(value)
            | Self::Experience { value, .. }
            | Self::Gold { value, .. }
            | Self::Spells { value, .. }
            | Self::KeyTent { value, .. }
            | Self::Prison { value, .. }
            | Self::QuestExperience { value, .. }
            | Self::QuestGold { value, .. }
            | Self::Scroll { value, .. } => Some(value),
            Self::Scholar => Some(raw::RMG_SCHOLAR_REWARD_VALUE as i32),
            Self::WitchHut => Some(raw::RMG_WITCH_HUT_REWARD_VALUE as i32),
            Self::Creature(_) | Self::Dwelling(_) | Self::QuestCreature { .. } => None,
        }
    }
    /// Source isTerrainDependent policy; faction-sensitive boxes/dwellings are false.
    #[must_use]
    pub const fn terrain_dependent(self) -> bool {
        matches!(
            self,
            Self::KeyTent { .. }
                | Self::QuestCreature { .. }
                | Self::QuestExperience { .. }
                | Self::QuestGold { .. }
        )
    }
}

/// Stable identity within one prepared catalog; invalidated by preparation again.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct DefinitionId {
    index: usize,
    owner: OwnerId,
}
impl DefinitionId {
    /// Native insertion ordinal, distinct from object kind or subtype.
    #[must_use]
    pub const fn index(self) -> usize {
        self.index
    }
}

/// One admitted constructor record; selection metadata cannot be mutated separately.
#[derive(Clone, Copy, Debug)]
pub struct TreasureDefinition {
    kind: ObjectKind,
    subtype: i32,
    density: NonZeroU32,
    reward: TreasureReward,
}
impl TreasureDefinition {
    /// Native object-family selector; definitions remain even if that family is empty.
    #[must_use]
    pub const fn kind(self) -> ObjectKind {
        self.kind
    }
    /// Native selector subtype, including family ordinals for tents and seer huts.
    #[must_use]
    pub const fn subtype(self) -> i32 {
        self.subtype
    }
    /// Positive source roulette weight.
    #[must_use]
    pub const fn density(self) -> NonZeroU32 {
        self.density
    }
    /// Admitted payload/value policy.
    #[must_use]
    pub const fn reward(self) -> TreasureReward {
        self.reward
    }
}

/// Reusable contiguous definition storage; no per-definition object allocations.
#[derive(Default, Debug)]
pub struct TreasureWorkspace {
    definitions: Vec<TreasureDefinition>,
}
/// Immutable prepared definitions and their exact prototype/creature context.
/// Does not reset map-owned tent, hero or quest reservations.
pub struct TreasureCatalog<'workspace, 'assets, 'source> {
    owner: OwnerId,
    definitions: &'workspace [TreasureDefinition],
    prototypes: &'assets PrototypeCatalog<'source>,
    creatures: &'assets CreatureCatalog,
}
impl TreasureCatalog<'_, '_, '_> {
    /// Definition identities in native insertion order, without allocating.
    #[must_use]
    pub fn ids(&self) -> impl ExactSizeIterator<Item = DefinitionId> + DoubleEndedIterator + '_ {
        (0..self.definitions.len()).map(|index| DefinitionId {
            index,
            owner: self.owner,
        })
    }
    /// Resolved definitions and their identities in insertion order, without allocating.
    #[must_use]
    pub fn iter(
        &self,
    ) -> impl ExactSizeIterator<Item = (DefinitionId, &TreasureDefinition)> + DoubleEndedIterator + '_
    {
        self.ids().zip(self.definitions)
    }
    /// Resolve only an identity belonging to this preparation.
    #[must_use]
    pub fn get(&self, id: DefinitionId) -> Option<&TreasureDefinition> {
        (id.owner == self.owner)
            .then(|| self.definitions.get(id.index))
            .flatten()
    }

    /// Definitions in native insertion order, including repeated values.
    #[must_use]
    pub const fn definitions(&self) -> &[TreasureDefinition] {
        self.definitions
    }
    /// Prototype context used to expand family ordinals and versioned blocks.
    #[must_use]
    pub const fn prototypes(&self) -> &PrototypeCatalog<'_> {
        self.prototypes
    }
    /// Creature traits used to compute all constructor reward counts.
    #[must_use]
    pub const fn creatures(&self) -> &CreatureCatalog {
        self.creatures
    }
}
impl TreasureWorkspace {
    /// Expand canonical recipes before generation begins, without RNG or pool mutations.
    /// The caller skips this initialization on the native no-template path.
    ///
    /// # Errors
    /// Reports invalid canonical data, eager creature-count arithmetic or allocation failure.
    pub fn prepare<'workspace, 'assets, 'source>(
        &'workspace mut self,
        prototypes: &'assets PrototypeCatalog<'source>,
        creatures: &'assets CreatureCatalog,
    ) -> Result<TreasureCatalog<'workspace, 'assets, 'source>, TreasureError> {
        let owner = OwnerId::new().ok_or(TreasureError::IdentityExhausted)?;
        self.definitions.clear();
        for &recipe in raw::TREASURE_RECIPES {
            self.expand(recipe, prototypes, creatures)?;
        }
        Ok(TreasureCatalog {
            owner,
            definitions: &self.definitions,
            prototypes,
            creatures,
        })
    }
    fn push(
        &mut self,
        kind: u32,
        subtype: i32,
        density: u32,
        reward: TreasureReward,
    ) -> Result<(), TreasureError> {
        let kind = i32::try_from(kind)
            .ok()
            .and_then(ObjectKind::parse)
            .ok_or(TreasureError::Recipe)?;
        let density = NonZeroU32::new(density)
            .filter(|value| i32::try_from(value.get()).is_ok())
            .ok_or(TreasureError::Recipe)?;
        self.definitions.try_reserve(1)?;
        self.definitions.push(TreasureDefinition {
            kind,
            subtype,
            density,
            reward,
        });
        Ok(())
    }
    fn creature_rewards(
        &mut self,
        version: MapVersion,
        creatures: &CreatureCatalog,
        seer: Option<FamilyOrdinal>,
    ) -> Result<(), TreasureError> {
        let count = if version == MapVersion::Restoration {
            raw::RMG_ROE_CREATURE_TYPE_COUNT
        } else {
            raw::RMG_CREATURE_TYPE_COUNT
        };
        for index in (0..count).rev() {
            let id =
                CreatureId::parse(i32::try_from(index).map_err(|_| TreasureError::Arithmetic)?)
                    .ok_or(TreasureError::Recipe)?;
            if creatures.get(id).tier().is_none() {
                continue;
            }
            let reward = CreatureReward::new(id, creatures)?;
            match seer {
                Some(ordinal) => self.push(
                    raw::SEER,
                    ordinal.value(),
                    raw::RMG_CREATURE_REWARD_DENSITY,
                    TreasureReward::QuestCreature { ordinal, reward },
                )?,
                None => self.push(
                    raw::BLACK_BOX,
                    0,
                    raw::RMG_CREATURE_REWARD_DENSITY,
                    TreasureReward::Creature(reward),
                )?,
            }
        }
        Ok(())
    }
    fn expand(
        &mut self,
        recipe: raw::TreasureRecipe,
        prototypes: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
    ) -> Result<(), TreasureError> {
        use raw::TreasureRecipe as R;
        use TreasureReward as T;
        match recipe {
            R::Plain(kind, subtype, value, density) => self.push(
                unsigned(kind)?,
                subtype,
                unsigned(density)?,
                T::Plain(value),
            ),
            R::Artifact(kind, value) => self.push(
                unsigned(kind)?,
                0,
                raw::RMG_ARTIFACT_REWARD_DENSITY,
                T::Artifact(value),
            ),
            R::Experience(value, amount) => self.push(
                raw::BLACK_BOX,
                0,
                raw::RMG_EXPERIENCE_BOX_DENSITY,
                T::Experience { value, amount },
            ),
            R::Gold(value, amount) => self.push(
                raw::BLACK_BOX,
                0,
                raw::RMG_GOLD_BOX_DENSITY,
                T::Gold { value, amount },
            ),
            R::Spells(value, minimum, maximum, schools) => self.push(
                raw::BLACK_BOX,
                0,
                raw::RMG_SPELL_BOX_DENSITY,
                T::Spells {
                    value,
                    spells: SpellReward::new(minimum, maximum, schools)?,
                },
            ),
            R::Prison(value, experience) => self.push(
                raw::PRISON,
                0,
                raw::RMG_PRISON_REWARD_DENSITY,
                T::Prison { value, experience },
            ),
            R::Resource(kind, subtype, value, density) => self.push(
                unsigned(kind)?,
                subtype,
                unsigned(density)?,
                T::Resource(value),
            ),
            R::Scholar => self.push(raw::SCHOLAR, 0, raw::RMG_SCHOLAR_REWARD_DENSITY, T::Scholar),
            R::Shrine(kind, value) => self.push(
                unsigned(kind)?,
                0,
                raw::RMG_SHRINE_REWARD_DENSITY,
                T::Shrine(value),
            ),
            R::Scroll(level, value) => self.push(
                raw::SPELL_SCROLL,
                0,
                raw::RMG_SCROLL_REWARD_DENSITY,
                T::Scroll {
                    level: NonZeroU32::new(unsigned(level)?).ok_or(TreasureError::Recipe)?,
                    value,
                },
            ),
            R::WitchHut => self.push(
                raw::WITCH_HUT,
                0,
                raw::RMG_WITCH_HUT_REWARD_DENSITY,
                T::WitchHut,
            ),
            R::CreatureBoxes => self.creature_rewards(prototypes.version(), creatures, None),
            R::KeyTents => self.key_tents(prototypes),
            R::Dwellings => self.dwellings(prototypes),
            R::Seers => self.seers(prototypes, creatures),
        }
    }
    fn key_tents(&mut self, prototypes: &PrototypeCatalog<'_>) -> Result<(), TreasureError> {
        use TreasureReward as T;
        for index in (0..family_count(prototypes, raw::BORDER_TENT)?).rev() {
            let ordinal = FamilyOrdinal::new(index)?;
            for &value in &raw::KEY_TENT_REWARD_VALUES {
                self.push(
                    raw::BORDER_TENT,
                    ordinal.value(),
                    raw::RMG_KEY_TENT_REWARD_DENSITY,
                    T::KeyTent { ordinal, value },
                )?;
            }
        }
        Ok(())
    }
    fn dwellings(&mut self, prototypes: &PrototypeCatalog<'_>) -> Result<(), TreasureError> {
        use TreasureReward as T;
        let count = if prototypes.version() == MapVersion::Restoration {
            raw::RMG_ROE_DWELLING_SUBTYPE_COUNT
        } else {
            raw::RMG_DWELLING_SUBTYPE_COUNT
        };
        for subtype in (0..count).rev() {
            let index = usize::try_from(subtype).map_err(|_| TreasureError::Arithmetic)?;
            let creature = raw::DWELLING_CREATURES
                .get(index)
                .copied()
                .and_then(CreatureId::parse)
                .ok_or(TreasureError::Recipe)?;
            self.push(
                raw::CREATURE_GENERATOR_1,
                i32::try_from(subtype).map_err(|_| TreasureError::Arithmetic)?,
                raw::RMG_DWELLING_REWARD_DENSITY,
                T::Dwelling(creature),
            )?;
        }
        Ok(())
    }
    fn seers(
        &mut self,
        prototypes: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
    ) -> Result<(), TreasureError> {
        use TreasureReward as T;
        for index in 0..family_count(prototypes, raw::SEER)? {
            let ordinal = FamilyOrdinal::new(index)?;
            self.creature_rewards(prototypes.version(), creatures, Some(ordinal))?;
            for &recipe in raw::SEER_REWARD_RECIPES {
                let reward = match recipe {
                    raw::SeerRewardRecipe::Experience(value, amount) => T::QuestExperience {
                        ordinal,
                        value,
                        amount,
                    },
                    raw::SeerRewardRecipe::Gold(value, amount) => T::QuestGold {
                        ordinal,
                        value,
                        amount,
                    },
                };
                self.push(
                    raw::SEER,
                    ordinal.value(),
                    raw::RMG_QUEST_REWARD_DENSITY,
                    reward,
                )?;
            }
        }
        Ok(())
    }
}
fn unsigned(value: i32) -> Result<u32, TreasureError> {
    u32::try_from(value).map_err(|_| TreasureError::Recipe)
}
fn family_count(prototypes: &PrototypeCatalog<'_>, kind: u32) -> Result<usize, TreasureError> {
    let kind = i32::try_from(kind)
        .ok()
        .and_then(ObjectKind::parse)
        .ok_or(TreasureError::Recipe)?;
    let count = prototypes.family(kind).len();
    i32::try_from(count).map_err(|_| TreasureError::Arithmetic)?;
    Ok(count)
}

#[cfg(test)]
mod tests;
