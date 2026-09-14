// winmgr.cpp - E:\gamedcs\winmgr.cpp (compiland winmgr.obj)
#include <va.h>
#include "winmgr.h"
#include "message.h"
#include "mousemgr.h"
#include "window.h"
#include "widget.h"
#include "inputmgr.h"
#include "kbwin.h"
#include "soundmgr.h"
// Close deletes both owned bitmaps through the virtual slot-0 tail, so
// this TU needs the COMPLETE Bitmap16Bit.
#include "bitmap16.h"
#include "wingraph.h"
// Open answers a failed screen-bitmap allocation with MemError.
#include "kb.h"
#include "remote.h"

#if 0  // @carcass

// E:\gamedcs\winmgr.cpp:101
DC_ONLY(0x19a840, 0x11A)
int heroWindowManager::Open(int newPriority)
{
    // @stub
}

#endif  // @carcass

// The four screen-geometry slots Open hands to Bitmap16Bit::reference.
// They are read at exactly one site in the whole image - these four
// instructions - and written at none, so nothing names them; the widths are
// the ones `reference(int, int, int, unsigned short*)` imposes.
DATA(0x006aac94) extern int g_unnamed6aac94;
DATA(0x006aac98) extern int g_unnamed6aac98;
DATA(0x006aac9c) extern int g_unnamed6aac9c;
DATA(0x006aaca0) extern unsigned short* g_unnamed6aaca0;

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

    m_screenBitmap->reference(g_unnamed6aac94, g_unnamed6aac98, g_unnamed6aac9c,
                            g_unnamed6aaca0);
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
    heroWindow* w;
    int endFlag;

    if (g_dialogNestCount++ == 0)
        setNoDialogMenus(0);
    try {
        g_inDialog = 1;
        try {
            for (w = m_tailWindow; w; w = w->m_prevWindow)
                w->sleepAllWidgets(1);
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
                for (w = m_headWindow; w; w = w->m_nextWindow)
                    w->sleepAllWidgets(0);
                throw;
            }
            for (w = m_headWindow; w; w = w->m_nextWindow)
                w->sleepAllWidgets(0);
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
    heroWindow* w;
    int endFlag;

    if (g_dialogNestCount++ == 0)
        setNoDialogMenus(0);
    try {
        g_inDialog = 1;
        try {
            for (w = m_tailWindow; w; w = w->m_prevWindow)
                w->sleepAllWidgets(1);
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
                for (w = m_headWindow; w; w = w->m_nextWindow)
                    w->sleepAllWidgets(0);
                throw;
            }
            for (w = m_headWindow; w; w = w->m_nextWindow)
                w->sleepAllWidgets(0);
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
    heroWindow* w;

    g_mouseManager->hidePointer();
    try {
        for (w = m_tailWindow; w; w = w->m_prevWindow)
            w->sleepAllWidgets(1);
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
                    if (g_videoPaused && g_dPlay) {
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
            for (w = m_headWindow; w; w = w->m_nextWindow)
                w->sleepAllWidgets(0);
            throw;
        }
        for (w = m_headWindow; w; w = w->m_nextWindow)
            w->sleepAllWidgets(0);
    } catch (...) {
        g_mouseManager->showPointer(false);
        throw;
    }
    g_mouseManager->showPointer(false);
}

#if 0  // @carcass

// E:\gamedcs\winmgr.cpp:844
DC_ONLY(0x19b1f0, 0x3E)
void heroWindowManager::updateScreen()
{
    // @stub
}

// E:\gamedcs\winmgr.cpp:916
DC_ONLY(0x19b328, 0x9C)
void heroWindowManager::updateScreen(int x, int y, int width, int height, int dx, int dy)
{
    // @stub
}

// E:\gamedcs\winmgr.cpp:964
DC_ONLY(0x19b3c4, 0x2A)
void heroWindowManager::BlitToScreenWithPointer(int x, int y, int w, int h)
{
    // @stub
}

// E:\gamedcs\winmgr.cpp:986
DC_ONLY(0x19b3f0, 0x38)
void heroWindowManager::BlitToScreenWithPointerX(int x, int y, int w, int h, int dx, int dy)
{
    // @stub
}

#endif  // @carcass

VA(0x00602bd0, 0x7C)  // dc 0x19b230
void heroWindowManager::updateScreen(int x, int y, int width, int height)
{
    RECT region;

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
    if (width > 0 && height > 0) {
        region.left = x;
        region.top = y;
        region.right = x + width;
        region.bottom = y + height;
        ddAppBlit(&region);
    }
    pollSound();
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

#if 0  // @carcass

// E:\gamedcs\winmgr.cpp:1051
DC_ONLY(0x19b490, 0x2C)
void heroWindowManager::ScreenShot()
{
    // @stub
}

// E:\gamedcs\winmgr.cpp:1085
DC_ONLY(0x19b4bc, 0x104)
void heroWindowManager::SaveFizzleSource(int startX, int startY, int width, int height)
{
    // @stub
}

// E:\gamedcs\winmgr.cpp:1194
DC_ONLY(0x19b66c, 0x28E)
void heroWindowManager::FizzleForward(int startX, int startY, int width, int height, int iFadeTime)
{
    // @stub
}

// The three rows below have NO retail body. The winmgr tail is
// order-mapped exhaustively: the five carve rows past FadeScreen are
// 0x602cc0 SaveFizzleSourceX, 0x602dc0 FizzleForwardX, 0x6030c0
// ReleaseFizzleSource, 0x6030e0 FadeToBlack, 0x6032e0 FadeFromBlack,
// and the next row (0x6034d0) is a cinit followed by the import-thunk
// run, so the compiland ends there. ScreenShot, SaveFizzleSource,
// FizzleForward, NextFlashFrame, Flash and FadeBlit are all
// retail-dropped or inlined - retail kept only the X half of the
// fizzle pair.

// E:\gamedcs\winmgr.cpp:1455
DC_ONLY(0x19bbd4, 0x80)
void heroWindowManager::NextFlashFrame(int startX, int startY, int width, int height, int iFadeTime)
{
    // @stub
}

// E:\gamedcs\winmgr.cpp:1463
DC_ONLY(0x19bc54, 0x1D2)
void heroWindowManager::Flash(int startX, int startY, int width, int height, int iFadeTime)
{
    // @stub
}

// E:\gamedcs\winmgr.cpp:1545
DC_ONLY(0x19be28, 0x394)
void heroWindowManager::FadeBlit(int sx, int sy, int sw, int sh, const Bitmap816* src_bmp, int dx, int dy, unsigned char tblit, int nframes, int period)
{
    // @stub
}

// E:\gamedcs\Bitmap816.h:73
DC_ONLY(0x19c5e8, 0x8)
const Palette16* Bitmap816::getPalette()
{
    // @stub
}

// E:\gamedcs\Bitmap816.h:104
DC_ONLY(0x19c5f0, 0xE)
const unsigned char* Bitmap816::getMap(int x, int y)
{
    // @stub
}

#endif  // @carcass

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

// ONE `RECT` serves both blit sites - retail writes the same four slots at
// [ebp-0x54] in the frame loop and after it, and two separate declarations
// cost the exact 0x10 of frame the record occupies (0x90 against retail's
// 0x80). Its field order differs between the two sites and that is source:
// left/right/top/bottom inside the loop, left/top/right/bottom after it.

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

            RECT rect;
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
                        int fromRed = *od & g_colorMaskRed;
                        int toRed = *s & g_colorMaskRed;
                        int fromGreen = *od & g_colorMaskGreen;
                        int toGreen = *s & g_colorMaskGreen;
                        int fromBlue = *od & g_colorMaskBlue;
                        int toBlue = *s & g_colorMaskBlue;
                        const int outRed =
                            ((toRed - fromRed) * alpha >> 16) + fromRed;
                        const int outGreen =
                            ((toGreen - fromGreen) * alpha >> 16) + fromGreen;
                        const int outBlue =
                            ((toBlue - fromBlue) * alpha >> 16) + fromBlue;
                        *d = static_cast<unsigned short>(
                            (outRed & g_colorMaskRed)
                            | (outGreen & g_colorMaskGreen)
                            | (outBlue & g_colorMaskBlue));
                        d++;
                        s++;
                        od++;
                    }
                    // Canonical DC GetPitch boundaries (lines 1407/1409).
                    if (row + 1 < height)
                        screen.m_bytes += m_screenBitmap->getPitch();
                    target.m_bytes += destination.getPitch();
                    source.m_bytes += m_bmpFizzleSource->getPitch();
                }

                pollSound();
                rect.left = startX;
                rect.right = startX + width;
                rect.top = startY;
                rect.bottom = startY + height;
                robAppBlit(&rect);
                GameTime::delayTil(deadline);
            }

            destination.draw(0, 0, width, height, m_screenBitmap->getMap(0, 0),
                             startX, startY, m_screenBitmap->getWidth(),
                             m_screenBitmap->getHeight(), m_screenBitmap->getPitch(),
                             false);
            rect.left = startX;
            rect.top = startY;
            rect.right = startX + width;
            rect.bottom = startY + height;
            robAppBlit(&rect);

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

