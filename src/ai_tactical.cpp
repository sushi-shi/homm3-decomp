// ai_tactical.cpp - E:\gamedcs\ai_tactical.cpp (compiland ai_tactical.obj)
#include <va.h>
#include <math.h>
#include <string.h>
#include "ai_tactical.h"
#include "findpath.h"
#include "game.h"
// consider_spell's summon arm asks get_elemental_type which creature
// the spell would put on the field.
#include "spells.h"
// cast_spell jitters the chosen spell's value through Random().
#include "misc.h"
// The compiler-generated army copy constructor the retail link parked in
// this compiland (0x437a00) instantiates TResourceHandle<CSprite> and
// TResourceHandle<sample>, whose refcounting copy reaches
// resource::ReferenceCount - so both resource types have to be COMPLETE
// here. No other TU in the tree copies an `army`.
#include "csprite.h"
#include "sample.h"
#include "includes.h"

// The reference-returning min/max this TU's call sites were compiled
// against. They resemble <xutility>'s `_cpp_min`/`_cpp_max` (the
// <algorithm> min/max macros expand to those) but they are NOT the
// header's declaration: XUTILITY:71-83 takes both operands as
// `const _Ty&`, and that signature is byte-REFUTED here.

// The by-value parameters are what retail's code proves, not a
// convenience. A `const _Ty&` parameter binds an lvalue argument's own
// address and emits no copy; retail copies BOTH operands into fresh
// slots even when both are lvalues:
//   - get_fastest_speed 0x4249a1 (ai_combat) homes the enregistered
//     accumulator AND `monsters[i].speed`, whose address is already in
//     eax, before the compare. Under `const _Ty&` our CL passes eax
//     straight through and homes the accumulator once (100.00 ->
//     78.06).
//   - get_breath_bonus 0x436760 stores `damage` into a dead parameter
//     slot INSIDE the `simulated` branch, at the call - a temporary,
//     not a home; under `const _Ty&` the store sinks to the variable's
//     definition (100.00 -> 97.19).
// Switching the whole pair to `const _Ty&` was measured 2026-08-08 and
// cost eight exact functions (ai_combat get_resurrection_value,
// get_spell_damage, get_fastest_speed,
// get_next_chain_lightning_target, get_damage_spell_value,
// get_mass_damage_value; ai_tactical get_multi_head_bonus,
// get_breath_bonus) while raising nothing. Declared file-locally so
// the TU does not pull the STL surface in; ai_combat.cpp carries the
// same pair.
// Artifact 83, the id cast_spell (0x43c800) asks
// hero::IsWieldingArtifact for on BOTH heroes before refusing every
// spell above traits level 2 - Recanter's Cloak's rule and no other
// artifact's. TU-LOCAL for the reason ai_combat.cpp's identical
// constant is (and this is the third TU to carry it): adding an
// enumerator to armygrp.h's artifact roster measurably perturbs
// unrelated units through the shared type environment, and this one
// would also collide with ai_spellvalue.h's own file-scope constant in
// every TU that takes both.
const int g_artifactRecantersCloak = 0x53;

// The AI's luck/morale weights. Retail LOADS all four from .rdata
// instead of folding them into immediates, which is what pins them as
// named constants rather than literals; the morale pair is double and
// the luck pair is float (AI_value_of_luck 0x435960 widens each with
// fld dword / fstp qword before the call).
DATA(0x0063b780) const double g_aiGoodMoraleValue = 0.0173;
DATA(0x0063b788) const double g_aiBadMoraleValue = -0.0833;
DATA(0x0063b790) const float g_aiGoodLuckValue = 0.0173f;
DATA(0x0063b794) const float g_aiBadLuckValue = -0.0122f;

// The odds ladder: how lopsided the two sides' live combat values have
// to be before the AI shortens its planning horizon. The
// type_AI_combat_parameters ctor (0x435ec0) walks it with
// `fcomp qword ptr [8*odds + 0x63b798]` and stops at the first rung the
// high/low ratio reaches, so `odds` lands in 1..7. Values read from the
// hash-verified image at RVA 0x23b798, not from a roster.
DATA(0x0063b798) const double g_aiOddsLadder[6] = {
    2.6, 1.9, 1.5, 1.31, 1.2, 1.13
};

VA(0x00435830, 0x1E)  // dc 0x3c55c
double aiValueOfMorale(long morale, long change)
{
    return valueOfLuckAndMorale(morale, change, g_aiGoodMoraleValue,
                                   g_aiBadMoraleValue);
}

VA(0x00435850, 0x102)  // dc 0x3c29c
double valueOfLuckAndMorale(long value, long change, double goodValueMultiplier, double badValueMultiplier)
{
    if (change > 0) {
        if (value >= 3)
            return 0.0;
        if (value + change > 3)
            change = 3 - value;
        if (value >= 0)
            return static_cast<double>(change) * goodValueMultiplier;
        if (change <= value)
            return -(static_cast<double>(change) * badValueMultiplier);
        return static_cast<double>(value + change) * goodValueMultiplier
               + static_cast<double>(value) * badValueMultiplier;
    }
    if (value <= -3)
        return 0.0;
    if (value + change < -3)
        change = -3 - value;
    if (value <= 0)
        return -(static_cast<double>(change) * badValueMultiplier);
    if (-change <= value)
        return static_cast<double>(change) * goodValueMultiplier;
    return -(static_cast<double>(value) * goodValueMultiplier)
           - static_cast<double>(value + change) * badValueMultiplier;
}

VA(0x00435960, 0x1E)  // dc 0x3c580
double aiValueOfLuck(long luck, long change)
{
    return valueOfLuckAndMorale(luck, change, g_aiGoodLuckValue,
                                   g_aiBadLuckValue);
}

VA_COMPGEN(0x00437a00, 0x6FA, IMPLICIT_COPY_CTOR, army)

VA(0x00435980, 0x2A)  // dc 0x3c810
long aiGetAttackDamage(const army& currentArmy, long ourHits, const army& enemy, unsigned char ranged, long distance)
{
    long troops = (currentArmy.m_monInfo.m_hitPoints + ourHits - 1)
                  / currentArmy.m_monInfo.m_hitPoints;
    return currentArmy.getAverageDamage(&enemy, ranged, troops, 1, distance);
}

// E:\gamedcs\ai_tactical.cpp:215 - dc 0x3c854. No retail slot of its
// own: simulate_attack (0x4359b0) carries it inlined three times, so
// /OPT:REF dropped the out-of-line copy.
void type_AI_combat_parameters::simulateSingleAttack(const army& currentArmy, long& ourHits, const army& enemy, long& enemyHits, unsigned char ranged, long distance) const
{
    long troops = (currentArmy.m_monInfo.m_hitPoints + ourHits - 1)
                  / currentArmy.m_monInfo.m_hitPoints;
    long damage = currentArmy.getAverageDamage(&enemy, ranged, troops, 1,
                                                   distance);
    if (!ranged) {
        long fire = g_combatManager->computeFireShieldDamage(
            damage, &currentArmy, &enemy, enemyHits);
        if (fire > 0) {
            ourHits -= fire;
            if (ourHits < 0)
                ourHits = 0;
        }
    }
    enemyHits -= damage;
    if (enemyHits < 0)
        enemyHits = 0;
}

VA(0x004359b0, 0x1D5)  // dc 0x3c8dc
void type_AI_combat_parameters::simulateAttack(const army& currentArmy, long& ourHits, const army& enemy, long& enemyHits, unsigned char ranged, long distance) const
{
    if (ranged)
        ranged = currentArmy.canShoot(0);
    simulateSingleAttack(currentArmy, ourHits, enemy, enemyHits, ranged, distance);
    if (ourHits == 0 || enemyHits <= 0 || ranged)
        return;
    unsigned char noRetaliation = static_cast<unsigned char>(static_cast<unsigned>(currentArmy.m_monInfo.m_attributes) >> 16);
    if ((noRetaliation & 1) == 0 && enemy.m_spellInfluence[70] == 0
            && enemy.m_retaliationCount > 0
            && (g_game->m_setup.m_difficulty > 0 || g_combatManager->m_sideIsAi[m_ourGroup])) {
        simulateSingleAttack(enemy, enemyHits, currentArmy, ourHits, 0, 0);
        if (ourHits == 0 || enemyHits == 0)
            return;
    }
    unsigned char doubleAttack = static_cast<unsigned char>(static_cast<unsigned>(currentArmy.m_monInfo.m_attributes) >> 15);
    if (doubleAttack & 1)
        simulateSingleAttack(currentArmy, ourHits, enemy, enemyHits, 0, 0);
}

VA(0x00435b90, 0xD2)  // dc 0x3c9ac
long type_AI_combat_parameters::getSimpleAttackEffect(const army& currentArmy, long ourTotal, const army& enemy, long enemyTotal, unsigned char ranged, long distance) const
{
    long ourHits;
    long enemyHits;

    if (ranged)
        ranged = currentArmy.canShoot(0);
    if (m_simulated) {
        ourTotal -= currentArmy.getAIExpectedDamage();
        if (ourTotal <= 0)
            return 0;
        enemyTotal -= enemy.getAIExpectedDamage();
        if (enemyTotal <= 0)
            return 0;
    }
    ourHits = ourTotal;
    enemyHits = enemyTotal;
    simulateAttack(currentArmy, ourHits, enemy, enemyHits, ranged, distance);
    long value = enemy.getLossCombatValue(m_lowestAttack, m_lowestDefense, ranged,
                                              enemyTotal - enemyHits, m_killsOnly);
    if (ourTotal > ourHits)
        value -= currentArmy.getLossCombatValue(m_lowestAttack, m_lowestDefense,
                                                     ranged, ourTotal - ourHits, 0);
    return value;
}

VA(0x00435c70, 0x3D)  // dc 0x3ca94
long type_AI_combat_parameters::getSimpleAttackEffect(const army& currentArmy, const army& enemy, unsigned char ranged, long distance) const
{
    long ourTotal = currentArmy.getTotalHitPoints(0);
    long enemyTotal = enemy.getTotalHitPoints(0);
    return getSimpleAttackEffect(currentArmy, ourTotal, enemy, enemyTotal, ranged, distance);
}

VA(0x00435cb0, 0x10E)  // dc 0x3cae4
long type_AI_combat_parameters::getRangedAttackValue(const army& currentArmy, const army& enemy) const
{
    long value = getSimpleAttackEffect(currentArmy, enemy, 1, 0);
    if (!g_game->m_setup.m_difficulty && !g_combatManager->m_sideIsAi[m_ourGroup])
        return value;
    if (enemy.isIncapacitated())
        return value / 10;
    return (!enemy.cannotAttack()
                    && enemy.getAITarget() != 0
                    && enemy.getAITargetTime(enemy.getSpeed()) <= 5)
            ? value / enemy.getAITargetTime(enemy.getSpeed())
            : value / 5;
}

VA(0x00435dc0, 0xF3)  // dc 0x3cba0
long type_AI_combat_parameters::getExchangeEffect(const army& currentArmy, const army& enemy, long distance) const
{
    unsigned char ranged = currentArmy.canShoot(0);
    long ourHits = currentArmy.getTotalHitPoints(m_simulated);
    long enemyHits = enemy.getTotalHitPoints(m_simulated);
    long ourLeft = ourHits;
    long enemyLeft = enemyHits;
    simulateAttack(currentArmy, ourLeft, enemy, enemyLeft, ranged, distance);
    if (enemyLeft > 0)
        simulateAttack(enemy, enemyLeft, currentArmy, ourLeft, ranged, 0);
    long value = enemy.getLossCombatValue(m_lowestAttack, m_lowestDefense, ranged,
                                              enemyHits - enemyLeft,
                                              static_cast<unsigned char>(m_killsOnly && !m_simulated));
    if (ourHits > ourLeft)
        value -= currentArmy.getLossCombatValue(m_lowestAttack, m_lowestDefense, ranged,
                                                     ourHits - ourLeft,
                                                     static_cast<unsigned char>(m_killsOnly && !m_simulated));
    return value;
}

// The ctor establishes the whole AI's frame of reference for one
// combat: the WEAKEST attack and defense modifiers on the field (both
// sides, skipping empty stacks and the arrow tower), the four
// combat-value totals those two numbers price, whether the AI is far
// enough behind to start trading kills for kills, and how many turns
// ahead it is worth planning.

VA(0x00435ec0, 0x1F7)  // dc 0x3cc8c
type_AI_combat_parameters::type_AI_combat_parameters(const combatManager* combat, long side)
{
    unsigned char first = 1;
    this->m_ourGroup = side;
    m_enemyGroup = 1 - side;
    m_lowestAttack = 0;
    m_lowestDefense = 0;
    m_killsOnly = 0;
    m_simulated = 0;
    for (long group = 0; group < 2; group++) {
        for (long i = 0; i < combat->m_numArmies[group]; i++) {
            const army* ourArmy = &combat->m_armies[group][i];
            if (ourArmy->m_numTroops <= 0)
                continue;
            if (ourArmy->m_creatureType == CREATURE_ARROW_TOWER)
                continue;
            unsigned char ranged = ourArmy->canShoot(0);
            long attack = ourArmy->getAttackModifier(0, ranged);
            long defense = ourArmy->getDefenseModifier();
            if (first || m_lowestAttack > attack)
                m_lowestAttack = attack;
            if (first || m_lowestDefense > defense)
                m_lowestDefense = defense;
            first = 0;
        }
    }
    m_friendlyCombatValue = combat->getTotalCombatValue(this->m_ourGroup, m_lowestAttack,
                                               m_lowestDefense, 1);
    m_awakeFriendlyValue = combat->getTotalCombatValue(this->m_ourGroup, m_lowestAttack,
                                                    m_lowestDefense, 0);
    m_enemyCombatValue = combat->getTotalCombatValue(m_enemyGroup, m_lowestAttack,
                                                 m_lowestDefense, 1);
    m_awakeEnemyValue = combat->getTotalCombatValue(m_enemyGroup, m_lowestAttack,
                                                      m_lowestDefense, 0);
    if (m_friendlyCombatValue * 2 < m_enemyCombatValue
            && (g_game->m_setup.m_difficulty > 0
                || g_combatManager->m_sideIsAi[this->m_ourGroup]))
        m_killsOnly = 1;
    long high = m_awakeFriendlyValue;
    long low = m_awakeEnemyValue;
    if (high < low) {
        high = m_awakeEnemyValue;
        low = m_awakeFriendlyValue;
    }
    if (low * 5 > high && high != 0) {
        double ratio = static_cast<double>(high) / static_cast<double>(low);
        for (m_roundsLeft = 0; m_roundsLeft < 6; m_roundsLeft++) {
            if (ratio >= g_aiOddsLadder[m_roundsLeft])
                break;
        }
        m_roundsLeft++;
    } else {
        m_roundsLeft = 1;
    }
}

VA(0x004360c0, 0xBC)  // dc 0x3ceb8
type_AI_attack_hex_chooser::type_AI_attack_hex_chooser(const army* attacker, const army* defender, const long* attackArray, searchArray* search, const type_AI_combat_parameters* combatData)
{
    m_data = combatData;
    m_attackArmy = attacker;
    m_speed = attacker->getSpeed();
    m_enemyArmy = defender;
    m_bestValue = 0;
    m_bestAttackTime = 0;
    m_enemyAttackArray = attackArray;
    m_searchData = search;
    m_bestHex = -1;
    long enemyHits = defender->getTotalHitPoints(combatData->m_simulated);
    long ourHits = attacker->getTotalHitPoints(combatData->m_simulated);
    long troops = (attacker->m_monInfo.m_hitPoints + ourHits - 1) / attacker->m_monInfo.m_hitPoints;
    enemyHits -= attacker->getAverageDamage(defender, 0, troops, 1, 0);
    if (enemyHits < 0)
        enemyHits = 0;
    m_enemyTroopsLeft = (defender->m_monInfo.m_hitPoints + enemyHits - 1) / defender->m_monInfo.m_hitPoints;
    m_ourTroops = (attacker->m_monInfo.m_hitPoints + ourHits - 1) / attacker->m_monInfo.m_hitPoints;
}

// E:\gamedcs\ai_tactical.cpp:511
VA(0x00436180, 0x17A)  // anchor-global, dc 0x3cf50
long type_AI_attack_hex_chooser::getHexAttackValue(long hex, long& checked)
{
    double combatValue;
    long value = 0;
    if (!g_game->m_setup.m_difficulty
        && !g_combatManager->m_sideIsAi[m_data->getGroup()])
        return 0;
    for (long direction = 0; direction < 6; direction++) {
        long index = g_combatManager->m_adjacentCells[hex][direction];
        if (!g_combatManager->validHex(index))
            continue;
        army* enemy = g_combatManager->m_cells[index].getArmy();
        if (!enemy)
            continue;
        if (enemy->m_combatSide == g_combatManager->m_currentSide)
            continue;
        if (enemy == m_attackArmy)
            continue;
        long mask = 1 << enemy->m_bitIndex;
        if (mask & checked)
            continue;
        checked |= mask;
        if (!enemy->canShoot(m_attackArmy))
            continue;
        long hits = enemy->getTotalHitPoints(m_data->m_simulated);
        if (hits == 0)
            continue;
        combatValue = enemy->getUnitCombatValue(
                           m_data->m_lowestAttack, m_data->m_lowestDefense, 1,
                           m_attackArmy);
        combatValue -= enemy->getUnitCombatValue(
                           m_data->m_lowestAttack, m_data->m_lowestDefense, 0, 0);
        long share = static_cast<long>(static_cast<double>(hits) * combatValue
                                       / static_cast<double>(enemy->m_monInfo.m_hitPoints));
        if (share < 1)
            share = 1;
        value += share;
    }
    return value;
}

// E:\gamedcs\ai_tactical.cpp:575
// How many turns this stack needs to reach the hex `cell` describes.
// No retail body - the carve cuts nothing between the chooser's
// constructor at 0x4360c0 and check_adjacent_hexes at 0x436300 - so it
// is `inline`, and the RETURN is what the caller's shape needs: retail
// leaves the answer in EAX across all four exits and stores it ONCE,
// where the same statements written out in the caller store to the
// slot at every assignment.
DC_ONLY(0x3d154, 0x8E)
inline long type_AI_attack_hex_chooser::getAttackTime(const pathCell* cell) const
{
    if (m_speed == 0)
        return 0 < cell->m_cost ? 100 : 1;
    long turns = (cell->m_cost + m_speed - 1) / m_speed;
    if (m_searchData->isMoat(cell->m_point.m_x))
        turns++;
    if (turns < 1)
        turns = 1;
    return turns;
}

