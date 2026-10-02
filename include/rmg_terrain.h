// Complete-only random-map terrain transition support.
#ifndef HOMM3_RMG_TERRAIN_H
#define HOMM3_RMG_TERRAIN_H

#include <memory>
#include <set>
#include <vector>

#include "rmg.h"

// Grid-direction addition preserves the signed TPoint copy/compound-add
// before conversion through TRmgGridPoint(const TPoint&) at 0x4fa520.
inline TPoint operator+(const TPoint& point, const TPoint& offset)
{
    TPoint result = point;
    return result += offset;
}

// Adapter slots 1 and 4 exchange two ints and two flip bytes. Names are
// role-derived: Dreamcast has no RMG compiland.
struct rmgTerrainTile {
    // Kind of the adapter-selected layer: terrain, road or river.
    int m_terrain;
    int m_frame;
    unsigned char m_flipX;
    unsigned char m_flipY;
    // +0x0a..0x0b are natural padding. Retail copies only the named fields.

    rmgTerrainTile() {}
    rmgTerrainTile(int newTerrain, int newFrame)
        : m_terrain(newTerrain), m_frame(newFrame), m_flipX(0), m_flipY(0) {}
    int getFrame() const { return m_frame; }
    unsigned char getFlipX() const { return m_flipX; }
    unsigned char getFlipY() const { return m_flipY; }
    // Keep implicit copy construction for adapter returns; 0x55edc0 constructs
    // a separate snapshot, while the output-reference wrapper assigns fields.
    rmgTerrainTile& operator=(const rmgTerrainTile& other)
    {
        m_terrain = other.m_terrain;
        m_frame = other.m_frame;
        m_flipX = other.m_flipX;
        m_flipY = other.m_flipY;
        return *this;
    }
};

struct TRmgTerrainFlip {
    unsigned char m_flipX;
    unsigned char m_flipY;

    TRmgTerrainFlip() {}
    TRmgTerrainFlip(unsigned char x, unsigned char y) : m_flipX(x), m_flipY(y) {}
};

// BuildNeighbourKinds (0x5b68a0) returns zero for equal terrain or a sand
// centre, and also for a dirt centre when both rules permit blending. Other
// mutually blending pairs produce one; remaining terrain changes produce two.
enum TRmgTerrainNeighbourKind {
    RMG_NEIGHBOUR_NO_EDGE = 0,
    RMG_NEIGHBOUR_BLEND_EDGE = 1,
    RMG_NEIGHBOUR_HARD_EDGE = 2
};

// The cache word is decoded identically throughout the 0x5b3dd0..0x5b76f0
// retail cluster. Its constructor clears only the validity bit; the upper
// two bits survive every fill from the map adapter.
struct TRmgPackedTerrainCell {
    unsigned short m_initialized : 1;
    unsigned short m_terrain : 4;
    unsigned short m_frame : 7;
    unsigned short m_flipX : 1;
    unsigned short m_flipY : 1;
    unsigned short m_unknown14 : 2;

    TRmgPackedTerrainCell() : m_initialized(0) {}

    inline int getTerrain() const { return m_terrain; }
    inline int getFrame() const { return m_frame; }
    inline unsigned char getFlipX() const { return m_flipX; }
    inline unsigned char getFlipY() const { return m_flipY; }
    inline rmgTerrainTile getTile() const
    {
        rmgTerrainTile tile;
        tile.m_terrain = getTerrain();
        tile.m_frame = getFrame();
        tile.m_flipX = getFlipX();
        tile.m_flipY = getFlipY();
        return tile;
    }
    inline void setInitialized() { m_initialized = 1; }
    inline void setTerrain(int value) { m_terrain = value; }
    inline void setFrame(int value) { m_frame = value; }
    inline void setFlipX(unsigned char value) { m_flipX = value; }
    inline void setFlipY(unsigned char value) { m_flipY = value; }
    // Copy only the tile payload. Callers own validity timing: cache fills
    // mark initialized afterward, while adapter writes mark it beforehand.
    inline void setTileValues(const rmgTerrainTile& tile)
    {
        setTerrain(tile.m_terrain);
        setFrame(tile.m_frame);
        setFlipX(tile.m_flipX);
        setFlipY(tile.m_flipY);
    }
};

