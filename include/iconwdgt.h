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

    // Resource dispatch uses resrce.h's EResourceType (DC RType_sprite
    // through RType_combat_hero, 64..73). The former ESpriteResType copy
    // was an unsupported workaround for recruit's compiler-state score.

    CSprite* m_sprite;
    int m_frame;
    int m_seqId;
    // Before normalization: IsFlipped.
    // DC stores a lowered T_UCHAR record, but the constructor public symbol
    // (dc0xd9350, ...HH_NIH1@Z) encodes a bool input. Retail Draw forwards
    // this field to native-bool sprite methods without test/setne; bool
    // restores that behavior while keeping the one-byte field layout.
    bool m_isFlipped;
    // Before normalization: PostPostWalkSequence.
    int m_postPostWalkSequence;
    unsigned short m_backColor;

    iconWidget(int x, int y, int w, int h, int id, const char* image,
               int frame, int sequence, bool flipped,
               unsigned backColor, int style);
    virtual ~iconWidget();  // retail 0x4ea7b0
    virtual int main(message& msg);
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const;
    // Before normalization (function): iconWidget::Draw.
    virtual void draw() const;
    // Overrides of widget's two size slots; retail 0x4eab30 / 0x4eab20
    // (vtable 0x63ec48 slots 5 and 6). Both answer with the sprite's
    // own extent, not the widget rect.
    virtual int getRealHeight() const;  // slot 5, retail 0x4eab30
    virtual int getRealWidth() const;   // slot 6, retail 0x4eab20
    // Slot 13 of 0x63ec48, i.e. a virtual iconWidget ADDS on top of
    // widget's twelve-plus-_vslot12 - not an override. That is what the
    // vtable widths say: button and type_func_button stop at 13 slots
    // (evidence/vtables/named.tsv) where iconWidget has 14, so no
    // common base declares it. `border` declares its own twin
    // (dc 0x54590, same four-byte "return 0" body) and /OPT:ICF folded
    // the two onto this one row - which is why the carve labels it
    // border_vslot13.
    // Before normalization (function): iconWidget::handle_click.
    // Before normalization (locals): down_click, right_click.
    // DC public UAA_N_N0 proves bool for the result and both click flags.
    virtual bool handleClick(bool downClick, bool rightClick);

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
