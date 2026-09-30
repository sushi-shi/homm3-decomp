#ifndef HOMM3_CONTENT_COUNTS_H
#define HOMM3_CONTENT_COUNTS_H

// Native game content, including Cove. Keep table owners and consumers on
// the same dimensions; serialized legacy counts are handled by the loaders.
enum {
    HOMM3_SAVE_VERSION = 43,
    HOMM3_TOWN_COUNT = 10,
    HOMM3_CREATURE_COUNT = 166,
    HOMM3_HERO_CLASS_COUNT = 20,
    HOMM3_HERO_COUNT = 183,
    HOMM3_HERO_TRAIT_COUNT = 183,
    HOMM3_BUILDING_COUNT = 45,
    HOMM3_DWELLING_COST_COUNT = 15
};
#endif
