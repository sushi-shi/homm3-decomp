//! Shared post-town placement checkpoint rendering.
use homm3_rmg::{
    domain::{Level, WorldPosition},
    geometry::Point,
    object::ObjectKind,
    placement::{Fort, ObjectArena, PathReservation, TownsPlaced},
    prototype::PrototypeCatalog,
    raw,
    rng::RngCheckpoint,
};
use std::fmt::Write;

fn position(x: i32, y: i32, level: Level) -> WorldPosition {
    WorldPosition {
        point: Point::new(x, y),
        level,
    }
}
pub fn snapshot(
    towns: &TownsPlaced<'_, '_, '_>,
    objects: &ObjectArena,
    catalog: &PrototypeCatalog<'_>,
    checkpoint: RngCheckpoint,
) -> String {
    let mut actual = String::new();
    let map = towns.map();
    writeln!(
        actual,
        "{} {} {}",
        checkpoint.state,
        map.next_object_id(),
        if map.coverage().map().behavior().is_hotfix() {
            1
        } else {
            -1
        }
    )
    .unwrap();
    writeln!(actual, "{}", map.coverage().map().zones().len()).unwrap();
    for zone in map.coverage().map().zones() {
        let entrance = zone
            .primary_town()
            .unwrap_or(position(0, 0, Level::Surface));
        writeln!(
            actual,
            "{} {} {} {} {}",
            zone.alignment()
                .map_or(-1, |town| i32::try_from(town.index()).unwrap()),
            u8::from(zone.primary_town().is_some()),
            entrance.point.x,
            entrance.point.y,
            entrance.level.index()
        )
        .unwrap();
    }
    writeln!(actual, "{}", towns.towns().len()).unwrap();
    for town in towns.towns() {
        let geometry = objects.get(town.object()).unwrap();
        let anchor = geometry.position().unwrap();
        let entrance = town.entrance();
        writeln!(
            actual,
            "{} {} {} {} {} {} {} {} {} {}",
            catalog
                .get(geometry.prototype())
                .unwrap()
                .prototype()
                .source_row(),
            town.id().value(),
            town.owner()
                .map_or(-1, |owner| i32::try_from(owner.index()).unwrap()),
            u8::from(town.fort() == Fort::Present),
            anchor.point.x,
            anchor.point.y,
            anchor.level.index(),
            entrance.point.x,
            entrance.point.y,
            entrance.level.index()
        )
        .unwrap();
    }
    writeln!(actual, "{}", towns.road_targets().len()).unwrap();
    for entrance in towns.road_targets() {
        writeln!(
            actual,
            "{} {} {}",
            entrance.point.x,
            entrance.point.y,
            entrance.level.index()
        )
        .unwrap();
    }
    write_counts(&mut actual, towns);
    write_cells(&mut actual, towns);
    actual
}
fn write_counts(actual: &mut String, towns: &TownsPlaced<'_, '_, '_>) {
    let map = towns.map();
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
fn write_cells(actual: &mut String, towns: &TownsPlaced<'_, '_, '_>) {
    let map = towns.map();
    let side = i32::try_from(map.coverage().map().raster().dimension()).unwrap();
    for level in [Level::Surface, Level::Underground]
        .into_iter()
        .take(map.coverage().map().request().levels().count() as usize)
    {
        for y in 0..side {
            for x in 0..side {
                let at = position(x, y, level);
                let cell = map.cell(at).unwrap();
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
                    write!(
                        actual,
                        " {}",
                        towns
                            .towns()
                            .iter()
                            .find(|town| town.object() == object)
                            .unwrap()
                            .id()
                            .value()
                    )
                    .unwrap();
                }
                actual.push('\n');
            }
        }
    }
}
