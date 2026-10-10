// EditStartingBonusDlg.cpp - one bonus of a scenario's bonus starting
// option (h3ccmped 0x41e210..0x421bc0). Each bonus type has its own child
// dialog in the bonus dialog's frame; the hero bonuses share a recipient
// combo (the player's most powerful hero, the hero generated at the main
// town when the map makes one, or one of the player's heroes).
#include "campaign_editor/stdafx.h"

#include <stdio.h>

#include "armygrp.h"
#include "artifact.h"
#include "herospec.h"
#include "sskilltraits.h"
#include "town_type.h"
#include "va.h"
#include "editor/Army.h"
#include "editor/Array.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/FormattedString.h"
#include "editor/GameResource.h"
#include "editor/Hero.h"
#include "editor/Town.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/EditStartingBonusDlg.h"

VA(0x0041e240, 0x22)
TStartingBonusTypeDlg::TStartingBonusTypeDlg(TClient* pClient, bool bValid)
    : _m_pClient(pClient), _m_bValid(bValid)
{
}

VA(0x0041e280, 0x1c)
void TStartingBonusTypeDlg::setBValid(bool bValid)
{
    if (bValid != _m_bValid) {
        _m_bValid = bValid;
        _m_pClient->onBValidChange(this, bValid);
    }
}

namespace {

// A bonus a hero receives: the recipient combo.
class TStartingHeroBonusDlg : public TStartingBonusTypeDlg {
public:
    TStartingHeroBonusDlg(TClient* pClient, bool bValid, const TCampaignScenarioMap* pMap, int player);

    void setBonus(const TScenarioHeroBonus& bonus);

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    const TCampaignScenarioMap* _m_pMap;
    int _m_player;
    int _m_hero;
    CComboBox _m_heroCombo;
};

// A spell for the hero to learn.
class TStartingBonusSpellDlg : public TStartingHeroBonusDlg {
public:
    enum { IDD = IDD_STARTING_BONUS_SPELL };

    TStartingBonusSpellDlg(TClient* pClient, const TCampaignScenarioMap* pMap, int player);

    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TScenarioStartingBonus> getBonus() const;
    void setBonus(const TScenarioBonusSpell& bonus);

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    int _m_spell;
    CComboBox _m_spellCombo;
};

// A creature stack for the hero's army.
class TStartingBonusCreatureDlg : public TStartingHeroBonusDlg {
public:
    enum { IDD = IDD_STARTING_BONUS_CREATURE };

    TStartingBonusCreatureDlg(TClient* pClient, const TCampaignScenarioMap* pMap, int player);

    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TScenarioStartingBonus> getBonus() const;
    void setBonus(const TScenarioBonusCreature& bonus);

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnKillFocusQuantityEdit();
    DECLARE_MESSAGE_MAP()

private:
    TCreatureStack _m_stack;
    CComboBox _m_creatureCombo;
    CEdit _m_quantityEdit;
    CSpinButtonCtrl _m_quantitySpin;
};

// A building of the player's main town; only a player with a main town of
// a known type can take one.
class TStartingBonusBuildingDlg : public TStartingBonusTypeDlg {
public:
    enum { IDD = IDD_STARTING_BONUS_BUILDING };

    TStartingBonusBuildingDlg(TClient* pClient, const TCampaignScenarioMap* pMap, int player);

    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual bool isAvailable() const;
    virtual std::auto_ptr<TScenarioStartingBonus> getBonus() const;
    void setBonus(const TScenarioBonusBuilding& bonus);

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    const TCampaignScenarioMap* _m_pMap;
    int _m_player;
    int _m_building;
    CComboBox _m_buildingCombo;
};

// An artifact for the hero: the campaign version's artifacts, the war
// machines among the special ones.
class TStartingBonusArtifactDlg : public TStartingHeroBonusDlg {
public:
    enum { IDD = IDD_STARTING_BONUS_ARTIFACT };

    TStartingBonusArtifactDlg(TClient* pClient, const TCampaignScenarioMap* pMap, int player, int campaignVersion);

    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TScenarioStartingBonus> getBonus() const;
    void setBonus(const TScenarioBonusArtifact& bonus);

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    int _m_campaignVersion;
    int _m_artifact;
    CComboBox _m_artifactCombo;
};

// A spell scroll for the hero (the spell dialog's template).
class TStartingBonusSpellScrollDlg : public TStartingHeroBonusDlg {
public:
    enum { IDD = IDD_STARTING_BONUS_SPELL };

    TStartingBonusSpellScrollDlg(TClient* pClient, const TCampaignScenarioMap* pMap, int player);

    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TScenarioStartingBonus> getBonus() const;
    void setBonus(const TScenarioBonusSpellScroll& bonus);

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    int _m_spell;
    CComboBox _m_spellCombo;
};

// Points for the hero's primary skills, each 0..s_kMaxSkill; a bonus
// needs one skill above 0.
class TStartingBonusPrimarySkillDlg : public TStartingHeroBonusDlg {
public:
    enum { IDD = IDD_STARTING_BONUS_PRIMARY_SKILL };
    enum { s_kMaxSkill = 99 };

    TStartingBonusPrimarySkillDlg(TClient* pClient, const TCampaignScenarioMap* pMap, int player);

    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TScenarioStartingBonus> getBonus() const;
    void setBonus(const TScenarioBonusPrimarySkill& bonus);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnChangeAttackEdit();
    afx_msg void OnChangeDefenseEdit();
    afx_msg void OnChangeSpellPowerEdit();
    afx_msg void OnChangeKnowledgeEdit();
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

    void _onChangeSkillEdit(unsigned int skill);
    void _onKillFocusSkillEdit(unsigned int skill);

    TArray<unsigned int, kNumPrimarySkills> _m_skills;
    // The skills above 0.
    unsigned int _m_numSkills;
    _TSkillControls _m_aSkillControls[kNumPrimarySkills];
    // Whether the edits are attached: their change notifications count.
    bool _m_bControls;
};

// A secondary skill for the hero, at a mastery.
class TStartingBonusSecondarySkillDlg : public TStartingHeroBonusDlg {
public:
    enum { IDD = IDD_STARTING_BONUS_SECONDARY_SKILL };

    TStartingBonusSecondarySkillDlg(TClient* pClient, const TCampaignScenarioMap* pMap, int player);

    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TScenarioStartingBonus> getBonus() const;
    void setBonus(const TScenarioBonusSecondarySkill& bonus);

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    int _m_skill;
    int _m_mastery;
    CComboBox _m_skillCombo;
    CComboBox _m_masteryCombo;
};

// An amount of a resource, of wood and ore, or of the rare resources.
class TStartingBonusResourceDlg : public TStartingBonusTypeDlg {
public:
    enum { IDD = IDD_STARTING_BONUS_RESOURCE };
    enum { s_kMaxAmount = 999999 };

