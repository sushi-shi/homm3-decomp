#ifndef HOMM3_COMMAND_H
#define HOMM3_COMMAND_H

#include "va.h"

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
extern int g_combatControlNetPos[2];

#endif  /* HOMM3_COMMAND_H */
