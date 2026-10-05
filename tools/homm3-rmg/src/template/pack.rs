//! Variable-column template reader, following HotA.dll RVAs 0x1d4c60,
//! 0x1d54d0, 0x1d5dc0, 0x1d65e0 and 0x1d58c0 (`hota-rmg` 3a1421ef5).
//! Both encodings produce the ordinary Template/Zone/Connection model.

use super::apply_availability as availability;
use super::{
    positive, Availability, Connection, ConnectionKind, ConnectionOptions, MapOptions, Placement,
    PlayerSlot, Request, RoadPolicy, Ruleset, Template, TemplateCandidate, TemplateError,
    TemplateFault, TemplateSource, TreasureBand, Water, Zone, ZoneMonsters, ZoneOptions, ZoneRole,
    BANDS, RESOURCES,
};
use crate::{domain::Level, geometry::ZoneId, parse, request::MapVersion};
use homm3_resource::{Field, SpreadsheetRow};
use std::borrow::Cow;

/// Resource encoding; the ruleset is chosen separately.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TemplateFormat {
    /// Original fixed-column `rmg.txt`.
    Legacy,
    /// A `.h3t` spreadsheet with a `Pack` header and explicit section widths.
    Pack,
}

/// Lobby and map-wide settings shared by every template in a pack.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct PackSettings {
    /// Original resource bytes, without requiring UTF-8.
    pub name: Option<Vec<u8>>,
    /// Pack description.
    pub description: Option<Vec<u8>>,
    /// Availability in town-catalog order.
    pub towns: Vec<Availability>,
    /// Availability in hero-catalog order.
    pub heroes: Vec<Availability>,
    /// Generate the template's mirrored counterpart.
    pub mirror: bool,
    /// Pack tags.
    pub tags: Option<Vec<u8>>,
    /// Battle turn limit; absent denotes the native -1 default.
    pub max_battle_rounds: Option<u16>,
    /// Disable hero hiring for all players.
    pub forbid_hiring_heroes: bool,
}

pub(super) struct Columns {
    pack: usize,
    towns: usize,
    terrains: usize,
    kinds: usize,
    pack_new: usize,
    map_new: usize,
    zone_new: usize,
    connection_new: usize,
    name: usize,
    zone: usize,
    connection: usize,
}
impl Columns {
    fn legacy() -> Self {
        Self {
            pack: 0,
            towns: 9,
            terrains: 8,
            kinds: 4,
            pack_new: 0,
            map_new: 0,
            zone_new: 0,
            connection_new: 0,
            name: 0,
            zone: 3,
            connection: 76,
        }
    }
    fn resolve(&mut self, row: &Record<'_>) -> Result<(), TemplateError> {
        let overflow = || row.error(0, "field-count arithmetic overflow");
        self.name = self.pack.checked_add(self.pack_new).ok_or_else(overflow)?;
        self.zone = self
            .name
            .checked_add(3)
            .and_then(|v| v.checked_add(self.map_new))
            .ok_or_else(overflow)?;
        self.connection = self
            .zone
            .checked_add(43)
            .and_then(|v| v.checked_add(self.towns.checked_mul(2)?))
            .and_then(|v| v.checked_add(self.terrains))
            .and_then(|v| v.checked_add(self.kinds))
            .and_then(|v| v.checked_add(self.zone_new))
            .ok_or_else(overflow)?;
        self.connection
            .checked_add(9)
            .and_then(|v| v.checked_add(self.connection_new))
            .ok_or_else(overflow)?;
        Ok(())
    }
}

