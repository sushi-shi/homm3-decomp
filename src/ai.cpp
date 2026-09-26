#include "va.h"
#include "includes.h"

#include <algorithm>

#include "ai.h"

#include "ai_player.h"
#include "ai_spellvalue.h"
#include "ai_tactical.h"
#include "csprite.h"
#include "findpath.h"
#include "game.h"
#include "hero.h"
#include "misc.h"
#include "prefs.h"
#include "sample.h"
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
// Windows is exact with one value local shared by the two scans: each
// loop-local declaration gave the same instructions but cycled the five
// stack homes. Dreamcast records no value local, which is a coverage limit.
// The Mac comparison remains 97.9873%, with only second-scan register homes
// different; the shared value local leaves that comparison byte-flat.
// Mac Complete's 0+0x1f2b4 body calls the same defender-factor helper at
// 0+0x4fd04 (Air Shield, Petrify, hero defense factor) but divides by its
// returned factor in both scans. Windows retail uses FMUL and DC ai.cpp:61/89
// calls __muld. Keep that port difference at the expression itself. The
// shared retry condition, default-five arm, and result/best assignment order
// preserve the Windows byte match. Placing damage after estimate raises Mac to
// 97.5636% (921/944 bytes, all 14 calls in order). Reusing one scan index,
// consistent with DC's r11 across both loops, raises Mac to 97.9873%
// (925/944); the first scan is exact and the second only differs in its army
// pointer register. Naming a divided `damage` local
// lowered Mac to 95.3390% and was removed.

VA(0x0041e190, 0x2A8) MAC_ADDRESS(0x01f2b4, 0x3b0)  // order-map(DC ai.obj head) + anchor-callee find_AI_targets, dc 0x23450
int combatManager::chooseBallistaTarget(int targetGroup, int attackSkill, int averageDamage)
{
    long bestValue = 0;
    long result = -1;
    type_AI_combat_parameters estimate(this, 1 - targetGroup);
    double damage;
    long value;

    findAITargets(targetGroup, 0, 0, &estimate, 0);

    long i;
    { for (i = 0; i < m_numArmies[targetGroup]; i++) {
            army* currentArmy = &m_armies[targetGroup][i];
            if (currentArmy->is(creatureImmobilized))
                continue;
#ifdef HOMM3_TARGET_MAC
            value = static_cast<long>(
                averageDamage / currentArmy->computeDefenderDamageReduction(1));
#else
            damage = averageDamage;
            value = static_cast<long>(
                damage * currentArmy->computeDefenderDamageReduction(1));
#endif
            value = currentArmy->getLossCombatValue(
                estimate.m_lowestAttack, estimate.m_lowestDefense, 1, value,
                estimate.m_killsOnly);
            if (currentArmy->cannotAttack() || !currentArmy->getAITarget()
                    || currentArmy->getAITargetTime() > 5)
                value /= 5;
            else
                value /= currentArmy->getAITargetTime();
            if (value >= bestValue) {
                result = i;
                bestValue = value;
            }
        }
    }

    if (!estimate.m_killsOnly || (result >= 0 && bestValue > 0))
        return result;

    { for (i = 0; i < m_numArmies[targetGroup]; i++) {
                army* currentArmy = &m_armies[targetGroup][i];
                if (currentArmy->is(creatureImmobilized))
                    continue;
#ifdef HOMM3_TARGET_MAC
                value = static_cast<long>(
                    averageDamage / currentArmy->computeDefenderDamageReduction(1));
#else
                damage = averageDamage;
                value = static_cast<long>(
                    damage * currentArmy->computeDefenderDamageReduction(1));
#endif
                value = currentArmy->getLossCombatValue(
                    estimate.m_lowestAttack, estimate.m_lowestDefense, 1, value, 0);
                if (currentArmy->cannotAttack() || !currentArmy->getAITarget()
                        || currentArmy->getAITargetTime() > 5)
                    value /= 5;
                else
                    value /= currentArmy->getAITargetTime();
                if (value >= bestValue) {
                    result = i;
                    bestValue = value;
                }
            }
    }
    return result;
}

VA(0x0041e440, 0x129) MAC_ADDRESS(0x01f664, 0x210)  // dc 0x23750
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
            if (currentArmy->is(creatureImmobilized))
                continue;
            if (currentArmy->is(creatureFlyingArmy | creatureCatapult))
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
            if (enemyArmy->is(creatureImmobilized))
                continue;
            if (enemyArmy->m_creatureType == CREATURE_ARROW_TOWER)
                continue;
            if (!combatManager::inCastle(enemyArmy->m_gridIndex))
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

// The first hero guard returns 0 explicitly (DC ai.cpp:164..165 and Mac
// 0+0x1f8ac). The remaining guards share the bottom return in a nested-if
// pyramid. Written as thirteen early returns the
// body scores 60.44 with seventeen epilogues against retail's four; the
// pyramid alone is +24.22 and makes the branch census exact (54/54
// branches, 4/4 rets).

