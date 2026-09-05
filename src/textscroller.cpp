// textscroller.cpp - the Complete-era scenario-description scroller
// compiland (provisional unit name; see include/textscroller.h).
// HAND-OWNED after admission.
//
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

// VC6's own <xutility> reference-returning max, declared file-locally for
// the same reason textwdgt.cpp declares _cpp_min: the slider's state count
// stores BOTH operands to stack temps and selects between their ADDRESSES
// with two LEAs, which no value-returning spelling produces, and the TU
// needs no other <algorithm> surface.
template <class _TYPE>
inline const _TYPE& _cpp_max(_TYPE _X, _TYPE _Y)
{
    return (_X < _Y ? _Y : _X);
}

// Slot 16 of the scroller's private slider vtable 0x642cc8 - the only
// slot it overrides. Thirteen bytes, no frame: it reads the slider's own
// currentState and the owner at +0x68 and tail-calls the scroller's
// repaint.
VA(0x005B9FA0, 0xD)  // anchor-vtable 0x642cc8 slot 16, retail-only
void type_text_slider::Close()
{
    owner->Refresh(currentState);
}

// The scroller constructor. Retail fixes the whole argument list: the
// first five go to the widget base as (x, y, w, h) with a literal -1 id
// and style 1, the sixth is kept in the +0x30 font name, the seventh is
// forwarded to every per-line textWidget as its colour, and the eighth
// reaches the slider constructor's EGraphics slot.
VA(0x005B9FB0, 0x2FF)  // anchor-vtable 0x642d0c + slider/textWidget ctors, retail-only
type_text_scroller::type_text_scroller(const char* text, int x, int y,
                                       int w, int h, const char* fontName,
                                       font::TColor color,
                                       slider::EGraphics graphics)
    : widget(x, y, w, h, -1, 1), font_filename(fontName)
{
    font* textFont = ResourceManager::GetFont(font_filename);
    int lineY = this->y;
    background = 0;
    textFont->FillLinesVector(text, width - 24, &text_lines);

    int visibleLines = height / textFont->fs.height;
    if (text_lines.size() <= visibleLines) {
        textFont->FillLinesVector(text, width - 3, &text_lines);
        for (int pad = text_lines.size(); pad < visibleLines; pad++)
            text_lines.push_back(std::string(""));
    }

    textWidget* lineWidget;
    for (int line = 0; line < visibleLines; line++) {
        lineWidget = new textWidget(
            this->x, lineY, width, textFont->fs.height,
            text_lines[line].c_str(), font_filename, color, -1, 0, 0, 8);
        lineY += textFont->fs.height;
        line_images.push_back(lineWidget);
    }

    text_slider = new type_text_slider(
        this->x + width - 16, this->y, 16, height, -1,
        _cpp_max<int>(1, text_lines.size() - line_images.size() + 1),
        0, graphics, line_images.size(), 1, this);
    textFont->Dispose();
}

// The scalar deleting destructor, slot 0 of vtable 0x642d0c.
VA_COMPGEN(0x005BA2B0, 0x21, SCALAR_DELETING_DTOR, type_text_scroller)

// Slot 1. Hands every line widget and the slider to the opening window at
// consecutive priorities above the scroller's own, then folds the slider
// away when the text fits without scrolling.
VA(0x005BA2E0, 0xC6)  // anchor-vtable 0x642d0c slot 1 + AddWidget, retail-only
int type_text_scroller::Open(int newPriority, heroWindow* parent)
{
    int result = widget::Open(newPriority, parent);
    if (result)
        return result;

    for (unsigned int i = 0; i < line_images.size(); i++)
        parent->AddWidget(line_images[i], priority + i + 1);
    parent->AddWidget(text_slider, priority + line_images.size() + 1);

    if (text_lines.size() <= line_images.size()) {
        text_slider->SetState(0);
        text_slider->SetResolution(1);
        text_slider->hide();
    }
    return 0;
}

// The destructor. It owns every line widget, the slider and the grabbed
// backdrop; the two vector teardowns and ~widget are compiler-generated.
VA(0x005BA3B0, 0x101)  // anchor-vtable 0x642d0c slot 0 callee, retail-only
type_text_scroller::~type_text_scroller()
{
    for (unsigned int i = 0; i < line_images.size(); i++)
        delete line_images[i];
    delete text_slider;
    delete background;
}

