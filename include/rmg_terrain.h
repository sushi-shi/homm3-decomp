// Complete-only random-map terrain transition support.
#ifndef HOMM3_RMG_TERRAIN_H
#define HOMM3_RMG_TERRAIN_H

#include <memory>
#include <set>
#include <vector>

#include "rmg.h"


// Brush strength scales each terrain's special base-frame chance, which is
// a percentage at full strength. The generator always paints at half.
enum ERmgBrushStrength {
    RMG_FULL_BRUSH_STRENGTH = 8,
    RMG_BRUSH_STRENGTH = 4
};

// Transition identities; the diagrams are beside the classifier in rmg_terrain.cpp.
enum ERmgTerrainShape {
    SHAPE_FILL = 0,
    SHAPE_N_W_BLEND = 2,
    SHAPE_W_BLEND = 3,
    SHAPE_N_BLEND = 4,
    SHAPE_SE_BLEND = 5,
    SHAPE_N_W_DIAG_BLEND = 6,
    SHAPE_SE_DIAG_BLEND = 7,
    SHAPE_N_W_HARD = 8,
    SHAPE_W_HARD = 9,
    SHAPE_N_HARD = 10,
    SHAPE_SE_HARD = 11,
    SHAPE_N_W_DIAG_HARD = 12,
    SHAPE_SE_DIAG_HARD = 13,
    SHAPE_NW_SE_BLEND = 14,
    SHAPE_NW_BLEND_SE_HARD = 15,
    SHAPE_NW_SE_HARD = 16,
    SHAPE_E_BLEND_SW_HARD = 17,
    SHAPE_S_BLEND_NE_HARD = 18,
    SHAPE_E_BLEND_SE_HARD = 19,
    SHAPE_S_BLEND_SE_HARD = 20,
    SHAPE_E_HARD_SW_BLEND = 21,
    SHAPE_S_HARD_NE_BLEND = 22,
    SHAPE_N_W_SE_BLEND = 23,
    SHAPE_N_W_SE_HARD = 24,
    SHAPE_N_W_BLEND_SE_HARD = 25,
    SHAPE_N_W_HARD_SE_BLEND = 26,
    SHAPE_E_S_BLEND_SE_HARD = 27,
    SHAPE_E_S_BLEND_NE_SW_HARD = 28,
    RMG_TERRAIN_SHAPE_COUNT = 29
};

// No edge for equal terrain, a sand centre, or a dirt centre that blends with
// its neighbour; other blending pairs blend, remaining changes are hard edges.
enum ERmgTerrainNeighbourKind {
    RMG_NEIGHBOUR_NO_EDGE = 0,
    RMG_NEIGHBOUR_BLEND_EDGE = 1,
    RMG_NEIGHBOUR_HARD_EDGE = 2
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

struct TRmgTerrainPatternEntry {
    ERmgTerrainShape m_transition;
    b8 m_special;
};

struct TRmgTerrainTransitionEntry {
    ERmgTerrainShape m_transition;
    b8 m_flipX;
    b8 m_flipY;

    bool matches(ERmgTerrainShape transition, b8 flipX, b8 flipY) const;
};


// Grid points add tile directions through the signed TPoint: refresh and
// both paintPoints build `point + g_tileDirections[d]` as a TPoint copy, the
// retained TPoint::operator+= and a copied result, then convert back through
// the retained TRmgGridPoint(const TPoint&) constructor at 0x4fa520. The sum
// lives here with its only users; in rmg.h it perturbs rmg the same way.
inline TPoint operator+(const TPoint& point, const TPoint& offset)
{
    TPoint result = point;
    return result += offset;
}

// Retail adapter slots 1 and 4 exchange this three-dword value. The first
// two dwords are the terrain and frame fields; the low two bytes of the last
// dword are the independent sprite flips. The names in this file describe
// proven roles because the Dreamcast build has no RMG compiland.
// Prior provisional class role: TRmgTerrainTile.
struct TRmgTerrainTile {
    int m_terrain;
    int m_frame;
    unsigned char m_flipX;
    unsigned char m_flipY;
    // +0x0a..0x0b are natural alignment padding, not source members.
    // Painter copies at 0x55edc0 and 0x55f350 transfer the two dwords and
    // only these two flip bytes; an explicit padding array makes copies
    // transfer data that neither retail operation owns. Prior role: pad000a.

    TRmgTerrainTile() {}
    TRmgTerrainTile(int newTerrain, int newFrame)
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
    unsigned char m_flipX;
    unsigned char m_flipY;

