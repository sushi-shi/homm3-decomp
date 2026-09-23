// Mac declaration view for source-owned AI bodies in src/ai.cpp.
// Offsets used here are evidenced by the paired AI bodies at code 0+0x223e4,
// 0+0x22cb8, and 0+0x245e4, plus their retained callees.
#ifndef HOMM3_MAC_AI_H
#define HOMM3_MAC_AI_H

#define HOMM3_TARGET_MAC 1

#include "creature_flags.h"
#include "artifact_type.h"
#include "va.h"

// Mac 0+0x21d28 and 0+0x21dd8 pass destination and byte count to the
// retained zero-fill routine at 0+0x26ad3c. AI's source memset uses zero at
// every call site; this view exposes the platform's two-argument ABI.
extern "C" void bzero(void*, unsigned long);
#define memset(destination, fill, count) bzero(destination, count)

// The retained MSL vector has capacity, size, and data at +0/+4/+8. Its
// constructor calls the one-word compressed-pair constructor for capacity,
// then stores zero size and data in the caller; clear writes zero size.
namespace std { template<class T> class allocator {}; }
namespace Metrowerks {
namespace details {
template<class Allocator, class Size, int Version> class compressed_pair_imp;
template<class Allocator, class Size>
class compressed_pair_imp<Allocator, Size, 1> : private Allocator {
    Size m_second;
public:
    explicit compressed_pair_imp(Size value) : m_second(value) {}
};
}
template<class Allocator, class Size>
class compressed_pair
    : private details::compressed_pair_imp<Allocator, Size, 1> {
    typedef details::compressed_pair_imp<Allocator, Size, 1> base;
public:
    explicit compressed_pair(Size value) : base(value) {}
};
}
namespace std {
template<class T> class __vector_deleter {
protected:
    Metrowerks::compressed_pair<allocator<T>, unsigned long> m_capacity;
    unsigned long m_size;
    T* m_data;
    __vector_deleter() : m_capacity(0), m_size(0), m_data(0) {}
};
template<class T> class __vector_imp : private __vector_deleter<T> {
protected:
    __vector_imp() {}
    void clear() { this->m_size = 0; }
    unsigned long size() const { return this->m_size; }
    T& operator[](unsigned long index) { return this->m_data[index]; }
};
template<class T> class vector : private __vector_imp<T> {
public:
    vector() {}
    ~vector();
    void clear() { __vector_imp<T>::clear(); }
    unsigned long size() const { return __vector_imp<T>::size(); }
    T& operator[](unsigned long index) { return __vector_imp<T>::operator[](index); }
    void push_back(const T& value);
};
}

enum {
    CREATURE_MASTER_GENIE = 0x25,
    CREATURE_OGRE_MAGE = 0x5b,
    CREATURE_HARPY = 0x48,
    CREATURE_HARPY_HAG = 0x49,
    CREATURE_BALLISTA = 0x92,
    CREATURE_ARROW_TOWER = 0x95,
    ARMY_CREATURE_FIRST_AID_TENT = 0x93,
    ARMY_CREATURE_AMMO_CART = 0x94,
    COMBAT_GRID_CELLS = 0xbb,
    COMBAT_GRID_ROW_STRIDE = 17,
    COMBAT_GRID_LAST_COLUMN = 0x10,
    VICTORY_CONDITION_DEFEAT_HERO = 5,
    TAVERN_ID = 5,
    SPECIAL_BUILDING_ID = 17,
    TOWN_STRONGHOLD = 6,
    GOLD = 6
};

enum TArtifactSlot { MAC_ARTIFACT_SLOT_PLACEHOLDER = 0 };
typedef int SpellID;

struct type_artifact {
    TArtifact m_artifactId;
    int m_extra;
#include "mac_shared/type_artifact_constructors.h"
};

struct TArtifactTraits {
    const char* m_name;
    int m_cost;
    char m_tail[0x20 - 8];
};
extern const TArtifactTraits (&g_artifactTraits)[144];
extern long long g_bitNumber[];
extern const unsigned char g_castleWallColumns[];
long aiGetValueOfArtifact(const type_artifact& artifact, long playerId);
#include "mac_shared/cpp_max.h"
#include "mac_shared/max_int.h"

int random(int min, int max);
class army;
class hero;
class combatManager;
class searchArray;
extern searchArray* g_searchArray;
void findAttackHexes(const army* ourArmy, long targetHex, long start,
                     long stop, long limitCost,
                     const searchArray* currentSearchArray,
                     std::vector<long>* result);

