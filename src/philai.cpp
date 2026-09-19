#include <va.h>
#include "creaturetype.h"
#include <algorithm>
#include <functional>
#include <math.h>
#include <string.h>
#include "philai.h"
#include "ai_spellvalue.h"
#include "ai_player.h"  // purchaser and AI player interfaces used in this TU
#include "hero.h"
#include "town.h"
#include "tradpost.h"
#include "game.h"
#include "recruit.h"
#include "advmgr.h"  // gpAdvManager + advManager::get_treasure_data, for the
                     // adventure-object appraisals (custom item / scroll / ...)
#include "findpath.h"
#include "kb.h"
#include "kbwin.h"
#include "mousemgr.h"
#include "soundmgr.h"
#include "hillfortwindow.h"
#include "includes.h"

double aiValueOfMorale(long morale, long change);
double aiValueOfLuck(long luck, long change);
long aiGetValueOfArtifact(type_artifact artifact, const hero* owner,
                              unsigned char equipped, unsigned char exact);
long aiGetEquipValue(type_artifact artifact, const hero* ourHero,
                        unsigned char exact);
void aiSetHeroBonuses(hero* ourHero);
void aiEquipArtifacts(hero* currentHero);
int aiChooseDestination(hero* currentHero, long maxDistance,
                          HeroDestination& bestPoint,
                          long& bestRawValue,
                          unsigned char allowSpells,
                          unsigned char exploreMode);
void aiAttemptMove(hero* currentHero, HeroDestination& bestPoint,
                    long& bestRawValue, unsigned char exploreMode);
void moveHero(hero* currentHero, unsigned char isLastHero,
               unsigned char& exploreMode);
long getArtifactPurchasePrice(TArtifact artifact, long marketCount,
                                 EGameResource* bestResource);
TCreatureType siegeArtifactToCreature(TArtifact engine);
TCreatureType upgradedCreatureType(TCreatureType type);

long getSkillValue(const hero* ourHero, TSecondarySkill skill,
                     unsigned char complexChoice);
long getSchoolValue(const hero* ourHero, TSecondarySkill skill);

int aiResourceCost(const playerData* player, const int* resources);
int aiResourceCost(long playerId, const int* resources);
long valueOfUniversity(const hero* currentHero,
                         type_university* university,
                         unsigned char mustPay);
void buySpecialBuilding(hero* currentHero, town* currentTown);
void buySiegeEngine(hero* currentHero, town* currentTown,
                      type_building_id building, TArtifact engine);

long type_AI_creature_swapper::getSwapValue(
    const hero* currentHero, const armyGroup* sourceArmy,
    const hero* secondHero, unsigned char newHasAngelicAlliance);

long aiValueOfCombat(const hero* attackingHero, const hero* defendingHero,
    const armyGroup& defendingArmy, const town* defendingTown,
    NewmapCell* cell);
long valueOfReinforcing(hero* currentHero, town* currentTown,
                          short moveCost);
long valueOfTownBuildings(const hero* currentHero, town* currentTown);
int calcTerrainCost(const NewmapCell* cell, int dir, int pointsLeft,
    long pathfinding, long endRoad, long flying, long waterWalking,
    long nativeTerrain, unsigned char param9);

// CheckDoMain's two TU-local frame-pacing cells. Dreamcast CodeView names the
// second one iLastFrameRateTimer; no readable symbol names the first flag.
DATA(0x0069cca4) static unsigned char g_unnamed69cca4;
DATA(0x0069ccac) static unsigned long g_lastFrameRateTimer;

// Dreamcast names this shared cursor-suppression flag bSpecialHideCursor.
// Its retail cell immediately follows the nine-row town hierarchy table; this
// is its sole retail code reference, so this TU owns the public definition.
DATA(0x006983f8) int g_specialHideCursor;

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// E:\gamedcs\philai.cpp:58
static int onMySide(int whichPlayer)
{
    if (whichPlayer < 0)
        return 0;
    return g_game->onSameTeam(whichPlayer, g_netLocalGamePos);
}

VA(0x005242d0, 0x82)  // dc 0x10d47c
void checkDoMain(int forceMouseCheck, int mouseOnly)
{
    if (!(g_unnamed69cca4 & 1)) {
        g_unnamed69cca4 |= 1;
        g_lastFrameRateTimer = GameTime::get();
    }

    if (GameTime::elapsedSince(g_lastFrameRateTimer) > 15
        || GameTime::isPast(g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT])) {
        process1WindowsMessage();
        pollSound();
        if (GameTime::isPast(g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT])) {
            if (!mouseOnly)
                g_specialHideCursor = 0;
            g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] =
                GameTime::get() + 180;
        }
        g_lastFrameRateTimer = GameTime::get();
    }
}

#if 0  // @carcass

// E:\gamedcs\philai.cpp:102
DC_ONLY(0x10d510, 0x4)
void ShowStatus()
{
    // @stub
}

#endif  // @carcass

VA(0x00524360, 0x3)  // dc 0x10d514
philAI::philAI()
{
}

VA(0x00524370, 0x73)  // dc 0x10d640
void aiEnterGarrison(hero* currentHero, garrison* ourGarrison)
{
    if (currentHero->m_owner != ourGarrison->m_playerOwner)
        return;
    if (!ourGarrison->m_removableTroops)
        return;
    if (g_unk69774c && g_game->m_campaign.m_currentCampaign < 7)
        return;

    unsigned char hasAngelicAlliance =
        g_game->m_players[currentHero->m_owner].hasGivenArtifact(
            ARTIFACT_ANGELIC_ALLIANCE);
    type_AI_creature_swapper swapper;
    swapper.doSwap(currentHero, &ourGarrison->m_garrisonArmy, 0,
                    hasAngelicAlliance);
}

void buyArtifacts(hero* currentHero, TArtifact* artifactList,
                   long marketCount);

VA(0x005243f0, 0x8)  // dc 0x10db64
void aiVisitBlackMarket(hero* currentHero, TBlackMarket* blackMarket)
{
    buyArtifacts(currentHero, blackMarket->m_artifacts, 5);
}

// E:\gamedcs\philai.cpp:123
// Complete expands both known calls; the helper boundary is retained from
// the Dreamcast source rather than flattening the phase adjustment twice.
__forceinline void incrementHourGlass()
{
    int numHeroes = g_currentPlayer->m_numHeroes;
    ++g_curHourGlassPhase;
    if (numHeroes == philAI::ONE_ACTIVE_HERO)
        g_curHourGlassPhase += philAI::TWO_ACTIVE_HEROES;
    else if (numHeroes == philAI::TWO_ACTIVE_HEROES) {
        if (g_curHourGlassPhase != philAI::ONE_ACTIVE_HERO)
            ++g_curHourGlassPhase;
    } else if (numHeroes == philAI::THREE_ACTIVE_HEROES) {
        if (g_curHourGlassPhase == philAI::THIRD_HOURGLASS_PHASE
            || g_curHourGlassPhase == philAI::SIXTH_HOURGLASS_PHASE)
            ++g_curHourGlassPhase;
    }
    if (g_curHourGlassPhase > philAI::LAST_HOURGLASS_PHASE)
        g_curHourGlassPhase = philAI::LAST_HOURGLASS_PHASE;
}

// E:\gamedcs\philai.cpp:150
// DC 0x10d57c is an ordinary static helper. Complete naturally expands it
// at MoveHero's sole retail call site; no forced-inline declaration is needed.
static void restoreMouse(unsigned char mouseWasVisible)
{
    if (mouseWasVisible && !g_mouseManager->isVis()) {
        int saveShowIt = g_completeDrawEnabled;
        g_completeDrawEnabled = 1;
        g_mouseManager->showPointer(0);
        g_completeDrawEnabled = saveShowIt;
    }
}

// E:\gamedcs\philai.cpp:165
// DC 0x10d5b4 is an ordinary static helper. The 170/171 negative-ID return
// closes before GetTown/AI_enter_town; Complete naturally expands this body.
static void checkForTown(hero* currentHero)
{
    int townId = g_game->getTownId(currentHero->m_x, currentHero->m_y,
                                    currentHero->m_z);
    if (townId < 0)
        return;
    aiEnterTown(currentHero, g_game->getTown(townId));
}

#if 0  // @carcass

// E:\gamedcs\philai.cpp:207
DC_ONLY(0x10d684, 0x132)
void upgradeCreatures(hero* current_hero, const town* current_town)
{
    // @stub
}

// get_artifact_purchase_value promoted to a retail claim below. Its active
// definition remains here in Dreamcast source order.

// E:\gamedcs\philai.cpp:326
// Retail claim promoted to the reconstructed body below; DC identity retained.
void buyArtifacts(hero* current_hero, TArtifact* artifact_list, long market_count)
{
    // @stub
}

// E:\gamedcs\philai.cpp:370
DC_ONLY(0x10dae8, 0x7C)
long valueOfBlackMarket(const hero* current_hero, const NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:397
DC_ONLY(0x10db74, 0x150)
void buyArtifacts(hero* current_hero, town* current_town)
{
    // @stub
}

// E:\gamedcs\philai.cpp:583
DC_ONLY(0x10e0f8, 0x20)
void aiVisitWarFactory(hero* current_hero)
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\philai.cpp:207. Retail /Ob2 folds this source helper into
// AI_enter_town. Dreamcast proves the helper boundary and its five named
// locals; retail widens the two cost rows from short to int and proves the
// seven-resource difference/debit loops directly.
static void upgradeCreatures(hero* currentHero, const town* currentTown)
{
    int difference[NUM_RESOURCES];
    long amount;
    long dwelling;
    const int* upgradeCost;
    const int* baseCost;

    for (dwelling = 0; dwelling < TOWN_DWELLING_COUNT; ++dwelling) {
        if (!currentTown->hasBuilding(
                DWELLING_0_UPG_ID + dwelling, 1))
            continue;

        TCreatureType upgrade = g_townUpgradedDwellingCreatures[
            currentTown->m_type * 2 * TOWN_DWELLING_COUNT + dwelling];

        for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
            if (currentHero->m_army.m_armyTypes[slot]
                    != g_townDwellingCreatures[
                        currentTown->m_type * 2 * TOWN_DWELLING_COUNT
                        + dwelling])
                continue;

            // DC :232/:236 retains base_cost and upgrade_cost as pointers
            // into akCreatureTypeTraits.cost. Complete widens cost entries
            // from short to int; retail proves +0x20 in a 116-byte record.
            baseCost = g_creatureTypeTraits[
                currentHero->m_army.m_armyTypes[slot]].m_cost;
            upgradeCost = g_creatureTypeTraits[upgrade].m_cost;
            amount = currentHero->m_army.m_numTroops[slot];

            int resource;
            for (resource = 0; resource < NUM_RESOURCES; ++resource) {
                difference[resource] =
                    (upgradeCost[resource] - baseCost[resource]) * amount;
                if (difference[resource]
                        > g_currentPlayer->m_resources[resource])
                    break;
            }

            if (resource < NUM_RESOURCES)
                continue;

            for (resource = 0; resource < NUM_RESOURCES; ++resource)
                g_currentPlayer->m_resources[resource] -=
                    difference[resource];
            currentHero->m_army.m_armyTypes[slot] = upgrade;
        }
    }
}

// E:\gamedcs\philai.cpp:298, dc 0x10d91c. CodeView records internal linkage.
static long getArtifactPurchaseValue(
    TArtifact artifactId, long marketCount, long* funds)
{
    if (artifactId == ARTIFACT_NONE)
        return 0;

    EGameResource resource;
    long price = getArtifactPurchasePrice(
        artifactId, marketCount, &resource);
    if (price > funds[resource])
        return 0;

    type_artifact artifact(artifactId);
    long value = static_cast<long>(
        static_cast<double>(aiGetValueOfArtifact(
            artifact, g_netLocalGamePos))
        - static_cast<double>(price)
            * g_currentPlayer->m_ai.m_resourceValue[resource]);
    if (value < 0)
        value = 0;
    return value;
}

// E:\\gamedcs\\philai.cpp:370. Dreamcast gives this six-statement helper
// and Complete expands it into AI_value_of_event's BLACK_MARKET arm: reject
// a full backpack, select the cell's seven-artifact market record, and sum
// the five-marketplace purchase value of every slot.
inline long valueOfBlackMarket(const hero* currentHero,
                                  const NewmapCell* cell)
{
    if (const_cast<hero*>(currentHero)->getNumberInBackpack(1)
            == HERO_BACKPACK_CAPACITY)
        return 0;

    const TBlackMarket& market = g_game->m_blackMarkets[cell->m_extraInfo];
    long value = 0;
    for (int artifact = 0; artifact < 7; ++artifact) {
        value += getArtifactPurchaseValue(
            market.m_artifacts[artifact], 5, g_currentPlayer->m_resources);
    }
    return value;
}

static long valueOfWarFactory(const hero* currentHero,
                                 TArtifact engine, long moveCost);

// E:\gamedcs\philai.cpp:561, dc 0x10e064.
static void visitWarFactory(hero* currentHero, TArtifact engine)
{
    if (valueOfWarFactory(currentHero, engine, 0) > 0) {
        TCreatureType creature = siegeArtifactToCreature(engine);
        const int* costs = g_creatureTypeTraits[creature].m_cost;
        for (int resource = 0; resource < 7; resource++)
            g_currentPlayer->m_resources[resource] -= costs[resource];

        type_artifact artifact(engine);
        currentHero->giveArtifact(&artifact, 1, 1);
    }
}

// E:\gamedcs\philai.cpp:636
static const hero* getBestHero(long playerId)
{
    const hero* bestHero = 0;
    int bestSkill = 0;
    playerData* player = &g_game->m_players[playerId];
    for (int i = 0; i < player->m_numHeroes; ++i) {
        hero* candidate = g_game->getHero(player->m_heroes[i]);
        int skill = candidate->getPrimarySkillTotal();
        if (skill >= bestSkill) {
            bestSkill = skill;
            bestHero = candidate;
        }
    }
    return bestHero;
}

// E:\gamedcs\philai.cpp:662
static unsigned char shouldGarrisonTown(const hero* currentHero,
                                          const town* currentTown)
{
    VictoryConditionStruct& victory = g_game->m_mapHeader.m_victoryCondition;
    if (victory.m_type == VICTORY_CONDITION_CAPTURE_TOWN) {
        int townId = g_game->getTownId(victory.m_townX, victory.m_townY,
                                        victory.m_townZ);
        if (townId >= 0 && townId == currentTown->m_id) {
            const hero* bestHero = getBestHero(g_netLocalGamePos);
            if (currentHero != bestHero)
                return 1;
        }
    }
    return 0;
}

// E:\gamedcs\philai.cpp:833
// Complete expands this helper into MoveHero. The source-real loops and
// `cost` local are fixed by the Dreamcast line/scope table; the extra dock
// cost and Dinkumware vector layout are retail facts.
// DC 0x10e6e8: the resource return (839/840), absent dock (851/852), and
// absent boat (882/883) all end before the following work. Keep those scopes
// and the ordinary static declaration, not the old __forceinline substitute.
// With all four MoveHero helpers ordinary, seven deleted shipyard fences
// restore 86.6362% (from 86.3549%) without changing any other tracked TU row.
// Controls: the 256-state fence/boundary family, 97-state scope/value family,
// and 33-state four-helper family. Retaining a forced checkForTown peaks at
// 89.6719%; the old forced shipyard enclosure reaches 93.3304%. Neither is
// source evidence for a forced declaration. Direct/reference point arguments
// in the fully ordinary family score 86.2031%; keep the real copied value.
static void markShipyards(playerData* player)
{
    int cost[7];

    if (player->m_resources[WOOD] < 10
        || player->m_resources[GOLD] < 1000)
        return;

    for (int townIndex = 0; townIndex < player->m_numTowns;
         ++townIndex) {
        town* currentTown = g_game->getTown(
            player->m_townIds[townIndex]);
        if (currentTown->m_dockSite == town::TOWN_DOCK_SITE_NONE)
            continue;

        NewmapCell* cell = g_game->m_worldMap.cell(
            currentTown->m_dockSite, currentTown->m_dockSiteY,
            currentTown->m_mapZ);
        unsigned char canBuildShip = 0;
        if (currentTown->hasBuilding(DOCK_ID, true)) {
            canBuildShip = 1;
        } else if (currentTown->canBuild(DOCK_ID)) {
            canBuildShip = 1;
            currentTown->getBuildCost(DOCK_ID, cost);
            cost[WOOD] += 10;
            cost[GOLD] += 1000;
            for (int resource = 0; resource < 7; ++resource) {
                if (player->m_resources[resource] < cost[resource])
                    canBuildShip = 0;
            }
        }
        cell->m_canBuildShip = canBuildShip;
    }

    for (unsigned int shipyardIndex = 0;
         shipyardIndex < player->m_shipyards.size(); ++shipyardIndex) {
        type_point shipyardPoint = player->m_shipyards[shipyardIndex];
        NewmapCell* shipyard = g_game->getCell(shipyardPoint);
        const ShipyardInfo* info = static_cast<const ShipyardInfo*>(
            static_cast<const void*>(&shipyard->m_extraInfo));
        if (info->m_boatX == ShipyardInfo::NO_BOAT)
            continue;

        g_game->m_worldMap.cell(
            info->m_boatX, info->m_boatY,
            player->m_shipyards[shipyardIndex].m_z)->m_canBuildShip = 1;
    }
}

// E:\gamedcs\philai.cpp:896
// Complete expands this helper into MoveHero immediately after the second
// set_danger_zones statement.
// DC 0x10e894: the 920/921 absent-boat continue closes before the 925/926
// cell update. Its size/getCell/cell calls need no inline-depth overrides.
static void clearShipyards(playerData* player)
{
    for (int townIndex = 0; townIndex < player->m_numTowns; ++townIndex) {
        town* currentTown = g_game->getTown(player->m_townIds[townIndex]);
        if (currentTown->m_dockSite < town::TOWN_DOCK_SITE_NONE
            && currentTown->m_dockSiteY < town::TOWN_DOCK_SITE_NONE) {
            NewmapCell* cell = g_game->m_worldMap.cell(
                currentTown->m_dockSite, currentTown->m_dockSiteY,
                currentTown->m_mapZ);
            cell->m_canBuildShip = 0;
        }
    }

    for (unsigned int shipyardIndex = 0;
         shipyardIndex < player->m_shipyards.size(); ++shipyardIndex) {
        type_point shipyardPoint = player->m_shipyards[shipyardIndex];
        NewmapCell* shipyard = g_game->getCell(shipyardPoint);
        const ShipyardInfo* info = static_cast<const ShipyardInfo*>(
            static_cast<const void*>(&shipyard->m_extraInfo));
        if (info->m_boatX == ShipyardInfo::NO_BOAT)
            continue;

        g_game->m_worldMap.cell(
            info->m_boatX, info->m_boatY,
            player->m_shipyards[shipyardIndex].m_z)->m_canBuildShip = 0;
    }
}

#if 0  // @carcass -- philai body-evidence claims, retail RVA order (divergent from DC link order)

// E:\gamedcs\philai.cpp:1239
DC_ONLY(0x10f0d0, 0x9C)
void move_all_heroes(long player_id, long* danger_zones)
{
    // @stub
}

// E:\gamedcs\philai.cpp:1529
DC_ONLY(0x10f94c, 0x136)
long type_spellvalue::get_summoning_value(long damage, long times_castable)
{
    // @stub
}

// E:\gamedcs\philai.cpp:1770
DC_ONLY(0x110018, 0x15A)
void philAI::getTurnAIVars(int whichPlayer)
{
    // @stub
}

// E:\gamedcs\philai.cpp:1854
DC_ONLY(0x110390, 0x34)
int valueOfArena(const hero* current_hero, NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:1883
DC_ONLY(0x110408, 0x16A)
int valueOfMapArtifact(const hero* current_hero, NewmapCell* cell)
{
    // @stub
}

#endif  // @carcass

// Source-order declarations for helpers whose retained Complete bodies live
// later in retail RVA order.
int netValueOfArtifact(const hero* currentHero, int artifactValue,
    int goldCost, int resourceCost, EGameResource resourceType);
long valueOfCustomItem(const hero* currentHero, NewmapCell* cell,
    long itemValue);
long valueOfLearning(const hero* currentHero, SpellID spell);
static long getArtifactPurchaseValue(
    TArtifact artifactId, long marketCount, long* funds);
long valueOfEnemyTown(const hero* currentHero, const town* enemyTown,
                         short moveCost, NewmapCell* cell);

int aiResourceCost(const playerData* player, const int* resources)
{
    int value = 0;
    for (int resource = 0; resource < NUM_RESOURCES; resource++)
        value += resources[resource] * player->m_ai.m_resourceValue[resource];
    return value;
}

int aiResourceCost(long playerId, const int* resources)
{
    return aiResourceCost(&g_game->m_players[playerId], resources);
}

// E:\\gamedcs\\philai.cpp:1610. Dreamcast preserves this helper boundary,
// its one type_creature_value local, the seven-slot scan, push_back and the
// descending sort. Complete's /Ob2 folds the single call into the constructor
// at 0x526d40; retaining it here in its original lexical position reproduces
// that expansion without flattening the source.
void type_spellvalue::fillCreatureValueList()
{
    type_creature_value creature;
    for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
        creature.m_type = m_ourHero->m_army.m_armyTypes[i];
        if (creature.m_type != CREATURE_NONE) {
            creature.m_amount = m_ourHero->m_army.m_numTroops[i];
            creature.m_value = g_creatureTypeTraits[creature.m_type].m_aiValue
                * creature.m_amount;
            m_list.push_back(creature);
        }
    }
    std::sort(m_list.begin(), m_list.end(),
              std::greater<type_creature_value>());
}

