// EditRewardDlg.cpp - the reward editor (h3maped 0x412cfa..0x41576b; GOG
// only): a radio per reward kind and the kind's page. The bonus pages
// clamp their edits to the reward's range, the morale and luck pages take
// a radio per bonus, and the lists offer the map version's artifacts and
// creatures, the spells a scroll may hold and every resource and skill.
#include "editor/stdafx.h"

#include <stdio.h>

#include <memory>

#include "artifact.h"
#include "exceptions.h"
#include "va.h"
#include "editor/Army.h"
#include "editor/Clamp.h"
#include "editor/CreatureCombo.h"
#include "editor/DialogTemplate.h"
#include "editor/Digits.h"
#include "editor/EditRewardDlg.h"
#include "editor/FormattedString.h"
#include "editor/GameResource.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"

namespace {

// A page of the reward editor: it builds the reward of its kind.
class TRewardTypeDlg : public CDialog {
public:
    TRewardTypeDlg() {}

    virtual void OnOK() { CDialog::OnOK(); }
    // Creates the page as a child of pParent.
    virtual BOOL create(CWnd* pParent) = 0;
    virtual std::auto_ptr<TSeersHut::TReward> getReward() const = 0;
};

// No reward: an empty page.
class TRewardNoneDlg : public TRewardTypeDlg {
public:
    virtual BOOL create(CWnd* pParent)
    {
        TEmptyDialogTemplate dialogTemplate;
        return CreateIndirect(&dialogTemplate, pParent);
    }
    virtual std::auto_ptr<TSeersHut::TReward> getReward() const
    {
        return std::auto_ptr<TSeersHut::TReward>(NULL);
    }
};

// Experience, 1..s_kMaxBonus.
class TRewardExperienceDlg : public TRewardTypeDlg {
public:
    enum { IDD = IDD_REWARD_EXPERIENCE, s_kMaxBonus = 99999999 };

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TSeersHut::TReward> getReward() const;
    void setReward(const TSeersHut::TRewardExperience& reward);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnKillFocusBonusEdit();
    DECLARE_MESSAGE_MAP()

private:
    CEdit _m_bonusEdit;
    CSpinButtonCtrl _m_bonusSpin;
    int _m_bonus;
};

// Spell points, 1..s_kMaxBonus.
class TRewardManaDlg : public TRewardTypeDlg {
public:
    enum { IDD = IDD_REWARD_MANA, s_kMaxBonus = 999 };

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TSeersHut::TReward> getReward() const;
    void setReward(const TSeersHut::TRewardMana& reward);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnKillFocusBonusEdit();
    DECLARE_MESSAGE_MAP()

private:
    CEdit _m_bonusEdit;
    CSpinButtonCtrl _m_bonusSpin;
    int _m_bonus;
};

enum { kNumMoraleBonuses = 3, kNumLuckBonuses = 3 };

// Morale, +1..+3: a radio per bonus.
class TRewardMoraleDlg : public TRewardTypeDlg {
public:
    enum { IDD = IDD_REWARD_MORALE };

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TSeersHut::TReward> getReward() const;
    void setReward(const TSeersHut::TRewardMorale& reward);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    int _m_bonusIndex;
    int _m_bonus;
};

// Luck, +1..+3: a radio per bonus.
class TRewardLuckDlg : public TRewardTypeDlg {
public:
    enum { IDD = IDD_REWARD_LUCK };

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TSeersHut::TReward> getReward() const;
    void setReward(const TSeersHut::TRewardLuck& reward);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    int _m_bonusIndex;
    int _m_bonus;
};

// A resource and its quantity, 1..TRewardResource::s_kMaxQuantity.
class TRewardResourceDlg : public TRewardTypeDlg {
public:
    enum { IDD = IDD_REWARD_RESOURCE };

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TSeersHut::TReward> getReward() const;
    void setReward(const TSeersHut::TRewardResource& reward);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnKillFocusQuantityEdit();
    DECLARE_MESSAGE_MAP()

private:
    CComboBox _m_typeCombo;
    CEdit _m_quantityEdit;
    CSpinButtonCtrl _m_quantitySpin;
    TGameResourceType _m_type;
    unsigned int _m_quantity;
};

// A primary skill and its bonus, 1..s_kMaxBonus.
class TRewardPrimarySkillDlg : public TRewardTypeDlg {
public:
    enum { IDD = IDD_REWARD_PRIMARY_SKILL, s_kMaxBonus = 99 };

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TSeersHut::TReward> getReward() const;
    void setReward(const TSeersHut::TRewardPrimarySkill& reward);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnKillFocusBonusEdit();
    DECLARE_MESSAGE_MAP()

private:
    CComboBox _m_skillCombo;
    CEdit _m_bonusEdit;
    CSpinButtonCtrl _m_bonusSpin;
    TPrimarySkill _m_skill;
    int _m_bonus;
};

// A secondary skill and its mastery.
class TRewardSecondarySkillDlg : public TRewardTypeDlg {
public:
    enum { IDD = IDD_REWARD_SECONDARY_SKILL };

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TSeersHut::TReward> getReward() const;
    void setReward(const TSeersHut::TRewardSecondarySkill& reward);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    CComboBox _m_skillCombo;
    CComboBox _m_masteryCombo;
    TSecondarySkill _m_skill;
    TSkillMastery _m_mastery;
};

// An artifact of the map's version.
class TRewardArtifactDlg : public TRewardTypeDlg {
public:
    enum { IDD = IDD_REWARD_ARTIFACT };

