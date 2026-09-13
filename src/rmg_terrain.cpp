// rmg_terrain.cpp - Complete-only random-map terrain transition support.

// The Dreamcast build contains no random-map generator compiland. Function
// ownership, field layout, helper boundaries, and call/expansion decisions in
// this unit therefore come directly from the retail x86 cluster.
#include <va.h>
#include <stdlib.h>
#include "rmg_terrain.h"
#include "exceptions.h"
#include "tiles.h"
#include "includes.h"

DATA(0x00642BD8) extern TRmgTerrainRule* const g_rmgTerrainRules[];

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
// Its provisional canPaint name is inverted here: nonzero prevents painting.
unsigned char TRmgLinePainterTile::isBlocked()
{
    return m_painter->canPaint(m_point);
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

// Retail 0x4f9f00: preserve the original tile proxy across neighbour queries,
// select an id/flip pair, and draw a random frame only if its pattern or flips
// differ. The eight-direction table starts at north, unlike g_rmgDirections.
// The current tile's terrain field survives; the adapter owns layer updates.
// Initial source retains extra grid-copy calls while expanding the neighbour's
// compound add and proxy call. Its value-return tile getter also makes a
// three-dword copy after the virtual output-reference call; retail has none.
// Explicit tile output, assigned proxy coordinates and assignment-built grid
// translation raise 51.0615% to 63.9923%, preserving every existing exact RMG
// row and improving terrain paintPoint too. All 384 construction/return/query
// combinations were tested; the remaining proxy and arithmetic calls still
// need recovery. Keep their canonical boundaries rather than flattening them.
// Shared grid/proxy controls can restore the += call (70.2846%) but introduce
// unwanted arithmetic calls in terrain painting. Five ordinary operator+
// placements add no gain; parameter-by-value proxy construction also loses.
// The retained calls at 0x4f9f60/0x4f9f77/0x4f9f86 (TPoint add, grid
// conversion, proxy factory) sit where this body's own budget is above
// 700, so retail expands the neighbour query from a nested context: an
// ordinary neighbour-land helper on the painter interface holds the sum,
// the conversion and the factory, and its budget divides by the sites
// after it, which the current tile's frame and flip accessors in the
// pattern test supply (71.92 -> 91.48%; a tile-returning neighbour
// factory 83.55%, no accessors 79.75%, getters and setters together
// 86.74%). Retail then keeps the selected pattern in EDI across the
// rand() call, which the output variable itself cannot do once its
// address has been passed: a plain copy taken after the tile read holds
// it (97.35%; the copy before the read 96.75%, a range reference 94.74%,
// the flip locals declared first 91.5%, register 91.5%). Residual: the
// point and painter parameters take ESI/EDI in the opposite roles from
// retail; a direct proxy construction, reading the land through the
// painter first, building the mask first, and accessor coordinates do
// not swap them (84-97%).
VA(0x004F9F00, 0x146) // anchor-caller 0x4fa080/0x4fa3c0; fastcall, no stack args
void refreshRmgLinePoint(TRmgLinePainterInterface* painter, const TRmgGridPoint& point)
{
    TRmgLinePainterTile tile = painter->at(point);
    int oldType = tile.getLand();
    unsigned char available[TILE_DIR_COUNT];
    buildTileNeighbourMask(painter->m_size.m_x, painter->m_size.m_y,
                           point.m_x, point.m_y, available);
    unsigned char matches[TILE_DIR_COUNT];
    for (unsigned int direction = 0; direction < TILE_DIR_COUNT; ++direction) {
        if (available[direction])
            matches[direction] = painter->getNeighbourLand(point, direction) == oldType;
        else
            matches[direction] = 0;
    }
    TRmgLinePatternTable* table = painter->getPattern(oldType);
    unsigned char flipX, flipY;
    int selected;
    selectRmgLinePattern(matches, table, selected, flipX, flipY);
    rmgTerrainTile current;
    tile.getTile(current);
    int pattern = selected;
    if (table->m_patterns[current.getFrame()] != pattern
        || current.getFlipX() != flipX || current.getFlipY() != flipY) {
        unsigned int frame = table->m_ranges[pattern].m_firstIndex
            + rand() % table->m_ranges[pattern].m_valueCount;
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

VA(0x004FA080, 0x1FB) // anchor-callee 0x4fa42c; fastcall, no stack args
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

VA(0x004FA280, 0x30) // anchor-caller 0x55ee50/0x55f3b0; thiscall ret 0xc
TRmgLineWalker::TRmgLineWalker(
    TRmgLinePainterInterface* newPainter,
    int newRiverType,
    const TRmgGridPoint& start)
    : m_painter(newPainter), m_riverType(newRiverType), m_position(start)
{
    paintPoint(m_position);
}

VA(0x004FA2B0, 0x110) // anchor-caller 0x548040 and createRiver; thiscall ret 4
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

VA(0x004FA3C0, 0x156) // anchor-caller 0x4fa280/0x4fa2b0; thiscall, ret 4
void TRmgLineWalker::paintPoint(const TRmgGridPoint& point)
{
    TRmgLinePainterTile tile(m_painter, point);
    int oldType = tile.getLand();
    if (oldType == m_riverType || tile.isBlocked())
        return;
    if (oldType)
        clearRmgLineRectangle(m_painter, TRmgGridRectangle(point, TRmgGridPoint(1, 1)));
    tile.setOverlay(m_riverType);
    refreshRmgLinePoint(m_painter, point);

    int riverType = m_riverType;
    unsigned char matches[TILE_DIR_COUNT];
    unsigned int direction;
    {
        TRmgLinePainterInterface* painter = m_painter;
        unsigned char available[TILE_DIR_COUNT];
        buildTileNeighbourMask(painter->m_size.m_x, painter->m_size.m_y,
                               point.m_x, point.m_y, available);
        for (direction = 0; direction < TILE_DIR_COUNT; ++direction) {
            if (available[direction])
                matches[direction] = painter->getNeighbourLand(point, direction) == riverType;
            else
                matches[direction] = 0;
        }
    }
    for (direction = 0; direction < TILE_DIR_COUNT; ++direction) {
        if (matches[direction])
            refreshRmgLinePoint(m_painter, point + g_tileDirections[direction]);
    }
}

VA(0x004FA520, 0x16) // anchor-callee 0x4f9f77; thiscall, ret 4
TRmgGridPoint::TRmgGridPoint(const TPoint& point)
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
TRmgPatternTerrainRule::TRmgPatternTerrainRule(
    unsigned char blendsWithOtherTerrain, unsigned char allowsSeparatedNeighbours,
    int defaultFrame, unsigned int entryCount, const TRmgTerrainPatternEntry* entries)
    : TRmgTerrainRule(blendsWithOtherTerrain, allowsSeparatedNeighbours),
      m_defaultFrame(defaultFrame), m_entryCount(entryCount), m_entries(entries)
{
    int frame = m_entries[0].m_frame;
    unsigned char special = m_entries[0].m_special;
    int range = frame * 2 + special;
    ++m_ranges[range].m_count;
    for (unsigned int index = 1; index < m_entryCount; ++index) {
        const TRmgTerrainPatternEntry& entry = m_entries[index];
        if (entry.m_frame != frame || entry.m_special != special) {
            frame = entry.m_frame;
            special = entry.m_special;
            range = frame * 2 + special;
            m_ranges[range].m_firstIndex = index;
        }
        ++m_ranges[range].m_count;
    }
}

// Vtable 0x642c98 slot 1 tests the count for pattern value 1. The constructor
// at 0x5b3780 builds that range at +0x1c/+0x20 from its supplied entry array.
VA(0x005B3840, 0x0C)  // Complete-only pattern terrain rule
unsigned char TRmgPatternTerrainRule::hasEntries()
{
    return 0 < m_ranges[1].m_count;
}

VA(0x005B3850, 0x07)  // terrain-rule deleting destructors; Complete-only
TRmgTerrainRule::~TRmgTerrainRule()
{
}

// Vtable 0x642c98 slot 2 reads the byte at +4 in an eight-byte source entry.
// Constructor 0x5b3780 retains the entry pointer at +0x10 (0x5b37a2).
VA(0x005B3860, 0x11)  // Complete-only pattern terrain rule
unsigned char TRmgPatternTerrainRule::isSpecialFrame(int frame)
{
    return m_entries[frame].m_special;
}

// Each source entry is two dwords. Vtable 0x642c98 slot 3 returns
// the first dword of the requested entry through the pointer at +0x10.
VA(0x005B3880, 0x10)  // Complete-only pattern terrain rule
int TRmgPatternTerrainRule::getEntry(int index)
{
    return m_entries[index].m_frame;
}

// The base-frame selector keeps a zero-tagged old entry. Otherwise it picks
// the secondary range with the rule's strength-scaled percentage, falling
// back to the primary range, then chooses uniformly within that range.
VA(0x005B3890, 0x58)
int TRmgPatternTerrainRule::selectBaseFrame(int value, int oldFrame)
{
    if (oldFrame == -1 || m_entries[oldFrame].m_frame != 0) {
        TRmgTerrainPatternRange* range;
        if (m_ranges[1].m_count > 0) {
            unsigned int chance =
                static_cast<unsigned int>(m_defaultFrame * value) / 8;
            if (static_cast<unsigned int>(rand() % 100) < chance)
                range = &m_ranges[1];
            else
                range = &m_ranges[0];
        } else {
            range = &m_ranges[0];
        }
        oldFrame = rand() % range->m_count + range->m_firstIndex;
    }
    return oldFrame;
}

// Transition ranges are stored as two first/count pairs per transition.
// This selector uses the first pair and preserves an old frame whose entry
// already names the requested transition; the requested flip is copied out.
VA(0x005B38F0, 0x41)
int TRmgPatternTerrainRule::selectTransitionFrame(
    int transition,
    TRmgTerrainFlip requestedFlip,
    TRmgTerrainFlip& selectedFlip,
    int oldFrame)
{
    if (oldFrame == -1 || m_entries[oldFrame].m_frame != transition) {
        TRmgTerrainPatternRange& range = m_ranges[transition * 2];
        oldFrame = rand() % range.m_count + range.m_firstIndex;
    }
    selectedFlip = requestedFlip;
    return oldFrame;
}

DATA(0x006A4158)
TRmgTerrainPatternTable g_rmgTerrainPatternRanges;

VA(0x005B3940, 0xC5)
TRmgTerrainPatternTable::TRmgTerrainPatternTable()
{
    int frame = g_rmgTerrainPatterns[0].m_frame;
    unsigned char flipX = g_rmgTerrainPatterns[0].m_flipX;
    unsigned char flipY = g_rmgTerrainPatterns[0].m_flipY;
    TRmgTerrainPatternRange* range =
        &m_ranges[(frame * 2 + flipX) * 2 + flipY];
    ++range->m_count;
    for (unsigned int index = 1; index < 48; ++index) {
        if (g_rmgTerrainPatterns[index].m_frame != frame || g_rmgTerrainPatterns[index].m_flipX != flipX
            || g_rmgTerrainPatterns[index].m_flipY != flipY) {
            frame = g_rmgTerrainPatterns[index].m_frame;
            flipX = g_rmgTerrainPatterns[index].m_flipX;
            flipY = g_rmgTerrainPatterns[index].m_flipY;
            range = &m_ranges[(frame * 2 + flipX) * 2 + flipY];
            range->m_firstIndex = index;
        }
        ++range->m_count;
    }
}

VA(0x005B3A20, 0x11)  // Complete-only table terrain rule
TRmgTableTerrainRule::TRmgTableTerrainRule()
{
}

VA(0x005B3A40, 0x03)
unsigned char TRmgTableTerrainRule::hasEntries()
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
int TRmgTableTerrainRule::getEntry(int index)
{
    return g_rmgTerrainPatterns[index].m_frame;
}

VA(0x005B3AA0, 0x31)  // vtable 0x642cb0 slot 4; Complete-only table rule
int TRmgTableTerrainRule::selectBaseFrame(int, int oldFrame)
{
    if (oldFrame == -1
        || g_rmgTerrainPatterns[oldFrame].m_frame != 0) {
        oldFrame = rand() % g_rmgTerrainPatternRanges.m_ranges[0].m_count
            + g_rmgTerrainPatternRanges.m_ranges[0].m_firstIndex;
    }
    return oldFrame;
}

VA(0x005B3AE0, 0x74)
int TRmgTableTerrainRule::selectTransitionFrame(
    int transition, TRmgTerrainFlip requestedFlip,
    TRmgTerrainFlip& selectedFlip, int oldFrame)
{
    if (oldFrame == -1
        || g_rmgTerrainPatterns[oldFrame].m_frame != transition
        || g_rmgTerrainPatterns[oldFrame].m_flipX != requestedFlip.m_flipX
        || g_rmgTerrainPatterns[oldFrame].m_flipY != requestedFlip.m_flipY) {
        TRmgTerrainPatternRange& range = g_rmgTerrainPatternRanges.m_ranges[
            (transition * 2 + requestedFlip.m_flipX) * 2 + requestedFlip.m_flipY];
        oldFrame = rand() % range.m_count + range.m_firstIndex;
    }
    selectedFlip = TRmgTerrainFlip(0, 0);
    return oldFrame;
}

// by the call at 0x5b5f5b. All selector names are provisional retail roles.
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

VA(0x005B3E80, 0x75F)  // fastcall call at 0x5b5f5b; retail-only
int __fastcall selectTerrainTransition(
    const int* neighbours, TRmgTerrainFlip* flip)
{
    DATA(0x006A52B8)
    static TRmgTerrainFlip flips[4] = {
        makeTerrainFlip(0, 0), makeTerrainFlip(0, 1),
        makeTerrainFlip(1, 0), makeTerrainFlip(1, 1)
    };
    unsigned int i;
    for (i = 0; i < 4; ++i) {
        const int* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[2]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[4]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            if (neighbours[order[1]] == RMG_NEIGHBOUR_HARD_EDGE &&
                neighbours[order[5]] == RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[i];
                return 28;
            }
            if (neighbours[order[3]] == RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[i];
                return 27;
            }
        }
    }
    for (i = 0; i < 4; ++i) {
        const int* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[0]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[6]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            if (neighbours[order[3]] != RMG_NEIGHBOUR_NO_EDGE) {
                *flip = flips[i];
                return neighbours[order[3]] == RMG_NEIGHBOUR_BLEND_EDGE ? 23 : 25;
            }
        } else if (neighbours[order[0]] == RMG_NEIGHBOUR_HARD_EDGE &&
                   neighbours[order[6]] == RMG_NEIGHBOUR_HARD_EDGE) {
            if (neighbours[order[3]] != RMG_NEIGHBOUR_NO_EDGE) {
                *flip = flips[i];
                return neighbours[order[3]] == RMG_NEIGHBOUR_HARD_EDGE ? 24 : 26;
            }
        }
    }
    for (i = 0; i < 4; ++i) {
        const int* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[2]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[4]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            if (neighbours[order[5]] != RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[i];
                return 21;
            } else {
                *flip = makeTerrainFlip(!flips[i].m_flipX, !flips[i].m_flipY);
                return 8;
            }
        }
        if (neighbours[order[2]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[4]] == RMG_NEIGHBOUR_HARD_EDGE) {
            if (neighbours[order[1]] != RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[i];
                return 22;
            } else {
                *flip = makeTerrainFlip(!flips[i].m_flipX, !flips[i].m_flipY);
                return 8;
            }
        }
    }
    for (i = 0; i < 4; ++i) {
        const int* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[2]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[4]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            if (neighbours[order[5]] == RMG_NEIGHBOUR_HARD_EDGE)
                return 17;
            if (neighbours[order[1]] == RMG_NEIGHBOUR_HARD_EDGE)
                return 18;
        }
    }
    for (i = 0; i < 4; ++i) {
        const int* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[0]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[6]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return 2;
        }
        if (neighbours[order[0]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[6]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return 8;
        }
    }
    for (i = 0; i < 4; ++i) {
        const int* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[2]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[5]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return 17;
        }
        if (neighbours[order[4]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[1]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return 18;
        }
        if (neighbours[order[2]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[5]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return 21;
        }
        if (neighbours[order[4]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[1]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return 22;
        }
        if ((neighbours[order[6]] == RMG_NEIGHBOUR_BLEND_EDGE &&
             neighbours[order[1]] == RMG_NEIGHBOUR_BLEND_EDGE) ||
            (neighbours[order[0]] == RMG_NEIGHBOUR_BLEND_EDGE &&
             neighbours[order[5]] == RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[i];
            return 2;
        }
        if ((neighbours[order[6]] == RMG_NEIGHBOUR_HARD_EDGE &&
             neighbours[order[1]] == RMG_NEIGHBOUR_HARD_EDGE) ||
            (neighbours[order[0]] == RMG_NEIGHBOUR_HARD_EDGE &&
             neighbours[order[5]] == RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[i];
            return 8;
        }
    }
    for (i = 0; i < 4; ++i) {
        const int* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[2]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[3]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return 19;
        }
        if (neighbours[order[4]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[3]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return 20;
        }
    }
    for (i = 0; i < 4; ++i) {
        const int* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[0]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return 4;
        }
        if (neighbours[order[0]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return 10;
        }
        if (neighbours[order[6]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return 3;
        }
        if (neighbours[order[6]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return 9;
        }
    }
    for (i = 0; i < 4; ++i) {
        const int* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[7]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[3]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return 14;
        }
        if (neighbours[order[7]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[3]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return 15;
        }
        if (neighbours[order[7]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[3]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return 16;
        }
    }
    for (i = 0; i < 4; ++i) {
        const int* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[3]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return 5;
        }
        if (neighbours[order[3]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return 11;
        }
    }
    *flip = makeTerrainFlip(0, 0);
    return 0;
}

VA(0x005B45F0, 0x26D)
rmgTerrainPainter::rmgTerrainPainter(
    TRmgMapInterface* newAdapter, int terrain, int strength)
    : m_adapter(newAdapter), m_paintTerrain(terrain), m_transitionStrength(strength)
{
    m_size = m_adapter->getSize();
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

// The base-tile block of paintPoint as paintRectangle's own helper: its one
// site is what paintRectangle's terrain test needs (see there), while
// paintPoint expands the same three operations from its own block.
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
int rmgTerrainPainter::getPaintTerrain() const
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

VA(0x005B4B20, 0x5CB) // anchor-callee 0x5b4960, 0x5b5440; thiscall, ret 4
void rmgTerrainPainter::paintPoint(const TRmgGridPoint& point)
{
    {
        int frame = selectBaseFrame(point, m_paintTerrain, -1);
        setTile(point, rmgTerrainTile(m_paintTerrain, frame));
    }

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

VA(0x005B50F0, 0x34E) // anchor-callee 0x5b4c72, 0x5b50dd; thiscall, ret 4
void rmgTerrainPainter::queueOtherTerrainNeighbours(const TRmgGridPoint& point)
{
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

// Repair a one-cell terrain gap, then merge all but the largest remaining
// gap in the neighbour ring. Cardinal directions have weight two and
// diagonals weight one. The worklist at 0x5b72f0 passes its first primary
// point to this method; PaintPoint consumes the resulting repairs.
// Separate paint statements let the preceding predicate temporaries expire:
// retail reuses EBP-0x10 at +0x32 and +0xbd. Together with canonical dimension
// and paint-terrain accessors, this raises MAX 82.8347 -> 89.7150%. At that
// checkpoint all cache reads retained getPackedCell, but two vertical-gap
// second-coordinate constructors over-inlined. Retail retains copied x at +0x5bb.
// Controls: separate statements alone 80.9056%; dimensions alone 84.5649%;
// together 89.5970%. A positive-guard rewrite is byte-identical; a byte choice
// local introduces extra result/branch code (87.1956%). Explicit helper edge
// returns do not restore the missing constructors (89.3019%). Returning a
// named point after += is byte-flat; copy-constructing it changes the load
// order (89.8583% scratch) but still omits retail's copied-x store. Retain the
// coordinate construction supported by paintPoint's separate retail evidence.
// The neighbour-ring scan uses an explicit while(1) header and a word-sized
// diagonal local: retail +0x4de has a single advance/test header and +0x511
// copies the full direction before masking its low bit. This raises 90.0961%
// to 91.3390% and restores the complete retail CFG, without TU collateral.
// Sixty outer-loop/receiver/width controls and six inner-loop forms over the
// ten best parents retain the original inner do loop. A top-tested for(;;),
// explicit header label, do(1), or assignment-condition while still rotates
// the outer test; reference/pointer gap receivers also lose code agreement.
// Retail +0x52d leaves both loops directly. A compound inner do condition
// followed by another test duplicates this comparison and rotates the exit.
// At 91.3390%, the only remaining call-sequence difference is the coordinate
// constructor in the final vertical-gap expansion (retail +0x449); the other
// 45 calls agree. The paired gap-point lifetime controls below do not fix it.
// Coordinate-assigned operator+ temporaries reach 92.3508% with all exact
// siblings intact, but keep that same missing constructor and lower both
// terrain paintPoint and line refresh. This is not a resolved call boundary.
// Goto audit: the outer cycle exit is a byte-neutral break. Replacing the
// inner exit with break plus an outer equality test loses 5.9713 points;
// a bottom-tested outer cycle loses 6.6914. The multi-level exit remains.
// Inner break followed by the outer cycle-completion test scores 85.3676%
// versus 91.3389%; it preserves gap construction but changes the loop CFG.
// A cycle-complete result with either a loop-head test or bottom break
// scores 86.0995% or 86.1619% for bool/byte/int, below 91.3389%.
VA(0x005B5440, 0x628) // anchor-callee 0x5b7358; thiscall, ret 4; retail-only
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
        unsigned int gapCount = 0;
        unsigned int first = 0;
        while (!matches[first])
            ++first;

        unsigned int direction = first;
        while (1) {
            direction = (direction + 1) % TILE_DIR_COUNT;
            if (direction == first)
                break;
            if (!matches[direction]) {
                unsigned int currentGap = gapCount++;
                gaps[currentGap].m_weight = 0;
                gaps[currentGap].m_start = direction;
                gaps[currentGap].m_length = 0;
                do {
                    unsigned int diagonal = direction & 1;
                    gaps[currentGap].m_weight += diagonal ? 1 : 2;
                    ++gaps[currentGap].m_length;
                    direction = (direction + 1) % TILE_DIR_COUNT;
                    if (direction == first)
                        goto gapsBuilt;
                } while (!matches[direction]);
            }
        }

    gapsBuilt:
        unsigned char neighbourExists[TILE_DIR_COUNT];
        buildTileNeighbourMask(getWidth(), getHeight(), point.m_x, point.m_y,
                               neighbourExists);
        do {
            unsigned int smallest = 0;
            unsigned int smallestWeight = gaps[0].m_weight;
            {
                for (unsigned int gap = 1; gap < gapCount; ++gap) {
                    if (gaps[gap].m_weight < smallestWeight) {
                        smallest = gap;
                        smallestWeight = gaps[gap].m_weight;
                    }
                }
            }
            unsigned int end =
                (gaps[smallest].m_start + gaps[smallest].m_length) % TILE_DIR_COUNT;
            for (direction = gaps[smallest].m_start; direction != end;
                 direction = (direction + 1) % TILE_DIR_COUNT) {
                if (neighbourExists[direction])
                    paintPoint(point + g_tileDirections[direction]);
            }
            --gapCount;
            for (unsigned int gap = smallest; gap < gapCount; ++gap)
                gaps[gap] = gaps[gap + 1];
        } while (gapCount > 1);
    }
}

VA(0x005B5A70, 0x8A7)
void rmgTerrainPainter::paintTransitions()
{
    std::vector<unsigned char> edgeCounts(getWidth() * getHeight());
    TRmgGridPoint point;

    for (point.setY(0); point.m_y < getHeight() - 1; point.setY(point.getY() + 1)) {
        int terrain = getTerrain(TRmgGridPoint(0, point.m_y));

        if (getTerrain(TRmgGridPoint(1, point.m_y)) != terrain) {
            ++edgeCounts[point.getY() * getWidth()];
            ++edgeCounts[point.getY() * getWidth() + 1];
        }
        if (getTerrain(TRmgGridPoint(1, point.m_y + 1)) != terrain) {
            ++edgeCounts[point.getY() * getWidth()];
            ++edgeCounts[(point.getY() + 1) * getWidth() + 1];
        }
        if (getTerrain(TRmgGridPoint(0, point.m_y + 1)) != terrain) {
            ++edgeCounts[point.getY() * getWidth()];
            ++edgeCounts[(point.getY() + 1) * getWidth()];
        }

        for (point.setX(1); point.m_x < getWidth() - 1; point.setX(point.getX() + 1)) {
            terrain = getTerrain(point);

            TRmgGridPoint east(point.getX() + 1, point.getY());
            if (getTerrain(east) != terrain) {
                ++edgeCounts[point.getY() * getWidth() + point.getX()];
                ++edgeCounts[point.getY() * getWidth() + point.getX() + 1];
            }
            TRmgGridPoint southEast(point.getX() + 1, point.getY() + 1);
            if (getTerrain(southEast) != terrain) {
                ++edgeCounts[point.getY() * getWidth() + point.getX()];
                ++edgeCounts[(point.getY() + 1) * getWidth() + point.getX() + 1];
            }
            TRmgGridPoint south(point.getX(), point.getY() + 1);
            if (getTerrain(south) != terrain) {
                ++edgeCounts[point.getY() * getWidth() + point.getX()];
                ++edgeCounts[(point.getY() + 1) * getWidth() + point.getX()];
            }
            TRmgGridPoint southWest(point.getX() - 1, point.getY() + 1);
            if (getTerrain(southWest) != terrain) {
                ++edgeCounts[point.getY() * getWidth() + point.getX()];
                ++edgeCounts[(point.getY() + 1) * getWidth() + point.getX() - 1];
            }
        }

        terrain = getTerrain(point);
        TRmgGridPoint south(point.getX(), point.getY() + 1);
        if (getTerrain(south) != terrain) {
            ++edgeCounts[point.getY() * getWidth() + point.getX()];
            ++edgeCounts[(point.getY() + 1) * getWidth() + point.getX()];
        }
        TRmgGridPoint southWest(point.getX() - 1, point.getY() + 1);
        if (getTerrain(southWest) != terrain) {
            ++edgeCounts[point.getY() * getWidth() + point.getX()];
            ++edgeCounts[(point.getY() + 1) * getWidth() + point.getX() - 1];
        }
    }

    for (point.setX(0); point.m_x < getWidth() - 1; point.setX(point.getX() + 1)) {
        int terrain = getTerrain(point);
        TRmgGridPoint east(point.getX() + 1, point.getY());
        if (getTerrain(east) != terrain) {
            ++edgeCounts[point.getY() * getWidth() + point.getX()];
            ++edgeCounts[point.getY() * getWidth() + point.getX() + 1];
        }
    }

    for (point.setY(0); point.m_y < getHeight(); point.setY(point.getY() + 1)) {
        for (point.setX(0); point.m_x < getWidth(); point.setX(point.getX() + 1)) {
            unsigned int index = point.getY() * getWidth() + point.getX();

            if (edgeCounts[index] > 0) {
                int neighbours[8];
                buildNeighbourKinds(point, neighbours);

                int transition;
                TRmgTerrainFlip flip;
                transition = selectTerrainTransition(neighbours, &flip);
                if (transition == RMG_TERRAIN_FIRST_DIAGONAL_LOW) {
                    if (checkFirstDiagonal(point, flip))
                        transition = 6;
                } else if (transition == RMG_TERRAIN_FIRST_DIAGONAL_HIGH) {
                    if (checkFirstDiagonal(point, flip))
                        transition = 12;
                } else if (transition == RMG_TERRAIN_SECOND_DIAGONAL_LOW) {
                    if (checkSecondDiagonal(point, flip))
                        transition = 7;
                } else if (transition == RMG_TERRAIN_SECOND_DIAGONAL_HIGH) {
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
unsigned char rmgTerrainPainter::isHorizontalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.m_x > 0 && point.m_x < getWidth() - 1
        && getTerrain(TRmgGridPoint(point.m_x - 1, point.m_y)) != terrain
        && getTerrain(TRmgGridPoint(point.m_x + 1, point.m_y)) != terrain;
}

VA(0x005B6430, 0x106)
unsigned char rmgTerrainPainter::isVerticalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.m_y > 0 && point.m_y < getHeight() - 1
        && getTerrain(TRmgGridPoint(point.m_x, point.m_y - 1)) != terrain
        && getTerrain(TRmgGridPoint(point.m_x, point.m_y + 1)) != terrain;
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
// 78.32%). Remaining: the south-east fill's index and cell-base registers
// swap roles and the frame is 0x38 against 0x30.
VA(0x005B6540, 0x2CA) // anchor-callee 0x5b58f8, 0x5b681e; retail-only
void rmgTerrainPainter::buildMatchingNeighbourMask(
    const TRmgGridPoint& point, unsigned char* matches)
{
    int terrain = getTerrain(point);
    unsigned int north = point.m_y > 0 ? point.m_y - 1 : point.m_y;
    unsigned int south = point.m_y < m_size.m_y - 1 ? point.m_y + 1 : point.m_y;
    unsigned int west = point.m_x > 0 ? point.m_x - 1 : point.m_x;
    unsigned int east = point.m_x < m_size.m_x - 1 ? point.m_x + 1 : point.m_x;
    TRmgGridPoint low(west, north);
    TRmgGridPoint high(east, south);

    TRmgGridPoint nearby;
    nearby.setX(point.m_x);
    nearby.setY(low.getY());
    matches[TILE_DIR_NORTH] = getTerrain(nearby) == terrain;
    nearby.setX(point.m_x);
    nearby.setY(high.getY());
    matches[TILE_DIR_SOUTH] = getTerrain(nearby) == terrain;
    nearby.setX(low.getX());
    nearby.setY(point.m_y);
    matches[TILE_DIR_WEST] = getTerrain(nearby) == terrain;
    nearby.setX(high.getX());
    nearby.setY(point.m_y);
    matches[TILE_DIR_EAST] = getTerrain(nearby) == terrain;
    matches[TILE_DIR_NORTHWEST] =
        (matches[TILE_DIR_NORTH] || matches[TILE_DIR_WEST])
        && getTerrain(TRmgGridPoint(low.getX(), low.getY())) == terrain;
    matches[TILE_DIR_NORTHEAST] =
        (matches[TILE_DIR_NORTH] || matches[TILE_DIR_EAST])
        && getTerrain(TRmgGridPoint(high.getX(), low.getY())) == terrain;
    matches[TILE_DIR_SOUTHWEST] =
        (matches[TILE_DIR_SOUTH] || matches[TILE_DIR_WEST])
        && getTerrain(TRmgGridPoint(low.getX(), high.getY())) == terrain;
    matches[TILE_DIR_SOUTHEAST] =
        (matches[TILE_DIR_SOUTH] || matches[TILE_DIR_EAST])
        && getTerrain(TRmgGridPoint(high.getX(), high.getY())) == terrain;
}

VA(0x005B6810, 0x84)
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
        if (direction == first) {
noSeparation:
            return 0;
        }
    } while (!matches[direction]);
    do {
        direction = (direction + 1) % TILE_DIR_COUNT;
        if (direction == first)
            goto noSeparation;
    } while (matches[direction]);
    while (!matches[direction]) {
        direction = (direction + 1) % TILE_DIR_COUNT;
        if (direction == first)
            goto noSeparation;
    }
    return 1;
}

VA(0x005B68A0, 0x2FF)
void rmgTerrainPainter::buildNeighbourKinds(
    const TRmgGridPoint& point, int* neighbours)
{
    int terrain = getTerrain(point);
    unsigned int north = point.m_y > 0 ? point.m_y - 1 : point.m_y;
    unsigned int south = point.m_y < m_size.m_y - 1 ? point.m_y + 1 : point.m_y;
    unsigned int west = point.m_x > 0 ? point.m_x - 1 : point.m_x;
    unsigned int east = point.m_x < m_size.m_x - 1 ? point.m_x + 1 : point.m_x;

    {
        TRmgGridPoint nearby(point.m_x, north);
        int nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_NORTH] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(point.m_x, south);
        int nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_SOUTH] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(west, point.m_y);
        int nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_WEST] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(east, point.m_y);
        int nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_EAST] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(west, north);
        int nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_NORTHWEST] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(east, north);
        int nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_NORTHEAST] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(west, south);
        int nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_SOUTHWEST] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(east, south);
        int nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_SOUTHEAST] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
}

VA(0x005B6BA0, 0x24C)
unsigned char rmgTerrainPainter::checkFirstDiagonal(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    DATA(0x006A5260)
    static TPoint offsets[4][2] = {
        { TPoint(-1, 1), TPoint(1, -1) },
        { TPoint(1, 1), TPoint(-1, -1) },
        { TPoint(-1, -1), TPoint(1, 1) },
        { TPoint(1, -1), TPoint(-1, 1) }
    };
    int terrain = getTerrain(point);
    const TPoint* pair = offsets[(flip.m_flipY << 1) | flip.m_flipX];
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
unsigned char rmgTerrainPainter::checkSecondDiagonal(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    DATA(0x006A3D68)
    static TPoint offsets[4] = {
        TPoint(2, 2), TPoint(-2, 2), TPoint(2, -2), TPoint(-2, -2)
    };
    int terrain = getTerrain(point);
    const TPoint& offset = offsets[(flip.m_flipY << 1) | flip.m_flipX];
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
int rmgTerrainPainter::changeTerrain(int terrain, int strength)
{
    int previous = m_paintTerrain;
    finish();
    m_paintTerrain = terrain;
    m_transitionStrength = strength;
    return previous;
}

VA(0x005B7250, 0x9A) // anchor-callee 0x54017e; allocation and throw RTTI
TRmgTerrainBrush::TRmgTerrainBrush(
    TRmgMapInterface* map, int terrain, int strength)
    : m_painter(new rmgTerrainPainter(map, terrain, strength))
{
    if (!m_painter.get())
        throw TAllocationFailure();
}

VA(0x005B72F0, 0x225) // anchor-callee 0x540207; auto_ptr ownership cleanup
TRmgTerrainBrush::~TRmgTerrainBrush()
{
}

VA(0x005B7520, 0x16A)
void TRmgTerrainBrush::changeTerrain(int terrain, int strength)
{
    m_painter->changeTerrain(terrain, strength);
}

VA(0x005B7690, 0x1F) // anchor-callee 0x5401e9; four unsigned rectangle args
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

VA(0x005B76F0, 0x209) // anchor-callee 0x5b76e0; retained painter destructor
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
// Public insert reaches 79.83%: its named call sequence agrees, but VC6 elides the lock
// scope's EH frame and emits four returns where retail shares one tail.
// Preserve the canonical Dinkumware implementation and real comparator.
VA_COMPGEN(0x005B7CD0, 0x156, TREE_INSERT, TRmgGridPoint)
VA_COMPGEN(0x005B8670, 0xA8, TREE_INIT, TRmgGridPoint)
VA_COMPGEN(0x005B8720, 0x2FE, TREE_NODE_INSERT, TRmgGridPoint)
// Public insert's predecessor test calls this node walk; its color field
// at +0x14 and nil references identify the same terrain point-set instance.
// The naturally emitted body matches all 179 bytes.
VA_COMPGEN(0x005B8AA0, 0xB3, TREE_CONST_ITERATOR_DEC, TRmgGridPoint)

VA_COMPGEN(0x005B7F60, 0x59, TREE_ERASE_KEY, TRmgGridPoint)

// The retained erase(key) calls 0x5b7e30 with two iterators and a hidden
// result pointer. Its whole-range branch recursively clears nodes through
// 0x5b85f0; its partial-range branch increments then calls 0x5b8090.
// All three share the point tree's nil sentinel at 0x6a52c4. These ordinary
// Dinkumware bodies are naturally emitted by the existing set operations.
// All 289/1295/126 bytes match respectively. The retained _Lockit destructor
// at 0x60b634 releases the CRT lock through LeaveCriticalSection.
VA_COMPGEN(0x005B7E30, 0x121, TREE_ERASE_RANGE, TRmgGridPoint)
VA_COMPGEN(0x005B8090, 0x50F, TREE_ERASE_ITERATOR, TRmgGridPoint)
VA_COMPGEN(0x005B85F0, 0x7E, TREE_ERASE, TRmgGridPoint)

// Both erase overloads and the admitted distance loop retain this successor
// walk. Its 0x6a52c4 nil references prove the terrain point-set ownership;
// the naturally emitted TRmgGridPoint specialization matches all 163 bytes.
// This replaces the provisional TPoint claim and its artificial emission
// wrapper in rmg.cpp. The two specializations have distinct nil symbols.
VA_COMPGEN(0x005B8BC0, 0xA3, TREE_CONST_ITERATOR_INC, TRmgGridPoint)

VA_COMPGEN(0x005B7FC0, 0x57, TREE_FIND, TRmgGridPoint)

VA_COMPGEN(0x005B8020, 0x35, VECTOR_ERASE, TRmgPackedTerrainCell)

VA_COMPGEN(0x005B8060, 0x24, VECTOR_UFILL, unsigned_char)

// PaintPoint and TRmgTerrainBrush::changeTerrain retain this one-dword
// iterator wrapper around the tree's raw-node lower bound.
VA_COMPGEN(0x005B85A0, 0x17, TREE_LOWER_BOUND, TRmgGridPoint)
// PaintPoint retains the two-bound wrapper returning its iterator pair.
VA_COMPGEN(0x005B85C0, 0x2C, TREE_EQUAL_RANGE, TRmgGridPoint)
VA_COMPGEN(0x005B8A20, 0x17, TREE_UPPER_BOUND, TRmgGridPoint)

// The retained public wrappers above delegate to these raw-node searches.
VA_COMPGEN(0x005B8A40, 0x59, TREE_LBOUND, TRmgGridPoint)
VA_COMPGEN(0x005B8B60, 0x59, TREE_UBOUND, TRmgGridPoint)

VA_COMPGEN(0x005B4860, 0x6E, IMPLICIT_DTOR, set)

// erase(key) in TRmgTerrainBrush::changeTerrain retains Dinkumware's
// public distance wrapper and its category-dispatched overload. The wrapper
// increments the caller's count directly; the unused tag argument accounts
// for the tagged body's missing self-store.
VA_COMPGEN(0x005B8C70, 0x2B, STD_DISTANCE, TRmgGridPoint)
VA_COMPGEN(0x005B8CD0, 0x28, STD_DISTANCE_TAGGED, TRmgGridPoint)

VA(0x005B8CA0, 0x20) // anchor-callee 0x5b4e96; fastcall, two point references
bool operator<(const TRmgGridPoint& left, const TRmgGridPoint& right)
{
    return left.m_y < right.m_y || (left.m_y == right.m_y && left.m_x < right.m_x);
}
