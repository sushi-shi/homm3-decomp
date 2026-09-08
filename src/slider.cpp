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
// Before normalization: iLeftRightSave.
DATA(0x0069fdd4)
static int g_leftRightSave;

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
// Before normalization (locals): resource_name.
VA(0x00596050, 0x7D)  // literal/callee/field-layout proof, dc 0x149a48
void slider::initialize(const char* resourceName)
{
    if (m_width > m_height) {
        m_length = m_width;
        m_sliderSprite = ResourceManager::getSprite(resourceName);
        m_sliderBitmap = ResourceManager::getBitmap816(
            DATA_COMPGEN(0x00683980, sliderHorizontalBitmap, "slider.pcx"));
        m_knobStart = m_sliderSprite->m_width;
    } else {
        m_length = m_height;
        m_sliderSprite = ResourceManager::getSprite(resourceName);
        m_sliderBitmap = ResourceManager::getBitmap816(
            DATA_COMPGEN(0x00683974, sliderVerticalBitmap, "sliderV.pcx"));
        m_knobStart = m_sliderSprite->m_height;
    }

    m_knobPos = m_knobStart;
    m_knobRange = m_length - m_knobPos * 3;
    m_oldState = 0;
    m_currentState = 0;
}

// E:\gamedcs\slider.cpp:96
VA(0x005960D0, 0xA8)  // ctor/EH/vtable/literal proof, dc 0x149ae8
slider::slider(int x, int y, int w, int h, int id, int num,
               TSliderFunction func, EGraphics graphics, int page,
               unsigned char hotKey)
    : widget(x, y, w, h, id, 1)
{
    m_pageSize = page;
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
    m_sliderFunction = func;
    m_numStates = num;
    m_hotKeys = hotKey;
}

// E:\gamedcs\slider.cpp:124
VA(0x00596180, 0x59)  // vtable/resource Dispose/base dtor, dc 0x149ba4
slider::~slider()
{
    m_sliderBitmap->dispose();
    m_sliderSprite->dispose();
}

// E:\gamedcs\slider.cpp:143
VA(0x005961E0, 0x4C)  // contiguous slider block, dc 0x149bec
void slider::setState(int state)
{
    if (state < 0)
        state = 0;
    else if (state >= m_numStates)
        state = m_numStates - 1;

    m_oldState = m_currentState;
    m_currentState = state;
    if (m_numStates <= 1)
        m_knobPos = m_knobStart;
    else
        m_knobPos = m_knobStart + m_knobRange * state / (m_numStates - 1);
}

// E:\gamedcs\slider.cpp:159
VA(0x00596230, 0x2A2)  // contiguous slider block, dc 0x149c90
void slider::keyAccel(int x1, int x2, int x3, int x4, int key)
{
    m_status |= WIDGET_SELECTED;
    m_sliderSprite->drawInterface(
        x1, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
        g_windowManager->m_screenBitmap,
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y,
        0);
    int endY = m_parentWindow->m_y + m_y;
    endY -= m_knobStart;
    endY += m_length;
    m_sliderSprite->drawInterface(
        x2, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
        g_windowManager->m_screenBitmap,
        m_x + m_parentWindow->m_x,
        endY,
        0);
    m_sliderBitmap->draw(
        x3, 0, m_width, m_length - m_knobStart * 2,
        g_windowManager->m_screenBitmap,
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y + m_knobStart, 0);
    m_sliderSprite->drawInterface(
        x4, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
        g_windowManager->m_screenBitmap,
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y + m_knobPos,
        0);
    g_windowManager->updateScreen(
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
    g_timers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT] = GameTime::get() + 60;

    if (m_oldState != m_currentState) {
        m_oldState = m_currentState;
        close();
        if (m_sliderFunction)
            m_sliderFunction(m_currentState, m_parentWindow);
    }

    switch (key) {
    case KEYCODE_KP_9:
        setState(m_currentState - m_pageSize);
        break;
    case KEYCODE_KP_3:
        setState(m_currentState + m_pageSize);
        break;
    case KEYCODE_KP_8:
        --m_currentState;
        m_knobPos = m_knobRange * m_currentState / (m_numStates - 1) + m_knobStart;
        break;
    case KEYCODE_KP_2:
        ++m_currentState;
        m_knobPos = m_knobRange * m_currentState / (m_numStates - 1) + m_knobStart;
        break;
    }

    m_status &= ~WIDGET_SELECTED;
    draw();
    unsigned long repeatTime = g_timers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT];
    while (static_cast<int>(
               GameTime::get() - repeatTime) <= 0) {
        pollSound();
        process1WindowsMessage();
        repeatTime = g_timers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT];
    }
    g_windowManager->updateScreen(
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);

    if (m_oldState != m_currentState) {
        m_oldState = m_currentState;
        close();
        if (m_sliderFunction)
            m_sliderFunction(m_currentState, m_parentWindow);
    }
}

