//! Allocation-free, row-major path patches with native clipping and protection.

use super::{PlacementError, PlacementMap};
use crate::{
    domain::{Terrain, WorldPosition},
    geometry::Point,
    raw,
};

/// The two square neighborhoods used by native placement and path carving.
///
/// North is up; `C` is the centre and `o` an included cell. Both squares include
/// their centre and diagonals, and are clipped to the current map plane.
///
/// ```text
///   ThreeByThree   FiveByFive
///     o o o        o o o o o
///     o C o        o o o o o
///     o o o        o o C o o
///                  o o o o o
///                  o o o o o
/// ```
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Neighborhood {
    /// Centre and its eight immediate neighbours.
    ThreeByThree,
    /// Centre and all cells within two steps on either axis.
    FiveByFive,
}
impl Neighborhood {
    fn radius(self) -> i32 {
        i32::try_from(match self {
            Self::ThreeByThree => raw::RMG_NEIGHBORHOOD_3X3,
            Self::FiveByFive => raw::RMG_NEIGHBORHOOD_5X5,
        })
        .unwrap()
    }

    // Visit the clipped rectangle north to south, west to east in each row.
    // At a map's northwest corner a 3x3 patch has this order (C is first):
    //   C 1
    //   2 3
    pub(super) fn cells(
        self,
        center: WorldPosition,
        side: usize,
    ) -> Result<impl Iterator<Item = WorldPosition>, PlacementError> {
        let radius = self.radius();
        let side = i32::try_from(side).map_err(|_| PlacementError::CoordinateOverflow)?;
        let minimum_y = center
            .point
            .y
            .checked_sub(radius)
            .ok_or(PlacementError::CoordinateOverflow)?
            .max(0);
        let minimum_x = center
            .point
            .x
            .checked_sub(radius)
            .ok_or(PlacementError::CoordinateOverflow)?
            .max(0);
        let maximum_y = center
            .point
            .y
            .checked_add(radius)
            .and_then(|v| v.checked_add(1))
            .ok_or(PlacementError::CoordinateOverflow)?
            .min(side);
        let maximum_x = center
            .point
            .x
            .checked_add(radius)
            .and_then(|v| v.checked_add(1))
            .ok_or(PlacementError::CoordinateOverflow)?
            .min(side);
        Ok((minimum_y..maximum_y).flat_map(move |y| {
            (minimum_x..maximum_x).map(move |x| WorldPosition {
                point: Point::new(x, y),
                level: center.level,
            })
        }))
    }
}

impl PlacementMap<'_, '_, '_> {
    /// Open the centre first, then its clipped 3x3 patch, retaining border guards.
    /// The centre uses native flat indexing, as does the source helper.
    ///
    /// # Errors
    /// Reports coordinate overflow or access outside the cell allocation.
    /// Centre mutation precedes neighborhood arithmetic, matching the source.
    pub fn open_path_patch(&mut self, center: WorldPosition) -> Result<(), PlacementError> {
        let index = self.view().native_index(center)?;
        self.cells[index].open_path();
        for position in Neighborhood::ThreeByThree.cells(center, self.view().side())? {
            let index = self.view().index(position)?;
            self.cells[index].open_path();
        }
        Ok(())
    }

    /// Mark the centre for obstacles, then release nearby dry, passable cells
    /// without entrances. Existing obstacle reservations and borders survive.
    ///
    /// # Errors
    /// Reports coordinate overflow or access outside the cell allocation.
    pub fn mark_obstacle_patch(&mut self, center: WorldPosition) -> Result<(), PlacementError> {
        let index = self.view().native_index(center)?;
        self.cells[index].mark_obstacle();
        for position in Neighborhood::ThreeByThree.cells(center, self.view().side())? {
            let index = self.view().index(position)?;
            if self.cells[index].entrance.is_none()
                && self.view().passable(index)
                && self.terrain.tiles()[index].terrain() != Terrain::Water
            {
                self.cells[index].release_path();
            }
        }
        Ok(())
    }

    /// Release clearance in a clipped square only where object membership is empty.
    /// Terrain and passability do not affect this helper; borders remain protected.
    ///
    /// # Errors
    /// Reports coordinate overflow or an accessed plane outside the map.
    pub fn release_neighborhood_path(
        &mut self,
        center: WorldPosition,
        neighborhood: Neighborhood,
    ) -> Result<(), PlacementError> {
        for position in neighborhood.cells(center, self.view().side())? {
            let index = self.view().index(position)?;
            if self.memberships.first(self.cells[index].objects).is_none() {
                self.cells[index].release_path();
            }
        }
        Ok(())
    }

    /// Open and return the cell directly south of an object's entrance.
    /// Native flat indexing can reach the next plane from the last map row.
    ///
    /// For an entrance at mask coordinate `(1, 1)`, `P` is the object anchor,
    /// `T = P - (1, 1)` is its trigger, and `A = T + (0, 1)` is the approach.
    /// This shows coordinates, not occupancy; the anchor need not be the trigger.
    ///
    /// ```text
    ///   T .
    ///   A P
    /// ```
    ///
    /// # Errors
    /// Reports coordinate overflow or access outside the cell allocation.
    pub fn open_entrance_approach(
        &mut self,
        entrance: WorldPosition,
    ) -> Result<WorldPosition, PlacementError> {
        let approach = WorldPosition {
            point: Point::new(
                entrance.point.x,
                entrance
                    .point
                    .y
                    .checked_add(1)
                    .ok_or(PlacementError::CoordinateOverflow)?,
            ),
            level: entrance.level,
        };
        let index = self.view().native_index(approach)?;
        self.cells[index].open_path();
        Ok(approach)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::domain::Level;
    #[test]
    fn clipping_keeps_row_major_order_and_does_not_require_center_inside_map() {
        let at = |x, y| WorldPosition {
            point: Point::new(x, y),
            level: Level::Underground,
        };
        assert!(Neighborhood::ThreeByThree.cells(at(0, 0), 36).unwrap().eq([
            at(0, 0),
            at(1, 0),
            at(0, 1),
            at(1, 1)
        ]));
        assert!(Neighborhood::ThreeByThree
            .cells(at(-1, 1), 36)
            .unwrap()
            .eq([at(0, 0), at(0, 1), at(0, 2)]));
        assert!(Neighborhood::FiveByFive
            .cells(at(-3, -3), 36)
            .unwrap()
            .next()
            .is_none());
        assert_eq!(
            Neighborhood::FiveByFive
                .cells(at(35, 35), 36)
                .unwrap()
                .count(),
            9
        );
        assert!(matches!(
            Neighborhood::ThreeByThree.cells(at(i32::MAX, 0), 36),
            Err(PlacementError::CoordinateOverflow)
        ));
    }
}
