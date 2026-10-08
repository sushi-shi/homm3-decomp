// Tile.h - the editor's tilesets and zoom levels (Loki Tile.cpp).
// Tile.cpp loads the ground, river and road tileset sprites into the
// traits tables below and cycles the palettes of the animated ones; each
// zoom level's traits hold the tile size and the drawing functions that
// scale a tile or an adventure object to it.
//
// The layouts are proven by the image: TGroundTilesetTraits by its inline
// constructor (sprite pointer cleared at +0, then the bool, the tile count
// and the per-tile traits) and the assert texts that name m_pSprite and
// m_numTiles; the river traits (8 bytes) and road traits (4 bytes) by
// their .data tables; TZoomTraits by akZoomTraits (three 24-byte rows
// {32, 1, 0}, {16, 2, 1}, {8, 4, 2} with three function pointers each).
// The bools are true exactly for the colour-cycled tilesets (lava and
// water; the clear, mud and lava rivers), so they are named m_bAnimated.
// That name, the per-tile flag, the TZoomTraits fields and the zoom
// enumerators are not proven, except kNumZooms (the rulers' asserts).
#ifndef HOMM3_EDITOR_TILE_H
#define HOMM3_EDITOR_TILE_H

#include "editor/stdafx.h"

class CSprite;

struct TGroundTilesetTraits {
    struct TTileTraits {
        bool m_bAnimated;
    };

    TGroundTilesetTraits(bool bAnimated, unsigned int numTiles,
                         const TTileTraits* pTileTraits)
        : m_pSprite(0),
          m_bAnimated(bAnimated),
          m_numTiles(numTiles),
          m_pTileTraits(pTileTraits)
    {
    }

    const CSprite* m_pSprite;
    bool m_bAnimated;
    unsigned int m_numTiles;
    const TTileTraits* m_pTileTraits;
};

struct TRiverTilesetTraits {
    const CSprite* m_pSprite;
    bool m_bAnimated;
};

struct TRoadTilesetTraits {
    const CSprite* m_pSprite;
};

// Indexed by TTerrainType; river and road traits by type - 1.
extern const TGroundTilesetTraits* akGroundTilesetTraits;
extern const TRiverTilesetTraits* akRiverTilesetTraits;
extern const TRoadTilesetTraits* akRoadTilesetTraits;

enum TZoom {
    eZoom100,
    eZoom50,
    eZoom25,
    kNumZooms
};

struct TZoomTraits {
    typedef void (*TDrawTileFunc)(const CSprite* pSprite, unsigned int frameNum,
                                  int x, int y, uword* pBuffer,
                                  unsigned int width, unsigned int height,
                                  unsigned int pitch, bool bHFlip, bool bVFlip);
    typedef void (*TDrawAdvObjFunc)(const CSprite* pSprite, unsigned int frameNum,
                                    int srcX, int srcY,
                                    unsigned int srcWidth, unsigned int srcHeight,
                                    int x, int y, uword* pBuffer,
                                    unsigned int width, unsigned int height,
                                    unsigned int pitch, unsigned short flagColor);
    typedef void (*TDrawAdvObjShadowFunc)(const CSprite* pSprite, unsigned int frameNum,
                                          int srcX, int srcY,
                                          unsigned int srcWidth, unsigned int srcHeight,
                                          int x, int y, uword* pBuffer,
                                          unsigned int width, unsigned int height,
                                          unsigned int pitch);

    unsigned int m_tileSize;
    unsigned int m_scale;
    unsigned int m_scaleShift;
    TDrawTileFunc m_pDrawTile;
    TDrawAdvObjFunc m_pDrawAdvObj;
    TDrawAdvObjShadowFunc m_pDrawAdvObjShadow;
};

extern const TZoomTraits akZoomTraits[3];

void loadTilesets();
void animateTilesets(unsigned int frameNum);

#endif  /* HOMM3_EDITOR_TILE_H */
