// Random-map terrain, road and river tile painting.
#include "va.h"
#include "includes.h"

#include <stdlib.h>

#include "rmg_terrain.h"

#include "exceptions.h"
#include "tiles.h"

// The fixed table has no special frames.
MAC_ADDRESS(0x259d44, 0x8)
b8 TRmgTableTerrainRule::isSpecialFrame(int) { return false; }

TRmgLinePainterTile::TRmgLinePainterTile(
    TRmgLinePainterInterface* painter, const TRmgGridPoint& point)
    : m_painter(painter), m_point(point)
{
}

int TRmgLinePainterTile::getLineType()
{
    return m_painter->getLineType(m_point);
}

void TRmgLinePainterTile::getTile(rmgTerrainTile& tile)
{
    m_painter->getTile(m_point, tile);
}

void TRmgLinePainterTile::setTile(const rmgTerrainTile& tile)
{
    m_painter->setTile(m_point, tile);
}

b8 TRmgLinePainterTile::isBlocked()
{
    return m_painter->isBlocked(m_point);
}

void TRmgLinePainterTile::setLineType(int value)
{
    m_painter->setLineType(m_point, value);
}

TRmgGridRectangle::TRmgGridRectangle(const TRmgGridPoint& origin, const TRmgGridPoint& size)
    : m_origin(origin), m_size(size)
{
}

int selectRmgLinePattern(
    const b8* neighbours, const TRmgLinePatternTable* table,
    b8& flipX, b8& flipY)
{
    int pattern;
    selectRmgLinePattern(neighbours, table, pattern, flipX, flipY);
    return pattern;
}

// Marks each on-map neighbour that carries the same line type.
static inline void buildMatchingLineNeighbourMask(
    TRmgLinePainterInterface* painter, const TRmgGridPoint& point,
    int lineType, b8* matches)
{
    b8 available[TILE_DIR_COUNT];
    buildTileNeighbourMask(painter->m_size.m_x, painter->m_size.m_y,
                           point.m_x, point.m_y, available);
    for (u32 direction = 0; direction < TILE_DIR_COUNT; ++direction) {
        if (available[direction])
            matches[direction] = painter->getNeighbourLineType(point, direction) == lineType;
        else
            matches[direction] = false;
    }
}

VA(0x004F9F00, 0x146)
MAC_ADDRESS(0x22273c, 0x168)
void refreshRmgLinePoint(TRmgLinePainterInterface* painter, const TRmgGridPoint& point)
{
    TRmgLinePainterTile tile = painter->at(point);
    int lineType = tile.getLineType();
    b8 matches[TILE_DIR_COUNT];
    buildMatchingLineNeighbourMask(painter, point, lineType, matches);
    TRmgLinePatternTable* table = painter->getPattern(lineType);
    b8 flipX, flipY;
    int selected = selectRmgLinePattern(matches, table, flipX, flipY);
    rmgTerrainTile current;
    tile.getTile(current);
    if (table->m_framePatterns[current.getFrame()] != selected
        || current.getFlipX() != flipX || current.getFlipY() != flipY) {
        u32 frame = table->m_ranges[selected].m_firstIndex
            + rand() % table->m_ranges[selected].m_frameCount;
        current.m_frame = frame;
        current.m_flipX = flipX;
        current.m_flipY = flipY;
        tile.setTile(current);
    }
}

VA(0x004FA050, 0x22)
TRmgLinePainterTile TRmgLinePainterInterface::at(const TRmgGridPoint& point)
{
    return TRmgLinePainterTile(this, point);
}

int TRmgLinePainterInterface::getNeighbourLineType(const TRmgGridPoint& point, u32 direction)
{
    TRmgGridPoint nearby = point + g_tileDirections[direction];
    return at(nearby).getLineType();
}

// Border cells without a line are skipped and consume no frame draw.
static inline void refreshExistingRmgLinePoint(
    TRmgLinePainterInterface* painter, const TRmgGridPoint& point)
{
    if (painter->at(point).getLineType())
        refreshRmgLinePoint(painter, point);
}

// Refreshes one border column from the row above the rectangle through the
// row below it, when that row stays below the limit.
static inline void refreshRmgLineBorderColumn(TRmgLinePainterInterface* painter,
    u32 x, const TRmgGridRectangle& rectangle, u32 limit)
{
    u32 first = rectangle.m_origin.m_y > 0 ? rectangle.m_origin.m_y - 1 : 0;
    u32 end = rectangle.m_origin.m_y + rectangle.m_size.m_y;
    if (end < limit)
        ++end;
    for (TRmgGridPoint point(x, first); point.m_y < end; ++point.m_y)
        refreshExistingRmgLinePoint(painter, point);
}

// Refreshes one border row across the rectangle's width.
static inline void refreshRmgLineBorderRow(TRmgLinePainterInterface* painter,
    u32 y, const TRmgGridRectangle& rectangle)
{
    for (TRmgGridPoint point(rectangle.m_origin.m_x, y);
         point.m_x < rectangle.m_origin.m_x + rectangle.m_size.m_x; ++point.m_x)
        refreshExistingRmgLinePoint(painter, point);
}

VA(0x004FA080, 0x1FB)
MAC_ADDRESS(0x2228b8, 0x388)
void clearRmgLineRectangle(TRmgLinePainterInterface* painter, const TRmgGridRectangle& rectangle)
{
    TRmgGridPoint point;
    for (point.m_y = rectangle.m_origin.m_y;
         point.m_y < rectangle.m_origin.m_y + rectangle.m_size.m_y; ++point.m_y) {
        for (point.m_x = rectangle.m_origin.m_x;
             point.m_x < rectangle.m_origin.m_x + rectangle.m_size.m_x; ++point.m_x) {
            TRmgLinePainterTile tile(painter, point);
            if (tile.getLineType())
                tile.setTile(rmgTerrainTile(0, 0));
        }
    }
    if (rectangle.m_origin.m_x > 0) {
        refreshRmgLineBorderColumn(painter, rectangle.m_origin.m_x - 1,
            rectangle, painter->m_size.m_y);
    }
    if (rectangle.m_origin.m_x + rectangle.m_size.m_x < painter->m_size.m_x) {
        // Retail quirk: unlike the left edge, a rectangle ending on the
        // penultimate row does not refresh the last row here.
        refreshRmgLineBorderColumn(painter, rectangle.m_origin.m_x + rectangle.m_size.m_x,
            rectangle, painter->m_size.m_y - 1);
    }
    if (rectangle.m_origin.m_y > 0)
        refreshRmgLineBorderRow(painter, rectangle.m_origin.m_y - 1, rectangle);
    if (rectangle.m_origin.m_y + rectangle.m_size.m_y < painter->m_size.m_y)
        refreshRmgLineBorderRow(painter, rectangle.m_origin.m_y + rectangle.m_size.m_y, rectangle);
}

VA(0x004FA280, 0x30)
MAC_ADDRESS(0x222c40, 0x4c)
TRmgLineWalker::TRmgLineWalker(
    TRmgLinePainterInterface* newPainter,
    int newLineType,
    const TRmgGridPoint& start)
    : m_painter(newPainter), m_lineType(newLineType), m_position(start)
{
    paintPoint(m_position);
}

// Four-connected Bresenham line: the corner cell is painted on each
// minor-axis step so the road or river stays connected.
VA(0x004FA2B0, 0x110)
MAC_ADDRESS(0x222c8c, 0x190)
void TRmgLineWalker::drawTo(const TRmgGridPoint& destination)
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
        paintPoint(TRmgGridPoint(x.m_position, y.m_position));
        error += minor->m_distance;
        if (error >= major->m_distance) {
            error -= major->m_distance;
            minor->m_position += minor->m_step;
            paintPoint(TRmgGridPoint(x.m_position, y.m_position));
        }
        major->m_position += major->m_step;
    }
    if (error + minor->m_distance >= major->m_distance)
        paintPoint(TRmgGridPoint(x.m_position, y.m_position));
    m_position = destination;
}

VA(0x004FA3C0, 0x156)
MAC_ADDRESS(0x222e1c, 0x18c)
void TRmgLineWalker::paintPoint(const TRmgGridPoint& point)
{
    TRmgLinePainterTile tile(m_painter, point);
    int oldType = tile.getLineType();
    if (oldType == m_lineType || tile.isBlocked())
        return;
    if (oldType)
        clearRmgLineRectangle(m_painter, TRmgGridRectangle(point, TRmgGridPoint(1, 1)));
    tile.setLineType(m_lineType);
    refreshRmgLinePoint(m_painter, point);

    b8 matches[TILE_DIR_COUNT];
    buildMatchingLineNeighbourMask(m_painter, point, m_lineType, matches);
    for (u32 direction = 0; direction < TILE_DIR_COUNT; ++direction) {
        if (matches[direction])
            refreshRmgLinePoint(m_painter, point + g_tileDirections[direction]);
    }
}

template<class Coordinate>
// VA instance: TRmgCoordinatePoint<unsigned int>::TRmgCoordinatePoint(const TPoint&)
VA(0x004FA520, 0x16)
MAC_ADDRESS(0x2228a4, 0x14)
TRmgCoordinatePoint<Coordinate>::TRmgCoordinatePoint(const TPoint& point)
    : m_x(point.m_x), m_y(point.m_y)
{
}

VA(0x004FA540, 0x21)
TPoint& TPoint::operator+=(const TPoint& offset)
{
    m_x += offset.m_x;
    m_y += offset.m_y;
    return *this;
}

