#include "text.h"
#include "va.h"
#include "includes.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "events.h"

#include "advmgr.h"
#include "advmgr_objects.h"
#include "cmbtmgr.h"
#include "command.h"
#include "creature_bank.h"
#include "creaturetype.h"
#include "cursor.h"
#include "exec.h"
#include "game.h"
#include "hillfortwindow.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "mousemgr.h"
#include "philai.h"
#include "recruit.h"
#include "remote.h"
#include "remotedlg.h"
#include "resourcemanager.h"
#include "sacrifice_window.h"
#include "soundmgr.h"
#include "swapmgr.h"
#include "textresource.h"
#include "townmgr.h"
#include "tradpost.h"
#include "university_window.h"
#include "winmgr.h"

// Complete sends the raw primary-skill bytes in DoCombat's level update;
// getPrimarySkill would clamp them. This accessor is a provisional Windows
// boundary carried from the target branch, with no known DC declaration.
void hero::copyPrimarySkills(signed char* stats) const
{
    memcpy(stats, m_stats, sizeof(m_stats));
}

#if 0  // @carcass

// E:\gamedcs\events.cpp:300
// RETAIL_LOCATED(0x0049e170, 0x16E)  // anchor-global, dc 0x90348
void advManager::eraseAndFizzle(NewmapCell* eventCell, type_point point, int fizzleSound)
{
    // @stub
}