    TRewardArtifactDlg(EGameVersion mapVersion);

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TSeersHut::TReward> getReward() const;
    void setReward(const TSeersHut::TRewardArtifact& reward);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    EGameVersion _m_mapVersion;
    CComboBox _m_artifactCombo;
    TArtifact _m_artifact;
};

// A spell a scroll may hold.
class TRewardSpellDlg : public TRewardTypeDlg {
public:
    enum { IDD = IDD_REWARD_SPELL };

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TSeersHut::TReward> getReward() const;
    void setReward(const TSeersHut::TRewardSpell& reward);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    CComboBox _m_spellCombo;
    SpellID _m_spell;
};

// A creature stack of the map's version, 1..TCreatureStack::s_kMaxQuantity.
class TRewardCreatureDlg : public TRewardTypeDlg {
public:
    enum { IDD = IDD_REWARD_CREATURE };

    TRewardCreatureDlg(EGameVersion mapVersion);

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TSeersHut::TReward> getReward() const;
    void setReward(const TSeersHut::TRewardCreature& reward);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnKillFocusQuantityEdit();
    DECLARE_MESSAGE_MAP()

private:
    EGameVersion _m_mapVersion;
    CComboBox _m_creatureCombo;
    CEdit _m_quantityEdit;
    CSpinButtonCtrl _m_quantitySpin;
    TCreatureStack _m_creatureStack;
};

}

// The editor's pages, allocated together, one per reward kind in radio
// order.
struct TEditRewardDlg::_TPages {
    _TPages(EGameVersion mapVersion);

    TRewardNoneDlg m_noneDlg;
    TRewardExperienceDlg m_experienceDlg;
    TRewardManaDlg m_manaDlg;
    TRewardMoraleDlg m_moraleDlg;
    TRewardLuckDlg m_luckDlg;
    TRewardResourceDlg m_resourceDlg;
    TRewardPrimarySkillDlg m_primarySkillDlg;
    TRewardSecondarySkillDlg m_secondarySkillDlg;
    TRewardArtifactDlg m_artifactDlg;
    TRewardSpellDlg m_spellDlg;
    TRewardCreatureDlg m_creatureDlg;
    TRewardTypeDlg* m_apDlg[s_kNumRewardTypes];
};

VA(0x00412ece, 0x6)
BEGIN_MESSAGE_MAP(TRewardExperienceDlg, TRewardTypeDlg)
    ON_EN_KILLFOCUS(IDC_EXPERIENCE_BONUS_EDIT, OnKillFocusBonusEdit)
END_MESSAGE_MAP()

VA(0x00412ed4, 0x4d)
void TRewardExperienceDlg::setReward(const TSeersHut::TRewardExperience& reward)
{
    _m_bonusEdit.SetWindowText(TFormattedString("%d", reward.getBonus()));
}

VA(0x00412f21, 0x81)
std::auto_ptr<TSeersHut::TReward> TRewardExperienceDlg::getReward() const
{
    std::auto_ptr<TSeersHut::TReward> pReward(new TSeersHut::TRewardExperience(_m_bonus));
    if (!pReward.get())
        throw TAllocationFailure();
    return pReward;
}

VA(0x00412fa2, 0x2e)
void TRewardExperienceDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_EXPERIENCE_BONUS_EDIT, _m_bonusEdit);
    DDX_Control(pDX, IDC_EXPERIENCE_BONUS_SPIN, _m_bonusSpin);
}

VA(0x00412fd0, 0x6c)
BOOL TRewardExperienceDlg::OnInitDialog()
{
    GetDlgItem(IDC_REWARD_EXPERIENCE_STATIC)->SetWindowText(SEditRewardDlgText::kExperienceBonusStaticStr);
    _m_bonus = 1;
    CDialog::OnInitDialog();
    _m_bonusEdit.SetWindowText("1");
    _m_bonusEdit.LimitText(TDigits<s_kMaxBonus>::getDigits());
    _m_bonusSpin.SetRange(1, UD_MAXVAL);
    return TRUE;
}

VA(0x0041303c, 0x8c)
void TRewardExperienceDlg::OnOK()
{
    CDialog::OnOK();
    _m_bonus = 0;
    CString text;
    _m_bonusEdit.GetWindowText(text);
    sscanf(text, "%d", &_m_bonus);
    _m_bonus = clamp(1, _m_bonus, int(s_kMaxBonus));
}

VA(0x004130c8, 0xa5)
void TRewardExperienceDlg::OnKillFocusBonusEdit()
{
    int bonus = 0;
    CString text;
    _m_bonusEdit.GetWindowText(text);
    sscanf(text, "%d", &bonus);
    if (bonus < 1 || bonus >= s_kMaxBonus) {
        bonus = clamp(1, bonus, int(s_kMaxBonus));
        text.Format("%d", bonus);
        _m_bonusEdit.SetWindowText(text);
    }
}

VA(0x0041316d, 0x6)
BEGIN_MESSAGE_MAP(TRewardManaDlg, TRewardTypeDlg)
    ON_EN_KILLFOCUS(IDC_MANA_BONUS_EDIT, OnKillFocusBonusEdit)
END_MESSAGE_MAP()

VA(0x00413173, 0x4d)
void TRewardManaDlg::setReward(const TSeersHut::TRewardMana& reward)
{
    _m_bonusEdit.SetWindowText(TFormattedString("%d", reward.getBonus()));
}

VA(0x004131c0, 0x81)
std::auto_ptr<TSeersHut::TReward> TRewardManaDlg::getReward() const
{
    std::auto_ptr<TSeersHut::TReward> pReward(new TSeersHut::TRewardMana(_m_bonus));
    if (!pReward.get())
        throw TAllocationFailure();
    return pReward;
}

VA(0x00413241, 0x2e)
void TRewardManaDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_MANA_BONUS_EDIT, _m_bonusEdit);
    DDX_Control(pDX, IDC_MANA_BONUS_SPIN, _m_bonusSpin);
}

