// ScenarioPropsStartingOptionsPage.cpp - the starting options page of the
// scenario properties sheet (h3ccmped 0x42e280..0x432460). Each option type
// has a modeless child dialog over the page's frame: a list of up to three
// bonuses for one player, of up to three crossover choices, or of up to
// three starting hero choices, each edited in its own modal dialog. The
// page writes the shown dialog's options into the scenario when it is left.
#include "campaign_editor/stdafx.h"

#include <vector>

#include "armygrp.h"
#include "herospec.h"
#include "artifact.h"
#include "artifact_type.h"
#include "sskilltraits.h"
#include "va.h"
#include "editor/FormattedString.h"
#include "editor/GameResource.h"
#include "editor/Hero.h"
#include "editor/Player.h"
#include "editor/Town.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/EditCrossoverScenarioOptionDlg.h"
#include "campaign_editor/EditStartingBonusDlg.h"
#include "campaign_editor/EditStartingHeroOptionDlg.h"
#include "campaign_editor/ScenarioPropsStartingOptionsPage.h"

namespace {

// The most choices one starting option offers.
enum { kMaxChoices = 3 };

// A bonus's line in the bonus list: what it gives and to which hero.
class TGetScenarioStartingBonusNameFunc : public TScenarioStartingBonus::TVisitor {
public:
    TGetScenarioStartingBonusNameFunc(const TScenario& scenario, int player);

    CString operator()(const TScenarioStartingBonus& bonus);

private:
    virtual void visit(const TScenarioBonusSpell& bonus);
    virtual void visit(const TScenarioBonusCreature& bonus);
    virtual void visit(const TScenarioBonusBuilding& bonus);
    virtual void visit(const TScenarioBonusArtifact& bonus);
    virtual void visit(const TScenarioBonusSpellScroll& bonus);
    virtual void visit(const TScenarioBonusPrimarySkill& bonus);
    virtual void visit(const TScenarioBonusSecondarySkill& bonus);
    virtual void visit(const TScenarioBonusResource& bonus);

    const char* _getHeroName(const TScenarioHeroBonus& bonus) const;

    const TScenario& _m_scenario;
    int _m_player;
    CString _m_name;
};

// One option type's controls: a modeless child dialog of the page.
class TScenarioStartingOptionsTypeDlg : public CDialog {
public:
    virtual BOOL create(CWnd* pParent) = 0;
    // The page becomes active: offer what the scenario's map allows.
    virtual void prepare() = 0;
    // The page is left: forget the options and the map.
    virtual void clear() = 0;
    virtual void hide();
    virtual void show();
    virtual std::auto_ptr<TScenarioStartingOptions> getOptions() const = 0;
    // Whether the scenario can have options of the type.
    virtual bool isAvailable() const { return true; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX) {}
};

// Bonuses one player chooses from.
class TScenarioOptionsBonusDlg : public TScenarioStartingOptionsTypeDlg {
public:
    TScenarioOptionsBonusDlg(const TScenario& scenario, int campaignVersion);

    void setOptions(const TScenarioOptionsBonus& options);

    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual void prepare();
    virtual void clear();
    virtual void hide();
    virtual std::auto_ptr<TScenarioStartingOptions> getOptions() const;

    enum { IDD = IDD_STARTING_OPTIONS_BONUS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnEnable(BOOL bEnable);
    afx_msg void OnAddButton();
    afx_msg void OnEditButton();
    afx_msg void OnRemoveButton();
    afx_msg void OnSelChangePlayerCombo();
    afx_msg void OnSelChangeBonusList();
    afx_msg void OnDblClkBonusList();
    afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
    DECLARE_MESSAGE_MAP()

private:
    void _addBonus(std::auto_ptr<TScenarioStartingBonus> pBonus);
    void _insertBonus(int index, std::auto_ptr<TScenarioStartingBonus> pBonus);
    void _removeBonus(int index);

    const TScenario& _m_scenario;
    int _m_campaignVersion;
    int _m_player;
    std::vector<std::auto_ptr<TScenarioStartingBonus> > _m_bonuses;
    CComboBox _m_playerCombo;
    CListBox _m_bonusList;
    CButton _m_addButton;
    CButton _m_editButton;
    CButton _m_removeButton;
};

// Earlier scenarios a player's heroes come from.
class TScenarioOptionsCrossoverScenarioDlg : public TScenarioStartingOptionsTypeDlg {
public:
    TScenarioOptionsCrossoverScenarioDlg(const TCampaign& campaign, int scenario);

    void setOptions(const TScenarioOptionsCrossoverScenario& options);

    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual void prepare();
    virtual void clear();
    virtual void hide();
    virtual std::auto_ptr<TScenarioStartingOptions> getOptions() const;
    virtual bool isAvailable() const;

    enum { IDD = IDD_STARTING_OPTIONS_CROSSOVER_SCENARIO };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnAddButton();
    afx_msg void OnEditButton();
    afx_msg void OnRemoveButton();
    afx_msg void OnEnable(BOOL bEnable);
    afx_msg void OnSelChangeChoiceList();
    afx_msg void OnDblClkChoiceList();
    afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
    DECLARE_MESSAGE_MAP()

private:
    void _insertChoice(int index, const TScenarioOptionsCrossoverScenario::TChoice& choice);
    void _removeChoice(int index);
    // The players the scenario's map lets a human play.
    TPlayerMask _getPlayers() const;

    const TCampaign& _m_campaign;
    int _m_scenario;
    // The prerequisite scenarios with a map no choice names yet.
    std::vector<bool> _m_abScenarioAvailable;
    unsigned int _m_numScenariosAvailable;
    std::vector<TScenarioOptionsCrossoverScenario::TChoice> _m_choices;
    CListBox _m_choiceList;
    CButton _m_addButton;
    CButton _m_editButton;
    CButton _m_removeButton;
};

// Heroes a player starts with.
class TScenarioOptionsStartingHeroDlg : public TScenarioStartingOptionsTypeDlg {
public:
    explicit TScenarioOptionsStartingHeroDlg(const TScenario& scenario);

    void setOptions(const TScenarioOptionsStartingHero& options);

    virtual BOOL create(CWnd* pParent) { return Create(IDD, pParent); }
    virtual void prepare();
    virtual void clear();
    virtual void hide();
    virtual std::auto_ptr<TScenarioStartingOptions> getOptions() const;
    virtual bool isAvailable() const;

    enum { IDD = IDD_STARTING_OPTIONS_STARTING_HERO };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnEnable(BOOL bEnable);
    afx_msg void OnAddButton();
    afx_msg void OnEditButton();
    afx_msg void OnRemoveButton();
    afx_msg void OnSelChangeChoiceList();
    afx_msg void OnDblClkChoiceList();
    afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
    DECLARE_MESSAGE_MAP()

private:
    void _insertChoice(int index, const TScenarioOptionsStartingHero::TChoice& choice);
    void _removeChoice(int index);

