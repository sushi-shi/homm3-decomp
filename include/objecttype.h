// objecttype.h - Complete object-template records shared with map loading
// and the random-map generator. Private registries and filters stay in
// objecttype.cpp, where their retail tables and function bodies are owned.
#ifndef HOMM3_OBJECTTYPE_H
#define HOMM3_OBJECTTYPE_H

#include <bitset>
#include <string>
#include <vector>
#include "mapcell.h"

class TObjectTypeFilter;

enum EObjectTypeFilterConstants {
    OBJECT_TYPE_FILTER_COUNT = 15
};

// Before normalization: gObjectTypeFilters.
extern TObjectTypeFilter* const g_objectTypeFilters[OBJECT_TYPE_FILTER_COUNT];

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
    // is defined in the Complete .msk/objects compiland in the
    // newgame..overview gap, where two function-local statics cache a table
    // of image records and this member returns the +0x0c string of the row
    // `imageNumber` selects (or a static empty string when the index is out
    // of range). The ROLE is proven by its conversion-constructor caller - the result is what
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
// loads before either store. objecttype.cpp owns the definition.
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

#endif  /* HOMM3_OBJECTTYPE_H */
