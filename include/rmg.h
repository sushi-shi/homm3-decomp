// Complete-only random-map generator declarations.
#ifndef HOMM3_RMG_H
#define HOMM3_RMG_H

#include <vector>
#include <va.h>

class TAbstractFile;

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
    char opaque0000[0x4];
    int kind;                       // +0x04: human (0) or computer (1)
    char opaque0008[0x14];
    int playerIndex;                // +0x1c
};

struct TRmgMapPosition {
    int x;
    int y;
    int z;
};

struct TRmgGeneratedTown {
    TRmgTownSlot* slot;              // +0x00
    int alignment;                   // +0x04
    char opaque0008[0x28];
    TRmgMapPosition position;         // +0x30
    unsigned char active;            // +0x3c
    char pad003d[0x3];
};

SIZE(TRmgTownSlot, 0x20);
SIZE(TRmgMapPosition, 0xc);
SIZE(TRmgGeneratedTown, 0x40);

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
    char opaque0000[0x4];
    int randomSeed;                                  // +0x004
    int mapVersion;                                  // +0x008
    char opaque000c[0xc];
    int mapSize;                                     // +0x018
    char opaque001c[0x4];
    int levels;                                      // +0x020
    char opaque0024[0xb0];
    std::vector<int> playerSlots;                    // +0x0d4
    char opaque00e4[0x480];
    std::vector<int> questSlots;                     // +0x564
    char opaque0574[0x964];
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
    std::vector<TRmgGeneratedTown*> generatedTowns;  // +0x10e0
    std::vector<type_treasure_def*> objectGenerators; // +0x10f0
    std::vector<unsigned char> disabledKeyTents;     // +0x1100

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
    void WriteMapHeader(TAbstractFile* outfile);
};

SIZE(type_random_map_generator, 0x1110);

// Retail 0x6824e0 is indexed by the creature-traits level dword before
// type_black_box_creature_def divides by that creature's AI value.
DATA(0x006824E0) extern int gRmgCreatureValueByLevel[];

// Retail's RMG set cluster stores x/y as consecutive dwords and compares y
// first, then x. The surrounding callers reach it only from random-map
// generation; the Dreamcast build has no corresponding RMG compiland.
struct TPoint {
    int x;
    int y;

    bool operator<(const TPoint& other) const
    {
        return y < other.y || (y == other.y && x < other.x);
    }
};

#endif  // HOMM3_RMG_H
