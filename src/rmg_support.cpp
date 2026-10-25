// rmg_support.cpp - retained Complete random-map helper bodies: the line
// pattern table and the river/road line walker. Retail keeps them together at
// 0x4f9be0..0x4fa561, one object in the original link order (Mac also keeps
// selectFrame between the table destructor and the neighbour-mask helper).

// Retail keeps these ordinary helpers out of line in CreateRiver.  Their
// declarations remain visible through rmg.h, while placing the definitions in
// this companion translation unit reproduces the natural body-visibility
// boundary without source-false inline controls.
#include "va.h"

#include <algorithm>
#include <math.h>
#include <stdlib.h>

#include "exceptions.h"
#include "rmg.h"
#include "rmg_terrain.h"
#include "lineplacement.h"
#include "tiles.h"


VA(0x004f9be0, 0xb7)
MAC_ADDRESS(0x22210c, 0x1ec)
TRmgLinePatternTable::TRmgLinePatternTable(u32 frameCount, const s32* framePatterns)
    : m_frameCount(frameCount), m_framePatterns(0)
{
    s32* allocated = new s32[m_frameCount];
    m_framePatterns = allocated;
    if (!allocated)
        throw TAllocationFailure();
    std::copy(framePatterns, framePatterns + m_frameCount, m_framePatterns);
    for (u32 value = LINE_END_S; value < LINE_PATTERN_COUNT; ++value) {
        m_ranges[value].m_firstFrame = 0;
        m_ranges[value].m_frameCount = 0;
    }
    // Each pattern occupies one contiguous frame range.
    s32 runPattern = m_framePatterns[0];
    ++m_ranges[runPattern].m_frameCount;
    for (u32 index = 1; index < m_frameCount; ++index) {
        if (m_framePatterns[index] != runPattern) {
            runPattern = m_framePatterns[index];
            m_ranges[runPattern].m_firstFrame = index;
        }
        ++m_ranges[runPattern].m_frameCount;
    }
}

// Both table cleanup thunks tail-call this body. The paired constructor owns
// only the copied pattern-id array at +4; the nine index/count pairs are plain
// integers and need no cleanup.
VA(0x004f9ca0, 0x0b)
MAC_ADDRESS(0x2222f8, 0x54)  // cinit cleanups 0x55ed90/0x55f310; Complete-only
VA_AT(h3maped, 0x004587ed, 0x0a)
TRmgLinePatternTable::~TRmgLinePatternTable()
{
    delete[] m_framePatterns;
}

// This library-side copy is retained separately from the terrain selector's
// 0x642c00 table. Retail 0x4f9df0 indexes it with the two bytes from 0x63ff1c.
// Values and row order are read from the pinned image, not inferred rotations.
// Neighbour direction order for each (flipX, flipY) reflection; the same
// values as g_rmgReflectedNeighbours. A canonical pattern's direction d reads
// neighbour order[d]. North is up; each grid puts order[d] at d.
//   none      flipY     flipX     both
//   NW N NE   SW S SE   NE N NW   SE S SW
//   W  .  E   W  .  E   E  .  W   E  .  W
//   SW S SE   NW N NE   SE S SW   NE N NW
DATA(0x0063fe9c)
static const s32 g_rmgLineReflectedNeighbours[2][2][TILE_DIR_COUNT] = {
    {
        {TILE_DIR_NORTH, TILE_DIR_NORTHEAST, TILE_DIR_EAST, TILE_DIR_SOUTHEAST,
         TILE_DIR_SOUTH, TILE_DIR_SOUTHWEST, TILE_DIR_WEST, TILE_DIR_NORTHWEST}, // none
        {TILE_DIR_SOUTH, TILE_DIR_SOUTHEAST, TILE_DIR_EAST, TILE_DIR_NORTHEAST,
         TILE_DIR_NORTH, TILE_DIR_NORTHWEST, TILE_DIR_WEST, TILE_DIR_SOUTHWEST} // flipY
    },
    {
        {TILE_DIR_NORTH, TILE_DIR_NORTHWEST, TILE_DIR_WEST, TILE_DIR_SOUTHWEST,
         TILE_DIR_SOUTH, TILE_DIR_SOUTHEAST, TILE_DIR_EAST, TILE_DIR_NORTHEAST}, // flipX
        {TILE_DIR_SOUTH, TILE_DIR_SOUTHWEST, TILE_DIR_WEST, TILE_DIR_NORTHWEST,
         TILE_DIR_NORTH, TILE_DIR_NORTHEAST, TILE_DIR_EAST, TILE_DIR_SOUTHEAST} // both
    }
};

DATA(0x0063ff1c)
static const bool g_rmgLineReflections[4][2] = {
    {0, 0}, {0, 1}, {1, 0}, {1, 1}
};

