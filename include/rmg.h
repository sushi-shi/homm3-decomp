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
struct TRmgTerrainTile;
struct TPoint;
struct TObjectType;

// Complete's random-map object factories share this five-dword prefix.  The
// constructor at 0x534160 writes the four fields, while vtable 0x640b64 proves
// three virtual operations: an object factory taking three arguments, a
// two-argument value query, and a parameterless boolean property.  The method
// names remain role descriptions until retail-era source identifies their
// original spelling; their boundaries and arities are retail-byte facts.
class type_treasure_def {
public:
    int objectType;
    int subtype;
    int value;
    int density;

    type_treasure_def(int objectType, int subtype, int value, int density);

    virtual void* Generate(void* owner, int x, int y);
    virtual int GetValue(void* object, void* map);
    virtual unsigned char IsTerrainDependent();
};

SIZE(type_treasure_def, 0x14);

// These identities come from the contiguous cross-build vtable roster.  The
// current-image constructor relocations independently fix each table address.
class type_shrine_def : public type_treasure_def {
public:
    type_shrine_def(int objectType, int value);
    virtual void* Generate(void* owner, int x, int y);
};

class type_witch_hut_def : public type_treasure_def {
public:
    type_witch_hut_def();
    virtual void* Generate(void* owner, int x, int y);
};

class type_spell_scroll_def : public type_treasure_def {
public:
    int spellLevel;

    type_spell_scroll_def(int spellLevel, int value);
    virtual void* Generate(void* owner, int x, int y);
};

class type_black_box_creature_def : public type_treasure_def {
public:
    int creatureType;
    int adjustedValue;

    type_black_box_creature_def(int creatureType);
    virtual void* Generate(void* owner, int x, int y);
    virtual int GetValue(void* object, void* map);
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

    virtual void* Generate(void* owner, int x, int y);
};

class type_black_box_experience_def : public type_treasure_def {
public:
    int experience;

    inline type_black_box_experience_def(int value, int experience)
        : type_treasure_def(6, 0, value, 20)
    {
        this->experience = experience;
    }

    virtual void* Generate(void* owner, int x, int y);
};

class type_black_box_gold_def : public type_treasure_def {
public:
    int gold;

    inline type_black_box_gold_def(int value, int gold)
        : type_treasure_def(6, 0, value, 5)
    {
        this->gold = gold;
    }

    virtual void* Generate(void* owner, int x, int y);
};

class type_black_box_spells_def : public type_treasure_def {
public:
    int minimumLevel;
    int maximumLevel;
    int schoolMask;

    inline type_black_box_spells_def(
        int value, int minimumLevel, int maximumLevel, int schoolMask)
        : type_treasure_def(6, 0, value, 2)
    {
        this->minimumLevel = minimumLevel;
        this->maximumLevel = maximumLevel;
        this->schoolMask = schoolMask;
    }

    virtual void* Generate(void* owner, int x, int y);
};

class type_key_tent_def : public type_treasure_def {
public:
    inline type_key_tent_def(int subtype, int value)
        : type_treasure_def(10, subtype, value, 10)
    {
    }

    virtual void* Generate(void* owner, int x, int y);
    virtual int GetValue(void* object, void* map);
    virtual unsigned char IsTerrainDependent();
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

    virtual void* Generate(void* owner, int x, int y);
};

class type_map_dwelling_def : public type_dwelling_def {
public:
    inline type_map_dwelling_def(int subtype)
        : type_dwelling_def(subtype)
    {
    }

    virtual int GetValue(void* object, void* map);
};

class type_resource_lump_def : public type_treasure_def {
public:
    inline type_resource_lump_def(
        int objectType, int subtype, int value, int density)
        : type_treasure_def(objectType, subtype, value, density)
    {
    }

    virtual void* Generate(void* owner, int x, int y);
};

class type_prison_def : public type_treasure_def {
public:
    int experience;

    inline type_prison_def(int value, int experience)
        : type_treasure_def(62, 0, value, 30)
    {
        this->experience = experience;
    }

    virtual void* Generate(void* owner, int x, int y);
};

class type_scholar_def : public type_treasure_def {
public:
    inline type_scholar_def()
        : type_treasure_def(81, 0, 1500, 100)
    {
    }

