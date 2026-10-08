// TerrainPlacement.h - terrain placement (Loki TerrainPlacement.cpp).
// Declared so far as far as TMapDoc and the map specifications sheet need
// it: the client interface the placement operation reports to (slot order
// from TMapDoc's thunks; its empty inline bodies are linkonce copies, and
// the sheet instantiates a plain client), and the operation itself. The
// operation's layout is its constructor's: the client, the map, the layer,
// the terrain type, the special tile frequency (all asserted by those
// parameter names) and two sets of tile points. The members' and the sets'
// names are not proven.
#ifndef HOMM3_EDITOR_TERRAINPLACEMENT_H
#define HOMM3_EDITOR_TERRAINPLACEMENT_H

#include <set>

#include "terrain_type.h"
#include "editor/Point.h"
#include "editor/Uncopyable.h"

class TGameMap;

class TTerrainPlacementOpClient {
public:
    virtual void onTerrainTypeChanged(bool bUnderground, unsigned int x, unsigned int y,
                                      TTerrainType newType) {}
    virtual void onTerrainUpdated(bool bUnderground, unsigned int left, unsigned int top,
                                  unsigned int width, unsigned int height) {}
};

class TTerrainPlacementOp : private TUncopyable {
public:
    static const unsigned int s_kMaxSpecialTileFrequency = 8;

    TTerrainPlacementOp(TTerrainPlacementOpClient* pClient, TGameMap* pMap, bool bSecondLayer,
                        TTerrainType terrainType, unsigned int specialTileFrequency);
    ~TTerrainPlacementOp();

    void operator()(unsigned int left, unsigned int top, unsigned int width, unsigned int height);

    static void repaintMap(TGameMap* pMap, unsigned int specialTileFrequency);

private:
    TTerrainPlacementOpClient* _m_pClient;
    TGameMap* _m_pMap;
    bool _m_bSecondLayer;
    TTerrainType _m_terrainType;
    unsigned int _m_specialTileFrequency;
    set<TPoint<unsigned int> > _m_aPlacedTiles;
    set<TPoint<unsigned int> > _m_aChangedTiles;
};

#endif  /* HOMM3_EDITOR_TERRAINPLACEMENT_H */
