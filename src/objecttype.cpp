// objecttype.cpp - Complete object-template and image-name registry support.
//
// This compiland is absent from the Dreamcast roster. Retail groups
// TObjectType::setImageName, TObjectTypeTable::load and the global image-name
// registry here; the registry's tree nodes hold a VC6 std::string at +0x0c.
#include <stdlib.h>

#include <va.h>
#define _MT
#include <yvals.h>
#undef _MT
#include <map>
#include <string>
#include <strstream>
#include <vector>

#include "advmgr_objects.h"
#include "exceptions.h"
#include "objecttype.h"
#include "resourcemanager.h"
#include "textresource.h"

// The registry's implicit default constructor, emitted as its own COMDAT:
// the _Tree constructor (shared-nil refcount at 0x69cba0, nil node at
// 0x69cba4, _Lockit around the head-node purchase) followed by the four
// zero stores of the vector at +0x10. GetImageName's function-local static
// and setImageName's are the same object, so both initialize through this.
VA_COMPGEN(0x00514060, 0xCA, CLASS_CTOR, TObjectImageNameTable)

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
// The registry row append is `rows.insert(rows.end(), found)` and the .msk
// cache append is `imageCache.push_back(newRecord)` - the two spellings are
// NOT interchangeable and they interact: rows-insert + cache-push_back is
// 42.6324, both push_back 33.8620 (the state this replaces), both insert
// 34.1581, and rows-push_back + cache-insert 29.9091. VC6 prices an /Ob2
// site at the callee's own front-end size, so `insert(end(), x)` charges
// vector::insert at the site while `push_back(x)` charges a free wrapper
// and prices the nested insert at budget/sites-remaining; which of the two
// reproduces retail depends on how much budget the site has left.
//
// Residual (42.63%): three /Ob2 over-inlines and nothing else. Retail
// CALLS the registry's own constructor, the pair constructor at 0x517c30
// and basic_string::append at 0x41b340; we expand all three, and append's
// expanded _Xlen throw path is exactly the 16 frame bytes (0x70 vs 0x60)
// and the two extra EH states the transcript reports (base [0,-1,1,-1,
// 2,-1,3] against retail's [0,-1,1,-1]). Caller mass is NOT the lever
// here: an `if (0)` titration reads 33.47 / 34.47 / 34.47 / 34.47 /
// 30.52 / 30.52 for N = 0,1,2,3,5,8 - flat then down, a 1.0-point ceiling.
// The one remaining structural difference is the guard bytes: retail
// tests 0x69cb64 and 0x6aba7d with mask 1 each, because the registry's
// static lives in the inline accessor GetImageName shares (see its own
// note); with both statics in this body VC6 packs them into one byte.
VA(0x00514610, 0x317)  // anchor-callee 0x514b80 per-row `>>`; anchor-global 0x6aba80 .msk cache; retail-only
TObjectType& TObjectType::setImageName(
    const std::basic_string<char, std::char_traits<char>,
                            std::allocator<char> >& name)
{
    TObjectImageNameTable& imageNames = GetObjectImageNames();

    unsigned int oldCount = imageNames.rows.size();
    TObjectImageNameTable::TNameIndex::iterator found =
        imageNames.nameIndex.find(name);
    if (found == imageNames.nameIndex.end()) {
        found = imageNames.nameIndex.insert(
            TObjectImageNameTable::TNameIndex::value_type(
                name, imageNames.rows.size())).first;
        imageNames.rows.insert(imageNames.rows.end(), found);
    }
    imageNumber = found->second;

    static std::vector<TImageInfo> imageCache;

    if (imageNumber == oldCount) {
        TImageInfo newRecord;
        newRecord.objectSize.x = 0;
        newRecord.objectSize.y = 0;
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
        int noTriggerX = gNoTriggerCell.x;
        int noTriggerY = gNoTriggerCell.y;
        triggerCell.x = noTriggerX;
        triggerCell.y = noTriggerY;
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
// Residual (69.25%): an /Ob2 SWAP inside the resize temporary's inline
// TObjectType constructor. Retail CALLS `bitset<48>::bitset(unsigned
// long)` at 0x5154a0 and EXPANDS `operator~` (the copy plus a flip call);
// we do the exact opposite, so the argument constructor's 32-bit set loop
// arrives as eleven extra blocks. Retail also expands TImageInfo's
// implicit default constructor where we call it - giving an explicit
// empty inline one is byte-flat, measured. The doctrinal lever for the
// over-inline half is caller-shrink, and this body has nothing to lift:
// its statements are all accounted for by the EH state transcript.
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
