// Random-map line painting and Voronoi helpers.

#include "va.h"

#include <algorithm>
#include <math.h>

#include "exceptions.h"
#include "rmg.h"
#include "rmg_terrain.h"
#include "tiles.h"

TRmgLinePainterInterface::TRmgLinePainterInterface(const TRmgGridPoint& size)
    : m_size(size)
{
}

VA(0x004f9be0, 0xb7)
MAC_ADDRESS(0x22210c, 0x1ec)
TRmgLinePatternTable::TRmgLinePatternTable(u32 frameCount, const s32* framePatterns)
    : m_frameCount(frameCount), m_framePatterns(0)
{
    s32* allocated = new s32[m_frameCount];
    m_framePatterns = allocated;
    if (!allocated)
        throw TAllocationFailure();
    std::copy(framePatterns, framePatterns + m_frameCount, m_framePatterns);
    for (u32 pattern = LINE_END_S; pattern < LINE_PATTERN_COUNT; ++pattern) {
        m_ranges[pattern].m_firstIndex = 0;
        m_ranges[pattern].m_frameCount = 0;
    }
    // Expects a nonempty, unvalidated list of ids 0..8 with each id's frames
    // contiguous.
    s32 runPattern = m_framePatterns[0];
    ++m_ranges[runPattern].m_frameCount;
    for (u32 index = 1; index < m_frameCount; ++index) {
        if (m_framePatterns[index] != runPattern) {
            runPattern = m_framePatterns[index];
            m_ranges[runPattern].m_firstIndex = index;
        }
        ++m_ranges[runPattern].m_frameCount;
    }
}

VA(0x004f9ca0, 0x0b)
MAC_ADDRESS(0x2222f8, 0x54)
TRmgLinePatternTable::~TRmgLinePatternTable()
{
    delete[] m_framePatterns;
}

// Neighbour direction order for each (flipX, flipY) reflection; the same
// values as g_rmgReflectedNeighbours. A canonical pattern's direction d reads
// neighbour order[d]. North is up; each grid puts order[d] at d.
//   none      flipY     flipX     both
//   NW N NE   SW S SE   NE N NW   SE S SW
//   W  .  E   W  .  E   E  .  W   E  .  W
//   SW S SE   NW N NE   SE S SW   NE N NW
DATA(0x0063fe9c)
static const s32 g_rmgLineReflectedNeighbours[2][2][8] = {
    {
        {TILE_DIR_NORTH, TILE_DIR_NORTHEAST, TILE_DIR_EAST, TILE_DIR_SOUTHEAST,
         TILE_DIR_SOUTH, TILE_DIR_SOUTHWEST, TILE_DIR_WEST, TILE_DIR_NORTHWEST}, // none
        {TILE_DIR_SOUTH, TILE_DIR_SOUTHEAST, TILE_DIR_EAST, TILE_DIR_NORTHEAST,
         TILE_DIR_NORTH, TILE_DIR_NORTHWEST, TILE_DIR_WEST, TILE_DIR_SOUTHWEST} // flipY
    },
    {
        {TILE_DIR_NORTH, TILE_DIR_NORTHWEST, TILE_DIR_WEST, TILE_DIR_SOUTHWEST,
         TILE_DIR_SOUTH, TILE_DIR_SOUTHEAST, TILE_DIR_EAST, TILE_DIR_NORTHEAST}, // flipX
        {TILE_DIR_SOUTH, TILE_DIR_SOUTHWEST, TILE_DIR_WEST, TILE_DIR_NORTHWEST,
         TILE_DIR_NORTH, TILE_DIR_NORTHEAST, TILE_DIR_EAST, TILE_DIR_SOUTHEAST} // both
    }
};

// Reflections tried for corner shapes, as {flipX, flipY}: none, flipY, flipX,
// then both.
DATA(0x0063ff1c)
static const b8 g_rmgLineReflections[4][2] = {
    {false, false}, {false, true}, {true, false}, {true, true}
};

static inline bool hasRmgHorizontalLineNeighbour(const b8* neighbours)
{
    return neighbours[TILE_DIR_WEST] || neighbours[TILE_DIR_EAST];
}

