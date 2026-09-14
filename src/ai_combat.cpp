// ai_combat.cpp - E:\gamedcs\ai_combat.cpp (compiland ai_combat.obj)

// NEW LEVER (2026-08-07, byte-proven here by AI_quick_combat and
// AI_auto_combat): under /GX the scope-exit destructor sequence keeps
// the EH state variable live across every call a destructor makes,
// emitting `mov [ebp-4], <state>` before each one. Retail emits none -
// its `operator delete` was visible as NOTHROW. VC6's <new> declares
// operator delete WITHOUT an exception specification, so a TU that
// wants retail's shape has to say so itself; see the
// __declspec(nothrow) redeclaration in include/ai_combat.h. That one
// declaration is what takes both entry points from 96-98% to exact.
#include <va.h>
// check_wall_archery_penalty's three fortification tests are
// town::HasBuilding calls in the Dreamcast body (dc 0x2a470, three
// `jsr @r9` with r5 = 7/8/9 and r6 = 0); see town.h for why the
// inline's visibility is scoped.
#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "ai_combat.h"
#include "ai_player.h"
#include "ai_tactical.h"
#include "advmgr.h"
#include "armygrp.h"
#include "game.h"
#include "magicterrain.h"
#include "misc.h"
#include "hero.h"
#include "town.h"
#include "includes.h"

#include "homm3_minmax.h"

// The mutually exclusive AI-dispatch family encoded in SSpellTraits::field_c.
// cast_spell masks precisely these six bits twice and switches on the five
// values below; the sixth bit has no quick-combat implementation. Names are
// behavior-derived and local to this TU. Kept as constants rather than a new
// enum because adding enumerators to armygrp.h measurably perturbs unrelated
// VC6 units through their shared type environment.
const unsigned int g_aiSpellDirectDamage = 0x8000;
const unsigned int g_aiSpellOpeningDamage = 0x10000;
const unsigned int g_aiSpellMassDamage = 0x20000;
const unsigned int g_aiSpellEnchantment = 0x40000;
const unsigned int g_aiSpellResurrection = 0x80000;
const unsigned int g_aiSpellClassMask = 0x1f8000;

// Retail uniquely proves artifact 0x53 suppresses level-3+ magic on either
// side (Recanter's Cloak). Familiar's 0x2b identity is now shared through
// TCreatureType: CastSpell and this AI projection independently prove it.
const int g_artifactRecantersCloak = 0x53;
const int g_secondarySkillWisdom = 7;
const int g_secondarySkillSiegeBallistics = 10;
const int g_secondarySkillEagleEye = 11;
const int g_secondarySkillTactics = 19;

// Attribute roles byte-proven by initialize_creatures. Kept TU-local for
// the same shared-header codegen reason as the cast_spell constants above.
const unsigned int g_ctaFlying = 0x2;
const unsigned int g_ctaShooter = 0x4;
const unsigned int g_ctaNoMeleePenalty = 0x1000;
const unsigned int g_ctaDoubleRangedValue = 0x8000;

// Retail .data 0x6604d0: five doubles selected by the game's signed
// difficulty byte. AI_value_of_combat uses the row when either combat
// side belongs to a human. DC's static-global roster supplies the name;
// retail fixes the extent and every value before the next datum at +0x28.
DATA(0x006604d0) static double g_defenseEstimates[5] = {
    0.5, 0.5, 1.0, 1.25, 1.25
};

float valueOfExperience(const hero* currentHero, const armyGroup& currentArmy);

VA(0x00423c80, 0x79)  // dc 0x29978
long type_monster_data::getEnchantmentValue(type_spell_choice& choice, const hero* castingHero, const hero* targetHero) const
{
    if (m_totalValue == 0)
        return 0;
    long turns = choice.m_duration;
    if (turns > 5)
        turns = 5;
    long value = g_spellTraits[choice.m_spell].m_masteryValues[choice.m_mastery];
    float chance = getSpellWorkChance(choice.m_spell, m_type, castingHero, targetHero);
    return static_cast<long>(value * turns * m_totalValue * chance / 500.0);
}

// E:\gamedcs\ai_combat.cpp:56
// RECONSTRUCTED FROM ITS INLINED COPIES (dc 0x29a30) - retail has no
// out-of-line row: /Ob2 inlined every call site (0x425c65 and 0x425d8d
// inside cast_enchantment) and OPT:REF dropped the COMDAT. Spelled
// `inline` here so our obj does not carry a base-only function retail
// never shipped. Bytes prove the shape: the pre-image
// total_hit_points*damage_modifier is taken BEFORE the update, the
// per-creature delta is a 64-bit imul/__alldiv, and damage_modifier is
// rewritten as the new total over that pre-image.
inline void type_monster_data::castEnchantment(long spellValue, unsigned char increase)
{
    double previous = m_totalValue * m_combatValuePerHit;
    // 64-bit local, not a long: retail spills the __alldiv result's
    // HIGH dword to a slot nothing ever reads (0x4258fa / 0x425a2c)
    // while consuming the low half straight out of eax.
    __int64 perCreature =
        static_cast<__int64>(m_value) * spellValue / m_totalValue;
    if (increase) {
        m_totalValue += spellValue;
        m_value += static_cast<long>(perCreature);
    } else {
        m_totalValue -= spellValue;
        m_value -= static_cast<long>(perCreature);
    }
    m_combatValuePerHit = m_totalValue / previous;
}

VA(0x00423d00, 0xDA)  // dc 0x29b94
long type_monster_data::getResurrectionValue(type_spell_choice& choice, const hero* castingHero) const
{
    if (m_originalNumber <= m_number)
        return 0;
    if (getSpellWorkChance(choice.m_spell, m_type, castingHero, castingHero) == 0.0)
        return 0;
    long value = choice.getMasteryValue()
                 + g_spellTraits[choice.m_spell].m_powerFactor * choice.m_power;
    if (castingHero)
        value += const_cast<hero*>(castingHero)->getHeroSpellBonus(
            choice.m_spell, g_creatureTypeTraits[m_type].m_level, value);
    long resurrected = min(static_cast<long>(value * m_combatValuePerHit) / m_value,
                           m_originalNumber - m_number);
    return resurrected * m_value;
}

// E:\gamedcs\ai_combat.cpp:110
// Retail expands this helper at cast_spell's selected target and emits no
// out-of-line row. The Dreamcast supplies the helper boundary/name; the
// statements below are reconstructed from the retail expansion.
inline void type_monster_data::castResurrection(
    type_spell_choice& choice,
    const hero* castingHero)
{
    long resurrected = getResurrectionValue(choice, castingHero)
                       / m_value;
    m_number += resurrected;
    m_totalValue += resurrected * m_value;
}

VA(0x00423de0, 0xB1)  // dc 0x29ce0
long type_monster_data::getSpellDamage(SpellID spell, const hero* castingHero, const hero* targetHero, long damage) const
{
    if (m_totalValue == 0)
        return 0;
    damage = static_cast<long>(getSpellWorkChance(spell, m_type, castingHero, targetHero) * damage);
    if (damage == 0)
        return 0;
    damage = modifySpellDamage(damage, spell, m_type);
    if (damage == 0)
        return 0;
    damage = const_cast<hero*>(castingHero)->modifySpellDamage(spell, damage, 0);
    if (damage == 0)
        return 0;
    return min(static_cast<long>(damage * m_combatValuePerHit), m_totalValue);
}

VA(0x00423ea0, 0x36)  // dc 0x29dec
long type_monster_data::takeDamage(long damage)
{
    if (m_totalValue < damage) {
        damage = m_totalValue;
        m_totalValue = 0;
        m_number = 0;
    } else {
        m_totalValue -= damage;
        m_number = (m_totalValue + m_value - 1) / m_value;
    }
    return damage;
}

VA(0x00423ee0, 0x233)  // dc 0x29e2c
type_AI_combat_data::type_AI_combat_data(const hero* newHero, const armyGroup* newArmy, double baseModifier, const hero* enemyHero, const town* enemyTown, NewmapCell* mapCell)
{
    m_currentHero = const_cast<hero*>(newHero);
    m_currentArmy = const_cast<armyGroup*>(newArmy);
    checkWallArcheryPenalty(enemyTown);
    m_enemyHero = const_cast<hero*>(enemyHero);

    if (newHero == 0)
        m_mana = 0;
    else
        m_mana = m_currentHero->m_mana;

    m_canCastSpells = 1;
    if (m_currentHero == 0
        || !m_currentHero->isWieldingArtifact(ARTIFACT_SPELLBOOK))
        m_canCastSpells = 0;
    if (m_currentHero != 0
        && m_currentHero->isWieldingArtifact(ARTIFACT_ORB_OF_INHIBITION))
        m_canCastSpells = 0;

    m_terrain = -1;
    if (mapCell != 0) {
        switch (mapCell->getSpecialTerrain()) {
        case MAGIC_PLAINS:
            m_terrain = kMagicTerrainMagicPlains;
            break;
        case LUCID_POOLS:
            m_terrain = kMagicTerrainLucidPools;
            break;
        case FIERY_FIELDS:
            m_terrain = kMagicTerrainFieryFields;
            break;
        case ROCKLANDS:
            m_terrain = kMagicTerrainRocklands;
            break;
        case MAGIC_CLOUDS:
            m_terrain = kMagicTerrainMagicClouds;
            break;
        case CURSED_GROUND:
            m_terrain = MAGIC_TERRAIN_CURSED_GROUND;
            if (g_game->m_f1f698 >= 2)
                break;
        case GARRISON:
            m_canCastSpells = 0;
            break;
        }
    }

    if (m_enemyHero != 0
        && m_enemyHero->isWieldingArtifact(ARTIFACT_ORB_OF_INHIBITION))
        m_canCastSpells = 0;
    initializeCreatures(baseModifier, m_enemyHero);
}

