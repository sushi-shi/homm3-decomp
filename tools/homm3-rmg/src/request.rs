//! Parse the lobby record into supported generation domains.

use crate::{
    behavior::Behavior,
    domain::{Level, Ordinal, Terrain},
    raw,
};
use std::{
    error::Error,
    fmt,
    ops::{Index, IndexMut},
};

/// Number of player colours, obtained from the shared C++ declaration.
pub const PLAYER_COUNT: usize = raw::RMG_PLAYER_COUNT as usize;

/// A playable colour, distinct from a template's player slot.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Player(u8);
impl Player {
    /// Index into request colour arrays.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0 as usize
    }
    /// Every colour in lobby order.
    pub fn all() -> impl DoubleEndedIterator<Item = Self> + ExactSizeIterator + Clone {
        const { assert!(PLAYER_COUNT <= u8::MAX as usize) };
        (0..=u8::MAX).map(Self).take(PLAYER_COUNT)
    }
}

/// One value per player colour, indexed by `Player` rather than a bare index.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PerColour<T>([T; PLAYER_COUNT]);
impl<T> PerColour<T> {
    /// Values in lobby colour order.
    pub const fn new(values: [T; PLAYER_COUNT]) -> Self {
        Self(values)
    }
    /// Values in lobby colour order, for serialization.
    pub const fn values(&self) -> &[T; PLAYER_COUNT] {
        &self.0
    }
    /// Values with their colours, in lobby order.
    pub fn iter(&self) -> impl DoubleEndedIterator<Item = (Player, &T)> {
        Player::all().zip(&self.0)
    }
    /// Apply a function to each colour's value.
    pub fn map<U>(self, f: impl FnMut(T) -> U) -> PerColour<U> {
        PerColour(self.0.map(f))
    }
}
impl<T> Index<Player> for PerColour<T> {
    type Output = T;
    fn index(&self, player: Player) -> &T {
        &self.0[player.index()]
    }
}
impl<T> IndexMut<Player> for PerColour<T> {
    fn index_mut(&mut self, player: Player) -> &mut T {
        &mut self.0[player.index()]
    }
}

/// An input field that cannot produce a supported typed request.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct InputError {
    /// Name of the request field or relationship that failed.
    pub field: &'static str,
    /// Offending value, including unsigned raw enum values without truncation.
    pub value: i64,
}

impl fmt::Display for InputError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "unsupported {}: {}", self.field, self.value)
    }
}
impl Error for InputError {}

fn invalid(field: &'static str, value: impl Into<i64>) -> InputError {
    InputError {
        field,
        value: value.into(),
    }
}

/// Square map dimensions supported by the lobby.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum MapSize {
    /// 36 tiles per side.
    Small,
    /// 72 tiles per side.
    Medium,
    /// 108 tiles per side.
    Large,
    /// 144 tiles per side.
    ExtraLarge,
}
impl MapSize {
    /// Side length in tiles, sourced from `EMapDimension`.
    #[must_use]
    pub const fn dimension(self) -> u32 {
        match self {
            Self::Small => raw::MAP_DIMENSION_SMALL,
            Self::Medium => raw::MAP_DIMENSION_MEDIUM,
            Self::Large => raw::MAP_DIMENSION_LARGE,
            Self::ExtraLarge => raw::MAP_DIMENSION_EXTRA_LARGE,
        }
    }

    fn parse(width: i32, height: i32) -> Result<Self, InputError> {
        if width != height {
            return Err(invalid("square map height", height));
        }
        match u32::try_from(width).ok() {
            Some(raw::MAP_DIMENSION_SMALL) => Ok(Self::Small),
            Some(raw::MAP_DIMENSION_MEDIUM) => Ok(Self::Medium),
            Some(raw::MAP_DIMENSION_LARGE) => Ok(Self::Large),
            Some(raw::MAP_DIMENSION_EXTRA_LARGE) => Ok(Self::ExtraLarge),
            _ => Err(invalid("map width", width)),
        }
    }
}

