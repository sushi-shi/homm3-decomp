//! Integer Delaunay subdivision and its Voronoi dual.
//!
//! Edge order, integer truncation and cocircular tie decisions follow the C++
//! RMG. Storage is contiguous, with IDs replacing owning and cyclic pointers.

use std::{cmp::Ordering, collections::TryReserveError, error::Error, fmt};

/// Signed world-plane position; outside-map sites are useful boundary inputs.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Point {
    /// Eastward coordinate.
    pub x: i32,
    /// Southward coordinate.
    pub y: i32,
}

impl Ord for Point {
    fn cmp(&self, other: &Self) -> Ordering {
        (self.y, self.x).cmp(&(other.y, other.x))
    }
}
impl PartialOrd for Point {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Point {
    /// Construct a signed plane point. This does not claim it is a map cell.
    #[must_use]
    pub const fn new(x: i32, y: i32) -> Self {
        Self { x, y }
    }

    fn minus(self, other: Self) -> Self {
        Self::new(self.x.wrapping_sub(other.x), self.y.wrapping_sub(other.y))
    }
    fn plus(self, other: Self) -> Self {
        Self::new(self.x.wrapping_add(other.x), self.y.wrapping_add(other.y))
    }
    fn times(self, factor: i32) -> Self {
        Self::new(self.x.wrapping_mul(factor), self.y.wrapping_mul(factor))
    }
    fn divided(self, divisor: i32) -> Result<Self, GeometryError> {
        Ok(Self::new(
            self.x
                .checked_div(divisor)
                .ok_or(GeometryError::InvalidDivision)?,
            self.y
                .checked_div(divisor)
                .ok_or(GeometryError::InvalidDivision)?,
        ))
    }
    fn dot(self, other: Self) -> i32 {
        self.y
            .wrapping_mul(other.y)
            .wrapping_add(self.x.wrapping_mul(other.x))
    }
    fn squared_norm(self) -> i32 {
        self.dot(self)
    }

    /// Retail 32-bit squared distance; overflow wraps as in the x86 operations.
    #[must_use]
    pub fn squared_distance(self, other: Self) -> i32 {
        self.minus(other).squared_norm()
    }

    /// Euclidean distance truncated after the original 32-bit squared norm.
    ///
    /// # Errors
    /// Returns an arithmetic fault if overflow makes the squared norm negative.
    pub fn distance(self, other: Self) -> Result<i32, GeometryError> {
        let squared = self.squared_distance(other);
        if squared < 0 {
            return Err(GeometryError::NegativeSquaredDistance);
        }
        // The input is a nonnegative i32, so the truncated root fits i32.
        #[allow(clippy::cast_possible_truncation)]
        Ok(f64::from(squared).sqrt() as i32)
    }
}

/// Signed twice-area: positive is clockwise on the map, whose Y grows down.
#[must_use]
pub fn orientation(first: Point, second: Point, third: Point) -> i32 {
    let a = second.minus(first);
    let b = third.minus(first);
    a.x.wrapping_mul(b.y).wrapping_sub(a.y.wrapping_mul(b.x))
}

/// A dense zone identity. Template preparation owns assignment of these IDs.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct ZoneId(usize);
impl ZoneId {
    /// Index into the owning prepared template/generation's zone storage.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0
    }

    pub(crate) const fn new(index: usize) -> Self {
        Self(index)
    }
}

