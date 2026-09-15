// campaign.cpp - the Complete-only campaign-set chooser: the five "CSS"
// plates (Shadow of Death, Armageddon's Blade, Restoration of Erathia, the
// custom-campaign chooser and Exit) that kb.cpp's DoCampaignWindow opens
// before any campaign page.

// THIS COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER, and its NAME is an
// inference the link order bounds rather than proves (the netmsg.obj
// precedent). Retail's .text is laid out in strict alphabetical compiland
// order, so this object's name sorts strictly between `button` and
// `campaignbrief`: button.obj ends at 0x456e94 and campaignbrief.obj opens
// at 0x457990. `campaign` is the shortest spelling in that interval and the
// one the class's role suggests; treat it as provisional.

// The compiland's whole .text contribution is the ten bodies below,
// bracketed by two static-initializer runs of exactly the shape netmsg.obj
// carries - a 32-byte guard-byte row at 0x456ea0 opening the object, and
// 32/89/96/97 plus seven ~95-byte bitset initializers at 0x4575a0..0x45798f
// closing it. Those are the excluded initializer class and are not claimed.
#include <va.h>

#include "campaign.h"

#include "button.h"
#include "kb.h"
#include "message.h"
#include "smackmgr.h"
#include "soundmgr.h"
#include "textresource.h"
#include "widget.h"
#include "window.h"
#include "winmgr.h"

static int campaignSetSodHandler(Message& msg);
static int campaignSetArmHandler(Message& msg);
static int campaignSetCusHandler(Message& msg);
static int campaignSetExitHandler(Message& msg);
static int campaignSetRoeHandler(Message& msg);

// The rollover latch the handler keeps between messages: the widget id the
// mouse was last over, -1 for none. Retail .data 0x660dc8, the one dword in
// this compiland's own data band ahead of its string pool, written only by
// the handler below.
DATA(0x00660dc8)
static int g_campaignSetHoverId = -1;

// The three plate rectangles retail reads out of .rdata rather than folding.
// SoD's and Armageddon's Blade's are immediates in the constructor's own
// push run; these three are `movsx`-loaded word by word from 0x63bbf0,
// which is what fixes them as const-array elements and the element type as
// `short`. Layout order in .rdata is source order (roe, cus, exit), and the
// run ends exactly on this class's vtable at 0x63bc08. Names INVENTED.
DATA(0x0063bbf0) static const short g_campaignSetRoeRect[4] = { 494, 116, 287, 130 };
DATA(0x0063bbf8) static const short g_campaignSetCusRect[4] = { 554, 358, 169, 110 };
DATA(0x0063bc00) static const short g_campaignSetExitRect[4] = { 576, 464, 126, 108 };

// Retail 0x456ec0, the compiland's first real body. Five plates on an
// 800x600 heroWindow, each a type_func_button carrying its own callback and
// its own hotkey; the Armageddon's Blade plate exists only in
// video-game-state 3, so it also shifts every later widget id down by one
// when it is absent. `Widgets.reserve(4)` is retail's - one short of the
// five plates, which is why the vector still grows.

// Residual (99.9458%): ONE BYTE, the `sub esp` immediate - retail reserves
// 0xc and we reserve 8. Both sides use the same 26 temporary slots and the
// same eleven EH-state transitions; retail's allocator ALTERNATES its two
// four-byte temporaries between [ebp-0x10] (14 uses) and [ebp-0x14] (12),
// spilling `this` into a third slot at [ebp-0x18], while ours coalesces the
// pair onto [ebp-0x10] (26 uses) and puts `this` at [ebp-0x14]. Every
// displacement is masked by the comparison, so the frame size is the whole
// reported residual. This score therefore understates the differing frame
// operands; raw instructions must also agree before this body is closed.
// Pointer-lifetime control: 66 states varied all five actual plate pointers
// between button and type_func_button, separate block lifetimes, and one
// shared pointer. Three emitted objects and all retained candidates reproduce;
// none exceeds 99.9458%, and no exact sibling changes. These source forms do
// not recover the extra frame slot; no pointer or scope change is retained.
// Container-entry control: all 243 combinations of implicit widget* conversion,
// insertion-scoped widget* locals and enclosing-block const pointer locals
// preserve all nine exact siblings. The best reproduced candidates reach
// 99.9639% and allocate retail's frame, but raw review of all 243 still finds
// at least ten wrong temporary-slot operands. A named ROE entry alone is
// enough to reach that plateau; it does not explain retail's alternating
// allocation/reserve, widget-entry and hotkey homes. No such local is retained.
VA(0x00456ec0, 0x337)  // anchor-string CSSsod.def + anchor-vtable 0x63bc08 + DoCampaignWindow's stack object, retail-only
CampaignSetWindow::CampaignSetWindow()
    : HeroWindow(0, 0, 800, 600, 0)
{
    m_widgets.reserve(4);

    int widgetId = SOD_PLATE_ID;

    FuncButton* sodPlate = new FuncButton(
        534, 8, 201, 119, widgetId++,
        DATA_COMPGEN(0x00660dfc, campaignSetSodSprite, "CSSsod.def"),
        campaignSetSodHandler, 0, 1);
    m_widgets.push_back(sodPlate);
    sodPlate->setHotkey(0x1f);

    if (*g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH) {
        FuncButton* armPlate = new FuncButton(
            486, 242, 305, 119, widgetId++,
            DATA_COMPGEN(0x00660df0, campaignSetArmSprite, "CSSarm.def"),
            campaignSetArmHandler, 0, 1);
        m_widgets.push_back(armPlate);
        armPlate->setHotkey(0x1e);
    }

    FuncButton* roePlate = new FuncButton(
        g_campaignSetRoeRect[0], g_campaignSetRoeRect[1],
        g_campaignSetRoeRect[2], g_campaignSetRoeRect[3], widgetId++,
        DATA_COMPGEN(0x00660de4, campaignSetRoeSprite, "CSSroe.def"),
        campaignSetRoeHandler, 0, 1);
    m_widgets.push_back(roePlate);
    roePlate->setHotkey(0x13);

    FuncButton* cusPlate = new FuncButton(
        g_campaignSetCusRect[0], g_campaignSetCusRect[1],
        g_campaignSetCusRect[2], g_campaignSetCusRect[3], widgetId++,
        DATA_COMPGEN(0x00660dd8, campaignSetCusSprite, "CSScus.def"),
        campaignSetCusHandler, 0, 1);
    m_widgets.push_back(cusPlate);
    cusPlate->setHotkey(0x2e);

    FuncButton* exitPlate = new FuncButton(
        g_campaignSetExitRect[0], g_campaignSetExitRect[1],
        g_campaignSetExitRect[2], g_campaignSetExitRect[3], widgetId,
        DATA_COMPGEN(0x00660dcc, campaignSetExitSprite, "CSSexit.def"),
        campaignSetExitHandler, 0, 1);
    m_widgets.push_back(exitPlate);
    exitPlate->setHotkey(1);

    addWidgetsToMessageStream();
}

