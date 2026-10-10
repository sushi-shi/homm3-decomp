// The terrain placement operation the map editor and the random-map
// generator share (the original's TerrainPlacement.h; Loki h3maped object
// 31 has its RoE form).
//
// h3maped proves the names. Its MapDoc.cpp (0x45e326) builds its own
// `_TTerrainPlacementOp` adapter over `TTerrainPlacementOp::TAbstractMap`
// (RTTI vtables 0x53974c, then 0x53972c), allocates eight bytes and passes
// the adapter to the constructor the game spells at 0x5b7250, exactly where
// Loki's MapDoc.cpp news a TTerrainPlacementOp; MapSpecsGeneralPage and the
// generator call the same constructor and its rectangle painter. The
// painter it owns (TRmgTerrainPainter, rmg_terrain.h) is reached only
// through it; that class keeps its provisional name.
#ifndef HOMM3_TERRAINPLACEMENT_H
#define HOMM3_TERRAINPLACEMENT_H

#include "va.h"

#include <memory>

#include "homm3_int.h"
#include "Point.h"

struct TRmgTerrainTile;
class TRmgTerrainPainter;

// Provisional member names. The ctor at 0x5b7250 initializes the exact VC6
// auto_ptr ownership byte/pointer pair; 0x5b72f0 conditionally deletes it.
class TTerrainPlacementOp {
public:
    // The map a placement paints. Retail has distinct seven-slot abstract
    // tables at 0x6409e8 (this map) and 0x640a58 (the river adapter). Their
    // deleting destructors at 0x5361b0/0x537910 store those different
    // tables, so matching operation slots do not establish one base
    // identity. The painting coordinates are the unsigned grid type.
    class TAbstractMap {
    public:
        // Inline: h3maped's map document expands it in its adapters'
        // destructors (0x45e3f2, 0x45e855). The game keeps one copy for
        // its unwind actions.
        VA(0x005361A0, 0x07)
        MAC_ADDRESS(0x22d34c, 0x48)
        virtual ~TAbstractMap() {}
        virtual void setTile(
            const TTilePoint& point, const TRmgTerrainTile& tile) = 0;
        virtual void setFrame(const TTilePoint& point, int value) = 0;
        // Slot 3 returns its explicit output reference. The adapters consume
        // that returned reference, which distinguishes this from a hidden
        // value result: together the map and both adapter bodies reproduce
        // retail.
#if defined(HOMM3_TARGET_MAC)
        // Mac slot 3 (0x22eb84) returns by value.
        virtual TTilePoint getSize() = 0;
#else
        virtual TTilePoint& getSize(TTilePoint& output) = 0;
#endif
        virtual TRmgTerrainTile getTile(const TTilePoint& point) = 0;
        virtual int getTerrain(const TTilePoint& point) = 0;
        virtual int getFrame(const TTilePoint& point) = 0;
    };

    std::auto_ptr<TRmgTerrainPainter> m_painter;

    TTerrainPlacementOp(TAbstractMap* map, s32 terrain, s32 strength);
    ~TTerrainPlacementOp();
    void changeTerrain(s32 terrain, s32 strength);
    void paintRectangle(
        u32 x, u32 y,
        u32 rectangleWidth, u32 rectangleHeight);
};

#endif  // HOMM3_TERRAINPLACEMENT_H
