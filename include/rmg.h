// Complete-only random-map generator declarations.
#ifndef HOMM3_RMG_H
#define HOMM3_RMG_H

#include "va.h"

#include <bitset>
#include <string>
#include <vector>

#include "advmgr_objects.h"
#include "terrain_type.h"
#include "progress_bar.h"
#include "rmg_request.h"
#include "Point.h"
#include "terrainplacement.h"


// Terrain types, dirt through rock.
enum ERmgTerrainLimits {
    RMG_TERRAIN_COUNT = eTerrainRock + 1
};

// Hero ids a map format knows: RoE maps stop before the expansion heroes.
// Prisons in later formats hold only the first RMG_PRISON_HERO_COUNT.
enum ERmgHeroCount {
    RMG_ROE_HERO_COUNT = 128,
    RMG_PRISON_HERO_COUNT = 145,
    RMG_HERO_COUNT = 156
};

enum ERmgTreasurePlacementLimits {
    RMG_TREASURE_ATTEMPTS = 3,
    RMG_TREASURE_MINIMUM_REMAINDER = 1500,
    RMG_TREASURE_MINIMUM_VALUE = 100
};

// m_townPlacement slots, read from rmg.txt's zone town fields: player-owned
// and neutral town counts and densities. CASTLE towns start with a fort;
// BASIC towns start without one.
enum ERmgTownPlacementParameter {
    RMG_TOWN_PLAYER_BASIC_COUNT = 0,
    RMG_TOWN_PLAYER_CASTLE_COUNT = 1,
    RMG_TOWN_PLAYER_BASIC_DENSITY = 2,
    RMG_TOWN_PLAYER_CASTLE_DENSITY = 3,
    RMG_TOWN_NEUTRAL_BASIC_COUNT = 4,
    RMG_TOWN_NEUTRAL_CASTLE_COUNT = 5,
    RMG_TOWN_NEUTRAL_BASIC_DENSITY = 6,
    RMG_TOWN_NEUTRAL_CASTLE_DENSITY = 7,
    RMG_TOWN_PLACEMENT_PARAMETER_COUNT = 8
};

// Zone monster strength, from the template letter n, w, a or s.
enum ERmgZoneMonsterStrength {
    RMG_ZONE_MONSTERS_NONE = 0,
    RMG_ZONE_MONSTERS_WEAK = 2,
    RMG_ZONE_MONSTERS_AVERAGE = 3,
    RMG_ZONE_MONSTERS_STRONG = 4
};

// Treasure bands per template zone in rmg.txt (low, medium and high value).
enum ERmgTreasureBandLimits {
    RMG_TREASURE_BAND_COUNT = 3
};

// Town count/density groups by ownership and starting fort.
enum ERmgTownPlacementCategory {
    RMG_TOWN_PLAYER_CASTLE,
    RMG_TOWN_PLAYER_BASIC,
    RMG_TOWN_NEUTRAL_CASTLE,
    RMG_TOWN_NEUTRAL_BASIC,
    RMG_TOWN_CATEGORY_COUNT
};

// The eight neighbour directions, clockwise.
enum ERmgDirectionLimits {
    RMG_DIRECTION_COUNT = 8
};

enum ERmgShipyardConstants {
    RMG_SHIPYARD_WATER_OFFSET_COUNT = 4
};

// Creature-type counts and guard limits. RoE maps lack the expansion creature
// types; their guards exclude creatures from 118 but evaluate only those below
// 117, so 117 slips through (retail bug).
enum ERmgGuardConstants {
    RMG_CREATURE_TYPE_COUNT = 145,
    RMG_ROE_CREATURE_TYPE_COUNT = 118,
    RMG_GUARD_MAXIMUM_COUNT = 100,
    RMG_GUARD_DISPOSITION = 3
};

// Path costs and zone graph distances start at RMG_UNREACHED_COST. Searches
// treat costs above RMG_REACHED_COST_LIMIT as unreached; some also reject the
// limit itself. Clearing a cell sets its costs and object distance to
// RMG_CLEARED_CELL_COST; removing an object resets the object distance under
// its footprint to it.
enum ERmgPathCostLimits {
    RMG_REACHED_COST_LIMIT = 30000,
    RMG_UNREACHED_COST = 32000,
    RMG_CLEARED_CELL_COST = 32700
};

// Cell bits of scoreObjectPlacement's marks grid: ADJACENT in the 3x3 around
// a blocked footprint cell, OVERLAP on a drawn footprint cell, BLOCKED on a
// blocked one (overwritten there; see its retail bug).
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

// Initial segment direction of a carved branching path. Each spans the map;
// North is up and # is the segment.
//   MAIN_DIAGONAL  VERTICAL  ANTI_DIAGONAL  HORIZONTAL
//   # . .          . # .     . . #          . . .
//   . # .          . # .     . # .          # # #
//   . . #          . # .     # . .          . . .
enum ERmgBranchSeedPattern {
    RMG_BRANCH_SEED_MAIN_DIAGONAL = 0,
    RMG_BRANCH_SEED_VERTICAL = 1,
    RMG_BRANCH_SEED_ANTI_DIAGONAL = 2,
    RMG_BRANCH_SEED_HORIZONTAL = 3,
    RMG_BRANCH_SEED_PATTERN_COUNT = 4
};


