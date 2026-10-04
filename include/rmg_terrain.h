// Random-map terrain transition painting.
#ifndef HOMM3_RMG_TERRAIN_H
#define HOMM3_RMG_TERRAIN_H

#include <memory>
#include <set>
#include <vector>

#include "rmg.h"
#include "rmg_terrain_data.h"

// Adds a signed direction offset; callers convert the result to a grid point.
inline TPoint operator+(const TPoint& point, const TPoint& offset)
{
    TPoint result = point;
    return result += offset;
}

// Both eight-neighbour rings (TILE_DIR_* and g_rmgDirections) alternate
// cardinal and diagonal directions; the odd entries are the diagonals.
inline bool isRmgDiagonalDirection(u32 direction)
{
    return (direction & 1) != 0;
}

// One map-layer tile as read from or written to the map adapter.
struct TRmgTerrainTile {
    // Terrain, road or river type, depending on the adapter's layer.
    s32 m_terrain;
    s32 m_frame;
    b8 m_flipX;
    b8 m_flipY;

    TRmgTerrainTile() {}
    TRmgTerrainTile(s32 newTerrain, s32 newFrame)
        : m_terrain(newTerrain), m_frame(newFrame), m_flipX(false), m_flipY(false) {}
    s32 getFrame() const { return m_frame; }
    b8 getFlipX() const { return m_flipX; }
    b8 getFlipY() const { return m_flipY; }
    TRmgTerrainTile& operator=(const TRmgTerrainTile& other)
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
    TRmgTerrainFlip(b8 flipX, b8 flipY) : m_flipX(flipX), m_flipY(flipY) {}
    const s32* getReflectedNeighbourOrder() const;
    s32 getIndex() const;
};

// Cached copy of one map tile. Only the validity bit is cleared on construction;
// the top two bits are never written.
struct TRmgPackedTerrainCell {
    u16 m_initialized : 1;
    u16 m_terrain : 4;
    u16 m_frame : 7;
    u16 m_flipX : 1;
    u16 m_flipY : 1;

    TRmgPackedTerrainCell() : m_initialized(false) {}

    inline s32 getTerrain() const { return m_terrain; }
    inline s32 getFrame() const { return m_frame; }
    inline b8 getFlipX() const { return m_flipX; }
    inline b8 getFlipY() const { return m_flipY; }
    inline TRmgTerrainTile getTile() const
    {
        TRmgTerrainTile tile;
        tile.m_terrain = getTerrain();
        tile.m_frame = getFrame();
        tile.m_flipX = getFlipX();
        tile.m_flipY = getFlipY();
        return tile;
    }
    inline void setInitialized() { m_initialized = true; }
    inline void setTerrain(s32 value) { m_terrain = value; }
    inline void setFrame(s32 value) { m_frame = value; }
    inline void setFlipX(b8 value) { m_flipX = value; }
    inline void setFlipY(b8 value) { m_flipY = value; }
    // Copies the tile payload; callers mark the cell initialized.
    inline void setTileValues(const TRmgTerrainTile& tile)
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
    b8 m_blendsWithOtherTerrain;     // +0x04
    b8 m_allowsSeparatedNeighbours;  // +0x05

    TRmgTerrainRule(b8 blendsWithOtherTerrain = false,
        b8 allowsSeparatedNeighbours = false)
        : m_blendsWithOtherTerrain(blendsWithOtherTerrain),
          m_allowsSeparatedNeighbours(allowsSeparatedNeighbours) {}
    virtual ~TRmgTerrainRule() = 0;
    virtual b8 hasSpecialBaseFrames() = 0;
    virtual b8 isSpecialFrame(s32 frame) = 0;
    virtual ERmgTerrainShape getTransition(s32 frame) = 0;
    virtual s32 selectBaseFrame(s32 strength, s32 oldFrame) = 0;
    virtual s32 selectTransitionFrame(
        ERmgTerrainShape transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        s32 oldFrame) = 0;
};

struct TRmgTerrainPatternRange {
    s32 m_firstFrame;
    u32 m_frameCount;

    TRmgTerrainPatternRange() : m_firstFrame(0), m_frameCount(0) {}
    s32 selectFrame() const;
};

DATA(0x006424a8)
extern const TRmgTerrainTransitionEntry g_rmgTerrainPatterns[];