    virtual void* Generate(void* owner, int x, int y);
};

class type_quest_creature_def : public type_black_box_creature_def {
public:
    inline type_quest_creature_def(int creatureType, int questIndex)
        : type_black_box_creature_def(creatureType)
    {
        objectType = 83;
        subtype = questIndex;
    }

    virtual void* Generate(void* owner, int x, int y);
    virtual int GetValue(void* object, void* map);
    virtual unsigned char IsTerrainDependent();
};

class type_quest_experience_def : public type_treasure_def {
public:
    int experience;

    inline type_quest_experience_def(
        int questIndex, int value, int experience)
        : type_treasure_def(83, questIndex, value, 10)
    {
        this->experience = experience;
    }

    virtual void* Generate(void* owner, int x, int y);
    virtual int GetValue(void* object, void* map);
    virtual unsigned char IsTerrainDependent();
};

class type_quest_gold_def : public type_treasure_def {
public:
    int gold;

    inline type_quest_gold_def(int questIndex, int value, int gold)
        : type_treasure_def(83, questIndex, value, 10)
    {
        this->gold = gold;
    }

    virtual void* Generate(void* owner, int x, int y);
    virtual int GetValue(void* object, void* map);
    virtual unsigned char IsTerrainDependent();
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
    int x;
    int y;
    int z;

    TRmgMapPosition() {}
    TRmgMapPosition(int newX, int newY, int newZ);

    TRmgMapPosition operator+(const TPoint& offset) const;
};

// Complete's zone-connection records are walked at a 0x1c-byte stride by
// the connection pass.  The first pointer identifies the opposite template
// zone; the three adjacent bytes select guard policy and record completion.
struct TRmgZoneConnection {
    TRmgTownSlot* destination;             // +0x00
    int value;                             // +0x04
    unsigned char unguarded;               // +0x08
    unsigned char placeBorderObjects;      // +0x09
    unsigned char connected;               // +0x0a
    char opaque000b[0x11];
};

enum ERmgTemplateZoneKind {
    RMG_TEMPLATE_HUMAN = 0,
    RMG_TEMPLATE_COMPUTER = 1,
    RMG_TEMPLATE_TREASURE = 2,
    RMG_TEMPLATE_JUNCTION = 3
};

struct TRmgTreasureRange {
    int minimum;
    int maximum;
    int density;
};

// ReadRmgTemplateZones allocates 0xd4 bytes and constructs connections at
// +0xc4. ConnectZones reads its _First at +0xc8 and _Last at +0xcc;
// those pointer offsets must not be mistaken for the vector's own offset.
// Unresolved scalar groups retain offset-based names until their consumers
// establish their roles. Other field names are provisional retail roles.
struct TRmgTownSlot {
    int zoneIndex;                    // +0x00
    int kind;                         // +0x04: ERmgTemplateZoneKind
    int size;                         // +0x08
    int minimumHumanPlayers;          // +0x0c
    int maximumHumanPlayers;          // +0x10
    int minimumPlayers;               // +0x14
    int maximumPlayers;               // +0x18
    int playerIndex;                  // +0x1c
    int parameters0020[8];
    unsigned char flag0040;
    unsigned char allowedTowns[9];    // +0x41
    int parameters004c[7];
    int parameters0068[7];
    unsigned char flag0084;
    unsigned char allowedTerrain[8];  // +0x85
    int monsterStrength;              // +0x90
    unsigned char flag0094;
    unsigned char allowedMonsters[10]; // +0x95
    TRmgTreasureRange treasure[3];     // +0xa0
    std::vector<TRmgZoneConnection> connections; // +0xc4

    TRmgZoneConnection* FindConnection(int destinationZone);
};
SIZE(TRmgTownSlot, 0xd4);

// The rmg.txt coordinator allocates this 0x38-byte object, assigns its
// name and size limits, and passes it to the zone reader in edx.
struct TRmgTemplate {
    std::string name;                  // +0x00
    std::vector<TRmgTownSlot*> zones;   // +0x10
    char opaque0020[0x10];
    int minimumSize;                  // +0x30
    int maximumSize;                  // +0x34

    ~TRmgTemplate();
    TRmgTownSlot* FindZone(int zoneIndex);
};
SIZE(TRmgTemplate, 0x38);

