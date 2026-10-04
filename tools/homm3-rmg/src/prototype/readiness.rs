//! Mode-specific admission of prototype catalogs to generation.

use super::{ObjectKind, PrototypeCatalog};
use crate::{
    behavior::Behavior,
    domain::Terrain,
    raw,
    request::{Levels, MapVersion},
    traits::{ArtifactCatalog, ArtifactId},
};
use std::{error::Error, fmt};

/// A missing relationship that the C++ hotfix checks before generation.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum RequiredPrototypeError {
    /// A family indexed unconditionally by generation is empty.
    Family(ObjectKind),
    /// A town's direct slot is absent, has another subtype, or lacks a trigger.
    Town {
        /// Expected direct town slot/subtype.
        index: usize,
    },
    /// No usable portal families exist or there are fewer exits than entrances.
    Portals,
    /// An entrance and exit in corresponding slots have different subtypes.
    PortalPair {
        /// Direct entrance/exit slot.
        index: usize,
    },
    /// A tent colour has no guard selectable on dirt.
    BorderGuard {
        /// Tent's original colour subtype.
        subtype: i32,
    },
    /// Seer huts need a subtype-zero random artifact selectable on dirt.
    RandomArtifact,
    /// An eligible quest artifact lacks a corresponding concrete prototype.
    QuestArtifact(ArtifactId),
}
impl fmt::Display for RequiredPrototypeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Family(kind) => write!(f, "required prototype family {} is empty", kind.index()),
            Self::Town { index } => write!(
                f,
                "town prototype slot {index} is absent, mismatched or triggerless"
            ),
            Self::Portals => f.write_str("missing portal families or insufficient one-way exits"),
            Self::PortalPair { index } => write!(
                f,
                "portal entrance/exit slot {index} has different subtypes"
            ),
            Self::BorderGuard { subtype } => {
                write!(f, "tent colour {subtype} has no guard selectable on dirt")
            }
            Self::RandomArtifact => {
                f.write_str("seer huts need a random artifact selectable on dirt")
            }
            Self::QuestArtifact(id) => write!(f, "quest artifact {} has no prototype", id.index()),
        }
    }
}
impl Error for RequiredPrototypeError {}

/// Catalog admitted under its original behavior/version and a fixed plane count.
/// Hotfix relationships have been checked; retail retains deferred candidate
/// faults and reports them at the generation operation that would use them.
#[derive(Debug)]
pub struct GenerationPrototypes<'a> {
    catalog: PrototypeCatalog<'a>,
    levels: Levels,
}
impl GenerationPrototypes<'_> {
    /// Immutable prototype buckets in their original selection order.
    #[must_use]
    pub const fn catalog(&self) -> &PrototypeCatalog<'_> {
        &self.catalog
    }
    /// Map format used for filtering and required town slots.
    #[must_use]
    pub const fn version(&self) -> MapVersion {
        self.catalog.version
    }
    /// Behavior used for filtering and admission.
    #[must_use]
    pub const fn behavior(&self) -> Behavior {
        self.catalog.behavior
    }
    /// Plane count admitted for this generation.
    #[must_use]
    pub const fn levels(&self) -> Levels {
        self.levels
    }
}
impl<'a> PrototypeCatalog<'a> {
    /// Admit this catalog for generation, consuming the unadmitted value.
    /// The hotfix checks its required families and cross-family relationships;
    /// retail deliberately defers missing-prototype faults until actual use.
    /// No randomness or additional allocation is involved.
    /// The generation entry point must preserve the native constructor's earlier
    /// random-water draw before reporting this gate's failure and RNG state.
    ///
    /// # Errors
    /// Reports the first missing relationship in the native hotfix check order.
    pub fn into_generation(
        self,
        levels: Levels,
        artifacts: &ArtifactCatalog,
    ) -> Result<GenerationPrototypes<'a>, RequiredPrototypeError> {
        self.require_generation(levels, artifacts)?;
        Ok(GenerationPrototypes {
            catalog: self,
            levels,
        })
    }

    pub(crate) fn require_generation(
        &self,
        levels: Levels,
        artifacts: &ArtifactCatalog,
    ) -> Result<(), RequiredPrototypeError> {
        if self.behavior.is_hotfix() {
            self.required(levels, artifacts)?;
        }
        Ok(())
    }

    fn required(
        &self,
        levels: Levels,
        artifacts: &ArtifactCatalog,
    ) -> Result<(), RequiredPrototypeError> {
        for kind in [
            raw::MONSTER,
            raw::RANDOM_MONSTER,
            raw::TERRAIN_HOLE,
            raw::SHIPYARD,
        ] {
            let kind = known_kind(kind);
            if self.family(kind).is_empty() {
                return Err(RequiredPrototypeError::Family(kind));
            }
        }
        if levels == Levels::Underground
            && self.family(known_kind(raw::UNDERGROUND_GATE)).is_empty()
        {
            return Err(RequiredPrototypeError::Family(known_kind(
                raw::UNDERGROUND_GATE,
            )));
        }
        let towns = self.family(known_kind(raw::TOWN));
        let town_count = usize::try_from(if self.version == MapVersion::Restoration {
            raw::TOWN_CONFLUX
        } else {
            raw::TOWN_CONFLUX + 1
        })
        .expect("canonical town count");
        if towns.len() < town_count {
            return Err(RequiredPrototypeError::Town { index: towns.len() });
        }
        for (index, town) in towns[..town_count].iter().enumerate() {
            if usize::try_from(town.prototype.subtype) != Ok(index)
                || town.prototype.entrance.is_none()
            {
                return Err(RequiredPrototypeError::Town { index });
            }
        }
        let entrances = self.family(known_kind(raw::LITH_ONEWAY_ENTRANCE));
        let exits = self.family(known_kind(raw::LITH_ONEWAY_EXIT));
        if (self.family(known_kind(raw::LITH_TWOWAY)).is_empty() && entrances.is_empty())
            || exits.len() < entrances.len()
        {
            return Err(RequiredPrototypeError::Portals);
        }
        for (index, (entrance, exit)) in entrances.iter().zip(exits).enumerate() {
            if entrance.prototype.subtype != exit.prototype.subtype {
                return Err(RequiredPrototypeError::PortalPair { index });
            }
        }
        let selectable = |family, subtype| {
            self.family(known_kind(family))
                .iter()
                .any(|entry| entry.prototype.selectable(subtype, Terrain::Dirt))
        };
        for tent in self.family(known_kind(raw::BORDER_TENT)) {
            let subtype = tent.prototype.subtype;
            if !selectable(raw::BORDER_GUARD, subtype) {
                return Err(RequiredPrototypeError::BorderGuard { subtype });
            }
        }
        if !self.family(known_kind(raw::SEER)).is_empty() {
            if !selectable(raw::RANDOM_ARTIFACT, 0) {
                return Err(RequiredPrototypeError::RandomArtifact);
            }
            let prototypes = self.family(known_kind(raw::ARTIFACT));
            for (id, artifact) in artifacts.entries().iter().enumerate() {
                if artifact.quest_eligible()
                    && !prototypes
                        .iter()
                        .any(|entry| usize::try_from(entry.prototype.subtype) == Ok(id))
                {
                    return Err(RequiredPrototypeError::QuestArtifact(
                        ArtifactId::parse(i32::try_from(id).expect("artifact table count"))
                            .expect("artifact table identity"),
                    ));
                }
            }
        }
        Ok(())
    }
}

fn known_kind(value: u32) -> ObjectKind {
    ObjectKind::parse(i32::try_from(value).expect("canonical object kind"))
        .expect("canonical object kind")
}