// E:\gamedcs\ai_combat.cpp:221
// DC 0x29f58 places these six named locals in the outer scope (its record
// order does not prove declaration order). Line 222 copies base_modifier
// before the tactics stores; retail likewise copies both dwords at entry.
// Preserve that initializer and the actual vector begin/end sort interface.

VA(0x00424120, 0x66E)  // dc-callgraph unique, dc 0x29f58
void type_AI_combat_data::initializeCreatures(double baseModifier, const hero* enemyHero)
{
    type_monster_data unit;
    long speedBonus;
    double hitPoints;
    double forceModifier = baseModifier;
    double archeryModifier;
    long hitBonus;

    m_tacticsAdvantage = 0;
    if (m_currentHero)
        m_tacticsAdvantage = m_currentHero->m_skillLevel[g_secondarySkillTactics];
    if (enemyHero) {
        m_tacticsAdvantage -= enemyHero->m_skillLevel[g_secondarySkillTactics];
        if (m_tacticsAdvantage < 0)
            m_tacticsAdvantage = 0;
    }

    if (m_currentHero) {
        long attack = m_currentHero->getPrimarySkill(0);
        long defense = m_currentHero->getPrimarySkill(1);
        if (enemyHero) {
            long enemyDefense = enemyHero->getPrimarySkill(1);
            attack -= min(attack, enemyDefense);
            long enemyAttack = enemyHero->getPrimarySkill(0);
            defense -= min(defense, enemyAttack);
        }
        forceModifier = sqrt(attack * 0.05 + 1.0)
                         * sqrt(defense * 0.05 + 1.0)
                         * baseModifier;
    }

    if (m_currentHero)
        archeryModifier = m_currentHero->getArcheryFactor() / 5.0;
    else
        archeryModifier = 0.2;

    speedBonus = 0;
    if (m_currentHero)
        speedBonus = m_currentHero->getCombatSpeedBonus();

    // DC263's nullary hit-point bonus precedes DC264's speed bonus. Retail
    // instead calls GetCombatSpeedBonus before this loop and the newer
    // GetHitPointBonus(int) per creature at 0x4242de. The audit's source-order
    // lead is this platform/ABI difference, not an omitted shared call.
    m_totalCombatValue = 0;
    for (long i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; i++) {
        TCreatureType creature = m_currentArmy->m_armyTypes[i];
        if (creature == CREATURE_NONE)
            continue;

        int creatureId = creature;
        const TCreatureTypeTraits& traits = g_creatureTypeTraits[creatureId];
        hitPoints = traits.m_hitPoints;
        if (m_currentHero) {
            hitBonus = m_currentHero->getHitPointBonus(creatureId);
            hitPoints += hitBonus;
        }

        long speed = traits.m_speed + speedBonus;
        unsigned int attributes = traits.m_attributes;

        unit.m_index = i;
        unit.m_type = creature;
        unit.m_number = m_currentArmy->m_numTroops[i];
        unit.m_originalNumber = unit.m_number;
        unit.m_speed = speed;
        unit.m_value = static_cast<long>(
            sqrt(hitPoints / traits.m_hitPoints)
            * traits.m_baseFightValue * forceModifier);
        unit.m_totalValue = unit.m_value * unit.m_number;
        unit.m_catagory = getCatagory(creature, speed);
        unit.m_combatValuePerHit = static_cast<double>(unit.m_value)
                               / hitPoints;
        unit.m_meleeModifier = 0.2;
        unit.m_finalMeleeModifier = 1.0;
        unit.m_rangedModifier = 0.0;

        if (unit.m_catagory == const_ranged) {
            if (!(attributes & g_ctaNoMeleePenalty)) {
                unit.m_meleeModifier = 0.1;
                unit.m_finalMeleeModifier = 0.7;
            }
            unit.m_rangedModifier = archeryModifier;
            if (attributes & g_ctaDoubleRangedValue)
                unit.m_rangedModifier *= 2.0;
            if (m_wallArcheryPenalty && creature != CREATURE_ARCH_MAGE)
                unit.m_rangedModifier /= 2.0;
        }

        m_totalCombatValue += unit.m_totalValue;
        m_creatures.push_back(unit);
    }

    std::sort(m_creatures.begin(), m_creatures.end());
}

VA(0x00424790, 0xE5)  // dc 0x2a470
void type_AI_combat_data::checkWallArcheryPenalty(const town* enemyTown)
{
    m_wallArcheryPenalty = 0;
    m_wallSpeedLimit = 0;
    if (enemyTown == 0)
        return;
    if (enemyTown->hasBuilding(CASTLE_FORT_ID, 0)) {
        m_wallArcheryPenalty = 1;
        m_wallSpeedLimit = 4;
    }
    if (enemyTown->hasBuilding(CASTLE_CITADEL_ID, 0)) {
        m_wallArcheryPenalty = 1;
        m_wallSpeedLimit = 5;
    }
    if (enemyTown->hasBuilding(CASTLE_CASTLE_ID, 0)) {
        m_wallArcheryPenalty = 1;
        m_wallSpeedLimit = 6;
    }
    if (m_currentHero) {
        if (m_currentHero->spellIsAvailable(SPELL_EARTHQUAKE)) {
            m_wallArcheryPenalty = 0;
            m_wallSpeedLimit = 0;
        }
        if (m_wallSpeedLimit > 0) {
            m_wallSpeedLimit = static_cast<short>(m_wallSpeedLimit - m_currentHero->m_skillLevel[g_secondarySkillSiegeBallistics]);
            if (m_wallSpeedLimit < 2)
                m_wallSpeedLimit = 2;
        }
    }
    if (m_wallSpeedLimit > 4)
        m_wallSpeedLimit = 4;
}

// E:\gamedcs\ai_combat.cpp:381
// DC 0x2a52c proves the const receiver and this source position. Retail
// expands the one call in initializeCreatures; that is not evidence for an
// explicit inline keyword. All eight declaration/order controls are score-
// flat; keep the ordinary canonical helper and its real read-only interface.
type_speed_catagory type_AI_combat_data::getCatagory(
    TCreatureType creature,
    long speed) const
{
    unsigned int attributes = g_creatureTypeTraits[creature].m_attributes;
    if (attributes & g_ctaShooter)
        return const_ranged;

    long catagory = (speed + 2 * (7 - m_tacticsAdvantage)) / speed;
    if (catagory > const_slow)
        catagory = const_slow;
    if (m_wallSpeedLimit > catagory && !(attributes & g_ctaFlying))
        catagory = m_wallSpeedLimit;
    type_speed_catagory result;
    memcpy(&result, &catagory, sizeof result);
    return result;
}

VA(0x00424880, 0xDB)  // dc 0x2a588
void type_AI_combat_data::adjustArmy(unsigned char dismissHero)
{
    if (m_totalCombatValue == 0) {
        for (short i = 0; i != armyGroup::ARMY_GROUP_SLOT_COUNT; i++)
            m_currentArmy->dismiss(i);
        if (m_currentHero && dismissHero)
            g_advManager->heroLoses(m_currentHero, 0);
        return;
    }
    for (short i = static_cast<short>(m_creatures.size()); i-- > 0; ) {
        type_monster_data unit = m_creatures[i];
        if (unit.m_index < 0)
            continue;
        if (unit.m_number == 0)
            m_currentArmy->dismiss(unit.m_index);
        else
            m_currentArmy->m_numTroops[unit.m_index] = unit.m_number;
    }
}

// E:\gamedcs\ai_combat.cpp:437
VA(0x00424960, 0x65)  // dc 0x2a644
long type_AI_combat_data::getFastestSpeed() const
{
    long fastest = 0;
    for (long i = m_creatures.size(); i-- > 0; )
        if (m_creatures[i].m_number > 0)
            fastest = max(fastest, m_creatures[i].m_speed);
    return fastest;
}