// The two seven-parameter statics below are added for the attacker's
// own multi-head and breath sweeps and SUBTRACTED for the enemy's
// retaliation, but only while the side is actually being played by the
// computer or the difficulty is above the lowest rung. The same gate
// guards the tie-break, where a JOUSTER (creatureType 10 or 11) breaks
// a tie toward the hex that costs MORE to reach - its charge bonus
// grows with distance - and everything else toward the cheaper hex.

// A double-wide attacker scores BOTH of its hexes: the second is one
// step in the direction it faces, its attack value is added, and the
// enemy threat taken is the SMALLER of the two hexes'.

VA(0x00436300, 0x31C)  // dc 0x3d1e4
void type_AI_attack_hex_chooser::checkAdjacentHexes(long enemyHex, long startDirection, long stopDirection)
{
    for (long direction = startDirection; direction < stopDirection; direction++) {
        long hex = g_combatManager->m_adjacentCells[enemyHex][direction];
        if (!g_combatManager->validHex(hex))
            continue;
        const pathCell* cell = m_searchData->getHex(hex);
        if (!cell->m_visited)
            continue;
        if (cell->m_flightCost > 0)
            continue;
        long turns = getAttackTime(cell);
        if (m_bestHex >= 0) {
            if (m_bestAttackTime < turns)
                continue;
        }
        long checked = 0;
        long value = getHexAttackValue(hex, checked);
        if (g_game->m_setup.m_difficulty > 0
                || g_combatManager->m_sideIsAi[m_data->getGroup()]) {
            unsigned char heads = static_cast<unsigned char>(
                static_cast<unsigned>(m_attackArmy->m_monInfo.m_attributes) >> 19);
            if (heads & 1)
                value += getMultiHeadBonus(g_combatManager->m_currentSide,
                                              m_attackArmy, hex, m_ourTroops,
                                              m_enemyArmy, m_enemyArmy->m_gridIndex,
                                              m_data);
            unsigned char breath = static_cast<unsigned char>(
                static_cast<unsigned>(m_attackArmy->m_monInfo.m_attributes) >> 3);
            if (breath & 1)
                value += getBreathBonus(g_combatManager->m_currentSide,
                                          m_attackArmy, hex, m_ourTroops,
                                          m_enemyArmy, m_enemyArmy->m_gridIndex,
                                          m_data);
            if (m_enemyArmy->canRetaliate(*m_attackArmy)) {
                unsigned char enemyHeads = static_cast<unsigned char>(
                    static_cast<unsigned>(m_enemyArmy->m_monInfo.m_attributes) >> 19);
                if (enemyHeads & 1)
                    value -= getMultiHeadBonus(m_enemyArmy->m_combatSide,
                                                  m_enemyArmy,
                                                  m_enemyArmy->m_gridIndex,
                                                  m_enemyTroopsLeft,
                                                  m_attackArmy, hex, m_data);
                unsigned char enemyBreath = static_cast<unsigned char>(
                    static_cast<unsigned>(m_enemyArmy->m_monInfo.m_attributes) >> 3);
                if (enemyBreath & 1)
                    value -= getBreathBonus(m_enemyArmy->m_combatSide,
                                              m_enemyArmy,
                                              m_enemyArmy->m_gridIndex,
                                              m_enemyTroopsLeft,
                                              m_attackArmy, hex, m_data);
            }
        }
        long threat = m_enemyAttackArray[hex];
        if (m_attackArmy->m_monInfo.m_attributes & 1) {
            long otherHex = hex + (m_attackArmy->m_facing ? 1 : -1);
            value += getHexAttackValue(otherHex, checked);
            threat = cppMin(m_enemyAttackArray[otherHex], threat);
        }
        value += threat;
        if (m_bestHex >= 0 && turns == m_bestAttackTime) {
            if (value < m_bestValue)
                continue;
            if (value == m_bestValue) {
                const pathCell* bestCell = m_searchData->getHex(m_bestHex);
                long difference = cell->m_cost - bestCell->m_cost;
                if ((m_attackArmy->m_creatureType == CREATURE_CAVALIER
                            || m_attackArmy->m_creatureType == CREATURE_CHAMPION)
                        && (g_game->m_setup.m_difficulty > 0
                            || g_combatManager->m_sideIsAi[m_data->getGroup()])) {
                    if (difference <= 0)
                        continue;
                } else {
                    if (difference >= 0)
                        continue;
                }
            }
        }
        m_bestValue = value;
        m_bestHex = hex;
        m_bestAttackTime = turns;
    }
}

// The kills_only argument here is a LITERAL 0, not estimate's own byte
// (`push 0` where get_breath_bonus pushes estimate->kills_only), and
// the ranged argument is 0 in both.
VA(0x00436620, 0x13A)  // dc 0x3c608
long getMultiHeadBonus(long ourGroup, const army* ourArmy, long ourHex, long troopCount, const army* enemy, long enemyHex, const type_AI_combat_parameters* estimate)
{
    long counted = 1 << enemy->m_bitIndex;
    long directions = ourArmy->getMultiHeadDirections(ourHex, enemy, enemyHex);
    long value = 0;
    for (long i = 0; i < 8; i++) {
        if ((directions & (1 << i)) == 0)
            continue;
        long hex = ourArmy->getAdjacentHex(ourHex, i);
        if (hex < 0 || hex >= 187)
            continue;
        army* target = g_combatManager->m_cells[hex].getArmy();
        if (target == 0)
            continue;
        if (target->m_combatSide == ourGroup)
            continue;
        if (counted & (1 << target->m_bitIndex))
            continue;
        long damage = ourArmy->getAverageDamage(target, 0, troopCount, 1, 0);
        if (estimate->m_simulated)
            damage = cppMin(damage, target->getTotalHitPoints(1));
        value += target->getLossCombatValue(estimate->m_lowestAttack,
                                               estimate->m_lowestDefense, 0, damage, 0);
        counted |= 1 << target->m_bitIndex;
    }
    return value;
}

VA(0x00436760, 0xDF)  // dc 0x3c708
long getBreathBonus(long ourGroup, const army* ourArmy, long ourHex, long troopCount, const army* enemy, long enemyHex, const type_AI_combat_parameters* estimate)
{
    long direction = ourArmy->getAttackDirection(ourHex, enemy, enemyHex);
    long breathHex = ourArmy->getAdjacentHex(ourHex, direction);
    long hex = ourArmy->getAdjacentCellIndex(breathHex, direction);
    if (hex < 0 || hex >= 187)
        return 0;
    army* target = g_combatManager->m_cells[hex].getArmy();
    if (target == 0 || target == enemy)
        return 0;
    long damage = ourArmy->getAverageDamage(target, 0, troopCount, 1, 0);
    if (estimate->m_simulated)
        damage = cppMin(damage, target->getTotalHitPoints(1));
    long value = target->getLossCombatValue(estimate->m_lowestAttack,
                                               estimate->m_lowestDefense, 0, damage,
                                               estimate->m_killsOnly);
    if (target->m_combatSide == ourGroup)
        return -value;
    return value;
}

VA(0x00436840, 0xEA)  // dc 0x3d440
unsigned char type_AI_attack_hex_chooser::findAttackHex()
{
    m_bestValue = 0;
    m_bestHex = -1;
    checkAdjacentHexes(m_enemyArmy->m_gridIndex, 0, 6);
    if (m_enemyArmy->m_monInfo.m_attributes & 1)
        checkAdjacentHexes(m_enemyArmy->getSecondGridIndex(), 0, 6);
    if (m_attackArmy->m_monInfo.m_attributes & 1) {
        long hex = m_enemyArmy->m_gridIndex;
        long offset = -(m_attackArmy->m_facing ? 1 : -1);
        if ((m_enemyArmy->m_monInfo.m_attributes & 1)
                && offset == (m_enemyArmy->m_facing ? 1 : -1))
            hex = m_enemyArmy->getSecondGridIndex();
        if (offset < 0) {
            long second = g_combatManager->m_adjacentCells[hex][4];
            if (second >= 0 && second < 187)
                checkAdjacentHexes(second, 3, 6);
        } else {
            long second = g_combatManager->m_adjacentCells[hex][1];
            if (second >= 0 && second < 187)
                checkAdjacentHexes(second, 0, 3);
        }
    }
    return m_bestHex >= 0 && m_bestHex < 187;
}

// E:\gamedcs\ai_tactical.cpp:744 - dc 0x3d524. No retail slot: both
// type_spell_choice ctors inline it whole (their bytes carry the five
// stores), so /OPT:REF dropped the out-of-line copy.
type_enchant_data::type_enchant_data(SpellID newSpell, TSkillMastery newMastery, long newPower, long newDuration)
{
    m_spell = newSpell;
    m_mastery = newMastery;
    m_power = newPower;
    m_duration = newDuration;
    m_checkResistance = 1;
}

VA(0x00436930, 0x1A)  // dc 0x3d56c
long type_enchant_data::getMasteryValue() const
{
    return g_spellTraits[m_spell].m_masteryBonus[m_mastery];
}

VA(0x00436950, 0x23)  // dc 0x3d584
type_spell_choice::type_spell_choice()
    : type_enchant_data(-1, eMasteryNone, 0, 0)
{
    m_value = 0;
    m_target = -1;
    m_secondTargetHex = -1;
    m_castNow = 0;
}

VA(0x00436980, 0x35)  // dc 0x3d5b0
type_spell_choice::type_spell_choice(SpellID newSpell, TSkillMastery newMastery, long newPower, long newDuration)
    : type_enchant_data(newSpell, newMastery, newPower, newDuration)
{
    m_value = 0;
    m_target = -1;
    m_secondTargetHex = -1;
    m_castNow = 0;
}

#if 0  // @carcass

// E:\gamedcs\ai_tactical.cpp:779
DC_ONLY(0x3d5dc, 0x28)
void type_AI_spellcaster::initialize(combatManager* combat, long side)
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\ai_tactical.cpp:817
DC_ONLY(0x3d6f0, 0x72)
inline type_AI_spellcaster::type_AI_spellcaster(type_AI_spellcaster* parent,
                                                combatManager* combat, long side,
                                                unsigned char creatureSpell)
    : m_estimate(combat, side)
{
    long enemy = 1 - side;
    this->m_isCreatureSpell = creatureSpell;
    this->m_side = side;
    m_enemySide = enemy;
    m_ourHero = combat->m_heroes[side];
    m_enemyHero = combat->m_heroes[enemy];
    m_winLikely = 0;
    m_enemyCaster = parent;
    m_ownsEnemyCaster = 0;
    checkSimulation();
    findEnemyAttacks();
}

// E:\gamedcs\ai_tactical.cpp:793
// EH-bearing (P2.2): push -1 / push 0xb / mov eax,fs:[0] frame around
// the operator-new of the 0x410-byte deputy caster - the ONE state the
// funclet cleans is that raw allocation, so the frame is the `new`
// expression's, not a local's.

// The body is the public caster's whole setup: the two heroes off
// combat->heroes[], the combat manager's own move order and simulation
// pass, this side's decided-fight check, both groups' AI targets, the
// melee census, and finally the DEPUTY - a second caster for the other
// side, owned by this one (owns_deputy = 1), whose constructor is
// inlined here in full.

// `1 - side` is a LOCAL, not a re-read of the member: retail spills it
// into the dead `combat` parameter slot at [ebp+8] and feeds the
// deputy from there. The member would have to be reloaded, since
// find_move_order / simulate_combat / find_AI_targets all sit between
// the store and the use and none of them lets VC6 assume `this` is
// unaliased.
// Residual (93.1850%, register homing): flow-distance is zero.  Retail
// homes combat in EBX before the base-member constructor and later reuses
// the dead parameter slot for enemy; this compile reloads combat and binds
// EBX to enemy instead.  why-reg classifies the equal definition slots as
// the C1 front-end processing-order family.  A named combat-manager alias
// regresses slightly, while swapping the adjacent side/creature stores is
// byte-flat; the model's automated adjacent-store mutation has no further
// legal source move.
VA(0x004369c0, 0x22B)  // anchor-callee, dc 0x3d604
type_AI_spellcaster::type_AI_spellcaster(combatManager* combat, long side,
                                         unsigned char creatureSpell)
    : m_estimate(combat, side)
{
    long enemy = 1 - side;
    this->m_isCreatureSpell = creatureSpell;
    this->m_side = side;
    m_enemySide = enemy;
    m_ourHero = combat->m_heroes[side];
    m_enemyHero = combat->m_heroes[enemy];
    m_winLikely = 0;
    combat->findMoveOrder(0);
    combat->simulateCombat(side, 0);
    checkSimulation();
    for (long group = 0; group < 2; group++)
        g_combatManager->findAITargets(group, 0, 0, &m_estimate, 0);
    findEnemyAttacks();
    m_enemyCaster = new type_AI_spellcaster(this, combat, enemy, creatureSpell);
    m_ownsEnemyCaster = 1;
}

VA_COMPGEN(0x00436bf0, 0x3A, SCALAR_DELETING_DTOR, type_AI_spellcaster)

VA(0x00436c30, 0x21)  // dc 0x3d764
type_AI_spellcaster::~type_AI_spellcaster()
{
    if (m_enemyCaster && m_ownsEnemyCaster)
        delete m_enemyCaster;
}

// E:\gamedcs\ai_tactical.cpp:837
// DC's ordinary const helper precedes should_attack_now in this TU. Its
// GetCurrentArmy, Is and IsIncapacitated calls remain canonical; VC6 expands
// the helper naturally at its three callers. Absence of a retained retail
// body does not justify an explicit inline keyword.
DC_ONLY(0x3d7b0, 0x86)
inline unsigned char type_AI_spellcaster::isLastAction() const
{
    const army* current = g_combatManager->getCurrentArmy();
    long total = g_combatManager->m_numArmies[m_side];
    for (long j = 0; j < total; j++) {
        const army* other = &g_combatManager->m_armies[m_side][j];
        if (other->is(0x200040) || other->isIncapacitated())
            continue;
        if (other->is(1u << 26))
            continue;
        if (other != current)
            return 0;
    }
    return 1;
}

VA(0x00436c60, 0x1C4)  // dc 0x3d838
unsigned char type_AI_spellcaster::shouldAttackNow(const army& enemy) const
{
    if (m_estimate.m_killsOnly)
        return 1;
    if (isLastAction())
        return 1;
    const army* current = g_combatManager->getCurrentArmy();
    if (current->m_combatSide == m_side && current->getAITarget() == &enemy
        && current->getAITargetTime(current->getSpeed()) == 1
        && !current->canShoot(0)
        && !current->is(1u << 16))
        return 1;
    if ((m_enemyCanAttack & (1 << enemy.m_bitIndex)) == 0)
        return 0;
    long total = g_combatManager->m_numArmies[m_side];
    for (long j = 0; j < total; j++) {
        const army* ourArmy = &g_combatManager->m_armies[m_side][j];
        if (ourArmy->is(1u << 21) || ourArmy->isIncapacitated())
            continue;
        if (ourArmy->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (ourArmy == current)
            continue;
        if (ourArmy->m_expectedMoveOrder > enemy.m_expectedMoveOrder)
            return 0;
    }
    return 1;
}

VA(0x00436e30, 0x125)  // dc 0x3d96c
long type_AI_spellcaster::getDamageValue(SpellID spell, long baseDamage, const hero* targetHero, const army* target) const
{
    unsigned char immune = static_cast<unsigned char>(static_cast<unsigned>(target->m_monInfo.m_attributes) >> 21);
    if ((immune & 1) || target->m_creatureType == CREATURE_ARROW_TOWER)
        return 0;
    long damage = g_combatManager->modifySpellDamage(baseDamage, spell, m_ourHero,
                                                     targetHero, target, 0);
    long creatureCast = m_isCreatureSpell != 0;
    long value = static_cast<long>(
        g_combatManager->spellCastWorkChance(spell, m_side, target, 0, 1,
                                             creatureCast) * damage);
    if (value <= 0)
        return 0;
    long capped = cppMin(target->getTotalHitPoints(0), value);
    value = target->getLossCombatValue(m_estimate.m_lowestAttack, m_estimate.m_lowestDefense,
                                          target->canShoot(0), capped,
                                          m_estimate.m_killsOnly);
    unsigned char flags = static_cast<unsigned char>(static_cast<unsigned>(target->m_monInfo.m_attributes) >> 21);
    if (target->m_spellInfluence[62] || target->m_spellInfluence[70] || target->m_spellInfluence[74]
            || (flags & 1)
            || target->m_creatureType == CREATURE_FIRST_AID_TENT
            || target->m_creatureType == CREATURE_AMMO_CART) {
        long total = target->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                    m_estimate.m_lowestDefense);
        if (total > value)
            value = value * 2 - total;
    }
    return value;
}

VA(0x00436f60, 0x45)  // dc 0x3da7c
long type_AI_spellcaster::getDamageSpellValue(const army* enemy, type_enchant_data caster) const
{
    long baseDamage = g_spellTraits[caster.m_spell].m_powerFactor * caster.m_power
                       + caster.getMasteryValue();
    return getDamageValue(caster.m_spell, baseDamage, m_enemyHero, enemy);
}

// E:\gamedcs\ai_tactical.cpp:965
// The per-GROUP half of every mass-damage pricer: sum get_damage_value
// over one side's stacks, walking BACKWARDS. No retail body - the
// carve cuts nothing between get_damage_spell_value (0x436f60, 69 B)
// and get_mass_damage_effect (0x436fb0). Keep its ordinary definition;
// absence of a retained retail body does not prove source inline. Its
// two PARAMETERS are what let VC6 hoist the group index and the hero
// out of the loop: consider_spell's mass arm homes them at [ebp+8] and
// [ebp-0x10] before the walk and strength-reduces the army address
// into a single decrementing byte offset. Spelling the same two loops
// out in the caller instead re-reads `this->enemy_side` every
// iteration and re-derives the whole 1352-byte stride each time, which
// is what capped consider_spell at 79.84.
// DC row 973 forms the army address before row 975's damage call. Giving
// that address its own target lifetime closes considerSpell 98.1927 -> 100;
// the 16-state follow-up finds pointer/reference bindings exact, while the
// inline address and a named damage result remain 98.1927. Mass-result
// declaration/argument lifetimes do not change either outcome.
DC_ONLY(0x3dabc, 0x6E)
long type_AI_spellcaster::getGroupDamageValue(SpellID spell, long baseDamage,
                                                        long group, hero* targetHero) const
{
    long value = 0;
    long count = g_combatManager->m_numArmies[group];
    while (count--) {
        const army* target = &g_combatManager->m_armies[group][count];
        value += getDamageValue(spell, baseDamage, targetHero, target);
    }
    return value;
}

