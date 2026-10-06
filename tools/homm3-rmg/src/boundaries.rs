//! Voronoi boundary construction and connections for additional water zones.

use crate::{
    behavior::{Behavior, TownMask},
    domain::{Level, Terrain, WorldPosition},
    geometry::{Delaunay, GeometryError, Point, Voronoi, ZoneId},
    layout::Layout,
    raster::{RasterError, RasterWorkspace, ZoneBounds, ZoneRaster},
    raw,
    request::{MapVersion, Request, Town, Water},
    rng::{RetailRng, RngCheckpoint},
    selection::Player,
    template::{PerSlot, PlayerSlot, Template, TemplateZoneId, Zone},
};
use std::{collections::TryReserveError, error::Error, fmt};

/// Generated zones keep each template zone's identity: template zones occupy
/// the first generated slots in template order, before appended water zones.
impl From<TemplateZoneId> for ZoneId {
    fn from(id: TemplateZoneId) -> Self {
        Self::new(id.index())
    }
}

/// Which source supplies a generated zone's placement rules.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ZoneOrigin {
    /// The original, filtered template zone.
    Template(TemplateZoneId),
    /// A radial water zone; its size is the parent zone's scaled radius.
    Water,
}

/// Creature affiliation is used for comparisons, never unchecked indexing.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CreaturePreference {
    /// Prefer creatures of this faction.
    Faction(Town),
    /// Prefer neutral creatures.
    Neutral,
    /// Replayed retail residue matches no defined faction or neutral sentinel.
    UnmatchedRetail,
}
impl CreaturePreference {
    fn from_town(town: Option<Town>) -> Self {
        town.map_or(Self::Neutral, Self::Faction)
    }
    fn water(behavior: Behavior) -> Self {
        match behavior {
            Behavior::Hotfix => Self::Neutral,
            Behavior::Retail(profile) => {
                let value = i32::from_le_bytes([profile.heap_byte; 4]);
                if value == raw::eTownNeutral {
                    Self::Neutral
                } else {
                    Town::parse(value).map_or(Self::UnmatchedRetail, Self::Faction)
                }
            }
        }
    }
}

/// Zone state after subdivision. Its source template remains immutable.
#[derive(Clone, Copy, Debug)]
pub struct BoundaryZone {
    id: ZoneId,
    origin: ZoneOrigin,
    position: WorldPosition,
    template_size: u32,
    scaled_size: u32,
    alignment: Option<Town>,
    terrain: Terrain,
    creatures: CreaturePreference,
    bounds: Option<ZoneBounds>,
    primary_town: Option<WorldPosition>,
}
impl BoundaryZone {
    /// Entrance of the first successfully placed town, regardless of its owner.
    #[must_use]
    pub const fn primary_town(self) -> Option<WorldPosition> {
        self.primary_town
    }
    /// Dense generation identity.
    #[must_use]
    pub const fn id(self) -> ZoneId {
        self.id
    }
    /// Original template record or generated water-zone rules.
    #[must_use]
    pub const fn origin(self) -> ZoneOrigin {
        self.origin
    }
    /// Zone centre at the current stage, updated when preparing terrain coverage.
    #[must_use]
    pub const fn position(self) -> WorldPosition {
        self.position
    }
    /// Unscaled placement size; added water zones use their parent's scaled size.
    #[must_use]
    pub const fn template_size(self) -> u32 {
        self.template_size
    }
    /// Radius used by boundary roughness.
    #[must_use]
    pub const fn scaled_size(self) -> u32 {
        self.scaled_size
    }
    /// Resolved town faction, including retail's unused draw for water zones.
    #[must_use]
    pub const fn alignment(self) -> Option<Town> {
        self.alignment
    }
    /// Resolved zone terrain.
    #[must_use]
    pub const fn terrain(self) -> Terrain {
        self.terrain
    }
    /// Creature affiliation, including defined replay handling for residue.
    #[must_use]
    pub const fn creatures(self) -> CreaturePreference {
        self.creatures
    }
    /// Nonempty cell bounds once terrain coverage is prepared; empty zones have none.
    #[must_use]
    pub const fn bounds(self) -> Option<ZoneBounds> {
        self.bounds
    }
}

