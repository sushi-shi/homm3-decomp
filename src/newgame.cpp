#include "text.h"
#include "va.h"

#include <stdio.h>
#include <string.h>

#include "newgame.h"

#include "campaignbrief.h"
#include "game.h"
#include "misc.h"
#include "savegame.h"
#include "scenarioinfo.h"
#include "textresource.h"
#include "town.h"
#include "window.h"
#include "winmgr.h"

VA(0x005132b0, 0x1B)  // dc 0x103494
long getAlignmentCount(int legalAlignments)
{
    long count = 0;
    for (int i = 0; i < 9; ++i) {
        if (legalAlignments & (1 << i))
            ++count;
    }
    return count;
}

VA(0x005132d0, 0x50)  // dc 0x1034b4
TTownType pickAlignment(int legalAlignments, unsigned char getFirstAvail)
{
    long count = getAlignmentCount(legalAlignments);
    int which = 1;

    if (!getFirstAvail && count > 0)
        which = random(1, count);

    for (int i = 0; i < 9; ++i) {
        if (legalAlignments & (1 << i)) {
            if (--which == 0) {
                TTownType alignment;
                memcpy(&alignment, &i, sizeof alignment);
                return alignment;
            }
        }
    }
    return TOWN_CASTLE;
}

// The three map formats InitNewGame accepts, in the order retail tests them.
// File-scope constants rather than a game.h enum, the same shape game.cpp
// already uses for MAP_VERSION_OLD_CAMPAIGN_HERO_IDS.
const int g_mapFormatSod = 28;
const int g_mapFormatRoe = 14;
const int g_mapFormatAb = 21;
// SGameSetupOptions::playerPos carries a human's seat ordinal, or this
// sentinel for a slot the computer takes.
const int g_setupPlayerPosComputer = 10;

VA(0x00513320, 0x41A)  // dc 0x1034fc
void game::initNewGame(int difficulty, int version,
                       NewSMapHeader* mapHeader, TAbstractFile* infile)
{
    int humanCount = 0;

    m_setup.m_fileInitialized = 0;
    m_setup.m_curSelectedPlayer = -1;
    m_setup.m_initializationNumHumans =
        static_cast<signed char>(g_numHumanPlayers);

    if (mapHeader) {
        this->m_mapHeader = *mapHeader;
    } else if (infile) {
        this->m_mapHeader.read(infile, version);
    } else {
        this->m_mapHeader.get(m_setup.m_path, m_setup.m_filename, version);
    }

    applyMapHeaderAvailability();

    if (this->m_mapHeader.m_version != g_mapFormatSod
            && this->m_mapHeader.m_version != g_mapFormatRoe
            && this->m_mapHeader.m_version != g_mapFormatAb)
        return;

    setMapSize(this->m_mapHeader.m_size, this->m_mapHeader.m_size);

    int slot;
    for (slot = 0; slot < 8; slot++)
        m_setup.m_color[slot] = static_cast<signed char>(slot);

    for (slot = 0; slot < 8; slot++) {
        if (!this->m_mapHeader.m_playerSlotAttributes[slot].m_canBeHuman && !this->m_mapHeader.m_playerSlotAttributes[slot].m_canBeComputer) {
            m_setup.m_handicap[slot] = -1;
            m_setup.m_alignment[slot] = -1;
            m_setup.m_playerPos[slot] = -1;
            m_setup.m_canFlipFromToComputer[slot] = -1;
        } else {
            m_setup.m_handicap[slot] = 0;

            m_setup.m_alignment[slot] = pickAlignment(
                this->m_mapHeader.m_playerSlotAttributes[slot].m_legalAlignments, 0);
            m_setup.m_playerPos[slot] = -1;
            m_setup.m_canFlipFromToComputer[slot] = -1;
        }
    }

    for (slot = 0; slot < 8; slot++) {
        if (this->m_mapHeader.m_playerSlotAttributes[slot].m_canBeHuman && !this->m_mapHeader.m_playerSlotAttributes[slot].m_canBeComputer) {
            m_setup.m_canFlipFromToComputer[slot] = 0;
            m_setup.m_playerPos[slot] = static_cast<signed char>(humanCount);
            humanCount++;
        } else if (!this->m_mapHeader.m_playerSlotAttributes[slot].m_canBeHuman && this->m_mapHeader.m_playerSlotAttributes[slot].m_canBeComputer) {
            m_setup.m_playerPos[slot] = g_setupPlayerPosComputer;
            m_setup.m_canFlipFromToComputer[slot] = 0;
        } else if (this->m_mapHeader.m_playerSlotAttributes[slot].m_canBeHuman && this->m_mapHeader.m_playerSlotAttributes[slot].m_canBeComputer) {
            m_setup.m_canFlipFromToComputer[slot] = 1;
        }
    }

    for (slot = 0; slot < 8; slot++) {
        if (m_setup.m_playerPos[slot] != -1)
            continue;

        if (humanCount < g_numHumanPlayers && this->m_mapHeader.m_playerSlotAttributes[slot].m_canBeHuman) {
            m_setup.m_playerPos[slot] = static_cast<signed char>(humanCount);
            humanCount++;
        } else if (this->m_mapHeader.m_playerSlotAttributes[slot].m_canBeComputer) {
            m_setup.m_playerPos[slot] = g_setupPlayerPosComputer;
        }
    }

    m_setup.m_fileInitialized = 1;
    m_setup.m_difficulty = static_cast<signed char>(difficulty);
}

