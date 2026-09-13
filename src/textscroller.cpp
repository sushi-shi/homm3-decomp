// textscroller.cpp - the Complete-era scenario-description scroller
// compiland (provisional unit name; see include/textscroller.h).

// LINK-ORDER BRACKET: text.obj's last row ends at 0x5b9f7c and
// textntry.obj's own cinit/atexit thunk opens at 0x5ba8d0, so this
// compiland is exactly 0x5b9f80..0x5ba8cf - its own atexit thunk
// (0x5b9f80, the excluded cinit class) plus the eight rows below.
// textwdgt.obj is NOT the owner: its banked band runs 0x5bc230..0x5bc890,
// past textntry.obj entirely.
#include <va.h>
#include <string>
#include <vector>
#include "textscroller.h"
#include "bitmap16.h"
#include "font.h"
#include "message.h"
#include "resourcemanager.h"
#include "slider.h"
#include "textwdgt.h"
#include "widget.h"
#include "window.h"
#include "winmgr.h"
#include "homm3_minmax.h"

// VC6's own <xutility> reference-returning max, declared file-locally for
// the same reason textwdgt.cpp declares _cpp_min: the slider's state count
// stores BOTH operands to stack temps and selects between their ADDRESSES
// with two LEAs, which no value-returning spelling produces, and the TU
// needs no other <algorithm> surface.
template <class _TYPE>
inline const _TYPE& cppMax(_TYPE x, _TYPE y)
{
    return (x < y ? y : x);
}

// Slot 16 of the scroller's private slider vtable 0x642cc8 - the only
// slot it overrides. Thirteen bytes, no frame: it reads the slider's own
// currentState and the owner at +0x68 and tail-calls the scroller's
// repaint.
VA(0x005B9FA0, 0xD)
void type_text_slider::close()
{
    m_owner->refresh(m_currentState);
}

VA(0x005B9FB0, 0x2FF)
type_text_scroller::type_text_scroller(const char* text, int x, int y,
                                       int w, int h, const char* fontName,
                                       font::TColor color,
                                       slider::EGraphics graphics)
    : widget(x, y, w, h, -1, 1), m_fontFilename(fontName)
{
    font* textFont = ResourceManager::getFont(m_fontFilename);
    int lineY = this->m_y;
    m_background = 0;
    textFont->fillLinesVector(text, m_width - 24, m_textLines);

    int visibleLines = m_height / textFont->m_fs.m_height;
    if (m_textLines.size() <= visibleLines) {
        textFont->fillLinesVector(text, m_width - 3, m_textLines);
        for (int pad = m_textLines.size(); pad < visibleLines; pad++)
            m_textLines.push_back(std::string(""));
    }

    textWidget* lineWidget;
    for (int line = 0; line < visibleLines; line++) {
        lineWidget = new textWidget(
            this->m_x, lineY, m_width, textFont->m_fs.m_height,
            m_textLines[line].c_str(), m_fontFilename, color, -1, 0, 0, 8);
        lineY += textFont->m_fs.m_height;
        m_lineImages.push_back(lineWidget);
    }

    m_textSlider = new type_text_slider(
        this->m_x + m_width - 16, this->m_y, 16, m_height, -1,
        max(1, m_textLines.size() - m_lineImages.size() + 1),
        0, graphics, m_lineImages.size(), 1, this);
    textFont->dispose();
}

// The scalar deleting destructor, slot 0 of vtable 0x642d0c.
VA_COMPGEN(0x005BA2B0, 0x21, SCALAR_DELETING_DTOR, type_text_scroller)

// Slot 1. Hands every line widget and the slider to the opening window at
// consecutive priorities above the scroller's own, then folds the slider
// away when the text fits without scrolling.
VA(0x005BA2E0, 0xC6)
int type_text_scroller::open(int newPriority, heroWindow* parent)
{
    int result = widget::open(newPriority, parent);
    if (result)
        return result;

    for (unsigned int i = 0; i < m_lineImages.size(); i++)
        parent->addWidget(m_lineImages[i], m_priority + i + 1);
    parent->addWidget(m_textSlider, m_priority + m_lineImages.size() + 1);

    if (m_textLines.size() <= m_lineImages.size()) {
        m_textSlider->setState(0);
        m_textSlider->setResolution(1);
        m_textSlider->hide();
    }
    return 0;
}

VA(0x005BA3B0, 0x101)
type_text_scroller::~type_text_scroller()
{
    for (unsigned int i = 0; i < m_lineImages.size(); i++)
        delete m_lineImages[i];
    delete m_textSlider;
    delete m_background;
}

// Slot 2. Only MESSAGE_WIDGET reaches the body: WIDGET_DRAW grabs the
// backdrop once, WIDGET_SET_STATUS / WIDGET_CLEAR_STATUS are relayed to
// every line and, when the text overflows, to the slider.
VA(0x005BA4C0, 0x13D)
int type_text_scroller::main(message& msg)
{
    if (msg.m_id == MESSAGE_WIDGET) {
        switch (msg.m_codeX) {
        case WIDGET_DRAW:
            if (!m_background) {
                m_background = new Bitmap16Bit(m_width, m_height);
                m_background->grab(g_windowManager->m_screenBitmap->getMap(0, 0),
                                 m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y,
                                 g_windowManager->m_screenBitmap->getWidth(),
                                 g_windowManager->m_screenBitmap->getHeight(),
                                 g_windowManager->m_screenBitmap->getPitch());
            }
            break;
        case WIDGET_SET_STATUS:
        case WIDGET_CLEAR_STATUS:
            for (unsigned int i = 0; i < m_lineImages.size(); i++)
                m_lineImages[i]->sendMessage(
                    static_cast<widget::ECommands>(msg.m_codeX), msg.m_extra);
            if (m_textLines.size() > m_lineImages.size())
                m_textSlider->sendMessage(
                    static_cast<widget::ECommands>(msg.m_codeX), msg.m_extra);
            break;
        }
    }
    return widget::main(msg);
}

