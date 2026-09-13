// button.cpp - E:\gamedcs\button.cpp (compiland button.obj)
// 36 functions in link order.
//
// Function coverage closed: the sixteen retained button-family functions
// at 0x455ec0..0x456e94 are exact. The three scalar deleting destructors
// are included even though the generated game-VA span starts at 0x455ef0.
// Four other button virtuals fold to the exact iconWidget width/height
// and textWidget empty-hook representatives; their own source overrides
// remain here without duplicate RVA claims. All 36 header consumers retain
// their tracked scores when these interfaces are restored.
//
// Flanks: the preceding gap contains 61 native stream/facet routines,
// cinit135 at 0x455e60 and button's scalar destructor. After the button
// family, cinit136 at 0x456ea0 precedes ten Complete campaign-set bodies
// (0x456ec0..0x457594), followed by cinit137..147 before campaignbrief's
// 0x457990 Select. The admitted init-table entries and retained neighbors
// account for every gap; no unclaimed button-method slot remains.
//
// DC roster: 25 owning-cpp records resolve to fifteen retained bodies,
// four folded virtuals, four expanded helpers (default button constructor,
// SetPalette, initialize and Deselect), and two absent constructor overloads
// explained at their inactive records. Complete adds vslot12. The two
// header helpers expand; nine STLport records are the replaced library.
// Source-audit limits remain explicit: PC drops the focus parameter and
// the CombatIsOver pump work, and uses sprite Dispose instead of the older
// manager entry point. Destructor and callback-type audit coverage gaps
// are not claims of zero source differences.
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

// E:\gamedcs\button.cpp:104..111. Original name: SetPalette.
// Main:163 calls this ordinary helper. Retail expands its successful
// palette update and disposal, then returns on both success and failure.
// The icon reload/disposal belongs to Main's separate SET_ICON_NAME arm.
void button::setPalette(const char* paletteName)
{
    TPalette16* newPalette = ResourceManager::getPalette(paletteName);
    if (newPalette) {
        m_buttonIcon->setPalette(newPalette->m_data);
        newPalette->dispose();
    }
}

// E:\gamedcs\button.cpp:115..127. TextButton:509 calls this ordinary
// eleven-argument initializer. DC118..124 proves the disabled/normal/
// selected/end stores, direct vector push_back, then GetSprite; Complete
// additionally initializes its highlighted frame before selected. Five store-
// order controls preserve DC order: this placement closes TextButton at 100%,
// versus 99.2% after selected. All scored siblings retain their prior bytes.
// Keep the ordinary helper boundary.
void button::initialize(int x, int y, int w, int h, int id,
                        const char* image, int normal, int selected,
                        unsigned char end, int hotkey, int style)
{
    widget::initialize(x, y, w, h, id, style);
    m_disabledFrame = 2;
    m_normalFrame = normal;
    m_highlightedFrame = 3;
    m_selectedFrame = selected;
    m_endDialog = end;
    m_hotKeyCodes.push_back(hotkey);
    m_buttonIcon = ResourceManager::getSprite(image);
}

