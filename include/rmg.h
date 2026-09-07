// Complete-only random-map generator declarations.
#ifndef HOMM3_RMG_H
#define HOMM3_RMG_H

#include <bitset>
#include <string>
#include <vector>
#include <va.h>
#include "terrain_type.h"

class TAbstractFile;
class TSpreadsheetResource;
struct TRmgTownSlot;
struct TRmgZone;
struct rmgTerrainTile;
struct TPoint;
struct TObjectType;

// The abstract progress sink driven by Complete's random-map generator.
// Retail constructor 0x530e20 stores vtable 0x6409c0, the step total at +4,
// and zero at +8. The vtable holds a scalar deleting destructor at 0x530e40,
// SetTotal at 0x530e80, and _purecall in the Advance slot.
class TProgressSink {
public:
    // Before normalization: steps.
    int m_steps;
    // Before normalization: done.
    int m_done;

    TProgressSink(int totalSteps);
    virtual ~TProgressSink();
    // Before normalization (function): TProgressSink::SetTotal.
    virtual void setTotal(int totalSteps);
    // Before normalization (function): TProgressSink::Advance.
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
    // Before normalization: objectType.
    int m_objectType;
    // Before normalization: subtype.
    int m_subtype;
    // Before normalization: value.
    int m_value;
    // Before normalization: density.
    int m_density;

    type_treasure_def(int objectType, int subtype, int value, int density);

    // Before normalization (function): type_treasure_def::Generate.
    virtual void* generate(void* owner, int x, int y);
    // Before normalization (function): type_treasure_def::GetValue.
    virtual int getValue(void* object, void* map);
    // Before normalization (function): type_treasure_def::IsTerrainDependent.
    virtual unsigned char isTerrainDependent();
};

SIZE(type_treasure_def, 0x14);

// These identities come from the contiguous cross-build vtable roster.  The
// current-image constructor relocations independently fix each table address.
class type_shrine_def : public type_treasure_def {
public:
    type_shrine_def(int objectType, int value);
    // Before normalization (function): type_shrine_def::Generate.
    virtual void* generate(void* owner, int x, int y);
};

class type_witch_hut_def : public type_treasure_def {
public:
    type_witch_hut_def();
    // Before normalization (function): type_witch_hut_def::Generate.
    virtual void* generate(void* owner, int x, int y);
};

class type_spell_scroll_def : public type_treasure_def {
public:
    // Before normalization: spellLevel.
    int m_spellLevel;

    type_spell_scroll_def(int spellLevel, int value);
    // Before normalization (function): type_spell_scroll_def::Generate.
    virtual void* generate(void* owner, int x, int y);
};

class type_black_box_creature_def : public type_treasure_def {
public:
    // Before normalization: creatureType.
    int m_creatureType;
    // Before normalization: adjustedValue.
    int m_adjustedValue;

    type_black_box_creature_def(int creatureType);
    // Before normalization (function): type_black_box_creature_def::Generate.
    virtual void* generate(void* owner, int x, int y);
    // Before normalization (function): type_black_box_creature_def::GetValue.
    virtual int getValue(void* object, void* map);
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

    // Before normalization (function): type_artifact_def::Generate.
    virtual void* generate(void* owner, int x, int y);
};

class type_black_box_experience_def : public type_treasure_def {
public:
    // Before normalization: experience.
    int m_experience;

    inline type_black_box_experience_def(int value, int experience)
        : type_treasure_def(6, 0, value, 20)
    {
        this->m_experience = experience;
    }

    // Before normalization (function): type_black_box_experience_def::Generate.
    virtual void* generate(void* owner, int x, int y);
};

class type_black_box_gold_def : public type_treasure_def {
public:
    // Before normalization: gold.
    int m_gold;

    inline type_black_box_gold_def(int value, int gold)
        : type_treasure_def(6, 0, value, 5)
    {
        this->m_gold = gold;
    }

    // Before normalization (function): type_black_box_gold_def::Generate.
    virtual void* generate(void* owner, int x, int y);
};

class type_black_box_spells_def : public type_treasure_def {
public:
    // Before normalization: minimumLevel.
    int m_minimumLevel;
    // Before normalization: maximumLevel.
    int m_maximumLevel;
    // Before normalization: schoolMask.
    int m_schoolMask;

    inline type_black_box_spells_def(
        int value, int minimumLevel, int maximumLevel, int schoolMask)
        : type_treasure_def(6, 0, value, 2)
    {
        this->m_minimumLevel = minimumLevel;
        this->m_maximumLevel = maximumLevel;
        this->m_schoolMask = schoolMask;
    }

    // Before normalization (function): type_black_box_spells_def::Generate.
    virtual void* generate(void* owner, int x, int y);
};

class type_key_tent_def : public type_treasure_def {
public:
    inline type_key_tent_def(int subtype, int value)
        : type_treasure_def(10, subtype, value, 10)
    {
    }

    // Before normalization (function): type_key_tent_def::Generate.
    virtual void* generate(void* owner, int x, int y);
    // Before normalization (function): type_key_tent_def::GetValue.
    virtual int getValue(void* object, void* map);
    // Before normalization (function): type_key_tent_def::IsTerrainDependent.
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

    // Before normalization (function): type_dwelling_def::Generate.
    virtual void* generate(void* owner, int x, int y);
};

class type_map_dwelling_def : public type_dwelling_def {
public:
    inline type_map_dwelling_def(int subtype)
        : type_dwelling_def(subtype)
    {
    }

    // Before normalization (function): type_map_dwelling_def::GetValue.
    virtual int getValue(void* object, void* map);
};

class type_resource_lump_def : public type_treasure_def {
public:
    inline type_resource_lump_def(
        int objectType, int subtype, int value, int density)
        : type_treasure_def(objectType, subtype, value, density)
    {
    }

    // Before normalization (function): type_resource_lump_def::Generate.
    virtual void* generate(void* owner, int x, int y);
};

class type_prison_def : public type_treasure_def {
public:
    // Before normalization: experience.
    int m_experience;

    inline type_prison_def(int value, int experience)
        : type_treasure_def(62, 0, value, 30)
    {
        this->m_experience = experience;
    }

