// EraseToolkit.h - the erase toolkit pane (Loki EraseToolkit.cpp).
// The file name and the member names are not proven.
#ifndef HOMM3_EDITOR_ERASETOOLKIT_H
#define HOMM3_EDITOR_ERASETOOLKIT_H

#include "editor/ToolkitBase.h"

class TEraseToolkit : public TToolkitBase {
public:
    TEraseToolkit(GtkWidget* thisWidget);
    virtual ~TEraseToolkit();

    CSize getMinSize() const { return _m_minSize; }

private:
    CSize _m_minSize;
    TToolkitBaseToolBar* _m_pToolBar;
};

#endif  /* HOMM3_EDITOR_ERASETOOLKIT_H */
