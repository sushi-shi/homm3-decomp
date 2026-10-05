use super::*;
use crate::{
    behavior::Behavior,
    placement_rules::PlacementRules,
    prototype::{ImageMask, MaskCell, PrototypeCatalog, PrototypeSource},
    request::MapVersion,
    rules::Ruleset,
};
use std::convert::Infallible;

/// Kind, subtype, (width, height) and trigger cells as (column, row).
type Object<'a> = (u32, i32, (u8, u8), &'a [(u8, u8)]);

/// One fully drawn, fully blocked prototype per object.
struct Fixture {
    text: Vec<u8>,
    sizes: Vec<(u8, u8)>,
    rules: PlacementRules,
}
impl Fixture {
    fn new(objects: &[Object<'_>]) -> Self {
        let mut rows = Vec::new();
        for (index, (kind, subtype, _, triggers)) in objects.iter().enumerate() {
            let mut trigger = 0_u64;
            for &(column, row) in *triggers {
                // Serialized masks list the anchor-relative ordinals in reverse.
                trigger |= 1 << (47 - (u32::from(row) * 8 + u32::from(column)));
            }
            rows.push(format!(
                "o{index}.def {:048b} {trigger:048b} {:012b} {:012b} {kind} {subtype} 0 0",
                0_u64, 0xfff, 0xfff
            ));
        }
        Self {
            text: format!("{}\r\n{}\r\n", rows.len(), rows.join("\r\n")).into_bytes(),
            sizes: objects.iter().map(|object| object.2).collect(),
            rules: PlacementRules::parse_for(
                b"header\r\nheader\r\nheader\r\n",
                Behavior::Hotfix,
                Ruleset::HotA181,
            )
            .unwrap(),
        }
    }
    fn catalog<'s>(&'s self, source: &'s PrototypeSource<'s>) -> PrototypeCatalog<'s> {
        source
            .prepare(&self.rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap()
    }
    fn source(&self) -> PrototypeSource<'_> {
        let mut next = 0;
        PrototypeSource::parse_for(&self.text, Ruleset::HotA181, |_| {
            let (width, height) = self.sizes[next];
            next += 1;
            let mut bytes = [0xff_u8; 14];
            bytes[0] = width;
            bytes[1] = height;
            Ok::<_, Infallible>(Some(ImageMask::parse(&bytes).unwrap()))
        })
        .unwrap()
    }
}

fn at(x: i32, y: i32) -> WorldPosition {
    WorldPosition {
        point: Point::new(x, y),
        level: Level::Surface,
    }
}

fn flagged(records: &CellRecords, flag: CellFlag) -> Vec<(i32, i32)> {
    let mut cells = Vec::new();
    for y in 0..records.side {
        for x in 0..records.side {
            if records.get(at(x, y)).unwrap().contains(flag) {
                cells.push((x, y));
            }
        }
    }
    cells
}

fn square(x0: i32, y0: i32, x1: i32, y1: i32) -> Vec<(i32, i32)> {
    let mut cells = Vec::new();
    for y in y0..y1 {
        for x in x0..x1 {
            cells.push((x, y));
        }
    }
    cells
}

const OUTSIDE: StampContext = StampContext {
    zone_treasures: false,
    exceeds_zone_guard: false,
};

#[test]
fn fixture_triggers_use_anchor_relative_cells() {
    let fixture = Fixture::new(&[(raw_kind(ObjectKind::MINE), 0, (3, 2), &[(1, 0)])]);
    let source = fixture.source();
    let catalog = fixture.catalog(&source);
    let prototype = catalog.entries()[0].prototype();
    assert!(prototype.is_trigger(MaskCell::parse(1, 0).unwrap()));
    assert!(!prototype.is_trigger(MaskCell::parse(0, 0).unwrap()));
    assert_eq!(prototype.entrance(), MaskCell::parse(1, 0));
}

fn raw_kind(kind: ObjectKind) -> u32 {
    u32::try_from(kind.index()).unwrap()
}

#[test]
fn monsters_outside_treasure_placement_are_strong() {
    let fixture = Fixture::new(&[(raw_kind(ObjectKind::MONSTER), 3, (1, 1), &[(0, 0)])]);
    let source = fixture.source();
    let catalog = fixture.catalog(&source);
    let mut records = CellRecords::default();
    records.reset(12, 1).unwrap();
    records
        .stamp(&catalog.entries()[0], at(5, 5), OUTSIDE)
        .unwrap();
    let near = square(4, 4, 7, 7);
    assert_eq!(flagged(&records, CellFlag::NearTrigger), near);
    assert_eq!(flagged(&records, CellFlag::NearMonster), near);
    assert_eq!(flagged(&records, CellFlag::NearStrongMonster), near);
    assert_eq!(flagged(&records, CellFlag::MonsterArea), square(3, 3, 8, 8));
    assert_eq!(flagged(&records, CellFlag::StrongMonster), [(5, 5)]);
    // Monsters never mark the cell below as an entrance.
    assert!(flagged(&records, CellFlag::Entrance).is_empty());

    // During treasure placement only a guard above the zone's maximum is strong.
    records.reset(12, 1).unwrap();
    let weak = StampContext {
        zone_treasures: true,
        exceeds_zone_guard: false,
    };
    records
        .stamp(&catalog.entries()[0], at(0, 0), weak)
        .unwrap();
    assert_eq!(flagged(&records, CellFlag::NearMonster), square(0, 0, 2, 2));
    assert!(flagged(&records, CellFlag::NearStrongMonster).is_empty());
    assert!(flagged(&records, CellFlag::StrongMonster).is_empty());
}