    // Before normalization (function): type_prison_def::Generate.
    virtual void* generate(void* owner, int x, int y);
};

class type_scholar_def : public type_treasure_def {
public:
    inline type_scholar_def()
        : type_treasure_def(81, 0, 1500, 100)
    {
    }

    // Before normalization (function): type_scholar_def::Generate.
    virtual void* generate(void* owner, int x, int y);
};

class type_quest_creature_def : public type_black_box_creature_def {
public:
    inline type_quest_creature_def(int creatureType, int questIndex)
        : type_black_box_creature_def(creatureType)
    {
        m_objectType = 83;
        m_subtype = questIndex;
    }

    // Before normalization (function): type_quest_creature_def::Generate.
    virtual void* generate(void* owner, int x, int y);
    // Before normalization (function): type_quest_creature_def::GetValue.
    virtual int getValue(void* object, void* map);
    // Before normalization (function): type_quest_creature_def::IsTerrainDependent.
    virtual unsigned char isTerrainDependent();
};

class type_quest_experience_def : public type_treasure_def {
public:
    // Before normalization: experience.
    int m_experience;

    inline type_quest_experience_def(
        int questIndex, int value, int experience)
        : type_treasure_def(83, questIndex, value, 10)
    {
        this->m_experience = experience;
    }

    // Before normalization (function): type_quest_experience_def::Generate.
    virtual void* generate(void* owner, int x, int y);
    // Before normalization (function): type_quest_experience_def::GetValue.
    virtual int getValue(void* object, void* map);
    // Before normalization (function): type_quest_experience_def::IsTerrainDependent.
    virtual unsigned char isTerrainDependent();
};

class type_quest_gold_def : public type_treasure_def {
public:
    // Before normalization: gold.
    int m_gold;

    inline type_quest_gold_def(int questIndex, int value, int gold)
        : type_treasure_def(83, questIndex, value, 10)
    {
        this->m_gold = gold;
    }

    // Before normalization (function): type_quest_gold_def::Generate.
    virtual void* generate(void* owner, int x, int y);
    // Before normalization (function): type_quest_gold_def::GetValue.
    virtual int getValue(void* object, void* map);
    // Before normalization (function): type_quest_gold_def::IsTerrainDependent.
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
    // Before normalization: x.
    int m_x;
    // Before normalization: y.
    int m_y;
    // Before normalization: z.
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
    // Before normalization: destination.
    TRmgTownSlot* m_destination;             // +0x00
    // Before normalization: value.
    int m_value;                             // +0x04
    // Before normalization: unguarded.
    unsigned char m_unguarded;               // +0x08
    // Before normalization: placeBorderObjects.
    unsigned char m_placeBorderObjects;      // +0x09
    // Before normalization: connected.
    unsigned char m_connected;               // +0x0a
    // Replaces synthetic opaque000b: +0x0b aligns four int limits.
    // Retail connection reader 0x5382c9..0x538304 parses spreadsheet
    // columns 81..84 into +0x0c/+0x10/+0x14/+0x18. At 0x538307..0x53832b
    // it compares the first pair with humanPlayerCount and the second
    // pair with humanPlayerCount + computerPlayerCount before insertion.
    // Role-derived names, consistent with the TRmgTownSlot limits below.
    // Before normalization: minimumHumanPlayers.
    int m_minimumHumanPlayers;               // +0x0c
    // Before normalization: maximumHumanPlayers.
    int m_maximumHumanPlayers;               // +0x10
    // Before normalization: minimumPlayers.
    int m_minimumPlayers;                    // +0x14
    // Before normalization: maximumPlayers.
    int m_maximumPlayers;                    // +0x18
};

enum ERmgTemplateZoneKind {
    RMG_TEMPLATE_HUMAN = 0,
    RMG_TEMPLATE_COMPUTER = 1,
    RMG_TEMPLATE_TREASURE = 2,
    RMG_TEMPLATE_JUNCTION = 3
};

struct TRmgTreasureRange {
    // Before normalization: minimum.
    int m_minimum;
    // Before normalization: maximum.
    int m_maximum;
    // Before normalization: density.
    int m_density;
};

// ReadRmgTemplateZones allocates 0xd4 bytes and constructs connections at
// +0xc4. ConnectZones reads its _First at +0xc8 and _Last at +0xcc;
// those pointer offsets must not be mistaken for the vector's own offset.
// Unresolved scalar groups retain offset-based names until their consumers
// establish their roles. Other field names are provisional retail roles.
struct TRmgTownSlot {
    // Before normalization: zoneIndex.
    int m_zoneIndex;                    // +0x00
    // Before normalization: kind.
    int m_kind;                         // +0x04: ERmgTemplateZoneKind
    // Before normalization: size.
    int m_size;                         // +0x08
    // Before normalization: minimumHumanPlayers.
    int m_minimumHumanPlayers;          // +0x0c
    // Before normalization: maximumHumanPlayers.
    int m_maximumHumanPlayers;          // +0x10
    // Before normalization: minimumPlayers.
    int m_minimumPlayers;               // +0x14
    // Before normalization: maximumPlayers.
    int m_maximumPlayers;               // +0x18
    // Before normalization: playerIndex.
    int m_playerIndex;                  // +0x1c
    // Before normalization: parameters0020.
    int m_parameters0020[8];
    // Before normalization: flag0040.
    unsigned char m_flag0040;
    // Before normalization: allowedTowns.
    unsigned char m_allowedTowns[9];    // +0x41
    // Before normalization: parameters004c.
    int m_parameters004c[7];
    // Before normalization: parameters0068.
    int m_parameters0068[7];
    // Before normalization: flag0084.
    unsigned char m_flag0084;
    // Before normalization: allowedTerrain.
    unsigned char m_allowedTerrain[8];  // +0x85
    // Before normalization: monsterStrength.
    int m_monsterStrength;              // +0x90
    // Before normalization: flag0094.
    unsigned char m_flag0094;
    // Before normalization: allowedMonsters.
    unsigned char m_allowedMonsters[10]; // +0x95
    // Before normalization: treasure.
    TRmgTreasureRange m_treasure[3];     // +0xa0
    // Before normalization: connections.
    std::vector<TRmgZoneConnection> m_connections; // +0xc4

