//! World fitting and highest-distance selection for an assembled local group.
use super::{
    offset_position, treasure_groups::local, Direction, NonEmptyOutline, ObjectArena,
    ObstacleEntrances, OutlineClearance, OutlineEntrances, PlacementError, TreasureGeneration,
    TreasureGenerationError, TreasureGroupWorkspace,
};
use crate::{
    domain::{Terrain, WorldPosition},
    geometry::{Point, ZoneId},
    raw,
    rng::RetailRng,
};

impl TreasureGeneration<'_, '_, '_, '_, '_, '_, '_> {
    fn group_fits(
        &self,
        group: &TreasureGroupWorkspace,
        origin: WorldPosition,
        zone: ZoneId,
        objects: &ObjectArena,
    ) -> Result<bool, PlacementError> {
        let map = self.ready.map();
        let catalog = self.ready.catalog.prototypes();
        let view = map.view();
        for id in group.objects() {
            let geometry = objects.get(id).ok_or(PlacementError::UnknownObject(id))?;
            let position = geometry
                .position()
                .ok_or(PlacementError::UnknownObject(id))?;
            let entry = catalog
                .get(geometry.prototype())
                .ok_or(PlacementError::UnknownPrototype(geometry.prototype()))?;
            if map.footprint_blocked(
                entry,
                offset_position(origin, position.point)?,
                Some(zone),
                ObstacleEntrances::Reject,
            )? {
                return Ok(false);
            }
        }
        if let Some(entrance) = group.guard_entrance {
            let guard = offset_position(origin, entrance)?;
            let side = i32::try_from(view.side).map_err(|_| PlacementError::Arithmetic)?;
            if guard.point.x < 1
                || guard.point.x >= side - 1
                || guard.point.y < 1
                || guard.point.y >= side - 1
            {
                return Ok(false);
            }
            for x in guard.point.x - 1..=guard.point.x + 1 {
                for y in guard.point.y - 1..=guard.point.y + 1 {
                    let index = view.native_index(WorldPosition {
                        point: Point::new(x, y),
                        level: origin.level,
                    })?;
                    if map.cells[index]
                        .entrance
                        .is_some_and(|kind| kind.index() == raw::MONSTER as usize)
                    {
                        return Ok(false);
                    }
                }
            }
        }
        let last = group
            .objects
            .last()
            .ok_or(PlacementError::EmptyOutline)?
            .object();
        let last_geometry = objects
            .get(last)
            .ok_or(PlacementError::UnknownObject(last))?;
        let entrance = objects
            .entrance(catalog, last)?
            .ok_or(PlacementError::UnknownObject(last))?;
        let directions = if last_geometry.kind().traits().enterable_from_north() {
            &Direction::ALL[..]
        } else {
            &Direction::ALL[1..4]
        };
        let water = map.zone(zone)?.terrain() == Terrain::Water;
        let mut approach = false;
        for &direction in directions {
            let point = offset_position(entrance, direction.offset())?.point;
            let index = group.view().native_index(local(point))?;
            if !group.clear_outline_cell(index) || !group.outline_marks[index] {
                continue;
            }
            let target = offset_position(origin, point)?;
            if !view.contains(target.point) {
                continue;
            }
            let index = view.native_index(target)?;
            if (view.terrain(index) == Terrain::Water) == water && view.clear_outline_cell(index) {
                approach = true;
                break;
            }
        }
        if !approach {
            return Ok(false);
        }
        let mut allow_entrances = group.guard_entrance.is_none();
        for id in group.objects() {
            allow_entrances &= objects
                .get(id)
                .ok_or(PlacementError::UnknownObject(id))?
                .kind()
                .traits()
                .cleared_on_visit();
        }
        let entrances = if allow_entrances {
            OutlineEntrances::Allow
        } else {
            OutlineEntrances::Reject
        };
        let outline =
            NonEmptyOutline::parse(group.outline()).ok_or(PlacementError::EmptyOutline)?;
        if !map.has_connected_outline(
            outline,
            origin,
            entrances,
            zone,
            OutlineClearance::Require,
        )? {
            return Ok(false);
        }
        let bounds = group.bounds().ok_or(PlacementError::EmptyOutline)?;
        for y in bounds.minimum().y..bounds.maximum().y {
            for x in bounds.minimum().x..bounds.maximum().x {
                let point = Point::new(x, y);
                if group
                    .cell(point)?
                    .reservation()
                    .eq(&super::PathReservation::Open)
                {
                    continue;
                }
                let target = offset_position(origin, point)?;
                // Source checks only upper XY bounds at this point; preserve flat
                // aliases when either coordinate is negative.
                let side = i32::try_from(view.side).map_err(|_| PlacementError::Arithmetic)?;
                if target.point.x < side
                    && target.point.y < side
                    && map.cells[view.native_index(target)?].entrance.is_some()
                {
                    return Ok(false);
                }
            }
        }
        Ok(true)
    }
    /// Find the group's world origin without publishing objects or completing
    /// quests. Keeps every highest-score tie and draws once when a site exists.
    ///
    /// A candidate is where local O lands; local cell g maps to origin + g.
    /// The occupied bounds (#) fit the zone, and distance under c sets spacing.
    /// North is up:
    /// ```text
    /// O . . . . .
    /// . . # # # .
    /// . . # c # .
    /// . . # # # .
    /// ```
    ///
    /// # Errors
    /// Reports missing group bounds, stale objects or an unsafe native access.
    pub fn find_group_site(
        &self,
        group: &mut TreasureGroupWorkspace,
        zone: ZoneId,
        spacing: i32,
        objects: &ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<Option<WorldPosition>, TreasureGenerationError> {
        self.check_group(group, objects)?;
        let zone = self.ready.map().zone(zone)?;
        let Some(bounds) = zone.bounds() else {
            return Ok(None);
        };
        let group_bounds = group.bounds().ok_or(PlacementError::EmptyOutline)?;
        let min = bounds
            .minimum()
            .checked_add(Point::new(
                -group_bounds.minimum().x,
                -group_bounds.minimum().y,
            ))
            .ok_or(PlacementError::CoordinateOverflow)?;
        let max = bounds
            .maximum()
            .checked_add(Point::new(
                1 - group_bounds.maximum().x,
                1 - group_bounds.maximum().y,
            ))
            .ok_or(PlacementError::CoordinateOverflow)?;
        let center = Point::new(
            (group_bounds.minimum().x + group_bounds.maximum().x) / 2,
            (group_bounds.minimum().y + group_bounds.maximum().y) / 2,
        );
        group.candidates.clear();
        let mut best = spacing;
        let view = self.ready.map().view();
        for y in min.y..max.y {
            for x in min.x..max.x {
                let origin = WorldPosition {
                    point: Point::new(x, y),
                    level: zone.position().level,
                };
                let index = view.native_index(offset_position(origin, center)?)?;
                let score = i32::from(view.cells[index].object_distance);
                if view.zone(index) == Some(zone.id())
                    && score >= best
                    && self.group_fits(group, origin, zone.id(), objects)?
                {
                    if score > best {
                        group.candidates.clear();
                        best = score;
                    }
                    group
                        .candidates
                        .try_reserve(1)
                        .map_err(PlacementError::from)?;
                    group.candidates.push(origin.point);
                }
            }
        }
        if group.candidates.is_empty() {
            return Ok(None);
        }
        let point = group.candidates[rng.draw() as usize % group.candidates.len()];
        Ok(Some(WorldPosition {
            point,
            level: zone.position().level,
        }))
    }
}