VA(0x004249d0, 0x218)  // dc 0x2a694
long type_AI_combat_data::getNextChainLightningTarget(long excluded, const type_AI_combat_data& defender, long start, long damage) const
{
    if (damage == 0)
        return -1;
    long i;
    for (i = start; i-- > 0; ) {
        if (excluded & (1 << i))
            continue;
        if (defender.m_creatures[i].getSpellDamage(SPELL_CHAIN_LIGHTNING, m_currentHero,
                                                  defender.m_currentHero, damage) > 0)
            break;
    }
    if (i >= 0)
        return i;
    for (i = start; (unsigned)++i < defender.m_creatures.size(); ) {
        if (excluded & (1 << i))
            continue;
        if (defender.m_creatures[i].getSpellDamage(SPELL_CHAIN_LIGHTNING, m_currentHero,
                                                  defender.m_currentHero, damage) > 0)
            break;
    }
    if ((unsigned)i < defender.m_creatures.size())
        return i;
    return -1;
}

// E:\gamedcs\ai_combat.cpp:498
// RECONSTRUCTED FROM ITS ONE INLINED COPY (dc 0x2a764) - no retail row:
// /Ob2 inlined the single call site (get_damage_spell_value 0x424e69)
// and OPT:REF dropped the COMDAT. Spelled `inline` so our obj does not
// carry a base-only function retail never shipped. It is the exact
// value-side mirror of cast_chain_lightning.
inline void type_AI_combat_data::getChainLightningValue(type_spell_choice& choice, const type_AI_combat_data& defender, long damage) const
{
    long excluded = 1 << choice.m_target;
    long target = choice.m_target;
    for (long i = 0; i < 3; i++) {
        damage /= 2;
        target = getNextChainLightningTarget(excluded, defender, target, damage);
        if (target < 0)
            break;
        choice.m_value += defender.m_creatures[target].getSpellDamage(
            choice.m_spell, m_currentHero, defender.m_currentHero, damage);
        excluded |= 1 << target;
    }
}

VA(0x00424bf0, 0x123)  // dc 0x2a7e4
void type_AI_combat_data::getAreaValue(type_spell_choice& choice, const type_AI_combat_data& defender, long damage, long extraTargets) const
{
    long targetIndex = defender.m_creatures[choice.m_target].m_index;
    for (unsigned i = 0; i < defender.m_creatures.size(); i++) {
        if (abs(targetIndex - defender.m_creatures[i].m_index) != 1)
            continue;
        long value = defender.m_creatures[i].getSpellDamage(choice.m_spell, m_currentHero,
                                                           defender.m_currentHero, damage);
        if (value <= 0)
            continue;
        choice.m_value += value;
        if (--extraTargets == 0)
            break;
    }
}

VA(0x00424d20, 0x290)  // dc 0x2a868
void type_AI_combat_data::getDamageSpellValue(type_spell_choice& choice, const type_AI_combat_data& defender) const
{
    long damage = choice.getMasteryValue()
                  + g_spellTraits[choice.m_spell].m_powerFactor * choice.m_power;
    for (long i = defender.m_creatures.size(); i-- > 0; ) {
        long value = defender.m_creatures[i].getSpellDamage(choice.m_spell, m_currentHero,
                                                           defender.m_currentHero, damage);
        if (value > choice.m_value) {
            choice.m_value = value;
            choice.m_target = i;
        }
    }
    if (choice.m_value <= 0)
        return;
    switch (choice.m_spell) {
    case SPELL_CHAIN_LIGHTNING:
        getChainLightningValue(choice, defender, damage);
        break;
    case SPELL_FROST_RING:
    case SPELL_FIREBALL:
    case SPELL_METEOR_SHOWER:
        getAreaValue(choice, defender, damage, 1);
        break;
    case SPELL_INFERNO:
        getAreaValue(choice, defender, damage, 2);
        break;
    }
}

VA(0x00424fb0, 0x145)  // dc 0x2a938
void type_AI_combat_data::castChainLightning(type_spell_choice& choice, type_AI_combat_data& defender, long damage) const
{
    type_AI_combat_data& targetData = defender;
    long excluded = 1 << choice.m_target;
    long target = choice.m_target;
    for (long i = 0; i < 3; i++) {
        damage /= 2;
        target = getNextChainLightningTarget(
            excluded, targetData, target, damage);
        if (target < 0)
            break;
        long value = targetData.m_creatures[target].getSpellDamage(
            choice.m_spell, m_currentHero, targetData.m_currentHero, damage);
        value = targetData.m_creatures[target].takeDamage(value);
        targetData.m_totalCombatValue -= value;
        excluded |= 1 << target;
    }
}

VA(0x00425100, 0x15A)  // dc 0x2a9e8
void type_AI_combat_data::castAreaEffect(type_spell_choice& choice, type_AI_combat_data& defender, long damage, long extraTargets) const
{
    long targetIndex = defender.m_creatures[choice.m_target].m_index;
    for (unsigned i = 0; i < defender.m_creatures.size(); i++) {
        if (abs(targetIndex - defender.m_creatures[i].m_index) != 1)
            continue;
        long value = defender.m_creatures[i].getSpellDamage(
            choice.m_spell, m_currentHero, defender.m_currentHero, damage);
        if (value <= 0)
            continue;
        defender.m_totalCombatValue -= defender.m_creatures[i].takeDamage(value);
        if (--extraTargets == 0)
            break;
    }
}

VA(0x00425260, 0x180)  // dc 0x2aa7c
void type_AI_combat_data::castDamageSpell(type_spell_choice& choice, type_AI_combat_data& defender) const
{
    long damage = choice.getMasteryValue()
                  + g_spellTraits[choice.m_spell].m_powerFactor * choice.m_power;
    long value = defender.m_creatures[choice.m_target].getSpellDamage(
        choice.m_spell, m_currentHero, defender.m_currentHero, damage);
    defender.m_totalCombatValue -= defender.m_creatures[choice.m_target].takeDamage(value);
    // The five-arm jump table at 0x4253cc: 0x13 chains, 0x14/0x15/0x17
    // hit one extra target, 0x16 hits two.
    switch (choice.m_spell) {
    case SPELL_CHAIN_LIGHTNING:
        castChainLightning(choice, defender, damage);
        break;
    case SPELL_FROST_RING:
    case SPELL_FIREBALL:
    case SPELL_METEOR_SHOWER:
        castAreaEffect(choice, defender, damage, 1);
        break;
    case SPELL_INFERNO:
        castAreaEffect(choice, defender, damage, 2);
        break;
    }
}

// E:\gamedcs\ai_combat.cpp:694; original has_creature, dc 0x2ab3c.
// Retail 0x425bd0 expands this const predicate before the one mana update.
// Keep the ordinary helper and source call; the previous pasted scan enlarged
// castSpell and changed its later mass-damage expansion decisions.
unsigned char type_AI_combat_data::hasCreature(TCreatureType creature) const
{
    for (long i = m_creatures.size(); i-- > 0; ) {
        if (m_creatures[i].m_type == creature && m_creatures[i].m_number > 0)
            return 1;
    }
    return 0;
}

VA(0x004253e0, 0x12F)  // dc 0x2ab88
long type_AI_combat_data::getMassDamageValue(type_spell_choice& choice, const hero* castingHero) const
{
    long value = 0;
    long damage = choice.getMasteryValue()
                  + g_spellTraits[choice.m_spell].m_powerFactor * choice.m_power;
    for (long i = m_creatures.size(); i-- > 0; )
        value += m_creatures[i].getSpellDamage(choice.m_spell, castingHero, m_currentHero, damage);
    return value;
}

// E:\gamedcs\ai_combat.cpp:731
// Retail expands this helper inside cast_spell and emits no out-of-line row.
// Its first nested get_mass_damage_value expands while the defender-side
// call stays out of line. DC 0x2ac18 proves the const receiver; the ordinary
// body retains that natural split without an invented inline qualifier.
void type_AI_combat_data::getMassDamageValue(
    type_spell_choice& choice,
    type_AI_combat_data& defender) const
{
    long ownDamage = getMassDamageValue(choice, m_currentHero);
    long defenderDamage = defender.getMassDamageValue(choice, m_currentHero);
    if (ownDamage < m_totalCombatValue && ownDamage < defenderDamage)
        choice.m_value = defenderDamage - ownDamage;
}

// E:\gamedcs\ai_combat.cpp:747
// Original cast_mass_damage_spell, dc 0x2ac58: one ordinary helper, two
// castSpell calls. Line 758 assigns take_damage's return to the running value;
// retail 0x425bd0 does so in both expansions (ESI/EDI <- EAX), then subtracts
// that capped value. Separate subscripts also reproduce retail's vector reload
// across getSpellDamage. The inherited cloned loops incorrectly carried the
// uncapped sum. Never restore that dataflow or a caller-specific clone for score.
// The 48-state boundary family and 96-state hasCreature follow-up exhausted
// cloned/canonical, retained/removed fences and the three dataflow/lifetime
// facts. Correct ordinary/unfenced + hasCreature scores castSpell 86.7910;
// keeping the first inherited fence scores 88.5664. Both old fences are
// removed. The remaining nested decisions close when castSpell recovers
// its separate getSummoningValue boundary, as documented below.
void type_AI_combat_data::castMassDamageSpell(
    type_spell_choice& choice,
    const hero* castingHero)
{
    long value = 0;
    long damage = choice.getMasteryValue()
                  + g_spellTraits[choice.m_spell].m_powerFactor * choice.m_power;
    for (long i = m_creatures.size(); i-- > 0; ) {
        value += m_creatures[i].getSpellDamage(
            choice.m_spell, castingHero, m_currentHero, damage);
        value = m_creatures[i].takeDamage(value);
        m_totalCombatValue -= value;
    }
}

