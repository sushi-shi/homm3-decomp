//! Native guard eligibility, including the `RoE` and missing-prototype quirks.

use super::{PrototypeCatalog, PrototypeId};
use crate::{
    raw,
    request::{MapVersion, Town},
    rng::RetailRng,
    template::Zone,
    traits::{CreatureCatalog, CreatureId},
};
use std::{error::Error, fmt, num::NonZeroU32};

const CREATURES: usize = raw::RMG_CREATURE_TYPE_COUNT as usize;

/// Guard affiliations after applying the zone-alignment override.
#[derive(Clone, Copy, Debug)]
pub enum GuardFactions<'a> {
    /// Only creatures of this town; neutral creatures are excluded.
    Matching(Town),
    /// Template order: neutral first, then the catalog's town factions.
    Allowed(&'a [bool]),
}
impl<'a> GuardFactions<'a> {
    /// Resolve the template's alignment-matching flag. A neutral alignment
    /// retains the template's allowed factions even when the flag is set.
    #[must_use]
    pub fn from_zone(zone: &'a Zone, alignment: Option<Town>) -> Self {
        match (zone.guards_match_alignment(), alignment) {
            (true, Some(town)) => Self::Matching(town),
            _ => Self::Allowed(zone.allowed_monsters()),
        }
    }
    fn allows(self, town: Option<Town>) -> bool {
        match self {
            Self::Matching(wanted) => town == Some(wanted),
            Self::Allowed(allowed) => allowed
                .get(town.map_or(0, |town| town.index() + 1))
                .copied()
                .unwrap_or(false),
        }
    }
}

/// A selected guard before the generation owner assigns its next object ID.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct GuardStack {
    prototype: PrototypeId,
    creature: CreatureId,
    count: i32,
}
impl GuardStack {
    /// Selected catalog entry (the last loaded prototype for its creature).
    #[must_use]
    pub const fn prototype(self) -> PrototypeId {
        self.prototype
    }
    /// Parsed creature identity, never the retail failure sentinel -1.
    #[must_use]
    pub const fn creature(self) -> CreatureId {
        self.creature
    }
    /// Native signed stack quantity. Custom resource values are not silently
    /// clamped; serialization keeps its low two bytes, as the native writer does.
    #[must_use]
    pub const fn count(self) -> i32 {
        self.count
    }
}

/// A retail operation has no defined safe guard result.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum GuardError {
    /// A retained retail monster subtype would index outside the creature table.
    CreatureSubtype(i32),
    /// Retail counted more eligible creatures than its loaded prototypes can supply.
    MissingSelectedPrototype,
    /// Selected creature has zero AI value, used as a divisor.
    ZeroAiValue(CreatureId),
    /// Signed native arithmetic has no representable 32-bit result.
    Arithmetic {
        /// Creature whose traits triggered the operation.
        creature: CreatureId,
        /// Operation being evaluated.
        operation: &'static str,
    },
}
impl fmt::Display for GuardError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::CreatureSubtype(id) => {
                write!(f, "guard prototype has invalid creature subtype {id}")
            }
            Self::MissingSelectedPrototype => {
                f.write_str("retail guard selection exhausted loaded prototypes")
            }
            Self::ZeroAiValue(id) => write!(f, "guard creature {} has zero AI value", id.index()),
            Self::Arithmetic {
                creature,
                operation,
            } => write!(
                f,
                "guard creature {} overflows {operation}",
                creature.index()
            ),
        }
    }
}
impl Error for GuardError {}

