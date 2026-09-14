// slider.h - retail slider widget (compiland slider.obj)
// and signatures; the Complete x86 bodies and vtable prove the retail layout.
#ifndef HOMM3_SLIDER_H
#define HOMM3_SLIDER_H

#include "widget.h"

class Bitmap816;
class CSprite;

// Before normalization (type): slider.
class Slider : public Widget {
public:
// Before normalization (type): slider::EGraphics.
    enum Graphics {
        BROWN = 0,
        BLUE = 1
    };

    typedef void (*TSliderFunction)(int, HeroWindow*);

    // Retail reordered the Dreamcast fields after widget. Every offset below
    // is read or written by 0x596050..0x597184; the resulting size is 0x68.

protected:
    CSprite* m_sliderSprite;          // +0x30
    Bitmap816* m_sliderBitmap;        // +0x34
    int m_oldState;                   // +0x38

public:
    int m_currentState;               // +0x3c
    int m_knobPos;                    // +0x40

protected:
    int m_knobRange;                  // +0x44

public:
    int m_numStates;                  // +0x48

protected:
    int m_length;                     // +0x4c

public:
    int m_pageSize;                   // +0x50

protected:
    long m_knobStart;                // +0x54
    short m_clickX;                   // +0x58
    short m_clickY;                   // +0x5a
    unsigned char m_hotKeys;          // +0x5c
    unsigned char m_scrolling;        // +0x5d

public:
    // Dreamcast declares hotKeys/scrolling as bytes followed by a
    // word-aligned lastFocus. Retail shifts them to +0x5c/+0x5d and +0x60;
    // NH3API also leaves +0x5e/+0x5f unnamed.
    unsigned char m_paddingBeforeLastFocus[2];

    Slider();
    Slider(int x, int y, int w, int h, int id, int num,
           TSliderFunction func, Graphics graphics, int page,
           unsigned char hotKey);
    virtual ~Slider();

    virtual int main(Message& msg);                 // slot 2
    virtual void zBufferDraw(unsigned short* zBuffer, int id) const; // slot 3
    virtual void draw() const;                            // slot 4
    virtual int getRealHeight() const;                    // slot 5
    virtual int getRealWidth() const;                     // slot 6
    virtual void enable(unsigned char on);          // slot 9
    virtual void onSetFocus();                      // slot 10
    virtual void onKillFocus();                     // slot 11
    virtual void setResolution(int num);            // slot 13
    virtual void setState(int state);               // slot 14
    virtual void updateResolution(int num);         // slot 15

protected:
    int m_lastFocus;                  // +0x60
    TSliderFunction m_sliderFunction; // +0x64

public:
    // Retail-only slot 16. The vtable points at the same empty `ret` body as
    // widget::Close (0x5bc690); no independent source body is claimable.
    virtual void close();

    int getMaximum() const { return m_numStates; }
    // No procedure/source location or active caller is known;
    // retain the API without borrowing get_maximum's body position.
    int getState() const;
    int select(Message* msg, unsigned char dragging);
    int deselect(Message* msg);
    void keyAccel(int x1, int x2, int x3, int x4, int key);

protected:
    void initialize(const char* resourceName);
    void setKnob(int inX);
};
SIZE(Slider, 0x68);

#endif  /* HOMM3_SLIDER_H */
