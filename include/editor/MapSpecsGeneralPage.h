// MapSpecsGeneralPage.h - the general page of the map specifications
// sheet (MapSpecsGeneralPage.cpp; Loki h3maped object 70): the name,
// description, difficulty, hero level limit, map version and the
// underground level. Layout from the image: the RoE radio, level spin,
// level edit, level check and the description and name edits from 0x8c,
// the DDX difficulty, description, name, two-level flag and version at
// 0x1f4, the AB radio at 0x208, then the map before and during the sheet,
// the strength a new underground is painted with and the modified flag.
#ifndef HOMM3_EDITOR_MAPSPECSGENERALPAGE_H
#define HOMM3_EDITOR_MAPSPECSGENERALPAGE_H

#include "editor/resource.h"

class TGameMap;

class TMapSpecsGeneralPage : public CPropertyPage {
public:
    enum { s_kMaxNameLen = 30, s_kMaxDescLen = 300, s_kMaxHeroLevel = 99 };

    TMapSpecsGeneralPage(const TGameMap& oldMap, TGameMap& newMap, int terrainStrength);
    virtual ~TMapSpecsGeneralPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MAP_SPECS_GENERAL };
    CButton _m_roeRadio;
    CSpinButtonCtrl _m_heroLevelSpin;
    CEdit _m_heroLevelEdit;
    CButton _m_limitHeroLevelCheck;
    CEdit _m_descriptionEdit;
    CEdit _m_nameEdit;
    int _m_difficulty;
    CString _m_description;
    CString _m_name;
    BOOL _m_bTwoLevel;
    int _m_version;
    CButton _m_abRadio;

    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual BOOL OnApply();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnKillFocusHeroLevelEdit();
    afx_msg void OnLimitHeroLevelCheck();
    DECLARE_MESSAGE_MAP()

private:
    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    int _m_terrainStrength;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_MAPSPECSGENERALPAGE_H */