// E:\gamedcs\events.cpp:317
// RETAIL_LOCATED(0x0049e2e0, 0x38B)  // located @stub (promoted to active VA), dc 0x903b4
void advManager::doEventShipyard(NewmapCell* cell, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:516
// RETAIL_LOCATED(0x0049ea40, 0x304)  // located @stub (promoted to active VA), dc 0x908dc
void advManager::fightForArtifact(hero* current_hero, NewmapCell* cell, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:570
// RETAIL_LOCATED(0x0049ed50, 0x2E8)  // located @stub (promoted to active VA), dc 0x90ad8
void advManager::payForArtifact(hero* current_hero, NewmapCell* cell, type_point point, const char* dialog_text, short gold_cost, short resource_cost, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:647
// RETAIL_LOCATED(0x0049f040, 0x23)  // anchor-global, dc 0x90d10
TreasureData* advManager::getTreasureData(NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\events.cpp:656
// RETAIL_LOCATED(0x0049f070, 0x765)  // located @stub (promoted to active VA), dc 0x90d34
void advManager::doCustomArtifact(hero* current_hero, NewmapCell* cell, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:760
// RETAIL_LOCATED(0x0049f7e0, 0x2A4)  // located @stub (promoted to active VA), dc 0x91104
void advManager::doEventArtifact(hero* current_hero, NewmapCell* cell, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:852
// RETAIL_LOCATED(0x0049fa90, 0x106B)  // located @stub (promoted to active VA), dc 0x9138c
unsigned char advManager::giveBlackBoxReward(const char* text, hero* current_hero, NewmapCell* cell, type_point point, unsigned char human_player, BlackBoxData* BlackBox)
{
    // @stub
}

// E:\gamedcs\events.cpp:1114
// RETAIL_LOCATED(0x004a0c20, 0x23)  // anchor-global, dc 0x91bfc
BlackBoxData* advManager::getBlackBox(const ExtraInfoUnion* cell)
{
    // @stub
}

// E:\gamedcs\events.cpp:1456
// RETAIL_LOCATED(0x004a15a0, 0x301)  // located @stub (promoted to active VA), dc 0x925fc
void advManager::doEventCreatureBank(hero* current_hero, NewmapCell* cell, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:1505
// RETAIL_LOCATED(0x004a18b0, 0x79A)  // located @stub (promoted to active VA), dc 0x92814
void advManager::doEventCreatureGenerator(hero* current_hero, NewmapCell* cell, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:2029
// LOCATED 2026-08-14 from the monolith pair, which calls it as their only
// unaccounted events.obj callee: 0x4a2800 sits in the event_record..exec
// bracket that owns this compiland, ends `ret 0x10` against these four
// parameters, is 319 B against the DC's 346 (0.92 in the SH4->x86 band),
// and takes (hero*, NewmapCell*, packed point, human_player) in exactly
// this order. See the declarator note in advmgr.h.
// RETAIL_LOCATED(0x004a2800, 0x13F)  // anchor-caller, dc 0x939bc
void advManager::doEventHero(hero* current_hero, NewmapCell* cell, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:2101
// E:\gamedcs\events.cpp:2515
// RETAIL_LOCATED(0x004a3eb0, 0x376)  // located @stub (promoted to active VA), dc 0x94760
void advManager::doEventPrison(hero* current_hero, NewmapCell* cell, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:2746
// RETAIL_LOCATED(0x004a4780, 0x45D)  // located @stub (promoted to active VA), dc 0x94ea4
void advManager::doCustomResource(NewmapCell* cell, hero* current_hero, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:3039
// RETAIL_LOCATED(0x004a5610, 0x346)  // located @stub (promoted to active VA), dc 0x957fc
void advManager::doEventShrine(hero* current_hero, NewmapCell* cell, const char* prompt, GlobalInfoFlags type, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:3133
// RETAIL_LOCATED(0x004a5a80, 0x41E)  // located @stub (promoted to active VA), dc 0x95b54
void advManager::doCustomSpellScroll(hero* current_hero, NewmapCell* cell, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:3377
// RETAIL_LOCATED(0x004a6440, 0xD8)  // located @stub (promoted to active VA), dc 0x962dc
void advManager::doTreasureDialog(hero* current_hero, int amount, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:3579
// RETAIL_LOCATED(0x004a6b30, 0x12A)  // located @stub (promoted to active VA), dc 0x96994
void advManager::monstersGiveReward(hero* current_hero, NewmapCell* cell, unsigned char human_player)
{
    // @stub
}

// LOCATED 2026-08-14 by their caller. advManager::DoWanderingMonsterResult
// (0x4a7740, reconstructed and exact below) calls exactly four handlers,
// and every discriminator lines up at once:
//   * ORDER - the DC roster runs fight < flee < join < sell_out and the
//     four retail rows run 0x4a6c60 < 0x4a6df0 < 0x4a7000 < 0x4a7250,
//     inside the same bracket that already carries get_like_modifier,
//     get_force_modifier and DoWanderingMonsterResult in DC order.
//   * ARITY - the DC decorations give fight and flee four arguments and
//     void, join and sell_out five and bool; retail's two `ret 0x10` void
//     rows and two `ret 0x14` rows returning AL agree exactly, and the
//     fifth argument at both of the latter is the `sete`-materialised
//     want_to_fight byte.
//   * ROLE - the caller reaches 0x4a6c60 when the disposition BEATS the
//     mood total (fight), 0x4a7000 on the most favourable band (join),
//     0x4a7250 on the band above it (sell out), and 0x4a6df0 only when
//     the stack neither wants to fight nor carries the never-flee bit
//     (flee).
// ALL FOUR are now reconstructed and claimed below, which confirms the
// four-way pairing from the other side: each body's own text rows, its
// own dialog reply and its own philai callee are what the location
// predicted, and the four carcass stubs here are superseded.

// E:\gamedcs\events.cpp:3627
// RETAIL_LOCATED(0x004a6c60, 0x188)  // caller-arity + role, dc 0x96b14
void advManager::monstersFight(hero* current_hero, NewmapCell* cell, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:3655
// RETAIL_LOCATED(0x004a6df0, 0x20B)  // caller-arity + role, dc 0x96c18
void advManager::monstersFlee(hero* current_hero, NewmapCell* cell, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:3698
// RETAIL_LOCATED(0x004a7000, 0x248)  // caller-arity + role, dc 0x96d54
unsigned char advManager::monstersJoin(hero* current_hero, NewmapCell* cell, type_point point, unsigned char want_to_fight, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:3738
// RETAIL_LOCATED(0x004a7250, 0x36F)  // caller-arity + role, dc 0x96eec
unsigned char advManager::monstersSellOut(hero* current_hero, NewmapCell* cell, type_point point, unsigned char want_to_fight, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:3805, dc 0x97144
int advManager::getLikeModifier(hero* current_hero, TCreatureType creature)
{
    // @stub
}

// E:\gamedcs\events.cpp:3868
// RETAIL_LOCATED(0x004a7740, 0x27E)  // linkorder, dc 0x97364
void advManager::doWanderingMonsterResult(NewmapCell* cell, hero* current_hero, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:3948
// RETAIL_LOCATED(0x004a79c0, 0x79)  // linkorder, dc 0x975a0
void advManager::doEventWanderingMonster(NewmapCell* cell, hero* current_hero, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:3970
// RETAIL_LOCATED(0x004a7a40, 0x1EA)  // linkorder, dc 0x97628
void advManager::doEventWarSchool(hero* current_hero, NewmapCell* cell, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:4039
// RETAIL_LOCATED(0x004a7c30, 0x1A1)  // linkorder, dc 0x9784c
void advManager::doEventWarriorTomb(hero* current_hero, NewmapCell* cell, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:4090
// RETAIL_LOCATED(0x004a7de0, 0xB1)  // linkorder, dc 0x97a9c
void advManager::doEventWaterWheel(hero* current_hero, NewmapCell* cell, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:4116
// RETAIL_LOCATED(0x004a7ea0, 0x111)  // linkorder, dc 0x97b7c
void advManager::doEventWateringHole(hero* current_hero, NewmapCell* cell, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:4144
// RETAIL_LOCATED(0x004a7fc0, 0xBD)  // linkorder, dc 0x97cac
void advManager::doEventWindmill(hero* current_hero, NewmapCell* cell, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:4170
// RETAIL_LOCATED(0x004a8080, 0x1A5)  // linkorder, dc 0x97dc8
void advManager::doEventWitchHut(hero* current_hero, NewmapCell* cell, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:4207
// RETAIL_LOCATED(0x004a8230, 0x154)  // linkorder, dc 0x97fa4
void advManager::doEventLithOneWay(hero* current_hero, NewmapCell* cell, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:4244
// RETAIL_LOCATED(0x004a8390, 0x155)  // linkorder, dc 0x980e8
void advManager::doEventLithTwoWay(hero* current_hero, NewmapCell* cell, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:4282
// RESOLVED 2026-08-14 - THIS ROW IS NOT do_event_whirlpool.
// 0x4a84f0 is advManager::DispatchEvent; the body settles it outright:
//   * it opens `MobilizeCurrHero(1, 0, 0)` and then switches on
//     `cell->type` (the +0x1e dword) through a 214-entry byte index at
//     0x4aa95c into a jump table at 0x4aa7e4, biased by 2 - an object
//     dispatcher, which a whirlpool handler is not;
//   * it takes FOUR stack arguments in the Dreamcast's own order -
//     current_hero at +8, cell at +0xc, point at +0x10, human_player at
//     +0x14 - where do_event_whirlpool has three;
//   * it is EH-framed with a 0x3c8-byte frame, which is what DC's
//     DispatchEvent locals (three windows, three `msg` records, an
//     armyGroup) need and a 96-byte leaf does not;
//   * one of its arms inlines game::SetInfoFlag with the same
//     `[pos + gpGame + 0x1f879]` teamInfo scan the watering hole does;
//   * the size screen agrees: DC 6256 B -> retail 9538 B is 1.53x,
//     inside the SH4->x86 band.
// do_event_whirlpool (dc 0x981ec, 96 B) therefore has NO retail body: it
// is a static-shaped helper whose sole DC caller is DispatchEvent, i.e.
// the /Ob2 single-call-site shape, and it was inlined here.
// Left unclaimed only because the row is DispatchEvent-sized work.

// TABLES DECODED 2026-08-14, straight off the hash-verified image, and
// they PROVE the whirlpool claim above from the other side. The byte
// index at 0x4aa95c is 214 entries covering cell->type 0x02..0xd7; its
// values reach a 94-entry jump table at 0x4aa7e4 whose last arm
// (0x4aa7ce, 612 B) is the default 121 of those types share. Arm 0x6f -
// the whirlpool - is 89 bytes at 0x4a95a5 and calls 0x4acaa0,
// game::get_random_whirlpool (0x4cde20), StopCursor (0x47f7d0) and
// TeleportTo (0x41d930): a whole handler in line, with no call to any
// 96-byte leaf.

// FIFTY-SIX arms are a single call to one handler, which is what makes
// this body tractable in pieces - `type -> handler`, and every one of
// these targets is an events.obj-band row this file already carries or
// still owes:
//   0x3->49e670  0x4->49e7d0  0x5->49f7e0  0x6->4a0c50  0x8->4a1010
//   0xc->4a1120  0xf->4a14b0  0x10->4a15a0  0x11,0x14->4a18b0
//   0x16->4a5480  0x17->4a2050  0x19->4a2140  0x1d->4a2230
//   0x1e->4a2480  0x1f->4a25f0  0x20->4a2710  0x22->4a2800
//   0x26->4a12f0  0x27->4a31a0  0x29->4a3280  0x2b->4a8230
//   0x2d->4a8390  0x2f->4a33e0  0x30->4a3590  0x31->4a3730
//   0x33->4a38b0  0x35->4a39a0  0x37->4a3bc0  0x38->4a3ca0
//   0x3d->4a3dc0  0x3e->4a3eb0  0x3f->4a4230  0x40->4a44c0
//   0x4e->4a4600  0x4f->4a4be0  0x51->4a4dc0  0x52->4a5030
//   0x53->573670  0x56->4a52a0  0x5c->4a5980  0x5d->4a5ea0
//   0x5e->4a60a0  0x60->4a6200  0x63->5ea0c0  0x64->4a6330
//   0x65->4a6520  0x66->4a6710  0x69->4a69b0  0x6b->4a7a40
//   0x6c->4a7c30  0x6d->4a7de0  0x6e->4a7ea0  0x70->4a7fc0
//   0x71->4a8080  0xd5->5e9e60  0xd7->572b60
// Four of those are already exact in this file and they FIX the object
// ids outright: 0x22 is do_event_hero, 0x2b/0x2d the one-way and two-way
// liths, 0x6b DoEventWarSchool, 0x6c the warrior's tomb, 0x6d the water
// wheel, 0x6e the watering hole, 0x70 the windmill and 0x71 the witch
// hut - i.e. the tail of the alphabetical run advevent.txt indexes 158
// upward, in the same order.

// The remaining 37 arms carry inlined work. The ones worth naming ahead
// of a reconstruction attempt: 0x21 is the garrison (HasCreatures ->
// DoCombat -> game::ClaimGarrison), 0x23 the hill fort (THillFortWindow
// built, DoModal'd and destroyed in line), 0x36 the wandering stack
// (CompleteDraw/UpdateScreen then DoWanderingMonsterResult), 0x39 the
// obelisk (ViewPuzzle + ComputeUALoc), 0x2a a mine (game::ClaimMine),
// 0x5f the tavern (DoMapTavern), 0x62 a town (TownEvent), 0x67 the
// subterranean gate (game::get_underground_gate_exit, SetVisibility,
// do_event_hero, TeleportTo), 0x68 the university - which builds the
// window whose destructor is 0x4aaa40, the row the note below covers,
// and reaches ~CAdvPopup at 0x41b120 through it - and 0x6a the creature
// dwelling (recruitUnit constructed and run through executive::DoDialog).
// E:\gamedcs\events.cpp:4302
// MISATTRIBUTION 2026-08-14, RESOLVED - DO NOT RECONSTRUCT DispatchEvent
// AGAINST THIS ROW; see 0x4a84f0 above for where DispatchEvent really is.
// 0x4aaa40 is type_university_window::~type_university_window (dc
// 0x9ce08). The body is a
// destructor and nothing else: it takes no arguments, frees the two
// Dinkumware vectors at this+0xec and this+0xdc IN REVERSE DECLARATION
// ORDER through operator delete, zeroes all six pointers, and tail-calls
// 0x41b120 with the same `this` - and 0x41b120 stores ??_7CAdvPopup@@6B@
// and chains to ~heroWindow, so it is ~CAdvPopup. The Dreamcast has
// exactly one events.obj class that derives from CAdvPopup AND carries
// exactly two std::vector members: type_university_window, whose
// selection_widgets/purchase_widgets sit 12 apart under STLport and 16
// apart here - which is precisely 0xec - 0xdc. (type_sacrifice_window,
// the other CAdvPopup child in this obj, has eight.)
// Assigning the destructor to this address is ordinary locate work;
// it is left for a lane that models the window layout.
// RETAIL_LOCATED(0x004aaa40, 0x5C)  // body: 2-vector dtor + ~CAdvPopup, dc 0x9ce08
void advManager::dispatchEvent(hero* current_hero, NewmapCell* cell, type_point point, bool human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:5146
// RETAIL_LOCATED(0x004aaaa0, 0x110)  // linkorder, dc 0x99abc
void advManager::doEvent(NewmapCell* eventCell, type_point point)
{
    // @stub
}

// E:\gamedcs\events.cpp:5179
// RETAIL_LOCATED(0x004aabb0, 0x239)  // anchor-global, dc 0x99bac
void advManager::eraseObj(NewmapCell* thisCell, type_point point, unsigned char record)
{
    // @stub
}

// E:\gamedcs\events.cpp:5240
// RETAIL_LOCATED(0x004aadf0, 0x1DC)  // linkorder, dc 0x99d98
void advManager::heroSwap(hero* leftHero, hero* rightHero)
{
    // @stub
}

// E:\gamedcs\events.cpp:5264
// RETAIL_LOCATED(0x004aafd0, 0x431)  // linkorder, dc 0x99eb0
void advManager::townEvent(NewmapCell* cell, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:5390
// RETAIL_LOCATED(0x004ab410, 0x632)  // anchor-bracket, dc 0x9a288
void advManager::eventSound(int eventID, int extraInfo)
{
    // @stub
}

// E:\gamedcs\events.cpp:5590
// RETAIL_LOCATED(0x004aba50, 0x361)  // located @stub (promoted to active VA), dc 0x9a5b0
void advManager::generatorEvent(hero* who, NewmapCell* eventCell, type_point point)
{
    // @stub
}

// E:\gamedcs\events.cpp:5661
// RETAIL_LOCATED(0x004abdc0, 0x6D0)  // located @stub (promoted to active VA), dc 0x9a898
int advManager::creatureBankEvent(hero* who, NewmapCell* cell, char* cText, type_point point, unsigned char human_player)
{
    // @stub
}

// E:\gamedcs\events.cpp:5805
// RETAIL_LOCATED(0x004ac490, 0xEE)  // located @stub (promoted to active VA), dc 0x9adcc
void advManager::doEventUndeadLair(hero* current_hero, NewmapCell* cell, const char* question_text, const char* empty_text, const char* reward_text, unsigned long visited_flag, type_point point)
{
    // @stub
}

// E:\gamedcs\events.cpp:5851
// RETAIL_LOCATED(0x004ac580, 0x3A7)  // located @stub (promoted to active VA), dc 0x9af34
int advManager::combatMonsterEvent(hero* who, TCreatureType monType, int* numMons, NewmapCell* eventCell, type_point point, TCreatureType monType2, int numMons2, int numGroups2, TCreatureType monType3, int numMons3, int numGroups3)
{
    // @stub
}

// E:\gamedcs\events.cpp:6007
// LOCATED 2026-08-07 from ai_combat: adjust_army's dc callgraph lists
// exactly one non-STL callee besides armyGroup::Dismiss - HeroLoses -
// and retail's adjust_army (0x4248c5) calls 0x4ac930 with ecx =
// gpAdvManager (0x699268) and (hero, 0). The body deallocates the hero
// and switches on the second argument's 0/1 to pick the
// "pickup%02d.82M" vanish sample, which is what `int vanish_sound`
// names. Sits inside events.obj's link bracket [0x4ab410..0x4acbb0].
// RETAIL_LOCATED(0x004ac930, 0x163)  // anchor-callee, dc 0x9b3b4
void advManager::heroLoses(hero* who, int vanish_sound)
{
    // @stub
}

// E:\gamedcs\events.cpp:6030
// RETAIL_LOCATED(0x004acaa0, 0x106)  // located @stub (promoted to active VA), dc 0x9b448
void advManager::doWhirlpool(hero* who)
{
    // @stub
}

// E:\gamedcs\events.cpp:6076
// RETAIL_LOCATED(0x004acbb0, 0xE4)  // anchor-global, dc 0x9b564
void advManager::fizzleCenter(int whichSound)
{
    // @stub
}

// E:\gamedcs\events.cpp:6118
// RETAIL_LOCATED(0x004acca0, 0xC3)  // located @stub (promoted to active VA), dc 0x9b670
void advManager::doAIEvent(NewmapCell* cell, hero* current_hero, type_point point)
{
    // @stub
}

// E:\gamedcs\events.cpp:6149
// RETAIL_LOCATED(0x004acd70, 0x365)  // located @stub (promoted to active VA), dc 0x9b788
int advManager::doNetCombat(CCombatInitMsg* pCombatInitMsg)
{
    // @stub
}

// E:\gamedcs\events.cpp:6283
// RETAIL_LOCATED(0x004ad470, 0x1531)  // located @stub (promoted to active VA), dc 0x9b970
int advManager::doCombat(type_point point, hero* leftHero, armyGroup* leftArmyGroup, long iRightPlayer, town* rightTown, hero* rightHero, armyGroup* rightArmyGroup, int iSeed, unsigned char bFinishHeroes, unsigned char alternate_layout)
{
    // @stub
}

// E:\gamedcs\events.cpp:6725
// RETAIL_LOCATED(0x004aeb50, 0x390)  // located @stub (promoted to active VA), dc 0x9c35c
void advManager::sendHeroTownData(type_point point, hero* leftHero, armyGroup* leftArmyGroup, long right_player, town* rightTown, hero* rightHero, armyGroup* rightArmyGroup, int iSeed, int toWhoNetPos, int iWinner, unsigned char bRetreatWin, unsigned char bCombatSurrender)
{
    // @stub
}

// E:\gamedcs\events.cpp:6781
// RETAIL_LOCATED(0x004aeee0, 0x3DF)  // located @stub (promoted to active VA), dc 0x9c554
void advManager::receiveHeroTownData(CCombatInitMsg* pCombatInitMsg, int* iFromWho, type_point* point, hero** leftHero, armyGroup** leftArmyGroup, int* right_player, town** rightTown, hero** rightHero, armyGroup** rightArmyGroup, int* iSeed, signed char* iWinner, unsigned char* bRetreatWin, unsigned char* bCombatSurrender)
{
    // @stub
}

// E:\gamedcs\events.cpp:5140
// RETAIL_LOCATED(0x004aaa40, 0x5C)  // located @stub (promoted to active VA), dc 0x9ce08
void type_university_window::~type_university_window()
{
    // @stub
}

#endif  // @carcass

// The three adventure text resources events.obj loads at startup, in
// retail .bss order. Names are the Dreamcast's own loader names carried
// onto the cells each loader stores into; PROVISIONAL as spellings, but
// the identity is byte-proven - 0x49e0e0 stores the ResourceManager
// return for "advevent.txt" (0x677710) into 0x696a18 and every one of
// the 194 references to that cell is inside events.obj's link bracket.
DATA(0x00696a18) static TTextResource* g_adventureEventText;
DATA(0x00696a1c) static TTextResource* g_randomSignTextResource;
DATA(0x00696a2c) static const char* g_artifactEventText[144];
DATA(0x00696c70) static TTextResource* g_artifactEventTextResource;
DATA(0x00696c74) static const char* g_randomSignText[25];

VA(0x0049e0e0, 0x15)  // dc 0x9028c
bool initializeAdventureEventText()
{
    g_adventureEventText = ResourceManager::getText(
        DATA_COMPGEN(0x00677710, advEventTextName, "advevent.txt"));
    if (!g_adventureEventText)
        return false;
    return true;
}

VA(0x0049e100, 0x33)  // dc 0x902b0
bool initializeArtifactEventText()
{
    g_artifactEventTextResource = ResourceManager::getText(
        DATA_COMPGEN(0x00677720, artEventTextName, "artevent.txt"));
    if (!g_artifactEventTextResource)
        return false;
    for (int i = 0; i < 144; i++)
        g_artifactEventText[i] = g_artifactEventTextResource->getText(i);
    return true;
}

VA(0x0049e140, 0x30)  // dc 0x902fc
bool initializeRandomSignText()
{
    g_randomSignTextResource = ResourceManager::getText(
        DATA_COMPGEN(0x00677730, randomSignTextName, "randsign.txt"));
    if (!g_randomSignTextResource)
        return false;
    for (int i = 0; i < 25; i++)
        g_randomSignText[i] = g_randomSignTextResource->getText(i);
    return true;
}

// The flash itself is FizzleCenter (0x4acbb0) inlined, exactly as in
// HeroLoses: the callee's own body is at the far end of this file and is
// emitted anyway, but /Ob2 expands it at both sites.
VA(0x0049e170, 0x16E)  // dc 0x90348
void advManager::eraseAndFizzle(NewmapCell* eventCell, type_point point,
                                int fizzleSound)
{
    unsigned char savedFlag = g_colorCyclingEnabled;
    g_colorCyclingEnabled = 0;
    unsigned char savedPause = m_animCtrPaused;
    m_animCtrPaused = 1;

    completeDraw(false);
    updateScreen(0, 0);
    eraseObj(eventCell, point, 1);
    fizzleCenter(fizzleSound);

    m_animCtrPaused = savedPause;
    g_colorCyclingEnabled = savedFlag;
}

// E:\gamedcs\events.cpp:317.
// [2026-08-27] Residual (91.93%): retail packs boat_point's three
// bitfields with an interleaved XOR-into-word idiom reading z through the
// point parameter's own packed dword (4*p / sar 2); our field-by-field
// short stores are close but order the packing differently. The 3-arg
// type_point constructor measured WORSE (85.51). A packed-bitfield store
// ordering wall.
VA(0x0049e2e0, 0x38B)  // dc-bracket forced, ret 0xc=p4, dc 0x903b4
void advManager::doEventShipyard(NewmapCell* cell, type_point point, unsigned char humanPlayer)
{
    mobilizeCurrHero(0, 0, 1);

    int owner = static_cast<int>(cell->m_extraInfo << 24) >> 24;
    if (!g_game->onSameTeam(owner, g_netLocalGamePos))
        g_game->claimShipyard(point, g_netLocalGamePos);

    if (!humanPlayer)
        return;

    type_point boatPoint;
    boatPoint.m_x = static_cast<short>((cell->m_extraInfo >> 8) & 0xff);
    boatPoint.m_y = static_cast<short>((cell->m_extraInfo >> 16) & 0xff);
    boatPoint.m_z = static_cast<short>(point.m_z);

    const int noBoatPosition = 0xff;
    if (boatPoint.m_x == noBoatPosition) {
        if (!g_game->getCurrHero()) {
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_ABANDONED_SHIPYARD],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        } else {
            normalDialog(formatString((*g_generalText)[GENERAL_TEXT_SHIPYARD_BLOCKED_FORMAT],
                         g_game->getCurrHero()->m_name).c_str(),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
        return;
    }

    NewmapCell* boatCell = g_game->getCell(boatPoint);

    if (boatCell->m_isTrigger
        && (boatCell->m_type == BOAT || boatCell->m_type == HERO
            || g_game->getBoatsBuilt() >= 64)) {
        normalDialog((*g_generalText)[GENERAL_TEXT_BOAT_BUILD_BLOCKED],
                     1, 208, 40, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    if (g_game->m_players[g_netLocalGamePos].m_resources[GOLD] < 1000
        || g_game->m_players[g_netLocalGamePos].m_resources[WOOD] < 10) {
        normalDialog((*g_generalText)[GENERAL_TEXT_BOAT_PURCHASE_CANNOT_AFFORD],
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    doShipyard(0);
    if (g_windowManager->m_dialogReturn == DIALOG_RETURN_OK
        && g_game->createBoat(
               static_cast<unsigned char>(cell->m_extraInfo >> 8),
               static_cast<unsigned char>(cell->m_extraInfo >> 16),
               boatPoint.m_z, g_netLocalGamePos, 0, 1) != -1) {
        g_game->m_players[g_netLocalGamePos].m_resources[GOLD] -= 1000;
        g_game->m_players[g_netLocalGamePos].m_resources[WOOD] -= 10;
    }
}

VA(0x0049e670, 0x15C)  // dc 0x90658
void advManager::doEventAnchor(hero* currentHero, bool humanPlayer)
{
    if (currentHero->m_flags & 0x40000) {
        currentHero->m_flags &= ~0x40000;
        if (!(currentHero->m_flags & 0x1000000)) {
            if (currentHero->isWieldingArtifact(0x88)) {
                int oldMaxMovePoints = currentHero->m_maxMovePoints;
                int oldMovePoints = currentHero->m_movePoints;
                int newMaxMovePoints = currentHero->getMobility(0);
                currentHero->m_maxMovePoints = newMaxMovePoints;
                currentHero->m_movePoints =
                    newMaxMovePoints * oldMovePoints / oldMaxMovePoints;
            } else {
                currentHero->m_movePoints = 0;
            }
            m_advWindow->updateHeroLocator(-1, 1, 1);
        }

        currentHero->m_facing = m_cursorDirection;
        m_cursorType = CURSOR_TYPE_34;
        m_cursorBaseFrame = 0;
        m_cursorSequence = currentHero->getStandSequence();
        m_drawCursor = 1;
        fizzleCenter(FIZZLE_SOUND_KILL_FADE);

        int foughtBattle;
        checkAdjacentMon(&foughtBattle);
    }
}

// The visit lane is the cell's, not a hero flag word: hero::VisitedArena
// (0x4e53c0) and SetVisitedArena (0x4e53e0) index a per-hero dword by the
// cell's own extra-info dword, which is the idiom the six "once per hero,
// keep the reward" handlers borrow.

// advevent.txt row 0 is the prompt and row 1 the already-fought line -
// the first two rows in the file, because "arena" is the first adventure
// object alphabetically. That is the strongest single check on the
// ordering this enum rests on.
// Original: advManager::EventSound; events.cpp:412, dc 0x906c0.
void advManager::eventSound(NewmapCell* cell)
{
    eventSound(cell->m_type, cell->m_extraInfo);
}

VA(0x0049e7d0, 0x118)  // dc 0x906d8
void advManager::doEventArena(hero* currentHero, NewmapCell* cell,
                              bool humanPlayer)
{
    if (currentHero->visitedArena(cell)) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_ARENA_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    int whichStat = 0;
    if (humanPlayer) {
        overrideBottomView(BOTTOM_VIEW_DEFAULT, -1);
        updBottomView(0, 1, 1);
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_ARENA],
                     10, -1, -1, 0x1f, 2, 0x20, 2, -1, 0, -1, 0);
        switch (g_windowManager->m_dialogReturn) {
        case DIALOG_RETURN_CHOICE_1:
            whichStat = 0;
            break;
        case DIALOG_RETURN_CHOICE_2:
            whichStat = 1;
            break;
        }
    } else if (currentHero->getPrimarySkill(0)
               > currentHero->getPrimarySkill(1)) {
        whichStat = 1;
    }

    currentHero->adjustPrimarySkill(whichStat, 2);
    currentHero->setVisitedArena(cell);
}

void aiEquipArtifacts(hero* currentHero);

long aiValueOfEvent(const hero* currentHero, type_point point);

// It takes the POINT, not the cell, and re-fetches through GetCell -
// which is what its `push point / call GetCell` opening says and what
// makes it usable from an arm that has already lost the cell pointer.

VA(0x0049e8f0, 0x146)  // dc 0x90814
void advManager::giveArtifact(hero* currentHero, type_point point,
                              bool humanPlayer)
{
    NewmapCell* cell = getCell(point);

    type_artifact artifact(ARTIFACT_NONE);
    artifact.m_artifactId = cell->getArtifactIndex();
    currentHero->giveArtifact(&artifact, 1, 1);
    if (!humanPlayer)
        aiEquipArtifacts(currentHero);

    eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
    currentHero->checkLevel();
}

// E:\gamedcs\events.cpp:498. Dreamcast proves this private helper, its
// short `artifact` local and statement order. Mac doEventArtifact retains
// a call to it at 0:0xa9d28; VC6 auto-inlines it into the free arm.
void advManager::doEventFreeArtifact(hero* currentHero,
                                            NewmapCell* cell,
                                            type_point point,
                                            bool humanPlayer)
{
    short artifact = cell->getArtifactIndex();
    if (humanPlayer)
        normalDialog(g_artifactEventText[artifact],
                     1, -1, -1, 8, artifact, -1, 0, -1, 0, -1, 0);
    giveArtifact(currentHero, point, humanPlayer);
}

VA(0x0049ea40, 0x304)  // dc 0x908dc
void advManager::fightForArtifact(hero* currentHero, NewmapCell* cell,
                                  type_point point, bool humanPlayer)
{
    int monsterType = cell->getArtifactDefender();
    int amount = (cell->m_extraInfo >> 17) & 0x3fff;
    short artifact = cell->getArtifactIndex();

    if (humanPlayer) {
        overrideBottomView(BOTTOM_VIEW_DEFAULT, -1);
        updBottomView(0, 1, 1);
        sprintf(g_text, (*g_generalText)[GENERAL_TEXT_GUARDED_ARTIFACT_PROMPT_FORMAT],
                armyGroup::getArmySizeName(amount, 2),
                getArmyName(monsterType, 2),
                getArmyName(monsterType, 2));
        normalDialog(g_text, 2, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT) {
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_ARTIFACT_FIGHT_DECLINED],
                         1, -1, -1, -1, 0,
                         -1, 0, -1, 0, -1, 0);
            return;
        }
    } else if (aiValueOfEvent(currentHero, point) <= 0) {
        return;
    }

    if (combatMonsterEvent(currentHero, monsterType, &amount, cell, point,
                           CREATURE_NONE, 0, 0,
                           CREATURE_NONE, 0, 0))
        return;

    currentHero->checkLevel();
    if (humanPlayer) {
        sprintf(g_text, (*g_adventureEventText)[ADV_EVENT_TEXT_CUSTOM_GUARDED_REWARD_FORMAT],
                g_artifactTraits[artifact].m_name);
        normalDialog(g_text, 1, -1, -1,
                     8, artifact, -1, 0, -1, 0, -1, 0);
    }
    giveArtifact(currentHero, point, humanPlayer);
}

VA(0x0049ed50, 0x2E8)  // dc 0x90ad8
void advManager::payForArtifact(hero* currentHero, NewmapCell* cell,
                                type_point point, const char* dialogText,
                                short goldCost, short resourceCost,
                                bool humanPlayer)
{
    int resourceType = cell->getArtifactResourceCost();

    if (humanPlayer) {
        short artifact = cell->getArtifactIndex();
        if (resourceCost > 0) {
            char resourceName[50];
            strcpy(resourceName, g_resourceNames[resourceType]);
            resourceName[0] = tolower(resourceName[0]);
            sprintf(g_text, dialogText,
                    g_artifactTraits[artifact].m_name,
                    resourceName);
        } else {
            sprintf(g_text, dialogText,
                    g_artifactTraits[artifact].m_name);
        }
        normalDialog(g_text, 2, -1, -1, 8, artifact,
                     -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT) {
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_ARTIFACT_DECLINED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            return;
        }
    } else if (aiValueOfEvent(currentHero, point) <= 0) {
        return;
    }

    long* resources = g_game->m_players[currentHero->m_owner].m_resources;
    if (resources[GOLD] < goldCost ||
        resources[resourceType] < resourceCost) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_ARTIFACT_CANNOT_AFFORD],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    resources[GOLD] -= goldCost;
    resources[resourceType] -= resourceCost;
    giveArtifact(currentHero, point, humanPlayer);
}

VA(0x0049f040, 0x23)  // decorated identity + event-pool index arithmetic
TreasureData* advManager::getTreasureData(NewmapCell* cell) const
{
    unsigned index = (cell->m_extraInfo >> 19) & 0xfff;
    return &m_fullMap->m_customTreasure[index];
}

// E:\gamedcs\events.cpp:656.  The customised artifact: the editor record
// may put a message OR a composed guardian roster in front of the pickup.
// The Dreamcast publishes the locals - treasure, the short artifact id,
// the four message strings (first_guard_amount / guards / first_guard /
// msg, declared up front and assigned after, which is what keeps _Tidy
// and the assigns out of line) and the guard_list armyGroup. Both exits
// inline advManager::GiveArtifact whole and cross-jump onto one final
// CheckLevel; the guarded copy keeps FizzleCenter as a call and the
// plain one folds it, exactly as DoCustomSpellScroll's pair does.
// [2026-09-01] Dreamcast's breakpoint rows make the declaration order
// positive source evidence: treasure is line 657 and artifactId line 659.
// Keep that order even though this SP3 compile currently scores below the
// old source-false local peak (current 98.24%, banked MAX 99.15%): retail
// interleaves the fullMap/_First chain with the cell loads, while SP3
// serializes the inlined accessor. Negative control: putting artifactId
// first raises the byte score but contradicts those two named statement
// rows, so it is not an admissible reconstruction.
// Negative control: spelling DC's unsigned-char human_player literally changes
// the x86 decorated identity; retail's `_N` suffix proves this parameter is bool.
// Splitting artifactId's declaration from its accessor assignment is byte-flat.
VA(0x0049f070, 0x765)  // dc-bracket forced, ret 0x10=p5, dc 0x90d34
void advManager::doCustomArtifact(hero* currentHero, NewmapCell* cell,
                                  type_point point, bool humanPlayer)
{
    TreasureData* treasure = getTreasureData(cell);
    short artifactId = cell->getArtifactIndex();

    if (treasure->m_hasCustomGuardians && treasure->m_guardians.getNumArmies()) {
        if (humanPlayer) {
            if (treasure->m_message.length() > 0) {
                overrideBottomView(BOTTOM_VIEW_DEFAULT, -1);
                updBottomView(0, 1, 1);
                normalDialog(treasure->m_message.c_str(),
                             2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            } else {
                std::string firstGuardAmount;
                std::string guards;
                std::string firstGuard;
                std::string msg;
                armyGroup guardList;
                guardList.initialize();
                for (int i = 0; i < 7; i++) {
                    if (treasure->m_guardians.m_armies[i] != CREATURE_NONE)
                        guardList.add(treasure->m_guardians.m_armies[i],
                                       treasure->m_guardians.m_numTroops[i], -1);
                }
                long numArmies = guardList.getNumArmies();
                firstGuardAmount =
                    armyGroup::getArmySizeName(guardList.m_numTroops[0], 2);
                guards = getArmyName(guardList.m_armies[0], 2);
                firstGuard = guards;
                if (numArmies > 1) {
                    firstGuard = (*g_generalText)[GENERAL_TEXT_GENERIC_CREATURE_PLURAL];
                    for (int i = 1; i < numArmies; i++) {
                        if (i == numArmies - 1)
                            guards += (*g_generalText)[GENERAL_TEXT_LIST_AND];
                        else
                            guards += ", ";
                        guards += armyGroup::getArmySizeName(
                            guardList.m_numTroops[i], 2);
                        guards += " ";
                        guards += getArmyName(guardList.m_armies[i], 2);
                    }
                }
                msg = formatString((*g_generalText)[GENERAL_TEXT_GUARDED_ARTIFACT_PROMPT_FORMAT],
                                    firstGuardAmount.c_str(),
                                    guards.c_str(), firstGuard.c_str());
                normalDialog(msg.c_str(),
                             2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            }
            if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
                return;
        } else if (aiValueOfEvent(currentHero, point) <= 0) {
            return;
        }

        if (doCombat(point, currentHero, &currentHero->m_army, -1, 0, 0,
                     &treasure->m_guardians, -1, 1, 0))
            return;
        currentHero->checkLevel();
        if (humanPlayer) {
            sprintf(g_text,
                    (*g_adventureEventText)[ADV_EVENT_TEXT_CUSTOM_GUARDED_REWARD_FORMAT],
                    g_artifactTraits[artifactId].m_name);
            normalDialog(g_text, 1, -1, -1, 8, artifactId,
                         -1, 0, -1, 0, -1, 0);
        }
        giveArtifact(currentHero, point, humanPlayer);
        return;
    }

    if (humanPlayer) {
        if (treasure->m_message.length() > 0)
            normalDialog(treasure->m_message.c_str(),
                         1, -1, -1, 8, artifactId, -1, 0, -1, 0, -1, 0);
        else
            normalDialog(g_artifactEventText[artifactId],
                         1, -1, -1, 8, artifactId, -1, 0, -1, 0, -1, 0);
    }
    giveArtifact(currentHero, point, humanPlayer);
}

// Dreamcast events.cpp:629-641 emits DoArtifactSkillRequirement as a
// separate helper; Mac doEventArtifact calls the same body at 0:0xaa2bc
// twice. VC6 auto-inlines the ordinary helper at both retail call sites.
// Its success arm calls DoEventFreeArtifact and its refusal names a short
// artifact; retail's skill-success dialogs read g_artifactEventText.
void advManager::doArtifactSkillRequirement(
    hero* currentHero, NewmapCell* cell, type_point point,
    int skill, const char* dialogText, bool humanPlayer)
{
    if (currentHero->m_skillLevel[skill]) {
        doEventFreeArtifact(currentHero, cell, point, humanPlayer);
    } else if (humanPlayer) {
        short artifact = cell->getArtifactIndex();
        sprintf(g_text, dialogText, g_artifactTraits[artifact].m_name);
        normalDialog(g_text, 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
    }
}

// E:\gamedcs\events.cpp:760. The Dreamcast signature and helper roster
// identify the source surface; retail fixes the price-arm order, costs and
// text indices. This is the ordinary artifact event dispatcher.
// Residual (85.81%, 2026-09-07): retail shares the two skill-success
// dialog/GiveArtifact tails; VC6 still emits separate copies. The free helper's
// short artifact load also stays before the human test instead of sinking.
// Restoring DoArtifactSkillRequirement's nested DoEventFreeArtifact call and
// short refusal local fixes the dialog semantics. With the old GiveArtifact
// reconstruction this measured 0%; recovering its proven constructor and
// accessor reaches 85.81439. Either redundant sentinel stores or the old
// memcpy assignment alone reproduces that 672-byte non-expanded control.
// The canonical 752-byte caller expands GiveArtifact in the free arm at
// cost/budget 113/113, retaining calls in the skill arms at budgets 6 and 4,
// exactly the retail call decisions. MAX before this reconstruction: 78.1174.
VA(0x0049f7e0, 0x2A4)  // anchor-callee DoCustomArtifact+FightForArtifact, ret 0x10=p5, dc 0x91104
void advManager::doEventArtifact(hero* currentHero, NewmapCell* cell,
                                 type_point point, bool humanPlayer)
{
    if (currentHero->getNumberInBackpack(1) >= 64) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_BACKPACK_FULL],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    if (cell->isCustomized()) {
        doCustomArtifact(currentHero, cell, point, humanPlayer);
        return;
    }
    if (cell->isDefendedArtifact()) {
        fightForArtifact(currentHero, cell, point, humanPlayer);
        return;
    }

    switch (cell->getArtifactPrice()) {
    case const_free_artifact:
        doEventFreeArtifact(currentHero, cell, point, humanPlayer);
        break;
    case const_artifact_requires_wisdom:
        doArtifactSkillRequirement(
            currentHero, cell, point, eSecSkillWisdom,
            (*g_adventureEventText)[ADV_EVENT_TEXT_ARTIFACT_WISDOM_FORMAT],
            humanPlayer);
        break;
    case const_artifact_requires_leadership:
        doArtifactSkillRequirement(
            currentHero, cell, point, eSecSkillLeadership,
            (*g_adventureEventText)[ADV_EVENT_TEXT_ARTIFACT_LEADERSHIP_FORMAT],
            humanPlayer);
        break;
    case const_artifact_costs_2000:
        payForArtifact(currentHero, cell, point,
                       (*g_adventureEventText)[ADV_EVENT_TEXT_ARTIFACT_COST_2000_FORMAT],
                       2000, 0, humanPlayer);
        break;
    case const_artifact_costs_2500:
        payForArtifact(currentHero, cell, point,
                       (*g_adventureEventText)[ADV_EVENT_TEXT_ARTIFACT_COST_2500_FORMAT],
                       2500, 3, humanPlayer);
        break;
    case const_artifact_costs_3000:
        payForArtifact(currentHero, cell, point,
                       (*g_adventureEventText)[ADV_EVENT_TEXT_ARTIFACT_COST_3000_FORMAT],
                       3000, 5, humanPlayer);
        break;
    }
}

// The two joiner reports GiveBlackBoxReward's creature tail needs before
// their own consumers further down declare them: philai.obj's
// AI_join_decision (0x52bc60) and townmgr.obj's armyGroup-overload join
// dialog (0x5d11d0 family). Both declarations repeat verbatim at the
// monsters_* family below.
void aiJoinDecision(hero* currentHero, TCreatureType creature,
                      short amount);
void doMonsterJoinDialog(hero* inHero, armyGroup* monsters, int flag);

static void showRewards(std::string& text,
                        std::vector<type_dialog_resource>& rewards,
                        long threshold);

// DC events.cpp:838-846, dc 0x91308: ordinary static add_reward owns
// push_back, the empty-text assignment, and show_rewards(..., 8).
// Retail expands this helper and push_back while retaining insert calls.
// Moving the flush into callers loses the nested inlining context.
static void addReward(std::string& text, const std::string& alternate,
                      std::vector<type_dialog_resource>& rewards,
                      EGameResource resource, long qualifier)
{
    type_dialog_resource reward;
    reward.m_resource = resource;
    reward.m_qualifier = qualifier;
    rewards.push_back(reward);
    if (text.length() == 0)
        text = alternate;
    showRewards(text, rewards, 8);
}

// E:\gamedcs\events.cpp:852
VA(0x0049fa90, 0x106B)  // dc-bracket forced, ret 0x18=p7 + format_string reward text, dc 0x9138c
unsigned char advManager::giveBlackBoxReward(const char* text, hero* currentHero,
    NewmapCell* cell, type_point point, unsigned char humanPlayer,
    BlackBoxData* blackBox)
{
    long exp = 0;
    unsigned char gave = 0;
    std::string message(text);
    std::string alternate;
    std::vector<type_dialog_resource> rewards;
    alternate = formatString((*g_adventureEventText)[ADV_EVENT_TEXT_BLACK_BOX_REWARD_FORMAT],
                              currentHero->m_name);

    if (blackBox->m_experienceBonus > 0) {
        exp = currentHero->getExperienceBonusFactor()
              * blackBox->m_experienceBonus;
        if (humanPlayer) {
            addReward(message, alternate, rewards, RES_EXPERIENCE, exp);
        }
        gave = 1;
    }

    for (int i = 0; i < 4; i++) {
        if (blackBox->m_primarySkillBonus[i] > 0) {
            if (humanPlayer) {
                int rewardType;
                rewardType = RES_PRIMARY_SKILL_ATTACK + i;
                addReward(message, alternate, rewards,
                          EGameResource(rewardType),
                           blackBox->m_primarySkillBonus[i]);
            }
            gave = 1;
            currentHero->adjustPrimarySkill(i, blackBox->m_primarySkillBonus[i]);
        }
    }

    for (unsigned int j = 0; j < blackBox->m_secondarySkills.size(); j++) {
        TSecondarySkill skill = blackBox->m_secondarySkills[j].m_type;
        TSkillMastery level = blackBox->m_secondarySkills[j].m_level;
        unsigned char skillGiven = 0;
        if (currentHero->m_skillLevel[skill] == 0 && currentHero->m_skillCount < 8) {
            currentHero->giveSS(skill, level);
            skillGiven = 1;
        } else if (currentHero->m_skillLevel[skill] > 0
                   && currentHero->m_skillLevel[skill] < level) {
            currentHero->giveSS(skill, level - currentHero->m_skillLevel[skill]);
            skillGiven = 1;
        }
        if (!skillGiven)
            continue;
        if (humanPlayer) {
            addReward(message, alternate, rewards, RES_SECONDARY_SKILL,
                       skill * 3 + level + 2);
        }
        gave = 1;
    }

    long mana = blackBox->m_manaBonus;
    if (mana != 0) {
        if (currentHero->m_mana + mana > 999) {
            mana = 999 - currentHero->m_mana;
        } else if (currentHero->m_mana + mana < 0) {
            mana = -currentHero->m_mana;
            alternate = formatString(g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_LOSE_MANA_FORMAT),
                                      currentHero->m_name);
        } else {
            alternate = formatString(g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_GAIN_MANA_FORMAT),
                                      currentHero->m_name);
        }
        if (humanPlayer && mana != 0) {
            addReward(message, alternate, rewards, RES_MANA, mana);
        }
        currentHero->m_mana += mana;
        gave = 1;
    }

    if (blackBox->m_moraleBonus != 0) {
        if (humanPlayer) {
            if (blackBox->m_moraleBonus < 0) {
                addReward(message, formatString(
                    g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_LOSE_MORALE_FORMAT),
                    currentHero->m_name), rewards, RES_BAD_MORALE,
                           blackBox->m_moraleBonus);
            } else {
                addReward(message, formatString(
                    g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_GAIN_MORALE_FORMAT),
                    currentHero->m_name), rewards, RES_GOOD_MORALE,
                           blackBox->m_moraleBonus);
            }
        }
        gave = 1;
        currentHero->m_moraleBonus += blackBox->m_moraleBonus;
    }

    if (blackBox->m_luckBonus != 0) {
        if (humanPlayer) {
            if (blackBox->m_luckBonus < 0) {
                addReward(message, formatString(
                    g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_LOSE_LUCK_FORMAT),
                    currentHero->m_name), rewards, RES_BAD_LUCK,
                           blackBox->m_luckBonus);
            } else {
                addReward(message, formatString(
                    g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_GAIN_LUCK_FORMAT),
                    currentHero->m_name), rewards, RES_GOOD_LUCK,
                           blackBox->m_luckBonus);
            }
        }
        gave = 1;
        currentHero->m_luckBonus += blackBox->m_luckBonus;
    }
    showRewards(message, rewards, 1);

    for (int k = 0; k < 7; k++) {
        if (blackBox->m_resQty[k] != 0) {
            if (humanPlayer) {
                int rewardType;
                rewardType = k;
                if (blackBox->m_resQty[k] > 0) {
                    addReward(message, formatString(
                        g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_GAIN_TREASURE_FORMAT),
                        currentHero->m_name), rewards, EGameResource(rewardType),
                               blackBox->m_resQty[k]);
                } else {
                    addReward(message, formatString(
                        g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_LOSE_TREASURE_FORMAT),
                        currentHero->m_name), rewards, EGameResource(rewardType),
                               blackBox->m_resQty[k] - 100000);
                }
            }
            currentHero->giveResource(k, blackBox->m_resQty[k]);
            gave = 1;
        }
    }
    showRewards(message, rewards, 1);

    type_artifact artifact(ARTIFACT_NONE);
    for (unsigned int m = 0; m < blackBox->m_artifacts.size(); m++) {
        if (currentHero->getNumberInBackpack(1) < 64) {
            if (humanPlayer) {
                addReward(message, formatString(
                    g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_GAIN_TREASURE_FORMAT),
                    currentHero->m_name), rewards, RES_ARTIFACT,
                           blackBox->m_artifacts[m]);
            }
            artifact.m_artifactId = blackBox->m_artifacts[m];
            currentHero->giveArtifact(&artifact, 1, 1);
            if (!humanPlayer)
                aiEquipArtifacts(currentHero);
            gave = 1;
        }
    }
    showRewards(message, rewards, 1);

    if (currentHero->isWieldingArtifact(0)) {
        for (unsigned int n = 0; n < blackBox->m_spells.size(); n++) {
            if (g_spellTraits[blackBox->m_spells[n]].m_level
                    <= currentHero->m_skillLevel[eSecSkillWisdom] + 2
                && !currentHero->isInSpellbook(blackBox->m_spells[n])) {
                if (humanPlayer) {
                    if (rewards.size() != 0) {
                        std::string pendingText = formatString(
                            g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_LEARN_SPELLS_FORMAT),
                            currentHero->m_name);
                        message = pendingText;
                    }
                    addReward(message, formatString(
                        g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_LEARN_SPELL_FORMAT),
                        currentHero->m_name), rewards, RES_SPELL,
                               blackBox->m_spells[n]);
                }
                currentHero->addSpell(blackBox->m_spells[n]);
                gave = 1;
            }
        }
    }
    showRewards(message, rewards, 1);

    unsigned char joinFailed = 0;
    armyGroup creatures = blackBox->m_creatures;
    // The retail loop-back uses signed jl; an unsigned index emits jb.
    for (int p = 0; p < 7; p++) {
        int type = creatures.m_armies[p];
        int count = creatures.m_numTroops[p];
        if (type == CREATURE_NONE)
            continue;
        if (humanPlayer) {
            if (count == 1)
                alternate = formatString(
                    g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_CREATURE_JOINS_FORMAT),
                    getArmyName(type, 1), currentHero->m_name);
            else
                alternate = formatString(
                    g_adventureEventText->getText(ADV_EVENT_TEXT_BLACK_BOX_CREATURES_JOIN_FORMAT),
                    getArmyName(type, 2), currentHero->m_name);
            addReward(message, alternate, rewards, RES_MONSTER,
                       ((count & 0xffff) << 16) | (type & 0xffff));
        }
        if (currentHero->m_army.add(type, count, -1)) {
            creatures.dismiss(p);
        } else if (humanPlayer) {
            joinFailed = 1;
        } else {
            int storage;
            storage = type;
            aiJoinDecision(currentHero, TCreatureType(storage), count);
        }
        gave = 1;
    }

    if (humanPlayer) {
        if (message.length() > 0)
            extendedDialog(message.c_str(), rewards, -1, -1, 0);
    }
    if (joinFailed)
        doMonsterJoinDialog(currentHero, &creatures, 0);
    if (exp > 0)
        currentHero->giveExperience(exp, 1, 1);
    if (humanPlayer)
        m_advWindow->updateHeroLocators(-1, 1, 1);
    updBottomView(1, 1, 1);
    return gave;
}

VA_COMPGEN(0x0054c120, 0x43, VECTOR_CLEAR, type_dialog_resource)

VA_COMPGEN(0x005b8cc0, 0x0f, STD_CONSTRUCT, type_dialog_resource)

VA(0x004a0b00, 0x112)  // dc 0x912bc
static void showRewards(std::string& text,
                  std::vector<type_dialog_resource>& rewards,
                  long threshold)
{
    if (rewards.size() >= static_cast<unsigned long>(threshold)) {
        extendedDialog(text.c_str(), rewards, -1, -1, 0);
        text = DATA_COMPGEN(0x00691210, adventureRolloverEmptyText, "");
        rewards.clear();
    }
}

VA(0x004a0c20, 0x23)  // decorated identity + event-pool index arithmetic
BlackBoxData* advManager::getBlackBox(const ExtraInfoUnion* cell) const
{
    unsigned index = cell->m_value & 0x3ff;
    return &m_fullMap->m_blackBoxes[index];
}

// The BlackBox pointer is HOMED on the frame because the guardians test
// walks a working copy of it forward to the armyGroup at +0x14 and the
// reward call needs the original back.
VA(0x004a0c50, 0x277)  // dc 0x91c18
void advManager::doEventBlackBox(hero* currentHero, NewmapCell* cell,
                                 type_point point, bool humanPlayer)
{
    BlackBoxData* blackBox = cell->getBlackBox();

    if (humanPlayer) {
        if (strlen(blackBox->m_message.c_str()) != 0)
            normalDialog(blackBox->m_message.c_str(), 1, -1, -1, -1, 0,
                         -1, 0, -1, 0, -1, 0);
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_BLACK_BOX_PROMPT],
                     2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
            return;
    } else if (aiValueOfEvent(currentHero, point) <= 0) {
        return;
    }

    if (blackBox->m_hasCustomGuardians && blackBox->m_guardians.getNumArmies()) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_BLACK_BOX_GUARDED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (doCombat(point, currentHero, &currentHero->m_army, -1, 0, 0,
                     &blackBox->m_guardians, -1, 1, 0))
            return;
        currentHero->checkLevel();
    }

    if (!giveBlackBoxReward("", currentHero, cell, point,
                            humanPlayer, blackBox)) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_BLACK_BOX_NOTHING],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }

    eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
}

VA(0x004a0ed0, 0x13D)  // dc 0x91db8
void advManager::handleMapEvent(hero* currentHero, NewmapCell* cell,
                                type_point point, bool humanPlayer)
{
    unsigned index = cell->m_extraInfo & 0x3ff;
    BlackBoxData* blackBox = &m_fullMap->m_blackBoxes[index];
    const char* text = blackBox->m_message.c_str();

    if (blackBox->m_hasCustomGuardians && blackBox->m_guardians.getNumArmies()) {
        if (humanPlayer) {
            if (!*text) {
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_BLACK_BOX_GUARDED],
                             1, -1, -1, -1, 0, -1, 0,
                             -1, 0, -1, 0);
            } else {
                normalDialog(text, 1, -1, -1, -1, 0, -1, 0,
                             -1, 0, -1, 0);
                text = "";
            }
        }
        if (doCombat(point, currentHero, &currentHero->m_army, -1, 0, 0,
                     &blackBox->m_guardians, -1, 1, 0))
            return;
        currentHero->checkLevel();
    }

    giveBlackBoxReward(text, currentHero, cell, point, humanPlayer,
                       blackBox);

    if (cell->m_extraInfo & 0x80000) {
        cell->m_cellFlags &= 0xefff;
        cell->m_extraInfo = 0;
        cell->m_typeValue = NOTHING;
    }
    if (g_game->m_mapHeader.m_victoryCondition.checkForTotalCreatures())
        checkEndGame(0);
    if (g_game->m_mapHeader.m_victoryCondition.checkForTotalResources())
        checkEndGame(0);
}

// The movement rewrite only runs for a hero not already at sea (bit
// 0x1000000), and it PRESERVES THE FRACTION: the new allowance is
// GetMobility(1) and the remainder is scaled by the old ratio with a
// signed `imul`/`cdq`/`idiv`. Without the Admiral's Hat (artifact 0x88)
// the remainder is simply zeroed - boarding costs the rest of the turn.
VA(0x004a1010, 0x10D)  // dc 0x91f44
void advManager::doEventBoat(hero* currentHero, NewmapCell* cell)
{
    boat* heroBoat = &g_game->m_boats[cell->m_extraInfo];

    heroBoat->restoreCell();
    currentHero->m_flags |= 0x40000;
    currentHero->m_flightLevel = -1;
    currentHero->m_waterWalkLevel = -1;
    if (!(currentHero->m_flags & 0x1000000)) {
        if (currentHero->isWieldingArtifact(0x88)) {
            int oldMaxMovePoints = currentHero->m_maxMovePoints;
            int oldMovePoints = currentHero->m_movePoints;
            int newMaxMovePoints = currentHero->getMobility(1);
            currentHero->m_maxMovePoints = newMaxMovePoints;
            currentHero->m_movePoints =
                newMaxMovePoints * oldMovePoints / oldMaxMovePoints;
        } else {
            currentHero->m_movePoints = 0;
        }
        m_advWindow->updateHeroLocator(-1, 1, 1);
    }

    heroBoat->m_occupyingHero = currentHero->m_id;
    heroBoat->m_occupied = 1;
    char owner = currentHero->m_owner;
    heroBoat->m_playerOwner = owner;
    if (g_game->isLocalHuman(owner)) {
        m_cursorType = CURSOR_TYPE_8;
        m_cursorDirection = heroBoat->m_facing;
        m_cursorBaseFrame = 0;
        m_cursorSequence = heroBoat->getStandSequence();
        m_drawCursor = 1;
        completeDraw(0);
        updateScreen(0, 0);
    }
}

// EraseAndFizzle is INLINED here, as in monsters_fight, and FizzleCenter
// inside it in turn; with the sound fixed at PICKUP the fizzle's switch
// folds to the sprintf arm alone.

VA(0x004a1120, 0x1C4)  // dc 0x922e8
void advManager::doEventCampfire(hero* currentHero, NewmapCell* cell,
                                 type_point point, bool humanPlayer)
{
    int qty = cell->getCampfireSize();
    int resource = cell->getCampfireResource();

    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_CAMPFIRE],
                     1, -1, -1, GOLD, qty * 100, resource, qty,
                     -1, 0, -1, 0);
    currentHero->giveResource(GOLD, qty * 100);
    currentHero->giveResource(resource, qty);

    eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
    if (humanPlayer)
        setEnvironmentOrigin(currentHero->getLocation(), 1);
}

VA(0x004a12f0, 0x1B3)  // dc 0x923b0
void advManager::doEventIdol(hero* currentHero, NewmapCell* cell,
                             bool humanPlayer)
{
    if (!(currentHero->m_flags & 0x10) && !(currentHero->m_flags & 0x2000000)) {
        unsigned short day = g_game->m_day;
        if (day == DAY_OF_WEEK_SUNDAY) {
            if (humanPlayer)
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_IDOL],
                             1, -1, -1, 0xb, 0, 0xe, 0, -1, 0, -1, 0);
            currentHero->m_luckBonus++;
            currentHero->m_moraleBonus++;
            currentHero->m_flags |= 0x2000010;
        } else if (day & 1) {
            if (humanPlayer)
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_IDOL],
                             1, -1, -1, 0xb, 0, -1, 0, -1, 0, -1, 0);
            currentHero->m_luckBonus++;
            currentHero->m_flags |= 0x10;
        } else {
            if (humanPlayer)
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_IDOL],
                             1, -1, -1, 0xe, 0, -1, 0, -1, 0, -1, 0);
            currentHero->m_moraleBonus++;
            currentHero->m_flags |= 0x2000000;
        }
        game* g = g_game;
        g->setInfoFlag(IdolOfFortuneInfo, g_netLocalGamePos);
    } else if (humanPlayer) {
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_IDOL_VISITED],
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }
}

VA(0x004a14b0, 0xEC)  // dc 0x92540
void advManager::doEventCoverOfDarkness(NewmapCell* cell, type_point point,
                                        bool humanPlayer)
{
    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_COVER_OF_DARKNESS],
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);

    if (g_remoteOn) {
        CResetVisibilityMsg message(point, g_netLocalGamePos, 20);
        transmitRemoteData(&message, 0x7f, 0, 1);
    }
    g_game->resetVisibility(point.m_x, point.m_y, point.m_z, g_netLocalGamePos, 20);
    g_game->resetAllPlayerVisibility();
    completeDraw(0);
    updateScreen(0, 0);
}

VA(0x004a15a0, 0x301)  // dc 0x925fc
void advManager::doEventCreatureBank(hero* currentHero, NewmapCell* cell,
                                     type_point point, bool humanPlayer)
{
    std::string name = g_constCreatureBankTraits[cell->m_objectIndex].m_name;
    std::string dialogText;

    cell->setCellVisited(currentHero->m_owner);
    if (cell->m_extraInfo & 0x2000000) {
        if (humanPlayer) {
            dialogText = formatString((*g_adventureEventText)[ADV_EVENT_TEXT_CREATURE_BANK_EMPTY_FORMAT],
                                        name.c_str());
            normalDialog(dialogText.c_str(),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
        return;
    }

    if (humanPlayer) {
        dialogText = formatString((*g_adventureEventText)[ADV_EVENT_TEXT_CREATURE_BANK_PROMPT_FORMAT],
                                    name.c_str());
        overrideBottomView(BOTTOM_VIEW_DEFAULT, -1);
        updBottomView(0, 1, 1);
        normalDialog(dialogText.c_str(),
                     2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
            return;
    } else if (aiValueOfEvent(currentHero, point) <= 0) {
        return;
    }

    creatureBankEvent(currentHero, cell, "", point,
                      humanPlayer);
}

// E:\gamedcs\events.cpp:1505.  The creature dwelling (jump-table arms
// 0x11 and 0x14): fight the guards if an enemy owns it, claim it, then
// either sweep the free growth (GeneratorEvent's own loop, with a line
// per outcome) or fall through to the recruit window when a leveled
// dwelling is present. The Dreamcast publishes the locals - the two
// rollover-name tables pick generator_name, the guarded prompt is a
// block-scoped string destroyed before the combat, and the AI tail hands
// an owned-or-unowned dwelling to AI_PurchaseCreatures.
// [2026-08-27] Residual (99.0902%): our CL keeps `&guards` (lea ebx+0x1c)
// live in EDI across the HasCreatures call and reuses it for the scan
// loop; retail recomputes the lea at both sites and stores the prompt's
// EH state as an immediate. why-reg: bindings agree at every first def,
// divergence past them (allocator CSE state, 11 slots). The
// generatorType/index locals, the inverted CREATURE_GENERATOR_1 arm and
// the shared GeneratorEvent tail above are measured (74.81 -> 99.09).
// Fresh 2026-09-01 source/structure comparison localizes the first mismatch
// to that same pair: retail passes `&guards` in ECX then recomputes its LEA
// after prompt construction, while this compile keeps it in EDI. The other
// 114/115 blocks are an alignment cascade from this one allocator choice,
// not evidence for a source control-flow rewrite.
VA(0x004a18b0, 0x79A)  // dc-bracket forced, ret 0x10=p5, dc 0x92814
void advManager::doEventCreatureGenerator(hero* currentHero, NewmapCell* cell,
                                          type_point point, bool humanPlayer)
{
    TAdventureObjectType generatorCount = cell->m_type;
    int index = cell->m_objectIndex;
    const char* generatorName;
    if (generatorCount == CREATURE_GENERATOR_1)
        generatorName = g_creatureGenerator1RolloverNames[index];
    else
        generatorName = g_creatureGenerator4RolloverNames[index];

    int id = g_game->getGeneratorId(point.m_x, point.m_y, point.m_z);
    generator& currentGenerator = g_game->m_generators[id];

    if (!g_game->onSameTeam(currentGenerator.getOwner(), g_netLocalGamePos)) {
        if (currentGenerator.m_guards.hasCreatures()) {
            if (humanPlayer) {
                std::string prompt;
                int i;
                for (i = 0; i < 7; i++) {
                    if (currentGenerator.m_guards.m_armies[i] != CREATURE_NONE)
                        break;
                }
                TCreatureType guardType =
                    TCreatureType(currentGenerator.m_guards.m_armies[i]);
                long guardQty = currentGenerator.m_guards.getCreatureTotal();
                overrideBottomView(BOTTOM_VIEW_DEFAULT, -1);
                updBottomView(0, 1, 1);
                prompt = formatString(
                    (*g_generalText)[GENERAL_TEXT_GUARDED_OBJECT_PROMPT_FORMAT], generatorName,
                    armyGroup::getArmySizeName(guardQty, 2),
                    getArmyName(guardType, 2));
                normalDialog(prompt.c_str(),
                             2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                if (g_windowManager->m_dialogReturn == DIALOG_RETURN_DECLINE)
                    return;
            } else if (aiValueOfEvent(currentHero, point) <= 0) {
                return;
            }
            if (doCombat(point, currentHero, &currentHero->m_army, -1, 0, 0,
                         &currentGenerator.m_guards, -1, 1, 0))
                return;
        }
        if (!g_game->onSameTeam(currentGenerator.getOwner(),
                                g_netLocalGamePos))
            g_game->claimGenerator(id, g_netLocalGamePos);
    }

    if (humanPlayer) {
        if (generatorCount != CREATURE_GENERATOR_1) {
            if (generatorCount == CREATURE_GENERATOR_4) {
                sprintf(g_text, (*g_adventureEventText)[ADV_EVENT_TEXT_DWELLING_RECRUIT_MULTIPLE_FORMAT],
                        generatorName,
                        getArmyName(currentGenerator.m_type[0], 2),
                        getArmyName(currentGenerator.m_type[1], 2),
                        getArmyName(currentGenerator.m_type[2], 2),
                        getArmyName(currentGenerator.m_type[3], 2));
            }
        } else {
            sprintf(g_text, (*g_adventureEventText)[ADV_EVENT_TEXT_DWELLING_RECRUIT_ONE_FORMAT],
                    generatorName,
                    getArmyName(currentGenerator.m_type[0], 2));
        }
        normalDialog(g_text, 2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn == DIALOG_RETURN_DECLINE)
            return;

        if (currentGenerator.getOwner() == g_netLocalGamePos) {
            bool canRecruit = false;
            std::string result;
            for (int i = 0; i < 4; i++) {
                TCreatureType creature = currentGenerator.m_type[i];
                if (creature == CREATURE_NONE)
                    continue;

                if (g_creatureTypeTraits[creature].m_level != 0) {
                    canRecruit = true;
                    continue;
                }

                if (currentGenerator.m_population[i] == 0) {
                    result += formatString((*g_generalText)[GENERAL_TEXT_NO_CREATURES_TO_RECRUIT_FORMAT],
                        g_creatureTypeTraits[creature].m_pluralName);
                } else if (!currentHero->m_army.add(
                               creature, currentGenerator.m_population[i],
                               -1)) {
                    result += formatString((*g_generalText)[GENERAL_TEXT_RECRUIT_INSUFFICIENT_PROVISIONS_FORMAT],
                        getArmyName(creature,
                                    currentGenerator.m_population[i]));
                } else {
                    result += formatString((*g_generalText)[GENERAL_TEXT_CREATURES_JOIN_FORMAT],
                        currentGenerator.m_population[i],
                        getArmyName(creature,
                                    currentGenerator.m_population[i]));
                    currentGenerator.m_population[i] = 0;
                }
            }

            if (result.length() > 0)
                normalDialog(result.c_str(),
                             1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            if (!canRecruit)
                return;
        }
        generatorEvent(currentHero, cell, point);
    } else {
        if (currentGenerator.getOwner() == g_netLocalGamePos
            || currentGenerator.getOwner() < 0)
            aiPurchaseCreatures(currentHero, &currentGenerator);
    }
}

// DC1653/1661 call TTextResource::operator[], not GetText directly. The same
// wrapper is recorded for Garden (1858/1866), MercenaryCamp (2303/2311), and
// PowerSchool (2492/2500). Restoring all eight calls preserves their exact
// retail bodies; it does not by itself recover dispatchEvent's retained calls.
// Keep humanPlayer as bool: the DC publics for all four handlers at
// 0x92d40/0x93368/0x941c8/0x946b4 end in PAVNewmapCell@@_N@Z. Their
// unsigned-char CodeView formal records are lowered bool representations,
// not evidence for changing the source interface or steering the inliner.
VA(0x004a2050, 0xE4)  // dc 0x92d40
void advManager::doEventDefenseTower(hero* currentHero, NewmapCell* cell,
                                     bool humanPlayer)
{
    if (currentHero->m_defenseTowerFlags & (1 << cell->m_extraInfo)) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_DEFENSE_TOWER_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }
    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_DEFENSE_TOWER],
                     1, -1, -1, 0x20, 1, -1, 0, -1, 0, -1, 0);
    currentHero->adjustPrimarySkill(1, 1);
    g_game->setInfoFlag(DefenseTowerInfo, g_netLocalGamePos);
    currentHero->m_defenseTowerFlags |= 1 << cell->m_extraInfo;
}

VA(0x004a2140, 0xE8)  // dc 0x92dec
void advManager::doEventDragonCity(hero* currentHero, NewmapCell* cell,
                                      type_point point, bool humanPlayer)
{
    cell->setCellVisited(currentHero->m_owner);
    if (cell->m_extraInfo & 0x2000000) {
        if (humanPlayer) {
            normalDialog((*g_generalText)[GENERAL_TEXT_DRAGON_CITY_EMPTIED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            return;
        }
    } else if (humanPlayer) {
        overrideBottomView(BOTTOM_VIEW_DEFAULT, -1);
        updBottomView(0, 1, 1);
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_DRAGON_CITY_PROMPT],
                     2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn == DIALOG_RETURN_DECLINE)
            return;
    }

    if (!humanPlayer && aiValueOfEvent(currentHero, point) <= 0)
        return;
    creatureBankEvent(currentHero, cell, "", point,
                      humanPlayer);
}

VA(0x004a2230, 0x250)  // dc 0x92fa8
void advManager::doEventFlotsam(hero* currentHero, NewmapCell* cell,
                                type_point point, bool humanPlayer)
{
    switch (cell->m_extraInfo) {
    case FLOTSAM_NOTHING:
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_FLOTSAM_NOTHING],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        break;
    case FLOTSAM_WOOD:
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_FLOTSAM_WOOD],
                         1, -1, -1, WOOD, 5, -1, 0, -1, 0, -1, 0);
        currentHero->giveResource(WOOD, 5);
        break;
    case FLOTSAM_WOOD_AND_GOLD:
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_FLOTSAM_WOOD_AND_GOLD],
                         1, -1, -1, WOOD, 5, GOLD, 200, -1, 0, -1, 0);
        currentHero->giveResource(WOOD, 5);
        currentHero->giveResource(GOLD, 200);
        break;
    case FLOTSAM_LARGE:
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_FLOTSAM_LARGE],
                         1, -1, -1, WOOD, 10, GOLD, 500, -1, 0, -1, 0);
        currentHero->giveResource(WOOD, 10);
        currentHero->giveResource(GOLD, 500);
        break;
    }

    eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
}