// E:\gamedcs\button.cpp:131..359. DC136 calls ElapsedSince; 158..176
// handles widget commands before keyboard/mouse arms and calls SetPalette.
// Retail's palette success/failure paths both return after the helper;
// SET_ICON_NAME separately disposes the old sprite before GetSprite. DC168
// calls ResourceManager::Dispose; retail uses the sprite virtual Dispose
// boundary (Main +0x580), reflecting Complete resource ownership. The
// earlier transcription wrongly reloaded the sprite on palette failure and
// omitted disposal on icon replacement, despite its old residual comment.
// Restore those source/behavior facts and the canonical initializer below.
// The corrected resource paths reach 99.7788% from 88.1009%. Retail B87
// tests DRAWN only for RIGHT_BUTTON_DOWN; all other unhandled paths enter
// B88 widget::Main directly. DC235..252 owns the right-hit-test scope and
// DC359 the final delegation. Restoring that case scope reaches 99.9734%;
// the former shared tail wrongly turned other events into right selections.
// DC329..335 and retail +0x2c1 separately test selection after capture:
// selected calls Deselect then returns 2; cancellation returns 1. Returning
// Deselect directly incorrectly returned 0 when capture ended outside.
//
// Preserve the shared capture-loop exit: separate Deselect statements
// expand the helper twice (the older 67.38% control). Select/Deselect retain
// retail's absence of DC's CombatIsOver polling; both are independently exact
// without those Dreamcast-only pump calls. The raw elapsed subtraction was
// byte-flat, but the documented GameTime helper belongs in the source.
// The final string-terminator SIB difference is sensitive to the actual
// keyboard index lifetime. One unsigned index, reinitialized in either key
// arm, makes the entire handler exact; separate case-local indices leave
// 99.9823%. Twenty-four caller-lifetime states reproduce ten retained
// candidates. Only this shared-index form (with or without a text argument
// local) reaches 100%; keep the direct SetText call and byte disabled flag.
// DC records the two searches at 195/199 and 216/220 but no named locals;
// sharing their index is a retail-tested source hypothesis, not a DC fact.
VA(0x00456190, 0x6CF)  // linkorder bracket; Select/widget-Main/manager callees byte-proven, dc 0x572d0
int button::main(message& msg)
{
    if (m_style == WIDGET_STYLE_AUTO_REPEAT && (m_status & WIDGET_SELECTED)) {
        unsigned long repeatTime = g_timers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT];
        if (GameTime::elapsedSince(repeatTime) > 0)
            return deselect(msg);
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
    unsigned int key;
    switch (msg.m_id) {
    case MESSAGE_WIDGET: {
        if (msg.m_codeY != m_id)
            break;
        switch (msg.m_codeX) {
        case widget::WIDGET_SET_PALETTE:
            setPalette(msg.m_extraText);
            return 1;
        case widget::WIDGET_SET_ICON_NAME:
            if (m_buttonIcon)
                m_buttonIcon->dispose();
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
    case MESSAGE_KEY_DOWN: {
        if (isDisabled)
            return 0;
        if (!(m_status & WIDGET_DRAWN))
            break;
        if (m_status & WIDGET_DIMMED)
            break;
        for (key = 0; key < m_hotKeyCodes.size(); key++) {
            if (m_hotKeyCodes[key] == msg.m_codeX)
                return select(msg);
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
        for (key = 0; key < m_hotKeyCodes.size(); key++) {
            if (m_hotKeyCodes[key] == msg.m_codeX)
                return deselect(msg);
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
        select(msg);
        // Both exits BREAK to one shared selection test and Deselect
        // below: Deselect is /Ob2-inlined, so spelling either
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
                        select(msg);
                } else {
                    deselect(msg);
                }
            }
            process1WindowsMessage();
            pollSound();
            msg = g_inputManager->getEvent();
            if (msg.m_id == MESSAGE_LEFT_BUTTON_UP)
                break;
        }
        if (m_status & WIDGET_SELECTED) {
            deselect(msg);
            return 2;
        }
        return 1;
    }
    case MESSAGE_LEFT_BUTTON_UP: {
        if (isDisabled)
            return 0;
        if (!(m_status & WIDGET_DRAWN))
            break;
        if (!(m_status & WIDGET_SELECTED))
            break;
        return deselect(msg);
    }
    case MESSAGE_RIGHT_BUTTON_DOWN: {
        if (!(m_status & WIDGET_DRAWN))
            break;
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
    default:
        if (isDisabled)
            return 0;
        break;
    }
    return widget::main(msg);
}

#if 0  // @carcass

#endif  // @carcass

// E:\gamedcs\button.cpp:366
// The click sample plays through MemorySample with soundManager
// field_84 toggled around the call, then the widget-select message is
// stamped and the repeat timer armed - homm2's KBTickCount idiom with
// the same sixty-tick delay. DC392 and retail Select +0xb3 call the
// canonical GameTime::Get; DC also proves the message reference formal.
// E:\gamedcs\button.cpp:366
VA(0x00456860, 0xDA)  // linkorder bracket; MemorySample/UpdateScreen callees byte-proven, dc 0x57730
int button::select(message& msg)
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
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SELECT;
    msg.m_codeY = m_id;
    g_timers[GLOBAL_BUTTON_REPEAT_TIMER_SLOT] = GameTime::get() + BUTTON_REPEAT_DELAY_TICKS;
    g_leftRightSave = msg.m_qualifier & 0x300;
    return 2;
}

// Original: button::Deselect; button.cpp:401, dc 0x57854.
// CodeView's Main calls this member at dc 0x57304/0x574ce/0x57676/
// 0x576e0/0x57708. Its body owns the selected-bit early return/clear,
// Draw, UpdateScreen, widget message, endDialog choice and qualifier reset.
// Complete expands the same member in Main: the first copy clears +0x16
// at 0x4561d6, draws at 0x4561dc, calls UpdateScreen at 0x456206, then
// stamps the message and clears gLeftRightSave at 0x45620e..0x45623e.
// The DC-only combat-screen offset arm at dc 0x5787a..0x578ba is absent
// in that retail expansion. Keep the reference formal proved by CodeView.
// Formerly DeselectSelected with an unsupported inline keyword copied from
// the homm2 reconstruction; use the actual HoMM3 name and source position.
int button::deselect(message& msg)
{
    if (!(m_status & WIDGET_SELECTED))
        return 0;
    m_status &= ~WIDGET_SELECTED;
    draw();
    g_windowManager->updateScreen(m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeY = m_id;
    if (m_endDialog == 1)
        msg.m_codeX = widget::WIDGET_END_DIALOG;
    else
        msg.m_codeX = widget::WIDGET_DESELECT;
    msg.m_qualifier = g_leftRightSave;
    g_leftRightSave = 0;
    return 2;
}

// E:\gamedcs\button.cpp:429
// DC GetRealWidth and GetRealHeight query the sprite, not the widget box.
// All three PC button vtables use the folded iconWidget representatives
// 0x4eab20/0x4eab30: load sprite at +0x30, then its +0x30/+0x34 dimension.
// Keep these distinct source overrides; their physical bodies are already
// claimed by iconwdgt.cpp, so they do not create duplicate RVA claims.
int button::getRealWidth() const
{
    return m_buttonIcon->getWidth();
}

// E:\gamedcs\button.cpp:435
int button::getRealHeight() const
{
    return m_buttonIcon->getHeight();
}

// E:\gamedcs\button.cpp:441
// DC57938 is an empty const two-argument override. All three PC button
// slot-3 entries fold to textWidget's ret-8 representative at 0x5bc7e0.
void button::zBufferDraw(unsigned short* zBuffer, int id) const
{
}

// E:\gamedcs\button.cpp:446
// Frame choice: highlighted (field_40) unless selected; dimmed or
// disabled fall to disabled_frame; selected to selectedFrame; any
// frame past the sequence-0 count clamps to 0.
// E:\gamedcs\button.cpp:446
VA(0x00456940, 0x99)  // vtable-slot 4 of button (0x63bb54), dc 0x5793c
void button::draw() const
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

// E:\gamedcs\button.cpp:471
// DC57a24 is empty; all three PC slot-8 entries fold to the bare ret at
// 0x5bc690, already claimed by textWidget. Inheriting widget::dim instead
// incorrectly introduces its screen-darkening operation.
void button::dim() const
{
}

// E:\gamedcs\button.cpp:477
VA(0x004569e0, 0x2E)  // anchor-global, dc 0x57a28
void button::setPlayerPaletteColors(int whichPlayer)
{
    ::setPlayerPaletteColors(m_buttonIcon->getPalette(), whichPlayer);
    ::setPlayerPaletteColors(m_buttonIcon->getPalette24(), whichPlayer);
}

VA(0x00456a10, 0x10)
void button::vslot12(int on)
{
    widget::vslot12(on);
}

#if 0  // @carcass

// E:\gamedcs\button.cpp:487
// Complete has no default-textButton constructor slot. Its vtable has only
// two retail references: the fourteen-argument ctor 0x456a50 and destructor
// 0x456bf0. No other constructor vptr store or gap body fits this overload.
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
textButton::textButton(int x, int y, int w, int h, int id, const char* image, const char* text, const char* fontName, int normal, int selected, unsigned char end, int hotkey, int style, font::TColor newColor)
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

// E:\gamedcs\button.cpp:525
// The pressed state nudges the caption right and down by one; dimmed
// or disabled shifts the color scheme by two; VC6's inlined c_str()
// supplies the empty-string literal when Text is unallocated. Keeping the
// shared parentWindow pointer as a named local reproduces retail's paired
// allocation: status stays in CX and the parent pointer occupies EAX.
// E:\gamedcs\button.cpp:525
VA(0x00456ca0, 0x82)  // vtable-slot 4 of textButton (0x63bb88), dc 0x57b98
void textButton::draw() const
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
                            drawX, drawY, m_width, m_height, font::TColor(color), 5, -1);
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
// Complete's type_func_button vtable has one retail reference, in the
// nine-argument ctor 0x456d30. The definition-record overload has no vptr
// store or unclaimed body slot in the fully accounted button band.
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
