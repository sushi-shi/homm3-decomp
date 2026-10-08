// DIBSection.cpp - the editor's DIB section bitmaps (h3maped
// 0x40d0cd..0x40d6b1, name inferred). The pixels are the section's bits,
// top-down (negative height); releasing them deletes the section. The
// destructor releases them itself before TBitmap's destructor runs with
// TBitmap's release.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/DIBSection.h"

VA(0x0040d1c2, 0x28)
T16bppDIBSection::T16bppDIBSection()
{
}

VA(0x0040d1ea, 0x59)
T16bppDIBSection::T16bppDIBSection(unsigned int width, unsigned int height)
{
    create(width, height);
}

VA_COMPGEN(0x0040d173, 0x1c, SCALAR_DELETING_DTOR, T16bppDIBSection)

VA(0x0040d243, 0x70)
T16bppDIBSection::~T16bppDIBSection()
{
    destroy();
}

VA(0x0040d2b3, 0x18)
T16bppDIBSection& T16bppDIBSection::operator=(const T16bppDIBSection& other)
{
    if (&other != this)
        TBitmapBase<unsigned short>::operator=(other);
    return *this;
}

VA(0x0040d2cb, 0xb2)
unsigned short* T16bppDIBSection::allocate(unsigned int width, unsigned int height)
{
    BITMAPINFOHEADER header = { sizeof(BITMAPINFOHEADER), width, -(int)height, 1, 16, BI_RGB, 0, 0, 0, 0, 0 };
    CClientDC dc(NULL);
    void* pBits;
    HBITMAP hBitmap = ::CreateDIBSection(dc.GetSafeHdc(), (BITMAPINFO*)&header, DIB_RGB_COLORS,
                                         &pBits, NULL, 0);
    if (hBitmap == NULL)
        throw TRuntimeError();
    Attach(hBitmap);
    return (unsigned short*)pBits;
}

VA(0x0040d37d, 0x12)
void T16bppDIBSection::free(unsigned short* pPixels)
{
    ::DeleteObject(Detach());
}
