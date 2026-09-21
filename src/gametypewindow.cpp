#include "gametypewindow.h"

#include "border.h"
#include "button.h"
#include "game.h"
#include "kb.h"
#include "message.h"
#include "smackmgr.h"
#include "soundmgr.h"
#include "va.h"
#include "widget.h"
#include "winmgr.h"

// Source-private in the Dreamcast compiland. Retail's constructor stores the
// active dialog here and its destructor clears it after deleting the widgets.
DATA(0x006972d8) static TGameTypeWindow* g_gameTypeWindow;

// Help.txt initialization fills this five-row pair table. The Dreamcast
// public calls it gNewGameHelp, and the retail handler's 8*id indexing proves
// the THelpText stride and the [0] field used by NormalDialog.
DATA(0x006a6bfc) extern THelpText g_newGameHelp[5];

// Dreamcast publishes gbNoCDRom, and retail oldmain writes the same address
// from SetupCDRom before both front-end menus consume it.
DATA(0x00698a2c) extern int g_noCdRom;

// Source-private, DC-attested local name; retail data contains the -1
// initializer and this handler is its only image-wide consumer.
DATA(0x006780f0) static int g_lastImHoverId = -1;

DATA(0x0063e6b0)
static const TGameTypeButtonRect g_gameTypeButtonRects[5] = {
    {545,   4, 209, 123},
    {568, 120, 162, 120},
    {541, 233, 211, 131},
    {545, 358, 205, 108},
    {582, 464, 127, 107}
};

DATA(0x006780e8)
static const char* g_gameTypeBackgrounds[2] = {
    "newgame.pcx", "loadgame.pcx"
};

VA(0x004d54c0, 0x39B)  // dc 0xc9164
TGameTypeWindow::TGameTypeWindow(unsigned char loadGameMode)
    : heroWindow(0, 0, 800, 600, 0)
{
    g_gameTypeWindow = this;
    g_game->m_isTutorial = 0;

    m_widgets.reserve(NWIDGETS);
    m_widgets.push_back(new bitmapBorder16(
        114, 312, 300, 48, NEW_LOAD_ID,
        g_gameTypeBackgrounds[loadGameMode], 0x800));

    const TGameTypeButtonRect& single = g_gameTypeButtonRects[0];
    const TGameTypeButtonRect& multi = g_gameTypeButtonRects[1];
    const TGameTypeButtonRect& campaign = g_gameTypeButtonRects[2];
    const TGameTypeButtonRect& tutorial = g_gameTypeButtonRects[3];
    const TGameTypeButtonRect& back = g_gameTypeButtonRects[4];

    if (!g_noCdRom) {
        m_widgets.push_back(new button(
            single.m_x, single.m_y, single.m_width, single.m_height, SINGLE_ID,
            "gtsingl.def", 0, 1, 0, 31, 2));

        m_widgets.push_back(new button(
            campaign.m_x, campaign.m_y, campaign.m_width, campaign.m_height,
            CAMPAIGN_ID, "gtcampn.def", 0, 1, 0, 46, 2));

        m_widgets.push_back(new button(
            tutorial.m_x, tutorial.m_y, tutorial.m_width, tutorial.m_height,
            TUTORIAL_ID, "gttutor.def", 0, 1, 0, 20, 2));
    }

    m_widgets.push_back(new button(
        multi.m_x, multi.m_y, multi.m_width, multi.m_height,
        MULTIPLAYER_ID, "gtmulti.def", 0, 1, 0, 50, 2));

    m_widgets.push_back(new button(
        back.m_x, back.m_y, back.m_width, back.m_height, QUIT_ID,
        "gtback.def", 0, 1, 0, 1, 2));

    widget** first = m_widgets.begin();
    if (first != m_widgets.end()) {
        for (widget** it = first; it != m_widgets.end(); ++it) {
            if (*it)
                addWidget(*it, -1);
            else
                memError();
        }
    }
}

VA_COMPGEN(0x004d5860, 0x21, SCALAR_DELETING_DTOR, TGameTypeWindow)

