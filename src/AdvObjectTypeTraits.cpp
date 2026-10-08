// AdvObjectTypeTraits.cpp of the Loki port (Loki object 3): the RoE
// adventure-object type traits and their names from objnames.txt.
#include <assert.h>
#include <string.h>

#include "objnames.h"

#include "autoarrayptr.h"
#include "exceptions.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "textresource.h"

namespace {

// The three placement flags are compiled in; the names are loaded.
TAdvObjectTypeTraits aAdvObjectTypeTraitsImp[MAX_EVENT_TYPE] = {
    { false, false, false },  // NOTHING
    { false, false, false },  // 1
    { false, false, false },  // ALTAR_OF_SACRIFICE
    { true, true, false },  // ANCHOR_POINT
    { false, false, false },  // ARENA
    { true, true, true },  // ARTIFACT
    { true, true, true },  // BLACK_BOX
    { false, false, false },  // BLACK_MARKET
    { true, true, true },  // BOAT
    { true, true, true },  // BORDER_GUARD
    { false, false, false },  // BORDER_TENT
    { true, true, false },  // BUOY
    { true, true, true },  // CAMPFIRE
    { false, false, false },  // CARTOGRAPHER
    { false, false, false },  // CLOVER_FIELD
    { false, false, false },  // COVER_OF_DARKNESS
    { false, false, false },  // CREATURE_BANK
    { false, false, false },  // CREATURE_GENERATOR_1
    { false, false, false },  // 18
    { false, false, false },  // 19
    { false, false, false },  // CREATURE_GENERATOR_4
    { false, false, false },  // CURSED_GROUND
    { true, true, false },  // DEAD_GUY
    { false, false, false },  // DEFENSE_TOWER
    { false, false, false },  // DERELICT_SHIP
    { false, false, false },  // DRAGON_CITY
    { true, true, true },  // EVENT
    { false, false, false },  // EYE_OF_MAGI
    { false, false, false },  // FAERIE_RING
    { true, true, true },  // FLOTSAM
    { false, false, false },  // FOUNTAIN_OF_FORTUNE
    { false, false, false },  // FOUNTAIN_OF_YOUTH
    { false, false, false },  // GARDEN_OF_REVELATION
    { false, true, false },  // GARRISON
    { true, true, true },  // HERO
    { false, false, false },  // HILL_FORT
    { true, true, true },  // HOLY_GRAIL
    { false, false, false },  // HUT_OF_MAGI
    { false, false, false },  // IDOL_OF_FORTUNE
    { false, false, false },  // LEAN_TO
    { false, false, false },  // 40
    { false, false, false },  // LIBRARY
    { false, false, false },  // LIGHTHOUSE
    { false, false, false },  // LITH_ONEWAY_ENTRANCE
    { false, false, false },  // LITH_ONEWAY_EXIT
    { false, false, false },  // LITH_TWOWAY
    { false, false, false },  // MAGIC_PLAINS
    { false, false, false },  // MAGIC_SCHOOL
    { false, false, false },  // MAGIC_SPRING
    { false, false, false },  // MAGIC_WELL
    { false, false, false },  // MARKET_OF_TIME
    { false, false, false },  // MERC_CAMP
    { true, false, false },  // MERMAID
    { false, false, false },  // MINE
    { true, true, true },  // MONSTER
    { false, false, false },  // MYSTICAL_GARDEN
    { false, false, false },  // OASIS
    { false, false, false },  // OBELISK
    { false, false, false },  // OBSERVATORY
    { true, true, false },  // OCEAN_BOTTLE
    { false, false, false },  // PILLAR_OF_FIRE
    { false, false, false },  // POWER_SCHOOL
    { true, false, false },  // PRISON
    { false, false, false },  // PYRAMID
    { false, false, false },  // RALLY_FLAG
    { true, true, true },  // RANDOM_ARTIFACT
    { true, true, true },  // RANDOM_ARTIFACT_1
    { true, true, true },  // RANDOM_ARTIFACT_2
    { true, true, true },  // RANDOM_ARTIFACT_3
    { true, true, true },  // RANDOM_ARTIFACT_4
    { true, true, true },  // RANDOM_HERO
    { true, true, true },  // RANDOM_MONSTER
    { true, true, true },  // RANDOM_MONSTER_1
    { true, true, true },  // RANDOM_MONSTER_2
    { true, true, true },  // RANDOM_MONSTER_3
    { true, true, true },  // RANDOM_MONSTER_4
    { true, true, true },  // RANDOM_RESOURCE
    { true, true, true },  // RANDOM_TOWN
    { false, false, false },  // REFUGEE_CAMP
    { true, true, true },  // RESOURCE
    { false, false, false },  // SANCTUARY
    { true, true, true },  // SCHOLAR
    { true, true, true },  // SEA_CHEST
    { false, false, false },  // SEER
    { false, false, false },  // SEPULCHER
    { true, true, false },  // SHIPWRECK
    { true, true, true },  // SHIPWRECK_SURVIVOR
    { false, false, false },  // SHIPYARD
    { false, false, false },  // SHRINE1
    { false, false, false },  // SHRINE2
    { false, false, false },  // SHRINE3
    { false, false, false },  // SIGN
    { true, false, false },  // SIREN
    { true, true, true },  // SPELL_SCROLL
    { false, false, false },  // STABLES
    { true, false, false },  // TAVERN
    { false, false, false },  // TEMPLE
    { false, false, false },  // THIEVES_DEN
    { false, false, false },  // TOWN
    { false, false, false },  // TRADING_POST
    { false, false, false },  // TRAINING_GROUNDS
    { true, true, true },  // TREASURE_CHEST
    { false, false, false },  // TREE_OF_KNOWLEDGE
    { false, false, false },  // UNDERGROUND_GATE
    { false, false, false },  // UNIVERSITY
    { false, false, false },  // WAGON
    { false, false, false },  // WAR_MACHINE_FACTORY
    { false, false, false },  // WAR_SCHOOL
    { false, false, false },  // WARRIOR_TOMB
    { false, false, false },  // WATER_WHEEL
    { false, false, false },  // WATERING_HOLE
    { false, true, false },  // WHIRLPOOL
    { false, false, false },  // WINDMILL
    { false, false, false },  // WITCH_HUT
    { false, false, false },  // TERRAIN_BRUSH
    { false, false, false },  // TERRAIN_BUSH
    { false, false, false },  // TERRAIN_CACTUS
    { false, false, false },  // TERRAIN_CANYON
    { false, false, false },  // TERRAIN_CRATER
    { false, false, false },  // TERRAIN_DEAD_VEGETATION
    { false, false, false },  // TERRAIN_FLOWER
    { false, false, false },  // TERRAIN_FROZEN_LAKE
    { false, false, false },  // TERRAIN_HEDGE
    { false, false, false },  // TERRAIN_HILL
    { false, false, false },  // TERRAIN_HOLE
    { false, false, false },  // TERRAIN_KELP
    { false, false, false },  // TERRAIN_LAKE
    { false, false, false },  // TERRAIN_LAVA_FLOW
    { false, false, false },  // TERRAIN_LAVA_LAKE
    { false, false, false },  // TERRAIN_MUSHROOM
    { false, false, false },  // TERRAIN_LOG
    { false, false, false },  // TERRAIN_MANDRAKE
    { false, false, false },  // TERRAIN_MOSS
    { false, false, false },  // TERRAIN_MOUND
    { false, false, false },  // TERRAIN_MOUNTAIN
    { false, false, false },  // TERRAIN_OAK_TREE
    { false, false, false },  // TERRAIN_OUTCROPPING
    { false, false, false },  // TERRAIN_PINE_TREE
    { false, false, false },  // TERRAIN_PLANT
    { false, false, false },  // TERRAIN_RIVER_1
    { false, false, false },  // TERRAIN_RIVER_2
    { false, false, false },  // TERRAIN_RIVER_3
    { false, false, false },  // TERRAIN_RIVER_4
    { false, false, false },  // TERRAIN_RIVER_DELTA
    { false, false, false },  // TERRAIN_ROAD_1
    { false, false, false },  // TERRAIN_ROAD_2
    { false, false, false },  // TERRAIN_ROAD_3
    { false, false, false },  // TERRAIN_ROCK
    { false, false, false },  // TERRAIN_SAND_DUNE
    { false, false, false },  // TERRAIN_SAND_PIT
    { false, false, false },  // TERRAIN_SHRUB
    { false, false, false },  // TERRAIN_SKULL
    { false, false, false },  // TERRAIN_STALAGMITE
    { false, false, false },  // TERRAIN_STUMP
    { false, false, false },  // TERRAIN_TAR_PIT
    { false, false, false },  // TERRAIN_TREE
    { false, false, false },  // TERRAIN_VINE
    { false, false, false },  // TERRAIN_VOLCANIC_VENT
    { false, false, false },  // TERRAIN_VOLCANO
    { false, false, false },  // TERRAIN_WILLOW_TREE
    { false, false, false },  // TERRAIN_YUCCA_TREE
    { false, false, false },  // TERRAIN_REEF
    { true, true, true },  // RANDOM_MONSTER_5
    { true, true, true },  // RANDOM_MONSTER_6
    { true, true, true },  // RANDOM_MONSTER_7
};

}

const TAdvObjectTypeTraits* akAdvObjectTypeTraits = aAdvObjectTypeTraitsImp;

void InitializeAdvObjectTypeTraitsTable()
{
    static TAutoArrayPtr<char> aNameBuffer(0);

#line 211
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("objnames.txt"));
    if (!pTextResource.get())
        throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"objnames.txt\".");
    assert(pTextResource->GetNumberOfStrings() >= MAX_EVENT_TYPE);

    unsigned int size = 0;
    unsigned int i;
    for (i = 0; i < MAX_EVENT_TYPE; i++)
        size += strlen(pTextResource->GetText(i)) + 1;

    aNameBuffer = TAutoArrayPtr<char>(new char[size]);
#line 224
    if (!aNameBuffer.get())
        throw TAllocationFailure(__FILE__, __LINE__);

    char* name = aNameBuffer.get();
    for (i = 0; i < MAX_EVENT_TYPE; i++) {
        const char* text = pTextResource->GetText(i);
        unsigned int length = strlen(text) + 1;
        memcpy(name, text, length);
        aAdvObjectTypeTraitsImp[i].m_name = name;
        name += length;
    }
}