void ReadRmgTemplateZones(
    const TSpreadsheetResource* sheet, TRmgTemplate* mapTemplate,
    int firstRow, int endRow, int humanPlayers, int computerPlayers,
    int mapVersion);

// Retained fastcall helper at 0x545e00, also expanded by zone connections.
int GetRmgGuardValue(int value, int strength);

// Retail's common direction table contains eight consecutive two-dword
// offsets.  Its cinit at 0x530da0 proves the user-provided constructor while
// the absence of an atexit registration proves that destruction is trivial.
// The comparator is independently used by the RMG set cluster.
struct TPoint {
    int x;
    int y;

    TPoint() {}
    TPoint(int newX, int newY) : x(newX), y(newY) {}

    int Length() const;

    // Provisional source surface for the paired component arithmetic in
    // the retail clipping and midpoint-displacement bodies.
    TPoint operator-(const TPoint& other) const
    {
        return TPoint(x - other.x, y - other.y);
    }
    TPoint operator*(int scale) const
    {
        return TPoint(x * scale, y * scale);
    }
    TPoint operator/(int divisor) const
    {
        return TPoint(x / divisor, y / divisor);
    }
    TPoint& operator+=(const TPoint& offset)
    {
        x += offset.x;
        y += offset.y;
        return *this;
    }
    bool operator==(const TPoint& other) const
    {
        return x == other.x && y == other.y;
    }
    bool operator!=(const TPoint& other) const
    {
        return !(*this == other);
    }

    bool operator<(const TPoint& other) const
    {
        return y < other.y || (y == other.y && x < other.x);
    }
};

// The map-painting grid uses unsigned coordinates: the terrain set's lower
// bound at 0x5b8a40 compares y, then x, with jb/jae. Its retained constructor
// at 0x5b76b0 reads both arguments through pointers. This role name is
// provisional; the signed geometry TPoint is a separate recovered surface.
struct TRmgGridPoint {
    unsigned int x;
    unsigned int y;

    TRmgGridPoint() {}
    TRmgGridPoint(const unsigned int& newX, const unsigned int& newY)
        : x(newX), y(newY) {}

    bool operator<(const TRmgGridPoint& other) const
    {
        return y < other.y || (y == other.y && x < other.x);
    }
    TRmgGridPoint& operator+=(const TPoint& offset)
    {
        x += offset.x;
        y += offset.y;
        return *this;
    }
    TRmgGridPoint operator+(const TPoint& offset) const
    {
        TRmgGridPoint result = *this;
        return result += offset;
    }
};

struct TRmgZoneBounds {
    int minimumX;
    int minimumY;
    int maximumX;
    int maximumY;

    bool Contains(const TPoint& point) const
    {
        return point.x >= minimumX && point.x < maximumX &&
            point.y >= minimumY && point.y < maximumY;
    }
};

TPoint ClipRmgBoundaryPoint(
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
    int x;
    int y;

    TRmgRiverDeltaOffset(int newX, int newY) : x(newX), y(newY) {}
    ~TRmgRiverDeltaOffset() {}
};

class type_object;

struct TRmgMovementCost {
    unsigned cost : 16;
    unsigned unknown : 16;
};

// The connection pass extracts the signed zone id from bits 16..23 with
// `shl 8; sar 24` while ranking candidate squares by the low word.  Keeping
// both fields in one dword reproduces the retail bitfield loads rather than
// masking raw storage in the algorithm.
struct TRmgZoneCellState {
    unsigned score : 16;
    signed zone : 8;
    signed connectionEligibility : 8;
};

// The six-bit signed land field is fixed by retail's `shl 26; sar 26`
// extraction in the river-delta path.  The four-bit field at bit 26 is
// tested as a unit when river routing prices an already decorated tile.
struct TRmgGroundTile {
    TTerrainType landType : 6;
    unsigned unknown06 : 20;
    unsigned decorationType : 4;
    unsigned unknown30 : 2;
};

