// tiles.h - prototypes of tiles.cpp, the map-grid neighbourhood helper
// compiland.

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

#include "va.h"

#include "rmg.h"

#include "tile_direction.h"

extern TPoint g_tileDirections[TILE_DIR_COUNT];

void __fastcall buildTileNeighbourMask(int width, int height, int x, int y,
                                       unsigned char* neighbourExists);

#endif
