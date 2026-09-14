// ai_tactical.h - prototypes of ai_tactical.cpp (compiland ai_tactical.obj)
#ifndef HOMM3_AI_TACTICAL_H
#define HOMM3_AI_TACTICAL_H

#include <va.h>
#include "army.h"
#include "armygrp.h"
#include "cmbtmgr.h"
#include "hexcell.h"
#include "herospec.h"
#include "spellschool.h"

class Hero;
class SearchArray;
// findpath.h's cell record; get_attack_time takes one by pointer and
// this header does not need the definition.
struct PathCell;

// PROVEN (2026-08-07) by set_melee_enemies (0x43bf20): a 16-byte record
// written as {army*, damage, 1, damage} at this+0x50 + i*0x10, with the
// whole 0x140-byte block cleared by one rep stosd of 0x50 dwords.
// Before normalization (type): type_AI_enemy_data.
struct AIEnemyData {
public:
    const Army* m_enemy;  // +0x00
    long m_damage;  // +0x04
    long m_count;  // +0x08
    long m_totalDamage;  // +0x0c
};

// The 0..3 rungs of that index. Byte-proven as a domain by
// ai_combat's get_enchantment_value (0x425510), which gates the
// two-sided Dispel pass on `mastery >= 2` and its fallback-slot
// rewrite on `mastery == 2`, and by cast_enchantment (0x425b35),
// which splits at `mastery >= 3`. These are uses of the canonical
// TSkillMastery ladder from herospec.h, not a second AI-only domain.
// TSpellSchool - get_protection_value (0x4396e0) takes it. The
// definition MOVED to spellschool.h (2026-08-08) so every consumer sees the
// same enum; the mastery domain likewise comes from herospec.h above.

// PROVEN layout (2026-08-07): get_mastery_value (0x436930) reads spell
// at +0, mastery at +4; get_damage_spell_value (0x436f60) reads power
// at +8; get_haste_value (0x4396b0) reads duration at +0xc. The
// by-value stack footprint is 20 bytes (ret 0x18 with one pointer
// argument), and type_spell_choice's ctors (0x436950/0x436980) store
// the same four words plus a 1-byte 1 at +0x10 before their own
// members - so the record ends at +0x14 and derives.
// Before normalization (type): type_enchant_data.
struct EnchantData {
public:
    SpellID m_spell;  // +0x00
    SkillMastery m_mastery;  // +0x04
    long m_power;  // +0x08
    long m_duration;  // +0x0c
    // Both ctors seed it to 1; no located consumer reads it yet.
    unsigned char m_checkResistance;  // +0x10

    EnchantData(SpellID newSpell, SkillMastery newMastery,
                      long newPower, long newDuration);
    long getMasteryValue() const;
};
SIZE(EnchantData, 0x14);

// PROVEN layout (2026-08-07): both ctors (0x436950 default,
// 0x436980 four-argument) write -1/-1/0 into +0x14/+0x18/+0x1c and a
// zero byte at +0x20 after the type_enchant_data prefix.
// Before normalization (type): type_spell_choice.
struct SpellChoice : public EnchantData {
public:
    // Names from ai_combat.obj's consumers: target indexes the
    // defender's monster vector in get_area_value (0x424c00), field_18
    // is cast_enchantment's fallback slot (0x425b2e), value is
    // get_area_value's accumulator (0x424ce7). The ctors seed
    // -1/-1/0/0.
    long m_target;  // +0x14 (-1)
    long m_secondTargetHex;  // +0x18 (-1)
    long m_value;  // +0x1c (0)
    unsigned char m_castNow;  // +0x20 (0)

    SpellChoice();
    SpellChoice(SpellID newSpell, SkillMastery newMastery,
                      long newPower, long newDuration);
};
SIZE(SpellChoice, 0x24);

