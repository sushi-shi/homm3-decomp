// objecttype.cpp - Complete object-template and image-name registry support.

// This compiland is absent from the Dreamcast roster. Retail groups
// TObjectType::setImageName, TObjectTypeTable::load and the global image-name
// registry here; the registry's tree nodes hold a VC6 std::string at +0x0c.
#include "va.h"

// Retail CRT slots 607/608 (0x516590/0x5165c0) construct <iostream>'s two
// header statics, _Ios_init at 0x69cb59 and _Wios_init at 0x69cb58, and
// register their destructors; this compiland included the header.
// Its facet-id initializer 0x51b830 also guards num_put (0x6aba98) before
// num_get (0x6aba7c). VC6 orders those guards by instantiation: an
// arithmetic insertion into an ostream parsed before the first arithmetic
// extraction reproduces retail's ctype<unsigned short>, num_put, num_get,
// numpunct body. Loki h3maped's ObjectType.cpp supplies that insertion: its
// objects.txt writer, operator<<(ostream&, const TObjectType&) (line 270,
// befriended in ObjectType.h), sits just before the reader. Retail keeps no
// ostream code here, so /OPT:REF dropped the unreferenced writer; with it
// restored the compiled initializer is byte-identical to 0x51b830.
#include <assert.h>
#include <iostream>
#include <map>
#include <stdlib.h>
#include <string>
#include <strstream>
#include <vector>
#include <yvals.h>

#include "objecttype.h"

#include "UniqueSet.h"
#include "advmgr_objects.h"
#if !defined(HOMM3_TARGET_MAC)
#include "editor/RawStream.h"
#endif
#include "exceptions.h"
#include "resourcemanager.h"
#include "textresource.h"

// The only references are TObjectTypeTable::load's default object and
// TObjectType::_setTriggerMask's no-trigger path, both in this compiland.
DATA(0x00640278) extern const unsigned int kMaxObjWidth = 8;
DATA(0x0064027c) extern const unsigned int kMaxObjHeight = 6;

// The map format's reserved bytes after an object type (Loki
// ObjectType.cpp's kNumObjectTypeReserved; the name is inferred there).
const unsigned int kNumObjectTypeReserved = 16;

// --- the object slot traits ----------------------------------------------
//
// Loki h3maped names the family (ObjectType.cpp): TObjectSlotTraits and its
// {anonymous}::TTerrainSlotTraits(TTerrainType), TAllTerrainSlotTraits() and
// TCategorySlotTraits(TSlotCategory), each with a const `contains`, and the
// fifteen-entry table apObjectSlotTraits. Retail fixes the rest: the base
// vtable 0x6402cc holds {scalar deleting dtor, _purecall}, the three
// concrete vtables (0x6402c4, 0x6402d4, 0x6402dc) hold {scalar deleting
// dtor, contains}, and the fifteen dynamic initializers at
// 0x514280..0x5145e0 give the source order and every constructor argument:
// nine terrain traits for terrains 0..8 (Loki asserts terrainType <
// eTerrainRock), the all-terrain traits, and five category traits for 1..5
// (Loki asserts category > eCategoryGeneric && category <
// kNumSlotCategories). The table at 0x640288 lists them in that order.
namespace {

// Retail 0x5141b0: an unplaced object (category 0) whose recommended
// terrain set contains this terrain and is small.
class TTerrainSlotTraits : public TObjectSlotTraits {
public:
    explicit TTerrainSlotTraits(TTerrainType terrainType);
    virtual bool contains(const TObjectType& objectType) const;

    TTerrainType m_terrainType;
};

// Retail 0x514220. No state - its constructor 0x5144b0 writes nothing but
// the vptr: an unplaced object whose recommended terrain set is wide.
class TAllTerrainSlotTraits : public TObjectSlotTraits {
public:
    TAllTerrainSlotTraits();
    virtual bool contains(const TObjectType& objectType) const;
};

// Retail 0x514260, the whole body a `sete` on one compare.
class TCategorySlotTraits : public TObjectSlotTraits {
public:
    explicit TCategorySlotTraits(int category);
    virtual bool contains(const TObjectType& objectType) const;

    int m_category;
};

} // namespace

namespace {
// Loki h3maped {anonymous}::getImageNameSet; the set holds a map at +0 and a
// numbering vector at +0x10.
MAC_ADDRESS(0x223798, 0x78)
TUniqueSet<std::string>& getImageNameSet()
{
    DATA_COMPGEN_GUARD(0x0069cb64, imageNameSetGuard, imageNameSet)
    DATA(0x0069cb80)
    static TUniqueSet<std::string> imageNameSet;
    VA_COMPGEN(0x00514050, 0xA, STATIC_DTOR, imageNameSet)
    return imageNameSet;
}

} // namespace

VA_COMPGEN(0x00517c30, 0x13F, PAIR_CTOR, string_uint_pair)

// The set's implicit default constructor, emitted as its own COMDAT.
VA_COMPGEN(0x00514060, 0xCA, CLASS_CTOR, TUniqueSet)

