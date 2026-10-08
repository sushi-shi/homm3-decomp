// Tile.cpp - Loki h3maped object 32: the tileset sprites, their traits
// tables and palette animation, and the per-zoom drawing functions.
// Assert and throw lines come from the retail immediates.
#include "editor/stdafx.h"

#include <assert.h>
#include <string>

#include "exceptions.h"
#include "csprite.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "terrain_type.h"
#include "editor/Tile.h"

namespace {

void DrawTile(const CSprite* pSprite, unsigned int frameNum, int x, int y,
              uword* pBuffer, unsigned int width, unsigned int height,
              unsigned int pitch, bool bHFlip, bool bVFlip)
{
#line 33
    assert(pSprite != NULL);
    unsigned int tileSize = akZoomTraits[eZoom100].m_tileSize;
    pSprite->DrawTile(frameNum, 0, 0, tileSize, tileSize, pBuffer, x, y,
                      width, height, pitch, bHFlip, bVFlip);
}

void DrawAdvObj(const CSprite* pSprite, unsigned int frameNum, int srcX, int srcY,
                unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                uword* pBuffer, unsigned int width, unsigned int height,
                unsigned int pitch, unsigned short flagColor)
{
#line 56
    assert(pSprite != NULL);
    pSprite->DrawAdvObjWithFlag(frameNum, srcX, srcY, srcWidth, srcHeight, pBuffer,
                                x, y, width, height, pitch, flagColor, false);
}

void DrawAdvObjShadow(const CSprite* pSprite, unsigned int frameNum, int srcX, int srcY,
                      unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                      uword* pBuffer, unsigned int width, unsigned int height,
                      unsigned int pitch)
{
#line 76
    assert(pSprite != NULL);
    pSprite->DrawAdvObjShadow(frameNum, srcX, srcY, srcWidth, srcHeight, pBuffer,
                              x, y, width, height, pitch, false);
}

void DrawTileScaled50(const CSprite* pSprite, unsigned int frameNum, int x, int y,
                      uword* pBuffer, unsigned int width, unsigned int height,
                      unsigned int pitch, bool bHFlip, bool bVFlip)
{
#line 95
    assert(pSprite != NULL);
    unsigned int tileSize = akZoomTraits[eZoom50].m_tileSize;
    pSprite->DrawTileScaled50(frameNum, 0, 0, tileSize, tileSize, pBuffer, x, y,
                              width, height, pitch, bHFlip, bVFlip);
}

void DrawAdvObjScaled50(const CSprite* pSprite, unsigned int frameNum, int srcX, int srcY,
                        unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                        uword* pBuffer, unsigned int width, unsigned int height,
                        unsigned int pitch, unsigned short flagColor)
{
#line 118
    assert(pSprite != NULL);
    pSprite->DrawAdvObjWithFlagScaled50(frameNum, srcX, srcY, srcWidth, srcHeight, pBuffer,
                                        x, y, width, height, pitch, flagColor);
}

void DrawAdvObjShadowScaled50(const CSprite* pSprite, unsigned int frameNum,
                              int srcX, int srcY,
                              unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                              uword* pBuffer, unsigned int width,
                              unsigned int height, unsigned int pitch)
{
#line 138
    assert(pSprite != NULL);
    pSprite->DrawAdvObjShadowScaled50(frameNum, srcX, srcY, srcWidth, srcHeight, pBuffer,
                                      x, y, width, height, pitch);
}

void DrawTileScaled25(const CSprite* pSprite, unsigned int frameNum, int x, int y,
                      uword* pBuffer, unsigned int width, unsigned int height,
                      unsigned int pitch, bool bHFlip, bool bVFlip)
{
#line 157
    assert(pSprite != NULL);
    unsigned int tileSize = akZoomTraits[eZoom25].m_tileSize;
    pSprite->DrawTileScaled25(frameNum, 0, 0, tileSize, tileSize, pBuffer, x, y,
                              width, height, pitch, bHFlip, bVFlip);
}

void DrawAdvObjScaled25(const CSprite* pSprite, unsigned int frameNum, int srcX, int srcY,
                        unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                        uword* pBuffer, unsigned int width, unsigned int height,
                        unsigned int pitch, unsigned short flagColor)
{
#line 180
    assert(pSprite != NULL);
    pSprite->DrawAdvObjWithFlagScaled25(frameNum, srcX, srcY, srcWidth, srcHeight, pBuffer,
                                        x, y, width, height, pitch, flagColor);
}

void DrawAdvObjShadowScaled25(const CSprite* pSprite, unsigned int frameNum,
                              int srcX, int srcY,
                              unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                              uword* pBuffer, unsigned int width,
                              unsigned int height, unsigned int pitch)
{
#line 200
    assert(pSprite != NULL);
    pSprite->DrawAdvObjShadowScaled25(frameNum, srcX, srcY, srcWidth, srcHeight, pBuffer,
                                      x, y, width, height, pitch);
}

}  // namespace