    TRmgTerrainFlip() {}
    TRmgTerrainFlip(unsigned char x, unsigned char y) : m_flipX(x), m_flipY(y) {}
};

// BuildNeighbourKinds (0x5b68a0) returns zero for no edge, one when both
// terrain rules permit blending, and two for the remaining terrain changes.


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
    inline TRmgTerrainTile getTile() const
    {
        TRmgTerrainTile tile;
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
class TRmgTerrainRule {
public:
    unsigned char m_blendsWithOtherTerrain; // +0x04
    // needsTerrainRepair and repairTerrainPoint
    // consult this byte before joining separated neighbour regions.
    unsigned char m_allowsSeparatedNeighbours; // +0x05
    char m_tailPadding[2];

    TRmgTerrainRule(unsigned char blendsWithOtherTerrain = 0,
        unsigned char allowsSeparatedNeighbours = 0)
        : m_blendsWithOtherTerrain(blendsWithOtherTerrain),
          m_allowsSeparatedNeighbours(allowsSeparatedNeighbours) {}
    // Retail's base vtable at 0x642c80 has six _purecall slots. The pure
    // destructor still has its ordinary out-of-line body at 0x5b3850.
    virtual ~TRmgTerrainRule() = 0;
    virtual unsigned char hasEntries() = 0;
    virtual unsigned char isSpecialFrame(int frame) = 0;
    virtual int getEntry(int index) = 0;
    virtual int selectBaseFrame(int value, int oldFrame) = 0;
    virtual int selectTransitionFrame(
        int transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        int oldFrame) = 0;
};

struct TRmgTerrainPatternRange {
    int m_firstIndex;
    unsigned int m_count;

    // Both table owners initialize their range arrays before the body scan.
    TRmgTerrainPatternRange() : m_firstIndex(0), m_count(0) {}
};



// Fixed table at retail 0x6424a8; the Complete-only source name is unknown.
// Unlike the pattern rule's special-frame flag, +4/+5 here are the two
// transition flips (selector 0x5b3ae0 and range constructor 0x5b3940).

DATA(0x006424A8)
extern const TRmgTerrainTransitionEntry g_rmgTerrainPatterns[];

// The table constructor at 0x5b3940 builds 116 first/count pairs from the
// fixed pattern records. The stateless table rule consumes the first pair.
// The static initializer at 0x5b3a10 passes this complete global as `this`.
// Complete-only owner spelling is provisional.
struct TRmgTerrainPatternTable {
    TRmgTerrainPatternRange m_ranges[116];
    TRmgTerrainPatternTable();
};
DATA(0x006A4158)
extern TRmgTerrainPatternTable g_rmgTerrainPatternRanges;

// Constructor 0x5b3780 retains the entry-array pointer at +0x10 and builds
// 58 first/count ranges at +0x14. This data-backed rule supplies vtable 0x642c98; its
// original Complete-only class name is unavailable.
class TRmgPatternTerrainRule : public TRmgTerrainRule {
public:
    int m_defaultFrame;                         // +0x08
    unsigned int m_entryCount;                  // +0x0c
    const TRmgTerrainPatternEntry* m_entries;   // +0x10
    TRmgTerrainPatternRange m_ranges[58];        // +0x14

    TRmgPatternTerrainRule(unsigned char blendsWithOtherTerrain,
        unsigned char allowsSeparatedNeighbours, int defaultFrame,
        unsigned int entryCount, const TRmgTerrainPatternEntry* entries);

    // Implicit destruction shares the base's retained cleanup at 0x5b3850;
    // both concrete rule vtables use the deleting wrapper at 0x5b3a50.
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
    TRmgTableTerrainRule();
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
extern TRmgTerrainRule* const g_rmgTerrainRules[];

// RepairTerrainPoint ranks up to four disjoint runs in an eight-cell ring.
struct TRmgTerrainGap {
    unsigned int m_weight;
    unsigned int m_start;
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
class TRmgTerrainPainter {
public:
    TRmgMapInterface* m_adapter;                // +0x00
    int m_paintTerrain;                               // +0x04
    int m_transitionStrength;                         // +0x08
    TRmgGridPoint m_size;                             // +0x0c
    std::set<TRmgGridPoint> m_primaryPoints;            // +0x14
    std::set<TRmgGridPoint> m_secondaryPoints;          // +0x24
    std::vector<TRmgPackedTerrainCell> m_packedCells;   // +0x34

    TRmgTerrainPainter(
        TRmgMapInterface* newAdapter,
        int newParameterA,
        int newTransitionStrength);
    ~TRmgTerrainPainter();

    void finish();
    int changeTerrain(int terrain, int strength);
    void paintRectangle(
        unsigned int x, unsigned int y,
        unsigned int rectangleWidth, unsigned int rectangleHeight);

    void initializePackedCell(const TRmgGridPoint& point, unsigned int index);
    TRmgPackedTerrainCell* getPackedCell(const TRmgGridPoint& point);
    int getTerrain(const TRmgGridPoint& point);
    int getFrame(const TRmgGridPoint& point);
    // Provisional dimension accessors inferred from paintTransitions' scalar
    // loads and inline boundaries. Unused declarations are byte-neutral;
    // the source calls restore all but one of its retained cache reads.
    unsigned int getWidth() const;
    unsigned int getHeight() const;
    void paintTransitions();
    int selectBaseFrame(const TRmgGridPoint& point, int terrain, int oldFrame);
    void setTile(const TRmgGridPoint& point, const TRmgTerrainTile& tile);
    void paintBaseTile(const TRmgGridPoint& point);
    const int& getPaintTerrain() const;
    unsigned char isPaintTerrain(const TRmgGridPoint& point);

    void paintPoint(const TRmgGridPoint& point);
    void queueOtherTerrainNeighbours(const TRmgGridPoint& point);
    void repairTerrainPoint(const TRmgGridPoint& point);
    // A gap cell C has other terrain x on both sides along one axis. North is up:
    //   horizontal  vertical
    //                   x
    //     x C x         C
    //                   x
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

// Provisional facade name. The ctor at 0x5b7250 initializes the exact VC6
// auto_ptr ownership byte/pointer pair; 0x5b72f0 conditionally deletes it.
class TRmgTerrainBrush {
public:
    std::auto_ptr<TRmgTerrainPainter> m_painter;

    TRmgTerrainBrush(TRmgMapInterface* map, int terrain, int strength);
    ~TRmgTerrainBrush();
    void changeTerrain(int terrain, int strength);
    void paintRectangle(
        unsigned int x, unsigned int y,
        unsigned int rectangleWidth, unsigned int rectangleHeight);
};

SIZE(TRmgTerrainTile, 0x0c);
SIZE(TRmgTerrainFlip, 0x02);
SIZE(TRmgPackedTerrainCell, 0x02);
SIZE(TRmgTerrainRule, 0x08);
SIZE(TRmgTerrainPainter, 0x44);
SIZE(TRmgTerrainBrush, 0x08);

#endif  // HOMM3_RMG_TERRAIN_H
