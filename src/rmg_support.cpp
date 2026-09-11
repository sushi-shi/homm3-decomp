// rmg_support.cpp - retained Complete random-map helper bodies.
//
// Retail keeps these ordinary helpers out of line in CreateRiver.  Their
// declarations remain visible through rmg.h, while placing the definitions in
// this companion translation unit reproduces the natural body-visibility
// boundary without source-false inline controls.
#include <va.h>
#include <algorithm>
#include <math.h>
#include "exceptions.h"
#include "rmg.h"
#include "rmg_terrain.h"
#include "tiles.h"

// The common painter prefix owns only the two dimensions and virtual API.
// Both retained final constructors obtain the adapter size before building
// this base, then store their own adapter at +0xc. Keep one ordinary helper.
TRmgLinePainterInterface::TRmgLinePainterInterface(const TRmgGridPoint& size)
    : m_size(size)
{
}

// Both cinit callers pass a nonempty table (13 river entries, 17 road
// entries). Retail copies it, clears nine interleaved index/count pairs,
// then counts each pattern id and records the start of each new run.
// It reads the first entry without an empty guard and does not validate
// the pattern ids: both are source-data preconditions, not new runtime checks.
// The allocation failure retains the canonical exception constructor.
// Exact: keeping the allocation result in a local gives std::copy retail's
// single source walk plus a destination displacement. Assigning/checking
// only the member keeps two pointer walks (93.5294%). Changing the standard
// copy operation to uninitialized_copy instead gives 92.5000%.
VA(0x004F9BE0, 0xB7) // anchor-callee 0x55ed7c/0x55f2fc; thiscall, ret 8; retail-only
TRmgLinePatternTable::TRmgLinePatternTable(unsigned int patternCount, const int* patterns)
    : m_patternCount(patternCount), m_patterns(0)
{
    int* allocated = new int[m_patternCount];
    m_patterns = allocated;
    if (!allocated)
        throw TAllocationFailure();
    std::copy(patterns, patterns + m_patternCount, m_patterns);
    for (unsigned int value = 0; value < 9; ++value) {
        m_ranges[value].m_firstIndex = 0;
        m_ranges[value].m_valueCount = 0;
    }
    int previous = m_patterns[0];
    ++m_ranges[previous].m_valueCount;
    for (unsigned int index = 1; index < m_patternCount; ++index) {
        if (m_patterns[index] != previous) {
            previous = m_patterns[index];
            m_ranges[previous].m_firstIndex = index;
        }
        ++m_ranges[previous].m_valueCount;
    }
}

// Both table cleanup thunks tail-call this body. The paired constructor owns
// only the copied pattern-id array at +4; the nine index/count pairs are plain
// integers and need no cleanup.
VA(0x004F9CA0, 0x0B)  // cinit cleanups 0x55ed90/0x55f310; Complete-only
TRmgLinePatternTable::~TRmgLinePatternTable()
{
    delete[] m_patterns;
}

// This library-side copy is retained separately from the terrain selector's
// 0x642c00 table. Retail 0x4f9df0 indexes it with the two bytes from 0x63ff1c.
// Values and row order are read from the pinned image, not inferred rotations.
DATA(0x0063FE9C)
static const int g_rmgLineReflectedNeighbours[2][2][8] = {
    {{0, 1, 2, 3, 4, 5, 6, 7}, {4, 3, 2, 1, 0, 7, 6, 5}},
    {{0, 7, 6, 5, 4, 3, 2, 1}, {4, 5, 6, 7, 0, 1, 2, 3}}
};

DATA(0x0063FF1C)
static const unsigned char g_rmgLineReflections[4][2] = {
    {0, 0}, {0, 1}, {1, 0}, {1, 1}
};