/// One directed template or added-water connection, in insertion order.
#[derive(Clone, Copy, Debug)]
pub struct ZoneConnection {
    /// Starting zone.
    pub source: ZoneId,
    /// Destination zone.
    pub destination: ZoneId,
    /// Unscaled guard value.
    pub value: i32,
    /// Wide connection without an ordinary guard.
    pub unguarded: bool,
    /// Whether to attempt a keymaster border guard.
    pub border_guard: bool,
    /// Added water-to-water connections begin complete; others are pending.
    pub connected: bool,
}

/// Boundary construction cannot proceed in the supported arithmetic/topology.
#[derive(Debug)]
pub enum BoundaryError {
    /// Geometry or integer subdivision failure.
    Geometry(GeometryError),
    /// Rasterization or clipping failure.
    Raster(RasterError),
    /// Zone data does not correspond to the prepared template or diagram.
    Topology,
    /// An intermediate exceeds the defined signed arithmetic domain.
    Arithmetic,
    /// Capacity could not be reserved.
    Allocation(TryReserveError),
}
impl fmt::Display for BoundaryError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Geometry(error) => error.fmt(f),
            Self::Raster(error) => error.fmt(f),
            Self::Topology => f.write_str("zone layout does not correspond to the subdivision"),
            Self::Arithmetic => f.write_str("unsupported boundary arithmetic"),
            Self::Allocation(error) => error.fmt(f),
        }
    }
}
impl Error for BoundaryError {}
impl From<GeometryError> for BoundaryError {
    fn from(value: GeometryError) -> Self {
        Self::Geometry(value)
    }
}
impl From<RasterError> for BoundaryError {
    fn from(value: RasterError) -> Self {
        Self::Raster(value)
    }
}
impl From<TryReserveError> for BoundaryError {
    fn from(value: TryReserveError) -> Self {
        Self::Allocation(value)
    }
}