    const TScenario& _m_scenario;
    std::vector<TScenarioOptionsStartingHero::TChoice> _m_choices;
    // The map's heroes no choice names yet, and whether the random hero is
    // still free.
    TEditStartingHeroOptionDlg::THeroMask _m_heroesAvailable;
    bool _m_bRandomHeroAvailable;
    CListBox _m_choiceList;
    CButton _m_addButton;
    CButton _m_editButton;
    CButton _m_removeButton;
};

}

// The type dialogs, in radio order.
struct TScenarioPropsStartingOptionsPage::_TTypeDlgs {
    _TTypeDlgs(const TCampaign& campaign, int scenario, int campaignVersion);

    TScenarioOptionsBonusDlg m_bonusDlg;
    TScenarioOptionsCrossoverScenarioDlg m_crossoverScenarioDlg;
    TScenarioOptionsStartingHeroDlg m_startingHeroDlg;
    TScenarioStartingOptionsTypeDlg* m_apDlg[kNumTypes];
};

VA(0x0042e2b0, 0x28)
TGetScenarioStartingBonusNameFunc::TGetScenarioStartingBonusNameFunc(const TScenario& scenario, int player)
    : _m_scenario(scenario), _m_player(player)
{
}

VA(0x0042e2e0, 0xa5)
CString TGetScenarioStartingBonusNameFunc::operator()(const TScenarioStartingBonus& bonus)
{
    bonus.accept(*this);
    CString name(_m_name);
    _m_name = CString();
    return name;
}

VA(0x0042e390, 0x92)
const char* TGetScenarioStartingBonusNameFunc::_getHeroName(const TScenarioHeroBonus& bonus) const
{
    int hero = bonus.m_hero;
    if (hero == TScenarioHeroBonus::kMostPowerfulHero)
        return kMostPowerfulHeroStr;
    if (hero == TScenarioHeroBonus::kGeneratedHero)
        return kGeneratedHeroStr;
    return _m_scenario.getPlayer(_m_player).m_heroes.find(hero)->second.c_str();
}

VA(0x0042e430, 0x97)
void TGetScenarioStartingBonusNameFunc::visit(const TScenarioBonusSpell& bonus)
{
    _m_name.Format(kGiveBonusFmtStr, (const char*)TFormattedString(kSpellBonusFmtStr, akSpellTraits[bonus.m_spell].m_name),
                   _getHeroName(bonus));
}

// The range test admits one id past the 150 rows of akCreatureTypeTraits.
VA(0x0042e4d0, 0xc4)
void TGetScenarioStartingBonusNameFunc::visit(const TScenarioBonusCreature& bonus)
{
    int creature = bonus.m_stack.getCreatureType();
    unsigned int quantity = bonus.m_stack.getQuantity();
    _m_name.Format(kGiveBonusFmtStr,
                   (const char*)TFormattedString(
                       "%d %s", quantity,
                       creature >= 0 && creature <= 150
                           ? (quantity == 1 ? akCreatureTypeTraits[creature].m_name
                                            : akCreatureTypeTraits[creature].m_plural_name)
                           : ""),
                   _getHeroName(bonus));
}

VA(0x0042e5a0, 0x50)
void TGetScenarioStartingBonusNameFunc::visit(const TScenarioBonusBuilding& bonus)
{
    int townType = _m_scenario.getPlayer(_m_player).m_mainTownType;
    _m_name.Format(kBuildBonusFmtStr, akTownTypeTraits[townType].m_akBuildingTraits[bonus.m_building].m_pName);
}

VA(0x0042e5f0, 0x38)
void TGetScenarioStartingBonusNameFunc::visit(const TScenarioBonusArtifact& bonus)
{
    int artifact = bonus.m_artifact;
    _m_name.Format(kGiveBonusFmtStr, akArtifactTraits[artifact].m_name, _getHeroName(bonus));
}

VA(0x0042e630, 0x9d)
void TGetScenarioStartingBonusNameFunc::visit(const TScenarioBonusSpellScroll& bonus)
{
    _m_name.Format(kGiveBonusFmtStr,
                   (const char*)TFormattedString("%s %s", akSpellTraits[bonus.m_spell].m_name,
                                                 akArtifactTraits[ARTIFACT_SPELL_SCROLL].m_name),
                   _getHeroName(bonus));
}

VA(0x0042e6d0, 0x127)
void TGetScenarioStartingBonusNameFunc::visit(const TScenarioBonusPrimarySkill& bonus)
{
    CString skills;
    unsigned int skill;
    for (skill = 0; bonus.m_skills[skill] <= 0; skill++)
        ;
    skills = TFormattedString("+%d %s", bonus.m_skills[skill], akHeroPrimarySkillTraits[skill].m_name);
    for (skill++; skill < kNumPrimarySkills; skill++) {
        if (bonus.m_skills[skill] > 0)
            skills += TFormattedString(", +%d %s", bonus.m_skills[skill], akHeroPrimarySkillTraits[skill].m_name);
    }
    _m_name.Format(kGiveBonusFmtStr, (const char*)skills, _getHeroName(bonus));
}

VA(0x0042e800, 0x9c)
void TGetScenarioStartingBonusNameFunc::visit(const TScenarioBonusSecondarySkill& bonus)
{
    _m_name.Format(kGiveBonusFmtStr,
                   (const char*)TFormattedString("%s %s", akHeroSkillMasteryTraits[bonus.m_level - eMasteryBasic].m_name,
                                                 akSSkillTraits[bonus.m_skill].m_name),
                   _getHeroName(bonus));
}

VA(0x0042e8a0, 0xec)
void TGetScenarioStartingBonusNameFunc::visit(const TScenarioBonusResource& bonus)
{
    CString resource;
    if (bonus.m_resource == TScenarioBonusResource::kWoodAndOre)
        resource.Format("%s %s %s", akGameResourceTypeTraits[eResourceWood].m_name, kAndStr,
                        akGameResourceTypeTraits[eResourceOre].m_name);
    else if (bonus.m_resource == TScenarioBonusResource::kRareResources)
        resource.Format("%s, %s, %s %s %s", akGameResourceTypeTraits[eResourceMercury].m_name,
                        akGameResourceTypeTraits[eResourceSulfur].m_name,
                        akGameResourceTypeTraits[eResourceCrystal].m_name, kAndStr,
                        akGameResourceTypeTraits[eResourceGems].m_name);
    else
        resource = akGameResourceTypeTraits[bonus.m_resource].m_name;
    _m_name.Format(kReceiveBonusFmtStr, bonus.m_amount, resource);
}

VA(0x0042e990, 0x8)
void TScenarioStartingOptionsTypeDlg::hide()
{
    ShowWindow(SW_HIDE);
}

VA(0x0042e9a0, 0x8)
void TScenarioStartingOptionsTypeDlg::show()
{
    ShowWindow(SW_SHOW);
}

VA(0x0042e9b0, 0x6)
BEGIN_MESSAGE_MAP(TScenarioOptionsBonusDlg, TScenarioStartingOptionsTypeDlg)
    ON_WM_ENABLE()
    ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
    ON_BN_CLICKED(IDC_EDIT_BUTTON, OnEditButton)
    ON_BN_CLICKED(IDC_DELETE_BUTTON, OnRemoveButton)
    ON_CBN_SELCHANGE(IDC_PLAYER_COMBO, OnSelChangePlayerCombo)
    ON_LBN_SELCHANGE(IDC_BONUS_LIST, OnSelChangeBonusList)
    ON_LBN_DBLCLK(IDC_BONUS_LIST, OnDblClkBonusList)
    ON_WM_VKEYTOITEM()
END_MESSAGE_MAP()

VA(0x0042e9c0, 0xdd)
TScenarioOptionsBonusDlg::TScenarioOptionsBonusDlg(const TScenario& scenario, int campaignVersion)
    : _m_scenario(scenario), _m_campaignVersion(campaignVersion)
{
}

VA_COMPGEN(0x0042ead0, 0x1e, SCALAR_DELETING_DTOR, TScenarioOptionsBonusDlg)
VA_COMPGEN(0x0042eaf0, 0xcf, IMPLICIT_DTOR, TScenarioOptionsBonusDlg)

VA(0x0042ebe0, 0x14b)
void TScenarioOptionsBonusDlg::setOptions(const TScenarioOptionsBonus& options)
{
    _m_player = options.m_player;
    int index;
    for (index = 0; (int)_m_playerCombo.GetItemData(index) != options.m_player; index++)
        ;
    _m_playerCombo.SetCurSel(index);
    for (unsigned int i = 0; i < options.m_bonuses.size(); i++) {
        std::auto_ptr<TScenarioStartingBonus> pBonus = options.m_bonuses[i]->clone();
        if (pBonus.get() == NULL)
            throw TAllocationFailure();
        _addBonus(pBonus);
    }
    if (_m_bonusList.GetCount() >= kMaxChoices)
        _m_addButton.EnableWindow(FALSE);
}

VA(0x0042ed30, 0x50)
void TScenarioOptionsBonusDlg::_addBonus(std::auto_ptr<TScenarioStartingBonus> pBonus)
{
    _insertBonus(-1, pBonus);
}

VA(0x0042ed80, 0xaf)
void TScenarioOptionsBonusDlg::prepare()
{
    const TCampaignScenarioMap* pMap = _m_scenario.getMap();
    if (pMap != NULL) {
        for (unsigned int player = 0; player < TCampaignScenarioMap::kNumPlayers; player++) {
            if (pMap->m_aPlayer[player].isHumanPlayable())
                _m_playerCombo.SetItemData(_m_playerCombo.AddString(akPlayerTraits[player].m_pName), player);
        }
        _m_playerCombo.SetCurSel(0);
        _m_player = _m_playerCombo.GetItemData(0);
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
    } else
        EnableWindow(FALSE);
}

VA(0x0042ee30, 0xe6)
void TScenarioOptionsBonusDlg::clear()
{
    if (_m_scenario.getMap() != NULL) {
        _m_playerCombo.ResetContent();
        _m_bonusList.ResetContent();
        _m_bonuses.clear();
        _m_addButton.EnableWindow(TRUE);
        _m_editButton.EnableWindow(TRUE);
        _m_removeButton.EnableWindow(TRUE);
    } else
        EnableWindow(TRUE);
}

VA(0x0042ef20, 0x5a)
void TScenarioOptionsBonusDlg::hide()
{
    TScenarioStartingOptionsTypeDlg::hide();
    _m_playerCombo.SetCurSel(0);
    int count = _m_bonusList.GetCount();
    for (int i = 0; i < count; i++) {
        _m_bonusList.SetCurSel(0);
        OnRemoveButton();
    }
}

VA(0x0042ef80, 0x205)
std::auto_ptr<TScenarioStartingOptions> TScenarioOptionsBonusDlg::getOptions() const
{
    std::vector<std::auto_ptr<TScenarioStartingBonus> > bonuses;
    bonuses.reserve(_m_bonuses.size());
    for (unsigned int i = 0; i < _m_bonuses.size(); i++) {
        std::auto_ptr<TScenarioStartingBonus> pBonus = _m_bonuses[i]->clone();
        if (pBonus.get() == NULL)
            throw TAllocationFailure();
        bonuses.push_back(pBonus);
    }
    std::auto_ptr<TScenarioStartingOptions> pOptions(new TScenarioOptionsBonus(_m_player, bonuses));
    if (pOptions.get() == NULL)
        throw TAllocationFailure();
    return pOptions;
}

VA(0x0042f190, 0x14a)
void TScenarioOptionsBonusDlg::_insertBonus(int index, std::auto_ptr<TScenarioStartingBonus> pBonus)
{
    TGetScenarioStartingBonusNameFunc getName(_m_scenario, _m_player);
    index = _m_bonusList.InsertString(index, getName(*pBonus));
    if (index == LB_ERR)
        throw TRuntimeError();
    _m_bonuses.insert(_m_bonuses.begin() + index, pBonus);
}

VA_COMPGEN(0x0042f2e0, 0x8, IMPLICIT_DTOR, TGetScenarioStartingBonusNameFunc)

VA(0x0042f2f0, 0xa4)
void TScenarioOptionsBonusDlg::_removeBonus(int index)
{
    _m_bonuses.erase(_m_bonuses.begin() + index);
    _m_bonusList.DeleteString(index);
}

VA(0x0042f3a0, 0x64)
void TScenarioOptionsBonusDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_PLAYER_COMBO, _m_playerCombo);
    DDX_Control(pDX, IDC_BONUS_LIST, _m_bonusList);
    DDX_Control(pDX, IDC_ADD_BUTTON, _m_addButton);
    DDX_Control(pDX, IDC_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_DELETE_BUTTON, _m_removeButton);
}

