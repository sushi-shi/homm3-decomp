// border.cpp - E:\gamedcs\border.cpp (compiland border.obj)
#include "terrain.h"
#include <va.h>
#include <string.h>
#include "border.h"
#include "bitmap816.h"
#include "bitmap16.h"
#include "resourcemanager.h"
#include "winmgr.h"
// bitmapBorder's two blitters offset the widget rect by its parent
// window's origin, so this TU needs the COMPLETE heroWindow.
#include "window.h"

// Ordinary source-local body. Retail expands it in the three derived
// constructors (0x450130, 0x4502d0, 0x450690), leaving a widget default-
// constructor call and a single derived vtable store. No explicit inline
// declaration is needed to expose this body to those same-TU callers.
// E:\gamedcs\border.cpp:34, dc 0x5433c
border::border() {}

VA_COMPGEN(0x0044fee0, 0x21, SCALAR_DELETING_DTOR, border)

VA(0x0044ff10, 0x32)  // dc 0x54378
border::border(int x, int y, int w, int h, int id, int style)
    : widget(x, y, w, h, id, style)
{
}

VA(0x0044ff50, 0xB)  // dc 0x543d8
border::~border()
{
}

VA(0x0044ff60, 0x1CD)  // dc 0x54440
int border::main(message& msg)
{
    if (m_sleepCount > 0)
        return 0;

    {
        if (!(m_status & WIDGET_ACTIVE)) {
            if (msg.m_id == MESSAGE_WIDGET)
                return widget::main(msg);
        } else {
            unsigned char isDisabled = 0;
            if (m_status & WIDGET_DISABLED)
                isDisabled = 1;

            switch (msg.m_id) {
            case MESSAGE_LEFT_BUTTON_DOWN:
                if (isDisabled)
                    return widget::main(msg);
                // fall through
            case MESSAGE_RIGHT_BUTTON_DOWN: {
                short mouseX = msg.m_codeX - m_parentWindow->m_x;
                short mouseY = msg.m_codeY - m_parentWindow->m_y;
                if (mouseX >= m_x && mouseY >= m_y && mouseX < m_x + m_width
                    && mouseY < m_y + m_height) {
                    if (msg.m_id == MESSAGE_RIGHT_BUTTON_DOWN) {
                        msg.m_qualifier = MESSAGE_MODIFIER_RIGHT;
                        msg.m_codeX = WIDGET_RIGHT_SELECT;
                    } else {
                        m_status |= WIDGET_SELECTED;
                        msg.m_codeX = WIDGET_SELECT;
                    }
                    if (handleClick(1, msg.m_id == MESSAGE_RIGHT_BUTTON_DOWN))
                        return 1;
                    msg.m_id = MESSAGE_WIDGET;
                    msg.m_codeY = m_id;
                    return 2;
                }
                break;
            }

            case MESSAGE_LEFT_BUTTON_UP:
                if (isDisabled)
                    return widget::main(msg);
                // fall through
            case MESSAGE_RIGHT_BUTTON_UP:
                if (m_status & WIDGET_SELECTED) {
                    m_status &= ~WIDGET_SELECTED;
                    if (handleClick(0, msg.m_id == MESSAGE_RIGHT_BUTTON_UP))
                        return 1;
                    msg.m_id = MESSAGE_WIDGET;
                    msg.m_codeX = WIDGET_DESELECT;
                    msg.m_codeY = m_id;
                    return 2;
                }
                break;
            default:
                return widget::main(msg);
            }
        }
    }

    return 0;
}

// E:\gamedcs\border.cpp:153/154. The DC body returns zero; its public
// UAA_N_N0 signature proves native bool for the return and both parameters.
// Retail border vslot 13 folds onto iconWidget's 0x4eab10 representative.
// Keep border's canonical source body without a duplicate retail claim.
DC_ONLY(0x54590, 0x4)
bool border::handleClick(bool downClick, bool rightClick)
{
    return false;
}

