#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "creaturetype.h"

#include "resourcemanager.h"
#include "textresource.h"
#include "town.h"

namespace {

// Original akCreatureTypeTraits references these 150 writable rows. Retail
// initializers retain sprite/sample names and flags before crtraits.txt loads.
DATA(0x006703b8)
TCreatureTypeTraits g_creatureTypeTraitsStorage[150] =
#include "rmg_data/creature_traits.inc"
;

}

DATA(0x006747b0)
const TCreatureTypeTraits (&g_creatureTypeTraits)[150] = g_creatureTypeTraitsStorage;

void initializeCreatureTypeTraits(int id,
    const std::vector<char*, std::allocator<char*> >& values);

// Provisional name: Mac retains this source helper at 0:0x888d0, immediately
// before isBaseCreature. All three following queries call it there; VC6
// expands its two-row lookup in their Windows bodies.
MAC_ADDRESS(0x0888d0, 0x70)
static int getCreatureDwellingIndex(TCreatureType type)
{
    const TCreatureTypeTraits& traits = g_creatureTypeTraits[type];
    int townType = traits.m_townType;
    if (townType == -1)
        return -1;

    int creatureIndex = traits.m_level;
    if (type == g_townDwellingCreatures[townType * 14 + creatureIndex])
        return creatureIndex;
    creatureIndex += 7;
    if (type == g_townDwellingCreatures[townType * 14 + creatureIndex])
        return creatureIndex;
    return -1;
}

VA(0x0047b120, 0x5D)
DC_ADDRESS(0x0718fc, 0x36)
MAC_ADDRESS(0x088940, 0x3c)
int isBaseCreature(TCreatureType monType)
{
    int creatureIndex = getCreatureDwellingIndex(monType);
    return creatureIndex >= 0 && creatureIndex < 7;
}

VA(0x0047b180, 0x16)
DC_ADDRESS(0x071934, 0x12)
MAC_ADDRESS(0x08897c, 0x24)
unsigned char isSiegeWeapon(TCreatureType creature)
{
    if (creature >= CREATURE_CATAPULT && creature <= CREATURE_AMMO_CART)
        return 1;
    return 0;
}

VA(0x0047b1a0, 0x71)
DC_ADDRESS(0x071948, 0x20)
MAC_ADDRESS(0x0889a0, 0x6c)
TCreatureType upgradedCreatureType(TCreatureType type)
{
    int creatureIndex = getCreatureDwellingIndex(type);
    if (creatureIndex < 0 || creatureIndex >= 7)
        return CREATURE_NONE;
    return g_townDwellingCreatures[
        g_creatureTypeTraits[type].m_townType * 14 + creatureIndex + 7];
}

VA(0x0047B220, 0x6D)
MAC_ADDRESS(0x088a0c, 0x64)
TCreatureType downgradedCreatureType(TCreatureType type)
{
    int creatureIndex = getCreatureDwellingIndex(type);
    if (creatureIndex < 7)
        return CREATURE_NONE;
    return g_townDwellingCreatures[
        g_creatureTypeTraits[type].m_townType * 14 + creatureIndex - 7];
}

VA(0x0047b290, 0x1E9)
DC_ADDRESS(0x071968, 0x1d8)
MAC_ADDRESS(0x088a70, 0x37c)
unsigned char initializeCreatureTypeTraitsTable()
{
    TSpreadsheetResource* traitsSheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00675514, creatureTraitsSpreadsheetName,
                     "crtraits.txt"));
    if (!traitsSheet)
        return 0;
    if (traitsSheet->getNumberOfRows() < 179) {
        traitsSheet->dispose();
        return 0;
    }

    int id = 0;
    int row = 2;
    for (; id < 14; id++, row++)
        initializeCreatureTypeTraits(id, traitsSheet->getRow(row));
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->getRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->getRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->getRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->getRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->getRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->getRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->getRow(row));
    }
    row += 3;
    { for (int i = 0; i < 6; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->getRow(row));
    }
    row += 3;
    { for (int i = 0; i < 14; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->getRow(row));
    }
    row += 3;
    { for (int i = 0; i < 13; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->getRow(row));
    }
    row += 3;
    { for (int i = 0; i < 5; i++, id++, row++)
            initializeCreatureTypeTraits(id, traitsSheet->getRow(row));
    }
    traitsSheet->dispose();
    return 1;
}