// E:\gamedcs\slider.cpp:244
// Retail's direct 0x48..0x51 jump table proves that KP4 shares KP8's
// decrement arm and KP6 shares KP2's increment arm. Letting all four action
// arms reach the common zero return, and making the state-message arm join
// its common success tail, reproduces all 85 blocks, 60 branches, 11 returns,
// and ten jump-table slots exactly.
VA(0x005964E0, 0x4A0)  // contiguous slider block, dc 0x149f04
int slider::main(message* msg)
{
    if (m_style == WIDGET_STYLE_AUTO_REPEAT && (m_status & WIDGET_SELECTED)) {
        unsigned long repeatTime = g_timers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT];
        if (static_cast<int>(GameTime::get() - repeatTime) > 0)
            return deselect(msg);
    }

    if (!(m_status & WIDGET_ACTIVE)) {
        if (msg->m_id != MESSAGE_WIDGET)
            return 0;
        return widget::main(msg);
    }

    unsigned char isDisabled = 0;
    if (m_status & WIDGET_DISABLED)
        isDisabled = 1;

    switch (msg->m_id) {
    case MESSAGE_KEY_UP:
        if (isDisabled)
            return 0;
        if (!(m_status & WIDGET_DRAWN) || (m_status & WIDGET_DIMMED))
            goto callWidgetMain;
        if (!m_hotKeys)
            return 0;
        switch (msg->m_codeX) {
        case KEYCODE_KP_9:
            keyAccel(1, 2, 0, 4, KEYCODE_KP_9);
            break;
        case KEYCODE_KP_3:
            keyAccel(0, 3, 0, 4, KEYCODE_KP_3);
            break;
        case KEYCODE_KP_4:
        case KEYCODE_KP_8:
            if (m_currentState > 0)
                keyAccel(1, 2, 0, 4, KEYCODE_KP_8);
            break;
        case KEYCODE_KP_6:
        case KEYCODE_KP_2:
            if (m_currentState < m_numStates - 1)
                keyAccel(0, 3, 0, 4, KEYCODE_KP_2);
            break;
        }
        return 0;

    case MESSAGE_LEFT_BUTTON_DOWN:
        if (!(m_status & WIDGET_DRAWN))
            goto callWidgetMain;
        if (isDisabled)
            return 0;
        m_clickX = msg->m_codeX - m_parentWindow->m_x;
        m_clickY = msg->m_codeY - m_parentWindow->m_y;
        if (m_status & WIDGET_DIMMED)
            return 0;
        if (m_clickX < m_x || m_clickY < m_y || m_clickX >= m_x + m_width
            || m_clickY >= m_y + m_height)
            return 0;

        select(msg, 0);
        for (;;) {
            if (msg->m_id == MESSAGE_LEFT_BUTTON_UP
                || msg->m_id == MESSAGE_RIGHT_BUTTON_UP)
                break;
            pollSound();
            g_mouseManager->main(*msg);
            if (msg->m_id == MESSAGE_MOUSE_MOVE) {
                m_clickX = msg->m_codeX - m_parentWindow->m_x;
                m_clickY = msg->m_codeY - m_parentWindow->m_y;
                if (m_width > m_height) {
                    int distance = m_y - m_clickY;
                    if (distance < 40 && distance > 0)
                        m_clickY = m_y;
                    distance = m_clickY - m_y - m_height;
                    if (distance < 40 && distance > 0)
                        m_clickY = m_y;
                    if (m_clickX >= m_x + m_knobStart
                        && m_clickX < m_x + m_width - m_knobStart
                        && m_clickY >= m_y && m_clickY < m_y + m_height)
                        select(msg, 1);
                } else {
                    int distance = m_x - m_clickX;
                    if (distance < 40 && distance > 0)
                        m_clickX = m_x;
                    distance = m_clickX - m_x - m_width;
                    if (distance < 40 && distance > 0)
                        m_clickX = m_x;
                    if (m_clickX >= m_x && m_clickY >= m_y + m_knobStart
                        && m_clickX < m_x + m_width
                        && m_clickY < m_y + m_height - m_knobStart)
                        select(msg, 1);
                }
            }
            process1WindowsMessage();
            *msg = g_inputManager->getEvent();
            if (msg->m_id == MESSAGE_LEFT_BUTTON_UP)
                break;
        }
        if (m_status & WIDGET_SELECTED) {
            deselect(msg);
            return 2;
        }
        return 1;

    case MESSAGE_LEFT_BUTTON_UP:
        if (isDisabled)
            return 0;
        if (!(m_status & WIDGET_DRAWN) || !(m_status & WIDGET_SELECTED))
            goto callWidgetMain;
        return deselect(msg);

    case MESSAGE_RIGHT_BUTTON_DOWN:
        if (!(m_status & WIDGET_DRAWN))
            goto callWidgetMain;
        m_clickX = msg->m_codeX - m_parentWindow->m_x;
        m_clickY = msg->m_codeY - m_parentWindow->m_y;
        if (m_clickX < m_x || m_clickY < m_y || m_clickX >= m_x + m_width
            || m_clickY >= m_y + m_height)
            return 0;
        msg->m_id = MESSAGE_WIDGET;
        msg->m_codeX = WIDGET_RIGHT_SELECT;
        msg->m_codeY = m_id;
        msg->m_qualifier = MESSAGE_MODIFIER_RIGHT;
        return 2;

    case MESSAGE_WIDGET:
        switch (msg->m_codeX) {
        case WIDGET_SET_SLIDER_STATE:
            if (msg->m_codeY == m_id) {
                setState(msg->m_extra);
                goto sliderMessageHandled;
            }
            break;
        case WIDGET_SET_SLIDER_RESOLUTION:
            if (msg->m_codeY == m_id) {
                setResolution(msg->m_extra);
                return 1;
            }
            break;
        }
        goto callWidgetMain;

    sliderMessageHandled:
        return 1;
    }