    TRmgZoneConnection* findConnection(int destinationZone);
};
SIZE(TRmgTownSlot, 0xd4);

// The rmg.txt coordinator allocates this 0x38-byte object, assigns its
// name and size limits, and passes it to the zone reader in edx.
struct TRmgTemplate {
    // Before normalization: name.
    std::string m_name;                  // +0x00
    // Before normalization: zones.
    std::vector<TRmgTownSlot*> m_zones;   // +0x10
    // Before normalization: opaque0020.
    char m_opaque0020[0x10];
    // Before normalization: minimumSize.
    int m_minimumSize;                  // +0x30
    // Before normalization: maximumSize.
    int m_maximumSize;                  // +0x34

    ~TRmgTemplate();
    // Before normalization (function): TRmgTemplate::FindZone.
    TRmgTownSlot* findZone(int zoneIndex);
};
SIZE(TRmgTemplate, 0x38);

// Before normalization (function): ReadRmgTemplateZones.
void readRmgTemplateZones(
    const TSpreadsheetResource* sheet, TRmgTemplate* mapTemplate,
    int firstRow, int endRow, int humanPlayers, int computerPlayers,
    int mapVersion);

// Retained fastcall helper at 0x545e00, also expanded by zone connections.
int getRmgGuardValue(int value, int strength);

// Voronoi's circumcenter arithmetic separates displacement vectors from
// positions: vector+vector is a member call, point+vector and point-point
// are free calls. All carry two signed dwords; names remain provisional.
struct TRmgVector {
    // Before normalization: x.
    int m_x;
    // Before normalization: y.
    int m_y;

    TRmgVector() {}
    TRmgVector(int newX, int newY) : m_x(newX), m_y(newY) {}

    int length() const;
    TRmgVector operator+(TRmgVector other) const;
    TRmgVector operator*(int scale) const;
    TRmgVector operator/(int divisor) const;
};

// Retail's common direction table contains eight consecutive two-dword
// offsets.  Its cinit at 0x530da0 proves the user-provided constructor while
// the absence of an atexit registration proves that destruction is trivial.
// The comparator is independently used by the RMG set cluster.
struct TPoint {
    // Before normalization: x.
    int m_x;
    // Before normalization: y.
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
};

// The retained 0x5fdd20/0x5fdd40 bodies pass both eight-byte operands on
// the stack and return a pair through ECX. BuildVertices uses subtraction
// to form a displacement and addition to translate the origin point.
// These are free operations; the vector sum above is a member operation.
TPoint operator+(TPoint point, TRmgVector offset);
TRmgVector operator-(TPoint left, TPoint right);

// The map-painting grid uses unsigned coordinates: the terrain set's lower
// bound at 0x5b8a40 compares y, then x, with jb/jae. Its retained constructor
// at 0x5b76b0 reads both arguments through pointers. This role name is
// provisional; the signed geometry TPoint is a separate recovered surface.
struct TRmgGridPoint {
    // Before normalization: x.
    unsigned int m_x;
    // Before normalization: y.
    unsigned int m_y;

    TRmgGridPoint() {}
    // The retained river-painter ctor at 0x55ee50 copies both GetSize result
    // components before storing its adapter. This copy boundary restores all
    // 118 bytes; an implicit copy interleaves adapter and y stores (99.71%).
    // Moving the adapter into the caller ctor body instead stores its vptr
    // too early (99.10%); a copy assignment does not affect construction.
    TRmgGridPoint(const TRmgGridPoint& other)
        : m_x(other.m_x), m_y(other.m_y) {}

    TRmgGridPoint(const unsigned int& newX, const unsigned int& newY)
        : m_x(newX), m_y(newY) {}

    TRmgGridPoint& operator+=(const TPoint& offset)
    {
        m_x += offset.m_x;
        m_y += offset.m_y;
        return *this;
    }
    TRmgGridPoint operator+(const TPoint& offset) const
    {
        // Copy-initialize the coordinate value, then return the compound
        // translation. Retail paintPoint 0x5b4e38..0x5b4e55 retains original x
        // at EBP-0x14 before the additions. With its guard-return terrain
        // predicate, a separate named return changes the direction register
        // and addition schedule (99.0163% versus 99.9204%). The older named-
        // return result depended on the comparison-return predicate.
        TRmgGridPoint result = TRmgGridPoint(m_x, m_y);
        return result += offset;
    }
};

// The retained comparison at 0x5b8ca0 receives both point addresses in
// ECX/EDX and returns without popping arguments: a free fastcall boundary.
bool operator<(const TRmgGridPoint& left, const TRmgGridPoint& right);

struct TRmgZoneBounds {
    // Before normalization: minimumX.
    int m_minimumX;
    // Before normalization: minimumY.
    int m_minimumY;
    // Before normalization: maximumX.
    int m_maximumX;
    // Before normalization: maximumY.
    int m_maximumY;

    // Before normalization (function): TRmgZoneBounds::Contains.
    bool contains(const TPoint& point) const
    {
        return point.m_x >= m_minimumX && point.m_x < m_maximumX &&
            point.m_y >= m_minimumY && point.m_y < m_maximumY;
    }
};

// Before normalization (function): ClipRmgBoundaryPoint.
TPoint clipRmgBoundaryPoint(
    const TRmgZoneBounds& bounds, TPoint point, TPoint toward);

enum ERmgConnectionConstants {
    RMG_SHIPYARD_WATER_OFFSET_COUNT = 4,
    RMG_WATER_NONE = 0,
    RMG_WATER_ISLANDS = 2
};

// The function-local river-delta table has a non-trivial empty destructor:
// retail registers its cleanup thunk when CreateRiver first reaches the
// table.  The type is shared here so the table has one canonical shape.
struct TRmgRiverDeltaOffset {
    // Before normalization: x.
    int m_x;
    // Before normalization: y.
    int m_y;

    TRmgRiverDeltaOffset(int newX, int newY) : m_x(newX), m_y(newY) {}
    ~TRmgRiverDeltaOffset() {}
};

class type_object;

