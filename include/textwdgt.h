#ifndef HOMM3_TEXTWDGT_H
#define HOMM3_TEXTWDGT_H

#include <string>
#include <vector>

#include "font.h"
#include "slider.h"
#include "widget.h"

// Dreamcast roster with the STLport->VC6 string shift: Text@0x30
// (16 B), Font@0x40, Color@0x44, BackColor@0x48, Justify@0x4c - total
// 0x50. Vtable 0x642db0; the dtor Disposes the font, the string
// teardown is implicit.
class textWidget : public widget {
public:
    std::string m_text;
    font* m_font;
    font::TColor m_color;
    int m_backColor;
    unsigned int m_justify;
    textWidget();
    textWidget(int x, int y, int w, int h, const char* text,
               const char* fontName, font::TColor color, int id,
               unsigned justify, int backColor, int style);
    textWidget(int x, int y, int w, int h, const char* text,
               const char* fontName, font::TColor color, int id,
               unsigned justify, int backColor, unsigned char focusable);
    void initialize(int x, int y, int w, int h, int id, int style,
                    const char* text, const char* fontName, font::TColor color,
                    unsigned int justify, unsigned char focusable);
    virtual ~textWidget();  // retail 0x5bc3b0
    virtual int main(message& msg);
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw() const;
    // Dreamcast textwdgt.cpp:257: empty Dim overrides widget dimming.
    virtual void dim() const;
    VA(0x0057C6D0, 0xAC) MAC_ADDRESS(0x005bcc, 0x4c)  // textWidget vtable slot 13 + DC header COMDAT, dc 0x1473f8
    virtual void setText(const char* newText) { m_text = newText; }
    // E:\gamedcs\TextWdgt.h:67; DC emits this header helper out of line,
    // while Complete folds the c_str() access into its callers.
    // Class-inline as in TextWdgt.h:67. Removing the unsupported forceinline
    // qualifier is byte-neutral across the affected widget/name-edit callers.
    const char* getText() { return m_text.c_str(); }
    // E:\gamedcs\TextWdgt.h:78, dc 0x1652f4
    void setColor(font::TColor newColor) { m_color = newColor; }
};

class CSprite;
// DC's sprite-backed label. Its Background and BackgroundFrame members are
// borrowed drawing inputs; the generated destructor tears down textWidget.
// No standalone Complete address or vtable is asserted for this class.
class iconBackedTextWidget : public textWidget {
public:
    iconBackedTextWidget();
    iconBackedTextWidget(int x, int y, int w, int h, const char* text,
                         const char* fontName, const char* backName,
                         font::TColor color, int id, unsigned int justify,
                         int style);
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw() const;
private:
    CSprite* m_background;
    int m_backgroundFrame;
};

class Bitmap816;
class Bitmap16Bit;

// Retail dtor 0x5bc6d0 is the empty derived dtor: the inlined
// ~textWidget body under this class's vtable store, then ~widget.
// It does NOT free the backing bitmap - that resource is borrowed.
class bitmapBackedTextWidget : public textWidget {
public:
    // +0x50: the 11-argument constructor 0x5bc760 stores GetBitmap816's
    // result here, and Draw 0x5bc7f0 blits out of it after clamping the
    // widget extent against its +0x24/+0x28 Width/Height.
    Bitmap816* m_image;
    bitmapBackedTextWidget();
    bitmapBackedTextWidget(int x, int y, int w, int h, const char* text,
                           const char* fontName, const char* backName,
                           font::TColor color, int id, unsigned justify,
                           int style);
    // Implicit destructor; CodeView dc 0x1653b0 compgenx.
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw() const;  // slot 4, retail 0x5bc7f0
};

#endif  /* HOMM3_TEXTWDGT_H */
