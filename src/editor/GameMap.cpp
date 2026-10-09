// GameMap.cpp - the map model (h3maped object from 0x41e5c5; Loki h3maped
// object 12). TGameMap and TGameMap::TLayer forward to their copy-on-write
// implementations, which this file defines.
//
// Ported so far: the first anonymous helper and the layer's cell lookup.
#include "editor/stdafx.h"

#include <assert.h>
#include <stdlib.h>
#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include "va.h"
#include "artifact.h"
#include "herotraits.h"
#include "editor/Array.h"
#include "editor/GameMap.h"
#include "editor/GameObject.h"
#include "editor/Hero.h"
#include "editor/Monster.h"
#include "editor/ObjectSpecializations.h"
#include "editor/Quest.h"
#include "editor/QuestLocation.h"
#include "editor/RawStream.h"
#include "editor/TilePoint.h"
#include "editor/TimedEvent.h"
#include "editor/Town.h"
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
                    if (x > 0 && !obj.getBCellPassable(x - 1, y))
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

    unsigned int getNumInfos() const { return _m_aInfo.size(); }

private:
    std::vector<TCappedObjectTypeInfo> _m_aInfo;
};

// Built on first use (h3maped 0x4226d8: guard 0x5aa6b3, object 0x5aa6c0).
inline const TCappedObjectTypeInfoMap& getCappedObjectTypeInfoMap()
{
    static const TCappedObjectTypeInfoMap s_map;
    return s_map;
}

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

// The first edition that has the object's type: Armageddon's Blade added
// the types from 165 and some subtypes (artifacts from 127, the eighth
// town, the Conflux dwellings, hero classes and monsters); Shadow of Death
// the types from 222, the combination artifacts from 129 and the new
// monoliths.
VA(0x0041ec1f, 0x79)
EGameVersion getRequiredVersion(const TObjectType& objType)
{
    if (objType.getType() >= CLOVER_FIELD_2)
        return GAME_VERSION_SOD;
    if (objType.getType() >= MAX_EVENT_TYPE)
        return GAME_VERSION_AB;
    switch (objType.getType()) {
    case ARTIFACT:
        if (objType.getExtra() >= 129)
            return GAME_VERSION_SOD;
        if (objType.getExtra() >= 127)
            return GAME_VERSION_AB;
        break;
    case CREATURE_GENERATOR_1:
        if (objType.getExtra() >= 59)
            return GAME_VERSION_AB;
        break;
    case HERO:
        if (objType.getExtra() >= 16)
            return GAME_VERSION_AB;
        break;
    case LITH_ONEWAY_ENTRANCE:
    case LITH_ONEWAY_EXIT:
        if (objType.getExtra() >= 3)
            return GAME_VERSION_SOD;
        break;
    case LITH_TWOWAY:
        if (objType.getExtra() >= 3)
            return GAME_VERSION_SOD;
        break;
    case MONSTER:
        if (objType.getExtra() >= 118)
            return GAME_VERSION_AB;
        break;
    case TOWN:
        if (objType.getExtra() >= 8)
            return GAME_VERSION_AB;
        break;
    }
    return GAME_VERSION_ROE;
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

// The kinds of object the map's conditions name; the reconstructions pass
// them to the layers' searches (isTown h3maped 0x41fe17, isHeroOrTown
// 0x4296c9, isMonster 0x4296a8).
inline bool isTown(const TGameObject& obj)
{
    return dynamic_cast<const TTown*>(&obj) != NULL;
}

inline bool isHero(const TGameObject& obj)
{
    return dynamic_cast<const TBasicHero*>(&obj) != NULL;
}

inline bool isMonster(const TGameObject& obj)
{
    return dynamic_cast<const TMonster*>(&obj) != NULL;
}

inline bool isHeroOrTown(const TGameObject& obj)
{
    return isHero(obj) || isTown(obj);
}

}  // namespace

// The object kinds the map keeps books on, the most derived first. Each
// link of a dispatch over the list tries one kind with a dynamic_cast and
// otherwise hands the object on with a null pointer to the rest of the
// list (h3maped's additions 0x434073..0x439dd4, removals 0x434989..0x439e19).
template <class THead, class TTail>
struct TObjectTypeList {
};

struct TNoObjectTypes {
};

typedef TObjectTypeList<THolyGrail,
        TObjectTypeList<TAbstractRandomlyAlignedGenerator,
        TObjectTypeList<TLinkableObject,
        TObjectTypeList<TQuestLocation, TNoObjectTypes> > > > TObjectTypesAfterTown;
typedef TObjectTypeList<TTown, TObjectTypesAfterTown> TObjectTypesFromTown;
typedef TObjectTypeList<THeroPlaceholder,
        TObjectTypeList<TRandomHero,
        TObjectTypeList<TNonRandomHero,
        TObjectTypeList<TPrison, TObjectTypesFromTown> > > > TBookkeptObjectTypes;

// The map's implementation: so far only the dimension of each size.
class TGameMap::_TImpl {
public:
    class _TGetVictoryConditionDataFunc;
    class _TGetLossConditionDataFunc;
    class _TVictoryConditionValidater;
    class _TLossConditionValidater;

    static void streamObject(streambuf* pStreamBuf, const TGameObject& obj);

    // The caps the failures report (h3maped 0x41ec98: 156 heroes; 0x41ecb8:
    // 48 towns).
    enum { s_kMaxHeroesOnMap = 156, s_kMaxTownsOnMap = 48, s_kMaxHeroesPerPlayer = 8 };

    static const unsigned int _s_akDimension[TGameMap::s_kNumSizes];

    _TImpl(TClient* pClient, EGameVersion version, TSize size, bool bTwoLayer);
    _TImpl(TClient* pClient, EGameVersion version, streambuf* pStreamBuf, int fileVersion);
    ~_TImpl();

    void importText(istream* pIStream);
    void exportText(ostream* pOStream) const;
    void save(streambuf* pStreamBuf) const;

    TLayer* getPLayer(unsigned int num) { return &_m_aLayer[num]; }
    TLayer* getPLayer(bool bSecondLayer);
    const TLayer* getPLayer(unsigned int num) const { return &_m_aLayer[num]; }
    const TLayer& getLayer(bool bSecondLayer) const { return _m_aLayer[bSecondLayer ? 1U : 0U]; }
    EGameVersion getVersion() const { return _m_version; }
    unsigned int getWidth() const { return _s_akDimension[_m_size]; }
    unsigned int getHeight() const { return _s_akDimension[_m_size]; }
    bool isTwoLayer() const { return _m_bTwoLayer; }
    const TGameObject* getPObject(bool bSecondLayer, unsigned int objID) const;
    const TGameObject* getPObject(const TMapObjectRef& objRef) const
    {
        return getPObject(objRef.getBSecondLayer(), objRef.getObjectID());
    }
    TTilePoint getObjectLoc(bool bSecondLayer, unsigned int objID) const
    {
        return getLayer(bSecondLayer).getObjectLoc(objID);
    }
    TMapObjectRef getLinkableObjectRef(int linkID) const { return _m_paLinkableObjectRef->find(linkID)->second; }

    const string& getName() const { return _m_pProperties->m_name; }
    void setName(const string& newName);
    const string& getDesc() const { return _m_pProperties->m_desc; }
    void setDesc(const string& newDesc);
    TDifficulty getDifficulty() const { return _m_pProperties->m_difficulty; }
    void setDifficulty(TDifficulty newDifficulty) { _m_pProperties->m_difficulty = newDifficulty; }
    unsigned int getMaxHeroLevel() const { return _m_pProperties->m_maxHeroLevel; }
    void setMaxHeroLevel(unsigned int newMaxHeroLevel) { _m_pProperties->m_maxHeroLevel = newMaxHeroLevel; }
    const TArray<TPlayerInfo, kNumPlayers>& getPlayers() const { return *_m_pProperties->m_paPlayer; }
    void setPlayers(const TArray<TPlayerInfo, kNumPlayers>& newPlayers);
    const TTeamInfo& getTeamInfo() const { return *_m_pProperties->m_pTeamInfo; }
    void setTeamInfo(const TTeamInfo& newTeamInfo);
    const vector<TRumor>& getRumors() const { return *_m_pProperties->m_paRumor; }
    void setRumors(const vector<TRumor>& newRumors);
    const vector<TTimedEvent>& getTimedEvents() const { return *_m_pProperties->m_paTimedEvent; }
    void setTimedEvents(const vector<TTimedEvent>& newTimedEvents);
    const TVictoryCondition* getPVictoryCondition() const { return _m_pProperties->m_pVictoryCondition.get(); }
    void setVictoryCondition(auto_ptr<TVictoryCondition> pNewVictoryCondition);
    const TLossCondition* getPLossCondition() const { return _m_pProperties->m_pLossCondition.get(); }
    void setLossCondition(auto_ptr<TLossCondition> pNewLossCondition);
    const bitset<kNumHeroes>& getDisabledHeroes() const { return _m_pProperties->m_disabledHeroes; }
    void setDisabledHeroes(const bitset<kNumHeroes>& newMask) { _m_pProperties->m_disabledHeroes = newMask; }
    const bitset<kNumArtifacts>& getDisabledArtifacts() const { return _m_pProperties->m_disabledArtifacts; }
    void setDisabledArtifacts(const bitset<kNumArtifacts>& newMask)
    {
        _m_pProperties->m_disabledArtifacts = newMask;
    }
    const bitset<kNumSpells>& getDisabledSpells() const { return _m_pProperties->m_disabledSpells; }
    void setDisabledSpells(const bitset<kNumSpells>& newMask) { _m_pProperties->m_disabledSpells = newMask; }
    const bitset<kNumSecSkills>& getDisabledSkills() const { return _m_pProperties->m_disabledSkills; }
    void setDisabledSkills(const bitset<kNumSecSkills>& newMask) { _m_pProperties->m_disabledSkills = newMask; }
    const THeroPrototype& getHeroPrototype(THeroID heroID) const { return (*_m_pProperties->m_aHeroPrototype)[heroID]; }
    void setHeroPrototype(THeroID heroID, const THeroPrototype& newPrototype);
    bool isPlayerPresent(TPlayer player) const { return _m_apPlayerBookkeeping[player]->m_numUnits > 0; }
    unsigned int getNumPlayableSlots() const { return _m_pBookkeeping->m_numPlayableSlots; }

