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

// Cell transition shapes, unreflected (flipX/flipY give the rest). Names list
// the other-terrain neighbours, each run followed by its edge kind: N_W = both
// sides (outer corner), SE = that diagonal only, DIAG = corner on a 45-degree
// edge. Id 1 is unused. Diagrams: docs/reference/rmg-terrain-shapes.md
// The basic blend shapes as their main rules find them (hard = blend + 6).
// North is up; C is the cell, e an edge of its kind, . none, ? not fixed.
//   N_W    W      N      SE
//   ? e ?  ? . .  ? e ?  . . ?
//   e C ?  e C ?  . C .  . C .
//   ? ? .  ? . .  . ? .  ? . e
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

// The diagonal-neighbour probes clamp through a reference selector of their
// own. Mac 0x258f18 keeps its operand order: value against minimum, then
// value against maximum (cmpw value,maximum; ble), whereas the shared
// includes.h tLimit (Mac 0x14cecc) tests maximum against value. Retail x86
// shows the same split: cmp value,maximum; jg here, cmp maximum,value; jl
// in tLimit's expansions.
template <class T>
inline const T& clampToRange(const T& minimum, const T& value,
                             const T& maximum)
{
    if (value < minimum) {
        return minimum;
    } else if (value > maximum) {
        return maximum;
    } else {
        return value;
    }
}

// Retail adapter slots 1 and 4 exchange this three-dword value. The first
// two dwords are the layer kind and frame fields: m_terrain holds terrain,
// road or river kind according to the adapter. The low two bytes of the last
// dword are the independent sprite flips. The names in this file describe
// proven roles because the Dreamcast build has no RMG compiland.
struct TRmgTerrainTile {
    s32 m_terrain;
    s32 m_frame;
    b8 m_flipX;
    b8 m_flipY;
    // +0x0a..0x0b are natural alignment padding, not source members.
    // Painter copies at 0x55edc0 and 0x55f350 transfer the two dwords and
    // only these two flip bytes; an explicit padding array makes copies
    // transfer data that neither retail operation owns. Prior role: pad000a.

    TRmgTerrainTile() {}
    TRmgTerrainTile(s32 newTerrain, s32 newFrame)
        : m_terrain(newTerrain), m_frame(newFrame), m_flipX(0), m_flipY(0) {}
    // Frame and flip accessors: the line refresh compares the current tile
    // through them so its neighbour helper keeps retail's three retained
    // calls (2026-09-12); the painters' own copies still use the fields.
    s32 getFrame() const { return m_frame; }
    b8 getFlipX() const { return m_flipX; }
    b8 getFlipY() const { return m_flipY; }
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
    b8 m_flipX;
    b8 m_flipY;

    TRmgTerrainFlip() {}
    TRmgTerrainFlip(b8 x, b8 y) : m_flipX(x), m_flipY(y) {}
};

// The cache word is decoded identically throughout the 0x5b3dd0..0x5b76f0
// retail cluster. Its constructor clears only the validity bit; the upper
// two bits survive every fill from the map adapter.
struct TRmgPackedTerrainCell {
    u16 m_initialized : 1;
    u16 m_terrain : 4;
    u16 m_frame : 7;
    u16 m_flipX : 1;
    u16 m_flipY : 1;
    u16 m_unknown14 : 2;

    TRmgPackedTerrainCell() : m_initialized(0) {}

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
    inline void setInitialized() { m_initialized = 1; }
    inline void setTerrain(s32 value) { m_terrain = value; }
    inline void setFrame(s32 value) { m_frame = value; }
    inline void setFlipX(b8 value) { m_flipX = value; }
    inline void setFlipY(b8 value) { m_flipY = value; }
};

// Vtable 0x642c98 fixes these six slots. Only the three methods used by the
// admitted renderer are named by role here; the concrete terrain-rule type
// and its source spellings remain unknown.
class TRmgTerrainRule {
public:
    b8 m_blendsWithOtherTerrain; // +0x04
    // needsTerrainRepair and repairTerrainPoint
    // consult this byte before joining separated neighbour regions.
    b8 m_allowsSeparatedNeighbours; // +0x05
    char m_tailPadding[2];