// E:\gamedcs\border.cpp:201 - promoted from DC_ONLY 2026-08-14, the
// constructor the earlier sdd note asked for. `ret 0x1c` is seven stack
// dwords; six of them go to widget::initialize in x,y,w,h,id,style order
// and the ODD one out - [ebp+0x1c], the sixth argument - is the dword
// stored to [this+0x30], which is what fixes the DC parameter order
// (..., int color_, int style) and colour's int width. [this+0x34] is
// zeroed as a BYTE, fixing colorize's type.
//
// The base is entered through ??0widget@@QAE@XZ with a SINGLE derived
// vtable store: border's default ctor is expanded in place and its
// ??_7border@@6B@ store dead-store-eliminated. widget::initialize runs
// in the ctor BODY (not a base initializer), which is why VC6 wraps the
// whole thing in an fs:[0] frame - the base subobject has to be
// unwindable across that call.
VA(0x00450130, 0x6D)  // anchor-bracket + arity (`ret 0x1c`), dc 0x54650
coloredBorderFrame::coloredBorderFrame(int x, int y, int w, int h, int id,
                                       int color, int style)
{
    initialize(x, y, w, h, id, style);
    m_color = color;
    m_colorize = 0;
}

VA_COMPGEN(0x004501a0, 0x21, SCALAR_DELETING_DTOR, coloredBorderFrame)

// inlined ~border, so only ??_7border@@6B@ survives before the
// CodeView dc 0x54dd8: CV_fldattr_t.compgenx marks this destructor
// as implicit. Its retained retail body performs only base/member teardown.
VA_COMPGEN(0x004501d0, 0xB, IMPLICIT_DTOR, coloredBorderFrame)

VA(0x004501e0, 0x5B)  // dc 0x546d4
void coloredBorderFrame::draw() const
{
    if (m_colorize)
        g_windowManager->m_screenBitmap->colorize(m_x + m_parentWindow->m_x,
            m_y + m_parentWindow->m_y, m_width, m_height, m_color);
    else
        g_windowManager->m_screenBitmap->frameRect(m_x + m_parentWindow->m_x,
            m_y + m_parentWindow->m_y, m_width, m_height, m_color);
}

VA(0x00450240, 0x82)  // dc 0x54744
int coloredBorderFrame::main(message& msg)
{
    if (m_sleepCount > 0)
        return 0;
    if (!(m_status & WIDGET_ACTIVE)) {
        if (msg.m_id != MESSAGE_WIDGET)
            return 0;
    } else if (msg.m_id == MESSAGE_WIDGET) {
        switch (msg.m_codeX) {
        case WIDGET_SET_COLOR:
            if (msg.m_codeY == m_id) {
                m_color = msg.m_extra & 0xFFFF;
                return 1;
            }
            break;
        case WIDGET_SET_COLORIZE:
            if (msg.m_codeY == m_id) {
                m_colorize = msg.m_extra != 0;
                return 1;
            }
            break;
        }
    }
    return border::main(msg);
}

VA(0x004502d0, 0x8C)  // dc 0x547c0
bitmapBorder::bitmapBorder(int x, int y, int w, int h, int id,
                           const char* image, int style)
{
    initialize(x, y, w, h, id, style);
    if (image)
        m_image = ResourceManager::getBitmap816(image);
    else
        m_image = 0;
}

#if 0  // @carcass

// E:\gamedcs\border.cpp:67
DC_ONLY(0x54408, 0x36)
void border::initialize(int x, int y, int w, int h, int id, int style, unsigned char focusable)
{
    // @stub
}

// E:\gamedcs\border.cpp:158
DC_ONLY(0x54594, 0x4)
void border::zBufferDraw()
{
    // @stub
}

// E:\gamedcs\border.cpp:161
DC_ONLY(0x54598, 0x4)
void border::draw()
{
    // @stub
}

// E:\gamedcs\border.cpp:174
DC_ONLY(0x5459c, 0x78)
void coloredBorder::coloredBorder(int x, int y, int w, int h, int id, int color_, int style)
{
    // @stub
}

// E:\gamedcs\border.cpp:182
DC_ONLY(0x54614, 0x4)
void coloredBorder::zBufferDraw()
{
    // @stub
}

// E:\gamedcs\border.cpp:185
DC_ONLY(0x54618, 0x38)
void coloredBorder::draw()
{
    // @stub
}

