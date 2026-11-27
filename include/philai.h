#ifndef HOMM3_PHILAI_H
#define HOMM3_PHILAI_H

#include "armygrp.h"
#include "herospec.h"

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
void tradePlayerResources(int playerId, int* costs, int flag);

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

#endif  /* HOMM3_PHILAI_H */
