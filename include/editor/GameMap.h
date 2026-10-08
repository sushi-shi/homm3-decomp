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

#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "terrain_type.h"
#include "va.h"
#include "editor/Point.h"
#include "editor/RefCountingPtr.h"

class TGameObject;

typedef unsigned int TMapLayerObjectID;

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

class TGameMap {
public:
    // The four map sizes; the dimension table (0x535214) reads 36, 72,
    // 108 and 144 tiles.
    enum TSize {
        s_kNumSizes = 4
    };

    class TLayer;

    // Public here: the layer's implementation reads the map's dimension
    // table (TGameMap::_TImpl::_s_akDimension), which g++ 2.95 let Loki's
    // private declaration grant and VC6 does not.
    class _TImpl;

private:
    friend class TLayer;
};

// One level of the map: its cells and the objects placed on them. The
// objects are numbered from 1 and kept in a list in placement order; a
// layer floats at most one object, taken off its cells while it moves.
class TGameMap::TLayer {
public:
    // One object's footprint record in a cell: the object and the
    // height of its placed cell there.
    struct _TObjectCellInfo {
        _TObjectCellInfo(unsigned int objID, unsigned int height) : m_objID(objID), m_height(height) {}

        unsigned int m_objID;
        unsigned int m_height;
    };

    class TCell;

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
    TMapLayerObjectID getFirstObjectID() const;
    TMapLayerObjectID getLastObjectID() const;
    TMapLayerObjectID getNextObjectID(unsigned int objID) const;
    TMapLayerObjectID getPrevObjectID(unsigned int objID) const;
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
