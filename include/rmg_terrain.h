// Random-map terrain transition painting.
#ifndef HOMM3_RMG_TERRAIN_H
#define HOMM3_RMG_TERRAIN_H

#include <memory>
#include <set>
#include <vector>

#include "rmg.h"

// Adds a signed direction offset; callers convert the result to a grid point.
inline TPoint operator+(const TPoint& point, const TPoint& offset)
{
    TPoint result = point;
    return result += offset;
}

// One map-layer tile as read from or written to the map adapter.
struct rmgTerrainTile {
    // Terrain, road or river type, depending on the adapter's layer.
    int m_terrain;
    int m_frame;
    b8 m_flipX;
    b8 m_flipY;

    rmgTerrainTile() {}
    rmgTerrainTile(int newTerrain, int newFrame)
        : m_terrain(newTerrain), m_frame(newFrame), m_flipX(false), m_flipY(false) {}
    int getFrame() const { return m_frame; }
    b8 getFlipX() const { return m_flipX; }
    b8 getFlipY() const { return m_flipY; }
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
    b8 m_flipX;
    b8 m_flipY;

    TRmgTerrainFlip() {}
    TRmgTerrainFlip(b8 x, b8 y) : m_flipX(x), m_flipY(y) {}
};

// No edge for equal terrain, a sand centre, or a dirt centre that blends with
// its neighbour; other blending pairs blend, remaining changes are hard edges.
enum TRmgTerrainNeighbourKind {
    RMG_NEIGHBOUR_NO_EDGE = 0,
    RMG_NEIGHBOUR_BLEND_EDGE = 1,
    RMG_NEIGHBOUR_HARD_EDGE = 2
};

// Cached copy of one map tile. Only the validity bit is cleared on construction;
// the top two bits are never written.
struct TRmgPackedTerrainCell {
    unsigned short m_initialized : 1;
    unsigned short m_terrain : 4;
    unsigned short m_frame : 7;
    unsigned short m_flipX : 1;
    unsigned short m_flipY : 1;
    unsigned short m_unknown14 : 2;

    TRmgPackedTerrainCell() : m_initialized(false) {}

    inline int getTerrain() const { return m_terrain; }
    inline int getFrame() const { return m_frame; }
    inline b8 getFlipX() const { return m_flipX; }
    inline b8 getFlipY() const { return m_flipY; }
    inline rmgTerrainTile getTile() const
    {
        rmgTerrainTile tile;
        tile.m_terrain = getTerrain();
        tile.m_frame = getFrame();
        tile.m_flipX = getFlipX();
        tile.m_flipY = getFlipY();
        return tile;
    }
    inline void setInitialized() { m_initialized = true; }
    inline void setTerrain(int value) { m_terrain = value; }
    inline void setFrame(int value) { m_frame = value; }
    inline void setFlipX(b8 value) { m_flipX = value; }
    inline void setFlipY(b8 value) { m_flipY = value; }
    // Copies the tile payload; callers mark the cell initialized.
    inline void setTileValues(const rmgTerrainTile& tile)
    {
        setTerrain(tile.m_terrain);
        setFrame(tile.m_frame);
        setFlipX(tile.m_flipX);
        setFlipY(tile.m_flipY);
    }
};

// Per-terrain frame selection rules used when painting transitions.
class TRmgTerrainRule {
public:
    b8 m_blendsWithOtherTerrain; // +0x04
    b8 m_allowsSeparatedNeighbours; // +0x05
    char m_tailPadding[2];

    TRmgTerrainRule(b8 blendsWithOtherTerrain = false,
        b8 allowsSeparatedNeighbours = false)
        : m_blendsWithOtherTerrain(blendsWithOtherTerrain),
          m_allowsSeparatedNeighbours(allowsSeparatedNeighbours) {}
    virtual ~TRmgTerrainRule() = 0;
    virtual b8 hasSpecialBaseFrames() = 0;
    virtual b8 isSpecialFrame(int frame) = 0;
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
    b8 m_special;
    char m_padding[3];
};

