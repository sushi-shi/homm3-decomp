// EditRumorDlg.h - the rumor editor of the map specifications' rumors page
// (Loki EditRumorDlg.cpp): the edit_rumor_dlg glade dialog edits a copy of
// one TRumor. Layout from the image: the name and text entries and the OK
// button, the entries' strings (_m_rumorName, _m_rumorText, from OnOK's
// asserts), the rumor, the modal result, then the vtable pointer
// (OnInitDialog, OnOK, OnCancel, DoModal). The widget member names are not
// proven.
#ifndef HOMM3_EDITOR_EDITRUMORDLG_H
#define HOMM3_EDITOR_EDITRUMORDLG_H

#include "editor/stdafx.h"

#include <string>

#include "editor/GameMap.h"

class TEditRumorDlg {
public:
    TEditRumorDlg(void* pParent, const TRumor& rumor);

    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    virtual int DoModal();

    void UpdateData(bool bSaveAndValidate);
    void OnChangeRumorNameEdit();
    void OnChangeRumorTextEdit();

    const TRumor& getRumor() const { return _m_rumor; }

private:
    void _checkStrings();

    GtkEditable* _m_rumorNameEdit;
    GtkEditable* _m_rumorTextEdit;
    GtkButton* _m_okButton;
    string _m_rumorName;
    string _m_rumorText;
    TRumor _m_rumor;
    int _m_result;
};

// The open dialog, for cppbridge.cpp's edit_rumor_dlg signal handlers.
extern TEditRumorDlg* rumorDlgModal;

#endif  /* HOMM3_EDITOR_EDITRUMORDLG_H */