/// Whether a map includes an underground plane.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Levels {
    /// Surface only.
    Surface,
    /// Surface and underground.
    Underground,
}
impl Levels {
    /// Number of planes.
    #[must_use]
    pub const fn count(self) -> u32 {
        match self {
            Self::Surface => 1,
            Self::Underground => raw::RMG_MAP_LEVEL_COUNT,
        }
    }
    /// Present planes, surface first.
    pub fn iter(self) -> impl Iterator<Item = Level> {
        let planes: &[Level] = match self {
            Self::Surface => &[Level::Surface],
            Self::Underground => &[Level::Surface, Level::Underground],
        };
        planes.iter().copied()
    }
}

/// Requested map format; the request ordinals differ from H3M wire versions.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum MapVersion {
    /// Restoration of Erathia.
    Restoration,
    /// Armageddon's Blade.
    ArmageddonsBlade,
    /// Shadow of Death / Complete.
    ShadowOfDeath,
}
impl MapVersion {
    fn parse(value: u32) -> Result<Self, InputError> {
        match value {
            raw::RMG_MAP_RESTORATION_OF_ERATHIA => Ok(Self::Restoration),
            raw::RMG_MAP_ARMAGEDDONS_BLADE => Ok(Self::ArmageddonsBlade),
            raw::RMG_MAP_SHADOW_OF_DEATH => Ok(Self::ShadowOfDeath),
            _ => Err(invalid("map version", value)),
        }
    }
}

/// An actual town faction; neither the random nor neutral sentinel is a town.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Town(u8);
impl Town {
    /// Parse a Complete-era faction ID.
    ///
    /// # Errors
    /// Rejects the neutral sentinel and values outside the known town domain.
    pub fn parse(value: i32) -> Result<Self, InputError> {
        if !(raw::TOWN_CASTLE..=raw::TOWN_CONFLUX).contains(&value) {
            return Err(invalid("town", value));
        }
        Ok(Self(
            u8::try_from(value).map_err(|_| invalid("town", value))?,
        ))
    }

    /// Original faction ordinal for tables and serialization.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0 as usize
    }

    #[allow(clippy::cast_possible_truncation, clippy::cast_sign_loss)]
    const fn known(value: raw::TTownType) -> Self {
        assert!(raw::TOWN_CASTLE <= value && value <= raw::TOWN_CONFLUX);
        Self(value as u8)
    }

    /// Castle.
    pub const CASTLE: Self = Self::known(raw::TOWN_CASTLE);
    /// Rampart.
    pub const RAMPART: Self = Self::known(raw::TOWN_RAMPART);
    /// Tower.
    pub const TOWER: Self = Self::known(raw::TOWN_TOWER);
    /// Inferno.
    pub const INFERNO: Self = Self::known(raw::TOWN_INFERNO);
    /// Necropolis.
    pub const NECROPOLIS: Self = Self::known(raw::TOWN_NECROPOLIS);
    /// Dungeon.
    pub const DUNGEON: Self = Self::known(raw::TOWN_DUNGEON);
    /// Stronghold.
    pub const STRONGHOLD: Self = Self::known(raw::TOWN_STRONGHOLD);
    /// Fortress.
    pub const FORTRESS: Self = Self::known(raw::TOWN_FORTRESS);
    /// Conflux, absent from `RoE` maps.
    pub const CONFLUX: Self = Self::known(raw::TOWN_CONFLUX);

    /// Every Complete-era faction in native order.
    pub const ALL: [Self; raw::TOWN_TYPE_COUNT as usize] = [
        Self::CASTLE,
        Self::RAMPART,
        Self::TOWER,
        Self::INFERNO,
        Self::NECROPOLIS,
        Self::DUNGEON,
        Self::STRONGHOLD,
        Self::FORTRESS,
        Self::CONFLUX,
    ];

    /// Terrain painted for a native-terrain zone of this alignment.
    #[must_use]
    pub const fn native_terrain(self) -> Terrain {
        NATIVE_TERRAIN[self.index()]
    }
}
impl Ordinal for Town {
    const ALL: &'static [Self] = &Self::ALL;
    fn index(self) -> usize {
        Self::index(self)
    }
}