// E:\gamedcs\ai_combat.cpp:768
// RECONSTRUCTED FROM ITS FOUR INLINED COPIES (dc 0x2ace4) - no retail
// row: /Ob2 inlined all four call sites in the two-side overload below
// and OPT:REF dropped the body. The const signature and ordinary definition
// preserve all four expansions; emission alone does not prove source inline.
void type_AI_combat_data::getEnchantmentValue(type_spell_choice& choice, const hero* castingHero) const
{
    unsigned char mass = !spellTargetsASingleArmy(choice.m_spell, choice.m_mastery);
    for (long i = m_creatures.size(); i-- > 0; ) {
        const type_monster_data& monster = m_creatures[i];
        long value = monster.getEnchantmentValue(choice, castingHero, m_currentHero);
        if (mass) {
            choice.m_value += value;
        } else if (choice.m_value < value) {
            choice.m_target = i;
            choice.m_value = value;
        }
    }
}

VA(0x00425510, 0x382)  // dc 0x2ad58
void type_AI_combat_data::getEnchantmentValue(type_spell_choice& choice, type_AI_combat_data& defender) const
{
    if (!defender.m_canCastSpells
        && (choice.m_spell == SPELL_DISPEL
            || choice.m_spell == SPELL_ANTI_MAGIC
            || choice.m_spell == SPELL_PROTECTION_FROM_AIR
            || choice.m_spell == SPELL_PROTECTION_FROM_FIRE
            || choice.m_spell == SPELL_PROTECTION_FROM_WATER
            || choice.m_spell == SPELL_PROTECTION_FROM_EARTH
            || choice.m_spell == SPELL_MAGIC_MIRROR
            || choice.m_spell == SPELL_CURE))
        return;
    if (choice.m_spell == SPELL_DISPEL) {
        getEnchantmentValue(choice, m_currentHero);
        if (choice.m_mastery < eMasteryAdvanced)
            return;
        // retail copies the whole 0x24-byte record with one rep movsd
        // (0x4256b7) and compares the ourChoice value after the second pass
        type_spell_choice ourChoice = choice;
        defender.getEnchantmentValue(choice, m_currentHero);
        if (choice.m_mastery != eMasteryAdvanced)
            return;
        if (ourChoice.m_value >= choice.m_value)
            return;
        choice.m_secondTargetHex = choice.m_target;
        choice.m_target = -1;
        return;
    }
    if (g_spellTraits[choice.m_spell].m_karma > 0)
        getEnchantmentValue(choice, m_currentHero);
    else
        defender.getEnchantmentValue(choice, m_currentHero);
}

VA(0x004258a0, 0x269)  // dc 0x2ae60
void type_AI_combat_data::castEnchantment(type_spell_choice& choice, const hero* castingHero, unsigned char increase)
{
    long value;
    if (spellTargetsASingleArmy(choice.m_spell, choice.m_mastery)) {
        value = m_creatures[choice.m_target].getEnchantmentValue(choice, castingHero, m_currentHero);
        m_creatures[choice.m_target].castEnchantment(value, increase);
        return;
    }
    for (long i = m_creatures.size(); i-- > 0; ) {
        value = m_creatures[i].getEnchantmentValue(choice, castingHero, m_currentHero);
        if (value > 0)
            m_creatures[i].castEnchantment(value, increase);
    }
}

// E:\gamedcs\ai_combat.cpp:871
VA(0x00425b10, 0xB4)  // dc 0x2af04
void type_AI_combat_data::castEnchantment(type_spell_choice& choice, type_AI_combat_data& defender)
{
    if (choice.m_spell == SPELL_DISPEL) {
        if (choice.m_mastery < eMasteryExpert) {
            if (choice.m_target < 0) {
                if (choice.m_secondTargetHex >= 0) {
                    choice.m_target = choice.m_secondTargetHex;
                    defender.castEnchantment(choice, m_currentHero, 0);
                }
            } else {
                castEnchantment(choice, m_currentHero, 1);
            }
        } else {
            castEnchantment(choice, m_currentHero, 1);
            defender.castEnchantment(choice, m_currentHero, 0);
        }
    } else if (g_spellTraits[choice.m_spell].m_karma > 0) {
        castEnchantment(choice, m_currentHero, 1);
    } else {
        defender.castEnchantment(choice, m_currentHero, 0);
    }
}

// The const receiver and reference choice are positive CodeView facts.
// DC912 dispatches 38/39 to resurrection and 60/66..69 to no-op exits;
// 924 scans downward, 926 gets the value, 927 compares strictly, and
// 929/930 store value before target. Retail expands this ordinary helper
// in castSpell; VC6 reduces its switch to the signed 38..39 range there.
void type_AI_combat_data::getSummoningValue(type_spell_choice& choice) const
{
    switch (choice.m_spell) {
    case SPELL_HYPNOTIZE:
    case SPELL_SUMMON_FIRE_ELEMENTAL:
    case SPELL_SUMMON_EARTH_ELEMENTAL:
    case SPELL_SUMMON_WATER_ELEMENTAL:
    case SPELL_SUMMON_AIR_ELEMENTAL:
        break;
    case SPELL_RESURRECTION:
    case SPELL_ANIMATE_DEAD:
        for (long i = m_creatures.size(); i-- > 0; ) {
            const type_monster_data& monster = m_creatures[i];
            long value = monster.getResurrectionValue(choice, m_currentHero);
            if (value > choice.m_value) {
                choice.m_value = value;
                choice.m_target = i;
            }
        }
        break;
    }
}

// DC943 has the same no-op/resurrection dispatch and 955 calls the
// selected monster's cast_resurrection. Preserve this ordinary boundary
// even though retail expands it and retains no separate body.
void type_AI_combat_data::castSummoning(type_spell_choice& choice)
{
    switch (choice.m_spell) {
    case SPELL_HYPNOTIZE:
    case SPELL_SUMMON_FIRE_ELEMENTAL:
    case SPELL_SUMMON_EARTH_ELEMENTAL:
    case SPELL_SUMMON_WATER_ELEMENTAL:
    case SPELL_SUMMON_AIR_ELEMENTAL:
        break;
    case SPELL_RESURRECTION:
    case SPELL_ANIMATE_DEAD:
        m_creatures[choice.m_target].castResurrection(choice, m_currentHero);
        break;
    }
}

// E:\gamedcs\ai_combat.cpp:965
VA(0x00425bd0, 0x593)  // anchor-global, dc 0x2b094
void type_AI_combat_data::castSpell(
    type_AI_combat_data& defender,
    type_speed_catagory round)
{
    if (m_totalCombatValue == 0 || m_mana == 0 || !m_canCastSpells)
        return;

    type_spell_choice bestChoice;
    unsigned char recantersCloak = 0;
    if (m_currentHero->isWieldingArtifact(g_artifactRecantersCloak))
        recantersCloak = 1;
    if (m_enemyHero
        && m_enemyHero->isWieldingArtifact(g_artifactRecantersCloak))
        recantersCloak = 1;

    register long spellPower = m_currentHero->getPrimarySkill(2);
    long spellDuration = spellPower + m_currentHero->getSpellDurationBonus();
    long bestManaCost;
    TSkillMastery mastery;

    for (SpellID spell = 10; spell < hero::NUM_SPELLS; spell++) {
        if (!m_currentHero->spellIsAvailable(spell))
            continue;

        if (g_spellTraits[spell].m_level > 1
            && m_terrain == MAGIC_TERRAIN_CURSED_GROUND)
            continue;
        if (g_spellTraits[spell].m_level > 2 && recantersCloak)
            continue;

        mastery = m_currentHero->getSpellLevel(spell, m_terrain);
        long manaCost = m_currentHero->getManaCost(spell, defender.m_currentArmy, m_terrain);
        if (manaCost > m_mana)
            continue;

        type_spell_choice choice(spell, mastery, spellPower, spellDuration);
        switch (g_spellTraits[spell].m_flags & g_aiSpellClassMask) {
        case g_aiSpellDirectDamage:
            getDamageSpellValue(choice, defender);
            break;
        case g_aiSpellOpeningDamage:
            if (round == const_very_fast)
                getDamageSpellValue(choice, defender);
            break;
        case g_aiSpellMassDamage:
            getMassDamageValue(choice, defender);
            break;
        case g_aiSpellEnchantment:
            getEnchantmentValue(choice, defender);
            break;
        case g_aiSpellResurrection:
            getSummoningValue(choice);
            break;
        }

        if (choice.m_value > bestChoice.m_value) {
            bestChoice = choice;
            bestManaCost = manaCost;
        }
    }

    if (bestChoice.m_spell == -1)
        return;

    m_mana -= bestManaCost;
    if (defender.m_currentHero && defender.hasCreature(CREATURE_FAMILIAR))
        defender.m_mana += bestManaCost / 5;

    switch (g_spellTraits[bestChoice.m_spell].m_flags
            & g_aiSpellClassMask) {
    case g_aiSpellDirectDamage:
    case g_aiSpellOpeningDamage:
        castDamageSpell(bestChoice, defender);
        return;
    case g_aiSpellMassDamage:
        castMassDamageSpell(bestChoice, m_currentHero);
        defender.castMassDamageSpell(
            bestChoice, m_currentHero);
        return;
    case g_aiSpellEnchantment:
        castEnchantment(bestChoice, defender);
        return;
    case g_aiSpellResurrection:
        castSummoning(bestChoice);
        return;
    }
}

