// RiverToolkit.cpp - Loki h3maped object 65: the river toolkit pane.
// The throw line comes from the retail immediate.
#include "editor/stdafx.h"

#include <string>

#include "exceptions.h"
#include "editor/RiverToolkit.h"

TRiverToolkit::TRiverToolkit(GtkWidget* thisWidget)
    : TToolkitBase(211, thisWidget)
{
    if (!(_m_pToolBar = new TToolkitBaseToolBar(thisWidget, 37)))
#line 42
        throw TAllocationFailure(__FILE__, __LINE__);
}

TRiverToolkit::~TRiverToolkit()
{
    delete _m_pToolBar;
}
