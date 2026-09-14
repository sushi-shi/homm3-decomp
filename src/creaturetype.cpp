// creaturetype.cpp - E:\gamedcs\creaturetype.cpp (compiland creaturetype.obj)
#include <va.h>
#include <stdlib.h>
#include <string.h>
#include "creaturetype.h"
#include "resourcemanager.h"
#include "textresource.h"
#include "town.h"

namespace {

// The 150-entry traits storage the reference cell akCreatureTypeTraits
// (armygrp.h, 0x6747b0) points at - it sits immediately before that cell,
// 150 * 116 == 0x43f8 bytes ending exactly there. Initialized by this
// compiland's parser below; the source initializer for its zero state is a
// separate admission, as it is for artifact.obj's own pair.
DATA(0x006703b8)
TCreatureTypeTraits g_creatureTypeTraitsStorage[150];

}

void initializeCreatureTypeTraits(int id,
                                  const TSpreadsheetResource::TStringVector& values);

#if 0  // @carcass

// E:\gamedcs\creaturetype.cpp:202
DC_ONLY(0x718dc, 0x20)
TCreatureType GetBaseCreature(TTownType townType, int baseCreatureNbr)
{
    // @stub
}

#endif

VA(0x0047b120, 0x5D)  // dc 0x718fc
int isBaseCreature(TCreatureType monType)
{
    const TCreatureTypeTraits& traits = g_creatureTypeTraits[monType];
    int townType = traits.m_townType;
    if (townType == -1)
        return 0;

    int creatureIndex = traits.m_level;
    if (monType != g_townDwellingCreatures[townType * 14 + creatureIndex]) {
        creatureIndex += 7;
        if (monType != g_townDwellingCreatures[townType * 14 + creatureIndex])
            return 0;
    }
    if (creatureIndex < 0 || creatureIndex >= 7)
        return 0;
    return 1;
}

VA(0x0047b180, 0x16)  // dc 0x71934
unsigned char isSiegeWeapon(TCreatureType creature)
{
    if (creature >= CREATURE_CATAPULT && creature <= CREATURE_AMMO_CART)
        return 1;
    return 0;
}

VA(0x0047b1a0, 0x71)  // dc 0x71948
TCreatureType upgradedCreatureType(TCreatureType type)
{
    const TCreatureTypeTraits& traits = g_creatureTypeTraits[type];
    int townType = traits.m_townType;
    if (townType == -1)
        return CREATURE_NONE;

    int creatureIndex = traits.m_level;
    if (type != g_townDwellingCreatures[townType * 14 + creatureIndex]) {
        creatureIndex += 7;
        if (type != g_townDwellingCreatures[townType * 14 + creatureIndex])
            return CREATURE_NONE;
    }
    if (creatureIndex < 0 || creatureIndex >= 7)
        return CREATURE_NONE;
    return g_townDwellingCreatures[
        g_creatureTypeTraits[type].m_townType * 14 + creatureIndex + 7];
}

VA(0x0047B220, 0x6D)
TCreatureType downgradedCreatureType(TCreatureType type)
{
    do {
        const TCreatureTypeTraits& traits = g_creatureTypeTraits[type];
        int townType = traits.m_townType;
        int creatureIndex;
        if (townType == -1)
            break;

        creatureIndex = traits.m_level;
        if (type != g_townDwellingCreatures[townType * 14 + creatureIndex]) {
            creatureIndex += 7;
            if (type != g_townDwellingCreatures[townType * 14 + creatureIndex])
                break;
        }
        if (creatureIndex < 7)
            break;
        return g_townDwellingCreatures[
            g_creatureTypeTraits[type].m_townType * 14 + creatureIndex - 7];
    } while (0);
    return CREATURE_NONE;
}

VA(0x0047b290, 0x1E9)  // dc 0x71968
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
    // E:\gamedcs\creaturetype.cpp:399, dc 0x71eec
    TAutoStrPtr() : m_string(0) {}
    // E:\gamedcs\creaturetype.cpp:402, dc 0x71ef4
    ~TAutoStrPtr() { delete[] m_string; }
    // E:\gamedcs\creaturetype.cpp:404, dc 0x71f0c
    void set(char* value) { m_string = value; }
    // E:\gamedcs\creaturetype.cpp:406, dc 0x71f10
    char* get() const { return m_string; }

private:
    char* m_string;
};

}

// neighbouring column takes a dword.
VA(0x0047b480, 0x322)  // dc 0x71b40
void initializeCreatureTypeTraits(int id,
                                  const TSpreadsheetResource::TStringVector& values)
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

#if 0  // @carcass

// E:\gamedcs\creaturetype.cpp:399
DC_ONLY(0x71eec, 0x8)
void `anonymous namespace'::TAutoStrPtr::TAutoStrPtr()
{
    // @stub
}

// E:\gamedcs\creaturetype.cpp:402
DC_ONLY(0x71ef4, 0x18)
void `anonymous namespace'::TAutoStrPtr::~TAutoStrPtr()
{
    // @stub
}

// E:\gamedcs\creaturetype.cpp:404
DC_ONLY(0x71f0c, 0x4)
void `anonymous namespace'::TAutoStrPtr::set(char* pStr)
{
    // @stub
}

// E:\gamedcs\creaturetype.cpp:406
DC_ONLY(0x71f10, 0x4)
char* `anonymous namespace'::TAutoStrPtr::get()
{
    // @stub
}

#endif
