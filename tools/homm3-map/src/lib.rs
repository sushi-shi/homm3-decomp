#![no_std]
#![forbid(unsafe_code)]
//! Allocation-free parsing of complete retail H3M/TUT and H3C byte streams.
//!
//! Inputs are already-inflated streams. Each stage borrows its validated
//! extent and exposes the remaining bytes to the next retail loader stage.

mod body;
pub mod campaign;

pub use body::{
    MapBody, PlacedObject, PlacedObjects, TimedEvent, TimedEvents, MAP_TRAILING_PADDING_SIZE,
};

use core::fmt;

/// Number of player slots in every supported map generation.
pub const PLAYER_COUNT: usize = 8;
/// Reserved bytes at the end of the map header.
pub const HEADER_PADDING_SIZE: usize = 31;
/// Hero-availability bits in Restoration of Erathia headers.
pub const LEGACY_HERO_BYTES: usize = 128 / 8;
/// Hero-availability bits in Armageddon's Blade and Shadow of Death headers.
pub const COMPLETE_HERO_BYTES: usize = 156_usize.div_ceil(8);

/// Retail map-format generation.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
#[repr(i32)]
pub enum Version {
    /// Restoration of Erathia.
    Restoration = 14,
    /// Armageddon's Blade.
    ArmageddonsBlade = 21,
    /// Shadow of Death / Complete.
    ShadowOfDeath = 28,
}

impl Version {
    fn parse(value: i32) -> Result<Self, Error> {
        match value {
            14 => Ok(Self::Restoration),
            21 => Ok(Self::ArmageddonsBlade),
            28 => Ok(Self::ShadowOfDeath),
            _ => Err(Error::UnsupportedVersion(value)),
        }
    }
}

/// A malformed scenario-map header.
#[allow(missing_docs)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    Short {
        offset: usize,
        needed: usize,
        available: usize,
    },
    UnsupportedVersion(i32),
    BadCount {
        offset: usize,
        value: i32,
        field: CountField,
    },
    SizeOverflow {
        offset: usize,
    },
    BadObjectTypeIndex {
        offset: usize,
        value: u32,
        count: u32,
        object_ordinal: u32,
        previous_class: Option<u32>,
    },
    BadObjectClass {
        offset: usize,
        value: u32,
    },
    BadQuestType {
        offset: usize,
        value: u8,
    },
    BadSeerReward {
        offset: usize,
        value: u8,
    },
    NonZeroPadding {
        offset: usize,
    },
}

/// Variable-count field associated with [`Error::BadCount`].
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CountField {
    /// Custom heroes retained by a player slot.
    PlayerHeroes,
    /// Hero placeholder list.
    Placeholders,
    /// Global map rumor list.
    Rumours,
    /// Secondary skills in one custom hero setup.
    HeroSecondarySkills,
    /// Adventure-object template table.
    ObjectTypes,
    /// Placed adventure-object table.
    Objects,
    /// Secondary-skill vector in a placed hero record.
    PlacedHeroSecondarySkills,
    /// Town-specific timed-event list.
    TownEvents,
    /// Global timed-event list at the end of the map.
    TimedEvents,
    /// Creature stacks in a Pandora/event reward record.
    BlackBoxCreatures,
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
                "map field at {offset:#x} needs {needed} bytes, only {available} remain"
            ),
            Self::UnsupportedVersion(version) => {
                write!(f, "map version {version} is not retail 14, 21, or 28")
            }
            Self::BadCount {
                offset,
                value,
                field,
            } => write!(f, "map {field:?} count {value} at {offset:#x} is invalid"),
            Self::SizeOverflow { offset } => {
                write!(f, "map extent at {offset:#x} overflows the host")
            }
            Self::BadObjectTypeIndex {
                offset,
                value,
                count,
                object_ordinal,
                previous_class,
            } => write!(
                f,
                "map object {object_ordinal} type index {value} at {offset:#x} is outside {count} templates (previous class {previous_class:?})"
            ),
            Self::BadObjectClass { offset, value } => write!(
                f,
                "map object class {value} at {offset:#x} is outside the 232-row retail table"
            ),
            Self::BadQuestType { offset, value } => {
                write!(f, "map quest type {value} at {offset:#x} is invalid")
            }
            Self::BadSeerReward { offset, value } => {
                write!(f, "map seer reward type {value} at {offset:#x} is invalid")
            }
            Self::NonZeroPadding { offset } => {
                write!(f, "map reserved padding at {offset:#x} is nonzero")
            }
        }
    }
}

