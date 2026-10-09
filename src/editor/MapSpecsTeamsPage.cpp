// MapSpecsTeamsPage.cpp - the teams page of the map specifications sheet
// (h3maped 0x476c1b..0x477e53; Loki h3maped object 74). A team that loses
// its last player takes one from the largest team; enabling teams or
// changing their number deals the present players out evenly in order.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsTeamsPage.h"

DATA(0x0053b450)
int TMapSpecsTeamsPage::* const TMapSpecsTeamsPage::_s_apPlayerTeam[kNumPlayers] = {
    &TMapSpecsTeamsPage::_m_player1Team,
    &TMapSpecsTeamsPage::_m_player2Team,
    &TMapSpecsTeamsPage::_m_player3Team,
    &TMapSpecsTeamsPage::_m_player4Team,
    &TMapSpecsTeamsPage::_m_player5Team,
    &TMapSpecsTeamsPage::_m_player6Team,
    &TMapSpecsTeamsPage::_m_player7Team,
    &TMapSpecsTeamsPage::_m_player8Team
};

DATA(0x0053b470)
static const int g_akPlayerStaticIDs[kNumPlayers] = { IDC_PLAYER1_STATIC, IDC_PLAYER2_STATIC, IDC_PLAYER3_STATIC, IDC_PLAYER4_STATIC, IDC_PLAYER5_STATIC, IDC_PLAYER6_STATIC, IDC_PLAYER7_STATIC, IDC_PLAYER8_STATIC };

DATA(0x0053b490)
static const int g_akTeamRadioIDs[kNumPlayers][TTeamInfo::s_kMaxTeams] = {
    { IDC_PLAYER1_TEAM1_RADIO, IDC_PLAYER1_TEAM2_RADIO, IDC_PLAYER1_TEAM3_RADIO, IDC_PLAYER1_TEAM4_RADIO, IDC_PLAYER1_TEAM5_RADIO, IDC_PLAYER1_TEAM6_RADIO, IDC_PLAYER1_TEAM7_RADIO },
    { IDC_PLAYER2_TEAM1_RADIO, IDC_PLAYER2_TEAM2_RADIO, IDC_PLAYER2_TEAM3_RADIO, IDC_PLAYER2_TEAM4_RADIO, IDC_PLAYER2_TEAM5_RADIO, IDC_PLAYER2_TEAM6_RADIO, IDC_PLAYER2_TEAM7_RADIO },
    { IDC_PLAYER3_TEAM1_RADIO, IDC_PLAYER3_TEAM2_RADIO, IDC_PLAYER3_TEAM3_RADIO, IDC_PLAYER3_TEAM4_RADIO, IDC_PLAYER3_TEAM5_RADIO, IDC_PLAYER3_TEAM6_RADIO, IDC_PLAYER3_TEAM7_RADIO },
    { IDC_PLAYER4_TEAM1_RADIO, IDC_PLAYER4_TEAM2_RADIO, IDC_PLAYER4_TEAM3_RADIO, IDC_PLAYER4_TEAM4_RADIO, IDC_PLAYER4_TEAM5_RADIO, IDC_PLAYER4_TEAM6_RADIO, IDC_PLAYER4_TEAM7_RADIO },
    { IDC_PLAYER5_TEAM1_RADIO, IDC_PLAYER5_TEAM2_RADIO, IDC_PLAYER5_TEAM3_RADIO, IDC_PLAYER5_TEAM4_RADIO, IDC_PLAYER5_TEAM5_RADIO, IDC_PLAYER5_TEAM6_RADIO, IDC_PLAYER5_TEAM7_RADIO },
    { IDC_PLAYER6_TEAM1_RADIO, IDC_PLAYER6_TEAM2_RADIO, IDC_PLAYER6_TEAM3_RADIO, IDC_PLAYER6_TEAM4_RADIO, IDC_PLAYER6_TEAM5_RADIO, IDC_PLAYER6_TEAM6_RADIO, IDC_PLAYER6_TEAM7_RADIO },
    { IDC_PLAYER7_TEAM1_RADIO, IDC_PLAYER7_TEAM2_RADIO, IDC_PLAYER7_TEAM3_RADIO, IDC_PLAYER7_TEAM4_RADIO, IDC_PLAYER7_TEAM5_RADIO, IDC_PLAYER7_TEAM6_RADIO, IDC_PLAYER7_TEAM7_RADIO },
    { IDC_PLAYER8_TEAM1_RADIO, IDC_PLAYER8_TEAM2_RADIO, IDC_PLAYER8_TEAM3_RADIO, IDC_PLAYER8_TEAM4_RADIO, IDC_PLAYER8_TEAM5_RADIO, IDC_PLAYER8_TEAM6_RADIO, IDC_PLAYER8_TEAM7_RADIO }
};

DATA(0x0053b570)
static const int g_akNumTeamsRadioIDs[TMapSpecsTeamsPage::s_kNumNumTeamsRadios] = { IDC_NUM_TEAMS2_RADIO, IDC_NUM_TEAMS3_RADIO, IDC_NUM_TEAMS4_RADIO, IDC_NUM_TEAMS5_RADIO, IDC_NUM_TEAMS6_RADIO, IDC_NUM_TEAMS7_RADIO };

