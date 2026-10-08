// rmg_support.cpp - retained Complete random-map helper bodies.

// Retail keeps these ordinary helpers out of line in CreateRiver.  Their
// declarations remain visible through rmg.h, while placing the definitions in
// this companion translation unit reproduces the natural body-visibility
// boundary without source-false inline controls.
#include "va.h"

#include <algorithm>
#include <math.h>

#include "exceptions.h"
#include "rmg.h"
#include "rmg_terrain.h"
#include "tiles.h"



// The common painter prefix owns only the two dimensions and virtual API.
// Both retained final constructors obtain the adapter size before building
// this base, then store their own adapter at +0xc. Keep one ordinary helper.
TMapLineFilter::TMapLineFilter(const TRmgGridPoint& size)
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
    for (u32 value = LINE_END_S; value < LINE_PATTERN_COUNT; ++value) {
        m_ranges[value].m_firstFrame = 0;
        m_ranges[value].m_frameCount = 0;
    }
    // Each pattern occupies one contiguous frame range.
    s32 runPattern = m_framePatterns[0];
    ++m_ranges[runPattern].m_frameCount;
    for (u32 index = 1; index < m_frameCount; ++index) {
        if (m_framePatterns[index] != runPattern) {
            runPattern = m_framePatterns[index];
            m_ranges[runPattern].m_firstFrame = index;
        }
        ++m_ranges[runPattern].m_frameCount;
    }
}

// Both table cleanup thunks tail-call this body. The paired constructor owns
// only the copied pattern-id array at +4; the nine index/count pairs are plain
// integers and need no cleanup.
VA(0x004f9ca0, 0x0b)
MAC_ADDRESS(0x2222f8, 0x54)  // cinit cleanups 0x55ed90/0x55f310; Complete-only
TRmgLinePatternTable::~TRmgLinePatternTable()
{
    delete[] m_framePatterns;
}