VA_COMPGEN(0x00457200, 0x21, SCALAR_DELETING_DTOR, CampaignSetWindow)

VA(0x00457230, 0x4E)
CampaignSetWindow::~CampaignSetWindow()
{
    deleteWidgets();
}

// The five plate callbacks. Each answers a right-click with its own help
// row and a left-release with the modal result DoCampaignWindow switches
// on; the Exit plate answers the shared 0x7801 dialog-cancel id.
VA(0x00457280, 0x64)
static int campaignSetSodHandler(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(g_generalText->getText(CampaignSetWindow::CAMPAIGN_SET_SOD_HELP), 4, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.m_codeX == Widget::WIDGET_DESELECT && !(msg.m_qualifier & 0x200)) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = CampaignSetWindow::CAMPAIGN_SET_SOD_ID;
        msg.m_codeX = Widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x004572f0, 0x64)
static int campaignSetArmHandler(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(g_generalText->getText(CampaignSetWindow::CAMPAIGN_SET_ARM_HELP), 4, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.m_codeX == Widget::WIDGET_DESELECT && !(msg.m_qualifier & 0x200)) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = CampaignSetWindow::CAMPAIGN_SET_AB_ID;
        msg.m_codeX = Widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x00457360, 0x64)
static int campaignSetCusHandler(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(g_generalText->getText(CampaignSetWindow::CAMPAIGN_SET_CUS_HELP), 4, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.m_codeX == Widget::WIDGET_DESELECT && !(msg.m_qualifier & 0x200)) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = CampaignSetWindow::CUSTOM_CAMPAIGN_ID;
        msg.m_codeX = Widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x004573d0, 0x64)
static int campaignSetExitHandler(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(g_generalText->getText(CampaignSetWindow::CAMPAIGN_SET_EXIT_HELP), 4, -1,
                     -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.m_codeX == Widget::WIDGET_DESELECT && !(msg.m_qualifier & 0x200)) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = DIALOG_RETURN_CANCEL;
        msg.m_codeX = Widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x00457440, 0x60)
static int campaignSetRoeHandler(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(g_generalText->getText(CampaignSetWindow::CAMPAIGN_SET_ROE_HELP), 4, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.m_codeX == Widget::WIDGET_DESELECT && !(msg.m_qualifier & 0x200)) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = CampaignSetWindow::CAMPAIGN_SET_ROE_ID;
        msg.m_codeX = Widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x004574a0, 0x2C)
void CampaignSetWindow::doModal()
{
    g_soundManager->startMP3(
        DATA_COMPGEN(0x00660e08, campaignSetMusic, "MainMenu"), 0, 1);
    g_windowManager->doDialog(this, heroWindowHandler, 0);
}

VA(0x004574d0, 0xC4)
int CampaignSetWindow::handleMessage(Message& msg)
{
    unsigned char hoverChanged = 0;

    pollSound();

    if (msg.m_id == MESSAGE_MOUSE_MOVE) {
        int hovered = findWidget(msg.m_mouseX, msg.m_mouseY);
        if (hovered != g_campaignSetHoverId) {
            hoverChanged = 1;
            if (g_campaignSetHoverId != -1)
                getWidget(g_campaignSetHoverId)
                    ->sendMessage(Widget::WIDGET_CLEAR_STATUS, 0x10);
            if (hovered != -1)
                getWidget(hovered)->sendMessage(Widget::WIDGET_SET_STATUS,
                                                 0x10);
            g_campaignSetHoverId = hovered;
        }
    }

    if (videoNeedsUpdate() || hoverChanged) {
        int lastPlate = LAST_PLATE_ID;
        if (*g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH)
            lastPlate = LAST_PLATE_WITH_AB_ID;
        drawWindow(0, SOD_PLATE_ID, lastPlate);
        g_windowManager->updateScreen(482, 9, 308, 562);
        videoDrawRects();
    }

    return MESSAGE_DISPATCH_CONSUME;
}
