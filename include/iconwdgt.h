// iconwdgt.h - iconwdgt.cpp (compiland iconwdgt.obj)
#ifndef HOMM3_ICONWDGT_H
#define HOMM3_ICONWDGT_H

#include "widget.h"
#include "csprite.h"

// Dreamcast roster shifted by the retail widget base (48): Sprite@0x30,
// Frame@0x34, seqId@0x38, IsFlipped@0x3c, PostPostWalkSequence@0x40,
// BackColor@0x44. Vtable 0x63ec48; the dtor Disposes the sprite.
class iconWidget : public widget {
public:
    // The three widget::style values iconWidget::Draw (0x4eab40)
    // dispatches on, byte-derived from its `sub eax,0x10 / dec / dec`
    // chain at 0x4eab64 - anything else draws nothing. NAMES ARE A
    // BOOTSTRAP INVENTION (no DC enum exists) but the roles are
    // byte-fixed: PLAIN draws at the widget origin; CENTERED first
    // centres the sprite in the widget rect (horizontally by half the
    // slack, vertically flush to the bottom less two); CREATURE runs
    // the clipped portrait path that measures itself off the cs_wait
    // frame's crop box.
    enum EIconStyle {
        ICON_STYLE_PLAIN = 0x10,
        ICON_STYLE_CENTERED = 0x11,
        ICON_STYLE_CREATURE = 0x12
    };

    // The EResourceType values Draw's two jump tables span: DC
    // RType_sprite..RType_combat_hero, 64..73 (evidence/dreamcast/
    // enums.csv), byte-proven contiguous by those tables' `resType - 64`
    // bound of 9 and by each arm calling the CSprite entry point its
    // type is named after. SPRITEDEF (65) and SPRITEFRAME (72) are the
    // two that fall to the default, which is why both tables carry a
    // default at index 1 and index 8; they are listed because that gap
    // is what fixes the span.

    // These belong in resource.h's EResourceType and are deliberately
    // NOT there yet. resource.h sits at the head of recruit.obj's
    // include closure and recruitUnit::Update is knife-edge on
    // symbol-handle position, dropping 90.8376% -> 88.2360% when the
    // enum grows (the same-sized edit made to csprite.h instead is
    // inert).

    // Practical consequence: three more enumerators can be added to
    // EResourceType for free, but this ten-name block still cannot
    // move. Scoping the names to iconWidget keeps them collision-free
    // for the lane that eventually does add them, once
    // recruitUnit::Update is closed.

    CSprite* m_sprite;
    int m_frame;
    int m_seqId;
    bool m_isFlipped;
    int m_postPostWalkSequence;
    unsigned short m_backColor;

    iconWidget(int x, int y, int w, int h, int id, const char* image,
               int frame, int sequence, bool flipped,
               unsigned backColor, int style);
    virtual ~iconWidget();  // retail 0x4ea7b0
    virtual int main(message& msg);
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw() const;
    virtual int getRealHeight() const;
    virtual int getRealWidth() const;
    virtual bool handleClick(bool downClick,
                                       bool rightClick);

    void setIconFrame(int newFrame);
    void setIconSequence(int newSequence);
    void setPalette(const char* paletteName);
    void setPlayerPaletteColors(int whichPlayer);
    void setSprite(const char* newSprite);
    void nextRandomFrame();
    void nextRandomSiegeEngineFrame();
};

#endif  /* HOMM3_ICONWDGT_H */
