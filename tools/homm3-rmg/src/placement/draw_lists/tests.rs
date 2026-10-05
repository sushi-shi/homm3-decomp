use super::*;
use crate::{
    behavior::Behavior,
    domain::Level,
    object::ObjectKind,
    placement_rules::PlacementRules,
    prototype::{ImageMask, PrototypeSource},
    request::MapVersion,
    rules::Ruleset,
};
use std::convert::Infallible;

/// Kind, (width, height) and passable-but-drawn cells as (column, row).
type Object<'a> = (u32, (u8, u8), &'a [(u8, u8)]);

struct Fixture {
    text: Vec<u8>,
    sizes: Vec<(u8, u8)>,
    rules: PlacementRules,
}
impl Fixture {
    fn new(objects: &[Object<'_>]) -> Self {
        let mut rows = Vec::new();
        for (index, (kind, _, passable)) in objects.iter().enumerate() {
            let mut mask = 0_u64;
            for &(column, row) in *passable {
                mask |= 1 << (47 - (u32::from(row) * 8 + u32::from(column)));
            }
            rows.push(format!(
                "o{index}.def {mask:048b} {:048b} {:012b} {:012b} {kind} 0 0 0",
                0_u64, 0xfff, 0xfff
            ));
        }
        Self {
            text: format!("{}\r\n{}\r\n", rows.len(), rows.join("\r\n")).into_bytes(),
            sizes: objects.iter().map(|object| object.1).collect(),
            rules: PlacementRules::parse_for(
                b"header\r\nheader\r\nheader\r\n",
                Behavior::Hotfix,
                Ruleset::HotA181,
            )
            .unwrap(),
        }
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
    fn catalog<'s>(&'s self, source: &'s PrototypeSource<'s>) -> PrototypeCatalog<'s> {
        source
            .prepare(&self.rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap()
    }
}

fn at(x: i32, y: i32) -> WorldPosition {
    WorldPosition {
        point: Point::new(x, y),
        level: Level::Surface,
    }
}

fn id(catalog: &PrototypeCatalog<'_>, kind: u32) -> PrototypeId {
    let kind = ObjectKind::parse(i32::try_from(kind).unwrap()).unwrap();
    catalog.at(kind.family(), 0).unwrap()
}

fn place(
    lists: &mut DrawLists,
    objects: &mut ObjectArena,
    catalog: &PrototypeCatalog<'_>,
    prototype: PrototypeId,
    anchor: WorldPosition,
) -> ObjectId {
    let object = objects.create(catalog, prototype).unwrap();
    objects.set_position(object, anchor);
    lists
        .stamp(catalog.get(prototype).unwrap(), object, anchor)
        .unwrap();
    object
}

#[test]
fn lists_follow_drawn_cells_and_remove_first_occurrences() {
    let fixture = Fixture::new(&[(53, (2, 2), &[])]);
    let source = fixture.source();
    let catalog = fixture.catalog(&source);
    let mine = id(&catalog, 53);
    let mut lists = DrawLists::default();
    lists.reset(4, 1).unwrap();
    let mut objects = ObjectArena::default();
    let a = place(&mut lists, &mut objects, &catalog, mine, at(1, 1));
    let b = place(&mut lists, &mut objects, &catalog, mine, at(0, 1));
    let list = |lists: &DrawLists, x, y| lists.objects(at(x, y)).unwrap().collect::<Vec<_>>();
    assert_eq!(list(&lists, 1, 1), [a]);
    assert_eq!(list(&lists, 0, 0), [a, b]);
    assert_eq!(list(&lists, 0, 1), [a, b]);
    assert!(list(&lists, 2, 2).is_empty());
    // A second stamp of `a` appends again; removal erases one occurrence.
    let entry = catalog.get(mine).unwrap();
    lists.stamp(entry, a, at(1, 1)).unwrap();
    assert_eq!(list(&lists, 0, 0), [a, b, a]);
    lists.unstamp(entry, a, at(1, 1)).unwrap();
    assert_eq!(list(&lists, 0, 0), [b, a]);
    assert_eq!(list(&lists, 1, 1), [a]);
    // The underground plane is absent from a one-level map.
    let below = WorldPosition {
        point: Point::new(1, 1),
        level: Level::Underground,
    };
    assert!(matches!(
        lists.stamp(entry, a, below),
        Err(PlacementError::OutsideMap(_))
    ));
}

#[test]
fn opposite_orders_at_two_shared_cells_conflict() {
    // A 1x3 mine whose drawn middle cell is passable has priorities 1, 2, 1
    // from its anchor northwards; a blocked 1x3 candidate has 1, 2, 3.
    let fixture = Fixture::new(&[(53, (1, 3), &[(0, 1)]), (5, (1, 3), &[])]);
    let source = fixture.source();
    let catalog = fixture.catalog(&source);
    let mut lists = DrawLists::default();
    lists.reset(8, 1).unwrap();
    let mut objects = ObjectArena::default();
    place(
        &mut lists,
        &mut objects,
        &catalog,
        id(&catalog, 53),
        at(5, 5),
    );
    let mut scratch = ObstacleWorkspace::default();
    let candidate = id(&catalog, 5);
    let check = |scratch: &mut ObstacleWorkspace, anchor| {
        lists
            .consistent_draw_order(scratch, &catalog, &objects, candidate, anchor)
            .unwrap()
    };
    // At (5, 4) the mine is behind at its middle cell and covered above it.
    assert!(!check(&mut scratch, at(5, 4)));
    // Aligned, the candidate covers every shared cell.
    assert!(check(&mut scratch, at(5, 5)));
    // Sharing only the middle cell, the mine is merely behind.
    assert!(check(&mut scratch, at(5, 3)));
}

#[test]
fn decorations_lose_priority_ties_to_other_objects() {
    let fixture = Fixture::new(&[
        (53, (1, 3), &[(0, 1)]),
        (118, (1, 3), &[]),
        (5, (1, 3), &[]),
    ]);
    let source = fixture.source();
    let catalog = fixture.catalog(&source);
    let mut lists = DrawLists::default();
    lists.reset(8, 1).unwrap();
    let mut objects = ObjectArena::default();
    // Mine priorities 1, 2, 1; aligned candidates have 1, 2, 3.
    place(
        &mut lists,
        &mut objects,
        &catalog,
        id(&catalog, 53),
        at(3, 3),
    );
    let mut scratch = ObstacleWorkspace::default();
    let mut check = |kind| {
        lists
            .consistent_draw_order(
                &mut scratch,
                &catalog,
                &objects,
                id(&catalog, kind),
                at(3, 3),
            )
            .unwrap()
    };
    // The artifact covers the mine at both ties and above them.
    assert!(check(5));
    // A decoration is behind at the ties but covers the top cell.
    assert!(!check(118));
}
