// button.cpp - E:\gamedcs\button.cpp (compiland button.obj)
// 36 functions in link order.
#include <va.h>
#include "button.h"
#include "resourcemanager.h"
#include "window.h"
#include "winmgr.h"
#include "bitmap16.h"
#include "soundmgr.h"
#include "sample.h"
#include "message.h"
#include "kb.h"
#include "mousemgr.h"
#include "inputmgr.h"
#include "kbwin.h"
#include "palette.h"

// Thunk-form timeGetTime (an E8 rel32, not an IAT indirect - Select's
// match proves the form); the plain declaration and the per-TU
// import-form doctrine live in winmm_thunks.h.
#include "winmm_thunks.h"

// homm2 BUTTON.cpp's file-static modifier latch, same name and role.
DATA(0x00694da8)
static int g_leftRightSave;

// Unimplemented carcass stubs stay lexically present (labels and
// the va-claims gate scan text) but outside compilation.
// E:\gamedcs\button.cpp:43
// No claim: retail dropped the standalone copy (OPT:REF), but the body
// survives inlined at the head of the textButton ctor 0x456a50.
button::button()
    : widget(0, 0, 0, 0, 0, 0)
{
    m_buttonIcon = 0;
    m_normalFrame = 0;
    m_selectedFrame = 0;
    m_disabledFrame = 2;
    m_highlightedFrame = 3;
    m_endDialog = 0;
}

VA_COMPGEN(0x00455ec0, 0x21, SCALAR_DELETING_DTOR, button)

VA(0x00455ef0, 0x1F7)  // dc 0x57130
button::button(int x, int y, int w, int h, int id, const char* image, int normal, int selected, unsigned char end, int hotkey, int style)
    : widget(x, y, w, h, id, style)
{
    m_disabledFrame = 2;
    m_normalFrame = normal;
    m_selectedFrame = selected;
    m_highlightedFrame = 3;
    m_endDialog = end;
    m_hotKeyCodes.push_back(hotkey);
    m_buttonIcon = ResourceManager::getSprite(image);
}

#if 0  // @carcass

#endif  // @carcass

VA(0x004560f0, 0x9A)  // dc 0x571ec
inline button::~button()
{
    m_buttonIcon->dispose();
}

#if 0  // @carcass

// E:\gamedcs\button.cpp:104
DC_ONLY(0x57234, 0x32)
void button::setPalette(const char* palette_name)
{
    // @stub
}

#endif  // @carcass

// Dreamcast button.cpp:115-127 (dc 0x57268); textButton's constructor
// calls this ordinary helper at line 509. Retail 0x456a50 expands it,
// including the Complete-only highlighted-frame initialization.
void button::initialize(int x, int y, int w, int h, int id, const char* image, int normal, int selected, unsigned char end, int hotkey, int style)
{
    widget::initialize(x, y, w, h, id, style);
    m_normalFrame = normal;
    m_selectedFrame = selected;
    m_disabledFrame = 2;
    m_highlightedFrame = 3;
    m_endDialog = end;
    setHotkey(hotkey);
    m_buttonIcon = ResourceManager::getSprite(image);
}

