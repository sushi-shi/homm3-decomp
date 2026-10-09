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
static const b8 g_rmgLineReflections[4][2] = {
    {0, 0}, {0, 1}, {1, 0}, {1, 1}
};

VA(0x004f9cb0, 0x24e)
MAC_ADDRESS(0x222498, 0x2a4)
void selectRmgLinePattern(
    const b8* neighbours, const TRmgLinePatternTable* table,
    s32& pattern, b8& flipX, b8& flipY)
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
// walker's first neighbour pass its retained compound add (78.03 against
// 87.12%); the factory and rectangle clear are byte-identical either way.
TRmgLinePainterTile::TRmgLinePainterTile(
    TMapLineFilter* painter, const TTilePoint& point)
    : m_painter(painter), m_point(point)
{
}

// The 168-case query family tested receiver/coordinate reference bindings,
// their order, named results and real proxy construction (80 code results).
// No gain over the direct query: clear 94.5070%, point 79.9380%, refresh 63.9923%.
s32 TRmgLinePainterTile::getLineType()
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
// mask helper; refreshRmgLinePoint calls it at 0x222850. Its Windows expansion
// is local to this TU, so keep an ordinary source body visible to that caller.
MAC_ADDRESS(0x22234c, 0x54)
u32 TRmgLinePatternTable::selectFrame(s32 pattern)
{
    return m_ranges[pattern].m_firstFrame
        + rand() % m_ranges[pattern].m_frameCount;
}

// Retail 0x4f9f00 keeps one tile proxy across neighbour queries and selects
// a pattern/flip pair before reading the current tile. A new random frame is
// drawn only when the pattern or flips differ. Its ordinary neighbour helper
// supplies nested TPoint-add, grid-conversion and proxy-factory call sites.
// Returning the selected integer through this ordinary overload preserves the
// canonical output-reference selector and the two independent byte outputs.
// It recovers retail's register roles and selected-pattern comparison; a
// returned aggregate packs the flips together and changes their lifetimes.
// No DC counterpart: these helper boundaries are retail-derived hypotheses.
// Residual 99.7462%: all instructions/calls align, but the selector output
// shares the old-terrain slot; retail keeps both and has a 0x5c vs 0x58 frame.
// Value/const-value/const-reference caller bindings do not separate the slots.
s32 selectRmgLinePattern(
    const b8* neighbours, const TRmgLinePatternTable* table,
    b8& flipX, b8& flipY)
{
    s32 pattern;
    selectRmgLinePattern(neighbours, table, pattern, flipX, flipY);
    return pattern;
}