    TMapLayerObjectID placeObject(bool bSecondLayer, auto_ptr<TGameObject> pObj, const TTilePoint& loc,
                                  TTileExtent* pUpdatedExtent);
    void removeObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent);
    TMapLayerObjectID insertObject(bool bSecondLayer, auto_ptr<TGameObject> pObj, const TTilePoint& loc);
    void eraseObject(bool bSecondLayer, unsigned int objID);
    void floatObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent);
    void unfloatObject(bool bSecondLayer, unsigned int x, unsigned int y, TTileExtent* pUpdatedExtent);
    void removeFloatingObject(bool bSecondLayer);
    bool onTerrainTypeChanged(bool bSecondLayer, unsigned int x, unsigned int y, TTerrainType oldTerrainType,
                              TTileExtent* pUpdatedExtent);
    void setPlaceholderHeroID(bool bSecondLayer, unsigned int objID, THeroID newHeroID);
    void clearPlaceholderHeroID(bool bSecondLayer, unsigned int objID);
    void setPlaceholderOwner(bool bSecondLayer, unsigned int objID, TPlayer newOwner);
    void setHeroID(bool bSecondLayer, unsigned int objID, THeroID newHeroID);
    void setHeroOwner(bool bSecondLayer, unsigned int objID, TPlayer newOwner);
    void setVisitingHero(const THero* pHero, bool bSecondLayer, unsigned int objID);
    void removeVisitingHero(bool bSecondLayer, unsigned int objID);
    void setTownOwner(TPlayer newOwner, bool bSecondLayer, unsigned int objID);
    void linkGeneratorToTown(const TMapObjectRef& generatorRef, const TMapObjectRef& townRef);
    void unlinkGenerator(const TMapObjectRef& generatorRef);
    void setQuest(const TMapObjectRef& questLocationRef, auto_ptr<TQuest> pQuest);
    void clearQuest(const TMapObjectRef& questLocationRef);
    void removeSecondLayer();
    void addSecondLayer();
    void setVersion(EGameVersion newVersion);
    const TLinkableObject* getPLinkableObject(int linkID) const;

    bitset<kNumHeroes> getHeroesOnMap() const { return _m_pBookkeeping->m_heroesOnMap; }
    bitset<kNumPlayers> getAvailableHeroOwnersMask() const;
    const set<TMapObjectRef>& getPlayerTownRefs(TPlayer player) const
    {
        return _m_apPlayerBookkeeping[player]->m_townRefs;
    }
    unsigned int getNumTownsOnMap() const { return _m_pBookkeeping->m_numTowns; }
    bool isGrailOnMap() const { return _m_pBookkeeping->m_bGrailPlaced; }
    unsigned int getNumObelisksOnMap() const;
    TPlayerInfo::TTownTypes getDefaultTownTypes(TPlayer player) const;

    bool isValidPlacement(const TGameObject& obj, bool bSecondLayer, unsigned int x, unsigned int y) const
    {
        return _isValidPlacement(getLayer(bSecondLayer), obj, x, y);
    }

    static bool _isValidPlacement(const TLayer& layer, const TGameObject& obj, unsigned int x, unsigned int y);
    static bool _isValidShipyardPlacement(const TLayer& layer, const TGameObject& shipyard, unsigned int x,
                                          unsigned int y);

    bool _isOnMap(const TGameObject& obj, unsigned int x, unsigned int y) const;
    void _removeObjectHelper(bool bSecondLayer, unsigned int objID);

    // Placement dispatches over the same kinds: each kind's checks may
    // adjust or refuse the new object before the layer takes it, and the
    // books follow; a failed placement deletes the object.
    template <class TObject, class TRest>
    TMapLayerObjectID _placeObject(bool bSecondLayer, TGameObject* pObj, const TTilePoint& loc,
                                   TTileExtent* pUpdatedExtent, TObjectTypeList<TObject, TRest>*)
    {
        TObject* pTypedObj = dynamic_cast<TObject*>(pObj);
        if (pTypedObj != NULL)
            return _placeTypedObject(bSecondLayer, pTypedObj, loc, pUpdatedExtent);
        return _placeObject(bSecondLayer, pObj, loc, pUpdatedExtent, static_cast<TRest*>(NULL));
    }
    TMapLayerObjectID _placeObject(bool bSecondLayer, TGameObject* pObj, const TTilePoint& loc,
                                   TTileExtent* pUpdatedExtent, TNoObjectTypes*)
    {
        return _placeTypedObject(bSecondLayer, pObj, loc, pUpdatedExtent);
    }
    template <class TObject>
    TMapLayerObjectID _placeTypedObject(bool bSecondLayer, TObject* pObj, const TTilePoint& loc,
                                        TTileExtent* pUpdatedExtent)
    {
        TMapLayerObjectID objID;
        try {
            objID = _placeNewObject(bSecondLayer, pObj, loc, pUpdatedExtent);
        } catch (...) {
            delete pObj;
            throw;
        }
        _onObjectAdded(dynamic_cast<const TObject*>(getLayer(bSecondLayer).getPObject(objID)), bSecondLayer, objID);
        return objID;
    }
    // A town brings its visiting hero along.
    TMapLayerObjectID _placeTypedObject(bool bSecondLayer, TTown* pTown, const TTilePoint& loc,
                                        TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeNewObject(bool bSecondLayer, TGameObject* pObj, const TTilePoint& loc,
                                      TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeNewObject(bool bSecondLayer, TBasicHero* pHero, const TTilePoint& loc,
                                      TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeNewObject(bool bSecondLayer, THeroPlaceholder* pPlaceholder, const TTilePoint& loc,
                                      TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeNewObject(bool bSecondLayer, TRandomHero* pHero, const TTilePoint& loc,
                                      TTileExtent* pUpdatedExtent)
    {
        _assignUniqueLinkID(pHero);
        return _placeNewObject(bSecondLayer, static_cast<TBasicHero*>(pHero), loc, pUpdatedExtent);
    }
    TMapLayerObjectID _placeNewObject(bool bSecondLayer, TNonRandomHero* pHero, const TTilePoint& loc,
                                      TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeNewObject(bool bSecondLayer, TPrison* pPrison, const TTilePoint& loc,
                                      TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeNewObject(bool bSecondLayer, TTown* pTown, const TTilePoint& loc,
                                      TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeNewObject(bool bSecondLayer, THolyGrail* pHolyGrail, const TTilePoint& loc,
                                      TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeNewObject(bool bSecondLayer, TAbstractRandomlyAlignedGenerator* pGenerator,
                                      const TTilePoint& loc, TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeNewObject(bool bSecondLayer, TLinkableObject* pLinkable, const TTilePoint& loc,
                                      TTileExtent* pUpdatedExtent)
    {
        _assignUniqueLinkID(pLinkable);
        return _placeNewObject(bSecondLayer, static_cast<TGameObject*>(pLinkable), loc, pUpdatedExtent);
    }
    TMapLayerObjectID _placeNewObject(bool bSecondLayer, TQuestLocation* pQuestLocation, const TTilePoint& loc,
                                      TTileExtent* pUpdatedExtent);
    void _assignUniqueLinkID(TLinkableObject* pLinkable) const;

    template <class TObject, class TRest>
    void _onObjectAdded(const TGameObject& obj, bool bSecondLayer, unsigned int objID,
                        TObjectTypeList<TObject, TRest>*)
    {
        const TObject* pObj = dynamic_cast<const TObject*>(&obj);
        if (pObj != NULL)
            _onObjectAdded(pObj, bSecondLayer, objID);
        else
            _onObjectAdded(obj, bSecondLayer, objID, static_cast<TRest*>(NULL));
    }
    // A town brings its visiting hero along.
    void _onObjectAdded(const TGameObject& obj, bool bSecondLayer, unsigned int objID, TObjectTypesFromTown*);
    void _onObjectAdded(const TGameObject& obj, bool bSecondLayer, unsigned int objID, TNoObjectTypes*)
    {
        _onGeneralObjectAdded(obj, bSecondLayer, objID);
    }
    void _onObjectAdded(const THeroPlaceholder* pPlaceholder, bool bSecondLayer, unsigned int objID)
    {
        _onHeroPlaceholderAdded(pPlaceholder, bSecondLayer, objID);
    }
    void _onObjectAdded(const TRandomHero* pHero, bool bSecondLayer, unsigned int objID)
    {
        _onRandomHeroAdded(pHero, bSecondLayer, objID);
    }
    void _onObjectAdded(const TIdentifiedHero* pHero, bool bSecondLayer, unsigned int objID)
    {
        _onIdentifiedHeroAdded(pHero, bSecondLayer, objID);
    }
    void _onObjectAdded(const THolyGrail* pHolyGrail, bool bSecondLayer, unsigned int objID)
    {
        _onHolyGrailAdded(pHolyGrail, bSecondLayer, objID);
    }
    void _onObjectAdded(const TLinkableObject* pLinkable, bool bSecondLayer, unsigned int objID)
    {
        _onLinkableAdded(pLinkable, bSecondLayer, objID);
    }
    void _onObjectAdded(const TGameObject* pObj, bool bSecondLayer, unsigned int objID)
    {
        _onGeneralObjectAdded(*pObj, bSecondLayer, objID);
    }

    template <class TObject, class TRest>
    void _onRemovingObject(const TGameObject& obj, TObjectTypeList<TObject, TRest>*)
    {
        const TObject* pObj = dynamic_cast<const TObject*>(&obj);
        if (pObj != NULL)
            _onRemovingObject(pObj);
        else
            _onRemovingObject(obj, static_cast<TRest*>(NULL));
    }
    // A town takes its visiting hero along.
    void _onRemovingObject(const TGameObject& obj, TObjectTypesFromTown*);
    void _onRemovingObject(const TGameObject& obj, TNoObjectTypes*) { _onRemovingGeneralObject(obj); }
    void _onRemovingObject(const THeroPlaceholder* pPlaceholder) { _onRemovingHeroPlaceholder(pPlaceholder); }
    void _onRemovingObject(const TRandomHero* pHero) { _onRemovingRandomHero(pHero); }
    void _onRemovingObject(const TIdentifiedHero* pHero) { _onRemovingIdentifiedHero(pHero); }
    void _onRemovingObject(const THolyGrail* pHolyGrail) { _onRemovingHolyGrail(pHolyGrail); }
    void _onRemovingObject(const TLinkableObject* pLinkable) { _onRemovingLinkable(pLinkable); }
    void _onRemovingObject(const TGameObject* pObj) { _onRemovingGeneralObject(*pObj); }

    void _onGeneralObjectAdded(const TGameObject& obj, bool bSecondLayer, unsigned int objID);
    void _onRemovingGeneralObject(const TGameObject& obj);
    void _registerLinkable(const TLinkableObject& linkable, bool bSecondLayer, unsigned int objID);
    void _onLinkableAdded(const TLinkableObject* pLinkable, bool bSecondLayer, unsigned int objID);
    void _unregisterLinkable(const TLinkableObject& linkable);
    void _onRemovingLinkable(const TLinkableObject* pLinkable);
    void _onPlayableAdded(const TPlayableObject* pPlayable, bool bSecondLayer, unsigned int objID);
    void _onRemovingPlayable(const TPlayableObject* pPlayable);
    void _onTownAdded(const TTown* pTown, bool bSecondLayer, unsigned int objID);
    void _onRemovingTown(const TTown* pTown);
    void _onBasicHeroAdded(const TBasicHero* pHero, bool bSecondLayer, unsigned int objID);
    void _onRemovingBasicHero(const TBasicHero* pHero);
    void _onHeroPlaceholderAdded(const THeroPlaceholder* pPlaceholder, bool bSecondLayer, unsigned int objID);
    void _onRemovingHeroPlaceholder(const THeroPlaceholder* pPlaceholder);
    void _onRandomHeroAdded(const TRandomHero* pHero, bool bSecondLayer, unsigned int objID);
    void _onRemovingRandomHero(const TRandomHero* pHero);
    void _onIdentifiedHeroAdded(const TIdentifiedHero* pHero, bool bSecondLayer, unsigned int objID);
    void _onRemovingIdentifiedHero(const TIdentifiedHero* pHero);
    void _onHolyGrailAdded(const THolyGrail* pHolyGrail, bool bSecondLayer, unsigned int objID);
    void _onRemovingHolyGrail(const THolyGrail* pHolyGrail);
    bool _isPlayable() const;
    bool _isHeroAvailable(THeroID heroID) const;
    THeroID _pickRandomHero(THeroClass heroClass) const;
    unsigned int _pickAvailableTeam() const;
    void _makeLinkIDUnique(TLinkableObject& linkable) const;
    const TLinkableObject* _findLinkableObject(unsigned int linkID) const;
    bool _isRandomTownLink(unsigned int linkID) const;

    void _getObjectLoc(const TMapObjectRef& objRef, TMapLoc* pLoc) const;
    void _readHeroSettings(TRawIStream* pIStream, int version);
    void _writeHeroSettings(TRawOStream* pOStream, int version) const;
    TMapObjectRef _findObject(const TMapLoc& loc, bool (*pfnPredicate)(const TGameObject&)) const;
    auto_ptr<TVictoryCondition> _reconstructVictoryCondition(const TVictoryConditionData& vcData) const;
    auto_ptr<TLossCondition> _reconstructLossCondition(const TLossConditionData& lcData) const;

    void onObjectRemoved();

private:
    // The map's specifications (0x7c bytes; h3maped 0x43334a constructs
    // them): each container is a handle of its own, so the dialogs' edits
    // copy only what they change.
    struct _TProperties {
        _TProperties();

        string m_name;
        string m_desc;
        TDifficulty m_difficulty;
        unsigned int m_maxHeroLevel;
        TRefCountingPtr<TArray<TPlayerInfo, kNumPlayers> > m_paPlayer;
        TRefCountingPtr<TTeamInfo> m_pTeamInfo;
        TRefCountingPtr<vector<TRumor> > m_paRumor;
        TRefCountingPtr<vector<TTimedEvent> > m_paTimedEvent;
        TRefCountingAutoPtr<TVictoryCondition> m_pVictoryCondition;
        TRefCountingAutoPtr<TLossCondition> m_pLossCondition;
        bitset<kNumHeroes> m_disabledHeroes;
        bitset<kNumArtifacts> m_disabledArtifacts;
        bitset<kNumSpells> m_disabledSpells;
        bitset<kNumSecSkills> m_disabledSkills;
        TRefCountingPtr<TArray<THeroPrototype, kNumHeroes> > m_aHeroPrototype;
    };

    // The counts behind the caps (0x38 bytes; 0x4338c8).
    struct _TBookkeeping {
        _TBookkeeping();

        unsigned int m_numPlayableSlots;
        unsigned int m_numTowns;
        unsigned int m_numHeroes;
        bool m_bGrailPlaced;
        vector<unsigned int> m_aNumObjsOfCappedType;
        bitset<kNumHeroes> m_heroesOnMap;
        TRefCountingPtr<map<THeroID, TMapObjectRef> > m_paHeroRef;
    };

    // One player's counts (0x74 bytes; 0x438696).
    struct _TPlayerBookkeeping {
        _TPlayerBookkeeping();

        unsigned int m_numUnits;
        set<TMapObjectRef> m_townRefs;
        unsigned int m_numRandomTowns;
        TArray<unsigned int, kNumTownTypes> m_aNumTownsOfType;
        unsigned int m_numHeroes;
        unsigned int m_numRandomHeroes;
        unsigned int m_numHeroPlaceholders;
        TRefCountingPtr<set<THeroID> > m_paHeroID;
        TRefCountingPtr<set<THeroID> > m_paHeroPlaceholderID;
        TArray<unsigned int, kNumTownTypes> m_aNumHeroesOfType;
    };

    TClient* _m_pClient;
    EGameVersion _m_version;
    TSize _m_size;
    bool _m_bTwoLayer;
    TRefCountingPtr<_TProperties> _m_pProperties;
    vector<TLayer> _m_aLayer;
    TRefCountingPtr<_TBookkeeping> _m_pBookkeeping;
    TArray<TRefCountingPtr<_TPlayerBookkeeping>, kNumPlayers> _m_apPlayerBookkeeping;
    TRefCountingPtr<map<unsigned int, TMapObjectRef> > _m_paLinkableObjectRef;
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

// Whether a condition still names objects of the right kinds; a missing
// condition is valid. The two validaters' isValid bodies fold (0x41ef90).
class TGameMap::_TImpl::_TVictoryConditionValidater : public TVictoryCondition::TVisitor {
public:
    _TVictoryConditionValidater(const _TImpl& map) : _m_map(map), _m_bValid(false) {}

    bool isValid(const TVictoryCondition* pVC);

    virtual void visit(const TVCAquireArtifact& vc) { _m_bValid = true; }
    virtual void visit(const TVCAccumulateCreature& vc) { _m_bValid = true; }
    virtual void visit(const TVCAccumulateResource& vc) { _m_bValid = true; }
    virtual void visit(const TVCUpgradeTown& vc);
    virtual void visit(const TVCBuildHolyGrailStruct& vc);
    virtual void visit(const TVCDefeatHero& vc);
    virtual void visit(const TVCCaptureTown& vc);
    virtual void visit(const TVCDefeatMonster& vc);
    virtual void visit(const TVCFlagAllCreatureGenerators& vc) { _m_bValid = true; }
    virtual void visit(const TVCFlagAllMines& vc) { _m_bValid = true; }
    virtual void visit(const TVCTransportArtifact& vc);

private:
    bool _isTown(const TMapObjectRef& objRef) const;

    const _TImpl& _m_map;
    bool _m_bValid;
};

class TGameMap::_TImpl::_TLossConditionValidater : public TLossCondition::TVisitor {
public:
    _TLossConditionValidater(const _TImpl& map) : _m_map(map), _m_bValid(false) {}

    bool isValid(const TLossCondition* pLC);

    virtual void visit(const TLCLoseTown& lc);
    virtual void visit(const TLCLoseHero& lc);
    virtual void visit(const TLCTimeExpires& lc) { _m_bValid = true; }

private:
    const _TImpl& _m_map;
    bool _m_bValid;
};

VA(0x0041ef90, 0x20)
bool TGameMap::_TImpl::_TVictoryConditionValidater::isValid(const TVictoryCondition* pVC)
{
    if (pVC == NULL)
        return true;
    pVC->accept(this);
    bool bValid = _m_bValid;
    _m_bValid = false;
    return bValid;
}

bool TGameMap::_TImpl::_TLossConditionValidater::isValid(const TLossCondition* pLC)
{
    if (pLC == NULL)
        return true;
    pLC->accept(this);
    bool bValid = _m_bValid;
    _m_bValid = false;
    return bValid;
}

void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCUpgradeTown& vc)
{
    _m_bValid = _isTown(vc.getTownRef());
}

VA(0x0041efb0, 0x2f)
void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCBuildHolyGrailStruct& vc)
{
    _m_bValid = vc.getTownRef() == TMapObjectRef() || _isTown(vc.getTownRef());
}

VA(0x0041efdf, 0x7f)
void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCDefeatHero& vc)
{
    const TGameObject* pObj = _m_map.getPObject(vc.getHeroRef());
    if (pObj != NULL) {
        if (dynamic_cast<const TBasicHero*>(pObj) != NULL) {
            _m_bValid = true;
        } else {
            const TTown* pTown = dynamic_cast<const TTown*>(pObj);
            _m_bValid = pTown != NULL && pTown->getPVisitingHero() != NULL;
        }
    } else {
        _m_bValid = false;
    }
}

VA(0x0041f05e, 0x17)
void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCCaptureTown& vc)
{
    _m_bValid = _isTown(vc.getTownRef());
}

VA(0x0041f075, 0x4c)
void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCDefeatMonster& vc)
{
    const TGameObject* pObj = _m_map.getPObject(vc.getMonsterRef());
    _m_bValid = pObj != NULL && dynamic_cast<const TMonster*>(pObj) != NULL;
}

VA(0x0041f0c8, 0x17)
void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCTransportArtifact& vc)
{
    _m_bValid = _isTown(vc.getTownRef());
}

VA(0x0041f0df, 0x44)
bool TGameMap::_TImpl::_TVictoryConditionValidater::_isTown(const TMapObjectRef& objRef) const
{
    const TGameObject* pObj = _m_map.getPObject(objRef);
    return pObj != NULL && dynamic_cast<const TTown*>(pObj) != NULL;
}

VA(0x0041f123, 0x4c)
void TGameMap::_TImpl::_TLossConditionValidater::visit(const TLCLoseTown& lc)
{
    const TGameObject* pObj = _m_map.getPObject(lc.getTownRef());
    _m_bValid = pObj != NULL && dynamic_cast<const TTown*>(pObj) != NULL;
}

VA(0x0041f16f, 0x7f)
void TGameMap::_TImpl::_TLossConditionValidater::visit(const TLCLoseHero& lc)
{
    const TGameObject* pObj = _m_map.getPObject(lc.getHeroRef());
    if (pObj != NULL) {
        if (dynamic_cast<const TBasicHero*>(pObj) != NULL) {
            _m_bValid = true;
        } else {
            const TTown* pTown = dynamic_cast<const TTown*>(pObj);
            _m_bValid = pTown != NULL && pTown->getPVisitingHero() != NULL;
        }
    } else {
        _m_bValid = false;
    }
}

VA(0x0041f1ee, 0x2f)
void TGameMap::_TImpl::streamObject(streambuf* pStreamBuf, const TGameObject& obj)
{
    TRawOStream stream(pStreamBuf);
    stream << obj.getObjectType();
    obj.write(&stream, 2);
}

// A new map: one or two layers of plain water, and every hero the edition
// does not offer, or that only campaigns use, disabled.
VA(0x0041f21d, 0x133)
TGameMap::_TImpl::_TImpl(TClient* pClient, EGameVersion version, TSize size, bool bTwoLayer)
    : _m_pClient(pClient), _m_version(version), _m_size(size), _m_bTwoLayer(bTwoLayer)
{
    _m_aLayer.resize(_m_bTwoLayer ? 2 : 1, TLayer(_m_size));
    int edition = _m_version >= GAME_VERSION_AB;
    for (THeroID heroID = 0; heroID < kNumHeroes; heroID++) {
        const THeroTraits& traits = akHeroTraits[heroID];
        _m_pProperties->m_disabledHeroes.set(heroID, traits.m_availability.m_special
                                                     || !traits.m_abAvailableIn[edition]);
    }
}

VA(0x0041fee1, 0x5d)
TGameMap::_TImpl::~_TImpl()
{
}

VA(0x0041ff3e, 0x2d)
void TGameMap::_TImpl::setName(const string& newName)
{
    _m_pProperties->m_name = newName;
}

VA(0x0041ff6b, 0x2d)
void TGameMap::_TImpl::setDesc(const string& newDesc)
{
    _m_pProperties->m_desc = newDesc;
}

VA(0x0041ff98, 0x5a)
void TGameMap::_TImpl::setPlayers(const TArray<TPlayerInfo, kNumPlayers>& newPlayers)
{
    *_m_pProperties->m_paPlayer = newPlayers;
}

VA(0x0041fff2, 0x38)
void TGameMap::_TImpl::setTeamInfo(const TTeamInfo& newTeamInfo)
{
    *_m_pProperties->m_pTeamInfo = newTeamInfo;
}

VA(0x00420062, 0x38)
void TGameMap::_TImpl::setRumors(const vector<TRumor>& newRumors)
{
    *_m_pProperties->m_paRumor = newRumors;
}

VA(0x0042009a, 0x38)
void TGameMap::_TImpl::setTimedEvents(const vector<TTimedEvent>& newTimedEvents)
{
    *_m_pProperties->m_paTimedEvent = newTimedEvents;
}

VA(0x004200d2, 0x7b)
void TGameMap::_TImpl::setVictoryCondition(auto_ptr<TVictoryCondition> pNewVictoryCondition)
{
    TRefCountingAutoPtr<TVictoryCondition> pVictoryCondition(pNewVictoryCondition);
    _m_pProperties->m_pVictoryCondition = pVictoryCondition;
}

VA(0x0042014d, 0x7b)
void TGameMap::_TImpl::setLossCondition(auto_ptr<TLossCondition> pNewLossCondition)
{
    TRefCountingAutoPtr<TLossCondition> pLossCondition(pNewLossCondition);
    _m_pProperties->m_pLossCondition = pLossCondition;
}

VA(0x004201c8, 0x5a)
void TGameMap::_TImpl::setHeroPrototype(THeroID heroID, const THeroPrototype& newPrototype)
{
    if (!(newPrototype == getHeroPrototype(heroID)))
        (*_m_pProperties->m_aHeroPrototype)[heroID] = newPrototype;
}

// The placement dispatch takes the object over.
VA(0x00420222, 0x79)
TMapLayerObjectID TGameMap::_TImpl::placeObject(bool bSecondLayer, auto_ptr<TGameObject> pObj, const TTilePoint& loc,
                                               TTileExtent* pUpdatedExtent)
{
    return _placeObject(bSecondLayer, pObj.release(), loc, pUpdatedExtent, static_cast<TBookkeptObjectTypes*>(NULL));
}

VA(0x0042029b, 0x45)
void TGameMap::_TImpl::removeObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent)
{
    TLayer* pLayer = getPLayer(bSecondLayer);
    *pUpdatedExtent = pLayer->getObjectExtent(objID);
    _removeObjectHelper(bSecondLayer, objID);
}

VA(0x004202e0, 0x58)
TMapLayerObjectID TGameMap::_TImpl::insertObject(bool bSecondLayer, auto_ptr<TGameObject> pObj, const TTilePoint& loc)
{
    return getPLayer(bSecondLayer)->_placeObject(pObj, loc);
}

VA(0x00420338, 0x12)
TGameMap::TLayer* TGameMap::_TImpl::getPLayer(bool bSecondLayer)
{
    return &_m_aLayer[bSecondLayer ? 1U : 0U];
}

VA(0x0042034a, 0x31)
void TGameMap::_TImpl::eraseObject(bool bSecondLayer, unsigned int objID)
{
    getPLayer(bSecondLayer)->_removeObject(objID);
}

VA(0x0042037b, 0x53)
void TGameMap::_TImpl::floatObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent)
{
    TLayer* pLayer = getPLayer(bSecondLayer);
    *pUpdatedExtent = pLayer->getObjectExtent(objID);
    pLayer->_floatObject(objID);
}

VA(0x004203ce, 0x11b)
void TGameMap::_TImpl::unfloatObject(bool bSecondLayer, unsigned int x, unsigned int y, TTileExtent* pUpdatedExtent)
{
    TLayer* pLayer = getPLayer(bSecondLayer);
    TMapLayerObjectID objID = pLayer->getFloatingObjID();
    const TGameObject& obj = pLayer->getObject(objID);
    if (!_isOnMap(obj, x, y))
        throw TPlaceObjFailurePlacementNotOnMap();
    if (!(x >= 9 && y >= 9 && x < getWidth() - 9 && y < getHeight() - 9)) {
        const THolyGrail* pHolyGrail = dynamic_cast<const THolyGrail*>(&obj);
        if (pHolyGrail != NULL)
            throw TPlaceObjFailureHolyGrailTooCloseToEdge();
    }
    if (!_isValidPlacement(*pLayer, obj, x, y))
        throw TPlaceObjFailureInvalidPlacement();
    pLayer->_unfloatObject(TTilePoint(x, y));
    *pUpdatedExtent = pLayer->getObjectExtent(objID);
}

VA(0x0042059b, 0x1e)
void TGameMap::_TImpl::removeFloatingObject(bool bSecondLayer)
{
    _removeObjectHelper(bSecondLayer, getPLayer(bSecondLayer)->getFloatingObjID());
}

// A cell's new terrain removes the objects whose blocking cells (or an
// underlay's cells) it no longer allows, and a shipyard left without water.
VA(0x004205b9, 0x34e)
bool TGameMap::_TImpl::onTerrainTypeChanged(bool bSecondLayer, unsigned int x, unsigned int y,
                                            TTerrainType oldTerrainType, TTileExtent* pUpdatedExtent)
{
    bool bObjectsRemoved = false;
    const TLayer& layer = getLayer(bSecondLayer);
    TTerrainType newTerrainType = layer.getCell(x, y).getTerrainType();
    unsigned int numObjs = layer.getNumObjectIDsAtCell(x, y);
    static vector<int> aObjID;
    aObjID.clear();
    aObjID.reserve(numObjs);
    for (unsigned int i = 0; i < numObjs; i++)
        aObjID.push_back(layer.getObjectIDAtCell(x, y, i));
    for (vector<int>::const_iterator pObjID = aObjID.begin(); pObjID != aObjID.end(); ++pObjID) {
        const TGameObject& obj = layer.getObject(*pObjID);
        if (!obj.getTerrainMask()[newTerrainType]) {
            TTilePoint objLoc = layer.getObjectLoc(*pObjID);
            unsigned int i = objLoc.x() - x;
            unsigned int j = objLoc.y() - y;
            if (obj.getBUnderlay() ? obj.getBCellPlaced(i, j) : !obj.getBCellPassable(i, j)) {
                TTileExtent removedExtent;
                removeObject(bSecondLayer, *pObjID, &removedExtent);
                if (!bObjectsRemoved) {
                    *pUpdatedExtent = removedExtent;
                    bObjectsRemoved = true;
                } else {
                    *pUpdatedExtent |= removedExtent;
                }
            }
        }
    }
    if (oldTerrainType == eTerrainWater) {
        bool abAdjacent[8];
        computeAdjacentDirs(layer.getWidth(), layer.getHeight(), x, y, abAdjacent);
        for (unsigned int dir = 0; dir < 8; dir++) {
            if (!abAdjacent[dir])
                continue;
            TTilePoint adjLoc = TPoint<int>(x, y) + akAdjOffset[dir];
            if (layer.getCell(adjLoc).getTerrainType() != eTerrainWater) {
                unsigned int numAdjObjs = layer.getNumObjectIDsAtCell(adjLoc.x(), adjLoc.y());
                static vector<int> aAdjObjID;
                aAdjObjID.clear();
                aAdjObjID.reserve(numAdjObjs);
                for (unsigned int i = 0; i < numAdjObjs; i++)
                    aAdjObjID.push_back(layer.getObjectIDAtCell(adjLoc.x(), adjLoc.y(), i));
                for (vector<int>::const_iterator pObjID = aAdjObjID.begin(); pObjID != aAdjObjID.end();
                     ++pObjID) {
                    const TGameObject& obj = layer.getObject(*pObjID);
                    if (obj.getType() == SHIPYARD) {
                        TTilePoint objLoc = layer.getObjectLoc(*pObjID);
                        if (!_isValidShipyardPlacement(layer, obj, objLoc.x(), objLoc.y())) {
                            TTileExtent removedExtent;
                            removeObject(bSecondLayer, *pObjID, &removedExtent);
                            if (!bObjectsRemoved) {
                                *pUpdatedExtent = removedExtent;
                                bObjectsRemoved = true;
                            } else {
                                *pUpdatedExtent |= removedExtent;
                            }
                        }
                    }
                }
            }
        }
    }
    return bObjectsRemoved;
}

// A placeholder's hero leaves the books before the new one enters.
VA(0x0042091b, 0x150)
void TGameMap::_TImpl::setPlaceholderHeroID(bool bSecondLayer, unsigned int objID, THeroID newHeroID)
{
    THeroPlaceholder* pPlaceholder = dynamic_cast<THeroPlaceholder*>(getPLayer(bSecondLayer)->getPObject(objID));
    _TPlayerBookkeeping& playerBookkeeping = *_m_apPlayerBookkeeping[pPlaceholder->getOwner()];
    if (pPlaceholder->getHeroID() != -1) {
        THeroID heroID = pPlaceholder->getHeroID();
        playerBookkeeping.m_aNumHeroesOfType[THero::s_akClassTraits[THero::s_akTraits[heroID].m_class].m_townType]--;
        playerBookkeeping.m_paHeroPlaceholderID->erase(heroID);
        _m_pBookkeeping->m_heroesOnMap.set(heroID, false);
    } else {
        playerBookkeeping.m_numHeroPlaceholders--;
    }
    pPlaceholder->setHeroID(newHeroID);
    _m_pBookkeeping->m_heroesOnMap.set(newHeroID, true);
    playerBookkeeping.m_paHeroPlaceholderID->insert(newHeroID);
    playerBookkeeping.m_aNumHeroesOfType[THero::s_akClassTraits[THero::s_akTraits[newHeroID].m_class].m_townType]++;
}

VA(0x00420a6b, 0xf7)
void TGameMap::_TImpl::clearPlaceholderHeroID(bool bSecondLayer, unsigned int objID)
{
    THeroPlaceholder* pPlaceholder = dynamic_cast<THeroPlaceholder*>(getPLayer(bSecondLayer)->getPObject(objID));
    _TPlayerBookkeeping& playerBookkeeping = *_m_apPlayerBookkeeping[pPlaceholder->getOwner()];
    THeroID heroID = pPlaceholder->getHeroID();
    THeroClass heroClass = THero::s_akTraits[heroID].m_class;
    _m_apPlayerBookkeeping[pPlaceholder->getOwner()]
        ->m_aNumHeroesOfType[THero::s_akClassTraits[heroClass].m_townType]--;
    playerBookkeeping.m_paHeroPlaceholderID->erase(heroID);
    _m_pBookkeeping->m_heroesOnMap.set(heroID, false);
    pPlaceholder->setHeroID(-1);
}

VA(0x00420b62, 0x6c)
void TGameMap::_TImpl::setPlaceholderOwner(bool bSecondLayer, unsigned int objID, TPlayer newOwner)
{
    THeroPlaceholder* pPlaceholder = dynamic_cast<THeroPlaceholder*>(getPLayer(bSecondLayer)->getPObject(objID));
    _onRemovingHeroPlaceholder(pPlaceholder);
    pPlaceholder->setOwner(newOwner);
    _onHeroPlaceholderAdded(pPlaceholder, bSecondLayer, objID);
}

// The hero may stand on the map or visit a town there.
VA(0x00420bce, 0x1fc)
void TGameMap::_TImpl::setHeroID(bool bSecondLayer, unsigned int objID, THeroID newHeroID)
{
    TIdentifiedHero* pHero = dynamic_cast<TIdentifiedHero*>(getPLayer(bSecondLayer)->getPObject(objID));
    if (pHero == NULL)
        pHero = static_cast<TIdentifiedHero*>(
            dynamic_cast<TTown*>(getPLayer(bSecondLayer)->getPObject(objID))->getPVisitingHero());
    THeroID oldHeroID = pHero->getHeroID();
    if (pHero->getOwner() != ePlayerNone) {
        _TPlayerBookkeeping& playerBookkeeping = *_m_apPlayerBookkeeping[pHero->getOwner()];
        playerBookkeeping.m_aNumHeroesOfType[THero::s_akClassTraits[THero::s_akTraits[oldHeroID].m_class].m_townType]--;
        playerBookkeeping.m_paHeroID->erase(oldHeroID);
    }
    _m_pBookkeeping->m_paHeroRef->erase(oldHeroID);
    _m_pBookkeeping->m_heroesOnMap.set(oldHeroID, false);
    pHero->setHeroID(newHeroID);
    _m_pBookkeeping->m_heroesOnMap.set(newHeroID, true);
    _m_pBookkeeping->m_paHeroRef->insert(map<THeroID, TMapObjectRef>::value_type(newHeroID,
                                                                                 TMapObjectRef(bSecondLayer, objID)));
    if (pHero->getOwner() != ePlayerNone) {
        _TPlayerBookkeeping& playerBookkeeping = *_m_apPlayerBookkeeping[pHero->getOwner()];
        playerBookkeeping.m_paHeroID->insert(newHeroID);
        playerBookkeeping.m_aNumHeroesOfType[THero::s_akClassTraits[THero::s_akTraits[newHeroID].m_class].m_townType]++;
    }
}

VA(0x00420dca, 0xa1)
void TGameMap::_TImpl::setHeroOwner(bool bSecondLayer, unsigned int objID, TPlayer newOwner)
{
    THero* pHero = dynamic_cast<THero*>(getPLayer(bSecondLayer)->getPObject(objID));
    TRandomHero* pRandomHero = dynamic_cast<TRandomHero*>(pHero);
    if (pRandomHero != NULL)
        _onRemovingRandomHero(pRandomHero);
    else
        _onRemovingIdentifiedHero(static_cast<TIdentifiedHero*>(pHero));
    pHero->setOwner(newOwner);
    if (pRandomHero != NULL)
        _onRandomHeroAdded(pRandomHero, bSecondLayer, objID);
    else
        _onIdentifiedHeroAdded(static_cast<TIdentifiedHero*>(pHero), bSecondLayer, objID);
}

// The town keeps its own copy of the hero.
VA(0x00420e6b, 0x98)
void TGameMap::_TImpl::setVisitingHero(const THero* pHero, bool bSecondLayer, unsigned int objID)
{
    TTown* pTown = dynamic_cast<TTown*>(getPLayer(bSecondLayer)->getPObject(objID));
    pTown->setVisitingHero(pHero);
    TRandomHero* pRandomHero = dynamic_cast<TRandomHero*>(pTown->getPVisitingHero());
    if (pRandomHero != NULL)
        _onRandomHeroAdded(pRandomHero, bSecondLayer, objID);
    else
        _onIdentifiedHeroAdded(static_cast<TIdentifiedHero*>(pTown->getPVisitingHero()), bSecondLayer, objID);
}

VA(0x00420f03, 0x92)
void TGameMap::_TImpl::removeVisitingHero(bool bSecondLayer, unsigned int objID)
{
    TTown* pTown = dynamic_cast<TTown*>(getPLayer(bSecondLayer)->getPObject(objID));
    TRandomHero* pRandomHero = dynamic_cast<TRandomHero*>(pTown->getPVisitingHero());
    if (pRandomHero != NULL)
        _onRemovingRandomHero(pRandomHero);
    else
        _onRemovingIdentifiedHero(static_cast<TIdentifiedHero*>(pTown->getPVisitingHero()));
    pTown->setVisitingHero(NULL);
    onObjectRemoved();
}

// The visiting hero changes owner with the town.
VA(0x00420f95, 0xe7)
void TGameMap::_TImpl::setTownOwner(TPlayer newOwner, bool bSecondLayer, unsigned int objID)
{
    TTown* pTown = dynamic_cast<TTown*>(getPLayer(bSecondLayer)->getPObject(objID));
    THero* pVisitingHero = pTown->getPVisitingHero();
    if (pVisitingHero != NULL) {
        TRandomHero* pRandomHero = dynamic_cast<TRandomHero*>(pVisitingHero);
        if (pRandomHero != NULL)
            _onRemovingRandomHero(pRandomHero);
        else
            _onRemovingIdentifiedHero(static_cast<TIdentifiedHero*>(pVisitingHero));
    }
    _onRemovingTown(pTown);
    pTown->setOwner(newOwner);
    if (pVisitingHero != NULL)
        pVisitingHero->setOwner(newOwner);
    _onTownAdded(pTown, bSecondLayer, objID);
    if (pVisitingHero != NULL) {
        TRandomHero* pRandomHero = dynamic_cast<TRandomHero*>(pVisitingHero);
        if (pRandomHero != NULL)
            _onRandomHeroAdded(pRandomHero, bSecondLayer, objID);
        else
            _onIdentifiedHeroAdded(static_cast<TIdentifiedHero*>(pVisitingHero), bSecondLayer, objID);
    }
}

VA(0x0042107c, 0xa6)
void TGameMap::_TImpl::linkGeneratorToTown(const TMapObjectRef& generatorRef, const TMapObjectRef& townRef)
{
    TAbstractRandomlyAlignedGenerator* pGenerator = dynamic_cast<TAbstractRandomlyAlignedGenerator*>(
        getPLayer(generatorRef.getBSecondLayer())->getPObject(generatorRef.getObjectID()));
    TTown* pTown = dynamic_cast<TTown*>(getPLayer(townRef.getBSecondLayer())->getPObject(townRef.getObjectID()));
    pGenerator->setTownLinkID(pTown->getLinkID());
}

VA(0x00421122, 0x55)
void TGameMap::_TImpl::unlinkGenerator(const TMapObjectRef& generatorRef)
{
    TAbstractRandomlyAlignedGenerator* pGenerator = dynamic_cast<TAbstractRandomlyAlignedGenerator*>(
        getPLayer(generatorRef.getBSecondLayer())->getPObject(generatorRef.getObjectID()));
    pGenerator->setTownLinkID(TLinkableObject::s_kNoLinkID);
}

VA(0x00421177, 0x8f)
void TGameMap::_TImpl::setQuest(const TMapObjectRef& questLocationRef, auto_ptr<TQuest> pQuest)
{
    TQuestLocation* pQuestLocation = dynamic_cast<TQuestLocation*>(
        getPLayer(questLocationRef.getBSecondLayer())->getPObject(questLocationRef.getObjectID()));
    pQuestLocation->setQuest(pQuest);
}

// A linked quest whose target leaves the map goes with it (0x4278a6).
VA(0x00421206, 0x53)
void TGameMap::_TImpl::clearQuest(const TMapObjectRef& questLocationRef)
{
    TQuestLocation* pQuestLocation = dynamic_cast<TQuestLocation*>(
        getPLayer(questLocationRef.getBSecondLayer())->getPObject(questLocationRef.getObjectID()));
    pQuestLocation->clearQuest();
}

VA(0x00421259, 0x64)
void TGameMap::_TImpl::removeSecondLayer()
{
    TLayer* pLayer = getPLayer(true);
    TLayer::TObjectIDIter iter = pLayer->objectIDBegin();
    while (iter != pLayer->objectIDEnd()) {
        TMapLayerObjectID objID = *iter++;
        _removeObjectHelper(true, objID);
    }
    _m_aLayer.pop_back();
    _m_bTwoLayer = false;
}

VA(0x004212d6, 0x43)
void TGameMap::_TImpl::addSecondLayer()
{
    _m_bTwoLayer = true;
    _m_aLayer.push_back(TLayer(_m_size));
}

// One edition at a time. Armageddon's Blade enables the heroes Restoration
// lacked, moves hero 4 to its new id 144 and disables the heroes it lacks.
VA(0x00421319, 0x1d1)
void TGameMap::_TImpl::setVersion(EGameVersion newVersion)
{
    if (newVersion - _m_version > 1)
        setVersion(EGameVersion(newVersion - 1));
    EGameVersion oldVersion = _m_version;
    if (oldVersion == GAME_VERSION_ROE && newVersion == GAME_VERSION_AB) {
        bitset<kNumHeroes>& disabledHeroes = _m_pProperties->m_disabledHeroes;
        for (unsigned int heroID = 0; heroID < kNumHeroes; heroID++) {
            if (!akHeroTraits[heroID].m_availability.m_availableInOriginal
                && !akHeroTraits[heroID].m_availability.m_special)
                disabledHeroes.set(heroID, false);
        }
    }
    _m_version = newVersion;
    if (oldVersion == GAME_VERSION_ROE && newVersion == GAME_VERSION_AB) {
        unsigned int numLayers = _m_bTwoLayer ? 2 : 1;
        for (unsigned int layerNum = 0; layerNum < numLayers; layerNum++) {
            TLayer* pLayer = getPLayer(layerNum);
            for (TLayer::TObjectIDIter iter = pLayer->objectIDBegin(); iter != pLayer->objectIDEnd(); ++iter) {
                TGameObject* pObj = pLayer->getPObject(*iter);
                TIdentifiedHero* pHero = dynamic_cast<TIdentifiedHero*>(pObj);
                if (pHero == NULL) {
                    TTown* pTown = dynamic_cast<TTown*>(pObj);
                    if (pTown != NULL && pTown->getPVisitingHero() != NULL)
                        pHero = dynamic_cast<TIdentifiedHero*>(pTown->getPVisitingHero());
                }
                if (pHero != NULL && pHero->getHeroID() == 4) {
                    setHeroID(layerNum != 0, *iter, 144);
                    goto heroMoved;
                }
            }
        }
    heroMoved:
        bitset<kNumHeroes>& disabledHeroes = _m_pProperties->m_disabledHeroes;
        for (unsigned int heroID = 0; heroID < kNumHeroes; heroID++) {
            if (!akHeroTraits[heroID].m_availability.m_availableInExpansion)
                disabledHeroes.set(heroID, true);
        }
    }
}

VA(0x004214ea, 0x39)
const TGameObject* TGameMap::_TImpl::getPObject(bool bSecondLayer, unsigned int objID) const
{
    const TLayer& layer = getLayer(bSecondLayer);
    if (!layer.isObjectIDValid(objID))
        return NULL;
    return layer.getPObject(objID);
}

// The object a link id names: the map keeps where each linkable object
// lies, and a town holds its visiting hero's id as well.
VA(0x00421523, 0x69)
const TLinkableObject* TGameMap::_TImpl::getPLinkableObject(int linkID) const
{
    TMapObjectRef objRef = _m_paLinkableObjectRef->find(linkID)->second;
    const TLinkableObject* pLinkable = dynamic_cast<const TLinkableObject*>(
        &getLayer(objRef.getBSecondLayer()).getObject(objRef.getObjectID()));
    while (pLinkable->getLinkID() != linkID)
        pLinkable = pLinkable->getPContainedObject();
    return pLinkable;
}

// Whether an object fits at (x, y): its trigger cells on the map, its
// blocking cells on terrain it allows, no trigger under another's blocking
// cell or blocking cell over another's trigger, and no object both above
// and below it.
VA(0x0042650e, 0x3dd)
bool TGameMap::_TImpl::_isValidPlacement(const TLayer& layer, const TGameObject& obj, unsigned int x, unsigned int y)
{
    static vector<int> aLowerObjIDs;
    static vector<int> aHigherObjIDs;
    aLowerObjIDs.clear();
    aHigherObjIDs.clear();
    unsigned int heightMap[kMaxObjWidth][kMaxObjHeight];
    constructObjectHeightMap(obj, heightMap);
    for (unsigned int i = 0; i < obj.getWidth(); i++) {
        unsigned int cellX = x - i;
        for (unsigned int j = 0; j < obj.getHeight(); j++) {
            unsigned int cellY = y - j;
            if (cellX >= layer.getWidth() || cellY >= layer.getHeight()) {
                if (obj.getBCellTrigger(i, j))
                    return false;
                continue;
            }
            if (!obj.getBCellPlaced(i, j))
                continue;
            unsigned int height = heightMap[i][j];
            const TLayer::TCell& cell = layer.getCell(cellX, cellY);
            const vector<TLayer::_TObjectCellInfo>* paObjInfo = cell._m_paObjInfo.get();
            if (paObjInfo != NULL) {
                const vector<TLayer::_TObjectCellInfo>& aObjInfo = *paObjInfo;
                vector<TLayer::_TObjectCellInfo>::const_iterator pObjInfo = aObjInfo.begin();
                while (pObjInfo != aObjInfo.end()) {
                    if (pObjInfo->m_height >= height)
                        break;
                    if (find(aLowerObjIDs.begin(), aLowerObjIDs.end(), pObjInfo->m_objID) == aLowerObjIDs.end())
                        aLowerObjIDs.push_back(pObjInfo->m_objID);
                    ++pObjInfo;
                }
                while (pObjInfo != aObjInfo.end()) {
                    if (pObjInfo->m_height > height)
                        break;
                    ++pObjInfo;
                }
                while (pObjInfo != aObjInfo.end()) {
                    if (find(aHigherObjIDs.begin(), aHigherObjIDs.end(), pObjInfo->m_objID) == aHigherObjIDs.end())
                        aHigherObjIDs.push_back(pObjInfo->m_objID);
                    ++pObjInfo;
                }
            }
            bool bPassable = obj.getBCellPassable(i, j);
            if ((!bPassable || obj.getBUnderlay()) && !obj.getTerrainMask()[cell.getTerrainType()])
                return false;
            if (!bPassable && paObjInfo != NULL) {
                const vector<TLayer::_TObjectCellInfo>& aObjInfo = *paObjInfo;
                if (obj.getBCellTrigger(i, j)) {
                    for (vector<TLayer::_TObjectCellInfo>::const_iterator pObjInfo = aObjInfo.begin();
                         pObjInfo != aObjInfo.end(); ++pObjInfo) {
                        const TGameObject& otherObj = layer.getObject(pObjInfo->m_objID);
                        TTilePoint otherLoc = layer.getObjectLoc(pObjInfo->m_objID);
                        unsigned int otherI = otherLoc.x() - cellX;
                        unsigned int otherJ = otherLoc.y() - cellY;
                        if (!otherObj.getBCellPassable(otherI, otherJ))
                            return false;
                    }
                } else {
                    for (vector<TLayer::_TObjectCellInfo>::const_iterator pObjInfo = aObjInfo.begin();
                         pObjInfo != aObjInfo.end(); ++pObjInfo) {
                        const TGameObject& otherObj = layer.getObject(pObjInfo->m_objID);
                        TTilePoint otherLoc = layer.getObjectLoc(pObjInfo->m_objID);
                        unsigned int otherI = otherLoc.x() - cellX;
                        unsigned int otherJ = otherLoc.y() - cellY;
                        if (otherObj.getBCellTrigger(otherI, otherJ))
                            return false;
                    }
                }
            }
        }
    }
    for (vector<int>::const_iterator pObjID = aLowerObjIDs.begin(); pObjID != aLowerObjIDs.end(); ++pObjID)
        if (find(aHigherObjIDs.begin(), aHigherObjIDs.end(), *pObjID) != aHigherObjIDs.end())
            return false;
    return obj.getType() != SHIPYARD || _isValidShipyardPlacement(layer, obj, x, y);
}

// A shipyard needs water at one of the cells beside its dock.
VA(0x0042693b, 0x129)
bool TGameMap::_TImpl::_isValidShipyardPlacement(const TLayer& layer, const TGameObject& shipyard, unsigned int x,
                                                 unsigned int y)
{
    static const TPoint<int> akWaterOffset[] = {
        TPoint<int>(1, 1), TPoint<int>(1, 0), TPoint<int>(1, -1), TPoint<int>(0, 1), TPoint<int>(0, -1),
        TPoint<int>(-1, 1), TPoint<int>(-1, -1), TPoint<int>(-2, 1), TPoint<int>(-2, -1), TPoint<int>(-3, 1),
        TPoint<int>(-3, 0), TPoint<int>(-3, -1)
    };
    for (unsigned int i = 0; i < sizeof(akWaterOffset) / sizeof(akWaterOffset[0]); i++) {
        TTilePoint loc = TPoint<int>(x, y) + akWaterOffset[i];
        if (loc.x() >= layer.getWidth() || loc.y() >= layer.getHeight())
            continue;
        if (layer.getCell(loc).getTerrainType() == eTerrainWater)
            return true;
    }
    return false;
}

// The map's customized heroes (Shadow of Death maps): per hero a flag,
// then each customized part behind its own flag.
VA(0x00426a65, 0x215)
void TGameMap::_TImpl::_readHeroSettings(TRawIStream* pIStream, int version)
{
    for (THeroID heroID = 0; heroID < kNumHeroes; heroID++) {
        unsigned char bCustomized;
        *pIStream >> bCustomized;
        if (!bCustomized)
            continue;
        THeroPrototype& prototype = (*_m_pProperties->m_aHeroPrototype)[heroID];
        unsigned char bHasExperience;
        *pIStream >> bHasExperience;
        if (bHasExperience) {
            int experience;
            *pIStream >> experience;
            prototype.setExperience(experience);
        }
        bool bHasSecondarySkills;
        *pIStream >> bHasSecondarySkills;
        if (bHasSecondarySkills) {
            THeroPrototype::TSecondarySkills secondarySkills;
            secondarySkills.read(pIStream, version);
            prototype.setSecondarySkills(secondarySkills);
        }
        bool bHasArtifacts;
        *pIStream >> bHasArtifacts;
        if (bHasArtifacts)
            prototype.setArtifacts(THeroPrototype::TArtifactContainer(pIStream, version));
        bool bHasBiography;
        *pIStream >> bHasBiography;
        if (bHasBiography) {
            string biography;
            *pIStream >> biography;
            prototype.setBiography(biography);
        }
        signed char sex;
        *pIStream >> sex;
        if (sex != -1)
            prototype.setSex(sex);
        unsigned char bHasSpells;
        *pIStream >> bHasSpells;
        if (bHasSpells)
            prototype.setSpells(THeroPrototype::TSpells(pIStream, version));
        bool bHasPrimarySkills;
        *pIStream >> bHasPrimarySkills;
        if (bHasPrimarySkills)
            prototype.setPrimarySkills(THeroPrototype::TPrimarySkills(pIStream, version));
    }
}

// A town brings its visiting hero along.
VA(0x00426d00, 0xed)
TMapLayerObjectID TGameMap::_TImpl::_placeTypedObject(bool bSecondLayer, TTown* pTown, const TTilePoint& loc,
                                                     TTileExtent* pUpdatedExtent)
{
    TMapLayerObjectID objID;
    try {
        objID = _placeNewObject(bSecondLayer, pTown, loc, pUpdatedExtent);
    } catch (...) {
        delete pTown;
        throw;
    }
    const TTown* pPlacedTown = dynamic_cast<const TTown*>(getLayer(bSecondLayer).getPObject(objID));
    _onTownAdded(pPlacedTown, bSecondLayer, objID);
    if (pPlacedTown->getPVisitingHero() != NULL) {
        const TNonRandomHero* pNonRandomHero = dynamic_cast<const TNonRandomHero*>(pPlacedTown->getPVisitingHero());
        if (pNonRandomHero != NULL)
            _onIdentifiedHeroAdded(pNonRandomHero, bSecondLayer, objID);
        else
            _onRandomHeroAdded(static_cast<const TRandomHero*>(pPlacedTown->getPVisitingHero()), bSecondLayer, objID);
    }
    return objID;
}

// What every object must pass: it lies on the map, the map's edition has
// it, its type's cap has room and its cells take it. The layer then owns
// it, and the cells it covers are the update.
VA(0x00426ded, 0x15a)
TMapLayerObjectID TGameMap::_TImpl::_placeNewObject(bool bSecondLayer, TGameObject* pObj, const TTilePoint& loc,
                                                   TTileExtent* pUpdatedExtent)
{
    if (!_isOnMap(*pObj, loc.x(), loc.y()))
        throw TPlaceObjFailurePlacementNotOnMap();
    EGameVersion requiredVersion = getRequiredVersion(pObj->getObjectType());
    if (_m_version < requiredVersion)
        throw TPlaceObjFailureNotSupportedByReleaseVersion(requiredVersion);
    TCappedObjectTypeInfoMap::const_iterator pCappedObjTypeInfo = getCappedObjectTypeInfoMap().find(pObj->getType());
    if (pCappedObjTypeInfo != getCappedObjectTypeInfoMap().end()) {
        const TCappedObjectTypeInfo* pInfo = pCappedObjTypeInfo->second;
        const TRefCountingPtr<_TBookkeeping>& pConstBookkeeping = _m_pBookkeeping;
        if (pConstBookkeeping->m_aNumObjsOfCappedType[pInfo->m_ordinal] >= pInfo->m_cap)
            throw TPlaceObjFailureTooManyInstancesOfTypeOnMap(pInfo->m_type, pInfo->m_cap);
    }
    TLayer* pLayer = getPLayer(bSecondLayer);
    if (!_isValidPlacement(*pLayer, *pObj, loc.x(), loc.y()))
        throw TPlaceObjFailureInvalidPlacement();
    TMapLayerObjectID objID = pLayer->_placeObject(auto_ptr<TGameObject>(pObj), loc);
    *pUpdatedExtent = pLayer->getObjectExtent(objID);
    return objID;
}

VA(0x00426fad, 0x50)
void TGameMap::_TImpl::_assignUniqueLinkID(TLinkableObject* pLinkable) const
{
    map<unsigned int, TMapObjectRef>::const_iterator pObjRef = _m_paLinkableObjectRef->find(pLinkable->getLinkID());
    while (pObjRef != _m_paLinkableObjectRef->end()) {
        pLinkable->assignNewLinkID();
        pObjRef = _m_paLinkableObjectRef->find(pLinkable->getLinkID());
    }
}

// No more heroes than the map's cap, nor than eight for a player, counting
// the hero a player's main town generates.
VA(0x00427025, 0x93)
TMapLayerObjectID TGameMap::_TImpl::_placeNewObject(bool bSecondLayer, TBasicHero* pHero, const TTilePoint& loc,
                                                   TTileExtent* pUpdatedExtent)
{
    const TRefCountingPtr<_TBookkeeping>& pConstBookkeeping = _m_pBookkeeping;
    if (pConstBookkeeping->m_numHeroes >= s_kMaxHeroesOnMap)
        throw TPlaceObjFailureTooManyHeroesOnMap();
    if (pHero->getOwner() != ePlayerNone) {
        const TRefCountingPtr<_TPlayerBookkeeping>& pConstPlayerBookkeeping =
            _m_apPlayerBookkeeping[pHero->getOwner()];
        unsigned int numHeroes = pConstPlayerBookkeeping->m_numHeroes;
        const TRefCountingPtr<_TProperties>& pConstProperties = _m_pProperties;
        if ((*pConstProperties->m_paPlayer)[pHero->getOwner()].getBGenerateHero())
            numHeroes++;
        if (numHeroes >= s_kMaxHeroesPerPlayer)
            throw TPlaceObjFailureTooManyHeroesForPlayer();
    }
    return _placeNewObject(bSecondLayer, static_cast<TPlayableObject*>(pHero), loc, pUpdatedExtent);
}

// A placeholder's hero must be free, or the placeholder falls back to its
// power rank.
VA(0x0042710f, 0x47)
TMapLayerObjectID TGameMap::_TImpl::_placeNewObject(bool bSecondLayer, THeroPlaceholder* pPlaceholder,
                                                   const TTilePoint& loc, TTileExtent* pUpdatedExtent)
{
    if (pPlaceholder->getHeroID() != -1 && !_isHeroAvailable(pPlaceholder->getHeroID()))
        pPlaceholder->setHeroID(-1);
    return _placeNewObject(bSecondLayer, static_cast<TBasicHero*>(pPlaceholder), loc, pUpdatedExtent);
}

// A hero that is taken becomes another of its class.
VA(0x00427156, 0x87)
TMapLayerObjectID TGameMap::_TImpl::_placeNewObject(bool bSecondLayer, TNonRandomHero* pHero, const TTilePoint& loc,
                                                   TTileExtent* pUpdatedExtent)
{
    if (!_isHeroAvailable(pHero->getHeroID())) {
        THeroID heroID = _pickRandomHero(pHero->getHeroClass());
        if (heroID == -1)
            throw TPlaceObjFailureNoAvailableHeroesInClass();
        pHero->setHeroID(heroID);
    }
    _assignUniqueLinkID(pHero);
    return _placeNewObject(bSecondLayer, static_cast<TBasicHero*>(pHero), loc, pUpdatedExtent);
}

// A prisoner that is taken becomes another of its class, or of any.
VA(0x00427207, 0xa2)
TMapLayerObjectID TGameMap::_TImpl::_placeNewObject(bool bSecondLayer, TPrison* pPrison, const TTilePoint& loc,
                                                   TTileExtent* pUpdatedExtent)
{
    if (!_isHeroAvailable(pPrison->getHeroID())) {
        THeroID heroID = _pickRandomHero(pPrison->getHeroClass());
        if (heroID == -1) {
            for (int heroClass = 0; heroClass < kNumHeroClasses; heroClass++) {
                heroID = _pickRandomHero(THeroClass(heroClass));
                if (heroID != -1)
                    break;
            }
            if (heroID == -1)
                throw TPlaceObjFailureNoAvailableHeroesInClass();
        }
        pPrison->setHeroID(heroID);
    }
    _assignUniqueLinkID(pPrison);
    return _placeNewObject(bSecondLayer, static_cast<TBasicHero*>(pPrison), loc, pUpdatedExtent);
}

// The town cap, and the visiting hero's: a taken visitor becomes another of
// its class or leaves.
VA(0x004272a9, 0x16e)
TMapLayerObjectID TGameMap::_TImpl::_placeNewObject(bool bSecondLayer, TTown* pTown, const TTilePoint& loc,
                                                   TTileExtent* pUpdatedExtent)
{
    const TRefCountingPtr<_TBookkeeping>& pConstBookkeeping = _m_pBookkeeping;
    if (pConstBookkeeping->m_numTowns == s_kMaxTownsOnMap)
        throw TPlaceObjFailureTooManyTownsOnMap();
    THero* pVisitingHero = pTown->getPVisitingHero();
    if (pVisitingHero != NULL) {
        if (pConstBookkeeping->m_numHeroes >= s_kMaxHeroesOnMap)
            throw TPlaceObjFailureTooManyHeroesOnMap();
        const TRefCountingPtr<_TPlayerBookkeeping>& pConstPlayerBookkeeping =
            _m_apPlayerBookkeeping[pTown->getOwner()];
        unsigned int numHeroes = pConstPlayerBookkeeping->m_numHeroes;
        const TRefCountingPtr<_TProperties>& pConstProperties = _m_pProperties;
        if ((*pConstProperties->m_paPlayer)[pTown->getOwner()].getBGenerateHero())
            numHeroes++;
        if (numHeroes >= s_kMaxHeroesPerPlayer)
            throw TPlaceObjFailureTooManyHeroesForPlayer();
        TIdentifiedHero* pIdentifiedHero = dynamic_cast<TIdentifiedHero*>(pVisitingHero);
        if (pIdentifiedHero != NULL && !_isHeroAvailable(pIdentifiedHero->getHeroID())) {
            THeroID heroID = _pickRandomHero(pIdentifiedHero->getHeroClass());
            if (heroID != -1)
                pIdentifiedHero->setHeroID(heroID);
            else
                pTown->setVisitingHero(NULL);
        }
        _assignUniqueLinkID(pVisitingHero);
    }
    _assignUniqueLinkID(pTown);
    return _placeNewObject(bSecondLayer, static_cast<TPlayableObject*>(pTown), loc, pUpdatedExtent);
}

// One Grail, at least nine cells from every edge.
VA(0x00427444, 0x89)
TMapLayerObjectID TGameMap::_TImpl::_placeNewObject(bool bSecondLayer, THolyGrail* pHolyGrail, const TTilePoint& loc,
                                                   TTileExtent* pUpdatedExtent)
{
    if (isGrailOnMap())
        throw TPlaceObjFailureHolyGrailAlreadyPlaced();
    if (!(loc.x() >= 9 && loc.y() >= 9 && loc.x() < getWidth() - 9 && loc.y() < getHeight() - 9))
        throw TPlaceObjFailureHolyGrailTooCloseToEdge();
    return _placeNewObject(bSecondLayer, static_cast<TGameObject*>(pHolyGrail), loc, pUpdatedExtent);
}

// A dwelling keeps its town only if that is a random town on the map.
VA(0x004274f7, 0x54)
TMapLayerObjectID TGameMap::_TImpl::_placeNewObject(bool bSecondLayer, TAbstractRandomlyAlignedGenerator* pGenerator,
                                                   const TTilePoint& loc, TTileExtent* pUpdatedExtent)
{
    if (pGenerator->getTownLinkID() != TLinkableObject::s_kNoLinkID && !_isRandomTownLink(pGenerator->getTownLinkID()))
        pGenerator->setTownLinkID(TLinkableObject::s_kNoLinkID);
    return _placeNewObject(bSecondLayer, static_cast<TGameObject*>(pGenerator->getPFlaggableObject()), loc,
                           pUpdatedExtent);
}

// A defeat quest keeps its target only if the map still has it.
VA(0x0042754b, 0xb5)
TMapLayerObjectID TGameMap::_TImpl::_placeNewObject(bool bSecondLayer, TQuestLocation* pQuestLocation,
                                                   const TTilePoint& loc, TTileExtent* pUpdatedExtent)
{
    if (pQuestLocation->getPQuest() != NULL) {
        const TQuestDefeatHero* pDefeatHero = dynamic_cast<const TQuestDefeatHero*>(pQuestLocation->getPQuest());
        if (pDefeatHero != NULL) {
            const TLinkableObject* pTarget = _findLinkableObject(pDefeatHero->getHeroLinkID());
            if (pTarget == NULL || dynamic_cast<const THero*>(pTarget) == NULL)
                pQuestLocation->clearQuest();
        } else {
            const TQuestDefeatMonster* pDefeatMonster =
                dynamic_cast<const TQuestDefeatMonster*>(pQuestLocation->getPQuest());
            if (pDefeatMonster != NULL) {
                const TLinkableObject* pTarget = _findLinkableObject(pDefeatMonster->getMonsterLinkID());
                if (pTarget == NULL || dynamic_cast<const TMonster*>(pTarget) == NULL)
                    pQuestLocation->clearQuest();
            }
        }
    }
    return _placeNewObject(bSecondLayer, static_cast<TGameObject*>(pQuestLocation), loc, pUpdatedExtent);
}

VA(0x00427600, 0x61)
void TGameMap::_TImpl::_removeObjectHelper(bool bSecondLayer, unsigned int objID)
{
    TLayer* pLayer = getPLayer(bSecondLayer);
    _onRemovingObject(pLayer->getObject(objID), static_cast<TBookkeptObjectTypes*>(NULL));
    pLayer->_removeObject(objID);
    onObjectRemoved();
    _m_pClient->onMapObjectRemoved(bSecondLayer, objID);
}

// A removed object may have been a condition's town, hero or monster.
VA(0x00427661, 0x101)
void TGameMap::_TImpl::onObjectRemoved()
{
    const TRefCountingPtr<_TProperties>& pConstProperties = _m_pProperties;
    if (!_TLossConditionValidater(*this).isValid(pConstProperties->m_pLossCondition.get())) {
        TRefCountingAutoPtr<TLossCondition> pNoLossCondition((auto_ptr<TLossCondition>()));
        _m_pProperties->m_pLossCondition = pNoLossCondition;
    }
    if (!_TVictoryConditionValidater(*this).isValid(pConstProperties->m_pVictoryCondition.get())) {
        TRefCountingAutoPtr<TVictoryCondition> pNoVictoryCondition((auto_ptr<TVictoryCondition>()));
        _m_pProperties->m_pVictoryCondition = pNoVictoryCondition;
    }
}

// A capped type's count follows its objects (the linkable and playable
// additions pass their place, which the count ignores).
VA(0x00427762, 0x5e)
void TGameMap::_TImpl::_onGeneralObjectAdded(const TGameObject& obj, bool bSecondLayer, unsigned int objID)
{
    TCappedObjectTypeInfoMap::const_iterator pCappedObjTypeInfo = getCappedObjectTypeInfoMap().find(obj.getType());
    if (pCappedObjTypeInfo != getCappedObjectTypeInfoMap().end()) {
        unsigned int typeOrdinal = pCappedObjTypeInfo->second->m_ordinal;
        _m_pBookkeeping->m_aNumObjsOfCappedType[typeOrdinal]++;
    }
}

VA(0x004277c0, 0x5e)
void TGameMap::_TImpl::_onRemovingGeneralObject(const TGameObject& obj)
{
    TCappedObjectTypeInfoMap::const_iterator pCappedObjTypeInfo = getCappedObjectTypeInfoMap().find(obj.getType());
    if (pCappedObjTypeInfo != getCappedObjectTypeInfoMap().end()) {
        unsigned int typeOrdinal = pCappedObjTypeInfo->second->m_ordinal;
        _m_pBookkeeping->m_aNumObjsOfCappedType[typeOrdinal]--;
    }
}

// Where each linkable object lies, by its link id.
VA(0x0042781e, 0x4a)
void TGameMap::_TImpl::_registerLinkable(const TLinkableObject& linkable, bool bSecondLayer, unsigned int objID)
{
    _m_paLinkableObjectRef->insert(map<unsigned int, TMapObjectRef>::value_type(linkable.getLinkID(),
                                                                                TMapObjectRef(bSecondLayer, objID)));
}

VA(0x00427868, 0x3e)
void TGameMap::_TImpl::_onLinkableAdded(const TLinkableObject* pLinkable, bool bSecondLayer, unsigned int objID)
{
    _onGeneralObjectAdded(*pLinkable, bSecondLayer, objID);
    _registerLinkable(*pLinkable, bSecondLayer, objID);
}

// A linkable object leaves: the linked quests naming it go, and so does
// its place in the registry.
VA(0x004278a6, 0x12a)
void TGameMap::_TImpl::_unregisterLinkable(const TLinkableObject& linkable)
{
    unsigned int numLayers = _m_bTwoLayer ? 2 : 1;
    for (unsigned int layerNum = 0; layerNum < numLayers; layerNum++) {
        const TLayer* pLayer = getPLayer(layerNum);
        for (TLayer::TObjectIDIter iter = pLayer->objectIDBegin(); iter != pLayer->objectIDEnd(); ++iter) {
            const TQuestLocation* pQuestLocation = dynamic_cast<const TQuestLocation*>(pLayer->getPObject(*iter));
            if (pQuestLocation != NULL && pQuestLocation->getPQuest() != NULL) {
                const TLinkedQuest* pLinkedQuest = dynamic_cast<const TLinkedQuest*>(pQuestLocation->getPQuest());
                if (pLinkedQuest != NULL && pLinkedQuest->getLinkID() == linkable.getLinkID())
                    clearQuest(TMapObjectRef(layerNum != 0, *iter));
            }
        }
    }
    _m_paLinkableObjectRef->erase(_m_paLinkableObjectRef->find(linkable.getLinkID()));
}

VA(0x004279d0, 0x2d)
void TGameMap::_TImpl::_onRemovingLinkable(const TLinkableObject* pLinkable)
{
    _unregisterLinkable(*pLinkable);
    _onRemovingGeneralObject(*pLinkable);
}

// A player's first unit makes the player present: human and computer
// playable as the map's first playable slot, otherwise computer playable
// and on the smallest team.
VA(0x004279fd, 0x11a)
void TGameMap::_TImpl::_onPlayableAdded(const TPlayableObject* pPlayable, bool bSecondLayer, unsigned int objID)
{
    _onGeneralObjectAdded(*pPlayable, bSecondLayer, objID);
    TPlayer owner = pPlayable->getOwner();
    if (owner != ePlayerNone && _m_apPlayerBookkeeping[owner]->m_numUnits++ == 0) {
        TPlayerInfo& player = (*_m_pProperties->m_paPlayer)[owner];
        if (_m_pBookkeeping->m_numPlayableSlots == 0) {
            player.setBHumanPlayable(true);
            player.setBComputerPlayable(true);
        } else {
            const TRefCountingPtr<_TProperties>& pConstProperties = _m_pProperties;
            if (pConstProperties->m_pTeamInfo->getBHasTeams())
                _m_pProperties->m_pTeamInfo->setPlayerTeam(owner, _pickAvailableTeam());
            player.setBComputerPlayable(true);
        }
        _m_pBookkeeping->m_numPlayableSlots++;
    }
}

// A player's last unit makes the player absent: the first remaining
// computer-playable player becomes human playable if none is, the teams
// close up, and each hero the player could hire is offered elsewhere.
VA(0x00427b17, 0x3d1)
void TGameMap::_TImpl::_onRemovingPlayable(const TPlayableObject* pPlayable)
{
    TPlayer owner = pPlayable->getOwner();
    if (owner != ePlayerNone && --_m_apPlayerBookkeeping[owner]->m_numUnits == 0) {
        TPlayerInfo& player = (*_m_pProperties->m_paPlayer)[owner];
        player = TPlayerInfo();
        if (--_m_pBookkeeping->m_numPlayableSlots != 0) {
            TPlayerInfo* pFirstPlayable = NULL;
            unsigned int i;
            for (i = 0; i < kNumPlayers; i++) {
                if ((*_m_pProperties->m_paPlayer)[i].getBHumanPlayable())
                    break;
                if (pFirstPlayable == NULL && (*_m_pProperties->m_paPlayer)[i].getBComputerPlayable())
                    pFirstPlayable = &(*_m_pProperties->m_paPlayer)[i];
            }
            if (i == kNumPlayers)
                pFirstPlayable->setBHumanPlayable(true);
        }
        const TRefCountingPtr<_TProperties>& pConstProperties = _m_pProperties;
        if (pConstProperties->m_pTeamInfo->getBHasTeams()) {
            TTeamInfo& teamInfo = *_m_pProperties->m_pTeamInfo;
            if (_m_pBookkeeping->m_numPlayableSlots == TTeamInfo::s_kMinTeams) {
                teamInfo.setBHasTeams(false);
            } else {
                unsigned int team = teamInfo.getPlayerTeam(owner);
                teamInfo.setPlayerTeam(owner, 0);
                bool bTeamInUse = false;
                unsigned int otherPlayer;
                for (otherPlayer = 0; otherPlayer < kNumPlayers; otherPlayer++) {
                    if ((*_m_pProperties->m_paPlayer)[otherPlayer].getBPresent()
                        && teamInfo.getPlayerTeam(TPlayer(otherPlayer)) == team) {
                        bTeamInUse = true;
                        break;
                    }
                }
                if (!bTeamInUse) {
                    if (teamInfo.getNumTeams() == TTeamInfo::s_kMinTeams) {
                        teamInfo.setBHasTeams(false);
                    } else {
                        for (otherPlayer = 0; otherPlayer < kNumPlayers; otherPlayer++) {
                            if ((*_m_pProperties->m_paPlayer)[otherPlayer].getBPresent()) {
                                unsigned int otherTeam = teamInfo.getPlayerTeam(TPlayer(otherPlayer));
                                if (otherTeam > team)
                                    teamInfo.setPlayerTeam(TPlayer(otherPlayer), otherTeam - 1);
                            }
                        }
                        teamInfo.setNumTeams(teamInfo.getNumTeams() - 1);
                    }
                } else if (_m_pBookkeeping->m_numPlayableSlots == teamInfo.getNumTeams()) {
                    unsigned int lastTeam = teamInfo.getNumTeams() - 1;
                    for (unsigned int player = 0;; player++) {
                        if ((*_m_pProperties->m_paPlayer)[player].getBPresent()
                            && teamInfo.getPlayerTeam(TPlayer(player)) == lastTeam) {
                            teamInfo.setPlayerTeam(TPlayer(player), 0);
                            break;
                        }
                    }
                    teamInfo.setNumTeams(lastTeam);
                }
            }
        }
        for (THeroID heroID = 0; heroID < kNumHeroes; heroID++) {
            TPlayerMask availability = getHeroPrototype(heroID).getAvailability();
            unsigned int availableTo = owner;
            if (availability[owner]) {
                TPlayerMask presentPlayers;
                for (int otherPlayer = 0; otherPlayer < kNumPlayers; otherPlayer++)
                    if (otherPlayer != owner)
                        presentPlayers.set(otherPlayer, isPlayerPresent(TPlayer(otherPlayer)));
                if (!presentPlayers.any())
                    continue;
                if (!(availability & presentPlayers).none())
                    continue;
                for (availableTo = 0; !presentPlayers[availableTo]; availableTo++)
                    ;
            }
            availability.set(availableTo, true);
            (*_m_pProperties->m_aHeroPrototype)[heroID].setAvailability(availability);
        }
    }
    _onRemovingGeneralObject(*pPlayable);
}

// A town counts toward the map's towns and, when owned, its player's
// towns of its faction (or random ones).
VA(0x00427f0e, 0xb0)
void TGameMap::_TImpl::_onTownAdded(const TTown* pTown, bool bSecondLayer, unsigned int objID)
{
    _registerLinkable(*pTown, bSecondLayer, objID);
    _onPlayableAdded(pTown, bSecondLayer, objID);
    _m_pBookkeeping->m_numTowns++;
    if (pTown->getOwner() != ePlayerNone) {
        _TPlayerBookkeeping& playerBookkeeping = *_m_apPlayerBookkeeping[pTown->getOwner()];
        playerBookkeeping.m_townRefs.insert(TMapObjectRef(bSecondLayer, objID));
        if (pTown->getType() == TOWN)
            playerBookkeeping.m_aNumTownsOfType[pTown->getTownType()]++;
        else
            playerBookkeeping.m_numRandomTowns++;
    }
}

// A random town's dwellings lose their link to it; its player loses it
// as a main town.
VA(0x00427fbe, 0x1dd)
void TGameMap::_TImpl::_onRemovingTown(const TTown* pTown)
{
    TMapObjectRef townRef = getLinkableObjectRef(pTown->getLinkID());
    if (pTown->getType() == RANDOM_TOWN) {
        unsigned int numLayers = _m_bTwoLayer ? 2 : 1;
        for (unsigned int layerNum = 0; layerNum < numLayers; layerNum++) {
            TLayer* pLayer = getPLayer(layerNum);
            for (TLayer::TObjectIDIter iter = pLayer->objectIDBegin(); iter != pLayer->objectIDEnd(); ++iter) {
                TAbstractRandomlyAlignedGenerator* pGenerator =
                    dynamic_cast<TAbstractRandomlyAlignedGenerator*>(pLayer->getPObject(*iter));
                if (pGenerator != NULL && pGenerator->getTownLinkID() == pTown->getLinkID())
                    pGenerator->setTownLinkID(TLinkableObject::s_kNoLinkID);
            }
        }
    }
    if (pTown->getOwner() != ePlayerNone) {
        _TPlayerBookkeeping& playerBookkeeping = *_m_apPlayerBookkeeping[pTown->getOwner()];
        if (pTown->getType() == TOWN)
            playerBookkeeping.m_aNumTownsOfType[pTown->getTownType()]--;
        else
            playerBookkeeping.m_numRandomTowns--;
        const TRefCountingPtr<_TProperties>& pConstProperties = _m_pProperties;
        const TPlayerInfo& constPlayer = (*pConstProperties->m_paPlayer)[pTown->getOwner()];
        if (constPlayer.getBHasMainTown() && constPlayer.getMainTownRef() == townRef)
            (*_m_pProperties->m_paPlayer)[pTown->getOwner()].clearMainTown();
        playerBookkeeping.m_townRefs.erase(townRef);
    }
    _m_pBookkeeping->m_numTowns--;
    _onRemovingPlayable(pTown);
    _unregisterLinkable(*pTown);
}

// Heroes and placeholders count toward the map's heroes and their
// player's.
VA(0x0042819b, 0x65)
void TGameMap::_TImpl::_onBasicHeroAdded(const TBasicHero* pHero, bool bSecondLayer, unsigned int objID)
{
    _onPlayableAdded(pHero, bSecondLayer, objID);
    _m_pBookkeeping->m_numHeroes++;
    if (pHero->getOwner() != ePlayerNone) {
        _TPlayerBookkeeping& playerBookkeeping = *_m_apPlayerBookkeeping[pHero->getOwner()];
        playerBookkeeping.m_numHeroes++;
    }
}

VA(0x00428200, 0x5d)
void TGameMap::_TImpl::_onRemovingBasicHero(const TBasicHero* pHero)
{
    if (pHero->getOwner() != ePlayerNone) {
        _TPlayerBookkeeping& playerBookkeeping = *_m_apPlayerBookkeeping[pHero->getOwner()];
        playerBookkeeping.m_numHeroes--;
    }
    _m_pBookkeeping->m_numHeroes--;
    _onRemovingPlayable(pHero);
}

// A placeholder names its hero, which is then on the map, or only a power
// rank.
VA(0x0042825d, 0xf1)
void TGameMap::_TImpl::_onHeroPlaceholderAdded(const THeroPlaceholder* pPlaceholder, bool bSecondLayer,
                                               unsigned int objID)
{
    _onBasicHeroAdded(pPlaceholder, bSecondLayer, objID);
    if (pPlaceholder->getHeroID() != -1) {
        THeroID heroID = pPlaceholder->getHeroID();
        THeroClass heroClass = THero::s_akTraits[heroID].m_class;
        _m_pBookkeeping->m_heroesOnMap.set(heroID, true);
        _m_apPlayerBookkeeping[pPlaceholder->getOwner()]->m_paHeroPlaceholderID->insert(heroID);
        _m_apPlayerBookkeeping[pPlaceholder->getOwner()]
            ->m_aNumHeroesOfType[THero::s_akClassTraits[heroClass].m_townType]++;
    } else {
        _m_apPlayerBookkeeping[pPlaceholder->getOwner()]->m_numHeroPlaceholders++;
    }
}

VA(0x0042834e, 0xeb)
void TGameMap::_TImpl::_onRemovingHeroPlaceholder(const THeroPlaceholder* pPlaceholder)
{
    if (pPlaceholder->getHeroID() != -1) {
        THeroID heroID = pPlaceholder->getHeroID();
        THeroClass heroClass = THero::s_akTraits[heroID].m_class;
        _m_apPlayerBookkeeping[pPlaceholder->getOwner()]
            ->m_aNumHeroesOfType[THero::s_akClassTraits[heroClass].m_townType]--;
        _m_apPlayerBookkeeping[pPlaceholder->getOwner()]->m_paHeroPlaceholderID->erase(heroID);
        _m_pBookkeeping->m_heroesOnMap.set(heroID, false);
    } else {
        _m_apPlayerBookkeeping[pPlaceholder->getOwner()]->m_numHeroPlaceholders--;
    }
    _onRemovingBasicHero(pPlaceholder);
}

VA(0x00428439, 0x51)
void TGameMap::_TImpl::_onRandomHeroAdded(const TRandomHero* pHero, bool bSecondLayer, unsigned int objID)
{
    _registerLinkable(*pHero, bSecondLayer, objID);
    _onBasicHeroAdded(pHero, bSecondLayer, objID);
    _m_apPlayerBookkeeping[pHero->getOwner()]->m_numRandomHeroes++;
}

VA(0x0042848a, 0x46)
void TGameMap::_TImpl::_onRemovingRandomHero(const TRandomHero* pHero)
{
    _m_apPlayerBookkeeping[pHero->getOwner()]->m_numRandomHeroes--;
    _onRemovingBasicHero(pHero);
    _unregisterLinkable(*pHero);
}

// A specific hero is on the map once, where the map keeps it by its id.
VA(0x004284d0, 0x130)
void TGameMap::_TImpl::_onIdentifiedHeroAdded(const TIdentifiedHero* pHero, bool bSecondLayer, unsigned int objID)
{
    _registerLinkable(*pHero, bSecondLayer, objID);
    _onBasicHeroAdded(pHero, bSecondLayer, objID);
    THeroID heroID = pHero->getHeroID();
    _m_pBookkeeping->m_paHeroRef->insert(map<THeroID, TMapObjectRef>::value_type(heroID,
                                                                                 TMapObjectRef(bSecondLayer, objID)));
    _m_pBookkeeping->m_heroesOnMap.set(heroID, true);
    if (pHero->getOwner() != ePlayerNone) {
        _m_apPlayerBookkeeping[pHero->getOwner()]->m_paHeroID->insert(heroID);
        _m_apPlayerBookkeeping[pHero->getOwner()]
            ->m_aNumHeroesOfType[THero::s_akClassTraits[pHero->getHeroClass()].m_townType]++;
    }
}

VA(0x00428600, 0x10d)
void TGameMap::_TImpl::_onRemovingIdentifiedHero(const TIdentifiedHero* pHero)
{
    THeroID heroID = pHero->getHeroID();
    if (pHero->getOwner() != ePlayerNone) {
        _m_apPlayerBookkeeping[pHero->getOwner()]
            ->m_aNumHeroesOfType[THero::s_akClassTraits[pHero->getHeroClass()].m_townType]--;
        _m_apPlayerBookkeeping[pHero->getOwner()]->m_paHeroID->erase(heroID);
    }
    _m_pBookkeeping->m_heroesOnMap.set(heroID, false);
    _m_pBookkeeping->m_paHeroRef->erase(heroID);
    _onRemovingBasicHero(pHero);
    _unregisterLinkable(*pHero);
}

VA(0x0042870d, 0x42)
void TGameMap::_TImpl::_onHolyGrailAdded(const THolyGrail* pHolyGrail, bool bSecondLayer, unsigned int objID)
{
    _onGeneralObjectAdded(*pHolyGrail, bSecondLayer, objID);
    _m_pBookkeeping->m_bGrailPlaced = true;
}

VA(0x0042874f, 0x39)
void TGameMap::_TImpl::_onRemovingHolyGrail(const THolyGrail* pHolyGrail)
{
    _m_pBookkeeping->m_bGrailPlaced = false;
    _onRemovingGeneralObject(*pHolyGrail);
}

VA(0x00428788, 0x8f)
void TGameMap::_TImpl::_onObjectAdded(const TGameObject& obj, bool bSecondLayer, unsigned int objID,
                                      TObjectTypesFromTown*)
{
    const TTown* pTown = dynamic_cast<const TTown*>(&obj);
    if (pTown != NULL) {
        _onTownAdded(pTown, bSecondLayer, objID);
        if (pTown->getPVisitingHero() != NULL) {
            const TNonRandomHero* pNonRandomHero = dynamic_cast<const TNonRandomHero*>(pTown->getPVisitingHero());
            if (pNonRandomHero != NULL)
                _onIdentifiedHeroAdded(pNonRandomHero, bSecondLayer, objID);
            else
                _onRandomHeroAdded(static_cast<const TRandomHero*>(pTown->getPVisitingHero()), bSecondLayer, objID);
        }
    } else {
        _onObjectAdded(obj, bSecondLayer, objID, static_cast<TObjectTypesAfterTown*>(NULL));
    }
}

VA(0x00428817, 0x7b)
void TGameMap::_TImpl::_onRemovingObject(const TGameObject& obj, TObjectTypesFromTown*)
{
    const TTown* pTown = dynamic_cast<const TTown*>(&obj);
    if (pTown != NULL) {
        if (pTown->getPVisitingHero() != NULL) {
            const TNonRandomHero* pNonRandomHero = dynamic_cast<const TNonRandomHero*>(pTown->getPVisitingHero());
            if (pNonRandomHero != NULL)
                _onRemovingIdentifiedHero(pNonRandomHero);
            else
                _onRemovingRandomHero(static_cast<const TRandomHero*>(pTown->getPVisitingHero()));
        }
        _onRemovingTown(pTown);
    } else {
        _onRemovingObject(obj, static_cast<TObjectTypesAfterTown*>(NULL));
    }
}

// Each hero the map customizes, member by member where it differs from the
// game's definition (_readHeroSettings reads it back).
VA(0x00428892, 0x237)
void TGameMap::_TImpl::_writeHeroSettings(TRawOStream* pOStream, int version) const
{
    for (THeroID heroID = 0; heroID < kNumHeroes; heroID++) {
        const THeroPrototype& prototype = (*_m_pProperties->m_aHeroPrototype)[heroID];
        const THeroPrototype& gamePrototype = THero::s_akTraits[heroID].m_prototype;
        bool bCustomized = !(prototype == gamePrototype);
        *pOStream << static_cast<unsigned char>(bCustomized);
        if (bCustomized) {
            if (prototype.getExperience() != gamePrototype.getExperience()) {
                *pOStream << true;
                *pOStream << prototype.getExperience();
            } else {
                *pOStream << false;
            }
            if (prototype.getSecondarySkills() != gamePrototype.getSecondarySkills()) {
                *pOStream << true;
                prototype.getSecondarySkills().write(pOStream, version);
            } else {
                *pOStream << false;
            }
            if (prototype.getArtifacts() != gamePrototype.getArtifacts()) {
                *pOStream << true;
                prototype.getArtifacts().write(pOStream, version);
            } else {
                *pOStream << false;
            }
            if (prototype.getBiography() != gamePrototype.getBiography()) {
                *pOStream << true;
                *pOStream << prototype.getBiography();
            } else {
                *pOStream << false;
            }
            int sex = prototype.getSex();
            if (sex == gamePrototype.getSex())
                sex = -1;
            *pOStream << static_cast<signed char>(sex);
            if (prototype.getSpells() != gamePrototype.getSpells()) {
                *pOStream << true;
                prototype.getSpells().write(pOStream, version);
            } else {
                *pOStream << false;
            }
            if (!(prototype.getPrimarySkills() == gamePrototype.getPrimarySkills())) {
                *pOStream << true;
                prototype.getPrimarySkills().write(pOStream, version);
            } else {
                *pOStream << false;
            }
        }
    }
}

// Whether the game can play the map: someone plays it, enough heroes are
// enabled to fill the taverns and towns, and some artifact is left for the
// random ones once the special artifacts and those that quests or the
// victory condition ask for are set aside.
VA(0x00428b34, 0x211)
bool TGameMap::_TImpl::_isPlayable() const
{
    if (_m_pBookkeeping->m_numPlayableSlots == 0)
        return false;
    if ((~_m_pProperties->m_disabledHeroes).count()
        < _m_pBookkeeping->m_numTowns + _m_pBookkeeping->m_numPlayableSlots * 10)
        return false;
    bitset<kNumArtifacts> availableArtifacts = ~_m_pProperties->m_disabledArtifacts;
    for (int artifact = 0; artifact < kNumArtifacts; artifact++)
        if (akArtifactTraits[artifact].m_class & ArtifactClassSpecial)
            availableArtifacts.set(artifact, false);
    unsigned int numLayers = _m_bTwoLayer ? 2 : 1;
    for (unsigned int layerNum = 0; layerNum < numLayers; layerNum++) {
        const TLayer* pLayer = getPLayer(layerNum);
        for (TLayer::TObjectIDIter iter = pLayer->objectIDBegin(); iter != pLayer->objectIDEnd(); ++iter) {
            const TQuestLocation* pQuestLocation = dynamic_cast<const TQuestLocation*>(pLayer->getPObject(*iter));
            if (pQuestLocation != NULL && pQuestLocation->getPQuest() != NULL
                && dynamic_cast<const TQuestBringArtifacts*>(pQuestLocation->getPQuest()) != NULL) {
                const multiset<TArtifact>& artifacts =
                    static_cast<const TQuestBringArtifacts*>(pQuestLocation->getPQuest())->getArtifacts();
                for (multiset<TArtifact>::const_iterator pArtifact = artifacts.begin(); pArtifact != artifacts.end();
                     ++pArtifact)
                    availableArtifacts.set(*pArtifact, false);
            }
        }
    }
    const TVictoryCondition* pVictoryCondition = _m_pProperties->m_pVictoryCondition.get();
    if (pVictoryCondition != NULL) {
        if (dynamic_cast<const TVCAquireArtifact*>(pVictoryCondition) != NULL)
            availableArtifacts.set(static_cast<const TVCAquireArtifact*>(pVictoryCondition)->getArtifact(), false);
        else if (dynamic_cast<const TVCTransportArtifact*>(pVictoryCondition) != NULL)
            availableArtifacts.set(static_cast<const TVCTransportArtifact*>(pVictoryCondition)->getArtifact(), false);
    }
    return availableArtifacts.any();
}

// Whether any cell the object covers lies on the map; the object's
// location is its bottom right cell.
VA(0x00428d45, 0xa9)
bool TGameMap::_TImpl::_isOnMap(const TGameObject& obj, unsigned int x, unsigned int y) const
{
    if (x + 1 >= obj.getWidth() && x + 1 - obj.getWidth() >= getWidth())
        return false;
    if (y + 1 >= obj.getHeight() && y + 1 - obj.getHeight() >= getHeight())
        return false;
    for (unsigned int j = 0; j < obj.getHeight(); j++) {
        if (y - j < getHeight()) {
            for (unsigned int i = 0; i < obj.getWidth(); i++) {
                if (x - i < getWidth() && obj.getBCellPlaced(i, j))
                    return true;
            }
        }
    }
    return false;
}

// A hero the map's version has, not yet on the map and not disabled.
VA(0x00428dee, 0x98)
bool TGameMap::_TImpl::_isHeroAvailable(THeroID heroID) const
{
    return THero::s_akTraits[heroID].m_gameVersions.test(_m_version) && !_m_pBookkeeping->m_heroesOnMap.test(heroID)
           && !_m_pProperties->m_disabledHeroes.test(heroID);
}

// A random available hero of the class, a special one only when no other
// is left; none (-1) when the class has none.
VA(0x00428e86, 0x1ec)
THeroID TGameMap::_TImpl::_pickRandomHero(THeroClass heroClass) const
{
    const THero::TClassTraits& classTraits = THero::s_akClassTraits[heroClass];
    vector<THeroID> candidates;
    candidates.reserve(classTraits.m_heroes.size());
    set<int>::const_iterator pHeroID;
    for (pHeroID = classTraits.m_heroes.begin(); pHeroID != classTraits.m_heroes.end(); ++pHeroID) {
        const THero::TTraits& traits = THero::s_akTraits[*pHeroID];
        if (traits.m_gameVersions.test(_m_version) && !traits.m_bSpecial
            && !_m_pBookkeeping->m_heroesOnMap.test(*pHeroID) && !_m_pProperties->m_disabledHeroes.test(*pHeroID))
            candidates.push_back(*pHeroID);
    }
    if (candidates.empty()) {
        for (pHeroID = classTraits.m_heroes.begin(); pHeroID != classTraits.m_heroes.end(); ++pHeroID) {
            const THero::TTraits& traits = THero::s_akTraits[*pHeroID];
            if (traits.m_gameVersions.test(_m_version) && traits.m_bSpecial
                && !_m_pBookkeeping->m_heroesOnMap.test(*pHeroID) && !_m_pProperties->m_disabledHeroes.test(*pHeroID))
                candidates.push_back(*pHeroID);
        }
    }
    THeroID result;
    if (candidates.size() == 0)
        result = -1;
    else
        result = candidates[rand() % candidates.size()];
    return result;
}

// The smallest team.
VA(0x00429072, 0x89)
unsigned int TGameMap::_TImpl::_pickAvailableTeam() const
{
    const TTeamInfo& teamInfo = *_m_pProperties->m_pTeamInfo;
    unsigned int aTeamSize[kNumPlayers];
    fill_n(aTeamSize, teamInfo.getNumTeams(), 0U);
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        if ((*_m_pProperties->m_paPlayer)[player].getBPresent()) {
            unsigned int team = teamInfo.getPlayerTeam(TPlayer(player));
            if (team < teamInfo.getNumTeams())
                aTeamSize[team]++;
        }
    }
    unsigned int result = 0;
    for (unsigned int team = 1; team < teamInfo.getNumTeams(); team++)
        if (aTeamSize[team] < aTeamSize[result])
            result = team;
    return result;
}

