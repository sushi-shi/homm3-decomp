// GameMap.cpp - Loki h3maped object 12: the map model. TGameMap and
// TGameMap::TLayer forward to their copy-on-write implementations; this
// file defines both implementations, the cell, the rumors, players and
// teams, the object bookkeeping and the binary and text forms.
//
// Reconstruction in progress: the layer handle and the cell come first.
#include "editor/stdafx.h"

#include <assert.h>
#include <ctype.h>
#include <algorithm>
#include <functional>
#include <iostream.h>
#include <map>

#include "adventureobjecttype.h"
#include "editor/GameMap.h"
#include "editor/GameObject.h"
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

TCappedObjectTypeInfoMap kCappedObjectTypeInfoMap;

}  // namespace

class TGameMap::_TImpl {
public:
    static const unsigned int s_kMaxHeroesOnMap = 128;
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
    void _removeObjectHelper(bool bSecondLayer, unsigned int objID);
    bool _isMapPlayable() const;
    TMapLayerObjectID _findObject(bool bSecondLayer, const TTilePoint& loc,
                                  bool (*pfnPredicate)(const TGameObject&)) const;

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
        TArray<unsigned int, kNumPlayers> m_aNumTownsOfType;
        unsigned int m_numHeroes;
        unsigned int m_numRandomHeroes;
        TArray<unsigned int, kNumPlayers> m_aNumHeroesOfType;
    };

    static bool _isValidPlacement(const TLayer& layer, const TGameObject& obj, unsigned int x, unsigned int y);

    TClient* _m_pClient;
    const TObjectFactory* _m_pObjectFactory;
    TSize _m_size;
    bool _m_bTwoLayer;
    TRefCountingPtr<_TProperties> _m_pProperties;
    vector<TLayer> _m_aLayer;
    TRefCountingPtr<_TBookkeeping> _m_pBookkeeping;
    TArray<TRefCountingPtr<_TPlayerBookkeeping>, kNumPlayers> _m_apPlayerBookkeeping;
};

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

const set<TMapObjectRef>& TGameMap::_TImpl::getPlayerTownRefs(TPlayer player) const
{
#line 3034
    assert(player >= 0 && player < kNumPlayers);
    return _m_apPlayerBookkeeping[player]->m_townRefs;
}

bool TGameMap::_TImpl::_isMapPlayable() const
{
    return _m_pBookkeeping->m_numPlayableSlots != 0;
}

TMapLayerObjectID TGameMap::_TImpl::_findObject(bool bSecondLayer, const TTilePoint& loc,
                                                bool (*pfnPredicate)(const TGameObject&)) const
{
#line 4903
    assert(!bSecondLayer || _m_bTwoLayer);
    const TLayer& layer = getLayer(bSecondLayer);
    return layer._findObject(loc, pfnPredicate);
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