// Original class spellings: the GOG Complete editor h3maped.exe (sha256
// 4480fba1...) compiles the same RMG with /GR. Its RTTI type descriptors and
// vtables appear in the game's vtable order (editor 0x540a04..0x540cbc, game
// 0x6409c0..0x640c44); the editor omits the three abstract map-interface
// tables. Class hierarchy descriptors confirm each base. Previous spellings:
//   type_progress_bar              was TProgressSink (editor vtable 0x540a04)
//   type_random_map                was TRmgMap (editor vtable 0x540a14)
//   type_road_map                  was TRmgRoadMapAdapter (editor vtable 0x540a34)
//   type_river_map                 was TRmgRiverMapAdapter (editor vtable 0x540a54)
//   type_object                    was TRmgObject (editor vtable 0x540a74)
//   type_monster                   was TRmgMonsterObject (editor vtable 0x540a88)
//   type_town                      was TRmgTownObject (editor vtable 0x540a9c)
//   type_flaggable                 was TRmgOwnableObject (editor vtable 0x540ab0)
//   type_random_artifact           was TRmgArtifactObject (editor vtable 0x540ac4)
//   type_resource_lump             was TRmgResourceObject (editor vtable 0x540ad8)
//   type_black_box                 was TRmgBlackBoxObject (editor vtable 0x540aec)
//   type_key_tent                  was TRmgKeyTentObject (editor vtable 0x540b00)
//   t_quest_artifact               was TRmgQuestArtifactObject (editor vtable 0x540b14)
//   type_seer_hut                  was TRmgSeerHutObject (editor vtable 0x540b28)
//   type_hero                      was TRmgHeroObject (editor vtable 0x540b3c)
//   type_scholar                   was TRmgScholarObject (editor vtable 0x540b50)
//   type_shrine                    was TRmgShrineObject (editor vtable 0x540b64)
//   type_spell_scroll              was TRmgSpellScrollObject (editor vtable 0x540b78)
//   type_witch_hut                 was TRmgWitchHutObject (editor vtable 0x540b8c)
//   type_treasure_def              was TRmgTreasureDef (editor vtable 0x540ba0)
//   type_artifact_def              was TRmgArtifactDef (editor vtable 0x540bb0)
//   type_black_box_creature_def    was TRmgBlackBoxCreatureDef (editor vtable 0x540bc0)
//   type_black_box_experience_def  was TRmgBlackBoxExperienceDef (editor vtable 0x540bd0)
//   type_black_box_gold_def        was TRmgBlackBoxGoldDef (editor vtable 0x540be0)
//   type_black_box_spells_def      was TRmgBlackBoxSpellsDef (editor vtable 0x540bf0)
//   type_map_dwelling_def          was TRmgMapDwellingDef (editor vtable 0x540c00)
//   type_resource_lump_def         was TRmgResourceLumpDef (editor vtable 0x540c10)
//   type_prison_def                was TRmgPrisonDef (editor vtable 0x540c20)
//   type_scholar_def               was TRmgScholarDef (editor vtable 0x540c30)
//   type_shrine_def                was TRmgShrineDef (editor vtable 0x540c40)
//   type_witch_hut_def             was TRmgWitchHutDef (editor vtable 0x540c50)
//   type_quest_creature_def        was TRmgQuestCreatureDef (editor vtable 0x540c60)
//   type_quest_experience_def      was TRmgQuestExperienceDef (editor vtable 0x540c70)
//   type_quest_gold_def            was TRmgQuestGoldDef (editor vtable 0x540c80)
//   type_spell_scroll_def          was TRmgSpellScrollDef (editor vtable 0x540c90)
//   type_key_tent_def              was TRmgKeyTentDef (editor vtable 0x540ca0)
//   t_abstract_random_generator    was TRmgGeneratorBase (editor vtable 0x540cb0)
//   type_random_map_generator      was TRmgGenerator (editor vtable 0x540cbc)
//   type_flaggable_def             was TRmgDwellingDef (base of type_map_dwelling_def)
// The map editor's own objects name the placement operations the generator
// shares with it (RTTI vtables and class hierarchy descriptors):
//   TTerrainPlacementOp            was TRmgTerrainBrush (terrainplacement.h)
//   TTerrainPlacementOp::TAbstractMap was TRmgMapInterface (editor vtable 0x53974c)
//   TMapLineFilter                 was TRmgLinePainterInterface (base of both line ops)
//   TRiverOp                       was TRmgRiverLinePainter (editor vtable 0x54183c)
//   TRiverOp::TAbstractMap         was TRmgRiverMapAdapterInterface (editor vtable 0x53952c)
//   TRiverPlacementOp              was TRmgRiverPainter (editor vtable 0x54185c)
//   TRoadOp                        was TRmgRoadLinePainter (editor vtable 0x5419d4)
//   TRoadOp::TAbstractMap          was TRmgRoadMapAdapterInterface (editor vtable 0x53956c)
//   TRoadPlacementOp               was TRmgRoadPainter (editor vtable 0x5419f4)
class TAbstractFile;
class TSpreadsheetResource;
class type_random_map_generator;
struct TRmgTemplateZone;
struct TRmgZone;
struct TRmgTerrainTile;
struct TRmgPoint;
struct TObjectType;
struct TRmgObjectPropertiesRef;
class type_object;

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
#if defined(HOMM3_RMG_HOTFIX)
    // Factories are deleted through this base.
    virtual ~type_treasure_def() {}
#endif

    // Shared caller 0x5464cd..0x5464de passes the selected property reference,
    // generator and zone, then consumes the result as a type_object pointer.
    // This replaces the earlier placeholder void*/int/int factory signature.
    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
    virtual int getValue(TRmgZone* zone, type_random_map_generator* generator);
    virtual unsigned char requiresLinkedPlacement();
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
    int m_creatureCount;

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
    virtual unsigned char requiresLinkedPlacement();
};

// Retail writes both 0x640bac and 0x640bb8 after the retained base call.
// That is direct evidence for this two-level dwelling hierarchy. The
// Complete editor's hierarchy descriptor names the intermediate class:
// type_map_dwelling_def <- type_flaggable_def <- type_treasure_def.
class type_flaggable_def : public type_treasure_def {
public:
    inline type_flaggable_def(int subtype)
        : type_treasure_def(17, subtype, -1, 40)
    {
    }

    virtual type_object* generate(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, TRmgZone* zone);
};

class type_map_dwelling_def : public type_flaggable_def {
public:
    inline type_map_dwelling_def(int subtype)
        : type_flaggable_def(subtype)
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
    virtual unsigned char requiresLinkedPlacement();
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
    virtual unsigned char requiresLinkedPlacement();
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
    virtual unsigned char requiresLinkedPlacement();
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
SIZE(type_flaggable_def, 0x14);
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
    // The analogous Graphics Gems vector dot is a tiny header inline. Retail
    // likewise expands both calls in buildVertices and retains no separate
    // body; the by-value operand also recovers that caller's register homes.
    int dot(TRmgVector other) const
    {
        return m_y * other.m_y + m_x * other.m_x;
    }
};

// The random map generator's signed geometry point. Retail's RMG direction
// table (cinit at 0x530da0) and the Voronoi code construct it by value,
// which separates it from the TPoint<int> of the shared placement code
// (Point.h), whose retained TPoint<unsigned int> constructor (0x5b76b0)
// takes its coordinates by reference. The name is provisional. The absence
// of an atexit registration proves that destruction is trivial. The
// comparator is independently used by the RMG set cluster.
struct TRmgPoint {
    int m_x;
    int m_y;

    TRmgPoint() {}
    TRmgPoint(int newX, int newY) : m_x(newX), m_y(newY) {}

    TRmgPoint& operator+=(const TRmgVector& offset)
    {
        m_x += offset.m_x;
        m_y += offset.m_y;
        return *this;
    }
    bool operator==(const TRmgPoint& other) const
    {
        return m_x == other.m_x && m_y == other.m_y;
    }
    bool operator!=(const TRmgPoint& other) const
    {
        return !(*this == other);
    }

    bool operator<(const TRmgPoint& other) const
    {
        return m_y < other.m_y || (m_y == other.m_y && m_x < other.m_x);
    }
    int getX() const { return m_x; }
    int getY() const { return m_y; }
};

// The native distance callers pass the position's XY subobject as a TRmgPoint
// value, without constructing another point. This shared base reproduces
// that conversion; the retained Windows constructor keeps offsets 0/4/8
// and the same twelve-byte position layout.
struct TRmgMapPosition : TRmgPoint {
    int m_z;

    TRmgMapPosition() {}
    TRmgMapPosition(int newX, int newY, int newZ);

