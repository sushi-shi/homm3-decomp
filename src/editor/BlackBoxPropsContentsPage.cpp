// BlackBoxPropsContentsPage.cpp - the contents page of the black box and
// event property sheets (h3maped 0x40794c..0x40bcf8; GOG only). Each
// category's dialog edits a copy of the box's contents; the list dialogs
// add, edit and remove entries (Delete removes the selected one) and keep
// their add buttons enabled while there is room.
#include "editor/stdafx.h"

#include <stdio.h>

#include <algorithm>
#include <bitset>

#include "artifact.h"
#include "va.h"
#include "editor/BlackBoxPropsContentsPage.h"
#include "editor/Digits.h"
#include "editor/EditCreatureStackDlg.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/ResourceQuantitiesDlg.h"
#include "editor/SelectArtifactDlg.h"

namespace {

// The experience a box gives.
class TExperienceBonusDlg : public CDialog {
public:
    TExperienceBonusDlg(const TBlackBox* pBlackBox);

    int getExperienceBonus() const { return _m_experienceBonus; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_EXPERIENCE_BONUS };

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    const TBlackBox* _m_pBlackBox;
    CEdit _m_experienceBonusEdit;
    CSpinButtonCtrl _m_experienceBonusSpin;
    int _m_experienceBonus;
    bool _m_bModified;
};

// The mana a box gives or takes.
class TManaBonusDlg : public CDialog {
public:
    TManaBonusDlg(const TBlackBox* pBlackBox);

    int getManaBonus() const { return _m_manaBonus; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MANA_BONUS };

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    const TBlackBox* _m_pBlackBox;
    CEdit _m_manaBonusEdit;
    CSpinButtonCtrl _m_manaBonusSpin;
    int _m_take;
    int _m_manaBonus;
    bool _m_bModified;
};

// The morale a box gives or takes: one radio per bonus.
class TMoraleBonusDlg : public CDialog {
public:
    TMoraleBonusDlg(const TBlackBox* pBlackBox)
        : CDialog(TMoraleBonusDlg::IDD), _m_pBlackBox(pBlackBox), _m_bModified(false) {}

    int getMoraleBonus() const { return _m_moraleBonus; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MORALE_BONUS };

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    static const int _s_kaBonus[TBlackBox::TContents::s_kMaxMoraleBonus - TBlackBox::TContents::s_kMinMoraleBonus + 1];

    const TBlackBox* _m_pBlackBox;
    int _m_bonusIndex;
    int _m_moraleBonus;
    bool _m_bModified;
};

// The luck a box gives or takes: one radio per bonus.
class TLuckBonusDlg : public CDialog {
public:
    TLuckBonusDlg(const TBlackBox* pBlackBox)
        : CDialog(TLuckBonusDlg::IDD), _m_pBlackBox(pBlackBox), _m_bModified(false) {}

    int getLuckBonus() const { return _m_luckBonus; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_LUCK_BONUS };

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    static const int _s_kaBonus[TBlackBox::TContents::s_kMaxLuckBonus - TBlackBox::TContents::s_kMinLuckBonus + 1];

    const TBlackBox* _m_pBlackBox;
    int _m_bonusIndex;
    int _m_luckBonus;
    bool _m_bModified;
};

// The resources a box gives or takes: the resource quantities dialog in
// place of a frame control.
class TResourcesDlg : public CDialog {
public:
    TResourcesDlg(const TBlackBox* pBlackBox);
    virtual ~TResourcesDlg();

    const TResourceQuantities& getResourceQuantities() const
    {
        return _m_pResourceQuantitiesDlg->getResourceQuantities();
    }
    bool wasModified() const { return _m_pResourceQuantitiesDlg->wasModified(); }

    enum { IDD = IDD_RESOURCES_CONTENTS };

    virtual void OnOK();

protected:
    virtual BOOL OnInitDialog();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

private:
    TResourceQuantitiesDlg* _m_pResourceQuantitiesDlg;
};

// The primary skill bonuses a box gives.
class TPrimarySkillBonusesDlg : public CDialog {
public:
    TPrimarySkillBonusesDlg(const TBlackBox* pBlackBox);

    const TPrimarySkillBonuses& getPrimarySkillBonuses() const { return _m_primarySkillBonuses; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_PRIMARY_SKILL_BONUSES };

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    // One skill's edit and spin.
    struct _TSkillControls {
        CEdit m_edit;
        CSpinButtonCtrl m_spin;
    };

    const TBlackBox* _m_pBlackBox;
    bool _m_bModified;
    TPrimarySkillBonuses _m_primarySkillBonuses;
    _TSkillControls _m_aSkillControls[kNumPrimarySkills];
};

// A secondary skill and its mastery, the skill among the available ones.
class TEditSecondarySkillDlg : public CDialog {
public:
    // A new skill: the first available one, basic.
    TEditSecondarySkillDlg(CWnd* pParent, const std::bitset<kNumSecSkills>& abAvailableSkill);
    TEditSecondarySkillDlg(CWnd* pParent, const std::bitset<kNumSecSkills>& abAvailableSkill,
                           const TSecondarySkillRecord& record);

    const TSecondarySkillRecord& getRecord() const { return _m_record; }

    enum { IDD = IDD_EDIT_SECONDARY_SKILL };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

private:
    TSecondarySkillRecord _m_record;
    std::bitset<kNumSecSkills> _m_abAvailableSkill;
    CComboBox _m_typeCombo;
    int _m_masteryIndex;
};

// The secondary skills a box gives, at most s_kMaxSecSkills.
class TSecondarySkillsDlg : public CDialog {
public:
    TSecondarySkillsDlg(const TBlackBox* pBlackBox);

    const std::vector<TSecondarySkillRecord>& getSecondarySkills() const { return _m_aSecondarySkill; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_SECONDARY_SKILLS_CONTENTS };

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnAddButton();
    afx_msg void OnEditButton();
    afx_msg void OnRemoveButton();
    afx_msg void OnRemoveAllButton();
    afx_msg void OnSelChangeList();
    afx_msg void OnSelCancelList();
    afx_msg void OnDblclkList();
    afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
    DECLARE_MESSAGE_MAP()

private:
    std::bitset<kNumSecSkills> _getAvailableSkills() const;

    const TBlackBox* _m_pBlackBox;
    bool _m_bModified;
    std::vector<TSecondarySkillRecord> _m_aSecondarySkill;
    CListBox _m_list;
    CButton _m_addButton;
    CButton _m_editButton;
    CButton _m_removeButton;
    CButton _m_removeAllButton;
};

// The artifacts a box gives, at most s_kMaxArtifacts.
class TArtifactsDlg : public CDialog {
public:
    TArtifactsDlg(const TBlackBox* pBlackBox, EGameVersion mapVersion);

    const std::vector<TArtifact>& getArtifacts() const { return _m_aArtifact; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_ARTIFACTS_CONTENTS };

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnAddButton();
    afx_msg void OnEditButton();
    afx_msg void OnRemoveButton();
    afx_msg void OnRemoveAllButton();
    afx_msg void OnSelChangeList();
    afx_msg void OnSelCancelList();
    afx_msg void OnDblclkList();
    afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
    DECLARE_MESSAGE_MAP()

private:
    const TBlackBox* _m_pBlackBox;
    EGameVersion _m_mapVersion;
    bool _m_bModified;
    std::vector<TArtifact> _m_aArtifact;
    CListBox _m_list;
    CButton _m_addButton;
    CButton _m_editButton;
    CButton _m_removeButton;
    CButton _m_removeAllButton;
};

// Picks a spell among the available ones.
class TSelectSpellDlg : public CDialog {
public:
    TSelectSpellDlg(CWnd* pParent, const std::bitset<kNumSpells>& abAvailableSpell, SpellID spell);

    SpellID getSpell() const { return _m_spell; }

    enum { IDD = IDD_SELECT_SPELL };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSelChangeList();
    afx_msg void OnSelCancelList();
    afx_msg void OnDblclkList();
    DECLARE_MESSAGE_MAP()

private:
    SpellID _m_spell;
    std::bitset<kNumSpells> _m_abAvailableSpell;
    CButton _m_okButton;
    CListBox _m_list;
};

// The spells a box gives, each at most once.
class TSpellsDlg : public CDialog {
public:
    TSpellsDlg(const TBlackBox* pBlackBox);

    const std::vector<SpellID>& getSpells() const { return _m_aSpell; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_SPELLS_CONTENTS };

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnAddButton();
    afx_msg void OnEditButton();
    afx_msg void OnRemoveButton();
    afx_msg void OnRemoveAllButton();
    afx_msg void OnSelChangeList();
    afx_msg void OnSelCancelList();
    afx_msg void OnDblclkList();
    afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
    DECLARE_MESSAGE_MAP()

private:
    const TBlackBox* _m_pBlackBox;
    bool _m_bModified;
    std::vector<SpellID> _m_aSpell;
    std::bitset<kNumSpells> _m_abAvailableSpell;
    CListBox _m_list;
    CButton _m_addButton;
    CButton _m_editButton;
    CButton _m_removeButton;
    CButton _m_removeAllButton;
};

// The creatures a box gives, at most s_kMaxCreatureStacks stacks.
class TCreaturesDlg : public CDialog {
public:
    TCreaturesDlg(const TBlackBox* pBlackBox, EGameVersion mapVersion);

