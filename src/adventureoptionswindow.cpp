// adventureoptionswindow.cpp - E:\gamedcs\adventureoptionswindow.cpp
// (compiland adventureoptionswindow.obj)
// HAND-OWNED after admission; retail bytes are authoritative.
#include <va.h>
#include "adventureoptionswindow.h"
#include "border.h"
#include "button.h"
#include "game.h"
#include "kb.h"
#include "message.h"
#include "mousemgr.h"
#include "soundmgr.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// DC public gAdventureWindowHelp; retail consumers prove seven THelpText
// rows at 0x6a6530 (the sixth row is unused by this dialog's ID mapping).
DATA(0x006a6530) extern THelpText gAdventureWindowHelp[7];

// File-static hover latch: the retail initializer at 0x65f46c is -1 and the
// handler is its only image-wide reader/writer.
DATA(0x0065f46c) static int lastIMHoverID = -1;

// E:\gamedcs\adventureoptionswindow.cpp:40
// Residual (89.5711%): every widget, argument, literal, hotkey, player gate,
// post-reserve gameplay branch and return agrees. Retail's inlined
// vector<widget*>::reserve calls the shared _Ucopy/range-destroy helpers;
// this SP3 compile expands the element-copy/cleanup loop, adding two branches
// and perturbing downstream register scheduling. The identical instantiation
// is already bounded by the quickinfowindow/armygrp probes: inline-depth 0/1,
// a reserve adapter, named-local variants and type-population noise did not
// recover retail's helper choice.
VA(0x004051d0, 0x4AA)  // advopts.pcx + advManager caller, dc 0x4cf4
TAdventureOptionsWindow::TAdventureOptionsWindow()
    : CAdvPopup(255, 106, 289, 387, 0x12)
{
    Widgets.reserve(8);

    bitmapBorder* background = new bitmapBorder(
        0, 0, 289, 387, ADVENTURE_OPTION_BACKGROUND_ID,
        "AdvOpts.pcx", 0x800);
    background->SetPlayerPaletteColors(gpGame->GetLocalPlayerGamePos());
    Widgets.push_back(background);

    button* action = new button(
        25, 24, 49, 51, VIEW_WORLD_ID,
        "AdvView.def", 0, 1, 0, 0, 2);
    action->set_hotkey(ADVENTURE_OPTION_VIEW_HOTKEY);
    Widgets.push_back(action);

    action = new button(
        25, 82, 49, 51, VIEW_PUZZLE_ID,
        "AdvPuz.def", 0, 1, 0, 0, 2);
    action->set_hotkey(ADVENTURE_OPTION_PUZZLE_HOTKEY);
    Widgets.push_back(action);

    action = new button(
        25, 140, 49, 51, DIG_ID,
        "AdvDig.def", 0, 1, 0, 0, 2);
    action->set_hotkey(ADVENTURE_OPTION_DIG_HOTKEY);
    Widgets.push_back(action);

    action = new button(
        25, 198, 49, 51, VIEW_SCENARIO_ID,
        "AdvInfo.def", 0, 1, 0, 0, 2);
    action->set_hotkey(ADVENTURE_OPTION_INFO_HOTKEY);
    Widgets.push_back(action);

    action = new button(
        25, 256, 49, 51, REPLAY_ID,
        "AdvTurn.def", 0, 1, 0, 0, 2);
    action->enable(gpGame->replay_available());
    action->set_hotkey(ADVENTURE_OPTION_TURN_HOTKEY);
    Widgets.push_back(action);

    button* accept = new button(
        203, 313, 64, 32, ADVENTURE_OPTION_ACCEPT_ID,
        "iOk6432.def", 0, 1, 1, 0, 2);
    accept->set_hotkey(ADVENTURE_OPTION_ACCEPT_HOTKEY_1);
    accept->set_hotkey(ADVENTURE_OPTION_ACCEPT_HOTKEY_2);
    Widgets.push_back(accept);

    RolloverWidget = new textWidget(
        6, 360, 275, 20, "", "smalfont.fnt", font::PRIMARY,
        ADVENTURE_OPTION_ROLLOVER_ID, 5, 0, 8);
    Widgets.push_back(RolloverWidget);

    for (widget** it = Widgets.begin(); it != Widgets.end(); ++it) {
        if (*it)
            AddWidget(*it, -1);
        else
            MemError();
    }

    // THE DIG GATE ASKS game::GetCurrHeroId (89.5711 -> 94.3823,
    // 2026-08-15): dc 0x4cf4 line 90 is a call to
    // `?GetCurrHeroId@game@@QAAHXZ` where this body read
    // `gpCurrentPlayer->currHeroId` directly. Same load either way - what
    // moves is the /Ob2 candidate-site count.
    if (gpGame->GetCurrHeroId() == -1) {
        widget* dig = GetWidget(DIG_ID);
        dig->enable(0);
    }

    if (!gpCurrentPlayer->IsLocalHuman()) {
        widget* turn = GetWidget(REPLAY_ID);
        turn->enable(0);
        widget* dig = GetWidget(DIG_ID);
        dig->enable(0);
    }
}

