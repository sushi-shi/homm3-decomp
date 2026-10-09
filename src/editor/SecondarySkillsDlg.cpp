// SecondarySkillsDlg.cpp - the eight secondary skill slots (h3maped
// 0x4b5519..0x4b6002; GOG only). Choosing a skill in a slot takes it out
// of the other slots' combos and gives the slot basic mastery; clearing it
// puts the old skill back in order and clears the mastery.
#include "editor/stdafx.h"

#include <algorithm>

#include "exceptions.h"
#include "va.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"
#include "editor/SecondarySkillsDlg.h"

DATA(0x00541ca0)
static const int g_aakSlotControlIDs[TSecondarySkillsDlg::s_kNumSlots][2] = {
    { IDC_SKILL_COMBO1, IDC_MASTERY_COMBO1 },
    { IDC_SKILL_COMBO2, IDC_MASTERY_COMBO2 },
    { IDC_SKILL_COMBO3, IDC_MASTERY_COMBO3 },
    { IDC_SKILL_COMBO4, IDC_MASTERY_COMBO4 },
    { IDC_SKILL_COMBO5, IDC_MASTERY_COMBO5 },
    { IDC_SKILL_COMBO6, IDC_MASTERY_COMBO6 },
    { IDC_SKILL_COMBO7, IDC_MASTERY_COMBO7 },
    { IDC_SKILL_COMBO8, IDC_MASTERY_COMBO8 }
};

VA(0x004b5535, 0x8)
void TSecondarySkillsDlg::OnSelChangeSkillCombo1()
{
    _onSelChangeSkillCombo(0);
}

VA(0x004b553d, 0x8)
void TSecondarySkillsDlg::OnSelChangeSkillCombo2()
{
    _onSelChangeSkillCombo(1);
}

VA(0x004b5545, 0x8)
void TSecondarySkillsDlg::OnSelChangeSkillCombo3()
{
    _onSelChangeSkillCombo(2);
}

VA(0x004b554d, 0x8)
void TSecondarySkillsDlg::OnSelChangeSkillCombo4()
{
    _onSelChangeSkillCombo(3);
}

VA(0x004b5555, 0x8)
void TSecondarySkillsDlg::OnSelChangeSkillCombo5()
{
    _onSelChangeSkillCombo(4);
}

VA(0x004b555d, 0x8)
void TSecondarySkillsDlg::OnSelChangeSkillCombo6()
{
    _onSelChangeSkillCombo(5);
}

VA(0x004b5565, 0x8)
void TSecondarySkillsDlg::OnSelChangeSkillCombo7()
{
    _onSelChangeSkillCombo(6);
}

VA(0x004b556d, 0x8)
void TSecondarySkillsDlg::OnSelChangeSkillCombo8()
{
    _onSelChangeSkillCombo(7);
}

VA(0x004b5575, 0x8)
void TSecondarySkillsDlg::OnSelChangeMasteryCombo1()
{
    _onSelChangeMasteryCombo(0);
}

VA(0x004b557d, 0x8)
void TSecondarySkillsDlg::OnSelChangeMasteryCombo2()
{
    _onSelChangeMasteryCombo(1);
}

VA(0x004b5585, 0x8)
void TSecondarySkillsDlg::OnSelChangeMasteryCombo3()
{
    _onSelChangeMasteryCombo(2);
}

VA(0x004b558d, 0x8)
void TSecondarySkillsDlg::OnSelChangeMasteryCombo4()
{
    _onSelChangeMasteryCombo(3);
}

VA(0x004b5595, 0x8)
void TSecondarySkillsDlg::OnSelChangeMasteryCombo5()
{
    _onSelChangeMasteryCombo(4);
}

VA(0x004b559d, 0x8)
void TSecondarySkillsDlg::OnSelChangeMasteryCombo6()
{
    _onSelChangeMasteryCombo(5);
}

VA(0x004b55a5, 0x8)
void TSecondarySkillsDlg::OnSelChangeMasteryCombo7()
{
    _onSelChangeMasteryCombo(6);
}

