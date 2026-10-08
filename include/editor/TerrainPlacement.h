// TerrainPlacement.h - terrain placement (Loki TerrainPlacement.cpp).
// Declared so far only as far as TMapDoc needs it: the client interface
// the placement operation reports to (slot order from TMapDoc's thunks).
#ifndef HOMM3_EDITOR_TERRAINPLACEMENT_H
#define HOMM3_EDITOR_TERRAINPLACEMENT_H

#include "terrain_type.h"

class TTerrainPlacementOpClient {
public:
    virtual void onTerrainTypeChanged(bool bUnderground, unsigned int x, unsigned int y,
                                      TTerrainType newType) = 0;
    virtual void onTerrainUpdated(bool bUnderground, unsigned int left, unsigned int top,
                                  unsigned int width, unsigned int height) = 0;
};

#endif  /* HOMM3_EDITOR_TERRAINPLACEMENT_H */