    TStartingBonusResourceDlg(TClient* pClient);

    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual std::auto_ptr<TScenarioStartingBonus> getBonus() const;
    void setBonus(const TScenarioBonusResource& bonus);

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnKillFocusAmountEdit();
    DECLARE_MESSAGE_MAP()

private:
    int _m_resource;
    int _m_amount;
    bool _m_bInitialized;
    CComboBox _m_resourceCombo;
    CEdit _m_amountEdit;
    CSpinButtonCtrl _m_amountSpin;
};

}

// The type dialogs, in TScenarioBonusType order.
struct TEditStartingBonusDlg::_TTypeDlgs {
    _TTypeDlgs(TStartingBonusTypeDlg::TClient* pClient, const TCampaignScenarioMap* pMap, int player,
               int campaignVersion);

    TStartingBonusSpellDlg m_spellDlg;
    TStartingBonusCreatureDlg m_creatureDlg;
    TStartingBonusBuildingDlg m_buildingDlg;
    TStartingBonusArtifactDlg m_artifactDlg;
    TStartingBonusSpellScrollDlg m_spellScrollDlg;
    TStartingBonusPrimarySkillDlg m_primarySkillDlg;
    TStartingBonusSecondarySkillDlg m_secondarySkillDlg;
    TStartingBonusResourceDlg m_resourceDlg;
    TStartingBonusTypeDlg* m_apDlg[kNumBonusTypes];
};

VA(0x0041e2a0, 0x70)
TStartingHeroBonusDlg::TStartingHeroBonusDlg(TClient* pClient, bool bValid, const TCampaignScenarioMap* pMap,
                                             int player)
    : TStartingBonusTypeDlg(pClient, bValid), _m_pMap(pMap), _m_player(player)
{
}

VA_COMPGEN(0x0041e310, 0x1e, SCALAR_DELETING_DTOR, TStartingHeroBonusDlg)
VA_COMPGEN(0x0041e330, 0x4c, IMPLICIT_DTOR, TStartingHeroBonusDlg)

VA(0x0041e380, 0x16)
void TStartingHeroBonusDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_RECIPIENT_COMBO, _m_heroCombo);
}

VA(0x0041e3a0, 0x145)
BOOL TStartingHeroBonusDlg::OnInitDialog()
{
    GetDlgItem(IDC_RECIPIENT_STATIC)->SetWindowText(SEditStartingBonusDlgText::kRecipientStaticStr);
    _m_hero = TScenarioHeroBonus::kMostPowerfulHero;
    CDialog::OnInitDialog();
    _m_heroCombo.SetItemData(_m_heroCombo.AddString(kMostPowerfulHeroStr), TScenarioHeroBonus::kMostPowerfulHero);
    const TCampaignScenarioMap::TPlayerInfo& info = _m_pMap->m_aPlayer[_m_player];
    if (info.m_bGenerateHeroAtMainTown)
        _m_heroCombo.SetItemData(_m_heroCombo.AddString(kGeneratedHeroStr), TScenarioHeroBonus::kGeneratedHero);
    for (map<int, string>::const_iterator it = info.m_heroes.begin(); it != info.m_heroes.end(); ++it)
        _m_heroCombo.SetItemData(_m_heroCombo.AddString(it->second.c_str()), it->first);
    int index = 0;
    while (_m_heroCombo.GetItemData(index) != _m_hero)
        index++;
    _m_heroCombo.SetCurSel(index);
    return TRUE;
}

void TStartingHeroBonusDlg::setBonus(const TScenarioHeroBonus& bonus)
{
    _m_hero = bonus.m_hero;
    int index = 0;
    while (_m_heroCombo.GetItemData(index) != _m_hero)
        index++;
    _m_heroCombo.SetCurSel(index);
}

void TStartingHeroBonusDlg::OnOK()
{
    TStartingBonusTypeDlg::OnOK();
    _m_hero = _m_heroCombo.GetItemData(_m_heroCombo.GetCurSel());
}

VA(0x0041e4f0, 0x6c)
TStartingBonusSpellDlg::TStartingBonusSpellDlg(TClient* pClient, const TCampaignScenarioMap* pMap, int player)
    : TStartingHeroBonusDlg(pClient, true, pMap, player)
{
}

VA_COMPGEN(0x0041e560, 0x1e, SCALAR_DELETING_DTOR, TStartingBonusSpellDlg)
VA_COMPGEN(0x0041e580, 0x5f, IMPLICIT_DTOR, TStartingBonusSpellDlg)

VA(0x0041e5e0, 0xa5)
std::auto_ptr<TScenarioStartingBonus> TStartingBonusSpellDlg::getBonus() const
{
    std::auto_ptr<TScenarioStartingBonus> pBonus(new TScenarioBonusSpell(_m_hero, _m_spell));
    if (pBonus.get() == NULL)
        throw TAllocationFailure();
    return pBonus;
}

void TStartingBonusSpellDlg::setBonus(const TScenarioBonusSpell& bonus)
{
    TStartingHeroBonusDlg::setBonus(bonus);
    _m_spell = bonus.m_spell;
    int index = 0;
    while (_m_spellCombo.GetItemData(index) != _m_spell)
        index++;
    _m_spellCombo.SetCurSel(index);
}

void TStartingBonusSpellDlg::DoDataExchange(CDataExchange* pDX)
{
    TStartingHeroBonusDlg::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SCROLL_SPELL_COMBO, _m_spellCombo);
}

BOOL TStartingBonusSpellDlg::OnInitDialog()
{
    GetDlgItem(IDC_SPELL_STATIC)->SetWindowText(SEditStartingBonusDlgText::kSpellStaticStr);
    _m_spell = SPELL_MAGIC_ARROW;
    TStartingHeroBonusDlg::OnInitDialog();
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        if (!(akSpellTraits[spell].m_flags & 0x2000) && akSpellTraits[spell].m_schoolBits != 0)
            _m_spellCombo.SetItemData(_m_spellCombo.AddString(akSpellTraits[spell].m_name), spell);
    }
    int index = 0;
    while (_m_spellCombo.GetItemData(index) != _m_spell)
        index++;
    _m_spellCombo.SetCurSel(index);
    return TRUE;
}

VA(0x0041e690, 0x3b)
void TStartingBonusSpellDlg::OnOK()
{
    TStartingHeroBonusDlg::OnOK();
    _m_spell = _m_spellCombo.GetItemData(_m_spellCombo.GetCurSel());
}

VA(0x0041e6d0, 0x6)
BEGIN_MESSAGE_MAP(TStartingBonusCreatureDlg, TStartingHeroBonusDlg)
    ON_EN_KILLFOCUS(IDC_STACK_QUANTITY_EDIT, OnKillFocusQuantityEdit)
END_MESSAGE_MAP()

VA(0x0041e6e0, 0xaa)
TStartingBonusCreatureDlg::TStartingBonusCreatureDlg(TClient* pClient, const TCampaignScenarioMap* pMap, int player)
    : TStartingHeroBonusDlg(pClient, true, pMap, player)
{
}

