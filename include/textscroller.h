// textscroller.h - prototypes of textscroller.cpp, the Complete-era
// compiland that owns the scenario-description scroller.
// HAND-OWNED after admission.
//
// PROVISIONAL UNIT NAME. The Dreamcast roster has no module between
// text.obj and textntry.obj, and no DC compiland declares either class
// below, so the 0x5b9f80..0x5ba8cf block is Complete-only: its own
// cinit/atexit thunk opens it at 0x5b9f80 and textntry's closes it at
// 0x5ba8d0, and textwdgt.obj's own band starts far later at 0x5bc230.
// `textscroller` is the house name for it; `type_text_scroller` itself
// is the retail-attested class identity declared below.
#ifndef HOMM3_TEXTSCROLLER_H
#define HOMM3_TEXTSCROLLER_H

#include <string>
#include <vector>
#include "slider.h"
#include "textwdgt.h"

class Bitmap16Bit;

class type_text_slider;

// The scenario-description scroller shared by the selection window and the
// stand-alone scenario-info popup. Retail fixes its original type identity,
// constructor ABI, member offsets, and complete 0x5c extent.
class type_text_scroller : public widget {
public:
    type_text_scroller(const char* text, int x, int y, int w, int h,
                       const char* fontName, font::TColor color,
                       slider::EGraphics graphics);
    virtual ~type_text_scroller();
    // Before normalization (function): type_text_scroller::Open.
    virtual int open(int priority, heroWindow* parent);
    // Before normalization (function): type_text_scroller::Main.
    virtual int main(message* msg);
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    // Before normalization (function): type_text_scroller::Draw.
    virtual void draw() const;
    // Before normalization (function): type_text_scroller::SetText.
    void setText(const char* text);
    // Before normalization (function): type_text_scroller::Refresh.
    void refresh(int knobRange);

    // Before normalization: font_filename.
    const char* m_fontFilename;
    // Before normalization: text_lines.
    std::vector<std::string> m_textLines;
    // Before normalization: line_images.
    std::vector<textWidget*> m_lineImages;
    // Before normalization: text_slider.
    type_text_slider* m_textSlider;
    // Before normalization: background.
    Bitmap16Bit* m_background;
};
SIZE(type_text_scroller, 0x5c);

// Compatibility spelling for already reconstructed callers. Being a typedef,
// it emits the retail `type_text_scroller` decorated names rather than a
// second source-false class identity.
typedef type_text_scroller CScrollTextWidget;

#endif  /* HOMM3_TEXTSCROLLER_H */
