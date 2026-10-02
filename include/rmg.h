// Random-map generator declarations. Most names describe recovered roles
// rather than original source spellings.
#ifndef HOMM3_RMG_H
#define HOMM3_RMG_H

#include "va.h"
#include "homm3_bool.h"

#include <bitset>
#include <string>
#include <vector>

#include "advmgr_objects.h"
#include "terrain_type.h"

class TAbstractFile;
class TSpreadsheetResource;
class type_random_map_generator;
struct TRmgTemplateZone;
struct TRmgZone;
struct rmgTerrainTile;
struct TPoint;
struct TObjectType;
struct TRmgObjectPropertiesRef;
class type_object;

// Progress reporting interface driven by the random-map generator.
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

// Treasure definition: an object type/subtype with a value and placement
// density, plus a factory for the generated object.
class type_treasure_def {
public:
    int m_objectType;
    int m_subtype;
    int m_value;
    int m_density;

    type_treasure_def(int objectType, int subtype, int value, int density);

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
    virtual b8 isTerrainDependent();
};

SIZE(type_treasure_def, 0x14);

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
    // Stack size: the level's reward value divided by AI value, rounded.
    int m_creatureCount;

    type_black_box_creature_def(int creatureType);
    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
};

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
        : type_treasure_def(BLACK_BOX, 0, value, 20)
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
        : type_treasure_def(BLACK_BOX, 0, value, 5)
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
        : type_treasure_def(BLACK_BOX, 0, value, 2)
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
        : type_treasure_def(BORDER_TENT, subtype, value, 10)
    {
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
    virtual b8 isTerrainDependent();
};

