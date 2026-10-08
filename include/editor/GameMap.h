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

#include <bitset>
#include <iterator>
#include <stddef.h>
#include <set>
#include <string>
#include <vector>

#include "herodefs.h"
#include "terrain_type.h"
#include "exceptions.h"
#include "editor/Array.h"
#include "editor/Player.h"
#include "editor/Point.h"
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
class TVictoryCondition;
class TLossCondition;
class TTimedEvent;
class TRumor;
class TPlayerInfo;
class TTeamInfo;
class TMapObjectRef;

enum TRiverType {
    kNumRiverTypes = 5
};

enum TRoadType {
    kNumRoadTypes = 4
};

typedef unsigned int TMapLayerObjectID;

class TGameMap {
    class _TImpl;

public:
    class TClient;
    class TObjectFactory;

    enum TSize {
        s_kNumSizes = 4
    };

    enum TDifficulty {
        s_kNumDifficulties = 5
    };

    class TLayer {
    public:
        static const TMapLayerObjectID s_kInvalidObjID;

        // One object's footprint record in a cell: the object and the
        // height of its placed cell there.
        struct _TObjectCellInfo {
            _TObjectCellInfo(unsigned int objID, unsigned int height) : m_objID(objID), m_height(height) {}

            unsigned int m_objID;
            unsigned int m_height;
        };

        class TCell {
        public:
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

            vector<_TObjectCellInfo>* get()
            {
                if (_m_pWrapper == NULL)
                    construct();
                else if (_m_pWrapper->m_refCnt > 1)
                    _split();
                return &_m_pWrapper->m_a;
            }
            const vector<_TObjectCellInfo>* get() const
            {
                return _m_pWrapper != NULL ? &_m_pWrapper->m_a : NULL;
            }
            vector<_TObjectCellInfo>* operator*() { return get(); }
            const vector<_TObjectCellInfo>* operator*() const { return get(); }

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

            vector<unsigned int>* get()
            {
                if (_m_pWrapper == NULL)
                    construct();
                else if (_m_pWrapper->m_refCnt > 1)
                    _split();
                return &_m_pWrapper->m_a;
            }
            const vector<unsigned int>* get() const
            {
                return _m_pWrapper != NULL ? &_m_pWrapper->m_a : NULL;
            }
            vector<unsigned int>* operator*() { return get(); }
            const vector<unsigned int>* operator*() const { return get(); }

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

            TCell()
                : _m_terrainType(eTerrainWater), _m_riverType(0), _m_roadType(0), _m_tileNum(0),
                  _m_riverTileNum(0), _m_roadTileNum(0), _m_bHFlipped(false), _m_bVFlipped(false),
                  _m_bRiverHFlipped(false), _m_bRiverVFlipped(false), _m_bRoadHFlipped(false),
                  _m_bRoadVFlipped(false) {}

            TTerrainType getTerrainType() const { return TTerrainType(_m_terrainType); }
            void setTerrainType(TTerrainType newTerrainType);
            unsigned int getTileNum() const { return _m_tileNum; }
            void setTileNum(unsigned int newTileNum);
            bool getBHFlipped() const { return _m_bHFlipped; }
            void setBHFlipped(bool bFlipped) { _m_bHFlipped = bFlipped; }
            bool getBVFlipped() const { return _m_bVFlipped; }
            void setBVFlipped(bool bFlipped) { _m_bVFlipped = bFlipped; }

            TRiverType getRiverType() const { return TRiverType(_m_riverType); }
            void setRiverType(TRiverType newRiverType);
            unsigned int getRiverTileNum() const { return _m_riverTileNum; }
            void setRiverTileNum(unsigned int newTileNum);
            bool getBRiverHFlipped() const { return _m_bRiverHFlipped; }
            void setBRiverHFlipped(bool bFlipped) { _m_bRiverHFlipped = bFlipped; }
            bool getBRiverVFlipped() const { return _m_bRiverVFlipped; }
            void setBRiverVFlipped(bool bFlipped) { _m_bRiverVFlipped = bFlipped; }

            TRoadType getRoadType() const { return TRoadType(_m_roadType); }
            void setRoadType(TRoadType newRoadType);
            unsigned int getRoadTileNum() const { return _m_roadTileNum; }
            void setRoadTileNum(unsigned int newTileNum);
            bool getBRoadHFlipped() const { return _m_bRoadHFlipped; }
            void setBRoadHFlipped(bool bFlipped) { _m_bRoadHFlipped = bFlipped; }
            bool getBRoadVFlipped() const { return _m_bRoadVFlipped; }
            void setBRoadVFlipped(bool bFlipped) { _m_bRoadVFlipped = bFlipped; }

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

        // Walks a layer's placed objects in link order.
        class TObjectIDIter : public forward_iterator<TMapLayerObjectID, ptrdiff_t> {
        public:
            TObjectIDIter(const TLayer* pLayer, unsigned int objID) : _m_pLayer(pLayer), _m_objID(objID) {}

            TMapLayerObjectID operator*() const { return _m_objID; }
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
            bool operator==(const TObjectIDIter& other) const
            {
                return _m_pLayer == other._m_pLayer && _m_objID == other._m_objID;
            }
            bool operator!=(const TObjectIDIter& other) const { return !(*this == other); }

        private:
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
        const TCell& getCell(unsigned int x, unsigned int y) const { return *getPCell(x, y); }
        const TCell& getCell(const TTilePoint& loc) const { return *getPCell(loc.x(), loc.y()); }
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
        unsigned int getNumShadowIDsAtCell(unsigned int x, unsigned int y) const;
        TMapLayerObjectID getShadowIDAtCell(unsigned int x, unsigned int y, unsigned int which) const;

    private:
        class _TImpl;
        friend class TGameMap;
        friend class TGameMap::_TImpl;

        TMapLayerObjectID _placeObject(const TGameObject& obj, const TTilePoint& loc);
        void _removeObject(unsigned int objID);
        void _floatObject(unsigned int objID);
        void _unfloatObject(const TTilePoint& loc);
        TMapLayerObjectID _findObject(const TTilePoint& loc, bool (*pfnPredicate)(const TGameObject&)) const;

        TRefCountingPtr<_TImpl> _m_pImpl;
    };

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
    friend class TLayer::_TImpl;

    TRefCountingPtr<_TImpl> _m_pImpl;
};

void readCellData(TRawIStream& stream, TGameMap::TLayer* pLayer);
void writeCellData(TRawOStream& stream, const TGameMap::TLayer& layer);

#endif  /* HOMM3_EDITOR_GAMEMAP_H */
