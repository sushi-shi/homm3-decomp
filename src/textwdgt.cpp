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

// Original: textWidget::textWidget; textwdgt.cpp:36, dc 0x164c14.
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
// image builds its labels with. `ret 0x2c` is eleven stack dwords, which is
// exactly this declarator; the base call is
// `??0widget@@QAE@FFFFFF@Z(x, y, w, h, id, 8)` with the style argument a
// LITERAL 8, and the vtable store names 0x642db0. The trailing `style`
// parameter is read nowhere in the body - retail carries it dead, exactly as
// the header's declarator does.

// Residual (99.9130%): ONE instruction, and it is inside the inlined
// `basic_string::assign(const char*)`, not in this body. Retail terminates
// the copy with `mov byte ptr [ecx+eax], 0` - SIB base `_Ptr`, index `_Len` -
// where this compile emits the same store with the two registers exchanged
// (`[eax+ecx]`). All 16 blocks, 9 branches, 7 calls and 9 relocations agree
// exactly; the operand roles are chosen inside a vendored Dinkumware header
// this TU may not respell.

// The three trailing member stores WERE reachable, +0.05: retail emits them
// backColor -> 0x48, justify -> 0x4c, color -> 0x44, and VC6 rotates the
// emitted run one place left against source order, so the source has to be
// written `Color; BackColor; Justify;` to land retail's order.
VA(0x005bc280, 0x12D)  // anchor-vtable 0x642db0 + ret 0x2c, dc 0x164c80
textWidget::textWidget(int x, int y, int w, int h, const char* text,
                       const char* fontName, font::TColor color, int id,
                       unsigned justify, int backColor, int style)
    : widget(static_cast<short>(x), static_cast<short>(y),
             static_cast<short>(w), static_cast<short>(h),
             static_cast<short>(id), 8)
{
    m_font = ResourceManager::getFont(fontName);
    // DC textwdgt.cpp:64..65 records the conditional body's lexical scope.
    // Removing these braces leaves this constructor's bytes unchanged, but
    // makes VC6 expand it in bitmapBackedTextWidget (retail call 0x5bc7ab).
    if (text) {
        m_text = text;
    }
    m_color = color;
    m_backColor = backColor;
    m_justify = justify;
}

VA(0x005bc3b0, 0x8A)  // dc 0x164d24
textWidget::~textWidget()
{
    m_font->dispose();
}

// Original: textWidget::initialize; textwdgt.cpp:102, dc 0x164d68.
// Complete has no widget::focusable member; the remaining fields and calls
// are shared with the retained parameterized constructor.
void textWidget::initialize(int x, int y, int w, int h, int id, int style,
                             const char* text, const char* fontName,
                             font::TColor color, unsigned int justify,
                             unsigned char focusable)
{
    widget::initialize(x, y, w, h, id, style);
    m_font = ResourceManager::getFont(fontName);
    if (text) {
        m_text = text;
    }
    m_justify = justify;
    m_color = color;
}

// E:\gamedcs\textwdgt.cpp:120
VA(0x005bc440, 0x1AD)  // vtable 0x642db0 slot 2 + widget-message protocol, dc 0x164dd4
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

void textWidget::zBufferDraw(unsigned short* zBuffer, int id) const
{
}

VA(0x005bc5f0, 0x92)  // dc 0x164f80
void textWidget::draw() const
{
    if (m_status & WIDGET_DRAWN) {
        int drawX = m_x + m_parentWindow->m_x;
        int drawY = m_y + m_parentWindow->m_y;
        if (m_backColor) {
            g_windowManager->m_screenBitmap->fillRect(
                drawX, drawY, m_width, m_height,
                g_systemPalette->m_data[m_backColor]);
        }
        int colorScheme;
        if (m_status & WIDGET_DIMMED)
            colorScheme = m_color + 2;
        else
            colorScheme = m_color;
        m_font->drawBoundedString(m_text.c_str(), g_windowManager->m_screenBitmap,
                                drawX, drawY, m_width, m_height,
                                font::TColor(colorScheme), m_justify, -1);
    }
}

VA(0x005bc690, 0x1)  // dc 0x165034
void textWidget::dim() const
{
}

