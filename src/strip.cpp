// strip.cpp - E:\gamedcs\strip.cpp (compiland strip.obj)
// 5 retail functions in link order (of 8 DC procs).

// Retail span: the spells->subwindow gap. The gap's head belongs to
// spells.cpp's tail (the spell-id probe 0x5a93a0 and the std::map
// _Lockit template bodies 0x5a9450/0x5a9570/0x5a9870 - the last two
// are the bracket's two EH-prologue functions, neither strip's) and a
// cinit run (guard byte 0x6abaa0, ten-iteration bitset initializers,
// 0x5a9930..0x5a9d1f - excluded class). strip.obj's code is exactly
// 0x5a9d20..0x5aa31d in DC source order; then subwindow.cpp opens
// with its own cinit funclet at 0x5aa320 and the TSubWindow ctors at
// 0x5aa340.

// ~strip (DC :70) IS in retail, but not at an address this compiland
// can claim: it is empty, so it compiles to a lone `ret` that /OPT:ICF
// folded into the image-wide empty-body fold at 0x5bc690 (carve name
// border_vslot04). townmgr's `delete strip` proves the call survives -
// ::UnloadTown 0x5c70b0 and ::SwapHeroes 0x5d5150 both emit
// `mov ecx,<p> / call 0x5bc690 / push <p> / call ??3@YAXPAX@Z`. So
// strip.h declares it and nothing defines it: a definition here would
// emit an unpairable 1-byte body into strip.obj, and an inline
// definition in the header would delete the call at every use site.
// (Correction of an earlier note that read the NOP fill at
// 0x5a9d77..0x5a9d80 as proof the destructor was absent.)

// Absent from retail (documented, not forced): DrawNumber (DC :124)
// and DrawSelector (DC :253) - both survive only /Ob2-inlined inside DrawIcons
// 0x5a9db0 (the sprintf/SET_TEXT pair; three selector expansions at
// msg slots -0x48/-0x68/-0x68). The `inline` definitions below
// reproduce the absence under the non-/Gy profile (winfile Exists
// precedent).
#include <va.h>
#include <stdio.h>
#include "strip.h"
#include "hero.h"
#include "kb.h"
#include "message.h"
#include "widget.h"
#include "window.h"
#include "winmgr.h"

VA(0x005a9d20, 0x57)  // dc 0x158880
strip::strip(int inX, int inY, int inPos, int newIcons, int newIconFrame,
             long newOwner, hero* newHero, armyGroup* groupToDraw,
             int firstId, unsigned char update, heroWindow* inWin)
{
    m_x = inX;
    m_y = inY;
    m_owner = newOwner;
    m_thisHero = newHero;
    m_pos = inPos;
    m_icons = newIcons;
    m_iconFrame = newIconFrame;
    m_group = groupToDraw;
    m_current = -2;
    m_win = inWin;
    drawIcons(update, CREATURE_NONE);
}

VA(0x005a9d80, 0x30)  // dc 0x1588e8
void strip::draw(TCreatureType divideCreature)
{
    drawIcons(1, divideCreature);
    g_windowManager->updateScreen(m_x, m_y, 494, 64);
}

