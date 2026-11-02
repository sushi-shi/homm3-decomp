// tiles.cpp - the map-grid neighbourhood helper compiland: the eight
// clockwise direction offsets and the edge mask that says which of them
// exist for a given cell.

// THIS COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER; see tiles.h for why the
// name is an inference the link order bounds rather than proves. Its whole
// .text contribution is the one body below plus two rows of the excluded
// initializer class - the 32-byte atexit guard at 0x5bc870 and the direction
// table's own dynamic initializer at 0x5bc890, which is nothing but eight
// inlined TPoint(int, int) constructors writing 0x6a80a8..0x6a80e7.

// Four retail bodies in two other compilands call the mask builder and walk
// the table straight afterwards: 0x4f9f00 and 0x4fa3c0 (the compiland between
// levelupwindow.obj and lodfile.obj) and TRmgTerrainPainter's 0x5b4b20 and
// 0x5b5440. 0x4f9f00 is the proof of the pairing - it calls 0x5bc910 with the
// mask on its own frame, then loops `edx` from 0x6a80a8 to 0x6a80e8 in steps
// of eight while indexing that same mask by the loop counter.
#include "va.h"

#include <assert.h>
#include <algorithm>

#include "tiles.h"

#include "rmg.h"

// Retail .bss 0x6a80a8, sixteen dwords: (0,-1) (1,-1) (1,0) (1,1) (0,1)
// (-1,1) (-1,0) (-1,-1). TPoint is non-POD, so the values arrive through the
// compiland's own dynamic initializer rather than from .data - which is why
// the array is in .bss at all.
DATA(0x006a80a8)
const TPoint<int> akAdjOffset[TILE_DIR_COUNT] = {
    TPoint<int>(0, -1),  TPoint<int>(1, -1),  TPoint<int>(1, 0),   TPoint<int>(1, 1),
    TPoint<int>(0, 1),   TPoint<int>(-1, 1),  TPoint<int>(-1, 0),  TPoint<int>(-1, -1)
};

// Loki h3maped TilePoint.cpp's body, also the map editor's.
VA(0x005BC910, 0x7D)
MAC_ADDRESS(0x25bcd4, 0xf8)
void computeAdjacentDirs(unsigned int width, unsigned int height,
                         unsigned int x, unsigned int y,
                         bool (&abDir)[TILE_DIR_COUNT])
{
    assert(width > 0);
    assert(height > 0);
    assert(x >= 0 && x < width);
    assert(y >= 0 && y < height);
    assert(abDir != NULL);

    std::fill(abDir, abDir + 8, true);
    if (y == 0)
        abDir[0] = abDir[7] = abDir[1] = false;
    else if (y == height - 1)
        abDir[4] = abDir[5] = abDir[3] = false;
    if (x == 0)
        abDir[6] = abDir[7] = abDir[5] = false;
    else if (x == width - 1)
        abDir[2] = abDir[1] = abDir[3] = false;
}