struct MacVictoryCondition {
    signed char m_type;
    char m_beforeHeroId[0x34 - 1];
    int m_heroId;
    char m_tail[0x4c - 0x38];
};
struct MacMapHeader {
    char m_beforeVictory[0x2c];
    MacVictoryCondition m_victoryCondition;
};
struct MacGameSetup {
    signed char m_difficulty;
};
struct playerData {
    char m_beforeNumTowns[0x3e];
    signed char m_numTowns;
    char m_beforeTownIds;
    signed char m_townIds[0x48];
    char m_beforeResources[0x98 - 0x88];
    long m_resources[7];
    char m_tail[0x15c - 0xb4];
};

#pragma options align=packed
class town {
public:
    char m_id;
    signed char m_owner;
    char m_beforeType[2];
    signed char m_type;
    char m_beforeBuilt[0x144 - 0x5];
    long long m_built;
    long long m_active;
    char m_tail[0x15c - 0x154];
#include "mac_shared/town_has_building.h"
};

class hero {
public:
    char m_beforeId[0x1a];
    int m_id;
    char m_beforeOwner[0x22 - 0x1e];
    signed char m_owner;
    char m_beforeExperience[0x50 - 0x23];
    int m_experience;
    char m_beforeArtifacts[0x12d - 0x54];
    type_artifact m_equipped[19];
    unsigned char m_artifactSlotCounts[15];
    type_artifact m_backpack[64];
    unsigned char isWieldingArtifact(int artifact) const;
#include "mac_shared/hero_get_artifact.h"
#include "mac_shared/hero_get_backpack.h"
};
#pragma options align=reset

class game {
public:
    // chooseMeleeTarget's second pre-map gate reads this word at +0x1ef24.
    char m_beforeVersion[0x1ef24];
    int m_gameVersion;
    char m_beforeSetup[0x1ef64 - 0x1ef28];
    MacGameSetup m_setup;
    char m_beforeMapHeader[0x1f0f8 - 0x1ef65];
    MacMapHeader m_mapHeader;
    char m_beforePlayers[0x1ff48 - 0x1f0f8 - sizeof(MacMapHeader)];
    playerData m_players[8];
    char m_beforeTowns[0x20a30 - 0x1ff48 - 8 * sizeof(playerData)];
    town* m_towns;
#include "mac_shared/game_get_town.h"
};
extern game* g_game;

struct type_AI_enemy_data {
    const army* m_enemy;
    long m_damage;
    long m_count;
    long m_totalDamage;
};

struct type_AI_combat_parameters {
    // chooseCreatureSpell reads +0/+4/+0x20 in Mac retail.
    long m_lowestAttack;
    long m_lowestDefense;
    unsigned char m_killsOnly;
    unsigned char m_simulated;
    char pad[0x20 - 10];
    long m_ourGroup;
    long m_enemyGroup;
    type_AI_combat_parameters(const combatManager* combat, long side);
    long getSimpleAttackEffect(const army& currentArmy, const army& enemy,
                               unsigned char ranged, long distance) const;
#include "mac_shared/ai_parameters_get_group.h"
    long getEnemyGroup() const { return m_enemyGroup; }
};

// The retained chooser constructor at 0+0x3d9ac and AI callers use this
// 0x2c-byte record and its three canonical inline accessors.
struct type_AI_attack_hex_chooser {
    const army* m_attackArmy;
    long m_speed;
    const army* m_enemyArmy;
    searchArray* m_searchData;
    const long* m_enemyAttackArray;
    long m_enemyTroopsLeft;
    long m_ourTroops;
    long m_bestValue;
    long m_bestHex;
    long m_bestAttackTime;
    const type_AI_combat_parameters* m_data;
    type_AI_attack_hex_chooser(const army* attacker, const army* defender,
                               const long* attackArray, searchArray* search,
                               const type_AI_combat_parameters* combatData);
    unsigned char findAttackHex();
#include "mac_shared/ai_attack_chooser_get_attack_time.h"
#include "mac_shared/ai_attack_chooser_get_best_hex.h"
#include "mac_shared/ai_attack_chooser_get_hex_value.h"
};

