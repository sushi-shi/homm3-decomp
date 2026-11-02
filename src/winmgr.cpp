#include "va.h"

#include <stdio.h>

#include "winmgr.h"

#include "bitmap16.h"
#include "bitmap816.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "message.h"
#include "mousemgr.h"
#include "remote.h"
#include "soundmgr.h"
#include "terrain.h"
#include "widget.h"
#include "window.h"
#include "wingraph.h"

// DC gbInDialog and gbSendMouseMoveMessages; the nest counter is retail-only.
DATA(0x006989cc) int g_inDialog;
DATA(0x00698a1c) int g_sendMouseMoveMessages;
DATA(0x006aad20) int g_dialogNestCount;


// Original file-static currScreenShot, DC data section 3:0x1fc74.
// Only the unclaimed release screenshot helper uses this counter.
static int g_currScreenShot;

// Original ScreenLimits/NullLimits, DC winmgr.cpp:54/56. Retail startup
// 0x602110/0x602140 constructs these immediately before the manager ctor.
// NullLimits is the empty accumulator copied by combat drawing callers.
DATA(0x006aad00) const SLimitData heroWindowManager::s_screenLimits(0, 0, 799, 599);
DATA(0x006aace8) const SLimitData heroWindowManager::s_nullLimits(799, 599, 0, 0);

VA(0x00602170, 0x38)
DC_ADDRESS(0x19a7ec, 0x54)
MAC_ADDRESS(0x20d13c, 0x6c)
heroWindowManager::heroWindowManager()
{
    m_status = 0;
    m_activeWindow = 0;
    m_lastActive = 0;
    m_tailWindow = 0;
    m_headWindow = 0;
    m_screenBitmap = 0;
    m_colorCyclingOn = 0;
    m_bmpFizzleSource = 0;
    m_lastHover = -1;
    m_dialogReturn = -1;
    m_isWaitingForFadeIn = 0;
}

VA(0x006021b0, 0x114)
DC_ADDRESS(0x19a840, 0x11a)
MAC_ADDRESS(0x20d1a8, 0x10c)
int heroWindowManager::open(int newPriority)
{
    initVideo();

    m_screenBitmap = new Bitmap16Bit(0, 0);
    if (m_screenBitmap == 0)
        memError();

    // DC winmgr.cpp:112 calls these four InitWin accessors.
    m_screenBitmap->reference(g_initWin.GetWidth(), g_initWin.GetHeight(),
                            g_initWin.GetPitch(), g_initWin.GetMap(0, 0));
    m_screenBitmap->FillRect(0, 0, 800, 600, 0);

    RECT tempRect;
    tempRect.left = 0;
    tempRect.top = 0;
    tempRect.right = 800;
    tempRect.bottom = 600;
    robAppBlit(&tempRect);

    g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
    g_mouseManager->showPointer(1);

    m_priority = newPriority;
    m_id = 32;
    m_status = STATUS_ACTIVE;
    strcpy(m_mgrName,
           DATA_COMPGEN(0x0068d260, heroWindowManagerName,
                        "heroWindowManager"));
    return 0;
}

VA(0x006022d0, 0x46)
DC_ADDRESS(0x19a95c, 0x62)
MAC_ADDRESS(0x20d2b4, 0xac)
void heroWindowManager::close()
{
    if (m_status != STATUS_ACTIVE)
        return;
    heroWindow* w = m_tailWindow;
    while (w) {
        heroWindow* prev = w->m_prevWindow;
        removeWindow(w);
        w = prev;
    }
    if (m_screenBitmap)
        delete m_screenBitmap;
    if (m_bmpFizzleSource)
        delete m_bmpFizzleSource;
    m_status = 0;
}

VA(0x00602320, 0x36)
DC_ADDRESS(0x19a9c0, 0x46)
MAC_ADDRESS(0x20d360, 0x78)
int heroWindowManager::main(message& msg)
{
    int result = 0;

    for (heroWindow* w = m_tailWindow; w; w = w->m_prevWindow) {
        if (w->m_sleepCount > 0)
            continue;
        result = w->broadcastMessage(msg);
        if (result > 0 && result <= MESSAGE_DISPATCH_FORWARD)
            break;
    }
    return result;
}

VA(0x00602360, 0x10)
DC_ADDRESS(0x19aa08, 0x16)
MAC_ADDRESS(0x20d3d8, 0x2c)
int heroWindowManager::convertToHover(message& msg)
{
    return main(msg);
}

VA(0x00602370, 0x3B)
DC_ADDRESS(0x19aa20, 0x44)
MAC_ADDRESS(0x20d404, 0x64)
int heroWindowManager::broadcastMessage(int msgId, int msgCodeX, int msgCodeY, int msgExtra)
{
    message msg;

    msg.m_id = msgId;
    msg.m_codeX = msgCodeX;
    msg.m_codeY = msgCodeY;
    msg.m_extra = msgExtra;
    return main(msg);
}