VA(0x0041326f, 0x62)
BOOL TRewardManaDlg::OnInitDialog()
{
    GetDlgItem(IDC_REWARD_MANA_STATIC)->SetWindowText(SEditRewardDlgText::kManaBonusStaticStr);
    CDialog::OnInitDialog();
    _m_bonusEdit.SetWindowText("1");
    _m_bonusEdit.LimitText(TDigits<s_kMaxBonus>::getDigits());
    _m_bonusSpin.SetRange(1, s_kMaxBonus);
    return TRUE;
}

VA(0x004132d1, 0x8c)
void TRewardManaDlg::OnOK()
{
    CDialog::OnOK();
    _m_bonus = 0;
    CString text;
    _m_bonusEdit.GetWindowText(text);
    sscanf(text, "%d", &_m_bonus);
    _m_bonus = clamp(1, _m_bonus, int(s_kMaxBonus));
}

VA(0x0041335d, 0xa5)
void TRewardManaDlg::OnKillFocusBonusEdit()
{
    int bonus = 0;
    CString text;
    _m_bonusEdit.GetWindowText(text);
    sscanf(text, "%d", &bonus);
    if (bonus < 1 || bonus >= s_kMaxBonus) {
        bonus = clamp(1, bonus, int(s_kMaxBonus));
        text.Format("%d", bonus);
        _m_bonusEdit.SetWindowText(text);
    }
}

// The bonus of each radio, in radio order.
DATA(0x005331b0)
static const int g_akMoraleBonus[kNumMoraleBonuses] = { 1, 2, 3 };

DATA(0x005331bc)
static const int g_akLuckBonus[kNumLuckBonuses] = { 1, 2, 3 };

VA(0x00413402, 0x2b)
void TRewardMoraleDlg::setReward(const TSeersHut::TRewardMorale& reward)
{
    for (_m_bonusIndex = 0; g_akMoraleBonus[_m_bonusIndex] != reward.getBonus(); _m_bonusIndex++)
        ;
    UpdateData(FALSE);
}

VA(0x0041342d, 0x7e)
std::auto_ptr<TSeersHut::TReward> TRewardMoraleDlg::getReward() const
{
    std::auto_ptr<TSeersHut::TReward> pReward(new TSeersHut::TRewardMorale(_m_bonus));
    if (!pReward.get())
        throw TAllocationFailure();
    return pReward;
}

// Folded by /OPT:ICF onto TRewardLuckDlg::DoDataExchange.
void TRewardMoraleDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Radio(pDX, IDC_PLUS_ONE_RADIO, _m_bonusIndex);
}

VA(0x004134ab, 0x2b)
BOOL TRewardMoraleDlg::OnInitDialog()
{
    GetDlgItem(IDC_REWARD_MORALE_STATIC)->SetWindowText(SEditRewardDlgText::kMoraleBonusStaticStr);
    _m_bonusIndex = 0;
    CDialog::OnInitDialog();
    return TRUE;
}

VA(0x004134d6, 0x17)
void TRewardMoraleDlg::OnOK()
{
    CDialog::OnOK();
    _m_bonus = g_akMoraleBonus[_m_bonusIndex];
}

VA(0x004134ed, 0x2b)
void TRewardLuckDlg::setReward(const TSeersHut::TRewardLuck& reward)
{
    for (_m_bonusIndex = 0; g_akLuckBonus[_m_bonusIndex] != reward.getBonus(); _m_bonusIndex++)
        ;
    UpdateData(FALSE);
}

VA(0x00413518, 0x7e)
std::auto_ptr<TSeersHut::TReward> TRewardLuckDlg::getReward() const
{
    std::auto_ptr<TSeersHut::TReward> pReward(new TSeersHut::TRewardLuck(_m_bonus));
    if (!pReward.get())
        throw TAllocationFailure();
    return pReward;
}

VA(0x00413596, 0x15)
void TRewardLuckDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Radio(pDX, IDC_PLUS_ONE_RADIO, _m_bonusIndex);
}

VA(0x004135ab, 0x2b)
BOOL TRewardLuckDlg::OnInitDialog()
{
    GetDlgItem(IDC_REWARD_LUCK_STATIC)->SetWindowText(SEditRewardDlgText::kLuckBonusStaticStr);
    _m_bonusIndex = 0;
    CDialog::OnInitDialog();
    return TRUE;
}

VA(0x004135d6, 0x17)
void TRewardLuckDlg::OnOK()
{
    CDialog::OnOK();
    _m_bonus = g_akLuckBonus[_m_bonusIndex];
}

VA(0x004135ed, 0x6)
BEGIN_MESSAGE_MAP(TRewardResourceDlg, TRewardTypeDlg)
    ON_EN_KILLFOCUS(IDC_REWARD_QUANTITY_EDIT, OnKillFocusQuantityEdit)
END_MESSAGE_MAP()

VA(0x004135f3, 0x9e)
void TRewardResourceDlg::setReward(const TSeersHut::TRewardResource& reward)
{
    int index;
    for (index = 0; _m_typeCombo.GetItemData(index) != reward.getType(); index++)
        ;
    _m_typeCombo.SetCurSel(index);
    CString text;
    text.Format("%d", reward.getQuantity());
    _m_quantityEdit.SetWindowText(text);
}

VA(0x00413691, 0x8f)
std::auto_ptr<TSeersHut::TReward> TRewardResourceDlg::getReward() const
{
    std::auto_ptr<TSeersHut::TReward> pReward(new TSeersHut::TRewardResource(_m_type, _m_quantity));
    if (!pReward.get())
        throw TAllocationFailure();
    return pReward;
}

VA(0x00413720, 0x40)
void TRewardResourceDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_RESOURCE_COMBO, _m_typeCombo);
    DDX_Control(pDX, IDC_REWARD_QUANTITY_EDIT, _m_quantityEdit);
    DDX_Control(pDX, IDC_REWARD_QUANTITY_SPIN, _m_quantitySpin);
}

