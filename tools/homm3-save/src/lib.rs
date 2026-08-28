#![no_std]
#![forbid(unsafe_code)]
//! Allocation-free validation of inflated Heroes III save streams.
//!
//! Retail Complete accepts versions 16--18 and 25--42. Every accepted
//! revision is parsed using the version gates in the retail loaders. Gzip is
//! an outer transport concern and is intentionally left to the `std` oracle
//! binary.

use core::fmt;

/// Current save-format version emitted by retail Complete.
pub const VERSION: i32 = 42;
/// Fixed byte count of the pre-version-28 campaign snapshot.
pub const LEGACY_CAMPAIGN_SIZE: usize = 0x66a9;
/// Number of player records in a save.
pub const PLAYER_COUNT: usize = 8;
/// Number of hero records in Complete.
pub const HERO_COUNT: usize = 156;
/// Serialized size of one current player record.
pub const PLAYER_SIZE: usize = 146;
/// Fixed bytes in one current hero record, excluding custom-name bytes.
pub const HERO_FIXED_SIZE: usize = 1_094;
/// Serialized size of the current setup-options record.
pub const SETUP_SIZE: usize = 436;

/// Save family selected by the eight-byte file identifier.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Kind {
    /// Ordinary scenario save (`H3SVG`).
    Game,
    /// Campaign save (`H3SVC`).
    Campaign,
}

/// A variable-count field used to identify malformed extents.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CountField {
    /// Campaign score rows.
    CampaignScores,
    /// Campaign carry-over pools.
    CampaignPools,
    /// Heroes in a carry-over pool.
    CampaignHeroes,
    /// Artifacts in a carry-over pool.
    CampaignArtifacts,
    /// Campaign word-vector tail.
    CampaignWords,
    /// Saved custom hero setup rows.
    HeroSetups,
    /// Saved rumors.
    Rumours,
    /// Black-market records.
    BlackMarkets,
    /// Per-cell object references.
    CellObjects,
    /// Saved object templates.
    ObjectTypes,
    /// Saved placed objects.
    Objects,
    /// Pandora's-box records.
    BlackBoxes,
    /// Custom treasure records.
    Treasures,
    /// Custom monster records.
    Monsters,
    /// Seer-hut records.
    SeerHuts,
    /// Quest-guard records.
    QuestGuards,
    /// Secondary skills in a Pandora's box.
    BlackBoxSkills,
    /// Artifacts in a Pandora's box.
    BlackBoxArtifacts,
    /// Spells in a Pandora's box.
    BlackBoxSpells,
    /// Creature stacks in a Pandora's box.
    BlackBoxCreatures,
    /// Artifact requirements in a quest.
    QuestArtifacts,
    /// Creature requirements in a quest.
    QuestCreatures,
    /// Global timed events.
    TimedEvents,
    /// Town timed events.
    TownEvents,
    /// Sign records.
    Signs,
    /// Mine records.
    Mines,
    /// Generator records.
    Generators,
    /// Garrison records.
    Garrisons,
    /// Boat records.
    Boats,
    /// Town records.
    Towns,
    /// Packed-point vector records.
    Points,
    /// University records.
    Universities,
    /// Creature-bank records.
    CreatureBanks,
    /// Artifacts in a creature bank.
    CreatureBankArtifacts,
    /// Recorded actions.
    RecordedEvents,
    /// Visibility changes in a recorded shroud action.
    ShroudChanges,
}

/// A malformed or unsupported inflated save stream.
#[allow(missing_docs)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    Short {
        offset: usize,
        needed: usize,
        available: usize,
    },
    BadMagic([u8; 8]),
    UnsupportedVersion(i32),
    UnsupportedMapVersion(i32),
    BadMapSize(i32),
    BadCount {
        offset: usize,
        value: i32,
        field: CountField,
    },
    SizeOverflow {
        offset: usize,
    },
    BadVictoryType {
        offset: usize,
        value: u8,
    },
    BadLossType {
        offset: usize,
        value: u8,
    },
    BadObjectType {
        offset: usize,
        value: u16,
    },
    BadObjectTypeIndex {
        offset: usize,
        value: u16,
        count: usize,
    },
    BadQuestType {
        offset: usize,
        value: u8,
    },
    BadRecordedEventType {
        offset: usize,
        value: u8,
    },
    TrailingBytes {
        offset: usize,
        count: usize,
    },
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::Short {
                offset,
                needed,
                available,
            } => write!(
                f,
                "save field at {offset:#x} needs {needed} bytes, only {available} remain"
            ),
            Self::BadMagic(magic) => write!(f, "unrecognized save identifier {magic:?}"),
            Self::UnsupportedVersion(version) => write!(
                f,
                "save version {version} is not accepted by retail Complete"
            ),
            Self::UnsupportedMapVersion(version) => {
                write!(f, "saved map version {version} is not 14, 21, or 28")
            }
            Self::BadMapSize(size) => write!(f, "saved map size {size} is invalid"),
            Self::BadCount {
                offset,
                value,
                field,
            } => write!(f, "save {field:?} count {value} at {offset:#x} is invalid"),
            Self::SizeOverflow { offset } => {
                write!(f, "save extent at {offset:#x} overflows the host")
            }
            Self::BadVictoryType { offset, value } => {
                write!(f, "saved victory type {value} at {offset:#x} is invalid")
            }
            Self::BadLossType { offset, value } => {
                write!(f, "saved loss type {value} at {offset:#x} is invalid")
            }
            Self::BadObjectType { offset, value } => write!(
                f,
                "saved object type {value} at {offset:#x} is outside the 232-row retail table"
            ),
            Self::BadObjectTypeIndex {
                offset,
                value,
                count,
            } => write!(
                f,
                "saved object type index {value} at {offset:#x} is outside {count} templates"
            ),
            Self::BadQuestType { offset, value } => {
                write!(f, "saved quest type {value} at {offset:#x} is invalid")
            }
            Self::BadRecordedEventType { offset, value } => write!(
                f,
                "saved recorded-event type {value} at {offset:#x} is invalid"
            ),
            Self::TrailingBytes { offset, count } => {
                write!(f, "save has {count} trailing bytes at {offset:#x}")
            }
        }
    }
}

