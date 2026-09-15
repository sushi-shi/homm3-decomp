// objecttype.cpp - Complete object-template and image-name registry support.

// This compiland is absent from the Dreamcast roster. Retail groups
// TObjectType::setImageName, TObjectTypeTable::load and the global image-name
// registry here; the registry's tree nodes hold a VC6 std::string at +0x0c.
#include <stdlib.h>

#include <va.h>
#include <yvals.h>
#include <map>
#include <string>
#include <strstream>
#include <vector>

#include "advmgr_objects.h"
#include "exceptions.h"
#include "objecttype.h"
#include "resourcemanager.h"
#include "textresource.h"

// Provisional role name; retail stores the two grid dimensions here.
// The only references are TObjectTypeTable::load's default object and
// TObjectType::setTriggerMask's no-trigger path, both in this compiland.
DATA(0x00640278) const ObjectType::Point g_noTriggerCell = {8, 6};
// Retail publishes this class's whole layout at the .bss object 0x69cb80
// that both accessors address:
//   +0x00  a 16-byte Dinkumware _Tree - allocator byte, comparator byte,
//          _Head at +4, _Multi at +8, _Size at +0x0c - whose constructor
//          allocates 0x24-byte nodes, i.e. a tree header plus
//          pair<const string, int>;
//   +0x10  a 16-byte vector whose elements are FOUR bytes wide.
// The 4-byte element carries a map node address, modeled here with the
// map's iterator. GetImageName reads `rows[i]` and adds 0x0c to reach the
// key string; setImageName reads +0x1c to reach the mapped index. These
// are exactly the iterator's `->first` and `->second` accesses. A probe
// using the pinned STL's raw node-pointer type instead reproduces both
// setImageName record-order checkpoints byte for byte, so these bytes
// do not distinguish the two source representations. The
// registry's growth path in setImageName confirms it from the other side:
// it inserts into the tree and then push_backs the RETURNED ITERATOR.
//
// NAMES ARE PROVISIONAL - nothing attests this class; only the offsets, the
// node size and the two accessors' arithmetic are retail-proven.
// Before normalization (type): TObjectImageNameTable.
class ObjectImageNameTable {
public:
    typedef std::map<std::string, int> TNameIndex;
    TNameIndex m_nameIndex;
    std::vector<TNameIndex::iterator> m_rows;

    // Provisional name and boundary inferred from retail setImageName:
    // its first rows.size() expands, but this insertion path calls size,
    // the pair constructor and row insert. Flattening this lookup into
    // the caller expands the pair constructor and scores 47.77 vs 54.77.
    // Keep the returned entry distinct from the iterator passed by reference
    // to vector::insert. In setImageName this preserves retail's existing-
    // entry EAX path and reloads only after insertion (fn+0xe8). With the
    // ordinary registry accessor, returning the mapped value by value also
    // restores the caller's scratch allocation; see setImageName's controls.
    int getIndex(const std::string& name)
    {
        TNameIndex::iterator found = m_nameIndex.find(name);
        TNameIndex::iterator result = found;
        if (found == m_nameIndex.end()) {
            // Retail copies both returned fields, including the unused
            // bool into a stack home. Extracting .first directly drops it.
            std::pair<TNameIndex::iterator, bool> inserted = m_nameIndex.insert(
                TNameIndex::value_type(name, m_rows.size()));
            found = inserted.first;
            m_rows.insert(m_rows.end(), found);
            result = found;
        }
        return result->second;
    }
};

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
class ObjectTypeFilter {
public:
    virtual ~ObjectTypeFilter();
    // Retail 0x5141bd returns its literal zero through AL. The native-terrain
    // override is exact with this byte result and a direct logical tail.
    virtual unsigned char accepts(const ObjectType* objectType) const = 0;
};

// Retail 0x5141b0. The terrain id lands at +4 and the predicate reads
// TObjectType's slotCategory (+0x24) and recommendedTerrainMask (+0x18):
// an unplaced object (category 0) whose recommended terrain set contains
// this terrain and is SMALL - the `count() <= 3` arm, against the
// any-terrain filter's `count() > 3` next door.
// Before normalization (type): TNativeTerrainObjectFilter.
class NativeTerrainObjectFilter : public ObjectTypeFilter {
public:
    explicit NativeTerrainObjectFilter(int terrain);
    virtual unsigned char accepts(const ObjectType* objectType) const;

    int m_terrain;
};

// Retail 0x514220. No state - its constructor 0x5144b0 writes nothing but
// the vptr - and the mirror of the filter above: an unplaced object whose
// recommended terrain set is WIDE.
// Before normalization (type): TAnyTerrainObjectFilter.
class AnyTerrainObjectFilter : public ObjectTypeFilter {
public:
    AnyTerrainObjectFilter();
    virtual unsigned char accepts(const ObjectType* objectType) const;
};

// Retail 0x514260, the whole body a `sete` on one compare: the object's
// slotCategory against the one this filter carries at +4.
// Before normalization (type): TSlotCategoryObjectFilter.
class SlotCategoryObjectFilter : public ObjectTypeFilter {
public:
    explicit SlotCategoryObjectFilter(int slotCategory);
    virtual unsigned char accepts(const ObjectType* objectType) const;

    int m_slotCategory;
};

// The per-row parser TObjectTypeTable::load runs over each objects.txt
// line, retail 0x514b80. Free and therefore __fastcall under /Gr: the
// stream arrives in ECX and the record in EDX, and it answers the stream
// so the caller can chain.
std::istream& operator>>(std::istream& is, ObjectType& objectType);

static ObjectImageNameTable& getObjectImageNames()
{
    static ObjectImageNameTable imageNames;
    return imageNames;
}

VA_COMPGEN(0x00517c30, 0x13F, PAIR_CTOR, string_int_pair)