    const std::vector<TCreatureStack>& getCreatureStacks() const { return _m_aCreatureStack; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_CREATURES_CONTENTS };

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnAddButton();
    afx_msg void OnEditButton();
    afx_msg void OnRemoveButton();
    afx_msg void OnRemoveAllButton();
    afx_msg void OnSelChangeList();
    afx_msg void OnSelCancelList();
    afx_msg void OnDblclkList();
    afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
    DECLARE_MESSAGE_MAP()

private:
    const TBlackBox* _m_pBlackBox;
    EGameVersion _m_mapVersion;
    bool _m_bModified;
    std::vector<TCreatureStack> _m_aCreatureStack;
    CListBox _m_list;
    CButton _m_addButton;
    CButton _m_editButton;
    CButton _m_removeButton;
    CButton _m_removeAllButton;
};

}

// The page's dialogs, allocated together.
struct TBlackBoxPropsContentsPage::_TDialogs {
    _TDialogs(const TBlackBox* pBlackBox, EGameVersion mapVersion);

    TExperienceBonusDlg m_experienceBonusDlg;
    TManaBonusDlg m_manaBonusDlg;
    TMoraleBonusDlg m_moraleBonusDlg;
    TLuckBonusDlg m_luckBonusDlg;
    TResourcesDlg m_resourcesDlg;
    TPrimarySkillBonusesDlg m_primarySkillBonusesDlg;
    TSecondarySkillsDlg m_secondarySkillsDlg;
    TArtifactsDlg m_artifactsDlg;
    TSpellsDlg m_spellsDlg;
    TCreaturesDlg m_creaturesDlg;
};

VA(0x00407bde, 0x6e)
TExperienceBonusDlg::TExperienceBonusDlg(const TBlackBox* pBlackBox)
    : CDialog(TExperienceBonusDlg::IDD),
      _m_pBlackBox(pBlackBox),
      _m_bModified(false)
{
}

VA_COMPGEN(0x00407c4c, 0x1c, SCALAR_DELETING_DTOR, TExperienceBonusDlg)
VA_COMPGEN(0x00407c68, 0x47, IMPLICIT_DTOR, TExperienceBonusDlg)

VA(0x00407caf, 0x2e)
void TExperienceBonusDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_EXPERIENCE_BONUS_EDIT, _m_experienceBonusEdit);
    DDX_Control(pDX, IDC_EXPERIENCE_BONUS_SPIN, _m_experienceBonusSpin);
}

VA(0x00407cdd, 0xbb)
BOOL TExperienceBonusDlg::OnInitDialog()
{
    GetDlgItem(IDC_EXPERIENCE_BONUS_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kExperienceBonusStaticStr);
    _m_bModified = false;
    _m_experienceBonus = _m_pBlackBox->getContents().getExperienceBonus();
    CDialog::OnInitDialog();
    _m_experienceBonusEdit.LimitText(TDigits<TBlackBox::TContents::s_kMaxExperienceBonus>::getDigits());
    _m_experienceBonusSpin.SetRange(0, UD_MAXVAL);
    CString text;
    text.Format("%d", _m_experienceBonus);
    _m_experienceBonusEdit.SetWindowText(text);
    return TRUE;
}

VA(0x00407d98, 0x80)
void TExperienceBonusDlg::OnOK()
{
    CDialog::OnOK();
    CString text;
    _m_experienceBonusEdit.GetWindowText(text);
    sscanf(text, "%d", &_m_experienceBonus);
    _m_bModified = _m_bModified || _m_experienceBonus != _m_pBlackBox->getContents().getExperienceBonus();
}

VA(0x00407e18, 0x6e)
TManaBonusDlg::TManaBonusDlg(const TBlackBox* pBlackBox)
    : CDialog(TManaBonusDlg::IDD),
      _m_pBlackBox(pBlackBox),
      _m_bModified(false)
{
}

VA_COMPGEN(0x00407e86, 0x1c, SCALAR_DELETING_DTOR, TManaBonusDlg)
VA_COMPGEN(0x00407ea2, 0x47, IMPLICIT_DTOR, TManaBonusDlg)

VA(0x00407ee9, 0x40)
void TManaBonusDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_MANA_BONUS_EDIT, _m_manaBonusEdit);
    DDX_Control(pDX, IDC_MANA_BONUS_SPIN, _m_manaBonusSpin);
    DDX_Radio(pDX, IDC_GIVE_MANA_RADIO, _m_take);
}

VA(0x00407f29, 0x103)
BOOL TManaBonusDlg::OnInitDialog()
{
    GetDlgItem(IDC_MANA_BONUS_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kManaBonusStaticStr);
    GetDlgItem(IDC_GIVE_MANA_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kGiveRadioStr);
    GetDlgItem(IDC_TAKE_MANA_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kTakeRadioStr);
    _m_bModified = false;
    _m_manaBonus = _m_pBlackBox->getContents().getManaBonus();
    _m_take = _m_manaBonus < 0;
    CDialog::OnInitDialog();
    _m_manaBonusEdit.LimitText(TDigits<TBlackBox::TContents::s_kMaxManaBonus>::getDigits());
    _m_manaBonusSpin.SetRange(0, TBlackBox::TContents::s_kMaxManaBonus);
    CString text;
    text.Format("%d", _m_manaBonus < 0 ? -_m_manaBonus : _m_manaBonus);
    _m_manaBonusEdit.SetWindowText(text);
    return TRUE;
}

VA(0x0040802c, 0x96)
void TManaBonusDlg::OnOK()
{
    CDialog::OnOK();
    _m_manaBonus = 0;
    CString text;
    _m_manaBonusEdit.GetWindowText(text);
    sscanf(text, "%d", &_m_manaBonus);
    if (_m_take)
        _m_manaBonus = -_m_manaBonus;
    _m_bModified = _m_bModified || _m_manaBonus != _m_pBlackBox->getContents().getManaBonus();
}

DATA(0x005309b8)
const int TMoraleBonusDlg::_s_kaBonus[] = { 0, 1, 2, 3, -1, -2, -3 };

VA(0x004080c2, 0x15)
void TMoraleBonusDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Radio(pDX, IDC_NONE_RADIO, _m_bonusIndex);
}

VA(0x004080d7, 0x10a)
BOOL TMoraleBonusDlg::OnInitDialog()
{
    GetDlgItem(IDC_MORALE_BONUS_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kMoraleBonusStaticStr);
    GetDlgItem(IDC_NONE_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kNoneRadioStr);
    GetDlgItem(IDC_PLUS_ONE_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kPlusOneRadioStr);
    GetDlgItem(IDC_PLUS_TWO_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kPlusTwoRadioStr);
    GetDlgItem(IDC_PLUS_THREE_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kPlusThreeRadioStr);
    GetDlgItem(IDC_MINUS_ONE_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kMinusOneRadioStr);
    GetDlgItem(IDC_MINUS_TWO_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kMinusTwoRadioStr);
    GetDlgItem(IDC_MINUS_THREE_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kMinusThreeRadioStr);
    _m_bModified = false;
    _m_moraleBonus = _m_pBlackBox->getContents().getMoraleBonus();
    _m_bonusIndex = std::find(_s_kaBonus, _s_kaBonus + sizeof(_s_kaBonus) / sizeof(_s_kaBonus[0]), _m_moraleBonus)
                    - _s_kaBonus;
    CDialog::OnInitDialog();
    return TRUE;
}

VA(0x004081e1, 0x32)
void TMoraleBonusDlg::OnOK()
{
    CDialog::OnOK();
    _m_moraleBonus = _s_kaBonus[_m_bonusIndex];
    _m_bModified = _m_bModified || _m_moraleBonus != _m_pBlackBox->getContents().getMoraleBonus();
}

DATA(0x005309d4)
const int TLuckBonusDlg::_s_kaBonus[] = { 0, 1, 2, 3, -1, -2, -3 };

void TLuckBonusDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Radio(pDX, IDC_NONE_RADIO, _m_bonusIndex);
}

VA(0x00408213, 0x10a)
BOOL TLuckBonusDlg::OnInitDialog()
{
    GetDlgItem(IDC_LUCK_BONUS_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kLuckBonusStaticStr);
    GetDlgItem(IDC_NONE_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kNoneRadioStr);
    GetDlgItem(IDC_PLUS_ONE_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kPlusOneRadioStr);
    GetDlgItem(IDC_PLUS_TWO_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kPlusTwoRadioStr);
    GetDlgItem(IDC_PLUS_THREE_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kPlusThreeRadioStr);
    GetDlgItem(IDC_MINUS_ONE_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kMinusOneRadioStr);
    GetDlgItem(IDC_MINUS_TWO_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kMinusTwoRadioStr);
    GetDlgItem(IDC_MINUS_THREE_RADIO)->SetWindowText(SBlackBoxPropsContentsPageText::kMinusThreeRadioStr);
    _m_bModified = false;
    _m_luckBonus = _m_pBlackBox->getContents().getLuckBonus();
    _m_bonusIndex = std::find(_s_kaBonus, _s_kaBonus + sizeof(_s_kaBonus) / sizeof(_s_kaBonus[0]), _m_luckBonus)
                    - _s_kaBonus;
    CDialog::OnInitDialog();
    return TRUE;
}

