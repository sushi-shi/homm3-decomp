// ResourceQuantitiesDlg.h - the resource quantities dialog
// (Loki ResourceQuantitiesDlg.cpp): seven spin buttons and seven "give"
// toggles of the timed event glade tree edit a copy of the caller's
// quantities. The layout is the image's (the original's address, the
// modified flag, the copy, then the vtable pointer). The file name and the
// member names are not proven.
#ifndef HOMM3_EDITOR_RESOURCEQUANTITIESDLG_H
#define HOMM3_EDITOR_RESOURCEQUANTITIESDLG_H

#include "editor/stdafx.h"
#include "editor/ResourceQuantities.h"

class TResourceQuantitiesDlg {
public:
    TResourceQuantitiesDlg(const TResourceQuantities& resourceQuantities);

    virtual void OnOK();
    virtual BOOL OnInitDialog();

    const TResourceQuantities& getResourceQuantities() const { return _m_resourceQuantities; }
    bool wasModified() const { return _m_bModified; }

private:
    const TResourceQuantities& _m_originalQuantities;
    bool _m_bModified;
    TResourceQuantities _m_resourceQuantities;
};

#endif  /* HOMM3_EDITOR_RESOURCEQUANTITIESDLG_H */
