// ai_player.cpp - E:\gamedcs\ai_player.cpp (compiland ai_player.obj)

// end_turn's marketplace gate is a town::HasBuilding call in the
// Dreamcast body (dc 0x2e7d8 line 452, `mov #14,r5 / mov #1,r6`); see
// town.h for why the inline's visibility is scoped.
// find_all_destinations' grail-spot tail expands the canonical game::getCell
// wrapper and naturally retains its nested NewfullMap::cell call.
#include <va.h>
#include "creaturetype.h"
#include <algorithm>
#include <functional>
#include <math.h>
#include "ai_player.h"
#include "advmgr.h"
#include "ai_combat.h"
#include "ai_spellvalue.h"
#include "armygrp.h"
#include "mousemgr.h"
#include "findpath.h"
#include "exec.h"
#include "game.h"
#include "hero.h"
#include "kb.h"
#include "misc.h"
#include "netgame.h"
#include "remote.h"
#include "soundmgr.h"
#include "town.h"
#include "tradpost.h"
#include "recruit.h"
#include "includes.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// events.h publishes the same seventh-day member for its adventure-event
// readers. Keep this TU's narrow include set while naming the calendar value
// used by Rampart's treasury appraisal below.
const unsigned short g_aiDayOfWeekSunday = 7;

int aiResourceCost(long playerId, const int* resources);
int aiResourceCost(const PlayerData* player, const int* resources);
long aiGetSpellValue(const Hero* ourHero, SpellID spell);
bool considerHiring(long playerId, Hero* candidate);
const std::bitset<9>& armyGrpFn0044A460();
int canBuy(const Town* currTown, int buildingId);
double getTradeRatio(GameResource source, GameResource dest,
                       double efficiency);
const unsigned int g_ctaShooter = 0x4;

// Dreamcast names the 144 vector rows and Complete's initializer passes this
// address, count and 16-byte stride to the vector-constructor iterator.
DATA(0x00692e18)
std::vector<ArtifactEffect*> g_constArtifactEffects[144];
// Retail and Dreamcast both make this an 8-byte strategy object: a
// three-slot vptr followed by the current player id. start_turn inlines
// both constructors and calls check_towns on one base and one derived
// instance. The virtual roster/order comes from the three retail vtable
// entries and the corresponding DC public names.
// Before normalization (type): type_town_threat_checker.
class TownThreatChecker {
protected:
    void markTowns(Hero* enemyHero, SearchArray* currentSearchArray);

public:
    int m_currentPlayerId;

    // E:\gamedcs\ai_player.cpp:89, dc 0x2dd40
    TownThreatChecker(int newPlayer) { m_currentPlayerId = newPlayer; }
    void checkTowns();
    virtual void clearMarks() const;
    virtual unsigned char isMarked(const Town* ourTown) const;
    virtual void markTown(Town* ourTown) const;
};

VA(0x004280e0, 0x171)  // dc 0x2dd64
void TownThreatChecker::checkTowns()
{
    clearMarks();

    for (int playerId = 0; playerId < 8; ++playerId) {
        const PlayerData& player = g_game->m_players[playerId];
        if (!g_game->onSameTeam(playerId, m_currentPlayerId)
            && !g_game->m_playerDisabled[playerId]) {
            for (int heroIndex = 0; heroIndex < player.m_numHeroes;
                 ++heroIndex) {
                Hero* enemyHero = g_game->getHero(player.m_heroes[heroIndex]);
                long mobility = enemyHero->getMobility() + 800;
                type_point start(enemyHero->m_x, enemyHero->m_y, enemyHero->m_z);
                type_point target(-1, -1, -1);
                enemyHero->m_bounty = 0;
                g_searchArray->seedPosition(
                    enemyHero, start, target, mobility,
                    (enemyHero->m_flags >> 18) & 1,
                    const_AI_enemy_search, mobility, 0);
                markTowns(enemyHero, g_searchArray);
            }
        }
    }
}

VA(0x00428260, 0x4E)  // dc 0x2de68
void TownThreatChecker::clearMarks() const
{
    for (unsigned int i = 0; i < g_game->m_towns.size(); ++i)
        g_game->m_towns[i].m_threateningHeroes = 0;
}

VA(0x004282b0, 0x157)  // dc 0x2deac
void TownThreatChecker::markTowns(Hero* enemyHero,
                                          SearchArray* currentSearchArray)
{
    PlayerData& player = g_game->m_players[m_currentPlayerId];

    for (int townIndex = 0; townIndex < player.m_numTowns; ++townIndex) {
        Town* ourTown = g_game->getTown(player.m_townIds[townIndex]);
        if (!isMarked(ourTown)) {
            type_point location(ourTown->m_mapX, ourTown->m_mapY,
                                ourTown->m_mapZ);
            if (currentSearchArray->getCell(location, 0)->m_visited
                && canTakeTown(enemyHero, ourTown)) {
                enemyHero->m_bounty = 5000000 / player.m_numTowns;
                markTown(ourTown);
            }
        }
    }
}

VA(0x00428410, 0x160)  // dc 0x2dc00
unsigned char canTakeTown(const Hero* attackingHero, const Town* defendingTown)
{
    ArmyGroup attackingArmy = attackingHero->m_army;
    ArmyGroup defendingArmy = defendingTown->getArmy();
    NewmapCell* cell = g_game->getCell(defendingTown->getLocation());
    AICombatData attacker(attackingHero, &attackingArmy, 1.25, 0,
                                 defendingTown, cell);
    AICombatData defender(0, &defendingArmy, 0.75, attackingHero, 0,
                                 cell);
    attacker.simulateCombat(defender);
    return attacker.m_totalCombatValue > 0;
}

#if 0  // @carcass

// THE THREAT-CHECKER BRACKET was only partially decidable when surveyed
// 2026-08-08. Retail puts only FOUR rows between can_take_town
// (0x428410, claimed) and get_attack_bonus (0x428710, claimed) -
// 0x428570 (13 B), 0x428580 (289 B), 0x4286b0 (33 B) and 0x4286e0
// (38 B) - while the Dreamcast roster lists ELEVEN bodies there, six of
// them 4- to 18-byte accessors that /Ob2 folds away without trace. With
// seven of eleven candidates unplaceable the ranks originally carried no
// information. The three game bodies are now independently resolved:
//   * 0x428570 is now proven as type_town_threat_checker::mark_town:
//     it is slot 2 of the checker's three-entry vtable; DC puts this method
//     in slot 2 too and names town byte +3 `threatening_heroes`; the body
//     increments precisely that byte. The active exact claim is below.
//   * 0x428580 is type_garrison_purchaser::mark_town: derived vtable slot 2
//     agrees with DC, and its town-army/artifact calls feed the now-proven
//     six-argument Complete purchaser path. The active claim is below.
//   * 0x4286b0 is now proven as type_creature_source's three-argument
//     constructor. The old bracket note mistakenly counted the implicit
//     `this` as a fourth stack parameter; `ret 0xc` is exactly the DC
//     signature, and the four stores reproduce its 0/4/8/10 layout.
//   * 0x4286e0 (`ret`, frees [this+0x30], caller in another TU's band)
//     is the same compiler-generated vector teardown shape as ai.obj's
//     0x420cf0, not an ai_player.cpp body at all.
// Only 0x4286e0 is not a source body; it remains compiler-generated teardown.

// E:\gamedcs\ai_player.cpp:89
DC_ONLY(0x2dd40, 0x22)
void TownThreatChecker::TownThreatChecker(long new_player)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:97
DC_ONLY(0x2dd64, 0x104)
void TownThreatChecker::checkTowns()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:134
DC_ONLY(0x2de68, 0x42)
void TownThreatChecker::clearMarks()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:146
DC_ONLY(0x2deac, 0xF2)
void TownThreatChecker::markTowns(Hero* enemy_hero, SearchArray* search_array)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:179
DC_ONLY(0x2dfa0, 0x4)
unsigned char TownThreatChecker::isMarked(const Town* our_town)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:186
DC_ONLY(0x2dfa4, 0x12)
void TownThreatChecker::markTown(Town* our_town)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:195
DC_ONLY(0x2dfb8, 0x32)
void GarrisonPurchaser::GarrisonPurchaser(long new_player)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:202
DC_ONLY(0x2dfec, 0x4)
void GarrisonPurchaser::clearMarks()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:209
DC_ONLY(0x2dff0, 0x4)
unsigned char GarrisonPurchaser::isMarked(const Town* our_town)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:217
DC_ONLY(0x2dff4, 0xA0)
void GarrisonPurchaser::markTown(Town* our_town)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:230
DC_ONLY(0x2e094, 0xC8)
long AIPlayer::getResourceValue(int* resources)
{
    // @stub
}

#endif  // @carcass

VA(0x00428570, 0x0D)  // dc 0x2dfa4
void TownThreatChecker::markTown(Town* ourTown) const
{
    ++ourTown->m_threateningHeroes;
}
// Before normalization (type): type_garrison_purchaser.
class GarrisonPurchaser : public TownThreatChecker {
public:
    // E:\gamedcs\ai_player.cpp:195, dc 0x2dfb8
    GarrisonPurchaser(int newPlayer)
        : TownThreatChecker(newPlayer) {}
    virtual void clearMarks() const;
    virtual unsigned char isMarked(const Town* ourTown) const;
    virtual void markTown(Town* ourTown) const;
};

VA(0x00428580, 0x121)  // dc 0x2dff4
void GarrisonPurchaser::markTown(Town* ourTown) const
{
    AICreaturePurchaser purchaser(m_currentPlayerId, ourTown);
    PlayerData* player = &g_game->m_players[m_currentPlayerId];
    unsigned char hasAngelicAlliance = player->hasGivenArtifact(0x81);
    purchaser.setSubtractMode(0);
    purchaser.doPurchase(&ourTown->getArmy(), 3, 0, player->m_resources,
                          1, hasAngelicAlliance);
}

VA_COMPGEN(0x004286e0, 0x26, IMPLICIT_DTOR, AICreaturePurchaser)

VA(0x00428710, 0x2D)  // dc 0x2e15c
float AIPlayer::getAttackBonus(short player)
{
    if (player < 0)
        return 0.0f;
    if (g_game->isHuman(player))
        return s_attackHumanBonus;
    return s_attackComputerBonus;
}

// E:\gamedcs\ai_player.cpp:258
// Retail establishes the complete economy model: current stock plus two
// turns of production supplies resources; every legal build contributes
// the per-resource maximum cost; all 145 creature records are valued and
// sorted so the strongest three populations contribute recruitment cost;
// Marketplace count selects the trading efficiency; and the resulting
// seven doubles are mirrored into playerData before the six-resource
// running total is divided by five.

// Residual (97.4340%): the algorithm, calls, loops, floating-point flow,
// and persistent fields agree. Two levers closed the old 86.38 plateau
// (2026-08-20): the top-three cost loop copies its creature record BY VALUE
// (retail's three-dword copy with spilled value/amount reads `type` once,
// precomputes the traits row, strength-reduces both walks and counts DOWN
// `mov edx,7 / dec/jne`; the const-reference re-read `type` per iteration
// and pinned an indexed up-count, +7.03), and the /Ob2 numerator device
// below the average loop (+4.03, see its comment). Remaining delta:
// ECX/EDX and EBX/EDI transpositions in the trading-value loop and the
// amount-word read (`movsx esi, cx` from the register copy vs our
// `movsx esi, word ptr [ecx+8]` from the source) - register-homing family;
// the creation-order probes measured against it are in the device note.
VA(0x00428740, 0x68E)  // linkorder, dc 0x2e188
void AIPlayer::calculateDemand()
{
    PlayerData* player = &g_game->m_players[m_team];
    memset(m_resourceSupply, 0, sizeof(m_resourceSupply));
    memset(m_resourceDemand, 0, sizeof(m_resourceDemand));

    int supplyResource;
    for (supplyResource = 0; supplyResource < 7; supplyResource++)
        m_resourceSupply[supplyResource] = player->m_resources[supplyResource]
            + 2 * player->m_ai.m_turnProductionResource[supplyResource];

    int buildingTownIndex;
    for (buildingTownIndex = 0; buildingTownIndex < player->m_numTowns;
         buildingTownIndex++) {
        Town* currentTown = g_game->getTown(
            player->m_townIds[buildingTownIndex]);
        __int64 buildMask = currentTown->getBuildableMask();
        int building;
        for (building = 0; building < 44; building++) {
            if (g_bitNumber[building] & buildMask) {
                int* buildCost = currentTown->getBuildCostArray(
                    type_building_id(building));
                int buildResource;
                for (buildResource = 0; buildResource < 7; buildResource++)
                    m_resourceDemand[buildResource] = cppMax(
                        m_resourceDemand[buildResource],
                        static_cast<long>(buildCost[buildResource]));
            }
        }
    }

    std::vector<CreatureValue> creatures(145);
    int creatureIndex;
    for (creatureIndex = 0; creatureIndex < 145; creatureIndex++) {
        {
            int value = creatureIndex;
            memcpy(&creatures[creatureIndex].m_type, &value,
                   sizeof creatures[creatureIndex].m_type);
        }
        creatures[creatureIndex].m_amount = 0;
    }

    int dwellingTownIndex;
    for (dwellingTownIndex = 0; dwellingTownIndex < player->m_numTowns;
         dwellingTownIndex++) {
        Town* currentTown = g_game->getTown(
            player->m_townIds[dwellingTownIndex]);
        short* population = currentTown->m_population;
        for (int dwelling = 0; dwelling < 14; dwelling++, population++) {
            short amount = *population;
            if (g_game->m_day >= 5) {
                short growth = currentTown->getGrowthRate(dwelling);
                amount += growth;
            }
            if (amount > 0) {
                int creatureType = g_townDwellingCreatures[
                    currentTown->m_type * 14 + dwelling];
                creatures[creatureType].m_amount += amount;
            }
        }
    }

    int valueCreature;
    for (valueCreature = 0; valueCreature < 145; valueCreature++)
        creatures[valueCreature].m_value = creatures[valueCreature].m_amount
            * g_creatureTypeTraits[valueCreature].m_aiValue;

    std::sort(creatures.begin(), creatures.end(),
              std::greater<CreatureValue>());
    int valuableCreature;
    for (valuableCreature = 0;
         valuableCreature < 3 && valuableCreature < creatures.size();
         valuableCreature++) {
        CreatureValue creatureInfo = creatures[valuableCreature];
        int costResource;
        for (costResource = 0; costResource < 7; costResource++)
            m_resourceDemand[costResource] +=
                g_creatureTypeTraits[creatureInfo.m_type].m_cost[costResource]
                * creatureInfo.m_amount;
    }

    int markets = 0;
    int marketTownIndex;
    for (marketTownIndex = 0; marketTownIndex < player->m_numTowns;
         marketTownIndex++) {
        Town* currentTown = g_game->getTown(
            player->m_townIds[marketTownIndex]);
        if (currentTown->isLegalBuilding(MARKETPLACE_ID))
            markets++;
    }
    markets = limit(1, markets, 10);
    double efficiency = g_tradingPostEfficency[markets];

    int valueResource;
    for (valueResource = 0; valueResource < 7;
         valueResource++) {
        double totalValue;
        if (m_resourceDemand[valueResource] == 0) {
            totalValue = efficiency;
        } else {
            totalValue = m_resourceDemand[valueResource];
            if (m_resourceDemand[valueResource]
                <= m_resourceSupply[valueResource]) {
                totalValue += (m_resourceSupply[valueResource]
                    - m_resourceDemand[valueResource]) * efficiency;
                totalValue /= m_resourceSupply[valueResource];
            } else {
                if (m_resourceSupply[valueResource] > 1)
                    totalValue /= m_resourceSupply[valueResource];
                if (totalValue > 1.0 / efficiency)
                    totalValue = 1.0 / efficiency;
            }
        }
        totalValue *= getMarketValue(GameResource(valueResource));
        m_resourceValue[valueResource] = totalValue;
        player->m_ai.m_resourceValue[valueResource] = totalValue;
    }

    double averageValue = 0;
    int averageResource;
    for (averageResource = 0; averageResource < 6; averageResource++)
        averageValue += m_resourceValue[averageResource];
    player->m_ai.m_averageResourceValue = averageValue / 5;
}

VA(0x00428dd0, 0x33E)  // dc 0x2e7d8
void AIPlayer::endTurn()
{
    PlayerData* player = &g_game->m_players[m_team];
    g_game->calculateProduction();

    for (int resource = 0; resource < 7; resource++) {
        m_reservedFunds[resource] -= player->m_ai.m_turnProductionResource[resource];
        if (m_reservedFunds[resource] < 0)
            m_reservedFunds[resource] = 0;
    }

    GarrisonPurchaser purchaser(m_team);
    purchaser.checkTowns();
    TownThreatChecker checker(m_team);
    checker.checkTowns();

    purchaseBuildings();
    hireHeroes();
    calculateDemand();

    short townIndex = 0;
    if (townIndex < player->m_numTowns) {
        while (true) {
            Town* currentTown = g_game->getTown(player->m_townIds[townIndex]);
            if (currentTown->hasBuilding(MARKETPLACE_ID, 1)) {
                for (short playerId = 0; playerId < 8; playerId++) {
                    if (!g_game->m_playerDisabled[playerId]
                        && playerId != m_team
                        && g_game->onSameTeam(playerId, m_team)
                        && !g_game->m_players[playerId].isHuman())
                        makeGift(playerId);
                }
                for (short humanPlayerId = 0; humanPlayerId < 8;
                     humanPlayerId++) {
                    if (!g_game->m_playerDisabled[humanPlayerId]
                        && humanPlayerId != m_team
                        && g_game->onSameTeam(humanPlayerId, m_team)
                        && g_game->m_players[humanPlayerId].isHuman())
                        makeGift(humanPlayerId);
                }
                break;
            }
            townIndex++;
            if (townIndex >= player->m_numTowns)
                break;
        }
    }

    std::string msg;
    for (short warningResource = 0; warningResource < 7; ++warningResource) {
        if (player->m_resources[warningResource] < 0) {
            msg += formatString(
                g_aiResourceWarningFormat,
                player->m_resources[warningResource],
                g_resourceNames[warningResource]);
        }
    }
    if (msg.length() > 0)
        normalDialog(msg.c_str(), 1, -1, -1, -1, 0, -1, 0,
                     -1, 0, -1, 0);
}

// E:\gamedcs\ai_player.cpp:504
// Retail proves both halves of the alliance exchange. Positive surplus is
// capped by the recipient gap, minimum reserves, explicit reservations,
// threshold and five-times-recipient tests. AI recipients refresh demand
// and receive only their shortage; human recipients receive local dialogs
// or the 0x432/0x433 network messages, including negative-surplus requests.
// The request dialog alone observes CTurnDuration and times out at 15000 ms.

// Dreamcast names player as playerData& and calls the by-value min/max
// wrappers in the surplus calculation (lines 519, 522/526, 541). Retail
// independently copies both inputs to each selector into temporary homes.
// Those recovered boundaries move the current 79.71% implementation to
// 84.82%. The recipient-demand selector also copies both inputs in retail
// (0x4292d6 / 0x4292db), unlike DC's direct-reference std::min; the ordinary
// min wrapper reproduces those copies and reaches 85.05%.

// Both dialog-resource records are initialized BEFORE IsLocalHuman, then
// shared by the local display and remote-message arms. DC lines 591-600 and
// 622-631 prove the scope; retail stores the first pair at 0x4293c9/0x4293d1
// before the call and reloads it for CGiftMsg. Recovering these lifetimes
// reaches 87.31% and fixes the entry register roles previously described as
// an unreachable compiler-state wall.

VA(0x00429110, 0x6AC)  // linkorder, dc 0x2ea20
void AIPlayer::makeGift(long playerId)
{
    PlayerData& player = g_game->m_players[m_team];
    long surplus[7];
    int resource;

    for (resource = 0; resource < 7; resource++) {
        surplus[resource] = m_resourceSupply[resource]
            - m_resourceDemand[resource];
        if (surplus[resource] > 0) {
            long recipientAmount = g_game->m_players[playerId].m_resources[resource];
            surplus[resource] = min(
                surplus[resource],
                (player.m_resources[resource] - recipientAmount) / 2L);
            if (resource == GOLD)
                surplus[resource] = min(
                    surplus[resource], player.m_resources[resource] - 10000L);
            else
                surplus[resource] = min(
                    surplus[resource], player.m_resources[resource] - 20L);

            surplus[resource] -= m_reservedFunds[resource];
            if (resource == GOLD) {
                if (surplus[resource] < 1000)
                    surplus[GOLD] = 0;
            } else if (surplus[resource] < 5) {
                surplus[resource] = 0;
            }
            if (surplus[resource] < 5 * recipientAmount)
                surplus[resource] = 0;
            surplus[resource] = max(surplus[resource], 0L);
        }
    }

    bool hasSurplus = false;
    for (resource = 0; resource < 7; resource++) {
        if (surplus[resource] > 0)
            hasSurplus = true;
    }
    if (!hasSurplus)
        return;

    if (!g_game->m_players[playerId].isHuman()) {
        AIPlayer* recipientAi = &g_aiPlayers[playerId];
        recipientAi->calculateDemand();
        for (resource = 0; resource < 7; resource++) {
            surplus[resource] = min(
                surplus[resource],
                recipientAi->m_resourceDemand[resource]
                    - recipientAi->m_resourceSupply[resource]);
            if (surplus[resource] > 0) {
                g_game->m_players[playerId].m_resources[resource] += surplus[resource];
                player.m_resources[resource] -= surplus[resource];
            }
        }
        return;
    }

    for (resource = 0; resource < 7; resource++) {
        if (surplus[resource] > 0) {
            g_game->m_players[playerId].m_resources[resource] += surplus[resource];
            player.m_resources[resource] -= surplus[resource];
        }
    }

    if (!g_game->m_players[playerId].isHuman())
        return;

    std::vector<DialogResource> list;
    for (resource = 0; resource < 7; resource++) {
        if (surplus[resource] > 0) {
            DialogResource displayedResource;
            displayedResource.m_resource = resource;
            displayedResource.m_qualifier = surplus[resource];
            if (g_game->m_players[playerId].isLocalHuman()) {
                list.push_back(displayedResource);
            } else if (g_networkActive69954c) {
                CGiftMsg msg(g_netLocalGamePos, displayedResource.m_resource,
                             displayedResource.m_qualifier);
                transmitRemoteData(&msg, playerId, 0, 1);
            }
        }
    }

    std::string message;
    if (g_game->m_players[playerId].isLocalHuman()) {
        message = formatString(
            g_generalText->getText(GENERAL_TEXT_AI_GIFT_RECEIVED),
            g_playerColorNames[m_team]);
        extendedDialog(message.c_str(), list, -1, -1, 0);
    }

    list.clear();
    for (resource = 0; resource < 7; resource++) {
        if (surplus[resource] < 0) {
            DialogResource requestedResource;
            requestedResource.m_resource = resource;
            requestedResource.m_qualifier = 0;
            if (g_game->m_players[playerId].isLocalHuman()) {
                list.push_back(requestedResource);
            } else if (g_networkActive69954c) {
                CGiftRequestMsg msg(g_netLocalGamePos, requestedResource.m_resource);
                transmitRemoteData(&msg, playerId, 0, 1);
            }
        }
    }

    if (g_game->m_players[playerId].isLocalHuman() && list.size()) {
        if (list.size() == 1) {
            message = formatString(
                g_generalText->getText(
                    GENERAL_TEXT_AI_SINGLE_RESOURCE_REQUEST),
                g_playerColorNames[m_team],
                g_resourceNames[list[0].m_resource]);
        } else {
            message = formatString(
                g_generalText->getText(
                    GENERAL_TEXT_AI_MULTIPLE_RESOURCE_REQUEST),
                g_playerColorNames[m_team]);
        }
        int timeout = 0;
        if (g_turnDuration69d630.isOn())
            timeout = 15000;
        extendedDialog(message.c_str(), list, -1, -1, timeout);
    }
}

VA(0x004297c0, 0x149)  // dc 0x2f148
void AIPlayer::startTurn()
{
    PlayerData* player = &g_game->m_players[m_team];

    for (int i = 0; i < player->m_numHeroes; i++) {
        Hero* currentHero = g_game->getHero(player->m_heroes[i]);
        currentHero->m_targetIsCritical = 0;
        currentHero->m_isSleeping = 0;
    }

    for (int j = 0; j < player->m_numTowns; j++) {
        Town* currentTown = g_game->getTown(player->m_townIds[j]);
        if (currentTown->m_garrisonHeroId >= 0) {
            Hero* currentHero = g_game->getHero(currentTown->m_garrisonHeroId);
            currentHero->m_targetIsCritical = 0;
            currentHero->m_isSleeping = 0;
        }
    }

    g_game->calculateProduction();
    m_magusHutValue = findMagusHutValue(
        m_team, g_game->m_setup.m_difficulty > 0 && player->m_numTowns > 0);

    GarrisonPurchaser garrisonPurchaser(m_team);
    garrisonPurchaser.checkTowns();
    TownThreatChecker threatChecker(m_team);
    threatChecker.checkTowns();

    calculateReserve();
    calculateDemand();
    player->guessGrailLocation(m_team);
}

VA(0x00429910, 0x195)  // dc 0x2efc8
long findMagusHutValue(long playerId, unsigned char exploreMode)
{
    long value = 0;
    type_point point;
    for (point.m_z = 0; point.m_z < g_game->m_worldMap.getNumLevels(); point.m_z++) {
        for (point.m_x = 0; point.m_x < g_mapWidth; point.m_x++) {
            for (point.m_y = 0; point.m_y < g_mapHeight; point.m_y++) {
                NewmapCell* cell = g_game->getCell(point);
                if (cell->m_type == EYE_OF_MAGI && cell->m_isTrigger)
                    value += aiValueOfObservatory(point, playerId, 10);
            }
        }
    }
    if (exploreMode && value > 0)
        return 1000000;
    return value;
}

VA(0x00429ab0, 0x12)  // dc 0x2f268
void AIPlayer::resetMagusHutValue()
{
    m_magusHutValue = findMagusHutValue(m_team, 0);
}

VA(0x00429ad0, 0x280)  // anchor-callee, dc 0x2f280
void AIPlayer::calculateReserve()
{
    PlayerData* player = &g_game->m_players[m_team];
    std::vector<CreatureValue> creatures;
    memset(m_reservedFunds, 0, sizeof(m_reservedFunds));
    short dwelling;
    short* population;
    Town* currentTown;

    for (short townIndex = 0; townIndex < player->m_numTowns; townIndex++) {
        currentTown = g_game->getTown(player->m_townIds[townIndex]);
        creatures.clear();

        dwelling = 0;
        population = currentTown->m_population;
        for (; dwelling < 14; dwelling++, population++) {
            if (*population > 0) {
                CreatureValue creatureInfo;
                {
                    int value = g_townDwellingCreatures[
                        currentTown->m_type * 14 + dwelling];
                    memcpy(&creatureInfo.m_type, &value,
                           sizeof creatureInfo.m_type);
                }
                creatureInfo.m_amount = *population;
                creatureInfo.m_value = static_cast<short>(creatureInfo.m_amount
                    * g_creatureTypeTraits[creatureInfo.m_type].m_aiValue);
                creatures.push_back(creatureInfo);
            }
        }

        std::sort(creatures.begin(), creatures.end());
        // Dreamcast names total_cost as long[7], distinct from int cost[7].
        // Restoring that type is byte-flat at 91.2557%.
        long totalCost[7];
        memset(totalCost, 0, sizeof(totalCost));
        int cost[7];
        for (short creature = static_cast<short>(creatures.size() - 1);
             creature >= 0 && creature >= creatures.size() - 2;
             creature--) {
            getMonsterCost(creatures[creature].m_type, cost);
            for (short resource = 0; resource < 7; resource++)
                totalCost[resource] += cost[resource]
                    * creatures[creature].m_amount;
        }

        for (int reserveResource = 0; reserveResource < 7; reserveResource++) {
            if (totalCost[reserveResource] > m_reservedFunds[reserveResource])
                m_reservedFunds[reserveResource] = totalCost[reserveResource];
        }
    }
}

// DC ai_player.cpp:895..919 proves this ordinary adjacent helper and its
// long parameter. Complete naturally expands both calls in fillProhibitedArray;
// the forceinline removal and short-loop family preserve both expansions and
// the exact caller. Do not add an inline keyword to steer that decision.
static long sumPlayerDwellings(long playerId)
{
    long value = 0;
    PlayerData* player = &g_game->m_players[playerId];
    for (int townIndex = 0; townIndex < player->m_numTowns; ++townIndex) {
        Town* currentTown = g_game->getTown(player->m_townIds[townIndex]);
        for (int dwelling = 0; dwelling < 14; ++dwelling) {
            long growth = currentTown->getGrowthRate(dwelling);
            if (growth > 0) {
                CreatureType creature = g_townDwellingCreatures[
                    currentTown->m_type * 14 + dwelling];
                value += g_creatureTypeTraits[creature].m_aiValue * growth;
            }
        }
    }
    return value;
}

VA(0x00429d50, 0x3F9)  // dc 0x2f694
void fillProhibitedArray(PlayerData* player, unsigned char* prohibited)
{
    long humanStrength;
    int income[7];
    short i;
    int resources[7];

    for (i = 0; i < 7; ++i)
        income[i] = player->m_ai.m_turnProductionResource[i] * 7;

    for (i = 0; i < player->m_numTowns; ++i) {
        Town* currentTown = g_game->getTown(player->m_townIds[i]);
        short dwelling;
        for (dwelling = 0; dwelling < 14; ++dwelling) {
            short growth = currentTown->getGrowthRate(dwelling);
            if (growth > 0) {
                CreatureType creature = g_townDwellingCreatures[
                    currentTown->m_type * 14 + dwelling];
                getMonsterCost(creature, resources);
                for (short resource = 0; resource < 7; ++resource) {
                    income[resource] -= resources[resource] * growth;
                }
            }
        }
    }

    long localGrowth = 0;
    humanStrength = 0;
    if (g_game->m_setup.m_difficulty == 0) {
        int localTeam = g_netLocalGamePos < 0
            ? g_netLocalGamePos
            : g_game->m_mapHeader.m_teamInfo[g_netLocalGamePos];
        if (localTeam < 0 || !g_game->isHumanTeam(localTeam)) {
            for (i = 0; i < 8; ++i) {
                if (!g_game->m_playerDisabled[i]
                    && g_game->isHuman(i)) {
                    humanStrength = max(humanStrength, sumPlayerDwellings(i));
                }
            }

            localGrowth = sumPlayerDwellings(g_netLocalGamePos);
        }
    }

    for (int creature = 0; creature < 145; ++creature) {
        prohibited[creature] = 0;
        getMonsterCost(creature, resources);
        for (short resource = 0; resource < 6; ++resource) {
            if (resources[resource] > 0 && income[resource] <= 0)
                prohibited[creature] = 1;
        }

        if (g_game->m_setup.m_difficulty == 0) {
            int localTeam = g_netLocalGamePos < 0
                ? g_netLocalGamePos
                : g_game->m_mapHeader.m_teamInfo[g_netLocalGamePos];
            if (localTeam < 0 || !g_game->isHumanTeam(localTeam)) {
                    if (g_creatureTypeTraits[creature].m_level
                        == TOWN_DWELLING_COUNT - 1)
                    prohibited[creature] = 1;
                if (g_creatureTypeTraits[creature].m_growthRate
                            * g_creatureTypeTraits[creature].m_aiValue
                        + localGrowth
                        > humanStrength) {
                    prohibited[creature] = 1;
                }
            }
        }
    }
}

#if 0  // @carcass

// value_of_dwelling (dc 0x2f4b0) promoted to VA(0x0042b520) in RVA order above.
// value_of_dwelling_upgrade (dc 0x2f548) promoted to VA(0x0042b5b0) above.

// E:\gamedcs\ai_player.cpp:895
DC_ONLY(0x2f5fc, 0x98)
long sumPlayerDwellings(long player_id)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:926
DC_ONLY(0x2f694, 0x20C)
void fillProhibitedArray(PlayerData* player, unsigned char* prohibited)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:1006
DC_ONLY(0x2f8a0, 0xF8)
int valueOfCastleUpgrade(Town* current_town, int* extra_cost)
{
    // @stub
}

// value_of_horde (dc 0x2f9bc) promoted to VA(0x0042b790) in RVA order above.
// value_of_horde_upgrade (dc 0x2fa88) promoted to VA(0x0042b800) above.

#endif  // @carcass

VA(0x0042a150, 0x157)  // dc 0x301c4
long AIPlayer::getTotalValue(long basicValue, int* cost)
{
    PlayerData* player = &g_game->m_players[m_team];
    unsigned char tradeNeeded = 0;
    for (int i = 0; i < 7; i++) {
        if (cost[i] > player->m_resources[i]
            && player->m_ai.m_turnProductionResource[i] == 0)
            tradeNeeded = 1;
    }

    if (tradeNeeded) {
        int supply[7];
        std::vector<long> tradeQty;
        if (!checkTradeSupply(cost, 1, supply, tradeQty))
            return -1;
        if (!canTradeResources(cost, supply, tradeQty))
            return -1;
    }

    long totalCost = 0;
    for (int resource = 0; resource < 7; resource++)
        totalCost = static_cast<long>(
            totalCost + cost[resource] * m_resourceValue[resource]);
    return basicValue * 1000 / totalCost;
}

