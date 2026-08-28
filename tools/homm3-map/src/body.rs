//! Placed adventure objects and the final timed-event list.

use crate::{checked_count, CountField, Cursor, Error, MapString, ObjectTable, Version};

const RETAIL_OBJECT_CLASS_COUNT: u32 = 232;
const ARMY_SLOTS: usize = 7;
const RESOURCE_COUNT: usize = 7;
/// Zero-filled editor trailer after the final timed-event record.
pub const MAP_TRAILING_PADDING_SIZE: usize = 124;

/// A completely validated H3M body after the object-template table.
#[derive(Clone, Copy, Debug)]
pub struct MapBody<'a> {
    table: ObjectTable<'a>,
    version: Version,
    object_records: &'a [u8],
    event_bytes: &'a [u8],
    events: TimedEvents<'a>,
    padding: &'a [u8],
    trailing: &'a [u8],
    consumed: usize,
}

impl<'a> MapBody<'a> {
    /// Parse every placed object and the global timed-event list that follows.
    ///
    /// Object class dispatch follows retail `NewfullMap::readObject`, including
    /// every override installed into the 232-row adventure-trait table by the
    /// retail initializer.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for truncated records, invalid template references,
    /// object classes outside the retail table, invalid quest/reward tags, or
    /// impossible signed counts.
    pub fn parse(table: ObjectTable<'a>, version: Version) -> Result<Self, Error> {
        let source = table.objects();
        let mut cursor = Cursor::new(source);
        let mut previous_class = None;
        for object_ordinal in 0..table.object_count() {
            let object = parse_placed_object(
                &mut cursor,
                source,
                table,
                version,
                object_ordinal,
                previous_class,
            )?;
            previous_class = Some(object.object_class);
        }
        let objects_end = cursor.position();

        let event_bytes_start = cursor.position();
        let count_offset = cursor.position();
        let event_count = checked_count(cursor.i32()?, count_offset, CountField::TimedEvents)?;
        let event_records_start = cursor.position();
        for _ in 0..event_count {
            let _ = parse_timed_event(&mut cursor, source, version)?;
        }
        let events_end = cursor.position();
        let padding_start = cursor.position();
        let padding = cursor.take(MAP_TRAILING_PADDING_SIZE)?;
        if let Some(index) = padding.iter().position(|&byte| byte != 0) {
            return Err(Error::NonZeroPadding {
                offset: padding_start + index,
            });
        }
        let consumed = cursor.position();

        Ok(Self {
            table,
            version,
            object_records: checked_slice(source, 0, objects_end)?,
            event_bytes: checked_slice(source, event_bytes_start, events_end)?,
            events: TimedEvents {
                records: checked_slice(source, event_records_start, events_end)?,
                count: event_count,
                version,
            },
            padding,
            trailing: cursor.remaining(),
            consumed,
        })
    }

    /// Number of bytes through the zero-filled editor trailer.
    #[must_use]
    pub const fn consumed(self) -> usize {
        self.consumed
    }

    /// Raw placed-object records, excluding their count dword.
    #[must_use]
    pub const fn object_bytes(self) -> &'a [u8] {
        self.object_records
    }

    /// Iterate the already-validated placed-object records.
    #[must_use]
    pub const fn objects(self) -> PlacedObjects<'a> {
        PlacedObjects {
            cursor: Cursor::new(self.object_records),
            source: self.object_records,
            table: self.table,
            version: self.version,
            remaining: self.table.object_count(),
            next_ordinal: 0,
            previous_class: None,
        }
    }

    /// Raw final event-list bytes, including its count dword.
    #[must_use]
    pub const fn event_bytes(self) -> &'a [u8] {
        self.event_bytes
    }

    /// Validated final timed-event list.
    #[must_use]
    pub const fn timed_events(self) -> TimedEvents<'a> {
        self.events
    }

    /// Validated zero-filled 124-byte editor trailer.
    #[must_use]
    pub const fn padding(self) -> &'a [u8] {
        self.padding
    }

    /// Bytes after the final declared timed event.
    #[must_use]
    pub const fn trailing(self) -> &'a [u8] {
        self.trailing
    }
}

