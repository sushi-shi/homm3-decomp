// Player.cpp - the player traits table (Loki h3maped object 24).
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "exceptions.h"
#include "autoarrayptr.h"
#include "resourceptr.h"
#include "textresource.h"
#include "editor/MapEditorText.h"
#include "editor/Player.h"

namespace {
TPlayerTraits aPlayerTraitsImp[kNumPlayers];
}

const TPlayerTraits* akPlayerTraits = aPlayerTraitsImp;

bool InitializePlayerTraitsTable()
{
#line 48
    assert(kPlayerNameFmtStr != NULL);
    static TAutoArrayPtr<char> aNames[kNumPlayers];
    static TAutoArrayPtr<char> aColorNames[kNumPlayers];
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("plcolors.txt"));
    if (!pTextResource.get())
#line 55
        throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"plcolors.txt\".");
    assert(pTextResource->GetNumberOfStrings() >= kNumPlayers);
    for (int i = 0; i < kNumPlayers; i++) {
        aColorNames[i] = TAutoArrayPtr<char>(new char[strlen(pTextResource->GetText(i)) + 1]);
        if (!aColorNames[i].get())
#line 64
            throw TAllocationFailure(__FILE__, __LINE__);
        strcpy(aColorNames[i].get(), pTextResource->GetText(i));
        aPlayerTraitsImp[i].m_pColorName = aColorNames[i].get();
        char name[256];
        sprintf(name, kPlayerNameFmtStr, i + 1, pTextResource->GetText(i));
        aNames[i] = TAutoArrayPtr<char>(new char[strlen(name) + 1]);
        if (!aNames[i].get())
#line 75
            throw TAllocationFailure(__FILE__, __LINE__);
        strcpy(aNames[i].get(), name);
        aPlayerTraitsImp[i].m_pName = aNames[i].get();
    }
    return true;
}