// Residual (85.8333%, polish-45 - first full evidence pass on this row):
// blocks (31/31), branches (17/17) and the report-level call view AGREE, so
// the whole 63 B is induction-variable HOMING.  The loop needs four
// call-crossing values - &supply[resource], trade_qty, this+0x40 (the shared
// base for reserved_funds/resource_supply/resource_demand) and
// &player->resources[resource] - plus the constant `cost - supply` byte
// difference.  Retail parks only TWO in registers (ESI = supply, EDX =
// this+0x40, which is live only on the else path and therefore never crosses
// push_back) and keeps player->resources in memory at [ebp+0x14] and the
// constant at [ebp-0x8]; that leaves EDI free, so retail loads
// `cost[resource]` into EDI for the `> 0` test and divides with `idiv edi`.
// This compile keeps player->resources in EDX and this+0x40 in EDI, so the
// `> 0` test has to use EAX, EAX is then needed for the dividend, and the
// divisor is re-read as the memory operand `idiv [ecx+esi]` (plus the extra
// `mov ecx,[ebp+0x14]` that costs the byte delta).  Dreamcast agrees with the
// source as written: dc 0x30334 loads cost[resource] twice (bp 1398 and the
// `cmp/pl` at 0x303a8) and reuses the SECOND load as the __divls divisor, so
// a single hoisted `int resource_cost` above the statement would contradict
// it.  Per docs/vc6/regalloc.md 5 this is first-fit fed a different pseudo
// processing order - the C1 handle-state class.
// E:\gamedcs\ai_player.cpp:1383
VA(0x0042a2b0, 0x1BF)  // retail link order + arity, dc 0x30334
bool AIPlayer::checkTradeSupply(const int* cost, long number,
                                        int* supply,
                                        std::vector<long>& tradeQty)
{
    unsigned char tradeNeeded = 0;
    unsigned char supplyAvailable = 0;
    PlayerData* player = &g_game->m_players[m_team];
    long limit;

    tradeQty.push_back(number);
    for (int resource = 0; resource < 7; ++resource) {
        supply[resource] =
            player->m_resources[resource] - cost[resource] * number;
        if (supply[resource] < 0 && cost[resource] > 0) {
            tradeNeeded = 1;
            limit = player->m_resources[resource] / cost[resource] + 1;
            tradeQty.push_back(limit);
        } else {
            int reserve = m_reservedFunds[resource];
            if (reserve < 20)
                reserve = 20;
            supply[resource] -= reserve;

            int available = static_cast<int>(m_resourceSupply[resource]
                                             - m_resourceDemand[resource]);
            if (supply[resource] > available)
                supply[resource] = available;
            if (supply[resource] <= 0)
                supply[resource] = 0;
            else
                supplyAvailable = 1;
        }
    }

    if (!tradeNeeded || !supplyAvailable)
        return false;

    std::sort(tradeQty.begin(), tradeQty.end());
    for (int i = static_cast<int>(tradeQty.size()) - 1; i >= 1; --i) {
        if (tradeQty[i] == tradeQty[i - 1])
            tradeQty.erase(tradeQty.begin() + i);
    }
    return true;
}

VA(0x0042a470, 0x110)  // dc 0x304cc
void AIPlayer::tradeResources(const int* cost, long number)
{
    std::vector<long> tradeQty;
    int supply[7];
    if (!checkTradeSupply(cost, number, supply, tradeQty))
        return;
    if (!canTradeResources(cost, supply, tradeQty))
        return;
    if (buildMarkets(supply)) {
        tradeQty.clear();
        if (!checkTradeSupply(cost, number, supply, tradeQty))
            return;
        if (!canTradeResources(cost, supply, tradeQty))
            return;
    }
    doResourceTrade(supply);
}

// E:\gamedcs\ai_player.cpp:1474
VA(0x0042a580, 0x5BE)  // retail link order + arity, dc 0x305b4
bool AIPlayer::canTradeResources(const int* cost, int* supply,
                                         std::vector<long>& tradeQty)
{
    long markets = 0;
    unsigned char canBuildMarket = 0;
    PlayerData* player = &g_game->m_players[m_team];
    if (supply[0] >= 0
        && player->m_ai.m_turnProductionResource[0] > 0)
        canBuildMarket = 1;

    for (int townIndex = 0; townIndex < player->m_numTowns;
         ++townIndex) {
        Town* currentTown = g_game->getTown(
            player->m_townIds[townIndex]);
        if (currentTown->hasBuilding(MARKETPLACE_ID, true)
            || (canBuildMarket
                && currentTown->canBuild(MARKETPLACE_ID)))
            ++markets;
    }

    markets = min(markets, 10);
    if (markets == 0)
        return false;
    double efficiency = g_tradingPostEfficency[markets];
    long marketValue = 0;

    std::vector<long> baseCost;
    std::vector<long> unitCost;
    baseCost.insert(baseCost.begin(), tradeQty.size(), 0);
    unitCost.insert(unitCost.begin(), tradeQty.size(), 0);

    int i;
    for (i = 0; i < 7; ++i) {
        if (supply[i] > 0) {
            GameResource resource;
            {
                int ordinal = i;
                memcpy(&resource, &ordinal, sizeof resource);
            }
            marketValue = static_cast<long>(
                getMarketValue(resource) * supply[i]
                * efficiency + marketValue);
        } else if (supply[i] != 0) {
            long onHand = player->m_resources[i];
            int resourceValue;
            resourceValue = i;
            long value = getMarketValue(GameResource(resourceValue));
            for (unsigned int j = 0; j < tradeQty.size(); ++j) {
                if (tradeQty[j] * cost[i] > onHand) {
                    baseCost[j] += (tradeQty[j] * cost[i] - onHand)
                        * value;
                    unitCost[j] += cost[i] * value;
                }
            }
        }
    }

    int j;
    for (j = static_cast<int>(baseCost.size()) - 1; j >= 0; --j) {
        if (baseCost[j] <= marketValue)
            break;
    }
    if (j < 0)
        return false;

    if (j == static_cast<int>(tradeQty.size()) - 1)
        return true;

    long qty = tradeQty[j];
    if (j + 1 < tradeQty.size()
        && tradeQty[j] + 1 < tradeQty[j + 1]) {
        qty += (marketValue - baseCost[j]) / unitCost[j];
        if (qty >= tradeQty[j + 1])
            qty = tradeQty[j + 1] - 1;
    }

    long remaining = tradeQty.back() - qty;
    for (int k = 0; k < 7; ++k) {
        if (supply[k] < 0) {
            supply[k] += cost[k] * remaining;
            if (supply[k] > 0)
                supply[k] = 0;
        }
    }
    return true;
}

VA(0x0042ab40, 0xD1)  // dc 0x309d4
bool AIPlayer::buildMarkets(int* supply)
{
    PlayerData* player = &g_game->m_players[m_team];
    bool built = false;
    if (supply[0] < 0 || player->m_ai.m_turnProductionResource[0] <= 0)
        return false;
    for (int townIndex = 0; townIndex < player->m_numTowns; ++townIndex) {
        Town* currentTown = g_game->getTown(player->m_townIds[townIndex]);
        if (!(currentTown->m_active & g_bitNumber[MARKETPLACE_ID])
            && currentTown->canBuild(MARKETPLACE_ID)) {
            if (!canBuy(currentTown, MARKETPLACE_ID))
                return built;
            currentTown->buyBuilding(MARKETPLACE_ID);
            built = true;
        }
    }
    return built;
}

VA(0x0042ac20, 0x1DE)  // dc 0x30a70
void AIPlayer::doResourceTrade(int* supply)
{
    int marketCount = 0;
    PlayerData* player = &g_game->m_players[m_team];
    for (int townIndex = 0; townIndex < player->m_numTowns; ++townIndex) {
        Town* currentTown = g_game->getTown(player->m_townIds[townIndex]);
        if (currentTown->m_active & g_bitNumber[MARKETPLACE_ID])
            ++marketCount;
    }

    marketCount = cppMin(marketCount, 10);
    if (marketCount == 0)
        return;

    double efficiency = g_tradingPostEfficency[marketCount];
    for (int source = 0; source < 7; ++source) {
        if (supply[source] <= 0)
            continue;
        for (int dest = 0; dest < 7; ++dest) {
            if (supply[dest] >= 0)
                continue;
            double ratio;
            {
                GameResource sourceResource;
                GameResource destResource;
                {
                    int ordinal = source;
                    memcpy(&sourceResource, &ordinal, sizeof sourceResource);
                }
                {
                    int ordinal = dest;
                    memcpy(&destResource, &ordinal, sizeof destResource);
                }
                ratio = getTradeRatio(sourceResource, destResource,
                                      efficiency);
            }
            long traded = static_cast<long>(0.99999 - supply[dest] * ratio);
            long limit = static_cast<long>(
                static_cast<long>(supply[source] / ratio) * ratio);
            if (traded > limit)
                traded = limit;
            supply[source] -= traded;
            player->m_resources[source] -= traded;
            long cost = static_cast<long>(traded / ratio);
            supply[dest] += cost;
            player->m_resources[dest] += cost;
            if (supply[dest] > 0)
                supply[dest] = 0;
            if (supply[source] <= 0)
                break;
        }
    }
    calculateDemand();
}

long valueOfDwelling(Town* currentTown, short dwelling,
                       unsigned char* prohibited, int* extraCost);
long valueOfDwellingUpgrade(Town* currentTown, short dwelling,
                               int* extraCost);
int valueOfCastleUpgrade(Town* currentTown, int* extraCost);
long valueOfHorde(Town* currentTown, type_building_id building,
                    unsigned char* prohibited, int* extraCost);
long valueOfHordeUpgrade(Town* currentTown, type_building_id building,
                            unsigned char* prohibited, int* extraCost);
long valueOfHall(Town* currentTown, type_building_id building);
int aiResourceCost(const PlayerData* player, const int* resources);
int canBuy(const Town* currTown, int buildingId);

// E:\gamedcs\ai_player.cpp:1045
// Single-call-site static: /Ob2 folds it into value_of_building below,
// which is itself folded into purchase_building - no retail body.
DC_ONLY(0x2f998, 0x24)
static long valueOfSilo(Town* currentTown, PlayerData* player)
{
    return 7 * aiResourceCost(player, currentTown->getSiloIncome());
}

// E:\gamedcs\ai_player.cpp:1147
// Prices one candidate building for the town, seeding extra_cost with the
// creature costs the dwelling/horde helpers accumulate. -1 refuses
// illegal, built, and Grail slots and every threatened growth building.
// The faction switch keeps retail's source order (Stronghold's arm sits
// between Tower's and Necropolis'). Single call site - no retail body.
DC_ONLY(0x2fdac, 0x29c)
static long valueOfBuilding(Town* currentTown, type_building_id building,
                              unsigned char* prohibitedCreatures,
                              int* extraCost)
{
    PlayerData* player = &g_game->m_players[currentTown->m_owner];
    switch (building) {
    case CASTLE_CITADEL_ID:
    case CASTLE_CASTLE_ID:
        return valueOfCastleUpgrade(currentTown, extraCost);
    case HALL_VILLAGE_ID:
    case HALL_TOWN_ID:
    case HALL_CITY_ID:
    case HALL_CAPITOL_ID:
        return valueOfHall(currentTown, building);
    case MARKETPLACE_SILO_ID:
        if (currentTown->m_threateningHeroes)
            return -1;
        return valueOfSilo(currentTown, player);
    case HORDE_ID:
    case HORDE_2_ID:
        if (currentTown->m_threateningHeroes)
            return -1;
        return valueOfHorde(currentTown, building, prohibitedCreatures,
                              extraCost);
    case HORDE_UPG_ID:
    case HORDE_2_UPG_ID:
        if (currentTown->m_threateningHeroes)
            return -1;
        return valueOfHordeUpgrade(currentTown, building,
                                      prohibitedCreatures, extraCost);
    case DWELLING_0_ID:
    case DWELLING_1_ID:
    case DWELLING_2_ID:
    case DWELLING_3_ID:
    case DWELLING_4_ID:
    case DWELLING_5_ID:
    case DWELLING_6_ID:
        if (currentTown->m_threateningHeroes)
            return -1;
        return valueOfDwelling(currentTown, building - DWELLING_0_ID,
                                 prohibitedCreatures, extraCost);
    case DWELLING_0_UPG_ID:
    case DWELLING_1_UPG_ID:
    case DWELLING_2_UPG_ID:
    case DWELLING_3_UPG_ID:
    case DWELLING_4_UPG_ID:
    case DWELLING_5_UPG_ID:
    case DWELLING_6_UPG_ID:
        if (currentTown->m_threateningHeroes)
            return -1;
        return valueOfDwellingUpgrade(currentTown,
                                         building - DWELLING_0_ID,
                                         extraCost);
    default:
        switch (currentTown->m_type) {
        case TOWN_RAMPART:
            if (building == EXTRA_1_ID) {
                if (g_game->m_day == g_aiDayOfWeekSunday)
                    return static_cast<long>(
                        player->m_resources[GOLD]
                        * player->m_ai.m_resourceValue[GOLD] / 10.0);
            } else if (building == SPECIAL_BUILDING_ID) {
                return 2 * player->m_ai.m_averageResourceValue;
            }
            break;
        case TOWN_TOWER:
            if (building == EXTRA_0_ID)
                return 100;
            break;
        case TOWN_STRONGHOLD:
            if (building == SPECIAL_BUILDING_ID
                && currentTown->m_threateningHeroes
                && currentTown->m_garrisonHeroId >= 0)
                return 5000;
            break;
        case TOWN_NECROPOLIS:
            if (building == EXTRA_0_ID) {
                long value = 0;
                for (int i = 0; i < player->m_numHeroes; ++i) {
                    if (g_game->getHero(player->m_heroes[i])->m_heroClass
                        == classNecromancer)
                        value += 1000;
                }
                return value;
            } else if (building == SPECIAL_BUILDING_ID) {
                return 10;
            }
            break;
        case TOWN_FORTRESS:
            if ((building == EXTRA_0_ID || building == EXTRA_1_ID)
                && currentTown->m_threateningHeroes)
                return static_cast<const Town*>(currentTown)
                           ->getArmy().getAIValue() / 20;
            break;
        }
        return 0;
    }
}

// E:\gamedcs\ai_player.cpp:1279
// Chases the requirement chain: starting from the building's own bit,
// every unmet requirement pulls its faction row from gHierarchyMask,
// drops what the town already built or the chain already holds, and
// rescans from zero. Returns the chain, or 0 when a link is not legal
// here. Single call site - no retail body.

// `k` STARTS AT `building`, NOT AT 0, and retail proves it twice inside
// purchase_building: `k = 0` makes `k < MAX_BUILDING_TYPE` provably true,
// so VC6 drops the while's top test (54 branches against retail's 55) AND
// strength-reduces `building` away entirely, giving a 0x690 frame against
// retail's 0x694. With `k = building` the guard is retail's
// `cmp <&bitNumber[building]>, <end> / jge` and `building` keeps its own
// slot at [ebp-0x34]. The two spellings are equivalent - `requirements`
// starts as bitNumber[building] alone, so a scan from 0 can only hit at
// `building` - and this one is what the bytes say. 96.20 -> 97.32.
DC_ONLY(0x30048, 0x106)
static __int64 getRequirements(const Town* currentTown,
                                type_building_id building)
{
    __int64 requirements = g_bitNumber[building];
    __int64 seen = 0;
    int k = building;
    while (k < MAX_BUILDING_TYPE) {
        if (requirements & g_bitNumber[k]) {
            {
                type_building_id buildingId;
                int ordinal = k;
                memcpy(&buildingId, &ordinal, sizeof buildingId);
                if (!currentTown->isLegalBuilding(buildingId))
                    return 0;
            }
            seen |= g_bitNumber[k];
            requirements |= g_hierarchyMask[currentTown->m_type][k];
            requirements &= ~currentTown->m_active;
            requirements &= ~seen;
            k = 0;
        } else {
            ++k;
        }
    }
    return seen;
}

// E:\gamedcs\ai_player.cpp:1313
// Single call site - no retail body.
DC_ONLY(0x30150, 0x74)
static void getFullCost(const Town* currentTown, int* result,
                          __int64 requirements)
{
    for (int k = 0; k < MAX_BUILDING_TYPE; ++k) {
        if (requirements & g_bitNumber[k]) {
            int* costs;
            {
                type_building_id buildingId;
                int ordinal = k;
                memcpy(&buildingId, &ordinal, sizeof buildingId);
                costs = currentTown->getBuildCostArray(buildingId);
            }
            for (int i = 0; i < 7; ++i)
                result[i] += costs[i];
        }
    }
}

// E:\gamedcs\ai_player.cpp:1367
// Single call site - no retail body. `k` is a SIGNED int: retail's
// strength-reduced back edge is `cmp <ptr>, <end> / jl`, and an unsigned
// counter can only ever emit `jb` (97.32 -> 97.42, branches clean).
DC_ONLY(0x302e0, 0x54)
static void markValues(long* fullValue, long totalValue,
                        __int64 requirements)
{
    for (int k = 0; k < MAX_BUILDING_TYPE; ++k) {
        if (requirements & g_bitNumber[k])
            fullValue[k] += totalValue;
    }
}

// E:\gamedcs\ai_player.cpp:1686
// Prices every candidate building in every town: value_of_building seeds
// basic_value/extra_costs per building, get_requirements resolves the
// build chain, get_full_cost adds the chain's resource bill, and
// get_total_value's affordability-scaled score is spread over the chain
// by mark_values. The best buildable-this-turn candidate across all
// towns is bought after a trade pass, gated by CanBuy for the
// hall/marketplace band and by reserved funds everywhere else.

// Residual (97.42%): VC6 hoists a second `gpGame` load above the faction
// switch, where retail reloads it inside the RAMPART arm and again inside
// the NECROPOLIS hero scan; that stolen register is also why our scan
// counter spills to the frame where retail keeps it in EDX, and why the
// three growth arms' scratch registers are rotated by one against
// retail's. Branches, rets and the frame are all exact.
VA(0x0042ae00, 0x718)  // retail callee set + arity, dc 0x30d6c
unsigned char AIPlayer::purchaseBuilding(
    unsigned char* prohibitedCreatures)
{
    int extraCosts[MAX_BUILDING_TYPE][7];
    long fullValue[MAX_BUILDING_TYPE];
    long basicValue[MAX_BUILDING_TYPE];
    long bestValue = 0;
    Town* bestTown = 0;
    int bestBuilding = MAX_BUILDING_TYPE;
    __int64 requirements;
    PlayerData* player = &g_game->m_players[m_team];

    for (short townIndex = 0; townIndex < player->m_numTowns;
         ++townIndex) {
        Town* currentTown = g_game->getTown(player->m_townIds[townIndex]);
        __int64 buildMask = currentTown->getBuildableMask();
        if (g_game->m_towns[currentTown->m_id].m_builtThisTurn)
            continue;

        memset(extraCosts, 0, sizeof(extraCosts));
        int building;
        for (building = 0; building < MAX_BUILDING_TYPE; ++building) {
            {
                type_building_id buildingId;
                int ordinal = building;
                memcpy(&buildingId, &ordinal, sizeof buildingId);
                if (!currentTown->isLegalBuilding(buildingId)
                    || (currentTown->m_active & g_bitNumber[building])
                    || building == HOLY_GRAIL_ID) {
                    basicValue[building] = -1;
                    continue;
                }
            }
            {
                type_building_id buildingId;
                int ordinal = building;
                memcpy(&buildingId, &ordinal, sizeof buildingId);
                basicValue[building] = valueOfBuilding(
                    currentTown, buildingId,
                    prohibitedCreatures, extraCosts[building]);
            }
        }

        memset(fullValue, 0, sizeof(fullValue));
        for (building = 0; building < MAX_BUILDING_TYPE; ++building) {
            if (basicValue[building] <= 0)
                continue;
            {
                type_building_id buildingId;
                int ordinal = building;
                memcpy(&buildingId, &ordinal, sizeof buildingId);
                requirements = getRequirements(currentTown, buildingId);
            }
            if (requirements == 0)
                continue;
            getFullCost(currentTown, extraCosts[building],
                          requirements);
            long totalValue = getTotalValue(basicValue[building],
                                               extraCosts[building]);
            if (totalValue < 0)
                continue;
            markValues(fullValue, totalValue, requirements);
        }

        for (building = 0; building < MAX_BUILDING_TYPE; ++building) {
            if ((buildMask & g_bitNumber[building])
                && fullValue[building] > bestValue) {
                bestValue = fullValue[building];
                bestTown = currentTown;
                bestBuilding = building;
            }
        }
    }

    if (!bestTown)
        return 0;

    int cost[7];
    {
        type_building_id buildingId;
        int ordinal = bestBuilding;
        memcpy(&buildingId, &ordinal, sizeof buildingId);
        bestTown->getBuildCost(buildingId, cost);
    }
    tradeResources(cost, 1);
    if (g_game->m_towns[bestTown->m_id].m_builtThisTurn)
        return 0;
    if (bestBuilding >= HALL_VILLAGE_ID
        && bestBuilding <= MARKETPLACE_SILO_ID) {
        if (!canBuy(bestTown, bestBuilding))
            return 0;
    } else {
        // MAX 97.8283 was measured with `i != 7` - an unnamed domain compare
        // that fails the cleanliness floor (docs/vc6/behavior-catalog.md D24).
        for (short i = 0; i < 7; ++i) {
            if (m_reservedFunds[i] + cost[i] > player->m_resources[i])
                return 0;
        }
    }
    {
        type_building_id buildingId;
        int ordinal = bestBuilding;
        memcpy(&buildingId, &ordinal, sizeof buildingId);
        if (!bestTown->buyBuilding(buildingId))
            return 0;
    }
    calculateDemand();
    return 1;
}

VA(0x0042b520, 0x8b)  // dc 0x2f4b0
long valueOfDwelling(Town* currentTown, short dwelling, unsigned char* prohibited, int* extraCost)
{
    CreatureType creature = g_townDwellingCreatures[
        currentTown->m_type * 14 + dwelling];
    if (prohibited[creature])
        return -1;
    const CreatureTypeTraits& traits = g_creatureTypeTraits[creature];
    long growth = traits.m_growthRate;
    if (g_game->m_day >= 5)
        growth = currentTown->getCastleGrowthBonus(creature) + 2 * growth;
    for (int i = 0; i < 7; i++)
        extraCost[i] += traits.m_cost[i] * growth;
    return traits.m_aiValue * growth;
}

VA(0x0042b5b0, 0xbe)  // dc 0x2f548
long valueOfDwellingUpgrade(Town* currentTown, short dwelling, int* extraCost)
{
    short baseDwelling = dwelling - 7;
    CreatureType creature = g_townDwellingCreatures[
        currentTown->m_type * 14 + baseDwelling];
    CreatureType upgraded = g_townDwellingCreatures[
        currentTown->m_type * 14 + dwelling];
    long amount = currentTown->m_population[baseDwelling];
    if (g_game->m_day >= 5)
        amount += currentTown->getGrowthRate(baseDwelling);
    const CreatureTypeTraits& baseTraits = g_creatureTypeTraits[creature];
    const CreatureTypeTraits& upgradedTraits = g_creatureTypeTraits[upgraded];
    for (int i = 0; i < 7; i++)
        extraCost[i] += (upgradedTraits.m_cost[i]
                          - baseTraits.m_cost[i]) * amount;
    return (upgradedTraits.m_aiValue - baseTraits.m_aiValue) * amount;
}

VA(0x0042b670, 0x111)  // dc 0x2f8a0
int valueOfCastleUpgrade(Town* currentTown, int* extraCost)
{
    long value = 0;
    if (g_game->m_mapHeader.m_victoryCondition.m_type
            == VICTORY_CONDITION_UPGRADE_TOWN
        && g_game->m_mapHeader.m_victoryCondition.m_townX == currentTown->m_mapX
        && g_game->m_mapHeader.m_victoryCondition.m_townY == currentTown->m_mapY
        && g_game->m_mapHeader.m_victoryCondition.m_townZ == currentTown->m_mapZ
        && !(g_bitNumber[CASTLE_FORT_ID
                       + g_game->m_mapHeader.m_victoryCondition.m_castleLevel]
             & currentTown->m_active))
        value = 5000000;
    if (g_game->m_day >= 5) {
        for (short dwelling = 0; dwelling < 14; ++dwelling) {
            if (currentTown->getGrowthRate(dwelling) > 0) {
                int creature = g_townDwellingCreatures[
                    currentTown->m_type * 14 + dwelling];
                const CreatureTypeTraits* traits =
                    g_creatureTypeTraits + creature;
                for (int i = 0; i < 7; ++i)
                    extraCost[i] += traits->m_cost[i];
                value += g_creatureTypeTraits[creature].m_aiValue;
            }
        }
    }
    return value;
}

VA(0x0042b790, 0x62)  // dc 0x2f9bc
long valueOfHorde(Town* currentTown, type_building_id building, unsigned char* prohibited, int* extraCost)
{
    HordeEffect* horde = currentTown->getHordeEffect(building);
    CreatureType creature = horde->m_creature;
    if (prohibited[creature])
        return -1;
    const CreatureTypeTraits& traits = g_creatureTypeTraits[creature];
    for (int i = 0; i < 7; i++)
        extraCost[i] += horde->m_bonus * traits.m_cost[i];
    return traits.m_aiValue * horde->m_bonus;
}

VA(0x0042b800, 0xa2)  // dc 0x2fa88
long valueOfHordeUpgrade(Town* currentTown, type_building_id building, unsigned char* prohibited, int* extraCost)
{
    HordeEffect* horde = currentTown->getHordeEffect(building);
    if (!horde)
        return -1;
    if (g_bitNumber[building - 1] & currentTown->m_built)
        return -1;
    CreatureType creature = horde->m_creature;
    if (prohibited[creature])
        return -1;
    const CreatureTypeTraits& traits = g_creatureTypeTraits[creature];
    for (int i = 0; i < 7; i++)
        extraCost[i] += horde->m_bonus * traits.m_cost[i];
    return traits.m_aiValue * horde->m_bonus;
}

VA(0x0042b8b0, 0x130)  // dc 0x2fb2c
long valueOfHall(Town* currentTown, type_building_id building)
{
    long value = 0;
    if (currentTown->m_threateningHeroes > 0)
        return -1;
    if (g_game->m_mapHeader.m_victoryCondition.m_type
            == VICTORY_CONDITION_UPGRADE_TOWN
        && g_game->m_mapHeader.m_victoryCondition.m_townX == currentTown->m_mapX
        && g_game->m_mapHeader.m_victoryCondition.m_townY == currentTown->m_mapY
        && g_game->m_mapHeader.m_victoryCondition.m_townZ == currentTown->m_mapZ
        && building >= g_game->m_mapHeader.m_victoryCondition.m_hallLevel
                           + HALL_TOWN_ID)
        value = 5000000;
    PlayerData* player = &g_game->m_players[currentTown->m_owner];
    switch (building) {
    case HALL_VILLAGE_ID:
        return static_cast<long>(
            player->m_ai.m_resourceValue[GOLD] * 3500.0 + value);
    case HALL_TOWN_ID:
        return static_cast<long>(
            player->m_ai.m_resourceValue[GOLD] * 3500.0 + value);
    case HALL_CITY_ID:
        return static_cast<long>(
            player->m_ai.m_resourceValue[GOLD] * 7000.0 + value);
    case HALL_CAPITOL_ID:
        return static_cast<long>(
            player->m_ai.m_resourceValue[GOLD] * 14000.0 + value);
    default:
        return value;
    }
}

// E:\gamedcs\ai_player.cpp:1808, dc 0x31030.
static int __cdecl maxBuyableCreatures(
    const long* funds, CreatureType type, int limit)
{
    int resources[7];
    getMonsterCost(type, resources);
    for (int resource = 0; resource < 7; ++resource) {
        if (resources[resource] > 0) {
            int affordable;
            if (funds[resource] > 0)
                affordable = funds[resource] / resources[resource];
            else
                affordable = 0;
            if (affordable < limit)
                limit = affordable;
        }
    }
    return limit;
}

// E:\gamedcs\ai_player.cpp:1838, dc 0x31094.
// Complete extends the prohibited-creature table to 145 entries.
DC_ONLY(0x31094, 0x60)
void AIPlayer::purchaseBuildings()
{
    unsigned char prohibitedCreatures[145];
    fillProhibitedArray(&g_game->m_players[m_team], prohibitedCreatures);
    while (purchaseBuilding(prohibitedCreatures)) {
    }
}

// E:\gamedcs\ai_player.cpp:1850
// Retail expands the purchaser ctor, do_swap, both set overloads,
// TownAlreadyBuiltOn (towns[id].field_02) and is_human_ally in place while
// keeping set(town-ctor path)/do_purchase/get_purchase_value/AI_arrange_army
// out of line. The dwelling scan walks bitNumber[DWELLING_0_ID..DWELLING_6_ID]
// against get_buildable_mask and prices each candidate through the
// single-candidate set overload with the leftover supply as funds.
VA(0x0042ba60, 0x447)  // retail callee set + arity, dc 0x310f4
void AIPlayer::buyCreatures(Hero* currentHero, Town* currentTown)
{
    PlayerData* player = &g_game->m_players[m_team];
    AICreaturePurchaser purchaser(currentHero->m_owner, currentTown);

    Hero* garrisonHero = 0;
    if (currentTown->m_garrisonHeroId > -1)
        garrisonHero = g_game->getHero(currentTown->m_garrisonHeroId);

    unsigned char alliance = g_game->m_players[currentHero->m_owner]
        .hasGivenArtifact(ARTIFACT_ANGELIC_ALLIANCE);
    purchaser.doSwap(currentHero,
                      const_cast<ArmyGroup*>(
                          &static_cast<const Town*>(currentTown)->getArmy()),
                      garrisonHero, alliance);

    purchaser.setSubtractMode(0);
    purchaser.doPurchase(&currentHero->m_army,
                          currentHero->getMorale(0, 0, 1),
                          const_cast<ArmyGroup*>(
                              &static_cast<const Town*>(currentTown)
                                   ->getArmy()),
                          player->m_resources, 1, alliance);

    // DC ai_player.cpp:1869/1873 has two early exits; 1893 records short
    // morale, and amount/funds/traits belong to function scope. Retail
    // retains the player pointer, but no separate resources-pointer local:
    // spelling player->resources directly restores EBX for garrison_hero,
    // the TownAlreadyBuiltOn byte test, and the 42-block control flow.
    // Function-scoped amount recovers the exact 0x94 frame and stack homes.
    // Measured 2026-09-06: CUR 72.22 -> 76.57 (early exits), 76.84 (GetTeam),
    // 97.95 (funds array), 98.18 (amount scope). The recovered two-argument
    // set helper reaches 97.77; the three-argument control is archived.
    // Residual: the single-candidate set expansion retains insert(pos,n,x)
    // where retail calls insert(pos,x), plus mask-loop register scheduling.
    // TownAlreadyBuiltOn, short morale, the other recorded local scopes,
    // and the constructor's header/initializer form are byte-flat controls.
    if (g_game->townAlreadyBuiltOn(currentTown->m_id))
        return;
    if (!g_game->m_setup.m_difficulty
        && !g_game->isHumanAlly(
               g_game->getTeam(g_netLocalGamePos)))
        return;
    short amount;
    const CreatureTypeTraits* traits;
    CreatureType creature;
    long funds[7];
    long bestValue = 0;
    union {
        int m_index;
        type_building_id m_id;
    } building, bestBuilding;
    __int64 buildMask = currentTown->getBuildableMask();
    short morale = currentHero->getMorale(0, 0, 1);
    for (building.m_index = DWELLING_0_ID;
         building.m_index <= DWELLING_6_ID; building.m_index++) {
        if (buildMask & g_bitNumber[building.m_index]) {
            creature = g_townDwellingCreatures[
                currentTown->m_type * TOWN_DWELLING_SLOTS
                + building.m_index - DWELLING_0_ID];
            traits = &g_creatureTypeTraits[creature];
            int* cost = currentTown->getBuildCostArray(building.m_id);
            unsigned char affordable = 1;
            for (int resource = 0; resource < 7; ++resource) {
                funds[resource] = player->m_resources[resource] - cost[resource];
                if (funds[resource] < 0)
                    affordable = 0;
            }
            if (affordable) {
                amount = traits->m_growthRate;
                purchaser.set(creature, &amount);
                long value = purchaser.getPurchaseValue(
                    &currentHero->m_army, morale,
                    &static_cast<const Town*>(currentTown)
                         ->getArmy(),
                    funds, alliance);
                if (value > bestValue) {
                    bestValue = value;
                    bestBuilding.m_index = building.m_index;
                }
            }
        }
    }
    if (bestValue > 0) {
        int* cost =
            currentTown->getBuildCostArray(bestBuilding.m_id);
        currentTown->buildBuilding(bestBuilding.m_index, 1, 1);
        for (int resource = 0; resource < 7; ++resource)
            player->m_resources[resource] -= cost[resource];
        purchaser.set(currentTown);
        purchaser.doPurchase(&currentHero->m_army, morale,
                              const_cast<ArmyGroup*>(
                                  &static_cast<const Town*>(
                                       currentTown)->getArmy()),
                              player->m_resources, 1, alliance);
    }
}

VA(0x0042beb0, 0x187)  // dc 0x31398
void AIPlayer::buyMageGuild(Hero* currentHero, Town* currentTown)
{
    int building;
    building = currentTown->m_mageLevel;

    if (building >= 5
        || building >= currentHero->m_skillLevel[eSecSkillWisdom] + 2
        || !currentTown->canBuild(building))
        return;

    if ((currentHero->getPrimarySkill(2) < 3
         || currentHero->getPrimarySkill(3) < 3)
        && (building > 0
            || currentHero->spellIsAvailable(SPELL_CURE)
            || currentHero->spellIsAvailable(SPELL_DISPEL)))
        return;

    if (building > 0
        || m_resourceSupply[WOOD] < m_resourceDemand[WOOD]
        || m_resourceSupply[ORE] < m_resourceDemand[ORE]) {
        PlayerData* player = &g_game->m_players[m_team];
        for (int townIndex = 0; townIndex < player->m_numTowns;
             ++townIndex) {
            Town* otherTown = g_game->getTown(player->m_townIds[townIndex]);
            int otherLevel = otherTown->m_mageLevel;
            if (otherLevel > building
                && otherLevel < currentHero->m_skillLevel[eSecSkillWisdom] + 2
                && otherTown->canBuild(otherTown->m_mageLevel))
                return;
        }
    }

    int cost[7];
    currentTown->getBuildCost(type_building_id(building), cost);
    tradeResources(cost, 1);
    if (canBuy(currentTown, building)
        && !g_game->m_towns[currentTown->m_id].m_builtThisTurn)
        currentTown->buyBuilding(type_building_id(building));
}