VA_COMPGEN(0x0041e7b0, 0x1e, SCALAR_DELETING_DTOR, TStartingBonusCreatureDlg)
VA_COMPGEN(0x0041e7d0, 0x7f, IMPLICIT_DTOR, TStartingBonusCreatureDlg)

// The creature list's order: by town (the neutrals last), then level, then
// id.
VA(0x0041e850, 0xb8)
static bool isCreatureBefore(TCreatureType lhs, TCreatureType rhs)
{
    TCreatureTypeTraits lhsTraits = akCreatureTypeTraits[lhs];
    TCreatureTypeTraits rhsTraits = akCreatureTypeTraits[rhs];
    int lhsTown = lhsTraits.townType == eTownNeutral ? kNumTownTypes : lhsTraits.townType;
    int rhsTown = rhsTraits.townType == eTownNeutral ? kNumTownTypes : rhsTraits.townType;
    if (lhsTown != rhsTown)
        return lhsTown < rhsTown;
    if (lhsTraits.level != rhsTraits.level)
        return lhsTraits.level < rhsTraits.level;
    return lhs < rhs;
}

// Inserts a creature in the list's order, its item data the creature.
VA(0x0041e910, 0x80)
static void addCreature(CComboBox& combo, TCreatureType creature)
{
    int index = combo.GetCount();
    while (index > 0 && isCreatureBefore(creature, TCreatureType(combo.GetItemData(index - 1))))
        index--;
    combo.InsertString(index, akCreatureTypeTraits[creature].m_name);
    combo.SetItemData(index, creature);
}

VA(0x0041e990, 0xe1)
void TStartingBonusCreatureDlg::setBonus(const TScenarioBonusCreature& bonus)
{
    TStartingHeroBonusDlg::setBonus(bonus);
    _m_stack = bonus.m_stack;
    int index = 0;
    while (_m_creatureCombo.GetItemData(index) != _m_stack.getCreatureType())
        index++;
    _m_creatureCombo.SetCurSel(index);
    CString text;
    text.Format("%d", _m_stack.getQuantity());
    _m_quantityEdit.SetWindowText(text);
}

VA(0x0041ea80, 0xb4)
std::auto_ptr<TScenarioStartingBonus> TStartingBonusCreatureDlg::getBonus() const
{
    std::auto_ptr<TScenarioStartingBonus> pBonus(
        new TScenarioBonusCreature(_m_hero, _m_stack));
    if (pBonus.get() == NULL)
        throw TAllocationFailure();
    return pBonus;
}

VA(0x0041eb40, 0x49)
void TStartingBonusCreatureDlg::DoDataExchange(CDataExchange* pDX)
{
    TStartingHeroBonusDlg::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CREATURE_COMBO, _m_creatureCombo);
    DDX_Control(pDX, IDC_STACK_QUANTITY_EDIT, _m_quantityEdit);
    DDX_Control(pDX, IDC_STACK_QUANTITY_SPIN, _m_quantitySpin);
}

VA(0x0041eb90, 0xc9)
BOOL TStartingBonusCreatureDlg::OnInitDialog()
{
    GetDlgItem(IDC_CREATURE_TYPE_STATIC)->SetWindowText(SEditStartingBonusDlgText::kCreatureStaticStr);
    GetDlgItem(IDC_QUANTITY_STATIC)->SetWindowText(SEditStartingBonusDlgText::kQuantityStaticStr);
    TStartingHeroBonusDlg::OnInitDialog();
    for (unsigned int creature = 0; creature < kNumCreatureTypes; creature++) {
        if (akCreatureTypeTraits[creature].level >= 0)
            addCreature(_m_creatureCombo, TCreatureType(creature));
    }
    _m_creatureCombo.SetCurSel(0);
    _m_quantityEdit.SetWindowText("1");
    _m_quantityEdit.LimitText(TDigits<TCreatureStack::s_kMaxQuantity>::getDigits());
    _m_quantitySpin.SetRange(1, TCreatureStack::s_kMaxQuantity);
    return TRUE;
}

VA(0x0041ec60, 0xf3)
void TStartingBonusCreatureDlg::OnOK()
{
    TStartingHeroBonusDlg::OnOK();
    _m_stack.setCreatureType(TCreatureType(_m_creatureCombo.GetItemData(_m_creatureCombo.GetCurSel())));
    int quantity = 0;
    CString text;
    _m_quantityEdit.GetWindowText(text);
    sscanf(text, "%d", &quantity);
    _m_stack.setQuantity(clamp(1, quantity, int(TCreatureStack::s_kMaxQuantity)));
}

VA(0x0041ed60, 0xd6)
void TStartingBonusCreatureDlg::OnKillFocusQuantityEdit()
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

VA(0x0041ee40, 0x6b)
TStartingBonusBuildingDlg::TStartingBonusBuildingDlg(TClient* pClient, const TCampaignScenarioMap* pMap, int player)
    : TStartingBonusTypeDlg(pClient, true), _m_pMap(pMap), _m_player(player)
{
}

VA_COMPGEN(0x0041eed0, 0x1e, SCALAR_DELETING_DTOR, TStartingBonusBuildingDlg)
VA_COMPGEN(0x0041eef0, 0x4c, IMPLICIT_DTOR, TStartingBonusBuildingDlg)

VA(0x0041ef40, 0x5a)
void TStartingBonusBuildingDlg::setBonus(const TScenarioBonusBuilding& bonus)
{
    _m_building = bonus.m_building;
    int index = 0;
    while (_m_buildingCombo.GetItemData(index) != _m_building)
        index++;
    _m_buildingCombo.SetCurSel(index);
}

VA(0x0041efa0, 0x2b)
bool TStartingBonusBuildingDlg::isAvailable() const
{
    const TCampaignScenarioMap::TPlayerInfo& info = _m_pMap->m_aPlayer[_m_player];
    return info.m_bHasMainTown && info.m_mainTownType != -1;
}

VA(0x0041efd0, 0x9c)
std::auto_ptr<TScenarioStartingBonus> TStartingBonusBuildingDlg::getBonus() const
{
    std::auto_ptr<TScenarioStartingBonus> pBonus(new TScenarioBonusBuilding(_m_building));
    if (pBonus.get() == NULL)
        throw TAllocationFailure();
    return pBonus;
}

VA(0x0041f070, 0x16)
void TStartingBonusBuildingDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_BUILDING_COMBO, _m_buildingCombo);
}

VA(0x0041f090, 0xb7)
BOOL TStartingBonusBuildingDlg::OnInitDialog()
{
    GetDlgItem(IDC_BUILDING_STATIC)->SetWindowText(SEditStartingBonusDlgText::kBuildingStaticStr);
    CDialog::OnInitDialog();
    int townType = _m_pMap->m_aPlayer[_m_player].getMainTownType();
    if (townType != -1) {
        const TTown::TBuildingTraits (&akBuildingTraits)[TTown::s_kNumBuildings] =
            akTownTypeTraits[townType].m_akBuildingTraits;
        for (unsigned int building = 0; building < TTown::s_kNumBuildings; building++) {
            if (!akBuildingTraits[building].isDisallowed())
                _m_buildingCombo.SetItemData(_m_buildingCombo.AddString(akBuildingTraits[building].m_pName),
                                             building);
        }
        _m_buildingCombo.SetCurSel(0);
    }
    return TRUE;
}

