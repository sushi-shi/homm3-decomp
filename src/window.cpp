#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "window.h"

#include "bitmap16.h"
#include "kb.h"
#include "message.h"
#include "mousemgr.h"
#include "resourcemanager.h"
#include "smackmgr.h"
#include "soundmgr.h"
#include "textresource.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// Complete's 37-row window-text routing table.  Its initialized bytes occupy
// 0x68c710..0x68c837; the immediately following jktext.txt literal at
// 0x68c838 bounds the array independently.  SetWinText's +0/+2/+4 loads and
// 8-byte stride prove the layout, and the values below are the pinned retail
// initializer rather than an inferred enumeration.
DATA(0x0068c710)
static SWinSetup g_winSetup[37] = {
    { 0x06, 0x0067, 0 }, { 0x06, 0x0068, 0 },
    { 0x06, 0x0069, 0 }, { 0x06, 0x006a, 0 },
    { 0x06, 0x006b, 0 }, { 0x06, 0x006c, 0 },
    { 0x06, 0x006d, 0 }, { 0x06, 0x006e, 0 },
    { 0x06, 0x006f, 0 }, { 0x09, 0x0029, 0 },
    { 0x0c, 0x0001, 0 }, { 0x0c, 0x0002, 0 },
    { 0x0e, 0x0320, 0 }, { 0x0e, 0x0321, 0 },
    { 0x0e, 0x0322, 0 }, { 0x0e, 0x0323, 0 },
    { 0x0e, 0x0324, 0 }, { 0x0e, 0x0325, 0 },
    { 0x0e, 0x0326, 0 }, { 0x0e, 0x0327, 0 },
    { 0x0e, 0x025c, 0 }, { 0x0e, 0x025d, 0 },
    { 0x0e, 0x025e, 0 }, { 0x0e, 0x025f, 0 },
    { 0x0e, 0x0260, 0 }, { 0x0e, 0x0261, 0 },
    { 0x0e, 0x0262, 0 }, { 0x0e, 0x0263, 0 },
    { 0x0e, 0x0264, 0 }, { 0x0e, 0x026c, 0 },
    { 0x0e, 0x026d, 0 }, { 0x0e, 0x026e, 0 },
    { 0x16, 0x0001, 0 }, { 0x16, 0x0003, 0 },
    { 0x18, 0x0001, 0 }, { 0x18, 0x0002, 0 },
    { 0x19, 0x0002, 0 }
};

VA(0x005fe9f0, 0x5E) MAC_ADDRESS(0x20ae1c, 0xa0)  // dc 0x197138
heroWindow::heroWindow(int winX, int winY, int winWidth, int winHeight, unsigned winType)
    : m_sleepCount(0)
{
    m_nextWindow = m_prevWindow = 0;
    m_priority = -1;
    m_x = winX;
    m_y = winY;
    m_width = winWidth;
    m_height = winHeight;
    m_type = winType;
    m_status = 0;
    m_headWidget = m_tailWidget = 0;
    m_background = 0;
    m_focusId = -1;
}

VA_COMPGEN(0x005fea50, 0x21, SCALAR_DELETING_DTOR, heroWindow)

VA(0x005fea80, 0x60) MAC_ADDRESS(0x20aebc, 0x94)  // dc 0x1971d0
heroWindow::~heroWindow()
{
    if (m_background)
        delete m_background;
}