VA(0x0040831d, 0x32)
void TLuckBonusDlg::OnOK()
{
    CDialog::OnOK();
    _m_luckBonus = _s_kaBonus[_m_bonusIndex];
    _m_bModified = _m_bModified || _m_luckBonus != _m_pBlackBox->getContents().getLuckBonus();
}

VA(0x0040834f, 0x6)
BEGIN_MESSAGE_MAP(TResourcesDlg, CDialog)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

VA(0x00408355, 0x96)
TResourcesDlg::TResourcesDlg(const TBlackBox* pBlackBox)
    : CDialog(TResourcesDlg::IDD),
      _m_pResourceQuantitiesDlg(NULL)
{
    _m_pResourceQuantitiesDlg = new TResourceQuantitiesDlg(pBlackBox->getContents().getResourceQuantities());
    if (!_m_pResourceQuantitiesDlg)
        throw TAllocationFailure();
}

VA_COMPGEN(0x004083eb, 0x1c, SCALAR_DELETING_DTOR, TResourcesDlg)

VA(0x00408407, 0x41)
TResourcesDlg::~TResourcesDlg()
{
    delete _m_pResourceQuantitiesDlg;
}

VA(0x00408448, 0x2a)
int TResourcesDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1
        || !_m_pResourceQuantitiesDlg->Create(TResourceQuantitiesDlg::IDD, this))
        return -1;
    return 0;
}

VA(0x00408472, 0x14)
void TResourcesDlg::OnDestroy()
{
    _m_pResourceQuantitiesDlg->DestroyWindow();
    CDialog::OnDestroy();
}

VA(0x00408486, 0x58)
BOOL TResourcesDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pResourceQuantitiesDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    pFrame->DestroyWindow();
    return TRUE;
}

VA(0x004084de, 0x15)
void TResourcesDlg::OnOK()
{
    CDialog::OnOK();
    _m_pResourceQuantitiesDlg->OnOK();
}

DATA(0x00530a40)
static const int g_aakPrimarySkillControlIDs[kNumPrimarySkills][2] = {
    { IDC_ATTACK_EDIT, IDC_ATTACK_SPIN },
    { IDC_DEFENSE_EDIT, IDC_DEFENSE_SPIN },
    { IDC_SPELL_POWER_EDIT, IDC_SPELL_POWER_SPIN },
    { IDC_KNOWLEDGE_EDIT, IDC_KNOWLEDGE_SPIN }
};

VA(0x004084f3, 0x76)
TPrimarySkillBonusesDlg::TPrimarySkillBonusesDlg(const TBlackBox* pBlackBox)
    : CDialog(TPrimarySkillBonusesDlg::IDD),
      _m_pBlackBox(pBlackBox),
      _m_bModified(false)
{
}

VA_COMPGEN(0x00408569, 0x1c, SCALAR_DELETING_DTOR, TPrimarySkillBonusesDlg)
VA_COMPGEN(0x00408585, 0x41, CLASS_CTOR, _TSkillControls)
VA_COMPGEN(0x004085c6, 0x35, IMPLICIT_DTOR, _TSkillControls)
VA_COMPGEN(0x004085fb, 0x48, IMPLICIT_DTOR, TPrimarySkillBonusesDlg)

VA(0x00408643, 0x3c)
void TPrimarySkillBonusesDlg::DoDataExchange(CDataExchange* pDX)
{
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++) {
        DDX_Control(pDX, g_aakPrimarySkillControlIDs[skill][0], _m_aSkillControls[skill].m_edit);
        DDX_Control(pDX, g_aakPrimarySkillControlIDs[skill][1], _m_aSkillControls[skill].m_spin);
    }
}

VA(0x0040867f, 0x12e)
BOOL TPrimarySkillBonusesDlg::OnInitDialog()
{
    GetDlgItem(IDC_ATTACK_BONUS_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kAttackSkillStaticStr);
    GetDlgItem(IDC_DEFENSE_BONUS_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kDefenseSkillStaticStr);
    GetDlgItem(IDC_SPELL_POWER_BONUS_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kSpellPowerStaticStr);
    GetDlgItem(IDC_KNOWLEDGE_BONUS_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kKnowledgeStaticStr);
    _m_bModified = false;
    _m_primarySkillBonuses = _m_pBlackBox->getContents().getPrimarySkillBonuses();
    CDialog::OnInitDialog();
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++) {
        _m_aSkillControls[skill].m_edit.LimitText(TDigits<TPrimarySkillBonuses::s_kMax>::getDigits());
        CString text;
        text.Format("%d", _m_primarySkillBonuses.get(TPrimarySkill(skill)));
        _m_aSkillControls[skill].m_edit.SetWindowText(text);
        _m_aSkillControls[skill].m_spin.SetRange(0, TPrimarySkillBonuses::s_kMax);
    }
    return TRUE;
}

VA(0x004087ad, 0xba)
void TPrimarySkillBonusesDlg::OnOK()
{
    CDialog::OnOK();
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++) {
        int bonus = 0;
        CString text;
        _m_aSkillControls[skill].m_edit.GetWindowText(text);
        sscanf(text, "%d", &bonus);
        _m_primarySkillBonuses.set(TPrimarySkill(skill), bonus);
    }
    _m_bModified = _m_bModified || _m_primarySkillBonuses != _m_pBlackBox->getContents().getPrimarySkillBonuses();
}

VA(0x00408867, 0x6)
BEGIN_MESSAGE_MAP(TEditSecondarySkillDlg, CDialog)
    ON_WM_CREATE()
END_MESSAGE_MAP()

static TSecondarySkillRecord getFirstAvailableSkill(const std::bitset<kNumSecSkills>& abAvailableSkill);

VA(0x0040886d, 0x6c)
TEditSecondarySkillDlg::TEditSecondarySkillDlg(CWnd* pParent, const std::bitset<kNumSecSkills>& abAvailableSkill)
    : CDialog(TEditSecondarySkillDlg::IDD, pParent),
      _m_record(getFirstAvailableSkill(abAvailableSkill)),
      _m_abAvailableSkill(abAvailableSkill)
{
}

VA_COMPGEN(0x004088d9, 0x1c, SCALAR_DELETING_DTOR, TEditSecondarySkillDlg)
VA_COMPGEN(0x004088f5, 0x35, IMPLICIT_DTOR, TEditSecondarySkillDlg)

VA(0x0040892a, 0x74)
TEditSecondarySkillDlg::TEditSecondarySkillDlg(CWnd* pParent, const std::bitset<kNumSecSkills>& abAvailableSkill,
                                               const TSecondarySkillRecord& record)
    : CDialog(TEditSecondarySkillDlg::IDD, pParent),
      _m_record(record),
      _m_abAvailableSkill(abAvailableSkill)
{
    _m_abAvailableSkill.set(_m_record.getType(), true);
}

VA(0x0040899e, 0x4d)
static TSecondarySkillRecord getFirstAvailableSkill(const std::bitset<kNumSecSkills>& abAvailableSkill)
{
    unsigned int skill = 0;
    while (!abAvailableSkill.test(skill))
        skill++;
    return TSecondarySkillRecord(TSecondarySkill(skill), eMasteryBasic);
}

VA(0x004089eb, 0x2e)
void TEditSecondarySkillDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TYPE_COMBO, _m_typeCombo);
    DDX_Radio(pDX, IDC_BASIC_RADIO, _m_masteryIndex);
}

VA(0x00408a19, 0x25)
int TEditSecondarySkillDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kEditSecondarySkillCaptionStr);
    return 0;
}

VA(0x00408a3e, 0x163)
BOOL TEditSecondarySkillDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_TYPE_STATIC)->SetWindowText(SEditSecondarySkillDlgText::kTypeStaticStr);
    GetDlgItem(IDC_MASTERY_STATIC)->SetWindowText(SEditSecondarySkillDlgText::kMasteryStaticStr);
    GetDlgItem(IDC_BASIC_RADIO)->SetWindowText(SEditSecondarySkillDlgText::kBasicRadioStr);
    GetDlgItem(IDC_ADVANCED_RADIO)->SetWindowText(SEditSecondarySkillDlgText::kAdvancedRadioStr);
    GetDlgItem(IDC_EXPERT_RADIO)->SetWindowText(SEditSecondarySkillDlgText::kExpertRadioStr);
    _m_masteryIndex = _m_record.getMastery() - eMasteryBasic;
    CDialog::OnInitDialog();
    for (unsigned int skill = 0; skill < kNumSecSkills; skill++) {
        if (_m_abAvailableSkill[skill])
            _m_typeCombo.SetItemData(_m_typeCombo.AddString(THero::s_akSecondarySkillTraits[skill].m_name), skill);
    }
    int index = 0;
    while (_m_typeCombo.GetItemData(index) != _m_record.getType())
        index++;
    _m_typeCombo.SetCurSel(index);
    return TRUE;
}

VA(0x00408ba1, 0x54)
void TEditSecondarySkillDlg::OnOK()
{
    CDialog::OnOK();
    _m_record = TSecondarySkillRecord(TSecondarySkill(_m_typeCombo.GetItemData(_m_typeCombo.GetCurSel())),
                                      TSkillMastery(_m_masteryIndex + eMasteryBasic));
}

VA(0x00408bf5, 0x6)
BEGIN_MESSAGE_MAP(TSecondarySkillsDlg, CDialog)
    ON_BN_CLICKED(IDC_SEC_SKILLS_ADD_BUTTON, OnAddButton)
    ON_BN_CLICKED(IDC_SEC_SKILLS_EDIT_BUTTON, OnEditButton)
    ON_BN_CLICKED(IDC_SEC_SKILLS_REMOVE_BUTTON, OnRemoveButton)
    ON_BN_CLICKED(IDC_SEC_SKILLS_REMOVE_ALL_BUTTON, OnRemoveAllButton)
    ON_LBN_SELCHANGE(IDC_SEC_SKILLS_LIST, OnSelChangeList)
    ON_LBN_SELCANCEL(IDC_SEC_SKILLS_LIST, OnSelCancelList)
    ON_LBN_DBLCLK(IDC_SEC_SKILLS_LIST, OnDblclkList)
    ON_WM_VKEYTOITEM()
END_MESSAGE_MAP()

VA(0x00408bfb, 0x7e)
static CString getSecondarySkillText(TSecondarySkill skill, TSkillMastery mastery)
{
    return CString(THero::s_akSecondarySkillTraits[skill].m_name) + "\t"
           + THero::s_akSkillMasteryTraits[mastery - eMasteryBasic].m_name;
}

VA(0x00408c79, 0xb9)
TSecondarySkillsDlg::TSecondarySkillsDlg(const TBlackBox* pBlackBox)
    : CDialog(TSecondarySkillsDlg::IDD),
      _m_pBlackBox(pBlackBox),
      _m_bModified(false)
{
}

VA_COMPGEN(0x00408d32, 0x1c, SCALAR_DELETING_DTOR, TSecondarySkillsDlg)
VA_COMPGEN(0x00408d4e, 0x80, IMPLICIT_DTOR, TSecondarySkillsDlg)

VA(0x00408dce, 0x6b)
std::bitset<kNumSecSkills> TSecondarySkillsDlg::_getAvailableSkills() const
{
    std::bitset<kNumSecSkills> abAvailableSkill;
    abAvailableSkill.set();
    int count = _m_list.GetCount();
    for (int index = 0; index < count; index++)
        abAvailableSkill.set(LOWORD(_m_list.GetItemData(index)), false);
    return abAvailableSkill;
}

VA(0x00408e39, 0x64)
void TSecondarySkillsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_SEC_SKILLS_LIST, _m_list);
    DDX_Control(pDX, IDC_SEC_SKILLS_ADD_BUTTON, _m_addButton);
    DDX_Control(pDX, IDC_SEC_SKILLS_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_SEC_SKILLS_REMOVE_BUTTON, _m_removeButton);
    DDX_Control(pDX, IDC_SEC_SKILLS_REMOVE_ALL_BUTTON, _m_removeAllButton);
}

VA(0x00408e9d, 0x1b2)
BOOL TSecondarySkillsDlg::OnInitDialog()
{
    GetDlgItem(IDC_SECONDARY_SKILL_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kSecondarySkillStaticStr);
    GetDlgItem(IDC_MASTERY_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kMasteryStaticStr);
    GetDlgItem(IDC_SEC_SKILLS_ADD_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kAddButtonStr);
    GetDlgItem(IDC_SEC_SKILLS_EDIT_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kEditButtonStr);
    GetDlgItem(IDC_SEC_SKILLS_REMOVE_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kRemoveButtonStr);
    GetDlgItem(IDC_SEC_SKILLS_REMOVE_ALL_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kRemoveAllButtonStr);
    _m_bModified = false;
    _m_aSecondarySkill = _m_pBlackBox->getContents().getSecondarySkills();
    CDialog::OnInitDialog();
    _m_list.SetTabStops(171);
    for (std::vector<TSecondarySkillRecord>::const_iterator it = _m_aSecondarySkill.begin();
         it != _m_aSecondarySkill.end(); ++it) {
        int index = _m_list.AddString(getSecondarySkillText(it->getType(), it->getMastery()));
        _m_list.SetItemData(index, MAKELONG(it->getType(), it->getMastery()));
    }
    _m_list.SetCurSel(-1);
    _m_addButton.EnableWindow(_m_list.GetCount() < TBlackBox::TContents::s_kMaxSecSkills);
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_removeAllButton.EnableWindow(_m_list.GetCount() > 0);
    return TRUE;
}

VA(0x0040904f, 0xa6)
void TSecondarySkillsDlg::OnOK()
{
    CDialog::OnOK();
    _m_aSecondarySkill.erase(_m_aSecondarySkill.begin(), _m_aSecondarySkill.end());
    int count = _m_list.GetCount();
    _m_aSecondarySkill.reserve(count);
    for (int index = 0; index < count; index++) {
        DWORD data = _m_list.GetItemData(index);
        _m_aSecondarySkill.push_back(TSecondarySkillRecord(TSecondarySkill(LOWORD(data)), TSkillMastery(HIWORD(data))));
    }
    _m_bModified = _m_bModified || _m_aSecondarySkill != _m_pBlackBox->getContents().getSecondarySkills();
}

VA(0x004090f5, 0x11c)
void TSecondarySkillsDlg::OnAddButton()
{
    TEditSecondarySkillDlg dlg(this, _getAvailableSkills());
    if (dlg.DoModal() == IDOK) {
        CString text = getSecondarySkillText(dlg.getRecord().getType(), dlg.getRecord().getMastery());
        int index = _m_list.GetCurSel();
        if (index == LB_ERR)
            index = _m_list.AddString(text);
        else
            _m_list.InsertString(index, text);
        _m_list.SetItemData(index, MAKELONG(dlg.getRecord().getType(), dlg.getRecord().getMastery()));
        if (_m_list.GetCount() >= TBlackBox::TContents::s_kMaxSecSkills)
            _m_addButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(TRUE);
    }
}

VA(0x00409211, 0x123)
void TSecondarySkillsDlg::OnEditButton()
{
    int index = _m_list.GetCurSel();
    DWORD data = _m_list.GetItemData(index);
    TSecondarySkillRecord record(TSecondarySkill(LOWORD(data)), TSkillMastery(HIWORD(data)));
    TEditSecondarySkillDlg dlg(this, _getAvailableSkills(), record);
    if (dlg.DoModal() == IDOK
        && (dlg.getRecord().getType() != record.getType() || dlg.getRecord().getMastery() != record.getMastery())) {
        _m_list.DeleteString(index);
        _m_list.InsertString(index, getSecondarySkillText(dlg.getRecord().getType(), dlg.getRecord().getMastery()));
        _m_list.SetItemData(index, MAKELONG(dlg.getRecord().getType(), dlg.getRecord().getMastery()));
        _m_list.SetCurSel(index);
    }
}

VA(0x00409334, 0xb8)
void TSecondarySkillsDlg::OnRemoveButton()
{
    int index = _m_list.GetCurSel();
    _m_list.DeleteString(index);
    int count = _m_list.GetCount();
    if (count > 0) {
        if (count < TBlackBox::TContents::s_kMaxSecSkills)
            _m_addButton.EnableWindow(TRUE);
        _m_list.SetCurSel(index < count ? index : count - 1);
    } else {
        _m_addButton.EnableWindow(TRUE);
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(FALSE);
        GotoDlgCtrl(&_m_addButton);
    }
}

VA(0x004093ec, 0x55)
void TSecondarySkillsDlg::OnRemoveAllButton()
{
    int count = _m_list.GetCount();
    if (_m_list.GetCurSel() == LB_ERR)
        _m_list.SetCurSel(0);
    for (; count > 0; count--)
        OnRemoveButton();
}

VA(0x00409441, 0x1f)
void TSecondarySkillsDlg::OnSelChangeList()
{
    _m_editButton.EnableWindow(TRUE);
    _m_removeButton.EnableWindow(TRUE);
}

VA(0x00409460, 0x1f)
void TSecondarySkillsDlg::OnSelCancelList()
{
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
}

VA(0x0040947f, 0x26)
void TSecondarySkillsDlg::OnDblclkList()
{
    if (_m_list.GetCurSel() != LB_ERR)
        OnEditButton();
}

VA(0x004094a5, 0x47)
int TSecondarySkillsDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    if (nKey == VK_DELETE && _m_list.GetCurSel() != LB_ERR) {
        OnRemoveButton();
        return -2;
    }
    return CDialog::OnVKeyToItem(nKey, pListBox, nIndex);
}