// Frame ranges of the fixed table, keyed by transition and both flips.
struct TRmgTerrainPatternTable {
    TRmgTerrainPatternRange m_ranges[RMG_TERRAIN_SHAPE_COUNT * 2 * 2];
    TRmgTerrainPatternTable();
    TRmgTerrainPatternRange& getRange(ERmgTerrainShape transition, b8 flipX, b8 flipY);
};
DATA(0x006a4158)
extern TRmgTerrainPatternTable g_rmgTerrainPatternRanges;

// Rule driven by a per-terrain frame list; ranges are keyed by transition
// and special flag.
class TRmgPatternTerrainRule : public TRmgTerrainRule {
public:
    s32 m_specialFrameChance;                                       // +0x08: percentage at strength 8
    u32 m_entryCount;                                               // +0x0c
    const TRmgTerrainPatternEntry* m_entries;                       // +0x10
    TRmgTerrainPatternRange m_ranges[RMG_TERRAIN_SHAPE_COUNT * 2];  // +0x14

    TRmgPatternTerrainRule(b8 blendsWithOtherTerrain,
        b8 allowsSeparatedNeighbours, s32 specialFrameChance,
        u32 entryCount, const TRmgTerrainPatternEntry* entries);
    TRmgTerrainPatternRange& getRange(ERmgTerrainShape transition, b8 special);

    virtual b8 hasSpecialBaseFrames();
    virtual b8 isSpecialFrame(s32 frame);
    virtual ERmgTerrainShape getTransition(s32 frame);
    virtual s32 selectBaseFrame(s32 strength, s32 oldFrame);
    virtual s32 selectTransitionFrame(
        ERmgTerrainShape transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        s32 oldFrame);
};

// Stateless rule reading the fixed transition table.
class TRmgTableTerrainRule : public TRmgTerrainRule {
public:
    TRmgTableTerrainRule();
    virtual b8 hasSpecialBaseFrames();
    virtual b8 isSpecialFrame(s32 frame);
    virtual ERmgTerrainShape getTransition(s32 frame);
    virtual s32 selectBaseFrame(s32 strength, s32 oldFrame);
    virtual s32 selectTransitionFrame(
        ERmgTerrainShape transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        s32 oldFrame);
};

// Terrain rule per terrain type.
extern TRmgTerrainRule* const g_rmgTerrainRules[];

// Nonmatching run of a cell's neighbour ring. Its weight is how much of the
// cell's border it covers (edge-sharing cardinals 2, corner diagonals 1);
// repair fills the lightest gaps first.
struct TRmgTerrainGap {
    u32 m_weight;
    u32 m_start;
    u32 m_length;
};

// A gap cell C has other terrain x on both sides along one axis. North is up:
//   horizontal  vertical
//                   x
//     x C x         C
//                   x
enum ERmgTerrainGapAxis {
    RMG_HORIZONTAL_GAP,
    RMG_VERTICAL_GAP
};

class TRmgTerrainPainter {
public:
    TRmgMapInterface* m_adapter;                       // +0x00
    s32 m_paintTerrain;                                // +0x04
    // Brush strength (ERmgBrushStrength); each same-terrain cardinal
    // neighbour with a special frame halves it for a cell.
    s32 m_specialFrameStrength;                        // +0x08
    TRmgGridPoint m_size;                              // +0x0c
    // Paint-terrain cells whose shape still needs repair.
    std::set<TRmgGridPoint> m_repairPoints;            // +0x14
    // Other-terrain neighbours of settled cells, repainted with the paint
    // terrain if they need repair.
    std::set<TRmgGridPoint> m_otherTerrainPoints;      // +0x24
    // Lazily filled tile cache, row by row: cell (x, y) is at y * width + x.
    std::vector<TRmgPackedTerrainCell> m_packedCells;  // +0x34

    TRmgTerrainPainter(
        TRmgMapInterface* newAdapter,
        s32 terrain,
        s32 strength);
    ~TRmgTerrainPainter();

    void finish();
    s32 changeTerrain(s32 terrain, s32 strength);
    void paintRectangle(
        u32 x, u32 y,
        u32 rectangleWidth, u32 rectangleHeight);

