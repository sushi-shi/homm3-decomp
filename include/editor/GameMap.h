// GameMap.h - the editor's map model (GameMap.cpp; Loki h3maped object 12).
// TGameMap and its TLayer are copy-on-write handles: each is a
// TRefCountingPtr to a private _TImpl defined in GameMap.cpp. A layer is a
// grid of TCells; a cell packs its terrain, river and road types and tiles
// and their flip flags into one word, followed by its object and shadow
// lists (12 bytes: the cell lookup at 0x42a4d0 scales by 12).
//
// Ported so far: the declarations the matched part of GameMap.cpp needs,
// in Loki's spelling. The rest of Loki's header follows with the rest of
// the object.
#ifndef HOMM3_EDITOR_GAMEMAP_H
#define HOMM3_EDITOR_GAMEMAP_H

#include <bitset>
#include <exception>
#include <iosfwd>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "gameversion.h"
#include "secondaryskill.h"
#include "terrain_type.h"
#include "town_type.h"
#include "va.h"
#include "editor/Array.h"
#include "editor/Hero.h"
#include "editor/MapObjectRef.h"
#include "editor/Player.h"
#include "editor/Point.h"
#include "editor/RefCountingPtr.h"

class TGameObject;
class TObjectType;
class TTimedEvent;
class TVictoryCondition;
class TLossCondition;
class TLinkableObject;

typedef unsigned int TMapLayerObjectID;

// The artifacts a map may disable: the properties' mask is five dwords
// (h3maped 0x429d07 copies it whole).
enum { kNumArtifacts = 144 };

// A rumor: a name and a text of at most s_kMaxTextLen characters, either
// both empty or both with something besides white space. The rumor
// editor's OnInitDialog limits its text edit to 300 characters.
class TRumor {
public:
    enum { s_kMaxTextLen = 300 };

    TRumor() {}

    const std::string& getName() const { return _m_name; }
    const std::string& getText() const { return _m_text; }
    void setNameAndText(const std::string& newName, const std::string& newText);

private:
    std::string _m_name;
    std::string _m_text;
};

// Who may play a player, how the computer plays it, and its main town
// (0x18 bytes). Complete adds the town types the player may start with:
// the map's own default (0x423a2e) unless they are customized. The flags
// share one byte (the save at 0x423b20 masks bits 0, 1, 2, 3 and 4).
class TPlayerInfo {
public:
    enum TBehaviorType {
    };

    // The town types a player may start with, and whether one is drawn
    // at random among them.
    struct TTownTypes {
        std::bitset<kNumTownTypes> m_mask;
        bool m_bRandom;
    };

    bool getBPresent() const { return _m_bHumanPlayable || _m_bComputerPlayable; }
    bool getBHumanPlayable() const { return _m_bHumanPlayable; }
    bool getBComputerPlayable() const { return _m_bComputerPlayable; }
    bool getBGenerateHero() const { return _m_bGenerateHero; }
    bool getBHasMainTown() const { return _m_bHasMainTown; }
    bool getBCustomTownTypes() const { return _m_bCustomTownTypes; }
    TBehaviorType getBehaviorType() const { return _m_behaviorType; }
    const TMapObjectRef& getMainTownRef() const { return _m_mainTownRef; }
    const TTownTypes& getTownTypes() const { return _m_townTypes; }

private:
    bool _m_bHumanPlayable : 1;
    bool _m_bComputerPlayable : 1;
    bool _m_bGenerateHero : 1;
    bool _m_bHasMainTown : 1;
    bool _m_bCustomTownTypes : 1;
    TBehaviorType _m_behaviorType;
    TMapObjectRef _m_mainTownRef;
    TTownTypes _m_townTypes;
};