// DC's IsActive and Is calls remain canonical in the fight-value loop.
// A raw attribute-word carrier previously selected retail's test instruction,
// but bypassed that interface. Positive active-arm nesting and a consumed
// named done Boolean do not change the residual with the helpers restored.
// Keep the artifact-value result ahead of the by-value max so its argument
// copy dies in that arm; the final quotient likewise owns its float slot.
VA(0x0041e570, 0x546) MAC_ADDRESS(0x01f874, 0x7e4)  // order-map(DC ai.obj head) + anchor-callee failed_siege, dc 0x2389c
unsigned char combatManager::aiCheckRetreat()
{
    if (!m_heroes[m_currentSide])
        return 0;
    if ((m_sideIsAi[m_currentSide]
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
                    if (currentTown->hasBuilding(TAVERN_ID, true)) {
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
                        && m_defendingTown->hasBuilding(SPECIAL_BUILDING_ID, true)))) {
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
                        if (!currentArmy->is(creatureImmobilized)
                            && !currentArmy->is(creatureSiegeWeapon)
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
                                        if (!sideArmy->isActive())
                                            continue;
                                        long value =
                                            sideArmy->m_numTroops
                                            * sideArmy->m_monInfo.m_baseFightValue;
                                        if (!sideArmy->is(creatureDone))
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

                        // Mac 0+0x1fef4..0x1ff2c loads 0.16f and adds
                        // 0.06f, 0.05f, or 0.04f in the selected arm.
                        // VC6 also needs the third increment: 0.16f +
                        // 0.04f rounds to retail 0x3e4ccccc in single
                        // precision; the decimal 0.2f is 0x3e4ccccd.
                        float threshold = 0.16f;
                        if (combatValue > 10000)
                            threshold += 0.06f;
                        else if (combatValue > 5000)
                            threshold += 0.05f;
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

VA(0x0041eac0, 0xB8) MAC_ADDRESS(0x020058, 0x128)  // dc 0x23f2c
long combatManager::getTotalCombatValue(long side, long lowestAttack, long lowestDefense, unsigned char includeCripples) const
{
    long total = 0;
    const army* currentArmy = m_armies[side];
    for (long i = 0; i < m_numArmies[side]; i++, currentArmy++) {
        if (currentArmy->is(creatureImmobilized) || currentArmy->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (!includeCripples && currentArmy->cannotAttack())
            continue;
        total += currentArmy->getTotalCombatValue(lowestAttack, lowestDefense);
    }
    return total;
}

VA(0x0041eb80, 0x220) MAC_ADDRESS(0x020318, 0x2ac)  // dc 0x240e4
long combatManager::chooseShooterTarget(const army* currentArmy, type_AI_combat_parameters* data, long* bestValue) const
{
    long bestTarget = -1;
    long hex;
    long ourGroup = data->getGroup();
    long enemyGroup = data->getEnemyGroup();
    unsigned char isAreaEffect = 0;
    const army* bestArmy = 0;
    std::vector<army*> targets;

    if (currentArmy->m_creatureType == CREATURE_MAGOG
            || currentArmy->m_creatureType == CREATURE_LICH
            || currentArmy->m_creatureType == CREATURE_POWER_LICH)
        isAreaEffect = 1;

    for (long i = 0; i < m_numArmies[enemyGroup]; i++) {
        const army* target = &m_armies[enemyGroup][i];
        if (target->is(creatureImmobilized) || target->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (data->m_simulated && target->getTotalHitPoints(1) == 0)
            continue;

        hex = target->m_gridIndex;
        long value;
        if (isAreaEffect) {
            value = getAreaAttackValue(currentArmy, hex, ourGroup, data);
            if (target->is(creatureDoubleWide)) {
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

VA(0x0041eda0, 0xFD) MAC_ADDRESS(0x020180, 0x114)  // dc 0x2400c
long getAreaAttackValue(const army* currentArmy, long hex, long ourGroup, type_AI_combat_parameters* data)
{
    std::vector<army*> targets;
    long total = 0;
    g_combatManager->markAreaEffect(hex, 1, 1, targets);
    for (unsigned i = targets.size(); i-- != 0; ) {
        army* target = targets[i];
        if (currentArmy->is(creatureUndead) && target->is(creatureUndead)
                && target->m_gridIndex != hex
                && target->getSecondGridIndex() != hex)
            continue;
        if (target->getOwningSide() == ourGroup)
            total -= data->getSimpleAttackEffect(*(currentArmy), *(target), 1, 0);
        else
            total += data->getRangedAttackValue(*(currentArmy), *(target));
    }
    return total;
}

VA(0x0041eea0, 0x1B9) MAC_ADDRESS(0x0205c4, 0x3cc)  // dc 0x2429c
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

VA(0x0041f060, 0xD1) MAC_ADDRESS(0x020990, 0x124)  // dc 0x2452c
void combatManager::chooseShooterAction(const army* currentArmy, unsigned char simulated, long side)
{
    long bestValue = 0;
    type_AI_combat_parameters data(this, side);
    data.m_simulated = simulated;
    findAITargets(1 - side, 0, 0, &data, 0);
    long actionValue = chooseShooterTarget(currentArmy, &data, &bestValue);
    if (!simulated && currentArmy->is(creatureCatapult)
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
MAC_ADDRESS(0x020ab4, 0xe0)
static long getMoveOrder(const army* currentArmy)
{
    if (currentArmy->m_creatureType == CREATURE_FIRST_AID_TENT
            || currentArmy->m_creatureType == CREATURE_AMMO_CART)
        return -100000;
    if (currentArmy->getSpellTime(62) > 1 || currentArmy->getSpellTime(70) > 1)
        return -10000;
    if (currentArmy->is(creatureDone)
            || const_cast<army*>(currentArmy)->isIncapacitated())
        return currentArmy->getSpeed() - 1000;
    if (currentArmy->is(creatureWaiting) || g_combatManager->isInSecondPhase())
        return -currentArmy->getSpeed();
    return currentArmy->getSpeed();
}

VA(0x0041f140, 0x23F) MAC_ADDRESS(0x020b94, 0x2b4)  // dc 0x24694
void combatManager::findMoveOrder(std::vector<army*>* result)
{
    std::vector<army*> order;
    for (long side = 0; side < 2; side++) {
        for (long i = 0; i < m_numArmies[side]; i++) {
            army* currentArmy = &m_armies[side][i];
            if (currentArmy->m_creatureType == CREATURE_ARROW_TOWER)
                continue;
            if (currentArmy->is(creatureImmobilized))
                continue;
            currentArmy->m_expectedMoveOrder = getMoveOrder(currentArmy);
            order.push_back(currentArmy);
        }
    }
    std::sort(order.begin(), order.end(), func_moves_before());
    long wantSide = m_actingSide;
    for (unsigned i = 0; i < order.size(); i++) {
        if (order[i]->getOwningSide() != wantSide) {
            long key = order[i]->m_expectedMoveOrder;
            for (unsigned j = i + 1; j < order.size(); j++) {
                if (order[j]->m_expectedMoveOrder != key)
                    break;
                if (order[j]->getOwningSide() == wantSide) {
                    std::swap(order[i], order[j]);
                    break;
                }
            }
        }
        wantSide = 1 - order[i]->getOwningSide();
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

MAC_ADDRESS(0x020e48, 0x14c)
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

VA(0x0041f3b0, 0x1C2) MAC_ADDRESS(0x020f94, 0x15c)  // dc 0x24a34
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

VA(0x0041f580, 0x304) MAC_ADDRESS(0x0210f0, 0x3cc)  // dc 0x24b64
unsigned char combatManager::moveToward(const army* currentArmy, long targetHex, const long* enemyAttacks, unsigned char considerWaiting)
{
    if (!currentArmy->getSpellTime(72) && currentArmy->getSpeed()) {
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
            if (m_creaturePlacement || isInSecondPhase())
                considerWaiting = 0;
            if (g_game->m_setup.m_difficulty < 2 && !m_sideIsAi[currentArmy->getOwningSide()])
                considerWaiting = 0;
            if (enemyAttacks == 0) {
                considerWaiting = 0;
            } else {
                bestDanger = enemyAttacks[hex];
                if (currentArmy->is(creatureDoubleWide))
                    bestDanger = min(bestDanger,
                            enemyAttacks[hex
                                    + currentArmy->offsetToFront(-1)]);
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
                    if (!validHex(hex))
                        break;
                    const pathCell* cell = g_searchArray->getHex(hex);
                    if (cell->m_flightCost == 0) {
                        long secondHex = (currentArmy->is(creatureDoubleWide))
                                ? hex + currentArmy->offsetToFront(-1) : hex;
                        if (!(((step <= limit && committed) || considerWaiting)
                                    && enemyAttacks != 0)
                                || (enemyAttacks[hex] >= bestDanger
                                    && enemyAttacks[secondHex] >= bestDanger)) {
                            if (!m_creaturePlacement
                                    || !isOutsidePlacementBoundry(
                                            currentArmy->getOwningSide(), hex)) {
                                m_nextActionGridIndex = hex;
                                committed = 1;
                                if (enemyAttacks != 0) {
                                    bestDanger = enemyAttacks[hex];
                                    if (currentArmy->is(creatureDoubleWide))
                                        bestDanger = min(bestDanger,
                                                enemyAttacks[secondHex]);
                                }
                            }
                        }
                        if (!currentArmy->is(creatureFlyingArmy)) {
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

VA(0x0041f890, 0x8F) MAC_ADDRESS(0x0214bc, 0xd8)  // dc 0x24e5c
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

VA(0x0041f920, 0x234) MAC_ADDRESS(0x021594, 0x254)  // dc 0x24ef4
long combatManager::getAreaEffect(long side, const army* ourArmy, long markedEnemies, const type_AI_combat_parameters* estimate) const
{
    long total = 0;
    const army* enemy = m_armies[side];
    for (long i = 0; i < m_numArmies[side]; i++, enemy++) {
        if ((markedEnemies & (1 << enemy->m_bitIndex)) == 0)
            continue;
        if (enemy->is(creatureFireballAttack) && enemy->canShoot(0))
            total += enemy->getAverageDamage(ourArmy, 1, enemy->m_numTroops,
                                               1, 0);
        if (enemy->is(creatureHasExtendedAttack))
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

// Original: get_enemy_attack_limit; ai.cpp:1000, dc 0x250e0
MAC_ADDRESS(0x0217e8, 0x78)
static long getEnemyAttackLimit(const army* ourArmy,
                                const type_AI_combat_parameters& estimate)
{
    long hitPoints = ourArmy->getTotalHitPoints(estimate.m_simulated);
    return -ourArmy->getLossCombatValue(
        estimate.m_lowestAttack, estimate.m_lowestDefense,
        ourArmy->canShoot(0), hitPoints, 0);
}

VA(0x0041fb60, 0x1F6) MAC_ADDRESS(0x021860, 0x23c)  // dc 0x25124
void combatManager::markFriendlyArmies(const army* ourArmy, long* enemyAttacks, long markedEnemies, const type_AI_combat_parameters* estimate) const
{
    long enemySide = estimate->getEnemyGroup();
    long areaEffect = getAreaEffect(enemySide, ourArmy, markedEnemies,
                                       estimate);
    unsigned char checked[COMBAT_GRID_CELLS];
    memset(checked, 0, COMBAT_GRID_CELLS);
    long floorValue = getEnemyAttackLimit(ourArmy, *estimate);
    const army* friendly = m_armies[estimate->getGroup()];
    for (long i = 0; i < m_numArmies[estimate->getGroup()]; i++, friendly++) {
        if (friendly->is(creatureImmobilized))
            continue;
        if (friendly->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (friendly == ourArmy)
            continue;
        long meleeValue;
        if (friendly->is(creatureHasExtendedAttack)) {
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
        long count = (friendly->is(creatureDoubleWide)) ? 8 : 6;
        for (long direction = count; direction-- > 0; ) {
            long hex = friendly->getAdjacentHex(direction);
            if (!combatManager::validHex(hex))
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
            if (friendly->is(creatureHasExtendedAttack)) {
                long farHex = friendly->getAdjacentCellIndex(hex, direction);
                if (!combatManager::validHex(farHex))
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

// DC ai.cpp:1124 calls vector::clear at this helper's entry. Mac Complete's
// retained body at 0+0x21bc0 likewise zeros the result size before its first
// getSpeed/findAttackHexes calls. The result is a reference in DC CodeView.
MAC_ADDRESS(0x021bc0, 0x100)
static void findAttackHexes(const army* ourArmy, const army* enemy, const searchArray* currentSearchArray, std::vector<long>& result)
{
    result.clear();
    long sides = (ourArmy->is(creatureDoubleWide)) ? 8 : 6;
    findAttackHexes(ourArmy, ourArmy->m_gridIndex, 0, sides,
                      enemy->getSpeed(), currentSearchArray, &result);
    if (enemy->is(creatureDoubleWide)) {
        long secondHex = ourArmy->m_gridIndex - enemy->offsetToFront(-1);
        if (enemy->m_facing == 0)
            findAttackHexes(ourArmy, secondHex, 0, 3, enemy->getSpeed(),
                              currentSearchArray, &result);
        else
            findAttackHexes(ourArmy, secondHex, 3, 6, enemy->getSpeed(),
                              currentSearchArray, &result);
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

// The vector clear belongs to the four-argument helper above, as both DC
// ai.cpp:1124 and the retained Mac helper prove. An earlier caller-local
// `clear()` was only a temporary inlining model.

// Moving clear() into the DC/Mac-proven wrapper makes Complete VC6 retain
// vector<long>::erase at +0x148 and matches all 758 body bytes and 38 CFG
// blocks. The retail symbol is vector<int>::erase; both 32-bit template
// instantiations have the same emitted call bytes. DC locals place the vector
// below the two 187-byte marks; declaring priced, counted, then hexes gives
// the paired Mac body the same stack-object order and keeps Windows exact.
// Mac 0+0x21cc0 now has equal size, all ten calls at exact offsets, and
// 601/636 matching bytes. Remaining differences are a uniform four-byte
// stack-object displacement, one getHex null-path branch, two array-address
// register choices, and the vector destructor's nondeleting flag.
// E:\gamedcs\ai.cpp:1152
VA(0x0041fd60, 0x2F6) MAC_ADDRESS(0x021cc0, 0x27c)  // anchor-callee, dc 0x2544c
void combatManager::markMultiheadedEnemy(const army* ourArmy, const army* enemy, long* enemyAttacks, long limitValue, searchArray* currentSearchArray, type_AI_combat_parameters* estimate) const
{
    const army* other = m_armies[estimate->getGroup()];
    long value = -estimate->getSimpleAttackEffect(*(enemy), *(ourArmy), 0, 0);
    unsigned char priced[COMBAT_GRID_CELLS];
    unsigned char counted[COMBAT_GRID_CELLS];
    std::vector<long> hexes;
    memset(priced, 0, COMBAT_GRID_CELLS);
    for (long i = m_numArmies[estimate->getGroup()]; i-- > 0; other++) {
        if (other->is(creatureImmobilized))
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
        memset(counted, 0, COMBAT_GRID_CELLS);
        findAttackHexes(other, enemy, currentSearchArray, hexes);
        for (long j = hexes.size(); j-- > 0; ) {
            long hex = hexes[j];
            long directions = enemy->getMultiHeadDirections(
                    hex, other, other->m_gridIndex);
            long count = (enemy->is(creatureDoubleWide)) ? 8 : 6;
            for (long direction = count; direction-- > 0; ) {
                if ((directions & (1 << direction)) == 0)
                    continue;
                long target = enemy->getAdjacentHex(hex, direction);
                if (!validHex(target))
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

VA(0x00420060, 0x1FB) MAC_ADDRESS(0x021a9c, 0x124)  // dc 0x25308
void findAttackHexes(const army* ourArmy, long targetHex, long start, long stop, long limitCost, const searchArray* currentSearchArray, std::vector<long>* result)
{
    for (long direction = start; direction < stop; direction++) {
        long hex = ourArmy->getAdjacentHex(targetHex, direction);
        if (!combatManager::validHex(hex))
            continue;
        const pathCell* cell = currentSearchArray->getHex(hex);
        if (!cell->m_visited)
            continue;
        if (cell->m_cost > limitCost)
            continue;
        result->push_back(hex);
    }
}

VA(0x00420260, 0x368) MAC_ADDRESS(0x021fc0, 0x424)  // dc 0x256a0
void combatManager::markEnemyAttacks(const army* ourArmy, long* enemyAttacks, long* dangerousEnemies, type_AI_combat_parameters* estimate) const
{
    long side = estimate->getGroup();
    long enemySide = estimate->getEnemyGroup();
    long floorValue = getEnemyAttackLimit(ourArmy, *estimate);
    const army* enemy = m_armies[enemySide];
    *dangerousEnemies = 0;
    for (long i = 0; i < m_numArmies[enemySide]; i++, enemy++) {
        if (enemy->cannotAttack()
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
            if (friendly->is(creatureImmobilized))
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
            if (enemy->is(creatureMultiHeaded) && g_game->m_setup.m_difficulty >= 2
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
        if (enemy->is(creatureDoubleWide)) {
            long direction = enemy->m_facing ? 1 : 4;
            for (long hex = 0; hex < COMBAT_GRID_CELLS; hex++) {
                const pathCell* cell = g_searchArray->getHex(hex);
                if (!cell->m_visited)
                    continue;
                if (g_searchArray->isMoat(static_cast<short>(hex)))
                    continue;
                long adjacent = m_adjacentCells[hex][direction];
                if (!validHex(adjacent))
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

// DC's best_travel_time / best_hexes_covered are deliberately uninitialised:
// retail writes neither before the loop and both are only read once *best_hex
// is no longer -1.

// E:\gamedcs\ai.cpp:1357
VA(0x004205d0, 0x185) MAC_ADDRESS(0x0223e4, 0x1ec)  // linkorder, dc 0x25998
unsigned char combatManager::chooseDefenseHex(const army* currentArmy, const army* client, long* bestHex, long* openHexes, searchArray* currentSearchArray)
{
    long bestTravelTime;
    long bestHexesCovered;

    *openHexes = 0;
    *bestHex = -1;
    for (long direction = 0; direction < 8; direction++) {
        if (direction >= 6 && !client->is(creatureDoubleWide))
            continue;
        long hex = client->getAdjacentHex(direction);
        if (!combatManager::validHex(hex))
            continue;
        hexcell* cell = &m_cells[hex];
        army* occupant = cell->getArmy();
        if (occupant != 0 && occupant != currentArmy)
            continue;
        (*openHexes)++;
        const pathCell* path = currentSearchArray->getHex(hex);
        if (!path->m_visited)
            continue;
        long travelTime = m_creaturePlacement
                ? 1
                : currentSearchArray->getTravelTime(currentArmy, hex);
        long hexesCovered;
        if (currentArmy->is(creatureDoubleWide)
                && client->isAdjacent(
                    hex + currentArmy->offsetToFront(-1)))
            hexesCovered = 2;
        else
            hexesCovered = 1;
        if (*bestHex >= 0) {
            if (travelTime > bestTravelTime)
                continue;
            if (travelTime == bestTravelTime) {
                if (hexesCovered < bestHexesCovered)
                    continue;
                if (hexesCovered == bestHexesCovered) {
                    if (client->m_facing == 1) {
                        if (cell->m_refX < m_cells[*bestHex].m_refX)
                            continue;
                    } else if (cell->m_refX > m_cells[*bestHex].m_refX) {
                        continue;
                    }
                }
            }
        }
        *bestHex = hex;
        bestTravelTime = travelTime;
        bestHexesCovered = hexesCovered;
    }
    return static_cast<unsigned char>(*bestHex >= 0);
}

VA(0x00420760, 0x187) MAC_ADDRESS(0x0225d0, 0x1ac)  // dc 0x25b0c
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
        if (client->is(creatureImmobilized))
            continue;
        if (client->m_creatureType == CREATURE_ARROW_TOWER
                || client->m_creatureType == CREATURE_CATAPULT)
            continue;
        if (!client->canShoot(0))
            continue;
        if (client->is(creatureNoMeleePenalty))
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

VA(0x004208f0, 0x184) MAC_ADDRESS(0x02277c, 0x248)  // dc 0x25c80
unsigned char combatManager::chooseToRun(const army* ourArmy, const long* enemyAttacks, const searchArray* currentSearchArray)
{
    if (g_game->m_setup.m_difficulty < 2
        && !m_sideIsAi[ourArmy->getOwningSide()])
        return 0;

    long worstDanger = enemyAttacks[ourArmy->m_gridIndex];
    if (ourArmy->is(creatureDoubleWide)) {
        long secondHex = ourArmy->getSecondGridIndex();
        worstDanger = min(worstDanger, enemyAttacks[secondHex]);
    }

    if (worstDanger >= 0 || ourArmy->isIncapacitated()
        || ourArmy->getSpellTime(72))
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
        if (ourArmy->is(creatureDoubleWide)) {
            long secondHex = hex + ourArmy->offsetToFront(-1);
            danger = min(danger, enemyAttacks[secondHex]);
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

VA(0x00420a80, 0x264) MAC_ADDRESS(0x0229c4, 0x28c)  // dc 0x25df8
unsigned char combatManager::hasRangedAdvantage(type_AI_combat_parameters* data)
{
    long totalValue[2];
    long shooterValue[2];

    for (long side = 0; side < 2; side++) {
        shooterValue[side] = 0;
        totalValue[side] = 0;
        const army* stack = m_armies[side];
        for (long i = 0; i < m_numArmies[side]; i++, stack++) {
            if (!stack->is(creatureImmobilized)
                    && !stack->isIncapacitated()
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

    return shooterValue[data->getGroup()] > shooterValue[data->getEnemyGroup()];
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
// Windows is byte-exact with one `i` initialized before the caster; a
// separate count local moves the loop decrement. Mac -O3 is 98.5981%:
// 428 bytes and all ten calls agree, with only six references to this
// stack caster at SP+0x40 instead of retail SP+0x44. The full canonical
// 0x410 Mac class layout and -O4 leave those offsets unchanged. No DC
// local or retail operation supports adding a source word just for padding.

// E:\gamedcs\ai.cpp:1635
VA(0x00420d20, 0x1D5) MAC_ADDRESS(0x022cb8, 0x1ac)  // anchor-callee, dc 0x2600c
unsigned char combatManager::chooseCreatureSpell(const army* currentArmy, long* bestValue, type_AI_combat_parameters* estimate)
{
    // Dreamcast calls getGroup again in the loop; Complete's Mac body keeps
    // this side in a register across the loop, and Windows retains the same
    // loop CFG only when the side is cached before construction.
    long side = estimate->getGroup();
    long i = m_numArmies[side];
    long bestHex = -1;
    long ourValue = currentArmy->getTotalCombatValue(
            estimate->m_lowestAttack, estimate->m_lowestDefense);
    type_AI_spellcaster caster(this, estimate->getGroup(), 1);
    if (*bestValue != 0 && random(1, 100) <= 30)
        return 0;
    for (; i-- > 0; ) {
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
// Mac Complete's paired 0+0x22e64 body (0x130 bytes) has the same four
// ordered calls. It stores bestValue before bestHex in the acceptance arm;
// that source order raises the Mac match from 95.0658% to 97.6974% while
// Windows stays 94.52% with the exact thirteen-block CFG and four calls.
// The remaining Mac differences are caster stack/frame offsets; the
// Windows residual remains the currentArmy/bestHex register-home swap.
VA(0x00420f00, 0xFB) MAC_ADDRESS(0x022e64, 0x130)
bool combatManager::sodChooseFaerieDragonSpell(
        const army* currentArmy, long& bestValue,
        type_AI_combat_parameters& estimate)
{
    long bestHex = -1;
    type_AI_spellcaster caster(this, estimate.getGroup(), 1);
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
        bestValue = value;
        bestHex = hex;
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
// `army demon_army` is for, built once outside the walk and valued instead of
// the corpse. The Archangel prices the stack it is actually restoring.
// Either way the value doubles while our side is already ahead on live
// value and the odds have not turned.

// EH-bearing (P2.2) and NOT blocked by it: the fs:[0] frame is the local
// army's unwind scaffolding, one state.

// DC's can_cast_resurrect/get_group helpers restore retail's caller-saved
// register schedule, and Complete calls findDemonicResurrectionTarget for
// both Pit Lord probes. Separate per-arm assignments preserve DC's branch
// shape; VC6 merges their x87 results and emits retail's single __ftol.
// E:\gamedcs\ai.cpp:1694
VA(0x00421000, 0x275) MAC_ADDRESS(0x022f94, 0x2e4)  // anchor-callee, dc 0x26140
unsigned char combatManager::chooseResurrectAction(const army* currentArmy, long* bestValue, type_AI_combat_parameters* estimate)
{
    long bestTargetHex = -1;  // Original: best_target_hex.
    if (!currentArmy->canCastResurrect())
        return 0;
    army demonArmy;  // Original: demon_army.
    if (currentArmy->m_creatureType == CREATURE_PIT_LORD)
        demonArmy.initialize(TCreatureType(CREATURE_DEMON), 1,
                             m_heroes[estimate->getGroup()],
                             estimate->getGroup(), 0, 0);
    for (long i = m_numArmies[estimate->getGroup()]; i--; ) {
        const army* target = &m_armies[estimate->getGroup()][i];
        if (target == currentArmy)
            continue;
        if (target->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        long hex = target->m_gridIndex;
        if (currentArmy->m_creatureType == CREATURE_PIT_LORD) {
            if (findDemonicResurrectionTarget(
                    estimate->getGroup(), hex) != target) {
                if (!target->is(creatureDoubleWide))
                    continue;
                hex = target->getSecondGridIndex();
                if (findDemonicResurrectionTarget(
                        estimate->getGroup(), hex) != target)
                    continue;
            }
            if (!target->is(creatureImmobilized))
                continue;
        } else {
            if (findResurrectionTarget(
                    estimate->getGroup(), hex, 1) != target) {
                if (!target->is(creatureDoubleWide))
                    continue;
                hex = target->getSecondGridIndex();
                if (findResurrectionTarget(
                        estimate->getGroup(), hex, 1) != target)
                    continue;
            }
        }
        long size = currentArmy->getResurrectionSize(target);
        if (size == 0)
            continue;
        long value;
        if (currentArmy->m_creatureType == CREATURE_PIT_LORD)
            value = static_cast<long>(demonArmy.getUnitCombatValue(
                                          estimate->m_lowestAttack,
                                          estimate->m_lowestDefense, 0, 0)
                                      * size);
        else
            value = static_cast<long>(target->getUnitCombatValue(
                                           estimate->m_lowestAttack,
                                           estimate->m_lowestDefense,
                                           target->canShoot(0), 0)
                                       * size);
        if (estimate->m_awakeFriendlyValue > estimate->m_awakeEnemyValue
                && estimate->m_roundsLeft <= 1)
            value += value;
        if (value > *bestValue) {
            *bestValue = value;
            bestTargetHex = hex;
        }
    }
    if (bestTargetHex < 0)
        return 0;
    m_nextActionGridIndex = bestTargetHex;
    m_nextAction = 10;
    return 1;
}

VA(0x00421280, 0x166) MAC_ADDRESS(0x023278, 0x148)  // dc 0x26464
unsigned char combatManager::chooseSpellAction(const army* currentArmy, long* bestValue, type_AI_combat_parameters* estimate)
{
    if (m_creaturePlacement)
        return 0;
    if (!canCastSpells(estimate->getGroup(), 0))
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

VA(0x004213f0, 0xF5) MAC_ADDRESS(0x0233c0, 0x160)  // dc 0x264fc
unsigned char combatManager::shouldStayInCastle(type_AI_combat_parameters* estimate)
{
    if (!m_fortificationLevel)
        return 0;
    if (estimate->getGroup() != 1)
        return 0;
    { for (const long* target = g_castleWallGateTargets;
           target < (g_castleWallGateTargets + 5); target++) {
        if (m_wallStrength[*target])
            continue;
        if (!hexIsBlocked(s_wallTargets[*target].getBlockedHex()))
            return 0;
    } }
    if (!hasRangedAdvantage(estimate))
        return 0;
    const army* ourArmy = &m_armies[estimate->getGroup()][0];
    for (long i = 0; i < m_numArmies[estimate->getGroup()]; i++, ourArmy++) {
        if (!ourArmy->is(creatureImmobilized)
                && ourArmy->m_creatureType != CREATURE_ARROW_TOWER
                && !combatManager::inCastle(ourArmy->m_gridIndex))
            return 0;
    }
    return 1;
}

VA(0x004214f0, 0x94) MAC_ADDRESS(0x023520, 0xdc)  // dc 0x26600
void combatManager::markFirewalls(const army* currentArmy, long* enemyAttacks, type_AI_combat_parameters* estimate)
{
    for (long i = 0; i < 187; i++) {
        if ((m_cells[i].m_attributes & hexcell::fireWall) == 0)
            continue;
        TObstacle* obstacle = &getObstacle(m_cells[i].m_obstacleIndex);
        long base = obstacle->m_spellDamage;
        long damage = modifySpellDamage(base, SPELL_FIRE_WALL,
                                        m_heroes[obstacle->m_owner],
                                        m_heroes[estimate->getGroup()],
                                        currentArmy, 0);
        enemyAttacks[i] -= currentArmy->getLossCombatValue(
                estimate->m_lowestAttack, estimate->m_lowestAttack, 0, damage,
                estimate->m_killsOnly);
    }
}

VA(0x00421590, 0xE1) MAC_ADDRESS(0x0235fc, 0x128)
void combatManager::markMoat(const army* currentArmy, long* enemyAttacks,
                         type_AI_combat_parameters* estimate)
{
    if (!m_moatOn)
        return;

    long row;
    for (row = 0; row < 11; row++) {
        long hex = g_moatHexes[row];
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
        long hex = g_innerMoatHexes[row];
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
// DC ai.cpp:1899/1900 names value locals our_group and enemy_group and
// calls get_group/get_enemy_group. Retail copies both before markFirewalls;
// references to the estimate fields would instead reload through opaque calls.
// The conditional incapacity assignment (1925/1926) also preserves the
// budget's separate lifetime. Together these restore retail's 0x350 frame.
// Keep chooser.getAttackTime() at its query and acceptance sites (1976,
// 2027/2028, 2052) and the by-value max at 2059. A prematurely cached attack
// time is unnecessary; VC6 can hoist the repeated accessor itself.
// Windows 0x421680: naming the final movement condition after its early guard
// keeps the final moveToward as a third distinct call (97.0793 -> 98.5943%):
// 29 named calls and ten returns now agree. Mac shape retains all 30 direct
// calls in order. Naming the earlier call's condition drops Windows to 97.87%,
// and moving the final declaration before the guard drops it to 97.38%; both
// forms were rejected. Register/stack layout still differs after the tail.
// Moving enemyAttacks below the scalar declarations is byte-flat on Mac.
// A branch-model unsigned-char ourGroup proposal contradicts the Dreamcast
// CodeView long local, so retain the proven type pending new source evidence.
// E:\gamedcs\ai.cpp:1896
VA(0x00421680, 0x8F9) MAC_ADDRESS(0x023724, 0xbb0)  // linkorder, dc 0x266d4
unsigned char combatManager::chooseMeleeTarget(const army* currentArmy, unsigned char teleport, long* actionValue, type_AI_combat_parameters* estimate)
{
    long enemyAttacks[COMBAT_GRID_CELLS];

    long ourGroup = estimate->getGroup();
    long enemyGroup = estimate->getEnemyGroup();
    army* bestTarget = 0;
    long bestValue = 0;
    long bestTime = 0;
    long bestHexValue = 0;
    unsigned char bestIsBlockingAction = 0;
    long markedEnemies = 0;
    long bestHex = -1;
    long budget = 127;
    memset(enemyAttacks, 0, sizeof(enemyAttacks));

    markFirewalls(currentArmy, enemyAttacks, estimate);
    if (g_game->m_gameVersion >= 2)
        markMoat(currentArmy, enemyAttacks, estimate);
    if (g_game->m_setup.m_difficulty > 0 || m_sideIsAi[ourGroup])
        markEnemyAttacks(currentArmy, enemyAttacks, &markedEnemies,
                           estimate);
    if (g_game->m_setup.m_difficulty >= 2 || m_sideIsAi[ourGroup])
        markFriendlyArmies(currentArmy, enemyAttacks, markedEnemies,
                             estimate);

    if (currentArmy->isIncapacitated())
        budget = 0;
    if (teleport)
        g_searchArray->markTeleport(currentArmy, ourGroup);
    else
        g_searchArray->seedCombatPosition(currentArmy, ourGroup, budget,
                                          m_creaturePlacement, -1);
    unsigned char stayingInCastle = shouldStayInCastle(estimate);

    for (long i = 0; i < m_numArmies[enemyGroup]; i++) {
        army* enemy = &m_armies[enemyGroup][i];
        if (enemy->is(creatureImmobilized))
            continue;
        if (enemy->m_creatureType == CREATURE_ARROW_TOWER)
            continue;
        if (estimate->m_simulated && enemy->getTotalHitPoints(1) == 0)
            continue;
        if (currentArmy->getSpeed() == 0 || currentArmy->getSpellTime(72)) {
            const pathCell* stand = g_searchArray->getHex(enemy->m_gridIndex);
            if (stand->m_cost > 0)
                continue;
        }
        if (stayingInCastle && !combatManager::inCastle(enemy->m_gridIndex)
                && (!enemy->is(creatureDoubleWide)
                    || !combatManager::inCastle(enemy->getSecondGridIndex())))
            continue;

        long change = 0;
        unsigned char isBlockingAction = 0;
        if (m_cells[enemy->m_gridIndex].m_validMove
                && (g_game->m_setup.m_difficulty >= 2 || m_sideIsAi[ourGroup])
                && !currentArmy->cannotAttack()
                && !enemy->is(creatureMultiHeaded)) {
            const pathCell* reach = g_searchArray->getHex(enemy->m_gridIndex);
            if (reach->m_cost <= currentArmy->getSpeed())
                change = getAttackChange(currentArmy, enemy, *estimate);
        }

        type_AI_attack_hex_chooser chooser(currentArmy, enemy, enemyAttacks,
                                           g_searchArray, estimate);
        if (!chooser.findAttackHex())
            continue;
        if (!currentArmy->cannotAttack()) {
            long distance = 0;
            if (chooser.getAttackTime() == 1) {
                const pathCell* attackCell = g_searchArray->getHex(chooser.getBestHex());
                distance = attackCell->m_cost;
            }
            change += estimate->getSimpleAttackEffect(*(currentArmy), *(enemy), 0, distance);
        }

        if (chooser.getHexValue() <= 0 || currentArmy->is(creatureSummoned)
                || (g_game->m_setup.m_difficulty == 0 && !m_sideIsAi[ourGroup])) {
            change += chooser.getHexValue();
        } else if (currentArmy->m_creatureType == CREATURE_HARPY
                   || currentArmy->m_creatureType == CREATURE_HARPY_HAG) {
            if (change < chooser.getHexValue()) {
                isBlockingAction = 1;
                change = chooser.getHexValue();
            }
        } else if (change < 0 && hasRangedAdvantage(estimate)) {
            change = chooser.getHexValue();
            isBlockingAction = 1;
        } else {
            change += chooser.getHexValue();
        }
        if (change < 0
                && (g_game->m_setup.m_difficulty > 0 || m_sideIsAi[ourGroup])
                && chooser.getHexValue() < enemyAttacks[chooser.getBestHex()])
            continue;

        long randomFactor = ::random(75, 100);
        long score = randomFactor * change / 100;
        if (bestTarget != 0) {
            if (enemy->isIncapacitated() && !bestTarget->isIncapacitated())
                continue;
            if (!(bestTarget->isIncapacitated() && !enemy->isIncapacitated())) {
                if (bestTime < chooser.getAttackTime())
                    continue;
                if (bestTime == chooser.getAttackTime()) {
                    if (bestValue > score)
                        continue;
                    if (bestValue == score) {
                        if (bestTarget->m_topCreatureDamage
                                > enemy->m_topCreatureDamage)
                            continue;
                        if (bestTarget->m_topCreatureDamage
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
        bestTarget = enemy;
        bestIsBlockingAction = isBlockingAction;
        bestHex = chooser.getBestHex();
        bestTime = chooser.getAttackTime();
        bestHexValue = chooser.getHexValue() * randomFactor / (bestTime * 100);
        bestValue = score / bestTime;
    }

    *actionValue = max(bestValue, bestHexValue);
    if (teleport) {
        if (bestTarget != 0 && *actionValue >= 0) {
            m_nextAction = 6;
            m_nextActionExtra = bestHex;
            m_nextActionGridIndex = bestTarget->m_gridIndex;
            return 1;
        }
        m_nextAction = 3;
        return 1;
    }
    if (!estimate->m_simulated && !m_creaturePlacement
            && chooseSpellAction(currentArmy, actionValue, estimate))
        return 1;
    if (bestIsBlockingAction) {
        *actionValue = bestHexValue;
        if (bestHex == currentArmy->m_gridIndex)
            return 0;
        moveToward(currentArmy, bestHex, enemyAttacks,
                    static_cast<unsigned char>(!estimate->m_simulated
                                               && bestTime > 1));
        return 1;
    }

    if (bestTarget == 0) {
        if (m_fortificationLevel > 0 && m_currentSide == 0) {
            long hex = g_castleWallColumns[combatManager::gridY(currentArmy->m_gridIndex)];
            while (hex > currentArmy->m_gridIndex) {
                const pathCell* cell = g_searchArray->getHex(hex);
                if (cell->m_visited) {
                    if (!isInMoat(hex, 0)) {
                        if (!currentArmy->is(creatureDoubleWide))
                            break;
                        if (!isInMoat(hex + currentArmy->offsetToFront(-1), 0))
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
    if (bestTarget == 0
            || (bestValue < 0 && bestValue < bestHexValue
                && !currentArmy->is(creatureSummoned)
                && (g_game->m_setup.m_difficulty > 0 || m_sideIsAi[ourGroup])
                && hasRangedAdvantage(estimate))) {
        *actionValue = 0;
        if (!estimate->m_simulated
                && getAreaEffect(enemyGroup, currentArmy,
                                   markedEnemies,
                                   estimate) == 0
                && attemptShooterDefense(currentArmy, g_searchArray, estimate))
            return 1;
        if (bestTarget == 0) {
            if (!estimate->m_simulated
                    && chooseToRun(currentArmy, enemyAttacks, g_searchArray))
                return 1;
            return 0;
        }
    }

    *actionValue = bestValue;
    if (bestTime <= 1 && !m_creaturePlacement) {
        m_nextActionExtra = bestHex;
        m_nextAction = 6;
        m_nextActionGridIndex = bestTarget->m_gridIndex;
        return 1;
    }
    if (bestHex == currentArmy->m_gridIndex)
        return 0;
    bool shouldMoveToward =
        !estimate->m_simulated && bestTime > 1;
    moveToward(currentArmy, bestHex, enemyAttacks,
               shouldMoveToward);
    return 1;
}

VA(0x00421f80, 0xD5) MAC_ADDRESS(0x0242d4, 0x124)  // dc 0x26ee0
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
    if (!isInSecondPhase() && (g_game->m_setup.m_difficulty >= 2 || m_sideIsAi[side])) {
        m_nextAction = 8;
        return 0;
    }
    m_nextAction = 3;
    return 0;
}

VA(0x00422060, 0x18E) MAC_ADDRESS(0x0243f8, 0x1ec)  // dc 0x26fa8
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
            if (dir >= 6 && !currentArmy->is(creatureDoubleWide))
                continue;
            long adjacent = currentArmy->getAdjacentHex(newHex, dir);
            if (!validHex(adjacent))
                continue;
            army* other = m_cells[adjacent].getArmy();
            if (other != 0 && other != currentArmy) {
                if (other->is(creatureShootingArmy))
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

// DC lines 2289-2292 put army::Is in a nested branch, then attribute the
// assignments of 2 and 3 to separate scoped arms. The ternary spelling
// emitted the same behavior but omitted retail's `and eax,0xff` after
// `not al`. Restoring the separate arms makes VC6 exact: 9/9 CFG blocks,
// all five calls, and all 71 instructions. The canonical army::is returns
// bool as its DC mangled signature says. Mac 0:0x245e4 remains exact at
// 272 bytes with the same five ordered calls.
VA(0x004221f0, 0xD0) MAC_ADDRESS(0x0245e4, 0x110)  // anchor-callee, dc 0x27138
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
    else if (currentArmy->is(creatureFlyingArmy))
        action = 2;
    else
        action = 3;
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

VA(0x004222c0, 0x175) MAC_ADDRESS(0x0246f4, 0x1f0)  // dc 0x27200
void combatManager::berserkAttack(army* currentArmy, const army* target)
{
    currentArmy->m_side = target->getOwningSide();
    currentArmy->m_slot = target->m_bitIndex;
    long hex = target->m_gridIndex;
    if (inInvisibleColumn(hex) && target->is(creatureDoubleWide))
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
        if (target->getOwningSide() == currentArmy->getOwningSide())
            m_playDoh[target->getOwningSide()] = 1;
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
    if (target->getOwningSide() == currentArmy->getOwningSide())
        m_playDoh[target->getOwningSide()] = 1;
}

// DC ai.cpp:2378 calls includes.h's value-returning min wrapper before
// GetFireShieldStrength at 2382. Keep the capped damage as its own statement:
// retail reproduces the comparison and temporary slots without a second
// selector or a reference escaping a by-value helper's parameters.
VA(0x00422440, 0x99) MAC_ADDRESS(0x0248e4, 0x120)  // dc 0x27318
long combatManager::computeFireShieldDamage(long damage, const army* attacker, const army* target, long targetHits) const
{
    if (!target->getSpellTime(SPELL_FIRE_SHIELD)
            && target->m_creatureType != CREATURE_EFREET_SULTAN)
        return 0;
    if (attacker->is(creatureImmuneToFireSpells))
        return 0;
    damage = min(damage, targetHits);
    damage = static_cast<long>(target->getFireShieldStrength() * damage);
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
MAC_ADDRESS(0x024a04, 0xc4)
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

VA(0x004224e0, 0x2B4) MAC_ADDRESS(0x024ac8, 0x1f0)  // dc 0x2746c
void combatManager::simulateMeleeAttack(army* currentArmy, long hex,
                                          army* target, long enemyHex,
                                          long ourGroup)
{
    if (currentArmy->is(creatureMultiHeaded)) {
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
            if (victim->getOwningSide() == ourGroup)
                continue;
            hit |= bit;
            simulateSimpleAttack(currentArmy, victim, 0, 0, 0);
        } while (direction-- > 0);
        return;
    }

    simulateSimpleAttack(currentArmy, target,
                           g_searchArray->getHex(hex)->m_cost, 0, 0);

    if (currentArmy->is(creatureHasExtendedAttack)) {
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

VA(0x004227a0, 0xDB) MAC_ADDRESS(0x024cb8, 0x140)  // dc 0x275e8
void combatManager::simulateMeleeAttack(army* currentArmy, army* target,
                                          long ourGroup)
{
    long hex = m_nextActionExtra;
    if (!validHex(hex))
        return;

    long hitPoints = target->getTotalHitPoints(0);
    simulateMeleeAttack(currentArmy, hex, target, target->m_gridIndex,
                          ourGroup);

    if (target->canRetaliate(*currentArmy)
        && (g_game->m_setup.m_difficulty > 0 || m_sideIsAi[ourGroup]))
        simulateMeleeAttack(target, target->m_gridIndex, currentArmy, hex,
                              1 - ourGroup);

    if (currentArmy->is(creatureTwoAttacks) && target->getAIExpectedDamage() < hitPoints)
        simulateMeleeAttack(currentArmy, hex, target, target->m_gridIndex,
                              ourGroup);
}

VA(0x00422880, 0x1B5) MAC_ADDRESS(0x024df8, 0x22c)  // dc 0x27698
long combatManager::simulateActions(std::vector<army*>& list, long i,
                                     long ourGroup)
{
    type_AI_combat_parameters data(this, ourGroup);

    for (; i < list.size(); i++) {
        army* currentArmy = list[i];
        if (currentArmy->cannotAttack()
            || currentArmy->getSpellTime(59)
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
        if (!validHex(hex))
            continue;
        army* target = m_cells[hex].getArmy();
        if (!target)
            continue;
        if (shooting) {
            simulateSimpleAttack(currentArmy, target, 0, 1, 0);
        } else {
            simulateMeleeAttack(currentArmy, target, ourGroup);
        }
    }
    return i;
}

VA(0x00422a40, 0xD8) MAC_ADDRESS(0x025024, 0xdc)  // dc 0x277f4
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

// E:\gamedcs\ai.cpp:2608..2693, dc 0x27888: separate outer/inner pointer
// inductions, a split shooter guard, and source calls to getSpellTime and
// clearAIValues. Complete's x86 and Mac bodies clear four AI target fields
// through the retail-era helper without clearing expectedDamage, unlike the
// older DC helper. Windows is byte-exact after restoring these boundaries.
// Mac 0:0x25100 has the same 724-byte size and all 11 ordinary calls; its
// remaining null-path branch order comes from the canonical getHex expansion.
// A named pathCell pointer at that call site was byte-flat and was removed.
VA(0x00422b20, 0x278) MAC_ADDRESS(0x025100, 0x2d4)  // anchor-caller(choose_shooter_action/choose_melee_action) + anchor-callee(SeedCombatPosition), dc 0x27888
void combatManager::findAITargets(long ourGroup, const army* currentArmy,
                                    unsigned char meleeOnly,
                                    const type_AI_combat_parameters* data,
                                    searchArray* currentSearchArray)
{
    long enemyGroup = 1 - ourGroup;
    if (currentSearchArray == 0)
        currentSearchArray = g_searchArray;

    long i;
    long j;
    army* ours = m_armies[ourGroup];
    army* theirs;
    unsigned char shooter;
    for (i = 0; i < m_numArmies[ourGroup]; i++, ours++) {
        ours->clearAIValues();
        if (ours->is(creatureImmobilized))
            continue;
        if (ours->is(creatureSiegeWeapon)
            && ours->m_creatureType != CREATURE_BALLISTA)
            continue;
        if (data->m_simulated && ours->getTotalHitPoints(1) == 0)
            continue;
        if (ours->getSpellTime(70) > 1)
            continue;
        if (ours->getSpellTime(62) > 1)
            continue;
        if (ours->getSpellTime(60))
            continue;
        if (ours->getSpellTime(59))
            continue;
        if (ours == currentArmy)
            continue;

        shooter = ours->canShoot(0);
        if (shooter && meleeOnly)
            continue;
        if (!shooter) {
            if (meleeOnly)
                currentSearchArray->seedCombatPosition(
                    ours, ourGroup, ours->getSpeed(), m_creaturePlacement, -1);
            else
                currentSearchArray->seedCombatPosition(
                    ours, ourGroup, 0x7f, m_creaturePlacement, -1);
        }

        theirs = m_armies[enemyGroup];
        for (j = 0; j < m_numArmies[enemyGroup]; j++, theirs++) {
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

VA(0x00422da0, 0x1AD) MAC_ADDRESS(0x0253d4, 0x170)  // dc 0x27b18
unsigned char combatManager::doSpellAI()
{
    m_nextAction = 0;
    if (m_spellsCast[m_currentSide])
        return 0;
    if (m_creaturePlacement)
        return 0;
    if (getCurrentArmy()->is(creatureSiegeWeapon))
        return 0;
    if (m_playerIds[m_currentSide] >= 0
        && g_game->isHuman(m_playerIds[m_currentSide])
        && !((m_autoCombatOn || g_goSolo)
             && g_config.m_combatAutoSpells)
        && !static_cast<const combatManager*>(this)->isQuickCombat())
        return 0;
    long side = m_currentSide;
    if (!canCastSpells(side, 1))
        return 0;

    type_AI_spellcaster caster(this, m_currentSide, 0);
    if (caster.castSpell(aiCheckRetreat()))
        return 1;
    m_nextAction = 0;
    return 0;
}

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