/// One placed adventure-object record.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PlacedObject<'a> {
    /// Map coordinates in x/y/layer order.
    pub coordinates: [u8; 3],
    /// Index into the preceding object-template table.
    pub type_index: u32,
    /// Original class stored in the object-template record.
    pub serialized_class: u32,
    /// Retail adventure-object dispatch class after the trait-table remap.
    pub object_class: u32,
    /// Template subtype used by class-specific readers.
    pub extra: u32,
    /// Complete serialized record, including coordinates and five-byte pad.
    pub raw: &'a [u8],
    /// Class-specific bytes after the common twelve-byte prefix.
    pub payload: &'a [u8],
}

/// Iterator over validated placed-object records.
#[derive(Clone, Debug)]
pub struct PlacedObjects<'a> {
    cursor: Cursor<'a>,
    source: &'a [u8],
    table: ObjectTable<'a>,
    version: Version,
    remaining: u32,
    next_ordinal: u32,
    previous_class: Option<u32>,
}

impl<'a> Iterator for PlacedObjects<'a> {
    type Item = PlacedObject<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.remaining == 0 {
            return None;
        }
        self.remaining -= 1;
        let object = parse_placed_object(
            &mut self.cursor,
            self.source,
            self.table,
            self.version,
            self.next_ordinal,
            self.previous_class,
        )
        .ok()?;
        self.next_ordinal += 1;
        self.previous_class = Some(object.object_class);
        Some(object)
    }
}

/// Validated global timed-event records.
#[derive(Clone, Copy, Debug)]
pub struct TimedEvents<'a> {
    records: &'a [u8],
    count: u32,
    version: Version,
}

impl<'a> TimedEvents<'a> {
    /// Number of timed events.
    #[must_use]
    pub const fn len(self) -> u32 {
        self.count
    }

    /// Whether the list is empty.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        self.count == 0
    }

    /// Iterate validated records.
    #[must_use]
    pub const fn iter(self) -> TimedEventIter<'a> {
        TimedEventIter {
            cursor: Cursor::new(self.records),
            source: self.records,
            remaining: self.count,
            version: self.version,
        }
    }
}

impl<'a> IntoIterator for TimedEvents<'a> {
    type Item = TimedEvent<'a>;
    type IntoIter = TimedEventIter<'a>;

    fn into_iter(self) -> Self::IntoIter {
        self.iter()
    }
}

/// One global or town timed-event base record.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct TimedEvent<'a> {
    /// Editor event name. Retail reads and discards this field during H3M load.
    pub name: MapString<'a>,
    /// Player-facing event message.
    pub message: MapString<'a>,
    /// Seven signed resource deltas.
    pub resources: [i32; RESOURCE_COUNT],
    /// Player applicability mask.
    pub player_flags: u8,
    /// Human-player flag; forced true by retail before format 28.
    pub apply_to_human: bool,
    /// Computer-player flag.
    pub apply_to_computer: bool,
    /// Zero-based first day stored in H3M.
    pub stored_first_day: u16,
    /// First day after retail's wrapping increment.
    pub first_day: u16,
    /// Repeat interval.
    pub interval: u16,
    /// Complete base record including the discarded sixteen-byte tail.
    pub raw: &'a [u8],
}

/// Iterator over validated timed-event records.
#[derive(Clone, Debug)]
pub struct TimedEventIter<'a> {
    cursor: Cursor<'a>,
    source: &'a [u8],
    remaining: u32,
    version: Version,
}

impl<'a> Iterator for TimedEventIter<'a> {
    type Item = TimedEvent<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.remaining == 0 {
            return None;
        }
        self.remaining -= 1;
        parse_timed_event(&mut self.cursor, self.source, self.version).ok()
    }
}

