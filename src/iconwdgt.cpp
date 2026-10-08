#include "va.h"

#include "iconwdgt.h"

#include "button.h"
#include "csequence.h"
#include "csprite.h"
#include "cspriteframe.h"
#include "message.h"
#include "palette.h"
#include "resourcemanager.h"
#include "terrain.h"
#include "window.h"
#include "winmgr.h"

int random(int min, int max);

// Original: iconWidget::iconWidget; iconwdgt.cpp:35
DC_ADDRESS(0x0d92fc, 0x54)
iconWidget::iconWidget() : widget(0, 0, 0, 0, 0, 0)
{
    m_sprite = 0;
    m_frame = 0;
    m_seqId = 0;
    m_backColor = 0;
    m_isFlipped = 0;
}

VA_COMPGEN(0x004ea6f0, 0x21, SCALAR_DELETING_DTOR, iconWidget)

VA(0x004ea720, 0x8C)
DC_ADDRESS(0x0d9350, 0xa2)
MAC_ADDRESS(0x10c3f0, 0xa8)
iconWidget::iconWidget(int x, int y, int w, int h, int id, const char* image,
                       int frame, int sequence, bool flipped,
                       unsigned backColor, int style)
    : widget(x, y, w, h, id, style),
      m_frame(frame),
      m_seqId(sequence),
      m_isFlipped(flipped),
      m_backColor(static_cast<unsigned short>(backColor)),
      m_postPostWalkSequence(cs_wait)
{
    m_sprite = image ? ResourceManager::GetSprite(image) : 0;
}

// Original: iconWidget::initialize; iconwdgt.cpp:75
// Complete removed widget's focusable field (DC +0x20); preserve the
// source interface without writing that obsolete controller-state slot.
DC_ADDRESS(0x0d93f4, 0x6e)
void iconWidget::initialize(int x, int y, int w, int h, int id,
    const char* image, int frame, int sequence, unsigned char flipped,
    unsigned int backColor, int style, unsigned char focusable)
{
    widget::initialize(x, y, w, h, id, style);
    m_sprite = ResourceManager::GetSprite(image);
    m_frame = frame;
    m_seqId = sequence;
    m_isFlipped = flipped != 0;
    m_backColor = static_cast<unsigned short>(backColor);
    m_postPostWalkSequence = cs_wait;
}

VA(0x004ea7b0, 0x55)
DC_ADDRESS(0x0d9464, 0x3e)
MAC_ADDRESS(0x10c498, 0x7c)
iconWidget::~iconWidget()
{
    if (m_sprite)
        ResourceManager::Dispose(m_sprite);
}

// E:\gamedcs\iconwdgt.cpp:119
// Retail fixes the message protocol and six widget-command arms. DC 151
// calls SetIconSequence; its 447..448 stores are seqId then Frame. DC 159
// calls the ordinary SetPalette helper; restore that canonical call rather
// than its copied load/dispose body. Both helpers expand naturally here.

