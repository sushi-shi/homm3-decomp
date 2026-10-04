//! Portal endpoints, protection and the native remaining-connection pass.
use super::{
    offset_position, BorderColor, BorderGuardCount, ConnectionError, DirectConnections, Movement,
    ObjectArena, ObjectId, PathReservation, PlacementError, PlacementMap, TownsPlaced,
};
use crate::{
    boundaries::ZoneConnection,
    domain::{Terrain, WorldPosition},
    geometry::{Point, ZoneId},
    object::ObjectKind,
    prototype::{PrototypeCatalog, PrototypeId},
    raw,
    rng::{RetailRng, RngCheckpoint},
    traits::CreatureCatalog,
};

/// Routing list used by later road searches. One-way lists contain both endpoints.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PortalDirection {
    /// Entrances and exits in their successful placement order.
    OneWay,
    /// Every successfully placed two-way endpoint.
    TwoWay,
}
#[derive(Default, Debug)]
pub(super) struct PortalState {
    one_way: Vec<ObjectId>,
    two_way: Vec<ObjectId>,
}
impl PortalState {
    pub(super) fn reset(&mut self) {
        self.one_way.clear();
        self.two_way.clear();
    }
    fn append(
        &mut self,
        direction: PortalDirection,
        object: ObjectId,
    ) -> Result<(), PlacementError> {
        let list = match direction {
            PortalDirection::OneWay => &mut self.one_way,
            PortalDirection::TwoWay => &mut self.two_way,
        };
        list.try_reserve(1)?;
        list.push(object);
        Ok(())
    }
}
/// One-way selection cannot lose its corresponding exit prototype.
#[derive(Clone, Copy)]
enum PortalPrototype {
    TwoWay(PrototypeId),
    OneWay {
        entrance: PrototypeId,
        exit: PrototypeId,
    },
}
impl PortalPrototype {
    fn select(catalog: &PrototypeCatalog<'_>, index: usize) -> Result<Self, ConnectionError> {
        let two_way = ObjectKind::LITH_TWOWAY;
        let count = catalog.family(two_way).len();
        if index < count {
            Ok(Self::TwoWay(indexed(catalog, two_way, index)?))
        } else {
            let index = index - count;
            // Both indexed reads precede guard scaling and object allocation.
            let entrance = indexed(catalog, ObjectKind::LITH_ONEWAY_ENTRANCE, index)?;
            let exit = indexed(catalog, ObjectKind::LITH_ONEWAY_EXIT, index)?;
            Ok(Self::OneWay { entrance, exit })
        }
    }
    fn entrance(self) -> (PrototypeId, PortalDirection) {
        match self {
            Self::TwoWay(prototype) => (prototype, PortalDirection::TwoWay),
            Self::OneWay { entrance, .. } => (entrance, PortalDirection::OneWay),
        }
    }
}

