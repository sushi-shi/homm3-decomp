// ToolkitWnd.cpp - Loki h3maped object 61: the toolkit window, which owns
// the terrain, river, road and erase panes and the object palette, created
// over their glade widgets. Assert and throw lines come from the retail
// immediates.
#include "editor/stdafx.h"

#include <string>

namespace {
#include <gtk/gtk.h>
}

#include "exceptions.h"
#include "editor/EraseToolkit.h"
#include "editor/ObjectPaletteWnd.h"
#include "editor/RiverToolkit.h"
#include "editor/RoadToolkit.h"
#include "editor/TerrainToolkit.h"
#include "editor/ToolkitWnd.h"

TToolkitWnd::TToolkitWnd(GtkWidget* thisWidget, TToolkitWndClient* pClient)
    : _m_pClient(pClient),
      _m_pTerrainToolkit(NULL),
      _m_pRiverToolkit(NULL),
      _m_pRoadToolkit(NULL),
      _m_pEraseToolkit(NULL),
      _m_pObjectPaletteWnd(NULL)
{
#line 39
    assert(pClient != NULL);
    _m_hWnd = NULL;
    try {
        GtkWidget* pWidget = _widget("terraintoolkit");
        if (!(_m_pTerrainToolkit = new TTerrainToolkit(pWidget)))
#line 64
            throw TAllocationFailure(__FILE__, __LINE__);
        pWidget = _widget("rivertoolkit");
        if (!(_m_pRiverToolkit = new TRiverToolkit(pWidget)))
#line 71
            throw TAllocationFailure(__FILE__, __LINE__);
        pWidget = _widget("roadtoolkit");
        if (!(_m_pRoadToolkit = new TRoadToolkit(pWidget)))
#line 76
            throw TAllocationFailure(__FILE__, __LINE__);
        pWidget = _widget("erasertoolkit");
        if (!(_m_pEraseToolkit = new TEraseToolkit(pWidget)))
#line 81
            throw TAllocationFailure(__FILE__, __LINE__);
        pWidget = _widget("objpalettewnd");
        GtkWidget* pVScroll = _widget("objpalettevscroll");
        GtkAdjustment* pVAdjust = gtk_range_get_adjustment(GTK_RANGE(pVScroll));
        if (!(_m_pObjectPaletteWnd = new TObjectPaletteWnd(pWidget, this, pVAdjust)))
#line 88
            throw TAllocationFailure(__FILE__, __LINE__);
    } catch (...) {
        _deleteAll();
        throw;
    }
}

TToolkitWnd::~TToolkitWnd()
{
    _deleteAll();
}

void TToolkitWnd::showTerrainToolkit()
{
}

void TToolkitWnd::showRiverToolkit()
{
}

void TToolkitWnd::showRoadToolkit()
{
}

void TToolkitWnd::showEraseToolkit()
{
}

void TToolkitWnd::showObjectPalette(TObjectSlot slot)
{
#line 152
    assert(_m_pObjectPaletteWnd != NULL);
    _m_pObjectPaletteWnd->setSlot(slot);
}

void TToolkitWnd::setPlayer(TPlayer player)
{
#line 160
    assert(_m_pObjectPaletteWnd != NULL);
    _m_pObjectPaletteWnd->setPlayer(player);
}

bool TToolkitWnd::onPaletteCanCreateObject(TObjectPaletteWnd* pPaletteWnd,
                                           const TObjectType& objType)
{
#line 167
    assert(pPaletteWnd == _m_pObjectPaletteWnd);
    assert(_m_pClient != NULL);
    return _m_pClient->onToolkitCanCreateObject(this, objType);
}

void TToolkitWnd::onPaletteGrabObject(TObjectPaletteWnd* pPaletteWnd,
                                      const TObjectType& objType)
{
#line 175
    assert(pPaletteWnd == _m_pObjectPaletteWnd);
    assert(_m_pClient != NULL);
    _m_pClient->onToolkitGrabObject(this, objType);
}

void TToolkitWnd::_deleteAll()
{
    delete _m_pObjectPaletteWnd;
    delete _m_pEraseToolkit;
    delete _m_pRoadToolkit;
    delete _m_pRiverToolkit;
    delete _m_pTerrainToolkit;
}
