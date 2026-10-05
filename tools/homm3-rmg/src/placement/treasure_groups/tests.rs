use super::*;
use crate::{
    behavior::Behavior,
    object::ObjectKind,
    placement_rules::PlacementRules,
    prototype::{ImageMask, PrototypeSource},
    request::MapVersion,
};
use std::convert::Infallible;

fn with_catalog(kind: u32, test: impl FnOnce(&PrototypeCatalog<'_>)) {
    let rules = PlacementRules::parse(b"header\r\nheader\r\nheader\r\n", Behavior::Hotfix).unwrap();
    let mut bytes = [0; 14];
    bytes[0] = 1;
    bytes[1] = 1;
    bytes[7] = 128;
    let mask = ImageMask::parse(&bytes).unwrap();
    let row = format!("1\r\ntest.def 0 {:048b} 1 1 {kind} 0 0 0\r\n", 1_u64 << 47);
    let source =
        PrototypeSource::parse(row.as_bytes(), |_| Ok::<_, Infallible>(Some(mask))).unwrap();
    let catalog = source
        .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
        .unwrap();
    test(&catalog);
}

#[test]
fn group_outline_uses_clearance_and_keeps_cached_order_and_bounds() {
    let mut group = TreasureGroupWorkspace::default();
    group.prepare_placement().unwrap();
    assert_eq!(group.bounds(), None);
    assert!(group.outline().is_empty());
    group.cells[8 * SIDE + 8].release_path();
    group.prepare_placement().unwrap();
    assert_eq!(group.bounds().unwrap().minimum(), Point::new(8, 8));
    assert_eq!(group.bounds().unwrap().maximum(), Point::new(9, 9));
    let ring = [
        (8, 7),
        (9, 7),
        (9, 8),
        (9, 9),
        (8, 9),
        (7, 9),
        (7, 8),
        (7, 7),
    ]
    .map(|(x, y)| Point::new(x, y));
    assert_eq!(group.outline(), ring);
    assert!(ring.into_iter().all(|p| group.is_outline(p).unwrap()));
    let outline_storage = group.outline.as_ptr();
    group.cells[10 * SIDE + 10].passable = false;
    group.prepare_placement().unwrap();
    assert_eq!(group.bounds().unwrap().maximum(), Point::new(11, 11));
    assert_eq!(group.outline(), ring); // Native trace uses a nonempty cached ring.
    group.reset_after_disposal();
    assert_eq!(group.bounds().unwrap().maximum(), Point::new(11, 11));
    assert!(group.outline.is_empty());
    assert_eq!(group.outline.as_ptr(), outline_storage);
    assert!(group.outline_marks.iter().all(|&mark| !mark));
    assert!((0..CELLS).all(|i| group.clear_outline_cell(i)));
}

#[test]
fn outline_halo_is_signed_and_marking_reports_native_allocation_fault() {
    let mut group = TreasureGroupWorkspace::default();
    group.cells[0].passable = false;
    group.trace_outline().unwrap();
    assert_eq!(group.outline()[0], Point::new(0, -1));
    assert!(group.outline().contains(&Point::new(-1, 0)));
    assert!(matches!(
        group.prepare_placement(),
        Err(PlacementError::OutsideMap(_))
    ));
}

#[test]
fn fitting_uses_native_row_aliases_and_reads_neighbors_before_footprint_bounds() {
    with_catalog(raw::RESOURCE, |catalog| {
        let mut group = TreasureGroupWorkspace::default();
        let entry = &catalog.entries()[0];
        assert!(group.can_fit(entry, Point::new(0, 4)).unwrap());
        // (-1,4), immediately west of the entrance, aliases (15,3).
        group.cells[4 * SIDE - 1].entrance = Some(ObjectKind::TOWN);
        assert!(!group.can_fit(entry, Point::new(0, 4)).unwrap());
        assert!(matches!(
            group.can_fit(entry, Point::new(-30, -30)),
            Err(PlacementError::OutsideMap(_))
        ));
        group.cells[4 * SIDE - 1].entrance = None;
        assert!(!group.can_fit(entry, Point::new(-1, 4)).unwrap());
    });
}

#[test]
fn group_ownership_rejects_copied_ids_and_other_group_disposal() {
    with_catalog(raw::RESOURCE, |catalog| {
        let mut arena = ObjectArena::default();
        let prototype = catalog
            .choose(
                ObjectKind::RESOURCE,
                0,
                crate::domain::Terrain::Dirt,
                &mut RetailRng::new(1),
            )
            .unwrap()
            .id();
        let object = arena.create(catalog, prototype).unwrap();
        let owner = OwnerId::new().unwrap();
        arena.claim_group(object, owner).unwrap();
        assert!(matches!(
            arena.require_world_insertion(object),
            Err(PlacementError::GroupOwned(_))
        ));
        assert!(matches!(
            arena.discard_unplaced(object),
            Err(PlacementError::PreviouslyPlaced(_))
        ));
        assert!(arena
            .recycle_group(object, OwnerId::new().unwrap())
            .is_err());
        assert!(arena.get(object).is_some());
        arena.set_position(object, local(Point::new(8, 8)));
        arena.recycle_group(object, owner).unwrap();
        let replacement = arena.create(catalog, prototype).unwrap();
        assert_eq!(replacement.index(), object.index());
        assert!(arena.claim_group(object, owner).is_err());
        arena.publish(replacement); // Even wholly clipped insertion publishes ownership.
        assert!(arena.claim_group(replacement, owner).is_err());
        assert!(arena.recycle_unplaced(replacement).is_err());
    });
}

#[test]
fn resolved_object_views_distinguish_unpositioned_retained_and_stale_records() {
    with_catalog(raw::RESOURCE, |catalog| {
        let prototype = catalog.at(ObjectKind::RESOURCE, 0).unwrap().id();
        let mut objects = ObjectArena::default();
        assert!(matches!(
            objects.create_shipyard(catalog, prototype),
            Err(PlacementError::PayloadKind {
                expected: ObjectKind::SHIPYARD,
                actual: ObjectKind::RESOURCE
            })
        ));
        assert!(objects.owner().is_none());
        let id = objects.create_resource(catalog, prototype).unwrap();
        assert!(objects.resolve(id).unwrap().positioned().is_none());
        assert!(matches!(
            objects.positioned(id),
            Err(PlacementError::UnpositionedObject(_))
        ));
        let position = local(Point::new(4, 5));
        objects.set_position(id, position);
        let object = objects.positioned(id).unwrap();
        assert_eq!(object.position(), position);
        assert_eq!(object.entrance(catalog).unwrap(), position);
        with_catalog(raw::RESOURCE, |foreign| {
            assert!(matches!(
                object.entrance(foreign),
                Err(PlacementError::UnknownPrototype(_))
            ));
        });
        assert_eq!(object.prototype(), prototype);
        assert!(matches!(
            object.payload(),
            crate::placement::ObjectPayload::Resource
        ));
        objects.retire(id, true).unwrap();
        assert_eq!(objects.positioned(id).unwrap().position(), position);
        objects.retire(id, false).unwrap();
        assert!(objects.resolve(id).is_none());
        let replacement = objects.create(catalog, prototype).unwrap();
        assert!(objects.resolve(replacement).is_some());
        assert!(objects.resolve(id).is_none());
        let mut other = ObjectArena::default();
        let foreign = other.create(catalog, prototype).unwrap();
        assert!(objects.resolve(foreign).is_none());
        assert!(other.resolve(replacement).is_none());
        objects.discard_unplaced(replacement).unwrap();
        let reused = objects.create(catalog, prototype).unwrap();
        assert_eq!(reused.index(), replacement.index());
        assert!(objects.resolve(replacement).is_none());
        assert!(objects.resolve(reused).is_some());
    });
}