// PROVEN layout (2026-08-07): the ctor (0x435ec0) writes every field
// below in order; get_simple_attack_effect (0x435b90) forwards +0/+4/+8
// into army::get_loss_combat_value as (lowest_attack, lowest_defense,
// kills_only) and branches on the byte at +9. The record ends at 0x28
// because type_AI_spellcaster embeds it at +0x20 and owns +0x48.
struct AICombatParameters {
public:
    long m_lowestAttack;  // +0x00
    long m_lowestDefense;  // +0x04
    unsigned char m_killsOnly;  // +0x08
    unsigned char m_simulated;  // +0x09
    // Dreamcast and retail byte-field boundaries agree: this gap
    // aligns the following four-byte field or aggregate.
    char m_paddingBeforeFriendlyCombatValue[0x2];
    // Original Dreamcast type_AI_combat_parameters::friendly_combat_value; retail field role agrees.
    long m_friendlyCombatValue;  // +0x0c  get_total_combat_value(side, .., 1)
    // Original Dreamcast type_AI_combat_parameters::enemy_combat_value; retail field role agrees.
    long m_enemyCombatValue;  // +0x10  ... (enemy_side, .., 1)
    // Original Dreamcast type_AI_combat_parameters::awake_friendly_value; retail field role agrees.
    long m_awakeFriendlyValue;  // +0x14  ... (side, .., 0)
    // Original Dreamcast type_AI_combat_parameters::awake_enemy_value; retail field role agrees.
    long m_awakeEnemyValue;  // +0x18  ... (enemy_side, .., 0)
    // Original Dreamcast type_AI_combat_parameters::rounds_left; retail field role agrees.
    long m_roundsLeft;  // +0x1c  1..7, from the 0x63b798 ladder
    // Original Dreamcast type_AI_combat_parameters::our_group; retail field role agrees.
    long m_ourGroup;  // +0x20
    // Original Dreamcast type_AI_combat_parameters::enemy_group; retail field role agrees.
    long m_enemyGroup;  // +0x24
    AICombatParameters(const CombatManager* combat, long side);
    long getExchangeEffect(const Army& currentArmy, const Army& enemy,
                             long distance) const;

    long getGroup() const { return m_ourGroup; }
    long getRangedAttackValue(const Army& currentArmy, const Army& enemy) const;
    long getSimpleAttackEffect(const Army& currentArmy, long ourTotal,
                                  const Army& enemy, long enemyTotal,
                                  unsigned char ranged, long distance) const;
    long getSimpleAttackEffect(const Army& currentArmy, const Army& enemy,
                                  unsigned char ranged, long distance) const;
    void simulateAttack(const Army& currentArmy, long& ourHits,
                         const Army& enemy, long& enemyHits,
                         unsigned char ranged, long distance) const;
    // DC ai_tactical.cpp:200..390 proves const army references, referenced
    // hit outputs, and const combat-query receivers across this family.
    // Retail uses those same pointer-width ABI slots and writes only the hit
    // outputs, so preserve the canonical interfaces at every source call.
    void simulateSingleAttack(const Army& currentArmy, long& ourHits,
                                const Army& enemy, long& enemyHits,
                                unsigned char ranged, long distance) const;
};
SIZE(AICombatParameters, 0x28);

// PROVEN layout (2026-08-07): the ctor (0x4360c0) writes +0/4/8/c/10/
// 14/18/1c/20/24/28 and find_attack_hex (0x436840) reads +8 (enemy),
// +0 (attacker) and +0x20 (the chosen hex).
// Before normalization (type): type_AI_attack_hex_chooser.
struct AIAttackHexChooser {
public:
    const Army* m_attackArmy;  // +0x00

protected:
    long m_speed;  // +0x04

public:
    const Army* m_enemyArmy;  // +0x08
    SearchArray* m_searchData;  // +0x0c
    const long* m_enemyAttackArray;  // +0x10
    long m_enemyTroopsLeft;  // +0x14
    long m_ourTroops;  // +0x18

protected:
    long m_bestValue;  // +0x1c
    long m_bestHex;  // +0x20

public:
    long m_bestAttackTime;  // +0x24
    const AICombatParameters* m_data;  // +0x28

