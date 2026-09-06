// rmg_terrain.cpp - Complete-only random-map terrain transition support.
//
// The Dreamcast build contains no random-map generator compiland. Function
// ownership, field layout, helper boundaries, and call/expansion decisions in
// this unit therefore come directly from the retail x86 cluster.
#include <va.h>
#include "rmg_terrain.h"
#include "exceptions.h"
#include "tiles.h"

DATA(0x00642BD8) extern TRmgTerrainRule* const gRmgTerrainRules[];

// Provisional role spelling. The fastcall ABI and two-byte output are fixed
// by the call at 0x5b5f4e. All selector names are provisional retail roles.
int __fastcall SelectTerrainTransition(
    const int* neighbours, TRmgTerrainFlip* flip);

// Four reflections of the eight neighbour slots. Read from the pinned
// retail image; both flip bytes index this table independently.
DATA(0x00642C00)
const int gRmgReflectedNeighbours[2][2][8] = {
    {{0, 1, 2, 3, 4, 5, 6, 7}, {4, 3, 2, 1, 0, 7, 6, 5}},
    {{0, 7, 6, 5, 4, 3, 2, 1}, {4, 5, 6, 7, 0, 1, 2, 3}}
};

// Provisional value-return helper, auto-inlined at every selector site.
// Returning the constructed value reproduces retail's temporary at ebp-2
// and the saved output pointer at ebp-8. A named local return instead puts
// them at ebp-8 and ebp-4 (99.93%); replacing the helper calls with direct
// construction changes the fourth reflection loop's registers (99.7991%).
// An explicit empty flip destructor prevents the helper from auto-inlining.
static TRmgTerrainFlip MakeTerrainFlip(unsigned char x, unsigned char y)
{
    return TRmgTerrainFlip(x, y);
}

VA(0x005B3DD0, 0x6F)  // called and expanded in the retail terrain cluster
void rmgTerrainPainter::initializePackedCell(
    const TRmgGridPoint& point, unsigned int index)
{
    TRmgTerrainTile tile = m_adapter->GetTile(point);
    TRmgPackedTerrainCell& packed = m_packedCells[index];
    packed.terrain = tile.terrain;
    packed.frame = tile.frame;
    packed.flipX = tile.flipX;
    packed.flipY = tile.flipY;
    packed.initialized = 1;
}