// The registry's implicit default constructor, emitted as its own COMDAT:
// the _Tree constructor (shared-nil refcount at 0x69cba0, nil node at
// 0x69cba4, _Lockit around the head-node purchase) followed by the four
// zero stores of the vector at +0x10. GetImageName's function-local static
// and setImageName's are the same object, so both initialize through this.
VA_COMPGEN(0x00514060, 0xCA, CLASS_CTOR, ObjectImageNameTable)

// --- the object-type filter family -----------------------------------------

VA(0x005141B0, 0x6E)
unsigned char NativeTerrainObjectFilter::accepts(const ObjectType* objectType) const
{
    if (objectType->m_slotCategory != 0)
        return 0;
    return objectType->m_recommendedTerrainMask.test(m_terrain)
        && objectType->m_recommendedTerrainMask.count() <= 3;
}

VA(0x00514220, 0x3D)
unsigned char AnyTerrainObjectFilter::accepts(const ObjectType* objectType) const
{
    return objectType->m_slotCategory == 0
        && objectType->m_recommendedTerrainMask.count() > 3;
}

VA(0x00514260, 0x19)
unsigned char SlotCategoryObjectFilter::accepts(const ObjectType* objectType) const
{
    return objectType->m_slotCategory == m_slotCategory;
}

VA(0x005142A0, 0x15)
NativeTerrainObjectFilter::NativeTerrainObjectFilter(int terrain)
    : m_terrain(terrain)
{
}

VA_COMPGEN(0x005142C0, 0x21, SCALAR_DELETING_DTOR, NativeTerrainObjectFilter)

VA(0x005144B0, 0x9)
AnyTerrainObjectFilter::AnyTerrainObjectFilter()
{
}

VA_COMPGEN(0x005144C0, 0x21, SCALAR_DELETING_DTOR, AnyTerrainObjectFilter)

VA(0x00514510, 0x15)
SlotCategoryObjectFilter::SlotCategoryObjectFilter(int slotCategory)
    : m_slotCategory(slotCategory)
{
}

VA(0x00514530, 0x7)
ObjectTypeFilter::~ObjectTypeFilter()
{
}

// The fifteen filter objects, in the order their dynamic initializers run.
// Terrain ids follow terrain_type.h; rock (9) has no filter.
DATA(0x0069cb30) NativeTerrainObjectFilter g_dirtObjectFilter(0);
DATA(0x0069cb38) NativeTerrainObjectFilter g_sandObjectFilter(1);
DATA(0x0069cb10) NativeTerrainObjectFilter g_grassObjectFilter(2);
DATA(0x0069caf8) NativeTerrainObjectFilter g_snowObjectFilter(3);
DATA(0x0069cb28) NativeTerrainObjectFilter g_swampObjectFilter(4);
DATA(0x0069cac8) NativeTerrainObjectFilter g_roughObjectFilter(5);
DATA(0x0069cad8) NativeTerrainObjectFilter g_subterraneanObjectFilter(6);
DATA(0x0069cad0) NativeTerrainObjectFilter g_lavaObjectFilter(7);
DATA(0x0069cb20) NativeTerrainObjectFilter g_waterObjectFilter(8);
DATA(0x0069cae0) AnyTerrainObjectFilter g_anyTerrainObjectFilter;
DATA(0x0069cb18) SlotCategoryObjectFilter g_slotCategory1ObjectFilter(1);
DATA(0x0069caf0) SlotCategoryObjectFilter g_slotCategory2ObjectFilter(2);
DATA(0x0069cae8) SlotCategoryObjectFilter g_slotCategory3ObjectFilter(3);
DATA(0x0069cb08) SlotCategoryObjectFilter g_slotCategory4ObjectFilter(4);
DATA(0x0069cb00) SlotCategoryObjectFilter g_slotCategory5ObjectFilter(5);

// Retail 0x640288, fifteen relocations in the initializer order above.
DATA(0x00640288)
ObjectTypeFilter* const g_objectTypeFilters[OBJECT_TYPE_FILTER_COUNT] = {
    &g_dirtObjectFilter,          &g_sandObjectFilter,
    &g_grassObjectFilter,         &g_snowObjectFilter,
    &g_swampObjectFilter,         &g_roughObjectFilter,
    &g_subterraneanObjectFilter,  &g_lavaObjectFilter,
    &g_waterObjectFilter,         &g_anyTerrainObjectFilter,
    &g_slotCategory1ObjectFilter, &g_slotCategory2ObjectFilter,
    &g_slotCategory3ObjectFilter, &g_slotCategory4ObjectFilter,
    &g_slotCategory5ObjectFilter
};

// Provisional cache accessor: retail's independent guard at 0x6aba7d
// initializes the vector at 0x6aba80 through its retained constructor.
// At both the 90.64 and 96.08 checkpoints VC6 expands this accessor but
// gives the nested constructor budget 43 against cost 51. At the earlier
// checkpoint a caller-local static expands that constructor and scores
// 87.9486 instead of 90.6364 in setImageName.
// A caller-local class owning or deriving from the vector also preserves
// the constructor call and is byte-identical at 96.5929. The retained
// constructor therefore establishes a boundary, not this accessor uniquely.
// External inline storage makes the cache fields kind 7 in VC6. Together
// with the ordinary registry accessor and value-returning lookup, it
// reproduces the retail cache/string operands. A static inline definition
// keeps kind 8 storage and does not recover those operands.
inline std::vector<ObjectType::ImageInfo>& getObjectImageCache()
{
    static std::vector<ObjectType::ImageInfo> imageCache;
    return imageCache;
}

VA_COMPGEN(0x00514930, 0x2A, LOCAL_STATIC_DTOR, imageCache)

// The retained cache insert at 0x516c10 agrees in every non-relocation
// byte of its 522-byte body, including its ten call sites. Its _Construct
// at 0x517b50 uses a six-dword rep movsd, consistent with the ordinary copy.
// An explicit point copy constructor contradicts that callee: _Construct
// grows from 20 to 39 bytes and replaces rep movsd with six individual
// load/store pairs. Coordinate constructors taking values or references
// are neutral when the empty point is initialized before the row count.
VA_COMPGEN(0x00517b50, 0x14, STD_CONSTRUCT, ImageInfo)

