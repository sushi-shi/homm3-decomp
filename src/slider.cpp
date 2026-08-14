// slider.cpp - E:\gamedcs\slider.cpp (compiland slider.obj)
// HAND-OWNED after admission. Retail Complete x86 is authoritative; the
// Dreamcast roster supplies names and source-level signatures.
#include <va.h>
#include "bitmap816.h"
#include "csprite.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "message.h"
#include "mousemgr.h"
#include "resourcemanager.h"
#include "slider.h"
#include "soundmgr.h"
#include "window.h"
#include "winmgr.h"

// Dreamcast names slider.obj's private left/right modifier latch. Retail's
// Select stores the message mask here and Deselect consumes and clears it.
DATA(0x0069fdd4)
static int iLeftRightSave;

VA_COMPGEN(0x00596020, 0x21, SCALAR_DELETING_DTOR, slider)

// E:\gamedcs\slider.cpp:35
// No standalone Complete entry survives; keep the cross-build body out of
// the retail object until a caller proves whether it was inlined or dropped.
#if 0  // @carcass -- no retail entry
DC_ONLY(0x1499f0, 0x58)
slider::slider()
{
}
#endif

// E:\gamedcs\slider.cpp:50
VA(0x00596050, 0x7D)  // literal/callee/field-layout proof, dc 0x149a48
void slider::initialize(const char* resource_name)
{
    if (width > height) {
        length = width;
        sliderSprite = ResourceManager::GetSprite(resource_name);
        sliderBitmap = ResourceManager::GetBitmap816(
            DATA_COMPGEN(0x00683980, sliderHorizontalBitmap, "slider.pcx"));
        knob_start = sliderSprite->Width;
    } else {
        length = height;
        sliderSprite = ResourceManager::GetSprite(resource_name);
        sliderBitmap = ResourceManager::GetBitmap816(
            DATA_COMPGEN(0x00683974, sliderVerticalBitmap, "sliderV.pcx"));
        knob_start = sliderSprite->Height;
    }

    knobPos = knob_start;
    knobRange = length - knobPos * 3;
    oldState = 0;
    currentState = 0;
}

// E:\gamedcs\slider.cpp:96
VA(0x005960D0, 0xA8)  // ctor/EH/vtable/literal proof, dc 0x149ae8
slider::slider(int x, int y, int w, int h, int id, int num,
               TSliderFunction func, EGraphics graphics, int page,
               unsigned char hotKey)
    : widget(x, y, w, h, id, 1)
{
    pageSize = page;
    if (w > h) {
        if (graphics == BROWN)
            initialize(DATA_COMPGEN(0x006839A8, sliderBrownHorizontalSprite,
                                    "iGPCrDiv.def"));
        else
            initialize(DATA_COMPGEN(0x00660E1C, sliderBlueHorizontalSprite,
                                    "SlideBuH.def"));
    } else {
        if (graphics == BROWN)
            initialize(DATA_COMPGEN(0x0068399C, sliderBrownVerticalSprite,
                                    "OvButn2.def"));
        else
            initialize(DATA_COMPGEN(0x0068398C, sliderBlueVerticalSprite,
                                    "SlideBuV.def"));
    }
    sliderFunction = func;
    numStates = num;
    hotKeys = hotKey;
}

// E:\gamedcs\slider.cpp:124
VA(0x00596180, 0x59)  // vtable/resource Dispose/base dtor, dc 0x149ba4
slider::~slider()
{
    sliderBitmap->Dispose();
    sliderSprite->Dispose();
}

// E:\gamedcs\slider.cpp:143
VA(0x005961E0, 0x4C)  // contiguous slider block, dc 0x149bec
void slider::SetState(int state)
{
    if (state < 0)
        state = 0;
    else if (state >= numStates)
        state = numStates - 1;

    oldState = currentState;
    currentState = state;
    if (numStates <= 1)
        knobPos = knob_start;
    else
        knobPos = knob_start + knobRange * state / (numStates - 1);
}