VA(0x005feae0, 0x17A) MAC_ADDRESS(0x20af50, 0x208)  // dc 0x19721c
int heroWindow::open(int newPriority, unsigned char update)
{
    if (m_status & WINDOW_STATE_OPEN)
        return 3;
    if (m_type & WINDOW_FLAG_SAVE_BACKGROUND) {
        if (saveBackground())
            return 3;
    }
    m_priority = newPriority;
    if (m_type & WINDOW_FLAG_SHADOWED) {
        g_windowManager->m_screenBitmap->darken(m_x + m_width, m_y + 9, 7, m_height - 9);
        g_windowManager->m_screenBitmap->darken(m_x + m_width, m_y + 8, 8, m_height - 8);
        g_windowManager->m_screenBitmap->darken(m_x + 9, m_y + m_height, m_width - 2, 7);
        g_windowManager->m_screenBitmap->darken(m_x + 8, m_y + m_height, m_width, 8);
    }
    if (videoPlaying()) {
        drawWindow(0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        videoDrawCurrentFrame();
        if (update && !(m_type & WINDOW_FLAG_FIXED_LAYER)) {
            if (m_type & WINDOW_FLAG_SHADOWED)
                g_windowManager->updateScreen(m_x, m_y, m_width + 8, m_height + 8);
            else
                g_windowManager->updateScreen(m_x, m_y, m_width, m_height);
        }
    } else {
        drawWindow(update, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    }
    m_status = WINDOW_STATE_OPEN;
    return 0;
}

VA(0x005fec60, 0x49) MAC_ADDRESS(0x20b158, 0x78)  // dc 0x1972e0
void heroWindow::close(unsigned char update)
{
    if ((m_type & WINDOW_FLAG_SAVE_BACKGROUND) && (m_status & WINDOW_STATE_OPEN))
        restoreBackground(update);
    widget* current = m_tailWidget;
    while (current) {
        widget* prev = current->m_prevWidget;
        removeWidget(current);
        current = prev;
    }
    m_status = 0;
}

// Original: heroWindow::handle_message; window.cpp:194, dc 0x19731c.
// Retail base vtable0x643cc4 slot3 shares the return-zero body0x4ec560.
// Mac retains this body at 0x20b1d0; CSingleSelPopup::handleMessage calls it.
MAC_ADDRESS(0x20b1d0, 0x8)
int heroWindow::handleMessage(message& msg)
{
    return 0;
}

// Original: heroWindow::handle_widget_hover; window.cpp:202, dc 0x197320.
// Base vtable slot4 shares the empty ret4 body0x485d80.
void heroWindow::handleWidgetHover(widget* current)
{
}

VA(0x005fecb0, 0xA5) MAC_ADDRESS(0x20b1dc, 0x124)  // dc 0x197324
void heroWindow::addWidget(widget* newWidget, int newPriority)
{
    widget* current = m_tailWidget;
    if (newPriority == -1) {
        if (!current)
            newPriority = 0;
        else
            newPriority = current->m_priority + 1;
    }
    if (newWidget->open(newPriority, this))
        return;
    while (current && current->m_priority > newPriority)
        current = current->m_prevWidget;
    if (!current) {
        newWidget->m_nextWidget = m_headWidget;
        newWidget->m_prevWidget = 0;
        m_headWidget = newWidget;
        if (!m_tailWidget)
            m_tailWidget = newWidget;
    } else if (!current->m_nextWidget) {
        newWidget->m_prevWidget = m_tailWidget;
        newWidget->m_nextWidget = 0;
        m_tailWidget->m_nextWidget = newWidget;
        m_tailWidget = newWidget;
    } else {
        newWidget->m_prevWidget = current;
        newWidget->m_nextWidget = current->m_nextWidget;
        current->m_nextWidget->m_prevWidget = newWidget;
        current->m_nextWidget = newWidget;
    }
}

VA(0x005fed60, 0x7A) MAC_ADDRESS(0x20b300, 0xe8)  // dc 0x1973b4
void heroWindow::removeWidget(widget* killWidget)
{
    if (!killWidget)
        return;
    killWidget->close();
    if (killWidget == m_headWidget) {
        widget* next = killWidget->m_nextWidget;
        m_headWidget = next;
        if (!next)
            m_tailWidget = 0;
        else
            next->m_prevWidget = 0;
    } else if (killWidget == m_tailWidget) {
        widget* prev = killWidget->m_prevWidget;
        m_tailWidget = prev;
        prev->m_nextWidget = 0;
    } else {
        killWidget->m_prevWidget->m_nextWidget = killWidget->m_nextWidget;
        killWidget->m_nextWidget->m_prevWidget = killWidget->m_prevWidget;
    }
    widget* prev = killWidget->m_prevWidget;
    if (!prev) {
        m_headWidget = m_tailWidget = 0;
    } else {
        widget* next = killWidget->m_nextWidget;
        prev->m_nextWidget = next;
        if (next)
            next->m_prevWidget = prev;
    }
}

// Original: heroWindow::RemoveAndDeleteWidget; window.cpp:346, dc 0x19742c.
// The recorded release body only unlinks matching widgets; the source-line
// gap after RemoveWidget does not establish a missing delete statement.
void heroWindow::removeAndDeleteWidget(int id)
{
    widget* current = m_headWidget;
    while (current) {
        widget* next = current->m_nextWidget;
        if (current->m_id == id)
            removeWidget(current);
        current = next;
    }
}

// E:\gamedcs\window.cpp:381
// DC's message& parameter and GetWidget(m_focusId) call at 391 are canonical.
// The two-state helper control preserves all retained function bytes; retail
// expands GetWidget's tailWidget/prevWidget walk here. DC413's combat-over
// handling is absent from Complete's 94-byte body: retail goes directly from
// the focused widget's result to the ordinary widget traversal.
// The /Ob2 witness: this body recurs inlined inside the 4-int
// overload and both WidgetSet/ClearStatus while the out-of-line copy
// still serves the external callers - auto-inlining with
// unconditional emission (see the profile note in units.toml).
VA(0x005fede0, 0x5E) MAC_ADDRESS(0x20b3e8, 0xc4)  // linkorder bracket; widget Main-slot calls byte-proven, dc 0x197480
int heroWindow::broadcastMessage(message& msg)
{
    int result = 0;
    widget* current = m_tailWidget;
    if (m_focusId != -1) {
        widget* focused = getWidget(m_focusId);
        if (focused) {
            result = focused->main(msg);
            if (result)
                return result;
        }
    }
    while (current) {
        result = current->main(msg);
        if (result > 0 && result <= 2)
            return result;
        current = current->m_prevWidget;
    }
    return result;
}

VA(0x005fee40, 0x8C) MAC_ADDRESS(0x20b4ac, 0x58)  // dc 0x197530
int heroWindow::broadcastMessage(int id, int codeX, int codeY, int extra)
{
    message msg;
    msg.m_id = id;
    msg.m_codeX = codeX;
    msg.m_codeY = codeY;
    msg.m_qualifier = 0;
    msg.m_mouseX = 0;
    msg.m_mouseY = 0;
    msg.m_extra = extra;
    msg.m_window = 0;
    return broadcastMessage(msg);
}

VA(0x005feed0, 0x8E) MAC_ADDRESS(0x20b504, 0x30)  // dc 0x197570
int heroWindow::widgetSetStatus(int id, int status)
{
    return broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_STATUS,
                            id, status);
}

VA(0x005fef60, 0x8E) MAC_ADDRESS(0x20b534, 0x30)  // dc 0x19758c
int heroWindow::widgetClearStatus(int id, int status)
{
    return broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_CLEAR_STATUS,
                            id, status);
}