// Further boundary controls do not close the residual: an ordinary free
// GetIndex is neutral; a separate registry-append helper changes nested
// decisions. Separate whole-mask, raw-data and decode helpers reach only
// 88.7945, 88.1739 and 84.2727. A TImageInfo::readMask member remains called.
// Naming the loop's two bitset references, narrowing dimension lifetimes,
// and reusing oldCount for cell also fail. Moving only the byte-index/bit
// declarations outside the loop is neutral. bitset::at retains two bounds
// branches absent from retail; it is not an elided-check explanation.
// Include-order controls are neutral. The configured CPU scheduling agrees
// with /G5; /G6 lowers the score to 84.1067. why-reg's first named-flag probe
// worsens its register distance from 48 to 53. Its retained-value mapping
// already agrees: ESI=this, EBX=oldCount, EDI=name. With a loop-local index,
// file and cell share ESI and this/name swap; early cell separates them.
// The full inline trace is unchanged from 90.6364 to 96.0790: caller cost
// 577, 22 root candidates, and identical budgets at every nested site.
// Resource-fallback expressions, named proxies and iterator initialization
// versus assignment are neutral. A separate appended iterator restores
// retail's EAX return path but removes its initial found store (88.0316).
// Putting imageNumber's assignment inside the lookup scores 95.2767.
// Unsigned mapped indices and signed loop indices with unsigned byte-offset
// arithmetic are neutral; SHR alone does not prove the counter's type.
// A separate result iterator, initialized from found and refreshed after
// the row append, preserves the initial found store AND restores retail's
// EAX lookup exit. It scores 88.4269 because later temporary allocation
// changes. Branch-specific result assignments add control flow instead.
// Entry reference/pointer returns score 95.3676. A generic registry template
// and find_last_of('.') are neutral; the pinned character overload of
// find_last_of calls the same rfind overload, so that call does not prove
// which public spelling retail used.
// Selecting record after maskName's copy raises MAX to 96.5929 with the
// same 791-byte extent and 22 branch connections. This still selects the
// record later than retail's emitted load; it is not an exact schedule.
// Moving selection past extension replacement scores 90.6759, and moving
// cell's initialization to the loop with this order scores 92.0079.
// The normalized names and headers at master 7ea23f51 reproduce both
// record-order checkpoints byte for byte; spelling += as append is also
// byte-neutral at both checkpoints.
// The substring constructor with name.get_allocator() captures npos in
// ESI before _Tidy and scores at most 93.1146; retail loads npos afterward.
// Its default-allocator form also loses the byte copied from name. These
// controls support the copy constructor rather than a substring operation.
// Naming the end iterator, widening the insertion pair's scope, taking
// the containers directly in the lookup helper, and copy-initializing the
// empty point are byte-neutral. A string-pointer parameter is also neutral
// after changing only the comparison symbol's spelling. Calling an append
// helper directly from this caller adds eight CFG blocks; a const-iterator
// result adds three. Neither recovers the lookup's retained call decisions.
// Returning a stable index reference through a separate result iterator
// raises MAX to 96.6403: all 22 instruction-pattern blocks now agree, with
// the same 27 named calls and EH states 0,-1,1,-1. The existing-entry path
// keeps EAX and only the insertion path reloads found. Returning the whole
// map entry by const reference is byte-identical. Moving record before the
// string copy scores 96.1265; a loop-local counter gives 92.0079 and merges
// a block. A named scalar index in the caller gives 87.8696. The size-query
// wrappers remain neutral together; typed resource reads give 87.8696, and
// returning an insertion pair changes the CFG and gives 73.5652.
// PCH creation/reuse and iterator/reference constness are byte-neutral.
// A name-copy helper stays called (95.6127), replacing retail's separate
// _Tidy and assign calls; a complete filename factory also remains called.
// A by-value temporary point changes the lookup's nested expansion decisions.
// Byte-verified C2 tracing confirms the rotating volatile-register path is
// active here (36 requests); see docs/vc6/regalloc.md section 3a. A preferred
// register does not advance its cursor, so simple first-fit cannot explain
// the remaining scratch-register choices.
// Returning GetIndex by value removes its mapped-load allocation request
// (35 instead of 36), leaving the following cursor at EDX rather than EAX.
// Both traces reproduce their respective complete objects byte for byte
// outside timestamps; this explains the 87.8696 control's allocation shift.
// Operand tracing separates the registry end's preassigned EAX temporary
// from the cache end's rotating EDX request. Naming the append position,
// push_back, an iterator assignment argument, and a named index reference
// are byte-identical here. Branch-specific mapped-value pointers score
// 88.9526 and change the CFG; they do not recover retail's lookup exit.
// Global-allocation tracing (docs/vc6/regalloc.md section 3b) assigns cell
// ESI first with either initializer position. Early initialization excludes
// ESI from maskFile's candidates; the loop-local control allows ESI reuse
// and moves this to EDI. This is an interference difference, not merely
// creation order. The registry end has equal costs for EAX/ECX/EDX/ESI and
// takes EAX by tie-break. A shared iterator return changes nested expansion
// and scores 73.5455; explicit cache insert also changes the CFG (87.0316).
// Moving cell's initialization before the third or fourth read keeps 96.6403
// and the file/index registers, but moves XOR ESI to +0x233 or +0x23f:
// equal scores are not byte identity. Retail initializes it at +0x251.
// Separate field-read and file-acquisition helpers reproduce the rejected
// loop-local-counter function byte for byte. Moving the nullable-file guard
// or filename lookup into the mask reader instead scores 80.4466/84.7470.
// The reference-returning lookup as an ordinary free function is byte-neutral
// with either parameter order. rows.begin()+rows.size() retains an extra
// size call and scores 90.4743; its EDX append position is not retail's load.
// A reference-returning lookup with an early existing-entry return scores
// 88.7352: it adds separate +0x1c address calculations and a forward jump.
// C2 storage-class tracing resolves the accessor mismatch: the ordinary
// registry accessor gives kind-8 storage, allowing rows.end() to propagate;
// the external inline cache accessor gives kind 7 and retains its result.
// With GetIndex returning by value and record selected before the string
// copy, MAX rises to 99.2095. All 27 calls and EH states still agree; the
// only instruction difference is XOR ESI,ESI at +0x21b instead of +0x251.
// The old guard-byte evidence proved a shared registry accessor, not inline.
// Negative controls at this checkpoint: an ordinary cache gives 96.2055;
// static inline does not change its storage kind; a reference-returning
// lookup with inline cache gives 94.6443. Selecting record after the string
// copy gives 95.9486. Moving the counter initializer after the reads or the
// width store changes register ownership (91.7312); a loop-local counter
// gives 92.5020. Preincrement from unsigned -1 reproduces that latter body.
// An early do/while is byte-identical to 99.2095. Moving initialization before
// the file guard or lookup changes the CFG and gives 93.6482/93.6680.
// With the loop-local counter, a named or reused whole point gives 88.8103;
// an early reference to imageInfo gives 81.9447; bitset::size gives 88.8300.
// Unsigned long, /8 and %8 arithmetic, reusing oldCount or dot, and explicit
// success/failure joins all reproduce the 92.5020 late-counter body exactly.
// A full-width bit mask instead gives 92.4625. None preserves retail's file
// register: the byte-verified current trace still assigns maskFile ESI first,
// then moves oldCount to EDI and this to EBX (docs/vc6/regalloc.md section 3b).
// Pointer and reference return signatures produce identical function bytes
// with either counter placement. A separate selected resource is also neutral;
// a ternary fallback gives 92.1265 and changes the branch connections. Using
// imageNumber for the record index reloads the member after insert (83.7273),
// absent from retail; capturing that member before insert gives 92.0909.
// The current output-reference lookup controls give 96.5415/96.0395 and
// preserve the wrong registry operands. A flattened lookup with a separate
// entry factory or typed make_pair gives 78.1067/75.8103 and different calls.
// With the merged normalized headers, declaring cell or maskFile at function
// entry but assigning at the original use reproduces the 92.5020 late-counter
// body byte for byte. Hoisting all raw-read locals changes stack allocation
// and scores 89.9842. Declaration order alone does not recover the file/counter
// interference; homm3 vc6 why-reg --model also finds no binding permutation
// in the 99.2095 body (the residual is instruction placement).
// Remaining: counter initialization placement. No inline-depth controls or
// release-elided operations are used.
VA(0x00514610, 0x317)  // anchor-callee 0x514b80 per-row `>>`; anchor-global 0x6aba80 .msk cache; retail-only
ObjectType& ObjectType::setImageName(
    const std::basic_string<char, std::char_traits<char>,
                            std::allocator<char> >& name)
{
    Point emptySize = { 0, 0 };
    ObjectImageNameTable& imageNames = getObjectImageNames();

    unsigned int oldCount = imageNames.m_rows.size();
    m_imageNumber = imageNames.getIndex(name);

    std::vector<ImageInfo>& imageCache = getObjectImageCache();

    if (m_imageNumber == oldCount) {
        imageCache.push_back(ImageInfo(emptySize));
        ImageInfo* record = &imageCache[oldCount];

        std::basic_string<char, std::char_traits<char>,
                          std::allocator<char> > maskName(name);
        std::string::size_type dot = maskName.rfind('.');
        if (dot != std::string::npos) {
            maskName.replace(dot, maskName.size() - dot,
                             DATA_COMPGEN(0x00640280, objectMaskExtension,
                                          ".msk"));
        } else {
            maskName += DATA_COMPGEN(0x00640280, objectMaskExtension,
                                     ".msk");
        }

        LODFile* maskFile =
            ResourceManager::pointToSpriteResource(maskName.c_str());
        if (maskFile == 0) {
            maskFile = ResourceManager::pointToSpriteResource("default.msk");
        }
        if (maskFile != 0) {
            unsigned int cell = 0;
            char width;
            char height;
            unsigned char drawBits[6];
            unsigned char shadowBits[6];

            ResourceManager::readFromBitmapResource(maskFile, &width, 1);
            ResourceManager::readFromBitmapResource(maskFile, &height, 1);
            ResourceManager::readFromBitmapResource(maskFile, drawBits, 6);
            ResourceManager::readFromBitmapResource(maskFile, shadowBits, 6);
            record->m_objectSize.m_x = width;
            record->m_objectSize.m_y = height;
            for (; cell < 48; ++cell) {
                unsigned int byteIndex = cell >> 3;
                unsigned char bit =
                    static_cast<unsigned char>(1 << (cell & 7));
                record->m_drawMask[cell] = (drawBits[byteIndex] & bit) != 0;
                record->m_shadowMask[cell] = (shadowBits[byteIndex] & bit) != 0;
            }
        }
    }

    m_imageInfo.m_objectSize.m_x = imageCache[m_imageNumber].m_objectSize.m_x;
    m_imageInfo.m_objectSize.m_y = imageCache[m_imageNumber].m_objectSize.m_y;
    m_imageInfo.m_drawMask = imageCache[m_imageNumber].m_drawMask;
    m_imageInfo.m_shadowMask = imageCache[m_imageNumber].m_shadowMask;
    return *this;
}

