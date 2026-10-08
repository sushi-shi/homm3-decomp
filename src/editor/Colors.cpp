// Colors.cpp - terrain and player colours (h3maped 0x40c9d0..0x40cc2a;
// Loki h3maped object 8).
#include "editor/stdafx.h"

#include <assert.h>

#include "va.h"
#include "csprite.h"
#include "palette.h"
#include "resourcemanager.h"
#include "editor/Colors.h"
#include "editor/Tile.h"

namespace {
DATA(0x0059a6f4) TTerrainColors aTerrainColorsImp[10];
DATA(0x0059a6e0) unsigned short aPlayerColorImp[9];
}

DATA(0x0057ce94) const TTerrainColors* akTerrainColors = aTerrainColorsImp;
DATA(0x0057ce98) const unsigned short* akPlayerColor = aPlayerColorImp;

// Loki's port reports a missing game.pal with its name; the Windows
// release throws the bare runtime error.
VA(0x0040cba4, 0x86)
void initColors()
{
    for (unsigned int tilesetNum = 0; tilesetNum < 10; tilesetNum++) {
        assert(akGroundTilesetTraits[tilesetNum].m_pSprite != NULL);
        const unsigned short* aColors = akGroundTilesetTraits[tilesetNum].m_pSprite->GetPalette();
        assert(aColors != NULL);
        aTerrainColorsImp[tilesetNum].m_color = aColors[8];
        aTerrainColorsImp[tilesetNum].m_obstacleColor = aColors[9];
    }
    TPalette16* pPalette = ResourceManager::GetPalette("game.pal");
    if (!pPalette)
        throw TRuntimeError();
    for (unsigned int i = 0; i < 9; i++)
        aPlayerColorImp[i] = pPalette->Palette[i + 63];
    ResourceManager::Dispose(pPalette);
}