impl PrototypeCatalog<'_> {
    /// Select a native guard without allocating a candidate vector or object.
    /// The owner assigns an object ID only after a successful `Some` result.
    ///
    /// # Errors
    /// Reports retained invalid subtypes, undefined arithmetic and retail's
    /// missing-prototype selection fault. Hotfix returns `None` for the latter.
    pub fn select_guard(
        &self,
        value: i32,
        factions: GuardFactions<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<Option<GuardStack>, GuardError> {
        let mut prototypes = [None; CREATURES];
        let start = self.offsets[raw::MONSTER as usize];
        let end = self.offsets[raw::MONSTER as usize + 1];
        for (index, entry) in self.entries[start..end].iter().enumerate() {
            let creature = CreatureId::parse(entry.prototype.subtype)
                .ok_or(GuardError::CreatureSubtype(entry.prototype.subtype))?;
            prototypes[creature.index()] = Some(self.id(start + index));
        }
        self.choose_guard(value, factions, creatures, prototypes, rng)
    }

    fn choose_guard(
        &self,
        value: i32,
        factions: GuardFactions<'_>,
        creatures: &CreatureCatalog,
        mut prototypes: [Option<PrototypeId>; CREATURES],
        rng: &mut RetailRng,
    ) -> Result<Option<GuardStack>, GuardError> {
        let evaluated = if self.version == MapVersion::Restoration {
            prototypes[raw::RMG_ROE_CREATURE_TYPE_COUNT as usize..].fill(None);
            // 117 stays in the selection array, but is not evaluated or counted.
            raw::RMG_ROE_CREATURE_TYPE_COUNT as usize - 1
        } else {
            CREATURES
        };
        let mut eligible = 0;
        for index in (0..evaluated).rev() {
            let creature = creature_at(index);
            let traits = *creatures.get(creature);
            let ai = traits.ai_value().map_or(0, std::num::NonZeroI32::get);
            let (low, high) = traits.wandering_counts();
            let minimum = arithmetic(high.checked_add(low), creature, "wandering count sum")? / 2;
            let minimum = arithmetic(minimum.checked_mul(ai), creature, "minimum guard value")?;
            // Preserve native short-circuit order, including arithmetic faults.
            if minimum <= value
                && value
                    <= arithmetic(
                        ai.checked_mul(
                            i32::try_from(raw::RMG_GUARD_MAXIMUM_COUNT)
                                .expect("native guard count"),
                        ),
                        creature,
                        "maximum guard value",
                    )?
                && traits.tier().is_some()
                && factions.allows(traits.town())
            {
                eligible += 1;
            } else {
                prototypes[index] = None;
            }
        }
        let Some(eligible) = NonZeroU32::new(eligible) else {
            return Ok(None);
        };
        let rank = rng.below(eligible) as usize;
        let Some((index, prototype)) = prototypes
            .into_iter()
            .enumerate()
            .rev()
            .filter_map(|(index, prototype)| prototype.map(|prototype| (index, prototype)))
            .nth(rank)
        else {
            return if self.behavior.is_hotfix() {
                Ok(None)
            } else {
                Err(GuardError::MissingSelectedPrototype)
            };
        };
        let creature = creature_at(index);
        let ai = creatures
            .get(creature)
            .ai_value()
            .ok_or(GuardError::ZeroAiValue(creature))?
            .get();
        let rounded = arithmetic(value.checked_add(ai / 2), creature, "rounded guard value")?;
        let mut count = arithmetic(rounded.checked_div(ai), creature, "guard count division")?;
        let variation = count / 4 + 1;
        if let Some(variation) = u32::try_from(variation)
            .ok()
            .and_then(NonZeroU32::new)
            .filter(|value| value.get() > 1)
        {
            let first = i32::try_from(rng.below(variation)).expect("CRT draw fits i32");
            let second = i32::try_from(rng.below(variation)).expect("CRT draw fits i32");
            count = arithmetic(
                count.checked_add(first - second),
                creature,
                "varied guard count",
            )?;
        }
        Ok(Some(GuardStack {
            prototype,
            creature,
            count,
        }))
    }
}

fn creature_at(index: usize) -> CreatureId {
    CreatureId::parse(i32::try_from(index).expect("creature table index"))
        .expect("creature table identity")
}
fn arithmetic(
    value: Option<i32>,
    creature: CreatureId,
    operation: &'static str,
) -> Result<i32, GuardError> {
    value.ok_or(GuardError::Arithmetic {
        creature,
        operation,
    })
}