const TZoomTraits akZoomTraits[3] = {
    { 32, 1, 0, DrawTile, DrawAdvObj, DrawAdvObjShadow },
    { 16, 2, 1, DrawTileScaled50, DrawAdvObjScaled50, DrawAdvObjShadowScaled50 },
    { 8, 4, 2, DrawTileScaled25, DrawAdvObjScaled25, DrawAdvObjShadowScaled25 },
};

namespace {

const char* const akGroundTilesetName[10] = {
    "dirttl.def",
    "sandtl.def",
    "grastl.def",
    "snowtl.def",
    "swmptl.def",
    "rougtl.def",
    "subbtl.def",
    "lavatl.def",
    "watrtl.def",
    "rocktl.def",
};

const char* const akRiverTilesetName[4] = {
    "clrrvr.def",
    "icyrvr.def",
    "mudrvr.def",
    "lavrvr.def",
};

const char* const akRoadTilesetName[3] = {
    "dirtrd.def",
    "gravrd.def",
    "cobbrd.def",
};

// Per-tile traits: only lava tiles 65-70 and every water tile animate.
const TGroundTilesetTraits::TTileTraits akDirtTileTraits[46] = {
    { false }
};
const TGroundTilesetTraits::TTileTraits akSandTileTraits[24] = {
    { false }
};
const TGroundTilesetTraits::TTileTraits akGenericTileTraits[79] = {
    { false }
};
const TGroundTilesetTraits::TTileTraits akLavaTileTraits[79] = {
    { false }, { false }, { false }, { false }, { false }, { false }, { false }, { false },
    { false }, { false }, { false }, { false }, { false }, { false }, { false }, { false },
    { false }, { false }, { false }, { false }, { false }, { false }, { false }, { false },
    { false }, { false }, { false }, { false }, { false }, { false }, { false }, { false },
    { false }, { false }, { false }, { false }, { false }, { false }, { false }, { false },
    { false }, { false }, { false }, { false }, { false }, { false }, { false }, { false },
    { false }, { false }, { false }, { false }, { false }, { false }, { false }, { false },
    { false }, { false }, { false }, { false }, { false }, { false }, { false }, { false },
    { false }, { true }, { true }, { true }, { true }, { true }, { true }, { false },
    { false }, { false }, { false }, { false }, { false }, { false }, { false },
};
const TGroundTilesetTraits::TTileTraits akWaterTileTraits[33] = {
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },
    { true },
};
const TGroundTilesetTraits::TTileTraits akRockTileTraits[48] = {
    { false }
};

TGroundTilesetTraits aGroundTilesetTraitsImp[10] = {
    TGroundTilesetTraits(false, 46, akDirtTileTraits),
    TGroundTilesetTraits(false, 24, akSandTileTraits),
    TGroundTilesetTraits(false, 79, akGenericTileTraits),
    TGroundTilesetTraits(false, 79, akGenericTileTraits),
    TGroundTilesetTraits(false, 79, akGenericTileTraits),
    TGroundTilesetTraits(false, 79, akGenericTileTraits),
    TGroundTilesetTraits(false, 79, akGenericTileTraits),
    TGroundTilesetTraits(true, 79, akLavaTileTraits),
    TGroundTilesetTraits(true, 33, akWaterTileTraits),
    TGroundTilesetTraits(false, 48, akRockTileTraits),
};

TRiverTilesetTraits aRiverTilesetTraitsImp[4] = {
    { NULL, true },
    { NULL, false },
    { NULL, true },
    { NULL, true },
};

TRoadTilesetTraits aRoadTilesetTraitsImp[3] = {
    { NULL },
    { NULL },
    { NULL },
};

}  // namespace

const TGroundTilesetTraits* akGroundTilesetTraits = aGroundTilesetTraitsImp;
const TRiverTilesetTraits* akRiverTilesetTraits = aRiverTilesetTraitsImp;
const TRoadTilesetTraits* akRoadTilesetTraits = aRoadTilesetTraitsImp;

void loadTilesets()
{
    unsigned int tilesetNum;

    static TResourcePtr<CSprite> apAutoGroundTilesetSprite[10];
    for (tilesetNum = 0; tilesetNum < 10; tilesetNum++) {
        apAutoGroundTilesetSprite[tilesetNum] =
            TResourcePtr<CSprite>(ResourceManager::GetSprite(akGroundTilesetName[tilesetNum]));
        if (!apAutoGroundTilesetSprite[tilesetNum].get())
#line 648
            throw TRuntimeError(__FILE__, __LINE__, string("Unable to load tileset:  \"") + akGroundTilesetName[tilesetNum] + "\".");
        assert(apAutoGroundTilesetSprite[ tilesetNum ]->GetNumFrames() == aGroundTilesetTraitsImp[ tilesetNum ].m_numTiles);
        aGroundTilesetTraitsImp[tilesetNum].m_pSprite = apAutoGroundTilesetSprite[tilesetNum].get();
    }

    static TResourcePtr<CSprite> apAutoRiverTilesetSprite[4];
    for (tilesetNum = 0; tilesetNum < 4; tilesetNum++) {
        apAutoRiverTilesetSprite[tilesetNum] =
            TResourcePtr<CSprite>(ResourceManager::GetSprite(akRiverTilesetName[tilesetNum]));
        if (!apAutoRiverTilesetSprite[tilesetNum].get())
#line 660
            throw TRuntimeError(__FILE__, __LINE__, string("Unable to load tileset:  \"") + akRiverTilesetName[tilesetNum] + "\".");
        aRiverTilesetTraitsImp[tilesetNum].m_pSprite = apAutoRiverTilesetSprite[tilesetNum].get();
    }

    static TResourcePtr<CSprite> apAutoRoadTilesetSprite[3];
    for (tilesetNum = 0; tilesetNum < 3; tilesetNum++) {
        apAutoRoadTilesetSprite[tilesetNum] =
            TResourcePtr<CSprite>(ResourceManager::GetSprite(akRoadTilesetName[tilesetNum]));
        if (!apAutoRoadTilesetSprite[tilesetNum].get())
#line 671
            throw TRuntimeError(__FILE__, __LINE__, string("Unable to load tileset:  \"") + akRoadTilesetName[tilesetNum] + "\".");
        aRoadTilesetTraitsImp[tilesetNum].m_pSprite = apAutoRoadTilesetSprite[tilesetNum].get();
    }
}

