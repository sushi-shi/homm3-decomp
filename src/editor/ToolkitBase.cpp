// ToolkitBase.cpp - Loki h3maped object 62: the toolkit and tool bar bases.
// Assert lines come from the retail immediates.
#include "editor/stdafx.h"

#include <string>

#include "editor/ToolkitBase.h"

TToolkitBase::TToolkitBase(unsigned int id, GtkWidget* thisWidget)
{
#line 35
    assert(thisWidget != NULL);
    _m_hWnd = thisWidget;
}

TToolkitBaseToolBar::TToolkitBaseToolBar(GtkWidget* thisWidget, unsigned int firstID,
                                         unsigned int numTools)
{
#line 151
    assert(thisWidget != NULL);
    _m_hWnd = thisWidget;
}