#if 0  // @carcass

// E:\gamedcs\ai_player.cpp:2022
DC_ONLY(0x31514, 0xA8)
// Before normalization (function): move_creatures.
void moveCreatures(ArmyGroup* army, CreatureType type, short amount)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2061
DC_ONLY(0x315bc, 0x1A)
void AICreatureSwapper::AICreatureSwapper()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2074
DC_ONLY(0x315d8, 0x92)
void AICreatureSwapper::addCreatures(CreatureType type, short amount, short slot)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2096
DC_ONLY(0x3166c, 0x168)
long AICreatureSwapper::doBestSwap(unsigned char can_take_all)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2171
DC_ONLY(0x317d4, 0x34)
short calculateImprovement(const Hero* current_hero, const Hero* second_hero)
{
    // @stub
}

// do_swap (dc 0x31808) promoted to VA(0x0042c3b0) in RVA order below;
// Complete adds the Angelic-Alliance parameter the philai callers pass.

// E:\gamedcs\ai_player.cpp:2209
DC_ONLY(0x31864, 0xC0)
long AICreatureSwapper::getSwapValue(const Hero* current_hero, const ArmyGroup* source_army, const Hero* second_hero)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2233
DC_ONLY(0x31924, 0xDA)
void AICreatureSwapper::dumpExtraCreature()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2282
DC_ONLY(0x31a00, 0xF2)
long AICreatureSwapper::chooseWeakestArmy(unsigned char is_shooter, unsigned char check_alignments)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2350
DC_ONLY(0x31af4, 0x346)
long AICreatureSwapper::valueOfAddingArmy(CreatureType type, short count, short* slot, unsigned char must_replace_creature)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2461
DC_ONLY(0x31e3c, 0x98)
void AICreaturePurchaser::AICreaturePurchaser(long player, Generator* current_generator)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2483
DC_ONLY(0x31ed4, 0x4E)
void AICreaturePurchaser::AICreaturePurchaser(long player, Town* current_town)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2495
DC_ONLY(0x31f24, 0x6E)
void AICreaturePurchaser::AICreaturePurchaser(long player, CreatureType type, short* amount, bool is_free)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2506
DC_ONLY(0x31f94, 0x68)
void AICreaturePurchaser::set(Town* current_town)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2524
DC_ONLY(0x31ffc, 0x3A)
void AICreaturePurchaser::set(CreatureType type, short* amount)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2535
DC_ONLY(0x32038, 0x250)
long AICreaturePurchaser::doBestPurchase(unsigned char trade_allowed)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2626
DC_ONLY(0x32288, 0x70)
void AICreaturePurchaser::doPurchase(ArmyGroup* new_army, short new_morale, ArmyGroup* new_adjacent_army, long* new_funds, unsigned char allow_trade)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2657
DC_ONLY(0x322f8, 0xC2)
long AICreaturePurchaser::getPurchaseValue(const ArmyGroup* new_army, short new_morale, const ArmyGroup* new_adjacent_army, const long* new_funds)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2692
DC_ONLY(0x323bc, 0x72)
void aiConsolidateArmy(ArmyGroup& current_army)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:2718
DC_ONLY(0x32430, 0x18A)
void aiArrangeArmy(ArmyGroup& current_army)
{
    // @stub
}

// split_armies (dc 0x32670) is claimed in retail-RVA order below.

// E:\gamedcs\ai_player.cpp:2952
DC_ONLY(0x3285c, 0x36)
void AI_arrange_army_for_combat(Hero* current_hero, const Hero* enemy_hero, const ArmyGroup* enemy)
{
    // @stub
}

// mark_danger_zones (dc 0x32894) is claimed in retail-RVA order below.

// AI_mark_danger_zones (dc 0x329f8) is reconstructed below.

// E:\gamedcs\ai_player.cpp:3044
DC_ONLY(0x32a84, 0x3AC)
long markDestinations(Hero* current_hero, long max_distance, SearchArray* search_array, unsigned short* friendly_distances, SearchType search_type)
{
    // @stub
}

// check_holy_grail (dc 0x32e30) is reconstructed below as the source-real
// helper that Complete VC6 expands into find_all_destinations.

// E:\gamedcs\ai_player.cpp:3225
DC_ONLY(0x33038, 0x3CA)
long findAllDestinations(Hero* current_hero, SearchArray* search_array, std::vector<HeroDestination,std::allocator<HeroDestination>* destinations, long max_distance, unsigned char hiring_hero, unsigned char allow_spells, unsigned char explore_mode)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:3390
DC_ONLY(0x33404, 0x450)
void markStrategicMap(Hero* current_hero, long* strategic_map, std::vector<HeroDestination,std::allocator<HeroDestination>* destinations)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:3498
DC_ONLY(0x33854, 0x1F6)
int netValueOfLocation(Hero* current_hero, HeroDestination* destination, long* strategic_map, PathCell* path_cell, SearchArray* search_array)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:3573
DC_ONLY(0x33a4c, 0x2AC)
void unblockLith(Hero* current_hero, HeroDestination* destination, long* best_distance)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:3645
DC_ONLY(0x33cf8, 0x46A)
int aiChooseDestination(Hero* current_hero, long max_distance, HeroDestination* best_point, long* best_raw_value, unsigned char allow_spells, unsigned char explore_mode)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:3813
DC_ONLY(0x34164, 0x90)
void considerHidingMouse(Hero* current_hero, int direction)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:3832
DC_ONLY(0x341f4, 0x1CE)
unsigned char attemptStep(Hero* current_hero, PathCell* path_cell, unsigned char bStandEnd, unsigned char first_step)
{
    // @stub
}

// build_path (dc 0x343c4) is promoted to its retained Complete body at
// VA(0x00430610) below.

// check_move_spell (dc 0x34508) is promoted to its retained Complete body
// at VA(0x004309a0) below.

// attempt_teleport (dc 0x34630) is promoted to its retained Complete body
// at VA(0x00430ab0) below.

// E:\gamedcs\ai_player.cpp:4155
DC_ONLY(0x34a7c, 0x8C)
void checkGatePurchase(type_point point)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:4179
DC_ONLY(0x34b08, 0x4B0)
void aiAttemptMove(Hero* current_hero, HeroDestination* best_point, long* best_raw_value, unsigned char explore_mode)
{
    // @stub
}

// value_of_hiring (dc 0x34fb8) promoted to the VA(0x00431bd0) carcass claim
// in RVA order below - see consider_hiring's anchor pair.

// total_artifact_value (dc 0x35400) promoted to VA(0x004339e0) in RVA order below.

// E:\gamedcs\ai_player.cpp:4476
DC_ONLY(0x354bc, 0x32E)
unsigned char considerHiring(long player_id, Hero* candidate)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:4565
DC_ONLY(0x357ec, 0x9C)
Town* getShipyardTown(const PlayerData* player, long x, long y, long z)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:4583
DC_ONLY(0x35888, 0x86)
unsigned char getMapShipyard(const PlayerData* player, long x, long y, long z)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:4607
DC_ONLY(0x35910, 0xFE)
void aiBuildShip(const Hero* our_hero, long x, long y, long z)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:4643
DC_ONLY(0x35a10, 0xB6)
long aiGetShipCost(const Hero* our_hero, type_point point)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:4670
DC_ONLY(0x35ac8, 0x178)
bool AIPlayer::hireHeroes()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:4728
DC_ONLY(0x35c40, 0x210)
long aiValueOfObservatory(type_point origin, long player_id, long range)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:4930
DC_ONLY(0x35f08, 0x2BE)
void initializeArtifactEffects()
{
    // @stub
}

// ~type_artifact_effect (dc 0x361f4) promoted to VA(0x00432500) below: the
// 7-byte retail body stores the class vtable WITHOUT the mov eax,ecx a VC6
// ctor's this-return always emits, and its one caller is the ICF-folded
// scalar deleting dtor 0x433080 - so the slot is the plain dtor, and the
// default CTOR (dc 0x361c8) is retail-inlined at its construction sites.

// E:\gamedcs\ai_player.cpp:5057
DC_ONLY(0x36214, 0x44)
void ScoutingArtifact::ScoutingArtifact(long new_bonus)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5090
DC_ONLY(0x362e0, 0x40)
void MightArtifact::MightArtifact(long new_bonus)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5109
DC_ONLY(0x36350, 0x40)
void PowerArtifact::PowerArtifact(long new_bonus)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5127
DC_ONLY(0x363b4, 0x3C)
void KnowledgeArtifact::KnowledgeArtifact(long new_bonus)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5145
DC_ONLY(0x36414, 0x3C)
void NecromancyArtifact::NecromancyArtifact(long new_bonus)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5182
DC_ONLY(0x364dc, 0x50)
void MovementArtifact::MovementArtifact(long new_bonus)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5198
DC_ONLY(0x36558, 0x44)
void SpellcasterArtifact::SpellcasterArtifact(long new_bonus)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5219
DC_ONLY(0x365e4, 0x48)
void MoraleArtifact::MoraleArtifact(long new_bonus)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5243
DC_ONLY(0x366d8, 0x48)
void LuckArtifact::LuckArtifact(long new_bonus)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5267
DC_ONLY(0x367cc, 0x48)
void DurationArtifact::DurationArtifact(long new_bonus)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5285
DC_ONLY(0x36838, 0x44)
void SchoolArtifact::SchoolArtifact(SpellSchool new_school, long new_bonus)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5347
DC_ONLY(0x369dc, 0x40)
void AntimagicArtifact::AntimagicArtifact(long _max_level)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5380
DC_ONLY(0x36ab4, 0x48)
void AntimoraleArtifact::AntimoraleArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5406
DC_ONLY(0x36c40, 0x50)
void AntiluckArtifact::AntiluckArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5433
DC_ONLY(0x36dd4, 0x54)
void TomeArtifact::TomeArtifact(SpellSchool new_school)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5477
DC_ONLY(0x36ee8, 0x6C)
void IncomeArtifact::IncomeArtifact(long new_amount, GameResource new_resource)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5495
DC_ONLY(0x36fec, 0x60)
void CreatureGrowthArtifact::CreatureGrowthArtifact(long new_level, long new_bonus)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5505
DC_ONLY(0x3704c, 0x148)
long CreatureGrowthArtifact::getValue(const Hero* owner, unsigned char equipped, unsigned char exact)
{
    // @stub
}

// AI_get_value_of_artifact (dc 0x37194) promoted to VA(0x004336c0) in RVA order below.

// E:\gamedcs\ai_player.cpp:5643
DC_ONLY(0x37464, 0xAE)
long aiGetEquipValue(type_artifact artifact, const Hero* our_hero, unsigned char exact)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5708
DC_ONLY(0x37588, 0x266)
long getFullValue(const Hero* our_hero)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5792
DC_ONLY(0x377f0, 0xA8)
long removeNegativeArtifacts(Hero* our_hero)
{
    // @stub
}

// add_artifact (dc 0x37898) promoted to VA(0x00433e20) in RVA order below.

// E:\gamedcs\ai_player.cpp:5940
DC_ONLY(0x37a58, 0x74)
void aiEquipArtifacts(Hero* our_hero)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5967
DC_ONLY(0x37acc, 0xEE)
void aiSwapArtifacts(Hero* source, Hero* dest)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:6012
DC_ONLY(0x37bbc, 0x7A)
void type_AI_initializer::type_AI_initializer()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:6096
DC_ONLY(0x37c38, 0x36)
void aiInitialize()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:6105
DC_ONLY(0x37c70, 0xBC)
void aiShutDown()
{
    // @stub
}

// E:\gamedcs\struct.h:114
DC_ONLY(0x37d2c, 0x5A)
unsigned char type_point::operator!=(const type_point* arg)
{
    // @stub
}

// E:\gamedcs\artifact.h:229
DC_ONLY(0x37d88, 0x2C)
unsigned char artifactAllowedInSlot(Artifact artifact, ArtifactSlot slot)
{
    // @stub
}

// E:\gamedcs\hero.h:981
DC_ONLY(0x37db4, 0x10)
SkillMastery Hero::getSecondarySkill(SecondarySkill skill)
{
    // @stub
}

// E:\gamedcs\hero.h:991
DC_ONLY(0x37dc4, 0x8)
long Hero::get_value_of_duration()
{
    // @stub
}

// E:\gamedcs\hero.h:996
DC_ONLY(0x37dcc, 0x8)
long Hero::getValueOfKnowledge()
{
    // @stub
}

// E:\gamedcs\hero.h:1001
DC_ONLY(0x37dd4, 0x8)
long Hero::getValueOfPower()
{
    // @stub
}

// E:\gamedcs\hero.h:1016
DC_ONLY(0x37ddc, 0x10)
unsigned char Hero::isInSpellbook(SpellID spell)
{
    // @stub
}

// E:\gamedcs\ai_player.h:263
DC_ONLY(0x37dec, 0x4)
void AIPlayer::init(short new_team)
{
    // @stub
}

// E:\gamedcs\ai_player.h:273
DC_ONLY(0x37df0, 0x6)
void AIPlayer::clear_magus_hut_value()
{
    // @stub
}

// E:\gamedcs\ai_player.h:278
DC_ONLY(0x37df8, 0xE)
double AIPlayer::getResourceValue(GameResource resource)
{
    // @stub
}

// E:\gamedcs\ai_player.h:299
DC_ONLY(0x37e08, 0x16)
void CreatureSource::CreatureSource(CreatureType new_type, short* new_amount, bool _is_free)
{
    // @stub
}

// E:\gamedcs\ai_player.h:313
DC_ONLY(0x37e20, 0x6)
void AICreaturePurchaser::setSubtractMode(unsigned char arg)
{
    // @stub
}

// E:\gamedcs\ai_creature_value.h:29
DC_ONLY(0x37e28, 0xA)
unsigned char CreatureValue::operator<(const CreatureValue* arg)
{
    // @stub
}

// E:\gamedcs\ai_creature_value.h:35
DC_ONLY(0x37e34, 0xA)
unsigned char CreatureValue::operator>(const CreatureValue* arg)
{
    // @stub
}

// E:\gamedcs\ai_spellvalue.h:84
DC_ONLY(0x37e40, 0x8)
unsigned char Spellvalue::canCastSpells()
{
    // @stub
}

// E:\gamedcs\ai_spellvalue.h:114
DC_ONLY(0x37e48, 0x4)
void Spellvalue::setPower(long arg)
{
    // @stub
}

// E:\gamedcs\findpath.h:231
DC_ONLY(0x37e4c, 0x18)
long SearchArray::getVisitedCount()
{
    // @stub
}

// E:\gamedcs\findpath.h:236
DC_ONLY(0x37e64, 0x18)
PathCell* SearchArray::getVisitedCell(long index)
{
    // @stub
}

// E:\gamedcs\findpath.h:247
DC_ONLY(0x37e7c, 0x8)
unsigned char SearchArray::limitWasReached()
{
    // @stub
}

// E:\gamedcs\findpath.h:257
DC_ONLY(0x37e84, 0x12)
void SearchArray::setRectangle(tagRECT* rect)
{
    // @stub
}

// E:\gamedcs\findpath.h:265
DC_ONLY(0x37e98, 0x54)
long* getDangerCell(long* danger_zones, type_point point)
{
    // @stub
}

#endif  // @carcass

VA(0x0042c040, 0x15)  // dc 0x315bc
AICreatureSwapper::AICreatureSwapper()
{
    m_army = 0;
    m_adjacentArmy = 0;
    m_morale = 0;
    m_alignmentCount = 0;
    m_armyValueIncrease = 0;
}

VA(0x0042c060, 0xC3)
void AICreatureSwapper::getAlignments()
{
    m_alignmentCount = m_army->getAlignments(m_alignments);
    if (!m_hasAngelicAlliance) {
        return;
    }
    for (int alignment = 0; alignment < 9; ++alignment) {
        if (m_alignments[alignment + 1] != 0 && m_hasAngelicAlliance) {
            const std::bitset<9>& alliedAlignments = armyGrpFn0044A460();
            if (!alliedAlignments.test(alignment)) {
                continue;
            }
            int other = 0;
            while (!alliedAlignments.test(other)) {
                ++other;
            }
            if (other != alignment) {
                if (m_alignments[other + 1] > 0) {
                    --m_alignmentCount;
                }
                m_alignments[other + 1] += m_alignments[alignment + 1];
                m_alignments[alignment + 1] = 0;
            }
        }
    }
}

VA(0x0042c130, 0x146)  // dc 0x315d8
void AICreatureSwapper::addCreatures(
    CreatureType type, short amount, short slot)
{
    CreatureType oldType = m_army->m_armyTypes[slot];
    m_armyValueIncrease += g_creatureTypeTraits[type].m_aiValue * amount;
    if (oldType != type && oldType != CREATURE_NONE) {
        m_armyValueIncrease -= g_creatureTypeTraits[oldType].m_aiValue
            * static_cast<short>(m_army->m_numTroops[slot]);
        short oldAmount = m_army->m_numTroops[slot];

        ArmyGroup* destination = m_adjacentArmy;
        if (destination && !destination->add(oldType, oldAmount, -1)) {
            long weakestValue = -g_creatureTypeTraits[oldType].m_aiValue
                * oldAmount;
            short weakestSlot = -1;
            for (short candidate = 0;
                 candidate < ArmyGroup::ARMY_GROUP_SLOT_COUNT;
                 ++candidate) {
                long value = -g_creatureTypeTraits[
                    destination->m_armyTypes[candidate]].m_aiValue
                    * destination->m_numTroops[candidate];
                if (value > weakestValue) {
                    weakestValue = value;
                    weakestSlot = candidate;
                }
            }
            if (weakestSlot >= 0) {
                destination->dismiss(weakestSlot);
                destination->add(oldType, oldAmount, weakestSlot);
            }
        }
        m_army->dismiss(slot);
    }
    m_army->add(type, amount, slot);
}

VA(0x0042c280, 0x126)  // dc 0x3166c
long AICreatureSwapper::doBestSwap(bool canTakeAll)
{
    long bestValue = 0;
    short bestArmySlot = -1;
    short bestSourceSlot = -1;
    short bestAmount = 0;
    getAlignments();

    for (short source = 0; source < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++source) {
        CreatureType type = m_adjacentArmy->m_armyTypes[source];
        if (type == CREATURE_NONE)
            continue;
        short count = m_adjacentArmy->m_numTroops[source];
        short slot;
        long value;
        if (canTakeAll) {
            value = valueOfAddingArmy(type, count, slot, false);
        } else {
            value = valueOfAddingArmy(type, count, slot, true);
            if (count > 1) {
                short reducedSlot;
                long reducedValue = valueOfAddingArmy(
                    type, count - 1, reducedSlot, false);
                if (reducedValue > value) {
                    count = count - 1;
                    value = reducedValue;
                    slot = reducedSlot;
                }
            }
        }
        long weighted = m_improvement * value / 40;
        if (weighted > bestValue) {
            bestValue = weighted;
            bestArmySlot = slot;
            bestSourceSlot = source;
            bestAmount = count;
        }
    }

    if (bestValue <= 0)
        return bestValue;

    CreatureType swapType = m_adjacentArmy->m_armyTypes[bestSourceSlot];
    if (static_cast<short>(m_adjacentArmy->m_numTroops[bestSourceSlot])
        == bestAmount)
        m_adjacentArmy->dismiss(bestSourceSlot);
    else
        m_adjacentArmy->m_numTroops[bestSourceSlot] -= bestAmount;
    addCreatures(swapType, bestAmount, bestArmySlot);
    return bestValue;
}

// E:\gamedcs\ai_player.cpp:2171. Dreamcast retains this source helper;
// Complete's /Ob2 folds it into both swap entry points. Retail independently
// proves the two short totals, optional subtraction, and zero floor.
// DC declares an ordinary static helper with const hero receivers; retain it.
// Forced inlining was byte-neutral in both standalone swaps but changed the
// buy_creatures expansion (97.7723 versus 95.2939 without the override).
// That caller residual must be recovered through its natural compiler state.
static short calculateImprovement(
    const Hero* currentHero, const Hero* secondHero)
{
    short improvement =
        currentHero->getPrimarySkillTotal();
    if (secondHero)
        improvement -=
            secondHero->getPrimarySkillTotal();
    if (improvement < 0)
        improvement = 0;
    return improvement;
}

VA(0x0042c3b0, 0xe3)  // dc 0x31808
void AICreatureSwapper::doSwap(Hero* currentHero,
                                       ArmyGroup* sourceArmy,
                                       Hero* secondHero,
                                       unsigned char newHasAngelicAlliance)
{
    m_hasAngelicAlliance = newHasAngelicAlliance;
    m_army = &currentHero->m_army;
    m_adjacentArmy = sourceArmy;
    m_morale = currentHero->getMorale(0, 0, 1);
    m_improvement = calculateImprovement(currentHero, secondHero);
    aiConsolidateArmy(*m_army);
    dumpExtraCreature();
    do {
    } while (doBestSwap(m_adjacentArmy->getNumArmies() > 1) > 0);
    aiArrangeArmy(*m_army);
}

// E:\gamedcs\ai_player.cpp:2209. The two 56-byte locals, six helper
// boundaries, and positive-value loop come from the Dreamcast dossier.
// Complete adds the Angelic-Alliance byte used by the three philai callers;
// retail proves its store at +8 and folds calculate_improvement plus the
// consolidation helper into this selected body.
// Residual (92.5052%): all calls and computations agree. The candidate
// promotes `value` into EDI after the consolidation loop, splitting one exit
// edge (16 blocks versus retail's 15); retail keeps it at [ebp-4]. A volatile
// control forced 15 blocks but changed the surrounding allocation and fell to
// 80.38%, so the source-false qualifier is rejected.
VA(0x0042c4a0, 0x108)  // DC method/locals + Complete parameter, dc 0x31864
long AICreatureSwapper::getSwapValue(
    const Hero* currentHero, const ArmyGroup* sourceArmy,
    const Hero* secondHero, unsigned char newHasAngelicAlliance)
{
    ArmyGroup localArmy(currentHero->m_army);
    ArmyGroup localSource(*sourceArmy);
    long value = 0;

    m_hasAngelicAlliance = newHasAngelicAlliance;
    m_army = &localArmy;
    m_adjacentArmy = &localSource;
    m_morale = currentHero->getMorale(0, 0, 1);
    m_improvement = calculateImprovement(currentHero, secondHero);

    aiConsolidateArmy(localArmy);
    dumpExtraCreature();

    long swapValue;
    do {
        swapValue = doBestSwap(m_adjacentArmy->getNumArmies() > 1);
        value += swapValue;
    } while (swapValue > 0);
    return value;
}

VA(0x0042c5b0, 0xD1)  // dc 0x31924
void AICreatureSwapper::dumpExtraCreature()
{
    if (!m_adjacentArmy
        || m_adjacentArmy->getNumArmies() == ArmyGroup::ARMY_GROUP_SLOT_COUNT
        || m_army->getNumArmies() == 1)
        return;

    for (int slot = 0; slot < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
        CreatureType type = m_army->m_armyTypes[slot];
        if (type != CREATURE_NONE) {
            if (m_adjacentArmy->getNumArmies()
                    == ArmyGroup::ARMY_GROUP_SLOT_COUNT
                || m_army->getNumArmies() == 1)
                return;

            int count = m_army->m_numTroops[slot];
            m_army->dismiss(slot);
            getAlignments();

            short addSlot;
            if (valueOfAddingArmy(type, count, addSlot, false) <= 0) {
                m_adjacentArmy->add(type, count, -1);
                if (m_adjacentArmy->getNumArmies()
                        == ArmyGroup::ARMY_GROUP_SLOT_COUNT
                    || m_army->getNumArmies() == 1)
                    return;
            } else {
                m_army->m_armyTypes[slot] = type;
                m_army->m_numTroops[slot] = count;
            }
        }
    }
}

// DC proves the identity, parameter roles, and shooter_count local. Retail
// adds the Complete elemental-alignment gate and exposes both shooter-policy
// predicates: preserve a sole shooter, but replace a shooter once there are
// already more than three. Alignment checking admits only a singleton group.
// DC's two flags are unsigned char, not bool; retail reads their low bytes.
// Restoring the declared types is byte-flat at 88.3357%, including its caller.
// Residual (88.34%): all policy/filter calls and 135 of retail's 140
// instructions are present across 41 versus 42 blocks. The remaining split
// is a VC6 register-role permutation in the Complete alignment fold;
// `why-reg` measures distance 119. Making the grouped alignment volatile
// improves that internal distance but worsens the real objdiff score to
// 86.33%, and declaration/reference/condition spellings are flat or worse.
VA(0x0042c690, 0x192)  // DC method + retail body/caller; dc 0x31a00
long AICreatureSwapper::chooseWeakestArmy(
    unsigned char isShooter, unsigned char checkAlignments)
{
    long shooterCount = 0;
    int shooterSlot;
    for (shooterSlot = 0;
         shooterSlot < ArmyGroup::ARMY_GROUP_SLOT_COUNT;
         ++shooterSlot) {
        CreatureType type = m_army->m_armyTypes[shooterSlot];
        if (type != CREATURE_NONE
            && (g_creatureTypeTraits[type].m_attributes & g_ctaShooter)) {
            ++shooterCount;
        }
    }

    bool replaceShooter = isShooter && shooterCount > 3;
    bool preserveShooter = !isShooter && shooterCount == 1;
    long weakestSlot = -1;
    long weakestValue = 0;

    int slot;
    for (slot = 0; slot < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
        CreatureType type = m_army->m_armyTypes[slot];
        if (type == CREATURE_NONE)
            continue;

        int groupedAlignment;
        const CreatureTypeTraits& traits = g_creatureTypeTraits[type];
        if (checkAlignments) {
            int alignment;
            if (g_game->m_f1f698 == 0
                && isBaseElemental(type)) {
                alignment = -1;
            } else {
                alignment = g_creatureTypeTraits[type].m_townType;
            }

            groupedAlignment = alignment;
            if (m_hasAngelicAlliance) {
                const std::bitset<9>& alliedAlignments =
                    armyGrpFn0044A460();
                if (alliedAlignments.test(alignment)) {
                    groupedAlignment = 0;
                    while (!alliedAlignments.test(groupedAlignment))
                        ++groupedAlignment;
                }
            }
            if (m_alignments[groupedAlignment + 1] != 1)
                continue;
        }

        if (replaceShooter && !(traits.m_attributes & g_ctaShooter))
            continue;
        if (preserveShooter && (traits.m_attributes & g_ctaShooter))
            continue;

        long value = m_army->m_numTroops[slot] * traits.m_aiValue;
        if (weakestSlot < 0 || value < weakestValue) {
            weakestValue = value;
            weakestSlot = slot;
        }
    }
    return weakestSlot;
}

// DC proves the signature, source-line phases, and both replacement exits.
// Retail adds Complete's elemental and Angelic-Alliance alignment folding.
// A new singleton alignment is rejected when its morale loss outweighs the
// incoming stack; slowing the army also subtracts the proportional movement
// loss before an existing, empty, or weakest slot is selected.
// Residual (89.09%): the 67/68-block and 287/292-instruction forms have the
// same substantive exits, calls, morale census and movement calculation.
// `why-reg` leaves distance 155; its best volatile-value probe moves that
// metric by only five and does not improve objdiff, while the measured
// pointer/reference, declaration and expression variants are flat or worse.
// DC records an unsigned-char must_replace_creature parameter and the
// min(int,int) wrapper at line 2404. Retail likewise copies both the current
// minimum and creature speed to separate temporary homes before choosing
// their addresses. Restoring min instead of the reference-only cppMin
// recovers 89.0856% from 86.1781%; the byte-flag signature is retained too.
// DC lines 2351/2352 place traits and value before the morale locals;
// restoring that declaration order is byte-flat at the recovered peak.
VA(0x0042c830, 0x33F)  // DC method/callgraph + retail Complete body; dc 0x31af4
long AICreatureSwapper::valueOfAddingArmy(
    CreatureType type, short count, short& slot,
    unsigned char mustReplaceCreature)
{
    const CreatureTypeTraits* traits = &g_creatureTypeTraits[type];
    long value = traits->m_aiValue * count;
    bool badMorale = false;
    long moraleArmyValue = 0;

    int alignment;
    if (g_game->m_f1f698 == 0
        && isBaseElemental(type)) {
        alignment = -1;
    } else {
        alignment = traits->m_townType;
    }
    if (m_hasAngelicAlliance) {
        const std::bitset<9>& alliedAlignments = armyGrpFn0044A460();
        if (alliedAlignments.test(alignment)) {
            alignment = 0;
            while (!alliedAlignments.test(alignment))
                ++alignment;
        }
    }

    if (m_alignments[alignment + 1] == 0 && m_army->getNumArmies() > 0) {
        int minimumMorale;
        if (g_game->m_f1f698 == 0
            && isBaseElemental(type)) {
            minimumMorale = 1;
        } else {
            minimumMorale = 2;
            if (traits->m_townType != TOWN_NECROPOLIS)
                minimumMorale = 1;
        }

        if (m_army->getMorale(0, 0, 0, 0, 0,
                           m_hasAngelicAlliance, 0)
                + m_morale < minimumMorale) {
            int index;
            for (index = 0; index < ArmyGroup::ARMY_GROUP_SLOT_COUNT;
                 ++index) {
                CreatureType current = m_army->m_armyTypes[index];
                if (current != CREATURE_NONE
                    && !(g_creatureTypeTraits[current].m_attributes
                         & g_ctaNoMorale)
                    && current != CREATURE_MINOTAUR
                    && current != CREATURE_MINOTAUR_KING) {
                    moraleArmyValue +=
                        g_creatureTypeTraits[current].m_aiValue
                        * m_army->m_numTroops[index];
                }
            }
            if (!(traits->m_attributes & g_ctaNoMorale)
                && type != CREATURE_MINOTAUR
                && type != CREATURE_MINOTAUR_KING) {
                moraleArmyValue += value;
            }
            badMorale = moraleArmyValue >= value * 10;
        }
    }

    int slowestSpeed = 20;
    int index;
    for (index = 0; index < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++index) {
        CreatureType current = m_army->m_armyTypes[index];
        if (current != CREATURE_NONE) {
            slowestSpeed = min(
                slowestSpeed, g_creatureTypeTraits[current].m_speed);
        }
    }
    if (slowestSpeed > traits->m_speed) {
        long oldMove = g_landMovement[slowestSpeed];
        long newMove = g_landMovement[traits->m_speed];
        long armyValue = m_army->getAIValue() + 500;
        value += static_cast<long>(
            static_cast<double>(newMove) * armyValue
            / static_cast<double>(oldMove)
            - static_cast<double>(armyValue));
    }

    slot = -1;
    for (index = 0; index < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++index) {
        if (m_army->m_armyTypes[index] == type) {
            slot = index;
            if (mustReplaceCreature)
                return -1;
            return value;
        }
    }

    if (!badMorale && !mustReplaceCreature
        && (m_army->getNumArmies() < 6 || !m_adjacentArmy)) {
        for (index = 0; index < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++index) {
            if (m_army->m_armyTypes[index] == CREATURE_NONE) {
                slot = index;
                return value;
            }
        }
    }

    slot = chooseWeakestArmy(
        (traits->m_attributes & g_ctaShooter) != 0, badMorale);
    if (slot < 0)
        return 0;
    return value - g_creatureTypeTraits[m_army->m_armyTypes[slot]].m_aiValue
        * m_army->m_numTroops[slot];
}

VA(0x0042cb70, 0x2b9)  // dc 0x31e3c
AICreaturePurchaser::AICreaturePurchaser(
    long player, Generator* currentGenerator)
{
    m_playerId = player;
    m_funds = 0;
    m_subtractCostMode = 1;
    for (short i = 0; i < 4; ++i) {
        CreatureType type = currentGenerator->m_type[i];
        if (type != CREATURE_NONE) {
            m_creatures.push_back(CreatureSource(
                type, &currentGenerator->m_population[i],
                g_creatureTypeTraits[type].m_level == 0));
        }
    }
}

VA(0x0042ce30, 0x114)  // dc 0x31ed4
AICreaturePurchaser::AICreaturePurchaser(
    long player, Town* currentTown)
{
    m_funds = 0;
    m_playerId = player;
    m_subtractCostMode = 1;
    set(currentTown);
}

// E:\\gamedcs\\ai_player.cpp:2495. Both architectures default-construct
// the swapper base and creature vector, initialize the same scalar tail, and
// push exactly one source carrying the supplied creature, amount and bool.
VA(0x0042cf50, 0x25a)  // dc 0x31f24
AICreaturePurchaser::AICreaturePurchaser(
    long player, CreatureType type, short* amount, bool isFree)
{
    m_playerId = player;
    m_funds = 0;
    m_subtractCostMode = 1;
    m_creatures.push_back(CreatureSource(type, amount, isFree));
}

VA(0x0042d1b0, 0x268)  // dc 0x31f94
void AICreaturePurchaser::set(Town* currentTown)
{
    m_creatures.clear();
    int dwelling = 0;
    int remaining = 14;
    short* population = currentTown->m_population;
    for (; remaining; ++dwelling, ++population, --remaining) {
        CreatureType type = g_townDwellingCreatures[
            currentTown->m_type * 14 + dwelling];
        short amount = *population;
        if (amount > 0) {
            m_creatures.push_back(CreatureSource(type, population, 0));
        }
    }
}

void AICreaturePurchaser::set(CreatureType newType,
                                     short* newAmount)
{
    m_creatures.clear();
    m_creatures.push_back(
        CreatureSource(newType, newAmount, false));
}

