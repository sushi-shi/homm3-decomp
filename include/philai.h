// philai.h - prototypes of philai.cpp (compiland philai.obj)
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
__forceinline int valueOfLeanTo(NewmapCell* cell, playerData* player);
__forceinline long valueOfHeroEvent(
    const hero* currentHero, NewmapCell* cell, short x, short y, short z,
    short moveCost);
__forceinline long valueOfHillFort(const hero* currentHero,
                                      long moveCost);
__forceinline int valueOfLibrary(const hero* currentHero,
                                 NewmapCell* cell);
__forceinline int valueOfLighthouse(NewmapCell* cell);
int valueOfMagicSchool(const hero* currentHero, NewmapCell* cell);
__forceinline int valueOfMercenaryCamp(const hero* currentHero,
                                       NewmapCell* cell);
int moraleIncreaseValue(const hero* currentHero, int value);
int luckIncreaseValue(const hero* currentHero, int value);
__forceinline long valueOfMagusHut(long playerId);
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
__forceinline int valueOfShrine(const hero* currentHero, NewmapCell* cell);
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

// --- globals ---
// CODEVIEW(E:\gamedcs\philai.cpp:58, dc 0x10d458) int OnMySide(int iWhichPlayer);
// CODEVIEW(E:\gamedcs\philai.cpp:102, dc 0x10d510) void ShowStatus();
// CODEVIEW(E:\gamedcs\philai.cpp:123, dc 0x10d518) void IncrementHourGlass();
// CODEVIEW(E:\gamedcs\philai.cpp:150, dc 0x10d57c) void RestoreMouse(unsigned char mouse_was_visible);
// CODEVIEW(E:\gamedcs\philai.cpp:165, dc 0x10d5b4) void check_for_town(hero* current_hero);
// CODEVIEW(E:\gamedcs\philai.cpp:207, dc 0x10d684) void upgrade_creatures(hero* current_hero, const town* current_town);
// CODEVIEW(E:\gamedcs\philai.cpp:326, dc 0x10da30) void buy_artifacts(hero* current_hero, TArtifact* artifact_list, long market_count);
// CODEVIEW(E:\gamedcs\philai.cpp:370, dc 0x10dae8) long value_of_black_market(const hero* current_hero, const NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:445, dc 0x10dcc4) void buy_special_building(const hero* current_hero, town* current_town);
// CODEVIEW(E:\gamedcs\philai.cpp:519, dc 0x10dea8) long value_of_war_factory(const hero* current_hero, TArtifact engine, long move_cost);
// CODEVIEW(E:\gamedcs\philai.cpp:561, dc 0x10e064) void visit_war_factory(hero* current_hero, TArtifact engine);
// CODEVIEW(E:\gamedcs\philai.cpp:583, dc 0x10e0f8) void AI_visit_war_factory(hero* current_hero);
// CODEVIEW(E:\gamedcs\philai.cpp:636, dc 0x10e22c) const hero* get_best_hero(long player_id);
// CODEVIEW(E:\gamedcs\philai.cpp:662, dc 0x10e298) unsigned char should_garrison_town(const hero* current_hero, const town* current_town);
// CODEVIEW(E:\gamedcs\philai.cpp:833, dc 0x10e6e8) void mark_shipyards(playerData* player);
// CODEVIEW(E:\gamedcs\philai.cpp:896, dc 0x10e894) void clear_shipyards(playerData* player);
// CODEVIEW(E:\gamedcs\philai.cpp:1056, dc 0x10ec58) void MoveHero(hero* current_hero, long* danger_zones, unsigned char is_last_hero, unsigned char* explore_mode);
// CODEVIEW(E:\gamedcs\philai.cpp:1239, dc 0x10f0d0) void move_all_heroes(long player_id, long* danger_zones);
// CODEVIEW(E:\gamedcs\philai.cpp:1833, dc 0x1102e4) int ComputeUpgradeValue(hero* current_hero, int iSourceType, int iDestType);
// CODEVIEW(E:\gamedcs\philai.cpp:1854, dc 0x110390) int ValueOfArena(const hero* current_hero, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:1883, dc 0x110408) int ValueOfMapArtifact(const hero* current_hero, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:1972, dc 0x1105d8) int ValueOfBlackBox(const hero* current_hero, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:2115, dc 0x1108e0) int ValueOfCampfire(playerData* player, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:2194, dc 0x110c58) int ValueOfDefenseTower(const hero* current_hero, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:2220, dc 0x110cf8) long value_of_garrison(const hero* current_hero, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:2250, dc 0x110db4) long value_of_idol(const hero* current_hero, long move_cost);
// CODEVIEW(E:\gamedcs\philai.cpp:2274, dc 0x110f88) int ValueOfFlotsam(playerData* player);
// CODEVIEW(E:\gamedcs\philai.cpp:2283, dc 0x111004) int ValueOfGarden(const hero* current_hero, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:2294, dc 0x111028) int ValueOfLeanTo(NewmapCell* cell, playerData* player);
// CODEVIEW(E:\gamedcs\philai.cpp:2392, dc 0x111328) long value_of_hero_event(const hero* current_hero, NewmapCell* cell, short x, short y, short z, short move_cost);
// CODEVIEW(E:\gamedcs\philai.cpp:2465, dc 0x1114f4) long value_of_hill_fort(const hero* current_hero, long move_cost);
// CODEVIEW(E:\gamedcs\philai.cpp:2550, dc 0x11173c) int ValueOfLibrary(const hero* current_hero, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:2567, dc 0x1117d0) int ValueOfLighthouse(NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:2615, dc 0x1118f8) int ValueOfMercenaryCamp(const hero* current_hero, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:2775, dc 0x111e18) long value_of_magus_hut(long player_id);
// CODEVIEW(E:\gamedcs\philai.cpp:2811, dc 0x111f54) long value_of_pyramid(const hero* current_hero, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:2948, dc 0x112488) int ValueOfSkeleton(const hero* current_hero, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:2997, dc 0x1125ec) int ValueOfShrine(const hero* current_hero, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\philai.cpp:3469, dc 0x1135ac) long get_skill_value(const hero* our_hero, TSecondarySkill skill, unsigned char complex_choice);
// CODEVIEW(E:\gamedcs\philai.cpp:3834, dc 0x113e24) long AI_value_of_event(const hero* current_hero, type_point point, long* move_cost);

// --- philAI ---
// CODEVIEW(E:\gamedcs\philai.cpp:1261, dc 0x10f16c) void philAI::DoAI(int whichPlayer);
// CODEVIEW(E:\gamedcs\philai.cpp:1770, dc 0x110018) void philAI::GetTurnAIVars(int whichPlayer);

// --- type_spellvalue ---
// CODEVIEW(E:\gamedcs\philai.cpp:1339, dc 0x10f37c) void type_spellvalue::type_spellvalue(const hero* new_hero);
// CODEVIEW(E:\gamedcs\philai.cpp:1529, dc 0x10f94c) long type_spellvalue::get_summoning_value(long damage, long times_castable);
// CODEVIEW(E:\gamedcs\philai.cpp:1610, dc 0x10fc6c) void type_spellvalue::fill_creature_value_list();
// CODEVIEW(E:\gamedcs\philai.cpp:1699, dc 0x10fe64) long type_spellvalue::get_value_of_increase(long base_value, long power_change, long duration_change, long mana_change);

#endif  /* HOMM3_PHILAI_H */