// A pasted or read object's link id must not repeat one on the map.
VA(0x004290fb, 0x3c)
void TGameMap::_TImpl::_makeLinkIDUnique(TLinkableObject& linkable) const
{
    while (_m_paLinkableObjectRef->find(linkable.getLinkID()) != _m_paLinkableObjectRef->end())
        linkable.assignNewLinkID();
}

VA(0x00429137, 0x5f)
const TLinkableObject* TGameMap::_TImpl::_findLinkableObject(unsigned int linkID) const
{
    map<unsigned int, TMapObjectRef>::const_iterator pObjRef = _m_paLinkableObjectRef->find(linkID);
    if (pObjRef != _m_paLinkableObjectRef->end())
        return dynamic_cast<const TLinkableObject*>(
            &getLayer(pObjRef->second.getBSecondLayer()).getObject(pObjRef->second.getObjectID()));
    return NULL;
}

// Whether a link id names a random town.
VA(0x00429196, 0x86)
bool TGameMap::_TImpl::_isRandomTownLink(unsigned int linkID) const
{
    map<unsigned int, TMapObjectRef>::const_iterator pObjRef = _m_paLinkableObjectRef->find(linkID);
    if (pObjRef != _m_paLinkableObjectRef->end()) {
        const TTown* pTown = dynamic_cast<const TTown*>(dynamic_cast<const TLinkableObject*>(
            &getLayer(pObjRef->second.getBSecondLayer()).getObject(pObjRef->second.getObjectID())));
        if (pTown != NULL && pTown->getType() == RANDOM_TOWN)
            return true;
    }
    return false;
}