VA(0x00476def, 0x6)
BEGIN_MESSAGE_MAP(TFocusRectStatic, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()

VA(0x00476df5, 0x96)
void TFocusRectStatic::OnPaint()
{
    if (_m_bShowFocusRect) {
        CRect updateRect;
        bool bUpdate = GetUpdateRect(&updateRect) != FALSE;
        Default();
        if (bUpdate) {
            CClientDC dc(this);
            dc.SelectClipRgn(NULL);
            dc.IntersectClipRect(&updateRect);
            CRect rect;
            GetClientRect(&rect);
            dc.DrawFocusRect(&rect);
        }
    } else {
        Default();
    }
}

VA(0x00476e8b, 0x13d)
TMapSpecsTeamsPage::TMapSpecsTeamsPage(const TGameMap& oldMap, TGameMap& newMap)
    : CPropertyPage(TMapSpecsTeamsPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_bModified(false)
{
    _m_player1Team = -1;
    _m_player2Team = -1;
    _m_player3Team = -1;
    _m_player4Team = -1;
    _m_player5Team = -1;
    _m_player6Team = -1;
    _m_player7Team = -1;
    _m_player8Team = -1;
    _m_numTeams = -1;
    m_psp.pszTitle = m_strCaption = kTeamsPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00476fc8, 0x1c, SCALAR_DELETING_DTOR, TMapSpecsTeamsPage)
VA_COMPGEN(0x00476fe4, 0x4a, CLASS_CTOR, _TPlayerControls)
VA_COMPGEN(0x0047702e, 0x1c, SCALAR_DELETING_DTOR, TFocusRectStatic)
VA_COMPGEN(0x0047704a, 0x5, IMPLICIT_DTOR, TFocusRectStatic)
VA_COMPGEN(0x0047704f, 0x3f, IMPLICIT_DTOR, _TPlayerControls)

VA(0x0047708e, 0xa3)
TMapSpecsTeamsPage::~TMapSpecsTeamsPage()
{
}

VA(0x00477131, 0x10d)
void TMapSpecsTeamsPage::_onPlayerTeamRadio(unsigned int player)
{
    int oldTeam = this->*_s_apPlayerTeam[player];
    UpdateData(TRUE);
    if (this->*_s_apPlayerTeam[player] == oldTeam)
        return;
    const TArray<TPlayerInfo, kNumPlayers>& players = _m_newMap.getPlayers();
    unsigned int other;
    for (other = 0; other < kNumPlayers; other++) {
        if (players[other].getBPresent() && this->*_s_apPlayerTeam[other] == oldTeam)
            return;
    }
    int numTeams = _m_numTeams + TTeamInfo::s_kMinTeams;
    unsigned int aTeamSize[TTeamInfo::s_kMaxTeams];
    int team;
    for (team = 0; team < numTeams; team++)
        aTeamSize[team] = 0;
    for (other = 0; other < kNumPlayers; other++) {
        if (players[other].getBPresent())
            ++aTeamSize[this->*_s_apPlayerTeam[other]];
    }
    unsigned int maxTeamSize = aTeamSize[0];
    for (unsigned int i = 1; i < unsigned(numTeams); i++) {
        if (aTeamSize[i] > maxTeamSize)
            maxTeamSize = aTeamSize[i];
    }
    other = player;
    do {
        if (++other == kNumPlayers)
            other = 0;
    } while (!players[other].getBPresent() || aTeamSize[this->*_s_apPlayerTeam[other]] != maxTeamSize);
    this->*_s_apPlayerTeam[other] = oldTeam;
    UpdateData(FALSE);
}

VA(0x0047723e, 0x2f)
void TMapSpecsTeamsPage::_onSetFocusPlayerTeamRadio(unsigned int player)
{
    _m_aPlayerControls[player].m_playerStatic.showFocusRect(true);
}

VA(0x0047726d, 0x2a)
void TMapSpecsTeamsPage::_onKillFocusPlayerTeamRadio(unsigned int player)
{
    _m_aPlayerControls[player].m_playerStatic.showFocusRect(false);
}

VA(0x00477297, 0x34)
void TMapSpecsTeamsPage::_onPlayerStatic(unsigned int player)
{
    GotoDlgCtrl(&_m_aPlayerControls[player].m_playerStatic);
    NextDlgCtrl();
}

VA(0x004772cb, 0x188)
void TMapSpecsTeamsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TEAM_NUMBER_STATIC, _m_teamNumberStatic);
    DDX_Control(pDX, IDC_TEAM_ASSIGNMENTS_STATIC, _m_teamAssignmentsGroup);
    DDX_Control(pDX, IDC_NUM_TEAMS_STATIC, _m_numTeamsGroup);
    DDX_Control(pDX, IDC_ENABLE_TEAMS_CHECK, _m_enableTeamsCheck);
    DDX_Radio(pDX, IDC_PLAYER1_TEAM1_RADIO, _m_player1Team);
    DDX_Radio(pDX, IDC_PLAYER2_TEAM1_RADIO, _m_player2Team);
    DDX_Radio(pDX, IDC_PLAYER3_TEAM1_RADIO, _m_player3Team);
    DDX_Radio(pDX, IDC_PLAYER4_TEAM1_RADIO, _m_player4Team);
    DDX_Radio(pDX, IDC_PLAYER5_TEAM1_RADIO, _m_player5Team);
    DDX_Radio(pDX, IDC_PLAYER6_TEAM1_RADIO, _m_player6Team);
    DDX_Radio(pDX, IDC_PLAYER7_TEAM1_RADIO, _m_player7Team);
    DDX_Radio(pDX, IDC_PLAYER8_TEAM1_RADIO, _m_player8Team);
    DDX_Radio(pDX, IDC_NUM_TEAMS2_RADIO, _m_numTeams);
    unsigned int i;
    for (i = 0; i < kNumPlayers; i++) {
        DDX_Control(pDX, g_akPlayerStaticIDs[i], _m_aPlayerControls[i].m_playerStatic);
        for (unsigned int team = 0; team < TTeamInfo::s_kMaxTeams; team++)
            DDX_Control(pDX, g_akTeamRadioIDs[i][team], _m_aPlayerControls[i].m_aTeamRadios[team]);
    }
    for (i = 0; i < s_kNumNumTeamsRadios; i++)
        DDX_Control(pDX, g_akNumTeamsRadioIDs[i], _m_aNumTeamsRadios[i]);
}