impl core::error::Error for Error {}

/// Structural counts collected while validating a save.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Counts {
    /// Serialized map cells.
    pub cells: usize,
    /// Extra object references attached to cells.
    pub cell_object_references: usize,
    /// Saved object templates.
    pub object_types: usize,
    /// Saved placed objects.
    pub objects: usize,
    /// Saved global timed events.
    pub timed_events: usize,
    /// Saved town timed events.
    pub town_events: usize,
    /// Saved towns.
    pub towns: usize,
    /// Fixed current hero records.
    pub heroes: usize,
    /// Campaign carry-over hero records.
    pub campaign_heroes: usize,
    /// Recorded replay actions.
    pub recorded_events: usize,
}

/// A completely validated inflated retail-compatible save stream.
#[derive(Clone, Copy, Debug)]
pub struct SaveGame<'a> {
    bytes: &'a [u8],
    /// Ordinary or campaign save family.
    pub kind: Kind,
    /// Save protocol version.
    pub version: i32,
    /// Game-version scalar stored in the save header.
    pub game_version: i32,
    /// Embedded scenario-map format version.
    pub map_version: i32,
    /// Square map side length.
    pub map_size: i32,
    /// Whether an underground layer is serialized.
    pub two_layers: bool,
    /// Structural counts collected during validation.
    pub counts: Counts,
}

impl<'a> SaveGame<'a> {
    /// Parse and exhaustively consume one inflated retail-compatible save.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for truncation, invalid extents, unsupported
    /// rejected versions, invalid discriminants, or trailing bytes.
    pub fn parse(bytes: &'a [u8]) -> Result<Self, Error> {
        let mut input = Cursor::new(bytes);
        let magic_offset = input.position();
        let magic_bytes = input.take(8)?;
        let mut magic = [0_u8; 8];
        magic.copy_from_slice(magic_bytes);
        let kind = match &magic {
            b"H3SVG\0\0\0" => Kind::Game,
            b"H3SVC\0\0\0" => Kind::Campaign,
            _ => {
                let _ = magic_offset;
                return Err(Error::BadMagic(magic));
            }
        };

        let version = input.i32()?;
        if !is_supported_version(version) {
            return Err(Error::UnsupportedVersion(version));
        }
        let game_version = if version >= 40 {
            input.i32()?
        } else if version <= 18 {
            0
        } else if version <= 30 {
            1
        } else {
            2
        };
        input.skip(32)?;

        let (map_version, map_size, two_layers) = parse_map_header(&mut input, version)?;
        input.skip(SETUP_SIZE)?;

        let campaign_flag = input.u16()?;
        let mut counts = Counts::default();
        if campaign_flag != 0 {
            parse_campaign(&mut input, version, &mut counts)?;
        }

        input.skip(351 + 2 + 1 + 8 + 32 + 4)?;
        parse_game_body(&mut input, version, map_size, two_layers, &mut counts)?;

        if input.remaining() != 0 {
            return Err(Error::TrailingBytes {
                offset: input.position(),
                count: input.remaining(),
            });
        }

        Ok(Self {
            bytes,
            kind,
            version,
            game_version,
            map_version,
            map_size,
            two_layers,
            counts,
        })
    }

    /// Complete inflated bytes covered by this parse.
    #[must_use]
    pub const fn bytes(&self) -> &'a [u8] {
        self.bytes
    }
}

/// Whether the retail Complete loader accepts this save revision.
#[must_use]
pub const fn is_supported_version(version: i32) -> bool {
    matches!(version, 16..=18 | 25..=42)
}