// E:\gamedcs\border.cpp:210
DC_ONLY(0x546d0, 0x4)
void coloredBorderFrame::zBufferDraw()
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x00450360, 0x21, SCALAR_DELETING_DTOR, bitmapBorder)

VA(0x00450390, 0x5B)  // dc 0x54860
bitmapBorder::~bitmapBorder()
{
    if (m_image)
        m_image->dispose();
}

#if 0  // @carcass

// E:\gamedcs\border.cpp:323
DC_ONLY(0x54988, 0x38)
void bitmapBorder::setPalette(const char* palette_name)
{
    // @stub
}

#endif  // @carcass

VA(0x004503f0, 0x55)  // dc 0x5489c
void bitmapBorder::zBufferDraw(unsigned short* zBuffer, int id) const
{
    if (m_image)
        m_image->zBufferDraw(0, 0, m_width, m_height, zBuffer,
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, 800, 600, 1600, id);
}

VA(0x00450450, 0x44)  // dc 0x548fc
void bitmapBorder::draw() const
{
    if (m_image)
        m_image->draw(0, 0, m_width, m_height, g_windowManager->m_screenBitmap,
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, 1);
}

VA(0x004504a0, 0xE)  // dc 0x54948
int bitmapBorder::getRealWidth() const
{
    if (m_image)
        return m_image->getWidth();
    return 0;
}

VA(0x004504b0, 0xE)  // dc 0x54968
int bitmapBorder::getRealHeight() const
{
    if (m_image)
        return m_image->getHeight();
    return 0;
}

VA(0x004504c0, 0x5B)  // dc 0x549c0
void bitmapBorder::setImage(const char* bitmapName)
{
    if (m_image != 0) {
        if (strcmp(m_image->getName(), bitmapName) == 0)
            return;
        m_image->dispose();
    }
    m_image = ResourceManager::getBitmap816(bitmapName);
}

VA(0x00450520, 0x2D)  // dc 0x549ec
void bitmapBorder::setPlayerPaletteColors(int whichPlayer)
{
    ::setPlayerPaletteColors(m_image->m_p16.m_colors.m_data, whichPlayer);
    ::setPlayerPaletteColors(m_image->m_p24, whichPlayer);
}

VA(0x00450550, 0x132)  // dc 0x54a20
int bitmapBorder::main(message& msg)
{
    if (m_sleepCount > 0)
        return 0;
    if (!(m_status & WIDGET_ACTIVE)) {
        if (msg.m_id != MESSAGE_WIDGET)
            return 0;
    } else if (msg.m_id == MESSAGE_WIDGET && msg.m_codeY == m_id) {
        switch (msg.m_codeX) {
        case WIDGET_SET_PALETTE: {
            const char* paletteName = msg.m_extraText;
            if (m_image) {
                TPalette16* newPalette =
                    ResourceManager::getPalette(paletteName);
                if (newPalette) {
                    m_image->setPalette(newPalette->m_data);
                    newPalette->dispose();
                }
            }
            return 1;
        }
        case WIDGET_SET_IMAGE:
            setImage(msg.m_extraText);
            return 1;
        case WIDGET_SET_PLAYER_PALETTE_COLORS:
            setPlayerPaletteColors(msg.m_extra);
            return 1;
        }
    }
    return border::main(msg);
}

VA(0x00450690, 0x8C)  // dc 0x54a98
bitmapBorder16::bitmapBorder16(int x, int y, int w, int h, int id,
                               const char* image, int style)
{
    initialize(x, y, w, h, id, style);
    if (image)
        m_image = ResourceManager::getBitmap16(image);
    else
        m_image = 0;
}

VA_COMPGEN(0x00450720, 0x21, SCALAR_DELETING_DTOR, bitmapBorder16)

VA(0x00450750, 0x5B)  // dc 0x54b68
bitmapBorder16::~bitmapBorder16()
{
    if (m_image)
        m_image->dispose();
}

#if 0  // @carcass

// E:\gamedcs\border.cpp:415
DC_ONLY(0x54ba4, 0x4)
void bitmapBorder16::zBufferDraw()
{
    // @stub
}