    AIAttackHexChooser(const Army* attacker, const Army* defender,
                               const long* attackArray, SearchArray* search,
                               const AICombatParameters* combatData);
    unsigned char findAttackHex();
    // dc 0x3d154. Inlined into check_adjacent_hexes and carrying no
    // retail body of its own.
    long getAttackTime(const PathCell* cell) const;
    // DC ai_tactical.h:471..482 (0x27fdc/0x27fe0/0x27fe4) returns
    // best_attack_time, best_hex and best_value, at the retail-proven offsets.
    long getAttackTime() const { return m_bestAttackTime; }
    long getBestHex() const { return m_bestHex; }
    long getHexValue() const { return m_bestValue; }

protected:
    void checkAdjacentHexes(long enemyHex, long startDirection,
                              long stopDirection);
    long getHexAttackValue(long hex, long& checked);
};

// PROVEN offsets (2026-08-07) from the ctor 0x4369c0 (vptr, +4/+8 the
// two sides' heroes, +0xc side, +0x10 enemy side, +0x1c/+0x1d flags,
// the embedded parameters at +0x20, the deputy caster at +0x48 with
// its owning flag at +0x4c) and the 0x410 operator-new size in the
// same body. Interior fields past +0x4c stay padded.
// Before normalization (type): type_AI_spellcaster.
struct AISpellcaster {
public:
    // +0x00 is the compiler's own vptr (vftable 0x63b7d8, one slot:
    // the scalar deleting destructor at 0x436bf0).
    Hero* m_ourHero;  // +0x04 combat->[0x53cc + side*4]

protected:
    Hero* m_enemyHero;  // +0x08 combat->[0x53cc + enemy_side*4]

public:
    long m_side;  // +0x0c
    long m_enemySide;  // +0x10
    // Bitmask over army::bitIndex of the stacks this caster is allowed
    // to act against: should_attack_now (0x436c60) answers 0 outright
    // when the enemy's bit is clear. Name pending a writer.
    long m_enemyCanAttack;  // +0x14
    long m_canBeAttacked;  // +0x18
    unsigned char m_winLikely;  // +0x1c
    // Original Dreamcast type_AI_spellcaster::is_creature_spell; retail field role agrees.
    unsigned char m_isCreatureSpell;  // +0x1d
    // Dreamcast and retail byte-field boundaries agree: this gap
    // aligns the following four-byte field or aggregate.
    char m_paddingBeforeEstimate[0x2];
    // Original Dreamcast type_AI_spellcaster::estimate; retail field role agrees.
    AICombatParameters m_estimate;  // +0x20
    // Original Dreamcast type_AI_spellcaster::enemy_caster; retail field role agrees.
    AISpellcaster* m_enemyCaster;  // +0x48
    // Original Dreamcast type_AI_spellcaster::owns_enemy_caster; retail field role agrees.
    unsigned char m_ownsEnemyCaster;  // +0x4c
    // Dreamcast and retail byte-field boundaries agree: this gap
    // aligns the following four-byte field or aggregate.
    char m_paddingBeforeMeleeEnemies[0x3];
    // Two parallel 20-record censuses, both indexed by army::bitIndex:
    // set_melee_enemies (0x43bf20) fills the first, and
    // get_defense_boost_value (0x4387c0) sums total_damage from BOTH
    // (this + i*16 + 0x5c and this + i*16 + 0x19c).
    // Original Dreamcast type_AI_spellcaster::melee_enemies; retail field role agrees.
    AIEnemyData m_meleeEnemies[20];  // +0x50
    AIEnemyData m_attacks[20];  // +0x190