namespace {

// CodeView field pStr; each loader owns its own private string class.
class TAutoStrPtr {
public:

    // E:\gamedcs\creaturetype.cpp:399
    DC_ADDRESS(0x071eec, 0x8)
    TAutoStrPtr() : m_string(0) {}

    // E:\gamedcs\creaturetype.cpp:402
    // Retail keeps this COMDAT at 0x47b7b0, directly after
    // initializeCreatureTypeTraits; identical private copies fold to it.
    VA(0x0047b7b0, 0xA)
    DC_ADDRESS(0x071ef4, 0x18)
    ~TAutoStrPtr() { delete[] m_string; }

    // E:\gamedcs\creaturetype.cpp:404
    DC_ADDRESS(0x071f0c, 0x4)
    void set(char* value) { m_string = value; }

    // E:\gamedcs\creaturetype.cpp:406
    DC_ADDRESS(0x071f10, 0x4)
    char* get() const { return m_string; }

private:
    char* m_string;
};

}

// neighbouring column takes a dword.
VA(0x0047b480, 0x322)
DC_ADDRESS(0x071b40, 0x350)
MAC_ADDRESS(0x088dec, 0x320)
void initializeCreatureTypeTraits(int id,
    const std::vector<char*, std::allocator<char*> >& values)
{
    TCreatureTypeTraits& traits = g_creatureTypeTraitsStorage[id];

    DATA_COMPGEN_GUARD(0x00696640, creatureTypeStringsGuard,
                       creatureTypeNames)
    DATA(0x006963e8)
    static TAutoStrPtr creatureTypeNames[150];

    creatureTypeNames[id].set(new char[strlen(values[0]) + 1]);
    strcpy(creatureTypeNames[id].get(), values[0]);
    traits.m_name = creatureTypeNames[id].get();

    DATA(0x00696644)
    static TAutoStrPtr creatureTypePluralNames[150];

    creatureTypePluralNames[id].set(new char[strlen(values[1]) + 1]);
    strcpy(creatureTypePluralNames[id].get(), values[1]);
    traits.m_pluralName = creatureTypePluralNames[id].get();

    traits.m_cost[0] = atoi(values[2]);
    traits.m_cost[1] = atoi(values[3]);
    traits.m_cost[2] = atoi(values[4]);
    traits.m_cost[3] = atoi(values[5]);
    traits.m_cost[4] = atoi(values[6]);
    traits.m_cost[5] = atoi(values[7]);
    traits.m_cost[6] = atoi(values[8]);
    traits.m_baseFightValue = atoi(values[9]);
    traits.m_aiValue = atoi(values[10]);
    traits.m_growthRate = atoi(values[11]);
    traits.m_hordeGrowthRate = atoi(values[12]);
    traits.m_hitPoints = atoi(values[13]);
    traits.m_speed = atoi(values[14]);
    traits.m_attackSkill = atoi(values[15]);
    traits.m_defenseSkill = atoi(values[16]);
    traits.m_damageLowBound = atoi(values[17]);
    traits.m_damageHighBound = atoi(values[18]);
    traits.m_numShots = atoi(values[19]);
    traits.m_hasSpell = atoi(values[20]);
    traits.m_wanderingLow = atoi(values[21]);
    traits.m_wanderingHigh = atoi(values[22]);

    DATA(0x00696190)
    static TAutoStrPtr creatureTypeAbilities[150];

    creatureTypeAbilities[id].set(new char[strlen(values[23]) + 1]);
    strcpy(creatureTypeAbilities[id].get(), values[23]);
    traits.m_specialAbility = creatureTypeAbilities[id].get();
}