// The retained shared selector first handles crosses/opposite cardinals,
// then reflected corners, then ends or straight fallback patterns. The table
// advertises optional ids 5 and 0 through their occurrence counts. It consumes
// a byte mask, not the terrain selector's three-way neighbour classifications.
// Only caller 0x4f9f00 supplies three distinct outputs (id, X flip, Y flip).
// Exact: each cardinal group shares its constant flip store, while the end
// case tests south before the complete pattern/X/Y stores in each arm.
// Moving the common pattern/X stores above that test lets VC6 replace even
// explicit early returns with SETE (92.3290%). Reversing pattern/X order in
// the full arms gives 99.9474%; changing the unsigned count test to != 0
// gives SETNE rather than retail's SETA (99.7368%). Byte and bool availability
// locals both match. All 590 bytes agree with the five table addresses resolved.
VA(0x004F9CB0, 0x24E) // anchor-callee 0x4f9fd8; fastcall, ret 0xc; retail-only
void selectRmgLinePattern(
    const unsigned char* neighbours, const TRmgLinePatternTable* table,
    int& pattern, unsigned char& flipX, unsigned char& flipY)
{
    if (neighbours[TILE_DIR_NORTH] && neighbours[TILE_DIR_EAST]
        && neighbours[TILE_DIR_SOUTH] && neighbours[TILE_DIR_WEST]) {
        pattern = 8;
        flipX = 0;
        flipY = 0;
        return;
    }
    if (neighbours[TILE_DIR_NORTH] && neighbours[TILE_DIR_SOUTH]) {
        if (neighbours[TILE_DIR_EAST]) {
            pattern = 6;
            flipX = 0;
        } else if (neighbours[TILE_DIR_WEST]) {
            pattern = 6;
            flipX = 1;
        } else {
            pattern = 2;
            flipX = 0;
        }
        flipY = 0;
        return;
    }
    if (neighbours[TILE_DIR_EAST] && neighbours[TILE_DIR_WEST]) {
        if (neighbours[TILE_DIR_SOUTH]) {
            pattern = 7;
            flipY = 0;
        } else if (neighbours[TILE_DIR_NORTH]) {
            pattern = 7;
            flipY = 1;
        } else {
            pattern = 3;
            flipY = 0;
        }
        flipX = 0;
        return;
    }
    unsigned char hasCornerVariant = table->m_ranges[5].m_valueCount > 0;
    for (unsigned int reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgLineReflectedNeighbours
            [g_rmgLineReflections[reflection][0]][g_rmgLineReflections[reflection][1]];
        if (neighbours[order[2]] && neighbours[order[4]]) {
            if (hasCornerVariant && (neighbours[order[1]] || neighbours[order[5]]))
                pattern = 5;
            else
                pattern = 4;
            flipX = g_rmgLineReflections[reflection][0];
            flipY = g_rmgLineReflections[reflection][1];
            return;
        }
    }
    if (table->m_ranges[0].m_valueCount > 0) {
        if (neighbours[TILE_DIR_WEST] || neighbours[TILE_DIR_EAST]) {
            pattern = 1;
            flipX = neighbours[TILE_DIR_WEST];
            flipY = 0;
        } else {
            if (neighbours[TILE_DIR_SOUTH]) {
                pattern = 0;
                flipX = 0;
                flipY = 0;
            } else {
                pattern = 0;
                flipX = 0;
                flipY = 1;
            }
        }
    } else {
        pattern = neighbours[TILE_DIR_WEST] || neighbours[TILE_DIR_EAST] ? 3 : 2;
        flipX = 0;
        flipY = 0;
    }
}

// Retail retains this tiny value constructor throughout the RMG pathfinding
// cluster.  Its three stores and `ret 0xc` fix both the by-value ABI and the
// 12-byte position layout.
VA(0x005355C0, 0x1A)  // retail RMG caller cluster; Complete-only helper
TRmgMapPosition::TRmgMapPosition(int newX, int newY, int newZ)
    : m_x(newX), m_y(newY), m_z(newZ)
{
}

// The river painter deliberately inherits the generic line walker as its
// second base.  Retail's calls use `this + 0x10`, which is the natural VC6
// adjustment for that source relationship.  Its otherwise-empty destructor
// restores the first base's vtable and is retained out of line.
VA(0x0055EDA0, 0x07)  // CreateRiver EH cleanup; retail-only RMG helper
TRmgRiverPainter::~TRmgRiverPainter()
{
}

// The first virtual slot returns the shared river pattern table. The argument
// selects within that table at later painting sites and is intentionally not
// consumed by this accessor.
VA(0x0055EDB0, 0x08)  // vtables 0x641174/0x641190; Complete-only
TRmgLinePatternTable* TRmgLinePainter::getPattern(int)
{
    return &g_rmgRiverPatternTable;
}