VA(0x0042f410, 0x9b)
BOOL TScenarioOptionsBonusDlg::OnInitDialog()
{
    GetDlgItem(IDC_PLAYER_STATIC)->SetWindowText(SScenarioPropsStartingOptionsPageText::kPlayerStaticStr);
    GetDlgItem(IDC_OPTIONS_STATIC)->SetWindowText(SScenarioPropsStartingOptionsPageText::kOptionsStaticStr);
    GetDlgItem(IDC_ADD_BUTTON)->SetWindowText(SScenarioPropsStartingOptionsPageText::kAddButtonStr);
    GetDlgItem(IDC_EDIT_BUTTON)->SetWindowText(SScenarioPropsStartingOptionsPageText::kEditButtonStr);
    GetDlgItem(IDC_DELETE_BUTTON)->SetWindowText(SScenarioPropsStartingOptionsPageText::kRemoveButtonStr);
    CDialog::OnInitDialog();
    return TRUE;
}

void TScenarioOptionsBonusDlg::OnEnable(BOOL bEnable)
{
    for (CWnd* pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetWindow(GW_HWNDNEXT))
        pChild->EnableWindow(bEnable);
    CDialog::OnEnable(bEnable);
}

VA(0x0042f4b0, 0x110)
void TScenarioOptionsBonusDlg::OnAddButton()
{
    TEditStartingBonusDlg dlg(GetParent(), _m_scenario.getMap(), _m_player, _m_campaignVersion, NULL);
    if (dlg.DoModal() == IDOK) {
        std::auto_ptr<TScenarioStartingBonus> pBonus = dlg.releaseBonus();
        _addBonus(pBonus);
        if (_m_bonusList.GetCount() >= kMaxChoices)
            _m_addButton.EnableWindow(FALSE);
    }
}