    // dc 0x3d604 (ai_tactical.cpp:793). ai.cpp's choose_creature_spell
    // (0x420d20) builds one on the stack with exactly (this, side, 1)
    // and the 0x420 frame the 0x410 operator-new size predicts.
    AISpellcaster(CombatManager* combat, long side,
                        unsigned char creatureSpell);
    // dc 0x3d6f0. The DEPUTY's constructor - the one the public ctor
    // reaches through `new` for the other side's caster, with `parent`
    // landing in the deputy's own +0x48 and its owns_deputy byte left
    // clear. Retail carries NO out-of-line body for it (the carve cuts
    // no row between type_spell_choice's ctor at 0x436980 and the
    // public ctor at 0x4369c0), so it is `inline` at its definition.
    AISpellcaster(AISpellcaster* parent, CombatManager* combat,
                        long side, unsigned char creatureSpell);
    virtual ~AISpellcaster();
    long getCaliphValue(const Army* target) const;
    // 0x43c330 / 0x43c4a0. choose_creature_spell dispatches to them on
    // creatureType - 0x5b (Dragon Fly) to the first, 0x25 (Master Genie)
    // to the second - which is the pairing that settled the 0x420d20 /
    // 0x420f00 twin question in the first place.
    long getOgreMageValue(const Army* target) const;

protected:
    // dc 0x425a8. "Is anything left on the other side that can still
    // fight?" - the answer lands in field_1c and it is what the two
    // constructors both end on. Inlined into both in retail.
    void checkSimulation();
    // dc 0x3d7b0. "Is this the last stack on our side that can still
    // act?" - inlined into consider_teleport, consider_resurrect and
    // consider_single_enchantment, with no retail body of its own.
    unsigned char isLastAction() const;
    // A THIRD census on the same 16-byte stride, byte-proven by
    // get_defense_skill_value (0x438910): it reads the record's `enemy`
    // pointer as `(bitIndex + 0x2d) * 16 + this`, i.e. this + 0x2d0 +
    // bitIndex*16, and bails when it is null. The DC roster's
    // set_worst_enemies (dc 0x42170) is the only unlocated writer left
    // that fits, so the name is provisional.
    AIEnemyData m_worstEnemies[20];  // +0x2d0

public:
    long getFaerieDragonSpellValue(long hex, long power, SpellID spell);

protected:
    void considerChainLightning(SpellChoice* choice) const;
    long getAgeValue(const Army* enemy, EnchantData caster) const;
    long getAirProtectionValue(const Army* ourArmy,
                                  EnchantData caster) const;
    long getAirShieldValue(const Army* ourArmy, EnchantData caster) const;
    long getAntimagicValue(const Army* ourArmy, EnchantData caster) const;
    long getAreaEffectValue(SpellID spell, long baseDamage,
                               SkillMastery mastery, long hex) const;
    // DC ai_tactical.cpp:1158/1186: get_attack_boost_value, const overloads.
    long getAttackBoostValue(const Army* ourArmy, const Army* enemy,
                            long oldDamage, long duration, double increase) const;
    long getAttackBoostValue(const Army* ourArmy, const Army* enemy,
                            long duration, double increase) const;
    long getAttackSkillValue(const Army* ourArmy, const Army* enemy,
                                long duration, long bonus) const;
    long getBacklashValue(const Army* ourArmy, EnchantData caster) const;
    long getBerserkValue(const Army* enemy, EnchantData caster) const;
    long getBlessValue(const Army* ourArmy, EnchantData caster) const;
    long getBloodLustValue(const Army* ourArmy, EnchantData caster) const;
    long getBlindValue(const Army* enemy, EnchantData caster) const;
    long getCancelValue(Army* currentArmy, unsigned char badSpellsOnly) const;
    long getChainLightningValue(long power, SkillMastery mastery,
                                   Army* target) const;
    long getCloneValue(const Army* ourArmy, EnchantData caster) const;
    long getCounterstrokeValue(const Army* ourArmy, EnchantData caster) const;
    long getCureValue(const Army* ourArmy, EnchantData caster) const;
    long getCurseValue(const Army* enemy, EnchantData caster) const;
    long getDamageValue(SpellID spell, long baseDamage,
                          const Hero* targetHero, const Army* target) const;
    long getDamageSpellValue(const Army* enemy, EnchantData caster) const;
    long getDefenseBoostValue(const Army* ourArmy, const Army* enemy,
                                 long duration, double increase) const;
    long getDefenseSkillValue(const Army* ourArmy, long duration,
                                 long bonus) const;
    long getDiseaseValue(const Army* enemy, EnchantData caster) const;
    long getDispelValue(const Army* ourArmy, EnchantData caster) const;
    // DC ai_tactical.cpp:2116, get_duration. Retail protection expands it.
    double getDuration(long turns, unsigned char movedThisTurn) const;
    long getDisruptiveRayValue(const Army* enemy, EnchantData caster) const;
    long getEarthProtectionValue(const Army* ourArmy,
                                    EnchantData caster) const;
    long getFireProtectionValue(const Army* ourArmy,
                                   EnchantData caster) const;
    long getFireShieldValue(const Army* ourArmy, EnchantData caster) const;
    long getForgetfulnessValue(const Army* enemy, EnchantData caster) const;
    // The luck twin of get_mirth_value / get_sorrow_value below;
    // get_enchantment_function (0x43b690) address-takes it for the
    // SPELL_FORTUNE row of its dispatch.
    long getFortuneValue(const Army* ourArmy, EnchantData caster) const;
    long getFrenzyValue(const Army* ourArmy, EnchantData caster) const;
    long getHasteValue(const Army* ourArmy, EnchantData caster) const;
    long getHypnotizeValue(const Army* enemy, EnchantData caster) const;
    long getMassDamageEffect(long enemyDamage, long friendlyDamage) const;
    long getMirthValue(const Army* ourArmy, EnchantData caster) const;
    long getMisfortuneValue(const Army* enemy, EnchantData caster) const;
    long getMuckAndMireValue(const Army* enemy, EnchantData caster) const;
    long getPoisonValue(const Army* enemy, EnchantData caster) const;
    long getPrayerValue(const Army* ourArmy, EnchantData caster) const;
    long getPrecisionValue(const Army* ourArmy, EnchantData caster) const;
    long getProtectionValue(const Army* ourArmy, SpellSchool school,
                              long level, long duration, long amount) const;
    long getShieldValue(const Army* ourArmy, EnchantData caster) const;
    long getSlayerValue(const Army* ourArmy, EnchantData caster) const;
    long getSorrowValue(const Army* enemy, EnchantData caster) const;
    long getSpeedValue(const Army* ourArmy, long increase, long duration) const;
    long getToughSkinValue(const Army* ourArmy, EnchantData caster) const;
    long getTraitorValue(const Army* enemy, const Army* target) const;
    long getWaterProtectionValue(const Army* ourArmy,
                                    EnchantData caster) const;
    long getWeaknessValue(const Army* enemy, EnchantData caster) const;
    unsigned char shouldAttackNow(const Army& enemy) const;
    long unimplemented(const Army* enemy, EnchantData caster) const;
    // The shape of every row in get_enchantment_function's table, and
    // the shape get_cancel_value (0x439a80) and get_caliph_value
    // (0x43c4a0) call back through: retail's `call dword ptr [ebp-x]`
    // with the object in ECX and no this-adjust is a single-inheritance
    // pointer-to-member-function, which is one code address wide. CodeView
    // declares these pricers const; their callback type keeps that receiver
    // qualifier too (get_enchantment_function, ai_tactical.cpp:2884).
    typedef long (AISpellcaster::*TEnchantValue)(const Army*,
                                                       EnchantData) const;

public:
    unsigned char castSpell(unsigned char retreating);

protected:
    void considerAreaEffect(SpellChoice* choice) const;
    void considerEarthquake(SpellChoice* choice) const;
    void considerEnchantment(SpellChoice* choice, long group) const;
    void considerResurrect(SpellChoice* choice) const;
    void considerSacrifice(SpellChoice& choice,
                            const Army* candidateHealedArmy, long candidateTargetHex) const;
    void considerSacrifice(SpellChoice& choice) const;
    void considerSingleEnchantment(SpellChoice* choice, long group) const;
    void considerSpell(SpellChoice* choice) const;
    void considerSummon(SpellChoice& choice) const;
    // DC 0x3de90: const member with a writable type_spell_choice reference.
    void considerMassDamage(SpellChoice& choice) const;
    void considerTeleport(SpellChoice* choice) const;
    void findEnemyAttacks();
    TEnchantValue getEnchantmentFunction(SpellID spell) const;
    // These pricers expand into consider_spell (0x43bb20), and the carve
    // has no retained row for them. That does not itself prove source inline;
    // the group, mass and summon definitions are ordinary source helpers.
    long getGroupDamageValue(SpellID spell, long baseDamage, long group,
                                Hero* targetHero) const;
    void setMeleeEnemies();
    unsigned char spellsNotRequired() const;
};
SIZE(AISpellcaster, 0x410);

