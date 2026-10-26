#include "va.h"
#include "includes.h"

#include "textwdgt.h"

#include "bitmap16.h"
#include "bitmap816.h"
#include "csprite.h"
#include "message.h"
#include "recruit.h"
#include "resourcemanager.h"
#include "window.h"
#include "winmgr.h"

// Original: textWidget::textWidget; textwdgt.cpp:36
DC_ADDRESS(0x164c14, 0x6c)
textWidget::textWidget() : widget(0, 0, 0, 0, 0, 0)
{
    m_font = 0;
    m_color = font::PRIMARY;
    m_backColor = 0;
    m_justify = font::CENTER_JUSTIFIED;
    m_style = 8;
}

VA_COMPGEN(0x005bc250, 0x21, SCALAR_DELETING_DTOR, textWidget)

// E:\gamedcs\textwdgt.cpp:62 - the twelve-argument constructor the whole
// image builds its labels with. The base call is widget(x, y, w, h, id, 8)
// with the style argument a LITERAL 8; the trailing `style` parameter is
// read nowhere in the body.
VA(0x005bc280, 0x12D)
DC_ADDRESS(0x164c80, 0xa4)
MAC_ADDRESS(0x1b149c, 0xb4)  // anchor-vtable 0x642db0 + ret 0x2c
textWidget::textWidget(int x, int y, int w, int h, const char* text,
                       const char* fontName, font::TColor color, int id,
                       unsigned justify, int backColor, int style)
    : widget(static_cast<short>(x), static_cast<short>(y),
             static_cast<short>(w), static_cast<short>(h),
             static_cast<short>(id), 8)
{
    m_font = ResourceManager::GetFont(fontName);
    // DC textwdgt.cpp:64..65 records the conditional body's lexical scope;
    // the braces stay.
    if (text) {
        m_text = text;
    }
    m_color = color;
    m_backColor = backColor;
    m_justify = justify;
}

VA(0x005bc3b0, 0x8A)
DC_ADDRESS(0x164d24, 0x44)
MAC_ADDRESS(0x1b1550, 0x80)
textWidget::~textWidget()
{
    ResourceManager::Dispose(m_font);
}

// Original: textWidget::initialize; textwdgt.cpp:102
// Complete has no widget::focusable member; the remaining fields and calls
// are shared with the retained parameterized constructor.
DC_ADDRESS(0x164d68, 0x6c)
void textWidget::initialize(int x, int y, int w, int h, int id, int style,
                             const char* text, const char* fontName,
                             font::TColor color, unsigned int justify,
                             unsigned char focusable)
{
    widget::initialize(x, y, w, h, id, style);
    m_font = ResourceManager::GetFont(fontName);
    if (text) {
        m_text = text;
    }
    m_justify = justify;
    m_color = color;
}

// E:\gamedcs\textwdgt.cpp:120
VA(0x005bc440, 0x1AD)
DC_ADDRESS(0x164dd4, 0x1a8)
MAC_ADDRESS(0x1b15d0, 0x260)  // vtable 0x642db0 slot 2 + widget-message protocol
int textWidget::main(message& msg)
{
    if (m_sleepCount > 0) {
        return 0;
    }

    if (!(m_status & WIDGET_ACTIVE)) {
        if (msg.m_id == MESSAGE_WIDGET) {
            return widget::main(msg);
        } else {
            return 0;
        }
    }

    bool isDisabled = false;
    if (m_status & WIDGET_DISABLED)
        isDisabled = true;

    switch (msg.m_id) {
    case MESSAGE_WIDGET:
        switch (msg.m_codeX) {
        case WIDGET_SET_TEXT:
            if (msg.m_codeY == m_id) {
                setText(msg.m_extraText);
                return MESSAGE_DISPATCH_CONSUME;
            }
            break;
        case WIDGET_SET_COLOR:
            if (msg.m_codeY == m_id) {
                setColor(font::TColor(msg.m_extra));
                return MESSAGE_DISPATCH_CONSUME;
            }
            break;
        }
        break;

    case MESSAGE_LEFT_BUTTON_DOWN:
        if (isDisabled)
            return 0;
        // fall through
    case MESSAGE_RIGHT_BUTTON_DOWN: {
        if (!(m_status & WIDGET_DRAWN))
            return 0;
        short mouseX = msg.m_codeX - m_parentWindow->m_x;
        short mouseY = msg.m_codeY - m_parentWindow->m_y;
        if (mouseX >= m_x && mouseY >= m_y && mouseX < m_x + m_width
            && mouseY < m_y + m_height) {
            if (msg.m_id == MESSAGE_RIGHT_BUTTON_DOWN) {
                msg.m_qualifier = MESSAGE_MODIFIER_RIGHT;
                msg.m_codeX = WIDGET_RIGHT_SELECT;
            } else {
                m_status |= WIDGET_SELECTED;
                msg.m_codeX = WIDGET_SELECT;
            }
            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeY = m_id;
            return MESSAGE_DISPATCH_FORWARD;
        } else {
            return 0;
        }
    }

    case MESSAGE_LEFT_BUTTON_UP:
        if (isDisabled)
            return 0;
        // fall through
    case MESSAGE_RIGHT_BUTTON_UP:
        if (!(m_status & WIDGET_DRAWN)) {
            return 0;
        }
        if (m_status & WIDGET_SELECTED) {
            m_status &= ~WIDGET_SELECTED;
            if (msg.m_id == MESSAGE_RIGHT_BUTTON_UP) {
                msg.m_qualifier = MESSAGE_MODIFIER_RIGHT;
            }
            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = WIDGET_DESELECT;
            msg.m_codeY = m_id;
            return MESSAGE_DISPATCH_FORWARD;
        } else {
            return 0;
        }

    }

    return widget::main(msg);
}

