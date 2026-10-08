// GameMap.h - the editor's map model (Loki h3maped GameMap.cpp).
// TGameMap and its TLayer are copy-on-write handles: each is a
// TRefCountingPtr to a private _TImpl defined in GameMap.cpp, and every
// public member forwards through the handle's operator-> (non-const
// members split a shared implementation first). A layer is a grid of
// TCells; a cell packs its terrain, river and road types and tiles and
// their flip flags into one word (setters assert the type ranges,
// GameMap.cpp:6244-6273).
//
// Declared so far: the members the matched code of GameMap.cpp proves
// with their mangled signatures and __PRETTY_FUNCTION__ return types.
// getAvailableHeroesInClass returns the set of free prototype numbers of a
// class, getAvailableHeroOwnersMask a bitset indexed by player (its users
// test `getAvailableHeroOwnersMask()[ _m_owner ]`).
#ifndef HOMM3_EDITOR_GAMEMAP_H
#define HOMM3_EDITOR_GAMEMAP_H

#include <exception>
#include <iterator>
#include <stddef.h>
#include <vector>
#include <set>
#include <string>
#include <bitset>

#include "editor/GameObject.h"
#include "editor/TimedEvent.h"
#include "artifact.h"
#include "herodefs.h"
#include "terrain.h"
#include "terrain_type.h"
#include "editor/Array.h"
#include "editor/MapObjectRef.h"
#include "editor/Player.h"
#include "editor/Point.h"
#include "editor/Uncopyable.h"
#include "editor/VictoryCondition.h"
#include "exceptions.h"
#include "editor/RefCountingPtr.h"

class istream;
class ostream;
class streambuf;
class TRawIStream;
class TRawOStream;
class TGameObject;
class TObjectType;
class THero;
class TTown;
class TGenericObject;
class TNonRandomHero;
class TRandomHero;
class TPrison;
class TEvent;
class TMonster;
class TSign;
class TFlaggableObject;
class TMine;
class TAbandonedMine;
class TGenerator;
class TGarrison;
class TGameArtifact;
class TSpellScroll;
class TGameResource;
class TBlackBox;
class TScholar;
class TSeersHut;
class THolyGrail;
class TShrine;
class TVictoryCondition;
class TLossCondition;
class TTimedEvent;

enum TRiverType {
    kNumRiverTypes = 5
};

enum TRoadType {
    kNumRoadTypes = 4
};

typedef unsigned int TMapLayerObjectID;

// A rumor: a name and a text of at most s_kMaxTextLen characters, either
// both empty or both with something besides white space.
class TRumor {
public:
    class TImportTextFailure : public exception {
    };

    static const unsigned int s_kMaxTextLen = 300;

    TRumor() {}

    const string& getName() const { return _m_name; }
    const string& getText() const { return _m_text; }
    void setNameAndText(const string& newName, const string& newText);

    void importText(istream* pIStream);
    void exportText(ostream* pOStream) const;

    friend bool operator==(const TRumor& lhs, const TRumor& rhs);

private:
    string _m_name;
    string _m_text;
};

// Inline: the map specifications' rumors page keeps the linkonce copies.
inline bool operator==(const TRumor& lhs, const TRumor& rhs)
{
    return lhs._m_name == rhs._m_name && lhs._m_text == rhs._m_text;
}

inline bool operator!=(const TRumor& lhs, const TRumor& rhs)
{
    return !(lhs == rhs);
}

TRawIStream& operator>>(TRawIStream& stream, TRumor& rumor);
TRawOStream& operator<<(TRawOStream& stream, const TRumor& rumor);

// Who may play a player, how the computer plays it, whether a hero is
// generated at its main town, and that town.
class TPlayerInfo {
public:
    enum TBehaviorType {
    };

    static const int s_kNumBehaviorTypes = 4;

    TPlayerInfo()
        : _m_bHumanPlayable(false), _m_bComputerPlayable(false), _m_behaviorType(TBehaviorType(0)),
          _m_bGenerateHero(false) {}

    void setBHumanPlayable(bool bPlayable) { _m_bHumanPlayable = bPlayable; }
    void setBComputerPlayable(bool bPlayable) { _m_bComputerPlayable = bPlayable; }
    void setBehaviorType(TBehaviorType newBehaviorType);
    void setBGenerateHero(bool bGenerate) { _m_bGenerateHero = bGenerate; }
    void setMainTownRef(const TMapObjectRef& newMainTownRef) { _m_mainTownRef = newMainTownRef; }