VA(0x00514960, 0xAD)
const std::basic_string<char, std::char_traits<char>, std::allocator<char> >&
ObjectType::getImageName()
{
    static std::string emptyImageName;
    ObjectImageNameTable& imageNames = getObjectImageNames();

    if (m_imageNumber < imageNames.m_rows.size())
        return imageNames.m_rows[m_imageNumber]->first;
    return emptyImageName;
}

VA(0x00514a60, 0x11D)
ObjectType& ObjectType::setTriggerMask(const std::bitset<48>& mask)
{
    m_triggerMask = mask & ~m_passableMask;
    m_hasTrigger = m_triggerMask.any();
    if (m_hasTrigger) {
        for (int y = 0;; ++y) {
            for (unsigned x = 0; x < 8; ++x) {
                if (m_triggerMask.test(CObjectType::getBitPos(x, y))) {
                    m_triggerCell.m_x = x;
                    m_triggerCell.m_y = y;
                    return *this;
                }
            }
        }
    } else {
        int noTriggerX = g_noTriggerCell.m_x;
        int noTriggerY = g_noTriggerCell.m_y;
        m_triggerCell.m_y = noTriggerY;
        m_triggerCell.m_x = noTriggerX;
    }
    return *this;
}

// Fluent object-template setters inferred from the retail row reader.
// Names are provisional. setPassableMask includes cells outside the image;
// setTerrainMask keeps recommended terrain inside the new legal terrain.
// Keep ordinary member boundaries: the reader's expanded chain reproduces
// retail's retained bitset operations and string destruction. Flattening
// these calls with the same declarations/default constructors scores 76.6378%.
ObjectType& ObjectType::setPassableMask(const std::bitset<48>& mask)
{
    m_passableMask = mask | ~m_imageInfo.m_drawMask;
    return *this;
}