VA(0x005feff0, 0x22) MAC_ADDRESS(0x20b564, 0x28)  // dc 0x1975a8
widget* heroWindow::getWidget(int id)
{
    widget* current = m_tailWidget;
    while (current) {
        if (current->m_id == id)
            return current;
        current = current->m_prevWidget;
    }
    return 0;
}

VA(0x005ff020, 0xDE) MAC_ADDRESS(0x20b58c, 0x144)  // dc 0x1975d8
void heroWindow::drawWindow(unsigned char update, int lowID, int highID)
{
    message msg;
    msg.m_codeY = 0;
    msg.m_qualifier = 0;
    msg.m_mouseX = 0;
    msg.m_mouseY = 0;
    msg.m_extra = 0;
    msg.m_window = 0;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_DRAW;
    widget* current = m_headWidget;
    while (current) {
        pollSound();
        if (lowID == WINDOW_ALL_WIDGETS_LOW && highID == WINDOW_ALL_WIDGETS_HIGH)
            current->main(msg);
        else if (lowID <= current->m_id && current->m_id <= highID)
            current->main(msg);
        current = current->m_nextWidget;
    }
    if (update && !(m_type & WINDOW_FLAG_FIXED_LAYER)) {
        videoDrawCurrentFrame();
        if (m_type & WINDOW_FLAG_SHADOWED)
            g_windowManager->updateScreen(m_x, m_y, m_width + 8, m_height + 8);
        else
            g_windowManager->updateScreen(m_x, m_y, m_width, m_height);
    }
}

