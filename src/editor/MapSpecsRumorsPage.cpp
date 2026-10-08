// MapSpecsRumorsPage.cpp - Loki h3maped object 73: the rumors page of the
// map specifications sheet. The rumors_list glade list mirrors _m_rumors
// by name; a map holds at most 30 rumors. The assert lines come from the
// retail immediates.
#include "editor/stdafx.h"

#include <vector>

#include "editor/cppbridge.h"
#include "editor/EditRumorDlg.h"
#include "editor/MapSpecsRumorsPage.h"

TMapSpecsRumorsPage::TMapSpecsRumorsPage(const TGameMap& map)
    : _m_map(map)
{
    _m_bModified = false;
    _m_bEdited = false;
}

TMapSpecsRumorsPage::~TMapSpecsRumorsPage()
{
}

void TMapSpecsRumorsPage::OnOK()
{
    _m_newRumors = _m_rumors;
    _m_bModified = _m_bModified || _m_newRumors != _m_map.getRumors();
}

BOOL TMapSpecsRumorsPage::OnInitDialog()
{
    _m_bModified = false;
    _m_newRumors = _m_map.getRumors();
    _m_rumors = _m_newRumors;

    GtkList* l = GTK_LIST(_widget("rumors_list"));
    if (l->children) {
        int n = g_list_length(l->children);
        if (n > 0)
            gtk_list_clear_items(l, 0, n);
    }
    GList* items = NULL;
    for (unsigned int i = 0; i < _m_rumors.size(); i++) {
        GtkListItem* li = GTK_LIST_ITEM(gtk_list_item_new_with_label(_m_rumors[i].getName().c_str()));
#line 134
        assert(li != NULL);
        gtk_widget_show(GTK_WIDGET(li));
        items = g_list_append(items, li);
    }
    gtk_list_append_items(l, items);

    if (g_list_length(l->children) < 30)
        enableWidget("add_rumor_button", TRUE);
    else
        enableWidget("add_rumor_button", FALSE);
    enableWidget("edit_rumor_button", FALSE);
    enableWidget("remove_rumor_button", FALSE);
    if (l->children && g_list_length(l->children))
        enableWidget("remall_rumor_button", TRUE);
    else
        enableWidget("remall_rumor_button", FALSE);
    return true;
}

void TMapSpecsRumorsPage::OnAddRumorButton()
{
    TEditRumorDlg dlg(this, TRumor());
    if (dlg.DoModal() == IDOK) {
        unsigned int newIndex = _m_rumors.size();
        _m_rumors.push_back(dlg.getRumor());
        GtkListItem* li = GTK_LIST_ITEM(gtk_list_item_new_with_label(_m_rumors.back().getName().c_str()));
#line 181
        assert(li != NULL);
        gtk_widget_show(GTK_WIDGET(li));
        GtkList* l = GTK_LIST(_widget("rumors_list"));
        GList* items = g_list_append(NULL, li);
        gtk_list_append_items(l, items);
        setCurrentSelection(l, newIndex);
        if (g_list_length(l->children) >= 30)
            enableWidget("add_rumor_button", FALSE);
        enableWidget("remall_rumor_button", TRUE);
    }
}

void TMapSpecsRumorsPage::OnEditRumorButton()
{
    GtkList* l = GTK_LIST(_widget("rumors_list"));
    int curSel = getCurrentSelection(l);
#line 213
    assert(curSel != -1);
    unsigned int rumorNum = curSel;
    assert(rumorNum < _m_rumors.size());
    _m_bEdited = true;
    TEditRumorDlg dlg(this, _m_rumors[rumorNum]);
    if (dlg.DoModal() == IDOK) {
        _m_rumors[rumorNum] = dlg.getRumor();
        GtkList* list = GTK_LIST(_widget("rumors_list"));
        gtk_list_clear_items(list, rumorNum, rumorNum + 1);
        GtkListItem* li = GTK_LIST_ITEM(gtk_list_item_new_with_label(_m_rumors[rumorNum].getName().c_str()));
#line 234
        assert(li != NULL);
        gtk_widget_show(GTK_WIDGET(li));
        GList* items = g_list_append(NULL, li);
        gtk_list_insert_items(list, items, rumorNum);
        setCurrentSelection(list, rumorNum);
    }
}

void TMapSpecsRumorsPage::OnRemoveRumorButton()
{
    GtkList* l = GTK_LIST(_widget("rumors_list"));
    int curSel = getCurrentSelection(l);
#line 256
    assert(curSel != -1);
    unsigned int rumorNum = curSel;
#line 259
    assert(rumorNum < _m_rumors.size());
    _m_rumors.erase(_m_rumors.begin() + rumorNum);
    gtk_list_clear_items(l, curSel, curSel + 1);
    int max = g_list_length(l->children);
    if (max > 0) {
        if (max < 30)
            enableWidget("add_rumor_button", TRUE);
        gtk_list_select_item(l, curSel >= max ? max - 1 : curSel);
    } else {
        enableWidget("add_rumor_button", TRUE);
        enableWidget("edit_rumor_button", FALSE);
        enableWidget("remove_rumor_button", FALSE);
        enableWidget("remall_rumor_button", FALSE);
    }
}

void TMapSpecsRumorsPage::OnRemoveAllRumorButton()
{
    GtkList* l = GTK_LIST(_widget("rumors_list"));
#line 314
    assert(l->children != NULL);
    int max = g_list_length(l->children);
#line 317
    assert(max > 0);
    for (int i = max; i > 0; i--) {
        setCurrentSelection(l, 0);
        OnRemoveRumorButton();
    }
}

void TMapSpecsRumorsPage::OnSelChangeRumorListbox()
{
    enableWidget("edit_rumor_button", TRUE);
    enableWidget("remove_rumor_button", TRUE);
}

void TMapSpecsRumorsPage::OnSelCancelRumorListbox()
{
    enableWidget("edit_rumor_button", FALSE);
    enableWidget("remove_rumor_button", FALSE);
}

void TMapSpecsRumorsPage::OnDblclkRumorListbox()
{
    OnEditRumorButton();
}
