// GameMap.cpp - Loki h3maped object 12: the map model. TGameMap and
// TGameMap::TLayer forward to their copy-on-write implementations; this
// file defines both implementations, the cell, the rumors, players and
// teams, the object bookkeeping and the binary and text forms.
//
// Reconstruction in progress: the layer handle and the cell come first.
#include "editor/stdafx.h"

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <algorithm>
#include <functional>
#include <iostream.h>
#include <limits>
#include <numeric>
#include <map>
#include <memory>

#include "adventureobjecttype.h"
#include "editor/GameMap.h"
#include "editor/GameObject.h"
#include "editor/Hero.h"
#include "editor/Monster.h"
#include "editor/ObjectSpecializations.h"
#include "editor/Town.h"
#include "editor/MapEditorText.h"
#include "editor/RawStream.h"
#include "editor/TilePoint.h"
#include "editor/TimedEvent.h"
#include "editor/VictoryCondition.h"

namespace {

inline bool isAllSpace(const string& text)
{
    return find_if(text.begin(), text.end(), not1(ptr_fun(isspace))) == text.end();
}

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

// The height of each placed cell of an object: an underlay lies at 0,
// anything else rises by one per row from its front, and a passable cell
// that continues a blocked one to its left takes that cell's height.
void copyCustomizations(const THero& hero, THero* pNewHero)
{
    pNewHero->setBCustomName(hero.getBCustomName());
    pNewHero->setBCustomPortrait(hero.getBCustomPortrait());
    pNewHero->setBCustomArmy(hero.getBCustomArmy());
    pNewHero->setBCustomSecondarySkills(hero.getBCustomSecondarySkills());
    pNewHero->setBCustomArtifacts(hero.getBCustomArtifacts());
    pNewHero->setName(hero.getCustomName());
    pNewHero->setPortrait(hero.getCustomPortrait());
    pNewHero->setArmy(hero.getArmy());
    pNewHero->setSecondarySkills(hero.getCustomSecondarySkills());
    pNewHero->setArtifacts(hero.getCustomArtifacts());
    pNewHero->setExperience(hero.getExperience());
    pNewHero->setBGroupedFormation(hero.getBGroupedFormation());
    pNewHero->setPatrol(hero.getPatrol());
}

void constructObjectHeightMap(const TGameObject& obj, unsigned int (&heightMap)[TObjectType::kMaxObjWidth][TObjectType::kMaxObjHeight])
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

// The object types the map caps, each with its ordinal in the bookkeeping
// and its cap.
struct TCappedObjectTypeInfo {
    TCappedObjectTypeInfo(unsigned int ordinal, unsigned int cap) : m_ordinal(ordinal), m_cap(cap) {}

    unsigned int m_ordinal;
    unsigned int m_cap;
};

class TCappedObjectTypeInfoMap : public map<int, TCappedObjectTypeInfo> {
public:
    TCappedObjectTypeInfoMap();
};

// The object types the map caps and their caps; the table's index is the
// type's ordinal in the bookkeeping's counts.
struct TCappedObjectType {
    int m_type;
    unsigned int m_cap;
};

const TCappedObjectType akCappedObjectTypes[] = {
    { EVENT, 200 },
    { BLACK_BOX, 200 },
    { OBELISK, 48 },
    { BOAT, 64 },
    { GARRISON, 48 },
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
    { UNIVERSITY, 32 },
    { WITCH_HUT, 32 },
    { SHRINE1, 32 },
    { SHRINE2, 32 },
    { SHRINE3, 32 },
    { SIREN, 32 },
    { MYSTICAL_GARDEN, 32 },
    { WATER_WHEEL, 32 },
    { WINDMILL, 32 },
    { MAGIC_SPRING, 32 },
    { DEAD_GUY, 32 },
    { LEAN_TO, 32 },
    { WARRIOR_TOMB, 32 },
    { WAGON, 32 },
    { SEER, 48 },
    { BLACK_MARKET, 32 },
};

TCappedObjectTypeInfoMap::TCappedObjectTypeInfoMap()
{
    for (unsigned int i = 0; i < sizeof(akCappedObjectTypes) / sizeof(akCappedObjectTypes[0]); i++)
        insert(value_type(akCappedObjectTypes[i].m_type, TCappedObjectTypeInfo(i, akCappedObjectTypes[i].m_cap)));
}

const TCappedObjectTypeInfoMap kCappedObjectTypeInfoMap;

inline bool isHero(const TGameObject& obj)
{
    return dynamic_cast<const THero*>(&obj) != NULL;
}

inline bool isTown(const TGameObject& obj)
{
    return dynamic_cast<const TTown*>(&obj) != NULL;
}

inline bool isMonster(const TGameObject& obj)
{
    return dynamic_cast<const TMonster*>(&obj) != NULL;
}

inline bool isHeroOrTown(const TGameObject& obj)
{
    return isHero(obj) || isTown(obj);
}

inline bool isArtifact(const TGameObject& obj)
{
    int type = obj.getType();
    return type == ARTIFACT || type == SPELL_SCROLL || type == BLACK_BOX;
}

struct TVictoryConditionData;
struct TLossConditionData;

}  // namespace

class TGameMap::_TImpl {
public:
    static const unsigned int s_kMaxHeroesOnMap = 128;
    static const unsigned int s_kMaxHeroesPerPlayer = 8;
    static const unsigned int s_kMaxTownsOnMap = 48;
    static const unsigned int s_kMaxMinesOnMap = 144;
    static const unsigned int s_kMaxGeneratorsOnMap = 144;
    static const unsigned int s_kMaxSignsOnMap = 128;
    static const unsigned int s_kMaxNameLen = 30;
    static const unsigned int s_kMaxDescLen = 300;
    static const unsigned int s_kMaxRumors = 30;
    static const unsigned int s_kMaxTimedEvents = 50;

    static const unsigned int _s_akDimension[TGameMap::s_kNumSizes];

    static void streamObject(streambuf* pStreamBuf, const TGameObject& obj);

    _TImpl(TClient* pClient, const TObjectFactory* pObjectFactory, TSize size, bool bTwoLayer);
    _TImpl(TClient* pClient, const TObjectFactory* pObjectFactory, streambuf* pStreamBuf, int version);
    ~_TImpl();

    void importText(istream* pIStream);
    void exportText(ostream* pOStream) const;
    void save(streambuf* pStreamBuf) const;

    unsigned int getWidth() const { return _s_akDimension[_m_size]; }
    unsigned int getHeight() const { return _s_akDimension[_m_size]; }
    bool isTwoLayer() const { return _m_bTwoLayer; }
    void removeSecondLayer();
    void addSecondLayer();
    TLayer* getPLayer(unsigned int num);
    TLayer* getPLayer(bool bSecondLayer) { return getPLayer(bSecondLayer ? 1U : 0U); }
    const TLayer* getPLayer(unsigned int num) const;
    const TLayer* getPLayer(bool bSecondLayer) const { return getPLayer(bSecondLayer ? 1U : 0U); }
    const TLayer& getLayer(unsigned int num) const { return *getPLayer(num); }
    const TLayer& getLayer(bool bSecondLayer) const { return *getPLayer(bSecondLayer); }

    const string& getName() const { return _m_pProperties->m_name; }
    void setName(const string& newName);
    const string& getDesc() const { return _m_pProperties->m_desc; }
    void setDesc(const string& newDesc);
    TDifficulty getDifficulty() const { return _m_pProperties->m_difficulty; }
    void setDifficulty(TDifficulty newDifficulty);
    const TArray<TPlayerInfo, kNumPlayers>& getPlayers() const { return _m_pProperties->m_players; }
    void setPlayers(const TArray<TPlayerInfo, kNumPlayers>& newPlayers);
    const TTeamInfo& getTeamInfo() const { return _m_pProperties->m_teamInfo; }
    void setTeamInfo(const TTeamInfo& newTeamInfo);
    const vector<TRumor>& getRumors() const { return _m_pProperties->m_rumors; }
    void setRumors(const vector<TRumor>& newRumors);
    const vector<TTimedEvent>& getTimedEvents() const { return _m_pProperties->m_timedEvents; }
    void setTimedEvents(const vector<TTimedEvent>& newTimedEvents);
    const TVictoryCondition* getPVictoryCondition() const { return _m_pProperties->m_pVictoryCondition; }
    void setVictoryCondition(const TVictoryCondition* pNewVictoryCondition);
    const TLossCondition* getPLossCondition() const { return _m_pProperties->m_pLossCondition; }
    void setLossCondition(const TLossCondition* pNewLossCondition);
    bool isPlayerPresent(TPlayer player) const { return _m_apPlayerBookkeeping[player]->m_numUnits != 0; }
    unsigned int getNumPlayableSlots() const { return _m_pBookkeeping->m_numPlayableSlots; }

    TMapLayerObjectID placeObject(bool bSecondLayer, const TGameObject& obj, unsigned int x, unsigned int y,
                                  TTileExtent* pUpdatedExtent);
    void removeObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent);
    void floatObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent);
    void unfloatObject(bool bSecondLayer, unsigned int x, unsigned int y, TTileExtent* pUpdatedExtent);
    void removeFloatingObject(bool bSecondLayer);
    bool isValidPlacement(const TGameObject& obj, bool bSecondLayer, unsigned int x, unsigned int y) const
    {
        return _isValidPlacement(getLayer(bSecondLayer), obj, x, y);
    }
    const TGameObject* getPObject(bool bSecondLayer, unsigned int objID) const;
    const TGameObject* getPObject(const TMapObjectRef& objRef) const
    {
        return getPObject(objRef.getBSecondLayer(), objRef.getObjectID());
    }
    TTilePoint getObjectLoc(bool bSecondLayer, unsigned int objID) const;

    bool onTerrainTypeChanged(bool bSecondLayer, unsigned int x, unsigned int y, TTerrainType oldTerrainType,
                              TTileExtent* pUpdatedExtent);
    void onObjectRemoved();
    void onHeroAdded(const THero& hero);
    void onRemovingHero(const THero& hero);
    void onHeroProtoChanged(THeroClass heroClass, unsigned int oldProtoNum, unsigned int newProtoNum);
    void onHeroClassChanged(THeroClass oldHeroClass, unsigned int oldProtoNum, THeroClass newHeroClass,
                            unsigned int newProtoNum);
    void onHeroOwnerChanged(const THero& hero, TPlayer oldOwner);
    void onTownOwnerChanged(const TTown& town, bool bSecondLayer, unsigned int objID, TPlayer oldOwner);

    TGameObject* createObject(const TObjectType& objType, TPlayer player, void* (*pfnAllocator)(unsigned int)) const;
    TGameObject* reconstructObject(streambuf* pStreamBuf, int version, void* (*pfnAllocator)(unsigned int)) const;
    bool canCreate(const TObjectType& objType, TPlayer player) const;
    bool isHeroOnMap(THeroID heroID) const;
    set<unsigned int> getAvailableHeroesInClass(THeroClass heroClass) const;
    bitset<kNumPlayers> getAvailableHeroOwnersMask() const;
    const set<TMapObjectRef>& getPlayerTownRefs(TPlayer player) const;
    unsigned int getNumTownsOnMap() const { return _m_pBookkeeping->m_numTowns; }
    bool isGrailOnMap() const { return _m_pBookkeeping->m_bGrailPlaced; }
    unsigned int getNumObelisksOnMap() const;

