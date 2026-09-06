// Complete-only random-map terrain transition support.
#ifndef HOMM3_RMG_TERRAIN_H
#define HOMM3_RMG_TERRAIN_H

#include <set>
#include <memory>
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

    TRmgTerrainFlip() {}
    TRmgTerrainFlip(unsigned char x, unsigned char y) : flipX(x), flipY(y) {}
};

// BuildNeighbourKinds (0x5b68a0) returns zero for no edge, one when both
// terrain rules permit blending, and two for the remaining terrain changes.
enum TRmgTerrainNeighbourKind {
    RMG_NEIGHBOUR_NO_EDGE = 0,
    RMG_NEIGHBOUR_BLEND_EDGE = 1,
    RMG_NEIGHBOUR_HARD_EDGE = 2
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
    unsigned char allowsSeparatedNeighbours; // +0x05
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

// Retail 0x642bd8 is a pointer table in the read-only .rdata section.
extern TRmgTerrainRule* const gRmgTerrainRules[];

// RepairTerrainPoint ranks up to four disjoint runs in an eight-cell ring.
struct TRmgTerrainGap {
    unsigned int weight;
    unsigned int start;
    unsigned int length;
};

enum TRmgTerrainTransitionCase {
    RMG_TERRAIN_FIRST_DIAGONAL_LOW = 2,
    RMG_TERRAIN_SECOND_DIAGONAL_LOW = 5,
    RMG_TERRAIN_FIRST_DIAGONAL_HIGH = 8,
    RMG_TERRAIN_SECOND_DIAGONAL_HIGH = 11
};

// Provisional role name. Allocation at 0x5b7250 proves the 0x44-byte object;
// the constructor at 0x5b45f0 proves the field order and the two Dinkumware
// point sets followed by the packed-cell vector.
// Prior provisional class role: TRmgTerrainPainter.
class rmgTerrainPainter {
public:
    TRmgMapAdapterInterface* m_adapter;                // +0x00; prior role: adapter
    int m_paintTerrain;                               // +0x04; prior role: paintTerrain
    int m_transitionStrength;                         // +0x08; prior role: transitionStrength
    unsigned int m_width;                             // +0x0c; prior role: width
    unsigned int m_height;                            // +0x10; prior role: height
    std::set<TRmgGridPoint> m_primaryPoints;            // +0x14; prior role: primaryPoints
    std::set<TRmgGridPoint> m_secondaryPoints;          // +0x24; prior role: secondaryPoints
    std::vector<TRmgPackedTerrainCell> m_packedCells;   // +0x34; prior role: packedCells

    rmgTerrainPainter(
        TRmgMapAdapterInterface* newAdapter,
        int newParameterA,
        int newTransitionStrength);
    ~rmgTerrainPainter();

    // Prior provisional role: Finish
    void finish();
    // Prior provisional role: ChangeTerrain
    void changeTerrain(int terrain, int strength);
    // Prior provisional role: PaintRectangle
    void paintRectangle(
        unsigned int x, unsigned int y,
        unsigned int rectangleWidth, unsigned int rectangleHeight);

    // Prior provisional role: InitializePackedCell
    void initializePackedCell(const TRmgGridPoint& point, unsigned int index);
    // Prior provisional role: GetPackedCell
    TRmgPackedTerrainCell* getPackedCell(const TRmgGridPoint& point);
    // Retail repeatedly expands this field accessor while retaining the
    // nested GetPackedCell call. Keeping the source helper is therefore
    // required even though it has no separately emitted body.
    // Prior provisional role: GetTerrain
    inline int getTerrain(const TRmgGridPoint& point)
    {
        return getPackedCell(point)->GetTerrain();
    }
    // Prior provisional role: PaintTransitions
    void paintTransitions();
    // Prior provisional role: SelectBaseFrame
    int selectBaseFrame(const TRmgGridPoint& point, int terrain, int oldFrame);
    // Prior provisional role: SetTile
    void setTile(const TRmgGridPoint& point, const TRmgTerrainTile& tile);
    // Prior provisional role: PaintPoint
    void paintPoint(const TRmgGridPoint& point);
    // Prior provisional role: QueueOtherTerrainNeighbours
    void queueOtherTerrainNeighbours(const TRmgGridPoint& point);
    // Prior provisional role: RepairTerrainPoint
    void repairTerrainPoint(const TRmgGridPoint& point);
    // Prior provisional role: IsHorizontalGap
    unsigned char isHorizontalGap(const TRmgGridPoint& point, int terrain);
    // Prior provisional role: IsVerticalGap
    unsigned char isVerticalGap(const TRmgGridPoint& point, int terrain);
    // Prior provisional role: IsHorizontalGap
    unsigned char isHorizontalGap(const TRmgGridPoint& point);
    // Prior provisional role: IsVerticalGap
    unsigned char isVerticalGap(const TRmgGridPoint& point);
    // Prior provisional role: NeedsTerrainRepair
    unsigned char needsTerrainRepair(const TRmgGridPoint& point);
    // Prior provisional role: HasSeparatedNeighbours
    unsigned char hasSeparatedNeighbours(const TRmgGridPoint& point);
    // Prior provisional role: BuildMatchingNeighbourMask
    void buildMatchingNeighbourMask(
        const TRmgGridPoint& point, unsigned char* matches);

    // Prior provisional role: BuildNeighbourKinds
    void buildNeighbourKinds(const TRmgGridPoint& point, int* neighbours);
    // Prior provisional role: CheckFirstDiagonal
    unsigned char checkFirstDiagonal(
        const TRmgGridPoint& point, const TRmgTerrainFlip& flip);
    // Prior provisional role: CheckSecondDiagonal
    unsigned char checkSecondDiagonal(
        const TRmgGridPoint& point, const TRmgTerrainFlip& flip);
    // Prior provisional role: GetTransitionStrength
    int getTransitionStrength(const TRmgGridPoint& point, int terrain);
};

// Provisional facade name. The ctor at 0x5b7250 initializes the exact VC6
// auto_ptr ownership byte/pointer pair; 0x5b72f0 conditionally deletes it.
class TRmgTerrainBrush {
public:
    std::auto_ptr<rmgTerrainPainter> painter;

    TRmgTerrainBrush(TRmgMapAdapterInterface* map, int terrain, int strength);
    ~TRmgTerrainBrush();
    void ChangeTerrain(int terrain, int strength);
    void PaintRectangle(
        unsigned int x, unsigned int y,
        unsigned int rectangleWidth, unsigned int rectangleHeight);
};

SIZE(TRmgTerrainTile, 0x0c);
SIZE(TRmgTerrainFlip, 0x02);
SIZE(TRmgPackedTerrainCell, 0x02);
SIZE(TRmgTerrainRule, 0x08);
SIZE(rmgTerrainPainter, 0x44);
SIZE(TRmgTerrainBrush, 0x08);

#endif  // HOMM3_RMG_TERRAIN_H
