#ifndef HOMM3_ADVMGR_OBJECTS_H
#define HOMM3_ADVMGR_OBJECTS_H

#include <bitset>
#include <string>
#include <vector>
#include "armygrp.h"
#include "mapcell.h"

class CSprite;
class textWidget;
class hero;
class boat;

// Dreamcast CodeView names the three pointer members and fixes this helper at
// 12 bytes. Retail's destructor and get_trigger_cell body independently
// confirm the same offsets and pointer roles.
class type_cell_adjuster {
public:
    enum {
        MOBILE_HERO_CELL_X = 9,
        MOBILE_HERO_CELL_Y = 8
    };

    // Before normalization: obscuring_hero.
    hero* m_obscuringHero;
    // Before normalization: obscuring_boat.
    boat* m_obscuringBoat;
    // Before normalization: mobile_hero.
    hero* m_mobileHero;

    // Ordinary DC constructor; body stays in advmgr.cpp before its callers.
    type_cell_adjuster();
    ~type_cell_adjuster();
    // Before normalization (function): type_cell_adjuster::get_trigger_cell.
    // Before normalization (locals): map_cell.
    NewmapCell* getTriggerCell(NewmapCell* mapCell, int x, int y);
    // Before normalization (function): type_cell_adjuster::restore_cell.
    void restoreCell();
};
SIZE(type_cell_adjuster, 0xc);

enum ECompleteDrawFps {
    COMPLETE_DRAW_FPS_FRAME_COUNT = 100
};

class CChatManager;
// Before normalization (function): UpdateCompleteDrawFps.
void __cdecl updateCompleteDrawFps(CChatManager* manager, const char* text);

// Before normalization: gCompleteDrawFpsFrame.
// Before normalization: gCompleteDrawFpsLastTime.
DATA(0x0065f690) extern int g_completeDrawFpsFrame;
DATA(0x00691240) extern unsigned long g_completeDrawFpsLastTime;
DATA(0x0069136c) extern int
    // Before normalization: gCompleteDrawFpsTimes.
    g_completeDrawFpsTimes[COMPLETE_DRAW_FPS_FRAME_COUNT];
// Before normalization: gCompleteDrawFpsText.
// Before normalization: gCompleteDrawFpsFormat.
DATA(0x006912ec) extern char g_completeDrawFpsText[];
DATA(0x00660388) extern char g_completeDrawFpsFormat[];

// Renderer object access uses canonical NewmapCell::TObjectCell and
// NewfullMap vectors. The former AdvMapCellObjectsView's +0xe vector
// and AdvFullMapObjectsView's +4/+0x14/+0x24 pointers were the same
// retail vector storage, not separate object types or padding fields.

enum EGetSoundObjectIndex {
    GET_SOUND_BANK_0 = 0,
    GET_SOUND_BANK_1,
    GET_SOUND_BANK_2,
    GET_SOUND_BANK_3,
    GET_SOUND_BANK_4,
    GET_SOUND_BANK_5,
    GET_SOUND_BANK_6,
    GET_SOUND_GARRISON_0 = 0,
    GET_SOUND_GARRISON_1,
    GET_SOUND_GENERATOR4_0 = 0,
    GET_SOUND_GENERATOR4_1,
    GET_SOUND_MINE_0 = 0,
    GET_SOUND_MINE_1,
    GET_SOUND_MINE_2,
    GET_SOUND_MINE_3,
    GET_SOUND_MINE_4,
    GET_SOUND_MINE_5,
    GET_SOUND_MINE_6
};