// E:\gamedcs\ai_combat.cpp:1082
// Retail inlines every use; these statements are reconstructed from the
// repeated retail expansions. The Dreamcast contributes only the helper's
// name/signature and retains an out-of-line body in that build.
inline void type_AI_combat_data::castSpells(
    type_AI_combat_data& defender,
    type_speed_catagory round)
{
    if (getFastestSpeed() < defender.getFastestSpeed()) {
        defender.castSpell(*this, round);
        castSpell(defender, round);
    } else {
        castSpell(defender, round);
        defender.castSpell(*this, round);
    }
}

#if 0  // @carcass

// E:\gamedcs\ai_combat.cpp:1100
// No retained PC slot: castSpell ends at 0x426163, then alignment leads
// directly to inflictMeleeDamage at 0x426170. All other span bodies and
// both flanks are accounted for. DC 0x2b380 filters equality of category
// before take_damage; PC inflictDamage uses the retained range-based
// inflictMeleeDamage helper. Keep the old roster entry without a false claim.
DC_ONLY(0x2b380, 0x88)
// Before normalization (function): type_AI_combat_data::inflict_catagory_damage.
long type_AI_combat_data::inflictCatagoryDamage(long damage, type_speed_catagory catagory)
{
    // @stub
}

#endif  // @carcass

VA(0x00426170, 0x131)  // dc 0x2b408
long type_AI_combat_data::inflictMeleeDamage(long damage, long start, long speedLimit)
{
    long sum = 0;
    unsigned i;
    for (i = 0; i < m_creatures.size(); i++)
        if (m_creatures[i].m_catagory >= start && m_creatures[i].m_catagory <= speedLimit)
            sum += m_creatures[i].m_totalValue;
    for (i = 0; i < m_creatures.size(); i++) {
        if (m_creatures[i].m_catagory < start)
            continue;
        if (m_creatures[i].m_catagory > speedLimit)
            continue;
        long hits = m_creatures[i].m_totalValue;
        if (hits <= 0)
            continue;
        long share = static_cast<long>(static_cast<double>(hits) * damage / sum);
        sum -= hits;
        damage -= m_creatures[i].takeDamage(share);
        if (damage <= 0)
            break;
        if (sum <= 0)
            break;
    }
    return damage;
}

VA(0x004262b0, 0x4F)  // dc 0x2b5a4
void type_AI_combat_data::kill()
{
    m_totalCombatValue = 0;
    for (long i = m_creatures.size(); i-- > 0; ) {
        m_creatures[i].m_number = 0;
        m_creatures[i].m_totalValue = 0;
    }
}

VA(0x00426300, 0x8D)  // dc 0x2b5ec
void type_AI_combat_data::inflictDamage(long damage, long blockerSpeed)
{
    m_totalCombatValue -= damage;
    if (m_totalCombatValue <= 0) {
        kill();
        return;
    }
    damage = inflictMeleeDamage(damage, 1, blockerSpeed);
    if (damage != 0)
        inflictMeleeDamage(damage, 0, 4);
}

VA(0x00426390, 0xBB)  // dc 0x2b624
long type_AI_combat_data::getAttack(type_speed_catagory speedLimit, unsigned char shootersBlocked) const
{
    long value = 0;
    for (long i = m_creatures.size(); i-- > 0; ) {
        if (m_creatures[i].m_catagory > speedLimit)
            continue;
        if (!shootersBlocked && m_creatures[i].m_catagory == SPEED_CATAGORY_SHOOTER)
            value = static_cast<long>(m_creatures[i].m_number * m_creatures[i].m_value
                                      * m_creatures[i].m_rangedModifier + value);
        else
            value = static_cast<long>(m_creatures[i].m_number * m_creatures[i].m_value
                                      * m_creatures[i].m_meleeModifier + value);
    }
    return value;
}

VA(0x00426450, 0x71)  // dc 0x2b7bc
long type_AI_combat_data::getFinalMeleeValue() const
{
    long value = 0;
    for (long i = m_creatures.size(); i-- > 0; )
        value = static_cast<long>(m_creatures[i].m_totalValue * m_creatures[i].m_finalMeleeModifier + value);
    return value;
}

// E:\gamedcs\ai_combat.cpp:1224
// Retail inlines every use; these statements are reconstructed from the
// repeated retail expansions. The Dreamcast contributes only the helper's
// name/signature and retains an out-of-line body in that build.
inline void type_AI_combat_data::doRangedCombat(
    type_AI_combat_data& defender)
{
    long ourAttack = getAttack(const_ranged, 0);
    long theirAttack = defender.getAttack(const_ranged, 0);
    inflictDamage(theirAttack, 0);
    defender.inflictDamage(ourAttack, 0);
}

// E:\gamedcs\ai_combat.cpp:1240
// Retail inlines every use; the Dreamcast body survives out of line.
inline void type_AI_combat_data::doMeleeCombat(
    type_speed_catagory attackerSpeed,
    type_AI_combat_data& defender)
{
    long ourAttack = getAttack(attackerSpeed, 0);
    long theirAttack = defender.getAttack(const_slow, 1);
    inflictDamage(theirAttack, attackerSpeed);
    defender.inflictDamage(ourAttack, 0);
}

// E:\gamedcs\ai_combat.cpp:1255
// Retail inlines every use; the Dreamcast body survives out of line.
inline void type_AI_combat_data::doMeleeCombat(
    type_AI_combat_data& defender)
{
    long ourAttack = getAttack(const_slow, 1);
    long theirAttack = defender.getAttack(const_slow, 1);
    inflictDamage(theirAttack, 0);
    defender.inflictDamage(ourAttack, 0);
}

// DC class 0x5a07's constructor method list 0x5a0f gives the copy constructor
// attributes 0x003 (explicit), unlike its 0x103 compiler-generated assignment
// and destructor. Preserve this memberwise source boundary. The native vector
// member owns its own separate retained copy constructor at 0x4276c0.
inline type_AI_combat_data::type_AI_combat_data(
    const type_AI_combat_data& other)
    : m_creatures(other.m_creatures),
      m_terrain(other.m_terrain),
      m_mana(other.m_mana),
      m_canCastSpells(other.m_canCastSpells),
      m_totalCombatValue(other.m_totalCombatValue),
      m_tacticsAdvantage(other.m_tacticsAdvantage),
      m_currentHero(other.m_currentHero),
      m_currentArmy(other.m_currentArmy),
      m_enemyHero(other.m_enemyHero),
      m_wallArcheryPenalty(other.m_wallArcheryPenalty),
      m_wallSpeedLimit(other.m_wallSpeedLimit)
{
}

VA(0x004264d0, 0x2ED)  // dc 0x2b948
void type_AI_combat_data::doGeneralMelee(type_AI_combat_data& defender)
{
    float attacker = static_cast<float>(getFinalMeleeValue());
    float target = static_cast<float>(defender.getFinalMeleeValue());
    if (attacker == 0.0)
        return;
    if (target == 0.0)
        return;
    float ratio;
    if (attacker > target) {
        defender.kill();
        ratio = target / attacker + 0.05;
        inflictDamage(static_cast<long>(ratio * target), 0);
    } else {
        kill();
        ratio = attacker / target + 0.05;
        defender.inflictDamage(static_cast<long>(ratio * attacker), 0);
    }
}

VA(0x004267c0, 0x3FD)  // dc 0x2bad8
bool type_AI_combat_data::chooseMelee(
    const type_AI_combat_data& enemy,
    type_speed_catagory currentRound) const
{
    long index;
    for (index = m_creatures.size(); index-- > 0; ) {
        if (m_creatures[index].m_catagory > currentRound)
            continue;
        if (m_creatures[index].m_catagory == SPEED_CATAGORY_SHOOTER)
            continue;
        if (m_creatures[index].m_number > 0)
            break;
    }
    if (index < 0)
        return false;

    long bestValue;
    long bestIndex;
    long meleeRound;
    for (meleeRound = const_slow;
         meleeRound >= currentRound;
         meleeRound--) {
        type_AI_combat_data localData(*this);
        type_AI_combat_data localEnemy(enemy);

        long round;
        for (round = currentRound; round < meleeRound; round++) {
            if (localData.getTotal() <= 0)
                break;
            if (localEnemy.getTotal() <= 0)
                break;
            localData.castSpells(
                localEnemy, (type_speed_catagory)round);
            localData.doRangedCombat(localEnemy);
        }

        for (round = meleeRound; round < const_slow; round++) {
            if (localData.getTotal() <= 0)
                break;
            if (localEnemy.getTotal() <= 0)
                break;
            localData.castSpells(
                localEnemy, (type_speed_catagory)round);
            localData.doMeleeCombat(
                (type_speed_catagory)round, localEnemy);
        }

        localData.doGeneralMelee(localEnemy);
        long value = localData.getTotal();
        if (value == 0)
            value = -localEnemy.getTotal();
        if (meleeRound == const_slow || bestValue < value) {
            bestValue = value;
            bestIndex = meleeRound;
        }
    }
    return static_cast<short>(bestIndex) == currentRound;
}

