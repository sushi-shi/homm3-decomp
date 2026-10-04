//! Explicit generation behavior; no process-wide compile-time switch.

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
    /// Uninitialized alignment-matching flag on generated water-zone templates.
    /// Explicitly independent of the general heap fill; false is the zero-fill baseline.
    pub water_guards_match_alignment: bool,
}
