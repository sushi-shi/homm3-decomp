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

#endif  // HOMM3_TERRAIN_TYPE_H
