// HolyGrailPropsDlg.h - the grail's placement radius dialog
// (HolyGrailPropsDlg.cpp; GOG only, Loki's port has no counterpart): an
// edit and its spin for the radius, 0..THolyGrail::s_kMaxRadius. Layout
// from the image: the spin at 0x5c, the edit at 0x98, the grail at 0xd4,
// the radius at 0xd8 and the modified flag at 0xdc.
#ifndef HOMM3_EDITOR_HOLYGRAILPROPSDLG_H
#define HOMM3_EDITOR_HOLYGRAILPROPSDLG_H

#include "editor/resource.h"

class THolyGrail;

class THolyGrailPropsDlg : public CDialog {
public:
    THolyGrailPropsDlg(CWnd* pParent, THolyGrail* pHolyGrail);

    virtual int DoModal();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_HOLY_GRAIL_PROPS };
    CSpinButtonCtrl _m_radiusSpin;
    CEdit _m_radiusEdit;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnKillFocusRadiusEdit();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

private:
    THolyGrail* _m_pHolyGrail;
    unsigned int _m_radius;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_HOLYGRAILPROPSDLG_H */
