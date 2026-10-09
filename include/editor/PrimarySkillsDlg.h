// PrimarySkillsDlg.h - the four primary skill edits (GOG
// PrimarySkillsDlg.cpp, not in Loki), a child dialog of the hero primary
// skills page; each skill runs 0..s_kMaxSkill. Layout from the image: the
// dialog, each skill's edit and spin at 0x5c (0x78 bytes each) and the
// skills at 0x23c (0x24c bytes).
#ifndef HOMM3_EDITOR_PRIMARYSKILLSDLG_H
#define HOMM3_EDITOR_PRIMARYSKILLSDLG_H

#include "primaryskill.h"
#include "editor/Array.h"
#include "editor/resource.h"

class TPrimarySkillsDlg : public CDialog {
public:
    enum { s_kMaxSkill = 99 };

    // Creates the dialog as a child of the parent.
    TPrimarySkillsDlg(CWnd* pParent);

    void setPrimarySkills(const TArray<int, kNumPrimarySkills>& skills);
    TArray<int, kNumPrimarySkills> getPrimarySkills() const;

    enum { IDD = IDD_PRIMARY_SKILLS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnEnable(BOOL bEnable);
    afx_msg void OnKillFocusAttackEdit();
    afx_msg void OnKillFocusDefenseEdit();
    afx_msg void OnKillFocusSpellPowerEdit();
    afx_msg void OnKillFocusKnowledgeEdit();
    DECLARE_MESSAGE_MAP()

private:
    // One skill's edit and spin.
    struct _TSkillControls {
        CEdit m_edit;
        CSpinButtonCtrl m_spin;
    };

    void _onKillFocusSkillEdit(unsigned int skill);

    _TSkillControls _m_aSkillControls[kNumPrimarySkills];
    TArray<int, kNumPrimarySkills> _m_skills;
};

#endif  /* HOMM3_EDITOR_PRIMARYSKILLSDLG_H */