struct TRmgMovementCost {
    // Before normalization: cost.
    unsigned m_cost : 16;
    // Role-derived; original name unknown. Before normalization: unknown.
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
    // Before normalization: score.
    unsigned m_score : 16;
    // Before normalization: zone.
    signed m_zone : 8;
    // Before normalization: connectionEligibility.
    signed m_connectionEligibility : 8;
};

// The six-bit signed land field is fixed by retail's `shl 26; sar 26`
// extraction in the river-delta path.  The four-bit field at bit 26 is
// tested as a unit when river routing prices an already decorated tile.
struct TRmgGroundTile {
    // Before normalization: landType.
    TTerrainType m_landType : 6;
    // Retail terrain adapter 0x532190 stores an eight-bit frame at bit 6;
    // getter 0x532288 sign-extends it. River adapter 0x532520 writes the
    // four-bit type at 14 and eight-bit frame at 18; 0x5327c0 sign-extends
    // both. Role-derived names; original spellings unknown.
    // Replaces unknown06.
    signed m_terrainFrame : 8;
    signed m_riverType : 4;
    signed m_riverFrame : 8;
    // Before normalization: decorationType. Road adapter 0x532360 writes
    // this at bit 26; getter 0x532447..0x532450 sign-extends four bits.
    signed m_roadType : 4;
    // Before normalization: unknown30.
    unsigned m_unknown30 : 2;
};

struct TRmgGroundTileData {
    // Road adapter 0x532360 writes all eight low bits; 0x53244a/0x532453
    // sign-extends the frame. Replaces roadSprite and synthetic unknown07.
    signed m_roadFrame : 8;
    // Before normalization: blockedDirections.
    unsigned m_blockedDirections : 4;
    // Before normalization: connectionDirection.
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
    // Retail cell writer 0x532972 maps this bit to flag 0x40 in the
    // seventh H3M cell byte. Reader 0x4fe220 uses that flag to create
    // non-water ANCHOR_POINT cells: this is the coastal marker.
    // Role-derived name; completes recovery of former unknown15.
    unsigned m_coastal : 1;
    // BuildRoadCostMap proves these two Complete-only routing flags at bits
    // 22 and 25.  The first marks an object entrance whose adventure-object
    // traits constrain approach directions; the second admits the tile to
    // the road-cost flood.
    // Before normalization: roadEntrance.
    unsigned m_roadEntrance : 1;
    // Role-derived; original name unknown. Before normalization: unknown23.
    // 0x535ee0 traces a closed placement perimeter into the point vector
    // at +0x38 (append 0x535fb2, closure 0x536051..0x536062). Accepted
    // placement 0x5468e8 calls it, then marks these points with bit 23
    // at 0x546923; 0x54b6df and 0x54bad5 mark the same outline in the
    // quest placement paths. Checker 0x546ed5 requires the marked bit.
    unsigned m_placementOutline : 1;
    // Before normalization: connectionVisited.
    unsigned m_connectionVisited : 1;
    // Before normalization: roadPassable.
    unsigned m_roadPassable : 1;
    // Before normalization: borderObject.
    unsigned m_borderObject : 1;
    // Before normalization: subterraneanGate.
    unsigned m_subterraneanGate : 1;
    // Before normalization: zoneBoundary.
    unsigned m_zoneBoundary : 1;
    // Before normalization: roadTarget.
    unsigned m_roadTarget : 1;
    // Before normalization: riverTarget.
    unsigned m_riverTarget : 1;
    // Before normalization: impassable.
    unsigned m_impassable : 1;
};

struct TRmgConnectionDecoration {
    // Before normalization: present.
    unsigned m_present : 1;
    // Before normalization: direction.
    unsigned m_direction : 4;
    // Before normalization: unknown05.
    unsigned m_unknown05 : 27;
};

// The rand_trn.txt reader appends 0x4c-byte rows. ScoreObjectPlacement
// consumes the ten terrain values and the two vectors indexed by rule id.
// These are Complete-only role names; no Dreamcast RMG records survive.
struct TRmgObjectPlacementRule {
    // Before normalization: index.
    int m_index;                         // +0x00
    // Before normalization: terrainScores.
    int m_terrainScores[10];             // +0x04
    // Before normalization: adjacentScores.
    std::vector<int> m_adjacentScores;    // +0x2c
    // Before normalization: blockedScores.
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
    // Before normalization: prototype.
    TObjectType* m_prototype;              // +0x00
    // Before normalization: preferredTerrain.
    // Previously unknown04; retail 0x536560 binds the first recommended terrain.
    int m_preferredTerrain;               // +0x04, rand_trn.txt rule binding
    // Before normalization: refCount.
    unsigned m_refCount;                   // +0x08
    // Before normalization: prototypeIndex.
    int m_prototypeIndex;
    // Previously opaque0010: ctor 0x532c80 owns outline; 0x532e40
    // initializes overlapPriorities and the +0xe4 flag. +0xe5..e7 aligns the tail.
    TRmgObjectPlacementRule* m_placementRule; // +0x10
    std::vector<TPoint> m_outline;         // +0x14
    int m_overlapPriorities[8][6];         // +0x24
    // Before normalization: prioritiesInitialized.
    unsigned char m_prioritiesInitialized; // +0xe4
    // Before normalization: pad00e5.
    char m_pad00e5[3];

    void buildOverlapPriorities();
};

class type_object {
public:
    // Before normalization: properties.
    TRmgObjectPropertiesRef* m_properties; // +0x04
    // Before normalization: position.
    TRmgMapPosition m_position;             // +0x08
    // Previously unknown14; placement scorer 0x536bc0 marks this relation.
    unsigned char m_candidateCovers;
    // Previously unknown15; placement scorer 0x536bc0 marks this relation.
    unsigned char m_candidateBehind;
    // Previously unknown16; placement scorer 0x536bc0 marks this relation.
    unsigned char m_adjacentToCandidate;
    // Previously unknown17; placement scorer 0x536bc0 marks this relation.
    unsigned char m_overlapsCandidate;
    // Before normalization: blockedByCandidate.
    // Previously unknown18; placement scorer 0x536bc0 marks this relation.
    unsigned char m_blockedByCandidate;
    // Before normalization: tailPadding.
    char m_tailPadding[3];

