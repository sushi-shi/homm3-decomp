// rmg_terrain.cpp - Complete-only random-map terrain transition support.
//
// The Dreamcast build contains no random-map generator compiland. Function
// ownership, field layout, helper boundaries, and call/expansion decisions in
// this unit therefore come directly from the retail x86 cluster.
#include <va.h>
#include <stdlib.h>
#include "rmg_terrain.h"
#include "exceptions.h"
#include "tiles.h"

DATA(0x00642BD8) extern TRmgTerrainRule* const g_rmgTerrainRules[];

TRmgLinePainterTile::TRmgLinePainterTile(
    TRmgLinePainterInterface* painter, const TRmgGridPoint& point)
    : m_painter(painter)
{
    m_point = point;
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

TRmgGridRectangle::TRmgGridRectangle(
    const TRmgGridPoint& origin, unsigned int width, unsigned int height)
    : m_origin(origin), m_size(width, height)
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
// Residual (71.9231%): the retained calls at 0x4f9f60/0x4f9f77/0x4f9f86
// (TPoint add, grid conversion, proxy factory) sit where this body's own
// budget is still above 700, so retail expands the loop body from a nested
// context whose budget is under 42; the signed-point arithmetic reproduces
// retail's operand shapes but not those three refusals (74.42% with the
// grid-side sum, which had the wrong shapes).
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
            matches[direction] = painter->at(point + g_tileDirections[direction]).getLand() == oldType;
        else
            matches[direction] = 0;
    }
    TRmgLinePatternTable* table = painter->getPattern(oldType);
    int pattern;
    unsigned char flipX, flipY;
    selectRmgLinePattern(matches, table, pattern, flipX, flipY);
    rmgTerrainTile current;
    tile.getTile(current);
    if (table->m_patterns[current.m_frame] != pattern
        || current.m_flipX != flipX || current.m_flipY != flipY) {
        unsigned int frame = table->m_ranges[pattern].m_firstIndex
            + rand() % table->m_ranges[pattern].m_valueCount;
        current.m_flipY = flipY;
        current.m_frame = frame;
        current.m_flipX = flipX;
        tile.setTile(current);
    }
}

// Retail 0x4fa050: thiscall with a hidden twelve-byte return and point
// reference on the stack (ret 8). The retained call at 0x4f9f86 uses this
// same proxy that the refresh's entry expands; keep one ordinary helper.
// A separate VC6 ABI control with this real grid class passes a by-value
// point as eight stack bytes, not an indirect pointer. With a hidden result
// that would require ret 0xc, contradicting this body; retain the reference.
// A separate 112-case explicit proxy-copy/return family produced 18 distinct
// objects but no gain in this retained body. Its best refresh score (67.3461%)
// traded this helper down to 63.7647%; no independent copy-constructor evidence
// justifies adopting that tradeoff. Keep those sources as experimental parents.
// All 192 constructor-binding variants (27 distinct objects) were also tested.
// A copied coordinate parameter retains the ordinary proxy ctor plus a result
// copy in at(); retail expands both. It does not improve either retained body.
VA(0x004FA050, 0x22) // anchor-callee 0x4f9f86; thiscall hidden value return
TRmgLinePainterTile TRmgLinePainterInterface::at(const TRmgGridPoint& point)
{
    return TRmgLinePainterTile(this, point);
}

// Retail clears the rectangle row-major, then refreshes left, right, top and
// bottom borders in that order. The shared point local survives each call.
// Keep the asymmetric right-border lower reach: 0x4fa18c decrements height
// before comparing the bottom extent; the left border at 0x4fa125 does not.
// These bounds are unsigned and end-exclusive, including empty rectangles.
// Direct construction through the ordinary proxy ctor removes the extra
// factory-result copy: 86.0093% -> 92.6465%, with all ten retail calls retained.
// The selected entry-family source preserves all other RMG peaks. Moving
// the lower-y expression into the for clause instead falls to 82.9395%; its
// value is computed before the upper bound in retail but stored afterward.
// A named lower bound followed by for-clause assignment restores that order
// (94.5070%). All 300 neighbour-family combinations compiled, producing 105
// distinct code results; no point-painting variant improved its prior peak.
// Named/direct border proxies add no gain over the existing factory temporary.
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

// Both final painters call this ordinary constructor with their second-base
// address. Retail initializes the type/painter pair, copies the start point,
// then paints that member (not the caller's input) through 0x4fa3c0.
// Keep the shared walker on this side of the painting-library boundary:
// defining it even after the final constructors in rmg_support.cpp expands
// it into both 0x55ee50/0x55f3b0, lowering their exact bodies to 69.83%.
// This source partition preserves the calls; it is not a recovered TU name.
VA(0x004FA280, 0x30) // anchor-caller 0x55ee50/0x55f3b0; thiscall ret 0xc
TRmgLineWalker::TRmgLineWalker(
    TRmgLinePainterInterface* newPainter,
    int newRiverType,
    const TRmgGridPoint& start)
    : m_painter(newPainter), m_riverType(newRiverType), m_position(start)
{
    paintPoint(m_position);
}

// Retail uses two axis records, not independent x/y induction variables.
// It visits destination first, stepping back toward m_position; a minor-axis
// advance emits a second point before the major-axis advance. The final
// error test also applies when the distance is zero. Keep this ordering and
// the canonical point-painting calls; those calls can update neighbouring tiles.
// Exact: reference-bound axis arguments and a cached unsigned count-up bound
// reproduce all 272 bytes, including VC6's resulting countdown loop. The
// initial value-argument/countdown form was 79.13%; a reference-bound countdown
// reaches 99.45% but uses JE rather than retail's initial JBE. Reversing the
// major/minor arms preserves semantics but leaves their polarity at 99.41%.
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

// The line walk's shared point operation first removes an old line, assigns
// the new type and refreshes this tile. It snapshots every neighbouring type
// before refreshing any neighbour. Both passes use north-first tile directions;
// the first retains compound addition at 0x4fa489, the second expands it.
// Direct entry construction removes a retained implicit proxy copy and raises
// 70.1938% -> 79.9380%, without changing the factory's proven neighbour calls.
// The first neighbour pass still expands += where retail retains it; the
// rectangle and translated-point lifetimes also leave different stack slots.
// The 300-case neighbour family varied sum value/reference bindings and
// copy/assignment/coordinate construction with real compound-add calls, both
// separate and in the query operand. None exceeded 79.9380%; the low was
// 72.8295%. Keep the existing source chain while recovering the helper boundary.
VA(0x004FA3C0, 0x156) // anchor-caller 0x4fa280/0x4fa2b0; thiscall, ret 4
void TRmgLineWalker::paintPoint(const TRmgGridPoint& point)
{
    TRmgLinePainterTile tile(m_painter, point);
    int oldType = tile.getLand();
    if (oldType == m_riverType || tile.isBlocked())
        return;
    if (oldType)
        clearRmgLineRectangle(m_painter, TRmgGridRectangle(point, 1, 1));
    tile.setOverlay(m_riverType);
    refreshRmgLinePoint(m_painter, point);

    int riverType = m_riverType;
    TRmgLinePainterInterface* painter = m_painter;
    unsigned char available[TILE_DIR_COUNT];
    buildTileNeighbourMask(painter->m_size.m_x, painter->m_size.m_y,
                           point.m_x, point.m_y, available);
    unsigned char matches[TILE_DIR_COUNT];
    unsigned int direction;
    for (direction = 0; direction < TILE_DIR_COUNT; ++direction) {
        if (available[direction])
            matches[direction] = painter->at(point + g_tileDirections[direction]).getLand() == riverType;
        else
            matches[direction] = 0;
    }
    for (direction = 0; direction < TILE_DIR_COUNT; ++direction) {
        if (matches[direction])
            refreshRmgLinePoint(m_painter, point + g_tileDirections[direction]);
    }
}