fn parse_map_header(input: &mut Cursor<'_>, save_version: i32) -> Result<(i32, i32, bool), Error> {
    let map_version = input.i32()?;
    if !matches!(map_version, 14 | 21 | 28) {
        return Err(Error::UnsupportedMapVersion(map_version));
    }
    input.skip(1)?;
    let size = input.i32()?;
    if size <= 0 {
        return Err(Error::BadMapSize(size));
    }
    let two_layers = input.u8()? != 0;
    input.string16()?;
    input.string16()?;
    input.skip(1)?;
    if save_version >= 27 {
        input.skip(1)?;
    }

    for _ in 0..PLAYER_COUNT {
        input.skip(3)?;
        input.skip(if save_version < 28 { 1 } else { 2 })?;
        input.skip(1)?;
        let generate_hero = input.u8()? != 0;
        if generate_hero {
            input.skip(3)?;
        }
        let hero_id = input.u8()?;
        if hero_id != 0xff {
            input.skip(1)?;
            input.string16()?;
        }
    }

    let victory_offset = input.position();
    let victory = input.u8()?;
    if victory != 0xff {
        input.skip(2)?;
        let payload = match victory {
            0 | 5 => 1,
            1..=3 => 5,
            4 | 6 | 7 => 3,
            8 | 9 => 4,
            _ => {
                return Err(Error::BadVictoryType {
                    offset: victory_offset,
                    value: victory,
                });
            }
        };
        input.skip(payload)?;
    }

    let loss_offset = input.position();
    let loss = input.u8()?;
    if loss != 0xff {
        let payload = match loss {
            0 => 3,
            1 if save_version == 16 => 3,
            1 | 2 => 2,
            _ => {
                return Err(Error::BadLossType {
                    offset: loss_offset,
                    value: loss,
                });
            }
        };
        input.skip(payload)?;
    }

    let teams = input.u8()?;
    if teams != 0 {
        input.skip(PLAYER_COUNT)?;
    }

    if save_version >= 30 {
        let setup_count = usize::from(input.u8()?);
        for _ in 0..setup_count {
            input.skip(2)?;
            input.string32()?;
            if save_version >= 31 {
                input.skip(1)?;
            }
        }
    }
    Ok((map_version, size, two_layers))
}

fn parse_campaign(
    input: &mut Cursor<'_>,
    save_version: i32,
    counts: &mut Counts,
) -> Result<(), Error> {
    if save_version < 28 {
        return input.skip(LEGACY_CAMPAIGN_SIZE);
    }

    input.skip(1)?;
    if save_version >= 26 {
        input.skip(1)?;
    }
    input.skip(5)?;
    input.string32()?;
    input.skip(if save_version >= 36 { 21 } else { 14 })?;

    let score_count = usize::from(input.u8()?);
    input.skip_mul(score_count, 11)?;

    let pool_count = usize::from(input.u8()?);
    for _ in 0..pool_count {
        let hero_count = usize::from(input.u8()?);
        counts.campaign_heroes =
            counts
                .campaign_heroes
                .checked_add(hero_count)
                .ok_or(Error::SizeOverflow {
                    offset: input.position(),
                })?;
        for _ in 0..hero_count {
            parse_hero(input, save_version)?;
        }
        let artifact_count = usize::from(input.u16()?);
        input.skip_mul(artifact_count, 4)?;
    }

    let word_count = usize::from(input.u8()?);
    input.skip_mul(word_count, 2)?;
    Ok(())
}

fn parse_game_body(
    input: &mut Cursor<'_>,
    save_version: i32,
    map_size: i32,
    two_layers: bool,
    counts: &mut Counts,
) -> Result<(), Error> {
    if save_version >= 41 {
        input.skip(1)?;
    }
    if save_version >= 34 {
        input.skip(0x90 + 0x90)?;
    } else if save_version >= 25 {
        input.skip(0x81 + 0x81)?;
    }
    if save_version >= 29 {
        input.skip(0x1c)?;
    }
    parse_rumours(input)?;

    let markets = usize::from(input.u8()?);
    input.skip_mul(markets, 0x1c)?;

    parse_world(input, save_version, map_size, two_layers, counts)?;
    parse_object_pools(input, save_version)?;

    input.skip(1 + 0x30)?;
    let player_size = if save_version >= 37 {
        PLAYER_SIZE
    } else {
        PLAYER_SIZE - 2
    };
    input.skip_mul(PLAYER_COUNT, player_size)?;

    let town_count = usize::from(input.u8()?);
    counts.towns = town_count;
    for _ in 0..town_count {
        parse_town(input, save_version)?;
    }

    let hero_count = if save_version < 25 { 128 } else { HERO_COUNT };
    for _ in 0..hero_count {
        parse_hero(input, save_version)?;
    }
    counts.heroes = hero_count;

    if save_version < 31 {
        input.skip(PLAYER_COUNT)?;
    }
    input.skip(hero_count)?;
    if save_version >= 31 {
        input.skip(HERO_COUNT)?;
    }
    input.skip(17 + 109 + 4)?;

    let side = usize::try_from(map_size).map_err(|_| Error::BadMapSize(map_size))?;
    let layers = if two_layers { 2_usize } else { 1_usize };
    let map_extra = side
        .checked_mul(side)
        .and_then(|cells| cells.checked_mul(layers))
        .and_then(|cells| cells.checked_mul(2))
        .ok_or(Error::SizeOverflow {
            offset: input.position(),
        })?;
    input.skip(map_extra)?;

    let point_vector_count = if save_version < 32 { 9 } else { 19 };
    for _ in 0..point_vector_count {
        parse_block_i16(input, 4, CountField::Points)?;
    }
    parse_block_i16(input, 16, CountField::Universities)?;
    parse_creature_banks(input)?;
    parse_recorded_events(input, save_version, counts)?;
    Ok(())
}

fn parse_rumours(input: &mut Cursor<'_>) -> Result<(), Error> {
    input.string16()?;
    input.skip(0x100)?;
    let count = input.count_i32(CountField::Rumours)?;
    for _ in 0..count {
        input.string16()?;
        input.skip(1)?;
    }
    Ok(())
}

fn parse_world(
    input: &mut Cursor<'_>,
    save_version: i32,
    map_size: i32,
    two_layers: bool,
    counts: &mut Counts,
) -> Result<(), Error> {
    let side = usize::try_from(map_size).map_err(|_| Error::BadMapSize(map_size))?;
    let layers = if two_layers { 2_usize } else { 1_usize };
    let cell_count = side
        .checked_mul(side)
        .and_then(|cells| cells.checked_mul(layers))
        .ok_or(Error::SizeOverflow {
            offset: input.position(),
        })?;
    counts.cells = cell_count;
    for _ in 0..cell_count {
        input.skip(18)?;
        let attached = input.count_i32(CountField::CellObjects)?;
        counts.cell_object_references =
            counts
                .cell_object_references
                .checked_add(attached)
                .ok_or(Error::SizeOverflow {
                    offset: input.position(),
                })?;
        input.skip_mul(attached, 4)?;
    }

    let type_count = input.count_i32(CountField::ObjectTypes)?;
    counts.object_types = type_count;
    for _ in 0..type_count {
        input.string16()?;
        input.skip(2 + 24)?;
        let type_offset = input.position();
        let object_type = input.u16()?;
        if object_type >= 232 {
            return Err(Error::BadObjectType {
                offset: type_offset,
                value: object_type,
            });
        }
        input.skip(4 + 1)?;
    }

    let object_count = input.count_i32(CountField::Objects)?;
    counts.objects = object_count;
    for _ in 0..object_count {
        input.skip(3)?;
        let index_offset = input.position();
        let type_index = input.u16()?;
        if usize::from(type_index) >= type_count {
            return Err(Error::BadObjectTypeIndex {
                offset: index_offset,
                value: type_index,
                count: type_count,
            });
        }
    }

    let black_boxes = input.count_i16(CountField::BlackBoxes)?;
    for _ in 0..black_boxes {
        parse_black_box(input, save_version)?;
    }
    let treasures = input.count_i16(CountField::Treasures)?;
    for _ in 0..treasures {
        parse_treasure(input)?;
    }
    let monsters = input.count_i16(CountField::Monsters)?;
    for _ in 0..monsters {
        input.string16()?;
        input.skip(28 + 1)?;
    }
    let seer_huts = input.count_i16(CountField::SeerHuts)?;
    for _ in 0..seer_huts {
        parse_quest_holder(input, save_version, true)?;
    }
    if save_version >= 25 {
        let quest_guards = input.count_i16(CountField::QuestGuards)?;
        for _ in 0..quest_guards {
            parse_quest_holder(input, save_version, false)?;
        }
    }

    let timed_events = input.count_i32(CountField::TimedEvents)?;
    counts.timed_events = timed_events;
    for _ in 0..timed_events {
        parse_timed_event(input, save_version)?;
    }
    let town_events = input.count_i32(CountField::TownEvents)?;
    counts.town_events = town_events;
    for _ in 0..town_events {
        parse_timed_event(input, save_version)?;
        input.skip(1 + 8 + 14)?;
    }
    Ok(())
}

fn parse_black_box(input: &mut Cursor<'_>, save_version: i32) -> Result<(), Error> {
    if input.u8()? != 0 {
        parse_treasure(input)?;
    }
    input.skip(8 + 2 + 28 + 4)?;
    let skills = usize::from(input.u8()?);
    input.skip_mul(skills, 2)?;
    let artifacts = usize::from(input.u8()?);
    input.skip(artifacts)?;
    let spells = usize::from(input.u8()?);
    input.skip(spells)?;
    let creatures = usize::from(input.u8()?);
    input.skip_mul(creatures, if save_version < 25 { 3 } else { 4 })?;
    Ok(())
}

fn parse_treasure(input: &mut Cursor<'_>) -> Result<(), Error> {
    input.string16()?;
    if input.u8()? != 0 {
        input.skip(56)?;
    }
    Ok(())
}

fn parse_quest_holder(
    input: &mut Cursor<'_>,
    save_version: i32,
    seer_hut: bool,
) -> Result<(), Error> {
    let type_offset = input.position();
    let quest_type = input.u8()?;
    if quest_type != 0 {
        parse_quest(input, save_version, quest_type, type_offset)?;
    }
    if seer_hut {
        input.skip(12 + 3)?;
    } else {
        input.skip(1)?;
    }
    Ok(())
}

