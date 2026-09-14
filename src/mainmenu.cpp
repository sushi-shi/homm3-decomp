// mainmenu.cpp - E:\gamedcs\mainmenu.cpp (compiland mainmenu.obj)

// Retail contribution (inside the lodfile->mapcell gap 0xfac34..0xfbf90):
//   terrain.h $E head 0xfaeb0..0xfb29f (12 bitset<10> init/atexit thunks, excluded
//   class), then 5 real rows 0xfb2a0..0xfbb94; mapcell's own $E head follows at
//   0xfbba0. The row at 0xfac40 is NOT mainmenu: it is lodfile's STL-COMDAT tail
//   (vector<LODEntry> insert, 32-byte elements, sole caller LODFile::open).
// VideomodeChoice (5 DC rows, 0xea9b0..0xeb370) has no retail slot: the retail
//   contribution is fully accounted for and its non-COMDAT globals would have to
//   sit in this run -> DC-port-only class; recorded unlocated, not forced.
#include <va.h>
#include "mainmenu.h"
#include "button.h"
#include "dxplay.h"
#include "gametypewindow.h"
#include "exec.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "remote.h"
#include "smackmgr.h"
#include "soundmgr.h"
#include "widget.h"
#include "winmgr.h"

// DC S_LPROC32 identifies this ordinary callback as TU-local.
static int mainMenuHandler(message& msg);

// Set after the one-time missing-CD notice has been shown. The constructor
// uses it only as the persistent suppression latch; the disk-space check has
// its own DC-named static below.
DATA(0x00699660) static TMainMenu* g_mainMenu;
DATA(0x00699674) static unsigned char g_cdMessageShown;
DATA(0x00699678) static unsigned long g_lastDiskSpaceCheck;

DATA(0x0067fa60) static int g_lastImHoverId = -1;
DATA(0x0067fa64) static unsigned char g_checkDiskSpace = 1;

// SetupCDDrive's result is stored by kb.obj's startup path and consumed here
// to select the localized missing-CD wording. No public DC name survives.
DATA(0x0069957c) extern int g_cdDriveNumber;

// DC public gMainMenuHelp; InitializeHelpText fills the same five retail
// THelpText rows at this address.
DATA(0x006a6c24) extern THelpText g_mainMenuHelp[5];

DATA(0x0063ff28)
static const TMainMenuButtonRect g_mainMenuButtonRects[5] = {
    {540,  10, 207, 121},
    {532, 132, 226, 120},
    {524, 251, 239, 106},
    {557, 359, 173, 110},
    {586, 469, 114, 102}
};

