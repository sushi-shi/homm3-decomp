// HeroPropsSecSkillsPage.h - the hero sheet's secondary skills page (Loki
// HeroPropsSecSkillsPage.cpp): eight slots of a skill combo and a mastery
// combo, filled from the hero's custom skills or its prototype's. A skill
// chosen in one slot leaves the other slots' lists. Layout from the image:
// the eight mastery selections (int members, -1 for none; the slot table
// points at them by pointer to member), the customize toggle state, the
// hero, the prototype, its class and number, _m_aSlotSkill, the edited
// skill array, the custom flag, the custom skills, the modified flag, then
// the vtable pointer (OnOK, OnInitDialog). Proven names: _m_aSlotSkill,
// _s_akSlotData and its skillItemData, _s_kNumSkillSlots, _TSkillArray,
// TSkillData and its m_skill; the others are not.
#ifndef HOMM3_EDITOR_HEROPROPSSECSKILLSPAGE_H
#define HOMM3_EDITOR_HEROPROPSSECSKILLSPAGE_H

#include "editor/stdafx.h"

#include <map>

#include "editor/Array.h"
#include "editor/Hero.h"

class THeroPropsSecSkillsPage {
public:
    THeroPropsSecSkillsPage(const THero& hero);
    ~THeroPropsSecSkillsPage();

    void setIdentity(THeroClass newHeroClass, unsigned int newProtoNum);

    virtual void OnOK();
    virtual BOOL OnInitDialog();
    void UpdateData(bool bSaveAndValidate);
    void OnCustomizeCheck();
    void OnSelChangeSkill1Combo();
    void OnSelChangeSkill2Combo();
    void OnSelChangeSkill3Combo();
    void OnSelChangeSkill4Combo();
    void OnSelChangeSkill5Combo();
    void OnSelChangeSkill6Combo();
    void OnSelChangeSkill7Combo();
    void OnSelChangeSkill8Combo();

    bool getBCustomSecondarySkills() const { return _m_bCustomSecondarySkills; }
    const map<TSecondarySkill, TSkillMastery>& getSecondarySkills() const { return _m_secondarySkills; }
    bool wasModified() const { return _m_bModified; }

private:
    static const int _s_kNumSkillSlots = 8;

    struct TSkillData {
        TSkillData() {}
        TSkillData(TSecondarySkill skill, TSkillMastery mastery) : m_skill(skill), m_mastery(mastery) {}

        TSecondarySkill m_skill;
        TSkillMastery m_mastery;
    };

    class _TSkillArray : public TArray<TSkillData, _s_kNumSkillSlots> {
    public:
        _TSkillArray();
        _TSkillArray(const map<TSecondarySkill, TSkillMastery>& skillSet);
        operator map<TSecondarySkill, TSkillMastery>() const;
    };

    // One slot's combos, its mastery selection and its skill combo's row
    // to skill table.
    struct TSlotData {
        GtkCombo* skillCombo;
        GtkCombo* masteryCombo;
        int THeroPropsSecSkillsPage::* pMastery;
        int skillItemData[30];
    };

    static TSlotData _s_akSlotData[_s_kNumSkillSlots];

    void _setSlotControlData(const _TSkillArray& skills);
    void _retrieveSlotControlData(_TSkillArray* pSkills);
    void _enableSlotControls();
    void _disableSlotControls();
    void _setSlotSkill(unsigned int slot, TSecondarySkill newSkill);
    void _onSelChangeSkillCombo(unsigned int slot);

    GtkCombo* _getPSkillCombo(unsigned int slot) { return _s_akSlotData[slot].skillCombo; }
    GtkCombo* _getPMasteryCombo(unsigned int slot) { return _s_akSlotData[slot].masteryCombo; }
    int* _getPMastery(unsigned int slot) { return &(this->*_s_akSlotData[slot].pMastery); }

    int _m_mastery0;
    int _m_mastery1;
    int _m_mastery2;
    int _m_mastery3;
    int _m_mastery4;
    int _m_mastery5;
    int _m_mastery6;
    int _m_mastery7;
    ubyte _m_bCustomize;
    const THero& _m_hero;
    const THeroPrototype* _m_pPrototype;
    THeroClass _m_heroClass;
    unsigned int _m_protoNum;
    TSecondarySkill _m_aSlotSkill[_s_kNumSkillSlots];
    _TSkillArray _m_skillArray;
    bool _m_bCustomSecondarySkills;
    map<TSecondarySkill, TSkillMastery> _m_secondarySkills;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_HEROPROPSSECSKILLSPAGE_H */
