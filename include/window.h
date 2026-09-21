#ifndef HOMM3_WINDOW_H
#define HOMM3_WINDOW_H

#include <vector>
#include "va.h"

class widget;
class textWidget;
class message;
class Bitmap16Bit;

// The retail table at 0x68c710 has 37 eight-byte rows.  SetWinText proves
// the byte/word/pointer field offsets (+0/+2/+4), while
// InitializeWinSetupText writes the final pointer column in seven source
// groups.  The names of the table and row type are Dreamcast publics/source
// evidence; the field spellings remain role names because no fieldlist was
// retained in the dump.
struct SWinSetup {
    unsigned char m_windowId;
    unsigned short m_widgetId;
    const char* m_text;
};
SIZE(SWinSetup, 8);

// One pooled empty rollover string shared by window::UpdateRollover and
// hero::initialize's custom-name string reset.
DATA(0x00691210) extern const char g_emptyRolloverText[];

// The rollover/right-click text pair CHeroWindowEx::SetHelpText hands
// to widget::set_help_text. Stride 8 is byte-proven by 0x5ff8e0's
// `lea edi,[8*eax]` row arithmetic and the +0/+4 field loads.
struct THelpText {
    const char* m_text;
    const char* m_rclick;
};

// heroWindow::type flag bits. FIXED_LAYER and SAVE_BACKGROUND carry
// homm2's WindowFlag names and values (byte-proven in Open/Close);
// 0x10 is retail-only - SaveBackground pads the grab by 8 pixels and
// Open draws the two-pass edge shadow under it - name provisional.
enum EWindowFlags {
    WINDOW_FLAG_FIXED_LAYER = 1,
    WINDOW_FLAG_SAVE_BACKGROUND = 2,
    WINDOW_FLAG_SHADOWED = 0x10
};

// homm2's WindowDrawId lineage, verbatim values.
enum EWindowDrawIds {
    WINDOW_ALL_WIDGETS_LOW = -0xffff,
    WINDOW_ALL_WIDGETS_HIGH = 0xffff
};

// homm2's WindowState lineage; Open/Close test and assign bit 1.
enum EWindowStates {
    WINDOW_STATE_OPEN = 1
};

// The 800x600 desktop (homm2's WindowConstant idiom at 640x480);
// names provisional.
enum EWindowMetrics {
    WINDOW_SCREEN_WIDTH = 800,
    WINDOW_SCREEN_HEIGHT = 600
};

// PROVEN layout (retail ctor 0x5fe9f0 stores every member; the DC
// fieldlist names them, offsets shifted only by the STLport->VC6
// vector width): priority@4, nextWindow@8, prevWindow@0xc, type@0x10,
// status@0x14, x@0x18, y@0x1c, width@0x20, height@0x24,
// headWidget@0x28, tailWidget@0x2c, Widgets@0x30 (VC6 vector, 16 B),
// focusId@0x40, background@0x44, field_48@0x48 - a retail-only sleep
// nesting counter (SleepAllWidgets increments/decrements it around
// virtual slot 8). Total 0x4c.

// Virtual roster BYTE-PROVEN by the retail heroWindow vtable 0x243cc4
// (config/retail/vtables.tsv: 9 slots; every slot's target is in
// config/retail/reloc-evidence.tsv 0x243cc4..0x243ce4):
//   0  sdd 0x5fea50 (~heroWindow 0x5fea80)   1  Open 0x5feae0
//   2  Close 0x5fec60                        3  handle_message 0x4ec560
//   4  handle_widget_hover 0x485d80 - the ICF-folded `ret 4`, so it
//      takes exactly one stack argument
//   5  DrawWindow 0x5ff020
//   6  0x5ff460 - masks ONE uchar argument and tail-calls
//      heroWindowManager::DoDialog(this, 0x5ff500, arg). DC's
//      DrawWindowX takes THREE arguments and has no retail row, so
//      slot 6 is DC's DoModal(unsigned char fadeIn) - the only
//      one-argument roster entry left in that bracket.
//   7  AddWidgetsToMessageStream 0x5ff570
//   8  0x5ff5f0 - retail-only, unsigned char arg, the per-widget twin
//      of SleepAllWidgets; reached only through SleepAllWidgets.
class heroWindow {
public:
    int m_priority;
    heroWindow* m_nextWindow;
    heroWindow* m_prevWindow;
    unsigned int m_type;
    int m_status;
    int m_x;
    int m_y;
    int m_width;
    int m_height;
    widget* m_headWidget;
    widget* m_tailWidget;
    std::vector<widget*> m_widgets;

protected:
    int m_focusId;

private:
    Bitmap16Bit* m_background;

public:
    int m_sleepCount;