VA(0x00477453, 0x6)
BEGIN_MESSAGE_MAP(TMapSpecsTeamsPage, CPropertyPage)
    ON_BN_CLICKED(IDC_ENABLE_TEAMS_CHECK, OnEnableTeamsCheck)
    ON_BN_CLICKED(IDC_PLAYER1_TEAM1_RADIO, OnPlayer1TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER2_TEAM1_RADIO, OnPlayer2TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER3_TEAM1_RADIO, OnPlayer3TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER4_TEAM1_RADIO, OnPlayer4TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER5_TEAM1_RADIO, OnPlayer5TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER6_TEAM1_RADIO, OnPlayer6TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER7_TEAM1_RADIO, OnPlayer7TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER8_TEAM1_RADIO, OnPlayer8TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER1_TEAM2_RADIO, OnPlayer1TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER1_TEAM3_RADIO, OnPlayer1TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER1_TEAM4_RADIO, OnPlayer1TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER1_TEAM5_RADIO, OnPlayer1TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER1_TEAM6_RADIO, OnPlayer1TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER1_TEAM7_RADIO, OnPlayer1TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER2_TEAM2_RADIO, OnPlayer2TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER2_TEAM3_RADIO, OnPlayer2TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER2_TEAM4_RADIO, OnPlayer2TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER2_TEAM5_RADIO, OnPlayer2TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER2_TEAM6_RADIO, OnPlayer2TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER2_TEAM7_RADIO, OnPlayer2TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER3_TEAM2_RADIO, OnPlayer3TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER3_TEAM3_RADIO, OnPlayer3TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER3_TEAM4_RADIO, OnPlayer3TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER3_TEAM5_RADIO, OnPlayer3TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER3_TEAM6_RADIO, OnPlayer3TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER3_TEAM7_RADIO, OnPlayer3TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER4_TEAM2_RADIO, OnPlayer4TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER4_TEAM3_RADIO, OnPlayer4TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER4_TEAM4_RADIO, OnPlayer4TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER4_TEAM5_RADIO, OnPlayer4TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER4_TEAM6_RADIO, OnPlayer4TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER4_TEAM7_RADIO, OnPlayer4TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER5_TEAM2_RADIO, OnPlayer5TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER5_TEAM3_RADIO, OnPlayer5TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER5_TEAM4_RADIO, OnPlayer5TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER5_TEAM5_RADIO, OnPlayer5TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER5_TEAM6_RADIO, OnPlayer5TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER5_TEAM7_RADIO, OnPlayer5TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER6_TEAM2_RADIO, OnPlayer6TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER6_TEAM3_RADIO, OnPlayer6TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER6_TEAM4_RADIO, OnPlayer6TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER6_TEAM5_RADIO, OnPlayer6TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER6_TEAM6_RADIO, OnPlayer6TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER6_TEAM7_RADIO, OnPlayer6TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER7_TEAM2_RADIO, OnPlayer7TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER7_TEAM3_RADIO, OnPlayer7TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER7_TEAM4_RADIO, OnPlayer7TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER7_TEAM5_RADIO, OnPlayer7TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER7_TEAM6_RADIO, OnPlayer7TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER7_TEAM7_RADIO, OnPlayer7TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER8_TEAM2_RADIO, OnPlayer8TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER8_TEAM3_RADIO, OnPlayer8TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER8_TEAM4_RADIO, OnPlayer8TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER8_TEAM5_RADIO, OnPlayer8TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER8_TEAM6_RADIO, OnPlayer8TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER8_TEAM7_RADIO, OnPlayer8TeamRadio)
    ON_BN_CLICKED(IDC_NUM_TEAMS2_RADIO, OnNumTeamsRadio)
    ON_BN_CLICKED(IDC_NUM_TEAMS3_RADIO, OnNumTeamsRadio)
    ON_BN_CLICKED(IDC_NUM_TEAMS4_RADIO, OnNumTeamsRadio)
    ON_BN_CLICKED(IDC_NUM_TEAMS5_RADIO, OnNumTeamsRadio)
    ON_BN_CLICKED(IDC_NUM_TEAMS6_RADIO, OnNumTeamsRadio)
    ON_BN_CLICKED(IDC_NUM_TEAMS7_RADIO, OnNumTeamsRadio)
    ON_BN_SETFOCUS(IDC_PLAYER1_TEAM1_RADIO, OnSetFocusPlayer1TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER1_TEAM2_RADIO, OnSetFocusPlayer1TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER1_TEAM3_RADIO, OnSetFocusPlayer1TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER1_TEAM4_RADIO, OnSetFocusPlayer1TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER1_TEAM5_RADIO, OnSetFocusPlayer1TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER1_TEAM6_RADIO, OnSetFocusPlayer1TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER1_TEAM7_RADIO, OnSetFocusPlayer1TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER2_TEAM1_RADIO, OnSetFocusPlayer2TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER2_TEAM2_RADIO, OnSetFocusPlayer2TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER2_TEAM3_RADIO, OnSetFocusPlayer2TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER2_TEAM4_RADIO, OnSetFocusPlayer2TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER2_TEAM5_RADIO, OnSetFocusPlayer2TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER2_TEAM6_RADIO, OnSetFocusPlayer2TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER2_TEAM7_RADIO, OnSetFocusPlayer2TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER3_TEAM1_RADIO, OnSetFocusPlayer3TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER3_TEAM2_RADIO, OnSetFocusPlayer3TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER3_TEAM3_RADIO, OnSetFocusPlayer3TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER3_TEAM4_RADIO, OnSetFocusPlayer3TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER3_TEAM5_RADIO, OnSetFocusPlayer3TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER3_TEAM6_RADIO, OnSetFocusPlayer3TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER3_TEAM7_RADIO, OnSetFocusPlayer3TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER4_TEAM1_RADIO, OnSetFocusPlayer4TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER4_TEAM2_RADIO, OnSetFocusPlayer4TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER4_TEAM3_RADIO, OnSetFocusPlayer4TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER4_TEAM4_RADIO, OnSetFocusPlayer4TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER4_TEAM5_RADIO, OnSetFocusPlayer4TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER4_TEAM6_RADIO, OnSetFocusPlayer4TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER4_TEAM7_RADIO, OnSetFocusPlayer4TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER5_TEAM1_RADIO, OnSetFocusPlayer5TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER5_TEAM2_RADIO, OnSetFocusPlayer5TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER5_TEAM3_RADIO, OnSetFocusPlayer5TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER5_TEAM4_RADIO, OnSetFocusPlayer5TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER5_TEAM5_RADIO, OnSetFocusPlayer5TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER5_TEAM6_RADIO, OnSetFocusPlayer5TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER5_TEAM7_RADIO, OnSetFocusPlayer5TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER6_TEAM1_RADIO, OnSetFocusPlayer6TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER6_TEAM2_RADIO, OnSetFocusPlayer6TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER6_TEAM3_RADIO, OnSetFocusPlayer6TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER6_TEAM4_RADIO, OnSetFocusPlayer6TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER6_TEAM5_RADIO, OnSetFocusPlayer6TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER6_TEAM6_RADIO, OnSetFocusPlayer6TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER6_TEAM7_RADIO, OnSetFocusPlayer6TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER7_TEAM1_RADIO, OnSetFocusPlayer7TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER7_TEAM2_RADIO, OnSetFocusPlayer7TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER7_TEAM3_RADIO, OnSetFocusPlayer7TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER7_TEAM4_RADIO, OnSetFocusPlayer7TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER7_TEAM5_RADIO, OnSetFocusPlayer7TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER7_TEAM6_RADIO, OnSetFocusPlayer7TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER7_TEAM7_RADIO, OnSetFocusPlayer7TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER8_TEAM1_RADIO, OnSetFocusPlayer8TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER8_TEAM2_RADIO, OnSetFocusPlayer8TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER8_TEAM3_RADIO, OnSetFocusPlayer8TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER8_TEAM4_RADIO, OnSetFocusPlayer8TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER8_TEAM5_RADIO, OnSetFocusPlayer8TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER8_TEAM6_RADIO, OnSetFocusPlayer8TeamRadio)
    ON_BN_SETFOCUS(IDC_PLAYER8_TEAM7_RADIO, OnSetFocusPlayer8TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER1_TEAM1_RADIO, OnKillFocusPlayer1TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER1_TEAM2_RADIO, OnKillFocusPlayer1TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER1_TEAM3_RADIO, OnKillFocusPlayer1TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER1_TEAM4_RADIO, OnKillFocusPlayer1TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER1_TEAM5_RADIO, OnKillFocusPlayer1TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER1_TEAM6_RADIO, OnKillFocusPlayer1TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER1_TEAM7_RADIO, OnKillFocusPlayer1TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER2_TEAM1_RADIO, OnKillFocusPlayer2TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER2_TEAM2_RADIO, OnKillFocusPlayer2TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER2_TEAM3_RADIO, OnKillFocusPlayer2TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER2_TEAM4_RADIO, OnKillFocusPlayer2TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER2_TEAM5_RADIO, OnKillFocusPlayer2TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER2_TEAM6_RADIO, OnKillFocusPlayer2TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER2_TEAM7_RADIO, OnKillFocusPlayer2TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER3_TEAM1_RADIO, OnKillFocusPlayer3TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER3_TEAM2_RADIO, OnKillFocusPlayer3TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER3_TEAM3_RADIO, OnKillFocusPlayer3TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER3_TEAM4_RADIO, OnKillFocusPlayer3TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER3_TEAM5_RADIO, OnKillFocusPlayer3TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER3_TEAM6_RADIO, OnKillFocusPlayer3TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER3_TEAM7_RADIO, OnKillFocusPlayer3TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER4_TEAM1_RADIO, OnKillFocusPlayer4TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER4_TEAM2_RADIO, OnKillFocusPlayer4TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER4_TEAM3_RADIO, OnKillFocusPlayer4TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER4_TEAM4_RADIO, OnKillFocusPlayer4TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER4_TEAM5_RADIO, OnKillFocusPlayer4TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER4_TEAM6_RADIO, OnKillFocusPlayer4TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER4_TEAM7_RADIO, OnKillFocusPlayer4TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER5_TEAM1_RADIO, OnKillFocusPlayer5TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER5_TEAM2_RADIO, OnKillFocusPlayer5TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER5_TEAM3_RADIO, OnKillFocusPlayer5TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER5_TEAM4_RADIO, OnKillFocusPlayer5TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER5_TEAM5_RADIO, OnKillFocusPlayer5TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER5_TEAM6_RADIO, OnKillFocusPlayer5TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER5_TEAM7_RADIO, OnKillFocusPlayer5TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER6_TEAM1_RADIO, OnKillFocusPlayer6TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER6_TEAM2_RADIO, OnKillFocusPlayer6TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER6_TEAM3_RADIO, OnKillFocusPlayer6TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER6_TEAM4_RADIO, OnKillFocusPlayer6TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER6_TEAM5_RADIO, OnKillFocusPlayer6TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER6_TEAM6_RADIO, OnKillFocusPlayer6TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER6_TEAM7_RADIO, OnKillFocusPlayer6TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER7_TEAM1_RADIO, OnKillFocusPlayer7TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER7_TEAM2_RADIO, OnKillFocusPlayer7TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER7_TEAM3_RADIO, OnKillFocusPlayer7TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER7_TEAM4_RADIO, OnKillFocusPlayer7TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER7_TEAM5_RADIO, OnKillFocusPlayer7TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER7_TEAM6_RADIO, OnKillFocusPlayer7TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER7_TEAM7_RADIO, OnKillFocusPlayer7TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER8_TEAM1_RADIO, OnKillFocusPlayer8TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER8_TEAM2_RADIO, OnKillFocusPlayer8TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER8_TEAM3_RADIO, OnKillFocusPlayer8TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER8_TEAM4_RADIO, OnKillFocusPlayer8TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER8_TEAM5_RADIO, OnKillFocusPlayer8TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER8_TEAM6_RADIO, OnKillFocusPlayer8TeamRadio)
    ON_BN_KILLFOCUS(IDC_PLAYER8_TEAM7_RADIO, OnKillFocusPlayer8TeamRadio)
    ON_BN_CLICKED(IDC_PLAYER1_STATIC, OnPlayer1Static)
    ON_BN_CLICKED(IDC_PLAYER2_STATIC, OnPlayer2Static)
    ON_BN_CLICKED(IDC_PLAYER3_STATIC, OnPlayer3Static)
    ON_BN_CLICKED(IDC_PLAYER4_STATIC, OnPlayer4Static)
    ON_BN_CLICKED(IDC_PLAYER5_STATIC, OnPlayer5Static)
    ON_BN_CLICKED(IDC_PLAYER6_STATIC, OnPlayer6Static)
    ON_BN_CLICKED(IDC_PLAYER7_STATIC, OnPlayer7Static)
    ON_BN_CLICKED(IDC_PLAYER8_STATIC, OnPlayer8Static)
