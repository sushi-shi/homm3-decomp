// LinePlacement.cpp - Loki h3maped object 17: line placement, shared by the
// river and road tools. A line is drawn cell by cell from the last point to
// the new one; each placed or erased cell recomputes its own and its
// neighbours' line shapes from the line cells around them, picks a random
// tile of that shape and reports the changed rectangle. Assert and throw
// lines come from the retail immediates.

#include <assert.h>
#include <stdlib.h>
#include <algorithm>

#include "exceptions.h"
#include "editor/LinePlacement.h"
#include "editor/TilePoint.h"

TLineTilesetTraits::TLineTilesetTraits(unsigned int numTiles, const TLineShape* akTileLineShape)
    : _m_numTiles(numTiles), _m_aTileLineShape(NULL)
{
#line 28
    assert(_m_numTiles > 0);
    assert(akTileLineShape != NULL);
    if ((_m_aTileLineShape = new TLineShape[_m_numTiles]) == NULL)
#line 33
        throw TAllocationFailure(__FILE__, __LINE__);
    copy(akTileLineShape, akTileLineShape + _m_numTiles, _m_aTileLineShape);
    for (unsigned int lineShape = 0; lineShape < kNumLineShapes; ++lineShape) {
        _m_aLineShapeProps[lineShape].m_firstTile = 0;
        _m_aLineShapeProps[lineShape].m_numTiles = 0;
    }
    unsigned int tileNum = 0;
    lineShape = _m_aTileLineShape[0];
#line 48
    assert(lineShape >= 0 && lineShape < kNumLineShapes);
    for (;;) {
        ++_m_aLineShapeProps[lineShape].m_numTiles;
        if (++tileNum >= _m_numTiles)
            break;
        if (_m_aTileLineShape[tileNum] != lineShape) {
            lineShape = _m_aTileLineShape[tileNum];
#line 59
            assert(lineShape >= 0 && lineShape < kNumLineShapes);
            _m_aLineShapeProps[lineShape].m_firstTile = tileNum;
        }
    }
}

TLineTilesetTraits::~TLineTilesetTraits()
{
    delete[] _m_aTileLineShape;
}

unsigned int TLineTilesetTraits::pickRandom(TLineShape lineShape) const
{
#line 74
    assert(lineShape >= 0 && lineShape < kNumLineShapes);
    assert(_m_aLineShapeProps[ lineShape ].m_numTiles > 0);
    return _m_aLineShapeProps[lineShape].m_firstTile + rand() % _m_aLineShapeProps[lineShape].m_numTiles;
}

TMapLineFilter::TCellRef TMapLineFilter::getCell(const TTilePoint& tilePoint)
{
#line 85
    assert(tilePoint.x() >= 0 && tilePoint.x() < _m_mapWidth);
    assert(tilePoint.y() >= 0 && tilePoint.y() < _m_mapHeight);
    return TCellRef(this, tilePoint);
}

TMapLineFilter::TCellConstRef TMapLineFilter::getCell(const TTilePoint& tilePoint) const
{
#line 93
    assert(tilePoint.x() >= 0 && tilePoint.x() < _m_mapWidth);
    assert(tilePoint.y() >= 0 && tilePoint.y() < _m_mapHeight);
    return TCellConstRef(*this, tilePoint);
}

void TMapLineFilter::_setCellInfo(const TTilePoint& loc, const TCellInfo& info)
{
    _setCellType(loc, info.m_type);
    _setCellTileNum(loc, info.m_tileNum);
    _setCellBHFlipped(loc, info.m_bHFlipped);
    _setCellBVFlipped(loc, info.m_bVFlipped);
}

void TMapLineFilter::_getCellInfo(const TTilePoint& loc, TCellInfo* pInfo) const
{
    pInfo->m_type = _getCellType(loc);
    pInfo->m_tileNum = _getCellTileNum(loc);
    pInfo->m_bHFlipped = _getCellBHFlipped(loc);
    pInfo->m_bVFlipped = _getCellBVFlipped(loc);
}