struct TRmgGroundTileData {
    unsigned roadSprite : 7;
    unsigned unknown07 : 1;
    unsigned blockedDirections : 4;
    unsigned connectionDirection : 3;
    unsigned unknown15 : 7;
    // BuildRoadCostMap proves these two Complete-only routing flags at bits
    // 22 and 25.  The first marks an object entrance whose adventure-object
    // traits constrain approach directions; the second admits the tile to
    // the road-cost flood.
    unsigned roadEntrance : 1;
    unsigned unknown23 : 1;
    unsigned connectionVisited : 1;
    unsigned roadPassable : 1;
    unsigned borderObject : 1;
    unsigned subterraneanGate : 1;
    unsigned zoneBoundary : 1;
    unsigned roadTarget : 1;
    unsigned riverTarget : 1;
    unsigned impassable : 1;
};

struct TRmgConnectionDecoration {
    unsigned present : 1;
    unsigned direction : 4;
    unsigned unknown05 : 27;
};

// The rand_trn.txt reader appends 0x4c-byte rows. ScoreObjectPlacement
// consumes the ten terrain values and the two vectors indexed by rule id.
// These are Complete-only role names; no Dreamcast RMG records survive.
struct TRmgObjectPlacementRule {
    int index;                         // +0x00
    int terrainScores[10];             // +0x04
    std::vector<int> adjacentScores;    // +0x2c
    std::vector<int> blockedScores;     // +0x3c
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
    TObjectType* prototype;              // +0x00
    int preferredTerrain;               // +0x04, rand_trn.txt rule binding
    unsigned refCount;                   // +0x08
    int prototypeIndex;
    TRmgObjectPlacementRule* placementRule; // +0x10
    std::vector<TPoint> outline;         // +0x14
    int overlapPriorities[8][6];         // +0x24
    unsigned char prioritiesInitialized; // +0xe4
    char pad00e5[3];

    void BuildOverlapPriorities();
};

class type_object {
public:
    TRmgObjectPropertiesRef* properties; // +0x04
    TRmgMapPosition position;             // +0x08
    unsigned char candidateCovers;
    unsigned char candidateBehind;
    unsigned char adjacentToCandidate;
    unsigned char overlapsCandidate;
    unsigned char blockedByCandidate;
    char pad0019[3];

    inline type_object(TRmgObjectPropertiesRef* newProperties)
        : properties(newProperties)
    {
        ++properties->refCount;
        position.x = -1;
        position.y = -1;
        position.z = -1;
        ClearPlacementMarks();
    }

    // The constructor and placement scorer share this five-byte reset.
    // The method name is provisional; retail preserves the store order.
    void ClearPlacementMarks()
    {
        candidateCovers = 0;
        candidateBehind = 0;
        adjacentToCandidate = 0;
        overlapsCandidate = 0;
        blockedByCandidate = 0;
    }

    unsigned char IsPlacementTouched() const
    {
        return adjacentToCandidate || blockedByCandidate || overlapsCandidate;
    }

    virtual ~type_object();
    virtual void UnknownOperation();
    virtual unsigned char IsWritable() const;
    virtual void Write(TAbstractFile* outfile);
};

struct TRmgMapItem {
    std::vector<type_object*> objects;    // +0x00
    TRmgMapPosition previousTile;         // +0x10
    TRmgMovementCost movement;            // +0x1c
    TRmgZoneCellState zoneState;           // +0x20
    TRmgGroundTile tile;                  // +0x24
    TRmgGroundTileData tileData;          // +0x28
    TRmgConnectionDecoration connection;  // +0x2c

    // ScoreObjectPlacement reads bit 27 with shr/test dl, whereas its
    // direct roadPassable condition tests the containing dword. The
    // provisional byte accessor reproduces that truncation; a direct
    // bitfield condition instead folds to test dword ptr [item+0x28],imm.
    unsigned char HasSubterraneanGate() const
    {
        return tileData.subterraneanGate;
    }

    // RepairWaterZoneBorders tests this flag after truncating it to a byte
    // at 0x53fe30, then tests roadPassable directly as a dword bit.
    unsigned char HasBorderObject() const
    {
        return tileData.borderObject;
    }

    // CreateRiver's reset pass copies a by-value predecessor before a
    // constant 32000 cost write, motivating this ordinary reset helper.
    // Its role name is provisional; Dreamcast has no RMG compiland.  A
    // generic cost parameter instead lowers the constant write as XOR,
    // whereas retail retains the constant AND/OR form.
    void ResetMovement(TRmgMapPosition previous)
    {
        movement.cost = 32000;
        previousTile = previous;
    }
};

