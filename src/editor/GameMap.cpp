// GameMap.cpp - Loki h3maped object 12: the map model. TGameMap and
// TGameMap::TLayer forward to their copy-on-write implementations; this
// file defines both implementations, the cell, the rumors, players and
// teams, the object bookkeeping and the binary and text forms.
//
// Reconstruction in progress: the layer handle and the cell come first.
#include "editor/stdafx.h"

#include <assert.h>

#include "editor/GameMap.h"
#include "editor/RawStream.h"

class TGameMap::_TImpl {
public:
    static const unsigned int _s_akDimension[TGameMap::s_kNumSizes];
};

class TGameMap::TLayer::_TImpl {
public:
    _TImpl(TGameMap::TSize size);
    _TImpl(const _TImpl& other);
    ~_TImpl();
    _TImpl& operator=(const _TImpl& other);

    unsigned int getWidth() const { return TGameMap::_TImpl::_s_akDimension[_m_size]; }
    unsigned int getHeight() const { return TGameMap::_TImpl::_s_akDimension[_m_size]; }
    TCell* getPCell(unsigned int x, unsigned int y);
    const TCell* getPCell(unsigned int x, unsigned int y) const;
    TGameObject* getPObject(unsigned int objID);
    const TGameObject* getPObject(unsigned int objID) const;
    TTilePoint getObjectLoc(unsigned int objID) const;
    TTileExtent getObjectExtent(unsigned int objID) const;
    TMapLayerObjectID getFloatingObjID() const { return _m_floatingObjID; }
    bool isObjectIDValid(unsigned int objID) const;
    TMapLayerObjectID getFirstObjectID() const;
    TMapLayerObjectID getLastObjectID() const;
    TMapLayerObjectID getNextObjectID(unsigned int objID) const;
    TMapLayerObjectID getPrevObjectID(unsigned int objID) const;
    unsigned int getNumObjectIDsAtCell(unsigned int x, unsigned int y) const;
    TMapLayerObjectID getObjectIDAtCell(unsigned int x, unsigned int y, unsigned int which) const;
    unsigned int getNumShadowIDsAtCell(unsigned int x, unsigned int y) const;
    TMapLayerObjectID getShadowIDAtCell(unsigned int x, unsigned int y, unsigned int which) const;

    TMapLayerObjectID _placeObject(const TGameObject& obj, const TTilePoint& loc);
    void _removeObject(unsigned int objID);
    void _floatObject(unsigned int objID);
    void _unfloatObject(const TTilePoint& loc);
    TMapLayerObjectID _findObject(const TTilePoint& loc, bool (*pfnPredicate)(const TGameObject&)) const;

private:
    class _TCellGrid;
    class _TObjectLink;

    TGameMap::TSize _m_size;
    TRefCountingPtr<_TCellGrid> _m_pCellGrid;
    TMapLayerObjectID _m_nextAvail;
    TRefCountingPtr<vector<_TObjectLink> > _m_paObjectLink;
    TMapLayerObjectID _m_floatingObjID;
};

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