VA(0x006030e0, 0x1F9)  // anchor-caller, dc 0x19c1bc
void heroWindowManager::fadeToBlack(int speed, unsigned char expectFadein)
{
    unsigned long maskBlue = (g_colorMaskBlue << 16) | g_colorMaskBlue;
    unsigned long maskGreen = (g_colorMaskGreen << 16) | g_colorMaskGreen;
    unsigned long maskRed = (g_colorMaskRed << 16) | g_colorMaskRed;
    Bitmap16Bit fadeFrom(WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT);
    RECT screenRect;

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
                unsigned long blue = (pair & maskBlue) >> shift;
                unsigned long green = (pair & maskGreen) >> shift;
                unsigned long red = (pair & maskRed) >> shift;
                dst[x] = (red & maskRed) | (green & maskGreen)
                    | (blue & maskBlue);
            }
            sourceBytes += fadeFrom.getPitch();
            destinationBytes += m_screenBitmap->getPitch();
        }
        screenRect.left = 0;
        screenRect.top = 0;
        screenRect.right = WINDOW_SCREEN_WIDTH;
        screenRect.bottom = WINDOW_SCREEN_HEIGHT;
        ddAppBlit(&screenRect);
        if (GameTime::get() - started > 50)
            break;
        GameTime::delayTil(deadline);
    }

    m_screenBitmap->fillRect(0, 0, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, 0);
    screenRect.left = 0;
    screenRect.top = 0;
    screenRect.right = WINDOW_SCREEN_WIDTH;
    screenRect.bottom = WINDOW_SCREEN_HEIGHT;
    ddAppBlit(&screenRect);
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
    unsigned long maskBlue = (g_colorMaskBlue << 16) | g_colorMaskBlue;
    unsigned long maskGreen = (g_colorMaskGreen << 16) | g_colorMaskGreen;
    unsigned long maskRed = (g_colorMaskRed << 16) | g_colorMaskRed;
    Bitmap16Bit fadeFrom(WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT);
    RECT screenRect;

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
                unsigned long blue = (pair & maskBlue) >> shift;
                unsigned long green = (pair & maskGreen) >> shift;
                unsigned long red = (pair & maskRed) >> shift;
                dst[x] = (red & maskRed) | (green & maskGreen)
                    | (blue & maskBlue);
            }
            sourceBytes += fadeFrom.getPitch();
            destinationBytes += m_screenBitmap->getPitch();
        }
        screenRect.left = 0;
        screenRect.top = 0;
        screenRect.right = WINDOW_SCREEN_WIDTH;
        screenRect.bottom = WINDOW_SCREEN_HEIGHT;
        ddAppBlit(&screenRect);
        if (GameTime::get() - started > 50)
            break;
        GameTime::delayTil(deadline);
    }

    fadeFrom.draw(0, 0, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT,
        m_screenBitmap->getMap(0, 0), 0, 0, m_screenBitmap->getWidth(), m_screenBitmap->getHeight(),
        m_screenBitmap->getPitch(), 0);
    screenRect.left = 0;
    screenRect.top = 0;
    screenRect.right = WINDOW_SCREEN_WIDTH;
    screenRect.bottom = WINDOW_SCREEN_HEIGHT;
    ddAppBlit(&screenRect);
}