#[test]
fn entrances_mark_the_band_below_until_treasure_placement() {
    let fixture = Fixture::new(&[(raw_kind(ObjectKind::MINE), 0, (3, 2), &[(1, 0)])]);
    let source = fixture.source();
    let catalog = fixture.catalog(&source);
    let mut records = CellRecords::default();
    records.reset(10, 1).unwrap();
    records
        .stamp(&catalog.entries()[0], at(6, 4), OUTSIDE)
        .unwrap();
    // The trigger is one column west of the anchor.
    assert_eq!(flagged(&records, CellFlag::NearTrigger), square(4, 3, 7, 6));
    assert_eq!(flagged(&records, CellFlag::Entrance), [(5, 5)]);
    assert_eq!(
        flagged(&records, CellFlag::NearEntrance),
        square(4, 4, 7, 7)
    );
    assert!(flagged(&records, CellFlag::NearMonster).is_empty());

    records.reset(10, 1).unwrap();
    let inside = StampContext {
        zone_treasures: true,
        exceeds_zone_guard: true,
    };
    records
        .stamp(&catalog.entries()[0], at(6, 4), inside)
        .unwrap();
    assert!(flagged(&records, CellFlag::Entrance).is_empty());
    // The bottom row has no cell below it.
    records.reset(10, 1).unwrap();
    records
        .stamp(&catalog.entries()[0], at(6, 9), OUTSIDE)
        .unwrap();
    assert!(flagged(&records, CellFlag::Entrance).is_empty());
}

#[test]
fn airship_yard_approach_wraps_to_the_adjacent_row() {
    let fixture = Fixture::new(&[(raw_kind(ObjectKind::SHIPYARD), 1, (1, 1), &[(0, 0)])]);
    let source = fixture.source();
    let catalog = fixture.catalog(&source);
    let mut records = CellRecords::default();
    records.reset(8, 2).unwrap();
    records
        .stamp(&catalog.entries()[0], at(0, 3), OUTSIDE)
        .unwrap();
    // x - 1 at column 0 is the last cell of row 3.
    assert_eq!(
        flagged(&records, CellFlag::AirshipYardEntrance),
        [(7, 3), (0, 4), (1, 4)]
    );
    assert_eq!(
        flagged(&records, CellFlag::NearAirshipYard),
        square(0, 3, 3, 6)
    );
    // At the last surface cell the flat x + 1 neighbour is underground.
    records.reset(8, 2).unwrap();
    records
        .stamp(&catalog.entries()[0], at(7, 6), OUTSIDE)
        .unwrap();
    assert_eq!(
        flagged(&records, CellFlag::AirshipYardEntrance),
        [(6, 7), (7, 7)]
    );
    let below = WorldPosition {
        point: Point::new(0, 0),
        level: Level::Underground,
    };
    assert!(records
        .get(below)
        .unwrap()
        .contains(CellFlag::AirshipYardEntrance));
}

#[test]
fn removal_rederives_from_remaining_entrances() {
    let fixture = Fixture::new(&[
        (raw_kind(ObjectKind::MONSTER), 0, (1, 1), &[(0, 0)]),
        (raw_kind(ObjectKind::MINE), 0, (3, 2), &[(1, 0)]),
    ]);
    let source = fixture.source();
    let catalog = fixture.catalog(&source);
    let mut records = CellRecords::default();
    records.reset(12, 1).unwrap();
    let monster = &catalog.family(ObjectKind::MONSTER)[0];
    records.stamp(monster, at(5, 5), OUTSIDE).unwrap();
    records.stamp(monster, at(7, 5), OUTSIDE).unwrap();
    // Remove the first monster; the second remains an entrance.
    let remaining = |position: WorldPosition| {
        Ok((position.point == Point::new(7, 5)).then_some(ObjectKind::MONSTER))
    };
    records.unstamp(monster, at(5, 5), remaining).unwrap();
    assert_eq!(flagged(&records, CellFlag::NearTrigger), square(6, 4, 9, 7));
    assert_eq!(flagged(&records, CellFlag::NearMonster), square(6, 4, 9, 7));
    // The removed trigger cleared prototype-local (0, 0), not map (5, 5),
    // and re-derivation found the strong monster still standing at (7, 5).
    assert_eq!(flagged(&records, CellFlag::StrongMonster), [(5, 5), (7, 5)]);
    // Because that local byte was clear, the removal did not count as a
    // strong monster and left the strong-neighbour bytes as stamped.
    assert_eq!(
        flagged(&records, CellFlag::NearStrongMonster),
        square(4, 4, 9, 7)
    );

    let mine = &catalog.family(ObjectKind::MINE)[0];
    records.reset(12, 1).unwrap();
    records.stamp(mine, at(6, 4), OUTSIDE).unwrap();
    records.write(1, 1, 0, CellFlag::Entrance, true);
    records.unstamp(mine, at(6, 4), |_| Ok(None)).unwrap();
    assert!(flagged(&records, CellFlag::NearTrigger).is_empty());
    assert!(flagged(&records, CellFlag::NearEntrance).is_empty());
    // Prototype-local (1, 1) was cleared; map (5, 5) keeps its byte.
    assert_eq!(flagged(&records, CellFlag::Entrance), [(5, 5)]);
}
