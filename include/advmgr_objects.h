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

    ~type_cell_adjuster();
    // Before normalization (function): type_cell_adjuster::get_trigger_cell.
    // Before normalization (locals): map_cell.
    NewmapCell* getTriggerCell(NewmapCell* mapCell, int x, int y);
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


// Map-editor/RMG object template consumed by the retail-identical
// CObjectType conversion constructor at 0x506080. The public names are from
// the HD structural bridge; retail independently fixes the 0x4c stride and
// every offset read by that constructor.
struct TObjectType {
    // Numeric slot identities shared with the Complete-only editor filter
    // table (0x640288). The RMG selector at 0x546040 admits categories 4/5
    // on non-water terrain without consulting the recommended mask. Their
    // original semantic labels have not been recovered.
    enum {
        // RMG footprint checker 0x5318b0 treats zero specially when the
        // recommended-terrain mask admits water; original role unresolved.
        SLOT_CATEGORY_0 = 0,
        SLOT_CATEGORY_4 = 4,
        SLOT_CATEGORY_5 = 5
    };

    struct TPoint {
        // Before normalization: x.
        int m_x;
        // Before normalization: y.
        int m_y;
    };
    struct TImageInfo {
        // Provisional overload: setImageName initializes the point before
        // either bitset constructor. TObjectType's default construction
        // leaves that point uninitialized, requiring a distinct size path.
        TImageInfo() {}
        explicit TImageInfo(const TPoint& size) : m_objectSize(size) {}

        // Before normalization: objectSize.
        TPoint m_objectSize;
        // Before normalization: drawMask.
        std::bitset<48> m_drawMask;
        // Before normalization: shadowMask.
        std::bitset<48> m_shadowMask;
    };

    // The default constructor load()'s `objectTypes.resize(count)` builds
    // its `_Ty()` temporary from, published one store at a time at
    // 0x514e2a-0x514ea1: every member in declaration order, an ALL-SET
    // passable mask spelled as a flipped `bitset<48>(0)`, and the {8,6}
    // no-trigger sentinel. imageInfo's TPoint stays uninitialized there,
    // which is why it has no initializer here either.
    TObjectType();

    // Before normalization: imageNumber.
    int m_imageNumber;
    // Before normalization: passableMask.
    std::bitset<48> m_passableMask;
    // Before normalization: triggerMask.
    std::bitset<48> m_triggerMask;
    // Before normalization: terrainMask.
    std::bitset<10> m_terrainMask;
    // Before normalization: recommendedTerrainMask.
    std::bitset<10> m_recommendedTerrainMask;
    // Before normalization: objectType.
    TAdventureObjectType m_objectType;
    // Before normalization: subtype.
    int m_subtype;
    // Before normalization: slotCategory.
    int m_slotCategory;
    // Before normalization: isUnderlay.
    unsigned char m_isUnderlay;
    // Before normalization: hasTrigger.
    unsigned char m_hasTrigger;
    // Before normalization: triggerCell.
    TPoint m_triggerCell;
    // Before normalization: imageInfo.
    TImageInfo m_imageInfo;

    // Retail 0x514960, thiscall with no arguments. DECLARED ONLY: the body
    // belongs to the unadmitted Complete-only .msk/objects compiland in the
    // newgame..overview gap, where two function-local statics cache a table
    // of image records and this member returns the +0x0c string of the row
    // `imageNumber` selects (or a static empty string when the index is out
    // of range). The ROLE is proven by its one caller - the result is what
    // CObjectType's conversion constructor assigns into ImageName - and the
    // NAME follows the role; nothing attests it.
    const std::basic_string<char, std::char_traits<char>,
                            // Before normalization (function): TObjectType::GetImageName.
                            std::allocator<char> >& getImageName();

    // CObjectType's conversion loads each dimension as a dword before
    // narrowing it to char. Direct field access folds those into byte
    // loads in VC6; ordinary integer accessors retain the observed boundary.
    // Their role names are provisional: this editor type is Complete-only.
    // Before normalization (function): TObjectType::GetWidth.
    int getWidth() const { return m_imageInfo.m_objectSize.m_x; }
    // Before normalization (function): TObjectType::GetHeight.
    int getHeight() const { return m_imageInfo.m_objectSize.m_y; }

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
    // Provisional fluent setter names: retail objects.txt extraction retains
    // the two setters above and expands this ordered field/invariant chain.
    // The corresponding ordinary definitions live in objecttype.cpp.
    TObjectType& setPassableMask(const std::bitset<48>& mask);
    TObjectType& setTerrainMask(const std::bitset<10>& mask);
    TObjectType& setRecommendedTerrainMask(const std::bitset<10>& mask);
    TObjectType& setObjectType(TAdventureObjectType type);
    TObjectType& setSubtype(int subtype);
    TObjectType& setSlotCategory(int category);
    TObjectType& setUnderlay(bool underlay);

};
SIZE(TObjectType, 0x4c);

// The "no trigger cell" sentinel, {8, 6} - the object mask grid's own
// dimensions - in .rdata at 0x640278. Both of its consumers, the default
// constructor above and TObjectType::setTriggerMask's else arm, issue both
// loads before either store. No compiland in the tree defines it yet.
// Before normalization: gNoTriggerCell.
extern const TObjectType::TPoint g_noTriggerCell;

// Shared header definition for the resize default value. Retail expands
// this constructor, which does not establish an explicit inline keyword:
// an ordinary definition in objecttype.cpp was byte-flat (2026-09-06).
inline TObjectType::TObjectType()
    : m_imageNumber(0),
      m_passableMask(~std::bitset<48>(0)),
      m_objectType(NOTHING),
      m_subtype(0),
      m_slotCategory(0),
      m_isUnderlay(0),
      m_hasTrigger(0),
      m_triggerCell(g_noTriggerCell)
{
}

class TObjectTypeTable {
public:
    // Before normalization: objectTypes.
    std::vector<TObjectType> m_objectTypes;
    void load(char* filename);
};
SIZE(TObjectTypeTable, 0x10);




#endif /* HOMM3_ADVMGR_OBJECTS_H */
