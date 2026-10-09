// TerrainToolkit.h - the terrain toolkit (TerrainToolkit.cpp; Loki h3maped
// object 66): the brush tool bar under its label and the terrain type tool
// bar under its own. Layout from the constructor (0xe4 bytes): the terrain
// type label at +0x5c, the brush label at +0x98, the minimum size and the
// two tool bars.
#ifndef HOMM3_EDITOR_TERRAINTOOLKIT_H
#define HOMM3_EDITOR_TERRAINTOOLKIT_H

#include "editor/ToolkitBase.h"

class TTerrainToolkit : public TToolkitBase {
public:
    TTerrainToolkit(CWnd* pParent);
    virtual ~TTerrainToolkit();

    CSize getMinSize() const { return _m_minSize; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    DECLARE_MESSAGE_MAP()

private:
    void _deleteAll();

    CStatic _m_terrainTypeStatic;
    CStatic _m_brushStatic;
    CSize _m_minSize;
    TToolkitBaseToolBar* _m_pBrushToolBar;
    TToolkitBaseToolBar* _m_pTerrainToolBar;
};

#endif  /* HOMM3_EDITOR_TERRAINTOOLKIT_H */
