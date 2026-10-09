// EditTimedEventGeneralPage.cpp - the general page of the timed and town
// event sheets (h3maped 0x415dc1..0x416c06; Loki h3maped object 80). The
// event must apply to the human or the computer players: clearing one
// check sets the other. Before Armageddon's Blade the message is limited
// to 300 characters; before Shadow of Death the human check is disabled.
#include "editor/stdafx.h"

#include <stdio.h>

#include "va.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/EditTimedEventGeneralPage.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"
#include "editor/StringUtil.h"
#include "editor/TimedEvent.h"

namespace {

// One entry of the repeat interval combo: its text and interval.
struct TSubsequentIntervalEntryInfo {
    TSubsequentIntervalEntryInfo(const char* pText, unsigned int interval)
        : m_pText(pText), m_interval(interval) {}
    const char* m_pText;
    unsigned int m_interval;
};

}

VA(0x00415ddd, 0x151)
TEditTimedEventGeneralPage::TEditTimedEventGeneralPage(TEditTimedEventGeneralPageParentSheet* pParentSheet,
                                                       const TTimedEvent& event,
                                                       const TPlayerMask& playersPresent,
                                                       EGameVersion mapVersion)
    : CPropertyPage(TEditTimedEventGeneralPage::IDD),
      _m_pParentSheet(pParentSheet),
      _m_event(event),
      _m_playersPresent(playersPresent),
      _m_mapVersion(mapVersion)
{
    _m_name = _T("");
    _m_message = _T("");
    _m_bApplyToComputer = FALSE;
    _m_bApplyToHuman = FALSE;
    m_psp.pszTitle = m_strCaption = kGeneralPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00415f2e, 0x1c, SCALAR_DELETING_DTOR, TEditTimedEventGeneralPage)

VA(0x00415f4a, 0xb4)
TEditTimedEventGeneralPage::~TEditTimedEventGeneralPage()
{
}

VA(0x00415ffe, 0x33)
bool TEditTimedEventGeneralPage::getBApplyToPlayer(TPlayer player) const
{
    return _m_bApplyToPlayer.test(player);
}

VA(0x00416031, 0x110)
void TEditTimedEventGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_EVENT_HUMAN_CHECK, _m_humanCheck);
    DDX_Control(pDX, IDC_EVENT_SUBSEQUENT_COMBO, _m_subsequentCombo);
    DDX_Control(pDX, IDC_EVENT_FIRST_OCCURENCE_SPIN, _m_firstOccurenceSpin);
    DDX_Control(pDX, IDC_EVENT_FIRST_OCCURENCE_EDIT, _m_firstOccurenceEdit);
    DDX_Control(pDX, IDC_MESSAGE_EDIT, _m_messageEdit);
    DDX_Text(pDX, IDC_EVENT_NAME_EDIT, _m_name);
    DDX_Text(pDX, IDC_MESSAGE_EDIT, _m_message);
    DDX_Check(pDX, IDC_EVENT_COMPUTER_CHECK, _m_bApplyToComputer);
    DDX_Check(pDX, IDC_EVENT_HUMAN_CHECK, _m_bApplyToHuman);
    const int aPlayerCheckIDs[kNumPlayers] = {
        IDC_EVENT_PLAYER1_CHECK, IDC_EVENT_PLAYER2_CHECK, IDC_EVENT_PLAYER3_CHECK, IDC_EVENT_PLAYER4_CHECK,
        IDC_EVENT_PLAYER5_CHECK, IDC_EVENT_PLAYER6_CHECK, IDC_EVENT_PLAYER7_CHECK, IDC_EVENT_PLAYER8_CHECK
    };
    for (unsigned int player = 0; player < kNumPlayers; player++)
        DDX_Control(pDX, aPlayerCheckIDs[player], _m_aPlayerChecks[player]);
}

VA(0x00416141, 0x6)
BEGIN_MESSAGE_MAP(TEditTimedEventGeneralPage, CPropertyPage)
    ON_EN_CHANGE(IDC_EVENT_NAME_EDIT, OnChangeNameEdit)
    ON_EN_KILLFOCUS(IDC_EVENT_FIRST_OCCURENCE_EDIT, OnKillFocusFirstOccurenceEdit)
    ON_BN_CLICKED(IDC_EVENT_COMPUTER_CHECK, OnComputerCheck)
    ON_BN_CLICKED(IDC_EVENT_HUMAN_CHECK, OnHumanCheck)
END_MESSAGE_MAP()