// Original DC public AddWindow ends PAVheroWindow@@H_N: update is bool,
// forwarded directly to the virtual window opener.
VA(0x006023b0, 0xE7)
DC_ADDRESS(0x19aa64, 0x1b0)
MAC_ADDRESS(0x20d468, 0x154)
void heroWindowManager::addWindow(heroWindow* newWindow, int newPriority,
                                  bool update)
{
    heroWindow* at = m_tailWindow;

    if (newWindow->m_type & WINDOW_FLAG_FIXED_LAYER) {
        newPriority = 0;
    } else {
        if (newPriority == -1) {
            if (!m_tailWindow)
                newPriority = 0;
            else
                newPriority = m_tailWindow->m_priority + 1;
        }
        if (newPriority != 0 && !m_headWindow)
            return;
    }

    if (newWindow->open(newPriority, update))
        return;

    while (at && at->m_priority > newPriority)
        at = at->m_prevWindow;

    if (!at) {
        newWindow->m_nextWindow = m_headWindow;
        newWindow->m_prevWindow = 0;
        m_headWindow = newWindow;
        if (!m_tailWindow)
            m_tailWindow = newWindow;
    } else if (!at->m_nextWindow) {
        newWindow->m_prevWindow = m_tailWindow;
        newWindow->m_nextWindow = 0;
        m_tailWindow->m_nextWindow = newWindow;
        m_tailWindow = newWindow;
    } else {
        newWindow->m_prevWindow = at;
        newWindow->m_nextWindow = at->m_nextWindow;
        at->m_nextWindow->m_prevWindow = newWindow;
        at->m_nextWindow = newWindow;
    }
    m_activeWindow = m_lastActive;
    m_lastActive = newWindow;
}

VA(0x006024a0, 0x78)
DC_ADDRESS(0x19ac14, 0x104)
MAC_ADDRESS(0x20d5bc, 0x104)
void heroWindowManager::removeWindow(heroWindow* killWindow)
{
    if (!killWindow)
        return;
    killWindow->close(1);
    if (killWindow == m_headWindow) {
        m_headWindow = killWindow->m_nextWindow;
        if (!m_headWindow)
            m_tailWindow = 0;
        else
            m_headWindow->m_prevWindow = 0;
    } else if (killWindow == m_tailWindow) {
        m_tailWindow = killWindow->m_prevWindow;
        m_tailWindow->m_nextWindow = 0;
    } else {
        heroWindow* prev = killWindow->m_prevWindow;
        if (prev)
            prev->m_nextWindow = killWindow->m_nextWindow;
        heroWindow* next = killWindow->m_nextWindow;
        if (next)
            next->m_prevWindow = killWindow->m_prevWindow;
    }
    if (m_activeWindow == killWindow)
        m_activeWindow = 0;
    m_lastActive = m_activeWindow ? m_activeWindow : m_tailWindow;
}

// E:\gamedcs\winmgr.cpp:461
// FOUR try levels, each re-running one cleanup and rethrowing. Innermost
// first they are: RemoveWindow, the SleepAllWidgets(0) walk, gbInDialog =
// 0, and the nest-count decrement.
VA(0x00602520, 0x280)
DC_ADDRESS(0x19ad18, 0x1dc)
MAC_ADDRESS(0x20d6c0, 0x308)  // anchor-global
int heroWindowManager::doDialog(heroWindow* dialogWindow,
                                TDialogHandler dialogFunction, int fadeIn)
{
    int endFlag;

    if (g_dialogNestCount++ == 0)
        setNoDialogMenus(0);
    try {
        g_inDialog = 1;
        try {
            sleepAllWindows(1);
            try {
                m_lastHover = -1;
                if (dialogWindow)
                    addWindow(dialogWindow, -1, 1);
                try {
                    if (fadeIn)
                        g_windowManager->fadeScreen(0, 4, 0);
                    g_inputManager->flush();
                    message msg;
                    m_dialogReturn = -1;
                    endFlag = 0;
                    msg.m_id = 0;
                    msg.m_codeX = 0;
                    msg.m_codeY = 0;
                    msg.m_qualifier = 0;
                    msg.m_mouseX = 0;
                    msg.m_mouseY = 0;
                    msg.m_extra = 0;
                    msg.m_window = 0;
                    while (!endFlag) {
                        pollSound();
                        process1WindowsMessage();
                        msg = g_inputManager->getEvent();
                        msg.m_window = dialogWindow;
                        g_mouseManager->main(msg);
                        if (dialogWindow
                            && (msg.m_id != MESSAGE_MOUSE_MOVE
                                || g_sendMouseMoveMessages)) {
                            int result = dialogWindow->broadcastMessage(msg);
                            if (result == MESSAGE_DISPATCH_CONSUME)
                                continue;
                            if (result == MESSAGE_DISPATCH_FORWARD
                                && msg.m_id == MESSAGE_WIDGET
                                && msg.m_codeX == widget::WIDGET_END_DIALOG) {
                                m_dialogReturn = msg.m_codeY;
                                endFlag = 1;
                            }
                        }
                        msg.m_window = dialogWindow;
                        if (dialogFunction(msg) == MESSAGE_DISPATCH_FORWARD
                            && msg.m_id == MESSAGE_WIDGET
                            && msg.m_codeX == widget::WIDGET_END_DIALOG)
                            endFlag = 1;
                    }
                } catch (...) {
                    if (dialogWindow)
                        removeWindow(dialogWindow);
                    throw;
                }
                if (dialogWindow)
                    removeWindow(dialogWindow);
            } catch (...) {
                sleepAllWindows(0);
                throw;
            }
            sleepAllWindows(0);
        } catch (...) {
            g_inDialog = 0;
            throw;
        }
        g_inDialog = 0;
    } catch (...) {
        if (--g_dialogNestCount == 0)
            setNoDialogMenus(1);
        throw;
    }
    if (--g_dialogNestCount == 0)
        setNoDialogMenus(1);
    return 0;
}