VA(0x005ff100, 0xBD) MAC_ADDRESS(0x20b6d0, 0xc0)  // dc 0x19776c
int heroWindow::saveBackground()
{
    if (m_type & WINDOW_FLAG_SHADOWED)
        m_background = new Bitmap16Bit(m_width + 8, m_height + 8);
    else
        m_background = new Bitmap16Bit(m_width, m_height);
    m_background->grab(g_windowManager->m_screenBitmap->getMap(0, 0), m_x, m_y,
                     g_windowManager->m_screenBitmap->getWidth(),
                     g_windowManager->m_screenBitmap->getHeight(),
                     g_windowManager->m_screenBitmap->getPitch());
    return 0;
}

VA(0x005ff1c0, 0x7E) MAC_ADDRESS(0x20b790, 0xe0)  // dc 0x1977dc
void heroWindow::restoreBackground(unsigned char update)
{
    if (!m_background)
        return;
    m_background->draw(0, 0, m_background->getWidth(), m_background->getHeight(),
                     g_windowManager->m_screenBitmap->getMap(0, 0), m_x, m_y,
                     g_windowManager->m_screenBitmap->getWidth(),
                     g_windowManager->m_screenBitmap->getHeight(),
                     g_windowManager->m_screenBitmap->getPitch(), 0);
    if (update)
        g_windowManager->updateScreen(m_x, m_y, m_background->getWidth(), m_background->getHeight());
    delete m_background;
    m_background = 0;
}

