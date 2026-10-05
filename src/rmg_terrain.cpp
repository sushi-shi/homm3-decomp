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


// oldFrame value of a cell painted from scratch.
enum ERmgTerrainFrameSentinel {
    RMG_NO_TERRAIN_FRAME = -1
};

// Frames in the fixed (rock) transition table.
enum ERmgFixedTransitionTableLimits {
    RMG_FIXED_TRANSITION_FRAME_COUNT = 48
};

// One-cell (offsetX, offsetY) steps toward a neighbour; x grows east and y
// grows south.
enum ERmgNeighbourStep {
    RMG_STEP_NONE = 0,
    RMG_STEP_WEST = -1,
    RMG_STEP_EAST = 1,
    RMG_STEP_NORTH = -1,
    RMG_STEP_SOUTH = 1
};


// Vtable 0x642cb0 slot 2 shares the false/ret-4 body at 0x5543f0.
MAC_ADDRESS(0x259d44, 0x8)
b8 TRmgTableTerrainRule::isSpecialFrame(s32) { return 0; }

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
TRmgGridRectangle::TRmgGridRectangle(const TRmgGridPoint& origin, const TRmgGridPoint& size)
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
void refreshRmgLinePoint(TRmgLinePainterInterface* painter, const TRmgGridPoint& point)
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
// Mac 0x2223a0 also wraps the complete matching-neighbour-mask loops used
// by refresh and the walker. Restoring that outer boundary currently retains
// extra grid conversion/addition calls in refresh and grid-constructor/proxy
// calls in the walker (84.8077% / 78.8527%); its source calls need joint recovery.
s32 TRmgLinePainterInterface::getNeighbourLineType(const TRmgGridPoint& point, u32 direction)
{
    TRmgGridPoint nearby = point + g_tileDirections[direction];
    return at(nearby).getLineType();
}

// Clears the rectangle, then refreshes the line tiles around it. North is up;
// # is the rectangle, digits the order in which its sides are refreshed.
//   1 3 3 3 2
//   1 # # # 2
//   1 4 4 4 2
VA(0x004fa080, 0x1fb)
MAC_ADDRESS(0x2228b8, 0x388) // anchor-callee 0x4fa42c; fastcall, no stack args
void clearRmgLineRectangle(TRmgLinePainterInterface* painter, const TRmgGridRectangle& rectangle)
{
    TRmgGridPoint point;
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
    TRmgLinePainterInterface* newPainter,
    s32 newLineType,
    const TRmgGridPoint& start)
    : m_painter(newPainter), m_lineType(newLineType), m_position(start)
{
    paintPoint(m_position);
}

VA(0x004fa2b0, 0x110)
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

VA(0x004fa3c0, 0x156)
MAC_ADDRESS(0x222e1c, 0x18c) // anchor-caller 0x4fa280/0x4fa2b0; thiscall, ret 4
void TRmgLineWalker::paintPoint(const TRmgGridPoint& point)
{
    TRmgLinePainterTile tile(m_painter, point);
    s32 oldType = tile.getLineType();
    if (oldType == m_lineType || tile.isBlocked())
        return;
    if (oldType)
        clearRmgLineRectangle(m_painter, TRmgGridRectangle(point, TRmgGridPoint(1, 1)));
    tile.setLineType(m_lineType);
    refreshRmgLinePoint(m_painter, point);

    s32 lineType = m_lineType;
    b8 matches[TILE_DIR_COUNT];
    u32 direction;
    {
        TRmgLinePainterInterface* painter = m_painter;
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
            refreshRmgLinePoint(m_painter, point + g_tileDirections[direction]);
    }
}

template<class Coordinate>
// VA instance: TRmgCoordinatePoint<u32>::TRmgCoordinatePoint(const TPoint&)
VA(0x004fa520, 0x16)
MAC_ADDRESS(0x2228a4, 0x14) // anchor-callee 0x4f9f77; thiscall, ret 4
TRmgCoordinatePoint<Coordinate>::TRmgCoordinatePoint(const TPoint& point)
    : m_x(point.m_x), m_y(point.m_y)
{
}

VA(0x004fa540, 0x21) // anchor-callers 0x4f9f00/0x4fa3c0; thiscall, ret 4
TPoint& TPoint::operator+=(const TPoint& offset)
{
    m_x += offset.m_x;
    m_y += offset.m_y;
    return *this;
}

VA(0x005b3780, 0xb3)
MAC_ADDRESS(0x254be0, 0xf4)
TRmgPatternTerrainRule::TRmgPatternTerrainRule(
    b8 blendsWithOtherTerrain, b8 allowsSeparatedNeighbours,
    s32 specialFrameChance, u32 entryCount, const TRmgTerrainPatternEntry* entries)
    : TRmgTerrainRule(blendsWithOtherTerrain, allowsSeparatedNeighbours),
      m_specialFrameChance(specialFrameChance), m_entryCount(entryCount), m_entries(entries)
{
    s32 transition = m_entries[0].m_transition;
    b8 special = m_entries[0].m_special;
    s32 range = transition * 2 + special;
    ++m_ranges[range].m_frameCount;
    for (u32 index = 1; index < m_entryCount; ++index) {
        const TRmgTerrainPatternEntry& entry = m_entries[index];
        if (entry.m_transition != transition || entry.m_special != special) {
            transition = entry.m_transition;
            special = entry.m_special;
            range = transition * 2 + special;
            m_ranges[range].m_firstFrame = index;
        }
        ++m_ranges[range].m_frameCount;
    }
}

// Tests whether the special base-frame range is nonempty. The constructor
// at 0x5b3780 builds that range at +0x1c/+0x20 from its supplied entry array.
VA(0x005b3840, 0x0c)
MAC_ADDRESS(0x259dac, 0x14)  // Complete-only pattern terrain rule
b8 TRmgPatternTerrainRule::hasSpecialBaseFrames()
{
    return 0 < m_ranges[1].m_frameCount;
}

VA(0x005b3850, 0x07)
MAC_ADDRESS(0x254b98, 0x48)  // terrain-rule deleting destructors; Complete-only
TRmgTerrainRule::~TRmgTerrainRule()
{
}

// Vtable 0x642c98 slot 2 reads the byte at +4 in an eight-byte source entry.
// Constructor 0x5b3780 retains the entry pointer at +0x10 (0x5b37a2).
VA(0x005b3860, 0x11)
MAC_ADDRESS(0x254ce4, 0x14)  // Complete-only pattern terrain rule
b8 TRmgPatternTerrainRule::isSpecialFrame(s32 frame)
{
    return m_entries[frame].m_special;
}