/// Both connection passes are complete, including failed native placement attempts.
/// Unreachable water edges may remain incomplete, as in the native generator.
pub struct ConnectionsPlaced<'state, 'zones, 'tiles> {
    direct: DirectConnections<'state, 'zones, 'tiles>,
    rng: RngCheckpoint,
}
impl<'state, 'zones, 'tiles> ConnectionsPlaced<'state, 'zones, 'tiles> {
    pub(super) fn map_mut(&mut self) -> &mut PlacementMap<'state, 'zones, 'tiles> {
        self.direct.connecting.map_mut()
    }
    /// Current map and all successfully registered connection objects.
    #[must_use]
    pub const fn map(&self) -> &PlacementMap<'_, '_, '_> {
        self.direct.map()
    }
    /// Earlier town records remain available for subsequent object placement.
    #[must_use]
    pub const fn towns(&self) -> &TownsPlaced<'_, '_, '_> {
        self.direct.towns()
    }
    /// RNG after the remaining-connection pass.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng
    }
}
impl<'state, 'zones, 'tiles> DirectConnections<'state, 'zones, 'tiles> {
    /// Retry shipyards for remaining edges, then place and protect portal endpoints.
    /// The portal prototype cycle is shared across zones and consumes no RNG.
    ///
    /// # Errors
    /// Reports absent indexed prototypes, invalid context, arithmetic, allocation,
    /// placement or guard faults, retaining native mutation and RNG ordering.
    pub fn connect_remaining_zones(
        mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<ConnectionsPlaced<'state, 'zones, 'tiles>, ConnectionError> {
        self.connecting
            .map_mut()
            .prepare_object_context(objects, catalog)?;
        let mut portal_index = 0_usize;
        for index in 0..self.map().coverage().map().zones().len() {
            let zone = self.map().coverage().map().zones()[index];
            if zone.terrain() == Terrain::Water {
                continue;
            }
            let first = self
                .map()
                .coverage()
                .map()
                .connections()
                .iter()
                .position(|edge| edge.source == zone.id() && !edge.connected);
            let Some(first) = first else {
                continue;
            };
            self.connecting.clear_connection_visits(zone.id())?;
            self.connecting
                .map_mut()
                .reflood_zone_shipyards(zone.id(), objects)?;
            for id in self.connecting.connection_ids().skip(first) {
                let connection = self.connecting.connection(id)?;
                if connection.source != zone.id() || connection.connected {
                    continue;
                }
                let reverse = self.connecting.reverse_connection(connection);
                if self
                    .connecting
                    .try_shipyard_connection(id, objects, catalog, creatures, rng)?
                {
                    continue;
                }
                if self.map().zone(connection.destination)?.terrain() == Terrain::Water {
                    continue;
                }
                self.connecting.map_mut().create_portal_connection(
                    connection,
                    portal_index,
                    objects,
                    catalog,
                    creatures,
                    rng,
                )?;
                // Even zero successful endpoints complete both records. Missing
                // reverse faults after current completion and before cursor advance.
                self.connecting
                    .complete_bidirectional(id, connection, reverse)?;
                let count = catalog
                    .family(ObjectKind::LITH_TWOWAY)
                    .len()
                    .checked_add(catalog.family(ObjectKind::LITH_ONEWAY_ENTRANCE).len())
                    .ok_or(PlacementError::Arithmetic)?;
                portal_index = portal_index
                    .checked_add(1)
                    .and_then(|next| next.checked_rem(count))
                    .ok_or(PlacementError::Arithmetic)?;
            }
        }
        Ok(ConnectionsPlaced {
            direct: self,
            rng: rng.checkpoint(),
        })
    }
}
impl PlacementMap<'_, '_, '_> {
    /// Borrow portal identities in successful insertion order, retaining duplicates.
    #[must_use]
    pub fn portals(&self, direction: PortalDirection) -> &[ObjectId] {
        match direction {
            PortalDirection::OneWay => &self.connections.portals.one_way,
            PortalDirection::TwoWay => &self.connections.portals.two_way,
        }
    }
    fn reflood_zone_shipyards(
        &mut self,
        zone: ZoneId,
        objects: &ObjectArena,
    ) -> Result<(), PlacementError> {
        for index in 0..self.active_objects().len() {
            let id = self.active_objects()[index];
            let object = objects.positioned(id)?;
            if object.kind() != ObjectKind::SHIPYARD {
                continue;
            }
            let position = object.position();
            let index = self.view().native_index(position)?;
            if self.coverage().map().raster().cells()[index].zone == Some(zone) {
                self.flood_shipyard_water(position)?;
            }
        }
        Ok(())
    }
    fn create_portal_connection(
        &mut self,
        connection: ZoneConnection,
        index: usize,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<(), ConnectionError> {
        let source = self.zone(connection.source)?;
        let destination = self.zone(connection.destination)?;
        if source.terrain() == Terrain::Water || destination.terrain() == Terrain::Water {
            return Ok(());
        }
        let prototypes = PortalPrototype::select(catalog, index)?;
        let (entrance, direction) = prototypes.entrance();
        let mut protection = PortalProtection {
            border_guard: connection.border_guard,
            guard_value: self.connection_guard_value(connection)?,
            catalog,
            creatures,
        };
        for (zone, tent_zone) in [
            (source.id(), destination.id()),
            (destination.id(), source.id()),
        ] {
            if let Some(position) =
                self.place_portal(entrance, direction, zone, objects, catalog, rng)?
            {
                protection.protect(self, position, tent_zone, objects, rng)?;
            }
        }
        if let PortalPrototype::OneWay { exit, .. } = prototypes {
            self.place_portal(
                exit,
                PortalDirection::OneWay,
                source.id(),
                objects,
                catalog,
                rng,
            )?;
            self.place_portal(
                exit,
                PortalDirection::OneWay,
                destination.id(),
                objects,
                catalog,
                rng,
            )?;
        }
        Ok(())
    }
    fn place_portal(
        &mut self,
        prototype: PrototypeId,
        direction: PortalDirection,
        zone: ZoneId,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<Option<WorldPosition>, PlacementError> {
        let object = objects.create(catalog, prototype)?;
        if !self.place_object_in_zone(object, zone, objects, catalog, rng)? {
            objects.discard_unplaced(object)?;
            return Ok(None);
        }
        self.connections.portals.append(direction, object)?;
        let position = objects.positioned(object)?.position();
        // Native portal entrances use the anchor, not the prototype trigger.
        self.connections
            .crossing
            .append_entrance(zone, position.point)?;
        Ok(Some(position))
    }
    fn place_portal_border_guard(
        &mut self,
        position: WorldPosition,
        tent_zone: ZoneId,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<bool, ConnectionError> {
        // Always rebuild every zone's paths before querying the portal cell.
        self.build_zone_connection_paths(objects, catalog, rng)?;
        let index = self.view().native_index(position)?;
        let cost = self.cells[index].movement.cost();
        if u32::from(cost) >= raw::RMG_REACHED_COST_LIMIT {
            return Ok(false);
        }
        let guard_position = match self.cells[index].movement {
            Movement::Seed | Movement::Arrived { cost: 0, .. } => {
                self.portal_guard_fallback(position, index)?
            }
            Movement::Arrived { previous, .. } => previous,
            Movement::Initial | Movement::Unreached => {
                return Err(ConnectionError::InvalidPredecessor(position));
            }
        };
        let Some(color) = self
            .place_border_guard(
                guard_position,
                BorderGuardCount::Single,
                tent_zone,
                objects,
                catalog,
                rng,
            )?
            .reported_color()
        else {
            return Ok(false);
        };
        let color = BorderColor::from_subtype(color);
        // North is up; digits are probe indices around portal P.
        //   2 P 1
        //   4 0 3
        // All five cells are marked, including those containing objects.
        for delta in raw::PORTAL_BORDER_OFFSETS {
            let at = offset_position(position, Point::new(delta.0, delta.1))?;
            let index = self.view().native_index(at)?;
            self.cells[index].mark_border(color);
        }
        Ok(true)
    }
    fn portal_guard_fallback(
        &self,
        position: WorldPosition,
        index: usize,
    ) -> Result<WorldPosition, PlacementError> {
        let zone = self.coverage().map().raster().cells()[index].zone;
        for delta in raw::PORTAL_BORDER_OFFSETS {
            let at = offset_position(position, Point::new(delta.0, delta.1))?;
            let index = self.view().native_index(at)?;
            if self.coverage().map().raster().cells()[index].zone == zone
                && self.cells[index].reservation == PathReservation::Open
            {
                return Ok(at);
            }
        }
        offset_position(position, Point::new(0, 1))
    }
}
fn indexed(
    catalog: &PrototypeCatalog<'_>,
    family: ObjectKind,
    index: usize,
) -> Result<PrototypeId, ConnectionError> {
    catalog
        .at(family, index)
        .ok_or(ConnectionError::MissingPortalPrototype { family, index })
}

// One connection shares guard suppression across its two protected endpoints.
// Borrowed immutable catalogs travel with that policy; no extra allocation.
struct PortalProtection<'catalog, 'source> {
    border_guard: bool,
    guard_value: i32,
    catalog: &'catalog PrototypeCatalog<'source>,
    creatures: &'catalog CreatureCatalog,
}
impl PortalProtection<'_, '_> {
    fn protect(
        &mut self,
        map: &mut PlacementMap<'_, '_, '_>,
        position: WorldPosition,
        tent_zone: ZoneId,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<(), ConnectionError> {
        if self.border_guard
            && map.place_portal_border_guard(position, tent_zone, objects, self.catalog, rng)?
        {
            self.guard_value = 0;
        } else if self.guard_value > 0 {
            map.place_guard(
                self.guard_value,
                offset_position(position, Point::new(0, 1))?,
                objects,
                self.catalog,
                self.creatures,
                rng,
            )?;
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        behavior::Behavior,
        placement_rules::PlacementRules,
        prototype::{ImageMask, PrototypeSource},
        request::MapVersion,
    };
    use std::convert::Infallible;

    #[test]
    fn one_way_selection_requires_its_corresponding_exit_only_when_selected() {
        let row = |family: u32| {
            format!(
                "portal.def {:048b} {:048b} {:09b} {:09b} {family} 0 0 0\r\n",
                0,
                1_u64 << 47,
                1,
                1
            )
        };
        let rules =
            PlacementRules::parse(b"header\r\nheader\r\nheader\r\n", Behavior::Hotfix).unwrap();
        let bytes = format!(
            "2\r\n{}{}",
            row(raw::LITH_TWOWAY),
            row(raw::LITH_ONEWAY_ENTRANCE)
        );
        let source = PrototypeSource::parse(bytes.as_bytes(), |_| {
            Ok::<_, Infallible>(Some(
                ImageMask::parse(&[1, 1, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0]).unwrap(),
            ))
        })
        .unwrap();
        let catalog = source
            .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap();
        assert!(matches!(
            PortalPrototype::select(&catalog, 0),
            Ok(PortalPrototype::TwoWay(_))
        ));
        assert!(
            matches!(PortalPrototype::select(&catalog, 1), Err(ConnectionError::MissingPortalPrototype { family, index: 0 }) if family == ObjectKind::LITH_ONEWAY_EXIT)
        );
        assert!(
            matches!(PortalPrototype::select(&catalog, 2), Err(ConnectionError::MissingPortalPrototype { family, index: 1 }) if family == ObjectKind::LITH_ONEWAY_ENTRANCE)
        );
    }
}
