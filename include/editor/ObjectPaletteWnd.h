// ObjectPaletteWnd.h - the object palette (Loki ObjectPaletteWnd.cpp).
// Methods as the image declares them; return types from __PRETTY_FUNCTION__
// texts or the retail bodies. Data members are not declared yet
// (TToolkitWnd allocates 0x140 bytes). The client's slot order is
// TToolkitWnd's thunks'.
#ifndef HOMM3_EDITOR_OBJECTPALETTEWND_H
#define HOMM3_EDITOR_OBJECTPALETTEWND_H

#include "editor/stdafx.h"
#include "editor/Player.h"
#include "objecttype.h"

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
};

#endif  /* HOMM3_EDITOR_OBJECTPALETTEWND_H */