// Vtable 0x642c98 fixes these six slots; source names remain unknown.
class TRmgTerrainRule {
public:
    unsigned char m_blendsWithOtherTerrain; // +0x04
    unsigned char m_allowsSeparatedNeighbours; // +0x05
    char m_tailPadding[2];

    TRmgTerrainRule(unsigned char blendsWithOtherTerrain = 0,
        unsigned char allowsSeparatedNeighbours = 0)
        : m_blendsWithOtherTerrain(blendsWithOtherTerrain),
          m_allowsSeparatedNeighbours(allowsSeparatedNeighbours) {}
    // Retail's base vtable at 0x642c80 has six _purecall slots. The pure
    // destructor still has its ordinary out-of-line body at 0x5b3850.
    virtual ~TRmgTerrainRule() = 0;
    virtual unsigned char hasSpecialBaseFrames() = 0;
    virtual unsigned char isSpecialFrame(int frame) = 0;
    virtual int getTransition(int frame) = 0;
    virtual int selectBaseFrame(int strength, int oldFrame) = 0;
    virtual int selectTransitionFrame(
        int transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        int oldFrame) = 0;
};

struct TRmgTerrainPatternRange {
    int m_firstIndex;
    unsigned int m_count;

    TRmgTerrainPatternRange() : m_firstIndex(0), m_count(0) {}
};

struct TRmgTerrainPatternEntry {
    int m_transition;
    unsigned char m_special;
    char m_padding[3];
};

// Table 0x6424a8 uses two flip bytes at +4/+5, unlike the pattern rule's
// special-frame flag.
struct TRmgTerrainTransitionEntry {
    int m_transition;
    unsigned char m_flipX;
    unsigned char m_flipY;
};
DATA(0x006424A8)
extern const TRmgTerrainTransitionEntry g_rmgTerrainPatterns[];

// Constructor 0x5b3940 builds 116 ranges. Cinit 0x5b3a10 passes this global
// as `this`; the stateless rule consumes its first pair.
struct TRmgTerrainPatternTable {
    TRmgTerrainPatternRange m_ranges[116];
    TRmgTerrainPatternTable();
};
DATA(0x006A4158)
extern TRmgTerrainPatternTable g_rmgTerrainPatternRanges;

// Constructor 0x5b3780 retains the entries and builds 58 ranges; this rule
// supplies vtable 0x642c98.
class TRmgPatternTerrainRule : public TRmgTerrainRule {
public:
    int m_specialFrameChance;                   // +0x08: percentage at strength 8
    unsigned int m_entryCount;                  // +0x0c
    const TRmgTerrainPatternEntry* m_entries;    // +0x10
    TRmgTerrainPatternRange m_ranges[58];        // +0x14

    TRmgPatternTerrainRule(unsigned char blendsWithOtherTerrain,
        unsigned char allowsSeparatedNeighbours, int specialFrameChance,
        unsigned int entryCount, const TRmgTerrainPatternEntry* entries);

    // Implicit destruction shares the base's retained cleanup at 0x5b3850;
    // both concrete rule vtables use the deleting wrapper at 0x5b3a50.
    virtual unsigned char hasSpecialBaseFrames();
    virtual unsigned char isSpecialFrame(int frame);
    virtual int getTransition(int frame);
    virtual int selectBaseFrame(int strength, int oldFrame);
    virtual int selectTransitionFrame(
        int transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        int oldFrame);
};

// Stateless rule, vtable 0x642cb0; reads the fixed table at 0x6424a8.
class TRmgTableTerrainRule : public TRmgTerrainRule {
public:
    TRmgTableTerrainRule();
    virtual unsigned char hasSpecialBaseFrames();
    virtual unsigned char isSpecialFrame(int frame);
    virtual int getTransition(int frame);
    virtual int selectBaseFrame(int strength, int oldFrame);
    virtual int selectTransitionFrame(
        int transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        int oldFrame);
};