/// Completed boundary stage, borrowing storage until the next generation.
pub struct BoundaryMap<'a> {
    workspace: &'a mut BoundaryWorkspace,
    request: &'a Request,
    template: &'a Template<'a>,
    players: PerSlot<Option<Player>>,
    water: Water,
}
impl<'a> BoundaryMap<'a> {
    pub(crate) fn complete_connection(&mut self, index: usize) {
        self.workspace.connections[index].connected = true;
    }
    pub(crate) fn record_primary_town(&mut self, id: ZoneId, entrance: WorldPosition, town: Town) {
        let hotfix = self.behavior().is_hotfix();
        let zone = &mut self.workspace.zones[id.index()];
        zone.primary_town = Some(entrance);
        if hotfix && zone.alignment.is_none() {
            zone.alignment = Some(town);
        }
    }
    /// Compatibility policy carried from the completed layout stage.
    #[must_use]
    pub const fn behavior(&self) -> Behavior {
        self.request.behavior()
    }
    /// Map format carried from the completed layout stage.
    #[must_use]
    pub const fn version(&self) -> MapVersion {
        self.request.version()
    }
    /// Exact request retained from layout for later placement and output.
    #[must_use]
    pub const fn request(&self) -> &Request {
        self.request
    }
    /// Selected source template retained from layout, without cloning its zones.
    #[must_use]
    pub const fn template(&self) -> &Template<'_> {
        self.template
    }
    /// Template rules of a generated zone; water zones have none.
    #[must_use]
    pub fn template_zone(&self, zone: &BoundaryZone) -> Option<&Zone> {
        match zone.origin {
            ZoneOrigin::Template(id) => Some(self.template.zone(id)),
            ZoneOrigin::Water => None,
        }
    }
    /// Resolved constructor water choice used in the map description.
    #[must_use]
    pub const fn water(&self) -> Water {
        self.water
    }
    /// Original player assignment for a template seat.
    #[must_use]
    pub fn player(&self, slot: PlayerSlot) -> Option<Player> {
        self.players[slot]
    }
    /// Cell ownership and terrain marks.
    #[must_use]
    #[expect(
        clippy::missing_panics_doc,
        reason = "private constructor requires an initialized raster"
    )]
    pub fn raster(&self) -> &ZoneRaster {
        self.workspace.grid.as_ref().unwrap()
    }
    /// Original zones followed by added water zones.
    #[must_use]
    pub fn zones(&self) -> &[BoundaryZone] {
        &self.workspace.zones
    }
    /// Directed connections; filter by source to recover each adjacency list.
    #[must_use]
    pub fn connections(&self) -> &[ZoneConnection] {
        &self.workspace.connections
    }
    /// Clipped polygon in insertion order, without allocating a per-zone list.
    #[must_use]
    pub fn polygon(&self, zone: ZoneId) -> impl DoubleEndedIterator<Item = Point> + '_ {
        self.workspace
            .polygons
            .iter()
            .filter_map(move |&(owner, point)| (owner == zone).then_some(point))
    }
    /// Distances to zones present before the last plane's additional sites.
    #[must_use]
    pub fn distances(&self, zone: ZoneId) -> Option<&[u16]> {
        let count = self.workspace.distance_columns;
        let start = zone.index().checked_mul(count)?;
        self.workspace
            .distances
            .get(start..start.checked_add(count)?)
    }
    /// RNG checkpoint after one plane, absent when that plane was not requested.
    #[must_use]
    pub fn level_rng(&self, level: Level) -> Option<RngCheckpoint> {
        self.workspace.level_rng[level.index()]
    }

    /// Recenter zones and paint island coverage in original zone order.
    /// Consuming this stage prevents applying its random coast displacement twice.
    ///
    /// # Errors
    /// Reports malformed zone ownership, an empty island polygon or raster failure.
    pub fn prepare_terrain(
        self,
        rng: &mut RetailRng,
    ) -> Result<TerrainCoverage<'a>, BoundaryError> {
        let behavior = self.behavior();
        self.workspace.prepare_coverage(self.water, behavior, rng)?;
        Ok(TerrainCoverage {
            boundaries: self,
            rng: rng.checkpoint(),
        })
    }
}

/// Zone centres, bounds and terrain coverage are ready for the terrain brush.
pub struct TerrainCoverage<'a> {
    boundaries: BoundaryMap<'a>,
    rng: RngCheckpoint,
}
impl<'a> TerrainCoverage<'a> {
    pub(crate) fn map_mut(&mut self) -> &mut BoundaryMap<'a> {
        &mut self.boundaries
    }
    /// Recentered zones and raster with completed island marks.
    #[must_use]
    pub const fn map(&self) -> &BoundaryMap<'_> {
        &self.boundaries
    }
    /// RNG state after island coast displacement and interior fill.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng
    }
}

#[derive(Clone, Copy, Default, Debug)]
struct CellSummary {
    bounds: Option<ZoneBounds>,
    total: Point,
    count: i32,
}