// --- the object slot traits ----------------------------------------------
namespace {

VA(0x005141B0, 0x6E)
MAC_ADDRESS(0x223910, 0x74)
bool TTerrainSlotTraits::contains(const TObjectType& objectType) const
{
    if (objectType.getSlotCategory() != 0)
        return false;
    const std::bitset<kNumTerrainTypes>& terrainMask =
        objectType.getRecommendedTerrainMask();
    return terrainMask[m_terrainType] && terrainMask.count() <= 3;
}

VA(0x00514220, 0x3D)
MAC_ADDRESS(0x223984, 0x4c)
bool TAllTerrainSlotTraits::contains(const TObjectType& objectType) const
{
    return objectType.getSlotCategory() == 0
        && objectType.getRecommendedTerrainMask().count() > 3;
}

VA(0x00514260, 0x19)
MAC_ADDRESS(0x2239d0, 0x18)
bool TCategorySlotTraits::contains(const TObjectType& objectType) const
{
    return objectType.getSlotCategory() == m_category;
}

VA(0x005142A0, 0x15)
VA_AT(h3maped, 0x00490115, 0x12)
TTerrainSlotTraits::TTerrainSlotTraits(TTerrainType terrainType)
    : m_terrainType(terrainType)
{
    assert(terrainType >= 0 && terrainType < eTerrainRock);
}

VA_COMPGEN(0x005142C0, 0x21, SCALAR_DELETING_DTOR, TTerrainSlotTraits)

VA(0x005144B0, 0x9)
VA_AT(h3maped, 0x00490285, 0x9)
TAllTerrainSlotTraits::TAllTerrainSlotTraits()
{
}

VA_COMPGEN(0x005144C0, 0x21, SCALAR_DELETING_DTOR, TAllTerrainSlotTraits)

VA(0x00514510, 0x15)
VA_AT(h3maped, 0x004902a6, 0x12)
TCategorySlotTraits::TCategorySlotTraits(int category)
    : m_category(category)
{
    assert(category > TObjectType::SLOT_CATEGORY_0
           && category <= TObjectType::SLOT_CATEGORY_5);
}

} // namespace

VA(0x00514530, 0x7)
MAC_ADDRESS(0x224728, 0x48)
TObjectSlotTraits::~TObjectSlotTraits()
{
}

namespace {

// The fifteen traits objects, in the order their dynamic initializers run.
DATA(0x0069cb30) TTerrainSlotTraits g_dirtSlotTraits(eTerrainDirt);
DATA(0x0069cb38) TTerrainSlotTraits g_sandSlotTraits(eTerrainSand);
DATA(0x0069cb10) TTerrainSlotTraits g_grassSlotTraits(eTerrainGrass);
DATA(0x0069caf8) TTerrainSlotTraits g_snowSlotTraits(eTerrainSnow);
DATA(0x0069cb28) TTerrainSlotTraits g_swampSlotTraits(eTerrainSwamp);
DATA(0x0069cac8) TTerrainSlotTraits g_roughSlotTraits(eTerrainRough);
DATA(0x0069cad8) TTerrainSlotTraits g_subterraneanSlotTraits(eTerrainSubterranean);
DATA(0x0069cad0) TTerrainSlotTraits g_lavaSlotTraits(eTerrainLava);
DATA(0x0069cb20) TTerrainSlotTraits g_waterSlotTraits(eTerrainWater);
DATA(0x0069cae0) TAllTerrainSlotTraits g_allTerrainSlotTraits;
DATA(0x0069cb18) TCategorySlotTraits g_category1SlotTraits(1);
DATA(0x0069caf0) TCategorySlotTraits g_category2SlotTraits(2);
DATA(0x0069cae8) TCategorySlotTraits g_category3SlotTraits(3);
DATA(0x0069cb08) TCategorySlotTraits g_category4SlotTraits(4);
DATA(0x0069cb00) TCategorySlotTraits g_category5SlotTraits(5);

} // namespace

// Retail 0x640288, fifteen relocations in the initializer order above.
DATA(0x00640288)
TObjectSlotTraits* const apObjectSlotTraits[OBJECT_SLOT_TRAITS_COUNT] = {
    &g_dirtSlotTraits,         &g_sandSlotTraits,
    &g_grassSlotTraits,        &g_snowSlotTraits,
    &g_swampSlotTraits,        &g_roughSlotTraits,
    &g_subterraneanSlotTraits, &g_lavaSlotTraits,
    &g_waterSlotTraits,        &g_allTerrainSlotTraits,
    &g_category1SlotTraits,    &g_category2SlotTraits,
    &g_category3SlotTraits,    &g_category4SlotTraits,
    &g_category5SlotTraits
};

// Loki's retained _getAImageInfo symbol and class declaration identify this
// static member; its sole caller is setImageName.
VA_AT(h3maped, 0x0049062d, 0x3d)
inline std::vector<TObjectType::_TImageInfo>& TObjectType::_getAImageInfo()
{
    DATA_COMPGEN_GUARD(0x006aba7d, imageCacheGuard, imageCache)
    DATA(0x006aba80)
    static std::vector<TObjectType::_TImageInfo> imageCache;
    return imageCache;
}

VA_COMPGEN(0x00514930, 0x2A, LOCAL_STATIC_DTOR, imageCache)

VA_COMPGEN(0x00517b50, 0x14, STD_CONSTRUCT, _TImageInfo)

