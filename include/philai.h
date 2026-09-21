#ifndef HOMM3_PHILAI_H
#define HOMM3_PHILAI_H

#include "armygrp.h"
#include "herospec.h"  // TSecondarySkill, the appraisals' skill parameter

class hero;
class town;
class garrison;
class generator;
class NewmapCell;
class playerData;
struct TBlackMarket;
struct type_university;
struct type_point;

// The Dreamcast class roster has no data members for this coordinator;
// its three public methods are the complete method roster.  Retail DoAI
// likewise uses `this` only to dispatch GetTurnAIVars.
class philAI {
public:
    enum {
        ONE_ACTIVE_HERO = 1,
        TWO_ACTIVE_HEROES = 2,
        THREE_ACTIVE_HEROES = 3,
        THIRD_HOURGLASS_PHASE = 3,
        SIXTH_HOURGLASS_PHASE = 6,
        LAST_HOURGLASS_PHASE = 9,
        // Retail's calendar word is compared with the final weekday before
        // a low-value destination puts the AI hero to sleep.
        AI_HERO_MOVE_SLEEP_DAY = 7
    };

    philAI();
    void doAI(int whichPlayer);
    void getTurnAIVars(int whichPlayer);
};

// Complete's computer-owner purchase wrapper, defined at 0x526d20 in
// philai.cpp. town::buyBuilding calls it before charging the resource row.
// No original name is known; retain the ordinal until source evidence exists.
void unnamed526d20(int playerId, int* costs, int flag);

long aiGetSpellValue(const hero* ourHero, SpellID spell);

// Dreamcast line 3834 publishes the reference-qualified
// move-cost parameter in the decorated name (`AAJ`) as well as the local and
// statement records. Complete retains the same register/stack ABI at
// 0x528040.
long aiValueOfEvent(const hero* currentHero, type_point point,
                       long& moveCost);
long aiValueOfEvent(const hero* currentHero, type_point point);
void aiJoinDecision(hero* currentHero, TCreatureType creature,
                      short amount);

// Source-real appraisal boundaries used by AI_value_of_event. Several are
// expanded or revision-adapted in Complete, but keeping these declarations
// makes the recovered Dreamcast dispatch state explicit while their bodies
// are promoted independently.
long valueOfBlackMarket(const hero* currentHero,
                           const NewmapCell* cell);
int valueOfArena(const hero* currentHero, NewmapCell* cell);
int valueOfMapArtifact(const hero* currentHero, NewmapCell* cell);
int valueOfBlackBox(const hero* currentHero, NewmapCell* cell);
int valueOfCampfire(playerData* player, NewmapCell* cell);
int valueOfDefenseTower(const hero* currentHero, NewmapCell* cell);
long valueOfBank(const hero* currentHero, NewmapCell* cell);
int valueOfGenerator(const hero* currentHero, int x, int y, int z,
                     NewmapCell* cell, int moveCost);
long valueOfGarrison(const hero* currentHero, NewmapCell* cell);
long valueOfIdol(const hero* currentHero, long moveCost);
int valueOfFlotsam(playerData* player);
int valueOfGarden(const hero* currentHero, NewmapCell* cell);
inline int valueOfLeanTo(NewmapCell* cell, playerData* player);
inline long valueOfHeroEvent(
    const hero* currentHero, NewmapCell* cell, short x, short y, short z,
    short moveCost);
inline long valueOfHillFort(const hero* currentHero,
                                      long moveCost);
inline int valueOfLibrary(const hero* currentHero,
                                 NewmapCell* cell);
inline int valueOfLighthouse(NewmapCell* cell);
int valueOfMagicSchool(const hero* currentHero, NewmapCell* cell);
inline int valueOfMercenaryCamp(const hero* currentHero,
                                       NewmapCell* cell);
int moraleIncreaseValue(const hero* currentHero, int value);
int luckIncreaseValue(const hero* currentHero, int value);
inline long valueOfMagusHut(long playerId);
int valueOfMine(const hero* currentHero, NewmapCell* cell);
long valueOfMonsters(const hero* currentHero, NewmapCell* cell,
                       type_point point);
int valueOfMoveSource(const hero* currentHero, long flag, short increase,
                         long& moveCost);
int valueOfObelisk(NewmapCell* cell, long playerId);
int valueOfPowerSchool(const hero* currentHero, NewmapCell* cell);
int valueOfPrison(NewmapCell* cell, playerData* player);
long valueOfPyramid(const hero* currentHero, NewmapCell* cell);
long getValueOfSpring(const hero* currentHero, const NewmapCell* cell,
                         unsigned short moveCost);
long getValueOfWell(const hero* currentHero, unsigned short moveCost);
int valueOfRallyFlag(const hero* currentHero, long& moveCost);
int valueOfRefugeeCamp(const hero* currentHero, NewmapCell* cell);
long valueOfResource(const hero* currentHero, NewmapCell* cell,
                     playerData* player);
int valueOfSeaChest(const hero* currentHero, NewmapCell* cell);
int valueOfSkeleton(const hero* currentHero, NewmapCell* cell);
int valueOfScroll(const hero* currentHero, NewmapCell* cell);
inline int valueOfShrine(const hero* currentHero, NewmapCell* cell);
int valueOfSirens(const hero* currentHero);
int valueOfStables(const hero* currentHero, long& moveCost);
long valueOfTown(const hero* currentHero, int x, int y, int z,
                   short moveCost);
int valueOfTreasure(const hero* currentHero);
int valueOfTree(const hero* currentHero, NewmapCell* cell);
int valueOfWagon(NewmapCell* cell, long playerId);
long valueOfWarFactory(const hero* currentHero, long moveCost);
int valueOfWarSchool(const hero* currentHero, NewmapCell* cell);
int valueOfWitchHut(const hero* currentHero, NewmapCell* cell);

void aiEnterTown(hero* currentHero, town* currentTown);

void aiEnterGarrison(hero* currentHero, garrison* ourGarrison);
void aiPurchaseCreatures(hero* currentHero, generator* currentGenerator);
void aiVisitBlackMarket(hero* currentHero, TBlackMarket* blackMarket);
void aiVisitHillFort(hero* currentHero);
void aiVisitUniversity(hero* currentHero, type_university* university);
void aiVisitWarFactory(hero* currentHero);

// Retail .data 0x678370, the row immediately after tradpost.h's
// fTradingPostEfficency (0x678344): three consecutive eleven-float rows
// fill 0x678344..0x6783c8, and get_artifact_purchase_price divides an
// artifact's gold cost by THIS row indexed by the marketplace count.
// The definition belongs to tradpost.cpp beside its sibling and moves
// there with the rest of that .data band; declared here meanwhile so the
// division has a typed name rather than a raw address.
extern float g_artifactPurchaseEfficency[];

// The two secondary-skill appraisals AI_choose_secondary_skill calls.
// The Dreamcast roster types the skill as TSecondarySkill; Complete's
// AI_visit_university passes the university's int-width skill slots directly,
// so the common retail-facing declaration keeps the byte-proven int width.
// Both retail bodies are located (0x524690 / 0x524dd0) but not reconstructed - so
// they cannot be declared static here: VC6 rejects a static function that
// is declared and called but never defined (C2129). Move them back into
// philai.cpp as statics when the bodies land.
long getSkillValue(const hero* ourHero, TSecondarySkill skill,
                     unsigned char complexChoice);
unsigned char wantsSkill(const hero* ourHero, TSecondarySkill skill,
                          unsigned char complexChoice);

#endif  /* HOMM3_PHILAI_H */