// Complete uses the nine-town alignment mask for both helpers.
// E:\gamedcs\newgame.cpp:355, dc 0x1037f8.
TTownType pickPrevAlignment(int legalAlignments, TTownType type)
{
    do {
        type = static_cast<TTownType>(type - 1) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */;
        if (type < -1)
            type = TOWN_CONFLUX;
        else if (type == -1)
            break;
    } while (!(legalAlignments & (1 << type)));
    return type;
}

// E:\gamedcs\newgame.cpp:368, dc 0x10380c.
TTownType pickNextAlignment(int legalAlignments, TTownType type)
{
    do {
        type = static_cast<TTownType>(type + 1) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */;
        if (type > TOWN_CONFLUX)
            type = static_cast<TTownType>(-1) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */;
    } while (type != -1 && !(legalAlignments & (1 << type)));
    return type;
}

VA(0x00513740, 0xBC)  // dc 0x103824
void game::showScenInfo()
{
    if (g_inCampaign) {
        TCampaignBrief campaignBrief(0, 1);
        campaignBrief.doModal();
        if (g_windowManager->m_dialogReturn == NEWGAME_CAMPAIGN_BRIEF_EXIT)
            g_gameCommand = NEWGAME_COMMAND_QUIT;
    } else {
        CScenarioInfoDlg scenarioInfo;
        scenarioInfo.doModal(0);
    }
}

VA(0x00513800, 0x1D5)  // dc 0x103888
void game::getLossConditionText(char* text)
{
    LossConditionStruct& loss = m_mapHeader.m_lossCondition;
    if (loss.m_type != -1) {
        switch (loss.m_type) {
        case LOSS_CONDITION_LOSE_TOWN: {
            town* targetTown = getTown(getTownId(
                loss.m_townX, loss.m_townY, loss.m_townZ));
            const char* targetType;
            if (targetTown->isCastle())
                targetType = g_generalText->getText(GENERAL_TEXT_CASTLE_LOWERCASE);
            else
                targetType = g_generalText->getText(GENERAL_TEXT_ATTACK_TARGET_TOWN);
            sprintf(text, g_generalText->getText(GENERAL_TEXT_LOSS_CONDITION_LOSE_TOWN_FORMAT), targetType,
                    targetTown->m_name.c_str());
            break;
        }
        case LOSS_CONDITION_LOSE_HERO: {
            hero* targetHero = getHero(loss.m_heroId);
            sprintf(text, g_generalText->getText(GENERAL_TEXT_LOSS_CONDITION_LOSE_HERO_FORMAT), targetHero->m_name);
            break;
        }
        case LOSS_CONDITION_TIME_LIMIT: {
            int month = (loss.m_numDays - 1) / 28 + 1;
            int week = (loss.m_numDays - (month - 1) * 28 - 1) / 7 + 1;
            int dayOfWeek = (loss.m_numDays - 1) % 7 + 1;
            sprintf(text, g_generalText->getText(GENERAL_TEXT_LOSS_CONDITION_TIME_LIMIT_FORMAT), month, week, dayOfWeek);
            break;
        }
        }
    } else {
        strcpy(text, g_generalText->getText(GENERAL_TEXT_LOSS_CONDITION_STANDARD));
    }
}

