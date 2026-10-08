// stdafx.h - the editor's common header (Loki h3maped), included first by
// every editor object. Only the part the model objects prove is here: each
// object's static initializer builds the ten terrain masks of terrain.h
// (bitset<10>(1) << K, K = 0..9) before its own statics.
//
// The Loki port also keeps its MFC shim here, entirely inline: CPoint,
// CSize, CRect and CWnd over GTK+ 1.2 (CWnd::MoveWindow's assert names
// stdafx.h, and its text is emitted by every object that includes the
// header). The bodies are the linkonce copies the image keeps; the owners
// are the window objects (MapView.cpp, MapFrameWnd.cpp, MapEditWnd.cpp,
// MiniMapWnd.cpp, TileHRuler.cpp, ToolkitWnd.cpp, ObjectPaletteWnd.cpp)
// and cppbridge.cpp. CWnd is _m_hWnd (the assert texts' name), a capture
// flag, then the vtable pointer g++ places after them; its one virtual is
// OnCaptureChanged. Not proven: the capture flag's name, the parameter
// names and the order of the members within each class.
#ifndef HOMM3_EDITOR_STDAFX_H
#define HOMM3_EDITOR_STDAFX_H

#include <assert.h>

// The Windows SDK that gcc_prefix.h imports for the shared game source is
// not part of Loki's port: its FALSE/TRUE (0/1) would shadow GLib's (0)
// and (!FALSE) (MoveWindow's assert text is "(0)"), and its MessageBox
// macro would rename CWnd::MessageBox.
//
// Only gtkwidget.h (with gdk.h, GtkObject, GtkAdjustment and GtkStyle) is
// global: the dialogs and cppbridge.cpp include the rest of <gtk/gtk.h>
// inside an anonymous namespace, so their GtkCombo, GtkCTreeNode, GtkCList
// and GtkButton are {anonymous} types (ArmyDlg.cpp's findCreatureIndex
// mangles PQ2_GLOBAL_.N.ArmyDlg.cpp..9_GtkCombo; cppbridge.cpp's asserts
// print "struct GtkWidget * {anonymous}::_widget(char *)" beside
// "{anonymous}::GtkButton *").
#undef FALSE
#undef TRUE
#undef MessageBox
#include <gtk/gtkwidget.h>

#include "terrain.h"

// The sized integer names of the editor's source: __PRETTY_FUNCTION__
// texts spell uword (Tile.cpp's drawing functions take a uword* buffer)
// and assert texts spell numeric_limits< ubyte >.
typedef unsigned char ubyte;
typedef unsigned short uword;

// A 15-bit (5:5:5) editor colour: CWnd::TColorToGdkColor names it, and
// "TColor TGUIGameObject::miniMapColor(TTerrainType) const" returns it.
typedef uword TColor;

// The MFC shim's BOOL: __PRETTY_FUNCTION__ texts spell it
// ("BOOL TResourceQuantitiesDlg::OnInitDialog()") and such functions return
// it in %al. bool is assumed: the shim's own predicates and the dialogs'
// UpdateData(bool) take bool.
typedef bool BOOL;

// cppbridge.cpp: the modal message box behind CWnd::MessageBox, and the
// glade widget lookup ("struct GtkWidget * {anonymous}::_widget(char *)",
// exported unmangled).
extern "C" void doMessageBox(const char* message);
namespace {
extern "C" GtkWidget* _widget(char* name);
}
// cppbridge.cpp's allocated colours, which the rulers draw with, and the
// initialized red the mini map crosses out an empty map with.
extern GdkColor _m_white;
extern GdkColor _m_black;
extern GdkColor _m_red;

class CPoint {
public:
    CPoint() : x(0), y(0) {}
    CPoint(int initX, int initY) : x(initX), y(initY) {}

    bool operator==(const CPoint& point) const
    {
        if (x == point.x && y == point.y)
            return true;
        return false;
    }
    bool operator!=(const CPoint& point) const
    {
        if (x != point.x || y != point.y)
            return true;
        return false;
    }
    CPoint operator+(const CPoint& point) const { return CPoint(x + point.x, y + point.y); }
    CPoint operator-(const CPoint& point) const { return CPoint(x - point.x, y - point.y); }
    void operator+=(const CPoint& point)
    {
        x += point.x;
        y += point.y;
    }

    int x;
    int y;
};

class CSize {
public:
    CSize() : cx(0), cy(0) {}
    CSize(int initCX, int initCY) : cx(initCX), cy(initCY) {}

    int cx;
    int cy;
};

