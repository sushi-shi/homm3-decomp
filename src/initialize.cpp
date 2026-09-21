#include "terrain.h"
#include "town.h"
#include <va.h>
#include <string.h>
// #include "initialize.h"

VA(0x004eb730, 0x3D5)  // dc 0xdc614
void initializeGameData();

// The nine town_buildings walks create_building_masks feeds to
// create_requirement_masks, town types 0..8, then the ten include
// lists for create_included_masks. Row grammar (byte-derived from the
// 0x4ebb10/0x4ebc50 loops): building id, its direct requirements until
// -1, next building...; -100 ends the list. Ids are the 0..43
// bitNumber slot space; every value below is the retail .rdata run
// 0x63ec80..0x63fe10 verbatim.
DATA(0x0063ec80)
static const int g_town0Buildings[] = {
    0, -1, 1, 0, -1, 2, 1, -1, 3, 2, -1, 6, -1, 5, -1, 22, 5, -1, 14, -1,
    15, 14, -1, 16, -1, 17, 6, -1, 7, -1, 8, 7, -1, 9, 8, -1, 11, 5, -1,
    12, 11, 16, 0, 14, -1, 13, 12, 9, -1, 30, 7, -1, 37, 30, -1, 31, 30, -1,
    38, 31, -1, 33, 30, 16, -1, 40, 33, -1, 32, 33, -1, 18, 32, -1,
    39, 32, -1, 19, 39, -1, 21, 33, -1, 35, 21, -1, 42, 35, -1, 34, 33, 0, -1,
    41, 34, -1, 36, 34, -1, 43, 36, -1, 26, -1,
    -100,
};

DATA(0x0063ee24)
static const int g_town1Buildings[] = {
    7, -1, 8, 7, -1, 9, 8, -1, 5, -1, 16, -1, 14, -1, 15, 14, -1, 0, -1,
    1, 0, -1, 2, 1, -1, 3, 2, -1, 4, 3, -1, 17, -1, 21, 17, -1, 11, 5, -1,
    12, 11, 0, 16, 14, -1, 13, 12, 9, -1, 30, 7, -1, 37, 30, -1, 31, 30, -1,
    18, 31, -1, 22, 18, -1, 38, 31, -1, 19, 38, -1, 32, 30, -1, 39, 32, -1,
    34, 32, -1, 24, 34, -1, 41, 34, -1, 25, 41, -1, 33, 32, -1, 40, 33, -1,
    35, 33, 34, -1, 42, 35, -1, 36, 35, 1, -1, 43, 36, 2, -1, 26, -1,
    -100,
};

DATA(0x0063efe4)
static const int g_town2Buildings[] = {
    7, -1, 21, 7, -1, 8, 7, -1, 9, 8, -1, 0, -1, 22, 0, -1, 23, 0, -1,
    1, 0, -1, 2, 1, -1, 3, 2, -1, 4, 3, -1, 5, -1, 16, -1, 14, -1, 15, 14, -1,
    17, 14, -1, 11, 5, -1, 12, 11, 14, 16, 0, -1, 13, 12, 9, -1, 30, 7, -1,
    37, 30, -1, 31, 30, -1, 18, 31, -1, 38, 31, -1, 19, 38, -1, 32, 30, -1,
    39, 32, -1, 33, 31, 32, 0, -1, 40, 33, 22, -1, 34, 33, -1, 41, 34, -1,
    35, 33, -1, 42, 35, -1, 36, 34, 35, -1, 43, 36, -1, 26, -1,
    -100,
};

DATA(0x0063f1a0)
static const int g_town3Buildings[] = {
    7, -1, 21, 7, -1, 8, 7, -1, 22, 8, -1, 9, 8, -1, 5, -1, 16, -1, 14, -1,
    15, 14, -1, 0, -1, 23, 0, -1, 1, 0, -1, 2, 1, -1, 3, 2, -1, 4, 3, -1,
    11, 5, -1, 12, 11, 16, 14, 0, -1, 13, 12, 9, -1, 30, 7, -1, 18, 30, -1,
    37, 30, -1, 19, 37, -1, 32, 30, -1, 24, 32, -1, 39, 32, -1, 25, 39, -1,
    31, 30, -1, 38, 31, -1, 33, 31, -1, 40, 33, -1, 35, 33, 0, -1, 42, 35, -1,
    34, 33, -1, 41, 34, 1, -1, 36, 34, 35, -1, 43, 36, -1, 26, -1,
    -100,
};

DATA(0x0063f364)
static const int g_town4Buildings[] = {
    7, -1, 17, 7, -1, 8, 7, -1, 9, 8, -1, 5, -1, 16, -1, 14, -1, 15, 14, -1,
    6, -1, 0, -1, 21, 0, -1, 1, 0, -1, 2, 1, -1, 3, 2, -1, 4, 3, -1,
    11, 5, -1, 12, 11, 14, 16, 0, -1, 13, 12, 9, -1, 30, 7, -1, 22, 30, -1,
    18, 22, -1, 37, 30, -1, 19, 37, 22, -1, 32, 30, -1, 39, 32, -1,
    31, 30, -1, 38, 31, -1, 33, 31, -1, 40, 33, 21, -1, 34, 31, 0, -1,
    41, 34, -1, 35, 33, 34, -1, 42, 35, -1, 36, 35, -1, 43, 36, -1, 26, -1,
    -100,
};