// E:\gamedcs\slider.cpp:159
VA(0x00596230, 0x2A2)  // contiguous slider block, dc 0x149c90
void slider::KeyAccel(int x1, int x2, int x3, int x4, int key)
{
    status |= WIDGET_SELECTED;
    sliderSprite->DrawInterface(
        x1, 0, 0, sliderSprite->Width, sliderSprite->Height,
        gpWindowManager->screenBitmap,
        x + parentWindow->x, y + parentWindow->y,
        0);
    int endY = parentWindow->y + y;
    endY -= knob_start;
    endY += length;
    sliderSprite->DrawInterface(
        x2, 0, 0, sliderSprite->Width, sliderSprite->Height,
        gpWindowManager->screenBitmap,
        x + parentWindow->x,
        endY,
        0);
    sliderBitmap->Draw(
        x3, 0, width, length - knob_start * 2,
        gpWindowManager->screenBitmap,
        x + parentWindow->x, y + parentWindow->y + knob_start, 0);
    sliderSprite->DrawInterface(
        x4, 0, 0, sliderSprite->Width, sliderSprite->Height,
        gpWindowManager->screenBitmap,
        x + parentWindow->x, y + parentWindow->y + knobPos,
        0);
    gpWindowManager->UpdateScreen(
        x + parentWindow->x, y + parentWindow->y, width, height);
    glTimers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT] = GameTime::Get() + 60;

    if (oldState != currentState) {
        oldState = currentState;
        Close();
        if (sliderFunction)
            sliderFunction(currentState, parentWindow);
    }

    switch (key) {
    case KEYCODE_KP_9:
        SetState(currentState - pageSize);
        break;
    case KEYCODE_KP_3:
        SetState(currentState + pageSize);
        break;
    case KEYCODE_KP_8:
        --currentState;
        knobPos = knobRange * currentState / (numStates - 1) + knob_start;
        break;
    case KEYCODE_KP_2:
        ++currentState;
        knobPos = knobRange * currentState / (numStates - 1) + knob_start;
        break;
    }

    status &= ~WIDGET_SELECTED;
    Draw();
    unsigned long repeatTime = glTimers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT];
    while (static_cast<int>(
               GameTime::Get() - repeatTime) <= 0) {
        PollSound();
        Process1WindowsMessage();
        repeatTime = glTimers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT];
    }
    gpWindowManager->UpdateScreen(
        x + parentWindow->x, y + parentWindow->y, width, height);

    if (oldState != currentState) {
        oldState = currentState;
        Close();
        if (sliderFunction)
            sliderFunction(currentState, parentWindow);
    }
}

