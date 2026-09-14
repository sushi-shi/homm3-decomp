// ai.cpp - E:\gamedcs\ai.cpp (compiland ai.obj)
#include <va.h>
#include "includes.h"
#include <algorithm>
// army's +0xdc spell-charge word and army::is_adjacent(int) are canonical
// class facts: the Dreamcast member/function records name both, while
// choose_spell_action, choose_creature_spell and choose_defense_hex prove
// their retail uses.
#include "ai.h"
#include "ai_player.h"
#include "ai_spellvalue.h"
#include "ai_tactical.h"
#include "findpath.h"
#include "game.h"
#include "hero.h"
#include "misc.h"
#include "prefs.h"
#include "csprite.h"   // TResourceHandle<CSprite>::~TResourceHandle calls resource::Dispose
#include "sample.h"    // TResourceHandle<sample>::~TResourceHandle calls resource::Dispose
#include "soundmgr.h"

// THE HEAD OF ai.obj, 0x41e190..0x41eac0 (2026-09-05). The three rows
// between the compiland's ten terrain.h bitset initializers
// (0x41ddc0..0x41e18f, the excluded cinit class, with their atexit
// thunk at 0x41dda0) and the already-claimed get_total_combat_value
// (0x41eac0) are ai.cpp's own first three bodies, in DC roster order:
// ChooseBallistaTarget (dc 0x23450, 766 B), failed_siege (dc 0x23750,
// 332 B) and AICheckRetreat (dc 0x2389c, 1680 B) against retail's 680,
// 297 and 1350 - ratios 0.89/0.89/0.80 against get_total_combat_value's
// own 0.82. Three further proofs land on the same map: 0x41e570 calls
// 0x41e440 as a thiscall with no argument, which is exactly the
// failed_siege edge the DC xref census records inside AICheckRetreat;
// cmbtmgr.h already carried 0x41e570 = AICheckRetreat from command.cpp's
// call site alone; and the NH3API IDB, whose HD pressing runs a constant
// +0x180 ahead of retail through this whole region (init_bitset_41E190 =
// our 0x41e010, get_total_combat_value 0x41ec40 = our 0x41eac0), names
// 0x41e310/0x41e5c0/0x41e6f0 with these three mangled symbols in order.
// That also retires the `Unnamed41e190` ordinal: the IDB prototype
// carries the DC parameter names verbatim.

// E:\gamedcs\ai.cpp:43
// The arrow tower's / ballista's target picker. `attack_skill` is a DEAD
// parameter in retail - nothing reads [ebp+0xc] - and the DC prototype
// names it, so it is transcribed rather than dropped. The scan runs
// TWICE with the identical body: the first pass prices each stack with
// the estimate's own kills_only, and when that pass leaves no candidate
// (or none worth a positive score) the whole walk repeats with kills_only
// forced to 0. Both passes keep the running best - the retry does not
// reset best_value or result - and both re-read numArmies[target_group]
// through the back edge.

VA(0x0041e190, 0x2A8)  // order-map(DC ai.obj head) + anchor-callee find_AI_targets, dc 0x23450
int combatManager::chooseBallistaTarget(int targetGroup, int attackSkill, int averageDamage)
{
    double damage;
    long bestValue = 0;
    long result = -1;
    type_AI_combat_parameters estimate(this, 1 - targetGroup);

    findAITargets(targetGroup, 0, 0, &estimate, 0);

    { for (long i = 0; i < m_numArmies[targetGroup]; i++) {
            army* currentArmy = &m_armies[targetGroup][i];
            if (currentArmy->is(1u << 21))
                continue;
            damage = averageDamage;
            long value = static_cast<long>(
                damage * currentArmy->computeDefenderDamageReduction(1));
            value = currentArmy->getLossCombatValue(
                estimate.m_lowestAttack, estimate.m_lowestDefense, 1, value,
                estimate.m_killsOnly);
            if (!currentArmy->cannotAttack() && currentArmy->getAITarget()
                    && currentArmy->getAITargetTime() <= 5)
                value /= currentArmy->getAITargetTime();
            else
                value /= 5;
            if (value >= bestValue) {
                bestValue = value;
                result = i;
            }
        }
    }

    if (!estimate.m_killsOnly)
        return result;
    if (result >= 0 && bestValue > 0)
        return result;

    { { for (long i = 0; i < m_numArmies[targetGroup]; i++) {
                army* currentArmy = &m_armies[targetGroup][i];
                if (currentArmy->is(1u << 21))
                    continue;
                damage = averageDamage;
                long value = static_cast<long>(
                    damage * currentArmy->computeDefenderDamageReduction(1));
                value = currentArmy->getLossCombatValue(
                    estimate.m_lowestAttack, estimate.m_lowestDefense, 1, value, 0);
                if (!currentArmy->cannotAttack() && currentArmy->getAITarget()
                        && currentArmy->getAITargetTime() <= 5)
                    value /= currentArmy->getAITargetTime();
                else
                    value /= 5;
                if (value >= bestValue) {
                    bestValue = value;
                    result = i;
                }
            }
        }
    }
    return result;
}

VA(0x0041e440, 0x129)  // dc 0x23750
unsigned char combatManager::failedSiege()
{
    DATA(0x0063abc0) static const TWallTargetId walls[4] = {
        WALL_TARGET_1, WALL_TARGET_2, WALL_TARGET_4, WALL_TARGET_5
    };

    army* currentArmy = m_armies[m_currentSide];
    if (m_fortificationLevel == COMBAT_FORTIFICATION_NONE)
        return 0;
    if (m_drawbridgeState != DRAWBRIDGE_UP)
        return 0;
    if (m_currentSide == 1)
        return 0;

    { for (long i = 0; i < m_numArmies[m_currentSide]; i++, currentArmy++) {
            unsigned attributes = currentArmy->m_monInfo.m_attributes;
            unsigned char dead = static_cast<unsigned char>(attributes >> 21);
            if ((dead & 1) != 0)
                continue;
            if ((attributes & ((1u << 1) | (1u << 5))) != 0)
                return 0;
            if (currentArmy->canShoot(0))
                return 0;
        }
    }

    { for (long i = 0; i < 4; i++) {
            if (!getWallStrength(walls[i]))
                return 0;
        }
    }

    army* enemyArmy = m_armies[1 - m_currentSide];
    { for (long i = 0; i < m_numArmies[1 - m_currentSide]; i++, enemyArmy++) {
            unsigned char dead = static_cast<unsigned char>(
                static_cast<unsigned>(enemyArmy->m_monInfo.m_attributes) >> 21);
            if ((dead & 1) != 0)
                continue;
            if (enemyArmy->m_creatureType == CREATURE_ARROW_TOWER)
                continue;
            if (!inCastle(enemyArmy->m_gridIndex))
                return 0;
        }
    }
    return 1;
}

// E:\gamedcs\ai.cpp:162
// The whole retreat decision, and the DC local roster names four of its
// variables verbatim (surrender_cost, combat_value, iSideFV, artifact).
// Five gates, then a value model:
//   - a non-AI side is gated on the scenario difficulty, and difficulty 1
//     only retreats on a coin flip;
//   - either hero wielding artifact 0x7d (Shackles of War) forbids it, and
//     so does being the target of a defeat-hero victory condition;
//   - a retreating hero needs a Tavern to arrive in, so the owner's town
//     roster is censused for one; if the only such town IS the town under
//     siege and we are the defender, there is nowhere to go;
//   - a defender in a siege additionally needs a Stronghold's Escape
//     Tunnel (town type 6, SPECIAL_BUILDING_ID);
//   - failed_siege answers yes outright.
// The value model prices the hero's equipped and backpack artifacts at
// max(AI value, half the traits cost), refuses to retreat a poor and
// inexperienced hero, retreats unconditionally when no stack of ours is
// still standing, refuses when the treasury cannot cover the surrender
// price plus 2500, and finally compares our side's share of the total
// fight value against a difficulty- and experience-adjusted threshold.

// The guards are written as retail wrote them - ONE `return 0` at the
// bottom of a nested-if pyramid. Written as thirteen early returns the
// body scores 60.44 with seventeen epilogues against retail's four; the
// pyramid alone is +24.22 and makes the branch census exact (54/54
// branches, 4/4 rets).

// Levers that paid, in order: the nested-if pyramid (60.44 -> 84.66);
// naming the AI_get_artifact_player_value result so the call is
// evaluated BEFORE the traits-cost operand of max - VC6 evaluates
// by-value arguments right to left, and with the call first the
// half-cost dies before it, which frees EDI for the loop
// index and lets combat_value live in EBX (84.66 -> 94.82); naming the
// attribute word so the flag test is `test ecx, 0x4000000` and not
// Is()'s shr/test pair, plus naming the final quotient so the division
// result round-trips through its own float slot (94.82 -> 95.33); and
// subscripting the fight-value walk instead of walking a named pointer
// (95.33 -> 95.39).
VA(0x0041e570, 0x546)  // order-map(DC ai.obj head) + anchor-callee failed_siege, dc 0x2389c
unsigned char combatManager::aiCheckRetreat()
{
    if (m_heroes[m_currentSide]
        && (m_sideIsAi[m_currentSide]
            || (g_game->m_setup.m_difficulty
                && (g_game->m_setup.m_difficulty != 1 || random(1, 100) > 50)))
        && (!m_heroes[0] || !m_heroes[0]->isWieldingArtifact(0x7d))
        && (!m_heroes[1] || !m_heroes[1]->isWieldingArtifact(0x7d))
        && (g_game->m_mapHeader.m_victoryCondition.m_type != VICTORY_CONDITION_DEFEAT_HERO
            || g_game->m_mapHeader.m_victoryCondition.m_heroId
               != m_heroes[m_currentSide]->m_id)) {
        long sideFV = m_currentSide;
        long count = 0;
        unsigned char besiegedTownOnly = 0;
        long i = 0;
        playerData* player = &g_game->m_players[m_heroes[m_currentSide]->m_owner];
        long numTowns = player->m_numTowns;
        if (numTowns > 0) {
            { for (; i < numTowns; i++) {
                    town* currentTown = g_game->getTown(player->m_townIds[i]);
                    if (currentTown->hasBuilding(TAVERN_ID, 1)) {
                        count++;
                        if (m_defendingTown == currentTown)
                            besiegedTownOnly = 1;
                    }
                }
            }
            if (count
                && (count != 1 || sideFV != 1 || !besiegedTownOnly)
                && (!m_defendingTown || sideFV != 1
                    || (m_defendingTown->m_type == TOWN_STRONGHOLD
                        && m_defendingTown->hasBuilding(SPECIAL_BUILDING_ID, 1)))) {
                if (failedSiege())
                    return 1;

                long combatValue = 0;
                type_artifact artifact;
                { for (long i = 0; i < 19; i++) {
                        artifact = m_heroes[m_currentSide]->getArtifact(TArtifactSlot(i));
                        if (artifact.m_artifactId == ARTIFACT_NONE)
                            continue;
                        long artifactValue = aiGetValueOfArtifact(
                            artifact, m_playerIds[m_currentSide]);
                        combatValue += max(
                            artifactValue,
                            static_cast<long>(
                                g_artifactTraits[artifact.m_artifactId].m_cost / 2));
                    }
                }
                { for (long i = 0; i < 64; i++) {
                        artifact = m_heroes[m_currentSide]->getBackpack(i);
                        if (artifact.m_artifactId == ARTIFACT_NONE)
                            continue;
                        long artifactValue = aiGetValueOfArtifact(
                            artifact, m_playerIds[m_currentSide]);
                        combatValue += max(
                            artifactValue,
                            static_cast<long>(
                                g_artifactTraits[artifact.m_artifactId].m_cost / 2));
                    }
                }
                if (combatValue >= 1000
                    || m_heroes[m_currentSide]->m_experience >= 2000) {
                    long surrenderCost = getSurrenderCost();
                    simulateCombat(m_currentSide, 1);

                    long remaining = m_numArmies[m_currentSide];
                    army* currentArmy = m_armies[m_currentSide];
                    while (remaining-- > 0) {
                        if (!(currentArmy->is(1u << 21))
                            && !(currentArmy->is(1u << 6))
                            && currentArmy->getTotalHitPoints(1) > 0)
                            break;
                        currentArmy++;
                    }
                    if (remaining < 0)
                        return 1;
                    if (player->m_resources[GOLD] >= surrenderCost + 2500) {
                        long fightValues[2];
                        { for (long side = 0; side < 2; side++) {
                                long fightValue = 0;
                                { for (long i = 0; i < 20; i++) {
                                        army* sideArmy = &m_armies[side][i];
                                        if (sideArmy->m_creatureType < 0)
                                            continue;
                                        if (sideArmy->m_numTroops <= 0)
                                            continue;
                                        long value =
                                            sideArmy->m_numTroops
                                            * sideArmy->m_monInfo.m_baseFightValue;
                                        unsigned attributes =
                                            sideArmy->m_monInfo.m_attributes;
                                        if (!(attributes & (1u << 26)))
                                            value = static_cast<long>(value * 1.2);
                                        fightValue += value;
                                    }
                                }
                                fightValues[side] = fightValue;
                                if (m_defendingTown && side == 1)
                                    fightValues[1] =
                                        static_cast<long>(fightValue * 1.1);
                            }
                        }
                        fightValues[1 - m_currentSide] = static_cast<long>(
                            fightValues[1 - m_currentSide] * 1.1);

                        float threshold = 0.16f;
                        if (combatValue > 10000)
                            threshold = 0.22f;
                        else if (combatValue > 5000)
                            threshold = 0.21f;
                        // The third arm is an INCREMENT, not a literal:
                        // retail stores 0x3e4ccccc where `threshold = 0.2f`
                        // gives 0x3e4ccccd.  VC6 constant-propagates the
                        // 0.16f initialiser and folds `0.16f + 0.04f` in
                        // SINGLE precision, which lands one ulp below the
                        // correctly-rounded decimal.  95.3935 -> 95.3960.
                        else if (combatValue > 0)
                            threshold += 0.04f;
                        threshold -= (4 - g_game->m_setup.m_difficulty) * 0.015;
                        float experienceBonus =
                            m_heroes[m_currentSide]->m_experience / 200000;
                        if (experienceBonus > 0.03)
                            experienceBonus = 0.03f;
                        threshold += experienceBonus;
                        if (!m_currentSide)
                            threshold -= 0.06f;
                        if (threshold > 0.16)
                            threshold = 0.16f;
                        float ratio =
                            static_cast<float>(fightValues[m_currentSide])
                            / static_cast<float>(fightValues[0]
                                                 + fightValues[1]);
                        if (ratio < threshold)
                            return 1;
                    }
                }
            }
        }
    }
    return 0;
}