private:
    class _TVictoryConditionValidater : public TVictoryCondition::TVisitor {
    public:
        _TVictoryConditionValidater(const _TImpl& map) : _m_map(map), _m_bValid(false) {}

        bool isValid() const { return _m_bValid; }

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
        bool _isArtifact(const TMapObjectRef& objRef) const;
        bool _isTown(const TMapObjectRef& objRef) const;

        const _TImpl& _m_map;
        bool _m_bValid;
    };

    class _TLossConditionValidater : public TLossCondition::TVisitor {
    public:
        _TLossConditionValidater(const _TImpl& map) : _m_map(map), _m_bValid(false) {}

        bool isValid() const { return _m_bValid; }

        virtual void visit(const TLCLoseTown& lc);
        virtual void visit(const TLCLoseHero& lc);
        virtual void visit(const TLCTimeExpires& lc);

    private:
        const _TImpl& _m_map;
        bool _m_bValid;
    };

    class _TVictoryConditionWriter : public TVictoryCondition::TVisitor {
    public:
        _TVictoryConditionWriter(const _TImpl& map, TRawOStream* pOStream) : _m_map(map), _m_pOStream(pOStream) {}

        virtual void visit(const TVCAquireArtifact& vc) { _m_map._write(_m_pOStream, vc); }
        virtual void visit(const TVCAccumulateCreature& vc) { _m_map._write(_m_pOStream, vc); }
        virtual void visit(const TVCAccumulateResource& vc) { _m_map._write(_m_pOStream, vc); }
        virtual void visit(const TVCUpgradeTown& vc) { _m_map._write(_m_pOStream, vc); }
        virtual void visit(const TVCBuildHolyGrailStruct& vc) { _m_map._write(_m_pOStream, vc); }
        virtual void visit(const TVCDefeatHero& vc) { _m_map._write(_m_pOStream, vc); }
        virtual void visit(const TVCCaptureTown& vc) { _m_map._write(_m_pOStream, vc); }
        virtual void visit(const TVCDefeatMonster& vc) { _m_map._write(_m_pOStream, vc); }
        virtual void visit(const TVCFlagAllCreatureGenerators& vc) { _m_map._write(_m_pOStream, vc); }
        virtual void visit(const TVCFlagAllMines& vc) { _m_map._write(_m_pOStream, vc); }
        virtual void visit(const TVCTransportArtifact& vc) { _m_map._write(_m_pOStream, vc); }

    private:
        const _TImpl& _m_map;
        TRawOStream* _m_pOStream;
    };

    class _TLossConditionWriter : public TLossCondition::TVisitor {
    public:
        _TLossConditionWriter(const _TImpl& map, TRawOStream* pOStream) : _m_map(map), _m_pOStream(pOStream) {}

        virtual void visit(const TLCLoseTown& lc) { _m_map._write(_m_pOStream, lc); }
        virtual void visit(const TLCLoseHero& lc) { _m_map._write(_m_pOStream, lc); }
        virtual void visit(const TLCTimeExpires& lc) { _m_map._write(_m_pOStream, lc); }

    private:
        const _TImpl& _m_map;
        TRawOStream* _m_pOStream;
    };

    friend class _TVictoryConditionValidater;
    friend class _TLossConditionValidater;
    friend class _TVictoryConditionWriter;
    friend class _TLossConditionWriter;

    bool _isValid(const TVictoryCondition& vc) const;
    bool _isValid(const TLossCondition& lc) const;
    void _write(TRawOStream* pOStream, const TMapObjectRef& objRef) const;
    void _writeVictoryCondition(TRawOStream* pOStream) const;
    void _write(TRawOStream* pOStream, const TVictoryCondition& vc, int type) const;
    void _write(TRawOStream* pOStream, const TVCAquireArtifact& vc) const;
    void _write(TRawOStream* pOStream, const TVCAccumulateCreature& vc) const;
    void _write(TRawOStream* pOStream, const TVCAccumulateResource& vc) const;
    void _write(TRawOStream* pOStream, const TVCUpgradeTown& vc) const;
    void _write(TRawOStream* pOStream, const TVCBuildHolyGrailStruct& vc) const;
    void _write(TRawOStream* pOStream, const TVCDefeatHero& vc) const;
    void _write(TRawOStream* pOStream, const TVCCaptureTown& vc) const;
    void _write(TRawOStream* pOStream, const TVCDefeatMonster& vc) const;
    void _write(TRawOStream* pOStream, const TVCFlagAllCreatureGenerators& vc) const;
    void _write(TRawOStream* pOStream, const TVCFlagAllMines& vc) const;
    void _write(TRawOStream* pOStream, const TVCTransportArtifact& vc) const;
    void _writeLossCondition(TRawOStream* pOStream) const;
    void _write(TRawOStream* pOStream, const TLCLoseTown& lc) const;
    void _write(TRawOStream* pOStream, const TLCLoseHero& lc) const;
    void _write(TRawOStream* pOStream, const TLCTimeExpires& lc) const;

    TGameObject* _createObject(const TObjectType& objType, TRawIStream* pIStream, int version,
                               void* (*pfnAllocator)(unsigned int)) const;
    TMapLayerObjectID _placeGeneralObject(bool bSecondLayer, const TGameObject& obj, unsigned int x,
                                          unsigned int y, TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeNonRandomHero(bool bSecondLayer, const TNonRandomHero& hero, unsigned int x,
                                          unsigned int y, TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placePrison(bool bSecondLayer, const TPrison& prison, unsigned int x, unsigned int y,
                                   TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeHero(bool bSecondLayer, const THero& hero, unsigned int x, unsigned int y,
                                 TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeTown(bool bSecondLayer, const TTown& town, unsigned int x, unsigned int y,
                                 TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeHolyGrail(bool bSecondLayer, const THolyGrail& holyGrail, unsigned int x,
                                      unsigned int y, TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeMine(bool bSecondLayer, const TMine& mine, unsigned int x, unsigned int y,
                                 TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeGenerator(bool bSecondLayer, const TGenerator& generator, unsigned int x,
                                      unsigned int y, TTileExtent* pUpdatedExtent);
    TMapLayerObjectID _placeSign(bool bSecondLayer, const TSign& sign, unsigned int x, unsigned int y,
                                 TTileExtent* pUpdatedExtent);
    void _removeObjectHelper(bool bSecondLayer, unsigned int objID);
    bool _isHeroAvailable(THeroClass heroClass, unsigned int protoNum) const;
    unsigned int _pickAvailableHero(THeroClass heroClass) const;
    unsigned int _pickAvailableTeam() const;
    void _onPlayableAdded(const TPlayableObject& playable);
    void _onRemovingPlayable(const TPlayableObject& playable);
    void _onTownAdded(const TTown& town, bool bSecondLayer, unsigned int objID);
    void _onRemovingTown(const TTown& town, bool bSecondLayer, unsigned int objID);
    void _onGeneralObjectAdded(const TGameObject& obj);
    void _onRemovingGeneralObject(const TGameObject& obj);
    void _onHolyGrailAdded(const THolyGrail& holyGrail);
    void _onRemovingHolyGrail(const THolyGrail& holyGrail);
    void _onMineAdded(const TMine& mine);
    void _onRemovingMine(const TMine& mine);
    void _onGeneratorAdded(const TGenerator& generator);
    void _onRemovingGenerator(const TGenerator& generator);
    void _onSignAdded(const TSign& sign);
    void _onRemovingSign(const TSign& sign);
    bool _isMapPlayable() const;
    TMapLayerObjectID _findObject(bool bSecondLayer, const TTilePoint& loc,
                                  bool (*pfnPredicate)(const TGameObject&)) const;
    const TNonRandomHero* _findPlayersNonRandomHero(TPlayer player) const;
    TRawIStream& readContainer(TRawIStream& stream, vector<TRumor>& aRumor);
    TRawIStream& readContainer(TRawIStream& stream, vector<TTimedEvent>& aTimedEvent);

    struct _TProperties {
        _TProperties()
            : m_difficulty(TDifficulty(1)), m_pVictoryCondition(NULL), m_pLossCondition(NULL) {}
        _TProperties(const _TProperties& other)
            : m_name(other.m_name), m_desc(other.m_desc), m_difficulty(other.m_difficulty),
              m_players(other.m_players), m_teamInfo(other.m_teamInfo), m_rumors(other.m_rumors),
              m_timedEvents(other.m_timedEvents), m_pVictoryCondition(NULL), m_pLossCondition(NULL)
        {
            if (other.m_pVictoryCondition != NULL)
                if ((m_pVictoryCondition = TVictoryCondition::clone(*other.m_pVictoryCondition, ::operator new)) == NULL)
#line 926
                    throw TAllocationFailure(__FILE__, __LINE__);
            if (other.m_pLossCondition != NULL)
                if ((m_pLossCondition = TLossCondition::clone(*other.m_pLossCondition, ::operator new)) == NULL)
#line 929
                    throw TAllocationFailure(__FILE__, __LINE__);
        }
        ~_TProperties()
        {
            assert(m_pVictoryCondition == __null && m_pLossCondition == __null);
        }

        string m_name;
        string m_desc;
        TDifficulty m_difficulty;
        TArray<TPlayerInfo, kNumPlayers> m_players;
        TTeamInfo m_teamInfo;
        vector<TRumor> m_rumors;
        vector<TTimedEvent> m_timedEvents;
        TVictoryCondition* m_pVictoryCondition;
        TLossCondition* m_pLossCondition;
    };

    struct _TBookkeeping {
        // Which prototypes of each hero class are still free to place.
        class TAABHeroAvailable : public TArray<bitset<8>, kNumHeroClasses> {
        public:
            TAABHeroAvailable();
        };

        _TBookkeeping()
            : m_numPlayableSlots(0), m_numTowns(0), m_numHeroes(0), m_bGrailPlaced(false), m_numMines(0),
              m_numGenerators(0), m_numSigns(0), m_aNumObjsOfCappedType(kCappedObjectTypeInfoMap.size(), 0U) {}

        unsigned int m_numPlayableSlots;
        unsigned int m_numTowns;
        unsigned int m_numHeroes;
        bool m_bGrailPlaced;
        unsigned int m_numMines;
        unsigned int m_numGenerators;
        unsigned int m_numSigns;
        vector<unsigned int> m_aNumObjsOfCappedType;
        TAABHeroAvailable m_aabHeroAvailable;
    };

    struct _TPlayerBookkeeping {
        _TPlayerBookkeeping()
            : m_numUnits(0), m_numRandomTowns(0), m_aNumTownsOfType(0), m_numHeroes(0), m_numRandomHeroes(0),
              m_aNumHeroesOfType(0) {}

        unsigned int m_numUnits;
        set<TMapObjectRef> m_townRefs;
        unsigned int m_numRandomTowns;
        TArray<unsigned int, kNumTownTypes> m_aNumTownsOfType;
        unsigned int m_numHeroes;
        unsigned int m_numRandomHeroes;
        TArray<unsigned int, kNumTownTypes> m_aNumHeroesOfType;
    };

    static bool _isValidPlacement(const TLayer& layer, const TGameObject& obj, unsigned int x, unsigned int y);
    static bool _isValidShipyardPlacement(const TLayer& layer, const TGameObject& shipyard, unsigned int x,
                                          unsigned int y);

    typedef TRefCountingPtr<_TProperties> _TPProperties;
    typedef TRefCountingPtr<_TBookkeeping> _TPBookkeeping;
    typedef TRefCountingPtr<_TPlayerBookkeeping> _TPPlayerBookkeeping;

    TClient* _m_pClient;
    const TObjectFactory* _m_pObjectFactory;
    TSize _m_size;
    bool _m_bTwoLayer;
    _TPProperties _m_pProperties;
    vector<TLayer> _m_aLayer;
    _TPBookkeeping _m_pBookkeeping;
    TArray<_TPPlayerBookkeeping, kNumPlayers> _m_apPlayerBookkeeping;
};

namespace {

// An object's trigger cell as the map file records it.
struct TMapLoc {
    ubyte m_x;
    ubyte m_y;
    ubyte m_layer;
};

TRawIStream& operator>>(TRawIStream& stream, TMapLoc& loc)
{
    stream >> loc.m_x >> loc.m_y >> loc.m_layer;
    return stream;
}

// A victory condition as the map file stores it.
struct TVictoryConditionData {
    signed char m_type;
    signed char m_bAllowNormalVictory;
    signed char m_bAppliesToComputer;
    union {
        struct {
            signed char m_artifact;
        } m_aquireArtifact;
        struct {
            signed char m_creatureType;
            long m_quantity;
        } m_accumulateCreature;
        struct {
            signed char m_resourceType;
            long m_quantity;
        } m_accumulateResource;
        struct {
            TMapLoc m_townLoc;
            signed char m_hallLevel;
            signed char m_castleLevel;
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
            signed char m_artifact;
            TMapLoc m_townLoc;
        } m_transportArtifact;
    };
};

TRawIStream& operator>>(TRawIStream& stream, TVictoryConditionData& vcData)
{
    stream >> vcData.m_type;
#line 358
    assert(vcData.m_type >= eVCNone && vcData.m_type < kNumVictoryConditionTypes);
    if (vcData.m_type != eVCNone) {
        stream >> vcData.m_bAllowNormalVictory >> vcData.m_bAppliesToComputer;
        switch (vcData.m_type) {
        case eVCAquireArtifact:
            stream >> vcData.m_aquireArtifact.m_artifact;
            break;
        case eVCAccumulateCreature:
            stream >> vcData.m_accumulateCreature.m_creatureType >> vcData.m_accumulateCreature.m_quantity;
            break;
        case eVCAccumulateResource:
            stream >> vcData.m_accumulateResource.m_resourceType >> vcData.m_accumulateResource.m_quantity;
            break;
        case eVCUpgradeTown:
            stream >> vcData.m_upgradeTown.m_townLoc >> vcData.m_upgradeTown.m_hallLevel
                   >> vcData.m_upgradeTown.m_castleLevel;
            break;
        case eVCBuildHolyGrailStruct:
            stream >> vcData.m_buildHolyGrailStruct.m_townLoc;
            break;
        case eVCDefeatHero:
            stream >> vcData.m_defeatHero.m_heroLoc;
            break;
        case eVCCaptureTown:
            stream >> vcData.m_captureTown.m_townLoc;
            break;
        case eVCDefeatMonster:
            stream >> vcData.m_defeatMonster.m_monsterLoc;
            break;
        case eVCFlagAllCreatureGenerators:
        case eVCFlagAllMines:
            break;
        case eVCTransportArtifact:
            stream >> vcData.m_transportArtifact.m_artifact >> vcData.m_transportArtifact.m_townLoc;
            break;
        }
    }
    return stream;
}

// A loss condition as the map file stores it.
struct TLossConditionData {
    signed char m_type;
    union {
        TMapLoc m_townLoc;
        TMapLoc m_heroLoc;
        short m_numDays;
    };
};

TRawIStream& operator>>(TRawIStream& stream, TLossConditionData& lcData)
{
    stream >> lcData.m_type;
#line 433
    assert(lcData.m_type >= eLCNone && lcData.m_type < kNumLossConditionTypes);
    if (lcData.m_type != eLCNone) {
        switch (lcData.m_type) {
        case eLCLoseTown:
            stream >> lcData.m_townLoc;
            break;
        case eLCLoseHero:
            stream >> lcData.m_heroLoc;
            break;
        case eLCTimeExpires:
            stream >> lcData.m_numDays;
            break;
        }
    }
    return stream;
}

}  // namespace

void TRumor::setNameAndText(const string& newName, const string& newText)
{
#line 469
    assert(( newName.empty() && newText.empty() ) || ( !isAllSpace( newName ) && !isAllSpace( newText ) ));
    assert(newName.find( '\n' ) == std::string::npos);
    assert(newName.find( '\t' ) == std::string::npos);
    assert(newText.size() <= s_kMaxTextLen);
    assert(newText.find( '\t' ) == std::string::npos);
    _m_name = newName;
    _m_text = newText;
}

void TRumor::importText(istream* pIStream)
{
#line 484
    assert(pIStream != NULL);
    string line;
    getline(*pIStream, line);
    if (line != string(kNameStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    replace(line.begin(), line.end(), '\t', ' ');
    string name = line;
    getline(*pIStream, line);
    if (line != string(kTextStr) + ':')
        throw TImportTextFailure();
    getline(*pIStream, line);
    replace(line.begin(), line.end(), '\t', '\n');
    if (line.size() > s_kMaxTextLen)
        line.erase(s_kMaxTextLen);
    string text = line;
    if (isAllSpace(name) || isAllSpace(text))
        throw TImportTextFailure();
    setNameAndText(name, text);
}

void TRumor::exportText(ostream* pOStream) const
{
#line 515
    assert(pOStream != NULL);
    assert(!isAllSpace( _m_name ) && !isAllSpace( _m_text ));
    *pOStream << kNameStr << ':' << '\n' << _m_name << '\n';
    string text = _m_text;
    replace(text.begin(), text.end(), '\n', '\t');
    *pOStream << kTextStr << ':' << '\n' << text << '\n';
}

TRawIStream& operator>>(TRawIStream& stream, TRumor& rumor)
{
    string name;
    string text;
    stream >> name >> text;
    if (text.size() > TRumor::s_kMaxTextLen)
        text.erase(TRumor::s_kMaxTextLen);
    rumor.setNameAndText(name, text);
    return stream;
}

TRawOStream& operator<<(TRawOStream& stream, const TRumor& rumor)
{
    stream << rumor.getName() << rumor.getText();
    return stream;
}

void TPlayerInfo::setBehaviorType(TBehaviorType newBehaviorType)
{
#line 551
    assert(newBehaviorType >= 0 && newBehaviorType < s_kNumBehaviorTypes);
    _m_behaviorType = newBehaviorType;
}

void TTeamInfo::setNumTeams(unsigned int newNumTeams)
{
#line 565
    assert(_m_bHasTeams);
    assert(newNumTeams >= s_kMinTeams && newNumTeams <= s_kMaxTeams);
    _m_numTeams = newNumTeams;
}

void TTeamInfo::setPlayerTeam(TPlayer player, unsigned int newTeam)
{
#line 574
    assert(_m_bHasTeams);
    assert(player >= 0 && player < kNumPlayers);
    assert(newTeam < _m_numTeams);
    _m_aPlayerTeam[player] = newTeam;
}

unsigned int TTeamInfo::getPlayerTeam(TPlayer player) const
{
#line 584
    assert(_m_bHasTeams);
    assert(player >= 0 && player < kNumPlayers);
    return _m_aPlayerTeam[player];
}

TRawIStream& operator>>(TRawIStream& stream, TTeamInfo& teamInfo)
{
    signed char numTeams;
    stream >> numTeams;
    if (numTeams == 0) {
        teamInfo._m_bHasTeams = false;
        teamInfo._m_numTeams = 0;
        fill(teamInfo._m_aPlayerTeam.begin(), teamInfo._m_aPlayerTeam.end(), 0U);
    } else {
#line 606
        assert(numTeams >= TTeamInfo::s_kMinTeams && numTeams <= TTeamInfo::s_kMaxTeams);
        teamInfo._m_bHasTeams = true;
        teamInfo._m_numTeams = numTeams;
        for (unsigned int player = 0; player < kNumPlayers; player++) {
            signed char playerTeam;
            stream >> playerTeam;
#line 615
            assert(playerTeam >= 0 && playerTeam < numTeams);
            teamInfo._m_aPlayerTeam[player] = playerTeam;
        }
    }
    return stream;
}

TRawOStream& operator<<(TRawOStream& stream, const TTeamInfo& teamInfo)
{
    if (!teamInfo._m_bHasTeams) {
        stream << static_cast<signed char>(0);
    } else {
#line 630
        assert(teamInfo._m_numTeams >= TTeamInfo::s_kMinTeams && teamInfo._m_numTeams <= TTeamInfo::s_kMaxTeams);
        stream << reinterpret_cast<const signed char&>(teamInfo._m_numTeams);
        for (unsigned int player = 0; player < kNumPlayers; player++)
            stream << reinterpret_cast<const signed char&>(teamInfo._m_aPlayerTeam[player]);
    }
    return stream;
}

TCreateObjFailureTooManyHeroesOnMap::TCreateObjFailureTooManyHeroesOnMap()
    : TCreateObjFailureTooManyInstancesOfTypeOnMap(HERO, TGameMap::_TImpl::s_kMaxHeroesOnMap)
{
}

TCreateObjFailureTooManyTownsOnMap::TCreateObjFailureTooManyTownsOnMap()
    : TCreateObjFailureTooManyInstancesOfTypeOnMap(TOWN, TGameMap::_TImpl::s_kMaxTownsOnMap)
{
}

TCreateObjFailureTooManyMinesOnMap::TCreateObjFailureTooManyMinesOnMap()
    : TCreateObjFailureTooManyInstancesOfTypeOnMap(MINE, TGameMap::_TImpl::s_kMaxMinesOnMap)
{
}

TCreateObjFailureTooManyGeneratorsOnMap::TCreateObjFailureTooManyGeneratorsOnMap()
    : TCreateObjFailureTooManyInstancesOfTypeOnMap(CREATURE_GENERATOR_1, TGameMap::_TImpl::s_kMaxGeneratorsOnMap)
{
}

TCreateObjFailureTooManySignsOnMap::TCreateObjFailureTooManySignsOnMap()
    : TCreateObjFailureTooManyInstancesOfTypeOnMap(SIGN, TGameMap::_TImpl::s_kMaxSignsOnMap)
{
}

TPlaceObjFailureTooManyHeroesOnMap::TPlaceObjFailureTooManyHeroesOnMap()
    : TPlaceObjFailureTooManyInstancesOfTypeOnMap(HERO, TGameMap::_TImpl::s_kMaxHeroesOnMap)
{
}

TPlaceObjFailureTooManyTownsOnMap::TPlaceObjFailureTooManyTownsOnMap()
    : TPlaceObjFailureTooManyInstancesOfTypeOnMap(TOWN, TGameMap::_TImpl::s_kMaxTownsOnMap)
{
}

TPlaceObjFailureTooManyMinesOnMap::TPlaceObjFailureTooManyMinesOnMap()
    : TPlaceObjFailureTooManyInstancesOfTypeOnMap(MINE, TGameMap::_TImpl::s_kMaxMinesOnMap)
{
}

TPlaceObjFailureTooManyGeneratorsOnMap::TPlaceObjFailureTooManyGeneratorsOnMap()
    : TPlaceObjFailureTooManyInstancesOfTypeOnMap(CREATURE_GENERATOR_1, TGameMap::_TImpl::s_kMaxGeneratorsOnMap)
{
}

TPlaceObjFailureTooManySignsOnMap::TPlaceObjFailureTooManySignsOnMap()
    : TPlaceObjFailureTooManyInstancesOfTypeOnMap(SIGN, TGameMap::_TImpl::s_kMaxSignsOnMap)
{
}

void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCAquireArtifact& vc)
{
#line 1278
    assert(vc.getArtifact() >= 0 && vc.getArtifact() < kNumArtifacts);
    _m_bValid = true;
}

void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCAccumulateCreature& vc)
{
#line 1285
    assert(vc.getCreatureType() >= 0 && vc.getCreatureType() < kNumCreatureTypes);
    _m_bValid = true;
}

void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCAccumulateResource& vc)
{
#line 1292
    assert(vc.getResourceType() >= 0 && vc.getResourceType() < kNumGameResourceTypes);
    _m_bValid = true;
}

void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCUpgradeTown& vc)
{
    _m_bValid = _isTown(vc.getTownRef());
}

void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCBuildHolyGrailStruct& vc)
{
    _m_bValid = vc.getTownRef() == TMapObjectRef() || _isTown(vc.getTownRef());
}

void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCDefeatHero& vc)
{
    const TGameObject* pObj = _m_map.getPObject(vc.getHeroRef());
    if (pObj != NULL) {
        if (isHero(*pObj)) {
            _m_bValid = true;
        } else {
            const TTown* pTown = dynamic_cast<const TTown*>(pObj);
            _m_bValid = pTown != NULL && pTown->getPVisitingHero() != NULL;
        }
    } else {
        _m_bValid = false;
    }
}

void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCCaptureTown& vc)
{
    _m_bValid = _isTown(vc.getTownRef());
}

void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCDefeatMonster& vc)
{
    const TGameObject* pObj = _m_map.getPObject(vc.getMonsterRef());
    _m_bValid = pObj != NULL && isMonster(*pObj);
}

