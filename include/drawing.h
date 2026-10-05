#ifndef HOMM3_DRAWING_H
#define HOMM3_DRAWING_H

#include "cmbtmgr.h"

// DrawOccupant's priority sentinels: 8 bypasses the army-priority filter,
// while 7 performs the first draw but suppresses the moat/redraw pass.
enum ECombatDrawPriority {
    COMBAT_DRAW_PRIORITY_WALL = 0,
    COMBAT_DRAW_PRIORITY_CORPSE = 1,
    COMBAT_DRAW_PRIORITY_OBSTACLE = 2,
    COMBAT_DRAW_PRIORITY_SINGLE_PASS = 7,
    COMBAT_DRAW_PRIORITY_ANY = 8
};

enum ECombatWallDrawingConstants {
    COMBAT_WALL_HEX_WIDTH = 44,
    COMBAT_ARCHER_X_BIAS = 196,
    COMBAT_ARCHER_Y_BIAS = 267,
    COMBAT_ARCHER_DEFENDING_SIDE = 1,
    COMBAT_ARCHER_ACTIVE_SEQUENCE = 2,
    COMBAT_ARCHER_DOUBLE_WIDE_ATTRIBUTE = 1
};

// Retail's battlefield indexing: eleven rows of seventeen cells, with the
// first and last column reserved as off-grid borders.
enum ECombatGridDimensions {
    COMBAT_GRID_COLUMN_COUNT = 17,
    COMBAT_GRID_RIGHT_BORDER_COLUMN = 16,
    COMBAT_GRID_HEX_COUNT = 187
};

#endif  /* HOMM3_DRAWING_H */
