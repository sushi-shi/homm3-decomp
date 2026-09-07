// rmg_support.cpp - retained Complete random-map helper bodies.
//
// Retail keeps these ordinary helpers out of line in CreateRiver.  Their
// declarations remain visible through rmg.h, while placing the definitions in
// this companion translation unit reproduces the natural body-visibility
// boundary without source-false inline controls.
#include <va.h>
#include <math.h>
#include "rmg.h"

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

// Cinit 0x55f2f0 builds the seventeen-entry road pattern table from the ids
// at 0x6411ac. The road painter's first virtual slot returns that table.
VA(0x0055F320, 0x08)  // vtables 0x6411f0/0x64120c; Complete-only
void* TRmgRoadLinePainter::getPattern(int)
{
    return &g_rmgRoadPatternTable;
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
