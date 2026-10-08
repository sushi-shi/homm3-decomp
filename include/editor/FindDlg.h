// FindDlg.h - the find dialog of the map view (Loki FindDlg.cpp, object
// 78): the find_dlg glade dialog lists every findable object kind (object
// types, artifacts, creature banks and generators, garrisons, hero
// classes, mines, monsters, resources and towns) and returns the chosen
// one's type and extra. Layout from the image: the type, the extra, the
// search direction, the entry number of each list row, then the vtable
// pointer (OnInitDialog, OnOK). The member names other than those the
// accessors and asserts name are not proven.
#ifndef HOMM3_EDITOR_FINDDLG_H
#define HOMM3_EDITOR_FINDDLG_H

#include "editor/stdafx.h"

class TFindDlg {
public:
    TFindDlg(int findType, int findExtra);

    virtual BOOL OnInitDialog();
    virtual void OnOK();

    int getFindType() const { return _m_findType; }
    int getFindExtra() const { return _m_findExtra; }
    bool getBSearchBackwards() const { return _m_bSearchBackwards; }

private:
    enum { _s_kMaxComboItems = 1024 };

    int _m_findType;
    int _m_findExtra;
    bool _m_bSearchBackwards;
    unsigned int _m_entryItemData[_s_kMaxComboItems];
};

#endif  /* HOMM3_EDITOR_FINDDLG_H */
