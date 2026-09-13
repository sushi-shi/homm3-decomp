// iconwdgt.h - prototypes of iconwdgt.cpp (compiland iconwdgt.obj)
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

    // WHAT MOVES IT IS A BUDGET OF FOUR, not "a single enumerator" -
    // corrected 2026-08-14 by bisection after RESOURCE_TYPE_FONT = 80
    // was appended for font::font 0x4b5070 and nothing in the tree
    // moved. With that enumerator in place, adding N more probe
    // enumerators immediately before RESOURCE_TYPE_SFX gives
    //     N = 1,2,3,4 -> 90.8376 (inert)
    //     N = 5,6,8,10 -> 88.2360
    // and the step is the same whether the names are appended past the
    // last value or inserted among the existing ones, so it is the
    // COUNT that matters, not the position. The earlier note here read
    // its own sweep as flat from N = 1; it was not, and the low counts
    // were never the ones that fired.

    // Practical consequence: three more enumerators can be added to
    // EResourceType for free, but this ten-name block still cannot
    // move. Scoping the names to iconWidget keeps them collision-free
    // for the lane that eventually does add them, once
    // recruitUnit::Update is closed.
    enum ESpriteResType {
        SPRITE_RES_SPRITE = 64,
        SPRITE_RES_SPRITEDEF = 65,
        SPRITE_RES_CREATURE = 66,
        SPRITE_RES_ADVOBJ = 67,
        SPRITE_RES_HERO = 68,
        SPRITE_RES_TILESET = 69,
        SPRITE_RES_POINTER = 70,
        SPRITE_RES_INTERFACE = 71,
        SPRITE_RES_SPRITEFRAME = 72,
        SPRITE_RES_COMBAT_HERO = 73
    };

    CSprite* m_sprite;
    int m_frame;
    int m_seqId;
    unsigned char m_isFlipped;
    int m_postPostWalkSequence;
    unsigned short m_backColor;

    iconWidget(int x, int y, int w, int h, int id, const char* image,
               int frame, int sequence, unsigned char flipped,
               unsigned backColor, int style);
    virtual ~iconWidget();  // retail 0x4ea7b0
    virtual int main(message& msg);
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    virtual void draw() const;
    virtual int getRealHeight() const;
    virtual int getRealWidth() const;
    virtual unsigned char handleClick(unsigned char downClick,
                                       unsigned char rightClick);

    void setIconFrame(int newFrame);
    void setIconSequence(int newSequence);
    void setPalette(const char* paletteName);
    void setPlayerPaletteColors(int whichPlayer);
    void setSprite(const char* newSprite);
    void nextRandomFrame();
    void nextRandomSiegeEngineFrame();
};

// --- CSprite ---
// CODEVIEW(E:\gamedcs\csprite.h:378, dc 0xd9f98) void CSprite::DrawPointer(int framenum, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip);

// --- iconWidget ---
// CODEVIEW(E:\gamedcs\iconwdgt.cpp:35, dc 0xd92fc) void iconWidget::iconWidget();
// CODEVIEW(E:\gamedcs\iconwdgt.cpp:75, dc 0xd93f4) void iconWidget::initialize(int x, int y, int w, int h, int id, const char* image, int frame, int sequence, unsigned char flipped, unsigned back_color, int style, unsigned char focusable);
// CODEVIEW(E:\gamedcs\iconwdgt.cpp:119, dc 0xd94a4) int iconWidget::Main(message* msg);
// CODEVIEW(E:\gamedcs\iconwdgt.cpp:275, dc 0xd96e4) void iconWidget::zBufferDraw();
// CODEVIEW(E:\gamedcs\iconwdgt.cpp:444, dc 0xd9ca4) void iconWidget::SetIconSequence(int new_sequence);
// CODEVIEW(E:\gamedcs\iconwdgt.cpp:452, dc 0xd9cac) void iconWidget::SetPalette(const char* palette_name);
// CODEVIEW(E:\gamedcs\iconwdgt.cpp:462, dc 0xd9ce0) void iconWidget::SetPlayerPaletteColors(int whichPlayer);
// CODEVIEW(E:\gamedcs\iconwdgt.cpp:41, dc 0xda018) void* iconWidget::`scalar deleting destructor'(unsigned __flags);

// --- resource ---
// CODEVIEW(E:\gamedcs\resrce.h:33, dc 0xd9f94) EResourceType resource::get_resType();

#endif  /* HOMM3_ICONWDGT_H */