VA(0x00436fb0, 0x8A)  // dc 0x3db2c
long type_AI_spellcaster::getMassDamageEffect(long enemyDamage, long friendlyDamage) const
{
    if (enemyDamage <= 0)
        return 0;
    if (friendlyDamage < 0)
        friendlyDamage = 0;
    if (enemyDamage <= friendlyDamage)
        return 0;
    long ourTotal = m_estimate.m_friendlyCombatValue;
    if (static_cast<float>(enemyDamage) / static_cast<float>(m_estimate.m_enemyCombatValue)
            <= static_cast<float>(friendlyDamage) / static_cast<float>(ourTotal))
        return 0;
    if (friendlyDamage >= ourTotal)
        return 0;
    return enemyDamage - friendlyDamage;
}

VA(0x00437040, 0x141)  // dc 0x3db84
long type_AI_spellcaster::getAreaEffectValue(SpellID spell, long baseDamage, TSkillMastery mastery, long hex) const
{
    long friendlyDamage = 0;
    long enemyDamage = 0;
    std::vector<army*> targets;
    g_combatManager->markAreaEffect(spell, hex, mastery, targets);
    for (long i = targets.size(); i-- > 0; ) {
        army* target = targets[i];
        if (target->m_combatSide == m_side)
            friendlyDamage += getDamageValue(spell, baseDamage, m_ourHero, target);
        else
            enemyDamage += getDamageValue(spell, baseDamage, m_enemyHero, target);
    }
    return getMassDamageEffect(enemyDamage, friendlyDamage);
}

// E:\gamedcs\ai_tactical.cpp:1035
// The area-effect sweep, inlined into consider_spell and carrying no
// retail body of its own. It tries every hex on the field and keeps
// the best get_area_effect_value, skipping only the two off-field
// margin columns - and it re-tests the 0..187 range in front of that
// column test, which is why retail emits a range guard the loop bound
// already guarantees.
DC_ONLY(0x3dc50, 0x72)
inline void type_AI_spellcaster::considerAreaEffect(type_spell_choice* choice) const
{
    long baseDamage = g_spellTraits[choice->m_spell].m_powerFactor * choice->m_power
                       + g_spellTraits[choice->m_spell].m_masteryBonus[choice->m_mastery];
    for (long hex = 0; hex < COMBAT_GRID_CELLS; hex++) {
        if (hex >= 0 && hex < COMBAT_GRID_CELLS) {
            long column = hex % COMBAT_GRID_ROW_STRIDE;
            if (column == 0)
                continue;
            if (column == COMBAT_GRID_LAST_COLUMN)
                continue;
        }
        long value = getAreaEffectValue(choice->m_spell, baseDamage,
                                           choice->m_mastery, hex);
        if (value > choice->m_value) {
            choice->m_target = hex;
            choice->m_value = value;
            choice->m_castNow = 1;
        }
    }
}

// The hop itself is combatManager's: mark the stack in the
// `effected` block, ask GetNextChainLightningTarget for the
// next hex, and stop the moment it answers off-field. ClearEffects
// wipes the marks before the walk starts.
VA(0x00437190, 0x17D)  // dc 0x3dcc4
long type_AI_spellcaster::getChainLightningValue(long power, TSkillMastery mastery, army* target) const
{
    long count = g_chainLightningTargets[mastery];
    long enemyDamage = 0;
    long friendlyDamage = 0;
    g_combatManager->clearEffects();
    long damage = g_spellTraits[SPELL_CHAIN_LIGHTNING].m_masteryBonus[mastery]
                  + g_spellTraits[SPELL_CHAIN_LIGHTNING].m_powerFactor * power;
    while (count--) {
        if (target->m_combatSide == m_side)
            friendlyDamage += getDamageValue(SPELL_CHAIN_LIGHTNING, damage,
                                                m_ourHero, target);
        else
            enemyDamage += getDamageValue(SPELL_CHAIN_LIGHTNING, damage,
                                             m_enemyHero, target);
        g_combatManager->m_effected[target->m_combatSide][target->m_bitIndex] = 1;
        long hex = g_combatManager->getNextChainLightningTarget(target, 0);
        if (hex < 0)
            break;
        if (hex >= COMBAT_GRID_CELLS)
            break;
        target = g_combatManager->m_cells[hex].getArmy();
        damage = damage / 2;
    }
    return getMassDamageEffect(enemyDamage, friendlyDamage);
}

VA(0x00437310, 0xD1)  // dc 0x3dde8
void type_AI_spellcaster::considerChainLightning(type_spell_choice* choice) const
{
    long targetSide = 1 - m_side;
    for (long i = 0; i < g_combatManager->m_numArmies[targetSide]; ++i) {
        army* target = &g_combatManager->m_armies[targetSide][i];
        unsigned char immune = static_cast<unsigned char>(
            static_cast<unsigned>(target->m_monInfo.m_attributes) >> 21);
        long creatureCast = m_isCreatureSpell != 0;
        if (!(immune & 1)
                && g_combatManager->validSpellTargetArmy(SPELL_CHAIN_LIGHTNING,
                                                   m_side, target, 1,
                                                   creatureCast)) {
            long value = getChainLightningValue(choice->m_power,
                                                    choice->m_mastery, target);
            if (value > choice->m_value) {
                choice->m_value = value;
                choice->m_target = target->m_gridIndex;
                choice->m_castNow = 1;
            }
        }
    }
}

// E:\gamedcs\ai_tactical.cpp:1126
// DC 0x3de90 proves this const/reference helper and both ordered group calls.
// Retail 0x43bb20 expands it but calls get_mass_damage_effect. Restoring
// consider_summon's get_mastery_value call (DC 3098/3167) recovers that split
// without a force-inline declaration or depth pin. The exhaustive 24-state
// boundary family preserves all five TUs' scores; flattening only that mastery
// call while leaving the effect unpinned lowers considerSpell 98.1927 ->
// 80.5073. Ordinary group/mass/summon helpers preserve every old code section.
void type_AI_spellcaster::considerMassDamage(
    type_spell_choice& choice) const
{
    long baseDamage =
        g_spellTraits[choice.m_spell].m_powerFactor * choice.m_power
        + choice.getMasteryValue();
    long enemyDamage = getGroupDamageValue(choice.m_spell, baseDamage,
                                               m_enemySide, m_enemyHero);
    long friendlyDamage = getGroupDamageValue(choice.m_spell, baseDamage,
                                                  m_side, m_ourHero);
    choice.m_value = getMassDamageEffect(enemyDamage, friendlyDamage);
    choice.m_castNow = 1;
}

VA(0x004373f0, 0x34)  // dc 0x3df28
long type_AI_spellcaster::getAgeValue(const army* enemy, type_enchant_data caster) const
{
    if (m_winLikely)
        return 0;
    return enemy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                         m_estimate.m_lowestDefense) / 3;
}

// DC ai_tactical.cpp:1158/1186.
// Ordinary const overloads: callers expand them in retail; /OPT:REF drops
// unreferenced retained bodies. Keep their source order before the callers.
long type_AI_spellcaster::getAttackBoostValue(const army* ourArmy,
    const army* enemy, long oldDamage, long duration, double increase) const
{
    long newDamage = static_cast<long>(oldDamage * increase);
    long enemyHits = enemy->getTotalHitPoints(0);
    if (newDamage > enemyHits) {
        newDamage = enemyHits;
        increase = static_cast<double>(enemyHits) / oldDamage;
    }
    if (newDamage <= oldDamage)
        return 0;
    double modifier = getDuration(duration, ourArmy->is(1u << 26));
    double total = static_cast<double>(ourArmy->getTotalCombatValue(
        m_estimate.m_lowestAttack, m_estimate.m_lowestDefense));
    return static_cast<long>((sqrt(increase) - 1.0) * total * modifier);
}

long type_AI_spellcaster::getAttackBoostValue(const army* ourArmy,
    const army* enemy, long duration, double increase) const
{
    long oldDamage = ourArmy->getAverageDamage(enemy, ourArmy->canShoot(0),
        ourArmy->m_numTroops, 1, 0);
    return getAttackBoostValue(ourArmy, enemy, oldDamage, duration, increase);
}

VA(0x00437430, 0x198)  // dc 0x3e17c
long type_AI_spellcaster::getBlessValue(const army* ourArmy, type_enchant_data caster) const
{
    const army* target = ourArmy->getAITarget();
    if (target == 0 || ourArmy->getAITargetTime() > 1)
        return 0;
    double average = ourArmy->getAverageDamage();
    long blessed = g_spellTraits[SPELL_BLESS].m_masteryBonus[caster.m_mastery]
                   + ourArmy->m_monInfo.m_damageHighBound;
    double increase = blessed / average;
    return getAttackBoostValue(ourArmy, target, caster.m_duration, increase);
}

// E:\gamedcs\ai_tactical.cpp:1218
// Frenzy is the only member of the boost family that SIMULATES: it
// prices the stack's swing before the exchange, runs one
// simulate_attack to find out how much of the stack survives to swing
// again, and adds the second (halved for a shooter that already strikes
// twice) swing to the first. The ratio of the two-swing total to the
// one-swing total is then the same `factor` get_bless_value builds, and
// the tail is the shared `(sqrt(factor) - 1) * total * scale` ladder.
// `caster` reaches the body only through its duration.
// No traits row at all - the mastery bonus never enters the price.

// Residual (99.0%): ONE frame slot, nothing else - every instruction,
// immediate, reloc and branch agrees unmasked. Retail runs BOTH
// int->double conversions of the `combined / old_damage` division
// through the same scratch slot [ebp-0x2c]; our CL gives the divisor's
// conversion a second slot at [ebp-0x24] (which it then reuses for
// `factor`, so no other displacement moves). Frame-slot colouring, the
// class already documented on get_attack_skill_value. Tried and
// rejected: dropping either explicit cast (98.98 both ways) and
// dropping the `combined` local for one inlined sum (98.98). Swept
// again 2026-08-08 with the naming lever that closed the
// SpellCastWorkChance family - `double increase = combined; increase
// /= old_damage;`, the same split with an explicit cast, and a named
// `divisor` - all three move the pair of scratch slots around
// (increase's own slot, or a named divisor slot) but NONE reproduces
// retail's single reused slot, because retail's `fld` of the numerator
// happens BEFORE the divisor's fild and no source order we can write
// makes our CL emit that fld early.
// With the canonical boost helper restored, direct ratio arguments,
// meaningful reuse of newDamage, a reference-bound ratio and a named return
// value also fail to reuse retail's divisor scratch (seven states, five
// emitted objects). The named ratio and both early guards stay intact.
VA(0x004375d0, 0x224)  // anchor-vtable, dc 0x3e280
long type_AI_spellcaster::getFrenzyValue(const army* ourArmy, type_enchant_data caster) const
{
    const army* target = ourArmy->getAITarget();
    if (target == 0 || ourArmy->getAITargetTime() > 1)
        return 0;
    unsigned char ranged = ourArmy->canShoot(0);
    long ourHits = ourArmy->getTotalHitPoints(0);
    long enemyHits = target->getTotalHitPoints(0);
    long oldDamage = aiGetAttackDamage(*(ourArmy), ourHits, *(target), ranged, 0);
    m_estimate.simulateAttack(*(ourArmy), ourHits, *(target), enemyHits, ranged, 0);
    if (ourHits == 0)
        return 0;
    long newDamage = aiGetAttackDamage(*(ourArmy), ourHits, *(target), ranged, 0);
    if (ranged && ourArmy->is(1u << 15))
        newDamage /= 2;
    double increase = static_cast<double>(newDamage + oldDamage)
                      / static_cast<double>(oldDamage);
    return getAttackBoostValue(ourArmy, target, caster.m_duration, increase);
}

VA(0x00437800, 0x1F5)  // dc 0x3e3bc
long type_AI_spellcaster::getAttackSkillValue(const army* ourArmy, const army* enemy, long duration, long bonus) const
{
    if (m_winLikely)
        return 0;
    army testArmy = *ourArmy;
    testArmy.m_monInfo.m_attackSkill += bonus;
    unsigned char ranged = ourArmy->canShoot(0);
    double oldDamage = ourArmy->getEstimatedDamage(enemy, 100, ranged, 0);
    double newDamage = testArmy.getEstimatedDamage(enemy, 100, ranged, 0);
    double increase = newDamage / oldDamage;
    return getAttackBoostValue(ourArmy, enemy, duration, increase);
}

VA(0x00438100, 0x64)  // dc 0x3e4a0
long type_AI_spellcaster::getBloodLustValue(const army* ourArmy, type_enchant_data caster) const
{
    if (!ourArmy->canShoot(0)) {
        const army* target = ourArmy->getAITarget();
        if (target != 0
                && ourArmy->getAITargetTime(ourArmy->getSpeed()) <= 1) {
            long bonus = g_spellTraits[SPELL_BLOODLUST].m_masteryBonus[caster.m_mastery];
            return getAttackSkillValue(ourArmy, target, caster.m_duration, bonus);
        }
    }
    return 0;
}

VA(0x00438170, 0x142)  // dc 0x3e50c
long type_AI_spellcaster::getMirthValue(const army* ourArmy, type_enchant_data caster) const
{
    unsigned char undead = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 17);
    if (undead & 1)
        return 0;
    long change = g_spellTraits[SPELL_MIRTH].m_masteryBonus[caster.m_mastery];
    double effect = aiValueOfMorale(ourArmy->getMorale(1), change);
    if (effect == 0.0)
        return 0;
    double portion;
    if (caster.m_duration >= m_estimate.m_roundsLeft)
        portion = 1.0;
    else
        portion = static_cast<double>(caster.m_duration) / static_cast<double>(m_estimate.m_roundsLeft);
    unsigned char slowFlag = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 26);
    double scale;
    if ((slowFlag & 1)
            && (portion = portion - 1.0 / static_cast<double>(m_estimate.m_roundsLeft)) < 0.0)
        scale = 0.0;
    else
        scale = portion;
    double total = static_cast<double>(ourArmy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                                        m_estimate.m_lowestDefense));
    return static_cast<long>(total * scale * effect);
}

VA(0x004382c0, 0x1C6)  // dc 0x3e658
long type_AI_spellcaster::getSorrowValue(const army* enemy, type_enchant_data caster) const
{
    unsigned char undead = static_cast<unsigned char>(static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 17);
    if (undead & 1)
        return 0;
    if ((m_enemyCanAttack & (1 << enemy->m_bitIndex)) == 0)
        return 0;
    if (m_estimate.m_killsOnly)
        return 0;
    if (m_winLikely)
        return 0;
    long change = g_spellTraits[SPELL_SORROW].m_masteryBonus[caster.m_mastery];
    double effect = -aiValueOfMorale(enemy->getMorale(1), -change);
    if (effect == 0.0)
        return 0;
    if (caster.m_checkResistance) {
        long creatureCast = m_isCreatureSpell != 0;
        effect = effect * g_combatManager->spellCastWorkChance(SPELL_SORROW, m_side, enemy,
                                                               0, 1, creatureCast);
    }
    double portion;
    if (caster.m_duration >= m_estimate.m_roundsLeft)
        portion = 1.0;
    else
        portion = static_cast<double>(caster.m_duration) / static_cast<double>(m_estimate.m_roundsLeft);
    unsigned char slowFlag = static_cast<unsigned char>(static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 26);
    double scale;
    if ((slowFlag & 1)
            && (portion = portion - 1.0 / static_cast<double>(m_estimate.m_roundsLeft)) < 0.0)
        scale = 0.0;
    else
        scale = portion;
    double total = static_cast<double>(enemy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                                     m_estimate.m_lowestDefense));
    return static_cast<long>(total * scale * effect);
}

// The doubling is HoMM3's luck rule read backwards. Bad luck halves a
// strike and good luck doubles it, so the pricer runs the same
// `(sqrt(factor) - 1) * total * scale` tail twice: once over the HALF
// damage the stack's standing NEGATIVE luck is costing it, once over
// the FULL damage its resulting POSITIVE luck would add - each
// weighted by how many luck points that arm actually moves and
// divided by 24, which is the per-point chance a lucky strike fires.

VA(0x00438490, 0x32B)  // dc 0x3e87c
long type_AI_spellcaster::getFortuneValue(const army* ourArmy, type_enchant_data caster) const
{
    const army* target = ourArmy->getAITarget();
    if (target == 0 || ourArmy->getAITargetTime() > 1)
        return 0;
    long bonus = g_spellTraits[SPELL_FORTUNE].m_masteryBonus[caster.m_mastery];
    long luck = ourArmy->getLuck(0);
    long damage = ourArmy->getAverageDamage(target, ourArmy->canShoot(0),
                                               ourArmy->m_numTroops, 0, 0);
    long strikes;
    long value = 0;
    if (luck >= 3)
        return 0;
    if (bonus + luck <= -3)
        return 0;
    if (bonus + luck > 3)
        bonus = 3 - luck;
    if (luck < 0) {
        if (bonus + luck > 0)
            strikes = -luck;
        else
            strikes = bonus;
        value = getAttackBoostValue(ourArmy, target,
            damage / 2, caster.m_duration, 2.0) * strikes / 24;
    }
    if (bonus + luck > 0) {
        if (luck < 0)
            strikes = bonus + luck;
        else
            strikes = bonus;
        value += getAttackBoostValue(ourArmy, target,
            damage, caster.m_duration, 2.0) * strikes / 24;
    }
    return value;
}

