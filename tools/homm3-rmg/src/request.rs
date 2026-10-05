//! Parse the lobby record into supported generation domains.

use crate::{behavior::Behavior, hero::HeroId, raw, rules::Ruleset};
use std::{error::Error, fmt};

/// Number of player colours, obtained from the shared C++ declaration.
pub const PLAYER_COUNT: usize = raw::RMG_PLAYER_COUNT as usize;

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
    /// 180 tiles per side (`HotA`).
    Huge,
    /// 216 tiles per side (`HotA`).
    ExtraHuge,
    /// 252 tiles per side (`HotA`).
    Giant,
}
impl MapSize {
    /// Side length from `EMapDimension` or `HotA`'s extended size controls.
    #[must_use]
    pub const fn dimension(self) -> u32 {
        match self {
            Self::Small => raw::MAP_DIMENSION_SMALL,
            Self::Medium => raw::MAP_DIMENSION_MEDIUM,
            Self::Large => raw::MAP_DIMENSION_LARGE,
            Self::ExtraLarge => raw::MAP_DIMENSION_EXTRA_LARGE,
            // Pinned DLL RVA 0x181da0: size writes at 0x181eab/0x181efc/0x181f4d.
            Self::Huge => 180,
            Self::ExtraHuge => 216,
            Self::Giant => 252,
        }
    }

