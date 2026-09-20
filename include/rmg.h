// Complete-only random-map generator declarations.
#ifndef HOMM3_RMG_H
#define HOMM3_RMG_H

#include <bitset>
#include <string>
#include <vector>
#include <va.h>
#include "terrain_type.h"
#include "advmgr_objects.h"

class TAbstractFile;
class TSpreadsheetResource;
class type_random_map_generator;
struct TRmgTownSlot;
struct TRmgZone;
struct rmgTerrainTile;
struct TPoint;
struct TObjectType;
struct TRmgObjectPropertiesRef;
class type_object;

// The abstract progress sink driven by Complete's random-map generator.
// Retail constructor 0x530e20 stores vtable 0x6409c0, the step total at +4,
// and zero at +8. The vtable holds a scalar deleting destructor at 0x530e40,
// SetTotal at 0x530e80, and _purecall in the Advance slot.
class TProgressSink {
public:
    int m_steps;
    int m_done;

    TProgressSink(int totalSteps);
    virtual ~TProgressSink();
    virtual void setTotal(int totalSteps);
    virtual void advance(int amount) = 0;
};
SIZE(TProgressSink, 0xc);

// Complete's random-map object factories share this five-dword prefix.  The
// constructor at 0x534160 writes the four fields, while vtable 0x640b64 proves
// three virtual operations: an object factory taking three arguments, a
// two-argument value query, and a parameterless boolean property.  The method
// names remain role descriptions until retail-era source identifies their
// original spelling; their boundaries and arities are retail-byte facts.
class type_treasure_def {
public:
    int m_objectType;
    int m_subtype;
    int m_value;
    int m_density;

    type_treasure_def(int objectType, int subtype, int value, int density);

    // Shared caller 0x5464cd..0x5464de passes the selected property reference,
    // generator and zone, then consumes the result as a type_object pointer.
    // This replaces the earlier placeholder void*/int/int factory signature.
    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
    virtual unsigned char isTerrainDependent();
};

SIZE(type_treasure_def, 0x14);

