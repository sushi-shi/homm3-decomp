// GameResource.cpp - the game resource type traits table (h3maped
// 0x43d43c; Loki h3maped object 14), filled from restypes.txt. The
// Windows release throws the bare runtime error and allocation failure.
#include "editor/stdafx.h"

#include <string.h>

#include "va.h"
#include "autoarrayptr.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "textresource.h"
#include "editor/GameResource.h"

namespace {
DATA(0x0059e43c) TGameResourceTypeTraits aGameResourceTypeTraitsImp[kNumGameResourceTypes];
}

DATA(0x00584188) const TGameResourceTypeTraits* akGameResourceTypeTraits = aGameResourceTypeTraitsImp;

VA(0x0043d43c, 0x136)
void InitializeGameResourceTypeTraitsTable()
{
    DATA_COMPGEN_GUARD(0x0059e438, resourceNamesGuard, pNames)
    VA_COMPGEN(0x0043d572, 0x16, STATIC_DTOR, pNames)
    DATA(0x0059e430) static TAutoArrayPtr<char> pNames(0);
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("restypes.txt"));
    if (!pTextResource.get())
        throw TRuntimeError();
    int size = 0;
    unsigned int i;
    for (i = 0; i < kNumGameResourceTypes; i++)
        size += strlen(pTextResource->GetText(i)) + 1;
    pNames = TAutoArrayPtr<char>(new char[size]);
    if (!pNames.get())
        throw TAllocationFailure();
    char* p = pNames.get();
    for (i = 0; i < kNumGameResourceTypes; i++) {
        const char* text = pTextResource->GetText(i);
        int length = strlen(text) + 1;
        memcpy(p, text, length);
        aGameResourceTypeTraitsImp[i].m_name = p;
        p += length;
    }
}
