// RoadToolkit.h - the road toolkit pane (Loki RoadToolkit.cpp).
// The file name and the member names are not proven.
#ifndef HOMM3_EDITOR_ROADTOOLKIT_H
#define HOMM3_EDITOR_ROADTOOLKIT_H

#include "editor/ToolkitBase.h"

class TRoadToolkit : public TToolkitBase {
public:
    TRoadToolkit(GtkWidget* thisWidget);
    virtual ~TRoadToolkit();

    CSize getMinSize() const { return _m_minSize; }

private:
    CSize _m_minSize;
    TToolkitBaseToolBar* _m_pToolBar;
};

#endif  /* HOMM3_EDITOR_ROADTOOLKIT_H */