VA(0x004094ec, 0x6)
BEGIN_MESSAGE_MAP(TArtifactsDlg, CDialog)
    ON_BN_CLICKED(IDC_ARTIFACTS_ADD_BUTTON, OnAddButton)
    ON_BN_CLICKED(IDC_ARTIFACTS_EDIT_BUTTON, OnEditButton)
    ON_BN_CLICKED(IDC_ARTIFACTS_REMOVE_BUTTON, OnRemoveButton)
    ON_BN_CLICKED(IDC_ARTIFACTS_REMOVE_ALL_BUTTON, OnRemoveAllButton)
    ON_LBN_SELCHANGE(IDC_ARTIFACT_LIST, OnSelChangeList)
    ON_LBN_SELCANCEL(IDC_ARTIFACT_LIST, OnSelCancelList)
    ON_LBN_DBLCLK(IDC_ARTIFACT_LIST, OnDblclkList)
    ON_WM_VKEYTOITEM()
END_MESSAGE_MAP()

VA(0x004094f2, 0xbf)
TArtifactsDlg::TArtifactsDlg(const TBlackBox* pBlackBox, EGameVersion mapVersion)
    : CDialog(TArtifactsDlg::IDD),
      _m_pBlackBox(pBlackBox),
      _m_mapVersion(mapVersion),
      _m_bModified(false)
{
}

VA_COMPGEN(0x004095b1, 0x1c, SCALAR_DELETING_DTOR, TArtifactsDlg)
VA_COMPGEN(0x004095cd, 0x80, IMPLICIT_DTOR, TArtifactsDlg)

VA(0x0040964d, 0x64)
void TArtifactsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_ARTIFACT_LIST, _m_list);
    DDX_Control(pDX, IDC_ARTIFACTS_ADD_BUTTON, _m_addButton);
    DDX_Control(pDX, IDC_ARTIFACTS_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_ARTIFACTS_REMOVE_BUTTON, _m_removeButton);
    DDX_Control(pDX, IDC_ARTIFACTS_REMOVE_ALL_BUTTON, _m_removeAllButton);
}

VA(0x004096b1, 0x18b)
BOOL TArtifactsDlg::OnInitDialog()
{
    GetDlgItem(IDC_ARTIFACTS_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kArtifactsStaticStr);
    GetDlgItem(IDC_ARTIFACTS_ADD_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kAddButtonStr);
    GetDlgItem(IDC_ARTIFACTS_EDIT_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kEditButtonStr);
    GetDlgItem(IDC_ARTIFACTS_REMOVE_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kRemoveButtonStr);
    GetDlgItem(IDC_ARTIFACTS_REMOVE_ALL_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kRemoveAllButtonStr);
    _m_bModified = false;
    _m_aArtifact = _m_pBlackBox->getContents().getArtifacts();
    CDialog::OnInitDialog();
    CRect rect;
    _m_list.GetClientRect(&rect);
    _m_list.SetColumnWidth(rect.Width() / 2);
    for (std::vector<TArtifact>::const_iterator it = _m_aArtifact.begin(); it != _m_aArtifact.end(); ++it)
        _m_list.SetItemData(_m_list.AddString(akArtifactTraits[*it].m_name), *it);
    _m_list.SetCurSel(-1);
    _m_addButton.EnableWindow(_m_list.GetCount() < TBlackBox::TContents::s_kMaxArtifacts);
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_removeAllButton.EnableWindow(_m_list.GetCount() > 0);
    return TRUE;
}

VA(0x0040983c, 0xa1)
void TArtifactsDlg::OnOK()
{
    CDialog::OnOK();
    _m_aArtifact.erase(_m_aArtifact.begin(), _m_aArtifact.end());
    int count = _m_list.GetCount();
    _m_aArtifact.reserve(count);
    for (int index = 0; index < count; index++) {
        TArtifact artifact = TArtifact(_m_list.GetItemData(index));
        _m_aArtifact.push_back(artifact);
    }
    _m_bModified = _m_bModified || _m_aArtifact != _m_pBlackBox->getContents().getArtifacts();
}

VA(0x004098dd, 0xf9)
void TArtifactsDlg::OnAddButton()
{
    TSelectArtifactDlg dlg(this, _m_mapVersion, ARTIFACT_NONE);
    if (dlg.DoModal() == IDOK) {
        TArtifact artifact = TArtifact(dlg.getArtifact());
        int index = _m_list.GetCurSel();
        if (index == LB_ERR)
            index = _m_list.AddString(akArtifactTraits[artifact].m_name);
        else
            _m_list.InsertString(index, akArtifactTraits[artifact].m_name);
        _m_list.SetItemData(index, artifact);
        if (_m_list.GetCount() >= TBlackBox::TContents::s_kMaxArtifacts)
            _m_addButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(TRUE);
    }
}

VA(0x00409a1d, 0xf1)
void TArtifactsDlg::OnEditButton()
{
    int index = _m_list.GetCurSel();
    TArtifact artifact = TArtifact(_m_list.GetItemData(_m_list.GetCurSel()));
    TSelectArtifactDlg dlg(this, _m_mapVersion, artifact);
    if (dlg.DoModal() == IDOK && dlg.getArtifact() != artifact) {
        artifact = TArtifact(dlg.getArtifact());
        _m_list.DeleteString(index);
        _m_list.InsertString(index, akArtifactTraits[artifact].m_name);
        _m_list.SetItemData(index, artifact);
        _m_list.SetCurSel(index);
    }
}

VA(0x00409b0e, 0xb8)
void TArtifactsDlg::OnRemoveButton()
{
    int index = _m_list.GetCurSel();
    _m_list.DeleteString(index);
    int count = _m_list.GetCount();
    if (count > 0) {
        if (count < TBlackBox::TContents::s_kMaxArtifacts)
            _m_addButton.EnableWindow(TRUE);
        _m_list.SetCurSel(index < count ? index : count - 1);
    } else {
        _m_addButton.EnableWindow(TRUE);
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(FALSE);
        GotoDlgCtrl(&_m_addButton);
    }
}

VA(0x00409be2, 0x55)
void TArtifactsDlg::OnRemoveAllButton()
{
    int count = _m_list.GetCount();
    if (_m_list.GetCurSel() == LB_ERR)
        _m_list.SetCurSel(0);
    for (; count > 0; count--)
        OnRemoveButton();
}

VA(0x00409c37, 0x1f)
void TArtifactsDlg::OnSelChangeList()
{
    _m_editButton.EnableWindow(TRUE);
    _m_removeButton.EnableWindow(TRUE);
}

void TArtifactsDlg::OnSelCancelList()
{
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
}

VA(0x00409c56, 0x26)
void TArtifactsDlg::OnDblclkList()
{
    if (_m_list.GetCurSel() != LB_ERR)
        OnEditButton();
}

VA(0x00409c7c, 0x47)
int TArtifactsDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    if (nKey == VK_DELETE && _m_list.GetCurSel() != LB_ERR) {
        OnRemoveButton();
        return -2;
    }
    return CDialog::OnVKeyToItem(nKey, pListBox, nIndex);
}

VA(0x00409cc3, 0x6)
BEGIN_MESSAGE_MAP(TSelectSpellDlg, CDialog)
    ON_WM_CREATE()
    ON_LBN_SELCHANGE(IDC_SPELL_LIST, OnSelChangeList)
    ON_LBN_SELCANCEL(IDC_SPELL_LIST, OnSelCancelList)
    ON_LBN_DBLCLK(IDC_SPELL_LIST, OnDblclkList)
END_MESSAGE_MAP()

VA(0x00409cc9, 0x8a)
TSelectSpellDlg::TSelectSpellDlg(CWnd* pParent, const std::bitset<kNumSpells>& abAvailableSpell, SpellID spell)
    : CDialog(TSelectSpellDlg::IDD, pParent),
      _m_spell(spell),
      _m_abAvailableSpell(abAvailableSpell)
{
    if (_m_spell != SPELL_NONE)
        _m_abAvailableSpell.set(_m_spell, true);
}

VA_COMPGEN(0x00409d53, 0x1c, SCALAR_DELETING_DTOR, TSelectSpellDlg)
VA_COMPGEN(0x00409d6f, 0x47, IMPLICIT_DTOR, TSelectSpellDlg)

VA(0x00409db6, 0x2b)
void TSelectSpellDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDOK, _m_okButton);
    DDX_Control(pDX, IDC_SPELL_LIST, _m_list);
}

VA(0x00409de1, 0x25)
int TSelectSpellDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kSelectSpellCaptionStr);
    return 0;
}

VA(0x00409e06, 0x128)
BOOL TSelectSpellDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    CDialog::OnInitDialog();
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        if (!(akSpellTraits[spell].m_flags & 0x2000) && akSpellTraits[spell].m_schoolBits != 0
            && _m_abAvailableSpell[spell])
            _m_list.SetItemData(_m_list.AddString(akSpellTraits[spell].m_name), spell);
    }
    if (_m_spell != SPELL_NONE) {
        int index = 0;
        while (_m_list.GetItemData(index) != _m_spell)
            index++;
        _m_list.SetCurSel(index);
        _m_list.SetTopIndex(index);
    } else {
        _m_okButton.EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x00409f2e, 0x36)
void TSelectSpellDlg::OnOK()
{
    CDialog::OnOK();
    _m_spell = SpellID(_m_list.GetItemData(_m_list.GetCurSel()));
}

VA(0x00409f64, 0xb)
void TSelectSpellDlg::OnSelChangeList()
{
    _m_okButton.EnableWindow(TRUE);
}

VA(0x00409f6f, 0xb)
void TSelectSpellDlg::OnSelCancelList()
{
    _m_okButton.EnableWindow(FALSE);
}

VA(0x00409f7a, 0x29)
void TSelectSpellDlg::OnDblclkList()
{
    if (_m_list.GetCurSel() != LB_ERR)
        OnOK();
}

VA(0x00409fa3, 0x6)
BEGIN_MESSAGE_MAP(TSpellsDlg, CDialog)
    ON_BN_CLICKED(IDC_SPELLS_ADD_BUTTON, OnAddButton)
    ON_BN_CLICKED(IDC_SPELLS_EDIT_BUTTON, OnEditButton)
    ON_BN_CLICKED(IDC_SPELLS_REMOVE_BUTTON, OnRemoveButton)
    ON_BN_CLICKED(IDC_SPELLS_REMOVE_ALL_BUTTON, OnRemoveAllButton)
    ON_LBN_SELCHANGE(IDC_SPELL_LIST, OnSelChangeList)
    ON_LBN_SELCANCEL(IDC_SPELL_LIST, OnSelCancelList)
    ON_LBN_DBLCLK(IDC_SPELL_LIST, OnDblclkList)
    ON_WM_VKEYTOITEM()
END_MESSAGE_MAP()

VA(0x00409fa9, 0xca)
TSpellsDlg::TSpellsDlg(const TBlackBox* pBlackBox)
    : CDialog(TSpellsDlg::IDD),
      _m_pBlackBox(pBlackBox),
      _m_bModified(false)
{
}

VA_COMPGEN(0x0040a073, 0x1c, SCALAR_DELETING_DTOR, TSpellsDlg)
VA_COMPGEN(0x0040a08f, 0x83, IMPLICIT_DTOR, TSpellsDlg)

VA(0x0040a112, 0x67)
void TSpellsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_SPELL_LIST, _m_list);
    DDX_Control(pDX, IDC_SPELLS_ADD_BUTTON, _m_addButton);
    DDX_Control(pDX, IDC_SPELLS_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_SPELLS_REMOVE_BUTTON, _m_removeButton);
    DDX_Control(pDX, IDC_SPELLS_REMOVE_ALL_BUTTON, _m_removeAllButton);
}

VA(0x0040a179, 0x1da)
BOOL TSpellsDlg::OnInitDialog()
{
    GetDlgItem(IDC_CONTENTS_SPELLS_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kSpellsStaticStr);
    GetDlgItem(IDC_SPELLS_ADD_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kAddButtonStr);
    GetDlgItem(IDC_SPELLS_EDIT_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kEditButtonStr);
    GetDlgItem(IDC_SPELLS_REMOVE_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kRemoveButtonStr);
    GetDlgItem(IDC_SPELLS_REMOVE_ALL_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kRemoveAllButtonStr);
    _m_bModified = false;
    _m_aSpell = _m_pBlackBox->getContents().getSpells();
    _m_abAvailableSpell.set();
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        if ((akSpellTraits[spell].m_flags & 0x2000) || akSpellTraits[spell].m_schoolBits == 0)
            _m_abAvailableSpell.set(spell, false);
    }
    CDialog::OnInitDialog();
    CRect rect;
    _m_list.GetClientRect(&rect);
    _m_list.SetColumnWidth(rect.Width() / 2);
    for (std::vector<SpellID>::const_iterator it = _m_aSpell.begin(); it != _m_aSpell.end(); ++it) {
        _m_abAvailableSpell.set(*it, false);
        _m_list.SetItemData(_m_list.AddString(akSpellTraits[*it].m_name), *it);
    }
    _m_list.SetCurSel(-1);
    _m_addButton.EnableWindow(_m_abAvailableSpell.any());
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_removeAllButton.EnableWindow(_m_list.GetCount() > 0);
    return TRUE;
}

VA(0x0040a353, 0xa1)
void TSpellsDlg::OnOK()
{
    CDialog::OnOK();
    _m_aSpell.erase(_m_aSpell.begin(), _m_aSpell.end());
    int count = _m_list.GetCount();
    _m_aSpell.reserve(count);
    for (int index = 0; index < count; index++) {
        SpellID spell = SpellID(_m_list.GetItemData(index));
        _m_aSpell.push_back(spell);
    }
    _m_bModified = _m_bModified || _m_aSpell != _m_pBlackBox->getContents().getSpells();
}

VA(0x0040a3f4, 0x10e)
void TSpellsDlg::OnAddButton()
{
    TSelectSpellDlg dlg(this, _m_abAvailableSpell, SPELL_NONE);
    if (dlg.DoModal() == IDOK) {
        SpellID spell = dlg.getSpell();
        _m_abAvailableSpell.set(spell, false);
        int index = _m_list.GetCurSel();
        if (index == LB_ERR)
            index = _m_list.AddString(akSpellTraits[spell].m_name);
        else
            _m_list.InsertString(index, akSpellTraits[spell].m_name);
        _m_list.SetItemData(index, spell);
        if (!_m_abAvailableSpell.any())
            _m_addButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(TRUE);
    }
}

VA(0x0040a502, 0x113)
void TSpellsDlg::OnEditButton()
{
    int index = _m_list.GetCurSel();
    SpellID spell = SpellID(_m_list.GetItemData(_m_list.GetCurSel()));
    TSelectSpellDlg dlg(this, _m_abAvailableSpell, spell);
    if (dlg.DoModal() == IDOK && dlg.getSpell() != spell) {
        SpellID newSpell = dlg.getSpell();
        _m_list.DeleteString(index);
        _m_abAvailableSpell.set(spell, true);
        _m_abAvailableSpell.set(newSpell, false);
        _m_list.InsertString(index, akSpellTraits[newSpell].m_name);
        _m_list.SetItemData(index, newSpell);
        _m_list.SetCurSel(index);
    }
}

VA(0x0040a615, 0xe2)
void TSpellsDlg::OnRemoveButton()
{
    int index = _m_list.GetCurSel();
    SpellID spell = SpellID(_m_list.GetItemData(index));
    _m_list.DeleteString(index);
    _m_abAvailableSpell.set(spell, true);
    int count = _m_list.GetCount();
    if (count > 0) {
        _m_list.SetCurSel(index < count ? index : count - 1);
        if (_m_abAvailableSpell.any())
            _m_addButton.EnableWindow(TRUE);
    } else {
        _m_addButton.EnableWindow(TRUE);
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(FALSE);
        GotoDlgCtrl(&_m_addButton);
    }
}

VA(0x0040a6f7, 0x55)
void TSpellsDlg::OnRemoveAllButton()
{
    int count = _m_list.GetCount();
    if (_m_list.GetCurSel() == LB_ERR)
        _m_list.SetCurSel(0);
    for (; count > 0; count--)
        OnRemoveButton();
}

VA(0x0040a74c, 0x1f)
void TSpellsDlg::OnSelChangeList()
{
    _m_editButton.EnableWindow(TRUE);
    _m_removeButton.EnableWindow(TRUE);
}

VA(0x0040a76b, 0x1f)
void TSpellsDlg::OnSelCancelList()
{
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
}

VA(0x0040a78a, 0x26)
void TSpellsDlg::OnDblclkList()
{
    if (_m_list.GetCurSel() != LB_ERR)
        OnEditButton();
}

VA(0x0040a7b0, 0x47)
int TSpellsDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    if (nKey == VK_DELETE && _m_list.GetCurSel() != LB_ERR) {
        OnRemoveButton();
        return -2;
    }
    return CDialog::OnVKeyToItem(nKey, pListBox, nIndex);
}

VA(0x0040a7f7, 0x6)
BEGIN_MESSAGE_MAP(TCreaturesDlg, CDialog)
    ON_BN_CLICKED(IDC_CREATURES_ADD_BUTTON, OnAddButton)
    ON_BN_CLICKED(IDC_CREATURES_EDIT_BUTTON, OnEditButton)
    ON_BN_CLICKED(IDC_CREATURES_REMOVE_BUTTON, OnRemoveButton)
    ON_BN_CLICKED(IDC_CREATURES_REMOVE_ALL_BUTTON, OnRemoveAllButton)
    ON_LBN_SELCHANGE(IDC_CREATURES_LIST, OnSelChangeList)
    ON_LBN_SELCANCEL(IDC_CREATURES_LIST, OnSelCancelList)
    ON_LBN_DBLCLK(IDC_CREATURES_LIST, OnDblclkList)
    ON_WM_VKEYTOITEM()
END_MESSAGE_MAP()

VA(0x0040a7fd, 0xa5)
static CString getCreatureStackText(TCreatureType creature, unsigned int quantity)
{
    CString quantityText;
    quantityText.Format("%d", quantity);
    return CString(akCreatureTypeTraits[creature].m_name) + "\t" + quantityText;
}