// homm2's inline DeselectSelected survives with the endDialog variant;
// /Ob2 expands it at all four Main sites and emits no standalone copy
// (the `inline` keyword keeps it out of the object, matching retail).
inline int button::deselectSelected(message* msg)
{
    if (!(m_status & WIDGET_SELECTED))
        return 0;
    m_status &= ~WIDGET_SELECTED;
    draw();
    g_windowManager->updateScreen(m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
    msg->m_id = MESSAGE_WIDGET;
    msg->m_codeY = m_id;
    if (m_endDialog == 1)
        msg->m_codeX = widget::WIDGET_END_DIALOG;
    else
        msg->m_codeX = widget::WIDGET_DESELECT;
    msg->m_qualifier = g_leftRightSave;
    g_leftRightSave = 0;
    return 2;
}

// E:\gamedcs\button.cpp:131
// Residual (88.1%, was 67.4 on 2026-08-08): two DUP-EXIT defects are
// GONE. (1) The capture loop's two exits now BREAK to one shared
// `return DeselectSelected(msg)`; spelling either as its own `return`
// expands the inlined deselect body twice (+8.4 points). (2) The
// WIDGET sub-switch case ORDER is retail's emission order -
// SET_PALETTE, SET_ICON_NAME, SET_TEXT, SET_PLAYER_PALETTE_COLORS -
// not ascending by value (+8.9 points); retail's jump table lands the
// palette body first and cross-jumps SET_ICON_NAME onto the
// palette-failure tail. Spelling the palette case positively likewise
// restores retail's success-first layout (+2.75 points).
// CORRECTED 2026-08-08 (closeout lane): the auto-repeat guard calls
// GameTime::Get(), NOT timeGetTime(). The delinked target reads
// `call ?Get@GameTime@@SIKXZ` at +0x37 where this TU was emitting the
// winmm thunk; kbwin's GameTime::Get (0x4f82e0) is a 6-byte
// `jmp [__imp__timeGetTime@0]`, so the two are observationally equal
// at runtime but a different callee in the object. button::Select's
// timeGetTime at +0x188 stays the thunk form - the per-TU import-form
// note below still applies to THAT call, not to this one.
// DC-census verdict (2026-08-14), both rows negative: `GameTime::ElapsedSince`
// x1 (E:\gamedcs\struct.h:411, dc 0x1eed4 = `Elapsed(Get(), t)`) against the
// `(int)(GameTime::Get() - repeatTime) > 0` below is byte-EXACTLY flat at
// 88.1009 modelled file-locally - it folds. And `combatManager::CombatIsOver`
// x3 is pure Dreamcast platform delta: the DC census records it from
// button::Select x1 and button::Deselect x1 as well, and both of those are
// already EXACT here without it, so the DC widget pump polls combat-over where
// retail does not. slider::Main carries the identical ElapsedSince row with
// the identical verdict.
// What is left is (a) a whole-body esi/edi role swap (retail pins msg
// in ESI) that source order does not steer, and (b) the /Ob2 boundary
// inside button::SetText. `Text = new_text` recovers retail's strlen and
// raw movsd/movsb tail, but this compile expands std::string::_Grow while
// retail calls its two-argument COMDAT at 0x404a90. The same header body
// makes the opposite inline-boundary choice in the textButton ctor, so
// this is translation-unit optimizer state rather than a text-layout gap.
// `inline_depth(1..4)` and scoped auto_inline did not move the decision.
// The left-click capture loop pumps the mouse manager and input queue
// until a button-up, toggling selection as the pointer crosses the
// widget - homm2's shape with h3's GetEvent-by-value copy. The WIDGET
// sub-switch inlines SetText and the palette swap; a failed palette
// load falls back to reloading the icon sprite by the same name.
// E:\gamedcs\button.cpp:131
VA(0x00456190, 0x6CF)  // linkorder bracket; Select/widget-Main/manager callees byte-proven, dc 0x572d0
int button::main(message& msg)
{
    if (m_style == WIDGET_STYLE_AUTO_REPEAT && (m_status & WIDGET_SELECTED)) {
        unsigned long repeatTime = g_timers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT];
        if (static_cast<int>(GameTime::get() - repeatTime) > 0)
            return deselectSelected(&msg);
    }
    if (m_sleepCount > 0)
        return 0;
    if (!(m_status & WIDGET_ACTIVE)) {
        if (msg.m_id != MESSAGE_WIDGET)
            return 0;
        return widget::main(msg);
    }
    unsigned char isDisabled = 0;
    if (m_status & WIDGET_DISABLED)
        isDisabled = 1;
    switch (msg.m_id) {
    case MESSAGE_KEY_DOWN: {
        if (isDisabled)
            return 0;
        if (!(m_status & WIDGET_DRAWN))
            break;
        if (m_status & WIDGET_DIMMED)
            break;
        for (unsigned int key = 0; key < m_hotKeyCodes.size(); key++) {
            if (m_hotKeyCodes[key] == msg.m_codeX)
                return select(&msg);
        }
        return 0;
    }
    case MESSAGE_KEY_UP: {
        if (isDisabled)
            return 0;
        if (!(m_status & WIDGET_DRAWN))
            break;
        if (m_status & WIDGET_DIMMED)
            break;
        for (unsigned int key = 0; key < m_hotKeyCodes.size(); key++) {
            if (m_hotKeyCodes[key] == msg.m_codeX)
                return deselectSelected(&msg);
        }
        return 0;
    }
    case MESSAGE_LEFT_BUTTON_DOWN: {
        if (isDisabled)
            return 0;
        if (!(m_status & WIDGET_DRAWN))
            break;
        short mouseX = msg.m_codeX - m_parentWindow->m_x;
        short mouseY = msg.m_codeY - m_parentWindow->m_y;
        if (m_status & WIDGET_DIMMED)
            return 0;
        if (mouseX < m_x || mouseY < m_y || mouseX >= m_x + m_width
            || mouseY >= m_y + m_height)
            return 0;
        select(&msg);
        // Both exits BREAK to one shared `return DeselectSelected(msg)`
        // below: DeselectSelected is /Ob2-inlined, so spelling either
        // exit as its own `return` expands the whole 139-byte deselect
        // body twice where retail expands it once (67.38 -> ...).
        for (;;) {
            if (msg.m_id == MESSAGE_LEFT_BUTTON_UP
                || msg.m_id == MESSAGE_RIGHT_BUTTON_UP)
                break;
            g_mouseManager->main(msg);
            if (msg.m_id == MESSAGE_MOUSE_MOVE) {
                short moveX = msg.m_codeX - m_parentWindow->m_x;
                short moveY = msg.m_codeY - m_parentWindow->m_y;
                if (moveX >= m_x && moveY >= m_y && moveX < m_x + m_width
                    && moveY < m_y + m_height) {
                    if (!(m_status & WIDGET_SELECTED))
                        select(&msg);
                } else {
                    deselectSelected(&msg);
                }
            }
            process1WindowsMessage();
            pollSound();
            msg = g_inputManager->getEvent();
            if (msg.m_id == MESSAGE_LEFT_BUTTON_UP)
                break;
        }
        return deselectSelected(&msg);
    }
    case MESSAGE_LEFT_BUTTON_UP: {
        if (isDisabled)
            return 0;
        if (!(m_status & WIDGET_DRAWN))
            break;
        if (!(m_status & WIDGET_SELECTED))
            break;
        return deselectSelected(&msg);
    }
    case MESSAGE_RIGHT_BUTTON_DOWN:
        break;
    case MESSAGE_WIDGET: {
        if (msg.m_codeY != m_id)
            break;
        switch (msg.m_codeX) {
        case widget::WIDGET_SET_PALETTE: {
            TPalette16* newPalette = ResourceManager::getPalette(msg.m_extraText);
            if (newPalette) {
                m_buttonIcon->setPalette(newPalette->m_data);
                newPalette->dispose();
                return 1;
            }
            if (m_buttonIcon)
                m_buttonIcon->dispose();
            m_buttonIcon = ResourceManager::getSprite(msg.m_extraText);
            return 1;
        }
        case widget::WIDGET_SET_ICON_NAME:
            m_buttonIcon = ResourceManager::getSprite(msg.m_extraText);
            return 1;
        case widget::WIDGET_SET_TEXT:
            setText(msg.m_extraText);
            return 1;
        case widget::WIDGET_SET_PLAYER_PALETTE_COLORS:
            setPlayerPaletteColors(msg.m_extra);
            return 1;
        }
        break;
    }
    default:
        if (isDisabled)
            return 0;
        break;
    }
    if (!(m_status & WIDGET_DRAWN))
        return widget::main(msg);
    short rightX = msg.m_codeX - m_parentWindow->m_x;
    short rightY = msg.m_codeY - m_parentWindow->m_y;
    if (rightX < m_x || rightY < m_y || rightX >= m_x + m_width
        || rightY >= m_y + m_height)
        return 0;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_RIGHT_SELECT;
    msg.m_codeY = m_id;
    msg.m_qualifier = MESSAGE_MODIFIER_RIGHT;
    return 2;
}

