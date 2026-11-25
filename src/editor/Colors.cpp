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

// Palette entries: each ground tileset's sprite keeps its mini-map colours
// of open ground and of obstacles at 8 and 9; game.pal keeps the player
// colours from 63 on (Loki Colors.cpp). Retail pushes "game.pal" as a
// string literal, so the palette name stays one here.
const unsigned char kGroundColorIndex = 8;
const unsigned char kObstacleColorIndex = 9;
const unsigned char kFirstPlayerColorIndex = 63;

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
        aTerrainColorsImp[tilesetNum].m_color = aColors[kGroundColorIndex];
        aTerrainColorsImp[tilesetNum].m_obstacleColor = aColors[kObstacleColorIndex];
    }
    TPalette16* pPalette = ResourceManager::GetPalette("game.pal");
    if (!pPalette)
        throw TRuntimeError();
    for (unsigned int i = 0; i < 9; i++)
        aPlayerColorImp[i] = pPalette->Palette[i + kFirstPlayerColorIndex];
    ResourceManager::Dispose(pPalette);
}
