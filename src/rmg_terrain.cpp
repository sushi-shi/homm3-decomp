// rmg_terrain.cpp - Complete-only random-map terrain transition support.
//
// The Dreamcast build contains no random-map generator compiland. Function
// ownership, field layout, helper boundaries, and call/expansion decisions in
// this unit therefore come directly from the retail x86 cluster.
#include <va.h>
#include "rmg_terrain.h"
#include "exceptions.h"
#include "tiles.h"

DATA(0x00642BD8) extern TRmgTerrainRule* const g_rmgTerrainRules[];

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

// Vtable 0x642cb0 slot 3 indexes the first dword of the fixed eight-byte
// transition records at 0x6424a8. There is no Dreamcast RMG counterpart.
VA(0x005B3A80, 0x11)
int TRmgTableTerrainRule::getEntry(int index)
{
    return g_rmgTerrainPatterns[index].m_frame;
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

// The grid copy constructor restores both retained set _Init calls and the
// expanded packed-vector insert (23.33% -> 91.22%). Assign the virtual result
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

// Provisional terrain-comparison interface, inferred from the first
// eight-neighbour read at retail 0x5b4e55. Both accessors participate:
// flattening the configured-terrain read into the predicate expands that
// cache call. Keep these ordinary helpers and the base-tile constructor;
// their combined expansions recover the first and final cache boundaries.
// The explicit guard returns below also preserve retail's inner distance
// wrapper. Flattening them to return the comparison drops the predicate's
// measured cost from 47 to 38 and over-expands that later library call.
int rmgTerrainPainter::getPaintTerrain() const
{
    return m_paintTerrain;
}

unsigned char rmgTerrainPainter::isPaintTerrain(const TRmgGridPoint& point)
{
    if (getTerrain(point) == getPaintTerrain())
        return 1;
    return 0;
}

// The brush forwards its four unsigned bounds to this body at 0x5b76a6.
// Retail walks one mutable grid point, paints cells of a different terrain,
// and refreshes the base frame for cells already using the selected terrain.
// This ordinary caller precedes paintPoint in the retail terrain cluster;
// there is no Dreamcast RMG compiland or recovered source spelling.
// Residual (69.9804%): all 13 CFG blocks have the retail destinations, but
// initializePackedCell is called where retail expands the adapter read and
// cache fill. The bound loads, loop registers and tile-zero stores also differ.
// Restoring this predecessor is byte-neutral for paintPoint's eight-byte
// multiplication residual. Sharing the base-frame/constructed-tile sequence
// through another helper leaves selectBaseFrame, the tile constructor and
// setTile called in paintPoint, contradicting its retail expansion (81.6203%).
VA(0x005B4960, 0x1B2) // anchor-callee 0x5b7690; thiscall, ret 16; retail-only
void rmgTerrainPainter::paintRectangle(
    unsigned int x, unsigned int y,
    unsigned int rectangleWidth, unsigned int rectangleHeight)
{
    rectangleWidth += x;
    rectangleHeight += y;
    TRmgGridPoint point;
    for (point.m_y = y; point.m_y < rectangleHeight; ++point.m_y) {
        for (point.m_x = x; point.m_x < rectangleWidth; ++point.m_x) {
            if (getPaintTerrain() != getTerrain(point)) {
                paintPoint(point);
            } else {
                int frame = selectBaseFrame(point, m_paintTerrain, -1);
                rmgTerrainTile tile(m_paintTerrain, frame);
                setTile(point, tile);
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
                TRmgGridPoint nearby = point + offset;
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
// Residual (63.6299%): the first three insert-result pair constructors
// expand here but retail retains 0x51b150. The northwest and northeast
// GetPackedCell reads over-inline; the final southeast InitializePackedCell
// remains a call where retail expands the adapter read and cache fill.
VA(0x005B50F0, 0x34E) // anchor-callee 0x5b4c72, 0x5b50dd; thiscall, ret 4
void rmgTerrainPainter::queueOtherTerrainNeighbours(const TRmgGridPoint& point)
{
    if (point.m_y > 0
        && getTerrain(TRmgGridPoint(point.m_x, point.m_y - 1)) != m_paintTerrain) {
        m_secondaryPoints.insert(TRmgGridPoint(point.m_x, point.m_y - 1));
    } else if (point.m_y < m_height - 1
        && getTerrain(TRmgGridPoint(point.m_x, point.m_y + 1)) != m_paintTerrain) {
        m_secondaryPoints.insert(TRmgGridPoint(point.m_x, point.m_y + 1));
    }
    if (point.m_x > 0
        && getTerrain(TRmgGridPoint(point.m_x - 1, point.m_y)) != m_paintTerrain) {
        m_secondaryPoints.insert(TRmgGridPoint(point.m_x - 1, point.m_y));
    } else if (point.m_x < m_width - 1
        && getTerrain(TRmgGridPoint(point.m_x + 1, point.m_y)) != m_paintTerrain) {
        m_secondaryPoints.insert(TRmgGridPoint(point.m_x + 1, point.m_y));
    }
    if (point.m_x > 0 && point.m_y > 0) {
        TRmgGridPoint nearby(point.m_x - 1, point.m_y - 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            m_secondaryPoints.insert(nearby);
    }
    if (point.m_x < m_width - 1 && point.m_y > 0) {
        TRmgGridPoint nearby(point.m_x + 1, point.m_y - 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            m_secondaryPoints.insert(nearby);
    }
    if (point.m_x > 0 && point.m_y < m_height - 1) {
        TRmgGridPoint nearby(point.m_x - 1, point.m_y + 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            m_secondaryPoints.insert(nearby);
    }
    if (point.m_x < m_width - 1 && point.m_y < m_height - 1) {
        TRmgGridPoint nearby(point.m_x + 1, point.m_y + 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !g_rmgTerrainRules[terrain]->m_allowsSeparatedNeighbours)
            m_secondaryPoints.insert(nearby);
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
unsigned char rmgTerrainPainter::needsTerrainRepair(const TRmgGridPoint& point)
{
    if (isHorizontalGap(point) || isVerticalGap(point))
        return 1;
    if (g_rmgTerrainRules[getTerrain(point)]->m_allowsSeparatedNeighbours)
        return 0;
    return hasSeparatedNeighbours(point);
}

// Repair a one-cell terrain gap, then merge all but the largest remaining
// gap in the neighbour ring. Cardinal directions have weight two and
// diagonals weight one. The worklist at 0x5b72f0 passes its first primary
// point to this method; PaintPoint consumes the resulting repairs.
// Remaining: some nested GetPackedCell and point-constructor calls still
// over-inline. Retail also retains the copied x component at +0x5bb while
// our point-addition return folds it away. Keep the canonical arithmetic
// and predicates; no inline-depth pin is used to hide those differences.
VA(0x005B5440, 0x628) // anchor-callee 0x5b7358; thiscall, ret 4; retail-only
void rmgTerrainPainter::repairTerrainPoint(const TRmgGridPoint& point)
{
    if (isVerticalGap(point)) {
        paintPoint(
            !needsTerrainRepair(TRmgGridPoint(point.m_x, point.m_y - 1))
                && (needsTerrainRepair(TRmgGridPoint(point.m_x, point.m_y + 1))
                    || (isHorizontalGap(
                            TRmgGridPoint(point.m_x, point.m_y - 1), m_paintTerrain)
                        && !isHorizontalGap(
                            TRmgGridPoint(point.m_x, point.m_y + 1), m_paintTerrain)))
            ? TRmgGridPoint(point.m_x, point.m_y + 1)
            : TRmgGridPoint(point.m_x, point.m_y - 1));
    }
    if (isHorizontalGap(point)) {
        paintPoint(
            !needsTerrainRepair(TRmgGridPoint(point.m_x - 1, point.m_y))
                && (needsTerrainRepair(TRmgGridPoint(point.m_x + 1, point.m_y))
                    || (isVerticalGap(
                            TRmgGridPoint(point.m_x - 1, point.m_y), m_paintTerrain)
                        && !isVerticalGap(
                            TRmgGridPoint(point.m_x + 1, point.m_y), m_paintTerrain)))
            ? TRmgGridPoint(point.m_x + 1, point.m_y)
            : TRmgGridPoint(point.m_x - 1, point.m_y));
    }

    if (!g_rmgTerrainRules[m_paintTerrain]->m_allowsSeparatedNeighbours
        && hasSeparatedNeighbours(point)) {
        unsigned char matches[TILE_DIR_COUNT];
        buildMatchingNeighbourMask(point, matches);
        TRmgTerrainGap gaps[TILE_DIR_COUNT / 2];
        unsigned int gapCount = 0;
        unsigned int first = 0;
        while (!matches[first])
            ++first;

        unsigned int direction = first;
        while ((direction = (direction + 1) % TILE_DIR_COUNT) != first) {
            if (!matches[direction]) {
                unsigned int currentGap = gapCount++;
                gaps[currentGap].m_weight = 0;
                gaps[currentGap].m_start = direction;
                gaps[currentGap].m_length = 0;
                do {
                    unsigned char diagonal = direction & 1;
                    gaps[currentGap].m_weight += diagonal ? 1 : 2;
                    ++gaps[currentGap].m_length;
                    direction = (direction + 1) % TILE_DIR_COUNT;
                    if (direction == first)
                        // Retail +0x52d leaves both loops directly. A
                        // compound do condition followed by another test
                        // duplicates this comparison and rotates the exit.
                        goto gapsBuilt;
                } while (!matches[direction]);
            }
        }

    gapsBuilt:
        unsigned char neighbourExists[TILE_DIR_COUNT];
        buildTileNeighbourMask(
            m_width, m_height, point.m_x, point.m_y, neighbourExists);
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
VA(0x005B5A70, 0x8A7)  // caller cluster reaches Complete RMG; retail-only
void rmgTerrainPainter::paintTransitions()
{
    std::vector<unsigned char> edgeCounts(m_width * m_height);
    TRmgGridPoint point;

    for (point.m_y = 0; point.m_y < m_height - 1; ++point.m_y) {
        int terrain = getTerrain(TRmgGridPoint(0, point.m_y));

        if (getTerrain(TRmgGridPoint(1, point.m_y)) != terrain) {
            ++edgeCounts[point.m_y * m_width];
            ++edgeCounts[point.m_y * m_width + 1];
        }
        if (getTerrain(TRmgGridPoint(1, point.m_y + 1)) != terrain) {
            ++edgeCounts[point.m_y * m_width];
            ++edgeCounts[(point.m_y + 1) * m_width + 1];
        }
        if (getTerrain(TRmgGridPoint(0, point.m_y + 1)) != terrain) {
            ++edgeCounts[point.m_y * m_width];
            ++edgeCounts[(point.m_y + 1) * m_width];
        }

        for (point.m_x = 1; point.m_x < m_width - 1; ++point.m_x) {
            terrain = getTerrain(point);

            if (getTerrain(TRmgGridPoint(point.m_x + 1, point.m_y)) != terrain) {
                ++edgeCounts[point.m_y * m_width + point.m_x];
                ++edgeCounts[point.m_y * m_width + point.m_x + 1];
            }
            if (getTerrain(TRmgGridPoint(point.m_x + 1, point.m_y + 1)) != terrain) {
                ++edgeCounts[point.m_y * m_width + point.m_x];
                ++edgeCounts[(point.m_y + 1) * m_width + point.m_x + 1];
            }
            if (getTerrain(TRmgGridPoint(point.m_x, point.m_y + 1)) != terrain) {
                ++edgeCounts[point.m_y * m_width + point.m_x];
                ++edgeCounts[(point.m_y + 1) * m_width + point.m_x];
            }
            if (getTerrain(TRmgGridPoint(point.m_x - 1, point.m_y + 1)) != terrain) {
                ++edgeCounts[point.m_y * m_width + point.m_x];
                ++edgeCounts[(point.m_y + 1) * m_width + point.m_x - 1];
            }
        }

        terrain = getTerrain(point);
        if (getTerrain(TRmgGridPoint(point.m_x, point.m_y + 1)) != terrain) {
            ++edgeCounts[point.m_y * m_width + point.m_x];
            ++edgeCounts[(point.m_y + 1) * m_width + point.m_x];
        }
        if (getTerrain(TRmgGridPoint(point.m_x - 1, point.m_y + 1)) != terrain) {
            ++edgeCounts[point.m_y * m_width + point.m_x];
            ++edgeCounts[(point.m_y + 1) * m_width + point.m_x - 1];
        }
    }

    for (point.m_x = 0; point.m_x < m_width - 1; ++point.m_x) {
        int terrain = getTerrain(point);
        if (getTerrain(TRmgGridPoint(point.m_x + 1, point.m_y)) != terrain) {
            ++edgeCounts[point.m_y * m_width + point.m_x];
            ++edgeCounts[point.m_y * m_width + point.m_x + 1];
        }
    }

    for (point.m_y = 0; point.m_y < m_height; ++point.m_y) {
        for (point.m_x = 0; point.m_x < m_width; ++point.m_x) {
            unsigned int index = point.m_y * m_width + point.m_x;

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
VA(0x005B6320, 0x107) // anchor-callee 0x5b569f; retail-only
unsigned char rmgTerrainPainter::isHorizontalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.m_x > 0 && point.m_x < m_width - 1
        && getTerrain(TRmgGridPoint(point.m_x - 1, point.m_y)) != terrain
        && getTerrain(TRmgGridPoint(point.m_x + 1, point.m_y)) != terrain;
}

VA(0x005B6430, 0x106) // anchor-callee 0x5b545f; retail-only
unsigned char rmgTerrainPainter::isVerticalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.m_y > 0 && point.m_y < m_height - 1
        && getTerrain(TRmgGridPoint(point.m_x, point.m_y - 1)) != terrain
        && getTerrain(TRmgGridPoint(point.m_x, point.m_y + 1)) != terrain;
}

// Cardinal neighbours use coordinates clamped to the map edge. A diagonal
// contributes only when at least one adjoining cardinal cell also matches.
// Remaining nested-inline decisions: retail retains GetPackedCell for the
// center and all four cardinals; this spelling already expands it for east.
// At southeast retail also expands InitializePackedCell through adapter
// slot +0x10, whereas this candidate retains InitializePackedCell.
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
// Declare direction before the initial scan so its failure can legally
// enter the shared return without bypassing an initialized declaration.
// VC6 still duplicates a false epilogue at +0x64 for the later loop exit.
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
            goto noSeparation;
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

#if 0  // @carcass - direct retained callees of PaintTransitions
VA(0x005B68A0, 0x2FF)  // thiscall at 0x5b5f45; retail-only
void rmgTerrainPainter::buildNeighbourKinds(
    const TRmgGridPoint& point, int* neighbours)
{
}  // @stub

VA(0x005B6BA0, 0x24C)  // transition 2/8 tests; retail-only
unsigned char rmgTerrainPainter::checkFirstDiagonal(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    return 0;  // @stub
}

VA(0x005B6E00, 0x1B3)  // transition 5/11 tests; retail-only
unsigned char rmgTerrainPainter::checkSecondDiagonal(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    return 0;  // @stub
}

VA(0x005B6FD0, 0x271)  // base-frame selection call; retail-only
int rmgTerrainPainter::getTransitionStrength(
    const TRmgGridPoint& point, int terrain)
{
    return 0;  // @stub
}
#endif

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

void rmgTerrainPainter::changeTerrain(int terrain, int strength)
{
    finish();
    m_paintTerrain = terrain;
    m_transitionStrength = strength;
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

// Exact: all 362 raw bytes after 14 relocations; all 13 blocks agree. The
// grid copy constructor restores erase(key)'s retained distance helper at
// 0x5b8cd0 and its count local. Implicit grid copies instead expand distance
// into iterator increments (81.33%). The final GetPackedCell expansion keeps
// its nested InitializePackedCell call, as retail does.
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

// PaintPoint and TRmgTerrainBrush::changeTerrain retain this one-dword
// iterator wrapper around the tree's raw-node lower bound.
VA_COMPGEN(0x005B85A0, 0x17, TREE_LOWER_BOUND, TRmgGridPoint)
VA_COMPGEN(0x005B8A20, 0x17, TREE_UPPER_BOUND, TRmgGridPoint)

// The set lookup at 0x5b4e96 retains this free comparison. Its unsigned
// y-then-x ordering also appears in the tree's expanded comparisons.
VA(0x005B8CA0, 0x20) // anchor-callee 0x5b4e96; fastcall, two point references
bool operator<(const TRmgGridPoint& left, const TRmgGridPoint& right)
{
    return left.m_y < right.m_y || (left.m_y == right.m_y && left.m_x < right.m_x);
}