// Loki initializes the mask counter after the four resource reads; the
// counter stays local to the loop.
VA(0x00514610, 0x317)
MAC_ADDRESS(0x223aa4, 0x2b8)  // anchor-callee 0x514b80 per-row `>>`; anchor-global 0x6aba80 .msk cache; retail-only
TObjectType& TObjectType::setImageName(
    const std::basic_string<char, std::char_traits<char>,
                            std::allocator<char> >& newImageName)
{
    TUniqueSet<std::string>& imageNameSet = getImageNameSet();

    unsigned int setSize = imageNameSet.numItems();
    _m_imageNum = imageNameSet.add(newImageName);

    std::vector<_TImageInfo>& aImageInfo = _getAImageInfo();

    if (_m_imageNum == setSize) {
        aImageInfo.push_back(_TImageInfo());
        _TImageInfo& imageInfo = aImageInfo[setSize];

        // The suffix is .rdata array storage, while "default.msk" below is a
        // pooled literal.
        DATA(0x00640280) static const char maskExtension[] = ".msk";
        std::basic_string<char, std::char_traits<char>,
                          std::allocator<char> > maskName = newImageName;
        std::string::size_type dotPos = maskName.find_last_of('.');
        if (dotPos != std::string::npos) {
            maskName.replace(dotPos, maskName.size() - dotPos, maskExtension);
        } else {
            maskName += maskExtension;
        }

        LODFile* maskFile =
            ResourceManager::PointToSpriteResource(maskName.c_str());
        if (maskFile == 0) {
            maskFile = ResourceManager::PointToSpriteResource("default.msk");
        }
        if (maskFile != 0) {
            char width;
            char height;
            unsigned char placedBits[6];
            unsigned char shadowBits[6];

            ResourceManager::ReadFromBitmapResource(maskFile, &width, 1);
            ResourceManager::ReadFromBitmapResource(maskFile, &height, 1);
            ResourceManager::ReadFromBitmapResource(maskFile, placedBits, 6);
            ResourceManager::ReadFromBitmapResource(maskFile, shadowBits, 6);
            imageInfo._m_width = width;
            imageInfo._m_height = height;
            for (unsigned int bit = 0; bit < OBJECT_MASK_CELLS; ++bit) {
                unsigned int byteNum = bit >> 3;
                unsigned char bitMask =
                    static_cast<unsigned char>(1 << (bit & 7));
                imageInfo._m_placedMask[bit] = (placedBits[byteNum] & bitMask) != 0;
                imageInfo._m_shadowMask[bit] = (shadowBits[byteNum] & bitMask) != 0;
            }
        }
    }

    _m_width = aImageInfo[_m_imageNum]._m_width;
    _m_height = aImageInfo[_m_imageNum]._m_height;
    _m_placedMask = aImageInfo[_m_imageNum]._m_placedMask;
    _m_shadowMask = aImageInfo[_m_imageNum]._m_shadowMask;
    return *this;
}

VA(0x00514960, 0xAD)
MAC_ADDRESS(0x223dcc, 0x80)
const std::basic_string<char, std::char_traits<char>, std::allocator<char> >&
TObjectType::getImageName() const
{
    // Bit 0 of 0x69cb70 guards the empty string.
    DATA_COMPGEN_GUARD(0x0069cb70, emptyImageNameGuard, emptyImageName)
    DATA(0x0069cb48)
    static std::string emptyImageName;
    VA_COMPGEN(0x00514a10, 0x45, STATIC_DTOR, emptyImageName)
    TUniqueSet<std::string>& imageNameSet = getImageNameSet();

    if (_m_imageNum < imageNameSet.numItems())
        return imageNameSet.get(_m_imageNum);
    return emptyImageName;
}

VA(0x00514a60, 0x11D)
VA_AT(h3maped, 0x004906fb, 0xce)
MAC_ADDRESS(0x223f44, 0x134)
TObjectType& TObjectType::_setTriggerMask(const std::bitset<OBJECT_MASK_CELLS>& mask)
{
    _m_triggerMask = mask & ~_m_passableMask;
    _m_bHasTrigger = _m_triggerMask.any();
    if (_m_bHasTrigger) {
        for (int y = 0;; ++y) {
            assert(y < kMaxObjHeight);
            for (unsigned x = 0; x < 8; ++x) {
                if (_m_triggerMask[_getBitPos(x, y)]) {
                    _m_triggerLoc = TTilePoint(x, y);
                    goto found;
                }
            }
        }
    found:
        ;
    } else {
        _m_triggerLoc = TTilePoint(kMaxObjWidth, kMaxObjHeight);
    }
    return *this;
}

// Fluent object-template setters inferred from the retail row reader. Names
// are provisional. setPassableMask includes cells outside the image;
// setTerrainMask keeps recommended terrain inside the new legal terrain.
MAC_ADDRESS(0x223ea8, 0x9c)
TObjectType& TObjectType::_setPassableMask(const std::bitset<OBJECT_MASK_CELLS>& mask)
{
    _m_passableMask = mask | ~_m_placedMask;
    return *this;
}

MAC_ADDRESS(0x223e4c, 0x48)
TObjectType& TObjectType::setTerrainMask(const std::bitset<kNumTerrainTypes>& mask)
{
    assert(!mask[eTerrainRock]);
    _m_recommendedTerrainMask &= mask;
    _m_terrainMask = mask;
    return *this;
}