// Cell transition shapes, unreflected (flipX/flipY give the rest). Names list
// the other-terrain neighbours, each run followed by its edge kind: N_W = both
// sides (outer corner), SE = that diagonal only, DIAG = corner on a 45-degree
// edge. Id 1 is unused. Diagrams: docs/reference/rmg-terrain-shapes.md
// The basic blend shapes as their main rules find them (hard = blend + 6).
// North is up; C is the cell, e an edge of its kind, . none, ? not fixed.
//   N_W    W      N      SE
//   ? e ?  ? . .  ? e ?  . . ?
//   e C ?  e C ?  . C .  . C .
//   ? ? .  ? . .  . ? .  ? . e
enum ERmgTerrainShape {
    SHAPE_FILL = 0,
    SHAPE_N_W_BLEND = 2,
    SHAPE_W_BLEND = 3,
    SHAPE_N_BLEND = 4,
    SHAPE_SE_BLEND = 5,
    SHAPE_N_W_DIAG_BLEND = 6,
    SHAPE_SE_DIAG_BLEND = 7,
    SHAPE_N_W_HARD = 8,
    SHAPE_W_HARD = 9,
    SHAPE_N_HARD = 10,
    SHAPE_SE_HARD = 11,
    SHAPE_N_W_DIAG_HARD = 12,
    SHAPE_SE_DIAG_HARD = 13,
    SHAPE_NW_SE_BLEND = 14,
    SHAPE_NW_BLEND_SE_HARD = 15,
    SHAPE_NW_SE_HARD = 16,
    SHAPE_E_BLEND_SW_HARD = 17,
    SHAPE_S_BLEND_NE_HARD = 18,
    SHAPE_E_BLEND_SE_HARD = 19,
    SHAPE_S_BLEND_SE_HARD = 20,
    SHAPE_E_HARD_SW_BLEND = 21,
    SHAPE_S_HARD_NE_BLEND = 22,
    SHAPE_N_W_SE_BLEND = 23,
    SHAPE_N_W_SE_HARD = 24,
    SHAPE_N_W_BLEND_SE_HARD = 25,
    SHAPE_N_W_HARD_SE_BLEND = 26,
    SHAPE_E_S_BLEND_SE_HARD = 27,
    SHAPE_E_S_BLEND_NE_SW_HARD = 28
};

// Range key: transition, then the special flag (0/1).
static inline TRmgTerrainPatternRange& getTerrainPatternRange(
    TRmgPatternTerrainRule& rule, int transition, b8 special)
{
    return rule.m_ranges[transition * 2 + special];
}

VA(0x005B3780, 0xB3)
MAC_ADDRESS(0x254be0, 0xf4)
TRmgPatternTerrainRule::TRmgPatternTerrainRule(
    b8 blendsWithOtherTerrain, b8 allowsSeparatedNeighbours,
    int specialFrameChance, u32 entryCount, const TRmgTerrainPatternEntry* entries)
    : TRmgTerrainRule(blendsWithOtherTerrain, allowsSeparatedNeighbours),
      m_specialFrameChance(specialFrameChance), m_entryCount(entryCount), m_entries(entries)
{
    int transition = m_entries[0].m_transition;
    b8 special = m_entries[0].m_special;
    TRmgTerrainPatternRange* range = &getTerrainPatternRange(*this, transition, special);
    ++range->m_count;
    for (u32 index = 1; index < m_entryCount; ++index) {
        const TRmgTerrainPatternEntry& entry = m_entries[index];
        if (entry.m_transition != transition || entry.m_special != special) {
            transition = entry.m_transition;
            special = entry.m_special;
            range = &getTerrainPatternRange(*this, transition, special);
            range->m_firstIndex = index;
        }
        ++range->m_count;
    }
}

// True when the terrain has special (decorated) base frames.
VA(0x005B3840, 0x0C)
MAC_ADDRESS(0x259dac, 0x14)
b8 TRmgPatternTerrainRule::hasSpecialBaseFrames()
{
    return 0 < getTerrainPatternRange(*this, SHAPE_FILL, true).m_count;
}

VA(0x005B3850, 0x07)
MAC_ADDRESS(0x254b98, 0x48)
TRmgTerrainRule::~TRmgTerrainRule()
{
}

VA(0x005B3860, 0x11)
MAC_ADDRESS(0x254ce4, 0x14)
b8 TRmgPatternTerrainRule::isSpecialFrame(int frame)
{
    return m_entries[frame].m_special;
}

VA(0x005B3880, 0x10)
MAC_ADDRESS(0x254cf8, 0x10)
int TRmgPatternTerrainRule::getTransition(int frame)
{
    return m_entries[frame].m_transition;
}

// Draws a random frame from a nonempty range (one rand() call, modulo bias).
static inline int selectTerrainRangeFrame(const TRmgTerrainPatternRange& range)
{
    return rand() % range.m_count + range.m_firstIndex;
}

// An existing frame (-1 for none) is kept when it already shows the
// requested transition.
template<class Entry>
static inline bool matchesTerrainFrame(const Entry* entries,
    int oldFrame, int transition)
{
    return oldFrame != -1 && entries[oldFrame].m_transition == transition;
}

VA(0x005B3890, 0x58)
MAC_ADDRESS(0x254d08, 0xc8)
int TRmgPatternTerrainRule::selectBaseFrame(int strength, int oldFrame)
{
    if (!matchesTerrainFrame(m_entries, oldFrame, SHAPE_FILL)) {
        b8 special = false;
        if (hasSpecialBaseFrames()) {
            u32 chance = static_cast<u32>(m_specialFrameChance * strength)
                / RMG_FULL_BRUSH_STRENGTH;
            special = static_cast<u32>(rand() % 100) < chance;
        }
        oldFrame = selectTerrainRangeFrame(getTerrainPatternRange(*this, SHAPE_FILL, special));
    }
    return oldFrame;
}

// Transitions always draw from the non-special range.
VA(0x005B38F0, 0x41)
MAC_ADDRESS(0x254dd0, 0x88)
int TRmgPatternTerrainRule::selectTransitionFrame(
    int transition,
    TRmgTerrainFlip requestedFlip,
    TRmgTerrainFlip& selectedFlip,
    int oldFrame)
{
    if (!matchesTerrainFrame(m_entries, oldFrame, transition)) {
        oldFrame = selectTerrainRangeFrame(getTerrainPatternRange(*this, transition, false));
    }
    selectedFlip = requestedFlip;
    return oldFrame;
}

static inline bool matchesTerrainTransition(
    const TRmgTerrainTransitionEntry& entry, int transition,
    b8 flipX, b8 flipY)
{
    return entry.m_transition == transition
        && entry.m_flipX == flipX && entry.m_flipY == flipY;
}

// Range key: transition, then flipX, then flipY (each flip 0/1).
static inline TRmgTerrainPatternRange& getTerrainTransitionRange(
    TRmgTerrainPatternTable& table, int transition,
    b8 flipX, b8 flipY)
{
    return table.m_ranges[(transition * 2 + flipX) * 2 + flipY];
}

// Frames in the fixed (rock) transition table.
enum ERmgFixedTransitionTableLimits {
    RMG_FIXED_TRANSITION_FRAME_COUNT = 48
};

DATA(0x006A4158)
TRmgTerrainPatternTable g_rmgTerrainPatternRanges;

VA(0x005B3940, 0xC5)
MAC_ADDRESS(0x254e58, 0xe4)
TRmgTerrainPatternTable::TRmgTerrainPatternTable()
{
    int transition = g_rmgTerrainPatterns[0].m_transition;
    b8 flipX = g_rmgTerrainPatterns[0].m_flipX;
    b8 flipY = g_rmgTerrainPatterns[0].m_flipY;
    TRmgTerrainPatternRange* range =
        &getTerrainTransitionRange(*this, transition, flipX, flipY);
    ++range->m_count;
    for (u32 index = 1; index < RMG_FIXED_TRANSITION_FRAME_COUNT; ++index) {
        if (!matchesTerrainTransition(g_rmgTerrainPatterns[index], transition, flipX, flipY)) {
            transition = g_rmgTerrainPatterns[index].m_transition;
            flipX = g_rmgTerrainPatterns[index].m_flipX;
            flipY = g_rmgTerrainPatterns[index].m_flipY;
            range = &getTerrainTransitionRange(*this, transition, flipX, flipY);
            range->m_firstIndex = index;
        }
        ++range->m_count;
    }
}

VA(0x005B3A20, 0x11)
MAC_ADDRESS(0x254f4c, 0x20)
TRmgTableTerrainRule::TRmgTableTerrainRule()
{
}

VA(0x005B3A40, 0x03)
MAC_ADDRESS(0x254f6c, 0x8)
b8 TRmgTableTerrainRule::hasSpecialBaseFrames()
{
    return false;
}

VA_COMPGEN(0x005B3A50, 0x21, SCALAR_DELETING_DTOR, TRmgTableTerrainRule)

VA(0x005B3A80, 0x11)
MAC_ADDRESS(0x254f74, 0x10)
int TRmgTableTerrainRule::getTransition(int frame)
{
    return g_rmgTerrainPatterns[frame].m_transition;
}

VA(0x005B3AA0, 0x31)
MAC_ADDRESS(0x254f84, 0x94)
int TRmgTableTerrainRule::selectBaseFrame(int, int oldFrame)
{
    if (!matchesTerrainFrame(g_rmgTerrainPatterns, oldFrame, SHAPE_FILL)) {
        oldFrame = selectTerrainRangeFrame(getTerrainTransitionRange(
            g_rmgTerrainPatternRanges, SHAPE_FILL, false, false));
    }
    return oldFrame;
}