// Slot 2. Only MESSAGE_WIDGET reaches the body: WIDGET_DRAW grabs the
// backdrop once, WIDGET_SET_STATUS / WIDGET_CLEAR_STATUS are relayed to
// every line and, when the text overflows, to the slider.
VA(0x005BA4C0, 0x13D)  // anchor-vtable 0x642d0c slot 2 + Grab, retail-only
int type_text_scroller::Main(message* msg)
{
    if (msg->id == MESSAGE_WIDGET) {
        switch (msg->codeX) {
        case WIDGET_DRAW:
            if (!background) {
                background = new Bitmap16Bit(width, height);
                background->Grab(gpWindowManager->screenBitmap->map,
                                 x + parentWindow->x, y + parentWindow->y,
                                 gpWindowManager->screenBitmap->Width,
                                 gpWindowManager->screenBitmap->Height,
                                 gpWindowManager->screenBitmap->Pitch);
            }
            break;
        case WIDGET_SET_STATUS:
        case WIDGET_CLEAR_STATUS:
            for (unsigned int i = 0; i < line_images.size(); i++)
                line_images[i]->send_message(
                    static_cast<widget::ECommands>(msg->codeX), msg->extra);
            if (text_lines.size() > line_images.size())
                text_slider->send_message(
                    static_cast<widget::ECommands>(msg->codeX), msg->extra);
            break;
        }
    }
    return widget::Main(msg);
}

// The repaint the slider's state-change hook drives: restore the grabbed
// backdrop, then re-text and redraw every visible line from `firstLine`.
VA(0x005BA600, 0xD7)  // anchor-callee (0x5b9fa0) + Bitmap16Bit::Draw, retail-only
void type_text_scroller::Refresh(int firstLine)
{
    background->Draw(0, 0, width - 16, height,
                     gpWindowManager->screenBitmap->map,
                     x + parentWindow->x, y + parentWindow->y,
                     gpWindowManager->screenBitmap->Width,
                     gpWindowManager->screenBitmap->Height,
                     gpWindowManager->screenBitmap->Pitch, false);

    for (unsigned int i = 0; i < line_images.size(); i++) {
        textWidget* lineWidget = line_images[i];
        lineWidget->SetText(text_lines[firstLine + i].c_str());
        lineWidget->Draw();
    }

    gpWindowManager->UpdateScreen(x + parentWindow->x, y + parentWindow->y,
                                  width, height);
}

// Re-wraps the whole scroller around a new string. The wrap width is
// re-tried at the narrow measure only when the wide one already fits, and
// the slider is re-ranged or hidden from the resulting line count.
// Residual (99.4444%): one instruction - our `push_back` expands the
// `insert(iterator, const T&)` forwarder and calls the three-argument
// primary (`push 1`), where retail calls the forwarder itself.  The whole
// budget ladder was swept: `erase(begin(), end())` in place of `clear()`
// 73.40, a direct two-argument `insert(end(), X)` 97.03, a direct
// three-argument `insert(end(), 1, X)` 39.40.  The remaining knob is a
// statement pin, which this tree does not admit.
VA(0x005BA6E0, 0x1EF)  // anchor-callee (font::FillLinesVector) + slider slots, retail-only
void type_text_scroller::SetText(const char* text)
{
    font* textFont = ResourceManager::GetFont(font_filename);
    text_lines.clear();
    textFont->FillLinesVector(text, width - 24, &text_lines);

    if (text_lines.size() <= line_images.size()) {
        text_lines.clear();
        textFont->FillLinesVector(text, width - 3, &text_lines);
        for (unsigned int pad = text_lines.size();
             pad < line_images.size(); pad++)
            text_lines.push_back(std::string(""));
        text_slider->SetState(0);
        text_slider->SetResolution(1);
        text_slider->hide();
    } else {
        if (status & WIDGET_DRAWN)
            text_slider->show();
        text_slider->SetResolution(
            text_lines.size() - line_images.size() + 1);
        text_slider->SetState(0);
    }

    for (unsigned int i = 0; i < line_images.size(); i++)
        line_images[i]->SetText(text_lines[i].c_str());
    textFont->Dispose();
}