// All four line-painter tables use this setter. It takes a snapshot of the
// ten meaningful tile bytes before dispatching adapter slot 1. The copied
// value, not the original caller-owned tile, is the forwarded argument.
// Exact with value construction and X-before-Y flip stores. A custom tile
// copy constructor also matched this body but changed the adapter-return
// handling in terrain users. Implicit-copy snapshot initialization is 87.23%;
// reversing these independent flip stores gives 99.82%, not retail order.
VA(0x0055EDC0, 0x36) // anchor-vtable 0x641174/0x641190/0x6411f0/0x64120c +4
void TRmgLinePainter::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    rmgTerrainTile snapshot(tile.m_terrain, tile.m_frame);
    snapshot.m_flipX = tile.m_flipX;
    snapshot.m_flipY = tile.m_flipY;
    m_adapter->setTile(point, snapshot);
}

void TRmgLinePainter::setOverlay(const TRmgGridPoint& point, int value)
{
    m_adapter->setOverlay(point, value);
}

void TRmgLinePainter::getTile(const TRmgGridPoint& point, rmgTerrainTile& tile)
{
    rmgTerrainTile snapshot = m_adapter->getTile(point);
    tile = snapshot;
}

// Slot 3 of all four river/road painter vtables forwards to the adapter's
// overlay query and accepts exactly the two retained paintable values.
VA(0x0055EE00, 0x28)  // vtables 0x641174/0x641190/0x6411f0/0x64120c
int TRmgLinePainter::canPaint(const TRmgGridPoint& point)
{
    int overlay = m_adapter->getOverlay(point);
    if (overlay == eTerrainWater || overlay == eTerrainRock)
        return 1;
    return 0;
}

// Vtable 0x641174/0x641190 slot 5 forwards the point to the adapter's
// getLand slot. The Complete-only RMG hierarchy has no Dreamcast counterpart.
VA(0x0055EE30, 0x13)
int TRmgLinePainter::getLand(const TRmgGridPoint& point)
{
    return m_adapter->getLand(point);
}

// Exact: all 118 raw bytes after seven relocations. The grid copy constructor
// keeps both GetSize result stores before the adapter store. An implicit copy
// interleaves the adapter and second component (99.71%); moving the adapter
// assignment into the base ctor body puts its vptr store too early (99.10%).
VA(0x0055EE50, 0x76)  // CreateRiver sole caller; retail-only RMG helper
TRmgRiverPainter::TRmgRiverPainter(
    TRmgMapAdapterInterface* newAdapter,
    int newRiverType,
    const TRmgGridPoint& newStart)
    : TRmgLinePainter(newAdapter),
      TRmgLineWalker(this, newRiverType, newStart)
{
}

// The derived vtable at 0x641190 places this compiler-generated deleting
// wrapper after the retained constructor. It calls the exact empty derived
// destructor at 0x55eda0 before conditionally releasing the complete object.
VA_COMPGEN(0x0055EED0, 0x21, SCALAR_DELETING_DTOR, TRmgRiverPainter)

// Cinit 0x55f2f0 builds the seventeen-entry road pattern table from the ids
// at 0x6411ac. The road painter's first virtual slot returns that table.
VA(0x0055F320, 0x08)  // vtables 0x6411f0/0x64120c; Complete-only
TRmgLinePatternTable* TRmgRoadLinePainter::getPattern(int)
{
    return &g_rmgRoadPatternTable;
}

void TRmgRoadLinePainter::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    rmgTerrainTile snapshot(tile.m_terrain, tile.m_frame);
    snapshot.m_flipX = tile.m_flipX;
    snapshot.m_flipY = tile.m_flipY;
    m_adapter->setTile(point, snapshot);
}

int TRmgRoadLinePainter::canPaint(const TRmgGridPoint& point)
{
    int overlay = m_adapter->getOverlay(point);
    if (overlay == eTerrainWater || overlay == eTerrainRock)
        return 1;
    return 0;
}

// The river and road line-painter vtables all share this ICF representative.
// Loading the adapter at +0xc and dispatching its slot +8 proves setOverlay's
// two-argument forwarding body; the road COMDAT wins retail link order.
VA(0x0055F330, 0x17)  // vtables 0x641174/0x641190/0x6411f0/0x64120c; Complete-only
void TRmgRoadLinePainter::setOverlay(const TRmgGridPoint& point, int value)
{
    m_adapter->setOverlay(point, value);
}