class type_dwelling_def : public type_treasure_def {
public:
    inline type_dwelling_def(int subtype)
        : type_treasure_def(CREATURE_GENERATOR_1, subtype, -1, 40)
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
        : type_treasure_def(PRISON, 0, value, 30)
    {
        this->m_experience = experience;
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_scholar_def : public type_treasure_def {
public:
    inline type_scholar_def()
        : type_treasure_def(SCHOLAR, 0, 1500, 100)
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
        m_objectType = SEER;
        m_subtype = questIndex;
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
    virtual b8 isTerrainDependent();
};

class type_quest_experience_def : public type_treasure_def {
public:
    int m_experience;

    inline type_quest_experience_def(
        int questIndex, int value, int experience)
        : type_treasure_def(SEER, questIndex, value, 10)
    {
        this->m_experience = experience;
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
    virtual b8 isTerrainDependent();
};

class type_quest_gold_def : public type_treasure_def {
public:
    int m_gold;

    inline type_quest_gold_def(int questIndex, int value, int gold)
        : type_treasure_def(SEER, questIndex, value, 10)
    {
        this->m_gold = gold;
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
    virtual b8 isTerrainDependent();
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

struct TRmgVector {
    int m_x;
    int m_y;

    TRmgVector() {}
    TRmgVector(int newX, int newY) : m_x(newX), m_y(newY) {}

    int length() const;
    TRmgVector operator+(TRmgVector other) const;
    TRmgVector operator*(int scale) const;
    TRmgVector operator/(int divisor) const;
    int dot(TRmgVector other) const
    {
        return m_y * other.m_y + m_x * other.m_x;
    }
};

// Signed 32-bit squared length; large values overflow.
inline int getRmgSquaredNorm(int x, int y)
{
    return x * x + y * y;
}

// Signed map-plane point; ordered by row, then column.
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
    int getX() const { return m_x; }
    int getY() const { return m_y; }
    TPoint& operator+=(const TPoint& offset);
};

// Map position: a plane point plus level (0 surface, 1 underground).
struct TRmgMapPosition : TPoint {
    int m_z;

    TRmgMapPosition() {}
    TRmgMapPosition(int newX, int newY, int newZ);

    TRmgMapPosition& operator+=(const TPoint& offset);
    TRmgMapPosition& operator-=(const TPoint& offset);
};

TRmgMapPosition operator+(TRmgMapPosition position, TPoint offset);

// Template connection to another zone: value, guard/border policy,
// whether it has been built, and the player counts for which it applies.
struct TRmgZoneConnection {
    TRmgTemplateZone* m_destination;             // +0x00
    int m_value;                             // +0x04
    b8 m_unguarded;               // +0x08
    b8 m_placeBorderObjects;      // +0x09
    b8 m_connected;               // +0x0a
    b8 isConnected() const;
    void setConnected();
    // Template columns 81..84. The connection exists only when the human
    // count and the total player count fall within these ranges.
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

// Template columns 14..21: player-owned and neutral town counts and
// densities. BASIC/OPTION mirror the serialized town-option byte.
enum ERmgTownPlacementParameter {
    RMG_TOWN_PLAYER_BASIC_COUNT = 0,
    RMG_TOWN_PLAYER_OPTION_COUNT = 1,
    RMG_TOWN_PLAYER_BASIC_DENSITY = 2,
    RMG_TOWN_PLAYER_OPTION_DENSITY = 3,
    RMG_TOWN_NEUTRAL_BASIC_COUNT = 4,
    RMG_TOWN_NEUTRAL_OPTION_COUNT = 5,
    RMG_TOWN_NEUTRAL_BASIC_DENSITY = 6,
    RMG_TOWN_NEUTRAL_OPTION_DENSITY = 7
};

struct TRmgTemplateZone {
    int m_zoneIndex;                    // +0x00
    int m_kind;                         // +0x04: ERmgTemplateZoneKind
    int m_size;                         // +0x08
    int m_minimumHumanPlayers;          // +0x0c
    int m_maximumHumanPlayers;          // +0x10
    int m_minimumPlayers;               // +0x14
    int m_maximumPlayers;               // +0x18
    int m_playerIndex;                  // +0x1c
    int m_townPlacement[8];             // +0x20: ERmgTownPlacementParameter
    // Template column 22: neutral towns use the zone's town alignment.
    b8 m_neutralTownsMatchZone; // +0x40
    b8 m_allowedTowns[9];    // +0x41
    int m_mineCounts[7];                // +0x4c: indexed by resource
    int m_mineDensities[7];             // +0x68: indexed by resource
    // Use the aligned town's native terrain.
    b8 m_useNativeTerrain;
    b8 m_allowedTerrain[8];  // +0x85
    int m_monsterStrength;              // +0x90
    // Template column 56: restrict guards to the zone's town alignment.
    b8 m_guardsMatchZone;    // +0x94
    b8 m_allowedMonsters[10]; // +0x95
    TRmgTreasureRange m_treasure[3];     // +0xa0
    std::vector<TRmgZoneConnection> m_connections; // +0xc4

    int selectAllowedTown();
    TRmgZoneConnection* findConnection(int destinationZone);
};
SIZE(TRmgTemplateZone, 0xd4);

// One rmg.txt template: name, zones and supported map-size range.
struct TRmgTemplate {
    std::string m_name;                  // +0x00
    std::vector<TRmgTemplateZone*> m_zones;   // +0x10
    char m_opaque0020[0x10];
    int m_minimumSize;                  // +0x30
    int m_maximumSize;                  // +0x34

    ~TRmgTemplate();
    TRmgTemplateZone* findZone(int zoneIndex);
};
SIZE(TRmgTemplate, 0x38);

void readRmgTemplateZones(
    const TSpreadsheetResource* sheet, TRmgTemplate* mapTemplate,
    int firstRow, int endRow, int humanPlayers, int computerPlayers,
    int mapVersion);

int getRmgGuardValue(int value, int strength);

// The eight neighbour directions, clockwise.
enum ERmgDirectionLimits {
    RMG_DIRECTION_COUNT = 8
};

// Point/vector arithmetic used by the Voronoi zone layout.
TPoint operator+(TPoint point, TRmgVector offset);
TRmgVector operator-(TPoint left, TPoint right);

int getRmgPointOrientation(TPoint first, TPoint second, TPoint third);
int getRmgSquaredDistance(TPoint first, TPoint second);

// Unsigned map-grid coordinate used by the terrain painters; ordered by
// row, then column. Signed geometry uses TPoint.
template<class Coordinate> struct TRmgCoordinatePoint {
    Coordinate m_x;
    Coordinate m_y;

    TRmgCoordinatePoint() {}

    // VA instance: TRmgCoordinatePoint<unsigned int>::TRmgCoordinatePoint(const unsigned int&, const unsigned int&)
    VA(0x005B76B0, 0x18)
    TRmgCoordinatePoint(const Coordinate& newX, const Coordinate& newY)
        : m_x(newX), m_y(newY) {}
    TRmgCoordinatePoint(const TPoint& point);

    Coordinate getX() const { return m_x; }
    Coordinate getY() const { return m_y; }
    void setX(Coordinate newX) { m_x = newX; }
    void setY(Coordinate newY) { m_y = newY; }

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

    // Empty accumulator for zone and treasure-group bounds (+/-32000).
    void resetEmpty()
    {
        m_minimumX = 32000;
        m_minimumY = 32000;
        m_maximumX = -32000;
        m_maximumY = -32000;
    }

    // Extend half-open bounds to include one unit cell.
    void includeCell(int x, int y)
    {
        m_minimumX = x < m_minimumX ? x : m_minimumX;
        m_minimumY = y < m_minimumY ? y : m_minimumY;
        int maximumX = x + 1;
        int maximumY = y + 1;
        m_maximumX = maximumX > m_maximumX ? maximumX : m_maximumX;
        m_maximumY = maximumY > m_maximumY ? maximumY : m_maximumY;
    }
};

TPoint clipRmgBoundaryPoint(
    const TRmgZoneBounds& bounds, TPoint point, TPoint toward);

// Pending rectangle of the midpoint-displacement island mask. Corners are
// (minX,minY), (minX,maxY), (maxX,minY), (maxX,maxY); variation bounds the
// random displacement.
struct TRmgNoiseRegion {
    TRmgZoneBounds m_bounds;
    int m_corners[4];
    int m_variation;
};
SIZE(TRmgNoiseRegion, 0x24);

// Edge midpoints of a noise region: averages of corners 0/2, 0/1, 1/3 and
// 2/3. Displacements are drawn in minX/minY/maxX/maxY order, then the centre.
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

enum ERmgShipyardConstants {
    RMG_SHIPYARD_WATER_OFFSET_COUNT = 4
};

// Requested water content; the generator resolves RANDOM to one of the
// other three.
enum ERmgWaterContent {
    RMG_WATER_NONE = 0,
    RMG_WATER_NORMAL = 1,
    RMG_WATER_ISLANDS = 2,
    RMG_WATER_RANDOM = 3
};

// Guard creature limits. On RoE maps creatures from 118 are excluded but only
// those below 117 are evaluated, so 117 slips through (retail bug).
enum ERmgGuardConstants {
    RMG_GUARD_CREATURE_COUNT = 145,
    RMG_GUARD_ROE_CREATURE_LIMIT = 117,
    RMG_GUARD_ROE_EXCLUDED_FIRST = 118,
    RMG_GUARD_MAXIMUM_COUNT = 100,
    RMG_GUARD_DISPOSITION = 3
};

// River-delta placement offset from the river's end, per direction.
struct TRmgRiverDeltaOffset {
    int m_x;
    int m_y;

    TRmgRiverDeltaOffset(int newX, int newY) : m_x(newX), m_y(newY) {}
};

class type_object;

struct TRmgMovementCost {
    unsigned m_cost : 16;
    // Cost of reaching this cell in a flood from another zone; ranks
    // ground-connection crossings. Water zones reuse it for distance from
    // their edge (cardinal 2, diagonal 3).
    unsigned m_zonePathCost : 16;
};

// Per-cell zone id and distance to the nearest object entrance (cardinal 2,
// diagonal 3), used to space placements.
struct TRmgZoneCellState {
    unsigned m_objectDistance : 16;
    signed m_zone : 8;
    // Neighbouring zone whose connection-cost flood crossed into this cell
    // (-1 none); m_connectionDirection points back into it.
    signed m_connectionZone : 8;
};

// Packed terrain, river and road types and the terrain and river frames of
// one map cell; the road frame is in TRmgGroundTileData.
struct TRmgGroundTile {
    signed m_landType : 6;
    signed m_terrainFrame : 8;
    signed m_riverType : 4;
    signed m_riverFrame : 8;
    signed m_roadType : 4;
    unsigned m_unknown30 : 2;
};

struct TRmgGroundTileData {
    signed m_roadFrame : 8;
    unsigned m_blockedDirections : 4;
    unsigned m_connectionDirection : 3;
    // Sprite reflections of the terrain, river and road layers.
    unsigned m_terrainFlipX : 1;
    unsigned m_terrainFlipY : 1;
    unsigned m_riverFlipX : 1;
    unsigned m_riverFlipY : 1;
    unsigned m_roadFlipX : 1;
    unsigned m_roadFlipY : 1;
    unsigned m_coastal : 1;
    // Object entrance (trigger) cell.
    unsigned m_roadEntrance : 1;
    // On an assembled treasure group's traced outline (group map only).
    unsigned m_placementOutline : 1;
    unsigned m_connectionVisited : 1;
    // Not blocked by an object.
    unsigned m_roadPassable : 1;
    unsigned m_borderObject : 1;
    // Kept clear for generated paths; obstacle footprints may not cover it.
    unsigned m_pathClearance : 1;
    // Set on zone cells whose terrain paintZoneTerrain paints.
    unsigned m_paintZoneTerrain : 1;
    // River painted here, or a mountain, lake or gem-mine cell rivers flow to.
    unsigned m_hasRiver : 1;
    unsigned m_riverTarget : 1;
    // On or beside a painted river; coast-bound rivers may not enter it.
    unsigned m_nearRiver : 1;
};

// Pending border-guard cell and the guard's key colour.
struct TRmgConnectionDecoration {
    unsigned m_present : 1;
    unsigned m_guardColor : 4;
    unsigned m_unknown05 : 27;
};

// One rand_trn.txt row: per-terrain scores and neighbour scores indexed by
// rule id, used to score object placement.
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

// Shared, reference-counted placement data for one objects.txt prototype;
// outline and overlap priorities are built lazily.
struct TRmgObjectPropertiesRef {
    TObjectType* m_prototype;              // +0x00
    // First recommended terrain.
    int m_preferredTerrain;               // +0x04, rand_trn.txt rule binding
    unsigned m_refCount;                   // +0x08
    int m_prototypeIndex;
    TRmgObjectPlacementRule* m_placementRule; // +0x10
    std::vector<TPoint> m_outline;         // +0x14
    int m_overlapPriorities[8][6];         // +0x24
    b8 m_prioritiesInitialized; // +0xe4
    char m_pad00e5[3];

    TRmgObjectPropertiesRef(TObjectType* prototype);

    void buildOutline();
    void buildOverlapPriorities();
};

class type_object {
public:
    TRmgObjectPropertiesRef* m_properties; // +0x04
    TRmgMapPosition m_position;             // +0x08
    b8 m_candidateCovers;
    b8 m_candidateBehind;
    b8 m_adjacentToCandidate;
    b8 m_overlapsCandidate;
    b8 m_blockedByCandidate;
    char m_tailPadding[3];

    type_object(TRmgObjectPropertiesRef* newProperties);
    TRmgMapPosition getPosition() const;

    void clearPlacementMarks();

    b8 isPlacementTouched() const
    {
        return m_adjacentToCandidate || m_blockedByCandidate || m_overlapsCandidate;
    }

    // Different overlapping cells can require the candidate to be both
    // behind and in front of this object, which invalidates its placement.
    b8 hasConflictingPlacementOrder() const
    {
        return m_candidateBehind && m_candidateCovers;
    }

    virtual ~type_object();
    // Undoes a discarded object's reservation; only prisons reserve (a hero).
    virtual void releaseReservation();
    // Runs once the object's treasure group is committed: key tents and quest
    // artifacts place their guard or seer hut, or are replaced by a treasure.
    // The success result is ignored.
    virtual b8 completePlacement();
    virtual void write(TAbstractFile* outputFile, int version);
};

// Guard monster stack: id, count and disposition are written to the map.
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
    virtual void write(TAbstractFile* outputFile, int version);
};
SIZE(rmgMonsterObject, 0x2c);

// Town with its owner and town option.
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
    virtual void write(TAbstractFile* outputFile, int version);
};
SIZE(rmgTownObject, 0x28);

// Ownable object such as a shipyard; written with an unowned player byte
// and three reserved bytes.
class rmgOwnableObject : public type_object {
public:
    rmgOwnableObject(TRmgObjectPropertiesRef* properties)
        : type_object(properties) {}
    virtual void write(TAbstractFile* outputFile, int version);
};

// Artifact; its record ends with one zero byte.
class rmgArtifactObject : public type_object {
public:
    rmgArtifactObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outputFile, int version);
};
SIZE(rmgArtifactObject, 0x1c);

class rmgResourceObject : public type_object {
public:
    rmgResourceObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outputFile, int version);
};
SIZE(rmgResourceObject, 0x1c);

// Pandora's Box with experience, resource, creature or spell rewards.
class rmgBlackBoxObject : public type_object {
public:
    int m_experience;                  // +0x1c
    int m_resources[7];                // +0x20, gold at +0x38
    int m_creatureType;                // +0x3c, -1 means no creature reward
    int m_creatureCount;               // +0x40
    std::vector<int> m_spells;         // +0x44

    rmgBlackBoxObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outputFile, int version);
};
SIZE(rmgBlackBoxObject, 0x54);

// Seer hut asking for an artifact and granting one reward.
class rmgSeerHutObject : public type_object {
public:
    int m_artifact;                    // +0x1c, required quest artifact
    int m_experience;                  // +0x20
    int m_resourceType;                // +0x24, defaults to gold (6)
    int m_resourceCount;               // +0x28
    int m_creatureType;                // +0x2c, defaults to -1
    int m_creatureCount;               // +0x30

    rmgSeerHutObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outputFile, int version);
};
SIZE(rmgSeerHutObject, 0x34);

// Quest artifact that owns its pending seer hut until placement hands the
// hut to the map; the pointer is cleared on success and failure alike.
class rmgQuestArtifactObject : public rmgArtifactObject {
public:
    type_random_map_generator* m_generator; // +0x1c
    rmgSeerHutObject* m_seerHut;             // +0x20
    type_treasure_def* m_definition;        // +0x24

    rmgQuestArtifactObject(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, rmgSeerHutObject* seerHut,
        type_treasure_def* definition);
    virtual ~rmgQuestArtifactObject();
    virtual b8 completePlacement();
};
SIZE(rmgQuestArtifactObject, 0x28);

// Keymaster's tent. Once committed it tries to place a matching border
// guard, and is replaced by another treasure if that fails.
class rmgKeyTentObject : public type_object {
public:
    type_random_map_generator* m_generator; // +0x1c
    int m_value;                           // +0x20

    rmgKeyTentObject(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, int value);
    virtual b8 completePlacement();
};
SIZE(rmgKeyTentObject, 0x24);

class rmgScholarObject : public type_object {
public:
    rmgScholarObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outputFile, int version);
};
SIZE(rmgScholarObject, 0x1c);

class rmgShrineObject : public type_object {
public:
    rmgShrineObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outputFile, int version);
};
SIZE(rmgShrineObject, 0x1c);

// Spell scroll; the spell is written as one byte.
class rmgSpellScrollObject : public type_object {
public:
    int m_spell; // +0x1c
    rmgSpellScrollObject(TRmgObjectPropertiesRef* properties, int spell);
    virtual void write(TAbstractFile* outputFile, int version);
};
SIZE(rmgSpellScrollObject, 0x20);

class rmgWitchHutObject : public type_object {
public:
    rmgWitchHutObject(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outputFile, int version);
};
SIZE(rmgWitchHutObject, 0x1c);

// Prison hero; releases the generator's hero reservation if rejected.
class rmgHeroObject : public type_object {
public:
    type_random_map_generator* m_generator; // +0x1c
    int m_objectId;                         // +0x20
    int m_heroIndex;                        // +0x24
    int m_experience;                       // +0x28, prison definition experience

