// TownPropsGeneralPage.h - the general page of the town property sheet
// (TownPropsGeneralPage.cpp; Loki h3maped object 93): the town's type,
// owner and name, its visiting hero and, for a random town, its alignment.
// The sheet implements its parent interface, which the page tells when the
// custom name turns blank or not. Layout from the image: the controls from
// 0x8c, the DDX type, hero class and name, custom-name flag and name at
// 0x230, then the parent sheet, the maps, the town's reference, the
// original town, the main-town and blank-name flags, the custom name and
// the modified flags (0x268 bytes, the sheet's new).
#ifndef HOMM3_EDITOR_TOWNPROPSGENERALPAGE_H
#define HOMM3_EDITOR_TOWNPROPSGENERALPAGE_H

#include "editor/MapObjectRef.h"
#include "editor/resource.h"

class TGameMap;
class THero;
class TTown;

// The sheet, told when its OK button may be pressed.
class TTownPropsGeneralPageParentSheet {
public:
    virtual void onEnableOK() = 0;
    virtual void onDisableOK() = 0;
};

class TTownPropsGeneralPage : public CPropertyPage {
public:
    TTownPropsGeneralPage(TTownPropsGeneralPageParentSheet* pParentSheet, const TGameMap& oldMap, TGameMap& newMap,
                          TMapObjectRef townRef);
    virtual ~TTownPropsGeneralPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_TOWN_PROPS_GENERAL };
    CStatic _m_alignmentStatic;
    CComboBox _m_alignmentCombo;
    CComboBox _m_playerCombo;
    CEdit _m_nameEdit;
    CButton _m_removeHeroButton;
    CButton _m_editHeroButton;
    CButton _m_addHeroButton;
    CString _m_townType;
    CString _m_visitingHeroClass;
    CString _m_visitingHeroName;
    BOOL _m_bCustomName;
    CString _m_name;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnAddHeroButton();
    afx_msg void OnEditHeroButton();
    afx_msg void OnRemoveHeroButton();
    afx_msg void OnChangeNameEdit();
    afx_msg void OnCustomizeCheck();
    afx_msg void OnSelChangePlayerCombo();
    DECLARE_MESSAGE_MAP()

private:
    // Creates a hero of a class the user picks, or none.
    THero* _createHero();
    const TTown* _getOldTown() const;
    TTown* _getNewTown();

    TTownPropsGeneralPageParentSheet* _m_pParentSheet;
    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    TMapObjectRef _m_townRef;
    const TTown* _m_pOldTown;
    bool _m_bIsMainTown;
    bool _m_bBlankName;
    CString _m_customName;
    bool _m_bModified;
    bool _m_bVisitingHeroModified;
};

#endif  /* HOMM3_EDITOR_TOWNPROPSGENERALPAGE_H */
