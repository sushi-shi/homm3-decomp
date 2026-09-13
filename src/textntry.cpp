// textntry.cpp - E:\gamedcs\textntry.cpp (compiland textntry.obj)
// 21 functions in link order.
#include <va.h>
#include <string.h>
#include "textntry.h"
#include "bitmap816.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "message.h"
#include "resourcemanager.h"
#include "window.h"
#include "winmgr.h"

// E:\gamedcs\textntry.cpp:44 (dc 0x163750). Inlined into
// textEntryWidget::SaveBackground 0x5bba70, its only caller; `inline`
// so no out-of-line body is emitted, which is what retail shows.
inline void CTextEntrySave::save(int saveX, int saveY)
{
    m_saved = 1;
    grab(g_windowManager->m_screenBitmap->getMap(0, 0), saveX, saveY,
        g_windowManager->m_screenBitmap->getWidth(),
        g_windowManager->m_screenBitmap->getHeight(),
        g_windowManager->m_screenBitmap->getPitch());
}

#if 0  // @carcass

// E:\gamedcs\textntry.cpp:60
DC_ONLY(0x16298c, 0x5C)
void textEntryWidget::textEntryWidget()
{
    // @stub
}

// E:\gamedcs\textntry.cpp:158

#endif  // @carcass

VA(0x005ba920, 0x1B5)  // dc 0x1629e8
textEntryWidget::textEntryWidget(int x, int y, int w, int h, int textSize,
    const char* text, const char* fontName, font::TColor color,
    unsigned justification, const char* backgroundIcon, int backgroundFrame,
    int id, int style, int readType, int insetX, int insetY)
    : textWidget(x, y, w, h, text, fontName, color, id, justification, 0, 0x100)
{
    m_cursorIndex = 0;
    m_autoDraw = 0;
    m_textBack = 0;
    m_saveBack = 0;
    if (backgroundIcon)
        m_textBack = ResourceManager::getBitmap816(backgroundIcon);
    m_displayStart = 0;
    m_textLines = 1;
    m_maxLength = static_cast<unsigned short>(textSize);
    m_justify = justification;
    m_hasFocus = 0;
    if (text)
        m_text = text;
    if (readType == READ_TYPE_INSET) {
        m_attributes = 1;
        m_boxX = static_cast<short>(this->m_x + insetX);
        m_boxY = static_cast<short>(this->m_y + insetY);
        m_boxWidth = static_cast<short>(this->m_width - insetX * 2);
        m_boxHeight = static_cast<short>(this->m_height - insetY * 2);
    } else {
        m_attributes = 0;
        m_boxX = this->m_x;
        m_boxY = this->m_y;
        m_boxWidth = this->m_width;
        m_boxHeight = this->m_height;
    }
    m_cursorIndex = static_cast<unsigned short>(m_text.size());
}

VA_COMPGEN(0x005ba8f0, 0x21, SCALAR_DELETING_DTOR, textEntryWidget)

VA(0x005baae0, 0x62)  // dc 0x162af8
textEntryWidget::~textEntryWidget()
{
    if (m_textBack)
        m_textBack->dispose();
    if (m_saveBack)
        delete m_saveBack;
}

#if 0  // @carcass

#endif  // @carcass

VA(0x005bab50, 0x49)  // dc 0x162b50
void textEntryWidget::setFocus(unsigned char state)
{
    m_hasFocus = state;
    if (m_autoDraw) {
        draw();
        g_windowManager->updateScreen(m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y,
            m_width, m_height);
    }
}

#if 0  // @carcass

#endif  // @carcass

VA(0x005baba0, 0xA4)  // dc 0x162bbc
char textEntryWidget::getCharPressed(message* msg)
{
    char pressed = 0;

    if (msg->m_codeX >= 0x100) {
        int code = msg->m_codeX;
        int scanCode = (code & 0xFF00) >> 8;
        switch (scanCode) {
            case KEYCODE_KP_0:  // numpad Ins
                pressed = '0';
                break;
            case KEYCODE_KP_1:  // numpad End
                pressed = '1';
                break;
            case KEYCODE_KP_2:  // numpad Down
                pressed = '2';
                break;
            case KEYCODE_KP_3:  // numpad PgDn
                pressed = '3';
                break;
            case KEYCODE_KP_4:  // numpad Left
                pressed = '4';
                break;
            case KEYCODE_KP_5:  // numpad center
                pressed = '5';
                break;
            case KEYCODE_KP_6:  // numpad Right
                pressed = '6';
                break;
            case KEYCODE_KP_7:  // numpad Home
                pressed = '7';
                break;
            case KEYCODE_KP_8:  // numpad Up
                pressed = '8';
                break;
            case KEYCODE_KP_9:  // numpad PgUp
                pressed = '9';
                break;
        }
    } else {
        pressed = static_cast<char>(msg->m_codeX);
        if (pressed == '{' || pressed == '}')
            pressed = 0;
    }
    return pressed;
}

