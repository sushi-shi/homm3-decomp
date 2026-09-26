#include "va.h"

#include <string.h>

#include "border.h"

#include "bitmap16.h"
#include "bitmap816.h"
#include "resourcemanager.h"
#include "terrain.h"
#include "window.h"
#include "winmgr.h"

// Ordinary source-local body. Retail expands it in the three derived
// constructors (0x450130, 0x4502d0, 0x450690), leaving a widget default-
// constructor call and a single derived vtable store. No explicit inline
// declaration is needed to expose this body to those same-TU callers.
// E:\gamedcs\border.cpp:34, dc 0x5433c
MAC_ADDRESS(0x05e244, 0x38)
border::border() {}

VA_COMPGEN(0x0044fee0, 0x21, SCALAR_DELETING_DTOR, border)

VA(0x0044ff10, 0x32) MAC_ADDRESS(0x05e27c, 0x50)  // dc 0x54378
border::border(int x, int y, int w, int h, int id, int style)
    : widget(x, y, w, h, id, style)
{
}

VA(0x0044ff50, 0xB) MAC_ADDRESS(0x05e2cc, 0x60)  // dc 0x543d8
border::~border()
{
}

// Original: border::initialize; border.cpp:67, dc 0x54408.
// Complete removes widget's focusable storage (see widget.h). The retained
// derived constructors expand this forwarding call to widget::initialize.
MAC_ADDRESS(0x05e32c, 0x20)
void border::initialize(int x, int y, int w, int h, int id, int style,
                        unsigned char focusable)
{
    widget::initialize(x, y, w, h, id, style);
}

VA(0x0044ff60, 0x1CD) MAC_ADDRESS(0x05e34c, 0x230)  // dc 0x54440
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
                    break;
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
                return 0;
            }

            case MESSAGE_LEFT_BUTTON_UP:
                if (isDisabled)
                    break;
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
                return 0;
            default:
                break;
            }
            // Mac 0x5e560: disabled button cases and default share this call.
            return widget::main(msg);
        }
    }

    return 0;
}

// E:\gamedcs\border.cpp:153/154. The DC body returns zero; its public
// UAA_N_N0 signature proves native bool for the return and both parameters.
// Retail border vslot 13 folds onto iconWidget's 0x4eab10 representative.
// Keep border's canonical source body without a duplicate retail claim.

bool border::handleClick(bool downClick, bool rightClick)
{
    return false;
}

// Original: border::zBufferDraw; border.cpp:158, dc 0x54594.
// Retail vtable 0x63ba24 slot 3 shares the empty ret-8 representative 0x5bc7e0.
void border::zBufferDraw(unsigned short* zBuffer, int id) const {}

// Original: border::Draw; border.cpp:161, dc 0x54598.
// Its slot 4 shares the empty no-argument body at 0x5bc690.
void border::draw() const {}

// Original: coloredBorder::coloredBorder; border.cpp:174, dc 0x5459c.
coloredBorder::coloredBorder(int x, int y, int w, int h, int id,
                             int color, int style)
{
    initialize(x, y, w, h, id, style);
    m_color = color;
}

// Original: coloredBorder::zBufferDraw; border.cpp:182, dc 0x54614.
void coloredBorder::zBufferDraw(unsigned short* zBuffer, int id) const {}

// Original: coloredBorder::Draw; border.cpp:185, dc 0x54618.
void coloredBorder::draw() const
{
    g_windowManager->m_screenBitmap->fillRect(m_x + m_parentWindow->m_x,
        m_y + m_parentWindow->m_y, m_width, m_height, m_color);
}

// E:\gamedcs\border.cpp:201 - located in retail 2026-08-14, the
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
VA(0x00450130, 0x6D) MAC_ADDRESS(0x05e58c, 0x88)  // anchor-bracket + arity (`ret 0x1c`), dc 0x54650
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

// Original: coloredBorderFrame::zBufferDraw; border.cpp:210, dc 0x546d0.
// Retail slot 3 at 0x63ba68 shares 0x5bc7e0 with border and textWidget.
void coloredBorderFrame::zBufferDraw(unsigned short* zBuffer, int id) const {}

VA(0x004501e0, 0x5B) MAC_ADDRESS(0x05e618, 0xa4)  // dc 0x546d4
void coloredBorderFrame::draw() const
{
    if (m_colorize)
        g_windowManager->m_screenBitmap->colorize(m_x + m_parentWindow->m_x,
            m_y + m_parentWindow->m_y, m_width, m_height, m_color);
    else
        g_windowManager->m_screenBitmap->frameRect(m_x + m_parentWindow->m_x,
            m_y + m_parentWindow->m_y, m_width, m_height, m_color);
}

VA(0x00450240, 0x82) MAC_ADDRESS(0x05e6bc, 0xe4)  // dc 0x54744
int coloredBorderFrame::main(message& msg)
{
    if (m_sleepCount > 0)
        return 0;
    if (!(m_status & WIDGET_ACTIVE)) {
        // Mac retains this base call separately at 0:0x5e6fc.
        if (msg.m_id == MESSAGE_WIDGET)
            return border::main(msg);
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

VA(0x004502d0, 0x8C) MAC_ADDRESS(0x05e7a0, 0x9c)  // dc 0x547c0
bitmapBorder::bitmapBorder(int x, int y, int w, int h, int id,
                           const char* image, int style)
{
    initialize(x, y, w, h, id, style);
    if (image)
        m_image = ResourceManager::getBitmap816(image);
    else
        m_image = 0;
}

VA_COMPGEN(0x00450360, 0x21, SCALAR_DELETING_DTOR, bitmapBorder)

VA(0x00450390, 0x5B) MAC_ADDRESS(0x05e83c, 0x7c)  // dc 0x54860
bitmapBorder::~bitmapBorder()
{
    if (m_image)
        m_image->dispose();
}

VA(0x004503f0, 0x55) MAC_ADDRESS(0x05e8b8, 0x7c)  // dc 0x5489c
void bitmapBorder::zBufferDraw(unsigned short* zBuffer, int id) const
{
    if (m_image)
        m_image->zBufferDraw(0, 0, m_width, m_height, zBuffer,
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, 800, 600, 1600, id);
}

VA(0x00450450, 0x44) MAC_ADDRESS(0x05e934, 0x70)  // dc 0x548fc
void bitmapBorder::draw() const
{
    if (m_image)
        m_image->draw(0, 0, m_width, m_height, g_windowManager->m_screenBitmap,
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, 1);
}

// Original: bitmapBorder::SetPalette; border.cpp:323, dc 0x54988.
// Retail Main's SET_PALETTE arm expands the same load/copy/dispose sequence.
MAC_ADDRESS(0x05e9dc, 0x6c)
void bitmapBorder::setPalette(const char* paletteName)
{
    if (m_image) {
        TPalette16* newPalette = ResourceManager::getPalette(paletteName);
        if (newPalette) {
            m_image->setPalette(newPalette->m_data);
            newPalette->dispose();
        }
    }
}

VA(0x004504a0, 0xE) MAC_ADDRESS(0x05e9a4, 0x1c)  // dc 0x54948
int bitmapBorder::getRealWidth() const
{
    if (m_image)
        return m_image->getWidth();
    return 0;
}

VA(0x004504b0, 0xE) MAC_ADDRESS(0x05e9c0, 0x1c)  // dc 0x54968
int bitmapBorder::getRealHeight() const
{
    if (m_image)
        return m_image->getHeight();
    return 0;
}

VA(0x004504c0, 0x5B) MAC_ADDRESS(0x05ea48, 0x74)  // dc 0x549c0
void bitmapBorder::setImage(const char* bitmapName)
{
    if (m_image != 0) {
        if (strcmp(m_image->getName(), bitmapName) == 0)
            return;
        m_image->dispose();
    }
    m_image = ResourceManager::getBitmap816(bitmapName);
}

VA(0x00450520, 0x2D) MAC_ADDRESS(0x05eabc, 0x50)  // dc 0x549ec
void bitmapBorder::setPlayerPaletteColors(int whichPlayer)
{
    ::setPlayerPaletteColors(m_image->getPalette().m_colors.m_data, whichPlayer);
    ::setPlayerPaletteColors(m_image->getPalette24(), whichPlayer);
}

VA(0x00450550, 0x132) MAC_ADDRESS(0x05eb0c, 0xd8)  // dc 0x54a20
int bitmapBorder::main(message& msg)
{
    if (m_sleepCount > 0)
        return 0;
    if (!(m_status & WIDGET_ACTIVE)) {
        // Mac retains this base call separately at 0:0x5eb44.
        if (msg.m_id == MESSAGE_WIDGET)
            return border::main(msg);
        return 0;
    } else if (msg.m_id == MESSAGE_WIDGET && msg.m_codeY == m_id) {
        switch (msg.m_codeX) {
        case WIDGET_SET_PALETTE:
            setPalette(msg.m_extraText);
            return 1;
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

VA(0x00450690, 0x8C) MAC_ADDRESS(0x05ebe4, 0x9c)  // dc 0x54a98
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

VA(0x00450750, 0x5B) MAC_ADDRESS(0x05ec80, 0x7c)  // dc 0x54b68
bitmapBorder16::~bitmapBorder16()
{
    if (m_image)
        m_image->dispose();
}

// Original: bitmapBorder16::zBufferDraw; border.cpp:415, dc 0x54ba4.
// The 0x63bacc vtable's slot 3 folds to the shared empty ret-8 body.
void bitmapBorder16::zBufferDraw(unsigned short* zBuffer, int id) const {}

VA(0x004507b0, 0x55) MAC_ADDRESS(0x05ed00, 0x8c)  // dc 0x54ba8
void bitmapBorder16::draw() const
{
    if (m_image) {
        Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
        m_image->draw(0, 0, m_width, m_height, screen->getMap(0, 0),
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, screen->getWidth(),
            screen->getHeight(), screen->getPitch(), 0);
    }
}

VA(0x00450810, 0x44) MAC_ADDRESS(0x05ed8c, 0x78)  // dc 0x54bf0
void bitmapBorder16::draw2() const
{
    if (m_image) {
        Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
        m_image->draw(0, 0, m_width, m_height, screen->getMap(0, 0), m_x, m_y, screen->getWidth(),
            screen->getHeight(), screen->getPitch(), 0);
    }
}

// Original: bitmapBorder16::GetRealWidth; border.cpp:431, dc 0x54c2c.
// Complete vslot 6 at 0x63bae4 shares bitmapBorder::getRealWidth, 0x4504a0:
// both bitmap types put Width at +0x24 after their resource base.
int bitmapBorder16::getRealWidth() const
{
    return m_image ? m_image->getWidth() : 0;
}

// Original: bitmapBorder16::GetRealHeight; border.cpp:436, dc 0x54c4c.
// Vslot 5 at 0x63bae0 similarly shares bitmapBorder::getRealHeight, 0x4504b0.
int bitmapBorder16::getRealHeight() const
{
    return m_image ? m_image->getHeight() : 0;
}

// E:\gamedcs\border.cpp:449 - located in retail, slot 2 of vtable
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
MAC_ADDRESS(0x05ee3c, 0x74)
void bitmapBorder16::setImage(const char* bitmapName)
{
    if (m_image != 0) {
        if (strcmp(m_image->getName(), bitmapName) == 0)
            return;
        m_image->dispose();
    }
    m_image = ResourceManager::getBitmap16(bitmapName);
}

VA(0x00450860, 0xC6) MAC_ADDRESS(0x05eeb0, 0xb0)  // dc 0x54c98
int bitmapBorder16::main(message& msg)
{
    if (m_sleepCount > 0)
        return 0;
    if (!(m_status & WIDGET_ACTIVE)) {
        // Mac retains this base call separately at 0:0x5eee8.
        if (msg.m_id == MESSAGE_WIDGET)
            return border::main(msg);
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

// COMDAT pairing: bitset<10>::_Xran, agreement 0.901 at an exactly equal
// 203-byte extent, and the only bitset width this object instantiates.
VA_COMPGEN(0x00404410, 0xCB, BITSET_XRAN, Bitset10)
