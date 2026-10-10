// EditCrossoverScenarioOptionDlg.cpp - one crossover choice of a scenario's
// starting options (h3ccmped 0x41d640..0x41e210).
#include "campaign_editor/stdafx.h"

#include "va.h"
#include "campaignmap.h"
#include "editor/FormattedString.h"
#include "editor/Player.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/EditCrossoverScenarioOptionDlg.h"

VA(0x0041d680, 0x1a5)
TEditCrossoverScenarioOptionDlg::TEditCrossoverScenarioOptionDlg(CWnd* pParent, const TCampaign* pCampaign,
                                                                 const vector<bool>& abScenarioAllowed,
                                                                 const TPlayerMask& players)
    : CDialog(TEditCrossoverScenarioOptionDlg::IDD, pParent), _m_pCampaign(pCampaign),
      _m_abScenarioAllowed(abScenarioAllowed), _m_players(players), _m_player(0)
{
    int scenario = 0;
    while (!_m_abScenarioAllowed[scenario])
        scenario++;
    int player = 0;
    while (!_m_players[player])
        player++;
    _m_scenario = scenario;
    _m_player = player;
}

VA_COMPGEN(0x0041d9a0, 0x1e, SCALAR_DELETING_DTOR, TEditCrossoverScenarioOptionDlg)
VA_COMPGEN(0x0041d9c0, 0x86, IMPLICIT_DTOR, TEditCrossoverScenarioOptionDlg)

VA(0x0041da50, 0x1e6)
TEditCrossoverScenarioOptionDlg::TEditCrossoverScenarioOptionDlg(
    CWnd* pParent, const TCampaign* pCampaign, const vector<bool>& abScenarioAllowed, const TPlayerMask& players,
    const TScenarioOptionsCrossoverScenario::TChoice& choice)
    : CDialog(TEditCrossoverScenarioOptionDlg::IDD, pParent), _m_pCampaign(pCampaign),
      _m_abScenarioAllowed(abScenarioAllowed), _m_players(players), _m_scenario(choice.m_scenario),
      _m_player(choice.m_player)
{
    _m_abScenarioAllowed[_m_scenario] = true;
    _m_players[_m_player] = true;
}

VA(0x0041dc40, 0x2e)
void TEditCrossoverScenarioOptionDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_PLAYER_COMBO, _m_playerCombo);
    DDX_Control(pDX, IDC_SCENARIO_COMBO, _m_scenarioCombo);
}

VA(0x0041dc70, 0x6)
BEGIN_MESSAGE_MAP(TEditCrossoverScenarioOptionDlg, CDialog)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x0041dc80, 0x28)
int TEditCrossoverScenarioOptionDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(SEditCrossoverScenarioOptionDlgText::kCaptionStr);
    return 0;
}

VA(0x0041dcb0, 0x2c0)
BOOL TEditCrossoverScenarioOptionDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_SCENARIO_STATIC)->SetWindowText(SEditCrossoverScenarioOptionDlgText::kScenarioStaticStr);
    GetDlgItem(IDC_PLAYER_STATIC)->SetWindowText(SEditCrossoverScenarioOptionDlgText::kPlayerStaticStr);
    CDialog::OnInitDialog();
    for (unsigned int scenario = 0; scenario < _m_pCampaign->getMapTraits().m_numRegions; scenario++) {
        if (_m_abScenarioAllowed[scenario]) {
            const TCampaignScenarioMap* pMap = _m_pCampaign->getScenario(scenario).getMap();
            int item = _m_scenarioCombo.AddString(
                TFormattedString("%s - %s", pMap->m_name.c_str(),
                                 _m_pCampaign->getMapTraits().m_regionTraits[scenario].m_name));
            _m_scenarioCombo.SetItemData(item, scenario);
        }
    }
    int item;
    for (item = 0; _m_scenarioCombo.GetItemData(item) != _m_scenario; item++)
        ;
    _m_scenarioCombo.SetCurSel(item);
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        if (_m_players[player])
            _m_playerCombo.SetItemData(_m_playerCombo.AddString(akPlayerTraits[player].m_pName), player);
    }
    for (item = 0; _m_playerCombo.GetItemData(item) != _m_player; item++)
        ;
    _m_playerCombo.SetCurSel(item);
    return TRUE;
}

VA(0x0041e0e0, 0x62)
void TEditCrossoverScenarioOptionDlg::OnOK()
{
    CDialog::OnOK();
    int scenario = _m_scenarioCombo.GetItemData(_m_scenarioCombo.GetCurSel());
    int player = _m_playerCombo.GetItemData(_m_playerCombo.GetCurSel());
    _m_scenario = scenario;
    _m_player = player;
}