// E:\gamedcs\winmgr.cpp:645
// DoDialog's sibling, with the same four try levels. Four deliberate
// divergences from DoDialog: the draw callback runs once before the fade;
// the window's CONSUME verdict does NOT short-circuit the handler; the
// level-2 body flushes the input queue after RemoveWindow; and the
// handler's verdict is a two-case SWITCH, not an if/else chain.
VA(0x006027a0, 0x29A)
DC_ADDRESS(0x19aef4, 0x208)
MAC_ADDRESS(0x20d9c8, 0x340)  // anchor-global
int heroWindowManager::doDialogDraw(heroWindow* dialogWindow,
                                    TDialogHandler dialogFunction,
                                    TDialogHandler dialogDrawFunction,
                                    int fadeIn)
{
    int endFlag;

    if (g_dialogNestCount++ == 0)
        setNoDialogMenus(0);
    try {
        g_inDialog = 1;
        try {
            sleepAllWindows(1);
            try {
                m_lastHover = -1;
                if (dialogWindow)
                    addWindow(dialogWindow, -1, 1);
                try {
                    message msg;
                    endFlag = 0;
                    msg.m_id = 0;
                    msg.m_codeX = 0;
                    msg.m_codeY = 0;
                    msg.m_qualifier = 0;
                    msg.m_mouseX = 0;
                    msg.m_mouseY = 0;
                    msg.m_extra = 0;
                    msg.m_window = 0;
                    dialogDrawFunction(msg);
                    if (fadeIn)
                        g_windowManager->fadeScreen(0, 4, 0);
                    g_inputManager->flush();
                    m_dialogReturn = -1;
                    while (!endFlag) {
                        pollSound();
                        process1WindowsMessage();
                        msg = g_inputManager->getEvent();
                        msg.m_window = dialogWindow;
                        g_mouseManager->main(msg);
                        if (dialogWindow
                            && (msg.m_id != MESSAGE_MOUSE_MOVE
                                || g_sendMouseMoveMessages)) {
                            if (dialogWindow->broadcastMessage(msg)
                                    == MESSAGE_DISPATCH_FORWARD
                                && msg.m_id == MESSAGE_WIDGET
                                && msg.m_codeX == widget::WIDGET_END_DIALOG) {
                                m_dialogReturn = msg.m_codeY;
                                endFlag = 1;
                            }
                        }
                        msg.m_window = dialogWindow;
                        if (dialogFunction(msg) == MESSAGE_DISPATCH_FORWARD
                            && msg.m_id == MESSAGE_WIDGET) {
                            switch (msg.m_codeX) {
                            case widget::WIDGET_END_DIALOG:
                                endFlag = 1;
                                break;
                            case widget::WIDGET_RETURN_32:
                                dialogDrawFunction(msg);
                                break;
                            }
                        }
                    }
                } catch (...) {
                    if (dialogWindow)
                        removeWindow(dialogWindow);
                    throw;
                }
                if (dialogWindow)
                    removeWindow(dialogWindow);
                g_inputManager->flush();
            } catch (...) {
                sleepAllWindows(0);
                throw;
            }
            sleepAllWindows(0);
        } catch (...) {
            g_inDialog = 0;
            throw;
        }
        g_inDialog = 0;
    } catch (...) {
        if (--g_dialogNestCount == 0)
            setNoDialogMenus(1);
        throw;
    }
    if (--g_dialogNestCount == 0)
        setNoDialogMenus(1);
    return 0;
}

VA(0x00602a40, 0x188)
DC_ADDRESS(0x19b0fc, 0xf2)
MAC_ADDRESS(0x20dd08, 0x224)
void heroWindowManager::doQuickView(heroWindow* window)
{
    g_mouseManager->hidePointer();
    try {
        sleepAllWindows(1);
        try {
            if (window)
                addWindow(window, -1, 1);
            try {
                for (;;) {
                    message msg;
                    pollSound();
                    process1WindowsMessage();
                    msg = g_inputManager->getEvent();
                    unsigned char done =
                        msg.m_id == MESSAGE_RIGHT_BUTTON_UP
                        || msg.m_id == MESSAGE_LEFT_BUTTON_DOWN
                        || msg.m_id == MESSAGE_LEFT_BUTTON_UP;
                    if (g_remoteOn && g_dPlay) {
                        CNetMsgHandler* pump =
                            g_dPlay->getNetMsgHandler();
                        if (pump) {
                            pump->checkHandleNet(1, 0);
                            if (pump->getAbortPopupMsg())
                                break;
                        }
                    }
                    if (done)
                        break;
                }
            } catch (...) {
                if (window)
                    removeWindow(window);
                throw;
            }
            if (window)
                removeWindow(window);
        } catch (...) {
            sleepAllWindows(0);
            throw;
        }
        sleepAllWindows(0);
    } catch (...) {
        g_mouseManager->showPointer(false);
        throw;
    }
    g_mouseManager->showPointer(false);
}

VA(0x00602bd0, 0x7C)
DC_ADDRESS(0x19b230, 0xf8)
MAC_ADDRESS(0x20df2c, 0x9c)
void heroWindowManager::updateScreen(int x, int y, int width, int height)
{
    if (m_isWaitingForFadeIn)
        return;
    g_mouseManager->disable();  // DC 0x19b280; no mutation in release.
    pollSound();
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (x + width > WINDOW_SCREEN_WIDTH)
        width = WINDOW_SCREEN_WIDTH - x;
    if (y + height > WINDOW_SCREEN_HEIGHT)
        height = WINDOW_SCREEN_HEIGHT - y;
    blitToScreenWithPointer(x, y, width, height);
    g_mouseManager->enable();  // DC winmgr.cpp:899.
    pollSound();
}

