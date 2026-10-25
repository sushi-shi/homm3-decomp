// objecttype.h - Complete object-template records shared with map loading
// and the random-map generator. Private registries and filters stay in
// objecttype.cpp, where their retail tables and function bodies are owned.
#ifndef HOMM3_OBJECTTYPE_H
#define HOMM3_OBJECTTYPE_H

#include "va.h"

#include <bitset>
#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

#include "Point.h"
#include "adventureobjecttype.h"
#include "objectmask.h"
#include "terrain_type.h"

// objects.txt begins with the number of object-template rows.
enum EObjectTypeTextIndex {
    OBJECT_TYPE_TEXT_COUNT = 0
};

class TRawIStream;
class TRawOStream;

enum EObjectSlotTraitsConstants {
    OBJECT_SLOT_TRAITS_COUNT = 15
};


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
        SLOT_CATEGORY_0 = 0,
        SLOT_CATEGORY_4 = 4,
        SLOT_CATEGORY_5 = 5
    };
    // One image's mask file, cached by image number (Loki's
    // TObjectType::_TImageInfo).
    struct _TImageInfo {
        _TImageInfo() : _m_width(0), _m_height(0) {}

        unsigned int _m_width;
        unsigned int _m_height;
        std::bitset<OBJECT_MASK_CELLS> _m_placedMask;
        std::bitset<OBJECT_MASK_CELLS> _m_shadowMask;
    };
    // The default constructor load()'s `objectTypes.resize(count)` builds
    // its `_Ty()` temporary from, published one store at a time at
    // 0x514e2a-0x514ea1: every member in declaration order, an ALL-SET
    // passable mask spelled as a flipped `bitset<48>(0)`, and the {8,6}
    // no-trigger cell. The image size stays uninitialized there, which is
    // why it has no initializer here either.
    TObjectType();

    // Loki h3maped ObjectType.h befriends its text and map-format streams
    // and its map key order.
    friend std::ostream& operator<<(std::ostream& os, const TObjectType& objectType);
    friend std::istream& operator>>(std::istream& is, TObjectType& objectType);
    friend TRawOStream& operator<<(TRawOStream& stream, const TObjectType& objectType);
    friend TRawIStream& operator>>(TRawIStream& stream, TObjectType& objectType);
    friend struct std::less<TObjectType>;

private:
    static std::vector<_TImageInfo>& _getAImageInfo();

    // The image and mask setters own these values and their invariants.
    // Member spellings are Loki ObjectType.h's, proved by the restored text
    // writer in objecttype.cpp. Loki keeps every member private behind its
    // getters; the random-map generator reads them through those getters.
    // The allowed-terrain mask stays public for now: the footprint scorer
    // (0x536bc0) and one generator terrain scan still index it directly,
    // through the non-const bitset subscript; Loki's const getTerrainMask
    // would select the const one there.
    unsigned int _m_imageNum;
    std::bitset<OBJECT_MASK_CELLS> _m_passableMask;
    std::bitset<OBJECT_MASK_CELLS> _m_triggerMask;
public:
    std::bitset<kNumTerrainTypes> _m_terrainMask;
private:
    std::bitset<kNumTerrainTypes> _m_recommendedTerrainMask;
    TAdventureObjectType _m_type;
    int _m_extra;
    int _m_slotCategory;
    unsigned char _m_bUnderlay;
    unsigned char _m_bHasTrigger;
    TTilePoint _m_triggerLoc;

    unsigned int _m_width;
    unsigned int _m_height;
    std::bitset<OBJECT_MASK_CELLS> _m_placedMask;
    std::bitset<OBJECT_MASK_CELLS> _m_shadowMask;