// DC proves the method, signature, and the add_creatures/value_of_adding_army
// edges. Retail proves the Complete purchaser tail: two independent cost
// arrays, optional resource trading, a seven-resource affordability cap, and
// the three-quarter cap on the cost penalty that selects the best source.
// DC's parameter is an unsigned char, not C++ bool. Residual (97.27%): 217
// of 219 instructions agree; the remaining delta is VC6 stack-slot coloring
// around the best-source state (`why-reg` distance 70), after equivalent
// declaration and expression orders were exhausted.
// Census 2026-09-04: base 217 vs retail 219; the two surplus retail rows are
// a dword copy of best_number out of its recycled [ebp+8] parameter home into
// a fresh [ebp-0x8] slot, which frees [ebp+8] to carry the resource loop's
// counter (we keep best_number in [ebp+8] and colour that counter onto
// [ebp-0x1c], the first loop's slot). Both spellings that name the copy in
// source - a fresh `short count` from MaxBuyableCreatures, and `short count =
// best_number` after the assignment - are byte-flat at 97.2740, so the extra
// slot is the allocator's, not a source local.
VA(0x0042d420, 0x264)  // DC method/callgraph + exact retail caller; dc 0x32038
long AICreaturePurchaser::doBestPurchase(
    unsigned char tradeAllowed)
{
    int resourceCost[7];
    short sourceIndex;
    int bestValue = 0;
    short bestNumber = 0;
    short bestSource = -1;
    short bestSlot = -1;

    getAlignments();
    for (sourceIndex = 0; sourceIndex < m_creatures.size(); ++sourceIndex) {
        CreatureType type = m_creatures[sourceIndex].m_type;
        short available = m_creatures[sourceIndex].m_number;
        if (available > 0) {
            long number;
            if (m_creatures[sourceIndex].m_isFree) {
                number = available;
            } else {
                getMonsterCost(type, resourceCost);
                if (tradeAllowed)
                    g_aiPlayers[m_playerId].tradeResources(
                        resourceCost, m_creatures[sourceIndex].m_number);
                number = maxBuyableCreatures(
                    m_funds, type, m_creatures[sourceIndex].m_number);
            }

            if (number > 0) {
                short slot;
                long value = valueOfAddingArmy(
                    type, number, slot, false);
                if (value > 0) {
                    if (m_subtractCostMode
                        && !m_creatures[sourceIndex].m_isFree) {
                        long costValue = aiResourceCost(
                            m_playerId, resourceCost) * number;
                        if (costValue > value * 3 / 4)
                            costValue = value * 3 / 4;
                        value -= costValue;
                    }

                    if (value > bestValue) {
                        bestSource = sourceIndex;
                        bestValue = value;
                        bestSlot = slot;
                        bestNumber = number;
                    }
                }
            }
        }
    }

    if (bestValue > 0) {
        CreatureType type = m_creatures[bestSource].m_type;
        addCreatures(type, bestNumber, bestSlot);
        if (!m_creatures[bestSource].m_isFree) {
            getMonsterCost(type, resourceCost);
            bestNumber = maxBuyableCreatures(
                m_funds, type, m_creatures[bestSource].m_number);
            for (short resource = 0; resource < 7; ++resource)
                m_funds[resource] -= resourceCost[resource] * bestNumber;
            m_creatures[bestSource].m_number -= bestNumber;
        }
    }
    return bestValue;
}

VA(0x0042d690, 0xE1)  // dc 0x32288
void AICreaturePurchaser::doPurchase(
    ArmyGroup* newArmy, short newMorale, ArmyGroup* newAdjacentArmy,
    long* newFunds, unsigned char allowTrade,
    unsigned char newHasAngelicAlliance)
{
    HOMM3_RELEASE_VERIFY(newArmy != 0);
    HOMM3_RELEASE_VERIFY(newFunds != 0);
    m_army = newArmy;
    m_adjacentArmy = newAdjacentArmy;
    m_morale = newMorale;
    m_funds = newFunds;
    m_hasAngelicAlliance = newHasAngelicAlliance;

    aiConsolidateArmy(*newArmy);

    dumpExtraCreature();
    do {
    } while (doBestPurchase(allowTrade) > 0);

    for (short source = 0; source < m_creatures.size(); ++source)
        *m_creatures[source].m_ptr = m_creatures[source].m_number;
}

// Complete adds the final Angelic-Alliance byte to the DC signature. Retail
// copies both armies and all seven resources, so the valuation can run the
// real purchaser loop without mutating any caller-owned state. The adjacent
// local is constructed even when the optional source pointer is null.
// Residual (97.33%): all 13 blocks, all 86 instructions, every call and the
// flow graph agree. Only two caller-saved allocation sites differ inside the
// inlined consolidation walk (EAX/ECX versus ECX/EDX, then ECX versus EDX).
// `why-reg` measures distance 12; both model and guided sweeps, declaration
// lifetime/order probes, direct-loop expansion and alternate call operands
// either leave it unchanged or worsen it.
VA(0x0042d780, 0xEF)  // DC method/locals + retail Complete tail; dc 0x322f8
long AICreaturePurchaser::getPurchaseValue(
    const ArmyGroup* newArmy, short newMorale,
    const ArmyGroup* newAdjacentArmy, const long* newFunds,
    unsigned char newHasAngelicAlliance)
{
    ArmyGroup localArmy(*newArmy);
    ArmyGroup localAdjacentArmy;
    long localFunds[7];
    memcpy(localFunds, newFunds, sizeof localFunds);
    long value = 0;

    m_army = &localArmy;
    m_morale = newMorale;
    m_funds = localFunds;
    m_hasAngelicAlliance = newHasAngelicAlliance;

    ArmyGroup* localAdjacent = 0;
    if (newAdjacentArmy) {
        localAdjacentArmy = *newAdjacentArmy;
        localAdjacent = &localAdjacentArmy;
    }
    m_adjacentArmy = localAdjacent;

    aiConsolidateArmy(*m_army);
    dumpExtraCreature();
    long purchase;
    do {
        purchase = doBestPurchase(false);
        value += purchase;
    } while (purchase > 0);
    return value;
}

VA(0x0042d870, 0x67)  // dc 0x323bc
void aiConsolidateArmy(ArmyGroup& currentArmy)
{
    for (int first = 0; first < ArmyGroup::ARMY_GROUP_SLOT_COUNT - 1;
         ++first) {
        CreatureType type = currentArmy.m_armyTypes[first];
        if (type != CREATURE_NONE) {
            for (int duplicate = first + 1;
                duplicate < ArmyGroup::ARMY_GROUP_SLOT_COUNT;
                ++duplicate) {
                if (currentArmy.m_armyTypes[duplicate] == type) {
                    currentArmy.m_numTroops[first] +=
                        currentArmy.m_numTroops[duplicate];
                    currentArmy.dismiss(duplicate);
                }
            }
        }
    }
}

VA(0x0042d8e0, 0x239)  // dc 0x32430
void aiArrangeArmy(ArmyGroup& currentArmy)
{
    std::vector<CreatureValue> values;
    CreatureValue entry;
    for (int i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
        CreatureType type = currentArmy.m_armyTypes[i];
        if (type != CREATURE_NONE) {
            entry.m_type = type;
            entry.m_amount = static_cast<short>(currentArmy.m_numTroops[i]);
            entry.m_value = g_creatureTypeTraits[type].m_speed;
            values.push_back(entry);
            currentArmy.dismiss(i);
        }
    }
    std::sort(values.begin(), values.end());

    int slot = 0;
    for (int shooter = static_cast<int>(values.size()) - 1; shooter >= 0;
         --shooter) {
        entry = values[shooter];
        if (g_creatureTypeTraits[entry.m_type].m_attributes & g_ctaShooter) {
            currentArmy.add(entry.m_type, entry.m_amount, slot);
            slot += 2;
            if (slot >= ArmyGroup::ARMY_GROUP_SLOT_COUNT)
                slot = 1;
        }
    }

    int freeSlot = 0;
    for (unsigned int walker = 0; walker < values.size(); ++walker) {
        entry = values[walker];
        if (!(g_creatureTypeTraits[entry.m_type].m_attributes & g_ctaShooter)) {
            while (currentArmy.m_armyTypes[freeSlot] != CREATURE_NONE)
                ++freeSlot;
            currentArmy.add(entry.m_type, entry.m_amount, freeSlot);
        }
    }
}

long splitArmy(ArmyGroup* currentArmy, short index, short limit,
                short openSlots);

// E:\gamedcs\ai_player.cpp:2817
// Dreamcast proves the const armyGroup& enemy parameter, armyGroup&
// current_army local, and long walker_count local. Retail's consolidation
// prefix is the ordinary aiConsolidateArmy body expanded at this call site;
// the canonical helper naturally inlines here, so no copied merge loop is
// needed. Its retained body and other callers keep their own decisions.
// The enemy counters precede the combat-value calls in DC lines 2832-2846
// and retail. The first split loop exits immediately when no slots remain.
// Recovering that order and exit reaches 86.50% from the old 83.62%.

// Retail adds army arrangement to DC's early exits, sharing one exit block
// for the guards and retaining a separate ordinary completion call. A scoped
// do/while(0) calculation removes all six gotos and improves 97.7143% to
// 98.8238%. Each split loop breaks on exhausted slots, then propagates that
// real result to the enclosing calculation. Keeping the ordinary completion
// call separate preserves retail's two arrangement paths.
// Negative controls: one unconditional arrangement after the calculation
// scores 94.3714%; nested positive guards remove four gotos but stay at
// 97.7143%. Per-guard arrange/return copies previously scored 76.55%.
// Residual: consolidation reload scheduling and final split-loop homing;
// keep the canonical aiConsolidateArmy boundary and reference local.
VA(0x0042db20, 0x249)  // retail callee set + arity, dc 0x32670
void splitArmies(Hero* currentHero, const Hero* enemyHero,
                  const ArmyGroup& enemy)
{
    ArmyGroup& currentArmy = currentHero->m_army;
    aiConsolidateArmy(currentArmy);

    do {
        int openSlots = 7 - currentArmy.getNumArmies();
        if (openSlots <= 0) {
            break;
        }
        int enemyShooterCount = 0;
        int enemyShooterValue = 0;
        int enemyMaxValue = 0;
        float ratio;
        if (enemyHero == 0)
            ratio = 1.0f;
        else
            ratio = const_cast<Hero*>(enemyHero)
                        ->getCombatValueModifier();
        ratio /= currentHero->getCombatValueModifier();

        int k;
        for (k = 0; k < 7; ++k) {
            CreatureType type = enemy.m_armyTypes[k];
            if (type == CREATURE_NONE)
                continue;
            long value = static_cast<long>(
                enemy.m_numTroops[k] * g_creatureTypeTraits[type].m_aiValue
                * ratio);
            if (g_creatureTypeTraits[type].m_attributes & g_ctaShooter) {
                ++enemyShooterCount;
                enemyShooterValue += value;
            }
            if (value > enemyMaxValue)
                enemyMaxValue = value;
        }

        int slot;
        for (slot = 0; slot < 7; ++slot) {
            CreatureType type = currentArmy.m_armyTypes[slot];
            if (type != CREATURE_NONE
                && (g_creatureTypeTraits[type].m_attributes & g_ctaShooter)) {
                openSlots -= splitArmy(&currentArmy, slot, enemyMaxValue * 5,
                                         openSlots);
                if (openSlots == 0) {
                    break;
                }
            }
        }

        if (openSlots == 0)
            break;
        if (enemyShooterCount == 0) {
            break;
        }
        long heroShooterValue = 0;
        long walkerCount = 0;
        int m;
        for (m = 0; m < 7; ++m) {
            CreatureType type = currentArmy.m_armyTypes[m];
            if (type == CREATURE_NONE)
                continue;
            if (g_creatureTypeTraits[type].m_attributes & g_ctaShooter)
                heroShooterValue += currentArmy.m_numTroops[m]
                    * g_creatureTypeTraits[type].m_aiValue;
            else
                ++walkerCount;
        }

        if (heroShooterValue >= enemyShooterValue) {
            break;
        }
        int splitsNeeded = (enemyShooterCount
            - heroShooterValue * enemyShooterCount
                / enemyShooterValue
            + 1) / 2 - walkerCount;
        if (splitsNeeded <= 0) {
            break;
        }
        if (splitsNeeded < openSlots)
            openSlots = splitsNeeded;
        for (slot = 0; slot < 7; ++slot) {
            CreatureType type = currentArmy.m_armyTypes[slot];
            if (type == CREATURE_NONE)
                continue;
            if (g_creatureTypeTraits[type].m_attributes & g_ctaShooter)
                continue;
            openSlots -= splitArmy(&currentArmy, slot, enemyMaxValue,
                                   openSlots);
            if (openSlots == 0)
                break;
        }
        if (openSlots == 0)
            break;
        aiArrangeArmy(currentArmy);
        return;
    } while (0);
    aiArrangeArmy(currentArmy);
}

VA(0x0042dd70, 0xdc)  // dc 0x325bc
long splitArmy(ArmyGroup* currentArmy, short index, short limit,
                short openSlots)
{
    CreatureType type = currentArmy->m_armyTypes[index];
    int pieces = g_creatureTypeTraits[type].m_aiValue
        * currentArmy->m_numTroops[index] / limit;
    if (pieces > openSlots + 1)
        pieces = openSlots + 1;
    if (pieces > currentArmy->m_numTroops[index])
        pieces = currentArmy->m_numTroops[index];
    if (pieces <= 1)
        return 0;
    int remaining = pieces;
    for (int slot = 0; slot < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
        if (currentArmy->m_armyTypes[slot] == CREATURE_NONE) {
            long per = currentArmy->m_numTroops[index] / remaining;
            currentArmy->add(type, per, slot);
            currentArmy->m_numTroops[index] -= per;
            --remaining;
            if (remaining <= 1)
                return pieces - 1;
        }
    }
    return pieces - 1;
}

// E:\gamedcs\ai_player.cpp:2975, dc 0x32894.
// CodeView makes mark_danger_zones a static three-argument helper and names
// GetMobility, GetLocation, SeedPosition and the visited-cell accessors.
// Retail expands this helper inside AI_mark_danger_zones at 0x42de50;
// the outer player/hero census belongs to that two-argument wrapper.
static void markDangerZones(const Hero* ourHero, Hero* enemyHero,
                            long* dangerZones)
{
    long value = aiValueOfCombat(
        ourHero, enemyHero, enemyHero->m_army, 0, 0);
    if (value < 0) {
        int mobility = enemyHero->getMobility() + 300;
        checkDoMain(0, 0);
        type_point start = enemyHero->getLocation();
        type_point target(-1, -1, -1);
        g_searchArray->seedPosition(
            enemyHero, start, target, mobility,
            (enemyHero->m_flags >> 18) & 1,
            const_AI_enemy_search, mobility, 0);

        for (long visitedIndex =
                 g_searchArray->getVisitedCount();
             visitedIndex--;) {
            const type_point& point = g_searchArray->getVisitedCell(visitedIndex)->m_point;
            if (value >= -500000000) {
                *getDangerCell(dangerZones, point) += value;
            } else {
                *getDangerCell(dangerZones, point) =
                    -1000000000;
            }
        }
    }
}

// E:\gamedcs\ai_player.cpp:3013, dc 0x329f8
VA(0x0042de50, 0x25c)  // outer census + MoveHero caller, dc 0x329f8
void aiMarkDangerZones(Hero* currentHero, long* dangerZones)
{
    for (int playerId = 0; playerId < 8; ++playerId) {
        const PlayerData& player = g_game->m_players[playerId];
        if (!g_game->onSameTeam(playerId, currentHero->m_owner)
            && !g_game->m_playerDisabled[playerId]) {
            for (int heroIndex = 0; heroIndex < player.m_numHeroes;
                 ++heroIndex) {
                Hero* enemyHero = g_game->getHero(player.m_heroes[heroIndex]);
                markDangerZones(currentHero, enemyHero, dangerZones);
            }
        }
    }
}

// The three helpers below are source-visible in the Dreamcast line table but
// folded into AI_choose_destination by retail VC6.  Their boundaries are
// reconstruction facts: keeping them here preserves the same helper calls
// without manufacturing retail-only out-of-line slots.

// E:\gamedcs\ai_player.cpp:3164, dc 0x32e30. Dreamcast preserves this
// static helper boundary, vector-reference parameter and local `point`;
// Complete adds the build-grail value change but expands the helper into
// its sole caller. DC lines 3181/3205 call game::get_cell and construct the
// typed artifact temporary for AI_get_value_of_artifact. Restoring those
// boundaries plus the caller's two GetMapExtra(point) uses naturally retains
// both retail calls (findAllDestinations +0x694/+0x73f), without either old
// depth pin. Before the point-overload recovery, direct-map/no-pin measured
// 93.7016%, and the typed temporary/no-artifact-pin measured 61.8460%.
static void checkHolyGrail(
    const Hero* currentHero, const SearchArray* currentSearchArray,
    std::vector<HeroDestination>& destinations,
    const unsigned short* friendlyDistances)
{
    PlayerData* player = &g_game->m_players[currentHero->m_owner];
    if (player->m_puzzleGuess.m_x >= 0) {
        HeroDestination point;
        point.m_point.m_x = player->m_puzzleGuess.m_x;
        point.m_point.m_y = player->m_puzzleGuess.m_y;
        point.m_point.m_z = player->m_puzzleGuess.m_z;
        point.m_isCritical = 0;
        PathCell* guessCell = currentSearchArray->getCell(point.m_point, 0);
        if (guessCell->m_visited) {
            NewmapCell* mapCell = g_game->getCell(point.m_point);
            if (!(mapCell->m_type == HERO && mapCell->m_isTrigger)
                || mapCell->m_extraInfo
                    == static_cast<unsigned long>(currentHero->m_id)) {
                if (currentHero->isInPatrolRadius(
                        point.m_point)) {
                    point.m_moveCost = guessCell->m_cost;
                    unsigned short friendlyCost = friendlyDistances[
                        (point.m_point.m_z * g_mapHeight
                         + point.m_point.m_y)
                            * g_mapWidth
                        + point.m_point.m_x];
                    if (point.m_moveCost <= friendlyCost) {
                        if (g_game->m_mapHeader.m_victoryCondition.m_type
                            == VICTORY_CONDITION_BUILD_GRAIL) {
                            point.m_value = 1968;
                        } else {
                            point.m_value = aiGetValueOfArtifact(
                                type_artifact(ARTIFACT_HOLY_GRAIL),
                                currentHero->m_owner);
                        }
                        point.m_moveCost = max(
                            point.m_moveCost,
                            currentHero->getMobility()
                                + currentHero->m_movePoints);
                        if (point.m_value > 0)
                            destinations.push_back(point);
                    }
                }
            }
        }
    }
}

// E:\gamedcs\ai_player.cpp:3390
static void markStrategicMap(
    Hero* currentHero, long* strategicMap,
    std::vector<HeroDestination>& destinations)
{
    tagRECT rect;
    SearchArray currentSearchArray;
    HeroDestination point;
    short topX;
    unsigned char wasTrigger;
    short topY;
    type_point pt;
    long levelSize = g_mapWidth * g_mapHeight;

    for (short i = 0; i < destinations.size(); ++i) {
        point = destinations[i];
        NewmapCell* cell = g_advManager->getCell(point.m_point);
        int type = cell->m_type;
        if (!(getMapExtra(point.m_point.m_x, point.m_point.m_y, point.m_point.m_z)
              & g_unnamed69ccc4)) {
            strategicMap[point.m_point.m_z * levelSize
                          + point.m_point.m_y * g_mapWidth + point.m_point.m_x]
                += point.m_value;
            continue;
        }

        wasTrigger = cell->m_isTrigger;
        if (g_adventureObjectTraits[type][0])
            cell->m_isTrigger = 0;

        rect.left = max(0L, static_cast<long>(point.m_point.m_x) - 5);
        rect.top = max(0L, static_cast<long>(point.m_point.m_y) - 5);
        rect.right = min(static_cast<long>(point.m_point.m_x) + 6, g_mapWidth);
        rect.bottom = min(static_cast<long>(point.m_point.m_y) + 6, g_mapHeight);
        currentSearchArray.setRectangle(rect);
        g_advManager->m_advWindow->animateBottomView(0);
        currentSearchArray.seedPosition(
            currentHero, point.m_point, type_point(-1, -1, -1), 500,
            cell->m_groundSet == eTerrainWater, const_AI_treasure_search,
            59999, 0);

        short nearbyCost;
        if (!g_adventureObjectTraits[type][0]) {
            nearbyCost = 0;
        } else {
            cell->m_isTrigger = wasTrigger;
            topX = max(0L, static_cast<long>(point.m_point.m_x) - 1);
            topY = max(0L, static_cast<long>(point.m_point.m_y) - 1);
            short stopX = min(static_cast<long>(point.m_point.m_x) + 2,
                               g_mapWidth);
            short stopY = min(static_cast<long>(point.m_point.m_y) + 2,
                               g_mapHeight);
            pt.m_z = point.m_point.m_z;
            nearbyCost = 0;
            for (pt.m_x = topX; pt.m_x < stopX; ++pt.m_x) {
                for (pt.m_y = topY; pt.m_y < stopY; ++pt.m_y) {
                    PathCell* nearby = currentSearchArray.getCell(pt, false);
                    if (nearby->m_cost > nearbyCost)
                        nearbyCost = nearby->m_cost;
                }
            }
        }

        for (long j = currentSearchArray.getVisitedCount(); j-- != 0;) {
            PathCell* visited = currentSearchArray.getVisitedCell(j);
            long value;
            if (visited->m_cost <= nearbyCost) {
                value = point.m_value;
            } else {
                value = point.m_value * 300
                    / (visited->m_cost - nearbyCost + 300);
            }
            strategicMap[visited->m_point.m_z * levelSize
                          + visited->m_point.m_y * g_mapWidth + visited->m_point.m_x]
                += value;
        }
    }
}

// E:\gamedcs\ai_player.cpp:3573
static void unblockLith(Hero* currentHero,
                                       HeroDestination& destination,
                                       long& bestDistance)
{
    // Complete snapshots the underlying byte directly (`mov cl,[hero+6] /
    // mov [ebp+0x17],cl` at 0x42e3f0); `is_on_map()`'s bool facade
    // normalizes it through `setne` and cannot produce that pair. Same
    // later-revision spelling search.cpp:581 already carries.
    unsigned char wasOnMap = currentHero->isOnMap();
    currentHero->restoreCell();
    NewmapCell* cell =
        g_game->getCell(currentHero->getLocation());

    if (!cell->m_isTrigger) {
        if (wasOnMap)
            currentHero->obscureCell();
        return;
    }
    if (cell->m_type == TOWN) {
        Town* currentTown = g_game->getTown(cell->m_extraInfo);
        if (currentTown->m_owner == currentHero->m_owner) {
            if (wasOnMap)
                currentHero->obscureCell();
            return;
        }
    }
    if (cell->m_type == SANCTUARY) {
        if (wasOnMap)
            currentHero->obscureCell();
        return;
    }

    type_point point;
    long closest = 0;
    point.m_z = currentHero->m_z;
    for (long direction = 0; direction < 8; ++direction) {
        point.m_x = currentHero->m_x + g_normalDirTable[direction].m_x;
        point.m_y = currentHero->m_y + g_normalDirTable[direction].m_y;
        if (!point.isValid())
            continue;
        if (g_game->m_worldMap.cell(
                point.m_x, point.m_y, point.m_z)->m_isTrigger)
            continue;
        if (getMapExtra(point.m_x, point.m_y, point.m_z) & MAP_EXTRA_MONSTER)
            continue;
        PathCell* currentPathCell =
            g_searchArray->getCell(point, false);
        if (!currentPathCell->m_visited)
            continue;
        if (destination.m_point.m_x >= 0 && closest <= currentPathCell->m_cost)
            continue;
        if (g_searchArray->getDangerValue(point) < 0)
            continue;
        closest = currentPathCell->m_cost;
        destination.m_point = point;
        destination.m_isCritical = 0;
        bestDistance = 0;
    }
    if (wasOnMap)
        currentHero->obscureCell();
}

// E:\gamedcs\ai_player.cpp:3645.  The DC decorated signature proves both
// output operands are references; move_hero's retail /Gr call passes their
// addresses in the same argument positions.  Retail corroborates the full
// helper/statement skeleton and folds mark_strategic_map plus unblock_lith.
// MAX 82.9640%: ordinary static helpers, the original game::getCell call,
// and searchArray's tagRECT assignment replace forced inlining and the
// eight statement pins. With four separate bounds assignments, ordinary
// helpers score 77.53057%, adding the game accessor 78.54367%, and removing
// all pins is byte-identical. Restoring tagRECT raises that form to 82.96397%;
// header and out-of-class aggregate assignments emit identical bytes.
// Keep cppMin/cppMax arguments by reference: their old by-value arguments
// produced dangling result references. This fix is byte-flat here.
// The source schedule still requires markStrategicMap to read cell->type
// before GetMapExtra, and Complete snapshots m_valid directly in unblockLith.
// Residual: the initial unblockLith cleanup paths still merge. A combined
// town-owner condition, a scope ending the initial cell before the direction
// loop, and both together are byte-flat. No corresponding source rewrite
// has yet recovered retail's fourth obscureCell call.
VA(0x0042e0b0, 0xb6e)  // anchor-caller move_hero + order bracket, dc 0x33cf8
int aiChooseDestination(Hero* currentHero, long maxDistance,
                          HeroDestination& bestPoint,
                          long& bestRawValue,
                          unsigned char allowSpells,
                          unsigned char exploreMode)
{
    long rawValue;
    long nearbyCost;
    unsigned char noTowns;
    short i;
    std::vector<HeroDestination> destinations(0);
    type_point start;
    HeroDestination point;
    long bestDistance;

    long mapCells = g_game->getNumMapLevels() * g_mapWidth * g_mapHeight;
    rawValue = findAllDestinations(currentHero, g_searchArray,
                                      destinations, maxDistance, 0,
                                      allowSpells, exploreMode);
    long* strategicMap = new long[mapCells];
    memset(strategicMap, 0, mapCells * sizeof(long));
    markStrategicMap(currentHero, strategicMap, destinations);

    start = currentHero->getLocation();
    nearbyCost = currentHero->m_maxMovePoints * 21 / 100;
    bestDistance = 0x7fff;
    noTowns = 0;
    // Retail forms the player record from the global index here, not from
    // the cached pointer: 0x42e397 loads gpGame and reads
    // [eax + 8*edx + 0x20b0e] with edx = 45 * gNetLocalGamePos, i.e.
    // players[gNetLocalGamePos].numTowns at the 360-byte stride.
    if (!g_game->m_players[g_netLocalGamePos].m_numTowns)
        noTowns = 1;
    bestPoint.m_isCritical = 0;

    if (bestPoint.m_point.m_x < 0) {
        bestPoint.m_isNearby = 0;
        if (rawValue < 0) {
            bestPoint.m_value = 0;
        } else {
            bestPoint.m_value = 1;
            if (!g_searchArray->limitWasReached())
                unblockLith(currentHero, bestPoint, bestDistance);
        }
    } else {
        bestDistance = bestPoint.m_moveCost;
        rawValue = bestRawValue;
        if (bestDistance > 100)
            rawValue = rawValue * 100 / bestDistance;
        PathCell* bestCell = g_searchArray->getCell(bestPoint.m_point,
                                                      false);
        bestPoint.m_isNearby =
            bestCell->m_lastPoint == start
            || bestCell->m_cost <= nearbyCost
            || bestPoint.m_moveCost <= nearbyCost;
        if (bestCell->m_adjustedCost > bestCell->m_cost)
            bestPoint.m_isNearby = 0;
        if (rawValue < 0 || !noTowns)
            bestPoint.m_isNearby = 0;
    }

    for (i = 0; i < destinations.size(); ++i) {
        point = destinations[i];
        if (bestPoint.m_value > 0 && point.m_value == 0)
            continue;

        PathCell* currentPathCell = g_searchArray->getCell(point.m_point, false);
        unsigned char isNearby = 0;
        if (currentPathCell->m_lastPoint == start
            && g_game->getCell(currentPathCell->m_lastPoint)->m_isTrigger) {
            isNearby = 1;
        }
        if (currentPathCell->m_cost <= nearbyCost
            || point.m_moveCost <= nearbyCost)
            isNearby = 1;
        if (currentPathCell->m_adjustedCost > currentPathCell->m_cost || !noTowns)
            isNearby = 0;

        long candidateRaw = netValueOfLocation(
            currentHero, &point, strategicMap, currentPathCell, g_searchArray);
        long value = candidateRaw;
        if (candidateRaw > 0) {
            if (point.m_moveCost > 100)
                value = candidateRaw * 100 / point.m_moveCost;
            if (value < 1)
                value = 1;
        } else if (candidateRaw < 0) {
            isNearby = 0;
        }
        g_advManager->m_advWindow->animateBottomView(0);

        if (point.m_value == 0 && value <= rawValue)
            continue;
        if (!point.m_isCritical
            && ((bestPoint.m_value == 0 && point.m_value == 0)
                || (bestPoint.m_value > 0 && point.m_value > 0))) {
            if (!bestPoint.m_isNearby) {
                if (!isNearby && value <= rawValue)
                    continue;
            } else {
                if (!isNearby)
                    continue;
                if (bestDistance < currentPathCell->m_cost)
                    continue;
                if (bestDistance == currentPathCell->m_cost && value <= rawValue)
                    continue;
            }
        }

        bestPoint = point;
        bestDistance = currentPathCell->m_cost;
        bestRawValue = candidateRaw;
        bestPoint.m_isNearby = isNearby;
        rawValue = value;
        if (bestPoint.m_isCritical)
            break;
    }

    if (bestDistance > currentHero->m_movePoints)
        bestDistance -= currentHero->m_movePoints;
    else
        bestDistance = 0;
    currentHero->m_pathTargetX = bestPoint.m_point.m_x;
    currentHero->m_pathTargetY = bestPoint.m_point.m_y;
    currentHero->m_pathTargetZ = bestPoint.m_point.m_z;
    currentHero->m_targetIsCritical = bestPoint.m_isCritical;
    currentHero->m_targetDistance = static_cast<short>(bestDistance);
    delete[] strategicMap;
    return rawValue;
}

#if 0  // @carcass

// E:\gamedcs\Town.h:299
DC_ONLY(0x37f50, 0x12)
__int64 Town::getBuildingMask()
{
    // @stub
}

// E:\gamedcs\game.h:839
DC_ONLY(0x37f64, 0x58)
unsigned char Game::isHumanTeam(int teamNum)
{
    // @stub
}

// E:\gamedcs\game.h:865
DC_ONLY(0x37fbc, 0x1C)
int Game::getTeam(int playerNum)
{
    // @stub
}

// is_human_ally (dc 0x37fd8, game.h:1370) is claimed at its retail COMDAT
// slot below (0x42b9e0); the callers additionally expand GetTeam at the
// call site before handing it the team.

#endif  // @carcass

// The nine functions below are located by the callee-fingerprint join against
// evidence/dc-xref-graph.tsv: for each retail carve row the cross-unit resolved
// calls (homm3 sema disasm) form a set that matches a unique ai_player DC
// callee-set through the RVA scramble. Reciprocal-best pairs; sizes carve-exact
// from config/retail-functions.tsv; claimed @stub in RVA order (ORDER gate).
//   0x2edd0 find_all_destinations - 7 shared (game::GetTownId, CheckDoMain,
//           AI_value_of_event, hero::is_in_patrol_radius, ...), marginR=12.
//   0x2f570 mark_destinations - searchArray ctor/dtor + is_in_patrol_radius +
//           type_point::is_valid (6 shared), marginD=7, r=1.10.
//   0x2f980 net_value_of_location - Random + advManager::FindAdjacentMonster
//           are each unique to this fn in both dc-xref and retail (proof).
//   0x2fc50 attempt_step - NewmapCell::cell_is_trigger, ::get_map_object and
//           hero::can_summon_boat are all unique-callee proofs.
//   0x30f80 AI_build_ship - game::CreateBoat + hero::belongs_to_human unique.
//   0x31800 consider_hiring - town::hire unique + 5 more (searchArray, town::*).
//   0x336c0 AI_get_value_of_artifact - hero::GetFirstAidFactor unique.
//   0x33c60 get_full_value - armyGroup::GetArmyLuck/GetArmyMorale and
//           type_spellvalue::get_best_spell_value all unique to this fn.
//   0x33fe0 AI_swap_artifacts - 4 backpack callees (equip/remove_artifact,
//           get_last_backpack_index, remove_backpack_artifact), r=1.03.
// E:\gamedcs\ai_player.cpp:3225
// Builds the candidate-destination list: every visited search cell that is
// a trigger (or an unexplored tile while the player still has towns) inside
// the patrol radius becomes a HeroDestination - valued 100/100000 for
// exploration or by AI_value_of_event, cost-adjusted, with the escort
// classes gated on the friendly-distance map. Inside a town only enemy
// heroes within this turn's movement qualify (is_critical). The tail adds
// the player's puzzle-guess grail spot, valued 1968 under the build-grail
// victory or as the Holy Grail artifact otherwise. Returns the danger under
// the hero (mark_destinations' result).
// Residual (96.365%, 80/81 blocks, 54/54 branches): Complete retains the
// first of three nested vector<HeroDestination>::size calls in the loop's
// push_back growth expression; this compile retains the latter two only.
// Dreamcast proves push_back itself, so do not replace it with insert or add
// caller mass. Recovered locals, helper boundaries, condition groups and the
// unnamed get_location temporary are retained through the expected dips.
// Forward prototypes: all three bodies sit later in RVA order.
long markDestinations(Hero* currentHero, long maxDistance,
                       SearchArray* currentSearchArray,
                       unsigned short* friendlyDistances,
                       SearchType searchType);
long aiValueOfEvent(const Hero* currentHero, type_point point,
                       long& moveCost);

