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

VA(0x00602170, 0x38)
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

VA(0x006021b0, 0x114)  // dc 0x19a840
int heroWindowManager::open(int newPriority)
{
    initVideo();

    m_screenBitmap = new Bitmap16Bit(0, 0);
    if (m_screenBitmap == 0)
        memError();

    // DC winmgr.cpp:112 calls these four InitWin accessors. Retail expands
    // them into reads at 0x6aac94/98/9c/a0 within the bitmap at 0x6aac70.
    m_screenBitmap->reference(g_initWin.getWidth(), g_initWin.getHeight(),
                            g_initWin.getPitch(), g_initWin.getMap(0, 0));
    m_screenBitmap->fillRect(0, 0, 800, 600, 0);

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

VA(0x006022d0, 0x46)  // dc 0x19a95c
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

VA(0x00602320, 0x36)  // dc 0x19a9c0
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

VA(0x00602360, 0x10)  // dc 0x19aa08
int heroWindowManager::convertToHover(message& msg)
{
    return main(msg);
}

VA(0x00602370, 0x3B)  // dc 0x19aa20
int heroWindowManager::broadcastMessage(int msgId, int msgCodeX, int msgCodeY, int msgExtra)
{
    message msg;

    msg.m_qualifier = 0;
    msg.m_mouseX = 0;
    msg.m_mouseY = 0;
    msg.m_window = 0;
    msg.m_id = msgId;
    msg.m_codeX = msgCodeX;
    msg.m_codeY = msgCodeY;
    msg.m_extra = msgExtra;
    return main(msg);
}

VA(0x006023b0, 0xE7)  // dc 0x19aa64
void heroWindowManager::addWindow(heroWindow* newWindow, int newPriority,
                                  unsigned char update)
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

VA(0x006024a0, 0x78)  // dc 0x19ac14
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
// FOUR try levels, read off the unwind funclets the carve split out at
// 0x602735 / 0x60274e / 0x60276d / 0x602780 - each re-runs one cleanup
// and then tail-calls _CxxThrowException(0,0), a RETHROW. Innermost
// first they are: RemoveWindow, the SleepAllWidgets(0) walk,
// gbInDialog = 0, and the nest-count decrement, which fixes both the
// nesting and where each try opens (the [ebp-4] walk 0/1/2/3 lands
// exactly on those four boundaries).
VA(0x00602520, 0x280)  // anchor-global, dc 0x19ad18
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
// DoDialog's sibling, same four try levels (funclets 0x6029cf /
// 0x6029e8 / 0x602a07 / 0x602a1a). Four deliberate divergences from
// DoDialog, all byte-forced: the draw callback runs once before the
// fade; the window's CONSUME verdict does NOT short-circuit the
// handler; the level-2 body flushes the input queue after RemoveWindow;
// and the handler's verdict is a SWITCH, not an if/else chain - the
// chain lays the WIDGET_END_DIALOG arm out first and falls into it,
// while retail sinks it past the redraw arm behind a forward `je`,
// which is exactly VC6's two-case switch layout.
VA(0x006027a0, 0x29A)  // anchor-global, dc 0x19aef4
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

VA(0x00602a40, 0x188)  // dc 0x19b0fc
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

// Original: heroWindowManager::UpdateScreen; winmgr.cpp:844, dc 0x19b1f0.
// Complete draws combat and adventure into the same 800x600 client surface.
// DC's combat-only (8,32) destination branch belongs to its translated viewport.
void heroWindowManager::updateScreen()
{
    updateScreen(0, 0, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT);
}

VA(0x00602bd0, 0x7C)  // dc 0x19b230
void heroWindowManager::updateScreen(int x, int y, int width, int height)
{
    if (m_isWaitingForFadeIn)
        return;
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
    pollSound();
}

// Original: heroWindowManager::BlitToScreenWithPointer; winmgr.cpp:964, dc 0x19b3c4.
// Complete UpdateScreen0x602bd0 expands this rectangle construction and calls
// RobAppBlit0x5ffe70, as do FadeToBlack0x6030e0 and FadeIn0x6032e0.
// The older DC DDAppBlit(const RECT&) null/global-mdr1 wrapper is console-only.
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

VA(0x00602c50, 0x63)  // dc 0x19b428
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

// Original: heroWindowManager::ScreenShot; winmgr.cpp:1051, dc 0x19b490.
// DC release retains only the counter/name and input flush; it emits no image
// export call. Preserve that observed operation without inventing an exporter
// or assigning an unproven Complete address to the counter.
void heroWindowManager::screenShot()
{
    char name[16];
    sprintf(name, "SHOT%04d.PCX", g_currScreenShot);
    ++g_currScreenShot;
    g_inputManager->flush();
}

// Original: heroWindowManager::SaveFizzleSource; winmgr.cpp:1085, dc 0x19b4bc.
// The ordinary non-X interface shares the saved bitmap with the retained X
// family. Neither debug nor retail evidence proves a distinct Complete body.
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
    m_bmpFizzleSource->grab(g_windowManager->m_screenBitmap, startX, startY);
}