// The alliances: whether the map has teams, how many, and each player's
// (0x28 bytes; the map's assignment 0x42002a copies them in this order).
class TTeamInfo {
public:
    bool getBHasTeams() const { return _m_bHasTeams; }
    unsigned int getNumTeams() const { return _m_numTeams; }
    unsigned int getPlayerTeam(TPlayer player) const { return _m_aPlayerTeam[player]; }

private:
    bool _m_bHasTeams;
    unsigned int _m_numTeams;
    TArray<unsigned int, kNumPlayers> _m_aPlayerTeam;
};

// Why an object could not be created... The hierarchies are h3maped's
// RTTI (vtables 0x535234..0x5353b4, each with exception's two slots);
// the NotSupportedByReleaseVersion pair is Complete's, and Loki's caps on
// mines, generators and signs are gone.
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

class TCreateObjFailureNotSupportedByReleaseVersion : public TCreateObjectFailure {
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

class TPlaceObjFailureNotSupportedByReleaseVersion : public TPlaceObjectFailure {
};

class TPlaceObjFailureInvalidPlacement : public TPlaceObjectFailure {
};

class TPlaceObjFailurePlacementNotOnMap : public TPlaceObjFailureInvalidPlacement {
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

// The map: its specifications, its one or two layers and the bookkeeping
// of what is placed on them. A handle to a copy-on-write implementation;
// every member forwards to it (h3maped 0x429a12..0x42a43a, in this order).
class TGameMap {
public:
    // The map's client (TMapDoc): one pure virtual, first in its vtable
    // (h3maped 0x5398a8).
    class TClient {
    public:
        virtual void onMapObjectRemoved(bool bSecondLayer, TMapLayerObjectID objID) = 0;
    };

    // importText's failure (h3maped vtable 0x535378).
    class TImportTextFailure : public exception {
    };

    // The four map sizes; the dimension table (0x535214) reads 36, 72,
    // 108 and 144 tiles.
    enum TSize {
        s_kNumSizes = 4
    };

    enum TDifficulty {
        s_kNumDifficulties = 5
    };

    class TLayer;

    // Public here: the layer's implementation reads the map's dimension
    // table (TGameMap::_TImpl::_s_akDimension), which g++ 2.95 let Loki's
    // private declaration grant and VC6 does not.
    class _TImpl;

    TGameMap(TClient* pClient, EGameVersion version, TSize size, bool bTwoLayer);
    TGameMap(TClient* pClient, EGameVersion version, std::streambuf* pStreamBuf, int fileVersion);
    ~TGameMap();
    TGameMap& operator=(const TGameMap& other);

    void importText(std::istream* pIStream);
    TLayer* getPLayer(unsigned int num);
    void setName(const std::string& newName);
    void setDesc(const std::string& newDesc);
    void setDifficulty(TDifficulty newDifficulty);
    void setMaxHeroLevel(unsigned int newMaxHeroLevel);
    void setPlayers(const TArray<TPlayerInfo, kNumPlayers>& newPlayers);
    void setTeamInfo(const TTeamInfo& newTeamInfo);
    void setRumors(const std::vector<TRumor>& newRumors);
    void setTimedEvents(const std::vector<TTimedEvent>& newTimedEvents);
    void setVictoryCondition(std::auto_ptr<TVictoryCondition> pNewVictoryCondition);
    void setLossCondition(std::auto_ptr<TLossCondition> pNewLossCondition);
    void setDisabledHeroes(const std::bitset<kNumHeroes>& newMask);
    void setDisabledArtifacts(const std::bitset<kNumArtifacts>& newMask);
    void setDisabledSpells(const std::bitset<kNumSpells>& newMask);
    void setDisabledSkills(const std::bitset<kNumSecSkills>& newMask);
    void setHeroPrototype(THeroID heroID, const THeroPrototype& pNewPrototype);