// The map view vtable at 0x6409cc has the same six painting operations as
// the river adapter. The pure base at 0x6409e8 confirms that both implement
// this interface; the former destructor-only map base was incomplete.
class TRmgMapAdapterInterface {
public:
    virtual ~TRmgMapAdapterInterface() {}
    virtual void SetTile(
        const TRmgGridPoint& point, const TRmgTerrainTile& tile) = 0;
    virtual void SetOverlay(const TRmgGridPoint& point, int value) = 0;
    virtual TRmgGridPoint GetSize() = 0;
    virtual TRmgTerrainTile GetTile(const TRmgGridPoint& point) = 0;
    virtual int GetLand(const TRmgGridPoint& point) = 0;
    virtual int GetOverlay(const TRmgGridPoint& point) = 0;
};

class type_random_map : public TRmgMapAdapterInterface {
public:
    unsigned char ownsMapItems;           // +0x04
    char pad0005[3];
    TRmgMapItem* mapItems;                // +0x08
    int mapWidth;                         // +0x0c
    int mapHeight;                        // +0x10
    int numberLevels;                     // +0x14

    // The plane-view construction at 0x54013e writes its vptr before the
    // data members. Body assignments allow the earlier vptr write; putting
    // every field in the initializer list places it last. The width store
    // still moves early in RepairWaterZoneBorders and remains a residual.
    // Keep the plane offset in the canonical coordinate accessor.
    inline type_random_map(type_random_map& source, int level)
    {
        mapWidth = source.mapWidth;
        mapHeight = source.mapHeight;
        numberLevels = 1;
        mapItems = source.GetMapItem(0, 0, level);
        ownsMapItems = 0;
    }

    virtual ~type_random_map()
    {
        if (ownsMapItems)
            delete[] mapItems;
    }

    virtual void SetTile(
        const TRmgGridPoint& point, const TRmgTerrainTile& tile);
    virtual void SetOverlay(const TRmgGridPoint& point, int value);
    virtual TRmgGridPoint GetSize();
    virtual TRmgTerrainTile GetTile(const TRmgGridPoint& point);
    virtual int GetLand(const TRmgGridPoint& point);
    virtual int GetOverlay(const TRmgGridPoint& point);

    TRmgMapItem* GetMapItem(int x, int y);
    inline TRmgMapItem* GetMapItem(int x, int y, int z)
    {
        return mapItems + (z * mapHeight + y) * mapWidth + x;
    }
    TRmgMapItem* GetMapItem(TRmgMapPosition point);

    unsigned char CanPlaceObject(
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
    type_random_map* map;

    inline TRmgMapAdapter(type_random_map* newMap) : map(newMap) {}

    virtual void SetTile(
        const TRmgGridPoint& point, const TRmgTerrainTile& tile);
    virtual void SetOverlay(const TRmgGridPoint& point, int value);
    virtual TRmgGridPoint GetSize();
    virtual TRmgTerrainTile GetTile(const TRmgGridPoint& point);
    virtual int GetLand(const TRmgGridPoint& point);
    virtual int GetOverlay(const TRmgGridPoint& point);
};

class TRmgLinePainter {
public:
    TRmgGridPoint size;
    TRmgMapAdapterInterface* adapter;

    inline TRmgLinePainter(TRmgMapAdapterInterface* newAdapter)
        : size(newAdapter->GetSize()), adapter(newAdapter)
    {
    }
    ~TRmgLinePainter() {}

    virtual void* GetPattern(int value);
    virtual void PaintTile(int value, const TRmgMapPosition& tile);
    virtual void PaintOverlay(int value, const TRmgMapPosition& tile);
    virtual int CanPaint(const TRmgGridPoint& point);
    virtual void PaintNeighbour(int value, const TRmgMapPosition& tile);
    virtual int PaintPoint(const TRmgGridPoint& point);
};

class TRmgLineWalker {
public:
    TRmgLinePainter* painter;
    int riverType;
    TRmgGridPoint position;