impl core::error::Error for Error {}

/// A dword-length-prefixed map string borrowing the consumed bytes.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct MapString<'a> {
    declared_length: i32,
    bytes: &'a [u8],
}

impl<'a> MapString<'a> {
    const EMPTY: Self = Self {
        declared_length: 0,
        bytes: &[],
    };

    /// Signed length as read by retail.
    #[must_use]
    pub const fn declared_length(self) -> i32 {
        self.declared_length
    }

    /// String bytes. Retail maps nonpositive and `>= 0xffff` lengths to empty.
    #[must_use]
    pub const fn bytes(self) -> &'a [u8] {
        self.bytes
    }
}

/// One variable-width player slot from the map header.
#[allow(clippy::struct_excessive_bools)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PlayerSlot<'a> {
    /// Whether a human can occupy the slot.
    pub can_be_human: bool,
    /// Whether the AI can occupy the slot.
    pub can_be_computer: bool,
    /// Stored AI strategy byte.
    pub ai_strategy: i8,
    /// Stored town/alignment availability mask.
    pub legal_alignments: u16,
    /// Whether the alignment may be random.
    pub has_random_alignment: bool,
    /// Whether a main-town record follows.
    pub has_main_town: bool,
    /// Whether the game should generate the starting hero.
    pub generate_hero: bool,
    /// Main town type, when serialized by newer formats.
    pub main_town_type: i8,
    /// Main town coordinates, or zeroes when no town is present.
    pub main_town: [u8; 3],
    /// Whether the starting hero may be random.
    pub has_random_hero: bool,
    /// Fixed hero id, with `0xff` preserved as the disk sentinel.
    pub hero_id: u8,
    /// Fixed hero portrait id.
    pub hero_portrait: u8,
    /// Fixed hero custom name.
    pub hero_name: MapString<'a>,
    /// Count of newer per-player hero identity records.
    pub retained_hero_count: u32,
}

impl PlayerSlot<'_> {
    const EMPTY: Self = Self {
        can_be_human: false,
        can_be_computer: false,
        ai_strategy: 0,
        legal_alignments: 0,
        has_random_alignment: false,
        has_main_town: false,
        generate_hero: false,
        main_town_type: -1,
        main_town: [0; 3],
        has_random_hero: false,
        hero_id: 0xff,
        hero_portrait: 0xff,
        hero_name: MapString::EMPTY,
        retained_hero_count: 0,
    };
}

/// A victory record with its condition-specific bytes preserved.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Victory<'a> {
    /// On-disk type byte (`0xff` means ordinary victory only).
    pub kind: u8,
    /// Common flag present for every non-sentinel condition.
    pub allow_normal: Option<bool>,
    /// Common flag present for every non-sentinel condition.
    pub applies_to_computer: Option<bool>,
    /// Condition-specific payload after the common flags.
    pub payload: &'a [u8],
}

/// A loss record with its condition-specific bytes preserved.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Loss<'a> {
    /// On-disk type byte (`0xff` means defeat-all only).
    pub kind: u8,
    /// Condition-specific payload.
    pub payload: &'a [u8],
}

