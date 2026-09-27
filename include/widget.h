// widget.h - the basewin widget base (compiland widget.obj; homm2's
// BASE/widget.h is the template).
#ifndef HOMM3_WIDGET_H
#define HOMM3_WIDGET_H

#include "va.h"

class heroWindow;
class message;

// PROVEN size 48: the retail ctor 0x5fe340 stores every member and
// button::buttonIcon at 0x30 bounds the total. The tail diverges from
// the Dreamcast roster (focusable@0x20, on@0x24, RollOver@0x28,
// RightClick@0x2c, freeText@0x30): retail's dtor 0x5fe430 frees
// [0x24] then [0x20] under the byte flag [0x28], and set_help_text
// 0x5fe840 stores text/rclick/copyText to exactly those three - so
// retail is RollOver@0x20, RightClick@0x24, freeText@0x28, and one
// remaining dword lands at [0x2c]. DC's `focusable` and `on` have no
// retail home: retail is exactly DC minus those two, plus field_2C.

// Virtual roster PROVEN by the retail widget vtable 0x243c90 -
// THIRTEEN slots, not twelve (config/retail/vtables.tsv row 0x243c90;
// heroWindow's own vtable begins immediately after at 0x243cc4 =
// 0x243c90 + 13*4, which bounds the count exactly):
//   0  scalar deleting dtor 0x5fe3b0 (~widget 0x5fe430 inlined - the
//      homm2 `inline` dtor idiom)
//   1  Open 0x5fe4d0 - non-virtual on DC, virtualized in retail
//   2  Main, 3 zBufferDraw, 4 Draw - all _purecall (0x217d9a) in the
//      widget vtable, pure on DC too; widget::Main still has an
//      out-of-line body 0x5fe4f0 that derived Mains call explicitly
//   5  GetRealHeight 0x21d0 / 6 GetRealWidth 0x21e0 - header-inline
//      COMDATs landed in adventuremapwindow.obj (first referencing
//      unit)
//   7  process_hover 0x5fe930   8  Dim 0x5fe800
//   9  enable 0x5fe940
//   10 OnSetFocus / 11 OnKillFocus - both 0x4df0, the /OPT:ICF-folded
//      empty inline (DC order fixes which name is which)
//   12 onSleepChange 0x485d80 - RETAIL-ONLY (slots 0..11 reproduce the DC
//      roster order exactly, with Open inserted; DC's list ends at
//      OnKillFocus, so 12 is appended). One dword argument (`ret 4`)
//      and an empty body that ICF folded into the program-wide `ret 4`
//      representative at 0x485d80, so no claim may sit on it - the
//      widget::Close precedent. Overridden in exactly one place in the
//      whole image: button's three vtables (0x23bb54/0x23bb88/
//      0x23bbbc) point slot 12 at 0x456a10, whose entire body is an
//      explicit `widget::onSleepChange(arg)` call; the other 25
//      widget-family vtables inherit 0x485d80. The original name is unknown;
//      onSleepChange describes the first-sleep/final-wake calls in sleep().
class widget {
public:
    // Dreamcast widget::EStatusFlags, values byte-corroborated by the
    // retail ctor (status = WIDGET_ACTIVE | WIDGET_DRAWN) and enable
    // (WIDGET_DISABLED mask).
    enum EStatusFlags {
        WIDGET_STATUS_MASK = 0xFFFF,
        WIDGET_SELECTED = 1,
        WIDGET_ACTIVE = 2,
        WIDGET_DRAWN = 4,
        WIDGET_DIMMED = 8,
        WIDGET_HIGHLIGHTED = 16,
        WIDGET_DISABLED = 32,
        WIDGET_DIMMED_NODRAW = 4096,
        WIDGET_ASLEEP = 8192,
        WIDGET_UPDATE = 16384
    };
    // widget::style values; 0x1000 is homm2's WIDGET_KIND_AUTO_REPEAT
    // role (button::Main's repeat-timer head keys on it) - name
    // provisional.
    enum EStyles {
        WIDGET_STYLE_AUTO_REPEAT = 0x1000
    };
    // Dreamcast widget::EReturnCodes, verbatim - plus one retail-only
    // member. DoDialogDraw's pump switches the SAME codeX domain on a
    // fifth value 0x20 (WIDGET_END_DIALOG re-draws nothing and ends the
    // loop; 0x20 re-runs dialogDrawFunction and keeps pumping), so the
    // VALUE is byte-proven and its role is "ask the dialog to redraw" -
    // but DC's roster stops at 14 and no dump names it, so the spelling
    // below is an ORDINAL PLACEHOLDER (the field_NN / _vslotN house
    // convention), NOT an attested name. Rename it the moment a
    // producer's TU proves one.
    enum EReturnCodes {
        WIDGET_END_DIALOG = 10,
        WIDGET_SELECT = 12,
        WIDGET_DESELECT = 13,
        WIDGET_RIGHT_SELECT = 14,
        WIDGET_RETURN_32 = 0x20
    };
    // Dreamcast widget::ECommands, verbatim.
    enum ECommands {
        WIDGET_ACTIVATE = 1,
        WIDGET_DRAW = 2,
        WIDGET_SET_TEXT = 3,
        WIDGET_SET_ICON_FRAME = 4,
        WIDGET_SET_STATUS = 5,
        WIDGET_CLEAR_STATUS = 6,
        WIDGET_GET_TEXT = 7,
        WIDGET_SET_ICON_COLOR = 8,
        WIDGET_SET_COLOR = 8,
        WIDGET_SET_ICON_NAME = 9,
        WIDGET_SET_PALETTE = 10,
        WIDGET_SET_IMAGE = 11,
        WIDGET_SET_ICON_SEQUENCE = 12,
        WIDGET_SET_PLAYER_PALETTE_COLORS = 13,
        WIDGET_SET_SLIDER_STATE = 49,
        WIDGET_SET_SLIDER_RESOLUTION = 50,
        WIDGET_SET_TEXT_LEN = 51,
        WIDGET_SET_X = 52,
        WIDGET_SET_Y = 53,
        WIDGET_SET_ITEM = 54,
        WIDGET_GET_ITEM = 55,
        WIDGET_ADD_ITEM = 56,
        WIDGET_CHANGE_ITEM = 57,
        WIDGET_DELETE_ITEM = 58,
        WIDGET_DELETE_ALL_ITEMS = 59,
        WIDGET_SET_WIDTH = 61,
        WIDGET_SET_HEIGHT = 62,
        WIDGET_SET_COLORIZE = 63,
        WIDGET_SET_FOCUS = 64
    };
    // Retail body 0x5fe410 (dc 0x196bd4) - the default ctor really is
    // emitted; it is not an inlined-away static.
    widget();
    widget(short widgetX, short widgetY, short widgetWidth, short widgetHeight, short widgetId, short widgetStyle);
    // Keep the retail virtual slot order as one block. CodeView's header
    // bodies at 144/147 and 186/187 precede the text/status helpers below.
    virtual ~widget();  // slot 0
    void initialize(int x, int y, int w, int h, int id, int style);
    virtual int open(int newPriority, heroWindow* parent);  // slot 1
    // Non-virtual on DC and in retail: heroWindow::RemoveWidget calls
    // it DIRECTLY (0x5bc690 - a /Gy header-COMDAT the link kept from an
    // earlier obj, ICF-folded with other empty bodies). The ordinary
    // definition remains in widget.cpp.
    void close();
    // DC Main(message&) is shared by the widget overrides; retail passes
    // the same address through slot 2.
    virtual int main(message& msg) = 0;  // slot 2
    // The formal DC type supplies the two draw arguments even where
    // optimized parameter records are empty. The shared representative
    // at 0x5bc7e0 is `ret 8`, and
    // TCampaignBrief dispatches this slot with the z-buffer and widget id.
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const = 0;  // slot 3
    // Original Draw, zBufferDraw and Dim have const receivers in CodeView.
    // These hooks write to the destination bitmap through its pointer.
    virtual void draw() const = 0;  // slot 4
    VA(0x004021d0, 0x5) MAC_ADDRESS(0x0041b4, 0x8)  // vtable slot 5 + exact height read, retail-only
    virtual int getRealHeight() const { return m_height; }  // slot 5
    VA(0x004021e0, 0x5) MAC_ADDRESS(0x0041bc, 0x8)  // vtable slot 6 + exact width read, retail-only
    virtual int getRealWidth() const { return m_width; }  // slot 6
    virtual void processHover();  // slot 7
    virtual void dim() const;  // slot 8
    virtual void enable(unsigned char on);  // slot 9
    void setHelpText(const char* text, const char* rclick, unsigned char copyText);
    int sendMessage(widget::ECommands command, int extra);

