// ObjectType.cpp of the Loki port (Loki object 47): the adventure-object
// template, its text and binary forms, its map ordering, the object-palette
// slot traits and the objects.txt loader.
#include <assert.h>
#include <stdlib.h>
#include <algorithm>
#include <functional>
#include <iostream.h>
#include <strstream.h>
#include <string>
#include <vector>

#include "objecttype.h"

#include "editor/RawStream.h"
#include "editor/UniqueSet.h"
#include "exceptions.h"
#include "resourcemanager.h"
#include "textresource.h"

namespace {

TUniqueSet<string>& getImageNameSet()
{
    static TUniqueSet<string> imageNameSet;
    return imageNameSet;
}

// The slot of the objects recommended for one terrain (at most three
// terrains recommended, generic category).
class TTerrainSlotTraits : public TObjectSlotTraits {
public:
    TTerrainSlotTraits(TTerrainType terrainType) : _m_terrainType(terrainType)
    {
#line 58
        assert(terrainType >= 0 && terrainType < eTerrainRock);
    }

    virtual bool contains(const TObjectType& objectType) const;

private:
    TTerrainType _m_terrainType;
};

// The slot of the generic objects recommended for more than three terrains.
class TAllTerrainSlotTraits : public TObjectSlotTraits {
public:
    virtual bool contains(const TObjectType& objectType) const;
};

// The slot of one object category.
class TCategorySlotTraits : public TObjectSlotTraits {
public:
    TCategorySlotTraits(TSlotCategory category) : _m_category(category)
    {
#line 100
        assert(category > eCategoryGeneric && category < kNumSlotCategories);
    }

    virtual bool contains(const TObjectType& objectType) const;

private:
    TSlotCategory _m_category;
};

bool TTerrainSlotTraits::contains(const TObjectType& objectType) const
{
    if (objectType.getSlotCategory() != eCategoryGeneric)
        return false;
    const TTerrainMask& recommendedTerrainMask = objectType.getRecommendedTerrainMask();
    return recommendedTerrainMask[_m_terrainType] && recommendedTerrainMask.count() <= 3;
}

bool TAllTerrainSlotTraits::contains(const TObjectType& objectType) const
{
    return objectType.getSlotCategory() == eCategoryGeneric && objectType.getRecommendedTerrainMask().count() > 3;
}

bool TCategorySlotTraits::contains(const TObjectType& objectType) const
{
    return objectType.getSlotCategory() == _m_category;
}

TTerrainSlotTraits dirtSlotTraits(eTerrainDirt);
TTerrainSlotTraits sandSlotTraits(eTerrainSand);
TTerrainSlotTraits grassSlotTraits(eTerrainGrass);
TTerrainSlotTraits snowSlotTraits(eTerrainSnow);
TTerrainSlotTraits swampSlotTraits(eTerrainSwamp);
TTerrainSlotTraits roughSlotTraits(eTerrainRough);
TTerrainSlotTraits subterraneanSlotTraits(eTerrainSubterranean);
TTerrainSlotTraits lavaSlotTraits(eTerrainLava);
TTerrainSlotTraits waterSlotTraits(eTerrainWater);
TAllTerrainSlotTraits allTerrainSlotTraits;
TCategorySlotTraits townSlotTraits(eCategoryTown);
TCategorySlotTraits monsterSlotTraits(eCategoryMonster);
TCategorySlotTraits heroSlotTraits(eCategoryHero);
TCategorySlotTraits artifactSlotTraits(eCategoryArtifact);
TCategorySlotTraits treasureSlotTraits(eCategoryTreasure);

}

const TObjectSlotTraits* const apObjectSlotTraits[] = {
    &dirtSlotTraits,
    &sandSlotTraits,
    &grassSlotTraits,
    &snowSlotTraits,
    &swampSlotTraits,
    &roughSlotTraits,
    &subterraneanSlotTraits,
    &lavaSlotTraits,
    &waterSlotTraits,
    &allTerrainSlotTraits,
    &townSlotTraits,
    &monsterSlotTraits,
    &heroSlotTraits,
    &artifactSlotTraits,
    &treasureSlotTraits
};

TObjectType::TObjectType()
    : _m_imageNum(0),
      _m_passableMask(~bitset<kMaxObjWidth * kMaxObjHeight>(0)),
      _m_triggerMask(),
      _m_terrainMask(),
      _m_recommendedTerrainMask(),
      _m_type(0),
      _m_extra(0),
      _m_slotCategory(eCategoryGeneric),
      _m_bUnderlay(false),
      _m_bHasTrigger(false),
      _m_triggerLoc(kMaxObjWidth, kMaxObjHeight)
{
}

inline vector<TObjectType::_TImageInfo>& TObjectType::_getAImageInfo()
{
    static vector<_TImageInfo> aImageInfo;
    return aImageInfo;
}

