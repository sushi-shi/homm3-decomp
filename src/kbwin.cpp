#include "prefs.h"
#include "va.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"

#include "kbwin.h"

#include "exec.h"
#include "game.h"
#include "inputmgr.h"
#include "kb.h"
#include "misc.h"
#include "mousemgr.h"
#include "smackmgr.h"
#include "soundmgr.h"
#include "terrain.h"
#include "wingraph.h"
#include "winmgr.h"

// Every cross-TU callee and global now comes from its owner's header
// (inputmgr.h message bridges, game.h Imm hooks, misc.h WritePrefs,
// kb.h InitMainClasses/oldmain/GameUnsaved/gText/bForegroundApp,
// wingraph.h AppPaint/InitGraphics, textresource.h's gpGeneralText).
// NOTE the timeGetTime import-form split: this TU takes the
// IAT form from mmsystem.h via <windows.h> and must never see
// winmm_thunks.h's plain declaration (see that header).

static int appInit(HINSTANCE instance, HINSTANCE previousInstance, int sw);

VA(0x004f7a30, 0x1CF)  // dc 0xe7c90
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR cmdLine, int sw)
{
    DWORD lastError;

    g_instance = instance;
    g_gameEvent = CreateEventA(0, 0, 0,
        DATA_COMPGEN(0x0067f9ac, winMainEventName, "Heroes III"));
    lastError = GetLastError();
    if (g_gameEvent == 0 || lastError == ERROR_ALREADY_EXISTS) {
        sprintf(g_text,
            DATA_COMPGEN(0x0067f958, winMainAlreadyRunning,
                "Heroes of Might and Magic III is already running."),
            DATA_COMPGEN(0x0067f98c, winMainTitleArg,
                "Heroes of Might and Magic III"));
        MessageBoxA(0, g_text,
            DATA_COMPGEN(0x0067f728, winMainStartupError, "Startup error"),
            MB_ICONHAND);
        return 0;
    }
    memset(g_commandLine, 0, 61);
    strncpy(g_commandLine, cmdLine, 60);
    timeBeginPeriod(1);
    if (!earlySetup())
        return 0;
    if (!appInit(instance, previousInstance, sw))
        return 0;
    oldmain();
    return 0;
}

// Original: AppInit; kbwin.cpp:166, dc 0xe7d20
// Complete uses ANSI window APIs and initializes the desktop Imm mouse;
// DC uses wide WinCE APIs and its own DirectInput/sound initialization.
// The ordinary helper expands into WinMain at 0x4f7a30 in Complete.
static int appInit(HINSTANCE instance, HINSTANCE previousInstance, int sw)
{
    WNDCLASSA appClass;
    RECT windowRect;
    DWORD windowStyle;
    DWORD windowExStyle;
    if (!previousInstance) {
        appClass.hCursor = 0;
        appClass.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(0x73));
        appClass.lpszMenuName = 0;
        appClass.lpszClassName = g_appName;
        appClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        appClass.hInstance = instance;
        appClass.style = CS_BYTEALIGNCLIENT | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        appClass.lpfnWndProc = appWndProc;
        appClass.cbWndExtra = 0;
        appClass.cbClsExtra = 0;
        if (!RegisterClassA(&appClass))
            return 0;
    }
    if (g_config.m_mainGameFullScreen) {
        windowStyle = WS_POPUP | WS_VISIBLE;
        windowExStyle = WS_EX_TOPMOST;
    } else {
        windowExStyle = 0;
        windowStyle = WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    }
    windowRect.top = windowRect.left = 0;
    windowRect.right = 800;
    windowRect.bottom = 600;
    AdjustWindowRect(&windowRect, windowStyle, g_config.m_mainGameFullScreen == 0);
    g_hwndApp = CreateWindowExA(windowExStyle, g_appName, g_title, windowStyle,
        g_config.m_mainGameX, g_config.m_mainGameY,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        0, g_config.m_mainGameFullScreen ? 0 : g_dfltMenu, instance, 0);
    if (!g_hwndApp)
        return 0;
    initGraphics();
    SetCursor(LoadCursorA(0, IDC_ARROW));
    initImmMouse(g_instance, g_hwndApp);
    return 1;
}

