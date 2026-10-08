// HeroPropsCreaturesPage.h - the hero sheet's creatures page (Loki
// HeroPropsCreaturesPage.cpp): the customize toggle, the formation radio
// buttons and a TArmyDlg over the "hp_" creature slots. Layout from the
// image: TArmyDlgClient (privately inherited, its vtable pointer first),
// the formation (an int, -1 until OnInitDialog), the parent sheet
// (_m_pParentSheet, assert), the hero, the modified flag, the army dialog
// and the custom army flag. The other member names are not proven.
#ifndef HOMM3_EDITOR_HEROPROPSCREATURESPAGE_H
#define HOMM3_EDITOR_HEROPROPSCREATURESPAGE_H

#include "editor/stdafx.h"

#include "editor/ArmyDlg.h"
#include "editor/Hero.h"

// The sheet's OK button follows whether a customized army has a stack.
class THeroPropsCreaturesPageParentSheet {
public:
    virtual void onEnableOK() = 0;
    virtual void onDisableOK() = 0;
};

class THeroPropsCreaturesPage : TArmyDlgClient {
public:
    THeroPropsCreaturesPage(THeroPropsCreaturesPageParentSheet* pParentSheet, const THero& hero);
    ~THeroPropsCreaturesPage();

    void setIdentity(THeroClass newHeroClass, unsigned int newProtoNum);
    const TArmy& getArmy() const;

    virtual void OnOK();
    virtual BOOL OnInitDialog();
    void OnCustomizeCheck();

    bool getBCustomArmy() const { return _m_bCustomArmy; }
    bool getBGroupedFormation() const { return _m_formation; }
    bool wasModified() const { return _m_bModified; }

    void OnSelChangeTypeCombo1() { _m_pArmyDlg->OnSelChangeTypeCombo1(); }
    void OnSelChangeTypeCombo2() { _m_pArmyDlg->OnSelChangeTypeCombo2(); }
    void OnSelChangeTypeCombo3() { _m_pArmyDlg->OnSelChangeTypeCombo3(); }
    void OnSelChangeTypeCombo4() { _m_pArmyDlg->OnSelChangeTypeCombo4(); }
    void OnSelChangeTypeCombo5() { _m_pArmyDlg->OnSelChangeTypeCombo5(); }
    void OnSelChangeTypeCombo6() { _m_pArmyDlg->OnSelChangeTypeCombo6(); }
    void OnSelChangeTypeCombo7() { _m_pArmyDlg->OnSelChangeTypeCombo7(); }
    void OnKillFocusQtyEdit1() { _m_pArmyDlg->OnKillFocusQtyEdit1(); }
    void OnKillFocusQtyEdit2() { _m_pArmyDlg->OnKillFocusQtyEdit2(); }
    void OnKillFocusQtyEdit3() { _m_pArmyDlg->OnKillFocusQtyEdit3(); }
    void OnKillFocusQtyEdit4() { _m_pArmyDlg->OnKillFocusQtyEdit4(); }
    void OnKillFocusQtyEdit5() { _m_pArmyDlg->OnKillFocusQtyEdit5(); }
    void OnKillFocusQtyEdit6() { _m_pArmyDlg->OnKillFocusQtyEdit6(); }
    void OnKillFocusQtyEdit7() { _m_pArmyDlg->OnKillFocusQtyEdit7(); }

private:
    virtual void onNumOccupiedStacksChanged(unsigned int newNum, unsigned int oldNum);

    int _m_formation;
    THeroPropsCreaturesPageParentSheet* _m_pParentSheet;
    const THero& _m_hero;
    bool _m_bModified;
    TArmyDlg* _m_pArmyDlg;
    bool _m_bCustomArmy;
};

#endif  /* HOMM3_EDITOR_HEROPROPSCREATURESPAGE_H */