VA(0x0041eac0, 0xB8)  // dc 0x23f2c
long combatManager::getTotalCombatValue(long side, long lowestAttack, long lowestDefense, unsigned char includeCripples) const
{
    long total = 0;
    const army* currentArmy = m_armies[side];
    for (long i = 0; i < m_numArmies[side]; i++, currentArmy++) {
        unsigned char dead = static_cast<unsigned char>(
            static_cast<unsigned>(currentArmy->m_monInfo.m_attributes) >> 21);
        if ((dead & 1) != 0 || currentArmy->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (!includeCripples) {
            if (currentArmy->m_spellInfluence[62] || currentArmy->m_spellInfluence[70]
                    || currentArmy->m_spellInfluence[74])
                continue;
            if (currentArmy->m_creatureType == CREATURE_FIRST_AID_TENT
                    || currentArmy->m_creatureType == CREATURE_AMMO_CART)
                continue;
        }
        total += currentArmy->getTotalCombatValue(lowestAttack, lowestDefense);
    }
    return total;
}

VA(0x0041eb80, 0x220)  // dc 0x240e4
long combatManager::chooseShooterTarget(const army* currentArmy, type_AI_combat_parameters* data, long* bestValue) const
{
    long bestTarget = -1;
    long hex;
    long ourGroup = data->m_ourGroup;
    long enemyGroup = data->m_enemyGroup;
    unsigned char isAreaEffect = 0;
    const army* bestArmy = 0;
    std::vector<army*> targets;

    if (currentArmy->m_creatureType == CREATURE_MAGOG
            || currentArmy->m_creatureType == CREATURE_LICH
            || currentArmy->m_creatureType == CREATURE_POWER_LICH)
        isAreaEffect = 1;

    for (long i = 0; i < m_numArmies[enemyGroup]; i++) {
        const army* target = &m_armies[enemyGroup][i];
        unsigned char dead = static_cast<unsigned char>(
            static_cast<unsigned>(target->m_monInfo.m_attributes) >> 21);
        if ((dead & 1) != 0 || target->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (data->m_simulated && target->getTotalHitPoints(1) == 0)
            continue;

        hex = target->m_gridIndex;
        long value;
        if (isAreaEffect) {
            value = getAreaAttackValue(currentArmy, hex, ourGroup, data);
            if (target->m_monInfo.m_attributes & 1) {
                long secondValue = getAreaAttackValue(
                    currentArmy, target->getSecondGridIndex(), ourGroup,
                    data);
                if (secondValue > value) {
                    hex = target->getSecondGridIndex();
                    value = secondValue;
                }
            }
            if (value < 0)
                continue;
        } else {
            value = data->getRangedAttackValue(*(currentArmy), *(target));
        }

        if (bestArmy) {
            if (target->isIncapacitated()
                    && !bestArmy->isIncapacitated())
                continue;
            if ((target->isIncapacitated() || !bestArmy->isIncapacitated())
                    && value < *bestValue)
                continue;
        }
        bestArmy = target;
        *bestValue = value;
        bestTarget = hex;
    }
    return bestTarget;
}

// THE TWO-HEX SKIP is a four-term conjunction and its polarity is the
// whole shape: an area effect is only DOUBLE-counted against a wide
// stack when the centre hex is neither of the stack's two hexes, so
// the body drops such a target and prices every other one.

VA(0x0041eda0, 0xFD)  // dc 0x2400c
long getAreaAttackValue(const army* currentArmy, long hex, long ourGroup, type_AI_combat_parameters* data)
{
    std::vector<army*> targets;
    long total = 0;
    g_combatManager->markHexAreaEffect(hex, 1, 1, targets);
    for (unsigned i = targets.size(); i-- != 0; ) {
        army* target = targets[i];
        if ((currentArmy->is(1u << 18)) && (target->is(1u << 18))
                && target->m_gridIndex != hex
                && target->getSecondGridIndex() != hex)
            continue;
        if (target->m_combatSide == ourGroup)
            total -= data->getSimpleAttackEffect(*(currentArmy), *(target), 1, 0);
        else
            total += data->getRangedAttackValue(*(currentArmy), *(target));
    }
    return total;
}

VA(0x0041eea0, 0x1B9)  // dc 0x2429c
unsigned char combatManager::chooseCyclopsAction(long bestValue, long side, type_AI_combat_parameters* estimate)
{
    DATA(0x0063abd0) static const TWallTargetId walls[4] = {
        WALL_TARGET_1, WALL_TARGET_2, WALL_TARGET_4, WALL_TARGET_5
    };

    if (side == 1)
        return 0;
    if (m_fortificationLevel == COMBAT_FORTIFICATION_NONE)
        return 0;

    long count = 0;
    { for (long i = 0; i < 4; i++) {
            if (getWallStrength(walls[i]) > 0)
                count++;
        }
    }
    if (!count)
        return 0;

    findAITargets(side, 0, 0, estimate, 0);
    count = 0;
    { for (long i = 0; i < m_numArmies[side]; i++) {
            army* currentArmy = &m_armies[side][i];
            if (!currentArmy->getAITarget())
                count += currentArmy->getTotalCombatValue(
                    estimate->m_lowestAttack, estimate->m_lowestDefense);
        }
    }

    if (static_cast<double>(count) / estimate->m_friendlyCombatValue
            <= static_cast<double>(bestValue) * 1.2 / estimate->m_enemyCombatValue)
        return 0;

    long weakest = 100;
    count = 0;
    { for (long i = 0; i < 4; i++) {
            long strength = getWallStrength(walls[i]);
            if (strength <= 0 || strength > weakest)
                continue;
            if (strength < weakest)
                count = 0;
            count++;
            weakest = strength;
        }
    }

    long choice = sRandom(1, count);
    long target = 0;
    for (; target < 4; target++) {
        long strength = getWallStrength(walls[target]);
        if (strength == weakest && --choice == 0)
            break;
    }
    m_nextAction = 9;
    m_nextActionGridIndex = s_wallTargets[walls[target]].m_targetHex;
    m_nextActionExtra = -1;
    return 1;
}

VA(0x0041f060, 0xD1)  // dc 0x2452c
void combatManager::chooseShooterAction(const army* currentArmy, unsigned char simulated, long side)
{
    long bestValue = 0;
    type_AI_combat_parameters data(this, side);
    data.m_simulated = simulated;
    findAITargets(1 - side, 0, 0, &data, 0);
    long actionValue = chooseShooterTarget(currentArmy, &data, &bestValue);
    if (!simulated && (currentArmy->is(1u << 5))
            && chooseCyclopsAction(bestValue, side, &data))
        return;
    if (actionValue < 0) {
        m_nextAction = 3;
        return;
    }
    if (data.m_killsOnly && !bestValue) {
        data.m_killsOnly = 0;
        actionValue = chooseShooterTarget(currentArmy, &data, &bestValue);
    }
    m_nextAction = 7;
    m_nextActionGridIndex = actionValue;
}

// E:\gamedcs\ai.cpp:597 - combatManager::find_move_order's std::sort
// predicate. A DC roster row (func_moves_before::operator(), 0x28024,
// three parameters = this + two) proves it is an empty FUNCTOR class,
// not a free function: the retail sort call pushes a garbage 4-byte
// slot for the by-value temp - the same uninitialised-empty-class
// coalescing std::vector's allocator subobject shows - where a function
// pointer would have taken a reloc. Retail expands it in the sort helpers
// and also retains the ordinary body at 0x4235c0, defined in RVA order
// below. The expansion in _Insertion_sort at 0x4233c9 reads it as
//   x->field_190 > y->field_190
//     || (x->field_190 == y->field_190 && x->bitIndex < y->bitIndex)
// - descending on the move key, ascending on the stack index so equal
// keys stay in slot order.
struct func_moves_before {
    unsigned char operator()(const army* a, const army* b);
};

// E:\gamedcs\ai.cpp:610
// STATIC with a single call site, so /Ob2 inlines it unconditionally
// and retail emits no out-of-line body (the DC's 0x24604 row has no
// retail slot). Written as its own function because that is what makes
// the five arms converge on ONE `mov [army+0x190], eax` at 0x41f2f7 -
// an inlined callee's single return point - instead of five stores.
// The `> 1` on the two disabled counters is byte-proven: retail
// materialises the constant 1 in EAX (`mov eax, 1`), compares both
// counters against it with `jg`, and then REUSES that same AL as the
// mask for the `test al, cl` bit test below.
static long getMoveOrder(const army* currentArmy)
{
    if (currentArmy->m_creatureType == CREATURE_FIRST_AID_TENT
            || currentArmy->m_creatureType == CREATURE_AMMO_CART)
        return -100000;
    if (currentArmy->m_spellInfluence[62] > 1 || currentArmy->m_spellInfluence[70] > 1)
        return -10000;
    unsigned char waited = static_cast<unsigned char>(
        static_cast<unsigned>(currentArmy->m_monInfo.m_attributes) >> 26);
    if ((waited & 1) != 0
            || const_cast<army*>(currentArmy)->isIncapacitated())
        return currentArmy->getSpeed() - 1000;
    unsigned char reversed = static_cast<unsigned char>(
        static_cast<unsigned>(currentArmy->m_monInfo.m_attributes) >> 25);
    if ((reversed & 1) != 0 || g_combatManager->m_inSecondPhase)
        return -currentArmy->getSpeed();
    return currentArmy->getSpeed();
}

VA(0x0041f140, 0x23F)  // dc 0x24694
void combatManager::findMoveOrder(std::vector<army*>* result)
{
    std::vector<army*> order;
    for (long side = 0; side < 2; side++) {
        for (long i = 0; i < m_numArmies[side]; i++) {
            army* currentArmy = &m_armies[side][i];
            if (currentArmy->m_creatureType == CREATURE_ARROW_TOWER)
                continue;
            unsigned char dead = static_cast<unsigned char>(
                static_cast<unsigned>(currentArmy->m_monInfo.m_attributes) >> 21);
            if ((dead & 1) != 0)
                continue;
            currentArmy->m_expectedMoveOrder = getMoveOrder(currentArmy);
            order.push_back(currentArmy);
        }
    }
    std::sort(order.begin(), order.end(), func_moves_before());
    long wantSide = m_actingSide;
    for (unsigned i = 0; i < order.size(); i++) {
        if (order[i]->m_combatSide != wantSide) {
            long key = order[i]->m_expectedMoveOrder;
            for (unsigned j = i + 1; j < order.size(); j++) {
                if (order[j]->m_expectedMoveOrder != key)
                    break;
                if (order[j]->m_combatSide == wantSide) {
                    std::swap(order[i], order[j]);
                    break;
                }
            }
        }
        wantSide = 1 - order[i]->m_combatSide;
    }
    for (unsigned k = 0; k < order.size(); k++) {
        order[k]->m_expectedMoveOrder = order.size() - k;
        if (result)
            result->push_back(order[k]);
    }
}

// E:\gamedcs\ai.cpp:696, original get_attack_value.
// DC 0x248b4 proves this ordinary file-static helper, its data reference,
// named double combat_value, and separate integer-return branches. Retail's
// expansion belongs to getAttackChange below; this helper has no retained
// standalone retail row. The neighboring 0x41f380 is IsIncapacitated.
DC_ONLY(0x248b4, 0x180)
static long getAttackValue(const army* currentArmy, const army* enemy,
                           long enemyHitPoints, type_AI_combat_parameters& data)
{
    if (enemyHitPoints <= 0)
        return 0;
    long currentHits = currentArmy->getTotalHitPoints(data.m_simulated);
    long damage = aiGetAttackDamage(*currentArmy, currentHits, *enemy, 0, 0);
    if (damage > enemyHitPoints)
        damage = enemyHitPoints;
    double combatValue = enemy->getUnitCombatValue(data.m_lowestAttack,
                                                   data.m_lowestDefense, 0, 0);
    if (data.m_killsOnly) {
        return ((enemyHitPoints % enemy->m_monInfo.m_hitPoints + damage)
                / enemy->m_monInfo.m_hitPoints) * combatValue;
    }
    return damage * combatValue / enemy->m_monInfo.m_hitPoints;
}

VA(0x0041f3b0, 0x1C2)  // dc 0x24a34
long combatManager::getAttackChange(const army* currentArmy, const army* enemy, type_AI_combat_parameters& data)
{
    if (enemy->getSpellTime(70) || enemy->m_retaliationCount == 0)
        return 0;
    if (enemy->m_retaliationCount > 1)
        return 0;
    long enemyHits = enemy->getTotalHitPoints(data.m_simulated);
    long ourHits = currentArmy->getTotalHitPoints(data.m_simulated);
    long committed = 0;
    long bestOther = 0;
    enemyHits -= aiGetAttackDamage(*currentArmy, ourHits, *enemy, 0, 0);
    if (enemyHits <= 0)
        return 0;
    const army* friendly = m_armies[m_currentSide];
    for (long i = 0; i < m_numArmies[m_currentSide]; i++, friendly++) {
        if (friendly->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if ((friendly->getAIPossibleTargets() & (1 << enemy->m_bitIndex)) == 0)
            continue;
        long change = getAttackValue(friendly, enemy, enemyHits, data);
        change -= friendly->getAITargetValue();
        if (friendly->getAITarget() == enemy && change > 0)
            committed += change;
        else if (change > bestOther)
            bestOther = change;
    }
    return committed + bestOther;
}

VA(0x0041f580, 0x304)  // dc 0x24b64
unsigned char combatManager::moveToward(const army* currentArmy, long targetHex, const long* enemyAttacks, unsigned char considerWaiting)
{
    if (!currentArmy->m_spellInfluence[72] && currentArmy->getSpeed()) {
        g_searchArray->findCombatPath(currentArmy, m_currentSide, targetHex,
                                      m_creaturePlacement, 0x7f, -1);
        if (static_cast<long>(g_searchArray->getPathSteps()) > 0) {
            m_nextAction = 2;
            long moveLeft = currentArmy->getSpeed();
            long pathIndex = g_searchArray->getPathSteps() - 1;
            long step;
            long startDanger;
            long limit;
            long bestDanger;
            long hex = currentArmy->m_gridIndex;
            unsigned char committed;

            bestDanger = 0;
            startDanger = 0;
            m_nextActionGridIndex = hex;
            if (m_creaturePlacement)
                moveLeft = pathIndex + 1;
            if (m_creaturePlacement || m_inSecondPhase)
                considerWaiting = 0;
            if (g_game->m_setup.m_difficulty < 2 && !m_sideIsAi[currentArmy->m_combatSide])
                considerWaiting = 0;
            if (enemyAttacks == 0) {
                considerWaiting = 0;
            } else {
                bestDanger = enemyAttacks[hex];
                if (currentArmy->m_monInfo.m_attributes & 1)
                    bestDanger = min(
                            enemyAttacks[hex
                                    + (currentArmy->m_facing != 0 ? 1 : -1)],
                            bestDanger);
                startDanger = bestDanger;
            }

            committed = 0;
            limit = (pathIndex / moveLeft) * moveLeft;
            if (pathIndex >= 0) {
                for (step = pathIndex + 1; pathIndex >= 0;
                        pathIndex--, step--, moveLeft--) {
                    if (moveLeft <= 0)
                        break;
                    hex = const_cast<army*>(currentArmy)->getAdjacentCellIndex(
                            hex, g_searchArray->getStep(pathIndex));
                    if (hex < 0 || hex >= 187)
                        break;
                    const pathCell* cell = g_searchArray->getHex(hex);
                    if (cell->m_flightCost == 0) {
                        long secondHex = (currentArmy->m_monInfo.m_attributes & 1)
                                ? hex + (currentArmy->m_facing != 0 ? 1 : -1) : hex;
                        if (!(((step <= limit && committed) || considerWaiting)
                                    && enemyAttacks != 0)
                                || (enemyAttacks[hex] >= bestDanger
                                    && enemyAttacks[secondHex] >= bestDanger)) {
                            if (!m_creaturePlacement
                                    || !isOutsidePlacementBoundry(
                                            currentArmy->m_combatSide, hex)) {
                                m_nextActionGridIndex = hex;
                                committed = 1;
                                if (enemyAttacks != 0) {
                                    bestDanger = enemyAttacks[hex];
                                    if (currentArmy->m_monInfo.m_attributes & 1)
                                        bestDanger = min(
                                                enemyAttacks[secondHex],
                                                bestDanger);
                                }
                            }
                        }
                        if ((static_cast<unsigned char>(static_cast<unsigned>(
                                        currentArmy->m_monInfo.m_attributes) >> 1) & 1) == 0) {
                            if (g_searchArray->isMoat(
                                        static_cast<short>(hex))
                                    || g_searchArray->isMoat(
                                            static_cast<short>(secondHex)))
                                moveLeft = 0;
                        }
                    }
                }
            }

            if (considerWaiting && m_nextActionGridIndex != hex
                    && bestDanger >= startDanger)
                m_nextAction = 8;
            if (m_nextActionGridIndex == currentArmy->m_gridIndex)
                m_nextAction = considerWaiting ? 8 : 3;
            return 1;
        }
    }
    return 0;
}

VA(0x0041f890, 0x8F)  // dc 0x24e5c
unsigned char combatManager::canCastSpells(long side, unsigned char heroSpell) const
{
    if (!heroSpell && m_magicTerrain == COMBAT_SPELL_RESTRICTION_NO_CREATURE_SPELLS)
        return 0;
    if (m_onAntiMagicGarrison)
        return 0;
    if (heroSpell) {
        if (m_heroes[side] == 0
                || !m_heroes[side]->isWieldingArtifact(ARTIFACT_SPELLBOOK))
            return 0;
    }
    if (m_heroes[0] != 0 && m_heroes[0]->isWieldingArtifact(ARTIFACT_ORB_OF_INHIBITION))
        return 0;
    if (m_heroes[1] != 0 && m_heroes[1]->isWieldingArtifact(ARTIFACT_ORB_OF_INHIBITION))
        return 0;
    return 1;
}

VA(0x0041f920, 0x234)  // dc 0x24ef4
long combatManager::getAreaEffect(long side, const army* ourArmy, long markedEnemies, const type_AI_combat_parameters* estimate) const
{
    long total = 0;
    const army* enemy = m_armies[side];
    for (long i = 0; i < m_numArmies[side]; i++, enemy++) {
        if ((markedEnemies & (1 << enemy->m_bitIndex)) == 0)
            continue;
        if ((enemy->is(1u << 20)) && enemy->canShoot(0))
            total += enemy->getAverageDamage(ourArmy, 1, enemy->m_numTroops,
                                               1, 0);
        if (enemy->is(1u << 3))
            total += enemy->getAverageDamage(ourArmy, 0, enemy->m_numTroops,
                                               1, 0);
    }
    if (canCastSpells(side, 1)) {
        hero* castingHero = m_heroes[side];
        long best = 0;
        for (SpellID spell = 10; spell < hero::NUM_SPELLS; spell++) {
            if (!castingHero->spellIsAvailable(spell))
                continue;
            if (spell == SPELL_FROST_RING || spell == SPELL_FIREBALL
                    || spell == SPELL_INFERNO
                    || spell == SPELL_METEOR_SHOWER) {
                TSkillMastery mastery =
                    castingHero->getSpellLevel(spell, m_magicTerrain);
                if (castingHero->getManaCost(spell, m_armyGroups[1 - side],
                                              m_magicTerrain)
                        > castingHero->m_mana)
                    continue;
                long damage = computeSpellDamage(spell, m_spellPower[side],
                                                 mastery, castingHero,
                                                 m_heroes[1 - side], ourArmy,
                                                 0);
                long value = static_cast<long>(
                        spellCastWorkChance(spell, side, ourArmy, 0, 1, 0)
                        * damage);
                if (value > best)
                    best = value;
            }
        }
        total += best;
    }
    if (total <= 0)
        return total;
    return ourArmy->getLossCombatValue(estimate->m_lowestAttack,
                                           estimate->m_lowestDefense,
                                           ourArmy->canShoot(0), total, 0);
}

#if 0  // @carcass

// E:\gamedcs\ai.cpp:1000
// NO RETAIL SLOT - a two-parameter static with one call site, folded
// into whichever of get_area_effect / mark_friendly_armies uses it.
DC_ONLY(0x250e0, 0x42)
// Before normalization (function): get_enemy_attack_limit.
long getEnemyAttackLimit(const army* our_army, const type_AI_combat_parameters* estimate)
{
    // @stub
}

#endif  // @carcass

VA(0x0041fb60, 0x1F6)  // dc 0x25124
void combatManager::markFriendlyArmies(const army* ourArmy, long* enemyAttacks, long markedEnemies, const type_AI_combat_parameters* estimate) const
{
    long enemySide = estimate->m_enemyGroup;
    long areaEffect = getAreaEffect(enemySide, ourArmy, markedEnemies,
                                       estimate);
    unsigned char checked[COMBAT_GRID_CELLS];
    memset(checked, 0, COMBAT_GRID_CELLS);
    long hitPoints = ourArmy->getTotalHitPoints(estimate->m_simulated);
    long floorValue = -ourArmy->getLossCombatValue(
            estimate->m_lowestAttack, estimate->m_lowestDefense,
            ourArmy->canShoot(0), hitPoints, 0);
    const army* friendly = m_armies[estimate->m_ourGroup];
    for (long i = 0; i < m_numArmies[estimate->m_ourGroup]; i++, friendly++) {
        if (friendly->is(1u << 21))
            continue;
        if (friendly->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (friendly == ourArmy)
            continue;
        long meleeValue;
        if (friendly->is(1u << 3)) {
            long damage = friendly->getAverageDamage(ourArmy, 0,
                                                       friendly->m_numTroops,
                                                       1, 0);
            meleeValue = ourArmy->getLossCombatValue(
                    estimate->m_lowestAttack, estimate->m_lowestDefense,
                    ourArmy->canShoot(0), damage, 0);
        } else {
            meleeValue = 0;
        }
        if (areaEffect == 0 && meleeValue == 0)
            continue;
        long count = (friendly->is(1u << 0)) ? 8 : 6;
        for (long direction = count; direction-- > 0; ) {
            long hex = friendly->getAdjacentHex(friendly->m_gridIndex,
                                                  direction);
            if (hex < 0 || hex >= COMBAT_GRID_CELLS)
                continue;
            if (areaEffect != 0) {
                if (checked[hex]) {
                    enemyAttacks[hex]--;
                } else {
                    checked[hex] = 1;
                    enemyAttacks[hex] -= areaEffect;
                }
                if (enemyAttacks[hex] < floorValue)
                    enemyAttacks[hex] = floorValue;
            }
            if (friendly->is(1u << 3)) {
                long farHex = friendly->getAdjacentCellIndex(hex, direction);
                if (farHex < 0 || farHex >= COMBAT_GRID_CELLS)
                    continue;
                enemyAttacks[farHex] -= meleeValue;
                if (enemyAttacks[farHex] < floorValue)
                    enemyAttacks[farHex] = floorValue;
            }
        }
    }
}

void findAttackHexes(const army* ourArmy, long targetHex, long start,
                       long stop, long limitCost,
                       const searchArray* currentSearchArray,
                       std::vector<long>* result);

// E:\gamedcs\ai.cpp:1121
// NO RETAIL SLOT, and REQUIRED anyway: a `static` with a single call
// site is the /Ob2 case that leaves no out-of-line copy, so its absence
// from the image is what proves the internal linkage rather than what
// argues against the function existing.

// It is also the whole reason mark_multiheaded_enemy below reproduces:
// retail keeps its three seven-parameter find_attack_hexes calls
// OUT-OF-LINE, which a caller with a ~1000-plus budget would never do.
// Routing them through this wrapper makes them DEPTH-2 expansions, and
// the RE'd rule hands a depth-2 site only `remaining budget / sites
// still ahead` (docs/vc6/inliner.md §2) - which starves every one of
// them exactly as retail's bytes show. Spelling the three calls inline
// in the caller instead inlines all three and costs 52 points.

// The body is READ OFF mark_multiheaded_enemy's retail expansion: the
// side count comes from OUR stack's two-hex bit, the extra sweep from
// the ENEMY's, and the enemy's facing picks both the tail hex and which
// half of the direction ring is searched.
DC_ONLY(0x253a8, 0xA4)
static void findAttackHexes(const army* ourArmy, const army* enemy, const searchArray* currentSearchArray, std::vector<long>* result)
{
    long sides = (ourArmy->is(1u << 0)) ? 8 : 6;
    findAttackHexes(ourArmy, ourArmy->m_gridIndex, 0, sides,
                      enemy->getSpeed(), currentSearchArray, result);
    if (enemy->m_monInfo.m_attributes & 1) {
        long secondHex = ourArmy->m_gridIndex - (enemy->m_facing ? 1 : -1);
        if (enemy->m_facing == 0)
            findAttackHexes(ourArmy, secondHex, 0, 3, enemy->getSpeed(),
                              currentSearchArray, result);
        else
            findAttackHexes(ourArmy, secondHex, 3, 6, enemy->getSpeed(),
                              currentSearchArray, result);
    }
}

// E:\gamedcs\ai.cpp:1152
// `ret 0x18` = six stack arguments over `this`, the DC count exactly,
// and the body calls army::get_multi_head_directions - the one callee
// in the whole TU that names this function.

// EH-bearing (P2.2) and NOT blocked by it: the fs:[0] frame is the local
// std::vector<long>'s unwind scaffolding, one state, entered at the
// vector's construction and never left.

// TWO parallel 187-byte marks: `priced` spans the whole call and decides
// whether a hex is charged the full breath value or just one more point,
// and `counted` is re-zeroed per defending stack so one stack cannot pay
// for the same hex twice.

// `hexes.clear()` is LOAD-BEARING and not a tidier spelling of
// `erase(begin(), end())`: written out longhand the erase body lands at
// depth 1 and takes so much budget that the whole shape collapses
// (67.13% against 94.54%). clear() is itself free (<=40) and pushes
// erase to depth 2, which is also where the three find_attack_hexes
// expansions have to be.

// Residual (94.5%): ONE inline decision - retail leaves
// std::vector<long>::erase an out-of-line CALL (0x54cdb0) where our CL
// still has budget for it, so our copy of the clear() carries the
// Dinkumware copy loop inline. By the RE'd rule that puts retail's
// caller_cb at or under the 500 that clamps the budget to the 1000
// floor and ours just over it; no statement in this body is spare
// enough to give back. Everything downstream of the erase - the whole
// direction walk, both marks and the danger arithmetic - is
// instruction-for-instruction identical apart from one EDX/ECX tie on
// the `facing ? 1 : -1` temp.
// E:\gamedcs\ai.cpp:1152
VA(0x0041fd60, 0x2F6)  // anchor-callee, dc 0x2544c
void combatManager::markMultiheadedEnemy(const army* ourArmy, const army* enemy, long* enemyAttacks, long limitValue, searchArray* currentSearchArray, type_AI_combat_parameters* estimate) const
{
    const army* other = m_armies[estimate->m_ourGroup];
    long value = -estimate->getSimpleAttackEffect(*(enemy), *(ourArmy), 0, 0);
    std::vector<long> hexes;
    unsigned char priced[COMBAT_GRID_CELLS];
    memset(priced, 0, COMBAT_GRID_CELLS);
    for (long i = m_numArmies[estimate->m_ourGroup]; i-- > 0; other++) {
        if (other->is(1u << 21))
            continue;
        if (other == ourArmy)
            continue;
        if (other->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (estimate->m_simulated && other->getTotalHitPoints(1) == 0)
            continue;
        if (!m_cells[other->m_gridIndex].m_validMove)
            continue;
        const pathCell* cell = currentSearchArray->getHex(other->m_gridIndex);
        if (cell->m_cost > enemy->getSpeed())
            continue;
        unsigned char counted[COMBAT_GRID_CELLS];
        memset(counted, 0, COMBAT_GRID_CELLS);
        hexes.clear();
        findAttackHexes(other, enemy, currentSearchArray, &hexes);
        for (long j = hexes.size(); j-- > 0; ) {
            long hex = hexes[j];
            long directions = enemy->getMultiHeadDirections(
                    hex, other, other->m_gridIndex);
            long count = (enemy->is(1u << 0)) ? 8 : 6;
            for (long direction = count; direction-- > 0; ) {
                if ((directions & (1 << direction)) == 0)
                    continue;
                long target = enemy->getAdjacentHex(hex, direction);
                if (target < 0 || target >= COMBAT_GRID_CELLS)
                    continue;
                if (counted[target])
                    continue;
                if (priced[target])
                    enemyAttacks[target]--;
                else
                    enemyAttacks[target] += value;
                if (enemyAttacks[target] < limitValue)
                    enemyAttacks[target] = limitValue;
                priced[target] = 1;
                counted[target] = 1;
            }
        }
    }
}

VA(0x00420060, 0x1FB)  // dc 0x25308
void findAttackHexes(const army* ourArmy, long targetHex, long start, long stop, long limitCost, const searchArray* currentSearchArray, std::vector<long>* result)
{
    for (long direction = start; direction < stop; direction++) {
        long hex = ourArmy->getAdjacentHex(targetHex, direction);
        if (hex < 0 || hex >= COMBAT_GRID_CELLS)
            continue;
        const pathCell* cell = currentSearchArray->getHex(hex);
        if (!cell->m_visited)
            continue;
        if (cell->m_cost > limitCost)
            continue;
        result->push_back(hex);
    }
}

VA(0x00420260, 0x368)  // dc 0x256a0
void combatManager::markEnemyAttacks(const army* ourArmy, long* enemyAttacks, long* dangerousEnemies, type_AI_combat_parameters* estimate) const
{
    long side = estimate->m_ourGroup;
    long enemySide = estimate->m_enemyGroup;
    long hitPoints = ourArmy->getTotalHitPoints(estimate->m_simulated);
    long floorValue = -ourArmy->getLossCombatValue(
            estimate->m_lowestAttack, estimate->m_lowestDefense,
            ourArmy->canShoot(0), hitPoints, 0);
    const army* enemy = m_armies[enemySide];
    *dangerousEnemies = 0;
    for (long i = 0; i < m_numArmies[enemySide]; i++, enemy++) {
        if (enemy->m_spellInfluence[62])
            continue;
        if (enemy->m_spellInfluence[70])
            continue;
        if (enemy->m_spellInfluence[74])
            continue;
        if (enemy->is(1u << 21))
            continue;
        if (enemy->m_creatureType == CREATURE_FIRST_AID_TENT
                || enemy->m_creatureType == CREATURE_AMMO_CART
                || enemy->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (enemy->canShoot(0)) {
            *dangerousEnemies |= 1 << enemy->m_bitIndex;
            continue;
        }
        g_searchArray->seedCombatPosition(enemy, enemySide,
                                          enemy->getSpeed() + 1, 0,
                                          enemy->getSpeed() + 1);
        long j;
        const army* friendly = m_armies[side];
        for (j = 0; j < m_numArmies[side]; j++, friendly++) {
            if (friendly->is(1u << 21))
                continue;
            if (friendly->m_creatureType == CREATURE_ARROW_TOWER)
                continue;
            const pathCell* cell = g_searchArray->getHex(friendly->m_gridIndex);
            if (m_cells[friendly->m_gridIndex].m_validMove
                    && cell->m_cost <= enemy->getSpeed())
                break;
        }
        if (j < m_numArmies[side]) {
            *dangerousEnemies |= 1 << enemy->m_bitIndex;
            if ((enemy->is(1u << 19)) && g_game->m_setup.m_difficulty >= 2
                    && !m_sideIsAi[side])
                markMultiheadedEnemy(ourArmy, enemy, enemyAttacks,
                                       floorValue, g_searchArray, estimate);
            continue;
        }
        long value = -estimate->getSimpleAttackEffect(*(enemy), *(ourArmy), 0, 0);
        if (value >= 0)
            continue;
        for (long hex = 0; hex < COMBAT_GRID_CELLS; hex++) {
            const pathCell* cell = g_searchArray->getHex(hex);
            if (!cell->m_visited)
                continue;
            enemyAttacks[hex] += value;
            if (enemyAttacks[hex] < floorValue)
                enemyAttacks[hex] = floorValue;
        }
        if (enemy->m_monInfo.m_attributes & 1) {
            long direction = enemy->m_facing ? 1 : 4;
            for (long hex = 0; hex < COMBAT_GRID_CELLS; hex++) {
                const pathCell* cell = g_searchArray->getHex(hex);
                if (!cell->m_visited)
                    continue;
                if (g_searchArray->isMoat(static_cast<short>(hex)))
                    continue;
                long adjacent = m_adjacentCells[hex][direction];
                if (adjacent < 0 || adjacent >= COMBAT_GRID_CELLS)
                    continue;
                const pathCell* other = g_searchArray->getHex(adjacent);
                if (other->m_visited)
                    continue;
                enemyAttacks[adjacent] += value;
                if (enemyAttacks[adjacent] < floorValue)
                    enemyAttacks[adjacent] = floorValue;
            }
        }
    }
}

// E:\gamedcs\ai.cpp:1357
// Where a stack should stand to shield `client`: the walk visits the
// client's six neighbours - eight when the client is two hexes wide -
// keeps the hexes that are empty or already the mover's own, and counts
// every one of them into *open_hexes even when it is unreachable. Among
// the reachable ones the cheapest to walk to wins; a tie goes to the hex
// that also puts the mover's SECOND cell next to the client, and a tie
// on that goes by screen x, toward the side the client faces.

// best_time / best_contact are deliberately uninitialised: retail writes
// neither before the loop and both are only read once *best_hex is no
// longer -1.

// E:\gamedcs\ai.cpp:1357
VA(0x004205d0, 0x185)  // linkorder, dc 0x25998
unsigned char combatManager::chooseDefenseHex(const army* currentArmy, const army* client, long* bestHex, long* openHexes, searchArray* currentSearchArray)
{
    long bestTime;
    long bestContact;

    *openHexes = 0;
    *bestHex = -1;
    for (long direction = 0; direction < 8; direction++) {
        if (direction >= 6 && !(client->m_monInfo.m_attributes & 1))
            continue;
        long hex = client->getAdjacentHex(client->m_gridIndex, direction);
        if (hex < 0 || hex >= COMBAT_GRID_CELLS)
            continue;
        hexcell* cell = &m_cells[hex];
        army* occupant = cell->getArmy();
        if (occupant != 0 && occupant != currentArmy)
            continue;
        (*openHexes)++;
        const pathCell* path = currentSearchArray->getHex(hex);
        if (!path->m_visited)
            continue;
        long time = m_creaturePlacement
                ? 1
                : currentSearchArray->getTravelTime(currentArmy, hex);
        long contact;
        if ((currentArmy->m_monInfo.m_attributes & 1)
                && client->isAdjacent(hex + (currentArmy->m_facing ? 1 : -1)))
            contact = 2;
        else
            contact = 1;
        if (*bestHex >= 0) {
            if (time > bestTime)
                continue;
            if (time == bestTime) {
                if (contact < bestContact)
                    continue;
                if (contact == bestContact) {
                    if (client->m_facing == 1) {
                        if (cell->m_refX < m_cells[*bestHex].m_refX)
                            continue;
                    } else if (cell->m_refX > m_cells[*bestHex].m_refX) {
                        continue;
                    }
                }
            }
        }
        bestTime = time;
        bestContact = contact;
        *bestHex = hex;
    }
    return static_cast<unsigned char>(*bestHex >= 0);
}

VA(0x00420760, 0x187)  // dc 0x25b0c
unsigned char combatManager::attemptShooterDefense(const army* currentArmy, searchArray* currentSearchArray, const type_AI_combat_parameters* estimate)
{
    long hex;

    long bestHex = -1;
    const army* client = m_armies[m_currentSide];
    long openHexes = 0;
    const army* bestClient = 0;
    long bestTime = 0;
    long bestValue = 0;
    for (long i = 0; i < m_numArmies[m_currentSide]; i++, client++) {
        if (client->is(1u << 21))
            continue;
        if (client->m_creatureType == CREATURE_ARROW_TOWER
                || client->m_creatureType == CREATURE_CATAPULT)
            continue;
        if (!client->canShoot(0))
            continue;
        if (client->is(1u << 12))
            continue;
        if (!chooseDefenseHex(currentArmy, client, &hex, &openHexes,
                                currentSearchArray))
            continue;
        long time = m_creaturePlacement
                ? 1
                : currentSearchArray->getTravelTime(currentArmy, hex);
        long value = client->getTotalCombatValue(estimate->m_lowestAttack,
                                                    estimate->m_lowestDefense);
        if (openHexes > 1)
            value /= openHexes;
        if (bestClient != 0) {
            if (time > bestTime)
                continue;
            if (time == bestTime && bestValue > value)
                continue;
        }
        bestValue = value;
        bestClient = client;
        bestTime = time;
        bestHex = hex;
    }
    if (bestClient == 0)
        return 0;
    if (hex == currentArmy->m_gridIndex) {
        m_nextAction = 3;
        return 1;
    }
    moveToward(currentArmy, bestHex, 0, 0);
    return 1;
}

VA(0x004208f0, 0x184)  // dc 0x25c80
unsigned char combatManager::chooseToRun(const army* ourArmy, const long* enemyAttacks, const searchArray* currentSearchArray)
{
    if (g_game->m_setup.m_difficulty < 2
        && !m_sideIsAi[ourArmy->m_combatSide])
        return 0;

    long worstDanger = enemyAttacks[ourArmy->m_gridIndex];
    if (ourArmy->m_monInfo.m_attributes & 1) {
        long secondHex = ourArmy->getSecondGridIndex();
        worstDanger = min(
            enemyAttacks[secondHex], worstDanger);
    }

    if (worstDanger >= 0
        || ourArmy->m_spellInfluence[62]
        || ourArmy->m_spellInfluence[70]
        || ourArmy->m_spellInfluence[74]
        || ourArmy->m_spellInfluence[72])
        return 0;

    long bestDistance = 0;
    long bestHex = -1;
    for (long hex = 0; hex < COMBAT_GRID_CELLS; ++hex) {
        const pathCell* cell = currentSearchArray->getHex(hex);
        if (!cell->m_visited || cell->m_flightCost > 0)
            continue;

        long distance = cell->m_cost;
        if (distance > ourArmy->getSpeed())
            continue;

        long danger = enemyAttacks[hex];
        if (ourArmy->m_monInfo.m_attributes & 1) {
            long secondHex = hex + (ourArmy->m_facing ? 1 : -1);
            danger = min(enemyAttacks[secondHex], danger);
        }
        if (danger < worstDanger)
            continue;
        if (danger == worstDanger && bestDistance < distance)
            continue;
        worstDanger = danger;
        bestDistance = distance;
        bestHex = hex;
    }

    if (bestHex < 0 || bestHex == ourArmy->m_gridIndex)
        return 0;
    return moveToward(ourArmy, bestHex, enemyAttacks, 0);
}

// Per side it sums get_total_combat_value over every stack that can
// still act - not petrified (creatureId bit 21), none of the three
// disable counters set, and not the immovable Arrow Tower - and keeps a
// second running total over just the shooters. A side whose hero can
// still cast then adds the best spell that hero has, priced by
// type_spellvalue with the side's WHOLE combat value as the stack
// value: a hero's spell reaches the enemy from the back row exactly as
// a shot does, so it belongs in the shooting column.

VA(0x00420a80, 0x264)  // dc 0x25df8
unsigned char combatManager::hasRangedAdvantage(type_AI_combat_parameters* data)
{
    long totalValue[2];
    long shooterValue[2];

    for (long side = 0; side < 2; side++) {
        shooterValue[side] = 0;
        totalValue[side] = 0;
        const army* stack = m_armies[side];
        for (long i = 0; i < m_numArmies[side]; i++, stack++) {
            if (!(stack->is(1u << 21))
                    && !stack->m_spellInfluence[62] && !stack->m_spellInfluence[70]
                    && !stack->m_spellInfluence[74]
                    && stack->m_creatureType != CREATURE_ARROW_TOWER) {
                long value = stack->getTotalCombatValue(
                    data->m_lowestAttack, data->m_lowestDefense);
                totalValue[side] += value;
                if (stack->canShoot(0))
                    shooterValue[side] += value;
            }
        }
        if (m_heroes[side] != 0 && canCastSpells(side, 1)) {
            type_spellvalue valuer(m_heroes[side]);
            valuer.setStackValue(totalValue[side]);
            shooterValue[side] += valuer.getBestSpellValue(0x8000);
        }
    }

    if (m_fortificationLevel >= COMBAT_FORTIFICATION_CITADEL) {
        int numArchers;
        int archerLevel;
        m_defendingTown->calcNumLevelArchers(&numArchers, &archerLevel);
        if (m_wallStrength[14] > 0)
            shooterValue[1] += g_creatureTypeTraits[CREATURE_ARCHER].m_aiValue
                                * numArchers;
        if (m_fortificationLevel == COMBAT_FORTIFICATION_CASTLE) {
            if (m_wallStrength[13] > 0)
                shooterValue[1] +=
                    g_creatureTypeTraits[CREATURE_ARCHER].m_aiValue
                    * (numArchers + 1) / 2;
            if (m_wallStrength[5] > 0)
                shooterValue[1] +=
                    g_creatureTypeTraits[CREATURE_ARCHER].m_aiValue
                    * (numArchers + 1) / 2;
        }
    }

    return shooterValue[data->m_ourGroup] > shooterValue[data->m_enemyGroup];
}

// CodeView's type_spellvalue destructor is compiler-generated (LF_ONEMETHOD
// compgenx, attributes 0x103; dc 0x28050). Retail's 0x627a40 unwind funclet
// uses this retained Dinkumware vector teardown; the implicit member also
// supplies each caller's expansion without a fabricated source body.
VA_COMPGEN(0x00420cf0, 0x26, IMPLICIT_DTOR, type_spellvalue)

// E:\gamedcs\ai.cpp:1635
// The Master Genie / Dragon Fly chooser: it walks OUR OWN side from the
// last slot down, skips anything the caster cannot legally target, and
// takes the best-scoring friend. Two gates are worth naming - once a
// spell has already been chosen this round the whole call is abandoned
// three times in ten, and once *best_value is positive a friend is only
// considered when it is worth at least as much as the caster itself.

// `value` IS UNINITIALISED on the fall-through arm: retail's third path
// through the creatureType test reads the dead `estimate` parameter slot
// straight back out (0x420dee reloads [ebp+0x10] with nothing having
// written it). Only choose_spell_action reaches this body, and it only
// sends the two ids the test names, so the arm is unreachable - but it
// is transcribed as written.

// EH-bearing (P2.2) and NOT blocked by it: the fs:[0] frame is the local
// type_AI_spellcaster's unwind scaffolding, one state.

// E:\gamedcs\ai.cpp:1635
VA(0x00420d20, 0x1D5)  // anchor-callee, dc 0x2600c
unsigned char combatManager::chooseCreatureSpell(const army* currentArmy, long* bestValue, type_AI_combat_parameters* estimate)
{
    long side = estimate->m_ourGroup;
    long count = m_numArmies[side];
    long bestHex = -1;
    long ourValue = currentArmy->getTotalCombatValue(
            estimate->m_lowestAttack, estimate->m_lowestDefense);
    type_AI_spellcaster caster(this, estimate->m_ourGroup, 1);
    if (*bestValue != 0 && random(1, 100) <= 30)
        return 0;
    for (long i = count; i-- > 0; ) {
        const army* target = &m_armies[side][i];
        if (target == currentArmy)
            continue;
        if (target->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (!currentArmy->canCastSpell(target->m_gridIndex))
            continue;
        if (*bestValue > 0
                && ourValue > target->getTotalCombatValue(
                        estimate->m_lowestAttack, estimate->m_lowestDefense))
            continue;
        long value;
        switch (currentArmy->m_creatureType) {
        case CREATURE_MASTER_GENIE:
            value = caster.getCaliphValue(target);
            break;
        case CREATURE_OGRE_MAGE:
            value = caster.getOgreMageValue(target);
            break;
        }
        if (value <= 0)
            continue;
        if (value <= *bestValue)
            continue;
        *bestValue = value;
        bestHex = target->m_gridIndex;
    }
    if (bestHex < 0)
        return 0;
    m_nextAction = 10;
    m_nextActionGridIndex = bestHex;
    return 1;
}

// RETAIL-ONLY SoD chooser. choose_spell_action's jump table routes only
// CREATURE_FAERIE_DRAGON here. It scans the visible 15 columns of every
// combat row and asks the creature-specific spell valuer for the best hex;
// positive ties do not replace an earlier choice. The local spellcaster
// owns the one-state /GX frame visible in retail.

// WALL 2026-08-22 (94.51765%, 251 bytes): control flow is exact - both
// sides have 85 instructions, nine conditional branches, two returns and
// thirteen blocks, with branch-shape distance zero. The remaining delta is
// one EBX/EDI role swap: candidate keeps current_army in EBX and best_hex
// in EDI, while retail does the reverse. why-reg v2 reports 28
// register-visible slots with identical definition slots/order, classifying
// the difference as C1/front-end handle state. Its model-selected adjacent
// best_hex/value declaration swap is byte-flat; making best_hex volatile
// regresses to 61 slots, and pre-creating an army pointer alias is also
// byte-flat. The independently fixed address lets the cross-build symbol
// contribute the private bool/reference declarator below, but that stronger
// source typing is likewise byte-identical and does not change the wall.
VA(0x00420f00, 0xFB)
bool combatManager::sodChooseFaerieDragonSpell(
        const army* currentArmy, long& bestValue,
        type_AI_combat_parameters& estimate)
{
    long bestHex = -1;
    type_AI_spellcaster caster(this, estimate.m_ourGroup, 1);
    for (long hex = 0; hex < COMBAT_GRID_CELLS; hex++) {
        if (inInvisibleColumn(hex))
            continue;
        long value = caster.getFaerieDragonSpellValue(
                hex, currentArmy->m_numTroops * 5,
                currentArmy->m_faerieDragonSpell);
        if (value <= 0)
            continue;
        if (bestHex >= 0 && value <= bestValue)
            continue;
        bestHex = hex;
        bestValue = value;
    }
    if (bestHex < 0)
        return 0;
    m_nextAction = 10;
    m_nextActionGridIndex = bestHex;
    return 1;
}

// E:\gamedcs\ai.cpp:1694
// Nothing else in the TU calls army::get_resurrection_size, and this
// body does. `ret 0xc` is the DC parameter count exactly.

// The Archangel and the Pit Lord share one chooser and differ in three
// places: the Pit Lord looks for ANIMATE-DEAD targets (and only DEAD
// ones), and it prices the raise as DEMONS - which is what the scratch
// `army temp` is for, built once outside the walk and valued instead of
// the corpse. The Archangel prices the stack it is actually restoring.
// Either way the value doubles while our side is already ahead on live
// value and the odds have not turned.

// EH-bearing (P2.2) and NOT blocked by it: the fs:[0] frame is the local
// army's unwind scaffolding, one state.

// Residual (95.8%): caller-saved register ties on the estimate->side and
// live-value loads, plus ONE redundant `fstp/fld` pair our CL inserts on
// the non-Pit-Lord arm of the value ternary where retail merges the two
// arms in st(0). The single `__ftol` is what forces the ternary
// spelling - two `static_cast<long>` arms emit two conversions, which
// retail does not have.
// E:\gamedcs\ai.cpp:1694
VA(0x00421000, 0x275)  // anchor-callee, dc 0x26140
unsigned char combatManager::chooseResurrectAction(const army* currentArmy, long* bestValue, type_AI_combat_parameters* estimate)
{
    long bestHex = -1;
    if ((currentArmy->m_creatureType != CREATURE_ARCHANGEL
            && currentArmy->m_creatureType != CREATURE_PIT_LORD)
            || currentArmy->m_monInfo.m_hasSpell <= 0)
        return 0;
    army temp;
    if (currentArmy->m_creatureType == CREATURE_PIT_LORD)
        temp.initialize(TCreatureType(CREATURE_DEMON), 1,
                        m_heroes[estimate->m_ourGroup],
                        estimate->m_ourGroup, 0, 0);
    for (long i = m_numArmies[estimate->m_ourGroup]; i--; ) {
        const army* target = &m_armies[estimate->m_ourGroup][i];
        if (target == currentArmy)
            continue;
        if (target->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        long hex = target->m_gridIndex;
        if (currentArmy->m_creatureType == CREATURE_PIT_LORD) {
            if (findAnimateDeadTarget(estimate->m_ourGroup, hex) != target) {
                if ((target->m_monInfo.m_attributes & 1) == 0)
                    continue;
                hex = target->getSecondGridIndex();
                if (findAnimateDeadTarget(estimate->m_ourGroup, hex) != target)
                    continue;
            }
            if ((target->is(1u << 21)) == 0)
                continue;
        } else {
            if (findResurrectionTarget(estimate->m_ourGroup, hex, 1) != target) {
                if ((target->m_monInfo.m_attributes & 1) == 0)
                    continue;
                hex = target->getSecondGridIndex();
                if (findResurrectionTarget(estimate->m_ourGroup, hex, 1) != target)
                    continue;
            }
        }
        long size = currentArmy->getResurrectionSize(target);
        if (size == 0)
            continue;
        long value = static_cast<long>(
                currentArmy->m_creatureType == CREATURE_PIT_LORD
                ? temp.getUnitCombatValue(estimate->m_lowestAttack,
                                             estimate->m_lowestDefense, 0, 0)
                        * size
                : target->getUnitCombatValue(estimate->m_lowestAttack,
                                                estimate->m_lowestDefense,
                                                target->canShoot(0), 0)
                        * size);
        if (estimate->m_awakeFriendlyValue > estimate->m_awakeEnemyValue
                && estimate->m_roundsLeft <= 1)
            value += value;
        if (value > *bestValue) {
            *bestValue = value;
            bestHex = hex;
        }
    }
    if (bestHex < 0)
        return 0;
    m_nextActionGridIndex = bestHex;
    m_nextAction = 10;
    return 1;
}

VA(0x00421280, 0x166)  // dc 0x26464
unsigned char combatManager::chooseSpellAction(const army* currentArmy, long* bestValue, type_AI_combat_parameters* estimate)
{
    if (m_creaturePlacement)
        return 0;
    if (!canCastSpells(estimate->m_ourGroup, 0))
        return 0;
    if (currentArmy->m_monInfo.m_hasSpell == 0)
        return 0;
    switch (currentArmy->m_creatureType) {
    case CREATURE_ARCHANGEL:
    case CREATURE_PIT_LORD:
        if (chooseResurrectAction(currentArmy, bestValue, estimate))
            return 1;
        break;
    case CREATURE_MASTER_GENIE:
    case CREATURE_OGRE_MAGE:
        if (chooseCreatureSpell(currentArmy, bestValue, estimate))
            return 1;
        break;
    case CREATURE_FAERIE_DRAGON:
        if (sodChooseFaerieDragonSpell(currentArmy, *bestValue, *estimate))
            return 1;
        break;
    }
    return 0;
}

// RECONSTRUCTED 2026-08-08. Only the DEFENDER (side 1) of a walled
// combat can stay in: every breached wall segment whose hex is walkable
// is a way in, and one of those loses the argument outright. If the
// walls still hold, the AI needs a shooting advantage AND every one of
// its own live stacks - bar the arrow tower - has to be inside the
// castle already.

VA(0x004213f0, 0xF5)  // dc 0x264fc
unsigned char combatManager::shouldStayInCastle(type_AI_combat_parameters* estimate)
{
    if (!m_fortificationLevel)
        return 0;
    if (estimate->m_ourGroup != 1)
        return 0;
    { for (const long* target = g_castleWallGateTargets;
           target < g_castleWallGateTargetsEnd; target++) {
        if (m_wallStrength[*target])
            continue;
        if (!hexIsBlocked(s_wallTargets[*target].getBlockedHex()))
            return 0;
    } }
    if (!hasRangedAdvantage(estimate))
        return 0;
    const army* ourArmy = &m_armies[estimate->m_ourGroup][0];
    for (long i = 0; i < m_numArmies[estimate->m_ourGroup]; i++, ourArmy++) {
        if ((ourArmy->is(1u << 21)) == 0
                && ourArmy->m_creatureType != CREATURE_ARROW_TOWER
                && !inCastle(ourArmy->m_gridIndex))
            return 0;
    }
    return 1;
}

// SPELL ID 0x0d IS SPELLED AS A LITERAL ON PURPOSE: the roster lives in
// armygrp.h, which another lane owns this session. It wants
// `SPELL_FIRE_WALL = 0xd` and this call should read it - flagged for
// the next armygrp change rather than reached across for.

VA(0x004214f0, 0x94)  // dc 0x26600
void combatManager::markFirewalls(const army* currentArmy, long* enemyAttacks, type_AI_combat_parameters* estimate)
{
    for (long i = 0; i < 187; i++) {
        if ((m_cells[i].m_attributes & 0x10) == 0)
            continue;
        TObstacle* obstacle = &getObstacle(m_cells[i].m_obstacleIndex);
        long base = obstacle->m_spellDamage;
        long damage = modifySpellDamage(base, 0xd,
                                        m_heroes[obstacle->m_owner],
                                        m_heroes[estimate->m_ourGroup],
                                        currentArmy, 0);
        enemyAttacks[i] -= currentArmy->getLossCombatValue(
                estimate->m_lowestAttack, estimate->m_lowestAttack, 0, damage,
                estimate->m_killsOnly);
    }
}

VA(0x00421590, 0xE1)
void combatManager::markMoat(const army* currentArmy, long* enemyAttacks,
                         type_AI_combat_parameters* estimate)
{
    if (!m_moatOn)
        return;

    long row;
    for (row = 0; row < 11; row++) {
        long hex = g_moatColumns[row];
        if (m_drawbridgeState == DRAWBRIDGE_UP
                || hex != COMBAT_HEX_GATE_MOAT) {
            long damage = g_moatDamage[m_defendingTown->m_type];
            enemyAttacks[hex] -= currentArmy->getLossCombatValue(
                    estimate->m_lowestAttack, estimate->m_lowestAttack, 0,
                    damage, estimate->m_killsOnly);
        }
    }

    if (!m_moatIsWide)
        return;
    for (row = 0; row < 11; row++) {
        long hex = g_outerMoatColumns[row];
        if (m_drawbridgeState == DRAWBRIDGE_UP
                || hex != COMBAT_HEX_OUTER_MOAT) {
            long damage = g_moatDamage[m_defendingTown->m_type];
            enemyAttacks[hex] -= currentArmy->getLossCombatValue(
                    estimate->m_lowestAttack, estimate->m_lowestAttack, 0,
                    damage, estimate->m_killsOnly);
        }
    }
}

// E:\gamedcs\ai.cpp:1896
// The melee chooser: score every enemy stack the mover could reach,
// keep the best, and turn that into an order - or, when nothing is
// worth attacking, into a defensive one. Four danger-map passes run
// first (fire walls always, the moat above fortification level 2, the
// enemy and friendly maps under the difficulty/AI gates), then the
// mover's reachability is seeded, then the enemy walk, then a tail
// with SEVEN exits.

// The decoded map that preceded this body (banked 2026-08-14, now
// spent) named seven pieces of missing surface. All seven landed:
// mark_moat and the two Harpy ids are now declared, maximum selection
// uses the canonical header helper, and the other four were
// already in the tree under names the map did not know - gpGame->
// f_1f698, army::boundFlag (+0x2b8), gCastleWallColumns (0x63bd00) and
// cmbtmgr.h's InCastle / combatManager::IsInMoat.

// Things worth knowing about the transcription:
//   * `budget` is spelled as an assignment from itself rather than an
//     `if (...) budget = 0;` because retail SELECTS into EAX (`xor eax,
//     eax` / `mov eax,[ebp-0x30]`) ahead of the teleport branch instead
//     of storing zero into the slot.
//   * the disabled-stack predicate is spelled out three fields at a
//     time everywhere it appears. army::IsIncapacitated is pinned
//     `auto_inline(off)` in this TU to protect find_move_order's single
//     retail call, so reaching for it here would emit a CALL where
//     retail has the fields.
//   * the two `field_3c = 6` exits store in DIFFERENT orders - the
//     teleport one writes 3c/40/44, the commit one 40/3c/44. Both are
//     transcribed as retail has them.
//   * DC 2119..2138 puts the defensive action in its own positive scope
//     after the fortification arm. That ordinary if also reproduces the
//     current retail comparison; a labelled commit is unnecessary.
//   * the moat-row walk is the fortification arm's last resort: index
//     gCastleWallColumns by the mover's row (gridIndex / 17), then scan
//     DOWN from that hex for the first one that is both reachable and
//     out of the moat, pricing BOTH cells of a two-hex stack.
// CURRENT (89.69%, from 82.20% on 2026-08-21): two excess source carriers
// caused the old whole-body register wall.  Leaving `estimate->enemy_side`
// at its three uses lets VC6 CSE it into a home without creating the early
// named pseudo; `estimate` then remains in EBX and `this` in ESI as retail
// requires (88.91%).  Leaving `chooser.best_value` at its uses removes the
// second unnecessary pseudo and reaches 89.69%.  Both are semantic no-ops,
// but their front-end creation order is byte-load-bearing in VC6.

// The remaining plateau is mixed.  Both sides still have 118 branches and
// 10 returns, but the base has 164 CFG blocks against retail's 166 and a
// 0x348 frame against 0x350.  `predict-inline` localises one difference:
// our CL expands the FINAL move_toward call while retail emits all three;
// a statement-granular `inline_depth(0)` at that site is byte-flat, so the
// ordinary inliner knob cannot reproduce the decision.  The rest is a
// 337-slot scratch-register/scheduling residue, especially the loop-index
// home and the maximum-selection tail.  Measured negative probes: a volatile loop
// counter (81.70), an address-carried budget (79.64), and moving a named
// enemy_side down beside the loop (76.38).  Naming/typing the side carrier
// as volatile-long or unsigned-char changes diagnostics but is score-flat.
// The direct address-carrier variants are exhausted too (2026-08-21): an
// unused `long* = &i`, incrementing through that pointer, and spelling `i`
// as a one-element stack array are all byte-flat at 89.6892; naming the
// 21-stack row offset explicitly regresses to 88.1274 and still strength-
// reduces the enemy pointer. The missing dword is not reachable by those
// obvious address-taken spellings.
// The Dreamcast function-scope roster is now exhausted too (2026-08-21):
// widening `random_factor`, `i`, and `is_blocking_action` from their current
// loop scopes is byte-flat individually, and widening all three together is
// still exactly 89.6892 with the same 0x348 frame, 118 branches / 10 returns,
// and sole polarity row. Lexical lifetime of the attested locals therefore
// cannot supply retail's extra scalar home.

// The two header accessors and conventional release VERIFY are bounded
// separately (2026-08-21). Dreamcast lines 1899/1900 call `get_group()` and
// `get_enemy_group()` into the named `our_group`/`enemy_group` locals, and
// retail's prologue likewise loads +0x20 and +0x24 before the zero stores.
// Restoring both inline accessors, declaring those two locals in that order,
// and using them at every site does create another home (frame 0x348 ->
// 0x34c), but rotates `this` out of ESI, adds polarity differences #42/#76,
// and falls to 82.88036; retail still has a 0x350 frame. On that exact
// source shape, release-style evaluations of `this != 0`, `estimate != 0`,
// and `current_army != 0` are individually byte-flat at 82.88036. Thus a
// conventional VERIFY is possible history but cannot select the retail
// allocator phase here; no fabricated macro is retained.

// E:\gamedcs\ai.cpp:1896
VA(0x00421680, 0x8F9)  // linkorder, dc 0x266d4
unsigned char combatManager::chooseMeleeTarget(const army* currentArmy, unsigned char teleport, long* actionValue, type_AI_combat_parameters* estimate)
{
    long enemyAttacks[COMBAT_GRID_CELLS];

    // BOUND BY `const long&`, not copied: retail re-reads `estimate->side`
    // across the opaque mark_* calls rather than keeping a cached copy live,
    // which is worth 89.6892 -> 91.6697.  Writing `estimate->side` out at all
    // twelve uses and dropping the local reproduces the same reloads and
    // scores 91.6502, so the named binding is the better spelling of the same
    // fact.  The same change at the sibling declarations (line 1955,
    // AICheckRetreat's caller) LOSES 13.6, so it is per-body.
    const long& side = estimate->m_ourGroup;
    const army* bestEnemy = 0;
    long bestValue = 0;
    long bestTroops = 0;
    long bestTime = 0;
    unsigned char bestFlag = 0;
    long dangerousEnemies = 0;
    long bestHex = -1;
    long budget = 127;
    memset(enemyAttacks, 0, sizeof(enemyAttacks));

    markFirewalls(currentArmy, enemyAttacks, estimate);
    if (g_game->m_f1f698 >= 2)
        markMoat(currentArmy, enemyAttacks, estimate);
    if (g_game->m_setup.m_difficulty > 0 || m_sideIsAi[side])
        markEnemyAttacks(currentArmy, enemyAttacks, &dangerousEnemies,
                           estimate);
    if (g_game->m_setup.m_difficulty >= 2 || m_sideIsAi[side])
        markFriendlyArmies(currentArmy, enemyAttacks, dangerousEnemies,
                             estimate);

    budget = (currentArmy->m_spellInfluence[62] || currentArmy->m_spellInfluence[70]
              || currentArmy->m_spellInfluence[74])
            ? 0
            : budget;
    if (teleport)
        g_searchArray->markTeleport(currentArmy, side);
    else
        g_searchArray->seedCombatPosition(currentArmy, side, budget,
                                          m_creaturePlacement, -1);
    unsigned char stayInCastle = shouldStayInCastle(estimate);

    for (long i = 0; i < m_numArmies[estimate->m_enemyGroup]; i++) {
        const army* enemy = &m_armies[estimate->m_enemyGroup][i];
        if (enemy->is(1u << 21))
            continue;
        if (enemy->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (estimate->m_simulated && enemy->getTotalHitPoints(1) == 0)
            continue;
        if (currentArmy->getSpeed() == 0 || currentArmy->m_spellInfluence[72]) {
            const pathCell* stand = g_searchArray->getHex(enemy->m_gridIndex);
            if (stand->m_cost > 0)
                continue;
        }
        if (stayInCastle && !inCastle(enemy->m_gridIndex)
                && (!(enemy->m_monInfo.m_attributes & 1)
                    || !inCastle(enemy->getSecondGridIndex())))
            continue;

        long change = 0;
        unsigned char flag = 0;
        if (m_cells[enemy->m_gridIndex].m_validMove
                && (g_game->m_setup.m_difficulty >= 2 || m_sideIsAi[side])
                && !currentArmy->m_spellInfluence[62]
                && !currentArmy->m_spellInfluence[70]
                && !currentArmy->m_spellInfluence[74]
                && !(currentArmy->is(1u << 21))
                && currentArmy->m_creatureType != CREATURE_FIRST_AID_TENT
                && currentArmy->m_creatureType != CREATURE_AMMO_CART
                && !(enemy->is(1u << 19))) {
            const pathCell* reach = g_searchArray->getHex(enemy->m_gridIndex);
            if (reach->m_cost <= currentArmy->getSpeed())
                change = getAttackChange(currentArmy, enemy, *estimate);
        }

        type_AI_attack_hex_chooser chooser(currentArmy, enemy, enemyAttacks,
                                           g_searchArray, estimate);
        if (!chooser.findAttackHex())
            continue;
        if (!currentArmy->m_spellInfluence[62] && !currentArmy->m_spellInfluence[70]
                && !currentArmy->m_spellInfluence[74]
                && !(currentArmy->is(1u << 21))
                && currentArmy->m_creatureType != CREATURE_FIRST_AID_TENT
                && currentArmy->m_creatureType != CREATURE_AMMO_CART) {
            long distance = 0;
            if (chooser.m_bestAttackTime == 1) {
                const pathCell* attackCell = g_searchArray->getHex(chooser.getBestHex());
                distance = attackCell->m_cost;
            }
            change += estimate->getSimpleAttackEffect(*(currentArmy), *(enemy), 0, distance);
        }

        if (chooser.getHexValue() <= 0 || (currentArmy->is(1u << 22))
                || (g_game->m_setup.m_difficulty == 0 && !m_sideIsAi[side])) {
            change += chooser.getHexValue();
        } else if (currentArmy->m_creatureType == CREATURE_HARPY
                   || currentArmy->m_creatureType == CREATURE_HARPY_HAG) {
            if (change < chooser.getHexValue()) {
                flag = 1;
                change = chooser.getHexValue();
            }
        } else if (change < 0 && hasRangedAdvantage(estimate)) {
            change = chooser.getHexValue();
            flag = 1;
        } else {
            change += chooser.getHexValue();
        }
        if (change < 0
                && (g_game->m_setup.m_difficulty > 0 || m_sideIsAi[side])
                && chooser.getHexValue() < enemyAttacks[chooser.getBestHex()])
            continue;

        long random = ::random(75, 100);
        long score = random * change / 100;
        long troops = chooser.m_bestAttackTime;
        if (bestEnemy != 0) {
            if ((enemy->m_spellInfluence[62] || enemy->m_spellInfluence[70]
                 || enemy->m_spellInfluence[74])
                    && !(bestEnemy->m_spellInfluence[62] || bestEnemy->m_spellInfluence[70]
                         || bestEnemy->m_spellInfluence[74]))
                continue;
            if (!((bestEnemy->m_spellInfluence[62] || bestEnemy->m_spellInfluence[70]
                   || bestEnemy->m_spellInfluence[74])
                  && !(enemy->m_spellInfluence[62] || enemy->m_spellInfluence[70]
                       || enemy->m_spellInfluence[74]))) {
                if (bestTroops < troops)
                    continue;
                if (bestTroops == troops) {
                    if (bestValue > score)
                        continue;
                    if (bestValue == score) {
                        if (bestEnemy->m_topCreatureDamage
                                > enemy->m_topCreatureDamage)
                            continue;
                        if (bestEnemy->m_topCreatureDamage
                                == enemy->m_topCreatureDamage) {
                            const pathCell* held = g_searchArray->getHex(bestHex);
                            const pathCell* offered
                                    = g_searchArray->getHex(chooser.getBestHex());
                            if (held->m_cost < offered->m_cost)
                                continue;
                        }
                    }
                }
            }
        }
        bestEnemy = enemy;
        bestFlag = flag;
        bestHex = chooser.getBestHex();
        bestTroops = troops;
        bestTime = chooser.getHexValue() * random / (troops * 100);
        bestValue = score / troops;
    }

    *actionValue = max(bestValue, bestTime);
    if (teleport) {
        if (bestEnemy != 0 && *actionValue >= 0) {
            m_nextAction = 6;
            m_nextActionExtra = bestHex;
            m_nextActionGridIndex = bestEnemy->m_gridIndex;
            return 1;
        }
        m_nextAction = 3;
        return 1;
    }
    if (!estimate->m_simulated && !m_creaturePlacement
            && chooseSpellAction(currentArmy, actionValue, estimate))
        return 1;
    if (bestFlag) {
        *actionValue = bestTime;
        if (bestHex == currentArmy->m_gridIndex)
            return 0;
        moveToward(currentArmy, bestHex, enemyAttacks,
                    static_cast<unsigned char>(!estimate->m_simulated
                                               && bestTroops > 1));
        return 1;
    }

    if (bestEnemy == 0) {
        if (m_fortificationLevel > 0 && m_currentSide == 0) {
            long hex = g_castleWallColumns[currentArmy->m_gridIndex / 17];
            while (hex > currentArmy->m_gridIndex) {
                const pathCell* cell = g_searchArray->getHex(hex);
                if (cell->m_visited) {
                    if (!isInMoat(hex, 0)) {
                        if (!(currentArmy->m_monInfo.m_attributes & 1))
                            break;
                        if (!isInMoat(hex + (currentArmy->m_facing ? 1 : -1), 0))
                            break;
                    }
                }
                hex--;
            }
            if (hex > currentArmy->m_gridIndex) {
                moveToward(currentArmy, hex, enemyAttacks, 0);
                return 1;
            }
            m_nextAction = 3;
            return 1;
        }
    }
    if (bestEnemy == 0
            || (bestValue < 0 && bestValue < bestTime
                && !(currentArmy->is(1u << 22))
                && (g_game->m_setup.m_difficulty > 0 || m_sideIsAi[side])
                && hasRangedAdvantage(estimate))) {
        *actionValue = 0;
        if (!estimate->m_simulated
                && getAreaEffect(estimate->m_enemyGroup, currentArmy,
                                   dangerousEnemies,
                                   estimate) == 0
                && attemptShooterDefense(currentArmy, g_searchArray, estimate))
            return 1;
        if (bestEnemy == 0) {
            if (!estimate->m_simulated
                    && chooseToRun(currentArmy, enemyAttacks, g_searchArray))
                return 1;
            return 0;
        }
    }

    *actionValue = bestValue;
    if (bestTroops <= 1 && !m_creaturePlacement) {
        m_nextActionExtra = bestHex;
        m_nextAction = 6;
        m_nextActionGridIndex = bestEnemy->m_gridIndex;
        return 1;
    }
    if (bestHex == currentArmy->m_gridIndex)
        return 0;
    moveToward(currentArmy, bestHex, enemyAttacks,
                static_cast<unsigned char>(!estimate->m_simulated
                                           && bestTroops > 1));
    return 1;
}

VA(0x00421f80, 0xD5)  // dc 0x26ee0
long combatManager::chooseMeleeAction(const army* currentArmy, unsigned char teleport, unsigned char simulated, long side)
{
    type_AI_combat_parameters data(this, side);
    data.m_simulated = simulated;
    findMoveOrder(0);
    findAITargets(side, currentArmy, 1, &data, 0);
    long actionValue = 0;
    if (chooseMeleeTarget(currentArmy, teleport, &actionValue, &data))
        return actionValue;
    if (!simulated && chooseSpellAction(currentArmy, &actionValue, &data))
        return actionValue;
    if (!m_inSecondPhase && (g_game->m_setup.m_difficulty >= 2 || m_sideIsAi[side])) {
        m_nextAction = 8;
        return 0;
    }
    m_nextAction = 3;
    return 0;
}

VA(0x00422060, 0x18E)  // dc 0x26fa8
void combatManager::placeShooter(const army* currentArmy)
{
    long bestHex;
    long bestOpenHexes;
    long newHex;
    if (!currentArmy->getSpeed() ||
        (!g_game->m_setup.m_difficulty && !m_sideIsAi[m_currentSide])) {
        m_nextAction = 8;
        return;
    }
    bestHex = currentArmy->m_gridIndex;
    bestOpenHexes = 100;
    g_searchArray->seedCombatPosition(currentArmy, m_currentSide, 127,
                                      m_creaturePlacement, -1);
    for (newHex = 0; newHex < COMBAT_GRID_CELLS; newHex++) {
        if (!validHex(newHex) || isOutsidePlacementBoundry(m_currentSide, newHex))
            continue;
        if (!g_searchArray->getHex(newHex)->m_visited)
            continue;
        long value = 0;
        for (long dir = 0; dir < 8; dir++) {
            if (dir >= 6 && !currentArmy->is(1u << 0))
                continue;
            long adjacent = currentArmy->getAdjacentHex(newHex, dir);
            if (!validHex(adjacent))
                continue;
            army* other = m_cells[adjacent].getArmy();
            if (other != 0 && other != currentArmy) {
                if (other->is(1u << 2))
                    value = 1000;
            } else {
                value++;
            }
        }
        if (value > bestOpenHexes)
            continue;
        if (value == bestOpenHexes && newHex != currentArmy->m_gridIndex)
            continue;
        bestHex = newHex;
        bestOpenHexes = value;
    }
    if (bestHex == currentArmy->m_gridIndex) {
        m_nextAction = 8;
        return;
    }
    m_nextAction = 2;
    m_nextActionGridIndex = bestHex;
    return;
}

// E:\gamedcs\ai.cpp:2272
// The TU's entry point: `ret 4` for the DC count, 208 B against 200,
// its only caller is OUTSIDE this span (0x477ee0), and its callees are
// the choose_shooter_action and choose_melee_action slots - the two
// arms a per-stack AI turn dispatches to.

// whichGroup IS DEAD. The body has no frame at all (two pushes, no
// ebp) and never reads [esp+0xc]; the side it dispatches on comes from
// currentSide, and the stack from get_current_army(). Transcribed
// faithfully - retail's own unused parameter.

// The 2-vs-3 arm is dead too: `action` can only be 1, 2 or 3 and only
// `== 1` is tested. Retail still computes the flier bit, which is why
// it is written out - a stack that is neither a shooter nor a ballista
// dispatches to choose_melee_action either way.

// Residual (98.5%): ONE instruction, an `and eax, 0xff` retail keeps
// between `not al` and `and eax, 1` and our CL folds away. It is the
// zero-extension of a byte-wide intermediate, and no spelling of that
// intermediate reproduces it - measured, all identical at 98.4697:
// `2 | ((unsigned char)~flying & 1)`, a two-statement `flying =
// ~flying;` then `2 | (flying & 1)`, `(flying & 1) ? 2 : 3`, the
// complement moved inside the cast, an intermediate `unsigned`
// variable, and routing it through the army::Is inline. Writing the
// mask twice by hand (`& 0xffu & 1`) is strictly WORSE (87.8) because
// VC6 then drops the `not` shape entirely, and `% 2` in place of `& 1`
// is worse again (81.4). Filed with the compiler-generation class.
// E:\gamedcs\ai.cpp:2272
// Residual (98.4697%): retail widens the complemented flying bit through a
// byte before the caller's mask - `shr eax / not al / AND EAX,0xFF / and eax,1
// / or al,2` against our `shr eax / not al / and eax,1 / or al,2`.  The extra
// zero-extension is what an `unsigned char`-returning attribute test emits;
// the Dreamcast public settles our declaration instead (`?Is@army@@QBA_NI@Z`,
// `_N` = bool), so the widening cannot come from this caller and the return
// type is not ours to change.  Measured and rejected, all byte-flat at
// 98.4697 unless noted: `static_cast<unsigned char>(Is(...)) ? 2 : 3`,
// `2 | (static_cast<unsigned char>(~Is(...)) & 1)`, a named `unsigned char`
// local holding Is(...), `2 | !Is(...)`, dropping the parentheses;
// `2 + !Is(...)` falls to 97.5510 and `3 - (Is(...) & 1)` to 90.9694.
VA(0x004221f0, 0xD0)  // anchor-callee, dc 0x27138
void combatManager::doCompAI(int whichGroup)
{
    m_lastMovedArmy = 0;
    turnOffHighlighter(1);
    army* currentArmy = getCurrentArmy();
    currentArmy->m_side = -1;
    currentArmy->m_slot = -1;
    long action;
    if (currentArmy->canShoot(0)
            || currentArmy->m_creatureType == CREATURE_BALLISTA)
        action = 1;
    else
        action = (currentArmy->is(1u << 1)) ? 2 : 3;
    m_nextAction = 3;
    if (action == 1) {
        if (m_creaturePlacement)
            placeShooter(currentArmy);
        else
            chooseShooterAction(currentArmy, 0, m_currentSide);
    } else {
        chooseMeleeAction(currentArmy, 0, 0, m_currentSide);
    }
}

// THIS FUNCTION IS THE PROOF THAT army+0x10/+0x14 ARE THE CHOSEN
// TARGET, not the stack's own identity: its first two statements copy
// the TARGET's combatSide (+0xf4) and bitIndex (+0xf8) into them, and
// it clears both back to -1 the moment it decides to walk instead of
// strike. army.h still calls them `side` and `slot` on ValidAttack's
// reading (the target hexcell's armySide/armySlot compare against
// them) - which is the same fact seen from the other end, since what
// ValidAttack checks is that the hex still holds the chosen target.
// The rename belongs to a lane that owns army.h's other consumers.

// Three outcomes, keyed off the path length: no path at all is order
// 12, a one-cell path (already adjacent) is order 6 aimed from where
// the stack stands, and anything longer is order 6 aimed at the first
// step - unless the target is further away than one turn's movement,
// in which case the order is dropped and move_toward walks instead.

VA(0x004222c0, 0x175)  // dc 0x27200
void combatManager::berserkAttack(army* currentArmy, const army* target)
{
    currentArmy->m_side = target->m_combatSide;
    currentArmy->m_slot = target->m_bitIndex;
    long hex = target->m_gridIndex;
    if (hex >= 0 && hex < COMBAT_GRID_CELLS
            && (hex % COMBAT_GRID_ROW_STRIDE == 0
                || hex % COMBAT_GRID_ROW_STRIDE == COMBAT_GRID_LAST_COLUMN)
            && (target->m_monInfo.m_attributes & 1))
        hex = target->getSecondGridIndex();
    g_searchArray->findCombatPath(currentArmy, -1, hex, m_creaturePlacement,
                                  127, -1);
    if (g_searchArray->getPathSteps() == 0) {
        m_nextAction = 12;
        return;
    }
    if (g_searchArray->getPathSteps() == 1) {
        m_nextAction = 6;
        m_nextActionExtra = currentArmy->m_gridIndex;
        m_nextActionGridIndex = target->m_gridIndex;
        if (target->m_combatSide == currentArmy->m_combatSide)
            m_playDoh[target->m_combatSide] = 1;
        return;
    }
    long step = g_searchArray->getStepCell(1)->m_point.m_x;
    const pathCell* cell = g_searchArray->getHex(target->m_gridIndex);
    if (cell->m_cost > currentArmy->getSpeed()) {
        currentArmy->m_side = -1;
        currentArmy->m_slot = -1;
        moveToward(currentArmy, step, 0, 0);
        return;
    }
    m_nextAction = 6;
    m_nextActionExtra = step;
    m_nextActionGridIndex = target->m_gridIndex;
    if (target->m_combatSide == currentArmy->m_combatSide)
        m_playDoh[target->m_combatSide] = 1;
}

// An earlier operand-order probe using the canonical selector measured
// 99.77% because of a reversed cmp. That does not establish a separate
// helper returning references to its own by-value arguments; the
// includes.h min wrapper owns the copies and returns the selected value.
VA(0x00422440, 0x99)  // dc 0x27318
long combatManager::computeFireShieldDamage(long damage, const army* attacker, const army* target, long targetHits) const
{
    if (!target->m_spellInfluence[29] && target->m_creatureType != CREATURE_EFREET_SULTAN)
        return 0;
    unsigned char fireImmune = static_cast<unsigned char>(
        static_cast<unsigned>(attacker->m_monInfo.m_attributes) >> 14);
    if (fireImmune & 1)
        return 0;
    damage = static_cast<long>(target->getFireShieldStrength()
                               * min(targetHits, damage));
    hero* targetHero = attacker->getController();
    hero* castingHero = target->getController();
    return modifySpellDamage(damage, SPELL_FIRE_SHIELD, castingHero,
                             targetHero, attacker, 0);
}

// E:\gamedcs\ai.cpp:2397. Canonical static single-exchange scorer;
// retail expands all three melee call sites. DC line 2409 tests ranged
// first, then breath_attack, before computing fire-shield retaliation.
// Keeping that meaningful ranged guard is byte-flat for these melee calls,
// which all pass zero, but preserves the helper's recovered semantics.
static void simulateSimpleAttack(army* currentArmy, army* target,
                                   long distance, unsigned char ranged,
                                   unsigned char breathAttack)
{
    long hits = currentArmy->getTotalHitPoints(1);
    if (hits <= 0)
        return;
    long damage = aiGetAttackDamage(*(currentArmy), hits, *(target), ranged, distance);
    if (!ranged && !breathAttack) {
        long targetHits = target->getTotalHitPoints(1);
        long burn = g_combatManager->computeFireShieldDamage(
            damage, currentArmy, target, targetHits);
        if (burn > 0)
            currentArmy->setAIExpectedDamage(
                currentArmy->getAIExpectedDamage() + burn);
    }
    target->setAIExpectedDamage(target->getAIExpectedDamage() + damage);
}

VA(0x004224e0, 0x2B4)  // dc 0x2746c
void combatManager::simulateMeleeAttack(army* currentArmy, long hex,
                                          army* target, long enemyHex,
                                          long ourGroup)
{
    if (currentArmy->is(1u << 19)) {
        long directions = currentArmy->getMultiHeadDirections(hex, target,
                                                                  enemyHex);
        long hit = 0;
        long direction = 7;
        do {
            if (!(directions & (1 << direction)))
                continue;
            long adjacent = currentArmy->getAdjacentHex(hex, direction);
            if (!validHex(adjacent))
                continue;
            army* victim = m_cells[adjacent].getArmy();
            if (!victim)
                continue;
            long bit = 1 << victim->m_bitIndex;
            if (hit & bit)
                continue;
            if (victim->m_combatSide == ourGroup)
                continue;
            hit |= bit;
            simulateSimpleAttack(currentArmy, victim, 0, 0, 0);
        } while (direction-- > 0);
        return;
    }

    simulateSimpleAttack(currentArmy, target,
                           g_searchArray->getHex(hex)->m_cost, 0, 0);

    if (currentArmy->is(1u << 3)) {
        long direction = currentArmy->getAttackDirection(hex, target,
                                                            enemyHex);
        long behindHex = currentArmy->getAdjacentHex(hex, direction);
        behindHex = currentArmy->getAdjacentCellIndex(behindHex, direction);
        if (!validHex(behindHex))
            return;
        army* behind = m_cells[behindHex].getArmy();
        if (!behind || behind == target)
            return;
        simulateSimpleAttack(currentArmy, behind, 0, 0, 1);
    }
}

VA(0x004227a0, 0xDB)  // dc 0x275e8
void combatManager::simulateMeleeAttack(army* currentArmy, army* target,
                                          long ourGroup)
{
    long hex = m_nextActionExtra;
    if (hex < 0 || hex >= COMBAT_GRID_CELLS)
        return;

    long hitPoints = target->getTotalHitPoints(0);
    simulateMeleeAttack(currentArmy, hex, target, target->m_gridIndex,
                          ourGroup);

    unsigned char noRetaliation = static_cast<unsigned char>(
        static_cast<unsigned>(currentArmy->m_monInfo.m_attributes) >> 16);
    if (!(noRetaliation & 1) && !target->m_spellInfluence[70]
        && target->m_retaliationCount > 0
        && (g_game->m_setup.m_difficulty > 0 || m_sideIsAi[ourGroup]))
        simulateMeleeAttack(target, target->m_gridIndex, currentArmy, hex,
                              1 - ourGroup);

    unsigned char doubleAttack = static_cast<unsigned char>(
        static_cast<unsigned>(currentArmy->m_monInfo.m_attributes) >> 15);
    if ((doubleAttack & 1) && target->m_aiExpectedDamage < hitPoints)
        simulateMeleeAttack(currentArmy, hex, target, target->m_gridIndex,
                              ourGroup);
}

VA(0x00422880, 0x1B5)  // dc 0x27698
long combatManager::simulateActions(std::vector<army*>& list, long i,
                                     long ourGroup)
{
    type_AI_combat_parameters data(this, ourGroup);

    for (; i < list.size(); i++) {
        army* currentArmy = list[i];
        if (currentArmy->isIncapacitated()
            || (static_cast<unsigned char>(
                    static_cast<unsigned>(currentArmy->m_monInfo.m_attributes) >> 21)
                & 1)
            || currentArmy->m_creatureType == CREATURE_FIRST_AID_TENT
            || currentArmy->m_creatureType == CREATURE_AMMO_CART
            || currentArmy->m_spellInfluence[59]
            || currentArmy->m_creatureType == CREATURE_CATAPULT
            || currentArmy->getTotalHitPoints(1) == 0)
            continue;
        if (currentArmy->getControllingSide() != ourGroup)
            return i;

        unsigned char shooting = currentArmy->canShoot(0);
        if (shooting) {
            chooseShooterAction(currentArmy, 1, ourGroup);
            if (m_nextAction != AI_ORDER_SHOOT)
                continue;
        } else {
            chooseMeleeAction(currentArmy, 0, 1, ourGroup);
            if (m_nextAction != AI_ORDER_MOVE_AND_ATTACK)
                continue;
        }

        long hex = m_nextActionGridIndex;
        if (hex < 0 || hex >= COMBAT_GRID_CELLS)
            continue;
        army* target = m_cells[hex].getArmy();
        if (!target)
            continue;
        if (shooting) {
            long hits = currentArmy->getTotalHitPoints(1);
            if (hits <= 0)
                continue;
            long damage = aiGetAttackDamage(*(currentArmy), hits, *(target), 1, 0);
            target->setAIExpectedDamage(target->m_aiExpectedDamage
                                           + damage);
        } else {
            simulateMeleeAttack(currentArmy, target, ourGroup);
        }
    }
    return i;
}

VA(0x00422a40, 0xD8)  // dc 0x277f4
void combatManager::simulateCombat(long ourGroup, unsigned char checkingSurrender)
{
    std::vector<army*> order;
    long saved3c = m_nextAction;
    long saved40 = m_nextActionExtra;
    long saved44 = m_nextActionGridIndex;
    long saved48 = m_nextActionGridIndex2;

    findMoveOrder(&order);
    for (unsigned i = 0; i < order.size(); i++)
        order[i]->setAIExpectedDamage(0);
    long stoppedAt = simulateActions(order, 0, ourGroup);
    if (checkingSurrender)
        simulateActions(order, stoppedAt, 1 - ourGroup);

    m_nextAction = saved3c;
    m_nextActionExtra = saved40;
    m_nextActionGridIndex = saved44;
    m_nextActionGridIndex2 = saved48;
}

// E:\gamedcs\ai.cpp:2608
VA(0x00422b20, 0x278)  // anchor-caller(choose_shooter_action/choose_melee_action) + anchor-callee(SeedCombatPosition), dc 0x27888
void combatManager::findAITargets(long ourGroup, const army* currentArmy,
                                    unsigned char meleeOnly,
                                    const type_AI_combat_parameters* data,
                                    searchArray* currentSearchArray)
{
    long enemyGroup = 1 - ourGroup;
    if (currentSearchArray == 0)
        currentSearchArray = g_searchArray;

    for (long i = 0; i < m_numArmies[ourGroup]; i++) {
        army* ours = &m_armies[ourGroup][i];
        m_armies[ourGroup][i].m_aiTarget = 0;
        m_armies[ourGroup][i].m_aiTargetValue = 0;
        m_armies[ourGroup][i].m_aiPossibleTargets = 0;
        m_armies[ourGroup][i].m_aiTargetTime = 0;
        if (static_cast<unsigned char>(
                static_cast<unsigned>(ours->m_monInfo.m_attributes) >> 21)
            & 1)
            continue;
        if ((static_cast<unsigned char>(
                 static_cast<unsigned>(ours->m_monInfo.m_attributes) >> 6)
             & 1)
            && ours->m_creatureType != CREATURE_BALLISTA)
            continue;
        if (data->m_simulated && ours->getTotalHitPoints(1) == 0)
            continue;
        if (ours->m_spellInfluence[70] > 1)
            continue;
        if (ours->m_spellInfluence[62] > 1)
            continue;
        if (ours->m_spellInfluence[60])
            continue;
        if (ours->m_spellInfluence[59])
            continue;
        if (ours == currentArmy)
            continue;

        unsigned char shooter = ours->canShoot(0);
        if (shooter) {
            if (meleeOnly)
                continue;
        } else if (meleeOnly) {
            currentSearchArray->seedCombatPosition(ours, ourGroup, ours->getSpeed(),
                                             m_creaturePlacement, -1);
        } else {
            currentSearchArray->seedCombatPosition(ours, ourGroup, 0x7f,
                                             m_creaturePlacement, -1);
        }

        for (long j = 0; j < m_numArmies[enemyGroup]; j++) {
            army* theirs = &m_armies[enemyGroup][j];
            if (theirs->m_creatureType == CREATURE_ARROW_TOWER)
                continue;
            if (meleeOnly && theirs->m_expectedMoveOrder >= ours->m_expectedMoveOrder)
                continue;
            if (!shooter && !m_cells[theirs->m_gridIndex].m_validMove)
                continue;
            if (data->m_simulated && theirs->getTotalHitPoints(1) == 0)
                continue;

            long time;
            if (shooter) {
                time = 1;
            } else {
                time = currentSearchArray->getHex(theirs->m_gridIndex)->m_cost;
                if (ours->getSpeed() == 0 && time > 0)
                    continue;
            }
            long effect;
            if (time > ours->getSpeed())
                effect = data->getSimpleAttackEffect(*(ours), *(theirs), shooter, 0);
            else
                effect = data->getSimpleAttackEffect(*(ours), *(theirs), shooter, time);
            ours->considerAttack(theirs, effect, time);
        }
    }
}

VA(0x00422da0, 0x1AD)  // dc 0x27b18
unsigned char combatManager::doSpellAI()
{
    m_nextAction = 0;
    if (m_spellsCast[m_currentSide])
        return 0;
    if (m_creaturePlacement)
        return 0;
    if (static_cast<unsigned char>(
            static_cast<unsigned>(m_armies[m_actingSide][m_actingSlot].m_monInfo.m_attributes)
            >> 6)
        & 1)
        return 0;
    if (m_playerIds[m_currentSide] >= 0
        && g_game->isHuman(m_playerIds[m_currentSide])
        && !((m_autoCombatOn || g_unk691209)
             && g_unnamed698758.m_combatAutoSpells)
        && !static_cast<const combatManager*>(this)->isQuickCombat())
        return 0;
    long side = m_currentSide;
    if (m_onAntiMagicGarrison)
        return 0;
    if (!m_heroes[side])
        return 0;
    if (!m_heroes[side]->isWieldingArtifact(0))
        return 0;
    if (m_heroes[0] && m_heroes[0]->isWieldingArtifact(0x7e))
        return 0;
    if (m_heroes[1] && m_heroes[1]->isWieldingArtifact(0x7e))
        return 0;

    type_AI_spellcaster caster(this, m_currentSide, 0);
    if (caster.castSpell(aiCheckRetreat()))
        return 1;
    m_nextAction = 0;
    return 0;
}

#if 0  // @carcass

// E:\gamedcs\ai_spellvalue.h:124
DC_ONLY(0x27c74, 0x4)
void type_spellvalue::setStackValue(long arg)
{
    // @stub
}

// E:\gamedcs\Army.h:718
DC_ONLY(0x27c78, 0x24)
bool army::canCastResurrect() const
{
    // @stub
}

// E:\gamedcs\Army.h:736
DC_ONLY(0x27c9c, 0x30)
int army::offsetToFront(int direction)
{
    // @stub
}

// E:\gamedcs\Army.h:752
DC_ONLY(0x27ccc, 0x16)
void army::clearAIValues()
{
    // @stub
}

// E:\gamedcs\Army.h:765
DC_ONLY(0x27ce4, 0xE)
bool army::is(unsigned attribute)
{
    // @stub
}

// E:\gamedcs\Army.h:770
DC_ONLY(0x27cf4, 0x8)
long army::getAIExpectedDamage()
{
    // @stub
}

// E:\gamedcs\Army.h:775
DC_ONLY(0x27cfc, 0x8)
const army* army::getAITarget()
{
    // @stub
}

// E:\gamedcs\Army.h:780
DC_ONLY(0x27d04, 0x8)
long army::getAITargetValue()
{
    // @stub
}

// E:\gamedcs\Army.h:785
DC_ONLY(0x27d0c, 0x28)
long army::getAITargetTime()
{
    // @stub
}

// E:\gamedcs\Army.h:790
DC_ONLY(0x27d34, 0x8)
long army::getAIPossibleTargets()
{
    // @stub
}

// E:\gamedcs\Army.h:795
DC_ONLY(0x27d3c, 0x8)
int army::getOwningSide()
{
    // @stub
}

// E:\gamedcs\Army.h:800
DC_ONLY(0x27d44, 0x30)
int army::getControllingSide()
{
    // @stub
}

// E:\gamedcs\Army.h:820
DC_ONLY(0x27d74, 0x12)
long army::getSpellTime(SpellID spell)
{
    // @stub
}

// E:\gamedcs\Army.h:830
DC_ONLY(0x27d88, 0x14)
bool army::isActive()
{
    // @stub
}

// E:\gamedcs\Army.h:840 - dc 0x27d9c, promoted to VA(0x0041f380) above.

// E:\gamedcs\Army.h:847
DC_ONLY(0x27dd8, 0x44)
bool army::canRetaliate(const army* attacker)
{
    // @stub
}

// E:\gamedcs\Army.h:855
DC_ONLY(0x27e1c, 0x54)
bool army::cannotAttack()
{
    // @stub
}

// E:\gamedcs\Army.h:864
DC_ONLY(0x27e70, 0x1C)
long army::getAdjacentHex(long direction)
{
    // @stub
}

// E:\gamedcs\hero.h:965
DC_ONLY(0x27e8c, 0x10)
const type_artifact* hero::getArtifact(TArtifactSlot slot)
{
    // @stub
}

// E:\gamedcs\hero.h:970
DC_ONLY(0x27e9c, 0x10)
const type_artifact* hero::getBackpack(long slot)
{
    // @stub
}

// E:\gamedcs\cmbtmgr.h:1150
DC_ONLY(0x27eac, 0x1C)
int combatManager::TWallTarget::getBlockedHex()
{
    // @stub
}

// E:\gamedcs\cmbtmgr.h:1460
DC_ONLY(0x27ec8, 0x12)
unsigned char combatManager::validHex(int iHex)
{
    // @stub
}

// E:\gamedcs\cmbtmgr.h:1473
DC_ONLY(0x27edc, 0x20)
long combatManager::getWallStrength(TWallTargetId target)
{
    // @stub
}

// E:\gamedcs\cmbtmgr.h:1478
DC_ONLY(0x27efc, 0x2C)
army* combatManager::getCurrentArmy()
{
    // @stub
}

// E:\gamedcs\cmbtmgr.h:1494
DC_ONLY(0x27f28, 0xC)
// Before normalization (function): combatManager::is_in_second_phase.
unsigned char combatManager::isInSecondPhase()
{
    // @stub
}

// E:\gamedcs\cmbtmgr.h:1513
DC_ONLY(0x27f34, 0x18)
int combatManager::gridY(int index)
{
    // @stub
}

// E:\gamedcs\cmbtmgr.h:1519
DC_ONLY(0x27f4c, 0x18)
int combatManager::gridX(int index)
{
    // @stub
}

// E:\gamedcs\cmbtmgr.h:1525
DC_ONLY(0x27f64, 0x3C)
unsigned char combatManager::inInvisibleColumn(int index)
{
    // @stub
}

// E:\gamedcs\cmbtmgr.h:1542
DC_ONLY(0x27fa0, 0x34)
combatManager::TObstacle* combatManager::getObstacle(int index)
{
    // @stub
}

// E:\gamedcs\ai_tactical.h:82
DC_ONLY(0x27fd4, 0x4)
// Before normalization (function): type_AI_combat_parameters::get_enemy_group.
long type_AI_combat_parameters::getEnemyGroup()
{
    // @stub
}

// E:\gamedcs\ai_tactical.h:87
DC_ONLY(0x27fd8, 0x4)
long type_AI_combat_parameters::getGroup()
{
    // @stub
}

// E:\gamedcs\ai_tactical.h:471
DC_ONLY(0x27fdc, 0x4)
long type_AI_attack_hex_chooser::getAttackTime()
{
    // @stub
}

// E:\gamedcs\ai_tactical.h:476
DC_ONLY(0x27fe0, 0x4)
long type_AI_attack_hex_chooser::get_best_hex()
{
    // @stub
}

// E:\gamedcs\ai_tactical.h:481
DC_ONLY(0x27fe4, 0x4)
long type_AI_attack_hex_chooser::get_hex_value()
{
    // @stub
}

// E:\gamedcs\FindPath.h:194
DC_ONLY(0x27fe8, 0x16)
pathCell* searchArray::getHex(long x)
{
    // @stub
}

// E:\gamedcs\FindPath.h:226
DC_ONLY(0x28000, 0x18)
const pathCell* searchArray::getStepCell(long i)
{
    // @stub
}

// E:\gamedcs\FindPath.h:242
DC_ONLY(0x28018, 0xA)
unsigned char searchArray::isMoat(short index)
{
    // @stub
}

// E:\gamedcs\ai.cpp:597
DC_ONLY(0x28024, 0x2A)
unsigned char func_moves_before::operator()(const army* first, const army* second)
{
    // @stub
}

// E:\gamedcs\ai.cpp:1626
DC_ONLY(0x28050, 0x18)
void type_spellvalue::~type_spellvalue()
{
    // @stub
}

// E:\gamedcs\ai.cpp:1786
DC_ONLY(0x28068, 0x54)
void army::~army()
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x280bc, 0x28)
void std::vector<type_creature_value,std::allocator<type_creature_value> >::~vector<type_creature_value,std::allocator<type_creature_value> >()
{
    // @stub
}

// ..\stlport\stl_deque.h:680
DC_ONLY(0x280e4, 0x54)
void std::deque<enum SpellID,std::allocator<enum SpellID>,0>::~deque<enum SpellID,std::allocator<enum SpellID>,0>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x28138, 0x4)
void std::allocator<enum SpellID>::~allocator<enum SpellID>()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x2813c, 0x4)
army** std::vector<army *,std::allocator<army *> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0x28140, 0x4)
army** std::vector<army *,std::allocator<army *> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x28144, 0xC)
unsigned std::vector<army *,std::allocator<army *> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x28150, 0x20)
army** std::vector<army *,std::allocator<army *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x28170, 0x1C)
void std::vector<army *,std::allocator<army *> >::vector<army *,std::allocator<army *> >(const std::allocator<army* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x2818c, 0x28)
void std::vector<army *,std::allocator<army *> >::~vector<army *,std::allocator<army *> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x281b4, 0x3C)
void std::vector<army *,std::allocator<army *> >::push_back(army** __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x281f0, 0x4)
void std::allocator<army *>::allocator<army *>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x281f4, 0x4)
void std::allocator<army *>::~allocator<army *>()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x281f8, 0x24)
combatManager::TObstacle* std::vector<combatManager::TObstacle,std::allocator<combatManager::TObstacle> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x2821c, 0xC)
unsigned std::vector<long,std::allocator<long> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x28228, 0x20)
long* std::vector<long,std::allocator<long> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x28248, 0x1C)
void std::vector<long,std::allocator<long> >::vector<long,std::allocator<long> >(const std::allocator<long>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x28264, 0x28)
void std::vector<long,std::allocator<long> >::~vector<long,std::allocator<long> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x2828c, 0x3C)
void std::vector<long,std::allocator<long> >::push_back(const long* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0x282c8, 0x38)
void std::vector<long,std::allocator<long> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x28300, 0x4)
void std::allocator<long>::allocator<long>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x28304, 0x4)
void std::allocator<long>::~allocator<long>()
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x28308, 0x40)
void std::_Vector_base<type_creature_value,std::allocator<type_creature_value> >::~_Vector_base<type_creature_value,std::allocator<type_creature_value> >()
{
    // @stub
}