MAC_ADDRESS(0x223e94, 0xc)
TObjectType& TObjectType::setRecommendedTerrainMask(const std::bitset<kNumTerrainTypes>& mask)
{
    assert((mask & ~_m_terrainMask).none());
    assert(!mask[eTerrainRock]);
    _m_recommendedTerrainMask = mask;
    return *this;
}
TObjectType& TObjectType::setType(TAdventureObjectType type)
{
    _m_type = type;
    return *this;
}
TObjectType& TObjectType::setExtra(int subtype)
{
    _m_extra = subtype;
    return *this;
}

MAC_ADDRESS(0x223ea0, 0x8)
TObjectType& TObjectType::setSlotCategory(int category)
{
    assert(category >= TObjectType::SLOT_CATEGORY_0
           && category <= TObjectType::SLOT_CATEGORY_5);
    _m_slotCategory = category;
    return *this;
}
TObjectType& TObjectType::setBUnderlay(bool underlay)
{
    _m_bUnderlay = underlay;
    return *this;
}

// The objects.txt form of an object type (Loki h3maped ObjectType.cpp:270,
// befriended in its ObjectType.h, defined just before the reader below).
// Nothing calls it, so /OPT:REF drops the body from both images; its
// arithmetic insertions are what order this compiland's facet ids.
std::ostream& operator<<(std::ostream& os, const TObjectType& objectType)
{
    os << objectType.getImageName()
       << ' ' << objectType._m_passableMask
       << ' ' << objectType._m_triggerMask
       << ' ' << std::bitset<kNumTerrainTypes - 1>(objectType._m_terrainMask.to_ulong())
       << ' ' << std::bitset<kNumTerrainTypes - 1>(objectType._m_recommendedTerrainMask.to_ulong())
       << ' ' << objectType._m_type
       << ' ' << objectType._m_extra
       << ' ' << objectType._m_slotCategory
       << ' ' << int(objectType._m_bUnderlay);
    return os;
}

// The per-row parser TObjectTypeTable::load runs over each objects.txt line;
// it answers the stream so the caller can chain.
VA(0x00514b80, 0x1F7)
MAC_ADDRESS(0x224078, 0x1ac)
std::istream& operator>>(std::istream& is, TObjectType& objectType)
{
    std::string imageName;
    std::bitset<OBJECT_MASK_CELLS> passable;
    std::bitset<OBJECT_MASK_CELLS> trigger;
    std::bitset<9> terrainRead;
    std::bitset<9> recommendedRead;
    int typeRead;
    int subtype;
    int slotCategory;
    int underlay;

    is >> imageName >> passable >> trigger >> terrainRead >> recommendedRead
        >> typeRead >> subtype >> slotCategory >> underlay;

    objectType.setImageName(imageName)._setPassableMask(passable)
        ._setTriggerMask(trigger).setTerrainMask(terrainRead.to_ulong())
        .setRecommendedTerrainMask(recommendedRead.to_ulong())
        .setType(TAdventureObjectType(typeRead)).setExtra(subtype)
        .setSlotCategory(slotCategory).setBUnderlay(underlay != 0);
    return is;
}

// CodeWarrior rejects the explicit template argument with a converted
// argument and the array extraction below; these map editor streams have
// no Mac caller.
#if !defined(HOMM3_TARGET_MAC)
// The map format's form of an object type (Loki h3maped ObjectType.cpp,
// TRawOStream& operator<<(TRawOStream&, const TObjectType&) and its
// reader): the image name, the two cell masks, the terrain masks in their
// nine-bit stored form, type, subtype, slot category and underlay, then
// sixteen reserved bytes. Only the map editor keeps them (h3maped 0x490944,
// 0x490a11); the game links them unreferenced.
// The writer's wall (90.58): retail expands the reserved fill_n (four
// stosd). Here the vetoed calls before it charge 959 of the 1000 budget
// and leave 41 against fill_n's cb 43 (`vc6 predict-inline --root`);
// a ternary or const getImageName, `bits[i]` in writeBitset and an
// out-of-line string writer do not free it. The nine-bit masks as
// implicit conversions (explicit template argument) give retail's
// recomputed temporary addresses.
VA_AT(h3maped, 0x00490944, 0xcd)
TRawOStream& operator<<(TRawOStream& stream, const TObjectType& objectType)
{
    stream << objectType.getImageName();
    writeBitset(stream, objectType._m_passableMask);
    writeBitset(stream, objectType._m_triggerMask);
    writeBitset<kNumTerrainTypes - 1>(stream, objectType._m_terrainMask.to_ulong());
    writeBitset<kNumTerrainTypes - 1>(stream, objectType._m_recommendedTerrainMask.to_ulong());
    stream << static_cast<long>(objectType._m_type)
           << static_cast<long>(objectType._m_extra)
           << static_cast<signed char>(objectType._m_slotCategory)
           << static_cast<signed char>(objectType._m_bUnderlay);
    signed char reserved[kNumObjectTypeReserved];
    std::fill_n(reserved, sizeof(reserved), 0);
    stream << reserved;
    return stream;
}

