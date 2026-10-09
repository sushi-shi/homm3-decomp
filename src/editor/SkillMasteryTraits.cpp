// SkillMasteryTraits.cpp - the secondary skill mastery names (h3maped
// 0x4b909d..0x4b9205, name inferred), basic mastery first, filled from
// skilllev.txt. Loki loads them in THero::initialize.
#include "editor/stdafx.h"

#include <string.h>

#include "va.h"
#include "autoarrayptr.h"
#include "herospec.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "textresource.h"
#include "editor/Hero.h"

namespace {
DATA(0x005a30f0) THero::TSkillMasteryTraits aHeroSkillMasteryTraitsImp[kNumMasteries - eMasteryBasic];
}

DATA(0x0058f1b4) THero::TSkillMasteryTraits* akHeroSkillMasteryTraits = aHeroSkillMasteryTraitsImp;

VA(0x004b90b9, 0x136)
void InitializeSkillMasteryTraitsTable()
{
    DATA_COMPGEN_GUARD(0x005a3108, skillMasteryNamesGuard, pNames)
    VA_COMPGEN(0x004b91ef, 0x16, STATIC_DTOR, pNames)
    DATA(0x005a3100) static TAutoArrayPtr<char> pNames(0);
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("skilllev.txt"));
    if (!pTextResource.get())
        throw TRuntimeError();
    int size = 0;
    unsigned int i;
    for (i = 0; i < kNumMasteries - eMasteryBasic; i++)
        size += strlen(pTextResource->GetText(i)) + 1;
    pNames = TAutoArrayPtr<char>(new char[size]);
    if (!pNames.get())
        throw TAllocationFailure();
    char* p = pNames.get();
    for (i = 0; i < kNumMasteries - eMasteryBasic; i++) {
        const char* text = pTextResource->GetText(i);
        int length = strlen(text) + 1;
        memcpy(p, text, length);
        aHeroSkillMasteryTraitsImp[i].m_name = p;
        p += length;
    }
}
