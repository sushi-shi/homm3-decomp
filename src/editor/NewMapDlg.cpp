// NewMapDlg.cpp - the new map dialog (h3maped 0x48a5ac..0x48b8ef; GOG
// only). The generate check grows the window to show the random map
// settings below it; the player and team combo boxes offer only the
// counts that still fit beside the human players chosen. OK resolves
// every random setting before it hands them to the map document.
#include "editor/stdafx.h"

#include <stdlib.h>

#include "exceptions.h"
#include "va.h"
#include "gamecontext.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"
#include "editor/NewMapDlg.h"
#include "editor/Player.h"

// The dialog's size in dialog units without and with the random map
// settings.
VA_COMPGEN(0x0048a780, 0x15, STATIC_CTOR, kCollapsedDialogSize)
DATA(0x005a1fb0) static const CSize kCollapsedDialogSize(226, 134);
VA_COMPGEN(0x0048a795, 0x15, STATIC_CTOR, kExpandedDialogSize)
DATA(0x005a1fa8) static const CSize kExpandedDialogSize(311, 272);

VA(0x0048a7aa, 0x167)
TNewMapDlg::TNewMapDlg(CWnd* pParent, const TMapDoc::TNewMapParams& params)
    : _m_params(params),
      _m_pTemplate(NULL)
{
    _m_size = -1;
    _m_bTwoLevel = FALSE;
    _m_version = -1;
    _m_waterContent = -1;
    _m_monsterStrength = -1;
    HINSTANCE hInstance = AfxGetResourceHandle();
    HRSRC hResource = FindResource(hInstance, MAKEINTRESOURCE(IDD), RT_DIALOG);
    if (hResource == NULL)
        return;
    HGLOBAL hTemplate = LoadResource(hInstance, hResource);
    if (hTemplate == NULL)
        return;
    const void* pTemplate = LockResource(hTemplate);
    if (pTemplate == NULL)
        return;
    DWORD size = SizeofResource(hInstance, hResource);
    _m_pTemplate = static_cast<DLGTEMPLATE*>(malloc(size));
    if (_m_pTemplate == NULL)
        return;
    memcpy(_m_pTemplate, pTemplate, size);
    _m_pTemplate->cx = short(kCollapsedDialogSize.cx);
    _m_pTemplate->cy = short(kCollapsedDialogSize.cy);
    InitModalIndirect(_m_pTemplate, pParent);
}

VA_COMPGEN(0x0048a911, 0x1c, SCALAR_DELETING_DTOR, TNewMapDlg)

VA(0x0048a92d, 0xa3)
TNewMapDlg::~TNewMapDlg()
{
    if (_m_pTemplate != NULL)
        free(_m_pTemplate);
}

VA(0x0048a9d0, 0xbe)
void TNewMapDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_GENERATE_RANDOM_MAP_CHECK, _m_randomMapCheck);
    DDX_Control(pDX, IDC_COMPUTER_TEAMS_COMBO, _m_computerTeamsCombo);
    DDX_Control(pDX, IDC_HUMAN_TEAMS_COMBO, _m_humanTeamsCombo);
    DDX_Control(pDX, IDC_COMPUTER_PLAYERS_COMBO, _m_computerPlayersCombo);
    DDX_Control(pDX, IDC_HUMAN_PLAYERS_COMBO, _m_humanPlayersCombo);
    DDX_Radio(pDX, IDC_SIZE_36X36_RADIO, _m_size);
    DDX_Check(pDX, IDC_TWO_LEVEL_MAP_CHECK, _m_bTwoLevel);
    DDX_Radio(pDX, IDC_VERSION_ROE_RADIO, _m_version);
    DDX_Radio(pDX, IDC_WATER_RANDOM_RADIO, _m_waterContent);
    DDX_Radio(pDX, IDC_MONSTER_RANDOM_RADIO, _m_monsterStrength);
}

