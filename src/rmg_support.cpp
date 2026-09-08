// rmg_support.cpp - retained Complete random-map helper bodies.
//
// Retail keeps these ordinary helpers out of line in CreateRiver.  Their
// declarations remain visible through rmg.h, while placing the definitions in
// this companion translation unit reproduces the natural body-visibility
// boundary without source-false inline controls.
#include <va.h>
#include <math.h>
#include "rmg.h"
#include "rmg_terrain.h"

// The Complete-only painter pattern tables are built by cinit 0x55ed70 and
// 0x55f2f0 from the constant pattern-id arrays at 0x641140 and 0x6411ac.
DATA(0x0069E5D0) extern TRmgLinePatternTable g_rmgRiverPatternTable;
DATA(0x0069E650) extern TRmgLinePatternTable g_rmgRoadPatternTable;

// Both table cleanup thunks tail-call this body. The paired constructor owns
// only the copied pattern-id array at +4; the nine index/count pairs are plain
// integers and need no cleanup.
VA(0x004F9CA0, 0x0B)  // cinit cleanups 0x55ed90/0x55f310; Complete-only
TRmgLinePatternTable::~TRmgLinePatternTable()
{
    delete[] m_patterns;
}

// Retail retains this tiny value constructor throughout the RMG pathfinding
// cluster.  Its three stores and `ret 0xc` fix both the by-value ABI and the
// 12-byte position layout.
VA(0x005355C0, 0x1A)  // retail RMG caller cluster; Complete-only helper
TRmgMapPosition::TRmgMapPosition(int newX, int newY, int newZ)
    : m_x(newX), m_y(newY), m_z(newZ)
{
}

// The two-dimensional accessor is retained by the RMG search initializers.
// The 0x30 scale independently proves TRmgMapItem's stride.
VA(0x00546990, 0x1E)  // retail RMG caller cluster; Complete-only helper
TRmgMapItem* type_random_map::getMapItem(int x, int y)
{
    return m_mapItems + y * m_mapWidth + x;
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
void* TRmgLinePainter::getPattern(int)
{
    return &g_rmgRiverPatternTable;
}

// Slot 1 makes a fieldwise tile copy before forwarding its address to the
// adapter. That copy deliberately omits the two tail-padding bytes and the
// indirect call agrees with all 54 retail bytes; all four painter vtables
// share this ICF representative.
VA(0x0055EDC0, 0x36)  // vtables 0x641174/0x641190/0x6411f0/0x64120c slot 1
void TRmgLinePainter::setTile(
    const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    rmgTerrainTile copiedTile;
    copiedTile.m_terrain = tile.m_terrain;
    copiedTile.m_frame = tile.m_frame;
    copiedTile.m_flipX = tile.m_flipX;
    copiedTile.m_flipY = tile.m_flipY;
    m_adapter->setTile(point, copiedTile);
}

void TRmgLinePainter::setOverlay(const TRmgGridPoint& point, int value)
{
    m_adapter->setOverlay(point, value);
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
void* TRmgRoadLinePainter::getPattern(int)
{
    return &g_rmgRoadPatternTable;
}

void TRmgRoadLinePainter::setTile(
    const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    rmgTerrainTile copiedTile;
    copiedTile.m_terrain = tile.m_terrain;
    copiedTile.m_frame = tile.m_frame;
    copiedTile.m_flipX = tile.m_flipX;
    copiedTile.m_flipY = tile.m_flipY;
    m_adapter->setTile(point, copiedTile);
}

// The river and road line-painter vtables all share this ICF representative.
// Loading the adapter at +0xc and dispatching its slot +8 proves setOverlay's
// two-argument forwarding body; the road COMDAT wins retail link order.
VA(0x0055F330, 0x17)  // vtables 0x641174/0x641190/0x6411f0/0x64120c; Complete-only
void TRmgRoadLinePainter::setOverlay(const TRmgGridPoint& point, int value)
{
    m_adapter->setOverlay(point, value);
}

// Both painter hierarchies share this slot-4 body. The caller at 0x4f9fdd
// passes an explicit destination after the point; the nested map-adapter call
// returns a value through its separate hidden first argument. This is not a
// painter value-return ABI. The original Complete-only spelling is unknown.
// Exact with the recovered ABI: reference-bound, named-value, assigned-value,
// and reversed first-two-store controls all reproduce the same 52 bytes.
VA(0x0055F350, 0x34) // vtables 0x641174/0x641190/0x6411f0/0x64120c slot 4
void TRmgLinePainter::getTile(const TRmgGridPoint& point, rmgTerrainTile& tile)
{
    const rmgTerrainTile& sourceTile = m_adapter->getTile(point);
    tile.m_terrain = sourceTile.m_terrain;
    tile.m_frame = sourceTile.m_frame;
    tile.m_flipX = sourceTile.m_flipX;
    tile.m_flipY = sourceTile.m_flipY;
}

void TRmgRoadLinePainter::getTile(const TRmgGridPoint& point, rmgTerrainTile& tile)
{
    const rmgTerrainTile& sourceTile = m_adapter->getTile(point);
    tile.m_terrain = sourceTile.m_terrain;
    tile.m_frame = sourceTile.m_frame;
    tile.m_flipX = sourceTile.m_flipX;
    tile.m_flipY = sourceTile.m_flipY;
}

// The road hierarchy's parallel vtables 0x6411f0/0x64120c use the same
// adapter getLand forwarding shape in slot 5.
VA(0x0055F390, 0x13)  // Complete-only road painter
int TRmgRoadLinePainter::getLand(const TRmgGridPoint& point)
{
    return m_adapter->getLand(point);
}

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

// The subdivision owns every allocated half-edge and its pointer vector.
// Its retained destructor proves the +0x04 vector and trivial edge cleanup.
VA(0x005FD330, 0x58) // anchor-callee 0x53e685; thiscall, ret 0
TRmgVoronoi::~TRmgVoronoi()
{
    for (int edge = 0; edge < m_edges.size(); ++edge)
        delete m_edges[edge];
}