    bool getBPresent() const { return _m_bHumanPlayable || _m_bComputerPlayable; }
    bool getBHumanPlayable() const { return _m_bHumanPlayable; }
    bool getBComputerPlayable() const { return _m_bComputerPlayable; }
    TBehaviorType getBehaviorType() const { return _m_behaviorType; }
    bool getBGenerateHero() const { return _m_bGenerateHero; }
    const TMapObjectRef& getMainTownRef() const { return _m_mainTownRef; }

    friend bool operator==(const TPlayerInfo& lhs, const TPlayerInfo& rhs);

private:
    bool _m_bHumanPlayable;
    bool _m_bComputerPlayable;
    TBehaviorType _m_behaviorType;
    bool _m_bGenerateHero;
    TMapObjectRef _m_mainTownRef;
};

// Inline: the map specifications' player page keeps the linkonce copies.
inline bool operator==(const TPlayerInfo& lhs, const TPlayerInfo& rhs)
{
    return lhs._m_bHumanPlayable == rhs._m_bHumanPlayable && lhs._m_bComputerPlayable == rhs._m_bComputerPlayable
           && lhs._m_behaviorType == rhs._m_behaviorType && lhs._m_bGenerateHero == rhs._m_bGenerateHero
           && lhs._m_mainTownRef == rhs._m_mainTownRef;
}

inline bool operator!=(const TPlayerInfo& lhs, const TPlayerInfo& rhs)
{
    return !(lhs == rhs);
}

// The alliances: whether the map has teams, how many, and each player's.
class TTeamInfo {
public:
    static const int s_kMinTeams = 2;
    static const int s_kMaxTeams = 7;

    TTeamInfo() : _m_bHasTeams(false), _m_numTeams(s_kMinTeams), _m_aPlayerTeam(0) {}

    void setBHasTeams(bool bHasTeams) { _m_bHasTeams = bHasTeams; }
    bool getBHasTeams() const { return _m_bHasTeams; }
    unsigned int getNumTeams() const { return _m_numTeams; }
    void setNumTeams(unsigned int newNumTeams);
    unsigned int getPlayerTeam(TPlayer player) const;
    void setPlayerTeam(TPlayer player, unsigned int newTeam);

    friend TRawIStream& operator>>(TRawIStream& stream, TTeamInfo& teamInfo);
    friend TRawOStream& operator<<(TRawOStream& stream, const TTeamInfo& teamInfo);
    friend bool operator==(const TTeamInfo& lhs, const TTeamInfo& rhs);

private:
    bool _m_bHasTeams;
    unsigned int _m_numTeams;
    TArray<unsigned int, kNumPlayers> _m_aPlayerTeam;
};

// Inline: the map specifications' teams page keeps the linkonce copies.
inline bool operator==(const TTeamInfo& lhs, const TTeamInfo& rhs)
{
    return lhs._m_bHasTeams == rhs._m_bHasTeams && lhs._m_numTeams == rhs._m_numTeams
           && lhs._m_aPlayerTeam == rhs._m_aPlayerTeam;
}

inline bool operator!=(const TTeamInfo& lhs, const TTeamInfo& rhs)
{
    return !(lhs == rhs);
}

// Why an object could not be created...
class TCreateObjectFailure : public exception {
};

class TCreateObjFailureTooManyInstancesOfTypeOnMap : public TCreateObjectFailure {
public:
    TCreateObjFailureTooManyInstancesOfTypeOnMap(int type, unsigned int cap) : _m_type(type), _m_cap(cap) {}

    int getType() const { return _m_type; }
    unsigned int getCap() const { return _m_cap; }

private:
    int _m_type;
    unsigned int _m_cap;
};

class TCreateObjFailureNoAvailableHeroesInClass : public TCreateObjectFailure {
};

class TCreateObjFailureNoOwnerForHero : public TCreateObjectFailure {
};

class TCreateObjFailureTooManyHeroesOnMap : public TCreateObjFailureTooManyInstancesOfTypeOnMap {
public:
    TCreateObjFailureTooManyHeroesOnMap();
};

class TCreateObjFailureTooManyHeroesForPlayer : public TCreateObjectFailure {
};

class TCreateObjFailureTooManyTownsOnMap : public TCreateObjFailureTooManyInstancesOfTypeOnMap {
public:
    TCreateObjFailureTooManyTownsOnMap();
};

