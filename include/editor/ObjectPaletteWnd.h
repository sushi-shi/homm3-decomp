// ObjectPaletteWnd.h - the object palette (Loki ObjectPaletteWnd.cpp).
// Methods as the image declares them; return types from __PRETTY_FUNCTION__
// texts or the retail bodies. The client's slot order is TToolkitWnd's
// thunks'.
//
// The data members (0x140 bytes, as TToolkitWnd allocates) follow the
// constructor and their users: the scroll adjustment and client (named
// _m_vAdjust and _m_pClient by asserts), the object-name label, the
// visible tool range and per-tool flags, the back-buffer pixmap, the
// current slot, one _TSlotInfo per slot (the slot's object types, which
// the constructor collects from kObjectTypeTable), the player and the
// hovered tool. Names other than _m_vAdjust and _m_pClient, and the
// meaning of the word at +0xc, are not proven.
#ifndef HOMM3_EDITOR_OBJECTPALETTEWND_H
#define HOMM3_EDITOR_OBJECTPALETTEWND_H

#include "editor/stdafx.h"
#include "editor/Player.h"
#include "objecttype.h"
#include "editor/Array.h"

#include <vector>

// The label member needs GtkLabel: like the editor's objects, the rest of
// <gtk/gtk.h> goes in an anonymous namespace (stdafx.h).
namespace {
#include <gtk/gtk.h>
}

class TObjectPaletteWnd;

class TObjectPaletteWndClient {
public:
    virtual bool onPaletteCanCreateObject(TObjectPaletteWnd* pPaletteWnd,
                                          const TObjectType& objType) = 0;
    virtual void onPaletteGrabObject(TObjectPaletteWnd* pPaletteWnd,
                                     const TObjectType& objType) = 0;
};

class TObjectPaletteWnd : public CWnd {
public:
    TObjectPaletteWnd(GtkWidget* thisWidget, TObjectPaletteWndClient* pClient,
                      GtkAdjustment* pVAdjustment);
    virtual ~TObjectPaletteWnd();

    void setSlot(TObjectSlot newSlot);
    void setPlayer(TPlayer newPlayer);
    CSize getMinSize() const;

    void OnSize(unsigned int type, int cx, int cy);
    void OnPaint();
    void OnPaint(const CRect& rect);
    void OnVScroll(unsigned int code);
    void OnLButtonDown(unsigned int flags, CPoint point);
    void OnMouseLeave();
    BOOL OnToolTipNeedText(unsigned int id);
    void OnMouseMove(unsigned int flags, CPoint point);

private:
    void _setupTools(int firstRow);
    void _removeAllTools();
    unsigned int _computeRows() const;

    struct _TSlotInfo {
        int m_firstRow;
        vector<const TObjectType*> m_objTypes;
    };

    int _m_scrollPos;
    GtkAdjustment* _m_vAdjust;
    TObjectPaletteWndClient* _m_pClient;
    GtkLabel* _m_pObjNameLabel;
    unsigned int _m_firstTool;
    unsigned int _m_numTools;
    vector<bool> _m_abToolEnabled;
    GdkPixmap* _m_pBackBuffer;
    TObjectSlot _m_slot;
    TArray<_TSlotInfo, kNumObjectSlots> _m_aSlotInfo;
    TPlayer _m_player;
    int _m_hotTool;
};

#endif  /* HOMM3_EDITOR_OBJECTPALETTEWND_H */
