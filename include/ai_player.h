// ai_player.h - prototypes of ai_player.cpp (compiland ai_player.obj)
#ifndef HOMM3_AI_PLAYER_H
#define HOMM3_AI_PLAYER_H

#include <vector>
#include "netmsg.h"
#include "turn_update_msg.h"
#include "armygrp.h"
#include "ai_creature_value.h"

class hero;
class playerData;
class searchArray;
class town;
class generator;
struct type_artifact;

// E:\gamedcs\ai_player.cpp:3013, dc 0x329f8
void aiMarkDangerZones(hero* currentHero, long* dangerZones);

// Five-entry AI hero caps indexed by game difficulty. Dreamcast names both
// compiland statics; retail hire_heroes proves these corresponding addresses.
DATA(0x00660518) extern int g_heroLimits[5];
DATA(0x0066052c) extern int g_globalLimits[5];

long aiGetValueOfArtifact(type_artifact artifact, const hero* owner,
                              unsigned char equipped, unsigned char exact);
long aiGetValueOfArtifact(const type_artifact& artifact, long playerId);
void aiSwapArtifacts(hero* source, hero* destination);
long aiGetEquipValue(type_artifact artifact, const hero* ourHero,
                        unsigned char exact);
// This overload values the artifact across a player's heroes. CodeView
// proves the const reference and long player id; retail retains 0x433aa0.
// E:\gamedcs\ai_player.cpp:5684, dc 0x37514
long aiGetValueOfArtifact(const type_artifact& artifact, long playerId);
long getFullValue(const hero* ourHero);
long removeNegativeArtifacts(hero* ourHero);
long aiGetShipCost(const hero* ourHero, type_point point);

// Full DC layout (classes.csv: 152 B, 6 members, 2 statics) and every
// offset is corroborated by a retail reader: reset_magus_hut_value
// (0x429ab0) reads the short at +0 and writes the long at +4, and
// get_total_value (0x42a150) walks the seven doubles from +0x60 with
// `add esi, 8` after biasing `this` by 0x60.

// The two statics carry their DC public names verbatim
// (?attack_computer_bonus@type_AI_player@@1MA at DC seg3 0x26c0,
// ?attack_human_bonus@ at 0x26c4); retail holds the same adjacent pair
// at 0x6604f8 / 0x6604fc, both initialised to 0.5f, and philai.obj's
// set_attack_bonuses(float computer_bonus, float human_bonus) names the
// order. They are DEFINED by philai.cpp, not here.
class type_AI_player {
public:
    // DC ai_player.h:263 (dc 0x37dec). Complete inlines the helper into
    // AI_initialize; retail leaves exactly the team-word store.
    void init(short newTeam) { m_team = newTeam; }
    static float getAttackBonus(short player);  // 0x428710

protected:
    short m_team;
    long m_magusHutValue;
    long m_reservedFunds[7];
    long m_resourceSupply[7];
    long m_resourceDemand[7];
    double m_resourceValue[7];

public:
    void calculateDemand();  // 0x428740
    void endTurn();  // 0x428dd0
    long getMagusHutValue() const { return m_magusHutValue; }

    // DC ai_player.h:273-274, dc 0x37df0: clear the cached value.
    void clearMagusHutValue() { m_magusHutValue = 0; }
    // DC ai_player.h:278 (dc 0x37df8, ?...@@QBANW4EGameResource@@@Z);
    // inlined into type_income_artifact::get_value, whose by-value double
    // return temp at [ebp-8] is what the retail bytes home under /Op.
    double getResourceValue(enum EGameResource resource) const
    {
        return m_resourceValue[resource];
    }
    void startTurn();  // 0x4297c0
    static void setAttackBonuses(float computerBonus,
                                   float humanBonus)
    {
        s_attackComputerBonus = computerBonus;
        s_attackHumanBonus = humanBonus;
    }

protected:
    void makeGift(long playerId);  // 0x429110

public:
    void buyCreatures(hero* currentHero, town* currentTown);  // 0x42ba60
    void buyMageGuild(hero* currentHero, town* currentTown);  // 0x42beb0
    bool hireHeroes();
    void resetMagusHutValue();  // 0x429ab0
    void tradeResources(const int* cost, long number);

protected:
    bool buildMarkets(int* supply);
    void calculateReserve();  // 0x429ad0
    bool canTradeResources(const int* cost, int* supply,
                             std::vector<long>& tradeQty);
    bool checkTradeSupply(const int* cost, long number, int* supply,
                            std::vector<long>& tradeQty);
    void doResourceTrade(int* supply);
    long getTotalValue(long basicValue, int* cost);  // 0x42a150
    // DC LF_ONEMETHOD protected; retail 0x42ae00 (the per-town pricing
    // pass purchase_buildings drives).
    unsigned char purchaseBuilding(unsigned char* prohibitedCreatures);
    // DC ?purchase_buildings@type_AI_player@@IAAXXZ: ordinary protected
    // helper; the prohibited-creature array belongs to its body.
    void purchaseBuildings();
    static float s_attackHumanBonus;
    static float s_attackComputerBonus;
};

