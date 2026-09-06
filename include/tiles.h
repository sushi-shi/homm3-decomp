// tiles.h - prototypes of tiles.cpp, the map-grid neighbourhood helper
// compiland.
//
// THE COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER (which carries no
// random-map generator at all) and its name is an inference the link order
// bounds rather than proves, exactly like campaignmusic.obj: retail's .text
// is strictly alphabetical by compiland, textwdgt.obj's last row ends at
// 0x5bc86c and town.obj opens with its own static-initializer run at
// 0x5bc990, so this object is 0x5bc870..0x5bc98d - a 32-byte atexit guard
// row, the .bss direction table's initializer, and the one body below.
// `tiles` is a spelling inside that interval that describes what the object
// holds. HAND-OWNED, provisional.
#ifndef HOMM3_TILES_H
#define HOMM3_TILES_H

#include <va.h>

// The eight neighbour directions, clockwise from north. The order is fixed by
// the retail .bss table at 0x6a80a8, whose sixteen dwords read
// (0,-1) (1,-1) (1,0) (1,1) (0,1) (-1,1) (-1,0) (-1,-1) - and by the mask
// builder below, which clears exactly the three entries whose dy is -1 on the
// top row, the three whose dy is +1 on the bottom row, the three whose dx is
// -1 in column zero and the three whose dx is +1 in the last column.
enum ETileDirection {
    TILE_DIR_NORTH = 0,
    TILE_DIR_NORTHEAST = 1,
    TILE_DIR_EAST = 2,
    TILE_DIR_SOUTHEAST = 3,
    TILE_DIR_SOUTH = 4,
    TILE_DIR_SOUTHWEST = 5,
    TILE_DIR_WEST = 6,
    TILE_DIR_NORTHWEST = 7,
    TILE_DIR_COUNT = 8
};

struct TPoint;
extern TPoint gTileDirections[TILE_DIR_COUNT];

// Retail 0x5bc910. Fills an eight-entry byte mask with "this neighbour is on
// the map", clearing the three directions each grid edge removes.
void __fastcall BuildTileNeighbourMask(int width, int height, int x, int y,
                                       unsigned char* neighbourExists);

#endif
