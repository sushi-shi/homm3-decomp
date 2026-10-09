// HeroPropsGeneralPage.h - the general pages of the hero property sheets
// (HeroPropsGeneralPage.cpp; Loki h3maped object 84). Each page edits the
// hero in a copy of the map, found by its reference: a hero object or the
// visiting hero of a town. The common page holds the experience and the
// patrol radius; the random hero's page adds the owner. A specific hero's
// page (RTTI TIdentifiedHeroPropsGeneralPage) adds the identity, name,
// portrait and gender and tells its sheet when the identity changes or the
// custom name turns blank; the specific hero page adds the owner and the
// class's name, the prison page the class.
//
// Layouts from the image: the common page's members from 0x8c (one word at
// 0x9c is never touched), the experience at 0xac and the three controls
// from 0xb4 (0x168 bytes); the random page from 0x168 (0x1c4 bytes); the
// specific hero's page from 0x168: the sheet, the maps, the reference, the
// heroes, the modified flag, the 163 portraits, the portrait bitmap and
// the controls from 0x198, of which a combo box and a check at 0x3b4 and
// 0x3f0 are constructed but never exchanged, then the identity, name,
// portrait, blank-name flag and gender radio index (0x440 bytes); the
// specific hero and prison pages from 0x444 (0x4a4 bytes each).
#ifndef HOMM3_EDITOR_HEROPROPSGENERALPAGE_H
#define HOMM3_EDITOR_HEROPROPSGENERALPAGE_H

#include <memory>

#include "autoarrayptr.h"
#include "gameversion.h"
#include "heroclass.h"
#include "editor/DIBSection.h"
#include "editor/Hero.h"
#include "editor/MapObjectRef.h"
#include "editor/resource.h"

class TGameMap;

class THeroPropsGeneralPage : public CPropertyPage {
public:
    THeroPropsGeneralPage(UINT nIDTemplate, const TGameMap& oldMap, TGameMap& newMap, TMapObjectRef heroRef);
    virtual ~THeroPropsGeneralPage();

    bool wasModified() const { return _m_bModified; }

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    // The longest experience the combo's edit takes.
    enum { s_kMaxExperience = 99999 };

    // Shows the hero's default experience unless the page customizes it.
    void _setDefaultExperience(THeroID heroID);

    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnCustomizeExperienceCheck();
    DECLARE_MESSAGE_MAP()

private:
    const THero* _getOldHero() const;
    THero* _getNewHero();

    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    TMapObjectRef _m_heroRef;
    int _m_unused;
    const THero* _m_pOldHero;
    THero* _m_pNewHero;
    bool _m_bModified;
    int _m_experience;
    bool _m_bCustomExperience;

public:
    CComboBox _m_experienceCombo;
    CComboBox _m_patrolCombo;
    CButton _m_customizeExperienceCheck;
};

class TRandomHeroPropsGeneralPage : public THeroPropsGeneralPage {
public:
    TRandomHeroPropsGeneralPage(const TGameMap& oldMap, TGameMap& newMap, TMapObjectRef heroRef,
                                bool bAnyAvailableOwner);
    virtual ~TRandomHeroPropsGeneralPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_RANDOM_HERO_PROPS_GENERAL };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

private:
    const TRandomHero* _getOldHero() const;
    TRandomHero* _getNewHero();

    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    TMapObjectRef _m_heroRef;
    bool _m_bAnyAvailableOwner;
    const TRandomHero* _m_pOldHero;
    TRandomHero* _m_pNewHero;
    bool _m_bModified;

public:
    CComboBox _m_ownerCombo;
};

// The sheet of a specific hero's page: told when its OK button may be
// pressed and when the page picks another hero.
class TIdentifiedHeroPropsGeneralPageParentSheet {
public:
    virtual void onEnableOK() = 0;
    virtual void onDisableOK() = 0;
    virtual void onSetHeroID(THeroID heroID) = 0;
};

class TIdentifiedHeroPropsGeneralPage : public THeroPropsGeneralPage {
public:
    TIdentifiedHeroPropsGeneralPage(UINT nIDTemplate, TIdentifiedHeroPropsGeneralPageParentSheet* pParentSheet,
                                    const TGameMap& oldMap, TGameMap& newMap, TMapObjectRef heroRef);
    virtual ~TIdentifiedHeroPropsGeneralPage();

