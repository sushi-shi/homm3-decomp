// GameMap.cpp - the map model (h3maped object from 0x41e5c5; Loki h3maped
// object 12). TGameMap and TGameMap::TLayer forward to their copy-on-write
// implementations, which this file defines.
//
// Ported so far: the first anonymous helper and the layer's cell lookup.
#include "editor/stdafx.h"

#include <assert.h>
#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include "va.h"
#include "editor/Array.h"
#include "editor/GameMap.h"
#include "editor/GameObject.h"
#include "editor/TilePoint.h"
#include "editor/VictoryCondition.h"
#include "retailobjecttype.h"

namespace {

VA(0x0041e799, 0x92)
bool isBeachBorder(const TGameMap::TLayer& layer, const TTilePoint& loc)
{
    if (layer.getCell(loc).getTerrainType() == eTerrainWater)
        return false;
    bool abAdjacent[8];
    computeAdjacentDirs(layer.getWidth(), layer.getHeight(), loc.x(), loc.y(), abAdjacent);
    for (unsigned int dir = 0; dir < 8; dir++) {
        if (!abAdjacent[dir])
            continue;
        TTilePoint adjLoc = TPoint<int>(loc) + akAdjOffset[dir];
        if (layer.getCell(adjLoc).getTerrainType() == eTerrainWater)
            return true;
    }
    return false;
}

// The largest object footprint; h3maped's height map rows are six cells.
enum { kMaxObjWidth = 8, kMaxObjHeight = 6 };

// The height of each placed cell of an object: an underlay lies at 0,
// anything else rises by one per row from its front, and a passable cell
// that continues a blocked one to its left takes that cell's height.
VA(0x0041e84b, 0xca)
void constructObjectHeightMap(const TGameObject& obj, unsigned int (&heightMap)[kMaxObjWidth][kMaxObjHeight])
{
    for (unsigned int x = 0; x < obj.getWidth(); x++) {
        unsigned int height = obj.getBUnderlay() ? 0 : 1;
        unsigned int y = 0;
        for (;;) {
            if (obj.getBCellPlaced(x, y))
                heightMap[x][y] = height;
            if (++y >= obj.getHeight())
                break;
            if (!obj.getBUnderlay()) {
                if (obj.getBCellPassable(x, y)) {
                    if (x != 0 && !obj.getBCellPassable(x - 1, y))
                        height = heightMap[x - 1][y];
                    else
                        height++;
                } else {
                    if (obj.getBCellPassable(x, y - 1))
                        height = 1;
                    else
                        height++;
                }
            }
        }
    }
}

// The object types the map caps: each info's type, its ordinal in the
// bookkeeping's counts and its cap. Complete lets up to four types share
// one cap (one info), so the map keys every type to its shared info.
struct TCappedObjectTypeInfo {
    TCappedObjectTypeInfo(int type, unsigned int ordinal, unsigned int cap)
        : m_type(type), m_ordinal(ordinal), m_cap(cap) {}

