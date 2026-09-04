// Complete-only random-map generator declarations.
#ifndef HOMM3_RMG_H
#define HOMM3_RMG_H

#include <bitset>
#include <string>
#include <vector>
#include <va.h>
#include "terrain_type.h"

class TAbstractFile;
struct TRmgZone;
struct TRmgTerrainTile;

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

// A generated town retains its source slot, selected alignment, and map
// position.  The map-header writer proves every named offset through the
// player-alignment and main-town serialization loops.
struct TRmgTownSlot {
    int zoneIndex;                    // +0x00
    int kind;                       // +0x04: human (0) or computer (1)
    char opaque0008[0x14];
    int playerIndex;                // +0x1c
};

struct TRmgMapPosition {
    int x;
    int y;
    int z;

    TRmgMapPosition() {}
    TRmgMapPosition(int newX, int newY, int newZ);
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

struct TRmgZoneBounds {
    int minimumX;
    int minimumY;
    int maximumX;
    int maximumY;
};

// Retail's common direction table contains eight consecutive two-dword
// offsets.  Its cinit at 0x530da0 proves the user-provided constructor while
// the absence of an atexit registration proves that destruction is trivial.
// The comparator is independently used by the RMG set cluster.
struct TPoint {
    int x;
    int y;

    TPoint() {}
    TPoint(int newX, int newY) : x(newX), y(newY) {}

    bool operator<(const TPoint& other) const
    {
        return y < other.y || (y == other.y && x < other.x);
    }
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
    unsigned unknown24 : 8;
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
    unsigned unknown12 : 10;
    // BuildRoadCostMap proves these two Complete-only routing flags at bits
    // 22 and 25.  The first marks an object entrance whose adventure-object
    // traits constrain approach directions; the second admits the tile to
    // the road-cost flood.
    unsigned roadEntrance : 1;
    unsigned unknown23 : 2;
    unsigned roadPassable : 1;
    unsigned borderObject : 1;
    unsigned subterraneanGate : 1;
    unsigned unknown28 : 1;
    unsigned roadTarget : 1;
    unsigned riverTarget : 1;
    unsigned impassable : 1;
};

struct TRmgConnectionDecoration {
    unsigned present : 1;
    unsigned direction : 4;
    unsigned unknown05 : 27;
};

struct TRmgObjectProperties {
    int defNumber;                       // +0x00
    unsigned char passable[8];           // +0x04
    unsigned char enterable[8];          // +0x0c
    unsigned land;                       // +0x14
    std::bitset<10> landPage;            // +0x18
    int type;                            // +0x1c
    int subtype;                         // +0x20
    int page;                            // +0x24
    unsigned char flat;                  // +0x28
    unsigned char hasEntrance;           // +0x29
    char pad002a[2];
    int enterX;                          // +0x2c
    int enterY;                          // +0x30
    int width;                           // +0x34
    int height;                          // +0x38
    unsigned char colors[8];             // +0x3c
    unsigned char shadows[8];            // +0x44
};

struct TRmgObjectPropertiesRef {
    TRmgObjectProperties* prototype;      // +0x00
    int unknown04;
    unsigned refCount;                   // +0x08
    int prototypeIndex;
    char opaque0010[0xd8];
};

class type_object {
public:
    TRmgObjectPropertiesRef* properties; // +0x04
    TRmgMapPosition position;             // +0x08
    unsigned char unknown14;
    unsigned char unknown15;
    unsigned char unknown16;
    unsigned char unknown17;
    unsigned char unknown18;
    char pad0019[3];

    inline type_object(TRmgObjectPropertiesRef* newProperties)
        : properties(newProperties)
    {
        ++properties->refCount;
        position.x = -1;
        position.y = -1;
        position.z = -1;
        unknown14 = 0;
        unknown15 = 0;
        unknown16 = 0;
        unknown17 = 0;
        unknown18 = 0;
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
};

class TRmgMapInterface {
public:
    virtual ~TRmgMapInterface() {}
};

class type_random_map : public TRmgMapInterface {
public:
    unsigned char ownsMapItems;           // +0x04
    char pad0005[3];
    TRmgMapItem* mapItems;                // +0x08
    int mapWidth;                         // +0x0c
    int mapHeight;                        // +0x10
    int numberLevels;                     // +0x14

    inline type_random_map(type_random_map& source, int level)
        : ownsMapItems(0),
          mapItems(
              source.mapItems
              + level * source.mapWidth * source.mapHeight),
          mapWidth(source.mapWidth),
          mapHeight(source.mapHeight),
          numberLevels(1)
    {
    }

