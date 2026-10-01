// 13 functions in link order.
#include "va.h"

#include <string.h>

#include "widget.h"

#include "bitmap16.h"
#include "message.h"
#include "window.h"
#include "winmgr.h"

// Project-inferred widget protocol operation. Keep the caller's existing
// modifiers, mouse coordinates, payload and window, including borrowed text.
void message::setWidgetCommand(int command, int widgetId)
{
    m_id = MESSAGE_WIDGET;
    m_codeX = command;
    m_codeY = widgetId;
}

// Project-inferred initialization operations. Neither releases owned text nor
// removes a live widget from its window; they only initialize these fields.
void widget::initializeLinks()
{
    m_parentWindow = 0;
    m_prevWidget = 0;
    m_nextWidget = 0;
}

void widget::initializeHelpText()
{
    m_rollOver = 0;
    m_rightClick = 0;
    m_freeText = 0;
}

VA(0x005fe340, 0x62)
DC_ADDRESS(0x196b4c, 0x88)
MAC_ADDRESS(0x20a504, 0x54)
widget::widget(short widgetX, short widgetY, short widgetWidth, short widgetHeight, short widgetId, short widgetStyle)
    : m_sleepCount(0)
{
    m_x = widgetX;
    m_y = widgetY;
    m_width = widgetWidth;
    m_height = widgetHeight;
    m_id = widgetId;
    initializeLinks();
    m_status = WIDGET_ACTIVE | WIDGET_DRAWN;
    m_priority = -1;
    m_style = widgetStyle;
    initializeHelpText();
}

VA_COMPGEN(0x005fe3b0, 0x5C, SCALAR_DELETING_DTOR, widget)

VA(0x005fe410, 0x1D)
DC_ADDRESS(0x196bd4, 0x3a)
MAC_ADDRESS(0x20a558, 0x28)
widget::widget()
    : m_sleepCount(0)
{
    initializeHelpText();
    m_status = WIDGET_ACTIVE | WIDGET_DRAWN;
}

VA(0x005fe430, 0x45)
DC_ADDRESS(0x196c10, 0x5a)
MAC_ADDRESS(0x20a580, 0x98)
widget::~widget()
{
    if (s_lastHoverWidget == this)
        clearHoverWidget();
    if (m_freeText) {
        if (m_rightClick)
            delete[] m_rightClick;
        if (m_rollOver)
            delete[] m_rollOver;
    }
}

VA(0x005fe480, 0x4E)
DC_ADDRESS(0x196c6c, 0x50)
MAC_ADDRESS(0x20a618, 0x54)
void widget::initialize(int x, int y, int w, int h, int id, int style)
{
    initializeLinks();
    m_x = x;
    m_y = y;
    m_width = w;
    m_height = h;
    m_id = id;
    m_status = WIDGET_ACTIVE | WIDGET_DRAWN;
    m_priority = -1;
    m_style = style;
}

VA(0x005fe4d0, 0x17)
DC_ADDRESS(0x196cbc, 0xe)
MAC_ADDRESS(0x20a66c, 0x14)
int widget::open(int newPriority, heroWindow* parent)
{
    m_priority = newPriority;
    m_parentWindow = parent;
    return 0;
}

// Original: widget::Close; widget.cpp:235
// heroWindow::RemoveWidget calls the shared empty retail representative
// at 0x5bc690. ICF removes a separate address, not this source definition.
DC_ADDRESS(0x196ccc, 0x4)
MAC_ADDRESS(0x20a680, 0x4)
void widget::close()
{
}