class TCreateObjFailureHolyGrailAlreadyPlaced : public TCreateObjectFailure {
};

class TCreateObjFailureTooManyMinesOnMap : public TCreateObjFailureTooManyInstancesOfTypeOnMap {
public:
    TCreateObjFailureTooManyMinesOnMap();
};

class TCreateObjFailureTooManyGeneratorsOnMap : public TCreateObjFailureTooManyInstancesOfTypeOnMap {
public:
    TCreateObjFailureTooManyGeneratorsOnMap();
};

class TCreateObjFailureTooManySignsOnMap : public TCreateObjFailureTooManyInstancesOfTypeOnMap {
public:
    TCreateObjFailureTooManySignsOnMap();
};

// ...and why it could not be placed.
class TPlaceObjectFailure : public exception {
};

class TPlaceObjFailureTooManyInstancesOfTypeOnMap : public TPlaceObjectFailure {
public:
    TPlaceObjFailureTooManyInstancesOfTypeOnMap(int type, unsigned int cap) : _m_type(type), _m_cap(cap) {}

    int getType() const { return _m_type; }
    unsigned int getCap() const { return _m_cap; }

private:
    int _m_type;
    unsigned int _m_cap;
};

class TPlaceObjFailureInvalidPlacement : public TPlaceObjectFailure {
};

class TPlaceObjFailureNoAvailableHeroesInClass : public TPlaceObjectFailure {
};

class TPlaceObjFailureTooManyHeroesOnMap : public TPlaceObjFailureTooManyInstancesOfTypeOnMap {
public:
    TPlaceObjFailureTooManyHeroesOnMap();
};

class TPlaceObjFailureTooManyHeroesForPlayer : public TPlaceObjectFailure {
};

class TPlaceObjFailureTooManyTownsOnMap : public TPlaceObjFailureTooManyInstancesOfTypeOnMap {
public:
    TPlaceObjFailureTooManyTownsOnMap();
};

class TPlaceObjFailureHolyGrailTooCloseToEdge : public TPlaceObjFailureInvalidPlacement {
};

class TPlaceObjFailureHolyGrailAlreadyPlaced : public TPlaceObjectFailure {
};

class TPlaceObjFailureTooManyMinesOnMap : public TPlaceObjFailureTooManyInstancesOfTypeOnMap {
public:
    TPlaceObjFailureTooManyMinesOnMap();
};

class TPlaceObjFailureTooManyGeneratorsOnMap : public TPlaceObjFailureTooManyInstancesOfTypeOnMap {
public:
    TPlaceObjFailureTooManyGeneratorsOnMap();
};

class TPlaceObjFailureTooManySignsOnMap : public TPlaceObjFailureTooManyInstancesOfTypeOnMap {
public:
    TPlaceObjFailureTooManySignsOnMap();
};

class TGameMap {
    class _TImpl;

public:
    // The map's client (TMapDoc): one pure virtual, first in TMapDoc's
    // vtable. Defined in the class: the image emits its type_info before
    // TImportTextFailure's.
    class TClient {
    public:
        virtual void onMapObjectRemoved(bool bUnderground, TMapLayerObjectID objID) = 0;
    };

    class TObjectFactory;

    // importText's failure: a section, a frame line or an object header
    // that does not read back (the rumor, timed event and object failures
    // are translated to it).
    class TImportTextFailure : public exception {
    };

    enum TSize {
        s_kNumSizes = 4
    };

    enum TDifficulty {
        s_kNumDifficulties = 5
    };

    class TLayer;

    TGameMap(TClient* pClient, const TObjectFactory* pObjectFactory, TSize size, bool bTwoLayer);
    TGameMap(TClient* pClient, const TObjectFactory* pObjectFactory, streambuf* pStreamBuf, int version);
    TGameMap(const TGameMap& other);
    ~TGameMap();
    TGameMap& operator=(const TGameMap& other);

    static void streamObject(streambuf* pStreamBuf, const TGameObject& obj);

    void importText(istream* pIStream);
    void exportText(ostream* pOStream) const;
    void save(streambuf* pStreamBuf) const;

