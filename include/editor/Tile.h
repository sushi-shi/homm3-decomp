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

#endif  /* HOMM3_EDITOR_TILE_H */