    // Picks the hero, listing the available heroes of its class first
    // when the class changes.
    void setHeroID(THeroID heroID);

    bool wasModified() const { return _m_bModified; }

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnSelChangeIdentityCombo();
    afx_msg void OnDeltaposPortraitSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnChangeNameEdit();
    afx_msg void OnCustomizeNameCheck();
    afx_msg void OnCustomizePortraitCheck();
    DECLARE_MESSAGE_MAP()

private:
    // The portraits a map of the version may show.
    static int _getNumPortraits(EGameVersion version)
    {
        return version >= GAME_VERSION_SOD ? 163 : version >= GAME_VERSION_AB ? 159 : 130;
    }

    const TIdentifiedHero* _getOldHero() const;
    TIdentifiedHero* _getNewHero();
    // A Restoration of Erathia map shows other portraits in place of 128
    // and 129.
    void _adjustPortrait(int& portrait) const;
    void _onSetHeroID();
    void _setPortrait(int portrait);

    TIdentifiedHeroPropsGeneralPageParentSheet* _m_pParentSheet;
    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    TMapObjectRef _m_heroRef;
    const TIdentifiedHero* _m_pOldHero;
    TIdentifiedHero* _m_pNewHero;
    bool _m_bModified;
    TAutoArrayPtr<std::auto_ptr<T16bppDIBSection> > _m_apPortrait;
    CBitmap _m_portraitBitmap;

public:
    CComboBox _m_identityCombo;
    CEdit _m_nameEdit;
    CButton _m_customizeNameCheck;
    CStatic _m_portraitStatic;
    CSpinButtonCtrl _m_portraitSpin;
    CButton _m_customizePortraitCheck;
    CButton _m_defaultGenderRadio;
    CButton _m_maleRadio;
    CButton _m_femaleRadio;
    CComboBox _m_experienceCombo;
    CButton _m_customizeExperienceCheck;

private:
    THeroID _m_heroID;
    CString _m_name;
    int _m_portrait;
    bool _m_bBlankName;
    int _m_gender;
};

class TNonRandomHeroPropsGeneralPage : public TIdentifiedHeroPropsGeneralPage {
public:
    TNonRandomHeroPropsGeneralPage(TIdentifiedHeroPropsGeneralPageParentSheet* pParentSheet, const TGameMap& oldMap,
                                   TGameMap& newMap, TMapObjectRef heroRef, bool bAnyAvailableOwner);
    virtual ~TNonRandomHeroPropsGeneralPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_HERO_PROPS_GENERAL };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

private:
    const TNonRandomHero* _getOldHero() const;
    TNonRandomHero* _getNewHero();

    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    TMapObjectRef _m_heroRef;
    bool _m_bAnyAvailableOwner;
    const TNonRandomHero* _m_pOldHero;
    TNonRandomHero* _m_pNewHero;
    bool _m_bModified;

public:
    CComboBox _m_ownerCombo;
    CString _m_className;
};

class TPrisonPropsGeneralPage : public TIdentifiedHeroPropsGeneralPage {
public:
    TPrisonPropsGeneralPage(TIdentifiedHeroPropsGeneralPageParentSheet* pParentSheet, const TGameMap& oldMap,
                            TGameMap& newMap, TMapObjectRef prisonRef);
    virtual ~TPrisonPropsGeneralPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_PRISON_PROPS_GENERAL };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnSelChangeClassCombo();
    DECLARE_MESSAGE_MAP()

private:
    const TPrison* _getOldPrison() const;
    TPrison* _getNewPrison();

    TIdentifiedHeroPropsGeneralPageParentSheet* _m_pParentSheet;
    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    TMapObjectRef _m_prisonRef;
    const TPrison* _m_pOldPrison;
    TPrison* _m_pNewPrison;
    bool _m_bModified;

public:
    CComboBox _m_classCombo;

private:
    THeroClass _m_heroClass;
};

#endif  /* HOMM3_EDITOR_HEROPROPSGENERALPAGE_H */
