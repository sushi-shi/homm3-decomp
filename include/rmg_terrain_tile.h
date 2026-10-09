// The terrain, river or road tile the placement adapters exchange.
#ifndef HOMM3_RMG_TERRAIN_TILE_H
#define HOMM3_RMG_TERRAIN_TILE_H

#include "homm3_bool.h"
#include "homm3_int.h"

// Retail adapter slots 1 and 4 exchange this three-dword value. The first
// two dwords are the layer kind and frame fields: m_terrain holds terrain,
// road or river kind according to the adapter. The low two bytes of the last
// dword are the independent sprite flips. The names in this file describe
// proven roles because the Dreamcast build has no RMG compiland.
struct TRmgTerrainTile {
    s32 m_terrain;
    s32 m_frame;
    b8 m_flipX;
    b8 m_flipY;
    // +0x0a..0x0b are natural alignment padding, not source members.
    // Painter copies at 0x55edc0 and 0x55f350 transfer the two dwords and
    // only these two flip bytes; an explicit padding array makes copies
    // transfer data that neither retail operation owns. Prior role: pad000a.

    TRmgTerrainTile() {}
    TRmgTerrainTile(s32 newTerrain, s32 newFrame)
        : m_terrain(newTerrain), m_frame(newFrame), m_flipX(0), m_flipY(0) {}
    // Frame and flip accessors: the line refresh compares the current tile
    // through them so its neighbour helper keeps retail's three retained
    // calls (2026-09-12); the painters' own copies still use the fields.
    s32 getFrame() const { return m_frame; }
    b8 getFlipX() const { return m_flipX; }
    b8 getFlipY() const { return m_flipY; }
    // 0x55edc0 constructs its snapshot separately from adapter return values.
    // Those returns keep an implicit copy boundary: a custom copy constructor
    // changes the retained 0x5b3dd0 fill and its expanded terrain callers.
    // The output-reference wrapper 0x55f350 assigns the same four fields.
    TRmgTerrainTile& operator=(const TRmgTerrainTile& other)
    {
        m_terrain = other.m_terrain;
        m_frame = other.m_frame;
        m_flipX = other.m_flipX;
        m_flipY = other.m_flipY;
        return *this;
    }
};

#endif  // HOMM3_RMG_TERRAIN_TILE_H
