// EditQuestDlg.cpp - the quest editor (h3maped 0x40e4f5..0x412cfa; GOG
// only): a radio per quest kind and the kind's page. A page reports when
// its quest becomes valid; the editor enables OK with the shown page's
// validity. The defeat quests list the heroes and monsters on the map,
// the bring quests their artifacts, creature stacks (one stack per type)
// and resources, a hero quest the version's heroes of a class.
#include "editor/stdafx.h"

#include <stdio.h>

#include <bitset>
#include <map>
#include <memory>
#include <set>

#include "adventureobjecttype.h"
#include "artifact.h"
#include "exceptions.h"
#include "objnames.h"
#include "retailobjecttype.h"
#include "va.h"
#include "editor/Army.h"
#include "editor/Clamp.h"
#include "editor/DialogTemplate.h"
#include "editor/Digits.h"
#include "editor/EditCreatureStackDlg.h"
#include "editor/EditQuestDlg.h"
#include "editor/FormattedString.h"
#include "editor/GameMap.h"
#include "editor/GameResource.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/Monster.h"
#include "editor/Player.h"
#include "editor/SelectArtifactDlg.h"
#include "editor/Town.h"

namespace {

// No quest: an empty page.
class TQuestNoneDlg : public TQuestTypeDlg {
public:
    TQuestNoneDlg(TClient* pClient) : TQuestTypeDlg(pClient, true) {}

    virtual BOOL create(CWnd* pParent);
    virtual std::auto_ptr<TQuest> getQuest() const { return std::auto_ptr<TQuest>(NULL); }
};

// Reach an experience level, 1..s_kMaxLevel.
class TQuestAchieveExperienceLevelDlg : public TQuestTypeDlg {
public:
    enum { IDD = IDD_QUEST_EXPERIENCE_LEVEL, s_kMaxLevel = 99 };
    TQuestAchieveExperienceLevelDlg(TClient* pClient, EGameVersion mapVersion)
        : TQuestTypeDlg(pClient, false), _m_mapVersion(mapVersion) {}

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual bool isAvailable() const { return _m_mapVersion >= GAME_VERSION_AB; }
    virtual std::auto_ptr<TQuest> getQuest() const;
    void setQuest(const TQuestAchieveExperienceLevel& quest);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeLevelEdit();
    afx_msg void OnKillFocusLevelEdit();
    DECLARE_MESSAGE_MAP()

private:
    EGameVersion _m_mapVersion;
    int _m_level;
    CEdit _m_levelEdit;
    CSpinButtonCtrl _m_levelSpin;
};

// Reach a level in some primary skills, each 0..s_kMaxLevel; valid while
// one skill is above 0.
class TQuestAchievePrimarySkillLevelDlg : public TQuestTypeDlg {
public:
    enum { IDD = IDD_QUEST_PRIMARY_SKILL_LEVEL, s_kMaxLevel = kMaxPrimarySkillLevel };
    TQuestAchievePrimarySkillLevelDlg(TClient* pClient, EGameVersion mapVersion)
        : TQuestTypeDlg(pClient, false), _m_mapVersion(mapVersion), _m_bCreated(false) {}

    virtual void OnOK() {}
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual bool isAvailable() const { return _m_mapVersion >= GAME_VERSION_AB; }
    virtual std::auto_ptr<TQuest> getQuest() const;
    void setQuest(const TQuestAchievePrimarySkillLevel& quest);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnChangeAttackEdit();
    afx_msg void OnChangeDefenseEdit();
    afx_msg void OnChangeSpellPowerEdit();
    afx_msg void OnChangeKnowledgeEdit();
    DECLARE_MESSAGE_MAP()

private:
    struct _TSkillControls {
        CEdit m_edit;
        CSpinButtonCtrl m_spin;
    };

    void _onLevelChange(int skill);

    EGameVersion _m_mapVersion;
    unsigned int _m_numSkills;
    TArray<int, kNumPrimarySkills> _m_aLevel;
    _TSkillControls _m_aSkillControls[kNumPrimarySkills];
    bool _m_bCreated;
};

// Defeat a hero on the map, a placed hero or one visiting a town.
class TQuestDefeatHeroDlg : public TQuestTypeDlg {
public:
    enum { IDD = IDD_QUEST_DEFEAT_HERO };
    TQuestDefeatHeroDlg(TClient* pClient, const TGameMap* pMap) : TQuestTypeDlg(pClient, false), _m_pMap(pMap) {}

    virtual void OnOK()
    {
        CDialog::OnOK();
        _m_heroLinkID = _m_heroCombo.GetItemData(_m_heroCombo.GetCurSel());
    }
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual bool isAvailable() const;
    virtual std::auto_ptr<TQuest> getQuest() const;
    void setQuest(const TQuestDefeatHero& quest)
    {
        _m_heroLinkID = quest.getHeroLinkID();
        int index;
        for (index = 0; _m_heroCombo.GetItemData(index) != _m_heroLinkID; index++)
            ;
        _m_heroCombo.SetCurSel(index);
    }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    void _addHero(const THero* pHero, bool bSecondLayer, const TTilePoint& loc);

    const TGameMap* _m_pMap;
    unsigned int _m_heroLinkID;
    CComboBox _m_heroCombo;
};

// Defeat a monster on the map.
class TQuestDefeatMonsterDlg : public TQuestTypeDlg {
public:
    enum { IDD = IDD_QUEST_DEFEAT_MONSTER };
    TQuestDefeatMonsterDlg(TClient* pClient, const TGameMap* pMap) : TQuestTypeDlg(pClient, false), _m_pMap(pMap) {}

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual bool isAvailable() const
    {
        return _m_pMap->getVersion() >= GAME_VERSION_AB && _m_monsterCombo.GetCount() > 0;
    }
    virtual std::auto_ptr<TQuest> getQuest() const;
    void setQuest(const TQuestDefeatMonster& quest)
    {
        _m_monsterLinkID = quest.getMonsterLinkID();
        int index;
        for (index = 0; _m_monsterCombo.GetItemData(index) != _m_monsterLinkID; index++)
            ;
        _m_monsterCombo.SetCurSel(index);
    }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    void _addMonster(const TMonster* pMonster, bool bSecondLayer, const TTilePoint& loc);

    const TGameMap* _m_pMap;
    unsigned int _m_monsterLinkID;
    CComboBox _m_monsterCombo;
};

// Bring artifacts, at most _getMaxArtifacts().
class TQuestBringArtifactsDlg : public TQuestTypeDlg {
public:
    enum { IDD = IDD_QUEST_BRING_ARTIFACTS };
    TQuestBringArtifactsDlg(TClient* pClient, EGameVersion mapVersion)
        : TQuestTypeDlg(pClient, false), _m_mapVersion(mapVersion) {}

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TQuest> getQuest() const;
    void setQuest(const TQuestBringArtifacts& quest);

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
    // Restoration of Erathia quests ask for one artifact.
    unsigned int _getMaxArtifacts() const { return _m_mapVersion < GAME_VERSION_AB ? 1 : 8; }

    EGameVersion _m_mapVersion;
    std::multiset<TArtifact> _m_artifacts;
    CListBox _m_list;
    CButton _m_addButton;
    CButton _m_editButton;
    CButton _m_removeButton;
    CButton _m_removeAllButton;
};

// Bring creature stacks, at most s_kMaxStacks, one per creature type.
class TQuestBringCreaturesDlg : public TQuestTypeDlg {
public:
    enum { IDD = IDD_QUEST_BRING_CREATURES, s_kMaxStacks = 7 };

    TQuestBringCreaturesDlg(TClient* pClient, EGameVersion mapVersion);

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual bool isAvailable() const { return _m_mapVersion >= GAME_VERSION_AB; }
    virtual std::auto_ptr<TQuest> getQuest() const;
    void setQuest(const TQuestBringCreatures& quest);

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
    EGameVersion _m_mapVersion;
    TQuestBringCreatures::TCreatures _m_creatures;
    std::bitset<kNumCreatureTypes> _m_abAvailableCreature;
    CListBox _m_list;
    CButton _m_addButton;
    CButton _m_editButton;
    CButton _m_removeButton;
    CButton _m_removeAllButton;
};

// Bring resources, each 0..UD_MAXVAL; valid while one is above 0.
class TQuestBringResourcesDlg : public TQuestTypeDlg {
public:
    enum { IDD = IDD_QUEST_BRING_RESOURCES };
    TQuestBringResourcesDlg(TClient* pClient, EGameVersion mapVersion)
        : TQuestTypeDlg(pClient, false), _m_mapVersion(mapVersion), _m_bCreated(false) {}

    virtual void OnOK() {}
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual bool isAvailable() const { return _m_mapVersion >= GAME_VERSION_AB; }
    virtual std::auto_ptr<TQuest> getQuest() const;
    void setQuest(const TQuestBringResources& quest);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnChangeRes1Edit();
    afx_msg void OnChangeRes2Edit();
    afx_msg void OnChangeRes3Edit();
    afx_msg void OnChangeRes4Edit();
    afx_msg void OnChangeRes5Edit();
    afx_msg void OnChangeRes6Edit();
    afx_msg void OnChangeRes7Edit();
    DECLARE_MESSAGE_MAP()

private:
    struct _TResourceControls {
        CEdit m_edit;
        CSpinButtonCtrl m_spin;
    };

    void _onQuantityChange(int type);

    EGameVersion _m_mapVersion;
    unsigned int _m_numTypes;
    TArray<int, kNumGameResourceTypes> _m_aQuantity;
    _TResourceControls _m_aResourceControls[kNumGameResourceTypes];
    bool _m_bCreated;
};

// Be a hero: a class, then one of the version's heroes of the class.
class TQuestBeASpecificHeroDlg : public TQuestTypeDlg {
public:
    enum { IDD = IDD_QUEST_BE_A_SPECIFIC_HERO };
    TQuestBeASpecificHeroDlg(TClient* pClient, const TGameMap* pMap) : TQuestTypeDlg(pClient, true), _m_pMap(pMap) {}

    virtual void OnOK();
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual bool isAvailable() const { return _m_pMap->getVersion() >= GAME_VERSION_SOD; }
    virtual std::auto_ptr<TQuest> getQuest() const;
    void setQuest(const TQuestBeASpecificHero& quest);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnSelChangeClassCombo();
    DECLARE_MESSAGE_MAP()

private:
    const TGameMap* _m_pMap;
    THeroID _m_heroID;
    CComboBox _m_classCombo;
    CComboBox _m_identityCombo;
};

// Belong to a player.
class TQuestBelongToASpecificPlayerDlg : public TQuestTypeDlg {
public:
    enum { IDD = IDD_QUEST_BELONG_TO_A_SPECIFIC_PLAYER };
    TQuestBelongToASpecificPlayerDlg(TClient* pClient, EGameVersion mapVersion)
        : TQuestTypeDlg(pClient, true), _m_mapVersion(mapVersion) {}

    virtual void OnOK()
    {
        CDialog::OnOK();
        _m_player = TPlayer(_m_playerCombo.GetItemData(_m_playerCombo.GetCurSel()));
    }
    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual bool isAvailable() const { return _m_mapVersion >= GAME_VERSION_SOD; }
    virtual std::auto_ptr<TQuest> getQuest() const;
    void setQuest(const TQuestBelongToASpecificPlayer& quest);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    EGameVersion _m_mapVersion;
    TPlayer _m_player;
    CComboBox _m_playerCombo;
};

}

// The editor's pages, allocated together, one per quest kind in radio
// order.
struct TEditQuestDlg::_TPages {
    _TPages(TQuestTypeDlg::TClient* pClient, const TGameMap* pMap);

    TQuestNoneDlg m_noneDlg;
    TQuestAchieveExperienceLevelDlg m_experienceLevelDlg;
    TQuestAchievePrimarySkillLevelDlg m_primarySkillLevelDlg;
    TQuestDefeatHeroDlg m_defeatHeroDlg;
    TQuestDefeatMonsterDlg m_defeatMonsterDlg;
    TQuestBringArtifactsDlg m_bringArtifactsDlg;
    TQuestBringCreaturesDlg m_bringCreaturesDlg;
    TQuestBringResourcesDlg m_bringResourcesDlg;
    TQuestBeASpecificHeroDlg m_beASpecificHeroDlg;
    TQuestBelongToASpecificPlayerDlg m_belongToASpecificPlayerDlg;
    TQuestTypeDlg* m_apDlg[s_kNumQuestTypes];
};

VA(0x0040e6c9, 0x1c)
void TQuestTypeDlg::_setValid(bool bValid)
{
    if (bValid != _m_bValid) {
        _m_bValid = bValid;
        _m_pClient->onValidityChanged(this, bValid);
    }
}

VA(0x0040e6e5, 0x26)
BOOL TQuestNoneDlg::create(CWnd* pParent)
{
    TEmptyDialogTemplate dialogTemplate;
    return CreateIndirect(&dialogTemplate, pParent);
}

VA(0x0040e739, 0x6)
BEGIN_MESSAGE_MAP(TQuestAchieveExperienceLevelDlg, TQuestTypeDlg)
    ON_EN_CHANGE(IDC_EXPERIENCE_LEVEL_EDIT, OnChangeLevelEdit)
    ON_EN_KILLFOCUS(IDC_EXPERIENCE_LEVEL_EDIT, OnKillFocusLevelEdit)
END_MESSAGE_MAP()

VA(0x0040e73f, 0x52)
void TQuestAchieveExperienceLevelDlg::setQuest(const TQuestAchieveExperienceLevel& quest)
{
    _m_level = quest.getLevel();
    TFormattedString text("%d", _m_level);
    _m_levelEdit.SetWindowText(text);
}

VA(0x0040e791, 0x9d)
std::auto_ptr<TQuest> TQuestAchieveExperienceLevelDlg::getQuest() const
{
    std::auto_ptr<TQuest> pQuest(new TQuestAchieveExperienceLevel(_m_level));
    if (!pQuest.get())
        throw TAllocationFailure();
    return pQuest;
}

VA(0x0040e82e, 0x2e)
void TQuestAchieveExperienceLevelDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_EXPERIENCE_LEVEL_EDIT, _m_levelEdit);
    DDX_Control(pDX, IDC_EXPERIENCE_LEVEL_SPIN, _m_levelSpin);
}

VA(0x0040e85c, 0x76)
BOOL TQuestAchieveExperienceLevelDlg::OnInitDialog()
{
    GetDlgItem(IDC_EXPERIENCE_LEVEL_STATIC)->SetWindowText(SEditQuestDlgText::kExperienceLevelStaticStr);
    _m_level = 1;
    _setValid(true);
    CDialog::OnInitDialog();
    _m_levelEdit.LimitText(TDigits<s_kMaxLevel>::getDigits());
    _m_levelSpin.SetRange(1, s_kMaxLevel);
    _m_levelEdit.SetWindowText("1");
    return TRUE;
}

VA(0x0040e8d2, 0x7e)
void TQuestAchieveExperienceLevelDlg::OnOK()
{
    CDialog::OnOK();
    CString text;
    _m_levelEdit.GetWindowText(text);
    sscanf(text, "%d", &_m_level);
    _m_level = clamp(1, _m_level, int(s_kMaxLevel));
}

VA(0x0040e950, 0x79)
void TQuestAchieveExperienceLevelDlg::OnChangeLevelEdit()
{
    if (_m_levelEdit.m_hWnd != NULL) {
        int level = 0;
        CString text;
        _m_levelEdit.GetWindowText(text);
        sscanf(text, "%d", &level);
        _setValid(level >= 1 && level <= s_kMaxLevel);
    }
}

VA(0x0040e9c9, 0xb0)
void TQuestAchieveExperienceLevelDlg::OnKillFocusLevelEdit()
{
    int level = 0;
    CString text;
    _m_levelEdit.GetWindowText(text);
    sscanf(text, "%d", &level);
    if (level < 1 || level > s_kMaxLevel) {
        level = clamp(1, level, int(s_kMaxLevel));
        text.Format("%d", level);
        _m_levelEdit.SetWindowText(text);
        _setValid(true);
    }
}

VA(0x0040ea79, 0x8)
void TQuestAchievePrimarySkillLevelDlg::OnChangeAttackEdit()
{
    _onLevelChange(ePriSkillAttack);
}

VA(0x0040ea81, 0x8)
void TQuestAchievePrimarySkillLevelDlg::OnChangeDefenseEdit()
{
    _onLevelChange(ePriSkillDefense);
}

VA(0x0040ea89, 0x8)
void TQuestAchievePrimarySkillLevelDlg::OnChangeSpellPowerEdit()
{
    _onLevelChange(ePriSkillPower);
}

VA(0x0040ea91, 0x8)
void TQuestAchievePrimarySkillLevelDlg::OnChangeKnowledgeEdit()
{
    _onLevelChange(ePriSkillKnowledge);
}

VA(0x0040ea99, 0x6)
BEGIN_MESSAGE_MAP(TQuestAchievePrimarySkillLevelDlg, TQuestTypeDlg)
    ON_WM_DESTROY()
    ON_EN_CHANGE(IDC_ATTACK_EDIT, OnChangeAttackEdit)
    ON_EN_CHANGE(IDC_DEFENSE_EDIT, OnChangeDefenseEdit)
    ON_EN_CHANGE(IDC_SPELL_POWER_EDIT, OnChangeSpellPowerEdit)
    ON_EN_CHANGE(IDC_KNOWLEDGE_EDIT, OnChangeKnowledgeEdit)
