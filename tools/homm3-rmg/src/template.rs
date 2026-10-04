//! Request-specific preparation of `rmg.txt` into linked, typed templates.

use crate::{
    geometry::ZoneId,
    raw,
    request::{MapVersion, Request, Water, PLAYER_COUNT},
};
use homm3_resource::{Field, Spreadsheet, SpreadsheetRow};
use std::{borrow::Cow, error::Error, fmt, num::NonZeroU32};

const COLUMNS: usize = raw::RMG_TEMPLATE_COLUMN_CONNECTION_MAXIMUM_PLAYERS as usize + 1;
const TOWNS: usize = raw::TOWN_TYPE_COUNT as usize;
const RESOURCES: usize = raw::NUM_RESOURCES as usize;
const LAND_TERRAINS: usize = raw::eTerrainWater as usize;
const BANDS: usize = raw::RMG_TREASURE_BAND_COUNT as usize;

/// A source row cannot be interpreted safely as a template record.
#[derive(Debug)]
pub enum TemplateError {
    /// The underlying spreadsheet encoding is malformed.
    Spreadsheet(homm3_resource::Error),
    /// An integer field exceeds the signed 32-bit domain of retail `atoi`.
    IntegerOverflow {
        /// Zero-based source row.
        row: usize,
        /// Zero-based source column.
        column: u32,
    },
    /// Retail would read a column beyond the end of a row.
    RetailShortRow {
        /// Zero-based source row.
        row: usize,
        /// The missing column.
        column: u32,
    },
}
impl fmt::Display for TemplateError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Spreadsheet(error) => error.fmt(f),
            Self::IntegerOverflow { row, column } => {
                write!(f, "rmg.txt row {row}, column {column}: integer overflow")
            }
            Self::RetailShortRow { row, column } => {
                write!(f, "rmg.txt row {row}: retail reads missing column {column}")
            }
        }
    }
}
impl Error for TemplateError {}

/// A candidate retail accepts, but cannot execute safely.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum RetailTemplateFault {
    /// A starting zone writes before retail's player-slot arrays (`owner == -1`).
    UnassignedPlayerZone {
        /// Zero-based source row.
        row: usize,
    },
    /// A zone has an unsupported size or player index.
    UnusableZone {
        /// Zero-based source row.
        row: usize,
    },
}
impl fmt::Display for RetailTemplateFault {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::UnassignedPlayerZone { row } => write!(
                f,
                "rmg.txt row {row}: unassigned player zone writes before retail player-slot arrays"
            ),
            Self::UnusableZone { row } => write!(f, "rmg.txt row {row}: unsupported retail zone"),
        }
    }
}
impl Error for RetailTemplateFault {}

/// A selectable candidate, including retail's potentially faulting templates.
///
/// Faults remain in source order and occupy a selection slot. Removing them
/// would change the modulo divisor and every later seeded template choice.
#[derive(Debug)]
pub enum TemplateCandidate<'a> {
    /// A template whose zone domains have been resolved.
    Ready(Template<'a>),
    /// Retail accepts this candidate; report its fault only when selected.
    RetailFault {
        /// Original resource name.
        name: Cow<'a, [u8]>,
        /// Failure encountered when resolving the zone domains.
        fault: RetailTemplateFault,
    },
}
impl<'a> TemplateCandidate<'a> {
    /// Original byte name, including for a deferred fault.
    #[must_use]
    pub fn name(&self) -> &[u8] {
        match self {
            Self::Ready(template) => template.name(),
            Self::RetailFault { name, .. } => name,
        }
    }

    /// Resolve the selected candidate without another allocation.
    ///
    /// # Errors
    /// Reports the selected candidate's modeled retail fault.
    pub fn into_template(self) -> Result<Template<'a>, RetailTemplateFault> {
        match self {
            Self::Ready(template) => Ok(template),
            Self::RetailFault { fault, .. } => Err(fault),
        }
    }
}