struct type_AI_spellcaster {
    // The canonical ai_tactical.h fields total 0x410 including the vptr.
    // The Mac ctor at 0+0x3e330 confirms +0, +0x20, +0x48, +0x4c and sizeof.
    hero* m_ourHero;
    hero* m_enemyHero;
    long m_side;
    long m_enemySide;
    long m_enemyCanAttack;
    long m_canBeAttacked;
    unsigned char m_winLikely;
    unsigned char m_isCreatureSpell;
    char m_paddingBeforeEstimate[2];
    type_AI_combat_parameters m_estimate;
    type_AI_spellcaster* m_enemyCaster;
    unsigned char m_ownsEnemyCaster;
    char m_paddingBeforeMeleeEnemies[3];
    type_AI_enemy_data m_meleeEnemies[20];
    type_AI_enemy_data m_attacks[20];
    type_AI_spellcaster(combatManager* combat, long side,
                        unsigned char creatureSpell);
    virtual ~type_AI_spellcaster();
    long getCaliphValue(const army* target) const;
    long getOgreMageValue(const army* target) const;
    long getFaerieDragonSpellValue(long hex, long power, SpellID spell);
    type_AI_enemy_data m_worstEnemies[20];
};

#pragma options align=packed

class hexcell {
public:
    short m_refX;
    char pad[0x4a - 2];
    unsigned char m_validMove;
    char padAfterMove[0x70 - 0x4b];

    army* getArmy() const;
};

struct pathCell {
    char pad0[4];
    unsigned int m_visited : 1;
    unsigned int padBits : 31;
    char pad1[0x18 - 8];
    unsigned short m_cost;
    char pad2[0x1e - 0x1a];
};

class searchArray {
    char pad[0x24];
    pathCell* m_cellData;
public:
#include "mac_shared/search_get_hex.h"
    long getTravelTime(const army* currentArmy, long hex) const;
    void seedCombatPosition(const army* thisArmy, long currentGroup,
                            long limit, unsigned char inPlacementPhase,
                            long baseSpeed);
    void markTeleport(const army* currentArmy, long currentGroup);
};

class army {
public:
    char pad0[0x10];
    int m_side;
    int m_slot;
    char pad1[0x34 - 0x18];
    int m_creatureType;
    int m_gridIndex;
    char pad2[0x44 - 0x3c];
    int m_facing;
    char pad3[0x4c - 0x48];
    int m_numTroops;
    char padBeforeTopDamage[0x58 - 0x50];
    int m_topCreatureDamage;
    char padBeforeMonInfo[0x74 - 0x5c];
    struct {
        char pad[0x10];
        unsigned m_attributes;
        char padBeforeBaseFightValue[0xb0 - 0x88];
        int m_baseFightValue;
    } m_monInfo;
    char pad4[0x190 - 0xb4];
    int m_expectedMoveOrder;
    char padBeforeSpells[0x198 - 0x194];
    int m_spellInfluence[81];
    char padBeforeFaerieSpell[0x4cc - (0x198 + 81 * 4)];
    int m_faerieDragonSpell;
    char pad5[0x514 - 0x4d0];
    army* m_aiTarget;
    long m_aiTargetValue;
    long m_aiTargetTime;
    unsigned m_aiPossibleTargets;

    unsigned char canShoot(const army* excluded) const;
    bool is(unsigned attribute) const;
    int offsetToFront(int direction) const;
    long getAdjacentHex(long hex, long direction) const;
    int getSecondGridIndex() const;
    unsigned char isAdjacent(int hex) const;
    unsigned char canCastSpell(long hex) const;
    long getTotalCombatValue(long lowestAttack, long lowestDefense) const;
    double computeDefenderDamageReduction(unsigned char isShooting) const;
    long getLossCombatValue(long lowestAttack, long lowestDefense,
                            unsigned char ranged, long damage,
                            unsigned char killsOnly) const;
    bool isIncapacitated() const;
    bool cannotAttack() const;
    const army* getAITarget() const;
    long getAITargetTime(long speed) const;
    long getAITargetTime() const;
    long getSpellTime(int spell) const;
    void clearAIValues();
    inline bool isActive() const;
    int getSpeed() const;
    long getMultiHeadDirections(long ourHex, const army* enemy,
                                long enemyHex) const;
    long getTotalHitPoints(unsigned char current) const;
    void considerAttack(const army* enemy, long value, long attackDistance);
};

#include "mac_shared/army_is.h"
#include "mac_shared/army_offset_to_front.h"
#include "mac_shared/army_get_ai_target.h"
#include "mac_shared/army_get_ai_target_time.h"
#include "mac_shared/army_is_incapacitated.h"
#include "mac_shared/army_cannot_attack.h"
#include "mac_shared/army_get_spell_time.h"
#include "mac_shared/army_clear_ai_values.h"
#include "mac_shared/army_is_active.h"