VA(0x004f9cb0, 0x24e)
MAC_ADDRESS(0x222498, 0x2a4)
VA_AT(h3maped, 0x00458893, 0x19c)
void computeLineShape(
    const bool* neighbours, const TRmgLinePatternTable* table,
    s32& pattern, bool& flipX, bool& flipY)
{
    if (neighbours[TILE_DIR_NORTH] && neighbours[TILE_DIR_EAST]
        && neighbours[TILE_DIR_SOUTH] && neighbours[TILE_DIR_WEST]) {
        pattern = LINE_CROSS;
        flipX = 0;
        flipY = 0;
        return;
    }
    if (neighbours[TILE_DIR_NORTH] && neighbours[TILE_DIR_SOUTH]) {
        if (neighbours[TILE_DIR_EAST]) {
            pattern = LINE_NES;
            flipX = 0;
        } else if (neighbours[TILE_DIR_WEST]) {
            pattern = LINE_NES;
            flipX = 1;
        } else {
            pattern = LINE_NS;
            flipX = 0;
        }
        flipY = 0;
        return;
    }
    if (neighbours[TILE_DIR_EAST] && neighbours[TILE_DIR_WEST]) {
        if (neighbours[TILE_DIR_SOUTH]) {
            pattern = LINE_ESW;
            flipY = 0;
        } else if (neighbours[TILE_DIR_NORTH]) {
            pattern = LINE_ESW;
            flipY = 1;
        } else {
            pattern = LINE_EW;
            flipY = 0;
        }
        flipX = 0;
        return;
    }
    b8 hasCornerVariant = table->m_ranges[LINE_SE_VARIANT].m_frameCount > 0;
    for (u32 reflection = 0; reflection < 4; ++reflection) {
        const s32* order = g_rmgLineReflectedNeighbours
            [g_rmgLineReflections[reflection][0]][g_rmgLineReflections[reflection][1]];
        if (neighbours[order[TILE_DIR_EAST]] && neighbours[order[TILE_DIR_SOUTH]]) {
            if (hasCornerVariant && (neighbours[order[TILE_DIR_NORTHEAST]] || neighbours[order[TILE_DIR_SOUTHWEST]]))
                pattern = LINE_SE_VARIANT;
            else
                pattern = LINE_SE;
            flipX = g_rmgLineReflections[reflection][0];
            flipY = g_rmgLineReflections[reflection][1];
            return;
        }
    }
    if (table->m_ranges[LINE_END_S].m_frameCount > 0) {
        if (neighbours[TILE_DIR_WEST] || neighbours[TILE_DIR_EAST]) {
            pattern = LINE_END_E;
            flipX = neighbours[TILE_DIR_WEST];
            flipY = 0;
        } else {
            if (neighbours[TILE_DIR_SOUTH]) {
                pattern = LINE_END_S;
                flipX = 0;
                flipY = 0;
            } else {
                pattern = LINE_END_S;
                flipX = 0;
                flipY = 1;
            }
        }
    } else {
        pattern = neighbours[TILE_DIR_WEST] || neighbours[TILE_DIR_EAST] ? LINE_EW : LINE_NS;
        flipX = 0;
        flipY = 0;
    }
}

// Initializer-list copy of the point: the body assignment costs the
// walker's first neighbour pass its retained compound add; the factory and
// rectangle clear are byte-identical either way.
TRmgLinePainterTile::TRmgLinePainterTile(
    TMapLineFilter* painter, const TTilePoint& point)
    : m_painter(painter), m_point(point)
{
}

// Receiver/coordinate reference bindings, their order, named results and
// real proxy construction give no gain over the direct query in the clear,
// point and refresh callers.
s32 TRmgLinePainterTile::getType()
{
    return m_painter->getLineType(m_point);
}

void TRmgLinePainterTile::getTile(TRmgTerrainTile& tile)
{
    m_painter->getTile(m_point, tile);
}

void TRmgLinePainterTile::setTile(const TRmgTerrainTile& tile)
{
    m_painter->setTile(m_point, tile);
}

// The point walker tests AL after slot 3 (0x4fa400..0x4fa405). Keep that
// low-byte gate while preserving the retained virtual's integer-return ABI.
b8 TRmgLinePainterTile::isBlocked()
{
    return m_painter->isBlocked(m_point);
}

void TRmgLinePainterTile::setLineType(s32 value)
{
    m_painter->setLineType(m_point, value);
}

// The size is a grid point: the walker's one-cell rectangle then constructs
// a unit size the way retail 0x4fa3c0 materializes it (see paintPoint).
TRmgGridRectangle::TRmgGridRectangle(const TTilePoint& origin, const TTilePoint& size)
    : m_origin(origin), m_size(size)
{
}