VA(0x00602cc0, 0xF2)  // dc 0x19b5c0
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
            m_bmpFizzleSource->grab(g_windowManager->m_screenBitmap->getMap(0, 0),
                           startX, startY,
                           g_windowManager->m_screenBitmap->getWidth(),
                           g_windowManager->m_screenBitmap->getHeight(),
                           g_windowManager->m_screenBitmap->getPitch());
        }
    }
}

// Original: heroWindowManager::FizzleForward; winmgr.cpp:1194, dc 0x19b66c.
// DC proves four frames and a 10ms default for this non-X operation. The
// retained X interface independently has eight frames and a 33ms default.
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
    target.grab(m_screenBitmap, startX, startY);
    for (int frame = 0; frame < 4; ++frame) {
        unsigned long nextFrameTime = GameTime::get() + fadeTime;
        const int factor = (frame << 16) / 4;
        Bitmap16ConstMapPointer source;
        source.m_pixels = target.getMap(0, 0);
        Bitmap16MapPointer destination;
        destination.m_pixels = m_screenBitmap->getMap(startX, startY);
        Bitmap16ConstMapPointer oldDestination;
        oldDestination.m_pixels = m_bmpFizzleSource->getMap(0, 0);
        for (int y = 0; y < height; ++y) {
            unsigned short* d = destination.m_pixels;
            const unsigned short* s = source.m_pixels;
            const unsigned short* od = oldDestination.m_pixels;
            for (int x = 0; x < width; ++x) {
                const int oldRed = *od & Bitmap16Bit::s_blueMask;
                const int red = *s & Bitmap16Bit::s_blueMask;
                const int oldGreen = *od & Bitmap16Bit::s_greenMask;
                const int green = *s & Bitmap16Bit::s_greenMask;
                const int oldBlue = *od & Bitmap16Bit::s_redMask;
                const int blue = *s & Bitmap16Bit::s_redMask;
                *d = static_cast<unsigned short>(
                    ((((red - oldRed) * factor >> 16) + oldRed) & Bitmap16Bit::s_blueMask)
                    | ((((green - oldGreen) * factor >> 16) + oldGreen) & Bitmap16Bit::s_greenMask)
                    | ((((blue - oldBlue) * factor >> 16) + oldBlue) & Bitmap16Bit::s_redMask));
                ++d;
                ++s;
                ++od;
            }
            destination.m_bytes += m_screenBitmap->getPitch();
            source.m_bytes += target.getPitch();
            oldDestination.m_bytes += m_bmpFizzleSource->getPitch();
        }
        pollSound();
        blitToScreenWithPointer(startX, startY, width, height);
        GameTime::delayTil(nextFrameTime);
    }
    target.draw(0, 0, width, height, m_screenBitmap, startX, startY, false);
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

// Mac retains BlitToScreenWithPointer at both update sites. The Windows
// optimizer expands the same helper's rectangle setup into this caller.

