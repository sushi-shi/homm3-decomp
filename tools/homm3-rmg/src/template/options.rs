//! Template policies shared by the generator's input formats.
//!
//! Extended defaults and column meanings follow the recovered 1.8.1 constructors
//! and reader (`hota-rmg` 3a1421ef5, `hota_rmg_templates.cpp`). Parsing a policy
//! does not imply that the generation stage consuming it has been implemented.

use crate::domain::Level;

/// Explicit override of a catalog's default availability.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Availability {
    /// Keep the catalog/profile default.
    #[default]
    Inherit,
    /// Admit this entry.
    Enabled,
    /// Exclude this entry.
    Disabled,
}

/// A road's relationship to a template connection.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum RoadPolicy {
    /// Let road generation decide.
    #[default]
    Automatic,
    /// Do not carry a road through this connection.
    Forbidden,
    /// Require a road through this connection.
    Required,
}

/// The connection mechanism requested by a template.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum ConnectionKind {
    /// Use the generation profile's ordinary connection attempts.
    #[default]
    Automatic,
    /// Direct land crossing.
    Ground,
    /// Underground gate pair.
    Underground,
    /// Teleport connection.
    Teleport,
    /// Select a mechanism during generation.
    Random,
}

/// Connection properties independent of its two endpoints and guard value.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ConnectionOptions {
    /// Road requirement.
    pub road: RoadPolicy,
    /// Connection mechanism.
    pub kind: ConnectionKind,
    /// A layout relationship without an ordinary physical connection.
    pub fictive: bool,
    /// Repel portal positions from the zone's other major entrances.
    pub portal_repulsion: bool,
}

/// Additional zone properties. Scripts remain byte strings until their owning
/// subsystem interprets them; resource text is not necessarily UTF-8.
#[derive(Clone, Debug, PartialEq, Eq)]
#[expect(
    clippy::struct_excessive_bools,
    reason = "independent template switches are not mutually exclusive states"
)]
pub struct ZoneOptions {
    /// Required world plane; absent lets layout choose.
    pub placement: Option<Level>,
    /// Zone object-definition overrides.
    pub objects: Option<Vec<u8>>,
    /// Minimum object counts.
    pub minimum_objects: Option<Vec<u8>>,
    /// Template-editor coordinates/settings, retained with their source zone.
    pub image_settings: Option<Vec<u8>>,
    /// Require neutral creatures.
    pub force_neutral_creatures: bool,
    /// Permit roads with disconnected components.
    pub allow_non_coherent_road: bool,
    /// Apply the zone repulsion constraint during layout.
    pub zone_repulsion: bool,
    /// Town relationships for the hint solver.
    pub town_hint: Option<Vec<u8>>,
    /// Native monster disposition ordinal (0..5).
    pub monster_disposition: u8,
    /// Explicit disposition strength (1..10) when the ordinal is custom.
    pub custom_monster_disposition: Option<u8>,
    /// Whether joining creatures require payment.
    pub monsters_join_only_for_money: bool,
    /// Joining fraction in percent.
    pub monsters_joining_percent: u8,
    /// Airship-yard placement count.
    pub minimum_airship_yards: i32,
    /// Airship-yard density; nonpositive disables density placement.
    pub airship_yard_density: i32,
    /// Terrain relationships for the hint solver.
    pub terrain_hint: Option<Vec<u8>>,
    /// Faction restrictions for the hint solver.
    pub allowed_factions: Option<Vec<u8>>,
    /// Faction relationships for the hint solver.
    pub faction_hint: Option<Vec<u8>>,
    /// Maximum blocking value; native -1 means unspecified.
    pub max_block_value: i32,
}
impl Default for ZoneOptions {
    fn default() -> Self {
        Self {
            placement: None,
            objects: None,
            minimum_objects: None,
            image_settings: None,
            force_neutral_creatures: false,
            allow_non_coherent_road: false,
            zone_repulsion: false,
            town_hint: None,
            monster_disposition: 3,
            custom_monster_disposition: None,
            monsters_join_only_for_money: true,
            monsters_joining_percent: 50,
            minimum_airship_yards: 0,
            airship_yard_density: 0,
            terrain_hint: None,
            allowed_factions: None,
            faction_hint: None,
            max_block_value: -1,
        }
    }
}

/// Map-wide options, separate from pack-wide lobby settings.
#[derive(Clone, Debug, PartialEq)]
pub struct MapOptions {
    /// Artifact availability overrides.
    pub artifacts: Option<Vec<u8>>,
    /// Combination-artifact availability overrides.
    pub combination_artifacts: Option<Vec<u8>>,
    /// Spell availability overrides.
    pub spells: Option<Vec<u8>>,
    /// Secondary-skill availability overrides.
    pub secondary_skills: Option<Vec<u8>>,
    /// Map-wide object-definition overrides.
    pub objects: Option<Vec<u8>>,
    /// Rock-block radius multiplier when explicitly enabled.
    pub rock_blocks: Option<f64>,
    /// Zone spacing multiplier.
    pub zone_sparseness: f64,
    /// Suppress special weeks.
    pub special_weeks_disabled: bool,
    /// Permit town spell research.
    pub spell_research: bool,
    /// Use the template's Anarchy policy.
    pub anarchy: bool,
}
impl Default for MapOptions {
    fn default() -> Self {
        Self {
            artifacts: None,
            combination_artifacts: None,
            spells: None,
            secondary_skills: None,
            objects: None,
            rock_blocks: None,
            zone_sparseness: 1.0,
            special_weeks_disabled: false,
            spell_research: true,
            anarchy: false,
        }
    }
}

// Three-state scanner of RVA 0x1c0360. A sign survives intervening non-digits
// until the first digit. Repeated signs replace it. IDs without signs are ignored.
pub(crate) fn apply_availability(text: &[u8], table: &mut [Availability]) -> Result<(), ()> {
    let mut sign = None;
    let mut id: Option<usize> = None;
    for byte in text.iter().copied().chain(std::iter::once(0)) {
        if byte.is_ascii_digit() {
            if sign.is_some() {
                let next = id
                    .unwrap_or(0)
                    .checked_mul(10)
                    .and_then(|n| n.checked_add(usize::from(byte - b'0')))
                    .ok_or(())?;
                if next > i32::MAX as usize {
                    return Err(());
                }
                id = Some(next);
            }
        } else {
            if let Some(index) = id.take() {
                if let Some(entry) = table.get_mut(index) {
                    *entry = sign.unwrap();
                }
                sign = None;
            }
            match byte {
                b'+' => sign = Some(Availability::Enabled),
                b'-' => sign = Some(Availability::Disabled),
                0 => break,
                _ => {}
            }
        }
    }
    Ok(())
}