VA(0x004b55ad, 0x8)
void TSecondarySkillsDlg::OnSelChangeMasteryCombo8()
{
    _onSelChangeMasteryCombo(7);
}

VA(0x004b55b5, 0x7e)
TSecondarySkillsDlg::TSecondarySkillsDlg(CWnd* pParent)
    : CDialog(TSecondarySkillsDlg::IDD, pParent)
{
    if (!Create(TSecondarySkillsDlg::IDD, pParent))
        throw TRuntimeError();
}

VA_COMPGEN(0x004b5633, 0x1c, SCALAR_DELETING_DTOR, TSecondarySkillsDlg)
VA_COMPGEN(0x004b564f, 0x3f, IMPLICIT_DTOR, TSecondarySkillsDlg)

VA(0x004b568e, 0x93)
void TSecondarySkillsDlg::setSecondarySkills(const THeroPrototype::TSecondarySkills& skills)
{
    unsigned int slot;
    for (slot = 0; slot < s_kNumSlots; slot++) {
        if (_m_aSlots[slot].m_skill != -1) {
            _m_aSlotControls[slot].m_skillCombo.SetCurSel(_findSkillIndex(slot, -1));
            _onSelChangeSkillCombo(slot);
        }
    }
    slot = 0;
    for (THeroPrototype::TSecondarySkills::const_iterator it = skills.begin(); it != skills.end(); ++it)
        _setSlot(slot++, _TSlot(it->first, it->second));
}

VA(0x004b5721, 0xaf)
THeroPrototype::TSecondarySkills TSecondarySkillsDlg::getSecondarySkills() const
{
    THeroPrototype::TSecondarySkills skills;
    for (unsigned int slot = 0; slot < s_kNumSlots; slot++) {
        if (_m_aSlots[slot].m_skill != -1)
            skills.insert(THeroPrototype::TSecondarySkills::value_type(TSecondarySkill(_m_aSlots[slot].m_skill),
                                                                        _m_aSlots[slot].m_mastery));
    }
    return skills;
}

VA(0x004b57d0, 0x1b1)
void TSecondarySkillsDlg::_onSelChangeSkillCombo(unsigned int slot)
{
    int oldSkill = _m_aSlots[slot].m_skill;
    _TSlotControls& controls = _m_aSlotControls[slot];
    CComboBox& masteryCombo = controls.m_masteryCombo;
    int newSkill = controls.m_skillCombo.GetItemData(controls.m_skillCombo.GetCurSel());
    if (newSkill == oldSkill)
        return;
    _m_aSlots[slot].m_skill = newSkill;
    unsigned int otherSlot;
    if (oldSkill == -1) {
        if (IsWindowEnabled())
            masteryCombo.EnableWindow(TRUE);
        int index = 0;
        while (masteryCombo.GetItemData(index) != eMasteryBasic)
            index++;
        masteryCombo.SetCurSel(index);
        _m_aSlots[slot].m_mastery = eMasteryBasic;
    } else {
        for (otherSlot = 0; otherSlot < s_kNumSlots; otherSlot++) {
            if (otherSlot != slot) {
                CComboBox& skillCombo = _m_aSlotControls[otherSlot].m_skillCombo;
                int count = skillCombo.GetCount();
                int index;
                for (index = 0; index < count && int(skillCombo.GetItemData(index)) <= oldSkill; index++)
                    ;
                skillCombo.InsertString(index, THero::s_akSecondarySkillTraits[oldSkill].m_name);
                skillCombo.SetItemData(index, oldSkill);
            }
        }
    }
    if (newSkill == -1) {
        _m_aSlots[slot].m_mastery = eMasteryNone;
        masteryCombo.SetCurSel(-1);
        masteryCombo.EnableWindow(FALSE);
    } else {
        for (otherSlot = 0; otherSlot < s_kNumSlots; otherSlot++) {
            if (otherSlot != slot)
                _m_aSlotControls[otherSlot].m_skillCombo.DeleteString(_findSkillIndex(otherSlot, newSkill));
        }
    }
}

