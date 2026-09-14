// towngatewindow.cpp - E:\gamedcs\towngatewindow.cpp (compiland towngatewindow.obj)
#include <va.h>
#include <string.h>
#include "towngatewindow.h"
#include "armygrp.h"
#include "border.h"
#include "button.h"
#include "game.h"
#include "iconwdgt.h"
#include "kb.h"
#include "message.h"
#include "slider.h"
#include "textresource.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// Every retail reference to this slot is in towngatewindow.obj. Dreamcast's
// constructor line 41 is the matching source-private current-window store.
DATA(0x006aa608) static TownGateWindow* g_townGateWindow;

static void townGateSliderCallback(int state, heroWindow* parentWindow);

VA(0x005c1ab0, 0x882)  // dc 0x1690b0
TownGateWindow::TownGateWindow(bool adventureSpell)
  : CAdvPopup(247, 65, 306, 469, 18),
    m_topTown(0), m_selectedTown(-1), m_adventureSpell(adventureSpell)
{
    g_townGateWindow = this;

    const char* title = m_adventureSpell
        ? g_spellTraits[SPELL_TOWN_PORTAL].m_name : 0;
    const char* selectTown = m_adventureSpell
        ? g_generalText->getText(687) : 0;

    m_towns.reserve(72 * g_game->getNumAllies(g_netLocalGamePos));
    m_widgets.reserve(17);

    m_widgets.push_back(new bitmapBorder(
        0, 0, m_width, m_height, BACKGROUND_ID, "TPGate.pcx", 0x800));
    m_widgets.push_back(new textWidget(
        0, 15, m_width, 30, title, "bigfont.fnt", font::HEADING,
        TITLE_TEXT_ID, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        14, 125, 262, 20, selectTown, "smalfont.fnt", font::PRIMARY,
        SELECT_TEXT_ID, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        14, 155, 262, 20, 0, "smalfont.fnt", font::PRIMARY,
        TOWN_0_ID, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        14, 180, 262, 20, 0, "smalfont.fnt", font::PRIMARY,
        TOWN_1_ID, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        14, 205, 262, 20, 0, "smalfont.fnt", font::PRIMARY,
        TOWN_2_ID, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        14, 230, 262, 20, 0, "smalfont.fnt", font::PRIMARY,
        TOWN_3_ID, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        14, 255, 262, 20, 0, "smalfont.fnt", font::PRIMARY,
        TOWN_4_ID, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        14, 280, 262, 20, 0, "smalfont.fnt", font::PRIMARY,
        TOWN_5_ID, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        14, 305, 262, 20, 0, "smalfont.fnt", font::PRIMARY,
        TOWN_6_ID, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        14, 330, 262, 20, 0, "smalfont.fnt", font::PRIMARY,
        TOWN_7_ID, 1, 0, 8));
    m_widgets.push_back(new textWidget(
        14, 355, 262, 20, 0, "smalfont.fnt", font::PRIMARY,
        TOWN_8_ID, 1, 0, 8));
    m_widgets.push_back(new bitmapBorder(
        14, 151, 262, 24, SELECTOR_ID, "TPGateS.pcx", 0x800));

    if (m_adventureSpell) {
        m_widgets.push_back(new iconWidget(
            111, 44, 83, 61, ICON_ID, "SpellScr.def",
            9, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
    } else {
        m_widgets.push_back(new iconWidget(
            78, 40, 150, 70, ICON_ID, "hallinfr.def",
            22, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
    }

    m_widgets.push_back(new slider(
        277, 120, 16, 258, SLIDER_ID, 10, townGateSliderCallback,
        slider::BROWN, NUM_TOWN_ENTRIES, 0));
    m_widgets.push_back(new button(
        15, 402, 64, 30, 0x7802, "iOkay.def", 0, 1, 0, 28, 2));
    m_widgets.push_back(new button(
        228, 402, 64, 30, 0x7801, "iCancel.def", 0, 1, 0, 1, 2));

    for (std::vector<widget*>::iterator it = m_widgets.begin();
         it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    message msg;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_PLAYER_PALETTE_COLORS;
    msg.m_codeY = 0;
    msg.m_extra = g_game->getLocalPlayerGamePos();
    broadcastMessage(msg);

    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
    msg.m_codeY = SELECTOR_ID;
    msg.m_extra = widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;
    broadcastMessage(msg);

    m_exitCommand = -1;
}

VA_COMPGEN(0x005c2340, 0x21, SCALAR_DELETING_DTOR, TTownGateWindow)

VA(0x005c2370, 0x8f)  // dc 0x169824
TownGateWindow::~TownGateWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

VA(0x005c2400, 0x1AF)  // dc 0x169890
void TownGateWindow::addTown(int newTown)
{
    m_towns.push_back(newTown);
}

VA(0x005c25b0, 0x1B5)  // dc 0x1698ac
void TownGateWindow::updateTownLocator(int i)
{
    message msg;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeY = TOWN_0_ID + i;
    msg.m_extra = widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;

    if (m_topTown + i >= m_towns.size()) {
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
    } else {
        msg.m_codeX = widget::WIDGET_SET_STATUS;
        broadcastMessage(msg);

        strcpy(g_text, g_game->getTownName(m_towns[m_topTown + i]));
        msg.m_codeX = widget::WIDGET_SET_TEXT;
        msg.m_extraText = g_text;
        broadcastMessage(msg);

        msg.m_codeX = widget::WIDGET_SET_COLOR;
        const Town* whichTown = g_game->getTown(m_towns[m_topTown + i]);
        if (((whichTown->m_active & g_bitNumber[EXTRA_1_ID]) != 0 || m_adventureSpell)
            && whichTown->m_visitingHeroId < 0) {
            msg.m_extra = font::PRIMARY;
            if (m_topTown + i == m_selectedTown) {
                broadcastMessage(msg);
                msg.m_codeX = widget::WIDGET_SET_STATUS;
                msg.m_codeY = SELECTOR_ID;
                msg.m_extra = widget::WIDGET_DRAWN;
                broadcastMessage(msg);
                msg.m_codeX = widget::WIDGET_SET_Y;
                msg.m_extra = 25 * i + 151;
            }
        } else {
            msg.m_extra = font::PRIMARY_HIGHLIGHT;
            broadcastMessage(msg);
            msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
            msg.m_extra = widget::WIDGET_ACTIVE;
        }
    }

    broadcastMessage(msg);
}

// E:\gamedcs\towngatewindow.cpp:168
// Complete has no retained body: VC6 expands this ordinary source helper in
// DoModal, WindowHandler, and TownGateSliderCallback. Dreamcast preserves the
// call boundary and one loop scope; both builds agree on every operation.
void TownGateWindow::updateTownLocators()
{
    message msg;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
    msg.m_codeY = SELECTOR_ID;
    msg.m_extra = widget::WIDGET_DRAWN;
    broadcastMessage(msg);

    for (int i = 0; i < NUM_TOWN_ENTRIES; ++i)
        updateTownLocator(i);

    drawWindow(1, 0xffff0001, 0xffff);
}

VA(0x005c2770, 0xC8)  // dc 0x169a88
void TownGateWindow::doModal()
{
    message msg;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_SLIDER_RESOLUTION;
    msg.m_codeY = SLIDER_ID;
    msg.m_extra = m_towns.size() - (NUM_TOWN_ENTRIES - 1);
    broadcastMessage(msg);

    updateTownLocators();

    getWidget(DIALOG_RETURN_OK)->enable(0);
    heroWindow::doModal(0);
}

// E:\gamedcs\towngatewindow.cpp:208
// The two widget verdicts this window answers: a list-row select moves the
// highlight, and a button deselect stamps gpWindowManager->dialogReturn with
// the chosen town id (or -1 for Cancel) and rewrites the message into the
// shared END_DIALOG forward. Every state read goes through the file-local
// window pointer, not `this` - only GetWidget uses the receiver.
// The END_DIALOG forward is written out in BOTH deselect arms. Written
// once after the inner switch it is a two-predecessor join, and VC6 sinks
// it past the CANCEL arm while retail lets the OK arm fall straight into
// it and reaches it from CANCEL with a backward jmp; the duplicate leaves
// the first copy single-predecessor, and the cross-jumper then merges the
// shared tail into retail's exact layout. This also dissolved what a
// standing note here called a pure register permutation (retail homes msg
// in ESI, `this` in EDI, the zero in EBX): the whole allocation falls into
// line behind the block order. 69.8969 -> 100.0000, 2026-09-05. A `goto`
// into the OK arm expresses the same merge and is byte-flat with the
// unduplicated form - only the duplicate moves it.
VA(0x005c2840, 0x13B)  // anchor-vtable (slot 9) + CAdvPopup::WindowHandler, dc 0x169ae0
int TownGateWindow::windowHandler(message& msg)
{
    int handled = CAdvPopup::windowHandler(msg);
    if (handled != 0)
        return handled;

    if (msg.m_id == MESSAGE_WIDGET) {
        switch (msg.m_codeX) {
        case widget::WIDGET_SELECT:
            if (msg.m_codeY >= TOWN_0_ID && msg.m_codeY <= TOWN_8_ID) {
                g_townGateWindow->m_selectedTown =
                    g_townGateWindow->m_topTown + msg.m_codeY - TOWN_0_ID;
                if (g_currentPlayer->isLocalHuman())
                    getWidget(DIALOG_RETURN_OK)->enable(1);
                g_townGateWindow->updateTownLocators();
            }
            break;
        case widget::WIDGET_DESELECT:
            switch (msg.m_codeY) {
            case DIALOG_RETURN_CANCEL:
                g_windowManager->m_dialogReturn = -1;
                msg.m_codeX = msg.m_codeY = widget::WIDGET_END_DIALOG;
                return MESSAGE_DISPATCH_FORWARD;
            case DIALOG_RETURN_OK:
                g_windowManager->m_dialogReturn =
                    g_townGateWindow->m_towns[g_townGateWindow->m_selectedTown];
                msg.m_codeX = msg.m_codeY = widget::WIDGET_END_DIALOG;
                return MESSAGE_DISPATCH_FORWARD;
            default:
                return MESSAGE_DISPATCH_CONSUME;
            }
        }
    }

    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x005c2980, 0x89)  // dc 0x169ba8
static void townGateSliderCallback(int state, heroWindow* parentWindow)
{
    g_townGateWindow->m_topTown = state;
    g_townGateWindow->updateTownLocators();
    g_townGateWindow->drawWindow(1, 0xffff0001, 0xffff);
}

#if 0  // @carcass

// E:\gamedcs\game.h:897
DC_ONLY(0x169c0c, 0x54)
unsigned char game::getNumAllies(int playerNum)
{
    // @stub
}

// E:\gamedcs\game.h:1022
DC_ONLY(0x169c60, 0x1C)
const Town* game::getTown(int which)
{
    // @stub
}

// E:\gamedcs\game.h:1027
DC_ONLY(0x169c7c, 0x1C)
const char* game::getTownName(int iTownId)
{
    // @stub
}

// E:\gamedcs\towngatewindow.cpp:98
DC_ONLY(0x169c98, 0x34)
void* TownGateWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// ..\stlport\stl_vector.c:68
DC_ONLY(0x169ccc, 0x94)
void std::vector<int,std::allocator<int> >::reserve(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:199
DC_ONLY(0x169d60, 0xC)
unsigned std::vector<int,std::allocator<int> >::capacity()
{
    // @stub
}

// ..\stlport\stl_vector.h:514
DC_ONLY(0x169d6c, 0x38)
std::vector<int,std::allocator<int> >::_M_allocate_and_copy(unsigned __n, int* __first, int* __last)
{
    // @stub
}

#endif  // @carcass
