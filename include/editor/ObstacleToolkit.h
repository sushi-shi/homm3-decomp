// ObstacleToolkit.h - the obstacle toolkit (ObstacleToolkit.cpp; GOG only,
// file name inferred from the RTTI name): the obstacle tool bar above the
// brush label and its tool bar. Layout from the constructor (0xb0 bytes):
// the label at +0x5c, the minimum size and the two tool bars, owned by
// auto_ptrs.
#ifndef HOMM3_EDITOR_OBSTACLETOOLKIT_H
#define HOMM3_EDITOR_OBSTACLETOOLKIT_H

#include <memory>

#include "editor/ToolkitBase.h"

class TObstacleToolkit : public TToolkitBase {
public:
    TObstacleToolkit(CWnd* pParent);

    CSize getMinSize() const { return _m_minSize; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()

private:
    CStatic _m_brushStatic;
    CSize _m_minSize;
    std::auto_ptr<TToolkitBaseToolBar> _m_pToolBar;
    std::auto_ptr<TToolkitBaseToolBar> _m_pBrushToolBar;
};

#endif  /* HOMM3_EDITOR_OBSTACLETOOLKIT_H */
