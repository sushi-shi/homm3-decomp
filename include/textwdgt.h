// textwdgt.h - prototypes of textwdgt.cpp (compiland textwdgt.obj)
#ifndef HOMM3_TEXTWDGT_H
#define HOMM3_TEXTWDGT_H

#include <string>
#include <vector>
#include "widget.h"
#include "font.h"
#include "slider.h"

// Dreamcast roster with the STLport->VC6 string shift: Text@0x30
// (16 B), Font@0x40, Color@0x44, BackColor@0x48, Justify@0x4c - total
// 0x50. Vtable 0x642db0; the dtor Disposes the font, the string
// teardown is implicit.
class textWidget : public widget {
public:
    std::string m_text;
    font* m_font;
    int m_color;
    int m_backColor;
    unsigned int m_justify;

    textWidget(int x, int y, int w, int h, const char* text,
               const char* fontName, font::TColor color, int id,
               unsigned justify, int backColor, int style);
    textWidget(int x, int y, int w, int h, const char* text,
               const char* fontName, font::TColor color, int id,
               unsigned justify, int backColor, unsigned char focusable);
    virtual ~textWidget();  // retail 0x5bc3b0
    virtual int main(message& msg);
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw();
    virtual void dim() const;

    // Original spelling SetColor(new_color), TextWdgt.h:78..81 (dc 0x1652f4).
    // Main calls this canonical header helper at textwdgt.cpp:152; retail
    // expands its single member store in the WIDGET_SET_COLOR arm.
    void setColor(font::TColor newColor) { m_color = newColor; }
    virtual void setText(const char* newText) { m_text = newText; }

    // E:\gamedcs\TextWdgt.h:67; DC emits this header helper out of line,
    // while Complete folds the c_str() access into its callers.
    // Class-inline as in TextWdgt.h:67. Removing the unsupported forceinline
    // qualifier is byte-neutral across the affected widget/name-edit callers.
    const char* getText() { return m_text.c_str(); }
};

class Bitmap816;
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
    virtual int open(int priority, heroWindow* parent);
    virtual int main(message& msg);
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw();
    void setText(const char* text);
    void refresh(int knobRange);

    const char* m_fontFilename;
    std::vector<std::string> m_textLines;
    std::vector<textWidget*> m_lineImages;
    type_text_slider* m_textSlider;
    Bitmap16Bit* m_background;
};
SIZE(type_text_scroller, 0x5c);

// Compatibility spelling for already reconstructed callers. Being a typedef,
// it emits the retail `type_text_scroller` decorated names rather than a
// second source-false class identity.
typedef type_text_scroller CScrollTextWidget;

// Retail dtor 0x5bc6d0 is the empty derived dtor: the inlined
// ~textWidget body under this class's vtable store, then ~widget.
// It does NOT free the backing bitmap - that resource is borrowed.
class bitmapBackedTextWidget : public textWidget {
public:
    // +0x50: the 11-argument constructor 0x5bc760 stores GetBitmap816's
    // result here, and Draw 0x5bc7f0 blits out of it after clamping the
    // widget extent against its +0x24/+0x28 Width/Height.
    Bitmap816* m_image;

    bitmapBackedTextWidget(int x, int y, int w, int h, const char* text,
                           const char* fontName, const char* backName,
                           font::TColor color, int id, unsigned justify,
                           int style);
    virtual ~bitmapBackedTextWidget();
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw();  // slot 4, retail 0x5bc7f0
};

// --- bitmapBackedTextWidget ---
// CODEVIEW(E:\gamedcs\textwdgt.cpp:319, dc 0x165184) void bitmapBackedTextWidget::bitmapBackedTextWidget();
// CODEVIEW(E:\gamedcs\textwdgt.cpp:345, dc 0x165254) void bitmapBackedTextWidget::zBufferDraw(unsigned short*, int) const;
// CODEVIEW(E:\gamedcs\textwdgt.cpp:320, dc 0x16537c) void* bitmapBackedTextWidget::`scalar deleting destructor'(unsigned __flags);

// --- iconBackedTextWidget ---
// CODEVIEW(E:\gamedcs\textwdgt.cpp:266, dc 0x165038) void iconBackedTextWidget::iconBackedTextWidget();
// CODEVIEW(E:\gamedcs\textwdgt.cpp:272, dc 0x165090) void iconBackedTextWidget::iconBackedTextWidget(int x, int y, int w, int h, const char* text, const char* font, const char* back, font::TColor color, int id, unsigned justify, int style);
// CODEVIEW(E:\gamedcs\textwdgt.cpp:295, dc 0x165110) void iconBackedTextWidget::zBufferDraw();
// CODEVIEW(E:\gamedcs\textwdgt.cpp:299, dc 0x165114) void iconBackedTextWidget::Draw();
// CODEVIEW(E:\gamedcs\textwdgt.cpp:267, dc 0x165330) void* iconBackedTextWidget::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\textwdgt.cpp:267, dc 0x165364) void iconBackedTextWidget::~iconBackedTextWidget();

// --- textWidget ---
// CODEVIEW(E:\gamedcs\textwdgt.cpp:36, dc 0x164c14) void textWidget::textWidget();
// CODEVIEW(E:\gamedcs\textwdgt.cpp:62, dc 0x164c80) void textWidget::textWidget(int textWidgetX, int textWidgetY, int textWidgetWidth, int textWidgetHeight, const char* textString, const char* textFontName, font::TColor color, int textWidgetId, unsigned justify, int back_color, int textWidgetStyle, unsigned char focusable);
// CODEVIEW(E:\gamedcs\textwdgt.cpp:102, dc 0x164d68) void textWidget::initialize(int x, int y, int w, int h, int id, int style, const char* _text, const char* _font, font::TColor _color, unsigned _justify, unsigned char focusable);
// CODEVIEW(E:\gamedcs\textwdgt.cpp:120, dc 0x164dd4) int textWidget::Main(message* msg);
// CODEVIEW(E:\gamedcs\TextWdgt.h:78, dc 0x1652f4) void textWidget::SetColor(font::TColor new_color);
// CODEVIEW(E:\gamedcs\textwdgt.cpp:42, dc 0x1652fc) void* textWidget::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_TEXTWDGT_H */