struct Record<'a> {
    cells: Vec<Field<'a>>,
    row: usize,
}
impl<'a> Record<'a> {
    fn new(row: SpreadsheetRow<'a>, index: usize) -> Self {
        Self {
            cells: row.cells().collect(),
            row: index,
        }
    }
    fn error(&self, column: usize, reason: &'static str) -> TemplateError {
        TemplateError::Pack {
            row: self.row,
            column,
            reason,
        }
    }
    fn first(&self, column: usize) -> Option<u8> {
        self.cells
            .get(column)
            .and_then(|cell| cell.decoded().next())
    }
    fn set(&self, column: usize) -> bool {
        !matches!(self.first(column), None | Some(0 | b' '))
    }
    fn number(&self, column: usize) -> Result<i32, TemplateError> {
        let cell = self
            .cells
            .get(column)
            .ok_or_else(|| self.error(column, "missing numeric cell"))?;
        parse::integer(cell.decoded()).ok_or_else(|| self.error(column, "integer overflow"))
    }
    fn optional_number(&self, column: usize, default: i32) -> Result<i32, TemplateError> {
        if self.set(column) {
            self.number(column)
        } else {
            Ok(default)
        }
    }
    fn text(&self, column: usize) -> Option<Vec<u8>> {
        self.cells
            .get(column)
            .map(|c| c.decoded().take_while(|&b| b != 0).collect())
    }
    fn name(&self, column: usize) -> Cow<'a, [u8]> {
        let Some(field) = self.cells.get(column) else {
            return Cow::Borrowed(&[]);
        };
        let encoded = field.encoded();
        if encoded.contains(&b'"') {
            Cow::Owned(field.decoded().take_while(|&b| b != 0).collect())
        } else {
            Cow::Borrowed(encoded.split(|&b| b == 0).next().unwrap_or_default())
        }
    }
    fn allows(&self, column: usize, request: &Request) -> Result<bool, TemplateError> {
        let parameters = request.constructor_parameters();
        let humans = i32::from(parameters.human_players.get());
        let total = humans + i32::from(parameters.computer_players.get());
        Ok(humans >= self.number(column)?
            && humans <= self.number(column + 1)?
            && total >= self.number(column + 2)?
            && total <= self.number(column + 3)?)
    }
    fn floating(&self, column: usize) -> Result<f64, TemplateError> {
        let bytes = self
            .text(column)
            .ok_or_else(|| self.error(column, "missing floating-point cell"))?;
        let bytes = bytes.as_slice();
        let start = bytes
            .iter()
            .position(|b| !b.is_ascii_whitespace())
            .unwrap_or(bytes.len());
        let mut end = start;
        if matches!(bytes.get(end), Some(b'+' | b'-')) {
            end += 1;
        }
        let mut digits = 0;
        while bytes.get(end).is_some_and(u8::is_ascii_digit) {
            end += 1;
            digits += 1;
        }
        if bytes.get(end) == Some(&b'.') {
            end += 1;
            while bytes.get(end).is_some_and(u8::is_ascii_digit) {
                end += 1;
                digits += 1;
            }
        }
        if digits == 0 {
            return Ok(0.0);
        }
        if matches!(bytes.get(end), Some(b'e' | b'E')) {
            let exponent = end;
            end += 1;
            if matches!(bytes.get(end), Some(b'+' | b'-')) {
                end += 1;
            }
            let first_digit = end;
            while bytes.get(end).is_some_and(u8::is_ascii_digit) {
                end += 1;
            }
            if end == first_digit {
                end = exponent;
            }
        }
        let number = std::str::from_utf8(&bytes[start..end])
            .ok()
            .and_then(|s| s.parse::<f64>().ok())
            .filter(|n| n.is_finite());
        number.ok_or_else(|| self.error(column, "floating-point overflow"))
    }
}

