// TerrainToolkit.cpp - Loki h3maped object 66: the terrain toolkit pane.
#include "editor/stdafx.h"

#include <string>

#include "editor/TerrainToolkit.h"

TTerrainToolkit::TTerrainToolkit(GtkWidget* thisWidget)
    : TToolkitBase(234, thisWidget),
      _m_pTerrainToolBar(NULL),
      _m_pBrushToolBar(NULL)
{
}

TTerrainToolkit::~TTerrainToolkit()
{
    _deleteAll();
}

void TTerrainToolkit::_deleteAll()
{
    delete _m_pBrushToolBar;
    delete _m_pTerrainToolBar;
}
