#ifndef HOMM3_STRIP_H
#define HOMM3_STRIP_H

#include "va.h"

#include "armygrp.h"

class hero;
class heroWindow;

// The one attested sentinel of the ctor's newIcons icon-set id:
// DrawOwner's SET_ICON_FRAME arm fires only for this set (every other
// set draws the akHeroTraits portrait by name). Name is a bootstrap
// invention - no DC/NH3API name survives; homm2's ctor slot was a
// portraitIconId.
enum EStripIconSet {
    STRIP_PORTRAIT_FRAME_SET = 0xa1
};

// The seven-slot troop strip: a widget-id remote control over an
// already-built heroWindow (all drawing goes through WIDGET_* messages
// to inWin; nothing touches a bitmap directly). `pos` selects one of
// two widget-id families on the same window:
//   pos == 0: owner frame 100, portrait 122, owner selector 123,
//             icons 101+i, numbers 108+i, selectors 115+i
//   pos != 0: portrait 124, owner selector 125,
//             icons 126+i, numbers 133+i, selectors 140+i

// PROVEN layout (retail ctor 0x5a9d20 stores every live member;
// sizeof from the `new strip` sites in townmgr's span - push 0x78
// before exe_new at 0x5c6e5a/0x5c6f1c/0x5c6fae/0x5c7045). The DC dump
// carries only a forward ref for strip (Size = 0), so the two pads
// have no name evidence: no retail body touches them, and the ctor's
// firstId parameter is dead (never stored). Member names follow the
// DC ctor parameter names; `current` is written only here (-2) -
// townManager's selection machinery owns it afterwards
// (?select_army@townManager@@AAAXPAVstrip@@J_N@Z).
class strip {
public:
    char m_pad00[0x1c];  // +0x00 untouched by the five retail bodies
    int m_x;              // +0x1c ctor inX
    int m_y;              // +0x20 ctor inY
    int m_pos;            // +0x24 ctor inPos - widget-id family selector
    long m_owner;         // +0x28 ctor new_owner (dead in the retail bodies)
    int m_current;        // +0x2c ctor -2; selected slot (-1 = owner, 0..6)
    char m_pad30[0x34];  // +0x30 untouched by the five retail bodies
    int m_icons;          // +0x64 ctor newIcons (icon-set id; 161 = frame set)
    heroWindow* m_win;    // +0x68 ctor inWin
    armyGroup* m_group;   // +0x6c ctor groupToDraw (0 = empty strip)
    int m_iconFrame;      // +0x70 ctor newIconFrame (-1 = no owner picture)
    hero* m_thisHero;

    strip(int inX, int inY, int inPos, int newIcons, int newIconFrame,
          long newOwner, hero* newHero, armyGroup* groupToDraw, int firstId,
          unsigned char update, heroWindow* inWin);
    // Declared, deliberately NOT defined - not here and not in strip.cpp.
    // Retail's `delete strip` calls a real out-of-line body before
    // operator delete: townManager::UnloadTown 0x5c70b0 and ::SwapHeroes
    // 0x5d5150 both emit `mov ecx,<p> / call 0x5bc690 / push <p> / call
    // ??3@YAXPAX@Z`. 0x5bc690 is a lone `ret`, which is what an empty
    // non-virtual destructor compiles to and what /OPT:ICF then folds
    // with every other empty body in the image (the carve names the fold
    // border_vslot04, so the address cannot be claimed here). Defining it
    // inline, or leaving it undeclared, drops the call and blocks every
    // body that frees a strip.
    ~strip();
    void draw(TCreatureType divideCreature);
    void drawIcons(unsigned char update, TCreatureType divideCreature);

protected:
    void drawNumber(int i);
    void drawOwner(int frame);
    void drawMonster(int i, int frame);
    void drawSelector(int i);
};
SIZE(strip, 0x78);

#endif  /* HOMM3_STRIP_H */