/// Flat, reusable storage for boundary generation, polygons and graph searches.
#[derive(Default, Debug)]
pub struct BoundaryWorkspace {
    zones: Vec<BoundaryZone>,
    connections: Vec<ZoneConnection>,
    polygons: Vec<(ZoneId, Point)>,
    sizes: Vec<u32>,
    distances: Vec<u16>,
    distance_columns: usize,
    work: Vec<(ZoneId, u16)>,
    diagram: Option<Delaunay>,
    grid: Option<ZoneRaster>,
    raster_work: RasterWorkspace,
    level_rng: [Option<RngCheckpoint>; raw::RMG_MAP_LEVEL_COUNT as usize],
    summaries: Vec<CellSummary>,
}
impl BoundaryWorkspace {
    /// Construct both planes' boundaries, extra water zones and connection graph.
    ///
    /// # Errors
    /// Reports arithmetic, topology, raster or reservation failures.
    #[expect(
        clippy::needless_pass_by_value,
        reason = "consume the completed layout token so its boundary stage cannot be repeated"
    )]
    pub fn generate<'a>(
        &'a mut self,
        completed: Layout<'a>,
        rng: &mut RetailRng,
    ) -> Result<BoundaryMap<'a>, BoundaryError> {
        let template = completed.template();
        let layout = completed.zones();
        let request = completed.request();
        let water = completed.water();
        self.zones.clear();
        self.connections.clear();
        self.polygons.clear();
        self.level_rng.fill(None);
        self.zones.try_reserve(layout.len())?;
        // Template zones take the first generated identities, in template
        // order; radial water zones are appended after them per level.
        for zone in layout {
            let source = template.zone(zone.id());
            self.zones.push(BoundaryZone {
                id: ZoneId::from(zone.id()),
                origin: ZoneOrigin::Template(zone.id()),
                position: zone.position(),
                template_size: source.size().get(),
                scaled_size: zone.scaled_size(),
                alignment: zone.alignment(),
                terrain: zone.terrain(),
                creatures: CreaturePreference::from_town(zone.creature_town()),
                bounds: None,
                primary_town: None,
            });
            self.connections.try_reserve(source.connections().len())?;
            for connection in source.connections() {
                self.connections.push(ZoneConnection {
                    source: ZoneId::from(zone.id()),
                    destination: ZoneId::from(connection.destination()),
                    value: connection.value(),
                    unguarded: connection.unguarded(),
                    border_guard: connection.border_guard(),
                    connected: false,
                });
            }
        }
        if let Some(grid) = &mut self.grid {
            grid.reset(request.size(), request.levels())?;
        } else {
            self.grid = Some(ZoneRaster::new(request.size(), request.levels())?);
        }
        for level in [Level::Surface, Level::Underground]
            .into_iter()
            .take(request.levels().count() as usize)
        {
            self.build_level(level, request, water, rng)?;
        }
        Ok(BoundaryMap {
            workspace: self,
            request,
            template,
            players: completed.players(),
            water,
        })
    }

    fn build_level(
        &mut self,
        level: Level,
        request: &Request,
        water: Water,
        rng: &mut RetailRng,
    ) -> Result<(), BoundaryError> {
        let mut diagram = match self.diagram.take() {
            Some(diagram) => diagram,
            None => Delaunay::new()?,
        };
        for zone in &self.zones {
            if zone.position.level == level {
                diagram.add_site(zone.position.point, Some(zone.id))?;
            }
        }
        let original_count = self.zones.len();
        if level == Level::Underground || water != Water::None {
            self.add_radial_sites(&mut diagram, original_count, level, request, rng)?;
        }
        let diagram = diagram.finish()?;
        self.sizes.clear();
        self.sizes.try_reserve(self.zones.len())?;
        self.sizes
            .extend(self.zones.iter().map(|zone| zone.scaled_size));
        let grid = self.grid.as_mut().ok_or(BoundaryError::Topology)?;
        let paint = level == Level::Underground || water != Water::Islands;
        for (index, zone) in self.zones.iter().enumerate() {
            if zone.position.level != level {
                continue;
            }
            // Duplicate sites retain their first owner in the C++ diagram.
            let owner = diagram
                .site_zone(zone.position.point)?
                .ok_or(BoundaryError::Topology)?;
            let polygon = self.raster_work.trace(
                grid,
                diagram.boundary(zone.position.point)?,
                owner,
                level,
                water,
                index < original_count && paint,
                &self.sizes,
                request.behavior(),
                rng,
            )?;
            self.polygons.try_reserve(polygon.len())?;
            self.polygons
                .extend(polygon.iter().map(|&point| (owner, point)));
        }
        for zone in &self.zones {
            if zone.position.level == level {
                self.raster_work.fill_zone(
                    grid,
                    zone.position.point,
                    level,
                    zone.id,
                    paint,
                    diagram.boundary(zone.position.point)?,
                )?;
            }
        }
        self.join_extra_zones(original_count, &diagram)?;
        self.level_rng[level.index()] = Some(rng.checkpoint());
        self.diagram = Some(diagram.recycle()?);
        Ok(())
    }

    fn add_radial_sites(
        &mut self,
        diagram: &mut Delaunay,
        original_count: usize,
        level: Level,
        request: &Request,
        rng: &mut RetailRng,
    ) -> Result<(), BoundaryError> {
        // Retail 0x53e149 passes unwritten stack town flags to 0x5329e0.
        // Nonperturbing pinned-executable captures observe nonempty live masks
        // and one draw on both levels, including initial stack fills 0 and -1.
        // The selected probe alignment is unused; hotfix explicitly allows none.
        if !request.behavior().is_hotfix() {
            rng.draw();
        }
        let extent =
            i32::try_from(request.size().dimension()).map_err(|_| BoundaryError::Arithmetic)?;
        for index in 0..original_count {
            let current = self.zones[index];
            if current.position.level != level {
                continue;
            }
            let radius = current.scaled_size;
            for direction in (0..raw::RADIAL_COSINE_BITS.len()).step_by(4) {
                let x_offset =
                    f64::from(radius) * f64::from_bits(raw::RADIAL_COSINE_BITS[direction]);
                let y_offset = f64::from(radius) * f64::from_bits(raw::RADIAL_SINE_BITS[direction]);
                let point = Point::new(
                    radial_coordinate(current.position.point.x, x_offset)?,
                    radial_coordinate(current.position.point.y, y_offset)?,
                );
                if too_far(point.x, extent, x_offset) || too_far(point.y, extent, y_offset) {
                    continue;
                }
                let mut allowed = true;
                for other in &self.zones {
                    if other.position.level != level {
                        continue;
                    }
                    let distance = point.distance(other.position.point)?;
                    let combined = i32::try_from(
                        radius
                            .checked_add(other.template_size)
                            .ok_or(BoundaryError::Arithmetic)?,
                    )
                    .map_err(|_| BoundaryError::Arithmetic)?;
                    if distance.checked_mul(10).ok_or(BoundaryError::Arithmetic)?
                        < combined.checked_mul(8).ok_or(BoundaryError::Arithmetic)?
                    {
                        allowed = false;
                        break;
                    }
                }
                if !allowed {
                    continue;
                }
                let owner = if level == Level::Surface {
                    let id = ZoneId::new(self.zones.len());
                    let alignment = match request.behavior() {
                        Behavior::Hotfix => None,
                        Behavior::Retail(profile) => {
                            select_water_town(profile.water_zone_town_mask(), rng)
                        }
                    };
                    self.zones.try_reserve(1)?;
                    self.zones.push(BoundaryZone {
                        id,
                        origin: ZoneOrigin::Water,
                        position: WorldPosition { point, level },
                        template_size: radius,
                        scaled_size: radius,
                        alignment,
                        terrain: Terrain::Water,
                        creatures: CreaturePreference::water(request.behavior()),
                        bounds: None,
                        primary_town: None,
                    });
                    Some(id)
                } else {
                    None
                };
                diagram.add_site(point, owner)?;
            }
        }
        Ok(())
    }

    fn add_connection(
        &mut self,
        source: ZoneId,
        destination: ZoneId,
        connected: bool,
    ) -> Result<(), BoundaryError> {
        self.connections.try_reserve(2)?;
        for (source, destination) in [(source, destination), (destination, source)] {
            self.connections.push(ZoneConnection {
                source,
                destination,
                value: 0,
                unguarded: true,
                border_guard: false,
                connected,
            });
        }
        Ok(())
    }

    fn join_extra_zones(
        &mut self,
        original_count: usize,
        diagram: &Voronoi,
    ) -> Result<(), BoundaryError> {
        let bounds = self.grid.as_ref().ok_or(BoundaryError::Topology)?.bounds();
        for index in original_count..self.zones.len() {
            let zone = self.zones[index];
            for other in index + 1..self.zones.len() {
                let destination = self.zones[other];
                if destination.position.level != zone.position.level {
                    continue;
                }
                if let Some((point, previous)) =
                    boundary_with(diagram, zone.position.point, destination.id)?
                {
                    if bounds.contains(bounds.clip(point, previous)?) {
                        self.add_connection(zone.id, destination.id, true)?;
                    }
                }
            }
        }
        self.distance_columns = original_count;
        let count = self
            .zones
            .len()
            .checked_mul(original_count)
            .ok_or(BoundaryError::Arithmetic)?;
        self.distances
            .try_reserve(count.saturating_sub(self.distances.len()))?;
        let unreached =
            u16::try_from(raw::RMG_UNREACHED_COST).map_err(|_| BoundaryError::Arithmetic)?;
        self.distances.resize(count, unreached);
        self.distances.fill(unreached);
        for index in 0..original_count {
            self.distances[index * original_count + index] = 0;
        }
        for index in 0..original_count {
            self.propagate(ZoneId::new(index))?;
        }
        for index in original_count..self.zones.len() {
            let zone = self.zones[index];
            for other in 0..original_count {
                let destination = self.zones[other];
                if destination.position.level != zone.position.level
                    || boundary_with(diagram, zone.position.point, destination.id)?.is_none()
                {
                    continue;
                }
                let shortens = (0..original_count).any(|column| {
                    column != other
                        && self.distances[other * original_count + column]
                            > self.distances[index * original_count + column] + 1
                });
                if !shortens {
                    self.add_connection(zone.id, destination.id, false)?;
                    self.propagate(destination.id)?;
                }
            }
        }
        Ok(())
    }

    fn propagate(&mut self, start: ZoneId) -> Result<(), BoundaryError> {
        let columns = self.distance_columns;
        for column in 0..columns {
            self.work.clear();
            self.work.try_reserve(1)?;
            self.work.push((start, 0));
            while let Some((current, _)) = self.work.pop() {
                let distance = self.distances[current.index() * columns + column] + 1;
                for connection in &self.connections {
                    if connection.source != current {
                        continue;
                    }
                    let target = connection.destination;
                    let slot = self
                        .distances
                        .get_mut(target.index() * columns + column)
                        .ok_or(BoundaryError::Topology)?;
                    if *slot > distance {
                        *slot = distance;
                        // Descending costs, newest equal cost inserted first;
                        // popping the back therefore processes older ties first.
                        let index = self.work.partition_point(|&(_, cost)| cost > distance);
                        self.work.try_reserve(1)?;
                        self.work.insert(index, (target, distance));
                    }
                }
            }
        }
        Ok(())
    }

    fn prepare_coverage(
        &mut self,
        water: Water,
        behavior: Behavior,
        rng: &mut RetailRng,
    ) -> Result<(), BoundaryError> {
        self.summaries.clear();
        self.summaries.try_reserve(self.zones.len())?;
        self.summaries
            .resize(self.zones.len(), CellSummary::default());
        let grid = self.grid.as_mut().ok_or(BoundaryError::Topology)?;
        let dimension = grid.dimension();
        // One pass computes the same bounds and coordinate sums as the C++
        // per-zone scans. This work consumes no randomness and is independent
        // of the island marks written afterwards.
        for (index, cell) in grid.cells().iter().enumerate() {
            let Some(owner) = cell.zone else {
                continue;
            };
            let zone = self
                .zones
                .get(owner.index())
                .ok_or(BoundaryError::Topology)?;
            let summary = &mut self.summaries[owner.index()];
            let point = Point::new(
                i32::try_from(index % dimension).map_err(|_| BoundaryError::Arithmetic)?,
                i32::try_from(index / dimension % dimension)
                    .map_err(|_| BoundaryError::Arithmetic)?,
            );
            if let Some(bounds) = &mut summary.bounds {
                bounds.include(point);
            } else {
                summary.bounds = Some(ZoneBounds::cell(point));
            }
            if index / (dimension * dimension) == zone.position.level.index() {
                summary.total.x += point.x;
                summary.total.y += point.y;
                summary.count += 1;
            }
        }
        for (zone, summary) in self.zones.iter_mut().zip(&self.summaries) {
            zone.bounds = summary.bounds;
            if summary.count > 0 {
                zone.position.point = Point::new(
                    summary.total.x / summary.count,
                    summary.total.y / summary.count,
                );
            }
            if water == Water::Islands && zone.position.level == Level::Surface {
                let polygon = self
                    .polygons
                    .iter()
                    .filter_map(|&(owner, point)| (owner == zone.id).then_some(point));
                self.raster_work.inset_island(
                    grid,
                    zone.position.point,
                    zone.id,
                    polygon,
                    zone.scaled_size,
                    behavior,
                    rng,
                )?;
            }
        }
        Ok(())
    }
}

