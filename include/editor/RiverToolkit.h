// RiverToolkit.h - the river toolkit pane (Loki RiverToolkit.cpp).
// The file name and the member names are not proven.
#ifndef HOMM3_EDITOR_RIVERTOOLKIT_H
#define HOMM3_EDITOR_RIVERTOOLKIT_H

#include "editor/ToolkitBase.h"

class TRiverToolkit : public TToolkitBase {
public:
    TRiverToolkit(GtkWidget* thisWidget);
    virtual ~TRiverToolkit();

    CSize getMinSize() const { return _m_minSize; }

private:
    CSize _m_minSize;
    TToolkitBaseToolBar* _m_pToolBar;
};

#endif  /* HOMM3_EDITOR_RIVERTOOLKIT_H */