class CRect {
public:
    CRect() : left(0), top(0), right(0), bottom(0) {}
    CRect(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}
    CRect(const CPoint& point, const CSize& size)
        : left(point.x), top(point.y), right(point.x + size.cx), bottom(point.y + size.cy) {}

    int Width() const { return right - left; }
    int Height() const { return bottom - top; }
    CSize Size() const { return CSize(Width(), Height()); }
    CPoint& TopLeft() { return *new CPoint(left, top); }

    void DeflateRect(int x, int y) { DeflateRect(x, y, x, y); }
    void DeflateRect(int l, int t, int r, int b)
    {
        left += l;
        right -= r;
        top += t;
        bottom -= b;
    }

    bool IntersectRect(const CRect* pRect1, const CRect* pRect2)
    {
        GdkRectangle rect1;
        GdkRectangle rect2;
        GdkRectangle dest;
        bool bResult = false;
        CRect_to_gdk_rectangle(pRect1, &rect1);
        CRect_to_gdk_rectangle(pRect2, &rect2);
        if (gdk_rectangle_intersect(&rect1, &rect2, &dest) == TRUE) {
            gdk_rectangle_to_CRect(&dest, this);
            bResult = true;
        }
        return bResult;
    }
    bool IntersectRect(const CRect& rect1, const CRect& rect2) { return IntersectRect(&rect1, &rect2); }

    bool UnionRect(const CRect* pRect1, const CRect* pRect2)
    {
        GdkRectangle rect1;
        GdkRectangle rect2;
        GdkRectangle dest;
        CRect_to_gdk_rectangle(pRect1, &rect1);
        CRect_to_gdk_rectangle(pRect2, &rect2);
        gdk_rectangle_union(&rect1, &rect2, &dest);
        gdk_rectangle_to_CRect(&dest, this);
        return Width() > 0 && Height() > 0;
    }
    bool UnionRect(CRect& rect1, CRect& rect2) { return UnionRect(&rect1, &rect2); }

    static void CRect_to_gdk_rectangle(const CRect* pRect, GdkRectangle* pGdkRect)
    {
        pGdkRect->x = pRect->left;
        pGdkRect->y = pRect->top;
        pGdkRect->width = pRect->Width();
        pGdkRect->height = pRect->Height();
    }
    static void gdk_rectangle_to_CRect(const GdkRectangle* pGdkRect, CRect* pRect)
    {
        pRect->left = pGdkRect->x;
        pRect->top = pGdkRect->y;
        pRect->right = pGdkRect->width + pGdkRect->x;
        pRect->bottom = pGdkRect->height + pGdkRect->y;
    }

    int left;
    int top;
    int right;
    int bottom;
};

class CWnd {
public:
    CWnd() {}

    virtual void OnCaptureChanged(GtkWidget* pWidget) {}

    void MessageBeep(unsigned long type) { gdk_beep(); }
    void MessageBox(const char* text) { doMessageBox(text); }

    void MoveWindow(int x, int y, int width, int height)
    {
#line 295 "stdafx.h"
        assert(FALSE);
    }

    bool GetClientRect(CRect* pRect) const
    {
        bool bResult = false;
        pRect->left = pRect->top = 0;
        pRect->right = 0;
        pRect->bottom = 0;
        if (_m_hWnd) {
            pRect->bottom = _m_hWnd->allocation.height;
            pRect->right = _m_hWnd->allocation.width;
            bResult = true;
        }
        return bResult;
    }
    void GetWindowRect(CRect& rect) { GetClientRect(&rect); }

    void ClientToScreen(CRect* pRect) const
    {
        gint x;
        gint y;
        gdk_window_get_position(_m_hWnd->window, &x, &y);
        gint topX;
        gint topY;
        gdk_window_get_position(gtk_widget_get_toplevel(_m_hWnd)->window, &topX, &topY);
        x += topX;
        y += topY;
        pRect->top += y;
        pRect->bottom += y;
        pRect->left += x;
        pRect->right += x;
    }
    void ScreenToClient(CRect* pRect) const
    {
        gint x;
        gint y;
        gdk_window_get_position(_m_hWnd->window, &x, &y);
        gint topX;
        gint topY;
        gdk_window_get_position(gtk_widget_get_toplevel(_m_hWnd)->window, &topX, &topY);
        x += topX;
        y += topY;
        pRect->top -= y;
        pRect->bottom -= y;
        pRect->left -= x;
        pRect->right -= x;
    }
    void ClientToScreen(CPoint* pPoint) const
    {
        gint x;
        gint y;
        gdk_window_get_position(_m_hWnd->window, &x, &y);
        gint topX;
        gint topY;
        gdk_window_get_position(gtk_widget_get_toplevel(_m_hWnd)->window, &topX, &topY);
        x += topX;
        y += topY;
        pPoint->x += x;
        pPoint->y += y;
    }
    void ScreenToClient(CPoint* pPoint) const
    {
        gint x;
        gint y;
        gdk_window_get_position(_m_hWnd->window, &x, &y);
        gint topX;
        gint topY;
        gdk_window_get_position(gtk_widget_get_toplevel(_m_hWnd)->window, &topX, &topY);
        x += topX;
        y += topY;
        pPoint->x -= x;
        pPoint->y -= y;
    }