    unsigned int getWidth() const;
    unsigned int getHeight() const;
    bool isTwoLayer() const;
    void removeSecondLayer();
    void addSecondLayer();
    TLayer* getPLayer(unsigned int num);
    const TLayer* getPLayer(unsigned int num) const;
    TLayer* getPLayer(bool bSecondLayer) { return getPLayer(bSecondLayer ? 1U : 0U); }
    const TLayer* getPLayer(bool bSecondLayer) const { return getPLayer(bSecondLayer ? 1U : 0U); }
    const TLayer& getLayer(unsigned int num) const;
    const TLayer& getLayer(bool bSecondLayer) const;

    const string& getName() const;
    void setName(const string& newName);
    const string& getDesc() const;
    void setDesc(const string& newDesc);
    TDifficulty getDifficulty() const;
    void setDifficulty(TDifficulty newDifficulty);
    const TArray<TPlayerInfo, kNumPlayers>& getPlayers() const;
    void setPlayers(const TArray<TPlayerInfo, kNumPlayers>& newPlayers);
    const TTeamInfo& getTeamInfo() const;
    void setTeamInfo(const TTeamInfo& newTeamInfo);
    const vector<TRumor>& getRumors() const;
    void setRumors(const vector<TRumor>& newRumors);
    const vector<TTimedEvent>& getTimedEvents() const;
    void setTimedEvents(const vector<TTimedEvent>& newTimedEvents);
    const TVictoryCondition* getPVictoryCondition() const;
    void setVictoryCondition(const TVictoryCondition* pNewVictoryCondition);
    const TLossCondition* getPLossCondition() const;
    void setLossCondition(const TLossCondition* pNewLossCondition);
    bool isPlayerPresent(TPlayer player) const;
    unsigned int getNumPlayableSlots() const;

    TMapLayerObjectID placeObject(bool bSecondLayer, const TGameObject& obj, unsigned int x, unsigned int y,
                                  TTileExtent* pUpdatedExtent);
    void removeObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent);
    void floatObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent);
    void unfloatObject(bool bSecondLayer, unsigned int x, unsigned int y, TTileExtent* pUpdatedExtent);
    void removeFloatingObject(bool bSecondLayer);
    bool isValidPlacement(const TGameObject& obj, bool bSecondLayer, unsigned int x, unsigned int y) const;
    const TGameObject* getPObject(bool bSecondLayer, unsigned int objID) const;
    TTilePoint getObjectLoc(bool bSecondLayer, unsigned int objID) const;
    const TGameObject* getPObject(const TMapObjectRef& ref) const
    {
        return getPObject(ref.getBSecondLayer(), ref.getObjectID());
    }
    TTilePoint getObjectLoc(const TMapObjectRef& ref) const
    {
        return getObjectLoc(ref.getBSecondLayer(), ref.getObjectID());
    }

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
    unsigned int getNumTownsOnMap() const;
    bool isGrailOnMap() const;
    unsigned int getNumObelisksOnMap() const;

private:
    friend class TLayer;

    TRefCountingPtr<_TImpl> _m_pImpl;
};

// Defined after the class: MapValidation.o writes getLayer after the
// in-class getObjectLoc(const TMapObjectRef&), MapView.o the unsigned
// overload before the bool one.
inline const TGameMap::TLayer& TGameMap::getLayer(unsigned int num) const
{
    return *getPLayer(num);
}

inline const TGameMap::TLayer& TGameMap::getLayer(bool bSecondLayer) const
{
    return *getPLayer(bSecondLayer);
}

