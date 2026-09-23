// Mac declaration view for source-owned philai.cpp bodies. The offsets below
// are observed in the bounded Mac target bodies at 0:0x13f074, 0:0x141bf8,
// and 0:0x13dce4.
// This view projects Mac layouts and includes canonical source inlines.
#ifndef HOMM3_MAC_PHILAI_H
#define HOMM3_MAC_PHILAI_H

#define HOMM3_TARGET_MAC 1
#include "va.h"
#include "artifact_type.h"
#define __forceinline inline
typedef long long __int64;
enum TCreatureType { CREATURE_NONE = -1 };
#include "herospec.h"

enum EGameResource { WOOD = 0, GOLD = 6 };
enum { HERO_BACKPACK_CAPACITY = 64 };
enum type_building_id {
    MAGE_GUILD_ID = 0,
    BLACKSMITH_ID = 16,
    SPECIAL_BUILDING_ID = 17,
    EXTRA_0_ID = 21,
    EXTRA_1_ID = 22,
    EXTRA_2_ID = 23,
    HOLY_GRAIL_ID = 26,
    DWELLING_0_UPG_ID = 37
};
enum TTownType {
    TOWN_TOWER = 2,
    TOWN_INFERNO = 3,
    TOWN_DUNGEON = 5,
    TOWN_STRONGHOLD = 6,
    TOWN_FORTRESS = 7,
    TOWN_CONFLUX = 8
};
enum { NUM_RESOURCES = 7, TOWN_DWELLING_COUNT = 7 };
typedef int SpellID;

struct type_artifact {
    TArtifact m_artifactId;
    int m_extra;
#include "type_artifact_constructors.inl"
};

struct armyGroup {
    enum { ARMY_GROUP_SLOT_COUNT = 7 };
    TCreatureType m_armyTypes[7];
    int m_numTroops[7];
    int getCreatureTotal() const;
    long getAIValue() const;
};

class mouseManager {
public:
    char m_beforeHideCount[0x68];
    int m_hideCount;
#include "inline/mousemgr_is_vis.inl"
    void showPointer(bool restore);
};
extern mouseManager* g_mouseManager;

#pragma options align=packed
class hero {
public:
    enum { kPatrolNone = 0xff };
    unsigned char hasArtifact(int whichArtifact) const;
    unsigned char removeArtifact(TArtifact artifact);
    long getNumberInBackpack(unsigned char countWarMachines) const;
    unsigned char giveArtifact(const type_artifact* artifact,
                               unsigned char announce, unsigned char checkEnd);
#include "inline/hero_get_primary_skill.inl"
#include "inline/hero_get_value_of_power.inl"
#include "inline/hero_get_value_of_knowledge.inl"
    unsigned char isWieldingArtifact(int artifact) const;
    static int getExperienceIncrement(int level);
    // Mac getSkillValue reads the troop IDs at +0x91, counts at +0xad,
    // mastery bytes at +0xc9, and skill count at +0x101. Mac getHero
    // indexes these records at stride 0x486.
    short m_x;
    short m_y;
    short m_z;
    char m_beforeId[0x1a - 0x06];
    int m_id;
    char m_beforeOwner[0x22 - 0x1e];
    signed char m_owner;
    char m_beforePatrol[0x44 - 0x23];
    unsigned char m_patrolX;
    unsigned char m_patrolY;
    signed char m_patrolRadius;
    char m_beforeMovePoints[0x4d - 0x47];
    int m_movePoints;
    int m_experience;
    short m_level;
    char m_beforeArmy[0x91 - 0x57];
    armyGroup m_army;
    signed char m_skillLevel[28];
    char m_beforeSkillCount[0x101 - 0xe5];
    int m_skillCount;
    char m_beforeExperienceRatio[0x109 - 0x105];
    float m_turnExperienceToRvRatio;
    char m_beforeSleeping[0x11c - 0x10d];
    unsigned char m_isSleeping;
    char m_beforeStats[0x46a - 0x11d];
    signed char m_stats[4];
    char m_beforePower[0x472 - 0x46e];
    long m_valueOfPower;
    char m_beforeKnowledge[0x47a - 0x476];
    long m_valueOfKnowledge;
    char m_afterKnowledge[0x486 - 0x47e];
};
#pragma options align=reset

// The Mac trait row is 0x74 bytes; its seven cost dwords begin at +0x20.
struct TCreatureTypeTraits {
    char m_beforeAttributes[0x10];
    unsigned int m_attributes;
    char m_beforeCost[0x20 - 0x14];
    int m_cost[7];
    char m_beforeAiValue[0x40 - 0x20 - 7 * sizeof(int)];
    int m_aiValue;
    char m_afterAiValue[0x74 - 0x44];
};
extern const TCreatureTypeTraits (&g_creatureTypeTraits)[150];
extern TCreatureType g_townDwellingCreatures[126];
extern TCreatureType g_townUpgradedDwellingCreatures[126];