// The painter API writes to an explicit output reference, while its adapter
// returns a tile by value. Retail therefore uses a 12-byte return temporary
// and copies its four fields to the caller's output. Both painter classes
// share the road COMDAT at this vtable slot.
// Exact with a named returned value followed by canonical assignment. Keep
// the implicit copy constructor: a custom four-field copy gave 62.96% here
// and introduced extra returned-value copies in the terrain fill path.
VA(0x0055F350, 0x34) // anchor-vtable 0x641174/0x641190/0x6411f0/0x64120c +0x10
void TRmgRoadLinePainter::getTile(const TRmgGridPoint& point, rmgTerrainTile& tile)
{
    rmgTerrainTile snapshot = m_adapter->getTile(point);
    tile = snapshot;
}

// The road hierarchy's parallel vtables 0x6411f0/0x64120c use the same
// adapter getLand forwarding shape in slot 5.
VA(0x0055F390, 0x13)  // Complete-only road painter
int TRmgRoadLinePainter::getLand(const TRmgGridPoint& point)
{
    return m_adapter->getLand(point);
}

// The road builder constructs adapter vtable 0x640a04 at 0x548120 and passes
// it here at 0x548143. As in the river constructor, the common painter prefix
// is passed unchanged to walker 0x4fa280, whose subobject begins at +0x10.
VA(0x0055F3B0, 0x76) // anchor-callee 0x548143; Complete-only, thiscall ret 0xc
TRmgRoadPainter::TRmgRoadPainter(
    TRmgRoadMapAdapterInterface* newAdapter,
    int newRoadType,
    const TRmgGridPoint& newStart)
    : TRmgRoadLinePainter(newAdapter),
      TRmgLineWalker(this, newRoadType, newStart)
{
}

// Recovering the real constructor emits the final vtable and this wrapper
// naturally. Its 33 bytes call the retained destructor, test the deleting
// flag, conditionally release this, and return the original object pointer.
VA_COMPGEN(0x0055F430, 0x21, SCALAR_DELETING_DTOR, TRmgRoadPainter)

// The road painter's empty derived destructor restores its distinct base
// vtable at 0x6411f0. The road builder at 0x548040 constructs this parallel
// hierarchy; its scalar deleting destructor is retained at 0x55f430.
VA(0x0055F460, 0x07)  // road painter cleanup; Complete-only RMG helper
TRmgRoadPainter::~TRmgRoadPainter()
{
}

// Called on the perpendicular edge vector by DrawIrregularZoneBoundary.
// Retail squares the integer components before converting their sum to
// double, then truncates sqrt's result. The name is provisional; this
// Complete geometry helper has no Dreamcast counterpart.
VA(0x005FCEB0, 0x39) // anchor-callee 0x53c0d1; thiscall; retail-only
int TRmgVector::length() const
{
    return static_cast<int>(sqrt(static_cast<double>(m_x * m_x + m_y * m_y)));
}

// Both constructors start a half-edge as its own ring with no vertex. The
// ordinary helper is expanded in both; its /Ob2 cost of 60 is spent twice
// inside createEdge's paired-constructor expansion (126 + 60 + 81 + 60),
// which is what leaves the second insert's count-insert body a budget of
// 34 and keeps its first size() call as retail does. The same stores
// written inline in each constructor (costs 157 and 87..135) never spend
// enough: createEdge stayed at 90.96% through 14 constructor spellings.
void TRmgBoundaryVertex::initialize()
{
    m_next = this;
    m_previous = this;
    m_positionComputed = 0;
    m_position.m_x = -1;
    m_position.m_y = -1;
}

// The retained paired constructor expands this ordinary twin constructor
// into the successful allocation arm. The same site/zone fields feed the
// Voronoi vertex calculations. Body assignments (cost 81) keep createEdge
// exact; the initializer-list form costs 70 and loses it (90.96%).
TRmgBoundaryVertex::TRmgBoundaryVertex(
    TPoint sitePosition, TRmgZone* zone, TRmgBoundaryVertex* twin)
{
    m_sitePosition = sitePosition;
    m_zone = zone;
    m_twin = twin;
    initialize();
}