VA(0x004387c0, 0x14A)  // dc 0x3e9d8
long type_AI_spellcaster::getDefenseBoostValue(const army* ourArmy, const army* enemy, long duration, double increase) const
{
    long damage = enemy->getAverageDamage(ourArmy, enemy->canShoot(0),
                                            enemy->m_numTroops, 0, 0);
    long reduced = static_cast<long>(static_cast<double>(damage) / increase);
    long hits = ourArmy->getTotalHitPoints(0);
    if (damage > hits) {
        increase = static_cast<double>(hits) / static_cast<double>(reduced);
        damage = hits;
    }
    if (reduced >= damage)
        return 0;
    if (m_winLikely) {
        if (ourArmy->getAIExpectedDamage() + ourArmy->m_topCreatureDamage
                < ourArmy->m_monInfo.m_hitPoints)
            return 0;
    }
    if ((m_attacks[ourArmy->m_bitIndex].m_totalDamage
                + m_meleeEnemies[ourArmy->m_bitIndex].m_totalDamage) * m_estimate.m_roundsLeft
            + ourArmy->m_topCreatureDamage < ourArmy->m_monInfo.m_hitPoints)
        return 0;
    double scale;
    if (duration >= m_estimate.m_roundsLeft)
        scale = 1.0;
    else
        scale = static_cast<double>(duration) / static_cast<double>(m_estimate.m_roundsLeft);
    double total = static_cast<double>(ourArmy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                                        m_estimate.m_lowestDefense));
    return static_cast<long>((sqrt(increase) - 1.0) * total * scale);
}

VA(0x00438910, 0xFB)  // dc 0x3ec10
long type_AI_spellcaster::getDefenseSkillValue(const army* ourArmy, long duration, long bonus) const
{
    const army* enemy = m_worstEnemies[ourArmy->m_bitIndex].m_enemy;
    if (!enemy)
        return 0;
    army testArmy = *ourArmy;
    testArmy.m_monInfo.m_defenseSkill += bonus;
    unsigned char ranged = enemy->canShoot(0);
    double oldDamage = enemy->getEstimatedDamage(ourArmy, 100, ranged, 0);
    double newDamage = enemy->getEstimatedDamage(&testArmy, 100, ranged, 0);
    double increase = oldDamage / newDamage;
    return getDefenseBoostValue(ourArmy, enemy, duration, increase);
}

// The shared SpellCastWorkChance call-site spelling is settled here
// (2026-08-08), and it is ONE naming: the normalised creature-spell
// flag is a NAMED LOCAL, not an inline `creature_spell != 0` argument.

//     long creature_cast = creature_spell != 0;
//     ... SpellCastWorkChance(spell, side, enemy, 0, 1, creature_cast)

VA(0x00438a10, 0xAD)  // dc 0x3ed34
long type_AI_spellcaster::getDiseaseValue(const army* enemy, type_enchant_data caster) const
{
    if ((m_enemyCanAttack & (1 << enemy->m_bitIndex)) == 0)
        return 0;
    if (m_estimate.m_killsOnly)
        return 0;
    double total = static_cast<double>(enemy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                                     m_estimate.m_lowestDefense));
    long value = static_cast<long>((total - total * 0.9)
                                   * static_cast<double>(caster.m_duration));
    if (caster.m_checkResistance) {
        long creatureCast = m_isCreatureSpell != 0;
        value = static_cast<long>(
            g_combatManager->spellCastWorkChance(SPELL_WEAKNESS, m_side, enemy, 0, 1,
                                                 creatureCast) * value);
    }
    return value;
}

VA(0x00438ac0, 0x85)  // dc 0x3eea8
long type_AI_spellcaster::getPrayerValue(const army* ourArmy, type_enchant_data caster) const
{
    long bonus = g_spellTraits[SPELL_PRAYER].m_masteryBonus[caster.m_mastery];
    const army* target = ourArmy->getAITarget();
    long value = getDefenseSkillValue(ourArmy, caster.m_duration, bonus);
    value += getSpeedValue(ourArmy, bonus, caster.m_duration);
    if (target != 0
            && ourArmy->getAITargetTime(ourArmy->getSpeed()) == 1)
        value += getAttackSkillValue(ourArmy, target, caster.m_duration, bonus);
    return value;
}

VA(0x00438b50, 0x64)  // dc 0x3ef24
long type_AI_spellcaster::getPrecisionValue(const army* ourArmy, type_enchant_data caster) const
{
    if (ourArmy->canShoot(0)) {
        const army* target = ourArmy->getAITarget();
        if (target != 0
                && ourArmy->getAITargetTime(ourArmy->getSpeed()) <= 1) {
            long bonus = g_spellTraits[SPELL_PRECISION].m_masteryBonus[caster.m_mastery];
            return getAttackSkillValue(ourArmy, target, caster.m_duration, bonus);
        }
    }
    return 0;
}

VA(0x00438bc0, 0x99)  // dc 0x3ef90
long type_AI_spellcaster::getAirShieldValue(const army* ourArmy, type_enchant_data caster) const
{
    const army* enemy = m_attacks[ourArmy->m_bitIndex].m_enemy;
    if (enemy == 0)
        return 0;
    long melee = m_meleeEnemies[ourArmy->m_bitIndex].m_totalDamage;
    long ranged = m_attacks[ourArmy->m_bitIndex].m_totalDamage;
    double increase = static_cast<double>(ranged + melee)
        / static_cast<double>(ranged * g_spellTraits[SPELL_AIR_SHIELD].m_masteryBonus[caster.m_mastery] / 100
                              + melee);
    return getDefenseBoostValue(ourArmy, enemy, caster.m_duration, increase);
}

VA(0x00438c60, 0x99)  // dc 0x3f080
long type_AI_spellcaster::getShieldValue(const army* ourArmy, type_enchant_data caster) const
{
    const army* enemy = m_meleeEnemies[ourArmy->m_bitIndex].m_enemy;
    if (enemy == 0)
        return 0;
    long ranged = m_attacks[ourArmy->m_bitIndex].m_totalDamage;
    long melee = m_meleeEnemies[ourArmy->m_bitIndex].m_totalDamage;
    double increase = static_cast<double>(melee + ranged)
        / static_cast<double>(melee * g_spellTraits[SPELL_SHIELD].m_masteryBonus[caster.m_mastery] / 100
                              + ranged);
    return getDefenseBoostValue(ourArmy, enemy, caster.m_duration, increase);
}

VA(0x00438d00, 0x81)  // dc 0x3f158
long type_AI_spellcaster::getSlayerValue(const army* ourArmy, type_enchant_data caster) const
{
    const army* target = ourArmy->getAITarget();
    if (target != 0 && ourArmy->getAITargetTime(ourArmy->getSpeed()) <= 1) {
        unsigned flags = static_cast<unsigned>(target->m_monInfo.m_attributes);
        if ((static_cast<unsigned char>(flags >> 7) & 1)
                || ((static_cast<unsigned char>(flags >> 8) & 1)
                    && caster.m_mastery >= eMasteryAdvanced)
                || ((static_cast<unsigned char>(flags >> 9) & 1)
                    && caster.m_mastery >= eMasteryExpert)) {
            return getAttackSkillValue(ourArmy, target, caster.m_duration,
                                          g_spellTraits[SPELL_SLAYER]
                                              .m_masteryBonus[caster.m_mastery]);
        }
    }
    return 0;
}

VA(0x00438d90, 0x25)  // dc 0x3f208
long type_AI_spellcaster::getToughSkinValue(const army* ourArmy, type_enchant_data caster) const
{
    return getDefenseSkillValue(ourArmy, caster.m_duration,
                                   g_spellTraits[SPELL_STONE_SKIN].m_masteryBonus[caster.m_mastery]);
}

VA(0x00438dc0, 0x109)  // dc 0x3f22c
long type_AI_spellcaster::getDisruptiveRayValue(const army* enemy, type_enchant_data caster) const
{
    const combatManager* combat = g_combatManager;
    const army* stacks = combat->m_armies[m_side];
    if (m_estimate.m_killsOnly)
        return 0;
    long count = combat->m_numArmies[m_side];
    long i;
    for (i = 0; i < count; i++)
        if (stacks[i].getAITarget() == enemy)
            break;
    if (i == count)
        return 0;
    long bonus = g_spellTraits[SPELL_DISRUPTING_RAY].m_masteryBonus[caster.m_mastery];
    double total = static_cast<double>(enemy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                                     m_estimate.m_lowestDefense));
    long value = static_cast<long>(
        total - total * sqrt(1.0 - static_cast<double>(bonus) * 0.05));
    if (caster.m_checkResistance) {
        long creatureCast = m_isCreatureSpell != 0;
        value = static_cast<long>(
            g_combatManager->spellCastWorkChance(SPELL_DISRUPTING_RAY, m_side, enemy, 0, 1,
                                                 creatureCast) * value);
    }
    return value;
}

VA(0x00438ed0, 0x8D)  // dc 0x3f408
long type_AI_spellcaster::getWeaknessValue(const army* enemy, type_enchant_data caster) const
{
    if ((m_enemyCanAttack & (1 << enemy->m_bitIndex)) != 0 && !m_estimate.m_killsOnly) {
        const army* target = enemy->getAITarget();
        if (target != 0
                && enemy->getAITargetTime(enemy->getSpeed()) <= 1) {
            long capped = cppMin(g_spellTraits[SPELL_WEAKNESS].m_masteryBonus[caster.m_mastery],
                                   enemy->m_monInfo.m_attackSkill);
            return getAttackSkillValue(enemy, target, caster.m_duration, capped);
        }
    }
    return 0;
}

VA(0x00438f60, 0x199)  // dc 0x3f5f0
long type_AI_spellcaster::getMisfortuneValue(const army* enemy, type_enchant_data caster) const
{
    if ((m_enemyCanAttack & (1 << enemy->m_bitIndex)) == 0)
        return 0;
    if (m_estimate.m_killsOnly)
        return 0;
    long change = g_spellTraits[SPELL_MISFORTUNE].m_masteryBonus[caster.m_mastery];
    double effect = -aiValueOfLuck(enemy->getLuck(1), -change);
    if (effect == 0.0)
        return 0;
    if (caster.m_checkResistance) {
        long creatureCast = m_isCreatureSpell != 0;
        effect = effect * g_combatManager->spellCastWorkChance(SPELL_MISFORTUNE, m_side, enemy,
                                                               0, 1, creatureCast);
    }
    double portion;
    if (caster.m_duration >= m_estimate.m_roundsLeft)
        portion = 1.0;
    else
        portion = static_cast<double>(caster.m_duration) / static_cast<double>(m_estimate.m_roundsLeft);
    unsigned char slowFlag = static_cast<unsigned char>(static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 26);
    double scale;
    if ((slowFlag & 1)
            && (portion = portion - 1.0 / static_cast<double>(m_estimate.m_roundsLeft)) < 0.0)
        scale = 0.0;
    else
        scale = portion;
    double total = static_cast<double>(enemy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                                     m_estimate.m_lowestDefense));
    return static_cast<long>(total * scale * effect);
}

VA(0x00439100, 0x169)  // dc 0x3f7f0
long type_AI_spellcaster::getBlindValue(const army* enemy, type_enchant_data caster) const
{
    if ((m_enemyCanAttack & (1 << enemy->m_bitIndex)) == 0)
        return 0;
    if (m_winLikely)
        return 0;
    long value = enemy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                               m_estimate.m_lowestDefense);
    if (value >= m_estimate.m_awakeEnemyValue) {
        long bonus = g_spellTraits[SPELL_BLIND].m_masteryBonus[caster.m_mastery];
        value = static_cast<long>((0.5 - sqrt(static_cast<double>(bonus) / 400.0))
                                  * static_cast<double>(value));
    } else {
        double portion;
        if (caster.m_duration >= m_estimate.m_roundsLeft)
            portion = 1.0;
        else
            portion = static_cast<double>(caster.m_duration) / static_cast<double>(m_estimate.m_roundsLeft);
        unsigned char slowFlag = static_cast<unsigned char>(static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 26);
        double scale;
        if ((slowFlag & 1)
                && (portion = portion - 1.0 / static_cast<double>(m_estimate.m_roundsLeft)) < 0.0)
            scale = 0.0;
        else
            scale = portion;
        value = static_cast<long>(static_cast<double>(value) * scale);
    }
    if (caster.m_checkResistance) {
        long creatureCast = m_isCreatureSpell != 0;
        value = static_cast<long>(
            g_combatManager->spellCastWorkChance(SPELL_BLIND, m_side, enemy, 0, 1,
                                                 creatureCast) * value);
    }
    return value;
}

#if 0  // @carcass

// E:\gamedcs\ai_tactical.cpp:1767
DC_ONLY(0x3f9d0, 0x52)
long type_AI_spellcaster::get_move_order_change_value(const army* our_army)
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\ai_tactical.cpp:1788
// Muck and mire is priced as a SLOW: it reads the Slow mastery row
// (54*136 + 0x34) for the speed it takes away and pushes the literal
// 0x36 into SpellCastWorkChance at the end, which is what pins both to
// SPELL_SLOW. The body is get_speed_value's shape run the other way
// round - the enemy is the one being slowed - and it prices two
// separate things, summing them into one running `value`:

//   - if the enemy is about to strike THIS turn (time == 1), the best
//     exchange any of our stacks that is targeting it could get instead
//     (the get_exchange_effect pair, exactly as in get_speed_value);
//   - if the enemy is a walker, the turns of approach the slow buys us,
//     priced through the same `(odds - t + 1) * total / odds` ladder
//     get_speed_value closes with.

// The delay uses this TU's overwrite-the-variable shape twice over:
// `new_time` is turned into a DELTA against `time`, capped at the
// spell's remaining turns, then turned back into an absolute time by
// adding `time` again before the odds cap.

// The work chance is the FLOAT round-trip (`fild dword / fstp DWORD /
// fmul dword`), not the double one, as in get_damage_value.

// Three edits took this from 74.62: inverting the null-target test so
// the zero arm is the FALL-THROUGH (`if (target == 0) { effect = 0; }
// else { ... }`, +13.7 - arm layout follows the source's test
// polarity), and hoisting the army pointer out of the loop with
// `j++, our_army++` in the increment instead of re-subscripting
// `&armies[side][j]` inside the body (+8.6 - the subscript form folds
// j into the pre-header base address, where retail computes
// `&armies[side][0]` with no j at all; set_melee_enemies below already
// carries the hoisted form).
VA(0x00439270, 0x28B)  // anchor-vtable, dc 0x3fa24
long type_AI_spellcaster::getMuckAndMireValue(const army* enemy, type_enchant_data caster) const
{
    if (enemy->getAITarget() == 0)
        return 0;
    if (m_winLikely)
        return 0;
    long time = enemy->getAITargetTime(enemy->getSpeed());
    if (time > m_estimate.m_roundsLeft)
        return 0;
    unsigned char slowFlag = static_cast<unsigned char>(static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 26);
    long turns = caster.m_duration;
    if (slowFlag & 1)
        turns--;
    if (turns == 0)
        return 0;
    long value = 0;
    long speed = enemy->getSpeed();
    long newSpeed = g_spellTraits[SPELL_SLOW].m_masteryBonus[caster.m_mastery] * speed / 100;
    if (newSpeed < 1)
        newSpeed = 1;
    if (time == 1) {
        const army* ourArmy = &g_combatManager->m_armies[m_side][0];
        for (long j = 0; j < g_combatManager->m_numArmies[m_side]; j++, ourArmy++) {
            if (ourArmy->getAITarget() != enemy)
                continue;
            if (ourArmy->m_spellInfluence[62])
                continue;
            if (ourArmy->m_spellInfluence[70])
                continue;
            if (ourArmy->m_spellInfluence[74])
                continue;
            unsigned char immune = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 21);
            if (immune & 1)
                continue;
            if (ourArmy->m_creatureType == CREATURE_FIRST_AID_TENT)
                continue;
            if (ourArmy->m_creatureType == CREATURE_AMMO_CART)
                continue;
            if (ourArmy->getSpeed() > speed)
                continue;
            if (ourArmy->getSpeed() <= newSpeed)
                continue;
            long effect;
            const army* target = ourArmy->getAITarget();
            if (target == 0) {
                effect = 0;
            } else {
                long ours = m_estimate.getExchangeEffect(*(ourArmy), *(target), 0);
                effect = m_estimate.getExchangeEffect(*(target), *(ourArmy), 0) + ours;
            }
            if (effect > value)
                value = effect;
        }
    }
    if (!enemy->canShoot(0)) {
        long newTime = enemy->getAITargetTime(newSpeed);
        newTime -= time;
        if (newTime > turns)
            newTime = turns;
        if (newTime > 0) {
            long total = enemy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                       m_estimate.m_lowestDefense);
            newTime += time;
            if (newTime > m_estimate.m_roundsLeft + 1)
                newTime = m_estimate.m_roundsLeft + 1;
            value += (m_estimate.m_roundsLeft - time + 1) * total / m_estimate.m_roundsLeft
                     - (m_estimate.m_roundsLeft - newTime + 1) * total / m_estimate.m_roundsLeft;
        }
    }
    if (caster.m_checkResistance) {
        long creatureCast = m_isCreatureSpell != 0;
        value = static_cast<long>(
            g_combatManager->spellCastWorkChance(SPELL_SLOW, m_side, enemy, 0, 1,
                                                 creatureCast) * value);
    }
    return value;
}

VA(0x00439500, 0x4C)  // dc 0x3fc20
long type_AI_spellcaster::getPoisonValue(const army* enemy, type_enchant_data caster) const
{
    if (m_winLikely)
        return 0;
    long damage = (enemy->m_monInfo.m_hitPoints - enemy->m_topCreatureDamage) / 2;
    return enemy->getLossCombatValue(m_estimate.m_lowestAttack,
                                        m_estimate.m_lowestDefense,
                                        enemy->canShoot(0), damage,
                                        m_estimate.m_killsOnly);
}

