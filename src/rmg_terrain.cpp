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
    for (unsigned int direction = 0; direction < TILE_DIR_COUNT; ++direction) {
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
    int oldType = tile.getLineType();
    b8 matches[TILE_DIR_COUNT];
    buildMatchingLineNeighbourMask(painter, point, oldType, matches);
    TRmgLinePatternTable* table = painter->getPattern(oldType);
    b8 flipX, flipY;
    int selected = selectRmgLinePattern(matches, table, flipX, flipY);
    rmgTerrainTile current;
    tile.getTile(current);
    if (table->m_patterns[current.getFrame()] != selected
        || current.getFlipX() != flipX || current.getFlipY() != flipY) {
        unsigned int frame = table->m_ranges[selected].m_firstIndex
            + rand() % table->m_ranges[selected].m_valueCount;
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

int TRmgLinePainterInterface::getNeighbourLineType(const TRmgGridPoint& point, unsigned int direction)
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
    unsigned int x, const TRmgGridRectangle& rectangle, unsigned int limit)
{
    unsigned int first = rectangle.m_origin.m_y > 0 ? rectangle.m_origin.m_y - 1 : 0;
    unsigned int end = rectangle.m_origin.m_y + rectangle.m_size.m_y;
    if (end < limit)
        ++end;
    for (TRmgGridPoint point(x, first); point.m_y < end; ++point.m_y)
        refreshExistingRmgLinePoint(painter, point);
}

// Refreshes one border row across the rectangle's width.
static inline void refreshRmgLineBorderRow(TRmgLinePainterInterface* painter,
    unsigned int y, const TRmgGridRectangle& rectangle)
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
    unsigned int error = 0;
    unsigned int distance = major->m_distance;
    for (unsigned int index = 0; index < distance; ++index) {
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
    for (unsigned int direction = 0; direction < TILE_DIR_COUNT; ++direction) {
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

VA(0x005B3780, 0xB3)
MAC_ADDRESS(0x254be0, 0xf4)
TRmgPatternTerrainRule::TRmgPatternTerrainRule(
    b8 blendsWithOtherTerrain, b8 allowsSeparatedNeighbours,
    int specialFrameChance, unsigned int entryCount, const TRmgTerrainPatternEntry* entries)
    : TRmgTerrainRule(blendsWithOtherTerrain, allowsSeparatedNeighbours),
      m_specialFrameChance(specialFrameChance), m_entryCount(entryCount), m_entries(entries)
{
    int transition = m_entries[0].m_transition;
    b8 special = m_entries[0].m_special;
    int range = transition * 2 + special;
    ++m_ranges[range].m_count;
    for (unsigned int index = 1; index < m_entryCount; ++index) {
        const TRmgTerrainPatternEntry& entry = m_entries[index];
        if (entry.m_transition != transition || entry.m_special != special) {
            transition = entry.m_transition;
            special = entry.m_special;
            range = transition * 2 + special;
            m_ranges[range].m_firstIndex = index;
        }
        ++m_ranges[range].m_count;
    }
}

// True when the terrain has special (decorated) base frames.
VA(0x005B3840, 0x0C)
MAC_ADDRESS(0x259dac, 0x14)
b8 TRmgPatternTerrainRule::hasSpecialBaseFrames()
{
    return 0 < m_ranges[1].m_count;
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
    if (!matchesTerrainFrame(m_entries, oldFrame, 0)) {
        TRmgTerrainPatternRange* range;
        if (m_ranges[1].m_count > 0) {
            unsigned int chance =
                static_cast<unsigned int>(m_specialFrameChance * strength) / 8;
            if (static_cast<unsigned int>(rand() % 100) < chance)
                range = &m_ranges[1];
            else
                range = &m_ranges[0];
        } else {
            range = &m_ranges[0];
        }
        oldFrame = selectTerrainRangeFrame(*range);
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
        TRmgTerrainPatternRange& range = m_ranges[transition * 2];
        oldFrame = selectTerrainRangeFrame(range);
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
    for (unsigned int index = 1; index < 48; ++index) {
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
    if (!matchesTerrainFrame(g_rmgTerrainPatterns, oldFrame, 0)) {
        oldFrame = selectTerrainRangeFrame(g_rmgTerrainPatternRanges.m_ranges[0]);
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

// Neighbour direction order for each (flipX, flipY) reflection.
DATA(0x00642C00)
const int g_rmgReflectedNeighbours[2][2][8] = {
    {{0, 1, 2, 3, 4, 5, 6, 7}, {4, 3, 2, 1, 0, 7, 6, 5}},
    {{0, 7, 6, 5, 4, 3, 2, 1}, {4, 5, 6, 7, 0, 1, 2, 3}}
};

static TRmgTerrainFlip makeTerrainFlip(b8 x, b8 y)
{
    return TRmgTerrainFlip(x, y);
}

VA(0x005B3DD0, 0x6F)
MAC_ADDRESS(0x2551b4, 0xd0)
void rmgTerrainPainter::initializePackedCell(
    const TRmgGridPoint& point, unsigned int index)
{
    rmgTerrainTile tile = m_adapter->getTile(point);
    TRmgPackedTerrainCell& packed = m_packedCells[index];
    packed.setTileValues(tile);
    packed.m_initialized = true;
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
            return terrain != eTerrainDirt;
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
    unsigned int reflection;
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (hasSoutheastTerrainCorner(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_BLEND_EDGE)) {
            if (neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE &&
                neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[reflection];
                return 28;
            }
            if (neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[reflection];
                return 27;
            }
        }
    }
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (hasNorthwestTerrainCorner(neighbours, order, RMG_NEIGHBOUR_BLEND_EDGE)) {
            if (neighbours[order[TILE_DIR_SOUTHEAST]] != RMG_NEIGHBOUR_NO_EDGE) {
                *flip = flips[reflection];
                return neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE ? 23 : 25;
            }
        } else if (hasNorthwestTerrainCorner(neighbours, order, RMG_NEIGHBOUR_HARD_EDGE)) {
            if (neighbours[order[TILE_DIR_SOUTHEAST]] != RMG_NEIGHBOUR_NO_EDGE) {
                *flip = flips[reflection];
                return neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE ? 24 : 26;
            }
        }
    }
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (hasSoutheastTerrainCorner(neighbours, order,
                RMG_NEIGHBOUR_HARD_EDGE, RMG_NEIGHBOUR_BLEND_EDGE)) {
            if (neighbours[order[TILE_DIR_SOUTHWEST]] != RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[reflection];
                return 21;
            } else {
                *flip = makeTerrainFlip(!flips[reflection].m_flipX, !flips[reflection].m_flipY);
                return 8;
            }
        }
        if (hasSoutheastTerrainCorner(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_HARD_EDGE)) {
            if (neighbours[order[TILE_DIR_NORTHEAST]] != RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[reflection];
                return 22;
            } else {
                *flip = makeTerrainFlip(!flips[reflection].m_flipX, !flips[reflection].m_flipY);
                return 8;
            }
        }
    }
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (hasSoutheastTerrainCorner(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            if (neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_HARD_EDGE)
                return 17;
            if (neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE)
                return 18;
        }
    }
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (hasNorthwestTerrainCorner(neighbours, order, RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            return 2;
        }
        if (hasNorthwestTerrainCorner(neighbours, order, RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[reflection];
            return 8;
        }
    }
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (hasEastSouthwestTerrainEdges(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[reflection];
            return 17;
        }
        if (hasSouthNortheastTerrainEdges(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[reflection];
            return 18;
        }
        if (hasEastSouthwestTerrainEdges(neighbours, order,
                RMG_NEIGHBOUR_HARD_EDGE, RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            return 21;
        }
        if (hasSouthNortheastTerrainEdges(neighbours, order,
                RMG_NEIGHBOUR_HARD_EDGE, RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            return 22;
        }
        if (hasOffsetNorthwestTerrainCorner(neighbours, order, RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            return 2;
        }
        if (hasOffsetNorthwestTerrainCorner(neighbours, order, RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[reflection];
            return 8;
        }
    }
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return 19;
        }
        if (neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return 20;
        }
    }
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[reflection];
            return 4;
        }
        if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return 10;
        }
        if (neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[reflection];
            return 3;
        }
        if (neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return 9;
        }
    }
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (hasOppositeTerrainDiagonalEdges(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            return 14;
        }
        if (hasOppositeTerrainDiagonalEdges(neighbours, order,
                RMG_NEIGHBOUR_BLEND_EDGE, RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[reflection];
            return 15;
        }
        if (hasOppositeTerrainDiagonalEdges(neighbours, order,
                RMG_NEIGHBOUR_HARD_EDGE, RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[reflection];
            return 16;
        }
    }
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[reflection];
            return 5;
        }
        if (neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return 11;
        }
    }
    *flip = makeTerrainFlip(false, false);
    return 0;
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
    unsigned int index = point.m_y * m_size.m_x + point.m_x;
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

unsigned int rmgTerrainPainter::getWidth() const
{
    return m_size.m_x;
}

unsigned int rmgTerrainPainter::getHeight() const
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
    if (getTerrain(point) != getPaintTerrain())
        return false;
    return true;
}

VA(0x005B4960, 0x1B2)
MAC_ADDRESS(0x255ef0, 0x11c)
void rmgTerrainPainter::paintRectangle(
    unsigned int x, unsigned int y,
    unsigned int rectangleWidth, unsigned int rectangleHeight)
{
    unsigned int endX = x + rectangleWidth;
    unsigned int endY = y + rectangleHeight;
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

// A queued gap that has closed no longer needs primary repair; dequeue it
// and queue its other-terrain neighbours instead.
static inline void resolveQueuedTerrainGap(rmgTerrainPainter& painter,
    const TRmgGridPoint& point, TRmgTerrainGapAxis axis)
{
    if (painter.m_primaryPoints.find(point) == painter.m_primaryPoints.end())
        return;
    b8 remainsGap = axis == RMG_HORIZONTAL_GAP
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
        if (point.m_y > 0) {
            TRmgGridPoint nearby(point.m_x, point.m_y - 1);
            resolveQueuedTerrainGap(*this, nearby, RMG_HORIZONTAL_GAP);
        }
        if (point.m_y < m_size.m_y - 1) {
            TRmgGridPoint nearby(point.m_x, point.m_y + 1);
            resolveQueuedTerrainGap(*this, nearby, RMG_HORIZONTAL_GAP);
        }
        if (point.m_x > 0) {
            TRmgGridPoint nearby(point.m_x - 1, point.m_y);
            resolveQueuedTerrainGap(*this, nearby, RMG_VERTICAL_GAP);
        }
        if (point.m_x < m_size.m_x - 1) {
            TRmgGridPoint nearby(point.m_x + 1, point.m_y);
            resolveQueuedTerrainGap(*this, nearby, RMG_VERTICAL_GAP);
        }
    } else {
        b8 neighbourExists[TILE_DIR_COUNT];
        buildTileNeighbourMask(
            m_size.m_x, m_size.m_y, point.m_x, point.m_y, neighbourExists);
        for (unsigned int direction = 0; direction < TILE_DIR_COUNT; ++direction) {
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

// Diagonal neighbours enter the secondary worklist only for terrain rules
// that require connected neighbours.
static inline void queueOtherTerrainDiagonalNeighbour(
    rmgTerrainPainter& painter, const TRmgGridPoint& neighbour)
{
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
    if (point.getX() > 0 && point.getY() > 0) {
        TRmgGridPoint nearby(point.getX() - 1, point.getY() - 1);
        queueOtherTerrainDiagonalNeighbour(*this, nearby);
    }
    if (point.getX() < getWidth() - 1 && point.getY() > 0) {
        TRmgGridPoint nearby(point.getX() + 1, point.getY() - 1);
        queueOtherTerrainDiagonalNeighbour(*this, nearby);
    }
    if (point.getX() > 0 && point.getY() < getHeight() - 1) {
        TRmgGridPoint nearby(point.getX() - 1, point.getY() + 1);
        queueOtherTerrainDiagonalNeighbour(*this, nearby);
    }
    if (point.getX() < getWidth() - 1 && point.getY() < getHeight() - 1) {
        TRmgGridPoint nearby(point.getX() + 1, point.getY() + 1);
        queueOtherTerrainDiagonalNeighbour(*this, nearby);
    }
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
    if (isHorizontalGap(point))
        return true;
    else if (isVerticalGap(point))
        return true;
    else if (g_rmgTerrainRules[getTerrain(point)]->m_allowsSeparatedNeighbours)
        return false;
    else
        return hasSeparatedNeighbours(point);
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
        unsigned int firstMatch = 0;
        while (!matches[firstMatch])
            ++firstMatch;
        unsigned int gapCount = 0;
        unsigned int direction = (firstMatch + 1) % TILE_DIR_COUNT;
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
                gap.m_weight += (direction & 1) ? 1 : 2;
                ++gap.m_length;
                direction = (direction + 1) % TILE_DIR_COUNT;
            } while (direction != firstMatch && !matches[direction]);
        }
        b8 neighbourExists[TILE_DIR_COUNT];
        buildTileNeighbourMask(getWidth(), getHeight(), point.m_x, point.m_y,
                               neighbourExists);
        do {
            unsigned int smallest = 0;
            unsigned int smallestWeight = gaps[0].m_weight;
            for (unsigned int gap = 1; gap < gapCount; ++gap) {
                if (gaps[gap].m_weight < smallestWeight) {
                    smallest = gap;
                    smallestWeight = gaps[gap].m_weight;
                }
            }
            unsigned int end =
                (gaps[smallest].m_start + gaps[smallest].m_length) % TILE_DIR_COUNT;
            for (unsigned int direction = gaps[smallest].m_start; direction != end;
                 direction = (direction + 1) % TILE_DIR_COUNT) {
                if (neighbourExists[direction])
                    paintPoint(point + g_tileDirections[direction]);
            }
            --gapCount;
            for (unsigned int remainingGap = smallest; remainingGap < gapCount; ++remainingGap)
                gaps[remainingGap] = gaps[remainingGap + 1];
        } while (gapCount > 1);
    }
}

// Count each undirected terrain boundary once, adding it to both endpoints.
static inline void countTerrainBoundary(
    rmgTerrainPainter& painter, const TRmgGridPoint& point,
    const TRmgGridPoint& neighbour, int terrain,
    std::vector<unsigned char>& edgeCounts)
{
    if (painter.getTerrain(neighbour) != terrain) {
        ++edgeCounts[point.getY() * painter.getWidth() + point.getX()];
        ++edgeCounts[neighbour.getY() * painter.getWidth() + neighbour.getX()];
    }
}

// Write a tile back only when its frame or either sprite reflection changes.
static inline void updateTerrainTileAppearance(rmgTerrainPainter& painter,
    const TRmgGridPoint& point, rmgTerrainTile& tile,
    int frame, b8 flipX, b8 flipY)
{
    if (tile.m_frame == frame && tile.m_flipX == flipX && tile.m_flipY == flipY)
        return;
    tile.m_frame = frame;
    tile.m_flipX = flipX;
    tile.m_flipY = flipY;
    painter.setTile(point, tile);
}

VA(0x005B5A70, 0x8A7)
MAC_ADDRESS(0x257214, 0xd54)
void rmgTerrainPainter::paintTransitions()
{
    // Assumes the map has at least two rows and columns.
    std::vector<unsigned char> edgeCounts(getWidth() * getHeight());
    TRmgGridPoint point;

    for (point.setY(0); point.m_y < getHeight() - 1; point.setY(point.getY() + 1)) {
        int terrain = getTerrain(TRmgGridPoint(0, point.m_y));

        countTerrainBoundary(*this, TRmgGridPoint(0, point.m_y),
            TRmgGridPoint(1, point.m_y), terrain, edgeCounts);
        countTerrainBoundary(*this, TRmgGridPoint(0, point.m_y),
            TRmgGridPoint(1, point.m_y + 1), terrain, edgeCounts);
        countTerrainBoundary(*this, TRmgGridPoint(0, point.m_y),
            TRmgGridPoint(0, point.m_y + 1), terrain, edgeCounts);

        for (point.setX(1); point.m_x < getWidth() - 1; point.setX(point.getX() + 1)) {
            terrain = getTerrain(point);

            TRmgGridPoint east(point.getX() + 1, point.getY());
            countTerrainBoundary(*this, point, east, terrain, edgeCounts);
            TRmgGridPoint southEast(point.getX() + 1, point.getY() + 1);
            countTerrainBoundary(*this, point, southEast, terrain, edgeCounts);
            TRmgGridPoint south(point.getX(), point.getY() + 1);
            countTerrainBoundary(*this, point, south, terrain, edgeCounts);
            TRmgGridPoint southWest(point.getX() - 1, point.getY() + 1);
            countTerrainBoundary(*this, point, southWest, terrain, edgeCounts);
        }

        terrain = getTerrain(point);
        TRmgGridPoint south(point.getX(), point.getY() + 1);
        countTerrainBoundary(*this, point, south, terrain, edgeCounts);
        TRmgGridPoint southWest(point.getX() - 1, point.getY() + 1);
        countTerrainBoundary(*this, point, southWest, terrain, edgeCounts);
    }

    for (point.setX(0); point.m_x < getWidth() - 1; point.setX(point.getX() + 1)) {
        int terrain = getTerrain(point);
        TRmgGridPoint east(point.getX() + 1, point.getY());
        countTerrainBoundary(*this, point, east, terrain, edgeCounts);
    }

    for (point.setY(0); point.m_y < getHeight(); point.setY(point.getY() + 1)) {
        for (point.setX(0); point.m_x < getWidth(); point.setX(point.getX() + 1)) {
            unsigned int index = point.getY() * getWidth() + point.getX();

            if (edgeCounts[index] > 0) {
                int neighbours[TILE_DIR_COUNT];
                buildNeighbourKinds(point, neighbours);

                int transition;
                TRmgTerrainFlip flip;
                transition = selectTerrainTransition(neighbours, &flip);
                if (transition == RMG_TERRAIN_FIRST_DIAGONAL_BLEND) {
                    if (checkFirstDiagonal(point, flip))
                        transition = 6;
                } else if (transition == RMG_TERRAIN_FIRST_DIAGONAL_HARD) {
                    if (checkFirstDiagonal(point, flip))
                        transition = 12;
                } else if (transition == RMG_TERRAIN_SECOND_DIAGONAL_BLEND) {
                    if (checkSecondDiagonal(point, flip))
                        transition = 7;
                } else if (transition == RMG_TERRAIN_SECOND_DIAGONAL_HARD) {
                    if (checkSecondDiagonal(point, flip))
                        transition = 13;
                }

                rmgTerrainTile tile = getPackedCell(point)->getTile();

                int newFrame;
                if (transition) {
                    newFrame = g_rmgTerrainRules[tile.m_terrain]
                        ->selectTransitionFrame(
                            transition, flip, flip, tile.m_frame);
                } else {
                    newFrame = selectBaseFrame(point, tile.m_terrain, tile.m_frame);
                }

                updateTerrainTileAppearance(*this, point, tile,
                    newFrame, flip.m_flipX, flip.m_flipY);
            } else {
                rmgTerrainTile tile = getPackedCell(point)->getTile();

                int newFrame = selectBaseFrame(point, tile.m_terrain, tile.m_frame);
                updateTerrainTileAppearance(*this, point, tile, newFrame, false, false);
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
    unsigned int north = point.m_y > 0 ? point.m_y - 1 : point.m_y;
    unsigned int south = point.m_y < size.m_y - 1 ? point.m_y + 1 : point.m_y;
    unsigned int west = point.m_x > 0 ? point.m_x - 1 : point.m_x;
    unsigned int east = point.m_x < size.m_x - 1 ? point.m_x + 1 : point.m_x;
    northWest = TRmgGridPoint(west, north);
    southEast = TRmgGridPoint(east, south);
}

static inline bool matchesTerrainAt(rmgTerrainPainter& painter,
    unsigned int x, unsigned int y, int terrain)
{
    TRmgGridPoint nearby;
    nearby.setX(x);
    nearby.setY(y);
    return painter.getTerrain(nearby) == terrain;
}

// Cardinal neighbours use coordinates clamped to the map edge. A diagonal
// contributes only when at least one adjoining cardinal cell also matches.
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
    matches[TILE_DIR_NORTHWEST] =
        (matches[TILE_DIR_NORTH] || matches[TILE_DIR_WEST])
        && matchesTerrainAt(*this, northWest.getX(), northWest.getY(), terrain);
    matches[TILE_DIR_NORTHEAST] =
        (matches[TILE_DIR_NORTH] || matches[TILE_DIR_EAST])
        && matchesTerrainAt(*this, southEast.getX(), northWest.getY(), terrain);
    matches[TILE_DIR_SOUTHWEST] =
        (matches[TILE_DIR_SOUTH] || matches[TILE_DIR_WEST])
        && matchesTerrainAt(*this, northWest.getX(), southEast.getY(), terrain);
    matches[TILE_DIR_SOUTHEAST] =
        (matches[TILE_DIR_SOUTH] || matches[TILE_DIR_EAST])
        && matchesTerrainAt(*this, southEast.getX(), southEast.getY(), terrain);
}

// Scan cyclic runs: after the first matching run and its following gap,
// another matching cell proves that the centre's neighbours are separated.
VA(0x005B6810, 0x84)
MAC_ADDRESS(0x2587f4, 0xd0)
b8 rmgTerrainPainter::hasSeparatedNeighbours(const TRmgGridPoint& point)
{
    b8 matches[TILE_DIR_COUNT];
    buildMatchingNeighbourMask(point, matches);
    unsigned int first = 0;
    unsigned int direction;
    while (matches[first]) {
        first = (first + 1) % TILE_DIR_COUNT;
        if (first == 0)
            return false;
    }
    direction = first;
    do {
        direction = (direction + 1) % TILE_DIR_COUNT;
        if (direction == first)
            return false;
    } while (!matches[direction]);
    do {
        direction = (direction + 1) % TILE_DIR_COUNT;
        if (direction == first)
            return false;
    } while (matches[direction]);
    while (!matches[direction]) {
        direction = (direction + 1) % TILE_DIR_COUNT;
        if (direction == first)
            return false;
    }
    return true;
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

VA(0x005B6BA0, 0x24C)
MAC_ADDRESS(0x258f18, 0x360)
b8 rmgTerrainPainter::checkFirstDiagonal(
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
    const TPoint* pair = firstDiagonalOffsets[(flip.m_flipY << 1) | flip.m_flipX];
    if (matchesTerrainAtClampedOffset(*this, point, pair[0], terrain))
        return true;
    return matchesTerrainAtClampedOffset(*this, point, pair[1], terrain);
}

VA(0x005B6E00, 0x1B3)
MAC_ADDRESS(0x259278, 0x288)
b8 rmgTerrainPainter::checkSecondDiagonal(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    DATA_COMPGEN_GUARD(0x006a3d64, secondDiagonalOffsetsGuard, secondDiagonalOffsets)

    VA_COMPGEN(0x005b6fc0, 0x1, STATIC_DTOR, secondDiagonalOffsets)
    DATA(0x006A3D68)
    static TPoint secondDiagonalOffsets[4] = {
        TPoint(2, 2), TPoint(-2, 2), TPoint(2, -2), TPoint(-2, -2)
    };
    int terrain = getTerrain(point);
    const TPoint& offset = secondDiagonalOffsets[(flip.m_flipY << 1) | flip.m_flipX];
    return !matchesTerrainAtClampedOffset(*this, point, TPoint(offset.getX(), 0), terrain)
        || !matchesTerrainAtClampedOffset(*this, point, TPoint(0, offset.getY()), terrain);
}

// Each same-terrain cardinal neighbour showing a special frame halves the
// special-frame strength.
static inline bool hasSpecialTerrainFrameAt(rmgTerrainPainter& painter,
    const TRmgGridPoint& point, int terrain, TRmgTerrainRule* rule)
{
    return painter.getTerrain(point) == terrain
        && rule->isSpecialFrame(painter.getFrame(point));
}

VA(0x005B6FD0, 0x271)
MAC_ADDRESS(0x259500, 0x444)
int rmgTerrainPainter::getSpecialFrameStrength(
    const TRmgGridPoint& point, int terrain)
{
    unsigned int strength = m_specialFrameStrength;
    TRmgTerrainRule* rule = g_rmgTerrainRules[terrain];
    if (point.getX() > 0) {
        TRmgGridPoint nearby(point.getX(), point.getY());
        nearby.setX(point.getX() - 1);
        if (hasSpecialTerrainFrameAt(*this, nearby, terrain, rule))
            strength >>= 1;
    }
    if (point.getY() > 0) {
        TRmgGridPoint nearby(point.getX(), point.getY());
        nearby.setY(point.getY() - 1);
        if (hasSpecialTerrainFrameAt(*this, nearby, terrain, rule))
            strength >>= 1;
    }
    if (point.getX() < getWidth() - 1) {
        TRmgGridPoint nearby(point.getX(), point.getY());
        nearby.setX(point.getX() + 1);
        if (hasSpecialTerrainFrameAt(*this, nearby, terrain, rule))
            strength >>= 1;
    }
    if (point.getY() < getHeight() - 1) {
        TRmgGridPoint nearby(point.getX(), point.getY());
        nearby.setY(point.getY() + 1);
        if (hasSpecialTerrainFrameAt(*this, nearby, terrain, rule))
            strength >>= 1;
    }
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
    unsigned int x, unsigned int y,
    unsigned int rectangleWidth, unsigned int rectangleHeight)
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
const TRmgTerrainTransitionEntry g_rmgTerrainPatterns[48] = {
    {0, false, false}, {0, false, false}, {0, false, false}, {0, false, false}, {0, false, false}, {0, false, false},
    {0, false, false}, {0, false, false}, {8, false, false}, {8, false, false}, {8, true, false}, {8, true, false},
    {8, false, true}, {8, false, true}, {8, true, true}, {8, true, true}, {9, false, false}, {9, false, false},
    {9, true, false}, {9, true, false}, {10, false, false}, {10, false, false}, {10, false, true}, {10, false, true},
    {11, false, false}, {11, false, false}, {11, true, false}, {11, true, false}, {11, false, true}, {11, false, true},
    {11, true, true}, {11, true, true}, {12, false, false}, {12, false, false}, {12, true, false}, {12, true, false},
    {12, false, true}, {12, false, true}, {12, true, true}, {12, true, true}, {13, false, false}, {13, false, false},
    {13, true, false}, {13, true, false}, {13, false, true}, {13, false, true}, {13, true, true}, {13, true, true},
};

DATA(0x00642628)
static const TRmgTerrainPatternEntry g_rmgLandPatternEntries[79] = {
    {2, false}, {2, false}, {2, false}, {2, false}, {3, false}, {3, false},
    {3, false}, {3, false}, {4, false}, {4, false}, {4, false}, {4, false},
    {5, false}, {5, false}, {5, false}, {5, false}, {6, false}, {6, false},
    {7, false}, {7, false}, {8, false}, {8, false}, {8, false}, {8, false},
    {9, false}, {9, false}, {9, false}, {9, false}, {10, false}, {10, false},
    {10, false}, {10, false}, {11, false}, {11, false}, {11, false}, {11, false},
    {12, false}, {12, false}, {13, false}, {13, false}, {14, false}, {15, false},
    {16, false}, {17, false}, {18, false}, {19, false}, {20, false}, {21, false},
    {22, false}, {0, false}, {0, false}, {0, false}, {0, false}, {0, false},
    {0, false}, {0, false}, {0, false}, {0, true}, {0, true}, {0, true},
    {0, true}, {0, true}, {0, true}, {0, true}, {0, true}, {0, true},
    {0, true}, {0, true}, {0, true}, {0, true}, {0, true}, {0, true},
    {0, true}, {23, false}, {24, false}, {25, false}, {26, false}, {28, false},
    {27, false},
};

DATA(0x006428A0)
static const TRmgTerrainPatternEntry g_rmgDirtPatternEntries[46] = {
    {8, false}, {8, false}, {8, false}, {8, false}, {9, false}, {9, false},
    {9, false}, {9, false}, {10, false}, {10, false}, {10, false}, {10, false},
    {11, false}, {11, false}, {11, false}, {11, false}, {12, false}, {12, false},
    {13, false}, {13, false}, {16, false}, {0, false}, {0, false}, {0, false},
    {0, false}, {0, false}, {0, false}, {0, false}, {0, false}, {0, true},
    {0, true}, {0, true}, {0, true}, {0, true}, {0, true}, {0, true},
    {0, true}, {0, true}, {0, true}, {0, true}, {0, true}, {0, true},
    {0, true}, {0, true}, {0, true}, {24, false},
};

DATA(0x00642A10)
static const TRmgTerrainPatternEntry g_rmgSandPatternEntries[24] = {
    {0, false}, {0, false}, {0, false}, {0, false}, {0, false}, {0, false},
    {0, false}, {0, false}, {0, true}, {0, true}, {0, true}, {0, true},
    {0, true}, {0, true}, {0, true}, {0, true}, {0, true}, {0, true},
    {0, true}, {0, true}, {0, true}, {0, true}, {0, true}, {0, true},
};

DATA(0x00642AD0)
static const TRmgTerrainPatternEntry g_rmgWaterPatternEntries[33] = {
    {8, false}, {8, false}, {8, false}, {8, false}, {9, false}, {9, false},
    {9, false}, {9, false}, {10, false}, {10, false}, {10, false}, {10, false},
    {11, false}, {11, false}, {11, false}, {11, false}, {12, false}, {12, false},
    {13, false}, {13, false}, {16, false}, {0, false}, {0, false}, {0, false},
    {0, false}, {0, false}, {0, false}, {0, false}, {0, false}, {0, false},
    {0, false}, {0, false}, {0, false},
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
