#include <va.h>
#include <stdlib.h>
#include <string.h>
#include "herodefs.h"
#include "hero.h"
#include "resourcemanager.h"
#include "textresource.h"

namespace {

// CodeView field pStr; each loader owns its own private string class.
class TAutoStrPtr {
public:
    // E:\gamedcs\herodefs.cpp:391, dc 0xd60d4
    TAutoStrPtr() : m_string(0) {}
    // E:\gamedcs\herodefs.cpp:394, dc 0xd60dc
    ~TAutoStrPtr() { delete[] m_string; }
    // E:\gamedcs\herodefs.cpp:396, dc 0xd60f4
    void set(char* value) { m_string = value; }
    // E:\gamedcs\herodefs.cpp:398, dc 0xd60f8
    char* get() const { return m_string; }

private:
    char* m_string;
};

}

static void initializeHeroTraits(int id, const TSpreadsheetResource::TStringVector& values);
static void initializeHeroClassTraits(int id, const TSpreadsheetResource::TStringVector& values);
static void initializeSSkillTraits(int id, const TSpreadsheetResource::TStringVector& values);

VA(0x004e67a0, 0x176)  // dc 0xd5a40
unsigned char initializeHeroTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x0067f154, heroTraitsSpreadsheetName,
                     "hotraits.txt"));
    if (!resource)
        return 0;

    if (resource->getNumberOfRows() < 158) {
        resource->dispose();
        return 0;
    }

    int id = 0;
    int row = 2;
    for (; id < 156; ++id, ++row) {
        initializeHeroTraits(id, resource->getRow(row));
    }

    resource->dispose();
    return 1;
}

VA(0x004e6920, 0x1E2)  // dc 0xd5ab4
bool initializeHeroClassTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x0067f164, heroClassTraitsSpreadsheetName,
                     "hctraits.txt"));
    if (!resource)
        return 0;

    if (resource->getNumberOfRows() < 20) {
        resource->dispose();
        return 0;
    }

    int id = 0;
    int row = 2;
    for (; id < 18; ++id, ++row) {
        initializeHeroClassTraits(id, resource->getRow(row));
    }

    resource->dispose();
    return 1;
}

VA(0x004e6b10, 0x1C8)  // dc 0xd5b28
bool initializeSSkillTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x0067f174, secondarySkillTraitsSpreadsheetName,
                     "sstraits.txt"));
    if (!resource)
        return 0;

    if (resource->getNumberOfRows() < 30) {
        resource->dispose();
        return 0;
    }

    int id = 0;
    int row = 2;
    for (; id < 28; ++id, ++row) {
        initializeSSkillTraits(id, resource->getRow(row));
    }

    resource->dispose();
    return 1;
}

// The DC table loaders call these ordinary static functions. Complete's
// table bodies retain the same row parsing and private string ownership,
// including the expanded static initialization and destruction families.
// Original: InitializeHeroTraits; herodefs.cpp:409, dc 0xd5bc0
static void initializeHeroTraits(int id, const TSpreadsheetResource::TStringVector& values)
{
    THeroTraits& traits = g_heroTraitsStorage[id];

    DATA_COMPGEN_GUARD(0x00698b99, heroStringsGuard, heroStrings)
    DATA(0x00698eb0)
    static TAutoStrPtr heroStrings[156];

    heroStrings[id].set(new char[strlen(values[0]) + 1]);
    strcpy(heroStrings[id].get(), values[0]);

    traits.m_defaultName = heroStrings[id].get();
    traits.m_firstStackLow = atoi(values[1]);
    traits.m_firstStackHigh = atoi(values[2]);
    traits.m_secondStackLow = atoi(values[4]);
    traits.m_secondStackHigh = atoi(values[5]);
    traits.m_thirdStackLow = atoi(values[7]);
    traits.m_thirdStackHigh = atoi(values[8]);
}

// Original: InitializeHeroClassTraits; herodefs.cpp:441, dc 0xd5d28
static void initializeHeroClassTraits(int id, const TSpreadsheetResource::TStringVector& values)
{
    THeroClassTraits& traits = g_heroClassTraits[id];

    DATA_COMPGEN_GUARD(0x00698b9a, heroClassStringsGuard,
                      heroClassStrings)
    DATA(0x00699120)
    static TAutoStrPtr heroClassStrings[18];

    heroClassStrings[id].set(new char[strlen(values[0]) + 1]);
    strcpy(heroClassStrings[id].get(), values[0]);
    traits.m_className = heroClassStrings[id].get();
    traits.m_aggression = static_cast<float>(atof(values[1]));

    int column;
    for (column = 0; column < 4; ++column)
        traits.m_initialPrimarySkill[column] =
            static_cast<signed char>(atoi(values[column + 2]));
    for (column = 0; column < 4; ++column)
        traits.m_gainPrimarySkillChance[column] =
            static_cast<signed char>(atoi(values[column + 6]));
    for (column = 0; column < 4; ++column)
        traits.m_gainPrimarySkillChance10P[column] =
            static_cast<signed char>(atoi(values[column + 10]));
    for (column = 0; column < 28; ++column)
        traits.m_gainSecondarySkillChance[column] =
            static_cast<signed char>(atoi(values[column + 14]));
    for (column = 0; column < 9; ++column)
        traits.m_foundInTownType[column] =
            static_cast<signed char>(atoi(values[column + 42]));
}

// Original: InitializeSSkillTraits; herodefs.cpp:489, dc 0xd5ee8
static void initializeSSkillTraits(int id, const TSpreadsheetResource::TStringVector& values)
{
    TSSkillTraits& traits = g_sSkillTraitsStorage[id];

    DATA_COMPGEN_GUARD(0x00698b98, secondarySkillStringsGuard,
                      secondarySkillNames)
    DATA(0x00698b28)
    static TAutoStrPtr secondarySkillNames[28];

    secondarySkillNames[id].set(new char[strlen(values[0]) + 1]);
    strcpy(secondarySkillNames[id].get(), values[0]);
    traits.m_name = secondarySkillNames[id].get();

    DATA(0x00698b9c)
    static TAutoStrPtr secondarySkillLevelNames[28][3];

    int level;
    for (level = 0; level < 3; ++level) {
        secondarySkillLevelNames[id][level].set(
            new char[strlen(values[level + 1]) + 1]);
        strcpy(secondarySkillLevelNames[id][level].get(),
               values[level + 1]);
        traits.m_levelNames[level] =
            secondarySkillLevelNames[id][level].get();
    }
}