// Original: heroWindow::MoveWindow; window.cpp:707, dc 0x197874.
// This relative-motion API uses the same saved-background operations as
// Complete's retained CenterWindow0x5ff240, with desktop800x600 clipping.
// No retained standalone MoveWindow address is claimed.
void heroWindow::moveWindow(int deltaX, int deltaY)
{
    int startX = m_x;
    int startY = m_y;
    int newX = m_x + deltaX;
    int newY = m_y + deltaY;
    int startW = m_width;
    int startH = m_height;
    if (newX < 0)
        newX = 0;
    if (newY < 0)
        newY = 0;
    if (m_width + newX > WINDOW_SCREEN_WIDTH)
        newX = WINDOW_SCREEN_WIDTH - m_width;
    if (m_height + newY > WINDOW_SCREEN_HEIGHT)
        newY = WINDOW_SCREEN_HEIGHT - m_height;
    m_background->draw(0, 0, m_background->getWidth(), m_background->getHeight(),
                       g_windowManager->m_screenBitmap, m_x, m_y, false);
    m_x = newX;
    m_y = newY;
    m_background->grab(g_windowManager->m_screenBitmap, m_x, m_y);
    drawWindow(0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    startW += abs(m_x - startX);
    startH += abs(m_y - startY);
    if (m_x < startX)
        startX = m_x;
    if (m_y < startY)
        startY = m_y;
    g_windowManager->updateScreen(startX, startY, startW, startH);
}

// E:\gamedcs\window.cpp:778
// DC 825 calls GetWidth/GetHeight and the bitmap-pointer Draw overload;
// 836 calls the bitmap-pointer Grab overload. The null-background arm stores
// X then Y at 814/815, and the drawing arm is an else scope ending at 852.
// These canonical calls and scopes are restored. The 32-state family emits
// 14 distinct objects; ten retained candidates reproduce, with every window
// score unchanged. CenterWindow retains a whole-body EBX/EDI role swap at
// 92.8767%: retail keeps centerX in EBX, this compile in EDI. The resulting
// height spill also rotates the four saved local homes.
// Earlier local-order/default/clamp/store variants (792 source candidates)
// did not close that swap. RTM and SP3 emit the same bytes, so compiler age
// is not the explanation. Do not invent a fifth local or remove the proven
// saved height to steer allocation; helper/scope restoration is not closure.
// Thirty-two conditional-compound/declaration-shape controls also emit one
// reproduced object: braces around the default, clamp and damage-bound pairs,
// crossed with separate saved-local declarations, leave 92.8767% unchanged.
VA(0x005ff240, 0x162) MAC_ADDRESS(0x20b870, 0x1b0)  // anchor-global, dc 0x19797c
void heroWindow::centerWindow(int centerX, int centerY)
{
    int startX = m_x;
    int startY = m_y;
    int startW = m_width;
    int startH = m_height;
    if (centerX == -1)
        centerX = (WINDOW_SCREEN_WIDTH - m_width) / 2;
    if (centerY == -1)
        centerY = (WINDOW_SCREEN_HEIGHT - m_height) / 2;
    if (centerX < 0)
        centerX = 0;
    if (centerY < 0)
        centerY = 0;
    if (m_width + centerX > WINDOW_SCREEN_WIDTH)
        centerX = WINDOW_SCREEN_WIDTH - m_width;
    if (m_height + centerY > WINDOW_SCREEN_HEIGHT)
        centerY = WINDOW_SCREEN_HEIGHT - m_height;
    if (!m_background) {
        m_x = centerX;
        m_y = centerY;
    } else {
        m_background->draw(0, 0, m_background->getWidth(), m_background->getHeight(),
                           g_windowManager->m_screenBitmap, m_x, m_y, false);
        m_x = centerX;
        m_y = centerY;
        m_background->grab(g_windowManager->m_screenBitmap, m_x, m_y);
        drawWindow(0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        startW += abs(m_x - startX);
        startH += abs(m_y - startY);
        if (m_x < startX)
            startX = m_x;
        if (m_y < startY)
            startY = m_y;
        g_windowManager->updateScreen(startX, startY, startW, startH);
    }
}

VA(0x005ff3b0, 0x23) MAC_ADDRESS(0x20ba20, 0x34)  // dc 0x197ad0
int heroWindow::findWidget(int mx, int my) const
{
    widget* found = findWidgetPtr(mx, my);
    if (found)
        return found->m_id;
    return -1;
}

VA(0x005ff3e0, 0x7D) MAC_ADDRESS(0x20ba54, 0xd8)  // dc 0x197aec
widget* heroWindow::findWidgetPtr(int mx, int my) const
{
    mx -= m_x;
    my -= m_y;
    for (widget* const* it = m_widgets.end(); it != m_widgets.begin(); --it) {
        widget* found = it[-1];
        if (mx >= found->m_x && my >= found->m_y
            && mx < found->m_x + found->m_width
            && my < found->m_y + found->m_height
            && (found->m_status & widget::WIDGET_ACTIVE)
            && !(found->m_status & widget::WIDGET_DIMMED)
            && !(found->m_status & widget::WIDGET_DIMMED_NODRAW))
            return found;
    }
    return 0;
}

// Original: heroWindow::EnableAllWidgets; window.cpp:893, dc 0x197bdc.
void heroWindow::enableAllWidgets(unsigned char enable)
{
    widget* current = m_headWidget;
    while (current) {
        current->enable(enable);
        current = current->m_nextWidget;
    }
}

VA(0x005ff460, 0x21) MAC_ADDRESS(0x20bb48, 0x3c)  // dc 0x197c08
void heroWindow::doModal(bool fadeIn)
{
    g_windowManager->doDialog(this, heroWindowHandler, fadeIn);
}

VA(0x005ff490, 0x6C) MAC_ADDRESS(0x20bb84, 0x98)  // dc 0x197c24
void heroWindow::setFocus(int id)
{
    if (m_focusId != -1) {
        widget* current = getWidget(m_focusId);
        m_focusId = -1;
        if (current)
            current->onKillFocus();
    }
    m_focusId = id;
    if (id != -1) {
        widget* current = getWidget(id);
        if (current)
            current->onSetFocus();
    }
}

// E:\gamedcs\window.cpp:934
// NO VA CLAIM - a CARVE GAP, not a missing body. Retail's handler sits
// at 0x5ff500, inside the unowned 0x5ff4fc..0x5ff510 run between
// SetFocus and delete_widgets, and config/retail/functions.tsv has no
// row there (it is MANUALLY MANAGED; correcting a boundary is not a
// matcher's call). The twelve bytes are decoded by hand:
//   mov eax,ecx / push eax / mov ecx,[eax+0x1c] / mov edx,[ecx] /
//   call [edx+0xc] / ret
// i.e. fastcall in ecx - the /Gr form of the STATIC member DC's
// fieldlist marks it as - message::window at +0x1c, and heroWindow
// vtable slot 3, which independently corroborates handle_message's
// slot in the roster in window.h. Defined here so that DoModal's
// address-take resolves; that claim is what scores the pair.
MAC_ADDRESS(0x20bc1c, 0x34)
int heroWindow::heroWindowHandler(message& msg)
{
    return msg.m_window->handleMessage(msg);
}

VA(0x005ff510, 0x60) MAC_ADDRESS(0x20bc50, 0x7c)  // dc 0x197c8c
void heroWindow::deleteWidgets()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
    m_widgets.clear();
}

VA(0x005ff570, 0x32) MAC_ADDRESS(0x20bccc, 0x74)  // dc 0x197cd4
void heroWindow::addWidgetsToMessageStream()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }
}

