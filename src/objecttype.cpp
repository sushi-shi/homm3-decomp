// objecttype.cpp - Complete object-template and image-name registry support.
//
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
// Before normalization: gNoTriggerCell.
DATA(0x00640278) const TObjectType::TPoint g_noTriggerCell = {8, 6};

// Shared registry at 0x69cb80, guard 0x69cb64. GetImageName's empty-name
// static has a separate guard at 0x69cb70, proving a shared accessor boundary.
// That does not prove an inline declaration: C2 classifies this ordinary
// helper's static as kind 8. The former inline definition made it kind 7,
// preventing propagation of rows.end() and assigning its temporary EAX
// where retail uses EDX. See docs/vc6/regalloc.md for the byte-verified trace.
static TObjectImageNameTable& getObjectImageNames()
{
    static TObjectImageNameTable imageNames;
    return imageNames;
}

// The registry map's value_type constructor, retail 0x517c30: the string
// copy expanded in place (allocator byte, _Tidy's three zero stores, then
// assign(_X, 0, npos)) and the mapped index read back through its
// reference. `ret 8` and the int at +0x10 are what separate it from
// type_map_hero_identity's copy constructor, which takes ONE reference and
// carries its int at +0.
// The toolchain's original pair template closes this body at 100%. With the
// nested registry lookup its call also survives in setImageName, making
// the former source specialization and out-of-line shim unnecessary.
VA_COMPGEN(0x00517c30, 0x13F, PAIR_CTOR, string_int_pair)

// The registry's implicit default constructor, emitted as its own COMDAT:
// the _Tree constructor (shared-nil refcount at 0x69cba0, nil node at
// 0x69cba4, _Lockit around the head-node purchase) followed by the four
// zero stores of the vector at +0x10. GetImageName's function-local static
// and setImageName's are the same object, so both initialize through this.
VA_COMPGEN(0x00514060, 0xCA, CLASS_CTOR, TObjectImageNameTable)

// --- the object-type filter family -----------------------------------------
//
// See objecttype.h for the class shapes and why every name here is a role
// description. Retail's own emission order is what fixes the source order:
// the three predicates first, then the fifteen file-scope objects whose
// dynamic initializers run 0x514280..0x5145e0 with the constructor arguments
// 0..8, none, and 1..5 - and the pointer table at 0x640288 lists them in
// exactly that sequence.

// Retail 0x5141b0. The unplaced-object gate first, then the bounds-checked
// bitset<10> test (retail leaves `bitset<10>::_Xran` a call at 0x404410),
// then Dinkumware's nibble-table `count()` over the single word.
//
// Byte-exact (2026-09-07): the virtual predicate returns an unsigned char.
// Retail's early slot-category rejection clears only AL, while the direct
// test/count conjunction materializes a full-width logical result. Keeping
// those source forms distinct prevents C2 from merging the early rejection
// with the later false return. The ESI/EDI saves then remain below the guard,
// and the bitset test folds into the retail memory operand.
// Controls: int return with explicit 1/0 arms 77.3913%; unsigned-char return
// with those arms 77.0652%; unsigned-char return with the direct conjunction
// 100%. The interface and overrides share the recovered byte-return ABI.
// Earlier int-return gate/ref/polarity probes were flat, and splitting the
// conjunction into independent guards regressed; they did not test the
// combination of return width and direct logical-result lowering.
VA(0x005141B0, 0x6E)  // anchor-vtable 0x6402c4 slot 1; anchor-global the nine 0..8 initializers at 0x514280..0x514450; retail-only
unsigned char TNativeTerrainObjectFilter::accepts(const TObjectType* objectType) const
{
    if (objectType->m_slotCategory != 0)
        return 0;
    return objectType->m_recommendedTerrainMask.test(m_terrain)
        && objectType->m_recommendedTerrainMask.count() <= 3;
}

// Retail 0x514220. The same gate and the same `count()`, with the opposite
// arm and no terrain test - which is what makes the pair a partition of the
// unplaced objects into terrain-specific and terrain-agnostic.
// Returning the logical conjunction is exact, including full-width EAX
// materialization. Explicit byte-return 1/0 arms score 99.375%; a separate
// category guard changes the exit layout (63.0833%).
VA(0x00514220, 0x3D)  // anchor-vtable 0x6402d4 slot 1; retail-only
unsigned char TAnyTerrainObjectFilter::accepts(const TObjectType* objectType) const
{
    return objectType->m_slotCategory == 0
        && objectType->m_recommendedTerrainMask.count() > 3;
}

// Retail 0x514260, one compare and a `sete`.
VA(0x00514260, 0x19)  // anchor-vtable 0x6402dc slot 1; anchor-global the five 1..5 initializers at 0x5144f0..0x5145e0; retail-only
unsigned char TSlotCategoryObjectFilter::accepts(const TObjectType* objectType) const
{
    return objectType->m_slotCategory == m_slotCategory;
}

