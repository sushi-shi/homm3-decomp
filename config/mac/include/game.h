// Mac declaration view for source-owned game logic. The gate arrays are
// located by the independently bounded retail body at 0:0xd7160..0xd72e0.
#ifndef HOMM3_MAC_GAME_H
#define HOMM3_MAC_GAME_H

#define HOMM3_TARGET_MAC 1
#include "va.h"

extern "C" double sqrt(double);
extern "C" void bzero(void*, unsigned long);
extern "C" unsigned long strlen(const char*);
extern "C" char* strcpy(char*, const char*);
extern "C" int sprintf(char*, const char*, ...);
#define memset(destination, fill, count) bzero(destination, count)

namespace std {
class string {
    struct rep {
        unsigned long m_length;
        char m_beforeData[8];
        char* m_data;
    }* m_handle;
public:
    string& assign(const char*, unsigned long);
    string& operator=(const char* value) { return assign(value, strlen(value)); }
    const char* c_str() const { return m_handle->m_data; }
};
}

enum THeroClass {
    classKnight = 0,
    classPlanesWalker = 16,
    classElementalist = 17,
    kNumHeroClasses = 18
};

enum {
    VIDEO_GAME_STATE_FORCED_BINK_LOW = 2,
    TOWN_CONFLUX = 8
};

struct THeroClassTraits {
    int m_townType;
    char m_beforeTownChance[0x30];
    signed char m_foundInTownType[9];
    char m_afterTownChance[3];
};
extern const THeroClassTraits (&g_heroClasses)[18];
int random(int minimum, int maximum);
extern int* g_videoGameState;
extern int g_gameOver;
extern long long g_bitNumber[];
extern int g_mapWidth;
extern int g_mapHeight;
void checkEndGame(int arg);
#include "mac_shared/game_artifact_wizards_well_id.h"
enum { TOWN_RAMPART = 1, SPECIAL_BUILDING_ID = 17, MAGE_GUILD_ID = 0,
       NUM_RESOURCES = 7 };
enum EGameResource { MERCURY = 1, SULFUR = 3, CRYSTAL = 4, GEMS = 5 };
extern const char* g_resourceObjectDefs[NUM_RESOURCES];
extern const char* g_artifactObjectDefFormat;
extern const char* g_townVillageObjectDefs[9];
extern const char* g_townFortObjectDefs[9];
extern const char* g_townCapitolObjectDefs[9];
enum TAdventureObjectType {
    NOTHING = 0, ARTIFACT = 5, MONSTER = 54, RANDOM_MONSTER = 71,
    RANDOM_TOWN = 77, RESOURCE = 79, TOWN = 98
};
enum {
    CASTLE_FORT_ID = 7, CASTLE_CITADEL_ID = 8, CASTLE_CASTLE_ID = 9,
    HALL_CAPITOL_ID = 13
};

class TAbstractFile {
public:
    virtual ~TAbstractFile();
    virtual int read(void* data, int size) = 0;
    virtual int write(const void* data, int size) = 0;
};

class armyGroup {
    char m_storage[56];
public:
    void initialize();
    int load(TAbstractFile* infile);
    int add(int armyType, int newNumTroops, int newIndex);
};

class mine {
public:
    char m_playerOwner;
    char m_type;
    unsigned char m_isAbandoned;
    char m_paddingBeforeGuards;
    armyGroup m_guards;
    unsigned char m_mapX;
    unsigned char m_mapY;
    unsigned char m_mapZ;
    char m_paddingAfterCoordinates;
    mine();
};

struct type_point {
    short m_x : 10;
    short m_y : 10;
    short m_z : 4;
    type_point() {}
};

namespace std {
template<unsigned long N> class bitset {
    unsigned long m_bits;
public:
    class reference {
        bitset& m_value;
        unsigned long m_position;
    public:
        reference(bitset& value, unsigned long position)
            : m_value(value), m_position(position) {}
        operator bool() const { return m_value.test(m_position); }
    };
    reference operator[](unsigned long position) {
        return reference(*this, position);
    }
    bool test(unsigned long position) const;
};
template<class T> class vector {
    unsigned long m_capacity;
    unsigned long m_size;
    T* m_data;
public:
    unsigned long size() const { return m_size; }
    void resize(unsigned long size);
    T& operator[](unsigned long index) { return m_data[index]; }
    const T& operator[](unsigned long index) const { return m_data[index]; }
    T& back() { return *(end() - 1); }
    void push_back(const T& value);
    T* begin() { return m_data; }
    T* end() { return m_data + m_size; }
};
}