VA(0x0042f5c0, 0x128)
void TScenarioOptionsBonusDlg::OnEditButton()
{
    int index = _m_bonusList.GetCurSel();
    TEditStartingBonusDlg dlg(GetParent(), _m_scenario.getMap(), _m_player, _m_campaignVersion,
                              _m_bonuses[index].get());
    if (dlg.DoModal() == IDOK) {
        std::auto_ptr<TScenarioStartingBonus> pBonus = dlg.releaseBonus();
        _removeBonus(index);
        _insertBonus(index, pBonus);
        _m_bonusList.SetCurSel(index);
    }
}

VA(0x0042f6f0, 0x84)
void TScenarioOptionsBonusDlg::OnRemoveButton()
{
    int index = _m_bonusList.GetCurSel();
    _removeBonus(index);
    _m_addButton.EnableWindow(TRUE);
    int count = _m_bonusList.GetCount();
    if (count > 0) {
        if (index >= count)
            index = count - 1;
        _m_bonusList.SetCurSel(index);
    } else {
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
    }
}

// A building bonus builds in the player's main town: another player with
// another town type loses them.
VA(0x0042f780, 0x105)
void TScenarioOptionsBonusDlg::OnSelChangePlayerCombo()
{
    int oldPlayer = _m_player;
    _m_player = _m_playerCombo.GetItemData(_m_playerCombo.GetCurSel());
    int oldTownType = _m_scenario.getPlayer(oldPlayer).getMainTownType();
    if (oldTownType != -1 && _m_scenario.getPlayer(_m_player).getMainTownType() != oldTownType) {
        int index = _m_bonusList.GetCount();
        while (index > 0) {
            index--;
            if (dynamic_cast<const TScenarioBonusBuilding*>(_m_bonuses[index].get()) != NULL) {
                if (index == _m_bonusList.GetCurSel())
                    OnRemoveButton();
                else {
                    _removeBonus(index);
                    _m_addButton.EnableWindow(TRUE);
                }
            }
        }
    }
}

VA(0x0042f890, 0x1f)
void TScenarioOptionsBonusDlg::OnSelChangeBonusList()
{
    _m_editButton.EnableWindow(TRUE);
    _m_removeButton.EnableWindow(TRUE);
}

VA(0x0042f8b0, 0x27)
void TScenarioOptionsBonusDlg::OnDblClkBonusList()
{
    if (_m_bonusList.GetCurSel() != LB_ERR)
        OnEditButton();
}

VA(0x0042f8e0, 0x50)
int TScenarioOptionsBonusDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    if (nKey == VK_DELETE && _m_bonusList.GetCurSel() != LB_ERR) {
        OnRemoveButton();
        return -2;
    }
    return TScenarioStartingOptionsTypeDlg::OnVKeyToItem(nKey, pListBox, nIndex);
}

VA(0x0042f930, 0x6)
BEGIN_MESSAGE_MAP(TScenarioOptionsCrossoverScenarioDlg, TScenarioStartingOptionsTypeDlg)
    ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
    ON_BN_CLICKED(IDC_EDIT_BUTTON, OnEditButton)
    ON_BN_CLICKED(IDC_DELETE_BUTTON, OnRemoveButton)
    ON_LBN_SELCHANGE(IDC_CROSSOVER_SCENARIO_LIST, OnSelChangeChoiceList)
    ON_LBN_DBLCLK(IDC_CROSSOVER_SCENARIO_LIST, OnDblClkChoiceList)
    ON_WM_VKEYTOITEM()
END_MESSAGE_MAP()

VA(0x0042f940, 0x11d)
TScenarioOptionsCrossoverScenarioDlg::TScenarioOptionsCrossoverScenarioDlg(const TCampaign& campaign, int scenario)
    : _m_campaign(campaign),
      _m_scenario(scenario),
      _m_abScenarioAvailable(campaign.getMapTraits().m_numRegions, false),
      _m_numScenariosAvailable(0)
{
}

VA_COMPGEN(0x0042fa80, 0x1e, SCALAR_DELETING_DTOR, TScenarioOptionsCrossoverScenarioDlg)
VA_COMPGEN(0x0042faa0, 0xb8, IMPLICIT_DTOR, TScenarioOptionsCrossoverScenarioDlg)

VA(0x0042fb60, 0x65)
void TScenarioOptionsCrossoverScenarioDlg::setOptions(const TScenarioOptionsCrossoverScenario& options)
{
    for (unsigned int i = 0; i < options.m_choices.size(); i++)
        _insertChoice(-1, options.m_choices[i]);
    if (_m_choiceList.GetCount() >= kMaxChoices || _m_numScenariosAvailable == 0)
        _m_addButton.EnableWindow(FALSE);
}

VA(0x0042fbd0, 0xcb)
void TScenarioOptionsCrossoverScenarioDlg::prepare()
{
    if (_m_campaign.getScenario(_m_scenario).getMap() != NULL) {
        for (unsigned int scenario = 0; scenario < _m_campaign.getMapTraits().m_numRegions; scenario++) {
            if (scenario != _m_scenario && _m_campaign.getScenario(scenario).getMap() != NULL
                && _m_campaign.getBPrerequisite(_m_scenario, scenario)) {
                _m_abScenarioAvailable[scenario] = true;
                _m_numScenariosAvailable++;
            }
        }
        _m_addButton.EnableWindow(_m_numScenariosAvailable > 0);
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
    } else
        EnableWindow(FALSE);
}

VA(0x0042fca0, 0xc3)
void TScenarioOptionsCrossoverScenarioDlg::clear()
{
    if (_m_campaign.getScenario(_m_scenario).getMap() != NULL) {
        std::fill(_m_abScenarioAvailable.begin(), _m_abScenarioAvailable.end(), false);
        _m_numScenariosAvailable = 0;
        _m_choiceList.ResetContent();
        _m_choices.clear();
        _m_addButton.EnableWindow(TRUE);
        _m_editButton.EnableWindow(TRUE);
        _m_removeButton.EnableWindow(TRUE);
    } else
        EnableWindow(TRUE);
}

