// The line placement operations the map editor and the random-map
// generator share (the original's LinePlacement.h; Loki h3maped object
// 17 has its RoE form): the river and road painters, the line walker
// and the tile pattern tables. The editor's MapDoc.cpp builds its own
// adapters over TRiverOp::TAbstractMap and TRoadOp::TAbstractMap.
#ifndef HOMM3_LINEPLACEMENT_H
#define HOMM3_LINEPLACEMENT_H

#include "va.h"

#include "Point.h"
#include "terrainplacement.h"

// Unreflected line shapes chosen by selectRmgLinePattern; reflections supply
// the other orientations. North is up; # is a line tile.
//   END_S  END_E  NS     EW     SE     NES    ESW    CROSS
//   . . .  . . .  . # .  . . .  . . .  . # .  . . .  . # .
//   . # .  . # #  . # .  # # #  . # #  . # #  # # #  # # #
//   . # .  . . .  . # .  . . .  . # .  . # .  . # .  . # .
// SE_VARIANT is SE with the NE or SW diagonal also a line tile.
// END_S also covers an isolated tile.
enum ERmgLinePattern {
    LINE_END_S = 0,
    LINE_END_E = 1,
    LINE_NS = 2,
    LINE_EW = 3,
    LINE_SE = 4,
    LINE_SE_VARIANT = 5,
    LINE_NES = 6,
    LINE_ESW = 7,
    LINE_CROSS = 8,
    LINE_PATTERN_COUNT = 9
};

// Cinit 0x55ed70/0x55f2f0 passes a frame count and an array of frame pattern IDs to
// the retained constructor at 0x4f9be0. That constructor allocates the copied
// pattern ids, then records the first index and occurrence count for each of
// the nine pattern values.
struct TRmgLinePatternRange {
    // Role-derived names: the constructor writes index/count at an 8-byte
    // stride, not two separate nine-element arrays.
    unsigned int m_firstFrame;
    unsigned int m_frameCount;
};
SIZE(TRmgLinePatternRange, 0x8);

struct TRmgLinePatternTable {
    unsigned int m_frameCount;
    int* m_framePatterns;
    TRmgLinePatternRange m_ranges[LINE_PATTERN_COUNT];

    TRmgLinePatternTable(unsigned int frameCount, const int* framePatterns);
    ~TRmgLinePatternTable();
    unsigned int selectFrame(int pattern);
};
SIZE(TRmgLinePatternTable, 0x50);

// The generator owns the real globals. Their constructor and destructor
// remain ordinary support-library bodies, retained by both init/exit paths.
extern TRmgLinePatternTable g_rmgRiverPatternTable;
extern TRmgLinePatternTable g_rmgRoadPatternTable;

void selectRmgLinePattern(
    const bool* neighbours, const TRmgLinePatternTable* table,
    int& pattern, bool& flipX, bool& flipY);

struct TRmgLinePainterTile;

// Both painter constructors pass their first base to the same retained walker
// constructor at 0x4fa280. Its helpers dispatch the six slots and read the
// dimensions at +4/+8 (0x4fa45b/0x4fa45f and 0x4fa122/0x4fa16a).
// This common abstract prefix is a retail-derived source model; the original
// Complete-only interface spelling is unknown. It has no virtual destructor
// slot: the two painters below append slot 6 as a pure destructor.
// The map editor's RTTI lists TUncopyable below this class in both line
// operations' hierarchies, as Loki's LinePlacement.h derives it privately.
// The empty base is left out here: declaring it moves VC6's choices in
// type_random_map_generator::loadTemplates (83.21 -> 83.19).
class TMapLineFilter {
public:
    TTilePoint m_size;

    // Both retained final constructors obtain the adapter size before
    // building this base, then store their own adapter at +0xc.
    TMapLineFilter(const TTilePoint& size)
        : m_size(size)
    {
    }
    virtual TRmgLinePatternTable* getPatternTable(int value) = 0;
    virtual void setTile(const TTilePoint& point, const TRmgTerrainTile& tile) = 0;
    virtual void setLineType(const TTilePoint& point, int value) = 0;
    virtual int isBlocked(const TTilePoint& point) = 0;
    virtual void getTile(const TTilePoint& point, TRmgTerrainTile& tile) = 0;
    virtual int getLineType(const TTilePoint& point) = 0;

