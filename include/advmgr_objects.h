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

    hero* obscuring_hero;
    boat* obscuring_boat;
    hero* mobile_hero;

    ~type_cell_adjuster();
    NewmapCell* get_trigger_cell(NewmapCell* map_cell, int x, int y);
};
SIZE(type_cell_adjuster, 0xc);

enum ECompleteDrawFps {
    COMPLETE_DRAW_FPS_FRAME_COUNT = 100
};

class CChatManager;
void __cdecl UpdateCompleteDrawFps(CChatManager* manager, const char* text);

DATA(0x0065f690) extern int gCompleteDrawFpsFrame;
DATA(0x00691240) extern unsigned long gCompleteDrawFpsLastTime;
DATA(0x0069136c) extern int
    gCompleteDrawFpsTimes[COMPLETE_DRAW_FPS_FRAME_COUNT];
DATA(0x006912ec) extern char gCompleteDrawFpsText[];
DATA(0x00660388) extern char gCompleteDrawFpsFormat[];

// These source-private renderer layouts are intentionally separate from the
// broad canonical map types: VC6 changes register allocation in the three
// large drawing bodies when their type identities are merged.
struct AdvObjectCellView {
    unsigned short objectIndex;
    unsigned char offsets;
    signed char layer;
};
SIZE(AdvObjectCellView, 4);

#pragma pack(push, 1)
struct AdvMapCellObjectsView {
    char pad_00[0xe];
    std::vector<AdvObjectCellView> objects;
};
#pragma pack(pop)
SIZE(AdvMapCellObjectsView, 0x1e);

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

DATA(0x0063d570) extern TCreatureType gCreatureGenerator1Types[];
DATA(0x00677938) extern TCreatureType gCreatureGenerator4Types[][4];

class CObjectType;

#ifdef HOMM3_GAME_OBJ_DECLS
int __fastcall Random(int minimum, int maximum);
#endif

class CObject {
public:
#ifdef HOMM3_MAPCELL_OBJECTS_VIEW
    // readScholarData reaches the scholar lanes of this dword directly -
    // it switches on a SIGNED three-bit award (`shl 0x1d / sar 0x1d`),
    // which no mask spelling over the plain dword produces. Only that one
    // arm is carried here; the other five typed views stay events-only.
    union {
        unsigned long extraInfo;
        ScholarInfo scholar_info;
    };
#else
    unsigned long extraInfo;
#endif
    unsigned char x;
    unsigned char y;
    unsigned char z;
    unsigned char pad_07;
    unsigned short typeIndex;
    unsigned char animationOffset;
    unsigned char pad_0b;

#ifdef HOMM3_GAME_OBJ_DECLS
    // MapCell.h:595. game::InsertObject byte-proves this header body: the
    // coordinates narrow to bytes, type starts at zero, extra info remains a
    // dword, and each dynamic object receives a random animation phase.
    CObject(unsigned char newX, unsigned char newY, unsigned char newZ,
            unsigned short newType, unsigned long newExtraInfo)
    {
        x = newX;
        y = newY;
        z = newZ;
        typeIndex = newType;
        extraInfo = newExtraInfo;
        animationOffset = static_cast<unsigned char>(Random(0, 255));
    }
#endif

    CObjectType* get_object_type_ptr();
    void FindTrigger(int* resultX, int* resultY);
};
SIZE(CObject, 0xc);

class CObjectType {
public:
    // The DC field list names every member of this record - ImageName,
    // Width, Height, then the FOUR 48-cell masks PlacementMask,
    // PassableMask, ShadowMask, TriggerMask, then Type/Extra/IsUnderlay -
    // at DC offsets 0/12/13/16/24/32/40/48/52/56. Retail widens the leading
    // string from 12 to 16 bytes and every offset after it moves by four,
    // which saveObjectType then confirms one Write at a time.
    std::basic_string<char, std::char_traits<char>, std::allocator<char> >
        ImageName;
    signed char width;
    signed char height;
    char pad_12[2];
    std::bitset<48> drawCells;
    // +0x1c, sliced out of the old pad: saveObjectType packs FOUR masks,
    // not three, and this is the second of them. DC name PassableMask; the
    // spelling follows its three siblings here rather than the DC's.
    std::bitset<48> passableCells;
    std::bitset<48> shadowCells;
    // Fourth 48-cell mask, byte-proven at +0x2c by FindTrigger. The prior
    // padding spelling incorrectly conflated it with shadowCells at +0x24.
    std::bitset<48> triggerCells;
    char pad_34[4];
    // loadObjectType stores the serialized type as a FULL DWORD - the
    // stream carries two bytes and retail widens them into all four of
    // +0x38..+0x3b. Overlaid rather than cast, exactly as NewmapCell's own
    // type field is: a cast here would be the tree's first cast into an
    // enum domain.
    //
    // GATED to the one view that deserializes the record. Unconditional,
    // the extra declarator moved events.obj's monsters_sell_out 100.0 ->
    // 99.95 through the include-set sensitivity class (measured 2026-08-20)
    // with no semantic change anywhere; the layout is identical either way.
#ifdef HOMM3_MAPCELL_OBJECTS_VIEW
    union {
        TAdventureObjectType objectType;
        unsigned long objectTypeValue;
    };
#else
    TAdventureObjectType objectType;
#endif
    int extra;
    unsigned char suppressDraw;
    char pad_41[3];
};
SIZE(CObjectType, 0x44);

struct AdvFullMapObjectsView {
    char pad_00[4];
    CObjectType* objectTypes;
    char pad_08[0xc];
    CObject* objects;
    char pad_18[0xc];
    CSprite** sprites;
};

#endif /* HOMM3_ADVMGR_OBJECTS_H */