fn boundary_with(
    diagram: &Voronoi,
    site: Point,
    destination: ZoneId,
) -> Result<Option<(Point, Point)>, BoundaryError> {
    let mut ring = diagram.boundary(site)?;
    let first = ring.next().ok_or(BoundaryError::Topology)?;
    let mut previous = first;
    // Search the closed ring starting after the first edge, testing it last.
    for edge in ring.chain(std::iter::once(first)) {
        if edge.opposite_zone == Some(destination) {
            return Ok(Some((
                edge.vertex.ok_or(BoundaryError::Topology)?,
                previous.vertex.ok_or(BoundaryError::Topology)?,
            )));
        }
        previous = edge;
    }
    Ok(None)
}

fn too_far(coordinate: i32, extent: i32, half_offset: f64) -> bool {
    (coordinate < 0 && f64::from(coordinate) < half_offset)
        || (coordinate >= extent && f64::from(coordinate) >= f64::from(extent) + half_offset)
}
fn radial_coordinate(center: i32, half_offset: f64) -> Result<i32, BoundaryError> {
    let value = f64::from(center) + half_offset * 2.0;
    if !value.is_finite()
        || value.trunc() < f64::from(i32::MIN)
        || value.trunc() > f64::from(i32::MAX)
    {
        return Err(BoundaryError::Arithmetic);
    }
    #[allow(clippy::cast_possible_truncation)]
    Ok(value as i32)
}
fn select_water_town(mask: TownMask, rng: &mut RetailRng) -> Option<Town> {
    // 0x532a47 counts nonzero flags; 0x532a59 skips rand only when none are set.
    // The heap-filled masks reaching 0x53e45c remain zero under the zero profile.
    mask.choose(rng)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn water_town_selection_preserves_empty_singleton_and_sparse_draws() {
        let mut rng = RetailRng::new(350_484_818);
        assert_eq!(select_water_town(TownMask::NONE, &mut rng), None);
        assert_eq!(
            rng.checkpoint(),
            RngCheckpoint {
                state: 350_484_818,
                draws: 0
            }
        );
        assert_eq!(
            select_water_town(TownMask::parse(1 << 8).unwrap(), &mut rng),
            Some(Town::parse(8).unwrap())
        );
        assert_eq!(
            rng.checkpoint(),
            RngCheckpoint {
                state: 1_001_028_301,
                draws: 1
            }
        );

        // Captured all-town water constructors, heap fills 91 and 255.
        let mut rng = RetailRng::new(350_484_818);
        for town in [1, 5, 5, 5, 1, 3, 8, 4, 4, 0, 3, 0, 0, 4, 8] {
            assert_eq!(
                select_water_town(TownMask::ALL, &mut rng),
                Some(Town::parse(town).unwrap())
            );
        }
        assert_eq!(
            rng.checkpoint(),
            RngCheckpoint {
                state: 4_030_193_355,
                draws: 15
            }
        );
        // A sparse mask selects by rank, not by reducing to the largest index.
        let mut rng = RetailRng::new(1);
        let sparse = TownMask::parse((1 << 2) | (1 << 7)).unwrap();
        assert_eq!(
            select_water_town(sparse, &mut rng),
            Some(Town::parse(7).unwrap())
        );
        assert_eq!(rng.draws(), 1);
    }
}