// AppWndProc retains its AppCommand call at +0x359 without an inline fence.
VA(0x004f7c00, 0x394)  // dc 0xe7e38
LRESULT CALLBACK appWndProc(HWND window, UINT message, WPARAM messageParam, LPARAM messageData)
{
    switch (message) {
        case WM_CREATE:
            // Mac core/make callback 0:0x20f7ac retains GameTime::get
            // before srand; Windows 0x4f7c39 expands its timeGetTime read.
            srand(GameTime::get());
            GdiSetBatchLimit(1);
            return 0;
        case WM_ACTIVATE:
            if (window != g_hwndApp)
                break;
            if (g_mouseManager) {
                unsigned char minimized = HIWORD(messageParam) != 0;
                if ((IsIconic(g_hwndApp) != 0) != minimized)
                    g_mouseManager->reset();
            }
            return 0;
        case WM_MOVE:
            if (!g_hwndApp)
                return 0;
            g_appWindowStyle = GetWindowLongA(g_hwndApp, GWL_STYLE);
            if (!(g_appWindowStyle & (WS_MINIMIZE | WS_MAXIMIZE)) && !g_closingApp) {
                immMouseWindowMoved();
                if (!g_config.m_mainGameFullScreen) {
                    GetWindowRect(window, &g_rcAppWindow);
                    g_config.m_mainGameX = g_rcAppWindow.left;
                    g_config.m_mainGameY = g_rcAppWindow.top;
                    writePrefs();
                }
            }
            return 0;
        case WM_PAINT:
            appPaint(window, 0);
            return 0;
        case WM_KEYDOWN:
        case WM_KEYUP:
            if (keyboardMessageHandler(window, message, messageParam, messageData) == 0)
                return 0;
            break;
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
            if (mouseMessageHandler(window, message, messageParam, messageData) == 0)
                return 0;
            break;
        case WM_CLOSE:
            if (window == g_hwndApp && gameUnsaved()) {
                videoPause();
                normalDialog((*g_generalText)[GENERAL_TEXT_QUIT], 2,
                    -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                videoResume();
                if (g_windowManager->m_dialogReturn == DIALOG_RETURN_ACCEPT)
                    DestroyWindow(window);
                return 0;
            }
        case WM_DESTROY:
            g_closingApp = 1;
            PostQuitMessage(0);
        case WM_QUIT:
            if (!g_shutDownDone)
                shutDown(0);
            return 0;
        case WM_ACTIVATEAPP: {
            if (window != g_hwndApp)
                break;
            unsigned char active = messageParam != 0;
            g_foregroundApp = active;
            if (active) {
                if (g_appDeactivated) {
                    g_musicWasPlaying = 0;
                    g_soundManager->resumeStream();
                    g_soundManager->resumeSamples();
                    if (!g_remoteOn)
                        videoResume();
                    g_mouseManager->showSystemCursor(0);
                    g_appDeactivated = 0;
                }
            } else {
                if (g_soundManager->musicPlaying() || g_soundManager->m_mp3Playing)
                    g_musicWasPlaying = 1;
                g_soundManager->pauseSamples();
                if (!g_remoteOn)
                    videoPause();
                if (!g_appDeactivated)
                    g_mouseManager->showSystemCursor(1);
                g_appDeactivated = 1;
            }
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_COMMAND:
            return appCommand(window, message, messageParam, messageData);
        default:
            return DefWindowProcA(window, message, messageParam, messageData);
    }
    return DefWindowProcA(window, message, messageParam, messageData);
}

VA(0x004f7fa0, 0xA)  // dc 0xe7fb8
void appExit()
{
    cleanUpWinGraphics();
    cleanUpMenus();
}

// Mac retains the shared event pump at code 0+0x20f90c. Its event polling
// uses Mac OS services; this Windows body pumps native window messages.
VA(0x004f7fb0, 0xAA) MAC_ADDRESS(0x20f90c, 0x350)  // dc 0xe7fd0
void process1WindowsMessage()
{
    MSG message;
    g_inMessageLoop = 1;
    while (1) {
        if (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
            continue;
        }
        if (IsIconic(g_hwndApp) && !g_remoteOn) {
            do {
                if (GetMessageA(&message, 0, 0, 0)) {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }
            } while (IsIconic(g_hwndApp) && !g_remoteOn);
        } else {
            break;
        }
    }
    videoNextFrame();
    g_inMessageLoop = 0;
}

// E:\gamedcs\kbwin.cpp:648
// homm2 lineage kept the three non-size menu commands; the About
// template is the ordinal 0x67 (homm2 passed the string "HEROES").
VA(0x004f8060, 0xD4)  // dc 0xe8014
LRESULT appCommand(HWND window, UINT message, WPARAM messageParam, LPARAM messageData)
{
    int command;

    command = LOWORD(messageParam);
    switch (command) {
        case KBWIN_MENU_ABOUT:
            g_mouseManager->showSystemCursor(1);
            DialogBoxParamA(g_instance, MAKEINTRESOURCEA(0x67), window,
                (DLGPROC)appAbout, 0);
            g_mouseManager->showSystemCursor(0);
            break;
        case KBWIN_MENU_HELP:
            if (g_config.m_mainGameFullScreen)
                SetForegroundWindow(GetDesktopWindow());
            WinHelpA(g_hwndApp,
                DATA_COMPGEN(0x0067fa20, appCommandHelpFile, ".\\HEROES3.HLP"),
                HELP_FINDER, 0);
            break;
        case KBWIN_MENU_FULLSCREEN:
            if (!setFullScreenStatus(1 - g_config.m_mainGameFullScreen))
                normalDialog(
                    DATA_COMPGEN(0x0067f9b8, appCommandColorModeText,
                        "This game runs in 65536 color mode. You must switch the desktop to this mode before playing the game."),
                    1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            break;
        default:
            return handleAppSpecificMenuCommands(command);
    }
    return 0;
}

// Original: UpdateDfltMenu; kbwin.cpp:680, dc 0xe8018.
// The released menu-update hook has an empty body. The adjacent 0x4f8140
// procedure is the four-argument About callback, not this one-argument hook.
void updateDfltMenu(HMENU menu)
{
}

VA(0x004f8140, 0x37)  // address-taken DialogBoxParamA callback, retail-only
BOOL CALLBACK appAbout(HWND dialog, UINT message, WPARAM messageParam, LPARAM messageData)
{
    unsigned short command;

    switch (message) {
        case WM_INITDIALOG:
            return 1;
        case WM_COMMAND:
            command = LOWORD(messageParam);
            if (command == IDOK)
                EndDialog(dialog, 1);
            return 1;
    }
    pollSound();
    return 0;
}

VA(0x004f8180, 0x5C)  // dc 0xe801c
void kbChangeMenu(HMENU newMenu)
{
    if (!newMenu)
        newMenu = g_currMenu;
    else
        g_currMenu = newMenu;
    g_activeMenu = newMenu;
    if (!g_config.m_mainGameFullScreen) {
        if (newMenu) {
            SetMenu(g_hwndApp, newMenu);
            DrawMenuBar(g_hwndApp);
        }
    } else {
        SetMenu(g_hwndApp, 0);
        DrawMenuBar(g_hwndApp);
    }
}

VA(0x004f81e0, 0x31) MAC_ADDRESS(0x20ff48, 0x38)  // dc 0xe8020
void setNoDialogMenus(int noMenus)
{
    if (g_menusSuppressed && !noMenus)
        return;
    if (!g_menusSuppressed && noMenus)
        return;
    if (!g_activeMenu)
        return;
    g_menusSuppressed = 1 - noMenus;
    setMenus(g_activeMenu, noMenus);
}

VA(0x004f8220, 0xB2)  // dc 0xe8054
void setMenus(HMENU menu, int enabled)
{
    int count;
    unsigned int commandId;
    unsigned int scanPosition;
    unsigned int commandPosition;
    int disableFlag;
    int index;

    count = GetMenuItemCount(menu);
    for (index = 0; index < count; index++) {
        commandId = GetMenuItemID(menu, index);
        if (commandId == static_cast<unsigned int>(-1)) {
            setMenus(GetSubMenu(menu, index), enabled);
            disableFlag = 0;
        } else {
            disableFlag = 0;
            if (enabled) {
                disableFlag = 1;
            } else {
                scanPosition = 0;
                for (commandPosition = 0; commandPosition < KBWIN_MENU_ENTRY_COUNT;
                     commandPosition++) {
                    if (g_menuEnableStatus[commandPosition].m_command == commandId) {
                        scanPosition = commandPosition;
                    }
                }
                if (g_inSetupDialog)
                    disableFlag = 1 - g_menuEnableStatus[scanPosition].m_setupEnabled;
                else
                    disableFlag = 1 - g_menuEnableStatus[scanPosition].m_normalEnabled;
            }
        }
        if (disableFlag != 0) {
            EnableMenuItem(menu, commandId, enabled == 0 ? MF_GRAYED : MF_ENABLED);
        }
    }
}

VA(0x004f82e0, 0x6) MAC_ADDRESS(0x20fc5c, 0xc)  // dc 0xe8058
unsigned long GameTime::get()
{
    return timeGetTime();
}

VA(0x004f82f0, 0xCD) MAC_ADDRESS(0x20fc68, 0x40)  // dc 0xe806c
void GameTime::delayTil(unsigned long time)
{
    while (!GameTime::isPast(time)) {
        process1WindowsMessage();
        pollSound();
    }
}

VA(0x004f83c0, 0xD0) MAC_ADDRESS(0x20fca8, 0x34)  // dc 0xe8098
void GameTime::delay(int interval)
{
    GameTime::delayTil(GameTime::get() + interval);
}

// Original: InitVideo; kbwin.cpp:851, dc 0xe80b4
// Empty hook; heroWindowManager::open retains the call to retail's
// shared ICF ret at 0x5bc690.
MAC_ADDRESS(0x20fcdc, 0x4)
void initVideo()
{
}

DATA(0x00699600)
HWND g_hwndApp;

DATA(0x006995b4)
HINSTANCE g_instance;

DATA(0x006995b8)
unsigned char g_inMessageLoop;

DATA(0x006995bc)
HMENU g_currMenu;

DATA(0x00699604)
HMENU g_activeMenu;

DATA(0x00699618)
int g_menusSuppressed;


// Former provisional g_videoPaused: this is the network-session latch.
// It prevents local window deactivation from pausing a live network game.
// Original DC name: gbRemoteOn; StartLocalPlayerTurn and remote message paths.
DATA(0x0069954c)
int g_remoteOn;

DATA(0x006989d0)
int g_inSetupDialog;

DATA(0x006989e4)
HMENU g_dfltMenu;

DATA(0x006989e8)
HMENU g_gameMenu;

DATA(0x0067f820)
char g_appName[] = "Heroes III";

DATA(0x0067f82c)
char g_title[] = "Heroes of Might and Magic III";

DATA(0x0069960c)
HANDLE g_gameEvent;

DATA(0x006995c0)
char g_commandLine[61];



DATA(0x006995a8)
LONG g_appWindowStyle;

DATA(0x00699598)
RECT g_rcAppWindow;

DATA(0x006989fc)
int g_closingApp;

DATA(0x00699608)
unsigned char g_shutDownDone;

DATA(0x00699609)
unsigned char g_appDeactivated;

DATA(0x00699614)
unsigned char g_musicWasPlaying;

// Values read from the retail image (.data 0x67f930, stride 8): a
// zero sentinel row - the scan's scanPosition=0 default - then the
// three surviving menu commands and 0x9ccc, an app-specific command
// (AppCommand's default arm forwards it to kb.cpp's
// HandleAppSpecificMenuCommands; unnamed until kb.cpp lands).
DATA(0x0067f930)
SMenuEnableStatus g_menuEnableStatus[KBWIN_MENU_ENTRY_COUNT] = {
    { 0, 0, 0 },
    { KBWIN_MENU_FULLSCREEN, 1, 1 },
    { KBWIN_MENU_HELP, 1, 1 },
    { KBWIN_MENU_ABOUT, 1, 1 },
    { 0x9ccc, 1, 1 },
};