fn parse_placed_object<'a>(
    cursor: &mut Cursor<'a>,
    source: &'a [u8],
    table: ObjectTable<'a>,
    version: Version,
    object_ordinal: u32,
    previous_class: Option<u32>,
) -> Result<PlacedObject<'a>, Error> {
    let start = cursor.position();
    let coordinates = [cursor.u8()?, cursor.u8()?, cursor.u8()?];
    let type_offset = cursor.position();
    let type_index = cursor.u32()?;
    if type_index > u32::from(u16::MAX) || type_index >= table.type_count() {
        return Err(Error::BadObjectTypeIndex {
            offset: type_offset,
            value: type_index,
            count: table.type_count(),
            object_ordinal,
            previous_class,
        });
    }
    let object_type = table.type_at(type_index).ok_or(Error::BadObjectTypeIndex {
        offset: type_offset,
        value: type_index,
        count: table.type_count(),
        object_ordinal,
        previous_class,
    })?;
    let serialized_class = object_type.object_class;
    if serialized_class >= RETAIL_OBJECT_CLASS_COUNT {
        return Err(Error::BadObjectClass {
            offset: type_offset,
            value: serialized_class,
        });
    }
    let object_class = retail_object_class(serialized_class);
    let _padding = cursor.take(5)?;
    let payload_start = cursor.position();
    parse_object_payload(cursor, source, object_class, object_type.extra, version)?;
    let end = cursor.position();
    Ok(PlacedObject {
        coordinates,
        type_index,
        serialized_class,
        object_class,
        extra: object_type.extra,
        raw: checked_slice(source, start, end)?,
        payload: checked_slice(source, payload_start, end)?,
    })
}

fn parse_object_payload<'a>(
    cursor: &mut Cursor<'a>,
    source: &'a [u8],
    object_class: u32,
    extra: u32,
    version: Version,
) -> Result<(), Error> {
    match object_class {
        5 | 65..=69 => parse_artifact(cursor, version)?,
        34 | 62 | 70 => parse_hero(cursor, version)?,
        8 => {}
        77 | 98 => parse_town(cursor, source, version)?,
        54 | 71..=75 | 162..=164 => parse_monster(cursor, version)?,
        26 => {
            parse_black_box(cursor, version)?;
            let _flags = cursor.take(3)?;
            let _padding = cursor.take(4)?;
        }
        214 => {
            let _owner = cursor.u8()?;
            let hero_id = cursor.u8()?;
            if hero_id == 0xff {
                let _power_rating = cursor.u8()?;
            }
        }
        93 => {
            parse_artifact(cursor, version)?;
            let _spell = cursor.u8()?;
            let _padding = cursor.take(3)?;
        }
        17 | 20 | 36 | 42 | 53 | 87 | 88..=90 => {
            let _class_value = cursor.u8()?;
            let _padding = cursor.take(3)?;
        }
        76 | 79 => {
            parse_artifact(cursor, version)?;
            let _amount = cursor.take(4)?;
            let _padding = cursor.take(4)?;
        }
        6 => parse_black_box(cursor, version)?,
        81 => {
            let _award_and_value = cursor.take(2)?;
            let _padding = cursor.take(6)?;
        }
        83 => parse_seer_hut(cursor, version)?,
        59 | 91 => {
            let _text = cursor.map_string()?;
            let _padding = cursor.take(4)?;
        }
        33 => parse_garrison(cursor, version)?,
        216 => {
            let _owner = cursor.u8()?;
            let _padding = cursor.take(3)?;
            let castle_id = cursor.i32()?;
            if castle_id == 0 {
                let _faction_mask = cursor.u16()?;
            }
            let _levels = cursor.take(2)?;
        }
        217 => {
            let _owner = cursor.u8()?;
            let _padding = cursor.take(3)?;
            let castle_id = cursor.i32()?;
            if castle_id == 0 {
                let _faction_mask = cursor.u16()?;
            }
        }
        218 => {
            let _owner = cursor.u8()?;
            let _padding = cursor.take(3)?;
            let _levels = cursor.take(2)?;
        }
        215 => parse_quest(cursor)?,
        113 if version != Version::Restoration => {
            let _allowed_skills = cursor.take(4)?;
        }
        _ => {
            let _ = extra;
        }
    }
    Ok(())
}