VA_AT(h3maped, 0x00490a11, 0x166)
TRawIStream& operator>>(TRawIStream& stream, TObjectType& objectType)
{
    std::string imageName;
    std::bitset<OBJECT_MASK_CELLS> passableMask;
    std::bitset<OBJECT_MASK_CELLS> triggerMask;
    std::bitset<kNumTerrainTypes - 1> terrainMask;
    std::bitset<kNumTerrainTypes - 1> recommendedTerrainMask;
    stream >> imageName;
    readBitset(stream, passableMask);
    readBitset(stream, triggerMask);
    readBitset(stream, terrainMask);
    readBitset(stream, recommendedTerrainMask);
    long type;
    long extra;
    signed char slotCategory;
    signed char bUnderlay;
    stream >> type >> extra >> slotCategory >> bUnderlay;
    signed char reserved[kNumObjectTypeReserved];
    stream >> reserved;
    objectType.setImageName(imageName)
        ._setPassableMask(passableMask)
        ._setTriggerMask(triggerMask)
        .setTerrainMask(terrainMask.to_ulong())
        .setRecommendedTerrainMask(recommendedTerrainMask.to_ulong())
        .setType(TAdventureObjectType(type))
        .setExtra(extra)
        .setSlotCategory(slotCategory)
        .setBUnderlay(bUnderlay != 0);
    return stream;
}
#endif

// Bit by bit from the highest (Loki's isLessThan<N>; h3maped keeps the
// 48-cell and terrain instances out of line, 0x492179 and 0x492203).
template <size_t N>
inline bool isLessThan(const std::bitset<N>& lhs, const std::bitset<N>& rhs)
{
    size_t bit = N;
    while (bit > 0) {
        --bit;
        if (lhs[bit] < rhs[bit])
            return true;
        else if (lhs[bit] != rhs[bit])
            return false;
    }
    return false;
}

// The map editor's object-type map key order (Loki's explicit
// less<TObjectType>, h3maped 0x490b77): image, type, subtype, slot
// category, underlay, then the masks.
VA_AT(h3maped, 0x00490b77, 0xd5)
bool std::less<TObjectType>::operator()(const TObjectType& lhs, const TObjectType& rhs) const
{
    if (lhs._m_imageNum < rhs._m_imageNum)
        return true;
    else if (lhs._m_imageNum != rhs._m_imageNum)
        return false;
    if (lhs._m_type < rhs._m_type)
        return true;
    else if (lhs._m_type != rhs._m_type)
        return false;
    if (lhs._m_extra < rhs._m_extra)
        return true;
    else if (lhs._m_extra != rhs._m_extra)
        return false;
    if (lhs._m_slotCategory < rhs._m_slotCategory)
        return true;
    else if (lhs._m_slotCategory != rhs._m_slotCategory)
        return false;
    if (lhs._m_bUnderlay < rhs._m_bUnderlay)
        return true;
    else if (lhs._m_bUnderlay != rhs._m_bUnderlay)
        return false;
    if (isLessThan(lhs._m_passableMask, rhs._m_passableMask))
        return true;
    else if (lhs._m_passableMask != rhs._m_passableMask)
        return false;
    if (isLessThan(lhs._m_triggerMask, rhs._m_triggerMask))
        return true;
    else if (lhs._m_triggerMask != rhs._m_triggerMask)
        return false;
    if (isLessThan(lhs._m_terrainMask, rhs._m_terrainMask))
        return true;
    else if (lhs._m_terrainMask != rhs._m_terrainMask)
        return false;
    if (isLessThan(lhs._m_recommendedTerrainMask, rhs._m_recommendedTerrainMask))
        return true;
    return false;
}

// The objects.txt reader NewfullMap::loadObjectTypeTemplates drives. Its EH
// data publishes the shape: one try block with a `catch (...)` handler that
// Disposes the text resource and rethrows, so the resource pointer lives in
// the dead parameter home rather than a holder; the per-row stream, its
// strstreambuf and its virtual base unwind separately. Both exit paths call
// the canonical ResourceManager::dispose(resource*).
VA(0x00514d80, 0x284)
MAC_ADDRESS(0x224224, 0x244)  // anchor-callee ResourceManager::GetText + anchor-bracket NewfullMap::loadObjectTypeTemplates; retail-only
void TObjectTypeTable::load(char* filename)
{
    assert(filename != 0);
    TTextResource* text = ResourceManager::GetText(filename);
    if (text == 0)
        throw TRuntimeError();

    try {
        assert(text->GetNumberOfStrings() >= 1);
        int count = atoi(text->GetText(OBJECT_TYPE_TEXT_COUNT));
        assert(text->GetNumberOfStrings() >= count + 1);
        m_objectTypes.resize(count);
        for (int i = 0; i < count; ++i) {
            std::istrstream row(text->GetText(i + 1));
            row >> m_objectTypes[i];
        }
    } catch (...) {
        ResourceManager::Dispose(text);
        throw;
    }
    ResourceManager::Dispose(text);
}

VA_COMPGEN(0x00517780, 0xA3, TREE_CONST_ITERATOR_INC, string)

// The map format's string reader (editor/RawStream.h) resizes its string:
// retail keeps that one-argument resize right after load, inside this
// compiland's band, though the game drops the reader itself.
VA_COMPGEN(0x00515010, 0x198, BASIC_STRING_RESIZE, char)