// Original: heroWindowManager::BlitToScreenWithPointer; winmgr.cpp:964
// Inlined in UpdateScreen, FadeToBlack and FadeIn, which call RobAppBlit.
// The older DC DDAppBlit(const RECT&) null/global-mdr1 wrapper is
// console-only.
DC_ADDRESS(0x19b3c4, 0x2a)
MAC_ADDRESS(0x20dfc8, 0x74)
void heroWindowManager::blitToScreenWithPointer(int x, int y, int w, int h)
{
    RECT tempRect;
    if (w > 0 && h > 0) {
        tempRect.left = x;
        tempRect.top = y;
        tempRect.right = x + w;
        tempRect.bottom = y + h;
        robAppBlit(&tempRect);
    }
}

VA(0x00602c50, 0x63)
DC_ADDRESS(0x19b428, 0x66)
MAC_ADDRESS(0x20e03c, 0xb4)
void heroWindowManager::fadeScreen(int inOut, int speed, unsigned char expectFadein)
{
    if (inOut == 1) {
        if (expectFadein)
            g_mouseManager->hidePointer();
        fadeToBlack(speed, expectFadein);
        if (expectFadein)
            m_isWaitingForFadeIn = 1;
    } else {
        fadeFromBlack(speed);
        if (m_isWaitingForFadeIn)
            g_mouseManager->showPointer(0);
        m_isWaitingForFadeIn = 0;
    }
}

// Original: heroWindowManager::ScreenShot; winmgr.cpp:1051
// DC release retains only the counter/name and input flush; it emits no image
// export call. Preserve that observed operation without inventing an exporter
// or assigning an unproven Complete address to the counter.
DC_ADDRESS(0x19b490, 0x2c)
void heroWindowManager::screenShot()
{
    char name[16];
    sprintf(name, "SHOT%04d.PCX", g_currScreenShot);
    ++g_currScreenShot;
    g_inputManager->flush();
}

// Original: heroWindowManager::SaveFizzleSource; winmgr.cpp:1085
// The ordinary non-X interface shares the saved bitmap with the X family;
// no distinct Complete body is claimed.
DC_ADDRESS(0x19b4bc, 0x104)
void heroWindowManager::saveFizzleSource(int startX, int startY, int width, int height)
{
    if (!g_completeDrawEnabled)
        return;
    if (startX < 0) {
        width += startX;
        startX = 0;
    }
    if (startY < 0) {
        height += startY;
        startY = 0;
    }
    if (startX + width > WINDOW_SCREEN_WIDTH)
        width = WINDOW_SCREEN_WIDTH - startX;
    if (startY + height > WINDOW_SCREEN_HEIGHT)
        height = WINDOW_SCREEN_HEIGHT - startY;
    if (width <= 0 || height <= 0)
        return;
    if (m_bmpFizzleSource)
        delete m_bmpFizzleSource;
    m_bmpFizzleSource = new Bitmap16Bit(width, height);
    m_bmpFizzleSource->Grab(g_windowManager->m_screenBitmap, startX, startY);
}

VA(0x00602cc0, 0xF2)
DC_ADDRESS(0x19b5c0, 0xaa)
MAC_ADDRESS(0x20e0f0, 0x110) // MAC_ABSTRACTION_FROM(tokens1:bede0a020786,56.9853): restore the DC-proven bitmap Grab overload; Mac 0x20e1e8 expands its map/width/height/pitch forwarding.
void heroWindowManager::saveFizzleSourceX(int startX, int startY, int width,
                                          int height)
{
    if (g_completeDrawEnabled) {
        if (startX < 0) {
            width += startX;
            startX = 0;
        }
        if (startY < 0) {
            height += startY;
            startY = 0;
        }
        if (startX + width > 800)
            width = 800 - startX;
        if (startY + height > 600)
            height = 600 - startY;

        if (width > 0 && height > 0) {
            delete m_bmpFizzleSource;
            m_bmpFizzleSource = new Bitmap16Bit(width, height);
            m_bmpFizzleSource->Grab(g_windowManager->m_screenBitmap,
                                    startX, startY);
        }
    }
}

