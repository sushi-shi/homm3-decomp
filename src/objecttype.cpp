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
// Residual (77.4%): the register-save placement, and only that. Retail
// returns from the slotCategory gate BEFORE any callee-saved push
// (`xor al,al / pop ebp / ret 4`) and pushes esi then edi afterwards; our
// CL pushes edi at entry and therefore merges all three false exits into
// one tail, which also costs the `test dword ptr [edi+4*esi], eax` folding
// (we load through ecx because esi is popped early). Tried and rejected,
// one compile each: `&&`-ing the gate into the other two conditions
// (77.39, byte-flat), a `const bitset<10>&` local for the two mask uses
// (77.39, byte-flat), `return false` for the gate's own exit (77.39,
// byte-flat), and splitting the test and count into separate guards
// (62.59 - that one really does change the flow, for the worse). The two
// sibling predicates below are EXACT with the same idioms, so the wall is
// not the spelling of any expression here.
VA(0x005141B0, 0x6E)  // anchor-vtable 0x6402c4 slot 1; anchor-global the nine 0..8 initializers at 0x514280..0x514450; retail-only
int TNativeTerrainObjectFilter::Accepts(const TObjectType* objectType) const
{
    if (objectType->slotCategory != 0)
        return 0;
    if (objectType->recommendedTerrainMask.test(m_terrain)
        && objectType->recommendedTerrainMask.count() <= 3)
        return 1;
    return 0;
}

// Retail 0x514220. The same gate and the same `count()`, with the opposite
// arm and no terrain test - which is what makes the pair a partition of the
// unplaced objects into terrain-specific and terrain-agnostic.
VA(0x00514220, 0x3D)  // anchor-vtable 0x6402d4 slot 1; retail-only
int TAnyTerrainObjectFilter::Accepts(const TObjectType* objectType) const
{
    if (objectType->slotCategory == 0
        && objectType->recommendedTerrainMask.count() > 3)
        return 1;
    return 0;
}