// ..\stlport\stl_deque.h:274
DC_ONLY(0x28348, 0x14)
void std::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >(const std::_Deque_iterator<enum* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x2835c, 0x2C)
void std::_Vector_base<army *,std::allocator<army *> >::_Vector_base<army *,std::allocator<army *> >(const std::allocator<army* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x28388, 0x30)
void std::_Vector_base<army *,std::allocator<army *> >::~_Vector_base<army *,std::allocator<army *> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x283b8, 0x4)
combatManager::TObstacle* std::vector<combatManager::TObstacle,std::allocator<combatManager::TObstacle> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x283bc, 0x4)
long* std::vector<long,std::allocator<long> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0x283c0, 0x4)
long* std::vector<long,std::allocator<long> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0x283c4, 0x3C)
long* std::vector<long,std::allocator<long> >::erase(long* __first, long* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x28400, 0x2C)
void std::_Vector_base<long,std::allocator<long> >::_Vector_base<long,std::allocator<long> >(const std::allocator<long>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x2842c, 0x30)
void std::_Vector_base<long,std::allocator<long> >::~_Vector_base<long,std::allocator<long> >()
{
    // @stub
}

// ..\stlport\stl_string.h:180
DC_ONLY(0x2845c, 0x18)
void std::_STL_alloc_proxy<type_creature_value *,type_creature_value,std::allocator<type_creature_value> >::~_STL_alloc_proxy<type_creature_value *,type_creature_value,std::allocator<type_creature_value> >()
{
    // @stub
}

