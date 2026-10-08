// SelectHeroClassDlg.h - the hero class picker (Loki SelectHeroClassDlg.cpp):
// the select_hero_dlg glade dialog lists the classes a mask allows. Layout
// from the image: the OK button, the class list (_m_heroClassListBox, from
// OnOK's assert), the mask, the chosen class (_m_heroClass, assert), the
// list row to class table (8 KiB), the modal result, then the vtable pointer
// (OnInitDialog, OnOK, OnCancel). The other member names and the table's
// bound are not proven.
#ifndef HOMM3_EDITOR_SELECTHEROCLASSDLG_H
#define HOMM3_EDITOR_SELECTHEROCLASSDLG_H

#include "editor/stdafx.h"

#include "editor/Hero.h"

class TSelectHeroClassDlg {
public:
    TSelectHeroClassDlg(GtkWidget* thisWidget, const THeroClassMask& mask);

    int DoModal();
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();

    void OnSelChangeHeroClassList();
    void OnSelCancelHeroClassList();
    void OnDblClkHeroClassList();

    THeroClass getHeroClass() const { return _m_heroClass; }

private:
    enum { _s_kMaxListItems = 2048 };

    GtkButton* _m_okButton;
    GtkList* _m_heroClassListBox;
    THeroClassMask _m_mask;
    THeroClass _m_heroClass;
    THeroClass _m_listItemClasses[_s_kMaxListItems];
    int _m_result;
};

// The open dialog, for cppbridge.cpp's select_hero_dlg signal handlers.
extern TSelectHeroClassDlg* selectHeroClassModal;

#endif  /* HOMM3_EDITOR_SELECTHEROCLASSDLG_H */
