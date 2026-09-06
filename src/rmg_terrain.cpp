// rmg_terrain.cpp - Complete-only random-map terrain transition support.
//
// The Dreamcast build contains no random-map generator compiland. Function
// ownership, field layout, helper boundaries, and call/expansion decisions in
// this unit therefore come directly from the retail x86 cluster.
#include <va.h>
#include "rmg_terrain.h"

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
    const TPoint& point, unsigned int index)
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

VA(0x005B48D0, 0x8D)  // repeated caller identity in 0x5b3dd0..0x5b76f0
TRmgPackedTerrainCell* TRmgTerrainPainter::GetPackedCell(
    const TPoint& point)
{
    unsigned int index = point.y * width + point.x;
    if (!packedCells[index].initialized)
        InitializePackedCell(point, index);
    return &packedCells[index];
}

// Residual (74.7623%): the first twelve terrain reads retain GetPackedCell
// in retail; this candidate retains seven and expands five only as far as
// InitializePackedCell. The last-row pair and final full-tile reads have the
// correct boundaries. Both cache helpers' standalone bodies remain exact.
// Retail also evaluates GetTransitionStrength before loading the rule's
// virtual receiver (as in neighboring 0x5b4960); spelling two strength locals
// reproduces that order but changes earlier inlining and falls to 41.51%.
// Rejected controls: GetTerrain via GetTile adds eleven calls (75.28%);
// reversed dimension product, explicit vector fill, postfix edge increments,
// function-scope terrain, transition initializer, and earlier tile declaration
// are byte-flat. These do not establish the missing source/helper state.
VA(0x005B5A70, 0x8A7)  // caller cluster reaches Complete RMG; retail-only
void TRmgTerrainPainter::PaintTransitions()
{
    std::vector<unsigned char> edgeCounts(width * height);
    TPoint point;

    for (point.y = 0; point.y < height - 1; ++point.y) {
        int terrain = GetTerrain(TPoint(0, point.y));

        if (GetTerrain(TPoint(1, point.y)) != terrain) {
            ++edgeCounts[point.y * width];
            ++edgeCounts[point.y * width + 1];
        }
        if (GetTerrain(TPoint(1, point.y + 1)) != terrain) {
            ++edgeCounts[point.y * width];
            ++edgeCounts[(point.y + 1) * width + 1];
        }
        if (GetTerrain(TPoint(0, point.y + 1)) != terrain) {
            ++edgeCounts[point.y * width];
            ++edgeCounts[(point.y + 1) * width];
        }

        for (point.x = 1; point.x < width - 1; ++point.x) {
            terrain = GetTerrain(point);

            if (GetTerrain(TPoint(point.x + 1, point.y)) != terrain) {
                ++edgeCounts[point.y * width + point.x];
                ++edgeCounts[point.y * width + point.x + 1];
            }
            if (GetTerrain(TPoint(point.x + 1, point.y + 1)) != terrain) {
                ++edgeCounts[point.y * width + point.x];
                ++edgeCounts[(point.y + 1) * width + point.x + 1];
            }
            if (GetTerrain(TPoint(point.x, point.y + 1)) != terrain) {
                ++edgeCounts[point.y * width + point.x];
                ++edgeCounts[(point.y + 1) * width + point.x];
            }
            if (GetTerrain(TPoint(point.x - 1, point.y + 1)) != terrain) {
                ++edgeCounts[point.y * width + point.x];
                ++edgeCounts[(point.y + 1) * width + point.x - 1];
            }
        }

        terrain = GetTerrain(point);
        if (GetTerrain(TPoint(point.x, point.y + 1)) != terrain) {
            ++edgeCounts[point.y * width + point.x];
            ++edgeCounts[(point.y + 1) * width + point.x];
        }
        if (GetTerrain(TPoint(point.x - 1, point.y + 1)) != terrain) {
            ++edgeCounts[point.y * width + point.x];
            ++edgeCounts[(point.y + 1) * width + point.x - 1];
        }
    }

    for (point.x = 0; point.x < width - 1; ++point.x) {
        int terrain = GetTerrain(point);
        if (GetTerrain(TPoint(point.x + 1, point.y)) != terrain) {
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
                    newFrame = gRmgTerrainRules[tile.terrain]
                        ->SelectBaseFrame(
                            GetTransitionStrength(point, tile.terrain),
                            tile.frame);
                }

                if (tile.frame != newFrame || tile.flipX != flip.flipX
                    || tile.flipY != flip.flipY) {
                    tile.flipX = flip.flipX;
                    tile.flipY = flip.flipY;
                    tile.frame = newFrame;
                    adapter->SetTile(point, tile);

                    TRmgPackedTerrainCell& updatedPacked =
                        packedCells[point.y * width + point.x];
                    updatedPacked.SetInitialized();
                    updatedPacked.SetTerrain(tile.terrain);
                    updatedPacked.SetFrame(tile.frame);
                    updatedPacked.SetFlipX(tile.flipX);
                    updatedPacked.SetFlipY(tile.flipY);
                }
            } else {
                TRmgTerrainTile tile = GetPackedCell(point)->GetTile();

                int newFrame = gRmgTerrainRules[tile.terrain]
                    ->SelectBaseFrame(
                        GetTransitionStrength(point, tile.terrain),
                        tile.frame);
                if (tile.frame != newFrame || tile.flipX || tile.flipY) {
                    tile.frame = newFrame;
                    tile.flipX = 0;
                    tile.flipY = 0;
                    adapter->SetTile(point, tile);

                    TRmgPackedTerrainCell& updatedPacked =
                        packedCells[point.y * width + point.x];
                    updatedPacked.SetInitialized();
                    updatedPacked.SetTerrain(tile.terrain);
                    updatedPacked.SetFrame(tile.frame);
                    updatedPacked.SetFlipX(tile.flipX);
                    updatedPacked.SetFlipY(tile.flipY);
                }
            }
        }
    }
}

#if 0  // @carcass - direct retained callees of PaintTransitions
VA(0x005B68A0, 0x2FF)  // thiscall at 0x5b5f45; retail-only
void TRmgTerrainPainter::BuildNeighbourKinds(
    const TPoint& point, int* neighbours)
{
}  // @stub

VA(0x005B6BA0, 0x24C)  // transition 2/8 tests; retail-only
unsigned char TRmgTerrainPainter::CheckFirstDiagonal(
    const TPoint& point, const TRmgTerrainFlip& flip)
{
    return 0;  // @stub
}

VA(0x005B6E00, 0x1B3)  // transition 5/11 tests; retail-only
unsigned char TRmgTerrainPainter::CheckSecondDiagonal(
    const TPoint& point, const TRmgTerrainFlip& flip)
{
    return 0;  // @stub
}

VA(0x005B6FD0, 0x271)  // base-frame selection call; retail-only
int TRmgTerrainPainter::GetTransitionStrength(
    const TPoint& point, int terrain)
{
    return 0;  // @stub
}
#endif