// Retail .bss 0x692950, eight adjacent 152-byte AI records. make_gift
// recalculates the recipient's demand through this array before giving
// resources to another computer player. Owner TU remains unlocated.
extern type_AI_player g_aiPlayers[8];

// Dreamcast records this exact 12-byte value object, and retail's
// constructor at 0x4286b0 writes the same four fields at 0/4/8/10.
struct type_creature_source {
public:
    VA(0x004286b0, 0x21)  // DC signature/layout + retail stores; dc 0x37e08
    type_creature_source(TCreatureType newType, short* newAmount,
                         bool isFree)
        : m_type(newType), m_ptr(newAmount), m_isFree(isFree)
    {
        m_number = *newAmount;
    }
    TCreatureType m_type;
    short* m_ptr;
    short m_number;
    unsigned char m_isFree;
};
SIZE(type_creature_source, 12);

// The DC field roster proves the seven base members and the purchaser's
// four-member tail. Retail adds the byte at +8 (do_purchase writes the
// Angelic-Alliance result there), which moves morale/alignment_count to
// +0xa/+0xc while leaving the 32-byte base extent unchanged. GetAlignments
// proves the ten-byte array at +0xe; the gap after the alliance byte and the
// base's tail are ordinary VC6 alignment and remain implicit.
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

public:
    type_AI_creature_swapper();

protected:
    void addCreatures(TCreatureType type, short amount, short slot);
    long chooseWeakestArmy(unsigned char isShooter, unsigned char checkAlignments);
    long doBestSwap(bool canTakeAll);
    void dumpExtraCreature();
    long valueOfAddingArmy(TCreatureType type, short count,
                              short& slot, unsigned char mustReplaceCreature);

public:
    void doSwap(hero* currentHero, armyGroup* sourceArmy,
                 hero* secondHero,
                 unsigned char newHasAngelicAlliance);
    long getSwapValue(const hero* currentHero,
                        const armyGroup* sourceArmy,
                        const hero* secondHero,
                        unsigned char newHasAngelicAlliance);
    long getArmyIncrease() const { return m_armyValueIncrease; }
};
SIZE(type_AI_creature_swapper, 0x20);

class type_AI_creature_purchaser : public type_AI_creature_swapper {
public:
    type_AI_creature_purchaser(long player,
                               generator* currentGenerator);
    type_AI_creature_purchaser(long player, town* currentTown);
    type_AI_creature_purchaser(long player, TCreatureType type,
                               short* amount, bool isFree);
    void set(town* currentTown);
    // DC 0x31ffc (ai_player.cpp:2524): the single-candidate overload.
    // No retail out-of-line body (set(town) ends 0x42d418, next row
    // 0x42d420); every caller inlines its clear + one push_back.
    void set(TCreatureType newType, short* newAmount);

protected:
    long doBestPurchase(unsigned char tradeAllowed);
    long m_playerId;
    long* m_funds;
    unsigned char m_subtractCostMode;
    std::vector<type_creature_source> m_creatures;

public:
    void doPurchase(armyGroup* newArmy, short newMorale,
                     armyGroup* newAdjacentArmy, long* newFunds,
                     unsigned char allowTrade,
                     unsigned char newHasAngelicAlliance);
    long getPurchaseValue(const armyGroup* newArmy, short newMorale,
                            const armyGroup* newAdjacentArmy,
                            const long* newFunds,
                            unsigned char newHasAngelicAlliance);
    void setSubtractMode(unsigned char arg) { m_subtractCostMode = arg; }
};
SIZE(type_AI_creature_purchaser, 0x3c);