// The diagram constructor allocates pairs using two by-value point/zone
// pairs. It retains this constructor, while createEdge 0x5fd390 expands it.
// Both paths expand the ordinary opposite-edge constructor above.
// Exact: initialize the zone, copy the site in the body, allocate the twin,
// then share initialize with the twin constructor (cost 126 + 60; the
// inline stores cost 157 and starve createEdge's second insertion). The 22
// partial-initializer forms found two exact choices; chaining the -1
// assignments scores 99.9535%, a temporary TPoint keeps a different
// final-store schedule, and component site copies (cost 173) lose the
// first point/zone schedule (97.2093%).
VA(0x005FCEF0, 0x6C) // anchor-callee 0x5fd078; Complete-only, ret 0x18
TRmgBoundaryVertex::TRmgBoundaryVertex(
    TPoint sitePosition, TRmgZone* zone, TPoint twinSitePosition, TRmgZone* twinZone)
    : m_zone(zone)
{
    m_sitePosition = sitePosition;
    m_twin = new TRmgBoundaryVertex(twinSitePosition, twinZone, this);
    initialize();
}

// The two swaps preserve the bidirectional ring after exchanging successors.
// This ordinary helper is retained by the diagram constructor and expanded
// twice in detach. The existing +0x10/+0x14 fields prove its semantic owner.
// Exact with a std::swap of the predecessors and a manual successor swap.
// Two std::swap calls give the same 49 bytes at /Ob2 cost 52; this form
// costs 65, and the diagram constructor's diagonal needs 63..71 so that
// connectEdges' nested budget (197 - 98) refuses createEdge and the second
// splice while expanding the first (predict-inline --trace 0x5fd010). Four
// manual temporaries (cost 77/82) or named successors with std::swap (62)
// break splice, detach or the constructor.
VA(0x005FCF60, 0x31) // anchor-callee 0x5fd308; thiscall, ret 4; Complete-only
void TRmgBoundaryVertex::splice(TRmgBoundaryVertex* other)
{
    std::swap(m_next->m_previous, other->m_next->m_previous);
    TRmgBoundaryVertex* next = m_next;
    m_next = other->m_next;
    other->m_next = next;
}

// addSite calls this before reusing an edge. Save the twin's predecessor
// before either splice, then detach each half-edge from its own ring.
// Exact: capture this predecessor first. Twin-first is 89.1667%; rereading
// the twin predecessor after the first splice is 59.6905% and loses the
// retail lifetime. The ordinary splice remains shared and auto-inlines here.
VA(0x005FCFA0, 0x61) // anchor-callee addSite 0x5fd790; thiscall, ret 0; Complete-only
void TRmgBoundaryVertex::detach()
{
    TRmgBoundaryVertex* previous = m_previous;
    TRmgBoundaryVertex* twinPrevious = m_twin->m_previous;
    splice(previous);
    m_twin->splice(twinPrevious);
}

// Complete starts with a rectangular outer subdivision spanning -200..400.
// Four paired edges form its perimeter; a fifth connects opposite corners.
// Each pair is owned through createEdge and joined through the shared splice,
// as Graphics Gems IV's Subdivision constructor splices ea->Sym() to eb.
// Exact (2026-09-11): the four fan splices go through getTwin, and the
// diagonal is connectEdges(fourthEdge, thirdEdge). With 13 candidate sites
// after the first createEdge its nested budget is (949 - 107) / 13 = 64:
// the first expansion's second insert wrapper still expands while the
// later ones (55, 45, 39) call it, as retail does. The diagonal's nested
// budget of 197 - 98 then refuses createEdge and the second splice but
// expands the first. Direct m_twin reads (r1 = 9) expand every wrapper and
// createEdge; getTwin in the diagonal too (r1 = 17) starves the first
// expansion (66.1639%).
VA(0x005FD010, 0x316) // anchor-caller 0x53e050 and five createEdge expansions/calls
TRmgVoronoi::TRmgVoronoi()
{
    TPoint first(-200, -200);
    TPoint second(400, -200);
    TPoint third(400, 400);
    TPoint fourth(-200, 400);
    TRmgBoundaryVertex* firstEdge = createEdge(first, 0, second, 0);
    TRmgBoundaryVertex* secondEdge = createEdge(second, 0, third, 0);
    TRmgBoundaryVertex* thirdEdge = createEdge(third, 0, fourth, 0);
    TRmgBoundaryVertex* fourthEdge = createEdge(fourth, 0, first, 0);
    firstEdge->getTwin()->splice(secondEdge);
    secondEdge->getTwin()->splice(thirdEdge);
    thirdEdge->getTwin()->splice(fourthEdge);
    fourthEdge->getTwin()->splice(firstEdge);
    connectEdges(fourthEdge, thirdEdge);
    m_root = firstEdge;
}

