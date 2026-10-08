// GameMap.cpp - Loki h3maped object 12: the map model. TGameMap and
// TGameMap::TLayer forward to their copy-on-write implementations; this
// file defines both implementations, the cell, the rumors, players and
// teams, the object bookkeeping and the binary and text forms.
//
// Reconstruction in progress: the layer handle and the cell come first.
#include "editor/stdafx.h"

#include <assert.h>

#include "editor/GameMap.h"
#include "editor/GameObject.h"
#include "editor/RawStream.h"

class TGameMap::_TImpl {
public:
    static const unsigned int _s_akDimension[TGameMap::s_kNumSizes];
};

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
                if ((_m_pWrapper = new _TWrapper(pObj)) == NULL)
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

TGameMap::TLayer::_TImpl::_TImpl(const _TImpl& other)
    : _m_size(other._m_size), _m_pCellGrid(other._m_pCellGrid), _m_nextAvail(other._m_nextAvail),
      _m_paObjectLink(other._m_paObjectLink), _m_floatingObjID(other._m_floatingObjID)
{
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