// This library-side copy is retained separately from the terrain selector's
// 0x642c00 table. Retail 0x4f9df0 indexes it with the two bytes from 0x63ff1c.
// Values and row order are read from the pinned image, not inferred rotations.
// Neighbour direction order for each (flipX, flipY) reflection; the same
// values as g_rmgReflectedNeighbours. A canonical pattern's direction d reads
// neighbour order[d]. North is up; each grid puts order[d] at d.
//   none      flipY     flipX     both
//   NW N NE   SW S SE   NE N NW   SE S SW
//   W  .  E   W  .  E   E  .  W   E  .  W
//   SW S SE   NW N NE   SE S SW   NE N NW
DATA(0x0063fe9c)
static const s32 g_rmgLineReflectedNeighbours[2][2][TILE_DIR_COUNT] = {
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

DATA(0x0063ff1c)
static const b8 g_rmgLineReflections[4][2] = {
    {0, 0}, {0, 1}, {1, 0}, {1, 1}
};

VA(0x004f9cb0, 0x24e)
MAC_ADDRESS(0x222498, 0x2a4)
void selectRmgLinePattern(
    const b8* neighbours, const TRmgLinePatternTable* table,
    s32& pattern, b8& flipX, b8& flipY)
{
    if (neighbours[TILE_DIR_NORTH] && neighbours[TILE_DIR_EAST]
        && neighbours[TILE_DIR_SOUTH] && neighbours[TILE_DIR_WEST]) {
        pattern = LINE_CROSS;
        flipX = 0;
        flipY = 0;
        return;
    }
    if (neighbours[TILE_DIR_NORTH] && neighbours[TILE_DIR_SOUTH]) {
        if (neighbours[TILE_DIR_EAST]) {
            pattern = LINE_NES;
            flipX = 0;
        } else if (neighbours[TILE_DIR_WEST]) {
            pattern = LINE_NES;
            flipX = 1;
        } else {
            pattern = LINE_NS;
            flipX = 0;
        }
        flipY = 0;
        return;
    }
    if (neighbours[TILE_DIR_EAST] && neighbours[TILE_DIR_WEST]) {
        if (neighbours[TILE_DIR_SOUTH]) {
            pattern = LINE_ESW;
            flipY = 0;
        } else if (neighbours[TILE_DIR_NORTH]) {
            pattern = LINE_ESW;
            flipY = 1;
        } else {
            pattern = LINE_EW;
            flipY = 0;
        }
        flipX = 0;
        return;
    }
    b8 hasCornerVariant = table->m_ranges[LINE_SE_VARIANT].m_frameCount > 0;
    for (u32 reflection = 0; reflection < 4; ++reflection) {
        const s32* order = g_rmgLineReflectedNeighbours
            [g_rmgLineReflections[reflection][0]][g_rmgLineReflections[reflection][1]];
        if (neighbours[order[TILE_DIR_EAST]] && neighbours[order[TILE_DIR_SOUTH]]) {
            if (hasCornerVariant && (neighbours[order[TILE_DIR_NORTHEAST]] || neighbours[order[TILE_DIR_SOUTHWEST]]))
                pattern = LINE_SE_VARIANT;
            else
                pattern = LINE_SE;
            flipX = g_rmgLineReflections[reflection][0];
            flipY = g_rmgLineReflections[reflection][1];
            return;
        }
    }
    if (table->m_ranges[LINE_END_S].m_frameCount > 0) {
        if (neighbours[TILE_DIR_WEST] || neighbours[TILE_DIR_EAST]) {
            pattern = LINE_END_E;
            flipX = neighbours[TILE_DIR_WEST];
            flipY = 0;
        } else {
            if (neighbours[TILE_DIR_SOUTH]) {
                pattern = LINE_END_S;
                flipX = 0;
                flipY = 0;
            } else {
                pattern = LINE_END_S;
                flipX = 0;
                flipY = 1;
            }
        }
    } else {
        pattern = neighbours[TILE_DIR_WEST] || neighbours[TILE_DIR_EAST] ? LINE_EW : LINE_NS;
        flipX = 0;
        flipY = 0;
    }
}

VA(0x0055eda0, 0x07)
MAC_ADDRESS(0x253ccc, 0x60)
TRiverPlacementOp::~TRiverPlacementOp()
{
}

// All river types use the same pattern table, so the argument is ignored.
VA(0x0055edb0, 0x08)
MAC_ADDRESS(0x253ad8, 0x8)  // vtables 0x641174/0x641190; Complete-only
TRmgLinePatternTable* TRiverOp::getPatternTable(s32)
{
    return &g_rmgRiverPatternTable;
}

VA(0x0055edc0, 0x36)
MAC_ADDRESS(0x253ae0, 0x54) // anchor-vtable 0x641174/0x641190/0x6411f0/0x64120c +4
void TRiverOp::setTile(const TRmgGridPoint& point, const TRmgTerrainTile& tile)
{
    TRmgTerrainTile snapshot(tile.m_terrain, tile.m_frame);
    snapshot.m_flipX = tile.m_flipX;
    snapshot.m_flipY = tile.m_flipY;
    m_adapter->setTile(point, snapshot);
}

MAC_ADDRESS(0x253b34, 0x30)
void TRiverOp::setLineType(const TRmgGridPoint& point, s32 value)
{
    m_adapter->setLineType(point, value);
}

MAC_ADDRESS(0x253ba8, 0x88)
void TRiverOp::getTile(const TRmgGridPoint& point, TRmgTerrainTile& tile)
{
    TRmgTerrainTile snapshot = m_adapter->getTile(point);
    tile = snapshot;
}

// Roads and rivers cannot be painted over water or rock terrain.
VA(0x0055ee00, 0x28)
MAC_ADDRESS(0x253b64, 0x44)  // vtables 0x641174/0x641190/0x6411f0/0x64120c
s32 TRiverOp::isBlocked(const TRmgGridPoint& point)
{
    s32 terrain = m_adapter->getTerrain(point);
    if (terrain == eTerrainWater || terrain == eTerrainRock)
        return 1;
    return 0;
}

VA(0x0055ee30, 0x13)
MAC_ADDRESS(0x253c30, 0x30)
s32 TRiverOp::getLineType(const TRmgGridPoint& point)
{
    return m_adapter->getLineType(point);
}

VA(0x0055ee50, 0x76)
MAC_ADDRESS(0x253c60, 0x6c)
TRiverPlacementOp::TRiverPlacementOp(
    TRiverOp::TAbstractMap* newAdapter,
    s32 newRiverType,
    const TRmgGridPoint& newStart)
    : TRiverOp(newAdapter),
      TRmgLineWalker(this, newRiverType, newStart)
{
}

VA_COMPGEN(0x0055eed0, 0x21, SCALAR_DELETING_DTOR, TRiverPlacementOp)

// Cinit 0x55f2f0 builds the seventeen-entry road pattern table from the ids
// at 0x6411ac. The road painter's first virtual slot returns that table.
VA(0x0055f320, 0x08)
MAC_ADDRESS(0x253fc0, 0x8)  // vtables 0x6411f0/0x64120c; Complete-only
TRmgLinePatternTable* TRoadOp::getPatternTable(s32)
{
    return &g_rmgRoadPatternTable;
}

MAC_ADDRESS(0x253fc8, 0x54)
void TRoadOp::setTile(const TRmgGridPoint& point, const TRmgTerrainTile& tile)
{
    TRmgTerrainTile snapshot(tile.m_terrain, tile.m_frame);
    snapshot.m_flipX = tile.m_flipX;
    snapshot.m_flipY = tile.m_flipY;
    m_adapter->setTile(point, snapshot);
}

MAC_ADDRESS(0x25404c, 0x44)
s32 TRoadOp::isBlocked(const TRmgGridPoint& point)
{
    s32 terrain = m_adapter->getTerrain(point);
    if (terrain == eTerrainWater || terrain == eTerrainRock)
        return 1;
    return 0;
}

VA(0x0055f330, 0x17)
MAC_ADDRESS(0x25401c, 0x30)  // vtables 0x641174/0x641190/0x6411f0/0x64120c; Complete-only
void TRoadOp::setLineType(const TRmgGridPoint& point, s32 value)
{
    m_adapter->setLineType(point, value);
}

VA(0x0055f350, 0x34)
MAC_ADDRESS(0x254090, 0x88) // anchor-vtable 0x641174/0x641190/0x6411f0/0x64120c +0x10
void TRoadOp::getTile(const TRmgGridPoint& point, TRmgTerrainTile& tile)
{
    TRmgTerrainTile snapshot = m_adapter->getTile(point);
    tile = snapshot;
}

// The road hierarchy's parallel vtables 0x6411f0/0x64120c use the same
// adapter getLineType forwarding shape in slot 5.
VA(0x0055f390, 0x13)
MAC_ADDRESS(0x254118, 0x30)  // Complete-only road painter
s32 TRoadOp::getLineType(const TRmgGridPoint& point)
{
    return m_adapter->getLineType(point);
}

// The road builder constructs adapter vtable 0x640a04 at 0x548120 and passes
// it here at 0x548143. As in the river constructor, the common painter prefix
// is passed unchanged to walker 0x4fa280, whose subobject begins at +0x10.
VA(0x0055f3b0, 0x76)
MAC_ADDRESS(0x254148, 0x6c) // anchor-callee 0x548143; Complete-only, thiscall ret 0xc
TRoadPlacementOp::TRoadPlacementOp(
    TRoadOp::TAbstractMap* newAdapter,
    s32 newRoadType,
    const TRmgGridPoint& newStart)
    : TRoadOp(newAdapter),
      TRmgLineWalker(this, newRoadType, newStart)
{
}

// Recovering the real constructor emits the final vtable and this wrapper
// naturally. Its 33 bytes call the retained destructor, test the deleting
// flag, conditionally release this, and return the original object pointer.
VA_COMPGEN(0x0055f430, 0x21, SCALAR_DELETING_DTOR, TRoadPlacementOp)

// The road painter's empty derived destructor restores its distinct base
// vtable at 0x6411f0. The road builder at 0x548040 constructs this parallel
// hierarchy; its scalar deleting destructor is retained at 0x55f430.
VA(0x0055f460, 0x07)
MAC_ADDRESS(0x2541b4, 0x60)  // road painter cleanup; Complete-only RMG helper
TRoadPlacementOp::~TRoadPlacementOp()
{
}

VA(0x005fceb0, 0x39)
MAC_ADDRESS(0x25c018, 0x64)
s32 TRmgVector::length() const
{
    return static_cast<s32>(sqrt(static_cast<double>(m_x * m_x + m_y * m_y)));
}

// Both constructors start a half-edge as its own ring with no vertex. The
// ordinary helper is expanded in both; its /Ob2 cost of 60 is spent twice
// inside createEdge's paired-constructor expansion (126 + 60 + 81 + 60),
// which is what leaves the second insert's count-insert body a budget of
// 34 and keeps its first size() call as retail does. The same stores
// written inline in each constructor (costs 157 and 87..135) never spend
// enough: createEdge stayed at 90.96% through 14 constructor spellings.
MAC_ADDRESS(0x25c164, 0x20)
void TRmgHalfEdge::initialize()
{
    m_next = this;
    m_previous = this;
    m_vertexComputed = 0;
    m_vertex.m_x = -1;
    m_vertex.m_y = -1;
}

// The retained paired constructor expands this ordinary twin constructor
// into the successful allocation arm. The same site/zone fields feed the
// Voronoi vertex calculations. Body assignments (cost 81) keep createEdge
// exact; the initializer-list form costs 70 and loses it (90.96%).
TRmgHalfEdge::TRmgHalfEdge(
    TPoint sitePosition, TRmgZone* zone, TRmgHalfEdge* twin)
{
    m_sitePosition = sitePosition;
    m_zone = zone;
    m_twin = twin;
    initialize();
}

VA(0x005fcef0, 0x6c)
MAC_ADDRESS(0x25c07c, 0x98) // anchor-callee 0x5fd078; Complete-only, ret 0x18
TRmgHalfEdge::TRmgHalfEdge(
    TPoint sitePosition, TRmgZone* zone, TPoint twinSitePosition, TRmgZone* twinZone)
    : m_zone(zone)
{
    m_sitePosition = sitePosition;
    m_twin = new TRmgHalfEdge(twinSitePosition, twinZone, this);
    initialize();
}

// Replacing the second pointer exchange with std::swap leaves this body exact
// but lowers the Voronoi constructor from 100% to 83.35% through expansion.
VA(0x005fcf60, 0x31)
MAC_ADDRESS(0x25c184, 0x34) // anchor-callee 0x5fd308; thiscall, ret 4; Complete-only
void TRmgHalfEdge::splice(TRmgHalfEdge* other)
{
    std::swap(m_next->m_previous, other->m_next->m_previous);
    TRmgHalfEdge* next = m_next;
    m_next = other->m_next;
    other->m_next = next;
}

VA(0x005fcfa0, 0x61)
MAC_ADDRESS(0x25c1b8, 0x4c) // anchor-callee addSite 0x5fd790; thiscall, ret 0; Complete-only
void TRmgHalfEdge::detach()
{
    TRmgHalfEdge* previous = m_previous;
    TRmgHalfEdge* twinPrevious = m_twin->m_previous;
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
MAC_ADDRESS(0x25c4e4, 0x14c) // anchor-caller 0x53e050 and five createEdge expansions/calls
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
    m_startingEdge = firstEdge;
}

// The subdivision owns every allocated half-edge and its pointer vector.
// Its retained destructor proves the +0x04 vector and trivial edge cleanup.
VA(0x005fd330, 0x58)
MAC_ADDRESS(0x25c6b4, 0xa4) // anchor-callee 0x53e685; thiscall, ret 0
TRmgVoronoi::~TRmgVoronoi()
{
    for (s32 edge = 0; edge < m_edges.size(); ++edge)
        delete m_edges[edge];
}

VA(0x005fd390, 0x21c)
MAC_ADDRESS(0x25c758, 0x170) // anchor-callers 0x5fd010/0x5fd790; Complete-only, ret 0x18
TRmgHalfEdge* TRmgVoronoi::createEdge(TPoint first, TRmgZone* firstZone,
    TPoint second, TRmgZone* secondZone)
{
    TRmgHalfEdge* edge = new TRmgHalfEdge(first, firstZone, second, secondZone);
    m_edges.push_back(edge);
    m_edges.push_back(edge->getTwin());
    return edge;
}

// Quad-edge Connect(a, b): a new edge from first's destination to second's
// origin, spliced into first's left face and second's origin ring. Ordinary
// and shared; retail expands it in the constructor (0x5fd2c3) and addSite
// (0x5fd72a) while calling createEdge and the fan splices inside it. Its
// /Ob2 cost of 98 sits inside the constructor's 63..145 bracket; field
// reads (93), a named twin (103), chained accessors (113) or endpoint
// locals (122) keep the same bytes here, and 137 or more loses the
// constructor's first diagonal splice.
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

VA(0x005fd5b0, 0xff)
MAC_ADDRESS(0x25c8c8, 0x100) // anchor-caller 0x5fd790; Complete-only, thiscall ret 4
void TRmgVoronoi::removeEdge(TRmgHalfEdge* edge)
{
    edge->detach();
    u32 index = 0;
    while (index < m_edges.size() && m_edges[index] != edge)
        ++index;
    m_edges.erase(m_edges.begin() + index);
    TRmgHalfEdge* twin = edge->getTwin();
    index = 0;
    while (index < m_edges.size() && m_edges[index] != twin)
        ++index;
    m_edges.erase(m_edges.begin() + index);
    delete edge;
    delete twin;
}

// Provisional shared edge-side predicate, used by locate and legalization.
// docs/reference/rmg-voronoi-provenance.md records the adaptation evidence and its
// limits; this resemblance does not establish an original name/declaration.
// Graphics Gems IV delaunay/quadedge.C's RightOf(x, e) is ccw(x, Dest, Org)
// over TriArea; Complete uses integer by-value TPoint and the canonical
// orientation below in the same cyclic order. The ccw layer is ordinary:
// addSite expands RightOf with a nested budget of 54, expands ccw (cost 31)
// and refuses the orientation call as retail does at 0x5fdfa7; locate's
// budgets expand all three. Retail locate's first expanded orientation has
// no spilled endpoint; the named twin restores all 215 bytes. Flattening
// the call boundary returns 91.0460%.
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

VA(0x005fd6b0, 0xd7)
MAC_ADDRESS(0x25c9c8, 0x124) // anchor-callers 0x53dad0/0x53e050/0x5fd790; ret 8
TRmgHalfEdge* TRmgVoronoi::locate(TPoint point)
{
    TRmgHalfEdge* edge = m_startingEdge;
    for (;;) {
        {
            TPoint origin = edge->m_sitePosition;
            if (point == origin)
                break;
        }
        {
            TPoint destination = edge->m_twin->m_sitePosition;
            if (point == destination) {
                edge = edge->m_twin;
                break;
            }
        }
        if (isRmgPointRightOfEdge(point, edge)) {
            edge = edge->m_twin;
        } else {
            TRmgHalfEdge* next = edge->m_next;
            if (!isRmgPointRightOfEdge(point, next)) {
                edge = next;
                continue;
            }
            TRmgHalfEdge* previous = edge->m_twin->m_previous->m_twin;
            if (isRmgPointRightOfEdge(point, previous))
                break;
            edge = previous;
        }
    }
    return edge;
}

// Provisional edge flip: retail saves both predecessors before detach,
// transfers their opposite sites/zones, and splices into the new rings.
// Using the opposite-site/zone and left-face accessors throughout this flip,
// detach and addSite's closing test lowers addSite from 100% to 93.74%; the
// other support functions remain exact. Preserve the matched helper boundaries.
MAC_ADDRESS(0x25c204, 0xb0)
static void flipRmgEdge(TRmgHalfEdge* edge)
{
    TRmgHalfEdge* previous = edge->m_previous;
    TRmgHalfEdge* twinPrevious = edge->m_twin->m_previous;
    edge->detach();
    edge->m_zone = previous->m_twin->m_zone;
    edge->m_sitePosition = previous->m_twin->m_sitePosition;
    edge->m_twin->m_zone = twinPrevious->m_twin->m_zone;
    edge->m_twin->m_sitePosition = twinPrevious->m_twin->m_sitePosition;
    edge->splice(previous->m_twin->m_previous);
    edge->m_twin->splice(twinPrevious->m_twin->m_previous);
}

// Provisional segment predicate, Graphics Gems IV's OnEdge: retail snapshots
// the opposite endpoint, calls the three squared distances, rejects a site
// beyond either endpoint, then evaluates the implicit line a*x + b*y + c
// (a = dy, b = -dx, c = -(a*org.x + b*org.y)) and materializes the zero
// test as a byte. Inside addSite this expansion gets a nested budget of 33
// (908 - 174 over 22 remaining sites), which refuses all three distance
// calls like retail; a TRmgLine constructor/evaluate pair is refused at
// that budget too (72.5%-77.6%), so the equation stays inline. An
// orientation call here scores 89.4762% against the accessor form's
// 91.1250%. The line origin is bound by reference to the site field, the
// way the flip helper reads it: retail loads org.x once into ecx and spills
// dx/dy to [ebp-8]/[ebp-0xc] for the four products, which only this binding
// reproduces; a by-value TPoint copy through the accessor re-reads the
// field and keeps dx in a register (addSite 90.3482% with the rest exact).
static b8 isRmgPointOnSegment(TPoint point, TRmgHalfEdge* edge)
{
    TPoint opposite = edge->getOppositeSitePosition();
    s32 firstDistance = getRmgSquaredDistance(point, edge->getSitePosition());
    s32 secondDistance = getRmgSquaredDistance(point, opposite);
    s32 edgeDistance = getRmgSquaredDistance(edge->getSitePosition(), opposite);
    if (firstDistance > edgeDistance || secondDistance > edgeDistance)
        return 0;
    const TPoint& origin = edge->m_sitePosition;
    s32 dx = opposite.m_x - origin.m_x;
    s32 dy = opposite.m_y - origin.m_y;
    s32 c = -(dy * origin.m_x - dx * origin.m_y);
    return dy * point.m_x - dx * point.m_y + c == 0;
}

// Provisional geometric predicate: retail snapshots three points before
// four orientation calls and a signed 64-bit circumcircle determinant.
// Sixty determinant-expression forms tested named versus embedded area
// calls, equivalent sum groupings, x/y square order, which product operand
// widens, and result lifetime. Twenty distinct objects span 33.2202% to the
// unchanged 45.4167% caller score; none recovers the missing orientation
// calls. Preserve the actual by-value point and signed-product boundaries.
MAC_ADDRESS(0x25cb80, 0x1c4)
static b8 isRmgPointInsideCircle(TPoint first, TPoint second,
    TPoint third, TPoint point)
{
    s32 firstArea = getRmgPointOrientation(second, third, point);
    s32 secondArea = getRmgPointOrientation(first, third, point);
    s32 thirdArea = getRmgPointOrientation(first, second, point);
    s32 pointArea = getRmgPointOrientation(first, second, third);
    s64 determinant = static_cast<s64>(third.m_x * third.m_x + third.m_y * third.m_y) * thirdArea
        - static_cast<s64>(second.m_x * second.m_x + second.m_y * second.m_y) * secondArea
        + static_cast<s64>(first.m_x * first.m_x + first.m_y * first.m_y) * firstArea
        - static_cast<s64>(point.m_x * point.m_x + point.m_y * point.m_y) * pointArea;
    return determinant > 0;
}

VA(0x005fd790, 0x348)
MAC_ADDRESS(0x25cd44, 0x218) // anchor-caller 0x53e050; Complete-only, thiscall ret 0xc
void TRmgVoronoi::addSite(TPoint point, TRmgZone* zone)
{
    TRmgHalfEdge* edge = locate(point);
    {
        TPoint origin = edge->getSitePosition();
        if (point == origin)
            return;
    }
    {
        TPoint destination = edge->getOppositeSitePosition();
        if (point == destination)
            return;
    }
    if (isRmgPointOnSegment(point, edge)) {
        edge = edge->getPrevious();
        removeEdge(edge->getNext());
    }
    TRmgHalfEdge* base = createEdge(edge->getSitePosition(), edge->getZone(), point, zone);
    base->splice(edge);
    m_startingEdge = base;
    do {
        base = connectEdges(edge, base->getTwin());
        edge = base->getPrevious();
    } while (edge->getTwin()->getPrevious() != m_startingEdge);

    for (;;) {
        TRmgHalfEdge* previous = edge->getPrevious();
        if (isRmgPointRightOfEdge(previous->getTwin()->getSitePosition(), edge)) {
            if (isRmgPointInsideCircle(edge->getSitePosition(),
                    previous->getTwin()->getSitePosition(), edge->getOppositeSitePosition(), point)) {
                flipRmgEdge(edge);
                edge = edge->getPrevious();
                continue;
            }
        }
        if (edge->getNext() == m_startingEdge)
            return;
        edge = edge->getNext()->getNext()->getTwin();
    }
}

VA(0x005fdae0, 0x2b)
MAC_ADDRESS(0x25c2b4, 0x50) // anchor-callee 0x5fd937/0x5fd97e; Complete-only
s32 getRmgPointOrientation(TPoint first, TPoint second, TPoint third)
{
    return (second.m_x - first.m_x) * (third.m_y - first.m_y)
        - (second.m_y - first.m_y) * (third.m_x - first.m_x);
}

// The subdivision constructor retains seven single-edge insertions at
// 0x5fd091/0x5fd0f6/0x5fd10e/0x5fd15a/0x5fd172/0x5fd1bb/0x5fd1d3.
// Four-byte elements, ret 8 and the owning m_edges vector identify this
// ordinary Dinkumware specialization independently of its ICF helper names.
VA_COMPGEN(0x005fdd60, 0x1b1, VECTOR_INSERT_SINGLE, TRmgHalfEdge)

// Retail's only callers of the four-byte fill at 0x5fdf20 are
// TRmgVoronoi::createEdge (0x5fd4b9, 0x5fd53d) and the boundary-vertex insert
// above (0x5fde8c): it is this unit's vector<TRmgHalfEdge*> _Ufill.
// Widget vectors reach the folded copy at 0x48d940 instead.
VA_COMPGEN(0x005fdf20, 0x26, VECTOR_UFILL, TRmgHalfEdge)
