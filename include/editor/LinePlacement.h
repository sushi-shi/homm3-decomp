// LinePlacement.h - line placement (Loki LinePlacement.cpp), the shared
// engine of the river and road tools. TMapLineFilter is a map seen as a
// grid of line cells (type, tile number, flips, blocked); the river and
// road operations derive from it and supply the cell accessors (the
// virtuals in vtable order, as TRiverOp's and TRoadOp's vtables list
// them). TLinePlacementOp draws a line from the last point to each new one,
// TLineEraseOp clears a rectangle; both report through the line clients
// (one and two pure virtuals, as their vtables show). The data members
// known so far are the ones the constructors initialize; the cell and
// tileset types' members are not declared yet.
#ifndef HOMM3_EDITOR_LINEPLACEMENT_H
#define HOMM3_EDITOR_LINEPLACEMENT_H

#include "editor/Point.h"
#include "editor/Uncopyable.h"

enum TLineShape {
};

class TLineTilesetTraits {
public:
    TLineTilesetTraits(unsigned int numTiles, const TLineShape* pLineShapes);
    ~TLineTilesetTraits();

    TLineShape pickRandom(TLineShape lineShape) const;
};

class TMapLineFilter : private TUncopyable {
public:
    struct TCellInfo {
        TCellInfo();
        TCellInfo(unsigned int type, unsigned int tileNum, bool bHFlipped, bool bVFlipped);
    };

    TMapLineFilter(unsigned int width, unsigned int height);

    unsigned int getWidth() const;
    unsigned int getHeight() const;

    virtual const TLineTilesetTraits& getTilesetTraits(unsigned int type) const = 0;

protected:
    virtual void _setCellInfo(const TPoint<unsigned int>& loc, const TCellInfo& info);
    virtual void _setCellType(const TPoint<unsigned int>& loc, unsigned int type) = 0;
    virtual void _setCellTileNum(const TPoint<unsigned int>& loc, unsigned int tileNum) = 0;
    virtual void _setCellBHFlipped(const TPoint<unsigned int>& loc, bool bHFlipped) = 0;
    virtual void _setCellBVFlipped(const TPoint<unsigned int>& loc, bool bVFlipped) = 0;
    virtual bool _isCellBlocked(const TPoint<unsigned int>& loc) const = 0;
    virtual void _getCellInfo(const TPoint<unsigned int>& loc, TCellInfo* pInfo) const;
    virtual unsigned int _getCellType(const TPoint<unsigned int>& loc) const = 0;
    virtual unsigned int _getCellTileNum(const TPoint<unsigned int>& loc) const = 0;
    virtual bool _getCellBHFlipped(const TPoint<unsigned int>& loc) const = 0;
    virtual bool _getCellBVFlipped(const TPoint<unsigned int>& loc) const = 0;

private:
    unsigned int _m_width;
    unsigned int _m_height;
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