END_MESSAGE_MAP()

VA(0x00477459, 0x469)
BOOL TMapSpecsTeamsPage::OnInitDialog()
{
    GetDlgItem(IDC_ENABLE_TEAMS_CHECK)->SetWindowText(SMapSpecsTeamsPageText::kEnableTeamsCheckStr);
    GetDlgItem(IDC_NUM_TEAMS_STATIC)->SetWindowText(SMapSpecsTeamsPageText::kNumberOfTeamsStaticStr);
    GetDlgItem(IDC_TEAM_ASSIGNMENTS_STATIC)->SetWindowText(SMapSpecsTeamsPageText::kTeamAssignmentsStaticStr);
    GetDlgItem(IDC_TEAM_NUMBER_STATIC)->SetWindowText(SMapSpecsTeamsPageText::kTeamNumberStaticStr);
    GetDlgItem(IDC_PLAYER1_STATIC)->SetWindowText(
        TFormattedString(SMapSpecsTeamsPageText::kPlayerStaticFmtStr, 1, akPlayerTraits[0].m_pColorName));
    GetDlgItem(IDC_PLAYER2_STATIC)->SetWindowText(
        TFormattedString(SMapSpecsTeamsPageText::kPlayerStaticFmtStr, 2, akPlayerTraits[1].m_pColorName));
    GetDlgItem(IDC_PLAYER3_STATIC)->SetWindowText(
        TFormattedString(SMapSpecsTeamsPageText::kPlayerStaticFmtStr, 3, akPlayerTraits[2].m_pColorName));
    GetDlgItem(IDC_PLAYER4_STATIC)->SetWindowText(
        TFormattedString(SMapSpecsTeamsPageText::kPlayerStaticFmtStr, 4, akPlayerTraits[3].m_pColorName));
    GetDlgItem(IDC_PLAYER5_STATIC)->SetWindowText(
        TFormattedString(SMapSpecsTeamsPageText::kPlayerStaticFmtStr, 5, akPlayerTraits[4].m_pColorName));
    GetDlgItem(IDC_PLAYER6_STATIC)->SetWindowText(
        TFormattedString(SMapSpecsTeamsPageText::kPlayerStaticFmtStr, 6, akPlayerTraits[5].m_pColorName));
    GetDlgItem(IDC_PLAYER7_STATIC)->SetWindowText(
        TFormattedString(SMapSpecsTeamsPageText::kPlayerStaticFmtStr, 7, akPlayerTraits[6].m_pColorName));
    GetDlgItem(IDC_PLAYER8_STATIC)->SetWindowText(
        TFormattedString(SMapSpecsTeamsPageText::kPlayerStaticFmtStr, 8, akPlayerTraits[7].m_pColorName));
    _m_bModified = false;
    const TTeamInfo& teamInfo = _m_newMap.getTeamInfo();
    unsigned int player;
    if (teamInfo.getBHasTeams()) {
        _m_numTeams = teamInfo.getNumTeams() - TTeamInfo::s_kMinTeams;
        for (player = 0; player < kNumPlayers; player++) {
            if (_m_newMap.getPlayers()[player].getBPresent())
                this->*_s_apPlayerTeam[player] = teamInfo.getPlayerTeam(TPlayer(player));
            else
                this->*_s_apPlayerTeam[player] = -1;
        }
    } else {
        _m_numTeams = -1;
        for (player = 0; player < kNumPlayers; player++)
            this->*_s_apPlayerTeam[player] = -1;
    }
    CPropertyPage::OnInitDialog();
    _m_enableTeamsCheck.SetCheck(teamInfo.getBHasTeams());
    if (!teamInfo.getBHasTeams()) {
        for (CWnd* pWnd = _m_enableTeamsCheck.GetWindow(GW_HWNDNEXT); pWnd != NULL; pWnd = pWnd->GetWindow(GW_HWNDNEXT))
            pWnd->EnableWindow(FALSE);
        if (_m_newMap.getNumPlayableSlots() <= TTeamInfo::s_kMinTeams)
            _m_enableTeamsCheck.EnableWindow(FALSE);
    } else {
        for (unsigned int i = _m_newMap.getNumPlayableSlots() - TTeamInfo::s_kMinTeams; i < s_kNumNumTeamsRadios; i++)
            _m_aNumTeamsRadios[i].EnableWindow(FALSE);
        for (player = 0; player < kNumPlayers; player++) {
            if (_m_newMap.getPlayers()[player].getBPresent()) {
                for (unsigned int team = _m_numTeams + TTeamInfo::s_kMinTeams; team < TTeamInfo::s_kMaxTeams; team++)
                    _m_aPlayerControls[player].m_aTeamRadios[team].EnableWindow(FALSE);
            } else {
                _m_aPlayerControls[player].m_playerStatic.EnableWindow(FALSE);
                for (unsigned int team = 0; team < TTeamInfo::s_kMaxTeams; team++)
                    _m_aPlayerControls[player].m_aTeamRadios[team].EnableWindow(FALSE);
            }
        }
    }
    return TRUE;
}