fn parse_artifact(cursor: &mut Cursor<'_>, version: Version) -> Result<(), Error> {
    if cursor.u8()? != 0 {
        parse_treasure(cursor, version)?;
    }
    Ok(())
}

fn parse_treasure(cursor: &mut Cursor<'_>, version: Version) -> Result<(), Error> {
    let _message = cursor.map_string()?;
    if cursor.u8()? != 0 {
        skip_army(cursor, version)?;
    }
    let _padding = cursor.take(4)?;
    Ok(())
}

fn parse_black_box(cursor: &mut Cursor<'_>, version: Version) -> Result<(), Error> {
    if cursor.u8()? != 0 {
        parse_treasure(cursor, version)?;
    }
    let _experience_mana_morale_luck = cursor.take(10)?;
    let _resources = cursor.take(RESOURCE_COUNT * 4)?;
    let _primary_skills = cursor.take(4)?;

    let secondary_count = usize::from(cursor.u8()?);
    take_repeated(cursor, secondary_count, 2)?;
    let artifact_count = usize::from(cursor.u8()?);
    take_repeated(cursor, artifact_count, item_width(version))?;
    let spell_count = usize::from(cursor.u8()?);
    take_repeated(cursor, spell_count, 1)?;

    let creature_count_offset = cursor.position();
    let creature_count = cursor.u8()?;
    if usize::from(creature_count) > ARMY_SLOTS {
        return Err(Error::BadCount {
            offset: creature_count_offset,
            value: i32::from(creature_count),
            field: CountField::BlackBoxCreatures,
        });
    }
    take_repeated(cursor, usize::from(creature_count), item_width(version) + 2)?;
    let _padding = cursor.take(8)?;
    Ok(())
}

fn parse_monster(cursor: &mut Cursor<'_>, version: Version) -> Result<(), Error> {
    if version != Version::Restoration {
        let _identifier = cursor.take(4)?;
    }
    let _quantity_and_grade = cursor.take(3)?;
    if cursor.u8()? != 0 {
        let _message = cursor.map_string()?;
        let _resources = cursor.take(RESOURCE_COUNT * 4)?;
        let _artifact = cursor.take(item_width(version))?;
    }
    let _flags_and_padding = cursor.take(4)?;
    Ok(())
}

fn parse_town<'a>(
    cursor: &mut Cursor<'a>,
    source: &'a [u8],
    version: Version,
) -> Result<(), Error> {
    if version != Version::Restoration {
        let _identifier = cursor.take(4)?;
    }
    let _owner = cursor.u8()?;
    if cursor.u8()? != 0 {
        let _name = cursor.map_string()?;
    }
    if cursor.u8()? != 0 {
        skip_army(cursor, version)?;
    }
    let _grouped = cursor.u8()?;
    if cursor.u8()? != 0 {
        let _building_masks = cursor.take(12)?;
    } else {
        let _has_fort = cursor.u8()?;
    }
    if version != Version::Restoration {
        let _fixed_spells = cursor.take(9)?;
    }
    let _possible_spells = cursor.take(9)?;

    let count_offset = cursor.position();
    let count = checked_count(cursor.i32()?, count_offset, CountField::TownEvents)?;
    for _ in 0..count {
        let _ = parse_timed_event(cursor, source, version)?;
        let _building_mask = cursor.take(6)?;
        let _generator_bonuses = cursor.take(14)?;
        let _padding = cursor.take(4)?;
    }
    if version == Version::ShadowOfDeath {
        let _random_town_alignment = cursor.u8()?;
    }
    let _padding = cursor.take(3)?;
    Ok(())
}