VA(0x0048aa8e, 0x235)
void TNewMapDlg::OnOK()
{
    CDialog::OnOK();
    _m_params.m_version = EGameVersion(_m_version);
    _m_params.m_size = TGameMap::TSize(_m_size);
    _m_params.m_bTwoLayer = _m_bTwoLevel != FALSE;
    if (!_m_randomMapCheck.GetCheck())
        return;
    int humanPlayerCount = _m_humanPlayersCombo.GetItemData(_m_humanPlayersCombo.GetCurSel());
    int humanTeamCount = _m_humanTeamsCombo.GetItemData(_m_humanTeamsCombo.GetCurSel());
    int computerPlayerCount = _m_computerPlayersCombo.GetItemData(_m_computerPlayersCombo.GetCurSel());
    int computerTeamCount = _m_computerTeamsCombo.GetItemData(_m_computerTeamsCombo.GetCurSel());
    if (humanPlayerCount == -1)
        humanPlayerCount = rand() % kNumPlayers + 1;
    if (humanTeamCount == -1)
        humanTeamCount = rand() % humanPlayerCount + 1;
    else if (humanTeamCount == 0 || humanTeamCount > humanPlayerCount)
        humanTeamCount = humanPlayerCount;
    if (humanPlayerCount >= kNumPlayers) {
        computerPlayerCount = computerTeamCount = 0;
    } else {
        if (computerPlayerCount == -1)
            computerPlayerCount = rand() % (kNumPlayers - humanPlayerCount + 1);
        else if (computerPlayerCount > kNumPlayers - humanPlayerCount)
            computerPlayerCount = kNumPlayers - humanPlayerCount;
        if (computerPlayerCount != 0) {
            if (computerTeamCount == -1)
                computerTeamCount = rand() % computerPlayerCount + 1;
            else if (computerTeamCount == 0 || computerTeamCount > computerPlayerCount)
                computerTeamCount = computerPlayerCount;
        } else
            computerTeamCount = 0;
    }
    ERmgWaterContent waterContent;
    if (_m_waterContent == 0)
        waterContent = RMG_WATER_RANDOM;
    else
        waterContent = ERmgWaterContent(_m_waterContent - 1);
    int monsterStrength;
    if (_m_monsterStrength == 0)
        monsterStrength = rand() % 3 - 1;
    else
        monsterStrength = _m_monsterStrength - 2;
    std::auto_ptr<TMapDoc::TRandomMapParams> pRandomMapParams(new TMapDoc::TRandomMapParams);
    if (!pRandomMapParams.get())
        throw TAllocationFailure();
    pRandomMapParams->m_humanPlayerCount = humanPlayerCount;
    pRandomMapParams->m_humanTeamCount = humanTeamCount;
    pRandomMapParams->m_computerPlayerCount = computerPlayerCount;
    pRandomMapParams->m_computerTeamCount = computerTeamCount;
    pRandomMapParams->m_waterContent = waterContent;
    pRandomMapParams->m_monsterStrength = monsterStrength;
    _m_params.m_pRandomMapParams = pRandomMapParams;
}

VA(0x0048acc3, 0x6)
BEGIN_MESSAGE_MAP(TNewMapDlg, CDialog)
    ON_WM_CREATE()
    ON_CBN_SELCHANGE(IDC_HUMAN_PLAYERS_COMBO, OnSelchangeHumanPlayersCombo)
    ON_CBN_SELCHANGE(IDC_COMPUTER_PLAYERS_COMBO, OnSelchangeComputerPlayersCombo)
    ON_BN_CLICKED(IDC_GENERATE_RANDOM_MAP_CHECK, OnGenerateRandomMapCheck)
END_MESSAGE_MAP()

VA(0x0048acc9, 0x25)
int TNewMapDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kNewMapCaptionStr);
    return 0;
}

// Separate population-loop counters with a shared insertion index reproduce
// retail's local homes. These random-map controls are absent from the Linux editor.
VA(0x0048acee, 0x684)
BOOL TNewMapDlg::OnInitDialog()
{
    {
        CRect rect;
        GetWindowRect(&rect);
        _m_collapsedSize = rect.Size();
    }
    {
        CRect rect(0, 0, kExpandedDialogSize.cx, kExpandedDialogSize.cy);
        MapDialogRect(&rect);
        AdjustWindowRectEx(&rect, GetStyle(), FALSE, GetExStyle());
        _m_expandedSize = rect.Size();
    }
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_VERSION_STATIC)->SetWindowText(SNewMapDlgText::kMapVersionStaticStr);
    GetDlgItem(IDC_VERSION_ROE_RADIO)->SetWindowText(SNewMapDlgText::kRoERadioStr);
    GetDlgItem(IDC_VERSION_AB_RADIO)->SetWindowText(SNewMapDlgText::kABRadioStr);
    GetDlgItem(IDC_VERSION_SOD_RADIO)->SetWindowText(SNewMapDlgText::kSoDRadioStr);
    GetDlgItem(IDC_MAP_SIZE_STATIC)->SetWindowText(SNewMapDlgText::kMapSizeStaticStr);
    GetDlgItem(IDC_SIZE_36X36_RADIO)->SetWindowText(SNewMapDlgText::k36x36RadioStr);
    GetDlgItem(IDC_SIZE_72X72_RADIO)->SetWindowText(SNewMapDlgText::k72x72RadioStr);
    GetDlgItem(IDC_SIZE_108X108_RADIO)->SetWindowText(SNewMapDlgText::k108x108RadioStr);
    GetDlgItem(IDC_SIZE_144X144_RADIO)->SetWindowText(SNewMapDlgText::k144x144RadioStr);
    GetDlgItem(IDC_TWO_LEVEL_MAP_CHECK)->SetWindowText(SNewMapDlgText::kTwoLevelMapCheckStr);
    GetDlgItem(IDC_GENERATE_RANDOM_MAP_CHECK)->SetWindowText(SNewMapDlgText::kGenerateRandomMapCheckStr);
    GetDlgItem(IDC_NUMBER_OF_PLAYERS_STATIC)->SetWindowText(SNewMapDlgText::kNumberOfPlayersStaticStr);
    GetDlgItem(IDC_HUMAN_PLAYERS_STATIC)->SetWindowText(SNewMapDlgText::kHumanOrComputerStaticStr);
    GetDlgItem(IDC_COMPUTER_PLAYERS_STATIC)->SetWindowText(SNewMapDlgText::kComputerOnlyStaticStr);
    GetDlgItem(IDC_NUM_TEAMS_STATIC)->SetWindowText(SNewMapDlgText::kNumberOfTeamsStaticStr);
    GetDlgItem(IDC_HUMAN_TEAMS_STATIC)->SetWindowText(SNewMapDlgText::kHumanOrComputerStaticStr);
    GetDlgItem(IDC_COMPUTER_TEAMS_STATIC)->SetWindowText(SNewMapDlgText::kComputerOnlyStaticStr);
    GetDlgItem(IDC_WATER_CONTENT_STATIC)->SetWindowText(SNewMapDlgText::kWaterContentStaticStr);
    GetDlgItem(IDC_WATER_RANDOM_RADIO)->SetWindowText(SNewMapDlgText::kWaterRandomRadioStr);
    GetDlgItem(IDC_WATER_NONE_RADIO)->SetWindowText(SNewMapDlgText::kWaterNoneRadioStr);
    GetDlgItem(IDC_WATER_NORMAL_RADIO)->SetWindowText(SNewMapDlgText::kWaterNormalRadioStr);
    GetDlgItem(IDC_WATER_ISLANDS_RADIO)->SetWindowText(SNewMapDlgText::kWaterIslandsRadioStr);
    GetDlgItem(IDC_MONSTER_STRENGTH_STATIC)->SetWindowText(SNewMapDlgText::kMonsterStrengthStaticStr);
    GetDlgItem(IDC_MONSTER_RANDOM_RADIO)->SetWindowText(SNewMapDlgText::kMonsterRandomRadioStr);
    GetDlgItem(IDC_MONSTER_WEAK_RADIO)->SetWindowText(SNewMapDlgText::kMonsterWeakRadioStr);
    GetDlgItem(IDC_MONSTER_NORMAL_RADIO)->SetWindowText(SNewMapDlgText::kMonsterNormalRadioStr);
    GetDlgItem(IDC_MONSTER_STRONG_RADIO)->SetWindowText(SNewMapDlgText::kMonsterStrongRadioStr);
    _m_version = _m_params.m_version;
    _m_size = _m_params.m_size;
    _m_waterContent = 0;
    _m_bTwoLevel = _m_params.m_bTwoLayer;
    _m_monsterStrength = 0;
    CDialog::OnInitDialog();
    _m_randomMapCheck.SetCheck(0);
    if (!g_gameContextFeatures[g_videoGameState].test(GAME_VERSION_AB)) {
        GetDlgItem(IDC_VERSION_AB_RADIO)->EnableWindow(FALSE);
        _m_randomMapCheck.ShowWindow(SW_HIDE);
    }
    int index = _m_humanPlayersCombo.AddString(kRandomStr);
    _m_humanPlayersCombo.SetItemData(index, DWORD(-1));
    _m_humanPlayersCombo.SetCurSel(index);
    for (int humanPlayerCount = 1; humanPlayerCount <= kNumPlayers; humanPlayerCount++) {
        index = _m_humanPlayersCombo.AddString(TFormattedString("%d", humanPlayerCount));
        _m_humanPlayersCombo.SetItemData(index, humanPlayerCount);
    }
    index = _m_computerPlayersCombo.AddString(kRandomStr);
    _m_computerPlayersCombo.SetItemData(index, DWORD(-1));
    _m_computerPlayersCombo.SetCurSel(index);
    for (int computerPlayerCount = 0; computerPlayerCount <= kNumPlayers - 1; computerPlayerCount++) {
        index = _m_computerPlayersCombo.AddString(TFormattedString("%d", computerPlayerCount));
        _m_computerPlayersCombo.SetItemData(index, computerPlayerCount);
    }
    _m_humanTeamsCombo.SetItemData(_m_humanTeamsCombo.AddString(kRandomStr), DWORD(-1));
    index = _m_humanTeamsCombo.AddString(kSelNoneStr);
    _m_humanTeamsCombo.SetItemData(index, 0);
    _m_humanTeamsCombo.SetCurSel(index);
    for (int humanTeamCount = 1; humanTeamCount <= kNumPlayers - 1; humanTeamCount++) {
        index = _m_humanTeamsCombo.AddString(TFormattedString("%d", humanTeamCount));
        _m_humanTeamsCombo.SetItemData(index, humanTeamCount);
    }
    _m_computerTeamsCombo.SetItemData(_m_computerTeamsCombo.AddString(kRandomStr), DWORD(-1));
    index = _m_computerTeamsCombo.AddString(kSelNoneStr);
    _m_computerTeamsCombo.SetItemData(index, 0);
    _m_computerTeamsCombo.SetCurSel(index);
    for (int computerTeamCount = 1; computerTeamCount <= kNumPlayers - 2; computerTeamCount++) {
        index = _m_computerTeamsCombo.AddString(TFormattedString("%d", computerTeamCount));
        _m_computerTeamsCombo.SetItemData(index, computerTeamCount);
    }
    return TRUE;
}