// E:\gamedcs\philai.cpp:1699.  The what-if probe AI_set_hero_bonuses runs
// six times: bump the valuer's power/duration/mana, re-ask the best spell
// value, restore, and report the delta against the caller's baseline.
// Defined here as the DC build does; retail keeps no out-of-line row -
// every site is expanded - and objdiff prices an unclaimed base-only
// symbol at nothing.
long type_spellvalue::getValueOfIncrease(long baseValue,
    long powerChange, long durationChange, long manaChange)
{
    m_power += powerChange;
    m_duration += durationChange;
    m_mana += manaChange;
    long increased = getBestSpellValue(SPELL_VALUE_CLASS_MASK);
    m_power -= powerChange;
    m_duration -= durationChange;
    m_mana -= manaChange;
    return increased - baseValue;
}

// E:\\gamedcs\\philai.cpp:1854. Dreamcast preserves this source-real helper
// boundary and its two calls; Complete keeps the same four-block body but
// expands it into AI_value_of_event's ARENA arm. The retail arm calls
// VisitedArena, returns zero on the visited edge, then doubles the level
// increment before applying turnExperienceToRVRatio.
inline int valueOfArena(const hero* currentHero, NewmapCell* cell)
{
    if (currentHero->visitedArena(cell))
        return 0;
    int experienceValue =
        2 * hero::getExperienceIncrement(currentHero->m_level);
    return static_cast<int>(
        experienceValue * currentHero->m_turnExperienceToRvRatio);
}

// E:\\gamedcs\\philai.cpp:1883. Dreamcast preserves the complete helper:
// backpack capacity, artifact appraisal and minimum-value clamp, customized
// and defended early returns, then the six MapArtifactInfo price cases.
// Complete expands this helper into AI_value_of_event's ARTIFACT arm and
// retains the same statement/branch shape, using its later artifact-player
// valuation entry point.
inline int valueOfMapArtifact(const hero* currentHero, NewmapCell* cell)
{
    if (const_cast<hero*>(currentHero)->getNumberInBackpack(1)
            >= HERO_BACKPACK_CAPACITY)
        return 0;

    type_artifact artifact(cell->getArtifactIndex());
    int value = aiGetValueOfArtifact(artifact, currentHero->m_owner);
    if (value < 10)
        value = 10;

    const ExtraInfoUnion* info = static_cast<const ExtraInfoUnion*>(
        static_cast<const void*>(cell));
    if (info->isCustomized()) {
        return valueOfCustomItem(currentHero, cell, value);
    }

    if (info->isDefendedArtifact()) {
        armyGroup monsters(info->getArtifactDefender(),
                           info->m_artifactInfo.m_guardQty);
        return aiValueOfCombat(currentHero, 0, monsters, 0, cell) + value;
    }

    switch (info->getArtifactPrice()) {
    case const_free_artifact:
        return value;
    case const_artifact_requires_wisdom:
        if (currentHero->m_skillLevel[eSecSkillWisdom] > 0)
            return value;
        return 0;
    case const_artifact_requires_leadership:
        if (currentHero->m_skillLevel[eSecSkillLeadership] > 0)
            return value;
        return 0;
    case const_artifact_costs_2000:
        return netValueOfArtifact(currentHero, value, 2000, 0, WOOD);
    case const_artifact_costs_2500:
        return netValueOfArtifact(currentHero, value, 2500, 3,
                                  info->getArtifactResourceCost());
    case const_artifact_costs_3000:
        return netValueOfArtifact(currentHero, value, 3000, 5,
                                  info->getArtifactResourceCost());
    }
    return 0;
}

// E:\\gamedcs\\philai.cpp:1972. The Dreamcast line table recovers this
// entire appraisal in source order, and Complete's BLACK_BOX arm preserves
// the same field walks and control flow after inlining: guardians,
// experience/resources, primary and secondary skills, artifacts, spells,
// then joinable creatures.
// The SecondarySkills loop's duplicated `value +=` is DELIBERATE, and the
// merge is measured (polish 16). It is the whole of AI_value_of_event's
// one remaining branch-polarity divergence: retail cross-jumps the two arms
// onto ONE `imul ecx,[ebp-4] / add esi,ecx` tail and reaches it with
// `jl` where this compile duplicates the tail and emits `jge`. Writing the
// merge in source DOES close the polarity - branches go from one flip to
// clean - and costs 0.0331 anyway (97.7399 -> 97.7068, and the skeleton's
// exact-block count FALLS 34 -> 25), because the merged form transposes the
// enclosing loop's registers: retail and the duplicated form both hold the
// index in EAX and the accumulator in ESI, the merged form swaps them.
// FOUR spellings were measured and all produce the byte-identical object at
// 97.7068 - a named `int amount = level` with `amount = level - current_level`,
// the same with `amount -= current_level`, reusing `level` itself as the
// accumulator (no new local at all), and a `goto` into an explicit
// `add_skill:` label reproducing retail's jump exactly. A fifth, an
// if/else-if/else chain with a trailing `continue`, is worse still (97.16,
// and the polarity flip comes back). The merge is therefore a FIXED POINT
// this compiler reaches from any source shape; the cost is the register
// transposition, not the spelling, so do not re-try the merge family.
inline int valueOfBlackBox(const hero* currentHero, NewmapCell* cell)
{
    BlackBoxData* blackBox = cell->getBlackBox();
    int value = 0;

    if (blackBox->m_hasCustomGuardians)
        value = aiValueOfCombat(
            currentHero, 0, blackBox->m_guardians, 0, cell);

    if (blackBox->m_experienceBonus > 0) {
        value = static_cast<int>(
            static_cast<float>(value)
            + static_cast<float>(blackBox->m_experienceBonus)
                * currentHero->m_turnExperienceToRvRatio);
    }

    value += aiResourceCost(
        &g_game->m_players[currentHero->m_owner], blackBox->m_resQty);

    int primarySkillValue = static_cast<int>(
        static_cast<float>(hero::getExperienceIncrement(currentHero->m_level))
        * currentHero->m_turnExperienceToRvRatio);
    for (int skill = 0; skill < 4; ++skill) {
        if (blackBox->m_primarySkillBonus[skill] > 0)
            value += blackBox->m_primarySkillBonus[skill]
                * primarySkillValue;
    }

    for (unsigned int secondaryIndex = 0;
         secondaryIndex < blackBox->m_secondarySkills.size();
         ++secondaryIndex) {
        signed char currentLevel = currentHero->m_skillLevel[
            blackBox->m_secondarySkills[secondaryIndex].m_type];
        int level = blackBox->m_secondarySkills[secondaryIndex].m_level;
        if (currentLevel == 0 && currentHero->m_skillCount < 8)
            value += level * primarySkillValue;
        else if (currentLevel > 0 && currentLevel < level)
            value += (level - currentLevel) * primarySkillValue;
    }

    value = static_cast<int>(
        static_cast<float>(value)
        + static_cast<float>(blackBox->m_artifacts.size())
            * g_currentPlayer->m_ai.m_turnValueOfAvgArtifact);

    if (currentHero->isWieldingArtifact(
            ARTIFACT_SPELLBOOK)) {
        for (unsigned int spell = 0;
             spell < blackBox->m_spells.size(); ++spell) {
            SpellID spellId;
            {
                int ordinal = blackBox->m_spells[spell];
                memcpy(&spellId, &ordinal, sizeof spellId);
            }
            value += valueOfLearning(currentHero, spellId);
        }
    }

    for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
        int creature = blackBox->m_creatures.m_armies[slot];
        if (creature != CREATURE_NONE
            && currentHero->m_army.canJoin(creature)) {
            value += blackBox->m_creatures.m_numTroops[slot]
                * g_creatureTypeTraits[creature].m_aiValue;
        }
    }
    return value;
}

inline long valueOfBank(const hero* currentHero, NewmapCell* cell)
{
    long value;

    ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
        static_cast<void*>(cell));
    type_creature_bank& bank = info->getCreatureBank();
    if (cell->m_extraInfo & 0x2000000)
        return 0;

    value = aiValueOfCombat(currentHero, 0, bank.m_guards, 0, cell);
    if (value <= -500000000)
        return value;

    value += aiResourceCost(currentHero->m_owner, bank.m_resources);

    if (bank.m_rewardCreatures > 0)
        value += bank.m_rewardCreatures
            * g_creatureTypeTraits[bank.m_rewardCreature].m_aiValue;

    value = bank.m_artifacts.size()
        * g_currentPlayer->m_ai.m_turnValueOfAvgArtifact + value;
    return value;
}

// E:\\gamedcs\\philai.cpp:2115. Dreamcast gives one size statement and
// one value statement, with the size reused in the gold and special-resource
// terms. Complete expands the two cell accessors and this helper into
// AI_value_of_event's CAMPFIRE arm; retail proves the 100-gold unit and the
// same left-to-right double expression.
inline int valueOfCampfire(playerData* player, NewmapCell* cell)
{
    int size = cell->getCampfireSize();
    return static_cast<int>(
        size * 100 * player->m_ai.m_resourceValue[GOLD]
        + size * player->m_ai.m_resourceValue[cell->getCampfireResource()]);
}

// E:\\gamedcs\\philai.cpp:2194. The recovered helper rejects a Defense
// Tower already visited by this hero, then prices one experience increment
// through the hero's turn ratio. Complete expands the helper and the no-arg
// experience accessor into the DEFENSE_TOWER arm.
inline int valueOfDefenseTower(const hero* currentHero, NewmapCell* cell)
{
    if (currentHero->m_defenseTowerFlags & (1UL << cell->m_extraInfo))
        return 0;
    return static_cast<int>(
        currentHero->getExperienceIncrement()
        * currentHero->m_turnExperienceToRvRatio);
}

// E:\\gamedcs\\philai.cpp:2220. Dreamcast recovers the GetGarrison helper,
// the local creature swapper, the same-owner and same-team arms, and the
// combat fallback. Complete adds the removable-troops and early-campaign
// gates plus the Angelic Alliance input used by its widened swapper method.
inline long valueOfGarrison(const hero* currentHero, NewmapCell* cell)
{
    garrison* currentGarrison = g_game->getGarrison(cell->m_extraInfo);
    if (currentGarrison->m_playerOwner == currentHero->m_owner
        && currentGarrison->m_removableTroops) {
        if (g_unk69774c && g_game->m_campaign.m_currentCampaign < 7)
            return 0;

        unsigned char hasAngelicAlliance =
            g_game->m_players[currentHero->m_owner].hasGivenArtifact(
                ARTIFACT_ANGELIC_ALLIANCE);
        type_AI_creature_swapper swapper;
        return swapper.getSwapValue(
            currentHero, &currentGarrison->m_garrisonArmy, 0,
            hasAngelicAlliance);
    }

    if (g_game->onSameTeam(currentGarrison->m_playerOwner,
                           currentHero->m_owner))
        return 0;
    return aiValueOfCombat(
        currentHero, 0, currentGarrison->m_garrisonArmy, 0, cell);
}

// E:\\gamedcs\\philai.cpp:2250. The shared Idol helper rejects the two
// visited flags and an unaffordable trip, values both morale and luck on
// Sunday, and otherwise selects one from the low flag bit. Complete expands
// that shared shape into AI_value_of_event. Dreamcast's final fallback after
// the same movement rejection is dc-only; retail has no corresponding edge.
inline long valueOfIdol(const hero* currentHero, long moveCost)
{
    if (currentHero->m_flags & 0x10)
        return 0;
    if (currentHero->m_flags & 0x2000000)
        return 0;
    if (moveCost > currentHero->m_movePoints)
        return 0;

    if (g_game->m_day == DAY_OF_WEEK_SUNDAY) {
        return static_cast<long>(
            aiValueOfMorale(
                currentHero->getMorale(0, 0, 1), 1)
            + aiValueOfLuck(
                currentHero->getLuck(0, 0, 1), 1));
    }
    if (currentHero->m_flags & 1) {
        return static_cast<long>(aiValueOfLuck(
            currentHero->getLuck(0, 0, 1), 1));
    }
    return static_cast<long>(aiValueOfMorale(
        currentHero->getMorale(0, 0, 1), 1));
}

// E:\\gamedcs\\philai.cpp:2274. Dreamcast recovers this as one expression
// with two resource-value products and one sum. Complete expands it into the
// FLOTSAM arm; retail's literal pool fixes the expected haul at 175 gold and
// five wood.
inline int valueOfFlotsam(playerData* player)
{
    return static_cast<int>(
        player->m_ai.m_resourceValue[GOLD] * 175.0
        + player->m_ai.m_resourceValue[WOOD] * 5.0);
}

// E:\\gamedcs\\philai.cpp:2283. A visited Garden of Revelation is worth
// nothing; otherwise its one knowledge point is worth the hero's cached
// knowledge value. Complete expands both the helper and accessor.
inline int valueOfGarden(const hero* currentHero, NewmapCell* cell)
{
    if (currentHero->m_gardenOfRevelationFlags & (1UL << cell->m_extraInfo))
        return 0;
    return currentHero->getValueOfKnowledge();
}

// E:\\gamedcs\\philai.cpp:2294. The recovered helper tests the item's
// visit bit in the player's Lean-To flags and prices an unvisited cache as
// three average resource units. Complete expands the helper and GetItemId
// into the event arm without changing that shape.
__forceinline int valueOfLeanTo(NewmapCell* cell, playerData* player)
{
    const ExtraInfoUnion* info = static_cast<const ExtraInfoUnion*>(
        static_cast<const void*>(cell));
    if (player->m_leanToFlags & (1UL << info->getItemId()))
        return 0;
    return 3 * player->m_ai.m_averageResourceValue;
}

// E:\\gamedcs\\philai.cpp:2392. Dreamcast recovers the complete meeting
// helper, including its two locals and the split between enemy and friendly
// heroes. Complete expands it into AI_value_of_event and adds the Angelic
// Alliance input to both creature-swap appraisals. Retail also confirms the
// enemy hero's bounty term, the stranded-player combat floor, and the same
// movement-cost guard around the friendly-army increase check.
__forceinline long valueOfHeroEvent(const hero* currentHero,
                                       NewmapCell* cell, short x, short y,
                                       short z, short moveCost)
{
    hero* secondHero = g_game->getHero(cell->m_extraInfo);
    if (secondHero->m_owner != currentHero->m_owner) {
        if (g_game->onSameTeam(secondHero->m_owner, currentHero->m_owner))
            return 0;

        short townId = static_cast<short>(g_game->getTownId(x, y, z));
        long value = aiValueOfCombat(
            currentHero, secondHero, secondHero->m_army, 0, cell);
        if (moveCost <= currentHero->m_movePoints) {
            value = static_cast<long>(
                static_cast<float>(value)
                + static_cast<float>(secondHero->m_bounty + 10000)
                    * type_AI_player::getAttackBonus(secondHero->m_owner));
        }

        if (townId < 0)
            return value;
        town* currentTown = g_game->getTown(townId);
        if (value <= -500000000) {
            if (currentHero->getPlayer()->m_numTowns > 0)
                return value;
            value = -1250000;
        }
        return valueOfEnemyTown(
            currentHero, currentTown, moveCost, cell) + value;
    }

    if (!g_game->m_setup.m_difficulty)
        return 0;

    type_AI_creature_swapper swapper;
    short currentSkill =
        const_cast<hero*>(currentHero)->getPrimarySkillTotal();
    short secondSkill = secondHero->getPrimarySkillTotal();
    if (secondSkill < currentSkill) {
        unsigned char hasAngelicAlliance =
            g_game->m_players[currentHero->m_owner].hasGivenArtifact(
                ARTIFACT_ANGELIC_ALLIANCE);
        long value = swapper.getSwapValue(
            currentHero, &secondHero->m_army, secondHero,
            hasAngelicAlliance) / 2;
        if (moveCost >= currentHero->m_maxMovePoints
            && swapper.getArmyIncrease()
                   < currentHero->m_army.getAIValue())
            return 0;
        return value;
    }
    if (secondSkill > currentSkill) {
        unsigned char hasAngelicAlliance =
            g_game->m_players[secondHero->m_owner].hasGivenArtifact(
                ARTIFACT_ANGELIC_ALLIANCE);
        return swapper.getSwapValue(
            secondHero, &currentHero->m_army, currentHero,
            hasAngelicAlliance);
    }
    return 0;
}

// E:\\gamedcs\\philai.cpp:2465. Dreamcast proves the two seven-resource
// locals, the seven army-slot walk, upgrade lookup/cost calls, affordability
// test, provisional-funds debit, and final movement/army-strength gate.
// Complete expands the helper into AI_value_of_event and routes the upgrade
// lookup through game::UpgradedCreatureType, whose real inline body carries
// Complete's elemental-upgrade restriction.
__forceinline long valueOfHillFort(const hero* currentHero,
                                      long moveCost)
{
    int funds[NUM_RESOURCES];
    memcpy(funds, g_currentPlayer->m_resources, sizeof(funds));
    long cost[NUM_RESOURCES];
    long value = 0;

    for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
        TCreatureType creature = currentHero->m_army.m_armyTypes[slot];
        if (creature == CREATURE_NONE)
            continue;

        TCreatureType upgrade = g_game->upgradedCreatureType(creature);
        if (upgrade == CREATURE_NONE)
            continue;

        getUpgradeCost(creature, upgrade,
                         currentHero->m_army.m_numTroops[slot], cost);
        cost[GOLD] = static_cast<int>(
            static_cast<float>(cost[GOLD])
            * g_afUpgradeCostFactor[g_creatureTypeTraits[creature].m_level]);

        int resource;
        for (resource = 0; resource <= GOLD; ++resource) {
            if (cost[resource] > funds[resource])
                break;
        }
        if (resource <= GOLD)
            continue;

        value += currentHero->m_army.m_numTroops[slot]
            * (g_creatureTypeTraits[upgrade].m_aiValue
               - g_creatureTypeTraits[creature].m_aiValue);
        for (resource = 0; resource <= GOLD; ++resource)
            funds[resource] -= cost[resource];
    }

    if (moveCost > 500
        && 3 * value < currentHero->m_army.getAIValue())
        return 0;
    return value;
}

// E:\\gamedcs\\philai.cpp:2550. Dreamcast recovers both early returns and
// the exact value ingredients. Complete expands the helper and its two hero
// value accessors into the event arm: a qualifying unvisited hero receives
// two power values, two knowledge values, and one tenth of the army value.
__forceinline int valueOfLibrary(const hero* currentHero, NewmapCell* cell)
{
    if (currentHero->m_libraryFlags & (1UL << cell->m_extraInfo))
        return 0;
    if (currentHero->m_level
            + 2 * currentHero->m_skillLevel[eSecSkillDiplomacy]
        < 10)
        return 0;
    return 2 * currentHero->getValueOfPower()
        + 2 * currentHero->getValueOfKnowledge()
        + currentHero->m_army.getAIValue() * 4 / 40;
}

// E:\\gamedcs\\philai.cpp:2567. The source helper indexes the lighthouse's
// mine record, tests its owner through OnSameTeam, and returns 1,000 only
// for an enemy lighthouse. Complete expands the vector access but retains
// the same OnSameTeam boundary.
__forceinline int valueOfLighthouse(NewmapCell* cell)
{
    if (g_game->onSameTeam(
            g_game->m_mines[cell->m_extraInfo].m_playerOwner,
            g_netLocalGamePos))
        return 0;
    return 1000;
}

// E:\\gamedcs\\philai.cpp:2615. An already visited Mercenary Camp is
// worthless; otherwise it grants one level increment, converted through the
// hero's current experience-to-resource ratio. Complete expands the helper
// and retains the static experience accessor call.
__forceinline int valueOfMercenaryCamp(const hero* currentHero,
                                       NewmapCell* cell)
{
    if (currentHero->m_mercCampFlags & (1UL << cell->m_extraInfo))
        return 0;
    return static_cast<int>(
        hero::getExperienceIncrement(currentHero->m_level)
        * currentHero->m_turnExperienceToRvRatio);
}

inline int valueOfMoveSource(const hero* currentHero, long flag,
                                short increase, long& moveCost)
{
    if (currentHero->m_flags & flag)
        return 0;
    if (moveCost >= increase) {
        moveCost -= increase;
        return const_cast<hero*>(currentHero)->moraleIncreaseValue(1);
    }
    moveCost = 0;
    return 10000;
}