VA(0x0041f150, 0x38)
void TStartingBonusBuildingDlg::OnOK()
{
    TStartingBonusTypeDlg::OnOK();
    _m_building = _m_buildingCombo.GetItemData(_m_buildingCombo.GetCurSel());
}

VA(0x0041f190, 0x76)
TStartingBonusArtifactDlg::TStartingBonusArtifactDlg(TClient* pClient, const TCampaignScenarioMap* pMap, int player,
                                                     int campaignVersion)
    : TStartingHeroBonusDlg(pClient, true, pMap, player), _m_campaignVersion(campaignVersion)
{
}

VA_COMPGEN(0x0041f230, 0x1e, SCALAR_DELETING_DTOR, TStartingBonusArtifactDlg)
VA_COMPGEN(0x0041f250, 0x5f, IMPLICIT_DTOR, TStartingBonusArtifactDlg)

VA(0x0041f2b0, 0x69)
void TStartingBonusArtifactDlg::setBonus(const TScenarioBonusArtifact& bonus)
{
    TStartingHeroBonusDlg::setBonus(bonus);
    _m_artifact = bonus.m_artifact;
    int index = 0;
    while (_m_artifactCombo.GetItemData(index) != _m_artifact)
        index++;
    _m_artifactCombo.SetCurSel(index);
}

VA(0x0041f320, 0xa5)
std::auto_ptr<TScenarioStartingBonus> TStartingBonusArtifactDlg::getBonus() const
{
    std::auto_ptr<TScenarioStartingBonus> pBonus(new TScenarioBonusArtifact(_m_hero, _m_artifact));
    if (pBonus.get() == NULL)
        throw TAllocationFailure();
    return pBonus;
}

VA(0x0041f3d0, 0x25)
void TStartingBonusArtifactDlg::DoDataExchange(CDataExchange* pDX)
{
    TStartingHeroBonusDlg::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_ARTIFACT_COMBO, _m_artifactCombo);
}

VA(0x0041f400, 0xe6)
BOOL TStartingBonusArtifactDlg::OnInitDialog()
{
    GetDlgItem(IDC_ARTIFACT_STATIC)->SetWindowText(SEditStartingBonusDlgText::kArtifactStaticStr);
    TStartingHeroBonusDlg::OnInitDialog();
    int numArtifacts;
    if (_m_campaignVersion >= eCampaignVersionShadowOfDeath)
        numArtifacts = 144;
    else
        numArtifacts = _m_campaignVersion >= eCampaignVersionArmageddonsBlade ? 129 : 127;
    for (int artifact = 0; artifact < numArtifacts; artifact++) {
        if (!akArtifactTraits[artifact].m_disabled
            && (!(akArtifactTraits[artifact].m_class & ArtifactClassSpecial) || artifact == ARTIFACT_CATAPULT
                || artifact == ARTIFACT_BALLISTA || artifact == ARTIFACT_AMMO_CART
                || artifact == ARTIFACT_FIRST_AID_TENT))
            _m_artifactCombo.SetItemData(_m_artifactCombo.AddString(akArtifactTraits[artifact].m_name), artifact);
    }
    _m_artifactCombo.SetCurSel(0);
    return TRUE;
}

VA(0x0041f4f0, 0x3b)
void TStartingBonusArtifactDlg::OnOK()
{
    TStartingHeroBonusDlg::OnOK();
    _m_artifact = _m_artifactCombo.GetItemData(_m_artifactCombo.GetCurSel());
}

VA(0x0041f530, 0x6c)
TStartingBonusSpellScrollDlg::TStartingBonusSpellScrollDlg(TClient* pClient, const TCampaignScenarioMap* pMap,
                                                           int player)
    : TStartingHeroBonusDlg(pClient, true, pMap, player)
{
}

VA_COMPGEN(0x0041f5c0, 0x1e, SCALAR_DELETING_DTOR, TStartingBonusSpellScrollDlg)
VA_COMPGEN(0x0041f5e0, 0x5f, IMPLICIT_DTOR, TStartingBonusSpellScrollDlg)

VA(0x0041f640, 0x69)
void TStartingBonusSpellScrollDlg::setBonus(const TScenarioBonusSpellScroll& bonus)
{
    TStartingHeroBonusDlg::setBonus(bonus);
    _m_spell = bonus.m_spell;
    int index = 0;
    while (_m_spellCombo.GetItemData(index) != _m_spell)
        index++;
    _m_spellCombo.SetCurSel(index);
}

VA(0x0041f6b0, 0xa5)
std::auto_ptr<TScenarioStartingBonus> TStartingBonusSpellScrollDlg::getBonus() const
{
    std::auto_ptr<TScenarioStartingBonus> pBonus(new TScenarioBonusSpellScroll(_m_hero, _m_spell));
    if (pBonus.get() == NULL)
        throw TAllocationFailure();
    return pBonus;
}

VA(0x0041f760, 0x25)
void TStartingBonusSpellScrollDlg::DoDataExchange(CDataExchange* pDX)
{
    TStartingHeroBonusDlg::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SCROLL_SPELL_COMBO, _m_spellCombo);
}

VA(0x0041f790, 0xda)
BOOL TStartingBonusSpellScrollDlg::OnInitDialog()
{
    GetDlgItem(IDC_SPELL_STATIC)->SetWindowText(SEditStartingBonusDlgText::kSpellStaticStr);
    _m_spell = SPELL_MAGIC_ARROW;
    TStartingHeroBonusDlg::OnInitDialog();
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        if (!(akSpellTraits[spell].m_flags & 0x2000) && akSpellTraits[spell].m_schoolBits != 0)
            _m_spellCombo.SetItemData(_m_spellCombo.AddString(akSpellTraits[spell].m_name), spell);
    }
    int index = 0;
    while (_m_spellCombo.GetItemData(index) != _m_spell)
        index++;
    _m_spellCombo.SetCurSel(index);
    return TRUE;
}

void TStartingBonusSpellScrollDlg::OnOK()
{
    TStartingHeroBonusDlg::OnOK();
    _m_spell = _m_spellCombo.GetItemData(_m_spellCombo.GetCurSel());
}

DATA(0x00488df8)
static const int g_aakSkillControlIDs[kNumPrimarySkills][2] = {
    { IDC_ATTACK_EDIT, IDC_ATTACK_SPIN },
    { IDC_DEFENSE_EDIT, IDC_DEFENSE_SPIN },
    { IDC_SPELL_POWER_EDIT, IDC_SPELL_POWER_SPIN },
    { IDC_KNOWLEDGE_EDIT, IDC_KNOWLEDGE_SPIN }
};