// Residual (96.9365%): all 81 CFG blocks agree in flow/instruction count;
// 29 call sites align, including vector::size at +0x3fd and both Grail
// calls. Remaining differences are register allocation/bitfield scheduling and folded
// template target identities. DC lines 3300/3359 call GetMapExtra(point),
// not pasted coordinate extraction. Their canonical header overload removes
// the old missing-size-call mismatch and permits both Grail pins to retire.
// Controls in the 24-state boundary family: only one point call restored
// leaves the unpinned caller at 91.0238..91.3555%; a named first-point copy
// with the typed Grail temporary gives 91.0238%. The original two direct
// point calls and typed temporary together give the current 96.9365%.
VA(0x0042edd0, 0x79b)  // anchor-callee + arity, dc 0x33038
long findAllDestinations(Hero* currentHero, SearchArray* currentSearchArray,
                           std::vector<HeroDestination>& destinations,
                           long maxDistance, unsigned char hiringHero,
                           unsigned char allowSpells,
                           unsigned char exploreMode)
{
    HeroDestination point;
    long currentValue;
    unsigned char protectingTown;
    long levelSize;

    levelSize = g_mapWidth * g_mapHeight;
    int levelCells = g_game->getNumMapLevels() * levelSize;
    unsigned short* friendlyDistances = new unsigned short[levelCells];
    memset(friendlyDistances, -1, levelCells * sizeof(unsigned short));

    SearchType searchType;
    if (allowSpells) {
        searchType = const_AI_search;
    } else {
        searchType = const_AI_alternate_search;
    }
    currentValue = markDestinations(
        currentHero, maxDistance, currentSearchArray, friendlyDistances,
        searchType);

    protectingTown = 0;
    int townId = g_game->getTownId(currentHero->m_x, currentHero->m_y,
                                    currentHero->m_z);
    if (townId >= 0) {
        Town* currentTown = g_game->getTown(townId);
        if (currentTown->m_threateningHeroes > 0) {
            protectingTown = 1;
            if (currentTown->m_threateningHeroes > 1) {
                delete[] friendlyDistances;
                return currentValue;
            }
        }
    }

    for (long j = currentSearchArray->getVisitedCount(); j-- != 0;) {
        PathCell* cell = currentSearchArray->getVisitedCell(j);
        if (!allowSpells && cell->m_cost < cell->m_adjustedCost)
            continue;
        NewmapCell* mapCell = g_advManager->getCell(cell->m_point);
        if (!mapCell->m_isTrigger) {
            if ((getMapExtra(cell->m_point) & g_unnamed69ccc4)
                || g_currentPlayer->m_numTowns == 0)
                continue;
        }
        if (!currentHero->isInPatrolRadius(cell->m_point))
            continue;
        g_advManager->m_advWindow->animateBottomView(0);
        checkDoMain(0, 0);

        point.m_isCritical = 0;
        if (protectingTown) {
            if (mapCell->m_type != HERO)
                continue;
            Hero* other = g_game->getHero(mapCell->m_extraInfo);
            if (g_game->onSameTeam(other->m_owner, currentHero->m_owner))
                continue;
            if (static_cast<int>(cell->m_cost) > currentHero->m_movePoints)
                continue;
            point.m_isCritical = 1;
        }

        point.m_point.m_x = cell->m_point.m_x;
        point.m_point.m_y = cell->m_point.m_y;
        point.m_point.m_z = cell->m_point.m_z;
        point.m_moveCost = cell->m_cost;
        if (point.m_point == currentHero->getLocation())
            continue;

        if (g_unnamed693718[mapCell->m_type]
            && point.m_moveCost > friendlyDistances[
                point.m_point.m_z * levelSize + point.m_point.m_y * g_mapWidth
                + point.m_point.m_x])
            continue;
        point.m_moveCost = cell->m_adjustedCost;
        if (hiringHero)
            point.m_moveCost = 10000;
        if (!(getMapExtra(point.m_point) & g_unnamed69ccc4)
            && g_currentPlayer->m_numTowns > 0) {
            if (exploreMode) {
                point.m_value = 100000;
            } else {
                point.m_value = 100;
            }
        } else {
            point.m_value = aiValueOfEvent(currentHero, point.m_point,
                                            point.m_moveCost);
            if (point.m_value <= 0
                && (point.m_value != 0 || currentValue >= 0))
                continue;
        }
        destinations.push_back(point);
    }

    if (!protectingTown)
        checkHolyGrail(currentHero, currentSearchArray, destinations,
                         friendlyDistances);
    delete[] friendlyDistances;
    return currentValue;
}

// Residual (77.61%): all 30 blocks and every edge agree (25 exact blocks,
// five size-only) after the ==0-arm swap and j-- reverse walk. The 0x98 vs
// 0x9c frame and remaining ecx<->eax / edx<->ecx family are confined to the
// two point builds and SeedPosition argument formation; why-reg reports the
// schedule aligned, so local-renaming proposals are a measured handle-state
// wall rather than permission to flatten a DC-proven helper.

// 77.6134 -> 91.6997 (2026-09-05): EVERY type_point BUILT FROM THREE FRESH
// COORDINATES IS A CONSTRUCTOR CALL, NOT THREE FIELD ASSIGNMENTS. A default
// -constructed point followed by `.x =`/`.y =`/`.z =` makes VC6 apply the
// bitfield xor-trick to each field separately (`xor ax, word ptr [mem] /
// and eax, 0x3ff / xor eax, edi / and ah, -0x3d`); the constructor form lets
// it merge the two fields that share the +2 unit into retail's single
// clear-then-or (`mov edx, dword ptr [mem] / and edx, 0xffffc000 /
// and ecx, 0x3ff / and eax, 0xf / xor edx, ecx / shl eax, 0xa / or edx, eax
// / mov word ptr [mem], dx`). Three doses: danger_point +8.23, a fresh
// `start` local for the hero's own seed +1.00, `friend_point` +4.86.
// MEASURED AND REJECTED: `point = type_point(a, b, c)` into an existing
// function-scope local scores 83.00 (a copy through a temporary, not the
// merge); the constructor form on `target` scores 88.25, because retail
// reassigns that one in the is_valid arm and so declares it uninitialised.
// Residual (91.70%): a whole-body EBX/EDI transposition - retail keeps
// `current_hero` in EBX and the packing temp in EDI, we do the reverse.
// E:\gamedcs\ai_player.cpp:3044
// Seeds the hero's own search, then for every OTHER hero of the current
// player seeds a friendly allied search from that hero's path target (or
// position when the target is invalid) and folds each visited cell's cost
// plus the friend's remaining-target cost into the friendly-distance map,
// clipped to the patrol radius. Returns the danger under the hero's feet.
VA(0x0042f570, 0x40e)  // anchor-callee + arity, dc 0x32a84
long markDestinations(Hero* currentHero, long maxDistance,
                       SearchArray* currentSearchArray,
                       unsigned short* friendlyDistances,
                       SearchType searchType)
{
    int mapCells = g_mapHeight * g_mapWidth;
    SearchArray friendlySearch;
    long movePoints = currentHero->m_movePoints;
    long heroDanger;
    type_point point;
    {
        type_point dangerPoint(currentHero->m_x, currentHero->m_y,
                                currentHero->m_z);
        heroDanger = currentSearchArray->getDangerValue(dangerPoint);
    }
    g_advManager->m_advWindow->animateBottomView(0);
    type_point start(currentHero->m_x, currentHero->m_y, currentHero->m_z);
    currentSearchArray->seedPosition(currentHero, start, type_point(-1, -1, -1),
                               maxDistance,
                               (currentHero->m_flags >> 18) & 1, searchType,
                               movePoints, 0);

    for (int i = 0; i < g_currentPlayer->m_numHeroes; ++i) {
        Hero* friendly = g_game->getHero(g_currentPlayer->m_heroes[i]);
        if (friendly == currentHero)
            continue;
        type_point friendPoint(friendly->m_x, friendly->m_y, friendly->m_z);
        PathCell* friendCell = currentSearchArray->getCell(friendPoint, 0);
        if (!friendCell->m_visited)
            continue;

        type_point target;
        target.m_x = friendly->m_pathTargetX;
        target.m_y = friendly->m_pathTargetY;
        target.m_z = friendly->m_pathTargetZ;
        unsigned short extraCost;
        if (!target.isValid()) {
            target.m_x = friendly->m_x;
            target.m_y = friendly->m_y;
            target.m_z = friendly->m_z;
            extraCost = 0;
        } else {
            extraCost = friendly->m_targetDistance;
            if (extraCost > friendly->m_movePoints)
                extraCost = 0;
            else
                extraCost -= friendly->m_movePoints;
        }

        NewmapCell* targetCell = g_advManager->getCell(target);
        g_advManager->m_advWindow->animateBottomView(0);
        friendlySearch.seedPosition(
            friendly, target, type_point(-1, -1, -1),
            friendly->m_maxMovePoints,
            targetCell->m_groundSet == eTerrainWater, const_AI_allied_search,
            friendly->m_maxMovePoints, 0);

        for (int j = static_cast<int>(friendlySearch.getVisitedCount());
             j-- != 0;) {
            PathCell* visited = friendlySearch.getVisitedCell(j);
            if (currentHero->isInPatrolRadius(visited->m_point)) {
                int index = visited->m_point.m_z * mapCells
                    + visited->m_point.m_y * g_mapWidth + visited->m_point.m_x;
                unsigned short cost = visited->m_cost + extraCost;
                if (cost < friendlyDistances[index])
                    friendlyDistances[index] = cost;
            }
        }
    }
    return heroDanger;
}

// Residual (85.35%): flow-distance 0; why-reg v2 reports first defs agree
// (ebx=destination, esi=point, edi=point) and the residual is a mid-body
// edx<->ecx x15 / eax<->ebx x12 rename family plus retail's RMW
// `add [dest+8], ecx` where we load-add-store - handle-state, no local
// spelling reaches it (measured 2026-08-27).
// E:\gamedcs\ai_player.cpp:3498
// Prices one candidate destination: a pickupable trigger already visited by
// this player refunds the final step (move_cost re-based to last_point's
// cost), the strategic map plus the path's barrier/danger fields give the
// base value, an adjacent monster other than the path's own adds its event
// value, and the hero's current path target scales the result by 1.5 (+20)
// where anything else is scaled by Random(1,25)+75 percent.
VA(0x0042f980, 0x2c9)  // anchor-callee unique (Random, FindAdjacentMonster), dc 0x33854
int netValueOfLocation(Hero* currentHero, HeroDestination* destination,
                          long* strategicMap, PathCell* currentPathCell,
                          SearchArray* currentSearchArray)
{
    type_point point = destination->m_point;
    NewmapCell* cell = g_advManager->getCell(point);
    int type = cell->m_type;
    if (cell->m_isTrigger && g_adventureObjectTraits[type][0]) {
        if (getMapExtra(point.m_x, point.m_y, point.m_z) & g_unnamed69ccc4) {
            destination->m_moveCost -= currentPathCell->m_cost;
            point = currentPathCell->m_lastPoint;
            PathCell* lastCell = currentSearchArray->getCell(point, 0);
            destination->m_moveCost += lastCell->m_cost;
        }
    }

    long value = *getDangerCell(strategicMap, point)
        + currentPathCell->m_barrierValue;
    if (currentPathCell->m_dangerValue <= -500000000 && value >= 1968)
        currentPathCell->m_dangerValue = -2500000;
    if (value >= -500000000)
        value += currentPathCell->m_dangerValue;

    if (!g_adventureObjectTraits[type][0]) {
        type_point monsterPos;
        if (g_advManager->findAdjacentMonster(destination->m_point,
                                              &monsterPos,
                                              destination->m_point)) {
            if (!(currentPathCell->m_monster == monsterPos)
                && value >= -500000000)
                value += aiValueOfEvent(currentHero, monsterPos,
                                           destination->m_moveCost);
        }
    }

    if (destination->m_point.m_x == currentHero->m_pathTargetX
        && destination->m_point.m_y == currentHero->m_pathTargetY
        && destination->m_point.m_z == currentHero->m_pathTargetZ) {
        float scaled = static_cast<float>(value);
        if (value < 0)
            scaled = scaled / 1.5f;
        else
            scaled = scaled * 1.5f;
        int result = static_cast<int>(scaled) + 20;
        if (currentHero->m_targetIsCritical) {
            destination->m_isCritical = 1;
            return result;
        }
        return result;
    }

    int factor = random(1, 25) + 75;
    if (value <= 0)
        return static_cast<int>(100.0 / factor * value);
    int result = factor * value / 100;
    if (result < 1)
        result = 1;
    return result;
}

void aiSetHeroBonuses(Hero* ourHero);
void aiBuildShip(const Hero* ourHero, long x, long y, long z);

static void buildPath(Hero* currentHero, SearchArray* currentSearchArray,
                       std::vector<PathCell>& path,
                       HeroDestination& destination);
static unsigned char checkMoveSpell(Hero* currentHero,
                                      std::vector<PathCell>& path,
                                      long step, SpellID spell,
                                      unsigned char alreadyActive);
static unsigned char attemptTeleport(Hero* currentHero,
                                      std::vector<PathCell>& path,
                                      long step);

// E:\gamedcs\ai_player.cpp:3832
VA(0x0042fc50, 0x285)  // dc 0x341f4
unsigned char attemptStep(Hero* currentHero, PathCell* currentPathCell,
                           unsigned char standEnd, unsigned char firstStep)
{
    type_point triggerPoint;
    int direction = currentPathCell->m_direction;
    if (g_mouseManager->m_hideCount == 0
        && g_advManager->considerHidingMouse(currentHero, direction)) {
        int saveDraw = g_completeDrawEnabled;
        g_completeDrawEnabled = 1;
        g_mouseManager->hidePointer();
        g_completeDrawEnabled = saveDraw;
    }

    triggerPoint = currentPathCell->m_point;
    NewmapCell* cell = g_game->getCell(triggerPoint);

    if (currentPathCell->m_inBoat && !(currentHero->m_flags & 0x40000)) {
        if (!(cell->m_type == BOAT && cell->m_isTrigger)) {
            g_advManager->stopCursor(1);
            if (currentHero->canSummonBoat()) {
                if (firstStep)
                    g_advManager->castSpell(SPELL_SUMMON_BOAT);
                return 0;
            }
            // can_build_ship is bit 11 of the proven cellFlags word (the
            // header pools bits 10-11; this is the first admitted reader).
            if (cell->m_cellFlags & 0x800)
                aiBuildShip(currentHero, triggerPoint.m_x,
                              triggerPoint.m_y, triggerPoint.m_z);
        }
    }

    int savedX = currentHero->m_pathTargetX;
    int savedY = currentHero->m_pathTargetY;
    int savedZ = currentHero->m_pathTargetZ;

    unsigned char retargeted =
        currentPathCell->m_flying && currentPathCell->m_canStop && cell->m_isTrigger;
    if (retargeted) {
        currentHero->m_pathTargetX = triggerPoint.m_x;
        currentHero->m_pathTargetY = triggerPoint.m_y;
        currentHero->m_pathTargetZ = triggerPoint.m_z;
    }

    int noMove;
    int foughtBattle;
    NewmapCell* eventCell = g_advManager->moveHero(
        currentPathCell->m_direction, standEnd, triggerPoint, &noMove, 1,
        &foughtBattle, 0);
    if (retargeted) {
        currentHero->m_pathTargetX = savedX;
        currentHero->m_pathTargetY = savedY;
        currentHero->m_pathTargetZ = savedZ;
    }

    if (currentHero->m_owner != g_netLocalGamePos)
        return 0;

    if (eventCell == 0) {
        if (noMove != 0)
            currentHero->m_movePoints = 0;
    } else {
        g_advManager->doAIEvent(eventCell, currentHero, triggerPoint);
        if (g_currentPlayer->m_currHeroId == -1)
            return 0;
        if (eventCell->getMapObject() == HUT_OF_MAGI
            && eventCell->cellIsTrigger())
            g_aiPlayers[currentHero->m_owner].clearMagusHutValue();
        aiSetHeroBonuses(currentHero);
    }

    return eventCell == 0 && noMove == 0 && foughtBattle == 0;
}

static void buildPath(Hero* currentHero, SearchArray* currentSearchArray,
                       std::vector<PathCell>& path,
                       HeroDestination& destination)
{
    path.clear();
    g_searchArray->buildPath(currentHero, 99999);
    long i = g_searchArray->getPathSteps();
    if (i == 0) {
        currentHero->m_movePoints = 0;
        return;
    }

    while (i-- > 0) {
        const PathCell* currentPathCell = g_searchArray->getStepCell(i);
        if (currentPathCell->m_flying
            && (abs(currentPathCell->m_point.m_x - currentPathCell->m_lastPoint.m_x) > 1
                || abs(currentPathCell->m_point.m_y - currentPathCell->m_lastPoint.m_y) > 1
                || currentPathCell->m_point.m_z != currentPathCell->m_lastPoint.m_z)) {
            destination.m_point.m_x = currentPathCell->m_lastPoint.m_x;
            destination.m_point.m_y = currentPathCell->m_lastPoint.m_y;
            destination.m_point.m_z = currentPathCell->m_lastPoint.m_z;
            currentHero->m_pathTargetX = currentPathCell->m_lastPoint.m_x;
            currentHero->m_pathTargetY = currentPathCell->m_lastPoint.m_y;
            currentHero->m_pathTargetZ = currentPathCell->m_lastPoint.m_z;
            return;
        }
        path.push_back(*currentPathCell);
    }
}

static unsigned char checkMoveSpell(Hero* currentHero,
                                      std::vector<PathCell>& path,
                                      long step, SpellID spell,
                                      unsigned char alreadyActive)
{
    long i;
    for (i = step; i < path.size(); ++i) {
        if (path[i].m_canStop)
            break;
    }

    if (i == path.size()) {
        if (step == 0)
            currentHero->m_movePoints = 0;
        return 0;
    }

    long moveCost = path[i].m_cost;
    if (step > 0)
        moveCost -= path[step - 1].m_cost;

    if (currentHero->m_movePoints < moveCost) {
        if (step == 0)
            currentHero->m_movePoints = 0;
        return 0;
    }

    if (!alreadyActive) {
        g_advManager->stopCursor(1);
        g_advManager->castSpell(spell);
    }
    return 1;
}

// E:\gamedcs\ai_player.cpp:4000
DATA(0x00660500) static const long g_constThresholds[6] = {
    1000, 150, 100, 75, 50, 25
};

static unsigned char attemptTeleport(Hero* currentHero,
                                      std::vector<PathCell>& path,
                                      long step)
{
    unsigned char atManaSource;
    unsigned char willTeleport;
    type_point startPoint;
    long startCost;
    unsigned char inBoat;
    long bestSavings;
    long manaCost;
    long savings;
    long destinationIndex;
    long threshold;

    // Dreamcast's leading line 4001 emits no SH4 bytes immediately before
    // the first path[step] access. A release-elided bounds assertion is a
    // plausible source for that row and states the invariant this body needs.
    HOMM3_RELEASE_VERIFY(step < path.size());

    if (!path[step].m_lastCanStop)
        return 0;
    if (!currentHero->spellIsAvailable(SPELL_DIMENSION_DOOR))
        return 0;
    if (currentHero->heroFn004E4EC0() == CURSED_GROUND)
        return 0;

    manaCost = currentHero->getManaCost(SPELL_DIMENSION_DOOR);
    if (manaCost + 20 > currentHero->m_mana)
        return 0;

    long mastery = currentHero->getSpellLevel(SPELL_DIMENSION_DOOR);
    if (currentHero->m_dWalkSpellsCast
        >= g_spellTraits[SPELL_DIMENSION_DOOR].m_masteryBonus[mastery]) {
        if (path[step].m_dimensionDoor) {
            if (step == 0)
                currentHero->m_movePoints = 0;
            return 1;
        }
        return 0;
    }

    long castsRemaining = (currentHero->m_mana - 20) / manaCost;
    // Retail keeps this source object live in EBX across get_target/get_cell.
    // Dreamcast's local inventory is a lower bound and cannot expose an
    // optimizer-only pointer alias.
    Game* currentGame = g_game;
    AdventureObjectType targetType =
        currentGame->getCell(currentHero->getTarget())->m_type;
    if (targetType == MAGIC_WELL || targetType == MAGIC_SPRING)
        atManaSource = 1;
    else
        atManaSource = 0;
    if (castsRemaining > 6)
        castsRemaining = 6;
    threshold = g_constThresholds[castsRemaining - 1]
                * currentHero->m_maxMovePoints / 100;
    if (atManaSource && castsRemaining > 1)
        threshold = 200;

    inBoat = (currentHero->m_flags & 0x40000) != 0;
    // Dreamcast decrements this index in get_location's call delay slot. Its
    // placement immediately before that call also reproduces retail VC6's
    // interleaved lifetime; moving it after the call falls to 96.3350%.
    long pathIndex = step - 1;
    destinationIndex = -1;
    startPoint = currentHero->getLocation();
    bestSavings = 0;

    do {
        willTeleport = 0;
        if (pathIndex < 0)
            startCost = 200;
        else
            startCost = path[pathIndex].m_cost + 200;

        savings = 0;
        long candidateDestination = -1;
        long i;
        for (i = pathIndex + 1; i < path.size(); ++i) {
            if (path[i].m_dimensionDoor)
                willTeleport = 1;
            if (!path[i].m_canStop)
                continue;
            if (abs(path[i].m_point.m_x - startPoint.m_x) > 9
                || abs(path[i].m_point.m_y - startPoint.m_y) > 8
                || path[i].m_point.m_z != startPoint.m_z)
                continue;
            if (g_game->getCell(path[i].m_point)->m_isTrigger)
                continue;
            if (path[i].m_inBoat != inBoat)
                continue;

            savings = path[i].m_cost - startCost;
            if (atManaSource && savings > 0 && i + 2 == path.size())
                willTeleport = 1;
            if (!willTeleport && savings < threshold)
                continue;
            candidateDestination = i;
        }

        if (pathIndex < step && candidateDestination < 0)
            return 0;

        if (destinationIndex < 0 || savings > bestSavings) {
            if (pathIndex >= step)
                return 0;
            bestSavings = savings;
            destinationIndex = candidateDestination;
        }

        for (++pathIndex; pathIndex < destinationIndex; ++pathIndex) {
            if (!path[pathIndex].m_canStop
                || path[pathIndex].m_dimensionDoor) {
                pathIndex = destinationIndex;
                break;
            }
            if (path[pathIndex].m_point.m_z == startPoint.m_z
                && path[pathIndex].m_inBoat == inBoat) {
                startPoint = path[pathIndex].m_point;
                break;
            }
        }
    } while (pathIndex < destinationIndex);

    g_advManager->teleportTo(currentHero, path[destinationIndex].m_point, 0,
                             0, 1, 0);
    currentHero->useSpell(manaCost);
    ++currentHero->m_dWalkSpellsCast;
    return 1;
}
// E:\gamedcs\ai_player.cpp:4155, dc 0x34a7c.
static __forceinline void checkGatePurchase(type_point point)
{
    int townId = g_game->getTownId(point.m_x, point.m_y, point.m_z);
    if (townId >= 0) {
        Town* currentTown = g_game->getTown(townId);
        if (!currentTown->hasBuilding(EXTRA_1_ID, true))
            currentTown->buyBuilding(EXTRA_1_ID);
    }
}

// E:\gamedcs\ai_player.cpp:4179.  The reference pair is fixed by the DC
// decorated signature and the retail /Gr call at move_hero+0x219.  The body
// lies after attempt_step and immediately before the Town.h COMDAT band.
// Residual (84.83%, 2026-09-01): the Dreamcast statement groups now recover
// the compound puzzle-guess test, both check_gate_purchase -> game::GetTown
// boundaries, the Complete-only can-stop map lookup and its fresh location
// temporary, and the unsigned vector-size loop exits. predict-inline is down
// to one backend divergence: candidate shares one operator-delete tail where
// retail emits two calls. The signed size casts are a measured negative
// control (84.18% and signed jge instead of retail/DC's unsigned jae/cmp-hi);
// flattening GetTown or NewfullMap::cell removes retained retail calls and the
// sixth point constructor. Preserve these positive source facts across the
// remaining VC6 tail-merging/register-allocation wall.
// LOCALISED 2026-09-06: the structural half of the residual is one
// cross-jump.  Retail gives the puzzle-guess early return its OWN inline
// vector teardown - `_Destroy` + `operator delete` + epilogue at
// 0x42fee0+0x11c..0x14f, reached by the ProcessSearch arm's `jmp` and the
// movePoints=0 arm's fall-through - so it carries THREE `ret 8`s against our
// two; we cross-jump both arms into the can-stop return's tail instead.
// That is the merged-return class (both predecessors are jumps) and the
// duplicate cannot be spelled, because the block IS a destructor.
VA(0x0042fee0, 0x6b8)  // anchor-caller move_hero + order bracket, dc 0x34b08
void aiAttemptMove(Hero* currentHero, HeroDestination& bestPoint,
                    long& bestRawValue, unsigned char exploreMode)
{
    long totalCost;
    std::vector<PathCell> path;
    unsigned char firstStep;
    long maxDistance;
    type_point destination;

    // Dreamcast line 4188 groups both comparisons and all four accessors in
    // one short-circuit statement; retail likewise constructs get_target's
    // temporary before the second get_location temporary.
    if (g_currentPlayer->m_puzzleGuess == currentHero->getLocation()
        && currentHero->getLocation() == currentHero->getTarget()) {
        if (currentHero->m_movePoints == currentHero->m_maxMovePoints)
            g_advManager->processSearch(currentHero->m_x, currentHero->m_y,
                                        currentHero->m_z);
        else
            currentHero->m_movePoints = 0;
        return;
    }

    buildPath(currentHero, g_searchArray, path, bestPoint);
    if (path.size() == 0) {
        currentHero->m_movePoints = 0;
        return;
    }

    if (path[0].m_startAtTrigger) {
        g_advManager->mobilizeCurrHero(0, 0, 1);
        type_point point = currentHero->getLocation();
        NewmapCell* cell =
            g_game->m_worldMap.cell(point.m_x, point.m_y, point.m_z);
        g_advManager->doAIEvent(cell, currentHero,
                                currentHero->getLocation());
        return;
    }

    maxDistance = min(static_cast<long>(path.size()) - 1, 7);
    maxDistance = max(
        min(static_cast<long>(path[maxDistance].m_cost),
            currentHero->m_movePoints),
        350);
    firstStep = 1;
    unsigned char standEnd = 0;
    totalCost = 0;

    for (long step = 0; step < path.size(); ++step) {
        if (path[step].m_castleGate) {
            type_point gatePoint = currentHero->getLocation();
            checkGatePurchase(gatePoint);
            checkGatePurchase(path[step].m_point);
            currentHero->m_movePoints = max(currentHero->m_movePoints - 100, 0);
            g_advManager->teleportTo(currentHero, path[step].m_point, 0,
                                     0, 1, 0);
            return;
        }

        if (path[step].m_townPortal && step == 0) {
            g_advManager->teleportTo(currentHero, path[step].m_point, 0,
                                     0, 1, 0);
            currentHero->useSpell(
                currentHero->getManaCost(SPELL_TOWN_PORTAL));
            if (currentHero->getSpellLevel(SPELL_TOWN_PORTAL)
                == eMasteryExpert)
                currentHero->m_movePoints -= 200;
            else
                currentHero->m_movePoints -= 300;
            if (currentHero->m_movePoints < 0)
                currentHero->m_movePoints = 0;
            return;
        }

        // Dreamcast retains attempt_teleport as a source-real static helper,
        // and Complete keeps the same out-of-line boundary at 0x00430ab0.
        if (attemptTeleport(currentHero, path, step))
            return;

        if (path[step].m_flying
            && !checkMoveSpell(currentHero, path, step, SPELL_FLY,
                                 currentHero->isFlying(0)))
            return;

        if (path[step].m_waterWalking
            && !checkMoveSpell(
                currentHero, path, step, SPELL_WATER_WALK,
                currentHero->isFlying(0)
                    || currentHero->canWalkOnWater(0)))
            return;

        if (step + 1 >= path.size()
            || static_cast<long>(path[step + 1].m_cost) - totalCost
                   > currentHero->m_movePoints)
            standEnd = 1;

        if (!attemptStep(currentHero, &path[step], standEnd, firstStep))
            return;
        if (standEnd)
            return;

        firstStep = 0;
        bestPoint.m_moveCost -=
            static_cast<long>(path[step].m_cost) - totalCost;
        if (bestPoint.m_moveCost < 0)
            bestPoint.m_moveCost = 0;
        totalCost = path[step].m_cost;

        if (path[step].m_canStop
            && currentHero->m_movePoints >= 100
            && !bestPoint.m_isCritical) {
            destination = bestPoint.m_point;
            aiChooseDestination(currentHero, maxDistance, bestPoint,
                                  bestRawValue, 0, exploreMode);
            if (destination != bestPoint.m_point) {
                buildPath(currentHero, g_searchArray, path, bestPoint);
                totalCost = 0;
                firstStep = 1;
                step = -1;
            }
        }
    }
}

VA(0x00430610, 0x384)  // dc 0x343c4
static void buildPath(Hero* currentHero, SearchArray* currentSearchArray,
                       std::vector<PathCell>& path,
                       HeroDestination& destination);

VA(0x004309a0, 0x103)  // dc 0x34508
static unsigned char checkMoveSpell(Hero* currentHero,
                                      std::vector<PathCell>& path,
                                      long step, SpellID spell,
                                      unsigned char alreadyActive);

// E:\gamedcs\ai_player.cpp:4000. The definition remains in the proven
// source order above AI_AttemptMove; this redeclaration records VC6's later
// retained emission slot.
VA(0x00430ab0, 0x4c1)  // caller/callee/body bridge, dc 0x34630
static unsigned char attemptTeleport(Hero* currentHero,
                                      std::vector<PathCell>& path,
                                      long step);

// E:\\gamedcs\\ai_player.cpp:4457. Dreamcast proves two lexical artifact
// loops, with construction and valuation grouped in each statement, and the
// source helper remains real even though Complete inlines both calls below.
// The direct TArtifact construction preserves Dreamcast's proved field type.
// Replacing it with the compatibility artifact_from_int helper is the negative
// control: CFG stays 77/77 exact but the caller frame is four bytes short
// (99.97449%). The caller gates both recruit ids, so this helper has no
// source-false null guard.
static long totalArtifactValue(Hero* candidate, long playerId)
{
    long total = 0;
    long slot;
    for (slot = 0; slot < HERO_BACKPACK_CAPACITY; ++slot) {
        type_artifact backpackArtifact(
            candidate->getBackpack(slot).m_artifactId);
        total += aiGetValueOfArtifact(backpackArtifact, playerId);
    }
    for (slot = 0; slot < 19; ++slot) {
        type_artifact equippedArtifact(
            candidate->getArtifact(ArtifactSlot(slot)).m_artifactId);
        total += aiGetValueOfArtifact(equippedArtifact, playerId);
    }
    return total;
}

// E:\gamedcs\ai_player.cpp:4565, dc 0x357ec.

// The Dreamcast roster marks both coordinate lookups static, and its
// AI_build_ship xrefs mark both calls inlined. Retail retains those two
// source-level passes: owned town docks first, then claimed map shipyards.
static Town* getShipyardTown(const PlayerData* player, long x, long y,
                               long z)
{
    for (long i = 0; i < player->m_numTowns; ++i) {
        Town* currentTown = g_game->getTown(player->m_townIds[i]);
        if (currentTown->m_dockSite == x && currentTown->m_dockSiteY == y
            && currentTown->m_mapZ == z)
            return currentTown;
    }
    return 0;
}

// E:\gamedcs\ai_player.cpp:4583, dc 0x35888.
static unsigned char getMapShipyard(const PlayerData* player, long x,
                                      long y, long z)
{
    for (unsigned long i = 0; i < player->m_shipyards.size(); ++i) {
        if (player->m_shipyards[i].m_z == z) {
            NewmapCell* cell = g_game->getCell(player->m_shipyards[i]);
            const ShipyardInfo* shipyard = static_cast<const ShipyardInfo*>(
                static_cast<const void*>(&cell->m_extraInfo));
            if (shipyard->m_boatX == x && shipyard->m_boatY == y)
                return 1;
        }
    }
    return 0;
}

// E:\gamedcs\ai_player.cpp:4607
VA(0x00430f80, 0x1d2)  // dc 0x35910
void aiBuildShip(const Hero* ourHero, long x, long y, long z)
{
    if (ourHero->belongsToHuman() && !g_unk691209)
        return;

    PlayerData* player = &g_game->m_players[ourHero->m_owner];
    Town* shipyardTown = getShipyardTown(player, x, y, z);
    if (!shipyardTown) {
        if (!getMapShipyard(player, x, y, z))
            return;
    } else if (!shipyardTown->hasBuilding(DOCK_ID, 1)
               && !shipyardTown->buyBuilding(DOCK_ID)) {
        return;
    }

    if (player->m_resources[WOOD] < 10 || player->m_resources[GOLD] < 1000)
        return;
    if (g_game->createBoat(x, y, z, ourHero->m_owner, 0, 1) == -1)
        return;
    player->m_resources[GOLD] -= 1000;
    player->m_resources[WOOD] -= 10;
}

VA(0x00431160, 0x1f3)  // dc 0x35a10
long aiGetShipCost(const Hero* ourHero, type_point point)
{
    const PlayerData* player = &g_game->m_players[ourHero->m_owner];
    Town* shipyardTown =
        getShipyardTown(player, point.m_x, point.m_y, point.m_z);
    int cost[7];
    memset(cost, 0, sizeof cost);

    if (!shipyardTown) {
        if (!getMapShipyard(player, point.m_x, point.m_y, point.m_z))
            return -200000;
    } else if (!shipyardTown->hasBuilding(DOCK_ID, 1)) {
        shipyardTown->getBuildCost(DOCK_ID, cost);
    }

    cost[WOOD] += 10;
    cost[GOLD] += 1000;
    return -aiResourceCost(player, cost);
}

