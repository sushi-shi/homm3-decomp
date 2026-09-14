// Complete-only random-map terrain transition support.
#ifndef HOMM3_RMG_TERRAIN_H
#define HOMM3_RMG_TERRAIN_H

#include <set>
#include <memory>
#include <vector>
#include "rmg.h"

// Grid points add tile directions through the signed TPoint: refresh and
// both paintPoints build `point + g_tileDirections[d]` as a TPoint copy, the
// retained TPoint::operator+= and a copied result, then convert back through
// the retained TRmgGridPoint(const TPoint&) constructor at 0x4fa520. The sum
// lives here with its only users; in rmg.h it perturbs rmg the same way.
inline Point operator+(const Point& point, const Point& offset)
{
    Point result = point;
    return result += offset;
}

// Retail adapter slots 1 and 4 exchange this three-dword value. The first
// two dwords are the terrain and frame fields; the low two bytes of the last
// dword are the independent sprite flips. The names in this file describe
// proven roles because the Dreamcast build has no RMG compiland.
// Prior provisional class role: TRmgTerrainTile.
struct rmgTerrainTile {
    int m_terrain;
    int m_frame;
    unsigned char m_flipX;
    unsigned char m_flipY;
    // +0x0a..0x0b are natural alignment padding, not source members.
    // Painter copies at 0x55edc0 and 0x55f350 transfer the two dwords and
    // only these two flip bytes; an explicit padding array makes copies
    // transfer data that neither retail operation owns. Prior role: pad000a.

    rmgTerrainTile() {}
    rmgTerrainTile(int newTerrain, int newFrame)
        : m_terrain(newTerrain), m_frame(newFrame), m_flipX(0), m_flipY(0) {}
    // Frame and flip accessors: the line refresh compares the current tile
    // through them so its neighbour helper keeps retail's three retained
    // calls (2026-09-12); the painters' own copies still use the fields.
    int getFrame() const { return m_frame; }
    unsigned char getFlipX() const { return m_flipX; }
    unsigned char getFlipY() const { return m_flipY; }
    // 0x55edc0 constructs its snapshot separately from adapter return values.
    // Those returns keep an implicit copy boundary: a custom copy constructor
    // changes the retained 0x5b3dd0 fill and its expanded terrain callers.
    // The output-reference wrapper 0x55f350 assigns the same four fields.
    rmgTerrainTile& operator=(const rmgTerrainTile& other)
    {
        m_terrain = other.m_terrain;
        m_frame = other.m_frame;
        m_flipX = other.m_flipX;
        m_flipY = other.m_flipY;
        return *this;
    }
};

// Before normalization (type): TRmgTerrainFlip.
#ifndef RmgTerrainFlip
#define RmgTerrainFlip TRmgTerrainFlip
#endif
struct RmgTerrainFlip {
    unsigned char m_flipX;
    unsigned char m_flipY;

    RmgTerrainFlip() {}
    RmgTerrainFlip(unsigned char x, unsigned char y) : m_flipX(x), m_flipY(y) {}
};

// BuildNeighbourKinds (0x5b68a0) returns zero for no edge, one when both
// terrain rules permit blending, and two for the remaining terrain changes.
// Before normalization (type): TRmgTerrainNeighbourKind.
#ifndef RmgTerrainNeighbourKind
#define RmgTerrainNeighbourKind TRmgTerrainNeighbourKind
#endif
enum RmgTerrainNeighbourKind {
    RMG_NEIGHBOUR_NO_EDGE = 0,
    RMG_NEIGHBOUR_BLEND_EDGE = 1,
    RMG_NEIGHBOUR_HARD_EDGE = 2
};

// The cache word is decoded identically throughout the 0x5b3dd0..0x5b76f0
// retail cluster. Its constructor clears only the validity bit; the upper
// two bits survive every fill from the map adapter.
// Before normalization (type): TRmgPackedTerrainCell.
#ifndef RmgPackedTerrainCell
#define RmgPackedTerrainCell TRmgPackedTerrainCell
#endif
struct RmgPackedTerrainCell {
    unsigned short m_initialized : 1;
    unsigned short m_terrain : 4;
    unsigned short m_frame : 7;
    unsigned short m_flipX : 1;
    unsigned short m_flipY : 1;
    unsigned short m_unknown14 : 2;

    RmgPackedTerrainCell() : m_initialized(0) {}

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
};

// Vtable 0x642c98 fixes these six slots. Only the three methods used by the
// admitted renderer are named by role here; the concrete terrain-rule type
// and its source spellings remain unknown.
// Before normalization (type): TRmgTerrainRule.
#ifndef RmgTerrainRule
#define RmgTerrainRule TRmgTerrainRule
#endif
class RmgTerrainRule {
public:
    unsigned char m_blendsWithOtherTerrain; // +0x04
    // needsTerrainRepair and repairTerrainPoint
    // consult this byte before joining separated neighbour regions.
    unsigned char m_allowsSeparatedNeighbours; // +0x05
    char m_tailPadding[2];