class CSprite;
// Mac's installed MSL expands vector<CSprite*>::push_back in convertObject.
// Its three capacity accessor calls and one reserve call are retained at
// code0+0xe0e98..0xe0eec; the 12-byte POD-vector layout is also visible in
// NewfullMap's sprite-vector fields at +0x18/+0x1c/+0x20.
namespace std { template<class T> class allocator {}; }
namespace Metrowerks {
namespace details {
template<class Allocator, class Size, int Version> class compressed_pair_imp;
template<class Allocator, class Size>
class compressed_pair_imp<Allocator, Size, 1> : private Allocator {
    Size m_second;
public:
    Size& second() { return m_second; }
};
}
template<class Allocator, class Size>
class compressed_pair : private details::compressed_pair_imp<Allocator, Size, 1> {
    typedef details::compressed_pair_imp<Allocator, Size, 1> base;
public:
    Size& second() { return base::second(); }
};
}
namespace std {
template<class T> class __vector_pod {
protected:
    Metrowerks::compressed_pair<allocator<T>, unsigned long> capacity_;
    unsigned long size_;
    T* data_;
    unsigned long& cap() { return capacity_.second(); }
    T*& data() { return data_; }
    void reserve(unsigned long n);
    void push_back(const T& value) {
        if (size_ == cap())
            reserve(cap() != 0 ? 2 * cap() : 1);
        data()[size_++] = value;
    }
};
template<> class vector<CSprite*> : private __vector_pod<CSprite*> {
public:
    unsigned long size() const { return this->size_; }
    CSprite*& operator[](unsigned long index) { return this->data_[index]; }
    void push_back(CSprite* const& value) {
        __vector_pod<CSprite*>::push_back(value);
    }
};
}
#pragma options align=packed
class CObjectType {
public:
    std::string m_imageName;
    signed char m_width;
    signed char m_height;
private:
    char m_beforeType[0x2c - 6];
public:
    TAdventureObjectType m_objectType;
    int m_extra;
private:
    char m_afterExtra[0x38 - 0x34];
};
class CObject {
    char m_extraInfo[4];
public:
    unsigned char m_x;
    unsigned char m_y;
    unsigned char m_z;
private:
    char m_beforeTypeIndex;
public:
    unsigned short m_typeIndex;
private:
    char m_afterTypeIndex[2];
};
class NewmapCell {
    char m_beforeFlags[0xc];
public:
    unsigned short m_flags0011 : 12;
    unsigned short m_isTrigger : 1;
    unsigned short m_flags1315 : 3;
    struct TObjectCell {
        unsigned short m_objectIndex;
        char m_afterIndex[2];
    };
    std::vector<TObjectCell> m_objects;
    TAdventureObjectType m_type;
    short m_objectIndex;
    short m_objectTypeIndex;
    TAdventureObjectType getMapObject() const;
    unsigned long getMapExtraInfo() const;
};
class NewfullMap {
public:
    std::vector<CObjectType> m_objectTypes;
    std::vector<CObject> m_objects;
    std::vector<CSprite*> m_sprites;
private:
    char m_beforeCellData[0x9c - 0x24];
    NewmapCell* m_cellData;
    int m_size;
    unsigned char m_hasTwoLevels;
    char m_afterCellData[0xb88 - 0xa5];
public:
    CObjectType* findObjectType(int type, int objectIndex);
    void calculateCellExtra(NewmapCell* cell, unsigned char flag);
    NewmapCell* cell(int x, int y, int z);
private:
    NewmapCell* zCell(int x, int y, int z);
};
#pragma options align=reset
#include "mac_shared/mapcell_z_cell.h"
#include "mac_shared/mapcell_cell_xyz.h"

namespace ResourceManager { CSprite* getSprite(const char* name); }

#pragma options align=packed
class hero {
public:
    char m_beforeMana[0x18];
    short m_mana;
    char m_beforeClass[0x30 - 0x1a];
    int m_heroClass;
    char m_beforeFlags[0x105 - 0x34];
    unsigned int m_flags;
    char m_beforeWalkSpells[0x10d - 0x109];
    signed char m_dWalkSpellsCast;
    int m_disguiseLevel;
    int m_flightLevel;
    int m_waterWalkLevel;
    char m_beforeVisions[0x129 - 0x11a];
    int m_visionsPower;
    char m_beforeStats[0x46a - 0x12d];
    signed char m_stats[4];
    char m_afterStats[0x486 - 0x46e];
#include "mac_shared/hero_get_primary_skill.h"
    float getIntelligenceFactor() const;
#include "mac_shared/hero_get_max_mana.h"
    unsigned char isWieldingArtifact(int artifact) const;
    int getMysticismBonus() const;
};
#pragma options align=reset