/// Geometry failures where retail can divide by zero or fail to locate a site.
#[derive(Debug)]
pub enum GeometryError {
    /// A site is outside the fixed enclosing triangulation.
    OutsideSubdivision(Point),
    /// A denominator is zero, or signed integer division overflows.
    InvalidDivision,
    /// A wrapped squared norm cannot be square-rooted.
    NegativeSquaredDistance,
    /// A bounded location walk found no containing edge.
    UnlocatableSite(Point),
    /// Storage could not be reserved.
    Allocation(TryReserveError),
}
impl fmt::Display for GeometryError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::OutsideSubdivision(point) => write!(f, "site outside subdivision: {point:?}"),
            Self::InvalidDivision => f.write_str("invalid integer division in geometry"),
            Self::NegativeSquaredDistance => f.write_str("negative wrapped squared distance"),
            Self::UnlocatableSite(point) => write!(f, "cannot locate subdivision site: {point:?}"),
            Self::Allocation(error) => error.fmt(f),
        }
    }
}
impl Error for GeometryError {}
impl From<TryReserveError> for GeometryError {
    fn from(value: TryReserveError) -> Self {
        Self::Allocation(value)
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct EdgeId(usize);

#[derive(Clone, Copy, Debug)]
struct HalfEdge {
    site: Point,
    zone: Option<ZoneId>,
    twin: EdgeId,
    next: EdgeId,
    previous: EdgeId,
    vertex: Option<Point>,
}

/// Mutable Delaunay construction. Consume it to obtain a completed Voronoi view.
#[derive(Debug)]
pub struct Delaunay {
    edges: Vec<HalfEdge>,
    // Active edges retain C++ insertion/erasure order. Removed storage is not
    // reused during a build, so existing edge IDs never acquire a new identity.
    order: Vec<EdgeId>,
    starting: EdgeId,
}

impl Delaunay {
    /// Start from the same square and diagonal as the C++ constructor.
    ///
    /// North is up (Y down); 1–4 are the outer edges, 5 the diagonal.
    /// ```text
    ///   first --1--> second
    ///     ^  \         |
    ///     4    5       2
    ///     |      \     v
    ///   fourth <-3-- third
    /// ```
    ///
    /// # Errors
    /// Reports a failed storage reservation.
    pub fn new() -> Result<Self, GeometryError> {
        let mut diagram = Self {
            edges: Vec::new(),
            order: Vec::new(),
            starting: EdgeId(0),
        };
        diagram.initialize()?;
        Ok(diagram)
    }

    fn initialize(&mut self) -> Result<(), GeometryError> {
        self.edges.clear();
        self.order.clear();
        self.edges.try_reserve(10)?;
        self.order.try_reserve(10)?;
        let a = Point::new(-200, -200);
        let b = Point::new(400, -200);
        let c = Point::new(400, 400);
        let d = Point::new(-200, 400);
        let first = self.create(a, None, b, None)?;
        let second = self.create(b, None, c, None)?;
        let third = self.create(c, None, d, None)?;
        let fourth = self.create(d, None, a, None)?;
        self.splice(self.twin(first), second);
        self.splice(self.twin(second), third);
        self.splice(self.twin(third), fourth);
        self.splice(self.twin(fourth), first);
        self.connect(fourth, third)?;
        self.starting = first;
        Ok(())
    }

    fn edge(&self, id: EdgeId) -> &HalfEdge {
        &self.edges[id.0]
    }
    fn twin(&self, id: EdgeId) -> EdgeId {
        self.edge(id).twin
    }
    fn next(&self, id: EdgeId) -> EdgeId {
        self.edge(id).next
    }
    fn previous(&self, id: EdgeId) -> EdgeId {
        self.edge(id).previous
    }
    fn left_next(&self, id: EdgeId) -> EdgeId {
        self.previous(self.twin(id))
    }
    fn left_previous(&self, id: EdgeId) -> EdgeId {
        self.twin(self.next(id))
    }
    fn destination(&self, id: EdgeId) -> Point {
        self.edge(self.twin(id)).site
    }

    fn create(
        &mut self,
        first: Point,
        first_zone: Option<ZoneId>,
        second: Point,
        second_zone: Option<ZoneId>,
    ) -> Result<EdgeId, GeometryError> {
        // Reserve both lists before mutation; no Box allocation per edge/twin.
        self.edges.try_reserve(2)?;
        self.order.try_reserve(2)?;
        let first_id = EdgeId(self.edges.len());
        let second_id = EdgeId(self.edges.len() + 1);
        self.edges.push(HalfEdge {
            site: first,
            zone: first_zone,
            twin: second_id,
            next: first_id,
            previous: first_id,
            vertex: None,
        });
        self.edges.push(HalfEdge {
            site: second,
            zone: second_zone,
            twin: first_id,
            next: second_id,
            previous: second_id,
            vertex: None,
        });
        self.order.extend([first_id, second_id]);
        Ok(first_id)
    }

    fn splice(&mut self, first: EdgeId, second: EdgeId) {
        let a = self.next(first);
        let b = self.next(second);
        let saved = self.previous(a);
        self.edges[a.0].previous = self.previous(b);
        self.edges[b.0].previous = saved;
        self.edges[first.0].next = b;
        self.edges[second.0].next = a;
    }

    fn detach(&mut self, edge: EdgeId) {
        let previous = self.previous(edge);
        let twin_previous = self.left_next(edge);
        self.splice(edge, previous);
        self.splice(self.twin(edge), twin_previous);
    }

    fn connect(&mut self, first: EdgeId, second: EdgeId) -> Result<EdgeId, GeometryError> {
        let opposite = *self.edge(self.twin(first));
        let next = *self.edge(second);
        let edge = self.create(opposite.site, opposite.zone, next.site, next.zone)?;
        self.splice(edge, self.left_next(first));
        self.splice(self.twin(edge), second);
        Ok(edge)
    }

    fn remove(&mut self, edge: EdgeId) {
        self.detach(edge);
        let twin = self.twin(edge);
        self.order.retain(|&id| id != edge && id != twin);
    }

    fn right_of(&self, point: Point, edge: EdgeId) -> bool {
        orientation(self.edge(edge).site, point, self.destination(edge)) > 0
    }

    fn locate(&self, point: Point) -> Result<EdgeId, GeometryError> {
        if !(-200..=400).contains(&point.x) || !(-200..=400).contains(&point.y) {
            return Err(GeometryError::OutsideSubdivision(point));
        }
        let mut edge = self.starting;
        // The walk has no changing state besides its directed edge. Revisiting
        // an edge repeats forever, so at most active-edge-count steps can help.
        for _ in 0..self.order.len() {
            if point == self.edge(edge).site {
                return Ok(edge);
            }
            if point == self.destination(edge) {
                return Ok(self.twin(edge));
            }
            if self.right_of(point, edge) {
                edge = self.twin(edge);
            } else if !self.right_of(point, self.next(edge)) {
                edge = self.next(edge);
            } else {
                let previous = self.twin(self.left_next(edge));
                if self.right_of(point, previous) {
                    return Ok(edge);
                }
                edge = previous;
            }
        }
        Err(GeometryError::UnlocatableSite(point))
    }

    fn flip(&mut self, edge: EdgeId) {
        let previous = self.previous(edge);
        let twin_previous = self.left_next(edge);
        let twin = self.twin(edge);
        self.detach(edge);
        let first = *self.edge(self.twin(previous));
        let second = *self.edge(self.twin(twin_previous));
        self.edges[edge.0].site = first.site;
        self.edges[edge.0].zone = first.zone;
        self.edges[twin.0].site = second.site;
        self.edges[twin.0].zone = second.zone;
        self.splice(edge, self.left_next(previous));
        self.splice(twin, self.left_next(twin_previous));
    }

    fn on_segment(&self, point: Point, edge: EdgeId) -> bool {
        let origin = self.edge(edge).site;
        let opposite = self.destination(edge);
        let length = origin.squared_distance(opposite);
        point.squared_distance(origin) <= length
            && point.squared_distance(opposite) <= length
            && orientation(origin, opposite, point) == 0
    }

    /// Insert a site in retail order; repeated coordinates leave the first site.
    ///
    /// # Errors
    /// Reports sites outside the subdivision, impossible walks, and allocation
    /// failures. Discard/recycle this build after an allocation failure.
    pub fn add_site(&mut self, point: Point, zone: Option<ZoneId>) -> Result<(), GeometryError> {
        let mut edge = self.locate(point)?;
        if point == self.edge(edge).site || point == self.destination(edge) {
            return Ok(());
        }
        if self.on_segment(point, edge) {
            edge = self.previous(edge);
            self.remove(self.next(edge));
        }
        let origin = *self.edge(edge);
        let mut fan_base = self.create(origin.site, origin.zone, point, zone)?;
        self.splice(fan_base, edge);
        self.starting = fan_base;
        loop {
            fan_base = self.connect(edge, self.twin(fan_base))?;
            edge = self.previous(fan_base);
            if self.left_next(edge) == self.starting {
                break;
            }
        }
        loop {
            let previous = self.previous(edge);
            if self.right_of(self.destination(previous), edge)
                && inside_circumcircle(
                    self.edge(edge).site,
                    self.destination(previous),
                    self.destination(edge),
                    point,
                )
            {
                self.flip(edge);
                edge = self.previous(edge);
            } else if self.next(edge) == self.starting {
                return Ok(());
            } else {
                edge = self.left_previous(self.next(edge));
            }
        }
    }

    /// Compute the dual vertices, preventing further site insertion afterward.
    ///
    /// # Errors
    /// Reports collinear/overflowing integer circumcenter divisions.
    pub fn finish(mut self) -> Result<Voronoi, GeometryError> {
        for index in 0..self.order.len() {
            let mut id = self.order[index];
            let edge = *self.edge(id);
            if edge.zone.is_some() && edge.vertex.is_none() {
                let vertex = circumcenter(
                    self.destination(self.next(id)),
                    edge.site,
                    self.destination(id),
                )?;
                for _ in 0..3 {
                    self.edges[id.0].vertex = Some(vertex);
                    id = self.left_previous(id);
                }
            }
        }
        Ok(Voronoi { diagram: self })
    }
}

/// Completed diagram: face vertices cannot become stale through site insertion.
#[derive(Debug)]
pub struct Voronoi {
    diagram: Delaunay,
}

/// One edge of the polygon surrounding a zone site.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct BoundaryEdge {
    /// Site across the boundary.
    pub opposite_site: Point,
    /// Zone across the boundary, absent for the enclosing or unowned sites.
    pub opposite_zone: Option<ZoneId>,
    /// Computed dual vertex; unowned enclosing faces need not have vertices.
    pub vertex: Option<Point>,
}

impl Voronoi {
    /// Iterate the site's ring in original `next` order without allocating.
    ///
    /// # Errors
    /// Reports a point outside the subdivision or not equal to an inserted site.
    pub fn boundary(
        &self,
        site: Point,
    ) -> Result<impl Iterator<Item = BoundaryEdge> + '_, GeometryError> {
        let first = self.diagram.locate(site)?;
        if self.diagram.edge(first).site != site {
            return Err(GeometryError::UnlocatableSite(site));
        }
        let mut current = Some(first);
        Ok(std::iter::from_fn(move || {
            let id = current?;
            let edge = self.diagram.edge(id);
            let opposite = self.diagram.edge(edge.twin);
            current = (edge.next != first).then_some(edge.next);
            Some(BoundaryEdge {
                opposite_site: opposite.site,
                opposite_zone: opposite.zone,
                vertex: edge.vertex,
            })
        }))
    }