void aiConsolidateArmy(armyGroup& currentArmy);
// 0x42d8e0 - do_swap's tail call (0x42c485), also reached from
// buy_creatures (0x42bbae), split_armies (0x42dd47/5b) and 0x431d9d.
void aiArrangeArmy(armyGroup& currentArmy);

// DC classes.csv: 16 B - point, value(+4), move_cost(+8), is_nearby(+12),
// is_critical(+13). net_value_of_location (0x42f980) adjusts move_cost and
// sets is_critical; find_all_destinations vectors these records.
struct HeroDestination {
public:
    type_point m_point;
    long m_value;
    long m_moveCost;
    unsigned char m_isNearby;
    unsigned char m_isCritical;

    // CodeView dc 0x380f0 marks this default constructor compiler-generated
    // (compgenx). The type_point member supplies its implicit construction.
};

long findAllDestinations(hero* currentHero, searchArray* currentSearchArray,
                           std::vector<HeroDestination>& destinations,
                           long maxDistance, unsigned char hiringHero,
                           unsigned char allowSpells,
                           unsigned char exploreMode);
int netValueOfLocation(hero* currentHero, HeroDestination* destination,
                          long* strategicMap, struct pathCell* currentPathCell,
                          searchArray* currentSearchArray);
int aiChooseDestination(hero* currentHero, long maxDistance,
                          HeroDestination& bestPoint,
                          long& bestRawValue,
                          unsigned char allowSpells,
                          unsigned char exploreMode);

unsigned char canTakeTown(const hero* attackingHero, const town* defendingTown);
long findMagusHutValue(long playerId, unsigned char exploreMode);
void fillProhibitedArray(playerData* player, unsigned char* prohibited);

extern const char* g_resourceNames[7];
extern char g_aiResourceWarningFormat[];

// Retail .bss 0x693718, one byte per TAdventureObjectType.
// find_all_destinations tests it for each trigger destination and, when
// set, requires the friendly-distance map to cover the cell at least as
// cheaply - a "wait for backup" class of objects. No DC symbol reaches
// it; named in the gUnnamed69ccc4 style. ai_player.obj is the nearest
// admitted consumer.
DATA(0x00693718)
extern unsigned char g_unnamed693718[];

// Dreamcast publishes this object-value table by name. Retail's
// AI_value_of_observatory indexes the same dword array with the trigger
// cell's adventure-object type.
DATA(0x006925ac)
extern long g_aiEventVisibilityValues[];

long aiValueOfObservatory(struct type_point origin, long playerId, long range);