#if 0  // @carcass

#endif  // @carcass

VA(0x00456860, 0xDA)  // dc 0x57730
int button::select(message* msg)
{
    m_status |= WIDGET_SELECTED;
    if (s_clickSample) {
        int saved = g_soundManager->m_playSounds;
        g_soundManager->m_playSounds = 1;
        s_clickSample->m_memSample.m_memVolume = 0x40;
        s_clickSample->m_memSample.m_memLooping = 1;
        s_clickSample->m_memSample.m_memCindex = 3;
        g_soundManager->memorySample(s_clickSample);
        g_soundManager->m_playSounds = saved;
    }
    draw();
    g_windowManager->updateScreen(m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
    msg->m_id = MESSAGE_WIDGET;
    msg->m_codeX = widget::WIDGET_SELECT;
    msg->m_codeY = m_id;
    g_timers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT] = timeGetTime() + BUTTON_REPEAT_DELAY_TICKS;
    g_leftRightSave = msg->m_qualifier & 0x300;
    return 2;
}

#if 0  // @carcass

// E:\gamedcs\button.cpp:401
DC_ONLY(0x57854, 0xBC)
int button::deselect(message* msg)
{
    // @stub
}

// E:\gamedcs\button.cpp:429
DC_ONLY(0x57910, 0x12)
int button::getRealWidth()
{
    // @stub
}

