// terrain_type.h - shared adventure-map terrain domain.
#ifndef HOMM3_TERRAIN_TYPE_H
#define HOMM3_TERRAIN_TYPE_H

// The ten-mask analysis recovers the complete ordering
// {Dirt=0, Sand=1, Grass=2, Snow=3, Swamp=4, Rough=5,
// Subterranean=6, Lava=7, Water=8, Rock=9}.  Only names needed by
// retail-proven consumers are admitted here so far.
enum TTerrainType {
    TERRAIN_NONE = -1,
    eTerrainSnow = 3,
    eTerrainLava = 7,
    eTerrainWater = 8,
    eTerrainRock = 9
};

#endif  // HOMM3_TERRAIN_TYPE_H