    int m_type;
    unsigned int m_ordinal;
    unsigned int m_cap;
};

class TCappedObjectTypeInfoMap : public std::map<int, const TCappedObjectTypeInfo*> {
public:
    TCappedObjectTypeInfoMap();

private:
    std::vector<TCappedObjectTypeInfo> _m_aInfo;
};

// One type and its cap.
struct TCappedObjectType {
    int m_type;
    unsigned int m_cap;
};

// Types sharing one cap.
template<unsigned int N>
struct TCappedObjectTypeGroup {
    int m_aType[N];
    unsigned int m_cap;
};

DATA(0x00535128)
static const TCappedObjectType akCappedObjectTypes[] = {
    { EVENT, 200 },
    { BLACK_BOX, 200 },
    { OBELISK, 48 },
    { BOAT, 64 },
    { TRAINING_GROUNDS, 32 },
    { DEFENSE_TOWER, 32 },
    { GARDEN_OF_REVELATION, 32 },
    { MERC_CAMP, 32 },
    { POWER_SCHOOL, 32 },
    { TREE_OF_KNOWLEDGE, 32 },
    { LIBRARY, 32 },
    { ARENA, 32 },
    { MAGIC_SCHOOL, 32 },
    { WAR_SCHOOL, 32 },
    { SIREN, 32 },
    { MYSTICAL_GARDEN, 32 },
    { MAGIC_SPRING, 32 },
    { DEAD_GUY, 32 },
    { LEAN_TO, 32 },
    { SEER, 48 },
    { BLACK_MARKET, 32 },
};

DATA(0x005351d0)
static const TCappedObjectTypeGroup<2> akCappedObjectTypePairs[] = {
    { { SIGN, OCEAN_BOTTLE }, 128 },
    { { GARRISON, GARRISON2 }, 48 },
};

DATA(0x005351e8)
static const TCappedObjectTypeGroup<3> kCappedMineTypes = { { MINE, LIGHTHOUSE, ABANDONED_MINE }, 144 };

DATA(0x005351f8)
static const TCappedObjectTypeGroup<5> kCappedGeneratorTypes = {
    { CREATURE_GENERATOR_1, CREATURE_GENERATOR_4, RANDOM_DWELLING_LVL, RANDOM_DWELLING_FACTION,
      RANDOM_DWELLING },
    144
};

VA(0x0041e98d, 0x28d)
TCappedObjectTypeInfoMap::TCappedObjectTypeInfoMap()
{
    const unsigned int kNumSingles = sizeof(akCappedObjectTypes) / sizeof(akCappedObjectTypes[0]);
    const unsigned int kNumPairs = sizeof(akCappedObjectTypePairs) / sizeof(akCappedObjectTypePairs[0]);
    _m_aInfo.reserve(kNumSingles + kNumPairs + 2);
    unsigned int ordinal = 0;
    unsigned int i;
    for (i = 0; i < kNumSingles; i++)
        _m_aInfo.push_back(TCappedObjectTypeInfo(akCappedObjectTypes[i].m_type, ordinal++,
                                                 akCappedObjectTypes[i].m_cap));
    for (i = 0; i < kNumPairs; i++)
        _m_aInfo.push_back(TCappedObjectTypeInfo(akCappedObjectTypePairs[i].m_aType[0], ordinal++,
                                                 akCappedObjectTypePairs[i].m_cap));
    _m_aInfo.push_back(TCappedObjectTypeInfo(kCappedMineTypes.m_aType[0], ordinal++,
                                             kCappedMineTypes.m_cap));
    _m_aInfo.push_back(TCappedObjectTypeInfo(kCappedGeneratorTypes.m_aType[0], ordinal++,
                                             kCappedGeneratorTypes.m_cap));

    unsigned int which = 0;
    for (i = 0; i < kNumSingles; i++)
        insert(value_type(akCappedObjectTypes[i].m_type, &_m_aInfo[which++]));
    for (i = 0; i < kNumPairs; i++, which++) {
        insert(value_type(akCappedObjectTypePairs[i].m_aType[0], &_m_aInfo[which]));
        insert(value_type(akCappedObjectTypePairs[i].m_aType[1], &_m_aInfo[which]));
    }
    insert(value_type(kCappedMineTypes.m_aType[0], &_m_aInfo[which]));
    insert(value_type(kCappedMineTypes.m_aType[1], &_m_aInfo[which]));
    insert(value_type(kCappedMineTypes.m_aType[2], &_m_aInfo[which]));
    which++;
    insert(value_type(kCappedGeneratorTypes.m_aType[0], &_m_aInfo[which]));
    insert(value_type(kCappedGeneratorTypes.m_aType[1], &_m_aInfo[which]));
    insert(value_type(kCappedGeneratorTypes.m_aType[2], &_m_aInfo[which]));
    insert(value_type(kCappedGeneratorTypes.m_aType[3], &_m_aInfo[which]));
    insert(value_type(kCappedGeneratorTypes.m_aType[4], &_m_aInfo[which]));
}

// An object's location as the map's condition records keep it: -1s for
// no object (h3maped 0x42921c fills it).
struct TMapLoc {
    int m_x;
    int m_y;
    int m_layer;
};

// A victory condition as the map keeps it (h3maped's visitors write the
// kind's ordinal as a dword, the two flags at +4/+5, the kind's data at +8).
struct TVictoryConditionData {
    int m_type;
    bool m_bAllowNormalVictory;
    bool m_bAppliesToComputer;
    union {
        struct {
            TArtifact m_artifact;
        } m_aquireArtifact;
        struct {
            TCreatureType m_creatureType;
            unsigned int m_quantity;
        } m_accumulateCreature;
        struct {
            TGameResourceType m_resourceType;
            unsigned int m_quantity;
        } m_accumulateResource;
        struct {
            TMapLoc m_townLoc;
            int m_hallLevel;
            int m_castleLevel;
        } m_upgradeTown;
        struct {
            TMapLoc m_townLoc;
        } m_buildHolyGrailStruct;
        struct {
            TMapLoc m_heroLoc;
        } m_defeatHero;
        struct {
            TMapLoc m_townLoc;
        } m_captureTown;
        struct {
            TMapLoc m_monsterLoc;
        } m_defeatMonster;
        struct {
            TArtifact m_artifact;
            TMapLoc m_townLoc;
        } m_transportArtifact;
    };
};

// A loss condition as the map keeps it.
struct TLossConditionData {
    int m_type;
    union {
        struct {
            TMapLoc m_townLoc;
        } m_loseTown;
        struct {
            TMapLoc m_heroLoc;
        } m_loseHero;
        struct {
            unsigned int m_numDays;
        } m_timeExpires;
    };
};

}  // namespace

// The map's implementation: so far only the dimension of each size.
class TGameMap::_TImpl {
public:
    class _TGetVictoryConditionDataFunc;
    class _TGetLossConditionDataFunc;

    void _getObjectLoc(const TMapObjectRef& objRef, TMapLoc* pLoc) const;

    // The caps the failures report (h3maped 0x41ec98: 156 heroes; 0x41ecb8:
    // 48 towns).
    enum { s_kMaxHeroesOnMap = 156, s_kMaxTownsOnMap = 48 };

    static const unsigned int _s_akDimension[TGameMap::s_kNumSizes];
};

VA(0x0041ec98, 0x20)
TPlaceObjFailureTooManyHeroesOnMap::TPlaceObjFailureTooManyHeroesOnMap()
    : TPlaceObjFailureTooManyInstancesOfTypeOnMap(HERO, TGameMap::_TImpl::s_kMaxHeroesOnMap)
{
}

VA(0x0041ecb8, 0x20)
TPlaceObjFailureTooManyTownsOnMap::TPlaceObjFailureTooManyTownsOnMap()
    : TPlaceObjFailureTooManyInstancesOfTypeOnMap(TOWN, TGameMap::_TImpl::s_kMaxTownsOnMap)
{
}

// Fills a map's victory-condition record from a condition (h3maped RTTI
// _TGetVictoryConditionDataFunc@_TImpl@TGameMap, twelve slots).
class TGameMap::_TImpl::_TGetVictoryConditionDataFunc : public TVictoryCondition::TVisitor {
public:
    _TGetVictoryConditionDataFunc(const _TImpl& map, TVictoryConditionData* pData)
        : _m_map(map), _m_pData(pData) {}

