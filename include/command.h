#ifndef HOMM3_COMMAND_H
#define HOMM3_COMMAND_H

#include "va.h"

// The combat command domain: what a click on a combat hex means.
// combatManager::GetCommand (0x476490) ANSWERS these and
// combatManager::DoCommand (0x476bd0) CONSUMES them, and the two bodies
// between them prove every value below - GetCommand by which guard
// returns it, DoCommand by its jump table's index byte (a 22-entry
// table over `command - 1`, so 1..22 is the whole domain and 0 means
// "nothing to do"). The DC roster's combatManager::GetPointer takes the
// same domain as its `inCombatCommand` argument.

// The NAMES are bootstrap inventions - no roster, string or Dreamcast
// field reaches the domain - but each one restates what the two bodies
// do with it: 1 and 2 both issue DoCommand's MOVE order and are split
// by the stack's flying bit in GetCommand; 3 and 15 both issue the
// SHOOT order and are split by ShotIsThroughWall / ShotIsNotOptimal; 4
// is the hero panel of the side on turn (spell book) against 21 for the
// other side's; 5 opens a stack's info panel; 7 carries the second hex
// in field_132d8, i.e. the melee attack; 16 bombards a wall target; 17
// is the first-aid tent's heal; 20 is a creature-cast spell; 22 opens
// the castle tower info. 6 is only ever ANSWERED (the plain hover over
// a hex that is not actionable) and falls into DoCommand's empty
// default, as do the values with no name here - the domain is sparse.
enum ECombatCommand {
    COMBAT_COMMAND_NONE = 0,
    COMBAT_COMMAND_WALK = 1,
    COMBAT_COMMAND_FLY = 2,
    COMBAT_COMMAND_SHOOT = 3,
    COMBAT_COMMAND_SPELL_BOOK = 4,
    COMBAT_COMMAND_VIEW_ARMY = 5,
    COMBAT_COMMAND_HOVER = 6,
    COMBAT_COMMAND_ATTACK = 7,
    COMBAT_COMMAND_SHOOT_PENALTY = 15,
    COMBAT_COMMAND_BOMBARD_WALL = 16,
    COMBAT_COMMAND_FIRST_AID = 17,
    COMBAT_COMMAND_CREATURE_SPELL = 20,
    COMBAT_COMMAND_VIEW_OTHER_HERO = 21,
    COMBAT_COMMAND_VIEW_TOWERS = 22
};

// The extended-dialog row vocabulary the two victory presentations
// carry, all of it read straight off their bytes.

// The ROW KINDS continue EGameResource past NUM_RESOURCES: 8 is the
// artifact row show_looted_artifacts (0x4772b0) stores with
// `mov dword ptr [ebp-0x14], 8`, and 9 the spell row show_eagle_eye
// (0x476fe0) stores with `mov dword ptr [ebp-0x1c], 9`. In both cases
// the qualifier beside the kind is NOT a resource amount - the artifact
// row packs `extra<<16 | artifactId`, the spell row carries the bare
// SpellID.

// PAGE_SIZE is the batch at which either sweep raises a dialog and
// starts a new page: `and al,-8 / cmp eax,0x40` over the eight-byte
// element in show_looted_artifacts, `and ecx,-8 / cmp ecx,0x40` in
// show_eagle_eye. show_eagle_eye's list separator turns into " and " one
// row EARLIER (`cmp eax,7` against a size taken before the push), which
// is PAGE_SIZE - 1 and is spelled that way.
enum EVictoryDialog {
    VICTORY_DIALOG_ARTIFACT_ROW = 8,
    VICTORY_DIALOG_SPELL_ROW = 9,
    VICTORY_DIALOG_PAGE_SIZE = 8
};

// `1 - winningGroup` maps a winning side onto the LOSING side's index,
// and maps the "nobody won" winner (-1) onto 2 - which is why
// DoVictory's ladder has a third arm that touches BOTH heroes rather
// than an index into heroes[]. Retail spells the three arms with
// constant displacements (0x53cc and 0x53d0), never `heroes[loser]`.
enum ELastAliveSide {
    LAST_ALIVE_ATTACKER = 0,
    LAST_ALIVE_DEFENDER = 1,
    LAST_ALIVE_NEITHER = 2
};

// 0x697788. DECLARATION ONLY - src/advmgr.cpp:88 owns the DATA claim,
// and a second claim on one RVA is a fatal duplicate at delink. It lives
// here rather than in the .cpp because a line-initial `extern` in a .cpp
// is a cleanliness-floor violation, and here rather than by including
// advmgr.h, whose closure command.obj does not otherwise need - the
// pattern hero.h already documents for bVideoPaused. DoVictory
// (0x477470) reads it once, crossed with the network latch, to decide
// whether the results dialog gets a deadline.
extern int g_thisNetGotAdventureControl;

// The two remote combat-control player positions. Dreamcast publishes the
// array name; retail ResetRound indexes [1-currentSide], producing relocs to
// both the base and its second element.
DATA(0x0069773c) extern int g_combatControlNetPos[2];

#endif  /* HOMM3_COMMAND_H */
