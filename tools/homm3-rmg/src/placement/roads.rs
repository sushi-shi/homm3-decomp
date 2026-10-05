//! Roads follow retained predecessor trees, rebuilding when painted costs change.
use super::{
    lines::Layer, offset_position, Direction, Movement, ObjectArena, ObstaclesPlaced,
    PlacementError, PlacementMap, PortalDirection, RoadType, TreasureGeneration,
};
use crate::{
    domain::{Level, Terrain, WorldPosition},
    object::ObjectKind,
    prototype::PrototypeCatalog,
    raw,
    rng::RetailRng,
};
use std::num::NonZeroU32;

/// One road type joins the stored town and shipyard targets.
pub(crate) struct RoadsCreated<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards> {
    obstacles: ObstaclesPlaced<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>,
}
impl<'state, 'zones, 'tiles> RoadsCreated<'state, 'zones, 'tiles, '_, '_, '_, '_> {
    pub(super) fn map_mut(&mut self) -> &mut PlacementMap<'state, 'zones, 'tiles> {
        self.obstacles.map_mut()
    }
    pub(super) const fn generation(&self) -> &TreasureGeneration<'_, '_, '_, '_, '_, '_, '_> {
        self.obstacles.generation()
    }
}
impl<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>
    ObstaclesPlaced<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>
{
    /// Draw the road type and build roads once, over the decorated map.
    ///
    /// # Errors
    /// Reports the retail empty-target underflow after its road-type draw,
    /// missing route predecessors, foreign objects/catalogs or placement faults.
    pub(crate) fn create_roads(
        mut self,
        catalog: &PrototypeCatalog<'_>,
        objects: &ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<
        RoadsCreated<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>,
        PlacementError,
    > {
        self.map_mut().create_roads(catalog, objects, rng)?;
        Ok(RoadsCreated { obstacles: self })
    }
}

impl PlacementMap<'_, '_, '_> {
    pub(super) fn reset_movement(&mut self) {
        for cell in self.cells.iter_mut() {
            cell.movement = Movement::Unreached;
        }
        self.connections.flood.clear();
    }
    pub(super) fn seed_movement(&mut self, source: WorldPosition) -> Result<usize, PlacementError> {
        self.connections.flood.seed(source)?;
        let index = self.view().native_index(source)?;
        self.cells[index].movement = Movement::Seed;
        Ok(index)
    }
    pub(super) fn queue_movement(
        &mut self,
        next: WorldPosition,
        cost: u32,
        previous: WorldPosition,
    ) -> Result<(), PlacementError> {
        let index = self.view().native_index(next)?;
        self.cells[index].movement = Movement::arrived(cost, previous)?;
        self.connections.flood.insert(
            next,
            i32::try_from(cost).map_err(|_| PlacementError::Arithmetic)?,
        )?;
        Ok(())
    }
    fn relax_road(
        &mut self,
        next: WorldPosition,
        cost: u32,
        previous: WorldPosition,
    ) -> Result<(), PlacementError> {
        let index = self.view().native_index(next)?;
        if u32::from(self.cells[index].movement.cost()) > cost {
            self.queue_movement(next, cost, previous)?;
        }
        Ok(())
    }
    fn build_road_costs(
        &mut self,
        source: WorldPosition,
        catalog: &PrototypeCatalog<'_>,
        objects: &ObjectArena,
    ) -> Result<(), PlacementError> {
        self.reset_movement();
        self.seed_movement(source)?;
        while let Some(position) = self.connections.flood.pop() {
            let index = self.view().native_index(position)?;
            let current = self.cells[index];
            let cost = u32::from(current.movement.cost());
            let mut directions = Direction::ALL.len();
            if let Some(kind) = current.entrance {
                let traits = kind.traits();
                if !traits.enterable_from_north() && !traits.cleared_on_visit() {
                    directions = Direction::NorthWest.index();
                }
                match kind {
                    ObjectKind::LITH_ONEWAY_ENTRANCE
                    | ObjectKind::LITH_ONEWAY_EXIT
                    | ObjectKind::LITH_TWOWAY => {
                        let object = self
                            .memberships
                            .first(current.objects)
                            .expect("entrance has a first object");
                        let geometry = objects
                            .get(object)
                            .ok_or(PlacementError::UnknownObject(object))?;
                        let prototype = catalog
                            .get(geometry.prototype())
                            .ok_or(PlacementError::UnknownPrototype(geometry.prototype()))?;
                        let direction = if kind == ObjectKind::LITH_TWOWAY {
                            PortalDirection::TwoWay
                        } else {
                            PortalDirection::OneWay
                        };
                        for ordinal in 0..self.portals(direction).len() {
                            let destination = self.portals(direction)[ordinal];
                            let geometry = objects.positioned(destination)?;
                            let entry = catalog
                                .get(geometry.prototype())
                                .ok_or(PlacementError::UnknownPrototype(geometry.prototype()))?;
                            if entry.prototype().subtype() == prototype.prototype().subtype() {
                                self.relax_road(
                                    geometry.position(),
                                    cost + raw::RMG_ROAD_MONOLITH_COST,
                                    position,
                                )?;
                            }
                        }
                    }
                    ObjectKind::UNDERGROUND_GATE => {
                        let level = match position.level {
                            Level::Surface => Level::Underground,
                            Level::Underground => Level::Surface,
                        };
                        self.relax_road(
                            WorldPosition { level, ..position },
                            cost + raw::RMG_ROAD_GATE_COST,
                            position,
                        )?;
                    }
                    _ => {}
                }
            }
            for &direction in Direction::ALL[..directions].iter().rev() {
                let next = offset_position(position, direction.offset())?;
                if !self.view().contains(next.point) {
                    continue;
                }
                let index = self.view().index(next)?;
                if self.terrain.tiles()[index].terrain() == Terrain::Water
                    || !self.view().passable(index)
                {
                    continue;
                }
                if let Some(kind) = self.cells[index].entrance {
                    let traits = kind.traits();
                    if (traits.blocks_landing() && !traits.cleared_on_visit())
                        || (!traits.enterable_from_north()
                            && !traits.cleared_on_visit()
                            && direction.southward())
                    {
                        continue;
                    }
                }
                let step =
                    if current.road.kind().is_some() && self.cells[index].road.kind().is_some() {
                        raw::RMG_ROAD_ALONG_ROAD_COST
                    } else {
                        raw::RMG_ROAD_OFF_ROAD_COST
                    };
                let step = if direction.index() % 2 != 0 {
                    step * raw::RMG_ROAD_DIAGONAL_FACTOR
                } else {
                    step
                };
                self.relax_road(next, cost + step, position)?;
            }
        }
        Ok(())
    }
    fn paint_road(
        &mut self,
        mut position: WorldPosition,
        kind: RoadType,
        rng: &mut RetailRng,
    ) -> Result<bool, PlacementError> {
        let mut painted = false;
        loop {
            let level = position.level;
            let mut previous = position;
            let mut index = self.view().native_index(position)?;
            while self.cells[index].road.kind() == Some(kind) {
                let next = match self.cells[index].movement {
                    Movement::Seed | Movement::Arrived { cost: 0, .. } => return Ok(painted),
                    Movement::Arrived { previous, .. } => previous,
                    Movement::Initial | Movement::Unreached => {
                        return Err(PlacementError::MissingPredecessor(position));
                    }
                };
                previous = position;
                position = next;
                index = self.view().native_index(position)?;
            }
            if position.level == level {
                position = previous;
                // Paint on the level where this run began. Skipping existing
                // roads can cross portals and return to this level while
                // `previous` still points to the other level.
                self.paint_line_point(Layer::Road(kind), WorldPosition { level, ..position }, rng)?;
                painted = true;
                loop {
                    let index = self.view().native_index(position)?;
                    position = match self.cells[index].movement {
                        Movement::Seed | Movement::Arrived { cost: 0, .. } => return Ok(painted),
                        Movement::Arrived { previous, .. } => previous,
                        Movement::Initial | Movement::Unreached => {
                            return Err(PlacementError::MissingPredecessor(position));
                        }
                    };
                    if position.level != level
                        || (position.point.x != previous.point.x
                            && position.point.y != previous.point.y)
                    {
                        break;
                    }
                    self.draw_line(
                        Layer::Road(kind),
                        WorldPosition { level, ..previous },
                        position,
                        rng,
                    )?;
                    previous = position;
                }
            }
        }
    }

    /// Choose one road type and join stored town/shipyard targets in source order.
    /// New roads lower search costs before subsequent destinations are considered.
    ///
    /// # Errors
    /// Reports the retail empty-target underflow after its road-type draw,
    /// missing route predecessors, foreign objects/catalogs or placement faults.
    // Cannot panic: the source-derived road domain and selected catalog
    // entries are bounded before lookup.
    fn create_roads(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        objects: &ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<(), PlacementError> {
        self.prepare_object_context(objects, catalog)?;
        let value =
            rng.below(NonZeroU32::new(raw::RMG_ROAD_TYPE_COUNT).unwrap()) + raw::RMG_ROAD_DIRT;
        let kind = match value {
            raw::RMG_ROAD_DIRT => RoadType::Dirt,
            raw::RMG_ROAD_GRAVEL => RoadType::Gravel,
            raw::RMG_ROAD_COBBLESTONE => RoadType::Cobblestone,
            _ => unreachable!("canonical road type range"),
        };
        if self.road_targets.is_empty() && !catalog.behavior().is_hotfix() {
            return Err(PlacementError::EmptyRoadTargets);
        }
        for first in 0..self.road_targets.len().saturating_sub(1) {
            let source = self.road_targets[first];
            self.build_road_costs(source, catalog, objects)?;
            for second in first + 1..self.road_targets.len() {
                let destination = self.road_targets[second];
                let index = self.view().native_index(destination)?;
                if u32::from(self.cells[index].movement.cost()) <= raw::RMG_REACHED_COST_LIMIT
                    && self.paint_road(destination, kind, rng)?
                    && second + 1 < self.road_targets.len()
                {
                    self.build_road_costs(source, catalog, objects)?;
                }
            }
        }
        Ok(())
    }
}