VA(0x004f9cb0, 0x24e)
MAC_ADDRESS(0x222498, 0x2a4)
void selectRmgLinePattern(
    const b8* neighbours, const TRmgLinePatternTable* table,
    s32& pattern, b8& flipX, b8& flipY)
{
    if (neighbours[TILE_DIR_NORTH] && neighbours[TILE_DIR_EAST]
        && neighbours[TILE_DIR_SOUTH] && neighbours[TILE_DIR_WEST]) {
        pattern = LINE_CROSS;
        flipX = false;
        flipY = false;
        return;
    }
    if (neighbours[TILE_DIR_NORTH] && neighbours[TILE_DIR_SOUTH]) {
        if (neighbours[TILE_DIR_EAST]) {
            pattern = LINE_NES;
            flipX = false;
        } else if (neighbours[TILE_DIR_WEST]) {
            pattern = LINE_NES;
            flipX = true;
        } else {
            pattern = LINE_NS;
            flipX = false;
        }
        flipY = false;
        return;
    }
    if (neighbours[TILE_DIR_EAST] && neighbours[TILE_DIR_WEST]) {
        if (neighbours[TILE_DIR_SOUTH]) {
            pattern = LINE_ESW;
            flipY = false;
        } else if (neighbours[TILE_DIR_NORTH]) {
            pattern = LINE_ESW;
            flipY = true;
        } else {
            pattern = LINE_EW;
            flipY = false;
        }
        flipX = false;
        return;
    }
    b8 hasCornerVariant = table->m_ranges[LINE_SE_VARIANT].m_frameCount > 0;
    const u32 reflectionCount = sizeof(g_rmgLineReflections) / sizeof(g_rmgLineReflections[0]);
    for (u32 reflection = 0; reflection < reflectionCount; ++reflection) {
        const s32* order = g_rmgLineReflectedNeighbours
            [g_rmgLineReflections[reflection][0]][g_rmgLineReflections[reflection][1]];
        if (neighbours[order[TILE_DIR_EAST]] && neighbours[order[TILE_DIR_SOUTH]]) {
            if (hasCornerVariant && (neighbours[order[TILE_DIR_NORTHEAST]]
                || neighbours[order[TILE_DIR_SOUTHWEST]]))
                pattern = LINE_SE_VARIANT;
            else
                pattern = LINE_SE;
            flipX = g_rmgLineReflections[reflection][0];
            flipY = g_rmgLineReflections[reflection][1];
            return;
        }
    }
    if (table->m_ranges[LINE_END_S].m_frameCount > 0) {
        if (hasRmgHorizontalLineNeighbour(neighbours)) {
            pattern = LINE_END_E;
            flipX = neighbours[TILE_DIR_WEST];
            flipY = false;
        } else {
            pattern = LINE_END_S;
            flipX = false;
            flipY = !neighbours[TILE_DIR_SOUTH];
        }
    } else {
        pattern = hasRmgHorizontalLineNeighbour(neighbours)
            ? LINE_EW : LINE_NS;
        flipX = false;
        flipY = false;
    }
}

VA(0x0055eda0, 0x07)
MAC_ADDRESS(0x253ccc, 0x60)
TRmgRiverPainter::~TRmgRiverPainter()
{
}

// All river types use the same pattern table, so the argument is ignored.
VA(0x0055edb0, 0x08)
MAC_ADDRESS(0x253ad8, 0x8)
TRmgLinePatternTable* TRmgRiverLinePainter::getPattern(s32)
{
    return &g_rmgRiverPatternTable;
}

template<class Adapter>
static inline void writeRmgLineTileSnapshot(Adapter* adapter,
    const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    rmgTerrainTile snapshot(tile.m_terrain, tile.m_frame);
    snapshot.m_flipX = tile.m_flipX;
    snapshot.m_flipY = tile.m_flipY;
    adapter->setTile(point, snapshot);
}

template<class Adapter>
static inline void readRmgLineTileSnapshot(Adapter* adapter,
    const TRmgGridPoint& point, rmgTerrainTile& tile)
{
    rmgTerrainTile snapshot = adapter->getTile(point);
    tile = snapshot;
}