pub(super) fn settings(
    source: &TemplateSource<'_>,
    rules: Ruleset,
) -> Result<(Columns, PackSettings), TemplateError> {
    if source.format == TemplateFormat::Pack && rules == Ruleset::Complete {
        return Err(TemplateError::FormatMismatch);
    }
    let mut columns = Columns::legacy();
    let mut settings = PackSettings {
        name: None,
        description: None,
        towns: vec![Availability::Inherit; rules.town_count()],
        heroes: vec![Availability::Inherit; rules.hero_count()],
        mirror: false,
        tags: None,
        max_battle_rounds: None,
        forbid_hiring_heroes: false,
    };
    if source.format == TemplateFormat::Legacy {
        return Ok((columns, settings));
    }
    let row = source.rows.get(3).ok_or(TemplateError::Pack {
        row: 3,
        column: 0,
        reason: "missing pack header",
    })?;
    let row = Record::new(*row, 3);
    let count =
        |c| usize::try_from(row.number(c)?).map_err(|_| row.error(c, "negative field count"));
    columns.pack = 7;
    columns.towns = count(0)?;
    columns.terrains = count(1)?;
    columns.kinds = count(2)?;
    columns.pack_new = count(3)?;
    columns.map_new = count(4)?;
    columns.zone_new = count(5)?;
    columns.connection_new = count(6)?;
    columns.resolve(&row)?;
    if columns.name > row.cells.len() {
        return Err(row.error(7, "truncated pack settings"));
    }
    if columns.pack_new > 0 {
        settings.name = row.text(7);
    }
    if columns.pack_new > 1 {
        settings.description = row.text(8);
    }
    if columns.pack_new > 2 {
        availability(&row.text(9).unwrap_or_default(), &mut settings.towns)
            .map_err(|()| row.error(9, "availability ID overflow"))?;
        let towns = &mut settings.towns;
        if [0, 1, 3]
            .into_iter()
            .any(|i| towns[i] == Availability::Disabled)
            || (towns[5] == Availability::Disabled && towns[6] == Availability::Disabled)
        {
            for town in towns.iter_mut().skip(columns.towns) {
                *town = Availability::Disabled;
            }
        }
        if towns.iter().all(|t| *t == Availability::Disabled) {
            towns[0] = Availability::Inherit;
        }
    }
    if columns.pack_new > 3 {
        availability(&row.text(10).unwrap_or_default(), &mut settings.heroes)
            .map_err(|()| row.error(10, "availability ID overflow"))?;
    }
    if columns.pack_new > 4 {
        settings.mirror = row.set(11);
    }
    if columns.pack_new > 5 {
        settings.tags = row.text(12);
    }
    if columns.pack_new > 6 {
        let rounds = row.optional_number(13, -1)?;
        settings.max_battle_rounds = if rounds > 0 {
            Some(u16::try_from(rounds.min(9999)).expect("positive battle limit capped to 9999"))
        } else {
            None
        };
    }
    if columns.pack_new > 7 {
        settings.forbid_hiring_heroes = row.set(14);
    }
    Ok((columns, settings))
}

fn map_options(row: &Record<'_>, columns: &Columns) -> Result<MapOptions, TemplateError> {
    let mut options = MapOptions::default();
    let first = columns.name + 3;
    for option in 0..columns.map_new.min(10) {
        let c = first + option;
        match option {
            0 => options.artifacts = row.text(c),
            1 => options.combination_artifacts = row.text(c),
            2 => options.spells = row.text(c),
            3 => options.secondary_skills = row.text(c),
            4 => options.objects = row.text(c),
            5 if row.set(c) => {
                options.rock_blocks = Some(
                    if row
                        .first(c)
                        .is_some_and(|b| b.is_ascii_digit() || b == b'.')
                    {
                        row.floating(c)?
                    } else {
                        1.0
                    },
                );
            }
            6 => options.zone_sparseness = if row.set(c) { row.floating(c)? } else { 1.0 },
            7 => options.special_weeks_disabled = row.set(c),
            8 => options.spell_research = row.set(c),
            9 => options.anarchy = row.set(c),
            _ => {}
        }
    }
    Ok(options)
}

fn zone_options(
    row: &Record<'_>,
    first: usize,
    count: usize,
) -> Result<ZoneOptions, TemplateError> {
    let mut options = ZoneOptions::default();
    for option in 0..count.min(18) {
        let c = first + option;
        match option {
            0 => {
                options.placement = match row.first(c).map(|b| b.to_ascii_lowercase()) {
                    Some(b'g') => Some(Level::Surface),
                    Some(b'u') => Some(Level::Underground),
                    _ => None,
                }
            }
            1 => options.objects = row.text(c),
            2 => options.minimum_objects = row.text(c),
            3 => options.image_settings = row.text(c),
            4 => options.force_neutral_creatures = row.set(c),
            5 => options.allow_non_coherent_road = row.set(c),
            6 => options.zone_repulsion = row.set(c),
            7 => options.town_hint = row.text(c),
            8 => {
                let n = row.optional_number(c, 3)?;
                options.monster_disposition = u8::try_from(n).ok().filter(|&n| n <= 5).unwrap_or(3);
            }
            9 if options.monster_disposition == 5 => {
                options.custom_monster_disposition = Some(
                    u8::try_from(row.optional_number(c, -1)?.clamp(1, 10))
                        .expect("disposition clamped to 1..10"),
                );
            }
            10 => {
                if row.set(c) {
                    if let Ok(index) = usize::try_from(row.number(c)?) {
                        if let Some(&percent) = [25, 50, 75, 100].get(index) {
                            options.monsters_joining_percent = percent;
                        }
                    }
                }
            }
            11 => options.monsters_join_only_for_money = row.set(c),
            12 => options.minimum_airship_yards = row.optional_number(c, 0)?,
            13 => options.airship_yard_density = row.optional_number(c, 0)?,
            14 => options.terrain_hint = row.text(c),
            15 => options.allowed_factions = row.text(c),
            16 => options.faction_hint = row.text(c),
            17 => options.max_block_value = row.optional_number(c, -1)?,
            _ => {}
        }
    }
    Ok(options)
}