VA(0x00431360, 0x463)  // dc 0x35ac8
bool AIPlayer::hireHeroes()
{
    PlayerData* player = &g_game->m_players[m_team];
    Hero* first = 0;
    Hero* second = 0;
    if (player->m_numHeroes >= PlayerData::HERO_SLOT_COUNT)
        return false;
    if (player->m_resources[GOLD] < g_heroGoldCost)
        return false;
    if (player->m_numHeroes >= g_heroLimits[g_game->m_setup.m_difficulty])
        return false;

    long globalHeroes = 0;
    for (long playerId = 0; playerId < 8; ++playerId) {
        if (!g_game->m_playerDisabled[playerId]
            && !g_game->isHuman(playerId)) {
            globalHeroes += g_game->m_players[playerId].m_numHeroes;
        }
    }
    if (player->m_numHeroes > 0
        && globalHeroes >= g_globalLimits[g_game->m_setup.m_difficulty]) {
        return false;
    }

    long firstId = player->m_recruits[0];
    long firstValue;
    if (firstId != -1) {
        first = g_game->getHero(firstId);
        firstValue = totalArtifactValue(first, m_team);
    }
    long secondId = player->m_recruits[1];
    long secondValue;
    if (secondId != -1) {
        second = g_game->getHero(secondId);
        secondValue = totalArtifactValue(second, m_team);
    }

    if (first && secondValue <= firstValue) {
        if (!second || secondValue < firstValue)
            return considerHiring(m_team, first);
        if (second->getPrimarySkillTotal()
            > first->getPrimarySkillTotal()) {
            return considerHiring(m_team, second);
        }
        return considerHiring(m_team, first);
    }
    if (!second)
        return false;
    return considerHiring(m_team, second);
}

// Local prototypes, the events.cpp pattern: value_of_hiring's body follows
// consider_hiring below (retail 0x431bd0); AI_resource_cost is philai.obj's
// long-id overload (philai.cpp:1040); CanBuy is castle.h's free checker.
long valueOfHiring(Town* currentTown, Hero* candidate,
                     SearchArray* currentSearchArray);
int aiResourceCost(long playerId, const int* resources);
int canBuy(const Town* currTown, int buildingId);

VA(0x00431800, 0x3c2)  // dc 0x354bc
bool considerHiring(long playerId, Hero* candidate)
{
    PlayerData& player = g_game->m_players[playerId];
    long total = totalArtifactValue(candidate, playerId);
    int slot;
    for (slot = 0; slot < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
        CreatureType type = candidate->m_army.m_armyTypes[slot];
        if (type != CREATURE_NONE) {
            const int* creatureCost = g_creatureTypeTraits[type].m_cost;
            double troops = candidate->m_army.m_numTroops[slot];
            for (int resource = 0; resource < 7; ++resource)
                total = static_cast<long>(
                    creatureCost[resource]
                    * player.m_ai.m_resourceValue[resource] * troops + total);
        }
    }

    Town* bestTown = 0;
    SearchArray currentSearchArray;
    long bestValue = static_cast<long>(
        static_cast<double>(player.m_numHeroes)
        * player.m_ai.m_resourceValue[GOLD] * g_heroGoldCost);
    if (bestValue > total
        && player.m_resources[GOLD] < player.m_numHeroes * g_heroGoldCost)
        return 0;

    for (int i = 0; i < player.m_numTowns; ++i) {
        Town* currentTown = g_game->getTown(player.m_townIds[i]);
        if (currentTown->m_visitingHeroId >= 0)
            continue;
        long value = total;
        if (!currentTown->hasBuilding(TAVERN_ID, 1)) {
            if (!currentTown->canBuild(TAVERN_ID))
                continue;
            if (!canBuy(currentTown, TAVERN_ID))
                continue;
            value -= aiResourceCost(
                playerId, currentTown->getBuildCostArray(TAVERN_ID));
        }
        value += valueOfHiring(currentTown, candidate, &currentSearchArray);
        if (value > bestValue) {
            bestValue = value;
            bestTown = currentTown;
        }
    }
    if (bestTown == 0)
        return 0;

    if (!bestTown->hasBuilding(TAVERN_ID, 1)) {
        if (!bestTown->buyBuilding(TAVERN_ID))
            return 0;
        if (player.m_resources[GOLD] < g_heroGoldCost)
            return 0;
    }
    bestTown->hire(candidate, playerId);
    return 1;
}

// E:\gamedcs\ai_player.cpp:4320
// value_of_hiring (dc 0x34fb8, 1094 B static) grew to retail's 1611 B.
// Double anchor: consider_hiring calls it at 0x432bce with (town ECX,
// candidate EDX, &searchArray stacked) - the DC three-argument signature -
// and its own body calls AI_arrange_army at 0x431d9d (inside the expanded
// do_swap). Simulates hiring the candidate at this town: army and funds are
// staged on copies, the candidate is teleported onto the town square, and
// find_all_destinations prices what the new hero could reach - shared
// against every own hero whose cell the search touched.
// Raw NB11 places all thirteen named DC locals in the procedure scope.  It
// also names the non-const town::get_army overload. The normalized retail
// target labels the one surviving ICF-folded body with the const public, but
// that synthesized name cannot prove which identical source overload called
// it. A const-receiver cast is byte/score-flat and violates the positive DC
// fact, so it is not an admissible reconstruction.
// Residual (99.95219%): all 56 blocks and 481 instructions agree; only two
// stack-color classes differ. Retail uses {player_id,-0x14; i,-0x1c} where
// our CL swaps them (their later best-value/touched partners follow), and
// {-0x28 point temp; -0x20 monster_cell} where ours uses {-0x24;-0x28}.
// The guided why-reg sweep tried all 17 applicable mutations without a gain.
// Also rejected: declaration/initializer splits and hoists (byte-flat),
// separate and for-scoped destination indices (byte-flat), swapping the
// two hero-counter initializers (99.70), unifying all three indices (98.20),
// and block-scoping cell/monster_cell per loop (94.40).
VA(0x00431bd0, 0x64b)  // anchor-callee (consider_hiring 0x432bce + AI_arrange_army 0x431d9d), dc 0x34fb8
long valueOfHiring(Town* currentTown, Hero* candidate,
                     SearchArray* currentSearchArray)
{
    short playerId = currentTown->m_owner;
    PlayerData* player = &g_game->m_players[currentTown->m_owner];
    ArmyGroup heroArmy = candidate->m_army;
    ArmyGroup townArmy = static_cast<const Town*>(currentTown)->getArmy();
    AICreaturePurchaser purchaser(playerId, currentTown);

    candidate->m_turnExperienceToRvRatio = 0;
    candidate->m_owner = static_cast<char>(playerId);
    int resources[7];
    memcpy(resources, player->m_resources, sizeof(resources));
    short population[14];
    memcpy(population, currentTown->m_population, sizeof(population));
    player->m_resources[GOLD] -= g_heroGoldCost;

    unsigned char hasAlliance =
        candidate->isWieldingArtifact(ARTIFACT_ANGELIC_ALLIANCE)
        || player->hasGivenArtifact(ARTIFACT_ANGELIC_ALLIANCE);
    purchaser.doSwap(candidate, &townArmy, 0, hasAlliance);
    purchaser.doPurchase(&candidate->m_army, candidate->getMorale(0, 0, 1),
                          &townArmy, player->m_resources, 0, hasAlliance);

    std::vector<HeroDestination> destinations;
    candidate->m_x = currentTown->m_mapX;
    candidate->m_y = currentTown->m_mapY;
    candidate->m_z = currentTown->m_mapZ;
    findAllDestinations(candidate, currentSearchArray, destinations, 0x7fff,
                          1, 0, 0);

    std::vector<PathCell*> monsters;
    long totalValue = 0;
    HeroDestination destination;
    PathCell* monsterCell;
    unsigned int i;
    for (i = 0; i < destinations.size(); ++i) {
        destination = destinations[i];
        PathCell* cell = currentSearchArray->getCell(destination.m_point, 0);
        NewmapCell* mapCell = g_advManager->getCell(destination.m_point);
        if (mapCell->m_type == HERO && mapCell->m_isTrigger
            && g_game->getHero(mapCell->m_extraInfo)->m_owner == playerId) {
            cell->m_barrierValue = destination.m_value;
        } else if (cell->m_barrierValue < 0
                   && cell->m_monster.m_x < 255) {
            monsterCell = currentSearchArray->getCell(cell->m_monster, 0);
            if (monsterCell->m_visited) {
                monsters.push_back(monsterCell);
                monsterCell->m_visited = 0;
            }
            monsterCell->m_barrierValue += destination.m_value;
        } else {
            totalValue += destination.m_value;
        }
    }
    for (i = 0; i < monsters.size(); ++i) {
        monsterCell = monsters[i];
        if (monsterCell->m_barrierValue > 0)
            totalValue += monsterCell->m_barrierValue;
    }

    long heroesTouched = 1;
    long bestHeroValue = 0;
    for (int heroIndex = 0; heroIndex < player->m_numHeroes; ++heroIndex) {
        Hero* other = g_game->getHero(player->m_heroes[heroIndex]);
        if (other->m_z == candidate->m_z) {
            PathCell* cell = currentSearchArray->getCell(
                type_point(other->m_x, other->m_y, other->m_z), 0);

            if (cell->m_visited) {
                ++heroesTouched;
                long value = cell->m_barrierValue;
                if (cell->m_monster.m_x < 255) {
                    monsterCell = currentSearchArray->getCell(cell->m_monster, 0);
                    if (monsterCell->m_barrierValue < 0)
                        value += monsterCell->m_barrierValue;
                }
                if (value > bestHeroValue)
                    bestHeroValue = value;
            }
        }
    }

    candidate->m_owner = -1;
    candidate->m_army = heroArmy;
    memcpy(player->m_resources, resources, sizeof(resources));
    memcpy(currentTown->m_population, population, sizeof(population));
    return (bestHeroValue + totalValue) / heroesTouched;
}

VA(0x00432220, 0x233)  // dc 0x35c40
long aiValueOfObservatory(type_point origin, long playerId, long range)
{
    long value = 0;
    type_point point;
    unsigned short playerBit = static_cast<unsigned short>(1 << playerId);
    double distance = static_cast<double>(range) + 0.5;
    RECT rect;
    rect.left = max(static_cast<long>(origin.m_x) - range, 0);
    rect.right = min(static_cast<long>(origin.m_x) + range + 1, g_mapWidth);
    rect.top = max(static_cast<long>(origin.m_y) - range, 0);
    rect.bottom = min(static_cast<long>(origin.m_y) + range + 1, g_mapHeight);
    point.m_z = origin.m_z;

    for (point.m_y = static_cast<short>(rect.top); point.m_y < rect.bottom;
         ++point.m_y) {
        for (point.m_x = static_cast<short>(rect.left); point.m_x < rect.right;
             ++point.m_x) {
            if (sqrt(static_cast<double>((point.m_x - origin.m_x)
                                        * (point.m_x - origin.m_x)
                                        + (point.m_y - origin.m_y)
                                          * (point.m_y - origin.m_y))) > distance)
                continue;
            if (getMapExtra(point.m_x, point.m_y, point.m_z) & playerBit)
                continue;

            ++value;
            NewmapCell* cell = g_game->getCell(point);
            if (cell->m_isTrigger)
                value += g_aiEventVisibilityValues[cell->m_type];
        }
    }
    return value;
}

VA_COMPGEN(0x004324b0, 0x18, DEFAULT_CTOR_CLOSURE, ArtifactEffect)

VA_COMPGEN(0x004324d0, 0x23, SCALAR_DELETING_DTOR, ArtifactEffect)

VA(0x00432500, 0x7)  // dc 0x361f4
ArtifactEffect::~ArtifactEffect()
{
}

VA(0x00432510, 0x24)  // dc 0x36258
long ScoutingArtifact::getValue(const Hero* owner, unsigned char, unsigned char) const
{
    return owner->m_maxMovePoints * m_bonus / 100;
}

VA(0x00432540, 0x15)  // dc 0x36274
CombatArtifact::CombatArtifact(long newBonus)
{
    m_bonus = newBonus;
}

VA(0x00432560, 0x32)  // dc 0x362b8
long CombatArtifact::getValue(const Hero* owner, unsigned char, unsigned char) const
{
    return owner->m_army.getAIValue() * m_bonus / 100;
}

VA(0x004325a0, 0x40)  // dc 0x36320
long MightArtifact::getValue(const Hero* owner, unsigned char, unsigned char exact) const
{
    if (exact)
        return 0;
    return owner->m_army.getAIValue() * m_bonus / 40;
}

VA(0x004325e0, 0x21)  // dc 0x36390
long PowerArtifact::getValue(const Hero* owner, unsigned char, unsigned char exact) const
{
    if (exact)
        return 0;
    return owner->getValueOfPower() * m_bonus;
}

VA(0x00432610, 0x21)  // dc 0x363f0
long KnowledgeArtifact::getValue(const Hero* owner, unsigned char, unsigned char exact) const
{
    if (exact)
        return 0;
    return owner->getValueOfKnowledge() * m_bonus;
}

// E:\gamedcs\ai_player.cpp:5152
VA(0x00432640, 0x97)  // artifact get_value cluster order-map + get_AI_value, dc 0x36450
long NecromancyArtifact::getValue(const Hero* owner, unsigned char equipped, unsigned char) const
{
    if (owner->m_skillLevel[12] == 0)
        return 0;
    long effect = static_cast<long>(
        (1.0f - owner->getNecromancyFactor(0)) * 100.0f);
    if (equipped) {
        if (effect > 0)
            effect = 0;
        effect += m_bonus;
    } else {
        effect = min(effect, m_bonus);
    }
    if (effect <= 0)
        return 0;
    return owner->m_army.getAIValue() * effect / 250;
}

VA(0x004326e0, 0x38)  // dc 0x3652c
long MovementArtifact::getValue(const Hero* owner, unsigned char, unsigned char) const
{
    return (owner->m_army.getAIValue() + 2500) * m_bonus / 100;
}

VA(0x00432720, 0x54)  // dc 0x3659c
long SpellcasterArtifact::getValue(const Hero* owner, unsigned char, unsigned char) const
{
    if (owner->getValueOfPower() == 0)
        return 0;
    if (owner->m_skillLevel[eSecSkillWisdom] == 0)
        return 0;
    return owner->m_army.getAIValue() * m_bonus / 100;
}

// The morale/luck effects weight AI_value_of_morale/AI_value_of_luck (fastcall
// free functions defined in philai.cpp, declared there and in ai_tactical.h)
// by the hero's whole-army value. Declared locally here, as philai.cpp does,
// to avoid pulling ai_tactical.h into this TU.
double aiValueOfMorale(long morale, long change);
double aiValueOfLuck(long luck, long change);

VA(0x00432780, 0x68)  // dc 0x3662c
long MoraleArtifact::getValue(const Hero* owner, unsigned char equipped, unsigned char exact) const
{
    if (exact)
        return 0;
    int morale = owner->getMorale(0, 0, 0);
    if (equipped)
        morale -= m_bonus;
    return static_cast<long>(aiValueOfMorale(morale, m_bonus)
                             * owner->m_army.getAIValue());
}

VA(0x004327f0, 0x68)  // dc 0x36720
long LuckArtifact::getValue(const Hero* owner, unsigned char equipped, unsigned char exact) const
{
    if (exact)
        return 0;
    int luck = owner->getLuck(0, 0, 0);
    if (equipped)
        luck -= m_bonus;
    return static_cast<long>(aiValueOfLuck(luck, m_bonus)
                             * owner->m_army.getAIValue());
}

VA(0x00432860, 0x21)  // dc 0x36814
long DurationArtifact::getValue(const Hero* owner, unsigned char, unsigned char exact) const
{
    if (exact)
        return 0;
    return owner->getValueOfDuration() * m_bonus;
}

VA(0x00432890, 0x1b2)  // dc 0x3687c
long SchoolArtifact::getValue(const Hero* owner, unsigned char equipped,
                                     unsigned char exact) const
{
    if (exact)
        return 0;

    Spellvalue caster(owner);
    if (!caster.canCastSpells())
        return 0;

    long power = owner->getPrimarySkill(2);

    long bestValue = 0;
    long baseValue;
    if (equipped) {
        baseValue = power;
        power = power * 100 / (m_bonus + 100);
    } else {
        baseValue = power * (m_bonus + 100) / 100;
    }

    for (SpellID spell = 0; spell < 70; spell++) {
        if (!owner->spellIsAvailable(spell))
            continue;
        if (!(g_spellTraits[spell].m_schoolBits & m_school))
            continue;
        if (!(g_spellTraits[spell].m_flags & 0x200))
            continue;

        caster.setPower(power);
        long value = caster.getRawSpellValue(spell);
        caster.setPower(baseValue);
        value = caster.getRawSpellValue(spell) - value;
        if (m_bonus < 0)
            bestValue = min(value, bestValue);
        else
            bestValue = max(value, bestValue);
    }
    return bestValue;
}

VA(0x00432a50, 0xc3)  // dc 0x36a1c
long AntimagicArtifact::getValue(const Hero* owner, unsigned char equipped, unsigned char exact) const
{
    long value;
    if (m_bonus == 0)
        value = owner->m_army.getAIValue() / 5;
    else
        value = owner->m_army.getAIValue() / 8;
    if (exact)
        return value;
    if (!equipped)
        return value;
    if (m_bonus == 0) {
        int m = owner->getPrimarySkill(2);
        return value - m * 50;
    }
    int m = owner->getPrimarySkill(2);
    return value - m * 25;
}

VA(0x00432b20, 0x78)  // dc 0x36afc
long AntimoraleArtifact::getValue(const Hero* owner, unsigned char, unsigned char exact) const
{
    long army = owner->m_army.getAIValue();
    long result = static_cast<long>(aiValueOfMorale(0, 2) * army);
    if (exact)
        return result;
    int morale = owner->getMorale(0, 0, 1);
    if (morale > 0)
        result = static_cast<long>(aiValueOfMorale(morale, -morale) * army + result);
    return result;
}

VA(0x00432ba0, 0x78)  // dc 0x36c90
long AntiluckArtifact::getValue(const Hero* owner, unsigned char, unsigned char exact) const
{
    long army = owner->m_army.getAIValue();
    long result = static_cast<long>(aiValueOfLuck(0, 2) * army);
    if (exact)
        return result;
    int luck = owner->getLuck(0, 0, 1);
    if (luck > 0)
        result = static_cast<long>(aiValueOfLuck(luck, -luck) * army + result);
    return result;
}

VA(0x00432c20, 0xf5)  // dc 0x36e28
long TomeArtifact::getValue(const Hero* owner, unsigned char equipped,
                                   unsigned char exact) const
{
    if (exact)
        return 0;

    Spellvalue caster(owner);
    if (!caster.canCastSpells())
        return 0;

    long bestValue = 0;
    for (SpellID spell = 0; spell < 70; spell++) {
        if (owner->isInSpellbook(spell))
            continue;
        if (!equipped && owner->spellIsAvailable(spell))
            continue;
        if (!(g_spellTraits[spell].m_schoolBits & m_school))
            continue;

        long value = caster.getRawSpellValue(spell);
        bestValue = max(value, bestValue);
    }
    return bestValue;
}

VA(0x00432d20, 0x49)  // dc 0x36f54
long IncomeArtifact::getValue(const Hero* owner, unsigned char,
                                     unsigned char) const
{
    return static_cast<long>(
        m_amount * g_aiPlayers[owner->m_owner].getResourceValue(m_resource) * 3.0);
}

VA(0x00432d70, 0x219)  // dc 0x3704c
long CreatureGrowthArtifact::getValue(const Hero* owner,
                                               unsigned char,
                                               unsigned char exact) const
{
    long value = 0;
    const PlayerData& player = g_game->m_players[owner->m_owner];

    if (exact) {
        int townId = g_game->getTownId(owner->m_x, owner->m_y, owner->m_z);
        if (townId < 0)
            return 0;

        Town* currentTown = g_game->getTown(townId);
        if (!currentTown->hasBuilding(DWELLING_0_ID + m_bonus, 1))
            return 0;
        if (currentTown->m_garrisonHeroId != owner->m_id)
            return 1;

        int dwelling = m_bonus;
        if (currentTown->hasBuilding(DWELLING_0_UPG_ID + m_bonus, 1))
            dwelling += TOWN_DWELLING_COUNT;
        CreatureType creature = g_townDwellingCreatures[
            currentTown->m_type * TOWN_DWELLING_SLOTS + dwelling];
        return g_creatureTypeTraits[creature].m_aiValue * m_growthBonus;
    }

    for (int i = 0; i < player.m_numTowns; ++i) {
        Town* currentTown = g_game->getTown(player.m_townIds[i]);
        if (!currentTown->hasBuilding(DWELLING_0_ID + m_bonus, 1))
            continue;
        int dwelling = m_bonus;
        if (currentTown->hasBuilding(DWELLING_0_UPG_ID + m_bonus, 1))
            dwelling += TOWN_DWELLING_COUNT;
        CreatureType creature = g_townDwellingCreatures[
            currentTown->m_type * TOWN_DWELLING_SLOTS + dwelling];
        value = max(
            value, g_creatureTypeTraits[creature].m_aiValue * m_growthBonus);
    }
    return value;
}

// Complete extends initialize_artifact_effects from DC's effect kinds
// 0..17 (dc 0x35f3c/0x35f3e) to 0..23 (retail 0x43415b). Its new
// jump-table entries 18..23 at 0x434578..0x43458c construct these six
// concrete effects at 0x43443e/0x434463/0x434485/0x4344a2/0x4344cd/
// 0x4344e3. Their vptr stores link each constructor to the getValue below:
// 0x63b74c/0x63b754/0x63b75c/0x63b764/0x63b778/0x63b770 respectively.
// Each table has the shared deleting destructor 0x433080 at slot 0 and
// the matching three-argument, ret 0xc getValue at slot 1. The exact
// Complete-only constructor and virtual definitions are reviewed in
// config/win_only.tsv; class names remain provisional semantic names.
VA(0x00432f90, 0xe4)  // vtable-slot 0x63b750 + get_raw_spell_value, retail-only
long SpellArtifact::getValue(const Hero* owner, unsigned char equipped,
                                    unsigned char exact) const
{
    if (exact)
        return 0;
    if (owner->isInSpellbook(SpellID(m_spell)))
        return 0;
    if (!equipped && owner->spellIsAvailable(m_spell))
        return 0;

    Spellvalue caster(owner);
    if (!caster.canCastSpells())
        return 0;
    long value = caster.getRawSpellValue(m_spell);
    return value;
}

VA_COMPGEN(0x00433080, 0x21, SCALAR_DELETING_DTOR, CombatArtifact)

VA(0x004330b0, 0x73)
long ShooterBonusArtifact::getValue(const Hero* owner, unsigned char, unsigned char) const
{
    long total = 0;
    for (int i = 0; i < 7; i++) {
        int type = owner->m_army.m_armies[i];
        if (type != -1 && (g_creatureTypeTraits[type].m_attributes & 0x4))
            total += g_creatureTypeTraits[type].m_aiValue * owner->m_army.m_numTroops[i];
    }
    return m_bonus * total / 100;
}

VA(0x00433130, 0x26f)
long AngelicAllianceArtifact::getValue(
    const Hero* owner, unsigned char, unsigned char exact) const
{
    std::bitset<9> alliedAlignments = armyGrpFn0044A460();
    PlayerData* player = &g_game->m_players[owner->m_owner];
    long total = 0;
    int heroIndex = 0;

    for (; heroIndex < player->m_numHeroes; ++heroIndex) {
        Hero* currentHero = g_game->getHero(player->m_heroes[heroIndex]);
        for (int heroSlot = 0;
             heroSlot < ArmyGroup::ARMY_GROUP_SLOT_COUNT;
             ++heroSlot) {
            int creature = currentHero->m_army.m_armies[heroSlot];
            if (creature == CREATURE_NONE)
                continue;
            if (g_game->m_f1f698 == 0
                && isBaseElemental(creature)) {
                continue;
            }
            int alignment = g_creatureTypeTraits[creature].m_townType;
            if (alignment != -1 && alliedAlignments.test(alignment)) {
                total += g_creatureTypeTraits[creature].m_aiValue
                         * currentHero->m_army.m_numTroops[heroSlot];
            }
        }
    }

    for (int townIndex = 0; townIndex < player->m_numTowns; ++townIndex) {
        const ArmyGroup& townArmy =
            static_cast<const Town*>(
                g_game->getTown(player->m_townIds[townIndex]))->getArmy();
        for (int townSlot = 0;
             townSlot < ArmyGroup::ARMY_GROUP_SLOT_COUNT;
             ++townSlot) {
            int creature = townArmy.m_armies[townSlot];
            if (creature == CREATURE_NONE)
                continue;
            if (g_game->m_f1f698 == 0
                && isBaseElemental(creature)) {
                continue;
            }
            int alignment = g_creatureTypeTraits[creature].m_townType;
            if (alignment != -1 && alliedAlignments.test(alignment)) {
                total += g_creatureTypeTraits[creature].m_aiValue
                         * townArmy.m_numTroops[townSlot];
            }
        }
    }

    long ownArmyValue;
    if (exact) {
        ownArmyValue = 0;
    } else {
        ownArmyValue = owner->m_army.getAIValue() * m_bonus / 40;
    }
    return ownArmyValue + total * 5 / 100;
}

// Residual (96.78%): all 29 blocks and all 12 branches agree. The remaining
// code delta is the first `/ 250` return's quotient register schedule (retail
// keeps it in edx across the split epilogue; this compiler spelling moves it
// through eax), plus cosmetic names for two float constants and the creature
// traits relocation. The later `/ 250` arm is instruction-exact. A bounded
// why-reg sweep found six local/declaration candidates flat or worse; naming
// either the numerator or the completed quotient in this early arm is also
// byte-flat. Keep the direct expression rather than manufacture a register
// carrier.
VA(0x004333a0, 0x174)  // vtable-slot 0x63b768 (provisional type), retail-only
long UndeadKingCloakArtifact::getValue(const Hero* owner,
                                                unsigned char equipped,
                                                unsigned char) const
{
    int necromancy;
    if (owner->m_skillLevel[12] == 0) {
        necromancy = static_cast<int>(
            (1.0f - owner->getNecromancyFactor(0)) * 100.0f);
        if (equipped) {
            if (necromancy > 0)
                necromancy = 0;
            necromancy += m_bonus;
        } else {
            necromancy = cppMin(necromancy, static_cast<int>(m_bonus));
        }
        if (necromancy <= 0)
            return 0;
        return owner->m_army.getAIValue() * necromancy / 250;
    }

    CreatureType creature;
    switch (owner->m_skillLevel[12]) {
    case eMasteryBasic:
        creature = CREATURE_WALKING_DEAD;
        break;
    case eMasteryAdvanced:
        creature = CREATURE_WIGHT;
        break;
    case eMasteryExpert:
        creature = CREATURE_LICH;
        break;
    }

    float multiplier =
        (static_cast<float>(g_creatureTypeTraits[creature].m_aiValue) -
         static_cast<float>(g_creatureTypeTraits[CREATURE_SKELETON].m_aiValue)) /
        static_cast<float>(g_creatureTypeTraits[CREATURE_SKELETON].m_aiValue);
    necromancy = static_cast<int>(
        (1.0f - owner->getNecromancyFactor(0)) * 100.0f);
    if (equipped) {
        if (necromancy > 0)
            necromancy = 0;
        necromancy += m_bonus;
    } else {
        necromancy = cppMin(necromancy, static_cast<int>(m_bonus));
    }

    long value;
    if (necromancy <= 0)
        value = 0;
    else
        value = owner->m_army.getAIValue() * necromancy / 250;
    return static_cast<long>(value * multiplier);
}

VA(0x00433520, 0x5a)
long ElixirOfLifeArtifact::getValue(const Hero* owner, unsigned char, unsigned char) const
{
    long total = 0;
    for (int i = 0; i < 7; i++) {
        int type = owner->m_army.m_armies[i];
        if (type != -1 && (g_creatureTypeTraits[type].m_attributes & 0x10))
            total += g_creatureTypeTraits[type].m_aiValue * owner->m_army.m_numTroops[i];
    }
    return total / 8;
}

VA(0x00433580, 0x13a)
long StatueOfLegionArtifact::getValue(
    const Hero* owner, unsigned char, unsigned char) const
{
    long total = 0;
    PlayerData* player = &g_game->m_players[owner->m_owner];
    for (int townIndex = 0; townIndex < player->m_numTowns; ++townIndex) {
        const Town* currentTown =
            g_game->getTown(player->m_townIds[townIndex]);
        for (int dwelling = 0; dwelling < TOWN_DWELLING_COUNT; ++dwelling) {
            if (!(currentTown->m_active
                  & g_bitNumber[DWELLING_0_ID + dwelling])) {
                continue;
            }

            int dwellingSlot = dwelling;
            if (currentTown->m_active
                & g_bitNumber[DWELLING_0_UPG_ID + dwelling]) {
                dwellingSlot += TOWN_DWELLING_COUNT;
            }
            CreatureType creature = g_townDwellingCreatures[
                currentTown->m_type * TOWN_DWELLING_SLOTS + dwellingSlot];
            long growth = g_creatureTypeTraits[creature].m_growthRate;
            growth += currentTown->getCastleGrowthBonus(creature);
            total += g_creatureTypeTraits[creature].m_aiValue * growth / 2;
        }
    }
    return total;
}

// E:\gamedcs\ai_player.cpp:5557
// Retail's /Gr call sites put owner/equipped in ECX/DL and the two-dword
// artifact plus exact on the stack. The contiguous 1..6 jump table separates
// the scroll and three war-machine appraisals from the ordinary effect-vector
// path; Dreamcast independently names the function, its four parameters and
// const_artifact_effects. The current ordinary-C++ reconstruction has retail's
// 28/28 branch stream and all eight returns. Residual (92.7585%) is confined to
// first-aid temporary homes and the two effect-vector loop schedules; 42 of 55
// CFG blocks are instruction-exact. An authentic inline first-aid helper was
// tested and rejected (88.3639%).
VA(0x004336c0, 0x320)  // anchor-callee unique (hero::GetFirstAidFactor), dc 0x37194
long aiGetValueOfArtifact(type_artifact artifact, const Hero* owner, unsigned char equipped, unsigned char exact)
{
    if (artifact.m_artifactId == ARTIFACT_NONE)
        return 0;

    long value = 0;
    switch (artifact.m_artifactId) {
    case ARTIFACT_SPELL_SCROLL: {
        if (owner->isInSpellbook(SpellID(artifact.m_extra)))
            return 0;
        if (!equipped && owner->spellIsAvailable(artifact.m_extra))
            return 0;
        // The ordinal copy belongs to this scroll appraisal. Keep memcpy
        // so the bridge preserves bits without an out-of-range enum cast.
        SpellID spell;
        {
            int ordinal = artifact.m_extra;
            memcpy(&spell, &ordinal, sizeof spell);
        }
        return aiGetSpellValue(owner, spell);
    }

    case ARTIFACT_HOLY_GRAIL:
    case ARTIFACT_CATAPULT:
        break;

    case ARTIFACT_BALLISTA: {
        value = static_cast<long>(
            sqrt(static_cast<double>(owner->getPrimarySkill(0) + 1))
            * 500.0);
        value += value * owner->m_skillLevel[20] / 2;
        long armyValue =
            owner->m_army.getAIValue()
            * (const_cast<Hero*>(owner)->getPrimarySkillTotal() + 40)
            / 40;
        return armyValue * value / (armyValue + value);
    }

    case ARTIFACT_AMMO_CART: {
        for (int i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
            CreatureType creature = owner->m_army.m_armyTypes[i];
            if (creature != CREATURE_NONE
                && (g_creatureTypeTraits[creature].m_attributes & g_ctaShooter)) {
                value += g_creatureTypeTraits[creature].m_aiValue
                         * owner->m_army.m_numTroops[i] / 40;
            }
        }
        return value;
    }

    case ARTIFACT_FIRST_AID_TENT: {
        int firstAid = static_cast<int>(
            owner->getFirstAidFactor() * 25.0f);
        for (int i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
            CreatureType creature = owner->m_army.m_armyTypes[i];
            if (creature != CREATURE_NONE) {
                const CreatureTypeTraits& traits =
                    g_creatureTypeTraits[creature];
                if (firstAid >= traits.m_hitPoints)
                    value = cppMax(value,
                                     static_cast<long>(traits.m_aiValue));
                else
                    value = cppMax(
                        value, static_cast<long>(
                                   traits.m_aiValue * firstAid
                                   / traits.m_hitPoints));
            }
        }
        return value;
    }
    }

    signed char victoryType = g_game->m_mapHeader.m_victoryCondition.m_type;
    if ((victoryType == VICTORY_CONDITION_ARTIFACT
         || victoryType == VICTORY_CONDITION_TRANSPORT_ARTIFACT)
        && g_game->m_mapHeader.m_victoryCondition.m_artifactNum
               == artifact.m_artifactId) {
        value = 1968;
    }

    for (unsigned int i = 0;
         i < g_constArtifactEffects[artifact.m_artifactId].size(); ++i) {
        value += g_constArtifactEffects[artifact.m_artifactId][i]->getValue(
            owner, equipped, exact);
    }

    int combination = g_artifactTraits[artifact.m_artifactId].m_comboType;
    if (combination != -1) {
        for (int component = 0; component < 144; ++component) {
            if (g_combinationArtifacts[combination].m_components[component]) {
                std::vector<ArtifactEffect*>::iterator effect =
                    g_constArtifactEffects[component].begin();
                while (effect != g_constArtifactEffects[component].end()) {
                    value += (*effect)->getValue(owner, equipped, exact);
                    ++effect;
                }
            }
        }
    }
    return value;
}

VA(0x004339e0, 0xb8)  // dc 0x37464
long aiGetEquipValue(type_artifact artifact, const Hero* ourHero,
                        unsigned char exact)
{
    int slot;
    for (slot = 0; slot < 19; ++slot) {
        if (const_cast<Hero*>(ourHero)->heroFn004E2550(
                artifact.m_artifactId, slot)) {
            break;
        }
    }

    long value = cppMax(
        aiGetValueOfArtifact(artifact, ourHero, 0, exact), 0L);
    if (slot >= 19) {
        long replacedValue = 0;
        for (int equippedSlot = 0; equippedSlot < 19;
             ++equippedSlot) {
            if (const_cast<Hero*>(ourHero)->heroFn004E2840(
                    artifact.m_artifactId, equippedSlot)) {
                replacedValue = aiGetValueOfArtifact(
                    ourHero->getArtifact(ArtifactSlot(equippedSlot)), ourHero, 1, exact);
            }
        }
        value = cppMax(0L, value - replacedValue);
    }
    return value;
}

