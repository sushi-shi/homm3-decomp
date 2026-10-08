// RoadPlacement.h - road placement (Loki RoadPlacement.cpp): the client
// interfaces the road operations report to (TRoadPlacementOpClient derives
// from TRoadOpClient, as their type_info functions show; slot order from
// TMapDoc's thunks) and the operations. TRoadOp is the road layer of one
// map layer seen as a TMapLineFilter (its constructor and the assert texts
// name _m_pMap and _m_bSecondLayer); the placement operation draws through a
// heap TLinePlacementOp (_m_pLinePlacementOp), the erase operation through
// an embedded TLineEraseOp; the bases' order and sizes (0x24, 0x2c) follow
// the constructors and TMapDoc's allocations; the line filter base is
// private (the RTTI base list), reached through _getMapLineFilter. The
// in-class members are the ones emitted after RoadPlacement.o's static
// initialization.
#ifndef HOMM3_EDITOR_ROADPLACEMENT_H
#define HOMM3_EDITOR_ROADPLACEMENT_H

#include "editor/GameMap.h"
#include "editor/LinePlacement.h"

class TRoadOpClient {
public:
    virtual void onRoadsUpdated(bool bUnderground, unsigned int left, unsigned int top,
                                unsigned int width, unsigned int height) = 0;
};

class TRoadPlacementOpClient : public TRoadOpClient {
public:
    virtual void onPlacingRoad(bool bUnderground, unsigned int x, unsigned int y) = 0;
};

// The roads of one map layer as a line filter: the cells' road type,
// tile number and flips (the river and road vtables share TMapLineFilter's
// slots and add a pure virtual destructor last: the slot is __pure_virtual).
class TRoadOp : private TMapLineFilter {
public:
    TRoadOp(TGameMap* pMap, bool bSecondLayer);
    virtual ~TRoadOp() = 0;

    virtual const TLineTilesetTraits& getTilesetTraits(unsigned int type) const;

protected:
    virtual void _setCellInfo(const TTilePoint& loc, const TCellInfo& info);
    virtual void _setCellType(const TTilePoint& loc, unsigned int type);
    virtual void _setCellTileNum(const TTilePoint& loc, unsigned int tileNum);
    virtual void _setCellBHFlipped(const TTilePoint& loc, bool bHFlipped);
    virtual void _setCellBVFlipped(const TTilePoint& loc, bool bVFlipped);
    virtual bool _isCellBlocked(const TTilePoint& loc) const;
    virtual void _getCellInfo(const TTilePoint& loc, TCellInfo* pInfo) const;
    virtual unsigned int _getCellType(const TTilePoint& loc) const;
    virtual unsigned int _getCellTileNum(const TTilePoint& loc) const;
    virtual bool _getCellBHFlipped(const TTilePoint& loc) const;
    virtual bool _getCellBVFlipped(const TTilePoint& loc) const;

    TMapLineFilter* _getMapLineFilter() { return this; }
    TGameMap::TLayer* _getPMapLayer() { return _m_pMap->getPLayer(_m_bSecondLayer); }
    const TGameMap::TLayer& _getMapLayer() const { return _m_pMap->getLayer(_m_bSecondLayer); }

    TGameMap* _m_pMap;
    bool _m_bSecondLayer;
};

class TRoadPlacementOp : public TRoadOp, public TLinePlacementOpClient {
public:
    TRoadPlacementOp(TRoadPlacementOpClient* pClient, TGameMap* pMap, bool bSecondLayer,
                     TRoadType roadType, unsigned int x, unsigned int y);
    virtual ~TRoadPlacementOp();

    void operator()(unsigned int x, unsigned int y) { (*_m_pLinePlacementOp)(x, y); }

    virtual void onLinesUpdated(unsigned int left, unsigned int top, unsigned int width,
                                unsigned int height);
    virtual void onPlacingLine(unsigned int x, unsigned int y);

private:
    TRoadPlacementOpClient* _m_pClient;
    TLinePlacementOp* _m_pLinePlacementOp;
};

class TRoadEraseOp : public TRoadOp, public TLineOpClient {
public:
    static void onTerrainTypeChanged(TRoadOpClient* pClient, TGameMap* pMap, bool bSecondLayer,
                                     unsigned int x, unsigned int y);

    TRoadEraseOp(TRoadOpClient* pClient, TGameMap* pMap, bool bSecondLayer);
    virtual ~TRoadEraseOp() {}

    void operator()(unsigned int left, unsigned int top, unsigned int width, unsigned int height)
    {
        _m_lineEraseOp(left, top, width, height);
    }

    virtual void onLinesUpdated(unsigned int left, unsigned int top, unsigned int width,
                                unsigned int height);

private:
    TRoadOpClient* _m_pClient;
    TLineEraseOp _m_lineEraseOp;
};

#endif  /* HOMM3_EDITOR_ROADPLACEMENT_H */
