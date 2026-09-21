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

// combatManager::CombatAreaLimits, retail .data 0x6aace8 - the value
// every accumulating draw pass resets the combat drawing extent to
// before it starts. Definition and DATA claim are src/drawing.cpp's;
// declared here because fly.obj resets the extent through it once per
// flight frame.
extern TDrawbridgeBounds g_combatAreaLimits;
// DC names the 58,86..740,557 clip rectangle GridAreaLimits. Retail's
// initializer at 0x462640 and UpdateGrid's four clamps prove the aggregate;
// its storage belongs to cmbtmgr.obj and this TU only references it.
DATA(0x00694ec8) extern SLimitData g_combatGridAreaLimits;

// The three combat animation speed multipliers at .rdata 0x63cf7c -
// 1.0f, 0.63f and 0.4f exactly - indexed by gUnnamed698758.combatSpeed.
// Read at eighteen sites image-wide (config/retail/reloc-evidence.tsv)
// and NOT owned by drawing.obj: no admitted TU defines it yet, so this
// is a reader-side declaration parked in the nearest combat-drawing
// header, the way winmgr.h carries DoDialog's three unowned dialog
// globals. The NAME is a source-facing invention; the address, extent
// and contents are read straight from the hash-verified image.
extern const float g_combatSpeedFactors[3];

#endif  /* HOMM3_DRAWING_H */