// Retail emits CHeroWindowEx's header-inline slot-3 forwarder at 0x405680
// before the class's generated deleting destructor. It is shared by 42
// vtables and is not one of this source file's five Dreamcast roster rows;
// ownership remains with the window-header emission until that ICF surface is
// admitted deliberately.

VA_COMPGEN(0x00405690, 0x21, SCALAR_DELETING_DTOR, TAdventureOptionsWindow)

// E:\gamedcs\adventureoptionswindow.cpp:105
VA(0x004056c0, 0x6B)  // scalar-dtor callee + derived vtable, dc 0x514c
TAdventureOptionsWindow::~TAdventureOptionsWindow()
{
    for (widget** it = Widgets.begin(); it != Widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// E:\gamedcs\adventureoptionswindow.cpp:112
// Dreamcast emits this const helper out of line. Retail's handler contains
// its ID-to-help-index switch inline and the 0x405680 slot is independently
// proved to be CHeroWindowEx's handle_message forwarder, so no retail body is
// forced into the remaining roster slot.
#if 0  // @carcass
DC_ONLY(0x51b0, 0x54)
int TAdventureOptionsWindow::convertID2HelpID(int id) const
{
    // @stub
}
#endif

// E:\gamedcs\adventureoptionswindow.cpp:143
VA(0x00405730, 0x1FC)  // derived vtable slot 9, dc 0x5204
int TAdventureOptionsWindow::WindowHandler(message* msg)
{
    int result = CAdvPopup::WindowHandler(msg);
    if (result)
        return result;

    PollSound();

    if (msg->qualifier & MESSAGE_MODIFIER_RIGHT) {
        if (msg->codeX == widget::WIDGET_SELECT
            || msg->codeX == widget::WIDGET_RIGHT_SELECT) {
            if (msg->codeY >= 0 && msg->codeY <= ADVENTURE_OPTION_ACCEPT_ID) {
                int helpID = convertID2HelpID(msg->codeY);
                if (helpID == -1)
                    goto consume;
                NormalDialog(gAdventureWindowHelp[helpID].rclick,
                    4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            }
        }
consume:
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg->id == MESSAGE_WIDGET) {
        if (msg->codeX == widget::WIDGET_DESELECT
            && (msg->codeY == ADVENTURE_OPTION_ACCEPT_ID
                || (msg->codeY > 0 && msg->codeY <= 5))) {
            msg->id = MESSAGE_WIDGET;
            gpWindowManager->dialogReturn = msg->codeY;
            msg->codeY = widget::WIDGET_END_DIALOG;
            msg->codeX = widget::WIDGET_END_DIALOG;
            return MESSAGE_DISPATCH_FORWARD;
        }
        goto consume;
    }

    if (msg->id == MESSAGE_MOUSE_MOVE) {
        // Residual (99.94%): retail loads mouseX into ECX while this VC6 SP3
        // build coalesces the now-dead msg/ESI home into mouseX; every branch,
        // epilogue, and remaining instruction agrees. Tried and rejected:
        // direct member arguments, split declarations, moving hoverID's
        // declaration ahead of the coordinates, and an unused symbol-order
        // perturbation.
        int mouseY = msg->mouseY, mouseX = msg->mouseX;
        int hoverID = findWidget(mouseX, mouseY);
        if (hoverID != lastIMHoverID) {
            lastIMHoverID = hoverID;
            const char* rollover = "";
            if (hoverID != -1) {
                gpMouseManager->SetPointer(1, mouseManager::DEFAULT_SET);
                if (hoverID >= 0 && hoverID <= ADVENTURE_OPTION_ACCEPT_ID) {
                    int helpID = convertID2HelpID(hoverID);
                    if (helpID != -1)
                        rollover = gAdventureWindowHelp[helpID].text;
                }
            } else {
                gpMouseManager->SetPointer(0, mouseManager::DEFAULT_SET);
            }
            RolloverWidget->SetText(rollover);
            DrawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
            return MESSAGE_DISPATCH_CONSUME;
        }
    }

    goto consume;
}