    TRmgTerrainRule(b8 blendsWithOtherTerrain = 0,
        b8 allowsSeparatedNeighbours = 0)
        : m_blendsWithOtherTerrain(blendsWithOtherTerrain),
          m_allowsSeparatedNeighbours(allowsSeparatedNeighbours) {}
    // Retail's base vtable at 0x642c80 has six _purecall slots. The pure
    // destructor still has its ordinary out-of-line body at 0x5b3850.
    virtual ~TRmgTerrainRule() = 0;
    virtual b8 hasSpecialBaseFrames() = 0;
    virtual b8 isSpecialFrame(s32 frame) = 0;
    virtual s32 getTransition(s32 frame) = 0;
    virtual s32 selectBaseFrame(s32 strength, s32 oldFrame) = 0;
    virtual s32 selectTransitionFrame(
        s32 transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        s32 oldFrame) = 0;
};

struct TRmgTerrainPatternRange {
    s32 m_firstFrame;
    u32 m_frameCount;

    // Both table owners initialize their range arrays before the body scan.
    TRmgTerrainPatternRange() : m_firstFrame(0), m_frameCount(0) {}
};



// Fixed table at retail 0x6424a8; the Complete-only source name is unknown.
// Unlike the pattern rule's special-frame flag, +4/+5 here are the two
// transition flips (selector 0x5b3ae0 and range constructor 0x5b3940).

DATA(0x006424a8)
extern const TRmgTerrainTransitionEntry g_rmgTerrainPatterns[];

// The table constructor at 0x5b3940 builds 116 first/count pairs from the
// fixed pattern records. The stateless table rule consumes the first pair.
// The static initializer at 0x5b3a10 passes this complete global as `this`.
// Complete-only owner spelling is provisional.
struct TRmgTerrainPatternTable {
    TRmgTerrainPatternRange m_ranges[RMG_TERRAIN_SHAPE_COUNT * 2 * 2];
    TRmgTerrainPatternTable();
};
DATA(0x006a4158)
extern TRmgTerrainPatternTable g_rmgTerrainPatternRanges;

// Constructor 0x5b3780 retains the entry-array pointer at +0x10 and builds
// 58 first/count ranges at +0x14. This data-backed rule supplies vtable 0x642c98; its
// original Complete-only class name is unavailable.
class TRmgPatternTerrainRule : public TRmgTerrainRule {
public:
    s32 m_specialFrameChance;                  // +0x08: percentage at strength 8
    u32 m_entryCount;                  // +0x0c
    const TRmgTerrainPatternEntry* m_entries;   // +0x10
    TRmgTerrainPatternRange m_ranges[RMG_TERRAIN_SHAPE_COUNT * 2];        // +0x14

    TRmgPatternTerrainRule(b8 blendsWithOtherTerrain,
        b8 allowsSeparatedNeighbours, s32 specialFrameChance,
        u32 entryCount, const TRmgTerrainPatternEntry* entries);

    // Implicit destruction shares the base's retained cleanup at 0x5b3850;
    // both concrete rule vtables use the deleting wrapper at 0x5b3a50.
    virtual b8 hasSpecialBaseFrames();
    virtual b8 isSpecialFrame(s32 frame);
    virtual s32 getTransition(s32 frame);
    virtual s32 selectBaseFrame(s32 strength, s32 oldFrame);
    virtual s32 selectTransitionFrame(
        s32 transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        s32 oldFrame);
};

// Vtable 0x642cb0 is the stateless, table-backed terrain-rule variant.  Its
// concrete source name is unavailable because the Dreamcast build predates
// the random-map generator; this role name follows the retail implementation,
// whose remaining slots read the fixed transition table at 0x6424a8.
class TRmgTableTerrainRule : public TRmgTerrainRule {
public:
    TRmgTableTerrainRule();
    virtual b8 hasSpecialBaseFrames();
    virtual b8 isSpecialFrame(s32 frame);
    virtual s32 getTransition(s32 frame);
    virtual s32 selectBaseFrame(s32 strength, s32 oldFrame);
    virtual s32 selectTransitionFrame(
        s32 transition,
        TRmgTerrainFlip requestedFlip,
        TRmgTerrainFlip& selectedFlip,
        s32 oldFrame);
};