// This is the dragon city's finding read from the other side, and the two
// together give the discriminator: a merged tail entered by an
// UNCONDITIONAL jmp that skips a test is a source `goto` (one call site);
// a TWO-WAY branch inside one argument list with everything else shared
// is two call sites.
VA(0x004a2480, 0x16C)  // dc 0x9312c
void advManager::doEventFountain(hero* currentHero, ExtraInfoUnion* cell,
                                 bool humanPlayer)
{
    cell->setCellVisited(currentHero->m_owner);
    if ((currentHero->m_flags & 0x20) || (currentHero->m_flags & 0x38000000)) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_FOUNTAIN_OF_FORTUNE_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    if (humanPlayer) {
        if (cell->m_fountainInfo.m_luck > 0)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_FOUNTAIN_OF_FORTUNE],
                         1, -1, -1, 11, 0, -1, 0, -1, 0, -1, 0);
        else
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_FOUNTAIN_OF_FORTUNE],
                         1, -1, -1, 13, 0, -1, 0, -1, 0, -1, 0);
    }

    switch (cell->m_fountainInfo.m_luck) {
    case FOUNTAIN_LUCK_CURSED:
        currentHero->m_flags |= 0x20;
        break;
    case FOUNTAIN_LUCK_PLUS_1:
        currentHero->m_flags |= 0x8000000;
        break;
    case FOUNTAIN_LUCK_PLUS_2:
        currentHero->m_flags |= 0x10000000;
        break;
    case FOUNTAIN_LUCK_PLUS_3:
        currentHero->m_flags |= 0x20000000;
        break;
    }

    game* g = g_game;
    g->setInfoFlag(FountainOfFortuneInfo, g_netLocalGamePos);
    currentHero->m_luckBonus += cell->m_fountainInfo.m_luck;
}

VA(0x004a25f0, 0x113)  // dc 0x93298
void advManager::doEventFountainOfYouth(hero* currentHero, NewmapCell* cell,
                                        bool humanPlayer)
{
    if (currentHero->m_flags & 0x4000) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_FOUNTAIN_OF_YOUTH_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }
    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_FOUNTAIN_OF_YOUTH],
                     1, -1, -1, 14, 0, -1, 0, -1, 0, -1, 0);
    game* g = g_game;
    g->setInfoFlag(FountainOfYouthInfo, g_netLocalGamePos);
    currentHero->m_flags |= 0x4000;
    currentHero->m_moraleBonus++;
    currentHero->m_maxMovePoints += 400;
    currentHero->m_movePoints += 400;
    if (humanPlayer)
        m_advWindow->updateHeroLocators(-1, 1, 1);
}

VA(0x004a2710, 0xE4)  // dc 0x93368
void advManager::doEventGarden(hero* currentHero, NewmapCell* cell,
                               bool humanPlayer)
{
    if (currentHero->m_gardenOfRevelationFlags & (1 << cell->m_extraInfo)) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_GARDEN_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }
    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_GARDEN],
                     1, -1, -1, 0x22, 1, -1, 0, -1, 0, -1, 0);
    currentHero->adjustPrimarySkill(3, 1);
    g_game->setInfoFlag(GardenOfRevelationInfo, g_netLocalGamePos);
    currentHero->m_gardenOfRevelationFlags |= 1 << cell->m_extraInfo;
}

// Dreamcast keeps these object visitors as named source boundaries. Mac also
// retains their bodies as direct calls; retail VC6 expands them in dispatchEvent.
// The Mac calls do not settle their inline qualifiers, so the existing VC6
// source declarations stay in place while the newly found gate is separate.
inline void advManager::doEventBorderGuard(type_point point, NewmapCell* cell,
                                           unsigned char humanPlayer)
{
    unsigned char visitedFlags =
        g_game->m_borderTentVisitFlags[cell->m_objectIndex];
    if (visitedFlags & g_curPlayerBit) {
        if (humanPlayer) {
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_BORDER_GUARD_PROMPT),
                         2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
                return;
        }
        eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
    } else if (humanPlayer) {
        normalDialog(g_adventureEventText->getText(
                         ADV_EVENT_TEXT_BORDER_GUARD_DENIED),
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }
}

// Mac 0:0xabe24..0xabec0 is the separate border-gate visitor called by
// dispatchEvent at 0:0xb61d0. The earlier Dreamcast build has no named body.
void advManager::doEventBorderGate(NewmapCell* cell,
                                   unsigned char humanPlayer)
{
    if (!(g_game->m_borderTentVisitFlags[cell->m_objectIndex]
          & g_curPlayerBit)) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[
                             ADV_EVENT_TEXT_BORDER_GUARD_DENIED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }
}

inline void advManager::doEventBorderTent(NewmapCell* cell,
                                          unsigned char humanPlayer)
{
    if (g_game->m_borderTentVisitFlags[cell->m_objectIndex]
        & g_curPlayerBit) {
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_BORDER_TENT_VISITED),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    } else {
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_BORDER_TENT),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        g_game->m_borderTentVisitFlags[cell->m_objectIndex] |= g_curPlayerBit;
    }
}

inline void advManager::doEventBouy(hero* currentHero, NewmapCell* cell,
                                    unsigned char humanPlayer)
{
    if (currentHero->m_flags & 4) {
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_BUOY_VISITED),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    } else {
        currentHero->m_flags |= 4;
        currentHero->m_moraleBonus += 1;
        g_game->setInfoFlag(BuoyInfo, g_netLocalGamePos);
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_BUOY),
                         1, -1, -1, 14, 0, -1, 0, -1, 0, -1, 0);
    }
}

inline void advManager::doEventCloverField(hero* currentHero,
                                           NewmapCell* cell,
                                           unsigned char humanPlayer)
{
    if (currentHero->m_flags & 8) {
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_CLOVER_FIELD_VISITED),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    } else {
        currentHero->m_flags |= 8;
        g_game->setInfoFlag(CloverFieldInfo, g_netLocalGamePos);
        currentHero->m_luckBonus += 2;
        currentHero->m_movePoints = 0;
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_CLOVER_FIELD),
                         1, -1, -1, 11, 0, -1, 0, -1, 0, -1, 0);
    }
}

inline void advManager::doEventFaerieRing(hero* currentHero,
                                          NewmapCell* cell,
                                          unsigned char humanPlayer)
{
    if (currentHero->m_flags & 0x2000) {
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_FAERIE_RING_VISITED),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    } else {
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_FAERIE_RING),
                         1, -1, -1, 11, 0, -1, 0, -1, 0, -1, 0);
        currentHero->m_flags |= 0x2000;
        g_game->setInfoFlag(FaerieRingInfo, g_netLocalGamePos);
        currentHero->m_luckBonus += 1;
    }
}

VA(0x004b0000, 0x8A)  // retained comparator; dc 0x9cdc0 proves const.
inline unsigned char spell_level_order::operator()(SpellID first,
                                                    SpellID second) const
{
    if (g_spellTraits[first].m_level == g_spellTraits[second].m_level)
        return strcmp(g_spellTraits[first].m_name,
                      g_spellTraits[second].m_name) < 0;
    return g_spellTraits[first].m_level > g_spellTraits[second].m_level;
}

// The two events.obj/philai.obj helpers this handler needs. 0x4a2940 is
// exchange_spells (dc 0x93464), the ONE events.obj row the Dreamcast
// xref graph gives do_event_hero as a bsr call, and retail reaches it as
// a /Gr fastcall with the two heroes in ecx and edx. 0x525dc0 is
// AI_friendly_hero_meeting (philai.obj, dc 0x10e678) - do_event_hero's
// only philai callee, taking the same pair in the same registers, and it
// is the AI's alternative to the human's HeroSwap on the very same
// branch. Declared file locally, the AI_approximate_strength precedent.
static void exchangeSpells(hero* left, hero* right);
void aiFriendlyHeroMeeting(hero* currentHero, hero* otherHero);

// The whole of game::GetHero is expanded at the top - its `-1` arm gives
// the null hero the cell can carry - and so is game::OnSameTeam, both
// of its `< 0` guards included, which is what puts the two signed tests
// ahead of the pair of teamInfo byte loads.

// The town test spends the point three times because GetTownId takes
// the three coordinates separately; the packed `y` and `z` share one
// 16-bit unit and both are extracted out of the SAME dword load, which
// is why the second half of the point is read at +0x12 rather than the
// field being addressed on its own.
VA(0x004a2800, 0x13F)  // dc 0x939bc
void advManager::doEventHero(hero* currentHero, NewmapCell* cell,
                               type_point point, bool humanPlayer)
{
    demobilizeCurrHero(0, 1);

    hero* otherHero = g_game->getHero(cell->m_extraInfo);
    if (humanPlayer || otherHero->belongsToHuman()) {
        g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
        g_mouseManager->showPointer(true);
    }

    if (g_game->onSameTeam(otherHero->m_owner, g_netLocalGamePos)) {
        exchangeSpells(currentHero, otherHero);
        if (humanPlayer)
            heroSwap(currentHero, otherHero);
        else
            aiFriendlyHeroMeeting(currentHero, otherHero);
        return;
    }

    if (g_game->getTownId(point.m_x, point.m_y, point.m_z) >= 0) {
        townEvent(cell, point, humanPlayer);
        return;
    }

    doCombat(point, currentHero, &currentHero->m_army, otherHero->m_owner, 0,
             otherHero, &otherHero->m_army, -1, 1, 0);
}

// E:\gamedcs\events.cpp:1898. Allied heroes exchange every spell the other
// can learn through Scholar. Dreamcast supplies the original helper, local
// inventory and scope order; retail proves the Complete limits, dialog rows
// and asymmetric seven-icon packing policy. Both builds put the two transfer
// directions under an exclusive spell-ownership if/else and reload the first
// hero's Scholar level for the dialog icon after swapping heroes.
// DC calls min at lines 1913/1915 and appends spells with push_back.
// DC's maximum uses signed-char std::max; retail 0x4a2969..0x4a298b instead
// selects between promoted four-byte copies, consistent with the existing
// by-value int max overload. DC 1979-2019 builds the message with operator+=.
// Residual 91.3003%: the final taught-message += retains string::append
// where retail expands it. Both builds destroy formatString's temporary at
// the end of that expression; extending its lifetime lacks source evidence.
VA(0x004a2940, 0x85C)  // anchor-callee from do_event_hero + full retail semantics, dc 0x93464
static void exchangeSpells(hero* firstHero, hero* secondHero)
{
    const int magicScholarLevel = max(
        firstHero->m_skillLevel[eSecSkillMagicScholar],
        secondHero->m_skillLevel[eSecSkillMagicScholar]);
    std::vector<SpellID> spellsLearned;
    std::vector<SpellID> spellsTaught;

    if (firstHero->m_skillLevel[eSecSkillMagicScholar]
        < secondHero->m_skillLevel[eSecSkillMagicScholar])
        std::swap(firstHero, secondHero);

    if (magicScholarLevel > 0
        && firstHero->isWieldingArtifact(ARTIFACT_SPELLBOOK)
        && secondHero->isWieldingArtifact(ARTIFACT_SPELLBOOK)) {
        const int firstSpellLevel = min(
            magicScholarLevel + 1,
            firstHero->m_skillLevel[eSecSkillWisdom] + 2);
        const int secondSpellLevel = min(
            magicScholarLevel + 1,
            secondHero->m_skillLevel[eSecSkillWisdom] + 2);

        SpellID spell;
        for (spell = 0; spell < hero::NUM_SPELLS; spell++) {
            if (firstHero->isInSpellbook(spell)) {
                if (!secondHero->isInSpellbook(spell)
                    && g_spellTraits[spell].m_level <= secondSpellLevel) {
                    secondHero->addSpell(spell);
                    if (g_currentPlayer->isLocalHuman())
                        spellsTaught.push_back(spell);
                }
            } else if (secondHero->isInSpellbook(spell)) {
                if (!firstHero->isInSpellbook(spell)
                    && g_spellTraits[spell].m_level <= firstSpellLevel) {
                    firstHero->addSpell(spell);
                    if (g_currentPlayer->isLocalHuman())
                        spellsLearned.push_back(spell);
                }
            }
        }
    }

    if (spellsLearned.size() + spellsTaught.size() == 0)
        return;

    std::string msg;
    std::vector<type_dialog_resource> spellsExchanged;
    type_dialog_resource spellInfo;

    msg = formatString((*g_generalText)[GENERAL_TEXT_SCHOLAR_MAGIC_INTRO_FORMAT], firstHero->m_name);
    spellInfo.m_resource = RES_SECONDARY_SKILL;
    spellInfo.m_qualifier = eSecSkillMagicScholar * 3
                           + firstHero->m_skillLevel[eSecSkillMagicScholar] + 2;
    spellsExchanged.push_back(spellInfo);

    std::sort(spellsLearned.begin(), spellsLearned.end(),
              spell_level_order());
    std::sort(spellsTaught.begin(), spellsTaught.end(),
              spell_level_order());

    const int learnedIconCount = max(
        4, 7 - static_cast<int>(spellsTaught.size()));
    const int taughtIconCount = max(
        3, 7 - static_cast<int>(spellsLearned.size()));

    if (spellsLearned.size()) {
        msg += (*g_generalText)[GENERAL_TEXT_LEARNS_FRAGMENT];
        for (int i = 0; i < spellsLearned.size(); i++) {
            if (i < learnedIconCount) {
                spellInfo.m_resource = RES_SPELL;
                spellInfo.m_qualifier = spellsLearned[i];
                spellsExchanged.push_back(spellInfo);
            }
            if (i > 0) {
                if (i == spellsLearned.size() - 1)
                    msg += (*g_generalText)[GENERAL_TEXT_LIST_AND];
                else
                    msg += DATA_COMPGEN(0x0066032c, listSeparator, ", ");
            }
            msg += g_spellTraits[spellsLearned[i]].m_name;
        }
        msg += formatString((*g_generalText)[GENERAL_TEXT_FROM_HERO_FORMAT], secondHero->m_name);
    }

    if (spellsTaught.size()) {
        if (spellsLearned.size()) {
            msg += DATA_COMPGEN(0x00660db4, commaText, ",");
            msg += (*g_generalText)[GENERAL_TEXT_LIST_AND];
        }
        msg += (*g_generalText)[GENERAL_TEXT_TEACHES_FRAGMENT];
        for (int i = 0; i < spellsTaught.size(); i++) {
            if (i < taughtIconCount) {
                spellInfo.m_resource = RES_SPELL;
                spellInfo.m_qualifier = spellsTaught[i];
                spellsExchanged.push_back(spellInfo);
            }
            if (i > 0) {
                if (i == spellsTaught.size() - 1)
                    msg += (*g_generalText)[GENERAL_TEXT_LIST_AND];
                else
                    msg += DATA_COMPGEN(0x0066032c, listSeparator, ", ");
            }
            msg += g_spellTraits[spellsTaught[i]].m_name;
        }
        msg += formatString((*g_generalText)[GENERAL_TEXT_TO_HERO_FORMAT], secondHero->m_name);
    }

    msg += DATA_COMPGEN(0x006603ec, saveExtensionDot, ".");
    extendedDialog(msg.c_str(), spellsExchanged, -1, -1, 0);
}

// GetLeanToAmount is decorated `short`, and that width is the whole
// reason the emptiness test is a sixteen-bit `test si,si` and the dialog
// argument a `movsx`: an int-wide read would not truncate.

VA(0x004a31a0, 0xDF)  // dc 0x93b18
void advManager::doEventLeanTo(hero* currentHero, ExtraInfoUnion* cell,
                               bool humanPlayer)
{
    short id = cell->getItemId();
    short amount = cell->getLeanToAmount();

    if (amount == 0) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_LEAN_TO_EMPTY],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        g_currentPlayer->m_leanToFlags |= 1 << id;
    } else {
        int resource = cell->getLeanToResource();
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_LEAN_TO],
                         1, -1, -1, resource, amount, -1, 0, -1, 0, -1, 0);
        currentHero->giveResource(resource, amount);
        cell->setLeanTo(id, 0, 0);
        g_currentPlayer->m_leanToFlags |= 1 << id;
    }
}

VA(0x004a3280, 0x15C)  // dc 0x93bf8
void advManager::doEventLibrary(hero* currentHero, NewmapCell* cell,
                                bool humanPlayer)
{
    unsigned long visit = 1 << cell->m_extraInfo;

    if (currentHero->m_libraryFlags & visit) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_LIBRARY_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }
    if (currentHero->m_level + currentHero->m_skillLevel[4] * 2 >= 10) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_LIBRARY],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        currentHero->adjustPrimarySkill(0, 2);
        currentHero->adjustPrimarySkill(1, 2);
        currentHero->adjustPrimarySkill(3, 2);
        currentHero->adjustPrimarySkill(2, 2);
        game* g = g_game;
        g->setInfoFlag(LibraryInfo, g_netLocalGamePos);
        currentHero->m_libraryFlags |= visit;
        return;
    }
    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_LIBRARY_UNWORTHY],
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
}

TPrimarySkill aiChooseMagicSkill(hero* currentHero);

// `game* g = gpGame;` is spelled out for the war school's reason: it is
// what puts the player position first in the SIB of the inlined
// teamInfo[playerNum] load (`[pos + gpGame]`).
VA(0x004a33e0, 0x1AD)  // dc 0x93db0
void advManager::doEventMagicSchool(hero* currentHero, NewmapCell* cell,
                                    type_point point, bool humanPlayer)
{
    if (currentHero->m_magicSchoolFlags & (1 << cell->m_extraInfo)) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MAGIC_SCHOOL_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    game* g = g_game;
    g->setInfoFlag(MagicSchoolInfo, g_netLocalGamePos);
    if (g_currentPlayer->m_resources[GOLD] < 1000) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MAGIC_SCHOOL_NO_GOLD],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    TPrimarySkill skill = ePriSkillPower;
    if (humanPlayer) {
        overrideBottomView(BOTTOM_VIEW_DEFAULT, -1);
        updBottomView(0, 1, 1);
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MAGIC_SCHOOL_CHOOSE],
                     10, -1, -1, 0x21, 1, 0x22, 1, -1, 0, -1, 0);
        switch (g_windowManager->m_dialogReturn) {
        case DIALOG_RETURN_CANCEL:
            return;
        case DIALOG_RETURN_CHOICE_1:
            skill = ePriSkillPower;
            break;
        case DIALOG_RETURN_CHOICE_2:
            skill = ePriSkillKnowledge;
            break;
        }
    } else {
        if (aiValueOfEvent(currentHero, point) <= 0)
            return;
        skill = aiChooseMagicSkill(currentHero);
    }

    currentHero->adjustPrimarySkill(skill, 1);
    currentHero->m_magicSchoolFlags |= 1 << cell->m_extraInfo;
    g_currentPlayer->m_resources[GOLD] -= 1000;
}

VA(0x004a3590, 0x19C)  // dc 0x93f6c
void advManager::doEventMagicSpring(hero* currentHero, ExtraInfoUnion* cell,
                                    bool humanPlayer)
{
    game* g = g_game;
    g->setInfoFlag(MagicSpringInfo, g_netLocalGamePos);
    // Retail records the visit before checking whether the spring is empty;
    // Dreamcast calls getItemId only after that branch and the mana award.
    short id = cell->m_magicSpringInfo.m_id;
    g_currentPlayer->m_magicSpringFlags |= 1 << id;

    if (!cell->magicSpringIsFull()) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MAGIC_SPRING_EMPTY],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    int cap = currentHero->getMaxMana() * 2;
    if (currentHero->m_mana >= cap) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MAGIC_SPRING_NO_ROOM],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    } else {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MAGIC_SPRING],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        currentHero->m_mana = cap;
        cell->fillMagicSpring(0);
    }
    if (humanPlayer)
        m_advWindow->updateHeroLocators(-1, 1, 1);
    updBottomView(1, 1, 1);
}

VA(0x004a3730, 0x17E)  // dc 0x94088
void advManager::doEventMagicWell(hero* currentHero, ExtraInfoUnion* cell,
                                  bool humanPlayer)
{
    if (currentHero->m_flags & 1) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MAGIC_WELL_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    cell->m_value = 0;
    int cap = currentHero->getMaxMana();
    if (currentHero->m_mana >= cap) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MAGIC_WELL_NO_ROOM],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    } else {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MAGIC_WELL],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        currentHero->m_mana = cap;
        currentHero->m_flags |= 1;
    }
    game* g = g_game;
    g->setInfoFlag(MagicWellInfo, g_netLocalGamePos);
    if (humanPlayer)
        m_advWindow->updateHeroLocators(-1, 1, 1);
    updBottomView(1, 1, 1);
}

