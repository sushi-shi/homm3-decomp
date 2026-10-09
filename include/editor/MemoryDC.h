// MemoryDC.h - the editor's memory DC and its GDI helpers (MemoryDC.cpp,
// name inferred from its alphabetical slot). RTTI names TMemoryDC (a CDC
// with a 29-slot vtable, built in the Map Editor.cpp span at 0x45c561)
// and its nested TError, which TMemoryDC throws when it cannot create its
// DC. The transparent blits treat one colour of a bitmap, by default its
// top-left pixel, as see-through; the menu helpers skip separators.
//
// The DC is compatible with another; its constructor is an inline
// emitted in Map Editor.cpp's span (0x45c540) and its destructor is CDC's.
#ifndef HOMM3_EDITOR_MEMORYDC_H
#define HOMM3_EDITOR_MEMORYDC_H

#include "exceptions.h"
#include "va.h"

class TMemoryDC : public CDC {
public:
    class TError : public TRuntimeError {
    public:
        TError();
    };

    VA(0x0045c540, 0x66)
    TMemoryDC(CDC* pDC)
    {
        if (!Attach(::CreateCompatibleDC(pDC->GetSafeHdc())))
            throw TError();
    }
};

COLORREF getTransparentColor(CBitmap* pBitmap);
void drawTransparentBitmap(CDC* pDC, CBitmap* pBitmap, int x, int y, COLORREF transparentColor);
void drawTransparentBitmap(CDC* pDC, CBitmap* pBitmap, int x, int y);
bool isPixelColor(CBitmap* pBitmap, int x, int y, COLORREF color);
bool isTransparentPixel(CBitmap* pBitmap, int x, int y);
unsigned int getFirstMenuItem(CMenu* pMenu);
bool setMenuItemText(CMenu* pMenu, unsigned int& index, CString text);

#endif  /* HOMM3_EDITOR_MEMORYDC_H */
