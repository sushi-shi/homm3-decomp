// TownPropsSpellsPage.cpp - Loki h3maped object 95: the spells page of the
// town property sheet. The list holds every spell of a mage guild level
// the town type has (and, for a real town type, a positive chance in it),
// selected unless disabled. The assert lines come from the retail
// immediates.
#include "editor/stdafx.h"

#include "editor/TownPropsSpellsPage.h"
#include "spelldefs.h"
#include "editor/cppbridge.h"

TTownPropsSpellsPage::TTownPropsSpellsPage(const TTown& town)
    : _m_town(town),
      _m_spellList(GTK_CLIST(_widget("town_props_spells_list"))),
      _m_bModified(false),
      _m_numRows(0)
{
    OnInitDialog();
}

TTownPropsSpellsPage::~TTownPropsSpellsPage()
{
}

BOOL TTownPropsSpellsPage::OnInitDialog()
{
    _m_bModified = false;
    _m_disabledSpellsMask = _m_town.getDisabledSpellsMask();
    _m_numRows = 0;
    gtk_clist_clear(_m_spellList);
    TTownType townType = _m_town.getTownType();
    const TTown::TTypeTraits& typeTraits = _m_town.getTownTypeTraits();
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        if (akSpellTraits[spell].m_school != 0 && typeTraits.hasMageGuildLevel(akSpellTraits[spell].m_level - 1)) {
            if (townType >= kNumTownTypes || akSpellTraits[spell].m_townGetsItChance[townType] > 0) {
                gchar* text[2] = { (gchar*)akSpellTraits[spell].m_name, NULL };
                gint row = gtk_clist_append(_m_spellList, text);
                _m_numRows++;
                gtk_clist_set_row_data(_m_spellList, row, (gpointer)spell);
                if (_m_disabledSpellsMask[spell])
                    gtk_clist_unselect_row(_m_spellList, row, 0);
                else
                    gtk_clist_select_row(_m_spellList, row, 0);
            }
        }
    }
    return true;
}

gboolean TTownPropsSpellsPage::check_clist_for_selection(GtkCList* clist, int row)
{
    GtkCListRow* clistRow = (GtkCListRow*)g_list_nth(clist->row_list, row)->data;
    if (clistRow->state == GTK_STATE_SELECTED)
        return TRUE;
    return FALSE;
}

void TTownPropsSpellsPage::OnOK()
{
    int i;
    int numRows = _m_numRows;
    for (i = 0; i < numRows; i++) {
        int spell = (int)gtk_clist_get_row_data(_m_spellList, i);
#line 145
        assert(spell >= 0 && spell < kNumSpells);
        assert(akSpellTraits[ spell ].m_school != 0);
        _m_disabledSpellsMask[spell] = !check_clist_for_selection(_m_spellList, i);
    }
    _m_bModified = _m_bModified || _m_disabledSpellsMask != _m_town.getDisabledSpellsMask();
}