VA(0x004b5981, 0x45)
void TSecondarySkillsDlg::_onSelChangeMasteryCombo(unsigned int slot)
{
    CComboBox& masteryCombo = _m_aSlotControls[slot].m_masteryCombo;
    _m_aSlots[slot].m_mastery = TSkillMastery(masteryCombo.GetItemData(masteryCombo.GetCurSel()));
}

VA(0x004b59c6, 0x9f)
void TSecondarySkillsDlg::_setSlot(unsigned int slot, const _TSlot& newSlot)
{
    if (newSlot.m_skill != _m_aSlots[slot].m_skill) {
        _m_aSlotControls[slot].m_skillCombo.SetCurSel(_findSkillIndex(slot, newSlot.m_skill));
        _onSelChangeSkillCombo(slot);
    }
    if (newSlot.m_mastery != _m_aSlots[slot].m_mastery) {
        CComboBox& masteryCombo = _m_aSlotControls[slot].m_masteryCombo;
        int index = 0;
        while (masteryCombo.GetItemData(index) != DWORD(newSlot.m_mastery))
            index++;
        masteryCombo.SetCurSel(index);
        _onSelChangeMasteryCombo(slot);
    }
}

VA(0x004b5a65, 0x3d)
int TSecondarySkillsDlg::_findSkillIndex(unsigned int slot, int skill)
{
    CComboBox& skillCombo = _m_aSlotControls[slot].m_skillCombo;
    int index = 0;
    while (int(skillCombo.GetItemData(index)) != skill)
        index++;
    return index;
}

VA(0x004b5aa2, 0x3c)
void TSecondarySkillsDlg::DoDataExchange(CDataExchange* pDX)
{
    for (unsigned int slot = 0; slot < s_kNumSlots; slot++) {
        DDX_Control(pDX, g_aakSlotControlIDs[slot][0], _m_aSlotControls[slot].m_skillCombo);
        DDX_Control(pDX, g_aakSlotControlIDs[slot][1], _m_aSlotControls[slot].m_masteryCombo);
    }
}

VA(0x004b5ade, 0x6)
BEGIN_MESSAGE_MAP(TSecondarySkillsDlg, CDialog)
    ON_WM_ENABLE()
    ON_CBN_SELCHANGE(IDC_SKILL_COMBO1, OnSelChangeSkillCombo1)
    ON_CBN_SELCHANGE(IDC_SKILL_COMBO2, OnSelChangeSkillCombo2)
    ON_CBN_SELCHANGE(IDC_SKILL_COMBO3, OnSelChangeSkillCombo3)
    ON_CBN_SELCHANGE(IDC_SKILL_COMBO4, OnSelChangeSkillCombo4)
    ON_CBN_SELCHANGE(IDC_SKILL_COMBO5, OnSelChangeSkillCombo5)
    ON_CBN_SELCHANGE(IDC_SKILL_COMBO6, OnSelChangeSkillCombo6)
    ON_CBN_SELCHANGE(IDC_SKILL_COMBO7, OnSelChangeSkillCombo7)
    ON_CBN_SELCHANGE(IDC_SKILL_COMBO8, OnSelChangeSkillCombo8)
    ON_CBN_SELCHANGE(IDC_MASTERY_COMBO1, OnSelChangeMasteryCombo1)
    ON_CBN_SELCHANGE(IDC_MASTERY_COMBO2, OnSelChangeMasteryCombo2)
    ON_CBN_SELCHANGE(IDC_MASTERY_COMBO3, OnSelChangeMasteryCombo3)
    ON_CBN_SELCHANGE(IDC_MASTERY_COMBO4, OnSelChangeMasteryCombo4)
    ON_CBN_SELCHANGE(IDC_MASTERY_COMBO5, OnSelChangeMasteryCombo5)
    ON_CBN_SELCHANGE(IDC_MASTERY_COMBO6, OnSelChangeMasteryCombo6)
    ON_CBN_SELCHANGE(IDC_MASTERY_COMBO7, OnSelChangeMasteryCombo7)
    ON_CBN_SELCHANGE(IDC_MASTERY_COMBO8, OnSelChangeMasteryCombo8)
END_MESSAGE_MAP()