// Mac retains this between the line-pattern table destructor and neighbour
// mask helper; recalcLineShape calls it at 0x222850. Its Windows expansion
// is local to this TU, so keep an ordinary source body visible to that caller.
MAC_ADDRESS(0x22234c, 0x54)
u32 TRmgLinePatternTable::selectFrame(s32 pattern)
{
    return m_ranges[pattern].m_firstFrame
        + rand() % m_ranges[pattern].m_frameCount;
}

// Complete retains this four-argument neighbour query in the editor and expands
// it in the game. Mac 0x2223a0 and Loki computeAdjacentLines identify the whole
// loop boundary; the former per-neighbour wrapper was only a budget hypothesis.
VA_AT(h3maped, 0x004587f7, 0x9c)
MAC_ADDRESS(0x2223a0, 0xf8)
void computeAdjacentLines(TMapLineFilter* pMapFilter, const TTilePoint& point,
                          s32 lineType, bool* abAdjLine)
{
    bool abAdjDir[TILE_DIR_COUNT];
    computeAdjacentDirs(pMapFilter->getWidth(), pMapFilter->getHeight(),
                        point.x(), point.y(), abAdjDir);
    for (u32 dir = 0; dir < TILE_DIR_COUNT; ++dir) {
        if (abAdjDir[dir]) {
            TTilePoint adjLoc = TPoint<int>(point) + akAdjOffset[dir];
            abAdjLine[dir] = pMapFilter->getCell(adjLoc).getType() == lineType;
        } else
            abAdjLine[dir] = false;
    }
}

// Windows and Mac keep the selected pattern separate from the old tile type.
// Loki also calls the canonical output-reference selector directly.
VA(0x004f9f00, 0x146)
MAC_ADDRESS(0x22273c, 0x168) // anchor-caller 0x4fa080/0x4fa3c0; fastcall, no stack args
VA_AT(h3maped, 0x00458a2f, 0xc7)
void recalcLineShape(TMapLineFilter* painter, const TTilePoint& point)
{
    TRmgLinePainterTile tile = painter->getCell(point);
    s32 oldType = tile.getType();
    bool matches[TILE_DIR_COUNT];
    computeAdjacentLines(painter, point, oldType, matches);
    TRmgLinePatternTable* table = painter->getPatternTable(oldType);
    s32 selected;
    bool flipX, flipY;
    computeLineShape(matches, table, selected, flipX, flipY);
    TRmgTerrainTile current;
    tile.getTile(current);
    if (table->m_framePatterns[current.getFrame()] != selected
        || current.getFlipX() != flipX || current.getFlipY() != flipY) {
        u32 frame = table->selectFrame(selected);
        current.m_frame = frame;
        current.m_flipX = flipX;
        current.m_flipY = flipY;
        tile.setTile(current);
    }
}

VA(0x004fa050, 0x22) // anchor-callee 0x4f9f86; thiscall hidden value return
TRmgLinePainterTile TMapLineFilter::getCell(const TTilePoint& point)
{
    return TRmgLinePainterTile(this, point);
}

// The first pass uses the filter factory, as Loki eraseLines does. Complete
// retains the resulting proxy copy in the editor and expands it in the game;
// both Windows bodies are exact with the ordinary factory call.
// Clears the rectangle, then refreshes the line tiles around it. North is up;
// # is the rectangle, digits the order in which its sides are refreshed.
//   1 3 3 3 2
//   1 # # # 2
//   1 4 4 4 2
VA(0x004fa080, 0x1fb)
MAC_ADDRESS(0x2228b8, 0x388) // anchor-callee 0x4fa42c; fastcall, no stack args
VA_AT(h3maped, 0x00458af6, 0x241)
void clearRmgLineRectangle(TMapLineFilter* painter, const TRmgGridRectangle& rectangle)
{
    TTilePoint point;
    for (point.m_y = rectangle.m_origin.m_y;
         point.m_y < rectangle.m_origin.m_y + rectangle.m_size.m_y; ++point.m_y) {
        for (point.m_x = rectangle.m_origin.m_x;
             point.m_x < rectangle.m_origin.m_x + rectangle.m_size.m_x; ++point.m_x) {
            TRmgLinePainterTile tile = painter->getCell(point);
            if (tile.getType())
                tile.setTile(TRmgTerrainTile(0, 0));
        }
    }
    if (rectangle.m_origin.m_x > 0) {
        point.m_x = rectangle.m_origin.m_x - 1;
        u32 first = rectangle.m_origin.m_y > 0 ? rectangle.m_origin.m_y - 1 : 0;
        u32 end = rectangle.m_origin.m_y + rectangle.m_size.m_y < painter->m_size.m_y
            ? rectangle.m_origin.m_y + rectangle.m_size.m_y + 1
            : rectangle.m_origin.m_y + rectangle.m_size.m_y;
        for (point.m_y = first; point.m_y < end; ++point.m_y) {
            if (painter->getCell(point).getType())
                recalcLineShape(painter, point);
        }
    }
    if (rectangle.m_origin.m_x + rectangle.m_size.m_x < painter->m_size.m_x) {
        point.m_x = rectangle.m_origin.m_x + rectangle.m_size.m_x;
        u32 first = rectangle.m_origin.m_y > 0 ? rectangle.m_origin.m_y - 1 : 0;
        u32 end = rectangle.m_origin.m_y + rectangle.m_size.m_y < painter->m_size.m_y - 1
            ? rectangle.m_origin.m_y + rectangle.m_size.m_y + 1
            : rectangle.m_origin.m_y + rectangle.m_size.m_y;
        for (point.m_y = first; point.m_y < end; ++point.m_y) {
            if (painter->getCell(point).getType())
                recalcLineShape(painter, point);
        }
    }
    if (rectangle.m_origin.m_y > 0) {
        point.m_y = rectangle.m_origin.m_y - 1;
        for (point.m_x = rectangle.m_origin.m_x;
             point.m_x < rectangle.m_origin.m_x + rectangle.m_size.m_x; ++point.m_x) {
            if (painter->getCell(point).getType())
                recalcLineShape(painter, point);
        }
    }
    if (rectangle.m_origin.m_y + rectangle.m_size.m_y < painter->m_size.m_y) {
        point.m_y = rectangle.m_origin.m_y + rectangle.m_size.m_y;
        for (point.m_x = rectangle.m_origin.m_x;
             point.m_x < rectangle.m_origin.m_x + rectangle.m_size.m_x; ++point.m_x) {
            if (painter->getCell(point).getType())
                recalcLineShape(painter, point);
        }
    }
}