VA(0x004a38b0, 0xE4)  // dc 0x941c8
void advManager::doEventMercenaryCamp(hero* currentHero, NewmapCell* cell,
                                      bool humanPlayer)
{
    if (currentHero->m_mercCampFlags & (1 << cell->m_extraInfo)) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MERC_CAMP_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }
    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MERC_CAMP],
                     1, -1, -1, 0x1f, 1, -1, 0, -1, 0, -1, 0);
    currentHero->adjustPrimarySkill(0, 1);
    g_game->setInfoFlag(MercCampInfo, g_netLocalGamePos);
    currentHero->m_mercCampFlags |= 1 << cell->m_extraInfo;
}

void doMonsterJoinDialog(hero* inHero, armyGroup* monsters, int flag);

VA(0x004a39a0, 0x21D)  // dc 0x94314
void advManager::doEventMine(NewmapCell* cell, hero* currentHero,
                             type_point point, bool human)
{
    mine* currentMine = &g_game->m_mines[cell->m_extraInfo];
    if (g_game->onSameTeam(currentMine->m_playerOwner, g_netLocalGamePos)) {
        if (currentMine->m_playerOwner == g_netLocalGamePos && human)
            doMonsterJoinDialog(currentHero, &currentMine->m_guards, 1);
        return;
    }

    if (currentMine->m_guards.hasCreatures()) {
        if (human) {
            overrideBottomView(BOTTOM_VIEW_DEFAULT, -1);
            updBottomView(0, 1, 1);
            if (currentMine->m_playerOwner < 0)
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MINE_GUARDED_PROMPT],
                             2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            else
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MINE_DEFENDED_PROMPT],
                             2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
                return;
        } else if (aiValueOfEvent(currentHero, point) <= 0) {
            return;
        }

        if (currentMine->m_playerOwner < 0
            && currentMine->m_guards.getNumArmies() == 1) {
            int guardCount = currentMine->m_guards.m_numTroops[0];
            if (combatMonsterEvent(
                    currentHero, currentMine->m_guards.m_armies[0],
                    &guardCount, cell, point,
                    CREATURE_NONE, 0, 0,
                    CREATURE_NONE, 0, 0)) {
                currentMine->m_guards.m_numTroops[0] = guardCount;
                return;
            }
            currentMine->m_guards.initialize();
            if (human)
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MINE_CLEARED],
                             1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        } else if (doCombat(point, currentHero, &currentHero->m_army,
                            currentMine->m_playerOwner, 0, 0,
                            &currentMine->m_guards, -1, 1, 0)) {
            return;
        }
        currentHero->restoreCell();
    }

    if (human)
        normalDialog(g_mineEventText[currentMine->m_type], 1, -1, -1,
                     currentMine->m_type,
                     -g_mineCharacteristics[currentMine->m_type],
                     -1, 0, -1, 0, -1, 0);
    g_game->claimMine(cell->m_extraInfo, g_netLocalGamePos, const_normal_action);
}

VA(0x004a3bc0, 0xDC)  // dc 0x944d4
void advManager::doEventMysticalGarden(hero* currentHero, ExtraInfoUnion* cell,
                                       bool humanPlayer)
{
    // Retail records the visit before the empty-garden branch; Dreamcast's
    // getItemId call follows GiveResource on the reward path instead.
    short id = cell->m_gardenInfo.m_id;
    EGameResource resource = cell->getGardenResource();
    g_currentPlayer->m_mysticalGardenFlags |= 1 << id;

    if (!cell->gardenIsFull()) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MYSTICAL_GARDEN_EMPTY],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    short amount = resource == GOLD ? 500 : 5;
    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MYSTICAL_GARDEN],
                     1, -1, -1, resource, amount, -1, 0, -1, 0, -1, 0);
    currentHero->giveResource(resource, amount);
    cell->setGardenEmpty();
}

VA(0x004a3ca0, 0x11C)  // dc 0x9459c
void advManager::doEventOasis(hero* currentHero, NewmapCell* cell,
                              bool humanPlayer)
{
    if (currentHero->m_flags & 0x80) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_OASIS_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }
    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_OASIS],
                     1, -1, -1, 14, 0, -1, 0, -1, 0, -1, 0);
    currentHero->m_flags |= 0x80;
    game* g = g_game;
    g->setInfoFlag(OasisInfo, g_netLocalGamePos);
    currentHero->m_moraleBonus++;
    currentHero->m_maxMovePoints += 800;
    currentHero->m_movePoints += 800;
    if (humanPlayer)
        m_advWindow->updateHeroLocators(-1, 1, 1);
}

VA(0x004a3dc0, 0xE4)  // dc 0x946b4
void advManager::doEventPowerSchool(hero* currentHero, NewmapCell* cell,
                                    bool humanPlayer)
{
    if (currentHero->m_powerSchoolFlags & (1 << cell->m_extraInfo)) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_POWER_SCHOOL_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }
    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_POWER_SCHOOL],
                     1, -1, -1, 0x21, 1, -1, 0, -1, 0, -1, 0);
    currentHero->adjustPrimarySkill(2, 1);
    g_game->setInfoFlag(PowerSchoolInfo, g_netLocalGamePos);
    currentHero->m_powerSchoolFlags |= 1 << cell->m_extraInfo;
}

VA(0x004a3eb0, 0x376)  // dc 0x94760
void advManager::doEventPrison(hero* currentHero, NewmapCell* cell,
                               type_point point, bool humanPlayer)
{
    const int prisonRescueText = 102;
    const int prisonHeroLimitText = 103;
    const int prisonEmptyText = 104;
    int heroID = cell->m_extraInfo;
    if (g_game->m_heroAvailability[heroID]
            != hero::HERO_AVAILABILITY_PRISON) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[prisonEmptyText],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
        return;
    }

    if (g_currentPlayer->m_numHeroes >= playerData::HERO_SLOT_COUNT) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[prisonHeroLimitText],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    if (humanPlayer)
        normalDialog((*g_adventureEventText)[prisonRescueText],
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);

    unsigned char oldColorCycling = g_colorCyclingEnabled;
    g_colorCyclingEnabled = 0;
    unsigned char oldAnimCtrPaused = m_animCtrPaused;
    m_animCtrPaused = 1;
    completeDraw(0);
    updateScreen(0, 0);
    eraseObj(cell, point, 1);

    hero* prisoner = &g_game->m_heroes[heroID];
    g_game->recordShowHero(prisoner, currentHero->m_owner, point, 0);
    prisoner->m_owner = currentHero->m_owner;
    g_game->m_heroAvailability[heroID] = currentHero->m_owner;
    g_game->m_heroPoolMap[heroID][currentHero->m_owner] = 1;
    g_currentPlayer->m_heroes[g_currentPlayer->m_numHeroes] = heroID;
    ++g_currentPlayer->m_numHeroes;
    prisoner->m_x = point.m_x;
    prisoner->m_y = point.m_y;
    prisoner->m_z = point.m_z;
    prisoner->m_flags = 0;
    prisoner->m_facing = hero::kFacingE;
    prisoner->m_movePoints = prisoner->getMobility();
    prisoner->m_maxMovePoints = prisoner->m_movePoints;
    cell->m_isTrigger = 0;
    cell->m_typeValue = 0;
    prisoner->obscureCell();

    fizzleCenter(FIZZLE_SOUND_PICKUP);
    m_animCtrPaused = oldAnimCtrPaused;
    g_colorCyclingEnabled = oldColorCycling;

    CMCRecruitHero change(heroID, point, g_netLocalGamePos);
    sendMapChange(&change);
    m_advWindow->updateHeroLocators(-1, 1, 1);
}

// The spell is written back BEFORE the hero is asked whether he can carry
// it: set_pyramid clears the guarded bit and re-stamps the same spell in
// one masked read-modify-write, and every later arm reads the local copy.
VA(0x004a4230, 0x28A)  // dc 0x949e0
void advManager::doEventPyramid(hero* currentHero, NewmapCell* cell,
                                  type_point point, bool humanPlayer)
{
    if (humanPlayer) {
        overrideBottomView(BOTTOM_VIEW_DEFAULT, -1);
        updBottomView(0, 1, 1);
        normalDialog(g_adventureEventText->getText(
                         ADV_EVENT_TEXT_PYRAMID_PROMPT),
                     2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn == DIALOG_RETURN_DECLINE)
            return;
    } else if (aiValueOfEvent(currentHero, point) <= 0) {
        return;
    }

    cell->setCellVisited(currentHero->m_owner);
    if (!cell->pyramidIsGuarded()) {
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_PYRAMID_ROBBED),
                         1, -1, -1, 0xd, 0, 0xd, 0, -1, 0, -1, 0);
        if (!(currentHero->m_flags & 0x1000)) {
            currentHero->m_flags |= 0x1000;
            currentHero->m_luckBonus -= 2;
        }
        return;
    }

    int goldGolems = 40;
    if (combatMonsterEvent(currentHero, 116, &goldGolems, cell, point,
                           CREATURE_DIAMOND_GOLEM, 20, 2,
                           CREATURE_NONE, 0, 0))
        return;
    currentHero->checkLevel();

    int spell = cell->getPyramidSpell();
    char text[500];
    sprintf(text, DATA_COMPGEN(0x00677750, quotedNameFormat, "%s'%s'."),
            g_adventureEventText->getText(ADV_EVENT_TEXT_PYRAMID_SPELL_PREFIX),
            g_spellTraits[spell].m_name);
    cell->setPyramid(0, spell);

    if (!currentHero->isWieldingArtifact(ARTIFACT_SPELLBOOK)) {
        if (humanPlayer) {
            strcat(text, g_adventureEventText->getText(
                              ADV_EVENT_TEXT_PYRAMID_NO_SPELLBOOK_SUFFIX));
            normalDialog(text, 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
    } else if (g_spellTraits[spell].m_level > currentHero->m_skillLevel[eSecSkillWisdom] + 2) {
        if (humanPlayer) {
            strcat(text, g_adventureEventText->getText(
                              ADV_EVENT_TEXT_PYRAMID_NO_WISDOM_SUFFIX));
            normalDialog(text, 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
    } else {
        currentHero->addSpell(spell);
        if (humanPlayer)
            normalDialog(text, 1, -1, -1, 9, spell, -1, 0, -1, 0, -1, 0);
    }
}

// The visit bit is materialised into a register at the top and reused for
// the write-back, which is what one named mask expression produces.
VA(0x004a44c0, 0x136)  // dc 0x94c8c
void advManager::doEventRallyFlag(hero* currentHero, NewmapCell* cell,
                                  bool humanPlayer)
{
    if (currentHero->m_flags & 0x10000) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_RALLY_FLAG_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }
    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_RALLY_FLAG],
                     1, -1, -1, 0xe, 0, 0xb, 0, -1, 0, -1, 0);
    currentHero->m_flags |= 0x10000;
    game* g = g_game;
    g->setInfoFlag(RallyFlagInfo, g_netLocalGamePos);
    currentHero->m_moraleBonus++;
    currentHero->m_luckBonus++;
    currentHero->m_maxMovePoints += 400;
    currentHero->m_movePoints += 400;
    m_advWindow->updateHeroLocators(-1, 0, 0);
    if (humanPlayer)
        m_advWindow->updateHeroLocators(-1, 1, 1);
    updBottomView(1, 1, 1);
}

void aiRecruitRefugees(hero* currentHero, TCreatureType type, short* number);

VA(0x004a4600, 0x17C)  // dc 0x94d84
void advManager::doEventRefugeeCamp(hero* currentHero, NewmapCell* cell,
                                    bool humanPlayer)
{
    if (!humanPlayer) {
        if (cell->m_extraInfo == 0)
            return;
    } else {
        if (cell->m_extraInfo == 0) {
            sprintf(g_text,
                    (*g_adventureEventText)[ADV_EVENT_TEXT_REFUGEE_CAMP_EMPTY_FORMAT],
                    g_quickViewText[cell->m_type]);
            normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            return;
        }
        sprintf(g_text,
                (*g_adventureEventText)[ADV_EVENT_TEXT_REFUGEE_CAMP_RECRUIT_FORMAT],
                g_quickViewText[cell->m_type],
                getArmyName(cell->m_objectIndex, 2));
        normalDialog(g_text, 2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
            return;
    }

    cell->m_extraInfo = recruitEvent(currentHero,
        TCreatureType(cell->m_objectIndex), cell->m_extraInfo);
}

VA(0x004a4780, 0x45D)  // dc 0x94ea4
void advManager::doCustomResource(NewmapCell* cell, hero* currentHero,
                                  type_point point, bool humanPlayer)
{
    TreasureData* treasure = getTreasureData(cell);
    int type = cell->m_objectIndex;
    int amount = cell->m_extraInfo & 0x7ffff;
    if (type == GOLD)
        amount = amount * 100;

    if (treasure->m_hasCustomGuardians && treasure->m_guardians.getNumArmies()) {
        if (humanPlayer) {
            if (treasure->m_message.length() > 0) {
                overrideBottomView(BOTTOM_VIEW_DEFAULT, -1);
                updBottomView(0, 1, 1);
                normalDialog(treasure->m_message.c_str(),
                             2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
                    return;
            }
        } else if (aiValueOfEvent(currentHero, point) <= 0) {
            return;
        }

        if (doCombat(point, currentHero, &currentHero->m_army, -1, 0, 0,
                     &treasure->m_guardians, -1, 1, 0))
            return;
        currentHero->checkLevel();
        currentHero->giveResource(type, amount);
        if (humanPlayer) {
            char prompt[200];
            strcpy(prompt, g_resourceNames[type]);
            prompt[0] = tolower(prompt[0]);
            sprintf(g_text,
                    (*g_adventureEventText)[ADV_EVENT_TEXT_RESOURCE_PICKUP_FORMAT],
                    prompt);
            bvResMsg(g_text, type, amount);
        }
        eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
        return;
    }

    if (humanPlayer) {
        if (treasure->m_message.length() > 0)
            normalDialog(treasure->m_message.c_str(),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        char prompt[52];
        strcpy(prompt, g_resourceNames[type]);
        prompt[0] = tolower(prompt[0]);
        sprintf(g_text,
                (*g_adventureEventText)[ADV_EVENT_TEXT_RESOURCE_PICKUP_FORMAT],
                prompt);
        bvResMsg(g_text, type, amount);
    }
    currentHero->giveResource(type, amount);
    eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
}

// EraseAndFizzle is inlined here exactly as in the campfire, and runs
// whether or not anyone was watching.
VA(0x004a4be0, 0x1D9)  // dc 0x9512c
void advManager::doEventResource(NewmapCell* cell, hero* currentHero,
                                 type_point point, bool humanPlayer)
{
    if (cell->isCustomized()) {
        doCustomResource(cell, currentHero, point, humanPlayer);
        return;
    }

    int type = cell->m_objectIndex;
    int amount;
    if (type == GOLD)
        amount = cell->m_extraInfo * 100;
    else
        amount = cell->m_extraInfo;
    currentHero->giveResource(type, amount);

    if (humanPlayer) {
        char prompt[200];
        strcpy(prompt, g_resourceNames[type]);
        prompt[0] = tolower(prompt[0]);
        sprintf(g_text,
                (*g_adventureEventText)[ADV_EVENT_TEXT_RESOURCE_PICKUP_FORMAT],
                prompt);
        bvResMsg(g_text, type, amount);
    }

    eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
}

// ONE text row serves all three arms; only the picture pair differs. The
// secondary-skill arm's picture EXTRA is an icon index computed from the
// skill and the level it just reached, which is why the mastery byte is
// read back after GiveSS rather than before it.

VA(0x004a4dc0, 0x263)  // dc 0x951f4
void advManager::doEventScholar(hero* currentHero, NewmapCell* cell,
                                type_point point, bool humanPlayer)
{
    ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
        static_cast<void*>(&cell->m_extraInfo));
    ScholarAwards award = info->getScholarAward();
    if (award == const_scholar_spell) {
        SpellID spell = info->getScholarSpell();
        if (currentHero->isInSpellbook(spell)
            || g_spellTraits[spell].m_level > currentHero->m_skillLevel[eSecSkillWisdom] + 2
            || !currentHero->isWieldingArtifact(ARTIFACT_SPELLBOOK)) {
            award = const_scholar_primary_skill;
        } else {
            if (humanPlayer)
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_SCHOLAR],
                             1, -1, -1, 9, spell, -1, 0, -1, 0, -1, 0);
            currentHero->addSpell(spell);
        }
    }
    if (award == const_scholar_secondary_skill) {
        int skill = info->getScholarSecondarySkill();
        if (!currentHero->giveSS(skill, 1)) {
            award = const_scholar_primary_skill;
        } else {
            if (humanPlayer)
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_SCHOLAR],
                             1, -1, -1, 0x14,
                             currentHero->m_skillLevel[skill] + skill * 3 + 2,
                             -1, 0, -1, 0, -1, 0);
        }
    }

    if (award == const_scholar_primary_skill) {
        int primary = info->getScholarPrimarySkill();
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_SCHOLAR],
                         1, -1, -1, primary + 0x1f, 1, -1, 0, -1, 0, -1, 0);
        currentHero->adjustPrimarySkill(primary, 1);
    }

    eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
}

VA(0x004a5030, 0x26E)  // dc 0x953cc
void advManager::doEventSeaChest(hero* currentHero, NewmapCell* cell,
                                 type_point point, bool humanPlayer)
{
    type_artifact artifact;
    artifact.m_artifactId = ARTIFACT_NONE;
    artifact.m_extra = -1;

    int reward = cell->getSeaChestReward();
    if (reward == const_sea_chest_artifact
        && currentHero->getNumberInBackpack(1) >= 64)
        reward = const_sea_chest_gold;

    switch (reward) {
    case const_sea_chest_nothing:
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_SEA_CHEST_EMPTY],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        break;
    case const_sea_chest_gold:
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_SEA_CHEST_GOLD],
                         1, -1, -1, GOLD, 1500, -1, 0, -1, 0, -1, 0);
        currentHero->giveResource(GOLD, 1500);
        break;
    case const_sea_chest_artifact:
        int artifactId = cell->getSeaChestArtifact();
        memcpy(&artifact.m_artifactId, &artifactId,
               sizeof artifact.m_artifactId);
        if (humanPlayer) {
            sprintf(g_text,
                    (*g_adventureEventText)[ADV_EVENT_TEXT_SEA_CHEST_ARTIFACT_FORMAT],
                    g_artifactTraits[artifact.m_artifactId].m_name);
            normalDialog(g_text, 1, -1, -1, 8, artifact.m_artifactId,
                         0x24, 1000, -1, 0, -1, 0);
        }
        currentHero->giveArtifact(&artifact, 1, 1);
        currentHero->giveResource(GOLD, 1000);
        if (!humanPlayer)
            aiEquipArtifacts(currentHero);
        break;
    }

    eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
}

VA(0x004a52a0, 0x1DE)  // dc 0x95564
void advManager::doEventSurvivor(hero* currentHero, NewmapCell* cell,
                                 type_point point, bool humanPlayer)
{
    if (currentHero->getNumberInBackpack(1) < 64) {
        if (humanPlayer) {
            sprintf(g_text,
                    (*g_adventureEventText)[ADV_EVENT_TEXT_SHIPWRECK_SURVIVOR_ARTIFACT_FORMAT],
                    g_artifactTraits[cell->m_extraInfo].m_name);
            normalDialog(g_text, 1, -1, -1, 8, cell->m_extraInfo,
                         -1, 0, -1, 0, -1, 0);
        }
        type_artifact artifact;
        memcpy(&artifact.m_artifactId, &cell->m_extraInfo,
               sizeof artifact.m_artifactId);
        artifact.m_extra = -1;
        currentHero->giveArtifact(&artifact, 1, 1);
        if (!humanPlayer)
            aiEquipArtifacts(currentHero);
    } else {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_SHIPWRECK_SURVIVOR_BACKPACK_FULL],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }

    eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
}

// The gold arm formats a general-text row with the pooled "%s." while the
// artifact arm joins an advevent.txt fragment to the artifact name with
// "%s %s" - two different resources in one body, which is what makes the
// two text pointers different globals.
VA(0x004a5480, 0x187)  // dc 0x95650
void advManager::doEventSkeleton(hero* currentHero, ExtraInfoUnion* cell,
                                 bool humanPlayer)
{
    if (cell->skeletonHasTreasure()) {
        if (currentHero->getNumberInBackpack(1) < 64) {
            type_artifact artifact;
            int artifactId = cell->getSkeletonArtifact();
            memcpy(&artifact.m_artifactId, &artifactId,
                   sizeof artifact.m_artifactId);
            artifact.m_extra = -1;
            if (humanPlayer) {
                sprintf(g_text,
                        DATA_COMPGEN(0x00660344, twoWordFormat, "%s %s"),
                        (*g_adventureEventText)[ADV_EVENT_TEXT_SKELETON_ARTIFACT_PREFIX],
                        g_artifactTraits[artifact.m_artifactId].m_name);
                normalDialog(g_text, 1, -1, -1, 8, artifact.m_artifactId,
                             -1, 0, -1, 0, -1, 0);
            }
            currentHero->giveArtifact(&artifact, 1, 1);
            if (!humanPlayer)
                aiEquipArtifacts(currentHero);
        } else {
            if (humanPlayer) {
                sprintf(g_text,
                        DATA_COMPGEN(0x00677758, sentenceFormat, "%s."),
                        (*g_generalText)[GENERAL_TEXT_TREASURE_CAPTION]);
                normalDialog(g_text, 1, -1, -1, GOLD, 1000,
                             -1, 0, -1, 0, -1, 0);
            }
            currentHero->giveResource(GOLD, 1000);
        }
        cell->setSkeleton(cell->getItemId(), 0, -1);
    } else {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_SKELETON_EMPTY],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }

    g_currentPlayer->m_deadGuyFlags |= 1 << cell->getItemId();
}

// E:\gamedcs\events.cpp:3039.  The shared handler for all three shrine
// tiers.  Dreamcast names the string local and its helper stream; retail
// fixes the packed spell as the signed ten-bit lane at bits 13..22.  The
// three refusal tails are advevent.txt rows 174 (already known), 131 (no
// spellbook), and 130 (insufficient Wisdom).
// [2026-08-27] Residual (89.67%): retail's SetInfoFlag expansion here
// CALLS game::GetTeam for the initial team lookup, where game.h's inline
// reads mapHeader.teamInfo[playerNum] directly (its own note records the
// direct read, and DispatchEvent's arms depend on it). Reconciling would
// be a shared-header change risking every SetInfoFlag consumer. The
// remainder is human_player homing (retail reloads the stack byte) and an
// esi/edi swap.
VA(0x004a5610, 0x346)  // DC identity + unique call/CFG stream, dc 0x957fc
void advManager::doEventShrine(hero* currentHero, NewmapCell* cell,
                               const char* prompt, GlobalInfoFlags type,
                               bool humanPlayer)
{
    SpellID spell = cell->getShrineSpell();
    std::string result;

    if (humanPlayer)
        result = formatString("%s'%s'.", prompt,
                               g_spellTraits[spell].m_name);

    g_game->setInfoFlag(type, g_netLocalGamePos);
    cell->setCellVisited(currentHero->m_owner);

    if (currentHero->isInSpellbook(spell)) {
        if (!humanPlayer)
            return;
        result += (*g_adventureEventText)[ADV_EVENT_TEXT_SHRINE_SPELL_KNOWN_SUFFIX];
        normalDialog(result.c_str(), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    if (!currentHero->isWieldingArtifact(ARTIFACT_SPELLBOOK)) {
        if (humanPlayer) {
            result += (*g_adventureEventText)[ADV_EVENT_TEXT_SHRINE_NO_SPELLBOOK_SUFFIX];
            normalDialog(result.c_str(), 1, -1, -1,
                         -1, 0, -1, 0, -1, 0, -1, 0);
        }
        return;
    }

    if (g_spellTraits[spell].m_level > currentHero->m_skillLevel[eSecSkillWisdom] + 2) {
        if (humanPlayer) {
            result += (*g_adventureEventText)[ADV_EVENT_TEXT_SHRINE_NO_WISDOM_SUFFIX];
            normalDialog(result.c_str(), 1, -1, -1,
                         -1, 0, -1, 0, -1, 0, -1, 0);
        }
        return;
    }

    if (humanPlayer)
        normalDialog(result.c_str(), 1, -1, -1,
                     9, spell, -1, 0, -1, 0, -1, 0);
    currentHero->addSpell(spell);
}

int aiVisitSirens(const hero* currentHero, armyGroup& army);

VA(0x004a5980, 0x100)  // dc 0x95a34
void advManager::doEventSiren(hero* currentHero, NewmapCell* cell,
                              bool humanPlayer)
{
    if (currentHero->m_flags & 0x100000) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_SIRENS_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    int experience = aiVisitSirens(currentHero, currentHero->m_army);
    if (experience) {
        if (humanPlayer) {
            sprintf(g_text,
                    (*g_adventureEventText)[ADV_EVENT_TEXT_SIRENS_EXPERIENCE_FORMAT],
                    experience);
            normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
        currentHero->giveExperience(experience, 1, 1);
    } else {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_SIRENS_NO_LOSS],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }
    currentHero->m_flags |= 0x100000;
}

// E:\gamedcs\events.cpp:3133
VA(0x004a5a80, 0x41E)  // dc-bracket forced, ret 0x10=p5, dc 0x95b54
void advManager::doCustomSpellScroll(hero* currentHero, NewmapCell* cell,
                                     type_point point, bool humanPlayer)
{
    TreasureData* treasure = getTreasureData(cell);
    int spell = cell->m_extraInfo & 0xff;
    // DC events.cpp:3136-3137 calls type_artifact(SpellID) for this scroll.
    type_artifact artifact(spell);

    if (treasure->m_hasCustomGuardians && treasure->m_guardians.getNumArmies()) {
        if (humanPlayer) {
            if (treasure->m_message.length() > 0) {
                normalDialog(treasure->m_message.c_str(),
                             2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
                    return;
            }
        } else if (aiValueOfEvent(currentHero, point) <= 0) {
            return;
        }

        if (doCombat(point, currentHero, &currentHero->m_army, -1, 0, 0,
                     &treasure->m_guardians, -1, 1, 0))
            return;
        currentHero->checkLevel();
        if (humanPlayer) {
            sprintf(g_text,
                    (*g_adventureEventText)[ADV_EVENT_TEXT_CUSTOM_GUARDED_REWARD_FORMAT],
                    g_spellTraits[spell].m_name);
            normalDialog(g_text, 1, -1, -1, 9, spell, -1, 0, -1, 0, -1, 0);
        }
        currentHero->giveArtifact(&artifact, 1, 1);
        if (!humanPlayer)
            aiEquipArtifacts(currentHero);
        eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
        return;
    }

    if (humanPlayer) {
        if (treasure->m_message.length() > 0) {
            normalDialog(treasure->m_message.c_str(),
                         1, -1, -1, 9, spell, -1, 0, -1, 0, -1, 0);
        } else {
            std::string text;
            text = formatString((*g_adventureEventText)[ADV_EVENT_TEXT_SPELL_SCROLL_FORMAT],
                                 g_spellTraits[spell].m_name);
            normalDialog(text.c_str(),
                         1, -1, -1, 9, spell, -1, 0, -1, 0, -1, 0);
        }
    }
    currentHero->giveArtifact(&artifact, 1, 1);
    if (!humanPlayer)
        aiEquipArtifacts(currentHero);
    eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
}

VA(0x004a5ea0, 0x1F9)  // dc 0x95e14
void advManager::doEventSpellScroll(hero* currentHero, NewmapCell* cell,
                                    type_point point, bool humanPlayer)
{
    if (currentHero->getNumberInBackpack(1) >= 64) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_BACKPACK_FULL],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    if (cell->isCustomized()) {
        doCustomSpellScroll(currentHero, cell, point, humanPlayer);
        return;
    }

    SpellID spell = cell->m_extraInfo;
    // DC events.cpp calls the same scroll constructor in the ordinary arm.
    type_artifact scroll(spell);
    if (humanPlayer) {
        sprintf(g_text,
                (*g_adventureEventText)[ADV_EVENT_TEXT_SPELL_SCROLL_FORMAT],
                g_spellTraits[spell].m_name);
        normalDialog(g_text, 1, -1, -1, 9, spell, -1, 0, -1, 0, -1, 0);
    }
    currentHero->giveArtifact(&scroll, 1, 1);
    if (!humanPlayer)
        aiEquipArtifacts(currentHero);

    eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
}

VA(0x004a60a0, 0x160)  // dc 0x95f18
void advManager::doEventStables(hero* currentHero, NewmapCell* cell,
                                bool humanPlayer)
{
    char granted = STABLES_NOTHING;

    if (!(currentHero->m_flags & 2)) {
        currentHero->m_flags |= 2;
        currentHero->m_maxMovePoints += g_stablesMovementBonus;
        currentHero->m_movePoints += g_stablesMovementBonus;
        if (humanPlayer)
            m_advWindow->updateHeroLocators(-1, 1, 1);
        granted = STABLES_MOVEMENT;
    }
    // 10 and 11 are the Cavalier and its Champion upgrade; the reward
    // dialogs below show 11 as their picture for the same reason.
    if (currentHero->creatureTypeCount(10)) {
        currentHero->upgradeCreatures(10, 11);
        granted |= STABLES_UPGRADE;
    }

    if (!humanPlayer)
        return;

    switch (granted) {
    case STABLES_NOTHING:
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_STABLES_NOTHING],
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        break;
    case STABLES_MOVEMENT:
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_STABLES_MOVEMENT],
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        break;
    case STABLES_UPGRADE:
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_STABLES_UPGRADE],
                     1, -1, -1, 0x15, 11, -1, 0, -1, 0, -1, 0);
        break;
    case STABLES_MOVEMENT_AND_UPGRADE:
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_STABLES_BOTH],
                     1, -1, -1, 0x15, 11, -1, 0, -1, 0, -1, 0);
        break;
    }
}

VA(0x004a6200, 0x12C)  // dc 0x960ac
void advManager::doEventTemple(hero* currentHero, NewmapCell* cell,
                               bool humanPlayer)
{
    if (!(currentHero->m_flags & 0x4000100)) {
        game* g = g_game;
        g->setInfoFlag(TempleInfo, g_netLocalGamePos);
        if (g_game->m_day == DAY_OF_WEEK_SUNDAY) {
            currentHero->m_flags |= 0x4000000;
            currentHero->m_moraleBonus += 2;
            if (humanPlayer)
                normalDialog(
                    (*g_adventureEventText)[ADV_EVENT_TEXT_TEMPLE],
                    1, -1, -1, 14, 0, 14, 0, -1, 0, -1, 0);
        } else {
            currentHero->m_flags |= 0x100;
            currentHero->m_moraleBonus++;
            if (humanPlayer)
                normalDialog(
                    (*g_adventureEventText)[ADV_EVENT_TEXT_TEMPLE],
                    1, -1, -1, 14, 0, -1, 0, -1, 0, -1, 0);
        }
    } else if (humanPlayer) {
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_TEMPLE_VISITED],
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }
}

