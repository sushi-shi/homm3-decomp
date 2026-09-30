// terrain_type.h - shared adventure-map terrain domain.
#ifndef HOMM3_TERRAIN_TYPE_H
#define HOMM3_TERRAIN_TYPE_H

// The ten-mask analysis recovers the complete ordering
// {Dirt=0, Sand=1, Grass=2, Snow=3, Swamp=4, Rough=5,
// Subterranean=6, Lava=7, Water=8, Rock=9}.
enum TTerrainType {
    TERRAIN_NONE = -1,
    eTerrainDirt = 0,
    eTerrainSand = 1,
    eTerrainGrass = 2,
    eTerrainSnow = 3,
    eTerrainSwamp = 4,
    eTerrainRough = 5,
    eTerrainSubterranean = 6,
    eTerrainLava = 7,
    eTerrainWater = 8,
    eTerrainRock = 9
};
// Dreamcast's enum also carries kNumTerrainTypes = 10. Adding that
// enumerator here changes VC6's code for recruitUnit::update (0x5503a0,
// CUR 99.94% -> 97.54%, 2026-09-29), so Complete's TTerrainType is kept
// without it; tables sized by terrain spell the count at their declaration.

#endif  // HOMM3_TERRAIN_TYPE_H
