// textscroller.h - prototypes of textscroller.cpp, the Complete-era
// compiland that owns the scenario-description scroller.
// HAND-OWNED after admission.
//
// PROVISIONAL UNIT NAME. The Dreamcast roster has no module between
// text.obj and textntry.obj, and no DC compiland declares either class
// below, so the 0x5b9f80..0x5ba8cf block is Complete-only: its own
// cinit/atexit thunk opens it at 0x5b9f80 and textntry's closes it at
// 0x5ba8d0, and textwdgt.obj's own band starts far later at 0x5bc230.
// `textscroller` is the house name for it; `type_text_scroller` itself
// is the retail-attested class identity modelled in textwdgt.h.
#ifndef HOMM3_TEXTSCROLLER_H
#define HOMM3_TEXTSCROLLER_H

#include "slider.h"
#include "textwdgt.h"

// The scroller's private slider. Retail proves the whole shape from the
// constructor 0x5b9fb0 and the vtable 0x642cc8: a 0x6c-byte object whose
// slider base ctor runs with the ten ordinary slider arguments, whose
// vptr is then re-stored to 0x642cc8, and whose one extra dword at +0x68
// is the owning scroller. That vtable copies slider's sixteen inherited
// slots and overrides only slot 16, the state-change hook, at 0x5b9fa0.
class type_text_slider : public slider {
public:
    type_text_scroller* owner;  // +0x68

    type_text_slider(int x, int y, int w, int h, int id, int num,
                     TSliderFunction func, EGraphics graphics, int page,
                     unsigned char hotKey, type_text_scroller* scroller)
        : slider(x, y, w, h, id, num, func, graphics, page, hotKey),
          owner(scroller)
    {
    }

    virtual void Close();  // slot 16, retail 0x5b9fa0
};
SIZE(type_text_slider, 0x6c);

#endif  /* HOMM3_TEXTSCROLLER_H */