// Roads and rivers cannot be painted over water or rock terrain.
template<class Adapter>
static inline b32 isRmgLinePaintingBlocked(Adapter* adapter,
    const TRmgGridPoint& point)
{
    s32 terrain = adapter->getTerrain(point);
    return terrain == eTerrainWater || terrain == eTerrainRock;
}

VA(0x0055edc0, 0x36)
MAC_ADDRESS(0x253ae0, 0x54)
void TRmgRiverLinePainter::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    writeRmgLineTileSnapshot(m_adapter, point, tile);
}

MAC_ADDRESS(0x253b34, 0x30)
void TRmgRiverLinePainter::setLineType(const TRmgGridPoint& point, s32 value)
{
    m_adapter->setLineType(point, value);
}

MAC_ADDRESS(0x253ba8, 0x88)
void TRmgRiverLinePainter::getTile(const TRmgGridPoint& point, rmgTerrainTile& tile)
{
    readRmgLineTileSnapshot(m_adapter, point, tile);
}

VA(0x0055ee00, 0x28)
MAC_ADDRESS(0x253b64, 0x44)
b32 TRmgRiverLinePainter::isBlocked(const TRmgGridPoint& point)
{
    return isRmgLinePaintingBlocked(m_adapter, point);
}

VA(0x0055ee30, 0x13)
MAC_ADDRESS(0x253c30, 0x30)
s32 TRmgRiverLinePainter::getLineType(const TRmgGridPoint& point)
{
    return m_adapter->getLineType(point);
}

VA(0x0055ee50, 0x76)
MAC_ADDRESS(0x253c60, 0x6c)
TRmgRiverPainter::TRmgRiverPainter(
    TRmgRiverMapAdapterInterface* newAdapter,
    s32 newRiverType,
    const TRmgGridPoint& newStart)
    : TRmgRiverLinePainter(newAdapter),
      TRmgLineWalker(this, newRiverType, newStart)
{
}

VA_COMPGEN(0x0055eed0, 0x21, SCALAR_DELETING_DTOR, TRmgRiverPainter)

VA(0x0055f320, 0x08)
MAC_ADDRESS(0x253fc0, 0x8)
TRmgLinePatternTable* TRmgRoadLinePainter::getPattern(s32)
{
    return &g_rmgRoadPatternTable;
}

MAC_ADDRESS(0x253fc8, 0x54)
void TRmgRoadLinePainter::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    writeRmgLineTileSnapshot(m_adapter, point, tile);
}

MAC_ADDRESS(0x25404c, 0x44)
b32 TRmgRoadLinePainter::isBlocked(const TRmgGridPoint& point)
{
    return isRmgLinePaintingBlocked(m_adapter, point);
}

VA(0x0055f330, 0x17)
MAC_ADDRESS(0x25401c, 0x30)
void TRmgRoadLinePainter::setLineType(const TRmgGridPoint& point, s32 value)
{
    m_adapter->setLineType(point, value);
}

VA(0x0055f350, 0x34)
MAC_ADDRESS(0x254090, 0x88)
void TRmgRoadLinePainter::getTile(const TRmgGridPoint& point, rmgTerrainTile& tile)
{
    readRmgLineTileSnapshot(m_adapter, point, tile);
}

VA(0x0055f390, 0x13)
MAC_ADDRESS(0x254118, 0x30)
s32 TRmgRoadLinePainter::getLineType(const TRmgGridPoint& point)
{
    return m_adapter->getLineType(point);
}

VA(0x0055f3b0, 0x76)
MAC_ADDRESS(0x254148, 0x6c)
TRmgRoadPainter::TRmgRoadPainter(
    TRmgRoadMapAdapterInterface* newAdapter,
    s32 newRoadType,
    const TRmgGridPoint& newStart)
    : TRmgRoadLinePainter(newAdapter),
      TRmgLineWalker(this, newRoadType, newStart)
{
}

VA_COMPGEN(0x0055f430, 0x21, SCALAR_DELETING_DTOR, TRmgRoadPainter)

VA(0x0055f460, 0x07)
MAC_ADDRESS(0x2541b4, 0x60)
TRmgRoadPainter::~TRmgRoadPainter()
{
}

