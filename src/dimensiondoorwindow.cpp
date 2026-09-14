// dimensiondoorwindow.cpp - E:\gamedcs\dimensiondoorwindow.cpp (compiland dimensiondoorwindow.obj)
#include <va.h>
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

#if 0  // @carcass

// E:\gamedcs\dimensiondoorwindow.cpp:53
DC_ONLY(0x827f8, 0x140)
void DimensionDoorWindow::DimensionDoorWindow()
{
    // @stub
}

// E:\gamedcs\dimensiondoorwindow.cpp:74
DC_ONLY(0x82938, 0x68)
void DimensionDoorWindow::~DimensionDoorWindow()
{
    // @stub
}

// E:\gamedcs\dimensiondoorwindow.cpp:100
DC_ONLY(0x829a0, 0x1E2)
int DimensionDoorWindow::windowHandler(message& msg)
{
    // @stub
}

// E:\gamedcs\dimensiondoorwindow.cpp:208
DC_ONLY(0x82b84, 0x16)
int DimensionDoorWindow::exitDialog(message& msg)
{
    // @stub
}

// E:\gamedcs\dimensiondoorwindow.cpp:235
DC_ONLY(0x82b9c, 0x114)
void SkuttleBoatWindow::SkuttleBoatWindow()
{
    // @stub
}

// E:\gamedcs\dimensiondoorwindow.cpp:254
DC_ONLY(0x82cb0, 0x62)
void SkuttleBoatWindow::~SkuttleBoatWindow()
{
    // @stub
}

// E:\gamedcs\dimensiondoorwindow.cpp:280
DC_ONLY(0x82d14, 0x1BC)
int SkuttleBoatWindow::windowHandler(message& msg)
{
    // @stub
}

#endif  // @carcass

VA(0x004916f0, 0x164)  // dc 0x827f8
DimensionDoorWindow::DimensionDoorWindow()
    : CAdvPopup(0, 0, 800, 600, 1)
{
    m_widgets.reserve(2);

    widget* mapWidget = g_advManager->m_advWindow->m_mapWidget;
    m_widgets.push_back(new border(mapWidget->m_x, mapWidget->m_y,
        mapWidget->m_width, mapWidget->m_height, 0, 1));

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }
}

VA_COMPGEN(0x00491860, 0x21, SCALAR_DELETING_DTOR, DimensionDoorWindow)

VA(0x00491890, 0x6B)  // dc 0x82938
DimensionDoorWindow::~DimensionDoorWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// The two handlers are one shape as well - a base forward, an animation
// catch-up, then a switch on msg->id with the same three arms. They differ
// in exactly three places: the skuttle-boat one has no WIDGET_DESELECT arm,
// its hover test is `cell->type == BOAT && cell->is_trigger` instead of the
// dimension-door passability mask, and it arms cursor 42 instead of 41.

// FOUR SPELLINGS ARE LOAD-BEARING HERE, each measured on the way up from
// 73.11:

// E:\gamedcs\dimensiondoorwindow.cpp:100
VA(0x00491900, 0x1B9)  // vtable slot 9 + source order, dc 0x829a0
int DimensionDoorWindow::windowHandler(message& msg)
{
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;

    unsigned long lastFrame = g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
    if (static_cast<long>(GameTime::get() - lastFrame) > 0) {
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
                    g_advManager->getMouseMapPoint());
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
            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = msg.m_codeY = widget::WIDGET_END_DIALOG;
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
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = msg.m_codeY = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// The twin of the claim at 0x491eb0, to the byte: the two classes share
// this exit path verbatim, which is why its 0x2c size shows up twice in
// the carve.

// E:\gamedcs\dimensiondoorwindow.cpp:208
VA(0x00491ac0, 0x2C)  // vtable slot 14 + source order, dc 0x82b84
int DimensionDoorWindow::exitDialog(message& msg)
{
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = msg.m_codeY = 10;
    g_windowManager->m_dialogReturn = 0;
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x00491af0, 0x164)  // dc 0x82b9c
SkuttleBoatWindow::SkuttleBoatWindow()
    : CAdvPopup(0, 0, 800, 600, 1)
{
    m_widgets.reserve(2);

    widget* mapWidget = g_advManager->m_advWindow->m_mapWidget;
    m_widgets.push_back(new border(mapWidget->m_x, mapWidget->m_y,
        mapWidget->m_width, mapWidget->m_height, 0, 1));

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }
}

VA_COMPGEN(0x00491c60, 0x21, SCALAR_DELETING_DTOR, SkuttleBoatWindow)

VA(0x00491c90, 0x6B)  // dc 0x82cb0
SkuttleBoatWindow::~SkuttleBoatWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// E:\gamedcs\dimensiondoorwindow.cpp:280
// DC 0x82d14 uses the same exit-flag/common message tail. Restoring it removes
// two gotos at 100%; duplicated direct exits score 81.9421%.
VA(0x00491d00, 0x1A9)  // vtable slot 9 + source order, dc 0x82d14
int SkuttleBoatWindow::windowHandler(message& msg)
{
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;

    unsigned long lastFrame = g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
    if (static_cast<long>(GameTime::get() - lastFrame) > 0) {
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
                    g_advManager->getMouseMapPoint());
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
            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = msg.m_codeY = widget::WIDGET_END_DIALOG;
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
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = msg.m_codeY = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// E:\gamedcs\dimensiondoorwindow.cpp:395
VA(0x00491eb0, 0x2c)  // vtable slot 14 + source order, dc 0x82ed0
int SkuttleBoatWindow::exitDialog(message& msg)
{
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = msg.m_codeY = 10;
    g_windowManager->m_dialogReturn = 0;
    return MESSAGE_DISPATCH_FORWARD;
}

#if 0  // @carcass

// E:\gamedcs\dimensiondoorwindow.cpp:71
DC_ONLY(0x82eec, 0x34)
void* DimensionDoorWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\dimensiondoorwindow.cpp:250
DC_ONLY(0x82f20, 0x34)
void* SkuttleBoatWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass
