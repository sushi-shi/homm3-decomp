//! Shared post-town placement checkpoint rendering.
use super::placement::{write_cells, write_counts};
use homm3_rmg::{
    domain::{Level, WorldPosition},
    geometry::Point,
    placement::{Fort, ObjectArena, ObjectPayload, TownsPlaced},
    prototype::PrototypeCatalog,
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
    towns: &TownsPlaced<'_>,
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
    writeln!(actual, "{}", towns.towns(objects).unwrap().count()).unwrap();
    for (object, town) in towns.towns(objects).unwrap() {
        let geometry = objects.get(object).unwrap();
        let anchor = geometry.position().unwrap();
        let entrance = objects
            .positioned(object)
            .unwrap()
            .entrance(catalog)
            .unwrap();
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
    write_counts(&mut actual, map);
    write_cells(&mut actual, map, |object| {
        match objects.payload(object).unwrap() {
            ObjectPayload::Town(town) => town.id().value(),
            _ => panic!("town-only checkpoint contains another payload"),
        }
    });
    actual
}