/// A validated scenario-map header borrowing all variable data.
#[derive(Clone, Copy, Debug)]
pub struct MapHeader<'a> {
    source: &'a [u8],
    header_len: usize,
    /// Map generation.
    pub version: Version,
    /// Playability flag.
    pub playable: bool,
    /// Square map dimension.
    pub size: i32,
    /// Whether the underground layer is present.
    pub two_layers: bool,
    /// Display name.
    pub name: MapString<'a>,
    /// Display description.
    pub description: MapString<'a>,
    /// Difficulty ordinal.
    pub difficulty: u8,
    /// Maximum hero level; zero for Restoration maps.
    pub max_hero_level: i8,
    /// Eight player slot records.
    pub players: [PlayerSlot<'a>; PLAYER_COUNT],
    /// Victory condition record.
    pub victory: Victory<'a>,
    /// Loss condition record.
    pub loss: Loss<'a>,
    /// Team count byte.
    pub team_count: u8,
    /// Eight team ids when `team_count != 0`, otherwise an empty slice.
    pub teams: &'a [u8],
    /// Packed hero-availability mask.
    pub available_heroes: &'a [u8],
    /// Count of hero placeholder ids consumed from the header.
    pub placeholder_count: u32,
    /// Count of Complete custom hero setup records.
    pub hero_setup_count: u8,
    /// Reserved 31-byte tail.
    pub padding: &'a [u8],
}

impl<'a> MapHeader<'a> {
    /// Parse the complete header consumed by retail `NewSMapHeader::Read`.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for unsupported versions, truncated variable fields,
    /// impossible negative counts, or size arithmetic overflow.
    pub fn parse(source: &'a [u8]) -> Result<Self, Error> {
        let mut cursor = Cursor::new(source);
        let version = Version::parse(cursor.i32()?)?;
        let playable = cursor.u8()? != 0;
        let size = cursor.i32()?;
        let two_layers = cursor.u8()? != 0;
        let name = cursor.map_string()?;
        let description = cursor.map_string()?;
        let difficulty = cursor.u8()?;
        let max_hero_level = if version == Version::Restoration {
            0
        } else {
            cursor.i8()?
        };

        let mut players = [PlayerSlot::EMPTY; PLAYER_COUNT];
        for player in &mut players {
            *player = parse_player(&mut cursor, version)?;
        }
        let victory = parse_victory(&mut cursor, version)?;
        let loss = parse_loss(&mut cursor)?;
        let team_count = cursor.u8()?;
        let teams = if team_count == 0 {
            &source[0..0]
        } else {
            cursor.take(PLAYER_COUNT)?
        };
        let hero_bytes = if version == Version::Restoration {
            LEGACY_HERO_BYTES
        } else {
            COMPLETE_HERO_BYTES
        };
        let available_heroes = cursor.take(hero_bytes)?;
        let placeholder_count = parse_placeholders(&mut cursor, version)?;
        let hero_setup_count = parse_hero_setups(&mut cursor, version)?;
        let padding = cursor.take(HEADER_PADDING_SIZE)?;
        let header_len = cursor.position();

        Ok(Self {
            source,
            header_len,
            version,
            playable,
            size,
            two_layers,
            name,
            description,
            difficulty,
            max_hero_level,
            players,
            victory,
            loss,
            team_count,
            teams,
            available_heroes,
            placeholder_count,
            hero_setup_count,
            padding,
        })
    }

    /// Number of inflated bytes consumed by the header.
    #[must_use]
    pub const fn header_len(self) -> usize {
        self.header_len
    }

    /// World payload beginning immediately after the reserved header tail.
    #[must_use]
    pub fn world(self) -> &'a [u8] {
        &self.source[self.header_len..]
    }
}

/// Global world-stream fields consumed by `game::LoadMap` before
/// `NewfullMap::Read` begins terrain and object parsing.
#[derive(Clone, Copy, Debug)]
pub struct WorldPrefix<'a> {
    source: &'a [u8],
    consumed: usize,
    /// Serialized disabled-artifact mask (absent in `RoE`).
    pub disabled_artifacts: &'a [u8],
    /// Serialized disabled-spell mask (`SoD` only).
    pub disabled_spells: &'a [u8],
    /// Serialized disabled-secondary-skill mask (`SoD` only).
    pub disabled_skills: &'a [u8],
    /// Number of two-string rumor records.
    pub rumour_count: u32,
    /// Number of present custom hero setup records among 156 flags.
    pub custom_hero_setups: usize,
}

