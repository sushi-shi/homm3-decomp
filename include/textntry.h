// textntry.h - textntry.cpp (compiland textntry.obj)
#ifndef HOMM3_TEXTNTRY_H
#define HOMM3_TEXTNTRY_H

#include "textwdgt.h"

class Bitmap816;

// The snapshot implementation is local to textntry.cpp.
class CTextEntrySave;

// textEntryWidget derives from textWidget in retail (the dtor calls
// ~textWidget as its base) - Dreamcast agrees. Vtable 0x642d40, 19
// slots: 0..12 widget's, 13 textWidget::SetText, and the five this
// class introduces - SetFocus(14), OnKeyPress(15), IgnoreKey(16),
// SetAutoDraw(17), SaveBackground(18). The slot->body assignment is
// forced from both ends: the retail vtable's own contents, and the
// exhaustive source-line order-map of the ten carve rows 0x5bac50..
// 0x5bbac0 onto textntry.cpp's DC roster lines 213..666.

// The 0x58..0x6e tail is read out of those ten bodies. Every field is
// a 16-bit slot except the three trailing flags; 0x64/0x66/0x68 keep
// house ordinal placeholders because no source we may read names them.
class textEntryWidget : public textWidget {
public:
    // The domain of field_68 below. Only the one value the retail
    // bodies branch on is recoverable: at 3, Draw renders a
    // horizontally scrolled window of the string (substr from
    // displayStart, truncated to boxWidth) and SetupDisplayString
    // maintains displayStart; every other value renders the whole
    // string. Nothing in the image writes the field, so the domain
    // cannot be enumerated further and this spelling is a house
    // placeholder describing the branch's effect, not an attested name.
    enum EField68 {
        FIELD_68_SCROLLED = 3
    };
    // The constructor's readType domain. Only one value is
    // recoverable - the one both retail call sites (armygrp.obj's two
    // split-count entries) pass and the only one the constructor
    // branches on: at 4 the text box is inset by (insetX, insetY) on
    // every side and field_66 latches 1. Placeholder spelling, same
    // rule as above.
    enum EReadType {
        READ_TYPE_INSET = 4
    };
    Bitmap816* m_textBack;  // 0x50, ResourceManager::GetBitmap816
    CTextEntrySave* m_saveBack;  // 0x54
    unsigned short m_cursorIndex;  // 0x58, = Text.size() after every edit
    unsigned short m_maxLength;  // 0x5a, the ctor's textStringSize
    short m_boxWidth;  // 0x5c, the inset text box
    short m_boxHeight;  // 0x5e
    short m_boxX;  // 0x60
    short m_boxY;  // 0x62
    short m_textLines;  // 0x64, ctor stores 1. OnKeyPress
                                 // compares it against
                                 // Font->LineLength(Text, boxWidth) and
                                 // rolls the edit back when the typed
                                 // character pushes the string past it,
                                 // i.e. a line-count ceiling. Still an
                                 // ordinal placeholder: 1 is the only
                                 // value attested and only the ctor
                                 // writes it.
    short m_attributes;  // 0x66, ctor stores the inset flag
    short m_type;  // 0x68, compared against 3 by Draw /
                                 // SetupDisplayString / OnKeyPress. NO
                                 // retail body anywhere in the image
                                 // writes it - scanned every 8/16/32-bit
                                 // store form at this displacement.
    short m_displayStart;  // 0x6a, first shown character
    unsigned char m_cursorFlashOn;  // 0x6c, the caret blink phase: OnKeyPress
                                 // forces it to 1 on every keystroke and
                                 // SetupDisplayString toggles it
                                 // (`1 - field_6C`) every 360 ticks off
                                 // glTimers[0]. Nothing in the image
                                 // READS it in this class; Dreamcast
                                 // independently supplies its name.
    unsigned char m_hasFocus;  // 0x6d, stored by SetFocus 0x5bab50
    unsigned char m_autoDraw;  // 0x6e, gates SetFocus's redraw
    // Dreamcast ends the 0x70-byte editor with autoDraw at +0x6e.
    // NH3API confirms that the last byte is alignment in the PC object.
    char m_paddingAfterAutoDraw[1];
    textEntryWidget(int x, int y, int w, int h, int textSize,
                    const char* text, const char* fontName,
                    font::TColor color, unsigned justification,
                    const char* backgroundIcon, int backgroundFrame, int id,
                    int style, int readType, int insetX, int insetY);
    virtual ~textEntryWidget();
    virtual int main(message& msg);
    virtual void draw() const;
    void setupDisplayString(char* core, unsigned short inCursorIndex);
    char getCharPressed(message* msg);
    virtual void onSetFocus();
    virtual void onKillFocus();
    virtual void setText(const char* newText);
    virtual void setFocus(unsigned char state);
    virtual int onKeyPress(message* msg);
    virtual unsigned char ignoreKey(message* msg);
    virtual void setAutoDraw(unsigned char b);

protected:
    virtual void saveBackground() const;  // slot 18, retail 0x5bba70
};
// No SIZE() assert: the class rides std::string, whose extent differs
// between the VC6 arm (0x10, giving textWidget 0x50 and this 0x70) and
// the clang editor arm.

#endif  /* HOMM3_TEXTNTRY_H */