    TRmgMapPosition& operator+=(const TRmgPoint& offset);
    TRmgMapPosition& operator-=(const TRmgPoint& offset);
};

// Native callers copy both operands before constructing the translated value.
TRmgMapPosition operator+(TRmgMapPosition position, TRmgPoint offset);

// Complete's zone-connection records are walked at a 0x1c-byte stride by
// the connection pass.  The first pointer identifies the opposite template
// zone; the three adjacent bytes select guard policy and record completion.
struct TRmgZoneConnection {
    TRmgTemplateZone* m_destination;             // +0x00
    int m_value;                             // +0x04
    unsigned char m_unguarded;               // +0x08
    unsigned char m_placeBorderObjects;      // +0x09
    unsigned char m_connected;               // +0x0a
    unsigned char isConnected() const;
    void setConnected();
    // Replaces synthetic opaque000b: +0x0b aligns four int limits.
    // Retail connection reader 0x5382c9..0x538304 parses spreadsheet
    // columns 81..84 into +0x0c/+0x10/+0x14/+0x18. At 0x538307..0x53832b
    // it compares the first pair with humanPlayerCount and the second
    // pair with humanPlayerCount + computerPlayerCount before insertion.
    // Role-derived names, consistent with the TRmgTemplateZone limits below.
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



struct TRmgTreasureRange {
    int m_minimum;
    int m_maximum;
    int m_density;

    bool isActive() const
    {
        return m_maximum >= RMG_TREASURE_MINIMUM_VALUE && m_density > 0;
    }
};

// ReadRmgTemplateZones allocates 0xd4 bytes and constructs connections at
// +0xc4. ConnectZones reads its _First at +0xc8 and _Last at +0xcc;
// those pointer offsets must not be mistaken for the vector's own offset.
// Unresolved scalar groups retain offset-based names until their consumers
// establish their roles. Other field names are provisional retail roles.
struct TRmgTemplateZone {
    int m_zoneIndex;                    // +0x00
    int m_kind;                         // +0x04: ERmgTemplateZoneKind
    int m_size;                         // +0x08
    int m_minimumHumanPlayers;          // +0x0c
    int m_maximumHumanPlayers;          // +0x10
    int m_minimumPlayers;               // +0x14
    int m_maximumPlayers;               // +0x18
    int m_playerIndex;                  // +0x1c
    int m_townPlacement[8];
    // Template column 22: preserve zone alignment for neutral towns.
    unsigned char m_neutralTownsMatchZone; // +0x40, provisional retail role
    unsigned char m_allowedTowns[9];    // +0x41
    int m_mineCounts[7];
    int m_mineDensities[7];
    // template byte to prefer the aligned town's native terrain table.
    // Complete-only provisional role name.
    unsigned char m_useNativeTerrain;
    unsigned char m_allowedTerrain[8];  // +0x85
    int m_monsterStrength;              // +0x90
    // Template column 56: restrict guards to the zone's town alignment.
    unsigned char m_guardsMatchZone;    // +0x94, provisional retail role
    unsigned char m_allowedMonsters[10]; // +0x95
    TRmgTreasureRange m_treasure[3];     // +0xa0
    std::vector<TRmgZoneConnection> m_connections; // +0xc4

#if defined(HOMM3_RMG_HOTFIX)
    void getTownCategories(s32* densities, s32* counts) const;
#endif
    int selectAllowedTown();
#if defined(HOMM3_RMG_HOTFIX)
    bool isUsable() const;
#endif
    TRmgZoneConnection* findConnection(int destinationZone);
};
SIZE(TRmgTemplateZone, 0xd4);

// The rmg.txt coordinator allocates this 0x38-byte object, assigns its
// name and size limits, and passes it to the zone reader in edx.
struct TRmgTemplate {
    std::string m_name;                  // +0x00
    std::vector<TRmgTemplateZone*> m_zones;   // +0x10
    char m_opaque0020[0x10];
    int m_minimumSize;                  // +0x30
    int m_maximumSize;                  // +0x34

    ~TRmgTemplate();
    TRmgTemplateZone* findZone(int zoneIndex);
#if defined(HOMM3_RMG_HOTFIX)
    void getPlayerSlots(b8* humanSlots, b8* allSlots) const;
#endif
    bool hasPlayerSlots(s32 humanPlayers, s32 computerPlayers) const;
#if defined(HOMM3_RMG_HOTFIX)
    bool isUsable(s32 humanPlayers, s32 computerPlayers) const;
#endif
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


// Voronoi's circumcenter arithmetic separates displacement vectors from
// positions: vector+vector is a member call, point+vector and point-point
// are free calls. All carry two signed dwords; names remain provisional.
// Retail rmg.obj expands all five where its callers use them, while their
// retained bodies sit in the Voronoi object after buildVertices: header
// inline definitions, emitted by the object that still calls them.
VA(0x005FDCB0, 0x1E) // caller 0x5fdc49; thiscall, hidden result + eight-byte operand
inline TRmgVector TRmgVector::operator+(TRmgVector other) const
{
    return TRmgVector(m_x + other.m_x, m_y + other.m_y);
}

VA(0x005FDCD0, 0x1D) // caller 0x5fdc2f; thiscall, ret 8
inline TRmgVector TRmgVector::operator*(int scale) const
{
    TRmgVector result;
    result.m_x = m_x * scale;
    result.m_y = m_y * scale;
    return result;
}

VA(0x005FDCF0, 0x25) // callers 0x5fdc36/0x5fdc50; signed division, ret 8
inline TRmgVector TRmgVector::operator/(int divisor) const
{
    return TRmgVector(m_x / divisor, m_y / divisor);
}

VA(0x005FDD20, 0x20)
inline TRmgPoint operator+(TRmgPoint point, TRmgVector offset)
{
    return TRmgPoint(point.m_x + offset.m_x, point.m_y + offset.m_y);
}

// The field-built result preserves all 32 retained bytes and improves the
// carveBranchingPaths expansion without adding a second arithmetic helper.
VA(0x005FDD40, 0x20)
inline TRmgVector operator-(TRmgPoint left, TRmgPoint right)
{
    return TRmgVector(left.m_x - right.m_x, left.m_y - right.m_y);
}

// Complete-only Voronoi addSite 0x5fd790 passes whole site positions to
// retained integer predicates. Aggregate-by-value arguments occupy the
// stack under /Gr: orientation 0x5fdae0 returns with ret 0x18, distance
// 0x5fdb10 with ret 0x10. Names describe the proven geometry operations.
int getRmgPointOrientation(TRmgPoint first, TRmgPoint second, TRmgPoint third);
// carveBranchingPaths expands the distance; addSite keeps its call.
VA(0x005FDB10, 0x21)
MAC_ADDRESS(0x25c3b4, 0x38) // anchor-callee addSite; Complete-only, ret 0x10; MAC_ABSTRACTION_FROM(tokens1:311342e68a5c,100.0000): aa93e92de's retail object split makes the squared distance a header inline body that carveBranchingPaths expands; CodeWarrior emits no out-of-line copy for addSite's call.
inline int getRmgSquaredDistance(TRmgPoint first, TRmgPoint second)
{
    int dy = first.m_y - second.m_y;
    int dx = first.m_x - second.m_x;
    return dx * dx + dy * dy;
}

struct TRmgZoneBounds {
    int m_minimumX;
    int m_minimumY;
    int m_maximumX;
    int m_maximumY;