VA(0x005fceb0, 0x39)
MAC_ADDRESS(0x25c018, 0x64)
s32 TRmgVector::length() const
{
    // The squared norm is 32-bit and may overflow; the root is truncated.
    return static_cast<s32>(sqrt(static_cast<double>(getRmgSquaredNorm(m_x, m_y))));
}

MAC_ADDRESS(0x25c164, 0x20)
void TRmgHalfEdge::initialize()
{
    m_next = this;
    m_previous = this;
    m_positionComputed = false;
    m_position.m_x = -1;
    m_position.m_y = -1;
}

TRmgHalfEdge::TRmgHalfEdge(
    TPoint sitePosition, TRmgZone* zone, TRmgHalfEdge* twin)
{
    m_sitePosition = sitePosition;
    m_zone = zone;
    m_twin = twin;
    initialize();
}

VA(0x005fcef0, 0x6c)
MAC_ADDRESS(0x25c07c, 0x98)
TRmgHalfEdge::TRmgHalfEdge(
    TPoint sitePosition, TRmgZone* zone, TPoint twinSitePosition, TRmgZone* twinZone)
    : m_zone(zone)
{
    m_sitePosition = sitePosition;
    m_twin = new TRmgHalfEdge(twinSitePosition, twinZone, this);
    initialize();
}

VA(0x005fcf60, 0x31)
MAC_ADDRESS(0x25c184, 0x34)
void TRmgHalfEdge::splice(TRmgHalfEdge* other)
{
    std::swap(m_next->m_previous, other->m_next->m_previous);
    std::swap(m_next, other->m_next);
}

VA(0x005fcfa0, 0x61)
MAC_ADDRESS(0x25c1b8, 0x4c)
void TRmgHalfEdge::detach()
{
    TRmgHalfEdge* previous = m_previous;
    TRmgHalfEdge* twinPrevious = getLeftNext();
    splice(previous);
    m_twin->splice(twinPrevious);
}

// Starts as a large square split by a diagonal. North is up (y down); 1-4 are
// firstEdge..fourthEdge and 5 the connectEdges diagonal from first to third.
//   first --1--> second
//     ^  \         |
//     4    5       2
//     |      \     v
//   fourth <-3-- third
VA(0x005fd010, 0x316)
MAC_ADDRESS(0x25c4e4, 0x14c)
TRmgVoronoi::TRmgVoronoi()
{
    TPoint first(-200, -200);
    TPoint second(400, -200);
    TPoint third(400, 400);
    TPoint fourth(-200, 400);
    TRmgHalfEdge* firstEdge = createEdge(first, 0, second, 0);
    TRmgHalfEdge* secondEdge = createEdge(second, 0, third, 0);
    TRmgHalfEdge* thirdEdge = createEdge(third, 0, fourth, 0);
    TRmgHalfEdge* fourthEdge = createEdge(fourth, 0, first, 0);
    firstEdge->getTwin()->splice(secondEdge);
    secondEdge->getTwin()->splice(thirdEdge);
    thirdEdge->getTwin()->splice(fourthEdge);
    fourthEdge->getTwin()->splice(firstEdge);
    connectEdges(fourthEdge, thirdEdge);
    m_root = firstEdge;
}

VA(0x005fd330, 0x58)
MAC_ADDRESS(0x25c6b4, 0xa4)
TRmgVoronoi::~TRmgVoronoi()
{
    for (s32 edge = 0; edge < m_edges.size(); ++edge)
        delete m_edges[edge];
}

VA(0x005fd390, 0x21c)
MAC_ADDRESS(0x25c758, 0x170)
TRmgHalfEdge* TRmgVoronoi::createEdge(TPoint first, TRmgZone* firstZone,
    TPoint second, TRmgZone* secondZone)
{
    TRmgHalfEdge* edge = new TRmgHalfEdge(first, firstZone, second, secondZone);
    m_edges.push_back(edge);
    m_edges.push_back(edge->getTwin());
    return edge;
}

// Quad-edge Connect(a, b): connect first's destination to second's origin,
// then splice into first's left face and second's origin ring.
MAC_ADDRESS(0x25caec, 0x94)
TRmgHalfEdge* TRmgVoronoi::connectEdges(TRmgHalfEdge* first,
    TRmgHalfEdge* second)
{
    TRmgHalfEdge* edge = createEdge(first->getOppositeSitePosition(),
        first->getOppositeZone(), second->getSitePosition(), second->getZone());
    edge->splice(first->getLeftNext());
    edge->getTwin()->splice(second);
    return edge;
}

