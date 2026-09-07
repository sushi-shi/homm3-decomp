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
void TRmgTerrainPainter::initializePackedCell(
    const TRmgGridPoint& point, unsigned int index)
{
    TRmgTerrainTile tile = m_adapter->getTile(point);
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

// Residual (23.3305%): retail calls both set _Init helpers, whereas the
// second expands here; the packed-vector insert has the opposite decision.
// Explicit versus default resize fill values are byte-neutral. The retained
// brush constructor calls this ordinary body at 0x5b7297.
VA(0x005B45F0, 0x26D) // anchor-callee 0x5b7297; retail-only
TRmgTerrainPainter::TRmgTerrainPainter(
    TRmgMapAdapterInterface* newAdapter, int terrain, int strength)
    : m_adapter(newAdapter), m_paintTerrain(terrain), m_transitionStrength(strength)
{
    TRmgGridPoint size = m_adapter->getSize();
    m_width = size.m_x;
    m_height = size.m_y;
    m_packedCells.resize(m_width * m_height, TRmgPackedTerrainCell());
}

VA(0x005B48D0, 0x8D)  // repeated caller identity in 0x5b3dd0..0x5b76f0
TRmgPackedTerrainCell* TRmgTerrainPainter::getPackedCell(
    const TRmgGridPoint& point)
{
    unsigned int index = point.m_y * m_width + point.m_x;
    if (!m_packedCells[index].m_initialized)
        initializePackedCell(point, index);
    return &m_packedCells[index];
}

// Own-terrain checks share the predicates used with the selected paint
// terrain. Retail retains the nested predicate in the former and expands
// the latter at the four adjacent-row/column probes.
unsigned char TRmgTerrainPainter::isHorizontalGap(const TRmgGridPoint& point)
{
    return isHorizontalGap(point, getTerrain(point));
}

unsigned char TRmgTerrainPainter::isVerticalGap(const TRmgGridPoint& point)
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
unsigned char TRmgTerrainPainter::needsTerrainRepair(const TRmgGridPoint& point)
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
void TRmgTerrainPainter::repairTerrainPoint(const TRmgGridPoint& point)
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
            for (unsigned int gap = 1; gap < gapCount; ++gap) {
                if (gaps[gap].m_weight < smallestWeight) {
                    smallest = gap;
                    smallestWeight = gaps[gap].m_weight;
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
            for (gap = smallest; gap < gapCount; ++gap)
                gaps[gap] = gaps[gap + 1];
        } while (gapCount > 1);
    }
}