    RmgTerrainRule(unsigned char blendsWithOtherTerrain = 0,
        unsigned char allowsSeparatedNeighbours = 0)
        : m_blendsWithOtherTerrain(blendsWithOtherTerrain),
          m_allowsSeparatedNeighbours(allowsSeparatedNeighbours) {}
    virtual ~RmgTerrainRule();
    virtual unsigned char hasEntries() = 0;
    virtual unsigned char isSpecialFrame(int frame) = 0;
    virtual int getEntry(int index) = 0;
    virtual int selectBaseFrame(int value, int oldFrame) = 0;
    virtual int selectTransitionFrame(
        int transition,
        RmgTerrainFlip requestedFlip,
        RmgTerrainFlip& selectedFlip,
        int oldFrame) = 0;
};

// Before normalization (type): TRmgTerrainPatternRange.
#ifndef RmgTerrainPatternRange
#define RmgTerrainPatternRange TRmgTerrainPatternRange
#endif
struct RmgTerrainPatternRange {
    int m_firstIndex;
    unsigned int m_count;

    // Both table owners initialize their range arrays before the body scan.
    RmgTerrainPatternRange() : m_firstIndex(0), m_count(0) {}
};

// Before normalization (type): TRmgTerrainPatternEntry.
#ifndef RmgTerrainPatternEntry
#define RmgTerrainPatternEntry TRmgTerrainPatternEntry
#endif
struct RmgTerrainPatternEntry {
    int m_frame;
    unsigned char m_special;
    char m_padding[3];
};

// Fixed table at retail 0x6424a8; the Complete-only source name is unknown.
// Unlike the pattern rule's special-frame flag, +4/+5 here are the two
// transition flips (selector 0x5b3ae0 and range constructor 0x5b3940).
// Before normalization (type): TRmgTerrainTransitionEntry.
#ifndef RmgTerrainTransitionEntry
#define RmgTerrainTransitionEntry TRmgTerrainTransitionEntry
#endif
struct RmgTerrainTransitionEntry {
    int m_frame;
    unsigned char m_flipX;
    unsigned char m_flipY;
};
DATA(0x006424A8)
extern const RmgTerrainTransitionEntry g_rmgTerrainPatterns[];

// The table constructor at 0x5b3940 builds 116 first/count pairs from the
// fixed pattern records. The stateless table rule consumes the first pair.
// The static initializer at 0x5b3a10 passes this complete global as `this`.
// Complete-only owner spelling is provisional.
// Before normalization (type): TRmgTerrainPatternTable.
#ifndef RmgTerrainPatternTable
#define RmgTerrainPatternTable TRmgTerrainPatternTable
#endif
struct RmgTerrainPatternTable {
    RmgTerrainPatternRange m_ranges[116];
    RmgTerrainPatternTable();
};
DATA(0x006A4158)
extern RmgTerrainPatternTable g_rmgTerrainPatternRanges;

// Constructor 0x5b3780 retains the entry-array pointer at +0x10 and builds
// 58 first/count ranges at +0x14. This data-backed rule supplies vtable 0x642c98; its
// original Complete-only class name is unavailable.
// Before normalization (type): TRmgPatternTerrainRule.
#ifndef RmgPatternTerrainRule
#define RmgPatternTerrainRule TRmgPatternTerrainRule
#endif
class RmgPatternTerrainRule : public RmgTerrainRule {
public:
    int m_defaultFrame;                         // +0x08
    unsigned int m_entryCount;                  // +0x0c
    const RmgTerrainPatternEntry* m_entries;   // +0x10
    RmgTerrainPatternRange m_ranges[58];        // +0x14

    RmgPatternTerrainRule(unsigned char blendsWithOtherTerrain,
        unsigned char allowsSeparatedNeighbours, int defaultFrame,
        unsigned int entryCount, const RmgTerrainPatternEntry* entries);

    virtual ~RmgPatternTerrainRule();
    virtual unsigned char hasEntries();
    virtual unsigned char isSpecialFrame(int frame);
    virtual int getEntry(int index);
    virtual int selectBaseFrame(int value, int oldFrame);
    virtual int selectTransitionFrame(
        int transition,
        RmgTerrainFlip requestedFlip,
        RmgTerrainFlip& selectedFlip,
        int oldFrame);
};

// Vtable 0x642cb0 is the stateless, table-backed terrain-rule variant.  Its
// concrete source name is unavailable because the Dreamcast build predates
// the random-map generator; this role name follows the retail implementation,
// whose remaining slots read the fixed transition table at 0x6424a8.
// Before normalization (type): TRmgTableTerrainRule.
#ifndef RmgTableTerrainRule
#define RmgTableTerrainRule TRmgTableTerrainRule
#endif
class RmgTableTerrainRule : public RmgTerrainRule {
public:
    RmgTableTerrainRule();
    virtual unsigned char hasEntries();
    virtual unsigned char isSpecialFrame(int frame);
    virtual int getEntry(int index);
    virtual int selectBaseFrame(int value, int oldFrame);
    virtual int selectTransitionFrame(
        int transition,
        RmgTerrainFlip requestedFlip,
        RmgTerrainFlip& selectedFlip,
        int oldFrame);
};

