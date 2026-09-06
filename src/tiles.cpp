// tiles.cpp - the map-grid neighbourhood helper compiland: the eight
// clockwise direction offsets and the edge mask that says which of them
// exist for a given cell.
//
// THIS COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER; see tiles.h for why the
// name is an inference the link order bounds rather than proves. Its whole
// .text contribution is the one body below plus two rows of the excluded
// initializer class - the 32-byte atexit guard at 0x5bc870 and the direction
// table's own dynamic initializer at 0x5bc890, which is nothing but eight
// inlined TPoint(int, int) constructors writing 0x6a80a8..0x6a80e7.
//
// Four retail bodies in two other compilands call the mask builder and walk
// the table straight afterwards: 0x4f9f00 and 0x4fa3c0 (the compiland between
// levelupwindow.obj and lodfile.obj) and TRmgTerrainPainter's 0x5b4b20 and
// 0x5b5440. 0x4f9f00 is the proof of the pairing - it calls 0x5bc910 with the
// mask on its own frame, then loops `edx` from 0x6a80a8 to 0x6a80e8 in steps
// of eight while indexing that same mask by the loop counter.
#include <va.h>
#include <algorithm>

#include "tiles.h"
#include "rmg.h"

// Retail .bss 0x6a80a8, sixteen dwords: (0,-1) (1,-1) (1,0) (1,1) (0,1)
// (-1,1) (-1,0) (-1,-1). TPoint is non-POD, so the values arrive through the
// compiland's own dynamic initializer rather than from .data - which is why
// the array is in .bss at all.
DATA(0x006a80a8)
TPoint gTileDirections[TILE_DIR_COUNT] = {
    TPoint(0, -1),  TPoint(1, -1),  TPoint(1, 0),   TPoint(1, 1),
    TPoint(0, 1),   TPoint(-1, 1),  TPoint(-1, 0),  TPoint(-1, -1)
};

// Retail 0x5bc910. Every entry starts set; each of the four grid edges the
// cell sits on clears the three directions that leave the map. The two
// vertical tests and the two horizontal tests are each an if/else-if pair -
// retail jumps over the `dec` that forms `height - 1` when y is zero.
//
// The opening fill is `std::fill`, not `memset`: retail computes the byte
// count at RUNTIME as `last - first` behind a `cmp first, last / je` guard,
// which is Dinkumware's plain `for (; _F != _L; ++_F) *_F = _X;` loop after
// C2's constant-store idiom turns it into `rep stosd` + `rep stosb`. A
// constant-count `memset` folds to two dword stores instead and scores
// 47.77, and it is also what forces `width` back out to [ebp-4]: the stos
// expansion owns ecx, edi and eax across the fill.
VA(0x005BC910, 0x7D)  // anchor-callee: the four callers at 0x4f9f00, 0x4fa3c0,
                      // 0x5b4b20, 0x5b5440 walk 0x6a80a8 by this mask; retail-only
void __fastcall BuildTileNeighbourMask(int width, int height, int x, int y,
                                       unsigned char* neighbourExists)
{
    std::fill(neighbourExists, neighbourExists + TILE_DIR_COUNT,
              static_cast<unsigned char>(1));
    if (y == 0) {
        neighbourExists[TILE_DIR_NORTHEAST] = 0;
        neighbourExists[TILE_DIR_NORTHWEST] = 0;
        neighbourExists[TILE_DIR_NORTH] = 0;
    } else if (y == height - 1) {
        neighbourExists[TILE_DIR_SOUTHEAST] = 0;
        neighbourExists[TILE_DIR_SOUTHWEST] = 0;
        neighbourExists[TILE_DIR_SOUTH] = 0;
    }
    if (x == 0) {
        neighbourExists[TILE_DIR_SOUTHWEST] = 0;
        neighbourExists[TILE_DIR_NORTHWEST] = 0;
        neighbourExists[TILE_DIR_WEST] = 0;
    } else if (x == width - 1) {
        neighbourExists[TILE_DIR_SOUTHEAST] = 0;
        neighbourExists[TILE_DIR_NORTHEAST] = 0;
        neighbourExists[TILE_DIR_EAST] = 0;
    }
}
