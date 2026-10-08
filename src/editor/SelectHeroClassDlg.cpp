// SelectHeroClassDlg.cpp - Loki h3maped object 90: the hero class picker.
// OnInitDialog lists every class the mask allows (the class table's names)
// and remembers each row's class; the OK button needs a selection, and a
// double click is an OK. The assert lines come from the retail immediates.
#include "editor/stdafx.h"

namespace {
#include <gtk/gtk.h>
}

#include "editor/cppbridge.h"
#include "editor/SelectHeroClassDlg.h"

TSelectHeroClassDlg* selectHeroClassModal = NULL;

TSelectHeroClassDlg::TSelectHeroClassDlg(GtkWidget* thisWidget, const THeroClassMask& mask)
    : _m_okButton(GTK_BUTTON(_widget("select_hero_ok"))),
      _m_heroClassListBox(GTK_LIST(_widget("select_hero_list"))),
      _m_mask(mask),
      _m_heroClass(THeroClass(kNumHeroClasses)),
      _m_result(0)
{
#line 36
    assert(thisWidget != NULL);
}

int TSelectHeroClassDlg::DoModal()
{
    _m_result = 0;
    OnInitDialog();
    selectHeroClassModal = this;
    GtkWidget* pDialog = _widget("select_hero_dlg");
    gtk_widget_show(pDialog);
    while (_m_result == 0)
        gtk_main_iteration();
    gtk_widget_hide(pDialog);
    selectHeroClassModal = NULL;
    return _m_result;
}

BOOL TSelectHeroClassDlg::OnInitDialog()
{
    emptyList(_m_heroClassListBox);
    unsigned int heroClass;
    GList* items = NULL;
    int item = 0;
    for (heroClass = 0; heroClass < kNumHeroClasses + 1; heroClass++) {
        if (_m_mask[heroClass]) {
            GtkListItem* li = GTK_LIST_ITEM(gtk_list_item_new_with_label(THero::s_akClassTraits[heroClass].m_name));
#line 121
            assert(li != NULL);
            gtk_widget_show(GTK_WIDGET(li));
            items = g_list_append(items, li);
            _m_listItemClasses[item] = THeroClass(heroClass);
            item++;
        }
    }
    gtk_list_append_items(_m_heroClassListBox, items);
    setCurrentSelection(_m_heroClassListBox, -1);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_okButton), FALSE);
    return true;
}

void TSelectHeroClassDlg::OnCancel()
{
    _m_result = 2;
}

void TSelectHeroClassDlg::OnOK()
{
#line 151
    assert(getCurrentSelection(_m_heroClassListBox) != -1);
    int item = getCurrentSelection(_m_heroClassListBox);
    _m_heroClass = _m_listItemClasses[item];
#line 159
    assert(_m_heroClass >= 0 && _m_heroClass < kNumHeroClasses + 1);
    _m_result = 1;
}

void TSelectHeroClassDlg::OnSelChangeHeroClassList()
{
    gtk_widget_set_sensitive(GTK_WIDGET(_m_okButton), getCurrentSelection(_m_heroClassListBox) != -1);
}

void TSelectHeroClassDlg::OnSelCancelHeroClassList()
{
    gtk_widget_set_sensitive(GTK_WIDGET(_m_okButton), FALSE);
}

void TSelectHeroClassDlg::OnDblClkHeroClassList()
{
    if (getCurrentSelection(_m_heroClassListBox) != -1)
        OnOK();
}
