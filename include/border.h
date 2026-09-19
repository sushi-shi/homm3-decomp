// border.h - prototypes of border.cpp (compiland border.obj)
#ifndef HOMM3_BORDER_H
#define HOMM3_BORDER_H

#include "widget.h"

// Head model only: the retail dtor 0x44ff50 is an empty body (vtable
// store + tail-jump to ~widget), so no border members are proven yet.
// Vtable 0x63ba24.
class border : public widget {
public:
    border(int x, int y, int w, int h, int id, int style);
    border();
    void initialize(int x, int y, int w, int h, int id, int style,
                    unsigned char focusable = 0);
    virtual int main(message& msg);  // slot 2, retail 0x44ff60
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw() const;  // slot 4
    // Slot 13, appended past widget's twelve-plus-_vslot12 exactly as
    // iconWidget appends its own twin (see iconwdgt.h). Main dispatches
    // it through `call [vptr+0x34]`, i.e. 13*4, which is what fixes the
    // index; the 4-byte `return 0` body ICF-folded onto iconWidget's.
    virtual bool handleClick(bool downClick,
                                       bool rightClick);
    virtual ~border();  // retail 0x44ff50
};

// DC's simple filled border has no admitted standalone Complete identity.
// Preserve its ordinary source interface without claiming a retail vtable.
class coloredBorder : public border {
public:
    int m_color;  // DC color; immediately after the platform's widget base.
    coloredBorder(int x, int y, int w, int h, int id, int color, int style);
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw() const;
};

// Complete vtable 0x63ba5c belongs to the frame variant; its Main handles
// color/colorize messages, and its constructor proves the two member offsets.
class coloredBorderFrame : public border {
public:
    // Retail's constructor at 0x450130 stores these directly after the
    // 0x30-byte widget/border head.  Dreamcast gives the same member names.
    int m_color;
    unsigned char m_colorize;
    // Dreamcast ends this class with the one-byte colorize flag and
    // three alignment bytes. Retail constructor 0x450130 preserves that tail
    // after its smaller base, ending at 0x38.
    char m_paddingAfterColorize[3];
    coloredBorderFrame(int x, int y, int w, int h, int id,
                       int color, int style);
    // Implicit destructor; CodeView dc 0x54dd8 compgenx.
    virtual int main(message& msg);
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw() const;  // slot 4, retail 0x4501e0
};
SIZE(coloredBorderFrame, 0x38);

class Bitmap816;

// The image member sits at +0x30 (first past widget), byte-proven by
// SetImage 0x4504c0 (name strcmp at image+4, Dispose vcall, reload
// through ResourceManager::GetBitmap816).
class bitmapBorder : public border {
public:
    Bitmap816* m_image;
    bitmapBorder(int x, int y, int w, int h, int id,
                 const char* image, int style);
    virtual ~bitmapBorder();
    virtual void draw() const;
    virtual int getRealHeight() const;
    virtual int getRealWidth() const;
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual int main(message& msg);
    void setImage(const char* bitmapName);
    void setPalette(const char* paletteName);
    void setPlayerPaletteColors(int whichPlayer);
};

class Bitmap16Bit;

// The 16-bit variant; image at +0x30 byte-proven by the dtor's
// Dispose vcall (0x450750).
class bitmapBorder16 : public border {
public:
    Bitmap16Bit* m_image;
    bitmapBorder16(int x, int y, int w, int h, int id,
                   const char* image, int style);
    virtual ~bitmapBorder16();
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw() const;
    void draw2() const;
    virtual int getRealWidth() const;
    virtual int getRealHeight() const;
    virtual int main(message& msg);  // slot 2, retail 0x450860
    // DC dc 0x54c6c. Retail has NO row for it: Main below is its only call
    // site, /Ob2 expanded it there and /OPT:REF then dropped the orphaned
    // COMDAT. Its inlined `return` is what gives Main retail's single
    // `return 1` tail rather than a duplicated epilogue.
    void setImage(const char* bitmapName);
};

// The free palette painters (declared for button.cpp in button.h;
// re-declared here for the border family).
class palette;
class paletteHiColor;
class TPalette24;
void setPlayerPaletteColors(unsigned short* pal, int whichPlayer);
void setPlayerPaletteColors(paletteHiColor* pal, int whichPlayer);
void setPlayerPaletteColors(TPalette24& pal, int whichPlayer);

// --- bitmapBorder ---
// CODEVIEW(E:\gamedcs\border.cpp:323, dc 0x54988) void bitmapBorder::SetPalette(const char* palette_name);
// CODEVIEW(E:\gamedcs\border.cpp:290, dc 0x54df0) void* bitmapBorder::`scalar deleting destructor'(unsigned __flags);

// --- bitmapBorder16 ---
// CODEVIEW(E:\gamedcs\border.cpp:415, dc 0x54ba4) void bitmapBorder16::zBufferDraw();
// CODEVIEW(E:\gamedcs\border.cpp:431, dc 0x54c2c) int bitmapBorder16::GetRealWidth();
// CODEVIEW(E:\gamedcs\border.cpp:436, dc 0x54c4c) int bitmapBorder16::GetRealHeight();
// CODEVIEW(E:\gamedcs\border.cpp:441, dc 0x54c6c) void bitmapBorder16::SetImage(const char* bitmap_name);
// CODEVIEW(E:\gamedcs\border.cpp:404, dc 0x54e24) void* bitmapBorder16::`scalar deleting destructor'(unsigned __flags);

// --- border ---
// CODEVIEW(E:\gamedcs\border.cpp:34, dc 0x5433c) void border::border();
// CODEVIEW(E:\gamedcs\border.cpp:67, dc 0x54408) void border::initialize(int x, int y, int w, int h, int id, int style, unsigned char focusable);
// CODEVIEW(E:\gamedcs\border.cpp:153, dc 0x54590) unsigned char border::handle_click(unsigned char down_click, unsigned char right_click);
// CODEVIEW(E:\gamedcs\border.cpp:158, dc 0x54594) void border::zBufferDraw();
// CODEVIEW(E:\gamedcs\border.cpp:161, dc 0x54598) void border::Draw();
// CODEVIEW(E:\gamedcs\border.cpp:35, dc 0x54d24) void* border::`scalar deleting destructor'(unsigned __flags);

// --- coloredBorder ---
// CODEVIEW(E:\gamedcs\border.cpp:174, dc 0x5459c) void coloredBorder::coloredBorder(int x, int y, int w, int h, int id, int color_, int style);
// CODEVIEW(E:\gamedcs\border.cpp:182, dc 0x54614) void coloredBorder::zBufferDraw();
// CODEVIEW(E:\gamedcs\border.cpp:185, dc 0x54618) void coloredBorder::Draw();
// CODEVIEW(E:\gamedcs\border.cpp:178, dc 0x54d58) void* coloredBorder::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\border.cpp:178, dc 0x54d8c) void coloredBorder::~coloredBorder();

// --- coloredBorderFrame ---
// CODEVIEW(E:\gamedcs\border.cpp:210, dc 0x546d0) void coloredBorderFrame::zBufferDraw();
// CODEVIEW(E:\gamedcs\border.cpp:206, dc 0x54da4) void* coloredBorderFrame::`scalar deleting destructor'(unsigned __flags);

// --- widget ---
// CODEVIEW(E:\gamedcs\Widget.h:187, dc 0x54d20) void widget::OnKillFocus();

#endif  /* HOMM3_BORDER_H */
