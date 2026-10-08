// SignPropsDlg.cpp - Loki h3maped object 89: the sign properties dialog.
// DoModal runs the sign_props_dlg glade dialog until OnOK (1) or OnCancel
// (2) and stores the edited message in the sign on OK. The assert line
// comes from the retail immediate.
#include "editor/stdafx.h"

#include <string.h>
#include <string>

namespace {
#include <gtk/gtk.h>
}

#include "editor/SignPropsDlg.h"
#include "editor/ObjectSpecializations.h"

TSignPropsDlg* signModalPtr = NULL;

TSignPropsDlg::TSignPropsDlg(CWnd* pParent, TSign* pSign)
    : _m_pSign(pSign),
      _m_bModified(false)
{
#line 43
    assert(_m_pSign != NULL);
}

int TSignPropsDlg::DoModal()
{
    _m_result = 0;
    signModalPtr = this;
    GtkWidget* pDialog = _widget("sign_props_dlg");
    gtk_widget_show(pDialog);
    while (_m_result == 0)
        gtk_main_iteration();
    gtk_widget_hide(pDialog);
    signModalPtr = NULL;
    int result = _m_result;
    if (result == 1)
        _m_pSign->setText(string(_m_text.c_str()));
    return result;
}

BOOL TSignPropsDlg::OnInitDialog()
{
    _m_bModified = false;
    _m_text = _m_pSign->getText();
    GtkText* pText = GTK_TEXT(_widget("sign_props_message"));
    int length = gtk_text_get_length(pText);
    if (length > 0)
        gtk_editable_delete_text(GTK_EDITABLE(pText), 0, length);
    gint position = 0;
    gchar* text = (gchar*)_m_text.c_str();
    gtk_editable_insert_text(GTK_EDITABLE(pText), text, strlen(text), &position);
    return true;
}

void TSignPropsDlg::OnOK()
{
    GtkEditable* pEditable = GTK_EDITABLE(_widget("sign_props_message"));
    gchar* text = gtk_editable_get_chars(pEditable, 0, -1);
    _m_text = text;
    g_free(text);
    _m_result = 1;
    _m_bModified = _m_bModified || _m_text != _m_pSign->getText();
}

void TSignPropsDlg::OnCancel()
{
    _m_result = 2;
}