fn parse_quest(
    input: &mut Cursor<'_>,
    save_version: i32,
    quest_type: u8,
    type_offset: usize,
) -> Result<(), Error> {
    match quest_type {
        1 | 8 => input.skip(2)?,
        2 => input.skip(4)?,
        3 => {
            input.skip(2)?;
            if save_version == 30 || save_version >= 36 {
                input.skip(1)?;
            }
        }
        4 => input.skip(7)?,
        5 => {
            let count = usize::from(input.u8()?);
            input.skip_mul(count, 2)?;
        }
        6 => {
            let count = usize::from(input.u8()?);
            input.skip_mul(count, 6)?;
        }
        7 => input.skip(28)?,
        9 => input.skip(1)?,
        _ => {
            return Err(Error::BadQuestType {
                offset: type_offset,
                value: quest_type,
            });
        }
    }
    input.skip(1 + 1 + 4)?;
    input.string32()?;
    input.string32()?;
    input.string32()?;
    Ok(())
}

fn parse_timed_event(input: &mut Cursor<'_>, save_version: i32) -> Result<(), Error> {
    input.string16()?;
    input.skip(28 + 1)?;
    if save_version >= 42 {
        input.skip(1)?;
    }
    input.skip(1 + 2 + 2)?;
    Ok(())
}

fn parse_object_pools(input: &mut Cursor<'_>, save_version: i32) -> Result<(), Error> {
    let signs = usize::from(input.u8()?);
    for _ in 0..signs {
        input.string16()?;
        input.skip(1)?;
    }

    let mines = usize::from(input.u8()?);
    input.skip_mul(mines, if save_version < 25 { 8 } else { 62 })?;

    let generators = input.count_i16(CountField::Generators)?;
    input.skip_mul(generators, 75)?;

    let garrisons = usize::from(input.u8()?);
    input.skip_mul(garrisons, if save_version < 28 { 60 } else { 61 })?;

    let boats = usize::from(input.u8()?);
    input.skip_mul(boats, 28)?;
    Ok(())
}

fn parse_town(input: &mut Cursor<'_>, save_version: i32) -> Result<(), Error> {
    input.skip(10 + 56 + 4)?;
    if save_version < 25 {
        input.skip(13)?;
    } else {
        input.string16()?;
    }
    input.skip(28 + 56 + 5 + 24 + 120 + 70 + 1 + 4 + 2)?;
    Ok(())
}

fn parse_hero(input: &mut Cursor<'_>, save_version: i32) -> Result<(), Error> {
    input.skip(20)?;
    if save_version >= 25 {
        input.skip(2)?;
        input.string32()?;
    }
    input.skip(19 + 34 + 60 + 56)?;
    input.skip(13 + 28 + 28 + 4 + 70 + 70)?;
    input.skip(if save_version <= 30 { 18 * 8 } else { 19 * 8 })?;
    input.skip(64 * 8)?;
    if save_version >= 32 {
        input.skip(15)?;
    }
    input.skip(1 + 6)?;
    Ok(())
}

fn parse_block_i16(input: &mut Cursor<'_>, width: usize, field: CountField) -> Result<(), Error> {
    let count = input.count_i16(field)?;
    input.skip_mul(count, width)
}

fn parse_creature_banks(input: &mut Cursor<'_>) -> Result<(), Error> {
    let count = input.count_i16(CountField::CreatureBanks)?;
    for _ in 0..count {
        input.skip(56 + 28 + 4 + 1)?;
        let artifacts = input.count_i16(CountField::CreatureBankArtifacts)?;
        input.skip_mul(artifacts, 4)?;
    }
    Ok(())
}

fn parse_recorded_events(
    input: &mut Cursor<'_>,
    save_version: i32,
    counts: &mut Counts,
) -> Result<(), Error> {
    let count = input.count_i32(CountField::RecordedEvents)?;
    counts.recorded_events = count;
    for _ in 0..count {
        let type_offset = input.position();
        let event_type = input.u8()?;
        match event_type {
            1 | 2 => input.skip(14)?,
            // Event 8 carries a dword hero id, unlike hero::save's
            // narrowed id, giving it the same seven-byte payload width.
            3 | 4 | 8 => input.skip(7)?,
            5 => input.skip(recorded_boat_payload_size(save_version))?,
            6 => input.skip(recorded_boat_payload_size(save_version) + 8)?,
            7 | 9 => input.skip(17)?,
            10 => input.skip(2)?,
            11 => {
                input.skip(1)?;
                let changes = input.count_i16(CountField::ShroudChanges)?;
                input.skip_mul(changes, 8)?;
            }
            _ => {
                return Err(Error::BadRecordedEventType {
                    offset: type_offset,
                    value: event_type,
                });
            }
        }
    }
    Ok(())
}

const fn recorded_boat_payload_size(save_version: i32) -> usize {
    if save_version >= 18 && save_version != 28 && (save_version <= 30 || save_version >= 35) {
        8
    } else {
        2
    }
}