VA(0x004fa280, 0x30)
MAC_ADDRESS(0x222c40, 0x4c) // anchor-caller 0x55ee50/0x55f3b0; thiscall ret 0xc
TRmgLineWalker::TRmgLineWalker(
    TMapLineFilter* newPainter,
    s32 newLineType,
    const TTilePoint& start)
    : m_painter(newPainter), m_lineType(newLineType), m_position(start)
{
    paintPoint(m_position);
}

VA(0x004fa2b0, 0x110)
MAC_ADDRESS(0x222c8c, 0x190) // anchor-caller 0x548040 and createRiver; thiscall ret 4
void TRmgLineWalker::drawTo(const TTilePoint& destination)
{
    TRmgLineWalkAxis x(destination.m_x, m_position.m_x);
    TRmgLineWalkAxis y(destination.m_y, m_position.m_y);
    TRmgLineWalkAxis* major;
    TRmgLineWalkAxis* minor;
    if (x.m_distance >= y.m_distance) {
        major = &x;
        minor = &y;
    } else {
        major = &y;
        minor = &x;
    }
    u32 error = 0;
    u32 distance = major->m_distance;
    for (u32 index = 0; index < distance; ++index) {
        paintPoint(TTilePoint(x.m_position, y.m_position));
        error += minor->m_distance;
        if (error >= major->m_distance) {
            error -= major->m_distance;
            minor->m_position += minor->m_step;
            paintPoint(TTilePoint(x.m_position, y.m_position));
        }
        major->m_position += major->m_step;
    }
    if (error + minor->m_distance >= major->m_distance)
        paintPoint(TTilePoint(x.m_position, y.m_position));
    m_position = destination;
}

VA(0x004fa3c0, 0x156)
MAC_ADDRESS(0x222e1c, 0x18c) // anchor-caller 0x4fa280/0x4fa2b0; thiscall, ret 4
void TRmgLineWalker::paintPoint(const TTilePoint& point)
{
    TRmgLinePainterTile tile = m_painter->getCell(point);
    s32 oldType = tile.getType();
    if (oldType == m_lineType || tile.isBlocked())
        return;
    if (oldType)
        clearRmgLineRectangle(m_painter, TRmgGridRectangle(point, TTilePoint(1, 1)));
    tile.setLineType(m_lineType);
    recalcLineShape(m_painter, point);

    bool matches[TILE_DIR_COUNT];
    computeAdjacentLines(m_painter, point, m_lineType, matches);
    u32 direction;
    for (direction = 0; direction < TILE_DIR_COUNT; ++direction) {
        if (matches[direction])
            recalcLineShape(m_painter, TPoint<int>(point) + akAdjOffset[direction]);
    }
}

VA_AT(h3maped, 0x00458f52, 0x0b)
TLineEraseOp::TLineEraseOp(TMapLineFilter* newPainter)
    : m_painter(newPainter)
{
}

VA_AT(h3maped, 0x00458f5d, 0x33)
void TLineEraseOp::operator()(unsigned int left, unsigned int top, unsigned int width,
                              unsigned int height)
{
    clearRmgLineRectangle(m_painter, TRmgGridRectangle(TTilePoint(left, top),
                                                       TTilePoint(width, height)));
}
