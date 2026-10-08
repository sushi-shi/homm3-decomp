// SignPropsDlg.h - the sign properties dialog (Loki SignPropsDlg.cpp): the
// sign_props_dlg glade dialog edits the sign's message. Layout from the
// image: the edited text, the sign (_m_pSign, from the constructor's
// assert), the modified flag, the modal result, then the vtable pointer
// (DoModal, OnInitDialog, OnOK, OnCancel). The other member names are not
// proven.
#ifndef HOMM3_EDITOR_SIGNPROPSDLG_H
#define HOMM3_EDITOR_SIGNPROPSDLG_H

#include "editor/stdafx.h"

#include <string>

class TSign;

class TSignPropsDlg {
public:
    TSignPropsDlg(CWnd* pParent, TSign* pSign);

    virtual int DoModal();
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();

    bool wasModified() const { return _m_bModified; }

private:
    string _m_text;
    TSign* _m_pSign;
    bool _m_bModified;
    int _m_result;
};

// The open dialog, for cppbridge.cpp's sign_props_dlg signal handlers.
extern TSignPropsDlg* signModalPtr;

#endif  /* HOMM3_EDITOR_SIGNPROPSDLG_H */
