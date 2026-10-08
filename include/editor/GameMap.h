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
#include <string>
#include <vector>

#include "terrain_type.h"
#include "va.h"
#include "editor/Point.h"
#include "editor/RefCountingPtr.h"

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

    unsigned int getWidth() const;
    unsigned int getHeight() const;
    const TCell* getPCell(unsigned int x, unsigned int y) const;
    const TCell& getCell(unsigned int x, unsigned int y) const { return *getPCell(x, y); }
    VA(0x0041e82b, 0x20)  // an out-of-line copy after its first user
    const TCell& getCell(const TTilePoint& loc) const { return getCell(loc.x(), loc.y()); }

private:
    class _TImpl;
    friend class TGameMap;

    TRefCountingPtr<_TImpl> _m_pImpl;
};

class TGameMap::TLayer::TCell {
public:
    TTerrainType getTerrainType() const { return TTerrainType(_m_terrainType); }

private:
    // A lazily created, shared and copy-on-write vector.
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

    private:
        struct _TWrapper {
            unsigned int m_refCnt;
            std::vector<T> m_a;
        };

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