    rmgHeroObject(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, const int& objectId, int heroIndex,
        int experience);

    virtual void releaseReservation();
    virtual void write(TAbstractFile* outputFile, int version);
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
    void write(TAbstractFile* outputFile);
    // Sets only the terrain type, frame and flips.
    void setTerrain(int terrain, int frame,
        b8 flipX, b8 flipY);

    bool hasRiver() const { return m_tileData.m_hasRiver != 0; }
    bool isRiverTarget() const { return m_tileData.m_riverTarget != 0; }
    bool isNearRiver() const { return m_tileData.m_nearRiver != 0; }

    // Any object footprint (entrance or blocked cell) covers this cell.
    b8 hasObjects() const
    {
        return m_objects.size() != 0;
    }

    // Type of the object whose entrance this cell is; only valid on an
    // entrance, whose owner is the cell's first object.
    TAdventureObjectType getEntranceObjectType() const
    {
        return m_objects[0]->m_properties->m_prototype->getObjectType();
    }

    b8 shouldPaintZoneTerrain() const
    {
        return m_tileData.m_paintZoneTerrain;
    }

    b8 isRoadEntrance() const
    {
        return m_tileData.m_roadEntrance;
    }

    b8 hasPathClearance() const
    {
        return m_tileData.m_pathClearance;
    }

    b8 isPassableLand() const
    {
        return m_tileData.m_roadPassable && getLandType() != eTerrainRock;
    }

    b8 hasBorderObject() const
    {
        return m_tileData.m_borderObject;
    }

    b8 isPlacementOutline() const
    {
        return m_tileData.m_placementOutline;
    }
    b8 isConnectionVisited() const
    {
        return m_tileData.m_connectionVisited;
    }
    void setConnectionVisited()
    {
        m_tileData.m_connectionVisited = true;
    }
    int getLandType() const
    {
        return m_tile.m_landType;
    }

    // Records a path cost and its predecessor tile.
    void setMovementCost(int cost, TRmgMapPosition previous)
    {
        m_movement.m_cost = cost;
        m_previousTile = previous;
    }

    // Resets the path cost to 32000 (unreached) with no predecessor.
    void resetMovement()
    {
        m_movement.m_cost = 32000;
        m_previousTile = TRmgMapPosition(-1, -1, -1);
    }

    // Water-zone spacing: distance and incoming direction; sets the
    // connection zone to 0 (reset before connections are built).
    void setWaterZoneDistance(unsigned int cost, int direction)
    {
        m_movement.m_zonePathCost = cost;
        m_tileData.m_connectionDirection = direction;
        m_zoneState.m_connectionZone = 0;
    }

    // Opens a generated path here unless a connection protects the cell.
    void openPath()
    {
        if (!m_connection.m_present) {
            m_tileData.m_borderObject = false;
            m_tileData.m_pathClearance = true;
        }
    }

    // Schedules border filling here unless a connection protects the cell.
    void markBorderObject()
    {
        if (!m_connection.m_present) {
            m_tileData.m_pathClearance = false;
            m_tileData.m_borderObject = true;
        }
    }

    // Clears the border-filling mark unless a connection protects the cell.
    void clearBorderObject()
    {
        if (!m_connection.m_present)
            m_tileData.m_borderObject = false;
    }

    // Removing a border connection also opens its cell.
    void clearBorderConnection()
    {
        m_connection.m_present = false;
        m_connection.m_guardColor = 0;
        openPath();
    }

    // An existing connection keeps its tile flags but takes the new colour.
    void markBorderConnection(int color)
    {
        markBorderObject();
        m_connection.m_guardColor = color;
        m_connection.m_present = true;
    }

    // Passable land with path clearance that is not an object entrance.
    b8 isClearOutlineCell() const
    {
        return !isRoadEntrance() && isPassableLand() && hasPathClearance();
    }

    // Allows obstacles here unless a connection protects the cell.
    void releasePathClearance()
    {
        if (!m_connection.m_present)
            m_tileData.m_pathClearance = false;
    }
};

// Terrain-layer view of a map used by the terrain painter.
class TRmgMapInterface {
public:
    virtual ~TRmgMapInterface();
    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile) = 0;
    virtual void setFrame(const TRmgGridPoint& point, int value) = 0;
#if defined(HOMM3_TARGET_MAC)
    virtual TRmgGridPoint getSize() = 0;
#else
    virtual TRmgGridPoint& getSize(TRmgGridPoint& output) = 0;
#endif
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point) = 0;
    virtual int getTerrain(const TRmgGridPoint& point) = 0;
    virtual int getFrame(const TRmgGridPoint& point) = 0;
};

class TRmgRiverMapAdapterInterface {
public:
    virtual ~TRmgRiverMapAdapterInterface();
    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile) = 0;
    virtual void setLineType(const TRmgGridPoint& point, int value) = 0;
    virtual TRmgGridPoint getSize() = 0;
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point) = 0;
    virtual int getLineType(const TRmgGridPoint& point) = 0;
    virtual int getTerrain(const TRmgGridPoint& point) = 0;
};

class TRmgRoadMapAdapterInterface {
public:
    virtual ~TRmgRoadMapAdapterInterface();
    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile) = 0;
    virtual void setLineType(const TRmgGridPoint& point, int value) = 0;
    virtual TRmgGridPoint getSize() = 0;
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point) = 0;
    virtual int getLineType(const TRmgGridPoint& point) = 0;
    virtual int getTerrain(const TRmgGridPoint& point) = 0;
};

class type_random_map : public TRmgMapInterface {
public:
    b8 m_ownsMapItems;           // +0x04
    char m_paddingBeforeMapItems[3];
    TRmgMapItem* m_mapItems;                // +0x08
    int m_mapWidth;                         // +0x0c
    int m_mapHeight;                        // +0x10
    int m_numberLevels;                     // +0x14

    // Allocates and owns the cells (generator map, treasure-group maps).
    type_random_map(int width, int height, int levels);

    // Single-level view over cells owned elsewhere.
    inline type_random_map(TRmgMapItem* items, int width, int height)
    {
        m_mapItems = items;
        m_mapWidth = width;
        m_mapHeight = height;
        m_numberLevels = 1;
        m_ownsMapItems = false;
    }

    virtual ~type_random_map();

    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile);
    virtual void setFrame(const TRmgGridPoint& point, int value);
#if defined(HOMM3_TARGET_MAC)
    virtual TRmgGridPoint getSize();
#else
    virtual TRmgGridPoint& getSize(TRmgGridPoint& output);
