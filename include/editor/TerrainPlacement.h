// TerrainPlacement.h - terrain placement (Loki TerrainPlacement.cpp).
// The client interface the placement operation reports to (slot order
// from TMapDoc's thunks; its empty inline bodies are linkonce copies, and
// the sheet instantiates a plain client), and the operation itself. The
// operation's layout is its constructor's: the client, the map, the layer,
// the terrain type, the special tile frequency (all asserted by those
// parameter names) and two sets of tile points. Asserts name the invalid
// set; the other members' names and the test set's are not proven.
#ifndef HOMM3_EDITOR_TERRAINPLACEMENT_H
#define HOMM3_EDITOR_TERRAINPLACEMENT_H

#include <set>

#include "terrain_type.h"
#include "editor/GameMap.h"
#include "editor/Point.h"
#include "editor/Uncopyable.h"

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
    TGameMap::TLayer* _getPMapLayer() { return _m_pMap->getPLayer(_m_bSecondLayer); }

    void _placeTile(unsigned int x, unsigned int y);
    void _placeTile(unsigned int x, unsigned int y, TTilePoint* pTopLeft, TTilePoint* pBottomRight);
    void _validTilePlaced(unsigned int x, unsigned int y);
    void _validateTile(unsigned int x, unsigned int y, TTilePoint* pTopLeft, TTilePoint* pBottomRight);
    void _processTransitions(TTilePoint* pTopLeft, TTilePoint* pBottomRight);
    static unsigned int _computeEffectiveFrequency(unsigned int specialTileFrequency,
                                                   const TGameMap::TLayer& mapLayer, unsigned int x,
                                                   unsigned int y, TTerrainType terrainType);
    unsigned int _pickFullTile(unsigned int x, unsigned int y, TTerrainType terrainType);
    unsigned int _pickFullTile(unsigned int x, unsigned int y, TTerrainType terrainType,
                               unsigned int requestedTileNum);
    static void _repaintMapLayer(TGameMap::TLayer* pMapLayer, unsigned int specialTileFrequency);

    TTerrainPlacementOpClient* _m_pClient;
    TGameMap* _m_pMap;
    bool _m_bSecondLayer;
    TTerrainType _m_terrainType;
    unsigned int _m_specialTileFrequency;
    set<TTilePoint> _m_invalidSet;
    set<TTilePoint> _m_testSet;
};

#endif  /* HOMM3_EDITOR_TERRAINPLACEMENT_H */