// ..\stlport\stl_string.h:180
DC_ONLY(0x28474, 0x18)
void std::_STL_alloc_proxy<army * *,army *,std::allocator<army *> >::~_STL_alloc_proxy<army * *,army *,std::allocator<army *> >()
{
    // @stub
}

// ..\stlport\stl_string.h:180
DC_ONLY(0x2848c, 0x18)
void std::_STL_alloc_proxy<long *,long,std::allocator<long> >::~_STL_alloc_proxy<long *,long,std::allocator<long> >()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x284a4, 0x2C)
void std::_STL_alloc_proxy<type_creature_value *,type_creature_value,std::allocator<type_creature_value> >::deallocate(type_creature_value* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x284d0, 0x4)
void std::allocator<type_creature_value>::~allocator<type_creature_value>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x284d4, 0xC)
void std::_STL_alloc_proxy<army * *,army *,std::allocator<army *> >::_STL_alloc_proxy<army * *,army *,std::allocator<army *> >(const std::allocator<army* __a, army*** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x284e0, 0x2C)
void std::_STL_alloc_proxy<army * *,army *,std::allocator<army *> >::deallocate(army** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x2850c, 0xC)
void std::_STL_alloc_proxy<long *,long,std::allocator<long> >::_STL_alloc_proxy<long *,long,std::allocator<long> >(const std::allocator<long>* __a, long** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x28518, 0x2C)
void std::_STL_alloc_proxy<long *,long,std::allocator<long> >::deallocate(long* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x28544, 0x20)
void std::allocator<type_creature_value>::deallocate(type_creature_value* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x28564, 0x1C)
void std::allocator<enum SpellID>::deallocate(SpellID* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x28580, 0x1C)
void std::allocator<army *>::deallocate(army** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x2859c, 0x1C)
void std::allocator<long>::deallocate(long* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_deque.c:104
DC_ONLY(0x285b8, 0x50)
void std::_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>::~_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>()
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x28608, 0xCC)
void std::vector<army *,std::allocator<army *> >::_M_insert_overflow(army** __position, army** __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x286d4, 0xCC)
void std::vector<long,std::allocator<long> >::_M_insert_overflow(long* __position, const long* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_algo.h:636
DC_ONLY(0x287a0, 0x64)
void std::sort(army** __first, army** __last, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_algobase.h:79
DC_ONLY(0x28804, 0xA)
void std::swap(army** __a, army** __b)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x28810, 0x30)
void std::destroy(type_creature_value* __first, type_creature_value* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x28840, 0x60)
void std::destroy(std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x288a0, 0x30)
void std::destroy(army** __first, army** __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x288d0, 0x28)
void std::construct(army** __p, army** __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x288f8, 0x30)
void std::destroy(long* __first, long* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x28928, 0x28)
void std::construct(long* __p, const long* __value)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x28950, 0x50)
long* std::copy(long* __first, long* __last, long* __result)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x289a0, 0x4)
std::allocator<type_creature_value>* std::__stl_alloc_rebind(std::allocator<type_creature_value>* __a, const type_creature_value* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x289a4, 0x4)
std::allocator<army* std::__stl_alloc_rebind(std::allocator<army* __a, army** __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x289a8, 0x4)
std::allocator<enum* std::__stl_alloc_rebind(std::allocator<enum* __a, const SpellID* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x289ac, 0x4)
std::allocator<long>* std::__stl_alloc_rebind(std::allocator<long>* __a, const long* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:238
DC_ONLY(0x289b0, 0x18)
void std::_STL_alloc_proxy<enum SpellID * *,enum SpellID *,std::allocator<enum SpellID> >::~_STL_alloc_proxy<enum SpellID * *,enum SpellID *,std::allocator<enum SpellID> >()
{
    // @stub
}

// ..\stlport\stl_alloc.h:238
DC_ONLY(0x289c8, 0x18)
void std::_STL_alloc_proxy<unsigned int,enum SpellID,std::allocator<enum SpellID> >::~_STL_alloc_proxy<unsigned int,enum SpellID,std::allocator<enum SpellID> >()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x289e0, 0x2C)
void std::_STL_alloc_proxy<enum SpellID * *,enum SpellID *,std::allocator<enum SpellID> >::deallocate(SpellID** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x28a0c, 0x28)
army** std::_STL_alloc_proxy<army * *,army *,std::allocator<army *> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x28a34, 0x28)
long* std::_STL_alloc_proxy<long *,long,std::allocator<long> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x28a5c, 0x24)
army** std::allocator<army *>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x28a80, 0x24)
long* std::allocator<long>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x28aa4, 0x1C)
void std::allocator<enum SpellID *>::deallocate(SpellID** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_deque.c:157
DC_ONLY(0x28ac0, 0x3C)
void std::_Deque_base<enum SpellID,std::allocator<enum SpellID>,0>::_M_destroy_nodes(SpellID** __nstart, SpellID** __nfinish)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x28afc, 0x38)
army** std::uninitialized_copy(army** __first, army** __last, army** __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x28b34, 0x38)
army** std::uninitialized_fill_n(army** __first, unsigned __n, army** __x)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x28b6c, 0x38)
long* std::uninitialized_copy(long* __first, long* __last, long* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x28ba4, 0x38)
long* std::uninitialized_fill_n(long* __first, unsigned __n, const long* __x)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x28bdc, 0x4)
army** std::value_type(army** __formal)
{
    // @stub
}

