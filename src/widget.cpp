// 13 functions in link order.
#include "va.h"

#include <string.h>

#include "widget.h"

#include "bitmap16.h"
#include "message.h"
#include "window.h"
#include "winmgr.h"

VA(0x005fe340, 0x62)  // dc 0x196b4c
widget::widget(short widgetX, short widgetY, short widgetWidth, short widgetHeight, short widgetId, short widgetStyle)
    : m_sleepCount(0)
{
    m_x = widgetX;
    m_y = widgetY;
    m_width = widgetWidth;
    m_height = widgetHeight;
    m_id = widgetId;
    m_parentWindow = 0;
    m_prevWidget = 0;
    m_nextWidget = 0;
    m_status = WIDGET_ACTIVE | WIDGET_DRAWN;
    m_priority = -1;
    m_style = widgetStyle;
    m_rollOver = 0;
    m_rightClick = 0;
    m_freeText = 0;
}

VA_COMPGEN(0x005fe3b0, 0x5C, SCALAR_DELETING_DTOR, widget)

VA(0x005fe410, 0x1D)  // dc 0x196bd4
widget::widget()
    : m_sleepCount(0)
{
    m_rollOver = 0;
    m_rightClick = 0;
    m_freeText = 0;
    m_status = WIDGET_ACTIVE | WIDGET_DRAWN;
}

VA(0x005fe430, 0x45)  // dc 0x196c10
widget::~widget()
{
    if (s_lastHoverWidget == this)
        s_lastHoverWidget = 0;
    if (m_freeText) {
        if (m_rightClick)
            delete m_rightClick;
        if (m_rollOver)
            delete m_rollOver;
    }
}

VA(0x005fe480, 0x4E)  // dc 0x196c6c
void widget::initialize(int x, int y, int w, int h, int id, int style)
{
    m_parentWindow = 0;
    m_prevWidget = 0;
    m_nextWidget = 0;
    m_x = x;
    m_y = y;
    m_width = w;
    m_height = h;
    m_id = id;
    m_status = WIDGET_ACTIVE | WIDGET_DRAWN;
    m_priority = -1;
    m_style = style;
}

VA(0x005fe4d0, 0x17)  // dc 0x196cbc
int widget::open(int newPriority, heroWindow* parent)
{
    m_priority = newPriority;
    m_parentWindow = parent;
    return 0;
}

// Original: widget::Close; widget.cpp:235, dc 0x196ccc.
// heroWindow::RemoveWidget calls the shared empty retail representative
// at 0x5bc690. ICF removes a separate address, not this source definition.
void widget::close()
{
}