END_MESSAGE_MAP()

struct TSkillControlIDs {
    int m_editID;
    int m_spinID;
};

DATA(0x005321d0)
static const TSkillControlIDs g_aSkillControlIDs[kNumPrimarySkills] = {
    { IDC_ATTACK_EDIT, IDC_ATTACK_SPIN },
    { IDC_DEFENSE_EDIT, IDC_DEFENSE_SPIN },
    { IDC_SPELL_POWER_EDIT, IDC_SPELL_POWER_SPIN },
    { IDC_KNOWLEDGE_EDIT, IDC_KNOWLEDGE_SPIN }
};

VA(0x0040ea9f, 0xa2)
void TQuestAchievePrimarySkillLevelDlg::setQuest(const TQuestAchievePrimarySkillLevel& quest)
{
    _m_aLevel = quest.getLevels();
    _m_numSkills = 0;
    for (int skill = 0; skill < kNumPrimarySkills; skill++) {
        if (_m_aLevel[skill] > 0)
            _m_numSkills++;
        TFormattedString text("%d", _m_aLevel[skill]);
        _m_aSkillControls[skill].m_edit.SetWindowText(text);
    }
    _setValid(_m_numSkills > 0);
}

VA(0x0040eb41, 0x9e)
std::auto_ptr<TQuest> TQuestAchievePrimarySkillLevelDlg::getQuest() const
{
    std::auto_ptr<TQuest> pQuest(new TQuestAchievePrimarySkillLevel(_m_aLevel));
    if (!pQuest.get())
        throw TAllocationFailure();
    return pQuest;
}

VA(0x0040ebdf, 0x4d)
void TQuestAchievePrimarySkillLevelDlg::DoDataExchange(CDataExchange* pDX)
{
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++) {
        DDX_Control(pDX, g_aSkillControlIDs[skill].m_editID, _m_aSkillControls[skill].m_edit);
        DDX_Control(pDX, g_aSkillControlIDs[skill].m_spinID, _m_aSkillControls[skill].m_spin);
    }
    if (!pDX->m_bSaveAndValidate)
        _m_bCreated = true;
}

VA(0x0040ec2c, 0xc)
void TQuestAchievePrimarySkillLevelDlg::OnDestroy()
{
    _m_bCreated = false;
    TQuestTypeDlg::OnDestroy();
}

VA(0x0040ec38, 0xce)
BOOL TQuestAchievePrimarySkillLevelDlg::OnInitDialog()
{
    GetDlgItem(IDC_ATTACK_STATIC)->SetWindowText(SEditQuestDlgText::kAttackSkillStaticStr);
    GetDlgItem(IDC_DEFENSE_STATIC)->SetWindowText(SEditQuestDlgText::kDefenseSkillStaticStr);
    GetDlgItem(IDC_SPELL_POWER_STATIC)->SetWindowText(SEditQuestDlgText::kSpellPowerStaticStr);
    GetDlgItem(IDC_KNOWLEDGE_STATIC)->SetWindowText(SEditQuestDlgText::kKnowledgeStaticStr);
    int skill;
    for (skill = 0; skill < kNumPrimarySkills; skill++)
        _m_aLevel[skill] = 0;
    _m_numSkills = 0;
    _setValid(false);
    CDialog::OnInitDialog();
    for (skill = 0; skill < kNumPrimarySkills; skill++) {
        _m_aSkillControls[skill].m_edit.LimitText(TDigits<s_kMaxLevel>::getDigits());
        _m_aSkillControls[skill].m_edit.SetWindowText("0");
        _m_aSkillControls[skill].m_spin.SetRange(0, s_kMaxLevel);
    }
    return TRUE;
}

VA(0x0040ed07, 0xa5)
void TQuestAchievePrimarySkillLevelDlg::_onLevelChange(int skill)
{
    if (_m_bCreated) {
        int level = 0;
        CString text;
        _m_aSkillControls[skill].m_edit.GetWindowText(text);
        sscanf(text, "%d", &level);
        int oldLevel = _m_aLevel[skill];
        _m_aLevel[skill] = level;
        if (level == 0) {
            if (oldLevel != 0 && --_m_numSkills == 0)
                _setValid(false);
        } else if (oldLevel == 0 && _m_numSkills++ == 0)
            _setValid(true);
    }
}

VA(0x0040edac, 0x32)
bool TQuestDefeatHeroDlg::isAvailable() const
{
    return _m_pMap->getVersion() >= GAME_VERSION_AB && _m_heroCombo.GetCount() > 0;
}

VA(0x0040edde, 0x9d)
std::auto_ptr<TQuest> TQuestDefeatHeroDlg::getQuest() const
{
    std::auto_ptr<TQuest> pQuest(new TQuestDefeatHero(_m_heroLinkID));
    if (!pQuest.get())
        throw TAllocationFailure();
    return pQuest;
}

VA(0x0040ee7b, 0x144)
void TQuestDefeatHeroDlg::_addHero(const THero* pHero, bool bSecondLayer, const TTilePoint& loc)
{
    CString heroName;
    const TIdentifiedHero* pIdentifiedHero = dynamic_cast<const TIdentifiedHero*>(pHero);
    if (pIdentifiedHero != NULL) {
        const std::string& name = pIdentifiedHero->getBCustomName()
                                      ? pIdentifiedHero->getName()
                                      : _m_pMap->getHeroPrototype(pIdentifiedHero->getHeroID()).getName();
        heroName.Format(kSpecificHeroAndClassFmtStr, name.c_str(),
                        THero::s_akClassTraits[pIdentifiedHero->getHeroClass()].m_name);
    } else if (pHero->getBCustomName())
        heroName.Format(kSpecificHeroAndClassFmtStr, pHero->getName().c_str(),
                        akAdvObjectTypeTraits[RANDOM_HERO].m_name);
    else
        heroName = akAdvObjectTypeTraits[RANDOM_HERO].m_name;
    TFormattedString item(kObjectAtLocationFmtStr, (LPCTSTR)heroName, loc.x(), loc.y(), bSecondLayer);
    int index = _m_heroCombo.AddString(item);
    _m_heroCombo.SetItemData(index, pHero->getLinkID());
}

VA(0x0040efbf, 0x15)
void TQuestDefeatHeroDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_HERO_COMBO, _m_heroCombo);
}

VA(0x0040efd4, 0x1a2)
BOOL TQuestDefeatHeroDlg::OnInitDialog()
{
    GetDlgItem(IDC_QUEST_HERO_STATIC)->SetWindowText(SEditQuestDlgText::kHeroStaticStr);
    CDialog::OnInitDialog();
    unsigned int numLayers = _m_pMap->isTwoLayer() ? 2 : 1;
    for (unsigned int layerNum = 0; layerNum < numLayers; layerNum++) {
        const TGameMap::TLayer& layer = _m_pMap->getLayer(layerNum);
        for (TGameMap::TLayer::TObjectIDIter it = layer.objectIDBegin(); it != layer.objectIDEnd(); ++it) {
            const TGameObject* pObject = layer.getPObject(*it);
            const THero* pHero = dynamic_cast<const THero*>(pObject);
            if (pHero != NULL)
                _addHero(pHero, layerNum != 0, layer.getObjectLoc(*it) - pHero->getTriggerLoc());
            else {
                const TTown* pTown = dynamic_cast<const TTown*>(pObject);
                if (pTown != NULL && pTown->getPVisitingHero() != NULL)
                    _addHero(pTown->getPVisitingHero(), layerNum != 0,
                             layer.getObjectLoc(*it) - pTown->getTriggerLoc());
            }
        }
    }
    if (_m_heroCombo.GetCount() > 0) {
        _m_heroCombo.SetCurSel(0);
        _setValid(true);
    }
    return TRUE;
}

VA(0x0040f176, 0x9d)
std::auto_ptr<TQuest> TQuestDefeatMonsterDlg::getQuest() const
{
    std::auto_ptr<TQuest> pQuest(new TQuestDefeatMonster(_m_monsterLinkID));
    if (!pQuest.get())
        throw TAllocationFailure();
    return pQuest;
}

VA(0x0040f213, 0xbf)
void TQuestDefeatMonsterDlg::_addMonster(const TMonster* pMonster, bool bSecondLayer, const TTilePoint& loc)
{
    const char* monsterName;
    if (pMonster->getType() == MONSTER)
        monsterName = getArmyName(pMonster->getCreatureType(), pMonster->getQuantity());
    else
        monsterName = pMonster->getQuantity() == 1 ? kRandomMonsterStr : kRandomMonstersStr;
    TFormattedString item(kObjectAtLocationFmtStr, monsterName, loc.x(), loc.y(), bSecondLayer);
    int index = _m_monsterCombo.AddString(item);
    _m_monsterCombo.SetItemData(index, pMonster->getLinkID());
}