// ..\stlport\stl_algo.h:606
DC_ONLY(0x28be0, 0x16)
int std::__lg(int __n)
{
    // @stub
}

// ..\stlport\stl_algo.c:1102
DC_ONLY(0x28bf8, 0x9C)
void std::__introsort_loop(army** __first, army** __last, army** __formal, int __depth_limit, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:1068
DC_ONLY(0x28c94, 0x54)
void std::__final_insertion_sort(army** __first, army** __last, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x28ce8, 0x4)
type_creature_value* std::value_type(const type_creature_value* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x28cec, 0x1C)
void std::__destroy(type_creature_value* __first, type_creature_value* __last, type_creature_value* __formal)
{
    // @stub
}

// ..\stlport\stl_deque.h:413
DC_ONLY(0x28d08, 0x4)
SpellID* std::value_type(const std::_Deque_iterator<enum* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x28d0c, 0x54)
void std::__destroy(std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, SpellID* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x28d60, 0x1C)
void std::__destroy(army** __first, army** __last, army** __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x28d7c, 0x4)
long* std::value_type(const long* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x28d80, 0x1C)
void std::__destroy(long* __first, long* __last, long* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x28d9c, 0xC)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const long* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x28da8, 0x4)
int* std::distance_type(const long* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x28dac, 0x1E)
long* std::__copy(long* __first, long* __last, long* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x28dcc, 0x4)
std::allocator<enum* std::__stl_alloc_rebind(std::allocator<enum* __a, SpellID** __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x28dd0, 0x2C)
void std::_STL_alloc_proxy<unsigned int,enum SpellID,std::allocator<enum SpellID> >::deallocate(SpellID* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x28dfc, 0x1C)
army** std::__uninitialized_copy(army** __first, army** __last, army** __result, army** __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x28e18, 0x1C)
army** std::__uninitialized_fill_n(army** __first, unsigned __n, army** __x, army** __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x28e34, 0x1C)
long* std::__uninitialized_copy(long* __first, long* __last, long* __result, long* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x28e50, 0x1C)
long* std::__uninitialized_fill_n(long* __first, unsigned __n, const long* __x, long* __formal)
{
    // @stub
}

