// Tile.h - the editor's tilesets (Tile.cpp; Loki h3maped object 32).
// Tile.cpp loads the ground, river and road tileset sprites into the
// traits tables below.
//
// TGroundTilesetTraits is Loki's layout, which h3maped proves again:
// initColors (0x40cba4) walks the table in 16-byte rows and calls the
// non-const CSprite::GetPalette (?GetPalette@CSprite@@QAEPAGXZ) on the
// pointer at +0, so the Windows field is a CSprite*, where Loki's is const.
// The bool's name and the per-tile traits are Loki's inferences.
#ifndef HOMM3_EDITOR_TILE_H
#define HOMM3_EDITOR_TILE_H

#include "editor/stdafx.h"

#include "va.h"

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

    CSprite* m_pSprite;
    bool m_bAnimated;
    unsigned int m_numTiles;
    const TTileTraits* m_pTileTraits;
};

// Indexed by TTerrainType.
extern const TGroundTilesetTraits* akGroundTilesetTraits;

// The map views' zoom levels: full size, half and quarter. Only kNumZooms
// is proven (Loki's ruler asserts); the saved zoom option is reset to the
// first when it is out of range.
enum TZoom {
    eZoom100,
    eZoom50,
    eZoom25,
    kNumZooms
};

// A zoom level's tile size and scale and the functions that draw a tile and
// an adventure object at it (three 24-byte rows, {32, 1, 0}, {16, 2, 1} and
// {8, 4, 2}, with Tile.cpp's drawing functions). Loki's layout; the field
// names are not proven.
struct TZoomTraits {
    typedef void (*TDrawTileFunc)(const CSprite* pSprite, unsigned int frameNum, int x, int y, uword* pBuffer,
                                  unsigned int width, unsigned int height, unsigned int pitch, bool bHFlip,
                                  bool bVFlip);
    typedef void (*TDrawAdvObjFunc)(const CSprite* pSprite, unsigned int frameNum, int srcX, int srcY,
                                    unsigned int srcWidth, unsigned int srcHeight, int x, int y, uword* pBuffer,
                                    unsigned int width, unsigned int height, unsigned int pitch,
                                    unsigned short flagColor);
    typedef void (*TDrawAdvObjShadowFunc)(const CSprite* pSprite, unsigned int frameNum, int srcX, int srcY,
                                          unsigned int srcWidth, unsigned int srcHeight, int x, int y,
                                          uword* pBuffer, unsigned int width, unsigned int height,
                                          unsigned int pitch);

    unsigned int m_tileSize;
    unsigned int m_scale;
    unsigned int m_scaleShift;
    TDrawTileFunc m_pDrawTile;
    TDrawAdvObjFunc m_pDrawAdvObj;
    TDrawAdvObjShadowFunc m_pDrawAdvObjShadow;
};

DATA(0x00543a30) extern const TZoomTraits akZoomTraits[kNumZooms];

#endif  /* HOMM3_EDITOR_TILE_H */
