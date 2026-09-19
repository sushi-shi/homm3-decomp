#ifndef HOMM3_CURSOR_H
#define HOMM3_CURSOR_H

class CMapChange;

enum ECursorMoveTuning {
    CURSOR_INSTANT_WALK_SPEED = 4,
    CURSOR_IRREGULAR_STEP_PIXELS = 10,
    CURSOR_IRREGULAR_MIDDLE_STEP_PIXELS = 12,
    CURSOR_TILE_PIXELS = 32
};

// --- globals ---
void sendMapChange(CMapChange* mapChange);  // 0x482390, dc 0x7c9f8

#endif  /* HOMM3_CURSOR_H */