// Refresh 0x4f9f77 builds the grid argument of the retained proxy call at
// 0x4f9f86 from the copied TPoint sum through this one-argument
// constructor. It is not the grid copy: retail's at() and getSize copy grid
// points memberwise, and a written copy constructor of any spelling
// reschedules them (see TRmgGridPoint in rmg.h). The conversion from the
// signed TPoint reproduces all 22 raw bytes without relocations.
VA(0x004FA520, 0x16) // anchor-callee 0x4f9f77; thiscall, ret 4
TRmgGridPoint::TRmgGridPoint(const TPoint& point)
    : m_x(point.m_x), m_y(point.m_y)
{
}

// TPoint's compound add: the receiver at refresh 0x4f9f51..0x4f9f5d is the
// TPoint copy of a grid point, so this is not the grid type's operator.
// Refresh 0x4f9f60 and line paintPoint's first neighbour pass retain this
// same two-dword add, returning the receiver for the subsequent value copy.
// There is no Dreamcast RMG inline declaration. One ordinary definition in
// this painting TU emits all 33 raw retail bytes while staying available for
// auto-inlining. The 168-state placement/lifetime family (48 code results)
// leaves every other tracked RMG score unchanged with this placement alone.
VA(0x004FA540, 0x21) // anchor-callers 0x4f9f00/0x4fa3c0; thiscall, ret 4
TPoint& TPoint::operator+=(const TPoint& offset)
{
    m_x += offset.m_x;
    m_y += offset.m_y;
    return *this;
}

// Constructor 0x5b3780 copies five caller arguments into the rule prefix,
// initializes 58 range records, then groups entries by frame/special flag.
// The base vptr survives through array initialization; the derived vptr is
// installed before the scan. Complete-only, with no Dreamcast counterpart.
// All 179 bytes match, including the shared range constructor's expansion.
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
// at 0x5b3780 builds that range at +0x1c/+0x20 from its copied entry array.
VA(0x005B3840, 0x0C)  // Complete-only pattern terrain rule
unsigned char TRmgPatternTerrainRule::hasEntries()
{
    return 0 < m_ranges[1].m_count;
}

// Both concrete terrain-rule deleting destructors call this retained base
// boundary. Retail restores the six-slot pure base vtable at 0x642c80.
VA(0x005B3850, 0x07)  // terrain-rule deleting destructors; Complete-only
TRmgTerrainRule::~TRmgTerrainRule()
{
}

// Vtables 0x642c98 and 0x642cb0 share the deleting wrapper at 0x5b3a50,
// which calls the base-only destructor at 0x5b3850. Neither derived rule
// owns its source table or has any additional destruction work.
TRmgPatternTerrainRule::~TRmgPatternTerrainRule()
{
}

TRmgTableTerrainRule::~TRmgTableTerrainRule()
{
}

// Vtable 0x642c98 slot 2 reads the byte at +4 in an eight-byte source entry.
// The pattern-rule constructor at 0x5b3780 copies the same entry records.
VA(0x005B3860, 0x11)  // Complete-only pattern terrain rule
unsigned char TRmgPatternTerrainRule::isSpecialFrame(int frame)
{
    return m_entries[frame].m_special;
}

// Each copied source entry is two dwords. Vtable 0x642c98 slot 3 returns
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

// Static initializer 0x5b3a10 passes the global at 0x6a4158. Retail clears
// its 116 ranges, then groups the 48 fixed records by transition and flips.
// No Dreamcast counterpart exists for this Complete-only table owner.
// Partial 96.61%: unsigned indexing restores the retail branch signedness
// (signed indexing: 95.76%). An explicit record-pointer loop gives 92.38%.
// Residual: VC6 anchors the scan at the Y-flip byte instead of X-flip and
// compares the record count rather than the fixed table's end address.
// Flat flip fields and the nested flip pair emit the same constructor bytes.
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
        const TRmgTerrainTransitionEntry* entry = &g_rmgTerrainPatterns[index];
        if (entry->m_frame != frame || entry->m_flipX != flipX
            || entry->m_flipY != flipY) {
            frame = entry->m_frame;
            flipX = entry->m_flipX;
            flipY = entry->m_flipY;
            range = &m_ranges[(frame * 2 + flipX) * 2 + flipY];
            range->m_firstIndex = index;
        }
        ++range->m_count;
    }
}

// The sole caller is the static initializer at 0x5b3da0. Retail clears the
// two inherited rule flags and installs vtable 0x642cb0.
VA(0x005B3A20, 0x11)  // Complete-only table terrain rule
TRmgTableTerrainRule::TRmgTableTerrainRule()
{
}

// Retail vtable 0x642cb0 slot 1 is this constant-false query.  The surrounding
// constructor at 0x5b3a20, vtable, fixed-table methods, and the first admitted
// painter method at 0x5b3dd0 place it in this Complete-only compiland.  There
// is no Dreamcast RMG counterpart; `xor al, al; ret` fixes the byte return.
// The direct constant-false body matched on the first scored candidate.
VA(0x005B3A40, 0x03)
unsigned char TRmgTableTerrainRule::hasEntries()
{
    return 0;
}

// Both concrete six-slot terrain-rule vtables use this ICF-folded deleting
// wrapper. The emitted table-rule closure calls the shared retained destructor
// at 0x5b3850 and has the same complete-object delete semantics.
VA_COMPGEN(0x005B3A50, 0x21, SCALAR_DELETING_DTOR, TRmgTableTerrainRule)

// Vtable 0x642cb0 slot 3 indexes the first dword of the fixed eight-byte
// transition records at 0x6424a8. There is no Dreamcast RMG counterpart.
VA(0x005B3A80, 0x11)
int TRmgTableTerrainRule::getEntry(int index)
{
    return g_rmgTerrainPatterns[index].m_frame;
}

// Slot 4 chooses from the first generated range when there is no old frame or
// the old pattern has a nonzero frame tag. A zero-tagged pattern preserves the
// caller's old index; keeping that index in EAX gives retail's shared return.
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

// Vtable 0x642cb0 slot 5. Reuse the old frame only when the fixed record
// matches the transition and both requested flips; otherwise choose from
// the corresponding four-way range. Table entries already encode flips,
// so the selected output flip is zero. Complete-only, no DC counterpart.
// Exact on the first candidate (116 bytes). The four data references name
// the fixed table at 0x6424a8, its +4/+5 flip fields, and ranges at 0x6a4158;
// the delinked image still labels these external data declarations by RVA.
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

// Provisional role spelling. The fastcall ABI and two-byte output are fixed
// by the call at 0x5b5f4e. All selector names are provisional retail roles.
// Before normalization (function): SelectTerrainTransition.
int __fastcall selectTerrainTransition(
    const int* neighbours, TRmgTerrainFlip* flip);

// Four reflections of the eight neighbour slots. Read from the pinned
// retail image; both flip bytes index this table independently.
// Before normalization: gRmgReflectedNeighbours.
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
// Before normalization (function): MakeTerrainFlip.
static TRmgTerrainFlip makeTerrainFlip(unsigned char x, unsigned char y)
{
    return TRmgTerrainFlip(x, y);
}