callWidgetMain:
    return widget::main(msg);
}

// E:\gamedcs\slider.cpp:477
VA(0x00596980, 0x167)  // contiguous slider block, dc 0x14a380
int slider::select(message* msg, unsigned char dragging)
{
    m_status |= WIDGET_SELECTED;

    if (m_width > m_height) {
        int click = m_clickX - m_x;
        if (click >= m_knobStart && click < m_length - m_knobStart) {
            if (m_pageSize > 0 && !dragging) {
                if (click < m_knobPos)
                    setState(m_currentState - m_pageSize);
                else if (click < m_knobPos + 16)
                    setKnob(m_clickX);
                else
                    setState(m_currentState + m_pageSize);
            } else {
                setKnob(m_clickX);
            }
        }
    } else {
        int click = m_clickY - m_y;
        if (click >= m_knobStart && click < m_length - m_knobStart) {
            if (m_pageSize > 0 && !dragging) {
                if (click < m_knobPos)
                    setState(m_currentState - m_pageSize);
                else if (click >= m_knobPos + m_knobStart)
                    setState(m_currentState + m_pageSize);
                else
                    setKnob(m_clickY);
            } else {
                setKnob(m_clickY);
            }
        }
    }

    draw();
    g_windowManager->updateScreen(
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
    msg->m_id = MESSAGE_WIDGET;
    msg->m_codeX = WIDGET_SELECT;
    msg->m_codeY = m_id;
    g_timers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT] = GameTime::get() + 60;
    g_leftRightSave = msg->m_qualifier & MESSAGE_MODIFIER_MASK;

    if (m_oldState != m_currentState) {
        m_oldState = m_currentState;
        close();
        if (m_sliderFunction)
            m_sliderFunction(m_currentState, m_parentWindow);
    }
    return 2;
}