VA(0x0042fd70, 0x48)
void TScenarioOptionsCrossoverScenarioDlg::hide()
{
    TScenarioStartingOptionsTypeDlg::hide();
    int count = _m_choiceList.GetCount();
    for (int i = 0; i < count; i++) {
        _m_choiceList.SetCurSel(0);
        OnRemoveButton();
    }
}

// No choice: the scenario's first human player's bonus options, without
// bonuses.
VA(0x0042fdc0, 0x1b8)
std::auto_ptr<TScenarioStartingOptions> TScenarioOptionsCrossoverScenarioDlg::getOptions() const
{
    std::auto_ptr<TScenarioStartingOptions> pOptions;
    if (_m_choices.size() > 0)
        pOptions = std::auto_ptr<TScenarioStartingOptions>(new TScenarioOptionsCrossoverScenario(_m_choices));
    else {
        const TCampaignScenarioMap* pMap = _m_campaign.getScenario(_m_scenario).getMap();
        int player;
        for (player = 0; !pMap->m_aPlayer[player].isHumanPlayable(); player++)
            ;
        pOptions = std::auto_ptr<TScenarioStartingOptions>(
            new TScenarioOptionsBonus(player, std::vector<std::auto_ptr<TScenarioStartingBonus> >()));
    }
    if (pOptions.get() == NULL)
        throw TAllocationFailure();
    return pOptions;
}

VA(0x0042ff80, 0x24)
bool TScenarioOptionsCrossoverScenarioDlg::isAvailable() const
{
    return _m_numScenariosAvailable > 0 || _m_choices.size() > 0;
}

VA(0x0042ffb0, 0x169)
void TScenarioOptionsCrossoverScenarioDlg::_insertChoice(int index,
                                                         const TScenarioOptionsCrossoverScenario::TChoice& choice)
{
    const TCampaignScenarioMap* pMap = _m_campaign.getScenario(choice.m_scenario).getMap();
    index = _m_choiceList.InsertString(
        index, TFormattedString("%s - %s\t%s", pMap->m_name.c_str(),
                                _m_campaign.getMapTraits().m_regionTraits[choice.m_scenario].m_name,
                                akPlayerTraits[choice.m_player].m_pName));
    if (index == LB_ERR)
        throw TRuntimeError();
    _m_choices.insert(_m_choices.begin() + index, choice);
    _m_abScenarioAvailable[choice.m_scenario] = false;
    _m_numScenariosAvailable--;
}

VA(0x00430120, 0x79)
void TScenarioOptionsCrossoverScenarioDlg::_removeChoice(int index)
{
    _m_numScenariosAvailable++;
    _m_abScenarioAvailable[_m_choices[index].m_scenario] = true;
    _m_choices.erase(_m_choices.begin() + index);
    _m_choiceList.DeleteString(index);
}

VA(0x004301a0, 0x67)
TPlayerMask TScenarioOptionsCrossoverScenarioDlg::_getPlayers() const
{
    const TCampaignScenarioMap* pMap = _m_campaign.getScenario(_m_scenario).getMap();
    TPlayerMask players;
    for (unsigned int player = 0; player < TCampaignScenarioMap::kNumPlayers; player++) {
        if (pMap->m_aPlayer[player].isHumanPlayable())
            players.set(player);
    }
    return players;
}

VA(0x00430210, 0x55)
void TScenarioOptionsCrossoverScenarioDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_CROSSOVER_SCENARIO_LIST, _m_choiceList);
    DDX_Control(pDX, IDC_ADD_BUTTON, _m_addButton);
    DDX_Control(pDX, IDC_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_DELETE_BUTTON, _m_removeButton);
}

VA(0x00430270, 0xbe)
BOOL TScenarioOptionsCrossoverScenarioDlg::OnInitDialog()
{
    GetDlgItem(IDC_SCENARIO_STATIC)->SetWindowText(SScenarioPropsStartingOptionsPageText::kScenarioColumnStr);
    GetDlgItem(IDC_PLAYER_STATIC)->SetWindowText(SScenarioPropsStartingOptionsPageText::kPlayerStaticStr);
    GetDlgItem(IDC_ADD_BUTTON)->SetWindowText(SScenarioPropsStartingOptionsPageText::kAddButtonStr);
    GetDlgItem(IDC_EDIT_BUTTON)->SetWindowText(SScenarioPropsStartingOptionsPageText::kEditButtonStr);
    GetDlgItem(IDC_DELETE_BUTTON)->SetWindowText(SScenarioPropsStartingOptionsPageText::kRemoveButtonStr);
    CDialog::OnInitDialog();
    int tabStop = 146;
    _m_choiceList.SetTabStops(1, &tabStop);
    return TRUE;
}

VA(0x00430330, 0x133)
void TScenarioOptionsCrossoverScenarioDlg::OnAddButton()
{
    TPlayerMask players = _getPlayers();
    TEditCrossoverScenarioOptionDlg dlg(GetParent(), &_m_campaign, _m_abScenarioAvailable, players);
    if (dlg.DoModal() == IDOK) {
        TScenarioOptionsCrossoverScenario::TChoice choice;
        choice.m_scenario = dlg.getScenario();
        choice.m_player = dlg.getPlayer();
        _insertChoice(-1, choice);
        if (_m_choiceList.GetCount() >= kMaxChoices || _m_numScenariosAvailable == 0)
            _m_addButton.EnableWindow(FALSE);
    }
}

VA(0x00430470, 0x155)
void TScenarioOptionsCrossoverScenarioDlg::OnEditButton()
{
    int index = _m_choiceList.GetCurSel();
    TPlayerMask players = _getPlayers();
    TEditCrossoverScenarioOptionDlg dlg(GetParent(), &_m_campaign, _m_abScenarioAvailable, players,
                                        _m_choices[index]);
    if (dlg.DoModal() == IDOK) {
        TScenarioOptionsCrossoverScenario::TChoice choice;
        choice.m_scenario = dlg.getScenario();
        choice.m_player = dlg.getPlayer();
        if (!(choice == _m_choices[index])) {
            _removeChoice(index);
            _insertChoice(index, choice);
            _m_choiceList.SetCurSel(index);
        }
    }
}

VA(0x004305d0, 0x84)
void TScenarioOptionsCrossoverScenarioDlg::OnRemoveButton()
{
    int index = _m_choiceList.GetCurSel();
    _removeChoice(index);
    _m_addButton.EnableWindow(TRUE);
    int count = _m_choiceList.GetCount();
    if (count > 0) {
        if (index >= count)
            index = count - 1;
        _m_choiceList.SetCurSel(index);
    } else {
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
    }
}

// Not in the message map (the page's enable handler folds into it).
VA(0x00430660, 0x8)
void TScenarioOptionsCrossoverScenarioDlg::OnEnable(BOOL bEnable)
{
    TScenarioStartingOptionsTypeDlg::OnEnable(bEnable);
}

