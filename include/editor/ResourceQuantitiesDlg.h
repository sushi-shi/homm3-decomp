// ResourceQuantitiesDlg.h - the resource quantities dialog
// (ResourceQuantitiesDlg.cpp; Loki h3maped object 26): an edit, a spin
// button and a give/take radio pair per resource edit a copy of the
// caller's quantities. The timed event and town event pages host it as a
// child. Layout from the image: the original's address at 0x5c, the
// modified flag, the seven control sets (0x7c bytes each) at 0x64 and the
// copy at 0x3c8 (0x3e4 bytes, the pages' new).
#ifndef HOMM3_EDITOR_RESOURCEQUANTITIESDLG_H
#define HOMM3_EDITOR_RESOURCEQUANTITIESDLG_H

#include "editor/resource.h"
#include "editor/ResourceQuantities.h"

class TResourceQuantitiesDlg : public CDialog {
public:
    TResourceQuantitiesDlg(const TResourceQuantities& resourceQuantities);

    const TResourceQuantities& getResourceQuantities() const { return _m_resourceQuantities; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_RESOURCE_QUANTITIES };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()

private:
    // One resource's controls; the radio index is 1 when it is taken.
    struct _TControls {
        CEdit m_edit;
        CSpinButtonCtrl m_spin;
        int m_take;
    };

    const TResourceQuantities& _m_originalQuantities;
    bool _m_bModified;
    _TControls _m_aControls[kNumGameResourceTypes];
    TResourceQuantities _m_resourceQuantities;
};

#endif  /* HOMM3_EDITOR_RESOURCEQUANTITIESDLG_H */