// E:\\gamedcs\\philai.cpp:2775. Dreamcast preserves this one-statement
// helper and the tiny type_AI_player accessor beneath it. Complete expands
// both boundaries into AI_value_of_event's HUT_OF_MAGI arm.
__forceinline long valueOfMagusHut(long playerId)
{
    return g_aiPlayers[playerId].getMagusHutValue();
}

// E:\\gamedcs\\philai.cpp:2948. Dreamcast recovers the item-id visit bit,
// owner-player lookup, backpack-capacity branch, and the two value sources.
// Complete expands the helper into AI_value_of_event's DEAD_GUY arm; retail
// fixes the later constants as one fifth of an average artifact, or 200 gold
// when the backpack is full.
inline int valueOfSkeleton(const hero* currentHero, NewmapCell* cell)
{
    const ExtraInfoUnion* info = static_cast<const ExtraInfoUnion*>(
        static_cast<const void*>(cell));
    unsigned long visited = 1UL << info->getItemId();
    if (g_currentPlayer->m_deadGuyFlags & visited)
        return 0;

    playerData* player = currentHero->getPlayer();
    if (const_cast<hero*>(currentHero)->getNumberInBackpack(1)
            < HERO_BACKPACK_CAPACITY)
        return static_cast<int>(player->m_ai.m_turnValueOfAvgArtifact / 5.0f);
    return static_cast<int>(player->m_ai.m_resourceValue[GOLD] * 200.0);
}

// E:\\gamedcs\\philai.cpp:2997. The helper reads the shrine spell through
// its named packed-cell accessor and prices learning it. Complete expands
// the helper/accessor but retains value_of_learning as a real call.
__forceinline int valueOfShrine(const hero* currentHero, NewmapCell* cell)
{
    const ExtraInfoUnion* info = static_cast<const ExtraInfoUnion*>(
        static_cast<const void*>(cell));
    SpellID spell = info->getShrineSpell();
#pragma inline_depth(0)
    return valueOfLearning(currentHero, spell);
#pragma inline_depth()
}

// CHECKPOINT (93.7290 -> 98.1308): Dreamcast line 355 prices the tray entry,
// then line 358 constructs the artifact from the tray again.  Removing the
// source-false artifact-id cache restores retail's reload and, with it, every
// register allocation and all 15 CFG blocks.  The sole residual is the
// scheduler's placement of the constructor's independent `extra = -1` store:
// retail hoists it across the resource debit while this build emits it after
// the artifact-id store. why-reg classifies the two-slot distance as a pure
// B16/C3/C4 schedule transpose; all nine catalog probes are flat or worse,
// and copy-initializing an equivalent constructor temporary falls to 92.48%.
VA(0x00524400, 0x149)  // anchor-global, dc 0x10da30
void buyArtifacts(hero* currentHero, TArtifact* artifactList,
                   long marketCount)
{
    while (currentHero->getNumberInBackpack(1)
           != HERO_BACKPACK_CAPACITY) {
        long bestArtifact = -1;
        long bestValue = 0;

        for (long i = 0; i < 7; i++) {
            if (artifactList[i] != ARTIFACT_NONE) {
                long value = getArtifactPurchaseValue(
                    artifactList[i], marketCount,
                    g_currentPlayer->m_resources);
                if (value > bestValue) {
                    bestValue = value;
                    bestArtifact = i;
                }
            }
        }

        if (bestArtifact >= 0) {
            EGameResource resource;
            long price = getArtifactPurchasePrice(
                artifactList[bestArtifact], marketCount, &resource);
            g_currentPlayer->m_resources[resource] -= price;

            type_artifact artifact(artifactList[bestArtifact]);
            currentHero->giveArtifact(&artifact, 1, 1);
            artifactList[bestArtifact] = ARTIFACT_NONE;
        } else {
            aiEquipArtifacts(currentHero);
            return;
        }
    }
}

// The gold price has to be ONE reused local: the traits cost is read
// into a register and staged back through that local's own frame slot
// before the fild, and the same slot then stages the price for the
// double conversion.  Reading the cost straight out of the traits row
// fild's from the table with no staging store at all, and giving the
// cost its own named local splits the slot in two.
VA(0x00524550, 0xD2)  // dc 0x10d7b8
long getArtifactPurchasePrice(TArtifact artifact, long marketCount,
    EGameResource* bestResource)
{
    long price = g_artifactTraits[artifact].m_cost;
    *bestResource = GOLD;
    price = static_cast<long>(static_cast<float>(price)
        / g_artifactPurchaseEfficency[marketCount]);
    long bestPrice = price;
    long bestValue = static_cast<long>(static_cast<double>(price)
        * g_currentPlayer->m_ai.m_resourceValue[GOLD]);

    for (int i = WOOD; i < GOLD; i++) {
        EGameResource resource;
        {
            int ordinal = i;
            memcpy(&resource, &ordinal, sizeof resource);
        }
        long amount = price * 2 / getMarketValue(resource);
        if (amount <= g_currentPlayer->m_resources[i]) {
            long value = static_cast<long>(static_cast<double>(amount)
                * g_currentPlayer->m_ai.m_resourceValue[i]);
            if (value <= bestValue) {
                bestValue = value;
                bestPrice = amount;
                *bestResource = resource;
            }
        }
    }
    return bestPrice;
}

VA(0x00524630, 0x60)
int hero::soDGetSeerSkillValue(int skill, int level)
{
    int typedSkill;
    typedSkill = skill;

    if (m_skillLevel[skill] >= level)
        return 0;
    if (m_skillLevel[skill] == 0) {
        if (m_skillCount >= 8)
            return 0;
        if (!wantsSkill(this, TSecondarySkill(typedSkill), 1))
            return 0;
    }
    return getSkillValue(
        this, TSecondarySkill(typedSkill), static_cast<unsigned char>(1));
}

// E:\gamedcs\philai.cpp:3469.  Retail 0x524690/1668 B: (ecx=hero, edx=skill,
// stack=complex_choice) returning a long, opening on `skillLevel[skill]` and
// walking the seven army slots - the DC get_skill_value shape exactly,
// 1334 -> 1668 B.  AI_choose_secondary_skill (0x52bbd0) calls it twice, at the
// two sites DC's own body has.
DATA(0x00681860)
static double g_aiPathfindingMapFactor[3] = {1.0, 1.0, 1.0};
DATA(0x00681878)
static double g_aiWaterMapFraction = 1.0;

// Residual (78.1958%, ANATOMISED 2026-09-05 - the whole gap is TWO
// cross-jumps we make and retail does not, plus their register fallout).
// The two sides agree on everything structural: 48 conditional branches
// each, the call census AGREES, and the 28-entry jump table pairs
// index-for-index INCLUDING its repeat pattern (indices 3/12/21 share one
// target, 6/9 share another, on both sides).  Retail has 44 `ret` sites to
// our 43, and the missing one is named:
//  - eSecSkillDefense (index 23).  Both sides compute `army_value * 7 / 100`
//    for Offense (22) and Defense (23), and retail EMITS BOTH - 0x24 bytes
//    each at fn+0x490 and fn+0x4b4 - because its two copies are NOT
//    byte-identical: the `mov eax,0x51eb851f` is scheduled third in Offense
//    and second in Defense, and the shift temporary is EDX in one and ECX in
//    the other.  Our Offense arm is byte-identical to retail's Offense arm;
//    our Defense arm is `lea ecx,[8*edi] / sub ecx,edi / jmp` into it.  That
//    single merge is the missing `ret` and 0x16 of the gap.
//  - eSecSkillIntelligence (index 24), 0x43 more.  Retail keeps the two
//    GetPrimarySkill(3) expansions separate, loading `[esi+0x479]` INSIDE
//    each arm (fn+0x4d8 and fn+0x4f5); we hoist that one byte load above the
//    `test dl,dl`, which makes the !complex_choice tail byte-identical to the
//    SchoolOfEarthMagic arm's and lets C2 cross-jump it away to fn+0x3c8.
//    The hoist is what enables the merge - it is the same defect twice.
// WHAT ACTUALLY BLOCKS RETAIL'S MERGES IS ONE SCRATCH REGISTER, and it is
// systematic across this body.  Retail's two `army_value / 50` arms are
// byte-identical except for the shift temporary - ECX at fn+0x13a (the
// Leadership/Luck copy) and EDX at fn+0x406 (BattleTactics) - and its
// Offense/Defense pair differs only in that register plus where the
// `mov eax,0x51eb851f` is scheduled.  Nothing semantic separates any of
// them; C2's first-fit allocator simply reached a different point in its
// preference order at each site, and near-identical is enough to defeat the
// cross-jumper.  Our compile picks the same register at both Offense and
// Defense, so it merges them.  That makes this a `why-reg` residual wearing
// a control-flow mask, and the knob is whatever moves the pseudo creation
// count between the arms - not the arms' own spelling.
// MEASURED, both directions, 2026-09-05:
//  - Grouping the three identical `army_value / 20` cases
//    (Scouting/Necromancy/Learning) at the LEARNING position REPRODUCES
//    RETAIL'S PLACEMENT EXACTLY - the shared body moves from our fn+0x113
//    (right after the index-2 arm) to fn+0x48f, between the index-20 and
//    index-22 arms, which is retail's fn+0x475 to the block.  It still LOSES,
//    78.1958 -> 77.7488, because the same renumbering makes C2 merge the
//    BattleTactics arm into the Leadership/Luck copy as well: the ret count
//    goes 43 -> 41 and index 19 starts sharing index 6/9's target, which
//    retail does not do.  So the grouping is probably the right source shape
//    and it is paying for a second-order register effect; whoever lands it
//    must keep BattleTactics' arm distinct at the same time.
//  - A named `long defense_value = army_value * 7;` in the Defense arm is
//    byte-flat to the digit - VC6 folds it straight back.
// MEASURED 2026-09-06, and this CLOSES the arm-level search. Grouping
// Leadership+Luck at the LEADERSHIP position makes the `ret` census EXACT -
// 48 branches and 44 rets on both sides, retail's own numbers - and costs
// 2.06 (78.1958 -> 76.1400), because with the census right C2 then merges
// Offense into Defense, which is the merge retail does not make. Every way
// of separating those two arms FROM INSIDE THEM was then tried against the
// grouped variant and every one is BYTE-FLAT to the digit, all four landing
// on exactly 76.1400: a named `long scaled = army_value * 7;` in the Defense
// arm, the same in the Offense arm, a `7L` literal, and `7 * army_value`
// operand order (which is also byte-flat at 78.1958 ungrouped). Swapping the
// two arms' source order costs 0.02 in both variants (78.1799 / 76.1220),
// and decomposing one arm as `army_value / 100 * 7 + (army_value % 100) * 7
// / 100` overshoots to 45 rets and 74.6100.
// So the separation is NOT arm-addressable: retail's two copies differ only
// in a scratch register and in where `mov eax,0x51eb851f` schedules, and
// every source expression that computes `army_value * 7 / 100` lowers to the
// same bytes - which is precisely why C2's cross-jumper merges them. The
// knob is the pseudo-creation count UPSTREAM of the two arms, and it is not
// reachable from either arm's own spelling. 78.1958 stays the max.
// 2026-09-07 upstream controls: Dreamcast philai.cpp:3491 computes the
// creature-traits address before line 3492's troop-value multiplication.
// Naming that pointer (or const reference) is byte-identical at 78.19582.
// Promoting GetPrimarySkill's local to int gives 76.2231; promoting this
// function's mastery local gives 74.4189; both give 69.9535. Keep the byte
// locals. These controls do not explain the later arithmetic cross-jumps.
VA(0x00524690, 0x684)  // anchor-callee, dc 0x1135ac
long getSkillValue(const hero* ourHero, TSecondarySkill skill,
                     unsigned char complexChoice)
{
    signed char level = ourHero->m_skillLevel[skill];
    if (level == eMasteryExpert)
        return 0;
    if (level == eMasteryNone
        && ourHero->m_skillCount == kNumSecSkillsPerHero)
        return 0;

    long armyValue = 1000;
    long rangedValue = 500;
    if (complexChoice) {
        for (int group = 0; group < armyGroup::ARMY_GROUP_SLOT_COUNT;
             group++) {
            TCreatureType creature = ourHero->m_army.m_armyTypes[group];
            if (creature != CREATURE_NONE) {
                long value = g_creatureTypeTraits[creature].m_aiValue
                    * ourHero->m_army.m_numTroops[group];
                armyValue += value;
                if (g_creatureTypeTraits[creature].m_attributes & 4)
                    rangedValue += value;
            }
        }
    }

    switch (skill) {
    case eSecSkillPathfinding:
        return static_cast<long>(
            armyValue * g_aiPathfindingMapFactor[level] / 4.0);
    case eSecSkillArchery: {
        DATA(0x00640380)
        static const int constArcheryValue[3] = {16, 7, 11};
        return constArcheryValue[level] * rangedValue / 100;
    }
    case eSecSkillLogistics:
        return armyValue / 10;
    case eSecSkillScouting:
        return armyValue / 20;
    case eSecSkillDiplomacy:
        return armyValue / 100;
    case eSecSkillNavigation:
        return static_cast<long>(
            armyValue * g_aiWaterMapFraction / 2.0);
    case eSecSkillLeadership:
        return armyValue / 50;
    case eSecSkillWisdom:
        if (complexChoice)
            return ourHero->getPrimarySkill(2)
                * ourHero->getValueOfPower() / 2;
        return ourHero->getPrimarySkill(2) * 25;
    case eSecSkillMysticism:
        if (complexChoice)
            return ourHero->getValueOfKnowledge() / 10;
        return 1;
    case eSecSkillLuck:
        return armyValue / 50;
    case eSecSkillSiegeBallistics:
        return armyValue / 8;
    case eSecSkillEagleEye:
        if (!ourHero->m_skillLevel[eSecSkillWisdom])
            return 0;
        if (complexChoice)
            return ourHero->getValueOfPower() / 5;
        return 2;
    case eSecSkillNecromancy:
        return armyValue / 20;
    case eSecSkillEstates: {
        DATA(0x0064038c)
        static const int constEstateValue[3] = {725, 725, 1550};
        if (complexChoice)
            return static_cast<long>(
                constEstateValue[level]
                * g_aiPlayers[ourHero->m_owner].getResourceValue(GOLD));
        return constEstateValue[level];
    }
    case eSecSkillSchoolOfFireMagic:
        if (complexChoice)
            return getSchoolValue(ourHero, skill);
        if (!ourHero->m_skillLevel[eSecSkillWisdom])
            return 0;
        return ourHero->getPrimarySkill(2) * 10;
    case eSecSkillSchoolOfAirMagic:
        if (complexChoice)
            return getSchoolValue(ourHero, skill);
        if (!ourHero->m_skillLevel[eSecSkillWisdom])
            return 25;
        return ourHero->getPrimarySkill(2) * 10;
    case eSecSkillSchoolOfWaterMagic:
        if (complexChoice)
            return getSchoolValue(ourHero, skill);
        if (!ourHero->m_skillLevel[eSecSkillWisdom])
            return 3;
        return ourHero->getPrimarySkill(2) * 10;
    case eSecSkillSchoolOfEarthMagic:
        if (complexChoice)
            return getSchoolValue(ourHero, skill);
        if (!ourHero->m_skillLevel[eSecSkillWisdom])
            return 30;
        return ourHero->getPrimarySkill(2) * 10;
    case eSecSkillMagicScholar:
        if (!ourHero->m_skillLevel[eSecSkillWisdom])
            return 0;
        if (complexChoice)
            return ourHero->getPrimarySkill(2)
                * ourHero->getValueOfPower() / 10;
        return ourHero->getPrimarySkill(2) * 5;
    case eSecSkillBattleTactics:
        return armyValue / 50;
    case eSecSkillBattlefieldBallistics:
        if (complexChoice
            && !ourHero->isWieldingArtifact(
                   ARTIFACT_BALLISTA))
            return 0;
        return ourHero->getPrimarySkill(0) * 10;
    case eSecSkillLearning:
        return armyValue / 20;
    case eSecSkillOffense:
        return armyValue * 7 / 100;
    case eSecSkillDefense:
        return armyValue * 7 / 100;
    case eSecSkillIntelligence:
        if (complexChoice)
            return ourHero->getPrimarySkill(3)
                * ourHero->getValueOfKnowledge() / 4;
        return ourHero->getPrimarySkill(3) * 10;
    case eSecSkillSorcery:
        if (complexChoice)
            return ourHero->getPrimarySkill(2)
                * ourHero->getValueOfPower() / 20;
        return ourHero->getPrimarySkill(2) * 2;
    case eSecSkillMagicResistance:
        return armyValue / 40;
    case eSecSkillFirstAid:
        if (complexChoice
            && !ourHero->isWieldingArtifact(
                   ARTIFACT_FIRST_AID_TENT))
            return 0;
        return 250;
    default:
        return 0;
    }
}

#if 0  // @carcass -- philai body-evidence claims, retail RVA order (divergent from DC link order)

// E:\gamedcs\philai.cpp:3645.  Retail 0x524dd0/243 B: same three-argument
// shape returning a byte, sweeping the 28 skill slots against the hero class's
// gainSecondarySkillChance row and calling get_skill_value above.  312 -> 243 B,
// and AI_choose_secondary_skill's tail is its one caller. Complete's call site
// passes the skill and choice as full-width integers.
// Retail claim promoted to the reconstructed body below.
unsigned char wantsSkill(const hero* our_hero, TSecondarySkill first, unsigned char complex_choice)
{
    // @stub
}

// E:\gamedcs\philai.cpp:3744
// Retail claim promoted to the reconstructed Complete-signature body below.
void aiVisitUniversity(hero* current_hero, NewmapCell* cell)
{
    // @stub
}

#endif  // @carcass

VA(0x00524d20, 0xa3)  // dc 0x11350c
long getSchoolValue(const hero* ourHero, TSecondarySkill skill)
{
    type_spellvalue value(ourHero);
    long baseValue = value.getBestSpellValue(SPELL_VALUE_CLASS_MASK);

    signed char level = ourHero->m_skillLevel[skill];
    int oldLevel = level;
    if (oldLevel == 0)
        const_cast<hero*>(ourHero)->m_skillLevel[skill] = 3;
    else
        const_cast<hero*>(ourHero)->m_skillLevel[skill] = level + 1;

    long schoolValue =
        value.getBestSpellValue(SPELL_VALUE_CLASS_MASK) - baseValue;
    const_cast<hero*>(ourHero)->m_skillLevel[skill] = oldLevel;
    return schoolValue;
}

VA(0x00524dd0, 0xF3)  // dc 0x113ae4
unsigned char wantsSkill(const hero* ourHero, TSecondarySkill first,
                         unsigned char complexChoice)
{
    long skillValue[28];
    TSecondarySkill skillIndex[28];

    int openSlots;
    int i;
    for (i = 0; i < 28; i++) {
        if (ourHero->m_skillLevel[i] <= 0
            && (g_heroClasses[ourHero->m_heroClass]
                    .m_gainSecondarySkillChance[i]
                || first == i))
            skillValue[i] = getSkillValue(ourHero, TSecondarySkill(i), complexChoice);
        else
            skillValue[i] = 0;
        skillIndex[i] = TSecondarySkill(i);
    }

    for (i = 0; i < 27; i++) {
        for (int j = i + 1; j < 28; j++) {
            if (skillValue[skillIndex[i]] > skillValue[skillIndex[j]])
                std::swap(skillIndex[i], skillIndex[j]);
        }
    }

    openSlots = 8 - ourHero->m_skillCount;
    for (i = 28; i-- > 0;) {
        int skill = skillIndex[i];
        if (ourHero->m_skillLevel[skill] == 0) {
            if (skill == first)
                return 1;
            if (--openSlots <= 0)
                return 0;
        }
    }
    return 1;
}

VA(0x00524ed0, 0xED)  // dc 0x113cbc
void aiVisitUniversity(hero* currentHero, type_university* university)
{
    if (currentHero->m_skillCount >= 8)
        return;
    if (g_currentPlayer->m_resources[GOLD] < 2000)
        return;

    const THeroClassTraits& traits = g_heroClasses[currentHero->m_heroClass];
    do {
        int bestSkill = -1;
        long bestValue = 0;

        for (int i = 0; i < 4; i++) {
            int skill = university->m_skills[i];
            if (traits.m_gainSecondarySkillChance[skill]
                && currentHero->m_skillLevel[skill] <= 0
                && wantsSkill(currentHero, TSecondarySkill(skill), 1)) {
                long value = getSkillValue(currentHero, TSecondarySkill(skill), 1);
                if (value >= bestValue) {
                    bestValue = value;
                    bestSkill = skill;
                }
            }
        }

        if (bestSkill == -1)
            return;
        currentHero->giveSS(bestSkill, 1);
        g_currentPlayer->m_resources[GOLD] -= 2000;
    } while (g_currentPlayer->m_resources[GOLD] >= 2000);
}

VA(0x00524fc0, 0x156)
void aiVisitWarFactory(hero* currentHero)
{
    visitWarFactory(currentHero, ARTIFACT_BALLISTA);
    visitWarFactory(currentHero, ARTIFACT_FIRST_AID_TENT);
    visitWarFactory(currentHero, ARTIFACT_AMMO_CART);
}