// Source table admitted once at compile time instead of at each lookup.
const NATIVE_TERRAIN: [Terrain; raw::TOWN_TYPE_COUNT as usize] = {
    let mut terrain = [Terrain::Dirt; raw::TOWN_TYPE_COUNT as usize];
    let mut town = 0;
    while town < terrain.len() {
        #[allow(clippy::cast_possible_wrap)] // small source terrain IDs
        let value = raw::NATIVE_TERRAIN[town] as i32;
        terrain[town] = match Terrain::parse(value) {
            Some(value) => value,
            None => panic!("native town terrain outside the terrain domain"),
        };
        town += 1;
    }
    terrain
};

/// A requested faction, before any random selection.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TownChoice {
    /// Resolve at the original generation stage.
    Random,
    /// A specific faction.
    Fixed(Town),
}

/// Water settings after a random request has been resolved.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Water {
    /// No added water.
    None,
    /// Ordinary water zones.
    Normal,
    /// Island layout.
    Islands,
}

/// Water choice without consuming the RNG during parsing.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum WaterChoice {
    /// Resolve at generator construction, before template selection.
    Random,
    /// A fixed water setting.
    Fixed(Water),
}

/// A bounded count of players or teams, including zero.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PlayerCount(u8);
impl PlayerCount {
    fn parse(value: i32, field: &'static str) -> Result<Self, InputError> {
        let count = u8::try_from(value).map_err(|_| invalid(field, value))?;
        if usize::from(count) > PLAYER_COUNT {
            return Err(invalid(field, value));
        }
        Ok(Self(count))
    }

    /// The bounded count.
    #[must_use]
    pub const fn get(self) -> u8 {
        self.0
    }
}

/// The entry wrapper's resolved monster strength, in 1..=5.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct MonsterStrength(u8);
impl MonsterStrength {
    /// Strength after the lobby offset and retail clamp.
    #[must_use]
    pub const fn get(self) -> u8 {
        self.0
    }
}

/// A parsed request whose scalar and player relationships are established.
///
/// The original record is retained for exact failure/post-call reporting.
/// Generation reads only the typed fields. A small copied record costs no heap
/// allocation and preserves noncanonical nonzero human-seat bytes verbatim.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Request {
    original: raw::TRandomMapRequest,
    behavior: Behavior,
    size: MapSize,
    levels: Levels,
    version: MapVersion,
    human_players: PlayerCount,
    computer_players: PlayerCount,
    human_teams: PlayerCount,
    computer_teams: PlayerCount,
    human_seats: PerColour<bool>,
    towns: PerColour<TownChoice>,
    water: WaterChoice,
    strength: MonsterStrength,
}

