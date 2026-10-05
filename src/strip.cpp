// 5 retail functions in link order (of 8 DC procs).

// DrawNumber (DC :124) and DrawSelector (DC :253) survive as ordinary
// helpers on Mac; Windows expands them inside DrawIcons 0x5a9db0
// (the SET_TEXT pair and three selector expansions at message slots
// -0x48/-0x68/-0x68). Keep their source bodies and calls below.
#include "va.h"

#include <stdio.h>

#include "strip.h"

#include "hero.h"
#include "kb.h"
#include "message.h"
#include "widget.h"
#include "window.h"
#include "winmgr.h"

VA(0x005a9d20, 0x57)
DC_ADDRESS(0x158880, 0x62)
MAC_ADDRESS(0x19af44, 0x6c)
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

// Original: strip::~strip; strip.cpp:70
DC_ADDRESS(0x1588e4, 0x4)
MAC_ADDRESS(0x19afb0, 0x40)
strip::~strip()
{
}

VA(0x005a9d80, 0x30)
DC_ADDRESS(0x1588e8, 0x28)
MAC_ADDRESS(0x19aff0, 0x50)
void strip::draw(TCreatureType divideCreature)
{
    drawIcons(1, divideCreature);
    g_windowManager->updateScreen(m_x, m_y, 494, 64);
}

// DC 0x15892a constructs an unused message before DrawOwner and only
// writes its id. This is separate from DrawNumber's live message:
// neither desktop body initializes that unused local before DrawOwner
// (Windows 0x5a9dbf, Mac 0x19b064).
VA(0x005a9db0, 0x2A2)
DC_ADDRESS(0x158910, 0xf0)
MAC_ADDRESS(0x19b040, 0x15c)
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
DC_ADDRESS(0x158a00, 0x80)
MAC_ADDRESS(0x19b19c, 0xa8)
void strip::drawNumber(int i)
{
    message msg;
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
VA(0x005aa060, 0x1CE)
DC_ADDRESS(0x158a80, 0x180)
MAC_ADDRESS(0x19b244, 0x24c)  // linkorder + body: akHeroTraits[frame] portrait via WIDGET_SET_IMAGE, owner widgets 100/122/123 (pos==0) and 124/125;
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
        msg.m_extra = widget::WIDGET_DRAWN;
        m_win->broadcastMessage(msg);
    } else {
        // Mac writes SET_IMAGE before loading the portrait at 0x19b410..0x19b42c.
        // Keep the direct assignment and both broadcasts (0x19b434/0x19b450);
        // an early portrait-name local changes Windows scheduling and cleanup.
        msg.m_codeX = widget::WIDGET_SET_IMAGE;
        msg.m_extraText = g_heroTraits[frame].m_largePortraitName;
        m_win->broadcastMessage(msg);
        msg.m_codeX = widget::WIDGET_SET_STATUS;
        msg.m_extra = widget::WIDGET_DRAWN;
        m_win->broadcastMessage(msg);
    }
    msg.m_codeY = 125;
    msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
    msg.m_extra = widget::WIDGET_DRAWN;
    m_win->broadcastMessage(msg);
}

VA(0x005aa230, 0xEE)
DC_ADDRESS(0x158c00, 0xba)
MAC_ADDRESS(0x19b490, 0x13c)
void strip::drawMonster(int i, int frame)
{
    message msg;
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
DC_ADDRESS(0x158cbc, 0x78)
MAC_ADDRESS(0x19b5cc, 0xac)
void strip::drawSelector(int i)
{
    message msg;
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