VA(0x00426bc0, 0x224)  // dc 0x2bc40
void type_AI_combat_data::simulateCombat(type_AI_combat_data& defender)
{
    for (long round = 1; round < 4; round++) {
        if (getTotal() <= 0)
            break;
        if (defender.getTotal() <= 0)
            break;
        unsigned char weMelee = chooseMelee(
            defender, (type_speed_catagory)round);
        unsigned char theyMelee = defender.chooseMelee(
            *this, (type_speed_catagory)round);
        castSpells(defender, (type_speed_catagory)round);
        if (weMelee) {
            if (theyMelee)
                doMeleeCombat(defender);
            else
                doMeleeCombat(
                    (type_speed_catagory)round, defender);
        } else if (theyMelee) {
            defender.doMeleeCombat(*this);
        } else {
            doRangedCombat(defender);
        }
    }
    doGeneralMelee(defender);
}

// E:\gamedcs\ai_combat.cpp:1398
static void doEagleEye(hero* winner, hero* loser)
{
    if (winner->m_skillLevel[g_secondarySkillEagleEye] > 0
        && winner->isWieldingArtifact(ARTIFACT_SPELLBOOK)) {
        for (short spell = 0; spell < hero::NUM_SPELLS; ++spell) {
            if (!loser->spellIsAvailable(spell)
                || winner->spellIsAvailable(spell))
                continue;
            const SSpellTraits& traits = g_spellTraits[spell];
            if (winner->m_skillLevel[g_secondarySkillEagleEye] + 1 < traits.m_level)
                continue;
            if (!(traits.m_flags & 1))
                continue;
            if (traits.m_level <= winner->m_skillLevel[g_secondarySkillWisdom] + 2) {
                winner->addSpell(spell);
                return;
            }
        }
    }
}

// E:\gamedcs\ai_combat.cpp:1424
// The DC `short amount` prototype does not survive retail bytes. The second
// (edx) argument is an armyGroup the
// body walks slot by slot - armies[i] at [esi], numTroops[i] at
// [esi+0x1c], esi stepping by 4 over seven iterations (0x426e36
// .. 0x426e89) - i.e. the losing side's stacks.
VA(0x00426df0, 0xED)  // corroborates (hd-crossbuild + ida), dc 0x2bd6c
void createSkeletons(const hero* currentHero, const armyGroup* deadArmy, armyGroup& destination)
{
    float factor = currentHero->getNecromancyFactor(1);
    if (factor <= 0.0f)
        return;
    factor += 0.02f;
    TCreatureType skeleton = const_cast<hero*>(currentHero)->getNecromancyCreature();
    long total = 0;
    long skeletonHitPoints = g_creatureTypeTraits[skeleton].m_hitPoints;
    float skeletonHitPointsF = static_cast<float>(skeletonHitPoints);
    for (long i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; i++) {
        long creature = deadArmy->m_armies[i];
        long count = deadArmy->m_numTroops[i];
        long hitPoints = g_creatureTypeTraits[creature].m_hitPoints;
        if (hitPoints > skeletonHitPoints)
            hitPoints = skeletonHitPoints;
        long raised = static_cast<long>(static_cast<float>(hitPoints * count)
                                        * factor / skeletonHitPointsF);
        if (raised > count)
            raised = count;
        total += raised;
    }
    if (total < 1)
        total = 1;
    destination.add(skeleton, total, -1);
}

// E:\gamedcs\ai_combat.cpp:1440, dc 0x2be54
VA(0x00426ee0, 0x1D8)  // anchor-global, dc 0x2be54
void type_AI_combat_data::doAftermath(type_AI_combat_data& defender, town* enemyTown)
{
    unsigned char retreated = 0;
    armyGroup* defeatedArmy = defender.getArmy();
    hero* defeatedHero = defender.getHero();

    if (m_currentHero)
        m_currentHero->m_mana = static_cast<short>(m_mana);
    if (defeatedHero)
        defeatedHero->m_mana = static_cast<short>(defender.m_mana);

    if (m_totalCombatValue > 0) {
        if (m_currentHero) {
            int experience;
            if (defeatedHero && random(0, 100) < 60) {
                retreated = 1;
                experience = g_game->experienceValueOfStack(
                    defeatedArmy, 0);
            } else {
                experience = g_game->experienceValueOfStack(
                    defeatedArmy, defeatedHero);
            }
            experience = static_cast<int>(
                m_currentHero->getExperienceBonusFactor() * experience);
            m_currentHero->giveExperience(experience, 1, 1);

            if (defeatedHero)
                defeatedHero->removeArtifact(ARTIFACT_HOLY_GRAIL);
            if (!retreated && defeatedHero)
                defeatedHero->transferArtifacts(m_currentHero);

            if (enemyTown)
                g_game->claimTown(enemyTown->m_id, m_currentHero->m_owner, 0, 1);
        }
    } else if (m_currentHero) {
        m_currentHero->removeArtifact(ARTIFACT_HOLY_GRAIL);
    }

    adjustArmy(1);
    defender.adjustArmy(1);

    if (m_totalCombatValue > 0 && m_currentHero) {
        createSkeletons(m_currentHero, defeatedArmy, *m_currentArmy);

        if (defeatedHero)
            doEagleEye(m_currentHero, defeatedHero);
    }

    if (m_currentHero)
        m_currentHero->applyBattleWinTemps();
    if (defeatedHero)
        defeatedHero->applyBattleLossTemps();
}

// E:\gamedcs\ai_combat.cpp:1511
// EH-bearing: the two stack-local type_AI_combat_data objects give the
// function a /GX frame (push -1 / push <ehfuncinfo> / mov eax,fs:[0])
// and the two `mov [ebp-4], state` writes between the constructors.
VA(0x004270c0, 0x149)  // anchor-global, dc 0x2c004
unsigned char aiQuickCombat(hero* attackingHero, hero* defendingHero, armyGroup& defendingArmy, town* defendingTown, NewmapCell* cell)
{
    float attackerModifier = random(75, 125) / 100.0f;
    float defenderModifier = random(75, 125) / 100.0f;
    type_AI_combat_data attacker(attackingHero, &attackingHero->m_army,
                                 attackerModifier, defendingHero,
                                 defendingTown, cell);
    type_AI_combat_data defender(defendingHero, &defendingArmy,
                                 defenderModifier, attackingHero, 0, cell);
    attacker.simulateCombat(defender);
    if (attacker.getTotal() > 0) {
        attacker.doAftermath(defender, defendingTown);
        return 1;
    }
    defender.doAftermath(attacker, 0);
    return 0;
}

// E:\gamedcs\ai_combat.cpp:1539
// EH-bearing, same shape as AI_quick_combat.
VA(0x00427210, 0x113)  // anchor-global, dc 0x2c140
void aiAutoCombat(hero* attackingHero, hero* defendingHero, armyGroup& attackingArmy, armyGroup& defendingArmy, const town* defendingTown, NewmapCell* cell)
{
    float attackerLuck = random(75, 125) / 100.0f;
    float defenderModifier = random(75, 125) / 100.0f;
    type_AI_combat_data attacker(attackingHero, &attackingArmy,
                                 attackerLuck, defendingHero,
                                 defendingTown, cell);
    type_AI_combat_data defender(defendingHero, &defendingArmy,
                                 defenderModifier, attackingHero, 0, cell);
    attacker.simulateCombat(defender);
    attacker.adjustArmy(0);
    defender.adjustArmy(0);
    attackingHero->m_mana = static_cast<short>(attacker.getMana());
    if (defendingHero)
        defendingHero->m_mana = static_cast<short>(defender.getMana());
}