DATA(0x0063f51c)
static const int g_town5Buildings[] = {
    7, -1, 8, 7, -1, 9, 8, -1, 22, -1, 23, -1, 5, -1, 16, -1, 14, -1,
    15, 14, -1, 17, 14, -1, 0, -1, 21, 0, -1, 1, 0, -1, 2, 1, -1, 3, 2, -1,
    4, 3, -1, 11, 5, -1, 12, 11, 16, 14, 0, -1, 13, 12, 9, -1, 30, 7, -1,
    18, 30, -1, 37, 30, -1, 19, 37, -1, 32, 30, -1, 39, 32, -1, 31, 30, -1,
    38, 31, -1, 33, 32, 31, -1, 40, 33, -1, 34, 33, -1, 41, 34, -1,
    35, 33, -1, 42, 35, -1, 36, 34, 35, 1, -1, 43, 36, 2, -1, 26, -1,
    -100,
};

DATA(0x0063f6d0)
static const int g_town6Buildings[] = {
    7, -1, 17, 7, -1, 23, 7, -1, 8, 7, -1, 9, 8, -1, 5, -1, 16, -1,
    22, 16, -1, 14, -1, 15, 14, -1, 21, 14, -1, 0, -1, 1, 0, -1, 2, 1, -1,
    11, 5, -1, 12, 11, 0, 14, 16, -1, 13, 12, 9, -1, 30, 7, -1, 18, 30, -1,
    37, 30, -1, 19, 37, -1, 31, 30, -1, 38, 31, 37, -1, 34, 31, -1,
    41, 34, -1, 36, 34, -1, 43, 36, -1, 32, 30, -1, 39, 32, 16, -1,
    33, 32, -1, 40, 33, 0, -1, 35, 33, -1, 42, 35, -1, 26, -1,
    -100,
};

DATA(0x0063f870)
static const int g_town7Buildings[] = {
    7, -1, 21, 7, -1, 22, 21, -1, 8, 7, -1, 9, 8, -1, 5, -1, 16, -1, 14, -1,
    15, 14, -1, 0, -1, 1, 0, -1, 2, 1, -1, 11, 5, -1, 17, 11, 21, -1,
    12, 11, 14, 16, 0, -1, 6, -1, 13, 12, 9, -1, 30, 7, -1, 18, 30, -1,
    37, 30, 5, -1, 19, 37, -1, 31, 30, -1, 38, 31, -1, 35, 31, -1, 42, 35, -1,
    32, 30, -1, 39, 32, -1, 33, 32, -1, 40, 33, -1, 34, 32, 31, -1,
    41, 34, 15, -1, 36, 35, 33, -1, 43, 36, -1, 26, -1,
    -100,
};

DATA(0x0063fa14)
static const int g_town8Buildings[] = {
    7, -1, 8, 7, -1, 9, 8, -1, 6, -1, 5, -1, 16, -1, 14, -1, 15, 14, -1,
    17, 14, -1, 0, -1, 1, 0, -1, 2, 1, -1, 3, 2, -1, 4, 3, -1, 21, 0, -1,
    11, 5, -1, 12, 11, 14, 16, 0, -1, 13, 12, 9, -1, 30, 7, -1, 18, 30, -1,
    37, 30, -1, 19, 37, -1, 31, 30, 0, -1, 38, 31, -1, 32, 30, 0, -1,
    39, 32, -1, 33, 31, -1, 40, 33, 38, -1, 34, 32, -1, 41, 34, -1,
    35, 33, 34, -1, 42, 35, 1, -1, 36, 35, -1, 43, 36, -1, 26, -1,
    -100,
};

// The town-independent half of every included mask: each upgraded
// dwelling / guild tier includes its base (37 includes 30, 12 includes
// 11, ...). create_included_mask adds this before the per-town list.
DATA(0x0063fbc4)
static const int g_commonIncludeList[] = {
    1, 0, -1, 2, 1, -1, 3, 2, -1, 4, 3, -1, 8, 7, -1, 9, 8, -1, 11, 10, -1,
    12, 11, -1, 13, 12, -1, 19, 18, -1, 25, 24, -1, 37, 30, -1, 38, 31, -1,
    39, 32, -1, 40, 33, -1, 41, 34, -1, 42, 35, -1, 43, 36, -1,
    -100,
};

DATA(0x0063fca0)
static const int g_town0IncludeList[] = {
    22, 5, -1, 18, 32, -1, 19, 39, 18, -1,
    -100,
};

DATA(0x0063fccc)
static const int g_town1IncludeList[] = {
    18, 31, -1, 19, 38, 18, -1, 24, 34, -1, 25, 41, 24, -1, 21, 17, -1,
    -100,
};

DATA(0x0063fd14)
static const int g_town2IncludeList[] = {
    18, 31, -1, 19, 38, 18, -1,
    -100,
};