// This is a library-template enrollment. Do not force its emission with
// an invented game function; its retained body must come from real use.

// --- Dinkumware COMDAT pairings -------------------------------------------

// COMDAT pairing: TUniqueSet<std::string>'s implicit destructor: the
// vector at +0x10 freed, then _Tree::_Erase over the head node.
VA_COMPGEN(0x00514130, 0x7E, IMPLICIT_DTOR, TUniqueSet)
MAC_COMPGEN_ADDRESS(0x223810, 0x88, IMPLICIT_DTOR, TUniqueSet)

// COMDAT pairing: basic_istream<char>'s streambuf constructor (the `_Bool`
// tie-parameter arm - the only istream ctor this object emits).
VA_COMPGEN(0x005151b0, 0xAA, CLASS_CTOR, basic_istream)

VA_COMPGEN(0x00515a40, 0x32, SCALAR_DELETING_DTOR, basic_istream)

// COMDAT pairing: ctype<char>'s three-argument constructor.
VA_COMPGEN(0x00515f50, 0x106, CLASS_CTOR, ctype)

// COMDAT pairing: ctype<char>'s destructor and its scalar deleting wrapper
// (the only ??_G here calling ??1?$ctype@D@std@@).
VA_COMPGEN(0x00516130, 0x21, SCALAR_DELETING_DTOR, ctype)

VA_COMPGEN(0x00516160, 0x24, IMPLICIT_DTOR, ctype)

VA_COMPGEN(0x00516560, 0x23, SCALAR_DELETING_DTOR, TObjectSlotTraits)

// COMDAT pairing: strstreambuf(const char*, int).
VA_COMPGEN(0x005165f0, 0xE7, CLASS_CTOR, strstreambuf)

// COMDAT pairing: istrstream's scalar deleting destructor (it calls the
// CRT's ??1istrstream@std@@).
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

// load retains this unsigned-long constructor for its flipped passable
// temporary. Default construction of trigger and both terrain masks instead
// expands to their retained _Tidy bodies.
VA_COMPGEN(0x005154a0, 0x61, BITSET_CTOR, bitset48)

// COMDAT pairing: bitset<48>::flip(), from TObjectType::setTriggerMask.
VA_COMPGEN(0x00516770, 0x28, BITSET_FLIP, bitset48)

// COMDAT pairing: vector<TObjectType>::insert(ptr, count, const&) and
// ::erase(first, last), both reached from TObjectTypeTable::load.
VA_COMPGEN(0x005167a0, 0x2E1, VECTOR_INSERT, TObjectType)

VA_COMPGEN(0x00516a90, 0x44, VECTOR_ERASE, TObjectType)

// COMDAT pairing: _Tree<string, pair<const string,int>>::erase(first, last)
// - the registry map's range eraser.
VA_COMPGEN(0x00516ae0, 0x12B, TREE_ERASE_RANGE, string)

VA_COMPGEN(0x00516e20, 0x1C, BASIC_STRING_SUBSCRIPT, char)

// COMDAT pairing: invalid_argument's _Doraise and its copy constructor,
// identified by the _ThrowInfo it throws through.
VA_COMPGEN(0x00516f10, 0x1D, EXCEPTION_DORAISE, invalid_argument)

VA_COMPGEN(0x00516f30, 0x157, IMPLICIT_COPY_CTOR, invalid_argument)

// COMDAT pairing: _Tree<string,...>::erase(iterator) (1342 B) and
// ::_Erase(node), the recursive subtree destroyer.
VA_COMPGEN(0x00517090, 0x53E, TREE_ERASE_ITERATOR, string)

VA_COMPGEN(0x005175d0, 0xAD, TREE_ERASE, string)

// COMDAT pairing: num_get<char, istreambuf_iterator<char>>::num_get(size_t),
// the facet constructor.
VA_COMPGEN(0x00517d70, 0x5C, CLASS_CTOR, num_get)
// The map editor's num_get and num_put deleting destructors fold into one.
VA_COMPGEN_AT(h3maped, 0x004360bf, 0x1c, SCALAR_DELETING_DTOR, num_get)

VA_COMPGEN(0x0051a120, 0xCC, CLASS_CTOR, basic_string)

// COMDAT pairing: _Tree<string,...>::insert(const value_type&) and the
// pair<iterator,bool> constructor it returns through; setImageName retains
// map<string,int>::insert as a thin wrapper around it.
VA_COMPGEN(0x00517B70, 0x2C, MAP_INSERT, string)

VA_COMPGEN(0x0051af50, 0x156, TREE_INSERT, string)

VA_COMPGEN(0x0051b150, 0x18, CLASS_CTOR, pair)

// COMDAT pairing: _Tree<string,...>::_Lbound and const_iterator::_Dec.
VA_COMPGEN(0x0051b510, 0xBC, TREE_LBOUND, string)

VA_COMPGEN(0x0051b5d0, 0xB3, TREE_CONST_ITERATOR_DEC, string)

// --- Dinkumware COMDAT pairings, part 2: the input-stream family ----------

// COMDAT pairing: basic_streambuf<char>::sgetc.
VA_COMPGEN(0x005157b0, 0x20, STREAMBUF_SGETC, char)