VA(0x005B3AE0, 0x74)
MAC_ADDRESS(0x255018, 0xd8)
int TRmgTableTerrainRule::selectTransitionFrame(
    int transition, TRmgTerrainFlip requestedFlip,
    TRmgTerrainFlip& selectedFlip, int oldFrame)
{
    if (oldFrame == -1
        || !matchesTerrainTransition(g_rmgTerrainPatterns[oldFrame], transition,
                                     requestedFlip.m_flipX, requestedFlip.m_flipY)) {
        TRmgTerrainPatternRange& range = getTerrainTransitionRange(
            g_rmgTerrainPatternRanges, transition, requestedFlip.m_flipX, requestedFlip.m_flipY);
        oldFrame = selectTerrainRangeFrame(range);
    }
    selectedFlip = TRmgTerrainFlip(false, false);
    return oldFrame;
}

int __fastcall selectTerrainTransition(
    const int* neighbours, TRmgTerrainFlip* flip);

// Neighbour direction order for each (flipX, flipY) reflection. North is up;
// each grid puts order[d], a TILE_DIR_* index, at canonical direction d.
//   none   flipY  flipX  both
//   7 0 1  5 4 3  1 0 7  3 4 5
//   6 . 2  6 . 2  2 . 6  2 . 6
//   5 4 3  7 0 1  3 4 5  1 0 7
DATA(0x00642C00)
const s32 g_rmgReflectedNeighbours[2][2][8] = {
    {{0, 1, 2, 3, 4, 5, 6, 7}, {4, 3, 2, 1, 0, 7, 6, 5}},
    {{0, 7, 6, 5, 4, 3, 2, 1}, {4, 5, 6, 7, 0, 1, 2, 3}}
};

static inline const int* getReflectedTerrainNeighbourOrder(const TRmgTerrainFlip& flip)
{
    return g_rmgReflectedNeighbours[flip.m_flipX][flip.m_flipY];
}

static TRmgTerrainFlip makeTerrainFlip(b8 x, b8 y)
{
    return TRmgTerrainFlip(x, y);
}

VA(0x005B3DD0, 0x6F)
MAC_ADDRESS(0x2551b4, 0xd0)
void rmgTerrainPainter::initializePackedCell(
    const TRmgGridPoint& point, u32 index)
{
    rmgTerrainTile tile = m_adapter->getTile(point);
    TRmgPackedTerrainCell& packed = m_packedCells[index];
    packed.setTileValues(tile);
    packed.setInitialized();
}

VA(0x005B3E40, 0x38)
MAC_ADDRESS(0x255284, 0x64)
int __fastcall getRmgTerrainNeighbourKind(int terrain, int neighbourTerrain)
{
    if (terrain == neighbourTerrain || terrain == eTerrainSand)
        return RMG_NEIGHBOUR_NO_EDGE;
    const TRmgTerrainRule* rule = g_rmgTerrainRules[terrain];
    const TRmgTerrainRule* neighbourRule = g_rmgTerrainRules[neighbourTerrain];
    if (rule->m_blendsWithOtherTerrain) {
        if (neighbourRule->m_blendsWithOtherTerrain)
            return terrain == eTerrainDirt
                ? RMG_NEIGHBOUR_NO_EDGE : RMG_NEIGHBOUR_BLEND_EDGE;
    }
    return RMG_NEIGHBOUR_HARD_EDGE;
}

// Pattern predicates over neighbour kinds, read through a reflection order.
static inline bool hasSoutheastTerrainCorner(const int* neighbours,
    const int* order, int eastKind, int southKind)
{
    return neighbours[order[TILE_DIR_EAST]] == eastKind
        && neighbours[order[TILE_DIR_SOUTH]] == southKind;
}

static inline bool hasNorthwestTerrainCorner(const int* neighbours,
    const int* order, int kind)
{
    return neighbours[order[TILE_DIR_NORTH]] == kind
        && neighbours[order[TILE_DIR_WEST]] == kind;
}

static inline bool hasOffsetNorthwestTerrainCorner(const int* neighbours,
    const int* order, int kind)
{
    return (neighbours[order[TILE_DIR_WEST]] == kind
            && neighbours[order[TILE_DIR_NORTHEAST]] == kind)
        || (neighbours[order[TILE_DIR_NORTH]] == kind
            && neighbours[order[TILE_DIR_SOUTHWEST]] == kind);
}

static inline bool hasOppositeTerrainDiagonalEdges(const int* neighbours,
    const int* order, int northwestKind, int southeastKind)
{
    return neighbours[order[TILE_DIR_NORTHWEST]] == northwestKind
        && neighbours[order[TILE_DIR_SOUTHEAST]] == southeastKind;
}

static inline bool hasEastSouthwestTerrainEdges(const int* neighbours,
    const int* order, int eastKind, int southwestKind)
{
    return neighbours[order[TILE_DIR_EAST]] == eastKind
        && neighbours[order[TILE_DIR_SOUTHWEST]] == southwestKind;
}

static inline bool hasSouthNortheastTerrainEdges(const int* neighbours,
    const int* order, int southKind, int northeastKind)
{
    return neighbours[order[TILE_DIR_SOUTH]] == southKind
        && neighbours[order[TILE_DIR_NORTHEAST]] == northeastKind;
}