// Creates the map's objects (TGUIGameObjectFactory in the editor): for each
// kind a fresh object and one read from a stream, both placed with
// pfnAllocator. The vtable order is TGUIGameObjectFactory's; the
// constructor builds a TUncopyable first, and the vtable pointer follows
// that empty base at +4.
class TGameMap::TObjectFactory : private TUncopyable {
public:
    virtual ~TObjectFactory() {}
    virtual TGenericObject* createGenericObject(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TGenericObject* createGenericObject(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TNonRandomHero* createNonRandomHero(const TObjectType& objType, TPlayer owner, unsigned int protoNum,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TNonRandomHero* createNonRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TRandomHero* createRandomHero(const TObjectType& objType, TPlayer owner,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TRandomHero* createRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TPrison* createPrison(const TObjectType& objType, THeroClass heroClass, unsigned int protoNum,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TPrison* createPrison(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TTown* createTown(const TObjectType& objType, TPlayer owner,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TTown* createTown(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TEvent* createEvent(const TObjectType& objType, void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TEvent* createEvent(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TMonster* createMonster(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TMonster* createMonster(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TSign* createSign(const TObjectType& objType, void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TSign* createSign(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TFlaggableObject* createFlaggable(const TObjectType& objType, TPlayer owner,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TFlaggableObject* createFlaggable(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TMine* createMine(const TObjectType& objType, TPlayer owner,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TMine* createMine(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TAbandonedMine* createAbandonedMine(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TAbandonedMine* createAbandonedMine(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TGenerator* createGenerator(const TObjectType& objType, TPlayer owner,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TGenerator* createGenerator(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TGarrison* createGarrison(const TObjectType& objType, TPlayer owner,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TGarrison* createGarrison(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TGameArtifact* createArtifact(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TGameArtifact* createArtifact(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TSpellScroll* createSpellScroll(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TSpellScroll* createSpellScroll(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TGameResource* createResource(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TGameResource* createResource(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TBlackBox* createBlackBox(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TBlackBox* createBlackBox(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TScholar* createScholar(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TScholar* createScholar(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TSeersHut* createSeersHut(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TSeersHut* createSeersHut(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual THolyGrail* createHolyGrail(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual THolyGrail* createHolyGrail(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TShrine* createShrine(const TObjectType& objType, void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual TShrine* createShrine(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const = 0;
};

class TGameMap::TLayer {
public:
    static const TMapLayerObjectID s_kInvalidObjID;

    class TCell;

    // Walks a layer's placed objects in link order.
    // The layer creates its iterators (objectIDBegin, objectIDEnd).
    class TObjectIDIter : public forward_iterator<TMapLayerObjectID, ptrdiff_t> {
    public:
        TObjectIDIter& operator++()
        {
            _m_objID = _m_pLayer->getNextObjectID(_m_objID);
            return *this;
        }
        TObjectIDIter operator++(int)
        {
            TObjectIDIter result = *this;
            ++*this;
            return result;
        }
        TMapLayerObjectID operator*() const { return _m_objID; }
        bool operator==(const TObjectIDIter& other) const
        {
            return _m_pLayer == other._m_pLayer && _m_objID == other._m_objID;
        }
        bool operator!=(const TObjectIDIter& other) const { return !(*this == other); }

    private:
        friend class TLayer;

        TObjectIDIter(const TLayer* pLayer, unsigned int objID) : _m_pLayer(pLayer), _m_objID(objID) {}

        const TLayer* _m_pLayer;
        TMapLayerObjectID _m_objID;
    };

    TLayer(TSize size);
    TLayer(const TLayer& other);
    ~TLayer();
    TLayer& operator=(const TLayer& other);

    unsigned int getWidth() const;
    unsigned int getHeight() const;
    TCell* getPCell(unsigned int x, unsigned int y);
    const TCell* getPCell(unsigned int x, unsigned int y) const;
    TCell* getPCell(const TTilePoint& loc) { return getPCell(loc.x(), loc.y()); }
    const TCell& getCell(unsigned int x, unsigned int y) const { return *getPCell(x, y); }
    const TCell& getCell(const TTilePoint& loc) const { return getCell(loc.x(), loc.y()); }
    TGameObject* getPObject(unsigned int objID);
    const TGameObject* getPObject(unsigned int objID) const;
    const TGameObject& getObject(unsigned int objID) const { return *getPObject(objID); }
    TTilePoint getObjectLoc(unsigned int objID) const;
    TTileExtent getObjectExtent(unsigned int objID) const;
    TMapLayerObjectID getFloatingObjID() const;
    bool isObjectIDValid(unsigned int objID) const;
    TMapLayerObjectID getFirstObjectID() const;
    TMapLayerObjectID getLastObjectID() const;
    TMapLayerObjectID getNextObjectID(unsigned int objID) const;
    TMapLayerObjectID getPrevObjectID(unsigned int objID) const;
    TObjectIDIter objectIDBegin() const { return TObjectIDIter(this, getFirstObjectID()); }
    TObjectIDIter objectIDEnd() const { return TObjectIDIter(this, s_kInvalidObjID); }
    unsigned int getNumObjectIDsAtCell(unsigned int x, unsigned int y) const;
    TMapLayerObjectID getObjectIDAtCell(unsigned int x, unsigned int y, unsigned int which) const;
    unsigned int getNumObjectIDsAtCell(const TTilePoint& loc) const { return getNumObjectIDsAtCell(loc.x(), loc.y()); }
    TMapLayerObjectID getObjectIDAtCell(const TTilePoint& loc, unsigned int which) const
    {
        return getObjectIDAtCell(loc.x(), loc.y(), which);
    }
    unsigned int getNumShadowIDsAtCell(unsigned int x, unsigned int y) const;
    TMapLayerObjectID getShadowIDAtCell(unsigned int x, unsigned int y, unsigned int which) const;

private:
    class _TImpl;
    friend class TGameMap;
    friend class TGameMap::_TImpl;

    // One object's footprint record in a cell: the object and the
    // height of its placed cell there.
    struct _TObjectCellInfo {
        _TObjectCellInfo(unsigned int objID, unsigned int height) : m_objID(objID), m_height(height) {}

        unsigned int m_objID;
        unsigned int m_height;
    };

    TMapLayerObjectID _placeObject(const TGameObject& obj, const TTilePoint& loc);
    TMapLayerObjectID _placeObject(const TGameObject& obj, unsigned int x, unsigned int y)
    {
        return _placeObject(obj, TTilePoint(x, y));
    }
    void _removeObject(unsigned int objID);
    void _floatObject(unsigned int objID);
    void _unfloatObject(const TTilePoint& loc);
    TMapLayerObjectID _findObject(const TTilePoint& loc, bool (*pfnPredicate)(const TGameObject&)) const;

    TRefCountingPtr<_TImpl> _m_pImpl;
};

class TGameMap::TLayer::TCell {
public:
    TCell()
        : _m_terrainType(eTerrainWater), _m_riverType(0), _m_roadType(0), _m_tileNum(0),
          _m_riverTileNum(0), _m_roadTileNum(0), _m_bHFlipped(false), _m_bVFlipped(false),
          _m_bRiverHFlipped(false), _m_bRiverVFlipped(false), _m_bRoadHFlipped(false),
          _m_bRoadVFlipped(false) {}

    void setTerrainType(TTerrainType newTerrainType);
    void setTileNum(unsigned int newTileNum);
    void setBHFlipped(bool bFlipped) { _m_bHFlipped = bFlipped; }
    void setBVFlipped(bool bFlipped) { _m_bVFlipped = bFlipped; }
    void setRiverType(TRiverType newRiverType);
    void setRiverTileNum(unsigned int newTileNum);
    void setBRiverHFlipped(bool bFlipped) { _m_bRiverHFlipped = bFlipped; }
    void setBRiverVFlipped(bool bFlipped) { _m_bRiverVFlipped = bFlipped; }
    void setRoadType(TRoadType newRoadType);
    void setRoadTileNum(unsigned int newTileNum);
    void setBRoadHFlipped(bool bFlipped) { _m_bRoadHFlipped = bFlipped; }
    void setBRoadVFlipped(bool bFlipped) { _m_bRoadVFlipped = bFlipped; }

    TTerrainType getTerrainType() const { return TTerrainType(_m_terrainType); }
    unsigned int getTileNum() const { return _m_tileNum; }
    bool getBHFlipped() const { return _m_bHFlipped; }
    bool getBVFlipped() const { return _m_bVFlipped; }
    TRiverType getRiverType() const { return TRiverType(_m_riverType); }
    unsigned int getRiverTileNum() const { return _m_riverTileNum; }
    bool getBRiverHFlipped() const { return _m_bRiverHFlipped; }
    bool getBRiverVFlipped() const { return _m_bRiverVFlipped; }
    TRoadType getRoadType() const { return TRoadType(_m_roadType); }
    unsigned int getRoadTileNum() const { return _m_roadTileNum; }
    bool getBRoadHFlipped() const { return _m_bRoadHFlipped; }
    bool getBRoadVFlipped() const { return _m_bRoadVFlipped; }

// A lazily created, shared and copy-on-write _TObjectCellInfo vector.
class _TPObjectCellInfoList {
public:
    _TPObjectCellInfoList() : _m_pWrapper(NULL) {}
    _TPObjectCellInfoList(const _TPObjectCellInfoList& other) : _m_pWrapper(other._m_pWrapper)
    {
        if (_m_pWrapper != NULL)
            ++_m_pWrapper->m_refCnt;
    }
    ~_TPObjectCellInfoList()
    {
        if (_m_pWrapper != NULL && --_m_pWrapper->m_refCnt == 0)
            delete _m_pWrapper;
    }

    void construct()
    {
        if ((_m_pWrapper = new _TWrapper) == NULL)
            _fail();
    }
    void clear()
    {
        if (--_m_pWrapper->m_refCnt == 0)
            delete _m_pWrapper;
        _m_pWrapper = NULL;
    }

    vector<_TObjectCellInfo>* get()
    {
        if (_m_pWrapper == NULL)
            construct();
        else if (_m_pWrapper->m_refCnt > 1)
            _split();
        return &_m_pWrapper->m_a;
    }
    vector<_TObjectCellInfo>& operator*() { return *get(); }
    const vector<_TObjectCellInfo>* get() const
    {
        return _m_pWrapper != NULL ? &_m_pWrapper->m_a : NULL;
    }
    const vector<_TObjectCellInfo>& operator*() const { return *get(); }

private:
    struct _TWrapper {
        _TWrapper() : m_refCnt(1) {}
        _TWrapper(const vector<_TObjectCellInfo>& a) : m_refCnt(1), m_a(a) {}

        unsigned int m_refCnt;
        vector<_TObjectCellInfo> m_a;
    };

    void _split()
    {
        _TWrapper* pNewWrapper = new _TWrapper(_m_pWrapper->m_a);
        if (pNewWrapper == NULL)
            _fail();
        --_m_pWrapper->m_refCnt;
        _m_pWrapper = pNewWrapper;
    }
    void _fail()
    {
#line 912 "GameMap.h"
        throw TAllocationFailure(__FILE__, __LINE__);
    }

    _TWrapper* _m_pWrapper;
};
// A lazily created, shared and copy-on-write unsigned int vector.
class _TPAObjectID {
public:
    _TPAObjectID() : _m_pWrapper(NULL) {}
    _TPAObjectID(const _TPAObjectID& other) : _m_pWrapper(other._m_pWrapper)
    {
        if (_m_pWrapper != NULL)
            ++_m_pWrapper->m_refCnt;
    }
    ~_TPAObjectID()
    {
        if (_m_pWrapper != NULL && --_m_pWrapper->m_refCnt == 0)
            delete _m_pWrapper;
    }

    void construct()
    {
        if ((_m_pWrapper = new _TWrapper) == NULL)
            _fail();
    }
    void clear()
    {
        if (--_m_pWrapper->m_refCnt == 0)
            delete _m_pWrapper;
        _m_pWrapper = NULL;
    }

    vector<unsigned int>* get()
    {
        if (_m_pWrapper == NULL)
            construct();
        else if (_m_pWrapper->m_refCnt > 1)
            _split();
        return &_m_pWrapper->m_a;
    }
    vector<unsigned int>& operator*() { return *get(); }
    const vector<unsigned int>* get() const
    {
        return _m_pWrapper != NULL ? &_m_pWrapper->m_a : NULL;
    }
    const vector<unsigned int>& operator*() const { return *get(); }

private:
    struct _TWrapper {
        _TWrapper() : m_refCnt(1) {}
        _TWrapper(const vector<unsigned int>& a) : m_refCnt(1), m_a(a) {}

        unsigned int m_refCnt;
        vector<unsigned int> m_a;
    };

    void _split()
    {
        _TWrapper* pNewWrapper = new _TWrapper(_m_pWrapper->m_a);
        if (pNewWrapper == NULL)
            _fail();
        --_m_pWrapper->m_refCnt;
        _m_pWrapper = pNewWrapper;
    }
    void _fail()
    {
#line 989 "GameMap.h"
        throw TAllocationFailure(__FILE__, __LINE__);
    }

    _TWrapper* _m_pWrapper;
};

private:
    unsigned int _m_terrainType : 4;
    unsigned int _m_riverType : 3;
    unsigned int _m_roadType : 3;
    unsigned int _m_tileNum : 7;
    unsigned int _m_riverTileNum : 4;
    unsigned int _m_roadTileNum : 5;
    unsigned int _m_bHFlipped : 1;
    unsigned int _m_bVFlipped : 1;
    unsigned int _m_bRiverHFlipped : 1;
    unsigned int _m_bRiverVFlipped : 1;
    unsigned int _m_bRoadHFlipped : 1;
    unsigned int _m_bRoadVFlipped : 1;

public:
    _TPObjectCellInfoList _m_paObjInfo;
    _TPAObjectID _m_paShadowID;
};

void readCellData(TRawIStream& stream, TGameMap::TLayer* pLayer);
void writeCellData(TRawOStream& stream, const TGameMap::TLayer& layer);

#endif  /* HOMM3_EDITOR_GAMEMAP_H */