// Fixed transition table entry; carries flips instead of a special-frame flag.
struct TRmgTerrainTransitionEntry {
    int m_transition;
    b8 m_flipX;
    b8 m_flipY;
};
DATA(0x006424A8)
extern const TRmgTerrainTransitionEntry g_rmgTerrainPatterns[];

// Frame ranges of the fixed table, keyed by transition and both flips.
struct TRmgTerrainPatternTable {
    TRmgTerrainPatternRange m_ranges[116];
    TRmgTerrainPatternTable();
};
DATA(0x006A4158)
extern TRmgTerrainPatternTable g_rmgTerrainPatternRanges;

// Rule driven by a per-terrain frame list; ranges are keyed by transition
// and special flag.
class TRmgPatternTerrainRule : public TRmgTerrainRule {
public:
    int m_specialFrameChance;                   // +0x08: percentage at strength 8
    unsigned int m_entryCount;                  // +0x0c
    const TRmgTerrainPatternEntry* m_entries;    // +0x10
    TRmgTerrainPatternRange m_ranges[58];        // +0x14

    TRmgPatternTerrainRule(b8 blendsWithOtherTerrain,
        b8 allowsSeparatedNeighbours, int specialFrameChance,
        unsigned int entryCount, const TRmgTerrainPatternEntry* entries);

    virtual b8 hasSpecialBaseFrames();
    virtual b8 isSpecialFrame(int frame);
    virtual int getTransition(int frame);
    virtual int selectBaseFrame(int strength, int oldFrame);
    virtual int selectTransitionFrame(
        int transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        int oldFrame);
};

// Stateless rule reading the fixed transition table.
class TRmgTableTerrainRule : public TRmgTerrainRule {
public:
    TRmgTableTerrainRule();
    virtual b8 hasSpecialBaseFrames();
    virtual b8 isSpecialFrame(int frame);
    virtual int getTransition(int frame);
    virtual int selectBaseFrame(int strength, int oldFrame);
    virtual int selectTransitionFrame(
        int transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        int oldFrame);
};

// Terrain rule per terrain type.
extern TRmgTerrainRule* const g_rmgTerrainRules[];

// Nonmatching run of a cell's neighbour ring. Its weight is how much of the
// cell's border it covers (edge-sharing cardinals 2, corner diagonals 1);
// repair fills the lightest gaps first.
struct TRmgTerrainGap {
    unsigned int m_weight;
    unsigned int m_start;
    unsigned int m_length;
};

class rmgTerrainPainter {
public:
    TRmgMapInterface* m_adapter;                // +0x00
    int m_paintTerrain;                               // +0x04
    int m_specialFrameStrength;                         // +0x08
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
    b8 isPaintTerrain(const TRmgGridPoint& point);

    void paintPoint(const TRmgGridPoint& point);
    void queueOtherTerrainNeighbours(const TRmgGridPoint& point);
    void repairTerrainPoint(const TRmgGridPoint& point);
    b8 isHorizontalGap(const TRmgGridPoint& point, int terrain);
    b8 isVerticalGap(const TRmgGridPoint& point, int terrain);
    b8 isHorizontalGap(const TRmgGridPoint& point);
    b8 isVerticalGap(const TRmgGridPoint& point);
    b8 needsTerrainRepair(const TRmgGridPoint& point);
    b8 hasSeparatedNeighbours(const TRmgGridPoint& point);
    void buildMatchingNeighbourMask(
        const TRmgGridPoint& point, b8* matches);

    void buildNeighbourKinds(const TRmgGridPoint& point, int* neighbours);
    b8 checkFirstDiagonal(
        const TRmgGridPoint& point, const TRmgTerrainFlip& flip);
    b8 checkSecondDiagonal(
        const TRmgGridPoint& point, const TRmgTerrainFlip& flip);
    int getSpecialFrameStrength(const TRmgGridPoint& point, int terrain);
};

// Owns a terrain painter for repeated rectangle painting.
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