    bool contains(const TRmgPoint& point) const
    {
        return point.m_x >= m_minimumX && point.m_x < m_maximumX &&
            point.m_y >= m_minimumY && point.m_y < m_maximumY;
    }
};

TRmgPoint clipRmgBoundaryPoint(
    const TRmgZoneBounds& bounds, TRmgPoint point, TRmgPoint toward);

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



// Complete's guard selector 0x540b20 uses these bounds, not the full combat
// creature array. Its RoE exclusion starts at 118 even though evaluation
// stops before 117; keep that observed boundary distinct.
enum ERmgGuardSelectionBounds {
    RMG_GUARD_ROE_CREATURE_LIMIT = 117,

    };

// CreateRiver's function-local river-delta table. Retail registers the
// bare-`ret` callback 0x549790 with _atexit when it first reaches the table.
// VC6 registers exactly that empty callback for a function-local static
// array of a class with a constructor and no destructor; a user-declared
// empty destructor instead registers a callback that runs the eh vector
// destructor iterator over the four elements.
struct TRmgRiverDeltaOffset {
    int m_x;
    int m_y;

    TRmgRiverDeltaOffset(int newX, int newY) : m_x(newX), m_y(newY) {}
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
    unsigned m_objectDistance : 16;
    signed m_zone : 8;
    signed m_connectionZone : 8;
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
    // Bit k bars a river from entering through side k. North is up:
    //     3
    //   2 . 0
    //     1
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
    unsigned m_objectEntrance : 1;
    // 0x535ee0 traces a closed placement perimeter into the point vector
    // at +0x38 (append 0x535fb2, closure 0x536051..0x536062). Accepted
    // placement 0x5468e8 calls it, then marks these points with bit 23
    // at 0x546923; 0x54b6df and 0x54bad5 mark the same outline in the
    // quest placement paths. Checker 0x546ed5 requires the marked bit.
    unsigned m_placementOutline : 1;
    unsigned m_connectionVisited : 1;
    unsigned m_passable : 1;
    unsigned m_obstacleFill : 1;
    unsigned m_pathClearance : 1;
    unsigned m_paintZoneTerrain : 1;
    // River adapters set bit 29 for a nonzero river kind (0x532769..0x532780).
    // Mountains, lakes and gem mines also mark joins before a river is painted.
    // Coastal and map-edge outlets use the separate flag at bit 30.
    unsigned m_riverJoinTarget : 1;
    unsigned m_riverOutletTarget : 1;
    unsigned m_nearRiver : 1;
};

struct TRmgBorderConnection {
    unsigned m_present : 1;
    // Key colour for a guarded connection; its interpretation needs m_present.
    unsigned m_guardColor : 4;
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
    std::vector<TRmgPoint> m_outline;         // +0x14
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
    // Footprint mask cell (column, row)
    // lies at position - (column, row): the mask grows west and north from the
    // object's bottom-right cell P. North is up.
    //   (2,1) (1,1) (0,1)
    //   (2,0) (1,0) (0,0)=P
    TRmgMapPosition getPosition() const;
    TRmgMapPosition getPlacedGroupPosition(const TRmgMapPosition& groupPosition) const;
#if defined(HOMM3_RMG_HOTFIX)
    TRmgMapPosition getEntrance() const;
#endif

    void clearPlacementMarks();

    unsigned char isPlacementTouched() const
    {
        return m_adjacentToCandidate || m_blockedByCandidate || m_overlapsCandidate;
    }

    virtual ~type_object();
    virtual void releaseReservation();
    // The quest-artifact override at 0x533a50 clears owned state, and its
    // generator callee replaces this object's property reference. These
    // mutable operations reject the earlier const receiver placeholder.
    virtual unsigned char completePlacement();
    virtual void write(TAbstractFile* outfile, int version);
};

// Provisional Complete-only role: createGuard (0x540b20) allocates 0x2c and
// installs vtable 0x640a84. Slot 3 (0x5331f0) serializes the id, count and
// disposition below; +0x28 is neither initialized nor read by those bodies.
class type_monster : public type_object {
public:
    int m_objectId;       // +0x1c
    int m_count;          // +0x20, serialized as two bytes
    int m_disposition;    // +0x24, serialized as one byte
    int m_unknown28;      // +0x28