// The retained fill is exact. A 60-case tile-lifetime / packed-receiver /
// existing-setter matrix leaves paintRectangle at 72.8497%; crossing its
// top ten parents with six orders of the cache query/terrain/dimension
// helpers also leaves that caller flat. Both batches move other terrain
// bodies, but none improves a current score. Preserve this canonical fill;
// its missing expansion in paintRectangle remains a caller-boundary lead.
VA(0x005B3DD0, 0x6F)  // called and expanded in the retail terrain cluster
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

// Provisional role spelling. BuildNeighbourKinds expands this ordinary
// classifier seven times and retains the final southeast call at 0x5b6b8e.
// ECX/EDX hold the two terrain indices; both rule flags are read at +4.
// Retail returns no edge for equal terrain or a sand center, hard edge if
// either rule forbids blending, otherwise the center's non-dirt predicate.
// Exact. Named rule pointers with nested tests give this helper the inline
// cost buildNeighbourKinds needs (87..104 with plain point locals): with
// the plain `&&` form (cost 70) or named rules alone (82) the caller's
// seven expansions leave 114 units and the eighth kind test expands where
// retail calls it (77.66% / 87.59%); a byte local, two byte locals or a
// nested conditional return are byte-identical alternatives, and splitting
// the first test breaks the body (67.27%). 108-state family, 2026-09-12.
VA(0x005B3E40, 0x38)  // anchor-callee 0x5b6b8e; fastcall; retail-only
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

