// ToolkitWnd.h - the toolkit window (Loki ToolkitWnd.cpp): the terrain,
// river, road and erase panes and the object palette, created from their
// glade widgets. Methods as the image declares them; the client's slot
// order is TMapView's thunks'. The layout follows the constructor (0x34
// bytes, as TMapView allocates); the word at +0x1c is never touched and
// its name and type are not recovered.
#ifndef HOMM3_EDITOR_TOOLKITWND_H
#define HOMM3_EDITOR_TOOLKITWND_H

#include "editor/stdafx.h"
#include "editor/ObjectPaletteWnd.h"

class TToolkitWnd;
class TToolkitWndClient;
class TTerrainToolkit;
class TRiverToolkit;
class TRoadToolkit;
class TEraseToolkit;

class TToolkitWnd : public CWnd, public TObjectPaletteWndClient {
public:
    TToolkitWnd(GtkWidget* thisWidget, TToolkitWndClient* pClient);
    virtual ~TToolkitWnd();

    void showTerrainToolkit();
    void showRiverToolkit();
    void showRoadToolkit();
    void showEraseToolkit();
    void showObjectPalette(TObjectSlot slot);
    void setPlayer(TPlayer player);

    bool onPaletteCanCreateObject(TObjectPaletteWnd* pPaletteWnd, const TObjectType& objType);
    void onPaletteGrabObject(TObjectPaletteWnd* pPaletteWnd, const TObjectType& objType);

    CSize getMinSize() const { return _m_minSize; }
    TObjectPaletteWnd* getObjPal() { return _m_pObjectPaletteWnd; }

private:
    void _deleteAll();

    TToolkitWndClient* _m_pClient;
    CSize _m_minSize;
    int _m_unknown;
    TTerrainToolkit* _m_pTerrainToolkit;
    TRiverToolkit* _m_pRiverToolkit;
    TRoadToolkit* _m_pRoadToolkit;
    TEraseToolkit* _m_pEraseToolkit;
    TObjectPaletteWnd* _m_pObjectPaletteWnd;
};

class TToolkitWndClient {
public:
    virtual bool onToolkitCanCreateObject(TToolkitWnd* pToolkitWnd, const TObjectType& objType) = 0;
    virtual void onToolkitGrabObject(TToolkitWnd* pToolkitWnd, const TObjectType& objType) = 0;
};

#endif  /* HOMM3_EDITOR_TOOLKITWND_H */
