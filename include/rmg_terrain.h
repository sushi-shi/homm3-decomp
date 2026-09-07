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
// Prior provisional class role: TRmgTerrainTile.
struct rmgTerrainTile {
    int m_terrain; // prior role: terrain
    int m_frame; // prior role: frame
    unsigned char m_flipX; // prior role: flipX
    unsigned char m_flipY; // prior role: flipY
    char m_tailPadding[2]; // prior role: pad000a

    rmgTerrainTile() {}
    rmgTerrainTile(int newTerrain, int newFrame)
        : m_terrain(newTerrain), m_frame(newFrame), m_flipX(0), m_flipY(0) {}
};

struct TRmgTerrainFlip {
    // Before normalization: flipX.
    unsigned char m_flipX;
    // Before normalization: flipY.
    unsigned char m_flipY;

    TRmgTerrainFlip() {}
    TRmgTerrainFlip(unsigned char x, unsigned char y) : m_flipX(x), m_flipY(y) {}
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
    // Before normalization: initialized.
    unsigned short m_initialized : 1;
    // Before normalization: terrain.
    unsigned short m_terrain : 4;
    // Before normalization: frame.
    unsigned short m_frame : 7;
    // Before normalization: flipX.
    unsigned short m_flipX : 1;
    // Before normalization: flipY.
    unsigned short m_flipY : 1;
    // Before normalization: unknown14.
    unsigned short m_unknown14 : 2;

    TRmgPackedTerrainCell() : m_initialized(0) {}

    // Before normalization (function): TRmgPackedTerrainCell::GetTerrain.
    inline int getTerrain() const { return m_terrain; }
    // Before normalization (function): TRmgPackedTerrainCell::GetFrame.
    inline int getFrame() const { return m_frame; }
    // Before normalization (function): TRmgPackedTerrainCell::GetFlipX.
    inline unsigned char getFlipX() const { return m_flipX; }
    // Before normalization (function): TRmgPackedTerrainCell::GetFlipY.
    inline unsigned char getFlipY() const { return m_flipY; }
    // Before normalization (function): TRmgPackedTerrainCell::GetTile.
    inline rmgTerrainTile getTile() const
    {
        rmgTerrainTile tile;
        tile.m_terrain = getTerrain();
        tile.m_frame = getFrame();
        tile.m_flipX = getFlipX();
        tile.m_flipY = getFlipY();
        return tile;
    }
    // Before normalization (function): TRmgPackedTerrainCell::SetInitialized.
    inline void setInitialized() { m_initialized = 1; }
    // Before normalization (function): TRmgPackedTerrainCell::SetTerrain.
    inline void setTerrain(int value) { m_terrain = value; }
    // Before normalization (function): TRmgPackedTerrainCell::SetFrame.
    inline void setFrame(int value) { m_frame = value; }
    // Before normalization (function): TRmgPackedTerrainCell::SetFlipX.
    inline void setFlipX(unsigned char value) { m_flipX = value; }
    // Before normalization (function): TRmgPackedTerrainCell::SetFlipY.
    inline void setFlipY(unsigned char value) { m_flipY = value; }
};

// Vtable 0x642c98 fixes these six slots. Only the three methods used by the
// admitted renderer are named by role here; the concrete terrain-rule type
// and its source spellings remain unknown.
class TRmgTerrainRule {
public:
    // Before normalization: blendsWithOtherTerrain.
    unsigned char m_blendsWithOtherTerrain; // +0x04
    // Previously opaque0005; needsTerrainRepair and repairTerrainPoint
    // consult this byte before joining separated neighbour regions.
    unsigned char m_allowsSeparatedNeighbours; // +0x05
    char m_tailPadding[2];

    virtual ~TRmgTerrainRule();
    // Before normalization (function): TRmgTerrainRule::HasEntries.
    virtual unsigned char hasEntries() = 0;
    // Before normalization (function): TRmgTerrainRule::IsSpecialFrame.
    virtual unsigned char isSpecialFrame(int frame) = 0;
    // Before normalization (function): TRmgTerrainRule::GetEntry.
    virtual int getEntry(int index) = 0;
    // Before normalization (function): TRmgTerrainRule::SelectBaseFrame.
    virtual int selectBaseFrame(int value, int oldFrame) = 0;
    // Before normalization (function): TRmgTerrainRule::SelectTransitionFrame.
    virtual int selectTransitionFrame(
        int transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        int oldFrame) = 0;
};

