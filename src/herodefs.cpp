// herodefs.cpp - E:\gamedcs\herodefs.cpp (compiland herodefs.obj)
#include <va.h>
#include <stdlib.h>
#include <string.h>
#include "herodefs.h"
#include "hero.h"
#include "resourcemanager.h"
#include "textresource.h"

namespace {

// CodeView field pStr; each loader owns its own private string class.
// Before normalization (type): TAutoStrPtr.
#ifndef AutoStrPtr
#define AutoStrPtr TAutoStrPtr
#endif
class AutoStrPtr {
public:
    // E:\gamedcs\herodefs.cpp:391, dc 0xd60d4
    AutoStrPtr() : m_string(0) {}
    // E:\gamedcs\herodefs.cpp:394, dc 0xd60dc
    ~AutoStrPtr() { delete[] m_string; }
    // E:\gamedcs\herodefs.cpp:396, dc 0xd60f4
    void set(char* value) { m_string = value; }
    // E:\gamedcs\herodefs.cpp:398, dc 0xd60f8
    char* get() const { return m_string; }

private:
    char* m_string;
};

}

VA(0x004e67a0, 0x176)  // dc 0xd5a40
unsigned char initializeHeroTraitsTable()
{
    SpreadsheetResource* resource = ResourceManager::getSpreadsheet(
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
        const SpreadsheetResource::TStringVector& values =
            resource->getRow(row);
        HeroTraits& traits = g_heroTraitsStorage[id];

        DATA_COMPGEN_GUARD(0x00698b99, heroStringsGuard, heroStrings)
        DATA(0x00698eb0)
        static AutoStrPtr heroStrings[156];

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

    resource->dispose();
    return 1;
}

VA(0x004e6920, 0x1E2)  // dc 0xd5ab4
unsigned char initializeHeroClassTraitsTable()
{
    SpreadsheetResource* resource = ResourceManager::getSpreadsheet(
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
        const SpreadsheetResource::TStringVector& values =
            resource->getRow(row);
        HeroClassTraits& traits = g_heroClassTraits[id];

        DATA_COMPGEN_GUARD(0x00698b9a, heroClassStringsGuard,
                          heroClassStrings)
        DATA(0x00699120)
        static AutoStrPtr heroClassStrings[18];

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

    resource->dispose();
    return 1;
}

VA(0x004e6b10, 0x1C8)  // dc 0xd5b28
unsigned char initializeSSkillTraitsTable()
{
    SpreadsheetResource* resource = ResourceManager::getSpreadsheet(
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
        const SpreadsheetResource::TStringVector& values =
            resource->getRow(row);
        SSkillTraits& traits = g_sSkillTraitsStorage[id];

        DATA_COMPGEN_GUARD(0x00698b98, secondarySkillStringsGuard,
                          secondarySkillNames)
        DATA(0x00698b28)
        static AutoStrPtr secondarySkillNames[28];

        secondarySkillNames[id].set(new char[strlen(values[0]) + 1]);
        strcpy(secondarySkillNames[id].get(), values[0]);
        traits.m_name = secondarySkillNames[id].get();

        DATA(0x00698b9c)
        static AutoStrPtr secondarySkillLevelNames[28][3];

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

    resource->dispose();
    return 1;
}

#if 0  // @carcass -- withdrawn inlined helpers and cinit rows

// E:\gamedcs\terrain.h:70

// Where the seven really went. The three Initialize*Traits rows are
// DC `static` with exactly one call site each - their Table function -
// so /Ob2 single-call-site inlining consumes them and no out-of-line
// retail body exists. The arithmetic corroborates: DC caller+callee
// 114+268=382 / 114+422=536 / 152+404=556 against retail's Table sizes
// 374 / 482 / 456 gives 0.98 / 0.90 / 0.82, one tight cluster inside the
// SH4->x86 band, where each Table taken alone would be 3.0-4.2x.
// The four TAutoStrPtr methods are anonymous-namespace accessors of
// DC size 8 / 24 / 4 / 4 bytes; mapping any of them onto a 95-byte body
// is 4x to 24x, far outside the band. They inline away entirely.
// ----------------------------------------------------------------------

// E:\gamedcs\herodefs.cpp:409
DC_ONLY(0xd5bc0, 0x10C)
void InitializeHeroTraits(int id, const std::vector<char* resource)
{
    // @stub
}

// E:\gamedcs\herodefs.cpp:441
DC_ONLY(0xd5d28, 0x1A6)
void InitializeHeroClassTraits(int id, const std::vector<char* resource)
{
    // @stub
}

// E:\gamedcs\herodefs.cpp:489
DC_ONLY(0xd5ee8, 0x194)
void InitializeSSkillTraits(int id, const std::vector<char* resource)
{
    // @stub
}

// E:\gamedcs\herodefs.cpp:391
DC_ONLY(0xd60d4, 0x8)
void `anonymous namespace'::AutoStrPtr::AutoStrPtr()
{
    // @stub
}

// E:\gamedcs\herodefs.cpp:394
DC_ONLY(0xd60dc, 0x18)
void `anonymous namespace'::AutoStrPtr::~AutoStrPtr()
{
    // @stub
}

// E:\gamedcs\herodefs.cpp:396
DC_ONLY(0xd60f4, 0x4)
void `anonymous namespace'::AutoStrPtr::set(char* pStr)
{
    // @stub
}

// E:\gamedcs\herodefs.cpp:398
DC_ONLY(0xd60f8, 0x4)
char* `anonymous namespace'::AutoStrPtr::get()
{
    // @stub
}

#endif  // @carcass