// E:\gamedcs\button.cpp:435
DC_ONLY(0x57924, 0x12)
int button::getRealHeight()
{
    // @stub
}

// E:\gamedcs\button.cpp:441
DC_ONLY(0x57938, 0x4)
void button::zBufferDraw()
{
    // @stub
}

#endif  // @carcass

VA(0x00456940, 0x99)  // dc 0x5793c
void button::draw()
{
    if (!(m_status & WIDGET_DRAWN))
        return;
    int frame = m_normalFrame;
    int frameCount = m_buttonIcon->getNumFrames(0);
    if ((m_status & WIDGET_HIGHLIGHTED) && !(m_status & WIDGET_SELECTED)) {
        frame = m_highlightedFrame;
    } else if (!(m_status & (WIDGET_DIMMED | WIDGET_DISABLED))) {
        if (m_status & WIDGET_SELECTED)
            frame = m_selectedFrame;
    } else {
        frame = m_disabledFrame;
    }
    if (frame >= frameCount)
        frame = 0;
    m_buttonIcon->drawInterface(frame, 0, 0, m_buttonIcon->getWidth(), m_buttonIcon->getHeight(),
                              g_windowManager->m_screenBitmap->getMap(0, 0),
                              m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y,
                              g_windowManager->m_screenBitmap->getWidth(),
                              g_windowManager->m_screenBitmap->getHeight(),
                              g_windowManager->m_screenBitmap->getPitch(), 0);
}

#if 0  // @carcass

// E:\gamedcs\button.cpp:471
DC_ONLY(0x57a24, 0x4)
void button::dim()
{
    // @stub
}

#endif  // @carcass

VA(0x004569e0, 0x2E)  // dc 0x57a28
void button::setPlayerPaletteColors(int whichPlayer)
{
    ::setPlayerPaletteColors(m_buttonIcon->getPalette(), whichPlayer);
    ::setPlayerPaletteColors(m_buttonIcon->m_p24, whichPlayer);
}

VA(0x00456a10, 0x10)
void button::vslot12(int on)
{
    widget::vslot12(on);
}

#if 0  // @carcass

// E:\gamedcs\button.cpp:487
DC_ONLY(0x57a5c, 0x58)
void textButton::textButton()
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x00456a20, 0x21, SCALAR_DELETING_DTOR, textButton)

// E:\gamedcs\button.cpp:508
// Residual (88.4%): retail latches the hotkey argument back into its
// own stack slot before the initialize call (the by-ref insert copy
// scheduled early) and the EH-handler reloc addend spells differently
// across the sides; every callee and store agrees.

// THE `#0 jbe -> jae` LEAD IS CLOSED, and it was never a condition. The
// guard is `basic_string::assign`'s length check. Retail computes the limit
// with a CALL - 0x404bc0, six bytes, `mov eax,0xfffffffd; ret`, i.e.
// max_size() - and compares `cmp eax,edi; jae`; this compile expands that
// callee and constant-folds it to `cmp edi,-3; jbe`. Same predicate, one
// side folded. The second real divergence is the same class: retail calls
// 0x404a70 (`_Eos`: `[ecx+8]=n; [ecx+4][n]=0`, 20 bytes) where we expand it
// inline. Both are A8/A9 depth stops inside SetText's nested expansion, so
// this row belongs to the INLINER, not to why-branch - the v1 mutation
// library has no site class for it because there is no source site.
// Everything else predict-inline reports here is the UNDER/OVER
// name-pairing artefact: `button_set_hotkey` is the delinker's label for
// 0x404200, which is the 521-byte three-argument vector<int>::insert this
// file's own note already identifies, and it pairs with our
// `?insert@?$vector@H...` - the same callee under two names, as do
// GetSprite and GetFont.

// The site-count lever has NO working setting here: appending free
// candidate sites to the body takes the row 88.4400 (+0) -> 58.3333 (+1,
// +2, +3, +4) -> 59.1800 (+6, +8). One site already starves SetText's whole
// expansion; there is no intermediate that keeps SetText expanded while
// stopping it above max_size/_Eos.

// Builds on the inlined button() default, then initializes through
// button::initialize - the DC shape, not a delegation to the eleven-arg
// button ctor.
VA(0x00456a50, 0x193)  // linkorder bracket; initialize/GetSprite/GetFont callees byte-proven, dc 0x57ab4
textButton::textButton(int x, int y, int w, int h, int id, const char* image, const char* text, const char* fontName, int normal, int selected, unsigned char end, int hotkey, int style, int newColor)
    : button()
{
    initialize(x, y, w, h, id, image, normal, selected, end, hotkey, style);
    setText(text);
    m_font = ResourceManager::getFont(fontName);
    m_textColor = newColor;
}