// COMDAT pairing: basic_string<char>::replace(pos, n, const char*, n),
// - the `.msk` suffix rewrite in setImageName.
VA_COMPGEN(0x00515a80, 0x199, BASIC_STRING_REPLACE, char)

// COMDAT pairing: basic_string<char>::rfind(const char*, pos, n) - the
// `rfind('.')` that finds the extension setImageName replaces.
VA_COMPGEN(0x00515c20, 0x74, BASIC_STRING_RFIND, char)

// COMDAT pairing: basic_istream<char>::ipfx(bool) - the
// whitespace-skipping prefix every extraction in this compiland runs
// through, and the reason use_facet<ctype<char>> is instantiated here.
VA_COMPGEN(0x00515ca0, 0x27A, ISTREAM_IPFX, char)

// COMDAT pairing: basic_ios<char>::setstate.
VA_COMPGEN(0x00515f20, 0x26, BASIC_IOS_SETSTATE, char)

// ctype<char>'s SDK cleanup, registered with atexit by its constructor and
// the expanded constructor in use_facet.
VA_COMPGEN(0x00516060, 0x0D, CTYPE_TERM, char)
VA_COMPGEN_AT(h3maped, 0x00432a00, 0xd, CTYPE_TERM, char)

// COMDAT pairing: ctype<char>'s four case-conversion virtuals (two call
// __Tolower, two __Toupper; scalar and range by size).
VA_COMPGEN(0x00516070, 0x1C, CTYPE_DO_TOLOWER_CHAR, char)

VA_COMPGEN(0x00516090, 0x38, CTYPE_DO_TOLOWER_RANGE, char)

VA_COMPGEN(0x005160d0, 0x1C, CTYPE_DO_TOUPPER_CHAR, char)

VA_COMPGEN(0x005160f0, 0x38, CTYPE_DO_TOUPPER_RANGE, char)

// COMDAT pairing: basic_ios<char>::clear and basic_streambuf<char>::sbumpc.
VA_COMPGEN(0x00517af0, 0x1D, BASIC_IOS_CLEAR, char)

VA_COMPGEN(0x00517b10, 0x32, STREAMBUF_SBUMPC, char)

// The image-name registry's map<string, size_t>::find, which TUniqueSet::add
// calls from setImageName.
VA_COMPGEN(0x00517ba0, 0x86, MAP_FIND, string)

VA_COMPGEN(0x00517dd0, 0x3AF, NUM_GET_DO_GET, char)

VA_COMPGEN(0x00518180, 0x3E1, NUM_GET_DO_GET, char)

VA_COMPGEN(0x00518570, 0x3DD, NUM_GET_DO_GET, char)

VA_COMPGEN(0x00518950, 0x3C9, NUM_GET_DO_GET, char)

VA_COMPGEN(0x00518d20, 0x3C9, NUM_GET_DO_GET, char)

VA_COMPGEN(0x005190f0, 0x40E, NUM_GET_DO_GET, char)

// COMDAT pairing: ctype<char>::is(mask, char) - reached
// from the float arm of do_get immediately above it.
VA_COMPGEN(0x00519500, 0x22, CTYPE_IS, char)

VA_COMPGEN(0x00519530, 0x414, NUM_GET_DO_GET, char)

VA_COMPGEN(0x00519950, 0x414, NUM_GET_DO_GET, char)

VA_COMPGEN(0x00519d70, 0x393, NUM_GET_DO_GET, char)

// COMDAT pairing: num_get<char>::_Getifld - the integer
// field scanner the five integral do_get arms share.
VA_COMPGEN(0x0051a1f0, 0x534, NUM_GET_GETIFLD, char)

VA_COMPGEN(0x0051a110, 0x7, IMPLICIT_DTOR, facet)

// COMDAT pairing: istreambuf_iterator<char>'s operator*, _Inc and _Peek.
VA_COMPGEN(0x0051a730, 0x4F, ISTREAMBUF_ITERATOR_DEREF, char)

VA_COMPGEN(0x0051a8e0, 0x53, ISTREAMBUF_ITERATOR_INC, char)

VA_COMPGEN(0x0051a940, 0x45, ISTREAMBUF_ITERATOR_PEEK, char)

// COMDAT pairing: the two use_facet<> instantiations (ctype's calls
// __Getctype).
VA_COMPGEN(0x0051a990, 0x1FE, USE_FACET_CTYPE, char)

VA_COMPGEN(0x0051ab90, 0x1FB, USE_FACET_NUMPUNCT, char)

// COMDAT pairing: istreambuf_iterator<char>::equal.
VA_COMPGEN(0x0051ad90, 0xB1, ISTREAMBUF_ITERATOR_EQUAL, char)

// COMDAT pairing: _Tidyfac's two remaining instantiations (ctype and
// num_get), each pair sharing one static.
VA_COMPGEN(0x0051ae50, 0x7B, TIDYFAC_CTYPE_SAVE, char)

VA_COMPGEN(0x0051aed0, 0x7B, TIDYFAC_NUM_GET_SAVE, char)

VA_COMPGEN(0x0051b0b0, 0x92, TIDYFAC_CTYPE_TIDY, char)

VA_COMPGEN(0x0051b170, 0x92, TIDYFAC_NUM_GET_TIDY, char)

