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

#include "campaignwindow.h"

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
static int CampaignSetSodHandler(message& msg);
static int CampaignSetArmHandler(message& msg);
static int CampaignSetCusHandler(message& msg);
static int CampaignSetExitHandler(message& msg);
static int CampaignSetRoeHandler(message& msg);

// The rollover latch the handler keeps between messages: the widget id the
// mouse was last over, -1 for none. Retail .data 0x660dc8, the one dword in
// this compiland's own data band ahead of its string pool, written only by
// the handler below.
DATA(0x00660dc8)
static int gCampaignSetHoverId = -1;

// The three plate rectangles retail reads out of .rdata rather than folding.
// SoD's and Armageddon's Blade's are immediates in the constructor's own
// push run; these three are `movsx`-loaded word by word from 0x63bbf0,
// which is what fixes them as const-array elements and the element type as
// `short`. Layout order in .rdata is source order (roe, cus, exit), and the
// run ends exactly on this class's vtable at 0x63bc08. Names INVENTED.
DATA(0x0063bbf0) static const short gCampaignSetRoeRect[4] = { 494, 116, 287, 130 };
DATA(0x0063bbf8) static const short gCampaignSetCusRect[4] = { 554, 358, 169, 110 };
DATA(0x0063bc00) static const short gCampaignSetExitRect[4] = { 576, 464, 126, 108 };

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
    Widgets.reserve(4);

    int widgetId = SOD_PLATE_ID;

    type_func_button* sodPlate = new type_func_button(
        534, 8, 201, 119, widgetId++,
        DATA_COMPGEN(0x00660dfc, campaignSetSodSprite, "CSSsod.def"),
        CampaignSetSodHandler, 0, 1);
    Widgets.push_back(sodPlate);
    sodPlate->set_hotkey(0x1f);

    if (*gpVideoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH) {
        type_func_button* armPlate = new type_func_button(
            486, 242, 305, 119, widgetId++,
            DATA_COMPGEN(0x00660df0, campaignSetArmSprite, "CSSarm.def"),
            CampaignSetArmHandler, 0, 1);
        Widgets.push_back(armPlate);
        armPlate->set_hotkey(0x1e);
    }

    type_func_button* roePlate = new type_func_button(
        gCampaignSetRoeRect[0], gCampaignSetRoeRect[1],
        gCampaignSetRoeRect[2], gCampaignSetRoeRect[3], widgetId++,
        DATA_COMPGEN(0x00660de4, campaignSetRoeSprite, "CSSroe.def"),
        CampaignSetRoeHandler, 0, 1);
    Widgets.push_back(roePlate);
    roePlate->set_hotkey(0x13);

    type_func_button* cusPlate = new type_func_button(
        gCampaignSetCusRect[0], gCampaignSetCusRect[1],
        gCampaignSetCusRect[2], gCampaignSetCusRect[3], widgetId++,
        DATA_COMPGEN(0x00660dd8, campaignSetCusSprite, "CSScus.def"),
        CampaignSetCusHandler, 0, 1);
    Widgets.push_back(cusPlate);
    cusPlate->set_hotkey(0x2e);

    type_func_button* exitPlate = new type_func_button(
        gCampaignSetExitRect[0], gCampaignSetExitRect[1],
        gCampaignSetExitRect[2], gCampaignSetExitRect[3], widgetId,
        DATA_COMPGEN(0x00660dcc, campaignSetExitSprite, "CSSexit.def"),
        CampaignSetExitHandler, 0, 1);
    Widgets.push_back(exitPlate);
    exitPlate->set_hotkey(1);

    AddWidgetsToMessageStream();
}

VA_COMPGEN(0x00457200, 0x21, SCALAR_DELETING_DTOR, TCampaignSetWindow)

// Retail 0x457230. The widget teardown plus the compiler-generated base
// destruction - the whole body of a window that owns nothing else.
VA(0x00457230, 0x4E)  // scalar-dtor callee + vtable 0x63bc08 slot 0's target, retail-only
TCampaignSetWindow::~TCampaignSetWindow()
{
    delete_widgets();
}

// The five plate callbacks. Each answers a right-click with its own help
// row and a left-release with the modal result DoCampaignWindow switches
// on; the Exit plate answers the shared 0x7801 dialog-cancel id.
VA(0x00457280, 0x64)  // ctor address-take (SoD plate), retail-only
static int CampaignSetSodHandler(message& msg)
{
    if (msg.codeX == widget::WIDGET_RIGHT_SELECT) {
        NormalDialog(gpGeneralText->GetText(TCampaignSetWindow::CAMPAIGN_SET_SOD_HELP), 4, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.codeX == widget::WIDGET_DESELECT && !(msg.qualifier & 0x200)) {
        msg.id = MESSAGE_WIDGET;
        msg.codeY = TCampaignSetWindow::CAMPAIGN_SET_SOD_ID;
        msg.codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x004572f0, 0x64)  // ctor address-take (Armageddon's Blade plate), retail-only
static int CampaignSetArmHandler(message& msg)
{
    if (msg.codeX == widget::WIDGET_RIGHT_SELECT) {
        NormalDialog(gpGeneralText->GetText(TCampaignSetWindow::CAMPAIGN_SET_ARM_HELP), 4, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.codeX == widget::WIDGET_DESELECT && !(msg.qualifier & 0x200)) {
        msg.id = MESSAGE_WIDGET;
        msg.codeY = TCampaignSetWindow::CAMPAIGN_SET_AB_ID;
        msg.codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x00457360, 0x64)  // ctor address-take (custom-campaign plate), retail-only
static int CampaignSetCusHandler(message& msg)
{
    if (msg.codeX == widget::WIDGET_RIGHT_SELECT) {
        NormalDialog(gpGeneralText->GetText(TCampaignSetWindow::CAMPAIGN_SET_CUS_HELP), 4, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.codeX == widget::WIDGET_DESELECT && !(msg.qualifier & 0x200)) {
        msg.id = MESSAGE_WIDGET;
        msg.codeY = TCampaignSetWindow::CUSTOM_CAMPAIGN_ID;
        msg.codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x004573d0, 0x64)  // ctor address-take (Exit plate), retail-only
static int CampaignSetExitHandler(message& msg)
{
    if (msg.codeX == widget::WIDGET_RIGHT_SELECT) {
        NormalDialog(gpGeneralText->GetText(TCampaignSetWindow::CAMPAIGN_SET_EXIT_HELP), 4, -1,
                     -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.codeX == widget::WIDGET_DESELECT && !(msg.qualifier & 0x200)) {
        msg.id = MESSAGE_WIDGET;
        msg.codeY = DIALOG_RETURN_CANCEL;
        msg.codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x00457440, 0x60)  // ctor address-take (Restoration of Erathia plate), retail-only
static int CampaignSetRoeHandler(message& msg)
{
    if (msg.codeX == widget::WIDGET_RIGHT_SELECT) {
        NormalDialog(gpGeneralText->GetText(TCampaignSetWindow::CAMPAIGN_SET_ROE_HELP), 4, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }
    if (msg.codeX == widget::WIDGET_DESELECT && !(msg.qualifier & 0x200)) {
        msg.id = MESSAGE_WIDGET;
        msg.codeY = TCampaignSetWindow::CAMPAIGN_SET_ROE_ID;
        msg.codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

// Retail 0x4574a0. Not a vtable slot - 0x63bc08 slot 6 is heroWindow's own
// DoModal - but the same shape plus the menu track, which is what makes the
// campaign-set page the one that restarts MainMenu.
VA(0x004574a0, 0x2C)  // anchor-string MainMenu + anchor-callee DoDialog(HeroWindowHandler), retail-only
void TCampaignSetWindow::DoModal()
{
    gpSoundManager->StartMP3(
        DATA_COMPGEN(0x00660e08, campaignSetMusic, "MainMenu"), 0, 1);
    gpWindowManager->DoDialog(this, HeroWindowHandler, 0);
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
int TCampaignSetWindow::handle_message(message& msg)
{
    unsigned char hoverChanged = 0;

    PollSound();

    if (msg.id == MESSAGE_MOUSE_MOVE) {
        int hovered = findWidget(msg.mouseX, msg.mouseY);
        if (hovered != gCampaignSetHoverId) {
            hoverChanged = 1;
            if (gCampaignSetHoverId != -1)
                GetWidget(gCampaignSetHoverId)
                    ->send_message(widget::WIDGET_CLEAR_STATUS, 0x10);
            if (hovered != -1)
                GetWidget(hovered)->send_message(widget::WIDGET_SET_STATUS,
                                                 0x10);
            gCampaignSetHoverId = hovered;
        }
    }

    if (VideoNeedsUpdate() || hoverChanged) {
        int lastPlate = LAST_PLATE_ID;
        if (*gpVideoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH)
            lastPlate = LAST_PLATE_WITH_AB_ID;
        DrawWindow(0, SOD_PLATE_ID, lastPlate);
        gpWindowManager->UpdateScreen(482, 9, 308, 562);
        VideoDrawRects();
    }

    return MESSAGE_DISPATCH_CONSUME;
}