// E:\gamedcs\slider.cpp:244
// The source-compatible 60-branch/11-return CFG is complete. Retail shares
// the KP3/KP2 forward KeyAccel suffix; this VC6 invocation instead shares the
// equivalent KP9/KP8 backward suffix, leaving the measured 95.190125% C2
// tail-merging plateau after ordering and lifetime probes.
VA(0x005964E0, 0x4A0)  // contiguous slider block, dc 0x149f04
int slider::Main(message* msg)
{
    if (style == WIDGET_STYLE_AUTO_REPEAT && (status & WIDGET_SELECTED)) {
        unsigned long repeatTime = glTimers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT];
        if (static_cast<int>(GameTime::Get() - repeatTime) > 0)
            return Deselect(msg);
    }

    if (!(status & WIDGET_ACTIVE)) {
        if (msg->id != MESSAGE_WIDGET)
            return 0;
        return widget::Main(msg);
    }

    unsigned char isDisabled = 0;
    if (status & WIDGET_DISABLED)
        isDisabled = 1;

    switch (msg->id) {
    case MESSAGE_KEY_UP:
        if (isDisabled)
            return 0;
        if (!(status & WIDGET_DRAWN) || (status & WIDGET_DIMMED))
            goto callWidgetMain;
        if (!hotKeys)
            return 0;
        switch (msg->codeX) {
        case KEYCODE_KP_9:
            KeyAccel(1, 2, 0, 4, KEYCODE_KP_9);
            return 0;
        case KEYCODE_KP_3:
            KeyAccel(0, 3, 0, 4, KEYCODE_KP_3);
            break;
        case KEYCODE_KP_8:
            if (currentState > 0) {
                KeyAccel(1, 2, 0, 4, KEYCODE_KP_8);
                return 0;
            }
            break;
        case KEYCODE_KP_2:
            if (currentState < numStates - 1)
                KeyAccel(0, 3, 0, 4, KEYCODE_KP_2);
            break;
        }
        return 0;

    case MESSAGE_LEFT_BUTTON_DOWN:
        if (!(status & WIDGET_DRAWN))
            goto callWidgetMain;
        if (isDisabled)
            return 0;
        clickX = msg->codeX - parentWindow->x;
        clickY = msg->codeY - parentWindow->y;
        if (status & WIDGET_DIMMED)
            return 0;
        if (clickX < x || clickY < y || clickX >= x + width
            || clickY >= y + height)
            return 0;

        Select(msg, 0);
        for (;;) {
            if (msg->id == MESSAGE_LEFT_BUTTON_UP
                || msg->id == MESSAGE_RIGHT_BUTTON_UP)
                break;
            PollSound();
            gpMouseManager->Main(*msg);
            if (msg->id == MESSAGE_MOUSE_MOVE) {
                clickX = msg->codeX - parentWindow->x;
                clickY = msg->codeY - parentWindow->y;
                if (width > height) {
                    int distance = y - clickY;
                    if (distance < 40 && distance > 0)
                        clickY = y;
                    distance = clickY - y - height;
                    if (distance < 40 && distance > 0)
                        clickY = y;
                    if (clickX >= x + knob_start
                        && clickX < x + width - knob_start
                        && clickY >= y && clickY < y + height)
                        Select(msg, 1);
                } else {
                    int distance = x - clickX;
                    if (distance < 40 && distance > 0)
                        clickX = x;
                    distance = clickX - x - width;
                    if (distance < 40 && distance > 0)
                        clickX = x;
                    if (clickX >= x && clickY >= y + knob_start
                        && clickX < x + width
                        && clickY < y + height - knob_start)
                        Select(msg, 1);
                }
            }
            Process1WindowsMessage();
            *msg = gpInputManager->GetEvent();
            if (msg->id == MESSAGE_LEFT_BUTTON_UP)
                break;
        }
        if (status & WIDGET_SELECTED) {
            Deselect(msg);
            return 2;
        }
        return 1;

    case MESSAGE_LEFT_BUTTON_UP:
        if (isDisabled)
            return 0;
        if (!(status & WIDGET_DRAWN) || !(status & WIDGET_SELECTED))
            goto callWidgetMain;
        return Deselect(msg);

    case MESSAGE_RIGHT_BUTTON_DOWN:
        if (!(status & WIDGET_DRAWN))
            goto callWidgetMain;
        clickX = msg->codeX - parentWindow->x;
        clickY = msg->codeY - parentWindow->y;
        if (clickX < x || clickY < y || clickX >= x + width
            || clickY >= y + height)
            return 0;
        msg->id = MESSAGE_WIDGET;
        msg->codeX = WIDGET_RIGHT_SELECT;
        msg->codeY = id;
        msg->qualifier = MESSAGE_MODIFIER_RIGHT;
        return 2;

    case MESSAGE_WIDGET:
        switch (msg->codeX) {
        case WIDGET_SET_SLIDER_STATE:
            if (msg->codeY == id) {
                SetState(msg->extra);
                return 1;
            }
            break;
        case WIDGET_SET_SLIDER_RESOLUTION:
            if (msg->codeY == id) {
                SetResolution(msg->extra);
                return 1;
            }
            break;
        }
        goto callWidgetMain;
    }

callWidgetMain:
    return widget::Main(msg);
}

// E:\gamedcs\slider.cpp:477
VA(0x00596980, 0x167)  // contiguous slider block, dc 0x14a380
int slider::Select(message* msg, unsigned char dragging)
{
    status |= WIDGET_SELECTED;

    if (width > height) {
        int click = clickX - x;
        if (click >= knob_start && click < length - knob_start) {
            if (pageSize > 0 && !dragging) {
                if (click < knobPos)
                    SetState(currentState - pageSize);
                else if (click < knobPos + 16)
                    SetKnob(clickX);
                else
                    SetState(currentState + pageSize);
            } else {
                SetKnob(clickX);
            }
        }
    } else {
        int click = clickY - y;
        if (click >= knob_start && click < length - knob_start) {
            if (pageSize > 0 && !dragging) {
                if (click < knobPos)
                    SetState(currentState - pageSize);
                else if (click >= knobPos + knob_start)
                    SetState(currentState + pageSize);
                else
                    SetKnob(clickY);
            } else {
                SetKnob(clickY);
            }
        }
    }

    Draw();
    gpWindowManager->UpdateScreen(
        x + parentWindow->x, y + parentWindow->y, width, height);
    msg->id = MESSAGE_WIDGET;
    msg->codeX = WIDGET_SELECT;
    msg->codeY = id;
    glTimers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT] = GameTime::Get() + 60;
    iLeftRightSave = msg->qualifier & MESSAGE_MODIFIER_MASK;

    if (oldState != currentState) {
        oldState = currentState;
        Close();
        if (sliderFunction)
            sliderFunction(currentState, parentWindow);
    }
    return 2;
}