// The map editor keeps num_put's and numpunct's cleanups here too; their
// _Save bodies (0x437363, 0x4373ff) register them.
VA_COMPGEN_AT(h3maped, 0x00437c1e, 0x4d, TIDYFAC_NUM_PUT_TIDY, char)
VA_COMPGEN_AT(h3maped, 0x00437cb8, 0x4d, TIDYFAC_NUMPUNCT_TIDY, char)

// COMDAT pairing: _Tree<string,...>::_Insert - the
// rebalancing node inserter behind the already-claimed public insert.
VA_COMPGEN(0x0051b210, 0x2FF, TREE_NODE_INSERT, string)

// COMDAT pairing: std::_Maklocstr - the locale-name
// duplicator, reached from use_facet<numpunct>.
VA_COMPGEN(0x0051b690, 0x39, MAKLOCSTR, char)

// COMDAT pairing: _Construct<pair<const string, int>> -
// the registry map's node initializer.
VA_COMPGEN(0x0051b6d0, 0x15B, STD_CONSTRUCT, string_uint_pair)

// COMDAT pairing: basic_string<char>'s two compare overloads.
VA_COMPGEN(0x0051b8b0, 0x54, BASIC_STRING_COMPARE_STR, char)

VA_COMPGEN(0x0051b910, 0x6A, BASIC_STRING_COMPARE_SUBSTR, char)

// --- Dinkumware COMDAT pairings, part 3: the two bitset extractions -------
VA_COMPGEN(0x00515560, 0x24D, ISTREAM_EXTRACT_BITSET, Bitset48)

// The default TObjectType expression combines its two 48-cell masks through
// this free operator.
VA_COMPGEN(0x00515510, 0x49, BITSET_OR, Bitset48)

VA_COMPGEN(0x005157f0, 0x247, ISTREAM_EXTRACT_BITSET, Bitset9)

VA_COMPGEN(0x00516e40, 0xCB, BITSET_XINV, Bitset48)

VA_COMPGEN(0x00517680, 0xCB, BITSET_XINV, Bitset9)

// These three sit far outside this compiland's span: this is one of the few
// objects that emits both runtime_error's and logic_error's copy
// constructors plus runtime_error::_Doraise, and each CatchableType's
// copyFunction identifies them.
VA_COMPGEN(0x004044e0, 0x159, IMPLICIT_COPY_CTOR, logic_error)

VA_COMPGEN(0x0041bc10, 0x1D, EXCEPTION_DORAISE, runtime_error)

VA_COMPGEN(0x0041bc30, 0x159, IMPLICIT_COPY_CTOR, runtime_error)

// The rest of logic_error's own COMDAT group, plus out_of_range's _Doraise,
// all four selected out of this object and all four sitting in the same
// 0x404400..0x404700 run as the copy constructor above.

// The two _Doraise bodies differ only in the copy constructor they call
// (logic_error's here, out_of_range's in customcampaign.cpp).

// 0x404690 is `??1logic_error` (it stores the class's own vtable, releases
// the `what` string and tail-calls ~exception); 0x404660 is the scalar
// deleting destructor that wraps it.

// bitset<10>::_Xran (claimed in border.cpp) is the throwing call site that
// pulls this group into the first COMDAT band.
VA_COMPGEN(0x00404640, 0x1D, EXCEPTION_DORAISE, logic_error)

VA_COMPGEN(0x00404660, 0x21, SCALAR_DELETING_DTOR, logic_error)
// The map editor's /OPT:ICF folds out_of_range's and invalid_argument's
// deleting destructors onto this one (their vtables' slot 1).
VA_COMPGEN_AT(h3maped, 0x004916dd, 0x1c, SCALAR_DELETING_DTOR, logic_error)

VA_COMPGEN(0x00404690, 0x4B, IMPLICIT_DTOR, logic_error)

VA_COMPGEN(0x004046e0, 0x1D, EXCEPTION_DORAISE, out_of_range)

// runtime_error's destructor (it stores runtime_error's vptr); out_of_range's
// emitted destructor installs logic_error's vptr instead.
VA_COMPGEN(0x004b6be0, 0x4B, IMPLICIT_DTOR, runtime_error)

// The TImageInfo single-element insert; its count-insert twin belongs to
// cmbtmgr's TObstacle specialization.
VA_COMPGEN(0x00516c10, 0x20A, VECTOR_INSERT_SINGLE, _TImageInfo)

// This insertion and setupAndLoadObstacles both call this folded size
// helper.
VA_COMPGEN(0x00517750, 0x21, VECTOR_SIZE, _TImageInfo)

// COMDAT pairing: basic_istream<char>'s destructor - the virtual-base vtable
// fixup.
VA_COMPGEN(0x00515260, 0xF, IMPLICIT_DTOR, basic_istream)
VA_COMPGEN_AT(h3maped, 0x00490e57, 0xf, IMPLICIT_DTOR, basic_istream)

VA_COMPGEN(0x00515270, 0x207, ISTREAM_EXTRACT_INT, char)

VA_COMPGEN(0x00517830, 0x2BE, ISTREAM_EXTRACT_STRING, char)

VA_COMPGEN(0x0054C910, 0x21, VECTOR_SIZE, TObjectType)