struct Row<'a> {
    fields: [Option<Field<'a>>; COLUMNS],
    count: usize,
    index: usize,
}
impl<'a> Row<'a> {
    fn new(row: SpreadsheetRow<'a>, index: usize) -> Self {
        let mut fields = [None; COLUMNS];
        for (target, field) in fields.iter_mut().zip(row.cells()) {
            *target = Some(field);
        }
        Self {
            fields,
            count: row.len(),
            index,
        }
    }
    fn bytes(&self, column: u32) -> impl Iterator<Item = u8> + '_ {
        self.fields[column as usize]
            .into_iter()
            .flat_map(Field::decoded)
    }
    fn set(&self, column: u32) -> bool {
        self.bytes(column)
            .next()
            .is_some_and(|byte| byte != 0 && byte != b' ')
    }
    fn number(&self, column: u32) -> Result<i32, TemplateError> {
        crate::parse::integer(self.bytes(column)).ok_or(TemplateError::IntegerOverflow {
            row: self.index,
            column,
        })
    }

    fn name(&self) -> Cow<'a, [u8]> {
        let Some(field) = self.fields[raw::RMG_TEMPLATE_COLUMN_NAME as usize] else {
            return Cow::Borrowed(&[]);
        };
        let encoded = field.encoded();
        if encoded.contains(&b'"') {
            Cow::Owned(field.decoded().take_while(|&byte| byte != 0).collect())
        } else {
            Cow::Borrowed(encoded.split(|&byte| byte == 0).next().unwrap_or_default())
        }
    }
    fn allows_players(&self, first: u32, request: &Request) -> Result<bool, TemplateError> {
        let humans = i32::from(request.human_players().get());
        let total = humans + i32::from(request.computer_players().get());
        Ok(self.number(first)? <= humans
            && self.number(first + raw::RMG_PLAYER_LIMIT_MAXIMUM_HUMAN_PLAYERS)? >= humans
            && self.number(first + raw::RMG_PLAYER_LIMIT_MINIMUM_PLAYERS)? <= total
            && self.number(first + raw::RMG_PLAYER_LIMIT_MAXIMUM_PLAYERS)? >= total)
    }

    fn kind(&self) -> u32 {
        // Later kind columns override earlier ones; absence defaults to treasure.
        (raw::RMG_TEMPLATE_COLUMN_KIND_HUMAN..=raw::RMG_TEMPLATE_COLUMN_KIND_JUNCTION)
            .rev()
            .find(|&column| self.set(column))
            .unwrap_or(raw::RMG_TEMPLATE_COLUMN_KIND_TREASURE)
    }
}

/// One of the eight template player numbers, represented internally from zero.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PlayerSlot(u8);
impl PlayerSlot {
    /// Index into the template's player-to-colour mapping.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0 as usize
    }
}

/// Player zones require a player slot; other zones may have no owner.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ZoneRole {
    /// Human-capable starting region.
    Human(PlayerSlot),
    /// Computer-only starting region.
    Computer(PlayerSlot),
    /// Treasure region and its optional owner.
    Treasure(Option<PlayerSlot>),
    /// Junction region and its optional owner.
    Junction(Option<PlayerSlot>),
}

/// A placement category with an initial count and optional positive density.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Placement {
    /// Negative initial counts are retained; retail's placement loop skips them.
    pub initial_count: i32,
    /// Nonpositive source densities disable further placements.
    pub density: Option<NonZeroU32>,
}

/// A treasure range, preserving retail's treatment of reversed endpoints.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct TreasureBand {
    /// Lower endpoint; maximum <= minimum selects maximum without a draw.
    pub minimum: i32,
    /// Exclusive upper endpoint when it exceeds minimum.
    pub maximum: i32,
    /// Nonpositive density or maximum below 100 disables this band.
    pub density: Option<NonZeroU32>,
}

/// Template monster strength before the global request modifier.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ZoneMonsters {
    /// No guards.
    None,
    /// Weak guards.
    Weak,
    /// Average guards; also the parser default.
    Average,
    /// Strong guards.
    Strong,
}

/// A connection already resolved to a zone in its owning template.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Connection {
    destination: ZoneId,
    value: i32,
    unguarded: bool,
    border_guard: bool,
}
impl Connection {
    /// Connected zone in this prepared template.
    #[must_use]
    pub const fn destination(self) -> ZoneId {
        self.destination
    }
    /// Unscaled guard value.
    #[must_use]
    pub const fn value(self) -> i32 {
        self.value
    }
    /// Wide connection, without the ordinary monster guard.
    #[must_use]
    pub const fn unguarded(self) -> bool {
        self.unguarded
    }
    /// Whether to try a keymaster border guard.
    #[must_use]
    pub const fn border_guard(self) -> bool {
        self.border_guard
    }
}

/// A parsed, request-filtered zone. Construction resolves its required domains.
#[derive(Debug)]
pub struct Zone {
    id: ZoneId,
    source_number: i32,
    role: ZoneRole,
    size: NonZeroU32,
    towns: [Placement; raw::RMG_TOWN_CATEGORY_COUNT as usize],
    neutral_towns_match_alignment: bool,
    allowed_towns: [bool; TOWNS],
    mines: [Placement; RESOURCES],
    use_native_terrain: bool,
    allowed_terrain: [bool; LAND_TERRAINS],
    monsters: ZoneMonsters,
    guards_match_alignment: bool,
    allowed_monsters: [bool; TOWNS + 1],
    treasure: [TreasureBand; BANDS],
    connections: Vec<Connection>,
}