VA(0x0041f870, 0x8)
void TStartingBonusPrimarySkillDlg::OnChangeAttackEdit()
{
    _onChangeSkillEdit(0);
}

VA(0x0041f880, 0x8)
void TStartingBonusPrimarySkillDlg::OnChangeDefenseEdit()
{
    _onChangeSkillEdit(1);
}

VA(0x0041f890, 0x8)
void TStartingBonusPrimarySkillDlg::OnChangeSpellPowerEdit()
{
    _onChangeSkillEdit(2);
}

VA(0x0041f8a0, 0x8)
void TStartingBonusPrimarySkillDlg::OnChangeKnowledgeEdit()
{
    _onChangeSkillEdit(3);
}

VA(0x0041f8b0, 0x8)
void TStartingBonusPrimarySkillDlg::OnKillFocusAttackEdit()
{
    _onKillFocusSkillEdit(0);
}

VA(0x0041f8c0, 0x8)
void TStartingBonusPrimarySkillDlg::OnKillFocusDefenseEdit()
{
    _onKillFocusSkillEdit(1);
}

VA(0x0041f8d0, 0x8)
void TStartingBonusPrimarySkillDlg::OnKillFocusSpellPowerEdit()
{
    _onKillFocusSkillEdit(2);
}

VA(0x0041f8e0, 0x8)
void TStartingBonusPrimarySkillDlg::OnKillFocusKnowledgeEdit()
{
    _onKillFocusSkillEdit(3);
}

VA(0x0041f8f0, 0x6)
BEGIN_MESSAGE_MAP(TStartingBonusPrimarySkillDlg, TStartingHeroBonusDlg)
    ON_WM_DESTROY()
    ON_EN_CHANGE(IDC_ATTACK_EDIT, OnChangeAttackEdit)
    ON_EN_CHANGE(IDC_DEFENSE_EDIT, OnChangeDefenseEdit)
    ON_EN_CHANGE(IDC_SPELL_POWER_EDIT, OnChangeSpellPowerEdit)
    ON_EN_CHANGE(IDC_KNOWLEDGE_EDIT, OnChangeKnowledgeEdit)
    ON_EN_KILLFOCUS(IDC_ATTACK_EDIT, OnKillFocusAttackEdit)
    ON_EN_KILLFOCUS(IDC_DEFENSE_EDIT, OnKillFocusDefenseEdit)
    ON_EN_KILLFOCUS(IDC_SPELL_POWER_EDIT, OnKillFocusSpellPowerEdit)
    ON_EN_KILLFOCUS(IDC_KNOWLEDGE_EDIT, OnKillFocusKnowledgeEdit)
END_MESSAGE_MAP()

VA(0x0041f900, 0xa4)
TStartingBonusPrimarySkillDlg::TStartingBonusPrimarySkillDlg(TClient* pClient, const TCampaignScenarioMap* pMap,
                                                             int player)
    : TStartingHeroBonusDlg(pClient, false, pMap, player), _m_skills(0), _m_numSkills(0), _m_bControls(false)
{
}

VA_COMPGEN(0x0041f9d0, 0x1e, SCALAR_DELETING_DTOR, TStartingBonusPrimarySkillDlg)
VA_COMPGEN(0x0041f9f0, 0x54, CLASS_CTOR, _TSkillControls)
VA_COMPGEN(0x0041fa50, 0x4c, IMPLICIT_DTOR, _TSkillControls)
VA_COMPGEN(0x0041faa0, 0x75, IMPLICIT_DTOR, TStartingBonusPrimarySkillDlg)

VA(0x0041fb20, 0xc8)
void TStartingBonusPrimarySkillDlg::setBonus(const TScenarioBonusPrimarySkill& bonus)
{
    TStartingHeroBonusDlg::setBonus(bonus);
    _m_numSkills = 0;
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++) {
        _m_skills[skill] = bonus.m_skills[skill];
        _m_aSkillControls[skill].m_edit.SetWindowText(TFormattedString("%d", _m_skills[skill]));
        if (_m_skills[skill] > 0)
            _m_numSkills++;
    }
    setBValid(_m_numSkills > 0);
}

VA(0x0041fbf0, 0xae)
std::auto_ptr<TScenarioStartingBonus> TStartingBonusPrimarySkillDlg::getBonus() const
{
    std::auto_ptr<TScenarioStartingBonus> pBonus(new TScenarioBonusPrimarySkill(_m_hero, _m_skills));
    if (pBonus.get() == NULL)
        throw TAllocationFailure();
    return pBonus;
}

VA(0x0041fca0, 0x113)
void TStartingBonusPrimarySkillDlg::_onChangeSkillEdit(unsigned int skill)
{
    if (_m_bControls) {
        CString text;
        _m_aSkillControls[skill].m_edit.GetWindowText(text);
        int value = 0;
        sscanf(text, "%d", &value);
        value = clamp(0, value, int(s_kMaxSkill));
        int oldValue = _m_skills[skill];
        _m_skills[skill] = value;
        if (value > 0) {
            if (oldValue == 0 && _m_numSkills++ == 0)
                setBValid(true);
        } else if (oldValue > 0 && --_m_numSkills == 0) {
            setBValid(false);
        }
    }
}

VA(0x0041fdc0, 0xe2)
void TStartingBonusPrimarySkillDlg::_onKillFocusSkillEdit(unsigned int skill)
{
    CString text;
    CEdit& edit = _m_aSkillControls[skill].m_edit;
    edit.GetWindowText(text);
    int value = 0;
    sscanf(text, "%d", &value);
    if (value < 0 || value > s_kMaxSkill) {
        value = clamp(0, value, int(s_kMaxSkill));
        text.Format("%d", value);
        edit.SetWindowText(text);
    }
}

VA(0x0041feb0, 0x8)
void TStartingBonusPrimarySkillDlg::OnDestroy()
{
    _m_bControls = false;
}

VA(0x0041fec0, 0x54)
void TStartingBonusPrimarySkillDlg::DoDataExchange(CDataExchange* pDX)
{
    TStartingHeroBonusDlg::DoDataExchange(pDX);
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++) {
        DDX_Control(pDX, g_aakSkillControlIDs[skill][0], _m_aSkillControls[skill].m_edit);
        DDX_Control(pDX, g_aakSkillControlIDs[skill][1], _m_aSkillControls[skill].m_spin);
    }
    if (!pDX->m_bSaveAndValidate)
        _m_bControls = true;
}