    virtual void visit(const TVCAquireArtifact& vc);
    virtual void visit(const TVCAccumulateCreature& vc);
    virtual void visit(const TVCAccumulateResource& vc);
    virtual void visit(const TVCUpgradeTown& vc);
    virtual void visit(const TVCBuildHolyGrailStruct& vc);
    virtual void visit(const TVCDefeatHero& vc);
    virtual void visit(const TVCCaptureTown& vc);
    virtual void visit(const TVCDefeatMonster& vc);
    virtual void visit(const TVCFlagAllCreatureGenerators& vc);
    virtual void visit(const TVCFlagAllMines& vc);
    virtual void visit(const TVCTransportArtifact& vc);

private:
    void _setHeader(TVictoryConditionType type, const TVictoryCondition& vc)
    {
        _m_pData->m_type = type;
        _m_pData->m_bAllowNormalVictory = vc.getBAllowNormalVictory();
        _m_pData->m_bAppliesToComputer = vc.getBAppliesToComputer();
    }

    const _TImpl& _m_map;
    TVictoryConditionData* _m_pData;
};

// ...and its loss-condition record (four slots).
class TGameMap::_TImpl::_TGetLossConditionDataFunc : public TLossCondition::TVisitor {
public:
    _TGetLossConditionDataFunc(const _TImpl& map, TLossConditionData* pData)
        : _m_map(map), _m_pData(pData) {}

    virtual void visit(const TLCLoseTown& lc);
    virtual void visit(const TLCLoseHero& lc);
    virtual void visit(const TLCTimeExpires& lc);

private:
    const _TImpl& _m_map;
    TLossConditionData* _m_pData;
};

VA(0x0041ecd8, 0x29)
void TGameMap::_TImpl::_TGetVictoryConditionDataFunc::visit(const TVCAquireArtifact& vc)
{
    _setHeader(eVCAquireArtifact, vc);
    _m_pData->m_aquireArtifact.m_artifact = vc.getArtifact();
}

VA(0x0041ed01, 0x38)
void TGameMap::_TImpl::_TGetVictoryConditionDataFunc::visit(const TVCAccumulateCreature& vc)
{
    _setHeader(eVCAccumulateCreature, vc);
    _m_pData->m_accumulateCreature.m_creatureType = vc.getCreatureType();
    _m_pData->m_accumulateCreature.m_quantity = vc.getQuantity();
}

VA(0x0041ed39, 0x38)
void TGameMap::_TImpl::_TGetVictoryConditionDataFunc::visit(const TVCAccumulateResource& vc)
{
    _setHeader(eVCAccumulateResource, vc);
    _m_pData->m_accumulateResource.m_resourceType = vc.getResourceType();
    _m_pData->m_accumulateResource.m_quantity = vc.getQuantity();
}

VA(0x0041ed71, 0x4d)
void TGameMap::_TImpl::_TGetVictoryConditionDataFunc::visit(const TVCUpgradeTown& vc)
{
    _setHeader(eVCUpgradeTown, vc);
    _m_map._getObjectLoc(vc.getTownRef(), &_m_pData->m_upgradeTown.m_townLoc);
    _m_pData->m_upgradeTown.m_hallLevel = vc.getHallLevel();
    _m_pData->m_upgradeTown.m_castleLevel = vc.getCastleLevel();
}

VA(0x0041edbe, 0x3b)
void TGameMap::_TImpl::_TGetVictoryConditionDataFunc::visit(const TVCBuildHolyGrailStruct& vc)
{
    _setHeader(eVCBuildHolyGrailStruct, vc);
    _m_map._getObjectLoc(vc.getTownRef(), &_m_pData->m_buildHolyGrailStruct.m_townLoc);
}

VA(0x0041edf9, 0x3b)
void TGameMap::_TImpl::_TGetVictoryConditionDataFunc::visit(const TVCDefeatHero& vc)
{
    _setHeader(eVCDefeatHero, vc);
    _m_map._getObjectLoc(vc.getHeroRef(), &_m_pData->m_defeatHero.m_heroLoc);
}

VA(0x0041ee34, 0x3b)
void TGameMap::_TImpl::_TGetVictoryConditionDataFunc::visit(const TVCCaptureTown& vc)
{
    _setHeader(eVCCaptureTown, vc);
    _m_map._getObjectLoc(vc.getTownRef(), &_m_pData->m_captureTown.m_townLoc);
}

VA(0x0041ee6f, 0x3b)
void TGameMap::_TImpl::_TGetVictoryConditionDataFunc::visit(const TVCDefeatMonster& vc)
{
    _setHeader(eVCDefeatMonster, vc);
    _m_map._getObjectLoc(vc.getMonsterRef(), &_m_pData->m_defeatMonster.m_monsterLoc);
}

VA(0x0041eeaa, 0x24)
void TGameMap::_TImpl::_TGetVictoryConditionDataFunc::visit(const TVCFlagAllCreatureGenerators& vc)
{
    _setHeader(eVCFlagAllCreatureGenerators, vc);
}

VA(0x0041eece, 0x24)
void TGameMap::_TImpl::_TGetVictoryConditionDataFunc::visit(const TVCFlagAllMines& vc)
{
    _setHeader(eVCFlagAllMines, vc);
}

VA(0x0041eef2, 0x42)
void TGameMap::_TImpl::_TGetVictoryConditionDataFunc::visit(const TVCTransportArtifact& vc)
{
    _setHeader(eVCTransportArtifact, vc);
    _m_pData->m_transportArtifact.m_artifact = vc.getArtifact();
    _m_map._getObjectLoc(vc.getTownRef(), &_m_pData->m_transportArtifact.m_townLoc);
}

VA(0x0041ef34, 0x20)
void TGameMap::_TImpl::_TGetLossConditionDataFunc::visit(const TLCLoseTown& lc)
{
    _m_pData->m_type = eLCLoseTown;
    _m_map._getObjectLoc(lc.getTownRef(), &_m_pData->m_loseTown.m_townLoc);
}