VA(0x00602dc0, 0x2F7)  // anchor-import + exhaustive tail order, dc 0x19b8fc
void heroWindowManager::fizzleForwardX(int startX, int startY, int width,
                                       int height, int fadeTime)
{
    // DC locals: DEFAULT_FADE_TIME, src/odst (read-only row pointers).
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
            destination.grab(m_screenBitmap->getMap(0, 0), startX, startY,
                             m_screenBitmap->getWidth(), m_screenBitmap->getHeight(),
                             m_screenBitmap->getPitch());

            for (int frame = 0; frame < 8; frame++) {
                unsigned long deadline = GameTime::get() + fadeTime;
                int alpha = (frame << 16) / 8;

                Bitmap16ConstMapPointer source;
                source.m_pixels = m_bmpFizzleSource->getMap(0, 0);
                Bitmap16ConstMapPointer target;
                target.m_pixels = destination.getMap(0, 0);
                Bitmap16MapPointer screen;
                screen.m_pixels = m_screenBitmap->getMap(startX, startY);

                for (int row = 0; row < height; row++) {
                    unsigned short* d = screen.m_pixels;
                    const unsigned short* s = target.m_pixels;
                    const unsigned short* od = source.m_pixels;
                    for (int col = 0; col < width; col++) {
                        int fromBlue = *od & Bitmap16Bit::s_blueMask;
                        int toBlue = *s & Bitmap16Bit::s_blueMask;
                        int fromGreen = *od & Bitmap16Bit::s_greenMask;
                        int toGreen = *s & Bitmap16Bit::s_greenMask;
                        int fromRed = *od & Bitmap16Bit::s_redMask;
                        int toRed = *s & Bitmap16Bit::s_redMask;
                        const int outBlue =
                            ((toBlue - fromBlue) * alpha >> 16) + fromBlue;
                        const int outGreen =
                            ((toGreen - fromGreen) * alpha >> 16) + fromGreen;
                        const int outRed =
                            ((toRed - fromRed) * alpha >> 16) + fromRed;
                        *d = static_cast<unsigned short>(
                            (outBlue & Bitmap16Bit::s_blueMask)
                            | (outGreen & Bitmap16Bit::s_greenMask)
                            | (outRed & Bitmap16Bit::s_redMask));
                        d++;
                        s++;
                        od++;
                    }
                    // Canonical DC GetPitch boundaries (lines 1407/1409).
                    screen.m_bytes += m_screenBitmap->getPitch();
                    target.m_bytes += destination.getPitch();
                    source.m_bytes += m_bmpFizzleSource->getPitch();
                }

                pollSound();
                blitToScreenWithPointer(startX, startY, width, height);
                GameTime::delayTil(deadline);
            }

            destination.draw(0, 0, width, height, m_screenBitmap->getMap(0, 0),
                             startX, startY, m_screenBitmap->getWidth(),
                             m_screenBitmap->getHeight(), m_screenBitmap->getPitch(),
                             false);
            blitToScreenWithPointer(startX, startY, width, height);

            m_colorCyclingOn = savedColorCycling;
            // DC line 1434 calls the ordinary helper; Complete expands it.
            releaseFizzleSource();
        }
    }
}

VA(0x006030c0, 0x19)  // dc 0x19bba8
void heroWindowManager::releaseFizzleSource()
{
    if (m_bmpFizzleSource)
        delete m_bmpFizzleSource;
    m_bmpFizzleSource = 0;
}

// E:\gamedcs\winmgr.cpp:1707 - the fade-out. Located by anchor-caller:
// the already-exact FadeScreen calls this at 0x602c78.

// The three colour masks are widened into both halves of a dword so one
// pass handles two 16-bit pixels; the fade itself is three passes that
// shift every component right by the pass index and re-mask it into its
// own field, so pass 0 is a plain copy. `speed` is dropped on the floor
// - the pacing is the fixed 50 ms per pass below - but the parameter is
// real: FadeScreen forwards it and `ret 8` fixes the frame at two
// dwords.

// Residual (88.51%): one register-allocation choice in the pixel loop,
// and its cascade. Retail keeps the loop's shift count in ebx and the
// SOURCE pointer in edi, hoists `pDst - pSrc` into a per-row delta slot
// and addresses the store as `[delta + src - 4]`; our CL puts the shift
// in edi, the source pointer in ebx, and keeps a second live dst
// pointer instead of the delta. Frame layout, every mask/counter/timer
// slot displacement and the whole post-loop tail are byte-identical.