VA(0x00413760, 0xd5)
BOOL TRewardResourceDlg::OnInitDialog()
{
    GetDlgItem(IDC_REWARD_RESOURCE_TYPE_STATIC)->SetWindowText(SEditRewardDlgText::kResourceTypeStaticStr);
    GetDlgItem(IDC_QUEST_QUANTITY_STATIC)->SetWindowText(SEditRewardDlgText::kQuantityStaticStr);
    CDialog::OnInitDialog();
    for (unsigned int type = 0; type < kNumGameResourceTypes; type++)
        _m_typeCombo.SetItemData(_m_typeCombo.AddString(akGameResourceTypeTraits[type].m_name), type);
    int index;
    for (index = 0; _m_typeCombo.GetItemData(index) != eResourceGold; index++)
        ;
    _m_typeCombo.SetCurSel(index);
    _m_quantityEdit.SetWindowText("1");
    _m_quantityEdit.LimitText(TDigits<TSeersHut::TRewardResource::s_kMaxQuantity>::getDigits());
    _m_quantitySpin.SetRange(1, UD_MAXVAL);
    return TRUE;
}

VA(0x00413835, 0xb4)
void TRewardResourceDlg::OnOK()
{
    CDialog::OnOK();
    _m_type = TGameResourceType(_m_typeCombo.GetItemData(_m_typeCombo.GetCurSel()));
    int quantity = 0;
    CString text;
    _m_quantityEdit.GetWindowText(text);
    sscanf(text, "%d", &quantity);
    _m_quantity = clamp(1, quantity, int(TSeersHut::TRewardResource::s_kMaxQuantity));
}

VA(0x004138e9, 0xa8)
void TRewardResourceDlg::OnKillFocusQuantityEdit()
{
    int quantity = 0;
    CString text;
    _m_quantityEdit.GetWindowText(text);
    sscanf(text, "%d", &quantity);
    if (quantity < 1 || quantity > TSeersHut::TRewardResource::s_kMaxQuantity) {
        quantity = clamp(1, quantity, int(TSeersHut::TRewardResource::s_kMaxQuantity));
        text.Format("%d", quantity);
        _m_quantityEdit.SetWindowText(text);
    }
}

VA(0x00413991, 0x6)
BEGIN_MESSAGE_MAP(TRewardPrimarySkillDlg, TRewardTypeDlg)
    ON_EN_KILLFOCUS(IDC_REWARD_BONUS_EDIT, OnKillFocusBonusEdit)
END_MESSAGE_MAP()

VA(0x00413997, 0x9e)
void TRewardPrimarySkillDlg::setReward(const TSeersHut::TRewardPrimarySkill& reward)
{
    int index;
    for (index = 0; _m_skillCombo.GetItemData(index) != reward.getSkill(); index++)
        ;
    _m_skillCombo.SetCurSel(index);
    CString text;
    text.Format("%d", reward.getBonus());
    _m_bonusEdit.SetWindowText(text);
}

VA(0x00413a35, 0x8f)
std::auto_ptr<TSeersHut::TReward> TRewardPrimarySkillDlg::getReward() const
{
    std::auto_ptr<TSeersHut::TReward> pReward(new TSeersHut::TRewardPrimarySkill(_m_skill, _m_bonus));
    if (!pReward.get())
        throw TAllocationFailure();
    return pReward;
}

VA(0x00413ac4, 0x40)
void TRewardPrimarySkillDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_REWARD_PRIMARY_SKILL_COMBO, _m_skillCombo);
    DDX_Control(pDX, IDC_REWARD_BONUS_EDIT, _m_bonusEdit);
    DDX_Control(pDX, IDC_REWARD_BONUS_SPIN, _m_bonusSpin);
}

VA(0x00413b04, 0xd5)
BOOL TRewardPrimarySkillDlg::OnInitDialog()
{
    GetDlgItem(IDC_REWARD_PRIMARY_SKILL_STATIC)->SetWindowText(SEditRewardDlgText::kPrimarySkillStaticStr);
    GetDlgItem(IDC_REWARD_BONUS_STATIC)->SetWindowText(SEditRewardDlgText::kBonusStaticStr);
    CDialog::OnInitDialog();
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++)
        _m_skillCombo.SetItemData(_m_skillCombo.AddString(THero::s_akPrimarySkillTraits[skill].m_name), skill);
    int index;
    for (index = 0; _m_skillCombo.GetItemData(index) != ePriSkillAttack; index++)
        ;
    _m_skillCombo.SetCurSel(index);
    _m_bonusEdit.SetWindowText("1");
    _m_bonusEdit.LimitText(TDigits<s_kMaxBonus>::getDigits());
    _m_bonusSpin.SetRange(1, s_kMaxBonus);
    return TRUE;
}

VA(0x00413bd9, 0xaf)
void TRewardPrimarySkillDlg::OnOK()
{
    CDialog::OnOK();
    _m_skill = TPrimarySkill(_m_skillCombo.GetItemData(_m_skillCombo.GetCurSel()));
    _m_bonus = 0;
    CString text;
    _m_bonusEdit.GetWindowText(text);
    sscanf(text, "%d", &_m_bonus);
    _m_bonus = clamp(1, _m_bonus, int(s_kMaxBonus));
}

VA(0x00413c88, 0xa6)
void TRewardPrimarySkillDlg::OnKillFocusBonusEdit()
{
    int bonus = 0;
    CString text;
    _m_bonusEdit.GetWindowText(text);
    sscanf(text, "%d", &bonus);
    if (bonus < 1 || bonus > s_kMaxBonus) {
        bonus = clamp(1, bonus, int(s_kMaxBonus));
        text.Format("%d", bonus);
        _m_bonusEdit.SetWindowText(text);
    }
}

VA(0x00413d2e, 0x88)
void TRewardSecondarySkillDlg::setReward(const TSeersHut::TRewardSecondarySkill& reward)
{
    int index;
    for (index = 0; _m_skillCombo.GetItemData(index) != reward.getSkill(); index++)
        ;
    _m_skillCombo.SetCurSel(index);
    for (index = 0; _m_masteryCombo.GetItemData(index) != reward.getMastery(); index++)
        ;
    _m_masteryCombo.SetCurSel(index);
}

