// PrimarySkillTraits.cpp - the primary skill names (h3maped
// 0x494c0b..0x494d73, name inferred), filled from priskill.txt. Loki loads
// them in THero::initialize.
#include "editor/stdafx.h"

#include <string.h>

#include "va.h"
#include "autoarrayptr.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "textresource.h"
#include "editor/Hero.h"

namespace {
DATA(0x005a2558) THero::TPrimarySkillTraits aHeroPrimarySkillTraitsImp[kNumPrimarySkills];
}

DATA(0x0058d348) THero::TPrimarySkillTraits* akHeroPrimarySkillTraits = aHeroPrimarySkillTraitsImp;

VA(0x00494c27, 0x136)
void InitializePrimarySkillTraitsTable()
{
    DATA_COMPGEN_GUARD(0x005a2570, primarySkillNamesGuard, pNames)
    VA_COMPGEN(0x00494d5d, 0x16, STATIC_DTOR, pNames)
    DATA(0x005a2568) static TAutoArrayPtr<char> pNames(0);
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("priskill.txt"));
    if (!pTextResource.get())
        throw TRuntimeError();
    int size = 0;
    unsigned int i;
    for (i = 0; i < kNumPrimarySkills; i++)
        size += strlen(pTextResource->GetText(i)) + 1;
    pNames = TAutoArrayPtr<char>(new char[size]);
    if (!pNames.get())
        throw TAllocationFailure();
    char* p = pNames.get();
    for (i = 0; i < kNumPrimarySkills; i++) {
        const char* text = pTextResource->GetText(i);
        int length = strlen(text) + 1;
        memcpy(p, text, length);
        aHeroPrimarySkillTraitsImp[i].m_name = p;
        p += length;
    }
}