    TRmgLineWalker(
        TRmgLinePainter* newPainter,
        int newRiverType,
        const TRmgGridPoint& start);
    void DrawTo(const TRmgGridPoint& destination);
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
    TRmgTownSlot* slot;              // +0x00
    int alignment;                   // +0x04
    char opaque0008[0x4];
    int terrain;                     // +0x0c
    TRmgMapPosition levelPosition;   // +0x10
    int boundaryRoughness;            // +0x1c: minimum of adjacent zones
    TRmgZoneBounds bounds;           // +0x20
    TRmgMapPosition position;        // +0x30: main town
    unsigned char active;            // +0x3c
    char opaque003d[7];
    int counts0044[232];             // +0x44: zeroed by the zone constructor
    // 0x53d9ae/0x53da0d load signed words; 0x53dc98 initializes 32000.
    std::vector<short> connectionDistances; // +0x3e4: signed graph distances
    std::vector<TPoint> boundary;    // +0x3f4: clipped polygon vertices
    std::vector<TPoint> entrances;   // +0x404

    // The candidate-placement filter consumes returned coordinate values;
    // its retained connection predicate compares center distance and size.
    // These names are provisional; the Dreamcast build has no RMG module.
    TRmgZone(TRmgTownSlot* slot);
    ~TRmgZone();
    TRmgMapPosition GetLevelPosition() const;
    void SetLevelPosition(TRmgMapPosition position);
    unsigned char CanConnect(const TRmgZone* other) const;
};

// Partial Voronoi topology recovered from TraceZoneBoundary and its caller
// at 0x53e050. The twin's owning zone identifies the region across an edge;
// following next traverses a closed polygon. Names are provisional.
struct TRmgBoundaryVertex {
    char opaque0000[8];
    TRmgZone* zone;                   // +0x08
    TRmgBoundaryVertex* twin;        // +0x0c
    TRmgBoundaryVertex* next;        // +0x10
    char opaque0014[8];
    TPoint position;                  // +0x1c
};

// The retained subdivision constructor and destructor own a root edge and
// a vector of allocated edges. The coordinator inserts zone sites, computes
// dual vertices, then looks up an edge for each site. All names are provisional.
class TRmgVoronoi {
public:
    TRmgBoundaryVertex* root;                 // +0x00
    std::vector<TRmgBoundaryVertex*> edges;  // +0x04

    TRmgVoronoi();
    ~TRmgVoronoi();
    void AddSite(TPoint point, TRmgZone* zone);
    TRmgBoundaryVertex* Locate(TPoint point);
    void BuildVertices();
};
SIZE(TRmgVoronoi, 0x14);

// The RMG progress sink is used through its third vtable slot by the zone
// connection coordinator.  No concrete implementation is owned by rmg.cpp.
class TRmgProgress {
public:
    virtual void UnknownProgressOperation0() = 0;
    virtual void UnknownProgressOperation1() = 0;
    virtual void Advance(int amount) = 0;
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
    int randomSeed;                                  // +0x004
    int mapVersion;                                  // +0x008
    type_random_map map;                             // +0x00c
    std::vector<TObjectType> objectsTxt;             // +0x024
    std::vector<TRmgObjectPropertiesRef*> objectPrototypes[232]; // +0x034
    std::vector<TRmgObjectPlacementRule> placementRules; // +0xeb4
    std::vector<type_object*> positions;             // +0xec4
    TRmgProgress* progress;                          // +0xed4
    unsigned char fixedHumanPlayers[8];              // +0x0ed8
    char opaque0ee0[0x4];
    int playerIndexMap[16];                          // +0x0ee4
    int townChoices[8];                              // +0x0f24
    char opaque0f44[0x4];
    int humanPlayerCount;                            // +0x0f48
    int humanTeamCount;                              // +0x0f4c
    int computerPlayerCount;                         // +0x0f50
    int computerTeamCount;                           // +0x0f54
    char opaque0f58[0x30];
    unsigned char disabledHeroes[156];               // +0x0f88
    char opaque1024[0x94];
    int waterContent;                                // +0x10b8
    int monsterStrength;                             // +0x10bc
    char opaque10c0[0x4];
    const char* templateName;                        // +0x10c4
    char opaque10c8[0x18];
    std::vector<TRmgZone*> zones;                    // +0x10e0
    std::vector<type_treasure_def*> objectGenerators; // +0x10f0
    std::vector<unsigned char> disabledKeyTents;     // +0x1100
    int objectCountByType[232];                      // +0x1110
    std::vector<TRmgMapPosition> roadTargets;        // +0x14b0
    std::vector<type_object*> monolithsOneWay;       // +0x14c0
    std::vector<type_object*> monolithsTwoWay;       // +0x14d0