VA(0x004778c2, 0xef)
void TMapSpecsTeamsPage::OnOK()
{
    CPropertyPage::OnOK();
    const TArray<TPlayerInfo, kNumPlayers>& players = _m_newMap.getPlayers();
    TTeamInfo teamInfo;
    teamInfo.setBHasTeams(_m_enableTeamsCheck.GetCheck() != 0);
    if (teamInfo.getBHasTeams()) {
        teamInfo.setNumTeams(_m_numTeams + TTeamInfo::s_kMinTeams);
        for (unsigned int player = 0; player < kNumPlayers; player++) {
            if (players[player].getBPresent())
                teamInfo.setPlayerTeam(TPlayer(player), this->*_s_apPlayerTeam[player]);
            else
                teamInfo.setPlayerTeam(TPlayer(player), 0);
        }
    }
    _m_newMap.setTeamInfo(teamInfo);
    _m_bModified = _m_bModified || _m_newMap.getTeamInfo() != _m_oldMap.getTeamInfo();
}

VA_COMPGEN(0x004779de, 0xa, IMPLICIT_DTOR, TTeamInfo)

VA(0x004779e8, 0x1f3)
void TMapSpecsTeamsPage::OnEnableTeamsCheck()
{
    if (_m_enableTeamsCheck.GetCheck()) {
        _m_numTeamsGroup.EnableWindow(TRUE);
        _m_teamAssignmentsGroup.EnableWindow(TRUE);
        _m_teamNumberStatic.EnableWindow(TRUE);
        _m_numTeams = 0;
        const TArray<TPlayerInfo, kNumPlayers>& players = _m_newMap.getPlayers();
        unsigned int team = 0;
        unsigned int numTeamsLeft = TTeamInfo::s_kMinTeams;
        unsigned int player = 0;
        unsigned int numPlayersLeft = _m_newMap.getNumPlayableSlots();
        while (numTeamsLeft > 0) {
            unsigned int numInTeam = (numPlayersLeft + numTeamsLeft - 1) / numTeamsLeft;
            numPlayersLeft -= numInTeam;
            numTeamsLeft--;
            while (numInTeam > 0) {
                if (players[player].getBPresent()) {
                    this->*_s_apPlayerTeam[player] = team;
                    numInTeam--;
                }
                player++;
            }
            team++;
        }
        CWnd* pWnd;
        for (pWnd = _m_enableTeamsCheck.GetWindow(GW_HWNDNEXT); pWnd != NULL; pWnd = pWnd->GetWindow(GW_HWNDNEXT)) {
            if ((pWnd->GetDlgCtrlID() & 0xffff) == 0xffff)
                pWnd->EnableWindow(TRUE);
        }
        for (unsigned int i = 0; i < _m_newMap.getNumPlayableSlots() - TTeamInfo::s_kMinTeams; i++)
            _m_aNumTeamsRadios[i].EnableWindow(TRUE);
        for (player = 0; player < kNumPlayers; player++) {
            if (_m_newMap.getPlayers()[player].getBPresent()) {
                _m_aPlayerControls[player].m_playerStatic.EnableWindow(TRUE);
                for (team = 0; team < TTeamInfo::s_kMinTeams; team++)
                    _m_aPlayerControls[player].m_aTeamRadios[team].EnableWindow(TRUE);
            }
        }
        UpdateData(FALSE);
    } else {
        _m_numTeams = -1;
        for (unsigned int player = 0; player < kNumPlayers; player++)
            this->*_s_apPlayerTeam[player] = -1;
        UpdateData(FALSE);
        for (CWnd* pWnd = _m_enableTeamsCheck.GetWindow(GW_HWNDNEXT); pWnd != NULL; pWnd = pWnd->GetWindow(GW_HWNDNEXT))
            pWnd->EnableWindow(FALSE);
    }
}

