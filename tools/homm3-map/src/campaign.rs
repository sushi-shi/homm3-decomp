//! Allocation-free parser for the inflated header member of retail H3C files.

use core::fmt;

use super::{Cursor, Error as StreamError, MapString};

/// Maximum retail campaign region count (Unholy Alliance).
pub const MAX_SCENARIOS: usize = 12;
const REGION_COUNTS: [u8; 21] = [
    0, 3, 4, 3, 7, 4, 3, 3, 4, 4, 4, 4, 3, 8, 4, 5, 4, 4, 4, 12, 4,
];
const CREATURE_MASK_BYTES: usize = 19;

/// Retail H3C generation.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
#[repr(u32)]
pub enum Version {
    /// Original Restoration of Erathia 1.0/1.1 campaign.
    LegacyRestoration = 1,
    /// Restoration of Erathia campaign.
    Restoration = 4,
    /// Armageddon's Blade campaign.
    ArmageddonsBlade = 5,
    /// Shadow of Death / Complete campaign.
    ShadowOfDeath = 6,
}

impl Version {
    fn parse(value: u32) -> Result<Self, Error> {
        match value {
            1 => Ok(Self::LegacyRestoration),
            4 => Ok(Self::Restoration),
            5 => Ok(Self::ArmageddonsBlade),
            6 => Ok(Self::ShadowOfDeath),
            _ => Err(Error::UnsupportedVersion(value)),
        }
    }
}

/// Starting-option record associated with a scenario.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
pub enum StartOptions {
    /// No selection.
    None = 0,
    /// Select from typed resource/hero bonuses.
    Bonus = 1,
    /// Select heroes crossing over from earlier scenarios.
    HeroCrossover = 2,
    /// Select a starting hero.
    Hero = 3,
}

impl StartOptions {
    fn parse(value: u8, scenario: usize) -> Result<Self, Error> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::Bonus),
            2 => Ok(Self::HeroCrossover),
            3 => Ok(Self::Hero),
            _ => Err(Error::BadStartOptions { scenario, value }),
        }
    }
}

/// A malformed H3C header stream.
#[allow(missing_docs)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    Stream(StreamError),
    UnsupportedVersion(u32),
    UnknownRegionMap(u8),
    BadStartOptions { scenario: usize, value: u8 },
    BadBonusType { scenario: usize, value: u8 },
}

impl From<StreamError> for Error {
    fn from(value: StreamError) -> Self {
        Self::Stream(value)
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::Stream(error) => error.fmt(f),
            Self::UnsupportedVersion(version) => {
                write!(f, "campaign version {version} is not retail 1, 4, 5, or 6")
            }
            Self::UnknownRegionMap(region) => {
                write!(f, "campaign region map {region} has no retail region count")
            }
            Self::BadStartOptions { scenario, value } => {
                write!(f, "campaign scenario {scenario} uses start option {value}")
            }
            Self::BadBonusType { scenario, value } => {
                write!(f, "campaign scenario {scenario} uses bonus type {value}")
            }
        }
    }
}

impl core::error::Error for Error {}

/// Optional scenario prologue or epilogue.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Cutscene<'a> {
    /// Whether the record is present.
    pub present: bool,
    /// Video id when present.
    pub video: u8,
    /// Music id when present.
    pub music: u8,
    /// Subtitle bytes when present.
    pub text: MapString<'a>,
}

impl Cutscene<'_> {
    const EMPTY: Self = Self {
        present: false,
        video: 0,
        music: 0,
        text: MapString::EMPTY,
    };
}