    TRmgLinePainterTile at(const TTilePoint& point);
    int getNeighbourLineType(const TTilePoint& point, unsigned int direction);
};

// The value returned at 0x4fa050 holds the painter and a copied coordinate.
// Caller 0x4f9f00 then dispatches through those stored members; the same
// twelve-byte record is expanded at its entry and in 0x4fa080/0x4fa3c0.
// These Complete-only names describe roles, not recovered original spellings.
struct TRmgLinePainterTile {
    TMapLineFilter* m_painter;
    TTilePoint m_point;

    TRmgLinePainterTile(TMapLineFilter* painter, const TTilePoint& point);
    int getLineType();
    void getTile(TRmgTerrainTile& tile);
    void setTile(const TRmgTerrainTile& tile);
    unsigned char isBlocked();
    void setLineType(int value);
};
SIZE(TRmgLinePainterTile, 0x0c);

// Retail clear 0x4fa080 reads an unsigned origin and extent, not the signed
// min/max bounds used for zones. The point walker builds a one-cell rectangle.
// This Complete-only role name does not assert an original class spelling.
struct TRmgGridRectangle {
    TTilePoint m_origin;
    TTilePoint m_size;

    TRmgGridRectangle(const TTilePoint& origin, const TTilePoint& size);
};
SIZE(TRmgGridRectangle, 0x10);

// Shared fastcall refresh reached by the rectangle clear and point walker.
void refreshRmgLinePoint(TMapLineFilter* painter, const TTilePoint& point);
void clearRmgLineRectangle(TMapLineFilter* painter, const TRmgGridRectangle& rectangle);

class TRiverOp : public TMapLineFilter {
public:
    // The map a river operation paints (h3maped RTTI
    // `TAbstractMap@TRiverOp`, vtable 0x53952c).
    class TAbstractMap {
    public:
        virtual ~TAbstractMap();
        virtual void setTile(
            const TTilePoint& point, const TRmgTerrainTile& tile) = 0;
        virtual void setLineType(const TTilePoint& point, int value) = 0;
        virtual TTilePoint getSize() = 0;
        virtual TRmgTerrainTile getTile(const TTilePoint& point) = 0;
        virtual int getLineType(const TTilePoint& point) = 0;
        virtual int getTerrain(const TTilePoint& point) = 0;
    };

    TAbstractMap* m_adapter;

    inline TRiverOp(TAbstractMap* newAdapter)
        : TMapLineFilter(newAdapter->getSize()), m_adapter(newAdapter)
    {
    }
    // Retail's table 0x641174 has seven slots and the seventh is _purecall,
    // where the river painter's 0x641190 has its deleting destructor: the
    // destructor is pure here, as TRmgTerrainRule's is. Its empty body is
    // still expanded in ~TRiverPlacementOp (vptr store at 0x55eda0).
    virtual ~TRiverOp() = 0;

    virtual TRmgLinePatternTable* getPatternTable(int value);
    virtual void setTile(
        const TTilePoint& point, const TRmgTerrainTile& tile);
    virtual void setLineType(const TTilePoint& point, int value);
    virtual int isBlocked(const TTilePoint& point);
    // Slot 4's caller at 0x4f9fdd pushes output first, then point. The
    // retained wrapper 0x55f350 writes through its second explicit argument;
    // unlike the adapter, this interface does not return a tile by value.
    virtual void getTile(const TTilePoint& point, TRmgTerrainTile& tile);
    virtual int getLineType(const TTilePoint& point);
};

// Inline, like the terrain map's: h3maped's map document expands it, and
// the game's river and road maps keep their implicit destructors (rmg.cpp).
MAC_ADDRESS(0x22eeec, 0x48)
inline TRiverOp::TAbstractMap::~TAbstractMap() {}

inline TRiverOp::~TRiverOp() {}