VA(0x0041ef54, 0x23)
void TGameMap::_TImpl::_TGetLossConditionDataFunc::visit(const TLCLoseHero& lc)
{
    _m_pData->m_type = eLCLoseHero;
    _m_map._getObjectLoc(lc.getHeroRef(), &_m_pData->m_loseHero.m_heroLoc);
}

VA(0x0041ef77, 0x19)
void TGameMap::_TImpl::_TGetLossConditionDataFunc::visit(const TLCTimeExpires& lc)
{
    _m_pData->m_type = eLCTimeExpires;
    _m_pData->m_timeExpires.m_numDays = lc.getNumDays();
}

DATA(0x00535214)
const unsigned int TGameMap::_TImpl::_s_akDimension[TGameMap::s_kNumSizes] = { 36, 72, 108, 144 };

// A layer's implementation: its size and its cells.
class TGameMap::TLayer::_TImpl {
public:
    static const TMapLayerObjectID s_kInvalidObjID;

    _TImpl(TGameMap::TSize size);
    _TImpl(const _TImpl& other);
    ~_TImpl();

    TCell* getPCell(unsigned int x, unsigned int y) { return _m_pCellGrid->getPCell(x, y); }
    TCell* getPCell(const TTilePoint& loc) { return getPCell(loc.x(), loc.y()); }
    unsigned int getWidth() const { return TGameMap::_TImpl::_s_akDimension[_m_size]; }
    unsigned int getHeight() const { return TGameMap::_TImpl::_s_akDimension[_m_size]; }
    const TCell* getPCell(unsigned int x, unsigned int y) const;
    const TCell& getCell(unsigned int x, unsigned int y) const { return *getPCell(x, y); }
    TGameObject* getPObject(unsigned int objID);
    const TGameObject* getPObject(unsigned int objID) const;
    const TGameObject& getObject(unsigned int objID) const { return *getPObject(objID); }
    TTilePoint getObjectLoc(unsigned int objID) const;
    TTileExtent getObjectExtent(unsigned int objID) const;
    TMapLayerObjectID getFloatingObjID() const { return _m_floatingObjID; }
    bool isObjectIDValid(unsigned int objID) const;
    TMapLayerObjectID getFirstObjectID() const { return (*_m_paObjectLink)[0].m_next; }
    TMapLayerObjectID getLastObjectID() const { return (*_m_paObjectLink)[0].m_prev; }
    TMapLayerObjectID getNextObjectID(unsigned int objID) const { return (*_m_paObjectLink)[objID].m_next; }
    TMapLayerObjectID getPrevObjectID(unsigned int objID) const { return (*_m_paObjectLink)[objID].m_prev; }
    unsigned int getNumObjectIDsAtCell(unsigned int x, unsigned int y) const;
    unsigned int getNumObjectIDsAtCell(const TTilePoint& loc) const { return getNumObjectIDsAtCell(loc.x(), loc.y()); }
    TMapLayerObjectID getObjectIDAtCell(unsigned int x, unsigned int y, unsigned int which) const;
    TMapLayerObjectID getObjectIDAtCell(const TTilePoint& loc, unsigned int which) const;
    unsigned int getNumShadowIDsAtCell(unsigned int x, unsigned int y) const;
    TMapLayerObjectID getShadowIDAtCell(unsigned int x, unsigned int y, unsigned int which) const;

    TMapLayerObjectID _placeObject(auto_ptr<TGameObject> pObj, const TTilePoint& loc);
    void _removeObject(unsigned int objID);
    void _floatObject(unsigned int objID);
    void _unfloatObject(const TTilePoint& loc);
    TMapLayerObjectID _findObject(const TTilePoint& loc, bool (*pfnPredicate)(const TGameObject&)) const;

private:
    // A slot in the layer's object list: the doubly linked order of placed
    // objects (slot 0 is the list head), the object's location and its shared,
    // copy-on-write object. Windows' wrapper owns the object through an
    // auto_ptr (h3maped 0x4370f4 takes it by value; the object follows the
    // count at +4, owner flag first).
    class _TObjectLink {
    public:
        _TObjectLink() : _m_pWrapper(NULL) {}
        _TObjectLink(const _TObjectLink& other)
            : m_next(other.m_next), m_prev(other.m_prev), m_loc(other.m_loc), _m_pWrapper(other._m_pWrapper)
        {
            if (_m_pWrapper != NULL)
                ++_m_pWrapper->m_refCnt;
        }
        ~_TObjectLink()
        {
            if (_m_pWrapper != NULL && --_m_pWrapper->m_refCnt == 0)
                delete _m_pWrapper;
        }
        _TObjectLink& operator=(const _TObjectLink& other)
        {
            m_next = other.m_next;
            m_prev = other.m_prev;
            m_loc = other.m_loc;
            _TWrapper* pOldWrapper = _m_pWrapper;
            if ((_m_pWrapper = other._m_pWrapper) != NULL)
                ++_m_pWrapper->m_refCnt;
            if (pOldWrapper != NULL && --pOldWrapper->m_refCnt == 0)
                delete pOldWrapper;
            return *this;
        }

        // Loki's setObject(NULL) is its own function here (0x42abe8).
        void setObject(auto_ptr<TGameObject> pObj)
        {
            _m_pWrapper = new _TWrapper(pObj);
            if (_m_pWrapper == NULL)
                _fail();
        }
        void clearObject()
        {
            if (--_m_pWrapper->m_refCnt == 0)
                delete _m_pWrapper;
            _m_pWrapper = NULL;
        }

