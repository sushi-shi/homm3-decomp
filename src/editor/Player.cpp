// Player.cpp - the player traits table (h3maped 0x494308..0x494551;
// Loki h3maped object 24).
#include "editor/stdafx.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "va.h"
#include "autoarrayptr.h"
#include "resourceptr.h"
#include "textresource.h"
#include "editor/MapEditorText.h"
#include "editor/Player.h"

namespace {
DATA(0x005a24f0) TPlayerTraits aPlayerTraitsImp[kNumPlayers];
}

DATA(0x0058d314) const TPlayerTraits* akPlayerTraits = aPlayerTraitsImp;

// Loki's port reports a missing plcolors.txt with its file name and line;
// the Windows release throws the bare runtime error.
VA(0x00494324, 0x205)
bool InitializePlayerTraitsTable()
{
    assert(kPlayerNameFmtStr != NULL);
    // One guard byte holds both arrays' bits; each array's teardown runs
    // ??_M over its eight elements.
    DATA_COMPGEN_GUARD(0x005a24e8, playerTraitsNamesGuard, aNames)

    VA_COMPGEN(0x0049453d, 0x14, STATIC_DTOR, aNames)
    DATA(0x005a2468) static TAutoArrayPtr<char> aNames[kNumPlayers];
    VA_COMPGEN(0x00494529, 0x14, STATIC_DTOR, aColorNames)
    DATA(0x005a24a8) static TAutoArrayPtr<char> aColorNames[kNumPlayers];
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("plcolors.txt"));
    if (!pTextResource.get())
        throw TRuntimeError();
    assert(pTextResource->GetNumberOfStrings() >= kNumPlayers);
    for (int i = 0; i < kNumPlayers; i++) {
        aColorNames[i] = TAutoArrayPtr<char>(new char[strlen(pTextResource->GetText(i)) + 1]);
        if (!aColorNames[i].get())
            throw TAllocationFailure();
        strcpy(aColorNames[i].get(), pTextResource->GetText(i));
        aPlayerTraitsImp[i].m_pColorName = aColorNames[i].get();
        char name[256];
        sprintf(name, kPlayerNameFmtStr, i + 1, pTextResource->GetText(i));
        aNames[i] = TAutoArrayPtr<char>(new char[strlen(name) + 1]);
        if (!aNames[i].get())
            throw TAllocationFailure();
        strcpy(aNames[i].get(), name);
        aPlayerTraitsImp[i].m_pName = aNames[i].get();
    }
    return true;
}