VA(0x005B3E80, 0x75F)  // fastcall call at 0x5b5f4e; retail-only
int __fastcall selectTerrainTransition(
    const int* neighbours, TRmgTerrainFlip* flip)
{
    // Retail +0x09..+0x7b: guarded local-static construction in this order,
    // with a registered empty cleanup. No Dreamcast RMG counterpart.
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

// A 45-cost grid copy site once restored both retained set _Init calls and
// the expanded packed-vector insert (23.33% -> 91.22%); with the trivial grid
// copy the assigned virtual result keeps them. Assign the virtual result
// into an existing local: this also expands the final size query, while erase
// remains a call, and recovers all 29 CFG blocks (97.92%). Direct construction
// and reference binding retain that size call. A grid member instead of the
// separate dimensions changes the constructor's earlier call boundaries.
// Residual: dimension stores and the area multiply in the first block.
// A named fill value changes those stores (96.33%); reversing the product
// operands and zero-initializing the local size are byte-neutral. Value/reference
// dimension queries and a shorter size scope are also flat. The retained brush
// constructor calls this ordinary body at 0x5b7297.
// Signed dimension fields, reversed dimension stores, a named area product,
// and moving the packed-cell flag initialization into its ctor body are flat.
// Copy-initializing the empty size temporary retains the wrong size call (92.04%).
// A focused 60-case matrix of dimension snapshots, member/local area operands,
// assignment-result references and named products is also flat at 97.9205%.
// A further 60-state family uses the ordinary getWidth/getHeight boundaries
// from paintTransitions, with five dimension bindings and three area-result
// lifetimes. Its 24 distinct objects and ten reproduced finalists add no
// tracked peak; the direct dimension stores remain the closest reconstruction.
VA(0x005B45F0, 0x26D) // anchor-callee 0x5b7297; retail-only
rmgTerrainPainter::rmgTerrainPainter(
    TRmgMapInterface* newAdapter, int terrain, int strength)
    : m_adapter(newAdapter), m_paintTerrain(terrain), m_transitionStrength(strength)
{
    TRmgGridPoint size;
    size = m_adapter->getSize();
    m_width = size.m_x;
    m_height = size.m_y;
    m_packedCells.resize(m_width * m_height, TRmgPackedTerrainCell());
}

VA(0x005B48D0, 0x8D)  // repeated caller identity in 0x5b3dd0..0x5b76f0
TRmgPackedTerrainCell* rmgTerrainPainter::getPackedCell(
    const TRmgGridPoint& point)
{
    unsigned int index = point.m_y * m_width + point.m_x;
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

unsigned int rmgTerrainPainter::getWidth() const
{
    return m_width;
}

unsigned int rmgTerrainPainter::getHeight() const
{
    return m_height;
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
    TRmgPackedTerrainCell& packed = m_packedCells[point.m_y * m_width + point.m_x];
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
// The comparison return costs 38 (free) against the guard form's 47. With
// the trivial grid copy and a direct-initialized neighbour it lets the
// final insert expand its pair constructor as retail does (paintPoint
// 97.29 -> 98.68%, 2026-09-12); the guard form kept that constructor out of
// line but retained the middle erase's distance wrapper, which now expands.
// Reading the configured terrain as the field, or swapping the operands,
// drops the caller below 90%.
int rmgTerrainPainter::getPaintTerrain() const
{
    return m_paintTerrain;
}

unsigned char rmgTerrainPainter::isPaintTerrain(const TRmgGridPoint& point)
{
    return getTerrain(point) == getPaintTerrain();
}

// The brush forwards its four unsigned bounds to this body at 0x5b76a6.
// Retail walks one mutable grid point, paints cells of a different terrain,
// and refreshes the base frame for cells already using the selected terrain.
// This ordinary caller precedes paintPoint in the retail terrain cluster;
// there is no Dreamcast RMG compiland or recovered source spelling.
// Residual (72.8497%): all 13 CFG blocks have the retail destinations, but
// initializePackedCell is called where retail expands the adapter read and
// cache fill. The bound loads, loop registers and tile-zero stores also differ.
// Sixty bounds/predicate/tile-lifetime candidates lift 69.9804% to this peak
// with named endpoints and the canonical isPaintTerrain call. Ten retained
// parents crossed with six for/while/guarded-do forms are all byte-flat here;
// no other terrain score changes. The nested cache-fill boundary stays open.
// Restoring this predecessor is byte-neutral for paintPoint's eight-byte
// multiplication residual. Sharing the base-frame/constructed-tile sequence
// through another helper leaves selectBaseFrame, the tile constructor and
// setTile called in paintPoint, contradicting its retail expansion (81.6203%).
// Exact (2026-09-12). Three things had to hold at once. Retail expands the
// cache read inside the terrain test AND its fill, which needs the test's
// nested budget above 346: with a direct `m_paintTerrain != getTerrain`
// test and the base-tile work in one ordinary helper there are two
// candidate sites from the test on (500 units; the predicate helper or an
// inline else branch leave 250). That helper alone drops this body to
// cost 157, a saved candidate the brush wrapper at 0x5b7690 then expands;
// walking the rectangle through the grid point's accessors keeps the body
// unsaved so the wrapper's retained call survives. The paint terrain reads
// first in the test for the retained compare order. paintPoint keeps its
// own inline copy of the base-tile block: sharing the helper costs it
// 98.68 -> 76.68/78.68%.
VA(0x005B4960, 0x1B2) // anchor-callee 0x5b7690; thiscall, ret 16; retail-only
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

// Paint a base tile, refresh its cache, then reconcile the two repair sets.
// Terrain rules that permit separated neighbours only release cardinal gaps;
// the other rules recheck every matching neighbour. The rectangle painter
// calls this body at 0x5b4a2d; RepairTerrainPoint and Finish share it.
// Names are provisional: this Complete-only code has no Dreamcast body.
// Exact: all 1483 retail bytes, including all 61 named relocations. Pass the
// constructed tile directly to setTile, ending its lifetime before the sets,
// and name the direction-table entry by const reference before translating.
// These source boundaries retain the guard-return terrain predicate and the
// compound grid-addition return. The temporary restores both width-first
// products; the direction reference restores the caller's inlining budget.
// Without that reference, the distance wrapper expands and loop registers
// change. Copying the direction value instead leaves 174 raw differences.
// No inlining pragma, dummy operation, or modified compiler is used. The
// passive compiler observations and identity controls are in regalloc.md 6n.
//
// The following controls refer to earlier named-tile checkpoints.
// At 99.9204%, eight raw bytes differ. The expanded cache
// products at 0x5b4f06/0x5b4fee load y and multiply by width; retail loads
// width and multiplies by y. All 61 named relocations, branch destinations,
// the 1483-byte length, 0x50-byte frame, and point translation are exact.
// The guard-return isPaintTerrain costs 47 rather than the comparison-return
// form's 38, crossing VC6's free-expansion cutoff of 40. This restores the
// inner three-argument _Distance call. With that boundary recovered, return
// the compound grid-addition result: a separate named return leaves 99.0163%
// with the wrong direction/translation registers. Reversing the compound
// field additions instead retains an unwanted helper (97.4864%).
//
// With the earlier comparison-return predicate, constructor + both accessors
// left 98.2893% under direct coordinate initialization. Copy initialization
// restored the original-x store (98.5805%); a named return then fixed the
// translation registers (99.5389%); ending the frame/tile lifetime before
// the worklists fixed the frame (99.5570%). The named-return observation
// does not transfer to the recovered guard-return predicate. Separate nearby
// declaration/assignment or direct initialization was neutral. Explicit zero
// flips, default flip arguments, and the flip factory had the same score.
//
// Negative controls before the point-copy recovery: plain terrain comparison
// and tile-field assignments leave 97.0506%, over-inlining the first cache
// read. A predicate without its configured-terrain accessor is neutral there;
// the two-accessor predicate alone restores the first call but expands the
// final one (96.6293%). The tile constructor alone blocks the inner tree find
// (85.7667%). Flattening setTile leaves 78.4213%; flattening frame selection
// leaves 90.9367% with a named frame or 92.2405% with direct frame assignment
// and the wrong entry load schedule. A by-value tile writer adds an entry
// copy (95.7884%); a nested packed-cell writer retains a call absent in retail
// (81.9566%). One nearby point shared across branches leaves 91.1519%.
// Explicit fieldwise grid copy construction retains an unwanted copy call
// (91.8861%); assignment-based copy construction is neutral. Direct sum
// construction, value offset arguments, signed-direction conversion (95.7902%)
// and free value-origin addition (92.3924%) change the translation schedule.
// Origin copy assignment leaves 93.5063%; coordinate accessors leave 97.0036%.
// Const results, named coordinates/terrain/index, loop guards, and index
// declaration before the mask do not restore the missing copy. A named set
// iterator changes find/end evaluation (89.1573%). A by-value terrain point
// adds input copies (72.9584%); cache-hit early return breaks the exact cache
// helper (93.4211% there). The recovered neighbour queue body is caller-neutral.
//
// A budget-preserving scratch C2 rejection of only the two unwanted copies
// reproduces the named retail call sequence at the old 97.0506% checkpoint,
// but keeps its storage deltas. This isolated the point-copy/lifetime problem;
// no modified-compiler object enters a matching build. Outer inline_depth(1)
// is byte-neutral, while pinning a direct cache read changes later decisions
// (91.4213%). All pragma probes were removed. Nested return-value selection
// in needsTerrainRepair changes the exact destructors' byte-result tests
// (95.0543% here); adjacent guards and cache multiplication operand reversal
// and a native-bool terrain predicate are neutral at the current checkpoint.
//
// Further controls at 99.5570%: coupled native-bool repair/separation returns,
// a byte result local, shared gap-return block, signed index/dimensions,
// named width, stepwise indexing,
// a width accessor, a cache-reference return, consistent configured-terrain
// accessors, early loop continues, negated iterator equality, and direct frame
// selection in the tile constructor leave the caller score unchanged. Grouping
// dimensions into a point also leaves this caller unchanged. Tile copy
// initialization adds two entry instructions (98.3183%); an owned flip value
// or explicit assignment-based grid copy constructor gives 98.5986%.
//
// The other direction-table caller at 0x4f9f00 retains a 22-byte point
// copy/conversion at 0x4fa520 and 33-byte compound addition at 0x4fa540. These
// prove operation boundaries there, not the classes' names or signedness.
// Applying signed-direction conversion before same-type grid addition here
// gives 98.0759%; signed translation followed by grid conversion gives
// 98.5986%. Both change the direction load schedule and keep the wrong erase
// overload. Preserve the current mixed-type arithmetic pending stronger proof.
//
// The retained three-argument _Distance body matches all 43 retail bytes at
// 0x5b8c70, including its _Inc call. A current-source scratch C2 rejection of
// only the inner wrapper expansion gives retail's 1483-byte length, but also
// changes earlier translation/backedge registers and leaves both commuted
// products. Thus the inline decision and storage deltas remain coupled;
// the diagnostic object is excluded from matching and the normal shim restored.
// At 99.9204%, product/addition commutation, coordinate locals, const nearby
// points, logically const queries with a mutable cache, and a linear-index
// helper with value/reference width arguments leave the same eight-byte
// caller residual. Unsigned long grid coordinates were neutral at 99.5570%.
// A named cache-cell reference, a pointer rather than reference cache parameter,
// and signed row/width intermediates also leave those eight bytes unchanged.
// Binding nearby to the translated temporary by const reference instead gives
// 98.5986% and over-expands the distance wrapper; named reference operands in
// the cache calculation disturb translation/backedge registers. Both were
// reverted. The restored rectangle caller supplies an independent instance
// of the retail width-first product without changing this caller's bytes.
// Passive C2 write watchpoints locate both operand reversals in its second
// expression pass, before native lowering. A diagnostic-only compiler swap
// of those two source lists reproduces all 1483 retail bytes; ordinary VC6
// still leaves the eight-byte residual. See docs/vc6/regalloc.md section 6n
// for the operand ranking, identity controls, and limits of that experiment.
// Current-checkpoint controls that initialize index from width or row, then
// apply *= and += in getPackedCell, also leave the same eight raw differences.
// With the corrected hash-tie evidence, named terrain return values, const
// entry frame/tile values, and a named direction reference remain byte-flat.
// A named cell pointer/reference in getTerrain leaves 235 raw differences;
// predicate comparison locals or a direct frame argument over-expand _Distance.
// A copied direction value changes the function extent. These controls retain
// no source changes; the actual sort ranks and hash inputs are in section 6n.
// Following the width-address creation back to setTile, a pointer cache alias,
// reversed product, and x-first sum remain byte-flat. A named setter index,
// a braced if/else predicate, and a flag-result predicate over-expand _Distance
// and still emit both residual products row-first. A named setter width also
// reverses the already-exact entry product. Reversing the predicate guard is
// byte-flat. All of these controls were compiled separately and reverted.
// At the same checkpoint, explicit copy initialization at all four cardinal
// sites leaves 117 raw differences; translating them through the existing
// point-addition helper grows the section to 1552 bytes. Both keep the two
// row-first products. Defaulted flip constructor parameters and assigning
// the two zero flips in the constructor body remain at eight differences.
// A used width reference bound before or after the setter's adapter call also
// over-expands _Distance and keeps both products row-first.
// Nesting the strength call directly in selectBaseFrame's virtual-call
// argument grows the section to 1504 bytes and keeps both products row-first.
// Implicit rather than explicit tile padding leaves the same eight bytes;
// frame-first constructor assignments leave 25. An owned flip value and
// reference constructor parameters over-expand _Distance and keep row first.
// Directly binding the constructed tile to setTile fixes both products;
// retaining a named tile via copy initialization or const-reference binding
// also fixes those products but changes the entry schedule/extent. With the
// direct temporary, comparison-return isPaintTerrain leaves 98.68% (including
// the four-argument distance call and translation registers); a named grid
// return or direct frame argument does not close that coupled residual. The
// retained guard predicate plus named direction reference closes it entirely.
// Residual (98.6781%): the middle erase's three-argument _Distance wrapper
// expands where retail calls it (nested budget 43 against cost 41) while the
// final insert's pair constructor now expands as retail does; the two sit
// on opposite sides of the same budget, and the direction pointer holds EBX
// where retail keeps EDX downstream of it. A 32-state family over the
// coordinate constructor's spelling, the predicate form and the neighbour
// binding separates only this pair (copy-initialized neighbour 96.87%,
// unnamed offset 91.60%); the sum/conversion forms of family 3 and the
// cache pair's spellings (family 7) move nothing here.
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
        if (point.m_y < m_height - 1) {
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
        if (point.m_x < m_width - 1) {
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
            m_width, m_height, point.m_x, point.m_y, neighbourExists);
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

// The first differing vertical neighbour and first differing horizontal
// neighbour enter the secondary set. The four diagonals additionally require
// a terrain rule that forbids separated neighbours. North/south and west/east
// are alternative arms: retail 0x5b5159 and 0x5b521f skip the opposite arm.
// The cardinal probes construct a fresh point for insertion; the diagonal
// probes retain their point and terrain value for the rule test and insertion.
// Residual (78.1881%, 63.63% before the secondary-queue helper below and
// the dimension accessors). Retail expands set::insert in all eight arms,
// calls the node insert everywhere, calls the result-pair copy in the four
// cardinal arms (merged into two tails) and expands it in the diagonals;
// it calls the cache read in the cardinal and north-west/north-east arms
// and expands it with its fill in the south-east arm. With the helper the
// pair copy is refused where its nested budget is under 105 and the read
// where its budget is under 90: north/south get 54/62 (their whole insert
// stays a call), west/east 69/81 (right), north-west 91 (pair refused,
// read at 106 expanded), north-east 108/120, south-east read 351 with the
// fill at 130 refused. Per-arm paint-terrain and rule accessors and a
// direct south-east insert (33 states) move nothing; a helper that also
// tests the terrain starves its own insert.
void rmgTerrainPainter::queueSecondaryPoint(const TRmgGridPoint& point)
{
    m_secondaryPoints.insert(point);
}

VA(0x005B50F0, 0x34E) // anchor-callee 0x5b4c72, 0x5b50dd; thiscall, ret 4
void rmgTerrainPainter::queueOtherTerrainNeighbours(const TRmgGridPoint& point)
{
    if (point.m_y > 0
        && getTerrain(TRmgGridPoint(point.m_x, point.m_y - 1)) != m_paintTerrain) {
        queueSecondaryPoint(TRmgGridPoint(point.m_x, point.m_y - 1));
    } else if (point.m_y < getHeight() - 1
        && getTerrain(TRmgGridPoint(point.m_x, point.m_y + 1)) != m_paintTerrain) {
        queueSecondaryPoint(TRmgGridPoint(point.m_x, point.m_y + 1));
    }
    if (point.m_x > 0
        && getTerrain(TRmgGridPoint(point.m_x - 1, point.m_y)) != m_paintTerrain) {
        queueSecondaryPoint(TRmgGridPoint(point.m_x - 1, point.m_y));
    } else if (point.m_x < getWidth() - 1
        && getTerrain(TRmgGridPoint(point.m_x + 1, point.m_y)) != m_paintTerrain) {
        queueSecondaryPoint(TRmgGridPoint(point.m_x + 1, point.m_y));
    }
    if (point.m_x > 0 && point.m_y > 0) {
        TRmgGridPoint nearby(point.m_x - 1, point.m_y - 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            queueSecondaryPoint(nearby);
    }
    if (point.m_x < getWidth() - 1 && point.m_y > 0) {
        TRmgGridPoint nearby(point.m_x + 1, point.m_y - 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            queueSecondaryPoint(nearby);
    }
    if (point.m_x > 0 && point.m_y < getHeight() - 1) {
        TRmgGridPoint nearby(point.m_x - 1, point.m_y + 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            queueSecondaryPoint(nearby);
    }
    if (point.m_x < getWidth() - 1 && point.m_y < getHeight() - 1) {
        TRmgGridPoint nearby(point.m_x + 1, point.m_y + 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            queueSecondaryPoint(nearby);
    }
}

// Own-terrain checks share the predicates used with the selected paint
// terrain. Retail retains the nested predicate in the former and expands
// the latter at the four adjacent-row/column probes.
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

// The unsigned grid/ref-argument correction is independently proved by the
// set comparator and the retained grid constructor. It changes the nested
// accessor expansion here (74.76 -> 42.02%). Shared SetTile initially lowers
// that to 38.6271%; shared strength-then-base-frame selection restores
// 74.7320% with the unsigned type intact. MAX/history retain 74.7623%.
// Before the unsigned-grid correction (74.7623%), the candidate retained
// seven of retail's first twelve GetPackedCell calls and expanded five only
// as far as InitializePackedCell. Both cache helpers remain exact.
// Retail also evaluates GetTransitionStrength before loading the rule's
// virtual receiver (as in neighboring 0x5b4960); spelling two strength locals
// reproduced that order but changed earlier inlining (41.51%). Earlier
// rejected controls: GetTerrain via GetTile adds eleven calls (75.28%);
// reversed dimension product, explicit vector fill, postfix edge increments,
// function-scope terrain, transition initializer, and earlier tile declaration
// are byte-flat. These do not establish the missing source/helper state.
// Scoped neighbour coordinates followed by the canonical += translation
// restore the retained vector _Ufill call and reach 81.5196%. A value-returning
// point + offset instead introduces 11 copy-ctor and six += calls (74.9229%);
// copying the point into each scoped local rather than constructing from its
// coordinates is 78.6334%. Translating the three left-edge neighbours is
// byte-identical to the retained direct-coordinate form.
// Dimension accessor calls then reach 84.0645% (2240 bytes, 68/66 blocks).
// Unused accessor definitions leave the 81.5196% body unchanged. Only the
// right-edge southwest cache read still over-inlines: getPackedCell costs
// 90 and receives 104; the following bottom-edge reads receive 127/145.
// A shared index helper and both comparison/guard-return terrain predicates
// are byte-identical at this checkpoint; none is retained as extra interface.
VA(0x005B5A70, 0x8A7)  // caller cluster reaches Complete RMG; retail-only
void rmgTerrainPainter::paintTransitions()
{
    std::vector<unsigned char> edgeCounts(getWidth() * getHeight());
    TRmgGridPoint point;

    for (point.m_y = 0; point.m_y < getHeight() - 1; ++point.m_y) {
        int terrain = getTerrain(TRmgGridPoint(0, point.m_y));

        if (getTerrain(TRmgGridPoint(1, point.m_y)) != terrain) {
            ++edgeCounts[point.m_y * getWidth()];
            ++edgeCounts[point.m_y * getWidth() + 1];
        }
        if (getTerrain(TRmgGridPoint(1, point.m_y + 1)) != terrain) {
            ++edgeCounts[point.m_y * getWidth()];
            ++edgeCounts[(point.m_y + 1) * getWidth() + 1];
        }
        if (getTerrain(TRmgGridPoint(0, point.m_y + 1)) != terrain) {
            ++edgeCounts[point.m_y * getWidth()];
            ++edgeCounts[(point.m_y + 1) * getWidth()];
        }

        for (point.m_x = 1; point.m_x < getWidth() - 1; ++point.m_x) {
            terrain = getTerrain(point);

            {
                TRmgGridPoint nearby(point.m_x, point.m_y);
                nearby += TPoint(1, 0);
                if (getTerrain(nearby) != terrain) {
                    ++edgeCounts[point.m_y * getWidth() + point.m_x];
                    ++edgeCounts[point.m_y * getWidth() + point.m_x + 1];
                }
            }
            {
                TRmgGridPoint nearby(point.m_x, point.m_y);
                nearby += TPoint(1, 1);
                if (getTerrain(nearby) != terrain) {
                    ++edgeCounts[point.m_y * getWidth() + point.m_x];
                    ++edgeCounts[(point.m_y + 1) * getWidth() + point.m_x + 1];
                }
            }
            {
                TRmgGridPoint nearby(point.m_x, point.m_y);
                nearby += TPoint(0, 1);
                if (getTerrain(nearby) != terrain) {
                    ++edgeCounts[point.m_y * getWidth() + point.m_x];
                    ++edgeCounts[(point.m_y + 1) * getWidth() + point.m_x];
                }
            }
            {
                TRmgGridPoint nearby(point.m_x, point.m_y);
                nearby += TPoint(-1, 1);
                if (getTerrain(nearby) != terrain) {
                    ++edgeCounts[point.m_y * getWidth() + point.m_x];
                    ++edgeCounts[(point.m_y + 1) * getWidth() + point.m_x - 1];
                }
            }
        }

        terrain = getTerrain(point);
        {
            TRmgGridPoint nearby(point.m_x, point.m_y);
            nearby += TPoint(0, 1);
            if (getTerrain(nearby) != terrain) {
                ++edgeCounts[point.m_y * getWidth() + point.m_x];
                ++edgeCounts[(point.m_y + 1) * getWidth() + point.m_x];
            }
        }
        {
            TRmgGridPoint nearby(point.m_x, point.m_y);
            nearby += TPoint(-1, 1);
            if (getTerrain(nearby) != terrain) {
                ++edgeCounts[point.m_y * getWidth() + point.m_x];
                ++edgeCounts[(point.m_y + 1) * getWidth() + point.m_x - 1];
            }
        }
    }

    for (point.m_x = 0; point.m_x < getWidth() - 1; ++point.m_x) {
        int terrain = getTerrain(point);
        {
            TRmgGridPoint nearby(point.m_x, point.m_y);
            nearby += TPoint(1, 0);
            if (getTerrain(nearby) != terrain) {
                ++edgeCounts[point.m_y * getWidth() + point.m_x];
                ++edgeCounts[point.m_y * getWidth() + point.m_x + 1];
            }
        }
    }

    for (point.m_y = 0; point.m_y < getHeight(); ++point.m_y) {
        for (point.m_x = 0; point.m_x < getWidth(); ++point.m_x) {
            unsigned int index = point.m_y * getWidth() + point.m_x;

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

// These ordinary gap predicates are both retained and expanded within
// RepairTerrainPoint. Their unsigned edge checks corroborate the grid type.
// The retained 263/262-byte bodies do not distinguish int from byte return;
// the callers' test al,al proves that only the byte result is consumed.
// Both bodies and the reference-taking grid constructor pass a raw-byte
// audit after resolving their real retail relocation destinations.
// Under the 157638d4 context, 60 named-point lifetime/scope/site combinations
// do not improve any terrain score: best repairTerrainPoint is 90.4604%,
// while its modified vertical predicate falls to 78.9709%. Crossing the ten
// best parents with six orders of these predicates and paintTransitions
// also gives no gain. Keep both exact canonical bodies and the original order.
VA(0x005B6320, 0x107) // anchor-callee 0x5b569f; retail-only
unsigned char rmgTerrainPainter::isHorizontalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.m_x > 0 && point.m_x < getWidth() - 1
        && getTerrain(TRmgGridPoint(point.m_x - 1, point.m_y)) != terrain
        && getTerrain(TRmgGridPoint(point.m_x + 1, point.m_y)) != terrain;
}

VA(0x005B6430, 0x106) // anchor-callee 0x5b545f; retail-only
unsigned char rmgTerrainPainter::isVerticalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.m_y > 0 && point.m_y < getHeight() - 1
        && getTerrain(TRmgGridPoint(point.m_x, point.m_y - 1)) != terrain
        && getTerrain(TRmgGridPoint(point.m_x, point.m_y + 1)) != terrain;
}

// Cardinal neighbours use coordinates clamped to the map edge. A diagonal
// contributes only when at least one adjoining cardinal cell also matches.
// Residual (MAX 77.2609%, rechecked 2026-09-07): retail retains the center
// and four cardinal getPackedCell calls, then expands the diagonals. The
// candidate already expands east; southeast retains initializePackedCell
// where retail expands its adapter read. Passive VC6 trace (identical
// 656-byte candidate) gives east budget 92 against getPackedCell's cost 90.
// That expansion consumes 90; southeast later gives initializePackedCell
// budget 107 against its cost 128. The two differences are sequentially
// coupled, not independent pins to add. Caller cb=432, initial budget=1000.
// Controls: existing getWidth/getHeight calls are byte-neutral; replacing
// the four diagonal conjunctions with explicit if/else guards gives 60.3877%
// versus 77.2609%. The source already retains the canonical terrain/cache
// helpers. RMG has no Dreamcast counterpart to supply the missing boundary.
VA(0x005B6540, 0x2CA) // anchor-callee 0x5b58f8, 0x5b681e; retail-only
void rmgTerrainPainter::buildMatchingNeighbourMask(
    const TRmgGridPoint& point, unsigned char* matches)
{
    int terrain = getTerrain(point);
    unsigned int north = point.m_y > 0 ? point.m_y - 1 : point.m_y;
    unsigned int south = point.m_y < m_height - 1 ? point.m_y + 1 : point.m_y;
    unsigned int west = point.m_x > 0 ? point.m_x - 1 : point.m_x;
    unsigned int east = point.m_x < m_width - 1 ? point.m_x + 1 : point.m_x;

    matches[TILE_DIR_NORTH] = getTerrain(TRmgGridPoint(point.m_x, north)) == terrain;
    matches[TILE_DIR_SOUTH] = getTerrain(TRmgGridPoint(point.m_x, south)) == terrain;
    matches[TILE_DIR_WEST] = getTerrain(TRmgGridPoint(west, point.m_y)) == terrain;
    matches[TILE_DIR_EAST] = getTerrain(TRmgGridPoint(east, point.m_y)) == terrain;
    matches[TILE_DIR_NORTHWEST] =
        (matches[TILE_DIR_NORTH] || matches[TILE_DIR_WEST])
        && getTerrain(TRmgGridPoint(west, north)) == terrain;
    matches[TILE_DIR_NORTHEAST] =
        (matches[TILE_DIR_NORTH] || matches[TILE_DIR_EAST])
        && getTerrain(TRmgGridPoint(east, north)) == terrain;
    matches[TILE_DIR_SOUTHWEST] =
        (matches[TILE_DIR_SOUTH] || matches[TILE_DIR_WEST])
        && getTerrain(TRmgGridPoint(west, south)) == terrain;
    matches[TILE_DIR_SOUTHEAST] =
        (matches[TILE_DIR_SOUTH] || matches[TILE_DIR_EAST])
        && getTerrain(TRmgGridPoint(east, south)) == terrain;
}

// The final top-tested loop reuses the preceding scan's known zero entry.
// A do/infinite-for form duplicates its increment and false epilogue
// (85.17%); putting the increment in the mask test leaves an extra test
// (91.27%). Separate false returns leave three short branches at
// +0x20/+0x52/+0x72 choosing the +0x64 epilogue instead of retail's +0x44
// (99.7458%); native bool is neutral. Sharing the return in the first
// empty-run scan reproduces all 132 raw bytes, including those destinations.
// The initial scan can return directly while the two later backward joins
// retain that early epilogue: all 132 bytes and the helper relocation are
// unchanged. Eight independent return combinations check each source edge;
// removing either remaining join changes its branch destination. Keep the
// direction lifetime; VC6 still duplicates the later +0x64 false epilogue.
// Positive continue-scan/else-return scopes score 87.7966% for the first
// surviving join and 99.6610% for the final one, versus 100%. A shared
// bool/byte/int false result is lower (81.5254..81.8644%).
VA(0x005B6810, 0x84) // anchor-callee 0x5b58e4; retail-only
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

// Retail reads the center, clamps north/south/west/east, then classifies
// N/S/W/E/NW/NE/SW/SE in that order. Every cache query is retained. The final
// classifier call is the source boundary shared with seven earlier expansions.
// Source-family result: scoped reference-bound point temporaries and named
// terrain results recover all ten call boundaries and all 61 retail blocks
// (87.5851 -> 94.9255). Direct temporaries expand the last classifier too.
// Residual: register scheduling begins to differ at +0xb4; bounds-accessor
// spelling and independent MapItem::clear snapshot order do not improve it.
// Exact (2026-09-12): plain point locals per block (a const reference bound
// to a constructed temporary adds a copy site each) and the neighbour-kind
// helper's cost window keep all nine cache calls and retail's single kind
// call for the south-east block; see the helper above.
VA(0x005B68A0, 0x2FF)  // thiscall at 0x5b5f45; retail-only
void rmgTerrainPainter::buildNeighbourKinds(
    const TRmgGridPoint& point, int* neighbours)
{
    int terrain = getTerrain(point);
    unsigned int north = point.m_y > 0 ? point.m_y - 1 : point.m_y;
    unsigned int south = point.m_y < m_height - 1 ? point.m_y + 1 : point.m_y;
    unsigned int west = point.m_x > 0 ? point.m_x - 1 : point.m_x;
    unsigned int east = point.m_x < m_width - 1 ? point.m_x + 1 : point.m_x;

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

// Provisional name for the common reference-returning signed clamp. Retail
// checkFirstDiagonal +0xfe/+0x126/+0x17b/+0x1a8 and checkSecondDiagonal
// +0xb1/+0x105 select one of three operand addresses before loading it.
// Keep this source boundary: a value-return clamp discards those lifetimes.
// The else chain costs 64 against the plain form's 56; the eight units are
// what checkSecondDiagonal's first neighbour query needs below 90 (the
// cache read stays a call as in retail) once the point and offset
// accessors give it ten remaining sites.
static const int& clampRmgTerrainCoordinate(
    const int& value, const int& minimum, const int& maximum)
{
    if (value < minimum)
        return minimum;
    else if (value > maximum)
        return maximum;
    else
        return value;
}

// Retail's guarded table at 0x6a5260 has two signed offsets per reflection.
// The first query clamps y then x; the second assigns x before clamping y.
// TPoint supplies the canonical signed two-dword construction. As with the
// selector's flips array, VC6 owns the local-static guard and cleanup thunk.
// Source-family result: real dimension accessors, staged point fields and
// the early-return reference clamp reach 89.7363 (direct member extents and
// a constructed/reused point start at 78.2388). The signed offset initializer
// already agrees byte-for-byte. Cache-call expansion and stack homes remain.
// Grid-point and TPoint accessors (2026-09-12) give the first neighbour
// query twelve remaining sites, so its cache read stays a call as in retail
// while the last query expands the read and its fill (calls now agree,
// 89.74 -> 90.71%). A separate second point also refuses that read but
// costs a frame slot (0x30 against 0x28). Residual: register binding across
// the clamp results (eax/edx/ecx roles) with the schedule aligned; why-reg's
// catalog has no knob for it.
VA(0x005B6BA0, 0x24C)  // transition 2/8 tests; retail-only
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
    TRmgGridPoint nearby;
    nearby.setX(clampRmgTerrainCoordinate(
        static_cast<int>(point.getX()) + pair[0].getX(), 0, static_cast<int>(getWidth()) - 1));
    nearby.setY(clampRmgTerrainCoordinate(
        static_cast<int>(point.getY()) + pair[0].getY(), 0, static_cast<int>(getHeight()) - 1));
    if (getTerrain(nearby) == terrain)
        return 1;
    nearby.setX(clampRmgTerrainCoordinate(
        static_cast<int>(point.getX()) + pair[1].getX(), 0, static_cast<int>(getWidth()) - 1));
    nearby.setY(clampRmgTerrainCoordinate(
        static_cast<int>(point.getY()) + pair[1].getY(), 0, static_cast<int>(getHeight()) - 1));
    return getTerrain(nearby) == terrain;
}

// Four signed two-cell offsets at 0x6a3d68 use the same flip index. The
// first query changes only x; the second changes only y. Either terrain
// mismatch succeeds. The final cache query expands in the retail body.
// Copy-initializing the first point and naming a separate second point
// raises its checkpoint from 70.7500 to 71.9615. The shared clamp's early-return
// form gives CUR 70.3141 without changing this body: MAX stays 71.9615.
// Residual: the center cache query expands where retail retains its call.
// Grid-point/TPoint accessors, the width and height accessors and the else
// chain in the shared clamp put the first neighbour query at ten remaining
// sites and 894 units (89 < 90), so its cache read stays a call as in
// retail and the second query expands the read and its fill: 71.96 ->
// 94.83% with copy-initialized points, 99.19% constructing the first point
// directly. Residual: the frame is 0x30 against retail's 0x28; three
// homed slots where retail keeps registers.
VA(0x005B6E00, 0x1B3)  // transition 5/11 tests; retail-only
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
        clampRmgTerrainCoordinate(static_cast<int>(point.getX()) + offset.getX(),
            0, static_cast<int>(getWidth()) - 1), point.getY());
    if (getTerrain(nearby) != terrain)
        return 1;
    TRmgGridPoint nextPoint;
    nextPoint.setX(point.getX());
    nextPoint.setY(clampRmgTerrainCoordinate(
        static_cast<int>(point.getY()) + offset.getY(), 0, static_cast<int>(getHeight()) - 1));
    return getTerrain(nextPoint) != terrain;
}

// Complete-only retail 0x5b6fd0: inspect west, north, east, then south.
// Each in-bounds neighbour of the requested terrain halves the unsigned
// strength only when the rule's slot-2 frame predicate succeeds. The two
// separate cache queries and the scoped point value are visible in retail;
// the later queries expand further than the first four retained calls.
// SHR at +0x6a/+0xb6/+0x121/+0x25c proves logical, not signed division.
// Generated source families: preserve dimension accessor calls (63.4074 ->
// 70.1029). A separate frame accessor, reference-bound rule or point, and
// copy-initialized point values do not recover the missing cache boundaries.
// Residual: the first frame query expands getPackedCell where retail calls
// it; later source call/expansion decisions are also displaced. No pins.
// A 60-case point/query matrix and a 60-case follow-up crossing ten parents
// with canonical cache-helper definition order both retain 70.1029% as best,
// with no collateral gain. Shared/copied points and named frame queries lower
// the score; moving the existing helper definitions leaves the leading caller
// unchanged. Reopening this family requires new evidence, not resampling it.
// Retail keeps five of the eight cache reads as calls, expands the east
// frame read with its fill called and both south reads with their fills
// expanded; here the four frame reads all expand (2026-09-12 trace). A
// painter-level getFrame(point) sibling of getTerrain drops this body to
// 32.53%; it is not the boundary.
VA(0x005B6FD0, 0x271)  // base-frame selection call; retail-only
int rmgTerrainPainter::getTransitionStrength(
    const TRmgGridPoint& point, int terrain)
{
    unsigned int strength = m_transitionStrength;
    TRmgTerrainRule* rule = g_rmgTerrainRules[terrain];
    if (point.m_x > 0) {
        TRmgGridPoint nearby(point.m_x - 1, point.m_y);
        if (getTerrain(nearby) == terrain
            && rule->isSpecialFrame(getPackedCell(nearby)->getFrame()))
            strength >>= 1;
    }
    if (point.m_y > 0) {
        TRmgGridPoint nearby(point.m_x, point.m_y - 1);
        if (getTerrain(nearby) == terrain
            && rule->isSpecialFrame(getPackedCell(nearby)->getFrame()))
            strength >>= 1;
    }
    if (point.m_x < getWidth() - 1) {
        TRmgGridPoint nearby(point.m_x + 1, point.m_y);
        if (getTerrain(nearby) == terrain
            && rule->isSpecialFrame(getPackedCell(nearby)->getFrame()))
            strength >>= 1;
    }
    if (point.m_y < getHeight() - 1) {
        TRmgGridPoint nearby(point.m_x, point.m_y + 1);
        if (getTerrain(nearby) == terrain
            && rule->isSpecialFrame(getPackedCell(nearby)->getFrame()))
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

// Residual (78.6021%): ownership cleanup and the completion worklist expand,
// but needsTerrainRepair remains a call where retail expands it and retains
// its nested cache/gap queries. The standalone painter destructor is exact.
// Sixty snapshot/condition/loop forms, ten parents crossed with six predicate
// forms, and ten joint parents crossed with six helper orders do not improve
// this boundary. Keep canonical auto_ptr cleanup and the ordinary finish;
// the predicate family independently improves repairTerrainPoint above.
// Fifty independent primary/secondary snapshot combinations also stay at
// this peak or lower: the two snapshot sites do not expose an intermediate
// expansion state in the measured five-lifetime/two-condition family.
// Copy-through-assignment grid construction expands the predicate (92.1398%),
// but loses the retained copy body and lowers exact changeTerrain to 81.3309%.
// Mixed construction, explicit assignment and returned-value controls do not
// recover all three boundaries together; the canonical copy interface stays.
VA(0x005B72F0, 0x225) // anchor-callee 0x540207; auto_ptr ownership cleanup
TRmgTerrainBrush::~TRmgTerrainBrush()
{
}

// Exact: all 362 raw bytes after 14 relocations; all 13 blocks agree.
// Retail expands erase(key) and the three-argument _Distance wrapper but
// calls its tagged four-argument body at 0x5b8cd0; that call needs the
// erase body under 231 units, which the painter's changeTerrain cost sets
// (see its note). The final getPackedCell expansion keeps its nested
// initializePackedCell call, as retail does.
VA(0x005B7520, 0x16A) // anchor-callee 0x5401c3; retail-only
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

VA(0x005B76F0, 0x209) // anchor-callee 0x5b76d9; retained painter destructor
rmgTerrainPainter::~rmgTerrainPainter()
{
    finish();
}

// The four late point constructions in RepairTerrainPoint pass x and y by
// reference. The retained two-store body is 24 bytes including ret 8.
VA_COMPGEN(0x005B76B0, 0x18, CLASS_CTOR, TRmgGridPoint)

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

// PaintPoint and changeTerrain erase points by key. Retail 0x5b7f60
// obtains upper/lower bounds, counts their iterator range, erases that
// range, and returns size_type (ret 4). This is public erase(key), not
// private _Erase(node): the old TREE_ERASE probe paired the wrong body
// and scored 36.62%. The existing set calls naturally emit this overload,
// which matches all 89 bytes under the correct TREE_ERASE_KEY claim.
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

// The one-point lookup calls _Lbound and tests y/x before returning an
// iterator through the hidden result pointer (ret 8). Residual 99.4634%:
// ECX/EDX roles differ in the expanded comparison; CFG and calls agree.
// Banked MAX 100%: two comparator batches (24 branch/binding forms, then
// 108 capture/operand forms) found 28 exact observations. A full build with
// `unsigned int ry = right.m_y; unsigned int ly = left.m_y;` before the
// lexical comparison verified and banked find's exact body. That form lowers
// standalone operator< to 98.75%; restoring its direct expression recovers
// its 100% while find's unchanged-source MAX retains the exact observation.
// Simultaneous CUR exactness remains unresolved; no candidate closes both.
VA_COMPGEN(0x005B7FC0, 0x57, TREE_FIND, TRmgGridPoint)

// The terrain painter constructor erases a range of packed two-byte cells.
// The naturally emitted specialization agrees with all 53 retail bytes.
VA_COMPGEN(0x005B8020, 0x35, VECTOR_ERASE, TRmgPackedTerrainCell)

// paintTransitions' edge-count vector constructor retains this byte fill
// after the recovered neighbour scopes and coordinate translation. Retail's
// zero-count guard, null destination guard and one-byte stride identify the
// canonical unsigned-char specialization independently of the caller. All
// 36 bytes agree on the first full checkpoint; the parked entry predated
// the caller's recovered neighbour scopes.
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

// The painter owns std::set<TRmgGridPoint> work queues. Their retained tree
// teardown, allocator release, and VC6 lock scope identify this destructor.
VA_COMPGEN(0x005B4860, 0x6E, IMPLICIT_DTOR, set)

// erase(key) in TRmgTerrainBrush::changeTerrain retains Dinkumware's
// public distance wrapper and its category-dispatched overload. The wrapper
// increments the caller's count directly; the unused tag argument accounts
// for the tagged body's missing self-store.
VA_COMPGEN(0x005B8C70, 0x2B, STD_DISTANCE, TRmgGridPoint)
VA_COMPGEN(0x005B8CD0, 0x28, STD_DISTANCE_TAGGED, TRmgGridPoint)

// The set lookup at 0x5b4e96 retains this free comparison. Its unsigned
// y-then-x ordering also appears in the tree's expanded comparisons.
VA(0x005B8CA0, 0x20) // anchor-callee 0x5b4e96; fastcall, two point references
bool operator<(const TRmgGridPoint& left, const TRmgGridPoint& right)
{
    return left.m_y < right.m_y || (left.m_y == right.m_y && left.m_x < right.m_x);
}