// Original: heroWindowManager::NextFlashFrame; winmgr.cpp:1455, dc 0x19bbd4.
void heroWindowManager::nextFlashFrame(int startX, int startY, int width,
                                      int height, int fadeTime)
{
    unsigned long nextFrameTime = GameTime::get() + fadeTime;
    pollSound();
    blitToScreenWithPointer(startX, startY, width, height);
    GameTime::delayTil(nextFrameTime);
}

// Original: heroWindowManager::Flash; winmgr.cpp:1463, dc 0x19bc54.
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
    target.grab(m_screenBitmap, startX, startY);
    nextFlashFrame(startX, startY, width, height, fadeTime);
    m_bmpFizzleSource->draw(0, 0, width, height, m_screenBitmap, startX, startY, false);
    nextFlashFrame(startX, startY, width, height, fadeTime);
    target.draw(0, 0, width, height, m_screenBitmap, startX, startY, false);
    nextFlashFrame(startX, startY, width, height, fadeTime);
    m_bmpFizzleSource->draw(0, 0, width, height, m_screenBitmap, startX, startY, false);
    nextFlashFrame(startX, startY, width, height, fadeTime);
    target.draw(0, 0, width, height, m_screenBitmap, startX, startY, false);
    nextFlashFrame(startX, startY, width, height, fadeTime);
    m_bmpFizzleSource->draw(0, 0, width, height, m_screenBitmap, startX, startY, false);
    blitToScreenWithPointer(startX, startY, width, height);
    m_colorCyclingOn = savedColorCycling;
    releaseFizzleSource();
}
// Original: heroWindowManager::FadeBlit; winmgr.cpp:1545, dc 0x19be28.
// DC records separate transparent/opaque loops, palette lookup and three
// component interpolation. Complete's bitmap/palette interfaces retain those
// operations, but no standalone address is claimed for this older entry point.
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
    if (dx + sw > m_screenBitmap->getWidth())
        sw = m_screenBitmap->getWidth() - dx;
    if (dy + sh > m_screenBitmap->getHeight())
        // DC line 1582 really calls GetWidth here, despite testing Height.
        sh = m_screenBitmap->getWidth() - dy;
    if (sw <= 0 || sh <= 0)
        return;

    Bitmap16Bit savedDest(sw, sh);
    savedDest.grab(m_screenBitmap, dx, dy);
    const unsigned short* sourcePalette = srcBitmap->getPalette().m_data;
    for (int frame = 1; frame <= frames; ++frame) {
        unsigned long nextFrameTime = GameTime::get() + period;
        const int factor = (frame << 16) / frames;
        const unsigned char* source = srcBitmap->getMap(sx, sy);
        Bitmap16MapPointer destination;
        destination.m_pixels = m_screenBitmap->getMap(dx, dy);
        Bitmap16ConstMapPointer oldDestination;
        oldDestination.m_pixels = savedDest.getMap(0, 0);
        if (transparent) {
            for (int y = 0; y < sh; ++y) {
                const unsigned char* s = source;
                unsigned short* d = destination.m_pixels;
                const unsigned short* od = oldDestination.m_pixels;
                for (int x = 0; x < sw; ++x) {
                    if (*s) {
                        unsigned short color = sourcePalette[*s];
                        const int oldRed = *od & Bitmap16Bit::s_blueMask;
                        const int red = color & Bitmap16Bit::s_blueMask;
                        const int oldGreen = *od & Bitmap16Bit::s_greenMask;
                        const int green = color & Bitmap16Bit::s_greenMask;
                        const int oldBlue = *od & Bitmap16Bit::s_redMask;
                        const int blue = color & Bitmap16Bit::s_redMask;
                        *d = static_cast<unsigned short>(
                            ((((red - oldRed) * factor >> 16) + oldRed) & Bitmap16Bit::s_blueMask)
                            | ((((green - oldGreen) * factor >> 16) + oldGreen) & Bitmap16Bit::s_greenMask)
                            | ((((blue - oldBlue) * factor >> 16) + oldBlue) & Bitmap16Bit::s_redMask));
                    }
                    ++d;
                    ++s;
                    ++od;
                }
                destination.m_bytes += m_screenBitmap->getPitch();
                source += srcBitmap->getPitch();
                oldDestination.m_bytes += savedDest.getPitch();
            }
        } else {
            for (int y = 0; y < sh; ++y) {
                const unsigned char* s = source;
                unsigned short* d = destination.m_pixels;
                const unsigned short* od = oldDestination.m_pixels;
                for (int x = 0; x < sw; ++x) {
                    unsigned short color = sourcePalette[*s];
                    const int oldRed = *od & Bitmap16Bit::s_blueMask;
                    const int red = color & Bitmap16Bit::s_blueMask;
                    const int oldGreen = *od & Bitmap16Bit::s_greenMask;
                    const int green = color & Bitmap16Bit::s_greenMask;
                    const int oldBlue = *od & Bitmap16Bit::s_redMask;
                    const int blue = color & Bitmap16Bit::s_redMask;
                    *d = static_cast<unsigned short>(
                        ((((red - oldRed) * factor >> 16) + oldRed) & Bitmap16Bit::s_blueMask)
                        | ((((green - oldGreen) * factor >> 16) + oldGreen) & Bitmap16Bit::s_greenMask)
                        | ((((blue - oldBlue) * factor >> 16) + oldBlue) & Bitmap16Bit::s_redMask));
                    ++d;
                    ++s;
                    ++od;
                }
                destination.m_bytes += m_screenBitmap->getPitch();
                source += srcBitmap->getPitch();
                oldDestination.m_bytes += savedDest.getPitch();
            }
        }
        updateScreen(dx, dy, sw, sh);
        GameTime::delayTil(nextFrameTime);
    }
}