VA(0x0040a8a2, 0xbf)
TCreaturesDlg::TCreaturesDlg(const TBlackBox* pBlackBox, EGameVersion mapVersion)
    : CDialog(TCreaturesDlg::IDD),
      _m_pBlackBox(pBlackBox),
      _m_mapVersion(mapVersion),
      _m_bModified(false)
{
}

VA_COMPGEN(0x0040a961, 0x1c, SCALAR_DELETING_DTOR, TCreaturesDlg)
VA_COMPGEN(0x0040a97d, 0x80, IMPLICIT_DTOR, TCreaturesDlg)

VA(0x0040a9fd, 0x64)
void TCreaturesDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_CREATURES_LIST, _m_list);
    DDX_Control(pDX, IDC_CREATURES_ADD_BUTTON, _m_addButton);
    DDX_Control(pDX, IDC_CREATURES_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_CREATURES_REMOVE_BUTTON, _m_removeButton);
    DDX_Control(pDX, IDC_CREATURES_REMOVE_ALL_BUTTON, _m_removeAllButton);
}

VA(0x0040aa61, 0x1b2)
BOOL TCreaturesDlg::OnInitDialog()
{
    GetDlgItem(IDC_CREATURE_TYPE_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kCreatureTypeStaticStr);
    GetDlgItem(IDC_QUANTITY_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kQuantityStaticStr);
    GetDlgItem(IDC_CREATURES_ADD_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kAddButtonStr);
    GetDlgItem(IDC_CREATURES_EDIT_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kEditButtonStr);
    GetDlgItem(IDC_CREATURES_REMOVE_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kRemoveButtonStr);
    GetDlgItem(IDC_CREATURES_REMOVE_ALL_BUTTON)->SetWindowText(SBlackBoxPropsContentsPageText::kRemoveAllButtonStr);
    _m_bModified = false;
    _m_aCreatureStack = _m_pBlackBox->getContents().getCreatureStacks();
    CDialog::OnInitDialog();
    _m_list.SetTabStops(171);
    for (std::vector<TCreatureStack>::const_iterator it = _m_aCreatureStack.begin(); it != _m_aCreatureStack.end();
         ++it) {
        int index = _m_list.AddString(getCreatureStackText(it->getCreatureType(), it->getQuantity()));
        _m_list.SetItemData(index, MAKELONG(it->getCreatureType(), it->getQuantity()));
    }
    _m_list.SetCurSel(-1);
    _m_addButton.EnableWindow(_m_list.GetCount() < TBlackBox::TContents::s_kMaxCreatureStacks);
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_removeAllButton.EnableWindow(_m_list.GetCount() > 0);
    return TRUE;
}

VA(0x0040ac13, 0xaa)
void TCreaturesDlg::OnOK()
{
    CDialog::OnOK();
    _m_aCreatureStack.erase(_m_aCreatureStack.begin(), _m_aCreatureStack.end());
    int count = _m_list.GetCount();
    _m_aCreatureStack.reserve(count);
    for (int index = 0; index < count; index++) {
        DWORD data = _m_list.GetItemData(index);
        _m_aCreatureStack.push_back(TCreatureStack(TCreatureType(LOWORD(data)), HIWORD(data)));
    }
    _m_bModified = _m_bModified || _m_aCreatureStack != _m_pBlackBox->getContents().getCreatureStacks();
}

VA(0x0040acbd, 0x147)
void TCreaturesDlg::OnAddButton()
{
    TEditCreatureStackDlg dlg(this, _m_mapVersion, ~std::bitset<kNumCreatureTypes>(0));
    if (dlg.DoModal() == IDOK) {
        int index = _m_list.GetCurSel();
        if (index == LB_ERR)
            index = _m_list.AddString(
                getCreatureStackText(dlg.getStack().getCreatureType(), dlg.getStack().getQuantity()));
        else
            _m_list.InsertString(index,
                                 getCreatureStackText(dlg.getStack().getCreatureType(), dlg.getStack().getQuantity()));
        _m_list.SetItemData(index, MAKELONG(dlg.getStack().getCreatureType(), dlg.getStack().getQuantity()));
        if (_m_list.GetCount() >= TBlackBox::TContents::s_kMaxCreatureStacks)
            _m_addButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(TRUE);
    }
}

VA_COMPGEN(0x0040ae04, 0x56, IMPLICIT_DTOR, TEditCreatureStackDlg)

VA(0x0040ae5a, 0x138)
void TCreaturesDlg::OnEditButton()
{
    int index = _m_list.GetCurSel();
    DWORD data = _m_list.GetItemData(index);
    TCreatureStack stack(TCreatureType(LOWORD(data)), HIWORD(data));
    TEditCreatureStackDlg dlg(this, stack, _m_mapVersion, ~std::bitset<kNumCreatureTypes>(0));
    if (dlg.DoModal() == IDOK
        && (dlg.getStack().getCreatureType() != stack.getCreatureType()
            || dlg.getStack().getQuantity() != stack.getQuantity())) {
        _m_list.DeleteString(index);
        _m_list.InsertString(index, getCreatureStackText(dlg.getStack().getCreatureType(), dlg.getStack().getQuantity()));
        _m_list.SetItemData(index, MAKELONG(dlg.getStack().getCreatureType(), dlg.getStack().getQuantity()));
        _m_list.SetCurSel(index);
    }
}

VA(0x0040af92, 0xb8)
void TCreaturesDlg::OnRemoveButton()
{
    int index = _m_list.GetCurSel();
    _m_list.DeleteString(index);
    int count = _m_list.GetCount();
    if (count > 0) {
        if (count < TBlackBox::TContents::s_kMaxCreatureStacks)
            _m_addButton.EnableWindow(TRUE);
        _m_list.SetCurSel(index < count ? index : count - 1);
    } else {
        _m_addButton.EnableWindow(TRUE);
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(FALSE);
        GotoDlgCtrl(&_m_addButton);
    }
}

VA(0x0040b04a, 0x55)
void TCreaturesDlg::OnRemoveAllButton()
{
    int count = _m_list.GetCount();
    if (_m_list.GetCurSel() == LB_ERR)
        _m_list.SetCurSel(0);
    for (; count > 0; count--)
        OnRemoveButton();
}

void TCreaturesDlg::OnSelChangeList()
{
    _m_editButton.EnableWindow(TRUE);
    _m_removeButton.EnableWindow(TRUE);
}

VA(0x0040b09f, 0x1f)
void TCreaturesDlg::OnSelCancelList()
{
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
}

VA(0x0040b0be, 0x26)
void TCreaturesDlg::OnDblclkList()
{
    if (_m_list.GetCurSel() != LB_ERR)
        OnEditButton();
}

VA(0x0040b0e4, 0x47)
int TCreaturesDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    if (nKey == VK_DELETE && _m_list.GetCurSel() != LB_ERR) {
        OnRemoveButton();
        return -2;
    }
    return CDialog::OnVKeyToItem(nKey, pListBox, nIndex);
}

VA(0x0040b12b, 0x143)
TBlackBoxPropsContentsPage::TBlackBoxPropsContentsPage(const TBlackBox* pBlackBox, EGameVersion mapVersion,
                                                       UINT nIDHelp)
    : CPropertyPage(TBlackBoxPropsContentsPage::IDD),
      _m_pDialogs(NULL)
{
    m_psp.pszTitle = m_strCaption = kContentsPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
    m_nIDHelp = nIDHelp;
    _m_pDialogs = new _TDialogs(pBlackBox, mapVersion);
    if (!_m_pDialogs)
        throw TAllocationFailure();
    _m_apCategoryDlg[0] = &_m_pDialogs->m_experienceBonusDlg;
    _m_apCategoryDlg[1] = &_m_pDialogs->m_manaBonusDlg;
    _m_apCategoryDlg[2] = &_m_pDialogs->m_moraleBonusDlg;
    _m_apCategoryDlg[3] = &_m_pDialogs->m_luckBonusDlg;
    _m_apCategoryDlg[4] = &_m_pDialogs->m_resourcesDlg;
    _m_apCategoryDlg[5] = &_m_pDialogs->m_primarySkillBonusesDlg;
    _m_apCategoryDlg[6] = &_m_pDialogs->m_secondarySkillsDlg;
    _m_apCategoryDlg[7] = &_m_pDialogs->m_artifactsDlg;
    _m_apCategoryDlg[8] = &_m_pDialogs->m_spellsDlg;
    _m_apCategoryDlg[9] = &_m_pDialogs->m_creaturesDlg;
}

VA(0x0040b26e, 0xef)
TBlackBoxPropsContentsPage::_TDialogs::_TDialogs(const TBlackBox* pBlackBox, EGameVersion mapVersion)
    : m_experienceBonusDlg(pBlackBox),
      m_manaBonusDlg(pBlackBox),
      m_moraleBonusDlg(pBlackBox),
      m_luckBonusDlg(pBlackBox),
      m_resourcesDlg(pBlackBox),
      m_primarySkillBonusesDlg(pBlackBox),
      m_secondarySkillsDlg(pBlackBox),
      m_artifactsDlg(pBlackBox, mapVersion),
      m_spellsDlg(pBlackBox),
      m_creaturesDlg(pBlackBox, mapVersion)
{
}

