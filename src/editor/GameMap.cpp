// GameMap.cpp - the map model (h3maped object from 0x41e5c5; Loki h3maped
// object 12). TGameMap and TGameMap::TLayer forward to their copy-on-write
// implementations, which this file defines.
//
// Ported so far: the first anonymous helper and the layer's cell lookup.
#include "editor/stdafx.h"

#include <assert.h>
#include <map>
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
    unsigned int getWidth() const { return TGameMap::_TImpl::_s_akDimension[_m_size]; }
    unsigned int getHeight() const { return TGameMap::_TImpl::_s_akDimension[_m_size]; }
    const TCell* getPCell(unsigned int x, unsigned int y) const { return _m_pCellGrid->getPCell(x, y); }

private:
    // The layer's cells in shared, copy-on-write segments of
    // s_kSegmentDim x s_kSegmentDim (six on Windows: the lookup at
    // 0x42a4d0 divides by 6; Loki's port uses 9).
    class _TCellGrid {
    public:
        const TCell* getPCell(unsigned int x, unsigned int y) const;

    private:
        enum { s_kSegmentDim = 6 };
        typedef TArray<TArray<TCell, s_kSegmentDim>, s_kSegmentDim> _TSegment;

        unsigned int _m_widthInSegments;
        vector<TRefCountingPtr<_TSegment> > _m_aSegment;
    };

    TGameMap::TSize _m_size;
    TRefCountingPtr<_TCellGrid> _m_pCellGrid;
};

unsigned int TGameMap::TLayer::getWidth() const
{
    return _m_pImpl->getWidth();
}

unsigned int TGameMap::TLayer::getHeight() const
{
    return _m_pImpl->getHeight();
}

VA(0x0042a4d0, 0x4c)
const TGameMap::TLayer::TCell* TGameMap::TLayer::_TImpl::_TCellGrid::getPCell(unsigned int x,
                                                                             unsigned int y) const
{
    return &(*_m_aSegment[y / s_kSegmentDim * _m_widthInSegments + x / s_kSegmentDim])
        [y % s_kSegmentDim][x % s_kSegmentDim];
}

VA(0x0042b60f, 0x1b)
const TGameMap::TLayer::TCell* TGameMap::TLayer::getPCell(unsigned int x, unsigned int y) const
{
    return _m_pImpl->getPCell(x, y);
}

// The capped-type map's and info vector's template members, emitted here; the
// vector's fill insert folds with the RMG's TRmgMapPosition copy (0x430b35).
VA_COMPGEN(0x0042c28f, 0xa4, VECTOR_RESERVE, TCappedObjectTypeInfo)
VA_COMPGEN(0x0042db3f, 0xf0, TREE_INSERT, TCappedObjectTypeInfo)
VA_COMPGEN(0x00430930, 0x89, TREE_INIT, TCappedObjectTypeInfo)