// Priority-ordered terrain pattern classification under four reflections.
// Each pass tries one pattern family over all reflections, which decides
// which overlapping pattern wins.
VA(0x005B3E80, 0x75F)
MAC_ADDRESS(0x2552e8, 0x980)
int __fastcall selectTerrainTransition(
    const int* neighbours, TRmgTerrainFlip* flip)
{
    DATA_COMPGEN_GUARD(0x006a52a1, terrainFlipsGuard, flips)

    VA_COMPGEN(0x005b45e0, 0x1, STATIC_DTOR, flips)
    DATA(0x006A52B8)
    static TRmgTerrainFlip flips[4] = {
        makeTerrainFlip(false, false), makeTerrainFlip(false, true),
        makeTerrainFlip(true, false), makeTerrainFlip(true, true)
    };
    const u32 reflectionCount = sizeof(flips) / sizeof(flips[0]);
    u32 reflection;
    for (reflection = 0; reflection < reflectionCount; ++reflection) {
        const int* order = getReflectedTerrainNeighbourOrder(flips[reflection]);
        if (hasSoutheastTerrainCorner(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_BLEND_EDGE)) {
            if (neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE &&
                neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[reflection];
                return SHAPE_E_S_BLEND_NE_SW_HARD;
            }
            if (neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[reflection];
                return SHAPE_E_S_BLEND_SE_HARD;
            }
        }
    }
    for (reflection = 0; reflection < reflectionCount; ++reflection) {
        const int* order = getReflectedTerrainNeighbourOrder(flips[reflection]);
        if (hasNorthwestTerrainCorner(neighbours, order, RMG_NEIGHBOUR_BLEND_EDGE)) {
            if (neighbours[order[TILE_DIR_SOUTHEAST]] != RMG_NEIGHBOUR_NO_EDGE) {
                *flip = flips[reflection];
                return neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE
                    ? SHAPE_N_W_SE_BLEND : SHAPE_N_W_BLEND_SE_HARD;
            }
        } else if (hasNorthwestTerrainCorner(neighbours, order, RMG_NEIGHBOUR_HARD_EDGE)) {
            if (neighbours[order[TILE_DIR_SOUTHEAST]] != RMG_NEIGHBOUR_NO_EDGE) {
                *flip = flips[reflection];
                return neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE
                    ? SHAPE_N_W_SE_HARD : SHAPE_N_W_HARD_SE_BLEND;
            }
        }
    }
    for (reflection = 0; reflection < reflectionCount; ++reflection) {
        const int* order = getReflectedTerrainNeighbourOrder(flips[reflection]);
        if (hasSoutheastTerrainCorner(neighbours, order,
                RMG_NEIGHBOUR_HARD_EDGE, RMG_NEIGHBOUR_BLEND_EDGE)) {
            if (neighbours[order[TILE_DIR_SOUTHWEST]] != RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[reflection];
                return SHAPE_E_HARD_SW_BLEND;
            } else {
                *flip = makeTerrainFlip(!flips[reflection].m_flipX, !flips[reflection].m_flipY);
                return SHAPE_N_W_HARD;
            }
        }
        if (hasSoutheastTerrainCorner(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_HARD_EDGE)) {
            if (neighbours[order[TILE_DIR_NORTHEAST]] != RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[reflection];
                return SHAPE_S_HARD_NE_BLEND;
            } else {
                *flip = makeTerrainFlip(!flips[reflection].m_flipX, !flips[reflection].m_flipY);
                return SHAPE_N_W_HARD;
            }
        }
    }
    for (reflection = 0; reflection < reflectionCount; ++reflection) {
        const int* order = getReflectedTerrainNeighbourOrder(flips[reflection]);
        if (hasSoutheastTerrainCorner(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            if (neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_HARD_EDGE)
                return SHAPE_E_BLEND_SW_HARD;
            if (neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE)
                return SHAPE_S_BLEND_NE_HARD;
        }
    }
    for (reflection = 0; reflection < reflectionCount; ++reflection) {
        const int* order = getReflectedTerrainNeighbourOrder(flips[reflection]);
        if (hasNorthwestTerrainCorner(neighbours, order, RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            return SHAPE_N_W_BLEND;
        }
        if (hasNorthwestTerrainCorner(neighbours, order, RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[reflection];
            return SHAPE_N_W_HARD;
        }
    }
    for (reflection = 0; reflection < reflectionCount; ++reflection) {
        const int* order = getReflectedTerrainNeighbourOrder(flips[reflection]);
        if (hasEastSouthwestTerrainEdges(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[reflection];
            return SHAPE_E_BLEND_SW_HARD;
        }
        if (hasSouthNortheastTerrainEdges(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[reflection];
            return SHAPE_S_BLEND_NE_HARD;
        }
        if (hasEastSouthwestTerrainEdges(neighbours, order,
                RMG_NEIGHBOUR_HARD_EDGE, RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            return SHAPE_E_HARD_SW_BLEND;
        }
        if (hasSouthNortheastTerrainEdges(neighbours, order,
                RMG_NEIGHBOUR_HARD_EDGE, RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            return SHAPE_S_HARD_NE_BLEND;
        }
        if (hasOffsetNorthwestTerrainCorner(neighbours, order, RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            return SHAPE_N_W_BLEND;
        }
        if (hasOffsetNorthwestTerrainCorner(neighbours, order, RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[reflection];
            return SHAPE_N_W_HARD;
        }
    }
    for (reflection = 0; reflection < reflectionCount; ++reflection) {
        const int* order = getReflectedTerrainNeighbourOrder(flips[reflection]);
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return SHAPE_E_BLEND_SE_HARD;
        }
        if (neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return SHAPE_S_BLEND_SE_HARD;
        }
    }
    for (reflection = 0; reflection < reflectionCount; ++reflection) {
        const int* order = getReflectedTerrainNeighbourOrder(flips[reflection]);
        if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[reflection];
            return SHAPE_N_BLEND;
        }
        if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return SHAPE_N_HARD;
        }
        if (neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[reflection];
            return SHAPE_W_BLEND;
        }
        if (neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return SHAPE_W_HARD;
        }
    }
    for (reflection = 0; reflection < reflectionCount; ++reflection) {
        const int* order = getReflectedTerrainNeighbourOrder(flips[reflection]);
        if (hasOppositeTerrainDiagonalEdges(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            return SHAPE_NW_SE_BLEND;
        }
        if (hasOppositeTerrainDiagonalEdges(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[reflection];
            return SHAPE_NW_BLEND_SE_HARD;
        }
        if (hasOppositeTerrainDiagonalEdges(neighbours, order,
                RMG_NEIGHBOUR_HARD_EDGE, RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[reflection];
            return SHAPE_NW_SE_HARD;
        }
    }
    for (reflection = 0; reflection < reflectionCount; ++reflection) {
        const int* order = getReflectedTerrainNeighbourOrder(flips[reflection]);
        if (neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[reflection];
            return SHAPE_SE_BLEND;
        }
        if (neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return SHAPE_SE_HARD;
        }
    }
    *flip = makeTerrainFlip(false, false);
    return SHAPE_FILL;
}

VA(0x005B45F0, 0x26D)
MAC_ADDRESS(0x255c68, 0xbc)
rmgTerrainPainter::rmgTerrainPainter(
    TRmgMapInterface* newAdapter, int terrain, int strength)
    : m_adapter(newAdapter), m_paintTerrain(terrain), m_specialFrameStrength(strength)
{
#if defined(HOMM3_TARGET_MAC)
    m_size = m_adapter->getSize();
#else
    m_size = m_adapter->getSize(TRmgGridPoint());
#endif
    m_packedCells.resize(getWidth() * getHeight(), TRmgPackedTerrainCell());
}

VA(0x005B48D0, 0x8D)
TRmgPackedTerrainCell* rmgTerrainPainter::getPackedCell(
    const TRmgGridPoint& point)
{
    u32 index = point.m_y * m_size.m_x + point.m_x;
    if (!m_packedCells[index].m_initialized)
        initializePackedCell(point, index);
    return &m_packedCells[index];
}

int rmgTerrainPainter::getTerrain(const TRmgGridPoint& point)
{
    return getPackedCell(point)->getTerrain();
}

int rmgTerrainPainter::getFrame(const TRmgGridPoint& point)
{
    return getPackedCell(point)->getFrame();
}

u32 rmgTerrainPainter::getWidth() const
{
    return m_size.m_x;
}

u32 rmgTerrainPainter::getHeight() const
{
    return m_size.m_y;
}

MAC_ADDRESS(0x259998, 0x60)
int rmgTerrainPainter::selectBaseFrame(
    const TRmgGridPoint& point, int terrain, int oldFrame)
{
    int strength = getSpecialFrameStrength(point, terrain);
    return g_rmgTerrainRules[terrain]->selectBaseFrame(strength, oldFrame);
}

// Writes through to the map and keeps the cached cell in sync.
MAC_ADDRESS(0x2550f0, 0xc4)
void rmgTerrainPainter::setTile(
    const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    m_adapter->setTile(point, tile);
    TRmgPackedTerrainCell& packed = m_packedCells[point.m_y * m_size.m_x + point.m_x];
    packed.setInitialized();
    packed.setTileValues(tile);
}

void rmgTerrainPainter::paintBaseTile(const TRmgGridPoint& point)
{
    int frame = selectBaseFrame(point, m_paintTerrain, -1);
    rmgTerrainTile tile(m_paintTerrain, frame);
    setTile(point, tile);
}

const int& rmgTerrainPainter::getPaintTerrain() const
{
    return m_paintTerrain;
}

b8 rmgTerrainPainter::isPaintTerrain(const TRmgGridPoint& point)
{
    return getTerrain(point) == getPaintTerrain();
}

VA(0x005B4960, 0x1B2)
MAC_ADDRESS(0x255ef0, 0x11c)
void rmgTerrainPainter::paintRectangle(
    u32 x, u32 y,
    u32 rectangleWidth, u32 rectangleHeight)
{
    u32 endX = x + rectangleWidth;
    u32 endY = y + rectangleHeight;
    TRmgGridPoint point;
    for (point.setY(y); point.getY() < endY; point.setY(point.getY() + 1)) {
        for (point.setX(x); point.getX() < endX; point.setX(point.getX() + 1)) {
            if (isPaintTerrain(point))
                paintBaseTile(point);
            else
                paintPoint(point);
        }
    }
}

enum TRmgTerrainGapAxis {
    RMG_HORIZONTAL_GAP,
    RMG_VERTICAL_GAP
};

// Painting beside a queued paint-terrain cell closes its gap on that axis.
// Without a perpendicular gap it no longer needs primary repair; dequeue it
// and queue its other-terrain neighbours instead.
static inline void resolveQueuedTerrainGap(rmgTerrainPainter& painter,
    const TRmgGridPoint& painted, int offsetX, int offsetY,
    TRmgTerrainGapAxis closedAxis)
{
    TRmgGridPoint point(painted.getX() + offsetX, painted.getY() + offsetY);
    if (painter.m_primaryPoints.find(point) == painter.m_primaryPoints.end())
        return;
    b8 remainsGap = closedAxis == RMG_VERTICAL_GAP
        ? painter.isHorizontalGap(point) : painter.isVerticalGap(point);
    if (!remainsGap) {
        painter.m_primaryPoints.erase(point);
        painter.queueOtherTerrainNeighbours(point);
    }
}

VA(0x005B4B20, 0x5CB)
MAC_ADDRESS(0x256014, 0x580)
void rmgTerrainPainter::paintPoint(const TRmgGridPoint& point)
{
    paintBaseTile(point);

    if (m_secondaryPoints.find(point) != m_secondaryPoints.end())
        m_secondaryPoints.erase(point);

    if (g_rmgTerrainRules[m_paintTerrain]->m_allowsSeparatedNeighbours) {
        if (point.m_y > 0)
            resolveQueuedTerrainGap(*this, point, 0, -1, RMG_VERTICAL_GAP);
        if (point.m_y < m_size.m_y - 1)
            resolveQueuedTerrainGap(*this, point, 0, 1, RMG_VERTICAL_GAP);
        if (point.m_x > 0)
            resolveQueuedTerrainGap(*this, point, -1, 0, RMG_HORIZONTAL_GAP);
        if (point.m_x < m_size.m_x - 1)
            resolveQueuedTerrainGap(*this, point, 1, 0, RMG_HORIZONTAL_GAP);
    } else {
        b8 neighbourExists[TILE_DIR_COUNT];
        buildTileNeighbourMask(
            m_size.m_x, m_size.m_y, point.m_x, point.m_y, neighbourExists);
        for (u32 direction = 0; direction < TILE_DIR_COUNT; ++direction) {
            if (neighbourExists[direction]) {
                const TPoint& offset = g_tileDirections[direction];
                TRmgGridPoint nearby(point + offset);
                if (isPaintTerrain(nearby)) {
                    if (m_primaryPoints.find(nearby) != m_primaryPoints.end()) {
                        if (!needsTerrainRepair(nearby)) {
                            m_primaryPoints.erase(nearby);
                            queueOtherTerrainNeighbours(nearby);
                        }
                    } else if (needsTerrainRepair(nearby)) {
                        m_primaryPoints.insert(nearby);
                    }
                }
            }
        }
    }
    if (needsTerrainRepair(point))
        m_primaryPoints.insert(point);
    else
        queueOtherTerrainNeighbours(point);
}

// Other-terrain diagonal neighbours are queued only when their own terrain
// requires connected neighbours.
static inline void queueOtherTerrainDiagonalNeighbour(rmgTerrainPainter& painter,
    const TRmgGridPoint& point, int offsetX, int offsetY)
{
    TRmgGridPoint neighbour(point.getX() + offsetX, point.getY() + offsetY);
    int terrain = painter.getTerrain(neighbour);
    if (terrain != painter.m_paintTerrain
        && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
        painter.m_secondaryPoints.insert(neighbour);
}

// Queues one cardinal neighbour of other terrain; returns whether it did.
static inline bool tryQueueOtherTerrainCardinalNeighbour(rmgTerrainPainter& painter,
    const TRmgGridPoint& point, int offsetX, int offsetY)
{
    TRmgGridPoint neighbour(point.getX() + offsetX, point.getY() + offsetY);
    if (painter.isPaintTerrain(neighbour))
        return false;
    painter.m_secondaryPoints.insert(neighbour);
    return true;
}

VA(0x005B50F0, 0x34E)
MAC_ADDRESS(0x2565ac, 0x638)
void rmgTerrainPainter::queueOtherTerrainNeighbours(const TRmgGridPoint& point)
{
    // At most one neighbour per cardinal axis, preferring north over south
    // and west over east.
    bool queuedNorth = point.getY() > 0
        && tryQueueOtherTerrainCardinalNeighbour(*this, point, 0, -1);
    if (!queuedNorth && point.getY() < getHeight() - 1)
        tryQueueOtherTerrainCardinalNeighbour(*this, point, 0, 1);
    bool queuedWest = point.getX() > 0
        && tryQueueOtherTerrainCardinalNeighbour(*this, point, -1, 0);
    if (!queuedWest && point.getX() < getWidth() - 1)
        tryQueueOtherTerrainCardinalNeighbour(*this, point, 1, 0);
    if (point.getX() > 0 && point.getY() > 0)
        queueOtherTerrainDiagonalNeighbour(*this, point, -1, -1);
    if (point.getX() < getWidth() - 1 && point.getY() > 0)
        queueOtherTerrainDiagonalNeighbour(*this, point, 1, -1);
    if (point.getX() > 0 && point.getY() < getHeight() - 1)
        queueOtherTerrainDiagonalNeighbour(*this, point, -1, 1);
    if (point.getX() < getWidth() - 1 && point.getY() < getHeight() - 1)
        queueOtherTerrainDiagonalNeighbour(*this, point, 1, 1);
}

b8 rmgTerrainPainter::isHorizontalGap(const TRmgGridPoint& point)
{
    return isHorizontalGap(point, getTerrain(point));
}

b8 rmgTerrainPainter::isVerticalGap(const TRmgGridPoint& point)
{
    return isVerticalGap(point, getTerrain(point));
}

MAC_ADDRESS(0x2588c4, 0x1a0)
b8 rmgTerrainPainter::needsTerrainRepair(const TRmgGridPoint& point)
{
    if (isHorizontalGap(point) || isVerticalGap(point))
        return true;
    return !g_rmgTerrainRules[getTerrain(point)]->m_allowsSeparatedNeighbours
        && hasSeparatedNeighbours(point);
}

// Fill one side of an axis gap: the negative side, unless it needs no repair
// and either the positive side does or only the negative side has a
// perpendicular gap in the paint terrain.
static inline void repairTerrainGap(rmgTerrainPainter& painter,
    const TRmgGridPoint& negative, const TRmgGridPoint& positive,
    TRmgTerrainGapAxis axis)
{
    bool fillPositive = false;
    if (!painter.needsTerrainRepair(negative)) {
        if (painter.needsTerrainRepair(positive)) {
            fillPositive = true;
        } else if (axis == RMG_VERTICAL_GAP) {
            fillPositive = painter.isHorizontalGap(negative, painter.getPaintTerrain())
                && !painter.isHorizontalGap(positive, painter.getPaintTerrain());
        } else {
            fillPositive = painter.isVerticalGap(negative, painter.getPaintTerrain())
                && !painter.isVerticalGap(positive, painter.getPaintTerrain());
        }
    }
    painter.paintPoint(fillPositive ? positive : negative);
}

// Repair one-cell gaps, then greedily fill the lightest neighbour-ring gaps
// until only one remains; ties go to the first gap. Painting order matters
// because each repair updates the worklists.
VA(0x005B5440, 0x628)
MAC_ADDRESS(0x256be4, 0x630)
void rmgTerrainPainter::repairTerrainPoint(const TRmgGridPoint& point)
{
    if (isVerticalGap(point)) {
        repairTerrainGap(*this, TRmgGridPoint(point.m_x, point.m_y - 1),
            TRmgGridPoint(point.m_x, point.m_y + 1), RMG_VERTICAL_GAP);
    }
    if (isHorizontalGap(point)) {
        repairTerrainGap(*this, TRmgGridPoint(point.m_x - 1, point.m_y),
            TRmgGridPoint(point.m_x + 1, point.m_y), RMG_HORIZONTAL_GAP);
    }

    if (!g_rmgTerrainRules[getPaintTerrain()]->m_allowsSeparatedNeighbours &&
        hasSeparatedNeighbours(point)) {
        b8 matches[TILE_DIR_COUNT];
        buildMatchingNeighbourMask(point, matches);
        TRmgTerrainGap gaps[TILE_DIR_COUNT / 2];
        // Run-length encode the nonmatching ring, starting after a known
        // match so a wrapped gap stays together. Separated neighbours imply
        // a match exists and at most four gaps.
        u32 firstMatch = 0;
        while (!matches[firstMatch])
            ++firstMatch;
        u32 gapCount = 0;
        u32 direction = (firstMatch + 1) % TILE_DIR_COUNT;
        while (direction != firstMatch) {
            if (matches[direction]) {
                direction = (direction + 1) % TILE_DIR_COUNT;
                continue;
            }
            TRmgTerrainGap& gap = gaps[gapCount++];
            gap.m_weight = 0;
            gap.m_start = direction;
            gap.m_length = 0;
            do {
                gap.m_weight += isRmgDiagonalDirection(direction) ? 1 : 2;
                ++gap.m_length;
                direction = (direction + 1) % TILE_DIR_COUNT;
            } while (direction != firstMatch && !matches[direction]);
        }
        b8 neighbourExists[TILE_DIR_COUNT];
        buildTileNeighbourMask(getWidth(), getHeight(), point.m_x, point.m_y,
                               neighbourExists);
        do {
            u32 smallest = 0;
            u32 smallestWeight = gaps[0].m_weight;
            for (u32 gap = 1; gap < gapCount; ++gap) {
                if (gaps[gap].m_weight < smallestWeight) {
                    smallest = gap;
                    smallestWeight = gaps[gap].m_weight;
                }
            }
            u32 end =
                (gaps[smallest].m_start + gaps[smallest].m_length) % TILE_DIR_COUNT;
            for (u32 direction = gaps[smallest].m_start; direction != end;
                 direction = (direction + 1) % TILE_DIR_COUNT) {
                if (neighbourExists[direction])
                    paintPoint(point + g_tileDirections[direction]);
            }
            --gapCount;
            for (u32 remainingGap = smallest; remainingGap < gapCount; ++remainingGap)
                gaps[remainingGap] = gaps[remainingGap + 1];
        } while (gapCount > 1);
    }
}

// Count the terrain boundary toward one neighbour, adding it to both cells.
static inline void countTerrainBoundary(
    rmgTerrainPainter& painter, const TRmgGridPoint& point, u32 direction,
    int terrain, std::vector<u8>& edgeCounts)
{
    TRmgGridPoint neighbour(point + g_tileDirections[direction]);
    if (painter.getTerrain(neighbour) != terrain) {
        ++edgeCounts[point.getY() * painter.getWidth() + point.getX()];
        ++edgeCounts[neighbour.getY() * painter.getWidth() + neighbour.getX()];
    }
}

VA(0x005B5A70, 0x8A7)
MAC_ADDRESS(0x257214, 0xd54)
void rmgTerrainPainter::paintTransitions()
{
    // Assumes the map has at least two rows and columns.
    std::vector<u8> edgeCounts(getWidth() * getHeight());
    TRmgGridPoint point;

    // Each cell counts its boundaries toward the neighbours that follow it
    // in scan order (E, SE, S and SW), so every boundary is counted once.
    for (point.setY(0); point.m_y < getHeight(); point.setY(point.getY() + 1)) {
        for (point.setX(0); point.m_x < getWidth(); point.setX(point.getX() + 1)) {
            int terrain = getTerrain(point);
            b8 neighbourExists[TILE_DIR_COUNT];
            buildTileNeighbourMask(getWidth(), getHeight(), point.m_x, point.m_y,
                                   neighbourExists);
            for (u32 direction = TILE_DIR_EAST; direction <= TILE_DIR_SOUTHWEST; ++direction) {
                if (neighbourExists[direction])
                    countTerrainBoundary(*this, point, direction, terrain, edgeCounts);
            }
        }
    }

    for (point.setY(0); point.m_y < getHeight(); point.setY(point.getY() + 1)) {
        for (point.setX(0); point.m_x < getWidth(); point.setX(point.getX() + 1)) {
            u32 index = point.getY() * getWidth() + point.getX();

            // A cell without a differing neighbour is drawn as shape 0, unflipped.
            int transition = SHAPE_FILL;
            TRmgTerrainFlip flip(false, false);
            if (edgeCounts[index] > 0) {
                int neighbours[TILE_DIR_COUNT];
                buildNeighbourKinds(point, neighbours);
                transition = selectTerrainTransition(neighbours, &flip);
                if (transition == SHAPE_N_W_BLEND) {
                    if (isOuterCornerOnDiagonalEdge(point, flip))
                        transition = SHAPE_N_W_DIAG_BLEND;
                } else if (transition == SHAPE_N_W_HARD) {
                    if (isOuterCornerOnDiagonalEdge(point, flip))
                        transition = SHAPE_N_W_DIAG_HARD;
                } else if (transition == SHAPE_SE_BLEND) {
                    if (isInnerCornerOnDiagonalEdge(point, flip))
                        transition = SHAPE_SE_DIAG_BLEND;
                } else if (transition == SHAPE_SE_HARD) {
                    if (isInnerCornerOnDiagonalEdge(point, flip))
                        transition = SHAPE_SE_DIAG_HARD;
                }
            }

            rmgTerrainTile tile = getPackedCell(point)->getTile();
            int newFrame;
            if (transition != SHAPE_FILL) {
                newFrame = g_rmgTerrainRules[tile.m_terrain]
                    ->selectTransitionFrame(transition, flip, flip, tile.m_frame);
            } else {
                newFrame = selectBaseFrame(point, tile.m_terrain, tile.m_frame);
            }

            // Write the tile back only when its frame or a reflection changes.
            if (tile.m_frame != newFrame || tile.m_flipX != flip.m_flipX
                || tile.m_flipY != flip.m_flipY) {
                tile.m_frame = newFrame;
                tile.m_flipX = flip.m_flipX;
                tile.m_flipY = flip.m_flipY;
                setTile(point, tile);
            }
        }
    }
}

VA(0x005B6320, 0x107)
MAC_ADDRESS(0x25803c, 0x160)
b8 rmgTerrainPainter::isHorizontalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.m_x > 0 && point.m_x < getWidth() - 1
        && getTerrain(TRmgGridPoint(point.getX() - 1, point.getY())) != terrain
        && getTerrain(TRmgGridPoint(point.getX() + 1, point.getY())) != terrain;
}

VA(0x005B6430, 0x106)
MAC_ADDRESS(0x25819c, 0x160)
b8 rmgTerrainPainter::isVerticalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.m_y > 0 && point.m_y < getHeight() - 1
        && getTerrain(TRmgGridPoint(point.getX(), point.getY() - 1)) != terrain
        && getTerrain(TRmgGridPoint(point.getX(), point.getY() + 1)) != terrain;
}

// The 3x3 neighbourhood window, clamped to the map edge.
static inline void getTerrainNeighbourBounds(const TRmgGridPoint& point,
    const TRmgGridPoint& size, TRmgGridPoint& northWest, TRmgGridPoint& southEast)
{
    u32 north = point.m_y > 0 ? point.m_y - 1 : point.m_y;
    u32 south = point.m_y < size.m_y - 1 ? point.m_y + 1 : point.m_y;
    u32 west = point.m_x > 0 ? point.m_x - 1 : point.m_x;
    u32 east = point.m_x < size.m_x - 1 ? point.m_x + 1 : point.m_x;
    northWest = TRmgGridPoint(west, north);
    southEast = TRmgGridPoint(east, south);
}

static inline bool matchesTerrainAt(rmgTerrainPainter& painter,
    u32 x, u32 y, int terrain)
{
    TRmgGridPoint nearby;
    nearby.setX(x);
    nearby.setY(y);
    return painter.getTerrain(nearby) == terrain;
}

// A diagonal neighbour matches only beside a matching cardinal neighbour.
static inline bool matchesTerrainCorner(rmgTerrainPainter& painter,
    b8 firstSide, b8 secondSide, u32 x, u32 y, int terrain)
{
    return (firstSide || secondSide) && matchesTerrainAt(painter, x, y, terrain);
}

// Neighbour coordinates are clamped to the map edge.
VA(0x005B6540, 0x2CA)
MAC_ADDRESS(0x2582fc, 0x4f8)
void rmgTerrainPainter::buildMatchingNeighbourMask(
    const TRmgGridPoint& point, b8* matches)
{
    int terrain = getTerrain(point);
    TRmgGridPoint northWest;
    TRmgGridPoint southEast;
    getTerrainNeighbourBounds(point, m_size, northWest, southEast);

    matches[TILE_DIR_NORTH] = matchesTerrainAt(*this, point.m_x, northWest.getY(), terrain);
    matches[TILE_DIR_SOUTH] = matchesTerrainAt(*this, point.m_x, southEast.getY(), terrain);
    matches[TILE_DIR_WEST] = matchesTerrainAt(*this, northWest.getX(), point.m_y, terrain);
    matches[TILE_DIR_EAST] = matchesTerrainAt(*this, southEast.getX(), point.m_y, terrain);
    matches[TILE_DIR_NORTHWEST] = matchesTerrainCorner(*this, matches[TILE_DIR_NORTH],
        matches[TILE_DIR_WEST], northWest.getX(), northWest.getY(), terrain);
    matches[TILE_DIR_NORTHEAST] = matchesTerrainCorner(*this, matches[TILE_DIR_NORTH],
        matches[TILE_DIR_EAST], southEast.getX(), northWest.getY(), terrain);
    matches[TILE_DIR_SOUTHWEST] = matchesTerrainCorner(*this, matches[TILE_DIR_SOUTH],
        matches[TILE_DIR_WEST], northWest.getX(), southEast.getY(), terrain);
    matches[TILE_DIR_SOUTHEAST] = matchesTerrainCorner(*this, matches[TILE_DIR_SOUTH],
        matches[TILE_DIR_EAST], southEast.getX(), southEast.getY(), terrain);
}

// Steps round the neighbour ring to the next direction whose match state is
// wanted; false once the scan returns to first.
static inline bool advanceToMatchState(const b8* matches, u32& direction,
    u32 first, b8 wanted)
{
    do {
        direction = (direction + 1) % TILE_DIR_COUNT;
        if (direction == first)
            return false;
    } while (matches[direction] != wanted);
    return true;
}

// Starting in a gap, a matching run, another gap and another match before
// returning to the start prove that the centre's neighbours are separated.
VA(0x005B6810, 0x84)
MAC_ADDRESS(0x2587f4, 0xd0)
b8 rmgTerrainPainter::hasSeparatedNeighbours(const TRmgGridPoint& point)
{
    b8 matches[TILE_DIR_COUNT];
    buildMatchingNeighbourMask(point, matches);
    u32 first = 0;
    while (matches[first]) {
        first = (first + 1) % TILE_DIR_COUNT;
        if (first == 0)
            return false;
    }
    u32 direction = first;
    return advanceToMatchState(matches, direction, first, true)
        && advanceToMatchState(matches, direction, first, false)
        && advanceToMatchState(matches, direction, first, true);
}

static inline int getTerrainNeighbourKindAt(
    rmgTerrainPainter& painter, const TRmgGridPoint& neighbour, int terrain)
{
    int neighbourTerrain = painter.getTerrain(neighbour);
    return getRmgTerrainNeighbourKind(terrain, neighbourTerrain);
}

VA(0x005B68A0, 0x2FF)
MAC_ADDRESS(0x258aa0, 0x478)
void rmgTerrainPainter::buildNeighbourKinds(
    const TRmgGridPoint& point, int* neighbours)
{
    int terrain = getTerrain(point);
    TRmgGridPoint northWest;
    TRmgGridPoint southEast;
    getTerrainNeighbourBounds(point, m_size, northWest, southEast);

    neighbours[TILE_DIR_NORTH] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(point.m_x, northWest.getY()), terrain);
    neighbours[TILE_DIR_SOUTH] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(point.m_x, southEast.getY()), terrain);
    neighbours[TILE_DIR_WEST] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(northWest.getX(), point.m_y), terrain);
    neighbours[TILE_DIR_EAST] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(southEast.getX(), point.m_y), terrain);
    neighbours[TILE_DIR_NORTHWEST] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(northWest.getX(), northWest.getY()), terrain);
    neighbours[TILE_DIR_NORTHEAST] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(southEast.getX(), northWest.getY()), terrain);
    neighbours[TILE_DIR_SOUTHWEST] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(northWest.getX(), southEast.getY()), terrain);
    neighbours[TILE_DIR_SOUTHEAST] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(southEast.getX(), southEast.getY()), terrain);
}

