// EraseToolkit.cpp - Loki h3maped object 63: the erase toolkit pane.
// The throw line comes from the retail immediate.
#include "editor/stdafx.h"

#include <string>

#include "exceptions.h"
#include "editor/EraseToolkit.h"

TEraseToolkit::TEraseToolkit(GtkWidget* thisWidget)
    : TToolkitBase(41, thisWidget)
{
    if (!(_m_pToolBar = new TToolkitBaseToolBar(thisWidget, 111)))
#line 41
        throw TAllocationFailure(__FILE__, __LINE__);
}

TEraseToolkit::~TEraseToolkit()
{
    delete _m_pToolBar;
}