    TMapLayerObjectID placeObject(bool bSecondLayer, std::auto_ptr<TGameObject> pObj, unsigned int x,
                                  unsigned int y);
    void removeObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent);
    TMapLayerObjectID insertObject(bool bSecondLayer, std::auto_ptr<TGameObject> pObj, const TTilePoint& loc);
    void eraseObject(bool bSecondLayer, unsigned int objID);
    void floatObject(bool bSecondLayer, unsigned int objID, TTileExtent* pUpdatedExtent);
    void unfloatObject(bool bSecondLayer, unsigned int x, unsigned int y, TTileExtent* pUpdatedExtent);
    void removeFloatingObject(bool bSecondLayer);
    void removeSecondLayer();
    void addSecondLayer();
    bool onTerrainTypeChanged(bool bSecondLayer, unsigned int x, unsigned int y, TTerrainType oldTerrainType,
                              TTileExtent* pUpdatedExtent);

    void save(std::streambuf* pStreamBuf) const;
    void exportText(std::ostream* pOStream) const;
    EGameVersion getVersion() const;
    unsigned int getWidth() const;
    unsigned int getHeight() const;
    bool isTwoLayer() const;
    const TGameObject* getPObject(bool bSecondLayer, unsigned int objID) const;
    TTilePoint getObjectLoc(bool bSecondLayer, unsigned int objID) const;
    TMapObjectRef getLinkableObjectRef(int linkID) const;
    const TLinkableObject* getPLinkableObject(int linkID) const;
    const TLayer* getPLayer(unsigned int num) const;
    const std::string& getName() const;
    const std::string& getDesc() const;
    TDifficulty getDifficulty() const;
    unsigned int getMaxHeroLevel() const;
    const TArray<TPlayerInfo, kNumPlayers>& getPlayers() const;
    const TTeamInfo& getTeamInfo() const;
    const std::vector<TRumor>& getRumors() const;
    const std::vector<TTimedEvent>& getTimedEvents() const;
    const TVictoryCondition* getPVictoryCondition() const;
    const TLossCondition* getPLossCondition() const;
    const std::bitset<kNumHeroes>& getDisabledHeroes() const;
    const std::bitset<kNumArtifacts>& getDisabledArtifacts() const;
    const std::bitset<kNumSpells>& getDisabledSpells() const;
    const std::bitset<kNumSecSkills>& getDisabledSkills() const;
    const THeroPrototype& getHeroPrototype(THeroID heroID) const;
    bool isPlayerPresent(TPlayer player) const;
    unsigned int getNumPlayableSlots() const;
    std::bitset<kNumHeroes> getHeroesOnMap() const;
    const std::set<TMapObjectRef>& getPlayerTownRefs(TPlayer player) const;
    unsigned int getNumTownsOnMap() const;
    bool isGrailOnMap() const;
    unsigned int getNumObelisksOnMap() const;
    bool isValidPlacement(const TGameObject& obj, bool bSecondLayer, unsigned int x, unsigned int y) const;
    TPlayerInfo::TTownTypes getDefaultTownTypes(TPlayer player) const;

private:
    friend class TLayer;

    TRefCountingPtr<_TImpl> _m_pImpl;
};

// One level of the map: its cells and the objects placed on them. The
// objects are numbered from 1 and kept in a list in placement order; a
// layer floats at most one object, taken off its cells while it moves.
class TGameMap::TLayer {
public:
    // One object's footprint record in a cell: the object and the
    // height of its placed cell there.
    struct _TObjectCellInfo {
        _TObjectCellInfo(int objID, unsigned int height) : m_objID(objID), m_height(height) {}

        int m_objID;
        unsigned int m_height;
    };

    class TCell;

    // Walks a layer's placed objects in link order (Loki's iterator; the
    // map's second-layer removal steps one out of line, h3maped 0x4212bd).
    class TObjectIDIter {
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

    // The no-object id (h3maped 0x535228): defined after the map's users
    // of it, which read it from memory.
    static const TMapLayerObjectID s_kInvalidObjID;

    TLayer(TSize size);
    ~TLayer();