// Mac player resources begin at +0x98. The resource-value doubles begin at
// +0x11c; the canonical AI field order places them +0x34 into the subobject.
struct AI {
    char m_beforeResourceValue[0x34];
    double m_resourceValue[7];
    int m_averageResourceValue;
    float m_turnValueOfAvgArtifact;
};
class town;
// CodeWarrior vector has capacity, size, data in three words; Mac getTown
// reads the data word at game+0x20a30, eight bytes into m_towns.
namespace std {
template<class T> class vector {
    unsigned long m_capacity;
    unsigned long m_size;
    T* m_data;
public:
    T& operator[](unsigned long index) { return m_data[index]; }
};
}
struct playerData {
    unsigned char addGarrisonHero(town* ourTown);
    char m_beforeNumHeroes;
    signed char m_numHeroes;
    char m_beforeCurrHeroId[2];
    int m_currHeroId;
    int m_heroes[8];
    char m_beforeResources[0x98 - 8 - 8 * sizeof(int)];
    long m_resources[7];
    char m_beforeAI[0xe8 - 0x98 - 7 * sizeof(long)];
    AI m_ai;
};
extern playerData* g_currentPlayer;

extern __int64 g_bitNumber[];

#pragma options align=packed
class town {
public:
    char m_id;
    signed char m_owner;
    char m_beforeType[2];
    signed char m_type;
    char m_beforeGarrison[0xc - 0x5];
    int m_garrisonHeroId;
    int m_visitingHeroId;
    char m_beforeBuilt[0x144 - 0x14];
    __int64 m_built;
    __int64 m_active;
    char m_afterActive[0x15c - 0x154];

    unsigned char isLegalBuilding(type_building_id building) const;
    unsigned char canBuild(short buildingId) const;
    unsigned char buyBuilding(type_building_id building);
    int* getBuildCostArray(type_building_id building) const;
    type_building_id buildBuilding(int buildingId, unsigned char setBuiltFlag,
                                   unsigned char soundFlag);
    void giveSpells(hero* forceHero) const;
    void applySpecialBuildingEffect(hero* townHero);
    const armyGroup& getArmy() const;
    void swapHeroes();
#include "inline/town_has_building.inl"
};
#pragma options align=reset

struct VictoryConditionStruct {
    unsigned char checkForGrailBuildingWin();
};
struct MacMapHeader {
    // moveHero's inlined GetTeam reads the signed team byte at +0x0d.
    char m_beforeTeams[0x0d];
    signed char m_teamInfo[8];
    char m_beforeVictory[0x2c - 0x15];
    VictoryConditionStruct m_victoryCondition;
};

// The Mac game pointer's setup difficulty byte is at effective +0x1ef64.
struct gameSetup {
    signed char m_difficulty;
};
// Mac NewfullMap has thirteen 12-byte vector members, then its cell pointer
// and size. The level flag is therefore +0xa4; moveHero reads it at
// game+0x1f464, placing m_worldMap at game+0x1f3c0.
struct NewfullMap {
    char m_beforeHasTwoLevels[0xa4];
    unsigned char m_hasTwoLevels;
    int getNumLevels();
};
#include "inline/mapcell_get_num_levels.inl"
struct game {
    char m_beforeSetup[0x1ef64];
    gameSetup m_setup;
    char m_beforeMapHeader[0x1f0f8 - 0x1ef65];
    MacMapHeader m_mapHeader;
    char m_beforeWorldMap[0x1f3c0 - 0x1f0f8 - sizeof(MacMapHeader)];
    NewfullMap m_worldMap;
    char m_beforePlayers[0x1ff48 - 0x1f3c0 - sizeof(NewfullMap)];
    playerData m_players[8];
    // Mac getTown reads vector data at game+0x20a30; getHero forms
    // game+0x20a34 + which*0x486 immediately after the 12-byte vector.
    std::vector<town> m_towns;
    hero m_heroes[156];
#include "game_get_hero.inl"
    int getTownId(int x, int y, int z);
#include "inline/game_get_town.inl"
#include "inline/game_get_curr_hero.inl"
#include "inline/game_get_num_map_levels.inl"
#include "game_get_team.inl"
    bool isHumanTeam(int teamNum) const;
    bool isHumanAlly(int playerNum) const;
    bool townAlreadyBuiltOn(int townId) const;
};
extern game* g_game;
#include "game_is_human_ally.inl"

// The Mac artifact-trait table has 0x20-byte rows and disabled at +0x1c.
struct TArtifactTraits {
    char m_beforeDisabled[0x1c];
    unsigned char m_disabled;
    char m_afterDisabled[3];
};
extern const TArtifactTraits (&g_artifactTraits)[144];

