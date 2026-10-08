// Tile.h - the editor's ground tileset traits (Loki Tile.cpp).
// Only what the traits' users need so far: the layout proven by the
// inline constructor TGroundTilesetTraits(bool, unsigned, const TTileTraits*)
// (sprite pointer cleared at +0, then the bool, the tile count and the
// per-tile traits) and the assert text that names m_pSprite and
// m_numTiles. The bool's name is not recovered.
#ifndef HOMM3_EDITOR_TILE_H
#define HOMM3_EDITOR_TILE_H

class CSprite;

struct TGroundTilesetTraits {
    struct TTileTraits;

    const CSprite* m_pSprite;
    bool m_bFlag;
    unsigned int m_numTiles;
    const TTileTraits* m_pTileTraits;
};

extern const TGroundTilesetTraits* const akGroundTilesetTraits;

#endif  /* HOMM3_EDITOR_TILE_H */