// Project-inferred presentation helpers. Drawing may change the widget or its
// parent; compute the update rectangle from the current fields afterward.
void widget::updateScreenRegion() const
{
    g_windowManager->updateScreen(
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
}

void widget::drawAndUpdate() const
{
    draw();
    updateScreenRegion();
}

// Project-inferred common hit box. Callers retain coordinate narrowing and
// their independent active/drawn/disabled policies.
bool widget::containsPoint(int x, int y) const
{
    return x >= m_x && y >= m_y && x < m_x + m_width && y < m_y + m_height;
}

// Project-inferred mouse-down preparation. Callers retain their hook order
// and only convert the message id after any hook has accepted the click.
void widget::prepareMouseSelection(message& msg)
{
    if (msg.m_id == MESSAGE_RIGHT_BUTTON_DOWN) {
        msg.m_qualifier = MESSAGE_MODIFIER_RIGHT;
        msg.m_codeX = WIDGET_RIGHT_SELECT;
    } else {
        m_status |= WIDGET_SELECTED;
        msg.m_codeX = WIDGET_SELECT;
    }
}

VA(0x005fe4f0, 0x2C8)
DC_ADDRESS(0x196cd0, 0x2b8)
MAC_ADDRESS(0x20a684, 0x388)
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
        if (!containsPoint(mouseX, mouseY))
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
                updateScreenRegion();
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
                updateScreenRegion();
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

// Project-inferred paired flag change. Preserve all other status bits and
// short storage; callers own any later draw and screen update.
void widget::setActiveAndDrawn(bool on)
{
    if (on)
        m_status |= WIDGET_ACTIVE | WIDGET_DRAWN;
    else
        m_status &= ~(WIDGET_ACTIVE | WIDGET_DRAWN);
}

VA(0x005fe7c0, 0x40)
DC_ADDRESS(0x196f88, 0x40)
MAC_ADDRESS(0x20aa0c, 0x74)
int widget::sendMessage(widget::ECommands command, int extra)
{
    message msg;
    msg.m_qualifier = 0;
    msg.m_mouseX = 0;
    msg.m_mouseY = 0;
    msg.setWidgetCommand(command, m_id);
    msg.m_extra = extra;
    msg.m_window = m_parentWindow;
    return main(msg);
}

VA(0x005fe800, 0x32)
DC_ADDRESS(0x196fc8, 0x34)
MAC_ADDRESS(0x20aa80, 0x54)
void widget::dim() const
{
    g_windowManager->m_screenBitmap->darken(
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
}

VA(0x005fe840, 0xE9)
DC_ADDRESS(0x196ffc, 0xaa)
MAC_ADDRESS(0x20aad4, 0x110)
void widget::setHelpText(const char* text, const char* rclick, unsigned char copyText)
{
    if (m_rollOver) {
        if (m_freeText)
            delete[] m_rollOver;
        m_rollOver = 0;
    }
    if (m_rightClick) {
        if (m_freeText)
            delete[] m_rightClick;
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

VA(0x005fe930, 0xC)
DC_ADDRESS(0x1970a8, 0x18)
MAC_ADDRESS(0x20abe4, 0x34)
void widget::processHover()
{
    m_parentWindow->handleWidgetHover(this);
}

// Dreamcast calls sendMessage in both enable branches; Mac retains both
// calls at 0:0x20ac34/0x20ac44. Windows expands the canonical helper body.
VA(0x005fe940, 0x83)
DC_ADDRESS(0x1970c0, 0x44)
MAC_ADDRESS(0x20ac18, 0x40)
void widget::enable(unsigned char arg)
{
    if (arg)
        sendMessage(WIDGET_CLEAR_STATUS, WIDGET_DISABLED);
    else
        sendMessage(WIDGET_SET_STATUS, WIDGET_DISABLED);
}

// widget::`scalar deleting destructor' (dc 0x197104) is claimed in
// retail link order above, at 0x5fe3b0.

DATA(0x006aac68)
widget* widget::s_lastHoverWidget;

// Complete-only sleep/wake hook: widget's vtable slot 12 and all inherited
// copies point to the empty ret 4 body folded at 0x485d80. Keep the body
// out of the header: button::onSleepChange retains its qualified base call.
MAC_ADDRESS(0x20ac58, 0x4)
void widget::onSleepChange(int on)
{
}