VA(0x006030e0, 0x1F9)  // anchor-caller, dc 0x19c1bc
void heroWindowManager::fadeToBlack(int speed, unsigned char expectFadein)
{
    unsigned long maskRed = (Bitmap16Bit::s_redMask << 16) | Bitmap16Bit::s_redMask;
    unsigned long maskGreen = (Bitmap16Bit::s_greenMask << 16) | Bitmap16Bit::s_greenMask;
    unsigned long maskBlue = (Bitmap16Bit::s_blueMask << 16) | Bitmap16Bit::s_blueMask;
    Bitmap16Bit fadeFrom(WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT);
    fadeFrom.grab(m_screenBitmap->getMap(0, 0), 0, 0, m_screenBitmap->getWidth(),
        m_screenBitmap->getHeight(), m_screenBitmap->getPitch());

    for (int shift = 0; shift < 3; shift++) {
        unsigned long deadline = GameTime::get() + 50;
        unsigned long started = GameTime::get();
        unsigned char* sourceBytes = static_cast<unsigned char*>(
            static_cast<void*>(fadeFrom.getMap(0, 0)));
        unsigned char* destinationBytes = static_cast<unsigned char*>(
            static_cast<void*>(m_screenBitmap->getMap(0, 0)));
        for (int y = 0; y < WINDOW_SCREEN_HEIGHT; y++) {
            unsigned long* src = static_cast<unsigned long*>(
                static_cast<void*>(sourceBytes));
            unsigned int* dst = static_cast<unsigned int*>(
                static_cast<void*>(destinationBytes));
            for (int x = 0; x < WINDOW_SCREEN_WIDTH / 2; x++) {
                unsigned long pair = *src++;
                unsigned long blue = (pair & maskRed) >> shift;
                unsigned long green = (pair & maskGreen) >> shift;
                unsigned long red = (pair & maskBlue) >> shift;
                dst[x] = (red & maskBlue) | (green & maskGreen)
                    | (blue & maskRed);
            }
            sourceBytes += fadeFrom.getPitch();
            destinationBytes += m_screenBitmap->getPitch();
        }
        blitToScreenWithPointer(0, 0, WINDOW_SCREEN_WIDTH,
                                WINDOW_SCREEN_HEIGHT);
        if (GameTime::get() - started > 50)
            break;
        GameTime::delayTil(deadline);
    }

    m_screenBitmap->fillRect(0, 0, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, 0);
    blitToScreenWithPointer(0, 0, WINDOW_SCREEN_WIDTH,
                            WINDOW_SCREEN_HEIGHT);
    if (expectFadein) {
        fadeFrom.draw(0, 0, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT,
            m_screenBitmap->getMap(0, 0), 0, 0, m_screenBitmap->getWidth(),
            m_screenBitmap->getHeight(), m_screenBitmap->getPitch(), 0);
    }
}