// Original: iconBackedTextWidget::iconBackedTextWidget; textwdgt.cpp:266, dc 0x165038.
iconBackedTextWidget::iconBackedTextWidget()
    : m_background(0), m_backgroundFrame(0)
{
}

// Original: iconBackedTextWidget::iconBackedTextWidget; textwdgt.cpp:272, dc 0x165090.
iconBackedTextWidget::iconBackedTextWidget(
    int x, int y, int w, int h, const char* text, const char* fontName,
    const char* backName, font::TColor color, int id, unsigned int justify,
    int style)
    : textWidget(x, y, w, h, text, fontName, color, id, justify, 0, style)
{
    m_background = ResourceManager::getSprite(backName);
    m_backgroundFrame = 0;
}

// Original: iconBackedTextWidget::zBufferDraw; textwdgt.cpp:295, dc 0x165110.
void iconBackedTextWidget::zBufferDraw(unsigned short* zBuffer, int id) const {}

// Original: iconBackedTextWidget::Draw; textwdgt.cpp:299, dc 0x165114.
void iconBackedTextWidget::draw() const
{
    int drawX = m_x + m_parentWindow->m_x;
    int drawY = m_y + m_parentWindow->m_y;
    m_background->drawInterface(m_backgroundFrame, 0, 0,
        m_background->getWidth(), m_background->getHeight(),
        g_windowManager->m_screenBitmap, drawX, drawY, 0);
    textWidget::draw();
}

// Original: bitmapBackedTextWidget::bitmapBackedTextWidget; textwdgt.cpp:319, dc 0x165184.
bitmapBackedTextWidget::bitmapBackedTextWidget() : m_image(0) {}

VA_COMPGEN(0x005bc6a0, 0x21, SCALAR_DELETING_DTOR, bitmapBackedTextWidget)

// E:\gamedcs\textwdgt.cpp:320
// CodeView dc 0x1653b0: CV_fldattr_t.compgenx marks this destructor
// as implicit. Its retained retail body performs only base/member teardown.
VA_COMPGEN(0x005bc6d0, 0x8A, IMPLICIT_DTOR, bitmapBackedTextWidget)

VA(0x005bc760, 0x7B)  // dc 0x1651d8
bitmapBackedTextWidget::bitmapBackedTextWidget(
    int x, int y, int w, int h, const char* text, const char* fontName,
    const char* backName, font::TColor color, int id, unsigned justify,
    int style)
    : textWidget(x, y, w, h, text, fontName, color, id, justify, 0, style)
{
    m_image = ResourceManager::getBitmap816(backName);
}

// Claim-only home for the ordinary textWidget definition above. Retail
// textWidget and bitmapBackedTextWidget slot 3 (0x642dbc, 0x642df4) both
// target this `ret 8`; DC independently proves the two-argument const
// signatures and empty bodies. Other classes share this ICF representative.
// Full VC6 checkpoint: both text claims and widget::OnSetFocus are exact;
// both text vtables remain 14 slots with the proven overrides at 3 and 8.
// The shared const interface moves unchanged CEnterNameEdit::OnKillFocus
// CUR 100 -> 99.8710; its MAX/HIST stay 100 and no banked peak is lost.
#if 0  // @carcass
VA(0x005bc7e0, 0x3)  // anchor-vtable (0x642dbc, 0x642df4), dc 0x164f7c
void textWidget::zBufferDraw(unsigned short* zBuffer, int id) const
{
    // @stub
}
#endif  // @carcass

// E:\gamedcs\textwdgt.cpp:345. Dreamcast dc 0x165254 proves a separate
// ordinary empty override; retail folds it with textWidget's body above.
void bitmapBackedTextWidget::zBufferDraw(unsigned short* zBuffer, int id) const
{
}

VA(0x005bc7f0, 0x7c)  // dc 0x165258
void bitmapBackedTextWidget::draw() const
{
    int drawX = m_x + m_parentWindow->m_x;
    int drawY = m_y + m_parentWindow->m_y;
    int blitWidth = min(m_image->getWidth(), m_width);
    int blitHeight = min(m_image->getHeight(), m_height);
    m_image->draw(0, 0, blitWidth, blitHeight,
                g_windowManager->m_screenBitmap, drawX, drawY, 0);
    textWidget::draw();
}