VA(0x00413db6, 0x8f)
std::auto_ptr<TSeersHut::TReward> TRewardSecondarySkillDlg::getReward() const
{
    std::auto_ptr<TSeersHut::TReward> pReward(new TSeersHut::TRewardSecondarySkill(_m_skill, _m_mastery));
    if (!pReward.get())
        throw TAllocationFailure();
    return pReward;
}

VA(0x00413e45, 0x2e)
void TRewardSecondarySkillDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_REWARD_SECONDARY_SKILL_COMBO, _m_skillCombo);
    DDX_Control(pDX, IDC_REWARD_MASTERY_COMBO, _m_masteryCombo);
}

VA(0x00413e73, 0xdd)
BOOL TRewardSecondarySkillDlg::OnInitDialog()
{
    GetDlgItem(IDC_REWARD_SECONDARY_SKILL_STATIC)->SetWindowText(SEditRewardDlgText::kSecondarySkillStaticStr);
    GetDlgItem(IDC_REWARD_MASTERY_STATIC)->SetWindowText(SEditRewardDlgText::kMasteryStaticStr);
    CDialog::OnInitDialog();
    unsigned int skill;
    for (skill = 0; skill < kNumSecSkills; skill++)
        _m_skillCombo.SetItemData(_m_skillCombo.AddString(THero::s_akSecondarySkillTraits[skill].m_name), skill);
    _m_skillCombo.SetCurSel(0);
    for (unsigned int mastery = eMasteryBasic; mastery < kNumMasteries; mastery++)
        _m_masteryCombo.SetItemData(
            _m_masteryCombo.AddString(THero::s_akSkillMasteryTraits[mastery - eMasteryBasic].m_name), mastery);
    int index;
    for (index = 0; _m_masteryCombo.GetItemData(index) != eMasteryBasic; index++)
        ;
    _m_masteryCombo.SetCurSel(index);
    return TRUE;
}

VA(0x00413f50, 0x58)
void TRewardSecondarySkillDlg::OnOK()
{
    CDialog::OnOK();
    _m_skill = TSecondarySkill(_m_skillCombo.GetItemData(_m_skillCombo.GetCurSel()));
    _m_mastery = TSkillMastery(_m_masteryCombo.GetItemData(_m_masteryCombo.GetCurSel()));
}

VA(0x00413fa8, 0x4f)
TRewardArtifactDlg::TRewardArtifactDlg(EGameVersion mapVersion) : _m_mapVersion(mapVersion)
{
}

VA_COMPGEN(0x0041400d, 0x1c, SCALAR_DELETING_DTOR, TRewardArtifactDlg)
VA_COMPGEN(0x00414029, 0x35, IMPLICIT_DTOR, TRewardArtifactDlg)

VA(0x0041405e, 0x53)
void TRewardArtifactDlg::setReward(const TSeersHut::TRewardArtifact& reward)
{
    int index;
    for (index = 0; _m_artifactCombo.GetItemData(index) != reward.getArtifact(); index++)
        ;
    _m_artifactCombo.SetCurSel(index);
}

VA(0x004140b1, 0x89)
std::auto_ptr<TSeersHut::TReward> TRewardArtifactDlg::getReward() const
{
    std::auto_ptr<TSeersHut::TReward> pReward(new TSeersHut::TRewardArtifact(_m_artifact));
    if (!pReward.get())
        throw TAllocationFailure();
    return pReward;
}

VA(0x0041413a, 0x15)
void TRewardArtifactDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_ARTIFACT_COMBO, _m_artifactCombo);
}

VA(0x0041414f, 0xa9)
BOOL TRewardArtifactDlg::OnInitDialog()
{
    GetDlgItem(IDC_REWARD_ARTIFACT_STATIC)->SetWindowText(SEditRewardDlgText::kArtifactStaticStr);
    CDialog::OnInitDialog();
    unsigned int numArtifacts;
    if (_m_mapVersion >= GAME_VERSION_SOD)
        numArtifacts = 144;
    else
        numArtifacts = _m_mapVersion >= GAME_VERSION_AB ? 129 : 127;
    for (unsigned int artifact = 0; artifact < numArtifacts; artifact++) {
        if (!akArtifactTraits[artifact].m_disabled && !(akArtifactTraits[artifact].m_class & ArtifactClassSpecial))
            _m_artifactCombo.SetItemData(_m_artifactCombo.AddString(akArtifactTraits[artifact].m_name), artifact);
    }
    _m_artifactCombo.SetCurSel(0);
    return TRUE;
}

VA(0x004141f8, 0x33)
void TRewardArtifactDlg::OnOK()
{
    CDialog::OnOK();
    _m_artifact = TArtifact(_m_artifactCombo.GetItemData(_m_artifactCombo.GetCurSel()));
}

VA(0x0041422b, 0x53)
void TRewardSpellDlg::setReward(const TSeersHut::TRewardSpell& reward)
{
    int index;
    for (index = 0; _m_spellCombo.GetItemData(index) != reward.getSpell(); index++)
        ;
    _m_spellCombo.SetCurSel(index);
}

VA(0x0041427e, 0x89)
std::auto_ptr<TSeersHut::TReward> TRewardSpellDlg::getReward() const
{
    std::auto_ptr<TSeersHut::TReward> pReward(new TSeersHut::TRewardSpell(_m_spell));
    if (!pReward.get())
        throw TAllocationFailure();
    return pReward;
}

VA(0x00414307, 0x15)
void TRewardSpellDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_SCROLL_SPELL_COMBO, _m_spellCombo);
}