    type_monster(TRmgObjectPropertiesRef* properties, int objectId, int count)
        : type_object(properties)
    {
        m_count = count;
        m_disposition = RMG_GUARD_DISPOSITION;
        m_objectId = objectId;
    }
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(type_monster, 0x2c);

// Complete town vtable 0x640a94; constructor expansion at 0x54543d
// stores owner/fort/id in the 0x28-byte allocation. Names are role-derived.
// Value/reference argument combinations and an ordinary out-of-class body
// leave both placement callers unchanged; neither model improves their residual.
class type_town : public type_object {
public:
    int m_objectId;
    int m_player;
    unsigned char m_hasFort;
    type_town(TRmgObjectPropertiesRef* properties, int objectId,
        int player, unsigned char hasFort) : type_object(properties)
    {
        m_player = player;
        m_hasFort = hasFort;
        m_objectId = objectId;
    }
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(type_town, 0x28);

// Provisional Complete-only role. The shipyard path allocates 0x1c bytes,
// calls type_object's constructor, then replaces its vptr with 0x640aa4.
// That table shares the base's middle slots and overrides serialization:
// 0x533460 appends an unowned player byte and three reserved bytes.
class type_flaggable : public type_object {
public:
    type_flaggable(TRmgObjectPropertiesRef* properties)
        : type_object(properties) {}
    virtual void write(TAbstractFile* outfile, int version);
};

// Artifact factory 0x5341f0 allocates the base 0x1c extent and installs
// vtable 0x640ab4. Its writer adds one zero byte after type_object's record;
// no additional instance fields are present. Complete-only role spelling.
class type_random_artifact : public type_object {
public:
    type_random_artifact(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(type_random_artifact, 0x1c);

// These four factories allocate the same 0x1c base extent and change only
// the writer vptr. Their distinct default H3M payloads prove separate classes;
// the Complete-only class spellings below describe those roles.
// Retail vtable 0x640ac4. The editor's class hierarchy descriptor lists
// type_resource_lump <- type_random_artifact <- type_object.
class type_resource_lump : public type_random_artifact {
public:
    type_resource_lump(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(type_resource_lump, 0x1c);

// Pandora's Box factories 0x534380/0x534410/0x534490 allocate 0x54 bytes
// and install vtable 0x640ad4. Writer 0x5336f0 identifies each payload field;
// the spell factory 0x534520 appends integer spell indices to the vector.
// The vector begins at +0x44 (its allocator byte), with _First at +0x48.
// These are provisional Complete-only role names, not Dreamcast identities.
class type_black_box : public type_object {
public:
    int m_experience;                  // +0x1c
    int m_resources[7];                // +0x20, gold at +0x38
    int m_creatureType;                // +0x3c, -1 means no creature reward
    int m_creatureCount;               // +0x40
    std::vector<int> m_spells;         // +0x44

    type_black_box(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(type_black_box, 0x54);

// Seer-hut factories 0x534b90/0x534cc0/0x534db0 allocate this 0x34-byte
// reward object (vtable 0x640b04). Writer 0x533a90 proves the field roles.
// Complete-only names are provisional; no Dreamcast RMG class is available.
class type_seer_hut : public type_object {
public:
    int m_artifact;                    // +0x1c, required quest artifact
    int m_experience;                  // +0x20
    int m_resourceType;                // +0x24, defaults to gold (6)
    int m_resourceCount;               // +0x28
    int m_creatureType;                // +0x2c, defaults to -1
    int m_creatureCount;               // +0x30

    type_seer_hut(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(type_seer_hut, 0x34);

// Artifact wrapper vtable 0x640af4 shares the ordinary artifact writer at
// 0x533500. It owns the pending seer hut until placement transfers it to
// the map; 0x533a50 clears that pointer on both success and failure.
class t_quest_artifact : public type_random_artifact {
public:
    type_random_map_generator* m_generator; // +0x1c
    type_seer_hut* m_seerHut;             // +0x20
    type_treasure_def* m_definition;        // +0x24

    t_quest_artifact(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, type_seer_hut* seerHut,
        type_treasure_def* definition);
    virtual ~t_quest_artifact();
    virtual unsigned char completePlacement();
};
SIZE(t_quest_artifact, 0x28);

// Key-tent definition factory 0x534fd0 allocates 0x24 bytes, installs
// vtable 0x640ae4, and supplies its generator and value. The writable
// override tries a corresponding guard, then substitutes another treasure
// if that placement fails. Its record uses the ordinary object writer.
// Complete-only class and method spellings describe the recovered roles.
class type_key_tent : public type_object {
public:
    type_random_map_generator* m_generator; // +0x1c
    int m_value;                           // +0x20

    type_key_tent(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, int value);
    virtual unsigned char completePlacement();
};
SIZE(type_key_tent, 0x24);

// Retail vtable 0x640b24.
class type_scholar : public type_object {
public:
    type_scholar(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(type_scholar, 0x1c);

// Retail vtable 0x640b34.
class type_shrine : public type_object {
public:
    type_shrine(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(type_shrine, 0x1c);

// Complete-only spell-scroll object. Factory 0x534ed0 allocates 0x20
// bytes, stores its selected spell at +0x1c and installs vtable 0x640b44.
// Writer 0x533ff0 emits that spell as one byte. Original class name unknown.
// Full-build collateral on admission: unchanged CEnterNameEdit::onKillFocus
// CUR 100 -> 99.8710 (two loads exchange order); MAX/HIST retain 100.
class type_spell_scroll : public type_object {
public:
    int m_spell; // +0x1c, role-derived name
    type_spell_scroll(TRmgObjectPropertiesRef* properties, int spell);
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(type_spell_scroll, 0x20);

// Retail vtable 0x640b54.
class type_witch_hut : public type_object {
public:
    type_witch_hut(TRmgObjectPropertiesRef* properties);
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(type_witch_hut, 0x1c);

// Factory 0x5348d0 allocates this 0x2c-byte derived object after reserving a
// hero. Vtable 0x640b14 slot 1 releases that reservation through the generator
// at +0x1c; the original Complete-only class spelling is unavailable.
class type_hero : public type_object {
public:
    type_random_map_generator* m_generator; // +0x1c
    int m_objectId;                         // +0x20
    int m_heroIndex;                        // +0x24
    int m_experience;                       // +0x28, prison definition experience

    type_hero(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, const int& objectId, int heroIndex,
        int experience);

    virtual void releaseReservation();
    virtual void write(TAbstractFile* outfile, int version);
};
SIZE(type_hero, 0x2c);

struct TRmgMapItem {
    std::vector<type_object*> m_objects;    // +0x00
    TRmgMapPosition m_previousTile;         // +0x10
    TRmgMovementCost m_movement;            // +0x1c
    TRmgZoneCellState m_zoneState;           // +0x20
    TRmgGroundTile m_tile;                  // +0x24
    TRmgGroundTileData m_tileData;          // +0x28
    TRmgBorderConnection m_connection;  // +0x2c

    TRmgMapItem();
    void clear();
    void write(TAbstractFile* outfile);
    // Retained cell writer 0x546940; four scalar inputs, terrain fields only.
    void setTerrain(int terrain, int frame,
        unsigned char flipX, unsigned char flipY);

    // CreateRiver's predicate reads shift the high tile bits and test a
    // byte result. These queries recover that boundary; direct field tests
    // instead use dword masks. Names remain provisional without RMG symbols.
    bool isRiverJoinTarget() const { return m_tileData.m_riverJoinTarget != 0; }
    bool isRiverOutletTarget() const { return m_tileData.m_riverOutletTarget != 0; }
    bool isNearRiver() const { return m_tileData.m_nearRiver != 0; }

    // PaintZoneTerrain extracts bit 28 then tests its byte result. The
    // direct field condition instead folds to a dword mask. Retail-only
    // accessor hypothesis, consistent with the adjacent flag queries.
    unsigned char shouldPaintZoneTerrain() const
    {
        return m_tileData.m_paintZoneTerrain;
    }

    // Placement helpers 0x531170/0x5318b0/0x531cf0 all shift bit 22 and
    // test the truncated byte. Direct bitfield conditions fold to a dword
    // mask; keep this same ordinary query at each recovered boundary.
    unsigned char isObjectEntrance() const
    {
        return m_tileData.m_objectEntrance;
    }

    // ScoreObjectPlacement reads path clearance (bit 27) with shr/test dl.
    // This byte result preserves that narrow test; a direct bitfield
    // condition instead folds to test dword ptr [item+0x28],imm.
    unsigned char hasPathClearance() const
    {
        return m_tileData.m_pathClearance;
    }

    // Native Mac group callers expand this byte-valued predicate. The
    // ordinary source-defined control retains calls; direct field tests
    // omit its Boolean result. This class body reproduces that predicate
    // shape; its seven-unit Windows control loses no exact functions.
    // Water remains passable here; callers apply their own water policy.
    unsigned char isPassable() const
    {
        return m_tileData.m_passable && getLandType() != eTerrainRock;
    }

    // RepairWaterZoneBorders tests this flag after truncating it to a byte
    // at 0x53fe30, then tests passability directly as a dword bit.
    unsigned char hasObstacleFill() const
    {
        return m_tileData.m_obstacleFill;
    }

    // Border connections protect their cells from changes to path
    // reservations. Setting either mark clears the other.
    void setObstacleFill(unsigned char obstacleFill);
    void setPathClearance(unsigned char pathClearance);

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

class type_random_map : public TTerrainPlacementOp::TAbstractMap {
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
    // Paired TRmgPoint/grid-point size arguments, by value/reference and in both
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
        const TTilePoint& point, const TRmgTerrainTile& tile);
    virtual void setFrame(const TTilePoint& point, int value);
#if defined(HOMM3_TARGET_MAC)
    virtual TTilePoint getSize();  // Mac 0x22eb84: hidden value result
#else
    virtual TTilePoint& getSize(TTilePoint& output);
#endif
    virtual TRmgTerrainTile getTile(const TTilePoint& point);
    virtual int getTerrain(const TTilePoint& point);
    virtual int getFrame(const TTilePoint& point);

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
        return m_mapItems + (z * m_mapHeight + y) * m_mapWidth + x;
    }
    TRmgMapItem* getMapItem(TRmgMapPosition point);

    // Complete-only path carving at 0x543e20 calls these retained map
    // helpers. Names describe the observed cell flags and ray traversal.
    void openPathPatch(int x, int y, int level);
    void markObstacleFillPatch(TRmgMapPosition position);
    TRmgPoint traceBranchEnd(TRmgPoint from, TRmgPoint toward, int level);

    unsigned char hasConnectedOutline(
        const std::vector<TRmgPoint>& outline, TRmgMapPosition position,
        unsigned char allowEntrances, TRmgZone* zone, unsigned char requirePathClearance);
    unsigned char isPlacementBlocked(
        TRmgObjectPropertiesRef* properties, TRmgMapPosition position,
        int zoneIndex, unsigned char rejectObstacleFill);
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
    std::vector<TRmgPoint> m_outline;           // +0x38
    // with the guard's local coordinates; canPlaceTreasureGroup checks them.
    unsigned char m_hasGuard;               // +0x48, cleared by reset
    char m_padding0049[3];
    // addGuard stores x/y at 0x53556f.
    // canPlaceTreasureGroup reads x/y
    // from +0x4c/+0x50 before translating the guard's neighborhood.
    TRmgPoint m_guardPosition;                 // +0x4c
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
    void discard();
    void markPlacementOutline();
    unsigned char addGuard(type_object* guard);
    unsigned char canFitObject(TRmgObjectPropertiesRef* properties, TRmgMapPosition position);
    unsigned char tryAddObject(type_object* object);
    unsigned char objectsAllowEntrances() const;
    void addObject(type_object* object, TRmgPoint point);
    void updateBounds();
    void traceOutline();
};
SIZE(TRmgTreasureGroup, 0x64);

#include "lineplacement.h"

// Complete-only road adapter, provisional role name. Vtable 0x640a04 has
// the seven-slot road interface; 0x548120 constructs the eight-byte object
// with the address of a type_random_map view at +4. Its methods independently
// index that map's 0x30-byte cells and read/write the road packed fields.
class type_road_map : public TRoadOp::TAbstractMap {
public:
    type_random_map* m_map;

    type_road_map(type_random_map* map) : m_map(map) {}
    virtual void setTile(const TTilePoint& point, const TRmgTerrainTile& tile);
    virtual void setLineType(const TTilePoint& point, int value);
    virtual TTilePoint getSize();
    virtual TRmgTerrainTile getTile(const TTilePoint& point);
    virtual int getLineType(const TTilePoint& point);
    virtual int getTerrain(const TTilePoint& point);
};

// Retail retains these support bodies outside CreateRiver while the adapter
// and map-view construction remains expanded at the call site.  Keeping the
// class definitions shared but the retained bodies in rmg_river.cpp and
// rmg_road.cpp reproduces that ordinary translation-unit visibility boundary.
class type_river_map : public TRiverOp::TAbstractMap {
public:
    type_random_map* m_map;

    inline type_river_map(type_random_map* newMap) : m_map(newMap) {}

    virtual void setTile(
        const TTilePoint& point, const TRmgTerrainTile& tile);
    virtual void setLineType(const TTilePoint& point, int value);
    virtual TTilePoint getSize();
    virtual TRmgTerrainTile getTile(const TTilePoint& point);
    virtual int getLineType(const TTilePoint& point);
    virtual int getTerrain(const TTilePoint& point);
};

// A generated zone owns both its template metadata and the Complete-only
// connection state.  WriteMapHeader proves the player/town fields through
// +0x3c; the connection pass independently proves the bounding rectangle and
// entrance vector at +0x404.  The 0x1c-stride connection vector belongs to
// the template record reached through `m_templateZone`, not to this generated zone.
struct TRmgZone {
    TRmgTemplateZone* m_templateZone;              // +0x00
    int m_alignment;                   // +0x04
    // H3API H3RmgZoneGenerator::townType2, INT32 at +08, commit
    // 92255ab18da784a5842ecc2b8bc0ce00e19a0c56. The surrounding town/terrain,
    // coordinates, object-count array and three vectors match this layout.
    // Retail type_map_dwelling_def::getValue at 0x5347f0 compares this
    // field with the dwelling creature's town type before valuing it.
    // Retail creature reward value 0x534324 compares this with the creature's
    // town alignment before weighting the reward by active-zone counts.
    int m_creatureTownType;
    // chooseTerrain 0x532ab0 stores the integer ordinal from its 0..7
    // selection loop; tryPlaceMine uses the same ordinal as a bitset index.
    // There is no DC enum ABI for this Complete-only field. Keep the field
    // and its local consumer consistent instead of casting into an inferred
    // enum after every selection. Named terrain constants share the encoding.
    int m_terrain;                      // +0x0c
    TRmgMapPosition m_levelPosition;   // +0x10
    int m_scaledSize;            // +0x1c: layout-scaled size
    TRmgZoneBounds m_bounds;           // +0x20
    TRmgMapPosition m_primaryTownEntrance; // +0x30: main town
    unsigned char m_hasPrimaryTown;            // +0x3c
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
    std::vector<TRmgPoint> m_boundary;    // +0x3f4: clipped polygon vertices
    std::vector<TRmgPoint> m_entrances;   // +0x404

    TRmgZone(TRmgTemplateZone* slot);
    void decrementObjectCount(TAdventureObjectType objectType);
    void chooseCreatureTownType(unsigned char expanded);
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
    TRmgMapPosition getPrimaryTownEntrance() const;
    // Template slot radius; the position filter reads it through this
    // accessor, which is what makes its first counting pass call size().
    int getSize() const
    {
        return m_templateZone->m_size;
    }
    unsigned char canConnect(const TRmgZone* other) const;
};

// Partial Voronoi topology recovered from TraceZoneBoundary and its caller
// at 0x53e050. The twin's owning zone identifies the region across an edge;
// following next traverses a closed polygon. Names are provisional.
struct TRmgHalfEdge {
    // The paired-edge constructor 0x5fcef0
    // copies a by-value point into +0/+4 and its zone into +8.
    // buildVertices 0x5fdb40 subtracts these site coordinates while
    // calculating the boundary point at +0x1c. Role-derived name.
    TRmgPoint m_sitePosition;               // +0x00
    TRmgZone* m_zone;                   // +0x08
    TRmgHalfEdge* m_twin;        // +0x0c
    TRmgHalfEdge* m_next;        // +0x10
    // Constructor 0x5fcef0 initializes both
    // ring links to self; splice 0x5fcf60 swaps next->previous together
    // with next, preserving the backward link. Role-derived name.
    TRmgHalfEdge* m_previous;     // +0x14
    // Constructor clears this byte. buildVertices tests it at 0x5fdb7a,
    // writes the computed point, then sets it on three incident edges at
    // 0x5fdc7d/89/9e. +0x19..1b is natural alignment before the point.
    unsigned char m_vertexComputed;  // +0x18
    TRmgPoint m_vertex;                  // +0x1c

    // The 0x5fcef0 retained constructor takes two by-value point/zone
    // pairs (ret 0x18), allocating the opposite half-edge at +0x0c.
    // Its expanded twin constructor takes the existing edge pointer.
    TRmgHalfEdge(TRmgPoint sitePosition, TRmgZone* zone,
        TRmgPoint twinSitePosition, TRmgZone* twinZone);
    TRmgHalfEdge(TRmgPoint sitePosition, TRmgZone* zone,
        TRmgHalfEdge* twin);
    // Role-derived names: 0x5fcf60 exchanges forward/backward ring links;
    // 0x5fcfa0 applies it to each half-edge and its predecessor.
    // Ordinary; both constructors expand it (see rmg_voronoi.cpp).
    void initialize();
    void splice(TRmgHalfEdge* other);
    void detach();
    // Quad-edge navigation (Graphics Gems IV Sym/Onext/Oprev/Lnext/Lprev,
    // Org2d/Dest2d): these inline accessors are candidate sites for the
    // /Ob2 inliner, and their count is what makes retail refuse the fan
    // splices, distance and orientation calls in addSite, the twin splice
    // in removeEdge and the fifth createEdge in the diagram constructor.
    // Site positions return by value: addSite's coincidence test loads both
    // coordinates before comparing, as a copied temporary does.
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
        return m_twin->m_previous;
    }
    TRmgHalfEdge* getLeftPrevious() const
    {
        return m_next->m_twin;
    }
    TRmgPoint getSitePosition() const
    {
        return m_sitePosition;
    }
    TRmgPoint getOppositeSitePosition() const
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
        return m_vertexComputed;
    }
    void setPosition(const TRmgPoint& position)
    {
        // A named snapshot gives buildVertices retail's final two-coordinate
        // transfer before each of its three expanded stores.
        TRmgPoint copy = position;
        m_vertex = copy;
        m_vertexComputed = 1;
    }
};
SIZE(TRmgHalfEdge, 0x24);

// The retained subdivision constructor and destructor own a root edge and
// a vector of allocated edges. The coordinator inserts zone sites, computes
// dual vertices, then looks up an edge for each site. All names are provisional.
class TRmgVoronoi {
public:
    TRmgHalfEdge* m_startingEdge;                 // +0x00
    std::vector<TRmgHalfEdge*> m_edges;  // +0x04

    TRmgVoronoi();
    ~TRmgVoronoi();
    // Retained 0x5fd390 creates and owns both halves; two point/zone pairs.
    TRmgHalfEdge* createEdge(TRmgPoint first, TRmgZone* firstZone,
        TRmgPoint second, TRmgZone* secondZone);
    TRmgHalfEdge* connectEdges(TRmgHalfEdge* first,
        TRmgHalfEdge* second);
    void removeEdge(TRmgHalfEdge* edge);
    void addSite(TRmgPoint point, TRmgZone* zone);
    TRmgHalfEdge* locate(TRmgPoint point);
    void buildVertices();
};
SIZE(TRmgVoronoi, 0x14);

SIZE(TRmgMapPosition, 0xc);
SIZE(TRmgZoneConnection, 0x1c);
SIZE(TRmgZoneBounds, 0x10);
SIZE(TRmgZone, 0x414);



// Complete-only 0x543e20 chooses one of these four initial branch segments.
// Names describe the endpoint stores; the original source spelling is unknown.


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
// this retained base boundary. t_abstract_random_generator is a provisional name.
class t_abstract_random_generator {
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
    std::vector<type_object*> m_objects;             // +0xec4
    type_progress_bar* m_progress;                          // +0xed4
    t_abstract_random_generator(int width, int height, int levels,
        type_progress_bar* progress, int additionalSteps, int version);
    virtual ~t_abstract_random_generator();
    virtual void addObject(type_object* object, TRmgMapPosition position);
    // Retained 0x536200 loads object records, builds the per-type vectors,
    // then calls the placement-rule loader. Both bodies are in rmg.cpp.
    void loadObjectPrototypes();
    void readObjectPlacementRules();
    int scoreObjectPlacement(
        TRmgObjectPropertiesRef* properties, TRmgMapPosition position);
    void decorateMap();
    void decorateMapCell(TRmgMapPosition position, int progressSteps);
};
SIZE(t_abstract_random_generator, 0xed8);

// Four fixed-count/density groups consumed by 0x544ae0 distinguish player and
// neutral towns, each with or without a starting fort.


class type_random_map_generator : public t_abstract_random_generator {
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
    // The retail constructor/roster does not initialize this field: its
    // first value comes from the caller's stack. Preserve that behavior;
    // execution comparisons must supply identical initial stack contents.
    int m_nextKeyTentColor;                            // +0x0f5c
    // 0x549bae clears nine alignment counts; 0x549be0..0x549c05 counts
    // zones with a primary town, both by their alignment (+4) and in the total.
    int m_primaryTownZoneCount;                        // +0x0f60
    int m_primaryTownZoneCountsByAlignment[9];          // +0x0f64
    unsigned char m_disabledHeroes[RMG_HERO_COUNT];               // +0x0f88
    // Role-derived names; original spellings unknown. Replaces opaque1024.
    // Ctor 0x537cc6 clears 144 bytes. Quest selection 0x54b490 excludes
    // marked artifacts; successful placement 0x54b813 marks the chosen ID.
    unsigned char m_usedQuestArtifacts[ARTIFACT_COUNT];           // +0x1024
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
        int waterContent, int monsterStrength, type_progress_bar* progress, int version);
    void loadTemplates();
    // Role-derived from generation coordinator 0x549b30 and placement 0x545250.
    void placeMines();
    // Provisional roles from the Complete-only connection coordinator.
    void prepareZoneConnections();
    void markZoneBoundaryObstacles();
    void populateWaterZoneIslands(TRmgZone* zone);
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
        int player, unsigned char hasFort);
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
    void drawIslandBoundary(TRmgPoint from, TRmgPoint to, int zoneIndex, int level, int roughness);
    void placeAdditionalTowns(TRmgZone* zone);
    unsigned char tryPlaceAdditionalTown(TRmgZone* zone, int alignment,
        int player, unsigned char hasFort, int spacing);
    void prepareJunctionZone(TRmgZone* zone);
    void connectJunctionEntrance(TRmgPoint from, TRmgPoint to, TRmgZone* zone);
    void placeZoneTreasures(TRmgZone* zone);
    type_object* createTreasureObject(TRmgZone* zone, int minimum, int maximum,
        int* value, unsigned char primary, unsigned char allowLinkedPlacement,
        unsigned char compact, TRmgMapPosition position);
    int fillTreasureGroup(TRmgZone* zone, TRmgTreasureGroup* group,
        unsigned char compact, int value);
    unsigned char assembleTreasureGroup(TRmgZone* zone, TRmgTreasureGroup* group,
        unsigned char compact, int minimum, int maximum);
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
#if defined(HOMM3_RMG_HOTFIX)
        // Unknown request versions are written as Shadow of Death maps.
        return 28;
#else
        // Retail bug: another version falls off the end of this function.
#endif
    }

    void initializeObjectGenerators();
    int selectPrisonHero();
    unsigned char canPlaceZone(TRmgZone* zone);
    void buildZoneBoundaries(TRmgTemplate* mapTemplate, int level);
    // Complete-only 0x53d8e0 propagates each signed-short distance column
    // from one zone through the template connection graph.
    void propagateZoneDistances(TRmgZone* zone);
    void fillZoneArea(TRmgZone* zone, TRmgHalfEdge* first);
    void joinExtraZones(int originalZones, TRmgVoronoi* diagram);
    int countPlacedZoneConnections(TRmgZone* zone) const;
    void filterZonePositions(
        TRmgZone* zone, std::vector<TRmgMapPosition>& candidates, int mapSize);
    void drawIrregularZoneBoundary(
        TRmgPoint from, TRmgPoint to, int zoneIndex, int level, int roughness);
    void drawStraightZoneBoundary(
        TRmgPoint from, TRmgPoint to, int zoneIndex, int level);
    void traceZoneBoundary(TRmgHalfEdge* first, unsigned char irregular);
    unsigned char createGroundConnection(
        TRmgZone* source,
        TRmgZoneConnection* connection,
        std::vector<TRmgMapItem*>* borderItems,
        std::vector<TRmgMapPosition>* borderPositions);
    void floodConnectionRegion(TRmgMapPosition position);
    void floodShipyardWater(type_object* shipyard);
    // Earlier provisional name: CreateBorderConnection/createBorderConnection.
    // Retail 0x541ad0 selects objectPrototypes[SHIPYARD] and places it beside
    // reachable water. No Dreamcast RMG name is available.
    unsigned char createShipyardConnection(
        TRmgZone* source, TRmgZoneConnection* connection);
    unsigned char canPlaceShipyard(TRmgMapPosition position);
    unsigned char createSubterraneanGate(
        TRmgZone* source, TRmgZoneConnection* connection);
    // Complete-only 0x542b00 places and marks a monolith entrance border.
    unsigned char placeMonolithBorderGuard(TRmgMapPosition position, TRmgZone* zone);
    void createMonolithConnection(
        TRmgZone* source,
        TRmgZoneConnection* connection,
        int prototypeIndex);
    void connectZones();
    // Retail 0x543e20: random midpoint displacement, queued side branches,
    // then terrain and border cleanup. No Dreamcast RMG names survive.
    bool contains(const TRmgPoint& point) const;
    void carveBranchingPaths();
    void repairWaterZoneBorders();
    // Complete-only roles proved by the predecessor walk at 0x5408e0 and
    // the surrounding connection-cell updates at 0x540fc0.
    void openConnectionPath(TRmgMapPosition position, unsigned char narrow);
    void markBorderObjectArea(TRmgMapPosition position, int color);
    int placeBorderGuard(
        TRmgMapPosition position, int count, TRmgZone* zone);
    type_object* createGuard(int value, TRmgZone* zone);
    unsigned char placeObjectInZone(type_object* object, TRmgZone* zone);
    void placeGuard(int value, TRmgMapPosition position);
    int getZoneGuardValue(const TRmgZone* zone, int value) const;
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
    unsigned char placeSeerHutForArtifact(t_quest_artifact* object);
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
#if defined(HOMM3_RMG_HOTFIX)
    bool hasRequiredPrototypes() const;
    bool hasPlayerTowns() const;
#endif
    void setHumanPlayer(int seat);
    void setTownChoice(int seat, int town);
    void removeObject(type_object* object);
    // Retail 0x548040 walks predecessor runs for the caller at 0x548408.
    // The Complete-only name is provisional; the by-value ABI is proven.
    unsigned char paintRoad(TRmgMapPosition position, int roadType);
    // Provisional spelling: retail's water-wheel caller and the river-delta
    // object selection prove the role; the Dreamcast build has no RMG TU.
    void createRiverToOutlet(TRmgMapPosition source);
    void markRiverJoinTargets();
    void markRiverOutletTargets();
    void markRiverCoastTarget(TRmgMapPosition position, int direction);
    void createRiverToJoin(TRmgMapPosition source);
    void createRivers();
    void writeMapHeader(TAbstractFile* outfile);
};

SIZE(TRmgMapPosition, 0x0c);
SIZE(TRmgPoint, 0x08);
SIZE(TTilePoint, 0x08);
SIZE(TRmgRiverDeltaOffset, 0x08);
SIZE(TRmgMovementCost, 0x04);
SIZE(TRmgZoneCellState, 0x04);
SIZE(TRmgGroundTile, 0x04);
SIZE(TRmgGroundTileData, 0x04);
SIZE(TRmgBorderConnection, 0x04);
SIZE(TRmgObjectPlacementRule, 0x4c);
SIZE(TRmgObjectPropertiesRef, 0xe8);
SIZE(type_object, 0x1c);
SIZE(type_flaggable, 0x1c);
SIZE(TRmgMapItem, 0x30);
SIZE(type_random_map, 0x18);
SIZE(TTerrainPlacementOp::TAbstractMap, 0x04);
SIZE(TRiverOp::TAbstractMap, 0x04);
SIZE(TRoadOp::TAbstractMap, 0x04);
SIZE(type_river_map, 0x08);
SIZE(TMapLineFilter, 0x0c);
SIZE(TRiverOp, 0x10);
SIZE(TRoadOp, 0x10);
SIZE(TRmgLineWalker, 0x10);
SIZE(TRiverPlacementOp, 0x20);
SIZE(TRoadPlacementOp, 0x20);
SIZE(type_random_map_generator, 0x14e0);

// Retail 0x6824e0 is indexed by the creature-traits level dword before
// type_black_box_creature_def divides by that creature's AI value.
DATA(0x006824e0) extern int g_rmgCreatureValueByLevel[];

#endif  // HOMM3_RMG_H