// Retail keeps this helper out of all three expansions of visit_war_factory.
// The whole-TU control reproduces that decision without auto_inline(off).
// DC 521/522 refuses before the remaining body, 525/526 forms the creature
// cost row before the value accumulator at 530, and 541/542 refuses excess
// cost. Preserve those boundaries and the canonical type_artifact constructor.
// Indexed costs score 96.5185%; walking the cost row raises this to 99.4198%.
// An explicit sum or index declaration move is neutral; 78 traversal states
// (14 objects) leave the funds/value increment ordering unresolved. A further
// six distinct source states restore the early refusal and cost-row order;
// three reproduced objects retain 99.4198% with no sibling changes.
VA(0x00525120, 0xE0)
static long valueOfWarFactory(const hero* currentHero,
                                 TArtifact engine, long moveCost)
{
    if (currentHero->hasArtifact(engine) || !g_game->m_setup.m_difficulty)
        return 0;

    long artifactValue = aiGetValueOfArtifact(
        type_artifact(engine), currentHero, false, true);
    TCreatureType creature = siegeArtifactToCreature(engine);
    const int* costs = g_creatureTypeTraits[creature].m_cost;
    const double* resourceValues = g_currentPlayer->m_ai.m_resourceValue;
    long resourceCost = 0;
    for (int resource = 0; resource < 7; ++resource, ++costs) {
        if (g_currentPlayer->m_resources[resource] < *costs)
            return 0;
        resourceCost += *costs * resourceValues[resource];
    }

    if (moveCost <= 500)
        resourceCost = 0;
    if (resourceCost > artifactValue)
        return 0;
    return artifactValue - resourceCost;
}

VA(0x00525200, 0x1d0)  // dc 0x10e334
void considerGarrisoning(hero* currentHero, town* currentTown)
{
    if (g_currentPlayer->m_numHeroes < 2
        || !g_game->m_setup.m_difficulty
        || currentTown->m_threateningHeroes > 0)
        return;

    hero* secondHero = 0;
    if (currentTown->m_garrisonHeroId != -1)
        secondHero = g_game->getHero(currentTown->m_garrisonHeroId);
    if (secondHero
        && secondHero->getPrimarySkillTotal()
               >= currentHero->getPrimarySkillTotal())
        return;

    VictoryConditionStruct& victory = g_game->m_mapHeader.m_victoryCondition;
    if (victory.m_type != VICTORY_CONDITION_CAPTURE_TOWN)
        return;
    int townId = g_game->getTownId(victory.m_townX, victory.m_townY,
                                    victory.m_townZ);
    if (townId < 0 || townId != currentTown->m_id)
        return;

    playerData* player = &g_game->m_players[g_netLocalGamePos];
    hero* bestHero = 0;
    int bestSkill = 0;
    for (int i = 0; i < player->m_numHeroes; ++i) {
        hero* candidate = g_game->getHero(player->m_heroes[i]);
        int skill = candidate->getPrimarySkillTotal();
        if (skill >= bestSkill) {
            bestSkill = skill;
            bestHero = candidate;
        }
    }
    if (currentHero == bestHero)
        return;

    currentHero->m_movePoints = 0;
    unsigned char hasAngelicAlliance =
        g_game->m_players[currentHero->m_owner].hasGivenArtifact(
            ARTIFACT_ANGELIC_ALLIANCE);
    type_AI_creature_swapper swapper;
    swapper.doSwap(
        currentHero,
        const_cast<armyGroup*>(
            &static_cast<const town*>(currentTown)->getArmy()),
        secondHero, hasAngelicAlliance);

    if (!secondHero) {
        currentHero->m_army.mergeArmies(
            *const_cast<armyGroup*>(
                &static_cast<const town*>(currentTown)->getArmy()));
        const_cast<armyGroup*>(
            &static_cast<const town*>(currentTown)->getArmy())->initialize();
    }
}

VA(0x005253d0, 0x60c)  // dc 0x10e3f8
void aiEnterTown(hero* currentHero, town* currentTown)
{
    if (currentHero->hasArtifact(ARTIFACT_HOLY_GRAIL)
        && !currentTown->hasBuilding(HOLY_GRAIL_ID, 0)
        && currentTown->isLegalBuilding(HOLY_GRAIL_ID)) {
        currentHero->removeArtifact(ARTIFACT_HOLY_GRAIL);
        currentTown->buildBuilding(HOLY_GRAIL_ID, 1, 1);
        if (g_game->m_mapHeader.m_victoryCondition.checkForGrailBuildingWin())
            checkEndGame(0);
    }

    hero* garrisonHero = 0;
    if (currentTown->m_garrisonHeroId != -1)
        garrisonHero = g_game->getHero(currentTown->m_garrisonHeroId);

    if (currentHero->m_owner == currentTown->m_owner) {
        considerGarrisoning(currentHero, currentTown);
        if (currentHero->m_movePoints > 0) {
            g_aiPlayers[currentHero->m_owner].buyCreatures(
                currentHero, currentTown);
            g_aiPlayers[currentHero->m_owner].buyMageGuild(
                currentHero, currentTown);
        }
    }

    {
        playerData* player = &g_game->m_players[currentHero->m_owner];
        if (player->m_resources[GOLD] >= 500
            && currentTown->hasBuilding(MAGE_GUILD_ID, 1)
            && !currentHero->isWieldingArtifact(ARTIFACT_SPELLBOOK)) {
            TArtifact artifactId;
            {
                int ordinal = ARTIFACT_SPELLBOOK;
                memcpy(&artifactId, &ordinal, sizeof artifactId);
            }
            type_artifact artifact(artifactId);
            currentHero->giveArtifact(&artifact, 1, 1);
            player->m_resources[GOLD] -= 500;
        }
    }
    currentTown->giveSpells(currentHero);

    if (currentHero->m_movePoints > 0
        && currentHero->m_owner == currentTown->m_owner) {
        upgradeCreatures(currentHero, currentTown);

        if (currentHero->m_owner == currentTown->m_owner
            && !g_game->m_towns[currentTown->m_id].m_builtThisTurn) {
            switch (currentTown->m_type) {
            case TOWN_TOWER: {
                if (!currentTown->canBuild(EXTRA_2_ID))
                    break;
                int* cost =
                    currentTown->getBuildCostArray(EXTRA_2_ID);
                int value = currentHero->getValueOfKnowledge();
                int owner = currentHero->m_owner;
#pragma inline_depth(0)
                if (value > aiResourceCost(
                        &g_game->m_players[owner], cost))
                    currentTown->buyBuilding(EXTRA_2_ID);
#pragma inline_depth()
                break;
            }
            case TOWN_INFERNO: {
                if (!currentTown->canBuild(EXTRA_2_ID))
                    break;
                int* cost =
                    currentTown->getBuildCostArray(EXTRA_2_ID);
                int value = currentHero->getValueOfPower();
                int owner = currentHero->m_owner;
#pragma inline_depth(0)
                if (value > aiResourceCost(
                        &g_game->m_players[owner], cost))
                    currentTown->buyBuilding(EXTRA_2_ID);
#pragma inline_depth()
                break;
            }
            case TOWN_DUNGEON: {
                if (!currentTown->canBuild(EXTRA_2_ID))
                    break;
                int* cost =
                    currentTown->getBuildCostArray(EXTRA_2_ID);
                int owner = currentHero->m_owner;
#pragma inline_depth(0)
                int resourceCost = aiResourceCost(
                    &g_game->m_players[owner], cost);
#pragma inline_depth()
                if (currentHero->m_turnExperienceToRvRatio * 1000.0f
                    > resourceCost)
                    currentTown->buyBuilding(EXTRA_2_ID);
                break;
            }
            case TOWN_STRONGHOLD: {
                if (!currentTown->canBuild(EXTRA_2_ID))
                    break;
                int* cost =
                    currentTown->getBuildCostArray(EXTRA_2_ID);
                long experience = hero::getExperienceIncrement(
                    currentHero->m_level);
#pragma inline_depth(0)
                int resourceCost = aiResourceCost(
                    &g_game->m_players[currentHero->m_owner], cost);
#pragma inline_depth()
                if (static_cast<float>(experience)
                    * currentHero->m_turnExperienceToRvRatio
                    > resourceCost)
                    currentTown->buyBuilding(EXTRA_2_ID);
                break;
            }
            case TOWN_FORTRESS: {
                if (!currentTown->canBuild(SPECIAL_BUILDING_ID))
                    break;
                int* cost = currentTown->getBuildCostArray(
                    SPECIAL_BUILDING_ID);
                long experience = hero::getExperienceIncrement(
                    currentHero->m_level);
#pragma inline_depth(0)
                int resourceCost = aiResourceCost(
                    &g_game->m_players[currentHero->m_owner], cost);
#pragma inline_depth()
                if (static_cast<float>(experience)
                    * currentHero->m_turnExperienceToRvRatio
                    > resourceCost)
                    currentTown->buyBuilding(SPECIAL_BUILDING_ID);
                break;
            }
            case TOWN_CONFLUX: {
                if (!currentTown->canBuild(EXTRA_0_ID))
                    break;
                type_university university;
                university.initializeMagicSkills();
#pragma inline_depth(0)
                long value = valueOfUniversity(
                    currentHero, &university, 0);
#pragma inline_depth()
                if (value > 0)
                    currentTown->buyBuilding(EXTRA_0_ID);
                break;
            }
            }
        }

        buySpecialBuilding(currentHero, currentTown);

        if (g_game->m_setup.m_difficulty) {
            if (garrisonHero) {
                aiSwapArtifacts(currentHero, garrisonHero);
                aiSwapArtifacts(garrisonHero, currentHero);
            }

            {
                TArtifact artifactId;
                int ordinal =
                    g_blacksmithArtifacts[currentTown->m_type].m_artifactId;
                memcpy(&artifactId, &ordinal, sizeof artifactId);
                buySiegeEngine(currentHero, currentTown, BLACKSMITH_ID,
                               artifactId);
            }
            if (currentTown->m_type == TOWN_STRONGHOLD)
                buySiegeEngine(currentHero, currentTown, EXTRA_1_ID,
                                 ARTIFACT_BALLISTA);
        }
    }

    if (currentTown->m_type == TOWN_CONFLUX
        && currentTown->hasBuilding(EXTRA_0_ID, 1)) {
        type_university university;
        university.initializeMagicSkills();
        aiVisitUniversity(currentHero, &university);
    }

    g_advManager->demobilizeCurrHero(0, 0);
    if (garrisonHero)
        currentTown->applySpecialBuildingEffect(garrisonHero);
    if (currentTown->m_visitingHeroId != -1)
        currentTown->applySpecialBuildingEffect(
            g_game->getHero(currentTown->m_visitingHeroId));
}

VA(0x005259e0, 0x205)  // dc 0x10db74
void buySpecialBuilding(hero* currentHero, town* currentTown)
{
    if (currentTown->m_type != TOWN_TOWER
        && currentTown->m_type != TOWN_DUNGEON)
        return;

    long marketCount = 0;
    for (long townIndex = 0; townIndex < g_currentPlayer->m_numTowns;
         ++townIndex) {
        town* ownedTown = g_game->getTown(
            g_currentPlayer->m_townIds[townIndex]);
        if (ownedTown->hasBuilding(MARKETPLACE_ID, 1))
            ++marketCount;
    }
    if (marketCount > 10)
        marketCount = 10;

    if (!currentTown->hasBuilding(SPECIAL_BUILDING_ID, 1)) {
        if (!currentTown->canBuild(SPECIAL_BUILDING_ID))
            return;

        long artifactValue = 0;
        int* costs =
            currentTown->getBuildCostArray(SPECIAL_BUILDING_ID);
        long funds[NUM_RESOURCES];
        for (int resource = 0; resource < NUM_RESOURCES; ++resource) {
            funds[resource] =
                g_currentPlayer->m_resources[resource] - costs[resource];
            if (funds[resource] < 0)
                return;
        }

        for (int artifactIndex = 0; artifactIndex < 7;
             ++artifactIndex) {
            artifactValue += getArtifactPurchaseValue(
                g_game->m_marketArtifacts[artifactIndex],
                marketCount, funds);
        }

        if (artifactValue < aiResourceCost(g_netLocalGamePos, costs))
            return;
        currentTown->buyBuilding(SPECIAL_BUILDING_ID);
    }

    buyArtifacts(currentHero, g_game->m_marketArtifacts, marketCount);
}

VA(0x00525bf0, 0xac)  // dc 0x113c1c
long valueOfUniversity(const hero* currentHero,
                         type_university* university,
                         unsigned char mustPay)
{
    if (currentHero->m_skillCount >= 8)
        return 0;
    if (mustPay && g_currentPlayer->m_resources[GOLD] < 2000)
        return 0;

    const THeroClassTraits* traits =
        &g_heroClasses[currentHero->m_heroClass];
    long total = 0;
    for (int i = 0; i < 4; i++) {
        int skill = university->m_skills[i];
        if (traits->m_gainSecondarySkillChance[skill]
            && currentHero->m_skillLevel[skill] <= 0
            && wantsSkill(currentHero, TSecondarySkill(skill), 1))
            total += getSkillValue(currentHero, TSecondarySkill(skill), 1);
    }
    return total;
}

VA(0x00525ca0, 0x11e)  // dc 0x10e118
void buySiegeEngine(hero* currentHero, town* currentTown,
                      type_building_id building, TArtifact engine)
{
    if (currentHero->hasArtifact(engine))
        return;

    long value = aiGetValueOfArtifact(
        type_artifact(engine), currentHero, false, true);
    TCreatureType creature = siegeArtifactToCreature(engine);
    const int* costs = g_creatureTypeTraits[creature].m_cost;
    if (!value)
        return;

    for (int resource = 0; resource < 7; ++resource) {
        if (g_currentPlayer->m_resources[resource] < costs[resource])
            return;
    }

    if (!currentTown->hasBuilding(building, 1)) {
        if (!currentTown->buyBuilding(building))
            return;
        for (int checkResource = 0; checkResource < 7; ++checkResource) {
            if (g_currentPlayer->m_resources[checkResource]
                    < costs[checkResource])
                return;
        }
    }

    for (int costResource = 0; costResource < 7; ++costResource)
        g_currentPlayer->m_resources[costResource] -= costs[costResource];

    currentHero->giveArtifact(&type_artifact(engine), 1, 1);
}

VA(0x00525dc0, 0xB1)  // dc 0x10e678
void aiFriendlyHeroMeeting(hero* currentHero, hero* secondHero)
{
    type_AI_creature_swapper swapper;
    short currentSkill = currentHero->getPrimarySkillTotal();
    short secondSkill = secondHero->getPrimarySkillTotal();

    if (secondSkill < currentSkill) {
        unsigned char hasAngelicAlliance =
            g_game->m_players[currentHero->m_owner].hasGivenArtifact(
                ARTIFACT_ANGELIC_ALLIANCE);
        swapper.doSwap(currentHero, &secondHero->m_army, secondHero,
                        hasAngelicAlliance);
    } else if (secondSkill > currentSkill) {
        unsigned char hasAngelicAlliance =
            g_game->m_players[secondHero->m_owner].hasGivenArtifact(
                ARTIFACT_ANGELIC_ALLIANCE);
        swapper.doSwap(secondHero, &currentHero->m_army, currentHero,
                        hasAngelicAlliance);
    }

    aiSwapArtifacts(currentHero, secondHero);
    aiSwapArtifacts(secondHero, currentHero);
}

// E:\gamedcs\philai.cpp:1261.  Retail inlines the whole per-hero move loop
// (move_all_heroes / DetermineHeroToMove path) into DoAI, so the body is ~4.5x
// the DC size; identity is proven by the philai-unique start_turn/end_turn/
// UpdBottomView edges, not by size.
void moveHero(hero* currentHero, long* dangerZones,
              unsigned char isLastHero, unsigned char* exploreMode);
hero* determineHeroToMove(int playerId, unsigned char* isLastHero);

// Residual (90.2542%): ONE inline decision, and the obvious repair is
// MEASURED AND REJECTED.  Retail CALLS game::GetTown (+2a0) and game::GetHero
// (+2c1) inside the expanded second selector; this compile expands both
// game.h accessors in place, which is also where our two surplus branches
// come from (44 against retail's 42 - the `cmp id,-1` arm of each accessor).
// They sit at inline depth 2 in retail, inside an expanded
// DetermineHeroToMove, and at depth 1 here because the helper's body is
// written out below.  Restoring `DetermineHeroToMove(whichPlayer,
// &is_last_hero)` as the call does NOT reproduce that: VC6 declines to expand
// it at all once the hand-written mass leaves caller_cb, emitting a plain call
// and taking the row to 25.9647% (20 branches against 42).  The expansion is
// the mass that justifies its own inlining, so the two accessors cannot be
// pushed to depth 2 by any statement in this body.
// 2026-09-05, easy lane 3 - the census names it exactly: retail CALLS
// game::GetTown at +0x2a0 and game::GetHero at +0x2c1 inside the second
// hero-selection copy, where this compile expands both (16 calls same, 2
// target-only, nothing base-only). That is the OVER-inline direction, so the
// knob is caller_cb, not a pin. Measured and rejected: lifting the whole
// garrison-promotion block (numHeroes guard + town sweep) into a
// single-call-site file static - the dose OVERSHOOTS badly, 90.2542 ->
// 79.4310, and the two calls do NOT come back out of line.
// For the next lane: the DC roster has a static `move_all_heroes` (156 B, dc
// 0x10f0d0) sitting immediately before DoAI, and DC's DoAI is only 192 B
// against retail's 866. So retail's source almost certainly calls
// move_all_heroes TWICE and /Ob2 expands it twice with different nested
// decisions - which is exactly the note below about the selector being
// retained in the first copy and expanded in the second. Reconstructing that
// helper is the real fix, and it is a reconstruction job, not polish.
VA(0x00525e80, 0x362)  // anchor-callee, dc 0x10f16c
void philAI::doAI(int whichPlayer)
{
    pollSound();
    g_advManager->updBottomView(0, 1, 1);

    if (!g_gameOver
        && (!g_unnamed6994f0 || whichPlayer == g_unnamed6994f0)) {
        long* dangerZones = new long[
            g_mapWidth * g_mapHeight * g_game->m_worldMap.getNumLevels()];
        type_AI_player* aiPlayer = &g_aiPlayers[whichPlayer];
        aiPlayer->startTurn();
        getTurnAIVars(whichPlayer);

        incrementHourGlass();

        unsigned char isLastHero = 0;
        unsigned char exploreMode = 1;
        if (!g_game->m_setup.m_difficulty || !g_currentPlayer->m_numTowns)
            exploreMode = 0;

        hero* currentHero =
            determineHeroToMove(whichPlayer, &isLastHero);
        while (currentHero) {
            moveHero(currentHero, dangerZones, isLastHero,
                     &exploreMode);
            if (g_gameOver)
                break;
            currentHero =
                determineHeroToMove(whichPlayer, &isLastHero);
        }
        g_advManager->demobilizeCurrHero(0, 1);
        aiPlayer->endTurn();

        exploreMode = 1;
        if (!g_game->m_setup.m_difficulty || !g_currentPlayer->m_numTowns)
            exploreMode = 0;

        // VC6 expands the selector into the second copy of the source helper,
        // although it retains the selector call in the first copy above.
        // Spell this call site out so that legitimate nested-inline decision
        // remains visible without forcing DetermineHeroToMove everywhere.
        for (;;) {
            hero* selectedHero = 0;
            short lowestSum = 0;
            playerData* player = &g_game->m_players[whichPlayer];
            unsigned char isLastHero = 1;

            for (short heroIndex = 0;
                 heroIndex < player->m_numHeroes; ++heroIndex) {
                hero* currentHero = &g_game->m_heroes[
                    static_cast<short>(player->m_heroes[heroIndex])];
                if (currentHero->m_movePoints > 0
                    && !currentHero->m_isSleeping) {
                    if (selectedHero)
                        isLastHero = 0;

                    short skillSum = 0;
                    for (short skill = 0; skill < 4; ++skill)
                        skillSum += currentHero->getPrimarySkill(skill);

                    if (!selectedHero
                        || (!(currentHero->m_patrolX != hero::kPatrolNone
                                 && selectedHero->m_patrolX
                                        == hero::kPatrolNone)
                            && ((currentHero->m_patrolX == hero::kPatrolNone
                                     && selectedHero->m_patrolX
                                            != hero::kPatrolNone)
                                || skillSum < lowestSum))) {
                        selectedHero = currentHero;
                        lowestSum = skillSum;
                    }
                }
            }

            if (!selectedHero) {
                isLastHero = 0;
                g_advManager->demobilizeCurrHero(0, 1);
                player->m_currHeroId = -1;
                if (player->m_numHeroes < playerData::HERO_SLOT_COUNT) {
                    for (short townIndex = 0;
                         townIndex < player->m_numTowns; ++townIndex) {
                        town* currentTown =
                            g_game->getTown(player->m_townIds[townIndex]);
                        short garrisonHeroId = static_cast<short>(
                            currentTown->m_garrisonHeroId);
                        if (garrisonHeroId >= 0
                            && currentTown->m_visitingHeroId < 0) {
                            hero* currentHero =
                                g_game->getHero(garrisonHeroId);
                            if (currentHero->m_army.getCreatureTotal()
                                && currentHero->m_movePoints
                                && !currentHero->m_isSleeping) {
                                currentTown->removeGarrisonHero();
                                selectedHero = currentHero;
                                break;
                            }
                        }
                    }
                }
            }

            if (!selectedHero)
                break;
            moveHero(selectedHero, dangerZones, isLastHero,
                     &exploreMode);
            if (g_gameOver)
                break;
        }
        g_advManager->demobilizeCurrHero(0, 1);
        delete [] dangerZones;
    }

    g_game->checkHeroConsistency();
    g_mouseManager->showPointer(1);
}