#endif
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point);
    virtual int getTerrain(const TRmgGridPoint& point);
    virtual int getFrame(const TRmgGridPoint& point);

    void clear();
    void addObject(type_object& object, TRmgMapPosition position);
    void markCoastalTiles();
    void floodConnectionCosts(TRmgMapPosition position, b8 waterZone);

    int getWidth() const { return m_mapWidth; }
    int getHeight() const { return m_mapHeight; }
    int getNumberLevels() const { return m_numberLevels; }

    TRmgMapItem* getMapItem(int x, int y);
    inline TRmgMapItem* getMapItem(int x, int y, int z)
    {
        return getMapItem(TRmgMapPosition(x, y, z));
    }
    TRmgMapItem* getMapItem(TRmgMapPosition point);

    // Path-carving helpers.
    void openPathPatch(int x, int y, int level);
    void markBorderPatch(TRmgMapPosition position);
    TPoint traceBranchEnd(TPoint from, TPoint toward, int level);

    b8 hasConnectedOutline(
        const std::vector<TPoint>& outline, TRmgMapPosition position,
        b8 allowEntrances, TRmgZone* zone, b8 requirePathClearance);
    b8 isPlacementBlocked(
        TRmgObjectPropertiesRef* properties, TRmgMapPosition position,
        int zoneIndex, b8 rejectBorder);
    b8 canPlaceObject(
        TRmgObjectPropertiesRef* properties,
        TRmgMapPosition position,
        TRmgZone* zone);

    // Bounds of one map plane; the caller validates the level.
    bool containsXY(const TPoint& point) const
    {
        return point.m_x >= 0 && point.m_x < getWidth()
            && point.m_y >= 0 && point.m_y < getHeight();
    }
};

// A treasure pile with its optional guard, assembled on a private map before
// being placed into a zone.
struct TRmgTreasureGroup {
    type_random_map m_map;                  // +0x00
    TRmgZoneBounds m_bounds;                // +0x18
    std::vector<type_object*> m_objects;    // +0x28
    std::vector<TPoint> m_outline;           // +0x38
    b8 m_hasGuard;               // +0x48, cleared by reset
    char m_padding0049[3];
    // Guard position in group-local coordinates.
    TPoint m_guardPosition;                 // +0x4c
    // Map offset chosen when the group is committed.
    TRmgMapPosition m_position;             // +0x54
    b8 m_ready;                  // +0x60, set after assembly
    char m_padding0061[3];

    TRmgTreasureGroup(int width, int height)
        : m_map(width, height, 1), m_hasGuard(false), m_ready(false)
    {
        reset();
    }
    void reset();
    void markPlacementOutline();
    b8 addGuard(type_object* guard);
    b8 canFitObject(TRmgObjectPropertiesRef* properties, TRmgMapPosition position);
    b8 tryAddObject(type_object* object);
    b8 objectsAllowEntrances() const;
    void addObject(type_object* object, TPoint point);
    void updateBounds();
    void traceOutline();
    // Bounds, outline and outline marks, needed before placing quest and
    // key-guard groups.
    void preparePlacement()
    {
        updateBounds();
        traceOutline();
        markPlacementOutline();
    }
};
SIZE(TRmgTreasureGroup, 0x64);

// Road-layer view of a map for the road painter.
class TRmgRoadMapAdapter : public TRmgRoadMapAdapterInterface {
public:
    type_random_map* m_map;

    TRmgRoadMapAdapter(type_random_map* map) : m_map(map) {}
    virtual void setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile);
    virtual void setLineType(const TRmgGridPoint& point, int value);
    virtual TRmgGridPoint getSize();
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point);
    virtual int getLineType(const TRmgGridPoint& point);
    virtual int getTerrain(const TRmgGridPoint& point);
};

// River-layer view of a map for the river painter.
class TRmgRiverMapAdapter : public TRmgRiverMapAdapterInterface {
public:
    type_random_map* m_map;

    inline TRmgRiverMapAdapter(type_random_map* newMap) : m_map(newMap) {}

    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile);
    virtual void setLineType(const TRmgGridPoint& point, int value);
    virtual TRmgGridPoint getSize();
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point);
    virtual int getLineType(const TRmgGridPoint& point);
    virtual int getTerrain(const TRmgGridPoint& point);
};

// Unreflected line shapes chosen by selectRmgLinePattern; reflections supply
// the other orientations. LINE_END_S also covers an isolated tile.
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

// Frames of one line pattern: first frame index and frame count.
struct TRmgLinePatternRange {
    unsigned int m_firstIndex;
    unsigned int m_valueCount;
};
SIZE(TRmgLinePatternRange, 0x8);

struct TRmgLinePatternTable {
    unsigned int m_patternCount;
    int* m_patterns;
    TRmgLinePatternRange m_ranges[LINE_PATTERN_COUNT];

    TRmgLinePatternTable(unsigned int patternCount, const int* patterns);
    ~TRmgLinePatternTable();
};
SIZE(TRmgLinePatternTable, 0x50);

extern TRmgLinePatternTable g_rmgRiverPatternTable;
extern TRmgLinePatternTable g_rmgRoadPatternTable;

void selectRmgLinePattern(
    const b8* neighbours, const TRmgLinePatternTable* table,
    int& pattern, b8& flipX, b8& flipY);

struct TRmgLinePainterTile;

// Map access used by the road/river line walker.
class TRmgLinePainterInterface {
public:
    TRmgGridPoint m_size;

    TRmgLinePainterInterface(const TRmgGridPoint& size);
    virtual TRmgLinePatternTable* getPattern(int value) = 0;
    virtual void setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile) = 0;
    virtual void setLineType(const TRmgGridPoint& point, int value) = 0;
    // Nonzero prevents painting.
    virtual b32 isBlocked(const TRmgGridPoint& point) = 0;
    virtual void getTile(const TRmgGridPoint& point, rmgTerrainTile& tile) = 0;
    virtual int getLineType(const TRmgGridPoint& point) = 0;

    TRmgLinePainterTile at(const TRmgGridPoint& point);
    int getNeighbourLineType(const TRmgGridPoint& point, unsigned int direction);
};

// Proxy for one tile of a line painter.
struct TRmgLinePainterTile {
    TRmgLinePainterInterface* m_painter;
    TRmgGridPoint m_point;

    TRmgLinePainterTile(TRmgLinePainterInterface* painter, const TRmgGridPoint& point);
    int getLineType();
    void getTile(rmgTerrainTile& tile);
    void setTile(const rmgTerrainTile& tile);
    b8 isBlocked();
    void setLineType(int value);
};
SIZE(TRmgLinePainterTile, 0x0c);

// Grid rectangle as origin and size (unlike the min/max zone bounds).
struct TRmgGridRectangle {
    TRmgGridPoint m_origin;
    TRmgGridPoint m_size;