namespace {

// Each neighbour direction (clockwise from north) as it lands after
// flipping a tile horizontally and/or vertically.
const unsigned int akFlippedDir[2][2][8] = {
    { { 0, 1, 2, 3, 4, 5, 6, 7 }, { 4, 3, 2, 1, 0, 7, 6, 5 } },
    { { 0, 7, 6, 5, 4, 3, 2, 1 }, { 4, 5, 6, 7, 0, 1, 2, 3 } }
};

// The four flips, unflipped first.
const bool akFlips[4][2] = { { false, false }, { false, true }, { true, false }, { true, true } };

inline void recompBoundingRect(unsigned int x, unsigned int y, TTilePoint* pTopLeft,
                               TTilePoint* pBottomRight)
{
    if (x < pTopLeft->x())
        pTopLeft->x(x);
    if (y < pTopLeft->y())
        pTopLeft->y(y);
    if (x >= pBottomRight->x())
        pBottomRight->x(x + 1);
    if (y >= pBottomRight->y())
        pBottomRight->y(y + 1);
}

void computeAdjacentLines(TMapLineFilter* pMapFilter, unsigned int x, unsigned int y,
                          unsigned int lineType, bool* abAdjLine)
{
    bool abAdjDir[8];
    computeAdjacentDirs(pMapFilter->getWidth(), pMapFilter->getHeight(), x, y, abAdjDir);
    for (unsigned int dir = 0; dir < 8; ++dir) {
        if (abAdjDir[dir]) {
            TTilePoint adjLoc = TPoint<int>((int)x, (int)y) + akAdjOffset[dir];
            abAdjLine[dir] = pMapFilter->getCell(adjLoc).getType() == lineType;
        } else
            abAdjLine[dir] = false;
    }
}

void computeLineShape(const bool* abAdjLine, const TLineTilesetTraits& traits,
                      TLineShape* pLineShape, bool* pbHFlipped, bool* pbVFlipped)
{
    if (abAdjLine[0] && abAdjLine[2] && abAdjLine[4] && abAdjLine[6]) {
        *pLineShape = eLS_nesw;
        *pbHFlipped = false;
        *pbVFlipped = false;
    } else if (abAdjLine[0] && abAdjLine[4]) {
        if (abAdjLine[2]) {
            *pLineShape = eLS_nes;
            *pbHFlipped = false;
        } else if (abAdjLine[6]) {
            *pLineShape = eLS_nes;
            *pbHFlipped = true;
        } else {
            *pLineShape = eLS_ns;
            *pbHFlipped = false;
        }
        *pbVFlipped = false;
    } else if (abAdjLine[2] && abAdjLine[6]) {
        if (abAdjLine[4]) {
            *pLineShape = eLS_esw;
            *pbVFlipped = false;
        } else if (abAdjLine[0]) {
            *pLineShape = eLS_esw;
            *pbVFlipped = true;
        } else {
            *pLineShape = eLS_ew;
            *pbVFlipped = false;
        }
        *pbHFlipped = false;
    } else {
        // Dead local: retail builds this 8-byte table in %eax:%edx at -O0 and
        // never reads it (the loop reads the namespace table). Only a
        // `register` local whose scope closes before the call does that; its
        // name is unproven.
        {
            register const bool akFlipsCopy[4][2] = { { false, false }, { false, true }, { true, false }, { true, true } };
        }
        bool bHasDiagTiles = traits.getNumTilesOfLineShape(eLS_esDiag) > 0;
        for (unsigned int i = 0; i < 4; ++i) {
            const unsigned int* akDir = akFlippedDir[akFlips[i][0]][akFlips[i][1]];
            if (abAdjLine[akDir[2]] && abAdjLine[akDir[4]]) {
                if (bHasDiagTiles && (abAdjLine[akDir[1]] || abAdjLine[akDir[5]]))
                    *pLineShape = eLS_esDiag;
                else
                    *pLineShape = eLS_es;
                *pbHFlipped = akFlips[i][0];
                *pbVFlipped = akFlips[i][1];
                return;
            }
        }
        bool bHasEndTiles = traits.getNumTilesOfLineShape(eLS_s) > 0;
        if (bHasEndTiles) {
#line 260
            assert(traits.getNumTilesOfLineShape( eLS_e ) > 0);
            if (abAdjLine[6] || abAdjLine[2]) {
                *pLineShape = eLS_e;
                *pbHFlipped = abAdjLine[6];
                *pbVFlipped = false;
                return;
            }
            if (abAdjLine[4]) {
                *pLineShape = eLS_s;
                *pbHFlipped = false;
                *pbVFlipped = false;
                return;
            }
            *pLineShape = eLS_s;
            *pbHFlipped = false;
            *pbVFlipped = true;
        } else {
            *pLineShape = abAdjLine[6] || abAdjLine[2] ? eLS_ew : eLS_ns;
            *pbHFlipped = false;
            *pbVFlipped = false;
        }
    }
}

bool recalcLineShape(TMapLineFilter* pMapFilter, unsigned int x, unsigned int y)
{
    TMapLineFilter::TCellRef cell = pMapFilter->getCell(x, y);
    unsigned int lineType = cell.getType();
    bool abAdjLine[8];
    computeAdjacentLines(pMapFilter, x, y, lineType, abAdjLine);
    const TLineTilesetTraits& traits = pMapFilter->getTilesetTraits(lineType);
    TLineShape lineShape;
    bool bHFlipped;
    bool bVFlipped;
    computeLineShape(abAdjLine, traits, &lineShape, &bHFlipped, &bVFlipped);
    TMapLineFilter::TCellInfo cellInfo;
    cell.getInfo(&cellInfo);
    if (traits.getLineShape(cellInfo.m_tileNum) != lineShape || cellInfo.m_bHFlipped != bHFlipped
        || cellInfo.m_bVFlipped != bVFlipped) {
        cellInfo.m_tileNum = traits.pickRandom(lineShape);
        cellInfo.m_bHFlipped = bHFlipped;
        cellInfo.m_bVFlipped = bVFlipped;
        cell.setInfo(cellInfo);
        return true;
    }
    return false;
}

void eraseLines(TMapLineFilter* pMapFilter, unsigned int left, unsigned int top, unsigned int width,
                unsigned int height, TTilePoint* pTopLeft, TTilePoint* pBottomRight)
{
    unsigned int right = left + width;
    unsigned int bottom = top + height;
    unsigned int y;
    unsigned int x;
    for (y = top; y < bottom; ++y) {
        for (x = left; x < right; ++x) {
            TMapLineFilter::TCellRef cell = pMapFilter->getCell(x, y);
            if (cell.getType() != kLineTypeNone) {
                cell.setInfo(TMapLineFilter::TCellInfo(kLineTypeNone, 0, false, false));
                recompBoundingRect(x, y, pTopLeft, pBottomRight);
            }
        }
    }
    if (left > 0) {
        x = left - 1;
        unsigned int startY = top > 0 ? top - 1 : 0;
        unsigned int endY = bottom < pMapFilter->getHeight() ? bottom + 1 : bottom;
        for (y = startY; y < endY; ++y) {
            if (pMapFilter->getCell(x, y).getType() != kLineTypeNone && recalcLineShape(pMapFilter, x, y))
                recompBoundingRect(x, y, pTopLeft, pBottomRight);
        }
    }
    if (right < pMapFilter->getWidth()) {
        x = right;
        unsigned int startY = top > 0 ? top - 1 : 0;
        unsigned int endY = bottom < pMapFilter->getHeight() - 1 ? bottom + 1 : bottom;
        for (y = startY; y < endY; ++y) {
            if (pMapFilter->getCell(x, y).getType() != kLineTypeNone && recalcLineShape(pMapFilter, x, y))
                recompBoundingRect(x, y, pTopLeft, pBottomRight);
        }
    }
    if (top > 0) {
        y = top - 1;
        for (x = left; x < right; ++x) {
            if (pMapFilter->getCell(x, y).getType() != kLineTypeNone && recalcLineShape(pMapFilter, x, y))
                recompBoundingRect(x, y, pTopLeft, pBottomRight);
        }
    }
    if (bottom < pMapFilter->getHeight()) {
        y = bottom;
        for (x = left; x < right; ++x) {
            if (pMapFilter->getCell(x, y).getType() != kLineTypeNone && recalcLineShape(pMapFilter, x, y))
                recompBoundingRect(x, y, pTopLeft, pBottomRight);
        }
    }
}

}  // namespace