    VA(0x00404df0, 0x1) MAC_ADDRESS(0x05ef60, 0x4)  // shared empty focus hook, vtable slots 10/11; dc 0x54d1c
    virtual void onSetFocus() {}  // slot 10
    virtual void onKillFocus() {}  // slot 11

    // Retail body 0x5fe410 (dc 0x196bd4) - the default ctor really is
    // emitted; it is not an inlined-away static.
    // DC Widget.h:225-226, dc 0x12859c: static hover reset.
    static void clearHoverWidget() { s_lastHoverWidget = 0; }

    // Dreamcast Widget.h:231. Retail callers reduce it to the +0x20
    // RollOver load, so no out-of-line body survives.
    const char* getHelpText() const { return m_rollOver; }
    // Dreamcast Widget.h:236 header inline. CampaignBriefHandler folds this
    // exact RightClick-or-RollOver choice into its retail body.
    const char* getRclickText() const
    {
        return m_rightClick ? m_rightClick : m_rollOver;
    }
    // DC-attested name (?sleep@widget@@QAAX_N@Z, E:\gamedcs\Widget.h:244)
    // on a RETAIL-ONLY body: DC's inline is the WIDGET_ASLEEP status-bit
    // send_message, retail's is the nest counter below. Header-inline
    // in both builds - retail's only call site is heroWindow's slot-8
    // body 0x5ff5f0, where /Ob2 expands it in full.
    void sleep(unsigned char on)
    {
        if (on) {
            if (m_sleepCount++ == 0)
                onSleepChange(1);
        } else {
            if (--m_sleepCount == 0)
                onSleepChange(0);
        }
    }
    // Dreamcast header inlines used by mode-switch paths.
    void hide()
    {
        sendMessage(WIDGET_CLEAR_STATUS, WIDGET_ACTIVE | WIDGET_DRAWN);
    }
    void show()
    {
        sendMessage(WIDGET_SET_STATUS, WIDGET_ACTIVE | WIDGET_DRAWN);
    }
    VA(0x005629b0, 0x22)  // hd-crossbuild; Widget.h:263, dc 0x56df8
    void setVisible(unsigned char arg)
    {
        if (arg)
            sendMessage(WIDGET_SET_STATUS, WIDGET_DRAWN);
        else
            sendMessage(WIDGET_CLEAR_STATUS, WIDGET_DRAWN);
    }

