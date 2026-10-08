// RoadToolkit.cpp - Loki h3maped object 64: the road toolkit pane.
// The throw line comes from the retail immediate.
#include "editor/stdafx.h"

#include <string>

#include "exceptions.h"
#include "editor/RoadToolkit.h"

TRoadToolkit::TRoadToolkit(GtkWidget* thisWidget)
    : TToolkitBase(193, thisWidget)
{
    if (!(_m_pToolBar = new TToolkitBaseToolBar(thisWidget, 112)))
#line 41
        throw TAllocationFailure(__FILE__, __LINE__);
}

TRoadToolkit::~TRoadToolkit()
{
    delete _m_pToolBar;
}
