// Tile.cpp - the tileset sprites, their traits tables and palette animation,
// and the per-zoom drawing functions (h3maped 0x4be1b8..0x4be7df; Loki
// h3maped object 32). Loki's port reports a missing tileset with its name;
// the Windows release throws the bare runtime error.
#include "editor/stdafx.h"

#include <assert.h>

#include "va.h"
#include "csprite.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "terrain_type.h"
#include "editor/Tile.h"

namespace {

VA(0x004be1b8, 0x33)
void DrawTile(const CSprite* pSprite, unsigned int frameNum, int x, int y,
              uword* pBuffer, unsigned int width, unsigned int height,
              unsigned int pitch, bool bHFlip, bool bVFlip)
{
    assert(pSprite != NULL);
    unsigned int tileSize = akZoomTraits[eZoom100].m_tileSize;
    pSprite->DrawTile(frameNum, 0, 0, tileSize, tileSize, pBuffer, x, y,
                      width, height, pitch, bHFlip, bVFlip);
}

VA(0x004be1eb, 0x33)
void DrawAdvObj(const CSprite* pSprite, unsigned int frameNum, int srcX, int srcY,
                unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                uword* pBuffer, unsigned int width, unsigned int height,
                unsigned int pitch, unsigned short flagColor)
{
    assert(pSprite != NULL);
    pSprite->DrawAdvObjWithFlag(frameNum, srcX, srcY, srcWidth, srcHeight, pBuffer,
                                x, y, width, height, pitch, flagColor, false);
}

VA(0x004be21e, 0x30)
void DrawAdvObjShadow(const CSprite* pSprite, unsigned int frameNum, int srcX, int srcY,
                      unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                      uword* pBuffer, unsigned int width, unsigned int height,
                      unsigned int pitch)
{
    assert(pSprite != NULL);
    pSprite->DrawAdvObjShadow(frameNum, srcX, srcY, srcWidth, srcHeight, pBuffer,
                              x, y, width, height, pitch, false);
}

VA(0x004be24e, 0x33)
void DrawTileScaled50(const CSprite* pSprite, unsigned int frameNum, int x, int y,
                      uword* pBuffer, unsigned int width, unsigned int height,
                      unsigned int pitch, bool bHFlip, bool bVFlip)
{
    assert(pSprite != NULL);
    unsigned int tileSize = akZoomTraits[eZoom50].m_tileSize;
    pSprite->DrawTileScaled50(frameNum, 0, 0, tileSize, tileSize, pBuffer, x, y,
                              width, height, pitch, bHFlip, bVFlip);
}

VA(0x004be281, 0x31)
void DrawAdvObjScaled50(const CSprite* pSprite, unsigned int frameNum, int srcX, int srcY,
                        unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                        uword* pBuffer, unsigned int width, unsigned int height,
                        unsigned int pitch, unsigned short flagColor)
{
    assert(pSprite != NULL);
    pSprite->DrawAdvObjWithFlagScaled50(frameNum, srcX, srcY, srcWidth, srcHeight, pBuffer,
                                        x, y, width, height, pitch, flagColor);
}

VA(0x004be2b2, 0x2e)
void DrawAdvObjShadowScaled50(const CSprite* pSprite, unsigned int frameNum,
                              int srcX, int srcY,
                              unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                              uword* pBuffer, unsigned int width,
                              unsigned int height, unsigned int pitch)
{
    assert(pSprite != NULL);
    pSprite->DrawAdvObjShadowScaled50(frameNum, srcX, srcY, srcWidth, srcHeight, pBuffer,
                                      x, y, width, height, pitch);
}

VA(0x004be2e0, 0x33)
void DrawTileScaled25(const CSprite* pSprite, unsigned int frameNum, int x, int y,
                      uword* pBuffer, unsigned int width, unsigned int height,
                      unsigned int pitch, bool bHFlip, bool bVFlip)
{
    assert(pSprite != NULL);
    unsigned int tileSize = akZoomTraits[eZoom25].m_tileSize;
    pSprite->DrawTileScaled25(frameNum, 0, 0, tileSize, tileSize, pBuffer, x, y,
                              width, height, pitch, bHFlip, bVFlip);
}

VA(0x004be313, 0x31)
void DrawAdvObjScaled25(const CSprite* pSprite, unsigned int frameNum, int srcX, int srcY,
                        unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                        uword* pBuffer, unsigned int width, unsigned int height,
                        unsigned int pitch, unsigned short flagColor)
{
    assert(pSprite != NULL);
    pSprite->DrawAdvObjWithFlagScaled25(frameNum, srcX, srcY, srcWidth, srcHeight, pBuffer,
                                        x, y, width, height, pitch, flagColor);
}

VA(0x004be344, 0x2e)
void DrawAdvObjShadowScaled25(const CSprite* pSprite, unsigned int frameNum,
                              int srcX, int srcY,
                              unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                              uword* pBuffer, unsigned int width,
                              unsigned int height, unsigned int pitch)
{
    assert(pSprite != NULL);
    pSprite->DrawAdvObjShadowScaled25(frameNum, srcX, srcY, srcWidth, srcHeight, pBuffer,
                                      x, y, width, height, pitch);
}

}  // namespace

