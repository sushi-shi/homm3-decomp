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

// Retail TRmgZone::chooseTerrain (0x532ab0) traverses the allowed-terrain
// byte array with a signed integer index, then stores that ordinal directly
// into its four-byte TTerrainType field. Representation bridge for ordinals.
inline TTerrainType terrainFromInt(int value)
{
    union {
        int m_integer;
        TTerrainType m_terrain;
    } converted;
    converted.m_integer = value;
    return converted.m_terrain;
}

#endif  // HOMM3_TERRAIN_TYPE_H
