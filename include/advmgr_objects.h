#ifndef HOMM3_ADVMGR_OBJECTS_H
#define HOMM3_ADVMGR_OBJECTS_H

#include <bitset>
#include <string>
#include <vector>

#include "armygrp.h"
#include "mapcell.h"
#include "objecttype.h"

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
    // Ordinary DC constructor; body stays in advmgr.cpp before its callers.
    type_cell_adjuster();
    ~type_cell_adjuster();
    NewmapCell* getTriggerCell(NewmapCell* mapCell, int x, int y);
    void restoreCell();

protected:
    hero* m_obscuringHero;
    boat* m_obscuringBoat;
    hero* m_mobileHero;
};
SIZE(type_cell_adjuster, 0xc);

enum ECompleteDrawFps {
    COMPLETE_DRAW_FPS_FRAME_COUNT = 100
};


extern int g_completeDrawFpsFrame;
extern unsigned long g_completeDrawFpsLastTime;
extern int
    g_completeDrawFpsTimes[COMPLETE_DRAW_FPS_FRAME_COUNT];
extern char g_completeDrawFpsText[];
extern char g_completeDrawFpsFormat[];

// GARRISON's two object subtypes (getSoundId's garrison and anti-magic
// garrison sounds). Project names; no native enumerator survives.
enum EGarrisonType {
    GARRISON_NORMAL = 0,
    GARRISON_ANTI_MAGIC = 1
};

// The dwellings of getSoundId's generator switch that no wider creature
// enum names, each named by its generator sound. NH3API spellings,
// Complete numbering; kept narrow like ai.h's roster.
enum EGetSoundCreatureType {
    CREATURE_SWORDSMAN = 6,
    CREATURE_CENTAUR = 14,
    CREATURE_UNICORN = 24,
    CREATURE_GREMLIN = 28,
    CREATURE_GIANT = 40,
    CREATURE_HELL_HOUND = 46,
    CREATURE_PIT_FIEND = 50,
    CREATURE_MANTICORE = 80,
    CREATURE_GOBLIN = 84,
    CREATURE_WOLF_RIDER = 86,
    CREATURE_OGRE = 90,
    CREATURE_ROC = 92,
    CREATURE_BEHEMOTH = 96,
    CREATURE_GNOLL = 98,
    CREATURE_GORGON = 102
};

extern const TCreatureType g_creatureGenerator1Types[];
extern TCreatureType g_creatureGenerator4Types[][4];

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

#endif /* HOMM3_ADVMGR_OBJECTS_H */