    TRmgGridRectangle(const TRmgGridPoint& origin, const TRmgGridPoint& size);
};
SIZE(TRmgGridRectangle, 0x10);

// Re-selects a line tile's frame from its neighbours.
void refreshRmgLinePoint(TRmgLinePainterInterface* painter, const TRmgGridPoint& point);
void clearRmgLineRectangle(TRmgLinePainterInterface* painter, const TRmgGridRectangle& rectangle);

class TRmgRiverLinePainter : public TRmgLinePainterInterface {
public:
    TRmgRiverMapAdapterInterface* m_adapter;

    inline TRmgRiverLinePainter(TRmgRiverMapAdapterInterface* newAdapter)
        : TRmgLinePainterInterface(newAdapter->getSize()), m_adapter(newAdapter)
    {
    }
    virtual ~TRmgRiverLinePainter() = 0;

    virtual TRmgLinePatternTable* getPattern(int value);
    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile);
    virtual void setLineType(const TRmgGridPoint& point, int value);
    virtual b32 isBlocked(const TRmgGridPoint& point);
    virtual void getTile(const TRmgGridPoint& point, rmgTerrainTile& tile);
    virtual int getLineType(const TRmgGridPoint& point);
};

inline TRmgRiverLinePainter::~TRmgRiverLinePainter() {}

// One axis of a line walk from the destination back toward the previous
// position: current coordinate, distance and unit step.
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
    int m_lineType;
    TRmgGridPoint m_position;

    TRmgLineWalker(
        TRmgLinePainterInterface* newPainter,
        int newLineType,
        const TRmgGridPoint& start);
    void drawTo(const TRmgGridPoint& destination);
    void paintPoint(const TRmgGridPoint& point);
};

class TRmgRiverPainter : public TRmgRiverLinePainter, public TRmgLineWalker {
public:
    TRmgRiverPainter(
        TRmgRiverMapAdapterInterface* newAdapter,
        int newRiverType,
        const TRmgGridPoint& start);
    virtual ~TRmgRiverPainter();
};

// Road painters parallel the river painter hierarchy.
class TRmgRoadLinePainter : public TRmgLinePainterInterface {
public:
    TRmgRoadMapAdapterInterface* m_adapter;

    inline TRmgRoadLinePainter(TRmgRoadMapAdapterInterface* newAdapter)
        : TRmgLinePainterInterface(newAdapter->getSize()), m_adapter(newAdapter)
    {
    }
    virtual ~TRmgRoadLinePainter() = 0;

    virtual TRmgLinePatternTable* getPattern(int value);
    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile);
    virtual void setLineType(const TRmgGridPoint& point, int value);
    virtual b32 isBlocked(const TRmgGridPoint& point);
    virtual void getTile(const TRmgGridPoint& point, rmgTerrainTile& tile);
    virtual int getLineType(const TRmgGridPoint& point);
};

inline TRmgRoadLinePainter::~TRmgRoadLinePainter() {}

class TRmgRoadPainter : public TRmgRoadLinePainter, public TRmgLineWalker {
public:
    TRmgRoadPainter(
        TRmgRoadMapAdapterInterface* newAdapter,
        int newRoadType,
        const TRmgGridPoint& start);
    virtual ~TRmgRoadPainter();
};

// A generated zone. It borrows its template zone (owned by TRmgTemplate,
// including the connection list) and owns its distance, boundary and
// entrance vectors.
struct TRmgZone {
    TRmgTemplateZone* m_templateZone;  // +0x00
    int m_alignment;                   // +0x04
    // Name from H3API (H3RmgZoneGenerator::townType2). Compared with a
    // creature's town when valuing dwellings and creature rewards.
    int m_townType2;
    // Chosen by chooseTerrain; added water zones use water.
    int m_terrain;                      // +0x0c
    TRmgMapPosition m_levelPosition;   // +0x10
    // Template size scaled to the map (H3API: size); used as the zone's
    // radius and to bound boundary roughness.
    int m_scaledSize;                   // +0x1c
    TRmgZoneBounds m_bounds;           // +0x20
    TRmgMapPosition m_position;        // +0x30: main town entrance
    b8 m_active;            // +0x3c
    char m_opaque003d[3];              // +0x3d..+0x3f
    // Graph distance turned into a randomized quest-zone priority that
    // penalizes immediately adjacent zones.
    int m_questPlacementScore;         // +0x40
    // Placed objects per object type, checked against per-zone limits.
    int m_objectCountByType[232];      // +0x44
    // Connection-graph distance to each original zone (32000 = unreached).
    std::vector<short> m_zoneDistances;// +0x3e4
    std::vector<TPoint> m_boundary;    // +0x3f4: clipped polygon vertices
    std::vector<TPoint> m_entrances;   // +0x404

    TRmgZone(TRmgTemplateZone* slot);
    void decrementObjectCount(TAdventureObjectType objectType);
    void chooseTownType(b8 expanded);
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
    int getSize() const
    {
        return m_templateZone->m_size;
    }
    b8 canConnect(const TRmgZone* other) const;
};

// Quad-edge half-edge of the zone Delaunay/Voronoi subdivision. The twin's
// zone is the region across the edge; next circles the edges leaving the same
// site, whose vertex positions outline that zone.
struct TRmgHalfEdge {
    TPoint m_sitePosition;               // +0x00
    TRmgZone* m_zone;                   // +0x08
    TRmgHalfEdge* m_twin;        // +0x0c
    TRmgHalfEdge* m_next;        // +0x10
    TRmgHalfEdge* m_previous;     // +0x14
    // Set once buildVertices has computed the Voronoi vertex below.
    b8 m_positionComputed;  // +0x18
    TPoint m_position;                  // +0x1c

    // The first form allocates its twin; the second builds that twin.
    TRmgHalfEdge(TPoint sitePosition, TRmgZone* zone,
        TPoint twinSitePosition, TRmgZone* twinZone);
    TRmgHalfEdge(TPoint sitePosition, TRmgZone* zone,
        TRmgHalfEdge* twin);
    void initialize();
    void splice(TRmgHalfEdge* other);
    void detach();
    // Quad-edge navigation corresponds to Graphics Gems IV's
    // Sym/Onext/Oprev/Lnext/Lprev and Org2d/Dest2d operations.
    TRmgHalfEdge* getTwin() const
    {
        return m_twin;
    }
    TRmgHalfEdge* getNext() const
    {
        return m_next;
    }
    TRmgHalfEdge* getPrevious() const
    {
        return m_previous;
    }
    TRmgHalfEdge* getLeftNext() const
    {
        return getTwin()->getPrevious();
    }
    TRmgHalfEdge* getLeftPrevious() const
    {
        return getNext()->getTwin();
    }
    TPoint getSitePosition() const
    {
        return m_sitePosition;
    }
    TPoint getOppositeSitePosition() const
    {
        return getTwin()->getSitePosition();
    }
    TRmgZone* getZone() const
    {
        return m_zone;
    }
    TRmgZone* getOppositeZone() const
    {
        return getTwin()->getZone();
    }
    b8 isPositionComputed() const
    {
        return m_positionComputed;
    }
    void setPosition(const TPoint& position)
    {
        TPoint copy = position;
        m_position = copy;
        m_positionComputed = true;
    }
};
SIZE(TRmgHalfEdge, 0x24);