    // Retail retains this body at 0x5330e0 beneath derived construction.
    // No Dreamcast inline declaration exists for this Complete-only type.
    type_object(TRmgObjectPropertiesRef* newProperties);
    TRmgMapPosition getPosition() const;

    // The constructor and placement scorer share this five-byte reset.
    // The method name is provisional; retail preserves the store order.
    void clearPlacementMarks();

    unsigned char isPlacementTouched() const
    {
        return m_adjacentToCandidate || m_blockedByCandidate || m_overlapsCandidate;
    }

    virtual ~type_object();
    // Before normalization (function): type_object::UnknownOperation.
    virtual void unknownOperation();
    // Before normalization (function): type_object::IsWritable.
    virtual unsigned char isWritable() const;
    // Before normalization (function): type_object::Write.
    // Retail base/ownable writers 0x533170/0x533460 both pop eight bytes.
    // The second stack word is unused there; its source role is unresolved.
    virtual void write(TAbstractFile* outfile, int parameter);
};

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

struct TRmgMapItem {
    // Before normalization: objects.
    std::vector<type_object*> m_objects;    // +0x00
    // Before normalization: previousTile.
    TRmgMapPosition m_previousTile;         // +0x10
    // Before normalization: movement.
    TRmgMovementCost m_movement;            // +0x1c
    // Before normalization: zoneState.
    TRmgZoneCellState m_zoneState;           // +0x20
    // Before normalization: tile.
    TRmgGroundTile m_tile;                  // +0x24
    // Before normalization: tileData.
    TRmgGroundTileData m_tileData;          // +0x28
    // Before normalization: connection.
    TRmgConnectionDecoration m_connection;  // +0x2c

    // CreateRiver's predicate reads shift the high tile bits and test a
    // byte result. These queries recover that boundary; direct field tests
    // instead use dword masks. Names remain provisional without RMG symbols.
    bool isRiverTarget() const { return m_tileData.m_riverTarget != 0; }
    bool isImpassable() const { return m_tileData.m_impassable != 0; }

    // ScoreObjectPlacement reads bit 27 with shr/test dl, whereas its
    // direct roadPassable condition tests the containing dword. The
    // provisional byte accessor reproduces that truncation; a direct
    // bitfield condition instead folds to test dword ptr [item+0x28],imm.
    unsigned char hasSubterraneanGate() const
    {
        return m_tileData.m_subterraneanGate;
    }

    // RepairWaterZoneBorders tests this flag after truncating it to a byte
    // at 0x53fe30, then tests roadPassable directly as a dword bit.
    unsigned char hasBorderObject() const
    {
        return m_tileData.m_borderObject;
    }

    // Retail road/river relaxation copies the predecessor to a separate
    // parameter home before storing cost and coordinates. The by-value
    // boundary is inferred from those repeated x86 copies; the name is
    // provisional because Dreamcast contains no RMG compiland. Keeping
    // the scalar cost write and struct assignment directly in each caller
    // loses that snapshot (BuildRoadCostMap 73.7139% versus 75.6686%).
    // Before normalization (function): TRmgMapItem::SetMovementCost.
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
    // Before normalization (function): TRmgMapItem::ResetMovement.
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
    virtual ~TRmgMapInterface() {}
    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile) = 0;
    virtual void setOverlay(const TRmgGridPoint& point, int value) = 0;
    virtual TRmgGridPoint getSize() = 0;
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

class type_random_map : public TRmgMapInterface {
public:
    // Before normalization: ownsMapItems.
    unsigned char m_ownsMapItems;           // +0x04
    // Before normalization: pad0005.
    // The ownership flag is a byte at +4 after the vptr, and
    // mapItems starts at +8. These three bytes align the pointer.
    char m_paddingBeforeMapItems[3];
    // Before normalization: mapItems.
    TRmgMapItem* m_mapItems;                // +0x08
    // Before normalization: mapWidth.
    int m_mapWidth;                         // +0x0c
    // Before normalization: mapHeight.
    int m_mapHeight;                        // +0x10
    // Before normalization: numberLevels.
    int m_numberLevels;                     // +0x14

    // The buffer-first view signature preserves the dimension values before
    // GetMapItem computes the plane pointer. In RepairWaterZoneBorders the
    // constructor/painting range 0x540124..0x54020c matches all 232 bytes after
    // relocation resolution and segment placement. The role is retail-only.
    // Controls: dimensions-first scalar arguments reload fields; a map/level
    // pair stores width early; a separate plane local keeps the wrong multiply
    // operand. Field assignments stay in the body: an all-member initializer
    // list moves the vptr store past them. Other view callers remain partial.
    inline type_random_map(TRmgMapItem* items, int width, int height)
    {
        m_mapWidth = width;
        m_mapHeight = height;
        m_mapItems = items;
        m_numberLevels = 1;
        m_ownsMapItems = 0;
    }

    virtual ~type_random_map()
    {
        if (m_ownsMapItems)
            delete[] m_mapItems;
    }

    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile);
    virtual void setOverlay(const TRmgGridPoint& point, int value);
    virtual TRmgGridPoint getSize();
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point);
    virtual int getLand(const TRmgGridPoint& point);
    virtual int getOverlay(const TRmgGridPoint& point);

    TRmgMapItem* getMapItem(int x, int y);
    // Before normalization (function): type_random_map::GetMapItem.
    inline TRmgMapItem* getMapItem(int x, int y, int z)
    {
        return m_mapItems + (z * m_mapHeight + y) * m_mapWidth + x;
    }
    // Before normalization (function): type_random_map::GetMapItem.
    TRmgMapItem* getMapItem(TRmgMapPosition point);

    // Before normalization (function): type_random_map::CanPlaceObject.
    unsigned char canPlaceObject(
        TRmgObjectPropertiesRef* properties,
        TRmgMapPosition position,
        TRmgZone* zone);
};

// Retail retains these support bodies outside CreateRiver while the adapter
// and map-view construction remains expanded at the call site.  Keeping the
// class definitions shared but the retained bodies in rmg_support.cpp
// reproduces that ordinary translation-unit visibility boundary.
class TRmgMapAdapter : public TRmgMapAdapterInterface {
public:
    // Before normalization: map.
    type_random_map* m_map;