fn parse_hero(cursor: &mut Cursor<'_>, version: Version) -> Result<(), Error> {
    if version != Version::Restoration {
        let _identifier = cursor.take(4)?;
    }
    let _owner = cursor.u8()?;
    let _hero_id = cursor.u8()?;
    if cursor.u8()? != 0 {
        let _name = cursor.map_string()?;
    }
    if version == Version::ShadowOfDeath {
        if cursor.u8()? != 0 {
            let _experience = cursor.take(4)?;
        }
    } else {
        let _experience = cursor.take(4)?;
    }
    if cursor.u8()? != 0 {
        let _portrait = cursor.u8()?;
    }
    if cursor.u8()? != 0 {
        let count_offset = cursor.position();
        let count = checked_count(
            cursor.i32()?,
            count_offset,
            CountField::PlacedHeroSecondarySkills,
        )?;
        take_repeated(
            cursor,
            usize::try_from(count).map_err(|_| Error::SizeOverflow {
                offset: count_offset,
            })?,
            2,
        )?;
    }
    if cursor.u8()? != 0 {
        skip_army(cursor, version)?;
    }
    let _formation = cursor.u8()?;
    if cursor.u8()? != 0 {
        let equipped_count = if version == Version::ShadowOfDeath {
            19
        } else {
            18
        };
        take_repeated(cursor, equipped_count, item_width(version))?;
        let backpack_count = usize::from((cursor.u16()? & 0xff) as u8);
        take_repeated(cursor, backpack_count, item_width(version))?;
    }
    let _patrol_radius = cursor.u8()?;
    if version != Version::Restoration {
        if cursor.u8()? != 0 {
            let _biography = cursor.map_string()?;
        }
        let _sex = cursor.u8()?;
        if version == Version::ArmageddonsBlade {
            let _spell = cursor.u8()?;
        } else {
            if cursor.u8()? != 0 {
                let _spells = cursor.take(9)?;
            }
            if cursor.u8()? != 0 {
                let _primary_skills = cursor.take(4)?;
            }
        }
    }
    let _padding = cursor.take(16)?;
    Ok(())
}

fn parse_garrison(cursor: &mut Cursor<'_>, version: Version) -> Result<(), Error> {
    let _owner = cursor.u8()?;
    let _padding = cursor.take(3)?;
    skip_army(cursor, version)?;
    if version != Version::Restoration {
        let _removable = cursor.u8()?;
    }
    let _padding = cursor.take(8)?;
    Ok(())
}

fn parse_quest(cursor: &mut Cursor<'_>) -> Result<(), Error> {
    let kind_offset = cursor.position();
    let kind = cursor.u8()?;
    match kind {
        0 => return Ok(()),
        1..=4 => {
            let _value = cursor.take(4)?;
        }
        5 => {
            let count = usize::from(cursor.u8()?);
            take_repeated(cursor, count, 2)?;
        }
        6 => {
            let count = usize::from(cursor.u8()?);
            take_repeated(cursor, count, 4)?;
        }
        7 => {
            let _resources = cursor.take(RESOURCE_COUNT * 4)?;
        }
        8 | 9 => {
            let _identity = cursor.u8()?;
        }
        _ => {
            return Err(Error::BadQuestType {
                offset: kind_offset,
                value: kind,
            });
        }
    }
    let _deadline = cursor.take(4)?;
    let _proposal = cursor.map_string()?;
    let _progress = cursor.map_string()?;
    let _completion = cursor.map_string()?;
    Ok(())
}