VA(0x0040f2d2, 0x15)
void TQuestDefeatMonsterDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_MONSTER_COMBO, _m_monsterCombo);
}

VA(0x0040f2e7, 0x133)
BOOL TQuestDefeatMonsterDlg::OnInitDialog()
{
    GetDlgItem(IDC_QUEST_MONSTER_STATIC)->SetWindowText(SEditQuestDlgText::kMonsterStaticStr);
    CDialog::OnInitDialog();
    unsigned int numLayers = _m_pMap->isTwoLayer() ? 2 : 1;
    for (unsigned int layerNum = 0; layerNum < numLayers; layerNum++) {
        const TGameMap::TLayer& layer = _m_pMap->getLayer(layerNum);
        for (TGameMap::TLayer::TObjectIDIter it = layer.objectIDBegin(); it != layer.objectIDEnd(); ++it) {
            const TMonster* pMonster = dynamic_cast<const TMonster*>(layer.getPObject(*it));
            if (pMonster != NULL)
                _addMonster(pMonster, layerNum != 0, layer.getObjectLoc(*it) - pMonster->getTriggerLoc());
        }
    }
    if (_m_monsterCombo.GetCount() > 0) {
        _m_monsterCombo.SetCurSel(0);
        _setValid(true);
    }
    return TRUE;
}

VA(0x0040f41a, 0x36)
void TQuestDefeatMonsterDlg::OnOK()
{
    CDialog::OnOK();
    _m_monsterLinkID = _m_monsterCombo.GetItemData(_m_monsterCombo.GetCurSel());
}

VA(0x0040f450, 0x6)
BEGIN_MESSAGE_MAP(TQuestBringArtifactsDlg, TQuestTypeDlg)
    ON_BN_CLICKED(IDC_QUEST_ADD_BUTTON, OnAddButton)
    ON_BN_CLICKED(IDC_QUEST_EDIT_BUTTON, OnEditButton)
    ON_BN_CLICKED(IDC_QUEST_REMOVE_BUTTON, OnRemoveButton)
    ON_BN_CLICKED(IDC_QUEST_REMOVE_ALL_BUTTON, OnRemoveAllButton)
    ON_LBN_SELCHANGE(IDC_ARTIFACT_LIST, OnSelChangeList)
    ON_LBN_SELCANCEL(IDC_ARTIFACT_LIST, OnSelCancelList)
    ON_LBN_DBLCLK(IDC_ARTIFACT_LIST, OnDblclkList)
    ON_WM_VKEYTOITEM()
END_MESSAGE_MAP()

VA(0x0040f456, 0xf0)
void TQuestBringArtifactsDlg::setQuest(const TQuestBringArtifacts& quest)
{
    _m_list.ResetContent();
    _m_artifacts = quest.getArtifacts();
    for (std::multiset<TArtifact>::const_iterator it = _m_artifacts.begin(); it != _m_artifacts.end(); ++it)
        _m_list.SetItemData(_m_list.AddString(akArtifactTraits[*it].m_name), *it);
    int count = _m_list.GetCount();
    _m_addButton.EnableWindow(unsigned(count) < _getMaxArtifacts());
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_removeAllButton.EnableWindow(count > 0);
    _setValid(count > 0);
}

VA(0x0040f546, 0x9e)
std::auto_ptr<TQuest> TQuestBringArtifactsDlg::getQuest() const
{
    std::auto_ptr<TQuest> pQuest(new TQuestBringArtifacts(_m_artifacts));
    if (!pQuest.get())
        throw TAllocationFailure();
    return pQuest;
}

VA(0x0040f5e4, 0x64)
void TQuestBringArtifactsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_ARTIFACT_LIST, _m_list);
    DDX_Control(pDX, IDC_QUEST_ADD_BUTTON, _m_addButton);
    DDX_Control(pDX, IDC_QUEST_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_QUEST_REMOVE_BUTTON, _m_removeButton);
    DDX_Control(pDX, IDC_QUEST_REMOVE_ALL_BUTTON, _m_removeAllButton);
}

VA(0x0040f648, 0xf4)
BOOL TQuestBringArtifactsDlg::OnInitDialog()
{
    GetDlgItem(IDC_QUEST_ARTIFACTS_STATIC)->SetWindowText(SEditQuestDlgText::kArtifactsStaticStr);
    GetDlgItem(IDC_QUEST_ADD_BUTTON)->SetWindowText(SEditQuestDlgText::kAddButtonStr);
    GetDlgItem(IDC_QUEST_EDIT_BUTTON)->SetWindowText(SEditQuestDlgText::kEditButtonStr);
    GetDlgItem(IDC_QUEST_REMOVE_BUTTON)->SetWindowText(SEditQuestDlgText::kRemoveButtonStr);
    GetDlgItem(IDC_QUEST_REMOVE_ALL_BUTTON)->SetWindowText(SEditQuestDlgText::kRemoveAllButtonStr);
    _setValid(false);
    CDialog::OnInitDialog();
    CRect rect;
    _m_list.GetClientRect(&rect);
    _m_list.SetColumnWidth(rect.Width() / 2);
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_removeAllButton.EnableWindow(FALSE);
    return TRUE;
}

VA(0x0040f73c, 0x70)
void TQuestBringArtifactsDlg::OnOK()
{
    CDialog::OnOK();
    _m_artifacts.clear();
    int count = _m_list.GetCount();
    for (int index = 0; index < count; index++)
        _m_artifacts.insert(TArtifact(_m_list.GetItemData(index)));
}

VA(0x0040f7ac, 0xdb)
void TQuestBringArtifactsDlg::OnAddButton()
{
    TSelectArtifactDlg dlg(this, _m_mapVersion, ARTIFACT_NONE);
    if (dlg.DoModal() == IDOK) {
        TArtifact artifact = TArtifact(dlg.getArtifact());
        _m_list.SetItemData(_m_list.AddString(akArtifactTraits[artifact].m_name), artifact);
        _m_addButton.EnableWindow(unsigned(_m_list.GetCount()) < _getMaxArtifacts());
        _m_removeAllButton.EnableWindow(TRUE);
        _setValid(true);
    }
}

VA(0x0040f887, 0xdc)
void TQuestBringArtifactsDlg::OnEditButton()
{
    int index = _m_list.GetCurSel();
    TArtifact artifact = TArtifact(_m_list.GetItemData(index));
    TSelectArtifactDlg dlg(this, _m_mapVersion, artifact);
    if (dlg.DoModal() == IDOK) {
        TArtifact newArtifact = TArtifact(dlg.getArtifact());
        if (newArtifact != artifact) {
            _m_list.DeleteString(index);
            _m_list.InsertString(index, akArtifactTraits[newArtifact].m_name);
            _m_list.SetItemData(index, newArtifact);
            _m_list.SetCurSel(index);
        }
    }
}

VA(0x0040f963, 0x97)
void TQuestBringArtifactsDlg::OnRemoveButton()
{
    int index = _m_list.GetCurSel();
    _m_list.DeleteString(index);
    _m_addButton.EnableWindow(TRUE);
    int count = _m_list.GetCount();
    if (count > 0)
        _m_list.SetCurSel(index < count ? index : count - 1);
    else {
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(FALSE);
        _setValid(false);
    }
}

VA(0x0040f9fa, 0x55)
void TQuestBringArtifactsDlg::OnRemoveAllButton()
{
    int count = _m_list.GetCount();
    if (_m_list.GetCurSel() == LB_ERR)
        _m_list.SetCurSel(0);
    for (; count > 0; count--)
        OnRemoveButton();
}

// Folded by /OPT:ICF onto TArtifactsDlg::OnSelChangeList.
void TQuestBringArtifactsDlg::OnSelChangeList()
{
    _m_editButton.EnableWindow(TRUE);
    _m_removeButton.EnableWindow(TRUE);
}

// Folded by /OPT:ICF onto TCreaturesDlg::OnSelCancelList.
void TQuestBringArtifactsDlg::OnSelCancelList()
{
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
}

VA(0x0040fa4f, 0x26)
void TQuestBringArtifactsDlg::OnDblclkList()
{
    if (_m_list.GetCurSel() != LB_ERR)
        OnEditButton();
}

VA(0x0040fa75, 0x47)
int TQuestBringArtifactsDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    if (nKey == VK_DELETE && _m_list.GetCurSel() != LB_ERR) {
        OnRemoveButton();
        return -2;
    }
    return TQuestTypeDlg::OnVKeyToItem(nKey, pListBox, nIndex);
}