VA(0x0042921c, 0x70)
void TGameMap::_TImpl::_getObjectLoc(const TMapObjectRef& objRef, TMapLoc* pLoc) const
{
    if (objRef.getObjectID() != TLayer::s_kInvalidObjID) {
        const TLayer& layer = getLayer(objRef.getBSecondLayer());
        const TObjectType::TPoint& triggerLoc = layer.getObject(objRef.getObjectID()).getTriggerLoc();
        TTilePoint loc = layer.getObjectLoc(objRef.getObjectID());
        pLoc->m_x = loc.x() - triggerLoc.m_x;
        pLoc->m_y = loc.y() - triggerLoc.m_y;
        pLoc->m_layer = objRef.getBSecondLayer() ? 1 : 0;
    } else {
        pLoc->m_x = -1;
        pLoc->m_y = -1;
        pLoc->m_layer = -1;
    }
}

// The object a condition record's location names, as the reading maps
// find it: none for no location.
VA(0x0042928c, 0x75)
TMapObjectRef TGameMap::_TImpl::_findObject(const TMapLoc& loc, bool (*pfnPredicate)(const TGameObject&)) const
{
    if (loc.m_x == -1)
        return TMapObjectRef();
    TMapLayerObjectID objID = _m_aLayer[loc.m_layer]._findObject(TTilePoint(loc.m_x, loc.m_y), pfnPredicate);
    if (objID == TLayer::s_kInvalidObjID)
        return TMapObjectRef();
    return TMapObjectRef(loc.m_layer != 0, objID);
}