    virtual ~type_random_map_generator();
    virtual void AddObject(type_object* object, TRmgMapPosition position);

    inline int GetSerializedMapVersion() const
    {
        switch (mapVersion) {
        case RMG_MAP_RESTORATION_OF_ERATHIA:
            return 14;
        case RMG_MAP_ARMAGEDDONS_BLADE:
            return 21;
        case RMG_MAP_SHADOW_OF_DEATH:
            return 28;
        }
    }

    void InitializeObjectGenerators();
    void ReadObjectPlacementRules();
    int ScoreObjectPlacement(
        TRmgObjectPropertiesRef* properties, TRmgMapPosition position);
    unsigned char CanPlaceZone(TRmgZone* zone);
    void BuildZoneBoundaries(TRmgTemplate* mapTemplate, int level);
    void FillZoneArea(TRmgZone* zone, TRmgBoundaryVertex* first);
    void JoinExtraZones(int originalZones, TRmgVoronoi* diagram);
    int CountPlacedZoneConnections(TRmgZone* zone) const;
    void FilterZonePositions(
        TRmgZone* zone, std::vector<TRmgMapPosition>& candidates, int mapSize);
    void DrawIrregularZoneBoundary(
        TPoint from, TPoint to, int zoneIndex, int level, int roughness);
    void DrawStraightZoneBoundary(
        TPoint from, TPoint to, int zoneIndex, int level);
    void TraceZoneBoundary(TRmgBoundaryVertex* first, unsigned char irregular);
    unsigned char CreateGroundConnection(
        TRmgZone* source,
        TRmgZoneConnection* connection,
        std::vector<TRmgMapItem*>* borderItems,
        std::vector<TRmgMapPosition>* borderPositions);
    void FloodConnectionRegion(TRmgMapPosition position);
    unsigned char CreateBorderConnection(
        TRmgZone* source, TRmgZoneConnection* connection);
    unsigned char CreateSubterraneanGate(
        TRmgZone* source, TRmgZoneConnection* connection);
    void CreateMonolithConnection(
        TRmgZone* source,
        TRmgZoneConnection* connection,
        int prototypeIndex);
    void ConnectZones();
    void RepairWaterZoneBorders();
    // Complete-only roles proved by the predecessor walk at 0x5408e0 and
    // the surrounding connection-cell updates at 0x540fc0.
    void OpenConnectionPath(TRmgMapPosition position, unsigned char narrow);
    void MarkBorderObjectArea(TRmgMapPosition position, int direction);
    int PlaceBorderObject(
        TRmgMapPosition position, int count, TRmgZone* zone);
    type_object* CreateGuard(int value, TRmgZone* zone);
    // Provisional Complete-only spelling: the 0x548290 road-target pass is
    // the sole direct caller, and the body builds the road traversal costs.
    void BuildRoadCostMap(TRmgMapPosition position);
    // Provisional spelling: retail's water-wheel caller and the river-delta
    // object selection prove the role; the Dreamcast build has no RMG TU.
    void CreateRiver(TRmgMapPosition source);
    void WriteMapHeader(TAbstractFile* outfile);
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
SIZE(TRmgMapItem, 0x30);
SIZE(type_random_map, 0x18);
SIZE(TRmgMapAdapterInterface, 0x04);
SIZE(TRmgMapAdapter, 0x08);
SIZE(TRmgLinePainter, 0x10);
SIZE(TRmgLineWalker, 0x10);
SIZE(TRmgRiverPainter, 0x20);
SIZE(type_random_map_generator, 0x14e0);

// Retail 0x6824e0 is indexed by the creature-traits level dword before
// type_black_box_creature_def divides by that creature's AI value.
DATA(0x006824E0) extern int gRmgCreatureValueByLevel[];

#endif  // HOMM3_RMG_H