    void initializePackedCell(const TRmgGridPoint& point, u32 index);
    TRmgPackedTerrainCell* getPackedCell(const TRmgGridPoint& point);
    s32 getTerrain(const TRmgGridPoint& point);
    s32 getFrame(const TRmgGridPoint& point);
    u32 getWidth() const;
    u32 getHeight() const;
    void paintTransitions();
    s32 selectBaseFrame(const TRmgGridPoint& point, s32 terrain, s32 oldFrame);
    void setTile(const TRmgGridPoint& point, const TRmgTerrainTile& tile);
    void paintBaseTile(const TRmgGridPoint& point);
    const s32& getPaintTerrain() const;
    b8 isPaintTerrain(const TRmgGridPoint& point);

    void paintPoint(const TRmgGridPoint& point);
    void resolveQueuedGap(const TRmgGridPoint& painted,
        s32 offsetX, s32 offsetY, ERmgTerrainGapAxis closedAxis);
    void queueOtherTerrainNeighbours(const TRmgGridPoint& point);
    void queueOtherTerrainDiagonalNeighbour(
        const TRmgGridPoint& point, s32 offsetX, s32 offsetY);
    bool tryQueueOtherTerrainCardinalNeighbour(
        const TRmgGridPoint& point, s32 offsetX, s32 offsetY);
    void repairTerrainPoint(const TRmgGridPoint& point);
    void repairTerrainGap(const TRmgGridPoint& negative,
        const TRmgGridPoint& positive, ERmgTerrainGapAxis axis);
    void countTerrainBoundary(const TRmgGridPoint& point, u32 direction,
        s32 terrain, std::vector<u8>& edgeCounts);
    b8 isHorizontalGap(const TRmgGridPoint& point, s32 terrain);
    b8 isVerticalGap(const TRmgGridPoint& point, s32 terrain);
    b8 isHorizontalGap(const TRmgGridPoint& point);
    b8 isVerticalGap(const TRmgGridPoint& point);
    b8 needsTerrainRepair(const TRmgGridPoint& point);
    b8 hasSeparatedNeighbours(const TRmgGridPoint& point);
    bool matchesTerrainAt(u32 x, u32 y, s32 terrain);
    bool matchesTerrainCorner(b8 firstSide, b8 secondSide,
        u32 x, u32 y, s32 terrain);
    void getNeighbourBounds(const TRmgGridPoint& point,
        TRmgGridPoint& northWest, TRmgGridPoint& southEast) const;
    void buildMatchingNeighbourMask(
        const TRmgGridPoint& point, b8* matches);

    ERmgTerrainNeighbourKind getTerrainNeighbourKindAt(const TRmgGridPoint& neighbour, s32 terrain);
    void buildNeighbourKinds(const TRmgGridPoint& point, ERmgTerrainNeighbourKind* neighbours);
    bool matchesTerrainAtClampedOffset(
        const TRmgGridPoint& point, const TPoint& offset, s32 terrain);
    b8 isOuterCornerOnDiagonalEdge(
        const TRmgGridPoint& point, const TRmgTerrainFlip& flip);
    b8 isInnerCornerOnDiagonalEdge(
        const TRmgGridPoint& point, const TRmgTerrainFlip& flip);
    bool hasSpecialTerrainFrameAt(const TRmgGridPoint& point,
        s32 offsetX, s32 offsetY, s32 terrain, TRmgTerrainRule* rule);
    s32 getSpecialFrameStrength(const TRmgGridPoint& point, s32 terrain);
};

// Owns a terrain painter for repeated rectangle painting.
class TRmgTerrainBrush {
public:
    std::auto_ptr<TRmgTerrainPainter> m_painter;

    TRmgTerrainBrush(TRmgMapInterface* map, s32 terrain, s32 strength);
    ~TRmgTerrainBrush();
    void changeTerrain(s32 terrain, s32 strength);
    void paintRectangle(
        u32 x, u32 y,
        u32 rectangleWidth, u32 rectangleHeight);
};

SIZE(TRmgTerrainTile, 0x0c);
SIZE(TRmgTerrainFlip, 0x02);
SIZE(TRmgPackedTerrainCell, 0x02);
SIZE(TRmgTerrainRule, 0x08);
SIZE(TRmgTerrainPainter, 0x44);
SIZE(TRmgTerrainBrush, 0x08);

#endif  // HOMM3_RMG_TERRAIN_H