// The victory condition a map's record describes.
VA(0x00429301, 0x30d)
auto_ptr<TVictoryCondition> TGameMap::_TImpl::_reconstructVictoryCondition(const TVictoryConditionData& vcData) const
{
    if (vcData.m_type == eVCNone)
        return auto_ptr<TVictoryCondition>();
    TVictoryCondition* pResult;
    switch (vcData.m_type) {
    case eVCAquireArtifact:
        pResult = new TVCAquireArtifact(vcData.m_bAppliesToComputer, vcData.m_aquireArtifact.m_artifact);
        break;
    case eVCAccumulateCreature:
        pResult = new TVCAccumulateCreature(vcData.m_bAllowNormalVictory, vcData.m_bAppliesToComputer,
                                            vcData.m_accumulateCreature.m_creatureType,
                                            vcData.m_accumulateCreature.m_quantity);
        break;
    case eVCAccumulateResource:
        pResult = new TVCAccumulateResource(vcData.m_bAllowNormalVictory, vcData.m_bAppliesToComputer,
                                            vcData.m_accumulateResource.m_resourceType,
                                            vcData.m_accumulateResource.m_quantity);
        break;
    case eVCUpgradeTown: {
        TMapObjectRef townRef = _findObject(vcData.m_upgradeTown.m_townLoc, isTown);
        pResult = new TVCUpgradeTown(vcData.m_bAllowNormalVictory, townRef,
                                     TVCUpgradeTown::THallLevel(vcData.m_upgradeTown.m_hallLevel),
                                     TVCUpgradeTown::TCastleLevel(vcData.m_upgradeTown.m_castleLevel));
        break;
    }
    case eVCBuildHolyGrailStruct: {
        TMapObjectRef townRef = _findObject(vcData.m_buildHolyGrailStruct.m_townLoc, isTown);
        pResult = new TVCBuildHolyGrailStruct(townRef);
        break;
    }
    case eVCDefeatHero: {
        TMapObjectRef heroRef = _findObject(vcData.m_defeatHero.m_heroLoc, isHeroOrTown);
        pResult = new TVCDefeatHero(heroRef);
        break;
    }
    case eVCCaptureTown: {
        TMapObjectRef townRef = _findObject(vcData.m_captureTown.m_townLoc, isTown);
        pResult = new TVCCaptureTown(vcData.m_bAllowNormalVictory, vcData.m_bAppliesToComputer, townRef);
        break;
    }
    case eVCDefeatMonster: {
        TMapObjectRef monsterRef = _findObject(vcData.m_defeatMonster.m_monsterLoc, isMonster);
        pResult = new TVCDefeatMonster(vcData.m_bAllowNormalVictory, monsterRef);
        break;
    }
    case eVCFlagAllCreatureGenerators:
        pResult = new TVCFlagAllCreatureGenerators(vcData.m_bAllowNormalVictory, vcData.m_bAppliesToComputer);
        break;
    case eVCFlagAllMines:
        pResult = new TVCFlagAllMines(vcData.m_bAllowNormalVictory, vcData.m_bAppliesToComputer);
        break;
    case eVCTransportArtifact: {
        TMapObjectRef townRef = _findObject(vcData.m_transportArtifact.m_townLoc, isTown);
        pResult = new TVCTransportArtifact(vcData.m_bAppliesToComputer, vcData.m_transportArtifact.m_artifact,
                                           townRef);
        break;
    }
    }
    if (pResult == NULL)
        throw TAllocationFailure();
    return auto_ptr<TVictoryCondition>(pResult);
}