VA(0x0041431c, 0x9b)
BOOL TRewardSpellDlg::OnInitDialog()
{
    GetDlgItem(IDC_REWARD_SPELL_STATIC)->SetWindowText(SEditRewardDlgText::kSpellStaticStr);
    CDialog::OnInitDialog();
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        if (!(akSpellTraits[spell].m_flags & 0x2000) && akSpellTraits[spell].m_schoolBits != 0)
            _m_spellCombo.SetItemData(_m_spellCombo.AddString(akSpellTraits[spell].m_name), spell);
    }
    int index;
    for (index = 0; _m_spellCombo.GetItemData(index) != SPELL_MAGIC_ARROW; index++)
        ;
    _m_spellCombo.SetCurSel(index);
    return TRUE;
}

VA(0x004143b7, 0x33)
void TRewardSpellDlg::OnOK()
{
    CDialog::OnOK();
    _m_spell = SpellID(_m_spellCombo.GetItemData(_m_spellCombo.GetCurSel()));
}

VA(0x004143ea, 0x6)
BEGIN_MESSAGE_MAP(TRewardCreatureDlg, TRewardTypeDlg)
    ON_EN_KILLFOCUS(IDC_STACK_QUANTITY_EDIT, OnKillFocusQuantityEdit)
END_MESSAGE_MAP()

// Inserts a creature in the list's order, its item data the creature.
VA(0x004143f0, 0x73)
void insertCreature(CComboBox& combo, TCreatureType creature)
{
    int index;
    for (index = combo.GetCount(); index > 0 && isCreatureBefore(creature, TCreatureType(combo.GetItemData(index - 1)));
         index--)
        ;
    combo.InsertString(index, akCreatureTypeTraits[creature].m_name);
    combo.SetItemData(index, creature);
}

VA(0x00414463, 0x8a)
TRewardCreatureDlg::TRewardCreatureDlg(EGameVersion mapVersion) : _m_mapVersion(mapVersion)
{
}

VA_COMPGEN(0x004144fe, 0x1c, SCALAR_DELETING_DTOR, TRewardCreatureDlg)
VA_COMPGEN(0x0041451a, 0x56, IMPLICIT_DTOR, TRewardCreatureDlg)

VA(0x00414570, 0x9b)
void TRewardCreatureDlg::setReward(const TSeersHut::TRewardCreature& reward)
{
    const TCreatureStack& creatureStack = reward.getCreatureStack();
    int index;
    for (index = 0; _m_creatureCombo.GetItemData(index) != creatureStack.getCreatureType(); index++)
        ;
    _m_creatureCombo.SetCurSel(index);
    CString text;
    text.Format("%d", creatureStack.getQuantity());
    _m_quantityEdit.SetWindowText(text);
}

VA(0x0041460b, 0x90)
std::auto_ptr<TSeersHut::TReward> TRewardCreatureDlg::getReward() const
{
    std::auto_ptr<TSeersHut::TReward> pReward(new TSeersHut::TRewardCreature(_m_creatureStack));
    if (!pReward.get())
        throw TAllocationFailure();
    return pReward;
}

VA(0x0041469b, 0x40)
void TRewardCreatureDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_CREATURE_COMBO, _m_creatureCombo);
    DDX_Control(pDX, IDC_STACK_QUANTITY_EDIT, _m_quantityEdit);
    DDX_Control(pDX, IDC_STACK_QUANTITY_SPIN, _m_quantitySpin);
}

VA(0x004146db, 0xd1)
BOOL TRewardCreatureDlg::OnInitDialog()
{
    GetDlgItem(IDC_QUEST_CREATURE_TYPE_STATIC)->SetWindowText(SEditRewardDlgText::kCreatureTypeStaticStr);
    GetDlgItem(IDC_QUEST_QUANTITY_STATIC)->SetWindowText(SEditRewardDlgText::kQuantityStaticStr);
    CDialog::OnInitDialog();
    unsigned int numCreatures = _m_mapVersion >= GAME_VERSION_AB ? kNumCreatureTypes : CREATURE_PIXIE;
    for (unsigned int creature = 0; creature < numCreatures; creature++) {
        if (akCreatureTypeTraits[creature].level >= 0)
            insertCreature(_m_creatureCombo, TCreatureType(creature));
    }
    _m_creatureCombo.SetCurSel(0);
    _m_quantityEdit.SetWindowText("1");
    _m_quantityEdit.LimitText(TDigits<TCreatureStack::s_kMaxQuantity>::getDigits());
    _m_quantitySpin.SetRange(1, TCreatureStack::s_kMaxQuantity);
    return TRUE;
}

VA(0x004147ac, 0xbd)
void TRewardCreatureDlg::OnOK()
{
    CDialog::OnOK();
    _m_creatureStack.setCreatureType(TCreatureType(_m_creatureCombo.GetItemData(_m_creatureCombo.GetCurSel())));
    int quantity = 0;
    CString text;
    _m_quantityEdit.GetWindowText(text);
    sscanf(text, "%d", &quantity);
    _m_creatureStack.setQuantity(clamp(1, quantity, int(TCreatureStack::s_kMaxQuantity)));
}

VA(0x00414869, 0xa8)
void TRewardCreatureDlg::OnKillFocusQuantityEdit()
{
    int quantity = 0;
    CString text;
    _m_quantityEdit.GetWindowText(text);
    sscanf(text, "%d", &quantity);
    if (quantity < 1 || quantity > TCreatureStack::s_kMaxQuantity) {
        quantity = clamp(1, quantity, int(TCreatureStack::s_kMaxQuantity));
        text.Format("%d", quantity);
        _m_quantityEdit.SetWindowText(text);
    }
}