TObjectType& TObjectType::setImageName(const string& newImageName)
{
#line 160
    assert(!newImageName.empty());
    TUniqueSet<string>& imageNameSet = getImageNameSet();
    unsigned int setSize = imageNameSet.numItems();
    _m_imageNum = imageNameSet.add(newImageName);
#line 169
    assert(_m_imageNum <= setSize);
    vector<_TImageInfo>& aImageInfo = _getAImageInfo();
#line 172
    assert(aImageInfo.size() == setSize);
    if (_m_imageNum == setSize) {
        aImageInfo.push_back(_TImageInfo());
        _TImageInfo& imageInfo = aImageInfo[setSize];

        string maskName = newImageName;
        unsigned int dotPos = maskName.find_last_of('.');
        if (dotPos != string::npos)
            maskName.replace(dotPos, maskName.size() - dotPos, ".msk");
        else
            maskName.append(".msk");

        if (ResourceManager::PointToSpriteResource(maskName.c_str())
            || ResourceManager::PointToSpriteResource("default.msk")) {
            char width;
            char height;
            unsigned char placedBits[kMaxObjHeight];
            unsigned char shadowBits[kMaxObjHeight];
            ResourceManager::ReadFromSpriteResource(&width, 1);
            ResourceManager::ReadFromSpriteResource(&height, 1);
            ResourceManager::ReadFromSpriteResource(placedBits, kMaxObjHeight);
            ResourceManager::ReadFromSpriteResource(shadowBits, kMaxObjHeight);
            imageInfo._m_width = width;
            imageInfo._m_height = height;
            for (unsigned int bit = 0; bit < kMaxObjWidth * kMaxObjHeight; bit++) {
                unsigned int byteNum = bit / 8;
                unsigned char bitMask = 1 << (bit % 8);
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

const string& TObjectType::getImageName() const
{
    static const string kNoImageName;
    TUniqueSet<string>& imageNameSet = getImageNameSet();
    return _m_imageNum < imageNameSet.numItems() ? imageNameSet.get(_m_imageNum) : kNoImageName;
}

TObjectType& TObjectType::setBCellPassable(unsigned int x, unsigned int y, bool bPassable)
{
#line 233
    assert(x < kMaxObjWidth);
    assert(y < kMaxObjHeight);
    if (!_m_placedMask[_getBitPos(x, y)])
        bPassable = true;
    _m_passableMask[_getBitPos(x, y)] = bPassable;
    return *this;
}

TObjectType& TObjectType::setBCellTrigger(unsigned int x, unsigned int y, bool bTrigger)
{
#line 246
    assert(x < kMaxObjWidth);
    assert(y < kMaxObjHeight);
    bitset<kMaxObjWidth * kMaxObjHeight> newTriggerMask = _m_triggerMask;
    newTriggerMask[_getBitPos(x, y)] = bTrigger;
    _setTriggerMask(newTriggerMask);
    return *this;
}

TObjectType& TObjectType::setTerrainMask(const TTerrainMask& newTerrainMask)
{
#line 259
    assert(!newTerrainMask[ eTerrainRock ]);
    _m_recommendedTerrainMask &= newTerrainMask;
    _m_terrainMask = newTerrainMask;
    return *this;
}

TObjectType& TObjectType::setRecommendedTerrainMask(const TTerrainMask& newRecommendedTerrainMask)
{
#line 269
    assert(( newRecommendedTerrainMask & ~ _m_terrainMask ) == 0);
    assert(!newRecommendedTerrainMask[ eTerrainRock ]);
    _m_recommendedTerrainMask = newRecommendedTerrainMask;
    return *this;
}

TObjectType& TObjectType::setSlotCategory(TSlotCategory newSlotCategory)
{
#line 279
    assert(newSlotCategory >= 0 && newSlotCategory < kNumSlotCategories);
    _m_slotCategory = newSlotCategory;
    return *this;
}

TObjectType& TObjectType::_setPassableMask(const bitset<kMaxObjWidth * kMaxObjHeight>& newPassableMask)
{
    _m_passableMask = newPassableMask | ~_m_placedMask;
    return *this;
}

TObjectType& TObjectType::_setTriggerMask(const bitset<kMaxObjWidth * kMaxObjHeight>& newTriggerMask)
{
    _m_triggerMask = newTriggerMask & ~_m_passableMask;
    _m_bHasTrigger = false;
    for (int bit = 0; bit < kMaxObjWidth * kMaxObjHeight; bit++)
        if (_m_triggerMask[bit])
            _m_bHasTrigger = true;

    if (_m_bHasTrigger) {
        for (unsigned int y = 0; ; y++) {
#line 311
            assert(y < kMaxObjHeight);
            for (unsigned int x = 0; x < kMaxObjWidth; x++)
                if (_m_triggerMask[_getBitPos(x, y)]) {
                    _m_triggerLoc = TPoint<unsigned int>(x, y);
                    goto found;
                }
        }
    found:
        ;
    } else
        _m_triggerLoc = TPoint<unsigned int>(kMaxObjWidth, kMaxObjHeight);
    return *this;
}

ostream& operator<<(ostream& os, const TObjectType& objectType)
{
    os << objectType.getImageName()
       << ' ' << objectType._m_passableMask
       << ' ' << objectType._m_triggerMask
       << ' ' << bitset<kNumTerrainTypes - 1>(objectType._m_terrainMask.to_ulong())
       << ' ' << bitset<kNumTerrainTypes - 1>(objectType._m_recommendedTerrainMask.to_ulong())
       << ' ' << objectType._m_type
       << ' ' << objectType._m_extra
       << ' ' << objectType._m_slotCategory
       << ' ' << int(objectType._m_bUnderlay);
    return os;
}

istream& operator>>(istream& is, TObjectType& objectType)
{
    string imageName;
    bitset<TObjectType::kMaxObjWidth * TObjectType::kMaxObjHeight> passableMask;
    bitset<TObjectType::kMaxObjWidth * TObjectType::kMaxObjHeight> triggerMask;
    bitset<kNumTerrainTypes - 1> terrainMask;
    bitset<kNumTerrainTypes - 1> recommendedTerrainMask;
    int type;
    int extra;
    int slotCategory;
    int bUnderlay;
    is >> imageName >> passableMask >> triggerMask >> terrainMask >> recommendedTerrainMask
       >> type >> extra >> slotCategory >> bUnderlay;
    objectType.setImageName(imageName)
        ._setPassableMask(passableMask)
        ._setTriggerMask(triggerMask)
        .setTerrainMask(TTerrainMask(terrainMask.to_ulong()))
        .setRecommendedTerrainMask(TTerrainMask(recommendedTerrainMask.to_ulong()))
        .setType(type)
        .setExtra(extra)
        .setSlotCategory(TSlotCategory(slotCategory))
        .setBUnderlay(bUnderlay != 0);
    return is;
}

TRawOStream& operator<<(TRawOStream& stream, const TObjectType& objectType)
{
    stream << objectType.getImageName();
    writeBitset(stream, objectType._m_passableMask);
    writeBitset(stream, objectType._m_triggerMask);
    writeBitset(stream, bitset<kNumTerrainTypes - 1>(objectType._m_terrainMask.to_ulong()));
    writeBitset(stream, bitset<kNumTerrainTypes - 1>(objectType._m_recommendedTerrainMask.to_ulong()));
    stream.operator<< <long>(objectType._m_type)
          .operator<< <long>(objectType._m_extra)
          .operator<< <signed char>(objectType._m_slotCategory)
          .operator<< <signed char>(objectType._m_bUnderlay);
    signed char reserved[16];
    fill_n(reserved, sizeof(reserved), 0);
    stream << reserved;
    return stream;
}

TRawIStream& operator>>(TRawIStream& stream, TObjectType& objectType)
{
    string imageName;
    bitset<TObjectType::kMaxObjWidth * TObjectType::kMaxObjHeight> passableMask;
    bitset<TObjectType::kMaxObjWidth * TObjectType::kMaxObjHeight> triggerMask;
    bitset<kNumTerrainTypes - 1> terrainMask;
    bitset<kNumTerrainTypes - 1> recommendedTerrainMask;
    stream >> imageName;
    readBitset(stream, &passableMask);
    readBitset(stream, &triggerMask);
    readBitset(stream, &terrainMask);
    readBitset(stream, &recommendedTerrainMask);
    long type;
    long extra;
    signed char slotCategory;
    signed char bUnderlay;
    stream >> type >> extra >> slotCategory >> bUnderlay;
    signed char reserved[16];
    stream >> reserved;
    objectType.setImageName(imageName)
        ._setPassableMask(passableMask)
        ._setTriggerMask(triggerMask)
        .setTerrainMask(TTerrainMask(terrainMask.to_ulong()))
        .setRecommendedTerrainMask(TTerrainMask(recommendedTerrainMask.to_ulong()))
        .setType(type)
        .setExtra(extra)
        .setSlotCategory(TSlotCategory(slotCategory))
        .setBUnderlay(bUnderlay != 0);
    return stream;
}

template<size_t N>
inline bool isLessThan(const bitset<N>& lhs, const bitset<N>& rhs)
{
    size_t bit = N;
    while (bit != 0) {
        --bit;
        if (lhs[bit] < rhs[bit])
            return true;
        else if (lhs[bit] != rhs[bit])
            return false;
    }
    return false;
}

bool less<TObjectType>::operator()(const TObjectType& lhs, const TObjectType& rhs) const
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

void TObjectTypeTable::load(const char* fileName)
{
#line 552
    assert(fileName != NULL);
    TTextResource* pTextResource = ResourceManager::GetText(fileName);
    if (pTextResource == NULL)
#line 556
        throw TRuntimeError(__FILE__, __LINE__, string("Unable to load \"") + fileName + "\".");
    try {
#line 560
        assert(pTextResource->GetNumberOfStrings() >= 1);
        int numElements = atoi(pTextResource->GetText(0));
#line 563
        assert(pTextResource->GetNumberOfStrings() >= numElements + 1);
        resize(numElements);
        for (int i = 0; i < numElements; i++) {
            istrstream stream(pTextResource->GetText(i + 1));
            stream >> (*this)[i];
        }
    } catch (...) {
        ResourceManager::Dispose(pTextResource);
        throw;
    }
    ResourceManager::Dispose(pTextResource);
}