VA(0x005139e0, 0x64C)  // dc 0x103a08
void game::getVictoryConditionText(char* text)
{
    VictoryConditionStruct& victory = m_mapHeader.m_victoryCondition;
    if (victory.m_type != -1) {
        switch (victory.m_type) {
        case VICTORY_CONDITION_CAPTURE_TOWN: {
            town* targetTown = getTown(getTownId(
                victory.m_townX, victory.m_townY, victory.m_townZ));
            const char* targetType;
            if (targetTown->isCastle())
                targetType = g_generalText->getText(GENERAL_TEXT_CASTLE_LOWERCASE);
            else
                targetType = g_generalText->getText(GENERAL_TEXT_ATTACK_TARGET_TOWN);
            sprintf(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_CAPTURE_TOWN_FORMAT), targetType,
                    targetTown->m_name.c_str());
            break;
        }
        case VICTORY_CONDITION_DEFEAT_HERO: {
            hero* targetHero = getHero(victory.m_heroId);
            sprintf(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_DEFEAT_HERO_FORMAT), targetHero->m_name);
            break;
        }
        case VICTORY_CONDITION_ARTIFACT:
            if (victory.m_artifactNum == ARTIFACT_HOLY_GRAIL) {
                strcpy(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_FIND_GRAIL));
            } else {
                sprintf(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_FIND_ARTIFACT_FORMAT),
                        g_artifactTraits[victory.m_artifactNum].m_name);
            }
            break;
        case VICTORY_CONDITION_TOTAL_RESOURCES:
            sprintf(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_ACCUMULATE_RESOURCE_FORMAT), victory.m_resourceAmount,
                    g_resourceNames[victory.m_resourceType]);
            break;
        case VICTORY_CONDITION_UPGRADE_TOWN: {
            town* targetTown = getTown(getTownId(
                victory.m_townX, victory.m_townY, victory.m_townZ));
            sprintf(text, g_generalText->getText(GENERAL_TEXT_UPGRADE_FORMAT), targetTown->m_name.c_str());
            break;
        }
        case VICTORY_CONDITION_BUILD_GRAIL: {
            type_point townPos(victory.m_townX, victory.m_townY, victory.m_townZ);
            if (townPos != type_point(-1, -1, -1)) {
                town* targetTown = getTown(getTownId(
                    victory.m_townX, victory.m_townY, victory.m_townZ));
                sprintf(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_BUILD_GRAIL_IN_TOWN_FORMAT),
                        targetTown->m_name.c_str());
            } else {
                strcpy(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_BUILD_GRAIL_ANYWHERE));
            }
            break;
        }
        case VICTORY_CONDITION_DEFEAT_MONSTER: {
            int monsterX = victory.m_monsterX;
            int monsterY = victory.m_monsterY;
            int monsterZ = victory.m_monsterZ;
            int size = m_mapHeader.m_size;
            int direction;
            if (static_cast<double>(monsterX) < static_cast<double>(size) * 0.33
                && static_cast<double>(monsterY) < static_cast<double>(size) * 0.33)
                direction = 7;
            else if (static_cast<double>(monsterX) < static_cast<double>(size) * 0.33
                     && static_cast<double>(monsterY) > static_cast<double>(size) * 0.66)
                direction = 5;
            else if (static_cast<double>(monsterX) < static_cast<double>(size) * 0.33)
                direction = 6;
            else if (static_cast<double>(monsterX) > static_cast<double>(size) * 0.66
                     && static_cast<double>(monsterY) < static_cast<double>(size) * 0.33)
                direction = 1;
            else if (static_cast<double>(monsterX) > static_cast<double>(size) * 0.66
                     && static_cast<double>(monsterY) > static_cast<double>(size) * 0.66)
                direction = 3;
            else if (static_cast<double>(monsterX) > static_cast<double>(size) * 0.66)
                direction = 2;
            else if (static_cast<double>(monsterX) < static_cast<double>(size) * 0.33)
                direction = 0;
            else if (static_cast<double>(monsterX) > static_cast<double>(size) * 0.66)
                direction = 4;
            else
                direction = 8;

            if (!monsterZ) {
                sprintf(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_DEFEAT_MONSTER_FORMAT),
                        victory.m_creatureType >= CREATURE_PIKEMAN
                            && victory.m_creatureType <= CREATURE_RETAIL_RANGE_MAX
                            ? H3_AT(g_creatureTypeTraits, victory.m_creatureType).m_pluralName
                            : "",
                        g_directions[direction]);
            } else {
                sprintf(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_DEFEAT_MONSTER_UNDERGROUND_FORMAT),
                        victory.m_creatureType >= CREATURE_PIKEMAN
                            && victory.m_creatureType <= CREATURE_RETAIL_RANGE_MAX
                            ? H3_AT(g_creatureTypeTraits, victory.m_creatureType).m_pluralName
                            : "",
                        g_directions[direction]);
            }
            break;
        }
        case VICTORY_CONDITION_TOTAL_CREATURES:
            sprintf(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_ACCUMULATE_CREATURES_FORMAT), victory.m_numCreatures,
                    H3_AT(g_creatureTypeTraits, victory.m_creatureType).m_pluralName);
            break;
        case VICTORY_CONDITION_FLAG_ALL_GENERATORS:
            strcpy(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_FLAG_DWELLINGS));
            break;
        case VICTORY_CONDITION_FLAG_ALL_MINES:
            strcpy(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_FLAG_MINES));
            break;
        case VICTORY_CONDITION_TRANSPORT_ARTIFACT: {
            town* targetTown = getTown(getTownId(
                victory.m_townX, victory.m_townY, victory.m_townZ));
            sprintf(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_TRANSPORT_ARTIFACT_FORMAT),
                    g_artifactTraits[victory.m_artifactNum].m_name,
                    targetTown->m_name.c_str());
            break;
        }
        }
        if (victory.m_allowNormalVictory)
            strcat(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_STANDARD_ALLOWED));
    } else {
        strcpy(text, g_generalText->getText(GENERAL_TEXT_VICTORY_CONDITION_STANDARD));
    }
}