// Nested sleeps notify widgets only on the first sleep and final wake.
VA(0x005ff5b0, 0x33) MAC_ADDRESS(0x20bd40, 0x74)  // anchor-callee, callers byte-proven
void heroWindow::sleepAllWidgets(unsigned char sleep)
{
    if (sleep) {
        if (m_sleepCount++ == 0)
            vslot8(1);
    } else {
        if (--m_sleepCount == 0)
            vslot8(0);
    }
}

VA(0x005ff5f0, 0x4F) MAC_ADDRESS(0x20bdb4, 0xbc)
void heroWindow::vslot8(unsigned char on)
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it)
        (*it)->sleep(on);
}

VA(0x005ff640, 0x61) MAC_ADDRESS(0x20be70, 0x40)  // dc 0x197d48
CHeroWindowEx::CHeroWindowEx(int winX, int winY, int winWidth, int winHeight,
                             unsigned winType)
    : heroWindow(winX, winY, winWidth, winHeight, winType)
{
    m_rolloverId = -1;
}

VA_COMPGEN(0x005ff6b0, 0x21, SCALAR_DELETING_DTOR, CHeroWindowEx)

VA(0x005ff6e0, 0xAE) MAC_ADDRESS(0x20beb0, 0x130)  // dc 0x197d9c
unsigned char CHeroWindowEx::processHover(int mouseX, int mouseY)
{
    textWidget* rollover = getRolloverWidget();
    if (!rollover)
        return 0;
    widget* hit = findWidgetPtr(mouseX, mouseY);
    int id = -1;
    if (hit)
        id = hit->m_id;
    if (id != m_rolloverId) {
        m_rolloverId = id;
        const char* text = "";
        if (hit) {
            text = hit->getHelpText();
            if (!text)
                text = "";
            g_mouseManager->setPointer(1, mouseManager::DEFAULT_SET);
        } else {
            g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
        }
        rollover->setText(text);
        drawWindow(0, rollover->m_id, rollover->m_id);
        g_windowManager->updateScreen(rollover->m_x + m_x, rollover->m_y + m_y,
                                      rollover->m_width, rollover->m_height);
    }
    return 1;
}

VA(0x005ff790, 0x82) MAC_ADDRESS(0x20bfe0, 0xb4)  // dc 0x197e58
unsigned char CHeroWindowEx::processRightSelect(int id)
{
    widget* current = getWidget(id);
    if (!current)
        return 0;
    const char* text = current->getRclickText();
    if (!text)
        return 0;
    if (strlen(text) == 0)
        return 0;
    normalDialog(text, 4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    return 1;
}

// E:\gamedcs\window.cpp:1036 - vtable 0x243ce8 slot 9. The three arms
// dispatch through slots 11/10/12 in that order (the [vptr+0x2c],
// [vptr+0x28] and [vptr+0x30] call sites are what pin the roster
// order), and the no-match arm returns 0 WITHOUT re-testing the exit
// flag - retail jumps straight to the shared `xor eax,eax` tail.
VA(0x005ff820, 0xA5) MAC_ADDRESS(0x20c094, 0x104)  // anchor-vtable (slot 9 of 0x243ce8), dc 0x197eb4
int CHeroWindowEx::windowHandler(message& msg)
{
    bool exitFlag = 0;

    if ((msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)
        && (msg.m_codeX == widget::WIDGET_SELECT
            || msg.m_codeX == widget::WIDGET_RIGHT_SELECT)) {
        if (processRightSelect(msg.m_codeY))
            return 1;
    } else if (msg.m_id == MESSAGE_MOUSE_MOVE) {
        if (processHover(msg.m_mouseX, msg.m_mouseY))
            return 1;
    } else if (msg.m_id == MESSAGE_WIDGET
               && msg.m_codeX == widget::WIDGET_DESELECT) {
        onWidgetDeselect(msg.m_codeY, exitFlag);
    } else {
        return 0;
    }
    if (exitFlag) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return 2;
    }
    return 0;
}

