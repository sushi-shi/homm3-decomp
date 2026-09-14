// button.h - the widget/button family (basewin lineage; homm2's
// widget/button are the templates).
#ifndef HOMM3_BUTTON_H
#define HOMM3_BUTTON_H

#include <va.h>
#include <string>
#include <string.h>
#include <vector>
#include "widget.h"
#include "csprite.h"
#include "resource.h"
#include "font.h"

// Player-color palette targets. Both overloads of the free
// SetPlayerPaletteColors (0x5ffe20 / 0x5ffe40) copy a per-player run
// out of a global table; the two differ in stride and destination
// offset (16 dwords -> +0x1c0 vs 24 dwords -> +0x2bc), which is what
// proves they are distinct types. Opaque here - only the pointers
// cross this TU. Type NAMES are provisional (no DC/NH3API evidence).
class palette;
class paletteHiColor;
class Palette24;
class sample;

void setPlayerPaletteColors(unsigned short* pal, int whichPlayer);
void setPlayerPaletteColors(paletteHiColor* pal, int whichPlayer);
void setPlayerPaletteColors(Palette24& pal, int whichPlayer);

// Layout PROVEN by the retail ctor 0x455ef0 (member stores) and dtor
// 0x4560f0 (member teardown): buttonIcon@0x30, normalFrame@0x34,
// selectedFrame@0x38, disabled_frame@0x3c (ctor seeds 2), field_40@0x40
// (ctor seeds 3 - retail-only, absent from the DC roster), endDialog@
// 0x44, hotKeyCodes@0x48 (VC6 std::vector<int>, 16 B), Text@0x58 (VC6
// std::string, 16 B; the dtor shows the native refcounted-string
// teardown, so retail uses VC6's own STL, not DC's STLport). Total 104.
// No SIZE assert: the clang arm's host STL sizes differ.
class button : public widget {
public:
    // DC textButton::Draw (0x57b98, button.cpp:538) directly passes
    // button::Text at +84 to string::c_str; its field record is private.
    // Retail 0x456ca0 does the same at +0x58: this specific derived class
    // therefore needs friendship, with no intervening text accessor.
    friend class textButton;

private:
    CSprite* m_buttonIcon;
    int m_normalFrame;
    int m_selectedFrame;
    int m_disabledFrame;

public:
    int m_highlightedFrame;

private:
    unsigned char m_endDialog;
    std::vector<int> m_hotKeyCodes;
    std::string m_text;

public:
    // homm2 BUTTON.cpp's REPEAT_DELAY_TICKS, verbatim value.
    enum EButtonConstants {
        BUTTON_REPEAT_DELAY_TICKS = 60
    };
    button();
    void initialize(int x, int y, int w, int h, int id, const char* image, int normal, int selected, unsigned char end, int hotkey, int style);
    // Dreamcast ?click_sample@button@@2PAVsample@@A; retail .bss
    // 0x694da4 (defined in button.cpp).
    static sample* s_clickSample;
    void setPalette(const char* paletteName);
    button(int x, int y, int w, int h, int id, const char* image, int normal, int selected, unsigned char end, int hotkey, int style);
    int select(message& msg);
    // E:\gamedcs\button.cpp:401, dc 0x57854
    int deselect(message& msg);
    // Dreamcast homes SetText and set_hotkey in Button.h itself; the
    // wrapper is inlined at its retail call sites. The old 0x404200 mapping
    // was disproven by that body's `ret 0xc`: it is the three-argument
    // vector<int>::insert implementation, not this one-argument member.
    void setText(const char* newText) { m_text = newText; }
    // Dreamcast button.h:99 (dc 0x669f4, 6 B SH4: one store). A free
    // /Ob2 candidate site wherever a caller uses it - see
    // TSingleSelectionWindow::CreateFilterWidgets, whose insert-expansion
    // sequence is reproduced only with this setter in its six loops.
    void setDisabledFrame(long frame) { m_disabledFrame = frame; }
    VA(0x004e1370, 0x1AF)
    void setHotkey(int code)
    {
        m_hotKeyCodes.push_back(code);
    }
    // Dreamcast button.h:120-122: the separate vector<int>::clear wrapper.
    // TAdvMenu::SetSleepImage retains this call in its source line table.
    void clearHotkeys() { m_hotKeyCodes.clear(); }
    virtual int main(message& msg);  // slot 2, retail 0x456190

    virtual int getRealWidth() const;  // slot 6, folded retail 0x4eab20
    virtual int getRealHeight() const; // slot 5, folded retail 0x4eab30
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const; // slot 3
    virtual void draw() const;  // slot 4, retail 0x456940
    virtual void dim() const;

    virtual ~button();
    // widget slot 12, overridden at 0x456a10 - the only override of it
    // in the image. Placeholder name inherited from widget.h.
    virtual void vslot12(int on);
    void setPlayerPaletteColors(int whichPlayer);
};

// Dreamcast roster: Font@96, textColor@100 (font::TColor) - retail
// button is 104, so Font@0x68, textColor@0x6c (the dtor Disposes
// [this+0x68]). Total 112.
class textButton : public button {
public:
    textButton(int x, int y, int w, int h, int id, const char* image, const char* text, const char* fontName, int normal, int selected, unsigned char end, int hotkey, int style, font::Color newColor);

    virtual void draw() const;    // slot 4, retail 0x456ca0

    virtual ~textButton();  // retail 0x456bf0

private:
    font* m_font;
    font::Color m_textColor;
};

// DC gives only a forward ref. The dtor (retail 0x456db0) tears down
// exactly button's members, so the tail is POD; Main proves handler is
// a fastcall message handler at 0x68 (call [this+0x68] with ecx=msg).
class type_func_button : public button {
public:
    typedef int (*handler_type)(message& msg);
    handler_type m_handler;
    type_func_button(long x, long y, long w, long h, long id,
                     const char* image, handler_type newHandler,
                     int normal, int selected);
    virtual int main(message& msg);  // slot 2, retail 0x456e50

    virtual ~type_func_button();  // retail 0x456db0
};

#endif  /* HOMM3_BUTTON_H */