ObjectType& ObjectType::setTerrainMask(const std::bitset<10>& mask)
{
    m_recommendedTerrainMask &= mask;
    m_terrainMask = mask;
    return *this;
}

ObjectType& ObjectType::setRecommendedTerrainMask(const std::bitset<10>& mask)
{
    m_recommendedTerrainMask = mask;
    return *this;
}
ObjectType& ObjectType::setObjectType(AdventureObjectType type)
{
    m_objectType = type;
    return *this;
}
ObjectType& ObjectType::setSubtype(int subtype)
{
    m_subtype = subtype;
    return *this;
}
ObjectType& ObjectType::setSlotCategory(int category)
{
    m_slotCategory = category;
    return *this;
}
ObjectType& ObjectType::setUnderlay(bool underlay)
{
    m_isUnderlay = underlay;
    return *this;
}

VA(0x00514b80, 0x1F7)
std::istream& operator>>(std::istream& is, ObjectType& objectType)
{
    std::string imageName;
    std::bitset<48> passable;
    std::bitset<48> trigger;
    std::bitset<9> terrainRead;
    std::bitset<9> recommendedRead;
    int typeRead;
    int subtype;
    int slotCategory;
    int underlay;

    is >> imageName >> passable >> trigger >> terrainRead >> recommendedRead
        >> typeRead >> subtype >> slotCategory >> underlay;

    objectType.setImageName(imageName).setPassableMask(passable)
        .setTriggerMask(trigger).setTerrainMask(std::bitset<10>(terrainRead.to_ulong()))
        .setRecommendedTerrainMask(std::bitset<10>(recommendedRead.to_ulong()))
        .setObjectType(AdventureObjectType(typeRead)).setSubtype(subtype)
        .setSlotCategory(slotCategory).setUnderlay(underlay != 0);
    return is;
}

// Retail 0x514d80, the objects.txt reader NewfullMapFn_00505DA0 drives.
// The whole shape is published by the function's own EH data at 0x650150:
// eight states, ONE try block spanning states 3..7, and a type-less
// (`catch (...)`) handler at 0x514ff3 that Disposes the text resource and
// rethrows - which is why state 3 itself carries no destructor and the
// resource pointer lives in the dead parameter home rather than a holder.
// The unwind funclets name the rest: 0x62e730 destroys the throw path's
// string temporary, 0x62e738 the exception object, and 0x62e743/5d/68 the
// per-row stream's virtual base (guarded by the construction flag at
// [ebp-0x14]), its strstreambuf and the stream itself.

VA(0x00514d80, 0x284)  // anchor-callee ResourceManager::GetText + anchor-bracket NewfullMapFn_00505DA0; retail-only
void ObjectTypeTable::load(char* filename)
{
    TextResource* text = ResourceManager::getText(filename);
    if (text == 0)
        throw RuntimeError();

    try {
        int count = atoi(text->getText(0));
        m_objectTypes.resize(count);
        for (int i = 0; i < count; ++i) {
            std::istrstream row(text->getText(i + 1));
            row >> m_objectTypes[i];
        }
    } catch (...) {
        text->dispose();
        throw;
    }
    text->dispose();
}

VA_COMPGEN(0x00517780, 0xA3, TREE_CONST_ITERATOR_INC, string)

// This is a library-template enrollment. Do not force its emission with
// an invented game function; its retained body must come from real use.

// --- Dinkumware COMDAT pairings -------------------------------------------

// COMDAT pairing: TObjectImageNameTable's implicit destructor, agreement
// 0.915. The class's implicit constructor is already claimed at 0x514060 and
// this is its mirror image - the vector at +0x10 freed, then _Tree::_Erase
// over the head node - reached only through the two function-local statics.
VA_COMPGEN(0x00514130, 0x7E, IMPLICIT_DTOR, ObjectImageNameTable)

// COMDAT pairing: basic_istream<char>'s streambuf constructor, agreement
// 0.931 (the `_Bool` tie-parameter arm - the only istream ctor this object
// emits).
VA_COMPGEN(0x005151b0, 0xAA, CLASS_CTOR, basic_istream)

VA_COMPGEN(0x00515a40, 0x32, SCALAR_DELETING_DTOR, basic_istream)

// COMDAT pairing: ctype<char>'s three-argument constructor, agreement 0.914.
VA_COMPGEN(0x00515f50, 0x106, CLASS_CTOR, ctype)

// COMDAT pairing: ctype<char>'s destructor and its scalar deleting wrapper.
// Three 33-byte ??_G bodies live in this span (0x5142c0, 0x5144c0, 0x516130)
// and they are identical apart from the destructor each calls; only this one
// calls 0x516160, whose 36-byte extent is exactly the compiled
// ??1?$ctype@D@std@@ - so the pair is settled from both ends at once. The
// other two both call the 7-byte 0x514530 and stay unclaimed.
VA_COMPGEN(0x00516130, 0x21, SCALAR_DELETING_DTOR, ctype)
VA_COMPGEN(0x00516160, 0x24, IMPLICIT_DTOR, ctype)

