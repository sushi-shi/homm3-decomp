//! Fixed and density-driven mines, their guards and adjacent resource piles.
use super::{
    density::Density, registration::entrance_position, GuardPlacementError, JunctionsPrepared,
    ObjectArena, ObjectId, PathReservation, PlacementError, PlacementMap, TownsPlaced,
};
use crate::{
    boundaries::{BoundaryZone, ZoneOrigin},
    domain::{Resource, WorldPosition},
    geometry::Point,
    object::ObjectKind,
    prototype::{OutlineWorkspace, PreparedPrototype, PrototypeCatalog, PrototypeId},
    raw,
    rng::{RetailRng, RngCheckpoint},
    template::ZoneRole,
    traits::CreatureCatalog,
};
use std::{error::Error, fmt, num::NonZeroU32};

/// Mine placement or subsequent guard/resource placement failed.
#[derive(Debug)]
pub enum MineError {
    /// Invalid context, geometry, arithmetic, cell access or allocation.
    Placement(PlacementError),
    /// Guard selection or registration failed after the mine was placed.
    Guard(GuardPlacementError),
}
impl fmt::Display for MineError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Placement(error) => error.fmt(f),
            Self::Guard(error) => error.fmt(f),
        }
    }
}
impl Error for MineError {}
impl From<PlacementError> for MineError {
    fn from(error: PlacementError) -> Self {
        Self::Placement(error)
    }
}
impl From<GuardPlacementError> for MineError {
    fn from(error: GuardPlacementError) -> Self {
        Self::Guard(error)
    }
}

/// Fixed and density-driven mine attempts, guards and resource piles are complete.
pub struct MinesPlaced<'state, 'zones, 'tiles> {
    junctions: JunctionsPrepared<'state, 'zones, 'tiles>,
    rng: RngCheckpoint,
}
impl<'state, 'zones, 'tiles> MinesPlaced<'state, 'zones, 'tiles> {
    pub(super) fn map_mut(&mut self) -> &mut PlacementMap<'state, 'zones, 'tiles> {
        self.junctions.map_mut()
    }
    /// Shared placement state after the mine pass.
    #[must_use]
    pub const fn map(&self) -> &PlacementMap<'_, '_, '_> {
        self.junctions.map()
    }
    /// Town records used by the following faction-count pass.
    #[must_use]
    pub const fn towns(&self) -> &TownsPlaced<'_, '_, '_> {
        self.junctions.towns()
    }
    /// RNG after the final mine attempt.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng
    }
}
impl<'state, 'zones, 'tiles> JunctionsPrepared<'state, 'zones, 'tiles> {
    /// Place fixed mine counts followed by density-driven extras in each zone.
    ///
    /// # Errors
    /// Reports context, geometry, arithmetic, allocation, placement or guard faults.
    pub fn place_mines(
        mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<MinesPlaced<'state, 'zones, 'tiles>, MineError> {
        self.map_mut().prepare_object_context(objects, catalog)?;
        for index in 0..self.map().coverage().map().zones().len() {
            let zone = self.map().coverage().map().zones()[index];
            let ZoneOrigin::Template(id) = zone.origin() else {
                // Added water zones have zero mine counts and densities.
                continue;
            };
            let rules = &self.map().coverage().map().template().zones()[id.index()];
            let placements = *rules.mines();
            let starting_town =
                if matches!(rules.role(), ZoneRole::Human(_) | ZoneRole::Computer(_)) {
                    zone.primary_town()
                } else {
                    None
                };
            for resource in Resource::ALL {
                let mut site = match (resource, starting_town) {
                    (Resource::Wood | Resource::Ore, Some(town)) => MineSite::Starting { town },
                    _ => MineSite::Ordinary { spacing: 0 },
                };
                for _ in 0..placements[resource.index()].initial_count {
                    if !self
                        .map_mut()
                        .try_place_mine(zone, resource, site, objects, catalog, creatures, rng)?
                    {
                        break;
                    }
                    site = MineSite::Ordinary { spacing: 0 };
                }
            }
            let mut density = Density::new(&placements)?;
            let area = i32::try_from(raw::RMG_TOWN_AND_MINE_DENSITY_AREA)
                .map_err(|_| PlacementError::Arithmetic)?;
            if let Some(spacing) = density.spacing(area) {
                while let Some(index) = density.next()? {
                    if !self.map_mut().try_place_mine(
                        zone,
                        Resource::ALL[index],
                        MineSite::Ordinary { spacing },
                        objects,
                        catalog,
                        creatures,
                        rng,
                    )? {
                        density.finish(index);
                    }
                }
            }
        }
        Ok(MinesPlaced {
            junctions: self,
            rng: rng.checkpoint(),
        })
    }
}

// Starting placement cannot exist without its primary town entrance.
#[derive(Clone, Copy)]
enum MineSite {
    Starting { town: WorldPosition },
    Ordinary { spacing: i32 },
}