// ORDER-MAP 2026-08-14, exhaustive over the seam. The ten carve rows
// 0x5bac50..0x5bbac0 that sit between GetCharPressed and textresource's
// first row take textntry.cpp's remaining DC roster in source-line
// order with no leftovers on either side, and the retail vtable
// 0x642d40 independently pins nine of the ten to a slot:

//   0x5bac50 1277  line 213  OnKeyPress          slot 15
//   0x5bb150  678  line 335  Main                slot  2
//   0x5bb400  596  line 477  Draw                slot  4
//   0x5bb660  743  line 568  SetupDisplayString  (non-virtual)
//   0x5bb950  208  line 628  SetText             slot 13
//   0x5bba20   34  line 635  IgnoreKey           slot 16
//   0x5bba50    8  line 648  OnSetFocus          slot 10
//   0x5bba60    8  line 653  OnKillFocus         slot 11
//   0x5bba70   68  line 658  SaveBackground      slot 18
//   0x5bbac0  130  line 666  SetAutoDraw         slot 17

// The flanks are attributed: 0x5ba8d0 and 0x5bbb50 are both cinit rows
// (guard 0x6abaa0 + atexit), the excluded class. The five CTextEntrySave
// rows and the DC default constructor have no retail row at all - the
// first four are inlined at their single call sites (see the class
// above), the sdd is the far COMDAT 0x557310, and nothing in the image
// constructs a default textEntryWidget.

// One derived class exists: thirteen byte-identical vtables (0x63a578,
// 0x63d4bc, 0x63ebf4, 0x640054, ...) reproduce 0x642d40 slot for slot
// except 0 / 15 / 16 and run past slot 18, i.e. a header-defined
// subclass that overrides OnKeyPress and IgnoreKey. That is also the
// uniqueness proof for the two bodies below that it does NOT override:
// slot 15's 0x5bac50 is referenced exactly once image-wide.
#if 0  // @carcass

// E:\gamedcs\textntry.cpp:38 / :44 / :50 - CTextEntrySave's ctor, Save
// and IsSaved. All three are inlined into their single call sites
// (SetAutoDraw / SaveBackground / Draw); the definitions live at the
// top of this file. No retail row.
DC_ONLY(0x16370c, 0x44)
DC_ONLY(0x163750, 0x2C)
DC_ONLY(0x16377c, 0xA)

// E:\gamedcs\textntry.cpp:51
DC_ONLY(0x163788, 0x34)
void* CTextEntrySave::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\textntry.cpp:51
DC_ONLY(0x1637bc, 0x18)
void CTextEntrySave::~CTextEntrySave()
{
    // @stub
}

// The DC default constructor (line 60, dc 0x16298c, carcass at the top
// of this file) has no retail row either: nothing in the image stores
// 0x642d40 except the sixteen-argument constructor 0x5ba920.

#endif  // @carcass

// Retail .bss cell written here and referenced NOWHERE else in the
// image - 0x1bb0fe is the only reloc against it in the whole reloc
// table, and its two neighbours 0x697784/0x697788 are already other
// units' claims, so the cell is textntry.obj's own. Its role is not
// recoverable: no body reads it.
DATA(0x00697780) int g_unnamed697780;

