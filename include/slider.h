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
    CSprite* sliderSprite;          // +0x30
    Bitmap816* sliderBitmap;        // +0x34
    int oldState;                   // +0x38
    int currentState;               // +0x3c
    int knobPos;                    // +0x40
    int knobRange;                  // +0x44
    int numStates;                  // +0x48
    int length;                     // +0x4c
    int pageSize;                   // +0x50
    long knob_start;                // +0x54
    short clickX;                   // +0x58
    short clickY;                   // +0x5a
    unsigned char hotKeys;          // +0x5c
    unsigned char scrolling;        // +0x5d
    unsigned char pad_5e[2];
    int lastFocus;                  // +0x60
    TSliderFunction sliderFunction; // +0x64

    slider();
    slider(int x, int y, int w, int h, int id, int num,
           TSliderFunction func, EGraphics graphics, int page,
           unsigned char hotKey);
    virtual ~slider();

    virtual int Main(message* msg);                 // slot 2
    virtual void zBufferDraw(unsigned short* zBuffer, int id); // slot 3
    virtual void Draw();                            // slot 4
    virtual int GetRealHeight();                    // slot 5
    virtual int GetRealWidth();                     // slot 6
    virtual void enable(unsigned char on);          // slot 9
    virtual void OnSetFocus();                      // slot 10
    virtual void OnKillFocus();                     // slot 11
    virtual void SetResolution(int num);            // slot 13
    virtual void SetState(int state);               // slot 14
    virtual void UpdateResolution(int num);         // slot 15
    // Retail-only slot 16. The vtable points at the same empty `ret` body as
    // widget::Close (0x5bc690); no independent source body is claimable.
    virtual void Close();

    int get_maximum() const { return numStates; }
    int get_state() const { return currentState; }
    int Select(message* msg, unsigned char dragging);
    int Deselect(message* msg);
    void KeyAccel(int x1, int x2, int x3, int x4, int key);

protected:
    void initialize(const char* resource_name);
    void SetKnob(int inX);
};
SIZE(slider, 0x68);

#endif  /* HOMM3_SLIDER_H */