// Compares terrain at a reflected offset, clamped to the map edge.
static inline bool matchesTerrainAtClampedOffset(rmgTerrainPainter& painter,
    const TRmgGridPoint& point, const TPoint& offset, int terrain)
{
    TRmgGridPoint nearby(
        tLimit(0, static_cast<int>(point.getX()) + offset.getX(),
            static_cast<int>(painter.getWidth()) - 1),
        tLimit(0, static_cast<int>(point.getY()) + offset.getY(),
            static_cast<int>(painter.getHeight()) - 1));
    return painter.getTerrain(nearby) == terrain;
}

// Row of a reflection in the corner offset tables: flipX + 2 * flipY.
static inline int getTerrainFlipIndex(const TRmgTerrainFlip& flip)
{
    return (flip.m_flipY << 1) | flip.m_flipX;
}

// An outer corner lies on a 45-degree edge when either diagonal beside it
// (NE or SW of the canonical NW corner) has the cell's own terrain.
// North is up; C is the cell, e an edge, o a probe, ? not tested.
//   ? e o
//   e C ?
//   o ? ?
VA(0x005B6BA0, 0x24C)
MAC_ADDRESS(0x258f18, 0x360)
b8 rmgTerrainPainter::isOuterCornerOnDiagonalEdge(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    DATA_COMPGEN_GUARD(0x006a52a0, firstDiagonalOffsetsGuard, firstDiagonalOffsets)

    VA_COMPGEN(0x005b6df0, 0x1, STATIC_DTOR, firstDiagonalOffsets)
    DATA(0x006A5260)
    static TPoint firstDiagonalOffsets[4][2] = {
        { TPoint(-1, 1), TPoint(1, -1) },
        { TPoint(1, 1), TPoint(-1, -1) },
        { TPoint(-1, -1), TPoint(1, 1) },
        { TPoint(1, -1), TPoint(-1, 1) }
    };
    int terrain = getTerrain(point);
    const TPoint* pair = firstDiagonalOffsets[getTerrainFlipIndex(flip)];
    if (matchesTerrainAtClampedOffset(*this, point, pair[0], terrain))
        return true;
    return matchesTerrainAtClampedOffset(*this, point, pair[1], terrain);
}