VA(0x005142A0, 0x15)  // anchor-global called by the nine terrain initializers; retail-only
TNativeTerrainObjectFilter::TNativeTerrainObjectFilter(int terrain)
    : m_terrain(terrain)
{
}

VA_COMPGEN(0x005142C0, 0x21, SCALAR_DELETING_DTOR, TNativeTerrainObjectFilter)

VA(0x005144B0, 0x9)  // anchor-global called by the single initializer at 0x514480; retail-only
TAnyTerrainObjectFilter::TAnyTerrainObjectFilter()
{
}

VA_COMPGEN(0x005144C0, 0x21, SCALAR_DELETING_DTOR, TAnyTerrainObjectFilter)

VA(0x00514510, 0x15)  // anchor-global called by the five category initializers; retail-only
TSlotCategoryObjectFilter::TSlotCategoryObjectFilter(int slotCategory)
    : m_slotCategory(slotCategory)
{
}

// Retail 0x514530: the base vptr store and nothing else, which is what both
// derived scalar deleting destructors above call directly.
VA(0x00514530, 0x7)  // anchor-vtable 0x6402cc slot 0's callee; retail-only
TObjectTypeFilter::~TObjectTypeFilter()
{
}

// The fifteen filter objects, in the order their dynamic initializers run.
// Terrain ids follow terrain_type.h; rock (9) has no filter.
// Before normalization: gDirtObjectFilter.
// Before normalization: gSandObjectFilter.
DATA(0x0069cb30) TNativeTerrainObjectFilter g_dirtObjectFilter(0);
// Before normalization: gGrassObjectFilter.
DATA(0x0069cb38) TNativeTerrainObjectFilter g_sandObjectFilter(1);
// Before normalization: gSnowObjectFilter.
DATA(0x0069cb10) TNativeTerrainObjectFilter g_grassObjectFilter(2);
// Before normalization: gSwampObjectFilter.
DATA(0x0069caf8) TNativeTerrainObjectFilter g_snowObjectFilter(3);
// Before normalization: gRoughObjectFilter.
DATA(0x0069cb28) TNativeTerrainObjectFilter g_swampObjectFilter(4);
// Before normalization: gSubterraneanObjectFilter.
DATA(0x0069cac8) TNativeTerrainObjectFilter g_roughObjectFilter(5);
// Before normalization: gLavaObjectFilter.
DATA(0x0069cad8) TNativeTerrainObjectFilter g_subterraneanObjectFilter(6);
// Before normalization: gWaterObjectFilter.
DATA(0x0069cad0) TNativeTerrainObjectFilter g_lavaObjectFilter(7);
// Before normalization: gAnyTerrainObjectFilter.
DATA(0x0069cb20) TNativeTerrainObjectFilter g_waterObjectFilter(8);
// Before normalization: gSlotCategory1ObjectFilter.
DATA(0x0069cae0) TAnyTerrainObjectFilter g_anyTerrainObjectFilter;
// Before normalization: gSlotCategory2ObjectFilter.
DATA(0x0069cb18) TSlotCategoryObjectFilter g_slotCategory1ObjectFilter(1);
// Before normalization: gSlotCategory3ObjectFilter.
DATA(0x0069caf0) TSlotCategoryObjectFilter g_slotCategory2ObjectFilter(2);
// Before normalization: gSlotCategory4ObjectFilter.
DATA(0x0069cae8) TSlotCategoryObjectFilter g_slotCategory3ObjectFilter(3);
// Before normalization: gSlotCategory5ObjectFilter.
DATA(0x0069cb08) TSlotCategoryObjectFilter g_slotCategory4ObjectFilter(4);
DATA(0x0069cb00) TSlotCategoryObjectFilter g_slotCategory5ObjectFilter(5);