// Only the creature ids selected by GetSoundId's retail switch are named.
// Ordinal spellings avoid importing an external semantic creature roster.
enum EGetSoundCreatureId {
    GET_SOUND_CREATURE_000 = 0,
    GET_SOUND_CREATURE_002 = 2,
    GET_SOUND_CREATURE_004 = 4,
    GET_SOUND_CREATURE_008 = 8,
    GET_SOUND_CREATURE_010 = 10,
    GET_SOUND_CREATURE_012 = 12,
    GET_SOUND_CREATURE_014 = 14,
    GET_SOUND_CREATURE_016 = 16,
    GET_SOUND_CREATURE_018 = 18,
    GET_SOUND_CREATURE_020 = 20,
    GET_SOUND_CREATURE_022 = 22,
    GET_SOUND_CREATURE_024 = 24,
    GET_SOUND_CREATURE_026 = 26,
    GET_SOUND_CREATURE_028 = 28,
    GET_SOUND_CREATURE_030 = 30,
    GET_SOUND_CREATURE_034 = 34,
    GET_SOUND_CREATURE_036 = 36,
    GET_SOUND_CREATURE_038 = 38,
    GET_SOUND_CREATURE_040 = 40,
    GET_SOUND_CREATURE_042 = 42,
    GET_SOUND_CREATURE_044 = 44,
    GET_SOUND_CREATURE_046 = 46,
    GET_SOUND_CREATURE_048 = 48,
    GET_SOUND_CREATURE_050 = 50,
    GET_SOUND_CREATURE_052 = 52,
    GET_SOUND_CREATURE_054 = 54,
    GET_SOUND_CREATURE_056 = 56,
    GET_SOUND_CREATURE_058 = 58,
    GET_SOUND_CREATURE_060 = 60,
    GET_SOUND_CREATURE_062 = 62,
    GET_SOUND_CREATURE_064 = 64,
    GET_SOUND_CREATURE_066 = 66,
    GET_SOUND_CREATURE_068 = 68,
    GET_SOUND_CREATURE_070 = 70,
    GET_SOUND_CREATURE_072 = 72,
    GET_SOUND_CREATURE_074 = 74,
    GET_SOUND_CREATURE_076 = 76,
    GET_SOUND_CREATURE_078 = 78,
    GET_SOUND_CREATURE_080 = 80,
    GET_SOUND_CREATURE_082 = 82,
    GET_SOUND_CREATURE_084 = 84,
    GET_SOUND_CREATURE_086 = 86,
    GET_SOUND_CREATURE_088 = 88,
    GET_SOUND_CREATURE_090 = 90,
    GET_SOUND_CREATURE_092 = 92,
    GET_SOUND_CREATURE_094 = 94,
    GET_SOUND_CREATURE_096 = 96,
    GET_SOUND_CREATURE_098 = 98,
    GET_SOUND_CREATURE_100 = 100,
    GET_SOUND_CREATURE_102 = 102,
    GET_SOUND_CREATURE_104 = 104,
    GET_SOUND_CREATURE_106 = 106,
    GET_SOUND_CREATURE_108 = 108,
    GET_SOUND_CREATURE_110 = 110,
    GET_SOUND_CREATURE_112 = 112,
    GET_SOUND_CREATURE_113 = 113,
    GET_SOUND_CREATURE_114 = 114,
    GET_SOUND_CREATURE_115 = 115
};

// Before normalization: gCreatureGenerator1Types.
// Before normalization: gCreatureGenerator4Types.
DATA(0x0063d570) extern TCreatureType g_creatureGenerator1Types[];
DATA(0x00677938) extern TCreatureType g_creatureGenerator4Types[][4];

class CObjectType;

// readObjectType's two success answers. It returns 1 normally and 100 when
// the object's own .msk resource was missing and default.msk stood in -
// `neg bl / sbb ebx,ebx / and ebx,0x63 / inc ebx` at its tail, 0x63 + 1.
// readMapObjects cases on the second value to collect the type indices it
// then reports against. GATED to the one TU that reads it.
enum EReadObjectTypeResult {
    READ_OBJECT_TYPE_OK = 1,
    READ_OBJECT_TYPE_DEFAULT_MASK = 100
};


// Complete object-template records are owned by the object-type module.
#include "objecttype.h"

#endif /* HOMM3_ADVMGR_OBJECTS_H */