// Retail 0x642bd8 is a pointer table in the read-only .rdata section.
extern TRmgTerrainRule* const g_rmgTerrainRules[];

// RepairTerrainPoint ranks up to four disjoint runs in an eight-cell ring.
struct TRmgTerrainGap {
    u32 m_weight;
    u32 m_start;
    u32 m_length;
};

// Provisional role name. Allocation at 0x5b7250 proves the 0x44-byte object;
// the constructor at 0x5b45f0 proves the field order and the two Dinkumware
// point sets followed by the packed-cell vector.
// Prior provisional class role: TRmgTerrainPainter.
class TRmgTerrainPainter {
public:
    TTerrainPlacementOp::TAbstractMap* m_adapter;                // +0x00
    s32 m_paintTerrain;                               // +0x04
    s32 m_transitionStrength;                         // +0x08
    TRmgGridPoint m_size;                             // +0x0c
    // Cells of the painted terrain that may need gap repair.
    std::set<TRmgGridPoint> m_repairPoints;            // +0x14
    // Adjacent cells of another terrain, checked after repair.
    std::set<TRmgGridPoint> m_otherTerrainPoints;          // +0x24
    std::vector<TRmgPackedTerrainCell> m_packedCells;   // +0x34

    TRmgTerrainPainter(
        TTerrainPlacementOp::TAbstractMap* newAdapter,
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
    // Provisional dimension accessors inferred from paintTransitions' scalar
    // loads and inline boundaries. Unused declarations are byte-neutral;
    // the source calls restore all but one of its retained cache reads.
    u32 getWidth() const;
    u32 getHeight() const;
    void paintTransitions();
    s32 selectBaseFrame(const TRmgGridPoint& point, s32 terrain, s32 oldFrame);
    void setTile(const TRmgGridPoint& point, const TRmgTerrainTile& tile);
    void paintBaseTile(const TRmgGridPoint& point);
    const s32& getPaintTerrain() const;
    b8 isPaintTerrain(const TRmgGridPoint& point);

    void paintPoint(const TRmgGridPoint& point);
    void queueOtherTerrainNeighbours(const TRmgGridPoint& point);
    void repairTerrainPoint(const TRmgGridPoint& point);
    // A gap cell C has other terrain x on both sides along one axis. North is up:
    //   horizontal  vertical
    //                   x
    //     x C x         C
    //                   x
    b8 isHorizontalGap(const TRmgGridPoint& point, s32 terrain);
    b8 isVerticalGap(const TRmgGridPoint& point, s32 terrain);
    b8 isHorizontalGap(const TRmgGridPoint& point);
    b8 isVerticalGap(const TRmgGridPoint& point);
    b8 needsTerrainRepair(const TRmgGridPoint& point);
    b8 hasSeparatedNeighbours(const TRmgGridPoint& point);
    void buildMatchingNeighbourMask(
        const TRmgGridPoint& point, b8* matches);

    void buildNeighbourKinds(const TRmgGridPoint& point, s32* neighbours);
    b8 hasMatchingDiagonalNeighbour(
        const TRmgGridPoint& point, const TRmgTerrainFlip& flip);
    b8 hasDifferentOuterAxisNeighbour(
        const TRmgGridPoint& point, const TRmgTerrainFlip& flip);
    s32 getTransitionStrength(const TRmgGridPoint& point, s32 terrain);
};

SIZE(TRmgTerrainTile, 0x0c);
SIZE(TRmgTerrainFlip, 0x02);
SIZE(TRmgPackedTerrainCell, 0x02);
SIZE(TRmgTerrainRule, 0x08);
SIZE(TRmgTerrainPainter, 0x44);
SIZE(TTerrainPlacementOp, 0x08);

#endif  // HOMM3_RMG_TERRAIN_H
