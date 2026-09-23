// Declaration view for authored AI purchaser bodies. Mac getPurchaseValue
// copies 56-byte armies and addresses purchaser members at 0/4/8/0xa/0x24.
#ifndef HOMM3_MAC_AI_PLAYER_H
#define HOMM3_MAC_AI_PLAYER_H

#include "va.h"

#define __cdecl

extern "C" void* memcpy(void*, const void*, unsigned long);
// Both calculateReserve zero fills have constant fill 0. Mac retail calls the
// verified two-argument .bzero routine at 0:0x26ad3c; DC calls _memset.
extern "C" void bzero(void*, unsigned long);
#define memset(destination, fill, count) bzero(destination, count)

enum TCreatureType { CREATURE_NONE = -1 };
enum type_building_id { DWELLING_0_ID = 30, DWELLING_6_ID = 36 };
enum SpellID { SPELL_DIMENSION_DOOR = 8 };
enum TSkillMastery { SKILL_BASIC = 1 };
enum TAdventureObjectType {
    CURSED_GROUND = 21,
    HERO = 34,
    MAGIC_SPRING = 48,
    MAGIC_WELL = 49
};
enum { TOWN_DWELLING_SLOTS = 14, ARTIFACT_ANGELIC_ALLIANCE = 0x81,
       GOLD = 6 };
typedef long long __int64;

class hero;
class town;
class NewmapCell;
extern int g_mapWidth;
extern int g_mapHeight;

// Mac NewfullMap begins at game+0x1f3c0. Its first thirteen vector members
// occupy 0x9c bytes; the cell pointer and map size are at +0x9c/+0xa0.
// The 232 trailing 12-byte vector members make the full class 0xb88 bytes.
class NewfullMap {
    unsigned char m_beforeCellData[0x9c];
    NewmapCell* m_cellData;
    int m_size;
    unsigned char m_hasTwoLevels;
    unsigned char m_beforeObjectTypeIndex[3];
    unsigned char m_objectTypeIndex[232 * 12];
public:
    NewmapCell* cell(int x, int y, int z);
private:
    NewmapCell* zCell(int x, int y, int z);
};

// Canonical struct.h:24 and findpath.h:40 packed point/path records.
struct type_point {
    short m_x : 10;
    short m_y : 10;
    short m_z : 4;
    type_point() {}
#include "mac_shared/type_point_ctor.h"
};

#pragma options align=packed
struct pathCell {
    type_point m_point;
    unsigned int m_visited : 1;
    unsigned int m_isTrigger : 1;
    unsigned int m_inBoat : 1;
    unsigned int m_magicForbidden : 1;
    unsigned int m_flying : 1;
    unsigned int m_waterWalking : 1;
    unsigned int m_townPortal : 1;
    unsigned int m_dimensionDoor : 1;
    unsigned int m_castleGate : 1;
    unsigned int m_startAtTrigger : 1;
    unsigned int m_canStop : 1;
    unsigned int m_lastCanStop : 1;
    unsigned int m_direction : 4;
    int m_deltaX : 5;
    int m_deltaY : 5;
    unsigned int m_flightCost : 6;
    type_point m_lastPoint;
    type_point m_monster;
    long m_barrierValue;
    long m_dangerValue;
    unsigned short m_cost;
    unsigned short m_adjustedCost;
    unsigned short m_moveLeft;
};
#pragma options align=reset

// ai_player.h's 16-byte destination record: point/value/move cost and flags.
struct HeroDestination {
    type_point m_point;
    long m_value;
    long m_moveCost;
    unsigned char m_isNearby;
    unsigned char m_isCritical;
};

class searchArray {
    unsigned char m_beforeCellData[0x24];
    pathCell* m_cellData;
public:
#include "mac_shared/search_get_cell.h"
};

class armyGroup {
public:
    enum { ARMY_GROUP_SLOT_COUNT = 7 };
    union {
        int m_armies[ARMY_GROUP_SLOT_COUNT];
        TCreatureType m_armyTypes[ARMY_GROUP_SLOT_COUNT];
    };
    armyGroup();
    int m_numTroops[ARMY_GROUP_SLOT_COUNT];
    int getNumArmies() const;
};

struct type_creature_source {
    type_creature_source(TCreatureType newType, short* newAmount, bool isFree);
    TCreatureType m_type;
    short* m_ptr;
    short m_number;
    unsigned char m_isFree;
};

// ai_creature_value.h and both retail calculateReserve bodies use a 12-byte
// sort key: creature type at +0, value at +4, amount at +8.
struct type_creature_value {
    TCreatureType m_type;
    long m_value;
    short m_amount;
    bool operator<(const type_creature_value& arg) const
    {
        return m_value < arg.m_value;
    }
};

