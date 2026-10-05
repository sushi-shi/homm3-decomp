//! River targets, randomized cardinal searches and ordered delta placement.
use super::{
    lines::Layer, offset_position, registration::entrance_position, Direction, Movement,
    ObjectArena, PlacementError, PlacementMap, RiverType, RoadsCreated, TreasureGeneration,
};
use crate::{
    domain::{Level, Terrain, WorldPosition},
    geometry::Point,
    object::ObjectKind,
    prototype::PrototypeCatalog,
    raw,
    rng::RetailRng,
};

#[derive(Clone, Copy, PartialEq, Eq)]
enum RiverGoal {
    /// Join an existing river or a marked mountain, lake or gem-mine cell.
    Join,
    /// Reach a coast, map edge or route already connected to an outlet.
    Outlet,
}

// Bit k bars river entry through side k. North is up:
//     3
//   2 . 0
//     1
// Moving toward a cell enters through the opposite side.
fn opposite_bit(direction: Direction) -> u8 {
    1 << (direction.opposite().index() / raw::RMG_CARDINAL_DIRECTION_STEP as usize)
}

/// Water-wheel rivers and deltas are placed: the final generation stage.
pub(crate) struct RiversCreated<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards> {
    roads: RoadsCreated<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>,
}
impl RiversCreated<'_, '_, '_, '_, '_, '_, '_> {
    /// Completed map and the payload context needed for serialization.
    pub(crate) const fn generation(&self) -> &TreasureGeneration<'_, '_, '_, '_, '_, '_, '_> {
        self.roads.generation()
    }
}
impl<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>
    RoadsCreated<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>
{
    /// Route rivers once, after roads have been painted.
    ///
    /// # Errors
    /// Reports native coast overreads, unwritten route predecessors, foreign
    /// context, invalid geometry, arithmetic or allocation faults.
    pub(crate) fn create_rivers(
        mut self,
        catalog: &PrototypeCatalog<'_>,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<
        RiversCreated<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>,
        PlacementError,
    > {
        self.map_mut().create_rivers(catalog, objects, rng)?;
        Ok(RiversCreated { roads: self })
    }
}

impl PlacementMap<'_, '_, '_> {
    fn coast_cell(&self, at: WorldPosition, hotfix: bool) -> Result<Option<usize>, PlacementError> {
        let side = self.view().signed_side();
        // Retail admits x == side, aliasing the next row or plane if allocated.
        if at.point.x < 0
            || at.point.x > side
            || (hotfix && at.point.x == side)
            || at.point.y < 0
            || at.point.y >= side
        {
            return Ok(None);
        }
        Ok(Some(self.view().native_index(at)?))
    }
    fn dry_coast_cell(
        &self,
        at: WorldPosition,
        hotfix: bool,
    ) -> Result<Option<usize>, PlacementError> {
        let Some(index) = self.coast_cell(at, hotfix)? else {
            return Ok(None);
        };
        Ok((self.terrain.tiles()[index].terrain() != Terrain::Water
            && self.cells[index].entrance.is_none())
        .then_some(index))
    }
    fn mark_coast_target(
        &mut self,
        at: WorldPosition,
        direction: Direction,
        hotfix: bool,
    ) -> Result<(), PlacementError> {
        let turn =
            |delta: usize| Direction::ALL[(direction.index() + delta) % Direction::ALL.len()];
        let step = turn(6).offset();
        // East-facing scan, north up: P/~ water, d dry strip, i inland, T target.
        //   ~ d
        //   P d i i T
        //   ~ d
        let mut point = offset_position(at, turn(2).offset())?;
        for _ in 0..3 {
            let Some(index) = self.coast_cell(point, hotfix)? else {
                return Ok(());
            };
            if self.terrain.tiles()[index].terrain() != Terrain::Water {
                return Ok(());
            }
            point = offset_position(point, step)?;
        }
        point = offset_position(at, turn(1).offset())?;
        for _ in 0..3 {
            if self.dry_coast_cell(point, hotfix)?.is_none() {
                return Ok(());
            }
            point = offset_position(point, step)?;
        }
        point = offset_position(at, direction.offset())?;
        let mut target = None;
        for _ in 0..4 {
            target = self.dry_coast_cell(point, hotfix)?;
            if target.is_none() {
                return Ok(());
            }
            point = offset_position(point, direction.offset())?;
        }
        let cell = &mut self.cells[target.expect("four admitted inland cells")];
        cell.blocked_river_directions |= opposite_bit(direction);
        cell.river_outlet_target = true;
        Ok(())
    }
    fn mark_river_outlet_targets(&mut self, hotfix: bool) -> Result<(), PlacementError> {
        let layout = self.view().layout;
        let (side, plane) = (layout.side(), layout.plane());
        for level in [Level::Surface, Level::Underground] {
            let start = level.index() * plane;
            if start >= self.cells.len() {
                break;
            }
            // Row-major: Y outer, X inner.
            for index in 0..plane {
                if self.terrain.tiles()[start + index].terrain() != Terrain::Water {
                    continue;
                }
                let at = WorldPosition {
                    point: layout.point(index),
                    level,
                };
                for direction in Direction::ALL
                    .into_iter()
                    .step_by(raw::RMG_CARDINAL_DIRECTION_STEP as usize)
                {
                    self.mark_coast_target(at, direction, hotfix)?;
                }
            }
        }
        for level in [Level::Surface, Level::Underground] {
            let start = level.index() * plane;
            if start >= self.cells.len() {
                break;
            }
            for y in 0..side {
                self.cells[start + y * side].river_outlet_target = true;
                self.cells[start + y * side + side - 1].river_outlet_target = true;
            }
            for x in 0..side {
                self.cells[start + x].river_outlet_target = true;
                self.cells[start + (side - 1) * side + x].river_outlet_target = true;
            }
        }
        Ok(())
    }
    fn mark_river_objects(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        objects: &ObjectArena,
    ) -> Result<(), PlacementError> {
        for ordinal in 0..self.active_objects().len() {
            let id = self.active_objects()[ordinal];
            let object = objects.positioned(id)?;
            let entry = catalog
                .get(object.prototype())
                .ok_or(PlacementError::UnknownPrototype(object.prototype()))?;
            let prototype = entry.prototype();
            let kind = prototype.kind().index();
            if kind != raw::TERRAIN_MOUNTAIN as usize
                && kind != raw::TERRAIN_LAKE as usize
                && !(kind == raw::MINE as usize && prototype.subtype() == raw::GEMS)
            {
                continue;
            }
            let anchor = object.position();
            let at = if prototype.entrance().is_some() {
                entrance_position(prototype, anchor)?
            } else {
                let size = entry.image_mask().size()?;
                offset_position(
                    anchor,
                    Point::new(-i32::from(size.width() / 2), -i32::from(size.height() / 2)),
                )?
            };
            if self.view().contains(at.point) {
                let index = self.view().index(at)?;
                self.cells[index].river_join_target = true;
            }
        }
        Ok(())
    }
    #[expect(
        clippy::too_many_lines,
        reason = "ordered seeds, edge draws, endpoint handling and painting follow one native route search"
    )]
    fn create_river(
        &mut self,
        source: WorldPosition,
        goal: RiverGoal,
        catalog: &PrototypeCatalog<'_>,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<(), PlacementError> {
        self.reset_movement();
        let mut inspected = self.seed_movement(source)?;
        let snow = self.terrain.tiles()[inspected].terrain() == Terrain::Snow;
        let kind = if snow {
            RiverType::Icy
        } else {
            RiverType::Clear
        };
        if goal == RiverGoal::Outlet {
            // Append seeds in native order; unlike relaxed ties they pop newest first.
            // North up: 1 and 2 are the extra seeds beside water-wheel cells.
            //   1 2
            //   S .
            self.seed_movement(offset_position(source, Point::new(0, -1))?)?;
            self.seed_movement(offset_position(source, Point::new(1, -1))?)?;
        }
        let mut next = None;
        while let Some(position) = self.connections.flood.pop() {
            inspected = self.view().native_index(position)?;
            let cost = u32::from(self.cells[inspected].movement.cost());
            for direction in Direction::ALL
                .into_iter()
                .step_by(raw::RMG_CARDINAL_DIRECTION_STEP as usize)
            {
                let at = offset_position(position, direction.offset())?;
                next = Some(at); // Assigned even when the bounds test rejects it.
                if !self.view().contains(at.point) {
                    continue;
                }
                inspected = self.view().index(at)?;
                let terrain = self.terrain.tiles()[inspected].terrain();
                if matches!(terrain, Terrain::Water | Terrain::Rock)
                    || (terrain == Terrain::Snow) != snow
                    || (goal == RiverGoal::Outlet && self.cells[inspected].near_river)
                {
                    continue;
                }
                // Draw even for an unimproved or subsequently blocked edge.
                let next_cost = cost
                    + (rng.draw() & crate::constants::RMG_RIVER_STEP_MASK)
                    + crate::constants::RMG_RIVER_MINIMUM_STEP_COST
                    + if self.cells[inspected].road.kind().is_some() {
                        crate::constants::RMG_RIVER_ROAD_PENALTY
                    } else {
                        0
                    };
                if next_cost >= u32::from(self.cells[inspected].movement.cost()) {
                    continue;
                }
                if goal == RiverGoal::Outlet
                    && self.cells[inspected].blocked_river_directions & opposite_bit(direction) != 0
                {
                    continue;
                }
                self.queue_movement(at, next_cost, position)?;
                if self.river_goal(inspected, goal) {
                    self.connections.flood.clear();
                    break;
                }
            }
        }
        // Retail tests the last inspected tile, independently of `next` and of
        // whether it was reached. Preserve the check; fault if its later route
        // needs an unwritten predecessor. Hotfix requires a reached target.
        if !self.river_goal(inspected, goal)
            || (catalog.behavior().is_hotfix()
                && u32::from(self.cells[inspected].movement.cost()) >= raw::RMG_UNREACHED_COST)
        {
            return Ok(());
        }
        let end = next.ok_or(PlacementError::MissingPredecessor(source))?;
        self.paint_line_point(Layer::River(kind), end, rng)?;
        let mut painted = end;
        if goal == RiverGoal::Outlet && self.cells[inspected].blocked_river_directions != 0 {
            let cardinal = self.cells[inspected]
                .blocked_river_directions
                .trailing_zeros() as usize;
            let ordinal = if snow {
                raw::RIVER_DELTA_SNOW[cardinal]
            } else {
                raw::RIVER_DELTA_LAND[cardinal]
            } as usize;
            let terrain = self.terrain.tiles()[inspected].terrain();
            let Some(prototype) = catalog
                .members(ObjectKind::TERRAIN_RIVER_DELTA)
                .filter(|member| member.entry().prototype().recommends(terrain))
                .nth(ordinal)
            else {
                return Ok(());
            };
            let id = objects.create(catalog, prototype.id())?;
            let (x, y) = raw::RIVER_DELTA_OFFSETS[cardinal];
            self.register_object(
                objects,
                catalog,
                id,
                offset_position(end, Point::new(x, y))?,
            )?;
            let mouth = offset_position(
                end,
                Direction::ALL[cardinal * raw::RMG_CARDINAL_DIRECTION_STEP as usize].offset(),
            )?;
            self.draw_line(Layer::River(kind), painted, mouth, rng)?;
            let index = self.view().native_index(mouth)?;
            self.cells[index].river_outlet_target = true;
            self.draw_line(Layer::River(kind), mouth, end, rng)?;
            painted = end;
            inspected = self.view().native_index(end)?;
        }
        loop {
            let previous = match self.cells[inspected].movement {
                Movement::Seed | Movement::Arrived { cost: 0, .. } => break,
                Movement::Arrived { previous, .. } => previous,
                Movement::Initial | Movement::Unreached => {
                    return Err(PlacementError::MissingPredecessor(painted));
                }
            };
            inspected = self.view().native_index(previous)?;
            if goal == RiverGoal::Outlet {
                self.cells[inspected].river_outlet_target = true;
            }
            self.draw_line(Layer::River(kind), painted, previous, rng)?;
            painted = previous;
        }
        Ok(())
    }
    fn river_goal(&self, index: usize, goal: RiverGoal) -> bool {
        match goal {
            RiverGoal::Join => self.cells[index].river_join_target,
            RiverGoal::Outlet => self.cells[index].river_outlet_target,
        }
    }
    /// Mark natural targets and coasts, then route rivers for each water wheel.
    /// Newly appended deltas retain native object order; buffers are reused.
    ///
    /// # Errors
    /// Reports native coast overreads, unwritten route predecessors, foreign
    /// context, invalid geometry, arithmetic or allocation faults.
    fn create_rivers(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<(), PlacementError> {
        self.prepare_object_context(objects, catalog)?;
        self.mark_river_objects(catalog, objects)?;
        self.mark_river_outlet_targets(catalog.behavior().is_hotfix())?;
        let mut index = 0;
        while index < self.active_objects().len() {
            let id = self.active_objects()[index];
            let object = objects.positioned(id)?;
            let entry = catalog
                .get(object.prototype())
                .ok_or(PlacementError::UnknownPrototype(object.prototype()))?;
            if entry.prototype().kind().index() == raw::WATER_WHEEL as usize {
                let entrance = entrance_position(entry.prototype(), object.position())?;
                self.create_river(entrance, RiverGoal::Join, catalog, objects, rng)?;
                self.create_river(
                    offset_position(entrance, Point::new(-2, 0))?,
                    RiverGoal::Outlet,
                    catalog,
                    objects,
                    rng,
                )?;
            }
            index += 1;
        }
        Ok(())
    }
}
