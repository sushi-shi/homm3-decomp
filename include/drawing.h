// drawing.h - prototypes of drawing.cpp (compiland drawing.obj)
#ifndef HOMM3_DRAWING_H
#define HOMM3_DRAWING_H

#include "cmbtmgr.h"

// DrawOccupant's priority sentinels: 8 bypasses the army-priority filter,
// while 7 performs the first draw but suppresses the moat/redraw pass.
// Before normalization (type): ECombatDrawPriority.
enum CombatDrawPriority {
    COMBAT_DRAW_PRIORITY_WALL = 0,
    COMBAT_DRAW_PRIORITY_CORPSE = 1,
    COMBAT_DRAW_PRIORITY_OBSTACLE = 2,
    COMBAT_DRAW_PRIORITY_SINGLE_PASS = 7,
    COMBAT_DRAW_PRIORITY_ANY = 8
};

// Before normalization (type): ECombatWallDrawingConstants.
enum CombatWallDrawingConstants {
    COMBAT_WALL_HEX_WIDTH = 44,
    COMBAT_ARCHER_X_BIAS = 196,
    COMBAT_ARCHER_Y_BIAS = 267,
    COMBAT_ARCHER_DEFENDING_SIDE = 1,
    COMBAT_ARCHER_ACTIVE_SEQUENCE = 2,
    COMBAT_ARCHER_DOUBLE_WIDE_ATTRIBUTE = 1
};

// Retail's battlefield indexing: eleven rows of seventeen cells, with the
// first and last column reserved as off-grid borders.
// Before normalization (type): ECombatGridDimensions.
enum CombatGridDimensions {
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
// Read at eighteen sites image-wide (config/retail-reloc-evidence.tsv)
// and NOT owned by drawing.obj: no admitted TU defines it yet, so this
// is a reader-side declaration parked in the nearest combat-drawing
// header, the way winmgr.h carries DoDialog's three unowned dialog
// globals. The NAME is a source-facing invention; the address, extent
// and contents are read straight from the hash-verified image.
extern const float g_combatSpeedFactors[3];

// --- globals ---
// CODEVIEW(E:\gamedcs\drawing.cpp:47, dc 0x831b4) void get_creature_spell_message(char* buffer, const army* current_army, long current_hex);
// CODEVIEW(E:\gamedcs\drawing.cpp:2662, dc 0x87278) void Rescale(int* x, int* y, unsigned char offset);

// --- CChatManager ---
// CODEVIEW(E:\gamedcs\remote.h:326, dc 0x87620) unsigned char CChatManager::ChatChanged();

// --- CSprite ---
// CODEVIEW(E:\gamedcs\CSprite.h:151, dc 0x87350) int CSprite::GetCroppedHeight(int seq, int frame);
// CODEVIEW(E:\gamedcs\CSprite.h:342, dc 0x87394) void CSprite::DrawCreature(int seqnum, int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip, unsigned short outcolor);
// CODEVIEW(E:\gamedcs\CSprite.h:348, dc 0x87438) void CSprite::DrawCreatureAlpha(int seqnum, int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip, unsigned short outcolor);
// CODEVIEW(E:\gamedcs\CSprite.h:444, dc 0x874dc) void CSprite::DrawCombatHero(int seqnum, int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip);
// CODEVIEW(E:\gamedcs\CSprite.h:450, dc 0x8757c) void CSprite::DrawSpellEffect(int seqnum, int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip, unsigned char alpha);

// --- CSpriteFrame ---
// CODEVIEW(E:\gamedcs\CSpriteFrame.h:88, dc 0x8734c) int CSpriteFrame::GetCroppedHeight();

// --- SLimitData ---
// CODEVIEW(E:\gamedcs\struct.h:284, dc 0x872a8) bool SLimitData::Intersects(const SLimitData& check_limits) const;
// CODEVIEW(E:\gamedcs\struct.h:300, dc 0x872dc) bool SLimitData::IsEmpty() const;

// --- TCombatHeroSubWindow ---
// CODEVIEW(E:\gamedcs\CombatControlSubWindow.h:148, dc 0x87344) unsigned char TCombatHeroSubWindow::IsShown();

// --- army ---
// CODEVIEW(E:\gamedcs\Army.h:881, dc 0x872f4) bool army::is_in_area_highlight();

// --- combatManager ---
// CODEVIEW(E:\gamedcs\drawing.cpp:326, dc 0x838f0) void combatManager::CombatMessage(int command);
// CODEVIEW(E:\gamedcs\drawing.cpp:492, dc 0x83e58) void combatManager::UpdateCombatArea();
// CODEVIEW(E:\gamedcs\drawing.cpp:506, dc 0x83e8c) void combatManager::FullUpdate();
// CODEVIEW(E:\gamedcs\drawing.cpp:513, dc 0x83ec0) void combatManager::UpdateCombatArea(SLimitData area);
// CODEVIEW(E:\gamedcs\drawing.cpp:520, dc 0x83ee8) void combatManager::UpdateCombatArea(int x, int y, int width, int height);
// CODEVIEW(E:\gamedcs\drawing.cpp:554, dc 0x83f84) unsigned char combatManager::ScrollCombatArea(int dx, int dy, unsigned char abs, unsigned char draw);
// CODEVIEW(E:\gamedcs\drawing.cpp:598, dc 0x8405c) unsigned char combatManager::ScrollTo(SLimitData extent, unsigned char draw, unsigned char doscroll_x, unsigned char doscroll_y);
// CODEVIEW(E:\gamedcs\drawing.cpp:666, dc 0x841d4) unsigned char combatManager::ScrollTo(int x, int y, unsigned char draw, unsigned char doscroll_x, unsigned char doscroll_y);
// CODEVIEW(E:\gamedcs\drawing.cpp:672, dc 0x84228) unsigned char combatManager::ScrollToPixel(int x, int y, unsigned char draw);
// CODEVIEW(E:\gamedcs\drawing.cpp:679, dc 0x84248) unsigned char combatManager::ScrollTo(int x, int y, int width, int height, unsigned char draw, unsigned char doscroll_x, unsigned char doscroll_y);
// CODEVIEW(E:\gamedcs\drawing.cpp:1141, dc 0x84e2c) void combatManager::DrawFrame(unsigned char update, unsigned char bLimitCreatureEffect, unsigned char bLimitDraw, int iDelay, unsigned char bRefreshBackground, unsigned char bDoDelayTil);
// CODEVIEW(E:\gamedcs\drawing.cpp:1399, dc 0x853f4) void combatManager::DrawObstacleAt(int hex_index);
// CODEVIEW(E:\gamedcs\drawing.cpp:1581, dc 0x857d4) void combatManager::DrawDeadOccupants(int index);
// CODEVIEW(E:\gamedcs\drawing.cpp:1738, dc 0x85b50) int combatManager::DrawCreatureAlpha(const CSprite* sprite, int sequence, int frame, int x, int y, SLimitData* psLimitData, unsigned char isFlipped, int iColor);

// --- hexcell ---
// CODEVIEW(E:\gamedcs\HexCell.h:85, dc 0x87300) SLimitData hexcell::limits(__$ReturnUdt);

#endif  /* HOMM3_DRAWING_H */