impl<'a> WorldPrefix<'a> {
    /// Parse the global availability/rumor/hero-setup prefix after an H3M header.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] when a mask, string, count, or custom hero record is
    /// truncated or carries a negative allocation count.
    pub fn parse(source: &'a [u8], version: Version) -> Result<Self, Error> {
        let mut cursor = Cursor::new(source);
        let disabled_artifacts = match version {
            Version::Restoration => &source[0..0],
            Version::ArmageddonsBlade => cursor.take(17)?,
            Version::ShadowOfDeath => cursor.take(18)?,
        };
        let (disabled_spells, disabled_skills) = if version == Version::ShadowOfDeath {
            (cursor.take(9)?, cursor.take(4)?)
        } else {
            (&source[0..0], &source[0..0])
        };

        let count_offset = cursor.position();
        let rumour_count = checked_count(cursor.i32()?, count_offset, CountField::Rumours)?;
        for _ in 0..rumour_count {
            let _name = cursor.map_string()?;
            let _text = cursor.map_string()?;
        }

        let mut custom_hero_setups = 0usize;
        if version == Version::ShadowOfDeath {
            for _ in 0..156 {
                if cursor.u8()? == 0 {
                    continue;
                }
                custom_hero_setups += 1;
                parse_custom_hero_setup(&mut cursor)?;
            }
        }
        Ok(Self {
            source,
            consumed: cursor.position(),
            disabled_artifacts,
            disabled_spells,
            disabled_skills,
            rumour_count,
            custom_hero_setups,
        })
    }

    /// Number of bytes consumed before `NewfullMap::Read`.
    #[must_use]
    pub const fn consumed(self) -> usize {
        self.consumed
    }

    /// Terrain/object stream beginning at `NewfullMap::Read`.
    #[must_use]
    pub fn map(self) -> &'a [u8] {
        &self.source[self.consumed..]
    }
}

/// One seven-byte H3M terrain cell.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct TerrainCell {
    /// Terrain set id.
    pub ground_set: i8,
    /// Terrain frame id.
    pub ground_index: i8,
    /// River set id.
    pub river_set: i8,
    /// River frame id.
    pub river_index: i8,
    /// Road set id.
    pub road_set: i8,
    /// Road frame id.
    pub road_index: i8,
    /// Six mirror bits plus the coastal/anchor flag.
    pub flags: u8,
}

/// Borrowed surface/underground terrain bands consumed by `readMapLayer`.
#[derive(Clone, Copy, Debug)]
pub struct Terrain<'a> {
    source: &'a [u8],
    bytes: &'a [u8],
    size: usize,
    layers: usize,
}

impl<'a> Terrain<'a> {
    /// Validate and borrow every seven-byte terrain cell.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] when dimensions overflow or the complete cell band
    /// is not present.
    pub fn parse(source: &'a [u8], size: i32, two_layers: bool) -> Result<Self, Error> {
        let dimension = usize::try_from(size).map_err(|_| Error::SizeOverflow { offset: 0 })?;
        let layers = usize::from(two_layers) + 1;
        let byte_count = dimension
            .checked_mul(dimension)
            .and_then(|value| value.checked_mul(layers))
            .and_then(|value| value.checked_mul(7))
            .ok_or(Error::SizeOverflow { offset: 0 })?;
        let bytes = source.get(..byte_count).ok_or(Error::Short {
            offset: 0,
            needed: byte_count,
            available: source.len(),
        })?;
        Ok(Self {
            source,
            bytes,
            size: dimension,
            layers,
        })
    }

    /// Square map dimension.
    #[must_use]
    pub const fn size(self) -> usize {
        self.size
    }

    /// One or two serialized layers.
    #[must_use]
    pub const fn layers(self) -> usize {
        self.layers
    }

    /// Total number of terrain cells.
    #[must_use]
    pub const fn len(self) -> usize {
        self.bytes.len() / 7
    }

    /// Whether the terrain band has no cells.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        self.bytes.is_empty()
    }

    /// Decode one cell by flat layer/y/x order.
    #[must_use]
    pub fn cell(self, index: usize) -> Option<TerrainCell> {
        let at = index.checked_mul(7)?;
        let bytes = self.bytes.get(at..at + 7)?;
        Some(TerrainCell {
            ground_set: i8::from_le_bytes([bytes[0]]),
            ground_index: i8::from_le_bytes([bytes[1]]),
            river_set: i8::from_le_bytes([bytes[2]]),
            river_index: i8::from_le_bytes([bytes[3]]),
            road_set: i8::from_le_bytes([bytes[4]]),
            road_index: i8::from_le_bytes([bytes[5]]),
            flags: bytes[6],
        })
    }

    /// Object-type table beginning after the final terrain cell.
    #[must_use]
    pub fn objects(self) -> &'a [u8] {
        &self.source[self.bytes.len()..]
    }
}