        TGameObject* getPObject()
        {
            if (_m_pWrapper != NULL) {
                if (_m_pWrapper->m_refCnt > 1)
                    _split();
                return _m_pWrapper->m_pObject.get();
            }
            return NULL;
        }
        const TGameObject* getPObject() const
        {
            return _m_pWrapper != NULL ? _m_pWrapper->m_pObject.get() : NULL;
        }

        TMapLayerObjectID m_next;
        TMapLayerObjectID m_prev;
        TTilePoint m_loc;

    private:
        struct _TWrapper {
            _TWrapper(auto_ptr<TGameObject> pObject) : m_refCnt(1), m_pObject(pObject) {}

            unsigned int m_refCnt;
            auto_ptr<TGameObject> m_pObject;
        };

        void _fail()
        {
            throw TAllocationFailure();
        }
        void _split()
        {
            auto_ptr<TGameObject> pClone = _m_pWrapper->m_pObject->clone();
            if (pClone.get() == NULL)
                _fail();
            _TWrapper* pNewWrapper = new _TWrapper(pClone);
            if (pNewWrapper == NULL)
                _fail();
            --_m_pWrapper->m_refCnt;
            _m_pWrapper = pNewWrapper;
        }

        _TWrapper* _m_pWrapper;
    };

    // The layer's cells in shared, copy-on-write segments of
    // s_kSegmentDim x s_kSegmentDim (six on Windows: the lookup at
    // 0x42a4d0 divides by 6; Loki's port uses 9).
    class _TCellGrid {
    public:
        _TCellGrid() : _m_widthInSegments(0) {}

        void resize(unsigned int width, unsigned int height)
        {
            _m_widthInSegments = width / s_kSegmentDim;
            _m_aSegment.resize(height / s_kSegmentDim * _m_widthInSegments);
        }
        TCell* getPCell(unsigned int x, unsigned int y)
        {
            return &(*_m_aSegment[y / s_kSegmentDim * _m_widthInSegments + x / s_kSegmentDim])
                [y % s_kSegmentDim][x % s_kSegmentDim];
        }
        const TCell* getPCell(unsigned int x, unsigned int y) const;

    private:
        enum { s_kSegmentDim = 6 };
        typedef TArray<TArray<TCell, s_kSegmentDim>, s_kSegmentDim> _TSegment;

        unsigned int _m_widthInSegments;
        vector<TRefCountingPtr<_TSegment> > _m_aSegment;
    };

    void _stampObject(TGameObject* pObj, const TTilePoint& loc, unsigned int objID);
    void _unstampObject(unsigned int objID);
    TTileExtent _computeObjExtent(const TTilePoint& loc, const TPoint<unsigned int>& size) const;

    TGameMap::TSize _m_size;
    TRefCountingPtr<_TCellGrid> _m_pCellGrid;
    TMapLayerObjectID _m_nextAvail;
    TRefCountingPtr<vector<_TObjectLink> > _m_paObjectLink;
    TMapLayerObjectID _m_floatingObjID;
};

const TMapLayerObjectID TGameMap::TLayer::_TImpl::s_kInvalidObjID = 0;

VA(0x0042a48c, 0x44)
bool TGameMap::TLayer::_TImpl::isObjectIDValid(unsigned int objID) const
{
    return objID < _m_paObjectLink->size() && (*_m_paObjectLink)[objID].getPObject() != NULL;
}

VA(0x0042a4d0, 0x4c)
const TGameMap::TLayer::TCell* TGameMap::TLayer::_TImpl::_TCellGrid::getPCell(unsigned int x,
                                                                             unsigned int y) const
{
    return &(*_m_aSegment[y / s_kSegmentDim * _m_widthInSegments + x / s_kSegmentDim])
        [y % s_kSegmentDim][x % s_kSegmentDim];
}

VA(0x0042a51c, 0x29)
TGameMap::TLayer::_TImpl::_TImpl(const _TImpl& other)
    : _m_size(other._m_size), _m_pCellGrid(other._m_pCellGrid), _m_nextAvail(other._m_nextAvail),
      _m_paObjectLink(other._m_paObjectLink), _m_floatingObjID(other._m_floatingObjID)
{
}

VA(0x0042a545, 0xbb)
TGameMap::TLayer::_TImpl::_TImpl(TGameMap::TSize size)
    : _m_size(size), _m_nextAvail(0), _m_floatingObjID(0)
{
    _m_pCellGrid->resize(getWidth(), getHeight());
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    aObjectLink.resize(1, _TObjectLink());
    aObjectLink[0].m_next = aObjectLink[0].m_prev = 0;
}

VA(0x0042a67d, 0x36)
TGameMap::TLayer::_TImpl::~_TImpl()
{
}

VA(0x0042a70f, 0x2b)
TGameObject* TGameMap::TLayer::_TImpl::getPObject(unsigned int objID)
{
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    return aObjectLink[objID].getPObject();
}

VA(0x0042a824, 0x16)
const TGameMap::TLayer::TCell* TGameMap::TLayer::_TImpl::getPCell(unsigned int x, unsigned int y) const
{
    return _m_pCellGrid->getPCell(x, y);
}

VA(0x0042a83a, 0x21)
const TGameObject* TGameMap::TLayer::_TImpl::getPObject(unsigned int objID) const
{
    const vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    return aObjectLink[objID].getPObject();
}

VA(0x0042a85b, 0x23)
TTilePoint TGameMap::TLayer::_TImpl::getObjectLoc(unsigned int objID) const
{
    const vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    return aObjectLink[objID].m_loc;
}

VA(0x0042a87e, 0x5b)
TTileExtent TGameMap::TLayer::_TImpl::getObjectExtent(unsigned int objID) const
{
    const vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    const TGameObject* pObj = aObjectLink[objID].getPObject();
    return _computeObjExtent(aObjectLink[objID].m_loc, TPoint<unsigned int>(pObj->getWidth(), pObj->getHeight()));
}