// Retail 0x514260, one compare and a `sete`.
VA(0x00514260, 0x19)  // anchor-vtable 0x6402dc slot 1; anchor-global the five 1..5 initializers at 0x5144f0..0x5145e0; retail-only
int TSlotCategoryObjectFilter::Accepts(const TObjectType* objectType) const
{
    return objectType->slotCategory == m_slotCategory;
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
DATA(0x0069cb30) TNativeTerrainObjectFilter gDirtObjectFilter(0);
DATA(0x0069cb38) TNativeTerrainObjectFilter gSandObjectFilter(1);
DATA(0x0069cb10) TNativeTerrainObjectFilter gGrassObjectFilter(2);
DATA(0x0069caf8) TNativeTerrainObjectFilter gSnowObjectFilter(3);
DATA(0x0069cb28) TNativeTerrainObjectFilter gSwampObjectFilter(4);
DATA(0x0069cac8) TNativeTerrainObjectFilter gRoughObjectFilter(5);
DATA(0x0069cad8) TNativeTerrainObjectFilter gSubterraneanObjectFilter(6);
DATA(0x0069cad0) TNativeTerrainObjectFilter gLavaObjectFilter(7);
DATA(0x0069cb20) TNativeTerrainObjectFilter gWaterObjectFilter(8);
DATA(0x0069cae0) TAnyTerrainObjectFilter gAnyTerrainObjectFilter;
DATA(0x0069cb18) TSlotCategoryObjectFilter gSlotCategory1ObjectFilter(1);
DATA(0x0069caf0) TSlotCategoryObjectFilter gSlotCategory2ObjectFilter(2);
DATA(0x0069cae8) TSlotCategoryObjectFilter gSlotCategory3ObjectFilter(3);
DATA(0x0069cb08) TSlotCategoryObjectFilter gSlotCategory4ObjectFilter(4);
DATA(0x0069cb00) TSlotCategoryObjectFilter gSlotCategory5ObjectFilter(5);

// Retail 0x640288, fifteen relocations in the initializer order above.
DATA(0x00640288)
TObjectTypeFilter* const gObjectTypeFilters[OBJECT_TYPE_FILTER_COUNT] = {
    &gDirtObjectFilter,          &gSandObjectFilter,
    &gGrassObjectFilter,         &gSnowObjectFilter,
    &gSwampObjectFilter,         &gRoughObjectFilter,
    &gSubterraneanObjectFilter,  &gLavaObjectFilter,
    &gWaterObjectFilter,         &gAnyTerrainObjectFilter,
    &gSlotCategory1ObjectFilter, &gSlotCategory2ObjectFilter,
    &gSlotCategory3ObjectFilter, &gSlotCategory4ObjectFilter,
    &gSlotCategory5ObjectFilter
};


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
// The tail RE-READS the cache's _First between every member of the copy,
// because `this` may alias the vector's storage - that is the plain
// assignment, not a hoisting failure.
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
// that two-argument insert emit naturally here. Its address remains
// unclaimed; the three-argument overload is already claimed at 0x46aeb0.
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
// Remaining: the imageCache constructor and both bitset::set calls still
// expand, unlike retail; 29 vs 22 blocks.
// Historical append-spelling rankings expired: before GetIndex, cache
// push_back and insert(end(), x) were byte-identical at 47.7708. Earlier
// dead-statement probes (removed) were flat then worse; synthetic caller
// mass did not recover the missing boundaries.
VA(0x00514610, 0x317)  // anchor-callee 0x514b80 per-row `>>`; anchor-global 0x6aba80 .msk cache; retail-only
TObjectType& TObjectType::setImageName(
    const std::basic_string<char, std::char_traits<char>,
                            std::allocator<char> >& name)
{
    TObjectImageNameTable& imageNames = GetObjectImageNames();

    unsigned int oldCount = imageNames.rows.size();
    imageNumber = imageNames.GetIndex(name);

    static std::vector<TImageInfo> imageCache;

    if (imageNumber == oldCount) {
        TPoint emptySize = { 0, 0 };
        TImageInfo newRecord(emptySize);
        imageCache.push_back(newRecord);
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
            ResourceManager::PointToSpriteResource(maskName.c_str());
        if (maskFile == 0) {
            maskFile = ResourceManager::PointToSpriteResource("default.msk");
        }
        if (maskFile != 0) {
            char width;
            char height;
            unsigned char drawBits[6];
            unsigned char shadowBits[6];

            ResourceManager::ReadFromBitmapResource(maskFile, &width, 1);
            ResourceManager::ReadFromBitmapResource(maskFile, &height, 1);
            ResourceManager::ReadFromBitmapResource(maskFile, drawBits, 6);
            ResourceManager::ReadFromBitmapResource(maskFile, shadowBits, 6);
            record->objectSize.x = width;
            record->objectSize.y = height;
            for (unsigned int cell = 0; cell < 48; ++cell) {
                unsigned char bit =
                    static_cast<unsigned char>(1 << (cell & 7));
                record->drawMask.set(
                    cell, (drawBits[cell >> 3] & bit) != 0);
                record->shadowMask.set(
                    cell, (shadowBits[cell >> 3] & bit) != 0);
            }
        }
    }

    imageInfo = imageCache[imageNumber];
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
TObjectType::GetImageName()
{
    static std::string emptyImageName;
    TObjectImageNameTable& imageNames = GetObjectImageNames();

    if (imageNumber < imageNames.rows.size())
        return imageNames.rows[imageNumber]->first;
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
    triggerMask = mask & ~passableMask;
    hasTrigger = triggerMask.any();
    if (hasTrigger) {
        for (int y = 0;; ++y) {
            for (unsigned x = 0; x < 8; ++x) {
                if (triggerMask.test(CObjectType::_getBitPos(x, y))) {
                    triggerCell.x = x;
                    triggerCell.y = y;
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
        int noTriggerX = gNoTriggerCell.x;
        int noTriggerY = gNoTriggerCell.y;
        triggerCell.y = noTriggerY;
        triggerCell.x = noTriggerX;
    }
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
// Residual (76.64%): frame exact at 0x64, and the whole delta is one
// /Ob2 split. Retail CALLS bitset<48>::_Tidy twice and bitset<9>::_Tidy
// once at the head and EXPANDS the string's _Tidy(true) at the tail (its
// second `ret` and its operator delete); we do the exact opposite. The
// lever for the over-inlined half is caller-shrink and this body has no
// mass to lift.
VA(0x00514b80, 0x1F7)  // anchor-caller 0x514d80 per-row loop; anchor-callee setImageName/setTriggerMask; retail-only
std::istream& operator>>(std::istream& is, TObjectType& objectType)
{
    std::string imageName;
    std::bitset<48> passable;
    std::bitset<48> trigger;
    std::bitset<9> terrainRead;
    std::bitset<9> recommendedRead;
    union {
        int raw;
        TAdventureObjectType typed;
    } typeRead;
    int subtype;
    int slotCategory;
    int underlay;

    is >> imageName >> passable >> trigger >> terrainRead >> recommendedRead
        >> typeRead.raw >> subtype >> slotCategory >> underlay;

    std::bitset<10> recommendedTerrain(recommendedRead.to_ulong());
    std::bitset<10> terrain(terrainRead.to_ulong());

    TObjectType& named = objectType.setImageName(imageName);
    named.passableMask = passable | ~named.imageInfo.drawMask;

    TObjectType& row = named.setTriggerMask(trigger);
    row.recommendedTerrainMask &= terrain;
    row.terrainMask = terrain;
    row.recommendedTerrainMask = recommendedTerrain;
    row.objectType = typeRead.typed;
    row.subtype = subtype;
    row.slotCategory = slotCategory;
    row.isUnderlay = underlay != 0;
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
// Residual (69.25%): inside the resize value's TObjectType constructor,
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
VA(0x00514d80, 0x273)  // anchor-callee ResourceManager::GetText + anchor-bracket NewfullMapFn_00505DA0; retail-only
void TObjectTypeTable::load(char* filename)
{
    TTextResource* text = ResourceManager::GetText(filename);
    if (text == 0)
        throw TRuntimeError();

    try {
        int count = atoi(text->GetText(0));
        objectTypes.resize(count);
        for (int i = 0; i < count; ++i) {
            std::istrstream row(text->GetText(i + 1));
            row >> objectTypes[i];
        }
    } catch (...) {
        text->Dispose();
        throw;
    }
    text->Dispose();
}

// Retail 0x517780 is the nine-block Dinkumware tree-successor walk, reached
// from two other bodies in this span (0x516bdb and 0x5170b7). The 0x24-byte
// node the registry's constructor buys is what settles the specialization:
// 36 bytes is a tree header plus pair<const string, int>, so this is the
// image-name MAP's iterator, not a set<string>'s (whose node is 32).
// Dreamcast's generic STLport _M_increment at dc 0x64214 independently
// corroborates the source helper boundary and its nine-block control flow.
VA_COMPGEN(0x00517780, 0xA3, TREE_CONST_ITERATOR_INC, string)

// Minimum ODR use needed to retain the real VC6/Dinkumware COMDAT. This
// wrapper is not a retail claim and adds no target/report row.
void __fastcall EmitObjectImageNameIndexIncrement(
    TObjectImageNameTable::TNameIndex::const_iterator* it)
{
    ++*it;
}

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

// IDENTIFIED 2026-09-06, NOT YET CLAIMABLE: retail 0x515010 (408 B) is
// `basic_string<char>::resize(size_type)`, this object's own COMDAT copy.
// The bytes are Dinkumware's one-argument overload with both arms expanded -
// `_N <= _Len ? erase(_N) : append(_N - _Len, _E(0))`. The erase arm is the
// no-op `_Xran` guard on `_Len < _P0` (retail's `cmp [ebx+8],esi / jae` over
// `_Xran`), `_Split()` (the `_Ptr[-1]` refcount test, `_Tidy(1)`, the inline
// strlen, `_Grow`, `memmove`) and `_Eos`; the append arm is `_Xlen()` behind
// `npos - _Len <= _N`, the second `_Xlen` behind `_Grow`'s own `npos - 3`
// test, and the `rep stosb` of `_Tr::assign(_Ptr + _Len, _N, _C)`. `ret 4`
// and the `[ebx+8]` / `[ebx+4]` member reads fix the arity and the receiver.
// It cannot be claimed yet: a VA_COMPGEN only pairs a COFF symbol VC6 has
// already emitted, and no site in this file's reconstructed source calls
// `resize`. The retail whole-image xref finds the only CALL in
// `collate<char>::do_transform` (0x614260, outside this band), so every use
// inside objecttype.obj was inlined and the emitted COMDAT is the leftover -
// which means the call site is in a body of this TU still unwritten, not in
// one already here. A future lane that adds it should also add
// BASIC_STRING_RESIZE to DIRECT_SYMBOL_COMPGEN_KINDS.

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
