// SignPropsDlg.h - the sign properties dialog (SignPropsDlg.cpp; Loki
// h3maped object 89): it edits the sign's message. Layout from the image:
// the message edit at 0x5c, the DDX text at 0x98, the sign (_m_pSign, from
// Loki's constructor assert) at 0x9c and the modified flag at 0xa0.
#ifndef HOMM3_EDITOR_SIGNPROPSDLG_H
#define HOMM3_EDITOR_SIGNPROPSDLG_H

#include "editor/resource.h"

class TSign;

class TSignPropsDlg : public CDialog {
public:
    TSignPropsDlg(CWnd* pParent, TSign* pSign);

    virtual int DoModal();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_SIGN_PROPS };
    CEdit _m_messageEdit;
    CString _m_text;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

private:
    TSign* _m_pSign;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_SIGNPROPSDLG_H */