    /// Begin another build while retaining the existing storage capacity.
    ///
    /// # Errors
    /// Reports a failed reservation if storage needs to grow.
    pub fn recycle(mut self) -> Result<Delaunay, GeometryError> {
        self.diagram.initialize()?;
        Ok(self.diagram)
    }
}

fn inside_circumcircle(first: Point, second: Point, third: Point, point: Point) -> bool {
    // Norms and orientations are i32 before widening. Cocircular points do not
    // request an edge flip; changing > to >= changes deterministic topology.
    let product = |a: Point, area: i32| i64::from(a.squared_norm()) * i64::from(area);
    product(third, orientation(first, second, point))
        .wrapping_sub(product(second, orientation(first, third, point)))
        .wrapping_add(product(first, orientation(second, third, point)))
        .wrapping_sub(product(point, orientation(first, second, third)))
        > 0
}

fn circumcenter(third: Point, origin: Point, second: Point) -> Result<Point, GeometryError> {
    let axis = second.minus(origin);
    let perpendicular = Point::new(axis.y.wrapping_neg(), axis.x);
    let second_side = third.minus(second);
    let third_side = origin.minus(third);
    let projected = perpendicular
        .times(second_side.dot(third_side))
        .divided(perpendicular.dot(third_side))?;
    Ok(origin.plus(axis.plus(projected).divided(2)?))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn assert_topology(diagram: &Delaunay) {
        for &id in &diagram.order {
            let edge = diagram.edge(id);
            assert_eq!(diagram.twin(edge.twin), id);
            assert_eq!(diagram.previous(edge.next), id);
            assert_eq!(diagram.next(edge.previous), id);
            assert_eq!(diagram.edge(edge.next).site, edge.site);
        }
    }

    #[test]
    fn insertion_on_diagonal_and_cocircular_sites_preserves_rings() {
        let mut diagram = Delaunay::new().unwrap();
        let points = [
            Point::new(10, 10),
            Point::new(20, 10),
            Point::new(20, 20),
            Point::new(10, 20),
            Point::new(15, 15),
        ];
        for (index, &point) in points.iter().enumerate() {
            diagram.add_site(point, Some(ZoneId::new(index))).unwrap();
            assert_topology(&diagram);
        }
        let edges = diagram.order.len();
        diagram.add_site(points[0], Some(ZoneId::new(99))).unwrap();
        assert_eq!(diagram.order.len(), edges);
        let voronoi = diagram.finish().unwrap();
        for point in points {
            assert!(voronoi
                .boundary(point)
                .unwrap()
                .all(|edge| edge.vertex.is_some()));
        }
        assert_eq!(voronoi.boundary(points[4]).unwrap().count(), 4);
        let recycled = voronoi.recycle().unwrap();
        assert_topology(&recycled);
        assert_eq!(recycled.order.len(), 10);
    }

    #[test]
    fn grid_sites_do_not_corrupt_insertion_order_or_dual_vertices() {
        let mut diagram = Delaunay::new().unwrap();
        for y in (0..144).step_by(12) {
            for x in (0..144).step_by(12) {
                let zone = ZoneId::new(diagram.order.len());
                diagram.add_site(Point::new(x, y), Some(zone)).unwrap();
                assert_topology(&diagram);
            }
        }
        let result = diagram.finish().unwrap();
        assert!(result
            .boundary(Point::new(60, 60))
            .unwrap()
            .all(|edge| edge.vertex.is_some()));
    }

    #[test]
    fn integer_circumcenter_and_ordering_follow_map_conventions() {
        assert_eq!(
            circumcenter(Point::new(0, 10), Point::new(0, 0), Point::new(10, 0)).unwrap(),
            Point::new(5, 5)
        );
        assert!(Point::new(10, 1) < Point::new(0, 2));
        assert_eq!(Point::new(0, 0).distance(Point::new(3, 4)).unwrap(), 5);
        assert!(matches!(
            Delaunay::new().unwrap().locate(Point::new(401, 0)),
            Err(GeometryError::OutsideSubdivision(_))
        ));
    }
}