// Original: heroWindowManager::FizzleForward; winmgr.cpp:1194
// DC proves four frames and a 10ms default for this non-X operation. The
// retained X interface independently has eight frames and a 33ms default.
DC_ADDRESS(0x19b66c, 0x28e)
void heroWindowManager::fizzleForward(int startX, int startY, int width,
                                     int height, int fadeTime)
{
    const int defaultFadeTime = 10;
    if (!g_completeDrawEnabled)
        return;
    if (startX < 0) {
        width += startX;
        startX = 0;
    }
    if (startY < 0) {
        height += startY;
        startY = 0;
    }
    if (startX + width > WINDOW_SCREEN_WIDTH)
        width = WINDOW_SCREEN_WIDTH - startX;
    if (startY + height > WINDOW_SCREEN_HEIGHT)
        height = WINDOW_SCREEN_HEIGHT - startY;
    if (width <= 0 || height <= 0)
        return;
    int savedColorCycling = m_colorCyclingOn;
    m_colorCyclingOn = 0;
    if (fadeTime == -1)
        fadeTime = defaultFadeTime;
    Bitmap16Bit target(width, height);
    target.Grab(m_screenBitmap, startX, startY);
    for (int frame = 0; frame < 4; ++frame) {
        unsigned long nextFrameTime = GameTime::get() + fadeTime;
        const int factor = (frame << 16) / 4;
        Bitmap16ConstMapPointer source;
        source.m_pixels = target.GetMap(0, 0);
        Bitmap16MapPointer destination;
        destination.m_pixels = m_screenBitmap->GetMap(startX, startY);
        Bitmap16ConstMapPointer oldDestination;
        oldDestination.m_pixels = m_bmpFizzleSource->GetMap(0, 0);
        for (int y = 0; y < height; ++y) {
            unsigned short* d = destination.m_pixels;
            const unsigned short* s = source.m_pixels;
            const unsigned short* od = oldDestination.m_pixels;
            for (int x = 0; x < width; ++x) {
                const int oldRed = *od & Bitmap16Bit::blue_mask;
                const int red = *s & Bitmap16Bit::blue_mask;
                const int oldGreen = *od & Bitmap16Bit::green_mask;
                const int green = *s & Bitmap16Bit::green_mask;
                const int oldBlue = *od & Bitmap16Bit::red_mask;
                const int blue = *s & Bitmap16Bit::red_mask;
                *d = static_cast<unsigned short>(
                    ((((red - oldRed) * factor >> 16) + oldRed) & Bitmap16Bit::blue_mask)
                    | ((((green - oldGreen) * factor >> 16) + oldGreen) & Bitmap16Bit::green_mask)
                    | ((((blue - oldBlue) * factor >> 16) + oldBlue) & Bitmap16Bit::red_mask));
                ++d;
                ++s;
                ++od;
            }
            destination.m_bytes += m_screenBitmap->GetPitch();
            source.m_bytes += target.GetPitch();
            oldDestination.m_bytes += m_bmpFizzleSource->GetPitch();
        }
        pollSound();
        blitToScreenWithPointer(startX, startY, width, height);
        GameTime::delayTil(nextFrameTime);
    }
    target.Draw(0, 0, width, height, m_screenBitmap, startX, startY, false);
    blitToScreenWithPointer(startX, startY, width, height);
    m_colorCyclingOn = savedColorCycling;
    releaseFizzleSource();
}

// E:\gamedcs\winmgr.cpp:1314. Cross-fade the saved fizzle source forward
// into the live screen over eight frames, then blit the destination in whole.

// Each frame interpolates the three 16-bit colour channels separately with a
// 16.16 fraction: `alpha = (frame << 16) / 8`, and per channel
// `((b & M) - (a & M)) * alpha >> 16) + (a & M)` re-masked with M. The three
// results are OR'd red | green | blue - retail evaluates the operands
// right-to-left (the blue channel's arithmetic comes first) and combines them
// left-associatively, which is what that spelling produces.

// The row pointers step by Pitch BYTES through the Bitmap16MapPointer union,
// the same view Bitmap16Bit::Draw uses; the manager's colour cycling is
// latched off for the whole fade and restored with the saved source.

// Mac retains BlitToScreenWithPointer at both update sites; inlined here.
VA(0x00602dc0, 0x2F7)
DC_ADDRESS(0x19b8fc, 0x2aa)
MAC_ADDRESS(0x20e200, 0x3e0)  // anchor-import + exhaustive tail order
void heroWindowManager::fizzleForwardX(int startX, int startY, int width,
                                       int height, int fadeTime)
{
    // DC locals: DEFAULT_FADE_TIME and factor are const int; src/odst
    // are read-only row pointers. Target, screen, then source are
    // initialized in that order. DC 1389..1394 reads the old/new red,
    // green, then blue channels; 1396..1402 interpolates and combines them
    // in that order.
    const int defaultFadeTime = 33;
    if (g_completeDrawEnabled) {
        if (startX < 0) {
            width += startX;
            startX = 0;
        }
        if (startY < 0) {
            height += startY;
            startY = 0;
        }
        if (startX + width > 800)
            width = 800 - startX;
        if (startY + height > 600)
            height = 600 - startY;

        if (width > 0 && height > 0) {
            int savedColorCycling = m_colorCyclingOn;
            m_colorCyclingOn = 0;
            if (fadeTime == -1)
                fadeTime = defaultFadeTime;

            Bitmap16Bit destination(width, height);
            destination.Grab(m_screenBitmap, startX, startY);

            for (int frame = 0; frame < 8; frame++) {
                unsigned long deadline = GameTime::get() + fadeTime;
                const int alpha = (frame << 16) / 8;

                Bitmap16ConstMapPointer target;
                target.m_pixels = destination.GetMap(0, 0);
                Bitmap16MapPointer screen;
                screen.m_pixels = m_screenBitmap->GetMap(startX, startY);
                Bitmap16ConstMapPointer source;
                source.m_pixels = m_bmpFizzleSource->GetMap(0, 0);

                for (int row = 0; row < height; row++) {
                    unsigned short* d = screen.m_pixels;
                    const unsigned short* s = target.m_pixels;
                    const unsigned short* od = source.m_pixels;
                    for (int col = 0; col < width; col++) {
                        int fromRed = *od & Bitmap16Bit::red_mask;
                        int toRed = *s & Bitmap16Bit::red_mask;
                        int fromGreen = *od & Bitmap16Bit::green_mask;
                        int toGreen = *s & Bitmap16Bit::green_mask;
                        int fromBlue = *od & Bitmap16Bit::blue_mask;
                        int toBlue = *s & Bitmap16Bit::blue_mask;
                        const int outRed =
                            ((toRed - fromRed) * alpha >> 16) + fromRed;
                        const int outGreen =
                            ((toGreen - fromGreen) * alpha >> 16) + fromGreen;
                        const int outBlue =
                            ((toBlue - fromBlue) * alpha >> 16) + fromBlue;
                        *d = static_cast<unsigned short>(
                            (outRed & Bitmap16Bit::red_mask)
                            | (outGreen & Bitmap16Bit::green_mask)
                            | (outBlue & Bitmap16Bit::blue_mask));
                        d++;
                        s++;
                        od++;
                    }
                    // Canonical DC GetPitch boundaries (lines 1407/1409).
                    screen.m_bytes += m_screenBitmap->GetPitch();
                    target.m_bytes += destination.GetPitch();
                    source.m_bytes += m_bmpFizzleSource->GetPitch();
                }

                pollSound();
                blitToScreenWithPointer(startX, startY, width, height);
                GameTime::delayTil(deadline);
            }

            destination.Draw(0, 0, width, height, m_screenBitmap,
                             startX, startY, false);
            blitToScreenWithPointer(startX, startY, width, height);

            m_colorCyclingOn = savedColorCycling;
            // DC line 1434 calls the ordinary helper; inlined.
            releaseFizzleSource();
        }
    }
}

