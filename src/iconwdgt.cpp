// iconwdgt.cpp - E:\gamedcs\iconwdgt.cpp (compiland iconwdgt.obj)
// 20 functions in link order.
#include "terrain.h"
#include <va.h>
#include "iconwdgt.h"
#include "button.h"
#include "csprite.h"
#include "csequence.h"
#include "cspriteframe.h"
#include "message.h"
#include "palette.h"
#include "resourcemanager.h"
#include "window.h"
#include "winmgr.h"

int random(int min, int max);

#if 0  // @carcass

// E:\gamedcs\iconwdgt.cpp:35
DC_ONLY(0xd92fc, 0x54)
void IconWidget::IconWidget()
{
    // @stub
}

// E:\gamedcs\iconwdgt.cpp:88

#endif  // @carcass

VA_COMPGEN(0x004ea6f0, 0x21, SCALAR_DELETING_DTOR, IconWidget)

VA(0x004ea720, 0x8C)  // dc 0xd9350
IconWidget::IconWidget(int x, int y, int w, int h, int id, const char* image,
                       int frame, int sequence, bool flipped,
                       unsigned backColor, int style)
    : Widget(x, y, w, h, id, style),
      m_frame(frame),
      m_seqId(sequence),
      m_isFlipped(flipped),
      m_backColor(static_cast<unsigned short>(backColor)),
      m_postPostWalkSequence(cs_wait)
{
    m_sprite = image ? ResourceManager::getSprite(image) : 0;
}

VA(0x004ea7b0, 0x55)  // dc 0xd9464
IconWidget::~IconWidget()
{
    if (m_sprite)
        m_sprite->dispose();
}

// E:\gamedcs\iconwdgt.cpp:119
// Retail fixes the message protocol and six widget-command arms. DC 151
// calls SetIconSequence; its 447..448 stores are seqId then Frame. DC 159
// calls the ordinary SetPalette helper; restore that canonical call rather
// than its copied load/dispose body. Both helpers expand naturally here.

