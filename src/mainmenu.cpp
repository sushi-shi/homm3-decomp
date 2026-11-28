// Retail contribution (inside the lodfile->mapcell gap 0xfac34..0xfbf90):
//   terrain.h $E head 0xfaeb0..0xfb29f (12 bitset<10> init/atexit thunks, excluded
//   class), then 5 real rows 0xfb2a0..0xfbb94; mapcell's own $E head follows at
//   0xfbba0. The row at 0xfac40 is NOT mainmenu: it is lodfile's STL-COMDAT tail
//   (vector<LODEntry> insert, 32-byte elements, sole caller LODFile::open).
// VideomodeChoice (5 DC rows, 0xea9b0..0xeb370) has no retail slot: the retail
//   contribution is fully accounted for and its non-COMDAT globals would have to
//   sit in this run -> DC-port-only class; recorded unlocated, not forced.
#include "va.h"
#include "text.h"

// DC S_LPROC32: TU-local. Declared before the header so the friend binds to it.
class message;
static int mainMenuHandler(message& msg);
#include "mainmenu.h"

#include "button.h"
#include "dxplay.h"
#include "exec.h"
#include "gametypewindow.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "remote.h"
#include "smackmgr.h"
#include "soundmgr.h"
#include "terrain.h"
#include "widget.h"
#include "winmgr.h"

// Retail scalar state; startup initial values come from the pinned image.
DATA(0x0069957c) int g_cdRomErr;

// Set after the one-time missing-CD notice has been shown. The constructor
// uses it only as the persistent suppression latch; the disk-space check has
// its own static local in mainMenuHandler.
DATA(0x00699660) static TMainMenu* g_mainMenu;
DATA(0x00699674) static unsigned char g_cdMessageShown;
DATA(0x00699678) static unsigned long g_lastDiskSpaceCheck;

// SetupCDDrive's result is stored by kb.obj's startup path and consumed here
// to select the localized missing-CD wording. No public DC name survives.


DATA(0x0063ff28)
static const TMainMenuButtonRect g_mainMenuButtonRects[5] = {
    {540,  10, 207, 121},
    {532, 132, 226, 120},
    {524, 251, 239, 106},
    {557, 359, 173, 110},
    {586, 469, 114, 102}
};

VA(0x004fb2a0, 0x385)
DC_ADDRESS(0x0ea2ec, 0x28c)
MAC_ADDRESS(0x11c674, 0x550)
TMainMenu::TMainMenu()
    : heroWindow(0, 0, 800, 600, 0)
{
    g_mainMenu = this;
    m_showCdMessage = g_noCdRom && !g_cdMessageShown;

    m_widgets.reserve(NWIDGETS);
    m_widgets.push_back(new button(
        g_mainMenuButtonRects[0].m_x, g_mainMenuButtonRects[0].m_y,
        g_mainMenuButtonRects[0].m_width, g_mainMenuButtonRects[0].m_height,
        NEW_GAME_ID, "mmenung.def", 0, 1, false, 49, 2));
    m_widgets.push_back(new button(
        g_mainMenuButtonRects[1].m_x, g_mainMenuButtonRects[1].m_y,
        g_mainMenuButtonRects[1].m_width, g_mainMenuButtonRects[1].m_height,
        LOAD_GAME_ID, "mmenulg.def", 0, 1, false, 38, 2));
    m_widgets.push_back(new button(
        g_mainMenuButtonRects[2].m_x, g_mainMenuButtonRects[2].m_y,
        g_mainMenuButtonRects[2].m_width, g_mainMenuButtonRects[2].m_height,
        HIGH_SCORE_ID, "mmenuhs.def", 0, 1, false, 35, 2));
    m_widgets.push_back(new button(
        g_mainMenuButtonRects[3].m_x, g_mainMenuButtonRects[3].m_y,
        g_mainMenuButtonRects[3].m_width, g_mainMenuButtonRects[3].m_height,
        CREDITS_ID, "mmenucr.def", 0, 1, false, 46, 2));
    m_widgets.push_back(new button(
        g_mainMenuButtonRects[4].m_x, g_mainMenuButtonRects[4].m_y,
        g_mainMenuButtonRects[4].m_width, g_mainMenuButtonRects[4].m_height,
        QUIT_ID, "mmenuqt.def", 0, 1, false, 1, 2));

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    if (g_lobbyLaunched) {
        if (g_dPlay && g_dPlay->isHost()) {
            // DC mainmenu.cpp:102/103 calls widget::hide at both sites.
            getWidget(HIGH_SCORE_ID)->hide();
            getWidget(CREDITS_ID)->hide();
        }
        g_lastDiskSpaceCheck = GameTime::get();
    }
}