VA_COMPGEN(0x0040b35d, 0x1c, SCALAR_DELETING_DTOR, TBlackBoxPropsContentsPage)

VA(0x0040b379, 0x5f)
TBlackBoxPropsContentsPage::~TBlackBoxPropsContentsPage()
{
    delete _m_pDialogs;
}

VA_COMPGEN(0x0040b3d8, 0xb3, IMPLICIT_DTOR, _TDialogs)

VA(0x0040b48b, 0xd)
int TBlackBoxPropsContentsPage::getExperienceBonus() const
{
    return _m_pDialogs->m_experienceBonusDlg.getExperienceBonus();
}

VA(0x0040b498, 0xd)
int TBlackBoxPropsContentsPage::getManaBonus() const
{
    return _m_pDialogs->m_manaBonusDlg.getManaBonus();
}

VA(0x0040b4a5, 0xd)
int TBlackBoxPropsContentsPage::getMoraleBonus() const
{
    return _m_pDialogs->m_moraleBonusDlg.getMoraleBonus();
}

VA(0x0040b4b2, 0xd)
int TBlackBoxPropsContentsPage::getLuckBonus() const
{
    return _m_pDialogs->m_luckBonusDlg.getLuckBonus();
}

VA(0x0040b4bf, 0x12)
const TResourceQuantities& TBlackBoxPropsContentsPage::getResourceQuantities() const
{
    return _m_pDialogs->m_resourcesDlg.getResourceQuantities();
}

VA(0x0040b4d1, 0xc)
const TPrimarySkillBonuses& TBlackBoxPropsContentsPage::getPrimarySkillBonuses() const
{
    return _m_pDialogs->m_primarySkillBonusesDlg.getPrimarySkillBonuses();
}

VA(0x0040b4dd, 0xc)
const std::vector<TSecondarySkillRecord>& TBlackBoxPropsContentsPage::getSecondarySkills() const
{
    return _m_pDialogs->m_secondarySkillsDlg.getSecondarySkills();
}

VA(0x0040b4e9, 0xc)
const std::vector<TArtifact>& TBlackBoxPropsContentsPage::getArtifacts() const
{
    return _m_pDialogs->m_artifactsDlg.getArtifacts();
}

VA(0x0040b4f5, 0xc)
const std::vector<SpellID>& TBlackBoxPropsContentsPage::getSpells() const
{
    return _m_pDialogs->m_spellsDlg.getSpells();
}

VA(0x0040b501, 0xc)
const std::vector<TCreatureStack>& TBlackBoxPropsContentsPage::getCreatureStacks() const
{
    return _m_pDialogs->m_creaturesDlg.getCreatureStacks();
}

VA(0x0040b50d, 0x62)
bool TBlackBoxPropsContentsPage::wasModified() const
{
    return _m_pDialogs->m_experienceBonusDlg.wasModified() || _m_pDialogs->m_manaBonusDlg.wasModified()
           || _m_pDialogs->m_moraleBonusDlg.wasModified() || _m_pDialogs->m_luckBonusDlg.wasModified()
           || _m_pDialogs->m_resourcesDlg.wasModified() || _m_pDialogs->m_primarySkillBonusesDlg.wasModified()
           || _m_pDialogs->m_secondarySkillsDlg.wasModified() || _m_pDialogs->m_artifactsDlg.wasModified()
           || _m_pDialogs->m_spellsDlg.wasModified() || _m_pDialogs->m_creaturesDlg.wasModified();
}

VA(0x0040b56f, 0x18)
void TBlackBoxPropsContentsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_CATEGORY_COMBO, _m_categoryCombo);
}

VA(0x0040b587, 0x6)
BEGIN_MESSAGE_MAP(TBlackBoxPropsContentsPage, CPropertyPage)
    ON_CBN_SELCHANGE(IDC_CATEGORY_COMBO, OnSelChangeCategoryCombo)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

VA(0x0040b58d, 0x138)
int TBlackBoxPropsContentsPage::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CPropertyPage::OnCreate(lpCreateStruct) == -1
        || !_m_pDialogs->m_experienceBonusDlg.Create(TExperienceBonusDlg::IDD, this)
        || !_m_pDialogs->m_manaBonusDlg.Create(TManaBonusDlg::IDD, this)
        || !_m_pDialogs->m_moraleBonusDlg.Create(TMoraleBonusDlg::IDD, this)
        || !_m_pDialogs->m_luckBonusDlg.Create(TLuckBonusDlg::IDD, this)
        || !_m_pDialogs->m_resourcesDlg.Create(TResourcesDlg::IDD, this)
        || !_m_pDialogs->m_primarySkillBonusesDlg.Create(TPrimarySkillBonusesDlg::IDD, this)
        || !_m_pDialogs->m_secondarySkillsDlg.Create(TSecondarySkillsDlg::IDD, this)
        || !_m_pDialogs->m_artifactsDlg.Create(TArtifactsDlg::IDD, this)
        || !_m_pDialogs->m_spellsDlg.Create(TSpellsDlg::IDD, this)
        || !_m_pDialogs->m_creaturesDlg.Create(TCreaturesDlg::IDD, this))
        return -1;
    return 0;
}

VA(0x0040b6c5, 0xd4)
void TBlackBoxPropsContentsPage::OnDestroy()
{
    _m_pDialogs->m_creaturesDlg.DestroyWindow();
    _m_pDialogs->m_spellsDlg.DestroyWindow();
    _m_pDialogs->m_artifactsDlg.DestroyWindow();
    _m_pDialogs->m_secondarySkillsDlg.DestroyWindow();
    _m_pDialogs->m_primarySkillBonusesDlg.DestroyWindow();
    _m_pDialogs->m_resourcesDlg.DestroyWindow();
    _m_pDialogs->m_luckBonusDlg.DestroyWindow();
    _m_pDialogs->m_moraleBonusDlg.DestroyWindow();
    _m_pDialogs->m_manaBonusDlg.DestroyWindow();
    _m_pDialogs->m_experienceBonusDlg.DestroyWindow();
    CPropertyPage::OnDestroy();
}

static CString getCategoryName(unsigned int category);

VA(0x0040b799, 0x12e)
BOOL TBlackBoxPropsContentsPage::OnInitDialog()
{
    GetDlgItem(IDC_CATEGORY_STATIC)->SetWindowText(SBlackBoxPropsContentsPageText::kCategoryStaticStr);
    _m_category = 0;
    CPropertyPage::OnInitDialog();
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    CWnd* pInsertAfter = pFrame;
    for (unsigned int category = 0; category < s_kNumCategories; category++) {
        int index = _m_categoryCombo.AddString(getCategoryName(category));
        _m_categoryCombo.SetItemData(index, category);
        _m_apCategoryDlg[category]->SetWindowPos(pInsertAfter, rect.left, rect.top, 0, 0, SWP_NOSIZE);
        pInsertAfter = _m_apCategoryDlg[category];
    }
    pFrame->DestroyWindow();
    int index = 0;
    while (_m_categoryCombo.GetItemData(index) != _m_category)
        index++;
    _m_categoryCombo.SetCurSel(index);
    _m_apCategoryDlg[_m_category]->ShowWindow(SW_SHOW);
    return TRUE;
}

VA(0x0040b8c7, 0xa7)
static CString getCategoryName(unsigned int category)
{
    DATA_COMPGEN_GUARD(0x005aa700, categoryNamesGuard, akCategoryNames)
    DATA(0x0059a668) static const char* const akCategoryNames[TBlackBoxPropsContentsPage::s_kNumCategories] = {
        kExperienceStr,
        kManaStr,
        kMoraleStr,
        kLuckStr,
        kResourcesStr,
        kPrimarySkillsStr,
        kSecondarySkillsStr,
        kArtifactsStr,
        kSpellsStr,
        kCreaturesStr
    };
    return CString(akCategoryNames[category]);
}

VA(0x0040b96e, 0xf0)
void TBlackBoxPropsContentsPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_pDialogs->m_experienceBonusDlg.OnOK();
    _m_pDialogs->m_manaBonusDlg.OnOK();
    _m_pDialogs->m_moraleBonusDlg.OnOK();
    _m_pDialogs->m_luckBonusDlg.OnOK();
    _m_pDialogs->m_resourcesDlg.OnOK();
    _m_pDialogs->m_primarySkillBonusesDlg.OnOK();
    _m_pDialogs->m_secondarySkillsDlg.OnOK();
    _m_pDialogs->m_artifactsDlg.OnOK();
    _m_pDialogs->m_spellsDlg.OnOK();
    _m_pDialogs->m_creaturesDlg.OnOK();
}

VA(0x0040ba5e, 0x56)
void TBlackBoxPropsContentsPage::OnSelChangeCategoryCombo()
{
    _m_apCategoryDlg[_m_category]->ShowWindow(SW_HIDE);
    _m_category = _m_categoryCombo.GetItemData(_m_categoryCombo.GetCurSel());
    _m_apCategoryDlg[_m_category]->ShowWindow(SW_SHOW);
}
