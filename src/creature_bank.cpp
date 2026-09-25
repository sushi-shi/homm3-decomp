#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "creature_bank.h"

#include "creaturetype.h"
#include "game.h"
#include "misc.h"
#include "resourcemanager.h"
#include "textresource.h"

// The bank traits table itself, and the pointer every consumer reads it
// through. Retail proves the split from both ends: the loader below
// addresses 0x695038 as an IMMEDIATE (a compiland-local array), while
// get_creature_bank_help_text at 0x40d3f0 reaches the same records with
// `mov edx, [0x67029c]` - a pointer LOAD. The array carries a constructor,
// so its cinit initializer at 0x47aa20 is retail's own and stays excluded.
DATA(0x00695038)
static type_creature_bank_traits g_creatureBankTraits[CREATURE_BANK_COUNT];

DATA(0x0067029c)
const type_creature_bank_traits* g_constCreatureBankTraits =
    g_creatureBankTraits;

// E:\gamedcs\creature_bank.cpp:25, dc 0x7152c
type_creature_bank_level::type_creature_bank_level() {}

// E:\gamedcs\creature_bank.cpp:25
VA(0x0047aad0, 0x5E) MAC_ADDRESS(0x089f90, 0x54)  // dc 0x714e0
type_creature_bank_traits::type_creature_bank_traits()
{
}

// E:\gamedcs\creature_bank.cpp:32; original initialize_creature_bank_level.
// DC proves static linkage and both reference parameters. Retail expands
// the one source call in initializeCreatureBankTraits; keep the real body.

MAC_ADDRESS(0x0893bc, 0x19c)
static void initializeCreatureBankLevel(type_creature_bank_level& traits,
                                       const std::vector<char*>& resource)
{
    int column = 2;
    traits.m_chance = atoi(resource[column++]);
    traits.m_guards.m_numTroops[0] = atoi(resource[column]);
    column += 2;
    traits.m_upgradeChance = atoi(resource[column++]);
    for (int guard = 1; guard < 4; ++guard) {
        traits.m_guards.m_numTroops[guard] = atoi(resource[column]);
        column += 2;
        if (traits.m_guards.m_numTroops[guard] == 0)
            traits.m_guards.m_armyTypes[guard] = CREATURE_NONE;
    }

    ++column;
    for (int resourceId = 0; resourceId < 7; ++resourceId)
        traits.m_resources[resourceId] = atoi(resource[column++]);

    traits.m_rewardCreatures = atoi(resource[column]);
    column += 2;
    if (traits.m_rewardCreatures == 0)
        traits.m_rewardCreature = CREATURE_NONE;

    traits.m_treasureArtifacts = atoi(resource[column++]);
    traits.m_minorArtifacts = atoi(resource[column++]);
    traits.m_majorArtifacts = atoi(resource[column]);
    traits.m_relicArtifacts = atoi(resource[column + 1]);
}

VA(0x0047ab30, 0x254) MAC_ADDRESS(0x089558, 0x1ac)  // dc 0x7112c
unsigned char initializeCreatureBankTraits()
{
    TSpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x006703a8, creatureBankSpreadsheetName, "crbanks.txt"));
    if (!sheet)
        return 0;
    // Complete and Mac dispose directly; Dreamcast called ResourceManager::Dispose.
    if (sheet->getNumberOfRows() < 13) {
        sheet->dispose();
        return 0;
    }

    DATA(0x006702a0)
    static TCreatureType guardTypes[CREATURE_BANK_COUNT][5] = {
        { CREATURE_CYCLOPS, CREATURE_NONE },
        { CREATURE_DWARF, CREATURE_NONE },
        { CREATURE_GRIFFIN, CREATURE_NONE },
        { CREATURE_IMP, CREATURE_NONE },
        { CREATURE_MEDUSA, CREATURE_NONE },
        { CREATURE_NAGA_SENTINEL, CREATURE_NONE },
        { CREATURE_DRAGON_FLY, CREATURE_NONE },
        { CREATURE_WIGHT, CREATURE_NONE },
        { CREATURE_WATER_ELEMENTAL, CREATURE_NONE },
        { CREATURE_SKELETON, CREATURE_WALKING_DEAD,
          CREATURE_WIGHT, CREATURE_VAMPIRE, CREATURE_NONE },
        { CREATURE_GREEN_DRAGON, CREATURE_RED_DRAGON,
          CREATURE_GOLD_DRAGON, CREATURE_BLACK_DRAGON, CREATURE_NONE }
    };
    DATA(0x0067037c)
    static TCreatureType rewardTypes[CREATURE_BANK_COUNT] = {
        CREATURE_NONE, CREATURE_NONE, CREATURE_ANGEL, CREATURE_NONE,
        CREATURE_NONE, CREATURE_NONE, CREATURE_WYVERN, CREATURE_NONE,
        CREATURE_NONE, CREATURE_NONE, CREATURE_NONE
    };

    int row = 2;
    for (int bank = 0; bank < CREATURE_BANK_COUNT; ++bank) {
        type_creature_bank_traits* traits = &g_creatureBankTraits[bank];
        const TSpreadsheetResource::TStringVector& resource = sheet->getRow(row);
        traits->m_name = resource[0];

        type_creature_bank_level* level = traits->m_levels;
        int levelsLeft = 4;
        do {
            level->m_guards.initialize();
            for (int slot = 0; slot < 5; ++slot) {
                if (guardTypes[bank][slot] == CREATURE_NONE)
                    break;
                level->m_guards.m_armyTypes[slot] = guardTypes[bank][slot];
            }
            level->m_rewardCreature = rewardTypes[bank];

            initializeCreatureBankLevel(*level, sheet->getRow(row));

            ++row;
            ++level;
        } while (--levelsLeft);

    }

    sheet->dispose();
    return 1;
}