void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCFlagAllCreatureGenerators& vc)
{
    _m_bValid = true;
}

void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCFlagAllMines& vc)
{
    _m_bValid = true;
}

void TGameMap::_TImpl::_TVictoryConditionValidater::visit(const TVCTransportArtifact& vc)
{
#line 1354
    assert(vc.getArtifact() >= 0 && vc.getArtifact() < kNumArtifacts);
    _m_bValid = _isTown(vc.getTownRef());
}

bool TGameMap::_TImpl::_TVictoryConditionValidater::_isArtifact(const TMapObjectRef& objRef) const
{
    const TGameObject* pObj = _m_map.getPObject(objRef);
    return pObj != NULL && isArtifact(*pObj);
}

bool TGameMap::_TImpl::_TVictoryConditionValidater::_isTown(const TMapObjectRef& objRef) const
{
    const TGameObject* pObj = _m_map.getPObject(objRef);
    return pObj != NULL && isTown(*pObj);
}

void TGameMap::_TImpl::_TLossConditionValidater::visit(const TLCLoseTown& lc)
{
    const TGameObject* pObj = _m_map.getPObject(lc.getTownRef());
    _m_bValid = pObj != NULL && dynamic_cast<const TTown*>(pObj) != NULL;
}

void TGameMap::_TImpl::_TLossConditionValidater::visit(const TLCLoseHero& lc)
{
    const TGameObject* pObj = _m_map.getPObject(lc.getHeroRef());
    if (pObj != NULL) {
        if (isHero(*pObj)) {
            _m_bValid = true;
        } else {
            const TTown* pTown = dynamic_cast<const TTown*>(pObj);
            _m_bValid = pTown != NULL && pTown->getPVisitingHero() != NULL;
        }
    } else {
        _m_bValid = false;
    }
}

void TGameMap::_TImpl::_TLossConditionValidater::visit(const TLCTimeExpires& lc)
{
    _m_bValid = true;
}

void TGameMap::_TImpl::streamObject(streambuf* pStreamBuf, const TGameObject& obj)
{
#line 1438
    assert(pStreamBuf != NULL);
    TRawOStream stream(pStreamBuf);
    stream << obj.getObjectType();
    obj.write(&stream);
}

TGameMap::_TImpl::_TImpl(TClient* pClient, const TObjectFactory* pObjectFactory, TSize size, bool bTwoLayer)
    : _m_pClient(pClient), _m_pObjectFactory(pObjectFactory), _m_size(size), _m_bTwoLayer(bTwoLayer)
{
#line 1452
    assert(_m_pClient != NULL);
    assert(_m_pObjectFactory != NULL);
    assert((int) _m_size >= (int) 0 && (int) _m_size < (int) s_kNumSizes);
    _m_aLayer.resize(_m_bTwoLayer ? 2 : 1, TLayer(_m_size));
}

TRawIStream& TGameMap::_TImpl::readContainer(TRawIStream& stream, vector<TRumor>& aRumor)
{
    aRumor.erase(aRumor.begin(), aRumor.end());
    long n;
    stream >> n;
    while (n-- > 0) {
        TRumor rumor;
        stream >> rumor;
        aRumor.push_back(rumor);
    }
    return stream;
}

TRawIStream& TGameMap::_TImpl::readContainer(TRawIStream& stream, vector<TTimedEvent>& aTimedEvent)
{
    aTimedEvent.erase(aTimedEvent.begin(), aTimedEvent.end());
    long n;
    stream >> n;
    while (n-- > 0) {
        TTimedEvent timedEvent;
        stream >> timedEvent;
        aTimedEvent.push_back(timedEvent);
    }
    return stream;
}


TGameMap::_TImpl::~_TImpl()
{
#line 1798
    assert(_m_aLayer.size() == ( _m_bTwoLayer ? 2 : 1 ));
    delete _m_pProperties->m_pLossCondition;
    _m_pProperties->m_pLossCondition = NULL;
    delete _m_pProperties->m_pVictoryCondition;
    _m_pProperties->m_pVictoryCondition = NULL;
}

TGameMap::TLayer* TGameMap::_TImpl::getPLayer(unsigned int num)
{
#line 1810
    assert(num < 1 || ( _m_bTwoLayer && num < 2 ));
    assert(_m_aLayer.size() == ( _m_bTwoLayer ? 2 : 1 ));
    return &_m_aLayer[num];
}

void TGameMap::_TImpl::setName(const string& newName)
{
#line 1818
    assert(newName.size() <= s_kMaxNameLen);
    assert(newName.find( '\n' ) == std::string::npos);
    assert(newName.find( '\t' ) == std::string::npos);
    _m_pProperties->m_name = newName;
}

void TGameMap::_TImpl::setDesc(const string& newDesc)
{
#line 1827
    assert(newDesc.size() <= s_kMaxDescLen);
    assert(newDesc.find( '\t' ) == std::string::npos);
    _m_pProperties->m_desc = newDesc;
}

void TGameMap::_TImpl::setDifficulty(TDifficulty newDifficulty)
{
#line 1835
    assert((int) newDifficulty >= 0 && (int) newDifficulty < (int) s_kNumDifficulties);
    _m_pProperties->m_difficulty = newDifficulty;
}

void TGameMap::_TImpl::setPlayers(const TArray<TPlayerInfo, kNumPlayers>& newPlayers)
{
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        const _TPPlayerBookkeeping& pConstPlayerBookkeeping = _m_apPlayerBookkeeping[player];
        if (pConstPlayerBookkeeping->m_numUnits != 0) {
#line 1852
            assert(newPlayers[ player ].getBPresent());
            if (newPlayers[player].getBGenerateHero()) {
#line 1857
                assert(pConstPlayerBookkeeping->m_numHeroes < s_kMaxHeroesPerPlayer);
                const TMapObjectRef& mainTownRef = newPlayers[player].getMainTownRef();
#line 1862
                assert(_m_apPlayerBookkeeping[ player ]->m_townRefs.find( mainTownRef ) != _m_apPlayerBookkeeping[ player ]->m_townRefs.end());
                const TTown* pTown = dynamic_cast<const TTown*>(
                    &getLayer(mainTownRef.getBSecondLayer()).getObject(mainTownRef.getObjectID()));
#line 1864
                assert(pTown != NULL);
                assert(pTown->getOwner() == player && pTown->getPVisitingHero() == __null);
            } else {
#line 1868
                assert(newPlayers[ player ].getMainTownRef() == TMapObjectRef());
            }
        } else {
#line 1872
            assert(!newPlayers[ player ].getBPresent());
            assert(!newPlayers[ player ].getBGenerateHero());
            assert(newPlayers[ player ].getMainTownRef() == TMapObjectRef());
        }
    }
    _m_pProperties->m_players = newPlayers;
}

void TGameMap::_TImpl::setTeamInfo(const TTeamInfo& newTeamInfo)
{
    if (newTeamInfo.getBHasTeams()) {
#line 1888
        assert(newTeamInfo.getNumTeams() < _m_pBookkeeping->m_numPlayableSlots);
        const _TPProperties& pConstProperties = _m_pProperties;
        for (unsigned int player = 0; player < kNumPlayers; player++) {
            if (pConstProperties->m_players[player].getBPresent()) {
#line 1895
                assert(newTeamInfo.getPlayerTeam( static_cast< TPlayer >( player ) ) < newTeamInfo.getNumTeams());
            } else {
#line 1897
                assert(newTeamInfo.getPlayerTeam( static_cast< TPlayer >( player ) ) == 0);
            }
        }
    }
    _m_pProperties->m_teamInfo = newTeamInfo;
}

void TGameMap::_TImpl::setRumors(const vector<TRumor>& newRumors)
{
#line 1909
    assert(newRumors.size() <= s_kMaxRumors);
    _m_pProperties->m_rumors = newRumors;
}

void TGameMap::_TImpl::setTimedEvents(const vector<TTimedEvent>& newTimedEvents)
{
#line 1916
    assert(newTimedEvents.size() <= s_kMaxTimedEvents);
    _m_pProperties->m_timedEvents = newTimedEvents;
}

void TGameMap::_TImpl::setVictoryCondition(const TVictoryCondition* pNewVictoryCondition)
{
    if ((_m_pProperties->m_pVictoryCondition == NULL && pNewVictoryCondition == NULL)
        || (_m_pProperties->m_pVictoryCondition != NULL && pNewVictoryCondition != NULL
            && TVictoryCondition::equivalent(*_m_pProperties->m_pVictoryCondition, *pNewVictoryCondition)))
        return;
    delete _m_pProperties->m_pVictoryCondition;
    _m_pProperties->m_pVictoryCondition = NULL;
    if (pNewVictoryCondition != NULL) {
#line 1937
        assert(_isValid( *pNewVictoryCondition ));
        _m_pProperties->m_pVictoryCondition = TVictoryCondition::clone(*pNewVictoryCondition, ::operator new);
        if (_m_pProperties->m_pVictoryCondition == NULL)
            throw TAllocationFailure(__FILE__, __LINE__);
    }
}

void TGameMap::_TImpl::setLossCondition(const TLossCondition* pNewLossCondition)
{
    if ((_m_pProperties->m_pLossCondition == NULL && pNewLossCondition == NULL)
        || (_m_pProperties->m_pLossCondition != NULL && pNewLossCondition != NULL
            && TLossCondition::equivalent(*_m_pProperties->m_pLossCondition, *pNewLossCondition)))
        return;
    delete _m_pProperties->m_pLossCondition;
    _m_pProperties->m_pLossCondition = NULL;
    if (pNewLossCondition != NULL) {
#line 1961
        assert(_isValid( *pNewLossCondition ));
        _m_pProperties->m_pLossCondition = TLossCondition::clone(*pNewLossCondition, ::operator new);
        if (_m_pProperties->m_pLossCondition == NULL)
            throw TAllocationFailure(__FILE__, __LINE__);
    }
}

TMapLayerObjectID TGameMap::_TImpl::placeObject(bool bSecondLayer, const TGameObject& obj, unsigned int x, unsigned int y,
                                                TTileExtent* pUpdatedExtent)
{
    if (const TNonRandomHero* pNonRandomHero = dynamic_cast<const TNonRandomHero*>(&obj))
        return _placeNonRandomHero(bSecondLayer, *pNonRandomHero, x, y, pUpdatedExtent);
    if (const TPrison* pPrison = dynamic_cast<const TPrison*>(&obj))
        return _placePrison(bSecondLayer, *pPrison, x, y, pUpdatedExtent);
    if (const THero* pHero = dynamic_cast<const THero*>(&obj))
        return _placeHero(bSecondLayer, *pHero, x, y, pUpdatedExtent);
    if (const TTown* pTown = dynamic_cast<const TTown*>(&obj))
        return _placeTown(bSecondLayer, *pTown, x, y, pUpdatedExtent);
    if (const THolyGrail* pHolyGrail = dynamic_cast<const THolyGrail*>(&obj))
        return _placeHolyGrail(bSecondLayer, *pHolyGrail, x, y, pUpdatedExtent);
    if (const TMine* pMine = dynamic_cast<const TMine*>(&obj))
        return _placeMine(bSecondLayer, *pMine, x, y, pUpdatedExtent);
    if (const TGenerator* pGenerator = dynamic_cast<const TGenerator*>(&obj))
        return _placeGenerator(bSecondLayer, *pGenerator, x, y, pUpdatedExtent);
    if (const TSign* pSign = dynamic_cast<const TSign*>(&obj))
        return _placeSign(bSecondLayer, *pSign, x, y, pUpdatedExtent);
    TMapLayerObjectID result = _placeGeneralObject(bSecondLayer, obj, x, y, pUpdatedExtent);
#line 2009
    assert(result != TLayer::s_kInvalidObjID);
    const TLayer* pLayer = getPLayer(bSecondLayer);
    _onGeneralObjectAdded(pLayer->getObject(result));
    return result;
}

void TGameMap::_TImpl::removeObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent)
{
#line 2019
    assert(pUpdatedExtent != NULL);
    assert(!bSecondLayer || _m_bTwoLayer);
    TLayer* pLayer = getPLayer(bSecondLayer);
    *pUpdatedExtent = pLayer->getObjectExtent(objID);
    _removeObjectHelper(bSecondLayer, objID);
}

void TGameMap::_TImpl::floatObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent)
{
#line 2031
    assert(pUpdatedExtent != NULL);
    assert(!bSecondLayer || _m_bTwoLayer);
    TLayer* pLayer = getPLayer(bSecondLayer);
    *pUpdatedExtent = pLayer->getObjectExtent(objID);
    pLayer->_floatObject(objID);
}