// Each source entry is two dwords. Vtable 0x642c98 slot 3 returns
// the first dword of the requested entry through the pointer at +0x10.
VA(0x005b3880, 0x10)
MAC_ADDRESS(0x254cf8, 0x10)  // Complete-only pattern terrain rule
s32 TRmgPatternTerrainRule::getTransition(s32 frame)
{
    return m_entries[frame].m_transition;
}

// The base-frame selector keeps a zero-tagged old entry. Otherwise it picks
// the secondary range with the rule's strength-scaled percentage, falling
// back to the primary range, then chooses uniformly within that range.
VA(0x005b3890, 0x58)
MAC_ADDRESS(0x254d08, 0xc8)
s32 TRmgPatternTerrainRule::selectBaseFrame(s32 strength, s32 oldFrame)
{
    if (oldFrame == -1 || m_entries[oldFrame].m_transition != 0) {
        TRmgTerrainPatternRange* range;
        if (m_ranges[1].m_frameCount > 0) {
            u32 chance =
                static_cast<u32>(m_specialFrameChance * strength) / 8;
            if (static_cast<u32>(rand() % 100) < chance)
                range = &m_ranges[1];
            else
                range = &m_ranges[0];
        } else {
            range = &m_ranges[0];
        }
        oldFrame = rand() % range->m_frameCount + range->m_firstFrame;
    }
    return oldFrame;
}

// Transition ranges are stored as two first/count pairs per transition.
// This selector uses the first pair and preserves an old frame whose entry
// already names the requested transition; the requested flip is copied out.
VA(0x005b38f0, 0x41)
MAC_ADDRESS(0x254dd0, 0x88)
s32 TRmgPatternTerrainRule::selectTransitionFrame(
    s32 transition,
    TRmgTerrainFlip requestedFlip,
    TRmgTerrainFlip& selectedFlip,
    s32 oldFrame)
{
    if (oldFrame == -1 || m_entries[oldFrame].m_transition != transition) {
        TRmgTerrainPatternRange& range = m_ranges[transition * 2];
        oldFrame = rand() % range.m_frameCount + range.m_firstFrame;
    }
    selectedFlip = requestedFlip;
    return oldFrame;
}

DATA(0x006a4158)
TRmgTerrainPatternTable g_rmgTerrainPatternRanges;

VA(0x005b3940, 0xc5)
MAC_ADDRESS(0x254e58, 0xe4)
TRmgTerrainPatternTable::TRmgTerrainPatternTable()
{
    s32 transition = g_rmgTerrainPatterns[0].m_transition;
    b8 flipX = g_rmgTerrainPatterns[0].m_flipX;
    b8 flipY = g_rmgTerrainPatterns[0].m_flipY;
    TRmgTerrainPatternRange* range =
        &m_ranges[(transition * 2 + flipX) * 2 + flipY];
    ++range->m_frameCount;
    for (u32 index = 1; index < 48; ++index) {
        if (g_rmgTerrainPatterns[index].m_transition != transition || g_rmgTerrainPatterns[index].m_flipX != flipX
            || g_rmgTerrainPatterns[index].m_flipY != flipY) {
            transition = g_rmgTerrainPatterns[index].m_transition;
            flipX = g_rmgTerrainPatterns[index].m_flipX;
            flipY = g_rmgTerrainPatterns[index].m_flipY;
            range = &m_ranges[(transition * 2 + flipX) * 2 + flipY];
            range->m_firstFrame = index;
        }
        ++range->m_frameCount;
    }
}

VA(0x005b3a20, 0x11)
MAC_ADDRESS(0x254f4c, 0x20)  // Complete-only table terrain rule
TRmgTableTerrainRule::TRmgTableTerrainRule()
{
}

VA(0x005b3a40, 0x03)
MAC_ADDRESS(0x254f6c, 0x8)
b8 TRmgTableTerrainRule::hasSpecialBaseFrames()
{
    return 0;
}

// Both concrete six-slot terrain-rule vtables use this ICF-folded deleting
// wrapper. The emitted table-rule closure calls the shared retained destructor
// at 0x5b3850 and has the same complete-object delete semantics.
// The stateless table rule uses its implicit virtual destructor: retail
// 0x5b3a56 calls the retained base, then bit 0 selects scalar deletion.
VA_COMPGEN(0x005b3a50, 0x21, SCALAR_DELETING_DTOR, TRmgTableTerrainRule)

VA(0x005b3a80, 0x11)
MAC_ADDRESS(0x254f74, 0x10)
s32 TRmgTableTerrainRule::getTransition(s32 frame)
{
    return g_rmgTerrainPatterns[frame].m_transition;
}

VA(0x005b3aa0, 0x31)
MAC_ADDRESS(0x254f84, 0x94)  // vtable 0x642cb0 slot 4; Complete-only table rule
s32 TRmgTableTerrainRule::selectBaseFrame(s32, s32 oldFrame)
{
    if (oldFrame == -1
        || g_rmgTerrainPatterns[oldFrame].m_transition != 0) {
        oldFrame = rand() % g_rmgTerrainPatternRanges.m_ranges[0].m_frameCount
            + g_rmgTerrainPatternRanges.m_ranges[0].m_firstFrame;
    }
    return oldFrame;
}

VA(0x005b3ae0, 0x74)
MAC_ADDRESS(0x255018, 0xd8)
s32 TRmgTableTerrainRule::selectTransitionFrame(
    s32 transition, TRmgTerrainFlip requestedFlip,
    TRmgTerrainFlip& selectedFlip, s32 oldFrame)
{
    if (oldFrame == -1
        || g_rmgTerrainPatterns[oldFrame].m_transition != transition
        || g_rmgTerrainPatterns[oldFrame].m_flipX != requestedFlip.m_flipX
        || g_rmgTerrainPatterns[oldFrame].m_flipY != requestedFlip.m_flipY) {
        TRmgTerrainPatternRange& range = g_rmgTerrainPatternRanges.m_ranges[
            (transition * 2 + requestedFlip.m_flipX) * 2 + requestedFlip.m_flipY];
        oldFrame = rand() % range.m_frameCount + range.m_firstFrame;
    }
    selectedFlip = TRmgTerrainFlip(0, 0);
    return oldFrame;
}

// paintTransitions calls this fastcall selector at 0x5b5f5b.
s32 __fastcall selectTerrainTransition(
    const s32* neighbours, TRmgTerrainFlip* flip);