// --- globals ---
// Retail 0x660858, four dwords {1, 1, 2, 3} read as [mastery]: how many
// turns a hypnotize at that mastery is worth. get_hypnotize_value
// (0x43a500) is its only located consumer, and the slot sits in the
// literal pool right behind a string, so the owning TU is unproven -
// no DATA claim until it is.
extern const long g_hypnotizeTurns[4];
// Retail 0x63b7c8, four dwords {4, 4, 5, 5} read as [mastery]: how
// many stacks a chain lightning at that mastery bounces through.
// get_chain_lightning_value (0x437190) is its only consumer and the
// slot sits in this TU's own .rdata, immediately before the class
// vftable at 0x63b7d8 - but it is left DECLARED, not defined, for the
// same reason akHypnotizeTurns above is: emitting it here would put a
// fresh .rdata allocation in our object whose placement retail's
// section layout does not have to agree with.
extern const long g_chainLightningTargets[4];

double valueOfLuckAndMorale(long value, long change,
                                double goodValueMultiplier,
                                double badValueMultiplier);
double aiValueOfMorale(long morale, long change);
double aiValueOfLuck(long luck, long change);
long aiGetAttackDamage(const Army& currentArmy, long ourHits,
                          const Army& enemy, unsigned char ranged,
                          long distance);