// The unsigned grid/ref-argument correction is independently proved by the
// set comparator and the retained grid constructor. It changes the nested
// accessor expansion here (74.76 -> 42.02%); retain the proven type and its
// historical peak while recovering this caller's original helper surface.
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
void TRmgTerrainPainter::paintTransitions()
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

                TRmgTerrainTile tile = getPackedCell(point)->getTile();

                int newFrame;
                if (transition) {
                    newFrame = g_rmgTerrainRules[tile.m_terrain]
                        ->selectTransitionFrame(
                            transition, flip, flip, tile.m_frame);
                } else {
                    newFrame = g_rmgTerrainRules[tile.m_terrain]
                        ->selectBaseFrame(
                            getTransitionStrength(point, tile.m_terrain),
                            tile.m_frame);
                }

                if (tile.m_frame != newFrame || tile.m_flipX != flip.m_flipX
                    || tile.m_flipY != flip.m_flipY) {
                    tile.m_flipX = flip.m_flipX;
                    tile.m_flipY = flip.m_flipY;
                    tile.m_frame = newFrame;
                    m_adapter->setTile(point, tile);

                    TRmgPackedTerrainCell& updatedPacked =
                        m_packedCells[point.m_y * m_width + point.m_x];
                    updatedPacked.setInitialized();
                    updatedPacked.setTerrain(tile.m_terrain);
                    updatedPacked.setFrame(tile.m_frame);
                    updatedPacked.setFlipX(tile.m_flipX);
                    updatedPacked.setFlipY(tile.m_flipY);
                }
            } else {
                TRmgTerrainTile tile = getPackedCell(point)->getTile();

                int newFrame = g_rmgTerrainRules[tile.m_terrain]
                    ->selectBaseFrame(
                        getTransitionStrength(point, tile.m_terrain),
                        tile.m_frame);
                if (tile.m_frame != newFrame || tile.m_flipX || tile.m_flipY) {
                    tile.m_frame = newFrame;
                    tile.m_flipX = 0;
                    tile.m_flipY = 0;
                    m_adapter->setTile(point, tile);

                    TRmgPackedTerrainCell& updatedPacked =
                        m_packedCells[point.m_y * m_width + point.m_x];
                    updatedPacked.setInitialized();
                    updatedPacked.setTerrain(tile.m_terrain);
                    updatedPacked.setFrame(tile.m_frame);
                    updatedPacked.setFlipX(tile.m_flipX);
                    updatedPacked.setFlipY(tile.m_flipY);
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
unsigned char TRmgTerrainPainter::isHorizontalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.m_x > 0 && point.m_x < m_width - 1
        && getTerrain(TRmgGridPoint(point.m_x - 1, point.m_y)) != terrain
        && getTerrain(TRmgGridPoint(point.m_x + 1, point.m_y)) != terrain;
}

VA(0x005B6430, 0x106) // anchor-callee 0x5b545f; retail-only
unsigned char TRmgTerrainPainter::isVerticalGap(
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
void TRmgTerrainPainter::buildMatchingNeighbourMask(
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
// (91.27%). The current body has the correct 132 bytes/instructions except
// three short-branch displacements: +0x20/+0x52/+0x72 choose the false
// epilogue at +0x64 instead of retail's +0x44 (99.7458%). Native bool with
// true/false literals produces the same three differences. Address-masked
// asm alone hides this; the resolved raw-byte comparison does not.
VA(0x005B6810, 0x84) // anchor-callee 0x5b58e4; retail-only
unsigned char TRmgTerrainPainter::hasSeparatedNeighbours(const TRmgGridPoint& point)
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
void TRmgTerrainPainter::buildNeighbourKinds(
    const TRmgGridPoint& point, int* neighbours)
{
}  // @stub

VA(0x005B6BA0, 0x24C)  // transition 2/8 tests; retail-only
unsigned char TRmgTerrainPainter::checkFirstDiagonal(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    return 0;  // @stub
}

VA(0x005B6E00, 0x1B3)  // transition 5/11 tests; retail-only
unsigned char TRmgTerrainPainter::checkSecondDiagonal(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    return 0;  // @stub
}

VA(0x005B6FD0, 0x271)  // base-frame selection call; retail-only
int TRmgTerrainPainter::getTransitionStrength(
    const TRmgGridPoint& point, int terrain)
{
    return 0;  // @stub
}
#endif

// The same primary/secondary worklist appears in the brush's terrain change
// and destructor. Preserve one ordinary completion helper and the canonical
// set erase(key); its distance walk expands only at the change site.
void TRmgTerrainPainter::finish()
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

void TRmgTerrainPainter::changeTerrain(int terrain, int strength)
{
    finish();
    m_paintTerrain = terrain;
    m_transitionStrength = strength;
}

VA(0x005B7250, 0x9A) // anchor-callee 0x54017e; allocation and throw RTTI
TRmgTerrainBrush::TRmgTerrainBrush(
    TRmgMapAdapterInterface* map, int terrain, int strength)
    : m_painter(new TRmgTerrainPainter(map, terrain, strength))
{
    if (!m_painter.get())
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

VA(0x005B76F0, 0x209) // anchor-callee 0x5b76d9; retained painter destructor
TRmgTerrainPainter::~TRmgTerrainPainter()
{
    finish();
}

// The four late point constructions in RepairTerrainPoint pass x and y by
// reference. The retained two-store body is 24 bytes including ret 8.
VA_COMPGEN(0x005B76B0, 0x18, CLASS_CTOR, TRmgGridPoint)