#[expect(
    clippy::too_many_lines,
    reason = "one source row is admitted before exposing a shared zone"
)]
fn read_zone(
    row: &Record<'_>,
    columns: &Columns,
    rules: Ruleset,
    request: &Request,
    id: ZoneId,
    kind: usize,
) -> Result<Result<Zone, TemplateFault>, TemplateError> {
    let c = columns.zone + 1 + columns.kinds;
    let invalid = TemplateFault::UnusableZone { row: row.row };
    let Some(size) = positive(row.number(c)?) else {
        return Ok(Err(invalid));
    };
    let owner = match row.number(c + 5)? {
        0 => None,
        n @ 1..=8 => Some(PlayerSlot(
            u8::try_from(n - 1).expect("player slot bounded to 0..7"),
        )),
        _ => return Ok(Err(invalid)),
    };
    let role = match (kind, owner) {
        (0, Some(owner)) => ZoneRole::Human(owner),
        (1, Some(owner)) => ZoneRole::Computer(owner),
        (2, owner) => ZoneRole::Treasure(owner),
        (3, owner) => ZoneRole::Junction(owner),
        (0 | 1, None) => return Ok(Err(TemplateFault::UnassignedPlayerZone { row: row.row })),
        _ => return Ok(Err(invalid)),
    };
    let placement = |count, density| -> Result<Placement, TemplateError> {
        Ok(Placement {
            initial_count: row.number(count)?,
            density: positive(row.number(density)?),
        })
    };
    let towns = [
        placement(c + 7, c + 9)?,
        placement(c + 6, c + 8)?,
        placement(c + 11, c + 13)?,
        placement(c + 10, c + 12)?,
    ];
    let mut allowed_towns = vec![false; rules.town_count()];
    for (i, allowed) in allowed_towns.iter_mut().enumerate().take(columns.towns) {
        *allowed = row.set(c + 15 + i);
    }
    if request.version() == MapVersion::Restoration {
        allowed_towns[8] = false;
    }
    let mines_first = c + 15 + columns.towns;
    let mut mines = [Placement {
        initial_count: 0,
        density: None,
    }; RESOURCES];
    for (i, mine) in mines.iter_mut().enumerate() {
        *mine = placement(mines_first + i, mines_first + RESOURCES + i)?;
    }
    let native = mines_first + 2 * RESOURCES;
    let terrain_first = native + 1;
    let mut allowed_terrain = vec![false; rules.terrain_count()];
    let mut any_terrain = false;
    for i in 0..columns.terrains {
        let target = if i < 8 { i } else { i + 2 };
        if let Some(allowed) = allowed_terrain.get_mut(target) {
            *allowed = row.set(terrain_first + i);
            any_terrain |= *allowed;
        }
    }
    // Older files allow newly introduced terrain only with all four base flags.
    let migrate_terrain = [0, 2, 4, 5].into_iter().all(|i| allowed_terrain[i]);
    for i in columns.terrains..rules.terrain_count() - 2 {
        if i >= 8 {
            allowed_terrain[i + 2] = migrate_terrain;
        }
    }
    if !any_terrain {
        allowed_terrain[0] = true;
    }
    let strength = terrain_first + columns.terrains;
    let monsters = match row.first(strength).map(|b| b.to_ascii_lowercase()) {
        Some(b'n') => ZoneMonsters::None,
        Some(b'w') => ZoneMonsters::Weak,
        Some(b's') => ZoneMonsters::Strong,
        _ => ZoneMonsters::Average,
    };
    let factions_first = strength + 2;
    let mut allowed_monsters = vec![false; rules.town_count() + 1];
    for (i, allowed) in allowed_monsters
        .iter_mut()
        .enumerate()
        .take(columns.towns + 1)
    {
        *allowed = row.set(factions_first + i);
    }
    if request.version() == MapVersion::Restoration {
        allowed_monsters[9] = false;
    }
    let treasure_first = factions_first + columns.towns + 1;
    let mut treasure = [TreasureBand {
        minimum: 0,
        maximum: 0,
        density: None,
    }; BANDS];
    for (i, band) in treasure.iter_mut().enumerate() {
        let c = treasure_first + 3 * i;
        band.minimum = row.number(c)?;
        band.maximum = row.number(c + 1)?;
        band.density = if band.maximum >= 100 {
            positive(row.number(c + 2)?)
        } else {
            None
        };
    }
    // RVA 0x1d4c60 migrates town and monster masks after reading the pack.
    if columns.towns < rules.town_count() {
        if [0, 1, 3, 5, 6].into_iter().all(|i| allowed_towns[i]) {
            allowed_towns[columns.towns..].fill(true);
        }
        if [1, 2, 4, 6, 7].into_iter().all(|i| allowed_monsters[i]) {
            allowed_monsters[columns.towns + 1..].fill(true);
        }
    }
    Ok(Ok(Zone {
        id,
        source_number: row.number(columns.zone)?,
        role,
        size,
        towns,
        neutral_towns_match_alignment: row.set(c + 14),
        allowed_towns: allowed_towns.into_boxed_slice(),
        mines,
        use_native_terrain: row.set(native),
        allowed_terrain: allowed_terrain.into_boxed_slice(),
        monsters,
        guards_match_alignment: row.set(strength + 1),
        allowed_monsters: allowed_monsters.into_boxed_slice(),
        treasure,
        connections: Vec::new(),
        options: zone_options(row, treasure_first + 3 * BANDS, columns.zone_new)?,
    }))
}

