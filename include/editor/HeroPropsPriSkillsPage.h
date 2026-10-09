// HeroPropsPriSkillsPage.h - the primary skills page of the hero property
// sheets (GOG HeroPropsPriSkillsPage.cpp, not in Loki): a customize check
// over a TPrimarySkillsDlg child that shows the custom skills, or the
// hero's default ones while uncustomized. Before Shadow of Death a map
// keeps no custom primary skills. Layout from the image: the check at
// 0x8c, the original and edited heroes, the map's version, the modified
// and custom flags, the default and custom skills and the dialog (0x100
// bytes, the sheets' new).
#ifndef HOMM3_EDITOR_HEROPROPSPRISKILLSPAGE_H
#define HOMM3_EDITOR_HEROPROPSPRISKILLSPAGE_H

#include <memory>

#include "gameversion.h"
#include "primaryskill.h"
#include "editor/Array.h"
#include "editor/PrimarySkillsDlg.h"
#include "editor/resource.h"

class THero;

class THeroPropsPriSkillsPage : public CPropertyPage {
public:
    THeroPropsPriSkillsPage(const THero* pOldHero, THero* pNewHero, EGameVersion mapVersion);
    virtual ~THeroPropsPriSkillsPage();

    bool wasModified() const { return _m_bModified; }
    // The skills shown while uncustomized (the hero's identity's).
    void setDefaultPrimarySkills(const TArray<int, kNumPrimarySkills>& skills);

    enum { IDD = IDD_HERO_PROPS_PRI_SKILLS };
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
    bool _m_bCustomSkills;
    TArray<int, kNumPrimarySkills> _m_defaultSkills;
    TArray<int, kNumPrimarySkills> _m_customSkills;
    std::auto_ptr<TPrimarySkillsDlg> _m_pSkillsDlg;
};

#endif  /* HOMM3_EDITOR_HEROPROPSPRISKILLSPAGE_H */
