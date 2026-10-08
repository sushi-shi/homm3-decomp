// EditRumorDlg.h - the rumor editor of the map specifications' rumors page
// (EditRumorDlg.cpp; Loki h3maped object 76). The dialog edits a copy of
// one TRumor through two DDX strings (_m_rumorName and _m_rumorText, Loki's
// assert spellings); OK stays disabled while either is blank. Layout from
// the image: the text edit and OK button controls at 0x5c and 0x98, the
// strings at 0xd4 and 0xd8, the rumor at 0xdc.
#ifndef HOMM3_EDITOR_EDITRUMORDLG_H
#define HOMM3_EDITOR_EDITRUMORDLG_H

#include "editor/GameMap.h"
#include "editor/resource.h"

class TEditRumorDlg : public CDialog {
public:
    TEditRumorDlg(CWnd* pParent, const TRumor& rumor);

    const TRumor& getRumor() const { return _m_rumor; }

    enum { IDD = IDD_EDIT_RUMOR };
    CEdit _m_rumorTextEdit;
    CButton _m_okButton;
    CString _m_rumorName;
    CString _m_rumorText;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnChangeRumorNameEdit();
    afx_msg void OnChangeRumorTextEdit();
    DECLARE_MESSAGE_MAP()

private:
    void _checkStrings();

    TRumor _m_rumor;
};

#endif  /* HOMM3_EDITOR_EDITRUMORDLG_H */