DATA(0x00543a30)
const TZoomTraits akZoomTraits[kNumZooms] = {
    { 32, 1, 0, DrawTile, DrawAdvObj, DrawAdvObjShadow },
    { 16, 2, 1, DrawTileScaled50, DrawAdvObjScaled50, DrawAdvObjShadowScaled50 },
    { 8, 4, 2, DrawTileScaled25, DrawAdvObjScaled25, DrawAdvObjShadowScaled25 },
};

namespace {

DATA(0x005438b0)
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

DATA(0x005438d8)
const char* const akRiverTilesetName[4] = {
    "clrrvr.def",
    "icyrvr.def",
    "mudrvr.def",
    "lavrvr.def",
};

DATA(0x005438e8)
const char* const akRoadTilesetName[3] = {
    "dirtrd.def",
    "gravrd.def",
    "cobbrd.def",
};

// Per-tile traits: only lava tiles 65-70 and every water tile animate.
DATA(0x005438f4)
const TGroundTilesetTraits::TTileTraits akDirtTileTraits[46] = {
    { false }
};
DATA(0x00543924)
const TGroundTilesetTraits::TTileTraits akSandTileTraits[24] = {
    { false }
};
DATA(0x0054393c)
const TGroundTilesetTraits::TTileTraits akGenericTileTraits[79] = {
    { false }
};
DATA(0x0054398c)
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
DATA(0x005439dc)
const TGroundTilesetTraits::TTileTraits akWaterTileTraits[33] = {
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },
    { true }, { true }, { true }, { true }, { true }, { true }, { true }, { true },
    { true },
};
DATA(0x00543a00)
const TGroundTilesetTraits::TTileTraits akRockTileTraits[48] = {
    { false }
};

DATA(0x005a4ef8)
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
VA_COMPGEN(0x004be377, 0x12c, STATIC_CTOR, aGroundTilesetTraitsImp)

DATA(0x00592ac8)
TRiverTilesetTraits aRiverTilesetTraitsImp[4] = {
    { NULL, true },
    { NULL, false },
    { NULL, true },
    { NULL, true },
};

DATA(0x005a4fe8)
TRoadTilesetTraits aRoadTilesetTraitsImp[3] = {
    { NULL },
    { NULL },
    { NULL },
};

}  // namespace

DATA(0x00592ae8) const TGroundTilesetTraits* akGroundTilesetTraits = aGroundTilesetTraitsImp;
DATA(0x00592aec) const TRiverTilesetTraits* akRiverTilesetTraits = aRiverTilesetTraitsImp;
DATA(0x00592af0) const TRoadTilesetTraits* akRoadTilesetTraits = aRoadTilesetTraitsImp;