VA(0x0048b372, 0xe0)
void TNewMapDlg::OnGenerateRandomMapCheck()
{
    bool bShow = _m_randomMapCheck.GetCheck() != 0;
    int showCommand = bShow ? SW_SHOW : SW_HIDE;
    for (CWnd* pWnd = _m_randomMapCheck.GetWindow(GW_HWNDNEXT); pWnd->GetDlgCtrlID() != IDOK;
         pWnd = pWnd->GetWindow(GW_HWNDNEXT))
        pWnd->ShowWindow(showCommand);
    CSize size = bShow ? _m_expandedSize : _m_collapsedSize;
    CRect rect;
    GetWindowRect(&rect);
    CWnd* pParent = GetParent();
    if (pParent != NULL && IsChild(pParent))
        pParent->ScreenToClient(&rect);
    rect.right = rect.left + size.cx;
    rect.bottom = rect.top + size.cy;
    MoveWindow(&rect);
}

VA(0x0048b452, 0x345)
void TNewMapDlg::OnSelchangeHumanPlayersCombo()
{
    int humanPlayerCount = _m_humanPlayersCombo.GetItemData(_m_humanPlayersCombo.GetCurSel());
    int maxHumanTeamCount = humanPlayerCount != -1 ? humanPlayerCount - 1 : kNumPlayers - 1;
    int index = _m_humanTeamsCombo.GetCount();
    int lastCount = _m_humanTeamsCombo.GetItemData(index - 1);
    if (lastCount > maxHumanTeamCount) {
        --index;
        do
            _m_humanTeamsCombo.DeleteString(index);
        while (_m_humanTeamsCombo.GetItemData(--index) > maxHumanTeamCount);
        if (_m_humanTeamsCombo.GetCurSel() == CB_ERR)
            _m_humanTeamsCombo.SetCurSel(_m_humanTeamsCombo.GetCount() - 1);
    } else {
        while (lastCount < maxHumanTeamCount) {
            ++lastCount;
            index = _m_humanTeamsCombo.AddString(TFormattedString("%d", lastCount));
            _m_humanTeamsCombo.SetItemData(index, lastCount);
        }
    }
    int maxComputerPlayerCount = humanPlayerCount != -1 ? kNumPlayers - humanPlayerCount : kNumPlayers - 1;
    index = _m_computerPlayersCombo.GetCount();
    int lastComputerPlayerCount = _m_computerPlayersCombo.GetItemData(index - 1);
    if (lastComputerPlayerCount > maxComputerPlayerCount) {
        --index;
        do
            _m_computerPlayersCombo.DeleteString(index);
        while (_m_computerPlayersCombo.GetItemData(--index) > maxComputerPlayerCount);
        if (_m_computerPlayersCombo.GetCurSel() == CB_ERR)
            _m_computerPlayersCombo.SetCurSel(_m_computerPlayersCombo.GetCount() - 1);
    } else {
        while (lastComputerPlayerCount < maxComputerPlayerCount) {
            ++lastComputerPlayerCount;
            index = _m_computerPlayersCombo.AddString(TFormattedString("%d", lastComputerPlayerCount));
            _m_computerPlayersCombo.SetItemData(index, lastComputerPlayerCount);
        }
    }
    int computerPlayerCount = _m_computerPlayersCombo.GetItemData(_m_computerPlayersCombo.GetCurSel());
    if (computerPlayerCount == -1)
        computerPlayerCount = maxComputerPlayerCount;
    int maxComputerTeamCount = computerPlayerCount > 0 ? computerPlayerCount - 1 : 0;
    index = _m_computerTeamsCombo.GetCount();
    lastCount = _m_computerTeamsCombo.GetItemData(index - 1);
    if (lastCount > maxComputerTeamCount) {
        --index;
        do
            _m_computerTeamsCombo.DeleteString(index);
        while (_m_computerTeamsCombo.GetItemData(--index) > maxComputerTeamCount);
        if (_m_computerTeamsCombo.GetCurSel() == CB_ERR)
            _m_computerTeamsCombo.SetCurSel(_m_computerTeamsCombo.GetCount() - 1);
    } else {
        while (lastCount < maxComputerTeamCount) {
            ++lastCount;
            index = _m_computerTeamsCombo.AddString(TFormattedString("%d", lastCount));
            _m_computerTeamsCombo.SetItemData(index, lastCount);
        }
    }
}

