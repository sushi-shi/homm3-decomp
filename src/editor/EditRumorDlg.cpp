// EditRumorDlg.cpp - Loki h3maped object 76: the rumor editor. DoModal runs
// the edit_rumor_dlg glade dialog until OnOK or OnCancel sets the result;
// the OK button is sensitive only while neither entry is blank. The assert
// lines come from the retail immediates.
#include "editor/stdafx.h"

#include <string.h>
#include <string>

#include "editor/cppbridge.h"
#include "editor/EditRumorDlg.h"

TEditRumorDlg* rumorDlgModal = NULL;

TEditRumorDlg::TEditRumorDlg(void* pParent, const TRumor& rumor)
    : _m_rumorNameEdit(GTK_EDITABLE(_widget("edit_rumor_name"))),
      _m_rumorTextEdit(GTK_EDITABLE(_widget("edit_rumor_text"))),
      _m_okButton(GTK_BUTTON(_widget("edit_rumor_ok"))),
      _m_rumor(rumor)
{
    _m_rumorName = "";
    _m_rumorText = "";
    OnInitDialog();
}

int TEditRumorDlg::DoModal()
{
    _m_result = 0;
    rumorDlgModal = this;
    GtkWidget* pDialog = _widget("edit_rumor_dlg");
    gtk_widget_show(pDialog);
    while (_m_result == 0)
        gtk_main_iteration();
    gtk_widget_hide(pDialog);
    rumorDlgModal = NULL;
    return _m_result;
}

void TEditRumorDlg::OnOK()
{
#line 120
    assert(!_isspace( _m_rumorName ));
    assert(!_isspace( _m_rumorText ));
    _m_rumor.setNameAndText(string(_m_rumorName.c_str()), string(_m_rumorText.c_str()));
    _m_result = 1;
}

void TEditRumorDlg::OnCancel()
{
    _m_result = 2;
}

void TEditRumorDlg::_checkStrings()
{
    UpdateData(false);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_okButton), !_isspace(_m_rumorName) && !_isspace(_m_rumorText));
}

void TEditRumorDlg::UpdateData(bool bSaveAndValidate)
{
    gchar* text = gtk_editable_get_chars(_m_rumorNameEdit, 0, -1);
    _m_rumorName = text;
    g_free(text);
    text = gtk_editable_get_chars(_m_rumorTextEdit, 0, -1);
    _m_rumorText = text;
    g_free(text);
}

BOOL TEditRumorDlg::OnInitDialog()
{
    _m_rumorName = _m_rumor.getName().c_str();
    _m_rumorText = _m_rumor.getText().c_str();

    gint position = 0;
    gtk_editable_set_position(_m_rumorNameEdit, 0);
    gtk_editable_delete_text(_m_rumorNameEdit, 0, -1);
    gtk_editable_insert_text(_m_rumorNameEdit, _m_rumorName.c_str(), strlen(_m_rumorName.c_str()), &position);
    position = 0;
    gtk_editable_set_position(_m_rumorTextEdit, 0);
    gtk_editable_delete_text(_m_rumorTextEdit, 0, -1);
    gtk_editable_insert_text(_m_rumorTextEdit, _m_rumorText.c_str(), strlen(_m_rumorText.c_str()), &position);
    gtk_entry_set_max_length(GTK_ENTRY(_m_rumorTextEdit), TRumor::s_kMaxTextLen);

    if (_isspace(_m_rumorName) || _isspace(_m_rumorText))
        gtk_widget_set_sensitive(GTK_WIDGET(_m_okButton), FALSE);
    return true;
}

void TEditRumorDlg::OnChangeRumorNameEdit()
{
    _checkStrings();
}

void TEditRumorDlg::OnChangeRumorTextEdit()
{
    _checkStrings();
}