VA(0x004a6330, 0x106)  // dc 0x961dc
void advManager::doEventTrainingGrounds(hero* currentHero, NewmapCell* cell,
                                        bool humanPlayer)
{
    if (currentHero->m_trainingGroundsFlags & (1 << cell->m_extraInfo)) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_TRAINING_GROUNDS_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    int amount = static_cast<int>(currentHero->getExperienceBonusFactor()
                                  * 1000.0f);
    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_TRAINING_GROUNDS],
                     1, -1, -1, 0x11, amount, -1, 0, -1, 0, -1, 0);
    currentHero->giveExperience(amount, 0, 1);
    currentHero->m_trainingGroundsFlags |= 1 << cell->m_extraInfo;
    // The `game* g` spelling, do_event_watering_hole's lever: it is what
    // puts the player position first in the SIB of the inlined
    // teamInfo[playerNum] load (`[pos + gpGame]`). The four
    // primary-skill handlers above want the direct call for the same
    // byte, so the choice stays per-call-site.
    game* g = g_game;
    g->setInfoFlag(TrainingGroundsInfo, g_netLocalGamePos);
    currentHero->checkLevel();
}

// The AI arm of DoTreasureDialog reaches this philai helper before the
// declaration accompanying its later tree-of-knowledge callers.
unsigned char aiChooseResourceOrExperience(const hero* currentHero,
                                               EGameResource resource,
                                               int cost, int value);

// E:\gamedcs\events.cpp:3377. The gold-or-experience offer shared by the
// treasure chest and the campfire-style pickups.
// A shared choice result with the gold arm first removes both experience
// joins and raises 83.0357% to 94.4643%. The separate CHOICE_1 resource
// return remains, as do the canonical experience/resource and AI helpers.
// Bool, unsigned char and int choice results reproduce the same winner;
// reversing the final two arms is neutral at the old 83.0357%. A single
// breakable choice scope is also neutral, while either individual copied
// GiveExperience/return exit lowers the match. This 44-state family checks
// all event siblings; the witch-hut refusal alternatives remain lower.
VA(0x004a6440, 0xD8)  // dc-bracket forced, ret 0xc=p4, dc 0x962dc
void advManager::doTreasureDialog(hero* currentHero, int amount,
                                  bool humanPlayer)
{
    bool takeExperience = 0;
    int experience = static_cast<int>(currentHero->getExperienceBonusFactor()
                                      * (amount - 500));

    if (humanPlayer) {
        overrideBottomView(BOTTOM_VIEW_DEFAULT, -1);
        updBottomView(0, 1, 1);
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_TREASURE_GOLD_OR_EXPERIENCE], 7, -1, -1, GOLD,
                     amount, 0x11, experience, 1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT) {
            if (g_windowManager->m_dialogReturn == DIALOG_RETURN_CHOICE_1) {
                currentHero->giveResource(GOLD, amount);
                return;
            }
            takeExperience = 1;
        }
    } else if (!aiChooseResourceOrExperience(currentHero, GOLD, amount,
                                                 experience)) {
        takeExperience = 1;
    }

    if (!takeExperience)
        currentHero->giveResource(GOLD, amount);
    else
        currentHero->giveExperience(experience, 0, 1);
}

// The chest is the only object in this file that ends with CheckLevel -
// DoTreasureDialog can pay experience instead of gold, so the hero may
// have levelled by the time the fizzle is over.
VA(0x004a6520, 0x1ED)  // dc 0x963d8
void advManager::doEventTreasure(hero* currentHero, NewmapCell* cell,
                                 type_point point, bool humanPlayer)
{
    if (cell->treasureIsArtifact()) {
        if (currentHero->getNumberInBackpack(1) < 64) {
            type_artifact artifact;
            int artifactId = cell->getTreasureArtifact();
            memcpy(&artifact.m_artifactId, &artifactId,
                   sizeof artifact.m_artifactId);
            artifact.m_extra = -1;
            if (humanPlayer) {
                sprintf(g_text,
                        (*g_adventureEventText)[ADV_EVENT_TEXT_TREASURE_ARTIFACT_FORMAT],
                        g_artifactTraits[artifact.m_artifactId].m_name);
                normalDialog(g_text, 1, -1, -1, 8, artifact.m_artifactId,
                             -1, 0, -1, 0, -1, 0);
            }
            currentHero->giveArtifact(&artifact, 1, 1);
            if (!humanPlayer)
                aiEquipArtifacts(currentHero);
        } else {
            doTreasureDialog(currentHero, 1000, humanPlayer);
        }
    } else {
        doTreasureDialog(currentHero, cell->getTreasureSize(), humanPlayer);
    }

    eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
    currentHero->checkLevel();
}

// The visit mask is materialised at the TOP - `1 << GetItemId()` into a
// frame slot - and read back for the write-back after the award, which is
// what one named local produces where two `1 << id` expressions would have
// been recomputed.
VA(0x004a6710, 0x29D)  // dc 0x964c4
void advManager::doEventTreeOfKnowledge(hero* currentHero,
                                        ExtraInfoUnion* cell,
                                        bool humanPlayer)
{
    unsigned long visited = 1 << cell->getItemId();
    if (currentHero->m_treeOfKnowledgeFlags & visited) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_TREE_OF_KNOWLEDGE_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    cell->setCellVisited(currentHero->m_owner);
    game* g = g_game;
    g->setInfoFlag(TreeOfKnowledgeInfo, g_netLocalGamePos);
    int experience = currentHero->getExperienceIncrement();

    switch (cell->getTreePrice()) {
    case const_tree_wants_nothing:
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_TREE_OF_KNOWLEDGE_FREE],
                         1, -1, -1, 0x11, -1, -1, 0, -1, 0, -1, 0);
        break;
    case const_tree_wants_gold:
        if (g_currentPlayer->m_resources[GOLD] < 2000) {
            if (humanPlayer)
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_TREE_OF_KNOWLEDGE_NO_GOLD],
                             1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            return;
        }
        if (humanPlayer) {
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_TREE_OF_KNOWLEDGE_GOLD_PROMPT],
                         2, -1, -1, 0x11, -1, -1, 0, -1, 0, -1, 0);
            if (g_windowManager->m_dialogReturn == DIALOG_RETURN_DECLINE)
                return;
        } else if (aiChooseResourceOrExperience(currentHero, GOLD, 2000,
                                                    experience)) {
            return;
        }
        g_currentPlayer->m_resources[GOLD] -= 2000;
        break;
    case const_tree_wants_gems:
        if (g_currentPlayer->m_resources[GEMS] < 10) {
            if (humanPlayer)
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_TREE_OF_KNOWLEDGE_NO_GEMS],
                             1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            return;
        }
        if (humanPlayer) {
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_TREE_OF_KNOWLEDGE_GEMS_PROMPT],
                         2, -1, -1, 0x11, -1, -1, 0, -1, 0, -1, 0);
            if (g_windowManager->m_dialogReturn == DIALOG_RETURN_DECLINE)
                return;
        } else if (aiChooseResourceOrExperience(currentHero, GEMS, 10,
                                                    experience)) {
            return;
        }
        g_currentPlayer->m_resources[GEMS] -= 10;
        break;
    }

    currentHero->giveExperience(experience, 0, 1);
    currentHero->m_treeOfKnowledgeFlags |= visited;
    currentHero->checkLevel();
}

VA(0x004a69b0, 0x178)  // dc 0x96784
void advManager::doEventWagon(hero* currentHero, ExtraInfoUnion* cell,
                              bool humanPlayer)
{
    cell->setCellVisited(currentHero->m_owner);
    if (!cell->wagonIsFull()) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WAGON_EMPTY],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    if (cell->wagonHasArtifact()
        && currentHero->getNumberInBackpack(1) < 64) {
        type_artifact artifact;
        int artifactId = cell->getWagonArtifact();
        memcpy(&artifact.m_artifactId, &artifactId,
               sizeof artifact.m_artifactId);
        artifact.m_extra = -1;
        if (humanPlayer) {
            sprintf(g_text,
                    (*g_adventureEventText)[ADV_EVENT_TEXT_WAGON_ARTIFACT_FORMAT],
                    g_artifactTraits[artifact.m_artifactId].m_name);
            // iResType1 8 is the artifact picture class, exactly as the
            // warrior's tomb passes it.
            normalDialog(g_text, 1, -1, -1, 8, artifact.m_artifactId,
                         -1, 0, -1, 0, -1, 0);
        }
        currentHero->giveArtifact(&artifact, 1, 1);
        if (!humanPlayer)
            aiEquipArtifacts(currentHero);
    } else {
        EGameResource resource = cell->getWagonResource();
        short amount = cell->getWagonAmount();
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WAGON_RESOURCE],
                         1, -1, -1, resource, amount, -1, 0, -1, 0, -1, 0);
        currentHero->giveResource(resource, amount);
    }
    cell->emptyWagon();
}

TCreatureType upgradedCreatureType(TCreatureType type);
TCreatureType downgradedCreatureType(TCreatureType type);
int isBaseCreature(TCreatureType type);

VA(0x004a6b30, 0x12A)  // dc 0x96994
void advManager::monstersGiveReward(hero* currentHero, NewmapCell* cell,
                                      bool humanPlayer)
{
    if (!cell->isCustomized())
        return;

    MonsterData* reward = &m_fullMap->m_customMonsterList[cell->m_monsterInfo.m_index];
    if (reward->m_artifact != ARTIFACT_NONE) {
        if (currentHero->getNumberInBackpack(1) >= 64) {
            if (humanPlayer)
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_BACKPACK_FULL], 1, -1, -1,
                             -1, 0, -1, 0, -1, 0, -1, 0);
        } else {
            if (humanPlayer)
                normalDialog("", 1, -1, -1, 8,
                             reward->m_artifact, -1, 0, -1, 0, -1, 0);
            // The Complete monster reward stores a decoded map ordinal; type_artifact retains its DC TArtifact constructor.
            type_artifact artifact(static_cast<TArtifact>(reward->m_artifact) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */);
            currentHero->giveArtifact(&artifact, 1, 1);
            if (!humanPlayer)
                aiEquipArtifacts(currentHero);
        }
    }

    int resource = 0;
    const int* qty = reward->m_resQty;
    int remaining = 7;
    do {
        if (*qty) {
            if (humanPlayer)
                normalDialog("", 1, -1, -1, resource,
                             *qty, -1, 0, -1, 0, -1, 0);
            currentHero->giveResource(resource, *qty);
        }
        ++resource;
        ++qty;
        --remaining;
    } while (remaining);
}

// EraseAndFizzle is INLINED here - its own out-of-line body stays emitted
// for its external linkage, and FizzleCenter is inlined inside it in turn,
// so the whole save/force/restore bracket and the 224x224 kill fade appear
// in this function. With the sound fixed at KILL_FADE the fizzle's switch
// folds away and only its gCompleteDrawEnabled gate survives - as an `if`
// around the block rather than as an early return, which is why the two
// restores appear ONCE here where EraseAndFizzle's own body carries three
// copies of them.

VA(0x004a6c60, 0x188)  // dc 0x96b14
void advManager::monstersFight(hero* currentHero, NewmapCell* cell,
                                type_point point, bool humanPlayer)
{
    TCreatureType monType;
    {
        monType = TCreatureType(cell->m_objectIndex);
    }
    int numMons = cell->m_monsterInfo.m_qty;
    int survived = combatMonsterEvent(currentHero, monType, &numMons,
                                      cell, point,
                                      CREATURE_NONE, 0, 0,
                                      CREATURE_NONE, 0, 0);
    cell->m_monsterInfo.m_qty = numMons;

    if (!survived) {
        monstersGiveReward(currentHero, cell, humanPlayer);
        if (g_game->m_mapHeader.m_victoryCondition.checkForDefeatedMonsterWin(
                currentHero, point))
            checkEndGame(0);
    }

    if (numMons == 0)
        eraseAndFizzle(cell, point, FIZZLE_SOUND_KILL_FADE);
}

void aiJoinDecision(hero* currentHero, TCreatureType creature, short amount);
unsigned char aiBribeMonsters(const hero* currentHero, NewmapCell* cell,
                                TCreatureType type, short amount,
                                long goldCost);
void doMonsterJoinDialog(hero* inHero, TCreatureType type, int amount);

// EraseAndFizzle is inlined here exactly as in monsters_fight, and with
// it FizzleCenter, so the save/force/restore bracket and the 224x224
// fade appear in this body.

// The two monsters_fight calls are separate sites with their own
// epilogues - the human arm forwards `human_player` and the AI arm the
// literal false, which is what keeps them from tail-merging.
VA(0x004a6df0, 0x20B)  // dc 0x96c18
void advManager::monstersFlee(hero* currentHero, NewmapCell* cell,
                               type_point point, bool humanPlayer)
{
    TCreatureType monType;
    {
        monType = TCreatureType(cell->m_objectIndex);
    }

    if (humanPlayer) {
        sprintf(g_text,
                (*g_adventureEventText)[ADV_EVENT_TEXT_MONSTERS_FLEE_FORMAT],
                getArmyName(monType, 2));
        normalDialog(g_text, 2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn == DIALOG_RETURN_ACCEPT) {
            monstersFight(currentHero, cell, point, humanPlayer);
            return;
        }
    } else if (aiValueOfEvent(currentHero, point) > 0) {
        monstersFight(currentHero, cell, point, false);
        return;
    }

    g_game->m_worldMap.notifyMonsterDefeated(point, currentHero->m_owner);
    eraseAndFizzle(cell, point, FIZZLE_SOUND_KILL_FADE);
    if (g_game->m_mapHeader.m_victoryCondition.checkForDefeatedMonsterWin(
            currentHero, point))
        checkEndGame(0);
}

// Joining is the same three steps in both surviving-army handlers: tell
// the map's object records the tile changed hands, put the creatures in
// the hero's army, and report the overflow when they do not fit - the
// human through townmgr's dialog, the AI through its own appraisal.
// monsters_give_reward then runs whatever happened to the army.
VA(0x004a7000, 0x248)  // dc 0x96d54
bool advManager::monstersJoin(hero* currentHero, NewmapCell* cell,
                               type_point point, bool wantToFight,
                               bool humanPlayer)
{
    TCreatureType monType;
    {
        monType = TCreatureType(cell->m_objectIndex);
    }
    int numMons = cell->m_monsterInfo.m_qty;

    if (humanPlayer) {
        sprintf(g_text,
                (*g_adventureEventText)[ADV_EVENT_TEXT_MONSTERS_JOIN_FORMAT],
                getArmyName(monType, 2));
        normalDialog(g_text, 2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn == DIALOG_RETURN_DECLINE) {
            if (wantToFight)
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MONSTERS_INSULTED],
                             1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            return false;
        }
    }

    g_game->m_worldMap.notifyMonsterDefeated(point, currentHero->m_owner);
    if (!currentHero->m_army.add(monType, numMons, -1)) {
        if (humanPlayer)
            doMonsterJoinDialog(currentHero, monType, numMons);
        else
            aiJoinDecision(currentHero, monType, numMons);
    }

    monstersGiveReward(currentHero, cell, humanPlayer);
    if (humanPlayer)
        updBottomView(1, 1, 1);

    eraseAndFizzle(cell, point, FIZZLE_SOUND_KILL_FADE);
    return true;
}

// The prompt is split by count, and the plural form is built in two
// pieces: the lead-in is copied whole into gText and the (count, name,
// gold) tail is formatted into a 300-byte local and appended, which is
// what the 0x13c frame is for. Both forms carry the price as an
// iResType1 GOLD picture on the dialog.

VA(0x004a7250, 0x36F)  // dc 0x96eec
bool advManager::monstersSellOut(hero* currentHero, NewmapCell* cell,
                                   type_point point, bool wantToFight,
                                   bool humanPlayer)
{
    TCreatureType monType;
    {
        monType = TCreatureType(cell->m_objectIndex);
    }
    int numMons = cell->m_monsterInfo.m_qty;
    int cost = g_creatureTypeTraits[monType].m_cost[GOLD] * numMons;

    if (cost > g_game->m_players[currentHero->m_owner].m_resources[GOLD])
        return false;

    if (!humanPlayer) {
        if (!aiBribeMonsters(currentHero, cell, monType, numMons, cost))
            return false;
    } else {
        if (numMons == 1) {
            sprintf(g_text,
                    (*g_adventureEventText)[ADV_EVENT_TEXT_MONSTERS_SELL_OUT_ONE_FORMAT],
                    getArmyName(monType, 1), cost);
        } else {
            char text[300];
            strcpy(g_text, (*g_adventureEventText)[ADV_EVENT_TEXT_MONSTERS_SELL_OUT_PREFIX]);
            sprintf(text,
                    (*g_adventureEventText)[ADV_EVENT_TEXT_MONSTERS_SELL_OUT_MANY_FORMAT],
                    numMons, getArmyName(monType, 2), cost);
            strcat(g_text, text);
        }
        normalDialog(g_text, 2, -1, -1, GOLD, cost, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn == DIALOG_RETURN_DECLINE) {
            if (wantToFight)
                normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_MONSTERS_INSULTED],
                             1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            return false;
        }
    }

    g_game->m_worldMap.notifyMonsterDefeated(point, currentHero->m_owner);
    g_game->m_players[currentHero->m_owner].m_resources[GOLD] -= cost;
    if (!currentHero->m_army.add(monType, numMons, -1)) {
        if (humanPlayer)
            doMonsterJoinDialog(currentHero, monType, numMons);
        else
            aiJoinDecision(currentHero, monType, numMons);
    }

    monstersGiveReward(currentHero, cell, humanPlayer);
    if (humanPlayer) {
        m_advWindow->updateResourceDisplay(1, 1);
        updBottomView(1, 1, 1);
    }

    eraseAndFizzle(cell, point, FIZZLE_SOUND_KILL_FADE);
    return true;
}

// The elemental guards are the Armageddon's Blade content switch. Without
// it the four base elementals have no upgrade and the four upgraded ones
// have no base, so each dwelling walk is skipped rather than called; the
// guard is spelled at both call sites, which is why `gpGame->f_1f698` is
// read three times and reloaded only after the intervening call.
VA(0x004a75c0, 0xFD)  // dc 0x97144
int advManager::getLikeModifier(hero* currentHero, TCreatureType creature)
{
    int kinCount = 0;
    int armyCount = 0;
    TCreatureType like;

    if ((!g_game->m_gameVersion
         && isBaseElemental(creature))
        || g_creatureTypeTraits[creature].m_townType == -1) {
        like = CREATURE_NONE;
    } else {
        if (!g_game->m_gameVersion
            && isBaseElemental(creature))
            like = CREATURE_NONE;
        else
            like = upgradedCreatureType(creature);
        if (like == CREATURE_NONE) {
            if (!g_game->m_gameVersion
                && (creature == CREATURE_ICE_ELEMENTAL
                    || creature == CREATURE_STORM_ELEMENTAL
                    || creature == CREATURE_MAGMA_ELEMENTAL
                    || creature == CREATURE_ENERGY_ELEMENTAL))
                like = CREATURE_NONE;
            else
                like = downgradedCreatureType(creature);
        }
    }

    for (int i = 0; i < 7; i++) {
        if (currentHero->m_army.m_numTroops[i] > 0) {
            armyCount += currentHero->m_army.m_numTroops[i];
            int type = currentHero->m_army.m_armies[i];
            if (type == creature || type == like)
                kinCount += currentHero->m_army.m_numTroops[i] * 2;
        }
    }

    if (kinCount == 0)
        return 0;
    return kinCount > armyCount ? 2 : 1;
}

VA(0x004a76c0, 0x75)  // dc 0x97228
int advManager::getForceModifier(float strengthRatio)
{
    if (strengthRatio >= 7.0)
        return 11;
    if (strengthRatio >= 1.0)
        return static_cast<int>(strengthRatio * 2 - 2);
    if (strengthRatio > 0.5)
        return -1;
    if (strengthRatio > 0.333)
        return -2;
    return -3;
}

// `?AI_approximate_strength@@YIJPBVhero@@@Z`.
long aiApproximateStrength(const hero* currentHero);

// The Easy-difficulty bonus is the reason `setup.difficulty` is read at
// all: on difficulty 0 a HUMAN player's diplomacy counts one grade
// higher, capped at expert.
VA(0x004a7740, 0x27E)  // dc 0x97364
void advManager::doWanderingMonsterResult(NewmapCell* cell,
                                          hero* currentHero, type_point point,
                                          bool humanPlayer)
{
    TCreatureType monType;
    {
        monType = TCreatureType(cell->m_objectIndex);
    }
    int numTroops = cell->m_monsterInfo.m_qty;
    int disposition = cell->m_monsterInfo.m_disposition;

    if (cell->isCustomized()) {
        MonsterData* customMonster =
            &m_fullMap->m_customMonsterList[cell->m_monsterInfo.m_index];
        if (humanPlayer && customMonster->m_message.size() > 0)
            normalDialog(customMonster->m_message.c_str(), 1, -1, -1, -1, 0, -1,
                         0, -1, 0, -1, 0);
    }

    float strengthRatio =
        static_cast<float>(aiApproximateStrength(currentHero))
        / static_cast<float>(g_creatureTypeTraits[monType].m_aiValue
                             * numTroops);
    short likeModifier = getLikeModifier(currentHero, monType);
    short diplomacy = currentHero->m_skillLevel[eSecSkillDiplomacy];
    short forceModifier = getForceModifier(strengthRatio);

    if (!g_game->m_setup.m_difficulty && humanPlayer) {
        int cappedDiplomacy = cppMin(diplomacy + 1, 3);
        diplomacy = cappedDiplomacy;
    }

    if (disposition > likeModifier + forceModifier + diplomacy) {
        monstersFight(currentHero, cell, point, humanPlayer);
        return;
    }

    bool wantToFight =
        disposition == likeModifier + forceModifier + diplomacy;
    if (disposition <= likeModifier + diplomacy + 1) {
        if (monstersJoin(currentHero, cell, point, wantToFight,
                          humanPlayer)) {
            if (g_game->m_mapHeader.m_victoryCondition.checkForDefeatedMonsterWin(
                    currentHero, point))
                checkEndGame(0);
            return;
        }
    } else if (disposition <= likeModifier + diplomacy * 2 + 1) {
        if (monstersSellOut(currentHero, cell, point, wantToFight,
                              humanPlayer)) {
            if (g_game->m_mapHeader.m_victoryCondition.checkForDefeatedMonsterWin(
                    currentHero, point))
                checkEndGame(0);
            return;
        }
    }

    if (wantToFight || cell->m_monsterInfo.m_neverFlee)
        monstersFight(currentHero, cell, point, humanPlayer);
    else
        monstersFlee(currentHero, cell, point, humanPlayer);
}

VA(0x004a79c0, 0x79)  // dc 0x975a0
void advManager::doEventWanderingMonster(NewmapCell* cell, hero* currentHero,
                                         type_point point, bool humanPlayer)
{
    m_movingObjectIndex = cell->m_objectTypeIndex;
    m_movingObjectSequence = cell->m_objectIndex;
    m_movingObjectFrame = currentHero->m_x < point.m_x;
    completeDraw(false);
    updateScreen(0, 0);
    doWanderingMonsterResult(cell, currentHero, point, humanPlayer);
    m_movingObjectIndex = -1;
    m_movingObjectSequence = -1;
}

// A human picks from a two-picture dialog (iMBType 10, the primary-skill
// pictures 0x1f and 0x20 with +1 each) and the reply is SWITCHED over
// three values with the decrement idiom; a cancel leaves the school
// untouched and unpaid. The AI instead asks whether 1000 gold is worth
// one level's experience increment, and then trains whichever of its two
// clamped skills is lower - ties go to Attack.
VA(0x004a7a40, 0x1EA)  // dc 0x97628
void advManager::doEventWarSchool(hero* currentHero, ExtraInfoUnion* cell,
                                  bool humanPlayer)
{
    if (currentHero->m_warSchoolFlags & (1 << cell->m_value)) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WAR_SCHOOL_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    game* g = g_game;
    g->setInfoFlag(WarSchoolInfo, g_netLocalGamePos);
    if (g_currentPlayer->m_resources[GOLD] < 1000) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WAR_SCHOOL_NO_GOLD],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    int whichStat = 0;
    if (humanPlayer) {
        overrideBottomView(BOTTOM_VIEW_DEFAULT, -1);
        updBottomView(0, 1, 1);
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WAR_SCHOOL_CHOOSE],
                     10, -1, -1, 0x1f, 1, 0x20, 1, -1, 0, -1, 0);
        switch (g_windowManager->m_dialogReturn) {
        case DIALOG_RETURN_CANCEL:
            return;
        case DIALOG_RETURN_CHOICE_1:
            whichStat = 0;
            break;
        case DIALOG_RETURN_CHOICE_2:
            whichStat = 1;
            break;
        }
    } else {
        if (aiChooseResourceOrExperience(currentHero, GOLD, 1000,
                              currentHero->getExperienceIncrement()))
            return;
        if (currentHero->getPrimarySkill(0) > currentHero->getPrimarySkill(1))
            whichStat = 1;
    }

    currentHero->adjustPrimarySkill(whichStat, 1);
    currentHero->m_warSchoolFlags |= 1 << cell->m_value;
    g_currentPlayer->m_resources[GOLD] -= 1000;
}

// The two guards on the AI arm are what the prompt replaces: it declines
// tombs its own team has already opened (the inlined PlayerKnowsCell over
// the eight-bit visited lane) and tombs it could not carry the artifact
// out of (a full 64-slot backpack). The human arm asks instead, and reads
// the answer back out of heroWindowManager::dialogReturn.

VA(0x004a7c30, 0x1A1)  // dc 0x9784c
void advManager::doEventWarriorTomb(hero* currentHero, ExtraInfoUnion* cell,
                                       bool humanPlayer)
{
    if (humanPlayer) {
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WARRIOR_TOMB_PROMPT],
                     2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
            return;
    } else {
        if (cell->playerKnowsCell(g_netLocalGamePos))
            return;
        if (currentHero->getNumberInBackpack(1) >= 64)
            return;
    }

    if (cell->tombIsFull() && currentHero->getNumberInBackpack(1) < 64) {
        type_artifact artifact;
        int artifactId = cell->getTombArtifact();
        memcpy(&artifact.m_artifactId, &artifactId,
               sizeof artifact.m_artifactId);
        artifact.m_extra = -1;
        if (humanPlayer) {
            sprintf(g_text,
                    (*g_adventureEventText)[ADV_EVENT_TEXT_WARRIOR_TOMB_ARTIFACT_FORMAT],
                    g_artifactTraits[artifact.m_artifactId].m_name);
            // iResType1 8 is the artifact picture class; the water wheel's
            // twin passes a resource id in the same slot.
            normalDialog(g_text, 1, -1, -1, 8, artifact.m_artifactId,
                         -1, 0, -1, 0, -1, 0);
        }
        currentHero->giveArtifact(&artifact, 1, 1);
        if (!humanPlayer)
            aiEquipArtifacts(currentHero);
        cell->emptyTomb();
    } else {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WARRIOR_TOMB_EMPTY],
                         1, -1, -1, 16, 0, 16, 0, -1, 0, 16, 0);
    }

    cell->setCellVisited(currentHero->m_owner);
    if (!(currentHero->m_flags & 0x200000)) {
        currentHero->m_flags |= 0x200000;
        currentHero->m_moraleBonus -= 3;
    }
}

VA(0x004a7de0, 0xB1)  // dc 0x97a9c
void advManager::doEventWaterWheel(hero* currentHero, ExtraInfoUnion* cell,
                                      bool humanPlayer)
{
    int gold = cell->getWheelGold();

    cell->setCellVisited(currentHero->m_owner);
    if (gold == 0) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WATER_WHEEL_EMPTY],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    } else {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WATER_WHEEL_GOLD],
                         1, -1, -1, GOLD, gold, -1, 0, -1, 0, -1, 0);
        currentHero->giveResource(GOLD, gold);
        cell->setWheelGold(0);
    }
}

// The eight-iteration teamInfo scan in the middle is game::SetInfoFlag
// (Game.h:917) inlined; the flag index 27 is WateringHoleInfo, which is
// what fixes the +0x4e35f byte as globalInfoFlags[27].
VA(0x004a7ea0, 0x111)  // dc 0x97b7c
void advManager::doEventWateringHole(hero* currentHero, NewmapCell* cell,
                                        bool humanPlayer)
{
    if (currentHero->m_flags & 0x40) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WATERING_HOLE_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }
    if (humanPlayer)
        normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WATERING_HOLE],
                     1, -1, -1, 14, 0, -1, 0, -1, 0, -1, 0);
    game* g = g_game;
    g->setInfoFlag(WateringHoleInfo, g_netLocalGamePos);
    currentHero->m_flags |= 0x40;
    currentHero->m_moraleBonus++;
    currentHero->m_maxMovePoints += 400;
    currentHero->m_movePoints += 400;
    if (humanPlayer)
        m_advWindow->updateHeroLocators(-1, 1, 1);
}

VA(0x004a7fc0, 0xBD)  // dc 0x97cac
void advManager::doEventWindmill(hero* currentHero, ExtraInfoUnion* cell,
                                   bool humanPlayer)
{
    short amount = cell->getWindmillAmount();
    EGameResource resource = cell->getWindmillResource();

    cell->setCellVisited(currentHero->m_owner);
    if (amount == 0) {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WINDMILL_EMPTY],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    } else {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WINDMILL_RESOURCE],
                         1, -1, -1, resource, amount, -1, 0, -1, 0, -1, 0);
        currentHero->giveResource(resource, amount);
        cell->setWindmill(resource, 0);
    }
}

VA(0x004a8080, 0x1A5)  // dc 0x97dc8
void advManager::doEventWitchHut(hero* currentHero, ExtraInfoUnion* cell,
                                    bool humanPlayer)
{
    int skill = cell->getWitchSkill();

    cell->setCellVisited(currentHero->m_owner);
    g_game->setInfoFlag(WitchHutInfo, g_netLocalGamePos);
    if (skill == -1) {
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_WITCH_HUT_NO_SKILL),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    } else {
        if (currentHero->m_skillLevel[skill]) {
            if (humanPlayer) {
                sprintf(g_text,
                        g_adventureEventText->getText(
                            ADV_EVENT_TEXT_WITCH_HUT_KNOWN_FORMAT),
                        g_sSkillTraits[skill].m_name);
            }
        } else if (currentHero->m_skillCount >= 8) {
            if (humanPlayer) {
                sprintf(g_text,
                        g_adventureEventText->getText(
                            ADV_EVENT_TEXT_WITCH_HUT_FULL_FORMAT),
                        g_sSkillTraits[skill].m_name);
            }
        } else {
            if (humanPlayer) {
                sprintf(g_text,
                        g_adventureEventText->getText(
                            ADV_EVENT_TEXT_WITCH_HUT_LEARN_FORMAT),
                        g_sSkillTraits[skill].m_name);
                // iResType1 20 is the secondary-skill picture class and the
                // extra is the icon slot: three mastery frames per skill, the
                // basic one being 3*skill + 3.
                normalDialog(g_text, 1, -1, -1, 20, skill * 3 + 3,
                             -1, 0, -1, 0, -1, 0);
            }
            currentHero->giveSS(skill, 1);
            return;
        }
        if (humanPlayer)
            normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }
}