    inline TRmgMapAdapter(type_random_map* newMap) : m_map(newMap) {}

    // Before normalization (function): TRmgMapAdapter::SetTile.
    virtual void setTile(
        const TRmgGridPoint& point, const rmgTerrainTile& tile);
    virtual void setOverlay(const TRmgGridPoint& point, int value);
    virtual TRmgGridPoint getSize();
    virtual rmgTerrainTile getTile(const TRmgGridPoint& point);
    virtual int getLand(const TRmgGridPoint& point);
    virtual int getOverlay(const TRmgGridPoint& point);
};

class TRmgLinePainter {
public:
    // Before normalization: size.
    TRmgGridPoint m_size;
    // Before normalization: adapter.
    TRmgMapAdapterInterface* m_adapter;

    inline TRmgLinePainter(TRmgMapAdapterInterface* newAdapter)
        : m_size(newAdapter->getSize()), m_adapter(newAdapter)
    {
    }
    ~TRmgLinePainter() {}

    // Before normalization (function): TRmgLinePainter::GetPattern.
    virtual void* getPattern(int value);
    // Before normalization (function): TRmgLinePainter::PaintTile.
    virtual void paintTile(int value, const TRmgMapPosition& tile);
    // Before normalization (function): TRmgLinePainter::PaintOverlay.
    virtual void paintOverlay(int value, const TRmgMapPosition& tile);
    virtual int canPaint(const TRmgGridPoint& point);
    virtual void paintNeighbour(int value, const TRmgMapPosition& tile);
    virtual int paintPoint(const TRmgGridPoint& point);
};

class TRmgLineWalker {
public:
    // Before normalization: painter.
    TRmgLinePainter* m_painter;
    // Before normalization: riverType.
    int m_riverType;
    TRmgGridPoint m_position;

    TRmgLineWalker(
        TRmgLinePainter* newPainter,
        int newRiverType,
        const TRmgGridPoint& start);
    void drawTo(const TRmgGridPoint& destination);
};

class TRmgRiverPainter : public TRmgLinePainter, public TRmgLineWalker {
public:
    TRmgRiverPainter(
        TRmgMapAdapterInterface* newAdapter,
        int newRiverType,
        const TRmgGridPoint& start);
    virtual ~TRmgRiverPainter();
};

// A generated zone owns both its template metadata and the Complete-only
// connection state.  WriteMapHeader proves the player/town fields through
// +0x3c; the connection pass independently proves the bounding rectangle and
// entrance vector at +0x404.  The 0x1c-stride connection vector belongs to
// the template record reached through `slot`, not to this generated zone.
struct TRmgZone {
    // Before normalization: slot.
    TRmgTownSlot* m_slot;              // +0x00
    // Before normalization: alignment.
    int m_alignment;                   // +0x04
    // H3API H3RmgZoneGenerator::townType2, INT32 at +08, commit
    // 92255ab18da784a5842ecc2b8bc0ce00e19a0c56. The surrounding town/terrain,
    // coordinates, object-count array and three vectors match this layout.
    // Reference-backed spelling/type; no retail semantic consumer located.
    // Before normalization: opaque0008.
    int m_townType2;
    // Before normalization: terrain.
    int m_terrain;                     // +0x0c
    // Before normalization: levelPosition.
    TRmgMapPosition m_levelPosition;   // +0x10
    // Before normalization: boundaryRoughness.
    int m_boundaryRoughness;            // +0x1c: minimum of adjacent zones
    // Before normalization: bounds.
    TRmgZoneBounds m_bounds;           // +0x20
    // Before normalization: position.
    TRmgMapPosition m_position;        // +0x30: main town
    // Before normalization: active.
    unsigned char m_active;            // +0x3c
    // Before normalization: opaque003d. The +0x40 word remains unresolved;
    // retain its surrounding storage rather than calling it padding.
    char m_opaque003d[7];              // +0x3d..+0x43
    // Retail ctor 0x5329e0 clears 232 dwords beginning at +0x44.
    // Placement 0x54039a increments by object type, removal 0x54bd30
    // decrements it, and 0x546270 checks the per-zone object-type limit.
    // Role-derived name, matching the generator's global counterpart.
    // Before normalization: objectCountByType.
    int m_objectCountByType[232];      // +0x44
    // Retail +0x3e4 has vector construction/destruction. 0x53dc84 resizes
    // to the zone count; +0x53dc98 fills signed shorts with 32000 and
    // +0x53dcb1 sets this zone's own index to zero. 0x53d9ae reads a
    // distance, adds one and relaxes connected zones. Role-derived name.
    // Before normalization: zoneDistances.
    std::vector<short> m_zoneDistances;// +0x3e4
    // Before normalization: boundary.
    std::vector<TPoint> m_boundary;    // +0x3f4: clipped polygon vertices
    // Before normalization: entrances.
    std::vector<TPoint> m_entrances;   // +0x404

    // The candidate-placement filter consumes returned coordinate values;
    // its retained connection predicate compares center distance and size.
    // These names are provisional; the Dreamcast build has no RMG module.
    TRmgZone(TRmgTownSlot* slot);
    ~TRmgZone();
    TRmgMapPosition getLevelPosition() const;
    void setLevelPosition(TRmgMapPosition position);
    unsigned char canConnect(const TRmgZone* other) const;
};

