// rmg_terrain.cpp - Complete-only random-map terrain transition support.
//
// The Dreamcast build contains no random-map generator compiland. Function
// ownership, field layout, helper boundaries, and call/expansion decisions in
// this unit therefore come directly from the retail x86 cluster.
#include <va.h>
#define _MT
#include <yvals.h>
#undef _MT
#include "rmg_terrain.h"

// Provisional role spelling. The fastcall ABI and two-byte output are fixed
// by the call at 0x5b5f4e; the 1,887-byte body remains an admission target.
int __fastcall SelectTerrainTransition(
    const int* neighbours, TRmgTerrainFlip* flip);

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

#if 0  // @carcass - retained transition selector
VA(0x005B3E80, 0x75F)  // fastcall call at 0x5b5f4e; retail-only
int __fastcall SelectTerrainTransition(
    const int* neighbours, TRmgTerrainFlip* flip)
{
    return 0;  // @stub
}
#endif

VA(0x005B48D0, 0x8D)  // repeated caller identity in 0x5b3dd0..0x5b76f0
TRmgPackedTerrainCell* TRmgTerrainPainter::GetPackedCell(
    const TPoint& point)
{
    unsigned int index = point.y * width + point.x;
    if (!packedCells[index].initialized)
        InitializePackedCell(point, index);
    return &packedCells[index];
}

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
