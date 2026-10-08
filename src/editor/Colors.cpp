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

namespace {
TTerrainColors aTerrainColorsImp[10];
unsigned short aPlayerColorImp[9];
}

const TTerrainColors* const akTerrainColors = aTerrainColorsImp;
const unsigned short* const akPlayerColor = aPlayerColorImp;

void initColors()
{
    for (unsigned int tilesetNum = 0; tilesetNum < 10; tilesetNum++) {
#line 57
        assert(akGroundTilesetTraits[ tilesetNum ].m_pSprite != NULL);
        const unsigned short* aColors = akGroundTilesetTraits[tilesetNum].m_pSprite->GetPalette();
        assert(aColors != NULL);
        aTerrainColorsImp[tilesetNum].m_color = aColors[8];
        aTerrainColorsImp[tilesetNum].m_obstacleColor = aColors[9];
    }
    TPalette16* pPalette = ResourceManager::GetPalette("game.pal", false);
    if (!pPalette)
#line 66
        throw TRuntimeError(__FILE__, __LINE__,
                            string("Unable to load palette:  \"") + "game.pal" + "\".");
    for (unsigned int i = 0; i < 9; i++)
        aPlayerColorImp[i] = pPalette->m_data[i + 63];
    ResourceManager::Dispose(pPalette);
}
