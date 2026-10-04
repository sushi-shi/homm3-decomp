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

// objects.txt begins with the number of object-template rows.
enum EObjectTypeTextIndex {
    OBJECT_TYPE_TEXT_COUNT = 0
};

enum EObjectTypeFilterConstants {
    OBJECT_TYPE_FILTER_COUNT = 15
};

extern TObjectTypeFilter* const g_objectTypeFilters[OBJECT_TYPE_FILTER_COUNT];

// Map-editor/RMG object template consumed by the retail-identical
// CObjectType conversion constructor at 0x506080. The public names are from
// the HD structural bridge; retail independently fixes the 0x4c stride and
// every offset read by that constructor.
struct TObjectType {
public:
    // Numeric slot identities shared with the Complete-only editor filter
    // table (0x640288). The RMG selector at 0x546040 admits categories 4/5
    // on non-water terrain without consulting the recommended mask. Their
    // original semantic labels have not been recovered.
    enum {
        // RMG footprint checker 0x5318b0 treats zero specially when the
        // recommended-terrain mask admits water; original role unresolved.
        SLOT_CATEGORY_0 = OBJECT_SLOT_CATEGORY_0,
        SLOT_CATEGORY_4 = OBJECT_SLOT_CATEGORY_4,
        SLOT_CATEGORY_5 = OBJECT_SLOT_CATEGORY_5
    };
    struct TPoint {
        int m_x;
        int m_y;
    };
    struct TImageInfo {
        // Provisional overload: setImageName initializes the point before
        // either bitset constructor. TObjectType's default construction
        // leaves that point uninitialized, requiring a distinct size path.
        TImageInfo() {}
        explicit TImageInfo(const TPoint& size) : m_objectSize(size) {}

        TPoint m_objectSize;
        std::bitset<48> m_drawMask;
        std::bitset<48> m_shadowMask;
    };
    // The default constructor load()'s `objectTypes.resize(count)` builds
    // its `_Ty()` temporary from, published one store at a time at
    // 0x514e2a-0x514ea1: every member in declaration order, an ALL-SET
    // passable mask spelled as a flipped `bitset<48>(0)`, and the {8,6}
    // no-trigger sentinel. imageInfo's TPoint stays uninitialized there,
    // which is why it has no initializer here either.
    TObjectType();
private:
    // The image and mask setters own these values and their invariants.
    int m_imageNumber;
    std::bitset<48> m_passableMask;
    std::bitset<48> m_triggerMask;
public:
    std::bitset<10> m_terrainMask;
    std::bitset<10> m_recommendedTerrainMask;
private:
    TAdventureObjectType m_objectType;
    int m_subtype;
public:
    int m_slotCategory;
private:
    unsigned char m_isUnderlay;
public:
    unsigned char m_hasTrigger;
    TPoint m_triggerCell;
private:
    TImageInfo m_imageInfo;
public:
    // The image-name registry lookup reads this record's image number;
    // lazy registry/empty-string initialization does not modify the record.
    // Constness is inferred from that ownership, not surviving DC types.
    const std::basic_string<char, std::char_traits<char>,
                            std::allocator<char> >& getImageName() const;
    // CObjectType's conversion loads each dimension as a dword before
    // narrowing it to char. Direct field access folds those into byte
    // loads in VC6; ordinary integer accessors retain the observed boundary.
    // Their role names are provisional: this editor type is Complete-only.
    int getWidth() const { return m_imageInfo.m_objectSize.m_x; }
    // Mac CObjectType conversion 0x128c7c..0x128d24 expands these four
    // coordinate-to-mask queries before assigning the destination cells.
    // The names and member boundaries are inferred; getBitPos owns the
    // shared 8-by-6 coordinate mapping. Mac placement 0x22e090..0x22e17c
    // expands the same trigger/passability queries; both Windows TUs need
    // their definitions visible here.
    bool isDrawCell(unsigned x, unsigned y) const
    {
        return m_imageInfo.m_drawMask.test(CObjectType::getBitPos(x, y));
    }
    bool isPassableCell(unsigned x, unsigned y) const
    {
        return m_passableMask.test(CObjectType::getBitPos(x, y));
    }
    bool isShadowCell(unsigned x, unsigned y) const
    {
        return m_imageInfo.m_shadowMask.test(CObjectType::getBitPos(x, y));
    }
    bool isTriggerCell(unsigned x, unsigned y) const
    {
        return m_triggerMask.test(CObjectType::getBitPos(x, y));
    }
    // Project name for the Complete-only terrain query. The non-const
    // subscript uses VC6's reference proxy, preserving the checked bitset
    // call in mine placement at 0x545a01. A const query expands test and
    // retains only _Xran.
    bool isRecommendedTerrain(int terrain)
    {
        return m_recommendedTerrainMask[terrain];
    }
    bool isRecommendedTerrain(int terrain) const
    {
        return m_recommendedTerrainMask.test(terrain);
    }
    int getHeight() const { return m_imageInfo.m_objectSize.m_y; }
    // Mac conversion 0x128d7c..0x128d94 reads this metadata after the
    // masks. These read-only counterparts of the existing fluent setters
    // have inferred names and boundaries; preserve each stored type.
    TAdventureObjectType getObjectType() const { return m_objectType; }
    int getSubtype() const { return m_subtype; }
    unsigned char isUnderlay() const { return m_isUnderlay; }
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
extern const TObjectType::TPoint g_noTriggerCell;

// Shared header definition for the resize default value. Retail expands
// this constructor, which does not establish an explicit inline keyword:
// an ordinary definition in objecttype.cpp was byte-flat (2026-09-06).
// Retail load calls _Tidy for the trigger and both terrain masks. Default
// construction reproduces that frontier; the flipped passable temporary
// keeps its explicit unsigned-long zero constructor.
inline TObjectType::TObjectType()
    : m_imageNumber(0),
      m_passableMask(~std::bitset<48>(0)),
      m_triggerMask(),
      m_terrainMask(),
      m_recommendedTerrainMask(),
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
    std::vector<TObjectType> m_objectTypes;
    void load(char* filename);
};
SIZE(TObjectTypeTable, 0x10);

#endif  /* HOMM3_OBJECTTYPE_H */
