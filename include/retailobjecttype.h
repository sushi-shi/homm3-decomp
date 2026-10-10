// retailobjecttype.h - Complete's adventure-object types after the
// Dreamcast domain (adventureobjecttype.h): DispatchEvent's and the map
// loader's arms in advmgr, and the map editor's object tables, which use it
// without advmgr.h.
#ifndef HOMM3_RETAILOBJECTTYPE_H
#define HOMM3_RETAILOBJECTTYPE_H

enum EAdvmgrRetailObjectType {
    // Complete's second obstacle set (165..205: the Dreamcast TERRAIN_*
    // obstacles again, each with _2 as CLOVER_FIELD_2 is spelled) and the
    // hills and foliage after it (206..211). The map editor's find list
    // pairs each of the former with its first-set type (h3maped
    // FindDlg.cpp's general patterns) and lists the latter on their own;
    // no original name survives.
    TERRAIN_BRUSH_2 = 165,
    TERRAIN_BUSH_2 = 166,
    TERRAIN_CACTUS_2 = 167,
    TERRAIN_CANYON_2 = 168,
    TERRAIN_CRATER_2 = 169,
    TERRAIN_DEAD_VEGETATION_2 = 170,
    TERRAIN_FLOWER_2 = 171,
    TERRAIN_FROZEN_LAKE_2 = 172,
    TERRAIN_HEDGE_2 = 173,
    TERRAIN_HILL_2 = 174,
    TERRAIN_HOLE_2 = 175,
    TERRAIN_KELP_2 = 176,
    TERRAIN_LAKE_2 = 177,
    TERRAIN_LAVA_FLOW_2 = 178,
    TERRAIN_LAVA_LAKE_2 = 179,
    TERRAIN_MUSHROOM_2 = 180,
    TERRAIN_LOG_2 = 181,
    TERRAIN_MANDRAKE_2 = 182,
    TERRAIN_MOSS_2 = 183,
    TERRAIN_MOUND_2 = 184,
    TERRAIN_MOUNTAIN_2 = 185,
    TERRAIN_OAK_TREE_2 = 186,
    TERRAIN_OUTCROPPING_2 = 187,
    TERRAIN_PINE_TREE_2 = 188,
    TERRAIN_PLANT_2 = 189,
    TERRAIN_RIVER_DELTA_2 = 190,
    TERRAIN_ROCK_2 = 191,
    TERRAIN_SAND_DUNE_2 = 192,
    TERRAIN_SAND_PIT_2 = 193,
    TERRAIN_SHRUB_2 = 194,
    TERRAIN_SKULL_2 = 195,
    TERRAIN_STALAGMITE_2 = 196,
    TERRAIN_STUMP_2 = 197,
    TERRAIN_TAR_PIT_2 = 198,
    TERRAIN_TREE_2 = 199,
    TERRAIN_VINE_2 = 200,
    TERRAIN_VOLCANIC_VENT_2 = 201,
    TERRAIN_VOLCANO_2 = 202,
    TERRAIN_WILLOW_TREE_2 = 203,
    TERRAIN_YUCCA_TREE_2 = 204,
    TERRAIN_REEF_2 = 205,
    TERRAIN_DESERT_HILLS = 206,
    TERRAIN_DIRT_HILLS = 207,
    TERRAIN_GRASS_HILLS = 208,
    TERRAIN_ROUGH_HILLS = 209,
    TERRAIN_SUBTERRANEAN_ROCKS = 210,
    TERRAIN_SWAMP_FOLIAGE = 211,
    BORDER_GATE = 212,
    // 213: DispatchEvent's 0xd5 arm gates DoFreelancersGuild(hero*) on
    // human_player - the map-object entry of tradpost's guild pair.
    FREELANCERS_GUILD = 213,
    HERO_PLACEHOLDER = 214,
    QUEST_GUARD = 215,
    RANDOM_DWELLING = 216,
    RANDOM_DWELLING_LVL = 217,
    RANDOM_DWELLING_FACTION = 218,
    // The map editor's object caps pair 219 with GARRISON and 220 with MINE
    // (h3maped's capped-type tables at 0x5351d0/0x5351e8). The spellings
    // are H3API's; no original name survives.
    GARRISON2 = 219,
    ABANDONED_MINE = 220,
    // The second spellings of the trading post, cursed ground and magic
    // plains, paired with the first in the editor's find list.
    TRADING_POST_2 = 221,
    CURSED_GROUND_2 = 223,
    MAGIC_PLAINS_2 = 230
};

#endif  // HOMM3_RETAILOBJECTTYPE_H