#pragma options align=packed
class town {
public:
    char m_id;
    signed char m_owner;
    unsigned char m_builtThisTurn;
    char m_beforeType;
    signed char m_type;
    char m_beforeHeroes[0xc - 5];
    int m_garrisonHeroId;
    int m_visitingHeroId;
    char m_beforePond[0x34 - 0x14];
    unsigned char m_pondAmount;
    char m_beforePondResource[3];
    int m_pondResource;
    char m_beforeBuilt[0x144 - 0x3c];
    long long m_built;
    long long m_active;
    char m_afterActive[0x15c - 0x154];
#include "mac_shared/town_has_building.h"
#include "mac_shared/town_is_castle.h"
#include "mac_shared/town_is_capitol.h"
};
#pragma options align=reset

struct VictoryConditionStruct {
    unsigned char checkForTotalResources();
};
struct MacMapHeader {
    char m_beforeTeams[0xd];
    signed char m_teamInfo[8];
    char m_beforeVictory[0x2c - 0x15];
    VictoryConditionStruct m_victoryCondition;
};
struct MacPlayerAI {
    char m_beforeProduction[0x10];
    long m_turnProductionResource[NUM_RESOURCES];
};
struct playerData {
    char m_beforeResources[0x98];
    long m_resources[NUM_RESOURCES];
    char m_beforeAI[0xf0 - 0xb4];
    MacPlayerAI m_ai;
    char m_afterAI[0x15c - 0xf0 - sizeof(MacPlayerAI)];
};

#pragma options align=packed
struct MacGameSetup {
    int m_alignment[8];
    char m_beforeDifficulty[0x28 - 0x20];
    signed char m_difficulty;
};
#pragma options align=reset

class game {
    // The hero choice body at 0:0xce398 proves these packed Mac offsets.
    unsigned char m_beforeGrailAsked[0x90];
public:
    unsigned char m_grailAsked;
private:
    unsigned char m_beforePlayerDisabled[0x1eec6 - 0x91];
public:
    // Mac perDay tests each flag with extsb.; the Windows field is unsigned.
    signed char m_playerDisabled[8];
    unsigned short m_day;
    short m_week;
private:
    unsigned char m_beforeVersion[0x1ef24 - 0x1eed2];
public:
    int m_gameVersion;
private:
    char m_beforeAlignment[0x1ef3c - 0x1ef28];
public:
    MacGameSetup m_setup;
private:
    char m_beforeMapHeader[0x1f0f8 - 0x1ef65];
public:
    MacMapHeader m_mapHeader;
private:
    char m_beforeWorldMap[0x1f3c0 - 0x1f0f8 - sizeof(MacMapHeader)];
public:
    NewfullMap m_worldMap;
public:
    playerData m_players[8];
    std::vector<town> m_towns;
public:
    enum { HERO_COUNT = 156 };
    hero m_heroes[HERO_COUNT];
    char m_heroAvailability[HERO_COUNT];
    std::bitset<8> m_heroPoolMap[HERO_COUNT];
private:
    // The 0x40-byte mine records begin at +0x4d048; gates at +0x4d3ec.
    unsigned char m_beforeMines[0x4d048 - 0x4cee8];
public:
    std::vector<mine> m_mines;
private:
    unsigned char m_beforeGateArrays[0x398];
public:
    std::vector<type_point> m_undergroundGateExits;
    std::vector<long> m_undergroundGatePairs;
    int loadMinePool(TAbstractFile* infile, int saveVersion);
    int getNewHeroId(int playerPos, THeroClass excluded,
                     unsigned char preferAlignment, THeroClass preferredClass);
    void matchUndergroundGates();
    void perDay();
    void perWeek();
    void perMonth();
    void convertObject(NewmapCell* tempCell);
    bool isHumanTeam(int teamNum) const;
    bool isHumanAlly(int playerNum) const;
#include "mac_shared/game_get_team.h"
    bool growCoverOfDarkness();
    void resetAllPlayerVisibility();
    void calculateProduction();
#include "mac_shared/game_get_hero.h"
#include "mac_shared/game_get_town.h"
};

extern game* g_game;
#include "mac_shared/game_is_human_ally.h"

#endif