// ..\stlport\stl_algo.h:677
DC_ONLY(0x28e6c, 0x40)
void std::partial_sort(army** __first, army** __middle, army** __last, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:65
DC_ONLY(0x28eac, 0x94)
army** std::__median(army** __a, army** __b, army** __c, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:941
DC_ONLY(0x28f40, 0x70)
army** std::__unguarded_partition(army** __first, army** __last, army* __pivot, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:1020
DC_ONLY(0x28fb0, 0x40)
void std::__insertion_sort(army** __first, army** __last, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:1050
DC_ONLY(0x28ff0, 0x34)
void std::__unguarded_insertion_sort(army** __first, army** __last, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x29024, 0x30)
void std::__destroy_aux(type_creature_value* __first, type_creature_value* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x29054, 0x54)
void std::__destroy_aux(std::_Deque_iterator<enum __first, std::_Deque_iterator<enum __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x290a8, 0x30)
void std::__destroy_aux(army** __first, army** __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:116
DC_ONLY(0x290d8, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_deque.h:276
DC_ONLY(0x290dc, 0x4)
SpellID* std::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator*()
{
    // @stub
}

// ..\stlport\stl_deque.h:285
DC_ONLY(0x290e0, 0x1C)
std::_Deque_iterator<enum* std::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator++()
{
    // @stub
}

// ..\stlport\stl_deque.h:199
DC_ONLY(0x290fc, 0x30)
void std::_Deque_iterator_base<enum SpellID,std::_Buf_size_traits<enum SpellID,0> >::_M_increment()
{
    // @stub
}

// ..\stlport\stl_deque.h:233
DC_ONLY(0x2912c, 0x10)
void std::_Deque_iterator_base<enum SpellID,std::_Buf_size_traits<enum SpellID,0> >::_M_set_node(SpellID** __new_node)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x2913c, 0x3C)
army** std::__uninitialized_copy_aux(army** __first, army** __last, army** __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x29178, 0x3C)
army** std::__uninitialized_fill_n_aux(army** __first, unsigned __n, army** __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:60
DC_ONLY(0x291b4, 0x18)
long* std::__uninitialized_copy_aux(long* __first, long* __last, long* __result, __true_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:231
DC_ONLY(0x291cc, 0x18)
long* std::__uninitialized_fill_n_aux(long* __first, unsigned __n, const long* __x, __true_type __formal)
{
    // @stub
}

// ..\stlport\stl_algo.c:1443
DC_ONLY(0x291e4, 0x84)
void std::__partial_sort(army** __first, army** __middle, army** __last, army** __formal, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_algobase.h:107
DC_ONLY(0x29268, 0x30)
void std::iter_swap(army** __a, army** __b)
{
    // @stub
}

// ..\stlport\stl_algo.c:1000
DC_ONLY(0x29298, 0x54)
void std::__linear_insert(army** __first, army** __last, army* __val, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:1042
DC_ONLY(0x292ec, 0x34)
void std::__unguarded_insertion_sort_aux(army** __first, army** __last, army** __formal, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x29320, 0x1C)
void std::destroy(type_creature_value* __pointer)
{
    // @stub
}

// ..\stlport\stl_deque.h:345
DC_ONLY(0x2933c, 0xC)
unsigned char std::operator!=(const std::_Deque_iterator_base<enum* __x, const std::_Deque_iterator_base<enum* __y)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x29348, 0x1C)
void std::destroy(SpellID* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x29364, 0x1C)
void std::destroy(army** __pointer)
{
    // @stub
}

// ..\stlport\stl_algobase.h:502
DC_ONLY(0x29380, 0x18)
long* std::fill_n(long* __first, unsigned __n, const long* __value)
{
    // @stub
}

// ..\stlport\stl_heap.c:235
DC_ONLY(0x29398, 0x4C)
void std::make_heap(army** __first, army** __last, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x293e4, 0x4)
int* std::distance_type(army** __formal)
{
    // @stub
}

// ..\stlport\stl_heap.h:85
DC_ONLY(0x293e8, 0x34)
void std::__pop_heap(army** __first, army** __last, army** __result, army* __value, func_moves_before __comp, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.h:118
DC_ONLY(0x2941c, 0x3C)
void std::sort_heap(army** __first, army** __last, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_algobase.h:96
DC_ONLY(0x29458, 0x18)
void std::__iter_swap(army** __a, army** __b, army** __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0x29470, 0x50)
army** std::copy_backward(army** __first, army** __last, army** __result)
{
    // @stub
}

// ..\stlport\stl_algo.c:974
DC_ONLY(0x294c0, 0x40)
void std::__unguarded_linear_insert(army** __last, army* __val, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x29500, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x29504, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x29508, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_heap.c:218
DC_ONLY(0x2950c, 0x5C)
void std::__make_heap(army** __first, army** __last, func_moves_before __comp, army** __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.c:151
DC_ONLY(0x29568, 0x9C)
void std::__adjust_heap(army** __first, int __holeIndex, int __len, army* __value, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_heap.c:183
DC_ONLY(0x29604, 0x34)
void std::pop_heap(army** __first, army** __last, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x29638, 0xC)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, army** __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0x29644, 0x22)
army** std::__copy_backward(army** __first, army** __last, army** __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.c:78
DC_ONLY(0x29668, 0x70)
void std::__push_heap(army** __first, int __holeIndex, int __topIndex, army* __value, func_moves_before __comp)
{
    // @stub
}

// ..\stlport\stl_heap.c:173
DC_ONLY(0x296d8, 0x40)
void std::__pop_heap_aux(army** __first, army** __last, army** __formal, func_moves_before __comp)
{
    // @stub
}

#endif  // @carcass

// COMDAT pairing: vector<army*>::size - ai.obj's own copy, 19 B against
// the 19-byte emitted COMDAT and the only candidate of that size here.
VA_COMPGEN(0x00423110, 0x13, VECTOR_SIZE, army)

VA_COMPGEN(0x00423130, 0x209, VECTOR_INSERT, army)

// COMDAT pairing: std::_Sort<army*, func_moves_before>, agreement 0.973.
VA_COMPGEN(0x00423630, 0x186, STD_SORT, army_ptr_func_moves_before)

// COMDAT pairing: std::_Sort_0<army*, func_moves_before>, agreement 0.998.
VA_COMPGEN(0x00423340, 0x27F, STD_SORT_0, army_ptr_func_moves_before)

// COMDAT pairing: std::_Unguarded_partition<army*, func_moves_before>, 0.927.
VA_COMPGEN(0x00423820, 0x87, STD_UNGUARDED_PARTITION, army_ptr_func_moves_before)

// COMDAT pairing: std::_Unguarded_insert<army*, func_moves_before>, 0.976.
VA_COMPGEN(0x004237c0, 0x5E, STD_UNGUARDED_INSERT, army_ptr_func_moves_before)

// E:\gamedcs\ai.cpp:597, dc 0x28024
VA(0x004235c0, 0x44)  // retained comparator + CodeView identity
unsigned char func_moves_before::operator()(const army* a, const army* b)
{
    if (a->m_expectedMoveOrder > b->m_expectedMoveOrder)
        return true;
    if (a->m_expectedMoveOrder < b->m_expectedMoveOrder)
        return false;
    return a->m_bitIndex < b->m_bitIndex;
}