// E:\gamedcs\philai.cpp:1056
VA(0x005261f0, 0x5ba)  // anchor-callee, dc 0x10ec58
void moveHero(hero* currentHero, long* dangerZones, unsigned char isLastHero, unsigned char* exploreMode)
{
    unsigned char mouseWasVisible = g_mouseManager->isVis();
    playerData* player = &g_game->m_players[currentHero->m_owner];

    checkForTown(currentHero);
    if (currentHero->m_patrolX != hero::kPatrolNone
        && currentHero->m_patrolRadius == 0) {
        currentHero->m_movePoints = 0;
        return;
    }
    if (g_gameOver)
        return;

    g_unnamed69ccd4 = 0;
    g_advManager->m_advWindow->animateBottomView(0);
    if (currentHero->m_movePoints > 0) {
        if (!g_unnamed698790 && !g_videoPaused
            && mapExtraPosAndAdjacentsSet(
                currentHero->m_x, currentHero->m_y, currentHero->m_z,
                g_mapVisibilityBit))
            g_completeDrawEnabled = 1;
        else
            g_completeDrawEnabled = 0;

        if (g_game->getCurrHero() == currentHero) {
            isLastHero = 0;
        } else {
            g_advManager->demobilizeCurrHero(0, 1);
            g_advManager->setHeroContext(currentHero->m_id, 1, 0, 1);
            memset(dangerZones, 0,
                   g_game->getNumMapLevels() * g_mapWidth * g_mapHeight
                       * sizeof(long));
            if (g_game->m_setup.m_difficulty > 0
                || g_game->isHumanAlly(
                    g_game->getTeam(g_netLocalGamePos)))
                aiMarkDangerZones(currentHero, dangerZones);
        }

        markShipyards(player);
        g_searchArray->setDangerZones(dangerZones);
        moveHero(currentHero, isLastHero, *exploreMode);
        g_searchArray->setDangerZones(0);
        clearShipyards(player);
    }

    if (currentHero->m_owner >= 0
        && (currentHero->m_movePoints <= 0 || currentHero->m_isSleeping)) {
        int townId = g_game->getTownId(currentHero->m_x, currentHero->m_y,
                                        currentHero->m_z);
        if (townId >= 0) {
            town* currentTown = g_game->getTown(townId);
            g_advManager->demobilizeCurrHero(0, 1);
            if (currentTown->m_owner == currentHero->m_owner) {
                if (currentTown->m_garrisonHeroId < 0) {
                    player->addGarrisonHero(currentTown);
                } else {
                    hero* garrisonHero = g_game->getHero(
                        currentTown->m_garrisonHeroId);
                    if ((garrisonHero->m_movePoints > 0
                         && !garrisonHero->m_isSleeping)
                        || (static_cast<const town*>(currentTown)
                                    ->getArmy().getCreatureTotal() > 0
                            && static_cast<const town*>(currentTown)
                                       ->getArmy().getAIValue()
                                   < currentHero->m_army.getAIValue()))
                        currentTown->swapHeroes();
                }
            }
        }
    }

    restoreMouse(mouseWasVisible);
}

VA(0x005267b0, 0x2da)  // dc 0x10e9a8
void moveHero(hero* currentHero, unsigned char isLastHero,
               unsigned char& exploreMode)
{
    HeroDestination destination;
    type_point oldTarget = currentHero->getTarget();
    long rawValue;
    type_point originalDestination;
    int rv;

    long maximumDistance = 32000;
    if (currentHero->m_pathTargetX < 0)
        currentHero->m_targetIsCritical = 0;
    destination.m_point.m_x = -1;
    destination.m_isNearby = 0;
    rawValue = 0;

    long maxDistance = 1000;
    if (currentHero->m_pathTargetX >= 0) {
        maxDistance = max(
            currentHero->m_movePoints
                + static_cast<unsigned short>(currentHero->m_targetDistance)
                + 200,
            1000);
    }
    if (isLastHero)
        maxDistance = 32000;

    if (currentHero->m_patrolX != hero::kPatrolNone) {
        maximumDistance = 200
            * (abs(currentHero->m_x - currentHero->m_patrolX)
               + abs(currentHero->m_y - currentHero->m_patrolY)
               + currentHero->m_patrolRadius);
        maxDistance = min(maxDistance, maximumDistance);
    }

    for (int attempt = 0; attempt < 5; ++attempt) {
        rv = aiChooseDestination(currentHero, maxDistance, destination,
                                   rawValue, 1, exploreMode);
        if (!((destination.m_point.m_x < 0
               || (rv < 0 && !destination.m_isCritical))
              && g_searchArray->limitWasReached()
              && maxDistance < maximumDistance))
            break;
        maxDistance = min(maxDistance * 2, maximumDistance);
    }

    if (currentHero->m_maxMovePoints == currentHero->m_movePoints)
        incrementHourGlass();

    if (destination.m_point.m_x < 0) {
        currentHero->m_pathTargetX = -1;
        currentHero->m_pathTargetY = -1;
        currentHero->m_isSleeping = 1;
        return;
    }

    originalDestination = destination.m_point;
    unsigned char destinationWasUnvisited =
        !(getMapExtra(originalDestination.m_x, originalDestination.m_y,
                      originalDestination.m_z)
          & g_unnamed69ccc4);
    int townId = g_game->getTownId(currentHero->m_x, currentHero->m_y,
                                    currentHero->m_z);
    if (townId != -1) {
        g_game->getTown(townId);
        if (rv < 75
            && g_game->m_day == philAI::AI_HERO_MOVE_SLEEP_DAY) {
            currentHero->m_isSleeping = 1;
            return;
        }
    }

    g_unnamed69ccd4 = 1;
    aiAttemptMove(currentHero, destination, rawValue, exploreMode);

    if (destinationWasUnvisited && exploreMode
        && (getMapExtra(originalDestination.m_x, originalDestination.m_y,
                        originalDestination.m_z)
            & g_unnamed69ccc4)) {
        g_aiPlayers[g_netLocalGamePos].resetMagusHutValue();
        exploreMode = 0;
        for (int i = 0; i < g_currentPlayer->m_numHeroes; ++i)
            g_game->getHero(g_currentPlayer->m_heroes[i])->m_isSleeping = 0;
    }
}

VA(0x00526a90, 0x1d4)  // dc 0x10eeb0
hero* determineHeroToMove(int playerId, unsigned char* isLastHero)
{
    hero* selectedHero = 0;
    short lowestSum = 0;
    playerData* player = &g_game->m_players[playerId];
    *isLastHero = 1;

    for (short heroIndex = 0; heroIndex < player->m_numHeroes; ++heroIndex) {
        hero* currentHero =
            &g_game->m_heroes[static_cast<short>(player->m_heroes[heroIndex])];
        if (currentHero->m_movePoints > 0 && !currentHero->m_isSleeping) {
            if (selectedHero)
                *isLastHero = 0;

            short skillSum = 0;
            for (short skill = 0; skill < 4; ++skill)
                skillSum += currentHero->getPrimarySkill(skill);

            if (!selectedHero
                || (!(currentHero->m_patrolX != hero::kPatrolNone
                         && selectedHero->m_patrolX == hero::kPatrolNone)
                    && ((currentHero->m_patrolX == hero::kPatrolNone
                             && selectedHero->m_patrolX
                                    != hero::kPatrolNone)
                        || skillSum < lowestSum))) {
                selectedHero = currentHero;
                lowestSum = skillSum;
            }
        }
    }

    if (selectedHero)
        return selectedHero;

    *isLastHero = 0;
    g_advManager->demobilizeCurrHero(0, 1);
    player->m_currHeroId = -1;
    if (player->m_numHeroes < playerData::HERO_SLOT_COUNT) {
        for (short townIndex = 0; townIndex < player->m_numTowns;
             ++townIndex) {
            town* currentTown = g_game->getTown(player->m_townIds[townIndex]);
            short garrisonHeroId =
                static_cast<short>(currentTown->m_garrisonHeroId);
            if (garrisonHeroId >= 0 && currentTown->m_visitingHeroId < 0) {
                hero* currentHero = g_game->getHero(garrisonHeroId);
                if (currentHero->m_army.getCreatureTotal()
                    && currentHero->m_movePoints
                    && !currentHero->m_isSleeping) {
                    currentTown->removeGarrisonHero();
                    selectedHero = currentHero;
                    break;
                }
            }
        }
    }
    return selectedHero;
}

VA(0x00526c70, 0x48)  // dc 0x10f22c
int aiResourceCost(const playerData* player, const int* resources);

VA(0x00526cc0, 0x55)  // dc 0x10f2f8
int aiResourceCost(long playerId, const int* resources);

// Complete's computer-owner purchase wrapper; its declaration belongs to
// philai.h with this definition. Retail buy_building calls it at 0x5bf476,
// then tests builtThisTurn at 0x5bf47b. DC buy_building instead proceeds
// from get_build_cost_array (dc 0x1672fc) to the resource checks/debits
// (source 1451..1459), without the new computer-owner trading path.
// Retail 0x526d2e selects g_aiPlayers[playerId] with a 152-byte stride,
// calls trade_resources at 0x526d35, and returns with ret 4.
VA(0x00526d20, 0x1e)  // anchor-callee type_AI_player::trade_resources + sole caller town::buy_building 0x5bf3c0
void unnamed526d20(int playerId, int* costs, int flag)
{
    g_aiPlayers[playerId].tradeResources(costs, flag);
}

// E:\gamedcs\philai.cpp:1339, dc 0x10f37c
VA(0x00526d40, 0x393)  // anchor-global, dc 0x10f37c
type_spellvalue::type_spellvalue(const hero* newHero)
{
    m_ourHero = newHero;
    if (!newHero->isWieldingArtifact(
            ARTIFACT_SPELLBOOK)
        || m_ourHero->isWieldingArtifact(
            ARTIFACT_ORB_OF_INHIBITION)) {
        m_power = 0;
        m_duration = 0;
        m_mana = 0;
        m_stackValue = 0;
    } else {
        m_stackValue = m_ourHero->m_army.getAIValue();
        m_power = m_ourHero->getPrimarySkill(2);
        m_duration = m_power
            + m_ourHero->getSpellDurationBonus();
        m_mana = const_cast<hero*>(m_ourHero)->getMaxMana();
        fillCreatureValueList();
    }
}

// The AI's spell-appraisal curve, one 32-byte row per "times castable"
// step.  Retail reaches all four columns off the same 32-byte stride
// (`row << 5` plus 0x640398 / +8 / +0x10 / +0x18), so it is ONE array of
// four-double rows, not four arrays: get_damage_spell_value walks column
// `threshold` to pick a row and then evaluates `slope * ratio +
// intercept` out of that same row, while both it and the summoning arm
// of get_raw_spell_value index column `cast_curve` by the (clamped)
// cast count.  Rows 1..12 of the slope/intercept pair are continuous at
// their own thresholds, which is what makes this a piecewise-linear
// curve rather than four unrelated tables.
DATA(0x00640398)
static const struct {
    double m_castCurve;
    double m_threshold;
    double m_slope;
    double m_intercept;
} g_spellValueCurve[14] = {
    { 0.0,  4.8,   0.0,   4.0   },
    { 0.83, 1.557, 0.5,   1.6   },
    { 1.53, 0.752, 0.987, 0.842 },
    { 2.11, 0.433, 1.452, 0.492 },
    { 2.59, 0.275, 1.888, 0.303 },
    { 2.99, 0.186, 2.291, 0.192 },
    { 3.33, 0.131, 2.657, 0.124 },
    { 3.6,  0.095, 2.986, 0.081 },
    { 3.84, 0.071, 3.277, 0.053 },
    { 4.03, 0.054, 3.353, 0.035 },
    { 4.19, 0.041, 3.755, 0.023 },
    { 4.33, 0.032, 3.947, 0.016 },
    { 4.44, 0.025, 4.112, 0.01  },
    { 4.53, 0.0,   4.53,  0.0 }
};

VA(0x005270e0, 0xDF)  // dc 0x10f404
long type_spellvalue::getDamageSpellValue(SpellID spell, TSkillMastery mastery,
    long timesCastable, long combatValue) const
{
    long damage = g_spellTraits[spell].m_masteryValues[mastery]
        * (m_power + mastery);
    if (spell == SPELL_TITANS_LIGHTNING_BOLT)
        damage = g_spellTraits[SPELL_TITANS_LIGHTNING_BOLT].m_masteryBonus[0];
    damage = const_cast<hero*>(m_ourHero)->modifySpellDamage(spell, damage, 0);

    double ratio = static_cast<double>(damage * 10);
    ratio /= static_cast<double>(combatValue);
    if (timesCastable >= 14)
        timesCastable = 13;

    long row = 0;
    while (row < 13 && ratio < g_spellValueCurve[row].m_threshold)
        row++;

    double byRatio = ratio * g_spellValueCurve[row].m_slope
        + g_spellValueCurve[row].m_intercept;
    double byCount = ratio * g_spellValueCurve[timesCastable].m_castCurve;
    if (byRatio > byCount)
        return static_cast<long>(static_cast<double>(combatValue) * byCount);
    return static_cast<long>(static_cast<double>(combatValue) * byRatio);
}

VA(0x005271c0, 0xC1)  // dc 0x10f648
long type_spellvalue::getMassDamageSpellValue(SpellID spell, TSkillMastery mastery,
    long timesCastable) const
{
    long total = 0;
    for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; i++) {
        int creature = m_ourHero->m_army.m_armies[i];
        if (creature != CREATURE_NONE) {
            double chance;
            {
                TCreatureType creatureType;
                int ordinal = creature;
                memcpy(&creatureType, &ordinal, sizeof creatureType);
                chance = getSpellWorkChance(spell, creatureType, m_ourHero, 0);
            }
            total = static_cast<long>(
                static_cast<double>(g_creatureTypeTraits[creature].m_aiValue)
                * static_cast<double>(m_ourHero->m_army.m_numTroops[i])
                * (1.0 - chance)
                + static_cast<double>(total));
        }
    }
    if (total == 0)
        return 0;

    long value = total - m_stackValue;
    value += getDamageSpellValue(spell, mastery, timesCastable, total);
    return value < 0 ? 0 : value;
}

VA(0x00527290, 0x134)  // dc 0x10f810
long type_spellvalue::getEnchantmentValue(SpellID spell, TSkillMastery mastery,
    long timesCastable) const
{
    const SSpellTraits* traits = &g_spellTraits[spell];
    unsigned char coversWholeArmy = !spellTargetsASingleArmy(spell, mastery);
    long totalDuration = m_duration * timesCastable;
    if (coversWholeArmy) {
        timesCastable = m_list.size();
        if (totalDuration > 7)
            totalDuration = 7;
    } else if (totalDuration < 7) {
        timesCastable = 1;
    } else {
        timesCastable = totalDuration / 7;
        if (timesCastable > m_duration)
            timesCastable = m_duration;
        totalDuration = 7;
    }

    long total = 0;
    for (long i = 0; i < m_list.size() && i < timesCastable; i++) {
        if (traits->m_karma > 0
            && getSpellWorkChance(spell, m_list[i].m_type, m_ourHero, 0) == 0.0)
            continue;
        total += traits->m_masteryValues[mastery] * m_list[i].m_value
            * totalDuration;
    }
    return total / 700;
}

VA(0x005273d0, 0x1DD)  // dc 0x10fa84
long type_spellvalue::getRawSpellValue(SpellID spell) const
{
    const SSpellTraits* traits = &g_spellTraits[spell];
    hero* caster = const_cast<hero*>(m_ourHero);
    int mastery = caster->getSpellLevel(spell, caster->getSpecialTerrain());
    int cost = const_cast<hero*>(m_ourHero)->getManaCost(spell, 0, -1);
    if (cost > m_mana)
        return 0;

    long timesCastable = cost > 0 ? m_mana / cost : 1000;
    switch (traits->m_flags & SPELL_VALUE_CLASS_MASK) {
    case SPELL_VALUE_DAMAGE:
        return getDamageSpellValue(spell, TSkillMastery(mastery), timesCastable,
                                      m_stackValue);
    case SPELL_VALUE_DAMAGE_ONCE:
        return getDamageSpellValue(spell, TSkillMastery(mastery), 1,
                                   m_stackValue);
    case SPELL_VALUE_MASS_DAMAGE:
        return getMassDamageSpellValue(spell, TSkillMastery(mastery),
                                       timesCastable);
    case SPELL_VALUE_ENCHANTMENT:
        return getEnchantmentValue(spell, TSkillMastery(mastery),
                                   timesCastable);
    case SPELL_VALUE_SUMMONING: {
        long damage = traits->m_masteryValues[mastery] * (m_power + mastery);
        double damageTaken = static_cast<double>(damage * 10);
        double ratio = damageTaken / static_cast<double>(m_stackValue);
        if (ratio > 0.9)
            ratio = 0.9;
        if (timesCastable >= 14)
            timesCastable = 13;
        ratio *= g_spellValueCurve[timesCastable].m_castCurve;
        if (ratio > 3.9)
            ratio = 3.9;
        return static_cast<long>(static_cast<double>(m_stackValue) * ratio);
    }
    case SPELL_VALUE_SPECIAL:
        return static_cast<long>(
            (sqrt(static_cast<double>(timesCastable)) * 0.001 + 0.009)
            * static_cast<double>(traits->m_masteryValues[mastery]
                                  * m_stackValue));
    }
    return 1;
}

VA(0x005275b0, 0x8A)  // dc 0x10fcf0
long type_spellvalue::getBestSpellValue(long bits) const
{
    long best = 0;
    unsigned char capped = 0;
    if (m_ourHero->isWieldingArtifact(
            g_artifactRecantersCloak))
        capped = 1;
    if (m_power == 0)
        return 0;

    for (int spell = 0; spell < hero::NUM_SPELLS; spell++) {
        if (!m_ourHero->spellIsAvailable(spell))
            continue;
        if (!(g_spellTraits[spell].m_flags & bits))
            continue;
        if (capped && g_spellTraits[spell].m_level > 2)
            continue;
        long value = getRawSpellValue(spell);
        if (value > best)
            best = value;
    }
    return best;
}

VA(0x00527640, 0xCB)  // dc 0x10fd78
long aiGetSpellValue(const hero* ourHero, SpellID spell)
{
    type_spellvalue value(ourHero);
    if (!value.canCastSpells())
        return 0;

    long raw = value.getRawSpellValue(spell);
    long damageClass = g_spellTraits[spell].m_flags & SPELL_VALUE_ANY_DAMAGE;
    if (damageClass == 0)
        return raw;

    long best = value.getBestSpellValue(damageClass);
    if (best >= raw)
        return 1;
    return raw - best;
}

VA(0x00527710, 0x4C)  // dc 0x10feb8
float valueOfExperience(const hero* currentHero, const armyGroup& currentArmy)
{
    int increment = currentHero->getExperienceIncrement();
    float armyValue = float(currentArmy.getAIValue());
    return (float(g_heroGoldCost) + armyValue) / float(increment * 40);
}

VA(0x00527760, 0x1f2)  // dc 0x10fef4
void aiSetHeroBonuses(hero* ourHero)
{
    type_spellvalue caster(ourHero);

    ourHero->m_turnExperienceToRvRatio =
        valueOfExperience(ourHero, ourHero->m_army);

    long baseValue = caster.getBestSpellValue(SPELL_VALUE_CLASS_MASK);
    long value =
        max(caster.getValueOfIncrease(baseValue, 1, 1, 0), 10);
    ourHero->setValueOfPower(value);
    value = caster.getValueOfIncrease(baseValue, 0, 1, 0);
    ourHero->setValueOfDuration(value);
    value = max(
        caster.getValueOfIncrease(baseValue, 0, 0, 30) / 3, 10);
    ourHero->setValueOfKnowledge(value);

    long initialMana = caster.getMana();
    caster.setMana(ourHero->m_mana);
    baseValue = caster.getBestSpellValue(SPELL_VALUE_CLASS_MASK);

    if (ourHero->m_mana >= initialMana)
        ourHero->setValueOfWell(0);
    else
        ourHero->setValueOfWell(caster.getValueOfIncrease(
            baseValue, 0, 0, initialMana - ourHero->m_mana));

    if (ourHero->m_mana >= 2 * initialMana)
        ourHero->setValueOfSpring(0);
    else
        ourHero->setValueOfSpring(caster.getValueOfIncrease(
            baseValue, 0, 0, 2 * initialMana - ourHero->m_mana));
}