// Retail 0x642bd8 is a pointer table in the read-only .rdata section.
extern TRmgTerrainRule* const g_rmgTerrainRules[];

// RepairTerrainPoint ranks up to four disjoint runs in an eight-cell ring.
struct TRmgTerrainGap {
    unsigned int m_weight;
    unsigned int m_start;
    unsigned int m_length;
};

enum TRmgTerrainTransitionCase {
    RMG_TERRAIN_FIRST_DIAGONAL_BLEND = 2,
    RMG_TERRAIN_SECOND_DIAGONAL_BLEND = 5,
    RMG_TERRAIN_FIRST_DIAGONAL_HARD = 8,
    RMG_TERRAIN_SECOND_DIAGONAL_HARD = 11
};

// Allocation at 0x5b7250 proves size 0x44; constructor 0x5b45f0 proves
// the field order, including two point sets followed by the packed-cell vector.
class rmgTerrainPainter {
public:
    TRmgMapInterface* m_adapter;                // +0x00
    int m_paintTerrain;                               // +0x04
    int m_transitionStrength;                         // +0x08
    TRmgGridPoint m_size;                             // +0x0c
    std::set<TRmgGridPoint> m_primaryPoints;            // +0x14
    std::set<TRmgGridPoint> m_secondaryPoints;          // +0x24
    std::vector<TRmgPackedTerrainCell> m_packedCells;   // +0x34

    rmgTerrainPainter(
        TRmgMapInterface* newAdapter,
        int terrain,
        int strength);
    ~rmgTerrainPainter();

    void finish();
    int changeTerrain(int terrain, int strength);
    void paintRectangle(
        unsigned int x, unsigned int y,
        unsigned int rectangleWidth, unsigned int rectangleHeight);

    void initializePackedCell(const TRmgGridPoint& point, unsigned int index);
    TRmgPackedTerrainCell* getPackedCell(const TRmgGridPoint& point);
    int getTerrain(const TRmgGridPoint& point);
    int getFrame(const TRmgGridPoint& point);
    unsigned int getWidth() const;
    unsigned int getHeight() const;
    void paintTransitions();
    int selectBaseFrame(const TRmgGridPoint& point, int terrain, int oldFrame);
    void setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile);
    void paintBaseTile(const TRmgGridPoint& point);
    const int& getPaintTerrain() const;
    unsigned char isPaintTerrain(const TRmgGridPoint& point);

    void paintPoint(const TRmgGridPoint& point);
    void queueOtherTerrainNeighbours(const TRmgGridPoint& point);
    void repairTerrainPoint(const TRmgGridPoint& point);
    unsigned char isHorizontalGap(const TRmgGridPoint& point, int terrain);
    unsigned char isVerticalGap(const TRmgGridPoint& point, int terrain);
    unsigned char isHorizontalGap(const TRmgGridPoint& point);
    unsigned char isVerticalGap(const TRmgGridPoint& point);
    unsigned char needsTerrainRepair(const TRmgGridPoint& point);
    unsigned char hasSeparatedNeighbours(const TRmgGridPoint& point);
    void buildMatchingNeighbourMask(
        const TRmgGridPoint& point, unsigned char* matches);

    void buildNeighbourKinds(const TRmgGridPoint& point, int* neighbours);
    unsigned char checkFirstDiagonal(
        const TRmgGridPoint& point, const TRmgTerrainFlip& flip);
    unsigned char checkSecondDiagonal(
        const TRmgGridPoint& point, const TRmgTerrainFlip& flip);
    int getTransitionStrength(const TRmgGridPoint& point, int terrain);
};

// Constructor 0x5b7250 and destructor 0x5b72f0 prove auto_ptr ownership.
class TRmgTerrainBrush {
public:
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
