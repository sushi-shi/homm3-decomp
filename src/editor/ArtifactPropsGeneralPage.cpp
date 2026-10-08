// ArtifactPropsGeneralPage.cpp - Loki h3maped object 101 (file name
// inferred; no assert names it): the artifact property sheet's general
// page shows the artifact's type name and edits its pickup message.
#include "editor/stdafx.h"

#include <string>

namespace {
#include <gtk/gtk.h>
}

#include "editor/ArtifactPropsGeneralPage.h"
#include "editor/ObjectSpecializations.h"

TArtifactPropsGeneralPage::TArtifactPropsGeneralPage(const TGameArtifact& artifact)
    : _m_messageText(GTK_TEXT(_widget("artifact_props_general_message"))),
      _m_typeLabel(GTK_LABEL(_widget("artifact_props_general_type_label"))),
      _m_artifact(artifact),
      _m_bModified(false)
{
    _m_typeName = "";
    _m_message = "";
    OnInitDialog();
}

TArtifactPropsGeneralPage::~TArtifactPropsGeneralPage()
{
}

BOOL TArtifactPropsGeneralPage::OnInitDialog()
{
    _m_bModified = false;
    _m_typeName = _m_artifact.getTypeName().c_str();
    gtk_label_set_text(_m_typeLabel, _m_typeName.c_str());
    _m_message = _m_artifact.getMessage().c_str();

    GtkText* pText = _m_messageText;
    gtk_text_set_point(pText, 0);
    gint length = gtk_text_get_length(pText);
    if (length > 0)
        gtk_editable_delete_text(GTK_EDITABLE(pText), 0, -1);
    gtk_text_insert(pText, NULL, NULL, NULL, _m_message.c_str(), -1);
    gtk_text_set_point(pText, 0);
    return true;
}

void TArtifactPropsGeneralPage::OnOK()
{
    gchar* text = gtk_editable_get_chars(GTK_EDITABLE(_m_messageText), 0, -1);
    _m_message = text;
    g_free(text);
    _m_bModified = _m_bModified || string(_m_message) != _m_artifact.getMessage();
}