// E:\gamedcs\window.cpp:1122/1123, dc 0x197f48. Ordinary source-owned
// body, not a header inline. Retail vtable slot 12 and CScenarioInfoDlg's
// qualified call resolve to 0x559140 (xor eax,eax; ret 8), ICF-folded with
// t_stdio_file_adapter::write; that existing claim remains the sole owner.

MAC_ADDRESS(0x20c198, 0x8)
int CHeroWindowEx::onWidgetDeselect(int id, bool& exitFlag)
{
    return 0;
}

VA(0x005ff8d0, 0x3) MAC_ADDRESS(0x20c1a0, 0x8)  // dc 0x197f4c
textWidget* CHeroWindowEx::getRolloverWidget()
{
    return 0;
}

VA(0x005ff8e0, 0x75) MAC_ADDRESS(0x20c1a8, 0x7c)  // dc 0x197f50
void CHeroWindowEx::setHelpText(THelpText* helpText, int start, int stop,
                                unsigned char copyText)
{
    for (int i = start; i < stop; i++) {
        widget* current = getWidget(i);
        if (current)
            current->setHelpText(helpText[i - start].m_text,
                                   helpText[i - start].m_rclick, copyText);
    }
}

VA(0x005ff960, 0xC3) MAC_ADDRESS(0x20c224, 0x200)  // dc 0x197fd8
unsigned char initializeWinSetupText()
{
    TTextResource* textResource = ResourceManager::getText(
        DATA_COMPGEN(0x0068c838, winSetupTextName, "jktext.txt"));
    if (!textResource)
        return 0;

    int textLine = 0;
    int setup = 0;
    int i;

    ++textLine;
    for (i = 0; i < 9; ++i, ++textLine, ++setup) {
        g_winSetup[setup].m_text = textResource->getText(textLine);
    }

    ++textLine;
    for (i = 0; i < 1; ++i, ++textLine, ++setup) {
        g_winSetup[setup].m_text = textResource->getText(textLine);
    }

    ++textLine;
    for (i = 0; i < 2; ++i, ++textLine, ++setup) {
        g_winSetup[setup].m_text = textResource->getText(textLine);
    }

    ++textLine;
    for (i = 0; i < 20; ++i, ++textLine, ++setup) {
        g_winSetup[setup].m_text = textResource->getText(textLine);
    }

    ++textLine;
    for (i = 0; i < 2; ++i, ++textLine, ++setup) {
        g_winSetup[setup].m_text = textResource->getText(textLine);
    }

    ++textLine;
    for (i = 0; i < 2; ++i, ++textLine, ++setup) {
        g_winSetup[setup].m_text = textResource->getText(textLine);
    }

    ++textLine;
    for (i = 0; i < 1; ++i, ++textLine, ++setup) {
        g_winSetup[setup].m_text = textResource->getText(textLine);
    }

    return 1;
}

VA(0x005ffa30, 0xC1) MAC_ADDRESS(0x20c424, 0xa0)  // dc 0x198130
void setWinText(heroWindow* win, int winId)
{
    message msg;
    msg.m_id = 0;
    msg.m_codeX = 0;
    msg.m_codeY = 0;
    msg.m_qualifier = 0;
    msg.m_mouseX = 0;
    msg.m_mouseY = 0;
    msg.m_extra = 0;
    msg.m_window = 0;
    for (unsigned i = 0; i < 37; ++i) {
        if (g_winSetup[i].m_windowId == winId) {
            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = widget::WIDGET_SET_TEXT;
            msg.m_codeY = g_winSetup[i].m_widgetId;
            msg.m_extraText = g_winSetup[i].m_text;
            win->broadcastMessage(msg);
        }
    }
}