VA(0x00433aa0, 0x9e)  // dc 0x37514
long aiGetValueOfArtifact(const type_artifact& artifact, long playerId)
{
    if (artifact.m_artifactId == -1)
        return 0;
    PlayerData* player = &g_game->m_players[playerId];
    long best = 10;
    for (int i = 0; i < player->m_numHeroes; ++i) {
        Hero* bestHero = g_game->getHero(player->m_heroes[i]);
        long value = aiGetEquipValue(artifact, bestHero, 0);
        if (value > best)
            best = value;
    }
    return best;
}

long removeNegativeArtifacts(Hero* ourHero);
unsigned char addArtifact(Hero* ourHero, type_artifact artifact,
                           long* baseValue, Hero* sourceHero,
                           long sourceSlot, long* sourceValue,
                           long bestChange);

VA(0x00433b40, 0x6d)  // dc 0x37a58
void aiEquipArtifacts(Hero* ourHero)
{
    long baseValue = removeNegativeArtifacts(ourHero);
    type_artifact artifact;
    int backpackSlot = ourHero->getLastBackpackIndex() + 1;
    while (backpackSlot-- > 0) {
        artifact = ourHero->getBackpack(backpackSlot);
        if (artifact.m_artifactId != ARTIFACT_NONE
            && addArtifact(ourHero, artifact, &baseValue, 0, 19, 0, 0)) {
            ourHero->removeBackpackArtifact(backpackSlot);
        }
    }
}

VA(0x00433bb0, 0xad)  // dc 0x377f0
long removeNegativeArtifacts(Hero* ourHero)
{
    type_artifact artifact;
    long bestValue = getFullValue(ourHero);
    if (ourHero->getNumberInBackpack(1) >= HERO_BACKPACK_CAPACITY)
        return bestValue;

    for (int slot = 0; slot < 17; ++slot) {
        artifact = ourHero->getArtifact(ArtifactSlot(slot));
        if (artifact.m_artifactId != ARTIFACT_NONE) {
            ourHero->removeArtifact(slot);
            long value = getFullValue(ourHero);
            if (value <= bestValue) {
                ourHero->equipArtifact(&artifact, slot);
            } else {
                ourHero->addToBackpack(&artifact, -1);
                bestValue = value;
                if (ourHero->getNumberInBackpack(1)
                    >= HERO_BACKPACK_CAPACITY) {
                    return bestValue;
                }
            }
        }
    }
    return bestValue;
}

VA(0x00433c60, 0x1b3)  // dc 0x37588
long getFullValue(const Hero* ourHero)
{
    Spellvalue caster(ourHero);
    long value = 0;

    for (int i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
        CreatureType creature = ourHero->m_army.m_armyTypes[i];
        if (creature != CREATURE_NONE) {
            unsigned char hasAlliance;
            if (ourHero->m_owner >= 0)
                hasAlliance = g_game->m_players[ourHero->m_owner]
                                  .hasGivenArtifact(ARTIFACT_ANGELIC_ALLIANCE);
            else
                hasAlliance = ourHero->isWieldingArtifact(
                    ARTIFACT_ANGELIC_ALLIANCE);

            long morale = ourHero->m_army.getArmyMorale(
                i, ourHero, 0, -1, hasAlliance, 1);
            long luck = ourHero->m_army.getArmyLuck(
                i, ourHero, 0, -1, 1);
            value = static_cast<long>(
                (aiValueOfMorale(0, morale) + 1.0) *
                    (aiValueOfLuck(0, luck) + 1.0) *
                    (g_creatureTypeTraits[creature].m_aiValue *
                     ourHero->m_army.m_numTroops[i]) +
                value);
        }
    }

    value = static_cast<long>(
        value * ourHero->getCombatValueModifier());
    value += caster.getBestSpellValue(
        SPELL_VALUE_CLASS_MASK ^ SPELL_VALUE_SPECIAL);
    value += caster.getBestSpellValue(SPELL_VALUE_SPECIAL);

    for (int slot = 0; slot < 19; ++slot) {
        type_artifact artifact = ourHero->getArtifact(ArtifactSlot(slot));
        if (artifact.m_artifactId != -1)
            value += aiGetValueOfArtifact(artifact, ourHero, 1, 1);
    }
    return value;
}

VA(0x00433e20, 0x1bf)  // dc 0x37898
unsigned char addArtifact(Hero* ourHero, type_artifact artifact,
                           long* baseValue, Hero* sourceHero,
                           long sourceSlot, long* sourceValue,
                           long bestChange)
{
    if (ourHero->getNumberInBackpack(1) >= HERO_BACKPACK_CAPACITY)
        return 0;

    type_artifact oldArtifact;
    int bestSlot = HeroScreenWindow::ARTIFACT_SLOT_COUNT;
    long bestValue;
    long bestSourceValue;
    unsigned char bestIsSwap;
    long newSourceValue = 0;

    for (int slot = 0; slot < 17; ++slot) {
        if (!ourHero->heroFn004E2840(artifact.m_artifactId, slot))
            continue;

        oldArtifact = ourHero->getArtifact(ArtifactSlot(slot));
        long value = 0;
        unsigned char isSwap = 0;
        if (sourceValue)
            newSourceValue = *sourceValue;

        if (oldArtifact.m_artifactId != ARTIFACT_NONE) {
            ourHero->removeArtifact(slot);
            if (sourceHero &&
                sourceHero->heroFn004E2840(oldArtifact.m_artifactId,
                                             sourceSlot)) {
                sourceHero->equipArtifact(&oldArtifact, sourceSlot);
                newSourceValue = getFullValue(sourceHero);
                if (newSourceValue > *sourceValue) {
                    value = newSourceValue - *sourceValue;
                    isSwap = 1;
                }
                sourceHero->removeArtifact(sourceSlot);
            }
        }

        ourHero->equipArtifact(&artifact, slot);
        long newValue = getFullValue(ourHero);
        value += newValue - *baseValue;
        ourHero->removeArtifact(slot);
        if (oldArtifact.m_artifactId != ARTIFACT_NONE)
            ourHero->equipArtifact(&oldArtifact, slot);

        if (value > bestChange) {
            bestValue = newValue;
            bestChange = value;
            bestSlot = slot;
            bestSourceValue = newSourceValue;
            bestIsSwap = isSwap;
        }
        if (oldArtifact.m_artifactId == ARTIFACT_NONE)
            break;
    }

    if (bestSlot == HeroScreenWindow::ARTIFACT_SLOT_COUNT)
        return 0;

    oldArtifact = ourHero->getArtifact(ArtifactSlot(bestSlot));
    if (oldArtifact.m_artifactId != ARTIFACT_NONE) {
        ourHero->removeArtifact(bestSlot);
        if (bestIsSwap) {
            sourceHero->equipArtifact(&oldArtifact, sourceSlot);
            *sourceValue = bestSourceValue;
        } else {
            ourHero->addToBackpack(&oldArtifact, -1);
        }
    }
    ourHero->equipArtifact(&artifact, bestSlot);
    *baseValue = bestValue;
    return 1;
}

VA(0x00433fe0, 0xf5)  // dc 0x37acc
void aiSwapArtifacts(Hero* source, Hero* dest)
{
    type_artifact artifact;
    long sourceValue = removeNegativeArtifacts(source);
    long destValue = removeNegativeArtifacts(dest);

    for (int slot = 0; slot < 17; ++slot) {
        artifact = source->getArtifact(ArtifactSlot(slot));
        if (artifact.m_artifactId != ARTIFACT_NONE) {
            source->removeArtifact(slot);
            long newSourceValue = getFullValue(source);
            if (addArtifact(dest, artifact, &destValue, source, slot,
                             &newSourceValue,
                             sourceValue - newSourceValue)) {
                sourceValue = newSourceValue;
            } else {
                source->equipArtifact(&artifact, -1);
            }
        }
    }

    int backpackSlot = source->getLastBackpackIndex() + 1;
    while (backpackSlot-- > 0) {
        artifact = source->getBackpack(backpackSlot);
        if (artifact.m_artifactId != ARTIFACT_NONE
            && addArtifact(dest, artifact, &destValue, 0, 19, 0, 0)) {
            source->removeBackpackArtifact(backpackSlot);
        }
    }
}

// Complete keeps the DC artifact-effect class boundaries but expands these
// tiny construction helpers into initialize_artifact_effects. DC also proves
// the explicit type_artifact_effect default constructor (line 5043); spelling
// it here is byte-flat but preserves that real source/inliner boundary.
// Retail retains type_combat_artifact at four later nested-base sites.
inline ArtifactEffect::ArtifactEffect()
{
}

inline ScoutingArtifact::ScoutingArtifact(long newBonus)
    : m_bonus(newBonus)
{
}

inline MightArtifact::MightArtifact(long newBonus)
    : CombatArtifact(newBonus)
{
}

inline PowerArtifact::PowerArtifact(long newBonus)
    : CombatArtifact(newBonus)
{
}

inline KnowledgeArtifact::KnowledgeArtifact(long newBonus)
    : CombatArtifact(newBonus)
{
}

inline NecromancyArtifact::NecromancyArtifact(long newBonus)
    : CombatArtifact(newBonus)
{
}

inline MovementArtifact::MovementArtifact(long newBonus)
    : CombatArtifact(newBonus)
{
}

inline SpellcasterArtifact::SpellcasterArtifact(long newBonus)
    : CombatArtifact(newBonus)
{
}

inline MoraleArtifact::MoraleArtifact(long newBonus)
    : CombatArtifact(newBonus)
{
}

inline LuckArtifact::LuckArtifact(long newBonus)
    : CombatArtifact(newBonus)
{
}

inline DurationArtifact::DurationArtifact(long newBonus)
    : PowerArtifact(newBonus)
{
}

inline SchoolArtifact::SchoolArtifact(SpellSchool newSchool,
                                                   long newBonus)
    : PowerArtifact(newBonus), m_school(newSchool)
{
}

inline AntimagicArtifact::AntimagicArtifact(long maxLevel)
    : m_bonus(maxLevel)
{
}

inline AntimoraleArtifact::AntimoraleArtifact()
{
}

inline AntiluckArtifact::AntiluckArtifact()
{
}

inline TomeArtifact::TomeArtifact(SpellSchool newSchool)
    : CombatArtifact(0), m_school(newSchool)
{
}

inline IncomeArtifact::IncomeArtifact(
    long newAmount, GameResource newResource)
    : m_amount(newAmount), m_resource(newResource)
{
}

inline CreatureGrowthArtifact::CreatureGrowthArtifact(
    long newLevel, long newBonus)
    : m_bonus(newLevel), m_growthBonus(newBonus)
{
}

inline UndeadKingCloakArtifact::UndeadKingCloakArtifact()
    : NecromancyArtifact(30)
{
}

inline SpellArtifact::SpellArtifact(SpellID newSpell)
    : m_spell(newSpell)
{
}

inline ShooterBonusArtifact::ShooterBonusArtifact(long newBonus)
    : CombatArtifact(newBonus)
{
}

inline AngelicAllianceArtifact::AngelicAllianceArtifact()
    : MightArtifact(8)
{
}

inline ElixirOfLifeArtifact::ElixirOfLifeArtifact()
{
}

inline StatueOfLegionArtifact::StatueOfLegionArtifact()
{
}

static void initializeArtifactEffects();

VA(0x004340e0, 0x20)  // dc 0x37c38
void aiInitialize()
{
    for (short i = 0; i < 8; ++i)
        g_aiPlayers[i].init(i);
    initializeArtifactEffects();
}

// Retail .rdata 0x63ac7c is a sentinel-delimited stream: artifact id,
// effect-kind/arguments..., -1, repeated, and a final -1.  The switch and
// every argument-consumption site are directly visible in the separately
// framed 0x434100 body. DC names the same helper at 0x35f08 and places a
// vector::clear before the inner loop; restoring that statement raises this
// body from 86.90% to 91.95%. Flattening the helper into AI_initialize
// measured 84.09% only because the old retail inventory incorrectly
// coalesced both functions, and is the boundary negative control. The one
// remaining extra CFG block is a retained type_combat_artifact call in our
// necromancy arm; inline_depth(255) and force-inlining the outer constructor
// were byte-flat, while inline-qualifying the shared base over-expanded later
// retail call sites and fell to 76.42%, so no synthetic control is retained.

// 91.95 -> 95.16 (polish 49, lever A/C census): retail NEVER folds the two
// stream reads of a two-argument arm into one pointer bump. The SCHOOL,
// INCOME and CREATURE_GROWTH arms each show `mov reg,[esi] / add esi,4 /
// call operator new / mov reg2,[esi] / add esi,4` (0x434dd1..0x434df6 for
// CREATURE_GROWTH) - the first datum is read BEFORE `operator new` and
// survives it in a callee-saved register, which only happens when the source
// binds it to a named local ahead of the new-expression. Our single
// `new T(*definition++, *definition++)` let CL merge both bumps into
// `add esi,8` at +0x222/+0x2dd/+0x305. Naming the first read restores all
// three sites and both esi bumps per arm.

// Residual (95.16%): ONE swapped inline decision - retail CALLS
// type_combat_artifact's ctor in the NECROMANCY arm (t+0x165) and expands
// it in SHOOTER_BONUS (t+0x37d); we do the exact opposite (+0x38c call,
// necromancy expanded). DURATION, SCHOOL and UNDEAD_KING_CLOAK keep their
// calls on both sides, so this is a per-site /Ob2 budget boundary, not a
// missing source element.
VA(0x00434100, 0x490)  // tail target/fresh frame + DC helper, dc 0x35f08
static void initializeArtifactEffects()
{
    const int* definition = g_aiArtifactEffectDefinitions;
    while (*definition >= 0) {
        std::vector<ArtifactEffect*>& effects =
            g_constArtifactEffects[*definition++];
        effects.clear();
        while (*definition >= 0) {
            ArtifactEffectKind kind = (ArtifactEffectKind)*definition++;
            ArtifactEffect* effect;
            switch (kind) {
                case ARTIFACT_EFFECT_MIGHT:
                    effect = new MightArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_POWER:
                    effect = new PowerArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_KNOWLEDGE:
                    effect = new KnowledgeArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_MORALE:
                    effect = new MoraleArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_LUCK:
                    effect = new LuckArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_SCOUTING:
                    effect = new ScoutingArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_NECROMANCY:
                    effect = new NecromancyArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_COMBAT:
                    effect = new CombatArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_MOVEMENT:
                    effect = new MovementArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_SPELLCASTER:
                    effect = new SpellcasterArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_DURATION:
                    effect = new DurationArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_SCHOOL: {
                    SpellSchool school = (SpellSchool)*definition++;
                    effect = new SchoolArtifact(
                        school, *definition++);
                    break;
                }
                case ARTIFACT_EFFECT_ANTIMAGIC:
                    effect = new AntimagicArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_ANTIMORALE:
                    effect = new AntimoraleArtifact;
                    break;
                case ARTIFACT_EFFECT_ANTILUCK:
                    effect = new AntiluckArtifact;
                    break;
                case ARTIFACT_EFFECT_TOME:
                    effect = new TomeArtifact(
                        (SpellSchool)*definition++);
                    break;
                case ARTIFACT_EFFECT_INCOME: {
                    long amount = *definition++;
                    effect = new IncomeArtifact(
                        amount, (GameResource)*definition++);
                    break;
                }
                case ARTIFACT_EFFECT_CREATURE_GROWTH: {
                    long level = *definition++;
                    effect = new CreatureGrowthArtifact(
                        level, *definition++);
                    break;
                }
                case ARTIFACT_EFFECT_SPELL:
                    effect = new SpellArtifact(
                        (SpellID)*definition++);
                    break;
                case ARTIFACT_EFFECT_SHOOTER_BONUS:
                    effect = new ShooterBonusArtifact(*definition++);
                    break;
                case ARTIFACT_EFFECT_ANGELIC_ALLIANCE:
                    effect = new AngelicAllianceArtifact;
                    break;
                case ARTIFACT_EFFECT_UNDEAD_KING_CLOAK:
                    effect = new UndeadKingCloakArtifact;
                    break;
                case ARTIFACT_EFFECT_ELIXIR_OF_LIFE:
                    effect = new ElixirOfLifeArtifact;
                    break;
                case ARTIFACT_EFFECT_STATUE_OF_LEGION:
                    effect = new StatueOfLegionArtifact;
                    break;
            }
            effects.push_back(effect);
        }
        ++definition;
    }
}

VA(0x00434590, 0x62)  // dc 0x37c70
void aiShutDown()
{
    for (int i = 0; i < 144; ++i) {
        for (unsigned int j = 0; j < g_constArtifactEffects[i].size(); ++j)
            delete g_constArtifactEffects[i][j];
        g_constArtifactEffects[i].clear();
    }
}

#if 0  // @carcass

// E:\gamedcs\game.h:1410
DC_ONLY(0x3803c, 0x28)
unsigned char Game::townAlreadyBuiltOn(int id)
{
    // @stub
}

// E:\gamedcs\mousemgr.h:221
DC_ONLY(0x38064, 0xA)
unsigned char MouseManager::isVis()
{
    // @stub
}

// E:\gamedcs\netmsg.h:828
DC_ONLY(0x38070, 0x38)
void CGiftMsg::CGiftMsg(int niceGuy, int resource, int qty)
{
    // @stub
}

// E:\gamedcs\netmsg.h:842
DC_ONLY(0x380a8, 0x30)
void CGiftRequestMsg::CGiftRequestMsg(int greedyGuy, int resource)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:224
DC_ONLY(0x380d8, 0x18)
void AICreaturePurchaser::~AICreaturePurchaser()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:3213
DC_ONLY(0x380f0, 0x28)
void HeroDestination::HeroDestination()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:4924
DC_ONLY(0x38118, 0x34)
void std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::`default constructor closure'()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:4924
DC_ONLY(0x3814c, 0x38)
void `vector destructor iterator'(void* __t, unsigned __s, int __n, void (*)()* __f)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5044
DC_ONLY(0x38184, 0x34)
void* ArtifactEffect::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5059
DC_ONLY(0x381b8, 0x34)
void* ScoutingArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5059
DC_ONLY(0x381ec, 0x18)
void ScoutingArtifact::~ScoutingArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5075
DC_ONLY(0x38204, 0x34)
void* CombatArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5075
DC_ONLY(0x38238, 0x18)
void CombatArtifact::~CombatArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5091
DC_ONLY(0x38250, 0x34)
void* MightArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5091
DC_ONLY(0x38284, 0x18)
void MightArtifact::~MightArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5110
DC_ONLY(0x3829c, 0x34)
void* PowerArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5110
DC_ONLY(0x382d0, 0x18)
void PowerArtifact::~PowerArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5128
DC_ONLY(0x382e8, 0x34)
void* KnowledgeArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5128
DC_ONLY(0x3831c, 0x18)
void KnowledgeArtifact::~KnowledgeArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5146
DC_ONLY(0x38334, 0x34)
void* NecromancyArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5146
DC_ONLY(0x38368, 0x18)
void NecromancyArtifact::~NecromancyArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5183
DC_ONLY(0x38380, 0x34)
void* MovementArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5183
DC_ONLY(0x383b4, 0x18)
void MovementArtifact::~MovementArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5199
DC_ONLY(0x383cc, 0x34)
void* SpellcasterArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5199
DC_ONLY(0x38400, 0x18)
void SpellcasterArtifact::~SpellcasterArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5220
DC_ONLY(0x38418, 0x34)
void* MoraleArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5220
DC_ONLY(0x3844c, 0x18)
void MoraleArtifact::~MoraleArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5244
DC_ONLY(0x38464, 0x34)
void* LuckArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5244
DC_ONLY(0x38498, 0x18)
void LuckArtifact::~LuckArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5268
DC_ONLY(0x384b0, 0x34)
void* DurationArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5268
DC_ONLY(0x384e4, 0x18)
void DurationArtifact::~DurationArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5287
DC_ONLY(0x384fc, 0x34)
void* SchoolArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5287
DC_ONLY(0x38530, 0x18)
void SchoolArtifact::~SchoolArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5349
DC_ONLY(0x38548, 0x34)
void* AntimagicArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5349
DC_ONLY(0x3857c, 0x18)
void AntimagicArtifact::~AntimagicArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5381
DC_ONLY(0x38594, 0x34)
void* AntimoraleArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5381
DC_ONLY(0x385c8, 0x18)
void AntimoraleArtifact::~AntimoraleArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5407
DC_ONLY(0x385e0, 0x34)
void* AntiluckArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5407
DC_ONLY(0x38614, 0x18)
void AntiluckArtifact::~AntiluckArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5435
DC_ONLY(0x3862c, 0x34)
void* TomeArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5435
DC_ONLY(0x38660, 0x18)
void TomeArtifact::~TomeArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5480
DC_ONLY(0x38678, 0x34)
void* IncomeArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5480
DC_ONLY(0x386ac, 0x18)
void IncomeArtifact::~IncomeArtifact()
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5498
DC_ONLY(0x386c4, 0x34)
void* CreatureGrowthArtifact::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\ai_player.cpp:5498
DC_ONLY(0x386f8, 0x18)
void CreatureGrowthArtifact::~CreatureGrowthArtifact()
{
    // @stub
}

// ..\stlport\stl_string.h:530
DC_ONLY(0x38710, 0x18)
unsigned std::basic_string<char,std::char_traits<char>,std::allocator<char> >::length()
{
    // @stub
}

// ..\stlport\stl_bitset.h:565
DC_ONLY(0x38728, 0x18)
unsigned char std::bitset<18,unsigned long>::operator[](unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x38740, 0x20)
unsigned std::vector<CreatureSource,std::allocator<CreatureSource> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x38760, 0x24)
CreatureSource* std::vector<CreatureSource,std::allocator<CreatureSource> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x38784, 0x1C)
void std::vector<CreatureSource,std::allocator<CreatureSource> >::vector<CreatureSource,std::allocator<CreatureSource> >(const std::allocator<CreatureSource>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x387a0, 0x28)
void std::vector<CreatureSource,std::allocator<CreatureSource> >::~vector<CreatureSource,std::allocator<CreatureSource> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x387c8, 0x3C)
void std::vector<CreatureSource,std::allocator<CreatureSource> >::push_back(const CreatureSource* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0x38804, 0x38)
void std::vector<CreatureSource,std::allocator<CreatureSource> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x3883c, 0x4)
void std::allocator<CreatureSource>::allocator<CreatureSource>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x38840, 0x4)
void std::allocator<CreatureSource>::~allocator<CreatureSource>()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x38844, 0x4)
CreatureValue* std::vector<CreatureValue,std::allocator<CreatureValue> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0x38848, 0x4)
CreatureValue* std::vector<CreatureValue,std::allocator<CreatureValue> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x3884c, 0x20)
unsigned std::vector<CreatureValue,std::allocator<CreatureValue> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x3886c, 0x24)
CreatureValue* std::vector<CreatureValue,std::allocator<CreatureValue> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x38890, 0x1C)
void std::vector<CreatureValue,std::allocator<CreatureValue> >::vector<CreatureValue,std::allocator<CreatureValue> >(const std::allocator<CreatureValue>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:230
DC_ONLY(0x388ac, 0x50)
void std::vector<CreatureValue,std::allocator<CreatureValue> >::vector<CreatureValue,std::allocator<CreatureValue> >(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x388fc, 0x3C)
void std::vector<CreatureValue,std::allocator<CreatureValue> >::push_back(const CreatureValue* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0x38938, 0x38)
void std::vector<CreatureValue,std::allocator<CreatureValue> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x38970, 0x4)
void std::allocator<CreatureValue>::allocator<CreatureValue>()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x38974, 0xC)
unsigned std::vector<PathCell,std::allocator<PathCell> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x38980, 0x20)
PathCell* std::vector<PathCell,std::allocator<PathCell> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x389a0, 0x1C)
void std::vector<PathCell,std::allocator<PathCell> >::vector<PathCell,std::allocator<PathCell> >(const std::allocator<PathCell>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x389bc, 0x28)
void std::vector<PathCell,std::allocator<PathCell> >::~vector<PathCell,std::allocator<PathCell> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x389e4, 0x3C)
void std::vector<PathCell,std::allocator<PathCell> >::push_back(const PathCell* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0x38a20, 0x38)
void std::vector<PathCell,std::allocator<PathCell> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x38a58, 0x4)
void std::allocator<PathCell>::allocator<PathCell>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x38a5c, 0x4)
void std::allocator<PathCell>::~allocator<PathCell>()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x38a60, 0x20)
PathCell** std::vector<PathCell *,std::allocator<PathCell *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x38a80, 0x1C)
void std::vector<PathCell *,std::allocator<PathCell *> >::vector<PathCell *,std::allocator<PathCell *> >(const std::allocator<PathCell* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x38a9c, 0x28)
void std::vector<PathCell *,std::allocator<PathCell *> >::~vector<PathCell *,std::allocator<PathCell *> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x38ac4, 0x3C)
void std::vector<PathCell *,std::allocator<PathCell *> >::push_back(PathCell** __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x38b00, 0x4)
void std::allocator<PathCell *>::allocator<PathCell *>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x38b04, 0x4)
void std::allocator<PathCell *>::~allocator<PathCell *>()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x38b08, 0xC)
unsigned std::vector<type_point,std::allocator<type_point> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:204
DC_ONLY(0x38b14, 0x20)
const type_point* std::vector<type_point,std::allocator<type_point> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:204
DC_ONLY(0x38b34, 0x24)
const Town* std::vector<Town,std::allocator<Town> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:365
DC_ONLY(0x38b58, 0x18)
long* std::vector<long,std::allocator<long> >::back()
{
    // @stub
}

// ..\stlport\stl_vector.h:472
DC_ONLY(0x38b70, 0x18)
void std::vector<long,std::allocator<long> >::insert(long* __pos, unsigned __n, const long* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:480
DC_ONLY(0x38b88, 0x4C)
long* std::vector<long,std::allocator<long> >::erase(long* __position)
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x38bd4, 0x20)
DialogResource* std::vector<DialogResource,std::allocator<DialogResource> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x38bf4, 0xC)
unsigned std::vector<HeroDestination,std::allocator<HeroDestination> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x38c00, 0x20)
HeroDestination* std::vector<HeroDestination,std::allocator<HeroDestination> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x38c20, 0x1C)
void std::vector<HeroDestination,std::allocator<HeroDestination> >::vector<HeroDestination,std::allocator<HeroDestination> >(const std::allocator<HeroDestination>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:230
DC_ONLY(0x38c3c, 0x5C)
void std::vector<HeroDestination,std::allocator<HeroDestination> >::vector<HeroDestination,std::allocator<HeroDestination> >(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x38c98, 0x28)
void std::vector<HeroDestination,std::allocator<HeroDestination> >::~vector<HeroDestination,std::allocator<HeroDestination> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x38cc0, 0x3C)
void std::vector<HeroDestination,std::allocator<HeroDestination> >::push_back(const HeroDestination* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x38cfc, 0x4)
void std::allocator<HeroDestination>::allocator<HeroDestination>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x38d00, 0x4)
void std::allocator<HeroDestination>::~allocator<HeroDestination>()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x38d04, 0xC)
unsigned std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x38d10, 0x20)
ArtifactEffect** std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:204
DC_ONLY(0x38d30, 0x20)
ArtifactEffect** std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x38d50, 0x1C)
void std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >(const std::allocator<ArtifactEffect* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x38d6c, 0x28)
void std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::~vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x38d94, 0x3C)
void std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::push_back(ArtifactEffect** __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0x38dd0, 0x38)
void std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x38e08, 0x4)
void std::allocator<ArtifactEffect *>::allocator<ArtifactEffect *>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x38e0c, 0x4)
void std::allocator<ArtifactEffect *>::~allocator<ArtifactEffect *>()
{
    // @stub
}

// ..\stlport\stl_bitset.h:507
DC_ONLY(0x38e10, 0x38)
unsigned char std::bitset<18,unsigned long>::_Unchecked_test(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x38e48, 0x4)
CreatureSource* std::vector<CreatureSource,std::allocator<CreatureSource> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0x38e4c, 0x4)
CreatureSource* std::vector<CreatureSource,std::allocator<CreatureSource> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0x38e50, 0x3C)
CreatureSource* std::vector<CreatureSource,std::allocator<CreatureSource> >::erase(CreatureSource* __first, CreatureSource* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x38e8c, 0x2C)
void std::_Vector_base<CreatureSource,std::allocator<CreatureSource> >::_Vector_base<CreatureSource,std::allocator<CreatureSource> >(const std::allocator<CreatureSource>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x38eb8, 0x40)
void std::_Vector_base<CreatureSource,std::allocator<CreatureSource> >::~_Vector_base<CreatureSource,std::allocator<CreatureSource> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0x38ef8, 0x3C)
CreatureValue* std::vector<CreatureValue,std::allocator<CreatureValue> >::erase(CreatureValue* __first, CreatureValue* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x38f34, 0x2C)
void std::_Vector_base<CreatureValue,std::allocator<CreatureValue> >::_Vector_base<CreatureValue,std::allocator<CreatureValue> >(const std::allocator<CreatureValue>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:94
DC_ONLY(0x38f60, 0x4C)
void std::_Vector_base<CreatureValue,std::allocator<CreatureValue> >::_Vector_base<CreatureValue,std::allocator<CreatureValue> >(unsigned __n, const std::allocator<CreatureValue>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x38fac, 0x4)
PathCell* std::vector<PathCell,std::allocator<PathCell> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0x38fb0, 0x4)
PathCell* std::vector<PathCell,std::allocator<PathCell> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0x38fb4, 0x3C)
PathCell* std::vector<PathCell,std::allocator<PathCell> >::erase(PathCell* __first, PathCell* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x38ff0, 0x2C)
void std::_Vector_base<PathCell,std::allocator<PathCell> >::_Vector_base<PathCell,std::allocator<PathCell> >(const std::allocator<PathCell>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x3901c, 0x30)
void std::_Vector_base<PathCell,std::allocator<PathCell> >::~_Vector_base<PathCell,std::allocator<PathCell> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x3904c, 0x2C)
void std::_Vector_base<PathCell *,std::allocator<PathCell *> >::_Vector_base<PathCell *,std::allocator<PathCell *> >(const std::allocator<PathCell* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x39078, 0x30)
void std::_Vector_base<PathCell *,std::allocator<PathCell *> >::~_Vector_base<PathCell *,std::allocator<PathCell *> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:180
DC_ONLY(0x390a8, 0x4)
const type_point* std::vector<type_point,std::allocator<type_point> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:180
DC_ONLY(0x390ac, 0x4)
const Town* std::vector<Town,std::allocator<Town> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x390b0, 0x4)
HeroDestination* std::vector<HeroDestination,std::allocator<HeroDestination> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x390b4, 0x2C)
void std::_Vector_base<HeroDestination,std::allocator<HeroDestination> >::_Vector_base<HeroDestination,std::allocator<HeroDestination> >(const std::allocator<HeroDestination>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:94
DC_ONLY(0x390e0, 0x4C)
void std::_Vector_base<HeroDestination,std::allocator<HeroDestination> >::_Vector_base<HeroDestination,std::allocator<HeroDestination> >(unsigned __n, const std::allocator<HeroDestination>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x3912c, 0x30)
void std::_Vector_base<HeroDestination,std::allocator<HeroDestination> >::~_Vector_base<HeroDestination,std::allocator<HeroDestination> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x3915c, 0x4)
ArtifactEffect** std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:180
DC_ONLY(0x39160, 0x4)
ArtifactEffect** std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0x39164, 0x4)
ArtifactEffect** std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0x39168, 0x3C)
ArtifactEffect** std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::erase(ArtifactEffect** __first, ArtifactEffect** __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x391a4, 0x2C)
void std::_Vector_base<ArtifactEffect *,std::allocator<ArtifactEffect *> >::_Vector_base<ArtifactEffect *,std::allocator<ArtifactEffect *> >(const std::allocator<ArtifactEffect* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x391d0, 0x30)
void std::_Vector_base<ArtifactEffect *,std::allocator<ArtifactEffect *> >::~_Vector_base<ArtifactEffect *,std::allocator<ArtifactEffect *> >()
{
    // @stub
}

// ..\stlport\stl_string.h:101
DC_ONLY(0x39200, 0x18)
void std::_STL_alloc_proxy<CreatureSource *,CreatureSource,std::allocator<CreatureSource> >::~_STL_alloc_proxy<CreatureSource *,CreatureSource,std::allocator<CreatureSource> >()
{
    // @stub
}

// ..\stlport\stl_string.h:101
DC_ONLY(0x39218, 0x18)
void std::_STL_alloc_proxy<PathCell *,PathCell,std::allocator<PathCell> >::~_STL_alloc_proxy<PathCell *,PathCell,std::allocator<PathCell> >()
{
    // @stub
}

// ..\stlport\stl_string.h:101
DC_ONLY(0x39230, 0x18)
void std::_STL_alloc_proxy<PathCell * *,PathCell *,std::allocator<PathCell *> >::~_STL_alloc_proxy<PathCell * *,PathCell *,std::allocator<PathCell *> >()
{
    // @stub
}

// ..\stlport\stl_string.h:101
DC_ONLY(0x39248, 0x18)
void std::_STL_alloc_proxy<HeroDestination *,HeroDestination,std::allocator<HeroDestination> >::~_STL_alloc_proxy<HeroDestination *,HeroDestination,std::allocator<HeroDestination> >()
{
    // @stub
}