impl Request {
    /// Parse and resolve deterministic entry repairs without consuming RNG.
    ///
    /// # Errors
    /// Rejects unsupported dimensions/domains or unsafe player relationships.
    /// Hotfix additionally rejects out-of-lobby monster strengths and factions
    /// unavailable in the requested map version, as its C++ entry check does.
    #[expect(
        clippy::missing_panics_doc,
        reason = "conversions follow bounds checks or convert fixed generated constants"
    )]
    pub fn parse(original: raw::TRandomMapRequest, behavior: Behavior) -> Result<Self, InputError> {
        let size = MapSize::parse(original.m_width, original.m_height)?;
        let levels = match original.m_levels {
            1 => Levels::Surface,
            2 => Levels::Underground,
            other => return Err(invalid("map levels", other)),
        };
        let mut human_players = PlayerCount::parse(original.m_humanPlayerCount, "human count")?;
        let mut computer_players =
            PlayerCount::parse(original.m_computerPlayerCount, "computer count")?;
        let total = human_players.get() + computer_players.get();
        if usize::from(total) > PLAYER_COUNT {
            return Err(invalid("total players", total));
        }
        if total < 2 {
            human_players = PlayerCount(1);
            computer_players = PlayerCount(1);
        }
        let human_seats = original.m_isHumanSeat.map(|value| value != 0);
        let fixed_humans = human_seats.iter().filter(|&&human| human).count();
        if behavior.is_hotfix() && fixed_humans > usize::from(human_players.get()) {
            return Err(invalid(
                "fixed human seats",
                i64::try_from(fixed_humans).unwrap(),
            ));
        }
        let human_teams = PlayerCount::parse(original.m_humanTeamCount, "human teams")?;
        let computer_teams = PlayerCount::parse(original.m_computerTeamCount, "computer teams")?;
        let water = match original.m_waterContent {
            raw::RMG_WATER_NONE => WaterChoice::Fixed(Water::None),
            raw::RMG_WATER_NORMAL => WaterChoice::Fixed(Water::Normal),
            raw::RMG_WATER_ISLANDS => WaterChoice::Fixed(Water::Islands),
            raw::RMG_WATER_RANDOM => WaterChoice::Random,
            other => return Err(invalid("water choice", other)),
        };
        let average = const { crate::constants::signed(raw::RMG_ZONE_MONSTERS_AVERAGE) };
        let strongest = const { crate::constants::signed(raw::RMG_STRONGEST_GUARD_STRENGTH) };
        let shifted_strength = original
            .m_monsterStrength
            .checked_add(average)
            .ok_or_else(|| invalid("monster strength overflow", original.m_monsterStrength))?;
        if behavior.is_hotfix() && !(1..=strongest).contains(&shifted_strength) {
            return Err(invalid("monster strength", original.m_monsterStrength));
        }
        let strength = MonsterStrength(u8::try_from(shifted_strength.clamp(1, strongest)).unwrap());
        let version = MapVersion::parse(original.m_mapVersion)?;
        let mut towns = [TownChoice::Random; PLAYER_COUNT];
        for (target, value) in towns.iter_mut().zip(original.m_townChoices) {
            if value == raw::eTownNeutral {
                continue;
            }
            let town = Town::parse(value)?;
            if behavior.is_hotfix()
                && version == MapVersion::Restoration
                && value == raw::TOWN_CONFLUX
            {
                return Err(invalid("RoE town", value));
            }
            *target = TownChoice::Fixed(town);
        }
        Ok(Self {
            original,
            behavior,
            size,
            levels,
            version,
            human_players,
            computer_players,
            human_teams,
            computer_teams,
            human_seats: PerColour::new(human_seats),
            towns: PerColour::new(towns),
            water,
            strength,
        })
    }

    /// Settings as originally supplied, before the minimum-player repair.
    #[must_use]
    pub const fn original(&self) -> &raw::TRandomMapRequest {
        &self.original
    }

    /// Post-entry record once request parsing has succeeded and repairs apply.
    #[must_use]
    pub fn repaired_record(&self) -> raw::TRandomMapRequest {
        let mut result = self.original;
        result.m_humanPlayerCount = i32::from(self.human_players.get());
        result.m_computerPlayerCount = i32::from(self.computer_players.get());
        result
    }

    /// Behavior selected while preparing this request.
    #[must_use]
    pub const fn behavior(&self) -> Behavior {
        self.behavior
    }
    /// Supported square dimensions.
    #[must_use]
    pub const fn size(&self) -> MapSize {
        self.size
    }
    /// Map planes.
    #[must_use]
    pub const fn levels(&self) -> Levels {
        self.levels
    }
    /// Requested file format.
    #[must_use]
    pub const fn version(&self) -> MapVersion {
        self.version
    }
    /// Human-capable players after the minimum-player repair.
    #[must_use]
    pub const fn human_players(&self) -> PlayerCount {
        self.human_players
    }
    /// Computer-only players after the minimum-player repair.
    #[must_use]
    pub const fn computer_players(&self) -> PlayerCount {
        self.computer_players
    }
    /// Requested human team count; final clamping occurs during serialization.
    #[must_use]
    pub const fn human_teams(&self) -> PlayerCount {
        self.human_teams
    }
    /// Requested computer team count; final clamping occurs during serialization.
    #[must_use]
    pub const fn computer_teams(&self) -> PlayerCount {
        self.computer_teams
    }
    /// Player colours that must be assigned human-capable slots.
    #[must_use]
    pub const fn human_seats(&self) -> &PerColour<bool> {
        &self.human_seats
    }
    /// Requested factions, without drawing random choices.
    #[must_use]
    pub const fn towns(&self) -> &PerColour<TownChoice> {
        &self.towns
    }
    /// Requested water, without drawing random choices.
    #[must_use]
    pub const fn water(&self) -> WaterChoice {
        self.water
    }
    /// Entry-normalized monster strength.
    #[must_use]
    pub const fn strength(&self) -> MonsterStrength {
        self.strength
    }
}