VA(0x0041ff20, 0x13a)
BOOL TStartingBonusPrimarySkillDlg::OnInitDialog()
{
    GetDlgItem(IDC_ATTACK_STATIC)->SetWindowText(SEditStartingBonusDlgText::kAttackStaticStr);
    GetDlgItem(IDC_DEFENSE_STATIC)->SetWindowText(SEditStartingBonusDlgText::kDefenseStaticStr);
    GetDlgItem(IDC_SPELL_POWER_STATIC)->SetWindowText(SEditStartingBonusDlgText::kSpellPowerStaticStr);
    GetDlgItem(IDC_KNOWLEDGE_STATIC)->SetWindowText(SEditStartingBonusDlgText::kKnowledgeStaticStr);
    TStartingHeroBonusDlg::OnInitDialog();
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++) {
        CEdit& edit = _m_aSkillControls[skill].m_edit;
        edit.LimitText(TDigits<s_kMaxSkill>::getDigits());
        _m_aSkillControls[skill].m_spin.SetRange(0, s_kMaxSkill);
        edit.SetWindowText(TFormattedString("%d", _m_skills[skill]));
    }
    setBValid(_m_numSkills > 0);
    return TRUE;
}

VA(0x00420060, 0x84)
TStartingBonusSecondarySkillDlg::TStartingBonusSecondarySkillDlg(TClient* pClient, const TCampaignScenarioMap* pMap,
                                                                 int player)
    : TStartingHeroBonusDlg(pClient, true, pMap, player)
{
}

VA_COMPGEN(0x00420110, 0x1e, SCALAR_DELETING_DTOR, TStartingBonusSecondarySkillDlg)
VA_COMPGEN(0x00420130, 0x6f, IMPLICIT_DTOR, TStartingBonusSecondarySkillDlg)

VA(0x004201a0, 0xb7)
void TStartingBonusSecondarySkillDlg::setBonus(const TScenarioBonusSecondarySkill& bonus)
{
    TStartingHeroBonusDlg::setBonus(bonus);
    _m_skill = bonus.m_skill;
    _m_mastery = bonus.m_level;
    int index = 0;
    while (_m_skillCombo.GetItemData(index) != _m_skill)
        index++;
    _m_skillCombo.SetCurSel(index);
    index = 0;
    while (_m_masteryCombo.GetItemData(index) != _m_mastery)
        index++;
    _m_masteryCombo.SetCurSel(index);
}

VA(0x00420260, 0xb5)
std::auto_ptr<TScenarioStartingBonus> TStartingBonusSecondarySkillDlg::getBonus() const
{
    std::auto_ptr<TScenarioStartingBonus> pBonus(new TScenarioBonusSecondarySkill(_m_hero, _m_skill, _m_mastery));
    if (pBonus.get() == NULL)
        throw TAllocationFailure();
    return pBonus;
}

VA(0x00420320, 0x37)
void TStartingBonusSecondarySkillDlg::DoDataExchange(CDataExchange* pDX)
{
    TStartingHeroBonusDlg::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SECONDARY_SKILL_COMBO, _m_skillCombo);
    DDX_Control(pDX, IDC_MASTERY_COMBO, _m_masteryCombo);
}

VA(0x00420360, 0xe8)
BOOL TStartingBonusSecondarySkillDlg::OnInitDialog()
{
    GetDlgItem(IDC_SECONDARY_SKILL_STATIC)->SetWindowText(SEditStartingBonusDlgText::kSecondarySkillStaticStr);
    GetDlgItem(IDC_MASTERY_STATIC)->SetWindowText(SEditStartingBonusDlgText::kMasteryStaticStr);
    TStartingHeroBonusDlg::OnInitDialog();
    for (unsigned int skill = 0; skill < kNumSecSkills; skill++)
        _m_skillCombo.SetItemData(_m_skillCombo.AddString(akSSkillTraits[skill].m_name), skill);
    _m_skillCombo.SetCurSel(0);
    for (unsigned int mastery = eMasteryBasic; mastery < kNumMasteries; mastery++)
        _m_masteryCombo.SetItemData(_m_masteryCombo.AddString(akHeroSkillMasteryTraits[mastery - eMasteryBasic].m_name),
                                    mastery);
    _m_masteryCombo.SetCurSel(0);
    return TRUE;
}

VA(0x00420450, 0x64)
void TStartingBonusSecondarySkillDlg::OnOK()
{
    TStartingHeroBonusDlg::OnOK();
    _m_skill = _m_skillCombo.GetItemData(_m_skillCombo.GetCurSel());
    _m_mastery = _m_masteryCombo.GetItemData(_m_masteryCombo.GetCurSel());
}

VA(0x004204c0, 0x6)
BEGIN_MESSAGE_MAP(TStartingBonusResourceDlg, TStartingBonusTypeDlg)
    ON_WM_DESTROY()
    ON_EN_KILLFOCUS(IDC_AMOUNT_EDIT, OnKillFocusAmountEdit)
END_MESSAGE_MAP()

VA(0x004204d0, 0x9c)
TStartingBonusResourceDlg::TStartingBonusResourceDlg(TClient* pClient)
    : TStartingBonusTypeDlg(pClient, true), _m_resource(eResourceGold), _m_amount(1), _m_bInitialized(false)
{
}

VA_COMPGEN(0x00420590, 0x1e, SCALAR_DELETING_DTOR, TStartingBonusResourceDlg)
VA_COMPGEN(0x004205b0, 0x6c, IMPLICIT_DTOR, TStartingBonusResourceDlg)

VA(0x00420620, 0xc0)
void TStartingBonusResourceDlg::setBonus(const TScenarioBonusResource& bonus)
{
    _m_resource = bonus.m_resource;
    _m_amount = bonus.m_amount;
    int index = 0;
    while (_m_resourceCombo.GetItemData(index) != _m_resource)
        index++;
    _m_resourceCombo.SetCurSel(index);
    _m_amountEdit.SetWindowText(TFormattedString("%d", _m_amount));
}

VA(0x004206e0, 0xab)
std::auto_ptr<TScenarioStartingBonus> TStartingBonusResourceDlg::getBonus() const
{
    std::auto_ptr<TScenarioStartingBonus> pBonus(new TScenarioBonusResource(_m_resource, _m_amount));
    if (pBonus.get() == NULL)
        throw TAllocationFailure();
    return pBonus;
}

VA(0x00420790, 0x40)
void TStartingBonusResourceDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_RESOURCE_COMBO, _m_resourceCombo);
    DDX_Control(pDX, IDC_AMOUNT_EDIT, _m_amountEdit);
    DDX_Control(pDX, IDC_AMOUNT_SPIN, _m_amountSpin);
}

VA(0x004207d0, 0x205)
BOOL TStartingBonusResourceDlg::OnInitDialog()
{
    GetDlgItem(IDC_RESOURCE_STATIC)->SetWindowText(SEditStartingBonusDlgText::kResourceStaticStr);
    GetDlgItem(IDC_QUANTITY_STATIC)->SetWindowText(SEditStartingBonusDlgText::kQuantityStaticStr);
    CDialog::OnInitDialog();
    for (unsigned int resource = 0; resource < kNumGameResourceTypes; resource++)
        _m_resourceCombo.SetItemData(_m_resourceCombo.AddString(akGameResourceTypeTraits[resource].m_name), resource);
    {
        TFormattedString name("%s %s %s", akGameResourceTypeTraits[eResourceWood].m_name, kAndStr,
                              akGameResourceTypeTraits[eResourceOre].m_name);
        _m_resourceCombo.SetItemData(_m_resourceCombo.AddString(name), TScenarioBonusResource::kWoodAndOre);
    }
    {
        TFormattedString name("%s, %s, %s %s %s", akGameResourceTypeTraits[eResourceMercury].m_name,
                              akGameResourceTypeTraits[eResourceSulfur].m_name,
                              akGameResourceTypeTraits[eResourceCrystal].m_name, kAndStr,
                              akGameResourceTypeTraits[eResourceGems].m_name);
        _m_resourceCombo.SetItemData(_m_resourceCombo.AddString(name), TScenarioBonusResource::kRareResources);
    }
    int index = 0;
    while (_m_resourceCombo.GetItemData(index) != _m_resource)
        index++;
    _m_resourceCombo.SetCurSel(index);
    _m_amountEdit.LimitText(TDigits<s_kMaxAmount>::getDigits());
    _m_amountSpin.SetRange(1, UD_MAXVAL);
    _m_amountEdit.SetWindowText(TFormattedString("%d", _m_amount));
    _m_bInitialized = true;
    return TRUE;
}

VA(0x004209e0, 0xe3)
void TStartingBonusResourceDlg::OnOK()
{
    TStartingBonusTypeDlg::OnOK();
    _m_resource = _m_resourceCombo.GetItemData(_m_resourceCombo.GetCurSel());
    int amount = 0;
    CString text;
    _m_amountEdit.GetWindowText(text);
    sscanf(text, "%d", &amount);
    _m_amount = clamp(1, amount, int(s_kMaxAmount));
}

VA(0x00420ad0, 0x9)
void TStartingBonusResourceDlg::OnDestroy()
{
    _m_bInitialized = false;
    TStartingBonusTypeDlg::OnDestroy();
}

VA(0x00420ae0, 0xd6)
void TStartingBonusResourceDlg::OnKillFocusAmountEdit()
{
    int amount = 0;
    CString text;
    _m_amountEdit.GetWindowText(text);
    sscanf(text, "%d", &amount);
    if (amount < 1 || amount > s_kMaxAmount) {
        amount = clamp(1, amount, int(s_kMaxAmount));
        text.Format("%d", amount);
        _m_amountEdit.SetWindowText(text);
    }
}

VA(0x00420bc0, 0x12c)
TEditStartingBonusDlg::_TTypeDlgs::_TTypeDlgs(TStartingBonusTypeDlg::TClient* pClient,
                                              const TCampaignScenarioMap* pMap, int player, int campaignVersion)
    : m_spellDlg(pClient, pMap, player), m_creatureDlg(pClient, pMap, player), m_buildingDlg(pClient, pMap, player),
      m_artifactDlg(pClient, pMap, player, campaignVersion), m_spellScrollDlg(pClient, pMap, player),
      m_primarySkillDlg(pClient, pMap, player), m_secondarySkillDlg(pClient, pMap, player),
      m_resourceDlg(pClient)
{
    m_apDlg[eBonusSpell] = &m_spellDlg;
    m_apDlg[eBonusCreature] = &m_creatureDlg;
    m_apDlg[eBonusBuilding] = &m_buildingDlg;
    m_apDlg[eBonusArtifact] = &m_artifactDlg;
    m_apDlg[eBonusSpellScroll] = &m_spellScrollDlg;
    m_apDlg[eBonusPrimarySkill] = &m_primarySkillDlg;
    m_apDlg[eBonusSecondarySkill] = &m_secondarySkillDlg;
    m_apDlg[eBonusResource] = &m_resourceDlg;
}

DATA(0x00488e68)
static const int g_akTypeRadioIDs[kNumBonusTypes] = {
    IDC_SPELL_RADIO,
    IDC_CREATURE_RADIO,
    IDC_BUILDING_RADIO,
    IDC_ARTIFACT_RADIO,
    IDC_SPELL_SCROLL_RADIO,
    IDC_PRIMARY_SKILL_RADIO,
    IDC_SECONDARY_SKILL_RADIO,
    IDC_RESOURCE_RADIO
};

VA(0x00420cf0, 0x2e4)
TEditStartingBonusDlg::TEditStartingBonusDlg(CWnd* pParent, const TCampaignScenarioMap* pMap, int player,
                                             int campaignVersion, const TScenarioStartingBonus* pBonus)
    : CDialog(TEditStartingBonusDlg::IDD, pParent), _m_bInactive(true)
{
    _m_type = -1;
    if (pBonus != NULL)
        _m_pBonus = pBonus->clone();
    else
        _m_pBonus = std::auto_ptr<TScenarioStartingBonus>(
            new TScenarioBonusSpell(TScenarioHeroBonus::kMostPowerfulHero, SPELL_MAGIC_ARROW));
    if (_m_pBonus.get() == NULL)
        throw TAllocationFailure();
    _m_pTypeDlgs = std::auto_ptr<_TTypeDlgs>(new _TTypeDlgs(this, pMap, player, campaignVersion));
    if (_m_pTypeDlgs.get() == NULL)
        throw TAllocationFailure();
}

VA_COMPGEN(0x00420fe0, 0x1e, SCALAR_DELETING_DTOR, TEditStartingBonusDlg)
VA(0x00421000, 0x1e3)
TEditStartingBonusDlg::~TEditStartingBonusDlg()
{
}

VA(0x004211f0, 0x11e)
std::auto_ptr<TScenarioStartingBonus> TEditStartingBonusDlg::releaseBonus()
{
    std::auto_ptr<TScenarioStartingBonus> pBonus(_m_pBonus);
    _m_pBonus = std::auto_ptr<TScenarioStartingBonus>(
        new TScenarioBonusSpell(TScenarioHeroBonus::kMostPowerfulHero, SPELL_MAGIC_ARROW));
    if (_m_pBonus.get() == NULL)
        throw TAllocationFailure();
    return pBonus;
}

VA(0x00421310, 0x60)
void TEditStartingBonusDlg::_setType(int type)
{
    _m_pTypeDlgs->m_apDlg[_m_type]->ShowWindow(SW_HIDE);
    _m_type = type;
    UpdateData(FALSE);
    TStartingBonusTypeDlg* pDlg = _m_pTypeDlgs->m_apDlg[_m_type];
    pDlg->ShowWindow(SW_SHOW);
    _m_okButton.EnableWindow(pDlg->getBValid());
}

VA(0x00421370, 0x1e)
void TEditStartingBonusDlg::visit(const TScenarioBonusSpell& bonus)
{
    _m_pTypeDlgs->m_spellDlg.setBonus(bonus);
    _setType(eBonusSpell);
}

VA(0x00421390, 0x24)
void TEditStartingBonusDlg::visit(const TScenarioBonusCreature& bonus)
{
    _m_pTypeDlgs->m_creatureDlg.setBonus(bonus);
    _setType(eBonusCreature);
}