namespace std { template<class T> class allocator {}; }
namespace Metrowerks {
namespace details {
template<class Allocator, class Size, int Version> class compressed_pair_imp;
template<class Allocator, class Size>
class compressed_pair_imp<Allocator, Size, 1> : private Allocator {
    Size m_second;
public:
    explicit compressed_pair_imp(Size value) : m_second(value) {}
    Size& second() { return m_second; }
};
}
template<class Allocator, class Size>
class compressed_pair
    : private details::compressed_pair_imp<Allocator, Size, 1> {
    typedef details::compressed_pair_imp<Allocator, Size, 1> base;
public:
    explicit compressed_pair(Size value) : base(value) {}
    Size& second() { return base::second(); }
};
}

namespace std {
// The pinned CodeWarrior MSL vector uses the non-POD
// __vector_deleter -> __vector_imp -> vector chain for type_creature_source.
// Its element access delegates to *(data() + index); all three fields retain
// the 12-byte ABI proved by Mac buyer and purchaser member accesses.
template<class T> class __vector_deleter {
protected:
    Metrowerks::compressed_pair<allocator<T>, unsigned long> m_capacity;
    unsigned long m_size;
    T* m_data;
    __vector_deleter() : m_capacity(0), m_size(0), m_data(0) {}
    T*& data() { return m_data; }
    T* const& data() const { return m_data; }
};
template<class T> class __vector_imp : private __vector_deleter<T> {
protected:
    __vector_imp() {}
    unsigned long size() const { return this->m_size; }
    T* begin() { return this->data(); }
    T* end() { return this->data() + this->m_size; }
    T& operator[](unsigned long index) { return *(this->data() + index); }
    const T& operator[](unsigned long index) const { return *(this->data() + index); }
};
template<class T> class vector : private __vector_imp<T> {
    typedef __vector_imp<T> base;
public:
    vector() {}
    ~vector();
    void clear();
    void push_back(const T& value);
    unsigned long size() const { return base::size(); }
    T* begin() { return base::begin(); }
    T* end() { return base::end(); }
    T& operator[](unsigned long index) { return base::operator[](index); }
    const T& operator[](unsigned long index) const { return base::operator[](index); }
};
// The installed CodeWarrior MSL routes POD pointers through __vector_pod.
// valueOfHiring expands its canonical push_back at Mac 0:0x3546c: the three
// capacity getter calls and retained reserve precede the indexed store.
template<class T> class __vector_pod {
protected:
    Metrowerks::compressed_pair<allocator<T>, unsigned long> capacity_;
    unsigned long size_;
    T* data_;
    __vector_pod();
    unsigned long& cap() { return capacity_.second(); }
    T*& data() { return data_; }
    void reserve(unsigned long n);
    void push_back(const T& x) {
        if (size_ == cap())
            reserve(cap() != 0 ? 2 * cap() : 1);
        data()[size_++] = x;
    }
};
template<class T>
inline __vector_pod<T>::__vector_pod()
    : capacity_(0), size_(0), data_(0) {}
// The installed MSL vector default constructor passes through an empty
// __vector_imp body before constructing its POD storage.
template<> class __vector_imp<pathCell*> : private __vector_pod<pathCell*> {
protected:
    __vector_imp() {}
    void push_back(pathCell* const& x) { __vector_pod<pathCell*>::push_back(x); }
    unsigned long size() const { return this->size_; }
    pathCell** begin() { return this->data_; }
    pathCell** end() { return this->data_ + this->size_; }
    pathCell*& operator[](unsigned long index) { return this->data_[index]; }
};
template<> class vector<pathCell*> : private __vector_imp<pathCell*> {
    typedef __vector_imp<pathCell*> base;
public:
    vector() {}
    ~vector();
    void push_back(pathCell* const& x) { base::push_back(x); }
    unsigned long size() const { return base::size(); }
    pathCell** begin() { return base::begin(); }
    pathCell** end() { return base::end(); }
    pathCell*& operator[](unsigned long index) { return base::operator[](index); }
};
template<class T> void sort(T* first, T* last);
}

class type_AI_player {
    short m_team;
    long m_magusHutValue;
    long m_reservedFunds[7];
    long m_resourceSupply[7];
    long m_resourceDemand[7];
    double m_resourceValue[7];
public:
    void calculateReserve();
    void buyCreatures(hero* currentHero, town* currentTown);
    void tradeResources(const int* cost, long number);
};
extern type_AI_player g_aiPlayers[8];

// The Mac buyCreatures body addresses these fields at the shown offsets:
// player records have 0x15c stride and resources at +0x98; the hero army
// begins at +0x91; game players begin at +0x1ff48.
class playerData {
public:
    signed char m_color;
    signed char m_numHeroes;
private:
    unsigned char m_beforeCurrHeroId[2];
public:
    int m_currHeroId;
    int m_heroes[8];
private:
    unsigned char m_beforeTownCount[0x3e - 0x28];
public:
    char m_numTowns;
    char m_currTownId;
    char m_townIds[0x48];
private:
    unsigned char m_beforeResources[0x98 - 0x88];
public:
    long m_resources[7];
private:
    unsigned char m_afterResources[0x15c - 0x98 - 28];
public:
    bool hasGivenArtifact(int artifact);
};