VA(0x00427330, 0x318)  // dc 0x2c27c
long aiValueOfCombat(const hero* attackingHero, const hero* defendingHero,
                        const armyGroup& defendingArmy,
                        const town* defendingTown, NewmapCell* cell)
{
    armyGroup localArmy = attackingHero->m_army;
    double aggression = attackingHero->getAggression();
    armyGroup localDefender = defendingArmy;
    double defenderLuck = 1.25;
    unsigned char humanCombat = 0;

    if (g_game->m_mapHeader.m_victoryCondition.m_type == VICTORY_CONDITION_DEFEAT_HERO
        && g_game->m_mapHeader.m_victoryCondition.m_heroId == attackingHero->m_id)
        aggression *= 0.5;

    if (attackingHero
        && attackingHero->belongsToHuman())
        humanCombat = 1;
    if (defendingHero
        && defendingHero->belongsToHuman())
        humanCombat = 1;
    if ((defendingTown && defendingTown->m_owner >= 0
         && g_game->isHuman(defendingTown->m_owner))
        || humanCombat)
        defenderLuck = g_defenseEstimates[g_game->m_setup.m_difficulty];

    type_AI_combat_data attacker(attackingHero, &localArmy, aggression,
                                 defendingHero, defendingTown, cell);
    type_AI_combat_data defender(defendingHero, &localDefender,
                                 defenderLuck, attackingHero, 0, cell);
    attacker.simulateCombat(defender);
    if (attacker.getTotal() == 0)
        return -1000000000;

    attacker.adjustArmy(1);
    long experience = g_game->experienceValueOfStack(&defendingArmy, 0);
    float experienceValue = static_cast<float>(experience);
    experience = static_cast<long>(
        experienceValue
        * const_cast<hero*>(attackingHero)->getExperienceBonusFactor());
    long value = static_cast<long>(
        valueOfExperience(attackingHero, localArmy)
        * experience);

    // DC1612 first computes get_creature_total for its short-amount helper.
    // Complete passes the whole defeated army to its species-aware helper.
    createSkeletons(attackingHero, &defendingArmy, localArmy);
    long originalValue = attackingHero->m_army.getAIValue();
    long armyLoss = originalValue - localArmy.getAIValue();
    value -= armyLoss;
    if (value < 0 && armyLoss * 4 < originalValue)
        value = 0;

    if (defendingHero || defendingTown) {
        short defenderPlayer = -1;
        if (defendingHero)
            defenderPlayer = defendingHero->m_owner;
        if (defendingTown)
            defenderPlayer = defendingTown->m_owner;
        value = static_cast<long>(
            static_cast<float>(defendingArmy.getAIValue())
            * type_AI_player::getAttackBonus(defenderPlayer)
            + static_cast<float>(value));

        if (defendingHero) {
            if (g_game->m_mapHeader.m_victoryCondition.m_type
                    == VICTORY_CONDITION_DEFEAT_HERO
                && g_game->m_mapHeader.m_victoryCondition.m_heroId == defendingHero->m_id
                && static_cast<unsigned char>(g_game->m_mapHeader.m_victoryCondition
                    .appliesToPlayer(g_netLocalGamePos)))
                value += 5000000;
            if (g_game->m_mapHeader.m_lossCondition.m_type == LOSS_CONDITION_LOSE_HERO
                && g_game->m_mapHeader.m_lossCondition.m_heroId == defendingHero->m_id)
                value += 5000000;
        }
    }
    return value;
}

#if 0  // @carcass

// E:\gamedcs\ai_combat.cpp:1656
// DC builds a temporary armyGroup and delegates to the five-argument
// AI_value_of_combat. No retained PC slot fits: the full evaluator ends
// at 0x427648 and the first approximate-strength overload starts at
// 0x427650; all surrounding bodies and native STL tails are identified.
DC_ONLY(0x2c5e8, 0x2C)
long aiValueOfCombat(const hero* attacking_hero, TCreatureType type, long size, NewmapCell* cell)
{
    // @stub
}

#endif  // @carcass

VA(0x00427650, 0x33)  // dc 0x2c614
long aiApproximateStrength(const hero* currentHero)
{
    return aiApproximateStrength(currentHero, currentHero->m_army);
}

// E:\gamedcs\ai_combat.cpp:1674
// LOCATED (hd-crossbuild + ida): same body with the group in edx.
VA(0x00427690, 0x2F)  // corroborates (hd-crossbuild + ida), dc 0x2c628
long aiApproximateStrength(const hero* currentHero, const armyGroup& currentArmy)
{
    long value = currentArmy.getAIValue();
    if (currentHero == 0)
        return value;
    return static_cast<long>(currentHero->getCombatValueModifier() * value);
}

#if 0  // @carcass

// E:\gamedcs\hero.h:669
DC_ONLY(0x2c668, 0x28)
int hero::getPrimarySkill(int skill)
{
    // @stub
}

// E:\gamedcs\hero.h:960
DC_ONLY(0x2c690, 0x8)
float hero::getAggression()
{
    // @stub
}

// E:\gamedcs\ai_combat.h:62
DC_ONLY(0x2c698, 0xA)
unsigned char type_monster_data::operator<(const type_monster_data* arg)
{
    // @stub
}

// E:\gamedcs\ai_combat.h:245
DC_ONLY(0x2c6a4, 0x4)
long type_AI_combat_data::getMana()
{
    // @stub
}

// E:\gamedcs\ai_combat.h:250
DC_ONLY(0x2c6a8, 0x4)
armyGroup* type_AI_combat_data::getArmy()
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x004276c0, 0x87, VECTOR_COPY_CTOR, type_monster_data)

VA_COMPGEN(0x00427750, 0x21, VECTOR_SIZE, type_monster_data)

#if 0  // @carcass

// E:\gamedcs\ai_combat.h:260
DC_ONLY(0x2c6b0, 0x4)
hero* type_AI_combat_data::getHero()
{
    // @stub
}

// E:\gamedcs\ai_combat.cpp:1356. No retail slot was found; the previously
// assigned 0x4276c0 body is the vector copy constructor above.
DC_ONLY(0x2c6b4, 0x54)
void type_AI_combat_data::type_AI_combat_data(const type_AI_combat_data* __that)
{
    // @stub
}

// E:\gamedcs\ai_combat.cpp:1356
DC_ONLY(0x2c708, 0x18)
void type_AI_combat_data::~type_AI_combat_data()
{
    // @stub
}

#endif  // @carcass

// The rows below are the DREAMCAST build's STLport instantiations and
// stay DC_ONLY: retail links Dinkumware, so these particular symbols
// have no retail counterpart at all (P2.3, answered 2026-08-07).
#if 0  // @carcass  (Dreamcast STLport surface - DC_ONLY by construction)

// ..\stlport\stl_vector.h:179
DC_ONLY(0x2c720, 0x4)
type_monster_data* std::vector<type_monster_data,std::allocator<type_monster_data> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0x2c724, 0x4)
type_monster_data* std::vector<type_monster_data,std::allocator<type_monster_data> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x2c728, 0x20)
unsigned std::vector<type_monster_data,std::allocator<type_monster_data> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x2c748, 0x24)
type_monster_data* std::vector<type_monster_data,std::allocator<type_monster_data> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:204
DC_ONLY(0x2c76c, 0x24)
const type_monster_data* std::vector<type_monster_data,std::allocator<type_monster_data> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x2c790, 0x1C)
void std::vector<type_monster_data,std::allocator<type_monster_data> >::vector<type_monster_data,std::allocator<type_monster_data> >(const std::allocator<type_monster_data>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:236
DC_ONLY(0x2c7ac, 0x60)
void std::vector<type_monster_data,std::allocator<type_monster_data> >::vector<type_monster_data,std::allocator<type_monster_data> >(const std::vector<type_monster_data,std::allocator<type_monster_data>* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x2c80c, 0x28)
void std::vector<type_monster_data,std::allocator<type_monster_data> >::~vector<type_monster_data,std::allocator<type_monster_data> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x2c834, 0x3C)
void std::vector<type_monster_data,std::allocator<type_monster_data> >::push_back(const type_monster_data* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x2c870, 0x4)
void std::allocator<type_monster_data>::allocator<type_monster_data>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x2c874, 0x4)
void std::allocator<type_monster_data>::~allocator<type_monster_data>()
{
    // @stub
}

