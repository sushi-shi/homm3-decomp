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

// The gate is widened with CObject's constructor below, whose body names
// Random: an in-class member body's non-dependent names are looked up at the
// closing brace of the class, and mapcell.cpp reaches misc.h only later in
// its include list.
int __fastcall Random(int minimum, int maximum);

class CObject {
public:
    // readScholarData reaches the scholar lanes of this dword directly -
    // it switches on a SIGNED three-bit award (`shl 0x1d / sar 0x1d`),
    // which no mask spelling over the plain dword produces. Only that one
    // arm is carried here; the other five typed views stay events-only.
    //
    // readObject (0x502e00) adds two more arms of the same dword, and both
    // are bitfield stores no mask spelling over the plain dword produces:
    // its SHIPYARD arm clears the low byte with `and cl,0` before merging
    // the owner in, and its SHRINE arm writes a signed ten-bit lane thirteen
    // bits up. The shipyard's record is the one game::ClaimShipyard already
    // reads off the CELL - the same encoding, because the object's dword is
    // what ends up in NewmapCell::extraInfo.
    union {
        unsigned long extraInfo;
        ScholarInfo scholar_info;
        ShipyardInfo shipyard_info;
        ShrineInfo shrine_info;
    };
    unsigned char x;
    unsigned char y;
    unsigned char z;
    unsigned char pad_07;
    unsigned short typeIndex;
    unsigned char animationOffset;
    unsigned char pad_0b;

    // MapCell.h:595. game::InsertObject byte-proves this header body: the
    // coordinates narrow to bytes, type starts at zero, extra info remains a
    // dword, and each dynamic object receives a random animation phase.
    //
    // The DEFAULT ARGUMENTS are byte-proven from the other end, by
    // loadMapObjects' `objects.resize(count)`: the `_Ty()` temporary
    // Dinkumware's resize materialises at the call site stores 0xff into
    // each coordinate, 0xffff into typeIndex and 0xffffffff into extraInfo,
    // then rolls the animation phase - this body verbatim, in this order,
    // with those five values. game::InsertObject's explicit five-argument
    // call is unaffected.
    CObject(unsigned char newX = 0xff, unsigned char newY = 0xff,
            unsigned char newZ = 0xff, unsigned short newType = 0xffff,
            unsigned long newExtraInfo = 0xffffffff)
    {
        x = newX;
        y = newY;
        z = newZ;
        typeIndex = newType;
        extraInfo = newExtraInfo;
        animationOffset = static_cast<unsigned char>(Random(0, 255));
    }

    CObjectType* get_object_type_ptr();
    void FindTrigger(int* resultX, int* resultY);
};
SIZE(CObject, 0xc);

#ifdef HOMM3_MAPCELL_OBJECT_TYPE_TABLE_VIEW
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
        TPoint objectSize;
        std::bitset<48> drawMask;
        std::bitset<48> shadowMask;
    };

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
};
SIZE(TObjectType, 0x4c);

class TObjectTypeTable {
public:
    std::vector<TObjectType> objectTypes;
    void load(char* filename);
};
SIZE(TObjectTypeTable, 0x10);
#endif

class CObjectType {
public:
#ifdef HOMM3_MAPCELL_OBJECT_TYPE_TABLE_VIEW
    CObjectType() {}
    CObjectType(TObjectType* source);                         // 0x506080
#endif
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
    // +0x12..+0x13 is alignment before the first bitset.  Keep it implicit:
    // retail's generated assignment skips these bytes.
    std::bitset<48> drawCells;
    // +0x1c, sliced out of the old pad: saveObjectType packs FOUR masks,
    // not three, and this is the second of them. DC name PassableMask; the
    // spelling follows its three siblings here rather than the DC's.
    std::bitset<48> passableCells;
    std::bitset<48> shadowCells;
    // Fourth 48-cell mask, byte-proven at +0x2c by FindTrigger. The prior
    // padding spelling incorrectly conflated it with shadowCells at +0x24.
    std::bitset<48> triggerCells;
    // +0x34, FOUR bytes and unchanged in layout, but not padding: the
    // default constructor CObjectType's `resize` temporary runs calls SIX
    // sub-constructors, and the sixth targets this slot through
    // std::bitset<10>::_Tidy at 0x506880 (`and eax,0x3ff` - _Trim with
    // 10 % 32 = 10), where the four masks above go through the
    // bitset<48> _Tidy at 0x4e66c0. `char pad_34[4]` emits five and cannot
    // produce the sixth. It also resolves the Dreamcast offset arithmetic:
    // DC's Type at 48 maps to retail 52 = 0x34, yet saveObjectType
    // byte-proves objectType at 0x38 - retail inserted one 4-byte member
    // the DC record does not have, and this is it.
    //
    // The mask's MEANING is unproven and its name is deliberately ordinal:
    // no serializer in this compiland reads or writes it, readObjectType
    // included.
    std::bitset<10> mask_34;
    // loadObjectType stores the serialized type as a full dword. Keep one
    // instance declarator here: VC6's generated union copy copies every
    // declarator, whereas retail copies +0x38 exactly once. The unused
    // static spelling preserves this header's established VC6 include-set
    // sensitivity without adding a second field to generated copies.
    union {
        TAdventureObjectType objectType;
    };
    static unsigned long objectTypeValue;
    int extra;
    unsigned char suppressDraw;
    // +0x41 remains implicit alignment, but retail's generated assignment
    // explicitly copies a word at +0x42; the old pad_41[3] hid that real
    // field and also made VC6 copy the otherwise-skipped +0x41 byte.
    unsigned short field_42;
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