#pragma options align=packed
// Hero.h's 0x18-byte packed base is shared by heroes and boats. Mac
// attemptTeleport expands its getLocation accessor from this base.
class type_obscuring_object {
public:
    short m_x;
    short m_y;
    short m_z;
private:
    unsigned char m_valid;
    type_point m_obscuredLocation;
public:
    char m_paddingBeforeObscuredType;
    TAdventureObjectType m_obscuredType;
private:
    unsigned char m_wasTrigger;
public:
    char m_paddingBeforeExtraInfo[3];
#include "mac_shared/obscuring_object_get_location.h"
private:
    unsigned long m_extraInfo;
};

class hero : public type_obscuring_object {
public:
    short m_mana;
private:
    unsigned char m_beforeOwner[0x22 - 0x1a];
public:
    signed char m_owner;
private:
    unsigned char m_beforePathTarget[0x35 - 0x23];
public:
    int m_pathTargetX;
    int m_pathTargetY;
    short m_pathTargetZ;
private:
    unsigned char m_beforeMovePoints[0x49 - 0x3f];
public:
    int m_maxMovePoints;
    int m_movePoints;
private:
    unsigned char m_beforeArmy[0x91 - 0x51];
public:
    armyGroup m_army;
    int getMorale(const hero* otherHero, unsigned char onCursedGround,
                  unsigned char includeArtifacts) const;
    short getPrimarySkillTotal() const;
    unsigned char isWieldingArtifact(int artifact) const;
    int getSpecialTerrain() const;
    int getManaCost(int whichSpell, const armyGroup* enemy,
                    int magicTerrain) const;
    TSkillMastery getSpellLevel(SpellID spell, int magicTerrain) const;
#include "mac_shared/hero_spell_is_available.h"
    TAdventureObjectType heroFn004E4EC0();
#include "mac_shared/hero_get_mana_cost.h"
#include "mac_shared/hero_get_spell_level.h"
#include "mac_shared/hero_get_target.h"
    void useSpell(int cost);
private:
    unsigned char m_beforeFlags[0x105 - 0x91 - sizeof(armyGroup)];
public:
    unsigned int m_flags;
public:
    float m_turnExperienceToRvRatio;
    signed char m_dWalkSpellsCast;
private:
    // Mac attemptTeleport tests spell 8 at hero+0x42c, so the table begins
    // at +0x424. The corresponding packed Windows table begins at +0x430.
    unsigned char m_beforeAvailableSpells[0x424 - 0x10e];
public:
    unsigned char m_availableSpells[70];
private:
    // Mac buyCreatures indexes game::m_heroes with the observed 0x486 stride.
    unsigned char m_unrecoveredTail[0x486 - 0x424 - 70];
};
#pragma options align=reset

class town {
public:
    char m_id;
    char m_owner;
    unsigned char m_builtThisTurn;
    unsigned char m_threateningHeroes;
public:
    char m_type;
    unsigned char m_mapX;
    unsigned char m_mapY;
    unsigned char m_mapZ;
private:
    unsigned char m_beforeGarrison[4];
public:
    int m_garrisonHeroId;
private:
    unsigned char m_beforePopulation[0x16 - 0x10];
public:
    short m_population[14];
private:
    unsigned char m_unrecoveredTail[0x15c - 0x16 - 28];
public:
    const armyGroup& getArmy() const;
    int* getBuildCostArray(type_building_id building) const;
    __int64 getBuildableMask() const;
    type_building_id buildBuilding(int buildingId, unsigned char setBuiltFlag,
                                   unsigned char soundFlag);
};

class game {
    unsigned char m_beforeSetup[0x1ef64];
public:
    struct { unsigned char m_difficulty; } m_setup;
private:
    unsigned char m_beforeMapHeader[0x1f0f8 - 0x1ef65];
public:
    struct {
        unsigned char m_beforeTeamInfo[0xd];
        signed char m_teamInfo[8];
    } m_mapHeader;
private:
    unsigned char m_beforeWorldMap[0x1f3c0 - 0x1f0f8 - 0x15];
public:
    NewfullMap m_worldMap;
private:
    // m_worldMap ends at game+0x1ff48, the beginning of the player records.
public:
    playerData m_players[8];
    std::vector<town> m_towns;
    hero m_heroes[156];
#include "mac_shared/game_get_hero.h"
#include "mac_shared/game_get_town.h"
    bool townAlreadyBuiltOn(int townId) const;
    bool isHumanTeam(int teamNum) const;
    bool isHumanAlly(int playerNum) const;
#include "mac_shared/game_get_team.h"
    NewmapCell* getCell(type_point point);
};
#include "mac_shared/game_get_cell.h"
#include "mac_shared/game_town_already_built.h"
#include "mac_shared/game_is_human_ally.h"
extern game* g_game;
extern int g_netLocalGamePos;