// E:\gamedcs\slider.cpp:545
// EXACT 2026-08-11. Successful decrements jump to the common redraw exit;
// preserving that source edge releases the coordinate lifetime and lets C2
// share retail's knob arithmetic without an extra EBX save.
VA(0x00596AF0, 0x143)  // contiguous slider block, dc 0x14a508
int slider::Deselect(message* msg)
{
    if (!(status & WIDGET_SELECTED))
        return 0;
    status &= ~WIDGET_SELECTED;

    if (width > height) {
        if (clickX - x < knob_start && currentState > 0) {
            --currentState;
            knobPos = knobRange * currentState / (numStates - 1)
                + knob_start;
            goto redraw;
        }
        if (clickX - x > length - knob_start
            && currentState < numStates - 1) {
            ++currentState;
            knobPos = knobRange * currentState / (numStates - 1)
                + knob_start;
        }
    } else {
        if (clickY - y < knob_start && currentState > 0) {
            --currentState;
            knobPos = knobRange * currentState / (numStates - 1)
                + knob_start;
            goto redraw;
        }
        if (clickY - y > length - knob_start
            && currentState < numStates - 1) {
            ++currentState;
            knobPos = knobRange * currentState / (numStates - 1)
                + knob_start;
        }
    }

redraw:
    Draw();
    gpWindowManager->UpdateScreen(
        x + parentWindow->x, y + parentWindow->y, width, height);
    msg->id = MESSAGE_WIDGET;
    msg->codeY = id;
    msg->codeX = WIDGET_DESELECT;
    msg->qualifier = iLeftRightSave;
    iLeftRightSave = 0;

    if (oldState != currentState) {
        oldState = currentState;
        Close();
        if (sliderFunction)
            sliderFunction(currentState, parentWindow);
    }
    return 2;
}

// Retail folds these header-sized bodies into representatives owned by other
// units (vtable targets 0x4eab20/30 and 0x5bc7e0).
#if 0  // @carcass -- ICF/header COMDAT, no slider.obj home
DC_ONLY(0x14a67c, 0x16)
int slider::GetRealWidth() { return width; }
DC_ONLY(0x14a694, 0x16)
int slider::GetRealHeight() { return height; }
DC_ONLY(0x14a6ac, 0x4)
void slider::zBufferDraw() {}
#endif

// E:\gamedcs\slider.cpp:613
VA(0x00596C40, 0x3D5)  // contiguous slider block, dc 0x14a6b0
void slider::Draw()
{
    if (width > height) {
        if ((status & WIDGET_SELECTED) && clickX - x < knob_start) {
            sliderSprite->DrawInterface(
                1, 0, 0, sliderSprite->Width, sliderSprite->Height,
                gpWindowManager->screenBitmap,
                x + parentWindow->x, y + parentWindow->y, 0);
        } else {
            sliderSprite->DrawInterface(
                0, 0, 0, sliderSprite->Width, sliderSprite->Height,
                gpWindowManager->screenBitmap,
                x + parentWindow->x, y + parentWindow->y, 0);
        }

        if ((status & WIDGET_SELECTED)
            && clickX - x > length - knob_start) {
            sliderSprite->DrawInterface(
                3, 0, 0, sliderSprite->Width, sliderSprite->Height,
                gpWindowManager->screenBitmap,
                x + parentWindow->x + length - knob_start,
                y + parentWindow->y, 0);
        } else {
            sliderSprite->DrawInterface(
                2, 0, 0, sliderSprite->Width, sliderSprite->Height,
                gpWindowManager->screenBitmap,
                x + parentWindow->x + length - knob_start,
                y + parentWindow->y, 0);
        }

        sliderBitmap->Draw(
            0, 0, length - knob_start * 2, height,
            gpWindowManager->screenBitmap,
            x + parentWindow->x + knob_start, y + parentWindow->y, 0);
        sliderSprite->DrawInterface(
            4, 0, 0, sliderSprite->Width, sliderSprite->Height,
            gpWindowManager->screenBitmap,
            x + parentWindow->x + knobPos, y + parentWindow->y, 0);
    } else {
        if ((status & WIDGET_SELECTED) && clickY - y < knob_start) {
            sliderSprite->DrawInterface(
                1, 0, 0, sliderSprite->Width, sliderSprite->Height,
                gpWindowManager->screenBitmap,
                x + parentWindow->x, y + parentWindow->y, 0);
        } else {
            sliderSprite->DrawInterface(
                0, 0, 0, sliderSprite->Width, sliderSprite->Height,
                gpWindowManager->screenBitmap,
                x + parentWindow->x, y + parentWindow->y, 0);
        }

        if ((status & WIDGET_SELECTED)
            && clickY - y > length - knob_start) {
            sliderSprite->DrawInterface(
                3, 0, 0, sliderSprite->Width, sliderSprite->Height,
                gpWindowManager->screenBitmap,
                x + parentWindow->x,
                y + parentWindow->y + length - knob_start, 0);
        } else {
            sliderSprite->DrawInterface(
                2, 0, 0, sliderSprite->Width, sliderSprite->Height,
                gpWindowManager->screenBitmap,
                x + parentWindow->x,
                y + parentWindow->y + length - knob_start, 0);
        }

        sliderBitmap->Draw(
            0, 0, width, length - knob_start * 2,
            gpWindowManager->screenBitmap,
            x + parentWindow->x, y + parentWindow->y + knob_start, 0);
        sliderSprite->DrawInterface(
            4, 0, 0, sliderSprite->Width, sliderSprite->Height,
            gpWindowManager->screenBitmap,
            x + parentWindow->x, y + parentWindow->y + knobPos, 0);
    }
}