VA(0x0042a8d9, 0x39)
unsigned int TGameMap::TLayer::_TImpl::getNumObjectIDsAtCell(unsigned int x, unsigned int y) const
{
    const vector<_TObjectCellInfo>* paObjInfo = getCell(x, y)._m_paObjInfo.get();
    return paObjInfo != NULL ? paObjInfo->size() : 0;
}

VA(0x0042a912, 0x2c)
TMapLayerObjectID TGameMap::TLayer::_TImpl::getObjectIDAtCell(unsigned int x, unsigned int y,
                                                             unsigned int which) const
{
    const vector<_TObjectCellInfo>* paObjInfo = getCell(x, y)._m_paObjInfo.get();
    return (*paObjInfo)[which].m_objID;
}

VA(0x0042a93e, 0x146)
TMapLayerObjectID TGameMap::TLayer::_TImpl::_placeObject(auto_ptr<TGameObject> pObj, const TTilePoint& loc)
{
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    TMapLayerObjectID objID;
    if (_m_nextAvail != s_kInvalidObjID) {
        objID = _m_nextAvail;
        _m_nextAvail = aObjectLink[_m_nextAvail].m_next;
    } else {
        objID = aObjectLink.size();
        aObjectLink.resize(aObjectLink.size() + 1, _TObjectLink());
        aObjectLink[objID].m_next = _m_nextAvail;
    }
    try {
        aObjectLink[objID].setObject(pObj);
        aObjectLink[objID].m_next = 0;
        aObjectLink[objID].m_prev = aObjectLink[0].m_prev;
        aObjectLink[aObjectLink[0].m_prev].m_next = objID;
        aObjectLink[0].m_prev = objID;
        aObjectLink[objID].m_loc = loc;
        try {
            _stampObject(aObjectLink[objID].getPObject(), loc, objID);
        } catch (...) {
            aObjectLink[aObjectLink[objID].m_prev].m_next = aObjectLink[objID].m_next;
            aObjectLink[aObjectLink[objID].m_next].m_prev = aObjectLink[objID].m_prev;
            aObjectLink[objID].m_next = _m_nextAvail;
            throw;
        }
    } catch (...) {
        _m_nextAvail = objID;
        throw;
    }
    return objID;
}

VA(0x0042ab68, 0x80)
void TGameMap::TLayer::_TImpl::_removeObject(unsigned int objID)
{
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    if (_m_floatingObjID != objID)
        _unstampObject(objID);
    else
        _m_floatingObjID = s_kInvalidObjID;
    aObjectLink[aObjectLink[objID].m_prev].m_next = aObjectLink[objID].m_next;
    aObjectLink[aObjectLink[objID].m_next].m_prev = aObjectLink[objID].m_prev;
    aObjectLink[objID].m_next = _m_nextAvail;
    _m_nextAvail = objID;
    aObjectLink[objID].clearObject();
}

VA(0x0042ac12, 0x332)
void TGameMap::TLayer::_TImpl::_stampObject(TGameObject* pObj, const TTilePoint& loc, unsigned int objID)
{
    const TTileExtent objExtent = _computeObjExtent(loc, TPoint<unsigned int>(pObj->getWidth(), pObj->getHeight()));
    unsigned int heightMap[kMaxObjWidth][kMaxObjHeight];
    constructObjectHeightMap(*pObj, heightMap);
    for (unsigned int x = 0; x < pObj->getWidth(); x++) {
        unsigned int mapX = loc.x() - x;
        if (mapX >= getWidth())
            continue;
        for (unsigned int y = 0; y < pObj->getHeight(); y++) {
            if (pObj->getBCellPlaced(x, y)) {
                unsigned int mapY = loc.y() - y;
                if (mapY < getHeight()) {
                    unsigned int height = heightMap[x][y];
                    TCell* pCell = getPCell(mapX, mapY);
                    vector<_TObjectCellInfo>& aObjInfo = *pCell->_m_paObjInfo;
                    vector<_TObjectCellInfo>::iterator pInsertAt = aObjInfo.end();
                    while (pInsertAt != aObjInfo.begin()) {
                        const _TObjectCellInfo* const pPrev = pInsertAt - 1;
                        if (height > pPrev->m_height)
                            break;
                        if (height == pPrev->m_height) {
                            if (pObj->getBUnderlay())
                                break;
                            bool bObjOnMapIsAbove = false;
                            TMapLayerObjectID objOnMapID = pPrev->m_objID;
                            const TGameObject& objOnMap = getObject(objOnMapID);
                            TTilePoint objOnMapLoc = getObjectLoc(objOnMapID);
                            TTileExtent objOnMapExtent = getObjectExtent(objOnMapID);
                            TTileExtent intersectExtent = objOnMapExtent & objExtent;
                            for (unsigned int ix = intersectExtent.left(); ix < intersectExtent.right(); ix++) {
                                unsigned int objX = loc.x() - ix;
                                unsigned int objOnMapX = objOnMapLoc.x() - ix;
                                for (unsigned int iy = intersectExtent.top(); iy < intersectExtent.bottom(); iy++) {
                                    unsigned int objY = loc.y() - iy;
                                    if (!pObj->getBCellPlaced(objX, objY))
                                        continue;
                                    unsigned int objOnMapY = objOnMapLoc.y() - iy;
                                    if (!objOnMap.getBCellPlaced(objOnMapX, objOnMapY))
                                        continue;
                                    unsigned int intersectHeight = heightMap[objX][objY];
                                    const TCell& intersectCell = getCell(ix, iy);
                                    const vector<_TObjectCellInfo>& aIntersectCellObjInfo = *intersectCell._m_paObjInfo;
                                    vector<_TObjectCellInfo>::const_iterator pObjOnMapCellInfo = aIntersectCellObjInfo.begin();
                                    while (pObjOnMapCellInfo->m_objID != objOnMapID)
                                        ++pObjOnMapCellInfo;
                                    unsigned int objOnMapHeight = pObjOnMapCellInfo->m_height;
                                    if (objOnMapHeight > intersectHeight) {
                                        bObjOnMapIsAbove = true;
                                        break;
                                    }
                                }
                                if (bObjOnMapIsAbove)
                                    break;
                            }
                            if (!bObjOnMapIsAbove)
                                break;
                        }
                        --pInsertAt;
                    }
                    aObjInfo.insert(pInsertAt, _TObjectCellInfo(objID, height));
                }
            }
            if (pObj->getBCellShadow(x, y)) {
                unsigned int mapY = loc.y() - y;
                if (mapY < getHeight()) {
                    TCell* pCell = getPCell(mapX, mapY);
                    vector<unsigned int>& aShadowID = *pCell->_m_paShadowID;
                    aShadowID.push_back(objID);
                }
            }
        }
    }
}