    // Original: widget::force_update; Widget.h:271, dc 0x56e20.
    // Bottom-view updates expand this same status message in Complete.
    void forceUpdate() { sendMessage(WIDGET_SET_STATUS, WIDGET_UPDATE); }

    // DC field-list order puts the data after the methods; CodeWarrior then
    // keeps the vptr at +0, as every Mac widget virtual call reads it.
public:
    heroWindow* m_parentWindow;
    widget* m_prevWidget;
    widget* m_nextWidget;
    short m_id;
    short m_priority;
    short m_style;
    short m_status;
    short m_x;
    short m_y;
    short m_width;
    short m_height;

protected:
    char* m_rollOver;
    char* m_rightClick;
    unsigned char m_freeText;

public:
    int m_sleepCount;

protected:
    // Dreamcast: protected static widget* last_hover_widget
    // (?last_hover_widget@widget@@1PAV1@A); retail .bss 0x6aac68,
    // cleared by the dtor when the dying widget is the hoveree.
    static widget* s_lastHoverWidget;

public:
    // Slot 12. The empty body lives in widget.cpp so button's qualified
    // base call stays out of line. Retail ICF folds it to 0x485d80.
    virtual void onSleepChange(int on);  // slot 12
};
SIZE(widget, 48);

#endif  /* HOMM3_WIDGET_H */