// Removes one half-edge reference without destroying it. The edge must be
// present: a failed search still erases end().
static inline void eraseRmgHalfEdgeReference(std::vector<TRmgHalfEdge*>& edges,
    TRmgHalfEdge* edge)
{
    u32 index = 0;
    while (index < edges.size() && edges[index] != edge)
        ++index;
    edges.erase(edges.begin() + index);
}

VA(0x005fd5b0, 0xff)
MAC_ADDRESS(0x25c8c8, 0x100)
void TRmgVoronoi::removeEdge(TRmgHalfEdge* edge)
{
    edge->detach();
    eraseRmgHalfEdgeReference(m_edges, edge);
    TRmgHalfEdge* twin = edge->getTwin();
    eraseRmgHalfEdgeReference(m_edges, twin);
    delete edge;
    delete twin;
}

// Integer forms of Graphics Gems IV's ccw/RightOf predicates; see
// docs/reference/rmg-voronoi-provenance.md.
MAC_ADDRESS(0x25c304, 0x68)
static s32 isRmgCounterClockwise(TPoint first, TPoint second, TPoint third)
{
    return getRmgPointOrientation(first, second, third) > 0;
}

static s32 isRmgPointRightOfEdge(TPoint point, TRmgHalfEdge* edge)
{
    TRmgHalfEdge* twin = edge->m_twin;
    return isRmgCounterClockwise(edge->m_sitePosition, point, twin->m_sitePosition);
}

static inline bool isRmgEdgeOrigin(TPoint point, TRmgHalfEdge* edge)
{
    TPoint origin = edge->getSitePosition();
    return point == origin;
}

static inline bool isRmgEdgeDestination(TPoint point, TRmgHalfEdge* edge)
{
    TPoint destination = edge->getOppositeSitePosition();
    return point == destination;
}

VA(0x005fd6b0, 0xd7)
MAC_ADDRESS(0x25c9c8, 0x124)
TRmgHalfEdge* TRmgVoronoi::locate(TPoint point)
{
    // Walking point location in the incremental Delaunay triangulation.
    // There is no iteration limit or outside-domain fallback.
    TRmgHalfEdge* edge = m_root;
    for (;;) {
        if (isRmgEdgeOrigin(point, edge))
            return edge;
        if (isRmgEdgeDestination(point, edge))
            return edge->m_twin;
        if (isRmgPointRightOfEdge(point, edge)) {
            edge = edge->m_twin;
        } else if (!isRmgPointRightOfEdge(point, edge->m_next)) {
            edge = edge->m_next;
        } else {
            TRmgHalfEdge* destinationPrevious = edge->getLeftNext()->getTwin();
            if (isRmgPointRightOfEdge(point, destinationPrevious))
                return edge;
            edge = destinationPrevious;
        }
    }
}

// A flipped half-edge takes both the site and its zone from the opposite
// endpoint of its saved predecessor.
static inline void copyRmgOppositeSite(TRmgHalfEdge* destination,
    TRmgHalfEdge* source)
{
    destination->m_zone = source->getOppositeZone();
    destination->m_sitePosition = source->getOppositeSitePosition();
}

// Delaunay edge flip: save both predecessors, detach, take their opposite
// sites/zones, and splice into the new rings.
MAC_ADDRESS(0x25c204, 0xb0)
static void flipRmgEdge(TRmgHalfEdge* edge)
{
    TRmgHalfEdge* previous = edge->m_previous;
    TRmgHalfEdge* twinPrevious = edge->getLeftNext();
    edge->detach();
    copyRmgOppositeSite(edge, previous);
    copyRmgOppositeSite(edge->m_twin, twinPrevious);
    edge->splice(previous->getLeftNext());
    edge->m_twin->splice(twinPrevious->getLeftNext());
}