// DC 190..217 and 229..245 retain nested mouse-hit and selected scopes.
// Mac retains three widget::main calls: inactive, unhandled disabled widget
// message, and the shared active tail. Restoring the middle source call also
// makes the Windows retail comparison exact.
VA(0x004ea810, 0x2F4)
DC_ADDRESS(0x0d94a4, 0x212)
MAC_ADDRESS(0x10c514, 0x33c)  // vtable 0x63ec48 slot 2
int iconWidget::main(message& msg)
{
    if (m_sleepCount > 0) {
        return 0;
    }

    if (!(m_status & WIDGET_ACTIVE)) {
        if (msg.m_id == MESSAGE_WIDGET)
            return widget::main(msg);
        return 0;
    }

    bool isDisabled = false;
    if (m_status & WIDGET_DISABLED)
        isDisabled = true;

    int messageId = msg.m_id;
    switch (messageId) {
    case MESSAGE_WIDGET:
        if (msg.m_codeY == m_id) {
            switch (msg.m_codeX) {
            case WIDGET_SET_ICON_NAME:
                setSprite(msg.m_extraText);
                return 1;
            case WIDGET_SET_ICON_FRAME:
                setIconFrame(msg.m_extra & 0xFFFF);
                return 1;
            case WIDGET_SET_ICON_SEQUENCE:
                setIconSequence(msg.m_extra & 0xFFFF);
                return 1;
            case WIDGET_SET_ICON_COLOR:
                m_backColor = static_cast<unsigned short>(msg.m_extra);
                return 1;
            case WIDGET_SET_PALETTE:
                setPalette(msg.m_extraText);
                return 1;
            case WIDGET_SET_PLAYER_PALETTE_COLORS:
                setPlayerPaletteColors(msg.m_extra);
                return 1;
            }
        }
        // Mac retains this disabled-message base call at 0:0x10c688.
        if (isDisabled)
            return widget::main(msg);
        break;

    case MESSAGE_LEFT_BUTTON_DOWN:
        if (isDisabled)
            return 0;
        // fall through
    case MESSAGE_RIGHT_BUTTON_DOWN: {
        short mouseX = msg.m_codeX - m_parentWindow->m_x;
        short mouseY = msg.m_codeY - m_parentWindow->m_y;
        if (mouseX >= m_x && mouseY >= m_y && mouseX < m_x + m_width
            && mouseY < m_y + m_height) {
            if (handleClick(true, messageId == MESSAGE_RIGHT_BUTTON_DOWN))
                return 1;
            if (msg.m_id == MESSAGE_RIGHT_BUTTON_DOWN) {
                msg.m_qualifier = MESSAGE_MODIFIER_RIGHT;
                msg.m_codeX = WIDGET_RIGHT_SELECT;
            } else {
                m_status |= WIDGET_SELECTED;
                msg.m_codeX = WIDGET_SELECT;
            }
            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeY = m_id;
            return 2;
        }
        return 0;
    }

    case MESSAGE_LEFT_BUTTON_UP:
        if (isDisabled)
            return 0;
        // fall through
    case MESSAGE_RIGHT_BUTTON_UP:
        if (m_status & WIDGET_SELECTED) {
            m_status &= ~WIDGET_SELECTED;
            msg.m_id = MESSAGE_WIDGET;
            msg.m_codeX = WIDGET_DESELECT;
            msg.m_codeY = m_id;
            // Mac 0x10c7c8..0x10c7e0 retains this argument expression
            // after the MESSAGE_WIDGET assignment above.
            if (handleClick(false, msg.m_id == MESSAGE_RIGHT_BUTTON_UP))
                return 1;
            if (msg.m_id == MESSAGE_RIGHT_BUTTON_UP)
                msg.m_qualifier = MESSAGE_MODIFIER_RIGHT;
            return 2;
        }
        return 0;

    default:
        if (isDisabled)
            return 0;
        break;
    }

    return widget::main(msg);
}

// Original: iconWidget::zBufferDraw; iconwdgt.cpp:275
// CodeView's formal type proves two arguments despite the old nil-argument
// carcass. Retail folds this empty hook onto the shared ret-8 at 0x5bc7e0.
DC_ADDRESS(0x0d96e4, 0x4)
void iconWidget::zBufferDraw(unsigned short* zBuffer, int id) const
{
}

// E:\gamedcs\iconwdgt.cpp:257
// Located in retail 2026-08-08 on four independent corroborations:
// the row is inside iconwdgt.obj's own carve span, it holds the DC
// roster's handle_click slot in order (immediately before GetRealWidth
// and GetRealHeight, exactly as at 0x4eab20 / 0x4eab30), the iconWidget
// vtable at 0x63ec48 stores it in slot 13, and the body's `ret 8`
// matches the DC's three parameters (this + two). Both arguments are
// dead - retail returns a bare zero.
// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x004eab10, 0x5)
DC_ADDRESS(0x0d96b8, 0x4)
MAC_ADDRESS(0x10c850, 0x8)  // anchor-vtable (slot 13 of 0x63ec48)
bool iconWidget::handleClick(bool downClick, bool rightClick)
{
    return 0;
}

VA(0x004eab20, 0x7)
DC_ADDRESS(0x0d96bc, 0x12)
MAC_ADDRESS(0x10c858, 0xc)
int iconWidget::getRealWidth() const
{
    return m_sprite->GetWidth();
}

VA(0x004eab30, 0x7)
DC_ADDRESS(0x0d96d0, 0x12)
MAC_ADDRESS(0x10c864, 0xc)
int iconWidget::getRealHeight() const
{
    return m_sprite->GetHeight();
}