    void Invalidate(bool bErase) { gtk_widget_queue_draw(_m_hWnd); }
    void InvalidateRect(CRect* pRect, int bErase)
    {
        gtk_widget_queue_draw_area(_m_hWnd, pRect->left, pRect->top, pRect->Width(), pRect->Height());
    }
    void InvalidateRect(CRect& rect, int bErase) { InvalidateRect(&rect, bErase); }

    bool GetCursorPos(CPoint* pPoint)
    {
        gint x;
        gint y;
        gdk_window_get_pointer(_m_hWnd->window, &x, &y, NULL);
        pPoint->x = x;
        pPoint->y = y;
        ClientToScreen(pPoint);
        return true;
    }

    CWnd* GetCapture() { return _m_bHasCapture ? this : NULL; }
    void SetCapture() { _m_bHasCapture = true; }
    void ReleaseCapture()
    {
        _m_bHasCapture = false;
        OnCaptureChanged(NULL);
    }

    bool button1Down()
    {
        GdkModifierType mask;
        bool bDown = false;
        gdk_window_get_pointer(_m_hWnd->window, NULL, NULL, &mask);
        if (mask & GDK_BUTTON1_MASK)
            bDown = true;
        return bDown;
    }
    bool button2Down()
    {
        GdkModifierType mask;
        bool bDown = false;
        gdk_window_get_pointer(_m_hWnd->window, NULL, NULL, &mask);
        if (mask & GDK_BUTTON2_MASK)
            bDown = true;
        return bDown;
    }

    // A 15-bit (5:5:5) editor colour as a GdkColor.
    void TColorToGdkColor(TColor color, GdkColor* pGdkColor)
    {
        pGdkColor->pixel = 0;
        pGdkColor->red = (gushort)(((color & 0x7c00) >> 10) / 31.0 * 65535.0);
        pGdkColor->green = (gushort)(((color & 0x3e0) >> 5) / 31.0 * 65535.0);
        pGdkColor->blue = (gushort)((color & 0x1f) / 31.0 * 65535.0);
    }

    // Scales srcImage's srcWidth x srcHeight pixels to dstWidth x dstHeight
    // into dstImage (a new image when NULL) by error accumulation.
    GdkImage* StretchBlit(GdkImage* srcImage, GdkImage* dstImage, int srcX, int srcY,
                          int srcWidth, int srcHeight, int dstX, int dstY,
                          int dstWidth, int dstHeight)
    {
        int x;
        int y;
        int xErrorStart = srcWidth - dstWidth;
        int xError;
        int yError = srcHeight - dstHeight;
        guchar* pSrcRow;
        guchar* pDstRow;
        guchar* pSrc;
        guchar* pDst;
        int bytesPerPixel;
        int dstPitch;
        int srcPitch;

        if (!dstImage)
            dstImage = gdk_image_new(GDK_IMAGE_FASTEST, gdk_window_get_visual(_m_hWnd->window),
                                     dstWidth, dstHeight);
        pSrcRow = pSrc = (guchar*)srcImage->mem;
        pDstRow = pDst = (guchar*)dstImage->mem;
        bytesPerPixel = srcImage->bpp / 8;
        dstPitch = dstImage->bpl;
        srcPitch = srcImage->bpl;
        for (y = 0; y < dstHeight; y++) {
            xError = xErrorStart;
            pSrc = pSrcRow;
            pDst = pDstRow;
            for (x = 0; x < dstWidth; x++) {
                for (int i = 0; i < bytesPerPixel; i++)
                    *pDst++ = pSrc[i];
                while (xError >= 0) {
                    pSrc += bytesPerPixel;
                    xError -= dstWidth;
                }
                xError += srcWidth;
            }
            while (yError >= 0) {
                pSrcRow += srcPitch;
                yError -= dstHeight;
            }
            yError += srcHeight;
            pDstRow += dstPitch;
        }
        return dstImage;
    }

    GtkWidget* _m_hWnd;
    bool _m_bHasCapture;
};

#endif  /* HOMM3_EDITOR_STDAFX_H */
