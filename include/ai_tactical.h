#ifndef HOMM3_AI_TACTICAL_H
#define HOMM3_AI_TACTICAL_H

#include "va.h"

#include "army.h"
#include "armygrp.h"
#include "cmbtmgr.h"
#include "herospec.h"
#include "hexcell.h"
#include "spellschool.h"

class hero;
class searchArray;
// findpath.h's cell record; get_attack_time takes one by pointer and
// this header does not need the definition.
struct pathCell;

// PROVEN (2026-08-07) by set_melee_enemies (0x43bf20): a 16-byte record
// written as {army*, damage, 1, damage} at this+0x50 + i*0x10, with the
// whole 0x140-byte block cleared by one rep stosd of 0x50 dwords.
struct type_AI_enemy_data {
public:
    const army* m_enemy;  // +0x00
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
struct type_enchant_data {
public:
    SpellID m_spell;  // +0x00
    TSkillMastery m_mastery;  // +0x04
    long m_power;  // +0x08
    long m_duration;  // +0x0c
    // Both ctors seed it to 1; no located consumer reads it yet.
    unsigned char m_checkResistance;  // +0x10

    type_enchant_data(SpellID newSpell, TSkillMastery newMastery,
                      long newPower, long newDuration);
    long getMasteryValue() const;
};
SIZE(type_enchant_data, 0x14);

// PROVEN layout (2026-08-07): both ctors (0x436950 default,
// 0x436980 four-argument) write -1/-1/0 into +0x14/+0x18/+0x1c and a
// zero byte at +0x20 after the type_enchant_data prefix.
struct type_spell_choice : public type_enchant_data {
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

    type_spell_choice();
    type_spell_choice(SpellID newSpell, TSkillMastery newMastery,
                      long newPower, long newDuration);
};
SIZE(type_spell_choice, 0x24);

// PROVEN layout (2026-08-07): the ctor (0x435ec0) writes every field
// below in order; get_simple_attack_effect (0x435b90) forwards +0/+4/+8
// into army::get_loss_combat_value as (lowest_attack, lowest_defense,
// kills_only) and branches on the byte at +9. The record ends at 0x28
// because type_AI_spellcaster embeds it at +0x20 and owns +0x48.
struct type_AI_combat_parameters {
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
    type_AI_combat_parameters(const combatManager* combat, long side);
    long getExchangeEffect(const army& currentArmy, const army& enemy,
                             long distance) const;

    // Original: type_AI_combat_parameters::get_enemy_group; ai_tactical.h:82, dc 0x27fd4.
    long getEnemyGroup() const { return m_enemyGroup; }
#include "inline/ai_parameters_get_group.inl"
    long getRangedAttackValue(const army& currentArmy, const army& enemy) const;
    long getSimpleAttackEffect(const army& currentArmy, long ourTotal,
                                  const army& enemy, long enemyTotal,
                                  unsigned char ranged, long distance) const;
    long getSimpleAttackEffect(const army& currentArmy, const army& enemy,
                                  unsigned char ranged, long distance) const;
    void simulateAttack(const army& currentArmy, long& ourHits,
                         const army& enemy, long& enemyHits,
                         unsigned char ranged, long distance) const;
    // DC ai_tactical.cpp:200..390 proves const army references, referenced
    // hit outputs, and const combat-query receivers across this family.
    // Retail uses those same pointer-width ABI slots and writes only the hit
    // outputs, so preserve the canonical interfaces at every source call.
    void simulateSingleAttack(const army& currentArmy, long& ourHits,
                                const army& enemy, long& enemyHits,
                                unsigned char ranged, long distance) const;
};
SIZE(type_AI_combat_parameters, 0x28);

// PROVEN layout (2026-08-07): the ctor (0x4360c0) writes +0/4/8/c/10/
// 14/18/1c/20/24/28 and find_attack_hex (0x436840) reads +8 (enemy),
// +0 (attacker) and +0x20 (the chosen hex).
struct type_AI_attack_hex_chooser {
public:
    const army* m_attackArmy;  // +0x00

protected:
    long m_speed;  // +0x04

public:
    const army* m_enemyArmy;  // +0x08
    searchArray* m_searchData;  // +0x0c
    const long* m_enemyAttackArray;  // +0x10
    long m_enemyTroopsLeft;  // +0x14
    long m_ourTroops;  // +0x18

protected:
    long m_bestValue;  // +0x1c
    long m_bestHex;  // +0x20

public:
    long m_bestAttackTime;  // +0x24
    const type_AI_combat_parameters* m_data;  // +0x28

