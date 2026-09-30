// CheckForTotalCreatures totals town garrisons through the NON-const
// town::get_army() half of the DC pair (the summoning-portal
// precedent: never defined, /OPT:ICF folded both bodies onto the const
// row 0x5c1460).
#include "va.h"

#include "game.h"

VA(0x005f15a0, 0x63)
DC_ADDRESS(0x18fdc4, 0x34)
MAC_ADDRESS(0x1fd30c, 0xc0)
int VictoryConditionStruct::appliesToPlayer(long playerId) const
{
    if (!m_appliesToComputer) {
        int team = g_game->getTeam(playerId);
        if (team >= 0) {
            int player = 0;
            signed char* teams = g_game->m_mapHeader.m_teamInfo;
            for (; player < 8; ++player) {
                if (teams[player] == team
                    && g_game->isHuman(player))
                    return 1;
            }
        }
        return 0;
    }
    return 1;
}

// The two campaign runs whose special victory assembles a combination
// from carried pieces. The ordinals are the engine campaign ids; the
// titles are provisional readings of the piece sets (7/1 collects the
// Armor of the Damned triple, 18/8 and 18/9 the two Angelic Alliance
// triples). File-scope named constants per the hero.cpp
// kStartLevelCampaign precedent.
static const int g_armorOfTheDamnedCampaign = 7;
static const int g_angelicAllianceCampaign = 18;
static const int g_angelicAllianceFirstMap = 8;
static const int g_angelicAllianceSecondMap = 9;