VA_COMPGEN(0x004fb630, 0x21, SCALAR_DELETING_DTOR, TMainMenu)

VA(0x004fb660, 0x75)
DC_ADDRESS(0x0ea578, 0x72)
MAC_ADDRESS(0x11cbc4, 0xb4)
TMainMenu::~TMainMenu()
{
    g_mainMenu = NULL;
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

VA(0x004fb6e0, 0x2C)
DC_ADDRESS(0x0ea5ec, 0x2c)
MAC_ADDRESS(0x11cc78, 0x58)
void TMainMenu::doModal()
{
    g_soundManager->startMP3("MainMenu", 0, 1);
    g_windowManager->doDialog(this, mainMenuHandler, 0);
}

// E:\gamedcs\mainmenu.cpp:135
// 90.0704 -> 93.1606: both CD-dialog arms pass format_string's temporary
// directly to NormalDialog; caching gpGeneralText or naming either
// std::string creates the wrong live ranges/stack slots.
// The wrong-edition arm (CD drive 5/6) comes first: retail's unwind map gives
// that arm's temporary EH state 0 and the generic arm's state 1, and Mac's
// specific-edition arm also precedes its generic warning (which on Mac
// passes the text without format_string). Adopting that order moved MAX
// 94.08 -> 93.48 (states and block layout agree).
// Mac 0x11ccf0..0x11cd28 guards two function-local statics around the two
// flag initializers: lastIMHoverID (-1), the flags, then check_disk_space
// (Dreamcast prints both names undecorated). Mac 0x11d058 switches on the
// widget command and leaves the declined quit with a break; its tail at
// 0x11d1d0..0x11d22c sets the exit flag on the lobby timeout and tests it
// again for the one end-dialog return. Dreamcast's only stack local,
// updatePlease (sp+0x33), is the redraw flag set at line 257 and read with
// VideoNeedsUpdate at 276; the exit flag is its r9 and has no recorded name.
// These forms are byte-identical to the earlier `confirmed` flag and
// early-return tail under VC6 (93.48%).
// Residual: retail keeps both formatString temporaries in one slot
// ([ebp-0x20], frame 0x14) and drops both destructors' _Tidy zeroing stores;
// this source gets two slots (frame 0x24), keeps the zeroing stores of the
// generic arm and caches zero in ESI. VC6 gives the last-created
// stack-memory local its own bin (docs/vc6/regalloc.md): any later local
// whose address is taken, such as a pointer or reference bound to helpID,
// hoverID, id or w, makes the two temporaries share and the function exact,
// with no other byte moving. Neither Dreamcast nor Mac shows such a local,
// so that form lives on the levers branch. With Mac's unformatted generic
// arm compiled for Mac, its frame (0xe0) and the one temporary's slot (0xa8)
// equal retail's, so the Mac build has no further memory local and the
// missing one is in Windows-only code. Not triggers: unused locals,
// the static locals, the address of a flag or of fill, named strings
// (90.3%), braces, else-if nesting (91.7%), a switch on the drive (86.5%),
// ternaries (78.4..91.6%), an inline const-reference dialog wrapper.
VA(0x004fb710, 0x484)
DC_ADDRESS(0x0ea618, 0x398)
MAC_ADDRESS(0x11ccd0, 0x574)  // admitted row includes the jump table/padding; decoded body ends at +0x46d
static int mainMenuHandler(message& msg)
{
    // Original DC names: lastIMHoverID, check_disk_space.
    DATA(0x0067fa60) static int lastImHoverId = -1;
    unsigned char done = 0;
    unsigned char updatePlease = 0;
    DATA(0x0067fa64) static unsigned char checkDiskSpace = 1;

    if (checkDiskSpace) {
        if (getAvailableDiskSpace() < 5 * 1024 * 1024) {
            normalDialog((*g_generalText)[GENERAL_TEXT_MAIN_MENU_LOW_DISK],
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            done = 1;
            g_windowManager->m_dialogReturn = TMainMenu::QUIT_ID;
        }
        checkDiskSpace = 0;
    }

    if (g_mainMenu->m_showCdMessage && !done) {
        const char* fill = (*g_generalText)[GENERAL_TEXT_SHADOW_OF_DEATH];

        g_mainMenu->drawWindow(true, WINDOW_ALL_WIDGETS_LOW,
                               WINDOW_ALL_WIDGETS_HIGH);
        if (g_cdRomErr == CD_ROM_RESTORATION_OF_ERATHIA_DISC ||
            g_cdRomErr == CD_ROM_ARMAGEDDONS_BLADE_DISC) {
            const char* drive = g_cdRomErr == CD_ROM_RESTORATION_OF_ERATHIA_DISC
                ? (*g_generalText)[GENERAL_TEXT_RESTORATION_OF_ERATHIA]
                : (*g_generalText)[GENERAL_TEXT_ARMAGEDDONS_BLADE];
            normalDialog(formatString(
                (*g_generalText)[GENERAL_TEXT_WRONG_CD_EDITION_FORMAT],
                drive, fill, fill, fill, fill).c_str(),
                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        } else {
#if defined(HOMM3_TARGET_MAC)
            // Mac 0x11cec8..0x11cf10 passes the generic text unformatted.
            normalDialog(
                (*g_generalText)[GENERAL_TEXT_CD_REQUIRED_GENERIC_FORMAT],
                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
#else
            normalDialog(formatString(
                (*g_generalText)[GENERAL_TEXT_CD_REQUIRED_GENERIC_FORMAT],
                fill, fill, fill, fill).c_str(),
                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
#endif
        }
        g_cdMessageShown = 1;
        g_mainMenu->m_showCdMessage = 0;
    }

    pollSound();
    if (g_windowManager->m_isWaitingForFadeIn)
        g_windowManager->fadeScreen(0, 4, 0);

    if (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT) {
        if ((msg.m_codeX == widget::WIDGET_SELECT ||
             msg.m_codeX == widget::WIDGET_RIGHT_SELECT)) {
            int helpID;
            switch (msg.m_codeY) {
            case TMainMenu::NEW_GAME_ID:  helpID = 0; break;
            case TMainMenu::LOAD_GAME_ID: helpID = 1; break;
            case TMainMenu::HIGH_SCORE_ID: helpID = 2; break;
            case TMainMenu::CREDITS_ID: helpID = 3; break;
            case TMainMenu::QUIT_ID: helpID = 4; break;
            default: helpID = -1; break;
            }
            if (helpID >= 0 && !g_lobbyLaunched)
                normalDialog(g_mainMenuHelp[helpID].m_rclick, 4, -1, -1,
                             -1, 0, -1, 0, -1, 0, -1, 0);
        }
    } else if (msg.m_id == MESSAGE_WIDGET) {
        if (msg.m_codeY < TMainMenu::NEW_GAME_ID ||
            msg.m_codeY > TMainMenu::QUIT_ID)
            return 0;

        switch (msg.m_codeX) {
        case widget::WIDGET_DESELECT:
            if (msg.m_codeY == TMainMenu::QUIT_ID) {
                videoPause();
                if (!g_lobbyLaunched) {
                    normalDialog((*g_generalText)[GENERAL_TEXT_QUIT],
                                 2, -1, -1, -1, 0, -1, 0,
                                 -1, 0, -1, 0);
                    videoResume();
                    if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT) {
                        done = 0;
                        break;
                    }
                }
            }
            done = 1;
            g_windowManager->m_dialogReturn = msg.m_codeY;
            break;
        }
    } else if (msg.m_id == MESSAGE_MOUSE_MOVE) {
        int hoverID = g_mainMenu->findWidget(msg.m_mouseX, msg.m_mouseY);
        // LEVER (owner decision): binding a reference to this later local
        // makes it a stack-memory symbol for C2, which is promoted again but
        // lets the two formatString temporaries above share [ebp-0x20]
        // (docs/vc6/regalloc.md). No native build shows which later local
        // had its address taken; helpID, id and w work the same way.
        const int& hoverRef = hoverID;
        if (hoverRef != lastImHoverId) {
            updatePlease = 1;
            lastImHoverId = hoverID;
            for (int id = TMainMenu::NEW_GAME_ID;
                 id <= TMainMenu::QUIT_ID; ++id) {
                widget* w = g_mainMenu->getWidget(id);
                if (w)
                    w->sendMessage(widget::WIDGET_CLEAR_STATUS,
                                    widget::WIDGET_HIGHLIGHTED);
            }
            if (hoverID != -1) {
                g_mainMenu->getWidget(hoverID)->sendMessage(
                    widget::WIDGET_SET_STATUS, widget::WIDGET_HIGHLIGHTED);
            }
        }
    }

    if (videoNeedsUpdate() || updatePlease) {
        g_mainMenu->drawWindow(false, TMainMenu::NEW_GAME_ID,
                               TMainMenu::QUIT_ID);
        g_windowManager->updateScreen(520, 4, 250, 567);
        videoDrawRects();
    }

    if (!done) {
        if (g_lobbyLaunched) {
            if (GameTime::elapsedSince(g_lastDiskSpaceCheck) > 10000) {
                g_windowManager->m_dialogReturn = TMainMenu::NEW_GAME_ID;
                done = 1;
            }
        }
    }

    if (done) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}