/// One variable-width H3M adventure-object template.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ObjectType<'a> {
    /// Sprite resource name.
    pub image_name: MapString<'a>,
    /// Six-byte passability mask stored in H3M.
    pub passable: &'a [u8],
    /// Six-byte trigger mask stored in H3M.
    pub trigger: &'a [u8],
    /// Original object class before the retail trait-table remap.
    pub object_class: u32,
    /// Object-class-specific subtype.
    pub extra: u32,
    /// Discarded editor group byte.
    pub group: u8,
    /// Whether the sprite is suppressed.
    pub suppress_draw: bool,
}

/// Validated object-template table and the placed-object count that follows it.
#[derive(Clone, Copy, Debug)]
pub struct ObjectTable<'a> {
    source: &'a [u8],
    types: &'a [u8],
    type_count: u32,
    object_count: u32,
    objects_offset: usize,
}

impl<'a> ObjectTable<'a> {
    /// Parse every object template consumed by `readObjectType` and the
    /// following placed-object count.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for negative counts or truncated strings/records.
    pub fn parse(source: &'a [u8]) -> Result<Self, Error> {
        let mut cursor = Cursor::new(source);
        let type_count_offset = cursor.position();
        let type_count = checked_count(cursor.i32()?, type_count_offset, CountField::ObjectTypes)?;
        let types_start = cursor.position();
        for _ in 0..type_count {
            let _ = parse_object_type(&mut cursor)?;
        }
        let types_end = cursor.position();
        let object_count_offset = cursor.position();
        let object_count = checked_count(cursor.i32()?, object_count_offset, CountField::Objects)?;
        let objects_offset = cursor.position();
        Ok(Self {
            source,
            types: &source[types_start..types_end],
            type_count,
            object_count,
            objects_offset,
        })
    }

    /// Number of object templates.
    #[must_use]
    pub const fn type_count(self) -> u32 {
        self.type_count
    }

    /// Number of placed objects whose records follow.
    #[must_use]
    pub const fn object_count(self) -> u32 {
        self.object_count
    }

    /// Iterate object templates in type-index order.
    #[must_use]
    pub const fn types(self) -> ObjectTypes<'a> {
        ObjectTypes {
            cursor: Cursor::new(self.types),
            remaining: self.type_count,
        }
    }

    /// Resolve one template by the on-disk placed-object index.
    #[must_use]
    pub fn type_at(self, index: u32) -> Option<ObjectType<'a>> {
        if index >= self.type_count {
            return None;
        }
        self.types().nth(usize::try_from(index).ok()?)
    }

    /// Placed-object records beginning after their count dword.
    #[must_use]
    pub fn objects(self) -> &'a [u8] {
        &self.source[self.objects_offset..]
    }
}

/// Iterator over a validated object-template table.
#[derive(Clone, Debug)]
pub struct ObjectTypes<'a> {
    cursor: Cursor<'a>,
    remaining: u32,
}

impl<'a> Iterator for ObjectTypes<'a> {
    type Item = ObjectType<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.remaining == 0 {
            return None;
        }
        self.remaining -= 1;
        parse_object_type(&mut self.cursor).ok()
    }
}

fn parse_object_type<'a>(cursor: &mut Cursor<'a>) -> Result<ObjectType<'a>, Error> {
    let image_name = cursor.map_string()?;
    let passable = cursor.take(6)?;
    let trigger = cursor.take(6)?;
    let _landscape_masks = cursor.take(4)?;
    let object_class = cursor.u32()?;
    let extra = cursor.u32()?;
    let group = cursor.u8()?;
    let suppress_draw = cursor.u8()? != 0;
    let _padding = cursor.take(16)?;
    Ok(ObjectType {
        image_name,
        passable,
        trigger,
        object_class,
        extra,
        group,
        suppress_draw,
    })
}