VA(0x005a9db0, 0x2A2)  // dc 0x158910
void strip::drawIcons(unsigned char update, TCreatureType divideCreature)
{
    int i;

    drawOwner(m_iconFrame);
    for (i = 0; i < 7; i++) {
        if (m_group == 0) {
            drawMonster(i, 0);
        } else {
            int type = m_group->m_armies[i];
            if (type != CREATURE_NONE && m_group->m_numTroops[i] > 0) {
                drawMonster(i, type + 2);
                sprintf(g_text, "%d", m_group->m_numTroops[i]);
                drawNumber(i);
                if (divideCreature == type && i != m_current)
                    drawSelector(i + 1);
            } else {
                drawMonster(i, 0);
                if (divideCreature != CREATURE_NONE)
                    drawSelector(i + 1);
            }
        }
    }
    if (divideCreature == CREATURE_NONE && m_current > -2)
        drawSelector(m_current + 1);
    if (update)
        m_win->drawWindow(0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

// E:\gamedcs\strip.cpp:124
// DC125 constructs message,126 sets its id; DrawIcons expands this ordinary
// helper. The 12-state source-fact family preserves all five tracked bodies.
void strip::drawNumber(int i)
{
    message msg;
    msg.m_qualifier = 0;
    msg.m_mouseX = 0;
    msg.m_mouseY = 0;
    msg.m_window = 0;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    if (m_pos == 0)
        msg.m_codeY = i + 108;
    else
        msg.m_codeY = i + 133;
    msg.m_extraText = g_text;
    m_win->broadcastMessage(msg);
    msg.m_codeX = widget::WIDGET_SET_STATUS;
    msg.m_extra = widget::WIDGET_DRAWN;
    m_win->broadcastMessage(msg);
}

// E:\gamedcs\strip.cpp:139
// DC140 constructs message,141 sets MESSAGE_WIDGET, and 143 tests pos.
// The shared constructor owns zero initialization. Removing the redundant
// caller stores and obsolete portrait const_casts is byte-neutral.
// Residual (93.1783%): the constructor expansion retains an extra codeX=0
// store before the branch. In the pos!=0 portrait arm, VC6 selects EAX for
// the traits base and EDX for the row index; retail uses EDX/ECX and loads
// the base before storing codeX. All eleven CFG blocks and ten named calls
// align apart from these instruction/size differences.
// The named portrait snapshot below avoids a different suffix merge. Prior
// direct-expression, row/base bindings and initialization-order controls
// did not resolve it; those bounded probes do not prove that no natural
// source form can produce retail's allocation. DC204 sets codeX and 205
// stores the portrait; no named portrait local is recorded.
VA(0x005aa060, 0x1CE)  // linkorder + body: akHeroTraits[frame] portrait via WIDGET_SET_IMAGE, owner widgets 100/122/123 (pos==0) and 124/125; dc 0x158a80
void strip::drawOwner(int frame)
{
    message msg;
    msg.m_id = MESSAGE_WIDGET;
    if (m_pos == 0) {
        if (m_iconFrame == -1) {
            msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
            msg.m_extra = widget::WIDGET_DRAWN;
            msg.m_codeY = 100;
            m_win->broadcastMessage(msg);
            msg.m_codeY = 122;
            m_win->broadcastMessage(msg);
        } else if (m_icons == STRIP_PORTRAIT_FRAME_SET) {
            msg.m_codeY = 100;
            msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
            msg.m_extra = frame;
            m_win->broadcastMessage(msg);
            msg.m_codeX = widget::WIDGET_SET_STATUS;
            msg.m_extra = widget::WIDGET_DRAWN;
            m_win->broadcastMessage(msg);
            msg.m_codeY = 122;
            msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
            msg.m_extra = widget::WIDGET_DRAWN;
            m_win->broadcastMessage(msg);
        } else {
            msg.m_codeY = 122;
            msg.m_codeX = widget::WIDGET_SET_IMAGE;
            msg.m_extraText = g_heroTraits[frame].m_largePortraitName;
            m_win->broadcastMessage(msg);
            msg.m_codeX = widget::WIDGET_SET_STATUS;
            msg.m_extra = widget::WIDGET_DRAWN;
            m_win->broadcastMessage(msg);
            msg.m_codeY = 100;
            msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
            msg.m_extra = widget::WIDGET_DRAWN;
            m_win->broadcastMessage(msg);
        }
        msg.m_codeY = 123;
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_extra = widget::WIDGET_DRAWN;
        m_win->broadcastMessage(msg);
        return;
    }
    msg.m_codeY = 124;
    if (m_iconFrame == -1) {
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
    } else {
        // The portrait name is read into a local AHEAD of the codeX
        // store. It is what stops the cross-jump that merged this arm's
        // closing broadcast with the pos==0 arm's (see the residual note
        // above the claim).
        const char* name = g_heroTraits[frame].m_largePortraitName;
        msg.m_codeX = widget::WIDGET_SET_IMAGE;
        msg.m_extraText = name;
        m_win->broadcastMessage(msg);
        msg.m_codeX = widget::WIDGET_SET_STATUS;
    }
    msg.m_extra = widget::WIDGET_DRAWN;
    m_win->broadcastMessage(msg);
    msg.m_codeY = 125;
    msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
    msg.m_extra = widget::WIDGET_DRAWN;
    m_win->broadcastMessage(msg);
}

VA(0x005aa230, 0xEE)  // dc 0x158c00
void strip::drawMonster(int i, int frame)
{
    message msg;
    msg.m_qualifier = 0;
    msg.m_mouseX = 0;
    msg.m_mouseY = 0;
    msg.m_window = 0;
    msg.m_id = MESSAGE_WIDGET;
    if (m_pos == 0)
        msg.m_codeY = i + 101;
    else
        msg.m_codeY = i + 126;
    if (frame == 0) {
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_extra = widget::WIDGET_DRAWN;
        m_win->broadcastMessage(msg);
        if (m_pos == 0)
            msg.m_codeY = i + 108;
        else
            msg.m_codeY = i + 133;
        m_win->broadcastMessage(msg);
    } else {
        msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
        msg.m_extra = frame;
        m_win->broadcastMessage(msg);
        msg.m_codeX = widget::WIDGET_SET_STATUS;
        msg.m_extra = widget::WIDGET_DRAWN;
        m_win->broadcastMessage(msg);
    }
    if (m_pos == 0)
        msg.m_codeY = i + 115;
    else
        msg.m_codeY = i + 140;
    msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
    msg.m_extra = widget::WIDGET_DRAWN;
    m_win->broadcastMessage(msg);
}

// E:\gamedcs\strip.cpp:253
// Ordinary helper expanded at all three DrawIcons call sites. DC254
// constructs message and 255 sets its id. i counts 1..7 for army slots and
// zero for the owner's selector widget.
void strip::drawSelector(int i)
{
    message msg;
    msg.m_qualifier = 0;
    msg.m_mouseX = 0;
    msg.m_mouseY = 0;
    msg.m_window = 0;
    msg.m_id = MESSAGE_WIDGET;
    if (m_pos == 0) {
        if (i == 0)
            msg.m_codeY = 123;
        else
            msg.m_codeY = i + 114;
    } else {
        if (i == 0)
            msg.m_codeY = 125;
        else
            msg.m_codeY = i + 139;
    }
    msg.m_codeX = widget::WIDGET_SET_STATUS;
    msg.m_extra = widget::WIDGET_DRAWN;
    m_win->broadcastMessage(msg);
}
