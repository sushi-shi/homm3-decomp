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

// readObjectType's two success answers. It returns 1 normally and 100 when
// the object's own .msk resource was missing and default.msk stood in -
// `neg bl / sbb ebx,ebx / and ebx,0x63 / inc ebx` at its tail, 0x63 + 1.
// readMapObjects cases on the second value to collect the type indices it
// then reports against. GATED to the one TU that reads it.
enum EReadObjectTypeResult {
    READ_OBJECT_TYPE_OK = 1,
    READ_OBJECT_TYPE_DEFAULT_MASK = 100
};


// Map-editor/RMG object template consumed by the retail-identical
// CObjectType conversion constructor at 0x506080. The public names are from
// the HD structural bridge; retail independently fixes the 0x4c stride and
// every offset read by that constructor.
struct TObjectType {
    struct TPoint {
        int x;
        int y;
    };
    struct TImageInfo {
        // Provisional overload: setImageName initializes the point before
        // either bitset constructor. TObjectType's default construction
        // leaves that point uninitialized, requiring a distinct size path.
        TImageInfo() {}
        explicit TImageInfo(const TPoint& size) : objectSize(size) {}

        TPoint objectSize;
        std::bitset<48> drawMask;
        std::bitset<48> shadowMask;
    };

    // The default constructor load()'s `objectTypes.resize(count)` builds
    // its `_Ty()` temporary from, published one store at a time at
    // 0x514e2a-0x514ea1: every member in declaration order, an ALL-SET
    // passable mask spelled as a flipped `bitset<48>(0)`, and the {8,6}
    // no-trigger sentinel. imageInfo's TPoint stays uninitialized there,
    // which is why it has no initializer here either.
    TObjectType();

    int imageNumber;
    std::bitset<48> passableMask;
    std::bitset<48> triggerMask;
    std::bitset<10> terrainMask;
    std::bitset<10> recommendedTerrainMask;
    TAdventureObjectType objectType;
    int subtype;
    int slotCategory;
    unsigned char isUnderlay;
    unsigned char hasTrigger;
    TPoint triggerCell;
    TImageInfo imageInfo;

    // Retail 0x514960, thiscall with no arguments. DECLARED ONLY: the body
    // belongs to the unadmitted Complete-only .msk/objects compiland in the
    // newgame..overview gap, where two function-local statics cache a table
    // of image records and this member returns the +0x0c string of the row
    // `imageNumber` selects (or a static empty string when the index is out
    // of range). The ROLE is proven by its one caller - the result is what
    // CObjectType's conversion constructor assigns into ImageName - and the
    // NAME follows the role; nothing attests it.
    const std::basic_string<char, std::char_traits<char>,
                            std::allocator<char> >& GetImageName();

    // CObjectType's conversion loads each dimension as a dword before
    // narrowing it to char. Direct field access folds those into byte
    // loads in VC6; ordinary integer accessors retain the observed boundary.
    // Their role names are provisional: this editor type is Complete-only.
    int GetWidth() const { return imageInfo.objectSize.x; }
    int GetHeight() const { return imageInfo.objectSize.y; }

    // Retail 0x514610 and 0x514a60, both in the same Complete-only
    // compiland and both returning *this - the per-row `>>` at 0x514b80
    // chains them off each other's result. setImageName resolves the
    // record's name through the image-name registry into `imageNumber`
    // (and, on a miss, loads the row's .msk to append one); setTriggerMask
    // stores `mask & ~passableMask`, sets `hasTrigger` from its any(), and
    // scans the 8x6 grid for the first set cell. NAMES ARE PROVISIONAL.
    TObjectType& setImageName(
        const std::basic_string<char, std::char_traits<char>,
                                std::allocator<char> >& name);
    TObjectType& setTriggerMask(const std::bitset<48>& mask);
};
SIZE(TObjectType, 0x4c);

// The "no trigger cell" sentinel, {8, 6} - the object mask grid's own
// dimensions - in .rdata at 0x640278. Both of its consumers, the default
// constructor above and TObjectType::setTriggerMask's else arm, issue both
// loads before either store. No compiland in the tree defines it yet.
extern const TObjectType::TPoint gNoTriggerCell;

// Shared header definition for the resize default value. Retail expands
// this constructor, which does not establish an explicit inline keyword:
// an ordinary definition in objecttype.cpp was byte-flat (2026-09-06).
inline TObjectType::TObjectType()
    : imageNumber(0),
      passableMask(~std::bitset<48>(0)),
      objectType(NOTHING),
      subtype(0),
      slotCategory(0),
      isUnderlay(0),
      hasTrigger(0),
      triggerCell(gNoTriggerCell)
{
}

class TObjectTypeTable {
public:
    std::vector<TObjectType> objectTypes;
    void load(char* filename);
};
SIZE(TObjectTypeTable, 0x10);


struct AdvFullMapObjectsView {
    char pad_00[4];
    CObjectType* objectTypes;
    char pad_08[0xc];
    CObject* objects;
    char pad_18[0xc];
    CSprite** sprites;
};

#endif /* HOMM3_ADVMGR_OBJECTS_H */
