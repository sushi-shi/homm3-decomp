// TerrainToolkit.h - the terrain toolkit pane (Loki TerrainToolkit.cpp).
// The pane holds two tool bar pointers, cleared by the constructor and
// deleted by _deleteAll (the second first); nothing in the object creates
// them. Their pointee type (a destructor-less class, as the plain
// operator delete shows), the file name and the member names are not proven.
#ifndef HOMM3_EDITOR_TERRAINTOOLKIT_H
#define HOMM3_EDITOR_TERRAINTOOLKIT_H

#include "editor/ToolkitBase.h"

class TTerrainToolkit : public TToolkitBase {
public:
    TTerrainToolkit(GtkWidget* thisWidget);
    virtual ~TTerrainToolkit();

    CSize getMinSize() const { return _m_minSize; }

private:
    void _deleteAll();

    CSize _m_minSize;
    TToolkitBaseToolBar* _m_pTerrainToolBar;
    TToolkitBaseToolBar* _m_pBrushToolBar;
};

#endif  /* HOMM3_EDITOR_TERRAINTOOLKIT_H */