fn connections(
    source: &TemplateSource<'_>,
    template: &mut Template<'_>,
    first: usize,
    end: usize,
    columns: &Columns,
    request: &Request,
) -> Result<(), TemplateError> {
    let c = columns.connection;
    for i in first..end {
        let row = Record::new(source.rows[i], i);
        if row.cells.len() < c + columns.connection_new + 9 || !row.set(c) || !row.set(c + 1) {
            continue;
        }
        let first_number = row.number(c)?;
        let second_number = row.number(c + 1)?;
        let Some(first) = template
            .zones
            .iter()
            .position(|z| z.source_number == first_number)
        else {
            continue;
        };
        let second = template
            .zones
            .iter()
            .position(|z| z.source_number == second_number);
        if second.is_none() && second_number != -1 {
            continue;
        }
        let mut options = ConnectionOptions::default();
        if columns.connection_new > 0 {
            options.road = match row.first(c + 5) {
                Some(b'-') => RoadPolicy::Forbidden,
                Some(b'+') => RoadPolicy::Required,
                _ => RoadPolicy::Automatic,
            };
        }
        if columns.connection_new > 1 {
            options.kind = match row.first(c + 6).map(|b| b.to_ascii_lowercase()) {
                Some(b'g') => ConnectionKind::Ground,
                Some(b'u') => ConnectionKind::Underground,
                Some(b't') => ConnectionKind::Teleport,
                Some(b'r') => ConnectionKind::Random,
                _ => ConnectionKind::Automatic,
            };
        }
        if columns.connection_new > 2 {
            options.fictive = row.set(c + 7);
        }
        if columns.connection_new > 3 {
            options.portal_repulsion = row.set(c + 8);
        }
        if !row.allows(c + 5 + columns.connection_new, request)? {
            continue;
        }
        let connection = Connection {
            destination: second.map(ZoneId::new),
            value: row.number(c + 2)?,
            unguarded: row.set(c + 3),
            border_guard: row.set(c + 4),
            options,
        };
        template.zones[first].connections.push(connection);
        if let Some(second) = second {
            template.zones[second].connections.push(Connection {
                destination: Some(ZoneId::new(first)),
                ..connection
            });
        }
    }
    Ok(())
}

