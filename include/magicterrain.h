// magicterrain.h - the battlefield magic-terrain domain.
// HAND-OWNED. Its own header rather than mapcell.h (whose
// NewmapCell::get_magic_terrain_type produces the value) because
// mapcell.h is inside initialize.cpp's include closure, where a new
// type definition moves initialize_game_data with no semantic change
// (the EArtifactId precedent recorded in artifact.h). Only hero.cpp
// needs the names today.
#ifndef HOMM3_MAGICTERRAIN_H
#define HOMM3_MAGICTERRAIN_H

// The battlefield's magic-terrain id, the second argument of the
// spell-school quartet in hero.cpp. Byte-proven as a domain by
// NewmapCell::get_magic_terrain_type (0x4fcf40), whose whole body is a
// 186-entry byte index into a six-arm jump table returning exactly
// 1 / 6 / 7 / 8 / 9 / -1, and by hero::GetSpellSchoolLevel (0x4e5100),
// which switches the same value over 1..9 - nine table slots, of which
// 2..5 fall to the default. The four unnamed slots are HoMM3's
// remaining terrain specials (cursed ground, holy ground, evil fog,
// clover field), none of which touches a spell school; they stay
// unnamed rather than invented.
enum TMagicTerrain {
    kMagicTerrainNone = -1,
    kMagicTerrainMagicPlains = 1,
    kMagicTerrainLucidPools = 6,
    kMagicTerrainFieryFields = 7,
    kMagicTerrainRocklands = 8,
    kMagicTerrainMagicClouds = 9
};

#endif  /* HOMM3_MAGICTERRAIN_H */
