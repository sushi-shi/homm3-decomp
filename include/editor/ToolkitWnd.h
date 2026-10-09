// ToolkitWnd.h - the toolkit window (ToolkitWnd.cpp; Loki h3maped object
// 61): the terrain, river, road, erase and obstacle toolkits and the object
// palette, of which it shows one at a time over its whole client area. It
// passes the palette's questions on to its own client. Layout from the
// constructor (0x80 bytes): the client, the minimum size (the largest of
// its panes'), the pane shown and the panes, each owned by an auto_ptr.
#ifndef HOMM3_EDITOR_TOOLKITWND_H
#define HOMM3_EDITOR_TOOLKITWND_H

#include "editor/stdafx.h"

#include <memory>

#include "editor/ObjectPaletteWnd.h"

class TToolkitWndClient;
class TTerrainToolkit;
class TRiverToolkit;
class TRoadToolkit;
class TEraseToolkit;
class TObstacleToolkit;

class TToolkitWnd : public CWnd, private TObjectPaletteWndClient {
public:
    TToolkitWnd(CWnd* pParent, TToolkitWndClient* pClient);
    virtual ~TToolkitWnd();

    void showTerrainToolkit();
    void showRiverToolkit();
    void showRoadToolkit();
    void showEraseToolkit();
    void showObstacleToolkit();
    void showObjectPalette(TObjectSlot slot);
    void setPlayer(TPlayer player);

    CSize getMinSize() const { return _m_minSize; }

protected:
    afx_msg void OnSize(UINT nType, int cx, int cy);
    DECLARE_MESSAGE_MAP()

private:
    virtual bool onPaletteCanCreateObject(TObjectPaletteWnd* pPaletteWnd, const TObjectType& objType);
    virtual void onPaletteGrabObject(TObjectPaletteWnd* pPaletteWnd, const TObjectType& objType);

    void _showPane(CWnd* pPane);

    TToolkitWndClient* _m_pClient;
    CSize _m_minSize;
    CWnd* _m_pShownPane;
    auto_ptr<TTerrainToolkit> _m_pTerrainToolkit;
    auto_ptr<TRiverToolkit> _m_pRiverToolkit;
    auto_ptr<TRoadToolkit> _m_pRoadToolkit;
    auto_ptr<TEraseToolkit> _m_pEraseToolkit;
    auto_ptr<TObstacleToolkit> _m_pObstacleToolkit;
    auto_ptr<TObjectPaletteWnd> _m_pObjectPaletteWnd;
};

class TToolkitWndClient {
public:
    virtual bool onToolkitCanCreateObject(TToolkitWnd* pToolkitWnd, const TObjectType& objType) = 0;
    virtual void onToolkitGrabObject(TToolkitWnd* pToolkitWnd, const TObjectType& objType) = 0;
};

#endif  /* HOMM3_EDITOR_TOOLKITWND_H */