// Partial Voronoi topology recovered from TraceZoneBoundary and its caller
// at 0x53e050. The twin's owning zone identifies the region across an edge;
// following next traverses a closed polygon. Names are provisional.
struct TRmgBoundaryVertex {
    // Previously opaque0000. The paired-edge constructor 0x5fcef0
    // copies a by-value point into +0/+4 and its zone into +8.
    // buildVertices 0x5fdb40 subtracts these site coordinates while
    // calculating the boundary point at +0x1c. Role-derived name.
    TPoint m_sitePosition;               // +0x00
    // Before normalization: zone.
    TRmgZone* m_zone;                   // +0x08
    // Before normalization: twin.
    TRmgBoundaryVertex* m_twin;        // +0x0c
    // Before normalization: next.
    TRmgBoundaryVertex* m_next;        // +0x10
    // Previously opaque0014. Constructor 0x5fcef0 initializes both
    // ring links to self; splice 0x5fcf60 swaps next->previous together
    // with next, preserving the backward link. Role-derived name.
    TRmgBoundaryVertex* m_previous;     // +0x14
    // Constructor clears this byte. buildVertices tests it at 0x5fdb7a,
    // writes the computed point, then sets it on three incident edges at
    // 0x5fdc7d/89/9e. +0x19..1b is natural alignment before the point.
    unsigned char m_positionComputed;  // +0x18
    // Before normalization: position.
    TPoint m_position;                  // +0x1c
};
SIZE(TRmgBoundaryVertex, 0x24);

// The retained subdivision constructor and destructor own a root edge and
// a vector of allocated edges. The coordinator inserts zone sites, computes
// dual vertices, then looks up an edge for each site. All names are provisional.
class TRmgVoronoi {
public:
    // Before normalization: root.
    TRmgBoundaryVertex* m_root;                 // +0x00
    // Before normalization: edges.
    std::vector<TRmgBoundaryVertex*> m_edges;  // +0x04

    TRmgVoronoi();
    ~TRmgVoronoi();
    void addSite(TPoint point, TRmgZone* zone);
    TRmgBoundaryVertex* locate(TPoint point);
    void buildVertices();
};
SIZE(TRmgVoronoi, 0x14);

// The RMG progress sink is used through its third vtable slot by the zone
// connection coordinator.  No concrete implementation is owned by rmg.cpp.
class TRmgProgress {
public:
    // Before normalization (function): TRmgProgress::UnknownProgressOperation0.
    virtual void unknownProgressOperation0() = 0;
    // Before normalization (function): TRmgProgress::UnknownProgressOperation1.
    virtual void unknownProgressOperation1() = 0;
    // Before normalization (function): TRmgProgress::Advance.
    virtual void advance(int amount) = 0;
};

SIZE(TRmgMapPosition, 0xc);
SIZE(TRmgZoneConnection, 0x1c);
SIZE(TRmgZoneBounds, 0x10);
SIZE(TRmgZone, 0x414);

enum ERmgMapVersion {
    RMG_MAP_RESTORATION_OF_ERATHIA = 0,
    RMG_MAP_ARMAGEDDONS_BLADE = 1,
    RMG_MAP_SHADOW_OF_DEATH = 2
};

// The Complete-only map-header writer extends the object-factory evidence
// into the late generator state.  Each named field below is read or written
// at its annotated offset by retail 0x549cb0; opaque spans preserve all
// unobserved state without guessing at its source identity.
class type_random_map_generator {
public:
    // Before normalization: randomSeed.
    int m_randomSeed;                                  // +0x004
    // Before normalization: mapVersion.
    int m_mapVersion;                                  // +0x008
    // Before normalization: map.
    type_random_map m_map;                             // +0x00c
    std::vector<TObjectType> m_objectsTxt;             // +0x024
    std::vector<TRmgObjectPropertiesRef*> m_objectPrototypes[232]; // +0x034
    // Before normalization: placementRules.
    // Previously unknownPointers/randomTerrainEntries; loader 0x536560 and
    // scorer 0x536bc0 prove value records and the terrain/relation semantics.
    std::vector<TRmgObjectPlacementRule> m_placementRules; // +0xeb4
    // Before normalization: positions.
    std::vector<type_object*> m_positions;             // +0xec4
    // Before normalization: progress.
    TRmgProgress* m_progress;                          // +0xed4
    // Before normalization: fixedHumanPlayers.
    unsigned char m_fixedHumanPlayers[8];              // +0x0ed8
    // Retail 0x5499fb initializes nine ints beginning here to -1;
    // 0x549a75/0x549ab8 populate entries 1..8. Readers use +0xee4 with
    // zero-based indexes. This is the mapping's sentinel entry, but the
    // full mapping extent and storage through +0xf23 remain unresolved.
    // Before normalization: opaque0ee0.
    char m_opaque0ee0[0x4];
    // Before normalization: playerIndexMap.
    int m_playerIndexMap[16];                          // +0x0ee4
    // Before normalization: townChoices.
    int m_townChoices[8];                              // +0x0f24
    // Role-derived; original name unknown. Constructor 0x537b10 seeds
    // +0xf44 to 1. Creation paths 0x534902, 0x540cfa, 0x545104 and
    // 0x54543d take then increment the counter, storing the taken ID in
    // the new object's derived data (+0x20 for the first, +0x1c for others).
    // Before normalization: opaque0f44.
    int m_nextObjectId;                               // +0x0f44
    // Before normalization: humanPlayerCount.
    int m_humanPlayerCount;                            // +0x0f48
    // Before normalization: humanTeamCount.
    int m_humanTeamCount;                              // +0x0f4c
    // Before normalization: computerPlayerCount.
    int m_computerPlayerCount;                         // +0x0f50
    // Before normalization: computerTeamCount.
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
    // Before normalization: disabledHeroes.
    unsigned char m_disabledHeroes[156];               // +0x0f88
    // Role-derived names; original spellings unknown. Replaces opaque1024.
    // Ctor 0x537cc6 clears 144 bytes. Quest selection 0x54b490 excludes
    // marked artifacts; successful placement 0x54b813 marks the chosen ID.
    unsigned char m_usedQuestArtifacts[144];           // +0x1024
    // 0x54b4f1 latches this when fewer than 20 eligible artifacts remain;
    // seer-hut value paths 0x534b0c/0x534c9c reject further candidates.
    unsigned char m_questArtifactPoolLow;              // +0x10b4
    // +0x10b5..0x10b7: implicit alignment before the next int.
    // Before normalization: waterContent.
    int m_waterContent;                                // +0x10b8
    // Before normalization: monsterStrength.
    int m_monsterStrength;                             // +0x10bc
    // Retail ctor 0x537b10 initializes a Dinkumware string at +0x10c0.
    // 0x54999c calls basic_string::assign with the selected template's
    // leading name string; 0x537fcc destroys it with basic_string::_Tidy.
    // The old templateName pointer at +0x10c4 was only its buffer member.
    // Replaces synthetic opaque10c0 and the first half of opaque10c8.
    // Before normalization: templateName.
    std::string m_templateName;                        // +0x10c0
    // Retail 0x538450 inserts TRmgTemplate pointers into this vector;
    // 0x537e84 destroys its elements with TRmgTemplate::~TRmgTemplate,
    // and 0x537fbc destroys the vector itself. Role-derived name.
    // Replaces the remaining half of synthetic opaque10c8.
    // Before normalization: templates.
    std::vector<TRmgTemplate*> m_templates;            // +0x10d0
    // Before normalization: zones.
    std::vector<TRmgZone*> m_zones;                    // +0x10e0
    // Before normalization: objectGenerators.
    std::vector<type_treasure_def*> m_objectGenerators; // +0x10f0
    // Before normalization: disabledKeyTents.
    std::vector<unsigned char> m_disabledKeyTents;     // +0x1100
    // Before normalization: objectCountByType.
    int m_objectCountByType[232];                      // +0x1110
    // Before normalization: roadTargets.
    std::vector<TRmgMapPosition> m_roadTargets;        // +0x14b0
    // Before normalization: monolithsOneWay.
    std::vector<type_object*> m_monolithsOneWay;       // +0x14c0
    // Before normalization: monolithsTwoWay.
    std::vector<type_object*> m_monolithsTwoWay;       // +0x14d0

