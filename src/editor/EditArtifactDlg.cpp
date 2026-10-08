// EditArtifactDlg.cpp - Loki h3maped object 82: the artifact editor. The
// where combo lists the backpack (unless full) and the unused slots; the
// artifact combo lists every artifact but the spellbook and scrolls that
// fits the chosen slot (all of them for the backpack). OK needs an
// artifact. The assert lines come from the retail immediates.
#include "editor/stdafx.h"

namespace {
#include <gtk/gtk.h>
}

#include "editor/cppbridge.h"
#include "editor/EditArtifactDlg.h"
#include "editor/MapEditorText.h"

TEditArtifactDlg* editArtifactDlg = NULL;

TEditArtifactDlg::TEditArtifactDlg(CWnd* pParent, const TArtifactSlotSet& unusedSlots, bool bBackpackFull)
    : _m_artifact(ARTIFACT_NONE),
      _m_slot(kNumArtifactSlots),
      _m_unusedSlots(unusedSlots),
      _m_bBackpackFull(bBackpackFull)
{
#line 46
    assert(!_m_unusedSlots.empty() || !_m_bBackpackFull);
    GtkCombo* c = GTK_COMBO(_widget("edit_artifact_where_combo"));
    emptyList(GTK_LIST(c->list));
    c = GTK_COMBO(_widget("edit_artifact_artifact_combo"));
    emptyList(GTK_LIST(c->list));
    if (_m_bBackpackFull)
        _m_slot = *_m_unusedSlots.begin();
    OnInitDialog();
}

TEditArtifactDlg::TEditArtifactDlg(CWnd* pParent, const TArtifactSlotSet& unusedSlots, bool bBackpackFull,
                                   TArtifact artifact, TArtifactSlot slot)
    : _m_artifact(artifact),
      _m_slot(slot),
      _m_unusedSlots(unusedSlots),
      _m_bBackpackFull(bBackpackFull)
{
#line 78
    assert(_m_artifact >= 0 && _m_artifact < kNumArtifacts);
    assert(_m_slot >= 0);
    GtkCombo* c = GTK_COMBO(_widget("edit_artifact_where_combo"));
    emptyList(GTK_LIST(c->list));
    c = GTK_COMBO(_widget("edit_artifact_artifact_combo"));
    emptyList(GTK_LIST(c->list));
    if (_m_slot < kNumArtifactSlots) {
#line 90
        assert(_m_unusedSlots.find( _m_slot ) == _m_unusedSlots.end());
        _m_unusedSlots.insert(_m_slot);
    } else
        _m_bBackpackFull = false;
    OnInitDialog();
}

void TEditArtifactDlg::OnOK()
{
    _m_result = 1;
}

void TEditArtifactDlg::OnCancel()
{
    _m_result = 2;
}

int TEditArtifactDlg::DoModal()
{
    _m_result = 0;
    editArtifactDlg = this;
    GtkWidget* pDialog = _widget("edit_artifact_dlg");
    gtk_widget_show(pDialog);
    while (_m_result == 0)
        gtk_main_iteration();
    gtk_widget_hide(pDialog);
    editArtifactDlg = NULL;
    return _m_result;
}

void TEditArtifactDlg::_setArtifacts()
{
    GtkCombo* c = GTK_COMBO(_widget("edit_artifact_artifact_combo"));
    emptyList(GTK_LIST(c->list));
    GList* items = NULL;
    int item = 0;
    unsigned int artifact;
    for (artifact = 0; artifact < kNumArtifacts; artifact++) {
        if (artifact == ARTIFACT_SPELL_SCROLL || artifact == 0)
            continue;
        if (_m_slot >= kNumArtifactSlots || artifactAllowedInSlot(TArtifact(artifact), _m_slot)) {
            items = g_list_append(items, (gpointer)akArtifactTraits[artifact].m_name);
            _m_listArtifacts[item] = artifact;
            item++;
        }
    }
    gtk_combo_set_popdown_strings(c, items);
    if (_m_artifact != ARTIFACT_NONE) {
        int count = g_list_length(GTK_LIST(c->list)->children);
        for (item = 0; item < count; item++)
            if (_m_listArtifacts[item] == _m_artifact)
                break;
        if (item < count)
            setCurrentSelection(GTK_LIST(c->list), item);
        else {
            _m_artifact = ARTIFACT_NONE;
            gtk_widget_set_sensitive(_widget("edit_artifact_ok"), FALSE);
        }
    }
}

BOOL TEditArtifactDlg::OnInitDialog()
{
    GtkCombo* c = GTK_COMBO(_widget("edit_artifact_where_combo"));
    GList* items = NULL;
    unsigned int cbIndex = 0;
    if (!_m_bBackpackFull) {
        items = g_list_append(items, (gpointer)kBackpackStr);
        _m_listSlots[cbIndex] = kNumArtifactSlots;
        cbIndex++;
    }
    for (TArtifactSlotSet::const_iterator slotIter = _m_unusedSlots.begin(); slotIter != _m_unusedSlots.end();
         ++slotIter) {
#line 253
        assert(*slotIter >= 0 && *slotIter < kNumArtifactSlots);
        items = g_list_append(items, (gpointer)akArtifactSlotTraits[*slotIter].m_name);
        _m_listSlots[cbIndex] = *slotIter;
        cbIndex++;
    }
    gtk_combo_set_popdown_strings(c, items);
    for (cbIndex = 0; _m_listSlots[cbIndex] != _m_slot; cbIndex++) {
#line 264
        assert(cbIndex < g_list_length(GTK_LIST(c->list)->children));
    }
    setCurrentSelection(GTK_LIST(c->list), cbIndex);
    _setArtifacts();
    if (_m_artifact == ARTIFACT_NONE)
        _m_artifact = TArtifact(_m_listArtifacts[0]);
    gtk_widget_set_sensitive(_widget("edit_artifact_ok"), _m_artifact != ARTIFACT_NONE);
    return true;
}

void TEditArtifactDlg::OnSelChangeWhereCombo()
{
    GtkCombo* c = GTK_COMBO(_widget("edit_artifact_where_combo"));
    int curSel = getCurrentSelection(GTK_LIST(c->list));
#line 285
    assert(curSel >= 0);
    _m_slot = TArtifactSlot(_m_listSlots[curSel]);
    _setArtifacts();
}

void TEditArtifactDlg::OnSelChangeArtifactCombo()
{
    GtkCombo* c = GTK_COMBO(_widget("edit_artifact_artifact_combo"));
    int curSel = getCurrentSelection(GTK_LIST(c->list));
    if (curSel >= 0) {
        _m_artifact = TArtifact(_m_listArtifacts[curSel]);
        gtk_widget_set_sensitive(_widget("edit_artifact_ok"), TRUE);
    } else if (_m_artifact != ARTIFACT_NONE) {
        _m_artifact = ARTIFACT_NONE;
        gtk_widget_set_sensitive(_widget("edit_artifact_ok"), FALSE);
    }
}