// E:\gamedcs\winmgr.cpp:1866 - the fade-in, FadeScreen's second call at
// 0x602c91. The mirror of FadeToBlack: the same three-pass shift walked
// DOWNWARDS from 2 and stopping before 0, so the last pass is the
// half-brightness frame and the full-brightness image is restored by
// the Draw below rather than by a pass of its own.

VA(0x006032e0, 0x1E5)  // anchor-caller, dc 0x19c3b8
void heroWindowManager::fadeFromBlack(int speed)
{
    unsigned long maskRed = (Bitmap16Bit::s_redMask << 16) | Bitmap16Bit::s_redMask;
    unsigned long maskGreen = (Bitmap16Bit::s_greenMask << 16) | Bitmap16Bit::s_greenMask;
    unsigned long maskBlue = (Bitmap16Bit::s_blueMask << 16) | Bitmap16Bit::s_blueMask;
    Bitmap16Bit fadeFrom(WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT);
    fadeFrom.grab(m_screenBitmap->getMap(0, 0), 0, 0, m_screenBitmap->getWidth(),
        m_screenBitmap->getHeight(), m_screenBitmap->getPitch());

    for (int shift = 2; shift > 0; shift--) {
        unsigned long deadline = GameTime::get() + 50;
        unsigned long started = GameTime::get();
        unsigned char* sourceBytes = static_cast<unsigned char*>(
            static_cast<void*>(fadeFrom.getMap(0, 0)));
        unsigned char* destinationBytes = static_cast<unsigned char*>(
            static_cast<void*>(m_screenBitmap->getMap(0, 0)));
        for (int y = 0; y < WINDOW_SCREEN_HEIGHT; y++) {
            const unsigned int* src = static_cast<const unsigned int*>(
                static_cast<void*>(sourceBytes));
            unsigned int* dst = static_cast<unsigned int*>(
                static_cast<void*>(destinationBytes));
            for (int x = 0; x < WINDOW_SCREEN_WIDTH / 2; x++) {
                unsigned long pair = *src++;
                unsigned long blue = (pair & maskRed) >> shift;
                unsigned long green = (pair & maskGreen) >> shift;
                unsigned long red = (pair & maskBlue) >> shift;
                dst[x] = (red & maskBlue) | (green & maskGreen)
                    | (blue & maskRed);
            }
            sourceBytes += fadeFrom.getPitch();
            destinationBytes += m_screenBitmap->getPitch();
        }
        blitToScreenWithPointer(0, 0, WINDOW_SCREEN_WIDTH,
                                WINDOW_SCREEN_HEIGHT);
        if (GameTime::get() - started > 50)
            break;
        GameTime::delayTil(deadline);
    }

    fadeFrom.draw(0, 0, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT,
        m_screenBitmap->getMap(0, 0), 0, 0, m_screenBitmap->getWidth(), m_screenBitmap->getHeight(),
        m_screenBitmap->getPitch(), 0);
    blitToScreenWithPointer(0, 0, WINDOW_SCREEN_WIDTH,
                            WINDOW_SCREEN_HEIGHT);
}

// Mac retains this shared window-state helper at code 0+0x20ece4. DoDialog,
// DoDialogDraw and DoQuickView each call it for the initial sleep and both
// normal and exception cleanup paths. The Windows loops were expanded in
// those callers. Dreamcast records the caller layouts but no helper name.
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