// ..\stlport\stl_string.h:101
DC_ONLY(0x39260, 0x18)
void std::_STL_alloc_proxy<ArtifactEffect * *,ArtifactEffect *,std::allocator<ArtifactEffect *> >::~_STL_alloc_proxy<ArtifactEffect * *,ArtifactEffect *,std::allocator<ArtifactEffect *> >()
{
    // @stub
}

// ..\stlport\stl_bitset.h:125
DC_ONLY(0x39278, 0x20)
unsigned long std::_Base_bitset<1,unsigned long>::_M_getword(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x39298, 0xC)
void std::_STL_alloc_proxy<CreatureSource *,CreatureSource,std::allocator<CreatureSource> >::_STL_alloc_proxy<CreatureSource *,CreatureSource,std::allocator<CreatureSource> >(const std::allocator<CreatureSource>* __a, CreatureSource** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x392a4, 0x2C)
void std::_STL_alloc_proxy<CreatureSource *,CreatureSource,std::allocator<CreatureSource> >::deallocate(CreatureSource* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x392d0, 0xC)
void std::_STL_alloc_proxy<CreatureValue *,CreatureValue,std::allocator<CreatureValue> >::_STL_alloc_proxy<CreatureValue *,CreatureValue,std::allocator<CreatureValue> >(const std::allocator<CreatureValue>* __a, CreatureValue** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x392dc, 0x28)
CreatureValue* std::_STL_alloc_proxy<CreatureValue *,CreatureValue,std::allocator<CreatureValue> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x39304, 0xC)
void std::_STL_alloc_proxy<PathCell *,PathCell,std::allocator<PathCell> >::_STL_alloc_proxy<PathCell *,PathCell,std::allocator<PathCell> >(const std::allocator<PathCell>* __a, PathCell** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x39310, 0x2C)
void std::_STL_alloc_proxy<PathCell *,PathCell,std::allocator<PathCell> >::deallocate(PathCell* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x3933c, 0xC)
void std::_STL_alloc_proxy<PathCell * *,PathCell *,std::allocator<PathCell *> >::_STL_alloc_proxy<PathCell * *,PathCell *,std::allocator<PathCell *> >(const std::allocator<PathCell* __a, PathCell*** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x39348, 0x2C)
void std::_STL_alloc_proxy<PathCell * *,PathCell *,std::allocator<PathCell *> >::deallocate(PathCell** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x39374, 0xC)
void std::_STL_alloc_proxy<HeroDestination *,HeroDestination,std::allocator<HeroDestination> >::_STL_alloc_proxy<HeroDestination *,HeroDestination,std::allocator<HeroDestination> >(const std::allocator<HeroDestination>* __a, HeroDestination** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x39380, 0x28)
HeroDestination* std::_STL_alloc_proxy<HeroDestination *,HeroDestination,std::allocator<HeroDestination> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x393a8, 0x2C)
void std::_STL_alloc_proxy<HeroDestination *,HeroDestination,std::allocator<HeroDestination> >::deallocate(HeroDestination* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x393d4, 0xC)
void std::_STL_alloc_proxy<ArtifactEffect * *,ArtifactEffect *,std::allocator<ArtifactEffect *> >::_STL_alloc_proxy<ArtifactEffect * *,ArtifactEffect *,std::allocator<ArtifactEffect *> >(const std::allocator<ArtifactEffect* __a, ArtifactEffect*** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x393e0, 0x2C)
void std::_STL_alloc_proxy<ArtifactEffect * *,ArtifactEffect *,std::allocator<ArtifactEffect *> >::deallocate(ArtifactEffect** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x3940c, 0x20)
void std::allocator<CreatureSource>::deallocate(CreatureSource* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x3942c, 0x28)
CreatureValue* std::allocator<CreatureValue>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x39454, 0x1C)
void std::allocator<PathCell>::deallocate(PathCell* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x39470, 0x1C)
void std::allocator<PathCell *>::deallocate(PathCell** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x3948c, 0x28)
HeroDestination* std::allocator<HeroDestination>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x394b4, 0x1C)
void std::allocator<HeroDestination>::deallocate(HeroDestination* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x394d0, 0x1C)
void std::allocator<ArtifactEffect *>::deallocate(ArtifactEffect** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x394ec, 0xDC)
void std::vector<CreatureSource,std::allocator<CreatureSource> >::_M_insert_overflow(CreatureSource* __position, const CreatureSource* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x395c8, 0xDC)
void std::vector<CreatureValue,std::allocator<CreatureValue> >::_M_insert_overflow(CreatureValue* __position, const CreatureValue* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x396a4, 0xD0)
void std::vector<PathCell,std::allocator<PathCell> >::_M_insert_overflow(PathCell* __position, const PathCell* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x39774, 0xCC)
void std::vector<PathCell *,std::allocator<PathCell *> >::_M_insert_overflow(PathCell** __position, PathCell** __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:283
DC_ONLY(0x39840, 0xCC)
void std::vector<long,std::allocator<long> >::_M_fill_insert(long* __position, unsigned __n, const long* __x)
{
    // @stub
}

// ..\stlport\stl_algo.h:636
DC_ONLY(0x3990c, 0x70)
void std::sort(CreatureValue* __first, CreatureValue* __last, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algobase.h:132
DC_ONLY(0x3997c, 0xE)
const long* std::min(const long* __a, const long* __b)
{
    // @stub
}

// ..\stlport\stl_algo.h:624
DC_ONLY(0x3998c, 0x64)
void std::sort(CreatureValue* __first, CreatureValue* __last)
{
    // @stub
}

// ..\stlport\stl_algobase.h:137
DC_ONLY(0x399f0, 0xE)
const long* std::max(const long* __a, const long* __b)
{
    // @stub
}

// ..\stlport\stl_algo.h:624
DC_ONLY(0x39a00, 0x58)
void std::sort(long* __first, long* __last)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x39a58, 0xD0)
void std::vector<HeroDestination,std::allocator<HeroDestination> >::_M_insert_overflow(HeroDestination* __position, const HeroDestination* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x39b28, 0xCC)
void std::vector<ArtifactEffect *,std::allocator<ArtifactEffect *> >::_M_insert_overflow(ArtifactEffect** __position, ArtifactEffect** __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x39bf4, 0x30)
void std::destroy(CreatureSource* __first, CreatureSource* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x39c24, 0x30)
void std::construct(CreatureSource* __p, const CreatureSource* __value)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x39c54, 0x38)
CreatureValue* std::uninitialized_fill_n(CreatureValue* __first, unsigned __n, const CreatureValue* __x)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x39c8c, 0x30)
void std::construct(CreatureValue* __p, const CreatureValue* __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x39cbc, 0x30)
void std::destroy(PathCell* __first, PathCell* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x39cec, 0x44)
void std::construct(PathCell* __p, const PathCell* __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x39d30, 0x28)
void std::construct(PathCell** __p, PathCell** __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x39d58, 0x1C)
void std::destroy(long* __pointer)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x39d74, 0x38)
HeroDestination* std::uninitialized_fill_n(HeroDestination* __first, unsigned __n, const HeroDestination* __x)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x39dac, 0x30)
void std::destroy(HeroDestination* __first, HeroDestination* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x39ddc, 0x34)
void std::construct(HeroDestination* __p, const HeroDestination* __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x39e10, 0x30)
void std::destroy(ArtifactEffect** __first, ArtifactEffect** __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x39e40, 0x28)
void std::construct(ArtifactEffect** __p, ArtifactEffect** __value)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x39e68, 0x50)
CreatureSource* std::copy(CreatureSource* __first, CreatureSource* __last, CreatureSource* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x39eb8, 0x50)
CreatureValue* std::copy(CreatureValue* __first, CreatureValue* __last, CreatureValue* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x39f08, 0x50)
PathCell* std::copy(PathCell* __first, PathCell* __last, PathCell* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x39f58, 0x50)
ArtifactEffect** std::copy(ArtifactEffect** __first, ArtifactEffect** __last, ArtifactEffect** __result)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x39fa8, 0x4)
std::allocator<CreatureSource>* std::__stl_alloc_rebind(std::allocator<CreatureSource>* __a, const CreatureSource* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x39fac, 0x4)
std::allocator<PathCell>* std::__stl_alloc_rebind(std::allocator<PathCell>* __a, const PathCell* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x39fb0, 0x4)
std::allocator<PathCell* std::__stl_alloc_rebind(std::allocator<PathCell* __a, PathCell** __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x39fb4, 0x4)
std::allocator<HeroDestination>* std::__stl_alloc_rebind(std::allocator<HeroDestination>* __a, const HeroDestination* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x39fb8, 0x4)
std::allocator<ArtifactEffect* std::__stl_alloc_rebind(std::allocator<ArtifactEffect* __a, ArtifactEffect** __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x39fbc, 0x28)
CreatureSource* std::_STL_alloc_proxy<CreatureSource *,CreatureSource,std::allocator<CreatureSource> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x39fe4, 0x28)
PathCell* std::_STL_alloc_proxy<PathCell *,PathCell,std::allocator<PathCell> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x3a00c, 0x28)
PathCell** std::_STL_alloc_proxy<PathCell * *,PathCell *,std::allocator<PathCell *> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x3a034, 0x28)
ArtifactEffect** std::_STL_alloc_proxy<ArtifactEffect * *,ArtifactEffect *,std::allocator<ArtifactEffect *> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x3a05c, 0x28)
CreatureSource* std::allocator<CreatureSource>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x3a084, 0x28)
PathCell* std::allocator<PathCell>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x3a0ac, 0x24)
PathCell** std::allocator<PathCell *>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x3a0d0, 0x24)
ArtifactEffect** std::allocator<ArtifactEffect *>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x3a0f4, 0x38)
CreatureSource* std::uninitialized_copy(CreatureSource* __first, CreatureSource* __last, CreatureSource* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x3a12c, 0x38)
CreatureSource* std::uninitialized_fill_n(CreatureSource* __first, unsigned __n, const CreatureSource* __x)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x3a164, 0x38)
CreatureValue* std::uninitialized_copy(CreatureValue* __first, CreatureValue* __last, CreatureValue* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x3a19c, 0x38)
PathCell* std::uninitialized_copy(PathCell* __first, PathCell* __last, PathCell* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x3a1d4, 0x38)
PathCell* std::uninitialized_fill_n(PathCell* __first, unsigned __n, const PathCell* __x)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x3a20c, 0x38)
PathCell** std::uninitialized_copy(PathCell** __first, PathCell** __last, PathCell** __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x3a244, 0x38)
PathCell** std::uninitialized_fill_n(PathCell** __first, unsigned __n, PathCell** __x)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0x3a27c, 0x50)
long* std::copy_backward(long* __first, long* __last, long* __result)
{
    // @stub
}

// ..\stlport\stl_algobase.h:495
DC_ONLY(0x3a2cc, 0x16)
void std::fill(long* __first, long* __last, const long* __value)
{
    // @stub
}

// ..\stlport\stl_algo.c:1102
DC_ONLY(0x3a2e4, 0xB8)
void std::__introsort_loop(CreatureValue* __first, CreatureValue* __last, CreatureValue* __formal, int __depth_limit, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:1068
DC_ONLY(0x3a39c, 0x68)
void std::__final_insertion_sort(CreatureValue* __first, CreatureValue* __last, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:1081
DC_ONLY(0x3a404, 0xA4)
void std::__introsort_loop(CreatureValue* __first, CreatureValue* __last, CreatureValue* __formal, int __depth_limit)
{
    // @stub
}

// ..\stlport\stl_algo.c:1057
DC_ONLY(0x3a4a8, 0x5C)
void std::__final_insertion_sort(CreatureValue* __first, CreatureValue* __last)
{
    // @stub
}

// ..\stlport\stl_algo.c:1081
DC_ONLY(0x3a504, 0x8C)
void std::__introsort_loop(long* __first, long* __last, long* __formal, int __depth_limit)
{
    // @stub
}

// ..\stlport\stl_algo.c:1057
DC_ONLY(0x3a590, 0x48)
void std::__final_insertion_sort(long* __first, long* __last)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x3a5d8, 0x38)
HeroDestination* std::uninitialized_copy(HeroDestination* __first, HeroDestination* __last, HeroDestination* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x3a610, 0x38)
ArtifactEffect** std::uninitialized_copy(ArtifactEffect** __first, ArtifactEffect** __last, ArtifactEffect** __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x3a648, 0x38)
ArtifactEffect** std::uninitialized_fill_n(ArtifactEffect** __first, unsigned __n, ArtifactEffect** __x)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x3a680, 0x4)
CreatureSource* std::value_type(const CreatureSource* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x3a684, 0x1C)
void std::__destroy(CreatureSource* __first, CreatureSource* __last, CreatureSource* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x3a6a0, 0x1C)
CreatureValue* std::__uninitialized_fill_n(CreatureValue* __first, unsigned __n, const CreatureValue* __x, CreatureValue* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x3a6bc, 0x4)
PathCell* std::value_type(const PathCell* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x3a6c0, 0x1C)
void std::__destroy(PathCell* __first, PathCell* __last, PathCell* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:55
DC_ONLY(0x3a6dc, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x3a6e0, 0x4)
HeroDestination* std::value_type(const HeroDestination* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x3a6e4, 0x1C)
HeroDestination* std::__uninitialized_fill_n(HeroDestination* __first, unsigned __n, const HeroDestination* __x, HeroDestination* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x3a700, 0x1C)
void std::__destroy(HeroDestination* __first, HeroDestination* __last, HeroDestination* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x3a71c, 0x4)
ArtifactEffect** std::value_type(ArtifactEffect** __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x3a720, 0x1C)
void std::__destroy(ArtifactEffect** __first, ArtifactEffect** __last, ArtifactEffect** __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x3a73c, 0xC)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const CreatureSource* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x3a748, 0x4)
int* std::distance_type(const CreatureSource* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x3a74c, 0x48)
CreatureSource* std::__copy(CreatureSource* __first, CreatureSource* __last, CreatureSource* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x3a794, 0xC)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const CreatureValue* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x3a7a0, 0x4)
int* std::distance_type(const CreatureValue* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x3a7a4, 0x48)
CreatureValue* std::__copy(CreatureValue* __first, CreatureValue* __last, CreatureValue* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x3a7ec, 0xC)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const PathCell* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x3a7f8, 0x4)
int* std::distance_type(const PathCell* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x3a7fc, 0x4C)
PathCell* std::__copy(PathCell* __first, PathCell* __last, PathCell* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x3a848, 0xC)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, ArtifactEffect** __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x3a854, 0x4)
int* std::distance_type(ArtifactEffect** __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x3a858, 0x1E)
ArtifactEffect** std::__copy(ArtifactEffect** __first, ArtifactEffect** __last, ArtifactEffect** __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x3a878, 0x1C)
CreatureSource* std::__uninitialized_copy(CreatureSource* __first, CreatureSource* __last, CreatureSource* __result, CreatureSource* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x3a894, 0x1C)
CreatureSource* std::__uninitialized_fill_n(CreatureSource* __first, unsigned __n, const CreatureSource* __x, CreatureSource* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x3a8b0, 0x1C)
CreatureValue* std::__uninitialized_copy(CreatureValue* __first, CreatureValue* __last, CreatureValue* __result, CreatureValue* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x3a8cc, 0x1C)
PathCell* std::__uninitialized_copy(PathCell* __first, PathCell* __last, PathCell* __result, PathCell* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x3a8e8, 0x1C)
PathCell* std::__uninitialized_fill_n(PathCell* __first, unsigned __n, const PathCell* __x, PathCell* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x3a904, 0x1C)
PathCell** std::__uninitialized_copy(PathCell** __first, PathCell** __last, PathCell** __result, PathCell** __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x3a920, 0x1C)
PathCell** std::__uninitialized_fill_n(PathCell** __first, unsigned __n, PathCell** __x, PathCell** __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0x3a93c, 0x22)
long* std::__copy_backward(long* __first, long* __last, long* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_algo.h:677
DC_ONLY(0x3a960, 0x40)
void std::partial_sort(CreatureValue* __first, CreatureValue* __middle, CreatureValue* __last, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:65
DC_ONLY(0x3a9a0, 0x80)
const CreatureValue* std::__median(const CreatureValue* __a, const CreatureValue* __b, const CreatureValue* __c, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:941
DC_ONLY(0x3aa20, 0x74)
CreatureValue* std::__unguarded_partition(CreatureValue* __first, CreatureValue* __last, CreatureValue __pivot, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:1020
DC_ONLY(0x3aa94, 0x48)
void std::__insertion_sort(CreatureValue* __first, CreatureValue* __last, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:1050
DC_ONLY(0x3aadc, 0x34)
void std::__unguarded_insertion_sort(CreatureValue* __first, CreatureValue* __last, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algo.h:664
DC_ONLY(0x3ab10, 0x38)
void std::partial_sort(CreatureValue* __first, CreatureValue* __middle, CreatureValue* __last)
{
    // @stub
}

// ..\stlport\stl_algo.c:44
DC_ONLY(0x3ab48, 0x68)
const CreatureValue* std::__median(const CreatureValue* __a, const CreatureValue* __b, const CreatureValue* __c)
{
    // @stub
}

// ..\stlport\stl_algo.c:923
DC_ONLY(0x3abb0, 0x6C)
CreatureValue* std::__unguarded_partition(CreatureValue* __first, CreatureValue* __last, CreatureValue __pivot)
{
    // @stub
}

// ..\stlport\stl_algo.c:1012
DC_ONLY(0x3ac1c, 0x40)
void std::__insertion_sort(CreatureValue* __first, CreatureValue* __last)
{
    // @stub
}

// ..\stlport\stl_algo.c:1035
DC_ONLY(0x3ac5c, 0x30)
void std::__unguarded_insertion_sort(CreatureValue* __first, CreatureValue* __last)
{
    // @stub
}

// ..\stlport\stl_algo.h:664
DC_ONLY(0x3ac8c, 0x38)
void std::partial_sort(long* __first, long* __middle, long* __last)
{
    // @stub
}

// ..\stlport\stl_algo.c:44
DC_ONLY(0x3acc4, 0x3C)
const long* std::__median(const long* __a, const long* __b, const long* __c)
{
    // @stub
}

// ..\stlport\stl_algo.c:923
DC_ONLY(0x3ad00, 0x4C)
long* std::__unguarded_partition(long* __first, long* __last, long __pivot)
{
    // @stub
}

// ..\stlport\stl_algo.c:1012
DC_ONLY(0x3ad4c, 0x38)
void std::__insertion_sort(long* __first, long* __last)
{
    // @stub
}

// ..\stlport\stl_algo.c:1035
DC_ONLY(0x3ad84, 0x30)
void std::__unguarded_insertion_sort(long* __first, long* __last)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x3adb4, 0x1C)
HeroDestination* std::__uninitialized_copy(HeroDestination* __first, HeroDestination* __last, HeroDestination* __result, HeroDestination* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x3add0, 0x1C)
ArtifactEffect** std::__uninitialized_copy(ArtifactEffect** __first, ArtifactEffect** __last, ArtifactEffect** __result, ArtifactEffect** __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x3adec, 0x1C)
ArtifactEffect** std::__uninitialized_fill_n(ArtifactEffect** __first, unsigned __n, ArtifactEffect** __x, ArtifactEffect** __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x3ae08, 0x30)
void std::__destroy_aux(CreatureSource* __first, CreatureSource* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x3ae38, 0x3C)
CreatureValue* std::__uninitialized_fill_n_aux(CreatureValue* __first, unsigned __n, const CreatureValue* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x3ae74, 0x30)
void std::__destroy_aux(PathCell* __first, PathCell* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x3aea4, 0x3C)
HeroDestination* std::__uninitialized_fill_n_aux(HeroDestination* __first, unsigned __n, const HeroDestination* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x3aee0, 0x30)
void std::__destroy_aux(HeroDestination* __first, HeroDestination* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x3af10, 0x30)
void std::__destroy_aux(ArtifactEffect** __first, ArtifactEffect** __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_function.h:104
DC_ONLY(0x3af40, 0x18)
unsigned char std::greater<CreatureValue>::operator()(const CreatureValue* __x, const CreatureValue* __y)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x3af58, 0x3C)
CreatureSource* std::__uninitialized_copy_aux(CreatureSource* __first, CreatureSource* __last, CreatureSource* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x3af94, 0x3C)
CreatureSource* std::__uninitialized_fill_n_aux(CreatureSource* __first, unsigned __n, const CreatureSource* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x3afd0, 0x3C)
CreatureValue* std::__uninitialized_copy_aux(CreatureValue* __first, CreatureValue* __last, CreatureValue* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x3b00c, 0x3C)
PathCell* std::__uninitialized_copy_aux(PathCell* __first, PathCell* __last, PathCell* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x3b048, 0x3C)
PathCell* std::__uninitialized_fill_n_aux(PathCell* __first, unsigned __n, const PathCell* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x3b084, 0x3C)
PathCell** std::__uninitialized_copy_aux(PathCell** __first, PathCell** __last, PathCell** __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x3b0c0, 0x3C)
PathCell** std::__uninitialized_fill_n_aux(PathCell** __first, unsigned __n, PathCell** __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_algo.c:1443
DC_ONLY(0x3b0fc, 0xA8)
void std::__partial_sort(CreatureValue* __first, CreatureValue* __middle, CreatureValue* __last, CreatureValue* __formal, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algobase.h:107
DC_ONLY(0x3b1a4, 0x30)
void std::iter_swap(CreatureValue* __a, CreatureValue* __b)
{
    // @stub
}

// ..\stlport\stl_algo.c:1000
DC_ONLY(0x3b1d4, 0x64)
void std::__linear_insert(CreatureValue* __first, CreatureValue* __last, CreatureValue __val, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:1042
DC_ONLY(0x3b238, 0x44)
void std::__unguarded_insertion_sort_aux(CreatureValue* __first, CreatureValue* __last, CreatureValue* __formal, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algo.c:1432
DC_ONLY(0x3b27c, 0x90)
void std::__partial_sort(CreatureValue* __first, CreatureValue* __middle, CreatureValue* __last, CreatureValue* __formal)
{
    // @stub
}

// ..\stlport\stl_algo.c:987
DC_ONLY(0x3b30c, 0x58)
void std::__linear_insert(CreatureValue* __first, CreatureValue* __last, CreatureValue __val)
{
    // @stub
}

// ..\stlport\stl_algo.c:1028
DC_ONLY(0x3b364, 0x3C)
void std::__unguarded_insertion_sort_aux(CreatureValue* __first, CreatureValue* __last, CreatureValue* __formal)
{
    // @stub
}

// ..\stlport\stl_algo.c:1432
DC_ONLY(0x3b3a0, 0x70)
void std::__partial_sort(long* __first, long* __middle, long* __last, long* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:107
DC_ONLY(0x3b410, 0x30)
void std::iter_swap(long* __a, long* __b)
{
    // @stub
}

// ..\stlport\stl_algo.c:987
DC_ONLY(0x3b440, 0x40)
void std::__linear_insert(long* __first, long* __last, long __val)
{
    // @stub
}

// ..\stlport\stl_algo.c:1028
DC_ONLY(0x3b480, 0x30)
void std::__unguarded_insertion_sort_aux(long* __first, long* __last, long* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x3b4b0, 0x3C)
HeroDestination* std::__uninitialized_copy_aux(HeroDestination* __first, HeroDestination* __last, HeroDestination* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x3b4ec, 0x3C)
ArtifactEffect** std::__uninitialized_copy_aux(ArtifactEffect** __first, ArtifactEffect** __last, ArtifactEffect** __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x3b528, 0x3C)
ArtifactEffect** std::__uninitialized_fill_n_aux(ArtifactEffect** __first, unsigned __n, ArtifactEffect** __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x3b564, 0x1C)
void std::destroy(CreatureSource* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x3b580, 0x1C)
void std::destroy(PathCell* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x3b59c, 0x1C)
void std::destroy(HeroDestination* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x3b5b8, 0x1C)
void std::destroy(ArtifactEffect** __pointer)
{
    // @stub
}

// ..\stlport\stl_heap.c:235
DC_ONLY(0x3b5d4, 0x4C)
void std::make_heap(CreatureValue* __first, CreatureValue* __last, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_heap.h:85
DC_ONLY(0x3b620, 0x4C)
void std::__pop_heap(CreatureValue* __first, CreatureValue* __last, CreatureValue* __result, CreatureValue __value, std::greater<CreatureValue> __comp, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.h:118
DC_ONLY(0x3b66c, 0x44)
void std::sort_heap(CreatureValue* __first, CreatureValue* __last, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algobase.h:96
DC_ONLY(0x3b6b0, 0x18)
void std::__iter_swap(CreatureValue* __a, CreatureValue* __b, CreatureValue* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:442
DC_ONLY(0x3b6c8, 0x50)
CreatureValue* std::copy_backward(CreatureValue* __first, CreatureValue* __last, CreatureValue* __result)
{
    // @stub
}

// ..\stlport\stl_algo.c:974
DC_ONLY(0x3b718, 0x54)
void std::__unguarded_linear_insert(CreatureValue* __last, CreatureValue __val, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_heap.c:207
DC_ONLY(0x3b76c, 0x40)
void std::make_heap(CreatureValue* __first, CreatureValue* __last)
{
    // @stub
}

// ..\stlport\stl_heap.h:64
DC_ONLY(0x3b7ac, 0x44)
void std::__pop_heap(CreatureValue* __first, CreatureValue* __last, CreatureValue* __result, CreatureValue __value, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.h:108
DC_ONLY(0x3b7f0, 0x3C)
void std::sort_heap(CreatureValue* __first, CreatureValue* __last)
{
    // @stub
}

// ..\stlport\stl_algo.c:960
DC_ONLY(0x3b82c, 0x54)
void std::__unguarded_linear_insert(CreatureValue* __last, CreatureValue __val)
{
    // @stub
}

// ..\stlport\stl_heap.c:207
DC_ONLY(0x3b880, 0x40)
void std::make_heap(long* __first, long* __last)
{
    // @stub
}

// ..\stlport\stl_heap.h:64
DC_ONLY(0x3b8c0, 0x30)
void std::__pop_heap(long* __first, long* __last, long* __result, long __value, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.h:108
DC_ONLY(0x3b8f0, 0x38)
void std::sort_heap(long* __first, long* __last)
{
    // @stub
}

// ..\stlport\stl_algobase.h:96
DC_ONLY(0x3b928, 0x18)
void std::__iter_swap(long* __a, long* __b, long* __formal)
{
    // @stub
}

// ..\stlport\stl_algo.c:960
DC_ONLY(0x3b940, 0x16)
void std::__unguarded_linear_insert(long* __last, long __val)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x3b958, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x3b95c, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x3b960, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x3b964, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_heap.c:218
DC_ONLY(0x3b968, 0x8C)
void std::__make_heap(CreatureValue* __first, CreatureValue* __last, std::greater<CreatureValue> __comp, CreatureValue* __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.c:151
DC_ONLY(0x3b9f4, 0xCC)
void std::__adjust_heap(CreatureValue* __first, int __holeIndex, int __len, CreatureValue __value, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_heap.c:183
DC_ONLY(0x3bac0, 0x34)
void std::pop_heap(CreatureValue* __first, CreatureValue* __last, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_algobase.h:79
DC_ONLY(0x3baf4, 0x2A)
void std::swap(CreatureValue* __a, CreatureValue* __b)
{
    // @stub
}

// ..\stlport\stl_algobase.h:382
DC_ONLY(0x3bb20, 0x4C)
CreatureValue* std::__copy_backward(CreatureValue* __first, CreatureValue* __last, CreatureValue* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.c:192
DC_ONLY(0x3bb6c, 0x84)
void std::__make_heap(CreatureValue* __first, CreatureValue* __last, CreatureValue* __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.c:112
DC_ONLY(0x3bbf0, 0xC8)
void std::__adjust_heap(CreatureValue* __first, int __holeIndex, int __len, CreatureValue __value)
{
    // @stub
}

// ..\stlport\stl_heap.c:142
DC_ONLY(0x3bcb8, 0x30)
void std::pop_heap(CreatureValue* __first, CreatureValue* __last)
{
    // @stub
}

// ..\stlport\stl_heap.c:192
DC_ONLY(0x3bce8, 0x54)
void std::__make_heap(long* __first, long* __last, long* __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.c:112
DC_ONLY(0x3bd3c, 0x78)
void std::__adjust_heap(long* __first, int __holeIndex, int __len, long __value)
{
    // @stub
}

// ..\stlport\stl_heap.c:142
DC_ONLY(0x3bdb4, 0x30)
void std::pop_heap(long* __first, long* __last)
{
    // @stub
}

// ..\stlport\stl_algobase.h:79
DC_ONLY(0x3bde4, 0xA)
void std::swap(long* __a, long* __b)
{
    // @stub
}

// ..\stlport\stl_heap.c:78
DC_ONLY(0x3bdf0, 0x90)
void std::__push_heap(CreatureValue* __first, int __holeIndex, int __topIndex, CreatureValue __value, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_heap.c:173
DC_ONLY(0x3be80, 0x58)
void std::__pop_heap_aux(CreatureValue* __first, CreatureValue* __last, CreatureValue* __formal, std::greater<CreatureValue> __comp)
{
    // @stub
}

// ..\stlport\stl_heap.c:44
DC_ONLY(0x3bed8, 0x90)
void std::__push_heap(CreatureValue* __first, int __holeIndex, int __topIndex, CreatureValue __value)
{
    // @stub
}

// ..\stlport\stl_heap.c:134
DC_ONLY(0x3bf68, 0x50)
void std::__pop_heap_aux(CreatureValue* __first, CreatureValue* __last, CreatureValue* __formal)
{
    // @stub
}

// ..\stlport\stl_heap.c:44
DC_ONLY(0x3bfb8, 0x4A)
void std::__push_heap(long* __first, int __holeIndex, int __topIndex, long __value)
{
    // @stub
}

// ..\stlport\stl_heap.c:134
DC_ONLY(0x3c004, 0x38)
void std::__pop_heap_aux(long* __first, long* __last, long* __formal)
{
    // @stub
}

#endif  // @carcass

// COMDAT pairing: vector<type_creature_source>::size, agreement 1.000.
VA_COMPGEN(0x00434600, 0x20, VECTOR_SIZE, CreatureSource)

VA_COMPGEN(0x00434620, 0x23, VECTOR_SIZE, PathCell)

VA_COMPGEN(0x00434650, 0x26, VECTOR_DTOR, PathCell)

VA_COMPGEN(0x00434680, 0x4D, VECTOR_ERASE, CreatureSource)

VA_COMPGEN(0x00434ba0, 0x43, VECTOR_UCOPY, CreatureSource)

// The retained uninitialized fill copies the three dwords of each
// type_creature_source record.  The emitted loop agrees with all 58 bytes.
VA_COMPGEN(0x0054d580, 0x3A, VECTOR_UFILL, CreatureSource)

// COMDAT pairing: vector<pathCell>::_Ucopy, agreement 0.952; the
// type_creature_source arm scores 0.531 at operand level.
VA_COMPGEN(0x00434bf0, 0x3D, VECTOR_UCOPY, PathCell)

// COMDAT pairing: std::_Sort<type_creature_value, greater>, agreement 0.979.
// The default-predicate arm scores 0.932 here and 0.976 at 0x34e80, so the
// two instantiations separate cleanly.
VA_COMPGEN(0x00434ce0, 0x199, STD_SORT, type_creature_value_greater)

// COMDAT pairing: std::_Sort<type_creature_value>, agreement 0.976.
VA_COMPGEN(0x00434e80, 0x191, STD_SORT, CreatureValue)

// COMDAT pairing: std::_Sort<long>, agreement 0.995.
VA_COMPGEN(0x00435070, 0xCF, STD_SORT, long)

// COMDAT pairing: std::_Insertion_sort_1<type_creature_value, greater>,
// agreement 0.955 against 0.920 for the default-predicate arm.
VA_COMPGEN(0x00435160, 0xD5, INSERTION_SORT_1, type_creature_value_greater)

// COMDAT pairing: std::_Insertion_sort_1<type_creature_value>, agreement
// 0.955 against 0.920 for the greater arm.
VA_COMPGEN(0x00435310, 0xD5, INSERTION_SORT_1, CreatureValue)

// COMDAT pairing: std::_Insertion_sort_1<long>, agreement 0.926.
VA_COMPGEN(0x004353f0, 0x67, INSERTION_SORT_1, long)

// COMDAT pairing: std::_Unguarded_partition<type_creature_value, greater>,
// agreement 0.929.
VA_COMPGEN(0x00435290, 0x78, STD_UNGUARDED_PARTITION, type_creature_value_greater)

// COMDAT pairing: std::_Median<type_creature_value, greater>, agreement
// 0.971. Its element arrives BY VALUE, so the key is decoded from the
// return type rather than from a pointer parameter.
VA_COMPGEN(0x00435240, 0x4E, STD_MEDIAN, type_creature_value_greater)

// COMDAT pairing: bitset<144>::_Xran. All five surviving `_Xran` bodies in
// the image are byte-identical (same "invalid bitset<N> position" literal at
// 0x65f450), so neither similarity nor size can separate them - the BOUND
// COMPARE in each caller can. Every caller of 0x346d0 guards the call with
// `cmp <reg>, 0x90`, and one of them is the already-claimed
// bitset<144>::set at 0x4cf9a0.
VA_COMPGEN(0x004346d0, 0xCB, BITSET_XRAN, Bitset144)

// COMDAT pairing: bitset<9>::_Xran, by the same bound-compare lever: all four
// callers of 0x34ad0 guard with `cmp <reg>, 0x9`, among them armygrp's
// claimed bitset<9>::set (0x44c680) and bitset<9>::reference::operator=
// (0x44c610).
VA_COMPGEN(0x00434ad0, 0xCB, BITSET_XRAN, Bitset9)

VA_COMPGEN(0x00435020, 0x42, STD_UNGUARDED_INSERT, CreatureValue)

// COMDAT pairing: std::_Unguarded_insert<long>. Both parameters are register
// arguments under /Gr, so this one ends on a bare `ret`.
VA_COMPGEN(0x00435140, 0x1C, STD_UNGUARDED_INSERT, long)
