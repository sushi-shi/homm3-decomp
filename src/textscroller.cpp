// textscroller.cpp - the Complete-era scenario-description scroller
// compiland (provisional unit name; see include/textscroller.h).

// LINK-ORDER BRACKET: text.obj's last row ends at 0x5b9f7c and
// textntry.obj's own cinit/atexit thunk opens at 0x5ba8d0, so this
// compiland is exactly 0x5b9f80..0x5ba8cf - its own atexit thunk
// (0x5b9f80, the excluded cinit class) plus the eight rows below.
// textwdgt.obj is NOT the owner: its banked band runs 0x5bc230..0x5bc890,
// past textntry.obj entirely.
#include "va.h"
#include "includes.h"

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

// The scroller's private slider. Retail proves the whole shape from the
// constructor 0x5b9fb0 and the vtable 0x642cc8: a 0x6c-byte object whose
// slider base ctor runs with the ten ordinary slider arguments, whose
// vptr is then re-stored to 0x642cc8, and whose one extra dword at +0x68
// is the owning scroller. That vtable copies slider's sixteen inherited
// slots and overrides only slot 16, the state-change hook, at 0x5b9fa0.
class type_text_slider : public slider {
public:
    type_text_scroller* m_owner;  // +0x68

    type_text_slider(int x, int y, int w, int h, int id, int num,
                     TSliderFunction func, EGraphics graphics, int page,
                     unsigned char hotKey, type_text_scroller* scroller);

    virtual void close();  // slot 16, retail 0x5b9fa0
};
SIZE(type_text_slider, 0x6c);

MAC_ADDRESS(0x25b0c4, 0x80)
type_text_slider::type_text_slider(int x, int y, int w, int h, int id, int num,
                                   TSliderFunction func, EGraphics graphics,
                                   int page, unsigned char hotKey,
                                   type_text_scroller* scroller)
    : slider(x, y, w, h, id, num, func, graphics, page, hotKey)
{
    m_owner = scroller;
}

// Slot 16 of the scroller's private slider vtable 0x642cc8 - the only
// slot it overrides. Thirteen bytes, no frame: it reads the slider's own
// currentState and the owner at +0x68 and calls the scroller's
// repaint.
VA(0x005B9FA0, 0xD) MAC_ADDRESS(0x25b144, 0x2c)
void type_text_slider::close()
{
    m_owner->refresh(m_currentState);
}

// x/y/w/h arguments go to the widget base with a literal -1 id
VA(0x005B9FB0, 0x2FF) MAC_ADDRESS(0x25b170, 0x2d4)
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
VA(0x005BA2E0, 0xC6) MAC_ADDRESS(0x25b444, 0xfc)
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

VA(0x005BA3B0, 0x101) MAC_ADDRESS(0x25b540, 0x12c)
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
VA(0x005BA4C0, 0x13D) MAC_ADDRESS(0x25b670, 0x144)
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
VA(0x005BA600, 0xD7) MAC_ADDRESS(0x25b7b8, 0x120)
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
// Residual (99.4444%): 29/30 blocks are exact; the empty-line push retains
// one extra count argument. Calling insert(end(), value) directly perturbs
// the /Ob2 frontier and falls to 97.03%, so keep the canonical push_back.
VA(0x005BA6E0, 0x1EF) MAC_ADDRESS(0x25b8d8, 0x1dc)  // anchor-callee (font::FillLinesVector) + slider slots, retail-only
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

// Retail vtable 0x642d0c slots 3/4 share the empty ret 8 / ret bodies
// at 0x5bc7e0 / 0x5bc690. Child widgets draw the scroller's contents.
void type_text_scroller::zBufferDraw(unsigned short* zBuffer, int id) const
{
}

void type_text_scroller::draw() const
{
}