DATA(0x0063fd34)
static const int g_town3IncludeList[] = {
    18, 30, -1, 19, 37, 18, -1, 24, 32, -1, 25, 39, 24, -1,
    -100,
};

DATA(0x0063fd70)
static const int g_town4IncludeList[] = {
    18, 30, -1, 19, 37, 18, -1,
    -100,
};

DATA(0x0063fd90)
static const int g_town5IncludeList[] = {
    18, 30, -1, 19, 37, 18, -1,
    -100,
};

DATA(0x0063fdb0)
static const int g_town6IncludeList[] = {
    18, 30, -1, 19, 37, 18, -1,
    -100,
};

DATA(0x0063fdd0)
static const int g_town7IncludeList[] = {
    18, 30, -1, 19, 37, 18, -1,
    -100,
};

DATA(0x0063fdf0)
static const int g_town8IncludeList[] = {
    18, 30, -1, 19, 37, 18, -1,
    -100,
};

VA(0x004ebb10, 0x5A)  // dc 0xdc368
static void addToIncludedMask(const int* includeList, __int64* includedBuildings)
{
    do {
        int building = *includeList++;
        int included;
        while ((included = *includeList) >= 0) {
            includedBuildings[building] |= g_bitNumber[included];
            ++includeList;
            includedBuildings[building] |= includedBuildings[included];
        }
    } while (*++includeList >= 0);
}

VA(0x004ebb70, 0xD8)  // dc 0xdc3cc
static void createIncludedMask(const int* includeList, __int64* includedBuildings)
{
    const int* const commonList = g_commonIncludeList;
    MEMSET_LOCAL(includedBuildings, 0,
                 TOWN_BUILDING_SLOTS * sizeof(includedBuildings[0]),
                 TOWN_BUILDING_SLOTS, index);
    addToIncludedMask(commonList, includedBuildings);
    addToIncludedMask(includeList, includedBuildings);
}

// E:\gamedcs\initialize.cpp:538
// No retail body: called once from initialize_game_data, fully inlined
// there (rows 0/1/3/2 as inline copies, 4..8 as calls to 0x4ebb70),
// and - a file static with no reference left - not emitted. The 3/2
// call order is retail's own (the row-3 copy precedes the row-2 copy
// at 0x4eba42/0x4eba7d).

static void createIncludedMasks()
{
    createIncludedMask(g_town0IncludeList, town::s_includedBuildings[0]);
    createIncludedMask(g_town1IncludeList, town::s_includedBuildings[1]);
    createIncludedMask(g_town3IncludeList, town::s_includedBuildings[3]);
    createIncludedMask(g_town2IncludeList, town::s_includedBuildings[2]);
    createIncludedMask(g_town4IncludeList, town::s_includedBuildings[4]);
    createIncludedMask(g_town5IncludeList, town::s_includedBuildings[5]);
    createIncludedMask(g_town6IncludeList, town::s_includedBuildings[6]);
    createIncludedMask(g_town7IncludeList, town::s_includedBuildings[7]);
    createIncludedMask(g_town8IncludeList, town::s_includedBuildings[8]);
}

VA(0x004ebc50, 0x99)  // dc 0xdc4a4
static void createRequirementMasks(const int* townBuildings, __int64* requirements, __int64& legalBuildings)
{
    legalBuildings = 0;
    do {
        int building = *townBuildings++;
        int required;
        legalBuildings |= g_bitNumber[building];
        requirements[building] = 0;
        while ((required = *townBuildings) >= 0) {
            requirements[building] |= g_bitNumber[required];
            ++townBuildings;
            requirements[building] |= requirements[required];
        }
    } while (*++townBuildings >= 0);
}

// E:\gamedcs\initialize.cpp:594
// No retail body: called once from initialize_game_data, fully inlined
// (rows 0..2 as inline copies of create_requirement_masks, 3..8 as
// calls to 0x4ebc50), then dropped as an unreferenced static.

static void createBuildingMasks()
{
    createRequirementMasks(g_town0Buildings, g_hierarchyMask[0],
                             g_townEligibleBuildMask[0]);
    createRequirementMasks(g_town1Buildings, g_hierarchyMask[1],
                             g_townEligibleBuildMask[1]);
    createRequirementMasks(g_town2Buildings, g_hierarchyMask[2],
                             g_townEligibleBuildMask[2]);
    createRequirementMasks(g_town3Buildings, g_hierarchyMask[3],
                             g_townEligibleBuildMask[3]);
    createRequirementMasks(g_town4Buildings, g_hierarchyMask[4],
                             g_townEligibleBuildMask[4]);
    createRequirementMasks(g_town5Buildings, g_hierarchyMask[5],
                             g_townEligibleBuildMask[5]);
    createRequirementMasks(g_town6Buildings, g_hierarchyMask[6],
                             g_townEligibleBuildMask[6]);
    createRequirementMasks(g_town7Buildings, g_hierarchyMask[7],
                             g_townEligibleBuildMask[7]);
    createRequirementMasks(g_town8Buildings, g_hierarchyMask[8],
                             g_townEligibleBuildMask[8]);
}

void initializeGameData()
{
    createBuildingMasks();
    createIncludedMasks();
    town::initializeHordes();
}