VA(0x004a8230, 0x154)  // dc 0x97fa4
void advManager::doEventLithOneWay(hero* currentHero, NewmapCell* cell,
                                       bool humanPlayer)
{
    type_point point;
    if (!g_game->getRandomLithExit(cell->m_objectIndex, point))
        return;

    NewmapCell* exitCell = g_game->getCell(point);
    if (exitCell->m_type == HERO) {
        if (g_remoteOn) {
            CSetVisibilityMsg message(point, g_netLocalGamePos, 1);
            transmitRemoteData(&message, 0x7f, 0, 1);
        }
        g_game->setVisibility(point.m_x, point.m_y, point.m_z,
                              g_netLocalGamePos, 1, 0);
        doEventHero(currentHero, exitCell, point, humanPlayer);
        if (currentHero->m_owner < 0)
            return;
    }

    stopCursor(1);
    teleportTo(currentHero, point,
               DATA_COMPGEN(0x0067775c, teleportOutSampleName,
                            "telptout.wav"),
               0, 1, 0);
}

VA(0x004a8390, 0x155)  // dc 0x980e8
void advManager::doEventLithTwoWay(hero* currentHero, NewmapCell* cell,
                                       bool humanPlayer)
{
    type_point point;
    if (!g_game->getRandomLith(cell->m_objectIndex, cell->m_extraInfo, point))
        return;

    NewmapCell* exitCell = g_game->getCell(point);
    if (exitCell->m_type == HERO) {
        if (g_remoteOn) {
            CSetVisibilityMsg message(point, g_netLocalGamePos, 1);
            transmitRemoteData(&message, 0x7f, 0, 1);
        }
        g_game->setVisibility(point.m_x, point.m_y, point.m_z,
                              g_netLocalGamePos, 1, 0);
        doEventHero(currentHero, exitCell, point, humanPlayer);
        if (currentHero->m_owner < 0)
            return;
    }

    stopCursor(1);
    g_advManager->teleportTo(currentHero, point, "telptout.wav", 0, 1, 0);
}

// NewmapCell now has its CodeView-proven ExtraInfoUnion base. Event
// handlers take that base directly; the old cellExtra cast wrapper is gone.
inline void advManager::doEventLighthouse(NewmapCell* cell,
                                          unsigned char humanPlayer)
{
    if (!g_game->onSameTeam(g_game->m_mines[cell->m_extraInfo].m_playerOwner,
                            g_netLocalGamePos)) {
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_LIGHTHOUSE),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        g_game->claimMine(cell->m_extraInfo, g_netLocalGamePos,
                          const_normal_action);
    }
}

inline void advManager::doEventMermaid(hero* currentHero, NewmapCell* cell,
                                       unsigned char humanPlayer)
{
    if (currentHero->m_flags & 0x8000) {
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_MERMAID_VISITED),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    } else {
        if (humanPlayer)
            normalDialog(g_adventureEventText->getText(
                             ADV_EVENT_TEXT_MERMAID),
                         1, -1, -1, 11, 0, -1, 0, -1, 0, -1, 0);
        currentHero->m_flags |= 0x8000;
        g_game->setInfoFlag(MermaidInfo, g_netLocalGamePos);
        currentHero->m_luckBonus += 1;
    }
}

inline void advManager::doEventWhirlpool(hero* currentHero,
                                           NewmapCell* cell,
                                           unsigned char humanPlayer)
{
    type_point exitPoint;
    if (g_game->getRandomWhirlpool(cell->m_extraInfo, exitPoint)) {
        stopCursor(1);
        g_advManager->teleportTo(currentHero, exitPoint, 0, 0, 1, 0);
    }
}