// Native uses last_scanned for the post-placement entrance and strip geometry,
// even when the randomly selected art differs. Both modes retain this behavior.
struct MinePrototype {
    selected: PrototypeId,
    last_scanned: PrototypeId,
}
fn kind(value: u32) -> ObjectKind {
    ObjectKind::parse(i32::try_from(value).expect("canonical kind fits i32"))
        .expect("canonical mine/resource kind is admitted")
}
impl MinePrototype {
    fn select(
        catalog: &PrototypeCatalog<'_>,
        resource: Resource,
        zone: BoundaryZone,
        rng: &mut RetailRng,
    ) -> Option<Self> {
        let family = kind(raw::MINE);
        let entries = catalog.family(family);
        let last_scanned = catalog.at(family, entries.len().checked_sub(1)?)?;
        let matching =
            |entry: &&PreparedPrototype<'_>| entry.prototype().subtype() == resource as i32;
        let recommended = entries
            .iter()
            .filter(matching)
            .filter(|entry| entry.prototype().recommends(zone.terrain()))
            .count();
        let mut candidates = entries.iter().enumerate().filter(|(_, entry)| {
            entry.prototype().subtype() == resource as i32
                && (recommended == 0 || entry.prototype().recommends(zone.terrain()))
        });
        let count = NonZeroU32::new(
            u32::try_from(candidates.clone().count()).expect("parsed family length fits i32"),
        )?;
        let (index, _) = candidates.nth(rng.below(count) as usize)?;
        Some(Self {
            selected: catalog.at(family, index)?,
            last_scanned,
        })
    }
}
impl PlacementMap<'_, '_, '_> {
    #[expect(
        clippy::too_many_arguments,
        reason = "mine attempt shares admitted catalogs, arena and RNG"
    )]
    fn try_place_mine(
        &mut self,
        zone: BoundaryZone,
        resource: Resource,
        site: MineSite,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<bool, MineError> {
        let Some(prototypes) = MinePrototype::select(catalog, resource, zone, rng) else {
            return Ok(false);
        };
        let object = objects.create_mine(catalog, prototypes.selected)?;
        let entry = catalog
            .get(prototypes.selected)
            .expect("selected from catalog");
        let Some(position) =
            self.place_mine_site(object, entry, zone, site, objects, catalog, rng)?
        else {
            objects.discard_unplaced(object)?;
            return Ok(false);
        };
        let base = match resource {
            Resource::Wood | Resource::Ore => raw::RMG_BASIC_MINE_GUARD_VALUE,
            Resource::Gold => raw::RMG_GOLD_MINE_GUARD_VALUE,
            _ => raw::RMG_RARE_MINE_GUARD_VALUE,
        };
        let value = self.zone_guard_value(
            i32::try_from(base).map_err(|_| PlacementError::Arithmetic)?,
            zone,
        )?;
        let last = catalog
            .get(prototypes.last_scanned)
            .expect("selected from catalog");
        let entrance = entrance_position(last.prototype(), position)?;
        let approach = self.open_entrance_approach(entrance)?;
        if value > 0 {
            self.place_guard(value, approach, objects, catalog, creatures, rng)?;
        }
        self.place_mine_resources(zone, resource, position, last, objects, catalog, rng)?;
        Ok(true)
    }

    #[expect(
        clippy::too_many_arguments,
        reason = "one native mine-site search owns one allocated object"
    )]
    fn place_mine_site(
        &mut self,
        object: ObjectId,
        entry: &PreparedPrototype<'_>,
        zone: BoundaryZone,
        site: MineSite,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<Option<WorldPosition>, PlacementError> {
        let mut best_obstacles = 0;
        let mut best_distance = i32::try_from(raw::RMG_STARTING_MINE_MAXIMUM_SQUARED_DISTANCE)
            .map_err(|_| PlacementError::Arithmetic)?;
        let mut best_score = match site {
            MineSite::Ordinary { spacing } => spacing,
            MineSite::Starting { .. } => 0,
        };
        let town_anchor = match site {
            MineSite::Starting { town } => {
                let trigger = super::registration::trigger_offset(entry.prototype());
                Some(
                    town.point
                        .checked_add(trigger)
                        .ok_or(PlacementError::CoordinateOverflow)?,
                )
            }
            MineSite::Ordinary { .. } => None,
        };
        let dimensions = entry.image_mask().size()?;
        // Trace even when no bounds/sites exist; this precedes native scanning.
        let mut outline = OutlineWorkspace::default();
        let outline = outline.trace(entry)?;
        self.zone_placement.candidates.clear();
        let Some(bounds) = zone.bounds() else {
            return Ok(None);
        };
        for y in bounds.minimum().y + i32::from(dimensions.height()) - 1..bounds.maximum().y {
            for x in bounds.minimum().x + i32::from(dimensions.width()) - 1..bounds.maximum().x {
                let position = WorldPosition {
                    point: Point::new(x, y),
                    level: zone.position().level,
                };
                let index = self.view().native_index(position)?;
                if self.coverage().map().raster().cells()[index].zone != Some(zone.id())
                    || !self.object_fits(entry, position, zone)?
                {
                    continue;
                }
                if let Some(town) = town_anchor {
                    let distance = position.point.squared_distance(town);
                    if distance > best_distance
                        || i64::from(distance)
                            < i64::from(raw::RMG_STARTING_MINE_MINIMUM_SQUARED_DISTANCE)
                    {
                        continue;
                    }
                    let distance = distance.max(
                        i32::try_from(raw::RMG_STARTING_MINE_NEAR_SQUARED_DISTANCE)
                            .map_err(|_| PlacementError::Arithmetic)?,
                    );
                    if distance < best_distance {
                        best_distance = distance;
                        best_obstacles = 0;
                        best_score = 0;
                        self.zone_placement.candidates.clear();
                    }
                }
                let score = i32::from(self.cells[index].object_distance);
                if score < best_score {
                    continue;
                }
                let obstacles = self.mine_obstacle_score(position, outline)?;
                if obstacles < best_obstacles {
                    continue;
                }
                if obstacles > best_obstacles {
                    self.zone_placement.candidates.clear();
                    best_obstacles = obstacles;
                }
                // A higher obstacle score deliberately retains the distance bound.
                if score > best_score {
                    self.zone_placement.candidates.clear();
                    best_score = score;
                }
                self.zone_placement.candidates.try_reserve(1)?;
                self.zone_placement.candidates.push(position);
            }
        }
        let count = u32::try_from(self.zone_placement.candidates.len())
            .map_err(|_| PlacementError::Arithmetic)?;
        let Some(count) = NonZeroU32::new(count) else {
            return Ok(None);
        };
        let position = self.zone_placement.candidates[rng.below(count) as usize];
        self.register_object(objects, catalog, object, position)?;
        Ok(Some(position))
    }

    fn mine_obstacle_score(
        &self,
        position: WorldPosition,
        outline: &[Point],
    ) -> Result<u32, PlacementError> {
        let mut obstacles = 0_u32;
        for &offset in outline {
            let point = position
                .point
                .checked_add(offset)
                .ok_or(PlacementError::CoordinateOverflow)?;
            if !self.view().contains(point) || point.y > position.point.y {
                continue;
            }
            let index = self.view().index(WorldPosition {
                point,
                level: position.level,
            })?;
            if self.view().passable(index)
                && self.cells[index].reservation == PathReservation::Obstacle
            {
                obstacles += 1;
            }
        }
        let obstacles = obstacles.min(raw::RMG_MINE_MAXIMUM_OBSTACLE_SCORE);
        Ok(obstacles)
    }

    #[expect(
        clippy::too_many_arguments,
        reason = "resource strip retains the native last-scanned mine art"
    )]
    fn place_mine_resources(
        &mut self,
        zone: BoundaryZone,
        resource: Resource,
        anchor: WorldPosition,
        last_mine: &PreparedPrototype<'_>,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<(), PlacementError> {
        let Some(prototype) =
            catalog.choose(kind(raw::RESOURCE), resource as i32, zone.terrain(), rng)
        else {
            return Ok(());
        };
        let entry = catalog.get(prototype).expect("selected from catalog");
        let width = last_mine.image_mask().signed_width();
        let side =
            i32::try_from(self.view().side).map_err(|_| PlacementError::CoordinateOverflow)?;
        // North is up. P is the mine anchor; w is the LAST scanned art's width.
        // The clipped row below spans x=P.x-w through P.x+1 (inclusive).
        //   ... mine ... P
        //   r r r r r r r r
        let minimum = anchor
            .point
            .checked_add(Point::new(-width, 1))
            .ok_or(PlacementError::CoordinateOverflow)?;
        let maximum = anchor
            .point
            .checked_add(Point::new(2, 2))
            .ok_or(PlacementError::CoordinateOverflow)?;
        let mut placed = 0;
        for y in minimum.y.max(0)..maximum.y.min(side) {
            for x in minimum.x.max(0)..maximum.x.min(side) {
                if placed >= raw::RMG_MINE_RESOURCE_PILE_LIMIT {
                    break;
                }
                let position = WorldPosition {
                    point: Point::new(x, y),
                    level: anchor.level,
                };
                if rng.draw() % 2 == 0 && self.object_fits(entry, position, zone)? {
                    placed += 1;
                    let object = objects.create_resource(catalog, prototype)?;
                    self.register_object(objects, catalog, object, position)?;
                }
            }
        }
        Ok(())
    }
}