// Delaunay subdivision of the zone sites; owns every allocated edge. Zone
// placement inserts the sites, computes the dual Voronoi vertices, then looks
// up an edge for each site.
class TRmgVoronoi {
public:
    TRmgHalfEdge* m_root;                 // +0x00
    std::vector<TRmgHalfEdge*> m_edges;  // +0x04

    TRmgVoronoi();
    ~TRmgVoronoi();
    TRmgHalfEdge* createEdge(TPoint first, TRmgZone* firstZone,
        TPoint second, TRmgZone* secondZone);
    TRmgHalfEdge* connectEdges(TRmgHalfEdge* first,
        TRmgHalfEdge* second);
    void removeEdge(TRmgHalfEdge* edge);
    void addSite(TPoint point, TRmgZone* zone);
    TRmgHalfEdge* locate(TPoint point);
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

// Initial segment direction of a carved branching path.
enum ERmgBranchSeedPattern {
    RMG_BRANCH_SEED_MAIN_DIAGONAL = 0,
    RMG_BRANCH_SEED_VERTICAL = 1,
    RMG_BRANCH_SEED_ANTI_DIAGONAL = 2,
    RMG_BRANCH_SEED_HORIZONTAL = 3
};

// Per-object-type placement limit.
struct TRmgObjectLimit {
    int m_objectType;
    int m_limit;
};

// Map, object prototypes and placement rules shared by the generator.
class TRmgGeneratorBase {
public:
    // Seeded from time().
    long m_randomSeed;                                 // +0x004
    int m_mapVersion;                                  // +0x008
    type_random_map m_map;                             // +0x00c
    TObjectTypeTable m_objectsTxt;                     // +0x024
    std::vector<TRmgObjectPropertiesRef*> m_objectPrototypes[232]; // +0x034
    // rand_trn.txt placement rules.
    std::vector<TRmgObjectPlacementRule> m_placementRules; // +0xeb4
    std::vector<type_object*> m_objects;               // +0xec4
    TProgressSink* m_progress;                          // +0xed4
    TRmgGeneratorBase(int width, int height, int levels,
        TProgressSink* progress, int additionalSteps, int version);
    virtual ~TRmgGeneratorBase();
    virtual void addObject(type_object* object, TRmgMapPosition position);
    // Loads objects.txt, builds the per-type prototype lists, then the
    // placement rules.
    void loadObjectPrototypes();
    void readObjectPlacementRules();
    int scoreObjectPlacement(
        TRmgObjectPropertiesRef* properties, TRmgMapPosition position);
    void decorateMap();
    void decorateMapCell(TRmgMapPosition position, int progressSteps);
};
SIZE(TRmgGeneratorBase, 0xed8);

// Town count/density groups by ownership and town option.
enum ERmgTownPlacementCategory {
    RMG_TOWN_PLAYER_OPTION,
    RMG_TOWN_PLAYER_BASIC,
    RMG_TOWN_NEUTRAL_OPTION,
    RMG_TOWN_NEUTRAL_BASIC
};

class type_random_map_generator : public TRmgGeneratorBase {
public:
    b8 m_fixedHumanPlayers[8];              // +0x0ed8
    // Entry zero is the unmapped sentinel.
    int m_playerIndexMap[9];                          // +0x0ee0
    char m_opaque0f04[0x20];                          // +0x0f04
    int m_townChoices[8];                              // +0x0f24
    // Counter for generated object ids; starts at 1.
    int m_nextObjectId;                               // +0x0f44
    int m_humanPlayerCount;                            // +0x0f48
    int m_humanTeamCount;                              // +0x0f4c
    int m_computerPlayerCount;                         // +0x0f50
    int m_computerTeamCount;                           // +0x0f54
    // Cycles through the seer-hut prototypes.
    int m_nextSeerHutPrototypeIndex;                   // +0x0f58
    // Next free keymaster tent colour. Retail bug: never initialized, so
    // its first value is whatever was on the caller's stack.
    int m_nextKeyTentColor;                            // +0x0f5c
    // Active zones, in total and per town alignment.
    int m_activeZoneCount;                             // +0x0f60
    int m_activeZoneCountsByAlignment[9];              // +0x0f64
    b8 m_disabledHeroes[156];               // +0x0f88
    // Artifacts already used as seer-hut quests.
    b8 m_usedQuestArtifacts[144];           // +0x1024
    // Set once fewer than 20 quest artifacts remain; seer huts are then
    // rejected.
    b8 m_questArtifactPoolLow;              // +0x10b4
    int m_waterContent;                                // +0x10b8: ERmgWaterContent
    int m_monsterStrength;                             // +0x10bc
    // Name of the selected template.
    std::string m_templateName;                        // +0x10c0
    // Owned templates loaded from rmg.txt.
    std::vector<TRmgTemplate*> m_templates;            // +0x10d0
    std::vector<TRmgZone*> m_zones;                    // +0x10e0
    std::vector<type_treasure_def*> m_objectGenerators; // +0x10f0
    std::vector<b8> m_disabledKeyTents;     // +0x1100
    int m_objectCountByType[232];                      // +0x1110
    std::vector<TRmgMapPosition> m_roadTargets;        // +0x14b0
    std::vector<type_object*> m_monolithsOneWay;       // +0x14c0
    std::vector<type_object*> m_monolithsTwoWay;       // +0x14d0