TLinePlacementOp::TLinePlacementOp(TLinePlacementOpClient* pClient, TMapLineFilter* pMapFilter,
                                   unsigned int lineType, unsigned int x, unsigned int y)
    : _m_pClient(pClient), _m_pMapFilter(pMapFilter), _m_lineType(lineType), _m_lastX(x), _m_lastY(y)
{
#line 430
    assert(_m_pClient != NULL);
    assert(_m_pMapFilter != NULL);
    assert(_m_lineType != kLineTypeNone);
    assert(_m_lastX >= 0 && _m_lastX < _m_pMapFilter->getWidth());
    assert(_m_lastY >= 0 && _m_lastY < _m_pMapFilter->getHeight());
    TTilePoint topLeft(_m_pMapFilter->getWidth(), _m_pMapFilter->getHeight());
    TTilePoint bottomRight(0, 0);
    _placeLine(_m_lastX, _m_lastY, &topLeft, &bottomRight);
    if (bottomRight.x() > topLeft.x()) {
#line 445
        assert(bottomRight.y() > topLeft.y());
        const TTilePoint size = bottomRight - topLeft;
        _m_pClient->onLinesUpdated(topLeft.x(), topLeft.y(), size.x(), size.y());
    }
}

void TLinePlacementOp::operator()(unsigned int x, unsigned int y)
{
#line 454
    assert(x >= 0 && x < _m_pMapFilter->getWidth());
    assert(y >= 0 && y < _m_pMapFilter->getHeight());
    TTilePoint topLeft(_m_pMapFilter->getWidth(), _m_pMapFilter->getHeight());
    TTilePoint bottomRight(0, 0);
    // Bresenham's walk from the new point back towards the last one.
    struct TAxis {
        unsigned int m_pos;
        unsigned int m_delta;
        int m_step;
    };
    TAxis xAxis;
    xAxis.m_pos = x;
    if (x <= _m_lastX) {
        xAxis.m_delta = _m_lastX - x;
        xAxis.m_step = 1;
    } else {
        xAxis.m_delta = x - _m_lastX;
        xAxis.m_step = -1;
    }
    TAxis yAxis;
    yAxis.m_pos = y;
    if (y <= _m_lastY) {
        yAxis.m_delta = _m_lastY - y;
        yAxis.m_step = 1;
    } else {
        yAxis.m_delta = y - _m_lastY;
        yAxis.m_step = -1;
    }
    TAxis* pMajor;
    TAxis* pMinor;
    if (xAxis.m_delta >= yAxis.m_delta) {
        pMajor = &xAxis;
        pMinor = &yAxis;
    } else {
        pMajor = &yAxis;
        pMinor = &xAxis;
    }
    unsigned int error = 0;
    for (unsigned int n = pMajor->m_delta; n != 0; --n) {
        _placeLine(xAxis.m_pos, yAxis.m_pos, &topLeft, &bottomRight);
        error += pMinor->m_delta;
        if (error >= pMajor->m_delta) {
            error -= pMajor->m_delta;
            pMinor->m_pos += pMinor->m_step;
            _placeLine(xAxis.m_pos, yAxis.m_pos, &topLeft, &bottomRight);
        }
        pMajor->m_pos += pMajor->m_step;
    }
    if (error + pMinor->m_delta >= pMajor->m_delta)
        _placeLine(xAxis.m_pos, yAxis.m_pos, &topLeft, &bottomRight);
    _m_lastX = x;
    _m_lastY = y;
    if (bottomRight.x() > topLeft.x()) {
#line 545
        assert(bottomRight.y() > topLeft.y());
        const TTilePoint size = bottomRight - topLeft;
        _m_pClient->onLinesUpdated(topLeft.x(), topLeft.y(), size.x(), size.y());
    }
}

