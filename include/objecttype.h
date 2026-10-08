// ObjectType.h of the Loki port (RoE editor source): the adventure-object
// template read from objects.txt, the object-palette slot traits and the
// object-type table. ObjectType.cpp (Loki object 47) owns the bodies;
// ObjectTypeTable.cpp (object 23) owns the loaded table.
#ifndef HOMM3_OBJECTTYPE_H
#define HOMM3_OBJECTTYPE_H

#include <bitset>
#include <functional>
#include <iostream.h>
#include <string>
#include <vector>

#include "terrain.h"
#include "terrain_type.h"
#include "editor/Point.h"

class TRawIStream;
class TRawOStream;

typedef bitset<kNumTerrainTypes> TTerrainMask;

// The object-palette category of objects.txt's group column. Only
// eCategoryGeneric and kNumSlotCategories are named by Loki's asserts; the
// other names follow the editor's palette buttons.
enum TSlotCategory {
    eCategoryGeneric = 0,
    eCategoryTown = 1,
    eCategoryMonster = 2,
    eCategoryHero = 3,
    eCategoryArtifact = 4,
    eCategoryTreasure = 5,
    kNumSlotCategories = 6
};

class TObjectType {
public:
    enum {
        kMaxObjWidth = 8,
        kMaxObjHeight = 6
    };

    TObjectType();

    TObjectType& setImageName(const string& newImageName);
    const string& getImageName() const;
    unsigned int getImageNum() const { return _m_imageNum; }
    unsigned int getWidth() const { return _m_width; }
    unsigned int getHeight() const { return _m_height; }

    bool getBCellPlaced(unsigned int x, unsigned int y) const
    {
        return _m_placedMask[_getBitPos(x, y)];
    }
    bool getBCellPassable(unsigned int x, unsigned int y) const
    {
        return _m_passableMask[_getBitPos(x, y)];
    }
    bool getBCellShadow(unsigned int x, unsigned int y) const
    {
        return _m_shadowMask[_getBitPos(x, y)];
    }
    bool getBCellTrigger(unsigned int x, unsigned int y) const
    {
        return _m_triggerMask[_getBitPos(x, y)];
    }
    TObjectType& setBCellPassable(unsigned int x, unsigned int y, bool bPassable);
    TObjectType& setBCellTrigger(unsigned int x, unsigned int y, bool bTrigger);

    const TTerrainMask& getTerrainMask() const { return _m_terrainMask; }
    TObjectType& setTerrainMask(const TTerrainMask& newTerrainMask);
    const TTerrainMask& getRecommendedTerrainMask() const { return _m_recommendedTerrainMask; }
    TObjectType& setRecommendedTerrainMask(const TTerrainMask& newRecommendedTerrainMask);

    int getType() const { return _m_type; }
    TObjectType& setType(int newType) { _m_type = newType; return *this; }
    int getExtra() const { return _m_extra; }
    TObjectType& setExtra(int newExtra) { _m_extra = newExtra; return *this; }
    TSlotCategory getSlotCategory() const { return _m_slotCategory; }
    TObjectType& setSlotCategory(TSlotCategory newSlotCategory);
    bool getBUnderlay() const { return _m_bUnderlay; }
    TObjectType& setBUnderlay(bool bUnderlay) { _m_bUnderlay = bUnderlay; return *this; }

    bool hasTrigger() const { return _m_bHasTrigger; }
    const TPoint<unsigned int>& getTriggerLoc() const { return _m_triggerLoc; }

private:
    // One per distinct image name, shared by every object type that uses
    // it: the size and the placed and shadow cells of the image's .msk.
    struct _TImageInfo {
        _TImageInfo() : _m_width(0), _m_height(0) {}

        unsigned int _m_width;
        unsigned int _m_height;
        bitset<kMaxObjWidth * kMaxObjHeight> _m_placedMask;
        bitset<kMaxObjWidth * kMaxObjHeight> _m_shadowMask;
    };

    static vector<_TImageInfo>& _getAImageInfo();
    static unsigned int _getBitPos(unsigned int x, unsigned int y)
    {
        return (kMaxObjHeight - y) * kMaxObjWidth - x - 1;
    }

    TObjectType& _setPassableMask(const bitset<kMaxObjWidth * kMaxObjHeight>& newPassableMask);
    TObjectType& _setTriggerMask(const bitset<kMaxObjWidth * kMaxObjHeight>& newTriggerMask);

    friend ostream& operator<<(ostream& os, const TObjectType& objectType);
    friend istream& operator>>(istream& is, TObjectType& objectType);
    friend TRawOStream& operator<<(TRawOStream& stream, const TObjectType& objectType);
    friend TRawIStream& operator>>(TRawIStream& stream, TObjectType& objectType);
    friend struct less<TObjectType>;

    unsigned int _m_imageNum;
    bitset<kMaxObjWidth * kMaxObjHeight> _m_passableMask;
    bitset<kMaxObjWidth * kMaxObjHeight> _m_triggerMask;
    TTerrainMask _m_terrainMask;
    TTerrainMask _m_recommendedTerrainMask;
    int _m_type;
    int _m_extra;
    TSlotCategory _m_slotCategory;
    bool _m_bUnderlay;
    bool _m_bHasTrigger;
    TPoint<unsigned int> _m_triggerLoc;
    unsigned int _m_width;
    unsigned int _m_height;
    bitset<kMaxObjWidth * kMaxObjHeight> _m_placedMask;
    bitset<kMaxObjWidth * kMaxObjHeight> _m_shadowMask;
};

ostream& operator<<(ostream& os, const TObjectType& objectType);
istream& operator>>(istream& is, TObjectType& objectType);
TRawOStream& operator<<(TRawOStream& stream, const TObjectType& objectType);
TRawIStream& operator>>(TRawIStream& stream, TObjectType& objectType);

// Object types are ordered member by member (image, type, subtype, slot
// category, underlay, then the masks) so they can key a map.
template<> bool less<TObjectType>::operator()(const TObjectType& lhs, const TObjectType& rhs) const;

// The object palette's slots: one per terrain but rock, the all-terrain
// slot and one per non-generic category.
class TObjectSlotTraits {
public:
    virtual ~TObjectSlotTraits() {}
    virtual bool contains(const TObjectType& objectType) const = 0;
};

extern const TObjectSlotTraits* const apObjectSlotTraits[];

// The object types of objects.txt, in file order.
class TObjectTypeTable : public vector<TObjectType> {
public:
    TObjectTypeTable() {}
    TObjectTypeTable(const char* fileName) { load(fileName); }
    TObjectTypeTable(unsigned int numElements) : vector<TObjectType>(numElements) {}

    void load(const char* fileName);
};

extern const TObjectTypeTable& kObjectTypeTable;

void loadObjectTypeTable();

#endif  /* HOMM3_OBJECTTYPE_H */
