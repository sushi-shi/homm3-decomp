// LinePlacement.h - line placement (Loki LinePlacement.cpp), the shared
// engine of the river and road tools. TMapLineFilter is a map seen as a
// grid of line cells (type, tile number, flips, blocked); the river and
// road operations derive from it and supply the cell accessors (the
// virtuals in vtable order, as TRiverOp's and TRoadOp's vtables list
// them). TLinePlacementOp draws a line from the last point to each new one,
// TLineEraseOp clears a rectangle; both report through the line clients
// (one and two pure virtuals, as their vtables show). A tileset lists each
// tile's line shape; the shapes' enumerators other than eLS_e (an assert)
// are not proven, nor are the members the asserts do not name.
#ifndef HOMM3_EDITOR_LINEPLACEMENT_H
#define HOMM3_EDITOR_LINEPLACEMENT_H

#include "editor/Point.h"
#include "editor/Uncopyable.h"

// No line in a cell.
const unsigned int kLineTypeNone = 0;

// A line tile's shape, by the neighbours it joins (before flipping).
enum TLineShape {
    eLS_s,
    eLS_e,
    eLS_ns,
    eLS_ew,
    eLS_es,
    eLS_esDiag,
    eLS_nes,
    eLS_esw,
    eLS_nesw,
    kNumLineShapes
};

class TLineTilesetTraits {
public:
    TLineTilesetTraits(unsigned int numTiles, const TLineShape* akTileLineShape);
    ~TLineTilesetTraits();

    TLineShape getLineShape(unsigned int tileNum) const { return _m_aTileLineShape[tileNum]; }
    unsigned int getNumTilesOfLineShape(TLineShape lineShape) const
    {
        return _m_aLineShapeProps[lineShape].m_numTiles;
    }
    unsigned int pickRandom(TLineShape lineShape) const;

private:
    struct _TLineShapeProps {
        unsigned int m_firstTile;
        unsigned int m_numTiles;
    };

    unsigned int _m_numTiles;
    TLineShape* _m_aTileLineShape;
    _TLineShapeProps _m_aLineShapeProps[kNumLineShapes];
};

class TMapLineFilter : private TUncopyable {
public:
    struct TCellInfo {
        TCellInfo() {}
        TCellInfo(unsigned int type, unsigned int tileNum, bool bHFlipped, bool bVFlipped)
            : m_type(type), m_tileNum(tileNum), m_bHFlipped(bHFlipped), m_bVFlipped(bVFlipped) {}

        unsigned int m_type;
        unsigned int m_tileNum;
        bool m_bHFlipped;
        bool m_bVFlipped;
    };

    // A cell of the filter, read through its virtuals.
    class TCellConstRef {
    public:
        TCellConstRef(const TMapLineFilter& mapFilter, const TTilePoint& loc)
            : _m_pMapFilter(&mapFilter), _m_loc(loc) {}

        bool isBlocked() const { return _m_pMapFilter->_isCellBlocked(_m_loc); }
        void getInfo(TCellInfo* pInfo) const { _m_pMapFilter->_getCellInfo(_m_loc, pInfo); }
        unsigned int getType() const { return _m_pMapFilter->_getCellType(_m_loc); }

    protected:
        const TMapLineFilter* _m_pMapFilter;
        TTilePoint _m_loc;
    };

    friend class TCellConstRef;

    class TCellRef : public TCellConstRef {
    public:
        TCellRef(TMapLineFilter* pMapFilter, const TTilePoint& loc) : TCellConstRef(*pMapFilter, loc) {}

        void setInfo(const TCellInfo& info) const
        {
            const_cast<TMapLineFilter*>(_m_pMapFilter)->_setCellInfo(_m_loc, info);
        }
        void setType(unsigned int type) const
        {
            const_cast<TMapLineFilter*>(_m_pMapFilter)->_setCellType(_m_loc, type);
        }
    };

    friend class TCellRef;

    TMapLineFilter(unsigned int width, unsigned int height) : _m_mapWidth(width), _m_mapHeight(height) {}

    unsigned int getWidth() const { return _m_mapWidth; }
    unsigned int getHeight() const { return _m_mapHeight; }

    TCellRef getCell(const TTilePoint& tilePoint);
    TCellConstRef getCell(const TTilePoint& tilePoint) const;
    TCellRef getCell(unsigned int x, unsigned int y) { return getCell(TTilePoint(x, y)); }
    TCellConstRef getCell(unsigned int x, unsigned int y) const { return getCell(TTilePoint(x, y)); }

    virtual const TLineTilesetTraits& getTilesetTraits(unsigned int type) const = 0;

protected:
    virtual void _setCellInfo(const TTilePoint& loc, const TCellInfo& info);
    virtual void _setCellType(const TTilePoint& loc, unsigned int type) = 0;
    virtual void _setCellTileNum(const TTilePoint& loc, unsigned int tileNum) = 0;
    virtual void _setCellBHFlipped(const TTilePoint& loc, bool bHFlipped) = 0;
    virtual void _setCellBVFlipped(const TTilePoint& loc, bool bVFlipped) = 0;
    virtual bool _isCellBlocked(const TTilePoint& loc) const = 0;
    virtual void _getCellInfo(const TTilePoint& loc, TCellInfo* pInfo) const;
    virtual unsigned int _getCellType(const TTilePoint& loc) const = 0;
    virtual unsigned int _getCellTileNum(const TTilePoint& loc) const = 0;
    virtual bool _getCellBHFlipped(const TTilePoint& loc) const = 0;
    virtual bool _getCellBVFlipped(const TTilePoint& loc) const = 0;

private:
    unsigned int _m_mapWidth;
    unsigned int _m_mapHeight;
};

class TLineOpClient {
public:
    virtual void onLinesUpdated(unsigned int left, unsigned int top, unsigned int width,
                                unsigned int height) = 0;
};

class TLinePlacementOpClient : public TLineOpClient {
public:
    virtual void onPlacingLine(unsigned int x, unsigned int y) = 0;
};

class TLinePlacementOp : private TUncopyable {
public:
    TLinePlacementOp(TLinePlacementOpClient* pClient, TMapLineFilter* pMapFilter, unsigned int lineType,
                     unsigned int x, unsigned int y);

    void operator()(unsigned int x, unsigned int y);

private:
    void _placeLine(unsigned int x, unsigned int y, TPoint<unsigned int>* pTopLeft,
                    TPoint<unsigned int>* pBottomRight);

    TLinePlacementOpClient* _m_pClient;
    TMapLineFilter* _m_pMapFilter;
    unsigned int _m_lineType;
    unsigned int _m_lastX;
    unsigned int _m_lastY;
};

class TLineEraseOp : private TUncopyable {
public:
    TLineEraseOp(TLineOpClient* pClient, TMapLineFilter* pMapFilter);

    void operator()(unsigned int left, unsigned int top, unsigned int width, unsigned int height);

private:
    TLineOpClient* _m_pClient;
    TMapLineFilter* _m_pMapFilter;
};

#endif  /* HOMM3_EDITOR_LINEPLACEMENT_H */
