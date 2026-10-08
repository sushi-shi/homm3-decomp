// SelectArtifactDlg.h - the artifact picker (SelectArtifactDlg.cpp; GOG
// only): a list of the map version's artifacts, the special and disabled
// ones left out. Layout from the image: the list at 0x5c, the OK button at
// 0x98, the map version at 0xd4 and the chosen artifact (-1 for none) at
// 0xd8. The object includes no terrain masks: its only initializer is the
// shared guard.
#ifndef HOMM3_EDITOR_SELECTARTIFACTDLG_H
#define HOMM3_EDITOR_SELECTARTIFACTDLG_H

#include "gameversion.h"
#include "editor/resource.h"

class TSelectArtifactDlg : public CDialog {
public:
    TSelectArtifactDlg(CWnd* pParent, EGameVersion mapVersion, int artifact);

    int getArtifact() const { return _m_artifact; }

    enum { IDD = IDD_SELECT_ARTIFACT };
    CListBox _m_artifactList;
    CButton _m_okButton;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSelChangeArtifactList();
    afx_msg void OnSelCancelArtifactList();
    afx_msg void OnDblClkArtifactList();
    DECLARE_MESSAGE_MAP()

private:
    EGameVersion _m_mapVersion;
    int _m_artifact;
};

#endif  /* HOMM3_EDITOR_SELECTARTIFACTDLG_H */
