// EditStartingHeroOptionDlg.cpp - one starting hero choice of a scenario's
// starting options (h3ccmped 0x421bc0..0x4224f0).
#include "campaign_editor/stdafx.h"

#include "va.h"
#include "editor/Player.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/EditStartingHeroOptionDlg.h"

VA(0x00421c10, 0x1e0)
TEditStartingHeroOptionDlg::TEditStartingHeroOptionDlg(CWnd* pParent, const TScenario* pScenario,
                                                       const THeroMask& heroes, bool bRandomHeroAllowed)
    : CDialog(TEditStartingHeroOptionDlg::IDD, pParent), _m_pScenario(pScenario), _m_heroes(heroes),
      _m_bRandomHeroAllowed(bRandomHeroAllowed), _m_hero(TScenarioOptionsStartingHero::kRandomHero), _m_player(0)
{
    _initPlayers();
    int player = 0;
    while (!_m_players[player])
        player++;
    int hero;
    if (_m_bRandomHeroAllowed) {
        hero = TScenarioOptionsStartingHero::kRandomHero;
    } else {
        hero = 0;
        while (!_m_heroes[hero])
            hero++;
    }
    _m_hero = hero;
    _m_player = player;
}

VA_COMPGEN(0x00421df0, 0x1e, SCALAR_DELETING_DTOR, TEditStartingHeroOptionDlg)
VA_COMPGEN(0x00421e10, 0x5c, IMPLICIT_DTOR, TEditStartingHeroOptionDlg)

VA(0x00421e70, 0x17e)
TEditStartingHeroOptionDlg::TEditStartingHeroOptionDlg(CWnd* pParent, const TScenario* pScenario,
                                                       const THeroMask& heroes, bool bRandomHeroAllowed,
                                                       const TScenarioOptionsStartingHero::TChoice& choice)
    : CDialog(TEditStartingHeroOptionDlg::IDD, pParent), _m_pScenario(pScenario), _m_heroes(heroes),
      _m_bRandomHeroAllowed(bRandomHeroAllowed), _m_hero(choice.m_hero), _m_player(choice.m_player)
{
    _initPlayers();
    if (_m_hero == TScenarioOptionsStartingHero::kRandomHero)
        _m_bRandomHeroAllowed = true;
    else
        _m_heroes[_m_hero] = true;
}

// The players whose part of the map is human-playable and gets a hero: one
// generated at the main town or a random one.
VA(0x00421ff0, 0x99)
void TEditStartingHeroOptionDlg::_initPlayers()
{
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        const TCampaignScenarioMap::TPlayerInfo& info = _m_pScenario->getMap()->m_aPlayer[player];
        _m_players[player] = info.isHumanPlayable() && (info.m_bGenerateHeroAtMainTown || info.m_bHasRandomHero);
    }
}

VA(0x00422090, 0x2e)
void TEditStartingHeroOptionDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_PLAYER_COMBO, _m_playerCombo);
    DDX_Control(pDX, IDC_HERO_COMBO, _m_heroCombo);
}

VA(0x004220c0, 0x6)
BEGIN_MESSAGE_MAP(TEditStartingHeroOptionDlg, CDialog)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x004220d0, 0x28)
int TEditStartingHeroOptionDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(SEditStartingHeroOptionDlgText::kCaptionStr);
    return 0;
}

VA(0x00422100, 0x2a0)
BOOL TEditStartingHeroOptionDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_HERO_STATIC)->SetWindowText(SEditStartingHeroOptionDlgText::kHeroStaticStr);
    GetDlgItem(IDC_PLAYER_STATIC)->SetWindowText(SEditStartingHeroOptionDlgText::kPlayerStaticStr);
    CDialog::OnInitDialog();
    if (_m_bRandomHeroAllowed)
        _m_heroCombo.SetItemData(_m_heroCombo.AddString(kRandomHeroStr), TScenarioOptionsStartingHero::kRandomHero);
    for (int hero = 0; hero < TCampaignScenarioMap::kNumHeroes; hero++) {
        if (_m_heroes[hero]) {
            int item = _m_heroCombo.AddString(_m_pScenario->getMap()->getHeroName(hero).c_str());
            _m_heroCombo.SetItemData(item, hero);
        }
    }
    int item;
    for (item = 0; _m_heroCombo.GetItemData(item) != _m_hero; item++)
        ;
    _m_heroCombo.SetCurSel(item);
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        if (_m_players[player])
            _m_playerCombo.SetItemData(_m_playerCombo.AddString(akPlayerTraits[player].m_pName), player);
    }
    for (item = 0; _m_playerCombo.GetItemData(item) != _m_player; item++)
        ;
    _m_playerCombo.SetCurSel(item);
    return TRUE;
}

VA(0x004223a0, 0x62)
void TEditStartingHeroOptionDlg::OnOK()
{
    CDialog::OnOK();
    int hero = _m_heroCombo.GetItemData(_m_heroCombo.GetCurSel());
    int player = _m_playerCombo.GetItemData(_m_playerCombo.GetCurSel());
    _m_hero = hero;
    _m_player = player;
}