// E:\gamedcs\creature_bank.cpp:146, dc 0x71218. Retail expands this file
// static at all five of initialize_creature_bank's call sites, so no
// out-of-line row survives. The free-slot cursor starts AT the slot being
// split and is carried across the whole run - every site's scan begins at
// its own `slot` argument, which is what fixes the parameter's second role.

MAC_ADDRESS(0x089704, 0x9c)
static void splitSlot(armyGroup* currentArmyGroup, long slot, long groups)
{
    long freeSlot = slot;
    long groupsLeft = groups;
    for (; groupsLeft > 1; --groupsLeft) {
        while (freeSlot < armyGroup::ARMY_GROUP_SLOT_COUNT
               && currentArmyGroup->m_armies[freeSlot] != -1)
            ++freeSlot;
        if (freeSlot == armyGroup::ARMY_GROUP_SLOT_COUNT)
            break;
        long amount = currentArmyGroup->m_numTroops[slot] / groupsLeft;
        currentArmyGroup->m_numTroops[slot] -= amount;
        currentArmyGroup->add(currentArmyGroup->m_armies[slot], amount, freeSlot);
    }
}

// One roll picks the level: the first three weights are walked down until
// the running roll goes non-positive, and falling off the end selects the
// fourth. The chosen record's guards, resource row, reward creature and
// reward count are copied straight into the bank, then four artifact loops
// draw from the relic, major, minor and treasure classes in that order.

// The guard split is a three-case switch on the army count, and its arms
// also decide which slot the upgrade roll may promote: one stack becomes
// five groups and slot 2 is the candidate, two become 2+3 with slot 3, and
// three become 2+2 with slot 0.

VA(0x0047ad90, 0x36E) MAC_ADDRESS(0x0897a0, 0x348)  // dc 0x712d0
void initializeCreatureBank(type_creature_bank* bank,
                              type_creature_bank_type type)
{
    int roll = random(1, 100);
    int which;
    for (which = 0; which < 3; ++which) {
        roll -= g_constCreatureBankTraits[type].m_levels[which].m_chance;
        if (roll <= 0)
            break;
    }

    const type_creature_bank_level* level =
        &g_constCreatureBankTraits[type].m_levels[which];
    bank->m_guards = level->m_guards;
    memcpy(bank->m_resources, level->m_resources, sizeof(bank->m_resources));
    bank->m_rewardCreature = level->m_rewardCreature;
    bank->m_rewardCreatures = level->m_rewardCreatures;

    int drawn;
    for (drawn = 0; drawn < level->m_relicArtifacts; ++drawn)
        bank->m_artifacts.push_back(g_game->getRandomArtifactId(16));
    for (drawn = 0; drawn < level->m_majorArtifacts; ++drawn)
        bank->m_artifacts.push_back(g_game->getRandomArtifactId(8));
    for (drawn = 0; drawn < level->m_minorArtifacts; ++drawn)
        bank->m_artifacts.push_back(g_game->getRandomArtifactId(4));
    for (drawn = 0; drawn < level->m_treasureArtifacts; ++drawn)
        bank->m_artifacts.push_back(g_game->getRandomArtifactId(2));

    long slot = 0;
    switch (bank->m_guards.getNumArmies()) {
    case CREATURE_BANK_GUARDS_ONE_STACK:
        splitSlot(&bank->m_guards, 0, 5);
        slot = 2;
        break;
    case CREATURE_BANK_GUARDS_TWO_STACKS:
        splitSlot(&bank->m_guards, 1, 2);
        splitSlot(&bank->m_guards, 0, 3);
        slot = 3;
        break;
    case CREATURE_BANK_GUARDS_THREE_STACKS:
        splitSlot(&bank->m_guards, 1, 2);
        splitSlot(&bank->m_guards, 0, 2);
        slot = 0;
        break;
    }

    if (random(1, 100) <= level->m_upgradeChance) {
        TCreatureType current = bank->m_guards.m_armyTypes[slot];
        if (!(g_game->m_gameVersion == 0 && isBaseElemental(current))
            && static_cast<unsigned char>(isBaseCreature(current))) {
            TCreatureType promoted = bank->m_guards.m_armyTypes[slot];
            int upgraded = g_game->upgradedCreatureType(promoted);
            bank->m_guards.m_armies[slot] = upgraded;
        }
    }
}
