// rmg_support.cpp - retained Complete random-map helper bodies.
//
// Retail keeps these ordinary helpers out of line in CreateRiver.  Their
// declarations remain visible through rmg.h, while placing the definitions in
// this companion translation unit reproduces the natural body-visibility
// boundary without source-false inline controls.
#include <va.h>
#include "rmg.h"

// Retail retains this tiny value constructor throughout the RMG pathfinding
// cluster.  Its three stores and `ret 0xc` fix both the by-value ABI and the
// 12-byte position layout.
VA(0x005355C0, 0x1A)  // retail RMG caller cluster; Complete-only helper
TRmgMapPosition::TRmgMapPosition(int newX, int newY, int newZ)
    : x(newX), y(newY), z(newZ)
{
}

// The two-dimensional accessor is retained by the RMG search initializers.
// The 0x30 scale independently proves TRmgMapItem's stride.
VA(0x00546990, 0x1E)  // retail RMG caller cluster; Complete-only helper
TRmgMapItem* type_random_map::GetMapItem(int x, int y)
{
    return mapItems + y * mapWidth + x;
}

// The river painter deliberately inherits the generic line walker as its
// second base.  Retail's calls use `this + 0x10`, which is the natural VC6
// adjustment for that source relationship.  Its otherwise-empty destructor
// restores the first base's vtable and is retained out of line.
VA(0x0055EDA0, 0x07)  // CreateRiver EH cleanup; retail-only RMG helper
TRmgRiverPainter::~TRmgRiverPainter()
{
}

VA(0x0055EE50, 0x76)  // CreateRiver sole caller; retail-only RMG helper
TRmgRiverPainter::TRmgRiverPainter(
    TRmgMapAdapterInterface* newAdapter,
    int newRiverType,
    const TPoint& newStart)
    : TRmgLinePainter(newAdapter),
      TRmgLineWalker(this, newRiverType, newStart)
{
}