long getMultiHeadBonus(long ourGroup, const Army* ourArmy, long ourHex,
                          long troopCount, const Army* enemy, long enemyHex,
                          const AICombatParameters* estimate);
long getBreathBonus(long ourGroup, const Army* ourArmy, long ourHex,
                      long troopCount, const Army* enemy, long enemyHex,
                      const AICombatParameters* estimate);

// --- army ---
// CODEVIEW(E:\gamedcs\Army.h:724, dc 0x429c0) int army::GetMorale(unsigned char apply_limits);
// CODEVIEW(E:\gamedcs\Army.h:730, dc 0x429f4) int army::GetLuck(unsigned char apply_limits);
// CODEVIEW(E:\gamedcs\Army.h:825, dc 0x42a28) TSkillMastery army::get_spell_level(SpellID spell);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:2183, dc 0x42aa8) void army::army(const army* __that);

// --- combatManager ---
// CODEVIEW(E:\gamedcs\cmbtmgr.h:1466, dc 0x42a3c) army* combatManager::find_resurrection_target(SpellID spell, long group, long hex, unsigned char creature_spell);

// --- std ---
// CODEVIEW(..\stlport\stl_deque.h:128, dc 0x42994) unsigned std::__deque_buf_size(unsigned __n, unsigned size);
// CODEVIEW(..\stlport\stl_deque.h:622, dc 0x42db0) void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::deque<enum SpellID,std::allocator<enum SpellID>,0>(const std::deque<enum* __x);
// CODEVIEW(..\stlport\stl_vector.h:236, dc 0x42e68) void std::vector<army *,std::allocator<army *> >::vector<army *,std::allocator<army *> >(const std::vector<army* __x);
// CODEVIEW(..\stlport\stl_deque.h:570, dc 0x42ec8) std::_Deque_iterator<enum std::deque<enum SpellID,std::allocator<enum SpellID>,0>::begin(__$ReturnUdt);
// CODEVIEW(..\stlport\stl_deque.h:571, dc 0x42edc) std::_Deque_iterator<enum std::deque<enum SpellID,std::allocator<enum SpellID>,0>::end(__$ReturnUdt);
// CODEVIEW(..\stlport\stl_deque.h:610, dc 0x42ef0) unsigned std::deque<enum SpellID,std::allocator<enum SpellID>,0>::size();
// CODEVIEW(..\stlport\stl_deque.h:452, dc 0x42f08) void std::_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>::_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>(const std::allocator<enum* __a, unsigned __num_elements);
// CODEVIEW(..\stlport\stl_deque.h:460, dc 0x42f70) std::allocator<enum std::_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>::get_allocator(__$ReturnUdt);
// CODEVIEW(..\stlport\stl_vector.h:153, dc 0x42f7c) std::allocator<army std::vector<army *,std::allocator<army *> >::get_allocator(__$ReturnUdt);
// CODEVIEW(..\stlport\stl_vector.h:94, dc 0x42f84) void std::_Vector_base<army *,std::allocator<army *> >::_Vector_base<army *,std::allocator<army *> >(unsigned __n, const std::allocator<army* __a);
// CODEVIEW(..\stlport\stl_deque.h:272, dc 0x42fcc) void std::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >();
// CODEVIEW(..\stlport\stl_deque.h:283, dc 0x42fe8) int std::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator-(const std::_Deque_iterator<enum* __x);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0x43000) void std::_STL_alloc_proxy<enum SpellID * *,enum SpellID *,std::allocator<enum SpellID> >::_STL_alloc_proxy<enum SpellID * *,enum SpellID *,std::allocator<enum SpellID> >(const std::allocator<enum* __a, SpellID*** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0x4300c) void std::_STL_alloc_proxy<unsigned int,enum SpellID,std::allocator<enum SpellID> >::_STL_alloc_proxy<unsigned int,enum SpellID,std::allocator<enum SpellID> >(const std::allocator<enum* __a, const unsigned* __p);
// CODEVIEW(..\stlport\stl_deque.h:190, dc 0x43018) void std::_Deque_iterator_base<enum SpellID,std::_Buf_size_traits<enum SpellID,0> >::_Deque_iterator_base<enum SpellID,std::_Buf_size_traits<enum SpellID,0> >();
// CODEVIEW(..\stlport\stl_deque.h:193, dc 0x43028) int std::_Deque_iterator_base<enum SpellID,std::_Buf_size_traits<enum SpellID,0> >::_M_subtract(const std::_Deque_iterator_base<enum* __x);
// CODEVIEW(..\stlport\stl_deque.c:118, dc 0x43054) void std::_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>::_M_initialize_map(unsigned __num_elements);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0x43118) std::_Deque_iterator<enum std::uninitialized_copy(__$ReturnUdt, std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, std::_Deque_iterator<enum __result);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0x431a4) army** std::uninitialized_copy(army** __first, army** __last, army** __result);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0x431dc) SpellID** std::_STL_alloc_proxy<enum SpellID * *,enum SpellID *,std::allocator<enum SpellID> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0x43204) SpellID** std::allocator<enum SpellID *>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_deque.c:144, dc 0x43228) void std::_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>::_M_create_nodes(SpellID** __nstart, SpellID** __nfinish);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0x43264) std::_Deque_iterator<enum std::__uninitialized_copy(__$ReturnUdt, std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, std::_Deque_iterator<enum __result, SpellID* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0x432e0) army** std::__uninitialized_copy(army** __first, army** __last, army** __result, army** __formal);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0x432fc) SpellID* std::_STL_alloc_proxy<unsigned int,enum SpellID,std::allocator<enum SpellID> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0x43324) SpellID* std::allocator<enum SpellID>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0x43348) std::_Deque_iterator<enum std::__uninitialized_copy_aux(__$ReturnUdt, std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, std::_Deque_iterator<enum __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0x433d4) army** std::__uninitialized_copy_aux(army** __first, army** __last, army** __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_deque.h:276, dc 0x43410) const SpellID* std::_Deque_iterator<enum SpellID,std::_Const_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator*();
// CODEVIEW(..\stlport\stl_deque.h:285, dc 0x43414) std::_Deque_iterator<enum* std::_Deque_iterator<enum SpellID,std::_Const_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator++();
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0x43430) void std::construct(SpellID* __p, const SpellID* __value);