// These identities come from the contiguous cross-build vtable roster.  The
// current-image constructor relocations independently fix each table address.
class type_shrine_def : public type_treasure_def {
public:
    type_shrine_def(int objectType, int value);
    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_witch_hut_def : public type_treasure_def {
public:
    type_witch_hut_def();
    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_spell_scroll_def : public type_treasure_def {
public:
    int m_spellLevel;

    type_spell_scroll_def(int spellLevel, int value);
    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_black_box_creature_def : public type_treasure_def {
public:
    int m_creatureType;
    int m_adjustedValue;

    type_black_box_creature_def(int creatureType);
    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
};

// The initializer at 0x538b10 expands these small constructors at their
// source-level `new` sites.  They stay inline here even though VC6 later
// stops expanding parts of the base-constructor and vector::push_back chains:
// the retained call pattern is a caller-specific /Ob2 decision, not license
// to erase the original helper boundaries.
class type_artifact_def : public type_treasure_def {
public:
    inline type_artifact_def(int objectType, int value)
        : type_treasure_def(objectType, 0, value, 150)
    {
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_black_box_experience_def : public type_treasure_def {
public:
    int m_experience;

    inline type_black_box_experience_def(int value, int experience)
        : type_treasure_def(6, 0, value, 20)
    {
        this->m_experience = experience;
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_black_box_gold_def : public type_treasure_def {
public:
    int m_gold;

    inline type_black_box_gold_def(int value, int gold)
        : type_treasure_def(6, 0, value, 5)
    {
        this->m_gold = gold;
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_black_box_spells_def : public type_treasure_def {
public:
    int m_minimumLevel;
    int m_maximumLevel;
    int m_schoolMask;

    inline type_black_box_spells_def(
        int value, int minimumLevel, int maximumLevel, int schoolMask)
        : type_treasure_def(6, 0, value, 2)
    {
        this->m_minimumLevel = minimumLevel;
        this->m_maximumLevel = maximumLevel;
        this->m_schoolMask = schoolMask;
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_key_tent_def : public type_treasure_def {
public:
    inline type_key_tent_def(int subtype, int value)
        : type_treasure_def(10, subtype, value, 10)
    {
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
    virtual unsigned char isTerrainDependent();
};

// Retail writes both 0x640bac and 0x640bb8 after the retained base call.
// That is direct evidence for this two-level dwelling hierarchy.  The older
// cross-build vtable roster supplies the final class name; the intermediate
// role name remains provisional until stronger source evidence appears.
class type_dwelling_def : public type_treasure_def {
public:
    inline type_dwelling_def(int subtype)
        : type_treasure_def(17, subtype, -1, 40)
    {
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_map_dwelling_def : public type_dwelling_def {
public:
    inline type_map_dwelling_def(int subtype)
        : type_dwelling_def(subtype)
    {
    }

    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
};

class type_resource_lump_def : public type_treasure_def {
public:
    inline type_resource_lump_def(
        int objectType, int subtype, int value, int density)
        : type_treasure_def(objectType, subtype, value, density)
    {
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_prison_def : public type_treasure_def {
public:
    int m_experience;

    inline type_prison_def(int value, int experience)
        : type_treasure_def(62, 0, value, 30)
    {
        this->m_experience = experience;
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_scholar_def : public type_treasure_def {
public:
    inline type_scholar_def()
        : type_treasure_def(81, 0, 1500, 100)
    {
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_quest_creature_def : public type_black_box_creature_def {
public:
    inline type_quest_creature_def(int creatureType, int questIndex)
        : type_black_box_creature_def(creatureType)
    {
        m_objectType = 83;
        m_subtype = questIndex;
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
    virtual unsigned char isTerrainDependent();
};

class type_quest_experience_def : public type_treasure_def {
public:
    int m_experience;

    inline type_quest_experience_def(
        int questIndex, int value, int experience)
        : type_treasure_def(83, questIndex, value, 10)
    {
        this->m_experience = experience;
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
    virtual unsigned char isTerrainDependent();
};

class type_quest_gold_def : public type_treasure_def {
public:
    int m_gold;

    inline type_quest_gold_def(int questIndex, int value, int gold)
        : type_treasure_def(83, questIndex, value, 10)
    {
        this->m_gold = gold;
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
    virtual unsigned char isTerrainDependent();
};

SIZE(type_shrine_def, 0x14);
SIZE(type_witch_hut_def, 0x14);
SIZE(type_spell_scroll_def, 0x18);
SIZE(type_black_box_creature_def, 0x1c);
SIZE(type_artifact_def, 0x14);
SIZE(type_black_box_experience_def, 0x18);
SIZE(type_black_box_gold_def, 0x18);
SIZE(type_black_box_spells_def, 0x20);
SIZE(type_key_tent_def, 0x14);
SIZE(type_dwelling_def, 0x14);
SIZE(type_map_dwelling_def, 0x14);
SIZE(type_resource_lump_def, 0x14);
SIZE(type_prison_def, 0x18);
SIZE(type_scholar_def, 0x14);
SIZE(type_quest_creature_def, 0x1c);
SIZE(type_quest_experience_def, 0x18);
SIZE(type_quest_gold_def, 0x18);

struct TRmgMapPosition {
    int m_x;
    int m_y;
    int m_z;

    TRmgMapPosition() {}
    TRmgMapPosition(int newX, int newY, int newZ);

    // ConnectZones constructs the translated coordinate as a returned
    // temporary before consuming it.  This inline source operation restores
    // retail's 0x98-byte frame and temporary lifetime; the RMG compiland is
    // absent from Dreamcast, so the operator spelling remains provisional.
    TRmgMapPosition operator+(TPoint offset) const;
    TRmgMapPosition& operator+=(const TPoint& offset);
    TRmgMapPosition& operator-=(const TPoint& offset);
};

// Complete's zone-connection records are walked at a 0x1c-byte stride by
// the connection pass.  The first pointer identifies the opposite template
// zone; the three adjacent bytes select guard policy and record completion.
struct TRmgZoneConnection {
    TRmgTownSlot* m_destination;             // +0x00
    int m_value;                             // +0x04
    unsigned char m_unguarded;               // +0x08
    unsigned char m_placeBorderObjects;      // +0x09
    unsigned char m_connected;               // +0x0a
    // Replaces synthetic opaque000b: +0x0b aligns four int limits.
    // Retail connection reader 0x5382c9..0x538304 parses spreadsheet
    // columns 81..84 into +0x0c/+0x10/+0x14/+0x18. At 0x538307..0x53832b
    // it compares the first pair with humanPlayerCount and the second
    // pair with humanPlayerCount + computerPlayerCount before insertion.
    // Role-derived names, consistent with the TRmgTownSlot limits below.
    int m_minimumHumanPlayers;               // +0x0c
    int m_maximumHumanPlayers;               // +0x10
    int m_minimumPlayers;                    // +0x14
    int m_maximumPlayers;                    // +0x18
};

enum ERmgTemplateZoneKind {
    RMG_TEMPLATE_HUMAN = 0,
    RMG_TEMPLATE_COMPUTER = 1,
    RMG_TEMPLATE_TREASURE = 2,
    RMG_TEMPLATE_JUNCTION = 3
};

enum ERmgTreasurePlacementLimits {
    RMG_TREASURE_ATTEMPTS = 3,
    RMG_TREASURE_MINIMUM_REMAINDER = 1500
};

struct TRmgTreasureRange {
    int m_minimum;
    int m_maximum;
    int m_density;
};

// ReadRmgTemplateZones allocates 0xd4 bytes and constructs connections at
// +0xc4. ConnectZones reads its _First at +0xc8 and _Last at +0xcc;
// those pointer offsets must not be mistaken for the vector's own offset.
// Unresolved scalar groups retain offset-based names until their consumers
// establish their roles. Other field names are provisional retail roles.
struct TRmgTownSlot {
    int m_zoneIndex;                    // +0x00
    int m_kind;                         // +0x04: ERmgTemplateZoneKind
    int m_size;                         // +0x08
    int m_minimumHumanPlayers;          // +0x0c
    int m_maximumHumanPlayers;          // +0x10
    int m_minimumPlayers;               // +0x14
    int m_maximumPlayers;               // +0x18
    int m_playerIndex;                  // +0x1c
    int m_parameters0020[8];
    unsigned char m_flag0040;
    unsigned char m_allowedTowns[9];    // +0x41
    int m_parameters004c[7];
    int m_parameters0068[7];
    // template byte to prefer the aligned town's native terrain table.
    // Complete-only provisional role name.
    unsigned char m_useNativeTerrain;
    unsigned char m_allowedTerrain[8];  // +0x85
    int m_monsterStrength;              // +0x90
    unsigned char m_flag0094;
    unsigned char m_allowedMonsters[10]; // +0x95
    TRmgTreasureRange m_treasure[3];     // +0xa0
    std::vector<TRmgZoneConnection> m_connections; // +0xc4

    TRmgZoneConnection* findConnection(int destinationZone);
};
SIZE(TRmgTownSlot, 0xd4);

// The rmg.txt coordinator allocates this 0x38-byte object, assigns its
// name and size limits, and passes it to the zone reader in edx.
struct TRmgTemplate {
    std::string m_name;                  // +0x00
    std::vector<TRmgTownSlot*> m_zones;   // +0x10
    char m_opaque0020[0x10];
    int m_minimumSize;                  // +0x30
    int m_maximumSize;                  // +0x34

    ~TRmgTemplate();
    TRmgTownSlot* findZone(int zoneIndex);
};
SIZE(TRmgTemplate, 0x38);

void readRmgTemplateZones(
    const TSpreadsheetResource* sheet, TRmgTemplate* mapTemplate,
    int firstRow, int endRow, int humanPlayers, int computerPlayers,
    int mapVersion);

// Retained fastcall helper at 0x545e00, also expanded by zone connections.
int getRmgGuardValue(int value, int strength);

// The eight clockwise neighbors are initialized at 0x530da0; group fit
// 0x5355e0 scans the whole domain when testing for an open neighbor.
enum ERmgDirectionLimits {
    RMG_DIRECTION_COUNT = 8
};

// Voronoi's circumcenter arithmetic separates displacement vectors from
// positions: vector+vector is a member call, point+vector and point-point
// are free calls. All carry two signed dwords; names remain provisional.
struct TRmgVector {
    int m_x;
    int m_y;

    TRmgVector() {}
    TRmgVector(int newX, int newY) : m_x(newX), m_y(newY) {}

    int length() const;
    TRmgVector operator+(TRmgVector other) const;
    TRmgVector operator*(int scale) const;
    TRmgVector operator/(int divisor) const;
    // The analogous Graphics Gems vector dot is a tiny header inline. Retail
    // likewise expands both calls in buildVertices and retains no separate
    // body; the by-value operand also recovers that caller's register homes.
    int dot(TRmgVector other) const
    {
        return m_y * other.m_y + m_x * other.m_x;
    }
};

// Retail's common direction table contains eight consecutive two-dword
// offsets.  Its cinit at 0x530da0 proves the user-provided constructor while
// the absence of an atexit registration proves that destruction is trivial.
// The comparator is independently used by the RMG set cluster.
struct TPoint {
    int m_x;
    int m_y;

    TPoint() {}
    TPoint(int newX, int newY) : m_x(newX), m_y(newY) {}

    TPoint& operator+=(const TRmgVector& offset)
    {
        m_x += offset.m_x;
        m_y += offset.m_y;
        return *this;
    }
    bool operator==(const TPoint& other) const
    {
        return m_x == other.m_x && m_y == other.m_y;
    }
    bool operator!=(const TPoint& other) const
    {
        return !(*this == other);
    }

    bool operator<(const TPoint& other) const
    {
        return m_y < other.m_y || (m_y == other.m_y && m_x < other.m_x);
    }
    // The retained 33-byte add at 0x4fa540 (rmg_terrain.cpp) is this
    // operator: refresh 0x4f9f60 and line paintPoint 0x4fa571 call it on a
    // TPoint copy of a grid point with a tile direction. Declared last so
    // the earlier member handles are unchanged.
    // Coordinate accessors, used by the terrain painter's diagonal checks
    // for the same site-count reason as the grid point's; declared after
    // the data so the earlier member handles are unchanged. Adding them
    // also returned rmg's quest-creature generate to 100% (include-set
    // state, 99.73% before).
    int getX() const { return m_x; }
    int getY() const { return m_y; }
    TPoint& operator+=(const TPoint& offset);
};

TPoint operator+(TPoint point, TRmgVector offset);
TRmgVector operator-(TPoint left, TPoint right);

// Complete-only Voronoi addSite 0x5fd790 passes whole site positions to
// retained integer predicates. Aggregate-by-value arguments occupy the
// stack under /Gr: orientation 0x5fdae0 returns with ret 0x18, distance
// 0x5fdb10 with ret 0x10. Names describe the proven geometry operations.
int getRmgPointOrientation(TPoint first, TPoint second, TPoint third);
int getRmgSquaredDistance(TPoint first, TPoint second);

// The map-painting grid uses unsigned coordinates: the terrain set's lower
// bound at 0x5b8a40 compares y, then x, with jb/jae. Its retained constructor
// at 0x5b76b0 reads both arguments through pointers. This role name is
// provisional; the signed geometry TPoint is a separate recovered surface.
// Those const-reference coordinates and the free comparator's position among
// the retail template bodies support a generic coordinate owner. Its unsigned
// specialization keeps both retained constructors exact and emits the comparator
// after tree insertion, preserving the latter's lock exception frame. This is
// a Complete/x86 source hypothesis, not a Dreamcast-proven original type name.
template<class Coordinate> struct TRmgCoordinatePoint {
    Coordinate m_x;
    Coordinate m_y;

    TRmgCoordinatePoint() {}

    // The four late point constructions in RepairTerrainPoint pass x and y by
    // reference. The retained two-store body is 24 bytes including ret 8.
    // VA instance: TRmgCoordinatePoint<unsigned int>::TRmgCoordinatePoint(const unsigned int&, const unsigned int&)
    VA(0x005B76B0, 0x18)
    TRmgCoordinatePoint(const Coordinate& newX, const Coordinate& newY)
        : m_x(newX), m_y(newY) {}
    TRmgCoordinatePoint(const TPoint& point);

    // Coordinate accessors: each use is a free inline site, and the terrain
    // painter's diagonal checks need those sites to divide their budgets so
    // that retail's retained cache reads stay calls (2026-09-12), and
    // paintRectangle walks its rectangle through them so its body stays
    // above the saved-body cliff. Every other body still reads the public
    // fields, and each migration is measured on its own.
    Coordinate getX() const { return m_x; }
    Coordinate getY() const { return m_y; }
    void setX(Coordinate newX) { m_x = newX; }
    void setY(Coordinate newY) { m_y = newY; }

    // paintTransitions steps one column with a grid-side compound add; the
    // retained add at 0x4fa540 is TPoint's, so this one stays inline.
    TRmgCoordinatePoint& operator+=(const TPoint& offset)
    {
        m_x += offset.m_x;
        m_y += offset.m_y;
        return *this;
    }
    operator TPoint() const
    {
        return TPoint(m_x, m_y);
    }
};

typedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;

template<class Coordinate>
bool operator<(const TRmgCoordinatePoint<Coordinate>& left,
    const TRmgCoordinatePoint<Coordinate>& right);

struct TRmgZoneBounds {
    int m_minimumX;
    int m_minimumY;
    int m_maximumX;
    int m_maximumY;

    bool contains(const TPoint& point) const
    {
        return point.m_x >= m_minimumX && point.m_x < m_maximumX &&
            point.m_y >= m_minimumY && point.m_y < m_maximumY;
    }
};

TPoint clipRmgBoundaryPoint(
    const TRmgZoneBounds& bounds, TPoint point, TPoint toward);

// The terrain noise generator 0x53ed00 keeps a vector of nine-dword regions.
// Its subdivision helper 0x53e9e0 copies each complete region, halves both
// coordinate intervals, and replaces three corner samples for each quadrant.
// Offsets +0..0xc are bounds; +0x10/+0x14/+0x18/+0x1c correspond to
// (minX,minY)/(minX,maxY)/(maxX,minY)/(maxX,maxY); +0x20 controls the random
// displacement range. These Complete-only role names have no DC counterpart.
struct TRmgNoiseRegion {
    TRmgZoneBounds m_bounds;
    int m_corners[4];
    int m_variation;
};
SIZE(TRmgNoiseRegion, 0x24);

// Passed as a four-dword value immediately after the region. 0x53ed00
// passes averages of corners 0/2, 0/1, 1/3 and 2/3 in these four slots.
// Random displacements are drawn in minX/minY/maxX/maxY order before the
// center displacement; the field order preserves the by-value call ABI.
struct TRmgNoiseMidpoints {
    int m_minYValue;
    int m_minXValue;
    int m_maxYValue;
    int m_maxXValue;
};
SIZE(TRmgNoiseMidpoints, 0x10);

void subdivideRmgNoiseRegion(std::vector<TRmgNoiseRegion>& pending,
    TRmgNoiseRegion region,
    TRmgNoiseMidpoints midpoints,
    int centerValue);

enum ERmgConnectionConstants {
    RMG_SHIPYARD_WATER_OFFSET_COUNT = 4,
    RMG_WATER_NONE = 0,
    RMG_WATER_NORMAL = 1,
    RMG_WATER_ISLANDS = 2,
    RMG_WATER_RANDOM = 3
};

// Complete's guard selector 0x540b20 uses these bounds, not the full combat
// creature array. Its RoE exclusion starts at 118 even though evaluation
// stops before 117; keep that observed boundary distinct.
enum ERmgGuardConstants {
    RMG_GUARD_CREATURE_COUNT = 145,
    RMG_GUARD_ROE_CREATURE_LIMIT = 117,
    RMG_GUARD_ROE_EXCLUDED_FIRST = 118,
    RMG_GUARD_MAXIMUM_COUNT = 100,
    RMG_GUARD_DISPOSITION = 3
};

// The function-local river-delta table has a non-trivial empty destructor:
// retail registers its cleanup thunk when CreateRiver first reaches the
// table.  The type is shared here so the table has one canonical shape.
struct TRmgRiverDeltaOffset {
    int m_x;
    int m_y;

    TRmgRiverDeltaOffset(int newX, int newY) : m_x(newX), m_y(newY) {}
    ~TRmgRiverDeltaOffset() {}
};

class type_object;

struct TRmgMovementCost {
    unsigned m_cost : 16;
    // Zone flood 0x53f1a0 clears this high word at the seed (0x53f242),
    // stays in the supplied zone (0x53f339), and relaxes it with step
    // costs 2/3 (0x53f34a..0x53f377). Ground-connection selection reads
    // it at 0x5412e9..0x5412ef to rank candidate crossings.
    unsigned m_zonePathCost : 16;
};

// The connection pass extracts the signed zone id from bits 16..23 with
// `shl 8; sar 24` while ranking candidate squares by the low word.  Keeping
// both fields in one dword reproduces the retail bitfield loads rather than
// masking raw storage in the algorithm.
struct TRmgZoneCellState {
    unsigned m_score : 16;
    signed m_zone : 8;
    signed m_connectionEligibility : 8;
};

// The six-bit signed land field is fixed by retail's `shl 26; sar 26`
// extraction in the river-delta path.  The four-bit field at bit 26 is
// tested as a unit when river routing prices an already decorated tile.
struct TRmgGroundTile {
    // All three painter adapters exchange integer kinds. The terrain setter
    // 0x532190 writes that generic integer directly, and getter 0x5322c0
    // sign-extends six bits. No Dreamcast enum declaration exists here.
    // Signed storage remains provisional: the existing TTerrainType field
    // plus casts in both integer setters emits identical setter/getter and
    // connection-pass bytes in eight field/local models. Integer setter
    // parameters alone do not distinguish those source declarations.
    signed m_landType : 6;
    // Retail terrain adapter 0x532190 stores an eight-bit frame at bit 6;
    // getter 0x532288 sign-extends it. River adapter 0x532520 writes the
    // four-bit type at 14 and eight-bit frame at 18; 0x5327c0 sign-extends
    // both. Role-derived names; original spellings unknown.
    // Replaces unknown06.
    signed m_terrainFrame : 8;
    signed m_riverType : 4;
    signed m_riverFrame : 8;
    // this at bit 26; getter 0x532447..0x532450 sign-extends four bits.
    signed m_roadType : 4;
    unsigned m_unknown30 : 2;
};

struct TRmgGroundTileData {
    // Road adapter 0x532360 writes all eight low bits; 0x53244a/0x532453
    // sign-extends the frame. Replaces roadSprite and synthetic unknown07.
    signed m_roadFrame : 8;
    unsigned m_blockedDirections : 4;
    unsigned m_connectionDirection : 3;
    // Adapter setters store paired flips: terrain 0x5321e6 (bits 15/16),
    // river 0x532587 (17/18), road 0x5323bd (19/20). The matching getters
    // extract each bit into TRmgTerrainTile's flipX/flipY bytes.
    // Replaces the first six bits of unknown15.
    unsigned m_terrainFlipX : 1;
    unsigned m_terrainFlipY : 1;
    unsigned m_riverFlipX : 1;
    unsigned m_riverFlipY : 1;
    unsigned m_roadFlipX : 1;
    unsigned m_roadFlipY : 1;
    unsigned m_coastal : 1;
    // BuildRoadCostMap proves these two Complete-only routing flags at bits
    // 22 and 25.  The first marks an object entrance whose adventure-object
    // traits constrain approach directions; the second admits the tile to
    // the road-cost flood.
    unsigned m_roadEntrance : 1;
    // 0x535ee0 traces a closed placement perimeter into the point vector
    // at +0x38 (append 0x535fb2, closure 0x536051..0x536062). Accepted
    // placement 0x5468e8 calls it, then marks these points with bit 23
    // at 0x546923; 0x54b6df and 0x54bad5 mark the same outline in the
    // quest placement paths. Checker 0x546ed5 requires the marked bit.
    unsigned m_placementOutline : 1;
    unsigned m_connectionVisited : 1;
    unsigned m_roadPassable : 1;
    unsigned m_borderObject : 1;
    unsigned m_subterraneanGate : 1;
    unsigned m_zoneBoundary : 1;
    // and 0x532769..0x532780 set bit 29 from a nonzero river kind. This is
    // river presence, not the separate routing target at bit 30.
    unsigned m_hasRiver : 1;
    unsigned m_riverTarget : 1;
    unsigned m_impassable : 1;
};

struct TRmgConnectionDecoration {
    unsigned m_present : 1;
    unsigned m_direction : 4;
    unsigned m_unknown05 : 27;
};

// The rand_trn.txt reader appends 0x4c-byte rows. ScoreObjectPlacement
// consumes the ten terrain values and the two vectors indexed by rule id.
// These are Complete-only role names; no Dreamcast RMG records survive.
struct TRmgObjectPlacementRule {
    int m_index;                         // +0x00
    int m_terrainScores[10];             // +0x04
    std::vector<int> m_adjacentScores;    // +0x2c
    std::vector<int> m_blockedScores;     // +0x3c
};

enum ERmgObjectPlacementMark {
    RMG_PLACEMENT_ADJACENT = 1,
    RMG_PLACEMENT_OVERLAP = 2,
    RMG_PLACEMENT_BLOCKED = 4
};

enum ERmgObjectPlacementScore {
    RMG_PLACEMENT_INVALID = -5000,
    RMG_PLACEMENT_MINIMUM_TERRAIN_SCORE = -1000,
    RMG_PLACEMENT_NO_TERRAIN_PREFERENCE = -1
};

// The prototype is the existing objects.txt TObjectType, whose 0x4c layout
// and masks are independently recovered in the object-type compiland.
// 0x532c80 owns the outline vector; 0x532e40 lazily fills the 8x6 priorities.
struct TRmgObjectPropertiesRef {
    TObjectType* m_prototype;              // +0x00
    // Retail 0x536560 binds the first recommended terrain.
    int m_preferredTerrain;               // +0x04, rand_trn.txt rule binding
    unsigned m_refCount;                   // +0x08
    int m_prototypeIndex;
    // Lazy builder 0x532c80 owns outline; 0x532e40
    // initializes overlapPriorities and the +0xe4 flag. +0xe5..e7 aligns the tail.
    TRmgObjectPlacementRule* m_placementRule; // +0x10
    std::vector<TPoint> m_outline;         // +0x14
    int m_overlapPriorities[8][6];         // +0x24
    unsigned char m_prioritiesInitialized; // +0xe4
    char m_pad00e5[3];

    TRmgObjectPropertiesRef(TObjectType* prototype);

    void buildOutline();
    void buildOverlapPriorities();
};

class type_object {
public:
    TRmgObjectPropertiesRef* m_properties; // +0x04
    TRmgMapPosition m_position;             // +0x08
    // Placement scorer 0x536bc0 marks this relation.
    unsigned char m_candidateCovers;
    // Placement scorer 0x536bc0 marks this relation.
    unsigned char m_candidateBehind;
    // Placement scorer 0x536bc0 marks this relation.
    unsigned char m_adjacentToCandidate;
    // Placement scorer 0x536bc0 marks this relation.
    unsigned char m_overlapsCandidate;
    // Placement scorer 0x536bc0 marks this relation.
    unsigned char m_blockedByCandidate;
    char m_tailPadding[3];

    type_object(TRmgObjectPropertiesRef* newProperties);
    TRmgMapPosition getPosition() const;

    void clearPlacementMarks();

    unsigned char isPlacementTouched() const
    {
        return m_adjacentToCandidate || m_blockedByCandidate || m_overlapsCandidate;
    }

    virtual ~type_object();
    virtual void unknownOperation();
    // The quest-artifact override at 0x533a50 clears owned state, and its
    // generator callee replaces this object's property reference. These
    // mutable operations reject the earlier const receiver placeholder.
    virtual unsigned char isWritable();
    virtual void write(TAbstractFile* outfile, int parameter);
};

// Provisional Complete-only role: createGuard (0x540b20) allocates 0x2c and
// installs vtable 0x640a84. Slot 3 (0x5331f0) serializes the id, count and
// disposition below; +0x28 is neither initialized nor read by those bodies.
class rmgMonsterObject : public type_object {
public:
    int m_objectId;       // +0x1c
    int m_count;          // +0x20, serialized as two bytes
    int m_disposition;    // +0x24, serialized as one byte
    int m_unknown28;      // +0x28

    rmgMonsterObject(TRmgObjectPropertiesRef* properties, int objectId, int count)
        : type_object(properties)
    {
        m_count = count;
        m_disposition = RMG_GUARD_DISPOSITION;
        m_objectId = objectId;
    }
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(rmgMonsterObject, 0x2c);

// Complete town vtable 0x640a94; constructor expansion at 0x54543d
// stores owner/option/id in the 0x28-byte allocation. Names are role-derived.
// Value/reference argument combinations and an ordinary out-of-class body
// leave both placement callers unchanged; neither model improves their residual.
class rmgTownObject : public type_object {
public:
    int m_objectId;
    int m_player;
    unsigned char m_townOption;
    rmgTownObject(TRmgObjectPropertiesRef* properties, int objectId,
        int player, unsigned char townOption) : type_object(properties)
    {
        m_player = player;
        m_townOption = townOption;
        m_objectId = objectId;
    }
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(rmgTownObject, 0x28);

// Provisional Complete-only role. The shipyard path allocates 0x1c bytes,
// calls type_object's constructor, then replaces its vptr with 0x640aa4.
// That table shares the base's middle slots and overrides serialization:
// 0x533460 appends an unowned player byte and three reserved bytes.
class rmgOwnableObject : public type_object {
public:
    rmgOwnableObject(TRmgObjectPropertiesRef* properties)
        : type_object(properties) {}
    virtual void write(TAbstractFile* outfile, int parameter);
};

// Artifact factory 0x5341f0 allocates the base 0x1c extent and installs
// vtable 0x640ab4. Its writer adds one zero byte after type_object's record;
// no additional instance fields are present. Complete-only role spelling.
class rmgArtifactObject : public type_object {
public:
    rmgArtifactObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int parameter);
};
SIZE(rmgArtifactObject, 0x1c);

// These four factories allocate the same 0x1c base extent and change only
// the writer vptr. Their distinct default H3M payloads prove separate classes;
// the Complete-only class spellings below describe those roles.
// Retail vtable 0x640ac4.
class rmgResourceObject : public type_object {
public:
    rmgResourceObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int parameter);
};
SIZE(rmgResourceObject, 0x1c);

// Pandora's Box factories 0x534380/0x534410/0x534490 allocate 0x54 bytes
// and install vtable 0x640ad4. Writer 0x5336f0 identifies each payload field;
// the spell factory 0x534520 appends integer spell indices to the vector.
// The vector begins at +0x44 (its allocator byte), with _First at +0x48.
// These are provisional Complete-only role names, not Dreamcast identities.
class rmgBlackBoxObject : public type_object {
public:
    int m_experience;                  // +0x1c
    int m_resources[7];                // +0x20, gold at +0x38
    int m_creatureType;                // +0x3c, -1 means no creature reward
    int m_creatureCount;               // +0x40
    std::vector<int> m_spells;         // +0x44

    rmgBlackBoxObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(rmgBlackBoxObject, 0x54);

// Seer-hut factories 0x534b90/0x534cc0/0x534db0 allocate this 0x34-byte
// reward object (vtable 0x640b04). Writer 0x533a90 proves the field roles.
// Complete-only names are provisional; no Dreamcast RMG class is available.
class rmgSeerHutObject : public type_object {
public:
    int m_artifact;                    // +0x1c, required quest artifact
    int m_experience;                  // +0x20
    int m_resourceType;                // +0x24, defaults to gold (6)
    int m_resourceCount;               // +0x28
    int m_creatureType;                // +0x2c, defaults to -1
    int m_creatureCount;               // +0x30

    rmgSeerHutObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(rmgSeerHutObject, 0x34);

// Artifact wrapper vtable 0x640af4 shares the ordinary artifact writer at
// 0x533500. It owns the pending seer hut until placement transfers it to
// the map; 0x533a50 clears that pointer on both success and failure.
class rmgQuestArtifactObject : public rmgArtifactObject {
public:
    type_random_map_generator* m_generator; // +0x1c
    rmgSeerHutObject* m_seerHut;             // +0x20
    type_treasure_def* m_definition;        // +0x24

    rmgQuestArtifactObject(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, rmgSeerHutObject* seerHut,
        type_treasure_def* definition);
    virtual ~rmgQuestArtifactObject();
    virtual unsigned char isWritable();
};
SIZE(rmgQuestArtifactObject, 0x28);

// Key-tent definition factory 0x534fd0 allocates 0x24 bytes, installs
// vtable 0x640ae4, and supplies its generator and value. The writable
// override tries a corresponding guard, then substitutes another treasure
// if that placement fails. Its record uses the ordinary object writer.
// Complete-only class and method spellings describe the recovered roles.
class rmgKeyTentObject : public type_object {
public:
    type_random_map_generator* m_generator; // +0x1c
    int m_value;                           // +0x20

    rmgKeyTentObject(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, int value);
    virtual unsigned char isWritable();
};
SIZE(rmgKeyTentObject, 0x24);

// Retail vtable 0x640b24.
class rmgScholarObject : public type_object {
public:
    rmgScholarObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int parameter);
};
SIZE(rmgScholarObject, 0x1c);

// Retail vtable 0x640b34.
class rmgShrineObject : public type_object {
public:
    rmgShrineObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int parameter);
};
SIZE(rmgShrineObject, 0x1c);

// Complete-only spell-scroll object. Factory 0x534ed0 allocates 0x20
// bytes, stores its selected spell at +0x1c and installs vtable 0x640b44.
// Writer 0x533ff0 emits that spell as one byte. Original class name unknown.
// Full-build collateral on admission: unchanged CEnterNameEdit::onKillFocus
// CUR 100 -> 99.8710 (two loads exchange order); MAX/HIST retain 100.
class rmgSpellScrollObject : public type_object {
public:
    int m_spell; // +0x1c, role-derived name
    rmgSpellScrollObject(TRmgObjectPropertiesRef* properties, int spell);
    virtual void write(TAbstractFile* outfile, int parameter);
};
SIZE(rmgSpellScrollObject, 0x20);

// Retail vtable 0x640b54.
class rmgWitchHutObject : public type_object {
public:
    rmgWitchHutObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int parameter);
};
SIZE(rmgWitchHutObject, 0x1c);

// Factory 0x5348d0 allocates this 0x2c-byte derived object after reserving a
// hero. Vtable 0x640b14 slot 1 releases that reservation through the generator
// at +0x1c; the original Complete-only class spelling is unavailable.
class rmgHeroObject : public type_object {
public:
    type_random_map_generator* m_generator; // +0x1c
    int m_objectId;                         // +0x20
    int m_heroIndex;                        // +0x24
    int m_experience;                       // +0x28, prison definition experience

    rmgHeroObject(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, const int& objectId, int heroIndex,
        int experience);

    virtual void unknownOperation();
    virtual void write(TAbstractFile* outfile, int parameter);
};
SIZE(rmgHeroObject, 0x2c);

struct TRmgMapItem {
    std::vector<type_object*> m_objects;    // +0x00
    TRmgMapPosition m_previousTile;         // +0x10
    TRmgMovementCost m_movement;            // +0x1c
    TRmgZoneCellState m_zoneState;           // +0x20
    TRmgGroundTile m_tile;                  // +0x24
    TRmgGroundTileData m_tileData;          // +0x28
    TRmgConnectionDecoration m_connection;  // +0x2c

    TRmgMapItem();
    void clear();
    void write(TAbstractFile* outfile);
    // Retained cell writer 0x546940; four scalar inputs, terrain fields only.
    void setTerrain(int terrain, int frame,
        unsigned char flipX, unsigned char flipY);

    // CreateRiver's predicate reads shift the high tile bits and test a
    // byte result. These queries recover that boundary; direct field tests
    // instead use dword masks. Names remain provisional without RMG symbols.
    bool hasRiver() const { return m_tileData.m_hasRiver != 0; }
    bool isRiverTarget() const { return m_tileData.m_riverTarget != 0; }
    bool isImpassable() const { return m_tileData.m_impassable != 0; }

    // PaintZoneTerrain extracts bit 28 then tests its byte result. The
    // direct field condition instead folds to a dword mask. Retail-only
    // accessor hypothesis, consistent with the adjacent flag queries.
    unsigned char isZoneBoundary() const
    {
        return m_tileData.m_zoneBoundary;
    }

    // Placement helpers 0x531170/0x5318b0/0x531cf0 all shift bit 22 and
    // test the truncated byte. Direct bitfield conditions fold to a dword
    // mask; keep this same ordinary query at each recovered boundary.
    unsigned char isRoadEntrance() const
    {
        return m_tileData.m_roadEntrance;
    }

    // ScoreObjectPlacement reads bit 27 with shr/test dl, whereas its
    // direct roadPassable condition tests the containing dword. The
    // provisional byte accessor reproduces that truncation; a direct
    // bitfield condition instead folds to test dword ptr [item+0x28],imm.
    unsigned char hasSubterraneanGate() const
    {
        return m_tileData.m_subterraneanGate;
    }

    unsigned char isPassableLand() const;

    // RepairWaterZoneBorders tests this flag after truncating it to a byte
    // at 0x53fe30, then tests roadPassable directly as a dword bit.
    unsigned char hasBorderObject() const
    {
        return m_tileData.m_borderObject;
    }

    // Group fit 0x546ed5 shifts bit 23 and tests the byte result.
    unsigned char isPlacementOutline() const
    {
        return m_tileData.m_placementOutline;
    }
    // Connection flood: the visited bit is read and written through the
    // same byte boundary the other flag queries use.
    unsigned char isConnectionVisited() const
    {
        return m_tileData.m_connectionVisited;
    }
    void setConnectionVisited()
    {
        m_tileData.m_connectionVisited = 1;
    }
    int getLandType() const
    {
        return m_tile.m_landType;
    }

    // Retail road/river relaxation copies the predecessor to a separate
    // parameter home before storing cost and coordinates. The by-value
    // boundary is inferred from those repeated x86 copies; the name is
    // provisional because Dreamcast contains no RMG compiland. Keeping
    // the scalar cost write and struct assignment directly in each caller
    // loses that snapshot (BuildRoadCostMap 73.7139% versus 75.6686%).
    void setMovementCost(int cost, TRmgMapPosition previous)
    {
        m_movement.m_cost = cost;
        m_previousTile = previous;
    }

    // CreateRiver's reset pass copies a by-value predecessor before a
    // constant 32000 cost write, motivating this ordinary reset helper.
    // Its role name is provisional; Dreamcast has no RMG compiland.  A
    // generic cost parameter instead lowers the constant write as XOR,
    // whereas retail retains the constant AND/OR form.
    void resetMovement(TRmgMapPosition previous)
    {
        m_movement.m_cost = 32000;
        m_previousTile = previous;
    }
};

// Retail has distinct seven-slot abstract tables at 0x6409e8 (map) and
// 0x640a58 (adapter). Their deleting destructors at 0x5361b0/0x537910
// store those different tables, so matching operation slots do not establish
// one base identity. The painting coordinates are the unsigned grid type.
class TRmgMapInterface {
public:
    virtual ~TRmgMapInterface();
    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile) = 0;
    virtual void setOverlay(const TRmgGridPoint& point, int value) = 0;
    // Slot 3 returns its explicit output reference. The adapters consume
    // that returned reference, which distinguishes this from a hidden value
    // result: together the map and both adapter bodies reproduce retail.
    virtual TRmgGridPoint& getSize(TRmgGridPoint& output) = 0;
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point) = 0;
    virtual int getLand(const TRmgGridPoint& point) = 0;
    virtual int getOverlay(const TRmgGridPoint& point) = 0;
};

class TRmgMapAdapterInterface {
public:
    virtual ~TRmgMapAdapterInterface();
    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile) = 0;
    virtual void setOverlay(const TRmgGridPoint& point, int value) = 0;
    virtual TRmgGridPoint getSize() = 0;
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point) = 0;
    virtual int getLand(const TRmgGridPoint& point) = 0;
    virtual int getOverlay(const TRmgGridPoint& point) = 0;
};

// The road-decoration adapter has the same seven-slot shape but a distinct
// abstract vtable at 0x640a20. Its concrete subclass writes the packed road
// fields through the bodies beginning at 0x532360.
class TRmgRoadMapAdapterInterface {
public:
    virtual ~TRmgRoadMapAdapterInterface();
    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile) = 0;
    virtual void setOverlay(const TRmgGridPoint& point, int value) = 0;
    virtual TRmgGridPoint getSize() = 0;
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point) = 0;
    virtual int getLand(const TRmgGridPoint& point) = 0;
    virtual int getOverlay(const TRmgGridPoint& point) = 0;
};

class type_random_map : public TRmgMapInterface {
public:
    unsigned char m_ownsMapItems;           // +0x04
    // The ownership flag is a byte at +4 after the vptr, and
    // mapItems starts at +8. These three bytes align the pointer.
    char m_paddingBeforeMapItems[3];
    TRmgMapItem* m_mapItems;                // +0x08
    int m_mapWidth;                         // +0x0c
    int m_mapHeight;                        // +0x10
    int m_numberLevels;                     // +0x14

    // Owning constructor retained at 0x530fb0, called by the generator base
    // and temporary treasure-group maps. Three dimensions, thiscall ret 0xc.
    type_random_map(int width, int height, int levels);

    // The buffer-first view signature preserves the dimension values before
    // GetMapItem computes the plane pointer. In RepairWaterZoneBorders the
    // constructor/painting range 0x540124..0x54020c matches all 232 bytes after
    // relocation resolution and segment placement. The role is retail-only.
    // Controls: dimensions-first scalar arguments reload fields; a map/level
    // pair stores width early; a separate plane local keeps the wrong multiply
    // operand. Field assignments stay in the body: an all-member initializer
    // list moves the vptr store past them. Other view callers remain partial.
    // Store-order control: items before width/height makes createWaterZoneIsland
    // (0x53efa0) exact and preserves repairWaterZoneBorders at 100%. The six
    // orders were scored across all seven header consumers. Restoring the old
    // width/height/items order loses the island's constructor scheduling
    // (95.8947%); the separate caller-only control does not recover it.
    // Paired TPoint/grid-point size arguments, by value/reference and in both
    // orders, leave the underground entry unchanged. Grouping the map's own
    // three dimensions as a position changes retry bodies but closes no further
    // function. Neither interface/layout change is adopted.

    inline type_random_map(TRmgMapItem* items, int width, int height)
    {
        m_mapItems = items;
        m_mapWidth = width;
        m_mapHeight = height;
        m_numberLevels = 1;
        m_ownsMapItems = 0;
    }

    virtual ~type_random_map();

    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile);
    virtual void setOverlay(const TRmgGridPoint& point, int value);
    virtual TRmgGridPoint& getSize(TRmgGridPoint& output);
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point);
    virtual int getLand(const TRmgGridPoint& point);
    virtual int getOverlay(const TRmgGridPoint& point);

    void clear();
    // Retail insertion supplies pointer prvalues to both STL reference
    // parameters; source reference contract inferred at 0x531ea0.
    void addObject(type_object& object, TRmgMapPosition position);
    void markCoastalTiles();
    void floodConnectionCosts(TRmgMapPosition position, unsigned char waterZone);

    int getWidth() const { return m_mapWidth; }
    int getHeight() const { return m_mapHeight; }
    int getNumberLevels() const { return m_numberLevels; }

    TRmgMapItem* getMapItem(int x, int y);
    inline TRmgMapItem* getMapItem(int x, int y, int z)
    {
        return getMapItem(TRmgMapPosition(x, y, z));
    }
    TRmgMapItem* getMapItem(TRmgMapPosition point);

    // Complete-only path carving at 0x543e20 calls these retained map
    // helpers. Names describe the observed cell flags and ray traversal.
    void openPathPatch(int x, int y, int level);
    void markBorderPatch(TRmgMapPosition position);
    TPoint traceBranchEnd(TPoint from, TPoint toward, int level);

    unsigned char hasConnectedOutline(
        const std::vector<TPoint>& outline, TRmgMapPosition position,
        unsigned char allowEntrances, TRmgZone* zone, unsigned char requireGate);
    unsigned char isPlacementBlocked(
        TRmgObjectPropertiesRef* properties, TRmgMapPosition position,
        int zoneIndex, unsigned char rejectBorder);
    unsigned char canPlaceObject(
        TRmgObjectPropertiesRef* properties,
        TRmgMapPosition position,
        TRmgZone* zone);
};

// Complete's treasure retries construct an owned map at +0, then bounds,
// object and outline vectors. There is no derived vptr store: this group
// contains the map. 0x547360 proves the 0x64-byte stack object and cleanup;
// 0x5470d0 reads its bounds at +0x18. Names are provisional retail roles.
struct TRmgTreasureGroup {
    type_random_map m_map;                  // +0x00
    TRmgZoneBounds m_bounds;                // +0x18
    std::vector<type_object*> m_objects;    // +0x28
    std::vector<TPoint> m_outline;           // +0x38
    // with the guard's local coordinates; canPlaceTreasureGroup checks them.
    unsigned char m_hasGuard;               // +0x48, cleared by reset
    char m_padding0049[3];
    // addGuard stores x/y at 0x53556f.
    // canPlaceTreasureGroup reads x/y
    // from +0x4c/+0x50 before translating the guard's neighborhood.
    TPoint m_guardPosition;                 // +0x4c
    // Retail commitTreasureGroup 0x5469ca stores the selected map offset.
    TRmgMapPosition m_position;             // +0x54
    unsigned char m_ready;                  // +0x60, set after assembly
    char m_padding0061[3];

    TRmgTreasureGroup(int width, int height)
        : m_map(width, height, 1), m_hasGuard(0), m_ready(0)
    {
        reset();
    }
    void reset();
    unsigned char addGuard(type_object* guard);
    unsigned char canFitObject(TRmgObjectPropertiesRef* properties, TRmgMapPosition position);
    unsigned char tryAddObject(type_object* object);
    void updateBounds();
    void traceOutline();
};
SIZE(TRmgTreasureGroup, 0x64);

// Complete-only road adapter, provisional role name. Vtable 0x640a04 has
// the seven-slot road interface; 0x548120 constructs the eight-byte object
// with the address of a type_random_map view at +4. Its methods independently
// index that map's 0x30-byte cells and read/write the road packed fields.
class TRmgRoadMapAdapter : public TRmgRoadMapAdapterInterface {
public:
    type_random_map* m_map;

    TRmgRoadMapAdapter(type_random_map* map) : m_map(map) {}
    virtual void setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile);
    virtual void setOverlay(const TRmgGridPoint& point, int value);
    virtual TRmgGridPoint getSize();
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point);
    virtual int getLand(const TRmgGridPoint& point);
    virtual int getOverlay(const TRmgGridPoint& point);
};

// Retail retains these support bodies outside CreateRiver while the adapter
// and map-view construction remains expanded at the call site.  Keeping the
// class definitions shared but the retained bodies in rmg_support.cpp
// reproduces that ordinary translation-unit visibility boundary.
class TRmgMapAdapter : public TRmgMapAdapterInterface {
public:
    type_random_map* m_map;

    inline TRmgMapAdapter(type_random_map* newMap) : m_map(newMap) {}

    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile);
    virtual void setOverlay(const TRmgGridPoint& point, int value);
    virtual TRmgGridPoint getSize();
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point);
    virtual int getLand(const TRmgGridPoint& point);
    virtual int getOverlay(const TRmgGridPoint& point);
};

// Cinit 0x55ed70/0x55f2f0 passes a pattern count and a source int array to
// the retained constructor at 0x4f9be0. That constructor allocates the copied
// pattern ids, then records the first index and occurrence count for each of
// the nine pattern values.
struct TRmgLinePatternRange {
    // Role-derived names: the constructor writes index/count at an 8-byte
    // stride, not two separate nine-element arrays.
    unsigned int m_firstIndex;
    unsigned int m_valueCount;
};
SIZE(TRmgLinePatternRange, 0x8);

struct TRmgLinePatternTable {
    unsigned int m_patternCount;
    int* m_patterns;
    TRmgLinePatternRange m_ranges[9];

    TRmgLinePatternTable(unsigned int patternCount, const int* patterns);
    ~TRmgLinePatternTable();
};
SIZE(TRmgLinePatternTable, 0x50);

// The generator owns the real globals. Their constructor and destructor
// remain ordinary support-library bodies, retained by both init/exit paths.
extern TRmgLinePatternTable g_rmgRiverPatternTable;
extern TRmgLinePatternTable g_rmgRoadPatternTable;

void selectRmgLinePattern(
    const unsigned char* neighbours, const TRmgLinePatternTable* table,
    int& pattern, unsigned char& flipX, unsigned char& flipY);

struct TRmgLinePainterTile;

// Both painter constructors pass their first base to the same retained walker
// constructor at 0x4fa280. Its helpers dispatch the six slots and read the
// dimensions at +4/+8 (0x4fa45b/0x4fa45f and 0x4fa122/0x4fa16a).
// This common abstract prefix is a retail-derived source model; the original
// Complete-only interface spelling is unknown. It has no virtual destructor
// slot: only the final river/road painters append slot 6.
class TRmgLinePainterInterface {
public:
    TRmgGridPoint m_size;

    TRmgLinePainterInterface(const TRmgGridPoint& size);
    virtual TRmgLinePatternTable* getPattern(int value) = 0;
    virtual void setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile) = 0;
    virtual void setOverlay(const TRmgGridPoint& point, int value) = 0;
    virtual int canPaint(const TRmgGridPoint& point) = 0;
    virtual void getTile(const TRmgGridPoint& point, rmgTerrainTile& tile) = 0;
    virtual int getLand(const TRmgGridPoint& point) = 0;

    TRmgLinePainterTile at(const TRmgGridPoint& point);
    int getNeighbourLand(const TRmgGridPoint& point, unsigned int direction);
};

// The value returned at 0x4fa050 holds the painter and a copied coordinate.
// Caller 0x4f9f00 then dispatches through those stored members; the same
// twelve-byte record is expanded at its entry and in 0x4fa080/0x4fa3c0.
// These Complete-only names describe roles, not recovered original spellings.
struct TRmgLinePainterTile {
    TRmgLinePainterInterface* m_painter;
    TRmgGridPoint m_point;

    TRmgLinePainterTile(TRmgLinePainterInterface* painter, const TRmgGridPoint& point);
    int getLand();
    void getTile(rmgTerrainTile& tile);
    void setTile(const rmgTerrainTile& tile);
    unsigned char isBlocked();
    void setOverlay(int value);
};
SIZE(TRmgLinePainterTile, 0x0c);

// Retail clear 0x4fa080 reads an unsigned origin and extent, not the signed
// min/max bounds used for zones. The point walker builds a one-cell rectangle.
// This Complete-only role name does not assert an original class spelling.
struct TRmgGridRectangle {
    TRmgGridPoint m_origin;
    TRmgGridPoint m_size;

    TRmgGridRectangle(const TRmgGridPoint& origin, const TRmgGridPoint& size);
};
SIZE(TRmgGridRectangle, 0x10);

// Shared fastcall refresh reached by the rectangle clear and point walker.
void refreshRmgLinePoint(TRmgLinePainterInterface* painter, const TRmgGridPoint& point);
void clearRmgLineRectangle(TRmgLinePainterInterface* painter, const TRmgGridRectangle& rectangle);

class TRmgLinePainter : public TRmgLinePainterInterface {
public:
    TRmgMapAdapterInterface* m_adapter;

    inline TRmgLinePainter(TRmgMapAdapterInterface* newAdapter)
        : TRmgLinePainterInterface(newAdapter->getSize()), m_adapter(newAdapter)
    {
    }
    ~TRmgLinePainter() {}

    virtual TRmgLinePatternTable* getPattern(int value);
    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile);
    virtual void setOverlay(const TRmgGridPoint& point, int value);
    virtual int canPaint(const TRmgGridPoint& point);
    // Slot 4's caller at 0x4f9fdd pushes output first, then point. The
    // retained wrapper 0x55f350 writes through its second explicit argument;
    // unlike the adapter, this interface does not return a tile by value.
    virtual void getTile(const TRmgGridPoint& point, rmgTerrainTile& tile);
    virtual int getLand(const TRmgGridPoint& point);
};

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
    TRmgLinePainterInterface* m_painter;
    int m_riverType;
    TRmgGridPoint m_position;

    TRmgLineWalker(
        TRmgLinePainterInterface* newPainter,
        int newRiverType,
        const TRmgGridPoint& start);
    void drawTo(const TRmgGridPoint& destination);
    // Shared by constructor 0x4fa280 and the two-axis walk 0x4fa2b0.
    void paintPoint(const TRmgGridPoint& point);
};

class TRmgRiverPainter : public TRmgLinePainter, public TRmgLineWalker {
public:
    TRmgRiverPainter(
        TRmgMapAdapterInterface* newAdapter,
        int newRiverType,
        const TRmgGridPoint& start);
    virtual ~TRmgRiverPainter();
};

// The road-building cluster at 0x548040 uses a parallel painter hierarchy.
// Its base and derived vtables at 0x6411f0/0x64120c differ from the river
// hierarchy's 0x641174/0x641190 tables, while retaining the same line-painting
// interface shape. Original Complete-only class spellings are unavailable.
class TRmgRoadLinePainter : public TRmgLinePainterInterface {
public:
    TRmgRoadMapAdapterInterface* m_adapter;

    inline TRmgRoadLinePainter(TRmgRoadMapAdapterInterface* newAdapter)
        : TRmgLinePainterInterface(newAdapter->getSize()), m_adapter(newAdapter)
    {
    }
    ~TRmgRoadLinePainter() {}

    virtual TRmgLinePatternTable* getPattern(int value);
    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile);
    virtual void setOverlay(const TRmgGridPoint& point, int value);
    virtual int canPaint(const TRmgGridPoint& point);
    virtual void getTile(const TRmgGridPoint& point, rmgTerrainTile& tile);
    virtual int getLand(const TRmgGridPoint& point);
};

class TRmgRoadPainter : public TRmgRoadLinePainter, public TRmgLineWalker {
public:
    TRmgRoadPainter(
        TRmgRoadMapAdapterInterface* newAdapter,
        int newRoadType,
        const TRmgGridPoint& start);
    virtual ~TRmgRoadPainter();
};

// A generated zone owns both its template metadata and the Complete-only
// connection state.  WriteMapHeader proves the player/town fields through
// +0x3c; the connection pass independently proves the bounding rectangle and
// entrance vector at +0x404.  The 0x1c-stride connection vector belongs to
// the template record reached through `slot`, not to this generated zone.
struct TRmgZone {
    TRmgTownSlot* m_slot;              // +0x00
    int m_alignment;                   // +0x04
    // H3API H3RmgZoneGenerator::townType2, INT32 at +08, commit
    // 92255ab18da784a5842ecc2b8bc0ce00e19a0c56. The surrounding town/terrain,
    // coordinates, object-count array and three vectors match this layout.
    // Retail type_map_dwelling_def::getValue at 0x5347f0 compares this
    // field with the dwelling creature's town type before valuing it.
    // Retail creature reward value 0x534324 compares this with the creature's
    // town alignment before weighting the reward by active-zone counts.
    int m_townType2;
    // chooseTerrain 0x532ab0 stores the integer ordinal from its 0..7
    // selection loop; tryPlaceMine uses the same ordinal as a bitset index.
    // There is no DC enum ABI for this Complete-only field. Keep the field
    // and its local consumer consistent instead of casting into an inferred
    // enum after every selection. Named terrain constants share the encoding.
    int m_terrain;                      // +0x0c
    TRmgMapPosition m_levelPosition;   // +0x10
    int m_boundaryRoughness;            // +0x1c: minimum of adjacent zones
    TRmgZoneBounds m_bounds;           // +0x20
    TRmgMapPosition m_position;        // +0x30: main town
    unsigned char m_active;            // +0x3c
    char m_opaque003d[3];              // +0x3d..+0x3f
    // Retail 0x54b180 relaxes graph
    // distances here; 0x54b300 converts them into randomized quest-zone
    // priorities, penalizing immediately adjacent zones. Role-derived name.
    int m_questPlacementScore;         // +0x40
    // Retail ctor 0x5329e0 clears 232 dwords beginning at +0x44.
    // Placement 0x54039a increments by object type, removal 0x54bd30
    // decrements it, and 0x546270 checks the per-zone object-type limit.
    // Role-derived name, matching the generator's global counterpart.
    int m_objectCountByType[232];      // +0x44
    // Retail +0x3e4 has vector construction/destruction. 0x53dc84 resizes
    // to the zone count; +0x53dc98 fills signed shorts with 32000 and
    // +0x53dcb1 sets this zone's own index to zero. 0x53d9ae reads a
    // distance, adds one and relaxes connected zones. Role-derived name.
    std::vector<short> m_zoneDistances;// +0x3e4
    std::vector<TPoint> m_boundary;    // +0x3f4: clipped polygon vertices
    std::vector<TPoint> m_entrances;   // +0x404

    TRmgZone(TRmgTownSlot* slot);
    void decrementObjectCount(TAdventureObjectType objectType);
    void chooseTerrain();
    ~TRmgZone();
    int getTerrain() const
    {
        return m_terrain;
    }
    const TRmgZoneBounds& getBounds() const
    {
        return m_bounds;
    }
    TRmgMapPosition getLevelPosition() const;
    void setLevelPosition(TRmgMapPosition position);
    // Template slot radius; the position filter reads it through this
    // accessor, which is what makes its first counting pass call size().
    int getSize() const
    {
        return m_slot->m_size;
    }
    unsigned char canConnect(const TRmgZone* other) const;
};

// Partial Voronoi topology recovered from TraceZoneBoundary and its caller
// at 0x53e050. The twin's owning zone identifies the region across an edge;
// following next traverses a closed polygon. Names are provisional.
struct TRmgBoundaryVertex {
    // The paired-edge constructor 0x5fcef0
    // copies a by-value point into +0/+4 and its zone into +8.
    // buildVertices 0x5fdb40 subtracts these site coordinates while
    // calculating the boundary point at +0x1c. Role-derived name.
    TPoint m_sitePosition;               // +0x00
    TRmgZone* m_zone;                   // +0x08
    TRmgBoundaryVertex* m_twin;        // +0x0c
    TRmgBoundaryVertex* m_next;        // +0x10
    // Constructor 0x5fcef0 initializes both
    // ring links to self; splice 0x5fcf60 swaps next->previous together
    // with next, preserving the backward link. Role-derived name.
    TRmgBoundaryVertex* m_previous;     // +0x14
    // Constructor clears this byte. buildVertices tests it at 0x5fdb7a,
    // writes the computed point, then sets it on three incident edges at
    // 0x5fdc7d/89/9e. +0x19..1b is natural alignment before the point.
    unsigned char m_positionComputed;  // +0x18
    TPoint m_position;                  // +0x1c

    // The 0x5fcef0 retained constructor takes two by-value point/zone
    // pairs (ret 0x18), allocating the opposite half-edge at +0x0c.
    // Its expanded twin constructor takes the existing edge pointer.
    TRmgBoundaryVertex(TPoint sitePosition, TRmgZone* zone,
        TPoint twinSitePosition, TRmgZone* twinZone);
    TRmgBoundaryVertex(TPoint sitePosition, TRmgZone* zone,
        TRmgBoundaryVertex* twin);
    // Role-derived names: 0x5fcf60 exchanges forward/backward ring links;
    // 0x5fcfa0 applies it to each half-edge and its predecessor.
    // Ordinary; both constructors expand it (see rmg_support.cpp).
    void initialize();
    void splice(TRmgBoundaryVertex* other);
    void detach();
    // Quad-edge navigation (Graphics Gems IV Sym/Onext/Oprev/Lnext/Lprev,
    // Org2d/Dest2d): these inline accessors are candidate sites for the
    // /Ob2 inliner, and their count is what makes retail refuse the fan
    // splices, distance and orientation calls in addSite, the twin splice
    // in removeEdge and the fifth createEdge in the diagram constructor.
    // Site positions return by value: addSite's coincidence test loads both
    // coordinates before comparing, as a copied temporary does.
    TRmgBoundaryVertex* getTwin() const
    {
        return m_twin;
    }
    TRmgBoundaryVertex* getNext() const
    {
        return m_next;
    }
    TRmgBoundaryVertex* getPrevious() const
    {
        return m_previous;
    }
    TRmgBoundaryVertex* getLeftNext() const
    {
        return m_twin->m_previous;
    }
    TRmgBoundaryVertex* getLeftPrevious() const
    {
        return m_next->m_twin;
    }
    TPoint getSitePosition() const
    {
        return m_sitePosition;
    }
    TPoint getOppositeSitePosition() const
    {
        return m_twin->m_sitePosition;
    }
    TRmgZone* getZone() const
    {
        return m_zone;
    }
    TRmgZone* getOppositeZone() const
    {
        return m_twin->m_zone;
    }
    // Voronoi vertex bookkeeping: the computed flag and the shared vertex.
    unsigned char isPositionComputed() const
    {
        return m_positionComputed;
    }
    void setPosition(const TPoint& position)
    {
        // A named snapshot gives buildVertices retail's final two-coordinate
        // transfer before each of its three expanded stores.
        TPoint copy = position;
        m_position = copy;
        m_positionComputed = 1;
    }
};
SIZE(TRmgBoundaryVertex, 0x24);

// The retained subdivision constructor and destructor own a root edge and
// a vector of allocated edges. The coordinator inserts zone sites, computes
// dual vertices, then looks up an edge for each site. All names are provisional.
class TRmgVoronoi {
public:
    TRmgBoundaryVertex* m_root;                 // +0x00
    std::vector<TRmgBoundaryVertex*> m_edges;  // +0x04

    TRmgVoronoi();
    ~TRmgVoronoi();
    // Retained 0x5fd390 creates and owns both halves; two point/zone pairs.
    TRmgBoundaryVertex* createEdge(TPoint first, TRmgZone* firstZone,
        TPoint second, TRmgZone* secondZone);
    TRmgBoundaryVertex* connectEdges(TRmgBoundaryVertex* first,
        TRmgBoundaryVertex* second);
    void removeEdge(TRmgBoundaryVertex* edge);
    void addSite(TPoint point, TRmgZone* zone);
    TRmgBoundaryVertex* locate(TPoint point);
    void buildVertices();
};
SIZE(TRmgVoronoi, 0x14);

SIZE(TRmgMapPosition, 0xc);
SIZE(TRmgZoneConnection, 0x1c);
SIZE(TRmgZoneBounds, 0x10);
SIZE(TRmgZone, 0x414);

enum ERmgMapVersion {
    RMG_MAP_RESTORATION_OF_ERATHIA = 0,
    RMG_MAP_ARMAGEDDONS_BLADE = 1,
    RMG_MAP_SHADOW_OF_DEATH = 2
};

// Complete-only 0x543e20 chooses one of these four initial branch segments.
// Names describe the endpoint stores; the original source spelling is unknown.
enum ERmgBranchSeedPattern {
    RMG_BRANCH_SEED_MAIN_DIAGONAL = 0,
    RMG_BRANCH_SEED_VERTICAL = 1,
    RMG_BRANCH_SEED_ANTI_DIAGONAL = 2,
    RMG_BRANCH_SEED_HORIZONTAL = 3
};

// The Complete-only map-header writer extends the object-factory evidence
// into the late generator state.  Each named field below is read or written
// at its annotated offset by retail 0x549cb0; opaque spans preserve all
// unobserved state without guessing at its source identity.
// Retail 0x537b10 reads two-int records from 0x640718 and 0x640808.
// Role-derived names; no Dreamcast RMG counterpart survives.
struct TRmgObjectLimit {
    int m_objectType;
    int m_limit;
};

// Constructor 0x536070 and destructor 0x5363b0 own the prefix through
// the progress pointer at +0xed4. Their vtable is 0x640c3c; the derived
// constructor/destructor replace it with 0x640c44. The old flat model hid
// this retained base boundary. TRmgGeneratorBase is a provisional name.
class TRmgGeneratorBase {
public:
    // time(&m_randomSeed) at 0x536140 proves VC6 time_t (long).
    long m_randomSeed;                                 // +0x004
    int m_mapVersion;                                  // +0x008
    type_random_map m_map;                             // +0x00c
    // 0x536213 calls TObjectTypeTable::load with this complete member.
    TObjectTypeTable m_objectsTxt;                     // +0x024
    std::vector<TRmgObjectPropertiesRef*> m_objectPrototypes[232]; // +0x034
    // Terrain-relation records populated by the loader and used by the scorer.
    std::vector<TRmgObjectPlacementRule> m_placementRules; // +0xeb4
    std::vector<type_object*> m_positions;             // +0xec4
    TProgressSink* m_progress;                          // +0xed4
    TRmgGeneratorBase(int width, int height, int levels,
        TProgressSink* progress, int additionalSteps, int version);
    virtual ~TRmgGeneratorBase();
    virtual void addObject(type_object* object, TRmgMapPosition position);
    // Retained 0x536200 loads object records, builds the per-type vectors,
    // then calls the placement-rule loader. Larger body not yet recovered.
    void loadObjectPrototypes();
    void readObjectPlacementRules();
    int scoreObjectPlacement(
        TRmgObjectPropertiesRef* properties, TRmgMapPosition position);
    void decorateMap();
    void decorateMapCell(TRmgMapPosition position, int progressSteps);
};
SIZE(TRmgGeneratorBase, 0xed8);

// Four fixed-count/density groups consumed by 0x544ae0; the option byte's
// gameplay meaning remains provisional, while ownership and ordering are proven.
enum ERmgTownPlacementCategory {
    RMG_TOWN_PLAYER_OPTION,
    RMG_TOWN_PLAYER_BASIC,
    RMG_TOWN_NEUTRAL_OPTION,
    RMG_TOWN_NEUTRAL_BASIC
};

class type_random_map_generator : public TRmgGeneratorBase {
public:
    unsigned char m_fixedHumanPlayers[8];              // +0x0ed8
    // Retail 0x5499fb clears nine integers at +0xee0; slot +1 is used
    // at 0x549a75/0x549ab8. Entry zero preserves the unmapped sentinel.
    int m_playerIndexMap[9];                          // +0x0ee0
    char m_opaque0f04[0x20];                          // +0x0f04
    int m_townChoices[8];                              // +0x0f24
    // The constructor seeds this object-ID counter to 1. Creation paths
    // 0x534902, 0x540cfa, 0x545104 and
    // 0x54543d take then increment the counter, storing the taken ID in
    // the new object's derived data (+0x20 for the first, +0x1c for others).
    int m_nextObjectId;                               // +0x0f44
    int m_humanPlayerCount;                            // +0x0f48
    int m_humanTeamCount;                              // +0x0f4c
    int m_computerPlayerCount;                         // +0x0f50
    int m_computerTeamCount;                           // +0x0f54
    // Role-derived names; original spellings unknown. Replaces opaque0f58.
    // 0x54b834 advances +0xf58 modulo objectPrototypes[83].size();
    // seer-hut value paths 0x534af0/0x534c80 require this prototype index.
    int m_nextSeerHutPrototypeIndex;                   // +0x0f58
    // 0x540d6d selects the key-tent subtype using +0xf5c. After placing
    // it, 0x540f68 marks its color disabled and scans for the next free one.
    int m_nextKeyTentColor;                            // +0x0f5c
    // 0x549bae clears nine alignment counts; 0x549be0..0x549c05 counts
    // active zones both by their alignment (+4) and in the total.
    int m_activeZoneCount;                             // +0x0f60
    int m_activeZoneCountsByAlignment[9];              // +0x0f64
    unsigned char m_disabledHeroes[156];               // +0x0f88
    // Role-derived names; original spellings unknown. Replaces opaque1024.
    // Ctor 0x537cc6 clears 144 bytes. Quest selection 0x54b490 excludes
    // marked artifacts; successful placement 0x54b813 marks the chosen ID.
    unsigned char m_usedQuestArtifacts[144];           // +0x1024
    // 0x54b4f1 latches this when fewer than 20 eligible artifacts remain;
    // seer-hut value paths 0x534b0c/0x534c9c reject further candidates.
    unsigned char m_questArtifactPoolLow;              // +0x10b4
    // +0x10b5..0x10b7: implicit alignment before the next int.
    int m_waterContent;                                // +0x10b8
    int m_monsterStrength;                             // +0x10bc
    // Retail ctor 0x537b10 initializes a Dinkumware string at +0x10c0.
    // 0x54999c calls basic_string::assign with the selected template's
    // leading name string; 0x537fcc destroys it with basic_string::_Tidy.
    // The old templateName pointer at +0x10c4 was only its buffer member.
    // Replaces synthetic opaque10c0 and the first half of opaque10c8.
    std::string m_templateName;                        // +0x10c0
    // Retail 0x538450 inserts TRmgTemplate pointers into this vector;
    // 0x537e84 destroys its elements with TRmgTemplate::~TRmgTemplate,
    // and 0x537fbc destroys the vector itself. Role-derived name.
    // Replaces the remaining half of synthetic opaque10c8.
    std::vector<TRmgTemplate*> m_templates;            // +0x10d0
    std::vector<TRmgZone*> m_zones;                    // +0x10e0
    std::vector<type_treasure_def*> m_objectGenerators; // +0x10f0
    std::vector<unsigned char> m_disabledKeyTents;     // +0x1100
    int m_objectCountByType[232];                      // +0x1110
    std::vector<TRmgMapPosition> m_roadTargets;        // +0x14b0
    std::vector<type_object*> m_monolithsOneWay;       // +0x14c0
    std::vector<type_object*> m_monolithsTwoWay;       // +0x14d0

    // Retail 0x537b10 forwards dimensions/progress/version to the base,
    // then initializes the derived template, zone and object-generator state.
    type_random_map_generator(int width, int height, int levels,
        int humanPlayers, int humanTeams, int computerPlayers, int computerTeams,
        int waterContent, int monsterStrength, TProgressSink* progress, int version);
    void loadTemplates();
    // Role-derived from generation coordinator 0x549b30 and placement 0x545250.
    void placeMines();
    // Provisional roles from the Complete-only connection coordinator.
    void prepareZoneConnections();
    void expandObstacleClearance();
    void prepareWaterZoneConnections(TRmgZone* zone);
    void createWaterZoneIsland(const TRmgZoneBounds& bounds, int level);
    void floodWaterZoneDistances(TRmgMapPosition position, int zoneIndex);
    void buildZoneConnectionPaths();
    void placeExtraMines(TRmgZone* zone);
    unsigned char placeMineSite(type_object* object, TRmgZone* zone,
        unsigned char startingMine, int spacing);
    unsigned char tryPlaceMine(TRmgZone* zone, int resource,
        unsigned char startingMine, int spacing);
    void placePrimaryTown(TRmgZone* zone);
    unsigned char tryPlacePrimaryTown(TRmgZone* zone, int alignment,
        int player, unsigned char townOption);
    void initializeZones(TRmgTemplate* mapTemplate);
    void positionZone(TRmgZone* zone, int mapSize);
    void appendZonePositions(TRmgZone* center, TRmgZone* zone,
        std::vector<TRmgMapPosition>& candidates);
    void getInitialZoneBounds(int& minimumY, int& minimumX,
        int& maximumY, int& maximumX) const;
    void paintZoneTerrain();
    void calculateZoneBounds();
    void recenterZone(TRmgZone* zone);
    void insetIslandZone(TRmgZone* zone);
    // Complete-only 0x53cf50 marks the inset island interior; provisional name.
    void fillIslandInterior(TRmgZone* zone);
    void drawIslandBoundary(TPoint from, TPoint to, int zoneIndex, int level, int roughness);
    void placeAdditionalTowns(TRmgZone* zone);
    unsigned char tryPlaceAdditionalTown(TRmgZone* zone, int alignment,
        int player, unsigned char townOption, int spacing);
    void prepareJunctionZone(TRmgZone* zone);
    void connectJunctionEntrance(TPoint from, TPoint to, TRmgZone* zone);
    void placeZoneTreasures(TRmgZone* zone);
    type_object* createTreasureObject(TRmgZone* zone, int minimum, int maximum,
        int* value, unsigned char primary, unsigned char allowTerrainDependent,
        unsigned char compact, TRmgMapPosition position);
    int fillTreasureGroup(TRmgZone* zone, TRmgTreasureGroup* group,
        unsigned char alternate, int value);
    unsigned char assembleTreasureGroup(TRmgZone* zone, TRmgTreasureGroup* group,
        unsigned char alternate, int minimum, int maximum);
    unsigned char placeTreasureGroup(TRmgTreasureGroup* group, TRmgZone* zone, int spacing);
    unsigned char canPlaceTreasureGroup(TRmgTreasureGroup* group,
        TRmgMapPosition position, TRmgZone* zone);
    void commitTreasureGroup(TRmgTreasureGroup* group, TRmgMapPosition position);
    void decorateUnderground();
    unsigned char generate();
    unsigned char writeMap(TAbstractFile* outfile);
    virtual ~type_random_map_generator();
    virtual void addObject(type_object* object, TRmgMapPosition position);

    inline int getSerializedMapVersion() const
    {
        switch (m_mapVersion) {
        case RMG_MAP_RESTORATION_OF_ERATHIA:
            return 14;
        case RMG_MAP_ARMAGEDDONS_BLADE:
            return 21;
        case RMG_MAP_SHADOW_OF_DEATH:
            return 28;
        }
    }

    void initializeObjectGenerators();
    int selectPrisonHero();
    unsigned char canPlaceZone(TRmgZone* zone);
    void buildZoneBoundaries(TRmgTemplate* mapTemplate, int level);
    // Complete-only 0x53d8e0 propagates each signed-short distance column
    // from one zone through the template connection graph.
    void propagateZoneDistances(TRmgZone* zone);
    void fillZoneArea(TRmgZone* zone, TRmgBoundaryVertex* first);
    void joinExtraZones(int originalZones, TRmgVoronoi* diagram);
    int countPlacedZoneConnections(TRmgZone* zone) const;
    void filterZonePositions(
        TRmgZone* zone, std::vector<TRmgMapPosition>& candidates, int mapSize);
    void drawIrregularZoneBoundary(
        TPoint from, TPoint to, int zoneIndex, int level, int roughness);
    void drawStraightZoneBoundary(
        TPoint from, TPoint to, int zoneIndex, int level);
    void traceZoneBoundary(TRmgBoundaryVertex* first, unsigned char irregular);
    unsigned char createGroundConnection(
        TRmgZone* source,
        TRmgZoneConnection* connection,
        std::vector<TRmgMapItem*>* borderItems,
        std::vector<TRmgMapPosition>* borderPositions);
    void floodConnectionRegion(TRmgMapPosition position);
    // Earlier provisional name: CreateBorderConnection/createBorderConnection.
    // Retail 0x541ad0 selects objectPrototypes[SHIPYARD] and places it beside
    // reachable water. No Dreamcast RMG name is available.
    unsigned char createShipyardConnection(
        TRmgZone* source, TRmgZoneConnection* connection);
    unsigned char canPlaceShipyard(TRmgMapPosition position);
    unsigned char createSubterraneanGate(
        TRmgZone* source, TRmgZoneConnection* connection);
    // Complete-only 0x542b00 places and marks a monolith entrance border.
    unsigned char placeMonolithBorder(TRmgMapPosition position, TRmgZone* zone);
    void createMonolithConnection(
        TRmgZone* source,
        TRmgZoneConnection* connection,
        int prototypeIndex);
    void connectZones();
    // Retail 0x543e20: random midpoint displacement, queued side branches,
    // then terrain and border cleanup. No Dreamcast RMG names survive.
    bool contains(const TPoint& point) const;
    void carveBranchingPaths();
    void repairWaterZoneBorders();
    // Complete-only roles proved by the predecessor walk at 0x5408e0 and
    // the surrounding connection-cell updates at 0x540fc0.
    void openConnectionPath(TRmgMapPosition position, unsigned char narrow);
    void markBorderObjectArea(TRmgMapPosition position, int direction);
    int placeBorderObject(
        TRmgMapPosition position, int count, TRmgZone* zone);
    type_object* createGuard(int value, TRmgZone* zone);
    unsigned char placeObjectInZone(type_object* object, TRmgZone* zone);
    void placeGuard(TRmgMapPosition position, int value);
    int getMineGuardValue(int resource, const TRmgZone* zone) const;
    // Complete-only prototype/subtype/terrain filter at retail 0x546040.
    TRmgObjectPropertiesRef* selectObjectPrototype(
        int terrain, int objectType, int subtype);
    void resetMovementCosts();
    // Provisional Complete-only spelling: the 0x548290 road-target pass is
    // the sole direct caller, and the body builds the road traversal costs.
    void buildRoadCostMap(TRmgMapPosition position);
    void createRoads();
    // Retail 0x54b490, called by the quest-artifact writable override.
    // It changes the artifact prototype and attempts to place its seer hut;
    // success transfers ownership to the generated map. Retained thiscall
    // boundary with one mutable artifact argument.
    unsigned char placeQuestArtifact(rmgQuestArtifactObject* object);
    // Retained Complete-only helpers at 0x54b180 and 0x54b300. The quest
    // artifact caller supplies its origin zone and the prepared hut group.
    // Original names are unavailable; the graph and placement roles are proven.
    void calculateQuestZoneDistances(TRmgZone* origin);
    unsigned char placeQuestGroup(TRmgTreasureGroup* group, TRmgZone* origin);
    // Retained helpers used by the key-tent override at 0x5338e0.
    // 0x54b8c0 finds objectPrototypes[9] of the same color and tries a
    // guarded treasure group; 0x54bc50 removes the old object's map marks,
    // counts and list entry without deleting the object itself.
    unsigned char placeKeyTentGuard(type_object* object, int maxValue);
    void setHumanPlayer(int seat);
    void setTownChoice(int seat, int town);
    void removeObject(type_object* object);
    // Retail 0x546190: zone, value range, output value, three byte flags,
    // then a by-value position (ret 0x28). Flags bypass the object-trait
    // filter, allow terrain-dependent definitions, and rank value per area.
    type_object* generateTreasure(TRmgZone* zone, int minValue, int maxValue,
        int* value, unsigned char ignoreObjectTraits,
        unsigned char allowTerrainDependent, unsigned char preferValueDensity,
        TRmgMapPosition position);
    // Retail 0x548040 walks predecessor runs for the caller at 0x548408.
    // The Complete-only name is provisional; the by-value ABI is proven.
    unsigned char paintRoad(TRmgMapPosition position, int roadType);
    // Provisional spelling: retail's water-wheel caller and the river-delta
    // object selection prove the role; the Dreamcast build has no RMG TU.
    void createRiver(TRmgMapPosition source);
    void markRiverObjectTargets();
    void markRiverTargets();
    void markRiverCoastTarget(TRmgMapPosition position, int direction);
    void createRiverToObject(TRmgMapPosition source);
    void createRivers();
    void writeMapHeader(TAbstractFile* outfile);
};

SIZE(TRmgMapPosition, 0x0c);
SIZE(TPoint, 0x08);
SIZE(TRmgGridPoint, 0x08);
SIZE(TRmgRiverDeltaOffset, 0x08);
SIZE(TRmgMovementCost, 0x04);
SIZE(TRmgZoneCellState, 0x04);
SIZE(TRmgGroundTile, 0x04);
SIZE(TRmgGroundTileData, 0x04);
SIZE(TRmgConnectionDecoration, 0x04);
SIZE(TRmgObjectPlacementRule, 0x4c);
SIZE(TRmgObjectPropertiesRef, 0xe8);
SIZE(type_object, 0x1c);
SIZE(rmgOwnableObject, 0x1c);
SIZE(TRmgMapItem, 0x30);
SIZE(type_random_map, 0x18);
SIZE(TRmgMapInterface, 0x04);
SIZE(TRmgMapAdapterInterface, 0x04);
SIZE(TRmgRoadMapAdapterInterface, 0x04);
SIZE(TRmgMapAdapter, 0x08);
SIZE(TRmgLinePainterInterface, 0x0c);
SIZE(TRmgLinePainter, 0x10);
SIZE(TRmgRoadLinePainter, 0x10);
SIZE(TRmgLineWalker, 0x10);
SIZE(TRmgRiverPainter, 0x20);
SIZE(TRmgRoadPainter, 0x20);
SIZE(type_random_map_generator, 0x14e0);

// Retail 0x6824e0 is indexed by the creature-traits level dword before
// type_black_box_creature_def divides by that creature's AI value.
DATA(0x006824E0) extern int g_rmgCreatureValueByLevel[];

#endif  // HOMM3_RMG_H