VA(0x00414911, 0x149)
TEditRewardDlg::_TPages::_TPages(EGameVersion mapVersion) : m_artifactDlg(mapVersion), m_creatureDlg(mapVersion)
{
    m_apDlg[0] = &m_noneDlg;
    m_apDlg[1] = &m_experienceDlg;
    m_apDlg[2] = &m_manaDlg;
    m_apDlg[3] = &m_moraleDlg;
    m_apDlg[4] = &m_luckDlg;
    m_apDlg[5] = &m_resourceDlg;
    m_apDlg[6] = &m_primarySkillDlg;
    m_apDlg[7] = &m_secondarySkillDlg;
    m_apDlg[8] = &m_artifactDlg;
    m_apDlg[9] = &m_spellDlg;
    m_apDlg[10] = &m_creatureDlg;
}

VA_COMPGEN(0x00414a7c, 0x5e, CLASS_CTOR, TRewardExperienceDlg)
VA_COMPGEN(0x00414aeb, 0x47, IMPLICIT_DTOR, TRewardExperienceDlg)
VA_COMPGEN(0x00414b32, 0x5e, CLASS_CTOR, TRewardManaDlg)
VA_COMPGEN(0x00414ba1, 0x47, IMPLICIT_DTOR, TRewardManaDlg)
VA_COMPGEN(0x00414be8, 0x75, CLASS_CTOR, TRewardResourceDlg)
VA_COMPGEN(0x00414c6e, 0x56, IMPLICIT_DTOR, TRewardResourceDlg)
VA_COMPGEN(0x00414cc4, 0x75, CLASS_CTOR, TRewardPrimarySkillDlg)
VA_COMPGEN(0x00414d4a, 0x56, IMPLICIT_DTOR, TRewardPrimarySkillDlg)
VA_COMPGEN(0x00414da0, 0x5d, CLASS_CTOR, TRewardSecondarySkillDlg)
VA_COMPGEN(0x00414e0e, 0x47, IMPLICIT_DTOR, TRewardSecondarySkillDlg)
VA_COMPGEN(0x00414e55, 0x47, CLASS_CTOR, TRewardSpellDlg)
VA_COMPGEN(0x00414ead, 0x35, IMPLICIT_DTOR, TRewardSpellDlg)
VA_COMPGEN(0x00414ee2, 0x1c, SCALAR_DELETING_DTOR, TRewardExperienceDlg)
VA_COMPGEN(0x00414efe, 0x1c, SCALAR_DELETING_DTOR, TRewardManaDlg)
VA_COMPGEN(0x00414f1a, 0x1c, SCALAR_DELETING_DTOR, TRewardResourceDlg)
VA_COMPGEN(0x00414f36, 0x1c, SCALAR_DELETING_DTOR, TRewardPrimarySkillDlg)
VA_COMPGEN(0x00414f52, 0x1c, SCALAR_DELETING_DTOR, TRewardSecondarySkillDlg)
VA_COMPGEN(0x00414f6e, 0x1c, SCALAR_DELETING_DTOR, TRewardSpellDlg)

VA(0x00414f8a, 0x124)
TEditRewardDlg::TEditRewardDlg(CWnd* pParent, const TSeersHut::TReward* pReward, EGameVersion mapVersion)
    : CDialog(TEditRewardDlg::IDD, pParent)
{
    _m_rewardType = -1;
    _m_pPages = std::auto_ptr<_TPages>(new _TPages(mapVersion));
    if (!_m_pPages.get())
        throw TAllocationFailure();
    if (pReward != NULL) {
        _m_pReward = pReward->clone();
        if (!_m_pReward.get())
            throw TAllocationFailure();
    }
}

VA_COMPGEN(0x004150ae, 0x1c, SCALAR_DELETING_DTOR, TEditRewardDlg)

VA(0x004150ca, 0x54)
TEditRewardDlg::~TEditRewardDlg()
{
}

VA(0x0041511e, 0x49)
std::auto_ptr<TSeersHut::TReward> TEditRewardDlg::getReward()
{
    std::auto_ptr<TSeersHut::TReward> pReward(_m_pReward);
    _m_pReward = std::auto_ptr<TSeersHut::TReward>();
    return pReward;
}

VA(0x00415167, 0x3e)
void TEditRewardDlg::_showPage(int rewardType)
{
    _m_pPages->m_apDlg[_m_rewardType]->ShowWindow(SW_HIDE);
    _m_rewardType = rewardType;
    UpdateData(FALSE);
    _m_pPages->m_apDlg[rewardType]->ShowWindow(SW_SHOW);
}

VA(0x004151a5, 0x20)
void TEditRewardDlg::visit(const TSeersHut::TRewardExperience& reward)
{
    _m_pPages->m_experienceDlg.setReward(reward);
    _showPage(1);
}

VA(0x004151c5, 0x23)
void TEditRewardDlg::visit(const TSeersHut::TRewardMana& reward)
{
    _m_pPages->m_manaDlg.setReward(reward);
    _showPage(2);
}

VA(0x004151e8, 0x23)
void TEditRewardDlg::visit(const TSeersHut::TRewardMorale& reward)
{
    _m_pPages->m_moraleDlg.setReward(reward);
    _showPage(3);
}

VA(0x0041520b, 0x23)
void TEditRewardDlg::visit(const TSeersHut::TRewardLuck& reward)
{
    _m_pPages->m_luckDlg.setReward(reward);
    _showPage(4);
}

VA(0x0041522e, 0x23)
void TEditRewardDlg::visit(const TSeersHut::TRewardResource& reward)
{
    _m_pPages->m_resourceDlg.setReward(reward);
    _showPage(5);
}

VA(0x00415251, 0x23)
void TEditRewardDlg::visit(const TSeersHut::TRewardPrimarySkill& reward)
{
    _m_pPages->m_primarySkillDlg.setReward(reward);
    _showPage(6);
}

VA(0x00415274, 0x23)
void TEditRewardDlg::visit(const TSeersHut::TRewardSecondarySkill& reward)
{
    _m_pPages->m_secondarySkillDlg.setReward(reward);
    _showPage(7);
}