VA(0x00477bdb, 0x14d)
void TMapSpecsTeamsPage::OnNumTeamsRadio()
{
    int oldNumTeams = _m_numTeams;
    UpdateData(TRUE);
    if (_m_numTeams == oldNumTeams)
        return;
    const TArray<TPlayerInfo, kNumPlayers>& players = _m_newMap.getPlayers();
    unsigned int team = 0;
    unsigned int player = 0;
    unsigned int numTeamsLeft = _m_numTeams + TTeamInfo::s_kMinTeams;
    unsigned int numPlayersLeft = _m_newMap.getNumPlayableSlots();
    while (numTeamsLeft > 0) {
        unsigned int numInTeam = (numPlayersLeft + numTeamsLeft - 1) / numTeamsLeft;
        numPlayersLeft -= numInTeam;
        numTeamsLeft--;
        while (numInTeam > 0) {
            if (players[player].getBPresent()) {
                this->*_s_apPlayerTeam[player] = team;
                unsigned int radio;
                for (radio = TTeamInfo::s_kMinTeams; radio < unsigned(_m_numTeams + TTeamInfo::s_kMinTeams); radio++)
                    _m_aPlayerControls[player].m_aTeamRadios[radio].EnableWindow(TRUE);
                for (; radio < TTeamInfo::s_kMaxTeams; radio++)
                    _m_aPlayerControls[player].m_aTeamRadios[radio].EnableWindow(FALSE);
                numInTeam--;
            }
            player++;
        }
        team++;
    }
    UpdateData(FALSE);
}