// E:\gamedcs\events.cpp:4302.  The adventure map's object dispatcher: a
// 94-arm jump-table switch over cell->type, biased by 2 and indexed
// through a 214-entry byte table.  Fifty-six arms are a single call to a
// handler this file already carries; the rest are the handlers the retail
// branch folded into the dispatcher itself.  Case order IS layout order - the 0xd4/0xd5/
// 0xd7 retail additions sit between DRAGON_CITY and EYE_OF_MAGI, and the
// whirlpool between the two monolith arms - and the five EH states run
// 0..4 in exactly that order: sacrifice window, hill fort window, thieves
// guild new, university window, war factory new.
// [2026-08-28] SOURCE-SHAPE CHECKPOINT (99.37%): the earlier 99.99% source
// was a local maximum produced by pasting nine Dreamcast event-handler bodies
// and four game accessors directly into this switch. The fatal DC gate now
// requires all thirteen calls, plus NewfullMap::cell in the eye sweep and the
// AI-first university statement order. The inline definitions above and the
// Game.h views preserve those boundaries while VC6 folds them back into this
// retail row. Do not trade that proof away for the old score: recover the
// remaining register/inlining decisions inside the named hierarchy. Restoring
// SetInfoFlag's own attested GetTeam call raised the coherent checkpoint from
// 99.05% to 99.29%; inline_depth(1) blocked SetInfoFlag itself (98.36%),
// inline_depth(2) returned to 99.05%, and the unforced natural declaration
// plus call produced 99.29%. Naming gpAdvManager as the whirlpool receiver
// closed that expansion, and restoring the dossier's single direct obelisk
// update (no invented pointer/mask locals) raised the checkpoint to 99.37%.
// Border-tent operand order/force-inline and a named faerie-ring game receiver
// were byte-flat; force-inlining GetTeam regressed this row to 99.13% and its
// independently exact COMDAT to zero, so only its attested inline declaration
// is retained.
// With all four primary-skill handlers' DC-proven text subscripts restored,
// removing their four call pins still expands the handlers (90.77%, versus
// pinned 99.4678%). Text-wrapper flattening was real source debt, but not the
// cause of these four retained-call decisions.
// Garden-specific control: replacing its visited early return with an else
// leaves the retained helper at 100% and this dispatcher at 99.4678%. Removing
// only the Garden pin gives 97.8007% under either structure; four distinct
// reproduced objects rule out that branch spelling as the call-boundary fix.
VA(0x004a84f0, 0x2542)  // anchor-callee cell->type jump table + ret 0x10=p5 (note above), dc 0x9824c
void advManager::dispatchEvent(hero* currentHero, NewmapCell* cell, type_point point, bool humanPlayer)
{
    int eventType = cell->m_type;
    mobilizeCurrHero(1, 0, 0);

    switch (eventType) {
    case ALTAR_OF_SACRIFICE:
        if (humanPlayer) {
            g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
            g_mouseManager->showPointer(1);
            {
                type_sacrifice_window sacrificeWindow(
                    currentHero, g_game->getLocalPlayerGamePos());
                sacrificeWindow.centerWindow(-1, -1);
                sacrificeWindow.doModal(0);
                reseed(0, 0);
            }
            g_game->setInfoFlag(const_sacrifice_info, g_netLocalGamePos);
        }
        break;
    case ANCHOR_POINT:
        doEventAnchor(currentHero, humanPlayer);
        break;
    case ARENA:
        doEventArena(currentHero, cell, humanPlayer);
        break;
    case ARTIFACT:
        doEventArtifact(currentHero, cell, point, humanPlayer);
        break;
    case BLACK_BOX:
        doEventBlackBox(currentHero, cell, point, humanPlayer);
        break;
    case BLACK_MARKET:
        if (!humanPlayer)
            aiVisitBlackMarket(currentHero,
                                  &g_game->m_blackMarkets[cell->m_extraInfo]);
        else
            doBlackMarket(currentHero,
                          g_game->m_blackMarkets[cell->m_extraInfo].m_artifacts);
        break;
    case BOAT:
        doEventBoat(currentHero, cell);
        break;
    case BORDER_GUARD:
        doEventBorderGuard(point, cell, humanPlayer);
        break;
    case BORDER_TENT:
        doEventBorderTent(cell, humanPlayer);
        break;
    case BUOY:
        doEventBouy(currentHero, cell, humanPlayer);
        break;
    case CAMPFIRE:
        doEventCampfire(currentHero, cell, point, humanPlayer);
        break;
    case CARTOGRAPHER:
        if (humanPlayer) {
            if (g_game->m_cartographerFlags[cell->m_objectIndex]
                & g_curPlayerBit) {
                normalDialog((*g_adventureEventText)[
                                 ADV_EVENT_TEXT_CARTOGRAPHER_VISITED],
                             1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                break;
            }
            if (g_currentPlayer->m_resources[GOLD] < 1000) {
                normalDialog((*g_adventureEventText)[
                                 ADV_EVENT_TEXT_CARTOGRAPHER_NO_GOLD],
                             1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                break;
            }
            switch (cell->m_objectIndex) {
            case CARTOGRAPHER_WATER:
                normalDialog((*g_adventureEventText)[
                                 ADV_EVENT_TEXT_CARTOGRAPHER_WATER],
                             2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                break;
            case CARTOGRAPHER_LAND:
                normalDialog((*g_adventureEventText)[
                                 ADV_EVENT_TEXT_CARTOGRAPHER_LAND],
                             2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                break;
            case CARTOGRAPHER_UNDERGROUND:
                normalDialog((*g_adventureEventText)[
                                 ADV_EVENT_TEXT_CARTOGRAPHER_UNDERGROUND],
                             2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                break;
            }
            if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
                break;
            g_currentPlayer->m_resources[GOLD] += -1000;
            g_game->makeTerrainVisible(
                g_netLocalGamePos,
                g_game->m_cartographerMask[cell->m_objectIndex]);
            g_game->m_cartographerFlags[cell->m_objectIndex]
                |= g_curPlayerBit;
            completeDraw(false);
            updateScreen(0, 0);
        }
        break;
    case CLOVER_FIELD:
        doEventCloverField(currentHero, cell, humanPlayer);
        break;
    case COVER_OF_DARKNESS:
        doEventCoverOfDarkness(cell, point, humanPlayer);
        break;
    case CREATURE_BANK:
        doEventCreatureBank(currentHero, cell, point, humanPlayer);
        break;
    case CREATURE_GENERATOR_1:
    case CREATURE_GENERATOR_4:
        doEventCreatureGenerator(currentHero, cell, point, humanPlayer);
        break;
    case DEAD_GUY:
        doEventSkeleton(currentHero, cell, humanPlayer);
        break;
    case DEFENSE_TOWER:
#pragma inline_depth(0)
        doEventDefenseTower(currentHero, cell, humanPlayer);
#pragma inline_depth()
        break;
    case DERELICT_SHIP:
        doEventUndeadLair(currentHero, cell,
                             (*g_adventureEventText)[
                                 ADV_EVENT_TEXT_DERELICT_SHIP_PROMPT],
                             (*g_adventureEventText)[
                                 ADV_EVENT_TEXT_DERELICT_SHIP_EMPTY],
                             (*g_adventureEventText)[
                                 ADV_EVENT_TEXT_DERELICT_SHIP_TREASURE],
                             0x800, point);
        break;
    case DRAGON_CITY:
        doEventDragonCity(currentHero, cell, point, humanPlayer);
        break;
    case BORDER_GATE:
        doEventBorderGate(cell, humanPlayer);
        break;
    case FREELANCERS_GUILD:
        if (humanPlayer)
            doFreelancersGuild(currentHero);
        break;
    case QUEST_GUARD:
        m_fullMap->m_questGuardList[cell->m_extraInfo].doEvent(
            currentHero, humanPlayer, cell, point);
        break;
    case EYE_OF_MAGI:
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[
                             ADV_EVENT_TEXT_EYE_OF_MAGI],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        break;
    case FAERIE_RING:
        doEventFaerieRing(currentHero, cell, humanPlayer);
        break;
    case FLOTSAM:
        doEventFlotsam(currentHero, cell, point, humanPlayer);
        break;
    case FOUNTAIN_OF_FORTUNE:
        doEventFountain(currentHero, cell, humanPlayer);
        break;
    case FOUNTAIN_OF_YOUTH:
        doEventFountainOfYouth(currentHero, cell, humanPlayer);
        break;
    case GARDEN_OF_REVELATION:
#pragma inline_depth(0)
        doEventGarden(currentHero, cell, humanPlayer);
#pragma inline_depth()
        break;
    case GARRISON: {
        g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
        g_mouseManager->showPointer(1);
        garrison* thisGarrison = g_game->getGarrison(cell->m_extraInfo);
        if (!g_game->onSameTeam(thisGarrison->m_playerOwner,
                                g_netLocalGamePos)) {
            if (thisGarrison->m_garrisonArmy.hasCreatures()) {
                if (doCombat(point, currentHero, &currentHero->m_army,
                             thisGarrison->m_playerOwner, 0, 0,
                             &thisGarrison->m_garrisonArmy, -1, 1, 0))
                    break;
            }
            g_game->claimGarrison(cell->m_extraInfo, g_netLocalGamePos);
            currentHero->checkLevel();
        }
        if (!humanPlayer)
            aiEnterGarrison(currentHero, thisGarrison);
        else
            doEventGarrison(currentHero, thisGarrison);
        break;
    }
    case HERO:
        doEventHero(currentHero, cell, point, humanPlayer);
        break;
    case HILL_FORT:
        if (!humanPlayer) {
            aiVisitHillFort(currentHero);
        } else {
            g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
            g_mouseManager->showPointer(1);
            THillFortWindow hillFortWindow;
            hillFortWindow.centerWindow(-1, -1);
            hillFortWindow.doModal();
        }
        g_game->setInfoFlag(HillFortInfo, g_netLocalGamePos);
        break;
    case HUT_OF_MAGI: {
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[
                             ADV_EVENT_TEXT_HUT_OF_MAGI],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_remoteOn && g_dPlay
            && g_netLocalGamePos == g_game->getLocalPlayerGamePos()) {
            CNetMsgHandler* handler = g_dPlay->getNetMsgHandler();
            if (handler)
                handler->setInPopup(0);
        }
        type_point savedOrigin = m_radarOrigin;
        demobilizeCurrHero(0, 1);
        for (int z = 0; z < g_game->getNumMapLevels(); z++) {
            for (int x = 0; x < g_mapWidth; x++) {
                for (int y = 0; y < g_mapHeight; y++) {
                    NewmapCell* eyeCell =
                        g_game->m_worldMap.cell(x, y, z);
                    if (eyeCell->m_type == EYE_OF_MAGI
                        && eyeCell->m_isTrigger) {
                        g_game->setVisibility(x, y, z, g_netLocalGamePos,
                                              10, 0);
                        if (humanPlayer) {
                            m_radarOrigin.m_x = x - 9;
                            m_radarOrigin.m_y = y - 8;
                            m_radarOrigin.m_z = z;
                            completeDraw(false);
                            updateRadar(1, 1, 0, 0, 0);
                            updateScreen(0, 0);
                            GameTime::delayTil(GameTime::get() + 2000);
                        }
                    }
                }
            }
        }
        m_radarOrigin = savedOrigin;
        if (humanPlayer) {
            completeDraw(false);
            updateScreen(0, 0);
        }
        break;
    }
    case IDOL_OF_FORTUNE:
        doEventIdol(currentHero, cell, humanPlayer);
        break;
    case LEAN_TO:
        doEventLeanTo(currentHero, cell, humanPlayer);
        break;
    case LIBRARY:
        doEventLibrary(currentHero, cell, humanPlayer);
        break;
    case LIGHTHOUSE:
        doEventLighthouse(cell, humanPlayer);
        break;
    case LITH_ONEWAY_ENTRANCE:
        doEventLithOneWay(currentHero, cell, humanPlayer);
        break;
    case LITH_ONEWAY_EXIT:
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[
                             ADV_EVENT_TEXT_LITH_ONEWAY_EXIT_BLOCKED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        break;
    case LITH_TWOWAY:
        doEventLithTwoWay(currentHero, cell, humanPlayer);
        break;
    case WHIRLPOOL:
        doWhirlpool(currentHero);
        doEventWhirlpool(currentHero, cell, humanPlayer);
        break;
    case MAGIC_SCHOOL:
        doEventMagicSchool(currentHero, cell, point, humanPlayer);
        break;
    case MAGIC_SPRING:
        doEventMagicSpring(currentHero, cell, humanPlayer);
        break;
    case MAGIC_WELL:
        doEventMagicWell(currentHero, cell, humanPlayer);
        break;
    case MERC_CAMP:
#pragma inline_depth(0)
        doEventMercenaryCamp(currentHero, cell, humanPlayer);
#pragma inline_depth()
        break;
    case MERMAID:
        doEventMermaid(currentHero, cell, humanPlayer);
        break;
    case MINE:
        doEventMine(cell, currentHero, point, humanPlayer);
        break;
    case MONSTER:
        doEventWanderingMonster(cell, currentHero, point, humanPlayer);
        break;
    case MYSTICAL_GARDEN:
        doEventMysticalGarden(currentHero, cell, humanPlayer);
        break;
    case OASIS:
        doEventOasis(currentHero, cell, humanPlayer);
        break;
    case OBELISK:
        if (!(g_game->m_obeliskFlags[cell->m_extraInfo] & g_curPlayerBit)) {
            g_game->m_obeliskFlags[cell->m_extraInfo]
                |= g_game->getTeamMask(g_netLocalGamePos);
            if (humanPlayer) {
                normalDialog((*g_adventureEventText)[
                                 ADV_EVENT_TEXT_OBELISK],
                             1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                viewPuzzle();
            } else {
                computeUALoc(currentHero->m_owner);
            }
        } else if (humanPlayer) {
            normalDialog((*g_adventureEventText)[
                             ADV_EVENT_TEXT_OBELISK_VISITED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
        break;
    case OBSERVATORY:
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[
                             ADV_EVENT_TEXT_OBSERVATORY],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_remoteOn) {
            CSetVisibilityMsg message(point, g_netLocalGamePos, 20);
            transmitRemoteData(&message, 0x7f, 0, 1);
        }
        g_game->setVisibility(point.m_x, point.m_y, point.m_z,
                              g_netLocalGamePos, 20, 0);
        if (humanPlayer) {
            completeDraw(false);
            updateScreen(0, 0);
        }
        break;
    case PILLAR_OF_FIRE:
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[
                             ADV_EVENT_TEXT_PILLAR_OF_FIRE],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_remoteOn) {
            CSetVisibilityMsg message(point, g_netLocalGamePos, 20);
            transmitRemoteData(&message, 0x7f, 0, 1);
        }
        g_game->setVisibility(point.m_x, point.m_y, point.m_z,
                              g_netLocalGamePos, 20, 0);
        if (humanPlayer) {
            completeDraw(false);
            updateScreen(0, 0);
        }
        break;
    case OCEAN_BOTTLE: {
        if (!humanPlayer)
            break;
        int signIndex = cell->m_extraInfo;
        if (signIndex == -1)
            break;
        if (g_game->m_signs[signIndex].m_hasText) {
            normalDialog(g_game->m_signs[signIndex].m_signText.c_str(),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        } else {
            normalDialog(g_randomSignText[point.m_x % 25],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
        eraseAndFizzle(cell, point, FIZZLE_SOUND_PICKUP);
        break;
    }
    case POWER_SCHOOL:
#pragma inline_depth(0)
        doEventPowerSchool(currentHero, cell, humanPlayer);
#pragma inline_depth()
        break;
    case PRISON:
        doEventPrison(currentHero, cell, point, humanPlayer);
        break;
    case PYRAMID:
        doEventPyramid(currentHero, cell, point, humanPlayer);
        break;
    case RALLY_FLAG:
        doEventRallyFlag(currentHero, cell, humanPlayer);
        break;
    case REFUGEE_CAMP:
        doEventRefugeeCamp(currentHero, cell, humanPlayer);
        break;
    case RESOURCE:
        doEventResource(cell, currentHero, point, humanPlayer);
        break;
    case SANCTUARY:
        if (humanPlayer)
            normalDialog((*g_adventureEventText)[
                             ADV_EVENT_TEXT_SANCTUARY],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        break;
    case SCHOLAR:
        doEventScholar(currentHero, cell, point, humanPlayer);
        break;
    case SEA_CHEST:
        doEventSeaChest(currentHero, cell, point, humanPlayer);
        break;
    case SEER:
        m_fullMap->m_seerHutList[cell->m_extraInfo].doSeerEvent(currentHero,
                                                          humanPlayer);
        break;
    case SEPULCHER:
        doEventUndeadLair(currentHero, cell,
                             (*g_adventureEventText)[
                                 ADV_EVENT_TEXT_SEPULCHER_PROMPT],
                             (*g_adventureEventText)[
                                 ADV_EVENT_TEXT_SEPULCHER_EMPTY],
                             (*g_adventureEventText)[
                                 ADV_EVENT_TEXT_SEPULCHER_TREASURE],
                             0x400, point);
        break;
    case SHIPWRECK:
        doEventUndeadLair(currentHero, cell,
                             (*g_adventureEventText)[
                                 ADV_EVENT_TEXT_SHIPWRECK_PROMPT],
                             (*g_adventureEventText)[
                                 ADV_EVENT_TEXT_SHIPWRECK_EMPTY],
                             (*g_adventureEventText)[
                                 ADV_EVENT_TEXT_SHIPWRECK_TREASURE],
                             0x200, point);
        break;
    case SHIPWRECK_SURVIVOR:
        doEventSurvivor(currentHero, cell, point, humanPlayer);
        break;
    case SHIPYARD:
        g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
        g_mouseManager->showPointer(1);
        doEventShipyard(cell, point, humanPlayer);
        break;
    case SHRINE1:
        doEventShrine(currentHero, cell,
                      (*g_adventureEventText)[ADV_EVENT_TEXT_SHRINE1_PREFIX],
                      Shrine1Info, humanPlayer);
        break;
    case SHRINE2:
        doEventShrine(currentHero, cell,
                      (*g_adventureEventText)[ADV_EVENT_TEXT_SHRINE2_PREFIX],
                      Shrine2Info, humanPlayer);
        break;
    case SHRINE3:
        doEventShrine(currentHero, cell,
                      (*g_adventureEventText)[ADV_EVENT_TEXT_SHRINE3_PREFIX],
                      Shrine3Info, humanPlayer);
        break;
    case SIGN: {
        if (!humanPlayer)
            break;
        int signIndex = cell->m_extraInfo;
        if (signIndex == -1)
            break;
        if (g_game->m_signs[signIndex].m_hasText) {
            normalDialog(g_game->m_signs[signIndex].m_signText.c_str(),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        } else {
            normalDialog(g_randomSignText[point.m_x % 25],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
        break;
    }
    case SIREN:
        doEventSiren(currentHero, cell, humanPlayer);
        break;
    case SPELL_SCROLL:
        doEventSpellScroll(currentHero, cell, point, humanPlayer);
        break;
    case STABLES:
        doEventStables(currentHero, cell, humanPlayer);
        break;
    case TAVERN: {
        if (!humanPlayer)
            break;
        type_point heroPos = currentHero->getLocation();
        if (heroPos.m_x != point.m_x || heroPos.m_y != point.m_y
            || heroPos.m_z != point.m_z) {
            g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
            g_mouseManager->showPointer(1);
            doMapTavern(point);
            m_advWindow->updateHeroLocators(-1, 1, 1);
        }
        break;
    }
    case TEMPLE:
        doEventTemple(currentHero, cell, humanPlayer);
        break;
    case THIEVES_DEN: {
        if (!humanPlayer)
            break;
        g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
        g_mouseManager->showPointer(1);
        normalDialog((*g_adventureEventText)[
                         ADV_EVENT_TEXT_THIEVES_DEN],
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        TThievesGuildWindow* guildWindow =
            new TThievesGuildWindow(99);
        if (!guildWindow)
            memError();
        guildWindow->doModal(0);
        if (guildWindow)
            delete guildWindow;
        redrawAdvScreen(1, 0);
        break;
    }
    case TOWN:
        g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
        g_mouseManager->showPointer(1);
        townEvent(cell, point, humanPlayer);
        break;
    case TRADING_POST:
        if (humanPlayer)
            doTradingPost();
        break;
    case TRAINING_GROUNDS:
        doEventTrainingGrounds(currentHero, cell, humanPlayer);
        break;
    case TREASURE_CHEST:
        doEventTreasure(currentHero, cell, point, humanPlayer);
        break;
    case TREE_OF_KNOWLEDGE:
        doEventTreeOfKnowledge(currentHero, cell,
                               humanPlayer);
        break;
    case UNDERGROUND_GATE: {
        type_point exitPoint = g_game->getUndergroundGateExit(cell);
        const int noGateExit = 0xff;
        if (exitPoint.m_x == noGateExit) {
            normalDialog((*g_adventureEventText)[
                             ADV_EVENT_TEXT_SUBTERRANEAN_BLOCKED],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            break;
        }
        NewmapCell* exitCell = g_game->getCell(exitPoint);
        if (exitCell->m_type == HERO) {
            if (g_remoteOn) {
                CSetVisibilityMsg message(exitPoint, g_netLocalGamePos, 1);
                transmitRemoteData(&message, 0x7f, 0, 1);
            }
            g_game->setVisibility(exitPoint.m_x, exitPoint.m_y, exitPoint.m_z,
                                  g_netLocalGamePos, 1, 0);
            doEventHero(currentHero, exitCell, exitPoint, humanPlayer);
            if (currentHero->m_owner < 0)
                break;
        }
        if (exitCell->m_type == UNDERGROUND_GATE)
            g_advManager->teleportTo(currentHero, exitPoint, 0, 0, 1, 0);
        break;
    }
    case UNIVERSITY:
        if (!humanPlayer || g_goSolo) {
            aiVisitUniversity(currentHero,
                                cell->getUniversity());
        } else {
            g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
            g_mouseManager->showPointer(1);
            {
                type_university_window universityWindow(
                    currentHero, cell->getUniversity(), 0);
                universityWindow.centerWindow(-1, -1);
                universityWindow.doModal(0);
            }
            g_game->setInfoFlag(UniversityInfo, g_netLocalGamePos);
        }
        break;
    case WAGON:
        doEventWagon(currentHero, cell, humanPlayer);
        break;
    case WAR_MACHINE_FACTORY:
        if (!humanPlayer) {
            aiVisitWarFactory(currentHero);
        } else {
            normalDialog((*g_adventureEventText)[
                             ADV_EVENT_TEXT_WAR_MACHINE_FACTORY_PROMPT],
                         2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
                break;
            short quantity = 4000;
            recruitUnit* unitRecruiter = new recruitUnit(
                currentHero, CREATURE_BALLISTA, &quantity,
                CREATURE_FIRST_AID_TENT, &quantity,
                CREATURE_AMMO_CART, &quantity, CREATURE_NONE, 0);
            if (!unitRecruiter)
                memError();
            g_executive->doDialog(unitRecruiter);
            delete unitRecruiter;
        }
        break;
    case WAR_SCHOOL:
        doEventWarSchool(currentHero, cell, humanPlayer);
        break;
    case WARRIOR_TOMB:
        doEventWarriorTomb(currentHero, cell, humanPlayer);
        break;
    case WATER_WHEEL:
        doEventWaterWheel(currentHero, cell, humanPlayer);
        break;
    case WATERING_HOLE:
        doEventWateringHole(currentHero, cell, humanPlayer);
        break;
    case WINDMILL:
        doEventWindmill(currentHero, cell, humanPlayer);
        break;
    case WITCH_HUT:
        doEventWitchHut(currentHero, cell, humanPlayer);
        break;
    }
}

VA_COMPGEN(0x004aaa40, 0x5C, IMPLICIT_DTOR, type_university_window)  // dc 0x9ce08

VA(0x004aaaa0, 0x110)  // dc 0x99abc
void advManager::doEvent(NewmapCell* eventCell, type_point point)
{
    hero* currentHero = g_game->getCurrHero();

    reseed(0, 0);
    eventSound(eventCell);
    dispatchEvent(currentHero, eventCell, point, 1);

    updateRadar(1, 1, 0, 0, 0);
    m_advWindow->updateHeroLocators(-1, 1, 0);
    m_advWindow->updateTownLocators(-1, 1, 0);
    m_advWindow->updateQuestLogButton(1);
    m_advWindow->updateSpellButton(currentHero);
    m_advWindow->updateSleepButton(currentHero);
    updBottomView(1, 1, 1);
    m_advWindow->updateResourceDisplay(1, 1);

    if (!g_soundManager->m_mp3Playing)
        g_soundManager->resumeStream();
    if (g_game->m_mapHeader.m_victoryCondition.checkForTotalCreatures())
        checkEndGame(0);
    if (g_game->m_mapHeader.m_victoryCondition.checkForTotalResources())
        checkEndGame(0);
}

// The double loop walks the object's bounding box from its anchor
// BACKWARDS (the anchor is the bottom-right cell), and every cell that
// falls off the map is skipped rather than clamped. Inside, the cell's
// own four-byte object list is searched for this object's index and the
// hit is spliced out with Dinkumware vector::erase - the copy-down loop
// and the `--_Last` are both inlined, and the element is POD so _Destroy
// vanishes.

// The 0x18-byte CMCEraseObject goes out through SendMapChange, and the
// final SetEnvironmentOrigin re-centres on the radar origin offset by the
// adventure viewport's own (9, 8) - the same pair type_cell_adjuster
// carries as MOBILE_HERO_CELL_X/Y.

VA(0x004aabb0, 0x239)  // dc 0x99bac
void advManager::eraseObj(NewmapCell* thisCell, type_point point,
                          unsigned char record)
{
    int objectIndex = thisCell->m_objectTypeIndex;
    if (objectIndex == -1)
        return;
    if (objectIndex < 0)
        return;
    if (objectIndex >= m_fullMap->m_objects.size())
        return;

    CObject* object = &m_fullMap->m_objects[objectIndex];
    CObjectType* objectType = &m_fullMap->m_objectTypes[object->m_typeIndex];

    if (record)
        g_game->recordEraseObject(thisCell, point);

    for (int iy = 0; iy < objectType->m_height; iy++) {
        if (object->m_y - iy < 0 || object->m_y - iy >= g_mapHeight)
            continue;
        for (int ix = 0; ix < objectType->m_width; ix++) {
            if (object->m_x - ix < 0 || object->m_x - ix >= g_mapWidth)
                continue;
            NewmapCell* cell = m_fullMap->cell(object->m_x - ix,
                                             object->m_y - iy, object->m_z);
            for (NewmapCell::TObjectCell* entry = cell->m_objects.begin();
                 entry != cell->m_objects.end(); entry++) {
                if (entry->m_objectIndex == objectIndex) {
                    cell->m_objects.erase(entry);
                    break;
                }
            }
            m_fullMap->calculateCellExtra(cell, 0);
        }
    }

    thisCell->m_extraInfo = -1;
    g_game->setupAdjacentMons();

    CMCEraseObject message(point);
    sendMapChange(&message);

    setEnvironmentOrigin(getMapCenter(), 1);
}

VA(0x004aadf0, 0x1DC)  // dc 0x99d98
void advManager::heroSwap(hero* leftHero, hero* rightHero)
{
    swapManager* manager = new swapManager(leftHero, rightHero);
    if (!manager)
        memError();

    if (g_remoteOn
        && g_currentPlayer->isLocalHuman()
        && g_game->isHuman(rightHero->m_owner)
        && rightHero->m_owner != leftHero->m_owner)
    {
        CTradeRequestMsg message(leftHero, rightHero);
        transmitRemoteData(&message, rightHero->m_owner, 1, 1);
    }

    g_executive->doDialog(manager);
    delete manager;
    redrawAdvScreen(1, 0);
}

VA(0x004aafd0, 0x431)  // dc 0x99eb0
void advManager::townEvent(NewmapCell* cell, type_point point,
                           unsigned char humanPlayer)
{
    town* thisTown = g_game->getTown(
        g_game->getTownId(point.m_x, point.m_y, point.m_z));
    hero* currentHero = g_game->getCurrHero();
    int oldOwner = thisTown->m_owner;

    demobilizeCurrHero(0, 1);

    if (g_game->onSameTeam(thisTown->m_owner, g_netLocalGamePos)) {
        if (thisTown->m_owner == g_netLocalGamePos
                && g_game->m_mapHeader.m_victoryCondition
                       .checkForArtifactTransportWin(currentHero, point))
        {
            checkEndGame(0);
            return;
        }
        if (humanPlayer)
            thisTown->view(0);
        else
            aiEnterTown(currentHero, thisTown);
    } else if (!thisTown->getArmy().hasCreatures()
               && (thisTown->m_visitingHeroId < 0
                   || thisTown->m_visitingHeroId == currentHero->m_id)) {
        if (thisTown->m_garrisonHeroId > -1) {
            heroLoses(&g_game->m_heroes[thisTown->m_garrisonHeroId], -1);
            thisTown->m_garrisonHeroId = -1;
        }
        g_game->claimTown(thisTown->m_id, g_netLocalGamePos, 0, 1);
        thisTown->destroyExtraCapitol();
        g_game->m_mapHeader.m_lossCondition.checkForDefeatedTownLoss(oldOwner,
                                                                 thisTown);
        checkEndGame(0);
        if (g_gameOver)
            return;
        updateRadar(1, 1, 0, 0, 0);
        m_advWindow->updateHeroLocators(-1, 1, 1);
        m_advWindow->updateTownLocators(-1, 1, 1);
        if (g_game->m_mapHeader.m_victoryCondition
                .checkForArtifactTransportWin(currentHero, point))
        {
            checkEndGame(0);
            return;
        }
        if (humanPlayer)
            thisTown->view(0);
        else
            aiEnterTown(currentHero, thisTown);
    } else {
        hero* defender;
        armyGroup* defendingArmy;

        if (thisTown->m_visitingHeroId >= 0) {
            defender = g_game->getHero(thisTown->m_visitingHeroId);
            if (thisTown->m_garrisonHeroId >= 0) {
                doCombat(point, currentHero, &currentHero->m_army,
                         defender->m_owner, 0, defender, &defender->m_army,
                         -1, 1, 0);
                currentHero->checkLevel();
                checkEndGame(0);
                return;
            }
            defender->m_army.mergeArmies(
                *const_cast<armyGroup*>(&thisTown->getArmy()));
            defendingArmy = &defender->m_army;
        } else {
            if (thisTown->m_garrisonHeroId < 0)
                defender = 0;
            else
                defender = g_game->getHero(thisTown->m_garrisonHeroId);
            defendingArmy = const_cast<armyGroup*>(&thisTown->getArmy());
        }

        if (doCombat(point, currentHero, &currentHero->m_army,
                     thisTown->m_owner, thisTown, defender, defendingArmy,
                     -1, 1, 0) == 0) {
            thisTown->m_garrisonHeroId = -1;
            g_game->claimTown(thisTown->m_id, g_netLocalGamePos, 0, 1);
            thisTown->destroyExtraCapitol();
            if (g_game->m_mapHeader.m_lossCondition.checkForDefeatedTownLoss(
                    oldOwner, thisTown))
                checkEndGame(0);
            if (g_gameOver)
                return;
            updateRadar(1, 1, 0, 0, 0);
            m_advWindow->updateHeroLocators(-1, 1, 1);
            m_advWindow->updateTownLocators(-1, 1, 1);
        }
    }

    thisTown->giveSpells(0);
    currentHero->checkLevel();
    g_game->m_mapHeader.m_victoryCondition.checkForTotalCreatures();
    g_game->m_mapHeader.m_victoryCondition.checkForTotalResources();
    checkEndGame(0);
}

VA(0x004ab410, 0x632)  // dc 0x9a288
void advManager::eventSound(int eventID, int extraInfo)
{
    std::string sampleName;

    switch (eventID) {
    case MINE:
        if (g_game->getMine(extraInfo)->m_guards.hasCreatures())
            sampleName = DATA_COMPGEN(0x00677898, mineGuardSampleName,
                                      "mystery.wav");
        else
            sampleName = DATA_COMPGEN(0x00677888, flagMineSampleName,
                                      "flagmine.wav");
        break;
    case FLOTSAM:
    case LEAN_TO:
    case WAGON:
    case WATER_WHEEL:
    case WINDMILL:
        sampleName = DATA_COMPGEN(0x0067787c, genieSampleName, "Genie.wav");
        break;
    case SEPULCHER:
    case WARRIOR_TOMB:
        sampleName = DATA_COMPGEN(0x0067786c, graveyardSampleName, "Graveyard.wav");
        break;
    case ALTAR_OF_SACRIFICE:
    case BLACK_BOX:
    case BLACK_MARKET:
    case DEAD_GUY:
    case OBELISK:
    case PYRAMID:
    case THIEVES_DEN:
        sampleName = DATA_COMPGEN(0x00677860, mysterySampleName, "Mystery.wav");
        break;
    case CREATURE_BANK:
    case DERELICT_SHIP:
    case PRISON:
    case SHIPWRECK:
        sampleName = DATA_COMPGEN(0x00677854, rogueSampleName, "Rogue.wav");
        break;
    case BORDER_GUARD:
    case BORDER_TENT:
    case UNDERGROUND_GATE:
    case BORDER_GATE:
    case QUEST_GUARD:
        sampleName = DATA_COMPGEN(0x00677844, caveHeadSampleName, "Cavehead.wav");
        break;
    case CARTOGRAPHER:
    case COVER_OF_DARKNESS:
    case EYE_OF_MAGI:
    case HUT_OF_MAGI:
    case LIGHTHOUSE:
    case OBSERVATORY:
    case PILLAR_OF_FIRE:
        sampleName = DATA_COMPGEN(0x00677834, lighthouseSampleName, "Lighthouse.wav");
        break;
    case SEA_CHEST:
    case TREASURE_CHEST:
        sampleName = DATA_COMPGEN(0x00677828, chestSampleName, "Chest.wav");
        break;
    case CAMPFIRE:
    case IDOL_OF_FORTUNE:
    case MYSTICAL_GARDEN:
        sampleName = DATA_COMPGEN(0x00677818, experienceSampleName, "Expernce.wav");
        break;
    case CREATURE_GENERATOR_1:
    case CREATURE_GENERATOR_4:
    case GARRISON:
    case HILL_FORT:
    case REFUGEE_CAMP:
    case WAR_SCHOOL:
        sampleName = DATA_COMPGEN(0x00677808, militarySampleName, "Military.wav");
        break;
    case ARENA:
    case DEFENSE_TOWER:
    case MERC_CAMP:
        sampleName = DATA_COMPGEN(0x006777fc, nomadSampleName, "Nomad.wav");
        break;
    case ARTIFACT:
    case SHIPWRECK_SURVIVOR:
        sampleName = DATA_COMPGEN(0x006777ec, treasureSampleName, "Treasure.wav");
        break;
    case MAGIC_SCHOOL:
    case MAGIC_SPRING:
    case MAGIC_WELL:
        sampleName = DATA_COMPGEN(0x006777e0, faerieSampleName, "Faerie.wav");
        break;
    case LIBRARY:
    case POWER_SCHOOL:
    case SCHOLAR:
    case TRAINING_GROUNDS:
    case TREE_OF_KNOWLEDGE:
    case UNIVERSITY:
    case WITCH_HUT:
        sampleName = DATA_COMPGEN(0x006777d4, gazeboSampleName, "Gazebo.wav");
        break;
    case BUOY:
    case FOUNTAIN_OF_YOUTH:
    case OASIS:
    case RALLY_FLAG:
    case WATERING_HOLE:
        sampleName = DATA_COMPGEN(0x006777c8, moraleSampleName, "Morale.wav");
        break;
    case CLOVER_FIELD:
    case FAERIE_RING:
    case FOUNTAIN_OF_FORTUNE:
    case MERMAID:
        sampleName = DATA_COMPGEN(0x006777bc, luckSampleName, "Luck.wav");
        break;
    case GARDEN_OF_REVELATION:
    case SANCTUARY:
        sampleName = DATA_COMPGEN(0x006777a8, getProtectionSampleName, "GetProtection.wav");
        break;
    case SHRINE1:
    case SHRINE2:
    case SHRINE3:
    case TEMPLE:
        sampleName = DATA_COMPGEN(0x0067779c, templeSampleName, "Temple.wav");
        break;
    case OCEAN_BOTTLE:
    case SHIPYARD:
    case SIGN:
    case STABLES:
    case TAVERN:
    case TRADING_POST:
    case WAR_MACHINE_FACTORY:
        sampleName = DATA_COMPGEN(0x00677790, storeSampleName, "Store.wav");
        break;
    case DRAGON_CITY:
        sampleName = DATA_COMPGEN(0x00677784, dragonSampleName, "Dragon.wav");
        break;
    case SEER:
        sampleName = DATA_COMPGEN(0x00677778, questSampleName, "Quest.wav");
        break;
    case SIREN:
    case WHIRLPOOL:
        sampleName = DATA_COMPGEN(0x0067776c, dangerSampleName, "Danger.wav");
        break;
    }

    if (sampleName.size())
        launchSample(sampleName.c_str(), -1, 3);
}

// Original: advManager::RecruitEvent; events.cpp:5568, dc 0x9a528.
// The ordinary helper is expanded into Complete's refugee-camp handler.
short advManager::recruitEvent(hero* who, TCreatureType creature, short available)
{
    if (who->belongsToHuman()) {
        recruitUnit dialog(&who->m_army, 0, creature, &available,
                           CREATURE_NONE, 0, CREATURE_NONE, 0,
                           CREATURE_NONE, 0);
        g_executive->doDialog(&dialog);
    } else {
        aiRecruitRefugees(who, creature, &available);
    }
    return available;
}

VA(0x004aba50, 0x361)  // dc 0x9a5b0
void advManager::generatorEvent(hero* who, NewmapCell* eventCell, type_point point)
{
    int id = g_game->getGeneratorId(point.m_x, point.m_y, point.m_z);
    generator& currentGenerator = g_game->m_generators[id];

    if (currentGenerator.getOwner() == g_netLocalGamePos) {
        bool canRecruit = false;
        std::string result;

        for (int i = 0; i < 4; i++) {
            TCreatureType creature = currentGenerator.m_type[i];
            if (creature == CREATURE_NONE)
                continue;

            if (g_creatureTypeTraits[creature].m_level != 0) {
                canRecruit = true;
                continue;
            }

            if (currentGenerator.m_population[i] == 0) {
                result += formatString((*g_generalText)[GENERAL_TEXT_NO_CREATURES_TO_RECRUIT_FORMAT],
                    g_creatureTypeTraits[creature].m_pluralName);
            } else if (!who->m_army.add(creature,
                                      currentGenerator.m_population[i], -1)) {
                result += formatString((*g_generalText)[GENERAL_TEXT_RECRUIT_INSUFFICIENT_PROVISIONS_FORMAT],
                    getArmyName(creature, currentGenerator.m_population[i]));
            } else {
                currentGenerator.m_population[i] = 0;
            }
        }

        if (result.length() > 0) {
            normalDialog(result.c_str(), 1, -1, -1, -1, 0,
                         -1, 0, -1, 0, -1, 0);
        }

        if (!canRecruit)
            return;
    }

    recruitUnit* manager = new recruitUnit(&who->m_army, 0,
        currentGenerator.m_type[0], &currentGenerator.m_population[0],
        currentGenerator.m_type[1], &currentGenerator.m_population[1],
        currentGenerator.m_type[2], &currentGenerator.m_population[2],
        currentGenerator.m_type[3], &currentGenerator.m_population[3]);
    if (!manager)
        memError();

    manager->m_viewOnly = currentGenerator.getOwner() != g_netLocalGamePos
                         && currentGenerator.getOwner() >= 0;
    g_executive->doDialog(manager);
    delete manager;
}

// townmgr.obj's creature-type join report (0x5d11d0, reconstructed there);
// the creature-bank joiner arm is its only consumer here. Declared
// file-locally, the armyGroup-overload precedent above.
void doMonsterJoinDialog(hero* inHero, TCreatureType type, int amount);

// E:\gamedcs\events.cpp:5661
VA(0x004abdc0, 0x6D0)  // anchor-callee ExtraInfoUnion::get_creature_bank, ret 0x14=p6, dc 0x9a898
int advManager::creatureBankEvent(hero* who, NewmapCell* cell, const char* text, type_point point, unsigned char humanPlayer)
{
    type_creature_bank& bank = cell->getCreatureBank();
    TCreatureType leaderMonster = CREATURE_NONE;
    long creatureCount = bank.m_guards.getCreatureTotal();
    if (humanPlayer) {
        int best = 0;
        for (int i = 0; i < 7; i++) {
            int type = bank.m_guards.m_armies[i];
            if (type != CREATURE_NONE
                && g_creatureTypeTraits[type].m_aiValue > best) {
                best = g_creatureTypeTraits[type].m_aiValue;
                leaderMonster = TCreatureType(type);
            }
        }
    }

    demobilizeCurrHero(0, 1);
    int seed = point.m_y * 0x36814 + point.m_z * 0x3d0b5 + point.m_x * 0x29875
               + 0x14075;
    sRand(seed);
    int winner = doCombat(point, who, &who->m_army, -1, 0, 0, &bank.m_guards,
                          seed, 1, 1);
    mobilizeCurrHero(0, 0, 1);
    if (winner)
        return 0;

    if (humanPlayer) {
        std::vector<type_dialog_resource> resources;
        std::vector<std::string> rewardStrings;
        std::string result;
        type_dialog_resource resource;

        if (bank.m_rewardCreatures > 0) {
            resource.m_resource = 0x15;
            resource.m_qualifier = bank.m_rewardCreature;
            resources.push_back(resource);
            result = formatString(
                DATA_COMPGEN(0x006778a4, resourceQuantityFormat, "%d %s"),
                bank.m_rewardCreatures,
                getArmyName(bank.m_rewardCreature, bank.m_rewardCreatures));
            rewardStrings.push_back(result);
        }

        unsigned int i;
        for (i = 0; i < bank.m_artifacts.size(); i++) {
            resource.m_resource = 8;
            resource.m_qualifier = bank.m_artifacts[i];
            resources.push_back(resource);
            result = formatString(
                (*g_adventureEventText)[ADV_EVENT_TEXT_ARTIFACT_NAME_WITH_ARTICLE_FORMAT],
                g_artifactTraits[bank.m_artifacts[i]].m_name);
            rewardStrings.push_back(result);
        }

        for (int j = 0; j <= 6; j++) {
            if (bank.m_resources[j] > 0) {
                resource.m_resource = j;
                resource.m_qualifier = bank.m_resources[j];
                if (j == GOLD && bank.m_artifacts.size() != 0)
                    resource.m_resource = 0x24;
                resources.push_back(resource);
                result = formatString(
                    DATA_COMPGEN(0x006778a4, resourceQuantityFormat,
                                 "%d %s"),
                    bank.m_resources[j], g_resourceNames[j]);
                rewardStrings.push_back(result);
            }
        }

        result = rewardStrings[0];
        for (i = 1; i < rewardStrings.size(); i++) {
            if (i == rewardStrings.size() - 1)
                result += (*g_generalText)[GENERAL_TEXT_LIST_AND];
            else
                result += ", ";
            result += rewardStrings[i];
        }

        // The initialiser form, not default-construct-then-assign: retail
        // builds the reward line directly into its destination (inlined
        // `_Tidy`, called `assign`), which is worth 89.3406 -> 91.5867.
        std::string rewardText = formatString(
            (*g_adventureEventText)[ADV_EVENT_TEXT_CREATURE_BANK_REWARD_FORMAT],
            getArmyName(leaderMonster, creatureCount), result.c_str());
        extendedDialog(rewardText.c_str(), resources, -1, -1, 0);
    }

    if (bank.m_rewardCreatures > 0) {
        if (!who->m_army.add(bank.m_rewardCreature, bank.m_rewardCreatures,
                           -1)) {
            if (humanPlayer)
                doMonsterJoinDialog(
                    who, bank.m_rewardCreature,
                    bank.m_rewardCreatures);
            else
                aiJoinDecision(who,
                                 bank.m_rewardCreature,
                                 bank.m_rewardCreatures);
        }
    }

    unsigned int k;
    for (k = 0; k < bank.m_artifacts.size(); k++) {
        type_artifact art;
        art.m_artifactId = bank.m_artifacts[k];
        art.m_extra = -1;
        who->giveArtifact(&art, 1, 1);
    }

    for (int m = 0; m <= 6; m++)
        who->giveResource(m, bank.m_resources[m]);

    cell->m_extraInfo |= 0x2000000;
    who->checkLevel();
    return 1;
}

VA(0x004ac490, 0xEE)  // dc 0x9adcc
void advManager::doEventUndeadLair(hero* currentHero, NewmapCell* cell, const char* questionText, const char* emptyText, const char* rewardText, unsigned long visitedFlag, type_point point)
{
    unsigned char humanPlayer = currentHero->belongsToHuman();
    if (humanPlayer) {
        normalDialog(questionText, 2, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn == DIALOG_RETURN_DECLINE)
            return;
    } else if (aiValueOfEvent(currentHero, point) <= 0) {
        return;
    }

    cell->setCellVisited(currentHero->m_owner);
    if (cell->m_extraInfo & 0x2000000) {
        if (humanPlayer)
            normalDialog(emptyText, 1, -1, -1,
                         0x10, 0, -1, 0, -1, 0, -1, 0);
        if (currentHero->m_flags & visitedFlag)
            return;
        currentHero->m_flags |= visitedFlag;
        currentHero->m_moraleBonus--;
        return;
    }

    creatureBankEvent(currentHero, cell, "", point,
                      humanPlayer);
}

// E:\gamedcs\events.cpp:5851.
VA(0x004ac580, 0x3A7)  // dc-bracket forced, ret 0x2c=p12 (unique), dc 0x9af34
int advManager::combatMonsterEvent(hero* who, int monType, int* numMons,
                                   NewmapCell* eventCell, type_point point,
                                   TCreatureType monType2, int numMons2,
                                   int numGroups2, TCreatureType monType3,
                                   int numMons3, int numGroups3)
{
    static double threshold[6] = { 3.0, 2.0, 1.5, 1.0, 0.67, 0.5 };
    static const int reorderMap[7][7][7] = {
        {
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 }
        },
        {
            { 1, 2, 3, 0, 4, 5, 6 },
            { 1, 2, 0, 3, 6, 4, 5 },
            { 1, 5, 2, 0, 3, 6, 4 },
            { 1, 4, 2, 0, 5, 3, 6 },
            { 3, 1, 4, 0, 5, 2, 6 },
            { 2, 3, 0, 4, 1, 5, 6 },
            { 1, 2, 3, 0, 4, 5, 6 }
        },
        {
            { 2, 3, 0, 4, 1, 5, 6 },
            { 2, 0, 3, 6, 4, 1, 5 },
            { 2, 0, 5, 3, 6, 1, 4 },
            { 4, 0, 2, 5, 3, 1, 6 },
            { 3, 0, 4, 2, 5, 1, 6 },
            { 2, 3, 0, 4, 1, 5, 6 },
            { 0, 0, 0, 0, 0, 0, 0 }
        },
        {
            { 3, 0, 4, 1, 5, 2, 6 },
            { 0, 3, 1, 6, 4, 2, 5 },
            { 0, 3, 5, 1, 6, 4, 2 },
            { 0, 4, 1, 3, 5, 2, 6 },
            { 3, 0, 4, 1, 5, 2, 6 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 }
        },
        {
            { 0, 4, 1, 5, 2, 6, 3 },
            { 0, 4, 1, 6, 2, 5, 3 },
            { 0, 5, 1, 4, 2, 6, 3 },
            { 0, 4, 1, 5, 2, 6, 3 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 }
        },
        {
            { 0, 1, 5, 2, 6, 3, 4 },
            { 0, 1, 5, 2, 6, 3, 4 },
            { 0, 1, 5, 2, 6, 3, 4 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 }
        },
        {
            { 0, 1, 2, 6, 3, 4, 5 },
            { 0, 1, 2, 6, 3, 4, 5 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0 }
        }
    };

    demobilizeCurrHero(0, 1);

    const int eventSeed = point.m_x * 0x3c907 + point.m_z * 0x4bb5f
                     + point.m_y * 0x4386d + 0x25ea7;
    sRand(eventSeed);

    int combatValue = g_creatureTypeTraits[monType].m_aiValue * *numMons;
    if (monType2 != CREATURE_NONE)
        combatValue += g_creatureTypeTraits[monType2].m_aiValue * numMons2;
    if (monType3 != CREATURE_NONE)
        combatValue += g_creatureTypeTraits[monType3].m_aiValue * numMons3;

    who->m_army.getAIValue();
    who->getPrimarySkillTotal();
    // [2026-08-30] Residual (99.2966%): one fld slot - retail colours the
    // ratio home, the int->double divisor temp and the quotient into ONE
    // reused qword ([ebp-0xc]) by reloading ratio BEFORE converting the
    // divisor; our CL hoists the divisor conversion above the reload, so
    // the two homes must coexist and the frame gains a slot. Tried and
    // rejected (all byte-flat): `ratio = ratio / combat_value`, a
    // block-scoped named double divisor, compound `/=`, an explicit
    // static_cast<double> divisor, and a named volatile divisor.  Making
    // ratio volatile restores the 0x88 frame and every later block, but
    // scores 98.78% because it fences the two independent numGroups/threshold
    // setup instructions after the division.  NB11 proves ratio is a plain
    // T_REAL64 local, so the volatile spelling is only a negative control.
    double ratio = who->m_army.getAIValue();
    ratio /= combatValue;

    int numGroups = 7;
    for (int thresholdIndex = 5;
         thresholdIndex > 0 && ratio >= threshold[thresholdIndex];
         --thresholdIndex)
        --numGroups;

    int chance = random(1, 100);
    if (chance <= 20)
        --numGroups;
    if (chance >= 80 && numGroups < 7)
        ++numGroups;

    if (monType2 != CREATURE_NONE)
        numGroups -= numGroups2;
    if (monType3 != CREATURE_NONE)
        numGroups -= numGroups3;
    if (numGroups < 1)
        numGroups = 1;
    if (numGroups > *numMons)
        numGroups = *numMons;

    armyGroup currentArmyGroup;
    for (int i = 0; i < numGroups; ++i) {
        currentArmyGroup.m_armies[i] = monType;
        currentArmyGroup.m_numTroops[i] = *numMons / numGroups
                                  + (*numMons % numGroups > i);
    }

    {
        int storage;
        storage = monType;
        if ((g_game->m_gameVersion
             || !isBaseElemental(monType))
            && static_cast<unsigned char>(
                   isBaseCreature(TCreatureType(storage)))
            && numGroups > 1
            && monType2 == CREATURE_NONE
            && monType3 == CREATURE_NONE
            && sRandom(1, 100) <= 50) {
            TCreatureType upgraded;
            if (!g_game->m_gameVersion
                && isBaseElemental(monType))
                upgraded = CREATURE_NONE;
            else {
                int upgradeType;
                upgradeType = monType;
                upgraded = upgradedCreatureType(TCreatureType(upgradeType));
            }
            currentArmyGroup.m_armyTypes[numGroups / 2] = upgraded;
        }
    }

    int totalGroups = numGroups;
    if (monType2 != CREATURE_NONE) {
        for (int i = 0; i < numGroups2; ++i) {
            currentArmyGroup.m_armyTypes[numGroups + i] = monType2;
            currentArmyGroup.m_numTroops[numGroups + i] =
                numMons2 / numGroups2 + (numMons2 % numGroups2 > i);
        }
        totalGroups += numGroups2;
    }

    if (monType3 != CREATURE_NONE) {
        for (int i = 0; i < numGroups3; ++i) {
            currentArmyGroup.m_armyTypes[totalGroups + i] = monType3;
            currentArmyGroup.m_numTroops[totalGroups + i] =
                numMons3 / numGroups3 + (numMons3 % numGroups3 > i);
        }
    }

    if (monType2 != CREATURE_NONE || monType3 != CREATURE_NONE) {
        int tempNumTroops[7];
        TCreatureType tempArmies[7];
        // DC names both arrays; Mac copies their slots without memcpy calls.
        for (int slot = 0; slot < 7; ++slot) {
            tempNumTroops[slot] = currentArmyGroup.m_numTroops[slot];
            tempArmies[slot] = currentArmyGroup.m_armyTypes[slot];
        }
        for (int i = 0; i < 7; ++i) {
            currentArmyGroup.m_armyTypes[i] =
                tempArmies[reorderMap[numGroups][numGroups3][i]];
            currentArmyGroup.m_numTroops[i] =
                tempNumTroops[reorderMap[numGroups][numGroups3][i]];
        }
    }

    int result = doCombat(point, who, &who->m_army, -1, 0, 0,
                          &currentArmyGroup, eventSeed, 1, 0);
    {
        int storage;
        storage = monType;
        *numMons = currentArmyGroup.getCreatureTotal(TCreatureType(storage));
    }
    mobilizeCurrHero(0, 0, 1);
    return result;
}

// The owner byte is READ INTO A LOCAL before Deallocate runs and only
// spent afterwards, because the hero record does not survive the call;
// that ordering is what puts it in a frame slot rather than a register
// rematerialization.

// The whole fizzle is FizzleCenter (0x4acbb0) inlined - its own body sits
// further down this file and is emitted anyway because the function has
// external linkage, but this is its only call site, so /Ob2 expands it
// here as well. That is also why the gCompleteDrawEnabled gate and the
// sound switch appear inside this body rather than behind a call.
VA(0x004ac930, 0x163)  // dc 0x9b3b4
void advManager::heroLoses(hero* who, int vanishSound)
{
    if (!who)
        return;

    completeDraw(false);
    updateScreen(0, 0);

    int owner = who->m_owner;
    who->deallocate(1, 0);
    fizzleCenter(vanishSound);

    updateRadar(1, 1, 0, 0, 0);
    m_advWindow->updateHeroLocators(-1, 1, 1);
    if (g_game->m_mapHeader.m_lossCondition.heroKilled(who)) {
        g_game->m_mapHeader.m_lossCondition.m_playerLoser = owner;
        checkEndGame(0);
    }
}

VA(0x004acaa0, 0x106)  // dc 0x9b448
void advManager::doWhirlpool(hero* who)
{
    if (!g_game->isHuman(who->m_owner)
        || who->isWieldingArtifact(ARTIFACT_SEA_CAPTAINS_HAT))
        return;

    int weakestValue = 99999999;
    int weakestArmy = -1;
    for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
        if (who->m_army.m_numTroops[i] > 0) {
            int value = who->m_army.m_numTroops[i]
                * g_creatureTypeTraits[who->m_army.m_armies[i]].m_baseFightValue;
            if (value < weakestValue) {
                weakestArmy = i;
                weakestValue = value;
            }
        }
    }

    if (who->m_army.getNumArmies() > 1) {
        who->m_army.m_numTroops[weakestArmy] >>= 1;
        if (!who->m_army.m_numTroops[weakestArmy])
            who->m_army.m_armies[weakestArmy] = CREATURE_NONE;
    } else if (who->m_army.m_numTroops[weakestArmy] > 1) {
        who->m_army.m_numTroops[weakestArmy] >>= 1;
    }

    normalDialog((*g_adventureEventText)[ADV_EVENT_TEXT_WHIRLPOOL], 1, -1, -1,
                 -1, 0, -1, 0, -1, 0, -1, 0);
}

VA(0x004acbb0, 0xE4)  // dc 0x9b564
void advManager::fizzleCenter(int whichSound)
{
    if (!g_completeDrawEnabled)
        return;
    switch (whichSound) {
    case FIZZLE_SOUND_KILL_FADE:
        strcpy(g_text, DATA_COMPGEN(0x00677740, killFadeSampleName, "killfade.82M"));
        break;
    case FIZZLE_SOUND_PICKUP:
        sprintf(g_text,
                DATA_COMPGEN(0x00670268, pickupSampleFormat, "pickup%02d.82M"),
                random(1, 7));
        break;
    default:
        return;
    }
    launchSample(g_text, -1, 3);
    g_mouseManager->hidePointer();
    g_windowManager->saveFizzleSourceX(0xc0, 0xa0, 0xe0, 0xe0);
    completeDraw(false);
    g_windowManager->fizzleForwardX(0xc0, 0xa0, 0xe0, 0xe0, 0x41);
    g_mouseManager->showPointer(false);
}

VA(0x004acca0, 0xC3)  // dc 0x9b670
void advManager::doAIEvent(NewmapCell* cell, hero* currentHero, type_point point)
{
    if (point.m_x == currentHero->m_pathTargetX
        && point.m_y == currentHero->m_pathTargetY
        && point.m_z == currentHero->m_pathTargetZ)
        currentHero->m_pathTargetX = currentHero->m_pathTargetY = -1;

    currentHero->m_movePoints = max(--currentHero->m_movePoints, 0);
    dispatchEvent(currentHero, cell, point, 0);

    if (currentHero->m_owner != -1)
        currentHero->checkLevel();
    if (g_game->m_mapHeader.m_victoryCondition.checkForTotalCreatures())
        checkEndGame(0);
    if (g_game->m_mapHeader.m_victoryCondition.checkForTotalResources())
        checkEndGame(0);
}

VA(0x004acd70, 0x365)  // dc 0x9b788
int advManager::doNetCombat(CNetMsg* netMsg)
{
    hero* leftHero = 0;
    armyGroup* leftArmyGroup = 0;
    town* rightTown = 0;
    hero* rightHero = 0;
    armyGroup* rightArmyGroup = 0;
    int rightPlayer = -1;

    CCombatInitMsg combatInitMsg;
    combatInitMsg.remoteFn00512E00(netMsg);

    if (IsIconic(g_hwndApp))
        ShowWindow(g_hwndApp, SW_RESTORE);
    SetForegroundWindow(g_hwndApp);

    int fromWho;
    type_point point;
    int seed;
    signed char winner;
    receiveHeroTownData(&combatInitMsg, &fromWho, point,
                        &leftHero, &leftArmyGroup, &rightPlayer,
                        &rightTown, &rightHero, &rightArmyGroup,
                        &seed, &winner,
                        &g_combatRetreated, &g_combatSurrendered);

    int leftPlayer = leftHero->m_owner;
    if (g_remoteOn && !g_thisNetGotAdventureControl) {
        if (rightHero)
            setHeroContext(rightHero->m_id, 0, 1, 1);
        else if (rightTown)
            setTownContext(rightTown->m_id, 1, 1);
    }

    winner = doCombat(point, leftHero, leftArmyGroup, rightPlayer,
                      rightTown, rightHero, rightArmyGroup, seed, 0, 0);

    if (!g_game->isHuman(leftPlayer))
        sendHeroTownData(point, leftHero, leftArmyGroup, rightPlayer,
                         rightTown, rightHero, rightArmyGroup, seed,
                         fromWho, winner,
                         g_combatRetreated, g_combatSurrendered);

    if (leftArmyGroup)
        delete leftArmyGroup;
    if (rightArmyGroup)
        delete rightArmyGroup;
    if (rightTown)
        delete rightTown;
    if (rightHero)
        delete rightHero;
    if (leftHero)
        delete leftHero;

    g_combatRetreated = 0;
    g_combatSurrendered = 0;
    return 1;
}

// E:\gamedcs\events.cpp:6283.  Located, not reconstructed - 5,425 EH-framed
// bytes DOMINATED by the same inline hero/town-copy walls that cap
// Send/ReceiveHeroTownData at 31-53% (see their notes below): a
// reconstruction inherits that ceiling, so this row is deferred until
// town.h's leading-pad-array surgery lands and lets the synthesized
// memberwise assign inline to retail's shape.

// STRUCTURE FULLY DECODED 2026-08-27 (dc 0x9b970), for the lane that takes
// it after the copy wall falls.  DC locals: iLeftPlayer, loser,
// winning_player, iSavePlayer, bSaveShowIt, turnDurationPause, iWinner,
// iFromWho, trightTown/tleftHero/temp_right_player/trightHero/
// tleftArmyGroup/trightArmyGroup (the ReceiveHeroTownData out-params),
// sText, target, msg, two dlg locals.  Body in order:
//   1. CTurnDuration turnDurationPause(&gUnnamed29d630); .Pause().  If
//      gbInCombatReplay (0x291209): mark BOTH combat-control seats busy
//      (gpGame+0x20bb1 / +0x20bb2 indexed by 0x29120c*0x48).
//   2. iLeftPlayer = rightHero ? rightHero->owner-ish... no: ebx =
//      leftHero ? leftHero->x22(owner byte) : -1.  bLeftHuman =
//      iRightPlayer>=0 && IsHuman(iRightPlayer); bRightHuman =
//      ebx>=0 && IsHuman(ebx).  iSeed = (iSeed==-1) ? Random(1,1000).
//   3. DemobilizeCurrHero(0,1); Reseed(0,0).
//   4. FAST PATH: if !bLeftHuman && !bRightHuman && !(replay && replaySub):
//      compute the cell via worldMap 65*9*2 chain, call
//      AI_quick_combat(leftHero, rightHero, rightArmyGroup, rightTown,
//      cell); result -> winner; CheckForHeroDefeatWin(winner, loser) ->
//      game_2450_sub18_f2ce0 (CheckEndGame); MobilizeCurrHero(0,0,1);
//      turnDurationPause.Resume(); clear the two busy seats; return winner.
//   5. INTERACTIVE: SetPointer(0,2)/ShowPointer(1).  iSavePlayer =
//      gNetLocalGamePos, bSaveShowIt = gbShowIt(0x2989c0).
//   6. REMOTE arm (iRightPlayer>=0 && IsHuman && !IsLocalHuman):
//      GetLocalPlayerGamePos->iCombatControlNetPos, SendHeroTownData(...),
//      if !IsHuman(iLeftPlayer): CWaitForRemoteBattleDlg dlg; dlg.Wait();
//      if the received byte is set: ReceiveHeroTownData(&dlg's msg, ...)
//      then the big inline hero copies (+0x3da string, +0x3ea/+0x430
//      spell tables, +0x476 stats) and operator delete of each t* - THE
//      WALL.  ~CNetMsgHandlerPause / game_ad470_sub03_ad130 / ~CAnimatedDlg.
//   7. if !IsLocalHuman(iLeftPlayer): TurnOffAIMusic; GetPlayerName +
//      sprintf(sText) + WaitForPlayer(sText, iLeftPlayer).
//   8. SetupCombat(point, leftHero, leftArmyGroup, iRightPlayer, rightTown,
//      rightHero, rightArmyGroup, iSeed, bFinishHeroes, alternate_layout,
//      showIt); split_armies(leftHero,...) x2; gpExecutive->CallManager
//      (gpCombatManager) - runs the battle; SetPointer/ShowPointer.
//   9. winner = gpCombatManager result; CheckLevel both heroes; if remote
//      TransmitRemoteData + CLevelPickWaitDlg::WaitForLevels(iFromWho).
//  10. NewfullMapFn_00505D20 x2 + 00505D60 redraw; CheckForHeroDefeatWin
//      x3 with CheckEndGame; loser-side sound (Random/sprintf/
//      LoadPlaySample "COMBT*.wav"); MobilizeCurrHero; Resume; clear busy
//      seats; return winner.
// The combat-init payload's two serializers, slots 0 and 1 of vtable
// 0x63e508. Their scalar prefix is written by SendHeroTownData and read
// back by ReceiveHeroTownData; the tail hands the five sub-objects to the
// serializers they own - armyGroup's, town's and hero's - with the
// CURRENT save version baked in, because a net packet is never a
// back-level file.
const int g_netCombatSaveVersion = 42;

// Residual on both (98.19% / 97.99%): one `push ecx`. Retail carries no
// frame at all - it homes the byte buffer at [ebp+0xb] and the dword at
// [ebp+8], overlapping inside the dead `infile` parameter slot once that
// pointer is live in ESI. Block-scoping the pair and swapping their
// declaration order are both byte-flat, measured. Moving int_buffer's
// declaration to its first assignment, and delaying write's const-cast
// alias until its tail calls, are byte-flat as well (2026-09-07).
// Eight paired scratch-type/lifetime controls (plain/signed char, int/long,
// shared/per-field dword scopes) produce two distinct objects and leave both
// scores unchanged. None recovers the overlapping dead parameter home.
VA(0x004ad1f0, 0x148)  // anchor-vtable 0x63e508 slot 0; anchor-callee town::load + hero::load, retail-only
unsigned char CCombatInitMsg::read(TAbstractFile* infile)
{
    char charBuffer;
    int intBuffer;

    infile->read(&m_point, sizeof(m_point));
    infile->read(&charBuffer, sizeof(charBuffer));
    m_leftHero = charBuffer != 0;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_rightTown = charBuffer != 0;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_rightHero = charBuffer != 0;
    infile->read(&intBuffer, sizeof(intBuffer));
    m_seed = intBuffer;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_winner = charBuffer;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_retreatWin = charBuffer != 0;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_combatSurrender = charBuffer != 0;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_leftOwner = charBuffer;
    infile->read(&intBuffer, sizeof(intBuffer));
    m_leftGold = intBuffer;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_rightOwner = charBuffer;
    infile->read(&intBuffer, sizeof(intBuffer));
    m_rightGold = intBuffer;

    m_leftArmyGroup.load(infile);
    m_rightArmyGroup.load(infile);
    m_town.load(infile, g_netCombatSaveVersion);
    m_leftHeroData.load(infile, g_netCombatSaveVersion);
    m_rightHeroData.load(infile, g_netCombatSaveVersion);
    return 1;
}

// The mirror. `write` is const across this whole message family - it is
// the base class's virtual - while every sub-object's own save() is not,
// so the five member calls share one mutable alias.
VA(0x004ad340, 0x126)  // anchor-vtable 0x63e508 slot 1; anchor-callee town::save + hero::save, retail-only
unsigned char CCombatInitMsg::write(TAbstractFile* outfile) const
{
    char charBuffer;
    int intBuffer;
    CCombatInitMsg* record = const_cast<CCombatInitMsg*>(this);

    outfile->write(&m_point, sizeof(m_point));
    charBuffer = m_leftHero;
    outfile->write(&charBuffer, sizeof(charBuffer));
    charBuffer = m_rightTown;
    outfile->write(&charBuffer, sizeof(charBuffer));
    charBuffer = m_rightHero;
    outfile->write(&charBuffer, sizeof(charBuffer));
    intBuffer = m_seed;
    outfile->write(&intBuffer, sizeof(intBuffer));
    charBuffer = m_winner;
    outfile->write(&charBuffer, sizeof(charBuffer));
    charBuffer = m_retreatWin;
    outfile->write(&charBuffer, sizeof(charBuffer));
    charBuffer = m_combatSurrender;
    outfile->write(&charBuffer, sizeof(charBuffer));
    charBuffer = m_leftOwner;
    outfile->write(&charBuffer, sizeof(charBuffer));
    intBuffer = m_leftGold;
    outfile->write(&intBuffer, sizeof(intBuffer));
    charBuffer = m_rightOwner;
    outfile->write(&charBuffer, sizeof(charBuffer));
    intBuffer = m_rightGold;
    outfile->write(&intBuffer, sizeof(intBuffer));

    record->m_leftArmyGroup.save(outfile);
    record->m_rightArmyGroup.save(outfile);
    record->m_town.save(outfile);
    record->m_leftHeroData.save(outfile);
    record->m_rightHeroData.save(outfile);
    return 1;
}

// E:\gamedcs\events.cpp:6248/6261 (dc 0x9ce40 / 0x9ceb0) - the RAII pause
// DoCombat holds across a whole battle; the class shape lives in
// events.h, the two bodies are this TU's own (their DC line numbers are
// events.cpp's). Retail inlines both at DoCombat's entry and at each of
// its two returns: Pause() plus the solo-seat marking on the way in;
// Resume() plus the seat clearing - gated on this machine still being
// the solo seat - on the way out.
// E:\gamedcs\events.cpp:6248, dc 0x9ce40.
inline CTurnDurationPause::CTurnDurationPause()
{
    g_turnDuration.pause();
    if (g_goSolo) {
        g_game->m_players[g_soloPos].m_isLocal = 1;
        g_game->m_players[g_soloPos].m_isHuman = 1;
    }
}

// E:\gamedcs\events.cpp:6261, dc 0x9ceb0.
// This is a written destructor; its retail copy belongs to this body.
VA(0x004ae9b0, 0x50)
inline CTurnDurationPause::~CTurnDurationPause()
{
    g_turnDuration.resume();
    if (g_goSolo && g_netLocalGamePos == g_soloPos) {
        g_game->m_players[g_soloPos].m_isLocal = 0;
        g_game->m_players[g_soloPos].m_isHuman = 0;
    }
}

// cText/alternate_layout ride to SetupCombat; bFinishHeroes gates the
// level-pick wait.  All callees are already declared (cmbtmgr.h,
// remotedlg.h, exec.h, game.h).
// Spell-array consolidation: removing army's duplicate spellInfluence
// overlay makes its copy constructor exact, but moves this body's current
// score 98.5379 -> 98.1758 without changing its source. Disposable Gruntz
// forest search (seed 20260906, baseline + 16 variants per placement):
// before this function all variants retain 98.1758; before TU includes,
// two islands appear and 10 variants recover 98.5379. No probe declarations
// are retained. The later retail-proven TPalette16 consolidation recovers
// 98.5379 in production without noise; preserve both canonical types.
// Goto audit: DC line 6482 destroys CWaitForRemoteBattleDlg before branching
// to the common aftermath (dc 0x9bd66 -> 0x9c012). A bool/byte remote-battle
// result guarding local setup scores 92.5247%; a breakable for scope scores
// 92.5181%, and do/while with break scores 71.2640%, versus 98.5379%.
// The direct forward exit to combatFinished preserves the dialog's destruction
// before the common aftermath. With canonical finish helpers it is byte-flat
// against the former do/while(0)+continue model (95.6234), without an artificial
// one-iteration loop or an outer scope around the aftermath.
// DC 6654..6670 restores HeroLoses calls and left/right/neither case order,
// replacing three pasted helper expansions and all four inline-depth pins.
// This recovers retail's three retained FizzleCenter calls followed by three
// retained HeroLoses calls. GetArmyName(1/2), Game::get_cell and text operator[]
// raise that unpinned model from 93.9236 to 95.6386; DC's sText[256] and SRandom
// give 95.6234. The old pinned implementation was 97.9862, not an unpinned peak.
// Remaining calls: town cleanup uses _Tidy instead of delete; CNetMsg's ctor
// remains out of line; the final CTurnDurationPause destructor also remains
// a call. CCombatInitMsg's destructor now correctly stays out of line.
// DC uses DestroyMsg for its pointer payload; Complete's independently proven
// CWaitForRemoteBattleDlg owns the payload by value, so no DestroyMsg is added.
// Logical bool/byte replay spellings are byte-flat. A direct stats argument
// does not compile: the DC member records independently prove stats private.
// Pause-guard source tests (header declaration versus CPP-owned in-class or
// ordinary out-of-class bodies) are flat at 95.6234 and keep its dtor exact.
// DC netmsg.h:488..494 confirms CHeroLevelUpdateMsg's two memcpy calls, not
// counted loops. Putting its scalar fields before both copies, whether by
// assignment or initializer list, gives 94.5662 here; putting numSSs between
// the copies is flat. DC's scheduled store alone does not settle the spelling.
VA(0x004ad470, 0x1531)  // anchor-callee CTurnDuration::Pause, ret 0x28=p11 (unique), dc 0x9b970
int advManager::doCombat(type_point point, hero* leftHero, armyGroup* leftArmyGroup, long rightPlayer, town* rightTown, hero* rightHero, armyGroup* rightArmyGroup, int seed, unsigned char finishHeroes, unsigned char alternateLayout)
{
    int leftPlayer = leftHero ? leftHero->m_owner : -1;
    int winningPlayer;  // DC winning_player
    hero* loser;
    CTurnDurationPause turnDurationPause;

    unsigned char rightHuman =
        rightPlayer >= 0 && g_game->isHuman(rightPlayer);
    unsigned char leftHuman = leftPlayer >= 0 && g_game->isHuman(leftPlayer);
    if (seed == -1)
        seed = random(1, 1000);
    demobilizeCurrHero(0, 1);
    reseed(0, 0);

    unsigned char replay = (g_goSolo && g_goSoloTest) ? 1 : 0;
    if (!rightHuman && !leftHuman && !replay) {
        int winner;
        NewmapCell* target = g_game->getCell(point);
        if (aiQuickCombat(leftHero, rightHero, *rightArmyGroup, rightTown,
                            target)) {
            winningPlayer = leftPlayer;
            winner = 0;
            loser = rightHero;
        } else {
            winner = 1;
            winningPlayer = rightPlayer;
            loser = leftHero;
        }
        if (g_game->m_mapHeader.m_victoryCondition.checkForHeroDefeatWin(
                winningPlayer, loser))
            checkEndGame(0);
        mobilizeCurrHero(0, 0, 1);
        return winner;
    }

    g_mouseManager->setPointer(2, mouseManager::DEFAULT_SET);
    g_mouseManager->showPointer(1);
    int savePlayer = g_netLocalGamePos;
    int saveShowIt = g_completeDrawEnabled;
    g_adventureCombatActive = 1;

    if (leftPlayer >= 0 && rightPlayer >= 0
        && g_game->isHuman(rightPlayer)) {
        if (!g_game->isLocalHuman(rightPlayer)) {
            g_combatControlNetPos[0] = g_game->getLocalPlayerGamePos();
            g_combatControlNetPos[1] = rightPlayer;
            sendHeroTownData(point, leftHero, leftArmyGroup, rightPlayer,
                             rightTown, rightHero, rightArmyGroup, seed,
                             rightPlayer, 0, 0, 0);
            if (!g_game->isHuman(leftPlayer)) {
                CWaitForRemoteBattleDlg dlg;
                dlg.wait(rightPlayer);
                if (dlg.m_combatInitMsgReceived) {
                    int fromWho;
                    hero* tleftHero;
                    armyGroup* tleftArmyGroup;
                    int tempRightPlayer;
                    town* trightTown;
                    hero* trightHero;
                    armyGroup* trightArmyGroup;
                    signed char winnerId;
                    receiveHeroTownData(&dlg.m_combatInitMsg, &fromWho,
                                        point, &tleftHero, &tleftArmyGroup,
                                        &tempRightPlayer, &trightTown,
                                        &trightHero, &trightArmyGroup, &seed,
                                        &winnerId, &g_combatRetreated,
                                        &g_combatSurrendered);
                    if (trightTown) {
                        *rightTown = *trightTown;
                        delete trightTown;
                    }
                    if (trightHero) {
                        *rightHero = *trightHero;
                        delete trightHero;
                    }
                    if (tleftHero) {
                        *leftHero = *tleftHero;
                        delete tleftHero;
                    }
                    if (tleftArmyGroup) {
                        *leftArmyGroup = *tleftArmyGroup;
                        delete tleftArmyGroup;
                    }
                    if (trightArmyGroup) {
                        *rightArmyGroup = *trightArmyGroup;
                        delete trightArmyGroup;
                    }
                    g_combatManager->m_winner = winnerId;
                } else {
                    g_combatManager->m_winner = 0;
                }
                goto combatFinished;
            }
        } else if (!g_game->isLocalHuman(leftPlayer)) {
            g_completeDrawEnabled = 1;
            g_game->turnOffAIMusic();
            char text[256];  // DC sText[256]
            const char* target;
            if (rightTown)
                target = g_generalText->getText(GENERAL_TEXT_ATTACK_TARGET_TOWN);
            else if (rightHero)
                target = g_generalText->getText(GENERAL_TEXT_ATTACK_TARGET_HERO);
            else
                target = g_generalText->getText(GENERAL_TEXT_ATTACK_TARGET_GARRISON);
            sprintf(text, g_generalText->getText(GENERAL_TEXT_TOWN_UNDER_ATTACK_FORMAT),
                    g_game->getPlayerName(rightPlayer), target);
            g_game->waitForPlayer(text, rightPlayer);
        }
    }

    g_completeDrawEnabled = 1;
    g_combatManager->setupCombat(point, leftHero, leftArmyGroup, rightPlayer,
                                 rightTown, rightHero, rightArmyGroup,
                                 point.m_x, point.m_y, seed, alternateLayout);
    if (!leftHuman)
        aiArrangeArmyForCombat(leftHero, rightHero, *rightArmyGroup);
    if (!rightHuman && rightHero
        && rightHero->m_skillLevel[eSecSkillBattleTactics]
               > leftHero->m_skillLevel[eSecSkillBattleTactics])
        aiArrangeArmyForCombat(rightHero, leftHero, *leftArmyGroup);
    if (g_highMemBuffer > 2900)
        g_adventureGraphicsPreserveMode = 2;
    else if (g_highMemBuffer > 900)
        g_adventureGraphicsPreserveMode = 1;
    g_executive->callManager(g_combatManager);
    g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
    g_mouseManager->showPointer(1);
    g_adventureGraphicsPreserveMode = 0;
    if (leftHero)
        leftHero->checkLevel();
    if (rightHero) {
        if (g_remoteOn && rightHuman && leftHuman
            && g_combatManager->m_winner == 1) {
            if (g_game->isLocalHuman(rightHero->m_owner)) {
                rightHero->checkLevel();
                signed char stats[4];
                rightHero->copyPrimarySkills(stats);
                CHeroLevelUpdateMsg msg(rightHero->m_id, rightHero->m_skillCount,
                                        rightHero->m_skillLevel, stats);
                transmitRemoteData(&msg, g_netLocalGamePos, 0, 1);
            } else {
                CLevelPickWaitDlg dlg2;
                dlg2.waitForLevels(rightHero->m_owner);
                if (dlg2.m_playerDropped)
                    rightHero->checkLevel();
            }
        } else {
            rightHero->checkLevel();
        }
    }


combatFinished:
    int winner = g_combatManager->m_winner;
    if (winner != 0)
        g_game->m_worldMap.notifyHeroDefeated(leftHero->m_id, rightPlayer);
    if (winner != 1) {
        if (rightHero)
            g_game->m_worldMap.notifyHeroDefeated(rightHero->m_id, leftPlayer);
        else
            g_game->m_worldMap.notifyMonsterDefeated(point, leftPlayer);
    }
    if (winner == -1) {
        if (g_game->m_mapHeader.m_victoryCondition.checkForHeroDefeatWin(
                leftPlayer, rightHero)
            || g_game->m_mapHeader.m_victoryCondition.checkForHeroDefeatWin(
                   rightPlayer, leftHero))
            checkEndGame(0);
    } else {
        if (winner == 0) {
            winningPlayer = leftPlayer;
            loser = rightHero;
        } else if (winner == 1) {
            winningPlayer = rightPlayer;
            loser = leftHero;
        }
        g_game->m_mapHeader.m_victoryCondition.checkForHeroDefeatWin(
            winningPlayer, loser);
    }

    if (g_combatManager->m_raisedCreatureCount > 0 && winner == 0
        && g_game->isLocalHuman(leftPlayer)) {
        sprintf(g_text, "pickup%02d.82M", sRandom(1, 7));
        SAMPLE2 sample = loadPlaySample(g_text);
        if (g_combatManager->m_raisedCreatureCount == 1) {
            sprintf(g_text, g_generalText->getText(GENERAL_TEXT_NECROMANCY_RAISE_ONE_FORMAT),
                    getArmyName(g_combatManager->m_raisedCreatureType, 1));
        } else {
            sprintf(g_text, g_generalText->getText(GENERAL_TEXT_NECROMANCY_RAISE_MANY_FORMAT),
                    g_combatManager->m_raisedCreatureCount,
                    getArmyName(g_combatManager->m_raisedCreatureType, 2));
        }
        normalDialog(
            g_text, 1, -1, -1, 0x15,
            (static_cast<unsigned short>(g_combatManager->m_raisedCreatureCount)
             << 16)
                | static_cast<unsigned short>(
                      g_combatManager->m_raisedCreatureType),
            -1, 0, -1, 15000, -1, 0);
        g_game->m_mapHeader.m_victoryCondition.checkForTotalCreatures();
        clearMemSample(sample);
    }

    if (finishHeroes) {
        switch (g_combatManager->m_winner) {
        case COMBAT_WINNER_LEFT:
            if (rightTown && rightHero
                && rightTown->m_garrisonHeroId == rightHero->m_id)
                heroLoses(rightHero, -1);
            else
                heroLoses(rightHero, 0);
            break;
        case COMBAT_WINNER_RIGHT:
            heroLoses(leftHero, 0);
            break;
        case COMBAT_WINNER_NONE:
            heroLoses(leftHero, 0);
            if (rightTown && rightHero
                && rightTown->m_garrisonHeroId == rightHero->m_id)
                heroLoses(rightHero, -1);
            else
                heroLoses(rightHero, 0);
            break;
        }
    }

    g_completeDrawEnabled = saveShowIt;
    g_netLocalGamePos = savePlayer;
    if (!g_currentPlayer->isHuman()) {
        if (!g_remoteOn)
            g_game->showComputerScreen();
        g_game->turnOnAIMusic();
        setNoDialogMenus(0);
    } else {
        setNoDialogMenus(1);
    }
    mobilizeCurrHero(0, 0, 1);
    if (finishHeroes) {
        g_combatRetreated = 0;
        g_combatSurrendered = 0;
    }
    g_adventureCombatActive = 0;
    g_mouseManager->showPointer(1);
    checkEndGame(0);
    return g_combatManager->m_winner;
}

VA(0x004aeb50, 0x390)  // dc 0x9c35c
void advManager::sendHeroTownData(type_point point, hero* leftHero, armyGroup* leftArmyGroup, long rightPlayer, town* rightTown, hero* rightHero, armyGroup* rightArmyGroup, int seed, int toWhoNetPos, int winner, unsigned char retreatWin, unsigned char combatSurrender)
{
    CCombatInitMsg combatInitMsg;
    combatInitMsg.m_point = point;
    combatInitMsg.m_leftHero = leftHero != 0;
    combatInitMsg.m_rightHero = rightHero != 0;
    combatInitMsg.m_rightTown = rightTown != 0;
    combatInitMsg.m_seed = seed;
    combatInitMsg.m_winner = winner;
    combatInitMsg.m_retreatWin = retreatWin;
    combatInitMsg.m_combatSurrender = combatSurrender;

    if (leftHero) {
        combatInitMsg.m_leftOwner = leftHero->m_owner;
        combatInitMsg.m_leftGold =
            g_game->m_players[leftHero->m_owner].m_resources[GOLD];
    } else {
        combatInitMsg.m_leftOwner = -1;
        combatInitMsg.m_leftGold = 0;
    }
    combatInitMsg.m_rightOwner = rightPlayer;
    if (rightHero)
        combatInitMsg.m_rightGold =
            g_game->m_players[rightHero->m_owner].m_resources[GOLD];
    else
        combatInitMsg.m_rightGold = 0;

    combatInitMsg.m_leftArmyGroup = *leftArmyGroup;
    combatInitMsg.m_rightArmyGroup = *rightArmyGroup;
    if (rightTown)
        combatInitMsg.m_town = *rightTown;
    if (leftHero)
        combatInitMsg.m_leftHeroData = *leftHero;
    if (rightHero)
        combatInitMsg.m_rightHeroData = *rightHero;

    int result = combatInitMsg.remoteFn00512D40(toWhoNetPos, 0, 1);
    if (!result)
        shutDown(0);
}

VA(0x004aeee0, 0x3DF)  // dc 0x9c554
void advManager::receiveHeroTownData(CCombatInitMsg* combatInitMsg, int* fromWho, type_point& point, hero** leftHero, armyGroup** leftArmyGroup, int* rightPlayer, town** rightTown, hero** rightHero, armyGroup** rightArmyGroup, int* seed, signed char* winner, unsigned char* retreatWin, unsigned char* combatSurrender)
{
    *leftHero = 0;
    *leftArmyGroup = 0;
    *rightTown = 0;
    *rightHero = 0;
    *rightArmyGroup = 0;
    *rightPlayer = -1;

    *fromWho = combatInitMsg->m_netmsg.m_from;
    point = combatInitMsg->m_point;
    int hasLeftHero = combatInitMsg->m_leftHero;
    int hasRightTown = combatInitMsg->m_rightTown;
    int hasRightHero = combatInitMsg->m_rightHero;
    *seed = combatInitMsg->m_seed;
    *winner = combatInitMsg->m_winner;
    *retreatWin = combatInitMsg->m_retreatWin;
    *combatSurrender = combatInitMsg->m_combatSurrender;

    if (combatInitMsg->m_leftOwner > 0)
        g_game->m_players[combatInitMsg->m_leftOwner].m_resources[GOLD] =
            combatInitMsg->m_leftGold;
    int rightOwner = combatInitMsg->m_rightOwner;
    *rightPlayer = rightOwner;
    if (rightOwner > 0)
        g_game->m_players[rightOwner].m_resources[GOLD] =
            combatInitMsg->m_rightGold;

    *leftArmyGroup = new armyGroup;
    **leftArmyGroup = combatInitMsg->m_leftArmyGroup;
    *rightArmyGroup = new armyGroup;
    **rightArmyGroup = combatInitMsg->m_rightArmyGroup;

    if (hasRightTown) {
        town* newTown = new town;
        *rightTown = newTown;
        *newTown = combatInitMsg->m_town;
    }

    g_combatControlNetPos[0] = *fromWho;
    g_combatControlNetPos[1] = g_game->getLocalPlayerGamePos();

    if (hasLeftHero) {
        hero* newHero = new hero;
        *leftHero = newHero;
        *newHero = combatInitMsg->m_leftHeroData;
    }
    if (hasRightHero) {
        hero* newHero = new hero;
        *rightHero = newHero;
        *newHero = combatInitMsg->m_rightHeroData;
    }
}

// RETIRED 2026-09-06, claim lane 31, and the note it replaces had the trade
// priced against only ONE of the two rows involved.

// The user-defined `~CWaitForRemoteBattleDlg()` with an interior
// `#pragma inline_depth(0)` was kept here because it made DoCombat's one
// inline expansion of the destructor call ~CNetMsgHandlerPause,
// ~CCombatInitMsg and ~CAnimatedDlg rather than expand their string
// teardowns, and because the synthesized spelling measured 98.54 there
// against 99.05. What that measurement never looked at is the emitted
// COMDAT: retail's own `??1CWaitForRemoteBattleDlg` is 0x4aea00 in this
// object's band, it has NO vtable store and it EXPANDS all three string
// teardowns - exactly the synthesized shape. The user-defined form scored
// that 253-byte row 28.9125.

// Removing the declaration (the class inherits CAnimatedDlg's virtual
// destructor, so the implicit one stays virtual) and the definition with
// it: 0x4aea00 28.9125 -> 100.0000 and DoCombat 99.0481 -> 98.5379, i.e.
// +180 B against -8 B, with SendHeroTownData, DoNetCombat,
// ReceiveHeroTownData, ~CCombatInitMsg, the constructor, Wait,
// handle_message and the scalar deleting destructor all holding at
// 100.0000. It also retires an inline-depth pin.

// COMDAT pairing: std::_Sort<int, spell_level_order>, agreement 0.985.
VA_COMPGEN(0x004b00b0, 0x294, STD_SORT, int_spell_level_order)

// COMDAT pairing: std::_Sort_0<int, spell_level_order>, agreement 1.000 over
// all 330 instructions.
VA_COMPGEN(0x004afcb0, 0x350, STD_SORT_0, int_spell_level_order)

// COMDAT pairing: std::_Unguarded_partition<int, spell_level_order>, 0.967.
VA_COMPGEN(0x004b0400, 0x123, STD_UNGUARDED_PARTITION, int_spell_level_order)

// COMDAT pairing: std::_Unguarded_insert<int, spell_level_order>, 0.966.
VA_COMPGEN(0x004b0350, 0xAB, STD_UNGUARDED_INSERT, int_spell_level_order)

// COMDAT pairing: ccombatinitmsg::1CCombatInitMsg, mnemonic agreement 0.938.
VA_COMPGEN(0x004ad130, 0xB4, IMPLICIT_DTOR, ccombatinitmsg)

// COMDAT pairing: clevelpickwaitdlg::1CLevelPickWaitDlg, mnemonic agreement 0.902.
VA_COMPGEN(0x004aeb00, 0x4B, IMPLICIT_DTOR, clevelpickwaitdlg)

// COMDAT pairing: vector<std::string>::insert(pos, n, val), agreement 0.985.
// Its single-element sibling and retained cleanup/copy/fill chain are now
// enrolled in seerhut, where their canonical <vector>/<algorithm> bodies
// still emit. Retail creatureBankEvent and five quest dialogs share them.
VA_COMPGEN(0x004af550, 0x2A5, VECTOR_INSERT, string)

// COMDAT pairing: std::_Construct<std::string>, agreement 0.972.
VA_COMPGEN(0x004afb40, 0x167, STD_CONSTRUCT, string)

// COMDAT pairing: vector<std::string>::~vector, agreement 0.950.
VA_COMPGEN(0x004af2c0, 0x6B, VECTOR_DTOR, string)

VA_COMPGEN(0x004af330, 0x13, VECTOR_SIZE, type_dialog_resource)

VA_COMPGEN(0x0054cba0, 0x1C5, VECTOR_INSERT, type_dialog_resource)

// COMDAT pairing: town's implicit destructor, agreement 1.000 at an exactly
// equal 74-byte extent.
VA_COMPGEN(0x004ad0e0, 0x4A, IMPLICIT_DTOR, town)

VA_COMPGEN(0x004aea00, 0xFD, IMPLICIT_DTOR, CWaitForRemoteBattleDlg)
