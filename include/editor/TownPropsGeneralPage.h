// TownPropsGeneralPage.h - the town property sheet's general page (Loki
// TownPropsGeneralPage.cpp): owner, custom name and visiting hero over the
// town_props_general_* glade widgets. A CWnd (its vtable follows CWnd's
// fields: OnCaptureChanged, OnOK, OnInitDialog). The page edits working
// copies (_m_owner, _m_name, _m_pVisitingHero) and OnOK publishes them to
// the _m_int* results the sheet reads. Layout from the image; member names
// from the asserts (_m_pTown, _m_pMap, _m_bIsMainTown, _m_bCustomName,
// _m_bNameIsSpace, _m_owner, _m_pVisitingHero, _m_pIntVisitingHero,
// _m_playerCombo), the others are not proven.
#ifndef HOMM3_EDITOR_TOWNPROPSGENERALPAGE_H
#define HOMM3_EDITOR_TOWNPROPSGENERALPAGE_H

#include "editor/stdafx.h"

#include <string>

#include "editor/Hero.h"
#include "editor/Player.h"

class TGameMap;
class TTown;

class TTownPropsGeneralPage : public CWnd {
public:
    TTownPropsGeneralPage(TTown* pTown, const TGameMap& map, bool bIsMainTown);
    ~TTownPropsGeneralPage();

    virtual void OnOK();
    virtual BOOL OnInitDialog();

    void OnVisitingAddButton();
    void OnVisitingEditButton();
    void OnVisitingRemoveButton();
    void OnChangeNameEdit();
    void OnCustomizeCheck();
    void UpdateData(bool bSaveAndValidate);
    void OnSelChangePlayerCombo();
    void OnDestroy();

    TPlayer getOwner() const { return _m_intOwner; }
    bool getBCustomName() const { return _m_bIntCustomName; }
    const string& getName() const { return _m_intName; }
    THero* getPVisitingHero() const { return _m_pIntVisitingHero; }
    bool getBVisitingHeroRemoved() const { return _m_bIntVisitingHeroRemoved; }
    bool wasModified() const { return _m_bModified; }

private:
    enum { _s_kMaxComboItems = 1024 };

    THero* _createHero();

    GtkCombo* _m_playerCombo;
    GtkEntry* _m_nameEdit;
    GtkButton* _m_removeButton;
    GtkButton* _m_editButton;
    GtkButton* _m_addButton;
    string _m_townTypeName;
    string _m_visitingHeroClass;
    string _m_visitingHeroName;
    bool _m_bCustomName;
    string _m_nameText;
    TTown* _m_pTown;
    TGameMap* _m_pMap;
    bool _m_bIsMainTown;
    bool _m_bInitialized;
    bool _m_bNameIsSpace;
    string _m_name;
    TPlayer _m_owner;
    THero* _m_pVisitingHero;
    bool _m_bVisitingHeroRemoved;
    bool _m_bIntCustomName;
    string _m_intName;
    TPlayer _m_intOwner;
    THero* _m_pIntVisitingHero;
    bool _m_bIntVisitingHeroRemoved;
    bool _m_bModified;
    bool _m_bVisitingHeroModified;
    int _m_aComboIndexPlayer[_s_kMaxComboItems];
};

#endif  /* HOMM3_EDITOR_TOWNPROPSGENERALPAGE_H */