impl Zone {
    /// Dense identity in the owning template.
    #[must_use]
    pub const fn id(&self) -> ZoneId {
        self.id
    }
    /// Template role with any required player slot.
    #[must_use]
    pub const fn role(&self) -> ZoneRole {
        self.role
    }
    /// Positive unscaled layout size.
    #[must_use]
    pub const fn size(&self) -> NonZeroU32 {
        self.size
    }
    /// Player castle/basic then neutral castle/basic placement categories.
    #[must_use]
    pub const fn towns(&self) -> &[Placement; raw::RMG_TOWN_CATEGORY_COUNT as usize] {
        &self.towns
    }
    /// Whether neutral towns inherit zone alignment.
    #[must_use]
    pub const fn neutral_towns_match_alignment(&self) -> bool {
        self.neutral_towns_match_alignment
    }
    /// Allowed factions after version filtering.
    #[must_use]
    pub const fn allowed_towns(&self) -> &[bool; TOWNS] {
        &self.allowed_towns
    }
    /// Mine categories indexed by resource.
    #[must_use]
    pub const fn mines(&self) -> &[Placement; RESOURCES] {
        &self.mines
    }
    /// Whether town alignment determines terrain.
    #[must_use]
    pub const fn use_native_terrain(&self) -> bool {
        self.use_native_terrain
    }
    /// Allowed land terrains; at least dirt is allowed.
    #[must_use]
    pub const fn allowed_terrain(&self) -> &[bool; LAND_TERRAINS] {
        &self.allowed_terrain
    }
    /// Template guard strength.
    #[must_use]
    pub const fn monsters(&self) -> ZoneMonsters {
        self.monsters
    }
    /// Whether guards must match zone alignment.
    #[must_use]
    pub const fn guards_match_alignment(&self) -> bool {
        self.guards_match_alignment
    }
    /// Neutral then faction guard availability, including the retail `RoE` quirk.
    #[must_use]
    pub const fn allowed_monsters(&self) -> &[bool; TOWNS + 1] {
        &self.allowed_monsters
    }
    /// Treasure bands in source order.
    #[must_use]
    pub const fn treasure(&self) -> &[TreasureBand; BANDS] {
        &self.treasure
    }
    /// Bidirectional connections, retaining source insertion order.
    #[must_use]
    pub fn connections(&self) -> &[Connection] {
        &self.connections
    }
}

/// A template accepted for one prepared request and resolved water choice.
#[derive(Debug)]
pub struct Template<'a> {
    name: Cow<'a, [u8]>,
    zones: Vec<Zone>,
}
impl Template<'_> {
    /// Original byte name; resource text need not be UTF-8.
    #[must_use]
    pub fn name(&self) -> &[u8] {
        &self.name
    }
    /// Zones in original filtered row order.
    #[must_use]
    pub fn zones(&self) -> &[Zone] {
        &self.zones
    }

    /// Distinct human/all slots, used later by player assignment.
    #[must_use]
    pub fn player_slots(&self) -> ([bool; PLAYER_COUNT], [bool; PLAYER_COUNT]) {
        let mut humans = [false; PLAYER_COUNT];
        let mut all = [false; PLAYER_COUNT];
        for zone in &self.zones {
            match zone.role {
                ZoneRole::Human(slot) => {
                    humans[slot.index()] = true;
                    all[slot.index()] = true;
                }
                ZoneRole::Computer(slot) => all[slot.index()] = true,
                _ => {}
            }
        }
        (humans, all)
    }

    fn admits(&self, request: &Request) -> bool {
        let (humans, all) = self.player_slots();
        let humans = humans.into_iter().filter(|&b| b).count();
        let all = all.into_iter().filter(|&b| b).count();
        humans >= usize::from(request.human_players().get())
            && all >= usize::from(request.human_players().get() + request.computer_players().get())
    }
}

/// Borrowed source rows, indexed once for repeated request preparation.
pub struct TemplateSource<'a> {
    rows: Vec<SpreadsheetRow<'a>>,
}
impl<'a> TemplateSource<'a> {
    /// Parse spreadsheet encoding without resolving player filters or randomness.
    ///
    /// # Errors
    /// Reports malformed text encoding.
    pub fn parse(bytes: &'a [u8]) -> Result<Self, TemplateError> {
        let sheet = Spreadsheet::parse(bytes).map_err(TemplateError::Spreadsheet)?;
        Ok(Self {
            rows: sheet.rows().collect(),
        })
    }