fn parse_custom_hero_setup(cursor: &mut Cursor<'_>) -> Result<(), Error> {
    if cursor.u8()? != 0 {
        let _experience = cursor.take(4)?;
    }
    if cursor.u8()? != 0 {
        let offset = cursor.position();
        let count = checked_count(cursor.i32()?, offset, CountField::HeroSecondarySkills)?;
        let bytes = usize::try_from(count)
            .map_err(|_| Error::SizeOverflow { offset })?
            .checked_mul(2)
            .ok_or(Error::SizeOverflow { offset })?;
        let _skills = cursor.take(bytes)?;
    }
    if cursor.u8()? != 0 {
        let _equipped_artifacts = cursor.take(19 * 2)?;
        let backpack_count = usize::from(cursor.u16()? & 0xff);
        let backpack_bytes = backpack_count.checked_mul(2).ok_or(Error::SizeOverflow {
            offset: cursor.position(),
        })?;
        let _backpack = cursor.take(backpack_bytes)?;
    }
    if cursor.u8()? != 0 {
        let _name = cursor.map_string()?;
    }
    let _sex = cursor.i8()?;
    if cursor.u8()? != 0 {
        let _spells = cursor.take(9)?;
    }
    if cursor.u8()? != 0 {
        let _primary_skills = cursor.take(4)?;
    }
    Ok(())
}

fn parse_player<'a>(cursor: &mut Cursor<'a>, version: Version) -> Result<PlayerSlot<'a>, Error> {
    let can_be_human = cursor.u8()? != 0;
    let can_be_computer = cursor.u8()? != 0;
    let ai_strategy = cursor.i8()?;
    let legal_alignments = if version == Version::Restoration {
        u16::from(cursor.u8()?)
    } else {
        if version != Version::ArmageddonsBlade {
            let _ = cursor.u8()?;
        }
        cursor.u16()?
    };
    let has_random_alignment = cursor.u8()? != 0;
    let has_main_town = cursor.u8()? != 0;
    let mut generate_hero = false;
    let mut main_town_type = -1;
    let mut main_town = [0; 3];
    if has_main_town {
        if version == Version::Restoration {
            generate_hero = true;
        } else {
            generate_hero = cursor.u8()? != 0;
            main_town_type = cursor.i8()?;
        }
        for coordinate in &mut main_town {
            *coordinate = cursor.u8()?;
        }
    }

    let has_random_hero = cursor.u8()? != 0;
    let hero_id = cursor.u8()?;
    let (hero_portrait, hero_name) = if hero_id == 0xff {
        (0xff, MapString::EMPTY)
    } else {
        (cursor.u8()?, cursor.map_string()?)
    };
    let retained_hero_count = if version == Version::Restoration {
        0
    } else {
        let _default_placeholder_count = cursor.u8()?;
        let offset = cursor.position();
        let count = cursor.i32()?;
        let count = checked_count(count, offset, CountField::PlayerHeroes)?;
        for _ in 0..count {
            let _hero_id = cursor.u8()?;
            let _hero_name = cursor.map_string()?;
        }
        count
    };

    Ok(PlayerSlot {
        can_be_human,
        can_be_computer,
        ai_strategy,
        legal_alignments,
        has_random_alignment,
        has_main_town,
        generate_hero,
        main_town_type,
        main_town,
        has_random_hero,
        hero_id,
        hero_portrait,
        hero_name,
        retained_hero_count,
    })
}

fn parse_victory<'a>(cursor: &mut Cursor<'a>, version: Version) -> Result<Victory<'a>, Error> {
    let kind = cursor.u8()?;
    if kind == 0xff {
        return Ok(Victory {
            kind,
            allow_normal: None,
            applies_to_computer: None,
            payload: &cursor.data[0..0],
        });
    }
    let allow_normal = Some(cursor.u8()? != 0);
    let applies_to_computer = Some(cursor.u8()? != 0);
    let payload_size = match kind {
        0 => usize::from(version != Version::Restoration) + 1,
        1 => usize::from(version != Version::Restoration) + 5,
        2 | 3 => 5,
        4..=7 => 3,
        10 | 12 => 4,
        _ => 0,
    };
    let payload = cursor.take(payload_size)?;
    Ok(Victory {
        kind,
        allow_normal,
        applies_to_computer,
        payload,
    })
}