// --- type_AI_attack_hex_chooser ---
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:511, dc 0x3cf50) long type_AI_attack_hex_chooser::get_hex_attack_value(long hex, long& checked);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:575, dc 0x3d154) long type_AI_attack_hex_chooser::get_attack_time(const pathCell* cell);

// --- type_AI_combat_parameters ---
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:215, dc 0x3c854) void type_AI_combat_parameters::simulate_single_attack(const army* current_army, long* our_hits, const army* enemy, long* enemy_hits, unsigned char ranged, long distance);

// --- type_AI_spellcaster ---
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:779, dc 0x3d5dc) void type_AI_spellcaster::initialize(combatManager* combat, long side);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:793, dc 0x3d604) void type_AI_spellcaster::type_AI_spellcaster(combatManager* combat, long side, unsigned char creature_spell);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:817, dc 0x3d6f0) void type_AI_spellcaster::type_AI_spellcaster(type_AI_spellcaster* parent, combatManager* combat, long side, unsigned char creature_spell);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:837, dc 0x3d7b0) unsigned char type_AI_spellcaster::is_last_action();
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:965, dc 0x3dabc) long type_AI_spellcaster::get_group_damage_value(SpellID spell, long base_damage, long group, hero* target_hero);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:1035, dc 0x3dc50) void type_AI_spellcaster::consider_area_effect(type_spell_choice* choice);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:1126, dc 0x3de90) void type_AI_spellcaster::consider_mass_damage(type_spell_choice* choice);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:1158, dc 0x3df5c) long type_AI_spellcaster::get_attack_boost_value(const army* our_army, const army* enemy, long old_damage, long duration, double increase);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:1186, dc 0x3e120) long type_AI_spellcaster::get_attack_boost_value(const army* our_army, const army* enemy, long duration, double increase);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:1218, dc 0x3e280) long type_AI_spellcaster::get_frenzy_value(const army* our_army, type_enchant_data caster);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:1767, dc 0x3f9d0) long type_AI_spellcaster::get_move_order_change_value(const army* our_army);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:1788, dc 0x3fa24) long type_AI_spellcaster::get_muck_and_mire_value(const army* enemy, type_enchant_data caster);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:2116, dc 0x40130) double type_AI_spellcaster::get_duration(long turns, unsigned char moved_this_turn);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:2553, dc 0x40ec0) void type_AI_spellcaster::consider_teleport(type_spell_choice* choice);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:2608, dc 0x4101c) void type_AI_spellcaster::consider_resurrect(type_spell_choice* choice);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:2685, dc 0x41278) void type_AI_spellcaster::consider_sacrifice(type_spell_choice* choice, const army* healed_army, long target_hex);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:3093, dc 0x41e5c) void type_AI_spellcaster::consider_summon(type_spell_choice* choice);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:3191, dc 0x420ac) void type_AI_spellcaster::set_melee_enemies();
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:3221, dc 0x42170) void type_AI_spellcaster::set_worst_enemies();
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:3237, dc 0x42220) void type_AI_spellcaster::add_enemy(type_AI_enemy_data* sum, const army* our_army, const army* enemy, unsigned char ranged);
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:3254, dc 0x4227c) void type_AI_spellcaster::find_enemy_attacks();
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:3377, dc 0x425a8) void type_AI_spellcaster::check_simulation();
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:3398, dc 0x42610) unsigned char type_AI_spellcaster::spells_not_required();
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:807, dc 0x42a74) void* type_AI_spellcaster::`scalar deleting destructor'(unsigned __flags);

// --- type_enchant_data ---
// CODEVIEW(E:\gamedcs\ai_tactical.cpp:744, dc 0x3d524) void type_enchant_data::type_enchant_data(SpellID new_spell, TSkillMastery new_mastery, long new_power, long new_duration);

#endif  /* HOMM3_AI_TACTICAL_H */