    heroWindow(int winX, int winY, int winWidth, int winHeight, unsigned winType);
    void centerWindow(int centerX, int centerY);
    void moveWindow(int deltaX, int deltaY);
    void enableAllWidgets(unsigned char enable);
    void removeAndDeleteWidget(int id);
    int broadcastMessage(message& msg);
    int broadcastMessage(int id, int codeX, int codeY, int extra);
    int widgetSetStatus(int id, int status);
    int widgetClearStatus(int id, int status);
    widget* findWidgetPtr(int mx, int my) const;
    int findWidget(int mx, int my) const;
    void addWidget(widget* newWidget, int newPriority);
    void removeWidget(widget* killWidget);
    widget* getWidget(int id);
    void setFocus(int id);

protected:
    // DC: protected STATIC (no vfptr slot). /Gr makes it fastcall, which
    // is exactly the TDialogHandler shape DoModal hands to DoDialog.
    static int heroWindowHandler(message& msg);

private:
    int saveBackground();
    void restoreBackground(unsigned char update);

public:
    void sleepAllWidgets(unsigned char sleep);

    virtual ~heroWindow();
    virtual int open(int zOrder, unsigned char update);
    virtual void close(unsigned char update);         // slot 2, retail 0x5fec60
    virtual int handleMessage(message& msg);         // slot 3, folded onto 0x4ec560
    virtual void handleWidgetHover(widget* w);      // slot 4, folded onto 0x485d80
    virtual void drawWindow(unsigned char update, int lowID, int highID);
    // DC DoModal's UAAX_N public and all three overrides prove void(bool).
    // Retail callers discard EAX; the dispatcher's residual value is not
    // a returned dialog result. The virtual slot remains unchanged.
    virtual void doModal(bool fadeIn);

protected:
    void deleteWidgets();
    virtual void addWidgetsToMessageStream();

public:
    // Slot 8 is NOT pure - 0x5ff5f0 is a real heroWindow body in
    // window.obj's own band (reconstructed 2026-08-08, once widget's
    // 13th slot was modelled). It runs widget::sleep over the whole
    // Widgets vector, i.e. it is the window-wide half of the same
    // nest-counter/edge-hook pair SleepAllWidgets runs on field_48.
    virtual void vslot8(unsigned char on);           // slot 8, retail 0x5ff5f0, unidentified
};

// CHeroWindowEx - heroWindow plus a rollover latch. Layout PROVEN by
// the ctor 0x5ff640: it inlines heroWindow's ctor store-for-store,
// stores the DERIVED vtable 0x243ce8 at +0, and adds exactly one
// dword, [+0x4c] = -1. Total 0x50.

// Virtual roster BYTE-PROVEN by vtable 0x243ce8 (14 slots,
// config/retail/vtables.tsv; targets in reloc-evidence.tsv
// 0x243ce8..0x243d1c): slots 0-8 are heroWindow's, with slot 0 the
// class's own sdd 0x5ff6b0 and slot 3 overridden at 0x405680; then
//   9   WindowHandler       0x5ff820  (ret 4 - one message*)
//   10  ProcessHover        0x5ff6e0  (ret 8 - findWidgetPtr(x, y))
//   11  ProcessRightSelect  0x5ff790  (ret 4 - one widget id)
//   12  OnWidgetDeselect    0x559140  (`xor eax,eax; ret 8` - the
//       ordinary window.cpp body, ICF-folded with another retail owner)
//   13  GetRolloverWidget   0x5ff8d0  (`xor eax,eax; ret`)
// This CORRECTS the previous 1:1 DC-order mapping of the seven retail
// rows onto the seven DC roster entries: the sdd sits second (the
// widget/heroWindow placement), OnWidgetDeselect has no distinct retail
// row here, and the three claims in between were each one slot low.
class CHeroWindowEx : public heroWindow {
public:
    int m_rolloverId;   // +0x4c, -1 in the ctor, latched by ProcessHover

    CHeroWindowEx(int winX, int winY, int winWidth, int winHeight, unsigned winType);

    VA(0x00405680, 0x10)  // shared slot-3 header forwarder, dc 0x2dcc
    virtual int handleMessage(message& msg)
    {
        return windowHandler(msg);
    }
    virtual int windowHandler(message& msg);                            // slot 9
    virtual unsigned char processHover(int mouseX, int mouseY);         // slot 10
    virtual unsigned char processRightSelect(int id);                   // slot 11
    void setHelpText(THelpText* helpText, int start, int stop, unsigned char copyText);

protected:
    virtual int onWidgetDeselect(int id, bool& exitFlag);  // slot 12
    virtual textWidget* getRolloverWidget();                            // slot 13
};

unsigned char initializeWinSetupText();
void setWinText(heroWindow* win, int winId);

#endif  /* HOMM3_WINDOW_H */
