//! Road/river tile layers and the shared four-connected native line walker.
use super::{CellState, Neighborhood, PlacementError, PlacementMap};
use crate::{
    domain::{Terrain, WorldPosition},
    geometry::Point,
    line::{LineTable, Reflection},
    raw,
    rng::RetailRng,
};

/// Road graphics chosen once for the generated map.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum RoadType {
    /// Dirt track.
    Dirt = raw::RMG_ROAD_DIRT,
    /// Gravel road.
    Gravel = raw::RMG_ROAD_GRAVEL,
    /// Cobblestone road.
    Cobblestone = raw::RMG_ROAD_COBBLESTONE,
}
/// River graphics selected from the source cell's terrain.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum RiverType {
    /// Dry non-snow terrain.
    Clear = raw::RMG_RIVER_CLEAR,
    /// Snow terrain.
    Icy = raw::RMG_RIVER_ICY,
}
/// A line layer retains frame/reflection data independently of its presence.
/// Fields are private so only a painter can construct a frame for its layer.
#[derive(Clone, Copy, Debug)]
pub struct LineTile<T> {
    kind: Option<T>,
    frame: u8,
    reflection: Reflection,
}
impl<T> Default for LineTile<T> {
    fn default() -> Self {
        Self {
            kind: None,
            frame: 0,
            reflection: Reflection::default(),
        }
    }
}
impl<T: Copy> LineTile<T> {
    /// Present road/river type, or an empty layer.
    #[must_use]
    pub const fn kind(self) -> Option<T> {
        self.kind
    }
    /// Sprite frame, retained even when the type is cleared independently.
    #[must_use]
    pub const fn frame(self) -> u8 {
        self.frame
    }
    /// Sprite reflection.
    #[must_use]
    pub const fn reflection(self) -> Reflection {
        self.reflection
    }
}
impl CellState {
    /// Current road layer.
    #[must_use]
    pub const fn road(&self) -> LineTile<RoadType> {
        self.road
    }
    /// Current river layer, separate from object-created river targets.
    #[must_use]
    pub const fn river(&self) -> LineTile<RiverType> {
        self.river
    }
}
#[derive(Clone, Copy)]
pub(super) enum Layer {
    Road(RoadType),
    River(RiverType),
}
impl Layer {
    fn table(self) -> LineTable {
        match self {
            Self::Road(_) => LineTable::ROAD,
            Self::River(_) => LineTable::RIVER,
        }
    }
    fn kind(self, cell: &CellState) -> Option<u32> {
        match self {
            Self::Road(_) => cell.road.kind.map(|k| k as u32),
            Self::River(_) => cell.river.kind.map(|k| k as u32),
        }
    }
    fn value(self) -> u32 {
        match self {
            Self::Road(k) => k as u32,
            Self::River(k) => k as u32,
        }
    }
}

impl PlacementMap<'_, '_, '_> {
    fn line_neighbour(&self, at: WorldPosition, direction: usize) -> Option<WorldPosition> {
        // Tile classifiers start at N; movement directions start at E.
        let movement = (direction + raw::RMG_DIRECTION_NORTH as usize) % raw::DIRECTIONS.len();
        let (x, y) = raw::DIRECTIONS[movement];
        // Native masking tests equality with the four edges, not general
        // containment. Raw river endpoints may alias another allocated row.
        let last = self.view().signed_side() - 1;
        if (at.point.x == 0 && x < 0)
            || (at.point.x == last && x > 0)
            || (at.point.y == 0 && y < 0)
            || (at.point.y == last && y > 0)
        {
            return None;
        }
        Some(WorldPosition {
            point: Point::new(at.point.x.wrapping_add(x), at.point.y.wrapping_add(y)),
            ..at
        })
    }
    fn set_line_frame(
        &mut self,
        layer: Layer,
        at: WorldPosition,
        frame: u8,
        reflection: Reflection,
    ) -> Result<(), PlacementError> {
        let index = self.view().native_index(at)?;
        match layer {
            Layer::Road(_) => {
                self.cells[index].road.frame = frame;
                self.cells[index].road.reflection = reflection;
            }
            Layer::River(_) => {
                self.cells[index].river.frame = frame;
                self.cells[index].river.reflection = reflection;
                self.cells[index].river_join_target = self.cells[index].river.kind.is_some();
                if self.cells[index].river_join_target {
                    for near in Neighborhood::ThreeByThree.cells(at, self.view().side())? {
                        let index = self.view().native_index(near)?;
                        self.cells[index].near_river = true;
                    }
                    for near in Neighborhood::FiveByFive.cells(at, self.view().side())? {
                        let index = self.view().native_index(near)?;
                        if self.cells[index].river.kind.is_none() {
                            self.cells[index].river_outlet_target = false;
                        }
                    }
                }
            }
        }
        Ok(())
    }
    fn refresh_line(
        &mut self,
        layer: Layer,
        at: WorldPosition,
        rng: &mut RetailRng,
    ) -> Result<(), PlacementError> {
        let index = self.view().native_index(at)?;
        let Some(kind) = layer.kind(&self.cells[index]) else {
            return Ok(());
        };
        let mut neighbours = [false; raw::TILE_DIR_COUNT as usize];
        for (direction, matches) in neighbours.iter_mut().enumerate() {
            if let Some(near) = self.line_neighbour(at, direction) {
                *matches = layer.kind(&self.cells[self.view().native_index(near)?]) == Some(kind);
            }
        }
        let table = layer.table();
        let selection = table.select(&neighbours);
        let range = table
            .range(selection.pattern)
            .expect("canonical line table has selected shape");
        let (frame, reflection) = match layer {
            Layer::Road(_) => (
                self.cells[index].road.frame,
                self.cells[index].road.reflection,
            ),
            Layer::River(_) => (
                self.cells[index].river.frame,
                self.cells[index].river.reflection,
            ),
        };
        if !(range.first()..range.first() + range.count().get()).contains(&u32::from(frame))
            || reflection != selection.reflection
        {
            let frame = range.select(rng);
            self.set_line_frame(layer, at, frame, selection.reflection)?;
        }
        Ok(())
    }
    fn clear_line_point(
        &mut self,
        layer: Layer,
        at: WorldPosition,
        rng: &mut RetailRng,
    ) -> Result<(), PlacementError> {
        let index = self.view().native_index(at)?;
        // clearRmgLineRectangle's only generation call is a 1x1 replacement.
        // North is up; # is cleared and digits give border refresh order:
        //   1 3 2
        //   1 # 2
        //   1 4 2
        // Grid rectangles use unsigned coordinates even when the map adapter
        // later interprets their bits as signed flat offsets.
        let x = u32::from_ne_bytes(at.point.x.to_ne_bytes());
        let y = u32::from_ne_bytes(at.point.y.to_ne_bytes());
        let side = self.view().signed_side().unsigned_abs();
        let first = y.saturating_sub(1);
        let south = y.wrapping_add(1);
        let east = x.wrapping_add(1);
        if x < east && y < south {
            match layer {
                Layer::Road(_) => self.cells[index].road = LineTile::default(),
                Layer::River(_) => {
                    self.cells[index].river = LineTile::default();
                    self.cells[index].river_join_target = false;
                }
            }
        }
        let position = |x: u32, y: u32| WorldPosition {
            point: Point::new(
                i32::from_ne_bytes(x.to_ne_bytes()),
                i32::from_ne_bytes(y.to_ne_bytes()),
            ),
            ..at
        };
        if x > 0 {
            let end = south.wrapping_add(u32::from(south < side));
            for row in first..end {
                self.refresh_line(layer, position(x - 1, row), rng)?;
            }
        }
        if east < side {
            // Native right column omits the last row at the penultimate row.
            let end = south.wrapping_add(u32::from(south < side - 1));
            for row in first..end {
                self.refresh_line(layer, position(east, row), rng)?;
            }
        }
        // Border-row loops can be empty when unsigned origin+width wraps.
        if x < east {
            if y > 0 {
                self.refresh_line(layer, position(x, y - 1), rng)?;
            }
            if south < side {
                self.refresh_line(layer, position(x, south), rng)?;
            }
        }
        Ok(())
    }
    pub(super) fn paint_line_point(
        &mut self,
        layer: Layer,
        at: WorldPosition,
        rng: &mut RetailRng,
    ) -> Result<(), PlacementError> {
        let index = self.view().native_index(at)?;
        let old = layer.kind(&self.cells[index]);
        if old == Some(layer.value())
            || matches!(
                self.terrain.tiles()[index].terrain(),
                Terrain::Water | Terrain::Rock
            )
        {
            return Ok(());
        }
        if old.is_some() {
            self.clear_line_point(layer, at, rng)?;
        }
        match layer {
            Layer::Road(kind) => self.cells[index].road.kind = Some(kind),
            Layer::River(kind) => {
                self.cells[index].river.kind = Some(kind);
                self.cells[index].river_join_target = true;
            }
        }
        self.refresh_line(layer, at, rng)?;
        for direction in 0..raw::TILE_DIR_COUNT as usize {
            if let Some(near) = self.line_neighbour(at, direction) {
                if layer.kind(&self.cells[self.view().native_index(near)?]) == Some(layer.value()) {
                    self.refresh_line(layer, near, rng)?;
                }
            }
        }
        Ok(())
    }
    pub(super) fn draw_line(
        &mut self,
        layer: Layer,
        from: WorldPosition,
        to: WorldPosition,
        rng: &mut RetailRng,
    ) -> Result<(), PlacementError> {
        if from.level != to.level {
            return Err(PlacementError::OutsideMap(to));
        }
        // Walk from destination back to previous. Paint the corner on every
        // minor-axis step, so a diagonal path stays four-connected:
        //   P # .
        //   . # #
        //   . . D
        // TRmgGridPoint arithmetic is unsigned. Keep its bit patterns until
        // each adapter access, including a raw negative retail river endpoint.
        let unsigned = |p: Point| {
            [
                u32::from_ne_bytes(p.x.to_ne_bytes()),
                u32::from_ne_bytes(p.y.to_ne_bytes()),
            ]
        };
        let from = unsigned(from.point);
        let mut point = unsigned(to.point);
        let distance = [from[0].abs_diff(point[0]), from[1].abs_diff(point[1])];
        let step = [
            if point[0] <= from[0] { 1 } else { -1 },
            if point[1] <= from[1] { 1 } else { -1 },
        ];
        let major = usize::from(distance[0] < distance[1]);
        let minor = 1 - major;
        let position = |p: [u32; 2]| WorldPosition {
            point: Point::new(
                i32::from_ne_bytes(p[0].to_ne_bytes()),
                i32::from_ne_bytes(p[1].to_ne_bytes()),
            ),
            ..to
        };
        let mut error = 0_u32;
        for _ in 0..distance[major] {
            self.paint_line_point(layer, position(point), rng)?;
            error = error.wrapping_add(distance[minor]);
            if error >= distance[major] {
                error -= distance[major];
                point[minor] = point[minor].wrapping_add_signed(step[minor]);
                self.paint_line_point(layer, position(point), rng)?;
            }
            point[major] = point[major].wrapping_add_signed(step[major]);
        }
        if error.wrapping_add(distance[minor]) >= distance[major] {
            self.paint_line_point(layer, position(point), rng)?;
        }
        Ok(())
    }
}