VA(0x00416147, 0x60c)
BOOL TEditTimedEventGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_EVENT_NAME_STATIC)->SetWindowText(SEditTimedEventGeneralPageText::kEventNameStaticStr);
    GetDlgItem(IDC_MESSAGE_STATIC)->SetWindowText(SEditTimedEventGeneralPageText::kMessageStaticStr);
    GetDlgItem(IDC_EVENT_ALLOWED_PLAYERS_STATIC)->SetWindowText(SEditTimedEventGeneralPageText::kAllowedPlayersStaticStr);
    GetDlgItem(IDC_EVENT_PLAYER1_CHECK)->SetWindowText(TFormattedString(SEditTimedEventGeneralPageText::kPlayerCheckFmtStr, 1));
    GetDlgItem(IDC_EVENT_PLAYER2_CHECK)->SetWindowText(TFormattedString(SEditTimedEventGeneralPageText::kPlayerCheckFmtStr, 2));
    GetDlgItem(IDC_EVENT_PLAYER3_CHECK)->SetWindowText(TFormattedString(SEditTimedEventGeneralPageText::kPlayerCheckFmtStr, 3));
    GetDlgItem(IDC_EVENT_PLAYER4_CHECK)->SetWindowText(TFormattedString(SEditTimedEventGeneralPageText::kPlayerCheckFmtStr, 4));
    GetDlgItem(IDC_EVENT_PLAYER5_CHECK)->SetWindowText(TFormattedString(SEditTimedEventGeneralPageText::kPlayerCheckFmtStr, 5));
    GetDlgItem(IDC_EVENT_PLAYER6_CHECK)->SetWindowText(TFormattedString(SEditTimedEventGeneralPageText::kPlayerCheckFmtStr, 6));
    GetDlgItem(IDC_EVENT_PLAYER7_CHECK)->SetWindowText(TFormattedString(SEditTimedEventGeneralPageText::kPlayerCheckFmtStr, 7));
    GetDlgItem(IDC_EVENT_PLAYER8_CHECK)->SetWindowText(TFormattedString(SEditTimedEventGeneralPageText::kPlayerCheckFmtStr, 8));
    GetDlgItem(IDC_EVENT_COMPUTER_CHECK)->SetWindowText(SEditTimedEventGeneralPageText::kAllowComputerCheckStr);
    GetDlgItem(IDC_EVENT_FIRST_OCCURENCE_STATIC)->SetWindowText(SEditTimedEventGeneralPageText::kFirstOccurenceStaticStr);
    GetDlgItem(IDC_EVENT_SUBSEQUENT_STATIC)->SetWindowText(SEditTimedEventGeneralPageText::kSubsequentOccurenceStaticStr);
    GetDlgItem(IDC_EVENT_HUMAN_CHECK)->SetWindowText(SEditTimedEventGeneralPageText::kAllowHumanCheckStr);
    _m_bModified = false;
    _m_name = _m_event.getName().c_str();
    _m_bBlankName = _isspace(_m_name);
    _m_message = _m_event.getMessage().c_str();
    _m_message.Replace("\n", "\r\n");
    unsigned int player;
    for (player = 0; player < kNumPlayers; player++)
        _m_bApplyToPlayer[player] = _m_event.getBApplyToPlayer(TPlayer(player));
    _m_bApplyToHuman = _m_event.getBApplyToHuman();
    _m_bApplyToComputer = _m_event.getBApplyToComputer();
    _m_firstOccurence = _m_event.getFirstOccurence();
    _m_subsequentInterval = _m_event.getSubsequentInterval();
    CPropertyPage::OnInitDialog();
    if (_m_mapVersion == GAME_VERSION_ROE)
        _m_messageEdit.LimitText(TTimedEvent::s_kMaxMessageLen);
    if (_m_bBlankName)
        _m_pParentSheet->onDisableOK();
    for (player = 0; player < kNumPlayers; player++) {
        if (_m_playersPresent[player])
            _m_aPlayerChecks[player].SetCheck(_m_bApplyToPlayer[player]);
        else
            _m_aPlayerChecks[player].EnableWindow(FALSE);
    }
    _m_firstOccurenceEdit.LimitText(TDigits<kNumDaysPerYear * 2>::getDigits());
    CString text;
    text.Format("%d", _m_firstOccurence + 1);
    _m_firstOccurenceEdit.SetWindowText(text);
    _m_firstOccurenceSpin.SetRange(1, kNumDaysPerYear * 2);
    static const TSubsequentIntervalEntryInfo akSubsequentIntervals[] = {
        TSubsequentIntervalEntryInfo(kNeverStr, 0),
        TSubsequentIntervalEntryInfo(kEveryDayStr, 1),
        TSubsequentIntervalEntryInfo(kEvery2DaysStr, 2),
        TSubsequentIntervalEntryInfo(kEvery3DaysStr, 3),
        TSubsequentIntervalEntryInfo(kEvery4DaysStr, 4),
        TSubsequentIntervalEntryInfo(kEvery5DaysStr, 5),
        TSubsequentIntervalEntryInfo(kEvery6DaysStr, 6),
        TSubsequentIntervalEntryInfo(kEvery7DaysStr, 7),
        TSubsequentIntervalEntryInfo(kEvery14DaysStr, 14),
        TSubsequentIntervalEntryInfo(kEvery21DaysStr, 21),
        TSubsequentIntervalEntryInfo(kEvery28DaysStr, 28)
    };
    for (unsigned int i = 0; i < sizeof(akSubsequentIntervals) / sizeof(akSubsequentIntervals[0]); i++) {
        int index = _m_subsequentCombo.AddString(akSubsequentIntervals[i].m_pText);
        _m_subsequentCombo.SetItemData(index, akSubsequentIntervals[i].m_interval);
    }
    int cbIndex = 0;
    while (_m_subsequentCombo.GetItemData(cbIndex) != _m_subsequentInterval)
        cbIndex++;
    _m_subsequentCombo.SetCurSel(cbIndex);
    if (_m_mapVersion < GAME_VERSION_SOD)
        _m_humanCheck.EnableWindow(FALSE);
    return TRUE;
}

