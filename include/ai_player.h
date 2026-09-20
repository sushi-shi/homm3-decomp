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
    long getResourceValue(int* resources) const;
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

// --- type_antimorale_artifact ---

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

#endif  /* HOMM3_AI_PLAYER_H */