VA(0x004be4a3, 0x207)
void loadTilesets()
{
    unsigned int tilesetNum;

    // One guard byte holds the three arrays' bits.
    DATA_COMPGEN_GUARD(0x005a4ee8, tilesetSpritesGuard, apAutoGroundTilesetSprite)

    VA_COMPGEN(0x004be6d2, 0x14, STATIC_DTOR, apAutoGroundTilesetSprite)
    DATA(0x005a4e98) static TResourcePtr<CSprite> apAutoGroundTilesetSprite[10];
    for (tilesetNum = 0; tilesetNum < 10; tilesetNum++) {
        apAutoGroundTilesetSprite[tilesetNum] =
            TResourcePtr<CSprite>(ResourceManager::GetSprite(akGroundTilesetName[tilesetNum]));
        if (!apAutoGroundTilesetSprite[tilesetNum].get())
            throw TRuntimeError();
        assert(apAutoGroundTilesetSprite[tilesetNum]->GetNumFrames(0)
               == aGroundTilesetTraitsImp[tilesetNum].m_numTiles);
        aGroundTilesetTraitsImp[tilesetNum].m_pSprite = apAutoGroundTilesetSprite[tilesetNum].get();
    }

    VA_COMPGEN(0x004be6be, 0x14, STATIC_DTOR, apAutoRiverTilesetSprite)
    DATA(0x005a4f98) static TResourcePtr<CSprite> apAutoRiverTilesetSprite[4];
    for (tilesetNum = 0; tilesetNum < 4; tilesetNum++) {
        apAutoRiverTilesetSprite[tilesetNum] =
            TResourcePtr<CSprite>(ResourceManager::GetSprite(akRiverTilesetName[tilesetNum]));
        if (!apAutoRiverTilesetSprite[tilesetNum].get())
            throw TRuntimeError();
        aRiverTilesetTraitsImp[tilesetNum].m_pSprite = apAutoRiverTilesetSprite[tilesetNum].get();
    }

    VA_COMPGEN(0x004be6aa, 0x14, STATIC_DTOR, apAutoRoadTilesetSprite)
    DATA(0x005a4fb8) static TResourcePtr<CSprite> apAutoRoadTilesetSprite[3];
    for (tilesetNum = 0; tilesetNum < 3; tilesetNum++) {
        apAutoRoadTilesetSprite[tilesetNum] =
            TResourcePtr<CSprite>(ResourceManager::GetSprite(akRoadTilesetName[tilesetNum]));
        if (!apAutoRoadTilesetSprite[tilesetNum].get())
            throw TRuntimeError();
        aRoadTilesetTraitsImp[tilesetNum].m_pSprite = apAutoRoadTilesetSprite[tilesetNum].get();
    }
}

VA(0x004be6ee, 0xf2)
void animateTilesets(unsigned int frameNum)
{
    register const int kWaterCycle1Begin = 229;
    register const int kWaterCycle1End = 241;
    register const int kWaterCycle2Begin = 242;
    register const int kWaterCycle2End = 254;
    int waterCycle1Step = -(frameNum % (kWaterCycle1End - kWaterCycle1Begin));
    int waterCycle2Step = -(frameNum % (kWaterCycle2End - kWaterCycle2Begin));
    CSprite* pWaterSprite = aGroundTilesetTraitsImp[eTerrainWater].m_pSprite;
    pWaterSprite->ColorCycle(kWaterCycle1Begin, kWaterCycle1End - 1, waterCycle1Step);
    pWaterSprite->ColorCycle(kWaterCycle2Begin, kWaterCycle2End - 1, waterCycle2Step);

    register const int kLavaCycleBegin = 246;
    register const int kLavaCycleEnd = 255;
    int lavaCycleStep = -(frameNum % (kLavaCycleEnd - kLavaCycleBegin));
    CSprite* pLavaSprite = aGroundTilesetTraitsImp[eTerrainLava].m_pSprite;
    pLavaSprite->ColorCycle(kLavaCycleBegin, kLavaCycleEnd - 1, lavaCycleStep);

    register const int kClearRiverCycle1Begin = 183;
    register const int kClearRiverCycle1End = 195;
    register const int kClearRiverCycle2Begin = 195;
    register const int kClearRiverCycle2End = 201;
    int clearRiverCycle1Step = -(frameNum % (kClearRiverCycle1End - kClearRiverCycle1Begin));
    int clearRiverCycle2Step = -(frameNum % (kClearRiverCycle2End - kClearRiverCycle2Begin));
    CSprite* pClearRiverSprite = aRiverTilesetTraitsImp[0].m_pSprite;
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
    CSprite* pMudRiverSprite = aRiverTilesetTraitsImp[2].m_pSprite;
    pMudRiverSprite->ColorCycle(kMudRiverCycle1Begin, kMudRiverCycle1End - 1, mudRiverCycle1Step);
    pMudRiverSprite->ColorCycle(kMudRiverCycle2Begin, kMudRiverCycle2End - 1, mudRiverCycle2Step);
    pMudRiverSprite->ColorCycle(kMudRiverCycle3Begin, kMudRiverCycle3End - 1, mudRiverCycle3Step);

    register const int kLavaRiverCycleBegin = 240;
    register const int kLavaRiverCycleEnd = 249;
    int lavaRiverCycleStep = -(frameNum % (kLavaRiverCycleEnd - kLavaRiverCycleBegin));
    CSprite* pLavaRiverSprite = aRiverTilesetTraitsImp[3].m_pSprite;
    pLavaRiverSprite->ColorCycle(kLavaRiverCycleBegin, kLavaRiverCycleEnd - 1, lavaRiverCycleStep);
}