VA(0x0048b797, 0x158)
void TNewMapDlg::OnSelchangeComputerPlayersCombo()
{
    int computerPlayerCount = _m_computerPlayersCombo.GetItemData(_m_computerPlayersCombo.GetCurSel());
    if (computerPlayerCount == -1) {
        int humanPlayerCount = _m_humanPlayersCombo.GetItemData(_m_humanPlayersCombo.GetCurSel());
        computerPlayerCount = humanPlayerCount != -1 ? kNumPlayers - humanPlayerCount : kNumPlayers - 1;
    }
    int maxComputerTeamCount = computerPlayerCount > 0 ? computerPlayerCount - 1 : 0;
    int index = _m_computerTeamsCombo.GetCount();
    int lastCount = _m_computerTeamsCombo.GetItemData(index - 1);
    if (lastCount > maxComputerTeamCount) {
        --index;
        do
            _m_computerTeamsCombo.DeleteString(index);
        while (_m_computerTeamsCombo.GetItemData(--index) > maxComputerTeamCount);
        if (_m_computerTeamsCombo.GetCurSel() == CB_ERR)
            _m_computerTeamsCombo.SetCurSel(_m_computerTeamsCombo.GetCount() - 1);
    } else {
        while (lastCount < maxComputerTeamCount) {
            ++lastCount;
            index = _m_computerTeamsCombo.AddString(TFormattedString("%d", lastCount));
            _m_computerTeamsCombo.SetItemData(index, lastCount);
        }
    }
}
