#include "cove.h"
#include "monframeinfo.h"
#include "hero.h"
#include "herospec.h"
#include "text.h"
#include "mapcell.h"
#include "game.h"

#include <limits.h>

extern THeroSpecificAbility g_heroSpecificAbilitiesImp[HOMM3_HERO_COUNT];

// Cove data adapted from the VCMI HotA port. HotA Crew and VCMI contributors;
// CC BY-SA 4.0. See extensions/cove/definition.json for the pinned sources.
namespace cove {
namespace {
#include "cove_data.inc"
}

bool hasLodestar()
{
    for (unsigned i = 0; i < g_game->m_towns.size(); ++i) {
        const town& settlement = g_game->m_towns[i];
        if (settlement.m_type == TOWN_COVE && settlement.hasBuilding(HOLY_GRAIL_ID, true))
            return true;
    }
    return false;
}

void initializeCreatureTraits(TCreatureTypeTraits* traits)
{
    for (int i = 0; i < creatureCount; ++i)
        traits[g_creatures[i].m_id] = g_creatures[i].m_traits;
}

int spellProbability(int spell)
{
    return spell >= 0 && spell < 70 ? g_coveSpellWeights[spell] : 0;
}

void initializeHeroTraits()
{
    for (int portrait = 156; portrait < 163; ++portrait) {
        g_heroTraitsStorage[portrait].m_defaultName = "Reserved";
        g_heroTraitsStorage[portrait].m_attributes = 65536;
    }
    for (int i = 0; i < 20; ++i) {
        g_heroTraitsStorage[163 + i] = g_coveHeroes[i];
        g_heroSpecificAbilitiesImp[163 + i] = g_coveSpecialties[i];
    }
}

void initializeHeroClasses()
{
    g_heroClassTraits[classCaptain] = g_coveClasses[0];
    g_heroClassTraits[classNavigator] = g_coveClasses[1];
    for (int i = 0; i < classCaptain; ++i)
        g_heroClassTraits[i].m_foundInTownType[TOWN_COVE] = 5;
}

void initializeText()
{
    for (int i = 0; i < 20; ++i)
        g_heroBio[163 + i] = g_coveBiographies[i];
    for (int j = 0; j < 16; ++j)
        g_townNames[TOWN_COVE][j] = g_coveTownNames[j];
}

void initializeAnimationTraits(SMonFrameInfo* traits)
{
    for (int i = 0; i < creatureCount; ++i)
        traits[firstCreature + i] = g_animationTraits[i];
}

TCreatureType upgradeCreature(TCreatureType creature)
{
    const Creature* definition = findCreature(creature);
    return definition ? TCreatureType(definition->m_upgrade) : CREATURE_NONE;
}

TCreatureType downgradeCreature(TCreatureType creature)
{
    if (!findCreature(creature))
        return CREATURE_NONE;
    for (int i = 0; i < creatureCount; ++i) {
        if (g_creatures[i].m_upgrade == creature)
            return TCreatureType(g_creatures[i].m_id);
    }
    return CREATURE_NONE;
}

const char* buildingName(int building)
{
    for (unsigned i = 0; i < sizeof(g_buildings) / sizeof(g_buildings[0]); ++i)
        if (g_buildings[i].m_id == building)
            return g_buildings[i].m_name;
    return "";
}

const char* buildingDescription(int building)
{
    for (unsigned i = 0; i < sizeof(g_buildings) / sizeof(g_buildings[0]); ++i)
        if (g_buildings[i].m_id == building)
            return g_buildings[i].m_description;
    return "";
}

int* buildingCost(int building)
{
    for (unsigned i = 0; i < sizeof(g_buildings) / sizeof(g_buildings[0]); ++i)
        if (g_buildings[i].m_id == building)
            return g_buildings[i].m_cost;
    return 0;
}

void initializeBuildingMasks()
{
    __int64* requirements = g_hierarchyMask[TOWN_COVE];
    __int64* included = town::s_includedBuildings[TOWN_COVE];
    __int64& legal = g_townEligibleBuildMask[TOWN_COVE];
    legal = 0;
    int building;
    for (building = 0; building < TOWN_BUILDING_SLOTS; ++building) {
        requirements[building] = 0;
        included[building] = 0;
        if (building < SPECIAL_BUILDING_ID) {
            requirements[building] = g_hierarchyMask[TOWN_CASTLE][building];
            included[building] = town::s_includedBuildings[TOWN_CASTLE][building];
        }
    }
    for (unsigned i = 0; i < sizeof(g_buildings) / sizeof(g_buildings[0]); ++i) {
        Building& definition = g_buildings[i];
        building = definition.m_id;
        legal |= (__int64(1) << building);
        requirements[building] |= definition.m_requires;
        if (building == DWELLING_0_ID)
            requirements[building] |= (__int64(1) << CASTLE_FORT_ID);
        if (definition.m_upgrade >= 0)
            included[building] |= (__int64(1) << definition.m_upgrade);
    }
    // Topological closure also handles a third dwelling upgrade. It does
    // not depend on the ordering of objects in the imported JSON source.
    for (int pass = 0; pass < TOWN_BUILDING_SLOTS; ++pass) {
        for (building = 0; building < TOWN_BUILDING_SLOTS; ++building) {
            for (int ancestor = 0; ancestor < TOWN_BUILDING_SLOTS; ++ancestor) {
                if (requirements[building] & (__int64(1) << ancestor))
                    requirements[building] |= requirements[ancestor];
                if (included[building] & (__int64(1) << ancestor))
                    included[building] |= included[ancestor];
            }
        }
    }
}

const Creature* findCreature(int creature)
{
    if (creature < firstCreature || creature >= firstCreature + creatureCount)
        return 0;
    return &g_creatures[creature - firstCreature];
}

const Dwelling* findDwelling(int tier, int stage)
{
    for (int i = 0; i < dwellingCount; ++i) {
        if (g_dwellings[i].m_tier == tier && g_dwellings[i].m_stage == stage)
            return &g_dwellings[i];
    }
    return 0;
}

const Dwelling* findBuilding(int building)
{
    for (int i = 0; i < dwellingCount; ++i) {
        if (g_dwellings[i].m_building == building)
            return &g_dwellings[i];
    }
    return 0;
}

bool quoteUpgrade(int from, int to, int count, int* costs)
{
    const Creature* base = findCreature(from);
    const Creature* upgrade = findCreature(to);
    if (!base || !upgrade || base->m_upgrade != to || count < 0 || !costs)
        return false;

    int quoted[NUM_RESOURCES];
    for (int resource = 0; resource < NUM_RESOURCES; ++resource) {
        int price = upgrade->m_traits.m_cost[resource]
                  - base->m_traits.m_cost[resource];
        if (price < 0)
            price = 0;
        if (count && price > INT_MAX / count)
            return false;
        quoted[resource] = price * count;
    }
    for (int i = 0; i < NUM_RESOURCES; ++i)
        costs[i] = quoted[i];
    return true;
}

} // namespace cove