VA(0x006030c0, 0x19)
DC_ADDRESS(0x19bba8, 0x2c)
MAC_ADDRESS(0x20e5e0, 0x54)
void heroWindowManager::releaseFizzleSource()
{
    if (m_bmpFizzleSource)
        delete m_bmpFizzleSource;
    m_bmpFizzleSource = 0;
}

// E:\gamedcs\winmgr.cpp:1707 - the fade-out, FadeScreen's first call.

// The three colour masks are widened into both halves of a dword so one
// pass handles two 16-bit pixels; the fade itself is three passes that
// shift every component right by the pass index and re-mask it into its
// own field, so pass 0 is a plain copy. `speed` is dropped on the floor -
// the pacing is the fixed 50 ms per pass below - but the parameter is real:
// FadeScreen forwards it.

// Original: heroWindowManager::NextFlashFrame; winmgr.cpp:1455
DC_ADDRESS(0x19bbd4, 0x80)
void heroWindowManager::nextFlashFrame(int startX, int startY, int width,
                                      int height, int fadeTime)
{
    unsigned long nextFrameTime = GameTime::get() + fadeTime;
    pollSound();
    blitToScreenWithPointer(startX, startY, width, height);
    GameTime::delayTil(nextFrameTime);
}

// Original: heroWindowManager::Flash; winmgr.cpp:1463
DC_ADDRESS(0x19bc54, 0x1d2)
void heroWindowManager::flash(int startX, int startY, int width, int height,
                             int fadeTime)
{
    const int defaultFadeTime = 10;
    if (!g_completeDrawEnabled)
        return;
    if (startX < 0) {
        width += startX;
        startX = 0;
    }
    if (startY < 0) {
        height += startY;
        startY = 0;
    }
    if (startX + width > WINDOW_SCREEN_WIDTH)
        width = WINDOW_SCREEN_WIDTH - startX;
    if (startY + height > WINDOW_SCREEN_HEIGHT)
        height = WINDOW_SCREEN_HEIGHT - startY;
    if (width <= 0 || height <= 0)
        return;
    int savedColorCycling = m_colorCyclingOn;
    m_colorCyclingOn = 0;
    if (fadeTime == -1)
        fadeTime = defaultFadeTime;
    Bitmap16Bit target(width, height);
    target.Grab(m_screenBitmap, startX, startY);
    nextFlashFrame(startX, startY, width, height, fadeTime);
    m_bmpFizzleSource->Draw(0, 0, width, height, m_screenBitmap, startX, startY, false);
    nextFlashFrame(startX, startY, width, height, fadeTime);
    target.Draw(0, 0, width, height, m_screenBitmap, startX, startY, false);
    nextFlashFrame(startX, startY, width, height, fadeTime);
    m_bmpFizzleSource->Draw(0, 0, width, height, m_screenBitmap, startX, startY, false);
    nextFlashFrame(startX, startY, width, height, fadeTime);
    target.Draw(0, 0, width, height, m_screenBitmap, startX, startY, false);
    nextFlashFrame(startX, startY, width, height, fadeTime);
    m_bmpFizzleSource->Draw(0, 0, width, height, m_screenBitmap, startX, startY, false);
    blitToScreenWithPointer(startX, startY, width, height);
    m_colorCyclingOn = savedColorCycling;
    releaseFizzleSource();
}

