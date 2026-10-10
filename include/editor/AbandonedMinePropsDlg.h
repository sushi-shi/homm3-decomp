// AbandonedMinePropsDlg.h - the abandoned mine's resources dialog
// (AbandonedMinePropsDlg.cpp; GOG only, Loki's port has no counterpart):
// one check per resource the mine may yield, wood excepted, at least one
// of them checked. Layout from the image: the mine at 0x5c, the modified
// flag at 0x60, the number of checked resources at 0x64, the potential
// resources at 0x68 and one check per resource type at 0x6c (wood's has no
// control).
#ifndef HOMM3_EDITOR_ABANDONEDMINEPROPSDLG_H
#define HOMM3_EDITOR_ABANDONEDMINEPROPSDLG_H

#include <bitset>

#include "editor/GameResource.h"
#include "editor/resource.h"

class TAbandonedMine;

class TAbandonedMinePropsDlg : public CDialog {
public:
    TAbandonedMinePropsDlg(CWnd* pParent, TAbandonedMine* pMine);

    virtual int DoModal();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_ABANDONED_MINE_PROPS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnRes2Check();
    afx_msg void OnRes3Check();
    afx_msg void OnRes4Check();
    afx_msg void OnRes5Check();
    afx_msg void OnRes6Check();
    afx_msg void OnRes7Check();
    DECLARE_MESSAGE_MAP()

private:
    void _onResourceCheck(unsigned int type);

    TAbandonedMine* _m_pMine;
    bool _m_bModified;
    unsigned int _m_numPotentialResources;
    std::bitset<kNumGameResourceTypes> _m_abPotentialResource;
    CButton _m_aResourceCheck[kNumGameResourceTypes];
};

#endif  /* HOMM3_EDITOR_ABANDONEDMINEPROPSDLG_H */