// True when the point lies on the edge's segment: within both endpoint
// distances and exactly on its line.
static b8 isRmgPointOnSegment(TPoint point, TRmgHalfEdge* edge)
{
    TPoint opposite = edge->getOppositeSitePosition();
    s32 distanceToOriginSquared = getRmgSquaredDistance(point, edge->getSitePosition());
    s32 distanceToDestinationSquared = getRmgSquaredDistance(point, opposite);
    s32 edgeLengthSquared = getRmgSquaredDistance(edge->getSitePosition(), opposite);
    if (distanceToOriginSquared > edgeLengthSquared
        || distanceToDestinationSquared > edgeLengthSquared)
        return false;
    const TPoint& origin = edge->m_sitePosition;
    s32 deltaX = opposite.m_x - origin.m_x;
    s32 deltaY = opposite.m_y - origin.m_y;
    s32 lineConstant = -(deltaY * origin.m_x - deltaX * origin.m_y);
    return deltaY * point.m_x - deltaX * point.m_y + lineConstant == 0;
}

// Incircle predicate for a counterclockwise triangle; zero (cocircular)
// does not request an edge flip.
MAC_ADDRESS(0x25cb80, 0x1c4)
static b8 isRmgPointInsideCircumcircle(TPoint first, TPoint second,
    TPoint third, TPoint point)
{
    // Squared norms and orientations are 32-bit before the 64-bit products,
    // so large coordinates overflow.
    s32 firstArea = getRmgPointOrientation(second, third, point);
    s32 secondArea = getRmgPointOrientation(first, third, point);
    s32 thirdArea = getRmgPointOrientation(first, second, point);
    s32 pointArea = getRmgPointOrientation(first, second, third);
    s64 determinant = static_cast<s64>(getRmgSquaredNorm(third.m_x, third.m_y)) * thirdArea
        - static_cast<s64>(getRmgSquaredNorm(second.m_x, second.m_y)) * secondArea
        + static_cast<s64>(getRmgSquaredNorm(first.m_x, first.m_y)) * firstArea
        - static_cast<s64>(getRmgSquaredNorm(point.m_x, point.m_y)) * pointArea;
    return determinant > 0;
}

VA(0x005fd790, 0x348)
MAC_ADDRESS(0x25cd44, 0x218)
void TRmgVoronoi::addSite(TPoint point, TRmgZone* zone)
{
    // Incremental Delaunay insertion: locate, split an existing edge when
    // necessary, build a triangle fan, then legalize it by local edge flips.
    // The Voronoi diagram is the dual built later by buildVertices().
    TRmgHalfEdge* edge = locate(point);
    if (isRmgEdgeOrigin(point, edge))
        return;
    if (isRmgEdgeDestination(point, edge))
        return;
    if (isRmgPointOnSegment(point, edge)) {
        edge = edge->getPrevious();
        removeEdge(edge->getNext());
    }
    TRmgHalfEdge* fanBase = createEdge(edge->getSitePosition(), edge->getZone(), point, zone);
    fanBase->splice(edge);
    m_root = fanBase;
    do {
        fanBase = connectEdges(edge, fanBase->getTwin());
        edge = fanBase->getPrevious();
    } while (edge->getLeftNext() != m_root);

    for (;;) {
        TRmgHalfEdge* previous = edge->getPrevious();
        if (isRmgPointRightOfEdge(previous->getOppositeSitePosition(), edge)
            && isRmgPointInsideCircumcircle(edge->getSitePosition(),
                previous->getOppositeSitePosition(), edge->getOppositeSitePosition(), point)) {
            flipRmgEdge(edge);
            edge = edge->getPrevious();
        } else if (edge->getNext() == m_root) {
            return;
        } else {
            edge = edge->getNext()->getLeftPrevious();
        }
    }
}

VA(0x005fdae0, 0x2b)
MAC_ADDRESS(0x25c2b4, 0x50)
s32 getRmgPointOrientation(TPoint first, TPoint second, TPoint third)
{
    // Signed twice-area; positive means counterclockwise.
    return (second.m_x - first.m_x) * (third.m_y - first.m_y)
        - (second.m_y - first.m_y) * (third.m_x - first.m_x);
}

VA_COMPGEN(0x005fdd60, 0x1b1, VECTOR_INSERT_SINGLE, TRmgHalfEdge)

VA_COMPGEN(0x005fdf20, 0x26, VECTOR_UFILL, TRmgHalfEdge)
