// TownPropsSpellsPage.h - the spells page of the town property sheet
// (TownPropsSpellsPage.cpp; Loki h3maped object 95): for one mage guild
// level, the spells that must appear in the guild and those that may.
// Layout from the image: the level combo at 0x8c, the maps, the town's
// reference, the original town and the modified flag from 0xc8, the two
// spell masks at 0xe0 and 0xec, each level's obligatory spell count at
// 0xf8, then the two check lists (0x194 bytes, the sheet's new).
#ifndef HOMM3_EDITOR_TOWNPROPSSPELLSPAGE_H
#define HOMM3_EDITOR_TOWNPROPSSPELLSPAGE_H

#include <bitset>

#include "armygrp.h"
#include "editor/MapObjectRef.h"
#include "editor/resource.h"

class TGameMap;
class TTown;

class TTownPropsSpellsPage : public CPropertyPage {
public:
    enum { s_kNumSpellLevels = 5 };

    TTownPropsSpellsPage(const TGameMap& oldMap, TGameMap& newMap, TMapObjectRef townRef);
    virtual ~TTownPropsSpellsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_TOWN_PROPS_SPELLS };
    CComboBox _m_levelCombo;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnSelChangeLevelCombo();
    afx_msg void OnCheckChangeObligatorySpellsList();
    afx_msg void OnCheckChangeDisabledSpellsList();
    DECLARE_MESSAGE_MAP()

private:
    void _fillSpellLists(unsigned int level);
    const TTown* _getOldTown() const;
    TTown* _getNewTown();

    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    TMapObjectRef _m_townRef;
    const TTown* _m_pOldTown;
    bool _m_bModified;
    std::bitset<kNumSpells> _m_obligatorySpells;
    std::bitset<kNumSpells> _m_disabledSpells;
    int _m_aNumObligatorySpells[s_kNumSpellLevels];
    CCheckListBox _m_obligatorySpellsList;
    CCheckListBox _m_disabledSpellsList;
};

#endif  /* HOMM3_EDITOR_TOWNPROPSSPELLSPAGE_H */
