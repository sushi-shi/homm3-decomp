// MapSpecsSpellsPage.h - the spells page of the map specifications sheet
// (MapSpecsSpellsPage.cpp; GOG only): a check list of the spells the map
// allows. Every spell level keeps at least one allowed spell. Layout from
// the image: the map before and during the sheet at 0x8c, the modified
// flag, the disabled spells at 0x98, the allowed count of each level at
// 0xa4 and the list at 0xb8.
#ifndef HOMM3_EDITOR_MAPSPECSSPELLSPAGE_H
#define HOMM3_EDITOR_MAPSPECSSPELLSPAGE_H

#include <bitset>

#include "armygrp.h"
#include "editor/resource.h"

class TGameMap;

class TMapSpecsSpellsPage : public CPropertyPage {
public:
    enum { s_kNumSpellLevels = 5 };

    TMapSpecsSpellsPage(const TGameMap& oldMap, TGameMap& newMap);
    virtual ~TMapSpecsSpellsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MAP_SPECS_SPELLS };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnCheckChangeSpellsList();
    DECLARE_MESSAGE_MAP()

private:
    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    bool _m_bModified;
    std::bitset<kNumSpells> _m_disabledSpells;
    unsigned int _m_aNumAllowedSpells[s_kNumSpellLevels];

public:
    CCheckListBox _m_spellsList;
};

#endif  /* HOMM3_EDITOR_MAPSPECSSPELLSPAGE_H */