VA(0x0042b127, 0x1a5)
void TGameMap::TLayer::_TImpl::_unstampObject(unsigned int objID)
{
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    TTilePoint loc = aObjectLink[objID].m_loc;
    const TGameObject* pObj = static_cast<const _TObjectLink&>(aObjectLink[objID]).getPObject();
    const TTileExtent objExtent = _computeObjExtent(loc, TPoint<unsigned int>(pObj->getWidth(), pObj->getHeight()));
    TTilePoint cell;
    for (cell.y(objExtent.top()); cell.y() < objExtent.bottom(); cell.y(cell.y() + 1)) {
        for (cell.x(objExtent.left()); cell.x() < objExtent.right(); cell.x(cell.x() + 1)) {
            TTilePoint objCell = loc - cell;
            if (pObj->getBCellPlaced(objCell.x(), objCell.y())) {
                TCell* pCell = getPCell(cell);
                vector<_TObjectCellInfo>& aObjInfo = *pCell->_m_paObjInfo;
                vector<_TObjectCellInfo>::iterator pObjInfo = aObjInfo.begin();
                while (pObjInfo->m_objID != objID)
                    ++pObjInfo;
                aObjInfo.erase(pObjInfo);
                if (aObjInfo.size() == 0)
                    pCell->_m_paObjInfo.clear();
            }
            if (pObj->getBCellShadow(objCell.x(), objCell.y())) {
                TCell* pCell = getPCell(cell);
                vector<unsigned int>& aShadowID = *pCell->_m_paShadowID;
                vector<unsigned int>::iterator pObjID = find(aShadowID.begin(), aShadowID.end(), objID);
                aShadowID.erase(pObjID);
                if (aShadowID.size() == 0)
                    pCell->_m_paShadowID.clear();
            }
        }
    }
}

VA(0x0042b32a, 0x6f)
void TGameMap::TLayer::_TImpl::_floatObject(unsigned int objID)
{
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    _unstampObject(objID);
    aObjectLink[aObjectLink[objID].m_prev].m_next = aObjectLink[objID].m_next;
    aObjectLink[aObjectLink[objID].m_next].m_prev = aObjectLink[objID].m_prev;
    aObjectLink[objID].m_next = aObjectLink[objID].m_prev = objID;
    _m_floatingObjID = objID;
}

VA(0x0042b399, 0x99)
void TGameMap::TLayer::_TImpl::_unfloatObject(const TTilePoint& loc)
{
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    aObjectLink[_m_floatingObjID].m_loc = loc;
    aObjectLink[_m_floatingObjID].m_next = 0;
    aObjectLink[_m_floatingObjID].m_prev = aObjectLink[0].m_prev;
    aObjectLink[aObjectLink[0].m_prev].m_next = _m_floatingObjID;
    aObjectLink[0].m_prev = _m_floatingObjID;
    _stampObject(const_cast<TGameObject*>(static_cast<const _TObjectLink&>(aObjectLink[_m_floatingObjID]).getPObject()),
                 loc, _m_floatingObjID);
    _m_floatingObjID = s_kInvalidObjID;
}

VA(0x0042b432, 0x7f)
TTileExtent TGameMap::TLayer::_TImpl::_computeObjExtent(const TTilePoint& loc, const TPoint<unsigned int>& size) const
{
    TTilePoint lastCell = loc;
    TPoint<unsigned int> clippedSize = size;
    if (clippedSize.x() > lastCell.x() + 1)
        clippedSize.x(lastCell.x() + 1);
    if (lastCell.x() >= getWidth()) {
        clippedSize.x(clippedSize.x() - (lastCell.x() - (getWidth() - 1)));
        lastCell.x(getWidth() - 1);
    }
    if (clippedSize.y() > lastCell.y() + 1)
        clippedSize.y(lastCell.y() + 1);
    if (lastCell.y() >= getHeight()) {
        clippedSize.y(clippedSize.y() - (lastCell.y() - (getHeight() - 1)));
        lastCell.y(getHeight() - 1);
    }
    return TTileExtent(lastCell + TPoint<unsigned int>(1, 1) - clippedSize, clippedSize);
}

VA(0x0042b4b1, 0x8b)
TMapLayerObjectID TGameMap::TLayer::_TImpl::_findObject(const TTilePoint& loc,
                                                        bool (*pfnPredicate)(const TGameObject&)) const
{
    unsigned int numObjs = getNumObjectIDsAtCell(loc);
    for (unsigned int i = 0; i < numObjs; i++) {
        TMapLayerObjectID objID = getObjectIDAtCell(loc, i);
        const TGameObject& obj = getObject(objID);
        TTilePoint objLoc = getObjectLoc(objID);
        TTilePoint objCell = objLoc - loc;
        if (obj.getBCellTrigger(objCell.x(), objCell.y()) && pfnPredicate(obj))
            return objID;
    }
    return s_kInvalidObjID;
}

