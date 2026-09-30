#ifndef HOMM3_COVE_H
#define HOMM3_COVE_H

#include "armygrp.h"
#include "town.h"

struct SMonFrameInfo;

// Cove content is appended to the original factions. Native IDs are local
// to this game and are not HotA's on-disk object IDs.
namespace cove {

enum {
    townType = TOWN_COVE,
    firstCreature = CREATURE_CANNON,
    creatureCount = 16,         // 15 recruitable creatures and the Cannon
    dwellingCount = 15,
    gunpowderWarehouse = GUNPOWDER_WAREHOUSE_ID
};

struct Creature {
    const char* m_key;
    int m_id;
    int m_upgrade;
    // Actual game layout, not a second creature-traits declaration.
    TCreatureTypeTraits m_traits;
    // Abilities beyond the shared attribute flags; their native rules live
    // in combat, movement, and town code. Names preserve upstream provenance.
    const char* m_specialAbilities;
};

struct Dwelling {
    int m_building;
    const char* m_name;
    int m_tier;                 // zero-based, as in town::m_population
    int m_stage;                // 0 = base, 1 = upgrade, 2 = Sea Dog
    int m_creature;
    int m_cost[NUM_RESOURCES];  // WOOD, MERCURY, ORE, SULFUR, CRYSTAL, GEMS, GOLD
};

const Creature* findCreature(int creature);
const Dwelling* findDwelling(int tier, int stage);
const Dwelling* findBuilding(int building);
void initializeCreatureTraits(TCreatureTypeTraits* traits);
void initializeAnimationTraits(SMonFrameInfo* traits);
TCreatureType upgradeCreature(TCreatureType creature);
TCreatureType downgradeCreature(TCreatureType creature);
void initializeBuildingMasks();
void initializeHeroTraits();
void initializeHeroClasses();
void initializeText();
bool hasLodestar();
int spellProbability(int spell);
const char* buildingName(int building);
const char* buildingDescription(int building);
int* buildingCost(int building);

// Quotes only an explicit upgrade edge. Rejections leave costs untouched.
bool quoteUpgrade(int from, int to, int count, int* costs);

} // namespace cove

#endif