// Retail 0x640288, fifteen relocations in the initializer order above.
DATA(0x00640288)
TObjectTypeFilter* const g_objectTypeFilters[OBJECT_TYPE_FILTER_COUNT] = {
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
inline std::vector<TObjectType::TImageInfo>& getObjectImageCache()
{
    static std::vector<TObjectType::TImageInfo> imageCache;
    return imageCache;
}

// The accessor registers this function-local vector teardown with atexit.
// Retail frees imageCache's allocation and clears its three pointer fields;
// the named cache relocation distinguishes the 42-byte static destructor.
VA_COMPGEN(0x00514930, 0x2A, LOCAL_STATIC_DTOR, imageCache)

// Retail 0x514610, TObjectType::setImageName - the .msk cache loader and
// the registry's growth path. Two function-local statics with SEPARATE
// guard bytes: the image-name registry at 0x69cb80 (guard 0x69cb64, which
// GetImageName tests too) and this compiland's own .msk cache at 0x6aba80
// (guard 0x6aba7d, atexit 0x514930) - a vector of 24-byte records whose
// layout IS TObjectType::TImageInfo, as the tail's member-by-member copy
// into imageInfo confirms.
//
// The row count is sampled BEFORE the lookup, and the `imageNumber ==
// oldCount` test at 0x51472e is how retail asks "did the insert append a
// new row?" - only then is the mask file read. The mask name is the image
// name with everything from its last '.' replaced by ".msk" (and appended
// when there is no '.'), and `rfind('.')` is what puts the character in
// the dead parameter home at [ebp+0xb].
//
// The tail reloads the cache's _First for each coordinate and each bitset.
// These are separate source assignments; a whole TImageInfo assignment
// emits rep movsd and loses the repeated loads.
// GetIndex is a provisional Complete-only source boundary. Retail expands
// the first rows.size(), then calls size, pair construction (0x517c30)
// and row insert inside the lookup's insertion arm. Encapsulating that
// arm with its lookup reproduces those decisions and raises 47.7708 to
// 54.7708; flattening it back into this caller is the negative control.
// The registry constructor also remains called. There is no Dreamcast
// counterpart proving this helper's name or declaration.
//
// Retail's cache append at fn+0x150 calls 0x516c10 (522 B), the TWO-argument
// vector<TImageInfo>::insert(iterator, const TImageInfo&) with the
// three-argument overload expanded inside: ret 8, /24 reciprocal
// 0x2aaaaaab, and add [ebx+8],0x18. The nested registry lookup also makes
// that two-argument insert emit naturally here. Its separate claim appears
// below; the three-argument overload is claimed at 0x46aeb0.
// The two zeroed dwords and two bitset<48>::_Tidy calls establish the
// 24-byte TImageInfo temporary; the row cursor uses oldCount * 24.
//
// Naming GetIndex's pair<iterator,bool> result recovers retail's otherwise
// dead bool copy; together with the size-initializing TImageInfo constructor
// this raises MAX to 58.5099. The size initializer must precede both bitset
// constructors. Negative controls: TImageInfo() leaves its point undefined
// under VC6 (54.79 is therefore invalid); an aggregate initializer is
// rejected as C2552; two body assignments occur after the bitset calls.
//
// The cache accessor, bitset proxy assignments and separate final field
// copies raise MAX from 58.5099 to 90.6364. operator[](cell) and its proxy's
// operator=(bool) expand; the two nested set calls remain at 0x51488f and
// 0x5148a6, including retail's two bool argument stack homes. With the
// final field copies and cache accessor, direct set calls score 73.2688.
// A separate mask-reader probe reaches 85.2530 but lacks those bool homes;
// the ordinary bitset API accounts for them without a new source helper.
//
// The appended TImageInfo is a full-expression temporary. Keeping a named
// record alive across the loader arm costs an extra 20 stack bytes; a
// block-scoped record produces the same code as the temporary. The
// resulting frame is retail's 0x60, and both packed masks occupy their
// retail slots. Removing the provisional constructors still makes an
// aggregate initializer fail with C2552; implicit default construction
// leaves the two point coordinates undefined. Passing the point by value,
// naming the record through a reference, separate zero assignments, and
// spelling bit arithmetic as / and % are byte-neutral controls.
//
// Initializing emptySize at function entry and cell before the resource
// reads recovers the retail point stores and this/name/count registers.
// Naming the shared packed-byte index also restores the loop's shift order:
// MAX 96.0790, 791 bytes, 22 matching branch connections and retail's 0x60
// frame. Moving cell's initialization back to the for header scores 91.4941;
// moving only its declaration is byte-neutral. A block-scoped emptySize
// gives 94.2925 before the byte index is named. Zeroing the point with
// memset is identical to the earlier aggregate initialization.
//
// Negative controls: alternate bitset initializers, point constness, signed
// byte arrays/masks, a named insertion index/result, and const map iterators
// are neutral. Coordinate constructors reach 92.8498 but fail to recover the
// surrounding allocation. Returning an index reference or an iterator is
// worse; an early return for existing registry entries adds a branch. Moving
// GetIndex's definition out of the class, before or after this caller, is
// neutral. The bitset size accessor changes the lookup's nested decisions.
// TPoint() under VC6 leaves coordinates undefined; it is not zeroing syntax.
//
// The retained cache insert at 0x516c10 agrees in every non-relocation
// byte of its 522-byte body, including its ten call sites. Its _Construct
// at 0x517b50 uses a six-dword rep movsd, consistent with the ordinary copy.
// An explicit point copy constructor contradicts that callee: _Construct
// grows from 20 to 39 bytes and replaces rep movsd with six individual
// load/store pairs. Coordinate constructors taking values or references
// are neutral when the empty point is initialized before the row count.
VA_COMPGEN(0x00517b50, 0x14, STD_CONSTRUCT, TImageInfo)
//
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
TObjectType& TObjectType::setImageName(
    const std::basic_string<char, std::char_traits<char>,
                            std::allocator<char> >& name)
{
    TPoint emptySize = { 0, 0 };
    TObjectImageNameTable& imageNames = getObjectImageNames();

    unsigned int oldCount = imageNames.m_rows.size();
    m_imageNumber = imageNames.getIndex(name);

    std::vector<TImageInfo>& imageCache = getObjectImageCache();

    if (m_imageNumber == oldCount) {
        imageCache.push_back(TImageInfo(emptySize));
        TImageInfo* record = &imageCache[oldCount];

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

// Retail 0x514960. Two function-local statics with independent guard bytes -
// the empty string at 0x69cb48 (guard 0x69cb70) and the registry at
// 0x69cb80 (guard 0x69cb64, atexit 0x514050) - then an UNSIGNED bound test
// (`cmp ecx,eax` / `jae`) of the int member against the vector's size, which
// retail computes inline from _First/_Last. The in-range arm returns
// `rows[imageNumber]` biased by 0x0c, i.e. the pair's key; the out-of-range
// arm returns the address of the empty string itself.
VA(0x00514960, 0xAD)  // anchor-global 0x69cb80 registry + 0x69cb48 empty name; sole caller CObjectType(TObjectType*), retail-only
const std::basic_string<char, std::char_traits<char>, std::allocator<char> >&
TObjectType::getImageName()
{
    static std::string emptyImageName;
    TObjectImageNameTable& imageNames = getObjectImageNames();

    if (m_imageNumber < imageNames.m_rows.size())
        return imageNames.m_rows[m_imageNumber]->first;
    return emptyImageName;
}

// Retail 0x514a60, chained off setImageName's result by the per-row `>>`
// at 0x514b80. The mask arrives by reference and is COPIED before the
// `&=`, which is Dinkumware's `operator&` written out at the call site:
// retail builds `~passableMask` first (copy, two-word flip, 0xffff trim),
// then copies the argument over it and ANDs high word down. `any()` walks
// the same two words downward into the byte at +0x29, and the false arm's
// {8, 6} sentinel comes straight out of memory. The scan's outer loop has
// NO bound in retail - any() has already guaranteed a hit - and its
// `0x2f - y*8 - x` is CObjectType::_getBitPos verbatim, strength-reduced
// onto a second induction variable in the dead parameter home.
//
// The sentinel arm issues BOTH loads before either store, which two plain
// assignments do not produce (96.26%); naming the two halves first does
// (99.98%, residual: two unclaimed .rdata reloc names).
VA(0x00514a60, 0x11D)  // anchor-callee 0x514b80 per-row `>>`; anchor-global {8,6} at 0x640278; retail-only
TObjectType& TObjectType::setTriggerMask(const std::bitset<48>& mask)
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
        // Retail's tail loads gNoTriggerCell.x then .y and stores
        // triggerCell.x then .y; VC6 emits this pair in the REVERSE of
        // source order, so the assignments are written y-then-x. Tried
        // and rejected: x-then-y assignments 99.9818 (both stores and
        // both loads transposed), `triggerCell = gNoTriggerCell` 96.26,
        // the two member-to-member assignments without the temps 96.26.
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
TObjectType& TObjectType::setPassableMask(const std::bitset<48>& mask)
{
    m_passableMask = mask | ~m_imageInfo.m_drawMask;
    return *this;
}

TObjectType& TObjectType::setTerrainMask(const std::bitset<10>& mask)
{
    m_recommendedTerrainMask &= mask;
    m_terrainMask = mask;
    return *this;
}

TObjectType& TObjectType::setRecommendedTerrainMask(const std::bitset<10>& mask)
{
    m_recommendedTerrainMask = mask;
    return *this;
}
TObjectType& TObjectType::setObjectType(TAdventureObjectType type)
{
    m_objectType = type;
    return *this;
}
TObjectType& TObjectType::setSubtype(int subtype)
{
    m_subtype = subtype;
    return *this;
}
TObjectType& TObjectType::setSlotCategory(int category)
{
    m_slotCategory = category;
    return *this;
}
TObjectType& TObjectType::setUnderlay(bool underlay)
{
    m_isUnderlay = underlay;
    return *this;
}

// Retail 0x514b80, the per-row parser load() runs over every objects.txt
// line after the first. Free and __fastcall under /Gr (stream in ECX,
// record in EDX), and it answers the stream.
//
// The row's nine fields are read as ONE chained expression: the image
// name, two 48-cell masks, TWO NINE-BIT terrain sets (0x44c6e0 masks with
// 0x1ff where the record's own members mask with 0x3ff), then four ints
// through basic_istream's member operator>>, whose four target addresses
// retail pushes ahead of the string extractor's own two arguments.
//
// The record is then filled through the two chaining setters' RETURN
// values - retail reloads EAX after each call rather than re-reading the
// parameter, which is what fixes the references below - and
// TAdventureObjectType arrives through the same union idiom
// NewfullMap::readObjectType already uses for this exact field.
//
// The `recommendedTerrainMask &= terrain` whose result is discarded two
// statements later is retail's, not a transcription slip: retail issues
// the operator&= call on the member at 0x514d04 and then overwrites the
// member at 0x514d12, and a store cannot be moved across that call.
// Exact: default-construct the four input masks and pass the converted
// ten-bit masks as temporary arguments in the fluent setter expression.
// Right-to-left argument evaluation constructs recommended terrain before
// legal terrain; their expression lifetime gives retail's stack-slot reuse.
// The full chain with named conversion locals scores 99.89189%; expression
// temporaries (explicit or implicit conversion) reach 100%. The flattened
// default-constructor control scores 76.63784% with identical declarations.
// The old three unsigned-long zero constructors compensated for flattened
// helpers: with the recovered chain they score 87.57838% (all four: 72.4162%).
// Only the passability/terrain setters with flattened scalar fields score
// 72.91892% on that old zero-constructor form. Retain the whole ordered API.
VA(0x00514b80, 0x1F7)  // anchor-caller 0x514d80 per-row loop; anchor-callee setImageName/setTriggerMask; retail-only
std::istream& operator>>(std::istream& is, TObjectType& objectType)
{
    std::string imageName;
    std::bitset<48> passable;
    std::bitset<48> trigger;
    std::bitset<9> terrainRead;
    std::bitset<9> recommendedRead;
    union {
        // Before normalization: raw.
        int m_raw;
        // Before normalization: typed.
        TAdventureObjectType m_typed;
    } typeRead;
    int subtype;
    int slotCategory;
    int underlay;

    is >> imageName >> passable >> trigger >> terrainRead >> recommendedRead
        >> typeRead.m_raw >> subtype >> slotCategory >> underlay;

    objectType.setImageName(imageName).setPassableMask(passable)
        .setTriggerMask(trigger).setTerrainMask(std::bitset<10>(terrainRead.to_ulong()))
        .setRecommendedTerrainMask(std::bitset<10>(recommendedRead.to_ulong()))
        .setObjectType(typeRead.m_typed).setSubtype(subtype)
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
//
// Retail catch handler 0x514ff3 belongs to this function: the admitted
// extent is 644 bytes (0x284), including its 17-byte tail.
// Residual (73.33% after boundary correction): inside the resize value's TObjectType constructor,
// retail calls bitset<48>(unsigned long) at 0x5154a0 and expands operator~
// into its copy plus flip call. The candidate expands the value constructor
// and calls operator~. It also retains TImageInfo's default constructor and
// expands one vector::size query that retail calls. The throw/catch and
// stream-construction EH states already agree.
// 2026-09-06 controls are byte-flat: implicit resize default, explicit
// TObjectType() second argument, and a named defaultObject; moving the
// ordinary TObjectType constructor into this TU is also byte-flat. The
// default value's expression/lifetime and constructor definition placement
// do not explain the remaining nested decisions in this build. The exact
// unsigned-long overload is confirmed from the retail 97-byte callee; a
// zero-argument bitset constructor would erase a real source boundary.
// Main-branch controls also measured .flip(), .set(), and default-ctor
// variants of the bitset initializer. The default constructor can score
// higher but erases the unsigned-long constructor that retail calls; keep
// that boundary. The apparent gain is not evidence for the default overload.
// Current trace: the TObjectType child budget starts at 121, expands the
// unsigned-long bitset constructor (cost 95), then rejects complement and
// TImageInfo (cost 42 each) at the remaining 26. Explicit m_imageInfo() and
// an ordinary TU-local constructor definition are byte-flat. VC6 rejects
// aggregate initialization of TImageInfo with C2552; its bitset members make
// that source form unavailable. No constructor or helper changes retained.
// The real tree-erasure callers now retain _Inc without the former artificial
// emission wrapper; removing that wrapper is flat across every claimed body.
// Eight count/row getText versus operator[] and implicit/explicit-zero
// istrstream-length controls are also byte-flat and emit no ulong bitset ctor.
// Defining the known {8,6} sentinel in this TU preserves both retail loads
// and is byte-flat across every function; it does not change this boundary.
// A minimal record with no user-declared constructors still fails C2552
// when aggregate-initialized with a point and omitted bitset members.
// An explicit 0UL argument in the passable-mask initializer is also flat
// at 73.3263% across this TU and emits no ulong constructor; argument
// conversion is not what selects the nested inline boundary.
VA(0x00514d80, 0x284)  // anchor-callee ResourceManager::GetText + anchor-bracket NewfullMapFn_00505DA0; retail-only
void TObjectTypeTable::load(char* filename)
{
    TTextResource* text = ResourceManager::getText(filename);
    if (text == 0)
        throw TRuntimeError();

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

// Retail 0x517780 is the nine-block Dinkumware tree-successor walk, reached
// from two other bodies in this span (0x516bdb and 0x5170b7). The 0x24-byte
// node the registry's constructor buys is what settles the specialization:
// 36 bytes is a tree header plus pair<const string, int>, so this is the
// image-name MAP's iterator, not a set<string>'s (whose node is 32).
// Dreamcast's generic STLport _M_increment at dc 0x64214 independently
// corroborates the source helper boundary and its nine-block control flow.
// The two retained erase overloads emit this specialization naturally. Removing
// the old uncalled increment wrapper leaves every claimed TU function unchanged.
VA_COMPGEN(0x00517780, 0xA3, TREE_CONST_ITERATOR_INC, string)

// --- Dinkumware COMDAT pairings -------------------------------------------
//
// This compiland is the owner of the whole 0x514030..0x51bd50 span: its own
// five bodies sit at the head, and everything after them is the istream /
// locale / map<string,int> instantiation set that `#include <strstream>`
// plus the image-name registry drag in. The pairings below are the byte
// sweep of objecttype.obj's own COMDATs against the unclaimed retail rows of
// that span - llvm-objdump/capstone mnemonic streams with direct branch
// targets masked, scored by difflib - restricted to candidates whose retail
// carve size equals the compiled COMDAT's content size exactly. Every one is
// its RVA's unique best match; where two of our COMDATs shared a shape the
// discriminator is named on the claim.

// COMDAT pairing: TObjectImageNameTable's implicit destructor, agreement
// 0.915. The class's implicit constructor is already claimed at 0x514060 and
// this is its mirror image - the vector at +0x10 freed, then _Tree::_Erase
// over the head node - reached only through the two function-local statics.
VA_COMPGEN(0x00514130, 0x7E, IMPLICIT_DTOR, TObjectImageNameTable)

// COMDAT pairing: basic_istream<char>'s streambuf constructor, agreement
// 0.931 (the `_Bool` tie-parameter arm - the only istream ctor this object
// emits).
VA_COMPGEN(0x005151b0, 0xAA, CLASS_CTOR, basic_istream)

// COMDAT pairing: basic_istream<char>'s scalar deleting destructor,
// agreement 0.944. The `lea esi,[ecx-8]` plus the virtual-base vtable
// write through `[eax+4]` is the virtually-derived stream shape, which
// separates it from every other ??_G in the span.
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

// CORRECTED 2026-09-06. This row was claimed as locale::facet's scalar
// deleting destructor on the strength of its 1.000 body agreement, but the
// body is generic - every `??_G` of a class with no members to destroy has
// these fifteen instructions - and the ONE discriminating operand says
// otherwise. It writes vtbl_2402cc, and 0x6402cc is a TWO-slot vtable whose
// second entry is `_purecall`: an abstract class with a virtual destructor
// and one pure virtual, which is exactly TObjectTypeFilter above (whose own
// `??1` at 0x514530 writes the same vtable and is EXACT). locale::facet has
// no pure virtual at all - its vtable 0x645700 is ONE slot wide - so the
// real `??_Gfacet` is the copy bottomviewsubwindow.obj keeps at 0x454740,
// and `??1facet` is 0x51a110 below.
VA_COMPGEN(0x00516560, 0x23, SCALAR_DELETING_DTOR, TObjectTypeFilter)

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
VA_COMPGEN(0x005167a0, 0x2E1, VECTOR_INSERT, TObjectType)
VA_COMPGEN(0x00516a90, 0x44, VECTOR_ERASE, TObjectType)

// COMDAT pairing: _Tree<string, pair<const string,int>>::erase(first, last),
// agreement 0.960 - the registry map's range eraser.
VA_COMPGEN(0x00516ae0, 0x12B, TREE_ERASE_RANGE, string)

// COMDAT pairing: basic_string<char>::operator[](size_t) const, agreement
// 0.917. The object emits both subscripts; the non-const one is 152 B and
// already proven elsewhere, this const one is 28 B and matches the carve.
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

// COMDAT pairing: basic_string<char>::basic_string(size_t, char, const
// allocator&), agreement 0.989. The object emits two string constructors -
// the copy constructor is 293 B and proven elsewhere - and only this one has
// the carve's 204-byte extent.
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
//
// The same sweep, over the kinds the label join had no vocabulary for until
// this change. Ordered by RVA; agreements are the masked-mnemonic difflib
// ratio against the compiled COMDAT of the same content size.

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

// COMDAT pairing: num_get<char, istreambuf_iterator<char>>::do_get, all NINE
// overloads as one group. The claims zip by RVA against the object's COFF
// order, and the zip is self-confirming: the compiled sizes run
// 941/977/973/955/955/1024/1030/1030/906 for
// (bool, unsigned short, unsigned int, long, unsigned long, float, double,
// long double, void*) and the retail extents run
// 943/993/989/969/969/1038/1044/1044/915 in RVA order - monotone in
// correspondence AND equal-sized at exactly the same two positions on both
// sides (the long/unsigned-long pair and the double/long-double pair). Six
// of the nine are the rows the census labelled from the shared
// "0123456789abcdef" digit table, which is what the integer arms read.
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

// COMDAT pairing: locale::facet's destructor - `mov [ecx], vtbl_245700 /
// ret`, and 0x645700 is the one-slot vtable whose single entry is the
// `??_Gfacet` bottomviewsubwindow.obj keeps at 0x454740. The two objects
// win one COMDAT of the class each, which is why neither is here twice.
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
//
// `operator>>(basic_istream<char>&, bitset<N>&)` is instantiated twice here,
// for the 48-bit trigger mask and the 9-bit terrain/landscape flags, and the
// two retail bodies are the same 353 bytes. The WIDTH separates them from
// inside: 0x5156ed, the continuation of 0x515560, tests and clamps against
// 0x30 and 0x5159bc, the continuation of 0x5157f0, against 9 - and the
// latter calls the already-claimed ?set@?$bitset@$08@ at 0x44c680. Each
// continuation then throws through exactly one of the two 203-byte
// "invalid bitset<N> char" bodies, which is what names those as _Xinv rather
// than _Xran and assigns each to its width.
//
// BOUNDARY CORRECTION 2026-09-06: retail's bodies are 589 and 583 bytes, not
// the 353 the carve gave them. Each is EH-bearing and the carve split it
// three ways - main span, typed catch funclet, continuation - and then began
// the continuation row six bytes late, past the register restores the
// funclet's `mov eax,<continuation>; ret` returns onto. Our stock
// specializations were already byte-right; only the extents were wrong, and
// both are EXACT at the corrected sizes. See config/retail-functions.tsv.
VA_COMPGEN(0x00515560, 0x24D, ISTREAM_EXTRACT_BITSET, Bitset48)
// The default TObjectType expression combines its two 48-cell masks through
// this naturally emitted free operator. Its two-dword copy and OR loop match
// the retained Complete helper.
VA_COMPGEN(0x00515510, 0x49, BITSET_OR, Bitset48)
VA_COMPGEN(0x005157f0, 0x247, ISTREAM_EXTRACT_BITSET, Bitset9)
VA_COMPGEN(0x00516e40, 0xCB, BITSET_XINV, Bitset48)
VA_COMPGEN(0x00517680, 0xCB, BITSET_XINV, Bitset9)

// --- three exception members named by retail RTTI --------------------------
//
// These three sit far outside this compiland's span; they are here because
// this is one of the few objects that emits BOTH runtime_error's and
// logic_error's copy constructors plus runtime_error::_Doraise, and a COMDAT
// name is image-unique so exactly one claim may hold each.
//
// The identification is not a similarity argument. Each exception class's
// CatchableType records the copyFunction the runtime uses to catch it by
// value, and the three that matter here read:
//
//   0x647f70  .?AVlogic_error@std@@     copyFunction 0x4044e0
//   0x648648  .?AVruntime_error@std@@   copyFunction 0x41bc30
//   0x650440  .?AVinvalid_argument@std@@ copyFunction 0x516f30 (claimed above)
//
// 0x41bc30 had been claimed in advmgr.cpp as logic_error's on a 0.974
// mnemonic agreement - the two bodies differ only in the vtable they store,
// which objdiff resolves by name - and 0x4044e0 was then refused as a
// duplicate. The RTTI settles both at once, and a second, independent
// witness agrees: 0x41bc10 constructs through 0x41bc30 and throws with the
// _ThrowInfo at 0x6487a8, whose catchable list is exactly
// `runtime_error / exception`. An out_of_range::_Doraise - which is what
// 0x41bc10 had been claimed as - would list out_of_range and logic_error too.
VA_COMPGEN(0x004044e0, 0x159, CLASS_CTOR, logic_error)
VA_COMPGEN(0x0041bc10, 0x1D, EXCEPTION_DORAISE, runtime_error)
VA_COMPGEN(0x0041bc30, 0x159, CLASS_CTOR, runtime_error)

// The rest of logic_error's own COMDAT group, plus out_of_range's _Doraise,
// all four selected out of this object and all four sitting in the same
// 0x404400..0x404700 run as the copy constructor above.
//
// The two _Doraise bodies are byte-identical apart from the CALLEE, which is
// the same discriminator the runtime_error claim above rests on: 0x404640
// copy-constructs through 0x4044e0 (logic_error's, claimed here) and 0x4046e0
// through 0x404700 (out_of_range's, claimed in customcampaign.cpp), and each
// then rethrows with its own _ThrowInfo. No similarity argument is involved
// and none would work - bad_cast's, invalid_argument's and runtime_error's
// bodies all agree 0.700 with both.
//
// 0x404690 is `??1logic_error`: it stores the class's own vtable 0x6455bc
// (whose only other writers are the two logic_error constructors), releases
// the reference-counted `what` string at +0x10, zeroes the three string
// words and tail-calls ~exception. Our COMDAT is the same 75 bytes,
// instruction for instruction. 0x404660 is the scalar deleting destructor
// that wraps it.
//
// The throwing call site that pulls this whole group into the image's first
// COMDAT band is bitset<10>::_Xran at 0x404410 (claimed in border.cpp) -
// the only other __CxxThrowException site below 0x405000.
VA_COMPGEN(0x00404640, 0x1D, EXCEPTION_DORAISE, logic_error)
VA_COMPGEN(0x00404660, 0x21, SCALAR_DELETING_DTOR, logic_error)
VA_COMPGEN(0x00404690, 0x4B, IMPLICIT_DTOR, logic_error)
VA_COMPGEN(0x004046e0, 0x1D, EXCEPTION_DORAISE, out_of_range)

// COMDAT pairing: vector<TObjectType::TImageInfo>::insert(ptr, count,
// const&), agreement 1.000 at an exactly equal 740-byte extent. TImageInfo
// is this header's nested type and no other object instantiates the vector,
// which is why the sizes agree to the byte.
// GetIndex leaves only the two-argument overload emitted. The explicit
// count claim stays unpaired then: that other body's ret 8 cannot name
// this retail ret-12 body or inherit its exact-match identity.
VA_COMPGEN(0x0046aeb0, 0x2E4, VECTOR_INSERT_COUNT, TImageInfo)

// SetImageName's insertion at 0x514760 retains the single-value overload.
// Its six-dword elements, returned insertion position and ret 8 distinguish
// it from the separate count overload above. The current source naturally
// emits this specialization through the canonical image-cache push_back.
// Exact after refreshing the retail target. Independent comparisons prove
// the six differently named nested calls are ICF: _Ucopy/_Ufill/size agree
// with TObstacleVector at 0x46b1a0/0x46b1e0/0x517750, and _Destroy agrees
// with type_artifact at 0x404140. All four comparisons agree in every view.
VA_COMPGEN(0x00516c10, 0x20A, VECTOR_INSERT_SINGLE, TImageInfo)

// COMDAT pairing: basic_istream<char>'s destructor, agreement 0.750 on a
// 15-byte body - the virtual-base vtable fixup, and 1:1 in this object.
VA_COMPGEN(0x00515260, 0xF, IMPLICIT_DTOR, basic_istream)

// COMDAT pairing: the two extraction operators the registry loader runs
// through. Neither is a similarity argument - the FACET each one builds
// names it outright. 0x515270 constructs num_get<char> at 0x517d70 and
// registers it through _Tidyfac<num_get>::_Save at 0x51aed0, so it is
// basic_istream<char>::operator>>(int&); 0x517830 constructs ctype<char> at
// 0x515f50, registers it through _Tidyfac<ctype>::_Save at 0x51ae50 and then
// drives basic_string::_Grow / _Split over sgetc/sbumpc, which is the free
// whitespace-delimited operator>>(istream&, string&). BOUNDARY CORRECTION
// 2026-09-06: retail's extents are 519 and 702, not 347 and 568 - the same
// three-way EH split as the bitset pair above, and for the string operator a
// fourth piece too, the wholly unowned 0x517ad3 `_Xlen` throw block its main
// body branches into off the parent frame. Both are EXACT at the corrected
// sizes; what read as "we expand what retail keeps out of line" was the
// carve, not the codegen.
VA_COMPGEN(0x00515270, 0x207, ISTREAM_EXTRACT_INT, char)
VA_COMPGEN(0x00517830, 0x2BE, ISTREAM_EXTRACT_STRING, char)

// TObjectTypeTable::load calls this specialization twice, and the RMG object
// table supplies the third retail call. The signed magic division by the
// proven 0x4c TObjectType stride distinguishes it from every pointer vector.
VA_COMPGEN(0x0054C910, 0x21, VECTOR_SIZE, TObjectType)