// The loss condition a map's record describes.
VA(0x00429708, 0xee)
auto_ptr<TLossCondition> TGameMap::_TImpl::_reconstructLossCondition(const TLossConditionData& lcData) const
{
    if (lcData.m_type == eLCNone)
        return auto_ptr<TLossCondition>();
    TLossCondition* pResult;
    switch (lcData.m_type) {
    case eLCLoseTown: {
        TMapObjectRef townRef = _findObject(lcData.m_loseTown.m_townLoc, isTown);
        pResult = new TLCLoseTown(townRef);
        break;
    }
    case eLCLoseHero: {
        TMapObjectRef heroRef = _findObject(lcData.m_loseHero.m_heroLoc, isHeroOrTown);
        pResult = new TLCLoseHero(heroRef);
        break;
    }
    case eLCTimeExpires:
        pResult = new TLCTimeExpires(lcData.m_timeExpires.m_numDays);
        break;
    }
    if (pResult == NULL)
        throw TAllocationFailure();
    return auto_ptr<TLossCondition>(pResult);
}

VA(0x00429a12, 0x4e)
TGameMap::TGameMap(TClient* pClient, EGameVersion version, TSize size, bool bTwoLayer)
    : _m_pImpl(_TImpl(pClient, version, size, bTwoLayer))
{
}