fn parse_seer_hut(cursor: &mut Cursor<'_>, version: Version) -> Result<(), Error> {
    if version == Version::Restoration {
        let _artifact_or_none = cursor.u8()?;
    } else {
        parse_quest(cursor)?;
    }

    let reward_offset = cursor.position();
    let reward = cursor.u8()?;
    match reward {
        0 => {}
        1 | 2 => {
            let _value = cursor.take(4)?;
        }
        3 | 4 | 9 => {
            let _value = cursor.u8()?;
        }
        5 => {
            let _resource = cursor.u8()?;
            let _quantity = cursor.take(4)?;
        }
        6 | 7 => {
            let _skill_and_value = cursor.take(2)?;
        }
        8 => {
            let _artifact = cursor.take(item_width(version))?;
        }
        10 => {
            let _creature = cursor.take(item_width(version))?;
            let _quantity = cursor.take(2)?;
        }
        _ => {
            return Err(Error::BadSeerReward {
                offset: reward_offset,
                value: reward,
            });
        }
    }
    let _padding = cursor.take(2)?;
    Ok(())
}

fn parse_timed_event<'a>(
    cursor: &mut Cursor<'a>,
    source: &'a [u8],
    version: Version,
) -> Result<TimedEvent<'a>, Error> {
    let start = cursor.position();
    let name = cursor.map_string()?;
    let message = cursor.map_string()?;
    let mut resources = [0; RESOURCE_COUNT];
    for resource in &mut resources {
        *resource = cursor.i32()?;
    }
    let player_flags = cursor.u8()?;
    let apply_to_human = if version == Version::ShadowOfDeath {
        cursor.u8()? != 0
    } else {
        true
    };
    let apply_to_computer = cursor.u8()? != 0;
    let stored_first_day = cursor.u16()?;
    let interval = cursor.u16()?;
    let _padding = cursor.take(16)?;
    let end = cursor.position();
    Ok(TimedEvent {
        name,
        message,
        resources,
        player_flags,
        apply_to_human,
        apply_to_computer,
        stored_first_day,
        first_day: stored_first_day.wrapping_add(1),
        interval,
        raw: checked_slice(source, start, end)?,
    })
}

fn skip_army(cursor: &mut Cursor<'_>, version: Version) -> Result<(), Error> {
    take_repeated(cursor, ARMY_SLOTS, item_width(version) + 2)
}

const fn item_width(version: Version) -> usize {
    if matches!(version, Version::Restoration) {
        1
    } else {
        2
    }
}

const fn retail_object_class(serialized: u32) -> u32 {
    match serialized {
        165..=189 => serialized - 51,
        190 => 143,
        191..=205 => serialized - 44,
        219 => 33,
        220 => 53,
        221 => 99,
        223 => 21,
        230 => 46,
        _ => serialized,
    }
}

fn take_repeated(cursor: &mut Cursor<'_>, count: usize, width: usize) -> Result<(), Error> {
    let offset = cursor.position();
    let size = count
        .checked_mul(width)
        .ok_or(Error::SizeOverflow { offset })?;
    let _ = cursor.take(size)?;
    Ok(())
}

fn checked_slice(source: &[u8], start: usize, end: usize) -> Result<&[u8], Error> {
    source.get(start..end).ok_or(Error::Short {
        offset: start,
        needed: end.saturating_sub(start),
        available: source.len().saturating_sub(start),
    })
}

#[cfg(test)]
mod tests {
    use super::retail_object_class;

    #[test]
    fn retail_trait_table_remaps_all_46_overrides() {
        for serialized in 165..=189 {
            assert_eq!(retail_object_class(serialized), serialized - 51);
        }
        assert_eq!(retail_object_class(190), 143);
        for serialized in 191..=205 {
            assert_eq!(retail_object_class(serialized), serialized - 44);
        }
        for (serialized, dispatch) in [(219, 33), (220, 53), (221, 99), (223, 21), (230, 46)] {
            assert_eq!(retail_object_class(serialized), dispatch);
        }
        for identity in [0, 164, 206, 218, 222, 224, 229, 231] {
            assert_eq!(retail_object_class(identity), identity);
        }
    }
}