// --- globals ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:895, dc 0x2f5fc) long sum_player_dwellings(long player_id);
// CODEVIEW(E:\gamedcs\ai_player.cpp:1045, dc 0x2f998) long value_of_silo(town* current_town, playerData* player);
// CODEVIEW(E:\gamedcs\ai_player.cpp:1147, dc 0x2fdac) long value_of_building(town* current_town, type_building_id building, unsigned char* prohibited_creatures, int* extra_cost);
// CODEVIEW(E:\gamedcs\ai_player.cpp:1279, dc 0x30048) __int64 get_requirements(const town* current_town, type_building_id building);
// CODEVIEW(E:\gamedcs\ai_player.cpp:1313, dc 0x30150) void get_full_cost(const town* current_town, int* result, __int64 requirements);
// CODEVIEW(E:\gamedcs\ai_player.cpp:1367, dc 0x302e0) void mark_values(long* full_value, long total_value, __int64 requirements);
// CODEVIEW(E:\gamedcs\ai_player.cpp:1808, dc 0x31030) int MaxBuyableCreatures(const long* funds, TCreatureType type, int limit);
// CODEVIEW(E:\gamedcs\ai_player.cpp:2022, dc 0x31514) void move_creatures(armyGroup* army, TCreatureType type, short amount);
// CODEVIEW(E:\gamedcs\ai_player.cpp:2171, dc 0x317d4) short calculate_improvement(const hero* current_hero, const hero* second_hero);
// CODEVIEW(E:\gamedcs\ai_player.cpp:2817, dc 0x32670) void split_armies(hero* current_hero, const hero* enemy_hero, const armyGroup& enemy);
// CODEVIEW(E:\gamedcs\ai_player.cpp:2952, dc 0x3285c) void AI_arrange_army_for_combat(hero* current_hero, const hero* enemy_hero, const armyGroup* enemy);
// CODEVIEW(E:\gamedcs\ai_player.cpp:3013, dc 0x329f8) void AI_mark_danger_zones(hero* current_hero, long* danger_zones);
// CODEVIEW(E:\gamedcs\ai_player.cpp:3044, dc 0x32a84) long mark_destinations(hero* current_hero, long max_distance, searchArray* search_array, unsigned short* friendly_distances, type_search_type search_type);
// CODEVIEW(E:\gamedcs\ai_player.cpp:3164, dc 0x32e30) void check_holy_grail(const hero* current_hero, const searchArray* search_array, std::vector<HeroDestination,std::allocator<HeroDestination>* destinations, const unsigned short* friendly_distances);
// CODEVIEW(E:\gamedcs\ai_player.cpp:3225, dc 0x33038) long find_all_destinations(hero* current_hero, searchArray* search_array, std::vector<HeroDestination,std::allocator<HeroDestination>* destinations, long max_distance, unsigned char hiring_hero, unsigned char allow_spells, unsigned char explore_mode);
// CODEVIEW(E:\gamedcs\ai_player.cpp:3390, dc 0x33404) void mark_strategic_map(hero* current_hero, long* strategic_map, std::vector<HeroDestination,std::allocator<HeroDestination>* destinations);
// CODEVIEW(E:\gamedcs\ai_player.cpp:3498, dc 0x33854) int net_value_of_location(hero* current_hero, HeroDestination* destination, long* strategic_map, pathCell* path_cell, searchArray* search_array);
// CODEVIEW(E:\gamedcs\ai_player.cpp:3573, dc 0x33a4c) void unblock_lith(hero* current_hero, HeroDestination* destination, long* best_distance);
// CODEVIEW(E:\gamedcs\ai_player.cpp:3645, dc 0x33cf8) int AI_choose_destination(hero* current_hero, long max_distance, HeroDestination* best_point, long* best_raw_value, unsigned char allow_spells, unsigned char explore_mode);
// CODEVIEW(E:\gamedcs\ai_player.cpp:3813, dc 0x34164) void ConsiderHidingMouse(hero* current_hero, int direction);
// CODEVIEW(E:\gamedcs\ai_player.cpp:4000, dc 0x34630) unsigned char attempt_teleport(hero* current_hero, std::vector<pathCell,std::allocator<pathCell>* path, long step);
// CODEVIEW(E:\gamedcs\ai_player.cpp:4155, dc 0x34a7c) void check_gate_purchase(type_point point);
// CODEVIEW(E:\gamedcs\ai_player.cpp:4179, dc 0x34b08) void AI_AttemptMove(hero* current_hero, HeroDestination* best_point, long* best_raw_value, unsigned char explore_mode);
// CODEVIEW(E:\gamedcs\ai_player.cpp:4320, dc 0x34fb8) long value_of_hiring(town* current_town, hero* candidate, searchArray* search_array);
// CODEVIEW(E:\gamedcs\ai_player.cpp:4457, dc 0x35400) long total_artifact_value(hero* candidate, long player_id);
// CODEVIEW(E:\gamedcs\ai_player.cpp:4565, dc 0x357ec) town* get_shipyard_town(const playerData* player, long x, long y, long z);
// CODEVIEW(E:\gamedcs\ai_player.cpp:4583, dc 0x35888) unsigned char get_map_shipyard(const playerData* player, long x, long y, long z);
// CODEVIEW(E:\gamedcs\ai_player.cpp:4930, dc 0x35f08) void initialize_artifact_effects();
// CODEVIEW(E:\gamedcs\ai_player.cpp:5557, dc 0x37194) long AI_get_value_of_artifact(type_artifact artifact, const hero* owner, unsigned char equipped, unsigned char exact);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5684, dc 0x37514) long AI_get_value_of_artifact(const type_artifact& artifact, long player_id);
// CODEVIEW(E:\gamedcs\ai_player.cpp:4924, dc 0x3814c) void `vector destructor iterator'(void* __t, unsigned __s, int __n, void (*)()* __f);