VA(0x00439550, 0x153)  // dc 0x3fc80
long type_AI_spellcaster::getSpeedValue(const army* ourArmy, long increase, long duration) const
{
    if (ourArmy->getAITarget() == 0)
        return 0;
    if (m_winLikely)
        return 0;
    unsigned char slowFlag = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 26);
    long turns = duration;
    if (slowFlag & 1)
        turns = duration - 1;
    if (turns == 0)
        return 0;
    long value = 0;
    long oldSpeed = ourArmy->getSpeed();
    long newSpeed = ourArmy->m_baseSpeed + increase;
    long oldTime = ourArmy->getAITargetTime(ourArmy->getSpeed());
    long newTime = ourArmy->getAITargetTime(newSpeed);
    if (newTime > m_estimate.m_roundsLeft)
        return 0;
    if (newTime == 1) {
        const army* target = ourArmy->getAITarget();
        if (target->getSpeed() >= oldSpeed && target->getSpeed() < newSpeed) {
            const army* enemy = ourArmy->getAITarget();
            if (enemy) {
                long ours = m_estimate.getExchangeEffect(*(ourArmy), *(enemy), 0);
                value = m_estimate.getExchangeEffect(*(enemy), *(ourArmy), 0) + ours;
                if (value < 0)
                    value = 0;
            } else {
                value = 0;
            }
        }
    }
    if (newTime < oldTime) {
        if (oldTime > m_estimate.m_roundsLeft + 1)
            oldTime = m_estimate.m_roundsLeft + 1;
        long total = ourArmy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                      m_estimate.m_lowestDefense);
        value += (m_estimate.m_roundsLeft - newTime + 1) * total / m_estimate.m_roundsLeft
                 - (m_estimate.m_roundsLeft - oldTime + 1) * total / m_estimate.m_roundsLeft;
    }
    return value;
}

VA(0x004396b0, 0x2E)  // dc 0x3fdb8
long type_AI_spellcaster::getHasteValue(const army* ourArmy, type_enchant_data caster) const
{
    return getSpeedValue(ourArmy, caster.getMasteryValue(),
                           caster.m_duration);
}

// The work chance stays FLOAT to __ftol here exactly as in
// get_damage_value - `fild dword / fstp DWORD / fmul dword` - while the
// closing odds ladder is the TU's usual double one.

VA(0x004396e0, 0x2BC)  // dc 0x3fde4
long type_AI_spellcaster::getProtectionValue(const army* ourArmy,
    TSpellSchool school, long level, long duration, long amount) const
{
    if (!g_combatManager->canCastSpells(m_enemySide, 1))
        return 0;
    if (m_winLikely)
        return 0;
    if (ourArmy->is(1u << 23))
        return 0;
    long power = g_combatManager->m_spellPower[m_enemySide];
    long value = 0;
    unsigned char ranged = ourArmy->canShoot(0);
    long ourHits = ourArmy->getTotalHitPoints(0);
    armyGroup* group = g_combatManager->m_armyGroups[m_enemySide];
    for (long i = 0; i < hero::NUM_SPELLS; i++) {
        if ((school & g_spellTraits[i].m_schoolBits) == 0)
            continue;
        if ((g_spellTraits[i].m_flags & 0x200) == 0)
            continue;
        if ((g_spellTraits[i].m_flags & 1) == 0)
            continue;
        if (g_spellTraits[i].m_level > level)
            continue;
        if (!m_enemyHero->spellIsAvailable(i))
            continue;
        if (!g_combatManager->validSpellTargetArmy(i, m_enemySide, ourArmy, 1, 0))
            continue;
        TSkillMastery mastery = m_enemyHero->getSpellLevel(i, g_combatManager->m_magicTerrain);
        long manaCost = m_enemyHero->getManaCost(
            i, group, g_combatManager->m_magicTerrain);
        if (manaCost > m_enemyHero->m_mana)
            continue;
        long damage = g_spellTraits[i].m_masteryBonus[mastery]
            + g_spellTraits[i].m_powerFactor * power;
        damage = g_combatManager->modifySpellDamage(
            damage, i, m_ourHero, m_enemyHero, ourArmy, 0);
        if (damage == 0)
            continue;
        long reduction = damage * amount / 100;
        if (damage > ourHits)
            damage = ourHits;
        if (reduction >= damage)
            continue;
        long blocked = static_cast<long>(
            g_combatManager->spellCastWorkChance(i, m_enemySide, ourArmy, 0, 1, 0)
            * (damage - reduction));
        long loss = ourArmy->getLossCombatValue(m_estimate.m_lowestAttack,
                                                    m_estimate.m_lowestDefense,
                                                    ranged, blocked, 0);
        if (loss > value)
            value = loss;
    }
    return static_cast<long>(static_cast<double>(value)
        * getDuration(duration, ourArmy->is(1u << 26)));
}

VA(0x004399a0, 0x29)  // dc 0x40060
long type_AI_spellcaster::getAirProtectionValue(const army* ourArmy, type_enchant_data caster) const
{
    return getProtectionValue(
        ourArmy, eSchoolAir, 5, caster.m_duration,
        g_spellTraits[SPELL_PROTECTION_FROM_AIR].m_masteryBonus[caster.m_mastery]);
}

VA(0x004399d0, 0x29)  // dc 0x4008c
long type_AI_spellcaster::getFireProtectionValue(const army* ourArmy, type_enchant_data caster) const
{
    return getProtectionValue(
        ourArmy, eSchoolFire, 5, caster.m_duration,
        g_spellTraits[SPELL_PROTECTION_FROM_FIRE].m_masteryBonus[caster.m_mastery]);
}

VA(0x00439a00, 0x32)  // dc 0x400b8
long type_AI_spellcaster::getEarthProtectionValue(const army* ourArmy, type_enchant_data caster) const
{
    long amount = g_spellTraits[caster.m_spell].m_masteryBonus[caster.m_mastery];
    return getProtectionValue(ourArmy, eSchoolEarth, 5, caster.m_duration, amount);
}

VA(0x00439a40, 0x32)  // dc 0x400f4
long type_AI_spellcaster::getWaterProtectionValue(const army* ourArmy, type_enchant_data caster) const
{
    long amount = g_spellTraits[caster.m_spell].m_masteryBonus[caster.m_mastery];
    return getProtectionValue(ourArmy, eSchoolWater, 5, caster.m_duration, amount);
}

// DC ai_tactical.cpp:2116-2129, dc 0x40130. Ordinary const helper;
// protection value expands it in retail.
double type_AI_spellcaster::getDuration(long turns, unsigned char movedThisTurn) const
{
    double result;
    if (turns >= m_estimate.m_roundsLeft)
        result = 1.0;
    else
        result = static_cast<double>(turns) / m_estimate.m_roundsLeft;
    if (movedThisTurn) {
        result -= 1.0 / m_estimate.m_roundsLeft;
        if (result < 0.0)
            return 0.0;
    }
    return result;
}

VA(0x00439a80, 0x135)  // dc 0x40248
long type_AI_spellcaster::getCancelValue(army* currentArmy, unsigned char badSpellsOnly) const
{
    long value = 0;
    for (long spell = 10; spell < 81; spell++) {
        long duration = currentArmy->m_spellInfluence[spell];
        if (duration == 0)
            continue;
        if (badSpellsOnly) {
            if (g_spellTraits[spell].m_karma >= 0)
                continue;
        } else {
            if (spell == SPELL_POISON)
                continue;
        }
        TEnchantValue valueOf = getEnchantmentFunction(spell);
        if (valueOf == 0)
            continue;
        type_enchant_data caster(spell, currentArmy->getSpellLevel(spell),
                                 duration, duration);
        caster.m_checkResistance = 0;
        currentArmy->cancelIndividualSpell(spell);
        long ours = currentArmy->m_combatSide == m_side;
        long bad = g_spellTraits[spell].m_karma < 0;
        if (bad == ours)
            value += (m_enemyCaster->*valueOf)(currentArmy, caster);
        else
            value -= (this->*valueOf)(currentArmy, caster);
    }
    return value;
}

VA(0x00439bc0, 0x6E)  // dc 0x40348
long type_AI_spellcaster::getDispelValue(const army* ourArmy, type_enchant_data caster) const
{
    army testArmy = *ourArmy;
    return getCancelValue(&testArmy, 0);
}

VA(0x00439c30, 0x10F)  // dc 0x403c0
long type_AI_spellcaster::getCureValue(const army* ourArmy, type_enchant_data caster) const
{
    army currentArmy = *ourArmy;
    long value = getCancelValue(&currentArmy, 1);
    int mastery = caster.getMasteryValue();
    int damage = ourArmy->m_topCreatureDamage;
    int healed = cppMin<int>(mastery + g_spellTraits[SPELL_CURE].m_powerFactor * caster.m_power, damage);
    if (m_winLikely) {
        if (ourArmy->m_topCreatureDamage + ourArmy->getAIExpectedDamage()
                < ourArmy->m_monInfo.m_hitPoints)
            healed = 0;
    }
    if (healed > 0)
        value = static_cast<long>(
            ourArmy->getUnitCombatValue(m_estimate.m_lowestAttack,
                                            m_estimate.m_lowestDefense,
                                            ourArmy->canShoot(0), 0)
            * healed / ourArmy->m_monInfo.m_hitPoints + value);
    return value;
}

VA(0x00439d40, 0x94)  // dc 0x40570
long type_AI_spellcaster::getAntimagicValue(const army* ourArmy, type_enchant_data caster) const
{
    army testArmy = *ourArmy;
    long value = getCancelValue(&testArmy, 0);
    value += getProtectionValue(ourArmy, eSchoolAll,
                                  g_spellTraits[SPELL_ANTI_MAGIC].m_masteryBonus[caster.m_mastery],
                                  caster.m_duration, 0);
    return value;
}

VA(0x00439de0, 0x94)  // dc 0x405d4
long type_AI_spellcaster::getBacklashValue(const army* ourArmy, type_enchant_data caster) const
{
    army testArmy = *ourArmy;
    return getProtectionValue(ourArmy, eSchoolAll, 5, caster.m_duration,
                                (50 - caster.getMasteryValue()) * 2);
}

// The griffins are the special cases, and they are what pin their two
// enumerators: the Griffin already retaliates twice, so each bought
// retaliation is worth double (`mult = 2`), while the Royal Griffin
// retaliates without limit and the spell is worth nothing to it. The
// census's `count` (how many attackers are queued against this stack)
// minus what it already retaliates for is the cap on how many of the
// mastery row's retaliations can actually be used.

// The traits row is the CONSTANT form - 58 folded into the displacement
// as `[akSpellTraits + 4*mastery + 0x1f04]` - not the run-time
// `caster.spell` form get_fire_shield_value carries.

// The second creatureId test is bit 15 (attacks twice) crossed with
// bit 2 (shoots) clear: a melee double-attacker gets one more swing out
// of the deal than a shooter does.

VA(0x00439e80, 0x290)  // dc 0x40628
long type_AI_spellcaster::getCounterstrokeValue(const army* ourArmy, type_enchant_data caster) const
{
    long mult = 1;
    if (ourArmy->m_creatureType == CREATURE_GRIFFIN)
        mult = 2;
    if (ourArmy->m_creatureType == CREATURE_ROYAL_GRIFFIN)
        return 0;
    if (m_winLikely)
        return 0;
    long bonus = g_spellTraits[SPELL_COUNTERSTRIKE].m_masteryBonus[caster.m_mastery];
    long extra = cppMin(m_meleeEnemies[ourArmy->m_bitIndex].m_count - mult, bonus);
    if (extra <= 0)
        return 0;
    const army* target = m_meleeEnemies[ourArmy->m_bitIndex].m_enemy;
    if (target == 0)
        return 0;
    long ourHits = ourArmy->getTotalHitPoints(0);
    ourHits -= target->getAverageDamage(ourArmy, 0, target->m_numTroops, 1, 0);
    if (ourHits <= 0)
        return 0;
    long enemyHits = target->getTotalHitPoints(0);
    long totalDamage = ourArmy->getAverageDamage(target, 0, ourArmy->m_numTroops, 1, 0);
    long newDamage = ourArmy->getAverageDamage(
        target, 0, (ourArmy->m_monInfo.m_hitPoints + ourHits - 1) / ourArmy->m_monInfo.m_hitPoints, 1, 0);
    if (newDamage > enemyHits)
        newDamage = enemyHits;
    totalDamage += newDamage * mult;
    unsigned char doubleAttack = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 15);
    unsigned char shooter = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 2);
    if ((doubleAttack & 1) && !(shooter & 1))
        totalDamage += newDamage;
    long combined = newDamage * extra + totalDamage;
    double increase = static_cast<double>(combined) / static_cast<double>(totalDamage);
    long damage = ourArmy->getAverageDamage(target, ourArmy->canShoot(0),
                                               ourArmy->m_numTroops, 1, 0);
    double factor = increase;
    long boosted = static_cast<long>(damage * increase);
    long targetHits = target->getTotalHitPoints(0);
    if (boosted > targetHits) {
        factor = static_cast<double>(targetHits) / static_cast<double>(damage);
        boosted = targetHits;
    }
    if (boosted <= damage)
        return 0;
    double portion;
    if (caster.m_duration >= m_estimate.m_roundsLeft)
        portion = 1.0;
    else
        portion = static_cast<double>(caster.m_duration) / static_cast<double>(m_estimate.m_roundsLeft);
    unsigned char slowFlag = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 26);
    double scale;
    if ((slowFlag & 1)
            && (portion = portion - 1.0 / static_cast<double>(m_estimate.m_roundsLeft)) < 0.0)
        scale = 0.0;
    else
        scale = portion;
    double total = static_cast<double>(ourArmy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                                        m_estimate.m_lowestDefense));
    return static_cast<long>((sqrt(factor) - 1.0) * total * scale);
}

// The Efreet Sultan already carries a fire shield, so its own casting
// is docked a flat 20 off the mastery row before the `<= 0` bail.

VA(0x0043a110, 0x222)  // dc 0x407e8
long type_AI_spellcaster::getFireShieldValue(const army* ourArmy, type_enchant_data caster) const
{
    if (m_winLikely)
        return 0;
    long count = m_meleeEnemies[ourArmy->m_bitIndex].m_count;
    long amount = g_spellTraits[caster.m_spell].m_masteryBonus[caster.m_mastery];
    if (ourArmy->m_creatureType == CREATURE_EFREET_SULTAN)
        amount -= 20;
    if (amount <= 0)
        return 0;
    const army* target = m_meleeEnemies[ourArmy->m_bitIndex].m_enemy;
    if (target == 0)
        return 0;
    unsigned char fireImmune = static_cast<unsigned char>(static_cast<unsigned>(target->m_monInfo.m_attributes) >> 14);
    if (fireImmune & 1)
        return 0;
    long reflected = target->getAverageDamage(ourArmy, 0, target->m_numTroops, 1, 0)
                     * amount / 100;
    long ourHits = ourArmy->getTotalHitPoints(0);
    const long& capped = cppMin(reflected, ourHits);
    long oldDamage = ourArmy->getAverageDamage(target, 0, ourArmy->m_numTroops, 1, 0);
    long combined = capped * count + oldDamage;
    double increase = static_cast<double>(combined) / static_cast<double>(oldDamage);
    long damage = ourArmy->getAverageDamage(target, ourArmy->canShoot(0),
                                               ourArmy->m_numTroops, 1, 0);
    double factor = increase;
    long boosted = static_cast<long>(damage * increase);
    long enemyHits = target->getTotalHitPoints(0);
    if (boosted > enemyHits) {
        factor = static_cast<double>(enemyHits) / static_cast<double>(damage);
        boosted = enemyHits;
    }
    if (boosted > damage) {
        double portion;
        if (caster.m_duration >= m_estimate.m_roundsLeft)
            portion = 1.0;
        else
            portion = static_cast<double>(caster.m_duration) / static_cast<double>(m_estimate.m_roundsLeft);
        unsigned char slowFlag = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 26);
        double scale;
        if ((slowFlag & 1)
                && (portion = portion - 1.0 / static_cast<double>(m_estimate.m_roundsLeft)) < 0.0)
            scale = 0.0;
        else
            scale = portion;
        double total = static_cast<double>(ourArmy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                                            m_estimate.m_lowestDefense));
        return static_cast<long>((sqrt(factor) - 1.0) * total * scale);
    }
    return 0;
}

VA(0x0043a340, 0xBE)  // dc 0x40928
long type_AI_spellcaster::getTraitorValue(const army* enemy, const army* target) const
{
    unsigned char ranged = enemy->canShoot(0);
    if (target->m_combatSide == m_side)
        return 0;
    long enemyHits = enemy->getTotalHitPoints(0);
    long enemyDamage = enemyHits;
    long targetHits = target->getTotalHitPoints(0);
    long targetDamage = targetHits;
    m_estimate.simulateAttack(*(enemy), enemyHits, *(target), targetHits, ranged, 0);
    enemyDamage -= enemyHits;
    targetDamage -= targetHits;
    long value = enemy->getLossCombatValue(m_estimate.m_lowestAttack, m_estimate.m_lowestDefense,
                                              ranged, enemyDamage,
                                              m_estimate.m_killsOnly);
    return value + target->getLossCombatValue(m_estimate.m_lowestAttack, m_estimate.m_lowestDefense,
                                                 ranged, targetDamage,
                                                 m_estimate.m_killsOnly);
}

VA(0x0043a400, 0xF8)  // dc 0x40a08
long type_AI_spellcaster::getBerserkValue(const army* enemy, type_enchant_data caster) const
{
    std::vector<army*> targets;
    if (m_winLikely)
        return 0;
    enemy->getBerserkTargets(targets);
    if (targets.size() == 0)
        return 0;
    long total = 0;
    for (unsigned i = 0; i < targets.size(); i++)
        total += getTraitorValue(enemy, targets[i]);
    return total / targets.size();
}

VA(0x0043a500, 0x16E)  // dc 0x40ac0
long type_AI_spellcaster::getHypnotizeValue(const army* enemy, type_enchant_data caster) const
{
    if (m_winLikely)
        return 0;
    if (enemy->m_spellInfluence[62])
        return 0;
    if (enemy->m_spellInfluence[70])
        return 0;
    if (enemy->m_spellInfluence[74])
        return 0;
    unsigned char flags = static_cast<unsigned char>(static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 21);
    if (flags & 1)
        return 0;
    if (enemy->m_creatureType == CREATURE_FIRST_AID_TENT
            || enemy->m_creatureType == CREATURE_AMMO_CART)
        return 0;
    long best = 0;
    const army* enemyRow = g_combatManager->m_armies[m_enemySide];
    long turns = cppMin(g_hypnotizeTurns[caster.m_mastery], m_estimate.m_roundsLeft);
    unsigned char slowFlags = static_cast<unsigned char>(static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 26);
    if (slowFlags & 1)
        turns--;
    if (turns == 0)
        return 0;
    g_searchArray->seedCombatPosition(enemy, m_side, enemy->m_monInfo.m_speed * turns, 0, -1);
    long count = g_combatManager->m_numArmies[m_enemySide];
    for (; count-- > 0; enemyRow++) {
        if (enemyRow == enemy)
            continue;
        unsigned char otherFlags = static_cast<unsigned char>(static_cast<unsigned>(enemyRow->m_monInfo.m_attributes) >> 21);
        if (otherFlags & 1)
            continue;
        if (!g_combatManager->m_cells[enemyRow->m_gridIndex].m_validMove)
            continue;
        best = cppMax(getTraitorValue(enemy, enemyRow), best);
    }
    return best;
}