VA(0x00415297, 0x23)
void TEditRewardDlg::visit(const TSeersHut::TRewardArtifact& reward)
{
    _m_pPages->m_artifactDlg.setReward(reward);
    _showPage(8);
}

VA(0x004152ba, 0x23)
void TEditRewardDlg::visit(const TSeersHut::TRewardSpell& reward)
{
    _m_pPages->m_spellDlg.setReward(reward);
    _showPage(9);
}

VA(0x004152dd, 0x23)
void TEditRewardDlg::visit(const TSeersHut::TRewardCreature& reward)
{
    _m_pPages->m_creatureDlg.setReward(reward);
    _showPage(10);
}

// Folded by /OPT:ICF onto TMoraleBonusDlg::DoDataExchange.
void TEditRewardDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Radio(pDX, IDC_NONE_RADIO, _m_rewardType);
}

VA(0x00415300, 0x6)
BEGIN_MESSAGE_MAP(TEditRewardDlg, CDialog)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_NONE_RADIO, OnRewardTypeRadio)
    ON_BN_CLICKED(IDC_EXPERIENCE_RADIO, OnRewardTypeRadio)
    ON_BN_CLICKED(IDC_MANA_RADIO, OnRewardTypeRadio)
    ON_BN_CLICKED(IDC_MORALE_RADIO, OnRewardTypeRadio)
    ON_BN_CLICKED(IDC_LUCK_RADIO, OnRewardTypeRadio)
    ON_BN_CLICKED(IDC_RESOURCE_RADIO, OnRewardTypeRadio)
    ON_BN_CLICKED(IDC_PRIMARY_SKILL_RADIO, OnRewardTypeRadio)
    ON_BN_CLICKED(IDC_SECONDARY_SKILL_RADIO, OnRewardTypeRadio)
    ON_BN_CLICKED(IDC_ARTIFACT_RADIO, OnRewardTypeRadio)
    ON_BN_CLICKED(IDC_SPELL_RADIO, OnRewardTypeRadio)
    ON_BN_CLICKED(IDC_CREATURES_RADIO, OnRewardTypeRadio)
END_MESSAGE_MAP()

VA(0x00415306, 0x4b)
int TEditRewardDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(SEditRewardDlgText::kCaptionStr);
    for (unsigned int rewardType = 0; rewardType < s_kNumRewardTypes; rewardType++) {
        if (!_m_pPages->m_apDlg[rewardType]->create(this))
            return -1;
    }
    return 0;
}

VA(0x00415351, 0x29)
void TEditRewardDlg::OnDestroy()
{
    for (unsigned int rewardType = 0; rewardType < s_kNumRewardTypes; rewardType++)
        _m_pPages->m_apDlg[rewardType]->DestroyWindow();
    CDialog::OnDestroy();
}

VA(0x0041537a, 0x220)
BOOL TEditRewardDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_SELECT_REWARD_TYPE_STATIC)->SetWindowText(SEditRewardDlgText::kSelectRewardTypeStaticStr);
    GetDlgItem(IDC_NONE_RADIO)->SetWindowText(SEditRewardDlgText::kNoneRadioStr);
    GetDlgItem(IDC_EXPERIENCE_RADIO)->SetWindowText(SEditRewardDlgText::kExperienceRadioStr);
    GetDlgItem(IDC_MANA_RADIO)->SetWindowText(SEditRewardDlgText::kManaRadioStr);
    GetDlgItem(IDC_MORALE_RADIO)->SetWindowText(SEditRewardDlgText::kMoraleRadioStr);
    GetDlgItem(IDC_LUCK_RADIO)->SetWindowText(SEditRewardDlgText::kLuckRadioStr);
    GetDlgItem(IDC_RESOURCE_RADIO)->SetWindowText(SEditRewardDlgText::kResourceRadioStr);
    GetDlgItem(IDC_PRIMARY_SKILL_RADIO)->SetWindowText(SEditRewardDlgText::kPrimarySkillRadioStr);
    GetDlgItem(IDC_SECONDARY_SKILL_RADIO)->SetWindowText(SEditRewardDlgText::kSecondarySkillRadioStr);
    GetDlgItem(IDC_ARTIFACT_RADIO)->SetWindowText(SEditRewardDlgText::kArtifactRadioStr);
    GetDlgItem(IDC_SPELL_RADIO)->SetWindowText(SEditRewardDlgText::kSpellRadioStr);
    GetDlgItem(IDC_CREATURES_RADIO)->SetWindowText(SEditRewardDlgText::kCreaturesRadioStr);
    _m_rewardType = 0;
    CDialog::OnInitDialog();
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    const CWnd* pInsertAfter = pFrame;
    for (unsigned int rewardType = 0; rewardType < s_kNumRewardTypes; rewardType++) {
        TRewardTypeDlg* pDlg = _m_pPages->m_apDlg[rewardType];
        pDlg->SetWindowPos(pInsertAfter, rect.left, rect.top, 0, 0, SWP_NOSIZE);
        pInsertAfter = pDlg;
    }
    pFrame->DestroyWindow();
    _m_pPages->m_apDlg[0]->ShowWindow(SW_SHOW);
    if (_m_pReward.get() != NULL)
        _m_pReward->accept(this);
    return TRUE;
}

VA(0x0041559a, 0x4f)
void TEditRewardDlg::OnOK()
{
    CDialog::OnOK();
    TRewardTypeDlg* pDlg = _m_pPages->m_apDlg[_m_rewardType];
    pDlg->OnOK();
    _m_pReward = pDlg->getReward();
}

VA(0x004155e9, 0x36)
void TEditRewardDlg::OnRewardTypeRadio()
{
    _m_pPages->m_apDlg[_m_rewardType]->ShowWindow(SW_HIDE);
    UpdateData(TRUE);
    _m_pPages->m_apDlg[_m_rewardType]->ShowWindow(SW_SHOW);
}