/// One campaign scenario descriptor.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Scenario<'a> {
    /// Embedded map name; an empty name marks a void region.
    pub map_name: MapString<'a>,
    /// Compressed map-member size recorded by H3C.
    pub packed_map_size: u32,
    /// Prerequisite-region bitmask (one or two bytes by campaign size).
    pub prerequisites: u16,
    /// Campaign-region palette index.
    pub region_color: u8,
    /// Initial scenario difficulty.
    pub difficulty: u8,
    /// Region hover text.
    pub region_text: MapString<'a>,
    /// Optional prologue.
    pub prologue: Cutscene<'a>,
    /// Optional epilogue.
    pub epilogue: Cutscene<'a>,
    /// Five low bits select hero properties retained across scenarios.
    pub hero_keeps: u8,
    /// Packed creature carryover mask.
    pub creatures: &'a [u8],
    /// Packed artifact carryover mask.
    pub artifacts: &'a [u8],
    /// Scenario starting-option family.
    pub start_options: StartOptions,
    /// Player color stored only for [`StartOptions::Bonus`].
    pub player_color: Option<u8>,
    /// Number of variable bonus records.
    pub bonus_count: u8,
    /// Exact encoded bonus-record bytes.
    pub bonuses: &'a [u8],
}

impl Scenario<'_> {
    const EMPTY: Self = Self {
        map_name: MapString::EMPTY,
        packed_map_size: 0,
        prerequisites: 0,
        region_color: 0,
        difficulty: 0,
        region_text: MapString::EMPTY,
        prologue: Cutscene::EMPTY,
        epilogue: Cutscene::EMPTY,
        hero_keeps: 0,
        creatures: &[],
        artifacts: &[],
        start_options: StartOptions::None,
        player_color: None,
        bonus_count: 0,
        bonuses: &[],
    };

    /// Whether this region owns an embedded map member.
    #[must_use]
    pub const fn is_not_void(self) -> bool {
        !self.map_name.bytes().is_empty()
    }
}

/// A validated H3C header record, either direct or inflated from its member.
#[derive(Clone, Copy, Debug)]
pub struct Campaign<'a> {
    /// Campaign generation.
    pub version: Version,
    /// Region-map id used to obtain the scenario count.
    pub region_map: u8,
    /// Display name.
    pub name: MapString<'a>,
    /// Display description.
    pub description: MapString<'a>,
    /// Whether the player chooses campaign difficulty.
    pub variable_difficulty: bool,
    /// Campaign music id.
    pub music: u8,
    scenarios: [Scenario<'a>; MAX_SCENARIOS],
    scenario_count: usize,
    trailing: &'a [u8],
}

impl<'a> Campaign<'a> {
    /// Parse a retail H3C header record after its optional gzip envelope.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for unsupported campaign versions/regions, malformed
    /// variable records, unknown start options, or unknown typed bonuses.
    pub fn parse(data: &'a [u8]) -> Result<Self, Error> {
        let mut cursor = Cursor::new(data);
        let version = Version::parse(cursor.u32()?)?;
        let region_map = cursor.u8()?;
        let scenario_count = REGION_COUNTS
            .get(usize::from(region_map))
            .copied()
            .filter(|&count| count != 0)
            .map(usize::from)
            .ok_or(Error::UnknownRegionMap(region_map))?;
        let name = cursor.map_string()?;
        let description = cursor.map_string()?;
        let (variable_difficulty, music) = if version == Version::LegacyRestoration {
            (false, 0)
        } else {
            let variable_difficulty = if version == Version::Restoration {
                false
            } else {
                cursor.u8()? != 0
            };
            (variable_difficulty, cursor.u8()?)
        };

        let mut scenarios = [Scenario::EMPTY; MAX_SCENARIOS];
        for (index, scenario) in scenarios[..scenario_count].iter_mut().enumerate() {
            *scenario = if version == Version::LegacyRestoration {
                parse_legacy_scenario(&mut cursor)?
            } else {
                parse_scenario(&mut cursor, version, scenario_count, index)?
            };
        }
        let trailing = cursor.remaining();
        Ok(Self {
            version,
            region_map,
            name,
            description,
            variable_difficulty,
            music,
            scenarios,
            scenario_count,
            trailing,
        })
    }

    /// Number of regions/scenario descriptors.
    #[must_use]
    pub const fn scenario_count(self) -> usize {
        self.scenario_count
    }