/// Mirror the C++ request constructor's defaults, using generated domains.
#[must_use]
#[expect(
    clippy::missing_panics_doc,
    reason = "closed size/level domains and the fixed player count fit i32"
)]
pub fn default_record(size: MapSize, levels: Levels) -> raw::TRandomMapRequest {
    raw::TRandomMapRequest {
        m_isHumanSeat: [0; PLAYER_COUNT],
        m_townChoices: [raw::eTownNeutral; PLAYER_COUNT],
        m_width: i32::try_from(size.dimension()).unwrap(),
        m_height: i32::try_from(size.dimension()).unwrap(),
        m_levels: i32::try_from(levels.count()).unwrap(),
        m_humanPlayerCount: 2,
        m_humanTeamCount: 2,
        m_computerPlayerCount: 0,
        m_computerTeamCount: i32::try_from(PLAYER_COUNT).unwrap(),
        m_waterContent: raw::RMG_WATER_RANDOM,
        m_monsterStrength: 0,
        m_mapVersion: raw::RMG_MAP_SHADOW_OF_DEATH,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::behavior::RetailProfile;

    fn default() -> raw::TRandomMapRequest {
        default_record(MapSize::Small, Levels::Surface)
    }

    #[test]
    fn repair_uses_the_effective_human_count() {
        for (humans, computers) in [(0, 0), (0, 1), (1, 0)] {
            let mut record = default();
            record.m_humanPlayerCount = humans;
            record.m_computerPlayerCount = computers;
            record.m_isHumanSeat[0] = 255;
            let request = Request::parse(record, Behavior::Hotfix).unwrap();
            assert_eq!(request.human_players().get(), 1);
            assert_eq!(request.computer_players().get(), 1);
            assert_eq!(request.original(), &record);
            assert_eq!(request.repaired_record().m_isHumanSeat[0], 255);
            record.m_isHumanSeat[1] = 1;
            assert!(Request::parse(record, Behavior::Hotfix).is_err());
        }
    }

    #[test]
    fn zero_humans_and_excess_requested_teams_are_accepted() {
        let mut record = default();
        record.m_humanPlayerCount = 0;
        record.m_computerPlayerCount = 8;
        record.m_humanTeamCount = 8;
        assert_eq!(
            Request::parse(record, Behavior::Hotfix)
                .unwrap()
                .human_players()
                .get(),
            0
        );
    }

    #[test]
    fn mode_differences_are_explicit() {
        let mut record = default();
        record.m_monsterStrength = 50;
        assert!(Request::parse(record, Behavior::Hotfix).is_err());
        let retail = Behavior::Retail(RetailProfile::default());
        assert_eq!(Request::parse(record, retail).unwrap().strength().get(), 5);
        record.m_monsterStrength = 0;
        record.m_mapVersion = raw::RMG_MAP_RESTORATION_OF_ERATHIA;
        record.m_townChoices[0] = raw::TOWN_CONFLUX;
        assert!(Request::parse(record, Behavior::Hotfix).is_err());
        assert!(Request::parse(record, retail).is_ok());
    }

    #[test]
    fn malformed_fields_never_enter_the_domain() {
        let mut cases = [default(); 7];
        cases[0].m_height = 72;
        cases[1].m_levels = 0;
        cases[2].m_mapVersion = u32::MAX;
        cases[3].m_waterContent = u32::MAX;
        cases[4].m_townChoices[0] = -2;
        cases[5].m_computerPlayerCount = 7;
        cases[6].m_monsterStrength = i32::MAX;
        for record in cases {
            assert!(Request::parse(record, Behavior::Hotfix).is_err());
        }
    }
}
