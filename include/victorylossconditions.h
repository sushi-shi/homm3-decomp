// victorylossconditions.h - victorylossconditions.cpp (compiland victorylossconditions.obj)
#ifndef HOMM3_VICTORYLOSSCONDITIONS_H
#define HOMM3_VICTORYLOSSCONDITIONS_H

#include <va.h>
#include "town.h"
#include "artifact.h"
#include "struct.h"

class hero;
class town;

class VictoryConditionStruct {
public:
    signed char m_type;
    signed char m_allowNormalVictory;
    signed char m_appliesToComputer;
    char m_paddingBeforeArtifact;
    // CheckForArtifactTransportWin and AI_get_value_of_artifact both read
    // the full retail dword at +4 as the requested artifact ordinal.
    // Dreamcast CodeView carries the source enum type; value_of_town is the
    // first retail consumer whose register schedule distinguishes the typed
    // member from an int-to-enum bridge. Storage remains the same dword;
    // game.cpp's loaders cross the map-format ordinal into it.
    TArtifact m_artifactNum;
    // The Dreamcast field list (dump 0x3e34) orders ArtifactNum,
    // CreatureType, NumCreatures between AppliesToComputer and
    // ResourceType; retail widens the trailing pair to ints.
    // CheckForTotalCreatures (0x5f1b10) pushes the dword at +0x8
    // straight into armyGroup::get_creature_total(TCreatureType) and
    // compares the summed total against the dword at +0xc, fixing both
    // offsets. ArtifactNum is exposed above from its independent +4 retail
    // reads.
    // Complete stores the widened creature ordinal as a dword. The condition
    // checker needs the enum type for its armyGroup call; display-only
    // consumers use the same proven representation without pulling the enum
    // through fragile include cycles.
    TCreatureType m_creatureType;
    int m_numCreatures;
    int m_resourceType;
    int m_resourceAmount;
    int m_townX;
    int m_townY;
    int m_townZ;
    // CheckForUpgradedTown (0x5f1d40) dispatches both of its switches
    // with `movsx` BYTE reads at +0x24/+0x25 - retail kept the DC pair
    // HallLevel/CastleLevel char-sized where it widened the neighbours.
    // ai_player.obj's two purchase_building helpers read them too:
    // value_of_hall reads HallLevel + HALL_TOWN_ID as its victory bar,
    // value_of_castle_upgrade indexes bitNumber[CASTLE_FORT_ID +
    // CastleLevel].
    signed char m_hallLevel;
    signed char m_castleLevel;
    char m_paddingBeforeHeroPosition[2];
    // ValidateVictoryLossConditions constructs a type_point from these
    // three dwords at +0x28/+0x2c/+0x30 before resolving HeroID.  They are
    // the retail-widened form of the DC HeroX/HeroY/HeroZ trio.
    int m_heroX;
    int m_heroY;
    int m_heroZ;
    int m_heroId;
    // CheckForDefeatedMonsterWin (0x5f2390) packs the words at
    // +0x38/+0x3c and the byte at +0x40 into a type_point - the DC
    // MonsterX/MonsterY/MonsterZ trio, int-widened like the town trio.
    int m_monsterX;
    int m_monsterY;
    int m_monsterZ;
    // CheckForTimeSurvival (0x5f2810) compares the computed absolute day
    // against the full dword at +0x44.
    int m_numDays;
    unsigned char m_gameWon;
    signed char m_playerWinner;
    char m_paddingAfterWinner[2];
    VA(0x004bc340, 0xE)  // anchor-caller (SavedGameHeader ctor), dc 0xbccdc
    VictoryConditionStruct()
      : m_type(-1), m_gameWon(0), m_playerWinner(-1) {}
    int appliesToPlayer(long playerId) const;
    // 0x5f1b10, CheckForTotalResources' twin. advManager::DoEvent
    // (0x4aaaa0) calls the pair back to back on the same
    // `gpGame->mapHeader.victoryCondition`, each followed by its own
    // CheckEndGame(0).
    unsigned char checkForTotalCreatures();
    unsigned char checkForTotalResources();
    unsigned char checkForUpgradedTown();
    bool checkForHeroDefeatWin(int winningPlayer, const hero* loser);
    bool isTownCaptureTarget(town* thisTown);
    unsigned char checkForTownCaptureWin();
    // `?CheckForDefeatedMonsterWin@VictoryConditionStruct@@QAA_NPBVhero@@
    // Utype_point@@@Z` fixes the whole signature - public, bool, a const
    bool checkForDefeatedMonsterWin(const hero* thisHero,
                                    const type_point monsterLoc);
    unsigned char checkForFlaggedGeneratorWin();
    unsigned char checkForFlaggedMineWin();
    unsigned char checkForArtifactTransportWin(const hero* thisHero,
                                               const type_point townLoc);
    unsigned char isGrailTarget(town* thisTown);
    unsigned char checkForTimeSurvival();
    unsigned char checkForArtifactWin();
    unsigned char checkForGrailBuildingWin();
};
SIZE(VictoryConditionStruct, 0x4C);

class LossConditionStruct {
public:
    signed char m_type;
    char m_paddingBeforeTownPosition[3];
    int m_townX;
    int m_townY;
    int m_townZ;
    int m_heroX;
    int m_heroY;
    int m_heroZ;
    int m_heroId;
    short m_numDays;
    unsigned char m_gameLost;
    signed char m_playerLoser;
    VA(0x0045bac0, 0xE)  // retained retail body; formerly enrolled by CLASS_CTOR
    LossConditionStruct()
      : m_type(-1), m_gameLost(0), m_playerLoser(-1) {}
    unsigned char checkForDefeatedHeroLoss(const hero* loser);
    unsigned char heroKilled(const hero* loser);
    unsigned char checkForDefeatedTownLoss(int oldOwner,
                                           const town* lostTown);
    unsigned char checkForTimeLimitExpired();
};
SIZE(LossConditionStruct, 0x24);

// --- VictoryConditionStruct ---
// Retail returns the full EAX value (`int`); the Dreamcast byte return above
// documents a platform/build divergence, not the PC signature.

#endif  /* HOMM3_VICTORYLOSSCONDITIONS_H */