VA(0x004d5890, 0x75)  // dc 0xc9490
TGameTypeWindow::~TGameTypeWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
    g_gameTypeWindow = 0;
}

VA(0x004d5910, 0x2C)  // dc 0xc94f8
void TGameTypeWindow::doModal()
{
    g_soundManager->startMP3(
        DATA_COMPGEN(0x00660e08, gameTypeMainMenuMusic, "MainMenu"), 0, 1);
    g_windowManager->doDialog(this, gameTypeWindowHandler, 0);
}

VA(0x004d5940, 0x220)  // dc 0xc9524
int gameTypeWindowHandler(message& msg)
{
    unsigned char exitFlag = 0;
    unsigned char redraw = 0;

    pollSound();

    if (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT) {
        if (msg.m_codeX == widget::WIDGET_SELECT
            || msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
            int helpIndex;
            switch (msg.m_codeY) {
            case TGameTypeWindow::SINGLE_ID:
                helpIndex = 0;
                break;
            case TGameTypeWindow::CAMPAIGN_ID:
                helpIndex = 1;
                break;
            case TGameTypeWindow::MULTIPLAYER_ID:
                helpIndex = 2;
                break;
            case TGameTypeWindow::TUTORIAL_ID:
                helpIndex = 3;
                break;
            case TGameTypeWindow::QUIT_ID:
                helpIndex = 4;
                break;
            default:
                helpIndex = -1;
                break;
            }
            if (helpIndex >= 0)
                normalDialog(g_newGameHelp[helpIndex].m_text,
                    4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
    } else if (msg.m_id == MESSAGE_WIDGET) {
        if (msg.m_codeX == widget::WIDGET_DESELECT
            && msg.m_codeY >= TGameTypeWindow::SINGLE_ID
            && msg.m_codeY <= TGameTypeWindow::QUIT_ID) {
            g_windowManager->m_dialogReturn = msg.m_codeY;
            exitFlag = 1;
        }
    } else if (msg.m_id == MESSAGE_MOUSE_MOVE) {
        int hoverID = g_gameTypeWindow->findWidget(msg.m_mouseX, msg.m_mouseY);
        if (hoverID != g_lastImHoverId) {
            redraw = 1;
            g_lastImHoverId = hoverID;

            if (!g_noCdRom) {
                for (int id = TGameTypeWindow::SINGLE_ID;
                     id <= TGameTypeWindow::QUIT_ID; ++id) {
                    g_gameTypeWindow->getWidget(id)->sendMessage(
                        widget::WIDGET_CLEAR_STATUS,
                        widget::WIDGET_HIGHLIGHTED);
                }
            } else {
                g_gameTypeWindow->getWidget(
                    TGameTypeWindow::MULTIPLAYER_ID)->sendMessage(
                        widget::WIDGET_CLEAR_STATUS,
                        widget::WIDGET_HIGHLIGHTED);
                g_gameTypeWindow->getWidget(
                    TGameTypeWindow::QUIT_ID)->sendMessage(
                        widget::WIDGET_CLEAR_STATUS,
                        widget::WIDGET_HIGHLIGHTED);
            }

            if (hoverID != -1) {
                g_gameTypeWindow->getWidget(hoverID)->sendMessage(
                    widget::WIDGET_SET_STATUS, widget::WIDGET_HIGHLIGHTED);
            }
        }
    }

    if (videoNeedsUpdate() || redraw) {
        g_gameTypeWindow->drawWindow(
            0, TGameTypeWindow::SINGLE_ID,
            TGameTypeWindow::NEW_LOAD_ID);
        g_windowManager->updateScreen(526, 7, 250, 560);
        g_windowManager->updateScreen(114, 312, 300, 48);
        videoDrawRects();
    }

    if (exitFlag) {
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = msg.m_codeY;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// E:\gamedcs\gametypewindow.cpp:92
// The scalar deleting destructor is represented by VA_COMPGEN above.
