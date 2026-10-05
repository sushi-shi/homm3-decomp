//! Explicit generation behavior; no process-wide compile-time switch.

use crate::{raw, request::Town};

/// Allowed town factions, in native faction order; a zero mask allows none.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct TownMask(u16);
impl TownMask {
    /// No town factions, so selection consumes no random draw.
    pub const NONE: Self = Self(0);
    /// Every Complete-era faction.
    pub const ALL: Self = Self((1 << raw::TOWN_TYPE_COUNT) - 1);

    /// Admit only bits corresponding to known town factions.
    #[must_use]
    pub const fn parse(bits: u16) -> Option<Self> {
        if bits & !Self::ALL.0 == 0 {
            Some(Self(bits))
        } else {
            None
        }
    }
    /// Bit `town.index()` records whether that faction is allowed.
    #[must_use]
    pub const fn bits(self) -> u16 {
        self.0
    }
    /// Number of allowed factions.
    #[must_use]
    pub const fn count(self) -> u32 {
        self.0.count_ones()
    }
    /// Whether this faction is allowed.
    #[must_use]
    pub const fn contains(self, town: Town) -> bool {
        self.0 & (1 << town.index()) != 0
    }
}

/// Selected semantics for a complete generation.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Behavior {
    /// Replay retail's defined behavior under recorded compatibility inputs.
    Retail(RetailProfile),
    /// The C++ `HOMM3_RMG_HOTFIX` behavior at the implementation base.
    Hotfix,
}

impl Behavior {
    /// Whether input preparation and generation use the current hotfix policy.
    #[must_use]
    pub const fn is_hotfix(self) -> bool {
        matches!(self, Self::Hotfix)
    }
}

/// Recorded memory inputs to the existing retail oracle.
///
/// This is deterministic input, never permission to access uninitialized memory.
/// Native, uncontrolled heap contents are deliberately not representable here.
/// Further observed residue effects must be given named inputs when modeled;
/// these two fills alone do not claim to describe every possible retail state.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct RetailProfile {
    /// Word filling the oracle entry stack; zero is the default oracle profile.
    pub stack_word: u32,
    /// Byte filling oracle allocations; zero is the default oracle profile.
    pub heap_byte: u8,
    /// Allowed-town residue on generated water-zone templates. None follows
    /// the oracle allocation fill: zero allows none, a nonzero byte allows all.
    /// An explicit mask can replay different observed residue or captures from
    /// an earlier reconstruction that enabled all towns under a zero heap fill.
    pub water_zone_towns: Option<TownMask>,
    /// Uninitialized alignment-matching flag on generated water-zone templates.
    /// Explicitly independent of the general heap fill; false is the zero-fill baseline.
    pub water_guards_match_alignment: bool,
    /// Initial unwritten key-tent cursor, independent of general stack/heap fills.
    /// Any signed subtype is meaningful for prototype lookup; availability is
    /// checked only when reserving a successfully placed tent. None requests a
    /// typed fault at the first cursor read, unless a rescan initialized it first.
    pub initial_key_tent_color: Option<i32>,
}

impl RetailProfile {
    /// Resolve the water-zone town mask without consuming randomness.
    #[must_use]
    pub const fn water_zone_town_mask(self) -> TownMask {
        match self.water_zone_towns {
            Some(mask) => mask,
            None if self.heap_byte == 0 => TownMask::NONE,
            None => TownMask::ALL,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn town_masks_reject_unknown_factions_and_resolve_heap_defaults() {
        assert_eq!(TownMask::parse(0), Some(TownMask::NONE));
        assert_eq!(TownMask::parse(511), Some(TownMask::ALL));
        assert!(TownMask::parse(512).is_none());
        assert!(TownMask::parse(u16::MAX).is_none());
        let sparse = TownMask::parse(0b1_0000_0010).unwrap();
        assert_eq!(sparse.count(), 2);
        assert!(sparse.contains(Town::parse(8).unwrap()));
        assert!(!sparse.contains(Town::parse(0).unwrap()));
        for heap_byte in [0, 91, 255] {
            let mut profile = RetailProfile {
                heap_byte,
                ..RetailProfile::default()
            };
            assert_eq!(
                profile.water_zone_town_mask(),
                if heap_byte == 0 {
                    TownMask::NONE
                } else {
                    TownMask::ALL
                }
            );
            profile.water_zone_towns = Some(sparse);
            assert_eq!(profile.water_zone_town_mask(), sparse);
        }
    }
}