void TLinePlacementOp::_placeLine(unsigned int x, unsigned int y, TTilePoint* pTopLeft,
                                  TTilePoint* pBottomRight)
{
    TMapLineFilter::TCellRef cell = _m_pMapFilter->getCell(x, y);
    unsigned int lineType = cell.getType();
    if (lineType == _m_lineType || cell.isBlocked())
        return;
    _m_pClient->onPlacingLine(x, y);
    if (lineType != kLineTypeNone)
        eraseLines(_m_pMapFilter, x, y, 1, 1, pTopLeft, pBottomRight);
    cell.setType(_m_lineType);
    recalcLineShape(_m_pMapFilter, x, y);
    recompBoundingRect(x, y, pTopLeft, pBottomRight);
    bool abAdjLine[8];
    computeAdjacentLines(_m_pMapFilter, x, y, _m_lineType, abAdjLine);
    for (unsigned int dir = 0; dir < 8; ++dir) {
        if (abAdjLine[dir]) {
            TTilePoint adjLoc = TPoint<int>((int)x, (int)y) + akAdjOffset[dir];
            if (recalcLineShape(_m_pMapFilter, adjLoc.x(), adjLoc.y()))
                recompBoundingRect(adjLoc.x(), adjLoc.y(), pTopLeft, pBottomRight);
        }
    }
}

TLineEraseOp::TLineEraseOp(TLineOpClient* pClient, TMapLineFilter* pMapFilter)
    : _m_pClient(pClient), _m_pMapFilter(pMapFilter)
{
#line 594
    assert(_m_pClient != NULL);
    assert(_m_pMapFilter != NULL);
}

void TLineEraseOp::operator()(unsigned int left, unsigned int top, unsigned int width,
                              unsigned int height)
{
#line 601
    assert(left + width <= _m_pMapFilter->getWidth());
    assert(top + height <= _m_pMapFilter->getHeight());
    TTilePoint topLeft(_m_pMapFilter->getWidth(), _m_pMapFilter->getHeight());
    TTilePoint bottomRight(0, 0);
    eraseLines(_m_pMapFilter, left, top, width, height, &topLeft, &bottomRight);
    if (bottomRight.x() > topLeft.x()) {
#line 612
        assert(bottomRight.y() > topLeft.y());
        const TTilePoint size = bottomRight - topLeft;
        _m_pClient->onLinesUpdated(topLeft.x(), topLeft.y(), size.x(), size.y());
    }
}