// Four reflections of the eight neighbour slots. Read from the pinned
// retail image; both flip bytes index this table independently.
// Neighbour direction order for each (flipX, flipY) reflection. A canonical
// pattern's direction d reads neighbour order[d]. North is up; each grid puts
// order[d] at d.
//   none      flipY     flipX     both
//   NW N NE   SW S SE   NE N NW   SE S SW
//   W  .  E   W  .  E   E  .  W   E  .  W
//   SW S SE   NW N NE   SE S SW   NE N NW
DATA(0x00642c00)
const s32 g_rmgReflectedNeighbours[2][2][TILE_DIR_COUNT] = {
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

// Provisional value-return helper, auto-inlined at every selector site.
// Returning the constructed value reproduces retail's temporary at ebp-2
// and the saved output pointer at ebp-8. A named local return instead puts
// them at ebp-8 and ebp-4 (99.93%); replacing the helper calls with direct
// construction changes the fourth reflection loop's registers (99.7991%).
// An explicit empty flip destructor prevents the helper from auto-inlining.
static TRmgTerrainFlip makeTerrainFlip(b8 x, b8 y)
{
    return TRmgTerrainFlip(x, y);
}

VA(0x005b3dd0, 0x6f)
MAC_ADDRESS(0x2551b4, 0xd0)
void TRmgTerrainPainter::initializePackedCell(
    const TRmgGridPoint& point, u32 index)
{
    TRmgTerrainTile tile = m_adapter->getTile(point);
    TRmgPackedTerrainCell& packed = m_packedCells[index];
    packed.m_terrain = tile.m_terrain;
    packed.m_frame = tile.m_frame;
    packed.m_flipX = tile.m_flipX;
    packed.m_flipY = tile.m_flipY;
    packed.m_initialized = 1;
}

VA(0x005b3e40, 0x38)
MAC_ADDRESS(0x255284, 0x64)
s32 __fastcall getRmgTerrainNeighbourKind(s32 terrain, s32 neighbourTerrain)
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

VA(0x005b3e80, 0x75f)
MAC_ADDRESS(0x2552e8, 0x980)  // fastcall call at 0x5b5f5b; retail-only
s32 __fastcall selectTerrainTransition(
    const s32* neighbours, TRmgTerrainFlip* flip)
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
    u32 i;
    for (i = 0; i < 4; ++i) {
        const s32* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            if (neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE &&
                neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[i];
                return SHAPE_E_S_BLEND_NE_SW_HARD;
            }
            if (neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[i];
                return SHAPE_E_S_BLEND_SE_HARD;
            }
        }
    }
    for (i = 0; i < 4; ++i) {
        const s32* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            if (neighbours[order[TILE_DIR_SOUTHEAST]] != RMG_NEIGHBOUR_NO_EDGE) {
                *flip = flips[i];
                return neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE ? SHAPE_N_W_SE_BLEND : SHAPE_N_W_BLEND_SE_HARD;
            }
        } else if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_HARD_EDGE &&
                   neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            if (neighbours[order[TILE_DIR_SOUTHEAST]] != RMG_NEIGHBOUR_NO_EDGE) {
                *flip = flips[i];
                return neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE ? SHAPE_N_W_SE_HARD : SHAPE_N_W_HARD_SE_BLEND;
            }
        }
    }
    for (i = 0; i < 4; ++i) {
        const s32* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            if (neighbours[order[TILE_DIR_SOUTHWEST]] != RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[i];
                return SHAPE_E_HARD_SW_BLEND;
            } else {
                *flip = makeTerrainFlip(!flips[i].m_flipX, !flips[i].m_flipY);
                return SHAPE_N_W_HARD;
            }
        }
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_HARD_EDGE) {
            if (neighbours[order[TILE_DIR_NORTHEAST]] != RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[i];
                return SHAPE_S_HARD_NE_BLEND;
            } else {
                *flip = makeTerrainFlip(!flips[i].m_flipX, !flips[i].m_flipY);
                return SHAPE_N_W_HARD;
            }
        }
    }
    for (i = 0; i < 4; ++i) {
        const s32* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            if (neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_HARD_EDGE)
                return SHAPE_E_BLEND_SW_HARD;
            if (neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE)
                return SHAPE_S_BLEND_NE_HARD;
        }
    }
    for (i = 0; i < 4; ++i) {
        const s32* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return SHAPE_N_W_BLEND;
        }
        if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return SHAPE_N_W_HARD;
        }
    }
    for (i = 0; i < 4; ++i) {
        const s32* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return SHAPE_E_BLEND_SW_HARD;
        }
        if (neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return SHAPE_S_BLEND_NE_HARD;
        }
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return SHAPE_E_HARD_SW_BLEND;
        }
        if (neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return SHAPE_S_HARD_NE_BLEND;
        }
        if ((neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
             neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE) ||
            (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_BLEND_EDGE &&
             neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_BLEND_EDGE)) {
            *flip = flips[i];
            return SHAPE_N_W_BLEND;
        }
        if ((neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_HARD_EDGE &&
             neighbours[order[TILE_DIR_NORTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) ||
            (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_HARD_EDGE &&
             neighbours[order[TILE_DIR_SOUTHWEST]] == RMG_NEIGHBOUR_HARD_EDGE)) {
            *flip = flips[i];
            return SHAPE_N_W_HARD;
        }
    }
    for (i = 0; i < 4; ++i) {
        const s32* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[TILE_DIR_EAST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return SHAPE_E_BLEND_SE_HARD;
        }
        if (neighbours[order[TILE_DIR_SOUTH]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return SHAPE_S_BLEND_SE_HARD;
        }
    }
    for (i = 0; i < 4; ++i) {
        const s32* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return SHAPE_N_BLEND;
        }
        if (neighbours[order[TILE_DIR_NORTH]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return SHAPE_N_HARD;
        }
        if (neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return SHAPE_W_BLEND;
        }
        if (neighbours[order[TILE_DIR_WEST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return SHAPE_W_HARD;
        }
    }
    for (i = 0; i < 4; ++i) {
        const s32* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[TILE_DIR_NORTHWEST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return SHAPE_NW_SE_BLEND;
        }
        if (neighbours[order[TILE_DIR_NORTHWEST]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return SHAPE_NW_BLEND_SE_HARD;
        }
        if (neighbours[order[TILE_DIR_NORTHWEST]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return SHAPE_NW_SE_HARD;
        }
    }
    for (i = 0; i < 4; ++i) {
        const s32* order = g_rmgReflectedNeighbours[flips[i].m_flipX][flips[i].m_flipY];
        if (neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return SHAPE_SE_BLEND;
        }
        if (neighbours[order[TILE_DIR_SOUTHEAST]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return SHAPE_SE_HARD;
        }
    }
    *flip = makeTerrainFlip(0, 0);
    return SHAPE_FILL;
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
VA(0x005b45f0, 0x26d)
MAC_ADDRESS(0x255c68, 0xbc)
TRmgTerrainPainter::TRmgTerrainPainter(
    TRmgMapInterface* newAdapter, s32 terrain, s32 strength)
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

VA(0x005b48d0, 0x8d)  // repeated caller identity in 0x5b3dd0..0x5b76f0
TRmgPackedTerrainCell* TRmgTerrainPainter::getPackedCell(
    const TRmgGridPoint& point)
{
    u32 index = point.m_y * m_size.m_x + point.m_x;
    if (!m_packedCells[index].m_initialized)
        initializePackedCell(point, index);
    return &m_packedCells[index];
}

// Retail proves the shared accessor and its expanded uses, but supplies no
// source inline qualifier. Its ordinary TU definition preserves paintPoint's
// 99.5570% checkpoint and both exact painter/brush destructors.
s32 TRmgTerrainPainter::getTerrain(const TRmgGridPoint& point)
{
    return getPackedCell(point)->getTerrain();
}

s32 TRmgTerrainPainter::getFrame(const TRmgGridPoint& point)
{
    return getPackedCell(point)->getFrame();
}

u32 TRmgTerrainPainter::getWidth() const
{
    return m_size.m_x;
}

u32 TRmgTerrainPainter::getHeight() const
{
    return m_size.m_y;
}

// The base-frame paths in PaintPoint and PaintTransitions first compute
// strength, then load the selected rule's virtual receiver. Keep that shared
// evaluation boundary and the captured terrain index across the first call.
// The helper's role and signature are inferred from retail expansions.
MAC_ADDRESS(0x259998, 0x60)
s32 TRmgTerrainPainter::selectBaseFrame(
    const TRmgGridPoint& point, s32 terrain, s32 oldFrame)
{
    s32 strength = getTransitionStrength(point, terrain);
    return g_rmgTerrainRules[terrain]->selectBaseFrame(strength, oldFrame);
}

// PaintPoint and both transition updates repeat this adapter/cache write.
// Preserve the shared operation, including validity before the four values;
// cache initialization from an adapter read has a different store order.
// This ordinary helper is inferred from retail expansions, with no DC name.
MAC_ADDRESS(0x2550f0, 0xc4)
void TRmgTerrainPainter::setTile(
    const TRmgGridPoint& point, const TRmgTerrainTile& tile)
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
void TRmgTerrainPainter::paintBaseTile(const TRmgGridPoint& point)
{
    s32 frame = selectBaseFrame(point, m_paintTerrain, -1);
    TRmgTerrainTile tile(m_paintTerrain, frame);
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
const s32& TRmgTerrainPainter::getPaintTerrain() const
{
    return m_paintTerrain;
}

b8 TRmgTerrainPainter::isPaintTerrain(const TRmgGridPoint& point)
{
    if (getTerrain(point) != getPaintTerrain())
        return 0;
    return 1;
}

VA(0x005b4960, 0x1b2)
MAC_ADDRESS(0x255ef0, 0x11c)
void TRmgTerrainPainter::paintRectangle(
    u32 x, u32 y,
    u32 rectangleWidth, u32 rectangleHeight)
{
    u32 endX = x + rectangleWidth;
    u32 endY = y + rectangleHeight;
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

VA(0x005b4b20, 0x5cb)
MAC_ADDRESS(0x256014, 0x580) // anchor-callee 0x5b4960, 0x5b5440; thiscall, ret 4
void TRmgTerrainPainter::paintPoint(const TRmgGridPoint& point)
{
    {
        s32 frame = selectBaseFrame(point, m_paintTerrain, -1);
        setTile(point, TRmgTerrainTile(m_paintTerrain, frame));
    }

    if (m_otherTerrainPoints.find(point) != m_otherTerrainPoints.end())
        m_otherTerrainPoints.erase(point);

    if (g_rmgTerrainRules[m_paintTerrain]->m_allowsSeparatedNeighbours) {
        if (point.m_y > 0) {
            TRmgGridPoint nearby(point.m_x, point.m_y - 1);
            if (m_repairPoints.find(nearby) != m_repairPoints.end()
                && !isHorizontalGap(nearby)) {
                m_repairPoints.erase(nearby);
                queueOtherTerrainNeighbours(nearby);
            }
        }
        if (point.m_y < m_size.m_y - 1) {
            TRmgGridPoint nearby(point.m_x, point.m_y + 1);
            if (m_repairPoints.find(nearby) != m_repairPoints.end()
                && !isHorizontalGap(nearby)) {
                m_repairPoints.erase(nearby);
                queueOtherTerrainNeighbours(nearby);
            }
        }
        if (point.m_x > 0) {
            TRmgGridPoint nearby(point.m_x - 1, point.m_y);
            if (m_repairPoints.find(nearby) != m_repairPoints.end()
                && !isVerticalGap(nearby)) {
                m_repairPoints.erase(nearby);
                queueOtherTerrainNeighbours(nearby);
            }
        }
        if (point.m_x < m_size.m_x - 1) {
            TRmgGridPoint nearby(point.m_x + 1, point.m_y);
            if (m_repairPoints.find(nearby) != m_repairPoints.end()
                && !isVerticalGap(nearby)) {
                m_repairPoints.erase(nearby);
                queueOtherTerrainNeighbours(nearby);
            }
        }
    } else {
        b8 neighbourExists[TILE_DIR_COUNT];
        buildTileNeighbourMask(
            m_size.m_x, m_size.m_y, point.m_x, point.m_y, neighbourExists);
        for (u32 direction = 0; direction < TILE_DIR_COUNT; ++direction) {
            if (neighbourExists[direction]) {
                const TPoint& offset = g_tileDirections[direction];
                TRmgGridPoint nearby(point + offset);
                if (isPaintTerrain(nearby)) {
                    if (m_repairPoints.find(nearby) != m_repairPoints.end()) {
                        if (!needsTerrainRepair(nearby)) {
                            m_repairPoints.erase(nearby);
                            queueOtherTerrainNeighbours(nearby);
                        }
                    } else if (needsTerrainRepair(nearby)) {
                        m_repairPoints.insert(nearby);
                    }
                }
            }
        }
    }
    if (needsTerrainRepair(point))
        m_repairPoints.insert(point);
    else
        queueOtherTerrainNeighbours(point);
}

VA(0x005b50f0, 0x34e)
MAC_ADDRESS(0x2565ac, 0x638) // anchor-callee 0x5b4c72, 0x5b50dd; thiscall, ret 4
void TRmgTerrainPainter::queueOtherTerrainNeighbours(const TRmgGridPoint& point)
{
    if (point.getY() > 0
        && getTerrain(TRmgGridPoint(point.getX(), point.getY() - 1)) != m_paintTerrain) {
        m_otherTerrainPoints.insert(TRmgGridPoint(point.getX(), point.getY() - 1));
    } else if (point.getY() < getHeight() - 1
        && getTerrain(TRmgGridPoint(point.getX(), point.getY() + 1)) != m_paintTerrain) {
        m_otherTerrainPoints.insert(TRmgGridPoint(point.getX(), point.getY() + 1));
    }
    if (point.getX() > 0
        && getTerrain(TRmgGridPoint(point.getX() - 1, point.getY())) != m_paintTerrain) {
        m_otherTerrainPoints.insert(TRmgGridPoint(point.getX() - 1, point.getY()));
    } else if (point.getX() < getWidth() - 1
        && getTerrain(TRmgGridPoint(point.getX() + 1, point.getY())) != m_paintTerrain) {
        m_otherTerrainPoints.insert(TRmgGridPoint(point.getX() + 1, point.getY()));
    }
    if (point.getX() > 0 && point.getY() > 0) {
        TRmgGridPoint nearby(point.getX() - 1, point.getY() - 1);
        s32 terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            m_otherTerrainPoints.insert(nearby);
    }
    if (point.getX() < getWidth() - 1 && point.getY() > 0) {
        TRmgGridPoint nearby(point.getX() + 1, point.getY() - 1);
        s32 terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            m_otherTerrainPoints.insert(nearby);
    }
    if (point.getX() > 0 && point.getY() < getHeight() - 1) {
        TRmgGridPoint nearby(point.getX() - 1, point.getY() + 1);
        s32 terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            m_otherTerrainPoints.insert(nearby);
    }
    if (point.getX() < getWidth() - 1 && point.getY() < getHeight() - 1) {
        TRmgGridPoint nearby(point.getX() + 1, point.getY() + 1);
        s32 terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            m_otherTerrainPoints.insert(nearby);
    }
}

// Own-terrain checks share the predicates used with the selected paint
// terrain. Retail retains the nested predicate in the horizontal check and expands
// the vertical check at the four adjacent-row/column probes.
b8 TRmgTerrainPainter::isHorizontalGap(const TRmgGridPoint& point)
{
    return isHorizontalGap(point, getTerrain(point));
}

b8 TRmgTerrainPainter::isVerticalGap(const TRmgGridPoint& point)
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
b8 TRmgTerrainPainter::needsTerrainRepair(const TRmgGridPoint& point)
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
// scores 86.0995% or 86.1619% for bool/byte/s32, below 91.3389%.
// At the current 93.6307%, all 46 calls and the complete CFG agree. The
// 16 coordinate-accessor forms preserve those calls and all sibling scores.
// Value accessors for the unchanged coordinate restore retail's fresh scalar
// copy and terrain reload, but retain the reversed ESI/EDI roles and change
// scheduling (85.9764..93.6307%). The lifetime mechanism is insufficient alone.
// Query-result widths, owned scalar results, shared gap counters, workspace
// lifetimes and gap-record construction/compaction leave 93.6307% unchanged.
// Passive C2 tracing reproduces the whole object and observes 227 temporary
// bindings; it does not identify the cause of the painter/point allocation.
// Returning configured terrain by const reference, together with value
// coordinate accessors in both gap predicates, reaches 99.1821%. All 1576
// bytes align except an ESI/EDI permutation; frame, scalar copies and all
// 46 calls agree. The accessor refers to the painter member, not a temporary.
VA(0x005b5440, 0x628)
MAC_ADDRESS(0x256be4, 0x630) // anchor-callee 0x5b7358; thiscall, ret 4; retail-only
void TRmgTerrainPainter::repairTerrainPoint(const TRmgGridPoint& point)
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
        b8 matches[TILE_DIR_COUNT];
        buildMatchingNeighbourMask(point, matches);
        TRmgTerrainGap gaps[TILE_DIR_COUNT / 2];
        u32 gapCount = 0;
        u32 first = 0;
        while (!matches[first])
            ++first;

        u32 direction = first;
        while (1) {
            direction = (direction + 1) % TILE_DIR_COUNT;
            if (direction == first)
                break;
            if (!matches[direction]) {
                u32 currentGap = gapCount++;
                gaps[currentGap].m_weight = 0;
                gaps[currentGap].m_start = direction;
                gaps[currentGap].m_length = 0;
                do {
                    u32 diagonal = direction & 1;
                    gaps[currentGap].m_weight += diagonal ? 1 : 2;
                    ++gaps[currentGap].m_length;
                    direction = (direction + 1) % TILE_DIR_COUNT;
                    if (direction == first)
                        goto gapsBuilt;
                } while (!matches[direction]);
            }
        }

    gapsBuilt:
        b8 neighbourExists[TILE_DIR_COUNT];
        buildTileNeighbourMask(getWidth(), getHeight(), point.m_x, point.m_y,
                               neighbourExists);
        do {
            u32 smallest = 0;
            u32 smallestWeight = gaps[0].m_weight;
            {
                for (u32 gap = 1; gap < gapCount; ++gap) {
                    if (gaps[gap].m_weight < smallestWeight) {
                        smallest = gap;
                        smallestWeight = gaps[gap].m_weight;
                    }
                }
            }
            u32 end =
                (gaps[smallest].m_start + gaps[smallest].m_length) % TILE_DIR_COUNT;
            for (direction = gaps[smallest].m_start; direction != end;
                 direction = (direction + 1) % TILE_DIR_COUNT) {
                if (neighbourExists[direction])
                    paintPoint(point + g_tileDirections[direction]);
            }
            --gapCount;
            for (u32 gap = smallest; gap < gapCount; ++gap)
                gaps[gap] = gaps[gap + 1];
        } while (gapCount > 1);
    }
}

// Each cell counts its boundaries toward the neighbours that follow it
// in scan order (E, SE, S and SW), so every boundary is counted once.
// North is up; C counts its boundary with each o.
//   . . .
//   . C o
//   o o o
VA(0x005b5a70, 0x8a7)
MAC_ADDRESS(0x257214, 0xd54)
void TRmgTerrainPainter::paintTransitions()
{
    std::vector<b8> edgeCounts(getWidth() * getHeight());
    TRmgGridPoint point;

    for (point.setY(0); point.m_y < getHeight() - 1; point.setY(point.getY() + 1)) {
        s32 terrain = getTerrain(TRmgGridPoint(0, point.m_y));

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
        s32 terrain = getTerrain(point);
        TRmgGridPoint east(point.getX() + 1, point.getY());
        if (getTerrain(east) != terrain) {
            ++edgeCounts[point.getY() * getWidth() + point.getX()];
            ++edgeCounts[point.getY() * getWidth() + point.getX() + 1];
        }
    }

    for (point.setY(0); point.m_y < getHeight(); point.setY(point.getY() + 1)) {
        for (point.setX(0); point.m_x < getWidth(); point.setX(point.getX() + 1)) {
            u32 index = point.getY() * getWidth() + point.getX();

            if (edgeCounts[index] > 0) {
                s32 neighbours[8];
                buildNeighbourKinds(point, neighbours);

                s32 transition;
                TRmgTerrainFlip flip;
                transition = selectTerrainTransition(neighbours, &flip);
                if (transition == SHAPE_N_W_BLEND) {
                    if (checkFirstDiagonal(point, flip))
                        transition = 6;
                } else if (transition == SHAPE_N_W_HARD) {
                    if (checkFirstDiagonal(point, flip))
                        transition = 12;
                } else if (transition == SHAPE_SE_BLEND) {
                    if (checkSecondDiagonal(point, flip))
                        transition = 7;
                } else if (transition == SHAPE_SE_HARD) {
                    if (checkSecondDiagonal(point, flip))
                        transition = 13;
                }

                TRmgTerrainTile tile = getPackedCell(point)->getTile();

                s32 newFrame;
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
                TRmgTerrainTile tile = getPackedCell(point)->getTile();

                s32 newFrame = selectBaseFrame(point, tile.m_terrain, tile.m_frame);
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

VA(0x005b6320, 0x107)
MAC_ADDRESS(0x25803c, 0x160)
b8 TRmgTerrainPainter::isHorizontalGap(
    const TRmgGridPoint& point, s32 terrain)
{
    return point.m_x > 0 && point.m_x < getWidth() - 1
        && getTerrain(TRmgGridPoint(point.getX() - 1, point.getY())) != terrain
        && getTerrain(TRmgGridPoint(point.getX() + 1, point.getY())) != terrain;
}

VA(0x005b6430, 0x106)
MAC_ADDRESS(0x25819c, 0x160)
b8 TRmgTerrainPainter::isVerticalGap(
    const TRmgGridPoint& point, s32 terrain)
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
VA(0x005b6540, 0x2ca)
MAC_ADDRESS(0x2582fc, 0x4f8) // anchor-callee 0x5b58f8, 0x5b681e; retail-only
void TRmgTerrainPainter::buildMatchingNeighbourMask(
    const TRmgGridPoint& point, b8* matches)
{
    s32 terrain = getTerrain(point);
    u32 north = point.m_y > 0 ? point.m_y - 1 : point.m_y;
    u32 south = point.m_y < m_size.m_y - 1 ? point.m_y + 1 : point.m_y;
    u32 west = point.m_x > 0 ? point.m_x - 1 : point.m_x;
    u32 east = point.m_x < m_size.m_x - 1 ? point.m_x + 1 : point.m_x;
    TRmgGridPoint low = TRmgGridPoint(west, north);
    TRmgGridPoint high = TRmgGridPoint(east, south);

    {
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
    }
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

// Starting in a gap, a matching run, another gap and another match before
// returning to the start prove that the centre's neighbours are separated.
// For example, North is up; # matches the centre C's terrain, . does not.
//   . # .
//   . C .
//   . # .
VA(0x005b6810, 0x84)
MAC_ADDRESS(0x2587f4, 0xd0)
b8 TRmgTerrainPainter::hasSeparatedNeighbours(const TRmgGridPoint& point)
{
    b8 matches[TILE_DIR_COUNT];
    buildMatchingNeighbourMask(point, matches);
    u32 first = 0;
    u32 direction;
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

VA(0x005b68a0, 0x2ff)
MAC_ADDRESS(0x258aa0, 0x478)
void TRmgTerrainPainter::buildNeighbourKinds(
    const TRmgGridPoint& point, s32* neighbours)
{
    s32 terrain = getTerrain(point);
    u32 north = point.m_y > 0 ? point.m_y - 1 : point.m_y;
    u32 south = point.m_y < m_size.m_y - 1 ? point.m_y + 1 : point.m_y;
    u32 west = point.m_x > 0 ? point.m_x - 1 : point.m_x;
    u32 east = point.m_x < m_size.m_x - 1 ? point.m_x + 1 : point.m_x;

    {
        TRmgGridPoint nearby(point.m_x, north);
        s32 nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_NORTH] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(point.m_x, south);
        s32 nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_SOUTH] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(west, point.m_y);
        s32 nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_WEST] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(east, point.m_y);
        s32 nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_EAST] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(west, north);
        s32 nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_NORTHWEST] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(east, north);
        s32 nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_NORTHEAST] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(west, south);
        s32 nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_SOUTHWEST] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
    {
        TRmgGridPoint nearby(east, south);
        s32 nearbyTerrain = getTerrain(nearby);
        neighbours[TILE_DIR_SOUTHEAST] = getRmgTerrainNeighbourKind(
            terrain, nearbyTerrain);
    }
}

// These Complete-only diagonal callers retain the opposite upper-clamp
// operand orientation under canonical tLimit. Spelling the comparison as
// value > maximum makes these two callers exact but regresses the retained
// helper and several Dreamcast-proven limit callers, so keep the shared helper
// canonical and recover the caller-specific compiler state separately.
// Min/max compositions do not recover these bodies.
// An outer corner lies on a 45-degree edge when either diagonal beside it
// (NE or SW of the canonical NW corner) has the cell's own terrain.
// North is up; C is the cell, e an edge, o a probe, ? not tested.
//   ? e o
//   e C ?
//   o ? ?
VA(0x005b6ba0, 0x24c)
MAC_ADDRESS(0x258f18, 0x360)
b8 TRmgTerrainPainter::checkFirstDiagonal(
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
    s32 terrain = getTerrain(point);
    const TPoint* pair = firstDiagonalOffsets[(flip.m_flipY << 1) | flip.m_flipX];
    TRmgGridPoint nearby(
        tLimit(
            0, static_cast<s32>(point.getX()) + pair[0].getX(), static_cast<s32>(getWidth()) - 1),
        tLimit(
            0, static_cast<s32>(point.getY()) + pair[0].getY(), static_cast<s32>(getHeight()) - 1));
    if (getTerrain(nearby) == terrain)
        return 1;
    nearby.setX(tLimit(
        0, static_cast<s32>(point.getX()) + pair[1].getX(), static_cast<s32>(getWidth()) - 1));
    nearby.setY(tLimit(
        0, static_cast<s32>(point.getY()) + pair[1].getY(), static_cast<s32>(getHeight()) - 1));
    return getTerrain(nearby) == terrain;
}

// An inner corner lies on a 45-degree edge when the cell two steps along
// either side (E or S of the canonical SE corner) has another terrain.
// North is up; C is the cell, e its edge, . none, o a probe, ? not tested.
//   C . o
//   . e ?
//   o ? ?
VA(0x005b6e00, 0x1b3)
MAC_ADDRESS(0x259278, 0x288)
b8 TRmgTerrainPainter::checkSecondDiagonal(
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
    s32 terrain = getTerrain(point);
    const TPoint& offset = secondDiagonalOffsets[(flip.m_flipY << 1) | flip.m_flipX];
    TRmgGridPoint nearby(
        tLimit(0, static_cast<s32>(point.getX()) + offset.getX(), static_cast<s32>(getWidth()) - 1), point.getY());
    if (getTerrain(nearby) != terrain)
        return 1;
    nearby.setX(point.getX());
    s32 maximum = static_cast<s32>(getHeight()) - 1;
    s32 y = static_cast<s32>(point.getY()) + offset.getY();
    nearby.setY(tLimit(0, y, maximum));
    return getPackedCell(nearby)->getTerrain() != terrain;
}

VA(0x005b6fd0, 0x271)
MAC_ADDRESS(0x259500, 0x444)
s32 TRmgTerrainPainter::getTransitionStrength(
    const TRmgGridPoint& point, s32 terrain)
{
    u32 strength = m_transitionStrength;
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
void TRmgTerrainPainter::finish()
{
    do {
        while (m_repairPoints.size()) {
            TRmgGridPoint point = *m_repairPoints.begin();
            repairTerrainPoint(point);
        }
        while (m_otherTerrainPoints.size()) {
            TRmgGridPoint point = *m_otherTerrainPoints.begin();
            m_otherTerrainPoints.erase(point);
            if (needsTerrainRepair(point))
                paintPoint(point);
        }
    } while (m_repairPoints.size());
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
s32 TRmgTerrainPainter::changeTerrain(s32 terrain, s32 strength)
{
    s32 previous = m_paintTerrain;
    finish();
    m_paintTerrain = terrain;
    m_transitionStrength = strength;
    return previous;
}

VA(0x005b7250, 0x9a)
MAC_ADDRESS(0x2599f8, 0xac) // anchor-callee 0x54017e; allocation and throw RTTI
TRmgTerrainBrush::TRmgTerrainBrush(
    TRmgMapInterface* map, s32 terrain, s32 strength)
    : m_painter(new TRmgTerrainPainter(map, terrain, strength))
{
    if (!m_painter.get())
        throw TAllocationFailure();
}

VA(0x005b72f0, 0x225)
MAC_ADDRESS(0x259aa4, 0xc8) // anchor-callee 0x540207; auto_ptr ownership cleanup
TRmgTerrainBrush::~TRmgTerrainBrush()
{
}

VA(0x005b7520, 0x16a)
MAC_ADDRESS(0x259c9c, 0x24)
void TRmgTerrainBrush::changeTerrain(s32 terrain, s32 strength)
{
    m_painter->changeTerrain(terrain, strength);
}

VA(0x005b7690, 0x1f)
MAC_ADDRESS(0x259cc0, 0x24) // anchor-callee 0x5401e9; four unsigned rectangle args
void TRmgTerrainBrush::paintRectangle(
    u32 x, u32 y,
    u32 rectangleWidth, u32 rectangleHeight)
{
    m_painter->paintRectangle(x, y, rectangleWidth, rectangleHeight);
}

// The brush constructor's allocation-failure unwind reaches the retained
// Dinkumware auto_ptr destructor. Its {owns, pointer} layout, pointee
// destructor call, and scalar delete exactly identify this specialization.
VA_COMPGEN(0x005b76d0, 0x20, IMPLICIT_DTOR, rmgTerrainPainter_auto_ptr)

VA(0x005b76f0, 0x209)
MAC_ADDRESS(0x259be4, 0xb8) // anchor-callee 0x5b76e0; retained painter destructor
TRmgTerrainPainter::~TRmgTerrainPainter()
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
VA_COMPGEN(0x005b7cd0, 0x156, TREE_INSERT, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005b8670, 0xa8, TREE_INIT, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005b8720, 0x2fe, TREE_NODE_INSERT, TRmgCoordinatePoint_unsigned_int)

// Public insert's predecessor test calls this node walk; its color field
// at +0x14 and nil references identify the same terrain point-set instance.
// The naturally emitted body matches all 179 bytes.
VA_COMPGEN(0x005b8aa0, 0xb3, TREE_CONST_ITERATOR_DEC, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005b7f60, 0x59, TREE_ERASE_KEY, TRmgCoordinatePoint_unsigned_int)

// The retained erase(key) calls 0x5b7e30 with two iterators and a hidden
// result pointer. Its whole-range branch recursively clears nodes through
// 0x5b85f0; its partial-range branch increments then calls 0x5b8090.
// All three share the point tree's nil sentinel at 0x6a52c4. These ordinary
// Dinkumware bodies are naturally emitted by the existing set operations.
// All 289/1295/126 bytes match respectively. The retained _Lockit destructor
// at 0x60b634 releases the CRT lock through LeaveCriticalSection.
VA_COMPGEN(0x005b7e30, 0x121, TREE_ERASE_RANGE, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005b8090, 0x50f, TREE_ERASE_ITERATOR, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005b85f0, 0x7e, TREE_ERASE, TRmgCoordinatePoint_unsigned_int)

// Both erase overloads and the admitted distance loop retain this successor
// walk. Its 0x6a52c4 nil references prove the terrain point-set ownership;
// the naturally emitted TRmgGridPoint specialization matches all 163 bytes.
// This replaces the provisional TPoint claim and its artificial emission
// wrapper in rmg.cpp. The two specializations have distinct nil symbols.
VA_COMPGEN(0x005b8bc0, 0xa3, TREE_CONST_ITERATOR_INC, TRmgCoordinatePoint_unsigned_int)

// The canonical coordinate comparator uses its getX/getY interface. Its
// retained 32-byte body stays exact, and the expanded comparison here now
// loads node-y before key-y as retail does. Direct fields leave 99.4634%.
VA_COMPGEN(0x005b7fc0, 0x57, TREE_FIND, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005b8020, 0x35, VECTOR_ERASE, TRmgPackedTerrainCell)

VA_COMPGEN(0x005b8060, 0x24, VECTOR_UFILL, unsigned_char)

// PaintPoint and TRmgTerrainBrush::changeTerrain retain this one-dword
// iterator wrapper around the tree's raw-node lower bound.
VA_COMPGEN(0x005b85a0, 0x17, TREE_LOWER_BOUND, TRmgCoordinatePoint_unsigned_int)

// PaintPoint retains the two-bound wrapper returning its iterator pair.
VA_COMPGEN(0x005b85c0, 0x2c, TREE_EQUAL_RANGE, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005b8a20, 0x17, TREE_UPPER_BOUND, TRmgCoordinatePoint_unsigned_int)

// The retained public wrappers above delegate to these raw-node searches.
VA_COMPGEN(0x005b8a40, 0x59, TREE_LBOUND, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005b8b60, 0x59, TREE_UBOUND, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005b4860, 0x6e, IMPLICIT_DTOR, set)

// erase(key) in TRmgTerrainBrush::changeTerrain retains Dinkumware's
// public distance wrapper and its category-dispatched overload. The wrapper
// increments the caller's count directly; the unused tag argument accounts
// for the tagged body's missing self-store.
VA_COMPGEN(0x005b8c70, 0x2b, STD_DISTANCE, TRmgCoordinatePoint_unsigned_int)

VA_COMPGEN(0x005b8cd0, 0x28, STD_DISTANCE_TAGGED, TRmgCoordinatePoint_unsigned_int)

template<class Coordinate>
// VA instance: operator< <u32>
VA(0x005b8ca0, 0x20) // anchor-callee 0x5b4e96; fastcall, two point references
bool operator<(const TRmgCoordinatePoint<Coordinate>& left,
    const TRmgCoordinatePoint<Coordinate>& right)
{
    return left.getY() < right.getY() || (left.getY() == right.getY() && left.getX() < right.getX());
}

// Complete terrain data, read from the pinned retail image. The initializer
// calls at 0x5b3b60..0x5b3da0 prove entry counts, arguments and object order;
// the table at 0x642bd8 proves terrain-index order. Names are role-derived.
// Terrain frame tables and the per-terrain rules built from them.
// Rock's fixed frames, as {shape, flipX, flipY}. The flips are drawn into the
// art, so each reflection has its own frames and no sprite flip. A flip
// mirrors the canonical shape; North is up and e is an edge, here for N_W:
//   none   flipX  flipY  both
//   . e .  . e .  . . .  . . .
//   e C .  . C e  e C .  . C e
//   . . .  . . .  . e .  . e .
// Frames: 0-7 base, then two per shape and flip, all hard: N_W 8-15 (none,
// flipX, flipY, both), W 16-19 (none, flipX), N 20-23 (none, flipY),
// SE 24-31, N_W_DIAG 32-39 and SE_DIAG 40-47.
DATA(0x006424a8)
const TRmgTerrainTransitionEntry g_rmgTerrainPatterns[48] = {
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

DATA(0x006428a0)
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

DATA(0x00642a10)
static const TRmgTerrainPatternEntry g_rmgSandPatternEntries[24] = {
    {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false},
    {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false}, {SHAPE_FILL, false},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
    {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true}, {SHAPE_FILL, true},
};

DATA(0x00642ad0)
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

// Retail's nine atexit callbacks load the matching rule into ecx and tail-call
// TRmgTerrainRule::~TRmgTerrainRule (0x005B3850). VC6 emits the same callbacks
// through the implicit derived destructor, whose bytes and vtable relocation
// are identical to the retained base destructor (LINK folds the two bodies).
DATA(0x006a48d0)
static TRmgPatternTerrainRule g_rmgDirtRule(1, 1, 50, 46, g_rmgDirtPatternEntries);

VA_COMPGEN(0x005b3b60, 0x23, STATIC_CTOR, g_rmgDirtRule)

VA_COMPGEN(0x005b3b90, 0x0a, STATIC_DTOR, g_rmgDirtRule)
DATA(0x006a44f8)
static TRmgPatternTerrainRule g_rmgSandRule(0, 1, 70, 24, g_rmgSandPatternEntries);

VA_COMPGEN(0x005b3ba0, 0x23, STATIC_CTOR, g_rmgSandRule)

VA_COMPGEN(0x005b3bd0, 0x0a, STATIC_DTOR, g_rmgSandRule)
DATA(0x006a3d88)
static TRmgPatternTerrainRule g_rmgGrassRule(1, 1, 50, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005b3be0, 0x23, STATIC_CTOR, g_rmgGrassRule)

VA_COMPGEN(0x005b3c10, 0x0a, STATIC_DTOR, g_rmgGrassRule)
DATA(0x006a3f70)
static TRmgPatternTerrainRule g_rmgSnowRule(1, 1, 80, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005b3c20, 0x23, STATIC_CTOR, g_rmgSnowRule)

VA_COMPGEN(0x005b3c50, 0x0a, STATIC_DTOR, g_rmgSnowRule)
DATA(0x006a46e0)
static TRmgPatternTerrainRule g_rmgSwampRule(1, 1, 80, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005b3c60, 0x23, STATIC_CTOR, g_rmgSwampRule)

VA_COMPGEN(0x005b3c90, 0x0a, STATIC_DTOR, g_rmgSwampRule)
DATA(0x006a4ab8)
static TRmgPatternTerrainRule g_rmgRoughRule(1, 1, 80, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005b3ca0, 0x23, STATIC_CTOR, g_rmgRoughRule)

VA_COMPGEN(0x005b3cd0, 0x0a, STATIC_DTOR, g_rmgRoughRule)
DATA(0x006a5070)
static TRmgPatternTerrainRule g_rmgSubterraneanRule(1, 1, 60, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005b3ce0, 0x23, STATIC_CTOR, g_rmgSubterraneanRule)

VA_COMPGEN(0x005b3d10, 0x0a, STATIC_DTOR, g_rmgSubterraneanRule)
DATA(0x006a4e88)
static TRmgPatternTerrainRule g_rmgLavaRule(1, 1, 80, 79, g_rmgLandPatternEntries);

VA_COMPGEN(0x005b3d20, 0x23, STATIC_CTOR, g_rmgLavaRule)

VA_COMPGEN(0x005b3d50, 0x0a, STATIC_DTOR, g_rmgLavaRule)
DATA(0x006a4ca0)
static TRmgPatternTerrainRule g_rmgWaterRule(0, 0, 0, 33, g_rmgWaterPatternEntries);

VA_COMPGEN(0x005b3d60, 0x23, STATIC_CTOR, g_rmgWaterRule)

VA_COMPGEN(0x005b3d90, 0x0a, STATIC_DTOR, g_rmgWaterRule)
DATA(0x006a48c8)
static TRmgTableTerrainRule g_rmgRockRule;

// Same owner/atexit and folded base-destructor proof as the pattern rules.
VA_COMPGEN(0x005b3da0, 0x16, STATIC_CTOR, g_rmgRockRule)

VA_COMPGEN(0x005b3dc0, 0x0a, STATIC_DTOR, g_rmgRockRule)

DATA(0x00642bd8)
TRmgTerrainRule* const g_rmgTerrainRules[10] = {
    &g_rmgDirtRule, &g_rmgSandRule, &g_rmgGrassRule, &g_rmgSnowRule,
    &g_rmgSwampRule, &g_rmgRoughRule, &g_rmgSubterraneanRule,
    &g_rmgLavaRule, &g_rmgWaterRule, &g_rmgRockRule
};
