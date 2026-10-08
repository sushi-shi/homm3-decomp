// HeroPropsGeneralPage.h - the hero sheet's general page (Loki
// HeroPropsGeneralPage.cpp) and its random hero, specific hero and prison
// variants, with the sheet interfaces they report to. The page shows the
// name, portrait (a scrolled strip of 130 portrait pixmaps), experience,
// patrol and owner; the specific hero page adds the identity combo, the
// prison page the class and identity combos. Layout from the image:
// _m_pParentSheet (assert), the hero, the prototype, the portrait pixmaps,
// the entries' strings, the patrol selection, the two customize toggles,
// the default name, the portrait selection, the custom flags, name,
// portrait, experience and patrol, the blank-name flag, the modified flag,
// the owner, identity and class combos' row tables (1000 rows each), then
// the vtable pointer (OnOK, OnInitDialog). The derived pages keep their own
// _m_pParentSheet and hero references (the asserts name _m_hero,
// _m_prison, _m_availableProtoNums and _m_aAvailableProtoNums). The other
// member names are not proven.
#ifndef HOMM3_EDITOR_HEROPROPSGENERALPAGE_H
#define HOMM3_EDITOR_HEROPROPSGENERALPAGE_H

#include "editor/stdafx.h"

#include <bitset>
#include <set>
#include <string>

#include "editor/Array.h"
#include "editor/Hero.h"
#include "editor/Player.h"

// The sheet's OK button follows whether the page's entries are valid.
class THeroPropsGeneralPageParentSheet {
public:
    virtual void onEnableOK() = 0;
    virtual void onDisableOK() = 0;
};

// Told when the specific hero page picks another hero of the class.
class TNonRandomHeroPropsGeneralPageParentSheet : public virtual THeroPropsGeneralPageParentSheet {
public:
    virtual void onSetNewProtoNum(unsigned int newProtoNum) = 0;
};

// Told when the prison page picks another class or hero.
class TPrisonPropsGeneralPageParentSheet : public virtual THeroPropsGeneralPageParentSheet {
public:
    virtual void onSetNewIdentity(THeroClass newHeroClass, unsigned int newProtoNum) = 0;
};

class THeroPropsGeneralPage {
public:
    THeroPropsGeneralPage(unsigned int idTemplate, THeroPropsGeneralPageParentSheet* pParentSheet, const THero& hero);
    ~THeroPropsGeneralPage();

    void UpdateData(bool bSaveAndValidate);
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    void OnChangeNameEdit();
    void OnCustomizeNameCheck();
    void OnCustomizePortraitCheck();

    bool getBCustomName() const { return _m_bCustomName; }
    bool getBCustomPortrait() const { return _m_bCustomPortrait; }
    string getName() const { return _m_name; }
    int getPortrait() const { return _m_portrait; }
    int getExperience() const { return _m_experience; }
    int getPatrol() const { return _m_patrol; }
    bool wasModified() const { return _m_bModified; }

protected:
    enum { _s_kMaxComboItems = 1000 };

    void _setNewPortrait(int portrait);

    THeroPropsGeneralPageParentSheet* _m_pParentSheet;
    const THero& _m_hero;
    const THeroPrototype* _m_pPrototype;
    GdkPixmap** _m_apPortraitPixmap;
    string _m_nameText;
    string _m_experienceText;
    int _m_patrolSel;
    bool _m_bCustomizeName;
    bool _m_bCustomizePortrait;
    string _m_defaultName;
    int _m_portraitSel;
    bool _m_bCustomName;
    bool _m_bCustomPortrait;
    string _m_name;
    int _m_portrait;
    int _m_experience;
    int _m_patrol;
    bool _m_bBlankName;
    bool _m_bModified;
    int _m_playerItemData[_s_kMaxComboItems];
    int _m_identityItemData[_s_kMaxComboItems];
    int _m_classItemData[_s_kMaxComboItems];
};

class TRandomHeroPropsGeneralPage : public THeroPropsGeneralPage {
public:
    TRandomHeroPropsGeneralPage(THeroPropsGeneralPageParentSheet* pParentSheet, const TRandomHero& hero,
                                const bitset<kNumPlayers>& availableOwners);

    virtual void OnOK();
    virtual BOOL OnInitDialog();

    TPlayer getOwner() const { return _m_owner; }

private:
    const TRandomHero& _m_hero;
    bitset<kNumPlayers> _m_availableOwners;
    TPlayer _m_owner;
};

class TNonRandomHeroPropsGeneralPage : public THeroPropsGeneralPage {
public:
    TNonRandomHeroPropsGeneralPage(TNonRandomHeroPropsGeneralPageParentSheet* pParentSheet, const TNonRandomHero& hero,
                                   const bitset<kNumPlayers>& availableOwners,
                                   const set<unsigned int>& availableProtoNums);

    void setProtoNum(unsigned int newProtoNum);
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    void OnSelChangeIdentityCombo();

    TPlayer getOwner() const { return _m_owner; }
    unsigned int getProtoNum() const { return _m_protoNum; }

private:
    TNonRandomHeroPropsGeneralPageParentSheet* _m_pParentSheet;
    const TNonRandomHero& _m_hero;
    bitset<kNumPlayers> _m_availableOwners;
    set<unsigned int> _m_availableProtoNums;
    string _m_identityText;
    TPlayer _m_owner;
    unsigned int _m_protoNum;
};

class TPrisonPropsGeneralPage : public THeroPropsGeneralPage {
public:
    TPrisonPropsGeneralPage(TPrisonPropsGeneralPageParentSheet* pParentSheet, const TPrison& prison,
                            const TArray<set<unsigned int>, kNumHeroClasses>& aAvailableProtoNums);

    void setIdentity(THeroClass newHeroClass, unsigned int newProtoNum);
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    void OnSelChangeClassCombo();
    void OnSelChangeIdentityCombo();

    THeroClass getHeroClass() const { return _m_heroClass; }
    unsigned int getProtoNum() const { return _m_protoNum; }

private:
    TPrisonPropsGeneralPageParentSheet* _m_pParentSheet;
    const TPrison& _m_prison;
    TArray<set<unsigned int>, kNumHeroClasses> _m_aAvailableProtoNums;
    THeroClass _m_heroClass;
    unsigned int _m_protoNum;
};

#endif  /* HOMM3_EDITOR_HEROPROPSGENERALPAGE_H */
