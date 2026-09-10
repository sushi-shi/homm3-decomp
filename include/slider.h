// slider.h - retail slider widget (compiland slider.obj)
// HAND-OWNED after admission. Dreamcast CodeView supplies the public names
// and signatures; the Complete x86 bodies and vtable prove the retail layout.
#ifndef HOMM3_SLIDER_H
#define HOMM3_SLIDER_H

#include "widget.h"

class Bitmap816;
class CSprite;

class slider : public widget {
public:
    enum EGraphics {
        BROWN = 0,
        BLUE = 1
    };

    typedef void (*TSliderFunction)(int, heroWindow*);

    // Retail reordered the Dreamcast fields after widget. Every offset below
    // is read or written by 0x596050..0x597184; the resulting size is 0x68.
    // Before normalization: sliderSprite.
    CSprite* m_sliderSprite;          // +0x30
    // Before normalization: sliderBitmap.
    Bitmap816* m_sliderBitmap;        // +0x34
    // Before normalization: oldState.
    int m_oldState;                   // +0x38
    // Before normalization: currentState.
    int m_currentState;               // +0x3c
    // Before normalization: knobPos.
    int m_knobPos;                    // +0x40
    // Before normalization: knobRange.
    int m_knobRange;                  // +0x44
    // Before normalization: numStates.
    int m_numStates;                  // +0x48
    // Before normalization: length.
    int m_length;                     // +0x4c
    // Before normalization: pageSize.
    int m_pageSize;                   // +0x50
    // Before normalization: knob_start.
    long m_knobStart;                // +0x54
    // Before normalization: clickX.
    short m_clickX;                   // +0x58
    // Before normalization: clickY.
    short m_clickY;                   // +0x5a
    // Before normalization: hotKeys.
    unsigned char m_hotKeys;          // +0x5c
    // Before normalization: scrolling.
    unsigned char m_scrolling;        // +0x5d
    // Before normalization: pad_5e.
    // Dreamcast declares hotKeys/scrolling as bytes followed by a
    // word-aligned lastFocus. Retail shifts them to +0x5c/+0x5d and +0x60;
    // NH3API also leaves +0x5e/+0x5f unnamed.
    unsigned char m_paddingBeforeLastFocus[2];
    // Before normalization: lastFocus.
    int m_lastFocus;                  // +0x60
    // Before normalization: sliderFunction.
    TSliderFunction m_sliderFunction; // +0x64

    slider();
    slider(int x, int y, int w, int h, int id, int num,
           TSliderFunction func, EGraphics graphics, int page,
           unsigned char hotKey);
    virtual ~slider();

    // Before normalization (function): slider::Main.
    virtual int main(message& msg);                 // slot 2
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const; // slot 3
    // Before normalization (function): slider::Draw.
    virtual void draw();                            // slot 4
    // Before normalization (function): slider::GetRealHeight.
    virtual int getRealHeight();                    // slot 5
    // Before normalization (function): slider::GetRealWidth.
    virtual int getRealWidth();                     // slot 6
    virtual void enable(unsigned char on);          // slot 9
    // Before normalization (function): slider::OnSetFocus.
    virtual void onSetFocus();                      // slot 10
    // Before normalization (function): slider::OnKillFocus.
    virtual void onKillFocus();                     // slot 11
    // Before normalization (function): slider::SetResolution.
    virtual void setResolution(int num);            // slot 13
    // Before normalization (function): slider::SetState.
    virtual void setState(int state);               // slot 14
    // Before normalization (function): slider::UpdateResolution.
    virtual void updateResolution(int num);         // slot 15
    // Retail-only slot 16. The vtable points at the same empty `ret` body as
    // widget::Close (0x5bc690); no independent source body is claimable.
    // Before normalization (function): slider::Close.
    virtual void close();

    // Before normalization (function): slider::get_maximum.
    int getMaximum() const { return m_numStates; }
    // Before normalization (function): slider::get_state.
    int getState() const { return m_currentState; }
    // Before normalization (function): slider::Select.
    int select(message* msg, unsigned char dragging);
    // Before normalization (function): slider::Deselect.
    int deselect(message* msg);
    // Before normalization (function): slider::KeyAccel.
    void keyAccel(int x1, int x2, int x3, int x4, int key);

protected:
    // Before normalization (locals): resource_name.
    void initialize(const char* resourceName);
    // Before normalization (function): slider::SetKnob.
    void setKnob(int inX);
};
SIZE(slider, 0x68);

#endif  /* HOMM3_SLIDER_H */