VA(0x0040fabc, 0x6)
BEGIN_MESSAGE_MAP(TQuestBringCreaturesDlg, TQuestTypeDlg)
    ON_BN_CLICKED(IDC_QUEST_ADD_BUTTON, OnAddButton)
    ON_BN_CLICKED(IDC_QUEST_EDIT_BUTTON, OnEditButton)
    ON_BN_CLICKED(IDC_QUEST_REMOVE_BUTTON, OnRemoveButton)
    ON_BN_CLICKED(IDC_QUEST_REMOVE_ALL_BUTTON, OnRemoveAllButton)
    ON_LBN_SELCHANGE(IDC_QUEST_CREATURE_LIST, OnSelChangeList)
    ON_LBN_SELCANCEL(IDC_QUEST_CREATURE_LIST, OnSelCancelList)
    ON_LBN_DBLCLK(IDC_QUEST_CREATURE_LIST, OnDblclkList)
    ON_WM_VKEYTOITEM()
END_MESSAGE_MAP()

VA(0x0040fac2, 0x107)
TQuestBringCreaturesDlg::TQuestBringCreaturesDlg(TClient* pClient, EGameVersion mapVersion)
    : TQuestTypeDlg(pClient, false),
      _m_mapVersion(mapVersion),
      _m_abAvailableCreature(~std::bitset<kNumCreatureTypes>())
{
}

VA_COMPGEN(0x0040fbda, 0x1c, SCALAR_DELETING_DTOR, TQuestBringCreaturesDlg)
VA_COMPGEN(0x0040fbf6, 0x5, IMPLICIT_DTOR, map)
VA_COMPGEN(0x0040fbfb, 0x83, IMPLICIT_DTOR, TQuestBringCreaturesDlg)

VA(0x0040fc7e, 0x100)
void TQuestBringCreaturesDlg::setQuest(const TQuestBringCreatures& quest)
{
    _m_creatures = quest.getCreatures();
    _m_abAvailableCreature = ~std::bitset<kNumCreatureTypes>();
    for (TQuestBringCreatures::TCreatures::const_iterator it = _m_creatures.begin(); it != _m_creatures.end();
         ++it) {
        TCreatureType type = it->first;
        int quantity = it->second;
        TFormattedString item("%s\t%d", akCreatureTypeTraits[type].m_name, quantity);
        _m_list.SetItemData(_m_list.AddString(item), MAKELONG(type, quantity));
        _m_abAvailableCreature.set(type, false);
    }
    _setValid(true);
}

VA(0x0040fd7e, 0x9e)
std::auto_ptr<TQuest> TQuestBringCreaturesDlg::getQuest() const
{
    std::auto_ptr<TQuest> pQuest(new TQuestBringCreatures(_m_creatures));
    if (!pQuest.get())
        throw TAllocationFailure();
    return pQuest;
}

VA(0x0040fe1c, 0x67)
void TQuestBringCreaturesDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_QUEST_CREATURE_LIST, _m_list);
    DDX_Control(pDX, IDC_QUEST_ADD_BUTTON, _m_addButton);
    DDX_Control(pDX, IDC_QUEST_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_QUEST_REMOVE_BUTTON, _m_removeButton);
    DDX_Control(pDX, IDC_QUEST_REMOVE_ALL_BUTTON, _m_removeAllButton);
}

VA(0x0040fe83, 0xfc)
BOOL TQuestBringCreaturesDlg::OnInitDialog()
{
    GetDlgItem(IDC_QUEST_CREATURE_TYPE_STATIC)->SetWindowText(SEditQuestDlgText::kCreatureTypeStaticStr);
    GetDlgItem(IDC_QUEST_QUANTITY_STATIC)->SetWindowText(SEditQuestDlgText::kQuantityStaticStr);
    GetDlgItem(IDC_QUEST_ADD_BUTTON)->SetWindowText(SEditQuestDlgText::kAddButtonStr);
    GetDlgItem(IDC_QUEST_EDIT_BUTTON)->SetWindowText(SEditQuestDlgText::kEditButtonStr);
    GetDlgItem(IDC_QUEST_REMOVE_BUTTON)->SetWindowText(SEditQuestDlgText::kRemoveButtonStr);
    GetDlgItem(IDC_QUEST_REMOVE_ALL_BUTTON)->SetWindowText(SEditQuestDlgText::kRemoveAllButtonStr);
    _setValid(false);
    CDialog::OnInitDialog();
    _m_list.SetTabStops(171);
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_removeAllButton.EnableWindow(FALSE);
    return TRUE;
}

VA(0x0040ff7f, 0x74)
void TQuestBringCreaturesDlg::OnOK()
{
    CDialog::OnOK();
    _m_creatures.clear();
    int count = _m_list.GetCount();
    for (int index = 0; index < count; index++) {
        DWORD data = _m_list.GetItemData(index);
        _m_creatures[TCreatureType(LOWORD(data))] = HIWORD(data);
    }
}

VA(0x0040fff3, 0x111)
void TQuestBringCreaturesDlg::OnAddButton()
{
    TEditCreatureStackDlg dlg(this, _m_mapVersion, _m_abAvailableCreature);
    if (dlg.DoModal() == IDOK) {
        TCreatureStack stack = dlg.getStack();
        TFormattedString item("%s\t%d", akCreatureTypeTraits[stack.getCreatureType()].m_name, stack.getQuantity());
        int index = _m_list.AddString(item);
        _m_list.SetItemData(index, MAKELONG(stack.getCreatureType(), stack.getQuantity()));
        _m_abAvailableCreature.set(stack.getCreatureType(), false);
        _m_addButton.EnableWindow(_m_list.GetCount() < s_kMaxStacks);
        _m_removeAllButton.EnableWindow(TRUE);
        _setValid(true);
    }
}

VA(0x00410104, 0x152)
void TQuestBringCreaturesDlg::OnEditButton()
{
    int index = _m_list.GetCurSel();
    DWORD data = _m_list.GetItemData(index);
    TCreatureStack oldStack(TCreatureType(LOWORD(data)), HIWORD(data));
    TEditCreatureStackDlg dlg(this, oldStack, _m_mapVersion, _m_abAvailableCreature);
    if (dlg.DoModal() == IDOK) {
        TCreatureStack stack = dlg.getStack();
        if (!(stack == oldStack)) {
            _m_abAvailableCreature.set(oldStack.getCreatureType(), true);
            _m_list.DeleteString(index);
            TFormattedString item("%s\t%d", akCreatureTypeTraits[stack.getCreatureType()].m_name,
                                  stack.getQuantity());
            _m_list.InsertString(index, item);
            _m_list.SetItemData(index, MAKELONG(stack.getCreatureType(), stack.getQuantity()));
            _m_list.SetCurSel(index);
            _m_abAvailableCreature.set(stack.getCreatureType(), false);
        }
    }
}

VA(0x00410256, 0xb4)
void TQuestBringCreaturesDlg::OnRemoveButton()
{
    int index = _m_list.GetCurSel();
    _m_abAvailableCreature.set(LOWORD(_m_list.GetItemData(index)), true);
    _m_list.DeleteString(index);
    _m_addButton.EnableWindow(TRUE);
    int count = _m_list.GetCount();
    if (count > 0) {
        if (index >= count)
            index = count - 1;
        _m_list.SetCurSel(index);
    } else {
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(FALSE);
        _setValid(false);
    }
}

VA(0x0041030a, 0x55)
void TQuestBringCreaturesDlg::OnRemoveAllButton()
{
    int count = _m_list.GetCount();
    if (_m_list.GetCurSel() == LB_ERR)
        _m_list.SetCurSel(0);
    for (; count > 0; count--)
        OnRemoveButton();
}

VA(0x0041035f, 0x1f)
void TQuestBringCreaturesDlg::OnSelChangeList()
{
    _m_editButton.EnableWindow(TRUE);
    _m_removeButton.EnableWindow(TRUE);
}

VA(0x0041037e, 0x1f)
void TQuestBringCreaturesDlg::OnSelCancelList()
{
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
}

VA(0x0041039d, 0x26)
void TQuestBringCreaturesDlg::OnDblclkList()
{
    if (_m_list.GetCurSel() != LB_ERR)
        OnEditButton();
}

VA(0x004103c3, 0x47)
int TQuestBringCreaturesDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    if (nKey == VK_DELETE && _m_list.GetCurSel() != LB_ERR) {
        OnRemoveButton();
        return -2;
    }
    return TQuestTypeDlg::OnVKeyToItem(nKey, pListBox, nIndex);
}

VA(0x0041040a, 0x8)
void TQuestBringResourcesDlg::OnChangeRes1Edit()
{
    _onQuantityChange(0);
}

VA(0x00410412, 0x8)
void TQuestBringResourcesDlg::OnChangeRes2Edit()
{
    _onQuantityChange(1);
}

VA(0x0041041a, 0x8)
void TQuestBringResourcesDlg::OnChangeRes3Edit()
{
    _onQuantityChange(2);
}

VA(0x00410422, 0x8)
void TQuestBringResourcesDlg::OnChangeRes4Edit()
{
    _onQuantityChange(3);
}

VA(0x0041042a, 0x8)
void TQuestBringResourcesDlg::OnChangeRes5Edit()
{
    _onQuantityChange(4);
}

