// rmg_terrain.cpp - Complete-only random-map terrain transition support.

// The Dreamcast build contains no random-map generator compiland. Function
// ownership, field layout, helper boundaries, and call/expansion decisions in
// this unit therefore come directly from the retail x86 cluster.
#include "va.h"
#include "includes.h"

#include <stdlib.h>

#include "rmg_terrain.h"

#include "exceptions.h"
#include "tiles.h"

// Vtable 0x642cb0 slot 2 shares the false/ret-4 body at 0x5543f0.
MAC_ADDRESS(0x259d44, 0x8)
unsigned char TRmgTableTerrainRule::isSpecialFrame(int) { return 0; }

// Initializer-list copy of the point: the body assignment costs the
// walker's first neighbour pass its retained compound add (78.03 against
// 87.12%); the factory and rectangle clear are byte-identical either way.
TRmgLinePainterTile::TRmgLinePainterTile(
    TRmgLinePainterInterface* painter, const TRmgGridPoint& point)
    : m_painter(painter), m_point(point)
{
}

// The 168-case query family tested receiver/coordinate reference bindings,
// their order, named results and real proxy construction (80 code results).
// No gain over the direct query: clear 94.5070%, point 79.9380%, refresh 63.9923%.
int TRmgLinePainterTile::getLand()
{
    return m_painter->getLand(m_point);
}

void TRmgLinePainterTile::getTile(rmgTerrainTile& tile)
{
    m_painter->getTile(m_point, tile);
}

void TRmgLinePainterTile::setTile(const rmgTerrainTile& tile)
{
    m_painter->setTile(m_point, tile);
}

// The point walker tests AL after slot 3 (0x4fa400..0x4fa405). Keep that
// low-byte gate while preserving the retained virtual's integer-return ABI.
// Nonzero blocks painting on water and rock.
unsigned char TRmgLinePainterTile::isBlocked()
{
    return m_painter->isBlocked(m_point);
}

void TRmgLinePainterTile::setOverlay(int value)
{
    m_painter->setOverlay(m_point, value);
}