#endif  // @carcass

VA(0x004507b0, 0x55)  // dc 0x54ba8
void bitmapBorder16::draw() const
{
    if (m_image) {
        Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
        m_image->draw(0, 0, m_width, m_height, screen->getMap(0, 0),
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, screen->getWidth(),
            screen->getHeight(), screen->getPitch(), 0);
    }
}

VA(0x00450810, 0x44)  // dc 0x54bf0
void bitmapBorder16::draw2() const
{
    if (m_image) {
        Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
        m_image->draw(0, 0, m_width, m_height, screen->getMap(0, 0), m_x, m_y, screen->getWidth(),
            screen->getHeight(), screen->getPitch(), 0);
    }
}

// E:\gamedcs\border.cpp:449 - promoted from DC_ONLY, slot 2 of vtable
// 0x63bacc (the only reference to this row in the whole image, stored by
// the constructor 0x450690 and re-stored by the destructor 0x450750).
// bitmapBorder::Main one class up, arm for arm: the same hoisted id test
// ahead of the codeX chain, the same `sub ecx,0xa` two-case subtract, and
// the same duplicated `return 0` epilogues.

// Only two commands survive on the 16-bit variant. SET_PALETTE is an
// EMPTY accepting arm - retail's `je` lands straight on `mov eax,1` -
// which is what a hi-colour image with no palette to retint leaves. The
// SET_IMAGE arm is the whole of bitmapBorder16::SetImage (dc 0x54c6c),
// expanded: retail has NO row for that method anywhere in border.obj's
// span because Main is its only caller, so /OPT:REF dropped the orphaned
// COMDAT after /Ob2 expanded it here. Writing it as the CALL is what gives
// retail's single `return 1` tail - the method's own `return` becomes a
// forward jump onto it, where a longhand `return 1` duplicates the
// epilogue and inverts the strcmp branch.
// E:\gamedcs\border.cpp:441 - bitmapBorder::SetImage one class up with the
// hi-colour loader. No VA: retail keeps no row for it (see the note below).
void bitmapBorder16::setImage(const char* bitmapName)
{
    if (m_image != 0) {
        if (strcmp(m_image->getName(), bitmapName) == 0)
            return;
        m_image->dispose();
    }
    m_image = ResourceManager::getBitmap16(bitmapName);
}

VA(0x00450860, 0xC6)  // dc 0x54c98
int bitmapBorder16::main(message& msg)
{
    if (m_sleepCount > 0)
        return 0;
    if (!(m_status & WIDGET_ACTIVE)) {
        if (msg.m_id != MESSAGE_WIDGET)
            return 0;
    } else if (msg.m_id == MESSAGE_WIDGET && msg.m_codeY == m_id) {
        switch (msg.m_codeX) {
        case WIDGET_SET_PALETTE:
            return 1;
        case WIDGET_SET_IMAGE:
            setImage(msg.m_extraText);
            return 1;
        }
    }
    return border::main(msg);
}

#if 0  // @carcass

// E:\gamedcs\border.cpp:431
DC_ONLY(0x54c2c, 0x20)
int bitmapBorder16::getRealWidth() const
{
    // @stub
}

// E:\gamedcs\border.cpp:436
DC_ONLY(0x54c4c, 0x20)
int bitmapBorder16::getRealHeight() const
{
    // @stub
}

// E:\gamedcs\border.cpp:35
DC_ONLY(0x54d24, 0x34)
void* border::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\border.cpp:178
DC_ONLY(0x54d58, 0x34)
void* coloredBorder::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\border.cpp:178
DC_ONLY(0x54d8c, 0x18)
void coloredBorder::~coloredBorder()
{
    // @stub
}

// E:\gamedcs\border.cpp:290
DC_ONLY(0x54df0, 0x34)
void* bitmapBorder::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\border.cpp:404
DC_ONLY(0x54e24, 0x34)
void* bitmapBorder16::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass

// COMDAT pairing: bitset<10>::_Xran, agreement 0.901 at an exactly equal
// 203-byte extent, and the only bitset width this object instantiates.
VA_COMPGEN(0x00404410, 0xCB, BITSET_XRAN, Bitset10)