VA(0x004213c0, 0x24)
void TEditStartingBonusDlg::visit(const TScenarioBonusBuilding& bonus)
{
    _m_pTypeDlgs->m_buildingDlg.setBonus(bonus);
    _setType(eBonusBuilding);
}

VA(0x004213f0, 0x24)
void TEditStartingBonusDlg::visit(const TScenarioBonusArtifact& bonus)
{
    _m_pTypeDlgs->m_artifactDlg.setBonus(bonus);
    _setType(eBonusArtifact);
}

VA(0x00421440, 0x24)
void TEditStartingBonusDlg::visit(const TScenarioBonusSpellScroll& bonus)
{
    _m_pTypeDlgs->m_spellScrollDlg.setBonus(bonus);
    _setType(eBonusSpellScroll);
}

VA(0x00421470, 0x24)
void TEditStartingBonusDlg::visit(const TScenarioBonusPrimarySkill& bonus)
{
    _m_pTypeDlgs->m_primarySkillDlg.setBonus(bonus);
    _setType(eBonusPrimarySkill);
}

VA(0x004214a0, 0x24)
void TEditStartingBonusDlg::visit(const TScenarioBonusSecondarySkill& bonus)
{
    _m_pTypeDlgs->m_secondarySkillDlg.setBonus(bonus);
    _setType(eBonusSecondarySkill);
}

VA(0x004214d0, 0x24)
void TEditStartingBonusDlg::visit(const TScenarioBonusResource& bonus)
{
    _m_pTypeDlgs->m_resourceDlg.setBonus(bonus);
    _setType(eBonusResource);
}

VA(0x00421500, 0x50)
void TEditStartingBonusDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDOK, _m_okButton);
    DDX_Radio(pDX, IDC_SPELL_RADIO, _m_type);
    for (unsigned int type = 0; type < kNumBonusTypes; type++)
        DDX_Control(pDX, g_akTypeRadioIDs[type], _m_aTypeRadios[type]);
}

VA(0x00421550, 0x6)
BEGIN_MESSAGE_MAP(TEditStartingBonusDlg, CDialog)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_SPELL_RADIO, OnTypeRadio)
    ON_BN_CLICKED(IDC_CREATURE_RADIO, OnTypeRadio)
    ON_BN_CLICKED(IDC_BUILDING_RADIO, OnTypeRadio)
    ON_BN_CLICKED(IDC_ARTIFACT_RADIO, OnTypeRadio)
    ON_BN_CLICKED(IDC_SPELL_SCROLL_RADIO, OnTypeRadio)
    ON_BN_CLICKED(IDC_PRIMARY_SKILL_RADIO, OnTypeRadio)
    ON_BN_CLICKED(IDC_SECONDARY_SKILL_RADIO, OnTypeRadio)
    ON_BN_CLICKED(IDC_RESOURCE_RADIO, OnTypeRadio)
END_MESSAGE_MAP()

VA(0x00421560, 0x52)
int TEditStartingBonusDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(SEditStartingBonusDlgText::kCaptionStr);
    for (unsigned int type = 0; type < kNumBonusTypes; type++) {
        if (!_m_pTypeDlgs->m_apDlg[type]->create(this))
            return -1;
    }
    return 0;
}

VA(0x004215c0, 0x33)
void TEditStartingBonusDlg::OnDestroy()
{
    for (unsigned int type = kNumBonusTypes; type > 0; )
        _m_pTypeDlgs->m_apDlg[--type]->DestroyWindow();
    CDialog::OnDestroy();
    _m_bInactive = true;
}

VA(0x00421600, 0x23a)
BOOL TEditStartingBonusDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_TYPE_STATIC)->SetWindowText(SEditStartingBonusDlgText::kTypeStaticStr);
    GetDlgItem(IDC_SPELL_RADIO)->SetWindowText(SEditStartingBonusDlgText::kSpellRadioStr);
    GetDlgItem(IDC_CREATURE_RADIO)->SetWindowText(SEditStartingBonusDlgText::kCreatureRadioStr);
    GetDlgItem(IDC_BUILDING_RADIO)->SetWindowText(SEditStartingBonusDlgText::kBuildingRadioStr);
    GetDlgItem(IDC_ARTIFACT_RADIO)->SetWindowText(SEditStartingBonusDlgText::kArtifactRadioStr);
    GetDlgItem(IDC_SPELL_SCROLL_RADIO)->SetWindowText(SEditStartingBonusDlgText::kSpellScrollRadioStr);
    GetDlgItem(IDC_PRIMARY_SKILL_RADIO)->SetWindowText(SEditStartingBonusDlgText::kPrimarySkillRadioStr);
    GetDlgItem(IDC_SECONDARY_SKILL_RADIO)->SetWindowText(SEditStartingBonusDlgText::kSecondarySkillRadioStr);
    GetDlgItem(IDC_RESOURCE_RADIO)->SetWindowText(SEditStartingBonusDlgText::kResourceRadioStr);
    _m_type = eBonusSpell;
    CDialog::OnInitDialog();
    CWnd* pFrame = GetDlgItem(IDC_BONUS_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    const CWnd* pInsertAfter = pFrame;
    unsigned int type;
    for (type = 0; type < kNumBonusTypes; type++) {
        TStartingBonusTypeDlg* pDlg = _m_pTypeDlgs->m_apDlg[type];
        pDlg->SetWindowPos(pInsertAfter, rect.left, rect.top, 0, 0, SWP_NOSIZE);
        pInsertAfter = pDlg;
    }
    pFrame->DestroyWindow();
    for (type = 0; type < kNumBonusTypes; type++)
        _m_aTypeRadios[type].EnableWindow(_m_pTypeDlgs->m_apDlg[type]->isAvailable());
    _m_pTypeDlgs->m_apDlg[eBonusSpell]->ShowWindow(SW_SHOW);
    _m_pBonus->accept(*this);
    _m_bInactive = false;
    return TRUE;
}

VA(0x00421840, 0xc0)
void TEditStartingBonusDlg::OnOK()
{
    CDialog::OnOK();
    TStartingBonusTypeDlg* pDlg = _m_pTypeDlgs->m_apDlg[_m_type];
    pDlg->OnOK();
    _m_pBonus = pDlg->getBonus();
}

VA(0x00421900, 0x54)
void TEditStartingBonusDlg::OnTypeRadio()
{
    _m_pTypeDlgs->m_apDlg[_m_type]->ShowWindow(SW_HIDE);
    UpdateData();
    TStartingBonusTypeDlg* pDlg = _m_pTypeDlgs->m_apDlg[_m_type];
    pDlg->ShowWindow(SW_SHOW);
    _m_okButton.EnableWindow(pDlg->getBValid());
}

VA(0x00421960, 0x1f)
void TEditStartingBonusDlg::onBValidChange(TStartingBonusTypeDlg* pDlg, bool bValid)
{
    if (!_m_bInactive)
        _m_okButton.EnableWindow(bValid);
}

VA_COMPGEN(0x004219a0, 0x214, IMPLICIT_DTOR, _TTypeDlgs)