// The creature path is the one with real arithmetic: it centres on the
// crop box of the FIRST FRAME OF THE cs_wait SEQUENCE (Sprite->s[2]
// ->f[0]), puts the portrait 275 rows up from the widget's bottom edge,
// and then clips the source rectangle on all four sides - each negative
// offset moving into sx/sy and shrinking sw/sh, each overrun clamping
// sw/sh against the widget box.
VA(0x004eab40, 0x4B0)
DC_ADDRESS(0x0d96e8, 0x592)
MAC_ADDRESS(0x10c874, 0x6ec)
void iconWidget::draw() const
{
    int drawX = m_x + m_parentWindow->m_x;
    int drawY = m_y + m_parentWindow->m_y;

    switch (m_style) {
    case ICON_STYLE_PLAIN:
        switch (m_sprite->get_resType()) {
        case RESOURCE_TYPE_SPRITE:
            m_sprite->Draw(m_seqId, m_frame, 0, 0, m_sprite->GetWidth(), m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped, 1);
            break;
        case RESOURCE_TYPE_CREATURE:
            m_sprite->DrawCreature(m_seqId, m_frame, 0, 0, m_sprite->GetWidth(),
                m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped, 0);
            break;
        case RESOURCE_TYPE_ADVENTURE_OBJECT:
            m_sprite->DrawAdvObj(m_frame, 0, 0, m_sprite->GetWidth(), m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_HERO:
            m_sprite->DrawHero(m_seqId, m_frame, 0, 0, m_sprite->GetWidth(),
                m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_TILESET:
            m_sprite->DrawTile(m_frame, 0, 0, m_sprite->GetWidth(), m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped, 0);
            break;
        case RESOURCE_TYPE_POINTER:
            m_sprite->DrawPointer(m_frame, g_windowManager->m_screenBitmap,
                drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_INTERFACE:
            m_sprite->DrawInterface(m_frame, 0, 0, m_sprite->GetWidth(),
                m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_COMBAT_HERO:
            m_sprite->DrawCombatHero(m_seqId, m_frame, 0, 0, m_sprite->GetWidth(),
                m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        }
        break;

    case ICON_STYLE_CENTERED:
        if (m_sprite->GetWidth() < m_width)
            drawX += (m_width - m_sprite->GetWidth()) / 2;
        if (m_sprite->GetHeight() + 2 < m_height)
            drawY += m_height - m_sprite->GetHeight() - 2;
        switch (m_sprite->get_resType()) {
        case RESOURCE_TYPE_SPRITE:
            m_sprite->Draw(m_seqId, m_frame, 0, 0, m_sprite->GetWidth(), m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped, 1);
            break;
        case RESOURCE_TYPE_CREATURE:
            m_sprite->DrawCreature(m_seqId, m_frame, 0, 0, m_sprite->GetWidth(),
                m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped, 0);
            break;
        case RESOURCE_TYPE_ADVENTURE_OBJECT:
            m_sprite->DrawAdvObj(m_frame, 0, 0, m_sprite->GetWidth(), m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_HERO:
            m_sprite->DrawHero(m_seqId, m_frame, 0, 0, m_sprite->GetWidth(),
                m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_TILESET:
            m_sprite->DrawTile(m_frame, 0, 0, m_sprite->GetWidth(), m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped, 0);
            break;
        case RESOURCE_TYPE_POINTER:
            m_sprite->DrawPointer(m_frame, g_windowManager->m_screenBitmap,
                drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_INTERFACE:
            m_sprite->DrawInterface(m_frame, 0, 0, m_sprite->GetWidth(),
                m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_COMBAT_HERO:
            m_sprite->DrawCombatHero(m_seqId, m_frame, 0, 0, m_sprite->GetWidth(),
                m_sprite->GetHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        }
        break;

    case ICON_STYLE_CREATURE: {
        int sx;
        int sy;
        int sw;
        int sh;
        int offX;
        int offY;

        sx = 0;
        sy = 0;
        sw = m_sprite->GetWidth();
        sh = m_sprite->GetHeight();
        offX = m_width / 2 - m_sprite->GetCroppedWidth(cs_wait, 0) / 2 - m_sprite->GetCroppedX(cs_wait, 0);
        offY = m_height - 275;
        if (offX < 0) {
            sx = -offX;
            sw += offX;
            offX = 0;
        }
        if (offY < 0) {
            sy = -offY;
            sh += offY;
            offY = 0;
        }
        if (offX + sw > m_width)
            sw = m_width - offX;
        if (offY + sh > m_height)
            sh = m_height - offY;
        m_sprite->DrawCreature(m_seqId, m_frame, sx, sy, sw, sh,
            g_windowManager->m_screenBitmap, offX + drawX, offY + drawY, 0, 0);
        break;
    }

    }
}

VA(0x004eaff0, 0x3E)
DC_ADDRESS(0x0d9c7c, 0x28)
MAC_ADDRESS(0x10cf60, 0x50)
void iconWidget::setIconFrame(int newFrame)
{
    m_frame = newFrame % m_sprite->GetNumFrames(m_seqId);
}

// E:\gamedcs\iconwdgt.cpp:444
// Dreamcast line 447 stores new_sequence to seqId and line 448 clears Frame;
// Main's retail-inlined WIDGET_SET_ICON_SEQUENCE arm corroborates the order:
// its six-instruction block is exact only with this source shape.
DC_ADDRESS(0x0d9ca4, 0x8)
MAC_ADDRESS(0x10cfb0, 0x10)
void iconWidget::setIconSequence(int newSequence)
{
    m_seqId = newSequence;
    m_frame = 0;
}

// E:\gamedcs\iconwdgt.cpp:452
DC_ADDRESS(0x0d9cac, 0x32)
MAC_ADDRESS(0x10cfc0, 0x60)
void iconWidget::setPalette(const char* paletteName)
{
    TPalette16* newPalette = ResourceManager::GetPalette(paletteName);
    if (newPalette) {
        m_sprite->SetPalette(newPalette->Palette);
        ResourceManager::Dispose(newPalette);
    }
}

// E:\gamedcs\iconwdgt.cpp:462
DC_ADDRESS(0x0d9ce0, 0x84)
MAC_ADDRESS(0x10d020, 0x54)
void iconWidget::setPlayerPaletteColors(int whichPlayer)
{
    ::setPlayerPaletteColors(m_sprite->GetPalette(), whichPlayer);
    // DC 465 calls the non-const GetPalette24 reference accessor.
    ::setPlayerPaletteColors(m_sprite->GetPalette24(), whichPlayer);
}

VA(0x004eb030, 0x22)
DC_ADDRESS(0x0d9d64, 0x2a)
MAC_ADDRESS(0x10d074, 0x5c)
void iconWidget::setSprite(const char* newSprite)
{
    if (m_sprite)
        ResourceManager::Dispose(m_sprite);
    m_sprite = ResourceManager::GetSprite(newSprite);
}

VA(0x004eb060, 0x1EB)
DC_ADDRESS(0x0d9d90, 0x158)
MAC_ADDRESS(0x10d0d0, 0x210)
void iconWidget::nextRandomFrame()
{
    if (m_frame + 1 < m_sprite->GetNumFrames(m_seqId)) {
        setIconFrame(m_frame + 1);
        return;
    }
    int chosen;
    if (m_seqId == cs_prewalk) {
        chosen = cs_walk;
    } else if (m_seqId == cs_postwalk) {
        chosen = m_postPostWalkSequence;
    } else {
        do {
            const struct {
                creature_seqid m_sequenceId;
                int m_chance;
            } sequenceList[11] = {
                {cs_walk, 65},
                {cs_wait, 15},
                {cs_defend, 5},
                {cs_fidget, 5},
                {cs_wince, 4},
                {cs_attack_ur, 1},
                {cs_attack_r, 1},
                {cs_attack_dr, 1},
                {cs_range_ur, 1},
                {cs_range_r, 1},
                {cs_range_dr, 1},
            };
            int roll = random(1, 100);
            int cumulative = 0;
            unsigned int pick = 0;
            for (; pick < 11; ++pick) {
                cumulative += sequenceList[pick].m_chance;
                if (roll <= cumulative)
                    break;
            }
            chosen = sequenceList[pick].m_sequenceId;
        } while (m_sprite->GetNumFrames(chosen) <= 0);
        if (chosen == cs_walk && m_seqId != cs_walk
            && m_sprite->GetNumFrames(cs_prewalk) > 0) {
            chosen = cs_prewalk;
        } else if (chosen != cs_walk && m_seqId == cs_walk
                   && m_sprite->GetNumFrames(cs_postwalk) > 0) {
            m_postPostWalkSequence = chosen;
            chosen = cs_postwalk;
        }
    }
    setIconSequence(chosen);
}

VA(0x004eb250, 0xED)
DC_ADDRESS(0x0d9ee8, 0xac)
MAC_ADDRESS(0x10d2e0, 0x190)
void iconWidget::nextRandomSiegeEngineFrame()
{
    if (m_frame + 1 < m_sprite->GetNumFrames(m_seqId)) {
        setIconFrame(m_frame + 1);
        return;
    }
    int chosen;
    do {
        const struct {
            creature_seqid m_sequenceId;
            int m_chance;
        } sequenceList[4] = {
            {cs_wait, 94},
            {cs_range_ur, 2},
            {cs_range_r, 2},
            {cs_range_dr, 2},
        };
        int roll = random(1, 100);
        int cumulative = 0;
        unsigned int pick = 0;
        for (; pick < 4; ++pick) {
            cumulative += sequenceList[pick].m_chance;
            if (roll <= cumulative)
                break;
        }
        chosen = sequenceList[pick].m_sequenceId;
    } while (m_sprite->GetNumFrames(chosen) <= 0);
    setIconSequence(chosen);
}