VA(0x0043a670, 0x291)  // dc 0x40bb8
void type_AI_spellcaster::considerSingleEnchantment(type_spell_choice* choice, long group) const
{
    TEnchantValue valueFunc = getEnchantmentFunction(choice->m_spell);
    const army* best = 0;
    for (long i = 0; i < g_combatManager->m_numArmies[group]; i++) {
        const army* target = &g_combatManager->m_armies[group][i];
        if (target->cannotAttack())
            continue;
        long creatureCast = m_isCreatureSpell != 0;
        if (!g_combatManager->validSpellTargetArmy(choice->m_spell, m_side, target, 1,
                                             creatureCast))
            continue;
        if (target->getSpellTime(choice->m_spell))
            continue;
        long value = (this->*valueFunc)(target, *choice);
        if (target->getSpellTime(SPELL_MAGIC_MIRROR)
                && group != m_side && value > 0
                && choice->m_spell != SPELL_DISPEL)
            value = (50 - target->getMirrorEffect()) * value * 2 / 100;
        if (value > choice->m_value) {
            best = target;
            choice->m_value = value;
            choice->m_target = target->m_gridIndex;
        }
    }
    if (best == 0)
        return;
    if (group == m_side) {
        choice->m_castNow = best == g_combatManager->getCurrentArmy()
                || isLastAction()
                || (g_spellTraits[choice->m_spell].m_flags & 0x4000);
    } else {
        choice->m_castNow = shouldAttackNow(*best);
    }
    if (choice->m_spell == SPELL_HASTE)
        choice->m_castNow = 1;
}

// The per-stack value goes through get_enchantment_function's
// pointer-to-member, so `*choice` is SLICED by value onto the stack
// exactly as in get_caliph_value: the family's second parameter is a
// type_enchant_data, and a type_spell_choice derives from it.

// The +0x198 row is read HERE by spell id, which is what the split of
// army.h's spell-row view out of the round view exists for.
VA(0x0043a910, 0x150)  // dc 0x40dc8
void type_AI_spellcaster::considerEnchantment(type_spell_choice* choice, long group) const
{
    if (spellTargetsASingleArmy(choice->m_spell, choice->m_mastery)) {
        considerSingleEnchantment(choice, group);
        return;
    }
    TEnchantValue valueOf = getEnchantmentFunction(choice->m_spell);
    long value = 0;
    for (long i = 0; i < g_combatManager->m_numArmies[group]; i++) {
        const army* target = &g_combatManager->m_armies[group][i];
        if (target->m_spellInfluence[62])
            continue;
        if (target->m_spellInfluence[70])
            continue;
        if (target->m_spellInfluence[74])
            continue;
        unsigned char immune = static_cast<unsigned char>(
            static_cast<unsigned>(target->m_monInfo.m_attributes) >> 21);
        if (immune & 1)
            continue;
        if (target->m_creatureType == CREATURE_FIRST_AID_TENT)
            continue;
        if (target->m_creatureType == CREATURE_AMMO_CART)
            continue;
        if (target->m_spellInfluence[choice->m_spell])
            continue;
        long creatureCast = m_isCreatureSpell != 0;
        if (g_combatManager->validSpellTargetArmy(choice->m_spell, m_side, target, 1,
                                            creatureCast))
            value += (this->*valueOf)(target, *choice);
    }
    choice->m_castNow = 1;
    choice->m_value = value;
}

// E:\gamedcs\ai_tactical.cpp:2553
// Teleport is priced by SIMULATION, not by a formula: for each of our
// walkers that the spell would actually land on, ask
// choose_melee_action what the stack is worth staying put (melee_only
// 0) and what it would be worth allowed to relocate (melee_only 1), and
// take the difference. The two calls leave combatManager's scratch
// answer behind, which is why the body then reads field_3c / field_40 /
// field_44 rather than anything it computed itself: 6 is the action
// code that means "move there", field_44 is the hex chosen, and
// field_40 is what lands in choice->field_18.

// Those calls also PERTURB the AI target state for both sides, so the
// body latches a flag the first time it commits to one and re-runs
// combatManager::find_AI_targets for group 0 and 1 on the way out. The
// flag is set before the first choose_melee_action, not after the
// choice is taken - the damage is done by asking, not by accepting.

// The trailing choice->field_20 block is the same inlined
// is_last_action test consider_sacrifice carries, with the acting-stack
// comparison widened by this stack's own disabled triple. Note the two
// are NOT the same helper: this one gates on the stack's disabled
// triple, consider_sacrifice's on `field_1c` and the HEALED stack -
// which is why neither can be factored into a shared function without
// putting a body in the image retail does not carry.

VA(0x0043aa60, 0x235)  // anchor-callee, dc 0x40ec0
void type_AI_spellcaster::considerTeleport(type_spell_choice* choice) const
{
    unsigned char moved = 0;
    const army* ourArmy = g_combatManager->m_armies[m_side];
    long count = g_combatManager->m_numArmies[m_side];
    for (; count-- > 0; ++ourArmy) {
        unsigned char immune = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 21);
        if (immune & 1)
            continue;
        unsigned char noTarget = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 26);
        if (noTarget & 1)
            continue;
        long creatureCast = m_isCreatureSpell != 0;
        if (!g_combatManager->validSpellTargetArmy(SPELL_TELEPORT, m_side, ourArmy, 1,
                                             creatureCast))
            continue;
        if (ourArmy->canShoot(0))
            continue;
        moved = 1;
        long before = g_combatManager->chooseMeleeAction(ourArmy, 0, 0, m_side);
        long gain = g_combatManager->chooseMeleeAction(ourArmy, 1, 0, m_side)
                    - before;
        if (g_combatManager->m_nextAction != AI_ORDER_MOVE_AND_ATTACK)
            continue;
        if (g_combatManager->m_nextActionGridIndex == ourArmy->m_gridIndex)
            continue;
        if (gain <= choice->m_value)
            continue;
        choice->m_value = gain;
        choice->m_target = ourArmy->m_gridIndex;
        choice->m_secondTargetHex = g_combatManager->m_nextActionExtra;
        long last = 1;
        const army* current = &g_combatManager->m_armies[g_combatManager->m_actingSide]
                                                      [g_combatManager->m_actingSlot];
        if (ourArmy != current && !ourArmy->m_spellInfluence[62]
                && !ourArmy->m_spellInfluence[70] && !ourArmy->m_spellInfluence[74]) {
            long total = g_combatManager->m_numArmies[m_side];
            for (long j = 0; j < total; j++) {
                const army* other = &g_combatManager->m_armies[m_side][j];
                if (other->m_monInfo.m_attributes & 0x200040)
                    continue;
                if (other->m_spellInfluence[62])
                    continue;
                if (other->m_spellInfluence[70])
                    continue;
                if (other->m_spellInfluence[74])
                    continue;
                unsigned char idle = static_cast<unsigned char>(static_cast<unsigned>(other->m_monInfo.m_attributes) >> 26);
                if (idle & 1)
                    continue;
                if (other != current) {
                    last = 0;
                    break;
                }
            }
        }
        choice->m_castNow = static_cast<unsigned char>(last);
    }
    if (moved) {
        for (long group = 0; group < 2; group++)
            g_combatManager->findAITargets(group, 0, 0, &m_estimate, 0);
    }
}

// E:\gamedcs\ai_tactical.cpp:2608
// Resurrection and Animate Dead share one pricer: for each of our own
// stacks the spell would land on, how many creatures come back and
// what are they worth. The two lookups are cmbtmgr.h's own
// find_resurrection_target SELECTOR inlined - the header already
// records that retail expands it rather than emitting it - and the
// second one exists because a DOUBLE-WIDE stack (creature bit 0)
// occupies two hexes and the lookup may name the other one.

// Two refusals shaped by the AI's own caution: a stack that has lost
// less than three quarters of its troops is not worth healing unless
// the fight is already decided, and BASIC Resurrection is refused
// outright in a decided fight. A won fight (our live value ahead and
// the odds ladder at its bottom rung) doubles the value.

// The trailing choice->field_20 is consider_teleport's inlined
// is_last_action once more, here with `field_1c` standing in for the
// disabled triple.

// Residual (97.0996%, audited 2026-08-22): the helper spelling is already
// the winning one (94.46 -> 97.10), and the remaining CFG is closed: both
// objects have 29 conditional branches and one return, with every mnemonic
// and symbolic target agreeing. The explicit delta is confined to the
// expanded is_last_action tail. Retail keeps the acting stack and live-stack
// count in EAX/ESI and merges the false/true result in EAX; this compile
// rotates the same values through ESI/ECX and merges the byte in EBX, leaving
// one extra instruction (232 against 231). `why-reg` measures distance 45;
// all 12 catalog mutations are flat or worse (naming origNumTroops and
// un-naming creature_cast are flat, while every volatile, store-order and
// choice->spell probe regresses). The remaining non-tail rows are only
// gpCombatManager relocation-name differences. This is C1 register-handle
// state, with no source-addressable lever found.
VA(0x0043aca0, 0x2AE)  // anchor-callee, dc 0x4101c
void type_AI_spellcaster::considerResurrect(type_spell_choice* choice) const
{
    const army* ourArmy = g_combatManager->m_armies[m_side];
    long count = g_combatManager->m_numArmies[m_side];
    for (; count-- > 0; ++ourArmy) {
        long creatureCast = m_isCreatureSpell != 0;
        if (!g_combatManager->validSpellTargetArmy(choice->m_spell, m_side, ourArmy, 1,
                                             creatureCast))
            continue;
        long hex = ourArmy->m_gridIndex;
        if (g_combatManager->findResurrectionTarget(choice->m_spell, m_side, hex, 0)
                != ourArmy) {
            if ((ourArmy->m_monInfo.m_attributes & 1) == 0)
                continue;
            hex = ourArmy->getSecondGridIndex();
            if (g_combatManager->findResurrectionTarget(choice->m_spell, m_side, hex, 0)
                    != ourArmy)
                continue;
        }
        long healable = (g_spellTraits[choice->m_spell].m_powerFactor * choice->m_power
                         + g_spellTraits[choice->m_spell].m_masteryBonus[choice->m_mastery])
                        / ourArmy->m_monInfo.m_hitPoints;
        long dead = ourArmy->m_origNumTroops - ourArmy->m_numTroops;
        if (healable > dead) {
            if (dead < ourArmy->m_origNumTroops * 3 / 4) {
                if (!m_winLikely)
                    continue;
            }
        }
        long healed = cppMin(healable, dead);
        if (healed < 1)
            continue;
        if (choice->m_spell == SPELL_RESURRECTION
                && choice->m_mastery < eMasteryAdvanced && m_winLikely)
            continue;
        long value = static_cast<long>(
            ourArmy->getUnitCombatValue(m_estimate.m_lowestAttack,
                                            m_estimate.m_lowestDefense,
                                            ourArmy->canShoot(0), 0)
            * healed);
        if (m_estimate.m_awakeFriendlyValue > m_estimate.m_awakeEnemyValue && m_estimate.m_roundsLeft <= 1)
            value += value;
        if (value <= choice->m_value)
            continue;
        choice->m_value = value;
        choice->m_target = hex;
        unsigned char last = 1;
        const army* current = &g_combatManager->m_armies[g_combatManager->m_actingSide]
                                                      [g_combatManager->m_actingSlot];
        if (ourArmy != current && !m_winLikely)
            last = isLastAction();
        choice->m_castNow = last;
    }
}

// E:\gamedcs\ai_tactical.cpp:2685
// The three-argument Sacrifice pricer: given ONE stack to heal and the
// hex it stands on, find the friendly stack that is worth the least to
// spend on it. The victim's own combat value is subtracted from the
// resurrection it buys, so a stack only gets sacrificed when the trade
// is positive.

// The count it resurrects is the spell's mastery row plus the VICTIM's
// per-creature hit points (akCreatureTypeTraits[..].hitPoints) plus the
// caster's power, all scaled by how many of the victim there are and
// divided by the healed stack's own hit points.

// The trailing `choice.field_20` block is the is_last_action test
// (dc 0x3d6xx, DC_ONLY - it has no retail body because it is inlined at
// every site): the choice is flagged when the healed stack IS the
// acting stack, or when no OTHER stack on our side can still act. Its
// scan is the same creatureId 0x200040 / disabled-triple / bit-26 walk
// should_attack_now opens with, and retail memory-homes its index the
// same way.

VA(0x0043af50, 0x284)  // anchor-callee, dc 0x41278
void type_AI_spellcaster::considerSacrifice(type_spell_choice& choice, const army* healedArmy, long targetHex) const
{
    const army* victim = g_combatManager->m_armies[m_side];
    long count = g_combatManager->m_numArmies[m_side];
    for (; count-- > 0; ++victim) {
        unsigned char immune = static_cast<unsigned char>(static_cast<unsigned>(victim->m_monInfo.m_attributes) >> 21);
        if (immune & 1)
            continue;
        if (victim == healedArmy)
            continue;
        long creatureCast = m_isCreatureSpell != 0;
        if (!g_combatManager->validSpellTargetArmy(choice.m_spell, m_side, victim, 0,
                                             creatureCast))
            continue;
        long resurrected = (g_spellTraits[choice.m_spell].m_masteryBonus[choice.m_mastery]
                            + g_creatureTypeTraits[victim->m_creatureType].m_hitPoints
                            + choice.m_power)
                           * victim->m_numTroops / healedArmy->m_monInfo.m_hitPoints;
        long missing = healedArmy->m_origNumTroops - healedArmy->m_numTroops;
        if (resurrected > missing
                && missing < healedArmy->m_origNumTroops * 3 / 4
                && !m_winLikely)
            continue;
        long healed = cppMin(resurrected, missing);
        if (healed < 1)
            continue;
        long value = static_cast<long>(
            healedArmy->getUnitCombatValue(m_estimate.m_lowestAttack,
                                               m_estimate.m_lowestDefense,
                                               victim->canShoot(0), 0) * healed);
        value -= victim->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                m_estimate.m_lowestDefense);
        if (value <= 0)
            continue;
        if (m_estimate.m_awakeFriendlyValue > m_estimate.m_awakeEnemyValue && m_estimate.m_roundsLeft <= 1)
            value += value;
        if (value <= choice.m_value)
            continue;
        choice.m_value = value;
        choice.m_target = targetHex;
        choice.m_secondTargetHex = victim->m_gridIndex;
        unsigned char last = 1;
        const army* current = &g_combatManager->m_armies[g_combatManager->m_actingSide]
                                                      [g_combatManager->m_actingSlot];
        if (healedArmy != current && !m_winLikely) {
            long total = g_combatManager->m_numArmies[m_side];
            for (long j = 0; j < total; j++) {
                const army* ourArmy = &g_combatManager->m_armies[m_side][j];
                if (ourArmy->m_monInfo.m_attributes & 0x200040)
                    continue;
                if (ourArmy->m_spellInfluence[62])
                    continue;
                if (ourArmy->m_spellInfluence[70])
                    continue;
                if (ourArmy->m_spellInfluence[74])
                    continue;
                unsigned char noTarget = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 26);
                if (noTarget & 1)
                    continue;
                if (ourArmy != current) {
                    last = 0;
                    break;
                }
            }
        }
        choice.m_castNow = last;
    }
}

VA(0x0043b1e0, 0xF2)  // dc 0x414a0
void type_AI_spellcaster::considerSacrifice(type_spell_choice& choice) const
{
    const army* candidateHealedArmy = g_combatManager->m_armies[m_side];
    long count = g_combatManager->m_numArmies[m_side];
    for (; count-- > 0; ++candidateHealedArmy) {
        long creatureSpell = m_isCreatureSpell != 0;
        if (!g_combatManager->validSpellTargetArmy(
                choice.m_spell, m_side, candidateHealedArmy, 1, creatureSpell))
            continue;

        long candidateTargetHex = candidateHealedArmy->m_gridIndex;
        army* target = g_combatManager->findResurrectionTarget(
            choice.m_spell, m_side, candidateTargetHex, 0);

        if (target != candidateHealedArmy) {
            if (!(candidateHealedArmy->m_monInfo.m_attributes & 1))
                continue;
            candidateTargetHex = candidateHealedArmy->getSecondGridIndex();
            target = g_combatManager->findResurrectionTarget(
                choice.m_spell, m_side, candidateTargetHex, 0);
            if (target != candidateHealedArmy)
                continue;
        }
        considerSacrifice(choice, candidateHealedArmy, candidateTargetHex);
    }
}

VA(0x0043b2e0, 0x85)  // dc 0x41558
long type_AI_spellcaster::getCloneValue(const army* ourArmy, type_enchant_data caster) const
{
    if (!m_winLikely) {
        unsigned char noTarget = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 26);
        if ((noTarget & 1) == 0) {
            const army* target = ourArmy->getAITarget();
            if (target != 0
                    && ourArmy->getAITargetTime(ourArmy->getSpeed()) <= 1) {
                long damage = ourArmy->getAverageDamage(target, ourArmy->canShoot(0),
                                                           ourArmy->m_numTroops, 1, 0);
                return target->getLossCombatValue(m_estimate.m_lowestAttack,
                                                     m_estimate.m_lowestDefense,
                                                     target->canShoot(0), damage,
                                                     m_estimate.m_killsOnly);
            }
        }
    }
    return 0;
}