// The repaint the slider's state-change hook drives: restore the grabbed
// backdrop, then re-text and redraw every visible line from `firstLine`.
VA(0x005BA600, 0xD7)
void type_text_scroller::refresh(int firstLine)
{
    m_background->draw(0, 0, m_width - 16, m_height,
                     g_windowManager->m_screenBitmap->getMap(0, 0),
                     m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y,
                     g_windowManager->m_screenBitmap->getWidth(),
                     g_windowManager->m_screenBitmap->getHeight(),
                     g_windowManager->m_screenBitmap->getPitch(), false);

    for (unsigned int i = 0; i < m_lineImages.size(); i++) {
        textWidget* lineWidget = m_lineImages[i];
        lineWidget->setText(m_textLines[firstLine + i].c_str());
        lineWidget->draw();
    }

    g_windowManager->updateScreen(m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y,
                                  m_width, m_height);
}

// Re-wraps the whole scroller around a new string. The wrap width is
// re-tried at the narrow measure only when the wide one already fits, and
// the slider is re-ranged or hidden from the resulting line count.
// Residual (99.4444%): one instruction - our `push_back` expands the
// `insert(iterator, const T&)` forwarder and calls the three-argument
// primary (`push 1`), where retail calls the forwarder itself.  The whole
// budget ladder was swept: `erase(begin(), end())` in place of `clear()`
// 73.40, a direct two-argument `insert(end(), X)` 97.03, a direct
// three-argument `insert(end(), 1, X)` 39.40. These controls leave the
// source of the caller's different expansion decision unresolved.
// The padding loop's push_back lowers to vector<string>::insert(pos, n, value)
// here where retail calls insert(pos, value) - the two-argument overload that
// returns an iterator.  Spelling the site as that overload directly
// (`text_lines.insert(text_lines.end(), std::string(""))`) is MEASURED AND
// REJECTED 2026-09-06 at 97.0339 against 99.4361: it produces retail's callee
// but loses the surrounding block.  push_back stays.
// The verified retail body constructs the padding temp, scans the empty
// literal and assigns it before insertion, then destroys it each iteration.
// The older default-constructor-only interpretation was incorrect; its probe
// lost that real assign path (89.7838%). Thirteen further source states
// (six objects, all independently reproduced) test implicit empty-string
// conversion, a named per-iteration string, and signed padding indices.
// push_back remains 99.4444%; direct insert remains 97.0317%, with no sibling
// movement. No source change from this family is retained.
// A passive C2 trace reproduces the unchanged object: the insert forwarder
// has cb 64 and fits the push_back child budget 68. Four actual padding/refresh
// loop-scope combinations are flat (one object), as are eighteen refresh
// element-access/lifetime forms (twelve objects, ten reproduced elites).
// Four further padding-construction forms produce four reproduced objects:
// copy initialization stays at 99.4444%; default construction followed by either
// operator= or assign scores 98.8095%, moves the EH-live transition ahead of
// assignment, and still calls the wrong insert overload. All siblings are
// unchanged. Keep the per-iteration temporary's constructor/assign/destructor
// path and canonical push_back; none of these alternatives is retained.
// Twelve further states (93d6a3b51ac327888fe5) cross the constructor max
// correction with named/const string values and const references that extend
// the padding temporary's lifetime. Ten objects and eight reproduced elites
// remain at 99.4444%; neither explicit nor implicit reference initialization
// recovers retail's retained insert forwarder. No padding edit is retained.
VA(0x005BA6E0, 0x1EF)  // anchor-callee (font::FillLinesVector) + slider slots, retail-only
void type_text_scroller::setText(const char* text)
{
    font* textFont = ResourceManager::getFont(m_fontFilename);
    m_textLines.clear();
    textFont->fillLinesVector(text, m_width - 24, m_textLines);

    if (m_textLines.size() <= m_lineImages.size()) {
        m_textLines.clear();
        textFont->fillLinesVector(text, m_width - 3, m_textLines);
        for (unsigned int pad = m_textLines.size();
             pad < m_lineImages.size(); pad++)
            m_textLines.push_back(std::string(""));
        m_textSlider->setState(0);
        m_textSlider->setResolution(1);
        m_textSlider->hide();
    } else {
        if (m_status & WIDGET_DRAWN)
            m_textSlider->show();
        m_textSlider->setResolution(
            m_textLines.size() - m_lineImages.size() + 1);
        m_textSlider->setState(0);
    }

    for (unsigned int i = 0; i < m_lineImages.size(); i++)
        m_lineImages[i]->setText(m_textLines[i].c_str());
    textFont->dispose();
}
