// RiverToolkit.h - the river toolkit (RiverToolkit.cpp; Loki h3maped
// object 64): the river type tool bar under its label. The Windows toolkit is a
// modeless dialog page (0xa4 bytes): the label at +0x5c, the minimum size
// it lays itself out to and the tool bar.
#ifndef HOMM3_EDITOR_RIVERTOOLKIT_H
#define HOMM3_EDITOR_RIVERTOOLKIT_H

#include "editor/ToolkitBase.h"

class TRiverToolkit : public TToolkitBase {
public:
    TRiverToolkit(CWnd* pParent);
    virtual ~TRiverToolkit();

    CSize getMinSize() const { return _m_minSize; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()

private:
    CStatic _m_riverTypeStatic;
    CSize _m_minSize;
    TToolkitBaseToolBar* _m_pToolBar;
};

#endif  /* HOMM3_EDITOR_RIVERTOOLKIT_H */