extern int g_curHourGlassPhase;
extern int g_unnamed691680;

class philAI {
public:
    void getTurnAIVars(int whichPlayer);
};
class type_AI_player {
public:
#include "ai_player_set_attack_bonuses.inl"
    void buyCreatures(hero* currentHero, town* currentTown);
    void buyMageGuild(hero* currentHero, town* currentTown);
    // Estate valuation loads GOLD's value at player +0x8c and indexes
    // g_aiPlayers with a 0x94-byte stride, so the seven doubles start +0x5c.
    char m_beforeResourceValue[0x5c];
    double m_resourceValue[7];
#include "ai_player_get_resource_value.inl"
    static float s_attackHumanBonus;
    static float s_attackComputerBonus;
};
extern type_AI_player g_aiPlayers[8];
struct type_university {
    TSecondarySkill m_skills[4];
    type_university* initializeMagicSkills();
};
class advManager {
public:
    char m_beforeAdvWindow[0x44];
    class TAdventureMapWindow* m_advWindow;
    void demobilizeCurrHero(unsigned char waitingPlayer, unsigned char update);
    void setHeroContext(int heroId, int inMove,
                        unsigned char waitingPlayer,
                        unsigned char drawChanges);
};
extern advManager* g_advManager;
class TAdventureMapWindow {
public:
    void animateBottomView(unsigned char inBackground);
};
class searchArray {
public:
    char m_beforeDangerZones[0x60];
    long* m_dangerZones;
#include "inline/search_set_danger_zones.inl"
};
extern searchArray* g_searchArray;
extern int g_gameOver;
extern int g_unnamed69ccd4;
// DC MoveHero reads ?gConfig@@3UconfigStruct@@A+0x38; the Mac body does
// likewise through TOC 1+0x5c4. Complete owns the equivalent standalone
// visibility flag at g_unnamed698790 (0x698790).
struct MacConfig {
    char m_beforeVisibilityFlag[0x38];
    int m_visibilityScanSuppressed;
};
extern MacConfig g_config;
#define g_unnamed698790 (g_config.m_visibilityScanSuppressed)
extern int g_videoPaused;
extern unsigned char g_mapVisibilityBit;
extern int g_completeDrawEnabled;
extern int g_netLocalGamePos;
extern int g_mapWidth;
extern int g_mapHeight;
int mapExtraPosAndAdjacentsSet(int x, int y, int z, unsigned char bit);
void aiMarkDangerZones(hero* currentHero, long* dangerZones);
static void checkForTown(hero* currentHero);
static void markShipyards(playerData* player);
static void clearShipyards(playerData* player);
static void restoreMouse(unsigned char mouseWasVisible);
static void moveHero(hero* currentHero, unsigned char isLastHero,
                     unsigned char& exploreMode);
extern type_artifact g_blacksmithArtifacts[9];
void checkEndGame(int forceWin);
static void considerGarrisoning(hero* currentHero, town* currentTown);
static void upgradeCreatures(hero* currentHero, const town* currentTown);
static void buySpecialBuilding(const hero* currentHero, town* currentTown);
void buyArtifacts(hero* currentHero, town* currentTown);
void buySiegeEngine(hero* currentHero, town* currentTown,
                    type_building_id building, TArtifact engine);
void aiSwapArtifacts(hero* source, hero* destination);
void aiVisitUniversity(hero* currentHero, type_university* university);
int aiResourceCost(const playerData* player, const int* resources);
long valueOfUniversity(const hero* currentHero,
                        type_university* university,
                        unsigned char mustPay);
static long getSchoolValue(const hero* ourHero, TSecondarySkill skill);
long getSkillValue(const hero* ourHero, TSecondarySkill skill,
                   unsigned char complexChoice);
void aiSetHeroBonuses(hero* ourHero);
long aiGetValueOfArtifact(const type_artifact& artifact, long playerId);
extern "C" void* memcpy(void*, const void*, unsigned long);
// Mac moveHero lowers the zero-filled danger map to the two-argument bzero.
extern "C" void bzero(void*, unsigned long);
#define memset(destination, fill, count) bzero(destination, count)

long aiGetValueOfArtifact(type_artifact artifact, const hero* owner,
                          unsigned char equipped, unsigned char exact);
TCreatureType siegeArtifactToCreature(TArtifact engine);
static long getArtifactPurchaseValue(TArtifact artifactId, long marketCount,
                                     long* funds);
long getArtifactPurchasePrice(TArtifact artifact, long marketCount,
                              EGameResource* bestResource);
void aiEquipArtifacts(hero* currentHero);
void aiEnterTown(hero* currentHero, town* currentTown);

#endif
