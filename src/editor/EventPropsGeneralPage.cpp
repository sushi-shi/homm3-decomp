// EventPropsGeneralPage.cpp - the general page of the event property sheet
// (h3maped 0x41948b..0x419da1; GOG only). Players not on the map keep
// their checks disabled; a Restoration of Erathia map keeps 300 characters
// of the message.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Event.h"
#include "editor/EventPropsGeneralPage.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"

DATA(0x00534ad0)
static const int g_akPlayerCheckIDs[kNumPlayers] = {
    IDC_EVENT_PLAYER1_CHECK, IDC_EVENT_PLAYER2_CHECK, IDC_EVENT_PLAYER3_CHECK, IDC_EVENT_PLAYER4_CHECK,
    IDC_EVENT_PLAYER5_CHECK, IDC_EVENT_PLAYER6_CHECK, IDC_EVENT_PLAYER7_CHECK, IDC_EVENT_PLAYER8_CHECK
};

VA(0x004196cb, 0xda)
TEventPropsGeneralPage::TEventPropsGeneralPage(TEvent* pEvent, const TPlayerMask& playersPresent,
                                               EGameVersion mapVersion)
    : CPropertyPage(TEventPropsGeneralPage::IDD),
      _m_pEvent(pEvent),
      _m_playersPresent(playersPresent),
      _m_mapVersion(mapVersion),
      _m_bModified(false),
      _m_bAllowPlayer(0)
{
    _m_bAllowComputer = FALSE;
    _m_bCancelAfterVisit = FALSE;
    _m_message = _T("");
    m_psp.pszTitle = m_strCaption = kGeneralPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x004197a5, 0x1c, SCALAR_DELETING_DTOR, TEventPropsGeneralPage)

VA(0x004197c1, 0x69)
TEventPropsGeneralPage::~TEventPropsGeneralPage()
{
}

VA(0x0041982a, 0x33)
bool TEventPropsGeneralPage::getBAllowPlayer(TPlayer player) const
{
    return _m_bAllowPlayer.test(player);
}

VA(0x0041985d, 0x76)
void TEventPropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_MESSAGE_EDIT, _m_messageEdit);
    DDX_Check(pDX, IDC_EVENT_COMPUTER_CHECK, _m_bAllowComputer);
    DDX_Check(pDX, IDC_EVENT_CANCEL_CHECK, _m_bCancelAfterVisit);
    DDX_Text(pDX, IDC_MESSAGE_EDIT, _m_message);
    for (unsigned int player = 0; player < kNumPlayers; player++)
        DDX_Control(pDX, g_akPlayerCheckIDs[player], _m_aPlayerChecks[player]);
}

VA(0x004198d3, 0x6)
BEGIN_MESSAGE_MAP(TEventPropsGeneralPage, CPropertyPage)
END_MESSAGE_MAP()

VA(0x004198d9, 0x364)
BOOL TEventPropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_MESSAGE_STATIC)->SetWindowText(SBlackBoxPropsGeneralPageText::kMessageStaticStr);
    GetDlgItem(IDC_EVENT_ALLOWED_PLAYERS_STATIC)->SetWindowText(SEventPropsGeneralPageText::kAllowedPlayersStaticStr);
    GetDlgItem(IDC_EVENT_PLAYER1_CHECK)->SetWindowText(TFormattedString(SEventPropsGeneralPageText::kPlayerCheckFmtStr, 1));
    GetDlgItem(IDC_EVENT_PLAYER2_CHECK)->SetWindowText(TFormattedString(SEventPropsGeneralPageText::kPlayerCheckFmtStr, 2));
    GetDlgItem(IDC_EVENT_PLAYER3_CHECK)->SetWindowText(TFormattedString(SEventPropsGeneralPageText::kPlayerCheckFmtStr, 3));
    GetDlgItem(IDC_EVENT_PLAYER4_CHECK)->SetWindowText(TFormattedString(SEventPropsGeneralPageText::kPlayerCheckFmtStr, 4));
    GetDlgItem(IDC_EVENT_PLAYER5_CHECK)->SetWindowText(TFormattedString(SEventPropsGeneralPageText::kPlayerCheckFmtStr, 5));
    GetDlgItem(IDC_EVENT_PLAYER6_CHECK)->SetWindowText(TFormattedString(SEventPropsGeneralPageText::kPlayerCheckFmtStr, 6));
    GetDlgItem(IDC_EVENT_PLAYER7_CHECK)->SetWindowText(TFormattedString(SEventPropsGeneralPageText::kPlayerCheckFmtStr, 7));
    GetDlgItem(IDC_EVENT_PLAYER8_CHECK)->SetWindowText(TFormattedString(SEventPropsGeneralPageText::kPlayerCheckFmtStr, 8));
    GetDlgItem(IDC_EVENT_COMPUTER_CHECK)->SetWindowText(SEventPropsGeneralPageText::kAllowComputerCheckStr);
    GetDlgItem(IDC_EVENT_CANCEL_CHECK)->SetWindowText(SEventPropsGeneralPageText::kCancelCheckStr);
    _m_bModified = false;
    _m_message = _m_pEvent->getMessage().c_str();
    _m_message.Replace("\n", "\r\n");
    _m_bAllowComputer = _m_pEvent->getBAllowComputer();
    _m_bCancelAfterVisit = _m_pEvent->getBCancelAfterVisit();
    unsigned int player;
    for (player = 0; player < kNumPlayers; player++)
        _m_bAllowPlayer[player] = _m_pEvent->getBAllowPlayer(TPlayer(player));
    CPropertyPage::OnInitDialog();
    if (_m_mapVersion == GAME_VERSION_ROE)
        _m_messageEdit.LimitText(TTreasure::s_kMaxMessageLen);
    for (player = 0; player < kNumPlayers; player++) {
        if (_m_playersPresent[player])
            _m_aPlayerChecks[player].SetCheck(_m_bAllowPlayer[player]);
        else
            _m_aPlayerChecks[player].EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x00419c3d, 0x164)
void TEventPropsGeneralPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_message.Replace("\r\n", "\n");
    unsigned int player;
    for (player = 0; player < kNumPlayers; player++) {
        if (_m_playersPresent[player])
            _m_bAllowPlayer.set(player, _m_aPlayerChecks[player].GetCheck() != 0);
    }
    _m_bModified = _m_bModified || std::string(_m_message) != _m_pEvent->getMessage()
                   || (_m_bAllowComputer != FALSE) != _m_pEvent->getBAllowComputer()
                   || (_m_bCancelAfterVisit != FALSE) != _m_pEvent->getBCancelAfterVisit();
    for (player = 0; !_m_bModified && player < kNumPlayers; player++)
        _m_bModified = _m_bAllowPlayer[player] != _m_pEvent->getBAllowPlayer(TPlayer(player));
}
