// GarrisonPropsDlg.h - the garrison's properties dialog
// (GarrisonPropsDlg.cpp; GOG only, Loki's port has no counterpart): the
// owner radios of TFlaggablePropsDlg, the army dialog in place of a frame
// control, and whether the units are removable (Armageddon's Blade maps
// and later). Layout from the image: the removable check at 0x5c, the DDX
// owner index (the owner plus one) at 0x98, the garrison at 0x9c, the map
// version at 0xa0, the army dialog at 0xa4, the modified flag at 0xa8 and
// the removable flag at 0xa9.
#ifndef HOMM3_EDITOR_GARRISONPROPSDLG_H
#define HOMM3_EDITOR_GARRISONPROPSDLG_H

#include "gameversion.h"
#include "editor/resource.h"

class TArmyDlg;
class TGarrison;

class TGarrisonPropsDlg : public CDialog {
public:
    TGarrisonPropsDlg(CWnd* pParent, TGarrison* pGarrison, EGameVersion version);
    virtual ~TGarrisonPropsDlg();

    virtual int DoModal();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_GARRISON_PROPS };
    CButton _m_removableCheck;
    int _m_owner;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

private:
    TGarrison* _m_pGarrison;
    EGameVersion _m_version;
    TArmyDlg* _m_pArmyDlg;
    bool _m_bModified;
    bool _m_bRemovableUnits;
};

#endif  /* HOMM3_EDITOR_GARRISONPROPSDLG_H */