// Native retains the campaign base across the special artifact-vector path.
// The shared reference restores those +2/+4 member accesses; Windows remains
// 86.10% versus the prior86.11%, with exact siblings unchanged.
// DC line 41 names GetTeam and IsHumanTeam directly (no is_human_ally
// row), and the explicit team local reproduces retail's expanded
// guarded scan: 85.91 -> 99.59 over the isHumanAlly wrapper (2026-09-29).
// The SCampaign reference and TArtifact element type are Windows
// byte-flat against int&/signed char& and vector<int>. Residual: the
// bitset<144>::test range throw at the tail. `vc6 predict-inline --trace`
// gives test 153 at depth 1, _Xran 95, and the nested out_of_range ctor
// (cost 58) only 30, so it stays a call where retail expands it and stores
// out_of_range's vptr after logic_error's ctor; about 28 more depth-1
// budget is missing and no evidenced statement supplies it.
VA(0x005f1610, 0x4FE)
DC_ADDRESS(0x18fdf8, 0x9e)
MAC_ADDRESS(0x1fd3cc, 0x44c)
unsigned char VictoryConditionStruct::checkForArtifactWin()
{
    SCampaign& campaign = g_game->m_campaign;
    if ((campaign.m_currentCampaign == g_armorOfTheDamnedCampaign
            && campaign.m_currentMap == 1)
        || (campaign.m_currentCampaign == g_angelicAllianceCampaign
            && (campaign.m_currentMap
                    == g_angelicAllianceFirstMap
                || campaign.m_currentMap
                    == g_angelicAllianceSecondMap))) {
        if (!g_currentPlayer->isHuman())
            return 0;

        // Native keeps the non-POD vector placement/copy path and takes these
        // artifact constants by reference; the original type spelling is unknown.
        std::vector<TArtifact> pieces;
        pieces.reserve(3);
        if (campaign.m_currentCampaign == g_armorOfTheDamnedCampaign) {
            pieces.push_back(ARTIFACT_SWORD_OF_HELLFIRE);
            pieces.push_back(ARTIFACT_SHIELD_OF_THE_DAMNED);
            pieces.push_back(ARTIFACT_BREASTPLATE_OF_BRIMSTONE);
        } else if (campaign.m_currentMap == g_angelicAllianceFirstMap) {
            pieces.push_back(ARTIFACT_CELESTIAL_NECKLACE_OF_BLISS);
            pieces.push_back(ARTIFACT_SANDALS_OF_THE_SAINT);
            pieces.push_back(ARTIFACT_HELM_OF_HEAVENLY_ENLIGHTENMENT);
        } else {
            pieces.push_back(ARTIFACT_SWORD_OF_JUDGEMENT);
            pieces.push_back(ARTIFACT_ARMOR_OF_WONDER);
            pieces.push_back(ARTIFACT_LIONS_SHIELD_OF_COURAGE);
        }

        for (int i = 0; i < g_currentPlayer->m_numHeroes; ++i) {
            hero* h = g_game->getHero(g_currentPlayer->m_heroes[i]);
            for (std::vector<TArtifact>::iterator it = pieces.begin();
                 it != pieces.end();) {
                if (h->hasArtifact(*it))
                    it = pieces.erase(it);
                else
                    ++it;
            }
            if (pieces.empty()) {
                m_playerWinner = static_cast<signed char>(g_netLocalGamePos);
                m_gameWon = 1;
                return 1;
            }
        }
        return 0;
    }

    if (m_type != VICTORY_CONDITION_ARTIFACT
        || !g_currentPlayer
        || g_game->m_playerDisabled[g_netLocalGamePos])
        return 0;

    int team = g_game->getTeam(g_netLocalGamePos);
    if ((team >= 0 && g_game->isHumanTeam(team)) || m_appliesToComputer) {
        int j;
        for (j = 0; j < g_currentPlayer->m_numHeroes; ++j) {
            if (g_game->getHero(g_currentPlayer->m_heroes[j])
                    ->hasArtifact(m_artifactNum)) {
                m_playerWinner = static_cast<signed char>(g_netLocalGamePos);
                m_gameWon = 1;
                return 1;
            }
        }
        int comboIdx = g_artifactTraits[m_artifactNum].m_comboType;
        if (comboIdx == -1)
            return 0;

        const std::bitset<144>& components =
            g_combinationArtifacts[comboIdx].m_components;
        for (j = 0; j < g_currentPlayer->m_numHeroes; ++j) {
            int remaining = components.count();
            hero* h = g_game->getHero(g_currentPlayer->m_heroes[j]);
            for (int i = 0;; ++i) {
                int hasComponent = components.test(i);
                if (hasComponent) {
                    bool carriesComponent = h->hasArtifact(i);
                    if (!carriesComponent)
                        break;
                    if (--remaining == 0) {
                        m_playerWinner =
                            static_cast<signed char>(g_netLocalGamePos);
                        m_gameWon = 1;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

VA(0x005f1b10, 0x169)
DC_ADDRESS(0x18fe98, 0xea)
MAC_ADDRESS(0x1fd818, 0x1e4)
unsigned char VictoryConditionStruct::checkForTotalCreatures()
{
    if (m_type == VICTORY_CONDITION_TOTAL_CREATURES) {
        long total = 0;
        if (g_currentPlayer
            && !g_game->m_playerDisabled[g_netLocalGamePos]) {
            if (g_game->isHumanAlly(g_netLocalGamePos) || m_appliesToComputer) {
                int i;
                for (i = 0; i < g_currentPlayer->m_numHeroes; ++i)
                    total += g_game->getHero(g_currentPlayer->m_heroes[i])
                        ->m_army.getCreatureTotal(m_creatureType);
                for (i = 0; i < g_currentPlayer->m_numTowns; ++i)
                    total += g_game->getTown(g_currentPlayer->m_townIds[i])
                        ->getArmy().getCreatureTotal(m_creatureType);
                if (total >= m_numCreatures) {
                    m_playerWinner = static_cast<signed char>(g_netLocalGamePos);
                    m_gameWon = 1;
                    return 1;
                }
            }
        }
    }
    return 0;
}

VA(0x005f1c80, 0xB9)
DC_ADDRESS(0x18ff84, 0xb4)
MAC_ADDRESS(0x1fd9fc, 0x130)
unsigned char VictoryConditionStruct::checkForTotalResources()
{
    if (m_type == VICTORY_CONDITION_TOTAL_RESOURCES
        && g_currentPlayer
        && !g_game->m_playerDisabled[g_netLocalGamePos]) {
        if (g_game->isHumanAlly(g_netLocalGamePos) || m_appliesToComputer) {
            if (g_currentPlayer->m_resources[m_resourceType] >= m_resourceAmount) {
                m_playerWinner = static_cast<signed char>(g_netLocalGamePos);
                m_gameWon = 1;
                return 1;
            }
        }
    }
    return 0;
}

VA(0x005f1d40, 0x1A4)
DC_ADDRESS(0x190038, 0xec)
MAC_ADDRESS(0x1fdb2c, 0x298)
unsigned char VictoryConditionStruct::checkForUpgradedTown()
{
    if (m_type != VICTORY_CONDITION_UPGRADE_TOWN
        || !g_currentPlayer
        || g_game->m_playerDisabled[g_netLocalGamePos])
        return 0;

    unsigned char hallOk = 0;
    unsigned char castleOk = 0;
    town* checkedTown =
        g_game->getTown(g_game->getTownId(m_townX, m_townY, m_townZ));
    signed char owner = checkedTown->m_owner;
    if (owner == -1)
        return 0;

    switch (m_hallLevel) {
    case VICTORY_HALL_TOWN:
        hallOk = checkedTown->hasBuilding(HALL_TOWN_ID, true);
        break;
    case VICTORY_HALL_CITY:
        hallOk = checkedTown->hasBuilding(HALL_CITY_ID, true);
        break;
    case VICTORY_HALL_CAPITOL:
        hallOk = checkedTown->hasBuilding(HALL_CAPITOL_ID, true);
        break;
    }
    switch (m_castleLevel) {
    case VICTORY_CASTLE_FORT:
        castleOk = checkedTown->hasBuilding(CASTLE_FORT_ID, true);
        break;
    case VICTORY_CASTLE_CITADEL:
        castleOk = checkedTown->hasBuilding(CASTLE_CITADEL_ID, true);
        break;
    case VICTORY_CASTLE_CASTLE:
        castleOk = checkedTown->hasBuilding(CASTLE_CASTLE_ID, true);
        break;
    }
    if (hallOk && castleOk) {
        m_gameWon = 1;
        m_playerWinner = owner;
        return 1;
    }
    return 0;
}

VA(0x005f1ef0, 0x203)
DC_ADDRESS(0x190124, 0x120)
MAC_ADDRESS(0x1fddc4, 0x2d0)
unsigned char VictoryConditionStruct::checkForGrailBuildingWin()
{
    if (m_type != VICTORY_CONDITION_BUILD_GRAIL
        || !g_currentPlayer
        || g_game->m_playerDisabled[g_netLocalGamePos])
        return 0;

    // Mac 0x1fde18 constructs the target before the loop; DC records
    // the target and wildcard locals at the same outer lifetime.
    type_point anyTownLoc(-1, -1, -1);
    type_point grailTownLoc(m_townX, m_townY, m_townZ);
    int player = 0;
    for (;;) {
        if (g_game->onSameTeam(player, g_netLocalGamePos)) {
            for (int j = 0; j < g_game->m_players[player].m_numTowns; ++j) {
                town* thisTown = g_game->getTown(
                    g_game->m_players[player].m_townIds[j]);
                type_point thisTownLoc = thisTown->getLocation();
                bool hasGrail = false;
                // Mac 0x1fdf0c..0x1fe024 expands the same coordinate and
                // wildcard comparison as isGrailTarget at 0x1fe124.
                if (isGrailTarget(thisTownLoc, grailTownLoc, anyTownLoc))
                    hasGrail = thisTown->hasBuilding(HOLY_GRAIL_ID, true);
                if (hasGrail) {
                    m_playerWinner = thisTown->m_owner;
                    m_gameWon = 1;
                    return 1;
                }
            }
        }
        ++player;
        if (player >= 8)
            return 0;
    }
}

VA(0x005f2100, 0x53)
DC_ADDRESS(0x190244, 0x80)
MAC_ADDRESS(0x1fe094, 0x90)
bool VictoryConditionStruct::checkForHeroDefeatWin(
    const int winningPlayer, const hero* loser)
{
    if (m_type == VICTORY_CONDITION_DEFEAT_HERO
        && loser
        && g_currentPlayer
        && !g_game->m_playerDisabled[g_netLocalGamePos]
        && loser->m_id == m_heroId) {
        m_playerWinner = winningPlayer;
        m_gameWon = 1;
        return true;
    }
    return false;
}

VA(0x005f2160, 0xFD)
DC_ADDRESS(0x1902c4, 0x7a)
MAC_ADDRESS(0x1fe124, 0x184)
unsigned char VictoryConditionStruct::isGrailTarget(town* thisTown)
{
    type_point anyTownLoc(-1, -1, -1);
    type_point grailTownLoc(m_townX, m_townY, m_townZ);
    type_point thisTownLoc = thisTown->getLocation();
    return isGrailTarget(thisTownLoc, grailTownLoc, anyTownLoc);
}

VA(0x005f2260, 0x34)
DC_ADDRESS(0x190340, 0x3a)
MAC_ADDRESS(0x1fe2a8, 0x6c)
bool VictoryConditionStruct::isTownCaptureTarget(town* thisTown)
{
    if (m_type != VICTORY_CONDITION_CAPTURE_TOWN)
        return 0;

    if (thisTown->m_id != g_game->getTownId(m_townX, m_townY, m_townZ))
        return false;
    return true;
}

VA(0x005f22a0, 0xE6)
DC_ADDRESS(0x19037c, 0x8e)
MAC_ADDRESS(0x1fe314, 0x154)
unsigned char VictoryConditionStruct::checkForTownCaptureWin()
{
    if (m_type != VICTORY_CONDITION_CAPTURE_TOWN
        || !g_currentPlayer
        || g_game->m_playerDisabled[g_netLocalGamePos])
        return 0;

    if (!(g_game->isHumanAlly(g_netLocalGamePos) || m_appliesToComputer))
        return 0;

    int townId = g_game->getTownId(m_townX, m_townY, m_townZ);
    if (townId < 0)
        return 0;

    town* capturedTown = g_game->getTown(townId);
    m_playerWinner = capturedTown->m_owner;
    m_gameWon = 1;
    return 1;
}

VA(0x005f2390, 0x267)
DC_ADDRESS(0x19040c, 0x7a)
MAC_ADDRESS(0x1fe468, 0x3a8)
bool VictoryConditionStruct::checkForDefeatedMonsterWin(
    const hero* thisHero, const type_point monsterLoc)
{
    if (m_type == VICTORY_CONDITION_DEFEAT_ALL_MONSTERS) {
        type_point pos;
        for (pos.m_z = 0; pos.m_z < g_game->getNumMapLevels(); ++pos.m_z) {
            for (pos.m_y = 0; pos.m_y < g_mapHeight; ++pos.m_y) {
                for (pos.m_x = 0; pos.m_x < g_mapWidth; ++pos.m_x) {
                    NewmapCell* cell = g_game->getCell(pos);
                    if (cell->m_isTrigger && cell->m_type == MONSTER) {
                        if (!(pos == monsterLoc))
                            return 0;
                    }
                }
            }
        }
        m_gameWon = 1;
        return 1;
    }
    if (m_type == VICTORY_CONDITION_DEFEAT_MONSTER
        && g_currentPlayer
        && !g_game->m_playerDisabled[g_netLocalGamePos]) {
        type_point pos(m_monsterX, m_monsterY, m_monsterZ);
        if (monsterLoc == pos) {
            m_playerWinner = thisHero->m_owner;
            m_gameWon = 1;
            return 1;
        }
    }
    return 0;
}

VA(0x005f2600, 0x117)
DC_ADDRESS(0x190488, 0xb0)
MAC_ADDRESS(0x1fe810, 0x194)
unsigned char VictoryConditionStruct::checkForFlaggedGeneratorWin()
{
    if (m_type != VICTORY_CONDITION_FLAG_ALL_GENERATORS
        || !g_currentPlayer
        || g_game->m_playerDisabled[g_netLocalGamePos])
        return 0;

    if (!(g_game->isHumanAlly(g_netLocalGamePos) || m_appliesToComputer))
        return 0;

    for (unsigned int i = 0; i < g_game->m_generators.size(); ++i) {
        int owner = g_game->m_generators[i].getOwner();
        if (!g_game->onSameTeam(owner, g_netLocalGamePos))
            return 0;
    }
    m_playerWinner = static_cast<signed char>(g_netLocalGamePos);
    m_gameWon = 1;
    return 1;
}

VA(0x005f2720, 0xEB)
DC_ADDRESS(0x190538, 0xe6)
MAC_ADDRESS(0x1fe9a4, 0x190)
unsigned char VictoryConditionStruct::checkForFlaggedMineWin()
{
    if (m_type != VICTORY_CONDITION_FLAG_ALL_MINES
        || !g_currentPlayer
        || g_game->m_playerDisabled[g_netLocalGamePos])
        return 0;

    if (!(g_game->isHumanAlly(g_netLocalGamePos) || m_appliesToComputer))
        return 0;

    for (unsigned int i = 0; i < g_game->m_mines.size(); ++i) {
        int owner = g_game->m_mines[i].m_playerOwner;
        if (!g_game->onSameTeam(owner, g_netLocalGamePos))
            return 0;
    }
    m_playerWinner = static_cast<signed char>(g_netLocalGamePos);
    m_gameWon = 1;
    return 1;
}

VA(0x005f2810, 0x45)
MAC_ADDRESS(0x1feb34, 0x6c)  // hd-crossbuild
unsigned char VictoryConditionStruct::checkForTimeSurvival()
{
    if (m_type == VICTORY_CONDITION_SURVIVE_TIME) {
        short days = g_game->getCurrentTurn();
        if (days > m_numDays) {
            m_gameWon = 1;
            return 1;
        }
    }
    return 0;
}

// DC lines448..458 retain the positive point-match scope. Complete retail
// needs the same scope to place the common failure epilogue after success.
VA(0x005f2860, 0x1DE)
DC_ADDRESS(0x190620, 0xb2)
MAC_ADDRESS(0x1feba0, 0x2bc)
unsigned char VictoryConditionStruct::checkForArtifactTransportWin(
    const hero* thisHero, const type_point townLoc)
{
    if (m_type != VICTORY_CONDITION_TRANSPORT_ARTIFACT
        || !g_currentPlayer
        || g_game->m_playerDisabled[g_netLocalGamePos])
        return 0;

    if (g_game->isHumanAlly(g_netLocalGamePos) || m_appliesToComputer) {
        type_point target(m_townX, m_townY, m_townZ);
        if (target == townLoc) {
            if (thisHero->hasArtifact(m_artifactNum)) {
                m_playerWinner = thisHero->m_owner;
                m_gameWon = 1;
                return 1;
            }
            int comboIdx = g_artifactTraits[m_artifactNum].m_comboType;
            if (comboIdx == -1)
                return 0;

            const std::bitset<144>& components =
                g_combinationArtifacts[comboIdx].m_components;
            int remaining = components.count();
            for (int i = 0;; ++i) {
                if (components.test(i)) {
                    if (!thisHero->hasArtifact(i))
                        return 0;
                    if (--remaining == 0) {
                        m_playerWinner = static_cast<signed char>(g_netLocalGamePos);
                        m_gameWon = 1;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

// CheckForDefeatedHeroLoss's campaign table: the ordinals with a
// special hero-death loss rule, the map ordinals each arm gates on,
// and the protected hero ids each arm tests. No roster in this tree
// names campaign slots or the campaign hero ids, so every constant
// stays value-ordinal (the hero.cpp kStartLevelCampaign precedent);
// the artifact ids come from the artifact domain. All values are
// retail compares at 0x5f2a40, and the switch's case ORDER is the
// retail jump-table arm layout (3,7,10,12,9,14,15,17,18,19 - source
// order IS layout order for a jump-table switch).
static const int g_lossCampaign3 = 3;
static const int g_lossCampaign7 = 7;
static const int g_lossCampaign9 = 9;
static const int g_lossCampaign10 = 10;
static const int g_lossCampaign12 = 12;
static const int g_lossCampaign14 = 14;
static const int g_lossCampaign15 = 15;
static const int g_lossCampaign17 = 17;
static const int g_lossCampaign18 = 18;
static const int g_lossCampaign19 = 19;
static const int g_map2 = 2;
static const int g_map3 = 3;
static const int g_map4 = 4;
static const int g_map5 = 5;
static const int g_map6 = 6;
static const int g_map7 = 7;
static const int g_map8 = 8;
static const int g_map9 = 9;
static const int g_map10 = 10;
static const int g_map11 = 11;
static const int g_lossHero4 = 4;
static const int g_lossHero22 = 0x16;
static const int g_lossHero27 = 0x1b;
static const int g_lossHero45 = 0x2d;
static const int g_lossHero74 = 0x4a;
static const int g_lossHero76 = 0x4c;
static const int g_lossHero96 = 0x60;
static const int g_lossHero97 = 0x61;
static const int g_lossHero102 = 0x66;
static const int g_lossHero104 = 0x68;
static const int g_lossHero110 = 0x6e;
static const int g_lossHero145 = 0x91;
static const int g_lossHero146 = 0x92;
static const int g_lossHero147 = 0x93;
static const int g_lossHero148 = 0x94;
static const int g_lossHero149 = 0x95;
static const int g_lossHero152 = 0x98;
static const int g_lossPortrait146 = 0x92;

// In campaign mode a per-campaign table of protected heroes (and, in
// two campaigns, carried quest artifacts) loses the game immediately;
// outside those arms the ordinary lose-hero condition compares ids.
// Retail's ordinary tail rejects other loss types, then returns a byte-valued
// id comparison. Keep that tail once: a named unsigned-char result reproduces
// its sete al epilogue without the wider bool-to-int temporary. Together with
// the early loss-type rejection this improves MAX 75.8636 -> 82.0170
// (2026-09-07). The older duplicated tail was compensating for the wrong
// result lowering; it is not source evidence.
// Retail +0x1b0 loads the combination-artifact table before initializing the
// loop index and keeps the components reference across hasArtifact calls.
// Re-subscripting the global on every iteration incorrectly reloads the table.
// DC proves the const hero parameter, but its older campaign-3-only
// implementation writes the loss state here. Complete performs those writes
// in the following helper; do not import the older stores or entry type guard.
// Controls: deleting only the duplicate tail 10.6676%; early guard with byte
// result 81.8182%; components reference alone 76.0625%; both repairs 82.0170%.
// On the combined source, direct bool return is 80.9943%, positive type-test
// nesting with byte result 11.2784%; loop/arm-local declarations are byte-flat.
// An else-if after the campaign-10 hero guard, a positive map-2/3/4 guard
// around the shared artifact checks, and initializing the byte result at entry
// also reproduce the same bytes at 82.0170%; none repairs return placement.
// Residual: the type guard still sits after campaign 14 instead of after the
// entire switch; several return-1 branches share different epilogues. The
// candidate has 17 returns versus retail's 19; the artifact call sequence
// agrees. Earlier polarity/goto/default/case-order controls did not resolve
// that placement. No compiler-generation conclusion follows from this gap.
// Use the proven const loser parameter directly: hasArtifact now retains its
// DC hero.cpp:1422 const receiver, so the old mutable h alias and const_cast
// are unnecessary. Legacy/const-alias/direct-parameter controls preserve all
// executable bytes and reference targets across this TU, including nineteen
// exact siblings. Their three object identities differ only in local label
// and temporary numbering; none repairs the return layout at 82.0170%.
// Mac keeps one campaign base pointer and retains map across the artifact
// checks before independently admitting maps 2..4. Keep that lifetime rather
// than borrowing the known-map fact from an else arm. Its component loop
// assigns the index to r28 and the component reference to r29; a loop-local
// index reproduces those homes. The full source model raises Mac to 55.6973%
// (previous recorded peak 51.2799%); Windows 81.15% vs 82.02% is recovery debt.
// E:\gamedcs\victorylossconditions.cpp:463
VA(0x005f2a40, 0x3C8)
DC_ADDRESS(0x1906d4, 0x78)
MAC_ADDRESS(0x1fee5c, 0x448)  // anchor-global
unsigned char LossConditionStruct::checkForDefeatedHeroLoss(const hero* loser)
{
    if (g_inCampaign) {
        int map;
        const SCampaign& campaign = g_game->m_campaign;
        switch (campaign.m_currentCampaign) {
        case g_lossCampaign3:
            if (campaign.m_currentMap == g_map2
                && (loser->m_id == g_lossHero4
                    || loser->m_portrait == g_lossPortrait146))
                return 1;
            break;
        case g_lossCampaign7:
            if ((campaign.m_currentMap == g_map6
                 || campaign.m_currentMap == g_map7)
                && (loser->m_id == g_lossHero148 || loser->m_id == g_lossHero152
                    || loser->m_id == g_lossHero146))
                return 1;
            break;
        case g_lossCampaign10:
            if (loser->m_id == g_lossHero149)
                return 1;
            if ((campaign.m_currentMap == 1
                 || campaign.m_currentMap == g_map2)
                && (loser->m_id == g_lossHero104 || loser->m_id == g_lossHero97
                    || loser->m_id == g_lossHero110))
                return 1;
            break;
        case g_lossCampaign12:
            if (loser->m_id == g_lossHero145)
                return 1;
            break;
        case g_lossCampaign9:
            if (loser->m_id == g_lossHero147)
                return 1;
            break;
        case g_lossCampaign14: {
            if (loser->m_id == g_lossHero45)
                return 1;
            map = campaign.m_currentMap;
            if (map == g_map2) {
                if (loser->hasArtifact(ARTIFACT_ORB_OF_THE_FIRMAMENT)
                    || loser->hasArtifact(ARTIFACT_ORB_OF_DRIVING_RAIN)
                    || loser->hasArtifact(ARTIFACT_ORB_OF_SILT)
                    || loser->hasArtifact(ARTIFACT_ORB_OF_TEMPESTUOUS_FIRE)
                    || loser->hasArtifact(ARTIFACT_ORB_OF_INHIBITION)
                    || loser->hasArtifact(ARTIFACT_SWORD_OF_HELLFIRE))
                    return 1;
            }
            if (map >= g_map2 && map <= g_map4) {
                if (loser->hasArtifact(ARTIFACT_ANGELIC_ALLIANCE))
                    return 1;
                const std::bitset<144>& components =
                    g_combinationArtifacts[0].m_components;
                for (int i = 0; i < 0x90; ++i) {
                    if (components.test(i)
                        && loser->hasArtifact(i))
                        return 1;
                }
            }
            break;
        }
        // Mac 0x1ff0bc..0x1ff13c continues through three independent map
        // predicates; VC6 propagates the known map into direct failure exits.
        case g_lossCampaign15:
            map = campaign.m_currentMap;
            if (map == 1 && loser->m_id == g_lossHero22)
                return 1;
            if (map == g_map2
                && (loser->m_id == g_lossHero22
                    || loser->hasArtifact(ARTIFACT_VAMPIRES_COWL)))
                return 1;
            if (map == g_map3
                && (loser->m_id == g_lossHero22
                    || loser->hasArtifact(ARTIFACT_DEAD_MANS_BOOTS)))
                return 1;
            break;
        case g_lossCampaign17:
            if ((campaign.m_currentMap == g_map2
                 || campaign.m_currentMap == g_map3)
                && (loser->m_id == g_lossHero74 || loser->m_id == g_lossHero76))
                return 1;
            break;
        case g_lossCampaign18:
            map = campaign.m_currentMap;
            if (map == g_map4) {
                if (loser->m_id == g_lossHero96)
                    return 1;
            } else if (map == g_map5) {
                if (loser->m_id == g_lossHero148)
                    return 1;
            } else if (map == g_map8) {
                if (loser->m_id == g_lossHero27 || loser->m_id == g_lossHero148)
                    return 1;
            } else if (map == g_map9) {
                if (loser->m_id == g_lossHero96 || loser->m_id == g_lossHero102)
                    return 1;
            } else if (map == g_map10 || map == g_map11) {
                if (loser->m_id == g_lossHero27 || loser->m_id == g_lossHero148
                    || loser->m_id == g_lossHero102
                    || loser->m_id == g_lossHero96)
                    return 1;
            }
            break;
        case g_lossCampaign19:
            if ((campaign.m_currentMap == 0
                 || campaign.m_currentMap == g_map2)
                && loser->m_id == g_lossHero74)
                return 1;
            break;
        }
    }
    if (m_type != LOSS_CONDITION_LOSE_HERO)
        return 0;
    unsigned char defeated = loser->m_id == m_heroId;
    return defeated;
}

VA(0x005f2e10, 0x2F)
MAC_ADDRESS(0x1ff2a4, 0x60)
unsigned char LossConditionStruct::heroKilled(const hero* loser)
{
    if (checkForDefeatedHeroLoss(loser)) {
        m_type = LOSS_CONDITION_LOSE_HERO;
        m_heroId = loser->m_id;
        m_gameLost = 1;
        return 1;
    }
    return 0;
}

// Mac is byte-exact with the early type guard, returned-location temporary
// as the equality operand, and stored loss member as the result. A named
// location local delays field extraction; an explicit result assignment
// adds another copy. Windows stays 99.8429% with only the two stack homes
// exchanged; keep the canonical constructor, getLocation and operator==.
VA(0x005f2e40, 0xD9)
DC_ADDRESS(0x19074c, 0x70)
MAC_ADDRESS(0x1ff304, 0x140)
unsigned char LossConditionStruct::checkForDefeatedTownLoss(
    const int oldOwner, const town* lostTown)
{
    if (m_type != LOSS_CONDITION_LOSE_TOWN)
        return 0;

    type_point target(m_townX, m_townY, m_townZ);
    if (target == lostTown->getLocation()) {
        m_playerLoser = static_cast<signed char>(oldOwner);
        m_gameLost = 1;
        return m_gameLost;
    }
    return 0;
}

VA(0x005f2f20, 0x50)
DC_ADDRESS(0x1907bc, 0xa0)
MAC_ADDRESS(0x1ff444, 0x78)
unsigned char LossConditionStruct::checkForTimeLimitExpired()
{
    if (m_type == LOSS_CONDITION_TIME_LIMIT) {
        // The time-limit check keeps the full unsigned calendar expression;
        // getCurrentTurn() returns a short and would narrow it first.
        int days = (static_cast<unsigned short>(g_game->m_month) * 4
            + static_cast<unsigned short>(g_game->m_week) - 5) * 7
          + g_game->m_day;
        if (days > m_numDays) {
            m_playerLoser = static_cast<signed char>(g_netLocalGamePos);
            m_gameLost = 1;
            return 1;
        }
    }
    return 0;
}

// COMDAT pairing: basic_string<char>::basic_string(const char*, const
// allocator&). Same single-member-group argument as smackmgr's copy ctor
// above: victorylossconditions.obj emits exactly one string constructor and
// it is this overload. `ret 8` matches its two arguments, and the callers
// reach hero, mapcell, rmg, this unit and three segments.
VA_COMPGEN(0x0048b370, 0xC1, CLASS_CTOR, basic_string)