// The retained walk at 0x4fa2b0 builds two three-dword records, selects them
// by distance, then updates their coordinate and step through those pointers.
// It walks from destination back toward the stored position; unsigned bounds
// at 0x4fa2c8/0x4fa2f4 and 0x4fa30c establish the coordinate/distance types.
// This Complete-only role name is provisional, not a recovered source name.
struct TRmgLineWalkAxis {
    unsigned int m_position;
    unsigned int m_distance;
    int m_step;

    TRmgLineWalkAxis(const unsigned int& destination, const unsigned int& previous)
        : m_position(destination)
    {
        if (destination <= previous) {
            m_distance = previous - destination;
            m_step = 1;
        } else {
            m_distance = destination - previous;
            m_step = -1;
        }
    }
};
SIZE(TRmgLineWalkAxis, 0x0c);

class TRmgLineWalker {
public:
    TMapLineFilter* m_painter;
    int m_lineType;
    TTilePoint m_position;

    TRmgLineWalker(
        TMapLineFilter* newPainter,
        int newLineType,
        const TTilePoint& start);
    void drawTo(const TTilePoint& destination);
    // Shared by constructor 0x4fa280 and the two-axis walk 0x4fa2b0.
    void paintPoint(const TTilePoint& point);
};

// The editor's RTTI lists only TRiverOp's lineage as bases: the line walker
// the constructor builds at +0x10 is a member.
class TRiverPlacementOp : public TRiverOp {
public:
    TRiverPlacementOp(
        TRiverOp::TAbstractMap* newAdapter,
        int newRiverType,
        const TTilePoint& start);
    virtual ~TRiverPlacementOp();

    TRmgLineWalker m_walker;
};

// The road-building cluster at 0x548040 uses a parallel painter hierarchy.
// Its base and derived vtables at 0x6411f0/0x64120c differ from the river
// hierarchy's 0x641174/0x641190 tables, while retaining the same line-painting
// interface shape. Original Complete-only class spellings are unavailable.
class TRoadOp : public TMapLineFilter {
public:
    // The map a road operation paints (h3maped RTTI
    // `TAbstractMap@TRoadOp`, vtable 0x53956c).
    class TAbstractMap {
    public:
        virtual ~TAbstractMap();
        virtual void setTile(
            const TTilePoint& point, const TRmgTerrainTile& tile) = 0;
        virtual void setLineType(const TTilePoint& point, int value) = 0;
        virtual TTilePoint getSize() = 0;
        virtual TRmgTerrainTile getTile(const TTilePoint& point) = 0;
        virtual int getLineType(const TTilePoint& point) = 0;
        virtual int getTerrain(const TTilePoint& point) = 0;
    };

    TAbstractMap* m_adapter;

    inline TRoadOp(TAbstractMap* newAdapter)
        : TMapLineFilter(newAdapter->getSize()), m_adapter(newAdapter)
    {
    }
    // Table 0x6411f0's seventh slot is _purecall as well (0x64120c has
    // ??_GTRoadPlacementOp there).
    virtual ~TRoadOp() = 0;

    virtual TRmgLinePatternTable* getPatternTable(int value);
    virtual void setTile(
        const TTilePoint& point, const TRmgTerrainTile& tile);
    virtual void setLineType(const TTilePoint& point, int value);
    virtual int isBlocked(const TTilePoint& point);
    virtual void getTile(const TTilePoint& point, TRmgTerrainTile& tile);
    virtual int getLineType(const TTilePoint& point);
};

MAC_ADDRESS(0x22ec98, 0x48)
inline TRoadOp::TAbstractMap::~TAbstractMap() {}

inline TRoadOp::~TRoadOp() {}

// The editor's RTTI lists only TRoadOp's lineage as bases: the line walker
// the constructor builds at +0x10 is a member.
class TRoadPlacementOp : public TRoadOp {
public:
    TRoadPlacementOp(
        TRoadOp::TAbstractMap* newAdapter,
        int newRoadType,
        const TTilePoint& start);
    virtual ~TRoadPlacementOp();

    TRmgLineWalker m_walker;
};

#endif  // HOMM3_LINEPLACEMENT_H