// Original: heroWindowManager::FadeBlit; winmgr.cpp:1545
// DC records separate transparent/opaque loops, palette lookup and three
// component interpolation; no standalone address is claimed for this older
// entry point.
DC_ADDRESS(0x19be28, 0x394)
void heroWindowManager::fadeBlit(int sx, int sy, int sw, int sh,
                                 const Bitmap816* srcBitmap, int dx, int dy,
                                 unsigned char transparent, int frames, int period)
{
    if (dx < 0) {
        sx -= dx;
        sw += dx;
        dx = 0;
    }
    if (dy < 0) {
        sy -= dy;
        sh += dy;
        dy = 0;
    }
    if (dx + sw > m_screenBitmap->GetWidth())
        sw = m_screenBitmap->GetWidth() - dx;
    if (dy + sh > m_screenBitmap->GetHeight())
        // DC line 1582 really calls GetWidth here, despite testing Height.
        sh = m_screenBitmap->GetWidth() - dy;
    if (sw <= 0 || sh <= 0)
        return;

    Bitmap16Bit savedDest(sw, sh);
    savedDest.Grab(m_screenBitmap, dx, dy);
    const unsigned short* sourcePalette = srcBitmap->GetPalette().Palette;
    for (int frame = 1; frame <= frames; ++frame) {
        unsigned long nextFrameTime = GameTime::get() + period;
        const int factor = (frame << 16) / frames;
        const unsigned char* source = srcBitmap->GetMap(sx, sy);
        Bitmap16MapPointer destination;
        destination.m_pixels = m_screenBitmap->GetMap(dx, dy);
        Bitmap16ConstMapPointer oldDestination;
        oldDestination.m_pixels = savedDest.GetMap(0, 0);
        if (transparent) {
            for (int y = 0; y < sh; ++y) {
                const unsigned char* s = source;
                unsigned short* d = destination.m_pixels;
                const unsigned short* od = oldDestination.m_pixels;
                for (int x = 0; x < sw; ++x) {
                    if (*s) {
                        unsigned short color = sourcePalette[*s];
                        const int oldRed = *od & Bitmap16Bit::blue_mask;
                        const int red = color & Bitmap16Bit::blue_mask;
                        const int oldGreen = *od & Bitmap16Bit::green_mask;
                        const int green = color & Bitmap16Bit::green_mask;
                        const int oldBlue = *od & Bitmap16Bit::red_mask;
                        const int blue = color & Bitmap16Bit::red_mask;
                        *d = static_cast<unsigned short>(
                            ((((red - oldRed) * factor >> 16) + oldRed) & Bitmap16Bit::blue_mask)
                            | ((((green - oldGreen) * factor >> 16) + oldGreen) & Bitmap16Bit::green_mask)
                            | ((((blue - oldBlue) * factor >> 16) + oldBlue) & Bitmap16Bit::red_mask));
                    }
                    ++d;
                    ++s;
                    ++od;
                }
                destination.m_bytes += m_screenBitmap->GetPitch();
                source += srcBitmap->GetPitch();
                oldDestination.m_bytes += savedDest.GetPitch();
            }
        } else {
            for (int y = 0; y < sh; ++y) {
                const unsigned char* s = source;
                unsigned short* d = destination.m_pixels;
                const unsigned short* od = oldDestination.m_pixels;
                for (int x = 0; x < sw; ++x) {
                    unsigned short color = sourcePalette[*s];
                    const int oldRed = *od & Bitmap16Bit::blue_mask;
                    const int red = color & Bitmap16Bit::blue_mask;
                    const int oldGreen = *od & Bitmap16Bit::green_mask;
                    const int green = color & Bitmap16Bit::green_mask;
                    const int oldBlue = *od & Bitmap16Bit::red_mask;
                    const int blue = color & Bitmap16Bit::red_mask;
                    *d = static_cast<unsigned short>(
                        ((((red - oldRed) * factor >> 16) + oldRed) & Bitmap16Bit::blue_mask)
                        | ((((green - oldGreen) * factor >> 16) + oldGreen) & Bitmap16Bit::green_mask)
                        | ((((blue - oldBlue) * factor >> 16) + oldBlue) & Bitmap16Bit::red_mask));
                    ++d;
                    ++s;
                    ++od;
                }
                destination.m_bytes += m_screenBitmap->GetPitch();
                source += srcBitmap->GetPitch();
                oldDestination.m_bytes += savedDest.GetPitch();
            }
        }
        updateScreen(dx, dy, sw, sh);
        GameTime::delayTil(nextFrameTime);
    }
}

// Mac checks its display mode to choose gamma or bitmap fade paths; those
// are platform paths, not missing helpers in the Windows fade. DC names
// const unsigned pixel masks, a read-only pixel source and the fade period,
// and the source row owner stays const across pitch advances (DC FadeFrom
// 19c450/19c468/19c502). DC locals: bmpFadeSource, red_mask_2,
// green_mask_2, blue_mask_2, next_fade_time and time1.
VA(0x006030e0, 0x1F9)
DC_ADDRESS(0x19c1bc, 0x1fa)
MAC_ADDRESS(0x20e634, 0x444) // MAC_ABSTRACTION_FROM(tokens1:a24615a82ad2,6.7766): restore bitmap Grab/Draw wrappers; Mac 0x20e6a8/0x20e7b8 and 0x20e8c8/0x20ea54 expand them in its separate fade paths.
void heroWindowManager::fadeToBlack(int speed, unsigned char expectFadein)
{
    const unsigned int redMask2 = (Bitmap16Bit::red_mask << 16) | Bitmap16Bit::red_mask;
    const unsigned int greenMask2 = (Bitmap16Bit::green_mask << 16) | Bitmap16Bit::green_mask;
    const unsigned int blueMask2 = (Bitmap16Bit::blue_mask << 16) | Bitmap16Bit::blue_mask;
    const int fadePeriod = 50;
    Bitmap16Bit bmpFadeSource(WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT);
    bmpFadeSource.Grab(m_screenBitmap, 0, 0);

    // DC winmgr.cpp:1793 calls mouseManager::Disable after Grab; its result
    // is unused.
    g_mouseManager->disable();

    for (int shift = 0; shift < 3; shift++) {
        unsigned long nextFadeTime = GameTime::get() + fadePeriod;
        unsigned long time1 = GameTime::get();
        Bitmap16MapPointer dst;
        dst.m_pixels = m_screenBitmap->GetMap(0, 0);
        Bitmap16ConstMapPointer src;
        src.m_pixels = bmpFadeSource.GetMap(0, 0);
        for (int y = 0; y < WINDOW_SCREEN_HEIGHT; y++) {
            const unsigned int* pixelSrc = src.m_pixelPairs;
            unsigned int* pixelDst = dst.m_pixelPairs;
            for (int x = 0; x < WINDOW_SCREEN_WIDTH / 2; x++) {
                const unsigned int r = *pixelSrc;
                unsigned long blue = (r & redMask2) >> shift;
                unsigned long green = (r & greenMask2) >> shift;
                unsigned long red = (r & blueMask2) >> shift;
                *pixelDst = (red & blueMask2) | (green & greenMask2)
                    | (blue & redMask2);
                pixelDst++;
                pixelSrc++;
            }
            dst.m_bytes += m_screenBitmap->GetPitch();
            src.m_bytes += bmpFadeSource.GetPitch();
        }
        blitToScreenWithPointer(0, 0, WINDOW_SCREEN_WIDTH,
                                WINDOW_SCREEN_HEIGHT);
        if (GameTime::get() - time1 > fadePeriod)
            break;
        GameTime::delayTil(nextFadeTime);
    }

    m_screenBitmap->FillRect(0, 0, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, 0);
    blitToScreenWithPointer(0, 0, WINDOW_SCREEN_WIDTH,
                            WINDOW_SCREEN_HEIGHT);
    if (expectFadein) {
        bmpFadeSource.Draw(0, 0, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT,
                           m_screenBitmap, 0, 0, false);
    }
}

