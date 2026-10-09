// MapSpecsSecSkillsPage.h - the secondary skills page of the map
// specifications sheet (MapSpecsSecSkillsPage.cpp; GOG only): a check
// list of the skills the map allows; at least four stay allowed. Layout
// from the image: the map before and during the sheet at 0x8c, the
// modified flag, the disabled skills at 0x98 and the list at 0x9c.
#ifndef HOMM3_EDITOR_MAPSPECSSECSKILLSPAGE_H
#define HOMM3_EDITOR_MAPSPECSSECSKILLSPAGE_H

#include <bitset>

#include "secondaryskill.h"
#include "editor/resource.h"

class TGameMap;

class TMapSpecsSecSkillsPage : public CPropertyPage {
public:
    enum { s_kMinNumAllowedSkills = 4 };

    TMapSpecsSecSkillsPage(const TGameMap& oldMap, TGameMap& newMap);
    virtual ~TMapSpecsSecSkillsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MAP_SPECS_SEC_SKILLS };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnCheckChangeSecSkillsList();
    DECLARE_MESSAGE_MAP()

private:
    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    bool _m_bModified;
    std::bitset<kNumSecSkills> _m_disabledSkills;

public:
    CCheckListBox _m_secSkillsList;
};

#endif  /* HOMM3_EDITOR_MAPSPECSSECSKILLSPAGE_H */