// The size is a grid point: the walker's one-cell rectangle then constructs
// a unit size the way retail 0x4fa3c0 materializes it (see paintPoint).
TRmgGridRectangle::TRmgGridRectangle(const TRmgGridPoint& origin, const TRmgGridPoint& size)
    : m_origin(origin), m_size(size)
{
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
int selectRmgLinePattern(
    const unsigned char* neighbours, const TRmgLinePatternTable* table,
    unsigned char& flipX, unsigned char& flipY)
{
    int pattern;
    selectRmgLinePattern(neighbours, table, pattern, flipX, flipY);
    return pattern;
}

// Shared cleanup helper: query valid neighbours in direction-table order.
// getNeighbourLand owns the signed-coordinate conversion and tile proxy.
static inline void buildMatchingLineNeighbourMask(
    TRmgLinePainterInterface* painter, const TRmgGridPoint& point,
    int lineType, unsigned char* matches)
{
    unsigned char available[TILE_DIR_COUNT];
    buildTileNeighbourMask(painter->m_size.m_x, painter->m_size.m_y,
                           point.m_x, point.m_y, available);
    for (unsigned int direction = 0; direction < TILE_DIR_COUNT; ++direction) {
        if (available[direction])
            matches[direction] = painter->getNeighbourLand(point, direction) == lineType;
        else
            matches[direction] = 0;
    }
}

VA(0x004F9F00, 0x146)
MAC_ADDRESS(0x22273c, 0x168) // anchor-caller 0x4fa080/0x4fa3c0; fastcall, no stack args
void refreshRmgLinePoint(TRmgLinePainterInterface* painter, const TRmgGridPoint& point)
{
    TRmgLinePainterTile tile = painter->at(point);
    int oldType = tile.getLand();
    unsigned char matches[TILE_DIR_COUNT];
    buildMatchingLineNeighbourMask(painter, point, oldType, matches);
    TRmgLinePatternTable* table = painter->getPattern(oldType);
    unsigned char flipX, flipY;
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

VA(0x004FA050, 0x22) // anchor-callee 0x4f9f86; thiscall hidden value return
TRmgLinePainterTile TRmgLinePainterInterface::at(const TRmgGridPoint& point)
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
int TRmgLinePainterInterface::getNeighbourLand(const TRmgGridPoint& point, unsigned int direction)
{
    TRmgGridPoint nearby = point + g_tileDirections[direction];
    return at(nearby).getLand();
}

VA(0x004FA080, 0x1FB)
MAC_ADDRESS(0x2228b8, 0x388) // anchor-callee 0x4fa42c; fastcall, no stack args
void clearRmgLineRectangle(TRmgLinePainterInterface* painter, const TRmgGridRectangle& rectangle)
{
    TRmgGridPoint point;
    for (point.m_y = rectangle.m_origin.m_y;
         point.m_y < rectangle.m_origin.m_y + rectangle.m_size.m_y; ++point.m_y) {
        for (point.m_x = rectangle.m_origin.m_x;
             point.m_x < rectangle.m_origin.m_x + rectangle.m_size.m_x; ++point.m_x) {
            TRmgLinePainterTile tile(painter, point);
            if (tile.getLand())
                tile.setTile(rmgTerrainTile(0, 0));
        }
    }
    if (rectangle.m_origin.m_x > 0) {
        point.m_x = rectangle.m_origin.m_x - 1;
        unsigned int first = rectangle.m_origin.m_y > 0 ? rectangle.m_origin.m_y - 1 : 0;
        unsigned int end = rectangle.m_origin.m_y + rectangle.m_size.m_y < painter->m_size.m_y
            ? rectangle.m_origin.m_y + rectangle.m_size.m_y + 1
            : rectangle.m_origin.m_y + rectangle.m_size.m_y;
        for (point.m_y = first; point.m_y < end; ++point.m_y) {
            if (painter->at(point).getLand())
                refreshRmgLinePoint(painter, point);
        }
    }
    if (rectangle.m_origin.m_x + rectangle.m_size.m_x < painter->m_size.m_x) {
        point.m_x = rectangle.m_origin.m_x + rectangle.m_size.m_x;
        unsigned int first = rectangle.m_origin.m_y > 0 ? rectangle.m_origin.m_y - 1 : 0;
        // Preserve retail's asymmetric right-edge limit: unlike the left
        // edge, a rectangle ending on the penultimate row excludes the last
        // row here. Normalizing these bounds would change refreshed tiles.
        unsigned int end = rectangle.m_origin.m_y + rectangle.m_size.m_y < painter->m_size.m_y - 1
            ? rectangle.m_origin.m_y + rectangle.m_size.m_y + 1
            : rectangle.m_origin.m_y + rectangle.m_size.m_y;
        for (point.m_y = first; point.m_y < end; ++point.m_y) {
            if (painter->at(point).getLand())
                refreshRmgLinePoint(painter, point);
        }
    }
    if (rectangle.m_origin.m_y > 0) {
        point.m_y = rectangle.m_origin.m_y - 1;
        for (point.m_x = rectangle.m_origin.m_x;
             point.m_x < rectangle.m_origin.m_x + rectangle.m_size.m_x; ++point.m_x) {
            if (painter->at(point).getLand())
                refreshRmgLinePoint(painter, point);
        }
    }
    if (rectangle.m_origin.m_y + rectangle.m_size.m_y < painter->m_size.m_y) {
        point.m_y = rectangle.m_origin.m_y + rectangle.m_size.m_y;
        for (point.m_x = rectangle.m_origin.m_x;
             point.m_x < rectangle.m_origin.m_x + rectangle.m_size.m_x; ++point.m_x) {
            if (painter->at(point).getLand())
                refreshRmgLinePoint(painter, point);
        }
    }
}

VA(0x004FA280, 0x30)
MAC_ADDRESS(0x222c40, 0x4c) // anchor-caller 0x55ee50/0x55f3b0; thiscall ret 0xc
TRmgLineWalker::TRmgLineWalker(
    TRmgLinePainterInterface* newPainter,
    int newLineType,
    const TRmgGridPoint& start)
    : m_painter(newPainter), m_lineType(newLineType), m_position(start)
{
    paintPoint(m_position);
}

// Integer error-accumulation line rasterization (a four-connected Bresenham
// variant). Walk destination-to-start and paint the intermediate cell before
// each minor-axis step, preserving river connectivity and visit order.
VA(0x004FA2B0, 0x110)
MAC_ADDRESS(0x222c8c, 0x190) // anchor-caller 0x548040 and createRiver; thiscall ret 4
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
MAC_ADDRESS(0x222e1c, 0x18c) // anchor-caller 0x4fa280/0x4fa2b0; thiscall, ret 4
void TRmgLineWalker::paintPoint(const TRmgGridPoint& point)
{
    TRmgLinePainterTile tile(m_painter, point);
    int oldType = tile.getLand();
    if (oldType == m_lineType || tile.isBlocked())
        return;
    if (oldType)
        clearRmgLineRectangle(m_painter, TRmgGridRectangle(point, TRmgGridPoint(1, 1)));
    tile.setOverlay(m_lineType);
    refreshRmgLinePoint(m_painter, point);

    unsigned char matches[TILE_DIR_COUNT];
    buildMatchingLineNeighbourMask(m_painter, point, m_lineType, matches);
    for (unsigned int direction = 0; direction < TILE_DIR_COUNT; ++direction) {
        if (matches[direction])
            refreshRmgLinePoint(m_painter, point + g_tileDirections[direction]);
    }
}

template<class Coordinate>
// VA instance: TRmgCoordinatePoint<unsigned int>::TRmgCoordinatePoint(const TPoint&)
VA(0x004FA520, 0x16)
MAC_ADDRESS(0x2228a4, 0x14) // anchor-callee 0x4f9f77; thiscall, ret 4
TRmgCoordinatePoint<Coordinate>::TRmgCoordinatePoint(const TPoint& point)
    : m_x(point.m_x), m_y(point.m_y)
{
}

VA(0x004FA540, 0x21) // anchor-callers 0x4f9f00/0x4fa3c0; thiscall, ret 4
TPoint& TPoint::operator+=(const TPoint& offset)
{
    m_x += offset.m_x;
    m_y += offset.m_y;
    return *this;
}

VA(0x005B3780, 0xB3)
MAC_ADDRESS(0x254be0, 0xf4)
TRmgPatternTerrainRule::TRmgPatternTerrainRule(
    unsigned char blendsWithOtherTerrain, unsigned char allowsSeparatedNeighbours,
    int specialFrameChance, unsigned int entryCount, const TRmgTerrainPatternEntry* entries)
    : TRmgTerrainRule(blendsWithOtherTerrain, allowsSeparatedNeighbours),
      m_specialFrameChance(specialFrameChance), m_entryCount(entryCount), m_entries(entries)
{
    int transition = m_entries[0].m_transition;
    unsigned char special = m_entries[0].m_special;
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

// Vtable 0x642c98 slot 1 tests the special base-frame range (key 1). The constructor
// at 0x5b3780 builds that range at +0x1c/+0x20 from its supplied entry array.
VA(0x005B3840, 0x0C)
MAC_ADDRESS(0x259dac, 0x14)  // Complete-only pattern terrain rule
unsigned char TRmgPatternTerrainRule::hasSpecialBaseFrames()
{
    return 0 < m_ranges[1].m_count;
}

VA(0x005B3850, 0x07)
MAC_ADDRESS(0x254b98, 0x48)  // terrain-rule deleting destructors; Complete-only
TRmgTerrainRule::~TRmgTerrainRule()
{
}

// Vtable 0x642c98 slot 2 reads the byte at +4 in an eight-byte source entry.
// Constructor 0x5b3780 retains the entry pointer at +0x10 (0x5b37a2).
VA(0x005B3860, 0x11)
MAC_ADDRESS(0x254ce4, 0x14)  // Complete-only pattern terrain rule
unsigned char TRmgPatternTerrainRule::isSpecialFrame(int frame)
{
    return m_entries[frame].m_special;
}

// Each source entry is two dwords. Vtable 0x642c98 slot 3 returns
// the first dword of the requested entry through the pointer at +0x10.
VA(0x005B3880, 0x10)
MAC_ADDRESS(0x254cf8, 0x10)  // Complete-only pattern terrain rule
int TRmgPatternTerrainRule::getTransition(int frame)
{
    return m_entries[frame].m_transition;
}

// Cleanup helper shared by both rule implementations. Keep retail's modulo
// selection, including its RNG consumption and bias; all selected ranges
// are nonempty in the admitted terrain tables.
static inline int selectTerrainRangeFrame(const TRmgTerrainPatternRange& range)
{
    return rand() % range.m_count + range.m_firstIndex;
}

// The base-frame selector keeps a zero-tagged old entry. Otherwise it picks
// the secondary range with the rule's strength-scaled percentage, falling
// back to the primary range, then uses the retail modulo draw within it.
VA(0x005B3890, 0x58)
MAC_ADDRESS(0x254d08, 0xc8)
int TRmgPatternTerrainRule::selectBaseFrame(int strength, int oldFrame)
{
    if (oldFrame == -1 || m_entries[oldFrame].m_transition != 0) {
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

// Transition ranges are stored as two first/count pairs per transition.
// This selector uses the first pair and preserves an old frame whose entry
// already names the requested transition; the requested flip is copied out.
VA(0x005B38F0, 0x41)
MAC_ADDRESS(0x254dd0, 0x88)
int TRmgPatternTerrainRule::selectTransitionFrame(
    int transition,
    TRmgTerrainFlip requestedFlip,
    TRmgTerrainFlip& selectedFlip,
    int oldFrame)
{
    if (oldFrame == -1 || m_entries[oldFrame].m_transition != transition) {
        TRmgTerrainPatternRange& range = m_ranges[transition * 2];
        oldFrame = selectTerrainRangeFrame(range);
    }
    selectedFlip = requestedFlip;
    return oldFrame;
}

DATA(0x006A4158)
TRmgTerrainPatternTable g_rmgTerrainPatternRanges;

VA(0x005B3940, 0xC5)
MAC_ADDRESS(0x254e58, 0xe4)
TRmgTerrainPatternTable::TRmgTerrainPatternTable()
{
    int transition = g_rmgTerrainPatterns[0].m_transition;
    unsigned char flipX = g_rmgTerrainPatterns[0].m_flipX;
    unsigned char flipY = g_rmgTerrainPatterns[0].m_flipY;
    TRmgTerrainPatternRange* range =
        &m_ranges[(transition * 2 + flipX) * 2 + flipY];
    ++range->m_count;
    for (unsigned int index = 1; index < 48; ++index) {
        if (g_rmgTerrainPatterns[index].m_transition != transition
            || g_rmgTerrainPatterns[index].m_flipX != flipX
            || g_rmgTerrainPatterns[index].m_flipY != flipY) {
            transition = g_rmgTerrainPatterns[index].m_transition;
            flipX = g_rmgTerrainPatterns[index].m_flipX;
            flipY = g_rmgTerrainPatterns[index].m_flipY;
            range = &m_ranges[(transition * 2 + flipX) * 2 + flipY];
            range->m_firstIndex = index;
        }
        ++range->m_count;
    }
}

VA(0x005B3A20, 0x11)
MAC_ADDRESS(0x254f4c, 0x20)  // Complete-only table terrain rule
TRmgTableTerrainRule::TRmgTableTerrainRule()
{
}

VA(0x005B3A40, 0x03)
MAC_ADDRESS(0x254f6c, 0x8)
unsigned char TRmgTableTerrainRule::hasSpecialBaseFrames()
{
    return 0;
}

// Both concrete six-slot terrain-rule vtables use this ICF-folded deleting
// wrapper. The emitted table-rule closure calls the shared retained destructor
// at 0x5b3850 and has the same complete-object delete semantics.
// The stateless table rule uses its implicit virtual destructor: retail
// 0x5b3a56 calls the retained base, then bit 0 selects scalar deletion.
VA_COMPGEN(0x005B3A50, 0x21, SCALAR_DELETING_DTOR, TRmgTableTerrainRule)

VA(0x005B3A80, 0x11)
MAC_ADDRESS(0x254f74, 0x10)
int TRmgTableTerrainRule::getTransition(int frame)
{
    return g_rmgTerrainPatterns[frame].m_transition;
}

VA(0x005B3AA0, 0x31)
MAC_ADDRESS(0x254f84, 0x94)  // vtable 0x642cb0 slot 4; Complete-only table rule
int TRmgTableTerrainRule::selectBaseFrame(int, int oldFrame)
{
    if (oldFrame == -1
        || g_rmgTerrainPatterns[oldFrame].m_transition != 0) {
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
        || g_rmgTerrainPatterns[oldFrame].m_transition != transition
        || g_rmgTerrainPatterns[oldFrame].m_flipX != requestedFlip.m_flipX
        || g_rmgTerrainPatterns[oldFrame].m_flipY != requestedFlip.m_flipY) {
        TRmgTerrainPatternRange& range = g_rmgTerrainPatternRanges.m_ranges[
            (transition * 2 + requestedFlip.m_flipX) * 2 + requestedFlip.m_flipY];
        oldFrame = selectTerrainRangeFrame(range);
    }
    selectedFlip = TRmgTerrainFlip(0, 0);
    return oldFrame;
}

// Called at 0x5b5f5b. All selector names are provisional retail roles.
int __fastcall selectTerrainTransition(
    const int* neighbours, TRmgTerrainFlip* flip);

// Four reflections of the eight neighbour slots. Read from the pinned
// retail image; both flip bytes index this table independently.
DATA(0x00642C00)
const int g_rmgReflectedNeighbours[2][2][8] = {
    {{0, 1, 2, 3, 4, 5, 6, 7}, {4, 3, 2, 1, 0, 7, 6, 5}},
    {{0, 7, 6, 5, 4, 3, 2, 1}, {4, 5, 6, 7, 0, 1, 2, 3}}
};

// Provisional value-return helper, auto-inlined at every selector site.
// Returning the constructed value reproduces retail's temporary at ebp-2
// and the saved output pointer at ebp-8. A named local return instead puts
// them at ebp-8 and ebp-4 (99.93%); replacing the helper calls with direct
// construction changes the fourth reflection loop's registers (99.7991%).
// An explicit empty flip destructor prevents the helper from auto-inlining.
static TRmgTerrainFlip makeTerrainFlip(unsigned char x, unsigned char y)
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
    packed.m_terrain = tile.m_terrain;
    packed.m_frame = tile.m_frame;
    packed.m_flipX = tile.m_flipX;
    packed.m_flipY = tile.m_flipY;
    packed.m_initialized = 1;
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

// Priority-ordered terrain pattern classification under four reflections.
// Each pass considers one pattern family; combining the passes into a single
// reflection loop would change which overlapping pattern wins.
VA(0x005B3E80, 0x75F)
MAC_ADDRESS(0x2552e8, 0x980)  // fastcall call at 0x5b5f5b; retail-only
int __fastcall selectTerrainTransition(
    const int* neighbours, TRmgTerrainFlip* flip)
{
    // Retail construction guard byte 0x6a52a1 (tested and set in this body).
    DATA_COMPGEN_GUARD(0x006a52a1, terrainFlipsGuard, flips)

    // atexit(0x5b45e0): the table's empty cleanup, right after this function.
    VA_COMPGEN(0x005b45e0, 0x1, STATIC_DTOR, flips)
    DATA(0x006A52B8)
    static TRmgTerrainFlip flips[4] = {
        makeTerrainFlip(0, 0), makeTerrainFlip(0, 1),
        makeTerrainFlip(1, 0), makeTerrainFlip(1, 1)
    };
    unsigned int reflection;
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_BLEND_EDGE) {
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
        if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            if (neighbours[order[TILE_DIR_SOUTHEAST]] != RMG_NEIGHBOUR_NO_EDGE) {
                *flip = flips[reflection];
                return neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE ? 23 : 25;
            }
        } else if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_HARD_EDGE &&
                   neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            if (neighbours[order[TILE_DIR_SOUTHEAST]] != RMG_NEIGHBOUR_NO_EDGE) {
                *flip = flips[reflection];
                return neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE ? 24 : 26;
            }
        }
    }
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            if (neighbours[order[TILE_DIR_SOUTHWEST]] != RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[reflection];
                return 21;
            } else {
                *flip = makeTerrainFlip(!flips[reflection].m_flipX, !flips[reflection].m_flipY);
                return 8;
            }
        }
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_HARD_EDGE) {
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
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_BLEND_EDGE) {
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
        if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[reflection];
            return 2;
        }
        if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return 8;
        }
    }
    for (reflection = 0; reflection < 4; ++reflection) {
        const int* order = g_rmgReflectedNeighbours
            [flips[reflection].m_flipX][flips[reflection].m_flipY];
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return 17;
        }
        if (neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return 18;
        }
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[reflection];
            return 21;
        }
        if (neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[reflection];
            return 22;
        }
        if ((neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
             neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE) ||
            (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_BLEND_EDGE &&
             neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[reflection];
            return 2;
        }
        if ((neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_HARD_EDGE &&
             neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) ||
            (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_HARD_EDGE &&
             neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_HARD_EDGE)) {
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
        if (neighbours[order[TILE_DIR_NORTHWEST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[reflection];
            return 14;
        }
        if (neighbours[order[TILE_DIR_NORTHWEST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[reflection];
            return 15;
        }
        if (neighbours[order[TILE_DIR_NORTHWEST]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
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
    *flip = makeTerrainFlip(0, 0);
    return 0;
}

// The explicit output temporary uses VC6's non-const-reference binding
// extension. The returned reference is copied before that temporary dies at
// the full-expression boundary, matching retail's short output lifetime.
// This preserves virtual slot 3's proven ABI and both exact adapter bodies.
// All 621 bytes, 16 direct calls and virtual slot 3 reproduce; a default
// output argument gives the same result. A separate value-query helper uses
// inline budget and leaves the shrinking size() call retained (93.0418%).
// No separate
// convenience helper or artificial caller scope is needed.
VA(0x005B45F0, 0x26D)
MAC_ADDRESS(0x255c68, 0xbc)
rmgTerrainPainter::rmgTerrainPainter(
    TRmgMapInterface* newAdapter, int terrain, int strength)
    : m_adapter(newAdapter), m_paintTerrain(terrain), m_transitionStrength(strength)
{
#if defined(HOMM3_TARGET_MAC)
    // Mac 0x255c68 calls the value-returning slot.
    m_size = m_adapter->getSize();
#else
    m_size = m_adapter->getSize(TRmgGridPoint());
#endif
    m_packedCells.resize(getWidth() * getHeight(), TRmgPackedTerrainCell());
}

VA(0x005B48D0, 0x8D)  // repeated caller identity in 0x5b3dd0..0x5b76f0
TRmgPackedTerrainCell* rmgTerrainPainter::getPackedCell(
    const TRmgGridPoint& point)
{
    unsigned int index = point.m_y * m_size.m_x + point.m_x;
    if (!m_packedCells[index].m_initialized)
        initializePackedCell(point, index);
    return &m_packedCells[index];
}

// Retail proves the shared accessor and its expanded uses, but supplies no
// source inline qualifier. Its ordinary TU definition preserves paintPoint's
// 99.5570% checkpoint and both exact painter/brush destructors.
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

// The base-frame paths in PaintPoint and PaintTransitions first compute
// strength, then load the selected rule's virtual receiver. Keep that shared
// evaluation boundary and the captured terrain index across the first call.
// The helper's role and signature are inferred from retail expansions.
MAC_ADDRESS(0x259998, 0x60)
int rmgTerrainPainter::selectBaseFrame(
    const TRmgGridPoint& point, int terrain, int oldFrame)
{
    int strength = getTransitionStrength(point, terrain);
    return g_rmgTerrainRules[terrain]->selectBaseFrame(strength, oldFrame);
}

// PaintPoint and both transition updates repeat this adapter/cache write.
// Preserve the shared operation, including validity before the four values;
// cache initialization from an adapter read has a different store order.
// This ordinary helper is inferred from retail expansions, with no DC name.
MAC_ADDRESS(0x2550f0, 0xc4)
void rmgTerrainPainter::setTile(
    const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    m_adapter->setTile(point, tile);
    TRmgPackedTerrainCell& packed = m_packedCells[point.m_y * m_size.m_x + point.m_x];
    packed.setInitialized();
    packed.setTerrain(tile.m_terrain);
    packed.setFrame(tile.m_frame);
    packed.setFlipX(tile.m_flipX);
    packed.setFlipY(tile.m_flipY);
}

// Shared base-tile operation for rectangle painting and individual repairs.
void rmgTerrainPainter::paintBaseTile(const TRmgGridPoint& point)
{
    int frame = selectBaseFrame(point, m_paintTerrain, -1);
    rmgTerrainTile tile(m_paintTerrain, frame);
    setTile(point, tile);
}

// Provisional terrain-comparison interface, inferred from the first
// eight-neighbour read at retail 0x5b4e55. Both accessors participate:
// flattening the configured-terrain read into the predicate expands that
// cache call. Keep these ordinary helpers and the base-tile constructor;
// their combined expansions recover the first and final cache boundaries.
// The guard return costs 47 against the comparison return's 38 (free).
// Under the trivial grid copy and a direct-initialized neighbour the free
// form expanded the loop erase's distance wrapper (98.68%); the 47-unit
// form refuses it while the final insert's pair constructor still expands,
// closing paintPoint (2026-09-12). Reading the configured terrain as the
// field, or swapping the operands, drops the caller below 90%.
const int& rmgTerrainPainter::getPaintTerrain() const
{
    return m_paintTerrain;
}

unsigned char rmgTerrainPainter::isPaintTerrain(const TRmgGridPoint& point)
{
    if (getTerrain(point) != getPaintTerrain())
        return 0;
    return 1;
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
            if (m_paintTerrain != getTerrain(point)) {
                paintPoint(point);
            } else {
                paintBaseTile(point);
            }
        }
    }
}

VA(0x005B4B20, 0x5CB)
MAC_ADDRESS(0x256014, 0x580) // anchor-callee 0x5b4960, 0x5b5440; thiscall, ret 4
void rmgTerrainPainter::paintPoint(const TRmgGridPoint& point)
{
    paintBaseTile(point);

    if (m_secondaryPoints.find(point) != m_secondaryPoints.end())
        m_secondaryPoints.erase(point);

    if (g_rmgTerrainRules[m_paintTerrain]->m_allowsSeparatedNeighbours) {
        if (point.m_y > 0) {
            TRmgGridPoint nearby(point.m_x, point.m_y - 1);
            if (m_primaryPoints.find(nearby) != m_primaryPoints.end()
                && !isHorizontalGap(nearby)) {
                m_primaryPoints.erase(nearby);
                queueOtherTerrainNeighbours(nearby);
            }
        }
        if (point.m_y < m_size.m_y - 1) {
            TRmgGridPoint nearby(point.m_x, point.m_y + 1);
            if (m_primaryPoints.find(nearby) != m_primaryPoints.end()
                && !isHorizontalGap(nearby)) {
                m_primaryPoints.erase(nearby);
                queueOtherTerrainNeighbours(nearby);
            }
        }
        if (point.m_x > 0) {
            TRmgGridPoint nearby(point.m_x - 1, point.m_y);
            if (m_primaryPoints.find(nearby) != m_primaryPoints.end()
                && !isVerticalGap(nearby)) {
                m_primaryPoints.erase(nearby);
                queueOtherTerrainNeighbours(nearby);
            }
        }
        if (point.m_x < m_size.m_x - 1) {
            TRmgGridPoint nearby(point.m_x + 1, point.m_y);
            if (m_primaryPoints.find(nearby) != m_primaryPoints.end()
                && !isVerticalGap(nearby)) {
                m_primaryPoints.erase(nearby);
                queueOtherTerrainNeighbours(nearby);
            }
        }
    } else {
        unsigned char neighbourExists[TILE_DIR_COUNT];
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

VA(0x005B50F0, 0x34E)
MAC_ADDRESS(0x2565ac, 0x638) // anchor-callee 0x5b4c72, 0x5b50dd; thiscall, ret 4
void rmgTerrainPainter::queueOtherTerrainNeighbours(const TRmgGridPoint& point)
{
    // Retail queues at most one neighbour on each cardinal axis, preferring
    // north over south and west over east. Keep the else-if priority.
    if (point.getY() > 0
        && getTerrain(TRmgGridPoint(point.getX(), point.getY() - 1)) != m_paintTerrain) {
        m_secondaryPoints.insert(TRmgGridPoint(point.getX(), point.getY() - 1));
    } else if (point.getY() < getHeight() - 1
        && getTerrain(TRmgGridPoint(point.getX(), point.getY() + 1)) != m_paintTerrain) {
        m_secondaryPoints.insert(TRmgGridPoint(point.getX(), point.getY() + 1));
    }
    if (point.getX() > 0
        && getTerrain(TRmgGridPoint(point.getX() - 1, point.getY())) != m_paintTerrain) {
        m_secondaryPoints.insert(TRmgGridPoint(point.getX() - 1, point.getY()));
    } else if (point.getX() < getWidth() - 1
        && getTerrain(TRmgGridPoint(point.getX() + 1, point.getY())) != m_paintTerrain) {
        m_secondaryPoints.insert(TRmgGridPoint(point.getX() + 1, point.getY()));
    }
    if (point.getX() > 0 && point.getY() > 0) {
        TRmgGridPoint nearby(point.getX() - 1, point.getY() - 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            m_secondaryPoints.insert(nearby);
    }
    if (point.getX() < getWidth() - 1 && point.getY() > 0) {
        TRmgGridPoint nearby(point.getX() + 1, point.getY() - 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            m_secondaryPoints.insert(nearby);
    }
    if (point.getX() > 0 && point.getY() < getHeight() - 1) {
        TRmgGridPoint nearby(point.getX() - 1, point.getY() + 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            m_secondaryPoints.insert(nearby);
    }
    if (point.getX() < getWidth() - 1 && point.getY() < getHeight() - 1) {
        TRmgGridPoint nearby(point.getX() + 1, point.getY() + 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            m_secondaryPoints.insert(nearby);
    }
}

// Own-terrain checks share the predicates used with the selected paint
// terrain. Retail retains the nested predicate in the horizontal check and expands
// the vertical check at the four adjacent-row/column probes.
unsigned char rmgTerrainPainter::isHorizontalGap(const TRmgGridPoint& point)
{
    return isHorizontalGap(point, getTerrain(point));
}

unsigned char rmgTerrainPainter::isVerticalGap(const TRmgGridPoint& point)
{
    return isVerticalGap(point, getTerrain(point));
}

// The repeated four-check expansion in RepairTerrainPoint and the painter
// worklist decides whether a neighbour itself needs repair. The boundary
// and role are inferred from retail; there is no Dreamcast counterpart.
// Return the separation helper's existing 0/1 result directly after the
// guards. Booleanizing it through &&, != 0, or a final conditional 1/0
// instead changes the two destructors' cmp al,bl into test al,al. This one
// comparison was their final raw-byte difference (549 and 521 bytes).
// An else-if chain preserves those direct byte returns and raises the
// repairTerrainPoint caller from 89.9680% to 90.0961% with no collateral.
// Six predicate forms over ten worklist parents, followed by six helper
// orders over ten joint parents, isolate this gain; keep the original order.
MAC_ADDRESS(0x2588c4, 0x1a0)
unsigned char rmgTerrainPainter::needsTerrainRepair(const TRmgGridPoint& point)
{
    if (isHorizontalGap(point))
        return 1;
    else if (isVerticalGap(point))
        return 1;
    else if (g_rmgTerrainRules[getTerrain(point)]->m_allowsSeparatedNeighbours)
        return 0;
    else
        return hasSeparatedNeighbours(point);
}

// Run-length encode the nonmatching portions of an eight-neighbour ring.
// Start after the first matching cell so wrapped gaps remain one run.
// Cardinal cells weigh two, diagonals one; at most four gaps can exist.
// This cleanup helper returns directly when either scan completes the ring.
static inline unsigned int buildTerrainGaps(
    const unsigned char* matches, TRmgTerrainGap* gaps)
{
    unsigned int firstMatch = 0;
    while (firstMatch < TILE_DIR_COUNT && !matches[firstMatch])
        ++firstMatch;
    if (firstMatch == TILE_DIR_COUNT)
        return 0;

    unsigned int gapCount = 0;
    unsigned int direction = firstMatch;
    for (;;) {
        direction = (direction + 1) % TILE_DIR_COUNT;
        if (direction == firstMatch)
            return gapCount;
        if (!matches[direction]) {
            TRmgTerrainGap& gap = gaps[gapCount++];
            gap.m_weight = 0;
            gap.m_start = direction;
            gap.m_length = 0;
            do {
                gap.m_weight += (direction & 1) ? 1 : 2;
                ++gap.m_length;
                direction = (direction + 1) % TILE_DIR_COUNT;
                if (direction == firstMatch)
                    return gapCount;
            } while (!matches[direction]);
        }
    }
}

// Repair one-cell gaps, then greedily fill the lightest neighbour-ring gaps
// until only one remains. Stable first-minimum selection preserves the retail
// tie order. Painting order matters because each repair updates the worklists.
// Prior matching probes reached 99.1821% with identical CFG and call sequence;
// the remaining difference was an ESI/EDI permutation. This cleanup restores
// structured control flow rather than the old multi-level goto spelling.
VA(0x005B5440, 0x628)
MAC_ADDRESS(0x256be4, 0x630) // anchor-callee 0x5b7358; thiscall, ret 4; retail-only
void rmgTerrainPainter::repairTerrainPoint(const TRmgGridPoint& point)
{
    if (isVerticalGap(point)) {
        if (!needsTerrainRepair(TRmgGridPoint(point.m_x, point.m_y - 1)) &&
            (needsTerrainRepair(TRmgGridPoint(point.m_x, point.m_y + 1)) ||
             (isHorizontalGap(TRmgGridPoint(point.m_x, point.m_y - 1),
                              getPaintTerrain()) &&
              !isHorizontalGap(TRmgGridPoint(point.m_x, point.m_y + 1),
                               getPaintTerrain()))))
            paintPoint(TRmgGridPoint(point.m_x, point.m_y + 1));
        else
            paintPoint(TRmgGridPoint(point.m_x, point.m_y - 1));
    }
    if (isHorizontalGap(point)) {
        if (!needsTerrainRepair(TRmgGridPoint(point.m_x - 1, point.m_y)) &&
            (needsTerrainRepair(TRmgGridPoint(point.m_x + 1, point.m_y)) ||
             (isVerticalGap(TRmgGridPoint(point.m_x - 1, point.m_y),
                            getPaintTerrain()) &&
              !isVerticalGap(TRmgGridPoint(point.m_x + 1, point.m_y),
                             getPaintTerrain()))))
            paintPoint(TRmgGridPoint(point.m_x + 1, point.m_y));
        else
            paintPoint(TRmgGridPoint(point.m_x - 1, point.m_y));
    }

    if (!g_rmgTerrainRules[getPaintTerrain()]->m_allowsSeparatedNeighbours &&
        hasSeparatedNeighbours(point)) {
        unsigned char matches[TILE_DIR_COUNT];
        buildMatchingNeighbourMask(point, matches);
        TRmgTerrainGap gaps[TILE_DIR_COUNT / 2];
        unsigned int gapCount = buildTerrainGaps(matches, gaps);
        unsigned char neighbourExists[TILE_DIR_COUNT];
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
// The caller retains the retail row-major east/south-diagonal visitation order.
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

VA(0x005B5A70, 0x8A7)
MAC_ADDRESS(0x257214, 0xd54)
void rmgTerrainPainter::paintTransitions()
{
    // RMG maps have at least two rows and columns. The retail border scan
    // assumes those dimensions when it reads column 1 and subtracts one.
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

                if (tile.m_frame != newFrame || tile.m_flipX != flip.m_flipX
                    || tile.m_flipY != flip.m_flipY) {
                    tile.m_flipX = flip.m_flipX;
                    tile.m_flipY = flip.m_flipY;
                    tile.m_frame = newFrame;
                    setTile(point, tile);
                }
            } else {
                rmgTerrainTile tile = getPackedCell(point)->getTile();

                int newFrame = selectBaseFrame(point, tile.m_terrain, tile.m_frame);
                if (tile.m_frame != newFrame || tile.m_flipX || tile.m_flipY) {
                    tile.m_frame = newFrame;
                    tile.m_flipX = 0;
                    tile.m_flipY = 0;
                    setTile(point, tile);
                }
            }
        }
    }
}

VA(0x005B6320, 0x107)
MAC_ADDRESS(0x25803c, 0x160)
unsigned char rmgTerrainPainter::isHorizontalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.m_x > 0 && point.m_x < getWidth() - 1
        && getTerrain(TRmgGridPoint(point.getX() - 1, point.getY())) != terrain
        && getTerrain(TRmgGridPoint(point.getX() + 1, point.getY())) != terrain;
}

VA(0x005B6430, 0x106)
MAC_ADDRESS(0x25819c, 0x160)
unsigned char rmgTerrainPainter::isVerticalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.m_y > 0 && point.m_y < getHeight() - 1
        && getTerrain(TRmgGridPoint(point.getX(), point.getY() - 1)) != terrain
        && getTerrain(TRmgGridPoint(point.getX(), point.getY() + 1)) != terrain;
}

// Cardinal neighbours use coordinates clamped to the map edge. A diagonal
// contributes only when at least one adjoining cardinal cell also matches.
// Retail retains the center and four cardinal cache reads, expands the
// diagonals' reads with their fills refused, and expands the south-east
// fill. Residual 99.2138% (77.26% with eight constructed temporaries and
// no corner points). The east read must be refused at the budget its
// remaining sites leave, and the north-west read expanded right after,
// which needs the diagonal region to hold three candidate sites per arm
// and the caller to sit near 1150 units: the clamped coordinates kept as
// two corner points whose accessors feed the diagonal temporaries, and
// one reused point moved through its setters for the cardinal reads, do
// both (constructed cardinal temporaries 92.92%; diagonals through the
// signed-point conversion are one object with this; the reused point for
// the diagonals is unconditional, so it changes the flow, 74.36%; the
// corners constructed straight from the clamps reverse the clamp order,
// 78.32%). Limiting the reused cardinal point to its own scope and copy-
// initializing the two corner values recovers the remaining register homes
// and retail's 0x30 frame: all 714 bytes match. The mask oracle preserves
// ordered, short-circuited queries in 185,856 states; five controls fail.
VA(0x005B6540, 0x2CA)
MAC_ADDRESS(0x2582fc, 0x4f8) // anchor-callee 0x5b58f8, 0x5b681e; retail-only
void rmgTerrainPainter::buildMatchingNeighbourMask(
    const TRmgGridPoint& point, unsigned char* matches)
{
    int terrain = getTerrain(point);
    unsigned int north = point.m_y > 0 ? point.m_y - 1 : point.m_y;
    unsigned int south = point.m_y < m_size.m_y - 1 ? point.m_y + 1 : point.m_y;
    unsigned int west = point.m_x > 0 ? point.m_x - 1 : point.m_x;
    unsigned int east = point.m_x < m_size.m_x - 1 ? point.m_x + 1 : point.m_x;
    TRmgGridPoint northWest = TRmgGridPoint(west, north);
    TRmgGridPoint southEast = TRmgGridPoint(east, south);

    {
        TRmgGridPoint nearby;
        nearby.setX(point.m_x);
        nearby.setY(northWest.getY());
        matches[TILE_DIR_NORTH] = getTerrain(nearby) == terrain;
        nearby.setX(point.m_x);
        nearby.setY(southEast.getY());
        matches[TILE_DIR_SOUTH] = getTerrain(nearby) == terrain;
        nearby.setX(northWest.getX());
        nearby.setY(point.m_y);
        matches[TILE_DIR_WEST] = getTerrain(nearby) == terrain;
        nearby.setX(southEast.getX());
        nearby.setY(point.m_y);
        matches[TILE_DIR_EAST] = getTerrain(nearby) == terrain;
    }
    matches[TILE_DIR_NORTHWEST] =
        (matches[TILE_DIR_NORTH] || matches[TILE_DIR_WEST])
        && getTerrain(TRmgGridPoint(northWest.getX(), northWest.getY())) == terrain;
    matches[TILE_DIR_NORTHEAST] =
        (matches[TILE_DIR_NORTH] || matches[TILE_DIR_EAST])
        && getTerrain(TRmgGridPoint(southEast.getX(), northWest.getY())) == terrain;
    matches[TILE_DIR_SOUTHWEST] =
        (matches[TILE_DIR_SOUTH] || matches[TILE_DIR_WEST])
        && getTerrain(TRmgGridPoint(northWest.getX(), southEast.getY())) == terrain;
    matches[TILE_DIR_SOUTHEAST] =
        (matches[TILE_DIR_SOUTH] || matches[TILE_DIR_EAST])
        && getTerrain(TRmgGridPoint(southEast.getX(), southEast.getY())) == terrain;
}

// Scan cyclic runs: after the first matching run and its following gap,
// another matching cell proves that the centre's neighbours are separated.
VA(0x005B6810, 0x84)
MAC_ADDRESS(0x2587f4, 0xd0)
unsigned char rmgTerrainPainter::hasSeparatedNeighbours(const TRmgGridPoint& point)
{
    unsigned char matches[TILE_DIR_COUNT];
    buildMatchingNeighbourMask(point, matches);
    unsigned int first = 0;
    unsigned int direction;
    while (matches[first]) {
        first = (first + 1) % TILE_DIR_COUNT;
        if (first == 0)
            return 0;
    }
    direction = first;
    do {
        direction = (direction + 1) % TILE_DIR_COUNT;
        if (direction == first)
            return 0;
    } while (!matches[direction]);
    do {
        direction = (direction + 1) % TILE_DIR_COUNT;
        if (direction == first)
            return 0;
    } while (matches[direction]);
    while (!matches[direction]) {
        direction = (direction + 1) % TILE_DIR_COUNT;
        if (direction == first)
            return 0;
    }
    return 1;
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
    unsigned int north = point.m_y > 0 ? point.m_y - 1 : point.m_y;
    unsigned int south = point.m_y < m_size.m_y - 1 ? point.m_y + 1 : point.m_y;
    unsigned int west = point.m_x > 0 ? point.m_x - 1 : point.m_x;
    unsigned int east = point.m_x < m_size.m_x - 1 ? point.m_x + 1 : point.m_x;

    neighbours[TILE_DIR_NORTH] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(point.m_x, north), terrain);
    neighbours[TILE_DIR_SOUTH] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(point.m_x, south), terrain);
    neighbours[TILE_DIR_WEST] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(west, point.m_y), terrain);
    neighbours[TILE_DIR_EAST] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(east, point.m_y), terrain);
    neighbours[TILE_DIR_NORTHWEST] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(west, north), terrain);
    neighbours[TILE_DIR_NORTHEAST] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(east, north), terrain);
    neighbours[TILE_DIR_SOUTHWEST] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(west, south), terrain);
    neighbours[TILE_DIR_SOUTHEAST] = getTerrainNeighbourKindAt(
        *this, TRmgGridPoint(east, south), terrain);
}

// These Complete-only diagonal callers retain the opposite upper-clamp
// operand orientation under canonical tLimit. Spelling the comparison as
// value > maximum makes these two callers exact but regresses the retained
// helper and several Dreamcast-proven limit callers, so keep the shared helper
// canonical and recover the caller-specific compiler state separately.
// Min/max compositions do not recover these bodies.
VA(0x005B6BA0, 0x24C)
MAC_ADDRESS(0x258f18, 0x360)
unsigned char rmgTerrainPainter::checkFirstDiagonal(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    // Retail construction guard byte 0x6a52a0 (tested and set in this body).
    DATA_COMPGEN_GUARD(0x006a52a0, firstDiagonalOffsetsGuard, firstDiagonalOffsets)

    // The guarded initializer registers atexit(0x5b6df0), this table's empty
    // cleanup, placed right after this function.
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
    TRmgGridPoint nearby(
        tLimit(
            0, static_cast<int>(point.getX()) + pair[0].getX(), static_cast<int>(getWidth()) - 1),
        tLimit(
            0, static_cast<int>(point.getY()) + pair[0].getY(), static_cast<int>(getHeight()) - 1));
    if (getTerrain(nearby) == terrain)
        return 1;
    nearby.setX(tLimit(
        0, static_cast<int>(point.getX()) + pair[1].getX(), static_cast<int>(getWidth()) - 1));
    nearby.setY(tLimit(
        0, static_cast<int>(point.getY()) + pair[1].getY(), static_cast<int>(getHeight()) - 1));
    return getTerrain(nearby) == terrain;
}

VA(0x005B6E00, 0x1B3)
MAC_ADDRESS(0x259278, 0x288)
unsigned char rmgTerrainPainter::checkSecondDiagonal(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    // Retail construction guard byte 0x6a3d64 (tested and set in this body).
    DATA_COMPGEN_GUARD(0x006a3d64, secondDiagonalOffsetsGuard, secondDiagonalOffsets)

    // atexit(0x5b6fc0): the table's empty cleanup, right after this function.
    VA_COMPGEN(0x005b6fc0, 0x1, STATIC_DTOR, secondDiagonalOffsets)
    DATA(0x006A3D68)
    static TPoint secondDiagonalOffsets[4] = {
        TPoint(2, 2), TPoint(-2, 2), TPoint(2, -2), TPoint(-2, -2)
    };
    int terrain = getTerrain(point);
    const TPoint& offset = secondDiagonalOffsets[(flip.m_flipY << 1) | flip.m_flipX];
    TRmgGridPoint nearby(
        tLimit(0, static_cast<int>(point.getX()) + offset.getX(), static_cast<int>(getWidth()) - 1), point.getY());
    if (getTerrain(nearby) != terrain)
        return 1;
    nearby.setX(point.getX());
    int maximum = static_cast<int>(getHeight()) - 1;
    int y = static_cast<int>(point.getY()) + offset.getY();
    nearby.setY(tLimit(0, y, maximum));
    return getPackedCell(nearby)->getTerrain() != terrain;
}

VA(0x005B6FD0, 0x271)
MAC_ADDRESS(0x259500, 0x444)
int rmgTerrainPainter::getTransitionStrength(
    const TRmgGridPoint& point, int terrain)
{
    unsigned int strength = m_transitionStrength;
    TRmgTerrainRule* rule = g_rmgTerrainRules[terrain];
    if (point.getX() > 0) {
        TRmgGridPoint nearby(point.getX(), point.getY());
        nearby.setX(point.getX() - 1);
        if (getTerrain(nearby) == terrain
            && rule->isSpecialFrame(getFrame(nearby)))
            strength >>= 1;
    }
    if (point.getY() > 0) {
        TRmgGridPoint nearby(point.getX(), point.getY());
        nearby.setY(point.getY() - 1);
        if (getTerrain(nearby) == terrain
            && rule->isSpecialFrame(getFrame(nearby)))
            strength >>= 1;
    }
    if (point.getX() < getWidth() - 1) {
        TRmgGridPoint nearby(point.getX(), point.getY());
        nearby.setX(point.getX() + 1);
        if (getTerrain(nearby) == terrain
            && rule->isSpecialFrame(getFrame(nearby)))
            strength >>= 1;
    }
    if (point.getY() < getHeight() - 1) {
        TRmgGridPoint nearby(point.getX(), point.getY());
        nearby.setY(point.getY() + 1);
        if (getTerrain(nearby) == terrain
            && rule->isSpecialFrame(getFrame(nearby)))
            strength >>= 1;
    }
    return strength;
}

// The same primary/secondary worklist appears in the brush's terrain change
// and destructor. Preserve one ordinary completion helper and the canonical
// set erase(key); its distance walk expands only at the change site.
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

// Returns the terrain that was being painted. The brush wrapper discards
// it, and retail's brush body has no trace of the load, so the return is
// provisional; what it does prove is this body's inline cost. At cost 43
// (finish plus two stores) the wrapper gives finish 957 units and the
// erase(key) body 234, so the tagged four-argument _Distance expands where
// retail calls it (81.33%). Any byte-neutral cost from 53 up (this form is
// 54; copying both parameters into locals is 53; const-reference
// parameters reach only 47) puts the erase body at 230 and the wrapper's
// tagged callee at 44 < 45, which restores retail's call. finish itself
// is rigid: size()/empty()/!= 0/> 0/this-> spellings all cost 169 and one
// object, set aliases cost 171 and change bytes, direct-initialized or
// iterator-local copies drop its body-saved flag, and moving
// paintTransitions() into both callers leaves the brush destructor at
// 92.14%.
MAC_ADDRESS(0x255ea4, 0x4c)
int rmgTerrainPainter::changeTerrain(int terrain, int strength)
{
    int previous = m_paintTerrain;
    finish();
    m_paintTerrain = terrain;
    m_transitionStrength = strength;
    return previous;
}

VA(0x005B7250, 0x9A)
MAC_ADDRESS(0x2599f8, 0xac) // anchor-callee 0x54017e; allocation and throw RTTI
TRmgTerrainBrush::TRmgTerrainBrush(
    TRmgMapInterface* map, int terrain, int strength)
    : m_painter(new rmgTerrainPainter(map, terrain, strength))
{
    if (!m_painter.get())
        throw TAllocationFailure();
}

VA(0x005B72F0, 0x225)
MAC_ADDRESS(0x259aa4, 0xc8) // anchor-callee 0x540207; auto_ptr ownership cleanup
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
MAC_ADDRESS(0x259cc0, 0x24) // anchor-callee 0x5401e9; four unsigned rectangle args
void TRmgTerrainBrush::paintRectangle(
    unsigned int x, unsigned int y,
    unsigned int rectangleWidth, unsigned int rectangleHeight)
{
    m_painter->paintRectangle(x, y, rectangleWidth, rectangleHeight);
}

// The brush constructor's allocation-failure unwind reaches the retained
// Dinkumware auto_ptr destructor. Its {owns, pointer} layout, pointee
// destructor call, and scalar delete exactly identify this specialization.
VA_COMPGEN(0x005B76D0, 0x20, IMPLICIT_DTOR, rmgTerrainPainter_auto_ptr)

VA(0x005B76F0, 0x209)
MAC_ADDRESS(0x259be4, 0xb8) // anchor-callee 0x5b76e0; retained painter destructor
rmgTerrainPainter::~rmgTerrainPainter()
{
    finish();
}

// The terrain work set's insertion at 0x5b7cd0 calls the admitted grid-point
// comparator and the retained node insertion at 0x5b8720. Both node insertion
// and initialization allocate 0x18-byte nodes and share nil at 0x6a52c4
// (reference count 0x6a52c8). Existing set operations emit all three bodies.
// Initialization and node insertion match all 168/766 bytes.
// Node insertion's _Construct call at 0x5b877c shares the 15-byte two-dword
// copy at 0x5b8cc0 with type_dialog_resource; both emitted bodies agree.
// The concrete-point control reaches 79.83%: VC6 compiles its comparator
// before insertion, elides the lock's EH frame and changes the return tails.
// The generic coordinate owner defers its ordinary template comparator and
// reproduces all 342 bytes, including the lock unwind, without altering STL.
VA_COMPGEN(0x005B7CD0, 0x156, TREE_INSERT, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8670, 0xA8, TREE_INIT, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8720, 0x2FE, TREE_NODE_INSERT, TRmgCoordinatePoint_unsigned_int)

// Public insert's predecessor test calls this node walk; its color field
// at +0x14 and nil references identify the same terrain point-set instance.
// The naturally emitted body matches all 179 bytes.
VA_COMPGEN(0x005B8AA0, 0xB3, TREE_CONST_ITERATOR_DEC, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B7F60, 0x59, TREE_ERASE_KEY, TRmgCoordinatePoint_unsigned_int)

// The retained erase(key) calls 0x5b7e30 with two iterators and a hidden
// result pointer. Its whole-range branch recursively clears nodes through
// 0x5b85f0; its partial-range branch increments then calls 0x5b8090.
// All three share the point tree's nil sentinel at 0x6a52c4. These ordinary
// Dinkumware bodies are naturally emitted by the existing set operations.
// All 289/1295/126 bytes match respectively. The retained _Lockit destructor
// at 0x60b634 releases the CRT lock through LeaveCriticalSection.
VA_COMPGEN(0x005B7E30, 0x121, TREE_ERASE_RANGE, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8090, 0x50F, TREE_ERASE_ITERATOR, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B85F0, 0x7E, TREE_ERASE, TRmgCoordinatePoint_unsigned_int)

// Both erase overloads and the admitted distance loop retain this successor
// walk. Its 0x6a52c4 nil references prove the terrain point-set ownership;
// the naturally emitted TRmgGridPoint specialization matches all 163 bytes.
// This replaces the provisional TPoint claim and its artificial emission
// wrapper in rmg.cpp. The two specializations have distinct nil symbols.
VA_COMPGEN(0x005B8BC0, 0xA3, TREE_CONST_ITERATOR_INC, TRmgCoordinatePoint_unsigned_int)

// The canonical coordinate comparator uses its getX/getY interface. Its
// retained 32-byte body stays exact, and the expanded comparison here now
// loads node-y before key-y as retail does. Direct fields leave 99.4634%.
VA_COMPGEN(0x005B7FC0, 0x57, TREE_FIND, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8020, 0x35, VECTOR_ERASE, TRmgPackedTerrainCell)

VA_COMPGEN(0x005B8060, 0x24, VECTOR_UFILL, unsigned_char)

// PaintPoint and TRmgTerrainBrush::changeTerrain retain this one-dword
// iterator wrapper around the tree's raw-node lower bound.
VA_COMPGEN(0x005B85A0, 0x17, TREE_LOWER_BOUND, TRmgCoordinatePoint_unsigned_int)

// PaintPoint retains the two-bound wrapper returning its iterator pair.
VA_COMPGEN(0x005B85C0, 0x2C, TREE_EQUAL_RANGE, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8A20, 0x17, TREE_UPPER_BOUND, TRmgCoordinatePoint_unsigned_int)

// The retained public wrappers above delegate to these raw-node searches.
VA_COMPGEN(0x005B8A40, 0x59, TREE_LBOUND, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8B60, 0x59, TREE_UBOUND, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B4860, 0x6E, IMPLICIT_DTOR, set)

// erase(key) in TRmgTerrainBrush::changeTerrain retains Dinkumware's
// public distance wrapper and its category-dispatched overload. The wrapper
// increments the caller's count directly; the unused tag argument accounts
// for the tagged body's missing self-store.
VA_COMPGEN(0x005B8C70, 0x2B, STD_DISTANCE, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005B8CD0, 0x28, STD_DISTANCE_TAGGED, TRmgCoordinatePoint_unsigned_int)

template<class Coordinate>
// VA instance: operator< <unsigned int>
VA(0x005B8CA0, 0x20) // anchor-callee 0x5b4e96; fastcall, two point references
bool operator<(const TRmgCoordinatePoint<Coordinate>& left,
    const TRmgCoordinatePoint<Coordinate>& right)
{
    return left.getY() < right.getY() || (left.getY() == right.getY() && left.getX() < right.getX());
}

// Complete terrain data, read from the pinned retail image. The initializer
// calls at 0x5b3b60..0x5b3da0 prove entry counts, arguments and object order;
// the table at 0x642bd8 proves terrain-index order. Names are role-derived.
DATA(0x006424A8)
const TRmgTerrainTransitionEntry g_rmgTerrainPatterns[48] = {
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {8, 0, 0}, {8, 0, 0}, {8, 1, 0}, {8, 1, 0},
    {8, 0, 1}, {8, 0, 1}, {8, 1, 1}, {8, 1, 1}, {9, 0, 0}, {9, 0, 0},
    {9, 1, 0}, {9, 1, 0}, {10, 0, 0}, {10, 0, 0}, {10, 0, 1}, {10, 0, 1},
    {11, 0, 0}, {11, 0, 0}, {11, 1, 0}, {11, 1, 0}, {11, 0, 1}, {11, 0, 1},
    {11, 1, 1}, {11, 1, 1}, {12, 0, 0}, {12, 0, 0}, {12, 1, 0}, {12, 1, 0},
    {12, 0, 1}, {12, 0, 1}, {12, 1, 1}, {12, 1, 1}, {13, 0, 0}, {13, 0, 0},
    {13, 1, 0}, {13, 1, 0}, {13, 0, 1}, {13, 0, 1}, {13, 1, 1}, {13, 1, 1},
};

DATA(0x00642628)
static const TRmgTerrainPatternEntry g_rmgLandPatternEntries[79] = {
    {2, 0}, {2, 0}, {2, 0}, {2, 0}, {3, 0}, {3, 0},
    {3, 0}, {3, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0},
    {5, 0}, {5, 0}, {5, 0}, {5, 0}, {6, 0}, {6, 0},
    {7, 0}, {7, 0}, {8, 0}, {8, 0}, {8, 0}, {8, 0},
    {9, 0}, {9, 0}, {9, 0}, {9, 0}, {10, 0}, {10, 0},
    {10, 0}, {10, 0}, {11, 0}, {11, 0}, {11, 0}, {11, 0},
    {12, 0}, {12, 0}, {13, 0}, {13, 0}, {14, 0}, {15, 0},
    {16, 0}, {17, 0}, {18, 0}, {19, 0}, {20, 0}, {21, 0},
    {22, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
    {0, 0}, {0, 0}, {0, 0}, {0, 1}, {0, 1}, {0, 1},
    {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
    {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
    {0, 1}, {23, 0}, {24, 0}, {25, 0}, {26, 0}, {28, 0},
    {27, 0},
};

DATA(0x006428A0)
static const TRmgTerrainPatternEntry g_rmgDirtPatternEntries[46] = {
    {8, 0}, {8, 0}, {8, 0}, {8, 0}, {9, 0}, {9, 0},
    {9, 0}, {9, 0}, {10, 0}, {10, 0}, {10, 0}, {10, 0},
    {11, 0}, {11, 0}, {11, 0}, {11, 0}, {12, 0}, {12, 0},
    {13, 0}, {13, 0}, {16, 0}, {0, 0}, {0, 0}, {0, 0},
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 1},
    {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
    {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
    {0, 1}, {0, 1}, {0, 1}, {24, 0},
};

DATA(0x00642A10)
static const TRmgTerrainPatternEntry g_rmgSandPatternEntries[24] = {
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
    {0, 0}, {0, 0}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
    {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
    {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
};

DATA(0x00642AD0)
static const TRmgTerrainPatternEntry g_rmgWaterPatternEntries[33] = {
    {8, 0}, {8, 0}, {8, 0}, {8, 0}, {9, 0}, {9, 0},
    {9, 0}, {9, 0}, {10, 0}, {10, 0}, {10, 0}, {10, 0},
    {11, 0}, {11, 0}, {11, 0}, {11, 0}, {12, 0}, {12, 0},
    {13, 0}, {13, 0}, {16, 0}, {0, 0}, {0, 0}, {0, 0},
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
    {0, 0}, {0, 0}, {0, 0},
};

// Retail's nine atexit callbacks load the matching rule into ecx and tail-call
// TRmgTerrainRule::~TRmgTerrainRule (0x005B3850). VC6 emits the same callbacks
// through the implicit derived destructor, whose bytes and vtable relocation
// are identical to the retained base destructor (LINK folds the two bodies).
DATA(0x006A48D0)
static TRmgPatternTerrainRule g_rmgDirtRule(1, 1, 50, 46, g_rmgDirtPatternEntries);

VA_COMPGEN(0x005B3B60, 0x23, STATIC_CTOR, g_rmgDirtRule)

VA_COMPGEN(0x005B3B90, 0x0A, STATIC_DTOR, g_rmgDirtRule)
DATA(0x006A44F8)
static TRmgPatternTerrainRule g_rmgSandRule(0, 1, 70, 24, g_rmgSandPatternEntries);

VA_COMPGEN(0x005B3BA0, 0x23, STATIC_CTOR, g_rmgSandRule)

VA_COMPGEN(0x005B3BD0, 0x0A, STATIC_DTOR, g_rmgSandRule)
DATA(0x006A3D88)
static TRmgPatternTerrainRule g_rmgGrassRule(1, 1, 50, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005B3BE0, 0x23, STATIC_CTOR, g_rmgGrassRule)

VA_COMPGEN(0x005B3C10, 0x0A, STATIC_DTOR, g_rmgGrassRule)
DATA(0x006A3F70)
static TRmgPatternTerrainRule g_rmgSnowRule(1, 1, 80, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005B3C20, 0x23, STATIC_CTOR, g_rmgSnowRule)

VA_COMPGEN(0x005B3C50, 0x0A, STATIC_DTOR, g_rmgSnowRule)
DATA(0x006A46E0)
static TRmgPatternTerrainRule g_rmgSwampRule(1, 1, 80, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005B3C60, 0x23, STATIC_CTOR, g_rmgSwampRule)

VA_COMPGEN(0x005B3C90, 0x0A, STATIC_DTOR, g_rmgSwampRule)
DATA(0x006A4AB8)
static TRmgPatternTerrainRule g_rmgRoughRule(1, 1, 80, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005B3CA0, 0x23, STATIC_CTOR, g_rmgRoughRule)

VA_COMPGEN(0x005B3CD0, 0x0A, STATIC_DTOR, g_rmgRoughRule)
DATA(0x006A5070)
static TRmgPatternTerrainRule g_rmgSubterraneanRule(1, 1, 60, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005B3CE0, 0x23, STATIC_CTOR, g_rmgSubterraneanRule)

VA_COMPGEN(0x005B3D10, 0x0A, STATIC_DTOR, g_rmgSubterraneanRule)
DATA(0x006A4E88)
static TRmgPatternTerrainRule g_rmgLavaRule(1, 1, 80, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005B3D20, 0x23, STATIC_CTOR, g_rmgLavaRule)

VA_COMPGEN(0x005B3D50, 0x0A, STATIC_DTOR, g_rmgLavaRule)
DATA(0x006A4CA0)
static TRmgPatternTerrainRule g_rmgWaterRule(0, 0, 0, 33, g_rmgWaterPatternEntries);

VA_COMPGEN(0x005B3D60, 0x23, STATIC_CTOR, g_rmgWaterRule)

VA_COMPGEN(0x005B3D90, 0x0A, STATIC_DTOR, g_rmgWaterRule)
DATA(0x006A48C8)
static TRmgTableTerrainRule g_rmgRockRule;

// Same owner/atexit and folded base-destructor proof as the pattern rules.
VA_COMPGEN(0x005B3DA0, 0x16, STATIC_CTOR, g_rmgRockRule)

VA_COMPGEN(0x005B3DC0, 0x0A, STATIC_DTOR, g_rmgRockRule)

DATA(0x00642BD8)
TRmgTerrainRule* const g_rmgTerrainRules[10] = {
    &g_rmgDirtRule, &g_rmgSandRule, &g_rmgGrassRule, &g_rmgSnowRule,
    &g_rmgSwampRule, &g_rmgRoughRule, &g_rmgSubterraneanRule,
    &g_rmgLavaRule, &g_rmgWaterRule, &g_rmgRockRule
};