DC_ADDRESS(0x164f7c, 0x4)
void textWidget::zBufferDraw(unsigned short* zBuffer, int id) const
{
}

VA(0x005bc5f0, 0x92)
DC_ADDRESS(0x164f80, 0xb2)
MAC_ADDRESS(0x1b1834, 0xfc)
void textWidget::draw() const
{
    if (m_status & WIDGET_DRAWN) {
        int drawX = m_x + m_parentWindow->m_x;
        int drawY = m_y + m_parentWindow->m_y;
        if (m_backColor) {
            g_windowManager->m_screenBitmap->FillRect(
                drawX, drawY, m_width, m_height,
                g_systemPalette->Palette[m_backColor]);
        }
        int colorScheme;
        if (m_status & WIDGET_DIMMED)
            colorScheme = m_color + 2;
        else
            colorScheme = m_color;
        m_font->DrawBoundedString(m_text.c_str(), g_windowManager->m_screenBitmap,
                                drawX, drawY, m_width, m_height,
                                font::TColor(colorScheme), m_justify, -1);
    }
}

VA(0x005bc690, 0x1)
DC_ADDRESS(0x165034, 0x4)
MAC_ADDRESS(0x1b1930, 0x4)
void textWidget::dim() const
{
}

// Original: iconBackedTextWidget::iconBackedTextWidget; textwdgt.cpp:266
DC_ADDRESS(0x165038, 0x58)
iconBackedTextWidget::iconBackedTextWidget()
    : m_background(0), m_backgroundFrame(0)
{
}

// Original: iconBackedTextWidget::iconBackedTextWidget; textwdgt.cpp:272
DC_ADDRESS(0x165090, 0x7e)
iconBackedTextWidget::iconBackedTextWidget(
    int x, int y, int w, int h, const char* text, const char* fontName,
    const char* backName, font::TColor color, int id, unsigned int justify,
    int style)
    : textWidget(x, y, w, h, text, fontName, color, id, justify, 0, style)
{
    m_background = ResourceManager::GetSprite(backName);
    m_backgroundFrame = 0;
}

// Original: iconBackedTextWidget::zBufferDraw; textwdgt.cpp:295
DC_ADDRESS(0x165110, 0x4)
void iconBackedTextWidget::zBufferDraw(unsigned short* zBuffer, int id) const {}

// Original: iconBackedTextWidget::Draw; textwdgt.cpp:299
DC_ADDRESS(0x165114, 0x6e)
void iconBackedTextWidget::draw() const
{
    int drawX = m_x + m_parentWindow->m_x;
    int drawY = m_y + m_parentWindow->m_y;
    m_background->DrawInterface(m_backgroundFrame, 0, 0,
        m_background->GetWidth(), m_background->GetHeight(),
        g_windowManager->m_screenBitmap, drawX, drawY, 0);
    textWidget::draw();
}

// Original: bitmapBackedTextWidget::bitmapBackedTextWidget; textwdgt.cpp:319
DC_ADDRESS(0x165184, 0x54)
bitmapBackedTextWidget::bitmapBackedTextWidget() : m_image(0) {}

VA_COMPGEN(0x005bc6a0, 0x21, SCALAR_DELETING_DTOR, bitmapBackedTextWidget)

// E:\gamedcs\textwdgt.cpp:320
// CodeView dc 0x1653b0: CV_fldattr_t.compgenx marks this destructor as
// implicit.
VA_COMPGEN(0x005bc6d0, 0x8A, IMPLICIT_DTOR, bitmapBackedTextWidget)
MAC_COMPGEN_ADDRESS(0x1b1a7c, 0x60, IMPLICIT_DTOR, bitmapBackedTextWidget)

VA(0x005bc760, 0x7B)
DC_ADDRESS(0x1651d8, 0x7a)
MAC_ADDRESS(0x1b1934, 0x74)
bitmapBackedTextWidget::bitmapBackedTextWidget(
    int x, int y, int w, int h, const char* text, const char* fontName,
    const char* backName, font::TColor color, int id, unsigned justify,
    int style)
    : textWidget(x, y, w, h, text, fontName, color, id, justify, 0, style)
{
    m_image = ResourceManager::GetBitmap816(backName);
}

// Claim-only home for the ordinary textWidget definition above. Retail
// textWidget and bitmapBackedTextWidget slot 3 (0x642dbc, 0x642df4) both
// target this `ret 8`; DC independently proves the two-argument const
// signatures and empty bodies. Other classes share this ICF representative.
#if 0  // @carcass

VA(0x005bc7e0, 0x3)
MAC_ADDRESS(0x1b1830, 0x4)  // anchor-vtable (0x642dbc, 0x642df4)
void textWidget::zBufferDraw(unsigned short* zBuffer, int id) const
{
    // @stub
}
#endif  // @carcass

// E:\gamedcs\textwdgt.cpp:345. Dreamcast proves a separate ordinary empty
// override; it shares textWidget's body above.
DC_ADDRESS(0x165254, 0x4)
void bitmapBackedTextWidget::zBufferDraw(unsigned short* zBuffer, int id) const
{
}

VA(0x005bc7f0, 0x7c)
DC_ADDRESS(0x165258, 0x9c)
MAC_ADDRESS(0x1b19ac, 0xd0)
void bitmapBackedTextWidget::draw() const
{
    int drawX = m_x + m_parentWindow->m_x;
    int drawY = m_y + m_parentWindow->m_y;
    int blitWidth = min(m_image->GetWidth(), m_width);
    int blitHeight = min(m_image->GetHeight(), m_height);
    m_image->Draw(0, 0, blitWidth, blitHeight,
                g_windowManager->m_screenBitmap, drawX, drawY, 0);
    textWidget::draw();
}
