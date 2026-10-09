// SecondarySkillsDlg.h - the eight secondary skill slots (GOG
// SecondarySkillsDlg.cpp, not in Loki), a child dialog of the hero
// secondary skills page. Each slot is a skill combo (none, then the skills
// no other slot holds) and a mastery combo, which a slot without a skill
// disables. Layout from the image: the dialog, the slots' controls at 0x5c
// (0x78 bytes each) and the slots' skills and masteries at 0x41c (0x45c
// bytes).
#ifndef HOMM3_EDITOR_SECONDARYSKILLSDLG_H
#define HOMM3_EDITOR_SECONDARYSKILLSDLG_H

#include "editor/Array.h"
#include "editor/Hero.h"
#include "editor/resource.h"

class TSecondarySkillsDlg : public CDialog {
public:
    enum { s_kNumSlots = kNumSecSkillsPerHero };

    // Creates the dialog as a child of the parent.
    TSecondarySkillsDlg(CWnd* pParent);

    void setSecondarySkills(const THeroPrototype::TSecondarySkills& skills);
    THeroPrototype::TSecondarySkills getSecondarySkills() const;

    enum { IDD = IDD_SECONDARY_SKILLS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnEnable(BOOL bEnable);
    afx_msg void OnSelChangeSkillCombo1();
    afx_msg void OnSelChangeSkillCombo2();
    afx_msg void OnSelChangeSkillCombo3();
    afx_msg void OnSelChangeSkillCombo4();
    afx_msg void OnSelChangeSkillCombo5();
    afx_msg void OnSelChangeSkillCombo6();
    afx_msg void OnSelChangeSkillCombo7();
    afx_msg void OnSelChangeSkillCombo8();
    afx_msg void OnSelChangeMasteryCombo1();
    afx_msg void OnSelChangeMasteryCombo2();
    afx_msg void OnSelChangeMasteryCombo3();
    afx_msg void OnSelChangeMasteryCombo4();
    afx_msg void OnSelChangeMasteryCombo5();
    afx_msg void OnSelChangeMasteryCombo6();
    afx_msg void OnSelChangeMasteryCombo7();
    afx_msg void OnSelChangeMasteryCombo8();
    DECLARE_MESSAGE_MAP()

private:
    // One slot's combos.
    struct _TSlotControls {
        CComboBox m_skillCombo;
        CComboBox m_masteryCombo;
    };

    // One slot's skill (none: -1) and mastery.
    struct _TSlot {
        _TSlot() : m_skill(-1), m_mastery(eMasteryNone) {}
        _TSlot(int skill, TSkillMastery mastery) : m_skill(skill), m_mastery(mastery) {}

        int m_skill;
        TSkillMastery m_mastery;
    };

    void _onSelChangeSkillCombo(unsigned int slot);
    void _onSelChangeMasteryCombo(unsigned int slot);
    void _setSlot(unsigned int slot, const _TSlot& newSlot);
    int _findSkillIndex(unsigned int slot, int skill);

    TArray<_TSlotControls, s_kNumSlots> _m_aSlotControls;
    TArray<_TSlot, s_kNumSlots> _m_aSlots;
};

#endif  /* HOMM3_EDITOR_SECONDARYSKILLSDLG_H */