    fn append_connections(
        &self,
        template: &mut Template<'a>,
        first: usize,
        end: usize,
        request: &Request,
    ) -> Result<(), TemplateError> {
        for index in first..end {
            let row = Row::new(self.rows[index], index);
            if row.count <= raw::RMG_TEMPLATE_COLUMN_CONNECTION_MAXIMUM_PLAYERS as usize
                || !row.set(raw::RMG_TEMPLATE_COLUMN_CONNECTION_FIRST_ZONE)
                || row
                    .bytes(raw::RMG_TEMPLATE_COLUMN_CONNECTION_SECOND_ZONE)
                    .next()
                    .is_none_or(|b| b == 0)
            {
                continue;
            }
            let first_number = row.number(raw::RMG_TEMPLATE_COLUMN_CONNECTION_FIRST_ZONE)?;
            let second_number = row.number(raw::RMG_TEMPLATE_COLUMN_CONNECTION_SECOND_ZONE)?;
            let first = template
                .zones
                .iter()
                .position(|z| z.source_number == first_number);
            let second = template
                .zones
                .iter()
                .position(|z| z.source_number == second_number);
            if let (Some(first), Some(second)) = (first, second) {
                if !row.allows_players(
                    raw::RMG_TEMPLATE_COLUMN_CONNECTION_MINIMUM_HUMAN_PLAYERS,
                    request,
                )? {
                    continue;
                }
                let connection = Connection {
                    destination: ZoneId::new(second),
                    value: row.number(raw::RMG_TEMPLATE_COLUMN_CONNECTION_VALUE)?,
                    unguarded: row.set(raw::RMG_TEMPLATE_COLUMN_CONNECTION_UNGUARDED),
                    border_guard: row.set(raw::RMG_TEMPLATE_COLUMN_CONNECTION_BORDER_GUARD),
                };
                template.zones[first].connections.push(connection);
                template.zones[second].connections.push(Connection {
                    destination: ZoneId::new(first),
                    ..connection
                });
            }
        }
        Ok(())
    }

    /// Apply the original size, player, version and behavior filters in order.
    ///
    /// # Errors
    /// Reports overflowing input numbers or a retail fault while reading rows.
    /// Faults that require selecting a candidate are deferred in the result.
    pub fn prepare(
        &self,
        request: &Request,
        water: Water,
    ) -> Result<Vec<TemplateCandidate<'a>>, TemplateError> {
        let mut templates = Vec::new();
        let dimension = request.size().dimension();
        let unit = raw::MAP_DIMENSION_SMALL;
        let mut size = dimension * dimension * request.levels().count() / (unit * unit);
        if water == Water::Islands {
            size = (size / 2).max(1);
        }
        let size = i64::from(size);
        let hotfix = request.behavior().is_hotfix();
        let mut row_index = raw::RMG_FIRST_DATA_ROW as usize;
        while row_index < self.rows.len() {
            let first = Row::new(self.rows[row_index], row_index);
            if first.count <= raw::RMG_TEMPLATE_COLUMN_MAXIMUM_SIZE as usize {
                if !hotfix && first.count == raw::RMG_TEMPLATE_COLUMN_MAXIMUM_SIZE as usize {
                    return Err(TemplateError::RetailShortRow {
                        row: row_index,
                        column: raw::RMG_TEMPLATE_COLUMN_MAXIMUM_SIZE,
                    });
                }
                row_index += 1;
                continue;
            }
            let mut end = row_index + 1;
            while end < self.rows.len()
                && !Row::new(self.rows[end], end).set(raw::RMG_TEMPLATE_COLUMN_NAME)
            {
                end += 1;
            }
            if i64::from(first.number(raw::RMG_TEMPLATE_COLUMN_MINIMUM_SIZE)?) <= size
                && i64::from(first.number(raw::RMG_TEMPLATE_COLUMN_MAXIMUM_SIZE)?) >= size
            {
                let mut template = Template {
                    name: first.name(),
                    zones: Vec::new(),
                };
                let mut fault = None;
                let mut human_zones = 0;
                let mut player_zones = 0;
                for index in row_index..end {
                    let row = Row::new(self.rows[index], index);
                    if !hotfix && row.count == raw::RMG_TEMPLATE_COLUMN_ZONE_INDEX as usize {
                        return Err(TemplateError::RetailShortRow {
                            row: index,
                            column: raw::RMG_TEMPLATE_COLUMN_ZONE_INDEX,
                        });
                    }
                    if row.count > raw::RMG_TEMPLATE_COLUMN_LAST_TREASURE_DENSITY as usize
                        && row.set(raw::RMG_TEMPLATE_COLUMN_ZONE_INDEX)
                        && row.allows_players(
                            raw::RMG_TEMPLATE_COLUMN_MINIMUM_HUMAN_PLAYERS,
                            request,
                        )?
                    {
                        match row.kind() {
                            raw::RMG_TEMPLATE_COLUMN_KIND_HUMAN => {
                                human_zones += 1;
                                player_zones += 1;
                            }
                            raw::RMG_TEMPLATE_COLUMN_KIND_COMPUTER => player_zones += 1,
                            _ => {}
                        }
                        match parse_zone(&row, ZoneId::new(template.zones.len()), request)? {
                            Ok(zone) => template.zones.push(zone),
                            Err(reason) => {
                                fault.get_or_insert(reason);
                            }
                        }
                    }
                }
                let admitted = if hotfix {
                    fault.is_none() && template.admits(request)
                } else {
                    human_zones >= usize::from(request.human_players().get())
                        && player_zones
                            >= usize::from(
                                request.human_players().get() + request.computer_players().get(),
                            )
                };
                if admitted {
                    if let Some(fault) = fault {
                        templates.push(TemplateCandidate::RetailFault {
                            name: template.name,
                            fault,
                        });
                    } else {
                        self.append_connections(&mut template, row_index, end, request)?;
                        templates.push(TemplateCandidate::Ready(template));
                    }
                }
            }
            row_index = end;
        }
        Ok(templates)
    }
}