VA(0x004fb2a0, 0x385)  // dc 0xea2ec
TMainMenu::TMainMenu()
    : heroWindow(0, 0, 800, 600, 0)
{
    g_mainMenu = this;
    m_showCdMessage = g_noCdRom && !g_cdMessageShown;

    m_widgets.reserve(NWIDGETS);
    m_widgets.push_back(new button(
        g_mainMenuButtonRects[0].m_x, g_mainMenuButtonRects[0].m_y,
        g_mainMenuButtonRects[0].m_width, g_mainMenuButtonRects[0].m_height,
        NEW_GAME_ID, "mmenung.def", 0, 1, 0, 49, 2));
    m_widgets.push_back(new button(
        g_mainMenuButtonRects[1].m_x, g_mainMenuButtonRects[1].m_y,
        g_mainMenuButtonRects[1].m_width, g_mainMenuButtonRects[1].m_height,
        LOAD_GAME_ID, "mmenulg.def", 0, 1, 0, 38, 2));
    m_widgets.push_back(new button(
        g_mainMenuButtonRects[2].m_x, g_mainMenuButtonRects[2].m_y,
        g_mainMenuButtonRects[2].m_width, g_mainMenuButtonRects[2].m_height,
        HIGH_SCORE_ID, "mmenuhs.def", 0, 1, 0, 35, 2));
    m_widgets.push_back(new button(
        g_mainMenuButtonRects[3].m_x, g_mainMenuButtonRects[3].m_y,
        g_mainMenuButtonRects[3].m_width, g_mainMenuButtonRects[3].m_height,
        CREDITS_ID, "mmenucr.def", 0, 1, 0, 46, 2));
    m_widgets.push_back(new button(
        g_mainMenuButtonRects[4].m_x, g_mainMenuButtonRects[4].m_y,
        g_mainMenuButtonRects[4].m_width, g_mainMenuButtonRects[4].m_height,
        QUIT_ID, "mmenuqt.def", 0, 1, 0, 1, 2));

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    if (g_dPlayReady) {
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
TMainMenu::~TMainMenu()
{
    g_mainMenu = 0;
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

VA(0x004fb6e0, 0x2C)  // dc 0xea5ec
void TMainMenu::doModal()
{
    g_soundManager->startMP3("MainMenu", 0, 1);
    g_windowManager->doDialog(this, mainMenuHandler, 0);
}

// E:\gamedcs\mainmenu.cpp:135
// 90.0704 -> 93.1606: both CD-dialog arms pass format_string's temporary
// directly to NormalDialog, so VC6 reuses one EH slot; caching gpGeneralText
// or naming either std::string creates the wrong live ranges/stack slots.
// The hover call also really passes Y then X here - retail loads +0x10 first,
// pushes it, then loads/pushes +0x14 as findWidget's first stack argument.

VA(0x004fb710, 0x484)  // admitted row includes the jump table/padding; decoded body ends at +0x46d, dc 0xea618
static int mainMenuHandler(message& msg)
{
    unsigned char updatePlease = 0;
    unsigned char hoverChanged = 0;

    if (g_checkDiskSpace) {
        if (getAvailableDiskSpace() < 5 * 1024 * 1024) {
            normalDialog(g_generalText->getText(GENERAL_TEXT_MAIN_MENU_LOW_DISK),
                         1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            updatePlease = 1;
            g_windowManager->m_dialogReturn = TMainMenu::QUIT_ID;
        }
        g_checkDiskSpace = 0;
    }

    if (g_mainMenu->m_showCdMessage && !updatePlease) {
        const char* fill = (*g_generalText)[GENERAL_TEXT_MAIN_MENU_CD_DEFAULT_ARGUMENT];

        g_mainMenu->drawWindow(1, WINDOW_ALL_WIDGETS_LOW,
                               WINDOW_ALL_WIDGETS_HIGH);
        if (g_cdDriveNumber != CD_DRIVE_NUMBER_5 &&
            g_cdDriveNumber != CD_DRIVE_NUMBER_6) {
            normalDialog(formatString(
                (*g_generalText)[GENERAL_TEXT_MAIN_MENU_CD_GENERIC_FORMAT],
                fill, fill, fill, fill).c_str(),
                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        } else {
            const char* drive = g_cdDriveNumber == CD_DRIVE_NUMBER_5
                ? (*g_generalText)[GENERAL_TEXT_MAIN_MENU_CD_DRIVE_5]
                : (*g_generalText)[GENERAL_TEXT_MAIN_MENU_CD_DRIVE_6];
            normalDialog(formatString(
                (*g_generalText)[GENERAL_TEXT_MAIN_MENU_CD_DRIVE_FORMAT],
                drive, fill, fill, fill, fill).c_str(),
                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
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
            if (helpID >= 0 && !g_dPlayReady)
                normalDialog(g_mainMenuHelp[helpID].m_text, 4, -1, -1,
                             -1, 0, -1, 0, -1, 0, -1, 0);
        }
    } else if (msg.m_id == MESSAGE_WIDGET) {
        if (msg.m_codeY < TMainMenu::NEW_GAME_ID ||
            msg.m_codeY > TMainMenu::QUIT_ID)
            return 0;

        if (msg.m_codeX == widget::WIDGET_DESELECT) {
            bool confirmed = 1;
            if (msg.m_codeY == TMainMenu::QUIT_ID) {
                videoPause();
                if (!g_dPlayReady) {
                    normalDialog((*g_generalText)[GENERAL_TEXT_QUIT],
                                 2, -1, -1, -1, 0, -1, 0,
                                 -1, 0, -1, 0);
                    videoResume();
                    if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT) {
                        updatePlease = 0;
                        confirmed = 0;
                    }
                }
            }
            if (confirmed) {
                updatePlease = 1;
                g_windowManager->m_dialogReturn = msg.m_codeY;
            }
        }
    } else if (msg.m_id == MESSAGE_MOUSE_MOVE) {
        int hoverID = g_mainMenu->findWidget(msg.m_mouseY, msg.m_mouseX);
        if (hoverID != g_lastImHoverId) {
            hoverChanged = 1;
            g_lastImHoverId = hoverID;
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

    if (videoNeedsUpdate() || hoverChanged) {
        g_mainMenu->drawWindow(0, TMainMenu::NEW_GAME_ID,
                               TMainMenu::QUIT_ID);
        g_windowManager->updateScreen(520, 4, 250, 567);
        videoDrawRects();
    }

    if (!updatePlease) {
        if (g_dPlayReady) {
            unsigned long lastCheck = g_lastDiskSpaceCheck;
            if (static_cast<long>(GameTime::get() - lastCheck) > 10000)
                g_windowManager->m_dialogReturn = TMainMenu::NEW_GAME_ID;
            else
                return MESSAGE_DISPATCH_CONSUME;
        } else {
            return MESSAGE_DISPATCH_CONSUME;
        }
    }

    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeY = widget::WIDGET_END_DIALOG;
    msg.m_codeX = widget::WIDGET_END_DIALOG;
    return MESSAGE_DISPATCH_FORWARD;
}

#if 0  // @carcass: Dreamcast-only video-mode class

// E:\gamedcs\mainmenu.cpp:308
DC_ONLY(0xea9b0, 0x6D0)
void VideomodeChoice::VideomodeChoice()
{
    // @stub
}

// E:\gamedcs\mainmenu.cpp:360
DC_ONLY(0xeb080, 0x62)
void VideomodeChoice::~VideomodeChoice()
{
    // @stub
}

// E:\gamedcs\mainmenu.cpp:369
DC_ONLY(0xeb0e4, 0x162)
void VideomodeChoice::Test()
{
    // @stub
}

// E:\gamedcs\mainmenu.cpp:410
DC_ONLY(0xeb248, 0xF4)
int VideomodeChoice::windowHandler(message& msg)
{
    // @stub
}

// E:\gamedcs\mainmenu.cpp:357
DC_ONLY(0xeb370, 0x34)
void* VideomodeChoice::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass
