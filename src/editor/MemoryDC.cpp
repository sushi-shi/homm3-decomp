// MemoryDC.cpp - the editor's memory DC errors and GDI helpers (h3maped
// 0x486f98..0x487715, name inferred). drawTransparentBitmap masks the
// transparent colour out of the bitmap and the bitmap out of the
// background in memory, then copies the combination in one blit.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/GDIObjectSelector.h"
#include "editor/MemoryDC.h"

VA(0x00486fb4, 0x6a)
COLORREF getTransparentColor(CBitmap* pBitmap)
{
    CDC dc;
    dc.Attach(::CreateCompatibleDC(NULL));
    TGDIObjectSelector<CBitmap> selector(&dc, pBitmap);
    return ::GetPixel(dc.m_hDC, 0, 0);
}

VA(0x0048701e, 0x12)
TMemoryDC::TError::TError()
{
}

VA(0x00487030, 0x12)
TGDIObjectSelectorError::TGDIObjectSelectorError()
{
}

VA(0x00487042, 0x454)
void drawTransparentBitmap(CDC* pDC, CBitmap* pBitmap, int x, int y, COLORREF transparentColor)
{
    BITMAP bitmap;
    pBitmap->GetBitmap(&bitmap);
    CSize size(bitmap.bmWidth, bitmap.bmHeight);
    CDC imageDC;
    CDC inverseMaskDC;
    CDC maskDC;
    CDC backgroundDC;
    CDC foregroundDC;
    CBitmap maskBitmap;
    CBitmap inverseMaskBitmap;
    CBitmap backgroundBitmap;
    CBitmap foregroundBitmap;
    try {
        if (!imageDC.Attach(::CreateCompatibleDC(pDC->GetSafeHdc())))
            return;
        TGDIObjectSelector<CBitmap> imageSelector(&imageDC, pBitmap);
        if (!inverseMaskDC.Attach(::CreateCompatibleDC(pDC->GetSafeHdc())))
            return;
        if (!maskBitmap.Attach(::CreateBitmap(size.cx, size.cy, 1, 1, NULL)))
            return;
        TGDIObjectSelector<CBitmap> inverseMaskSelector(&inverseMaskDC, &maskBitmap);
        if (!maskDC.Attach(::CreateCompatibleDC(pDC->GetSafeHdc())))
            return;
        if (!inverseMaskBitmap.Attach(::CreateBitmap(size.cx, size.cy, 1, 1, NULL)))
            return;
        TGDIObjectSelector<CBitmap> maskSelector(&maskDC, &inverseMaskBitmap);
        if (!backgroundDC.Attach(::CreateCompatibleDC(pDC->GetSafeHdc())))
            return;
        if (!backgroundBitmap.Attach(::CreateCompatibleBitmap(pDC->m_hDC, size.cx, size.cy)))
            return;
        TGDIObjectSelector<CBitmap> backgroundSelector(&backgroundDC, &backgroundBitmap);
        if (!foregroundDC.Attach(::CreateCompatibleDC(pDC->GetSafeHdc())))
            return;
        if (!foregroundBitmap.Attach(::CreateCompatibleBitmap(pDC->m_hDC, size.cx, size.cy)))
            return;
        TGDIObjectSelector<CBitmap> foregroundSelector(&foregroundDC, &foregroundBitmap);
        COLORREF oldBkColor = imageDC.SetBkColor(transparentColor);
        maskDC.BitBlt(0, 0, size.cx, size.cy, &imageDC, 0, 0, SRCCOPY);
        imageDC.SetBkColor(oldBkColor);
        inverseMaskDC.BitBlt(0, 0, size.cx, size.cy, &maskDC, 0, 0, NOTSRCCOPY);
        backgroundDC.BitBlt(0, 0, size.cx, size.cy, pDC, x, y, SRCCOPY);
        backgroundDC.BitBlt(0, 0, size.cx, size.cy, &maskDC, 0, 0, SRCAND);
        foregroundDC.BitBlt(0, 0, size.cx, size.cy, &imageDC, 0, 0, SRCCOPY);
        foregroundDC.BitBlt(0, 0, size.cx, size.cy, &inverseMaskDC, 0, 0, SRCAND);
        backgroundDC.BitBlt(0, 0, size.cx, size.cy, &foregroundDC, 0, 0, SRCPAINT);
        pDC->BitBlt(x, y, size.cx, size.cy, &backgroundDC, 0, 0, SRCCOPY);
    } catch (...) {
    }
}

VA(0x00487521, 0x22)
void drawTransparentBitmap(CDC* pDC, CBitmap* pBitmap, int x, int y)
{
    drawTransparentBitmap(pDC, pBitmap, x, y, getTransparentColor(pBitmap));
}

VA(0x00487543, 0x72)
bool isPixelColor(CBitmap* pBitmap, int x, int y, COLORREF color)
{
    CDC dc;
    dc.Attach(::CreateCompatibleDC(NULL));
    TGDIObjectSelector<CBitmap> selector(&dc, pBitmap);
    return ::GetPixel(dc.m_hDC, x, y) == color;
}

VA(0x004875b5, 0x1f)
bool isTransparentPixel(CBitmap* pBitmap, int x, int y)
{
    return isPixelColor(pBitmap, x, y, getTransparentColor(pBitmap));
}

VA(0x004875d4, 0x3f)
unsigned int getFirstMenuItem(CMenu* pMenu)
{
    MENUITEMINFO info;
    info.cbSize = sizeof(info);
    info.fMask = MIIM_TYPE;
    info.dwTypeData = NULL;
    unsigned int index = 0;
    while (info.cch = 0, ::GetMenuItemInfo(pMenu->m_hMenu, index, TRUE, &info), info.fType & MFT_SEPARATOR)
        index++;
    return index;
}

VA(0x00487613, 0xbb)
bool setMenuItemText(CMenu* pMenu, unsigned int& index, CString text)
{
    MENUITEMINFO info;
    info.cbSize = sizeof(info);
    info.fMask = MIIM_TYPE;
    info.dwTypeData = NULL;
    info.cch = 0;
    ::GetMenuItemInfo(pMenu->m_hMenu, index, TRUE, &info);
    info.dwTypeData = text.GetBuffer(0);
    ::SetMenuItemInfo(pMenu->m_hMenu, index, TRUE, &info);
    text.ReleaseBuffer();
    info.dwTypeData = NULL;
    for (;;) {
        index++;
        if (index >= (unsigned int)::GetMenuItemCount(pMenu->m_hMenu))
            return false;
        info.cch = 0;
        ::GetMenuItemInfo(pMenu->m_hMenu, index, TRUE, &info);
        if (!(info.fType & MFT_SEPARATOR))
            return true;
    }
}