struct TRmgTerrainPatternRange {
    int m_firstIndex;
    unsigned int m_count;
};

// Constructor 0x5b3780 copies its entry array and builds 58 first/count
// ranges at +0x14. This data-backed rule supplies vtable 0x642c98; its
// original Complete-only class name is unavailable.
class TRmgPatternTerrainRule : public TRmgTerrainRule {
public:
    int m_defaultFrame;                         // +0x08
    int m_entryCount;                           // +0x0c
    const int* m_entries;                       // +0x10
    TRmgTerrainPatternRange m_ranges[58];        // +0x14

    virtual ~TRmgPatternTerrainRule();
    virtual unsigned char hasEntries();
    virtual unsigned char isSpecialFrame(int frame);
    virtual int getEntry(int index);
    virtual int selectBaseFrame(int value, int oldFrame);
    virtual int selectTransitionFrame(
        int transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        int oldFrame);
};

// Vtable 0x642cb0 is the stateless, table-backed terrain-rule variant.  Its
// concrete source name is unavailable because the Dreamcast build predates
// the random-map generator; this role name follows the retail implementation,
// whose remaining slots read the fixed transition table at 0x6424a8.
class TRmgTableTerrainRule : public TRmgTerrainRule {
public:
    virtual ~TRmgTableTerrainRule();
    // Before normalization (function): TRmgTableTerrainRule::HasEntries.
    virtual unsigned char hasEntries();
    virtual unsigned char isSpecialFrame(int frame);
    virtual int getEntry(int index);
    virtual int selectBaseFrame(int value, int oldFrame);
    virtual int selectTransitionFrame(
        int transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        int oldFrame);
};

// Retail 0x642bd8 is a pointer table in the read-only .rdata section.
// Before normalization: gRmgTerrainRules.
extern TRmgTerrainRule* const g_rmgTerrainRules[];

// RepairTerrainPoint ranks up to four disjoint runs in an eight-cell ring.
struct TRmgTerrainGap {
    unsigned int m_weight;
    // Before normalization: start.
    unsigned int m_start;
    // Before normalization: length.
    unsigned int m_length;
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
    TRmgMapInterface* m_adapter;                // +0x00; prior role: adapter
    int m_paintTerrain;                               // +0x04; prior role: paintTerrain
    int m_transitionStrength;                         // +0x08; prior role: transitionStrength
    unsigned int m_width;                             // +0x0c; prior role: width
    unsigned int m_height;                            // +0x10; prior role: height
    std::set<TRmgGridPoint> m_primaryPoints;            // +0x14; prior role: primaryPoints
    std::set<TRmgGridPoint> m_secondaryPoints;          // +0x24; prior role: secondaryPoints
    std::vector<TRmgPackedTerrainCell> m_packedCells;   // +0x34; prior role: packedCells

    rmgTerrainPainter(
        TRmgMapInterface* newAdapter,
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
    // required even though it has no separately located retail body.
    // Prior provisional role: GetTerrain
    int getTerrain(const TRmgGridPoint& point);
    // Prior provisional role: PaintTransitions
    void paintTransitions();
    // Prior provisional role: SelectBaseFrame
    int selectBaseFrame(const TRmgGridPoint& point, int terrain, int oldFrame);
    // Prior provisional role: SetTile
    void setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile);
    int getPaintTerrain() const;
    unsigned char isPaintTerrain(const TRmgGridPoint& point);

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
    // Before normalization: painter.
    std::auto_ptr<rmgTerrainPainter> m_painter;

    TRmgTerrainBrush(TRmgMapInterface* map, int terrain, int strength);
    ~TRmgTerrainBrush();
    void changeTerrain(int terrain, int strength);
    void paintRectangle(
        unsigned int x, unsigned int y,
        unsigned int rectangleWidth, unsigned int rectangleHeight);
};

SIZE(rmgTerrainTile, 0x0c);
SIZE(TRmgTerrainFlip, 0x02);
SIZE(TRmgPackedTerrainCell, 0x02);
SIZE(TRmgTerrainRule, 0x08);
SIZE(rmgTerrainPainter, 0x44);
SIZE(TRmgTerrainBrush, 0x08);

#endif  // HOMM3_RMG_TERRAIN_H
