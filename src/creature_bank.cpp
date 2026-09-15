// creature_bank.cpp - E:\gamedcs\creature_bank.cpp (compiland creature_bank.obj)
#include <va.h>
#include <stdlib.h>
#include <string.h>
#include "creature_bank.h"
#include "creaturetype.h"
#include "game.h"
#include "misc.h"
// The traits loader reads crbanks.txt through the spreadsheet resource.
#include "resourcemanager.h"
#include "textresource.h"

// The bank traits table itself, and the pointer every consumer reads it
// through. Retail proves the split from both ends: the loader below
// addresses 0x695038 as an IMMEDIATE (a compiland-local array), while
// get_creature_bank_help_text at 0x40d3f0 reaches the same records with
// `mov edx, [0x67029c]` - a pointer LOAD. The array carries a constructor,
// so its cinit initializer at 0x47aa20 is retail's own and stays excluded.
DATA(0x00695038)
static CreatureBankTraits g_creatureBankTraits[CREATURE_BANK_COUNT];

DATA(0x0067029c)
const CreatureBankTraits* g_constCreatureBankTraits =
    g_creatureBankTraits;

// E:\gamedcs\creature_bank.cpp:25, dc 0x7152c
CreatureBankLevel::CreatureBankLevel() {}

// E:\gamedcs\creature_bank.cpp:25
VA(0x0047aad0, 0x5E)  // dc 0x714e0
CreatureBankTraits::CreatureBankTraits()
{
}

// E:\gamedcs\creature_bank.cpp:32; original initialize_creature_bank_level.
// DC proves static linkage and both reference parameters. Retail expands
// the one source call in initializeCreatureBankTraits; keep the real body.
DC_ONLY(0x70fe0, 0x14A)
static void initializeCreatureBankLevel(CreatureBankLevel& traits,
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

VA(0x0047ab30, 0x254)  // dc 0x7112c
unsigned char initializeCreatureBankTraits()
{
    SpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x006703a8, creatureBankSpreadsheetName, "crbanks.txt"));
    if (!sheet)
        return 0;
    if (sheet->getNumberOfRows() < 13) {
        sheet->dispose();
        return 0;
    }

    DATA(0x006702a0)
    static CreatureType guardTypes[CREATURE_BANK_COUNT][5] = {
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
    static CreatureType rewardTypes[CREATURE_BANK_COUNT] = {
        CREATURE_NONE, CREATURE_NONE, CREATURE_ANGEL, CREATURE_NONE,
        CREATURE_NONE, CREATURE_NONE, CREATURE_WYVERN, CREATURE_NONE,
        CREATURE_NONE, CREATURE_NONE, CREATURE_NONE
    };

    int row = 2;
    for (int bank = 0; bank < CREATURE_BANK_COUNT; ++bank) {
        CreatureBankTraits* traits = &g_creatureBankTraits[bank];
        const SpreadsheetResource::TStringVector& resource = sheet->getRow(row);
        traits->m_name = resource[0];

        CreatureBankLevel* level = traits->m_levels;
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
DC_ONLY(0x71218, 0xB8)
static void splitSlot(ArmyGroup* currentArmyGroup, long slot, long groups)
{
    long freeSlot = slot;
    long groupsLeft = groups;
    for (; groupsLeft > 1; --groupsLeft) {
        while (freeSlot < ArmyGroup::ARMY_GROUP_SLOT_COUNT
               && currentArmyGroup->m_armies[freeSlot] != -1)
            ++freeSlot;
        if (freeSlot == ArmyGroup::ARMY_GROUP_SLOT_COUNT)
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

VA(0x0047ad90, 0x36E)  // dc 0x712d0
void initializeCreatureBank(CreatureBank* bank,
                              CreatureBankType type)
{
    int roll = random(1, 100);
    int which;
    for (which = 0; which < 3; ++which) {
        roll -= g_constCreatureBankTraits[type].m_levels[which].m_chance;
        if (roll <= 0)
            break;
    }

    const CreatureBankLevel* level =
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
        CreatureType current = bank->m_guards.m_armyTypes[slot];
        if (!(g_game->m_f1f698 == 0 && isBaseElemental(current))
            && static_cast<unsigned char>(isBaseCreature(current))) {
            CreatureType promoted = bank->m_guards.m_armyTypes[slot];
            int upgraded = g_game->upgradedCreatureType(promoted);
            bank->m_guards.m_armies[slot] = upgraded;
        }
    }
}

#if 0  // @carcass -- remaining Dreamcast hypotheses

// E:\gamedcs\creature_bank.cpp:25
DC_ONLY(0x714e0, 0x34)
void CreatureBankTraits::CreatureBankTraits()
{
    // @stub
}

// E:\gamedcs\creature_bank.cpp:25
DC_ONLY(0x71514, 0x18)
void CreatureBankTraits::~CreatureBankTraits()
{
    // @stub
}

// E:\gamedcs\creature_bank.cpp:25
DC_ONLY(0x7152c, 0x1C)
void CreatureBankLevel::CreatureBankLevel()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x71548, 0x3C)
void std::vector<enum Artifact,std::allocator<enum Artifact> >::push_back(const Artifact* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x71584, 0x2C)
void std::_STL_alloc_proxy<enum Artifact *,enum Artifact,std::allocator<enum Artifact> >::deallocate(Artifact* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x715b0, 0x1C)
void std::allocator<enum Artifact>::deallocate(Artifact* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x715cc, 0xCC)
void std::vector<enum Artifact,std::allocator<enum Artifact> >::_M_insert_overflow(Artifact* __position, const Artifact* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x71698, 0x30)
void std::destroy(Artifact* __first, Artifact* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x716c8, 0x28)
void std::construct(Artifact* __p, const Artifact* __value)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x716f0, 0x4)
std::allocator<enum* std::__stl_alloc_rebind(std::allocator<enum* __a, const Artifact* __formal)
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x716f4, 0xC)
unsigned std::vector<enum Artifact,std::allocator<enum Artifact> >::size()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x71700, 0x28)
Artifact* std::_STL_alloc_proxy<enum Artifact *,enum Artifact,std::allocator<enum Artifact> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x71728, 0x24)
Artifact* std::allocator<enum Artifact>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x7174c, 0x38)
Artifact* std::uninitialized_copy(Artifact* __first, Artifact* __last, Artifact* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x71784, 0x38)
Artifact* std::uninitialized_fill_n(Artifact* __first, unsigned __n, const Artifact* __x)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x717bc, 0x4)
Artifact* std::value_type(const Artifact* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x717c0, 0x1C)
void std::__destroy(Artifact* __first, Artifact* __last, Artifact* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x717dc, 0x1C)
Artifact* std::__uninitialized_copy(Artifact* __first, Artifact* __last, Artifact* __result, Artifact* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x717f8, 0x1C)
Artifact* std::__uninitialized_fill_n(Artifact* __first, unsigned __n, const Artifact* __x, Artifact* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x71814, 0x30)
void std::__destroy_aux(Artifact* __first, Artifact* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x71844, 0x3C)
Artifact* std::__uninitialized_copy_aux(Artifact* __first, Artifact* __last, Artifact* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x71880, 0x3C)
Artifact* std::__uninitialized_fill_n_aux(Artifact* __first, unsigned __n, const Artifact* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x718bc, 0x1C)
void std::destroy(Artifact* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x718d8, 0x4)
void std::__destroy_aux()
{
    // @stub
}

#endif  // @carcass