// E:\gamedcs\slider.cpp:545
// EXACT 2026-08-11. Successful decrements jump to the common redraw exit;
// preserving that source edge releases the coordinate lifetime and lets C2
// share retail's knob arithmetic without an extra EBX save.
VA(0x00596AF0, 0x143)  // contiguous slider block, dc 0x14a508
int slider::deselect(message* msg)
{
    if (!(m_status & WIDGET_SELECTED))
        return 0;
    m_status &= ~WIDGET_SELECTED;

    if (m_width > m_height) {
        if (m_clickX - m_x < m_knobStart && m_currentState > 0) {
            --m_currentState;
            m_knobPos = m_knobRange * m_currentState / (m_numStates - 1)
                + m_knobStart;
            goto redraw;
        }
        if (m_clickX - m_x > m_length - m_knobStart
            && m_currentState < m_numStates - 1) {
            ++m_currentState;
            m_knobPos = m_knobRange * m_currentState / (m_numStates - 1)
                + m_knobStart;
        }
    } else {
        if (m_clickY - m_y < m_knobStart && m_currentState > 0) {
            --m_currentState;
            m_knobPos = m_knobRange * m_currentState / (m_numStates - 1)
                + m_knobStart;
            goto redraw;
        }
        if (m_clickY - m_y > m_length - m_knobStart
            && m_currentState < m_numStates - 1) {
            ++m_currentState;
            m_knobPos = m_knobRange * m_currentState / (m_numStates - 1)
                + m_knobStart;
        }
    }

redraw:
    draw();
    g_windowManager->updateScreen(
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
    msg->m_id = MESSAGE_WIDGET;
    msg->m_codeY = m_id;
    msg->m_codeX = WIDGET_DESELECT;
    msg->m_qualifier = g_leftRightSave;
    g_leftRightSave = 0;

    if (m_oldState != m_currentState) {
        m_oldState = m_currentState;
        close();
        if (m_sliderFunction)
            m_sliderFunction(m_currentState, m_parentWindow);
    }
    return 2;
}

// Retail folds these header-sized bodies into representatives owned by other
// units (vtable targets 0x4eab20/30 and 0x5bc7e0).
#if 0  // @carcass -- ICF/header COMDAT, no slider.obj home
DC_ONLY(0x14a67c, 0x16)
int slider::getRealWidth() const { /* @stub: DC calls the sprite GetWidth. */ }
DC_ONLY(0x14a694, 0x16)
int slider::getRealHeight() const { /* @stub: DC calls the sprite GetHeight. */ }
DC_ONLY(0x14a6ac, 0x4)
void slider::zBufferDraw() {}
#endif

// E:\gamedcs\slider.cpp:613
VA(0x00596C40, 0x3D5)  // contiguous slider block, dc 0x14a6b0
void slider::draw()
{
    if (m_width > m_height) {
        if ((m_status & WIDGET_SELECTED) && m_clickX - m_x < m_knobStart) {
            m_sliderSprite->drawInterface(
                1, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
                g_windowManager->m_screenBitmap,
                m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, 0);
        } else {
            m_sliderSprite->drawInterface(
                0, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
                g_windowManager->m_screenBitmap,
                m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, 0);
        }

        if ((m_status & WIDGET_SELECTED)
            && m_clickX - m_x > m_length - m_knobStart) {
            m_sliderSprite->drawInterface(
                3, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
                g_windowManager->m_screenBitmap,
                m_x + m_parentWindow->m_x + m_length - m_knobStart,
                m_y + m_parentWindow->m_y, 0);
        } else {
            m_sliderSprite->drawInterface(
                2, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
                g_windowManager->m_screenBitmap,
                m_x + m_parentWindow->m_x + m_length - m_knobStart,
                m_y + m_parentWindow->m_y, 0);
        }

        m_sliderBitmap->draw(
            0, 0, m_length - m_knobStart * 2, m_height,
            g_windowManager->m_screenBitmap,
            m_x + m_parentWindow->m_x + m_knobStart, m_y + m_parentWindow->m_y, 0);
        m_sliderSprite->drawInterface(
            4, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
            g_windowManager->m_screenBitmap,
            m_x + m_parentWindow->m_x + m_knobPos, m_y + m_parentWindow->m_y, 0);
    } else {
        if ((m_status & WIDGET_SELECTED) && m_clickY - m_y < m_knobStart) {
            m_sliderSprite->drawInterface(
                1, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
                g_windowManager->m_screenBitmap,
                m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, 0);
        } else {
            m_sliderSprite->drawInterface(
                0, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
                g_windowManager->m_screenBitmap,
                m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, 0);
        }

        if ((m_status & WIDGET_SELECTED)
            && m_clickY - m_y > m_length - m_knobStart) {
            m_sliderSprite->drawInterface(
                3, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
                g_windowManager->m_screenBitmap,
                m_x + m_parentWindow->m_x,
                m_y + m_parentWindow->m_y + m_length - m_knobStart, 0);
        } else {
            m_sliderSprite->drawInterface(
                2, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
                g_windowManager->m_screenBitmap,
                m_x + m_parentWindow->m_x,
                m_y + m_parentWindow->m_y + m_length - m_knobStart, 0);
        }

        m_sliderBitmap->draw(
            0, 0, m_width, m_length - m_knobStart * 2,
            g_windowManager->m_screenBitmap,
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y + m_knobStart, 0);
        m_sliderSprite->drawInterface(
            4, 0, 0, m_sliderSprite->m_width, m_sliderSprite->m_height,
            g_windowManager->m_screenBitmap,
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y + m_knobPos, 0);
    }
}

// E:\gamedcs\slider.cpp:706
VA(0x00597020, 0x84)  // contiguous slider block, dc 0x14aaa0
void slider::setKnob(int inX)
{
    // EXACT 2026-09-07 (88.44 -> 100.0). The old candidate collapsed the
    // width/height choice into a ternary fed by named `knobSize` and `base`
    // locals, then diagnosed the resulting ESI/EDI swap as a front-end wall.
    // Dreamcast line 707..710 instead proves two source arms and records no
    // locals. Repeating the member expression in those arms lets VC6 hoist
    // m_knobStart by itself; retail's register binding, subtraction order,
    // separate clamp test, and all ten blocks then match exactly.
    if (m_width > m_height)
        inX -= m_knobStart / 2 + m_x + m_knobStart;
    else
        inX -= m_knobStart / 2 + m_y + m_knobStart;
    if (inX < 0)
        inX = 0;
    if (inX > m_knobRange)
        inX = m_knobRange;

    int maximum = m_numStates - 1;
    m_currentState = (maximum * inX + m_knobRange / 2) / m_knobRange;
    if (m_numStates > 1)
        m_knobPos = m_knobStart + m_currentState * m_knobRange / maximum;
    else
        m_knobPos = m_knobStart;
}

// E:\gamedcs\slider.cpp:726
VA(0x005970B0, 0x35)  // virtual SetState dispatch, dc 0x14ab4c
void slider::updateResolution(int num)
{
    if (num != m_numStates) {
        if (num > 0)
            m_numStates = num;
        else
            m_numStates = 1;
        setState(m_currentState);
    }
}

// E:\gamedcs\slider.cpp:739
VA(0x005970F0, 0x2A)  // contiguous slider block, dc 0x14ab7c
void slider::setResolution(int num)
{
    m_knobPos = m_knobStart;
    m_oldState = 0;
    m_currentState = 0;
    if (num > 0)
        m_numStates = num;
    else
        m_numStates = 1;
}

// E:\gamedcs\slider.cpp:751
VA(0x00597120, 0x5)  // one-store body, dc 0x14aba0
void slider::onSetFocus()
{
    m_scrolling = 1;
}

// E:\gamedcs\slider.cpp:756
VA(0x00597130, 0x5)  // one-store body, dc 0x14aba8
void slider::onKillFocus()
{
    m_scrolling = 0;
}

// E:\gamedcs\slider.cpp:761
VA(0x00597140, 0x45)  // two status-message pairs, dc 0x14abb0
void slider::enable(unsigned char arg)
{
    if (arg) {
        sendMessage(WIDGET_CLEAR_STATUS, WIDGET_DISABLED);
        sendMessage(WIDGET_CLEAR_STATUS, WIDGET_STYLE_AUTO_REPEAT);
    } else {
        sendMessage(WIDGET_SET_STATUS, WIDGET_DISABLED);
        sendMessage(WIDGET_SET_STATUS, WIDGET_STYLE_AUTO_REPEAT);
    }
}