VA_COMPGEN(0x00516560, 0x23, SCALAR_DELETING_DTOR, ObjectTypeFilter)

// COMDAT pairing: strstreambuf(const char*, int), agreement 0.957.
VA_COMPGEN(0x005165f0, 0xE7, CLASS_CTOR, strstreambuf)

// COMDAT pairing: istrstream's scalar deleting destructor, agreement 1.000.
// It calls the CRT's own ??1istrstream@std@@ at 0x60af24 by name, so no
// similarity argument is needed.
VA_COMPGEN(0x00516720, 0x30, SCALAR_DELETING_DTOR, istrstream)

// COMDAT pairing: istrstream's `vbase destructor' closure, the sibling of
// the scalar deleting destructor above and byte-identical to this object's
// own `??_Distrstream` COMDAT - `lea esi,[ecx+0x58]` onto the virtual
// basic_ios subobject, then the CRT's ??1istrstream and the basic_ios<char>
// destructor at 0x453f40. The 20-byte extent matches ostrstream's already
// claimed twin in bottomviewsubwindow (0x451750, also 0x14). Declaration
// only, in MSVC's own backtick spelling - there is no source body.
#if 0  // @carcass: compiler-generated closure, claim only

VA(0x00516750, 0x14)
void istrstream::`vbase destructor'();

#endif  // @carcass

// COMDAT pairing: bitset<48>::flip(), agreement 1.000 - the trigger-mask
// member TObjectType::setTriggerMask flips, and 48 is the only bitset width
// whose flip this object emits.
VA_COMPGEN(0x00516770, 0x28, BITSET_FLIP, bitset48)

// COMDAT pairing: vector<TObjectType>::insert(ptr, count, const&) and
// ::erase(first, last), agreements 1.000 and 1.000, both reached from the
// already-claimed TObjectTypeTable::load at 0x514d80. These two are
// compiland-private by construction - TObjectType is this header's type -
// which is why the sizes agree to the byte.
VA_COMPGEN(0x005167a0, 0x2E1, VECTOR_INSERT, ObjectType)
VA_COMPGEN(0x00516a90, 0x44, VECTOR_ERASE, ObjectType)

// COMDAT pairing: _Tree<string, pair<const string,int>>::erase(first, last),
// agreement 0.960 - the registry map's range eraser.
VA_COMPGEN(0x00516ae0, 0x12B, TREE_ERASE_RANGE, string)

VA_COMPGEN(0x00516e20, 0x1C, BASIC_STRING_SUBSCRIPT, char)

// COMDAT pairing: invalid_argument's _Doraise and its copy constructor. Not
// a similarity argument at all - 0x516f10 throws through the _ThrowInfo at
// 0x650470, whose catchable-type array reads
// `.?AVinvalid_argument@std@@ / .?AVlogic_error@std@@ / .?AVexception@@`,
// and its one call is to 0x516f30, which is therefore that class's copy
// constructor. The three 29-byte _Doraise bodies in this object (runtime_error,
// logic_error, invalid_argument) are otherwise indistinguishable.
VA_COMPGEN(0x00516f10, 0x1D, EXCEPTION_DORAISE, invalid_argument)
VA_COMPGEN(0x00516f30, 0x157, CLASS_CTOR, invalid_argument)

// COMDAT pairing: _Tree<string,...>::erase(iterator) - at 1342 B the largest
// unclaimed body in the span - agreement 0.971, and ::_Erase(node), the
// recursive subtree destroyer, agreement 0.952.
VA_COMPGEN(0x00517090, 0x53E, TREE_ERASE_ITERATOR, string)
VA_COMPGEN(0x005175d0, 0xAD, TREE_ERASE, string)

// COMDAT pairing: num_get<char, istreambuf_iterator<char>>::num_get(size_t),
// agreement 0.889 - the facet constructor, the mirror of num_put's already
// claimed at 0x4546e0.
VA_COMPGEN(0x00517d70, 0x5C, CLASS_CTOR, num_get)

VA_COMPGEN(0x0051a120, 0xCC, CLASS_CTOR, basic_string)

// COMDAT pairing: _Tree<string,...>::insert(const value_type&), agreement
// 0.969, and the pair<iterator,bool> constructor it returns through,
// agreement 1.000 - the latter is 0x51af50's only call into this span.
// setImageName retains map<string,int>::insert as a thin hidden-return
// wrapper around the tree insertion below.
VA_COMPGEN(0x00517B70, 0x2C, MAP_INSERT, string)
VA_COMPGEN(0x0051af50, 0x156, TREE_INSERT, string)
VA_COMPGEN(0x0051b150, 0x18, CLASS_CTOR, pair)

// COMDAT pairing: _Tree<string,...>::_Lbound, agreement 0.941, and
// const_iterator::_Dec, agreement 0.952 - the predecessor walk whose
// successor twin is already claimed at 0x517780.
VA_COMPGEN(0x0051b510, 0xBC, TREE_LBOUND, string)
VA_COMPGEN(0x0051b5d0, 0xB3, TREE_CONST_ITERATOR_DEC, string)

// --- Dinkumware COMDAT pairings, part 2: the input-stream family ----------

// COMDAT pairing: basic_streambuf<char>::sgetc, agreement 1.000.
VA_COMPGEN(0x005157b0, 0x20, STREAMBUF_SGETC, char)

// COMDAT pairing: basic_string<char>::replace(pos, n, const char*, n),
// agreement 0.994 - the `.msk` suffix rewrite in setImageName.
VA_COMPGEN(0x00515a80, 0x199, BASIC_STRING_REPLACE, char)

// COMDAT pairing: basic_string<char>::rfind(const char*, pos, n), agreement
// 0.983 - the `rfind('.')` that finds the extension setImageName replaces.
VA_COMPGEN(0x00515c20, 0x74, BASIC_STRING_RFIND, char)

// COMDAT pairing: basic_istream<char>::ipfx(bool), agreement 0.969 - the
// whitespace-skipping prefix every extraction in this compiland runs
// through, and the reason use_facet<ctype<char>> is instantiated here.
VA_COMPGEN(0x00515ca0, 0x27A, ISTREAM_IPFX, char)

// COMDAT pairing: basic_ios<char>::setstate, agreement 1.000.
VA_COMPGEN(0x00515f20, 0x26, BASIC_IOS_SETSTATE, char)

// COMDAT pairing: ctype<char>'s four case-conversion virtuals. The two
// scalar bodies are byte-identical and so are the two range bodies; the CRT
// helper each calls is the whole discriminator, and it is unambiguous -
// 0x516070/0x516090 call __Tolower at 0x60c6dd, 0x5160d0/0x5160f0 call
// __Toupper at 0x60c8ec. Sizes 28/56 then separate scalar from range.
VA_COMPGEN(0x00516070, 0x1C, CTYPE_DO_TOLOWER_CHAR, char)
VA_COMPGEN(0x00516090, 0x38, CTYPE_DO_TOLOWER_RANGE, char)
VA_COMPGEN(0x005160d0, 0x1C, CTYPE_DO_TOUPPER_CHAR, char)
VA_COMPGEN(0x005160f0, 0x38, CTYPE_DO_TOUPPER_RANGE, char)

// COMDAT pairing: basic_ios<char>::clear and basic_streambuf<char>::sbumpc,
// agreements 1.000 and 1.000.
VA_COMPGEN(0x00517af0, 0x1D, BASIC_IOS_CLEAR, char)
VA_COMPGEN(0x00517b10, 0x32, STREAMBUF_SBUMPC, char)

// COMDAT pairing: _Tree<string,...>::find, agreement 0.984 - the registry
// lookup GetImageName runs.
VA_COMPGEN(0x00517ba0, 0x86, TREE_FIND, string)

VA_COMPGEN(0x00517dd0, 0x3AF, NUM_GET_DO_GET, char)
VA_COMPGEN(0x00518180, 0x3E1, NUM_GET_DO_GET, char)
VA_COMPGEN(0x00518570, 0x3DD, NUM_GET_DO_GET, char)
VA_COMPGEN(0x00518950, 0x3C9, NUM_GET_DO_GET, char)
VA_COMPGEN(0x00518d20, 0x3C9, NUM_GET_DO_GET, char)
VA_COMPGEN(0x005190f0, 0x40E, NUM_GET_DO_GET, char)

// COMDAT pairing: ctype<char>::is(mask, char), agreement 1.000 - reached
// from the float arm of do_get immediately above it.
VA_COMPGEN(0x00519500, 0x22, CTYPE_IS, char)

VA_COMPGEN(0x00519530, 0x414, NUM_GET_DO_GET, char)
VA_COMPGEN(0x00519950, 0x414, NUM_GET_DO_GET, char)
VA_COMPGEN(0x00519d70, 0x393, NUM_GET_DO_GET, char)

// COMDAT pairing: num_get<char>::_Getifld, agreement 0.968 - the integer
// field scanner the five integral do_get arms share.
VA_COMPGEN(0x0051a1f0, 0x534, NUM_GET_GETIFLD, char)

VA_COMPGEN(0x0051a110, 0x7, IMPLICIT_DTOR, facet)

// COMDAT pairing: istreambuf_iterator<char>'s operator*, _Inc and _Peek,
// agreements 1.000, 1.000 and 1.000.
VA_COMPGEN(0x0051a730, 0x4F, ISTREAMBUF_ITERATOR_DEREF, char)
VA_COMPGEN(0x0051a8e0, 0x53, ISTREAMBUF_ITERATOR_INC, char)
VA_COMPGEN(0x0051a940, 0x45, ISTREAMBUF_ITERATOR_PEEK, char)

// COMDAT pairing: the two use_facet<> instantiations. Sizes alone separate
// them (510 vs 507 on both sides), and the bytes agree independently:
// 0x51a990 calls __Getctype at 0x60c844, which only the ctype arm does.
VA_COMPGEN(0x0051a990, 0x1FE, USE_FACET_CTYPE, char)
VA_COMPGEN(0x0051ab90, 0x1FB, USE_FACET_NUMPUNCT, char)

// COMDAT pairing: istreambuf_iterator<char>::equal, agreement 1.000.
VA_COMPGEN(0x0051ad90, 0xB1, ISTREAMBUF_ITERATOR_EQUAL, char)

// COMDAT pairing: _Tidyfac's two remaining instantiations. Four bodies, two
// sizes, and the static each pair shares is the discriminator exactly as it
// was for num_put/numpunct at 0x455c20: 0x51ae50 stores into bss_29cbb0 and
// 0x51b0b0 clears it, 0x51aed0 stores into bss_29cbb4 and 0x51b170 clears
// it. Which pair is ctype's is settled from outside - 0x51b0b0's address is
// taken inside 0x51a990, the use_facet<ctype> arm - and corroborated from
// the other side, since the num_get pair's _Save is reached only from
// 0x515270, basic_istream<char>::operator>>(int&).
VA_COMPGEN(0x0051ae50, 0x7B, TIDYFAC_CTYPE_SAVE, char)
VA_COMPGEN(0x0051aed0, 0x7B, TIDYFAC_NUM_GET_SAVE, char)
VA_COMPGEN(0x0051b0b0, 0x92, TIDYFAC_CTYPE_TIDY, char)
VA_COMPGEN(0x0051b170, 0x92, TIDYFAC_NUM_GET_TIDY, char)

// COMDAT pairing: _Tree<string,...>::_Insert, agreement 0.968 - the
// rebalancing node inserter behind the already-claimed public insert.
VA_COMPGEN(0x0051b210, 0x2FF, TREE_NODE_INSERT, string)

// COMDAT pairing: std::_Maklocstr, agreement 1.000 - the locale-name
// duplicator, reached from use_facet<numpunct>.
VA_COMPGEN(0x0051b690, 0x39, MAKLOCSTR, char)

// COMDAT pairing: _Construct<pair<const string, int>>, agreement 0.962 -
// the registry map's node initializer.
VA_COMPGEN(0x0051b6d0, 0x15B, STD_CONSTRUCT, string_int_pair)

// COMDAT pairing: basic_string<char>'s two compare overloads, agreements
// 0.976 and 1.000; the sizes (84 vs 106) agree with the mangled parameter
// lists on both sides.
VA_COMPGEN(0x0051b8b0, 0x54, BASIC_STRING_COMPARE_STR, char)
VA_COMPGEN(0x0051b910, 0x6A, BASIC_STRING_COMPARE_SUBSTR, char)

// --- Dinkumware COMDAT pairings, part 3: the two bitset extractions -------

VA_COMPGEN(0x00515560, 0x24D, ISTREAM_EXTRACT_BITSET, Bitset48)
// The default TObjectType expression combines its two 48-cell masks through
// this naturally emitted free operator. Its two-dword copy and OR loop match
// the retained Complete helper.
VA_COMPGEN(0x00515510, 0x49, BITSET_OR, Bitset48)
VA_COMPGEN(0x005157f0, 0x247, ISTREAM_EXTRACT_BITSET, Bitset9)
VA_COMPGEN(0x00516e40, 0xCB, BITSET_XINV, Bitset48)
VA_COMPGEN(0x00517680, 0xCB, BITSET_XINV, Bitset9)

// These three sit far outside this compiland's span; they are here because
// this is one of the few objects that emits BOTH runtime_error's and
// logic_error's copy constructors plus runtime_error::_Doraise, and a COMDAT
// name is image-unique so exactly one claim may hold each.

// The identification is not a similarity argument. Each exception class's
// CatchableType records the copyFunction the runtime uses to catch it by
// value, and the three that matter here read:

//   0x647f70  .?AVlogic_error@std@@     copyFunction 0x4044e0
//   0x648648  .?AVruntime_error@std@@   copyFunction 0x41bc30
//   0x650440  .?AVinvalid_argument@std@@ copyFunction 0x516f30 (claimed above)

VA_COMPGEN(0x004044e0, 0x159, CLASS_CTOR, logic_error)
VA_COMPGEN(0x0041bc10, 0x1D, EXCEPTION_DORAISE, runtime_error)
VA_COMPGEN(0x0041bc30, 0x159, CLASS_CTOR, runtime_error)

// The rest of logic_error's own COMDAT group, plus out_of_range's _Doraise,
// all four selected out of this object and all four sitting in the same
// 0x404400..0x404700 run as the copy constructor above.

// The two _Doraise bodies are byte-identical apart from the CALLEE, which is
// the same discriminator the runtime_error claim above rests on: 0x404640
// copy-constructs through 0x4044e0 (logic_error's, claimed here) and 0x4046e0
// through 0x404700 (out_of_range's, claimed in customcampaign.cpp), and each
// then rethrows with its own _ThrowInfo. No similarity argument is involved
// and none would work - bad_cast's, invalid_argument's and runtime_error's
// bodies all agree 0.700 with both.

// 0x404690 is `??1logic_error`: it stores the class's own vtable 0x6455bc
// (whose only other writers are the two logic_error constructors), releases
// the reference-counted `what` string at +0x10, zeroes the three string
// words and tail-calls ~exception. Our COMDAT is the same 75 bytes,
// instruction for instruction. 0x404660 is the scalar deleting destructor
// that wraps it.

// The throwing call site that pulls this whole group into the image's first
// COMDAT band is bitset<10>::_Xran at 0x404410 (claimed in border.cpp) -
// the only other __CxxThrowException site below 0x405000.
VA_COMPGEN(0x00404640, 0x1D, EXCEPTION_DORAISE, logic_error)
VA_COMPGEN(0x00404660, 0x21, SCALAR_DELETING_DTOR, logic_error)
VA_COMPGEN(0x00404690, 0x4B, IMPLICIT_DTOR, logic_error)
VA_COMPGEN(0x004046e0, 0x1D, EXCEPTION_DORAISE, out_of_range)

// The old count-insert claim at 0x46aeb0 named TImageInfo only because
// its trivial 24-byte record produced the same generic vector code. Retail
// callers are combatManager::placeObstacle and castSpell, both operating
// on TObstacle. Their native count-insert bodies match all 740 retail bytes
// outside the two verified new/delete relocations. The retained claim now
// belongs to cmbtmgr's TObstacle specialization; no image-cache count-insert
// call or explicit instantiation is introduced just to emit another copy.

VA_COMPGEN(0x00516c10, 0x20A, VECTOR_INSERT_SINGLE, ImageInfo)

// This insertion and setupAndLoadObstacles (0x466290) both call 0x517750.
// Retail retains the folded size helper here in the objecttype cluster.
// The TObstacle copy expands in cmbtmgr; this native TImageInfo instance
// matches all 33 retail bytes, with no relocations or added instantiation.
VA_COMPGEN(0x00517750, 0x21, VECTOR_SIZE, ImageInfo)

// COMDAT pairing: basic_istream<char>'s destructor, agreement 0.750 on a
// 15-byte body - the virtual-base vtable fixup, and 1:1 in this object.
VA_COMPGEN(0x00515260, 0xF, IMPLICIT_DTOR, basic_istream)

VA_COMPGEN(0x00515270, 0x207, ISTREAM_EXTRACT_INT, char)
VA_COMPGEN(0x00517830, 0x2BE, ISTREAM_EXTRACT_STRING, char)

VA_COMPGEN(0x0054C910, 0x21, VECTOR_SIZE, ObjectType)
