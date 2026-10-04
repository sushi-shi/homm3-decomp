//! Shared terrain and native connection metadata rendering.
use homm3_rmg::{
    domain::{Level, WorldPosition},
    geometry::Point,
    placement::PlacementMap,
};
use std::fmt::Write;

pub fn write_terrain_and_distances(actual: &mut String, map: &PlacementMap<'_, '_, '_>) {
    let side = map.coverage().map().raster().dimension();
    for (index, tile) in map.terrain().tiles().iter().enumerate() {
        let at = WorldPosition {
            point: Point::new(
                i32::try_from(index % side).unwrap(),
                i32::try_from(index / side % side).unwrap(),
            ),
            level: if index < side * side {
                Level::Surface
            } else {
                Level::Underground
            },
        };
        let distance = map.cell(at).unwrap().zone_distance();
        let (zone, _) = distance.connection().map_or((-1, 0), |(zone, direction)| {
            (i32::try_from(zone.index()).unwrap(), direction.index())
        });
        writeln!(
            actual,
            "{} {} {} {} {} {} {}",
            tile.terrain() as i32,
            tile.frame(),
            u8::from(tile.reflection().flip_x),
            u8::from(tile.reflection().flip_y),
            distance.cost(),
            zone,
            distance.direction().index()
        )
        .unwrap();
    }
}
