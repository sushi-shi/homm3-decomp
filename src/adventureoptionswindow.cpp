// adventureoptionswindow.cpp - E:\gamedcs\adventureoptionswindow.cpp
// (compiland adventureoptionswindow.obj)
#include <va.h>
#include "adventureoptionswindow.h"
#include "border.h"
#include "button.h"
#include "game.h"
#include "kb.h"
#include "message.h"
#include "mousemgr.h"
#include "soundmgr.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// DC public gAdventureOptionsHelp; retail consumers prove seven THelpText
// rows at 0x6a6530 (the sixth row is unused by this dialog's ID mapping).
// Do not conflate it with Dreamcast's separate gAdventureWindowHelp table.
DATA(0x006a6530) extern THelpText g_adventureOptionsHelp[7];

// File-static hover latch: the retail initializer at 0x65f46c is -1 and the
// handler is its only image-wide reader/writer.
DATA(0x0065f46c) static int g_lastImHoverId = -1;

VA(0x004051d0, 0x4AA)  // dc 0x4cf4
TAdventureOptionsWindow::TAdventureOptionsWindow()
    : CAdvPopup(255, 106, 289, 387, 0x12)
{
    m_widgets.reserve(8);

    bitmapBorder* background = new bitmapBorder(
        0, 0, 289, 387, ADVENTURE_OPTION_BACKGROUND_ID,
        "AdvOpts.pcx", 0x800);
    background->setPlayerPaletteColors(g_game->getLocalPlayerGamePos());
    m_widgets.push_back(background);

    button* action = new button(
        25, 24, 49, 51, VIEW_WORLD_ID,
        "AdvView.def", 0, 1, 0, 0, 2);
    action->setHotkey(ADVENTURE_OPTION_VIEW_HOTKEY);
    m_widgets.push_back(action);

    action = new button(
        25, 82, 49, 51, VIEW_PUZZLE_ID,
        "AdvPuz.def", 0, 1, 0, 0, 2);
    action->setHotkey(ADVENTURE_OPTION_PUZZLE_HOTKEY);
    m_widgets.push_back(action);

    action = new button(
        25, 140, 49, 51, DIG_ID,
        "AdvDig.def", 0, 1, 0, 0, 2);
    action->setHotkey(ADVENTURE_OPTION_DIG_HOTKEY);
    m_widgets.push_back(action);

    action = new button(
        25, 198, 49, 51, VIEW_SCENARIO_ID,
        "AdvInfo.def", 0, 1, 0, 0, 2);
    action->setHotkey(ADVENTURE_OPTION_INFO_HOTKEY);
    m_widgets.push_back(action);

    action = new button(
        25, 256, 49, 51, REPLAY_ID,
        "AdvTurn.def", 0, 1, 0, 0, 2);
    action->enable(g_game->replayAvailable());
    action->setHotkey(ADVENTURE_OPTION_TURN_HOTKEY);
    m_widgets.push_back(action);

    button* accept = new button(
        203, 313, 64, 32, ADVENTURE_OPTION_ACCEPT_ID,
        "iOk6432.def", 0, 1, 1, 0, 2);
    accept->setHotkey(ADVENTURE_OPTION_ACCEPT_HOTKEY_1);
    accept->setHotkey(ADVENTURE_OPTION_ACCEPT_HOTKEY_2);
    m_widgets.push_back(accept);

    m_rolloverWidget = new textWidget(
        6, 360, 275, 20, "", "smalfont.fnt", font::PRIMARY,
        ADVENTURE_OPTION_ROLLOVER_ID, 5, 0, 8);
    m_widgets.push_back(m_rolloverWidget);

    widget** first = m_widgets.begin();
    if (first != m_widgets.end()) {
        widget** it = m_widgets.begin();
        do {
            if (*it)
                addWidget(*it, -1);
            else
                memError();
        } while (++it != m_widgets.end());
    }

    if (g_game->getCurrHeroId() == -1) {
        widget* dig = getWidget(DIG_ID);
        dig->enable(0);
    }

    if (!g_currentPlayer->isLocalHuman()) {
        widget* turn = getWidget(REPLAY_ID);
        turn->enable(0);
        widget* dig = getWidget(DIG_ID);
        dig->enable(0);
    }
}

VA(0x00405680, 0x10)
int CHeroWindowEx::handleMessage(message& msg)
{
    return windowHandler(&msg);
}

VA_COMPGEN(0x00405690, 0x21, SCALAR_DELETING_DTOR, TAdventureOptionsWindow)

VA(0x004056c0, 0x6B)  // dc 0x514c
TAdventureOptionsWindow::~TAdventureOptionsWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// E:\gamedcs\adventureoptionswindow.cpp:112
// Dreamcast emits this const helper out of line. Retail's handler contains
// both calls inline and the 0x405680 slot is independently proved to be
// CHeroWindowEx's handle_message forwarder, so the helper has no retail claim.
// DC 0x51b0 proves the early negative return followed by a switch assigning
// one result for the final return. Complete adds the upper-bound fast reject;
// with both bounds owned here, retail corroborates both inlined lowerings.
DC_ONLY(0x51b0, 0x54)
int TAdventureOptionsWindow::convertID2HelpID(int id) const
{
    if (id < 0)
        return -1;
    if (id > ADVENTURE_OPTION_ACCEPT_ID)
        return -1;

    int helpID;
    switch (id) {
    case VIEW_WORLD_ID: helpID = 0; break;
    case VIEW_PUZZLE_ID: helpID = 1; break;
    case VIEW_SCENARIO_ID: helpID = 2; break;
    case DIG_ID: helpID = 3; break;
    case REPLAY_ID: helpID = 4; break;
    case ADVENTURE_OPTION_ACCEPT_ID: helpID = 6; break;
    default: helpID = -1; break;
    }
    return helpID;
}

VA(0x00405730, 0x1FC)  // dc 0x5204
int TAdventureOptionsWindow::windowHandler(message* msg)
{
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;

    unsigned char closeDialog = false;
    pollSound();

    if (msg->m_qualifier & MESSAGE_MODIFIER_RIGHT) {
        if (msg->m_codeX == widget::WIDGET_SELECT
            || msg->m_codeX == widget::WIDGET_RIGHT_SELECT) {
            int helpID = convertID2HelpID(msg->m_codeY);
            if (helpID != -1)
                normalDialog(g_adventureOptionsHelp[helpID].m_text,
                    4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
    } else if (msg->m_id == MESSAGE_WIDGET) {
        if (msg->m_codeX == widget::WIDGET_DESELECT
            && (msg->m_codeY == ADVENTURE_OPTION_ACCEPT_ID
                || (msg->m_codeY > 0 && msg->m_codeY <= 5))) {
            closeDialog = true;
        }
    } else if (msg->m_id == MESSAGE_MOUSE_MOVE) {
        int hoverID = findWidget(msg->m_mouseX, msg->m_mouseY);
        if (hoverID != g_lastImHoverId) {
            g_lastImHoverId = hoverID;
            const char* rollover = "";
            if (hoverID != -1) {
                g_mouseManager->setPointer(1, mouseManager::DEFAULT_SET);
                int helpID = convertID2HelpID(hoverID);
                if (helpID != -1)
                    rollover = g_adventureOptionsHelp[helpID].m_text;
            } else {
                g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
            }
            m_rolloverWidget->setText(rollover);
            drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        }
    }

    if (closeDialog) {
        msg->m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = msg->m_codeY;
        msg->m_codeY = widget::WIDGET_END_DIALOG;
        msg->m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }

    return MESSAGE_DISPATCH_CONSUME;
}