VA(0x00430670, 0x1f)
void TScenarioOptionsCrossoverScenarioDlg::OnSelChangeChoiceList()
{
    _m_editButton.EnableWindow(TRUE);
    _m_removeButton.EnableWindow(TRUE);
}

VA(0x00430690, 0x27)
void TScenarioOptionsCrossoverScenarioDlg::OnDblClkChoiceList()
{
    if (_m_choiceList.GetCurSel() != LB_ERR)
        OnEditButton();
}

VA(0x004306c0, 0x50)
int TScenarioOptionsCrossoverScenarioDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    if (nKey == VK_DELETE && _m_choiceList.GetCurSel() != LB_ERR) {
        OnRemoveButton();
        return -2;
    }
    return TScenarioStartingOptionsTypeDlg::OnVKeyToItem(nKey, pListBox, nIndex);
}

VA(0x00430710, 0x6)
BEGIN_MESSAGE_MAP(TScenarioOptionsStartingHeroDlg, TScenarioStartingOptionsTypeDlg)
    ON_WM_ENABLE()
    ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
    ON_BN_CLICKED(IDC_EDIT_BUTTON, OnEditButton)
    ON_BN_CLICKED(IDC_DELETE_BUTTON, OnRemoveButton)
    ON_LBN_SELCHANGE(IDC_STARTING_HERO_LIST, OnSelChangeChoiceList)
    ON_LBN_DBLCLK(IDC_STARTING_HERO_LIST, OnDblClkChoiceList)
    ON_WM_VKEYTOITEM()
END_MESSAGE_MAP()

VA(0x00430720, 0xd4)
TScenarioOptionsStartingHeroDlg::TScenarioOptionsStartingHeroDlg(const TScenario& scenario)
    : _m_scenario(scenario)
{
}

VA_COMPGEN(0x00430820, 0x1e, SCALAR_DELETING_DTOR, TScenarioOptionsStartingHeroDlg)
VA_COMPGEN(0x00430840, 0x9a, IMPLICIT_DTOR, TScenarioOptionsStartingHeroDlg)

VA(0x004308e0, 0x80)
void TScenarioOptionsStartingHeroDlg::setOptions(const TScenarioOptionsStartingHero& options)
{
    for (unsigned int i = 0; i < options.m_choices.size(); i++)
        _insertChoice(-1, options.m_choices[i]);
    if (_m_choiceList.GetCount() >= kMaxChoices || (_m_heroesAvailable.none() && !_m_bRandomHeroAvailable))
        _m_addButton.EnableWindow(FALSE);
}

VA(0x00430960, 0x66)
void TScenarioOptionsStartingHeroDlg::prepare()
{
    if (_m_scenario.getMap() != NULL) {
        _m_heroesAvailable = _m_scenario.getMap()->m_heroes;
        _m_bRandomHeroAvailable = true;
        _m_addButton.EnableWindow(TRUE);
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
    } else
        EnableWindow(FALSE);
}

VA(0x004309d0, 0x88)
void TScenarioOptionsStartingHeroDlg::clear()
{
    if (_m_scenario.getMap() != NULL) {
        _m_choiceList.ResetContent();
        _m_choices.clear();
        _m_addButton.EnableWindow(TRUE);
        _m_editButton.EnableWindow(TRUE);
        _m_removeButton.EnableWindow(TRUE);
    } else
        EnableWindow(TRUE);
}

VA(0x00430a60, 0x48)
void TScenarioOptionsStartingHeroDlg::hide()
{
    TScenarioStartingOptionsTypeDlg::hide();
    int count = _m_choiceList.GetCount();
    for (int i = 0; i < count; i++) {
        _m_choiceList.SetCurSel(0);
        OnRemoveButton();
    }
}

// No choice: the scenario's first human player's bonus options, without
// bonuses.
VA(0x00430ab0, 0x1ab)
std::auto_ptr<TScenarioStartingOptions> TScenarioOptionsStartingHeroDlg::getOptions() const
{
    std::auto_ptr<TScenarioStartingOptions> pOptions;
    if (_m_choices.size() > 0)
        pOptions = std::auto_ptr<TScenarioStartingOptions>(new TScenarioOptionsStartingHero(_m_choices));
    else {
        const TCampaignScenarioMap* pMap = _m_scenario.getMap();
        int player;
        for (player = 0; !pMap->m_aPlayer[player].isHumanPlayable(); player++)
            ;
        pOptions = std::auto_ptr<TScenarioStartingOptions>(
            new TScenarioOptionsBonus(player, std::vector<std::auto_ptr<TScenarioStartingBonus> >()));
    }
    if (pOptions.get() == NULL)
        throw TAllocationFailure();
    return pOptions;
}

// A hero is left to choose, and a human player can start with one.
VA(0x00430c60, 0x57)
bool TScenarioOptionsStartingHeroDlg::isAvailable() const
{
    if (_m_heroesAvailable.any() || _m_bRandomHeroAvailable) {
        for (unsigned int player = 0; player < TCampaignScenarioMap::kNumPlayers; player++) {
            const TCampaignScenarioMap::TPlayerInfo& info = _m_scenario.getMap()->m_aPlayer[player];
            if (info.isHumanPlayable() && (info.m_bGenerateHeroAtMainTown || info.m_bHasRandomHero))
                return true;
        }
    }
    return false;
}

// Naming the vector insertion iterator leaves this caller's MAX unchanged.
// Preserve the formatted CString then conditional hero-name string cleanup.
VA(0x00430cc0, 0x22b)
void TScenarioOptionsStartingHeroDlg::_insertChoice(int index, const TScenarioOptionsStartingHero::TChoice& choice)
{
    index = _m_choiceList.InsertString(
        index, TFormattedString("%s\t%s",
                                choice.m_hero == TScenarioOptionsStartingHero::kRandomHero
                                    ? kRandomHeroStr
                                    : _m_scenario.getMap()->getHeroName(choice.m_hero).c_str(),
                                akPlayerTraits[choice.m_player].m_pName));
    if (index == LB_ERR)
        throw TRuntimeError();
    _m_choices.insert(_m_choices.begin() + index, choice);
    if (choice.m_hero == TScenarioOptionsStartingHero::kRandomHero)
        _m_bRandomHeroAvailable = false;
    else
        _m_heroesAvailable.reset(choice.m_hero);
}

VA(0x00430ef0, 0xa4)
void TScenarioOptionsStartingHeroDlg::_removeChoice(int index)
{
    const TScenarioOptionsStartingHero::TChoice& choice = _m_choices[index];
    if (choice.m_hero == TScenarioOptionsStartingHero::kRandomHero)
        _m_bRandomHeroAvailable = true;
    else
        _m_heroesAvailable.set(choice.m_hero);
    _m_choices.erase(_m_choices.begin() + index);
    _m_choiceList.DeleteString(index);
}