// An inner corner lies on a 45-degree edge when the cell two steps along
// either side (E or S of the canonical SE corner) has another terrain.
// North is up; C is the cell, e its edge, . none, o a probe, ? not tested.
//   C . o
//   . e ?
//   o ? ?
VA(0x005B6E00, 0x1B3)
MAC_ADDRESS(0x259278, 0x288)
b8 rmgTerrainPainter::isInnerCornerOnDiagonalEdge(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    DATA_COMPGEN_GUARD(0x006a3d64, secondDiagonalOffsetsGuard, secondDiagonalOffsets)

    VA_COMPGEN(0x005b6fc0, 0x1, STATIC_DTOR, secondDiagonalOffsets)
    DATA(0x006A3D68)
    static TPoint secondDiagonalOffsets[4] = {
        TPoint(2, 2), TPoint(-2, 2), TPoint(2, -2), TPoint(-2, -2)
    };
    int terrain = getTerrain(point);
    const TPoint& offset = secondDiagonalOffsets[getTerrainFlipIndex(flip)];
    return !matchesTerrainAtClampedOffset(*this, point, TPoint(offset.getX(), 0), terrain)
        || !matchesTerrainAtClampedOffset(*this, point, TPoint(0, offset.getY()), terrain);
}

// Each same-terrain cardinal neighbour showing a special frame halves the
// special-frame strength.
static inline bool hasSpecialTerrainFrameAt(rmgTerrainPainter& painter,
    const TRmgGridPoint& point, int dx, int dy, int terrain, TRmgTerrainRule* rule)
{
    TRmgGridPoint nearby(point.getX() + dx, point.getY() + dy);
    return painter.getTerrain(nearby) == terrain
        && rule->isSpecialFrame(painter.getFrame(nearby));
}

