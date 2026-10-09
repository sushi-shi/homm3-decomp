// MapSpecsAvailableHeroesPage.h - the heroes page of the map
// specifications sheet (MapSpecsAvailableHeroesPage.cpp; GOG only): a
// check list of the heroes of the map's version, by class, checked when
// available; on Shadow of Death maps the properties button edits the
// selected hero's definition. Layout from the image: the properties
// button, the map before and during the sheet, the modified flag, an
// unused hero mask and the check list (0x12c bytes).
#ifndef HOMM3_EDITOR_MAPSPECSAVAILABLEHEROESPAGE_H
#define HOMM3_EDITOR_MAPSPECSAVAILABLEHEROESPAGE_H

#include <bitset>

#include "editor/GameMap.h"
#include "editor/Hero.h"
#include "editor/resource.h"

class TMapSpecsAvailableHeroesPage : public CPropertyPage {
public:
    TMapSpecsAvailableHeroesPage(const TGameMap& oldMap, TGameMap& newMap);
    virtual ~TMapSpecsAvailableHeroesPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MAP_SPECS_AVAILABLE_HEROES };
    CButton _m_propertiesButton;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnSelChangeHeroList();
    afx_msg void OnSelCancelHeroList();
    afx_msg void OnPropertiesButton();
    afx_msg void OnDblClkHeroList();
    afx_msg void OnCheckChangeHeroList();
    DECLARE_MESSAGE_MAP()

private:
    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    bool _m_bModified;
    std::bitset<kNumHeroes> _m_disabledHeroes;
    CCheckListBox _m_heroList;
};

#endif  /* HOMM3_EDITOR_MAPSPECSAVAILABLEHEROESPAGE_H */
