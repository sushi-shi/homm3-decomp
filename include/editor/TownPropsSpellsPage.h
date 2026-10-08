// TownPropsSpellsPage.h - the spells page of the town property sheet (Loki
// TownPropsSpellsPage.cpp): a multi-selection list of the spells the
// town's mage guild may offer; unselected spells are disabled. Layout from
// the image: the town, the list (an {anonymous}::_GtkCList, as
// check_clist_for_selection's mangled name shows), the disabled spells
// mask, the modified flag, the row count, then the vtable pointer (OnOK,
// OnInitDialog). The member names are not proven.
#ifndef HOMM3_EDITOR_TOWNPROPSSPELLSPAGE_H
#define HOMM3_EDITOR_TOWNPROPSSPELLSPAGE_H

#include "editor/stdafx.h"

#include <bitset>

#include "editor/Town.h"

namespace {
#include <gtk/gtk.h>
}

class TTownPropsSpellsPage {
public:
    TTownPropsSpellsPage(const TTown& town);
    ~TTownPropsSpellsPage();

    virtual void OnOK();
    virtual BOOL OnInitDialog();

    const bitset<kNumSpells>& getDisabledSpellsMask() const { return _m_disabledSpellsMask; }
    bool wasModified() const { return _m_bModified; }

private:
    gboolean check_clist_for_selection(GtkCList* clist, int row);

    const TTown& _m_town;
    GtkCList* _m_spellList;
    bitset<kNumSpells> _m_disabledSpellsMask;
    bool _m_bModified;
    int _m_numRows;
};

#endif  /* HOMM3_EDITOR_TOWNPROPSSPELLSPAGE_H */