VA(0x005B6FD0, 0x271)
MAC_ADDRESS(0x259500, 0x444)
int rmgTerrainPainter::getSpecialFrameStrength(
    const TRmgGridPoint& point, int terrain)
{
    u32 strength = m_specialFrameStrength;
    TRmgTerrainRule* rule = g_rmgTerrainRules[terrain];
    if (point.getX() > 0 && hasSpecialTerrainFrameAt(*this, point, -1, 0, terrain, rule))
        strength >>= 1;
    if (point.getY() > 0 && hasSpecialTerrainFrameAt(*this, point, 0, -1, terrain, rule))
        strength >>= 1;
    if (point.getX() < getWidth() - 1
        && hasSpecialTerrainFrameAt(*this, point, 1, 0, terrain, rule))
        strength >>= 1;
    if (point.getY() < getHeight() - 1
        && hasSpecialTerrainFrameAt(*this, point, 0, 1, terrain, rule))
        strength >>= 1;
    return strength;
}

// Drains both repair worklists, then paints transitions. Runs on terrain
// change and destruction.
MAC_ADDRESS(0x257f68, 0xd4)
void rmgTerrainPainter::finish()
{
    do {
        while (m_primaryPoints.size()) {
            TRmgGridPoint point = *m_primaryPoints.begin();
            repairTerrainPoint(point);
        }
        while (m_secondaryPoints.size()) {
            TRmgGridPoint point = *m_secondaryPoints.begin();
            m_secondaryPoints.erase(point);
            if (needsTerrainRepair(point))
                paintPoint(point);
        }
    } while (m_primaryPoints.size());
    paintTransitions();
}

// Returns the previous terrain; the brush ignores it.
MAC_ADDRESS(0x255ea4, 0x4c)
int rmgTerrainPainter::changeTerrain(int terrain, int strength)
{
    int previous = m_paintTerrain;
    finish();
    m_paintTerrain = terrain;
    m_specialFrameStrength = strength;
    return previous;
}

VA(0x005B7250, 0x9A)
MAC_ADDRESS(0x2599f8, 0xac)
TRmgTerrainBrush::TRmgTerrainBrush(
    TRmgMapInterface* map, int terrain, int strength)
    : m_painter(new rmgTerrainPainter(map, terrain, strength))
{
    if (!m_painter.get())
        throw TAllocationFailure();
}

VA(0x005B72F0, 0x225)
MAC_ADDRESS(0x259aa4, 0xc8)
TRmgTerrainBrush::~TRmgTerrainBrush()
{
}

VA(0x005B7520, 0x16A)
MAC_ADDRESS(0x259c9c, 0x24)
void TRmgTerrainBrush::changeTerrain(int terrain, int strength)
{
    m_painter->changeTerrain(terrain, strength);
}

VA(0x005B7690, 0x1F)
MAC_ADDRESS(0x259cc0, 0x24)
void TRmgTerrainBrush::paintRectangle(
    u32 x, u32 y,
    u32 rectangleWidth, u32 rectangleHeight)
{
    m_painter->paintRectangle(x, y, rectangleWidth, rectangleHeight);
}

VA_COMPGEN(0x005B76D0, 0x20, IMPLICIT_DTOR, rmgTerrainPainter_auto_ptr)

VA(0x005B76F0, 0x209)
MAC_ADDRESS(0x259be4, 0xb8)
rmgTerrainPainter::~rmgTerrainPainter()
{
    finish();
}

VA_COMPGEN(0x005B7CD0, 0x156, TREE_INSERT, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8670, 0xA8, TREE_INIT, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8720, 0x2FE, TREE_NODE_INSERT, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8AA0, 0xB3, TREE_CONST_ITERATOR_DEC, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B7F60, 0x59, TREE_ERASE_KEY, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B7E30, 0x121, TREE_ERASE_RANGE, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8090, 0x50F, TREE_ERASE_ITERATOR, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B85F0, 0x7E, TREE_ERASE, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8BC0, 0xA3, TREE_CONST_ITERATOR_INC, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B7FC0, 0x57, TREE_FIND, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8020, 0x35, VECTOR_ERASE, TRmgPackedTerrainCell)

VA_COMPGEN(0x005B8060, 0x24, VECTOR_UFILL, unsigned_char)

VA_COMPGEN(0x005B85A0, 0x17, TREE_LOWER_BOUND, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B85C0, 0x2C, TREE_EQUAL_RANGE, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8A20, 0x17, TREE_UPPER_BOUND, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8A40, 0x59, TREE_LBOUND, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8B60, 0x59, TREE_UBOUND, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B4860, 0x6E, IMPLICIT_DTOR, set)

VA_COMPGEN(0x005B8C70, 0x2B, STD_DISTANCE, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8CD0, 0x28, STD_DISTANCE_TAGGED, TRmgCoordinatePoint_unsigned_int)

template<class Coordinate>
// VA instance: operator< <unsigned int>
VA(0x005B8CA0, 0x20)
bool operator<(const TRmgCoordinatePoint<Coordinate>& left,
    const TRmgCoordinatePoint<Coordinate>& right)
{
    return left.getY() < right.getY() || (left.getY() == right.getY() && left.getX() < right.getX());
}

// Terrain frame tables and the per-terrain rules built from them.
DATA(0x006424A8)
const TRmgTerrainTransitionEntry g_rmgTerrainPatterns[RMG_FIXED_TRANSITION_FRAME_COUNT] = {
    {SHAPE_FILL, false, false}, {SHAPE_FILL, false, false}, {SHAPE_FILL, false, false},
    {SHAPE_FILL, false, false}, {SHAPE_FILL, false, false}, {SHAPE_FILL, false, false},
    {SHAPE_FILL, false, false}, {SHAPE_FILL, false, false},
    {SHAPE_N_W_HARD, false, false}, {SHAPE_N_W_HARD, false, false}, {SHAPE_N_W_HARD, true, false},
    {SHAPE_N_W_HARD, true, false}, {SHAPE_N_W_HARD, false, true}, {SHAPE_N_W_HARD, false, true},
    {SHAPE_N_W_HARD, true, true}, {SHAPE_N_W_HARD, true, true},
    {SHAPE_W_HARD, false, false}, {SHAPE_W_HARD, false, false}, {SHAPE_W_HARD, true, false},
    {SHAPE_W_HARD, true, false},
    {SHAPE_N_HARD, false, false}, {SHAPE_N_HARD, false, false}, {SHAPE_N_HARD, false, true},
    {SHAPE_N_HARD, false, true},
    {SHAPE_SE_HARD, false, false}, {SHAPE_SE_HARD, false, false}, {SHAPE_SE_HARD, true, false},
    {SHAPE_SE_HARD, true, false}, {SHAPE_SE_HARD, false, true}, {SHAPE_SE_HARD, false, true},
    {SHAPE_SE_HARD, true, true}, {SHAPE_SE_HARD, true, true},
    {SHAPE_N_W_DIAG_HARD, false, false}, {SHAPE_N_W_DIAG_HARD, false, false},
    {SHAPE_N_W_DIAG_HARD, true, false}, {SHAPE_N_W_DIAG_HARD, true, false},
    {SHAPE_N_W_DIAG_HARD, false, true}, {SHAPE_N_W_DIAG_HARD, false, true},
    {SHAPE_N_W_DIAG_HARD, true, true}, {SHAPE_N_W_DIAG_HARD, true, true},
    {SHAPE_SE_DIAG_HARD, false, false}, {SHAPE_SE_DIAG_HARD, false, false},
    {SHAPE_SE_DIAG_HARD, true, false}, {SHAPE_SE_DIAG_HARD, true, false},
    {SHAPE_SE_DIAG_HARD, false, true}, {SHAPE_SE_DIAG_HARD, false, true},
    {SHAPE_SE_DIAG_HARD, true, true}, {SHAPE_SE_DIAG_HARD, true, true},
};

