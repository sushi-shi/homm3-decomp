#include "va.h"

#include "dimensiondoorwindow.h"

#include "advmgr.h"
#include "border.h"
#include "kb.h"
#include "kbwin.h"
#include "mapcell.h"
#include "message.h"
#include "mousemgr.h"
#include "widget.h"
#include "winmgr.h"

VA(0x004916f0, 0x164)
DC_ADDRESS(0x0827f8, 0x140)
MAC_ADDRESS(0x0a2b94, 0x160)
TDimensionDoorWindow::TDimensionDoorWindow()
    : CAdvPopup(0, 0, 800, 600, 1)
{
    m_widgets.reserve(2);

    widget* mapWidget = g_advManager->m_advWindow->m_mapWidget;
    m_widgets.push_back(new border(mapWidget->m_x, mapWidget->m_y,
        mapWidget->m_width, mapWidget->m_height, 0, 1));

    heroWindow::addWidgetsToMessageStream();
}

VA_COMPGEN(0x00491860, 0x21, SCALAR_DELETING_DTOR, TDimensionDoorWindow)

VA(0x00491890, 0x6B)
DC_ADDRESS(0x082938, 0x68)
MAC_ADDRESS(0x0a2cf4, 0xac)
TDimensionDoorWindow::~TDimensionDoorWindow()
{
    deleteWidgetObjects();
}

// The two handlers are one shape as well - a base forward, an animation
// catch-up, then a switch on msg->id with the same three arms. They differ
// in exactly three places: the skuttle-boat one has no WIDGET_DESELECT arm,
// its hover test is `cell->type == BOAT && cell->is_trigger` instead of the
// dimension-door passability mask, and it arms cursor 42 instead of 41.

// FOUR SPELLINGS ARE LOAD-BEARING HERE, each measured on the way up from
// 73.11:

// E:\gamedcs\dimensiondoorwindow.cpp:100
VA(0x00491900, 0x1B9)
DC_ADDRESS(0x0829a0, 0x1e2)
MAC_ADDRESS(0x0a2da0, 0x26c)  // vtable slot 9 + source order
int TDimensionDoorWindow::windowHandler(message& msg)
{
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;

    unsigned long lastFrame = g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
    if (GameTime::elapsedSince(lastFrame) > 0) {
        g_advManager->completeDraw(0);
        g_advManager->updateScreen(0, 0);
    }

    int mouseX = msg.m_mouseX;
    int mouseY = msg.m_mouseY;

    bool exitFlag = false;
    switch (msg.m_id) {
    case MESSAGE_KEY_DOWN:
        if (msg.m_codeX == DIALOG_CLOSE_KEY) {
            g_windowManager->m_dialogReturn = 0;
            exitFlag = true;
        }
        break;

    case MESSAGE_MOUSE_MOVE:
        if (g_advManager->inMapArea(mouseX, mouseY)) {
            int cellX = mouseX / 32;
            int cellY = mouseY / 32;
            if (g_advManager->m_lastHoverX != cellX
                || g_advManager->m_lastHoverY != cellY) {
                g_advManager->m_lastHoverX = cellX;
                g_advManager->m_lastHoverY = cellY;
                NewmapCell* cell = g_advManager->getCell(
                    g_advManager->get_mouse_map_point());
                if (!(cell->m_flags0011 & 0x100) && !cell->m_isTrigger) {
                    g_windowManager->m_dialogReturn = 1;
                    g_mouseManager->setPointer(ADV_DIMENSION_DOOR_POINTER,
                        mouseManager::ADVENTURE_SET);
                } else {
                    g_windowManager->m_dialogReturn = 0;
                    g_mouseManager->setPointer(ADV_ARROW_POINTER,
                        mouseManager::ADVENTURE_SET);
                }
            }
        } else {
            g_windowManager->m_dialogReturn = 0;
            g_mouseManager->setPointer(ADV_ARROW_POINTER,
                mouseManager::ADVENTURE_SET);
        }
        break;

    case MESSAGE_WIDGET:
        switch (msg.m_codeX) {
        case widget::WIDGET_SELECT:
            if (msg.m_codeY != 0)
                break;
            if (g_windowManager->m_dialogReturn != 1)
                break;
            msg.setDialogEnd(widget::WIDGET_END_DIALOG);
            return MESSAGE_DISPATCH_FORWARD;
        case widget::WIDGET_DESELECT:
            if (msg.m_codeY == DIALOG_RETURN_CANCEL) {
                g_windowManager->m_dialogReturn = 0;
                exitFlag = true;
            }
            break;
        case widget::WIDGET_RIGHT_SELECT:
            if (msg.m_codeY == 0) {
                g_windowManager->m_dialogReturn = 0;
                exitFlag = true;
            }
            break;
        }
        break;
    }
    if (exitFlag) {
        msg.setDialogEnd(widget::WIDGET_END_DIALOG);
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// The twin of the claim at 0x491eb0, to the byte: the two classes share
// this exit path verbatim, which is why its 0x2c size shows up twice in
// the carve.

// E:\gamedcs\dimensiondoorwindow.cpp:208
VA(0x00491ac0, 0x2C)
DC_ADDRESS(0x082b84, 0x16)
MAC_ADDRESS(0x0a300c, 0x2c)  // vtable slot 14 + source order
int TDimensionDoorWindow::exitDialog(message& msg)
{
    msg.setDialogEnd(widget::WIDGET_END_DIALOG);
    g_windowManager->m_dialogReturn = 0;
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x00491af0, 0x164)
DC_ADDRESS(0x082b9c, 0x114)
MAC_ADDRESS(0x0a3038, 0x160)
TSkuttleBoatWindow::TSkuttleBoatWindow()
    : CAdvPopup(0, 0, 800, 600, 1)
{
    m_widgets.reserve(2);

    widget* mapWidget = g_advManager->m_advWindow->m_mapWidget;
    m_widgets.push_back(new border(mapWidget->m_x, mapWidget->m_y,
        mapWidget->m_width, mapWidget->m_height, 0, 1));

    heroWindow::addWidgetsToMessageStream();
}

VA_COMPGEN(0x00491c60, 0x21, SCALAR_DELETING_DTOR, TSkuttleBoatWindow)

VA(0x00491c90, 0x6B)
DC_ADDRESS(0x082cb0, 0x62)
MAC_ADDRESS(0x0a3198, 0xac)
TSkuttleBoatWindow::~TSkuttleBoatWindow()
{
    deleteWidgetObjects();
}

// E:\gamedcs\dimensiondoorwindow.cpp:280
// DC uses the same exit-flag/common message tail. Restoring it removes
// two gotos at 100%; duplicated direct exits score 81.9421%.
VA(0x00491d00, 0x1A9)
DC_ADDRESS(0x082d14, 0x1bc)
MAC_ADDRESS(0x0a3244, 0x248)  // vtable slot 9 + source order
int TSkuttleBoatWindow::windowHandler(message& msg)
{
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;

    unsigned long lastFrame = g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
    if (GameTime::elapsedSince(lastFrame) > 0) {
        g_advManager->completeDraw(0);
        g_advManager->updateScreen(0, 0);
    }

    int mouseX = msg.m_mouseX;
    int mouseY = msg.m_mouseY;

    bool exitFlag = false;
    switch (msg.m_id) {
    case MESSAGE_KEY_DOWN:
        if (msg.m_codeX == DIALOG_CLOSE_KEY) {
            g_windowManager->m_dialogReturn = 0;
            exitFlag = true;
        }
        break;

    case MESSAGE_MOUSE_MOVE:
        if (g_advManager->inMapArea(mouseX, mouseY)) {
            int cellX = mouseX / 32;
            int cellY = mouseY / 32;
            if (g_advManager->m_lastHoverX != cellX
                || g_advManager->m_lastHoverY != cellY) {
                g_advManager->m_lastHoverX = cellX;
                g_advManager->m_lastHoverY = cellY;
                NewmapCell* cell = g_advManager->getCell(
                    g_advManager->get_mouse_map_point());
                if (cell->m_type == BOAT && cell->m_isTrigger) {
                    g_windowManager->m_dialogReturn = 1;
                    g_mouseManager->setPointer(ADV_SKUTTLE_BOAT_POINTER,
                        mouseManager::ADVENTURE_SET);
                } else {
                    g_windowManager->m_dialogReturn = 0;
                    g_mouseManager->setPointer(ADV_ARROW_POINTER,
                        mouseManager::ADVENTURE_SET);
                }
            }
        } else {
            g_windowManager->m_dialogReturn = 0;
            g_mouseManager->setPointer(ADV_ARROW_POINTER,
                mouseManager::ADVENTURE_SET);
        }
        break;

    case MESSAGE_WIDGET:
        switch (msg.m_codeX) {
        case widget::WIDGET_SELECT:
            if (msg.m_codeY != 0)
                break;
            if (g_windowManager->m_dialogReturn != 1)
                break;
            msg.setDialogEnd(widget::WIDGET_END_DIALOG);
            return MESSAGE_DISPATCH_FORWARD;
        case widget::WIDGET_RIGHT_SELECT:
            if (msg.m_codeY == 0) {
                g_windowManager->m_dialogReturn = 0;
                exitFlag = true;
            }
            break;
        }
        break;
    }
    if (exitFlag) {
        msg.setDialogEnd(widget::WIDGET_END_DIALOG);
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// E:\gamedcs\dimensiondoorwindow.cpp:395
VA(0x00491eb0, 0x2c)
DC_ADDRESS(0x082ed0, 0x1c)
MAC_ADDRESS(0x0a348c, 0x2c)  // vtable slot 14 + source order
int TSkuttleBoatWindow::exitDialog(message& msg)
{
    msg.setDialogEnd(widget::WIDGET_END_DIALOG);
    g_windowManager->m_dialogReturn = 0;
    return MESSAGE_DISPATCH_FORWARD;
}
