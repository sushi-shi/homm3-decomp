//! Shared placement counters and ordered per-cell state rendering.
use homm3_rmg::{
    domain::{Level, WorldPosition},
    geometry::Point,
    object::ObjectKind,
    placement::{ObjectId, PathReservation, PlacementMap},
    raw,
};
use std::fmt::Write;
fn position(x: i32, y: i32, level: Level) -> WorldPosition {
    WorldPosition {
        point: Point::new(x, y),
        level,
    }
}
pub fn write_counts(actual: &mut String, map: &PlacementMap<'_>) {
    let kind = |k| ObjectKind::parse(i32::try_from(k).unwrap()).unwrap();
    for k in 0..raw::ADVENTURE_OBJECT_TRAIT_COUNT {
        write!(actual, "{} ", map.object_count(kind(k))).unwrap();
    }
    actual.push('\n');
    for zone in map.coverage().map().zones() {
        for k in 0..raw::ADVENTURE_OBJECT_TRAIT_COUNT {
            write!(
                actual,
                "{} ",
                map.zone_object_count(zone.id(), kind(k)).unwrap()
            )
            .unwrap();
        }
        actual.push('\n');
    }
}
pub fn write_cells(
    actual: &mut String,
    map: &PlacementMap<'_>,
    object_id: impl Fn(ObjectId) -> i32,
) {
    let side = i32::try_from(map.coverage().map().raster().dimension()).unwrap();
    for level in [Level::Surface, Level::Underground]
        .into_iter()
        .take(map.coverage().map().request().levels().count() as usize)
    {
        for y in 0..side {
            for x in 0..side {
                let at = position(x, y, level);
                let cell = map.cell(at).unwrap().state();
                write!(
                    actual,
                    "{} {} {} {} {}",
                    u8::from(cell.passable()),
                    cell.entrance()
                        .map_or(-1, |kind| i32::try_from(kind.index()).unwrap()),
                    u8::from(cell.reservation() == PathReservation::Obstacle),
                    u8::from(cell.reservation() == PathReservation::Open),
                    cell.object_distance()
                )
                .unwrap();
                for object in map.objects_at(at).unwrap() {
                    write!(actual, " {}", object_id(object)).unwrap();
                }
                actual.push('\n');
            }
        }
    }
}
