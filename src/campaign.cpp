// campaign.cpp - the Complete-only campaign-set chooser: the five "CSS"
// plates (Shadow of Death, Armageddon's Blade, Restoration of Erathia, the
// custom-campaign chooser and Exit) that kb.cpp's DoCampaignWindow opens
// before any campaign page.
//
// THIS COMPILAND IS ABSENT FROM THE DREAMCAST ROSTER, and its NAME is an
// inference the link order bounds rather than proves (the netmsg.obj
// precedent). Retail's .text is laid out in strict alphabetical compiland
// order, so this object's name sorts strictly between `button` and
// `campaignbrief`: button.obj ends at 0x456e94 and campaignbrief.obj opens
// at 0x457990. `campaign` is the shortest spelling in that interval and the
// one the class's role suggests; treat it as provisional.
//
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

// The five plate callbacks the constructor address-takes, retail 0x457280 /
// 0x4572f0 / 0x457360 / 0x4573d0 / 0x457440. Names provisional (nothing
// attests them); each is named for the sprite its plate carries.
// Before normalization (function): CampaignSetSodHandler.
static int campaignSetSodHandler(message& msg);
// Before normalization (function): CampaignSetArmHandler.
static int campaignSetArmHandler(message& msg);
// Before normalization (function): CampaignSetCusHandler.
static int campaignSetCusHandler(message& msg);
// Before normalization (function): CampaignSetExitHandler.
static int campaignSetExitHandler(message& msg);
// Before normalization (function): CampaignSetRoeHandler.
static int campaignSetRoeHandler(message& msg);

// The rollover latch the handler keeps between messages: the widget id the
// mouse was last over, -1 for none. Retail .data 0x660dc8, the one dword in
// this compiland's own data band ahead of its string pool, written only by
// the handler below.
// Before normalization: gCampaignSetHoverId.
DATA(0x00660dc8)
static int g_campaignSetHoverId = -1;

// The three plate rectangles retail reads out of .rdata rather than folding.
// SoD's and Armageddon's Blade's are immediates in the constructor's own
// push run; these three are `movsx`-loaded word by word from 0x63bbf0,
// which is what fixes them as const-array elements and the element type as
// `short`. Layout order in .rdata is source order (roe, cus, exit), and the
// run ends exactly on this class's vtable at 0x63bc08. Names INVENTED.
// Before normalization: gCampaignSetRoeRect.
// Before normalization: gCampaignSetCusRect.
DATA(0x0063bbf0) static const short g_campaignSetRoeRect[4] = { 494, 116, 287, 130 };
// Before normalization: gCampaignSetExitRect.
DATA(0x0063bbf8) static const short g_campaignSetCusRect[4] = { 554, 358, 169, 110 };
DATA(0x0063bc00) static const short g_campaignSetExitRect[4] = { 576, 464, 126, 108 };

// Retail 0x456ec0, the compiland's first real body. Five plates on an
// 800x600 heroWindow, each a type_func_button carrying its own callback and
// its own hotkey; the Armageddon's Blade plate exists only in
// video-game-state 3, so it also shifts every later widget id down by one
// when it is absent. `Widgets.reserve(4)` is retail's - one short of the
// five plates, which is why the vector still grows.
//
// Residual (99.9458%): ONE BYTE, the `sub esp` immediate - retail reserves
// 0xc and we reserve 8. Both sides use the same 26 temporary slots and the
// same eleven EH-state transitions; retail's allocator ALTERNATES its two
// four-byte temporaries between [ebp-0x10] (14 uses) and [ebp-0x14] (12),
// spilling `this` into a third slot at [ebp-0x18], while ours coalesces the
// pair onto [ebp-0x10] (26 uses) and puts `this` at [ebp-0x14]. Every
// displacement is masked by the comparison, so the frame size is the whole
// residual. The alternation is the phase button.h's set_hotkey note
// describes and is not reachable from this body.
VA(0x00456ec0, 0x337)  // anchor-string CSSsod.def + anchor-vtable 0x63bc08 + DoCampaignWindow's stack object, retail-only
TCampaignSetWindow::TCampaignSetWindow()
    : heroWindow(0, 0, 800, 600, 0)
{
    m_widgets.reserve(4);

    int widgetId = SOD_PLATE_ID;

    type_func_button* sodPlate = new type_func_button(
        534, 8, 201, 119, widgetId++,
        DATA_COMPGEN(0x00660dfc, campaignSetSodSprite, "CSSsod.def"),
        campaignSetSodHandler, 0, 1);
    m_widgets.push_back(sodPlate);
    sodPlate->setHotkey(0x1f);

    if (*g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH) {
        type_func_button* armPlate = new type_func_button(
            486, 242, 305, 119, widgetId++,
            DATA_COMPGEN(0x00660df0, campaignSetArmSprite, "CSSarm.def"),
            campaignSetArmHandler, 0, 1);
        m_widgets.push_back(armPlate);
        armPlate->setHotkey(0x1e);
    }

    type_func_button* roePlate = new type_func_button(
        g_campaignSetRoeRect[0], g_campaignSetRoeRect[1],
        g_campaignSetRoeRect[2], g_campaignSetRoeRect[3], widgetId++,
        DATA_COMPGEN(0x00660de4, campaignSetRoeSprite, "CSSroe.def"),
        campaignSetRoeHandler, 0, 1);
    m_widgets.push_back(roePlate);
    roePlate->setHotkey(0x13);

    type_func_button* cusPlate = new type_func_button(
        g_campaignSetCusRect[0], g_campaignSetCusRect[1],
        g_campaignSetCusRect[2], g_campaignSetCusRect[3], widgetId++,
        DATA_COMPGEN(0x00660dd8, campaignSetCusSprite, "CSScus.def"),
        campaignSetCusHandler, 0, 1);
    m_widgets.push_back(cusPlate);
    cusPlate->setHotkey(0x2e);

    type_func_button* exitPlate = new type_func_button(
        g_campaignSetExitRect[0], g_campaignSetExitRect[1],
        g_campaignSetExitRect[2], g_campaignSetExitRect[3], widgetId,
        DATA_COMPGEN(0x00660dcc, campaignSetExitSprite, "CSSexit.def"),
        campaignSetExitHandler, 0, 1);
    m_widgets.push_back(exitPlate);
    exitPlate->setHotkey(1);

    addWidgetsToMessageStream();
}

VA_COMPGEN(0x00457200, 0x21, SCALAR_DELETING_DTOR, TCampaignSetWindow)

// Retail 0x457230. The widget teardown plus the compiler-generated base
// destruction - the whole body of a window that owns nothing else.
VA(0x00457230, 0x4E)  // scalar-dtor callee + vtable 0x63bc08 slot 0's target, retail-only
TCampaignSetWindow::~TCampaignSetWindow()
{
    deleteWidgets();
}

// The five plate callbacks. Each answers a right-click with its own help
// row and a left-release with the modal result DoCampaignWindow switches
// on; the Exit plate answers the shared 0x7801 dialog-cancel id.
VA(0x00457280, 0x64)  // ctor address-take (SoD plate), retail-only
static int campaignSetSodHandler(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(g_generalText->getText(TCampaignSetWindow::CAMPAIGN_SET_SOD_HELP), 4, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.m_codeX == widget::WIDGET_DESELECT && !(msg.m_qualifier & 0x200)) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = TCampaignSetWindow::CAMPAIGN_SET_SOD_ID;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x004572f0, 0x64)  // ctor address-take (Armageddon's Blade plate), retail-only
static int campaignSetArmHandler(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(g_generalText->getText(TCampaignSetWindow::CAMPAIGN_SET_ARM_HELP), 4, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.m_codeX == widget::WIDGET_DESELECT && !(msg.m_qualifier & 0x200)) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = TCampaignSetWindow::CAMPAIGN_SET_AB_ID;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x00457360, 0x64)  // ctor address-take (custom-campaign plate), retail-only
static int campaignSetCusHandler(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(g_generalText->getText(TCampaignSetWindow::CAMPAIGN_SET_CUS_HELP), 4, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.m_codeX == widget::WIDGET_DESELECT && !(msg.m_qualifier & 0x200)) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = TCampaignSetWindow::CUSTOM_CAMPAIGN_ID;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x004573d0, 0x64)  // ctor address-take (Exit plate), retail-only
static int campaignSetExitHandler(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(g_generalText->getText(TCampaignSetWindow::CAMPAIGN_SET_EXIT_HELP), 4, -1,
                     -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.m_codeX == widget::WIDGET_DESELECT && !(msg.m_qualifier & 0x200)) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = DIALOG_RETURN_CANCEL;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x00457440, 0x60)  // ctor address-take (Restoration of Erathia plate), retail-only
static int campaignSetRoeHandler(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(g_generalText->getText(TCampaignSetWindow::CAMPAIGN_SET_ROE_HELP), 4, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.m_codeX == widget::WIDGET_DESELECT && !(msg.m_qualifier & 0x200)) {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = TCampaignSetWindow::CAMPAIGN_SET_ROE_ID;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

// Retail 0x4574a0. Not a vtable slot - 0x63bc08 slot 6 is heroWindow's own
// DoModal - but the same shape plus the menu track, which is what makes the
// campaign-set page the one that restarts MainMenu.
VA(0x004574a0, 0x2C)  // anchor-string MainMenu + anchor-callee DoDialog(HeroWindowHandler), retail-only
void TCampaignSetWindow::doModal()
{
    g_soundManager->startMP3(
        DATA_COMPGEN(0x00660e08, campaignSetMusic, "MainMenu"), 0, 1);
    g_windowManager->doDialog(this, heroWindowHandler, 0);
}

// Retail 0x4574d0, vtable slot 3. The plate hover sweep: whichever plate is
// under the mouse gets WIDGET_SET_STATUS 0x10 and the one it replaced gets
// WIDGET_CLEAR_STATUS 0x10, and the plate band is repainted whenever that
// changed or the video layer needs it.
// The DrawWindow bound is written `x = a; if (c) x = b;` and not as a
// ternary: retail loads 103 unconditionally and overwrites it under `jne`,
// while `c ? 104 : 103` gives `sete cl / add ecx, 103` (95.13 against
// 100.00).
VA(0x004574d0, 0xC4)  // vtable 0x63bc08 slot 3, retail-only
int TCampaignSetWindow::handleMessage(message& msg)
{
    unsigned char hoverChanged = 0;

    pollSound();

    if (msg.m_id == MESSAGE_MOUSE_MOVE) {
        int hovered = findWidget(msg.m_mouseX, msg.m_mouseY);
        if (hovered != g_campaignSetHoverId) {
            hoverChanged = 1;
            if (g_campaignSetHoverId != -1)
                getWidget(g_campaignSetHoverId)
                    ->sendMessage(widget::WIDGET_CLEAR_STATUS, 0x10);
            if (hovered != -1)
                getWidget(hovered)->sendMessage(widget::WIDGET_SET_STATUS,
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