// Retail 0x642bd8 is a pointer table in the read-only .rdata section.
extern RmgTerrainRule* const g_rmgTerrainRules[];

// RepairTerrainPoint ranks up to four disjoint runs in an eight-cell ring.
// Before normalization (type): TRmgTerrainGap.
#ifndef RmgTerrainGap
#define RmgTerrainGap TRmgTerrainGap
#endif
struct RmgTerrainGap {
    unsigned int m_weight;
    unsigned int m_start;
    unsigned int m_length;
};

// Before normalization (type): TRmgTerrainTransitionCase.
#ifndef RmgTerrainTransitionCase
#define RmgTerrainTransitionCase TRmgTerrainTransitionCase
#endif
enum RmgTerrainTransitionCase {
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
    RmgMapInterface* m_adapter;                // +0x00
    int m_paintTerrain;                               // +0x04
    int m_transitionStrength;                         // +0x08
    RmgGridPoint m_size;                             // +0x0c
    std::set<RmgGridPoint> m_primaryPoints;            // +0x14
    std::set<RmgGridPoint> m_secondaryPoints;          // +0x24
    std::vector<RmgPackedTerrainCell> m_packedCells;   // +0x34

    rmgTerrainPainter(
        RmgMapInterface* newAdapter,
        int newParameterA,
        int newTransitionStrength);
    ~rmgTerrainPainter();

    void finish();
    int changeTerrain(int terrain, int strength);
    void paintRectangle(
        unsigned int x, unsigned int y,
        unsigned int rectangleWidth, unsigned int rectangleHeight);

    void initializePackedCell(const RmgGridPoint& point, unsigned int index);
    RmgPackedTerrainCell* getPackedCell(const RmgGridPoint& point);
    int getTerrain(const RmgGridPoint& point);
    int getFrame(const RmgGridPoint& point);
    // Provisional dimension accessors inferred from paintTransitions' scalar
    // loads and inline boundaries. Unused declarations are byte-neutral;
    // the source calls restore all but one of its retained cache reads.
    unsigned int getWidth() const;
    unsigned int getHeight() const;
    void paintTransitions();
    int selectBaseFrame(const RmgGridPoint& point, int terrain, int oldFrame);
    void setTile(const RmgGridPoint& point, const rmgTerrainTile& tile);
    void paintBaseTile(const RmgGridPoint& point);
    int getPaintTerrain() const;
    unsigned char isPaintTerrain(const RmgGridPoint& point);

    void paintPoint(const RmgGridPoint& point);
    void queueOtherTerrainNeighbours(const RmgGridPoint& point);
    void repairTerrainPoint(const RmgGridPoint& point);
    unsigned char isHorizontalGap(const RmgGridPoint& point, int terrain);
    unsigned char isVerticalGap(const RmgGridPoint& point, int terrain);
    unsigned char isHorizontalGap(const RmgGridPoint& point);
    unsigned char isVerticalGap(const RmgGridPoint& point);
    unsigned char needsTerrainRepair(const RmgGridPoint& point);
    unsigned char hasSeparatedNeighbours(const RmgGridPoint& point);
    void buildMatchingNeighbourMask(
        const RmgGridPoint& point, unsigned char* matches);

    void buildNeighbourKinds(const RmgGridPoint& point, int* neighbours);
    unsigned char checkFirstDiagonal(
        const RmgGridPoint& point, const RmgTerrainFlip& flip);
    unsigned char checkSecondDiagonal(
        const RmgGridPoint& point, const RmgTerrainFlip& flip);
    int getTransitionStrength(const RmgGridPoint& point, int terrain);
};

// Provisional facade name. The ctor at 0x5b7250 initializes the exact VC6
// auto_ptr ownership byte/pointer pair; 0x5b72f0 conditionally deletes it.
// Before normalization (type): TRmgTerrainBrush.
#ifndef RmgTerrainBrush
#define RmgTerrainBrush TRmgTerrainBrush
#endif
class RmgTerrainBrush {
public:
    std::auto_ptr<rmgTerrainPainter> m_painter;

    RmgTerrainBrush(RmgMapInterface* map, int terrain, int strength);
    ~RmgTerrainBrush();
    void changeTerrain(int terrain, int strength);
    void paintRectangle(
        unsigned int x, unsigned int y,
        unsigned int rectangleWidth, unsigned int rectangleHeight);
};

SIZE(rmgTerrainTile, 0x0c);
SIZE(RmgTerrainFlip, 0x02);
SIZE(RmgPackedTerrainCell, 0x02);
SIZE(RmgTerrainRule, 0x08);
SIZE(rmgTerrainPainter, 0x44);
SIZE(RmgTerrainBrush, 0x08);

#endif  // HOMM3_RMG_TERRAIN_H