class combatManager {
public:
#include "mac_shared/combat_valid_hex.h"
    char pad0[0x3c];
    int m_nextAction;
    int m_nextActionExtra;
    int m_nextActionGridIndex;
    char pad1[0x1c4 - 0x48];
    hexcell m_cells[COMBAT_GRID_CELLS];
    char padCells[0x53c8 - (0x1c4 + COMBAT_GRID_CELLS * 0x70)];
    town* m_defendingTown;
    hero* m_heroes[2];
    char padBeforeSideIsAi[0x54a4 - 0x53d4];
    unsigned char m_sideIsAi[2];
    char padBeforePlayerIds[0x54a8 - 0x54a6];
    int m_playerIds[2];
    char padBeforeNumArmies[0x54bc - 0x54b0];
    // chooseCreatureSpell reads +0x54bc + side*4 before scanning m_armies.
    int m_numArmies[2];
    char padArmies[0x54cc - 0x54c4];
    army m_armies[2][21];
    char pad2[0x12cd0 - (0x54cc + 2 * 21 * 0x524)];
    int m_actingSide;
    int m_actingSlot;
    int m_currentSide;
    char pad3[0x12ce0 - 0x12cdc];
    army* m_lastMovedArmy;
    char padBeforeFortification[0x132f4 - 0x12ce4];
    int m_fortificationLevel;
    char padBeforePlacement[0x1377c - 0x132f8];
    unsigned char m_creaturePlacement;

    void doCompAI(int whichGroup);
    unsigned char chooseDefenseHex(const army* currentArmy, const army* client,
                                   long* bestHex, long* openHexes,
                                   searchArray* currentSearchArray);
    unsigned char chooseCreatureSpell(const army* currentArmy, long* bestValue,
                                      type_AI_combat_parameters* estimate);
    bool sodChooseFaerieDragonSpell(const army* currentArmy, long& bestValue,
                                    type_AI_combat_parameters& estimate);
    void turnOffHighlighter(unsigned char restore);
#include "mac_shared/combat_get_current_army.h"
    void placeShooter(const army* currentArmy);
    void chooseShooterAction(const army* currentArmy,
                             unsigned char simulated, long side);
    long chooseMeleeAction(const army* currentArmy, unsigned char teleport,
                           unsigned char simulated, long side);
    unsigned char chooseMeleeTarget(const army* currentArmy,
                                    unsigned char teleport, long* actionValue,
                                    type_AI_combat_parameters* estimate);
    void markFirewalls(const army* currentArmy, long* enemyAttacks,
                       type_AI_combat_parameters* estimate);
    void markMoat(const army* currentArmy, long* enemyAttacks,
                  type_AI_combat_parameters* estimate);
    void markEnemyAttacks(const army* currentArmy, long* enemyAttacks,
                          long* markedEnemies,
                          type_AI_combat_parameters* estimate) const;
    void markFriendlyArmies(const army* currentArmy, long* enemyAttacks,
                            long markedEnemies,
                            const type_AI_combat_parameters* estimate) const;
    unsigned char shouldStayInCastle(type_AI_combat_parameters* estimate);
    static unsigned char inCastle(int hex);
    long getAttackChange(const army* currentArmy, const army* enemy,
                         type_AI_combat_parameters& data);
    unsigned char hasRangedAdvantage(type_AI_combat_parameters* estimate);
    unsigned char chooseSpellAction(const army* currentArmy, long* actionValue,
                                    type_AI_combat_parameters* estimate);
    unsigned char moveToward(const army* currentArmy, long targetHex,
                             const long* enemyAttacks,
                             unsigned char considerWaiting);
    unsigned char isInMoat(int hex, int* relative);
    long getAreaEffect(long side, const army* currentArmy,
                       long markedEnemies,
                       const type_AI_combat_parameters* estimate) const;
    unsigned char attemptShooterDefense(
            const army* currentArmy, searchArray* currentSearchArray,
            const type_AI_combat_parameters* estimate);
    unsigned char chooseToRun(const army* currentArmy,
                              const long* enemyAttacks,
                              const searchArray* currentSearchArray);
#include "mac_shared/combat_grid_y.h"
#include "mac_shared/combat_grid_x.h"
#include "mac_shared/combat_in_invisible_column.h"
    int chooseBallistaTarget(int targetGroup, int attackSkill, int averageDamage);
    void findAITargets(long ourGroup, const army* currentArmy,
                       unsigned char meleeOnly,
                       const type_AI_combat_parameters* data,
                       searchArray* currentSearchArray);
    unsigned char aiCheckRetreat();
    void markMultiheadedEnemy(const army* ourArmy, const army* enemy,
                              long* enemyAttacks, long limitValue,
                              searchArray* currentSearchArray,
                              type_AI_combat_parameters* estimate) const;
    unsigned char failedSiege();
    long getSurrenderCost();
    void simulateCombat(long side, unsigned char simulated);
};
#pragma options align=reset

#endif