fn parse_loss<'a>(cursor: &mut Cursor<'a>) -> Result<Loss<'a>, Error> {
    let kind = cursor.u8()?;
    let payload_size = match kind {
        0 | 1 => 3,
        2 => 2,
        _ => 0,
    };
    Ok(Loss {
        kind,
        payload: cursor.take(payload_size)?,
    })
}

fn parse_placeholders(cursor: &mut Cursor<'_>, version: Version) -> Result<u32, Error> {
    if version == Version::Restoration {
        return Ok(0);
    }
    let offset = cursor.position();
    let count = checked_count(cursor.i32()?, offset, CountField::Placeholders)?;
    let size = usize::try_from(count).map_err(|_| Error::SizeOverflow { offset })?;
    let _ = cursor.take(size)?;
    Ok(count)
}

fn parse_hero_setups(cursor: &mut Cursor<'_>, version: Version) -> Result<u8, Error> {
    if version != Version::ShadowOfDeath {
        return Ok(0);
    }
    let count = cursor.u8()?;
    for _ in 0..count {
        let _key = cursor.u8()?;
        let _hero_id = cursor.u8()?;
        let _name = cursor.map_string()?;
        let _availability = cursor.u8()?;
    }
    Ok(count)
}

pub(crate) fn checked_count(value: i32, offset: usize, field: CountField) -> Result<u32, Error> {
    u32::try_from(value).map_err(|_| Error::BadCount {
        offset,
        value,
        field,
    })
}

#[derive(Clone, Copy, Debug)]
pub(crate) struct Cursor<'a> {
    data: &'a [u8],
    position: usize,
}

impl<'a> Cursor<'a> {
    pub(crate) const fn new(data: &'a [u8]) -> Self {
        Self { data, position: 0 }
    }

    pub(crate) const fn position(self) -> usize {
        self.position
    }

    pub(crate) fn take(&mut self, size: usize) -> Result<&'a [u8], Error> {
        let end = self.position.checked_add(size).ok_or(Error::SizeOverflow {
            offset: self.position,
        })?;
        let bytes = self.data.get(self.position..end).ok_or(Error::Short {
            offset: self.position,
            needed: size,
            available: self.data.len().saturating_sub(self.position),
        })?;
        self.position = end;
        Ok(bytes)
    }

    pub(crate) fn u8(&mut self) -> Result<u8, Error> {
        Ok(self.take(1)?[0])
    }

    pub(crate) fn i8(&mut self) -> Result<i8, Error> {
        Ok(i8::from_le_bytes([self.u8()?]))
    }

    pub(crate) fn u16(&mut self) -> Result<u16, Error> {
        let bytes = self.take(2)?;
        Ok(u16::from_le_bytes([bytes[0], bytes[1]]))
    }

    pub(crate) fn i32(&mut self) -> Result<i32, Error> {
        let bytes = self.take(4)?;
        Ok(i32::from_le_bytes([bytes[0], bytes[1], bytes[2], bytes[3]]))
    }

    pub(crate) fn u32(&mut self) -> Result<u32, Error> {
        let bytes = self.take(4)?;
        Ok(u32::from_le_bytes([bytes[0], bytes[1], bytes[2], bytes[3]]))
    }

    pub(crate) fn remaining(&self) -> &'a [u8] {
        &self.data[self.position..]
    }

    pub(crate) fn map_string(&mut self) -> Result<MapString<'a>, Error> {
        let declared_length = self.i32()?;
        let bytes = if declared_length > 0 && declared_length < 0xffff {
            let offset = self.position;
            let size =
                usize::try_from(declared_length).map_err(|_| Error::SizeOverflow { offset })?;
            self.take(size)?
        } else {
            &self.data[0..0]
        };
        Ok(MapString {
            declared_length,
            bytes,
        })
    }
}