VA(0x005B3E80, 0x75F)  // fastcall call at 0x5b5f4e; retail-only
int __fastcall SelectTerrainTransition(
    const int* neighbours, TRmgTerrainFlip* flip)
{
    // Retail +0x09..+0x7b: guarded local-static construction in this order,
    // with a registered empty cleanup. No Dreamcast RMG counterpart.
    DATA(0x006A52B8)
    static TRmgTerrainFlip flips[4] = {
        MakeTerrainFlip(0, 0), MakeTerrainFlip(0, 1),
        MakeTerrainFlip(1, 0), MakeTerrainFlip(1, 1)
    };
    unsigned int i;
    for (i = 0; i < 4; ++i) {
        const int* order = gRmgReflectedNeighbours[flips[i].flipX][flips[i].flipY];
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
        const int* order = gRmgReflectedNeighbours[flips[i].flipX][flips[i].flipY];
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
        const int* order = gRmgReflectedNeighbours[flips[i].flipX][flips[i].flipY];
        if (neighbours[order[2]] == RMG_NEIGHBOUR_HARD_EDGE &&
            neighbours[order[4]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            if (neighbours[order[5]] != RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[i];
                return 21;
            } else {
                *flip = MakeTerrainFlip(!flips[i].flipX, !flips[i].flipY);
                return 8;
            }
        }
        if (neighbours[order[2]] == RMG_NEIGHBOUR_BLEND_EDGE &&
            neighbours[order[4]] == RMG_NEIGHBOUR_HARD_EDGE) {
            if (neighbours[order[1]] != RMG_NEIGHBOUR_HARD_EDGE) {
                *flip = flips[i];
                return 22;
            } else {
                *flip = MakeTerrainFlip(!flips[i].flipX, !flips[i].flipY);
                return 8;
            }
        }
    }
    for (i = 0; i < 4; ++i) {
        const int* order = gRmgReflectedNeighbours[flips[i].flipX][flips[i].flipY];
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
        const int* order = gRmgReflectedNeighbours[flips[i].flipX][flips[i].flipY];
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
        const int* order = gRmgReflectedNeighbours[flips[i].flipX][flips[i].flipY];
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
        const int* order = gRmgReflectedNeighbours[flips[i].flipX][flips[i].flipY];
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
        const int* order = gRmgReflectedNeighbours[flips[i].flipX][flips[i].flipY];
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
        const int* order = gRmgReflectedNeighbours[flips[i].flipX][flips[i].flipY];
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
        const int* order = gRmgReflectedNeighbours[flips[i].flipX][flips[i].flipY];
        if (neighbours[order[3]] == RMG_NEIGHBOUR_BLEND_EDGE) {
            *flip = flips[i];
            return 5;
        }
        if (neighbours[order[3]] == RMG_NEIGHBOUR_HARD_EDGE) {
            *flip = flips[i];
            return 11;
        }
    }
    *flip = MakeTerrainFlip(0, 0);
    return 0;
}

// Residual (23.3305%): retail calls both set _Init helpers, whereas the
// second expands here; the packed-vector insert has the opposite decision.
// Explicit versus default resize fill values are byte-neutral. The retained
// brush constructor calls this ordinary body at 0x5b7297.
VA(0x005B45F0, 0x26D) // anchor-callee 0x5b7297; retail-only
rmgTerrainPainter::rmgTerrainPainter(
    TRmgMapAdapterInterface* newAdapter, int terrain, int strength)
    : m_adapter(newAdapter), m_paintTerrain(terrain), m_transitionStrength(strength)
{
    TRmgGridPoint size = m_adapter->GetSize();
    m_width = size.x;
    m_height = size.y;
    m_packedCells.resize(m_width * m_height, TRmgPackedTerrainCell());
}

VA(0x005B48D0, 0x8D)  // repeated caller identity in 0x5b3dd0..0x5b76f0
TRmgPackedTerrainCell* rmgTerrainPainter::getPackedCell(
    const TRmgGridPoint& point)
{
    unsigned int index = point.y * m_width + point.x;
    if (!m_packedCells[index].initialized)
        initializePackedCell(point, index);
    return &m_packedCells[index];
}

// The base-frame paths in PaintPoint and PaintTransitions first compute
// strength, then load the selected rule's virtual receiver. Keep that shared
// evaluation boundary and the captured terrain index across the first call.
// The helper's role and signature are inferred from retail expansions.
int rmgTerrainPainter::selectBaseFrame(
    const TRmgGridPoint& point, int terrain, int oldFrame)
{
    int strength = getTransitionStrength(point, terrain);
    return gRmgTerrainRules[terrain]->SelectBaseFrame(strength, oldFrame);
}

// PaintPoint and both transition updates repeat this adapter/cache write.
// Preserve the shared operation, including validity before the four values;
// cache initialization from an adapter read has a different store order.
// This ordinary helper is inferred from retail expansions, with no DC name.
void rmgTerrainPainter::setTile(
    const TRmgGridPoint& point, const TRmgTerrainTile& tile)
{
    m_adapter->SetTile(point, tile);
    TRmgPackedTerrainCell& packed = m_packedCells[point.y * m_width + point.x];
    packed.SetInitialized();
    packed.SetTerrain(tile.terrain);
    packed.SetFrame(tile.frame);
    packed.SetFlipX(tile.flipX);
    packed.SetFlipY(tile.flipY);
}

// Paint a base tile, refresh its cache, then reconcile the two repair sets.
// Terrain rules that permit separated neighbours only release cardinal gaps;
// the other rules recheck every matching neighbour. The rectangle painter
// calls this body at 0x5b4a2d; RepairTerrainPoint and Finish share it.
// Names are provisional: this Complete-only code has no Dreamcast body.
// Residual (97.0506%): GetPackedCell expands at the first eight-neighbour
// terrain read (retail 0x5b4e55). The inner primary-set erase also expands
// the three-argument distance wrapper to its four-argument overload
// (0x5b4f7f). The frame is 0x54 versus 0x50; the retained pre-translation
// x copy at 0x5b4e3f is still missing. Constructing the translation's initial
// value from its coordinates restores the final GetPackedCell call at
// 0x5b509e; implicit whole-point copying leaves 95.3146% and expands it.
// Flattening SetTile leaves 78.4213%; flattening frame selection leaves
// 90.9367% with the named frame, or 92.2405% with direct tile.frame assignment
// but the wrong entry load schedule. Reusing one nearby point across all
// branches gives 91.1519% after both shared helpers are present.
// An explicit grid copy constructor retains a copy call absent from retail
// (91.8861%). Value offset arguments and direct sum construction change the
// direction-loop loads; copy assignment of the origin leaves 93.5063%.
// Tile-only scopes, separate nearby assignment, early loop guards, named
// terrain/index/coordinate values, and a free translation with reference
// operands do not improve the constructor-based result. A tile constructor
// changes later call boundaries (85.7667%); a by-value SetTile argument adds
// an extra entry copy (95.7884%). A nested packed-cell writer is retained
// where retail expands the field stores (81.9566%). The recovered neighbour
// queue body below leaves this caller unchanged. No inline controls are used.
// Budget tracing (object-identity gated): caller cb=933, initial budget
// 1866. The first extra getPackedCell expansion has cb=90, budget=106;
// the inner three-argument _Distance has cb=41, budget=45. The final rule
// read has budget=73 and correctly keeps the cb=90 cache helper out of line.
// A named lookup iterator changes find/end evaluation (89.1573%); a cache
// hit early return changes getPackedCell's retained bytes (93.4211% there).
// Coordinate accessors alter x/y allocation without restoring the original-x
// home (97.0036%); a by-value terrain-accessor point adds copies (72.9584%).
// Shared terrain comparison, const translation result, direct temporary
// return, and a validity accessor leave the two call-boundary deltas intact.
// A scratch C2 counterfactual rejects just the first cache expansion and
// the inner distance wrapper, preserving their original budget charges.
// Its full named call sequence matches retail, but the 0x54 frame and missing
// original-x store remain: those storage deltas need independent recovery.
// No counterfactual object enters the matching build. A call-site depth-one
// pragma is byte-neutral; pinning a direct cache call changes later decisions
// (91.4213%) and is not an isolated control. A signed-direction conversion
// gives 95.7902%; free value-origin addition with a named return gives 92.3924%.
// An assignment-based grid copy constructor, explicit grid assignment, and
// declaring the loop index before its mask leave 97.0506% unchanged.
VA(0x005B4B20, 0x5CB) // anchor-callee 0x5b4960, 0x5b5440; thiscall, ret 4
void rmgTerrainPainter::paintPoint(const TRmgGridPoint& point)
{
    int frame = selectBaseFrame(point, m_paintTerrain, -1);
    TRmgTerrainTile tile;
    tile.terrain = m_paintTerrain;
    tile.frame = frame;
    tile.flipX = 0;
    tile.flipY = 0;
    setTile(point, tile);

    if (m_secondaryPoints.find(point) != m_secondaryPoints.end())
        m_secondaryPoints.erase(point);

    if (gRmgTerrainRules[m_paintTerrain]->allowsSeparatedNeighbours) {
        if (point.y > 0) {
            TRmgGridPoint nearby(point.x, point.y - 1);
            if (m_primaryPoints.find(nearby) != m_primaryPoints.end()
                && !isHorizontalGap(nearby)) {
                m_primaryPoints.erase(nearby);
                queueOtherTerrainNeighbours(nearby);
            }
        }
        if (point.y < m_height - 1) {
            TRmgGridPoint nearby(point.x, point.y + 1);
            if (m_primaryPoints.find(nearby) != m_primaryPoints.end()
                && !isHorizontalGap(nearby)) {
                m_primaryPoints.erase(nearby);
                queueOtherTerrainNeighbours(nearby);
            }
        }
        if (point.x > 0) {
            TRmgGridPoint nearby(point.x - 1, point.y);
            if (m_primaryPoints.find(nearby) != m_primaryPoints.end()
                && !isVerticalGap(nearby)) {
                m_primaryPoints.erase(nearby);
                queueOtherTerrainNeighbours(nearby);
            }
        }
        if (point.x < m_width - 1) {
            TRmgGridPoint nearby(point.x + 1, point.y);
            if (m_primaryPoints.find(nearby) != m_primaryPoints.end()
                && !isVerticalGap(nearby)) {
                m_primaryPoints.erase(nearby);
                queueOtherTerrainNeighbours(nearby);
            }
        }
    } else {
        unsigned char neighbourExists[TILE_DIR_COUNT];
        BuildTileNeighbourMask(
            m_width, m_height, point.x, point.y, neighbourExists);
        for (unsigned int direction = 0; direction < TILE_DIR_COUNT; ++direction) {
            if (neighbourExists[direction]) {
                TRmgGridPoint nearby = point + gTileDirections[direction];
                if (getTerrain(nearby) == m_paintTerrain) {
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
    if (point.y > 0
        && getTerrain(TRmgGridPoint(point.x, point.y - 1)) != m_paintTerrain) {
        m_secondaryPoints.insert(TRmgGridPoint(point.x, point.y - 1));
    } else if (point.y < m_height - 1
        && getTerrain(TRmgGridPoint(point.x, point.y + 1)) != m_paintTerrain) {
        m_secondaryPoints.insert(TRmgGridPoint(point.x, point.y + 1));
    }
    if (point.x > 0
        && getTerrain(TRmgGridPoint(point.x - 1, point.y)) != m_paintTerrain) {
        m_secondaryPoints.insert(TRmgGridPoint(point.x - 1, point.y));
    } else if (point.x < m_width - 1
        && getTerrain(TRmgGridPoint(point.x + 1, point.y)) != m_paintTerrain) {
        m_secondaryPoints.insert(TRmgGridPoint(point.x + 1, point.y));
    }
    if (point.x > 0 && point.y > 0) {
        TRmgGridPoint nearby(point.x - 1, point.y - 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !gRmgTerrainRules[terrain]->allowsSeparatedNeighbours)
            m_secondaryPoints.insert(nearby);
    }
    if (point.x < m_width - 1 && point.y > 0) {
        TRmgGridPoint nearby(point.x + 1, point.y - 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !gRmgTerrainRules[terrain]->allowsSeparatedNeighbours)
            m_secondaryPoints.insert(nearby);
    }
    if (point.x > 0 && point.y < m_height - 1) {
        TRmgGridPoint nearby(point.x - 1, point.y + 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !gRmgTerrainRules[terrain]->allowsSeparatedNeighbours)
            m_secondaryPoints.insert(nearby);
    }
    if (point.x < m_width - 1 && point.y < m_height - 1) {
        TRmgGridPoint nearby(point.x + 1, point.y + 1);
        int terrain = getTerrain(nearby);
        if (terrain != m_paintTerrain
            && !gRmgTerrainRules[terrain]->allowsSeparatedNeighbours)
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
    if (gRmgTerrainRules[getTerrain(point)]->allowsSeparatedNeighbours)
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
            !needsTerrainRepair(TRmgGridPoint(point.x, point.y - 1))
                && (needsTerrainRepair(TRmgGridPoint(point.x, point.y + 1))
                    || (isHorizontalGap(
                            TRmgGridPoint(point.x, point.y - 1), m_paintTerrain)
                        && !isHorizontalGap(
                            TRmgGridPoint(point.x, point.y + 1), m_paintTerrain)))
            ? TRmgGridPoint(point.x, point.y + 1)
            : TRmgGridPoint(point.x, point.y - 1));
    }
    if (isHorizontalGap(point)) {
        paintPoint(
            !needsTerrainRepair(TRmgGridPoint(point.x - 1, point.y))
                && (needsTerrainRepair(TRmgGridPoint(point.x + 1, point.y))
                    || (isVerticalGap(
                            TRmgGridPoint(point.x - 1, point.y), m_paintTerrain)
                        && !isVerticalGap(
                            TRmgGridPoint(point.x + 1, point.y), m_paintTerrain)))
            ? TRmgGridPoint(point.x + 1, point.y)
            : TRmgGridPoint(point.x - 1, point.y));
    }

    if (!gRmgTerrainRules[m_paintTerrain]->allowsSeparatedNeighbours
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
                gaps[currentGap].weight = 0;
                gaps[currentGap].start = direction;
                gaps[currentGap].length = 0;
                do {
                    unsigned char diagonal = direction & 1;
                    gaps[currentGap].weight += diagonal ? 1 : 2;
                    ++gaps[currentGap].length;
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
        BuildTileNeighbourMask(
            m_width, m_height, point.x, point.y, neighbourExists);
        do {
            unsigned int smallest = 0;
            unsigned int smallestWeight = gaps[0].weight;
            for (unsigned int gap = 1; gap < gapCount; ++gap) {
                if (gaps[gap].weight < smallestWeight) {
                    smallest = gap;
                    smallestWeight = gaps[gap].weight;
                }
            }
            unsigned int end =
                (gaps[smallest].start + gaps[smallest].length) % TILE_DIR_COUNT;
            for (direction = gaps[smallest].start; direction != end;
                 direction = (direction + 1) % TILE_DIR_COUNT) {
                if (neighbourExists[direction])
                    paintPoint(point + gTileDirections[direction]);
            }
            --gapCount;
            for (gap = smallest; gap < gapCount; ++gap)
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

    for (point.y = 0; point.y < m_height - 1; ++point.y) {
        int terrain = getTerrain(TRmgGridPoint(0, point.y));

        if (getTerrain(TRmgGridPoint(1, point.y)) != terrain) {
            ++edgeCounts[point.y * m_width];
            ++edgeCounts[point.y * m_width + 1];
        }
        if (getTerrain(TRmgGridPoint(1, point.y + 1)) != terrain) {
            ++edgeCounts[point.y * m_width];
            ++edgeCounts[(point.y + 1) * m_width + 1];
        }
        if (getTerrain(TRmgGridPoint(0, point.y + 1)) != terrain) {
            ++edgeCounts[point.y * m_width];
            ++edgeCounts[(point.y + 1) * m_width];
        }

        for (point.x = 1; point.x < m_width - 1; ++point.x) {
            terrain = getTerrain(point);

            if (getTerrain(TRmgGridPoint(point.x + 1, point.y)) != terrain) {
                ++edgeCounts[point.y * m_width + point.x];
                ++edgeCounts[point.y * m_width + point.x + 1];
            }
            if (getTerrain(TRmgGridPoint(point.x + 1, point.y + 1)) != terrain) {
                ++edgeCounts[point.y * m_width + point.x];
                ++edgeCounts[(point.y + 1) * m_width + point.x + 1];
            }
            if (getTerrain(TRmgGridPoint(point.x, point.y + 1)) != terrain) {
                ++edgeCounts[point.y * m_width + point.x];
                ++edgeCounts[(point.y + 1) * m_width + point.x];
            }
            if (getTerrain(TRmgGridPoint(point.x - 1, point.y + 1)) != terrain) {
                ++edgeCounts[point.y * m_width + point.x];
                ++edgeCounts[(point.y + 1) * m_width + point.x - 1];
            }
        }

        terrain = getTerrain(point);
        if (getTerrain(TRmgGridPoint(point.x, point.y + 1)) != terrain) {
            ++edgeCounts[point.y * m_width + point.x];
            ++edgeCounts[(point.y + 1) * m_width + point.x];
        }
        if (getTerrain(TRmgGridPoint(point.x - 1, point.y + 1)) != terrain) {
            ++edgeCounts[point.y * m_width + point.x];
            ++edgeCounts[(point.y + 1) * m_width + point.x - 1];
        }
    }

    for (point.x = 0; point.x < m_width - 1; ++point.x) {
        int terrain = getTerrain(point);
        if (getTerrain(TRmgGridPoint(point.x + 1, point.y)) != terrain) {
            ++edgeCounts[point.y * m_width + point.x];
            ++edgeCounts[point.y * m_width + point.x + 1];
        }
    }

    for (point.y = 0; point.y < m_height; ++point.y) {
        for (point.x = 0; point.x < m_width; ++point.x) {
            unsigned int index = point.y * m_width + point.x;

            if (edgeCounts[index] > 0) {
                int neighbours[8];
                buildNeighbourKinds(point, neighbours);

                int transition;
                TRmgTerrainFlip flip;
                transition = SelectTerrainTransition(neighbours, &flip);
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

                TRmgTerrainTile tile = getPackedCell(point)->GetTile();

                int newFrame;
                if (transition) {
                    newFrame = gRmgTerrainRules[tile.terrain]
                        ->SelectTransitionFrame(
                            transition, flip, flip, tile.frame);
                } else {
                    newFrame = selectBaseFrame(point, tile.terrain, tile.frame);
                }

                if (tile.frame != newFrame || tile.flipX != flip.flipX
                    || tile.flipY != flip.flipY) {
                    tile.flipX = flip.flipX;
                    tile.flipY = flip.flipY;
                    tile.frame = newFrame;
                    setTile(point, tile);
                }
            } else {
                TRmgTerrainTile tile = getPackedCell(point)->GetTile();

                int newFrame = selectBaseFrame(point, tile.terrain, tile.frame);
                if (tile.frame != newFrame || tile.flipX || tile.flipY) {
                    tile.frame = newFrame;
                    tile.flipX = 0;
                    tile.flipY = 0;
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
    return point.x > 0 && point.x < m_width - 1
        && getTerrain(TRmgGridPoint(point.x - 1, point.y)) != terrain
        && getTerrain(TRmgGridPoint(point.x + 1, point.y)) != terrain;
}

VA(0x005B6430, 0x106) // anchor-callee 0x5b545f; retail-only
unsigned char rmgTerrainPainter::isVerticalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.y > 0 && point.y < m_height - 1
        && getTerrain(TRmgGridPoint(point.x, point.y - 1)) != terrain
        && getTerrain(TRmgGridPoint(point.x, point.y + 1)) != terrain;
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
    unsigned int north = point.y > 0 ? point.y - 1 : point.y;
    unsigned int south = point.y < m_height - 1 ? point.y + 1 : point.y;
    unsigned int west = point.x > 0 ? point.x - 1 : point.x;
    unsigned int east = point.x < m_width - 1 ? point.x + 1 : point.x;

    matches[TILE_DIR_NORTH] = getTerrain(TRmgGridPoint(point.x, north)) == terrain;
    matches[TILE_DIR_SOUTH] = getTerrain(TRmgGridPoint(point.x, south)) == terrain;
    matches[TILE_DIR_WEST] = getTerrain(TRmgGridPoint(west, point.y)) == terrain;
    matches[TILE_DIR_EAST] = getTerrain(TRmgGridPoint(east, point.y)) == terrain;
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
// (91.27%). The current body has the correct 132 bytes/instructions except
// three short-branch displacements: +0x20/+0x52/+0x72 choose the false
// epilogue at +0x64 instead of retail's +0x44 (99.7458%). Native bool with
// true/false literals produces the same three differences. Address-masked
// asm alone hides this; the resolved raw-byte comparison does not.
VA(0x005B6810, 0x84) // anchor-callee 0x5b58e4; retail-only
unsigned char rmgTerrainPainter::hasSeparatedNeighbours(const TRmgGridPoint& point)
{
    unsigned char matches[TILE_DIR_COUNT];
    buildMatchingNeighbourMask(point, matches);
    unsigned int first = 0;
    while (matches[first]) {
        first = (first + 1) % TILE_DIR_COUNT;
        if (first == 0)
            return 0;
    }
    unsigned int direction = first;
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
    TRmgMapAdapterInterface* map, int terrain, int strength)
    : painter(new rmgTerrainPainter(map, terrain, strength))
{
    if (!painter.get())
        throw TAllocationFailure();
}

VA(0x005B72F0, 0x225) // anchor-callee 0x540207; auto_ptr ownership cleanup
TRmgTerrainBrush::~TRmgTerrainBrush()
{
}

// Residual (81.33093%): erase(key)'s distance helper expands to the iterator
// increment call; retail instead retains 0x5b8cd0 and its count local.
// The remaining named repair/painter call sequence agrees, including the
// final GetPackedCell expansion with a retained InitializePackedCell call.
VA(0x005B7520, 0x16A) // anchor-callee 0x5401c3; retail-only
void TRmgTerrainBrush::ChangeTerrain(int terrain, int strength)
{
    painter->changeTerrain(terrain, strength);
}

VA(0x005B7690, 0x1F) // anchor-callee 0x5401e9; four unsigned rectangle args
void TRmgTerrainBrush::PaintRectangle(
    unsigned int x, unsigned int y,
    unsigned int rectangleWidth, unsigned int rectangleHeight)
{
    painter->paintRectangle(x, y, rectangleWidth, rectangleHeight);
}

VA(0x005B76F0, 0x209) // anchor-callee 0x5b76d9; retained painter destructor
rmgTerrainPainter::~rmgTerrainPainter()
{
    finish();
}

// The four late point constructions in RepairTerrainPoint pass x and y by
// reference. The retained two-store body is 24 bytes including ret 8.
VA_COMPGEN(0x005B76B0, 0x18, CLASS_CTOR, TRmgGridPoint)

// The set lookup at 0x5b4e96 retains this free comparison. Its unsigned
// y-then-x ordering also appears in the tree's expanded comparisons.
VA(0x005B8CA0, 0x20) // anchor-callee 0x5b4e96; fastcall, two point references
bool operator<(const TRmgGridPoint& left, const TRmgGridPoint& right)
{
    return left.y < right.y || (left.y == right.y && left.x < right.x);
}