VA(0x00429a60, 0x4e)
TGameMap::TGameMap(TClient* pClient, EGameVersion version, streambuf* pStreamBuf, int fileVersion)
    : _m_pImpl(_TImpl(pClient, version, pStreamBuf, fileVersion))
{
}

VA(0x00429aae, 0x5)
TGameMap::~TGameMap()
{
}

VA(0x00429ab3, 0x12)
TGameMap& TGameMap::operator=(const TGameMap& other)
{
    _m_pImpl = other._m_pImpl;
    return *this;
}

VA(0x00429ac5, 0x21)
void TGameMap::importText(istream* pIStream)
{
    _m_pImpl->importText(pIStream);
}

VA(0x00429ae6, 0x1f)
TGameMap::TLayer* TGameMap::getPLayer(unsigned int num)
{
    return _m_pImpl->getPLayer(num);
}

VA(0x00429b05, 0x21)
void TGameMap::setName(const string& newName)
{
    _m_pImpl->setName(newName);
}

VA(0x00429b26, 0x21)
void TGameMap::setDesc(const string& newDesc)
{
    _m_pImpl->setDesc(newDesc);
}

VA(0x00429b47, 0x2f)
void TGameMap::setDifficulty(TDifficulty newDifficulty)
{
    _m_pImpl->setDifficulty(newDifficulty);
}