void animateTilesets(unsigned int frameNum)
{
    register const int kWaterCycle1Begin = 229;
    register const int kWaterCycle1End = 241;
    register const int kWaterCycle2Begin = 242;
    register const int kWaterCycle2End = 254;
    int waterCycle1Step = -(frameNum % (kWaterCycle1End - kWaterCycle1Begin));
    int waterCycle2Step = -(frameNum % (kWaterCycle2End - kWaterCycle2Begin));
    CSprite* pWaterSprite = const_cast<CSprite*>(aGroundTilesetTraitsImp[eTerrainWater].m_pSprite);
    pWaterSprite->ColorCycle(kWaterCycle1Begin, kWaterCycle1End - 1, waterCycle1Step);
    pWaterSprite->ColorCycle(kWaterCycle2Begin, kWaterCycle2End - 1, waterCycle2Step);

    register const int kLavaCycleBegin = 246;
    register const int kLavaCycleEnd = 255;
    int lavaCycleStep = -(frameNum % (kLavaCycleEnd - kLavaCycleBegin));
    CSprite* pLavaSprite = const_cast<CSprite*>(aGroundTilesetTraitsImp[eTerrainLava].m_pSprite);
    pLavaSprite->ColorCycle(kLavaCycleBegin, kLavaCycleEnd - 1, lavaCycleStep);

    register const int kClearRiverCycle1Begin = 183;
    register const int kClearRiverCycle1End = 195;
    register const int kClearRiverCycle2Begin = 195;
    register const int kClearRiverCycle2End = 201;
    int clearRiverCycle1Step = -(frameNum % (kClearRiverCycle1End - kClearRiverCycle1Begin));
    int clearRiverCycle2Step = -(frameNum % (kClearRiverCycle2End - kClearRiverCycle2Begin));
    CSprite* pClearRiverSprite = const_cast<CSprite*>(aRiverTilesetTraitsImp[0].m_pSprite);
    pClearRiverSprite->ColorCycle(kClearRiverCycle1Begin, kClearRiverCycle1End - 1, clearRiverCycle1Step);
    pClearRiverSprite->ColorCycle(kClearRiverCycle2Begin, kClearRiverCycle2End - 1, clearRiverCycle2Step);

    register const int kMudRiverCycle1Begin = 228;
    register const int kMudRiverCycle1End = 240;
    register const int kMudRiverCycle2Begin = 183;
    register const int kMudRiverCycle2End = 189;
    register const int kMudRiverCycle3Begin = 240;
    register const int kMudRiverCycle3End = 246;
    int mudRiverCycle1Step = -(frameNum % (kMudRiverCycle1End - kMudRiverCycle1Begin));
    int mudRiverCycle2Step = -(frameNum % (kMudRiverCycle2End - kMudRiverCycle2Begin));
    int mudRiverCycle3Step = -(frameNum % (kMudRiverCycle3End - kMudRiverCycle3Begin));
    CSprite* pMudRiverSprite = const_cast<CSprite*>(aRiverTilesetTraitsImp[2].m_pSprite);
    pMudRiverSprite->ColorCycle(kMudRiverCycle1Begin, kMudRiverCycle1End - 1, mudRiverCycle1Step);
    pMudRiverSprite->ColorCycle(kMudRiverCycle2Begin, kMudRiverCycle2End - 1, mudRiverCycle2Step);
    pMudRiverSprite->ColorCycle(kMudRiverCycle3Begin, kMudRiverCycle3End - 1, mudRiverCycle3Step);

    register const int kLavaRiverCycleBegin = 240;
    register const int kLavaRiverCycleEnd = 249;
    int lavaRiverCycleStep = -(frameNum % (kLavaRiverCycleEnd - kLavaRiverCycleBegin));
    CSprite* pLavaRiverSprite = const_cast<CSprite*>(aRiverTilesetTraitsImp[3].m_pSprite);
    pLavaRiverSprite->ColorCycle(kLavaRiverCycleBegin, kLavaRiverCycleEnd - 1, lavaRiverCycleStep);
}