VA(0x0043b370, 0x18E)  // dc 0x41624
long type_AI_spellcaster::getCurseValue(const army* enemy, type_enchant_data caster) const
{
    if ((m_enemyCanAttack & (1 << enemy->m_bitIndex)) != 0 && !m_estimate.m_killsOnly && !m_winLikely) {
        long value = enemy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                   m_estimate.m_lowestDefense);
        double oldAverage = enemy->getAverageDamage();
        double newAverage = enemy->m_monInfo.m_damageLowBound - g_spellTraits[SPELL_CURSE].m_masteryBonus[caster.m_mastery];
        if (newAverage < 1.0)
            newAverage = 1.0;
        double decrease = newAverage / oldAverage;
        value = static_cast<long>(value - sqrt(decrease) * value);
        double portion;
        if (caster.m_duration >= m_estimate.m_roundsLeft)
            portion = 1.0;
        else
            portion = static_cast<double>(caster.m_duration) / static_cast<double>(m_estimate.m_roundsLeft);
        unsigned char slowFlag = static_cast<unsigned char>(static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 26);
        double scale;
        if ((slowFlag & 1)
                && (portion = portion - 1.0 / static_cast<double>(m_estimate.m_roundsLeft)) < 0.0)
            scale = 0.0;
        else
            scale = portion;
        value = static_cast<long>(value * scale);
        if (caster.m_checkResistance) {
            long creatureCast = m_isCreatureSpell != 0;
            value = static_cast<long>(
                g_combatManager->spellCastWorkChance(SPELL_CURSE, m_side, enemy, 0, 1,
                                                     creatureCast) * value);
        }
        return value;
    }
    return 0;
}

VA(0x0043b500, 0x17D)  // dc 0x41890
long type_AI_spellcaster::getForgetfulnessValue(const army* enemy, type_enchant_data caster) const
{
    if (enemy->canShoot(0)) {
        unsigned char immune = static_cast<unsigned char>(static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 23);
        if ((immune & 1) == 0 && !m_estimate.m_killsOnly && !m_winLikely) {
            double damage = enemy->getUnitCombatValue(m_estimate.m_lowestAttack,
                                                         m_estimate.m_lowestDefense, 1, 0);
            damage -= enemy->getUnitCombatValue(m_estimate.m_lowestAttack,
                                                   m_estimate.m_lowestDefense, 0, 0);
            double hits = enemy->getTotalHitPoints(0);
            long value = static_cast<long>(hits * damage / enemy->m_monInfo.m_hitPoints);
            double portion;
            if (caster.m_duration >= m_estimate.m_roundsLeft)
                portion = 1.0;
            else
                portion = static_cast<double>(caster.m_duration) / static_cast<double>(m_estimate.m_roundsLeft);
            unsigned char slowFlag = static_cast<unsigned char>(static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 26);
            double scale;
            if ((slowFlag & 1)
                    && (portion = portion - 1.0 / static_cast<double>(m_estimate.m_roundsLeft)) < 0.0)
                scale = 0.0;
            else
                scale = portion;
            value = static_cast<long>(value * scale);
            if (caster.m_checkResistance) {
                long creatureCast = m_isCreatureSpell != 0;
                value = static_cast<long>(
                    g_combatManager->spellCastWorkChance(SPELL_FORGETFULNESS, m_side, enemy,
                                                         0, 1, creatureCast) * value);
            }
            return value;
        }
    }
    return 0;
}

VA(0x0043b680, 0x10)  // dc 0x41a74
long type_AI_spellcaster::unimplemented(const army* enemy,
                                        type_enchant_data caster) const
{
    return 0;
}

VA(0x0043b690, 0x251)  // dc 0x41a7c
type_AI_spellcaster::TEnchantValue type_AI_spellcaster::getEnchantmentFunction(SpellID spell) const
{
    switch (spell) {
    case SPELL_AGE:
        return &type_AI_spellcaster::getAgeValue;
    case SPELL_AIR_SHIELD:
        return &type_AI_spellcaster::getAirShieldValue;
    case SPELL_ANTI_MAGIC:
        return &type_AI_spellcaster::getAntimagicValue;
    case SPELL_MAGIC_MIRROR:
        return &type_AI_spellcaster::getBacklashValue;
    case SPELL_BERSERK:
        return &type_AI_spellcaster::getBerserkValue;
    case SPELL_BLESS:
        return &type_AI_spellcaster::getBlessValue;
    case SPELL_BLIND:
    case SPELL_PARALYZE:
        return &type_AI_spellcaster::getBlindValue;
    case SPELL_BLOODLUST:
        return &type_AI_spellcaster::getBloodLustValue;
    case SPELL_CLONE:
        return &type_AI_spellcaster::getCloneValue;
    case SPELL_COUNTERSTRIKE:
        return &type_AI_spellcaster::getCounterstrokeValue;
    case SPELL_CURE:
        return &type_AI_spellcaster::getCureValue;
    case SPELL_CURSE:
        return &type_AI_spellcaster::getCurseValue;
    case SPELL_DISPEL:
        return &type_AI_spellcaster::getDispelValue;
    case SPELL_DISRUPTING_RAY:
        return &type_AI_spellcaster::getDisruptiveRayValue;
    case SPELL_MAGIC_ARROW:
    case SPELL_ICE_BOLT:
    case SPELL_LIGHTNING_BOLT:
    case SPELL_IMPLOSION:
    case SPELL_TITANS_LIGHTNING_BOLT:
        return &type_AI_spellcaster::getDamageSpellValue;
    case SPELL_DISEASE:
        return &type_AI_spellcaster::getDiseaseValue;
    case SPELL_FORGETFULNESS:
        return &type_AI_spellcaster::getForgetfulnessValue;
    case SPELL_FORTUNE:
        return &type_AI_spellcaster::getFortuneValue;
    case SPELL_FIRE_SHIELD:
        return &type_AI_spellcaster::getFireShieldValue;
    case SPELL_FRENZY:
        return &type_AI_spellcaster::getFrenzyValue;
    case SPELL_HYPNOTIZE:
        return &type_AI_spellcaster::getHypnotizeValue;
    case SPELL_MIRTH:
        return &type_AI_spellcaster::getMirthValue;
    case SPELL_MISFORTUNE:
        return &type_AI_spellcaster::getMisfortuneValue;
    case SPELL_SLOW:
        return &type_AI_spellcaster::getMuckAndMireValue;
    case SPELL_POISON:
        return &type_AI_spellcaster::getPoisonValue;
    case SPELL_PRAYER:
        return &type_AI_spellcaster::getPrayerValue;
    case SPELL_PRECISION:
        return &type_AI_spellcaster::getPrecisionValue;
    case SPELL_PROTECTION_FROM_AIR:
        return &type_AI_spellcaster::getAirProtectionValue;
    case SPELL_PROTECTION_FROM_EARTH:
        return &type_AI_spellcaster::getEarthProtectionValue;
    case SPELL_PROTECTION_FROM_FIRE:
        return &type_AI_spellcaster::getFireProtectionValue;
    case SPELL_PROTECTION_FROM_WATER:
        return &type_AI_spellcaster::getWaterProtectionValue;
    case SPELL_SHIELD:
        return &type_AI_spellcaster::getShieldValue;
    case SPELL_SLAYER:
        return &type_AI_spellcaster::getSlayerValue;
    case SPELL_SORROW:
        return &type_AI_spellcaster::getSorrowValue;
    case SPELL_HASTE:
        return &type_AI_spellcaster::getHasteValue;
    case SPELL_STONE_SKIN:
        return &type_AI_spellcaster::getToughSkinValue;
    case SPELL_WEAKNESS:
        return &type_AI_spellcaster::getWeaknessValue;
    case SPELL_BIND:
        return &type_AI_spellcaster::unimplemented;
    }
    return &type_AI_spellcaster::unimplemented;
}

// The value is then "what would our shooters be worth if the wall were
// not in the way", and it is proportional: `damage / (3 * total)` of
// each blocked shooter's combat value, where `total` is the whole
// castle's remaining strength summed over wallTargets and `damage` is
// what one cast takes off, capped at that total. A shooter with NO
// target at all counts its FULL value instead, but only while some
// segment is already at zero strength - which is what the running
// minimum is for.

VA(0x0043b8f0, 0x224)  // dc 0x41c30
void type_AI_spellcaster::considerEarthquake(type_spell_choice* choice) const
{
    if (m_side == 1)
        return;
    if (m_winLikely)
        return;
    if (g_combatManager->m_fortificationLevel == COMBAT_FORTIFICATION_NONE)
        return;
    long lowest = 0x7fff;
    long total = 0;
    for (long i = 0; i < WALL_TARGET_COUNT; i++) {
        long strength = g_combatManager->m_wallStrength[
            combatManager::s_wallTargets[i].m_wall];
        total += strength;
        lowest = cppMin(lowest, strength);
    }
    if (total == 0)
        return;
    const army* enemy = g_combatManager->m_armies[m_enemySide];
    long count = g_combatManager->m_numArmies[m_enemySide];
    for (; count-- > 0; ++enemy) {
        unsigned char immune = static_cast<unsigned char>(
            static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 21);
        if (immune & 1)
            continue;
        if (enemy->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (inCastle(enemy->m_gridIndex))
            break;
    }
    if (count < 0)
        return;
    long value = 0;
    const army* ourArmy = g_combatManager->m_armies[m_side];
    long damage = cppMin<long>(
        g_spellTraits[choice->m_spell].m_masteryBonus[choice->m_mastery], total);
    long remaining = g_combatManager->m_numArmies[m_side];
    for (; remaining-- > 0; ++ourArmy) {
        unsigned char immune = static_cast<unsigned char>(
            static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 21);
        if (immune & 1)
            continue;
        if (ourArmy->m_spellInfluence[62])
            continue;
        if (ourArmy->m_spellInfluence[70])
            continue;
        if (ourArmy->m_spellInfluence[74])
            continue;
        if (ourArmy->m_creatureType == CREATURE_FIRST_AID_TENT)
            continue;
        if (ourArmy->m_creatureType == CREATURE_AMMO_CART)
            continue;
        const army* target = ourArmy->getAITarget();
        if (target == 0) {
            if (lowest > 0)
                value += ourArmy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                          m_estimate.m_lowestDefense);
            continue;
        }
        if (!ourArmy->canShoot(0))
            continue;
        if (!g_combatManager->shotIsThroughWall(ourArmy, ourArmy->m_gridIndex,
                                                target->m_gridIndex))
            continue;
        value += ourArmy->getTotalCombatValue(m_estimate.m_lowestAttack,
                                                  m_estimate.m_lowestDefense)
                 * damage / (total * 3);
    }
    choice->m_value = value;
    choice->m_castNow = 1;
}

// E:\gamedcs\ai_tactical.cpp:3093
// DC 0x41e5c and dispatcher line 3167 prove this ordinary helper boundary.
// Line 3098 calls get_mastery_value; line 3107 writes cast_now after the split.
DC_ONLY(0x41e5c, 0x78)
void type_AI_spellcaster::considerSummon(type_spell_choice& choice) const
{
    if (m_winLikely)
        return;
    if (!g_combatManager->ableToSummonElemental(choice.m_spell, m_side))
        return;
    long power = choice.getMasteryValue() * choice.m_power;
    if (m_estimate.m_killsOnly) {
        choice.m_value = power * 1000;
    } else {
        TCreatureType summoned = getElementalType(choice.m_spell);
        choice.m_value = g_creatureTypeTraits[summoned].m_aiValue * power;
    }
    choice.m_castNow = 1;
}

VA(0x0043bb20, 0x3FC)  // dc 0x41ed4
void type_AI_spellcaster::considerSpell(type_spell_choice* choice) const
{
    switch (choice->m_spell) {
    case SPELL_RESURRECTION:
    case SPELL_ANIMATE_DEAD:
        considerResurrect(choice);
        return;
    case SPELL_CHAIN_LIGHTNING:
        considerChainLightning(choice);
        return;
    case SPELL_DEATH_RIPPLE:
    case SPELL_DESTROY_UNDEAD:
    case SPELL_ARMAGEDDON:
        considerMassDamage(*choice);
        return;
    case SPELL_DISPEL: {
        considerEnchantment(choice, m_side);
        if (choice->m_mastery == eMasteryAdvanced)
            considerEnchantment(choice, m_enemySide);
        if (choice->m_mastery == eMasteryExpert) {
            type_spell_choice mirror = *choice;
            considerEnchantment(&mirror, m_enemySide);
            choice->m_value += mirror.m_value;
        }
        break;
    }
    case SPELL_EARTHQUAKE:
        considerEarthquake(choice);
        return;
    case SPELL_FROST_RING:
    case SPELL_FIREBALL:
    case SPELL_INFERNO:
    case SPELL_METEOR_SHOWER:
        considerAreaEffect(choice);
        return;
    case SPELL_SACRIFICE:
        considerSacrifice(*choice);
        return;
    case SPELL_SUMMON_FIRE_ELEMENTAL:
    case SPELL_SUMMON_EARTH_ELEMENTAL:
    case SPELL_SUMMON_WATER_ELEMENTAL:
    case SPELL_SUMMON_AIR_ELEMENTAL:
        considerSummon(*choice);
        return;
    case SPELL_TELEPORT:
        considerTeleport(choice);
        return;
    }
    const SSpellTraits* traits = &g_spellTraits[choice->m_spell];
    if ((traits->m_flags & 0x70) == 0)
        return;
    if (traits->m_karma >= 0)
        considerEnchantment(choice, m_side);
    if (traits->m_karma <= 0)
        considerEnchantment(choice, m_enemySide);
}

// E:\gamedcs\ai_tactical.cpp:3191
VA(0x0043bf20, 0x119)  // anchor-global, dc 0x420ac
void type_AI_spellcaster::setMeleeEnemies()
{
    const army* ourArmy = &g_combatManager->m_armies[m_side][0];
    memset(m_meleeEnemies, 0, sizeof(m_meleeEnemies));
    for (long i = 0; i < g_combatManager->m_numArmies[m_side]; i++) {
        if (ourArmy->cannotAttack() || ourArmy->getSpellTime(SPELL_BLIND))
            continue;
        const army* target = ourArmy->getAITarget();
        if (!target || ourArmy->canShoot(0) || target->getAITargetTime() > 1)
            continue;
        m_meleeEnemies[i].m_enemy = target;
        long damage = target->getAverageDamage(ourArmy, 0, target->m_numTroops, 0, 0);
        m_meleeEnemies[i].m_damage = damage;
        m_meleeEnemies[i].m_totalDamage = damage;
        m_meleeEnemies[i].m_count = 1;
    }
}

#if 0  // @carcass

// E:\gamedcs\ai_tactical.cpp:3221
DC_ONLY(0x42170, 0xAE)
void type_AI_spellcaster::set_worst_enemies()
{
    // @stub
}

// E:\gamedcs\ai_tactical.cpp:3237
DC_ONLY(0x42220, 0x5A)
void type_AI_spellcaster::add_enemy(type_AI_enemy_data* sum, const army* our_army, const army* enemy, unsigned char ranged)
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\ai_tactical.cpp:3254
// The full two-sided census: set_melee_enemies fills enemies[] with the
// stack each of ours is already committed to, then this body walks
// every live enemy against every one of ours and sorts it into the
// RANGED census (attacks[]) or, if it can reach us on foot and is not
// the one already recorded, back into enemies[]. worst_enemies[] is
// then the per-stack maximum of the two.

// The DC roster's add_enemy (dc 0x42220) and set_worst_enemies
// (dc 0x42170) have NO retail slot: the count/total/max update stands
// open at both census sites and the merge loop stands open at the tail,
// so retail's source spells all three out here. Writing them as members
// would put two bodies in the image that retail does not carry.