    type_AI_attack_hex_chooser(const army* attacker, const army* defender,
                               const long* attackArray, searchArray* search,
                               const type_AI_combat_parameters* combatData);
    unsigned char findAttackHex();
    // dc 0x3d154. Inlined into check_adjacent_hexes and carrying no
    // retail body of its own.
    long getAttackTime(const pathCell* cell) const;
#include "inline/ai_attack_chooser_get_attack_time.inl"
#include "inline/ai_attack_chooser_get_best_hex.inl"
#include "inline/ai_attack_chooser_get_hex_value.inl"

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
struct type_AI_spellcaster {
public:
    // +0x00 is the compiler's own vptr (vftable 0x63b7d8, one slot:
    // the scalar deleting destructor at 0x436bf0).
    hero* m_ourHero;  // +0x04 combat->[0x53cc + side*4]

protected:
    hero* m_enemyHero;  // +0x08 combat->[0x53cc + enemy_side*4]

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
    type_AI_combat_parameters m_estimate;  // +0x20
    // Original Dreamcast type_AI_spellcaster::enemy_caster; retail field role agrees.
    type_AI_spellcaster* m_enemyCaster;  // +0x48
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
    type_AI_enemy_data m_meleeEnemies[20];  // +0x50
    type_AI_enemy_data m_attacks[20];  // +0x190