pub(super) fn prepare<'a>(
    source: &TemplateSource<'a>,
    request: &Request,
    water: Water,
    rules: Ruleset,
) -> Result<Vec<TemplateCandidate<'a>>, TemplateError> {
    let (columns, _) = settings(source, rules)?;
    let parameters = request.constructor_parameters();
    let dimension = request.size().dimension();
    let mut size = dimension * dimension * parameters.levels.count() / 1280;
    if water == Water::Islands {
        size = (size / 2).max(1);
    }
    let mut result = Vec::new();
    let mut row_index = 3;
    while row_index < source.rows.len() {
        let row = Record::new(source.rows[row_index], row_index);
        if row.cells.len() < columns.zone {
            row_index += 1;
            continue;
        }
        let mut end = row_index + 1;
        while end < source.rows.len() && !Record::new(source.rows[end], end).set(columns.name) {
            end += 1;
        }
        let name = row.name(columns.name);
        let minimum = row.number(columns.name + 1)?;
        let maximum = row.number(columns.name + 2)?;
        let options = map_options(&row, &columns)?;
        if i64::from(minimum) <= i64::from(size) && i64::from(size) <= i64::from(maximum) {
            let mut template = Template {
                name,
                zones: Vec::new(),
                options,
                ruleset: rules,
            };
            let mut fault = None;
            let mut humans = 0;
            let mut players = 0;
            for i in row_index..end {
                let row = Record::new(source.rows[i], i);
                if row.cells.len() < columns.connection
                    || !row.set(columns.zone)
                    || !row.allows(columns.zone + columns.kinds + 2, request)?
                {
                    continue;
                }
                let kind = (0..columns.kinds)
                    .rev()
                    .find(|&kind| row.set(columns.zone + 1 + kind))
                    .unwrap_or(2);
                if kind == 0 {
                    humans += 1;
                    players += 1;
                }
                if kind == 1 {
                    players += 1;
                }
                match read_zone(
                    &row,
                    &columns,
                    rules,
                    request,
                    ZoneId::new(template.zones.len()),
                    kind,
                )? {
                    Ok(zone) => template.zones.push(zone),
                    Err(reason) => {
                        fault.get_or_insert(reason);
                    }
                }
            }
            if humans >= usize::from(parameters.human_players.get())
                && players
                    >= usize::from(
                        parameters.human_players.get() + parameters.computer_players.get(),
                    )
            {
                if let Some(fault) = fault {
                    result.push(TemplateCandidate::NativeFault {
                        name: template.name,
                        fault,
                    });
                } else {
                    connections(source, &mut template, row_index, end, &columns, request)?;
                    result.push(TemplateCandidate::Ready(template));
                }
            }
        }
        row_index = end;
    }
    Ok(result)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        behavior::Behavior,
        request::{default_record, Levels, MapSize},
    };

    fn request() -> Request {
        Request::parse(
            default_record(MapSize::Large, Levels::Surface),
            Behavior::Hotfix,
        )
        .unwrap()
    }
    fn sheet(rows: &[Vec<String>]) -> Vec<u8> {
        let mut text = String::from("Pack\r\nsections\r\ncolumns\r\n");
        for row in rows {
            text.push_str(&row.join("\t"));
            text.push_str("\r\n");
        }
        text.into_bytes()
    }
    // Synthetic pack in the documented 1.8.1 12/10/4/8/10/18/4 column layout.
    fn row(name: &str, zone: i32, owner: i32) -> Vec<String> {
        let mut row = vec![String::new(); 140];
        for (column, value) in [
            (16, 9),
            (17, 16),
            (28, zone),
            (33, 30),
            (35, 8),
            (37, 8),
            (38, owner),
        ] {
            row[column] = value.to_string();
        }
        row[15] = name.into();
        row[29] = "x".into();
        row[48] = "x".into();
        row[75] = "x".into();
        row[87] = "x".into();
        if !name.is_empty() {
            for (column, value) in [12, 10, 4, 8, 10, 18, 4].into_iter().enumerate() {
                row[column] = value.to_string();
            }
            row[7] = "synthetic pack".into();
        }
        row
    }
    fn prepared(bytes: &[u8]) -> Vec<TemplateCandidate<'_>> {
        TemplateSource::parse(bytes)
            .unwrap()
            .prepare_for(&request(), Water::None, Ruleset::HotA181)
            .unwrap()
    }

    #[test]
    fn mirror_admission_uses_constructor_counts_and_planes() {
        use crate::{behavior::RetailProfile, request::RequestOptions};

        for (water, area) in [(Water::None, 9), (Water::Islands, 4)] {
            let mut first = row("mirror", 1, 1);
            for column in [16, 17] {
                first[column] = area.to_string();
            }
            // One human slot, with row and connection gates admitting one player.
            for column in [35, 37, 137, 139] {
                first[column] = "1".into();
            }
            first[127] = "1".into();
            first[128] = "-1".into();
            let bytes = sheet(&[first]);
            let source = TemplateSource::parse(&bytes).unwrap();
            for levels in [Levels::Surface, Levels::Underground] {
                for mirror in [false, true] {
                    let request = Request::parse_with_options(
                        default_record(MapSize::Large, levels),
                        Behavior::Retail(RetailProfile::default()),
                        RequestOptions {
                            ruleset: Ruleset::HotA181,
                            mirror,
                            ..RequestOptions::default()
                        },
                    )
                    .unwrap();
                    let candidates = source.prepare(&request, water).unwrap();
                    assert_eq!(candidates.len(), usize::from(mirror));
                    if mirror {
                        let TemplateCandidate::Ready(template) = &candidates[0] else {
                            panic!("valid mirror candidate")
                        };
                        assert_eq!(template.ruleset(), Ruleset::HotA181);
                        assert_eq!(template.zones().len(), 1);
                        assert_eq!(template.zones()[0].connections().len(), 1);
                        assert_eq!(template.zones()[0].connections()[0].destination(), None);
                    }
                }
            }
        }
    }

    #[test]
    fn pack_options_survive_normalization_and_complete_rejects_the_encoding() {
        let mut first = row("two zones", 10, 1);
        first[9] = "-0 +11".into();
        first[10] = "-214".into();
        first[11] = "x".into();
        first[13] = "100000".into();
        first[14] = "x".into();
        first[23] = "x".into();
        first[24] = "1.25e0suffix".into();
        first[109] = "u".into();
        first[117] = "5".into();
        first[119] = "2".into();
        first[126] = "15000".into();
        for (column, value) in [
            (127, "10"),
            (128, "90"),
            (129, "45000"),
            (132, "+"),
            (133, "t"),
            (134, "x"),
            (135, "x"),
            (137, "8"),
            (139, "8"),
        ] {
            first[column] = value.into();
        }
        let bytes = sheet(&[first, row("", 90, 2)]);
        let source = TemplateSource::parse(&bytes).unwrap();
        assert_eq!(source.format(), TemplateFormat::Pack);
        assert!(matches!(
            source.prepare(&request(), Water::None),
            Err(TemplateError::FormatMismatch)
        ));
        let settings = source.pack_settings(Ruleset::HotA181).unwrap();
        assert!(settings.mirror && settings.forbid_hiring_heroes);
        assert_eq!(settings.max_battle_rounds, Some(9999));
        assert_eq!(settings.towns[11], Availability::Enabled);
        assert_eq!(settings.heroes[214], Availability::Disabled);
        let template = prepared(&bytes).remove(0).into_template().unwrap();
        assert_eq!(template.ruleset(), Ruleset::HotA181);
        assert_eq!(template.options().rock_blocks, Some(1.0));
        assert_eq!(template.options().zone_sparseness, 1.25);
        assert!(!template.options().spell_research); // Present blank differs from omitted.
        let zone = &template.zones()[0];
        assert_eq!(zone.allowed_towns().len(), 12);
        assert_eq!(zone.allowed_terrain().len(), 12);
        assert_eq!(zone.allowed_monsters().len(), 13);
        assert_eq!(zone.options().placement, Some(Level::Underground));
        assert_eq!(zone.options().custom_monster_disposition, Some(1));
        assert_eq!(zone.options().monsters_joining_percent, 75);
        assert_eq!(zone.options().max_block_value, 15000);
        let forward = zone.connections()[0];
        let backward = template.zones()[1].connections()[0];
        assert_eq!(forward.destination().unwrap().index(), 1);
        assert_eq!(backward.destination().unwrap().index(), 0);
        assert_eq!(forward.options(), backward.options());
        assert_eq!(
            forward.options(),
            ConnectionOptions {
                road: RoadPolicy::Required,
                kind: ConnectionKind::Teleport,
                fictive: true,
                portal_repulsion: true
            }
        );
    }

    #[test]
    fn single_ended_connections_and_island_size_filters_are_preserved() {
        let mut first = row("mirror", 1, 1);
        for (column, value) in [(127, "1"), (128, "-1"), (133, "r"), (137, "8"), (139, "8")] {
            first[column] = value.into();
        }
        let bytes = sheet(&[first, row("", 2, 2)]);
        let template = prepared(&bytes).remove(0).into_template().unwrap();
        let connection = template.zones()[0].connections()[0];
        assert_eq!(connection.destination(), None);
        assert_eq!(connection.options().kind, ConnectionKind::Random);
        assert!(template.zones()[1].connections().is_empty());
        assert!(TemplateSource::parse(&bytes)
            .unwrap()
            .prepare_for(&request(), Water::Islands, Ruleset::HotA181)
            .unwrap()
            .is_empty());
    }

    #[test]
    fn availability_scanner_preserves_order_and_native_sign_state() {
        let mut flags = [Availability::Inherit; 5];
        availability(b"4 - words 1 +-2 +3-3 +99 -0\0+0", &mut flags).unwrap();
        assert_eq!(
            flags,
            [
                Availability::Disabled,
                Availability::Disabled,
                Availability::Disabled,
                Availability::Disabled,
                Availability::Inherit
            ]
        );
        availability(b"+0 +1 +2", &mut flags).unwrap();
        assert_eq!(&flags[..3], &[Availability::Enabled; 3]);
        assert!(availability(b"+2147483648", &mut flags).is_err());
    }

    #[test]
    fn legacy_migration_and_roe_monster_filter_follow_extended_rules() {
        let mut first = vec![String::new(); 85];
        let mut second = first.clone();
        for (r, name, id) in [(&mut first, "old", 1), (&mut second, "", 2)] {
            r[0] = name.into();
            // Legacy schema: id 3, kind 4..7, size 8, restrictions 9..12, owner 13.
            for (c, n) in [
                (1, 1),
                (2, 32),
                (3, id),
                (8, 10),
                (10, 8),
                (12, 8),
                (13, id),
            ] {
                r[c] = n.to_string();
            }
            r[4] = "x".into();
            for c in 23..32 {
                r[c] = "x".into();
            }
            for c in 47..55 {
                r[c] = "x".into();
            }
            for c in 57..67 {
                r[c] = "x".into();
            }
        }
        let mut bytes = sheet(&[first, second]);
        bytes[..4].copy_from_slice(b"Name");
        let source = TemplateSource::parse(&bytes).unwrap();
        let mut raw = default_record(MapSize::Large, Levels::Surface);
        raw.m_mapVersion = crate::raw::RMG_MAP_RESTORATION_OF_ERATHIA;
        let request = Request::parse(raw, Behavior::Hotfix).unwrap();
        let templates = source
            .prepare_for(&request, Water::None, Ruleset::HotA181)
            .unwrap();
        let TemplateCandidate::Ready(template) = &templates[0] else {
            panic!("valid migrated template")
        };
        let zone = &template.zones()[0];
        assert!(!zone.allowed_towns()[8]);
        assert_eq!(&zone.allowed_towns()[9..], &[true; 3]);
        assert!(zone.allowed_monsters()[8]); // Fortress: the Complete bug is not carried over.
        assert!(!zone.allowed_monsters()[9]); // Conflux.
        assert_eq!(&zone.allowed_monsters()[10..], &[true; 3]);
        assert_eq!(&zone.allowed_terrain()[8..], &[false, false, true, true]);
        assert!(template.options().spell_research); // No column, constructor default.
        assert!(zone.options().monsters_join_only_for_money);
    }

    #[test]
    fn malformed_headers_and_numbers_return_located_errors() {
        let mut first = row("bad", 1, 1);
        first[0] = "-1".into();
        let bytes = sheet(&[first.clone()]);
        assert!(matches!(
            TemplateSource::parse(&bytes)
                .unwrap()
                .pack_settings(Ruleset::HotA181),
            Err(TemplateError::Pack {
                row: 3,
                column: 0,
                ..
            })
        ));
        first[0] = "12".into();
        first[24] = "1e9999".into();
        let bytes = sheet(&[first, row("", 2, 2)]);
        assert!(matches!(
            TemplateSource::parse(&bytes).unwrap().prepare_for(
                &request(),
                Water::None,
                Ruleset::HotA181
            ),
            Err(TemplateError::Pack {
                row: 3,
                column: 24,
                ..
            })
        ));
    }
}