    /// Return one scenario descriptor.
    #[must_use]
    pub fn scenario(self, index: usize) -> Option<Scenario<'a>> {
        self.scenarios
            .get(index)
            .copied()
            .filter(|_| index < self.scenario_count)
    }

    /// Iterate scenario descriptors in region order.
    pub fn iter(self) -> impl Iterator<Item = Scenario<'a>> {
        self.scenarios.into_iter().take(self.scenario_count)
    }

    /// Bytes after the last known scenario descriptor.
    #[must_use]
    pub const fn trailing(self) -> &'a [u8] {
        self.trailing
    }

    /// Number of non-void regions that must have following gzip map members.
    #[must_use]
    pub fn embedded_map_count(self) -> usize {
        self.iter()
            .filter(|scenario| scenario.is_not_void())
            .count()
    }
}

fn parse_legacy_scenario<'a>(cursor: &mut Cursor<'a>) -> Result<Scenario<'a>, Error> {
    let map_name = cursor.map_string()?;
    let packed_map_size = cursor.u32()?;
    let prerequisites = u16::from(cursor.u8()?);
    Ok(Scenario {
        map_name,
        packed_map_size,
        prerequisites,
        ..Scenario::EMPTY
    })
}

fn parse_scenario<'a>(
    cursor: &mut Cursor<'a>,
    version: Version,
    scenario_count: usize,
    index: usize,
) -> Result<Scenario<'a>, Error> {
    let map_name = cursor.map_string()?;
    let packed_map_size = cursor.u32()?;
    let prerequisites = if scenario_count > 8 {
        cursor.u16()?
    } else {
        u16::from(cursor.u8()?)
    };
    let region_color = cursor.u8()?;
    let difficulty = cursor.u8()?;
    let region_text = cursor.map_string()?;
    let prologue = parse_cutscene(cursor)?;
    let epilogue = parse_cutscene(cursor)?;
    let hero_keeps = cursor.u8()?;
    let creatures = cursor.take(CREATURE_MASK_BYTES)?;
    let artifact_bytes = if version < Version::ShadowOfDeath {
        17
    } else {
        18
    };
    let artifacts = cursor.take(artifact_bytes)?;
    let start_options = StartOptions::parse(cursor.u8()?, index)?;
    let player_color = if start_options == StartOptions::Bonus {
        Some(cursor.u8()?)
    } else {
        None
    };
    let bonus_count = if start_options == StartOptions::None {
        0
    } else {
        cursor.u8()?
    };
    let bonuses_start = cursor.position();
    for _ in 0..bonus_count {
        skip_bonus(cursor, start_options, index)?;
    }
    let bonuses = &cursor.data[bonuses_start..cursor.position()];
    Ok(Scenario {
        map_name,
        packed_map_size,
        prerequisites,
        region_color,
        difficulty,
        region_text,
        prologue,
        epilogue,
        hero_keeps,
        creatures,
        artifacts,
        start_options,
        player_color,
        bonus_count,
        bonuses,
    })
}

fn parse_cutscene<'a>(cursor: &mut Cursor<'a>) -> Result<Cutscene<'a>, Error> {
    if cursor.u8()? == 0 {
        return Ok(Cutscene::EMPTY);
    }
    Ok(Cutscene {
        present: true,
        video: cursor.u8()?,
        music: cursor.u8()?,
        text: cursor.map_string()?,
    })
}

fn skip_bonus(
    cursor: &mut Cursor<'_>,
    options: StartOptions,
    scenario: usize,
) -> Result<(), Error> {
    let size = match options {
        StartOptions::None => 0,
        StartOptions::HeroCrossover => 2,
        StartOptions::Hero => 3,
        StartOptions::Bonus => {
            let bonus_type = cursor.u8()?;
            match bonus_type {
                0 | 4 => 3,
                1 | 5 => 6,
                2 => 1,
                3 | 6 => 4,
                7 => 5,
                _ => {
                    return Err(Error::BadBonusType {
                        scenario,
                        value: bonus_type,
                    });
                }
            }
        }
    };
    let _ = cursor.take(size)?;
    Ok(())
}