struct TCreatureTypeTraits {
    unsigned char m_beforeAiValue[0x40];
    int m_aiValue;
    int m_growthRate;
    // Canonical armygrp.h record is 0x74 bytes; both retail targets index
    // creature traits at this stride and read growth at +0x44.
    unsigned char m_unrecoveredTail[0x74 - 0x48];
};
extern const TCreatureTypeTraits g_creatureTypeTraits[150];
extern TCreatureType g_townDwellingCreatures[];
extern __int64 g_bitNumber[];

// Canonical armygrp.h:536: the mastery bonus row starts at +0x34 in a
// 0x88-byte spell record. attemptTeleport reads that row for spell 8.
struct SSpellTraits {
    unsigned char m_beforeMasteryBonus[0x34];
    int m_masteryBonus[4];
    unsigned char m_unrecoveredTail[0x88 - 0x44];
};
extern const SSpellTraits (&g_spellTraits)[81];

// Canonical mapcell.h:996 keeps the trigger bit in the +0xc flag word and
// the object-type enum at +0x1e. Only those proven reads are projected here.
#pragma options align=packed
class NewmapCell {
public:
    unsigned long m_extraInfo;
private:
    unsigned char m_beforeCellFlags[0xc - 4];
public:
    unsigned short m_flags0011 : 12;
    unsigned short m_isTrigger : 1;
    unsigned short m_flags1315 : 3;
private:
    unsigned char m_beforeType[0x1e - 0xe];
public:
    TAdventureObjectType m_type;
};
#pragma options align=reset

#include "mac_shared/mapcell_z_cell.h"
#include "mac_shared/mapcell_cell_xyz.h"

class advManager {
public:
    NewmapCell* getCell(type_point point);
    void teleportTo(hero* who, type_point destination, const char* sampleName,
                    unsigned char isRemoteMove, unsigned char drawChanges,
                    unsigned char isReplay);
};
extern advManager* g_advManager;
extern "C" int abs(int value);
extern int g_heroGoldCost;

long findAllDestinations(hero* currentHero, searchArray* currentSearchArray,
                         std::vector<HeroDestination>& destinations,
                         long maxDistance, unsigned char hiringHero,
                         unsigned char allowSpells,
                         unsigned char exploreMode);

void getMonsterCost(int monId, int* resCost);
int aiResourceCost(long playerId, const int* resources);
static int maxBuyableCreatures(const long* funds, TCreatureType type,
                              int limit);

class type_AI_creature_swapper {
protected:
    armyGroup* m_army;
    armyGroup* m_adjacentArmy;
    unsigned char m_hasAngelicAlliance;
    short m_morale;
    short m_alignmentCount;
    unsigned char m_alignments[10];
    long m_armyValueIncrease;
    short m_improvement;
    void getAlignments();
    void addCreatures(TCreatureType type, short amount, short slot);
    void dumpExtraCreature();
    long valueOfAddingArmy(TCreatureType type, short count,
                           short& slot, unsigned char mustReplaceCreature);
    long doBestSwap(bool canTakeAll);
public:
    void doSwap(hero* currentHero, armyGroup* sourceArmy,
                hero* secondHero, unsigned char newHasAngelicAlliance);
    long getSwapValue(const hero* currentHero, const armyGroup* sourceArmy,
                      const hero* secondHero,
                      unsigned char newHasAngelicAlliance);
};

class type_AI_creature_purchaser : public type_AI_creature_swapper {
public:
    type_AI_creature_purchaser(long player, town* currentTown);
    void set(town* currentTown);
    void set(TCreatureType newType, short* newAmount);
    void setSubtractMode(unsigned char mode) { m_subtractCostMode = mode; }
    void doPurchase(armyGroup* newArmy, short newMorale,
                     armyGroup* newAdjacentArmy, long* newFunds,
                     unsigned char allowTrade,
                     unsigned char newHasAngelicAlliance);
protected:
    long m_playerId;
    long* m_funds;
    unsigned char m_subtractCostMode;
    std::vector<type_creature_source> m_creatures;
    long doBestPurchase(unsigned char tradeAllowed);

public:
    long getPurchaseValue(const armyGroup* newArmy, short newMorale,
                          const armyGroup* newAdjacentArmy,
                          const long* newFunds,
                          unsigned char newHasAngelicAlliance);
};

void aiConsolidateArmy(armyGroup& currentArmy);

#endif
