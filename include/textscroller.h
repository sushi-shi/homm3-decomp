// textscroller.h - prototypes of textscroller.cpp, the Complete-era
// compiland that owns the scenario-description scroller.

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

class TextSlider;

// The scenario-description scroller shared by the selection window and the
// stand-alone scenario-info popup. Retail fixes its original type identity,
// constructor ABI, member offsets, and complete 0x5c extent.
// Before normalization (type): type_text_scroller.
class TextScroller : public Widget {
public:
    TextScroller(const char* text, int x, int y, int w, int h,
                       const char* fontName, Font::Color color,
                       Slider::Graphics graphics);
    virtual ~TextScroller();
    virtual int open(int priority, HeroWindow* parent);
    virtual int main(Message& msg);
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw() const;
    void setText(const char* text);
    void refresh(int knobRange);
    const char* m_fontFilename;
    std::vector<std::string> m_textLines;
    std::vector<TextWidget*> m_lineImages;
    TextSlider* m_textSlider;
    Bitmap16Bit* m_background;
};
SIZE(TextScroller, 0x5c);

// Compatibility spelling for already reconstructed callers. Being a typedef,
// it emits the retail `type_text_scroller` decorated names rather than a
// second source-false class identity.
typedef TextScroller CScrollTextWidget;

#endif  /* HOMM3_TEXTSCROLLER_H */