// The subdivision owns every allocated half-edge and its pointer vector.
// Its retained destructor proves the +0x04 vector and trivial edge cleanup.
VA(0x005FD330, 0x58) // anchor-callee 0x53e685; thiscall, ret 0
TRmgVoronoi::~TRmgVoronoi()
{
    for (int edge = 0; edge < m_edges.size(); ++edge)
        delete m_edges[edge];
}

// Constructor and site insertion share this retained factory. Retail expands
// the ordinary paired constructor, then inserts each half into the owning
// vector. The two source insertions have different nested inline decisions:
// the first push_back's count insert is refused (756 / 2 - 64 < 469), the
// second's is expanded with a body budget of (609 - 64 - 469) / 2 = 38, so
// its first size() stays a call like retail's four. Exact (2026-09-12) once
// the constructors spend 327 units before the second push_back and the
// twin goes through getTwin(): that temporary is homed in the dead
// firstZone argument slot (frame 8), while a named twin local or the field
// itself takes a third local slot (99.82% / 84.27%).
VA(0x005FD390, 0x21C) // anchor-callers 0x5fd010/0x5fd790; Complete-only, ret 0x18
TRmgBoundaryVertex* TRmgVoronoi::createEdge(TPoint first, TRmgZone* firstZone,
    TPoint second, TRmgZone* secondZone)
{
    TRmgBoundaryVertex* edge = new TRmgBoundaryVertex(first, firstZone, second, secondZone);
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
TRmgBoundaryVertex* TRmgVoronoi::connectEdges(TRmgBoundaryVertex* first,
    TRmgBoundaryVertex* second)
{
    TRmgBoundaryVertex* edge = createEdge(first->getOppositeSitePosition(),
        first->getOppositeZone(), second->getSitePosition(), second->getZone());
    edge->splice(first->getLeftNext());
    edge->getTwin()->splice(second);
    return edge;
}

// Site insertion removes a paired edge from the subdivision: detach both
// ring links, erase each owned pointer, then free both trivial half-edges.
// Retail expands detach's first splice but retains the twin's splice call.
// Both searches use unsigned indices and erase through the vector interface.
// Exact (2026-09-11): reading the twin through getTwin adds one candidate
// site after detach, so detach's nested budget is (1000 - 61) / 10 = 93
// and its second splice (65) is refused after the first, exactly as retail.
// With the field read (nine sites) the budget is 104 and both expand;
// the earlier 64 loop/index/erase forms never changed that site count.
VA(0x005FD5B0, 0xFF) // anchor-caller 0x5fd790; Complete-only, thiscall ret 4
void TRmgVoronoi::removeEdge(TRmgBoundaryVertex* edge)
{
    edge->detach();
    unsigned int index = 0;
    while (index < m_edges.size() && m_edges[index] != edge)
        ++index;
    m_edges.erase(m_edges.begin() + index);
    TRmgBoundaryVertex* twin = edge->getTwin();
    index = 0;
    while (index < m_edges.size() && m_edges[index] != twin)
        ++index;
    m_edges.erase(m_edges.begin() + index);
    delete edge;
    delete twin;
}

// Provisional shared edge-side predicate, used by locate and legalization.
// Graphics Gems IV delaunay/quadedge.C's RightOf(x, e) is ccw(x, Dest, Org)
// over TriArea; Complete uses integer by-value TPoint and the canonical
// orientation below in the same cyclic order. The ccw layer is ordinary:
// addSite expands RightOf with a nested budget of 54, expands ccw (cost 31)
// and refuses the orientation call as retail does at 0x5fdfa7; locate's
// budgets expand all three. Retail locate's first expanded orientation has
// no spilled endpoint; the named twin restores all 215 bytes. Flattening
// the call boundary returns 91.0460%.
static int isRmgCounterClockwise(TPoint first, TPoint second, TPoint third)
{
    return getRmgPointOrientation(first, second, third) > 0;
}

static int isRmgPointRightOfEdge(TPoint point, TRmgBoundaryVertex* edge)
{
    TRmgBoundaryVertex* twin = edge->m_twin;
    return isRmgCounterClockwise(edge->m_sitePosition, point, twin->m_sitePosition);
}

// The zone-building callers pass an eight-byte TPoint and receive an edge.
// Retail first recognizes either site endpoint, then follows the twin,
// successor or twin-predecessor-twin according to integer orientation.
// This is a Complete-only subdivision lookup; the role name is provisional.
// Exact: the shared edge-side predicate restores the eight-byte scratch
// frame and eliminates the first orientation's endpoint spill. All thirteen
// blocks, seven branches and 215 unmasked bytes agree, with no relocations.
// Earlier direct-call forms reached 91.0460%: endpoint scopes improved
// 75.2529% to 83.7127%, then the successor binding reached that plateau.
// Named scalar results and early-continue forms were neutral. Keep both
// canonical helper boundaries rather than pasting orientation arithmetic.
VA(0x005FD6B0, 0xD7) // anchor-callers 0x53dad0/0x53e050/0x5fd790; ret 8
TRmgBoundaryVertex* TRmgVoronoi::locate(TPoint point)
{
    TRmgBoundaryVertex* edge = m_root;
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
            TRmgBoundaryVertex* next = edge->m_next;
            if (!isRmgPointRightOfEdge(point, next)) {
                edge = next;
                continue;
            }
            TRmgBoundaryVertex* previous = edge->m_twin->m_previous->m_twin;
            if (isRmgPointRightOfEdge(point, previous))
                break;
            edge = previous;
        }
    }
    return edge;
}