// E:\gamedcs\slider.cpp:706
VA(0x00597020, 0x84)  // contiguous slider block, dc 0x14aaa0
void slider::SetKnob(int inX)
{
    // Residual (88.44%): the CFG and the 64-instruction tail agree. In the
    // arithmetic head retail binds knobSize/base to ESI/EDI while this source
    // binds them to EDI/ESI. why-reg v2 (2026-08-11, --il-order) proves equal
    // definition slots but different C1 pseudo processing order and caps the
    // transposition as front-end handle state, not a statement-level knob.
    // 2026-08-14 adds the second half of the picture: retail evaluates the
    // offset as (-half) - base - knobSize (`neg edx` on the halved value),
    // while our CL REASSOCIATES every spelling into (-knobSize) - half - base
    // (`neg edi` on knobSize). Six spellings measured - plain declarations
    // 88.36, base declared first 87.70, non-compound assignment 88.44, a named
    // `half` local 88.44, else-if clamp 84.92, fully parenthesised negation
    // 88.44 - so the reassociation is a C2 choice with no source handle. Retail
    // also re-tests with `test eax,eax; jge` where our CL folds the clamp into
    // the `add`'s flags with `jns`; no spelling separated them.
    int knobSize;
    int base = (knobSize = knob_start,
                width > height ? static_cast<int>(x) : static_cast<int>(y));
    inX += -(knobSize / 2) - base - knobSize;
    if (inX < 0)
        inX = 0;
    if (inX > knobRange)
        inX = knobRange;

    int maximum = numStates - 1;
    currentState = (maximum * inX + knobRange / 2) / knobRange;
    if (numStates > 1)
        knobPos = knob_start + currentState * knobRange / maximum;
    else
        knobPos = knob_start;
}

// E:\gamedcs\slider.cpp:726
VA(0x005970B0, 0x35)  // virtual SetState dispatch, dc 0x14ab4c
void slider::UpdateResolution(int num)
{
    if (num != numStates) {
        if (num > 0)
            numStates = num;
        else
            numStates = 1;
        SetState(currentState);
    }
}

// E:\gamedcs\slider.cpp:739
VA(0x005970F0, 0x2A)  // contiguous slider block, dc 0x14ab7c
void slider::SetResolution(int num)
{
    knobPos = knob_start;
    oldState = 0;
    currentState = 0;
    if (num > 0)
        numStates = num;
    else
        numStates = 1;
}

// E:\gamedcs\slider.cpp:751
VA(0x00597120, 0x5)  // one-store body, dc 0x14aba0
void slider::OnSetFocus()
{
    scrolling = 1;
}

// E:\gamedcs\slider.cpp:756
VA(0x00597130, 0x5)  // one-store body, dc 0x14aba8
void slider::OnKillFocus()
{
    scrolling = 0;
}

// E:\gamedcs\slider.cpp:761
VA(0x00597140, 0x45)  // two status-message pairs, dc 0x14abb0
void slider::enable(unsigned char arg)
{
    if (arg) {
        send_message(WIDGET_CLEAR_STATUS, WIDGET_DISABLED);
        send_message(WIDGET_CLEAR_STATUS, WIDGET_STYLE_AUTO_REPEAT);
    } else {
        send_message(WIDGET_SET_STATUS, WIDGET_DISABLED);
        send_message(WIDGET_SET_STATUS, WIDGET_STYLE_AUTO_REPEAT);
    }
}