VA(0x00477d28, 0x8)
void TMapSpecsTeamsPage::OnPlayer1TeamRadio()
{
    _onPlayerTeamRadio(0);
}

VA(0x00477d30, 0x8)
void TMapSpecsTeamsPage::OnPlayer2TeamRadio()
{
    _onPlayerTeamRadio(1);
}

VA(0x00477d38, 0x8)
void TMapSpecsTeamsPage::OnPlayer3TeamRadio()
{
    _onPlayerTeamRadio(2);
}

VA(0x00477d40, 0x8)
void TMapSpecsTeamsPage::OnPlayer4TeamRadio()
{
    _onPlayerTeamRadio(3);
}

VA(0x00477d48, 0x8)
void TMapSpecsTeamsPage::OnPlayer5TeamRadio()
{
    _onPlayerTeamRadio(4);
}

VA(0x00477d50, 0x8)
void TMapSpecsTeamsPage::OnPlayer6TeamRadio()
{
    _onPlayerTeamRadio(5);
}

VA(0x00477d58, 0x8)
void TMapSpecsTeamsPage::OnPlayer7TeamRadio()
{
    _onPlayerTeamRadio(6);
}

VA(0x00477d60, 0x8)
void TMapSpecsTeamsPage::OnPlayer8TeamRadio()
{
    _onPlayerTeamRadio(7);
}