VA(0x00430fa0, 0x55)
void TScenarioOptionsStartingHeroDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_STARTING_HERO_LIST, _m_choiceList);
    DDX_Control(pDX, IDC_ADD_BUTTON, _m_addButton);
    DDX_Control(pDX, IDC_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_DELETE_BUTTON, _m_removeButton);
}

VA(0x00431000, 0xbe)
BOOL TScenarioOptionsStartingHeroDlg::OnInitDialog()
{
    GetDlgItem(IDC_HERO_COLUMN_STATIC)->SetWindowText(SScenarioPropsStartingOptionsPageText::kHeroColumnStr);
    GetDlgItem(IDC_PLAYER_STATIC)->SetWindowText(SScenarioPropsStartingOptionsPageText::kPlayerStaticStr);
    GetDlgItem(IDC_ADD_BUTTON)->SetWindowText(SScenarioPropsStartingOptionsPageText::kAddButtonStr);
    GetDlgItem(IDC_EDIT_BUTTON)->SetWindowText(SScenarioPropsStartingOptionsPageText::kEditButtonStr);
    GetDlgItem(IDC_DELETE_BUTTON)->SetWindowText(SScenarioPropsStartingOptionsPageText::kRemoveButtonStr);
    CDialog::OnInitDialog();
    int tabStop = 146;
    _m_choiceList.SetTabStops(1, &tabStop);
    return TRUE;
}

VA(0x004310c0, 0x4e)
void TScenarioOptionsStartingHeroDlg::OnEnable(BOOL bEnable)
{
    for (CWnd* pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetWindow(GW_HWNDNEXT))
        pChild->EnableWindow(bEnable);
    CDialog::OnEnable(bEnable);
}

VA(0x00431110, 0x120)
void TScenarioOptionsStartingHeroDlg::OnAddButton()
{
    TEditStartingHeroOptionDlg dlg(GetParent(), &_m_scenario, _m_heroesAvailable, _m_bRandomHeroAvailable);
    if (dlg.DoModal() == IDOK) {
        TScenarioOptionsStartingHero::TChoice choice;
        choice.m_hero = dlg.getHero();
        choice.m_player = dlg.getPlayer();
        _insertChoice(-1, choice);
        if (_m_choiceList.GetCount() >= kMaxChoices || (_m_heroesAvailable.none() && !_m_bRandomHeroAvailable))
            _m_addButton.EnableWindow(FALSE);
    }
}

VA(0x00431230, 0x123)
void TScenarioOptionsStartingHeroDlg::OnEditButton()
{
    int index = _m_choiceList.GetCurSel();
    TEditStartingHeroOptionDlg dlg(GetParent(), &_m_scenario, _m_heroesAvailable, _m_bRandomHeroAvailable,
                                   _m_choices[index]);
    if (dlg.DoModal() == IDOK) {
        TScenarioOptionsStartingHero::TChoice choice;
        choice.m_hero = dlg.getHero();
        choice.m_player = dlg.getPlayer();
        if (!(choice == _m_choices[index])) {
            _removeChoice(index);
            _insertChoice(index, choice);
            _m_choiceList.SetCurSel(index);
        }
    }
}

VA(0x00431360, 0x84)
void TScenarioOptionsStartingHeroDlg::OnRemoveButton()
{
    int index = _m_choiceList.GetCurSel();
    _removeChoice(index);
    _m_addButton.EnableWindow(TRUE);
    int count = _m_choiceList.GetCount();
    if (count > 0) {
        if (index >= count)
            index = count - 1;
        _m_choiceList.SetCurSel(index);
    } else {
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
    }
}

void TScenarioOptionsStartingHeroDlg::OnSelChangeChoiceList()
{
    _m_editButton.EnableWindow(TRUE);
    _m_removeButton.EnableWindow(TRUE);
}

VA(0x004313f0, 0x27)
void TScenarioOptionsStartingHeroDlg::OnDblClkChoiceList()
{
    if (_m_choiceList.GetCurSel() != LB_ERR)
        OnEditButton();
}

VA(0x00431420, 0x50)
int TScenarioOptionsStartingHeroDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    if (nKey == VK_DELETE && _m_choiceList.GetCurSel() != LB_ERR) {
        OnRemoveButton();
        return -2;
    }
    return TScenarioStartingOptionsTypeDlg::OnVKeyToItem(nKey, pListBox, nIndex);
}

VA(0x00431470, 0x98)
TScenarioPropsStartingOptionsPage::_TTypeDlgs::_TTypeDlgs(const TCampaign& campaign, int scenario,
                                                          int campaignVersion)
    : m_bonusDlg(campaign.getScenario(scenario), campaignVersion),
      m_crossoverScenarioDlg(campaign, scenario),
      m_startingHeroDlg(campaign.getScenario(scenario))
{
    m_apDlg[0] = &m_bonusDlg;
    m_apDlg[1] = &m_crossoverScenarioDlg;
    m_apDlg[2] = &m_startingHeroDlg;
}

VA(0x00431510, 0x20a)
TScenarioPropsStartingOptionsPage::TScenarioPropsStartingOptionsPage(const TCampaign& oldCampaign,
                                                                     TCampaign& newCampaign, int scenario,
                                                                     int campaignVersion)
    : CPropertyPage(TScenarioPropsStartingOptionsPage::IDD),
      _m_oldCampaign(oldCampaign),
      _m_newCampaign(newCampaign),
      _m_scenario(scenario),
      _m_bModified(false)
{
    _m_type = -1;
    m_psp.pszTitle = m_strCaption = SScenarioPropsStartingOptionsPageText::kCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
    _m_pTypeDlgs = std::auto_ptr<_TTypeDlgs>(new _TTypeDlgs(_m_newCampaign, _m_scenario, campaignVersion));
    if (_m_pTypeDlgs.get() == NULL)
        throw TAllocationFailure();
}

VA_COMPGEN(0x00431720, 0x1e, SCALAR_DELETING_DTOR, TScenarioPropsStartingOptionsPage)

VA(0x00431740, 0x244)
TScenarioPropsStartingOptionsPage::~TScenarioPropsStartingOptionsPage()
{
}

VA(0x00431990, 0x1)
void TScenarioPropsStartingOptionsPage::update()
{
}

VA(0x004319a0, 0x72)
void TScenarioPropsStartingOptionsPage::_enableControls(bool bEnable)
{
    for (CWnd* pChild = GetWindow(GW_CHILD); pChild != _m_pTypeDlgs->m_apDlg[0];
         pChild = pChild->GetWindow(GW_HWNDNEXT))
        pChild->EnableWindow(bEnable);
    EnableWindow(bEnable);
}

VA(0x00431a20, 0x56)
void TScenarioPropsStartingOptionsPage::_setType(int type)
{
    if (type != _m_type) {
        _m_pTypeDlgs->m_apDlg[_m_type]->hide();
        _m_type = type;
        UpdateData(FALSE);
        _m_pTypeDlgs->m_apDlg[_m_type]->show();
    }
}