    virtual ~type_random_map()
    {
        if (ownsMapItems)
            delete[] mapItems;
    }

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
class TRmgMapAdapterInterface {
public:
    virtual ~TRmgMapAdapterInterface() {}
    virtual void SetTile(
        const TPoint& point, const TRmgTerrainTile& tile) = 0;
    virtual void SetOverlay(const TPoint& point, int value) = 0;
    virtual TPoint GetSize() = 0;
    virtual TRmgTerrainTile GetTile(const TPoint& point) = 0;
    virtual int GetLand(const TPoint& point) = 0;
    virtual int GetOverlay(const TPoint& point) = 0;
};

class TRmgMapAdapter : public TRmgMapAdapterInterface {
public:
    type_random_map* map;

    inline TRmgMapAdapter(type_random_map* newMap) : map(newMap) {}

    virtual void SetTile(
        const TPoint& point, const TRmgTerrainTile& tile);
    virtual void SetOverlay(const TPoint& point, int value);
    virtual TPoint GetSize();
    virtual TRmgTerrainTile GetTile(const TPoint& point);
    virtual int GetLand(const TPoint& point);
    virtual int GetOverlay(const TPoint& point);
};

class TRmgLinePainter {
public:
    TPoint size;
    TRmgMapAdapterInterface* adapter;

    inline TRmgLinePainter(TRmgMapAdapterInterface* newAdapter)
        : size(newAdapter->GetSize()), adapter(newAdapter)
    {
    }
    ~TRmgLinePainter() {}

    virtual void* GetPattern(int value);
    virtual void PaintTile(int value, const TRmgMapPosition& tile);
    virtual void PaintOverlay(int value, const TRmgMapPosition& tile);
    virtual int CanPaint(const TPoint& point);
    virtual void PaintNeighbour(int value, const TRmgMapPosition& tile);
    virtual int PaintPoint(const TPoint& point);
};

class TRmgLineWalker {
public:
    TRmgLinePainter* painter;
    int riverType;
    TPoint position;

    TRmgLineWalker(
        TRmgLinePainter* newPainter,
        int newRiverType,
        const TPoint& start);
    void DrawTo(const TPoint& destination);
};

class TRmgRiverPainter : public TRmgLinePainter, public TRmgLineWalker {
public:
    TRmgRiverPainter(
        TRmgMapAdapterInterface* newAdapter,
        int newRiverType,
        const TPoint& start);
    virtual ~TRmgRiverPainter();
};

// A generated zone owns both its template metadata and the Complete-only
// connection state.  WriteMapHeader proves the player/town fields through
// +0x3c; the connection pass independently proves the bounding rectangle,
// 0x1c-stride connection vector, and entrance vector at +0x404.
struct TRmgZone {
    TRmgTownSlot* slot;              // +0x00
    int alignment;                   // +0x04
    char opaque0008[0x4];
    int terrain;                     // +0x0c
    TRmgMapPosition levelPosition;   // +0x10
    int opaque001c;
    TRmgZoneBounds bounds;           // +0x20
    TRmgMapPosition position;        // +0x30: main town
    unsigned char active;            // +0x3c
    char opaque003d[0x8b];
    std::vector<TRmgZoneConnection> connections; // +0xc8
    char opaque00d8[0x32c];
    std::vector<TPoint> entrances;   // +0x404
};

SIZE(TRmgTownSlot, 0x20);
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
    std::vector<TRmgObjectProperties> objectsTxt;    // +0x024
    std::vector<TRmgObjectPropertiesRef*> objectPrototypes[232]; // +0x034
    std::vector<void*> unknownPointers;              // +0xeb4
    std::vector<type_object*> positions;             // +0xec4
    void* progress;                                  // +0xed4
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
    unsigned char CreateSubterraneanGate(
        TRmgZone* source, TRmgZoneConnection* connection);
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
SIZE(TRmgRiverDeltaOffset, 0x08);
SIZE(TRmgMovementCost, 0x04);
SIZE(TRmgZoneCellState, 0x04);
SIZE(TRmgGroundTile, 0x04);
SIZE(TRmgGroundTileData, 0x04);
SIZE(TRmgConnectionDecoration, 0x04);
SIZE(TRmgObjectProperties, 0x4c);
SIZE(TRmgObjectPropertiesRef, 0xe8);
SIZE(type_object, 0x1c);
SIZE(TRmgMapItem, 0x30);
SIZE(TRmgMapInterface, 0x04);
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