fn positive(value: i32) -> Option<NonZeroU32> {
    if value > 0 {
        NonZeroU32::new(u32::try_from(value).ok()?)
    } else {
        None
    }
}

fn placement(
    row: &Row<'_>,
    count_column: u32,
    density_column: u32,
) -> Result<Placement, TemplateError> {
    Ok(Placement {
        initial_count: row.number(count_column)?,
        density: positive(row.number(density_column)?),
    })
}

fn representable(categories: impl IntoIterator<Item = Placement> + Clone) -> bool {
    let maximum = i32::MAX.unsigned_abs() / 2;
    let mut product = 1;
    for category in categories.clone() {
        if let Some(density) = category.density {
            if density.get() > maximum / product {
                return false;
            }
            product *= density.get();
        }
    }
    let count_limit = i64::from(maximum / product - 1);
    categories.into_iter().all(|category| {
        category.density.is_none()
            || (-count_limit..=count_limit).contains(&i64::from(category.initial_count))
    })
}

#[expect(
    clippy::too_many_lines,
    reason = "one source row is parsed together before exposing a zone"
)]
// Encoding errors abort preparation; unsafe zone domains stay attached to their
// candidate so retail consumes its selection draw before reporting the fault.
fn parse_zone(
    row: &Row<'_>,
    id: ZoneId,
    request: &Request,
) -> Result<Result<Zone, RetailTemplateFault>, TemplateError> {
    let unusable = RetailTemplateFault::UnusableZone { row: row.index };
    let size = u32::try_from(row.number(raw::RMG_TEMPLATE_COLUMN_SIZE)?)
        .ok()
        .and_then(NonZeroU32::new);
    let Some(size) = size else {
        return Ok(Err(unusable));
    };
    let player = row.number(raw::RMG_TEMPLATE_COLUMN_PLAYER_INDEX)?;
    let owner = match player {
        0 => None,
        value if (1..=i32::try_from(raw::RMG_PLAYER_COUNT).unwrap()).contains(&value) => {
            Some(PlayerSlot(u8::try_from(value - 1).unwrap()))
        }
        _ => return Ok(Err(unusable)),
    };
    let role = match (row.kind(), owner) {
        (raw::RMG_TEMPLATE_COLUMN_KIND_HUMAN, Some(slot)) => ZoneRole::Human(slot),
        (raw::RMG_TEMPLATE_COLUMN_KIND_COMPUTER, Some(slot)) => ZoneRole::Computer(slot),
        (raw::RMG_TEMPLATE_COLUMN_KIND_TREASURE, owner) => ZoneRole::Treasure(owner),
        (raw::RMG_TEMPLATE_COLUMN_KIND_JUNCTION, owner) => ZoneRole::Junction(owner),
        // Retail getPlayerSlots indexes both eight-byte arrays with -1 for an
        // unassigned human zone (only allSlots for a computer zone). In generate,
        // 0x5499e0 writes humanSlots[-1]; 0x5499e5 writes allSlots[-1], aliasing
        // humanSlots[7]. Shipped 2SM2i(2) thus gains a phantom player slot.
        // Native success does not make these out-of-array accesses defined.
        _ => {
            return Ok(Err(RetailTemplateFault::UnassignedPlayerZone {
                row: row.index,
            }))
        }
    };
    let towns = [
        placement(
            row,
            raw::RMG_TEMPLATE_COLUMN_PLAYER_CASTLE_COUNT,
            raw::RMG_TEMPLATE_COLUMN_PLAYER_CASTLE_DENSITY,
        )?,
        placement(
            row,
            raw::RMG_TEMPLATE_COLUMN_PLAYER_BASIC_COUNT,
            raw::RMG_TEMPLATE_COLUMN_PLAYER_BASIC_DENSITY,
        )?,
        placement(
            row,
            raw::RMG_TEMPLATE_COLUMN_NEUTRAL_CASTLE_COUNT,
            raw::RMG_TEMPLATE_COLUMN_NEUTRAL_CASTLE_DENSITY,
        )?,
        placement(
            row,
            raw::RMG_TEMPLATE_COLUMN_NEUTRAL_BASIC_COUNT,
            raw::RMG_TEMPLATE_COLUMN_NEUTRAL_BASIC_DENSITY,
        )?,
    ];
    let mut mines = [Placement {
        initial_count: 0,
        density: None,
    }; RESOURCES];
    for (index, mine) in mines.iter_mut().enumerate() {
        let column = u32::try_from(index).unwrap();
        *mine = placement(
            row,
            raw::RMG_TEMPLATE_COLUMN_MINE_COUNTS + column,
            raw::RMG_TEMPLATE_COLUMN_MINE_DENSITIES + column,
        )?;
    }
    let mut treasure = [TreasureBand {
        minimum: 0,
        maximum: 0,
        density: None,
    }; BANDS];
    for (index, band) in treasure.iter_mut().enumerate() {
        let offset = raw::RMG_TEMPLATE_TREASURE_COLUMN_COUNT * u32::try_from(index).unwrap();
        band.minimum = row.number(raw::RMG_TEMPLATE_COLUMN_TREASURE_MINIMUM + offset)?;
        band.maximum = row.number(raw::RMG_TEMPLATE_COLUMN_TREASURE_MAXIMUM + offset)?;
        let density = row.number(raw::RMG_TEMPLATE_COLUMN_TREASURE_DENSITY + offset)?;
        band.density = if i64::from(band.maximum) >= i64::from(raw::RMG_TREASURE_MINIMUM_VALUE) {
            positive(density)
        } else {
            None
        };
    }
    if request.behavior().is_hotfix()
        && (!representable(towns)
            || !representable(mines)
            || !representable(treasure.map(|band| Placement {
                initial_count: 0,
                density: band.density,
            })))
    {
        return Ok(Err(unusable));
    }
    let flags = |first: u32, index: usize| row.set(first + u32::try_from(index).unwrap());
    let mut allowed_towns =
        std::array::from_fn(|index| flags(raw::RMG_TEMPLATE_COLUMN_ALLOWED_TOWNS, index));
    let mut allowed_monsters =
        std::array::from_fn(|index| flags(raw::RMG_TEMPLATE_COLUMN_ALLOWED_MONSTERS, index));
    if request.version() == MapVersion::Restoration {
        allowed_towns[raw::TOWN_CONFLUX as usize] = false;
        // Retail bug, retained by hotfix: slots are neutral, then factions.
        // Clearing Conflux's unshifted index therefore disables Fortress.
        allowed_monsters[raw::TOWN_CONFLUX as usize] = false;
    }
    let mut allowed_terrain =
        std::array::from_fn(|index| flags(raw::RMG_TEMPLATE_COLUMN_ALLOWED_TERRAIN, index));
    if !allowed_terrain.contains(&true) {
        allowed_terrain[raw::eTerrainDirt as usize] = true;
    }
    let monsters = match row
        .bytes(raw::RMG_TEMPLATE_COLUMN_MONSTER_STRENGTH)
        .next()
        .map(|b| b.to_ascii_lowercase())
    {
        Some(b'n') => ZoneMonsters::None,
        Some(b'w') => ZoneMonsters::Weak,
        Some(b's') => ZoneMonsters::Strong,
        _ => ZoneMonsters::Average,
    };
    Ok(Ok(Zone {
        id,
        source_number: row.number(raw::RMG_TEMPLATE_COLUMN_ZONE_INDEX)?,
        role,
        size,
        towns,
        neutral_towns_match_alignment: row.set(raw::RMG_TEMPLATE_COLUMN_NEUTRAL_TOWNS_MATCH_ZONE),
        allowed_towns,
        mines,
        use_native_terrain: row.set(raw::RMG_TEMPLATE_COLUMN_USE_NATIVE_TERRAIN),
        allowed_terrain,
        monsters,
        guards_match_alignment: row.set(raw::RMG_TEMPLATE_COLUMN_GUARDS_MATCH_ZONE),
        allowed_monsters,
        treasure,
        connections: Vec::new(),
    }))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        behavior::{Behavior, RetailProfile},
        request::{default_record, Levels, MapSize},
    };

    fn zone_row(name: &str, number: i32, player: i32) -> Vec<String> {
        let mut row = vec![String::new(); COLUMNS];
        row[raw::RMG_TEMPLATE_COLUMN_NAME as usize] = name.into();
        for (column, value) in [
            (raw::RMG_TEMPLATE_COLUMN_MINIMUM_SIZE, 1),
            (raw::RMG_TEMPLATE_COLUMN_MAXIMUM_SIZE, 32),
            (raw::RMG_TEMPLATE_COLUMN_ZONE_INDEX, number),
            (raw::RMG_TEMPLATE_COLUMN_SIZE, 10),
            (raw::RMG_TEMPLATE_COLUMN_MAXIMUM_HUMAN_PLAYERS, 8),
            (raw::RMG_TEMPLATE_COLUMN_MAXIMUM_PLAYERS, 8),
            (raw::RMG_TEMPLATE_COLUMN_PLAYER_INDEX, player),
        ] {
            row[column as usize] = value.to_string();
        }
        row[raw::RMG_TEMPLATE_COLUMN_KIND_HUMAN as usize] = "x".into();
        row
    }

    fn sheet(rows: &[Vec<String>]) -> Vec<u8> {
        let mut text = "header\r\nheader\r\nheader\r\n".to_owned();
        for row in rows {
            text.push_str(&row.join("\t"));
            text.push_str("\r\n");
        }
        text.into_bytes()
    }

    fn request(behavior: Behavior) -> Request {
        Request::parse(default_record(MapSize::Small, Levels::Surface), behavior).unwrap()
    }

    #[test]
    fn connections_resolve_sparse_source_numbers_and_ignore_filtered_zones() {
        let mut first = zone_row("test", 10, 1);
        first[raw::RMG_TEMPLATE_COLUMN_CONNECTION_FIRST_ZONE as usize] = "10".into();
        first[raw::RMG_TEMPLATE_COLUMN_CONNECTION_SECOND_ZONE as usize] = "90".into();
        first[raw::RMG_TEMPLATE_COLUMN_CONNECTION_MAXIMUM_HUMAN_PLAYERS as usize] = "8".into();
        first[raw::RMG_TEMPLATE_COLUMN_CONNECTION_MAXIMUM_PLAYERS as usize] = "8".into();
        let bytes = sheet(&[first, zone_row("", 90, 2)]);
        let source = TemplateSource::parse(&bytes).unwrap();
        let templates = source
            .prepare(&request(Behavior::Hotfix), Water::None)
            .unwrap();
        assert_eq!(templates.len(), 1);
        let template = templates
            .into_iter()
            .next()
            .unwrap()
            .into_template()
            .unwrap();
        assert_eq!(
            template.zones()[0].connections()[0].destination().index(),
            1
        );
        assert_eq!(
            template.zones()[1].connections()[0].destination().index(),
            0
        );
        assert!(matches!(template.name, Cow::Borrowed(_)));
    }

    #[test]
    fn hotfix_counts_distinct_players_where_retail_counts_zones() {
        let bytes = sheet(&[zone_row("repeated", 1, 1), zone_row("", 2, 1)]);
        let source = TemplateSource::parse(&bytes).unwrap();
        assert!(source
            .prepare(&request(Behavior::Hotfix), Water::None)
            .unwrap()
            .is_empty());
        assert_eq!(
            source
                .prepare(
                    &request(Behavior::Retail(RetailProfile::default())),
                    Water::None
                )
                .unwrap()
                .len(),
            1
        );
    }

    #[test]
    fn hotfix_drops_overflowing_density_products() {
        let mut first = zone_row("dense", 1, 1);
        for column in
            raw::RMG_TEMPLATE_COLUMN_MINE_DENSITIES..raw::RMG_TEMPLATE_COLUMN_USE_NATIVE_TERRAIN
        {
            first[column as usize] = "100".into();
        }
        let bytes = sheet(&[first, zone_row("", 2, 2)]);
        let source = TemplateSource::parse(&bytes).unwrap();
        assert!(source
            .prepare(&request(Behavior::Hotfix), Water::None)
            .unwrap()
            .is_empty());
    }

    #[test]
    fn source_number_parsing_keeps_crt_prefix_rules() {
        for (text, expected) in [
            ("", 0),
            ("  -15tail", -15),
            ("+7", 7),
            ("no", 0),
            ("-2147483648", i32::MIN),
        ] {
            let bytes = format!("{text}\r\n");
            let sheet = Spreadsheet::parse(bytes.as_bytes()).unwrap();
            let row = Row::new(sheet.rows().next().unwrap(), 0);
            assert_eq!(row.number(0).unwrap(), expected);
        }
        let sheet = Spreadsheet::parse(b"2147483648\r\n").unwrap();
        assert!(Row::new(sheet.rows().next().unwrap(), 0).number(0).is_err());
    }

    #[test]
    fn minimum_size_only_row_is_a_retail_fault_and_a_hotfix_skip() {
        let bytes = b"h\r\nh\r\nh\r\nshort\t1\r\n";
        let source = TemplateSource::parse(bytes).unwrap();
        assert!(source
            .prepare(&request(Behavior::Hotfix), Water::None)
            .unwrap()
            .is_empty());
        assert!(matches!(
            source.prepare(
                &request(Behavior::Retail(RetailProfile::default())),
                Water::None
            ),
            Err(TemplateError::RetailShortRow { .. })
        ));
    }

    #[test]
    fn roe_guard_offset_bug_and_dirt_fallback_remain_explicit() {
        let mut first = zone_row("roe", 1, 1);
        for column in
            raw::RMG_TEMPLATE_COLUMN_ALLOWED_MONSTERS..raw::RMG_TEMPLATE_COLUMN_TREASURE_MINIMUM
        {
            first[column as usize] = "x".into();
        }
        let bytes = sheet(&[first, zone_row("", 2, 2)]);
        let source = TemplateSource::parse(&bytes).unwrap();
        let mut record = default_record(MapSize::Small, Levels::Surface);
        record.m_mapVersion = raw::RMG_MAP_RESTORATION_OF_ERATHIA;
        let templates = source
            .prepare(
                &Request::parse(record, Behavior::Hotfix).unwrap(),
                Water::None,
            )
            .unwrap();
        let template = templates
            .into_iter()
            .next()
            .unwrap()
            .into_template()
            .unwrap();
        let zone = &template.zones()[0];
        assert!(!zone.allowed_monsters()[raw::TOWN_FORTRESS as usize + 1]);
        assert!(zone.allowed_monsters()[raw::TOWN_CONFLUX as usize + 1]);
        assert!(zone.allowed_terrain()[raw::eTerrainDirt as usize]);
    }

    #[test]
    fn unassigned_player_zone_faults_only_if_its_candidate_is_selected() {
        let bytes = sheet(&[
            zone_row("valid", 1, 1),
            zone_row("", 2, 2),
            zone_row("unassigned", 1, 0),
            zone_row("", 2, 2),
            zone_row("also valid", 1, 1),
            zone_row("", 2, 2),
        ]);
        let source = TemplateSource::parse(&bytes).unwrap();
        let candidates = source
            .prepare(
                &request(Behavior::Retail(RetailProfile::default())),
                Water::None,
            )
            .unwrap();
        assert_eq!(candidates.len(), 3);
        let mut rng = crate::rng::RetailRng::new(1);
        assert!(matches!(
            crate::selection::SelectedTemplate::select(
                &candidates[1..2],
                &request(Behavior::Retail(RetailProfile::default())),
                &mut rng,
            ),
            Err(crate::selection::SelectionError::RetailTemplate(
                RetailTemplateFault::UnassignedPlayerZone { row: 5 }
            ))
        ));
        assert_eq!(rng.draws(), 1);
        let mut candidates = candidates.into_iter();
        assert_eq!(
            candidates.next().unwrap().into_template().unwrap().name(),
            b"valid"
        );
        assert!(matches!(
            candidates.next().unwrap().into_template(),
            Err(RetailTemplateFault::UnassignedPlayerZone { row: 5 })
        ));
        assert_eq!(
            candidates.next().unwrap().into_template().unwrap().name(),
            b"also valid"
        );
        assert_eq!(
            source
                .prepare(&request(Behavior::Hotfix), Water::None)
                .unwrap()
                .len(),
            2
        );
    }

    #[test]
    fn selection_prioritizes_fixed_colours_and_faults_after_drawing() {
        use crate::{
            rng::RetailRng,
            selection::{SelectedTemplate, SelectionError},
        };
        let bytes = sheet(&[zone_row("seats", 1, 3), zone_row("", 2, 7)]);
        let source = TemplateSource::parse(&bytes).unwrap();
        let mut record = default_record(MapSize::Small, Levels::Surface);
        record.m_isHumanSeat[5] = 1;
        let request = Request::parse(record, Behavior::Hotfix).unwrap();
        let candidates = source.prepare(&request, Water::None).unwrap();
        let mut rng = RetailRng::new(1);
        let selected = SelectedTemplate::select(&candidates, &request, &mut rng).unwrap();
        assert_eq!(selected.players()[2].unwrap().index(), 5);
        assert_eq!(selected.players()[6].unwrap().index(), 0);
        assert_eq!(rng.draws(), 1);

        let bytes = sheet(&[zone_row("repeated", 1, 3), zone_row("", 2, 3)]);
        let source = TemplateSource::parse(&bytes).unwrap();
        let request = Request::parse(record, Behavior::Retail(RetailProfile::default())).unwrap();
        let candidates = source.prepare(&request, Water::None).unwrap();
        assert!(matches!(
            SelectedTemplate::select(&candidates, &request, &mut rng),
            Err(SelectionError::InsufficientSlots)
        ));
        assert_eq!(rng.draws(), 2);
        assert!(matches!(
            SelectedTemplate::select(&[], &request, &mut rng),
            Err(SelectionError::NoTemplates)
        ));
        assert_eq!(rng.draws(), 2);
    }
}