    type_random_map_generator(int width, int height, int levels,
        int humanPlayers, int humanTeams, int computerPlayers, int computerTeams,
        int waterContent, int monsterStrength, TProgressSink* progress, int version);
    void loadTemplates();
    void placeMines();
    void prepareZoneConnections();
    void markZoneBorders();
    void placeWaterZoneIslands(TRmgZone* zone);
    void createWaterZoneIsland(const TRmgZoneBounds& bounds, int level);
    void floodWaterZoneDistances(TRmgMapPosition position, int zoneIndex);
    void buildZoneConnectionPaths();
    void placeExtraMines(TRmgZone* zone);
    b8 placeMineSite(type_object* object, TRmgZone* zone,
        b8 startingMine, int spacing);
    b8 tryPlaceMine(TRmgZone* zone, int resource,
        b8 startingMine, int spacing);
    void placePrimaryTown(TRmgZone* zone);
    b8 tryPlacePrimaryTown(TRmgZone* zone, int alignment,
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
    void fillIslandInterior(TRmgZone* zone);
    void drawIslandBoundary(TPoint from, TPoint to, int zoneIndex, int level, int roughness);
    void placeAdditionalTowns(TRmgZone* zone);
    b8 tryPlaceAdditionalTown(TRmgZone* zone, int alignment,
        int player, unsigned char townOption, int spacing);
    void prepareJunctionZone(TRmgZone* zone);
    void connectJunctionEntrance(TPoint from, TPoint to, TRmgZone* zone);
    void placeZoneTreasures(TRmgZone* zone);
    // Compact selection keeps only treasures near the best value per
    // footprint cell; zones retry a failed treasure band with it.
    type_object* createTreasureObject(TRmgZone* zone, int minimum, int maximum,
        int* value, b8 primary, b8 allowTerrainDependent,
        b8 compact, TRmgMapPosition position);
    int fillTreasureGroup(TRmgZone* zone, TRmgTreasureGroup* group,
        b8 compact, int value);
    b8 assembleTreasureGroup(TRmgZone* zone, TRmgTreasureGroup* group,
        b8 compact, int minimum, int maximum);
    b8 placeTreasureGroup(TRmgTreasureGroup* group, TRmgZone* zone, int spacing);
    b8 canPlaceTreasureGroup(TRmgTreasureGroup* group,
        TRmgMapPosition position, TRmgZone* zone);
    void commitTreasureGroup(TRmgTreasureGroup* group, TRmgMapPosition position);
    void decorateUnderground();
    b8 generate();
    b8 writeMap(TAbstractFile* outputFile);
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
    b8 canPlaceZone(TRmgZone* zone);
    void buildZoneBoundaries(TRmgTemplate* mapTemplate, int level);
    // Propagates distances from one zone through the connection graph.
    void propagateZoneDistances(TRmgZone* zone);
    void fillZoneArea(TRmgZone* zone, TRmgHalfEdge* first);
    void joinExtraZones(int originalZones, TRmgVoronoi* diagram);
    int countPlacedZoneConnections(TRmgZone* zone) const;
    void filterZonePositions(
        TRmgZone* zone, std::vector<TRmgMapPosition>& candidates, int mapSize);
    void drawIrregularZoneBoundary(
        TPoint from, TPoint to, int zoneIndex, int level, int roughness);
    void drawStraightZoneBoundary(
        TPoint from, TPoint to, int zoneIndex, int level);
    void traceZoneBoundary(TRmgHalfEdge* first, b8 irregular);
    b8 createGroundConnection(
        TRmgZone* source,
        TRmgZoneConnection* connection,
        std::vector<TRmgMapItem*>* borderItems,
        std::vector<TRmgMapPosition>* borderPositions);
    void floodConnectionRegion(TRmgMapPosition position);
    void floodShipyardWater(type_object* shipyard);
    // Places a shipyard beside reachable water.
    b8 createShipyardConnection(
        TRmgZone* source, TRmgZoneConnection* connection);
    b8 canPlaceShipyard(TRmgMapPosition position);
    b8 createSubterraneanGate(
        TRmgZone* source, TRmgZoneConnection* connection);
    b8 placeMonolithBorder(TRmgMapPosition position, TRmgZone* keyTentZone);
    void createMonolithConnection(
        TRmgZone* source,
        TRmgZoneConnection* connection,
        int prototypeIndex);
    void connectZones();
    bool contains(const TPoint& point) const;
    // Random midpoint displacement, queued side branches, then terrain and
    // border cleanup.
    void carveBranchingPaths();
    void repairWaterZoneBorders();
    void openConnectionPath(TRmgMapPosition position, b8 narrow);
    void markBorderObjectArea(TRmgMapPosition position, int color);
    int placeBorderObject(
        TRmgMapPosition position, int guardCount, TRmgZone* keyTentZone);
    type_object* createGuard(int value, TRmgZone* zone);
    b8 placeObjectInZone(type_object* object, TRmgZone* zone);
    void placeGuard(int value, TRmgMapPosition position);
    int getMineGuardValue(int resource, const TRmgZone* zone) const;
    TRmgObjectPropertiesRef* selectObjectPrototype(
        int terrain, int objectType, int subtype);
    void resetMovementCosts();
    void buildRoadCostMap(TRmgMapPosition position);
    void createRoads();
    // Picks an unused quest artifact and places its seer hut in another zone,
    // handing the hut to the map; if the hut cannot be placed, the artifact
    // is replaced by a treasure.
    b8 placeQuestArtifact(rmgQuestArtifactObject* object);
    void calculateQuestZoneDistances(TRmgZone* origin);
    b8 placeQuestGroup(TRmgTreasureGroup* group, TRmgZone* origin);
    // Places a treasure group guarded by a same-colour border guard in another
    // zone; on success that colour stays disabled.
    b8 placeKeyTentGuard(type_object* object, int maxValue);
    // Change one colour's availability, then rescan for the first enabled
    // colour (size() when none remain).
    void setKeyTentColorDisabled(int color, b8 disabled)
    {
        m_disabledKeyTents[color] = disabled;
        m_nextKeyTentColor = 0;
        while (m_nextKeyTentColor < m_disabledKeyTents.size()
            && m_disabledKeyTents[m_nextKeyTentColor])
            ++m_nextKeyTentColor;
    }
    void setHumanPlayer(int seat);
    void setTownChoice(int seat, int town);
    void removeObject(type_object* object);
    // Paints a road back along the path-cost predecessors.
    b8 paintRoad(TRmgMapPosition position, int roadType);
    // Runs a river from beside a water wheel to a river target, possibly
    // ending in a river delta.
    void createRiver(TRmgMapPosition source);
    void markRiverObjectTargets();
    void markRiverTargets();
    void markRiverCoastTarget(TRmgMapPosition position, int direction);
    void createRiverToObject(TRmgMapPosition source);
    void createRivers();
    void writeMapHeader(TAbstractFile* outputFile);
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
SIZE(TRmgRiverMapAdapterInterface, 0x04);
SIZE(TRmgRoadMapAdapterInterface, 0x04);
SIZE(TRmgRiverMapAdapter, 0x08);
SIZE(TRmgLinePainterInterface, 0x0c);
SIZE(TRmgRiverLinePainter, 0x10);
SIZE(TRmgRoadLinePainter, 0x10);
SIZE(TRmgLineWalker, 0x10);
SIZE(TRmgRiverPainter, 0x20);
SIZE(TRmgRoadPainter, 0x20);
SIZE(type_random_map_generator, 0x14e0);

// Creature value by level, used to size creature rewards.
DATA(0x006824E0) extern int g_rmgCreatureValueByLevel[];

#endif  // HOMM3_RMG_H