// ..\stlport\stl_vector.h:153
DC_ONLY(0x2c878, 0x8)
std::allocator<type_monster_data> std::vector<type_monster_data,std::allocator<type_monster_data> >::get_allocator(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_vector.h:180
DC_ONLY(0x2c880, 0x4)
const type_monster_data* std::vector<type_monster_data,std::allocator<type_monster_data> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x2c884, 0x2C)
void std::_Vector_base<type_monster_data,std::allocator<type_monster_data> >::_Vector_base<type_monster_data,std::allocator<type_monster_data> >(const std::allocator<type_monster_data>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:94
DC_ONLY(0x2c8b0, 0x4C)
void std::_Vector_base<type_monster_data,std::allocator<type_monster_data> >::_Vector_base<type_monster_data,std::allocator<type_monster_data> >(unsigned __n, const std::allocator<type_monster_data>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x2c8fc, 0x40)
void std::_Vector_base<type_monster_data,std::allocator<type_monster_data> >::~_Vector_base<type_monster_data,std::allocator<type_monster_data> >()
{
    // @stub
}

// ..\stlport\stl_string.h:101
DC_ONLY(0x2c93c, 0x18)
void std::_STL_alloc_proxy<type_monster_data *,type_monster_data,std::allocator<type_monster_data> >::~_STL_alloc_proxy<type_monster_data *,type_monster_data,std::allocator<type_monster_data> >()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x2c954, 0xC)
void std::_STL_alloc_proxy<type_monster_data *,type_monster_data,std::allocator<type_monster_data> >::_STL_alloc_proxy<type_monster_data *,type_monster_data,std::allocator<type_monster_data> >(const std::allocator<type_monster_data>* __a, type_monster_data** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x2c960, 0x28)
type_monster_data* std::_STL_alloc_proxy<type_monster_data *,type_monster_data,std::allocator<type_monster_data> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x2c988, 0x2C)
void std::_STL_alloc_proxy<type_monster_data *,type_monster_data,std::allocator<type_monster_data> >::deallocate(type_monster_data* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x2c9b4, 0x28)
type_monster_data* std::allocator<type_monster_data>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x2c9dc, 0x20)
void std::allocator<type_monster_data>::deallocate(type_monster_data* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x2c9fc, 0xDC)
void std::vector<type_monster_data,std::allocator<type_monster_data> >::_M_insert_overflow(type_monster_data* __position, const type_monster_data* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_algo.h:624
DC_ONLY(0x2cad8, 0x64)
void std::sort(type_monster_data* __first, type_monster_data* __last)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x2cb3c, 0x38)
type_monster_data* std::uninitialized_copy(const type_monster_data* __first, const type_monster_data* __last, type_monster_data* __result)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x2cb74, 0x30)
void std::destroy(type_monster_data* __first, type_monster_data* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x2cba4, 0x44)
void std::construct(type_monster_data* __p, const type_monster_data* __value)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x2cbe8, 0x4)
std::allocator<type_monster_data>* std::__stl_alloc_rebind(std::allocator<type_monster_data>* __a, const type_monster_data* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x2cbec, 0x38)
type_monster_data* std::uninitialized_copy(type_monster_data* __first, type_monster_data* __last, type_monster_data* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x2cc24, 0x38)
type_monster_data* std::uninitialized_fill_n(type_monster_data* __first, unsigned __n, const type_monster_data* __x)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x2cc5c, 0x4)
type_monster_data* std::value_type(const type_monster_data* __formal)
{
    // @stub
}

// ..\stlport\stl_algo.c:1081
DC_ONLY(0x2cc60, 0xF4)
void std::__introsort_loop(type_monster_data* __first, type_monster_data* __last, type_monster_data* __formal, int __depth_limit)
{
    // @stub
}

// ..\stlport\stl_algo.c:1057
DC_ONLY(0x2cd54, 0x5C)
void std::__final_insertion_sort(type_monster_data* __first, type_monster_data* __last)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x2cdb0, 0x1C)
type_monster_data* std::__uninitialized_copy(const type_monster_data* __first, const type_monster_data* __last, type_monster_data* __result, type_monster_data* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x2cdcc, 0x1C)
void std::__destroy(type_monster_data* __first, type_monster_data* __last, type_monster_data* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x2cde8, 0x1C)
type_monster_data* std::__uninitialized_copy(type_monster_data* __first, type_monster_data* __last, type_monster_data* __result, type_monster_data* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x2ce04, 0x1C)
type_monster_data* std::__uninitialized_fill_n(type_monster_data* __first, unsigned __n, const type_monster_data* __x, type_monster_data* __formal)
{
    // @stub
}

// ..\stlport\stl_algo.h:664
DC_ONLY(0x2ce20, 0x38)
void std::partial_sort(type_monster_data* __first, type_monster_data* __middle, type_monster_data* __last)
{
    // @stub
}

// ..\stlport\stl_algo.c:44
DC_ONLY(0x2ce58, 0x68)
const type_monster_data* std::__median(const type_monster_data* __a, const type_monster_data* __b, const type_monster_data* __c)
{
    // @stub
}

// ..\stlport\stl_algo.c:923
DC_ONLY(0x2cec0, 0x6C)
type_monster_data* std::__unguarded_partition(type_monster_data* __first, type_monster_data* __last, type_monster_data __pivot)
{
    // @stub
}

// ..\stlport\stl_algo.c:1012
DC_ONLY(0x2cf2c, 0x68)
void std::__insertion_sort(type_monster_data* __first, type_monster_data* __last)
{
    // @stub
}

// ..\stlport\stl_algo.c:1035
DC_ONLY(0x2cf94, 0x30)
void std::__unguarded_insertion_sort(type_monster_data* __first, type_monster_data* __last)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x2cfc4, 0x3C)
type_monster_data* std::__uninitialized_copy_aux(const type_monster_data* __first, const type_monster_data* __last, type_monster_data* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x2d000, 0x30)
void std::__destroy_aux(type_monster_data* __first, type_monster_data* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x2d030, 0x3C)
type_monster_data* std::__uninitialized_copy_aux(type_monster_data* __first, type_monster_data* __last, type_monster_data* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x2d06c, 0x3C)
type_monster_data* std::__uninitialized_fill_n_aux(type_monster_data* __first, unsigned __n, const type_monster_data* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_algo.c:1432
DC_ONLY(0x2d0a8, 0xF4)
void std::__partial_sort(type_monster_data* __first, type_monster_data* __middle, type_monster_data* __last, type_monster_data* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:107
DC_ONLY(0x2d19c, 0x30)
void std::iter_swap(type_monster_data* __a, type_monster_data* __b)
{
    // @stub
}

// ..\stlport\stl_algo.c:987
DC_ONLY(0x2d1cc, 0xAC)
void std::__linear_insert(type_monster_data* __first, type_monster_data* __last, type_monster_data __val)
{
    // @stub
}

// ..\stlport\stl_algo.c:1028
DC_ONLY(0x2d278, 0x98)
void std::__unguarded_insertion_sort_aux(type_monster_data* __first, type_monster_data* __last, type_monster_data* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x2d310, 0x1C)
void std::destroy(type_monster_data* __pointer)
{
    // @stub
}

// ..\stlport\stl_heap.c:207
DC_ONLY(0x2d32c, 0x40)
void std::make_heap(type_monster_data* __first, type_monster_data* __last)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x2d36c, 0x4)
int* std::distance_type(const type_monster_data* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.h:64
DC_ONLY(0x2d370, 0x88)
void std::__pop_heap(type_monster_data* __first, type_monster_data* __last, type_monster_data* __result, type_monster_data __value, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.h:108
DC_ONLY(0x2d3f8, 0x3C)
void std::sort_heap(type_monster_data* __first, type_monster_data* __last)
{
    // @stub
}

// ..\stlport\stl_algobase.h:96
DC_ONLY(0x2d434, 0x18)
void std::__iter_swap(type_monster_data* __a, type_monster_data* __b, type_monster_data* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0x2d44c, 0x50)
type_monster_data* std::copy_backward(type_monster_data* __first, type_monster_data* __last, type_monster_data* __result)
{
    // @stub
}

// ..\stlport\stl_algo.c:960
DC_ONLY(0x2d49c, 0x84)
void std::__unguarded_linear_insert(type_monster_data* __last, type_monster_data __val)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x2d520, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_heap.c:192
DC_ONLY(0x2d524, 0xC8)
void std::__make_heap(type_monster_data* __first, type_monster_data* __last, type_monster_data* __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.c:112
DC_ONLY(0x2d5ec, 0x110)
void std::__adjust_heap(type_monster_data* __first, int __holeIndex, int __len, type_monster_data __value)
{
    // @stub
}

// ..\stlport\stl_heap.c:142
DC_ONLY(0x2d6fc, 0x30)
void std::pop_heap(type_monster_data* __first, type_monster_data* __last)
{
    // @stub
}

// ..\stlport\stl_algobase.h:79
DC_ONLY(0x2d72c, 0x82)
void std::swap(type_monster_data* __a, type_monster_data* __b)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x2d7b0, 0xC)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const type_monster_data* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0x2d7bc, 0x68)
type_monster_data* std::__copy_backward(type_monster_data* __first, type_monster_data* __last, type_monster_data* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.c:44
DC_ONLY(0x2d824, 0xC0)
void std::__push_heap(type_monster_data* __first, int __holeIndex, int __topIndex, type_monster_data __value)
{
    // @stub
}

// ..\stlport\stl_heap.c:134
DC_ONLY(0x2d8e4, 0x98)
void std::__pop_heap_aux(type_monster_data* __first, type_monster_data* __last, type_monster_data* __formal)
{
    // @stub
}

#endif  // @carcass

// push_back on the combat AI's monster table retains Dinkumware's
// three-argument vector::insert specialization in ai_combat.obj. Byte-
// verified against the emitted COMDAT at 0.987 mnemonic agreement over 740
// bytes - the largest single row the COMDAT pass recovered.
VA_COMPGEN(0x00427780, 0x2E4, VECTOR_INSERT, type_monster_data)

// COMDAT pairing: std::_Sort<type_monster_data>, agreement 0.983.
VA_COMPGEN(0x00427a90, 0x19B, STD_SORT, type_monster_data)

// COMDAT pairing: std::_Unguarded_partition<type_monster_data>, 0.913.
VA_COMPGEN(0x00427c30, 0x64, STD_UNGUARDED_PARTITION, type_monster_data)