VA(0x005fe4f0, 0x2C8)  // dc 0x196cd0
int widget::main(message& msg)
{
    if (m_sleepCount > 0)
        return 0;
    switch (msg.m_id) {
    case MESSAGE_MOUSE_MOVE: {
        if (!(m_status & WIDGET_ACTIVE))
            break;
        short mouseX = msg.m_codeX - m_parentWindow->m_x;
        short mouseY = msg.m_codeY - m_parentWindow->m_y;
        if (mouseX < m_x || mouseY < m_y || mouseX >= m_x + m_width
            || mouseY >= m_y + m_height)
            break;
        msg.m_codeY = m_id;
        if (s_lastHoverWidget != this) {
            s_lastHoverWidget = this;
            processHover();
        }
        return 2;
    }
    case MESSAGE_WIDGET:
        switch (msg.m_codeX) {
        case WIDGET_DRAW:
            if (m_status & WIDGET_DRAWN)
                draw();
            if ((m_status & (WIDGET_DRAWN | WIDGET_DIMMED))
                == (WIDGET_DRAWN | WIDGET_DIMMED))
                dim();
            break;
        case WIDGET_SET_STATUS:
            if (msg.m_codeY != m_id)
                break;
            if (msg.m_extra == WIDGET_DIMMED_NODRAW) {
                m_status |= WIDGET_DIMMED;
                return 1;
            }
            m_status |= msg.m_extra;
            if (msg.m_extra == WIDGET_DISABLED)
                return 1;
            if (m_status & WIDGET_DIMMED) {
                draw();
                dim();
            }
            if (m_status & WIDGET_UPDATE) {
                g_windowManager->updateScreen(
                    m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
                m_status &= ~WIDGET_UPDATE;
            }
            return 1;
        case WIDGET_CLEAR_STATUS: {
            if (msg.m_codeY != m_id)
                break;
            short flags = msg.m_extra;
            if (msg.m_extra == WIDGET_DIMMED_NODRAW) {
                m_status &= ~WIDGET_DIMMED;
                return 1;
            }
            m_status &= ~flags;
            if (flags & WIDGET_DIMMED)
                draw();
            if (flags & WIDGET_UPDATE)
                g_windowManager->updateScreen(
                    m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
            return 1;
        }
        case WIDGET_SET_X:
            if (msg.m_codeY != m_id)
                break;
            m_x = msg.m_extra;
            return 1;
        case WIDGET_SET_Y:
            if (msg.m_codeY != m_id)
                break;
            m_y = msg.m_extra;
            return 1;
        case WIDGET_SET_WIDTH:
            if (msg.m_codeY != m_id)
                break;
            m_width = msg.m_extra;
            return 1;
        }
        break;
    }
    return 0;
}

VA(0x005fe7c0, 0x40)  // dc 0x196f88
int widget::sendMessage(widget::ECommands command, int extra)
{
    message msg;
    msg.m_qualifier = 0;
    msg.m_mouseX = 0;
    msg.m_mouseY = 0;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = command;
    msg.m_codeY = m_id;
    msg.m_extra = extra;
    msg.m_window = m_parentWindow;
    return main(msg);
}

VA(0x005fe800, 0x32)  // dc 0x196fc8
void widget::dim() const
{
    g_windowManager->m_screenBitmap->darken(
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
}

VA(0x005fe840, 0xE9)  // dc 0x196ffc
void widget::setHelpText(const char* text, const char* rclick, unsigned char copyText)
{
    if (m_rollOver) {
        if (m_freeText)
            delete m_rollOver;
        m_rollOver = 0;
    }
    if (m_rightClick) {
        if (m_freeText)
            delete m_rightClick;
        m_rightClick = 0;
    }
    if (copyText) {
        m_freeText = 1;
        if (text) {
            m_rollOver = new char[strlen(text) + 1];
            strcpy(m_rollOver, text);
        }
        if (rclick) {
            m_rightClick = new char[strlen(rclick) + 1];
            strcpy(m_rightClick, rclick);
        }
    } else {
        m_freeText = 0;
        m_rollOver = const_cast<char*>(text);
        m_rightClick = const_cast<char*>(rclick);
    }
}

VA(0x005fe930, 0xC)  // dc 0x1970a8
void widget::processHover()
{
    m_parentWindow->handleWidgetHover(this);
}

VA(0x005fe940, 0x83)  // dc 0x1970c0
void widget::enable(unsigned char arg)
{
    if (arg) {
        message msg;
        msg.m_qualifier = 0;
        msg.m_mouseX = 0;
        msg.m_mouseY = 0;
        msg.m_codeY = m_id;
        msg.m_window = m_parentWindow;
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = WIDGET_CLEAR_STATUS;
        msg.m_extra = WIDGET_DISABLED;
        main(msg);
    } else {
        message msg;
        msg.m_qualifier = 0;
        msg.m_mouseX = 0;
        msg.m_mouseY = 0;
        msg.m_codeY = m_id;
        msg.m_window = m_parentWindow;
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = WIDGET_SET_STATUS;
        msg.m_extra = WIDGET_DISABLED;
        main(msg);
    }
}

// widget::`scalar deleting destructor' (dc 0x197104) is claimed in
// retail link order above, at 0x5fe3b0.

DATA(0x006aac68)
widget* widget::s_lastHoverWidget;