// E:\gamedcs\philai.cpp:1770. Retail retains the DC hero-bonus sweep and
// extends its artifact appraisal across Complete's full 144-entry table.
// The older build calls AI_get_value_of_artifact in the artifact loop;
// Complete's relocation instead names the exact retail-only
// AI_get_artifact_player_value helper. Keep this revision local: the
// independently exact value_of_obelisk still calls the older overload.

// Residual (97.8723%): all 13 CFG blocks and the 94-instruction multiset
// agree.  Retail reloads the staged double numerator before converting the
// divisor; VC6 schedules that reload immediately afterward. why-reg reports
// only a two-slot B16/C3/C4 transpose: its 16 guided mutations are flat or
// worse. Naming the numerator is byte-flat, while carrying it through a
// second average local falls to 95.57%, so the direct DC-shaped expression
// remains the strongest defensible spelling.
VA(0x00527960, 0x140)  // anchor-callee, dc 0x110018
void philAI::getTurnAIVars(int whichPlayer)
{
    g_curHourGlassPhase = 0;
    g_unnamed691680 = 0;

    for (int heroIndex = 0;
         heroIndex < g_currentPlayer->m_numHeroes; ++heroIndex) {
        aiSetHeroBonuses(
            g_game->getHero(g_currentPlayer->m_heroes[heroIndex]));
    }

    long artifactCount = 0;
    long totalArtifactValue = 0;
    for (int artifactId = ARTIFACT_FIRST_AID_TENT + 1;
         artifactId < ARTIFACT_COUNT; ++artifactId) {
        if (!g_artifactTraits[artifactId].m_disabled) {
            ++artifactCount;
            TArtifact artifactType;
            {
                int ordinal = artifactId;
                memcpy(&artifactType, &ordinal, sizeof artifactType);
            }
            type_artifact artifact(artifactType);
            totalArtifactValue +=
                aiGetValueOfArtifact(artifact, whichPlayer);
        }
    }
    g_currentPlayer->m_ai.m_turnValueOfAvgArtifact =
        static_cast<float>(static_cast<double>(totalArtifactValue)
                           / static_cast<double>(artifactCount));

    if (!g_game->m_setup.m_difficulty) {
        type_AI_player::setAttackBonuses(1.0f, -0.4f);
        return;
    }

    float difficulty = static_cast<float>(g_game->m_setup.m_difficulty);
    float humanBonus = (difficulty + 1.0f) * 0.25f;
    float computerBonus = 0.75f - difficulty * 0.25f;
    type_AI_player::setAttackBonuses(computerBonus, humanBonus);
}

VA(0x00527aa0, 0x56)  // dc 0x110574
int hero::valueOfSpell(SpellID spell) const
{
    if (g_spellTraits[spell].m_level <= m_skillLevel[eSecSkillWisdom] + 2
        && !m_inSpellbook[spell]
        && this->isWieldingArtifact(ARTIFACT_SPELLBOOK))
        return aiGetSpellValue(this, spell);
    return 0;
}

VA(0x00527b00, 0xa4)  // dc 0x110c04
void aiPurchaseCreatures(hero* currentHero, generator* currentGenerator)
{
    type_AI_creature_purchaser purchaser(currentHero->m_owner,
                                         currentGenerator);
    unsigned char hasAngelicAlliance =
        g_game->m_players[currentHero->m_owner].hasGivenArtifact(
            ARTIFACT_ANGELIC_ALLIANCE);
    purchaser.doPurchase(&currentHero->m_army,
                          currentHero->getMorale(0, 0, 1), 0,
                          g_currentPlayer->m_resources, 1,
                          hasAngelicAlliance);
}

#if 0  // @carcass

// E:\gamedcs\philai.cpp:1972
DC_ONLY(0x1105d8, 0x230)
int valueOfBlackBox(const hero* current_hero, NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2115
DC_ONLY(0x1108e0, 0xD6)
int valueOfCampfire(playerData* player, NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2194
DC_ONLY(0x110c58, 0x36)
int valueOfDefenseTower(const hero* current_hero, NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2220
DC_ONLY(0x110cf8, 0xBC)
long valueOfGarrison(const hero* current_hero, NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2250
DC_ONLY(0x110db4, 0x1D4)
long valueOfIdol(const hero* current_hero, long move_cost)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2274
DC_ONLY(0x110f88, 0x7C)
int valueOfFlotsam(playerData* player)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2283
DC_ONLY(0x111004, 0x24)
int valueOfGarden(const hero* current_hero, NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2294
DC_ONLY(0x111028, 0x32)
int valueOfLeanTo(NewmapCell* cell, playerData* player)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2392
DC_ONLY(0x111328, 0x1CC)
long valueOfHeroEvent(const hero* current_hero, NewmapCell* cell, short x, short y, short z, short move_cost)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2465
DC_ONLY(0x1114f4, 0x13A)
long valueOfHillFort(const hero* current_hero, long move_cost)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2513
// Retail claim promoted to the reconstructed body below; DC identity retained.
void aiVisitHillFort(hero* current_hero)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2550
DC_ONLY(0x11173c, 0x94)
int valueOfLibrary(const hero* current_hero, NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2567
DC_ONLY(0x1117d0, 0x36)
int valueOfLighthouse(NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2615
DC_ONLY(0x1118f8, 0x78)
int valueOfMercenaryCamp(const hero* current_hero, NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2692
DC_ONLY(0x111b7c, 0xC6)
int moraleIncreaseValue(const hero* current_hero, int value)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2728
DC_ONLY(0x111c78, 0xF0)
int luckIncreaseValue(const hero* current_hero, int value)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2775
DC_ONLY(0x111e18, 0x1C)
long valueOfMagusHut(long player_id)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2948
DC_ONLY(0x112488, 0x88)
int valueOfSkeleton(const hero* current_hero, NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2997
DC_ONLY(0x1125ec, 0x1E)
int valueOfShrine(const hero* current_hero, NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:3044
DC_ONLY(0x1126d0, 0x9C)
int valueOfSirens(const hero* current_hero)
{
    // @stub
}

// E:\gamedcs\philai.cpp:3069
DC_ONLY(0x11276c, 0xC4)
int valueOfStables(const hero* current_hero, long* move_cost)
{
    // @stub
}

#endif  // @carcass

VA(0x00527bb0, 0x11D)  // dc 0x111630
void aiVisitHillFort(hero* currentHero)
{
    long cost[NUM_RESOURCES];

    for (long i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; i++) {
        TCreatureType creature = currentHero->m_army.m_armyTypes[i];
        if (creature == CREATURE_NONE)
            continue;
        if (g_game->m_f1f698 == 0
            && isBaseElemental(creature))
            continue;

        TCreatureType upgrade = upgradedCreatureType(creature);
        if (upgrade == CREATURE_NONE)
            continue;

        getUpgradeCost(creature, upgrade,
                         currentHero->m_army.m_numTroops[i], cost);
        cost[GOLD] = static_cast<int>(static_cast<float>(cost[GOLD])
            * g_afUpgradeCostFactor[g_creatureTypeTraits[creature].m_level]);

        int resource;
        for (resource = 0; resource <= GOLD; resource++) {
            if (cost[resource] > g_currentPlayer->m_resources[resource])
                break;
        }

        if (resource > GOLD) {
            for (resource = 0; resource <= GOLD; resource++)
                g_currentPlayer->m_resources[resource] -= cost[resource];
            currentHero->m_army.m_armyTypes[i] = upgrade;
        }
    }
}

VA(0x00527cd0, 0x1b)  // dc 0x111808
TPrimarySkill aiChooseMagicSkill(hero* currentHero)
{
    return currentHero->getValueOfPower() < currentHero->getValueOfKnowledge()
        ? ePriSkillKnowledge : ePriSkillPower;
}

// E:\gamedcs\philai.cpp:2692. Retail changes the DC helper's explicit
VA(0x00527cf0, 0x89)  // dc 0x111b7c
int hero::moraleIncreaseValue(int value)
{
    if (m_army.hasAllUndead())
        return 0;

    double valueAdded = aiValueOfMorale(getMorale(0, 0, 1), value);
    valueAdded *= (getPrimarySkillTotal() + 40)
                   * m_army.getAIValue() / 40;
    return static_cast<int>(valueAdded);
}

VA(0x00527d80, 0x90)  // dc 0x111c78
int hero::luckIncreaseValue(int value)
{
    int luck = getLuck(0, 0, 1);
    if (luck + value > 3) {
        luck = 3 - luck;
        if (value <= 0)
            return 0;
    }

    double valueAdded = aiValueOfLuck(luck, value);
    valueAdded *= (getPrimarySkillTotal() + 40)
                   * m_army.getAIValue() / 40;
    return static_cast<int>(valueAdded);
}

VA(0x00527e10, 0xab)  // dc 0x112208
void aiRecruitRefugees(hero* currentHero, TCreatureType type, short* number)
{
    type_AI_creature_purchaser purchaser(currentHero->m_owner, type, number,
                                         0);
    unsigned char hasAngelicAlliance =
        g_game->m_players[currentHero->m_owner].hasGivenArtifact(
            ARTIFACT_ANGELIC_ALLIANCE);
    purchaser.doPurchase(&currentHero->m_army,
                          currentHero->getMorale(0, 0, 1), 0,
                          g_currentPlayer->m_resources, 1,
                          hasAngelicAlliance);
}

VA(0x00527ec0, 0x93)  // dc 0x11260c
int aiVisitSirens(const hero* currentHero, armyGroup& army)
{
    long total = 0;
    for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; i++) {
        int creature = army.m_armies[i];
        if (creature != CREATURE_NONE) {
            int troops = army.m_numTroops[i];
            if (troops > 1) {
                short sacrifice = static_cast<short>(
                    static_cast<float>(troops) * 0.7);
                army.m_numTroops[i] = sacrifice;
                total += g_creatureTypeTraits[creature].m_hitPoints
                    * (troops - sacrifice);
            }
        }
    }
    return static_cast<int>(total
        * const_cast<hero*>(currentHero)->getExperienceBonusFactor());
}

VA(0x00527f60, 0x73)  // dc 0x112dd4
unsigned char aiBribeMonsters(const hero* currentHero, NewmapCell* cell,
    TCreatureType type, short amount, long goldCost)
{
    armyGroup monsterArmy(type, amount);
    playerData* player = currentHero->getPlayer();
    long bribeWorth = static_cast<long>(monsterArmy.getAIValue()
        - goldCost * player->m_ai.m_resourceValue[GOLD]);
    long fightWorth = aiValueOfCombat(currentHero, 0, monsterArmy, 0, cell);
    return bribeWorth > fightWorth;
}

VA(0x00527fe0, 0x51)  // dc 0x112ee0
unsigned char aiChooseResourceOrExperience(const hero* currentHero,
    EGameResource resource, int amount, int experience)
{
    long experienceValue = static_cast<long>(
        static_cast<float>(experience)
        * currentHero->m_turnExperienceToRvRatio);
    playerData* player = currentHero->getPlayer();
    long resourceValue = static_cast<long>(
        static_cast<double>(amount) * player->m_ai.m_resourceValue[resource]);
    return resourceValue > experienceValue;
}

// E:\gamedcs\philai.cpp:3834.  The per-map-object event valuator: a giant
// object-type dispatch. Dreamcast fixes the player/cell setup, one leading
// absent source line, the arm order below and every named helper boundary.
// Complete adds object variants and widens a few packed fields, but keeps the
// same dispatch at this independently located retail address.
VA(0x00528040, 0x1648)  // anchor-callee + 100-row DC statement shape, dc 0x113e24
long aiValueOfEvent(const hero* currentHero, type_point point,
                       long& moveCost)
{
    playerData* player = currentHero->getPlayer();
    NewmapCell* cell = g_advManager->getCell(point);
    if (!cell->m_isTrigger)
        return 0;

    switch (cell->m_type) {
    case ARENA:
        return valueOfArena(currentHero, cell);
    case ARTIFACT:
        return valueOfMapArtifact(currentHero, cell);
    case BLACK_BOX:
        return valueOfBlackBox(currentHero, cell);
    case BLACK_MARKET:
        return valueOfBlackMarket(currentHero, cell);

    case BORDER_TENT:
        if (g_game->m_borderTentVisitFlags[cell->m_objectIndex]
            & g_unnamed69ccc4)
            return 0;
        return 5000;
    case BUOY:
        if (currentHero->m_flags & 4)
            return 0;
        if (moveCost > currentHero->m_movePoints)
            return 0;
        return const_cast<hero*>(currentHero)->moraleIncreaseValue(1);
    case CAMPFIRE:
        return valueOfCampfire(player, cell);
    case CLOVER_FIELD:
        if (currentHero->m_flags & 8)
            return 0;
        if (moveCost > currentHero->m_movePoints)
            return 0;
        if (moveCost + 200 < currentHero->m_movePoints)
            return 0;
        moveCost = max(moveCost, currentHero->m_movePoints);
        return const_cast<hero*>(currentHero)->luckIncreaseValue(2);
    case CREATURE_BANK:
        return valueOfBank(currentHero, cell);
    case CREATURE_GENERATOR_1:
    case CREATURE_GENERATOR_4:
        return valueOfGenerator(currentHero, point.m_x, point.m_y, point.m_z,
                                cell, moveCost);
    case DEAD_GUY:
        return valueOfSkeleton(currentHero, cell);
    case DEFENSE_TOWER:
        return valueOfDefenseTower(currentHero, cell);
    case DERELICT_SHIP:
    case DRAGON_CITY:
        return valueOfBank(currentHero, cell);
    case FAERIE_RING:
        if (currentHero->m_flags & 0x2000)
            return 0;
        if (moveCost > currentHero->m_movePoints)
            return 0;
        return const_cast<hero*>(currentHero)->luckIncreaseValue(1);
    case FLOTSAM:
        return valueOfFlotsam(player);

    case FOUNTAIN_OF_FORTUNE:
        if (currentHero->m_flags & 0x20)
            return 0;
        if (currentHero->m_flags & 0x8000000)
            return 0;
        if (currentHero->m_flags & 0x10000000)
            return 0;
        if (currentHero->m_flags & 0x20000000)
            return 0;
        if (moveCost > currentHero->m_movePoints)
            return 0;
        if (cell->playerKnowsCell(currentHero->m_owner)) {
            const ExtraInfoUnion* info =
                static_cast<const ExtraInfoUnion*>(
                    static_cast<const void*>(cell));
            return static_cast<int>(aiValueOfLuck(
                currentHero->getLuck(0, 0, 1),
                info->m_fountainInfo.m_luck));
        }
        return static_cast<int>(aiValueOfLuck(
            currentHero->getLuck(0, 0, 1), 1));
    case FOUNTAIN_OF_YOUTH:
        return valueOfMoveSource(
            currentHero, 0x4000, 200, moveCost);

    case GARDEN_OF_REVELATION:
        return valueOfGarden(currentHero, cell);
    case GARRISON:
        return valueOfGarrison(currentHero, cell);
    case IDOL_OF_FORTUNE:
        return valueOfIdol(currentHero, moveCost);
    case HERO:
        return valueOfHeroEvent(currentHero, cell, point.m_x, point.m_y,
                                   point.m_z, static_cast<short>(moveCost));
    case HILL_FORT:
        return valueOfHillFort(currentHero, moveCost);
    case HUT_OF_MAGI:
        return valueOfMagusHut(currentHero->m_owner);
    case LEAN_TO:
        return valueOfLeanTo(cell, player);
    case LIBRARY:
        return valueOfLibrary(currentHero, cell);
    case LIGHTHOUSE:
        return valueOfLighthouse(cell);
    case MAGIC_SCHOOL:
        // Both Dreamcast and Complete retain this named helper call. Keep
        // the source-real boundary even when VC6's current budget wants to
        // expand it.
#pragma inline_depth(0)
        return valueOfMagicSchool(currentHero, cell);
#pragma inline_depth()
    case MAGIC_SPRING:
        // Both targets retain this source-real helper boundary.
#pragma inline_depth(0)
        return getValueOfSpring(
            currentHero, cell,
            static_cast<unsigned short>(moveCost));
#pragma inline_depth()
    case MAGIC_WELL:
        // Complete retains this helper call as well; keep the decision
        // independent of the surrounding event arm's changing inline budget.
#pragma inline_depth(0)
        return getValueOfWell(
            currentHero, static_cast<unsigned short>(moveCost));
#pragma inline_depth()
    case MERC_CAMP:
        return valueOfMercenaryCamp(currentHero, cell);
    case MERMAID:
        if (currentHero->m_flags & 0x8000)
            return 0;
        if (moveCost > currentHero->m_movePoints)
            return 0;
#pragma inline_depth(0)
        return
            const_cast<hero*>(currentHero)->luckIncreaseValue(1);
#pragma inline_depth()
    case MINE:
        return valueOfMine(currentHero, cell);
    case MONSTER:
#pragma inline_depth(0)
        return valueOfMonsters(currentHero, cell, point);
#pragma inline_depth()

    case MYSTICAL_GARDEN: {
        const ExtraInfoUnion* info =
            static_cast<const ExtraInfoUnion*>(
                static_cast<const void*>(cell));
        if (!info->gardenIsFull())
            return 0;
        return static_cast<long>(
            (player->m_ai.m_resourceValue[GOLD] * 500.0
             + player->m_ai.m_resourceValue[GEMS] * 5.0)
            / 2.0);
    }

    case OASIS:
#pragma inline_depth(0)
        return valueOfMoveSource(
            currentHero, 0x80, 400, moveCost);
#pragma inline_depth()
    case OBELISK:
#pragma inline_depth(0)
        return valueOfObelisk(cell, currentHero->m_owner);
#pragma inline_depth()
    case OBSERVATORY:
        return
            aiValueOfObservatory(point, currentHero->m_owner, 20);
    case POWER_SCHOOL:
#pragma inline_depth(0)
        return valueOfPowerSchool(currentHero, cell);
#pragma inline_depth()
    case PRISON:
#pragma inline_depth(0)
        return valueOfPrison(cell, player);
#pragma inline_depth()
    case PYRAMID:
        return valueOfPyramid(currentHero, cell);
    case RALLY_FLAG:
        if (moveCost > currentHero->m_movePoints)
            return 0;
#pragma inline_depth(0)
        return valueOfRallyFlag(currentHero, moveCost);
#pragma inline_depth()
    case REFUGEE_CAMP:
#pragma inline_depth(0)
        return valueOfRefugeeCamp(currentHero, cell);
#pragma inline_depth()
    case RESOURCE:
#pragma inline_depth(0)
        return valueOfResource(currentHero, cell, player);
#pragma inline_depth()
    case SCHOLAR:
        return static_cast<int>(
            hero::getExperienceIncrement(currentHero->m_level)
            * currentHero->m_turnExperienceToRvRatio);
    case SEA_CHEST:
#pragma inline_depth(0)
        return valueOfSeaChest(currentHero, cell);
#pragma inline_depth()
    case SEER:
        return g_game->m_worldMap.m_seerHutList[cell->m_extraInfo].getValue(
            const_cast<hero*>(currentHero));
    case SEPULCHER:
    case SHIPWRECK:
#pragma inline_depth(0)
        return valueOfBank(currentHero, cell);
#pragma inline_depth()
    case SHIPYARD: {
        const ShipyardInfo* info = static_cast<const ShipyardInfo*>(
            static_cast<const void*>(&cell->m_extraInfo));
        // The Dreamcast statement is a named OnSameTeam call and Complete
        // retains that boundary.  Pin only the call; the surrounding event
        // arm remains ordinary source.
#pragma inline_depth(0)
        if (g_game->onSameTeam(
                info->m_owner, g_netLocalGamePos))
            return 0;
#pragma inline_depth()
        return 1000;
    }
    case SHRINE1:
    case SHRINE2:
    case SHRINE3:
        return valueOfShrine(currentHero, cell);
    case SIREN:
#pragma inline_depth(0)
        return valueOfSirens(currentHero);
#pragma inline_depth()
    case SPELL_SCROLL:
        return valueOfScroll(currentHero, cell);
    case STABLES:
#pragma inline_depth(0)
        return valueOfStables(currentHero, moveCost);
#pragma inline_depth()
    case TEMPLE:
        if (currentHero->m_flags & 0x100)
            return 0;
        if (currentHero->m_flags & 0x4000000)
            return 0;
        if (moveCost > currentHero->m_movePoints)
            return 0;
#pragma inline_depth(0)
        return
            const_cast<hero*>(currentHero)->moraleIncreaseValue(2);
#pragma inline_depth()
    case TOWN:
        return valueOfTown(
            currentHero, point.m_x, point.m_y, point.m_z,
            static_cast<short>(moveCost));
    case TRAINING_GROUNDS: {
        const ExtraInfoUnion* info = static_cast<const ExtraInfoUnion*>(
            static_cast<const void*>(cell));
        if (currentHero->m_trainingGroundsFlags
            & (1UL << info->getItemId()))
            return 0;
        return static_cast<int>(
            currentHero->m_turnExperienceToRvRatio * 1000.0f);
    }
    case TREASURE_CHEST:
        return valueOfTreasure(currentHero);
    case TREE_OF_KNOWLEDGE:
        return valueOfTree(currentHero, cell);
    case UNIVERSITY: {
        ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
            static_cast<void*>(cell));
#pragma inline_depth(0)
        return valueOfUniversity(
            currentHero, info->getUniversity(), 1);
#pragma inline_depth()
    }
    case WAGON:
#pragma inline_depth(0)
        return valueOfWagon(cell, currentHero->m_owner);
#pragma inline_depth()
    case WAR_MACHINE_FACTORY:
#pragma inline_depth(0)
        return valueOfWarFactory(currentHero, moveCost);
#pragma inline_depth()
    case WAR_SCHOOL:
#pragma inline_depth(0)
        return valueOfWarSchool(currentHero, cell);
#pragma inline_depth()
    case WARRIOR_TOMB: {
        const ExtraInfoUnion* info =
            static_cast<const ExtraInfoUnion*>(
                static_cast<const void*>(cell));
#pragma inline_depth(0)
        if (info->playerKnowsCell(g_netLocalGamePos))
            return 0;
#pragma inline_depth()
    }
    case SHIPWRECK_SURVIVOR:
        // Complete's retail jump table maps object type 86 directly to the
        // backpack-test / average-artifact tail which WARRIOR_TOMB reaches
        // by fallthrough. Dreamcast independently proves this named helper.
        if (const_cast<hero*>(currentHero)
                ->getNumberInBackpack(1) >= HERO_BACKPACK_CAPACITY)
            return 0;
        return static_cast<int>(g_currentPlayer->m_ai.m_turnValueOfAvgArtifact);
    case WATER_WHEEL: {
        const ExtraInfoUnion* info =
            static_cast<const ExtraInfoUnion*>(
                static_cast<const void*>(cell));
#pragma inline_depth(0)
        if (info->playerKnowsCell(g_netLocalGamePos)) {
#pragma inline_depth()
            return static_cast<long>(
                info->getWheelGold() * player->m_ai.m_resourceValue[GOLD]);
        }
        return static_cast<long>(
            player->m_ai.m_resourceValue[GOLD] * 1000.0);
    }
    case WATERING_HOLE:
#pragma inline_depth(0)
        return valueOfMoveSource(
            currentHero, 0x40, 200, moveCost);
#pragma inline_depth()
    case WINDMILL: {
        const ExtraInfoUnion* info =
            static_cast<const ExtraInfoUnion*>(
                static_cast<const void*>(cell));
#pragma inline_depth(0)
        if (info->playerKnowsCell(g_netLocalGamePos)) {
#pragma inline_depth()
            if (info->getWindmillAmount() == 0)
                return 0;
        }
        return g_currentPlayer->m_ai.m_averageResourceValue * 9 / 2;
    }
    case WITCH_HUT:
#pragma inline_depth(0)
        return valueOfWitchHut(currentHero, cell);
#pragma inline_depth()
    }
    return 0;
}

VA(0x00529750, 0x78)  // dc 0x10d91c
static long getArtifactPurchaseValue(
    TArtifact artifactId, long marketCount, long* funds);

VA(0x005297d0, 0x36)  // dc 0x10e028
long valueOfWarFactory(const hero* currentHero, long moveCost)
{
    return valueOfWarFactory(currentHero, ARTIFACT_FIRST_AID_TENT,
                                moveCost)
        + valueOfWarFactory(currentHero, ARTIFACT_AMMO_CART, moveCost)
        + valueOfWarFactory(currentHero, ARTIFACT_BALLISTA, moveCost);
}

VA(0x00529810, 0x7c)  // dc 0x110174
int netValueOfArtifact(const hero* currentHero, int artifactValue,
    int goldCost, int resourceCost, EGameResource resourceType)
{
    playerData* player = &g_game->m_players[currentHero->m_owner];
    if (player->m_resources[GOLD] < goldCost
        || player->m_resources[resourceType] < resourceCost)
        return 0;
    return static_cast<int>(
        static_cast<double>(artifactValue)
        - static_cast<double>(goldCost) * player->m_ai.m_resourceValue[GOLD]
        - static_cast<double>(resourceCost)
            * player->m_ai.m_resourceValue[resourceType]);
}

VA(0x00529890, 0x40)  // dc 0x1103c4
long valueOfCustomItem(const hero* currentHero, NewmapCell* cell, long itemValue)
{
    TreasureData* treasure = g_advManager->getTreasureData(cell);
    if (treasure->m_hasCustomGuardians)
        return aiValueOfCombat(currentHero, 0, treasure->m_guardians, 0, cell)
            + itemValue;
    return itemValue;
}

VA(0x005298d0, 0x4b)  // dc 0x1105ac
long valueOfLearning(const hero* currentHero, SpellID spell)
{
    if (g_spellTraits[spell].m_level > currentHero->m_skillLevel[eSecSkillWisdom] + 2
        || currentHero->isInSpellbook(SpellID(spell))
        || !currentHero->isWieldingArtifact(
               ARTIFACT_SPELLBOOK))
        return 0;
    return aiGetSpellValue(currentHero, spell);
}

#if 0  // @carcass -- philai body-evidence claims, retail RVA order (divergent from DC link order)

// E:\gamedcs\philai.cpp:2054
// Retail claim promoted to the source-order definition above.
long valueOfBank(const hero* current_hero, NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\philai.cpp:2128
#endif  // @carcass

VA(0x00529920, 0x10d)  // dc 0x110808
long valueOfBank(const hero* currentHero, NewmapCell* cell);

VA(0x00529a30, 0x27f)  // dc 0x1109b8
int valueOfGenerator(const hero* currentHero, int x, int y, int z, NewmapCell* cell, int moveCost)
{
    generator currentGenerator;
    long value = 0;
    short generatorId = g_game->getGeneratorId(x, y, z);
    currentGenerator = g_game->m_generators[generatorId];

    if (currentGenerator.getOwner() != g_netLocalGamePos
        && g_game->onSameTeam(currentGenerator.getOwner(),
                              g_netLocalGamePos))
        return 0;
    if (currentGenerator.getOwner() != g_netLocalGamePos
        && currentGenerator.m_guards.hasCreatures()) {
        value = aiValueOfCombat(currentHero, 0,
                                   currentGenerator.m_guards, 0, cell);
    }

    if (value <= -500000000)
        return value;

    type_AI_creature_purchaser purchaser(currentHero->m_owner,
                                          &currentGenerator);
    unsigned char hasAngelicAlliance =
        g_game->m_players[currentHero->m_owner].hasGivenArtifact(
            ARTIFACT_ANGELIC_ALLIANCE);
    long purchaseValue = purchaser.getPurchaseValue(
        &currentHero->m_army,
        currentHero->getMorale(0, 0, 1), 0,
        g_currentPlayer->m_resources, hasAngelicAlliance);

    if (moveCost >= 400
        && currentGenerator.getOwner() == g_netLocalGamePos
        && purchaser.getArmyIncrease()
               < currentHero->m_army.getAIValue() / 3)
        purchaseValue = 0;
    value += purchaseValue;

    if (static_cast<unsigned char>(
            g_game->m_mapHeader.m_victoryCondition.appliesToPlayer(
                g_netLocalGamePos))
        && g_game->m_mapHeader.m_victoryCondition.m_type
               == VICTORY_CONDITION_FLAG_ALL_GENERATORS
        && !g_game->onSameTeam(currentGenerator.getOwner(),
                               g_netLocalGamePos)
        && (g_game->m_f1f698 != 0
            || (currentGenerator.m_type[0] != CREATURE_AIR_ELEMENTAL
                && currentGenerator.m_type[0] != CREATURE_EARTH_ELEMENTAL
                && currentGenerator.m_type[0] != CREATURE_FIRE_ELEMENTAL
                && currentGenerator.m_type[0] != CREATURE_WATER_ELEMENTAL))
        && g_creatureTypeTraits[currentGenerator.m_type[0]].m_townType != -1) {
        value += 5000000 / g_game->m_generators.size();
    }
    return value;
}

VA(0x00529cb0, 0x2d9)  // dc 0x11105c
long valueOfEnemyTown(const hero* currentHero, const town* enemyTown, short moveCost, NewmapCell* cell)
{
    int creatureCost[NUM_RESOURCES];
    unsigned char includeGrowth;
    TCreatureType creature;
    hero* defendingHero;
    playerData* player = currentHero->getPlayer();
    defendingHero = 0;
    if (enemyTown->m_garrisonHeroId >= 0)
        defendingHero = g_game->getHero(enemyTown->m_garrisonHeroId);

    long townValue = 0;
    long combatValue = aiValueOfCombat(
        currentHero, defendingHero, enemyTown->getArmy(), enemyTown,
        cell);
    if (combatValue <= -500000000) {
        if (player->m_numTowns > 0)
            return combatValue;
        combatValue = -2500000;
    }

    townValue = static_cast<long>(
        enemyTown->getGoldIncome(0) * player->m_ai.m_resourceValue[GOLD] * 3.0);
    if (enemyTown->hasBuilding(MARKETPLACE_SILO_ID, 0)) {
        int* siloIncome = enemyTown->getSiloIncome();
        townValue += 3 * aiResourceCost(player, siloIncome);
    }

    includeGrowth = 0;
    if ((moveCost - currentHero->m_movePoints)
                / currentHero->m_maxMovePoints
            + g_game->m_day
        >= 7)
        includeGrowth = 1;
    for (int dwelling = 0; dwelling < TOWN_DWELLING_SLOTS; dwelling++) {
        long population = enemyTown->m_population[dwelling];
        if (includeGrowth)
            population += const_cast<town*>(enemyTown)->getGrowthRate(
                dwelling);

        if (population > 0) {
            creature = g_townDwellingCreatures[
                TOWN_DWELLING_SLOTS * enemyTown->m_type + dwelling];
            getMonsterCost(creature, creatureCost);
            long profit = g_creatureTypeTraits[creature].m_aiValue
                - aiResourceCost(player, creatureCost);
            if (profit > 0)
                townValue += profit * population;
        }
    }

    townValue = static_cast<long>(
        (type_AI_player::getAttackBonus(enemyTown->m_owner) + 1.0f)
        * townValue);
    if (player->m_numTowns == 0)
        townValue += 5000000;
    else
        townValue += 5000000 / g_game->m_towns.size();

    LossConditionStruct& loss = g_game->m_mapHeader.m_lossCondition;
    if (loss.m_type == LOSS_CONDITION_LOSE_TOWN
        && loss.m_townX == enemyTown->m_mapX
        && loss.m_townY == enemyTown->m_mapY
        && loss.m_townZ == enemyTown->m_mapZ)
        townValue += 5000000;

    return combatValue + townValue;
}

VA(0x00529f90, 0x72)  // dc 0x111834
int valueOfMagicSchool(const hero* currentHero, NewmapCell* cell)
{
    if ((1 << cell->m_extraInfo) & currentHero->m_magicSchoolFlags)
        return 0;
    playerData* player = currentHero->getPlayer();
    if (player->m_resources[GOLD] < 1000)
        return 0;

    return static_cast<int>(
        max(currentHero->getValueOfPower(),
                      currentHero->getValueOfKnowledge())
        - player->m_ai.m_resourceValue[GOLD] * 1000.0);
}

VA(0x0052a010, 0x12a)  // dc 0x111970
int valueOfMine(const hero* currentHero, NewmapCell* cell)
{
    mine* currentMine = &g_game->m_mines[cell->m_extraInfo];
    long value = 0;
    int mineType = currentMine->m_type;
    int sameTeam = onMySide(currentMine->m_playerOwner);
    if (sameTeam)
        return 0;

    if (currentMine->m_guards.hasCreatures()) {
        value = aiValueOfCombat(currentHero, 0, currentMine->m_guards, 0,
                                   cell);
        if (value <= -500000000)
            return value;
    }

    playerData* player = currentHero->getPlayer();
    int income = static_cast<int>(
        static_cast<double>(g_mineCharacteristics[mineType])
        * player->m_ai.m_resourceValue[mineType] * 2.0);
    value += static_cast<int>(static_cast<float>(income)
        * (type_AI_player::getAttackBonus(currentMine->m_playerOwner)
           + 1.0f));

    if (g_game->m_mapHeader.m_victoryCondition.m_type
            == VICTORY_CONDITION_FLAG_ALL_MINES)
        value += 5000000 / g_game->m_mines.size();
    return value;
}

VA(0x0052a140, 0x96)  // dc 0x111a9c
long valueOfMonsters(const hero* currentHero, NewmapCell* cell, type_point point)
{
    int typedCreature;
    typedCreature = cell->m_objectIndex;
    TCreatureType type = TCreatureType(typedCreature);
    armyGroup monsters(type,
        static_cast<unsigned short>(cell->m_extraInfo) & 0xfff);
    long value = aiValueOfCombat(currentHero, 0, monsters, 0, cell);
    if (value <= -500000000)
        return value;

    VictoryConditionStruct& victory = g_game->m_mapHeader.m_victoryCondition;
    if (victory.m_type == VICTORY_CONDITION_DEFEAT_MONSTER
        && victory.m_monsterX == point.m_x
        && victory.m_monsterY == point.m_y
        && victory.m_monsterZ == point.m_z)
        value += 5000000;
    return value;
}

VA(0x0052a1e0, 0xc3)  // dc 0x111c44
int valueOfMoveSource(const hero* currentHero, long flag,
                         short increase, long& moveCost);

VA(0x0052a2b0, 0xc5)  // dc 0x111d68
int valueOfObelisk(NewmapCell* cell, long playerId)
{
    if (g_game->m_obeliskFlags[cell->m_extraInfo] & (1 << playerId))
        return 0;
    if (!g_game->m_ultimateArtifactPresent)
        return 0;

    playerData* player = &g_game->m_players[playerId];
    if (player->m_puzzleGuess.m_x == g_game->m_ultimateArtifactX
        && player->m_puzzleGuess.m_y == g_game->m_ultimateArtifactY
        && player->m_puzzleGuess.m_z
            == static_cast<short>(g_game->m_ultimateArtifactZ))
        return 0;

    type_artifact grailArtifact(ARTIFACT_HOLY_GRAIL);
    return aiGetValueOfArtifact(grailArtifact, playerId)
        / g_game->m_numObelisks;
}

VA(0x0052a380, 0x1c)  // dc 0x111e34
int valueOfPowerSchool(const hero* currentHero, NewmapCell* cell)
{
    if ((1 << cell->m_extraInfo) & currentHero->m_powerSchoolFlags)
        return 0;
    return currentHero->getValueOfPower();
}

VA(0x0052a3a0, 0x61)  // dc 0x111ea4
int valueOfPrison(NewmapCell* cell, playerData* player)
{
    if (g_currentPlayer->m_numHeroes >= 8)
        return 0;
    hero& prisoner = g_game->m_heroes[cell->m_extraInfo];
    long armyValue = prisoner.m_army.getAIValue();
    return static_cast<int>(player->m_ai.m_resourceValue[GOLD] * 2500.0
        + armyValue);
}

const int g_pyramidSpellLevel = 5;
VA(0x0052a410, 0xF5)
long valueOfPyramid(const hero* currentHero, NewmapCell* cell)
{
    short owner = currentHero->m_owner;
    if (cell->playerKnowsCell(owner) && !cell->pyramidIsGuarded())
        return 0;

    armyGroup guardians;
    long value = 0;
    long count = 0;
    guardians.add(0x74, 0x28, -1);
    guardians.add(0x75, 0x14, -1);

    if (currentHero->m_skillLevel[eSecSkillWisdom] >= 3) {
        for (int spell = 0; spell < hero::NUM_SPELLS; spell++) {
            if (g_spellTraits[spell].m_level == g_pyramidSpellLevel) {
                if (!currentHero->spellIsAvailable(spell)) {
                    long spellValue;
                    if (currentHero->isInSpellbook(SpellID(spell)))
                        spellValue = 0;
                    else if (currentHero->isWieldingArtifact(
                                 ARTIFACT_SPELLBOOK))
                        spellValue = aiGetSpellValue(currentHero, spell);
                    else
                        spellValue = 0;
                    value += spellValue;
                }
                count++;
            }
        }
        value = value / count;
    }

    return aiValueOfCombat(currentHero, 0, guardians, 0, cell) + value;
}

VA(0x0052a510, 0xdf)  // dc 0x11206c
long getValueOfWell(const hero* currentHero, unsigned short moveCost)
{
    if (currentHero->m_flags & 1)
        return 0;

    type_point path;
    path.m_x = currentHero->m_pathTargetX;
    path.m_y = currentHero->m_pathTargetY;
    path.m_z = currentHero->m_pathTargetZ;
    type_point target = path;
    if (target.isValid() && moveCost > 300) {
        NewmapCell* cell = g_game->m_worldMap.cell(target);
        if (cell->m_type != MAGIC_WELL && cell->m_type != MAGIC_SPRING)
            return 0;
    }
    return currentHero->getValueOfWell();
}

VA(0x0052a5f0, 0x108)  // dc 0x1120e8
int valueOfRallyFlag(const hero* currentHero, long& moveCost)
{
    if (currentHero->m_flags & 0x10000)
        return 0;

    return static_cast<int>(
        valueOfMoveSource(currentHero, 0x10000, 200, moveCost)
        + aiValueOfLuck(
            currentHero->getLuck(0, 0, 1), 1)
        + aiValueOfMorale(
            currentHero->getMorale(0, 0, 1), 1));
}

long valueOfRecruiting(const hero* currentHero, TCreatureType creature,
                         short amount);

VA(0x0052a700, 0x10)  // dc 0x1121b8
int valueOfRefugeeCamp(const hero* currentHero, NewmapCell* cell)
{
    TCreatureType creature;
    {
        int ordinal = cell->m_objectIndex;
        memcpy(&creature, &ordinal, sizeof creature);
    }
    return valueOfRecruiting(currentHero, creature,
        static_cast<short>(cell->m_extraInfo));
}

#if 0  // @carcass

// E:\gamedcs\philai.cpp:2208
// Retail claim promoted to the reconstructed body below.
long valueOfRecruiting(const hero* current_hero, TCreatureType creature, short amount)
{
    // @stub
}

#endif  // @carcass

VA(0x0052a710, 0xad)  // dc 0x110c90
long valueOfRecruiting(const hero* currentHero, TCreatureType creature,
                         short amount)
{
    type_AI_creature_purchaser purchaser(
        currentHero->m_owner, creature, &amount, 0);
    unsigned char hasAngelicAlliance =
        g_game->m_players[currentHero->m_owner].hasGivenArtifact(
            ARTIFACT_ANGELIC_ALLIANCE);
    return purchaser.getPurchaseValue(
        &currentHero->m_army,
        currentHero->getMorale(0, 0, 1), 0,
        g_currentPlayer->m_resources, hasAngelicAlliance);
}

VA(0x0052a7c0, 0xa8)  // dc 0x112260
long valueOfResource(const hero* currentHero, NewmapCell* cell, playerData* player)
{
    long combatValue = 0;
    long amount;
    int resourceType = cell->m_objectIndex;
    if (!cell->isCustomized()) {
        amount = cell->m_extraInfo;
    } else {
        TreasureData* treasure = g_advManager->getTreasureData(cell);
        amount = cell->m_extraInfo & 0x7ffff;
        if (treasure->m_hasCustomGuardians && treasure->m_guardians.getNumArmies())
            combatValue = aiValueOfCombat(currentHero, 0,
                treasure->m_guardians, 0, cell);
    }
    if (resourceType == GOLD)
        amount *= 100;
    return static_cast<long>(static_cast<double>(amount)
            * player->m_ai.m_resourceValue[resourceType]
        + static_cast<double>(combatValue));
}

VA(0x0052a870, 0x4F)  // dc 0x1123ac
int valueOfSeaChest(const hero* currentHero, NewmapCell* cell)
{
    playerData* player = currentHero->getPlayer();
    if (const_cast<hero*>(currentHero)->getNumberInBackpack(1)
            < HERO_BACKPACK_CAPACITY)
        return static_cast<int>(player->m_ai.m_turnValueOfAvgArtifact / 10.0f
            + player->m_ai.m_resourceValue[GOLD] * 1200.0);
    return static_cast<int>(player->m_ai.m_resourceValue[GOLD] * 1200.0);
}

VA(0x0052a8c0, 0x9a)  // dc 0x112510
int valueOfScroll(const hero* currentHero, NewmapCell* cell)
{
    SpellID spell;
    if (const_cast<hero*>(currentHero)->getNumberInBackpack(1)
            >= HERO_BACKPACK_CAPACITY)
        return 0;

    int value = 10;
    if (cell->isCustomized()) {
        TreasureData* treasure = g_advManager->getTreasureData(cell);
        spell = cell->m_extraInfo & 0xff;
        if (treasure->m_hasCustomGuardians
                && treasure->m_guardians.getNumArmies() != 0)
            value += aiValueOfCombat(currentHero, 0,
                treasure->m_guardians, 0, cell);
    } else
        spell = cell->m_extraInfo;

    type_artifact artifact(spell);
    if (!currentHero->spellIsAvailable(spell))
        value += aiGetValueOfArtifact(artifact, currentHero, false, false);
    return value;
}

VA(0x0052a960, 0x158)  // dc 0x1126d0
int valueOfSirens(const hero* currentHero)
{
    armyGroup army = currentHero->m_army;
    int oldValue = army.getAIValue();
    int experience = aiVisitSirens(currentHero, army);
    int value = oldValue - army.getAIValue();
    value = value
        * (const_cast<hero*>(currentHero)->getPrimarySkillTotal() + 40)
        / 40;
    if (currentHero->m_turnExperienceToRvRatio == 0.0f)
        return -value;
    return static_cast<int>(valueOfExperience(currentHero, army)
        * experience - value);
}

VA(0x0052aac0, 0xB9)  // dc 0x11276c
int valueOfStables(const hero* currentHero, long& moveCost)
{
    int value = 0;
    if (!(currentHero->m_flags & 2)) {
        short movementValue = static_cast<short>(
            (8 - g_game->m_day) * g_stablesMovementBonus / 2);
        if (moveCost >= movementValue) {
            moveCost -= movementValue;
            value = 50;
        } else {
            moveCost = 0;
            value = 10000;
        }
    }

    int number = const_cast<hero*>(currentHero)->creatureTypeCount(10);
    if (number == 0)
        return value;

    int upgradeValue = (g_creatureTypeTraits[11].m_aiValue
            - g_creatureTypeTraits[10].m_aiValue) * number;
    if (const_cast<hero*>(currentHero)->creatureTypeCount(11) != 0)
        upgradeValue = static_cast<int>(upgradeValue * 0.5);
    return upgradeValue + value;
}

// E:\gamedcs\philai.cpp's CodeView global record names this exact static
// five-piece row. Retail .rdata 0x640558 is the consecutive 118..122 run,
// and value_of_town walks it by pointer from begin to one-past-end.
DATA(0x00640558) static const TArtifact g_legionArtifacts[5] = {
    ARTIFACT_LEGS_OF_LEGION, ARTIFACT_LOINS_OF_LEGION,
    ARTIFACT_TORSO_OF_LEGION, ARTIFACT_ARMS_OF_LEGION,
    ARTIFACT_HEAD_OF_LEGION
};

VA(0x0052ab80, 0x505)  // dc 0x112914
long valueOfTown(const hero* currentHero, int x, int y, int z, short moveCost)
{
    short townId = static_cast<short>(g_game->getTownId(x, y, z));
    type_point point(static_cast<short>(x), static_cast<short>(y),
                     static_cast<short>(z));
    NewmapCell* cell = g_game->getCell(point);
    if (townId < 0)
        return 0;

    town* currentTown = g_game->getTown(townId);
    if (currentTown->m_owner != currentHero->m_owner) {
        if (onMySide(currentTown->m_owner))
            return valueOfTownBuildings(currentHero, currentTown);

        if (currentTown->m_visitingHeroId >= 0) {
            hero* visitingHero =
                g_game->getHero(currentTown->m_visitingHeroId);
            aiValueOfCombat(currentHero, visitingHero,
                               visitingHero->m_army, 0, cell);
        }
        return valueOfEnemyTown(currentHero, currentTown, moveCost,
                                   cell);
    }

    long value = valueOfReinforcing(
        const_cast<hero*>(currentHero), currentTown, moveCost);
    value += valueOfTownBuildings(currentHero, currentTown);

    VictoryConditionStruct& victory = g_game->m_mapHeader.m_victoryCondition;
    if (currentHero->hasArtifact(ARTIFACT_HOLY_GRAIL)
        && currentTown->isLegalBuilding(HOLY_GRAIL_ID)
        && !currentTown->hasBuilding(HOLY_GRAIL_ID, 0)) {
        if (victory.m_type == VICTORY_CONDITION_BUILD_GRAIL) {
            if (victory.m_townX == currentTown->m_mapX
                && victory.m_townY == currentTown->m_mapY
                && victory.m_townZ == currentTown->m_mapZ) {
                value += 5000000;
            }
        } else {
            type_artifact grail(ARTIFACT_HOLY_GRAIL);
            value += aiGetValueOfArtifact(grail,
                                                   currentHero->m_owner);
        }
    }

    if (victory.m_type == VICTORY_CONDITION_TRANSPORT_ARTIFACT
        && static_cast<unsigned char>(
               victory.appliesToPlayer(g_netLocalGamePos))
        && victory.m_townX == currentTown->m_mapX
        && victory.m_townY == currentTown->m_mapY
        && victory.m_townZ == currentTown->m_mapZ
        && currentHero->hasArtifact(
               victory.m_artifactNum)) {
        value += 5000000;
    }

    if (currentTown->m_threateningHeroes > 0) {
        if (victory.m_type == VICTORY_CONDITION_CAPTURE_TOWN
            && victory.m_townX == currentTown->m_mapX
            && victory.m_townY == currentTown->m_mapY
            && victory.m_townZ == currentTown->m_mapZ) {
            value += 5000000;
        } else {
            value += 5000000 / g_game->m_towns.size();
        }
    }

    hero* garrisonHero = 0;
    if (currentTown->m_garrisonHeroId >= 0)
        garrisonHero = g_game->getHero(currentTown->m_garrisonHeroId);

    if (garrisonHero) {
        for (int i = 0; i < 5; ++i) {
            if (currentHero->hasArtifact(
                    g_legionArtifacts[i])) {
                type_artifact artifact(g_legionArtifacts[i]);
                value += aiGetEquipValue(artifact, garrisonHero, 1);
            }
        }
    }

    if (g_currentPlayer->m_numHeroes > 1 && g_game->m_setup.m_difficulty > 0
        && (!garrisonHero
            || garrisonHero->getPrimarySkillTotal() * 5 / 4
                   < const_cast<hero*>(currentHero)
                         ->getPrimarySkillTotal())
        && shouldGarrisonTown(currentHero, currentTown)) {
        if (victory.m_type == VICTORY_CONDITION_CAPTURE_TOWN
            && victory.m_townX == currentTown->m_mapX
            && victory.m_townY == currentTown->m_mapY
            && victory.m_townZ == currentTown->m_mapZ) {
            value += 5000000;
        } else {
            value += 5000000 / g_game->m_towns.size();
        }
    }

    return cppMin(value, 5000000L);
}

VA(0x0052b090, 0x14e)  // dc 0x112830
long valueOfReinforcing(hero* currentHero, town* currentTown, short moveCost)
{
    long playerId = currentHero->m_owner;
    playerData* player = &g_game->m_players[playerId];
    type_AI_creature_purchaser purchaser(playerId, currentTown);

    hero* garrisonHero = 0;
    if (currentTown->m_garrisonHeroId >= 0)
        garrisonHero = g_game->getHero(currentTown->m_garrisonHeroId);

    unsigned char hasAngelicAlliance =
        g_game->m_players[currentHero->m_owner].hasGivenArtifact(
            ARTIFACT_ANGELIC_ALLIANCE);
    long swapValue = purchaser.getSwapValue(
        currentHero,
        &static_cast<const town*>(currentTown)->getArmy(), garrisonHero,
        hasAngelicAlliance);
    long purchaseValue = purchaser.getPurchaseValue(
        &currentHero->m_army, currentHero->getMorale(0, 0, 1),
        &static_cast<const town*>(currentTown)->getArmy(), player->m_resources,
        hasAngelicAlliance);

    if (moveCost >= 400
        && purchaser.getArmyIncrease()
               < currentHero->m_army.getAIValue() / 3)
        return 0;

    return purchaseValue + swapValue / 2;
}

// Complete retains this separate building appraisal and value_of_town
// calls it at 0x52ac9c (allied town) and 0x52ad0c (owned town). DC's
// value_of_town owns the special-building arms itself at source 3235..3263
// (HasBuilding calls dc 0x112cb6/0x112cd4/0x112cf0/0x112d6e/0x112d96).
// The new helper also prices the Conflux university and unlearned guild
// spells, then checks the per-hero town bonus mask before the stat arms.
// Its role name is provisional; keep the retained body in this module.
VA(0x0052b1e0, 0x2f4)  // anchor-callee {type_university ctor, value_of_university, AI_get_spell_value, bitset _Xran}, 2 sites in value_of_town, retail-only
long valueOfTownBuildings(const hero* currentHero, town* currentTown)
{
    long value = 0;
    if (currentTown->m_type == TOWN_CONFLUX
        && currentTown->hasBuilding(EXTRA_0_ID, 1)) {
        type_university university;
        university.initializeMagicSkills();
#pragma inline_depth(0)
        value = valueOfUniversity(currentHero, &university, 1);
#pragma inline_depth()
    }

    if (currentTown->hasBuilding(MAGE_GUILD_ID, 1)) {
        if (!currentHero->isWieldingArtifact(
                ARTIFACT_SPELLBOOK)) {
            if (g_currentPlayer->m_resources[GOLD] >= 500)
                value += 1000;
        } else {
            for (int level = 0;
                 level < currentHero->m_skillLevel[eSecSkillWisdom] + 2; level++) {
                if (level > currentTown->m_mageLevel)
                    break;
                for (int slot = 0;
                     slot < currentTown->m_mageGuildSpellCounts[level];
                     slot++) {
                    int spell = currentTown->m_mageGuildSpells[level][slot];
                    if (!currentHero->isInSpellbook(SpellID(spell)))
                        value += aiGetSpellValue(currentHero, spell);
                }
            }
        }
    }

    if (const_cast<hero*>(currentHero)->m_townSpecialGrantedMask.test(
            currentTown->m_id))
        return value;

    switch (currentTown->m_type) {
    case TOWN_TOWER:
        if (currentTown->hasBuilding(EXTRA_2_ID, 1))
            value += currentHero->getValueOfKnowledge();
        break;
    case TOWN_INFERNO:
        if (currentTown->hasBuilding(EXTRA_2_ID, 1))
            value += currentHero->getValueOfPower();
        break;
    case TOWN_DUNGEON:
        if (currentTown->hasBuilding(EXTRA_2_ID, 1))
            value = static_cast<long>(
                currentHero->m_turnExperienceToRvRatio * 1000.0f
                + static_cast<float>(value));
        break;
    case TOWN_STRONGHOLD:
        if (currentTown->hasBuilding(EXTRA_2_ID, 1))
            value = static_cast<long>(
                static_cast<float>(hero::getExperienceIncrement(
                    currentHero->m_level))
                * currentHero->m_turnExperienceToRvRatio
                + static_cast<float>(value));
        break;
    case TOWN_FORTRESS:
        if (currentTown->hasBuilding(SPECIAL_BUILDING_ID, 1))
            value = static_cast<long>(
                static_cast<float>(hero::getExperienceIncrement(
                    currentHero->m_level))
                * currentHero->m_turnExperienceToRvRatio
                + static_cast<float>(value));
        break;
    }
    return value;
}

VA(0x0052b4e0, 0xbf)  // dc 0x112f6c
int valueOfTreasure(const hero* currentHero)
{
    playerData* player = currentHero->getPlayer();
    int experiencePart = static_cast<int>(
        currentHero->m_turnExperienceToRvRatio * 160.0f);
    int goldPart =
        static_cast<int>(player->m_ai.m_resourceValue[GOLD] * 320.0);
    int value;
    if (experiencePart > goldPart)
        value = experiencePart;
    else
        value = goldPart;
    experiencePart = static_cast<int>(
        currentHero->m_turnExperienceToRvRatio * 320.0f);
    goldPart = static_cast<int>(player->m_ai.m_resourceValue[GOLD] * 480.0);
    if (experiencePart > goldPart)
        value += experiencePart;
    else
        value += goldPart;

    experiencePart = static_cast<int>(
        currentHero->m_turnExperienceToRvRatio * 465.0f);
    goldPart = static_cast<int>(player->m_ai.m_resourceValue[GOLD] * 620.0);
    if (experiencePart > goldPart)
        value += experiencePart;
    else
        value += goldPart;
    return static_cast<int>(player->m_ai.m_turnValueOfAvgArtifact / 20.0f
        + static_cast<float>(value));
}

// E:\gamedcs\philai.cpp:3339.  A Tree of Knowledge: one level's worth of
// experience, once per hero per tree, less the tree's price - free, 2,000
// gold or 10 gems when this player has already learned which, or the
// three-way expected price (capped at two thirds of the level's worth)
// when he has not.  Extern for emission while the TREE arm is a stub.
// type_tree_info::price's domain, named the way PYRAMID_SPELL_LEVEL is:
// TU-local consts rather than a header enum - three enumerators in
// advmgr.h cross the include-set wall (recruitUnit::Update 90.84 ->
// 88.24, measured 2026-08-27, the bitmap16 narrowing precedent), and
// philai.h is itself in game.h's closure.
const int g_treePriceGold = 1;
const int g_treePriceGems = 2;
VA(0x0052b5a0, 0x161)  // dc 0x1130cc
int valueOfTree(const hero* currentHero, NewmapCell* cell)
{
    ExtraInfoUnion* info =
        static_cast<ExtraInfoUnion*>(static_cast<void*>(cell));
    if (currentHero->m_treeOfKnowledgeFlags
            & (1 << (static_cast<unsigned char>(cell->m_extraInfo) & 0x1f)))
        return 0;

    int increment = hero::getExperienceIncrement(currentHero->m_level);
    playerData* player = currentHero->getPlayer();
    int levelValue = static_cast<int>(static_cast<float>(increment)
        * currentHero->m_turnExperienceToRvRatio);

    if (cell->playerKnowsCell(currentHero->m_owner)) {
        switch (info->m_treeInfo.m_price) {
        case g_treePriceGold:
            if (g_currentPlayer->m_resources[GOLD] < 2000)
                return 0;
            return static_cast<int>(levelValue
                - player->m_ai.m_resourceValue[GOLD] * 2000.0);
        case g_treePriceGems:
            if (g_currentPlayer->m_resources[GEMS] < 10)
                return 0;
            return static_cast<int>(levelValue
                - player->m_ai.m_resourceValue[GEMS] * 10.0);
        default:
            return levelValue;
        }
    }

    int expectedPrice = static_cast<int>(
        (player->m_ai.m_resourceValue[GOLD] * 2000.0
         + player->m_ai.m_resourceValue[GEMS] * 10.0) / 3.0);
    int priceCap = levelValue * 2 / 3;
    if (expectedPrice > priceCap)
        expectedPrice = priceCap;
    return levelValue - expectedPrice;
}

VA(0x0052b710, 0x7e)  // dc 0x113324
int valueOfWagon(NewmapCell* cell, long playerId)
{
    if (cell->playerKnowsCell(static_cast<short>(playerId)))
        return 0;
    int resourcePart =
        g_game->m_players[playerId].m_ai.m_averageResourceValue * 7 / 4;
    return static_cast<int>(
        g_currentPlayer->m_ai.m_turnValueOfAvgArtifact * 2.0f / 5.0f
        + static_cast<float>(resourcePart));
}

VA(0x0052b790, 0x71)  // dc 0x113380
int valueOfWarSchool(const hero* currentHero, NewmapCell* cell)
{
    if ((1 << cell->m_extraInfo) & currentHero->m_warSchoolFlags)
        return 0;
    playerData* player = currentHero->getPlayer();
    if (player->m_resources[GOLD] < 1000)
        return 0;
    return static_cast<int>(
        static_cast<float>(
            hero::getExperienceIncrement(currentHero->m_level))
        * currentHero->m_turnExperienceToRvRatio
        - player->m_ai.m_resourceValue[GOLD] * 1000.0);
}

VA(0x0052b810, 0xe1)  // dc 0x11348c
long getValueOfSpring(const hero* currentHero, const NewmapCell* cell,
                         unsigned short moveCost)
{
    const ExtraInfoUnion* info = static_cast<const ExtraInfoUnion*>(
        static_cast<const void*>(cell));
    if (!info->magicSpringIsFull())
        return 0;

    type_point path;
    path.m_x = currentHero->m_pathTargetX;
    path.m_y = currentHero->m_pathTargetY;
    path.m_z = currentHero->m_pathTargetZ;
    type_point target = path;
    if (target.isValid() && moveCost > 300) {
        NewmapCell* destination = g_game->m_worldMap.cell(target);
        if (destination->m_type != MAGIC_WELL
            && destination->m_type != MAGIC_SPRING)
            return 0;
    }
    return currentHero->getValueOfSpring();
}

VA(0x0052b900, 0xb6)  // dc 0x113da0
int valueOfWitchHut(const hero* currentHero, NewmapCell* cell)
{
    ExtraInfoUnion* info =
        static_cast<ExtraInfoUnion*>(static_cast<void*>(cell));
    if (cell->playerKnowsCell(currentHero->m_owner)) {
        if (currentHero->m_skillCount >= 8)
            return 0;
        int skill = info->m_witchHutInfo.m_skill;
        if (skill == -1)
            return 0;
        if (currentHero->m_skillLevel[skill])
            return 0;
        if (!wantsSkill(currentHero, TSecondarySkill(skill), 1))
            return 0;
        return getSkillValue(currentHero, TSecondarySkill(skill), 1);
    }
    return static_cast<int>(
        static_cast<float>(
            hero::getExperienceIncrement(currentHero->m_level))
        * currentHero->m_turnExperienceToRvRatio);
}

VA(0x0052b9c0, 0x20c)  // dc 0x1147ac
void __cdecl aiExamineMap()
{
    type_point point;
    point.m_z = 0;
    long passableCells = 0;
    long waterCells = 0;
    long extraMovement[3] = {0, 0, 0};

    for (; point.m_z < g_game->m_worldMap.getNumLevels(); point.m_z++) {
        for (point.m_x = 0; point.m_x < g_mapWidth; point.m_x++) {
            for (point.m_y = 0; point.m_y < g_mapHeight; point.m_y++) {
                NewmapCell* cell = g_game->m_worldMap.cell(point.m_x, point.m_y, point.m_z);
                if ((cell->m_flags0011 & 0x40)
                        && cell->m_groundSet != eTerrainRock) {
                    passableCells++;
                    if (cell->m_groundSet == eTerrainWater) {
                        waterCells++;
                    } else {
                        for (int skill = 0; skill < 3; skill++) {
                            if (calcTerrainCost(cell, 0, 9999, skill, 0,
                                    -1, -1, -1, 0) <= 100)
                                break;
                            extraMovement[skill]++;
                        }
                    }
                }
            }
        }
    }

    g_aiWaterMapFraction = static_cast<double>(waterCells)
        / passableCells;
    for (int skill = 0; skill < 3; skill++)
        g_aiPathfindingMapFactor[skill]
            = static_cast<double>(extraMovement[skill]) / passableCells;
}

VA(0x0052bbd0, 0x87)  // dc 0x114a5c
TSecondarySkill aiChooseSecondarySkill(const hero* ourHero,
    TSecondarySkill first, TSecondarySkill second,
    unsigned char complexChoice)
{
    if ((ourHero->m_skillLevel[first] > 0)
            == (ourHero->m_skillLevel[second] > 0)) {
        if (getSkillValue(ourHero, first, complexChoice)
                >= getSkillValue(ourHero, second, complexChoice))
            return first;
        return second;
    }

    if (ourHero->m_skillLevel[second] == 0) {
        TSecondarySkill known = first;
        first = second;
        second = known;
    }
    if (wantsSkill(ourHero, first, complexChoice))
        return first;
    return second;
}

VA(0x0052bc60, 0xAB)  // dc 0x114adc
void aiJoinDecision(hero* currentHero, TCreatureType creature,
                      short amount)
{
    type_AI_creature_purchaser purchaser(currentHero->m_owner, creature,
                                         &amount, 1);
    unsigned char hasAngelicAlliance =
        g_game->m_players[currentHero->m_owner].hasGivenArtifact(
            ARTIFACT_ANGELIC_ALLIANCE);
    purchaser.doPurchase(&currentHero->m_army,
                          currentHero->getMorale(0, 0, 0), 0,
                          g_currentPlayer->m_resources, 0,
                          hasAngelicAlliance);
}

VA(0x0052bd10, 0x1D)  // dc 0x114b44
long aiValueOfEvent(const hero* currentHero, type_point point)
{
    long moveCost = 0;
    return aiValueOfEvent(currentHero, point, moveCost);
}

#if 0  // @carcass

// E:\gamedcs\philai.cpp:4204
DC_ONLY(0x114adc, 0x68)
void aiJoinDecision(hero* current_hero, TCreatureType creature, short amount)
{
    // @stub
}

// E:\gamedcs\philai.cpp:4217
DC_ONLY(0x114b44, 0x44)
long aiValueOfEvent(const hero* current_hero, type_point point)
{
    // @stub
}

#endif  // @carcass

// COMDAT pairing: std::_Unguarded_insert<type_creature_value, greater>. See
// ai_player's unpredicated twin: `ret 0x10` against its `ret 0xc` is the
// empty predicate's four stack bytes, and nothing else separates the two.
VA_COMPGEN(0x0052bd50, 0x42, STD_UNGUARDED_INSERT, type_creature_value_greater)