VA(0x00456bf0, 0xAB)  // dc 0x57b5c
textButton::~textButton()
{
    m_font->dispose();
}

#if 0  // @carcass

#endif  // @carcass

VA(0x00456ca0, 0x82)  // dc 0x57b98
void textButton::draw()
{
    if (!(m_status & WIDGET_DRAWN))
        return;
    button::draw();
    short buttonStatus = m_status;
    int color = m_textColor;
    if (buttonStatus & (WIDGET_DIMMED | WIDGET_DISABLED))
        color += 2;
    heroWindow* parent = m_parentWindow;
    int drawY;
    if (buttonStatus & WIDGET_SELECTED)
        drawY = m_y + parent->m_y;
    else
        drawY = m_y + parent->m_y - 1;
    int drawX;
    if (buttonStatus & WIDGET_SELECTED)
        drawX = m_x + parent->m_x + 1;
    else
        drawX = m_x + parent->m_x;
    m_font->drawBoundedString(m_text.c_str(), g_windowManager->m_screenBitmap,
                            drawX, drawY, m_width, m_height, color, 5, -1);
}

#if 0  // @carcass

#endif  // @carcass

VA(0x00456d30, 0x46)  // dc 0x57c4c
type_func_button::type_func_button(long x, long y, long w, long h, long id,
                                   const char* image,
                                   handler_type newHandler,
                                   int normal, int selected)
    : button(x, y, w, h, id, image, normal, selected, 0, 0, 2)
{
    m_handler = newHandler;
}

#if 0  // @carcass

// E:\gamedcs\button.cpp:565
DC_ONLY(0x57cdc, 0x6A)
void type_func_button::type_func_button(const type_icon_definition* def, int _id, int (*)()* _handler)
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x00456d80, 0x21, SCALAR_DELETING_DTOR, type_func_button)

VA(0x00456db0, 0x9A)  // dc 0x57e7c
type_func_button::~type_func_button()
{
}

VA(0x00456e50, 0x44)  // dc 0x57d48
int type_func_button::main(message& msg)
{
    int result = button::main(msg);
    if (result != 1 && (m_status & WIDGET_ACTIVE) && m_sleepCount <= 0
        && msg.m_id == MESSAGE_WIDGET && msg.m_codeY == m_id) {
        msg.m_window = m_parentWindow;
        return m_handler(msg);
    }
    return result;
}

#if 0  // @carcass

// E:\gamedcs\Button.h:78
DC_ONLY(0x57da4, 0x18)
void button::setText(const char* new_text)
{
    // @stub
}

// E:\gamedcs\CSprite.h:284
DC_ONLY(0x57dbc, 0x24)
TPalette24* CSprite::getPalette24()
{
    // @stub
}

// E:\gamedcs\button.cpp:51
DC_ONLY(0x57de0, 0x34)
void* button::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\button.cpp:488
DC_ONLY(0x57e14, 0x34)
void* textButton::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\button.cpp:559
DC_ONLY(0x57e48, 0x34)
void* type_func_button::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass

#if 0  // @carcass

// ..\stlport\stl_vector.h:203
DC_ONLY(0x57e94, 0x20)
int* std::vector<int,std::allocator<int> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x57eb4, 0x1C)
void std::vector<int,std::allocator<int> >::vector<int,std::allocator<int> >(const std::allocator<int>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x57ed0, 0x28)
void std::vector<int,std::allocator<int> >::~vector<int,std::allocator<int> >()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x57ef8, 0x4)
void std::allocator<int>::allocator<int>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x57efc, 0x4)
void std::allocator<int>::~allocator<int>()
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x57f00, 0x2C)
void std::_Vector_base<int,std::allocator<int> >::_Vector_base<int,std::allocator<int> >(const std::allocator<int>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x57f2c, 0x30)
void std::_Vector_base<int,std::allocator<int> >::~_Vector_base<int,std::allocator<int> >()
{
    // @stub
}

// ..\stlport\stl_string.h:101
DC_ONLY(0x57f5c, 0x18)
void std::_STL_alloc_proxy<int *,int,std::allocator<int> >::~_STL_alloc_proxy<int *,int,std::allocator<int> >()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x57f74, 0xC)
void std::_STL_alloc_proxy<int *,int,std::allocator<int> >::_STL_alloc_proxy<int *,int,std::allocator<int> >(const std::allocator<int>* __a, int** __p)
{
    // @stub
}

#endif  // @carcass

DATA(0x00694da4)
sample* button::s_clickSample;