VA(0x004b5ae4, 0x347)
BOOL TSecondarySkillsDlg::OnInitDialog()
{
    GetDlgItem(IDC_TYPE_STATIC)->SetWindowText(SHeroPropsSecSkillsPageText::kTypeStaticStr);
    GetDlgItem(IDC_MASTERY_STATIC)->SetWindowText(SHeroPropsSecSkillsPageText::kMasteryStaticStr);
    GetDlgItem(IDC_SLOT1_SKILL_STATIC)->SetWindowText(TFormattedString(SHeroPropsSecSkillsPageText::kSkillStaticFmtStr, 1));
    GetDlgItem(IDC_SLOT2_SKILL_STATIC)->SetWindowText(TFormattedString(SHeroPropsSecSkillsPageText::kSkillStaticFmtStr, 2));
    GetDlgItem(IDC_SLOT3_SKILL_STATIC)->SetWindowText(TFormattedString(SHeroPropsSecSkillsPageText::kSkillStaticFmtStr, 3));
    GetDlgItem(IDC_SLOT4_SKILL_STATIC)->SetWindowText(TFormattedString(SHeroPropsSecSkillsPageText::kSkillStaticFmtStr, 4));
    GetDlgItem(IDC_SLOT5_SKILL_STATIC)->SetWindowText(TFormattedString(SHeroPropsSecSkillsPageText::kSkillStaticFmtStr, 5));
    GetDlgItem(IDC_SLOT6_SKILL_STATIC)->SetWindowText(TFormattedString(SHeroPropsSecSkillsPageText::kSkillStaticFmtStr, 6));
    GetDlgItem(IDC_SLOT7_SKILL_STATIC)->SetWindowText(TFormattedString(SHeroPropsSecSkillsPageText::kSkillStaticFmtStr, 7));
    GetDlgItem(IDC_SLOT8_SKILL_STATIC)->SetWindowText(TFormattedString(SHeroPropsSecSkillsPageText::kSkillStaticFmtStr, 8));
    std::fill(_m_aSlots.begin(), _m_aSlots.end(), _TSlot());
    CDialog::OnInitDialog();
    for (unsigned int slot = 0; slot < s_kNumSlots; slot++) {
        _TSlotControls& controls = _m_aSlotControls[slot];
        int index = controls.m_skillCombo.AddString(kSelNoneStr);
        controls.m_skillCombo.SetItemData(index, -1);
        for (int skill = 0; skill < kNumSecSkills; skill++) {
            index = controls.m_skillCombo.AddString(THero::s_akSecondarySkillTraits[skill].m_name);
            controls.m_skillCombo.SetItemData(index, skill);
        }
        for (int mastery = eMasteryBasic; mastery < kNumMasteries; mastery++) {
            index = controls.m_masteryCombo.AddString(THero::s_akSkillMasteryTraits[mastery - eMasteryBasic].m_name);
            controls.m_masteryCombo.SetItemData(index, mastery);
        }
        controls.m_skillCombo.SetCurSel(_findSkillIndex(slot, -1));
        controls.m_masteryCombo.SetCurSel(-1);
        controls.m_masteryCombo.EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x004b5e2b, 0x6a)
void TSecondarySkillsDlg::OnEnable(BOOL bEnable)
{
    CDialog::OnEnable(bEnable);
    unsigned int slot;
    if (bEnable) {
        for (slot = 0; slot < s_kNumSlots; slot++) {
            _m_aSlotControls[slot].m_skillCombo.EnableWindow(TRUE);
            if (_m_aSlots[slot].m_skill != -1)
                _m_aSlotControls[slot].m_masteryCombo.EnableWindow(TRUE);
        }
    } else {
        for (slot = 0; slot < s_kNumSlots; slot++) {
            _m_aSlotControls[slot].m_skillCombo.EnableWindow(FALSE);
            _m_aSlotControls[slot].m_masteryCombo.EnableWindow(FALSE);
        }
    }
}

VA_COMPGEN(0x004b5f8d, 0x40, CLASS_CTOR, _TSlotControls)
VA_COMPGEN(0x004b5fcd, 0x35, IMPLICIT_DTOR, _TSlotControls)