VA(0x00431a80, 0x21)
void TScenarioPropsStartingOptionsPage::visit(const TScenarioOptionsBonus& options)
{
    _m_pTypeDlgs->m_bonusDlg.setOptions(options);
    _setType(0);
}

VA(0x00431ab0, 0x27)
void TScenarioPropsStartingOptionsPage::visit(const TScenarioOptionsCrossoverScenario& options)
{
    _m_pTypeDlgs->m_crossoverScenarioDlg.setOptions(options);
    _setType(1);
}

VA(0x00431ae0, 0x27)
void TScenarioPropsStartingOptionsPage::visit(const TScenarioOptionsStartingHero& options)
{
    _m_pTypeDlgs->m_startingHeroDlg.setOptions(options);
    _setType(2);
}

VA(0x00431b10, 0x63)
void TScenarioPropsStartingOptionsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Radio(pDX, IDC_BONUS_OPTIONS_RADIO, _m_type);
    const int akTypeRadioIDs[kNumTypes] = {
        IDC_BONUS_OPTIONS_RADIO, IDC_CROSSOVER_OPTIONS_RADIO, IDC_STARTING_HERO_OPTIONS_RADIO
    };
    for (unsigned int type = 0; type < kNumTypes; type++)
        DDX_Control(pDX, akTypeRadioIDs[type], _m_aTypeRadios[type]);
}

VA(0x00431b80, 0x6)
BEGIN_MESSAGE_MAP(TScenarioPropsStartingOptionsPage, CPropertyPage)
    ON_WM_DESTROY()
    ON_WM_CREATE()
    ON_BN_CLICKED(IDC_BONUS_OPTIONS_RADIO, OnTypeRadio)
    ON_BN_CLICKED(IDC_CROSSOVER_OPTIONS_RADIO, OnTypeRadio)
    ON_BN_CLICKED(IDC_STARTING_HERO_OPTIONS_RADIO, OnTypeRadio)
    ON_WM_ENABLE()
END_MESSAGE_MAP()

VA(0x00431b90, 0x43)
int TScenarioPropsStartingOptionsPage::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
        return -1;
    for (unsigned int type = 0; type < kNumTypes; type++) {
        if (!_m_pTypeDlgs->m_apDlg[type]->create(this))
            return -1;
    }
    return 0;
}

VA(0x00431be0, 0x2c)
void TScenarioPropsStartingOptionsPage::OnDestroy()
{
    for (unsigned int type = 0; type < kNumTypes; type++)
        _m_pTypeDlgs->m_apDlg[type]->DestroyWindow();
    CPropertyPage::OnDestroy();
}

// The type dialogs take the frame's place, after it in the z-order.
VA(0x00431c10, 0xf8)
BOOL TScenarioPropsStartingOptionsPage::OnInitDialog()
{
    GetDlgItem(IDC_OPTIONS_TYPE_STATIC)->SetWindowText(SScenarioPropsStartingOptionsPageText::kTypeStaticStr);
    GetDlgItem(IDC_BONUS_OPTIONS_RADIO)->SetWindowText(SScenarioPropsStartingOptionsPageText::kBonusRadioStr);
    GetDlgItem(IDC_CROSSOVER_OPTIONS_RADIO)->SetWindowText(SScenarioPropsStartingOptionsPageText::kCrossoverRadioStr);
    GetDlgItem(IDC_STARTING_HERO_OPTIONS_RADIO)
        ->SetWindowText(SScenarioPropsStartingOptionsPageText::kStartingHeroRadioStr);
    _m_bModified = false;
    CPropertyPage::OnInitDialog();
    CWnd* pFrame = GetDlgItem(IDC_BONUS_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    const CWnd* pInsertAfter = pFrame;
    for (unsigned int type = 0; type < kNumTypes; type++) {
        TScenarioStartingOptionsTypeDlg* pDlg = _m_pTypeDlgs->m_apDlg[type];
        pDlg->SetWindowPos(pInsertAfter, rect.left, rect.top, 0, 0, SWP_NOSIZE);
        pInsertAfter = pDlg;
    }
    pFrame->DestroyWindow();
    return TRUE;
}

VA(0x00431d10, 0xa0)
void TScenarioPropsStartingOptionsPage::OnOK()
{
    CPropertyPage::OnOK();
    const TScenario& oldScenario = _m_oldCampaign.getScenario(_m_scenario);
    const TScenario& newScenario = _m_newCampaign.getScenario(_m_scenario);
    _m_bModified = _m_bModified || (newScenario.getMap() != NULL) != (oldScenario.getMap() != NULL)
                   || (newScenario.getMap() != NULL
                       && !(*newScenario.getStartingOptions() == *oldScenario.getStartingOptions()));
}

VA(0x00431db0, 0xea)
BOOL TScenarioPropsStartingOptionsPage::OnSetActive()
{
    BOOL bResult = CPropertyPage::OnSetActive();
    if (bResult) {
        unsigned int type;
        for (type = 0; type < kNumTypes; type++)
            _m_pTypeDlgs->m_apDlg[type]->prepare();
        _m_type = 0;
        UpdateData(FALSE);
        _m_pTypeDlgs->m_bonusDlg.show();
        const TScenario& scenario = _m_newCampaign.getScenario(_m_scenario);
        if (scenario.getMap() != NULL) {
            for (type = 0; type < kNumTypes; type++)
                _m_aTypeRadios[type].EnableWindow(_m_pTypeDlgs->m_apDlg[type]->isAvailable());
            scenario.getStartingOptions()->accept(*this);
        } else
            _enableControls(false);
    }
    return bResult;
}

VA(0x00431ea0, 0x11c)
BOOL TScenarioPropsStartingOptionsPage::OnKillActive()
{
    BOOL bResult = CPropertyPage::OnKillActive();
    if (bResult) {
        if (_m_newCampaign.getScenario(_m_scenario).getMap() != NULL) {
            std::auto_ptr<TScenarioStartingOptions> pOptions = _m_pTypeDlgs->m_apDlg[_m_type]->getOptions();
            _m_newCampaign.setScenarioStartingOptions(_m_scenario, pOptions);
        } else
            _enableControls(true);
        _m_pTypeDlgs->m_apDlg[_m_type]->hide();
        _m_type = -1;
        for (unsigned int type = 0; type < kNumTypes; type++)
            _m_pTypeDlgs->m_apDlg[type]->clear();
    }
    return bResult;
}

VA(0x00431fc0, 0x44)
void TScenarioPropsStartingOptionsPage::OnTypeRadio()
{
    _m_pTypeDlgs->m_apDlg[_m_type]->hide();
    UpdateData(TRUE);
    _m_pTypeDlgs->m_apDlg[_m_type]->show();
}

void TScenarioPropsStartingOptionsPage::OnEnable(BOOL bEnable)
{
    CPropertyPage::OnEnable(bEnable);
}

VA_COMPGEN(0x00432030, 0x1dc, IMPLICIT_DTOR, _TTypeDlgs)