    virtual ~type_random_map_generator();
    // Before normalization (function): type_random_map_generator::AddObject.
    virtual void addObject(type_object* object, TRmgMapPosition position);

    // Before normalization (function): type_random_map_generator::GetSerializedMapVersion.
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

    // Before normalization (function): type_random_map_generator::InitializeObjectGenerators.
    void initializeObjectGenerators();
    void readObjectPlacementRules();
    int scoreObjectPlacement(
        TRmgObjectPropertiesRef* properties, TRmgMapPosition position);
    unsigned char canPlaceZone(TRmgZone* zone);
    void buildZoneBoundaries(TRmgTemplate* mapTemplate, int level);
    void fillZoneArea(TRmgZone* zone, TRmgBoundaryVertex* first);
    void joinExtraZones(int originalZones, TRmgVoronoi* diagram);
    int countPlacedZoneConnections(TRmgZone* zone) const;
    void filterZonePositions(
        TRmgZone* zone, std::vector<TRmgMapPosition>& candidates, int mapSize);
    void drawIrregularZoneBoundary(
        TPoint from, TPoint to, int zoneIndex, int level, int roughness);
    // Before normalization (function): type_random_map_generator::DrawStraightZoneBoundary.
    void drawStraightZoneBoundary(
        TPoint from, TPoint to, int zoneIndex, int level);
    // Before normalization (function): type_random_map_generator::TraceZoneBoundary.
    void traceZoneBoundary(TRmgBoundaryVertex* first, unsigned char irregular);
    // Before normalization (function): type_random_map_generator::CreateGroundConnection.
    unsigned char createGroundConnection(
        TRmgZone* source,
        TRmgZoneConnection* connection,
        std::vector<TRmgMapItem*>* borderItems,
        std::vector<TRmgMapPosition>* borderPositions);
    // Before normalization (function): type_random_map_generator::FloodConnectionRegion.
    void floodConnectionRegion(TRmgMapPosition position);
    // Earlier provisional name: CreateBorderConnection/createBorderConnection.
    // Retail 0x541ad0 selects objectPrototypes[SHIPYARD] and places it beside
    // reachable water. No Dreamcast RMG name is available.
    unsigned char createShipyardConnection(
        TRmgZone* source, TRmgZoneConnection* connection);
    unsigned char canPlaceShipyard(TRmgMapPosition position);
    // Before normalization (function): type_random_map_generator::CreateSubterraneanGate.
    unsigned char createSubterraneanGate(
        TRmgZone* source, TRmgZoneConnection* connection);
    // Before normalization (function): type_random_map_generator::CreateMonolithConnection.
    void createMonolithConnection(
        TRmgZone* source,
        TRmgZoneConnection* connection,
        int prototypeIndex);
    // Before normalization (function): type_random_map_generator::ConnectZones.
    void connectZones();
    void repairWaterZoneBorders();
    // Complete-only roles proved by the predecessor walk at 0x5408e0 and
    // the surrounding connection-cell updates at 0x540fc0.
    void openConnectionPath(TRmgMapPosition position, unsigned char narrow);
    void markBorderObjectArea(TRmgMapPosition position, int direction);
    int placeBorderObject(
        TRmgMapPosition position, int count, TRmgZone* zone);
    // Before normalization (function): type_random_map_generator::CreateGuard.
    type_object* createGuard(int value, TRmgZone* zone);
    unsigned char placeObjectInZone(type_object* object, TRmgZone* zone);
    void placeGuard(TRmgMapPosition position, int value);
    void resetMovementCosts();
    // Provisional Complete-only spelling: the 0x548290 road-target pass is
    // the sole direct caller, and the body builds the road traversal costs.
    // Before normalization (function): type_random_map_generator::BuildRoadCostMap.
    void buildRoadCostMap(TRmgMapPosition position);
    // Provisional spelling: retail's water-wheel caller and the river-delta
    // object selection prove the role; the Dreamcast build has no RMG TU.
    // Before normalization (function): type_random_map_generator::CreateRiver.
    void createRiver(TRmgMapPosition source);
    // Before normalization (function): type_random_map_generator::WriteMapHeader.
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
SIZE(TRmgMapAdapter, 0x08);
SIZE(TRmgLinePainter, 0x10);
SIZE(TRmgLineWalker, 0x10);
SIZE(TRmgRiverPainter, 0x20);
SIZE(type_random_map_generator, 0x14e0);

// Retail 0x6824e0 is indexed by the creature-traits level dword before
// type_black_box_creature_def divides by that creature's AI value.
// Before normalization: gRmgCreatureValueByLevel.
DATA(0x006824E0) extern int g_rmgCreatureValueByLevel[];

#endif  // HOMM3_RMG_H