VA(0x00416754, 0x261)
void TEditTimedEventGeneralPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_message.Replace("\r\n", "\n");
    unsigned int player;
    for (player = 0; player < kNumPlayers; player++) {
        if (_m_playersPresent.test(player))
            _m_bApplyToPlayer.set(player, _m_aPlayerChecks[player].GetCheck() != 0);
    }
    int day = 0;
    CString text;
    _m_firstOccurenceEdit.GetWindowText(text);
    sscanf(text, "%d", &day);
    day = clamp<int>(1, day, kNumDaysPerYear * 2);
    _m_firstOccurence = day - 1;
    _m_subsequentInterval = _m_subsequentCombo.GetItemData(_m_subsequentCombo.GetCurSel());
    _m_bModified = _m_bModified
                   || std::string(_m_name) != _m_event.getName()
                   || std::string(_m_message) != _m_event.getMessage()
                   || (_m_bApplyToHuman != FALSE) != _m_event.getBApplyToHuman()
                   || (_m_bApplyToComputer != FALSE) != _m_event.getBApplyToComputer()
                   || _m_firstOccurence != _m_event.getFirstOccurence()
                   || _m_subsequentInterval != _m_event.getSubsequentInterval();
    for (player = 0; !_m_bModified && player < kNumPlayers; player++)
        _m_bModified = _m_bApplyToPlayer[player] != _m_event.getBApplyToPlayer(TPlayer(player));
}

VA(0x004169b5, 0x4c)
void TEditTimedEventGeneralPage::OnChangeNameEdit()
{
    UpdateData(TRUE);
    if (!_isspace(_m_name)) {
        if (_m_bBlankName) {
            _m_pParentSheet->onEnableOK();
            _m_bBlankName = false;
        }
    } else {
        if (!_m_bBlankName) {
            _m_pParentSheet->onDisableOK();
            _m_bBlankName = true;
        }
    }
}

VA(0x00416a01, 0xa8)
void TEditTimedEventGeneralPage::OnKillFocusFirstOccurenceEdit()
{
    int day = 0;
    CString text;
    _m_firstOccurenceEdit.GetWindowText(text);
    sscanf(text, "%d", &day);
    if (day < 1 || day > kNumDaysPerYear * 2) {
        day = clamp<int>(1, day, kNumDaysPerYear * 2);
        text.Format("%d", day);
        _m_firstOccurenceEdit.SetWindowText(text);
    }
}

VA(0x00416aa9, 0x32)
void TEditTimedEventGeneralPage::OnComputerCheck()
{
    UpdateData(TRUE);
    if (!_m_bApplyToComputer && !_m_bApplyToHuman) {
        _m_bApplyToHuman = TRUE;
        UpdateData(FALSE);
    }
}

VA(0x00416adb, 0x2e)
void TEditTimedEventGeneralPage::OnHumanCheck()
{
    UpdateData(TRUE);
    if (!_m_bApplyToComputer && !_m_bApplyToHuman) {
        _m_bApplyToComputer = TRUE;
        UpdateData(FALSE);
    }
}