// --- HeroDestination ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:3213, dc 0x380f0) void HeroDestination::HeroDestination();

// --- std ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:4924, dc 0x38118) void std::vector<type_artifact_effect *,std::allocator<type_artifact_effect *> >::`default constructor closure'();

// --- type_AI_creature_purchaser ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:2524, dc 0x31ffc) void type_AI_creature_purchaser::set(TCreatureType type, short* amount);
// CODEVIEW(E:\gamedcs\ai_player.cpp:2535, dc 0x32038) long type_AI_creature_purchaser::do_best_purchase(unsigned char trade_allowed);
// CODEVIEW(E:\gamedcs\ai_player.cpp:2657, dc 0x322f8) long type_AI_creature_purchaser::get_purchase_value(const armyGroup* new_army, short new_morale, const armyGroup* new_adjacent_army, const long* new_funds);
// CODEVIEW(E:\gamedcs\ai_player.h:313, dc 0x37e20) void type_AI_creature_purchaser::set_subtract_mode(unsigned char arg);
// CODEVIEW(E:\gamedcs\ai_player.cpp:224, dc 0x380d8) void type_AI_creature_purchaser::~type_AI_creature_purchaser();

// --- type_AI_creature_swapper ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:2209, dc 0x31864) long type_AI_creature_swapper::get_swap_value(const hero* current_hero, const armyGroup* source_army, const hero* second_hero);
// CODEVIEW(E:\gamedcs\ai_player.cpp:2282, dc 0x31a00) long type_AI_creature_swapper::choose_weakest_army(unsigned char is_shooter, unsigned char check_alignments);
// CODEVIEW(E:\gamedcs\ai_player.cpp:2350, dc 0x31af4) long type_AI_creature_swapper::value_of_adding_army(TCreatureType type, short count, short* slot, unsigned char must_replace_creature);

// --- type_AI_initializer ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:6012, dc 0x37bbc) void type_AI_initializer::type_AI_initializer();

// --- type_AI_player ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:230, dc 0x2e094) long type_AI_player::get_resource_value(int* resources);
// CODEVIEW(E:\gamedcs\ai_player.cpp:258, dc 0x2e188) void type_AI_player::calculate_demand();
// CODEVIEW(E:\gamedcs\ai_player.cpp:504, dc 0x2ea20) void type_AI_player::make_gift(long player_id);
// CODEVIEW(E:\gamedcs\ai_player.cpp:752, dc 0x2f280) void type_AI_player::calculate_reserve();
// CODEVIEW(E:\gamedcs\ai_player.cpp:1383, dc 0x30334) unsigned char type_AI_player::check_trade_supply(const int* cost, long number, int* supply, std::vector<long,std::allocator<long>* trade_qty);
// CODEVIEW(E:\gamedcs\ai_player.cpp:1474, dc 0x305b4) unsigned char type_AI_player::can_trade_resources(const int* cost, int* supply, std::vector<long,std::allocator<long>* trade_qty);
// CODEVIEW(E:\gamedcs\ai_player.cpp:1686, dc 0x30d6c) unsigned char type_AI_player::purchase_building(unsigned char* prohibited_creatures);
// CODEVIEW(E:\gamedcs\ai_player.cpp:1838, dc 0x31094) void type_AI_player::purchase_buildings();
// CODEVIEW(E:\gamedcs\ai_player.cpp:1850, dc 0x310f4) void type_AI_player::buy_creatures(hero* current_hero, town* current_town);
// CODEVIEW(E:\gamedcs\ai_player.h:263, dc 0x37dec) void type_AI_player::init(short new_team);
// CODEVIEW(E:\gamedcs\ai_player.h:273, dc 0x37df0) void type_AI_player::clear_magus_hut_value();
// CODEVIEW(E:\gamedcs\ai_player.h:278, dc 0x37df8) double type_AI_player::get_resource_value(EGameResource resource);