#[derive(Clone, Copy)]
struct Cursor<'a> {
    bytes: &'a [u8],
    offset: usize,
}

impl<'a> Cursor<'a> {
    const fn new(bytes: &'a [u8]) -> Self {
        Self { bytes, offset: 0 }
    }

    const fn position(self) -> usize {
        self.offset
    }

    const fn remaining(self) -> usize {
        self.bytes.len() - self.offset
    }

    fn take(&mut self, count: usize) -> Result<&'a [u8], Error> {
        let start = self.offset;
        let end = start
            .checked_add(count)
            .ok_or(Error::SizeOverflow { offset: start })?;
        let bytes = self.bytes.get(start..end).ok_or(Error::Short {
            offset: start,
            needed: count,
            available: self.bytes.len().saturating_sub(start),
        })?;
        self.offset = end;
        Ok(bytes)
    }

    fn skip(&mut self, count: usize) -> Result<(), Error> {
        self.take(count).map(|_| ())
    }

    fn skip_mul(&mut self, count: usize, width: usize) -> Result<(), Error> {
        let bytes = count.checked_mul(width).ok_or(Error::SizeOverflow {
            offset: self.offset,
        })?;
        self.skip(bytes)
    }

    fn u8(&mut self) -> Result<u8, Error> {
        Ok(self.take(1)?[0])
    }

    fn u16(&mut self) -> Result<u16, Error> {
        let bytes = self.take(2)?;
        Ok(u16::from_le_bytes([bytes[0], bytes[1]]))
    }

    fn i16(&mut self) -> Result<i16, Error> {
        let bytes = self.take(2)?;
        Ok(i16::from_le_bytes([bytes[0], bytes[1]]))
    }

    fn i32(&mut self) -> Result<i32, Error> {
        let bytes = self.take(4)?;
        Ok(i32::from_le_bytes([bytes[0], bytes[1], bytes[2], bytes[3]]))
    }

    fn count_i16(&mut self, field: CountField) -> Result<usize, Error> {
        let offset = self.position();
        let value = self.i16()?;
        usize::try_from(value).map_err(|_| Error::BadCount {
            offset,
            value: i32::from(value),
            field,
        })
    }

    fn count_i32(&mut self, field: CountField) -> Result<usize, Error> {
        let offset = self.position();
        let value = self.i32()?;
        usize::try_from(value).map_err(|_| Error::BadCount {
            offset,
            value,
            field,
        })
    }

    fn string16(&mut self) -> Result<(), Error> {
        let length = self.i16()?;
        if length > 0 {
            self.skip(usize::try_from(length).map_err(|_| Error::SizeOverflow {
                offset: self.offset,
            })?)?;
        }
        Ok(())
    }

    fn string32(&mut self) -> Result<(), Error> {
        let length = self.i32()?;
        if length > 0 {
            self.skip(usize::try_from(length).map_err(|_| Error::SizeOverflow {
                offset: self.offset,
            })?)?;
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    extern crate std;

    use std::{vec, vec::Vec};

    use super::*;

    fn zeros(out: &mut Vec<u8>, count: usize) {
        out.resize(out.len() + count, 0);
    }

    fn word(out: &mut Vec<u8>, value: i16) {
        out.extend_from_slice(&value.to_le_bytes());
    }

    fn dword(out: &mut Vec<u8>, value: i32) {
        out.extend_from_slice(&value.to_le_bytes());
    }

    fn empty_hero(out: &mut Vec<u8>, save_version: i32) {
        zeros(out, 20);
        if save_version >= 25 {
            zeros(out, 2);
            dword(out, 0);
        }
        zeros(out, 19 + 34 + 60 + 56);
        zeros(out, 13 + 28 + 28 + 4 + 70 + 70);
        zeros(out, if save_version <= 30 { 18 * 8 } else { 19 * 8 });
        zeros(out, 64 * 8);
        if save_version >= 32 {
            zeros(out, 15);
        }
        zeros(out, 1 + 6);
    }

    fn minimal_save(save_version: i32) -> Vec<u8> {
        assert!(is_supported_version(save_version));
        let mut out = Vec::new();
        out.extend_from_slice(b"H3SVG\0\0\0");
        dword(&mut out, save_version);
        if save_version >= 40 {
            dword(&mut out, 0);
        }
        zeros(&mut out, 32);

        dword(&mut out, 28);
        zeros(&mut out, 1);
        dword(&mut out, 1);
        zeros(&mut out, 1);
        word(&mut out, 0);
        word(&mut out, 0);
        zeros(&mut out, 1);
        if save_version >= 27 {
            zeros(&mut out, 1);
        }
        for _ in 0..PLAYER_COUNT {
            zeros(&mut out, 3 + if save_version < 28 { 1 } else { 2 } + 1 + 1);
            out.push(0xff);
        }
        out.extend_from_slice(&[0xff, 0xff, 0]);
        if save_version >= 30 {
            out.push(0);
        }

        zeros(&mut out, SETUP_SIZE);
        word(&mut out, 0);
        zeros(&mut out, 351 + 2 + 1 + 8 + 32 + 4);

        if save_version >= 41 {
            zeros(&mut out, 1);
        }
        if save_version >= 34 {
            zeros(&mut out, 0x90 + 0x90);
        } else if save_version >= 25 {
            zeros(&mut out, 0x81 + 0x81);
        }
        if save_version >= 29 {
            zeros(&mut out, 0x1c);
        }
        word(&mut out, 0);
        zeros(&mut out, 0x100);
        dword(&mut out, 0);
        zeros(&mut out, 1);

        zeros(&mut out, 18);
        dword(&mut out, 0);
        dword(&mut out, 0);
        dword(&mut out, 0);
        for _ in 0..4 {
            word(&mut out, 0);
        }
        if save_version >= 25 {
            word(&mut out, 0);
        }
        dword(&mut out, 0);
        dword(&mut out, 0);

        zeros(&mut out, 1 + 1);
        word(&mut out, 0);
        zeros(&mut out, 1 + 1);
        zeros(&mut out, 1 + 0x30);
        zeros(
            &mut out,
            PLAYER_COUNT
                * if save_version >= 37 {
                    PLAYER_SIZE
                } else {
                    PLAYER_SIZE - 2
                },
        );
        zeros(&mut out, 1);
        let hero_count = if save_version < 25 { 128 } else { HERO_COUNT };
        for _ in 0..hero_count {
            empty_hero(&mut out, save_version);
        }
        if save_version < 31 {
            zeros(&mut out, PLAYER_COUNT);
        }
        zeros(&mut out, hero_count);
        if save_version >= 31 {
            zeros(&mut out, HERO_COUNT);
        }
        zeros(&mut out, 17 + 109 + 4 + 2);
        for _ in 0..if save_version < 32 { 9 } else { 19 } {
            word(&mut out, 0);
        }
        word(&mut out, 0);
        word(&mut out, 0);
        dword(&mut out, 0);
        out
    }

    fn assert_fully_consumed(input: Cursor<'_>) {
        assert_eq!(input.remaining(), 0, "{} unparsed bytes", input.remaining());
    }

    fn map_header_with_versioned_fields(save_version: i32) -> Vec<u8> {
        let mut out = Vec::new();
        dword(&mut out, 28);
        out.push(1);
        dword(&mut out, 36);
        out.push(0);
        word(&mut out, 0);
        word(&mut out, 0);
        out.push(2);
        if save_version >= 27 {
            out.push(0);
        }
        for _ in 0..PLAYER_COUNT {
            zeros(&mut out, 3);
            zeros(&mut out, if save_version < 28 { 1 } else { 2 });
            zeros(&mut out, 2);
            out.push(0xff);
        }
        out.push(0xff);
        out.push(1);
        zeros(&mut out, if save_version == 16 { 3 } else { 2 });
        out.push(0);
        if save_version >= 30 {
            out.push(1);
            zeros(&mut out, 2);
            dword(&mut out, 0);
            if save_version >= 31 {
                out.push(0xff);
            }
        }
        out
    }

    #[test]
    fn parses_minimal_version_42_stream_to_the_last_byte() {
        let bytes = minimal_save(VERSION);
        let save = SaveGame::parse(&bytes).unwrap();
        assert_eq!(save.kind, Kind::Game);
        assert_eq!(save.map_version, 28);
        assert_eq!(save.map_size, 1);
        assert_eq!(save.counts.cells, 1);
        assert_eq!(save.counts.heroes, HERO_COUNT);
        assert_eq!(save.bytes().len(), bytes.len());
    }

    #[test]
    fn every_retail_accepted_revision_reaches_exact_eof() {
        for version in (16..=18).chain(25..=VERSION) {
            let bytes = minimal_save(version);
            let save = SaveGame::parse(&bytes).unwrap();
            assert_eq!(save.version, version);
            let expected_game_version = if version <= 18 {
                0
            } else if version <= 30 {
                1
            } else if version < 40 {
                2
            } else {
                0
            };
            assert_eq!(save.game_version, expected_game_version);
            assert_eq!(save.counts.heroes, if version < 25 { 128 } else { 156 });
            assert_eq!(save.bytes().len(), bytes.len());
        }
    }

    #[test]
    fn revisions_rejected_by_retail_are_rejected_before_the_body() {
        for version in [15_i32, 19, 24, 43] {
            let mut bytes = minimal_save(VERSION);
            bytes[8..12].copy_from_slice(&version.to_le_bytes());
            assert!(matches!(
                SaveGame::parse(&bytes),
                Err(Error::UnsupportedVersion(actual)) if actual == version
            ));
        }
    }

    #[test]
    fn map_header_historical_widths_are_exercised() {
        for version in [16, 17, 25, 27, 28, 30, 31, 42] {
            let bytes = map_header_with_versioned_fields(version);
            let mut input = Cursor::new(&bytes);
            let (_, size, two_layers) = parse_map_header(&mut input, version).unwrap();
            assert_eq!(size, 36);
            assert!(!two_layers);
            assert_fully_consumed(input);
        }
    }

    #[test]
    fn campaign_legacy_blob_and_fieldwise_revisions_are_exercised() {
        let legacy = vec![0_u8; LEGACY_CAMPAIGN_SIZE];
        let mut input = Cursor::new(&legacy);
        parse_campaign(&mut input, 18, &mut Counts::default()).unwrap();
        assert_fully_consumed(input);

        for version in [28, 35, 36, 42] {
            let mut bytes = Vec::new();
            zeros(&mut bytes, 7);
            dword(&mut bytes, 0);
            zeros(&mut bytes, if version >= 36 { 21 } else { 14 });
            bytes.push(1);
            zeros(&mut bytes, 11);
            bytes.push(1);
            bytes.push(1);
            empty_hero(&mut bytes, version);
            word(&mut bytes, 1);
            zeros(&mut bytes, 4);
            bytes.push(1);
            word(&mut bytes, 7);

            let mut counts = Counts::default();
            let mut input = Cursor::new(&bytes);
            parse_campaign(&mut input, version, &mut counts).unwrap();
            assert_eq!(counts.campaign_heroes, 1);
            assert_fully_consumed(input);
        }
    }

    #[test]
    fn nested_historical_payload_widths_are_exercised() {
        for version in [18, 25] {
            let mut bytes = Vec::new();
            bytes.push(0);
            zeros(&mut bytes, 8 + 2 + 28 + 4);
            bytes.extend_from_slice(&[0, 0, 0, 1]);
            zeros(&mut bytes, if version < 25 { 3 } else { 4 });
            let mut input = Cursor::new(&bytes);
            parse_black_box(&mut input, version).unwrap();
            assert_fully_consumed(input);
        }

        for version in [25, 30, 31, 36] {
            let mut bytes = Vec::new();
            zeros(&mut bytes, 2);
            if version == 30 || version >= 36 {
                bytes.push(0);
            }
            zeros(&mut bytes, 1 + 1 + 4);
            for _ in 0..3 {
                dword(&mut bytes, 0);
            }
            let mut input = Cursor::new(&bytes);
            parse_quest(&mut input, version, 3, 0).unwrap();
            assert_fully_consumed(input);
        }

        let mut be_hero = Vec::new();
        zeros(&mut be_hero, 2 + 1 + 1 + 4);
        for _ in 0..3 {
            dword(&mut be_hero, 0);
        }
        let mut input = Cursor::new(&be_hero);
        parse_quest(&mut input, 42, 8, 0).unwrap();
        assert_fully_consumed(input);
    }

    #[test]
    fn pool_town_hero_and_event_revision_widths_are_exercised() {
        for version in [18, 27, 28] {
            let mut bytes = Vec::new();
            bytes.push(0);
            bytes.push(1);
            zeros(&mut bytes, if version < 25 { 8 } else { 62 });
            word(&mut bytes, 0);
            bytes.push(1);
            zeros(&mut bytes, if version < 28 { 60 } else { 61 });
            bytes.push(1);
            zeros(&mut bytes, 28);
            let mut input = Cursor::new(&bytes);
            parse_object_pools(&mut input, version).unwrap();
            assert_fully_consumed(input);
        }

        for version in [18, 25, 42] {
            let mut town = Vec::new();
            zeros(&mut town, 10 + 56 + 4);
            if version < 25 {
                zeros(&mut town, 13);
            } else {
                word(&mut town, 0);
            }
            zeros(&mut town, 28 + 56 + 5 + 24 + 120 + 70 + 1 + 4 + 2);
            let mut input = Cursor::new(&town);
            parse_town(&mut input, version).unwrap();
            assert_fully_consumed(input);

            let mut hero = Vec::new();
            empty_hero(&mut hero, version);
            let mut input = Cursor::new(&hero);
            parse_hero(&mut input, version).unwrap();
            assert_fully_consumed(input);
        }

        for version in [16, 18, 28, 30, 31, 35, 42] {
            let boat = recorded_boat_payload_size(version);
            let mut bytes = Vec::new();
            dword(&mut bytes, 2);
            bytes.push(5);
            zeros(&mut bytes, boat);
            bytes.push(6);
            zeros(&mut bytes, boat + 8);
            let mut counts = Counts::default();
            let mut input = Cursor::new(&bytes);
            parse_recorded_events(&mut input, version, &mut counts).unwrap();
            assert_eq!(counts.recorded_events, 2);
            assert_fully_consumed(input);
        }
    }

    #[test]
    fn timed_event_version_42_adds_exactly_one_byte() {
        for version in [41, 42] {
            let mut bytes = Vec::new();
            word(&mut bytes, 0);
            zeros(&mut bytes, if version >= 42 { 35 } else { 34 });
            let mut input = Cursor::new(&bytes);
            parse_timed_event(&mut input, version).unwrap();
            assert_fully_consumed(input);
        }
    }

    #[test]
    fn every_truncation_is_rejected_without_panicking() {
        let bytes = minimal_save(VERSION);
        for end in 0..bytes.len() {
            assert!(SaveGame::parse(&bytes[..end]).is_err(), "accepted {end}");
        }
    }
}