VA(0x004f9f00, 0x146)
MAC_ADDRESS(0x22273c, 0x168) // anchor-caller 0x4fa080/0x4fa3c0; fastcall, no stack args
void refreshRmgLinePoint(TMapLineFilter* painter, const TTilePoint& point)
{
    TRmgLinePainterTile tile = painter->at(point);
    s32 oldType = tile.getLineType();
    b8 available[TILE_DIR_COUNT];
    buildTileNeighbourMask(painter->m_size.m_x, painter->m_size.m_y,
                           point.m_x, point.m_y, available);
    b8 matches[TILE_DIR_COUNT];
    for (u32 direction = 0; direction < TILE_DIR_COUNT; ++direction) {
        if (available[direction])
            matches[direction] = painter->getNeighbourLineType(point, direction) == oldType;
        else
            matches[direction] = 0;
    }
    TRmgLinePatternTable* table = painter->getPatternTable(oldType);
    b8 flipX, flipY;
    s32 selected = selectRmgLinePattern(matches, table, flipX, flipY);
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
TRmgLinePainterTile TMapLineFilter::at(const TTilePoint& point)
{
    return TRmgLinePainterTile(this, point);
}

// Neighbour query used by refresh 0x4f9f00 and the walker's first pass
// 0x4fa3c0: an ordinary helper whose expansion carries the signed sum,
// the grid conversion and the proxy factory as nested sites. Retail
// retains all three in refresh and only the compound add in the walker,
// which is what this helper's divided budget gives them; queried inline
// they are expanded at both callers' own budgets. The converted sum is
// named: the walker's expansion then stores the proxy's painter before
// the converted coordinates as retail does (95.85 -> 100%; a named proxy
// costs refresh its factory call, 89.41%; a named signed sum is the same
// object; the proxy constructor copying the point through its fields,
// accessors, setters or a by-value parameter never helps and the first
// three cost the rectangle clear, 94.51%).
// Mac 0x2223a0 also wraps the complete matching-neighbour-mask loops used
// by refresh and the walker. Restoring that outer boundary currently retains
// extra grid conversion/addition calls in refresh and grid-constructor/proxy
// calls in the walker (84.8077% / 78.8527%); its source calls need joint recovery.
s32 TMapLineFilter::getNeighbourLineType(const TTilePoint& point, u32 direction)
{
    TTilePoint nearby = TPoint<int>(point) + g_tileDirections[direction];
    return at(nearby).getLineType();
}

// Clears the rectangle, then refreshes the line tiles around it. North is up;
// # is the rectangle, digits the order in which its sides are refreshed.
//   1 3 3 3 2
//   1 # # # 2
//   1 4 4 4 2
VA(0x004fa080, 0x1fb)
MAC_ADDRESS(0x2228b8, 0x388) // anchor-callee 0x4fa42c; fastcall, no stack args
void clearRmgLineRectangle(TMapLineFilter* painter, const TRmgGridRectangle& rectangle)
{
    TTilePoint point;
    for (point.m_y = rectangle.m_origin.m_y;
         point.m_y < rectangle.m_origin.m_y + rectangle.m_size.m_y; ++point.m_y) {
        for (point.m_x = rectangle.m_origin.m_x;
             point.m_x < rectangle.m_origin.m_x + rectangle.m_size.m_x; ++point.m_x) {
            TRmgLinePainterTile tile(painter, point);
            if (tile.getLineType())
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
            if (painter->at(point).getLineType())
                refreshRmgLinePoint(painter, point);
        }
    }
    if (rectangle.m_origin.m_x + rectangle.m_size.m_x < painter->m_size.m_x) {
        point.m_x = rectangle.m_origin.m_x + rectangle.m_size.m_x;
        u32 first = rectangle.m_origin.m_y > 0 ? rectangle.m_origin.m_y - 1 : 0;
        u32 end = rectangle.m_origin.m_y + rectangle.m_size.m_y < painter->m_size.m_y - 1
            ? rectangle.m_origin.m_y + rectangle.m_size.m_y + 1
            : rectangle.m_origin.m_y + rectangle.m_size.m_y;
        for (point.m_y = first; point.m_y < end; ++point.m_y) {
            if (painter->at(point).getLineType())
                refreshRmgLinePoint(painter, point);
        }
    }
    if (rectangle.m_origin.m_y > 0) {
        point.m_y = rectangle.m_origin.m_y - 1;
        for (point.m_x = rectangle.m_origin.m_x;
             point.m_x < rectangle.m_origin.m_x + rectangle.m_size.m_x; ++point.m_x) {
            if (painter->at(point).getLineType())
                refreshRmgLinePoint(painter, point);
        }
    }
    if (rectangle.m_origin.m_y + rectangle.m_size.m_y < painter->m_size.m_y) {
        point.m_y = rectangle.m_origin.m_y + rectangle.m_size.m_y;
        for (point.m_x = rectangle.m_origin.m_x;
             point.m_x < rectangle.m_origin.m_x + rectangle.m_size.m_x; ++point.m_x) {
            if (painter->at(point).getLineType())
                refreshRmgLinePoint(painter, point);
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
    TRmgLinePainterTile tile(m_painter, point);
    s32 oldType = tile.getLineType();
    if (oldType == m_lineType || tile.isBlocked())
        return;
    if (oldType)
        clearRmgLineRectangle(m_painter, TRmgGridRectangle(point, TTilePoint(1, 1)));
    tile.setLineType(m_lineType);
    refreshRmgLinePoint(m_painter, point);

    s32 lineType = m_lineType;
    b8 matches[TILE_DIR_COUNT];
    u32 direction;
    {
        TMapLineFilter* painter = m_painter;
        b8 available[TILE_DIR_COUNT];
        buildTileNeighbourMask(painter->m_size.m_x, painter->m_size.m_y,
                               point.m_x, point.m_y, available);
        for (direction = 0; direction < TILE_DIR_COUNT; ++direction) {
            if (available[direction])
                matches[direction] = painter->getNeighbourLineType(point, direction) == lineType;
            else
                matches[direction] = 0;
        }
    }
    for (direction = 0; direction < TILE_DIR_COUNT; ++direction) {
        if (matches[direction])
            refreshRmgLinePoint(m_painter, TPoint<int>(point) + g_tileDirections[direction]);
    }
}