// Provisional edge flip: retail saves both predecessors before detach,
// transfers their opposite sites/zones, and splices into the new rings.
static void flipRmgEdge(TRmgBoundaryVertex* edge)
{
    TRmgBoundaryVertex* previous = edge->m_previous;
    TRmgBoundaryVertex* twinPrevious = edge->m_twin->m_previous;
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
// orientation call here scores 89.4762% against this form's 91.1250%.
static unsigned char isRmgPointOnSegment(TPoint point, TRmgBoundaryVertex* edge)
{
    TPoint opposite = edge->getOppositeSitePosition();
    int firstDistance = getRmgSquaredDistance(point, edge->getSitePosition());
    int secondDistance = getRmgSquaredDistance(point, opposite);
    int edgeDistance = getRmgSquaredDistance(edge->getSitePosition(), opposite);
    if (firstDistance > edgeDistance || secondDistance > edgeDistance)
        return 0;
    TPoint origin = edge->getSitePosition();
    int dx = opposite.m_x - origin.m_x;
    int dy = opposite.m_y - origin.m_y;
    int c = -(dy * origin.m_x - dx * origin.m_y);
    return dy * point.m_x - dx * point.m_y + c == 0;
}

// Provisional geometric predicate: retail snapshots three points before
// four orientation calls and a signed 64-bit circumcircle determinant.
// Sixty determinant-expression forms tested named versus embedded area
// calls, equivalent sum groupings, x/y square order, which product operand
// widens, and result lifetime. Twenty distinct objects span 33.2202% to the
// unchanged 45.4167% caller score; none recovers the missing orientation
// calls. Preserve the actual by-value point and signed-product boundaries.
static unsigned char isRmgPointInsideCircle(TPoint first, TPoint second,
    TPoint third, TPoint point)
{
    int firstArea = getRmgPointOrientation(second, third, point);
    int secondArea = getRmgPointOrientation(first, third, point);
    int thirdArea = getRmgPointOrientation(first, second, point);
    int pointArea = getRmgPointOrientation(first, second, third);
    __int64 determinant = static_cast<__int64>(third.m_x * third.m_x + third.m_y * third.m_y) * thirdArea
        - static_cast<__int64>(second.m_x * second.m_x + second.m_y * second.m_y) * secondArea
        + static_cast<__int64>(first.m_x * first.m_x + first.m_y * first.m_y) * firstArea
        - static_cast<__int64>(point.m_x * point.m_x + point.m_y * point.m_y) * pointArea;
    return determinant > 0;
}

// Complete's incremental subdivision insertion, Graphics Gems IV's
// InsertSite: reject coincident endpoints, split an edge for a collinear
// site, connect the incident fan, then flip suspect diagonals with the
// integer circumcircle determinant (squared norm times orientation widened
// to signed 64 bits, imul/sbb/adc). Every call decision now matches retail:
// the accessor sites (getSitePosition/getTwin/getPrevious/getLeftNext...)
// count as inline candidates, so with 22 sites after the segment predicate
// its distances are refused, connectEdges gets 38 and calls both fan
// splices, RightOf gets 54 and calls the orientation, InCircle gets 42 and
// calls all four, and flipRmgEdge gets 33 and calls detach and both splices
// (predict-inline --trace 0x5fd790). Field reads instead of accessors gave
// 45.4167% with the same helpers.
// Naming the connected edge before advancing (next/edge/base) homes base in
// [ebp-8] across the fan loop with retail's re-entry past the reload; the
// named predecessor (homed in [ebp-0xc] before and after the orientation
// call, as retail) keeps point.m_y in EDI and edge in EBX, and spelling the
// loop condition as getTwin()->getPrevious() keeps both together
// (91.1250% -> 97.6518%). Re-reading the predecessor twice instead ranks
// edge above point.m_y and swaps EBX/EDI across 176 slots (94.03%); a
// separate suspect-loop variable or a site local scores 88.39% / 82.83%.
// A 40-state include-set sweep of this TU moves no function, so the rest is
// not TU state.
// Residual (97.6518%): retail spills the segment predicate's dx and dy to
// [ebp-8]/[ebp-0xc] and re-reads them for the four products, and homes the
// connected edge in the dead zone argument slot while base takes [ebp-8];
// here dx stays in a register and next shares base's slot. Six line
// spellings and five fan-loop forms (166 states over four families) do not
// separate them.
VA(0x005FD790, 0x348) // anchor-caller 0x53e050; Complete-only, thiscall ret 0xc
void TRmgVoronoi::addSite(TPoint point, TRmgZone* zone)
{
    TRmgBoundaryVertex* edge = locate(point);
    if (point == edge->getSitePosition() || point == edge->getOppositeSitePosition())
        return;
    if (isRmgPointOnSegment(point, edge)) {
        edge = edge->getPrevious();
        removeEdge(edge->getNext());
    }
    TRmgBoundaryVertex* base = createEdge(edge->getSitePosition(), edge->getZone(), point, zone);
    base->splice(edge);
    m_root = base;
    do {
        TRmgBoundaryVertex* next = connectEdges(edge, base->getTwin());
        edge = next->getPrevious();
        base = next;
    } while (edge->getTwin()->getPrevious() != m_root);

    for (;;) {
        TRmgBoundaryVertex* previous = edge->getPrevious();
        if (isRmgPointRightOfEdge(previous->getOppositeSitePosition(), edge)) {
            if (isRmgPointInsideCircle(edge->getSitePosition(),
                    previous->getOppositeSitePosition(), edge->getOppositeSitePosition(), point)) {
                flipRmgEdge(edge);
                edge = edge->getPrevious();
                continue;
            }
        }
        if (edge->getNext() == m_root)
            return;
        edge = edge->getNext()->getNext()->getTwin();
    }
}

// addSite tests the orientation of the current site, predecessor's opposite
// site and opposite site, then reuses the same operation in the circumcircle
// determinant. All six stack dwords originate in canonical TPoint fields;
// the body returns the signed cross product, not a normalized predicate.
// The ordinary externally visible helper emits naturally and matches all
// 43 bytes; the previous RMG probe's absent body was not an ABI limitation.
VA(0x005FDAE0, 0x2B) // anchor-callee 0x5fd937/0x5fd97e; Complete-only
int getRmgPointOrientation(TPoint first, TPoint second, TPoint third)
{
    return (second.m_x - first.m_x) * (third.m_y - first.m_y)
        - (second.m_y - first.m_y) * (third.m_x - first.m_x);
}

// The three calls at 0x5fd7e4/0x5fd7f6/0x5fd80d compare the new site's
// squared distance to both edge endpoints against the edge's squared length.
// Two whole site positions, signed subtraction and two integer products
// establish the operation and its aggregate-by-value calling boundary.
// Exact: Y then X local capture reproduces retail's register roles.
// The 24-form arithmetic batch found three exact forms; X-first capture
// keeps the same 33-byte CFG but scores 99.7143%. No TU score falls here.
VA(0x005FDB10, 0x21) // anchor-callee addSite; Complete-only, ret 0x10
int getRmgSquaredDistance(TPoint first, TPoint second)
{
    int dy = first.m_y - second.m_y;
    int dx = first.m_x - second.m_x;
    return dx * dx + dy * dy;
}

// The subdivision constructor retains seven single-edge insertions at
// 0x5fd091/0x5fd0f6/0x5fd10e/0x5fd15a/0x5fd172/0x5fd1bb/0x5fd1d3.
// Four-byte elements, ret 8 and the owning m_edges vector identify this
// ordinary Dinkumware specialization independently of its ICF helper names.
VA_COMPGEN(0x005FDD60, 0x1B1, VECTOR_INSERT_SINGLE, TRmgBoundaryVertex)