// --- type_antiluck_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5406, dc 0x36c40) void type_antiluck_artifact::type_antiluck_artifact();
// CODEVIEW(E:\gamedcs\ai_player.cpp:5407, dc 0x385e0) void* type_antiluck_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5407, dc 0x38614) void type_antiluck_artifact::~type_antiluck_artifact();

// --- type_antimagic_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5347, dc 0x369dc) void type_antimagic_artifact::type_antimagic_artifact(long _max_level);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5349, dc 0x38548) void* type_antimagic_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5349, dc 0x3857c) void type_antimagic_artifact::~type_antimagic_artifact();

// --- type_antimorale_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5380, dc 0x36ab4) void type_antimorale_artifact::type_antimorale_artifact();
// CODEVIEW(E:\gamedcs\ai_player.cpp:5381, dc 0x38594) void* type_antimorale_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5381, dc 0x385c8) void type_antimorale_artifact::~type_antimorale_artifact();

// Artifact-effect layout and virtual slot order are shared by the Dreamcast
// roster and the retail get_value bodies.
class type_artifact_effect {
public:
    type_artifact_effect();
    virtual ~type_artifact_effect();
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

// Dreamcast names this table `const_artifact_effects`; retail indexes the
// 144 vector objects directly with a 16-byte stride.
extern std::vector<type_artifact_effect*> g_constArtifactEffects[144];

// Complete's 0x63ac7c sentinel stream selects the concrete effect class
// created for each artifact. The numeric order is retail's jump table at
// 0x434530; the Dreamcast initializer corroborates the shared class family.
enum EArtifactEffectKind {
    ARTIFACT_EFFECT_MIGHT,
    ARTIFACT_EFFECT_POWER,
    ARTIFACT_EFFECT_KNOWLEDGE,
    ARTIFACT_EFFECT_MORALE,
    ARTIFACT_EFFECT_LUCK,
    ARTIFACT_EFFECT_SCOUTING,
    ARTIFACT_EFFECT_NECROMANCY,
    ARTIFACT_EFFECT_COMBAT,
    ARTIFACT_EFFECT_MOVEMENT,
    ARTIFACT_EFFECT_SPELLCASTER,
    ARTIFACT_EFFECT_DURATION,
    ARTIFACT_EFFECT_SCHOOL,
    ARTIFACT_EFFECT_ANTIMAGIC,
    ARTIFACT_EFFECT_ANTIMORALE,
    ARTIFACT_EFFECT_ANTILUCK,
    ARTIFACT_EFFECT_TOME,
    ARTIFACT_EFFECT_INCOME,
    ARTIFACT_EFFECT_CREATURE_GROWTH,
    ARTIFACT_EFFECT_SPELL,
    ARTIFACT_EFFECT_SHOOTER_BONUS,
    ARTIFACT_EFFECT_ANGELIC_ALLIANCE,
    ARTIFACT_EFFECT_UNDEAD_KING_CLOAK,
    ARTIFACT_EFFECT_ELIXIR_OF_LIFE,
    ARTIFACT_EFFECT_STATUE_OF_LEGION
};

extern const int g_aiArtifactEffectDefinitions[];

class type_scouting_artifact : public type_artifact_effect {
public:
    type_scouting_artifact(long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
    long m_bonus;
};

class type_combat_artifact : public type_artifact_effect {
public:
    type_combat_artifact(long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
    long m_bonus;
};

class type_might_artifact : public type_combat_artifact {
public:
    type_might_artifact(long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_power_artifact : public type_combat_artifact {
public:
    type_power_artifact(long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_knowledge_artifact : public type_combat_artifact {
public:
    type_knowledge_artifact(long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_necromancy_artifact : public type_combat_artifact {
public:
    type_necromancy_artifact(long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_movement_artifact : public type_combat_artifact {
public:
    type_movement_artifact(long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_spellcaster_artifact : public type_combat_artifact {
public:
    type_spellcaster_artifact(long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_morale_artifact : public type_combat_artifact {
public:
    type_morale_artifact(long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_luck_artifact : public type_combat_artifact {
public:
    type_luck_artifact(long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_antimorale_artifact : public type_artifact_effect {
public:
    type_antimorale_artifact();
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_antiluck_artifact : public type_artifact_effect {
public:
    type_antiluck_artifact();
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_creature_growth_artifact : public type_artifact_effect {
public:
    type_creature_growth_artifact(long newLevel, long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
    // This concrete type's +4 word is its dwelling level; its growth bonus
    // is the second constructor argument stored at +8.
    long m_bonus;
    long m_growthBonus;
};

class type_undead_king_cloak_artifact : public type_necromancy_artifact {
public:
    type_undead_king_cloak_artifact();
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_duration_artifact : public type_power_artifact {
public:
    type_duration_artifact(long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_school_artifact : public type_power_artifact {
public:
    type_school_artifact(TSpellSchool newSchool, long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
    TSpellSchool m_school;
};

class type_antimagic_artifact : public type_artifact_effect {
public:
    type_antimagic_artifact(long maxLevel);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
    long m_bonus;
};

// Retail's vtable at 0x63b74c identifies the concrete spell-granting
// artifact. Its get_value body reads this SpellID at +4. This also proves
// that the common virtual base is only its vptr; effect data belongs to each
// concrete branch, as in the Dreamcast/NH3API hierarchy.
class type_spell_artifact : public type_artifact_effect {
public:
    type_spell_artifact(SpellID newSpell);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
    SpellID m_spell;
};

class type_shooter_bonus_artifact : public type_combat_artifact {
public:
    type_shooter_bonus_artifact(long newBonus);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_angelic_alliance_artifact : public type_might_artifact {
public:
    type_angelic_alliance_artifact();
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_elixir_of_life_artifact : public type_artifact_effect {
public:
    type_elixir_of_life_artifact();
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_statue_of_legion_artifact : public type_artifact_effect {
public:
    type_statue_of_legion_artifact();
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
};

class type_tome_artifact : public type_combat_artifact {
public:
    type_tome_artifact(TSpellSchool newSchool);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
    TSpellSchool m_school;
};

// Retail get_value (0x432d20) prices amount * the owning AI player's
// resource_value[resource] * 3; the DC ctor (0x36ee8) stores the amount
// at +4 and the resource id at +8. EGameResource is town.h's enum, seen
// here through the elaborated specifier (the advmgr.h pattern) because
// every consumer includes ai_player.h before town.h.
class type_income_artifact : public type_artifact_effect {
public:
    type_income_artifact(long newAmount, enum EGameResource newResource);
    virtual long getValue(const hero* owner, unsigned char equipped,
                           unsigned char exact) const;
    long m_amount;
    enum EGameResource m_resource;
};

// --- type_artifact_effect ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5043, dc 0x361c8) void type_artifact_effect::type_artifact_effect();
// CODEVIEW(E:\gamedcs\ai_player.cpp:5044, dc 0x38184) void* type_artifact_effect::`scalar deleting destructor'(unsigned __flags);

// --- type_combat_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5075, dc 0x38204) void* type_combat_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5075, dc 0x38238) void type_combat_artifact::~type_combat_artifact();

// --- type_creature_growth_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5495, dc 0x36fec) void type_creature_growth_artifact::type_creature_growth_artifact(long new_level, long new_bonus);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5498, dc 0x386c4) void* type_creature_growth_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5498, dc 0x386f8) void type_creature_growth_artifact::~type_creature_growth_artifact();

// --- type_duration_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5267, dc 0x367cc) void type_duration_artifact::type_duration_artifact(long new_bonus);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5268, dc 0x384b0) void* type_duration_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5268, dc 0x384e4) void type_duration_artifact::~type_duration_artifact();

// --- type_garrison_purchaser ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:195, dc 0x2dfb8) void type_garrison_purchaser::type_garrison_purchaser(long new_player);
// CODEVIEW(E:\gamedcs\ai_player.cpp:202, dc 0x2dfec) void type_garrison_purchaser::clear_marks();
// CODEVIEW(E:\gamedcs\ai_player.cpp:209, dc 0x2dff0) unsigned char type_garrison_purchaser::is_marked(const town* our_town);

// --- type_income_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5477, dc 0x36ee8) void type_income_artifact::type_income_artifact(long new_amount, EGameResource new_resource);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5480, dc 0x38678) void* type_income_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5480, dc 0x386ac) void type_income_artifact::~type_income_artifact();

// --- type_knowledge_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5127, dc 0x363b4) void type_knowledge_artifact::type_knowledge_artifact(long new_bonus);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5128, dc 0x382e8) void* type_knowledge_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5128, dc 0x3831c) void type_knowledge_artifact::~type_knowledge_artifact();

// --- type_luck_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5243, dc 0x366d8) void type_luck_artifact::type_luck_artifact(long new_bonus);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5244, dc 0x38464) void* type_luck_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5244, dc 0x38498) void type_luck_artifact::~type_luck_artifact();

// --- type_might_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5090, dc 0x362e0) void type_might_artifact::type_might_artifact(long new_bonus);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5091, dc 0x38250) void* type_might_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5091, dc 0x38284) void type_might_artifact::~type_might_artifact();

// --- type_morale_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5219, dc 0x365e4) void type_morale_artifact::type_morale_artifact(long new_bonus);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5220, dc 0x38418) void* type_morale_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5220, dc 0x3844c) void type_morale_artifact::~type_morale_artifact();

// --- type_movement_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5182, dc 0x364dc) void type_movement_artifact::type_movement_artifact(long new_bonus);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5183, dc 0x38380) void* type_movement_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5183, dc 0x383b4) void type_movement_artifact::~type_movement_artifact();

// --- type_necromancy_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5145, dc 0x36414) void type_necromancy_artifact::type_necromancy_artifact(long new_bonus);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5152, dc 0x36450) long type_necromancy_artifact::get_value(const hero* owner, unsigned char equipped, unsigned char __formal);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5146, dc 0x38334) void* type_necromancy_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5146, dc 0x38368) void type_necromancy_artifact::~type_necromancy_artifact();

// --- type_power_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5109, dc 0x36350) void type_power_artifact::type_power_artifact(long new_bonus);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5110, dc 0x3829c) void* type_power_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5110, dc 0x382d0) void type_power_artifact::~type_power_artifact();

// --- type_school_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5285, dc 0x36838) void type_school_artifact::type_school_artifact(TSpellSchool new_school, long new_bonus);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5287, dc 0x384fc) void* type_school_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5287, dc 0x38530) void type_school_artifact::~type_school_artifact();

// --- type_scouting_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5057, dc 0x36214) void type_scouting_artifact::type_scouting_artifact(long new_bonus);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5059, dc 0x381b8) void* type_scouting_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5059, dc 0x381ec) void type_scouting_artifact::~type_scouting_artifact();

// --- type_spellcaster_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5198, dc 0x36558) void type_spellcaster_artifact::type_spellcaster_artifact(long new_bonus);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5199, dc 0x383cc) void* type_spellcaster_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5199, dc 0x38400) void type_spellcaster_artifact::~type_spellcaster_artifact();

// --- type_tome_artifact ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:5433, dc 0x36dd4) void type_tome_artifact::type_tome_artifact(TSpellSchool new_school);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5435, dc 0x3862c) void* type_tome_artifact::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\ai_player.cpp:5435, dc 0x38660) void type_tome_artifact::~type_tome_artifact();

// --- type_town_threat_checker ---
// CODEVIEW(E:\gamedcs\ai_player.cpp:89, dc 0x2dd40) void type_town_threat_checker::type_town_threat_checker(long new_player);
// CODEVIEW(E:\gamedcs\ai_player.cpp:179, dc 0x2dfa0) unsigned char type_town_threat_checker::is_marked(const town* our_town);

#endif  /* HOMM3_AI_PLAYER_H */
