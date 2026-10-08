// RiverPlacement.h - river placement (Loki RiverPlacement.cpp): the client
// interfaces the river operations report to (TRiverPlacementOpClient derives
// from TRiverOpClient, as their type_info functions show; slot order from
// TMapDoc's thunks) and the operations. TRiverOp is the river layer of one
// map layer seen as a TMapLineFilter (its constructor and the assert texts
// name _m_pMap and _m_bSecondLayer); the placement operation draws through a
// heap TLinePlacementOp (_m_pLinePlacementOp), the erase operation through
// an embedded TLineEraseOp; the bases' order and sizes (0x24, 0x2c) follow
// the constructors and TMapDoc's allocations; the line filter base is
// private (the RTTI base list), reached through _getMapLineFilter. The
// in-class members are the ones emitted after RiverPlacement.o's static
// initialization.
#ifndef HOMM3_EDITOR_RIVERPLACEMENT_H
#define HOMM3_EDITOR_RIVERPLACEMENT_H

#include "editor/GameMap.h"
#include "editor/LinePlacement.h"

class TRiverOpClient {
public:
    virtual void onRiversUpdated(bool bUnderground, unsigned int left, unsigned int top,
                                unsigned int width, unsigned int height) = 0;
};

class TRiverPlacementOpClient : public TRiverOpClient {
public:
    virtual void onPlacingRiver(bool bUnderground, unsigned int x, unsigned int y) = 0;
};

// The rivers of one map layer as a line filter: the cells' river type,
// tile number and flips (the river and road vtables share TMapLineFilter's
// slots and add a pure virtual destructor last: the slot is __pure_virtual).
class TRiverOp : private TMapLineFilter {
public:
    TRiverOp(TGameMap* pMap, bool bSecondLayer);
    virtual ~TRiverOp() = 0;

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

class TRiverPlacementOp : public TRiverOp, public TLinePlacementOpClient {
public:
    TRiverPlacementOp(TRiverPlacementOpClient* pClient, TGameMap* pMap, bool bSecondLayer,
                     TRiverType riverType, unsigned int x, unsigned int y);
    virtual ~TRiverPlacementOp();

    void operator()(unsigned int x, unsigned int y) { (*_m_pLinePlacementOp)(x, y); }

    virtual void onLinesUpdated(unsigned int left, unsigned int top, unsigned int width,
                                unsigned int height);
    virtual void onPlacingLine(unsigned int x, unsigned int y);

private:
    TRiverPlacementOpClient* _m_pClient;
    TLinePlacementOp* _m_pLinePlacementOp;
};

class TRiverEraseOp : public TRiverOp, public TLineOpClient {
public:
    static void onTerrainTypeChanged(TRiverOpClient* pClient, TGameMap* pMap, bool bSecondLayer,
                                     unsigned int x, unsigned int y);

    TRiverEraseOp(TRiverOpClient* pClient, TGameMap* pMap, bool bSecondLayer);
    virtual ~TRiverEraseOp() {}

    void operator()(unsigned int left, unsigned int top, unsigned int width, unsigned int height)
    {
        _m_lineEraseOp(left, top, width, height);
    }

    virtual void onLinesUpdated(unsigned int left, unsigned int top, unsigned int width,
                                unsigned int height);

private:
    TRiverOpClient* _m_pClient;
    TLineEraseOp _m_lineEraseOp;
};

#endif  /* HOMM3_EDITOR_RIVERPLACEMENT_H */