void TGameMap::_TImpl::unfloatObject(bool bSecondLayer, unsigned int x, unsigned int y, TTileExtent* pUpdatedExtent)
{
#line 2043
    assert(pUpdatedExtent != NULL);
    assert(!bSecondLayer || _m_bTwoLayer);
    TLayer* pLayer = getPLayer(bSecondLayer);
    TMapLayerObjectID objID = pLayer->getFloatingObjID();
#line 2048
    assert(objID != TLayer::s_kInvalidObjID);
    const TGameObject& obj = pLayer->getObject(objID);
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

void TGameMap::_TImpl::removeFloatingObject(bool bSecondLayer)
{
#line 2071
    assert(!bSecondLayer || _m_bTwoLayer);
    TLayer* pLayer = getPLayer(bSecondLayer);
    TMapLayerObjectID objID = pLayer->getFloatingObjID();
#line 2075
    assert(objID != TLayer::s_kInvalidObjID);
    _removeObjectHelper(bSecondLayer, objID);
}

bool TGameMap::_TImpl::onTerrainTypeChanged(bool bSecondLayer, unsigned int x, unsigned int y,
                                            TTerrainType oldTerrainType, TTileExtent* pUpdatedExtent)
{
#line 2083
    assert(!bSecondLayer || _m_bTwoLayer);
    assert(x < getWidth());
    assert(y < getHeight());
    assert(oldTerrainType >= 0 && oldTerrainType < kNumTerrainTypes);
    assert(pUpdatedExtent != NULL);
    bool bObjectsRemoved = false;
    const TLayer& layer = getLayer(bSecondLayer);
    TTerrainType newTerrainType = layer.getCell(x, y).getTerrainType();
#line 2093
    assert(newTerrainType != oldTerrainType);
    unsigned int numObjs = layer.getNumObjectIDsAtCell(x, y);
    static vector<unsigned int> aObjID;
    aObjID.clear();
    aObjID.reserve(numObjs);
    for (unsigned int i = 0; i < numObjs; i++)
        aObjID.push_back(layer.getObjectIDAtCell(x, y, i));
    for (vector<unsigned int>::const_iterator pObjID = aObjID.begin(); pObjID != aObjID.end(); ++pObjID) {
        const TGameObject& obj = layer.getObject(*pObjID);
        if (obj.getTerrainMask()[newTerrainType])
            continue;
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
    if (oldTerrainType == eTerrainWater) {
        bool abAdjacent[8];
        computeAdjacentDirs(layer.getWidth(), layer.getHeight(), x, y, abAdjacent);
        for (unsigned int dir = 0; dir < 8; dir++) {
            if (!abAdjacent[dir])
                continue;
            TTilePoint adjLoc = TPoint<int>(x, y) + akAdjOffset[dir];
            if (layer.getCell(adjLoc).getTerrainType() != eTerrainWater) {
                unsigned int numAdjObjs = layer.getNumObjectIDsAtCell(adjLoc);
                static vector<unsigned int> aAdjObjID;
                aAdjObjID.clear();
                aAdjObjID.reserve(numAdjObjs);
                for (unsigned int i = 0; i < numAdjObjs; i++)
                    aAdjObjID.push_back(layer.getObjectIDAtCell(adjLoc, i));
                for (vector<unsigned int>::const_iterator pObjID = aAdjObjID.begin(); pObjID != aAdjObjID.end();
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

void TGameMap::_TImpl::onObjectRemoved()
{
    const _TPProperties& pConstProperties = _m_pProperties;
    if (pConstProperties->m_pLossCondition != NULL && !_isValid(*pConstProperties->m_pLossCondition)) {
        delete _m_pProperties->m_pLossCondition;
        _m_pProperties->m_pLossCondition = NULL;
    }
    if (pConstProperties->m_pVictoryCondition != NULL && !_isValid(*pConstProperties->m_pVictoryCondition)) {
        delete _m_pProperties->m_pVictoryCondition;
        _m_pProperties->m_pVictoryCondition = NULL;
    }
}

void TGameMap::_TImpl::onHeroAdded(const THero& hero)
{
#line 2215
    assert(static_cast< _TPBookkeeping const & >( _m_pBookkeeping )->m_numHeroes < s_kMaxHeroesOnMap);
    _onPlayableAdded(hero);
    _m_pBookkeeping->m_numHeroes++;
    if (hero.getOwner() != ePlayerNone) {
        _TPlayerBookkeeping& playerBookkeeping = *_m_apPlayerBookkeeping[hero.getOwner()];
#line 2230
        assert(std::accumulate( playerBookkeeping.m_aNumHeroesOfType.begin(), playerBookkeeping.m_aNumHeroesOfType.end(), 0U ) + playerBookkeeping.m_numRandomHeroes == playerBookkeeping.m_numHeroes);
#line 2233
        assert(playerBookkeeping.m_numHeroes + getPlayers()[ hero.getOwner() ].getBGenerateHero() ? 1 : 0 < s_kMaxHeroesPerPlayer);
        playerBookkeeping.m_numHeroes++;
    } else {
#line 2237
        assert(dynamic_cast< TPrison const * >( &hero ) != __null);
    }
    if (dynamic_cast<const TRandomHero*>(&hero) == NULL) {
#line 2242
        assert(_m_pBookkeeping->m_aabHeroAvailable[ hero.getClass() ][ hero.getProtoNum() ]);
        _m_pBookkeeping->m_aabHeroAvailable[hero.getClass()][hero.getProtoNum()] = false;
        if (hero.getOwner() != ePlayerNone)
            _m_apPlayerBookkeeping[hero.getOwner()]->m_aNumHeroesOfType[hero.getClassTraits().m_townType]++;
    } else {
#line 2252
        assert(hero.getOwner() != ePlayerNone);
        _m_apPlayerBookkeeping[hero.getOwner()]->m_numRandomHeroes++;
    }
}

void TGameMap::_TImpl::onRemovingHero(const THero& hero)
{
#line 2260
    assert(static_cast< _TPBookkeeping const & >( _m_pBookkeeping )->m_numHeroes > 0);
    if (dynamic_cast<const TRandomHero*>(&hero) == NULL) {
        if (hero.getOwner() != ePlayerNone) {
#line 2267
            assert(_m_apPlayerBookkeeping[ hero.getOwner() ]->m_aNumHeroesOfType[ hero.getClassTraits().m_townType ] > 0);
            _m_apPlayerBookkeeping[hero.getOwner()]->m_aNumHeroesOfType[hero.getClassTraits().m_townType]--;
        }
#line 2272
        assert(!_m_pBookkeeping->m_aabHeroAvailable[ hero.getClass() ][ hero.getProtoNum() ]);
        _m_pBookkeeping->m_aabHeroAvailable[hero.getClass()][hero.getProtoNum()] = true;
    } else {
#line 2278
        assert(hero.getOwner() != ePlayerNone);
        assert(_m_apPlayerBookkeeping[ hero.getOwner() ]->m_numRandomHeroes > 0);
        _m_apPlayerBookkeeping[hero.getOwner()]->m_numRandomHeroes--;
    }
    if (hero.getOwner() != ePlayerNone) {
        _TPlayerBookkeeping& playerBookkeeping = *_m_apPlayerBookkeeping[hero.getOwner()];
#line 2288
        assert(playerBookkeeping.m_numHeroes > 0);
        playerBookkeeping.m_numHeroes--;
#line 2296
        assert(std::accumulate( playerBookkeeping.m_aNumHeroesOfType.begin(), playerBookkeeping.m_aNumHeroesOfType.end(), 0U ) + playerBookkeeping.m_numRandomHeroes == playerBookkeeping.m_numHeroes);
    } else {
#line 2299
        assert(dynamic_cast< TPrison const * >( &hero ) != __null);
    }
    _m_pBookkeeping->m_numHeroes--;
    _onRemovingPlayable(hero);
}

void TGameMap::_TImpl::onHeroProtoChanged(THeroClass heroClass, unsigned int oldProtoNum, unsigned int newProtoNum)
{
#line 2309
    assert(heroClass >= 0 && heroClass < kNumHeroClasses);
    assert(oldProtoNum < THero::s_akClassTraits[ heroClass ].m_numPrototypes);
    assert(newProtoNum < THero::s_akClassTraits[ heroClass ].m_numPrototypes);
#line 2313
    assert(!_m_pBookkeeping->m_aabHeroAvailable[ heroClass ][ oldProtoNum ]);
    assert(_m_pBookkeeping->m_aabHeroAvailable[ heroClass ][ newProtoNum ]);
    _m_pBookkeeping->m_aabHeroAvailable[heroClass][oldProtoNum] = true;
    _m_pBookkeeping->m_aabHeroAvailable[heroClass][newProtoNum] = false;
}

void TGameMap::_TImpl::onHeroClassChanged(THeroClass oldHeroClass, unsigned int oldProtoNum, THeroClass newHeroClass,
                                          unsigned int newProtoNum)
{
#line 2322
    assert(oldHeroClass >= 0 && oldHeroClass < kNumHeroClasses);
    assert(oldProtoNum < THero::s_akClassTraits[ oldHeroClass ].m_numPrototypes);
    assert(newHeroClass >= 0 && newHeroClass < kNumHeroClasses);
    assert(newProtoNum < THero::s_akClassTraits[ newHeroClass ].m_numPrototypes);
#line 2327
    assert(!_m_pBookkeeping->m_aabHeroAvailable[ oldHeroClass ][ oldProtoNum ]);
    _m_pBookkeeping->m_aabHeroAvailable[oldHeroClass][oldProtoNum] = true;
#line 2330
    assert(_m_pBookkeeping->m_aabHeroAvailable[ newHeroClass ][ newProtoNum ]);
    _m_pBookkeeping->m_aabHeroAvailable[newHeroClass][newProtoNum] = false;
}

void TGameMap::_TImpl::onHeroOwnerChanged(const THero& hero, TPlayer oldOwner)
{
#line 2337
    assert(oldOwner >= 0 && oldOwner < kNumPlayers);
    TPlayer newOwner = hero.getOwner();
    THero* pHero = const_cast<THero*>(&hero);
    pHero->setOwner(oldOwner);
    onRemovingHero(hero);
    pHero->setOwner(newOwner);
    onHeroAdded(hero);
}

void TGameMap::_TImpl::onTownOwnerChanged(const TTown& town, bool bSecondLayer, unsigned int objID, TPlayer oldOwner)
{
#line 2352
    assert(objID != TLayer::s_kInvalidObjID);
    assert(&getLayer( bSecondLayer ).getObject( objID ) == &town);
    assert(oldOwner >= ePlayerNone && oldOwner < kNumPlayers);
    TPlayer savedOwner = town.getOwner();
    TTown* pTown = const_cast<TTown*>(&town);
    THero* pVisitingHero = pTown->getPVisitingHero();
    if (pVisitingHero != NULL) {
#line 2362
        assert(pVisitingHero->getOwner() == savedOwner);
        pVisitingHero->setOwner(oldOwner);
        onRemovingHero(*pVisitingHero);
    }
    pTown->setOwner(oldOwner);
    _onRemovingTown(town, bSecondLayer, objID);
    pTown->setOwner(savedOwner);
    _onTownAdded(town, bSecondLayer, objID);
    if (pVisitingHero != NULL) {
        pVisitingHero->setOwner(savedOwner);
        onHeroAdded(*pVisitingHero);
    }
}

void TGameMap::_TImpl::removeSecondLayer()
{
#line 2383
    assert(isTwoLayer());
    TLayer* pLayer = getPLayer(true);
    TLayer::TObjectIDIter iter = pLayer->objectIDBegin();
    while (iter != pLayer->objectIDEnd()) {
        TMapLayerObjectID objID = *iter++;
        _removeObjectHelper(true, objID);
    }
    _m_aLayer.pop_back();
    _m_bTwoLayer = false;
}

void TGameMap::_TImpl::addSecondLayer()
{
#line 2401
    assert(!isTwoLayer());
    _m_bTwoLayer = true;
    _m_aLayer.push_back(TLayer(_m_size));
}

const TGameObject* TGameMap::_TImpl::getPObject(bool bSecondLayer, unsigned int objID) const
{
#line 2410
    assert(!bSecondLayer || _m_bTwoLayer);
    const TLayer& layer = getLayer(bSecondLayer);
    if (!layer.isObjectIDValid(objID))
        return NULL;
    return layer.getPObject(objID);
}

TTilePoint TGameMap::_TImpl::getObjectLoc(bool bSecondLayer, unsigned int objID) const
{
#line 2420
    assert(!bSecondLayer || _m_bTwoLayer);
    const TLayer& layer = getLayer(bSecondLayer);
#line 2422
    assert(layer.isObjectIDValid( objID ));
    return layer.getObjectLoc(objID);
}

const TGameMap::TLayer* TGameMap::_TImpl::getPLayer(unsigned int num) const
{
#line 2429
    assert(num == 0 || ( _m_bTwoLayer && num < 2 ));
    assert(_m_aLayer.size() == ( _m_bTwoLayer ? 2 : 1 ));
    return &_m_aLayer[num];
}

TGameObject* TGameMap::_TImpl::reconstructObject(streambuf* pStreamBuf, int version,
                                                 void* (*pfnAllocator)(unsigned int)) const
{
#line 2977
    assert(pStreamBuf != NULL);
    TRawIStream iStream(pStreamBuf);
    TObjectType objType;
    iStream >> objType;
    return _createObject(objType, &iStream, version, pfnAllocator);
}

bool TGameMap::_TImpl::isHeroOnMap(THeroID heroID) const
{
    THeroClass heroClass = THeroClass(heroID / 8);
#line 2989
    assert(heroClass >= 0 && heroClass < kNumHeroClasses);
    unsigned int protoNum = heroID - THero::s_akClassTraits[heroClass].m_firstHeroID;
    return !_m_pBookkeeping->m_aabHeroAvailable[heroClass][protoNum];
}

set<unsigned int> TGameMap::_TImpl::getAvailableHeroesInClass(THeroClass heroClass) const
{
    set<unsigned int> result;
    for (unsigned int protoNum = 0; protoNum < THero::s_akClassTraits[heroClass].m_numPrototypes; protoNum++)
        if (_m_pBookkeeping->m_aabHeroAvailable[heroClass][protoNum])
            result.insert(protoNum);
    return result;
}

bitset<kNumPlayers> TGameMap::_TImpl::getAvailableHeroOwnersMask() const
{
    bitset<kNumPlayers> result;
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        unsigned int numHeroes = _m_apPlayerBookkeeping[player]->m_numHeroes;
        if (getPlayers()[player].getBGenerateHero())
            numHeroes++;
        result[player] = numHeroes < s_kMaxHeroesPerPlayer;
    }
    return result;
}

const set<TMapObjectRef>& TGameMap::_TImpl::getPlayerTownRefs(TPlayer player) const
{
#line 3034
    assert(player >= 0 && player < kNumPlayers);
    return _m_apPlayerBookkeeping[player]->m_townRefs;
}

unsigned int TGameMap::_TImpl::getNumObelisksOnMap() const
{
    TCappedObjectTypeInfoMap::const_iterator pObeliskInfo = kCappedObjectTypeInfoMap.find(OBELISK);
#line 3042
    assert(pObeliskInfo != kCappedObjectTypeInfoMap.end());
    unsigned int obeliskOrdinal = pObeliskInfo->second.m_ordinal;
#line 3045
    assert(obeliskOrdinal < _m_pBookkeeping->m_aNumObjsOfCappedType.size());
    return _m_pBookkeeping->m_aNumObjsOfCappedType[obeliskOrdinal];
}


bool TGameMap::_TImpl::_isValidPlacement(const TLayer& layer, const TGameObject& obj, unsigned int x, unsigned int y)
{
    static vector<unsigned int> aLowerObjIDs;
    static vector<unsigned int> aHigherObjIDs;
    aLowerObjIDs.clear();
    aHigherObjIDs.clear();
    unsigned int heightMap[TObjectType::kMaxObjWidth][TObjectType::kMaxObjHeight];
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
                for (; pObjInfo != aObjInfo.end() && pObjInfo->m_height < height; ++pObjInfo)
                    if (find(aLowerObjIDs.begin(), aLowerObjIDs.end(), pObjInfo->m_objID) == aLowerObjIDs.end())
                        aLowerObjIDs.push_back(pObjInfo->m_objID);
                for (; pObjInfo != aObjInfo.end() && pObjInfo->m_height <= height; ++pObjInfo)
#line 3616
                    assert(pObjInfo->m_height == height);
                for (; pObjInfo != aObjInfo.end(); ++pObjInfo) {
#line 3621
                    assert(pObjInfo->m_height > height);
                    if (find(aHigherObjIDs.begin(), aHigherObjIDs.end(), pObjInfo->m_objID) == aHigherObjIDs.end())
                        aHigherObjIDs.push_back(pObjInfo->m_objID);
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
    for (vector<unsigned int>::const_iterator pObjID = aLowerObjIDs.begin(); pObjID != aLowerObjIDs.end(); ++pObjID)
        if (find(aHigherObjIDs.begin(), aHigherObjIDs.end(), *pObjID) != aHigherObjIDs.end())
            return false;
    return obj.getType() != SHIPYARD || _isValidShipyardPlacement(layer, obj, x, y);
}

bool TGameMap::_TImpl::_isValidShipyardPlacement(const TLayer& layer, const TGameObject& shipyard, unsigned int x,
                                                 unsigned int y)
{
#line 3695
    assert(shipyard.getType() == SHIPYARD);
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

TMapLayerObjectID TGameMap::_TImpl::_placeGeneralObject(bool bSecondLayer, const TGameObject& obj, unsigned int x,
                                                        unsigned int y, TTileExtent* pUpdatedExtent)
{
#line 3739
    assert(pUpdatedExtent != NULL);
    assert(!bSecondLayer || _m_bTwoLayer);
    const _TPBookkeeping& pConstBookkeeping = _m_pBookkeeping;
    TCappedObjectTypeInfoMap::const_iterator pCappedObjTypeInfo = kCappedObjectTypeInfoMap.find(obj.getType());
    if (pCappedObjTypeInfo != kCappedObjectTypeInfoMap.end()) {
        unsigned int typeOrdinal = pCappedObjTypeInfo->second.m_ordinal;
#line 3752
        assert(typeOrdinal < pConstBookkeeping->m_aNumObjsOfCappedType.size());
        if (pConstBookkeeping->m_aNumObjsOfCappedType[typeOrdinal] >= pCappedObjTypeInfo->second.m_cap)
            throw TPlaceObjFailureTooManyInstancesOfTypeOnMap(obj.getType(), pCappedObjTypeInfo->second.m_cap);
    }
    TLayer& layer = *getPLayer(bSecondLayer);
    if (!_isValidPlacement(layer, obj, x, y))
        throw TPlaceObjFailureInvalidPlacement();
    TMapLayerObjectID result = layer._placeObject(obj, x, y);
#line 3764
    assert(result != TLayer::s_kInvalidObjID);
    *pUpdatedExtent = layer.getObjectExtent(result);
    return result;
}

TMapLayerObjectID TGameMap::_TImpl::_placeNonRandomHero(bool bSecondLayer, const TNonRandomHero& hero, unsigned int x,
                                                        unsigned int y, TTileExtent* pUpdatedExtent)
{
    if (!_isHeroAvailable(hero.getClass(), hero.getProtoNum())) {
        unsigned int protoNum = _pickAvailableHero(hero.getClass());
        if (protoNum >= THero::s_akClassTraits[hero.getClass()].m_numPrototypes)
            throw TPlaceObjFailureNoAvailableHeroesInClass();
        auto_ptr<THero> pNewHero(
            _m_pObjectFactory->createNonRandomHero(hero.getObjectType(), hero.getOwner(), protoNum, ::operator new));
        if (pNewHero.get() == NULL)
#line 3791
            throw TAllocationFailure(__FILE__, __LINE__);
        copyCustomizations(hero, pNewHero.get());
        return _placeHero(bSecondLayer, *pNewHero, x, y, pUpdatedExtent);
    }
    return _placeHero(bSecondLayer, hero, x, y, pUpdatedExtent);
}

TMapLayerObjectID TGameMap::_TImpl::_placePrison(bool bSecondLayer, const TPrison& prison, unsigned int x, unsigned int y,
                                                 TTileExtent* pUpdatedExtent)
{
    if (!_isHeroAvailable(prison.getClass(), prison.getProtoNum())) {
        unsigned int heroClass = prison.getClass();
        unsigned int protoNum = _pickAvailableHero(THeroClass(heroClass));
        if (protoNum >= THero::s_akClassTraits[heroClass].m_numPrototypes) {
            for (heroClass = 0;;) {
                protoNum = _pickAvailableHero(THeroClass(heroClass));
                if (protoNum < THero::s_akClassTraits[heroClass].m_numPrototypes)
                    break;
                if (++heroClass >= kNumHeroClasses)
                    throw TPlaceObjFailureNoAvailableHeroesInClass();
            }
        }
        auto_ptr<THero> pNewPrison(
            _m_pObjectFactory->createPrison(prison.getObjectType(), THeroClass(heroClass), protoNum, ::operator new));
        if (pNewPrison.get() == NULL)
#line 3831
            throw TAllocationFailure(__FILE__, __LINE__);
        copyCustomizations(prison, pNewPrison.get());
        return _placeHero(bSecondLayer, *pNewPrison, x, y, pUpdatedExtent);
    }
    return _placeHero(bSecondLayer, prison, x, y, pUpdatedExtent);
}

TMapLayerObjectID TGameMap::_TImpl::_placeHero(bool bSecondLayer, const THero& hero, unsigned int x, unsigned int y,
                                               TTileExtent* pUpdatedExtent)
{
    const _TPBookkeeping& pConstBookkeeping = _m_pBookkeeping;
#line 3854
    assert(pConstBookkeeping->m_numHeroes <= s_kMaxHeroesOnMap);
    if (pConstBookkeeping->m_numHeroes >= s_kMaxHeroesOnMap)
        throw TPlaceObjFailureTooManyHeroesOnMap();
    if (hero.getOwner() != ePlayerNone) {
        const _TPPlayerBookkeeping& pConstPlayerBookkeeping = _m_apPlayerBookkeeping[hero.getOwner()];
        unsigned int numHeroes = pConstPlayerBookkeeping->m_numHeroes;
        if (getPlayers()[hero.getOwner()].getBGenerateHero())
            numHeroes++;
        if (numHeroes >= s_kMaxHeroesPerPlayer)
            throw TPlaceObjFailureTooManyHeroesForPlayer();
    }
    TMapLayerObjectID result = _placeGeneralObject(bSecondLayer, hero, x, y, pUpdatedExtent);
#line 3874
    assert(result != TLayer::s_kInvalidObjID);
    const TLayer* pLayer = getPLayer(bSecondLayer);
    const THero* pHero = dynamic_cast<const THero*>(pLayer->getPObject(result));
#line 3878
    assert(pHero != NULL);
    onHeroAdded(*pHero);
    return result;
}

TMapLayerObjectID TGameMap::_TImpl::_placeTown(bool bSecondLayer, const TTown& town, unsigned int x, unsigned int y,
                                               TTileExtent* pUpdatedExtent)
{
    const _TPBookkeeping& pConstBookkeeping = _m_pBookkeeping;
#line 3896
    assert(pConstBookkeeping->m_numTowns <= s_kMaxTownsOnMap);
    if (pConstBookkeeping->m_numTowns == s_kMaxTownsOnMap)
        throw TPlaceObjFailureTooManyTownsOnMap();
    const THero* pVisitingHero = town.getPVisitingHero();
    if (pVisitingHero != NULL) {
#line 3904
        assert(town.getOwner() >= 0 && town.getOwner() < kNumPlayers);
        if (pConstBookkeeping->m_numHeroes >= s_kMaxHeroesOnMap)
            throw TPlaceObjFailureTooManyHeroesOnMap();
        const _TPPlayerBookkeeping& pConstPlayerBookkeeping = _m_apPlayerBookkeeping[town.getOwner()];
        unsigned int numHeroes = pConstPlayerBookkeeping->m_numHeroes;
        if (getPlayers()[town.getOwner()].getBGenerateHero())
            numHeroes++;
        if (numHeroes >= s_kMaxHeroesPerPlayer)
            throw TPlaceObjFailureTooManyHeroesForPlayer();
        if (dynamic_cast<const TRandomHero*>(pVisitingHero) == NULL
            && !_isHeroAvailable(pVisitingHero->getClass(), pVisitingHero->getProtoNum())) {
            auto_ptr<TGameObject> pObjCopy(town.clone(::operator new));
            if (pObjCopy.get() == NULL)
#line 3927
                throw TAllocationFailure(__FILE__, __LINE__);
            TTown* pTownCopy = dynamic_cast<TTown*>(pObjCopy.get());
#line 3930
            assert(pTownCopy != NULL);
            unsigned int protoNum = _pickAvailableHero(pVisitingHero->getClass());
            if (protoNum < THero::s_akClassTraits[pVisitingHero->getClass()].m_numPrototypes) {
                auto_ptr<THero> pNewHero(_m_pObjectFactory->createNonRandomHero(
                    pVisitingHero->getObjectType(), pTownCopy->getOwner(), protoNum, ::operator new));
                if (pNewHero.get() == NULL)
#line 3937
                    throw TAllocationFailure(__FILE__, __LINE__);
                copyCustomizations(*pVisitingHero, pNewHero.get());
                pTownCopy->setVisitingHero(pNewHero.get());
#line 3942
                assert(pTownCopy->getPVisitingHero() != __null);
            } else {
                pTownCopy->setVisitingHero(NULL);
            }
            TMapLayerObjectID result = _placeTown(bSecondLayer, *pTownCopy, x, y, pUpdatedExtent);
#line 3949
            assert(result != TLayer::s_kInvalidObjID);
            return result;
        }
    }
    TMapLayerObjectID result = _placeGeneralObject(bSecondLayer, town, x, y, pUpdatedExtent);
#line 3956
    assert(result != TLayer::s_kInvalidObjID);
    const TLayer* pLayer = getPLayer(bSecondLayer);
    const TTown* pTown = dynamic_cast<const TTown*>(pLayer->getPObject(result));
#line 3960
    assert(pTown != NULL);
    _onTownAdded(*pTown, bSecondLayer, result);
    if (pTown->getPVisitingHero() != NULL)
        onHeroAdded(*pTown->getPVisitingHero());
    return result;
}

TMapLayerObjectID TGameMap::_TImpl::_placeHolyGrail(bool bSecondLayer, const THolyGrail& holyGrail, unsigned int x,
                                                    unsigned int y, TTileExtent* pUpdatedExtent)
{
    const _TPBookkeeping& pConstBookkeeping = _m_pBookkeeping;
    if (pConstBookkeeping->m_bGrailPlaced)
        throw TPlaceObjFailureHolyGrailAlreadyPlaced();
    if (!(x >= 9 && y >= 9 && x < getWidth() - 9 && y < getHeight() - 9))
        throw TPlaceObjFailureHolyGrailTooCloseToEdge();
    TMapLayerObjectID result = _placeGeneralObject(bSecondLayer, holyGrail, x, y, pUpdatedExtent);
#line 3991
    assert(result != TLayer::s_kInvalidObjID);
    const TLayer* pLayer = getPLayer(bSecondLayer);
    const THolyGrail* pHolyGrail = dynamic_cast<const THolyGrail*>(pLayer->getPObject(result));
#line 3995
    assert(pHolyGrail != NULL);
    _onHolyGrailAdded(*pHolyGrail);
    return result;
}

TMapLayerObjectID TGameMap::_TImpl::_placeMine(bool bSecondLayer, const TMine& mine, unsigned int x, unsigned int y,
                                               TTileExtent* pUpdatedExtent)
{
    const _TPBookkeeping& pConstBookkeeping = _m_pBookkeeping;
    if (pConstBookkeeping->m_numMines >= s_kMaxMinesOnMap)
        throw TPlaceObjFailureTooManyMinesOnMap();
    TMapLayerObjectID result = _placeGeneralObject(bSecondLayer, mine, x, y, pUpdatedExtent);
#line 4017
    assert(result != TLayer::s_kInvalidObjID);
    const TLayer* pLayer = getPLayer(bSecondLayer);
    const TMine* pMine = dynamic_cast<const TMine*>(pLayer->getPObject(result));
#line 4021
    assert(pMine != NULL);
    _onMineAdded(*pMine);
    return result;
}

TMapLayerObjectID TGameMap::_TImpl::_placeGenerator(bool bSecondLayer, const TGenerator& generator, unsigned int x, unsigned int y,
                                               TTileExtent* pUpdatedExtent)
{
    const _TPBookkeeping& pConstBookkeeping = _m_pBookkeeping;
    if (pConstBookkeeping->m_numGenerators >= s_kMaxGeneratorsOnMap)
        throw TPlaceObjFailureTooManyGeneratorsOnMap();
    TMapLayerObjectID result = _placeGeneralObject(bSecondLayer, generator, x, y, pUpdatedExtent);
#line 4043
    assert(result != TLayer::s_kInvalidObjID);
    const TLayer* pLayer = getPLayer(bSecondLayer);
    const TGenerator* pGenerator = dynamic_cast<const TGenerator*>(pLayer->getPObject(result));
#line 4047
    assert(pGenerator != NULL);
    _onGeneratorAdded(*pGenerator);
    return result;
}

TMapLayerObjectID TGameMap::_TImpl::_placeSign(bool bSecondLayer, const TSign& sign, unsigned int x, unsigned int y,
                                               TTileExtent* pUpdatedExtent)
{
    const _TPBookkeeping& pConstBookkeeping = _m_pBookkeeping;
    if (pConstBookkeeping->m_numSigns >= s_kMaxSignsOnMap)
        throw TPlaceObjFailureTooManySignsOnMap();
    TMapLayerObjectID result = _placeGeneralObject(bSecondLayer, sign, x, y, pUpdatedExtent);
#line 4069
    assert(result != TLayer::s_kInvalidObjID);
    const TLayer* pLayer = getPLayer(bSecondLayer);
    const TSign* pSign = dynamic_cast<const TSign*>(pLayer->getPObject(result));
#line 4073
    assert(pSign != NULL);
    _onSignAdded(*pSign);
    return result;
}

void TGameMap::_TImpl::_removeObjectHelper(bool bSecondLayer, unsigned int objID)
{
    TLayer* pLayer = getPLayer(bSecondLayer);
    const TGameObject& obj = pLayer->getObject(objID);
    if (const THero* pHero = dynamic_cast<const THero*>(&obj)) {
        onRemovingHero(*pHero);
    } else if (const TTown* pTown = dynamic_cast<const TTown*>(&obj)) {
        if (pTown->getPVisitingHero() != NULL)
            onRemovingHero(*pTown->getPVisitingHero());
        _onRemovingTown(*pTown, bSecondLayer, objID);
    } else if (const THolyGrail* pHolyGrail = dynamic_cast<const THolyGrail*>(&obj)) {
        _onRemovingHolyGrail(*pHolyGrail);
    } else if (const TMine* pMine = dynamic_cast<const TMine*>(&obj)) {
        _onRemovingMine(*pMine);
    } else if (const TGenerator* pGenerator = dynamic_cast<const TGenerator*>(&obj)) {
        _onRemovingGenerator(*pGenerator);
    } else if (const TSign* pSign = dynamic_cast<const TSign*>(&obj)) {
        _onRemovingSign(*pSign);
    } else {
        _onRemovingGeneralObject(obj);
    }
    pLayer->_removeObject(objID);
    onObjectRemoved();
    _m_pClient->onMapObjectRemoved(bSecondLayer, objID);
}

void TGameMap::_TImpl::_onGeneralObjectAdded(const TGameObject& obj)
{
    TCappedObjectTypeInfoMap::const_iterator pCappedObjTypeInfo = kCappedObjectTypeInfoMap.find(obj.getType());
    if (pCappedObjTypeInfo != kCappedObjectTypeInfoMap.end()) {
        unsigned int typeOrdinal = pCappedObjTypeInfo->second.m_ordinal;
#line 4140
        assert(typeOrdinal < _m_pBookkeeping->m_aNumObjsOfCappedType.size());
        assert(_m_pBookkeeping->m_aNumObjsOfCappedType[ typeOrdinal ] < pCappedObjTypeInfo->second.m_cap);
        _m_pBookkeeping->m_aNumObjsOfCappedType[typeOrdinal]++;
    }
}

void TGameMap::_TImpl::_onRemovingGeneralObject(const TGameObject& obj)
{
    TCappedObjectTypeInfoMap::const_iterator pCappedObjTypeInfo = kCappedObjectTypeInfoMap.find(obj.getType());
    if (pCappedObjTypeInfo != kCappedObjectTypeInfoMap.end()) {
        unsigned int typeOrdinal = pCappedObjTypeInfo->second.m_ordinal;
#line 4155
        assert(typeOrdinal < _m_pBookkeeping->m_aNumObjsOfCappedType.size());
        assert(_m_pBookkeeping->m_aNumObjsOfCappedType[ typeOrdinal ] > 0);
        _m_pBookkeeping->m_aNumObjsOfCappedType[typeOrdinal]--;
    }
}

void TGameMap::_TImpl::_onPlayableAdded(const TPlayableObject& playable)
{
    _onGeneralObjectAdded(playable);
    TPlayer owner = playable.getOwner();
#line 4168
    assert(owner >= ePlayerNone && owner < kNumPlayers);
    if (owner != ePlayerNone && ++_m_apPlayerBookkeeping[owner]->m_numUnits == 1) {
        TPlayerInfo& player = _m_pProperties->m_players[owner];
#line 4174
        assert(!player.getBPresent());
        if (_m_pBookkeeping->m_numPlayableSlots == 0) {
#line 4178
            assert(!_m_pProperties->m_teamInfo.getBHasTeams());
            player.setBHumanPlayable(true);
            player.setBComputerPlayable(true);
        } else {
            if (_m_pProperties->m_teamInfo.getBHasTeams()) {
#line 4188
                assert(_m_pBookkeeping->m_numPlayableSlots >= TTeamInfo::s_kMinTeams);
                _m_pProperties->m_teamInfo.setPlayerTeam(owner, _pickAvailableTeam());
            }
            player.setBComputerPlayable(true);
        }
        _m_pBookkeeping->m_numPlayableSlots++;
    }
}

void TGameMap::_TImpl::_onRemovingPlayable(const TPlayableObject& playable)
{
    TPlayer owner = playable.getOwner();
#line 4205
    assert(owner >= ePlayerNone && owner < kNumPlayers);
    if (owner != ePlayerNone && --_m_apPlayerBookkeeping[owner]->m_numUnits == 0) {
        TPlayerInfo& player = _m_pProperties->m_players[owner];
#line 4211
        assert(player.getBPresent());
        player = TPlayerInfo();
        if (--_m_pBookkeeping->m_numPlayableSlots != 0) {
            TPlayerInfo* pFirstPlayable = NULL;
            unsigned int i;
            for (i = 0; i < kNumPlayers; i++) {
                if (_m_pProperties->m_players[i].getBHumanPlayable())
                    break;
                if (pFirstPlayable == NULL && _m_pProperties->m_players[i].getBComputerPlayable())
                    pFirstPlayable = &_m_pProperties->m_players[i];
            }
            if (i == kNumPlayers) {
#line 4232
                assert(pFirstPlayable != NULL);
                pFirstPlayable->setBHumanPlayable(true);
            }
        }
        if (_m_pProperties->m_teamInfo.getBHasTeams()) {
            TTeamInfo& teamInfo = _m_pProperties->m_teamInfo;
#line 4240
            assert(_m_pBookkeeping->m_numPlayableSlots >= teamInfo.getNumTeams());
            if (_m_pBookkeeping->m_numPlayableSlots == TTeamInfo::s_kMinTeams) {
                teamInfo.setBHasTeams(false);
            } else {
                unsigned int team = teamInfo.getPlayerTeam(owner);
                teamInfo.setPlayerTeam(owner, 0);
                bool bTeamInUse = false;
                for (unsigned int otherPlayer = 0; otherPlayer < kNumPlayers; otherPlayer++) {
                    if (_m_pProperties->m_players[otherPlayer].getBPresent()
                        && teamInfo.getPlayerTeam(TPlayer(otherPlayer)) == team) {
                        bTeamInUse = true;
                        break;
                    }
                }
                if (!bTeamInUse) {
                    if (teamInfo.getNumTeams() == TTeamInfo::s_kMinTeams) {
                        teamInfo.setBHasTeams(false);
                    } else {
                        for (unsigned int otherPlayer = 0; otherPlayer < kNumPlayers; otherPlayer++) {
                            if (_m_pProperties->m_players[otherPlayer].getBPresent()) {
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
#line 4293
                        assert(player < kNumPlayers);
                        if (_m_pProperties->m_players[player].getBPresent()
                            && teamInfo.getPlayerTeam(TPlayer(player)) == lastTeam) {
                            teamInfo.setPlayerTeam(TPlayer(player), 0);
                            break;
                        }
                    }
                    teamInfo.setNumTeams(lastTeam);
                }
            }
        }
    }
    _onRemovingGeneralObject(playable);
}

void TGameMap::_TImpl::_onTownAdded(const TTown& town, bool bSecondLayer, unsigned int objID)
{
#line 4314
    assert(objID != TLayer::s_kInvalidObjID);
    assert(&getLayer( bSecondLayer ).getObject( objID ) == &town);
    assert(static_cast< _TPBookkeeping const & >( _m_pBookkeeping )->m_numTowns < s_kMaxTownsOnMap);
    _onPlayableAdded(town);
    _m_pBookkeeping->m_numTowns++;
    if (town.getOwner() != ePlayerNone) {
        _TPlayerBookkeeping& playerBookkeeping = *_m_apPlayerBookkeeping[town.getOwner()];
#line 4327
        assert(playerBookkeeping.m_townRefs.find( TMapObjectRef( bSecondLayer, objID ) ) == playerBookkeeping.m_townRefs.end());
#line 4333
        assert(std::accumulate( playerBookkeeping.m_aNumTownsOfType.begin(), playerBookkeeping.m_aNumTownsOfType.end(), 0U ) + playerBookkeeping.m_numRandomTowns == playerBookkeeping.m_townRefs.size());
        playerBookkeeping.m_townRefs.insert(TMapObjectRef(bSecondLayer, objID));
        if (town.getType() == TOWN) {
            playerBookkeeping.m_aNumTownsOfType[town.getTownType()]++;
        } else {
#line 4346
            assert(town.getType() == RANDOM_TOWN);
            playerBookkeeping.m_numRandomTowns++;
        }
    }
}

void TGameMap::_TImpl::_onRemovingTown(const TTown& town, bool bSecondLayer, unsigned int objID)
{
#line 4355
    assert(objID != TLayer::s_kInvalidObjID);
    assert(&getLayer( bSecondLayer ).getObject( objID ) == &town);
    assert(static_cast< _TPBookkeeping const & >( _m_pBookkeeping )->m_numTowns > 0);
    if (town.getOwner() != ePlayerNone) {
        _TPlayerBookkeeping& playerBookkeeping = *_m_apPlayerBookkeeping[town.getOwner()];
#line 4363
        assert(playerBookkeeping.m_townRefs.find( TMapObjectRef( bSecondLayer, objID ) ) != playerBookkeeping.m_townRefs.end());
        if (town.getType() == TOWN) {
#line 4368
            assert(playerBookkeeping.m_aNumTownsOfType[ town.getTownType() ] > 0);
            playerBookkeeping.m_aNumTownsOfType[town.getTownType()]--;
        } else {
#line 4374
            assert(town.getType() == RANDOM_TOWN);
            assert(playerBookkeeping.m_numRandomTowns > 0);
            playerBookkeeping.m_numRandomTowns--;
        }
        const TPlayerInfo& constPlayer = static_cast<const _TPProperties&>(_m_pProperties)->m_players[town.getOwner()];
        if (constPlayer.getBGenerateHero() && constPlayer.getMainTownRef() == TMapObjectRef(bSecondLayer, objID)) {
            TPlayerInfo& player = _m_pProperties->m_players[town.getOwner()];
            player.setMainTownRef(TMapObjectRef());
            player.setBGenerateHero(false);
        }
        playerBookkeeping.m_townRefs.erase(TMapObjectRef(bSecondLayer, objID));
#line 4397
        assert(std::accumulate( playerBookkeeping.m_aNumTownsOfType.begin(), playerBookkeeping.m_aNumTownsOfType.end(), 0U ) + playerBookkeeping.m_numRandomTowns == playerBookkeeping.m_townRefs.size());
    }
    _m_pBookkeeping->m_numTowns--;
    _onRemovingPlayable(town);
}

void TGameMap::_TImpl::_onHolyGrailAdded(const THolyGrail& holyGrail)
{
#line 4409
    assert(!static_cast< _TPBookkeeping const & >( _m_pBookkeeping )->m_bGrailPlaced);
    _onGeneralObjectAdded(holyGrail);
    _m_pBookkeeping->m_bGrailPlaced = true;
}

void TGameMap::_TImpl::_onRemovingHolyGrail(const THolyGrail& holyGrail)
{
#line 4419
    assert(static_cast< _TPBookkeeping const & >( _m_pBookkeeping )->m_bGrailPlaced);
    _m_pBookkeeping->m_bGrailPlaced = false;
    _onRemovingGeneralObject(holyGrail);
}

void TGameMap::_TImpl::_onMineAdded(const TMine& mine)
{
#line 4429
    assert(static_cast< _TPBookkeeping const & >( _m_pBookkeeping )->m_numMines < s_kMaxMinesOnMap);
    _onGeneralObjectAdded(mine);
    _m_pBookkeeping->m_numMines++;
}

void TGameMap::_TImpl::_onRemovingMine(const TMine& mine)
{
#line 4439
    assert(static_cast< _TPBookkeeping const & >( _m_pBookkeeping )->m_numMines > 0);
    _m_pBookkeeping->m_numMines--;
    _onRemovingGeneralObject(mine);
}

void TGameMap::_TImpl::_onGeneratorAdded(const TGenerator& generator)
{
#line 4449
    assert(static_cast< _TPBookkeeping const & >( _m_pBookkeeping )->m_numGenerators < s_kMaxGeneratorsOnMap);
    _onGeneralObjectAdded(generator);
    _m_pBookkeeping->m_numGenerators++;
}

void TGameMap::_TImpl::_onRemovingGenerator(const TGenerator& generator)
{
#line 4459
    assert(static_cast< _TPBookkeeping const & >( _m_pBookkeeping )->m_numGenerators > 0);
    _m_pBookkeeping->m_numGenerators--;
    _onRemovingGeneralObject(generator);
}

void TGameMap::_TImpl::_onSignAdded(const TSign& sign)
{
#line 4469
    assert(static_cast< _TPBookkeeping const & >( _m_pBookkeeping )->m_numSigns < s_kMaxSignsOnMap);
    _onGeneralObjectAdded(sign);
    _m_pBookkeeping->m_numSigns++;
}

void TGameMap::_TImpl::_onRemovingSign(const TSign& sign)
{
#line 4479
    assert(static_cast< _TPBookkeeping const & >( _m_pBookkeeping )->m_numSigns > 0);
    _m_pBookkeeping->m_numSigns--;
    _onRemovingGeneralObject(sign);
}

bool TGameMap::_TImpl::_isMapPlayable() const
{
    return _m_pBookkeeping->m_numPlayableSlots != 0;
}

bool TGameMap::_TImpl::_isHeroAvailable(THeroClass heroClass, unsigned int protoNum) const
{
#line 4495
    assert(heroClass >= 0 && heroClass < kNumHeroClasses);
    assert(protoNum >= 0 && protoNum < THero::s_akClassTraits[ heroClass ].m_numPrototypes);
    return _m_pBookkeeping->m_aabHeroAvailable[heroClass][protoNum];
}

unsigned int TGameMap::_TImpl::_pickAvailableHero(THeroClass heroClass) const
{
#line 4504
    assert(heroClass >= 0 && heroClass < kNumHeroClasses);
    if (_m_pBookkeeping->m_aabHeroAvailable[heroClass].none())
        return THero::s_akClassTraits[heroClass].m_numPrototypes;
    unsigned int pick = rand() % _m_pBookkeeping->m_aabHeroAvailable[heroClass].count();
    unsigned int result;
    for (result = 0;; result++)
        if (_m_pBookkeeping->m_aabHeroAvailable[heroClass][result] && pick-- == 0)
            break;
#line 4519
    assert(result < THero::s_akClassTraits[ heroClass ].m_numPrototypes);
    return result;
}

unsigned int TGameMap::_TImpl::_pickAvailableTeam() const
{
    const TTeamInfo& teamInfo = _m_pProperties->m_teamInfo;
#line 4528
    assert(teamInfo.getBHasTeams());
    unsigned int aTeamSize[TTeamInfo::s_kMaxTeams];
    fill_n(aTeamSize, teamInfo.getNumTeams(), 0U);
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        if (_m_pProperties->m_players[player].getBPresent()) {
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


bool TGameMap::_TImpl::_isValid(const TVictoryCondition& vc) const
{
    _TVictoryConditionValidater validater(*this);
    vc.accept(&validater);
    return validater.isValid();
}

bool TGameMap::_TImpl::_isValid(const TLossCondition& lc) const
{
    _TLossConditionValidater validater(*this);
    lc.accept(&validater);
    return validater.isValid();
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TMapObjectRef& objRef) const
{
#line 4575
    assert(!objRef.getBSecondLayer() || _m_bTwoLayer);
    if (objRef.getObjectID() != TLayer::s_kInvalidObjID) {
        const TLayer& layer = getLayer(objRef.getBSecondLayer());
        const TGameObject& obj = layer.getObject(objRef.getObjectID());
#line 4580
        assert(obj.hasTrigger());
        TTilePoint loc = layer.getObjectLoc(objRef.getObjectID()) - obj.getTriggerLoc();
        *pOStream << static_cast<ubyte>(loc.x()) << static_cast<ubyte>(loc.y())
                  << static_cast<ubyte>(objRef.getBSecondLayer());
    } else {
        *pOStream << std::numeric_limits<ubyte>::max() << std::numeric_limits<ubyte>::max()
                  << std::numeric_limits<ubyte>::max();
    }
}

void TGameMap::_TImpl::_writeVictoryCondition(TRawOStream* pOStream) const
{
    if (_m_pProperties->m_pVictoryCondition != NULL) {
#line 4600
        assert(_isValid( *_m_pProperties->m_pVictoryCondition ));
        _TVictoryConditionWriter writer(*this, pOStream);
        _m_pProperties->m_pVictoryCondition->accept(&writer);
    } else {
        *pOStream << static_cast<signed char>(eVCNone);
    }
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TVictoryCondition& vc, int type) const
{
    *pOStream << (const signed char&) type << static_cast<signed char>(vc.getBAllowNormalVictory())
              << static_cast<signed char>(vc.getBAppliesToComputer());
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TVCAquireArtifact& vc) const
{
    _write(pOStream, vc, eVCAquireArtifact);
    *pOStream << static_cast<signed char>(vc.getArtifact());
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TVCAccumulateCreature& vc) const
{
    _write(pOStream, vc, eVCAccumulateCreature);
    *pOStream << static_cast<signed char>(vc.getCreatureType()) << static_cast<long>(vc.getQuantity());
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TVCAccumulateResource& vc) const
{
    _write(pOStream, vc, eVCAccumulateResource);
    *pOStream << static_cast<signed char>(vc.getResourceType()) << static_cast<long>(vc.getQuantity());
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TVCUpgradeTown& vc) const
{
    _write(pOStream, vc, eVCUpgradeTown);
    _write(pOStream, vc.getTownRef());
    *pOStream << static_cast<signed char>(vc.getHallLevel()) << static_cast<signed char>(vc.getCastleLevel());
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TVCBuildHolyGrailStruct& vc) const
{
    _write(pOStream, vc, eVCBuildHolyGrailStruct);
    _write(pOStream, vc.getTownRef());
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TVCDefeatHero& vc) const
{
    _write(pOStream, vc, eVCDefeatHero);
    _write(pOStream, vc.getHeroRef());
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TVCCaptureTown& vc) const
{
    _write(pOStream, vc, eVCCaptureTown);
    _write(pOStream, vc.getTownRef());
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TVCDefeatMonster& vc) const
{
    _write(pOStream, vc, eVCDefeatMonster);
    _write(pOStream, vc.getMonsterRef());
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TVCFlagAllCreatureGenerators& vc) const
{
    _write(pOStream, vc, eVCFlagAllCreatureGenerators);
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TVCFlagAllMines& vc) const
{
    _write(pOStream, vc, eVCFlagAllMines);
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TVCTransportArtifact& vc) const
{
    _write(pOStream, vc, eVCTransportArtifact);
    *pOStream << (const signed char&) vc.getArtifact();
    _write(pOStream, vc.getTownRef());
}

void TGameMap::_TImpl::_writeLossCondition(TRawOStream* pOStream) const
{
    if (_m_pProperties->m_pLossCondition != NULL) {
#line 4698
        assert(_isValid( *_m_pProperties->m_pLossCondition ));
        _TLossConditionWriter writer(*this, pOStream);
        _m_pProperties->m_pLossCondition->accept(&writer);
    } else {
        *pOStream << static_cast<signed char>(eLCNone);
    }
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TLCLoseTown& lc) const
{
    *pOStream << static_cast<signed char>(eLCLoseTown);
    _write(pOStream, lc.getTownRef());
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TLCLoseHero& lc) const
{
    *pOStream << static_cast<signed char>(eLCLoseHero);
    _write(pOStream, lc.getHeroRef());
}

void TGameMap::_TImpl::_write(TRawOStream* pOStream, const TLCTimeExpires& lc) const
{
    *pOStream << static_cast<signed char>(eLCTimeExpires) << static_cast<short>(lc.getNumDays());
}

TMapLayerObjectID TGameMap::_TImpl::_findObject(bool bSecondLayer, const TTilePoint& loc,
                                                bool (*pfnPredicate)(const TGameObject&)) const
{
#line 4903
    assert(!bSecondLayer || _m_bTwoLayer);
    const TLayer& layer = getLayer(bSecondLayer);
    return layer._findObject(loc, pfnPredicate);
}

const TNonRandomHero* TGameMap::_TImpl::_findPlayersNonRandomHero(TPlayer player) const
{
#line 4911
    assert(player >= 0 && player < kNumPlayers);
    unsigned int numLayers = isTwoLayer() ? 2 : 1;
    for (unsigned int layerNum = 0; layerNum < numLayers; layerNum++) {
        const TLayer& layer = getLayer(layerNum);
        for (TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TPlayableObject* pPlayable = dynamic_cast<const TPlayableObject*>(layer.getPObject(*iter));
            if (pPlayable != NULL && pPlayable->getOwner() == player) {
                const TNonRandomHero* pHero = dynamic_cast<const TNonRandomHero*>(pPlayable);
                if (pHero != NULL)
                    return pHero;
                const TTown* pTown = dynamic_cast<const TTown*>(pPlayable);
                if (pTown != NULL && pTown->getPVisitingHero() != NULL) {
                    pHero = dynamic_cast<const TNonRandomHero*>(pTown->getPVisitingHero());
                    if (pHero != NULL)
                        return pHero;
                }
            }
        }
    }
    return NULL;
}

void TGameMap::streamObject(streambuf* pStreamBuf, const TGameObject& obj)
{
    _TImpl::streamObject(pStreamBuf, obj);
}

TGameMap::TGameMap(const TGameMap& other)
    : _m_pImpl(other._m_pImpl)
{
}

TGameMap::TGameMap(TClient* pClient, const TObjectFactory* pObjectFactory, TSize size, bool bTwoLayer)
    : _m_pImpl(_TImpl(pClient, pObjectFactory, size, bTwoLayer))
{
}

TGameMap::TGameMap(TClient* pClient, const TObjectFactory* pObjectFactory, streambuf* pStreamBuf, int version)
    : _m_pImpl(_TImpl(pClient, pObjectFactory, pStreamBuf, version))
{
}

TGameMap::~TGameMap()
{
}

TGameMap& TGameMap::operator=(const TGameMap& other)
{
    _m_pImpl = other._m_pImpl;
    return *this;
}

void TGameMap::importText(istream* pIStream)
{
    _m_pImpl->importText(pIStream);
}

TGameMap::TLayer* TGameMap::getPLayer(unsigned int num)
{
    return _m_pImpl->getPLayer(num);
}

void TGameMap::setName(const string& newName)
{
    _m_pImpl->setName(newName);
}

void TGameMap::setDesc(const string& newDesc)
{
    _m_pImpl->setDesc(newDesc);
}

void TGameMap::setDifficulty(TDifficulty newDifficulty)
{
    _m_pImpl->setDifficulty(newDifficulty);
}

void TGameMap::setPlayers(const TArray<TPlayerInfo, kNumPlayers>& newPlayers)
{
    _m_pImpl->setPlayers(newPlayers);
}

void TGameMap::setTeamInfo(const TTeamInfo& newTeamInfo)
{
    _m_pImpl->setTeamInfo(newTeamInfo);
}

void TGameMap::setRumors(const vector<TRumor>& newRumors)
{
    _m_pImpl->setRumors(newRumors);
}

void TGameMap::setTimedEvents(const vector<TTimedEvent>& newTimedEvents)
{
    _m_pImpl->setTimedEvents(newTimedEvents);
}

void TGameMap::setVictoryCondition(const TVictoryCondition* pNewVictoryCondition)
{
    _m_pImpl->setVictoryCondition(pNewVictoryCondition);
}

void TGameMap::setLossCondition(const TLossCondition* pNewLossCondition)
{
    _m_pImpl->setLossCondition(pNewLossCondition);
}

TMapLayerObjectID TGameMap::placeObject(bool bSecondLayer, const TGameObject& obj, unsigned int x, unsigned int y,
                                        TTileExtent* pUpdatedExtent)
{
    return _m_pImpl->placeObject(bSecondLayer, obj, x, y, pUpdatedExtent);
}

void TGameMap::removeObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent)
{
    _m_pImpl->removeObject(bSecondLayer, objID, pUpdatedExtent);
}

void TGameMap::floatObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent)
{
    _m_pImpl->floatObject(bSecondLayer, objID, pUpdatedExtent);
}

void TGameMap::unfloatObject(bool bSecondLayer, unsigned int x, unsigned int y, TTileExtent* pUpdatedExtent)
{
    _m_pImpl->unfloatObject(bSecondLayer, x, y, pUpdatedExtent);
}

void TGameMap::removeFloatingObject(bool bSecondLayer)
{
    _m_pImpl->removeFloatingObject(bSecondLayer);
}

bool TGameMap::onTerrainTypeChanged(bool bSecondLayer, unsigned int x, unsigned int y, TTerrainType oldTerrainType,
                                    TTileExtent* pUpdatedExtent)
{
    return _m_pImpl->onTerrainTypeChanged(bSecondLayer, x, y, oldTerrainType, pUpdatedExtent);
}

void TGameMap::onObjectRemoved()
{
    _m_pImpl->onObjectRemoved();
}

void TGameMap::onHeroAdded(const THero& hero)
{
    _m_pImpl->onHeroAdded(hero);
}

void TGameMap::onRemovingHero(const THero& hero)
{
    _m_pImpl->onRemovingHero(hero);
}

void TGameMap::onHeroProtoChanged(THeroClass heroClass, unsigned int oldProtoNum, unsigned int newProtoNum)
{
    _m_pImpl->onHeroProtoChanged(heroClass, oldProtoNum, newProtoNum);
}

void TGameMap::onHeroClassChanged(THeroClass oldHeroClass, unsigned int oldProtoNum, THeroClass newHeroClass,
                                  unsigned int newProtoNum)
{
    _m_pImpl->onHeroClassChanged(oldHeroClass, oldProtoNum, newHeroClass, newProtoNum);
}

void TGameMap::onHeroOwnerChanged(const THero& hero, TPlayer oldOwner)
{
    _m_pImpl->onHeroOwnerChanged(hero, oldOwner);
}

void TGameMap::onTownOwnerChanged(const TTown& town, bool bSecondLayer, unsigned int objID, TPlayer oldOwner)
{
    _m_pImpl->onTownOwnerChanged(town, bSecondLayer, objID, oldOwner);
}

void TGameMap::removeSecondLayer()
{
    _m_pImpl->removeSecondLayer();
}

void TGameMap::addSecondLayer()
{
    _m_pImpl->addSecondLayer();
}

void TGameMap::save(streambuf* pStreamBuf) const
{
    _m_pImpl->save(pStreamBuf);
}

void TGameMap::exportText(ostream* pOStream) const
{
    _m_pImpl->exportText(pOStream);
}

unsigned int TGameMap::getWidth() const
{
    return _m_pImpl->getWidth();
}

unsigned int TGameMap::getHeight() const
{
    return _m_pImpl->getHeight();
}

bool TGameMap::isTwoLayer() const
{
    return _m_pImpl->isTwoLayer();
}

const TGameObject* TGameMap::getPObject(bool bSecondLayer, unsigned int objID) const
{
    return _m_pImpl->getPObject(bSecondLayer, objID);
}

TTilePoint TGameMap::getObjectLoc(bool bSecondLayer, unsigned int objID) const
{
    return _m_pImpl->getObjectLoc(bSecondLayer, objID);
}

const TGameMap::TLayer* TGameMap::getPLayer(unsigned int num) const
{
    return _m_pImpl->getPLayer(num);
}

const string& TGameMap::getName() const
{
    return _m_pImpl->getName();
}

const string& TGameMap::getDesc() const
{
    return _m_pImpl->getDesc();
}

TGameMap::TDifficulty TGameMap::getDifficulty() const
{
    return _m_pImpl->getDifficulty();
}

const TArray<TPlayerInfo, kNumPlayers>& TGameMap::getPlayers() const
{
    return _m_pImpl->getPlayers();
}

const TTeamInfo& TGameMap::getTeamInfo() const
{
    return _m_pImpl->getTeamInfo();
}

const vector<TRumor>& TGameMap::getRumors() const
{
    return _m_pImpl->getRumors();
}

const vector<TTimedEvent>& TGameMap::getTimedEvents() const
{
    return _m_pImpl->getTimedEvents();
}

const TVictoryCondition* TGameMap::getPVictoryCondition() const
{
    return _m_pImpl->getPVictoryCondition();
}

const TLossCondition* TGameMap::getPLossCondition() const
{
    return _m_pImpl->getPLossCondition();
}

bool TGameMap::isPlayerPresent(TPlayer player) const
{
    return _m_pImpl->isPlayerPresent(player);
}

unsigned int TGameMap::getNumPlayableSlots() const
{
    return _m_pImpl->getNumPlayableSlots();
}

TGameObject* TGameMap::createObject(const TObjectType& objType, TPlayer player,
                                    void* (*pfnAllocator)(unsigned int)) const
{
    return _m_pImpl->createObject(objType, player, pfnAllocator);
}

TGameObject* TGameMap::reconstructObject(streambuf* pStreamBuf, int version,
                                         void* (*pfnAllocator)(unsigned int)) const
{
    return _m_pImpl->reconstructObject(pStreamBuf, version, pfnAllocator);
}

bool TGameMap::canCreate(const TObjectType& objType, TPlayer player) const
{
    return _m_pImpl->canCreate(objType, player);
}

bool TGameMap::isHeroOnMap(THeroID heroID) const
{
    return _m_pImpl->isHeroOnMap(heroID);
}

set<unsigned int> TGameMap::getAvailableHeroesInClass(THeroClass heroClass) const
{
    return _m_pImpl->getAvailableHeroesInClass(heroClass);
}

bitset<kNumPlayers> TGameMap::getAvailableHeroOwnersMask() const
{
    return _m_pImpl->getAvailableHeroOwnersMask();
}

const set<TMapObjectRef>& TGameMap::getPlayerTownRefs(TPlayer player) const
{
    return _m_pImpl->getPlayerTownRefs(player);
}

unsigned int TGameMap::getNumTownsOnMap() const
{
    return _m_pImpl->getNumTownsOnMap();
}

bool TGameMap::isGrailOnMap() const
{
    return _m_pImpl->isGrailOnMap();
}

unsigned int TGameMap::getNumObelisksOnMap() const
{
    return _m_pImpl->getNumObelisksOnMap();
}

bool TGameMap::isValidPlacement(const TGameObject& obj, bool bSecondLayer, unsigned int x, unsigned int y) const
{
    return _m_pImpl->isValidPlacement(obj, bSecondLayer, x, y);
}

const TMapLayerObjectID TGameMap::TLayer::s_kInvalidObjID = 0;

class TGameMap::TLayer::_TImpl {
    // A slot in the layer's object list: the doubly linked order of placed
    // objects (slot 0 is the list head), the object's location and its shared,
    // copy-on-write clone.
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

        TGameObject* getPObject()
        {
            if (_m_pWrapper != NULL) {
                if (_m_pWrapper->m_refCnt > 1)
                    _split();
                return _m_pWrapper->m_pObject;
            }
            return NULL;
        }
        const TGameObject* getPObject() const
        {
            return _m_pWrapper != NULL ? _m_pWrapper->m_pObject : NULL;
        }
        void setObject(const TGameObject* pObj)
        {
            if (pObj != NULL) {
#line 5435
                assert(_m_pWrapper == __null);
                _m_pWrapper = new _TWrapper(pObj);
                if (_m_pWrapper == NULL)
                    _fail();
            } else {
#line 5442
                assert(_m_pWrapper != __null);
                if (--_m_pWrapper->m_refCnt == 0)
                    delete _m_pWrapper;
                _m_pWrapper = NULL;
            }
        }

        TMapLayerObjectID m_next;
        TMapLayerObjectID m_prev;
        TTilePoint m_loc;

    private:
        struct _TWrapper {
            _TWrapper(const TGameObject* pObject) : m_refCnt(1), m_pObject(pObject->clone(::operator new))
            {
                if (m_pObject == NULL)
                    _fail();
            }
            ~_TWrapper() { delete m_pObject; }

            void _fail()
            {
#line 5473
                throw TAllocationFailure(__FILE__, __LINE__);
            }

            unsigned int m_refCnt;
            TGameObject* m_pObject;
        };

        void _fail()
        {
#line 5480
            throw TAllocationFailure(__FILE__, __LINE__);
        }
        void _split()
        {
#line 5483
            assert(_m_pWrapper != __null && _m_pWrapper->m_refCnt > 1);
            _TWrapper* pNewWrapper = new _TWrapper(_m_pWrapper->m_pObject);
            if (pNewWrapper == NULL)
                _fail();
            --_m_pWrapper->m_refCnt;
            _m_pWrapper = pNewWrapper;
        }

        _TWrapper* _m_pWrapper;
    };

    // The layer's cells in shared, copy-on-write segments of
    // s_kSegmentDim x s_kSegmentDim.
    class _TCellGrid {
    public:
        _TCellGrid() : _m_widthInSegments(0) {}

        void resize(unsigned int width, unsigned int height)
        {
#line 5506
            assert(width % s_kSegmentDim == 0);
            assert(height % s_kSegmentDim == 0);
            _m_widthInSegments = width / s_kSegmentDim;
            _m_aSegment.resize(height / s_kSegmentDim * _m_widthInSegments);
        }
        TCell* getPCell(unsigned int x, unsigned int y)
        {
            return &(*_m_aSegment[y / s_kSegmentDim * _m_widthInSegments + x / s_kSegmentDim])
                [y % s_kSegmentDim][x % s_kSegmentDim];
        }
        const TCell* getPCell(unsigned int x, unsigned int y) const
        {
            return &(*_m_aSegment[y / s_kSegmentDim * _m_widthInSegments + x / s_kSegmentDim])
                [y % s_kSegmentDim][x % s_kSegmentDim];
        }

    private:
        static const unsigned int s_kSegmentDim = 9;
        typedef TArray<TArray<TCell, s_kSegmentDim>, s_kSegmentDim> _TSegment;

        unsigned int _m_widthInSegments;
        vector<TRefCountingPtr<_TSegment> > _m_aSegment;
    };

public:
    _TImpl(TGameMap::TSize size);
    _TImpl(const _TImpl& other);
    ~_TImpl();
    _TImpl& operator=(const _TImpl& other);

    unsigned int getWidth() const { return TGameMap::_TImpl::_s_akDimension[_m_size]; }
    unsigned int getHeight() const { return TGameMap::_TImpl::_s_akDimension[_m_size]; }
    TCell* getPCell(unsigned int x, unsigned int y);
    TCell* getPCell(const TTilePoint& loc) { return getPCell(loc.x(), loc.y()); }
    const TCell* getPCell(unsigned int x, unsigned int y) const;
    const TCell& getCell(unsigned int x, unsigned int y) const { return *getPCell(x, y); }
    TGameObject* getPObject(unsigned int objID);
    const TGameObject* getPObject(unsigned int objID) const;
    const TGameObject& getObject(unsigned int objID) const { return *getPObject(objID); }
    TTilePoint getObjectLoc(unsigned int objID) const;
    TTileExtent getObjectExtent(unsigned int objID) const;
    TMapLayerObjectID getFloatingObjID() const { return _m_floatingObjID; }
    bool isObjectIDValid(unsigned int objID) const
    {
        return objID < _m_paObjectLink->size() && (*_m_paObjectLink)[objID].getPObject() != NULL;
    }
    TMapLayerObjectID getFirstObjectID() const { return (*_m_paObjectLink)[0].m_next; }
    TMapLayerObjectID getLastObjectID() const { return (*_m_paObjectLink)[0].m_prev; }
#line 5375
    TMapLayerObjectID getNextObjectID(unsigned int objID) const { assert(objID < _m_paObjectLink->size()); return (*_m_paObjectLink)[objID].m_next; }
    TMapLayerObjectID getPrevObjectID(unsigned int objID) const { assert(objID < _m_paObjectLink->size()); return (*_m_paObjectLink)[objID].m_prev; }
    unsigned int getNumObjectIDsAtCell(unsigned int x, unsigned int y) const;
    unsigned int getNumObjectIDsAtCell(const TTilePoint& loc) const { return getNumObjectIDsAtCell(loc.x(), loc.y()); }
    TMapLayerObjectID getObjectIDAtCell(unsigned int x, unsigned int y, unsigned int which) const;
    TMapLayerObjectID getObjectIDAtCell(const TTilePoint& loc, unsigned int which) const
    {
        return getObjectIDAtCell(loc.x(), loc.y(), which);
    }
    unsigned int getNumShadowIDsAtCell(unsigned int x, unsigned int y) const;
    TMapLayerObjectID getShadowIDAtCell(unsigned int x, unsigned int y, unsigned int which) const;

    TMapLayerObjectID _placeObject(const TGameObject& obj, const TTilePoint& loc);
    void _removeObject(unsigned int objID);
    void _floatObject(unsigned int objID);
    void _unfloatObject(const TTilePoint& loc);
    TMapLayerObjectID _findObject(const TTilePoint& loc, bool (*pfnPredicate)(const TGameObject&)) const;

private:
    void _stampObject(const TGameObject* pObj, const TTilePoint& loc, unsigned int objID);
    void _unstampObject(unsigned int objID);
    TTileExtent _computeObjExtent(const TTilePoint& loc, const TPoint<unsigned int>& size) const;

    TGameMap::TSize _m_size;
    TRefCountingPtr<_TCellGrid> _m_pCellGrid;
    TMapLayerObjectID _m_nextAvail;
    TRefCountingPtr<vector<_TObjectLink> > _m_paObjectLink;
    TMapLayerObjectID _m_floatingObjID;
};

TGameMap::TLayer::_TImpl::_TImpl(const _TImpl& other)
    : _m_size(other._m_size), _m_pCellGrid(other._m_pCellGrid), _m_nextAvail(other._m_nextAvail),
      _m_paObjectLink(other._m_paObjectLink), _m_floatingObjID(other._m_floatingObjID)
{
}

TGameMap::TLayer::_TImpl::_TImpl(TGameMap::TSize size)
    : _m_size(size), _m_nextAvail(0), _m_floatingObjID(0)
{
#line 5578
    assert((int) _m_size >= 0 && (int) _m_size < (int) s_kNumSizes);
    _m_pCellGrid->resize(getWidth(), getHeight());
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    aObjectLink.resize(1, _TObjectLink());
    aObjectLink[0].m_next = aObjectLink[0].m_prev = 0;
}

TGameMap::TLayer::_TImpl::~_TImpl()
{
}

TGameMap::TLayer::_TImpl& TGameMap::TLayer::_TImpl::operator=(const _TImpl& other)
{
    if (this != &other) {
        _m_size = other._m_size;
        _m_pCellGrid = other._m_pCellGrid;
        _m_nextAvail = other._m_nextAvail;
        _m_paObjectLink = other._m_paObjectLink;
        _m_floatingObjID = other._m_floatingObjID;
    }
    return *this;
}

TGameMap::TLayer::TCell* TGameMap::TLayer::_TImpl::getPCell(unsigned int x, unsigned int y)
{
#line 5611
    assert(x < getWidth());
    assert(y < getHeight());
    return _m_pCellGrid->getPCell(x, y);
}

TGameObject* TGameMap::TLayer::_TImpl::getPObject(unsigned int objID)
{
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
#line 5620
    assert(objID != 0 && objID < aObjectLink.size());
    TGameObject* pObj = aObjectLink[objID].getPObject();
    assert(pObj != __null);
    return pObj;
}

const TGameMap::TLayer::TCell* TGameMap::TLayer::_TImpl::getPCell(unsigned int x, unsigned int y) const
{
#line 5629
    assert(x < getWidth());
    assert(y < getHeight());
    return _m_pCellGrid->getPCell(x, y);
}

const TGameObject* TGameMap::TLayer::_TImpl::getPObject(unsigned int objID) const
{
    const vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
#line 5639
    assert(objID != 0 && objID < aObjectLink.size());
    const TGameObject* pObj = aObjectLink[objID].getPObject();
    assert(pObj != __null);
    return pObj;
}

TTilePoint TGameMap::TLayer::_TImpl::getObjectLoc(unsigned int objID) const
{
    const vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
#line 5649
    assert(objID != 0 && objID < aObjectLink.size());
    assert(aObjectLink[ objID ].getPObject() != __null);
    return aObjectLink[objID].m_loc;
}

TTileExtent TGameMap::TLayer::_TImpl::getObjectExtent(unsigned int objID) const
{
    const vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
#line 5659
    assert(objID != 0 && objID < aObjectLink.size());
    const TGameObject* pObj = aObjectLink[objID].getPObject();
#line 5662
    assert(pObj != __null);
    return _computeObjExtent(aObjectLink[objID].m_loc, TPoint<unsigned int>(pObj->getWidth(), pObj->getHeight()));
}

inline unsigned int TGameMap::TLayer::_TImpl::getNumObjectIDsAtCell(unsigned int x, unsigned int y) const
{
    const vector<_TObjectCellInfo>* paObjInfo = getCell(x, y)._m_paObjInfo.get();
    return paObjInfo != NULL ? paObjInfo->size() : 0;
}

inline TMapLayerObjectID TGameMap::TLayer::_TImpl::getObjectIDAtCell(unsigned int x, unsigned int y,
                                                                    unsigned int which) const
{
    const vector<_TObjectCellInfo>* paObjInfo = getCell(x, y)._m_paObjInfo.get();
#line 5678
    assert(paObjInfo != __null && paObjInfo->size() > 0);
    assert(which < paObjInfo->size());
    return (*paObjInfo)[which].m_objID;
}

inline unsigned int TGameMap::TLayer::_TImpl::getNumShadowIDsAtCell(unsigned int x, unsigned int y) const
{
    const vector<unsigned int>* paShadowID = getCell(x, y)._m_paShadowID.get();
    return paShadowID != NULL ? paShadowID->size() : 0;
}

inline TMapLayerObjectID TGameMap::TLayer::_TImpl::getShadowIDAtCell(unsigned int x, unsigned int y,
                                                                    unsigned int which) const
{
    const vector<unsigned int>* paShadowID = getCell(x, y)._m_paShadowID.get();
#line 5694
    assert(paShadowID != __null && paShadowID->size() > 0);
    assert(which < paShadowID->size());
    return (*paShadowID)[which];
}

TMapLayerObjectID TGameMap::TLayer::_TImpl::_placeObject(const TGameObject& obj, const TTilePoint& loc)
{
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    TMapLayerObjectID objID;
    if (_m_nextAvail != s_kInvalidObjID) {
#line 5706
        assert(_m_nextAvail < aObjectLink.size());
        objID = _m_nextAvail;
        _m_nextAvail = aObjectLink[_m_nextAvail].m_next;
    } else {
        objID = aObjectLink.size();
        aObjectLink.push_back(_TObjectLink());
    }
    aObjectLink[objID].m_next = _m_nextAvail;
    try {
        aObjectLink[objID].setObject(&obj);
    } catch (...) {
        _m_nextAvail = objID;
        throw;
    }
    aObjectLink[objID].m_next = 0;
    aObjectLink[objID].m_prev = aObjectLink[0].m_prev;
    aObjectLink[aObjectLink[0].m_prev].m_next = objID;
    aObjectLink[0].m_prev = objID;
    aObjectLink[objID].m_loc = loc;
    _stampObject(aObjectLink[objID].getPObject(), loc, objID);
    return objID;
}

void TGameMap::TLayer::_TImpl::_removeObject(unsigned int objID)
{
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
#line 5743
    assert(objID != 0 && objID < aObjectLink.size());
    assert(static_cast< _TObjectLink const & >( aObjectLink[ objID ] ).getPObject() != __null);
    if (objID != _m_floatingObjID)
        _unstampObject(objID);
    else
        _m_floatingObjID = s_kInvalidObjID;
    aObjectLink[aObjectLink[objID].m_prev].m_next = aObjectLink[objID].m_next;
    aObjectLink[aObjectLink[objID].m_next].m_prev = aObjectLink[objID].m_prev;
    aObjectLink[objID].m_next = _m_nextAvail;
    _m_nextAvail = objID;
    aObjectLink[objID].setObject(NULL);
}

void TGameMap::TLayer::_TImpl::_stampObject(const TGameObject* pObj, const TTilePoint& loc, unsigned int objID)
{
#line 5763
    assert(pObj != __null);
    const TTileExtent objExtent = _computeObjExtent(loc, TPoint<unsigned int>(pObj->getWidth(), pObj->getHeight()));
    unsigned int heightMap[TObjectType::kMaxObjWidth][TObjectType::kMaxObjHeight];
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
#line 5836
                            assert(intersect( objOnMapExtent, objExtent ));
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
#line 5859
                                    assert(intersectCell._m_paObjInfo.get() != __null);
                                    const vector<_TObjectCellInfo>& aIntersectCellObjInfo = *intersectCell._m_paObjInfo;
                                    vector<_TObjectCellInfo>::const_iterator pObjOnMapCellInfo = aIntersectCellObjInfo.begin();
#line 5863
                                    assert(pObjOnMapCellInfo != aIntersectCellObjInfo.end());
                                    while (pObjOnMapCellInfo->m_objID != objOnMapID) {
                                        ++pObjOnMapCellInfo;
#line 5868
                                        assert(pObjOnMapCellInfo != aIntersectCellObjInfo.end());
                                    }
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

void TGameMap::TLayer::_TImpl::_unstampObject(unsigned int objID)
{
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
    TTilePoint loc = aObjectLink[objID].m_loc;
    const TGameObject* pObj = static_cast< _TObjectLink const & >( aObjectLink[ objID ] ).getPObject();
    const TTileExtent objExtent = _computeObjExtent(loc, TPoint<unsigned int>(pObj->getWidth(), pObj->getHeight()));
    TTilePoint cell;
    for (cell.y(objExtent.top()); cell.y() < objExtent.bottom(); cell.y(cell.y() + 1)) {
        for (cell.x(objExtent.left()); cell.x() < objExtent.right(); cell.x(cell.x() + 1)) {
            TTilePoint objCell = loc - cell;
            if (pObj->getBCellPlaced(objCell.x(), objCell.y())) {
                TCell* pCell = getPCell(cell);
                vector<_TObjectCellInfo>& aObjInfo = *pCell->_m_paObjInfo;
#line 5930
                assert(aObjInfo.size() > 0);
                vector<_TObjectCellInfo>::iterator pObjInfo = aObjInfo.begin();
                while (pObjInfo->m_objID != objID) {
#line 5933
                    assert(pObjInfo != aObjInfo.end());
                    ++pObjInfo;
                }
                aObjInfo.erase(pObjInfo);
                if (aObjInfo.size() == 0)
                    pCell->_m_paObjInfo.clear();
            }
            if (pObj->getBCellShadow(objCell.x(), objCell.y())) {
                TCell* pCell = getPCell(cell);
                vector<unsigned int>& aShadowID = *pCell->_m_paShadowID;
#line 5946
                assert(aShadowID.size() > 0);
                vector<unsigned int>::iterator pObjID = find(aShadowID.begin(), aShadowID.end(), objID);
                assert(pObjID != aShadowID.end());
                aShadowID.erase(pObjID);
                if (aShadowID.size() == 0)
                    pCell->_m_paShadowID.clear();
            }
        }
    }
}

void TGameMap::TLayer::_TImpl::_floatObject(unsigned int objID)
{
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
#line 5962
    assert(_m_floatingObjID == s_kInvalidObjID);
    assert(objID != 0 && objID < aObjectLink.size());
    assert(static_cast< _TObjectLink const & >( aObjectLink[ objID ] ).getPObject() != __null);
    _unstampObject(objID);
    aObjectLink[aObjectLink[objID].m_prev].m_next = aObjectLink[objID].m_next;
    aObjectLink[aObjectLink[objID].m_next].m_prev = aObjectLink[objID].m_prev;
    aObjectLink[objID].m_next = aObjectLink[objID].m_prev = objID;
    _m_floatingObjID = objID;
}

void TGameMap::TLayer::_TImpl::_unfloatObject(const TTilePoint& loc)
{
    vector<_TObjectLink>& aObjectLink = *_m_paObjectLink;
#line 5980
    assert(_m_floatingObjID != s_kInvalidObjID);
    aObjectLink[_m_floatingObjID].m_loc = loc;
    aObjectLink[_m_floatingObjID].m_next = 0;
    aObjectLink[_m_floatingObjID].m_prev = aObjectLink[0].m_prev;
    aObjectLink[aObjectLink[0].m_prev].m_next = _m_floatingObjID;
    aObjectLink[0].m_prev = _m_floatingObjID;
    _stampObject(static_cast< _TObjectLink const & >( aObjectLink[ _m_floatingObjID ] ).getPObject(), loc,
                 _m_floatingObjID);
    _m_floatingObjID = s_kInvalidObjID;
}

TTileExtent TGameMap::TLayer::_TImpl::_computeObjExtent(const TTilePoint& loc, const TPoint<unsigned int>& size) const
{
#line 5998
    assert(loc.x() + 1 < size.x() || loc.x() - size.x() + 1 < getWidth());
    assert(loc.y() + 1 < size.y() || loc.y() - size.y() + 1 < getHeight());
    TTilePoint lastCell = loc;
    TPoint<unsigned int> clippedSize = size;
    if (clippedSize.x() > lastCell.x() + 1)
        clippedSize.x(lastCell.x() + 1);
    if (lastCell.x() >= getWidth()) {
        clippedSize.x(clippedSize.x() - (lastCell.x() + 1 - getWidth()));
        lastCell.x(getWidth() - 1);
    }
    if (clippedSize.y() > lastCell.y() + 1)
        clippedSize.y(lastCell.y() + 1);
    if (lastCell.y() >= getHeight()) {
        clippedSize.y(clippedSize.y() - (lastCell.y() + 1 - getHeight()));
        lastCell.y(getHeight() - 1);
    }
    return TTileExtent(lastCell + TPoint<unsigned int>(1, 1) - clippedSize, clippedSize);
}

TMapLayerObjectID TGameMap::TLayer::_TImpl::_findObject(const TTilePoint& loc,
                                                        bool (*pfnPredicate)(const TGameObject&)) const
{
    unsigned int numObjs = getNumObjectIDsAtCell(loc);
    for (unsigned int i = 0; i < numObjs; i++) {
        TMapLayerObjectID objID = getObjectIDAtCell(loc, i);
        const TGameObject& obj = getObject(objID);
        TTilePoint objLoc = getObjectLoc(objID);
#line 6036
        assert(objLoc.x() >= loc.x() && objLoc.y() >= loc.y());
        TTilePoint objCell = objLoc - loc;
        if (obj.getBCellTrigger(objCell.x(), objCell.y()) && pfnPredicate(obj))
            return objID;
    }
    return s_kInvalidObjID;
}

TGameMap::TLayer::TLayer(const TLayer& other)
    : _m_pImpl(other._m_pImpl)
{
}

TGameMap::TLayer::TLayer(TSize size)
    : _m_pImpl(_TImpl(size))
{
}

TGameMap::TLayer::~TLayer()
{
}

TGameMap::TLayer& TGameMap::TLayer::operator=(const TLayer& other)
{
    _m_pImpl = other._m_pImpl;
    return *this;
}

TGameMap::TLayer::TCell* TGameMap::TLayer::getPCell(unsigned int x, unsigned int y)
{
    return _m_pImpl->getPCell(x, y);
}

TGameObject* TGameMap::TLayer::getPObject(unsigned int objID)
{
    return _m_pImpl->getPObject(objID);
}

unsigned int TGameMap::TLayer::getWidth() const
{
    return _m_pImpl->getWidth();
}

unsigned int TGameMap::TLayer::getHeight() const
{
    return _m_pImpl->getHeight();
}

const TGameMap::TLayer::TCell* TGameMap::TLayer::getPCell(unsigned int x, unsigned int y) const
{
    return _m_pImpl->getPCell(x, y);
}

const TGameObject* TGameMap::TLayer::getPObject(unsigned int objID) const
{
    return _m_pImpl->getPObject(objID);
}

TTilePoint TGameMap::TLayer::getObjectLoc(unsigned int objID) const
{
    return _m_pImpl->getObjectLoc(objID);
}

TTileExtent TGameMap::TLayer::getObjectExtent(unsigned int objID) const
{
    return _m_pImpl->getObjectExtent(objID);
}

TMapLayerObjectID TGameMap::TLayer::getFloatingObjID() const
{
    return _m_pImpl->getFloatingObjID();
}

bool TGameMap::TLayer::isObjectIDValid(unsigned int objID) const
{
    return _m_pImpl->isObjectIDValid(objID);
}

TMapLayerObjectID TGameMap::TLayer::getFirstObjectID() const
{
    return _m_pImpl->getFirstObjectID();
}

TMapLayerObjectID TGameMap::TLayer::getLastObjectID() const
{
    return _m_pImpl->getLastObjectID();
}

TMapLayerObjectID TGameMap::TLayer::getNextObjectID(unsigned int objID) const
{
    return _m_pImpl->getNextObjectID(objID);
}

TMapLayerObjectID TGameMap::TLayer::getPrevObjectID(unsigned int objID) const
{
    return _m_pImpl->getPrevObjectID(objID);
}

unsigned int TGameMap::TLayer::getNumObjectIDsAtCell(unsigned int x, unsigned int y) const
{
    return _m_pImpl->getNumObjectIDsAtCell(x, y);
}

TMapLayerObjectID TGameMap::TLayer::getObjectIDAtCell(unsigned int x, unsigned int y, unsigned int which) const
{
    return _m_pImpl->getObjectIDAtCell(x, y, which);
}

unsigned int TGameMap::TLayer::getNumShadowIDsAtCell(unsigned int x, unsigned int y) const
{
    return _m_pImpl->getNumShadowIDsAtCell(x, y);
}

TMapLayerObjectID TGameMap::TLayer::getShadowIDAtCell(unsigned int x, unsigned int y, unsigned int which) const
{
    return _m_pImpl->getShadowIDAtCell(x, y, which);
}

TMapLayerObjectID TGameMap::TLayer::_placeObject(const TGameObject& obj, const TTilePoint& loc)
{
    return _m_pImpl->_placeObject(obj, loc);
}

void TGameMap::TLayer::_removeObject(unsigned int objID)
{
    _m_pImpl->_removeObject(objID);
}

void TGameMap::TLayer::_floatObject(unsigned int objID)
{
    _m_pImpl->_floatObject(objID);
}

void TGameMap::TLayer::_unfloatObject(const TTilePoint& loc)
{
    _m_pImpl->_unfloatObject(loc);
}

TMapLayerObjectID TGameMap::TLayer::_findObject(const TTilePoint& loc,
                                                bool (*pfnPredicate)(const TGameObject&)) const
{
    return _m_pImpl->_findObject(loc, pfnPredicate);
}

void readCell(TRawIStream* pIStream, TGameMap::TLayer::TCell* pCell);
void writeCell(TRawOStream* pOStream, const TGameMap::TLayer::TCell& cell, bool bBeachBorder);

void readCellData(TRawIStream& stream, TGameMap::TLayer* pLayer)
{
    for (unsigned int y = 0; y < pLayer->getHeight(); y++)
        for (unsigned int x = 0; x < pLayer->getWidth(); x++)
            readCell(&stream, pLayer->getPCell(x, y));
}

void writeCellData(TRawOStream& stream, const TGameMap::TLayer& layer)
{
    for (unsigned int y = 0; y < layer.getHeight(); y++)
        for (unsigned int x = 0; x < layer.getWidth(); x++)
            writeCell(&stream, layer.getCell(x, y), isBeachBorder(layer, TTilePoint(x, y)));
}

void TGameMap::TLayer::TCell::setTerrainType(TTerrainType newTerrainType)
{
#line 6244
    assert(newTerrainType >= 0);
    assert(newTerrainType < kNumTerrainTypes);
    _m_terrainType = newTerrainType;
}

void TGameMap::TLayer::TCell::setTileNum(unsigned int newTileNum)
{
    _m_tileNum = newTileNum;
}

void TGameMap::TLayer::TCell::setRiverType(TRiverType newRiverType)
{
#line 6258
    assert(newRiverType >= 0);
    assert(newRiverType < kNumRiverTypes);
    _m_riverType = newRiverType;
}

void TGameMap::TLayer::TCell::setRiverTileNum(unsigned int newTileNum)
{
    _m_riverTileNum = newTileNum;
}

void TGameMap::TLayer::TCell::setRoadType(TRoadType newRoadType)
{
#line 6272
    assert(newRoadType >= 0);
    assert(newRoadType < kNumRoadTypes);
    _m_roadType = newRoadType;
}

void TGameMap::TLayer::TCell::setRoadTileNum(unsigned int newTileNum)
{
    _m_roadTileNum = newTileNum;
}

void readCell(TRawIStream* pIStream, TGameMap::TLayer::TCell* pCell)
{
    signed char terrainType;
    signed char tileNum;
    signed char riverType;
    signed char riverTileNum;
    signed char roadType;
    signed char roadTileNum;
    unsigned char flags;
    *pIStream >> terrainType >> tileNum >> riverType >> riverTileNum >> roadType >> roadTileNum >> flags;
    pCell->setTerrainType(TTerrainType(terrainType));
    pCell->setTileNum(tileNum);
    pCell->setBHFlipped(flags & 1);
    pCell->setBVFlipped((flags >> 1) & 1);
    pCell->setRiverType(TRiverType(riverType));
    pCell->setRiverTileNum(riverTileNum);
    pCell->setBRiverHFlipped((flags >> 2) & 1);
    pCell->setBRiverVFlipped((flags >> 3) & 1);
    pCell->setRoadType(TRoadType(roadType));
    pCell->setRoadTileNum(roadTileNum);
    pCell->setBRoadHFlipped((flags >> 4) & 1);
    pCell->setBRoadVFlipped((flags >> 5) & 1);
}

void writeCell(TRawOStream* pOStream, const TGameMap::TLayer::TCell& cell, bool bBeachBorder)
{
    unsigned char flags = (cell.getBHFlipped() ? 1 : 0)
                        | (cell.getBVFlipped() ? 2 : 0)
                        | (cell.getBRiverHFlipped() ? 4 : 0)
                        | (cell.getBRiverVFlipped() ? 8 : 0)
                        | (cell.getBRoadHFlipped() ? 0x10 : 0)
                        | (cell.getBRoadVFlipped() ? 0x20 : 0)
                        | (bBeachBorder ? 0x40 : 0);
    *pOStream << static_cast<signed char>(cell.getTerrainType())
              << static_cast<signed char>(cell.getTileNum())
              << static_cast<signed char>(cell.getRiverType())
              << static_cast<signed char>(cell.getRiverTileNum())
              << static_cast<signed char>(cell.getRoadType())
              << static_cast<signed char>(cell.getRoadTileNum())
              << flags;
}