VA(0x005bac50, 0x4FD)  // dc 0x162c2c
int textEntryWidget::onKeyPress(message* msg)
{
    if (!m_hasFocus)
        return 0;
    if (ignoreKey(msg))
        return 0;

    m_cursorFlashOn = 1;
    short xLoc = m_x + m_parentWindow->m_x;
    short yLoc = m_y + m_parentWindow->m_y;
    char core[600];
    char temp[600];
    char save[600];

    strcpy(core, m_text.c_str());
    if (m_cursorIndex > strlen(core))
        m_cursorIndex = static_cast<unsigned short>(strlen(core));

    switch (msg->m_codeX) {
        case KEYCODE_KP_1:
            m_cursorIndex = static_cast<unsigned short>(strlen(core));
            break;

        case KEYCODE_KP_7:
            m_cursorIndex = 0;
            break;

        case KEYCODE_KP_DECIMAL:
            if (m_cursorIndex < strlen(core)) {
                strcpy(temp, &core[m_cursorIndex + 1]);
                strcpy(&core[m_cursorIndex], temp);
            }
            break;

        case KEYCODE_KP_4:
            if (m_cursorIndex > 0) {
                m_cursorIndex--;
                if (m_cursorIndex < m_displayStart)
                    m_displayStart = m_cursorIndex;
            }
            break;

        case KEYCODE_KP_6:
            if (m_cursorIndex < strlen(core))
                m_cursorIndex++;
            break;

        default:
            g_inputManager->asciiConvert(msg);
            if (msg->m_codeX == KEYCODE_ASCII_BACKSPACE) {
                if (m_cursorIndex > 0) {
                    strcpy(temp, &core[m_cursorIndex]);
                    strcpy(&core[m_cursorIndex - 1], temp);
                    m_cursorIndex--;
                    if (m_cursorIndex < m_displayStart)
                        m_displayStart = m_cursorIndex;
                }
            } else if (strlen(core) + 1 < m_maxLength && msg->m_codeX) {
                strcpy(save, core);
                char pressed = getCharPressed(msg);
                if (pressed) {
                    strcpy(temp, m_text.c_str());
                    m_text = core;
                    temp[m_cursorIndex] = pressed;
                    temp[m_cursorIndex + 1] = 0;
                    strcat(temp, &core[m_cursorIndex]);
                    strcpy(core, temp);
                    m_cursorIndex++;
                    if (m_type != FIELD_68_SCROLLED) {
                        if (m_font->lineLength(m_text.c_str(), m_boxWidth) > m_textLines) {
                            strcpy(core, save);
                            m_cursorIndex--;
                        }
                    }
                }
            }
            break;
    }

    setupDisplayString(core, m_cursorIndex);
    if (m_autoDraw) {
        draw();
        g_windowManager->updateScreen(xLoc, yLoc, m_width, m_height);
    }
    g_unnamed697780 = 0;
    msg->m_id = MESSAGE_WIDGET;
    msg->m_codeX = WIDGET_SELECT;
    msg->m_codeY = m_id;
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x005bb150, 0x2A6)  // dc 0x162f2c
int textEntryWidget::main(message& msg)
{
    if (m_sleepCount > 0)
        return 0;

    unsigned char disabled = 0;
    if (m_status & WIDGET_DISABLED)
        disabled = 1;

    switch (msg.m_id) {
        case MESSAGE_KEY_DOWN:
            if (disabled)
                return 0;
            if ((m_status & WIDGET_DRAWN) && !(m_status & WIDGET_DIMMED))
                return onKeyPress(&msg);
            break;

        case MESSAGE_LEFT_BUTTON_DOWN:
            if (disabled)
                return 0;
            // fall through
        case MESSAGE_RIGHT_BUTTON_DOWN:
            if (!(m_status & WIDGET_DRAWN))
                return 0;
            {
                short hitX = msg.m_codeX - m_parentWindow->m_x;
                short hitY = msg.m_codeY - m_parentWindow->m_y;
                if (msg.m_id == MESSAGE_RIGHT_BUTTON_DOWN) {
                    if (hitX >= m_x && hitY >= m_y && hitX < m_x + m_width
                        && hitY < m_y + m_height) {
                        msg.m_id = MESSAGE_WIDGET;
                        msg.m_codeX = WIDGET_RIGHT_SELECT;
                        msg.m_codeY = m_id;
                        msg.m_qualifier = MESSAGE_MODIFIER_RIGHT;
                        return MESSAGE_DISPATCH_FORWARD;
                    }
                } else if (hitX >= m_x && hitY >= m_y && hitX < m_x + m_width
                    && hitY < m_y + m_height) {
                    if (!m_hasFocus && m_parentWindow)
                        m_parentWindow->setFocus(m_id);
                    msg.m_id = MESSAGE_WIDGET;
                    msg.m_codeX = WIDGET_SELECT;
                    msg.m_codeY = m_id;
                    return MESSAGE_DISPATCH_FORWARD;
                }
            }
            return 0;

        case MESSAGE_WIDGET:
            switch (msg.m_codeX) {
                case WIDGET_SET_TEXT_LEN:
                    if (msg.m_codeY == m_id) {
                        m_maxLength = static_cast<unsigned short>(msg.m_extra);
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
                case WIDGET_SET_TEXT:
                    if (msg.m_codeY == m_id) {
                        setText(msg.m_extraText);
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
                case WIDGET_GET_TEXT:
                    if (msg.m_codeY == m_id) {
                        msg.m_extraText = const_cast<char*>(m_text.c_str());
                        return MESSAGE_DISPATCH_CONSUME;
                    }
                    break;
                case WIDGET_SET_FOCUS:
                    if (msg.m_codeY == m_id) {
                        if (!m_hasFocus)
                            setFocus(1);
                    } else if (m_hasFocus) {
                        setFocus(0);
                    }
                    return 0;
            }
            break;

        default:
            if (disabled)
                return 0;
            break;
    }
    return widget::main(msg);
}

VA(0x005bb400, 0x254)  // dc 0x163150
void textEntryWidget::draw()
{
    if (!(m_status & WIDGET_DRAWN))
        return;

    if (m_textBack) {
        m_textBack->draw(0, 0, m_boxWidth, m_boxHeight,
            g_windowManager->m_screenBitmap,
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, 0);
    } else if (m_saveBack) {
        if (!m_saveBack->isSaved())
            saveBackground();
        else
            m_saveBack->draw(0, 0, m_boxWidth, m_boxHeight,
                g_windowManager->m_screenBitmap->getMap(0, 0),
                m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y,
                g_windowManager->m_screenBitmap->getWidth(),
                g_windowManager->m_screenBitmap->getHeight(),
                g_windowManager->m_screenBitmap->getPitch(), 0);
    }

    if (m_type == FIELD_68_SCROLLED) {
        char shown[600];
        strcpy(shown, m_text.substr(m_displayStart).c_str());
        int len = strlen(shown);
        while (m_font->lineWidth(shown) > m_boxWidth)
            shown[--len] = 0;
        if (m_hasFocus)
            m_font->drawBoundedString(shown, g_windowManager->m_screenBitmap,
                m_boxX + m_parentWindow->m_x, m_boxY + m_parentWindow->m_y,
                m_boxWidth, m_boxHeight, font::TColor(m_color), m_justify, m_cursorIndex);
        else
            m_font->drawBoundedString(shown, g_windowManager->m_screenBitmap,
                m_boxX + m_parentWindow->m_x, m_boxY + m_parentWindow->m_y,
                m_boxWidth, m_boxHeight, font::TColor(m_color), m_justify, -1);
    } else if (m_hasFocus) {
        m_font->drawBoundedString(m_text.c_str(), g_windowManager->m_screenBitmap,
            m_boxX + m_parentWindow->m_x, m_boxY + m_parentWindow->m_y,
            m_boxWidth, m_boxHeight,
            font::TColor((m_status & WIDGET_DIMMED) ? font::PRIMARY_DIM : m_color),
            m_justify, m_cursorIndex);
    } else {
        m_font->drawBoundedString(m_text.c_str(), g_windowManager->m_screenBitmap,
            m_boxX + m_parentWindow->m_x, m_boxY + m_parentWindow->m_y,
            m_boxWidth, m_boxHeight,
            font::TColor((m_status & WIDGET_DIMMED) ? font::PRIMARY_DIM : m_color),
            m_justify, -1);
    }
}

VA(0x005bb660, 0x2E7)  // dc 0x1633d8
void textEntryWidget::setupDisplayString(char* core, unsigned short inCursorIndex)
{
    if (GameTime::isPast(g_timers[0])) {
        m_cursorFlashOn = static_cast<unsigned char>(1 - m_cursorFlashOn);
        g_timers[0] = GameTime::get() + 360;
    }

    if (inCursorIndex > 0)
        m_text = std::string(core).substr(0, inCursorIndex);
    else
        m_text.erase();

    if (strlen(core) > inCursorIndex)
        m_text.append(core + inCursorIndex);

    if (m_type == FIELD_68_SCROLLED) {
        char shown[300];
        for (;;) {
            strcpy(shown, m_text.substr(m_displayStart).c_str());
            if (m_font->lineWidth(shown) <= m_boxWidth)
                break;
            shown[inCursorIndex - m_displayStart + 1] = 0;
            if (m_font->lineWidth(shown) <= m_boxWidth)
                break;
            m_displayStart++;
        }
        if (m_displayStart > 0) {
            strcpy(shown, m_text.substr(m_displayStart - 1).c_str());
            if (m_font->lineWidth(shown) <= m_boxWidth)
                m_displayStart--;
        }
    }
}

VA(0x005bb950, 0xD0)  // dc 0x1635dc
void textEntryWidget::setText(const char* newText)
{
    m_text = newText;
    m_cursorIndex = static_cast<unsigned short>(m_text.size());
}

VA(0x005bba20, 0x22)  // dc 0x163600
unsigned char textEntryWidget::ignoreKey(message* msg)
{
    switch (msg->m_codeX) {
        case KEYCODE_ESCAPE:
        case KEYCODE_TAB:
        case KEYCODE_ENTER:
            return 1;
    }
    return 0;
}

VA(0x005bba50, 0x8)  // dc 0x163620
void textEntryWidget::onSetFocus()
{
    setFocus(1);
}

VA(0x005bba60, 0x8)  // dc 0x163638
void textEntryWidget::onKillFocus()
{
    setFocus(0);
}

VA(0x005bba70, 0x44)  // dc 0x163650
void textEntryWidget::saveBackground() const
{
    if (m_saveBack)
        m_saveBack->save(m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y);
}

VA(0x005bbac0, 0x82)  // dc 0x16367c
void textEntryWidget::setAutoDraw(unsigned char b)
{
    m_autoDraw = b;
    if (b && !m_textBack && !m_saveBack)
        m_saveBack = new CTextEntrySave(m_boxWidth, m_boxHeight);
}