DATA(0x00642628)
static const TRmgTerrainPatternEntry g_rmgLandPatternEntries[79] = {
    {SHAPE_N_W_BLEND, false}, {SHAPE_N_W_BLEND, false}, {SHAPE_N_W_BLEND, false},
    {SHAPE_N_W_BLEND, false},
    {SHAPE_W_BLEND, false}, {SHAPE_W_BLEND, false}, {SHAPE_W_BLEND, false}, {SHAPE_W_BLEND, false},
    {SHAPE_N_BLEND, false}, {SHAPE_N_BLEND, false}, {SHAPE_N_BLEND, false}, {SHAPE_N_BLEND, false},
    {SHAPE_SE_BLEND, false}, {SHAPE_SE_BLEND, false}, {SHAPE_SE_BLEND, false},
    {SHAPE_SE_BLEND, false}, {SHAPE_N_W_DIAG_BLEND, false}, {SHAPE_N_W_DIAG_BLEND, false},
    {SHAPE_SE_DIAG_BLEND, false}, {SHAPE_SE_DIAG_BLEND, false},
    {SHAPE_N_W_HARD, false}, {SHAPE_N_W_HARD, false}, {SHAPE_N_W_HARD, false},
    {SHAPE_N_W_HARD, false},
    {SHAPE_W_HARD, false}, {SHAPE_W_HARD, false}, {SHAPE_W_HARD, false}, {SHAPE_W_HARD, false},
    {SHAPE_N_HARD, false}, {SHAPE_N_HARD, false}, {SHAPE_N_HARD, false}, {SHAPE_N_HARD, false},
    {SHAPE_SE_HARD, false}, {SHAPE_SE_HARD, false}, {SHAPE_SE_HARD, false}, {SHAPE_SE_HARD, false},
    {SHAPE_N_W_DIAG_HARD, false}, {SHAPE_N_W_DIAG_HARD, false},
    {SHAPE_SE_DIAG_HARD, false}, {SHAPE_SE_DIAG_HARD, false}, {SHAPE_NW_SE_BLEND, false},
    {SHAPE_NW_BLEND_SE_HARD, false}, {SHAPE_NW_SE_HARD, false}, {SHAPE_E_BLEND_SW_HARD, false},
    {SHAPE_S_BLEND_NE_HARD, false}, {SHAPE_E_BLEND_SE_HARD, false}, {SHAPE_S_BLEND_SE_HARD, false},
    {SHAPE_E_HARD_SW_BLEND, false}, {SHAPE_S_HARD_NE_BLEND, false},
    {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false},
    {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_N_W_SE_BLEND, false}, {SHAPE_N_W_SE_HARD, false}, {SHAPE_N_W_BLEND_SE_HARD, false},
    {SHAPE_N_W_HARD_SE_BLEND, false}, {SHAPE_E_S_BLEND_NE_SW_HARD, false},
    {SHAPE_E_S_BLEND_SE_HARD, false},
};

DATA(0x006428A0)
static const TRmgTerrainPatternEntry g_rmgDirtPatternEntries[46] = {
    {SHAPE_N_W_HARD, false}, {SHAPE_N_W_HARD, false}, {SHAPE_N_W_HARD, false},
    {SHAPE_N_W_HARD, false},
    {SHAPE_W_HARD, false}, {SHAPE_W_HARD, false}, {SHAPE_W_HARD, false}, {SHAPE_W_HARD, false},
    {SHAPE_N_HARD, false}, {SHAPE_N_HARD, false}, {SHAPE_N_HARD, false}, {SHAPE_N_HARD, false},
    {SHAPE_SE_HARD, false}, {SHAPE_SE_HARD, false}, {SHAPE_SE_HARD, false}, {SHAPE_SE_HARD, false},
    {SHAPE_N_W_DIAG_HARD, false}, {SHAPE_N_W_DIAG_HARD, false},
    {SHAPE_SE_DIAG_HARD, false}, {SHAPE_SE_DIAG_HARD, false}, {SHAPE_NW_SE_HARD, false},
    {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false},
    {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_N_W_SE_HARD, false},
};

DATA(0x00642A10)
static const TRmgTerrainPatternEntry g_rmgSandPatternEntries[24] = {
    {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false},
    {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
};

DATA(0x00642AD0)
static const TRmgTerrainPatternEntry g_rmgWaterPatternEntries[33] = {
    {SHAPE_N_W_HARD, false}, {SHAPE_N_W_HARD, false}, {SHAPE_N_W_HARD, false},
    {SHAPE_N_W_HARD, false},
    {SHAPE_W_HARD, false}, {SHAPE_W_HARD, false}, {SHAPE_W_HARD, false}, {SHAPE_W_HARD, false},
    {SHAPE_N_HARD, false}, {SHAPE_N_HARD, false}, {SHAPE_N_HARD, false}, {SHAPE_N_HARD, false},
    {SHAPE_SE_HARD, false}, {SHAPE_SE_HARD, false}, {SHAPE_SE_HARD, false}, {SHAPE_SE_HARD, false},
    {SHAPE_N_W_DIAG_HARD, false}, {SHAPE_N_W_DIAG_HARD, false},
    {SHAPE_SE_DIAG_HARD, false}, {SHAPE_SE_DIAG_HARD, false}, {SHAPE_NW_SE_HARD, false},
    {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false},
    {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false},
    {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false},
};

DATA(0x006A48D0)
static TRmgPatternTerrainRule g_rmgDirtRule(true, true, 50, 46, g_rmgDirtPatternEntries);

VA_COMPGEN(0x005B3B60, 0x23, STATIC_CTOR, g_rmgDirtRule)

VA_COMPGEN(0x005B3B90, 0x0A, STATIC_DTOR, g_rmgDirtRule)
DATA(0x006A44F8)
static TRmgPatternTerrainRule g_rmgSandRule(false, true, 70, 24, g_rmgSandPatternEntries);

VA_COMPGEN(0x005B3BA0, 0x23, STATIC_CTOR, g_rmgSandRule)

VA_COMPGEN(0x005B3BD0, 0x0A, STATIC_DTOR, g_rmgSandRule)
DATA(0x006A3D88)
static TRmgPatternTerrainRule g_rmgGrassRule(true, true, 50, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005B3BE0, 0x23, STATIC_CTOR, g_rmgGrassRule)

VA_COMPGEN(0x005B3C10, 0x0A, STATIC_DTOR, g_rmgGrassRule)
DATA(0x006A3F70)
static TRmgPatternTerrainRule g_rmgSnowRule(true, true, 80, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005B3C20, 0x23, STATIC_CTOR, g_rmgSnowRule)

VA_COMPGEN(0x005B3C50, 0x0A, STATIC_DTOR, g_rmgSnowRule)
DATA(0x006A46E0)
static TRmgPatternTerrainRule g_rmgSwampRule(true, true, 80, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005B3C60, 0x23, STATIC_CTOR, g_rmgSwampRule)

VA_COMPGEN(0x005B3C90, 0x0A, STATIC_DTOR, g_rmgSwampRule)
DATA(0x006A4AB8)
static TRmgPatternTerrainRule g_rmgRoughRule(true, true, 80, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005B3CA0, 0x23, STATIC_CTOR, g_rmgRoughRule)

VA_COMPGEN(0x005B3CD0, 0x0A, STATIC_DTOR, g_rmgRoughRule)
DATA(0x006A5070)
static TRmgPatternTerrainRule g_rmgSubterraneanRule(true, true, 60, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005B3CE0, 0x23, STATIC_CTOR, g_rmgSubterraneanRule)

VA_COMPGEN(0x005B3D10, 0x0A, STATIC_DTOR, g_rmgSubterraneanRule)
DATA(0x006A4E88)
static TRmgPatternTerrainRule g_rmgLavaRule(true, true, 80, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005B3D20, 0x23, STATIC_CTOR, g_rmgLavaRule)

VA_COMPGEN(0x005B3D50, 0x0A, STATIC_DTOR, g_rmgLavaRule)
DATA(0x006A4CA0)
static TRmgPatternTerrainRule g_rmgWaterRule(false, false, 0, 33, g_rmgWaterPatternEntries);

VA_COMPGEN(0x005B3D60, 0x23, STATIC_CTOR, g_rmgWaterRule)

VA_COMPGEN(0x005B3D90, 0x0A, STATIC_DTOR, g_rmgWaterRule)
DATA(0x006A48C8)
static TRmgTableTerrainRule g_rmgRockRule;

VA_COMPGEN(0x005B3DA0, 0x16, STATIC_CTOR, g_rmgRockRule)

VA_COMPGEN(0x005B3DC0, 0x0A, STATIC_DTOR, g_rmgRockRule)

DATA(0x00642BD8)
TRmgTerrainRule* const g_rmgTerrainRules[10] = {
    &g_rmgDirtRule, &g_rmgSandRule, &g_rmgGrassRule, &g_rmgSnowRule,
    &g_rmgSwampRule, &g_rmgRoughRule, &g_rmgSubterraneanRule,
    &g_rmgLavaRule, &g_rmgWaterRule, &g_rmgRockRule
};
