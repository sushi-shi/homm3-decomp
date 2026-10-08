// Colors.cpp - terrain and player colours (Loki h3maped object 8).
#include <assert.h>
#include <string>

#include "exceptions.h"
#include "csprite.h"
#include "palette.h"
#include "resourcemanager.h"
#include "terrain.h"
#include "editor/Colors.h"
#include "editor/Tile.h"

// Palette entries: each ground tileset's sprite keeps its mini-map colours
// of open ground and of obstacles at 8 and 9; game.pal keeps the player
// colours from 63 on.
const unsigned char kGroundColorIndex = 8;
const unsigned char kObstacleColorIndex = 9;
const unsigned char kFirstPlayerColorIndex = 63;
const char kPlayerPaletteName[] = "game.pal";

namespace {
TTerrainColors aTerrainColorsImp[10];
unsigned short aPlayerColorImp[9];
}

const TTerrainColors* akTerrainColors = aTerrainColorsImp;
const unsigned short* akPlayerColor = aPlayerColorImp;

void initColors()
{
    for (unsigned int tilesetNum = 0; tilesetNum < 10; tilesetNum++) {
#line 57
        assert(akGroundTilesetTraits[ tilesetNum ].m_pSprite != NULL);
        const unsigned short* aColors = akGroundTilesetTraits[tilesetNum].m_pSprite->GetPalette();
        assert(aColors != NULL);
        aTerrainColorsImp[tilesetNum].m_color = aColors[kGroundColorIndex];
        aTerrainColorsImp[tilesetNum].m_obstacleColor = aColors[kObstacleColorIndex];
    }
    TPalette16* pPalette = ResourceManager::GetPalette(kPlayerPaletteName, false);
    if (!pPalette)
#line 66
        throw TRuntimeError(__FILE__, __LINE__,
                            string("Unable to load palette:  \"") + "game.pal" + "\".");
    for (unsigned int i = 0; i < 9; i++)
        aPlayerColorImp[i] = pPalette->m_data[i + kFirstPlayerColorIndex];
    ResourceManager::Dispose(pPalette);
}