// Defined after _findObject, its first user, which therefore calls it.
VA(0x0042b53c, 0x17)
TMapLayerObjectID TGameMap::TLayer::_TImpl::getObjectIDAtCell(const TTilePoint& loc,
                                                                    unsigned int which) const
{
    return getObjectIDAtCell(loc.x(), loc.y(), which);
}

VA(0x0042b553, 0x51)
TGameMap::TLayer::TLayer(TSize size) : _m_pImpl(_TImpl(size))
{
}

VA(0x0042b5a4, 0x5)
TGameMap::TLayer::~TLayer()
{
}

VA(0x0042b5a9, 0x38)
TGameMap::TLayer::TCell* TGameMap::TLayer::getPCell(unsigned int x, unsigned int y)
{
    return _m_pImpl->getPCell(x, y);
}

VA(0x0042b5e1, 0x21)
TGameObject* TGameMap::TLayer::getPObject(unsigned int objID)
{
    return _m_pImpl->getPObject(objID);
}

VA(0x0042b602, 0xd)
unsigned int TGameMap::TLayer::getWidth() const
{
    return _m_pImpl->getWidth();
}

unsigned int TGameMap::TLayer::getHeight() const
{
    return _m_pImpl->getHeight();
}

VA(0x0042b60f, 0x1b)
const TGameMap::TLayer::TCell* TGameMap::TLayer::getPCell(unsigned int x, unsigned int y) const
{
    return _m_pImpl->getPCell(x, y);
}

VA(0x0042b62a, 0x11)
const TGameObject* TGameMap::TLayer::getPObject(unsigned int objID) const
{
    return _m_pImpl->getPObject(objID);
}

VA(0x0042b63b, 0x25)
TTilePoint TGameMap::TLayer::getObjectLoc(unsigned int objID) const
{
    return _m_pImpl->getObjectLoc(objID);
}

VA(0x0042b660, 0x2a)
TTileExtent TGameMap::TLayer::getObjectExtent(unsigned int objID) const
{
    return _m_pImpl->getObjectExtent(objID);
}

VA(0x0042b68a, 0x6)
TMapLayerObjectID TGameMap::TLayer::getFloatingObjID() const
{
    return _m_pImpl->getFloatingObjID();
}

VA(0x0042b690, 0xb)
TMapLayerObjectID TGameMap::TLayer::getFirstObjectID() const
{
    return _m_pImpl->getFirstObjectID();
}

VA(0x0042b69b, 0xc)
TMapLayerObjectID TGameMap::TLayer::getLastObjectID() const
{
    return _m_pImpl->getLastObjectID();
}

VA(0x0042b6a7, 0x15)
TMapLayerObjectID TGameMap::TLayer::getNextObjectID(unsigned int objID) const
{
    return _m_pImpl->getNextObjectID(objID);
}

VA(0x0042b6bc, 0x16)
TMapLayerObjectID TGameMap::TLayer::getPrevObjectID(unsigned int objID) const
{
    return _m_pImpl->getPrevObjectID(objID);
}

VA(0x0042b6d2, 0x15)
unsigned int TGameMap::TLayer::getNumObjectIDsAtCell(unsigned int x, unsigned int y) const
{
    return _m_pImpl->getNumObjectIDsAtCell(x, y);
}

VA(0x0042b6e7, 0x19)
TMapLayerObjectID TGameMap::TLayer::getObjectIDAtCell(unsigned int x, unsigned int y, unsigned int which) const
{
    return _m_pImpl->getObjectIDAtCell(x, y, which);
}

VA(0x0042b700, 0x15)
unsigned int TGameMap::TLayer::getNumShadowIDsAtCell(unsigned int x, unsigned int y) const
{
    return _m_pImpl->getNumShadowIDsAtCell(x, y);
}

VA(0x0042b715, 0x39)
inline unsigned int TGameMap::TLayer::_TImpl::getNumShadowIDsAtCell(unsigned int x, unsigned int y) const
{
    const vector<unsigned int>* paShadowID = getCell(x, y)._m_paShadowID.get();
    return paShadowID != NULL ? paShadowID->size() : 0;
}

VA(0x0042b74e, 0x19)
TMapLayerObjectID TGameMap::TLayer::getShadowIDAtCell(unsigned int x, unsigned int y, unsigned int which) const
{
    return _m_pImpl->getShadowIDAtCell(x, y, which);
}

VA(0x0042b767, 0x2c)
inline TMapLayerObjectID TGameMap::TLayer::_TImpl::getShadowIDAtCell(unsigned int x, unsigned int y,
                                                                    unsigned int which) const
{
    const vector<unsigned int>* paShadowID = getCell(x, y)._m_paShadowID.get();
    return (*paShadowID)[which];
}

VA(0x0042b793, 0x55)
TMapLayerObjectID TGameMap::TLayer::_placeObject(auto_ptr<TGameObject> pObj, const TTilePoint& loc)
{
    return _m_pImpl->_placeObject(pObj, loc);
}

// The capped-type map's and info vector's template members, emitted here; the
// vector's fill insert folds with the RMG's TRmgMapPosition copy (0x430b35).
VA_COMPGEN(0x0042c28f, 0xa4, VECTOR_RESERVE, TCappedObjectTypeInfo)
VA_COMPGEN(0x0042db3f, 0xf0, TREE_INSERT, TCappedObjectTypeInfo)
VA_COMPGEN(0x00430930, 0x89, TREE_INIT, TCappedObjectTypeInfo)
