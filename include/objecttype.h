// objecttype.h - the Complete-only image-name registry shared by
// TObjectType::GetImageName and TObjectType::setImageName.
//
// Kept out of advmgr_objects.h deliberately: objecttype.cpp is the only
// consumer, and advmgr_objects.h reaches nine compilands through game.h and
// mapcell.h.
#ifndef HOMM3_OBJECTTYPE_H
#define HOMM3_OBJECTTYPE_H

#include <istream>
#include <map>
#include <string>
#include <vector>

#include "advmgr_objects.h"

// Retail publishes this class's whole layout at the .bss object 0x69cb80
// that both accessors address:
//   +0x00  a 16-byte Dinkumware _Tree - allocator byte, comparator byte,
//          _Head at +4, _Multi at +8, _Size at +0x0c - whose constructor
//          allocates 0x24-byte nodes, i.e. a tree header plus
//          pair<const string, int>;
//   +0x10  a 16-byte vector whose elements are FOUR bytes wide.
// The 4-byte element is the map's own ITERATOR: GetImageName reads
// `rows[i]` and adds 0x0c to reach the key string, and setImageName reads
// +0x1c from the same pointer to reach the mapped index - node+0x0c and
// node+0x1c are exactly `->first` and `->second` of that pair. The
// registry's growth path in setImageName confirms it from the other side:
// it inserts into the tree and then push_backs the RETURNED ITERATOR.
//
// NAMES ARE PROVISIONAL - nothing attests this class; only the offsets, the
// node size and the two accessors' arithmetic are retail-proven.
class TObjectImageNameTable {
public:
    typedef std::map<std::string, int> TNameIndex;

    // Before normalization: nameIndex.
    TNameIndex m_nameIndex;
    // Before normalization: rows.
    std::vector<TNameIndex::iterator> m_rows;

    // Provisional name and boundary inferred from retail setImageName:
    // its first rows.size() expands, but this insertion path calls size,
    // the pair constructor and row insert. Flattening this lookup into
    // the caller expands the pair constructor and scores 47.77 vs 54.77.
    // Before normalization (function): TObjectImageNameTable::GetIndex.
    int getIndex(const std::string& name)
    {
        TNameIndex::iterator found = m_nameIndex.find(name);
        if (found == m_nameIndex.end()) {
            // Retail copies both returned fields, including the unused
            // bool into a stack home. Extracting .first directly drops it.
            std::pair<TNameIndex::iterator, bool> inserted = m_nameIndex.insert(
                TNameIndex::value_type(name, m_rows.size()));
            found = inserted.first;
            m_rows.insert(m_rows.end(), found);
        }
        return found->second;
    }
};

// The registry is a function-local static of an INLINE ACCESSOR, not of
// either consumer, and retail's guard bytes prove it: GetImageName (0x514960)
// and setImageName (0x514610) each test 0x69cb64 with mask 1 for the same
// object, while GetImageName's own empty-name static gets a SECOND byte
// (0x69cb70), also with mask 1. Two statics declared in one body share a
// single guard byte with masks 1 and 2, which is what a shared accessor rules
// out. NAME PROVISIONAL - nothing attests it; only the guard-byte layout and
// the shared 0x69cb80 object are retail-proven.
// Before normalization (function): GetObjectImageNames.
inline TObjectImageNameTable& getObjectImageNames()
{
    static TObjectImageNameTable imageNames;
    return imageNames;
}


// --- the object-type filter family -----------------------------------------
//
// Fifteen file-scope filter objects and the fifteen-entry table of pointers
// to them at 0x640288, all retail-proven and all Complete-only (no Dreamcast
// row covers any of it), so every NAME below is a role description. What the
// bytes fix: three concrete classes over one abstract base with a virtual
// destructor and one pure virtual predicate; the base vtable 0x6402cc holds
// {scalar deleting dtor, _purecall}, and the three concrete vtables
// (0x6402c4, 0x6402d4, 0x6402dc) hold {scalar deleting dtor, the predicate}.
// The fifteen dynamic initializers at 0x514280..0x5145e0 give the source
// order and every constructor argument: nine of the first class with the
// terrain ids 0..8 (rock, 9, is absent), one of the second, and five of the
// third with 1..5 - and the pointer table lists them in exactly that order.
class TObjectTypeFilter {
public:
    virtual ~TObjectTypeFilter();
    // Before normalization (function): TObjectTypeFilter::Accepts.
    virtual int accepts(const TObjectType* objectType) const = 0;
};

// Retail 0x5141b0. The terrain id lands at +4 and the predicate reads
// TObjectType's slotCategory (+0x24) and recommendedTerrainMask (+0x18):
// an unplaced object (category 0) whose recommended terrain set contains
// this terrain and is SMALL - the `count() <= 3` arm, against the
// any-terrain filter's `count() > 3` next door.
class TNativeTerrainObjectFilter : public TObjectTypeFilter {
public:
    explicit TNativeTerrainObjectFilter(int terrain);
    // Before normalization (function): TNativeTerrainObjectFilter::Accepts.
    virtual int accepts(const TObjectType* objectType) const;

    int m_terrain;
};

// Retail 0x514220. No state - its constructor 0x5144b0 writes nothing but
// the vptr - and the mirror of the filter above: an unplaced object whose
// recommended terrain set is WIDE.
class TAnyTerrainObjectFilter : public TObjectTypeFilter {
public:
    TAnyTerrainObjectFilter();
    // Before normalization (function): TAnyTerrainObjectFilter::Accepts.
    virtual int accepts(const TObjectType* objectType) const;
};

// Retail 0x514260, the whole body a `sete` on one compare: the object's
// slotCategory against the one this filter carries at +4.
class TSlotCategoryObjectFilter : public TObjectTypeFilter {
public:
    explicit TSlotCategoryObjectFilter(int slotCategory);
    // Before normalization (function): TSlotCategoryObjectFilter::Accepts.
    virtual int accepts(const TObjectType* objectType) const;

    int m_slotCategory;
};

enum EObjectTypeFilterConstants {
    OBJECT_TYPE_FILTER_COUNT = 15
};

// Before normalization: gObjectTypeFilters.
extern TObjectTypeFilter* const g_objectTypeFilters[OBJECT_TYPE_FILTER_COUNT];

// The per-row parser TObjectTypeTable::load runs over each objects.txt
// line, retail 0x514b80. Free and therefore __fastcall under /Gr: the
// stream arrives in ECX and the record in EDX, and it answers the stream
// so the caller can chain.
std::istream& operator>>(std::istream& is, TObjectType& objectType);

#endif  /* HOMM3_OBJECTTYPE_H */