VA(0x00429b76, 0x2f)
void TGameMap::setMaxHeroLevel(unsigned int newMaxHeroLevel)
{
    _m_pImpl->setMaxHeroLevel(newMaxHeroLevel);
}

VA(0x00429ba5, 0x21)
void TGameMap::setPlayers(const TArray<TPlayerInfo, kNumPlayers>& newPlayers)
{
    _m_pImpl->setPlayers(newPlayers);
}

VA(0x00429bc6, 0x21)
void TGameMap::setTeamInfo(const TTeamInfo& newTeamInfo)
{
    _m_pImpl->setTeamInfo(newTeamInfo);
}

VA(0x00429be7, 0x21)
void TGameMap::setRumors(const vector<TRumor>& newRumors)
{
    _m_pImpl->setRumors(newRumors);
}

VA(0x00429c08, 0x21)
void TGameMap::setTimedEvents(const vector<TTimedEvent>& newTimedEvents)
{
    _m_pImpl->setTimedEvents(newTimedEvents);
}

VA(0x00429c29, 0x54)
void TGameMap::setVictoryCondition(auto_ptr<TVictoryCondition> pNewVictoryCondition)
{
    _m_pImpl->setVictoryCondition(pNewVictoryCondition);
}

VA(0x00429c7d, 0x54)
void TGameMap::setLossCondition(auto_ptr<TLossCondition> pNewLossCondition)
{
    _m_pImpl->setLossCondition(pNewLossCondition);
}

VA(0x00429cd1, 0x36)
void TGameMap::setDisabledHeroes(const bitset<kNumHeroes>& newMask)
{
    _m_pImpl->setDisabledHeroes(newMask);
}

VA(0x00429d07, 0x36)
void TGameMap::setDisabledArtifacts(const bitset<kNumArtifacts>& newMask)
{
    _m_pImpl->setDisabledArtifacts(newMask);
}

VA(0x00429d3d, 0x34)
void TGameMap::setDisabledSpells(const bitset<kNumSpells>& newMask)
{
    _m_pImpl->setDisabledSpells(newMask);
}

VA(0x00429d71, 0x31)
void TGameMap::setDisabledSkills(const bitset<kNumSecSkills>& newMask)
{
    _m_pImpl->setDisabledSkills(newMask);
}

VA(0x00429da2, 0x25)
void TGameMap::setHeroPrototype(THeroID heroID, const THeroPrototype& newPrototype)
{
    _m_pImpl->setHeroPrototype(heroID, newPrototype);
}

VA(0x00429dc7, 0x5b)
TMapLayerObjectID TGameMap::placeObject(bool bSecondLayer, auto_ptr<TGameObject> pObj, const TTilePoint& loc,
                                        TTileExtent* pUpdatedExtent)
{
    return _m_pImpl->placeObject(bSecondLayer, pObj, loc, pUpdatedExtent);
}

VA(0x00429e22, 0x29)
void TGameMap::removeObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent)
{
    _m_pImpl->removeObject(bSecondLayer, objID, pUpdatedExtent);
}

VA(0x00429e4b, 0x58)
TMapLayerObjectID TGameMap::insertObject(bool bSecondLayer, auto_ptr<TGameObject> pObj, const TTilePoint& loc)
{
    return _m_pImpl->insertObject(bSecondLayer, pObj, loc);
}

VA(0x00429ea3, 0x25)
void TGameMap::eraseObject(bool bSecondLayer, unsigned int objID)
{
    _m_pImpl->eraseObject(bSecondLayer, objID);
}

VA(0x00429ec8, 0x29)
void TGameMap::floatObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent)
{
    _m_pImpl->floatObject(bSecondLayer, objID, pUpdatedExtent);
}

VA(0x00429ef1, 0x2d)
void TGameMap::unfloatObject(bool bSecondLayer, unsigned int x, unsigned int y, TTileExtent* pUpdatedExtent)
{
    _m_pImpl->unfloatObject(bSecondLayer, x, y, pUpdatedExtent);
}

VA(0x00429f1e, 0x21)
void TGameMap::removeFloatingObject(bool bSecondLayer)
{
    _m_pImpl->removeFloatingObject(bSecondLayer);
}

VA(0x00429f3f, 0x30)
bool TGameMap::onTerrainTypeChanged(bool bSecondLayer, unsigned int x, unsigned int y, TTerrainType oldTerrainType,
                                    TTileExtent* pUpdatedExtent)
{
    return _m_pImpl->onTerrainTypeChanged(bSecondLayer, x, y, oldTerrainType, pUpdatedExtent);
}

VA(0x00429f6f, 0x29)
void TGameMap::setPlaceholderHeroID(bool bSecondLayer, unsigned int objID, THeroID newHeroID)
{
    _m_pImpl->setPlaceholderHeroID(bSecondLayer, objID, newHeroID);
}

VA(0x00429f98, 0x25)
void TGameMap::clearPlaceholderHeroID(bool bSecondLayer, unsigned int objID)
{
    _m_pImpl->clearPlaceholderHeroID(bSecondLayer, objID);
}

VA(0x00429fbd, 0x29)
void TGameMap::setPlaceholderOwner(bool bSecondLayer, unsigned int objID, TPlayer newOwner)
{
    _m_pImpl->setPlaceholderOwner(bSecondLayer, objID, newOwner);
}

VA(0x00429fe6, 0x29)
void TGameMap::setHeroID(bool bSecondLayer, unsigned int objID, THeroID newHeroID)
{
    _m_pImpl->setHeroID(bSecondLayer, objID, newHeroID);
}

VA(0x0042a00f, 0x29)
void TGameMap::setHeroOwner(bool bSecondLayer, unsigned int objID, TPlayer newOwner)
{
    _m_pImpl->setHeroOwner(bSecondLayer, objID, newOwner);
}

VA(0x0042a038, 0x29)
void TGameMap::setVisitingHero(const THero* pHero, bool bSecondLayer, unsigned int objID)
{
    _m_pImpl->setVisitingHero(pHero, bSecondLayer, objID);
}

VA(0x0042a061, 0x25)
void TGameMap::removeVisitingHero(bool bSecondLayer, unsigned int objID)
{
    _m_pImpl->removeVisitingHero(bSecondLayer, objID);
}

VA(0x0042a086, 0x29)
void TGameMap::setTownOwner(TPlayer newOwner, bool bSecondLayer, unsigned int objID)
{
    _m_pImpl->setTownOwner(newOwner, bSecondLayer, objID);
}

VA(0x0042a0af, 0x25)
void TGameMap::linkGeneratorToTown(const TMapObjectRef& generatorRef, const TMapObjectRef& townRef)
{
    _m_pImpl->linkGeneratorToTown(generatorRef, townRef);
}

VA(0x0042a0d4, 0x21)
void TGameMap::unlinkGenerator(const TMapObjectRef& generatorRef)
{
    _m_pImpl->unlinkGenerator(generatorRef);
}

VA(0x0042a0f5, 0x57)
void TGameMap::setQuest(const TMapObjectRef& questLocationRef, auto_ptr<TQuest> pQuest)
{
    _m_pImpl->setQuest(questLocationRef, pQuest);
}

VA(0x0042a14c, 0x21)
void TGameMap::clearQuest(const TMapObjectRef& questLocationRef)
{
    _m_pImpl->clearQuest(questLocationRef);
}

VA(0x0042a16d, 0x1b)
void TGameMap::removeSecondLayer()
{
    _m_pImpl->removeSecondLayer();
}

VA(0x0042a188, 0x1b)
void TGameMap::addSecondLayer()
{
    _m_pImpl->addSecondLayer();
}

VA(0x0042a1a3, 0x21)
void TGameMap::setVersion(EGameVersion newVersion)
{
    _m_pImpl->setVersion(newVersion);
}

VA(0x0042a1c4, 0x11)
void TGameMap::save(streambuf* pStreamBuf) const
{
    _m_pImpl->save(pStreamBuf);
}

VA(0x0042a1d5, 0x11)
void TGameMap::exportText(ostream* pOStream) const
{
    _m_pImpl->exportText(pOStream);
}

VA(0x0042a1e6, 0x6)
EGameVersion TGameMap::getVersion() const
{
    return _m_pImpl->getVersion();
}

VA(0x0042a1ec, 0xd)
unsigned int TGameMap::getWidth() const
{
    return _m_pImpl->getWidth();
}

unsigned int TGameMap::getHeight() const
{
    return _m_pImpl->getHeight();
}

VA(0x0042a1f9, 0x6)
bool TGameMap::isTwoLayer() const
{
    return _m_pImpl->isTwoLayer();
}

VA(0x0042a1ff, 0x15)
const TGameObject* TGameMap::getPObject(bool bSecondLayer, unsigned int objID) const
{
    return _m_pImpl->getPObject(bSecondLayer, objID);
}

VA(0x0042a214, 0x34)
TTilePoint TGameMap::getObjectLoc(bool bSecondLayer, unsigned int objID) const
{
    return _m_pImpl->getObjectLoc(bSecondLayer, objID);
}

VA(0x0042a248, 0x35)
TMapObjectRef TGameMap::getLinkableObjectRef(int linkID) const
{
    return _m_pImpl->getLinkableObjectRef(linkID);
}

VA(0x0042a27d, 0x11)
const TLinkableObject* TGameMap::getPLinkableObject(int linkID) const
{
    return _m_pImpl->getPLinkableObject(linkID);
}

VA(0x0042a28e, 0xf)
const TGameMap::TLayer* TGameMap::getPLayer(unsigned int num) const
{
    return _m_pImpl->getPLayer(num);
}

VA(0x0042a29d, 0x9)
const string& TGameMap::getName() const
{
    return _m_pImpl->getName();
}

VA(0x0042a2a6, 0x9)
const string& TGameMap::getDesc() const
{
    return _m_pImpl->getDesc();
}

VA(0x0042a2af, 0x9)
TGameMap::TDifficulty TGameMap::getDifficulty() const
{
    return _m_pImpl->getDifficulty();
}

VA(0x0042a2b8, 0x9)
unsigned int TGameMap::getMaxHeroLevel() const
{
    return _m_pImpl->getMaxHeroLevel();
}

VA(0x0042a2c1, 0xc)
const TArray<TPlayerInfo, kNumPlayers>& TGameMap::getPlayers() const
{
    return _m_pImpl->getPlayers();
}

VA(0x0042a2cd, 0xc)
const TTeamInfo& TGameMap::getTeamInfo() const
{
    return _m_pImpl->getTeamInfo();
}

VA(0x0042a2d9, 0xc)
const vector<TRumor>& TGameMap::getRumors() const
{
    return _m_pImpl->getRumors();
}

VA(0x0042a2e5, 0xc)
const vector<TTimedEvent>& TGameMap::getTimedEvents() const
{
    return _m_pImpl->getTimedEvents();
}

VA(0x0042a2f1, 0xc)
const TVictoryCondition* TGameMap::getPVictoryCondition() const
{
    return _m_pImpl->getPVictoryCondition();
}

VA(0x0042a2fd, 0xc)
const TLossCondition* TGameMap::getPLossCondition() const
{
    return _m_pImpl->getPLossCondition();
}

VA(0x0042a309, 0x9)
const bitset<kNumHeroes>& TGameMap::getDisabledHeroes() const
{
    return _m_pImpl->getDisabledHeroes();
}

VA(0x0042a312, 0x9)
const bitset<kNumArtifacts>& TGameMap::getDisabledArtifacts() const
{
    return _m_pImpl->getDisabledArtifacts();
}

VA(0x0042a31b, 0x9)
const bitset<kNumSpells>& TGameMap::getDisabledSpells() const
{
    return _m_pImpl->getDisabledSpells();
}

VA(0x0042a324, 0x9)
const bitset<kNumSecSkills>& TGameMap::getDisabledSkills() const
{
    return _m_pImpl->getDisabledSkills();
}

VA(0x0042a32d, 0x13)
const THeroPrototype& TGameMap::getHeroPrototype(THeroID heroID) const
{
    return _m_pImpl->getHeroPrototype(heroID);
}

VA(0x0042a340, 0x16)
bool TGameMap::isPlayerPresent(TPlayer player) const
{
    return _m_pImpl->isPlayerPresent(player);
}

VA(0x0042a356, 0x9)
unsigned int TGameMap::getNumPlayableSlots() const
{
    return _m_pImpl->getNumPlayableSlots();
}

VA(0x0042a3d7, 0x1a)
bitset<kNumHeroes> TGameMap::getHeroesOnMap() const
{
    return _m_pImpl->getHeroesOnMap();
}

VA(0x0042a40e, 0x10)
const set<TMapObjectRef>& TGameMap::getPlayerTownRefs(TPlayer player) const
{
    return _m_pImpl->getPlayerTownRefs(player);
}

VA(0x0042a41e, 0x9)
unsigned int TGameMap::getNumTownsOnMap() const
{
    return _m_pImpl->getNumTownsOnMap();
}

VA(0x0042a427, 0x9)
bool TGameMap::isGrailOnMap() const
{
    return _m_pImpl->isGrailOnMap();
}

VA(0x0042a430, 0xa)
unsigned int TGameMap::getNumObelisksOnMap() const
{
    return _m_pImpl->getNumObelisksOnMap();
}

VA(0x0042a43a, 0x29)
bool TGameMap::isValidPlacement(const TGameObject& obj, bool bSecondLayer, unsigned int x, unsigned int y) const
{
    return _m_pImpl->isValidPlacement(obj, bSecondLayer, x, y);
}

VA(0x0042a463, 0x29)
TPlayerInfo::TTownTypes TGameMap::getDefaultTownTypes(TPlayer player) const
{
    return _m_pImpl->getDefaultTownTypes(player);
}

DATA(0x00535214)
const unsigned int TGameMap::_TImpl::_s_akDimension[TGameMap::s_kNumSizes] = { 36, 72, 108, 144 };

DATA(0x00535228)
const TMapLayerObjectID TGameMap::TLayer::s_kInvalidObjID = 0;

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

VA(0x0042a93e, 0x19a)
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

bool TGameMap::TLayer::isObjectIDValid(unsigned int objID) const
{
    return _m_pImpl->isObjectIDValid(objID);
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

inline TMapLayerObjectID TGameMap::TLayer::_findObject(const TTilePoint& loc,
                                                       bool (*pfnPredicate)(const TGameObject&)) const
{
    return _m_pImpl->_findObject(loc, pfnPredicate);
}

inline void TGameMap::TLayer::_removeObject(unsigned int objID)
{
    _m_pImpl->_removeObject(objID);
}

inline void TGameMap::TLayer::_floatObject(unsigned int objID)
{
    _m_pImpl->_floatObject(objID);
}

inline void TGameMap::TLayer::_unfloatObject(const TTilePoint& loc)
{
    _m_pImpl->_unfloatObject(loc);
}

// The capped-type map's and info vector's template members, emitted here; the
// vector's fill insert folds with the RMG's TRmgMapPosition copy (0x430b35).
VA_COMPGEN(0x0042c28f, 0xa4, VECTOR_RESERVE, TCappedObjectTypeInfo)
VA_COMPGEN(0x0042db3f, 0xf0, TREE_INSERT, TCappedObjectTypeInfo)
VA_COMPGEN(0x00430930, 0x89, TREE_INIT, TCappedObjectTypeInfo)
