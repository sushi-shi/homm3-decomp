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
void TRmgTerrainPainter::InitializePackedCell(
    const TRmgGridPoint& point, unsigned int index)
{
    TRmgTerrainTile tile = adapter->GetTile(point);
    TRmgPackedTerrainCell& packed = packedCells[index];
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
TRmgTerrainPainter::TRmgTerrainPainter(
    TRmgMapAdapterInterface* newAdapter, int terrain, int strength)
    : adapter(newAdapter), paintTerrain(terrain), transitionStrength(strength)
{
    TRmgGridPoint size = adapter->GetSize();
    width = size.x;
    height = size.y;
    packedCells.resize(width * height, TRmgPackedTerrainCell());
}

VA(0x005B48D0, 0x8D)  // repeated caller identity in 0x5b3dd0..0x5b76f0
TRmgPackedTerrainCell* TRmgTerrainPainter::GetPackedCell(
    const TRmgGridPoint& point)
{
    unsigned int index = point.y * width + point.x;
    if (!packedCells[index].initialized)
        InitializePackedCell(point, index);
    return &packedCells[index];
}

// The base-frame paths in PaintPoint and PaintTransitions first compute
// strength, then load the selected rule's virtual receiver. Keep that shared
// evaluation boundary and the captured terrain index across the first call.
// The helper's role and signature are inferred from retail expansions.
int TRmgTerrainPainter::SelectBaseFrame(
    const TRmgGridPoint& point, int terrain, int oldFrame)
{
    int strength = GetTransitionStrength(point, terrain);
    return gRmgTerrainRules[terrain]->SelectBaseFrame(strength, oldFrame);
}

// PaintPoint and both transition updates repeat this adapter/cache write.
// Preserve the shared operation, including validity before the four values;
// cache initialization from an adapter read has a different store order.
// This ordinary helper is inferred from retail expansions, with no DC name.
void TRmgTerrainPainter::SetTile(
    const TRmgGridPoint& point, const TRmgTerrainTile& tile)
{
    adapter->SetTile(point, tile);
    TRmgPackedTerrainCell& packed = packedCells[point.y * width + point.x];
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
VA(0x005B4B20, 0x5CB) // anchor-callee 0x5b4960, 0x5b5440; thiscall, ret 4
void TRmgTerrainPainter::PaintPoint(const TRmgGridPoint& point)
{
    int frame = SelectBaseFrame(point, paintTerrain, -1);
    TRmgTerrainTile tile;
    tile.terrain = paintTerrain;
    tile.frame = frame;
    tile.flipX = 0;
    tile.flipY = 0;
    SetTile(point, tile);

    if (secondaryPoints.find(point) != secondaryPoints.end())
        secondaryPoints.erase(point);

    if (gRmgTerrainRules[paintTerrain]->allowsSeparatedNeighbours) {
        if (point.y > 0) {
            TRmgGridPoint nearby(point.x, point.y - 1);
            if (primaryPoints.find(nearby) != primaryPoints.end()
                && !IsHorizontalGap(nearby)) {
                primaryPoints.erase(nearby);
                QueueOtherTerrainNeighbours(nearby);
            }
        }
        if (point.y < height - 1) {
            TRmgGridPoint nearby(point.x, point.y + 1);
            if (primaryPoints.find(nearby) != primaryPoints.end()
                && !IsHorizontalGap(nearby)) {
                primaryPoints.erase(nearby);
                QueueOtherTerrainNeighbours(nearby);
            }
        }
        if (point.x > 0) {
            TRmgGridPoint nearby(point.x - 1, point.y);
            if (primaryPoints.find(nearby) != primaryPoints.end()
                && !IsVerticalGap(nearby)) {
                primaryPoints.erase(nearby);
                QueueOtherTerrainNeighbours(nearby);
            }
        }
        if (point.x < width - 1) {
            TRmgGridPoint nearby(point.x + 1, point.y);
            if (primaryPoints.find(nearby) != primaryPoints.end()
                && !IsVerticalGap(nearby)) {
                primaryPoints.erase(nearby);
                QueueOtherTerrainNeighbours(nearby);
            }
        }
    } else {
        unsigned char neighbourExists[TILE_DIR_COUNT];
        BuildTileNeighbourMask(
            width, height, point.x, point.y, neighbourExists);
        for (unsigned int direction = 0; direction < TILE_DIR_COUNT; ++direction) {
            if (neighbourExists[direction]) {
                TRmgGridPoint nearby = point + gTileDirections[direction];
                if (GetTerrain(nearby) == paintTerrain) {
                    if (primaryPoints.find(nearby) != primaryPoints.end()) {
                        if (!NeedsTerrainRepair(nearby)) {
                            primaryPoints.erase(nearby);
                            QueueOtherTerrainNeighbours(nearby);
                        }
                    } else if (NeedsTerrainRepair(nearby)) {
                        primaryPoints.insert(nearby);
                    }
                }
            }
        }
    }
    if (NeedsTerrainRepair(point))
        primaryPoints.insert(point);
    else
        QueueOtherTerrainNeighbours(point);
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
void TRmgTerrainPainter::QueueOtherTerrainNeighbours(const TRmgGridPoint& point)
{
    if (point.y > 0
        && GetTerrain(TRmgGridPoint(point.x, point.y - 1)) != paintTerrain) {
        secondaryPoints.insert(TRmgGridPoint(point.x, point.y - 1));
    } else if (point.y < height - 1
        && GetTerrain(TRmgGridPoint(point.x, point.y + 1)) != paintTerrain) {
        secondaryPoints.insert(TRmgGridPoint(point.x, point.y + 1));
    }
    if (point.x > 0
        && GetTerrain(TRmgGridPoint(point.x - 1, point.y)) != paintTerrain) {
        secondaryPoints.insert(TRmgGridPoint(point.x - 1, point.y));
    } else if (point.x < width - 1
        && GetTerrain(TRmgGridPoint(point.x + 1, point.y)) != paintTerrain) {
        secondaryPoints.insert(TRmgGridPoint(point.x + 1, point.y));
    }
    if (point.x > 0 && point.y > 0) {
        TRmgGridPoint nearby(point.x - 1, point.y - 1);
        int terrain = GetTerrain(nearby);
        if (terrain != paintTerrain
            && !gRmgTerrainRules[terrain]->allowsSeparatedNeighbours)
            secondaryPoints.insert(nearby);
    }
    if (point.x < width - 1 && point.y > 0) {
        TRmgGridPoint nearby(point.x + 1, point.y - 1);
        int terrain = GetTerrain(nearby);
        if (terrain != paintTerrain
            && !gRmgTerrainRules[terrain]->allowsSeparatedNeighbours)
            secondaryPoints.insert(nearby);
    }
    if (point.x > 0 && point.y < height - 1) {
        TRmgGridPoint nearby(point.x - 1, point.y + 1);
        int terrain = GetTerrain(nearby);
        if (terrain != paintTerrain
            && !gRmgTerrainRules[terrain]->allowsSeparatedNeighbours)
            secondaryPoints.insert(nearby);
    }
    if (point.x < width - 1 && point.y < height - 1) {
        TRmgGridPoint nearby(point.x + 1, point.y + 1);
        int terrain = GetTerrain(nearby);
        if (terrain != paintTerrain
            && !gRmgTerrainRules[terrain]->allowsSeparatedNeighbours)
            secondaryPoints.insert(nearby);
    }
}

// Own-terrain checks share the predicates used with the selected paint
// terrain. Retail retains the nested predicate in the former and expands
// the latter at the four adjacent-row/column probes.
unsigned char TRmgTerrainPainter::IsHorizontalGap(const TRmgGridPoint& point)
{
    return IsHorizontalGap(point, GetTerrain(point));
}

unsigned char TRmgTerrainPainter::IsVerticalGap(const TRmgGridPoint& point)
{
    return IsVerticalGap(point, GetTerrain(point));
}

// The repeated four-check expansion in RepairTerrainPoint and the painter
// worklist decides whether a neighbour itself needs repair. The boundary
// and role are inferred from retail; there is no Dreamcast counterpart.
// Return the separation helper's existing 0/1 result directly after the
// guards. Booleanizing it through &&, != 0, or a final conditional 1/0
// instead changes the two destructors' cmp al,bl into test al,al. This one
// comparison was their final raw-byte difference (549 and 521 bytes).
unsigned char TRmgTerrainPainter::NeedsTerrainRepair(const TRmgGridPoint& point)
{
    if (IsHorizontalGap(point) || IsVerticalGap(point))
        return 1;
    if (gRmgTerrainRules[GetTerrain(point)]->allowsSeparatedNeighbours)
        return 0;
    return HasSeparatedNeighbours(point);
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
void TRmgTerrainPainter::RepairTerrainPoint(const TRmgGridPoint& point)
{
    if (IsVerticalGap(point)) {
        PaintPoint(
            !NeedsTerrainRepair(TRmgGridPoint(point.x, point.y - 1))
                && (NeedsTerrainRepair(TRmgGridPoint(point.x, point.y + 1))
                    || (IsHorizontalGap(
                            TRmgGridPoint(point.x, point.y - 1), paintTerrain)
                        && !IsHorizontalGap(
                            TRmgGridPoint(point.x, point.y + 1), paintTerrain)))
            ? TRmgGridPoint(point.x, point.y + 1)
            : TRmgGridPoint(point.x, point.y - 1));
    }
    if (IsHorizontalGap(point)) {
        PaintPoint(
            !NeedsTerrainRepair(TRmgGridPoint(point.x - 1, point.y))
                && (NeedsTerrainRepair(TRmgGridPoint(point.x + 1, point.y))
                    || (IsVerticalGap(
                            TRmgGridPoint(point.x - 1, point.y), paintTerrain)
                        && !IsVerticalGap(
                            TRmgGridPoint(point.x + 1, point.y), paintTerrain)))
            ? TRmgGridPoint(point.x + 1, point.y)
            : TRmgGridPoint(point.x - 1, point.y));
    }

    if (!gRmgTerrainRules[paintTerrain]->allowsSeparatedNeighbours
        && HasSeparatedNeighbours(point)) {
        unsigned char matches[TILE_DIR_COUNT];
        BuildMatchingNeighbourMask(point, matches);
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
            width, height, point.x, point.y, neighbourExists);
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
                    PaintPoint(point + gTileDirections[direction]);
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
void TRmgTerrainPainter::PaintTransitions()
{
    std::vector<unsigned char> edgeCounts(width * height);
    TRmgGridPoint point;

    for (point.y = 0; point.y < height - 1; ++point.y) {
        int terrain = GetTerrain(TRmgGridPoint(0, point.y));

        if (GetTerrain(TRmgGridPoint(1, point.y)) != terrain) {
            ++edgeCounts[point.y * width];
            ++edgeCounts[point.y * width + 1];
        }
        if (GetTerrain(TRmgGridPoint(1, point.y + 1)) != terrain) {
            ++edgeCounts[point.y * width];
            ++edgeCounts[(point.y + 1) * width + 1];
        }
        if (GetTerrain(TRmgGridPoint(0, point.y + 1)) != terrain) {
            ++edgeCounts[point.y * width];
            ++edgeCounts[(point.y + 1) * width];
        }

        for (point.x = 1; point.x < width - 1; ++point.x) {
            terrain = GetTerrain(point);

            if (GetTerrain(TRmgGridPoint(point.x + 1, point.y)) != terrain) {
                ++edgeCounts[point.y * width + point.x];
                ++edgeCounts[point.y * width + point.x + 1];
            }
            if (GetTerrain(TRmgGridPoint(point.x + 1, point.y + 1)) != terrain) {
                ++edgeCounts[point.y * width + point.x];
                ++edgeCounts[(point.y + 1) * width + point.x + 1];
            }
            if (GetTerrain(TRmgGridPoint(point.x, point.y + 1)) != terrain) {
                ++edgeCounts[point.y * width + point.x];
                ++edgeCounts[(point.y + 1) * width + point.x];
            }
            if (GetTerrain(TRmgGridPoint(point.x - 1, point.y + 1)) != terrain) {
                ++edgeCounts[point.y * width + point.x];
                ++edgeCounts[(point.y + 1) * width + point.x - 1];
            }
        }

        terrain = GetTerrain(point);
        if (GetTerrain(TRmgGridPoint(point.x, point.y + 1)) != terrain) {
            ++edgeCounts[point.y * width + point.x];
            ++edgeCounts[(point.y + 1) * width + point.x];
        }
        if (GetTerrain(TRmgGridPoint(point.x - 1, point.y + 1)) != terrain) {
            ++edgeCounts[point.y * width + point.x];
            ++edgeCounts[(point.y + 1) * width + point.x - 1];
        }
    }

    for (point.x = 0; point.x < width - 1; ++point.x) {
        int terrain = GetTerrain(point);
        if (GetTerrain(TRmgGridPoint(point.x + 1, point.y)) != terrain) {
            ++edgeCounts[point.y * width + point.x];
            ++edgeCounts[point.y * width + point.x + 1];
        }
    }

    for (point.y = 0; point.y < height; ++point.y) {
        for (point.x = 0; point.x < width; ++point.x) {
            unsigned int index = point.y * width + point.x;

            if (edgeCounts[index] > 0) {
                int neighbours[8];
                BuildNeighbourKinds(point, neighbours);

                int transition;
                TRmgTerrainFlip flip;
                transition = SelectTerrainTransition(neighbours, &flip);
                if (transition == RMG_TERRAIN_FIRST_DIAGONAL_LOW) {
                    if (CheckFirstDiagonal(point, flip))
                        transition = 6;
                } else if (transition == RMG_TERRAIN_FIRST_DIAGONAL_HIGH) {
                    if (CheckFirstDiagonal(point, flip))
                        transition = 12;
                } else if (transition == RMG_TERRAIN_SECOND_DIAGONAL_LOW) {
                    if (CheckSecondDiagonal(point, flip))
                        transition = 7;
                } else if (transition == RMG_TERRAIN_SECOND_DIAGONAL_HIGH) {
                    if (CheckSecondDiagonal(point, flip))
                        transition = 13;
                }

                TRmgTerrainTile tile = GetPackedCell(point)->GetTile();

                int newFrame;
                if (transition) {
                    newFrame = gRmgTerrainRules[tile.terrain]
                        ->SelectTransitionFrame(
                            transition, flip, flip, tile.frame);
                } else {
                    newFrame = SelectBaseFrame(point, tile.terrain, tile.frame);
                }

                if (tile.frame != newFrame || tile.flipX != flip.flipX
                    || tile.flipY != flip.flipY) {
                    tile.flipX = flip.flipX;
                    tile.flipY = flip.flipY;
                    tile.frame = newFrame;
                    SetTile(point, tile);
                }
            } else {
                TRmgTerrainTile tile = GetPackedCell(point)->GetTile();

                int newFrame = SelectBaseFrame(point, tile.terrain, tile.frame);
                if (tile.frame != newFrame || tile.flipX || tile.flipY) {
                    tile.frame = newFrame;
                    tile.flipX = 0;
                    tile.flipY = 0;
                    SetTile(point, tile);
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
unsigned char TRmgTerrainPainter::IsHorizontalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.x > 0 && point.x < width - 1
        && GetTerrain(TRmgGridPoint(point.x - 1, point.y)) != terrain
        && GetTerrain(TRmgGridPoint(point.x + 1, point.y)) != terrain;
}

VA(0x005B6430, 0x106) // anchor-callee 0x5b545f; retail-only
unsigned char TRmgTerrainPainter::IsVerticalGap(
    const TRmgGridPoint& point, int terrain)
{
    return point.y > 0 && point.y < height - 1
        && GetTerrain(TRmgGridPoint(point.x, point.y - 1)) != terrain
        && GetTerrain(TRmgGridPoint(point.x, point.y + 1)) != terrain;
}

// Cardinal neighbours use coordinates clamped to the map edge. A diagonal
// contributes only when at least one adjoining cardinal cell also matches.
// Remaining nested-inline decisions: retail retains GetPackedCell for the
// center and all four cardinals; this spelling already expands it for east.
// At southeast retail also expands InitializePackedCell through adapter
// slot +0x10, whereas this candidate retains InitializePackedCell.
VA(0x005B6540, 0x2CA) // anchor-callee 0x5b58f8, 0x5b681e; retail-only
void TRmgTerrainPainter::BuildMatchingNeighbourMask(
    const TRmgGridPoint& point, unsigned char* matches)
{
    int terrain = GetTerrain(point);
    unsigned int north = point.y > 0 ? point.y - 1 : point.y;
    unsigned int south = point.y < height - 1 ? point.y + 1 : point.y;
    unsigned int west = point.x > 0 ? point.x - 1 : point.x;
    unsigned int east = point.x < width - 1 ? point.x + 1 : point.x;

    matches[TILE_DIR_NORTH] = GetTerrain(TRmgGridPoint(point.x, north)) == terrain;
    matches[TILE_DIR_SOUTH] = GetTerrain(TRmgGridPoint(point.x, south)) == terrain;
    matches[TILE_DIR_WEST] = GetTerrain(TRmgGridPoint(west, point.y)) == terrain;
    matches[TILE_DIR_EAST] = GetTerrain(TRmgGridPoint(east, point.y)) == terrain;
    matches[TILE_DIR_NORTHWEST] =
        (matches[TILE_DIR_NORTH] || matches[TILE_DIR_WEST])
        && GetTerrain(TRmgGridPoint(west, north)) == terrain;
    matches[TILE_DIR_NORTHEAST] =
        (matches[TILE_DIR_NORTH] || matches[TILE_DIR_EAST])
        && GetTerrain(TRmgGridPoint(east, north)) == terrain;
    matches[TILE_DIR_SOUTHWEST] =
        (matches[TILE_DIR_SOUTH] || matches[TILE_DIR_WEST])
        && GetTerrain(TRmgGridPoint(west, south)) == terrain;
    matches[TILE_DIR_SOUTHEAST] =
        (matches[TILE_DIR_SOUTH] || matches[TILE_DIR_EAST])
        && GetTerrain(TRmgGridPoint(east, south)) == terrain;
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
unsigned char TRmgTerrainPainter::HasSeparatedNeighbours(const TRmgGridPoint& point)
{
    unsigned char matches[TILE_DIR_COUNT];
    BuildMatchingNeighbourMask(point, matches);
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
void TRmgTerrainPainter::BuildNeighbourKinds(
    const TRmgGridPoint& point, int* neighbours)
{
}  // @stub

VA(0x005B6BA0, 0x24C)  // transition 2/8 tests; retail-only
unsigned char TRmgTerrainPainter::CheckFirstDiagonal(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    return 0;  // @stub
}

VA(0x005B6E00, 0x1B3)  // transition 5/11 tests; retail-only
unsigned char TRmgTerrainPainter::CheckSecondDiagonal(
    const TRmgGridPoint& point, const TRmgTerrainFlip& flip)
{
    return 0;  // @stub
}

VA(0x005B6FD0, 0x271)  // base-frame selection call; retail-only
int TRmgTerrainPainter::GetTransitionStrength(
    const TRmgGridPoint& point, int terrain)
{
    return 0;  // @stub
}
#endif

// The same primary/secondary worklist appears in the brush's terrain change
// and destructor. Preserve one ordinary completion helper and the canonical
// set erase(key); its distance walk expands only at the change site.
void TRmgTerrainPainter::Finish()
{
    do {
        while (primaryPoints.size()) {
            TRmgGridPoint point = *primaryPoints.begin();
            RepairTerrainPoint(point);
        }
        while (secondaryPoints.size()) {
            TRmgGridPoint point = *secondaryPoints.begin();
            secondaryPoints.erase(point);
            if (NeedsTerrainRepair(point))
                PaintPoint(point);
        }
    } while (primaryPoints.size());
    PaintTransitions();
}

void TRmgTerrainPainter::ChangeTerrain(int terrain, int strength)
{
    Finish();
    paintTerrain = terrain;
    transitionStrength = strength;
}

VA(0x005B7250, 0x9A) // anchor-callee 0x54017e; allocation and throw RTTI
TRmgTerrainBrush::TRmgTerrainBrush(
    TRmgMapAdapterInterface* map, int terrain, int strength)
    : painter(new TRmgTerrainPainter(map, terrain, strength))
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
    painter->ChangeTerrain(terrain, strength);
}

VA(0x005B7690, 0x1F) // anchor-callee 0x5401e9; four unsigned rectangle args
void TRmgTerrainBrush::PaintRectangle(
    unsigned int x, unsigned int y,
    unsigned int rectangleWidth, unsigned int rectangleHeight)
{
    painter->PaintRectangle(x, y, rectangleWidth, rectangleHeight);
}

VA(0x005B76F0, 0x209) // anchor-callee 0x5b76d9; retained painter destructor
TRmgTerrainPainter::~TRmgTerrainPainter()
{
    Finish();
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