VA(0x00477d68, 0x8)
void TMapSpecsTeamsPage::OnSetFocusPlayer1TeamRadio()
{
    _onSetFocusPlayerTeamRadio(0);
}

VA(0x00477d70, 0x8)
void TMapSpecsTeamsPage::OnSetFocusPlayer2TeamRadio()
{
    _onSetFocusPlayerTeamRadio(1);
}

VA(0x00477d78, 0x8)
void TMapSpecsTeamsPage::OnSetFocusPlayer3TeamRadio()
{
    _onSetFocusPlayerTeamRadio(2);
}

VA(0x00477d80, 0x8)
void TMapSpecsTeamsPage::OnSetFocusPlayer4TeamRadio()
{
    _onSetFocusPlayerTeamRadio(3);
}

VA(0x00477d88, 0x8)
void TMapSpecsTeamsPage::OnSetFocusPlayer5TeamRadio()
{
    _onSetFocusPlayerTeamRadio(4);
}

VA(0x00477d90, 0x8)
void TMapSpecsTeamsPage::OnSetFocusPlayer6TeamRadio()
{
    _onSetFocusPlayerTeamRadio(5);
}

VA(0x00477d98, 0x8)
void TMapSpecsTeamsPage::OnSetFocusPlayer7TeamRadio()
{
    _onSetFocusPlayerTeamRadio(6);
}

VA(0x00477da0, 0x8)
void TMapSpecsTeamsPage::OnSetFocusPlayer8TeamRadio()
{
    _onSetFocusPlayerTeamRadio(7);
}

VA(0x00477da8, 0x8)
void TMapSpecsTeamsPage::OnKillFocusPlayer1TeamRadio()
{
    _onKillFocusPlayerTeamRadio(0);
}

VA(0x00477db0, 0x8)
void TMapSpecsTeamsPage::OnKillFocusPlayer2TeamRadio()
{
    _onKillFocusPlayerTeamRadio(1);
}

VA(0x00477db8, 0x8)
void TMapSpecsTeamsPage::OnKillFocusPlayer3TeamRadio()
{
    _onKillFocusPlayerTeamRadio(2);
}

VA(0x00477dc0, 0x8)
void TMapSpecsTeamsPage::OnKillFocusPlayer4TeamRadio()
{
    _onKillFocusPlayerTeamRadio(3);
}

VA(0x00477dc8, 0x8)
void TMapSpecsTeamsPage::OnKillFocusPlayer5TeamRadio()
{
    _onKillFocusPlayerTeamRadio(4);
}

VA(0x00477dd0, 0x8)
void TMapSpecsTeamsPage::OnKillFocusPlayer6TeamRadio()
{
    _onKillFocusPlayerTeamRadio(5);
}

VA(0x00477dd8, 0x8)
void TMapSpecsTeamsPage::OnKillFocusPlayer7TeamRadio()
{
    _onKillFocusPlayerTeamRadio(6);
}

VA(0x00477de0, 0x8)
void TMapSpecsTeamsPage::OnKillFocusPlayer8TeamRadio()
{
    _onKillFocusPlayerTeamRadio(7);
}

VA(0x00477de8, 0x8)
void TMapSpecsTeamsPage::OnPlayer1Static()
{
    _onPlayerStatic(0);
}

VA(0x00477df0, 0x8)
void TMapSpecsTeamsPage::OnPlayer2Static()
{
    _onPlayerStatic(1);
}

VA(0x00477df8, 0x8)
void TMapSpecsTeamsPage::OnPlayer3Static()
{
    _onPlayerStatic(2);
}

VA(0x00477e00, 0x8)
void TMapSpecsTeamsPage::OnPlayer4Static()
{
    _onPlayerStatic(3);
}

VA(0x00477e08, 0x8)
void TMapSpecsTeamsPage::OnPlayer5Static()
{
    _onPlayerStatic(4);
}

VA(0x00477e10, 0x8)
void TMapSpecsTeamsPage::OnPlayer6Static()
{
    _onPlayerStatic(5);
}

VA(0x00477e18, 0x8)
void TMapSpecsTeamsPage::OnPlayer7Static()
{
    _onPlayerStatic(6);
}

VA(0x00477e20, 0x8)
void TMapSpecsTeamsPage::OnPlayer8Static()
{
    _onPlayerStatic(7);
}
