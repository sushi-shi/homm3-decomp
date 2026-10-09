// HeroPropsSecSkillsPage.h - the secondary skills page of the hero
// property sheets (HeroPropsSecSkillsPage.cpp; Loki h3maped object 19): a
// customize check over a TSecondarySkillsDlg child that shows the custom
// skills, or the hero's default ones while uncustomized. Layout from the
// image: the check at 0x8c, the original and edited heroes, the modified
// flag, the default skills, the custom flag, the custom skills and the
// dialog (0x100 bytes, the sheets' new).
#ifndef HOMM3_EDITOR_HEROPROPSSECSKILLSPAGE_H
#define HOMM3_EDITOR_HEROPROPSSECSKILLSPAGE_H

#include <memory>

#include "editor/Hero.h"
#include "editor/resource.h"
#include "editor/SecondarySkillsDlg.h"

class THeroPropsSecSkillsPage : public CPropertyPage {
public:
    THeroPropsSecSkillsPage(const THero* pOldHero, THero* pNewHero);
    virtual ~THeroPropsSecSkillsPage();

    bool wasModified() const { return _m_bModified; }
    // The skills shown while uncustomized (the hero's identity's).
    void setDefaultSecondarySkills(const THeroPrototype::TSecondarySkills& skills);

    enum { IDD = IDD_HERO_PROPS_SEC_SKILLS };
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
    bool _m_bModified;
    THeroPrototype::TSecondarySkills _m_defaultSkills;
    bool _m_bCustomSkills;
    THeroPrototype::TSecondarySkills _m_customSkills;
    std::auto_ptr<TSecondarySkillsDlg> _m_pSkillsDlg;
};

#endif  /* HOMM3_EDITOR_HEROPROPSSECSKILLSPAGE_H */
