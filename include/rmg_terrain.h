// Complete-only random-map terrain transition support.
#ifndef HOMM3_RMG_TERRAIN_H
#define HOMM3_RMG_TERRAIN_H

#include <set>
#include <vector>
#include "rmg.h"

// Retail adapter slots 1 and 4 exchange this three-dword value. The first
// two dwords are the terrain and frame fields; the low two bytes of the last
// dword are the independent sprite flips. The names in this file describe
// proven roles because the Dreamcast build has no RMG compiland.
struct TRmgTerrainTile {
    int terrain;
    int frame;
    unsigned char flipX;
    unsigned char flipY;
    char pad000a[2];
};

struct TRmgTerrainFlip {
    unsigned char flipX;
    unsigned char flipY;
};

// The cache word is decoded identically throughout the 0x5b3dd0..0x5b76f0
// retail cluster. Its constructor clears only the validity bit; the upper
// two bits survive every fill from the map adapter.
struct TRmgPackedTerrainCell {
    unsigned short initialized : 1;
    unsigned short terrain : 4;
    unsigned short frame : 7;
    unsigned short flipX : 1;
    unsigned short flipY : 1;
    unsigned short unknown14 : 2;

    TRmgPackedTerrainCell() : initialized(0) {}

    inline int GetTerrain() const { return terrain; }
    inline int GetFrame() const { return frame; }
    inline unsigned char GetFlipX() const { return flipX; }
    inline unsigned char GetFlipY() const { return flipY; }
    inline TRmgTerrainTile GetTile() const
    {
        TRmgTerrainTile tile;
        tile.terrain = GetTerrain();
        tile.frame = GetFrame();
        tile.flipX = GetFlipX();
        tile.flipY = GetFlipY();
        return tile;
    }
    inline void SetInitialized() { initialized = 1; }
    inline void SetTerrain(int value) { terrain = value; }
    inline void SetFrame(int value) { frame = value; }
    inline void SetFlipX(unsigned char value) { flipX = value; }
    inline void SetFlipY(unsigned char value) { flipY = value; }
};

// Vtable 0x642c98 fixes these six slots. Only the three methods used by the
// admitted renderer are named by role here; the concrete terrain-rule type
// and its source spellings remain unknown.
class TRmgTerrainRule {
public:
    unsigned char blendsWithOtherTerrain; // +0x04
    unsigned char opaque0005;              // +0x05
    char pad0006[2];

    virtual ~TRmgTerrainRule() {}
    virtual int HasEntries() = 0;
    virtual unsigned char IsSpecialFrame(int frame) = 0;
    virtual int GetEntry(int index) = 0;
    virtual int SelectBaseFrame(int value, int oldFrame) = 0;
    virtual int SelectTransitionFrame(
        int transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        int oldFrame) = 0;
};

extern TRmgTerrainRule* gRmgTerrainRules[];

enum TRmgTerrainTransitionCase {
    RMG_TERRAIN_FIRST_DIAGONAL_LOW = 2,
    RMG_TERRAIN_SECOND_DIAGONAL_LOW = 5,
    RMG_TERRAIN_FIRST_DIAGONAL_HIGH = 8,
    RMG_TERRAIN_SECOND_DIAGONAL_HIGH = 11
};

// Provisional role name. Allocation at 0x5b7250 proves the 0x44-byte object;
// the constructor at 0x5b45f0 proves the field order and the two Dinkumware
// point sets followed by the packed-cell vector.
class TRmgTerrainPainter {
public:
    TRmgMapAdapterInterface* adapter;               // +0x00
    int parameterA;                                 // +0x04
    int transitionStrength;                         // +0x08
    unsigned int width;                             // +0x0c
    unsigned int height;                            // +0x10
    std::set<TPoint> primaryPoints;                  // +0x14
    std::set<TPoint> secondaryPoints;                // +0x24
    std::vector<TRmgPackedTerrainCell> packedCells;  // +0x34

    TRmgTerrainPainter(
        TRmgMapAdapterInterface* newAdapter,
        int newParameterA,
        int newTransitionStrength);
    ~TRmgTerrainPainter();

    void InitializePackedCell(const TPoint& point, unsigned int index);
    TRmgPackedTerrainCell* GetPackedCell(const TPoint& point);
    // Retail repeatedly expands this field accessor while retaining the
    // nested GetPackedCell call. Keeping the source helper is therefore
    // required even though it has no separately emitted body.
    inline int GetTerrain(const TPoint& point)
    {
        return GetPackedCell(point)->GetTerrain();
    }
    void PaintTransitions();

    void BuildNeighbourKinds(const TPoint& point, int* neighbours);
    unsigned char CheckFirstDiagonal(
        const TPoint& point, const TRmgTerrainFlip& flip);
    unsigned char CheckSecondDiagonal(
        const TPoint& point, const TRmgTerrainFlip& flip);
    int GetTransitionStrength(const TPoint& point, int terrain);
};

SIZE(TRmgTerrainTile, 0x0c);
SIZE(TRmgTerrainFlip, 0x02);
SIZE(TRmgPackedTerrainCell, 0x02);
SIZE(TRmgTerrainRule, 0x08);
SIZE(TRmgTerrainPainter, 0x44);

#endif  // HOMM3_RMG_TERRAIN_H
