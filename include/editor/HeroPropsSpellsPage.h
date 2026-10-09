// HeroPropsSpellsPage.h - the spells page of the hero property sheets
// (GOG HeroPropsSpellsPage.cpp, not in Loki): a customize check over a
// TSpellsDlg child that shows the custom spells, or the hero's default
// ones while uncustomized. Before Armageddon's Blade a map keeps no custom
// spells. Layout from the image: the check at 0x8c, the original and
// edited heroes, the map's version, the modified flag, the default and
// custom spells, the custom flag and the dialog (0xfc bytes, the sheets'
// new).
#ifndef HOMM3_EDITOR_HEROPROPSSPELLSPAGE_H
#define HOMM3_EDITOR_HEROPROPSSPELLSPAGE_H

#include <bitset>
#include <memory>

#include "armygrp.h"
#include "gameversion.h"
#include "editor/resource.h"
#include "editor/SpellsDlg.h"

class THero;

class THeroPropsSpellsPage : public CPropertyPage {
public:
    THeroPropsSpellsPage(const THero* pOldHero, THero* pNewHero, EGameVersion mapVersion);
    virtual ~THeroPropsSpellsPage();

    bool wasModified() const { return _m_bModified; }
    // The spells shown while uncustomized (the hero's identity's).
    void setDefaultSpells(const std::bitset<kNumSpells>& spells);

    enum { IDD = IDD_HERO_PROPS_SPELLS };
    CButton _m_customizeCheck;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnCustomizeCheck();
    DECLARE_MESSAGE_MAP()

private:
    const THero* _m_pOldHero;
    THero* _m_pNewHero;
    EGameVersion _m_mapVersion;
    bool _m_bModified;
    std::bitset<kNumSpells> _m_defaultSpells;
    std::bitset<kNumSpells> _m_customSpells;
    bool _m_bCustomSpells;
    std::auto_ptr<TSpellsDlg> _m_pSpellsDlg;
};

#endif  /* HOMM3_EDITOR_HEROPROPSSPELLSPAGE_H */
