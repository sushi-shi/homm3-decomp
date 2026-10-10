// ScholarPropsDlg.h - the scholar's reward dialog (ScholarPropsDlg.cpp;
// GOG only, Loki's port has no counterpart): a random reward, a primary
// skill, a secondary skill or a spell, each kind with its combo. Layout
// from the image: the spell combo at 0x5c, the secondary skill combo at
// 0x98, the primary skill combo at 0xd4, the DDX radio index (the reward
// type plus one) at 0x110, the scholar at 0x114, the modified flag at
// 0x118, then the primary skill, secondary skill and spell.
#ifndef HOMM3_EDITOR_SCHOLARPROPSDLG_H
#define HOMM3_EDITOR_SCHOLARPROPSDLG_H

#include "armygrp.h"
#include "primaryskill.h"
#include "secondaryskill.h"
#include "editor/resource.h"

class TScholar;

class TScholarPropsDlg : public CDialog {
public:
    TScholarPropsDlg(CWnd* pParent, TScholar* pScholar);

    virtual int DoModal();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_SCHOLAR_PROPS };
    CComboBox _m_spellCombo;
    CComboBox _m_secondarySkillCombo;
    CComboBox _m_primarySkillCombo;
    int _m_rewardChoice;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnRewardRadio();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

private:
    TScholar* _m_pScholar;
    bool _m_bModified;
    TPrimarySkill _m_primarySkill;
    TSecondarySkill _m_secondarySkill;
    ESpellId _m_spell;
};

#endif  /* HOMM3_EDITOR_SCHOLARPROPSDLG_H */