    fn parse(width: i32, height: i32, rules: Ruleset) -> Result<Self, InputError> {
        if width != height {
            return Err(invalid("square map height", height));
        }
        match u32::try_from(width).ok() {
            Some(raw::MAP_DIMENSION_SMALL) => Ok(Self::Small),
            Some(raw::MAP_DIMENSION_MEDIUM) => Ok(Self::Medium),
            Some(raw::MAP_DIMENSION_LARGE) => Ok(Self::Large),
            Some(raw::MAP_DIMENSION_EXTRA_LARGE) => Ok(Self::ExtraLarge),
            Some(180) if rules == Ruleset::HotA181 => Ok(Self::Huge),
            Some(216) if rules == Ruleset::HotA181 => Ok(Self::ExtraHuge),
            Some(252) if rules == Ruleset::HotA181 => Ok(Self::Giant),
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
        Self::parse_for(value, crate::rules::Ruleset::Complete)
    }

    /// Parse a faction in the selected release's domain.
    ///
    /// # Errors
    /// Rejects sentinels and IDs outside the release's town catalog.
    pub fn parse_for(value: i32, rules: Ruleset) -> Result<Self, InputError> {
        if !usize::try_from(value).is_ok_and(|id| id < rules.town_count()) {
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
}

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

/// Inputs supplied outside the original lobby record by expansion hooks.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct RequestOptions {
    /// Native generation rules, independent of map-format ordinals.
    pub ruleset: Ruleset,
    /// Resolved HW mirror mode; pack parsing alone does not enable it.
    pub mirror: bool,
    /// Explicit starting heroes for all eight player colours. `None` is random.
    pub starting_heroes: [Option<HeroId>; PLAYER_COUNT],
}

/// Constructor-local dimensions and counts after the mirror hooks.
/// These do not replace the caller's record or its post-entry player counts.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct GeneratorParameters {
    /// Planes built before mirror output duplication.
    pub levels: Levels,
    /// Human-capable slots built by this generator.
    pub human_players: PlayerCount,
    /// Computer-only slots built by this generator.
    pub computer_players: PlayerCount,
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
    options: RequestOptions,
    size: MapSize,
    levels: Levels,
    version: MapVersion,
    human_players: PlayerCount,
    computer_players: PlayerCount,
    human_teams: PlayerCount,
    computer_teams: PlayerCount,
    human_seats: [bool; PLAYER_COUNT],
    towns: [TownChoice; PLAYER_COUNT],
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
    pub fn parse(original: raw::TRandomMapRequest, behavior: Behavior) -> Result<Self, InputError> {
        Self::parse_with_options(original, behavior, RequestOptions::default())
    }

    /// Parse versioned request inputs without consuming randomness.
    ///
    /// `HotA`'s constructor coerces water outside 0..=2 to none. HW mirror mode
    /// skips the entry minimum-player repair; its constructor changes are exposed
    /// separately by [`Self::constructor_parameters`].
    ///
    /// # Errors
    /// Rejects malformed domains, expansion options with Complete rules, or the
    /// Complete-only hotfix policy with the pinned `HotA` rules.
    #[expect(
        clippy::missing_panics_doc,
        reason = "conversions follow bounds checks or convert fixed generated constants"
    )]
    pub fn parse_with_options(
        original: raw::TRandomMapRequest,
        behavior: Behavior,
        options: RequestOptions,
    ) -> Result<Self, InputError> {
        let rules = options.ruleset;
        if rules == Ruleset::Complete {
            if options.mirror {
                return Err(invalid("Complete mirror mode", 1));
            }
            if let Some(hero) = options.starting_heroes.iter().flatten().next() {
                return Err(invalid(
                    "Complete starting hero",
                    i64::try_from(hero.index()).unwrap(),
                ));
            }
        } else if behavior.is_hotfix() {
            return Err(invalid("HotA Complete-hotfix policy", 1));
        }
        let size = MapSize::parse(original.m_width, original.m_height, rules)?;
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
        if total < 2 && !options.mirror {
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
            // HotA DLL RVA 0x1b8f30, before the base constructor's random draw.
            _ if rules == Ruleset::HotA181 => WaterChoice::Fixed(Water::None),
            raw::RMG_WATER_RANDOM => WaterChoice::Random,
            other => return Err(invalid("water choice", other)),
        };
        let average = i32::try_from(raw::RMG_ZONE_MONSTERS_AVERAGE).unwrap();
        let strongest = i32::try_from(raw::RMG_STRONGEST_GUARD_STRENGTH).unwrap();
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
            let town = Town::parse_for(value, rules)?;
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
            options,
            size,
            levels,
            version,
            human_players,
            computer_players,
            human_teams,
            computer_teams,
            human_seats,
            towns,
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
    /// Native generation rules admitted with this request.
    #[must_use]
    pub const fn ruleset(&self) -> Ruleset {
        self.options.ruleset
    }
    /// Resolved mirror mode supplied by the caller.
    #[must_use]
    pub const fn mirror(&self) -> bool {
        self.options.mirror
    }
    /// Starting-hero claims, including colours beyond the generated half.
    #[must_use]
    pub const fn starting_heroes(&self) -> &[Option<HeroId>; PLAYER_COUNT] {
        &self.options.starting_heroes
    }
    /// Constructor-local values; `HotA`'s outer setup still receives the entry counts.
    #[must_use]
    pub fn constructor_parameters(&self) -> GeneratorParameters {
        // HW_HOTA RVA 0x19220, inside HotA's outer constructor splice.
        if self.mirror() {
            GeneratorParameters {
                levels: Levels::Surface,
                human_players: PlayerCount((self.human_players.get() / 2).max(1)),
                computer_players: PlayerCount(self.computer_players.get() / 2),
            }
        } else {
            GeneratorParameters {
                levels: self.levels,
                human_players: self.human_players,
                computer_players: self.computer_players,
            }
        }
    }
    /// Town choices after HW's generation-entry compaction, retaining unused slots.
    ///
    /// # Errors
    /// Reports a mirror colour-pair read outside the eight-slot request. Native
    /// code can overrun for zero humans and eight computers; no faction is invented.
    pub fn generation_towns(&self) -> Result<[TownChoice; PLAYER_COUNT], InputError> {
        let mut towns = self.towns;
        if self.mirror() {
            // HW_HOTA RVA 0x191c0; runs after constructor/template preparation.
            let parameters = self.constructor_parameters();
            let count =
                usize::from(parameters.human_players.get() + parameters.computer_players.get());
            for (index, town) in (0_u8..).zip(towns.iter_mut().take(count)) {
                let source = index * 2;
                *town = *self
                    .towns
                    .get(usize::from(source))
                    .ok_or_else(|| invalid("mirror town source slot", source))?;
            }
        }
        Ok(towns)
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
    pub const fn human_seats(&self) -> &[bool; PLAYER_COUNT] {
        &self.human_seats
    }
    /// Requested factions, without drawing random choices.
    #[must_use]
    pub const fn towns(&self) -> &[TownChoice; PLAYER_COUNT] {
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

    fn hota(record: raw::TRandomMapRequest, mirror: bool) -> Request {
        Request::parse_with_options(
            record,
            Behavior::Retail(RetailProfile::default()),
            RequestOptions {
                ruleset: Ruleset::HotA181,
                mirror,
                ..RequestOptions::default()
            },
        )
        .unwrap()
    }

    #[test]
    fn expanded_dimensions_and_towns_require_expansion_rules() {
        let retail = Behavior::Retail(RetailProfile::default());
        for size in [MapSize::Huge, MapSize::ExtraHuge, MapSize::Giant] {
            let record = default_record(size, Levels::Underground);
            assert!(Request::parse(record, retail).is_err());
            let request = hota(record, false);
            assert_eq!(request.size(), size);
            assert_eq!(request.constructor_parameters().levels, Levels::Underground);
            assert_eq!(request.repaired_record(), record);
        }
        for id in 9..12 {
            let mut record = default();
            record.m_townChoices[7] = id;
            assert!(Request::parse(record, retail).is_err());
            assert_eq!(
                hota(record, false).towns()[7],
                TownChoice::Fixed(Town::parse_for(id, Ruleset::HotA181).unwrap())
            );
        }
        assert!(Town::parse_for(12, Ruleset::HotA181).is_err());
    }

    #[test]
    fn hota_water_coercion_preserves_the_record_and_consumes_no_draws() {
        use crate::{rng::RetailRng, selection::resolve_water};
        for (value, expected) in [
            (0, Water::None),
            (1, Water::Normal),
            (2, Water::Islands),
            (3, Water::None),
            (4, Water::None),
            (u32::MAX, Water::None),
        ] {
            let mut record = default();
            record.m_waterContent = value;
            let request = hota(record, false);
            let mut rng = RetailRng::new(12345);
            assert_eq!(resolve_water(request.water(), &mut rng), expected);
            assert_eq!(rng, RetailRng::new(12345));
            assert_eq!(request.repaired_record(), record);
        }
    }

    #[test]
    fn mirror_constructor_changes_do_not_rewrite_the_callers_request() {
        for (humans, computers, half_humans, half_computers) in [
            (0, 0, 1, 0),
            (1, 0, 1, 0),
            (2, 0, 1, 0),
            (3, 3, 1, 1),
            (2, 6, 1, 3),
            (8, 0, 4, 0),
        ] {
            let mut record = default_record(MapSize::Large, Levels::Underground);
            record.m_humanPlayerCount = humans;
            record.m_computerPlayerCount = computers;
            record.m_isHumanSeat[6] = 255;
            record.m_townChoices = [0, 1, 2, 3, 4, 5, 6, 7];
            let request = hota(record, true);
            assert_eq!(request.repaired_record(), record);
            assert_eq!(request.human_players().get(), humans as u8);
            assert_eq!(request.computer_players().get(), computers as u8);
            assert_eq!(request.levels(), Levels::Underground);
            let parameters = request.constructor_parameters();
            assert_eq!(parameters.levels, Levels::Surface);
            assert_eq!(parameters.human_players.get(), half_humans);
            assert_eq!(parameters.computer_players.get(), half_computers);
            let towns = request.generation_towns().unwrap();
            let count = usize::from(half_humans + half_computers);
            for (index, town) in towns.iter().enumerate() {
                assert_eq!(
                    *town,
                    request.towns()[if index < count { index * 2 } else { index }]
                );
            }
            assert_eq!(
                request.towns()[7],
                TownChoice::Fixed(Town::parse(7).unwrap())
            );
        }
        let mut record = default();
        record.m_humanPlayerCount = 0;
        let ordinary = hota(record, false);
        assert_eq!(ordinary.human_players().get(), 1);
        assert_eq!(ordinary.computer_players().get(), 1);
        record.m_computerPlayerCount = 8;
        let mirror = hota(record, true);
        assert_eq!(mirror.repaired_record(), record);
        assert_eq!(
            mirror.generation_towns(),
            Err(invalid("mirror town source slot", 8))
        );
    }

    #[test]
    fn expansion_options_are_explicit_and_keep_all_starting_heroes() {
        let retail = Behavior::Retail(RetailProfile::default());
        let mut options = RequestOptions {
            mirror: true,
            ..RequestOptions::default()
        };
        assert!(Request::parse_with_options(default(), retail, options).is_err());
        options.mirror = false;
        options.starting_heroes[7] = HeroId::parse(42);
        assert!(Request::parse_with_options(default(), retail, options).is_err());
        options.ruleset = Ruleset::HotA181;
        options.mirror = true;
        assert!(Request::parse_with_options(default(), Behavior::Hotfix, options).is_err());
        let request = Request::parse_with_options(default(), retail, options).unwrap();
        assert_eq!(request.ruleset(), Ruleset::HotA181);
        assert_eq!(request.starting_heroes(), &options.starting_heroes);
        assert_eq!(request.constructor_parameters().human_players.get(), 1);
    }
}