    // dc 0x3d604 (ai_tactical.cpp:793). ai.cpp's choose_creature_spell
    // (0x420d20) builds one on the stack with exactly (this, side, 1)
    // and the 0x420 frame the 0x410 operator-new size predicts.
    type_AI_spellcaster(combatManager* combat, long side,
                        unsigned char creatureSpell);
    // dc 0x3d6f0. The DEPUTY's constructor - the one the public ctor
    // reaches through `new` for the other side's caster, with `parent`
    // landing in the deputy's own +0x48 and its owns_deputy byte left
    // clear. Complete expands this ordinary constructor into the public
    // constructor; its definition is visible before that caller in the TU.
    type_AI_spellcaster(type_AI_spellcaster* parent, combatManager* combat,
                        long side, unsigned char creatureSpell);
    virtual ~type_AI_spellcaster();
    long getCaliphValue(const army* target) const;
    // 0x43c330 / 0x43c4a0. choose_creature_spell dispatches to them on
    // creatureType - 0x5b (Dragon Fly) to the first, 0x25 (Master Genie)
    // to the second - which is the pairing that settled the 0x420d20 /
    // 0x420f00 twin question in the first place.
    long getOgreMageValue(const army* target) const;

protected:
    void initialize(combatManager* combat, long side);
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
    // bitIndex*16, and bails when it is null. setWorstEnemies fills this
    // array with the larger of the melee and ranged damage records.
    type_AI_enemy_data m_worstEnemies[20];  // +0x2d0

public:
    long getFaerieDragonSpellValue(long hex, long power, SpellID spell);

protected:
    void considerChainLightning(type_spell_choice* choice) const;
    long getAgeValue(const army* enemy, type_enchant_data caster) const;
    long getAirProtectionValue(const army* ourArmy,
                                  type_enchant_data caster) const;
    long getAirShieldValue(const army* ourArmy, type_enchant_data caster) const;
    long getAntimagicValue(const army* ourArmy, type_enchant_data caster) const;
    long getAreaEffectValue(SpellID spell, long baseDamage,
                               TSkillMastery mastery, long hex) const;
    // DC ai_tactical.cpp:1158/1186: get_attack_boost_value, const overloads.
    long getAttackBoostValue(const army* ourArmy, const army* enemy,
                            long oldDamage, long duration, double increase) const;
    long getAttackBoostValue(const army* ourArmy, const army* enemy,
                            long duration, double increase) const;
    long getAttackSkillValue(const army* ourArmy, const army* enemy,
                                long duration, long bonus) const;
    long getBacklashValue(const army* ourArmy, type_enchant_data caster) const;
    long getBerserkValue(const army* enemy, type_enchant_data caster) const;
    long getBlessValue(const army* ourArmy, type_enchant_data caster) const;
    long getBloodLustValue(const army* ourArmy, type_enchant_data caster) const;
    long getBlindValue(const army* enemy, type_enchant_data caster) const;
    long getCancelValue(army* currentArmy, unsigned char badSpellsOnly) const;
    long getChainLightningValue(long power, TSkillMastery mastery,
                                   army* target) const;
    long getCloneValue(const army* ourArmy, type_enchant_data caster) const;
    long getCounterstrokeValue(const army* ourArmy, type_enchant_data caster) const;
    long getCureValue(const army* ourArmy, type_enchant_data caster) const;
    long getCurseValue(const army* enemy, type_enchant_data caster) const;
    long getDamageValue(SpellID spell, long baseDamage,
                          const hero* targetHero, const army* target) const;
    long getDamageSpellValue(const army* enemy, type_enchant_data caster) const;
    long getDefenseBoostValue(const army* ourArmy, const army* enemy,
                                 long duration, double increase) const;
    long getDefenseSkillValue(const army* ourArmy, long duration,
                                 long bonus) const;
    long getDiseaseValue(const army* enemy, type_enchant_data caster) const;
    long getDispelValue(const army* ourArmy, type_enchant_data caster) const;
    // DC ai_tactical.cpp:2116, get_duration. Retail protection expands it.
    double getDuration(long turns, unsigned char movedThisTurn) const;
    long getDisruptiveRayValue(const army* enemy, type_enchant_data caster) const;
    long getEarthProtectionValue(const army* ourArmy,
                                    type_enchant_data caster) const;
    long getFireProtectionValue(const army* ourArmy,
                                   type_enchant_data caster) const;
    long getFireShieldValue(const army* ourArmy, type_enchant_data caster) const;
    long getForgetfulnessValue(const army* enemy, type_enchant_data caster) const;
    // The luck twin of get_mirth_value / get_sorrow_value below;
    // get_enchantment_function (0x43b690) address-takes it for the
    // SPELL_FORTUNE row of its dispatch.
    long getFortuneValue(const army* ourArmy, type_enchant_data caster) const;
    long getFrenzyValue(const army* ourArmy, type_enchant_data caster) const;
    long getHasteValue(const army* ourArmy, type_enchant_data caster) const;
    long getHypnotizeValue(const army* enemy, type_enchant_data caster) const;
    long getMassDamageEffect(long enemyDamage, long friendlyDamage) const;
    long getMirthValue(const army* ourArmy, type_enchant_data caster) const;
    long getMisfortuneValue(const army* enemy, type_enchant_data caster) const;
    long getMoveOrderChangeValue(const army* ourArmy) const;
    long getMuckAndMireValue(const army* enemy, type_enchant_data caster) const;
    long getPoisonValue(const army* enemy, type_enchant_data caster) const;
    long getPrayerValue(const army* ourArmy, type_enchant_data caster) const;
    long getPrecisionValue(const army* ourArmy, type_enchant_data caster) const;
    long getProtectionValue(const army* ourArmy, TSpellSchool school,
                              long level, long duration, long amount) const;
    long getShieldValue(const army* ourArmy, type_enchant_data caster) const;
    long getSlayerValue(const army* ourArmy, type_enchant_data caster) const;
    long getSorrowValue(const army* enemy, type_enchant_data caster) const;
    long getSpeedValue(const army* ourArmy, long increase, long duration) const;
    long getToughSkinValue(const army* ourArmy, type_enchant_data caster) const;
    long getTraitorValue(const army* enemy, const army* target) const;
    long getWaterProtectionValue(const army* ourArmy,
                                    type_enchant_data caster) const;
    long getWeaknessValue(const army* enemy, type_enchant_data caster) const;
    unsigned char shouldAttackNow(const army& enemy) const;
    long unimplemented(const army* enemy, type_enchant_data caster) const;
    // The shape of every row in get_enchantment_function's table, and
    // the shape get_cancel_value (0x439a80) and get_caliph_value
    // (0x43c4a0) call back through: retail's `call dword ptr [ebp-x]`
    // with the object in ECX and no this-adjust is a single-inheritance
    // pointer-to-member-function, which is one code address wide. CodeView
    // declares these pricers const; their callback type keeps that receiver
    // qualifier too (get_enchantment_function, ai_tactical.cpp:2884).
    typedef long (type_AI_spellcaster::*TEnchantValue)(const army*,
                                                       type_enchant_data) const;

public:
    unsigned char castSpell(unsigned char retreating);

protected:
    void considerAreaEffect(type_spell_choice& choice) const;
    void considerEarthquake(type_spell_choice* choice) const;
    void considerEnchantment(type_spell_choice* choice, long group) const;
    void considerResurrect(type_spell_choice* choice) const;
    void considerSacrifice(type_spell_choice& choice,
                            const army* candidateHealedArmy, long candidateTargetHex) const;
    void considerSacrifice(type_spell_choice& choice) const;
    void considerSingleEnchantment(type_spell_choice* choice, long group) const;
    void considerSpell(type_spell_choice* choice) const;
    void considerSummon(type_spell_choice& choice) const;
    // DC 0x3de90: const member with a writable type_spell_choice reference.
    void considerMassDamage(type_spell_choice& choice) const;
    void considerTeleport(type_spell_choice* choice) const;
    void findEnemyAttacks();
    TEnchantValue getEnchantmentFunction(SpellID spell) const;
    // These pricers expand into consider_spell (0x43bb20), and the carve
    // has no retained row for them. That does not itself prove source inline;
    // the group, mass and summon definitions are ordinary source helpers.
    long getGroupDamageValue(SpellID spell, long baseDamage, long group,
                                hero* targetHero) const;
    void setMeleeEnemies();
    void setWorstEnemies();
    void addEnemy(type_AI_enemy_data& sum, const army* ourArmy,
                  const army* enemy, unsigned char ranged);
    unsigned char spellsNotRequired() const;
};
SIZE(type_AI_spellcaster, 0x410);

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
long aiGetAttackDamage(const army& currentArmy, long ourHits,
                          const army& enemy, unsigned char ranged,
                          long distance);
long getMultiHeadBonus(long ourGroup, const army* ourArmy, long ourHex,
                          long troopCount, const army* enemy, long enemyHex,
                          const type_AI_combat_parameters* estimate);
long getBreathBonus(long ourGroup, const army* ourArmy, long ourHex,
                      long troopCount, const army* enemy, long enemyHex,
                      const type_AI_combat_parameters* estimate);

#endif  /* HOMM3_AI_TACTICAL_H */