    TCell* getPCell(unsigned int x, unsigned int y);
    TGameObject* getPObject(unsigned int objID);
    unsigned int getWidth() const;
    unsigned int getHeight() const;
    const TCell* getPCell(unsigned int x, unsigned int y) const;
    const TCell& getCell(unsigned int x, unsigned int y) const { return *getPCell(x, y); }
    VA(0x0041e82b, 0x20)  // an out-of-line copy after its first user
    const TCell& getCell(const TTilePoint& loc) const { return getCell(loc.x(), loc.y()); }
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
    unsigned int getNumShadowIDsAtCell(unsigned int x, unsigned int y) const;
    TMapLayerObjectID getShadowIDAtCell(unsigned int x, unsigned int y, unsigned int which) const;

private:
    class _TImpl;
    friend class TGameMap;
    friend class TGameMap::_TImpl;
    friend class TCell;

    // Windows hands the layer its own clone of the object (h3maped
    // 0x42b793 passes an auto_ptr by value); Loki's port clones a const
    // reference inside.
    TMapLayerObjectID _placeObject(std::auto_ptr<TGameObject> pObj, const TTilePoint& loc);
    // The map reader's own placement (h3maped 0x41fdbe, an out-of-line copy).
    TMapLayerObjectID _placeObject(std::auto_ptr<TGameObject> pObj, unsigned int x, unsigned int y)
    {
        return _placeObject(pObj, TTilePoint(x, y));
    }
    TMapLayerObjectID _findObject(const TTilePoint& loc, bool (*pfnPredicate)(const TGameObject&)) const;
    void _removeObject(unsigned int objID);
    void _floatObject(unsigned int objID);
    void _unfloatObject(const TTilePoint& loc);

    TRefCountingPtr<_TImpl> _m_pImpl;
};

class TGameMap::TLayer::TCell {
public:
    // A new cell is plain water (h3maped 0x4395c8 stores the word 8).
    TCell() : _m_terrainType(eTerrainWater), _m_riverType(0), _m_roadType(0), _m_tileNum(0),
              _m_riverTileNum(0), _m_roadTileNum(0), _m_bHFlipped(false), _m_bVFlipped(false),
              _m_bRiverHFlipped(false), _m_bRiverVFlipped(false), _m_bRoadHFlipped(false),
              _m_bRoadVFlipped(false) {}

    TTerrainType getTerrainType() const { return TTerrainType(_m_terrainType); }

private:
    friend class TGameMap::TLayer::_TImpl;
    friend class TGameMap::_TImpl;

    // A lazily created, shared and copy-on-write vector. One template for
    // Loki's two classes: h3maped folds their creation (0x42b042).
    template<class T>
    class _TPVector {
    public:
        _TPVector() : _m_pWrapper(NULL) {}
        _TPVector(const _TPVector& other) : _m_pWrapper(other._m_pWrapper)
        {
            if (_m_pWrapper != NULL)
                ++_m_pWrapper->m_refCnt;
        }
        ~_TPVector()
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

        std::vector<T>* get()
        {
            if (_m_pWrapper == NULL)
                construct();
            else if (_m_pWrapper->m_refCnt > 1)
                _split();
            return &_m_pWrapper->m_a;
        }
        std::vector<T>& operator*() { return *get(); }
        const std::vector<T>* get() const
        {
            return _m_pWrapper == NULL ? NULL : &_m_pWrapper->m_a;
        }
        const std::vector<T>& operator*() const { return *get(); }

    private:
        struct _TWrapper {
            _TWrapper() : m_refCnt(1) {}
            _TWrapper(const std::vector<T>& a) : m_refCnt(1), m_a(a) {}

            unsigned int m_refCnt;
            std::vector<T> m_a;
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
            throw TAllocationFailure();
        }

        _TWrapper* _m_pWrapper;
    };

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
    _TPVector<_TObjectCellInfo> _m_paObjInfo;
    _TPVector<unsigned int> _m_paShadowID;
};

#endif  /* HOMM3_EDITOR_GAMEMAP_H */