VA(0x00410432, 0x8)
void TQuestBringResourcesDlg::OnChangeRes6Edit()
{
    _onQuantityChange(5);
}

VA(0x0041043a, 0x8)
void TQuestBringResourcesDlg::OnChangeRes7Edit()
{
    _onQuantityChange(6);
}

VA(0x00410442, 0x6)
BEGIN_MESSAGE_MAP(TQuestBringResourcesDlg, TQuestTypeDlg)
    ON_WM_DESTROY()
    ON_EN_CHANGE(IDC_RESOURCE1_EDIT, OnChangeRes1Edit)
    ON_EN_CHANGE(IDC_RESOURCE2_EDIT, OnChangeRes2Edit)
    ON_EN_CHANGE(IDC_RESOURCE3_EDIT, OnChangeRes3Edit)
    ON_EN_CHANGE(IDC_RESOURCE4_EDIT, OnChangeRes4Edit)
    ON_EN_CHANGE(IDC_RESOURCE5_EDIT, OnChangeRes5Edit)
    ON_EN_CHANGE(IDC_RESOURCE6_EDIT, OnChangeRes6Edit)
    ON_EN_CHANGE(IDC_RESOURCE7_EDIT, OnChangeRes7Edit)
END_MESSAGE_MAP()

struct TResourceControlIDs {
    int m_editID;
    int m_spinID;
};

DATA(0x00532490)
static const TResourceControlIDs g_aResourceControlIDs[kNumGameResourceTypes] = {
    { IDC_RESOURCE1_EDIT, IDC_RESOURCE1_SPIN },
    { IDC_RESOURCE2_EDIT, IDC_RESOURCE2_SPIN },
    { IDC_RESOURCE3_EDIT, IDC_RESOURCE3_SPIN },
    { IDC_RESOURCE4_EDIT, IDC_RESOURCE4_SPIN },
    { IDC_RESOURCE5_EDIT, IDC_RESOURCE5_SPIN },
    { IDC_RESOURCE6_EDIT, IDC_RESOURCE6_SPIN },
    { IDC_RESOURCE7_EDIT, IDC_RESOURCE7_SPIN }
};

VA(0x00410448, 0xa5)
void TQuestBringResourcesDlg::setQuest(const TQuestBringResources& quest)
{
    _m_aQuantity = quest.getQuantities();
    _m_numTypes = 0;
    for (int type = 0; type < kNumGameResourceTypes; type++) {
        if (unsigned(_m_aQuantity[type]) > 0)
            _m_numTypes++;
        TFormattedString text("%d", _m_aQuantity[type]);
        _m_aResourceControls[type].m_edit.SetWindowText(text);
    }
    _setValid(_m_numTypes > 0);
}

VA(0x004104ed, 0x9e)
std::auto_ptr<TQuest> TQuestBringResourcesDlg::getQuest() const
{
    std::auto_ptr<TQuest> pQuest(new TQuestBringResources(_m_aQuantity));
    if (!pQuest.get())
        throw TAllocationFailure();
    return pQuest;
}

VA(0x0041058b, 0x4d)
void TQuestBringResourcesDlg::DoDataExchange(CDataExchange* pDX)
{
    for (unsigned int type = 0; type < kNumGameResourceTypes; type++) {
        DDX_Control(pDX, g_aResourceControlIDs[type].m_editID, _m_aResourceControls[type].m_edit);
        DDX_Control(pDX, g_aResourceControlIDs[type].m_spinID, _m_aResourceControls[type].m_spin);
    }
    if (!pDX->m_bSaveAndValidate)
        _m_bCreated = true;
}

VA(0x004105d8, 0xc)
void TQuestBringResourcesDlg::OnDestroy()
{
    _m_bCreated = false;
    TQuestTypeDlg::OnDestroy();
}

VA(0x004105e4, 0x11f)
BOOL TQuestBringResourcesDlg::OnInitDialog()
{
    GetDlgItem(IDC_QUEST_RES1_STATIC)->SetWindowText(SEditQuestDlgText::kRes0StaticStr);
    GetDlgItem(IDC_QUEST_RES2_STATIC)->SetWindowText(SEditQuestDlgText::kRes1StaticStr);
    GetDlgItem(IDC_QUEST_RES3_STATIC)->SetWindowText(SEditQuestDlgText::kRes2StaticStr);
    GetDlgItem(IDC_QUEST_RES4_STATIC)->SetWindowText(SEditQuestDlgText::kRes3StaticStr);
    GetDlgItem(IDC_QUEST_RES5_STATIC)->SetWindowText(SEditQuestDlgText::kRes4StaticStr);
    GetDlgItem(IDC_QUEST_RES6_STATIC)->SetWindowText(SEditQuestDlgText::kRes5StaticStr);
    GetDlgItem(IDC_QUEST_RES7_STATIC)->SetWindowText(SEditQuestDlgText::kRes6StaticStr);
    int type;
    for (type = 0; type < kNumGameResourceTypes; type++)
        _m_aQuantity[type] = 0;
    _m_numTypes = 0;
    _setValid(false);
    CDialog::OnInitDialog();
    for (type = 0; type < kNumGameResourceTypes; type++) {
        _m_aResourceControls[type].m_edit.LimitText(TDigits<UD_MAXVAL>::getDigits());
        _m_aResourceControls[type].m_edit.SetWindowText("0");
        _m_aResourceControls[type].m_spin.SetRange(0, UD_MAXVAL);
    }
    return TRUE;
}

VA(0x00410703, 0xac)
void TQuestBringResourcesDlg::_onQuantityChange(int type)
{
    if (_m_bCreated) {
        int quantity = 0;
        CString text;
        _m_aResourceControls[type].m_edit.GetWindowText(text);
        sscanf(text, "%d", &quantity);
        int oldQuantity = _m_aQuantity[type];
        _m_aQuantity[type] = quantity;
        if (quantity == 0) {
            if (oldQuantity != 0 && --_m_numTypes == 0)
                _setValid(false);
        } else if (oldQuantity == 0 && _m_numTypes++ == 0)
            _setValid(true);
    }
}

VA(0x004107af, 0x6)
BEGIN_MESSAGE_MAP(TQuestBeASpecificHeroDlg, TQuestTypeDlg)
    ON_CBN_SELCHANGE(IDC_CLASS_COMBO, OnSelChangeClassCombo)
END_MESSAGE_MAP()

VA(0x004107b5, 0xa6)
void TQuestBeASpecificHeroDlg::setQuest(const TQuestBeASpecificHero& quest)
{
    _m_heroID = quest.getHeroID();
    THeroClass heroClass = THero::s_akTraits[_m_heroID].m_class;
    int index;
    for (index = 0; THeroClass(_m_classCombo.GetItemData(index)) != heroClass; index++)
        ;
    if (_m_classCombo.GetCurSel() != index) {
        _m_classCombo.SetCurSel(index);
        OnSelChangeClassCombo();
    }
    for (index = 0; THeroID(_m_identityCombo.GetItemData(index)) != _m_heroID; index++)
        ;
    _m_identityCombo.SetCurSel(index);
}

VA(0x0041085b, 0x9d)
std::auto_ptr<TQuest> TQuestBeASpecificHeroDlg::getQuest() const
{
    std::auto_ptr<TQuest> pQuest(new TQuestBeASpecificHero(_m_heroID));
    if (!pQuest.get())
        throw TAllocationFailure();
    return pQuest;
}

VA(0x004108f8, 0x2e)
void TQuestBeASpecificHeroDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_CLASS_COMBO, _m_classCombo);
    DDX_Control(pDX, IDC_IDENTITY_COMBO, _m_identityCombo);
}

VA(0x00410926, 0x1c2)
BOOL TQuestBeASpecificHeroDlg::OnInitDialog()
{
    GetDlgItem(IDC_PLACEHOLDER_CLASS_STATIC)->SetWindowText(SEditQuestDlgText::kClassStaticStr);
    GetDlgItem(IDC_HERO_IDENTITY_STATIC)->SetWindowText(SEditQuestDlgText::kIdentityStaticStr);
    CDialog::OnInitDialog();
    EGameVersion mapVersion = _m_pMap->getVersion();
    int numClasses = mapVersion >= GAME_VERSION_AB ? kNumHeroClasses : classPlanesWalker;
    for (int heroClass = 0; heroClass < numClasses; heroClass++)
        _m_classCombo.SetItemData(_m_classCombo.AddString(THero::s_akClassTraits[heroClass].m_name), heroClass);
    _m_classCombo.SetCurSel(0);
    const THero::TClassTraits& classTraits = THero::s_akClassTraits[_m_classCombo.GetItemData(0)];
    for (std::set<int>::const_iterator it = classTraits.m_heroes.begin(); it != classTraits.m_heroes.end(); ++it) {
        if (THero::s_akTraits[*it].m_gameVersions.test(mapVersion))
            _m_identityCombo.SetItemData(
                _m_identityCombo.AddString(_m_pMap->getHeroPrototype(*it).getName().c_str()), *it);
    }
    _m_identityCombo.SetCurSel(0);
    _m_heroID = _m_identityCombo.GetItemData(0);
    return TRUE;
}

VA(0x00410ae8, 0x36)
void TQuestBeASpecificHeroDlg::OnOK()
{
    CDialog::OnOK();
    _m_heroID = _m_identityCombo.GetItemData(_m_identityCombo.GetCurSel());
}

VA(0x00410b1e, 0x125)
void TQuestBeASpecificHeroDlg::OnSelChangeClassCombo()
{
    int heroClass = _m_classCombo.GetItemData(_m_classCombo.GetCurSel());
    _m_identityCombo.ResetContent();
    EGameVersion mapVersion = _m_pMap->getVersion();
    const THero::TClassTraits& classTraits = THero::s_akClassTraits[heroClass];
    for (std::set<int>::const_iterator it = classTraits.m_heroes.begin(); it != classTraits.m_heroes.end(); ++it) {
        if (THero::s_akTraits[*it].m_gameVersions.test(mapVersion))
            _m_identityCombo.SetItemData(
                _m_identityCombo.AddString(_m_pMap->getHeroPrototype(*it).getName().c_str()), *it);
    }
    _m_identityCombo.SetCurSel(0);
}

VA(0x00410c43, 0x51)
void TQuestBelongToASpecificPlayerDlg::setQuest(const TQuestBelongToASpecificPlayer& quest)
{
    _m_player = quest.getPlayer();
    int index;
    for (index = 0; TPlayer(_m_playerCombo.GetItemData(index)) != _m_player; index++)
        ;
    _m_playerCombo.SetCurSel(index);
}

VA(0x00410c94, 0x9d)
std::auto_ptr<TQuest> TQuestBelongToASpecificPlayerDlg::getQuest() const
{
    std::auto_ptr<TQuest> pQuest(new TQuestBelongToASpecificPlayer(_m_player));
    if (!pQuest.get())
        throw TAllocationFailure();
    return pQuest;
}

VA(0x00410d31, 0x15)
void TQuestBelongToASpecificPlayerDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_PLAYER_COMBO, _m_playerCombo);
}

VA(0x00410d46, 0x83)
BOOL TQuestBelongToASpecificPlayerDlg::OnInitDialog()
{
    GetDlgItem(IDC_QUEST_PLAYER_STATIC)->SetWindowText(SEditQuestDlgText::kPlayerStaticStr);
    CDialog::OnInitDialog();
    for (int player = 0; player < kNumPlayers; player++)
        _m_playerCombo.SetItemData(_m_playerCombo.AddString(akPlayerTraits[player].m_pName), player);
    _m_playerCombo.SetCurSel(0);
    _m_player = TPlayer(_m_playerCombo.GetItemData(0));
    return TRUE;
}

VA(0x00410dc9, 0x169)
TEditQuestDlg::_TPages::_TPages(TQuestTypeDlg::TClient* pClient, const TGameMap* pMap)
    : m_noneDlg(pClient),
      m_experienceLevelDlg(pClient, pMap->getVersion()),
      m_primarySkillLevelDlg(pClient, pMap->getVersion()),
      m_defeatHeroDlg(pClient, pMap),
      m_defeatMonsterDlg(pClient, pMap),
      m_bringArtifactsDlg(pClient, pMap->getVersion()),
      m_bringCreaturesDlg(pClient, pMap->getVersion()),
      m_bringResourcesDlg(pClient, pMap->getVersion()),
      m_beASpecificHeroDlg(pClient, pMap),
      m_belongToASpecificPlayerDlg(pClient, pMap->getVersion())
{
    m_apDlg[0] = &m_noneDlg;
    m_apDlg[1] = &m_experienceLevelDlg;
    m_apDlg[2] = &m_primarySkillLevelDlg;
    m_apDlg[3] = &m_defeatHeroDlg;
    m_apDlg[4] = &m_defeatMonsterDlg;
    m_apDlg[5] = &m_bringArtifactsDlg;
    m_apDlg[6] = &m_bringCreaturesDlg;
    m_apDlg[7] = &m_bringResourcesDlg;
    m_apDlg[8] = &m_beASpecificHeroDlg;
    m_apDlg[9] = &m_belongToASpecificPlayerDlg;
}

VA_COMPGEN(0x00410fb3, 0x1c, SCALAR_DELETING_DTOR, TQuestAchieveExperienceLevelDlg)
VA_COMPGEN(0x00410fcf, 0x47, IMPLICIT_DTOR, TQuestAchieveExperienceLevelDlg)
VA_COMPGEN(0x00411096, 0x1c, SCALAR_DELETING_DTOR, TQuestAchievePrimarySkillLevelDlg)
VA_COMPGEN(0x004110b2, 0x41, CLASS_CTOR, _TSkillControls)
VA_COMPGEN(0x004110f3, 0x35, IMPLICIT_DTOR, _TSkillControls)
VA_COMPGEN(0x00411128, 0x48, IMPLICIT_DTOR, TQuestAchievePrimarySkillLevelDlg)
VA_COMPGEN(0x004111da, 0x1c, SCALAR_DELETING_DTOR, TQuestDefeatHeroDlg)
VA_COMPGEN(0x004111f6, 0x35, IMPLICIT_DTOR, TQuestDefeatHeroDlg)
VA_COMPGEN(0x00411295, 0x1c, SCALAR_DELETING_DTOR, TQuestDefeatMonsterDlg)
VA_COMPGEN(0x004112b1, 0x35, IMPLICIT_DTOR, TQuestDefeatMonsterDlg)
VA_COMPGEN(0x004113ad, 0x5, IMPLICIT_DTOR, multiset)
VA_COMPGEN(0x004113c3, 0x1c, SCALAR_DELETING_DTOR, TQuestBringArtifactsDlg)
VA_COMPGEN(0x004113df, 0x80, IMPLICIT_DTOR, TQuestBringArtifactsDlg)
VA_COMPGEN(0x004114ec, 0x1c, SCALAR_DELETING_DTOR, TQuestBringResourcesDlg)
VA_COMPGEN(0x00411508, 0x41, CLASS_CTOR, _TResourceControls)
VA_COMPGEN(0x00411549, 0x35, IMPLICIT_DTOR, _TResourceControls)
VA_COMPGEN(0x0041157e, 0x4e, IMPLICIT_DTOR, TQuestBringResourcesDlg)
VA_COMPGEN(0x0041165f, 0x1c, SCALAR_DELETING_DTOR, TQuestBeASpecificHeroDlg)
VA_COMPGEN(0x0041167b, 0x47, IMPLICIT_DTOR, TQuestBeASpecificHeroDlg)
VA_COMPGEN(0x00411736, 0x1c, SCALAR_DELETING_DTOR, TQuestBelongToASpecificPlayerDlg)
VA_COMPGEN(0x00411752, 0x35, IMPLICIT_DTOR, TQuestBelongToASpecificPlayerDlg)

VA(0x00411787, 0x18f)
TEditQuestDlg::TEditQuestDlg(CWnd* pParent, const TQuest* pQuest, const TGameMap* pMap)
    : CDialog(TEditQuestDlg::IDD, pParent),
      _m_pMap(pMap),
      _m_bDestroyed(true)
{
    _m_questType = -1;
    _m_pPages = std::auto_ptr<_TPages>(new _TPages(this, pMap));
    if (!_m_pPages.get())
        throw TAllocationFailure();
    if (pQuest != NULL) {
        _m_pQuest = pQuest->clone();
        if (!_m_pQuest.get())
            throw TAllocationFailure();
    }
}

VA_COMPGEN(0x00411919, 0x1c, SCALAR_DELETING_DTOR, TEditQuestDlg)

VA(0x00411935, 0x83)
TEditQuestDlg::~TEditQuestDlg()
{
}

VA(0x004119b8, 0x7b)
std::auto_ptr<TQuest> TEditQuestDlg::getQuest()
{
    std::auto_ptr<TQuest> pQuest(_m_pQuest);
    _m_pQuest = std::auto_ptr<TQuest>();
    return pQuest;
}

VA(0x00411a33, 0x5f)
void TEditQuestDlg::_showPage(int questType)
{
    _m_pPages->m_apDlg[_m_questType]->ShowWindow(SW_HIDE);
    _m_questType = questType;
    UpdateData(FALSE);
    TQuestTypeDlg* pDlg = _m_pPages->m_apDlg[_m_questType];
    pDlg->ShowWindow(SW_SHOW);
    _m_okButton.EnableWindow(pDlg->isValid());
}

VA(0x00411a92, 0x20)
void TEditQuestDlg::visit(const TQuestAchieveExperienceLevel& quest)
{
    _m_pPages->m_experienceLevelDlg.setQuest(quest);
    _showPage(1);
}

VA(0x00411ab2, 0x23)
void TEditQuestDlg::visit(const TQuestAchievePrimarySkillLevel& quest)
{
    _m_pPages->m_primarySkillLevelDlg.setQuest(quest);
    _showPage(2);
}

VA(0x00411ad5, 0x23)
void TEditQuestDlg::visit(const TQuestDefeatHero& quest)
{
    _m_pPages->m_defeatHeroDlg.setQuest(quest);
    _showPage(3);
}

VA(0x00411af8, 0x23)
void TEditQuestDlg::visit(const TQuestDefeatMonster& quest)
{
    _m_pPages->m_defeatMonsterDlg.setQuest(quest);
    _showPage(4);
}

VA(0x00411b1b, 0x23)
void TEditQuestDlg::visit(const TQuestBringArtifacts& quest)
{
    _m_pPages->m_bringArtifactsDlg.setQuest(quest);
    _showPage(5);
}

VA(0x00411b3e, 0x23)
void TEditQuestDlg::visit(const TQuestBringCreatures& quest)
{
    _m_pPages->m_bringCreaturesDlg.setQuest(quest);
    _showPage(6);
}

VA(0x00411b61, 0x23)
void TEditQuestDlg::visit(const TQuestBringResources& quest)
{
    _m_pPages->m_bringResourcesDlg.setQuest(quest);
    _showPage(7);
}

VA(0x00411b84, 0x23)
void TEditQuestDlg::visit(const TQuestBeASpecificHero& quest)
{
    _m_pPages->m_beASpecificHeroDlg.setQuest(quest);
    _showPage(8);
}

VA(0x00411ba7, 0x23)
void TEditQuestDlg::visit(const TQuestBelongToASpecificPlayer& quest)
{
    _m_pPages->m_belongToASpecificPlayerDlg.setQuest(quest);
    _showPage(9);
}

DATA(0x00532500)
static const int g_akQuestTypeRadioIDs[TEditQuestDlg::s_kNumQuestTypes] = {
    IDC_NONE_RADIO,
    IDC_EXPERIENCE_LEVEL_RADIO,
    IDC_PRIMARY_SKILL_LEVEL_RADIO,
    IDC_DEFEAT_HERO_RADIO,
    IDC_DEFEAT_MONSTER_RADIO,
    IDC_BRING_ARTIFACTS_RADIO,
    IDC_BRING_CREATURES_RADIO,
    IDC_BRING_RESOURCES_RADIO,
    IDC_BE_A_SPECIFIC_HERO_RADIO,
    IDC_BELONG_TO_A_SPECIFIC_PLAYER_RADIO
};

VA(0x00411bca, 0x4f)
void TEditQuestDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDOK, _m_okButton);
    DDX_Radio(pDX, IDC_NONE_RADIO, _m_questType);
    for (unsigned int questType = 0; questType < s_kNumQuestTypes; questType++)
        DDX_Control(pDX, g_akQuestTypeRadioIDs[questType], _m_aQuestTypeRadio[questType]);
}

VA(0x00411c19, 0x6)
BEGIN_MESSAGE_MAP(TEditQuestDlg, CDialog)
    ON_BN_CLICKED(IDC_NONE_RADIO, OnQuestTypeRadio)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_EXPERIENCE_LEVEL_RADIO, OnQuestTypeRadio)
    ON_BN_CLICKED(IDC_PRIMARY_SKILL_LEVEL_RADIO, OnQuestTypeRadio)
    ON_BN_CLICKED(IDC_DEFEAT_HERO_RADIO, OnQuestTypeRadio)
    ON_BN_CLICKED(IDC_DEFEAT_MONSTER_RADIO, OnQuestTypeRadio)
    ON_BN_CLICKED(IDC_BRING_ARTIFACTS_RADIO, OnQuestTypeRadio)
    ON_BN_CLICKED(IDC_BRING_CREATURES_RADIO, OnQuestTypeRadio)
    ON_BN_CLICKED(IDC_BRING_RESOURCES_RADIO, OnQuestTypeRadio)
    ON_BN_CLICKED(IDC_BE_A_SPECIFIC_HERO_RADIO, OnQuestTypeRadio)
    ON_BN_CLICKED(IDC_BELONG_TO_A_SPECIFIC_PLAYER_RADIO, OnQuestTypeRadio)
END_MESSAGE_MAP()

VA(0x00411c1f, 0x4e)
int TEditQuestDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(SEditQuestDlgText::kCaptionStr);
    for (unsigned int questType = 0; questType < s_kNumQuestTypes; questType++) {
        if (!_m_pPages->m_apDlg[questType]->create(this))
            return -1;
    }
    return 0;
}

VA(0x00411c6d, 0x33)
void TEditQuestDlg::OnDestroy()
{
    for (unsigned int questType = 0; questType < s_kNumQuestTypes; questType++)
        _m_pPages->m_apDlg[questType]->DestroyWindow();
    CDialog::OnDestroy();
    _m_bDestroyed = true;
}

VA(0x00411ca0, 0x255)
BOOL TEditQuestDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_SELECT_QUEST_TYPE_STATIC)->SetWindowText(SEditQuestDlgText::kSelectQuestTypeStaticStr);
    GetDlgItem(IDC_NONE_RADIO)->SetWindowText(SEditQuestDlgText::kNoneRadioStr);
    GetDlgItem(IDC_EXPERIENCE_LEVEL_RADIO)->SetWindowText(SEditQuestDlgText::kExperienceLevelRadioStr);
    GetDlgItem(IDC_PRIMARY_SKILL_LEVEL_RADIO)->SetWindowText(SEditQuestDlgText::kPrimarySkillLevelRadioStr);
    GetDlgItem(IDC_DEFEAT_HERO_RADIO)->SetWindowText(SEditQuestDlgText::kDefeatHeroRadioStr);
    GetDlgItem(IDC_DEFEAT_MONSTER_RADIO)->SetWindowText(SEditQuestDlgText::kDefeatMonsterRadioStr);
    GetDlgItem(IDC_BRING_ARTIFACTS_RADIO)->SetWindowText(SEditQuestDlgText::kArtifactsRadioStr);
    GetDlgItem(IDC_BRING_CREATURES_RADIO)->SetWindowText(SEditQuestDlgText::kCreaturesRadioStr);
    GetDlgItem(IDC_BRING_RESOURCES_RADIO)->SetWindowText(SEditQuestDlgText::kResourcesRadioStr);
    GetDlgItem(IDC_BE_A_SPECIFIC_HERO_RADIO)->SetWindowText(SEditQuestDlgText::kBeHeroRadioStr);
    GetDlgItem(IDC_BELONG_TO_A_SPECIFIC_PLAYER_RADIO)->SetWindowText(SEditQuestDlgText::kBelongToPlayerRadioStr);
    _m_questType = 0;
    CDialog::OnInitDialog();
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    const CWnd* pInsertAfter = pFrame;
    unsigned int questType;
    for (questType = 0; questType < s_kNumQuestTypes; questType++) {
        TQuestTypeDlg* pDlg = _m_pPages->m_apDlg[questType];
        pDlg->SetWindowPos(pInsertAfter, rect.left, rect.top, 0, 0, SWP_NOSIZE);
        pInsertAfter = pDlg;
    }
    pFrame->DestroyWindow();
    for (questType = 0; questType < s_kNumQuestTypes; questType++)
        _m_aQuestTypeRadio[questType].EnableWindow(_m_pPages->m_apDlg[questType]->isAvailable());
    _m_pPages->m_apDlg[0]->ShowWindow(SW_SHOW);
    if (_m_pQuest.get() != NULL)
        _m_pQuest->accept(*this);
    _m_bDestroyed = false;
    return TRUE;
}

VA(0x00411ef5, 0x51)
void TEditQuestDlg::OnOK()
{
    CDialog::OnOK();
    TQuestTypeDlg* pDlg = _m_pPages->m_apDlg[_m_questType];
    pDlg->OnOK();
    _m_pQuest = pDlg->getQuest();
}

VA(0x00411f46, 0x59)
void TEditQuestDlg::OnQuestTypeRadio()
{
    _m_pPages->m_apDlg[_m_questType]->ShowWindow(SW_HIDE);
    UpdateData(TRUE);
    TQuestTypeDlg* pDlg = _m_pPages->m_apDlg[_m_questType];
    pDlg->ShowWindow(SW_SHOW);
    _m_okButton.EnableWindow(pDlg->isValid());
}

VA(0x00411f9f, 0x1a)
void TEditQuestDlg::onValidityChanged(TQuestTypeDlg* pDlg, bool bValid)
{
    if (!_m_bDestroyed)
        _m_okButton.EnableWindow(bValid);
}