// E:\gamedcs\winmgr.cpp:1866 - the fade-in, FadeScreen's second call. The
// mirror of FadeToBlack: the same three-pass shift walked
// DOWNWARDS from 2 and stopping before 0, so the last pass is the
// half-brightness frame and the full-brightness image is restored by
// the Draw below rather than by a pass of its own.

// Dreamcast rows 1957/1958 (FadeToBlack 1804/1805) take the screen map for
// dst before the fade copy's map for src, store with `*dst = ...` and
// `dst++` on separate rows (1973/1974), and advance dst's row before src's
// on row 1977 (1824). DC's function-scope dst/src are those row pointers
// (unsigned int* / const unsigned int*, advanced by the byte pitch), here
// the pixel-pair and byte views of Bitmap16MapPointer; the row's pixel
// walkers are register locals, src's loaded first (1962/1965).
// Unrecorded row 1975 is the source increment: `*pixelSrc` is read and
// incremented after the store.
VA(0x006032e0, 0x1E5)
DC_ADDRESS(0x19c3b8, 0x230)
MAC_ADDRESS(0x20ea78, 0x26c)  // anchor-caller
void heroWindowManager::fadeFromBlack(int speed)
{
    const unsigned int maskRed = (Bitmap16Bit::red_mask << 16) | Bitmap16Bit::red_mask;
    const unsigned int maskGreen = (Bitmap16Bit::green_mask << 16) | Bitmap16Bit::green_mask;
    const unsigned int maskBlue = (Bitmap16Bit::blue_mask << 16) | Bitmap16Bit::blue_mask;
    const int fadePeriod = 50;
    Bitmap16Bit fadeFrom(WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT);
    fadeFrom.Grab(m_screenBitmap, 0, 0);

    for (int shift = 2; shift > 0; shift--) {
        unsigned long deadline = GameTime::get() + fadePeriod;
        unsigned long started = GameTime::get();
        Bitmap16MapPointer dst;
        dst.m_pixels = m_screenBitmap->GetMap(0, 0);
        Bitmap16ConstMapPointer src;
        src.m_pixels = fadeFrom.GetMap(0, 0);
        for (int y = 0; y < WINDOW_SCREEN_HEIGHT; y++) {
            const unsigned int* pixelSrc = src.m_pixelPairs;
            unsigned int* pixelDst = dst.m_pixelPairs;
            for (int x = 0; x < WINDOW_SCREEN_WIDTH / 2; x++) {
                unsigned long pair = *pixelSrc;
                unsigned long blue = (pair & maskRed) >> shift;
                unsigned long green = (pair & maskGreen) >> shift;
                unsigned long red = (pair & maskBlue) >> shift;
                *pixelDst = (red & maskBlue) | (green & maskGreen)
                    | (blue & maskRed);
                pixelDst++;
                pixelSrc++;
            }
            dst.m_bytes += m_screenBitmap->GetPitch();
            src.m_bytes += fadeFrom.GetPitch();
        }
        blitToScreenWithPointer(0, 0, WINDOW_SCREEN_WIDTH,
                                WINDOW_SCREEN_HEIGHT);
        if (GameTime::get() - started > fadePeriod)
            break;
        GameTime::delayTil(deadline);
    }

    fadeFrom.Draw(0, 0, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT,
                  m_screenBitmap, 0, 0, false);
    blitToScreenWithPointer(0, 0, WINDOW_SCREEN_WIDTH,
                            WINDOW_SCREEN_HEIGHT);
}

// Mac retains this shared window-state helper; DoDialog, DoDialogDraw and
// DoQuickView call it for the initial sleep and both normal and exception
// cleanup paths, and Windows inlines it there.
MAC_ADDRESS(0x20ece4, 0x70)
void heroWindowManager::sleepAllWindows(unsigned char sleep)
{
    heroWindow* window;
    if (sleep) {
        for (window = m_tailWindow; window; window = window->m_prevWindow)
            window->sleepAllWidgets(1);
    } else {
        for (window = m_headWindow; window; window = window->m_nextWindow)
            window->sleepAllWidgets(0);
    }
}