public:
    // Loki h3maped TObjectType::getImageNum; the object sprite table's
    // index (h3maped 0x48fcfb reads it inline).
    unsigned int getImageNum() const { return _m_imageNum; }
    // The image-name registry lookup reads this record's image number;
    // lazy registry/empty-string initialization does not modify the record.
    // Constness is inferred from that ownership, not surviving DC types.
    const std::basic_string<char, std::char_traits<char>,
                            std::allocator<char> >& getImageName() const;
    // CObjectType's conversion loads each dimension as a dword before
    // narrowing it to char. Direct field access folds those into byte
    // loads in VC6; ordinary integer accessors retain the observed boundary.
    // Their role names are provisional: this editor type is Complete-only.
    static unsigned _getBitPos(unsigned x, unsigned y)
    {
        return OBJECT_MASK_CELLS - 1 - y * OBJECT_MASK_WIDTH - x;
    }
    int getWidth() const { return _m_width; }
    // Mac CObjectType conversion 0x128c7c..0x128d24 expands these four
    // coordinate-to-mask queries before assigning the destination cells.
    // The names and member boundaries are inferred; _getBitPos is the
    // shared 8-by-6 coordinate mapping (CObjectType::getBitPos's formula,
    // Loki's TObjectType spelling), so the map editor's MFC objects can use
    // these records without mapcell.h's CObject. Mac placement 0x22e090..0x22e17c
    // expands the same trigger/passability queries; both Windows TUs need
    // their definitions visible here.
    bool getBCellPlaced(unsigned x, unsigned y) const
    {
        return _m_placedMask.test(_getBitPos(x, y));
    }
    bool getBCellPassable(unsigned x, unsigned y) const
    {
        return _m_passableMask.test(_getBitPos(x, y));
    }
    bool getBCellShadow(unsigned x, unsigned y) const
    {
        return _m_shadowMask.test(_getBitPos(x, y));
    }
    bool getBCellTrigger(unsigned x, unsigned y) const
    {
        return _m_triggerMask.test(_getBitPos(x, y));
    }
    // Project name for the Complete-only terrain query. The non-const
    // subscript uses VC6's reference proxy, preserving the checked bitset
    // call in mine placement at 0x545a01. A const query expands test and
    // retains only _Xran.
    bool isRecommendedTerrain(int terrain)
    {
        return _m_recommendedTerrainMask[terrain];
    }
    bool isRecommendedTerrain(int terrain) const
    {
        return _m_recommendedTerrainMask.test(terrain);
    }
    int getHeight() const { return _m_height; }
    // Mac conversion 0x128d7c..0x128d94 reads this metadata after the
    // masks. These read-only counterparts of the existing fluent setters
    // have inferred names and boundaries; preserve each stored type.
    TAdventureObjectType getType() const { return _m_type; }
    int getExtra() const { return _m_extra; }
    unsigned char getBUnderlay() const { return _m_bUnderlay; }
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
    TObjectType& _setTriggerMask(const std::bitset<OBJECT_MASK_CELLS>& mask);
    // Provisional fluent setter names: retail objects.txt extraction retains
    // the two setters above and expands this ordered field/invariant chain.
    // The corresponding ordinary definitions live in objecttype.cpp.
    TObjectType& _setPassableMask(const std::bitset<OBJECT_MASK_CELLS>& mask);
    TObjectType& setTerrainMask(const std::bitset<kNumTerrainTypes>& mask);
    TObjectType& setRecommendedTerrainMask(const std::bitset<kNumTerrainTypes>& mask);
    TObjectType& setType(TAdventureObjectType type);
    TObjectType& setExtra(int subtype);
    TObjectType& setSlotCategory(int category);
    TObjectType& setBUnderlay(bool underlay);

    // Loki h3maped TObjectType::getRecommendedTerrainMask/getSlotCategory/
    // getTriggerLoc (0x8363ac0, 0x8363ad4, 0x8363b48).
    const std::bitset<kNumTerrainTypes>& getRecommendedTerrainMask() const
    {
        return _m_recommendedTerrainMask;
    }
    int getSlotCategory() const { return _m_slotCategory; }
    // Loki h3maped TObjectType::hasTrigger; the stored byte type matches
    // getBUnderlay's.
    unsigned char hasTrigger() const { return _m_bHasTrigger; }
    const TTilePoint& getTriggerLoc() const { return _m_triggerLoc; }
};
SIZE(TObjectType, 0x4c);

// The object footprint bounds (Loki's "x < kMaxObjWidth", "y < kMaxObjHeight"),
// in .rdata at 0x640278/0x64027c: the default constructor and
// TObjectType::_setTriggerMask's else arm bind both by reference to the
// trigger cell's "no trigger" value. objecttype.cpp owns the definitions.
extern const unsigned int kMaxObjWidth;
extern const unsigned int kMaxObjHeight;

// Shared header definition for the resize default value. Retail expands
// this constructor, which does not establish an explicit inline keyword:
// an ordinary definition in objecttype.cpp was byte-flat (2026-09-06).
// Retail load calls _Tidy for the trigger and both terrain masks. Default
// construction reproduces that frontier; the flipped passable temporary
// keeps its explicit unsigned-long zero constructor.
inline TObjectType::TObjectType()
    : _m_imageNum(0),
      _m_passableMask(~std::bitset<OBJECT_MASK_CELLS>(0)),
      _m_triggerMask(),
      _m_terrainMask(),
      _m_recommendedTerrainMask(),
      _m_type(NOTHING),
      _m_extra(0),
      _m_slotCategory(0),
      _m_bUnderlay(0),
      _m_bHasTrigger(0),
      _m_triggerLoc(kMaxObjWidth, kMaxObjHeight)
{
}

std::istream& operator>>(std::istream& is, TObjectType& objectType);
TRawOStream& operator<<(TRawOStream& stream, const TObjectType& objectType);
TRawIStream& operator>>(TRawIStream& stream, TObjectType& objectType);

// Object types are ordered member by member so they can key the map
// editor's object-type map (Loki h3maped's explicit specialization).
namespace std {
template <>
struct less<TObjectType> : public binary_function<TObjectType, TObjectType, bool> {
    bool operator()(const TObjectType& lhs, const TObjectType& rhs) const;
};
}

class TObjectTypeTable {
public:
    // The editor's persistent palette table retains this default constructor:
    // its vector's allocator byte followed by three null pointers.
    VA_AT(h3maped, 0x004928fb, 0x16)
    TObjectTypeTable() {}
    // The map editor's writer keeps one loaded from objtmplt.txt (h3maped
    // 0x4242b1 constructs it in place, then loads it).
    explicit TObjectTypeTable(char* fileName) { load(fileName); }
    // The map editor's hero class types: one per class and the random
    // hero's, loaded at start-up (h3maped 0x44a035).
    explicit TObjectTypeTable(unsigned int numTypes) : m_objectTypes(numTypes) {}

    std::vector<TObjectType> m_objectTypes;
    void load(char* fileName);
};
SIZE(TObjectTypeTable, 0x10);

// Whether an object type belongs in one of the map editor's palette slots;
// objecttype.cpp defines the traits family and the table (Loki h3maped
// ObjectType.cpp).
class TObjectSlotTraits {
public:
    virtual ~TObjectSlotTraits();
    virtual bool contains(const TObjectType& objectType) const = 0;
};

// Loki h3maped apObjectSlotTraits.
extern TObjectSlotTraits* const apObjectSlotTraits[OBJECT_SLOT_TRAITS_COUNT];

#endif  /* HOMM3_OBJECTTYPE_H */