VA(0x0043c040, 0x2E6)  // anchor-global, dc 0x4227c
void type_AI_spellcaster::findEnemyAttacks()
{
    m_canBeAttacked = 0;
    m_enemyCanAttack = 0;
    memset(m_worstEnemies, 0, sizeof(m_worstEnemies));
    memset(m_attacks, 0, sizeof(m_attacks));
    setMeleeEnemies();
    const army* ourArmy = &g_combatManager->m_armies[m_side][0];
    { for (long i = 0; i < g_combatManager->m_numArmies[m_side]; i++, ourArmy++) {
        unsigned char flags = static_cast<unsigned char>(static_cast<unsigned>(ourArmy->m_monInfo.m_attributes) >> 21);
        if (flags & 1)
            continue;
        if (ourArmy->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        army* enemy = &g_combatManager->m_armies[m_enemySide][0];
        const army* meleeEnemy = m_meleeEnemies[i].m_enemy;
        for (long j = 0; j < g_combatManager->m_numArmies[m_enemySide]; j++, enemy++) {
            if (enemy->m_spellInfluence[62] || enemy->m_spellInfluence[70] || enemy->m_spellInfluence[74])
                continue;
            unsigned char enemyFlags = static_cast<unsigned char>(static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 21);
            if (enemyFlags & 1)
                continue;
            if (enemy->m_creatureType == CREATURE_FIRST_AID_TENT
                    || enemy->m_creatureType == CREATURE_AMMO_CART)
                continue;
            if (enemy->m_spellInfluence[60])
                continue;
            if (enemy->m_creatureType == CREATURE_ARROW_TOWER)
                continue;
            if (enemy->getTotalHitPoints(1) == 0)
                continue;
            if (enemy->canShoot(0)) {
                long damage = enemy->getAverageDamage(ourArmy, 1, enemy->m_numTroops, 0, 0);
                m_attacks[i].m_count++;
                m_attacks[i].m_totalDamage += damage;
                if (damage > m_attacks[i].m_damage) {
                    m_attacks[i].m_damage = damage;
                    m_attacks[i].m_enemy = enemy;
                }
            } else {
                if (enemy == meleeEnemy)
                    continue;
                if ((enemy->getAIPossibleTargets() & (1 << i)) == 0)
                    continue;
                m_canBeAttacked |= 1 << j;
                long damage = enemy->getAverageDamage(ourArmy, 0, enemy->m_numTroops, 0, 0);
                m_meleeEnemies[i].m_count++;
                m_meleeEnemies[i].m_totalDamage += damage;
                if (damage > m_meleeEnemies[i].m_damage) {
                    m_meleeEnemies[i].m_damage = damage;
                    m_meleeEnemies[i].m_enemy = enemy;
                }
            }
            m_enemyCanAttack |= 1 << enemy->m_bitIndex;
        }
    } }
    { for (long i = 0; i < g_combatManager->m_numArmies[m_side]; i++) {
        m_worstEnemies[i] = m_meleeEnemies[i];
        if (m_attacks[i].m_damage > m_worstEnemies[i].m_damage)
            m_worstEnemies[i] = m_attacks[i];
    } }
}

VA(0x0043c330, 0x16C)  // dc 0x423f4
long type_AI_spellcaster::getOgreMageValue(const army* target) const
{
    if (target->m_spellInfluence[43])
        return 0;
    TSkillMastery mastery = eMasteryAdvanced;
    unsigned char expert = 0;
    switch (g_combatManager->m_magicTerrain) {
    case COMBAT_SPELL_RESTRICTION_ALL_EXPERT:
        expert = 1;
        break;
    case COMBAT_SPELL_RESTRICTION_WATER_EXPERT:
        expert = static_cast<unsigned char>(
            g_spellTraits[SPELL_BLOODLUST].m_schoolBits >> 2) & 1;
        break;
    case COMBAT_SPELL_RESTRICTION_FIRE_EXPERT:
        expert = static_cast<unsigned char>(
            g_spellTraits[SPELL_BLOODLUST].m_schoolBits >> 1) & 1;
        break;
    case COMBAT_SPELL_RESTRICTION_EARTH_EXPERT:
        expert = static_cast<unsigned char>(
            g_spellTraits[SPELL_BLOODLUST].m_schoolBits >> 3) & 1;
        break;
    case COMBAT_SPELL_RESTRICTION_AIR_EXPERT:
        expert = static_cast<unsigned char>(
            g_spellTraits[SPELL_BLOODLUST].m_schoolBits) & 1;
        break;
    }
    if (expert)
        mastery = eMasteryExpert;
    type_spell_choice choice(SPELL_BLOODLUST, mastery, 6, 6);
    if (spellTargetsASingleArmy(SPELL_BLOODLUST, mastery))
        return getBloodLustValue(target, choice);
    considerEnchantment(&choice, m_side);
    return choice.m_value;
}

VA(0x0043c4a0, 0x180)  // dc 0x424c4
long type_AI_spellcaster::getCaliphValue(const army* target) const
{
    long total = 0;
    long count = 0;
    for (long spell = 10; spell < 70; spell++) {
        if (!isValidCaliphSpell(spell, target))
            continue;
        TSkillMastery mastery = eMasteryAdvanced;
        unsigned char expert = 0;
        switch (g_combatManager->m_magicTerrain) {
        case COMBAT_SPELL_RESTRICTION_ALL_EXPERT:
            expert = 1;
            break;
        case COMBAT_SPELL_RESTRICTION_WATER_EXPERT:
            expert = static_cast<unsigned char>(
                g_spellTraits[spell].m_schoolBits >> 2) & 1;
            break;
        case COMBAT_SPELL_RESTRICTION_FIRE_EXPERT:
            expert = static_cast<unsigned char>(
                g_spellTraits[spell].m_schoolBits >> 1) & 1;
            break;
        case COMBAT_SPELL_RESTRICTION_EARTH_EXPERT:
            expert = static_cast<unsigned char>(
                g_spellTraits[spell].m_schoolBits >> 3) & 1;
            break;
        case COMBAT_SPELL_RESTRICTION_AIR_EXPERT:
            expert = static_cast<unsigned char>(
                g_spellTraits[spell].m_schoolBits) & 1;
            break;
        }
        if (expert)
            mastery = eMasteryExpert;
        count++;
        TEnchantValue valueOf = getEnchantmentFunction(spell);
        if (valueOf != 0) {
            type_spell_choice choice(spell, mastery, 6, 6);
            if (spellTargetsASingleArmy(spell, mastery)) {
                total += (this->*valueOf)(target, choice);
            } else {
                considerEnchantment(&choice, m_side);
                total += choice.m_value;
            }
        }
    }
    if (count == 0)
        return 0;
    return total / count;
}

VA(0x0043c620, 0x1DD)
long type_AI_spellcaster::getFaerieDragonSpellValue(
        long hex, long power, SpellID spell)
{
    TSkillMastery mastery = eMasteryAdvanced;
    unsigned char expert = 0;
    switch (g_combatManager->m_magicTerrain) {
    case COMBAT_SPELL_RESTRICTION_ALL_EXPERT:
        expert = 1;
        break;
    case COMBAT_SPELL_RESTRICTION_WATER_EXPERT:
        expert = static_cast<unsigned char>(
            g_spellTraits[spell].m_schoolBits >> 2) & 1;
        break;
    case COMBAT_SPELL_RESTRICTION_FIRE_EXPERT:
        expert = static_cast<unsigned char>(
            g_spellTraits[spell].m_schoolBits >> 1) & 1;
        break;
    case COMBAT_SPELL_RESTRICTION_EARTH_EXPERT:
        expert = static_cast<unsigned char>(
            g_spellTraits[spell].m_schoolBits >> 3) & 1;
        break;
    case COMBAT_SPELL_RESTRICTION_AIR_EXPERT:
        expert = static_cast<unsigned char>(
            g_spellTraits[spell].m_schoolBits) & 1;
        break;
    }
    if (expert)
        mastery = eMasteryExpert;

    army* target = g_combatManager->m_cells[hex].getArmy();
    long baseDamage;
    switch (spell) {
    case SPELL_MAGIC_ARROW:
    case SPELL_ICE_BOLT:
    case SPELL_LIGHTNING_BOLT:
    case SPELL_IMPLOSION:
        if (!target)
            return 0;
        baseDamage = g_spellTraits[spell].m_masteryBonus[mastery]
                      + g_spellTraits[spell].m_powerFactor * power;
        return getDamageValue(spell, baseDamage, m_enemyHero, target);

    case SPELL_CHAIN_LIGHTNING:
        if (target) {
            unsigned char immune = static_cast<unsigned char>(
                static_cast<unsigned>(target->m_monInfo.m_attributes) >> 21);
            long creatureCast = m_isCreatureSpell != 0;
            if (!(immune & 1)
                    && g_combatManager->validSpellTargetArmy(
                        SPELL_CHAIN_LIGHTNING, m_side, target, 1,
                        creatureCast)) {
                return getChainLightningValue(power, mastery, target);
            }
        }
        return 0;

    case SPELL_FROST_RING:
    case SPELL_FIREBALL:
    case SPELL_INFERNO:
    case SPELL_METEOR_SHOWER:
        baseDamage = g_spellTraits[spell].m_masteryBonus[mastery]
                      + g_spellTraits[spell].m_powerFactor * power;
        return getAreaEffectValue(spell, baseDamage, mastery, hex);
    }
    return 0;
}

// E:\gamedcs\ai_tactical.cpp:3377, dc 0x425a8.
// The scan itself: walk the OTHER side's stacks and answer "the fight
// is already decided" in field_1c unless some enemy is still magic-
// vulnerable (creature bit 21 clear), still alive, and still able to
// act (creature bit 6 clear). The walk is the TU's `count-- > 0`
// pointer form, the same one consider_teleport carries.
DC_ONLY(0x425a8, 0x68)
inline void type_AI_spellcaster::checkSimulation()
{
    const army* enemy = g_combatManager->m_armies[m_enemySide];
    long count = g_combatManager->m_numArmies[m_enemySide];
    for (; count-- > 0; ++enemy) {
        unsigned char immune = static_cast<unsigned char>(
            static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 21);
        if (immune & 1)
            continue;
        if (enemy->getTotalHitPoints(1) <= 0)
            continue;
        unsigned char idle = static_cast<unsigned char>(
            static_cast<unsigned>(enemy->m_monInfo.m_attributes) >> 6);
        if (idle & 1)
            continue;
        m_winLikely = 0;
        return;
    }
    m_winLikely = 1;
}

// E:\gamedcs\ai_tactical.cpp:3398
// DC 0x42610 proves the const helper and early returns; cast_spell calls it
// at line 3436. Complete also excludes Arrow Towers. Retail expands this
// ordinary helper into 0x43c800; the bracket has no retained body for it.
DC_ONLY(0x42610, 0xA0)
unsigned char type_AI_spellcaster::spellsNotRequired() const
{
    if (!m_winLikely)
        return 0;
    const army* ourArmy = g_combatManager->m_armies[m_side];
    long count = g_combatManager->m_numArmies[m_side];
    for (; count-- > 0; ++ourArmy) {
        if (ourArmy->is(1u << 21))
            continue;
        if (ourArmy->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (ourArmy->getAIExpectedDamage() + ourArmy->m_topCreatureDamage
                >= ourArmy->m_monInfo.m_hitPoints)
            return 0;
    }
    return 1;
}

// Four refusals before the pricing, and two of them corroborate
// domains this tree already carries. `field_53c0 == 2` refuses every
// spell above traits level 1 - which is Cursed Ground's published
// rule and the same code cmbtmgr.h already records as refusing
// creature casts. Either hero wearing artifact 83 refuses every spell
// above level 2 - Recanter's Cloak. The other two are the spell's own
// traits bits: bit 0 gates it at all, and bit 9 marks the spells that
// still make sense while RETREATING.

VA(0x0043c800, 0x308)  // dc 0x426b0
unsigned char type_AI_spellcaster::castSpell(unsigned char retreating)
{
    type_spell_choice best;
    long power = g_combatManager->m_spellPower[m_side];
    long duration = power;
    const armyGroup* enemyGroup = g_combatManager->m_armyGroups[m_enemySide];
    unsigned char inhibited = 0;
    if (m_ourHero->isWieldingArtifact(g_artifactRecantersCloak))
        inhibited = 1;
    if (m_enemyHero) {
        if (m_enemyHero->isWieldingArtifact(g_artifactRecantersCloak))
            inhibited = 1;
    }
    unsigned char healingOnly = spellsNotRequired();
    if (m_ourHero)
        duration = m_ourHero->getSpellDurationBonus() + power;
    if (retreating)
        m_estimate.m_killsOnly = 1;
    for (long spell = 0; spell < hero::NUM_SPELLS; spell++) {
        if (!m_ourHero->spellIsAvailable(spell))
            continue;
        if ((g_spellTraits[spell].m_flags & 1) == 0)
            continue;
        if (retreating) {
            if ((g_spellTraits[spell].m_flags & 0x200) == 0)
                continue;
        }
        if (g_combatManager->m_magicTerrain
                == COMBAT_SPELL_RESTRICTION_NO_CREATURE_SPELLS) {
            if (g_spellTraits[spell].m_level > 1)
                continue;
        }
        if (inhibited) {
            if (g_spellTraits[spell].m_level > 2)
                continue;
        }
        TSkillMastery mastery = m_ourHero->getSpellLevel(
            spell, g_combatManager->m_magicTerrain);
        long cost = m_ourHero->getManaCost(spell, enemyGroup,
                                          g_combatManager->m_magicTerrain);
        if (cost > m_ourHero->m_mana)
            continue;
        if (healingOnly) {
            if (spell != SPELL_RESURRECTION && spell != SPELL_ANIMATE_DEAD)
                continue;
        }
        type_spell_choice choice(spell, mastery, power, duration);
        considerSpell(&choice);
        if (choice.m_value <= 0)
            continue;
        if (m_ourHero->m_mana >= cost * 7)
            choice.m_value = choice.m_value * 5 / 2;
        else
            choice.m_value = static_cast<long>(
                sqrt(static_cast<double>(m_ourHero->m_mana / cost)) * choice.m_value);
        choice.m_value = random(75, 100) * choice.m_value / 100;
        if (choice.m_value > best.m_value)
            best = choice;
    }
    if (best.m_spell != -1) {
        if (best.m_castNow || retreating) {
            g_combatManager->m_nextAction = 1;
            g_combatManager->m_nextActionExtra = best.m_spell;
            g_combatManager->m_nextActionGridIndex = best.m_target;
            g_combatManager->m_nextActionGridIndex2 = best.m_secondTargetHex;
            return 1;
        }
    }
    return 0;
}

#if 0  // @carcass

// ..\stlport\stl_deque.h:128
DC_ONLY(0x42994, 0x2C)
unsigned std::__deque_buf_size(unsigned __n, unsigned size)
{
    // @stub
}

// E:\gamedcs\Army.h:724
DC_ONLY(0x429c0, 0x34)
int army::getMorale(unsigned char apply_limits)
{
    // @stub
}

// E:\gamedcs\Army.h:730
DC_ONLY(0x429f4, 0x34)
int army::getLuck(unsigned char apply_limits)
{
    // @stub
}

// E:\gamedcs\Army.h:825
DC_ONLY(0x42a28, 0x12)
TSkillMastery army::getSpellLevel(SpellID spell)
{
    // @stub
}

// E:\gamedcs\cmbtmgr.h:1466
DC_ONLY(0x42a3c, 0x38)
army* combatManager::findResurrectionTarget(SpellID spell, long group, long hex, unsigned char creature_spell)
{
    // @stub
}

// E:\gamedcs\ai_tactical.cpp:807
DC_ONLY(0x42a74, 0x34)
void* type_AI_spellcaster::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_tactical.cpp:2183
DC_ONLY(0x42aa8, 0x306)
void army::army(const army* __that)
{
    // @stub
}

// ..\stlport\stl_deque.h:622
DC_ONLY(0x42db0, 0xB8)
void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::deque<enum SpellID,std::allocator<enum SpellID>,0>(const std::deque<enum* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:236
DC_ONLY(0x42e68, 0x60)
void std::vector<army *,std::allocator<army *> >::vector<army *,std::allocator<army *> >(const std::vector<army* __x)
{
    // @stub
}

// ..\stlport\stl_deque.h:570
DC_ONLY(0x42ec8, 0x14)
std::_Deque_iterator<enum std::deque<enum SpellID,std::allocator<enum SpellID>,0>::begin(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_deque.h:571
DC_ONLY(0x42edc, 0x14)
std::_Deque_iterator<enum std::deque<enum SpellID,std::allocator<enum SpellID>,0>::end(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_deque.h:610
DC_ONLY(0x42ef0, 0x18)
unsigned std::deque<enum SpellID,std::allocator<enum SpellID>,0>::size()
{
    // @stub
}

// ..\stlport\stl_deque.h:452
DC_ONLY(0x42f08, 0x68)
void std::_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>::_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>(const std::allocator<enum* __a, unsigned __num_elements)
{
    // @stub
}

// ..\stlport\stl_deque.h:460
DC_ONLY(0x42f70, 0xA)
std::allocator<enum std::_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>::get_allocator(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_vector.h:153
DC_ONLY(0x42f7c, 0x8)
std::allocator<army std::vector<army *,std::allocator<army *> >::get_allocator(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_vector.h:94
DC_ONLY(0x42f84, 0x48)
void std::_Vector_base<army *,std::allocator<army *> >::_Vector_base<army *,std::allocator<army *> >(unsigned __n, const std::allocator<army* __a)
{
    // @stub
}

// ..\stlport\stl_deque.h:272
DC_ONLY(0x42fcc, 0x1C)
void std::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >()
{
    // @stub
}

// ..\stlport\stl_deque.h:283
DC_ONLY(0x42fe8, 0x18)
int std::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator-(const std::_Deque_iterator<enum* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x43000, 0xC)
void std::_STL_alloc_proxy<enum SpellID * *,enum SpellID *,std::allocator<enum SpellID> >::_STL_alloc_proxy<enum SpellID * *,enum SpellID *,std::allocator<enum SpellID> >(const std::allocator<enum* __a, SpellID*** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x4300c, 0xC)
void std::_STL_alloc_proxy<unsigned int,enum SpellID,std::allocator<enum SpellID> >::_STL_alloc_proxy<unsigned int,enum SpellID,std::allocator<enum SpellID> >(const std::allocator<enum* __a, const unsigned* __p)
{
    // @stub
}

// ..\stlport\stl_deque.h:190
DC_ONLY(0x43018, 0xE)
void std::_Deque_iterator_base<enum SpellID,std::_Buf_size_traits<enum SpellID,0> >::_Deque_iterator_base<enum SpellID,std::_Buf_size_traits<enum SpellID,0> >()
{
    // @stub
}

// ..\stlport\stl_deque.h:193
DC_ONLY(0x43028, 0x2A)
int std::_Deque_iterator_base<enum SpellID,std::_Buf_size_traits<enum SpellID,0> >::_M_subtract(const std::_Deque_iterator_base<enum* __x)
{
    // @stub
}

// ..\stlport\stl_deque.c:118
DC_ONLY(0x43054, 0xC4)
void std::_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>::_M_initialize_map(unsigned __num_elements)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x43118, 0x8C)
std::_Deque_iterator<enum std::uninitialized_copy(__$ReturnUdt, std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, std::_Deque_iterator<enum __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x431a4, 0x38)
army** std::uninitialized_copy(army** __first, army** __last, army** __result)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x431dc, 0x28)
SpellID** std::_STL_alloc_proxy<enum SpellID * *,enum SpellID *,std::allocator<enum SpellID> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x43204, 0x24)
SpellID** std::allocator<enum SpellID *>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_deque.c:144
DC_ONLY(0x43228, 0x3C)
void std::_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>::_M_create_nodes(SpellID** __nstart, SpellID** __nfinish)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x43264, 0x7C)
std::_Deque_iterator<enum std::__uninitialized_copy(__$ReturnUdt, std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, std::_Deque_iterator<enum __result, SpellID* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x432e0, 0x1C)
army** std::__uninitialized_copy(army** __first, army** __last, army** __result, army** __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x432fc, 0x28)
SpellID* std::_STL_alloc_proxy<unsigned int,enum SpellID,std::allocator<enum SpellID> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x43324, 0x24)
SpellID* std::allocator<enum SpellID>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x43348, 0x8C)
std::_Deque_iterator<enum std::__uninitialized_copy_aux(__$ReturnUdt, std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, std::_Deque_iterator<enum __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x433d4, 0x3C)
army** std::__uninitialized_copy_aux(army** __first, army** __last, army** __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_deque.h:276
DC_ONLY(0x43410, 0x4)
const SpellID* std::_Deque_iterator<enum SpellID,std::_Const_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator*()
{
    // @stub
}

// ..\stlport\stl_deque.h:285
DC_ONLY(0x43414, 0x1C)
std::_Deque_iterator<enum* std::_Deque_iterator<enum SpellID,std::_Const_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator++()
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x43430, 0x28)
void std::construct(SpellID* __p, const SpellID* __value)
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x0043cb10, 0xC, IMPLICIT_DTOR, TResourceHandle)

VA_COMPGEN(0x0043CB20, 0x2CF, DEQUE_PUSH_BACK, int)

VA_COMPGEN(0x0043cdf0, 0x6D, DEQUE_GROWMAP, int)
