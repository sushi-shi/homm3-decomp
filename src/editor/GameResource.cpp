// GameResource.cpp - the game resource type traits table (Loki h3maped
// object 14).
#include <assert.h>
#include <string.h>

#include "exceptions.h"
#include "autoarrayptr.h"
#include "resourceptr.h"
#include "textresource.h"
#include "editor/GameResource.h"

namespace {
TGameResourceTypeTraits aGameResourceTypeTraitsImp[kNumGameResourceTypes];
}

const TGameResourceTypeTraits* akGameResourceTypeTraits = aGameResourceTypeTraitsImp;

void InitializeGameResourceTypeTraitsTable()
{
    static TAutoArrayPtr<char> pNames(0);
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("restypes.txt"));
    if (!pTextResource.get())
#line 48
        throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"restypes.txt\".");
    assert(pTextResource->GetNumberOfStrings() >= kNumGameResourceTypes);
    int size = 0;
    unsigned int i;
    for (i = 0; i < kNumGameResourceTypes; i++)
        size += strlen(pTextResource->GetText(i)) + 1;
    pNames = TAutoArrayPtr<char>(new char[size]);
    if (!pNames.get())
#line 60
        throw TAllocationFailure(__FILE__, __LINE__);
    char* p = pNames.get();
    for (i = 0; i < kNumGameResourceTypes; i++) {
        const char* text = pTextResource->GetText(i);
        int length = strlen(text) + 1;
        memcpy(p, text, length);
        aGameResourceTypeTraitsImp[i].m_name = p;
        p += length;
    }
}