// DC 190..217 and 229..245 retain nested mouse-hit and selected scopes.
// The positive selected scope lets VC6 merge the zero epilogues. Together
// with positive hit testing, all five old goto returns become direct exits
// and Main improves 95.7040% to 99.9639%. The whole 756-byte body and all
// relocation references agree except two scratch-register operands:
// +0x113 movsx and +0x117 mov use EDX where retail uses ECX for widget id.
// All-direct returns with the old negative selected guard lose score;
// moving default/base delegation into separate switch arms gives at most
// 91.8050%. Neither failed form establishes an unavoidable source goto.
VA(0x004ea810, 0x2F4)  // vtable 0x63ec48 slot 2, dc 0xd94a4
int IconWidget::main(Message& msg)
{
    if (m_sleepCount > 0) {
        return 0;
    }

    if (!(m_status & WIDGET_ACTIVE)) {
        if (msg.m_id == MESSAGE_WIDGET)
            return Widget::main(msg);
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
            if (handleClick(false, false))
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

    return Widget::main(msg);
}

#if 0  // @carcass

// E:\gamedcs\iconwdgt.cpp:275
DC_ONLY(0xd96e4, 0x4)
void IconWidget::zBufferDraw()
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\iconwdgt.cpp:257
// Promoted from DC_ONLY 2026-08-08 on four independent corroborations:
// the row is inside iconwdgt.obj's own carve span, it holds the DC
// roster's handle_click slot in order (immediately before GetRealWidth
// and GetRealHeight, exactly as at 0x4eab20 / 0x4eab30), the iconWidget
// vtable at 0x63ec48 stores it in slot 13, and the body's `ret 8`
// matches the DC's three parameters (this + two). Both arguments are
// dead - retail returns a bare zero.
// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x004eab10, 0x5)  // anchor-vtable (slot 13 of 0x63ec48), dc 0xd96b8
bool IconWidget::handleClick(bool downClick, bool rightClick)
{
    return 0;
}

VA(0x004eab20, 0x7)  // dc 0xd96bc
int IconWidget::getRealWidth() const
{
    return m_sprite->getWidth();
}

VA(0x004eab30, 0x7)  // dc 0xd96d0
int IconWidget::getRealHeight() const
{
    return m_sprite->getHeight();
}

// The creature path is the one with real arithmetic: it centres on the
// crop box of the FIRST FRAME OF THE cs_wait SEQUENCE (Sprite->s[2]
// ->f[0]), puts the portrait 275 rows up from the widget's bottom edge,
// and then clips the source rectangle on all four sides - each negative
// offset moving into sx/sy and shrinking sw/sh, each overrun clamping
// sw/sh against the widget box.
VA(0x004eab40, 0x4B0)  // dc 0xd96e8
void IconWidget::draw() const
{
    int drawX = m_x + m_parentWindow->m_x;
    int drawY = m_y + m_parentWindow->m_y;

    switch (m_style) {
    case ICON_STYLE_PLAIN:
        switch (m_sprite->getResType()) {
        case RESOURCE_TYPE_SPRITE:
            m_sprite->draw(m_seqId, m_frame, 0, 0, m_sprite->getWidth(), m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped, 1);
            break;
        case RESOURCE_TYPE_CREATURE:
            m_sprite->drawCreature(m_seqId, m_frame, 0, 0, m_sprite->getWidth(),
                m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped, 0);
            break;
        case RESOURCE_TYPE_ADVENTURE_OBJECT:
            m_sprite->drawAdvObj(m_frame, 0, 0, m_sprite->getWidth(), m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_HERO:
            m_sprite->drawHero(m_seqId, m_frame, 0, 0, m_sprite->getWidth(),
                m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_TILESET:
            m_sprite->drawTile(m_frame, 0, 0, m_sprite->getWidth(), m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped, 0);
            break;
        case RESOURCE_TYPE_POINTER:
            m_sprite->drawPointer(m_frame, g_windowManager->m_screenBitmap,
                drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_INTERFACE:
            m_sprite->drawInterface(m_frame, 0, 0, m_sprite->getWidth(),
                m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_COMBAT_HERO:
            m_sprite->drawCombatHero(m_seqId, m_frame, 0, 0, m_sprite->getWidth(),
                m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        }
        break;

    case ICON_STYLE_CENTERED:
        if (m_sprite->getWidth() < m_width)
            drawX += (m_width - m_sprite->getWidth()) / 2;
        if (m_sprite->getHeight() + 2 < m_height)
            drawY += m_height - m_sprite->getHeight() - 2;
        switch (m_sprite->getResType()) {
        case RESOURCE_TYPE_SPRITE:
            m_sprite->draw(m_seqId, m_frame, 0, 0, m_sprite->getWidth(), m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped, 1);
            break;
        case RESOURCE_TYPE_CREATURE:
            m_sprite->drawCreature(m_seqId, m_frame, 0, 0, m_sprite->getWidth(),
                m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped, 0);
            break;
        case RESOURCE_TYPE_ADVENTURE_OBJECT:
            m_sprite->drawAdvObj(m_frame, 0, 0, m_sprite->getWidth(), m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_HERO:
            m_sprite->drawHero(m_seqId, m_frame, 0, 0, m_sprite->getWidth(),
                m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_TILESET:
            m_sprite->drawTile(m_frame, 0, 0, m_sprite->getWidth(), m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped, 0);
            break;
        case RESOURCE_TYPE_POINTER:
            m_sprite->drawPointer(m_frame, g_windowManager->m_screenBitmap,
                drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_INTERFACE:
            m_sprite->drawInterface(m_frame, 0, 0, m_sprite->getWidth(),
                m_sprite->getHeight(),
                g_windowManager->m_screenBitmap, drawX, drawY, m_isFlipped);
            break;
        case RESOURCE_TYPE_COMBAT_HERO:
            m_sprite->drawCombatHero(m_seqId, m_frame, 0, 0, m_sprite->getWidth(),
                m_sprite->getHeight(),
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
        sw = m_sprite->getWidth();
        sh = m_sprite->getHeight();
        offX = m_width / 2 - m_sprite->getCroppedWidth(cs_wait, 0) / 2 - m_sprite->getCroppedX(cs_wait, 0);
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
        m_sprite->drawCreature(m_seqId, m_frame, sx, sy, sw, sh,
            g_windowManager->m_screenBitmap, offX + drawX, offY + drawY, 0, 0);
        break;
    }

    }
}

VA(0x004eaff0, 0x3E)  // dc 0xd9c7c
void IconWidget::setIconFrame(int newFrame)
{
    m_frame = newFrame % m_sprite->getNumFrames(m_seqId);
}

// E:\gamedcs\iconwdgt.cpp:444
// Dreamcast line 447 stores new_sequence to seqId and line 448 clears Frame;
// Main's retail-inlined WIDGET_SET_ICON_SEQUENCE arm corroborates the order:
// its six-instruction block is exact only with this source shape.
DC_ONLY(0xd9ca4, 0x8)
void IconWidget::setIconSequence(int newSequence)
{
    m_seqId = newSequence;
    m_frame = 0;
}

// E:\gamedcs\iconwdgt.cpp:452
DC_ONLY(0xd9cac, 0x32)
void IconWidget::setPalette(const char* paletteName)
{
    Palette16* newPalette = ResourceManager::getPalette(paletteName);
    if (newPalette) {
        m_sprite->setPalette(newPalette->m_data);
        newPalette->dispose();
    }
}

// E:\gamedcs\iconwdgt.cpp:462
DC_ONLY(0xd9ce0, 0x84)
void IconWidget::setPlayerPaletteColors(int whichPlayer)
{
    ::setPlayerPaletteColors(m_sprite->getPalette(), whichPlayer);
    // DC 465 calls the non-const GetPalette24 reference accessor.
    ::setPlayerPaletteColors(m_sprite->getPalette24(), whichPlayer);
}

VA(0x004eb030, 0x22)  // dc 0xd9d64
void IconWidget::setSprite(const char* newSprite)
{
    if (m_sprite)
        m_sprite->dispose();
    m_sprite = ResourceManager::getSprite(newSprite);
}

VA(0x004eb060, 0x1EB)  // dc 0xd9d90
void IconWidget::nextRandomFrame()
{
    if (m_frame + 1 < m_sprite->getNumFrames(m_seqId)) {
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
                CreatureSeqid m_sequenceId;
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
        } while (m_sprite->getNumFrames(chosen) <= 0);
        if (chosen == cs_walk && m_seqId != cs_walk
            && m_sprite->getNumFrames(cs_prewalk) > 0) {
            chosen = cs_prewalk;
        } else if (chosen != cs_walk && m_seqId == cs_walk
                   && m_sprite->getNumFrames(cs_postwalk) > 0) {
            m_postPostWalkSequence = chosen;
            chosen = cs_postwalk;
        }
    }
    setIconSequence(chosen);
}

VA(0x004eb250, 0xED)  // dc 0xd9ee8
void IconWidget::nextRandomSiegeEngineFrame()
{
    if (m_frame + 1 < m_sprite->getNumFrames(m_seqId)) {
        setIconFrame(m_frame + 1);
        return;
    }
    int chosen;
    do {
        const struct {
            CreatureSeqid m_sequenceId;
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
    } while (m_sprite->getNumFrames(chosen) <= 0);
    setIconSequence(chosen);
}

#if 0  // @carcass

// E:\gamedcs\resrce.h:33
DC_ONLY(0xd9f94, 0x4)
ResourceType Resource::get_resType()
{
    // @stub
}

// E:\gamedcs\csprite.h:378
DC_ONLY(0xd9f98, 0x80)
void CSprite::drawPointer(int framenum, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip)
{
    // @stub
}

// E:\gamedcs\iconwdgt.cpp:41
DC_ONLY(0xda018, 0x34)
void* IconWidget::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass
