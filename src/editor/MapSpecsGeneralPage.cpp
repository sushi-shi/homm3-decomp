// MapSpecsGeneralPage.cpp - the general page of the map specifications
// sheet (h3maped 0x470bbd..0x4719b0; Loki h3maped object 70). Applying
// the sheet without the underground level asks before deleting it. A
// map's version can only rise, and only to versions the installed game
// supports.
//
// Not restored yet: OnOK (0x471532) and its anonymous
// TTerrainPlacementMapAdapter (0x470d91..0x470ea9), which paints a new
// underground level rock. They need TTerrainPlacementOp's map types,
// which the game declares in rmg.h beside the game's own CObject and
// TPoint; an MFC dialog cannot include those.
#include "editor/stdafx.h"

#include <stdio.h>

#include "va.h"
#include "gamecontext.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/FormattedString.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsGeneralPage.h"

VA(0x00470eac, 0x15c)
TMapSpecsGeneralPage::TMapSpecsGeneralPage(const TGameMap& oldMap, TGameMap& newMap, int terrainStrength)
    : CPropertyPage(TMapSpecsGeneralPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_terrainStrength(terrainStrength),
      _m_bModified(false)
{
    _m_difficulty = -1;
    _m_description = _T("");
    _m_name = _T("");
    _m_bTwoLevel = FALSE;
    _m_version = -1;
    m_psp.pszTitle = m_strCaption = kGeneralPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00471008, 0x1c, SCALAR_DELETING_DTOR, TMapSpecsGeneralPage)

VA(0x00471024, 0xb9)
TMapSpecsGeneralPage::~TMapSpecsGeneralPage()
{
}

VA(0x004710dd, 0xe3)
void TMapSpecsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_VERSION_ROE_RADIO, _m_roeRadio);
    DDX_Control(pDX, IDC_HERO_LEVEL_SPIN, _m_heroLevelSpin);
    DDX_Control(pDX, IDC_HERO_LEVEL_EDIT, _m_heroLevelEdit);
    DDX_Control(pDX, IDC_LIMIT_HERO_LEVEL_CHECK, _m_limitHeroLevelCheck);
    DDX_Control(pDX, IDC_MAP_DESCRIPTION_EDIT, _m_descriptionEdit);
    DDX_Control(pDX, IDC_MAP_NAME_EDIT, _m_nameEdit);
    DDX_Radio(pDX, IDC_DIFFICULTY_EASY_RADIO, _m_difficulty);
    DDX_Text(pDX, IDC_MAP_DESCRIPTION_EDIT, _m_description);
    DDX_Text(pDX, IDC_MAP_NAME_EDIT, _m_name);
    DDX_Check(pDX, IDC_TWO_LEVEL_MAP_CHECK, _m_bTwoLevel);
    DDX_Radio(pDX, IDC_VERSION_ROE_RADIO, _m_version);
    DDX_Control(pDX, IDC_VERSION_AB_RADIO, _m_abRadio);
}

VA(0x004711c0, 0x6)
BEGIN_MESSAGE_MAP(TMapSpecsGeneralPage, CPropertyPage)
    ON_EN_KILLFOCUS(IDC_HERO_LEVEL_EDIT, OnKillFocusHeroLevelEdit)
    ON_BN_CLICKED(IDC_LIMIT_HERO_LEVEL_CHECK, OnLimitHeroLevelCheck)
END_MESSAGE_MAP()

VA(0x004711c6, 0x36c)
BOOL TMapSpecsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_DIFFICULTY_STATIC)->SetWindowText(SMapSpecsGeneralPageText::kDifficultyStaticStr);
    GetDlgItem(IDC_DIFFICULTY_EASY_RADIO)->SetWindowText(SMapSpecsGeneralPageText::kEasyRadioStr);
    GetDlgItem(IDC_DIFFICULTY_NORMAL_RADIO)->SetWindowText(SMapSpecsGeneralPageText::kNormalRadioStr);
    GetDlgItem(IDC_DIFFICULTY_TOUGH_RADIO)->SetWindowText(SMapSpecsGeneralPageText::kToughRadioStr);
    GetDlgItem(IDC_DIFFICULTY_EXPERT_RADIO)->SetWindowText(SMapSpecsGeneralPageText::kExpertRadioStr);
    GetDlgItem(IDC_DIFFICULTY_IMPOSSIBLE_RADIO)->SetWindowText(SMapSpecsGeneralPageText::kImpossibleRadioStr);
    GetDlgItem(IDC_TWO_LEVEL_MAP_CHECK)->SetWindowText(SMapSpecsGeneralPageText::kTwoLevelMapCheckStr);
    GetDlgItem(IDC_LIMIT_HERO_LEVEL_CHECK)->SetWindowText(SMapSpecsGeneralPageText::kLimitHeroLevelCheckStr);
    GetDlgItem(IDC_MAP_NAME_STATIC)->SetWindowText(SMapSpecsGeneralPageText::kMapNameStaticStr);
    GetDlgItem(IDC_MAP_DESCRIPTION_STATIC)->SetWindowText(SMapSpecsGeneralPageText::kDescriptionStaticStr);
    GetDlgItem(IDC_VERSION_STATIC)->SetWindowText(SMapSpecsGeneralPageText::kMapVersionStaticStr);
    GetDlgItem(IDC_VERSION_ROE_RADIO)->SetWindowText(SMapSpecsGeneralPageText::kRoERadioStr);
    GetDlgItem(IDC_VERSION_AB_RADIO)->SetWindowText(SMapSpecsGeneralPageText::kABRadioStr);
    GetDlgItem(IDC_VERSION_SOD_RADIO)->SetWindowText(SMapSpecsGeneralPageText::kSoDRadioStr);
    _m_bModified = false;
    _m_version = _m_newMap.getVersion();
    _m_difficulty = _m_newMap.getDifficulty();
    _m_bTwoLevel = _m_newMap.isTwoLayer();
    _m_name = _m_newMap.getName().c_str();
    _m_description = _m_newMap.getDesc().c_str();
    _m_description.Replace("\n", "\r\n");
    CPropertyPage::OnInitDialog();
    if (_m_version > GAME_VERSION_ROE)
        _m_roeRadio.EnableWindow(FALSE);
    else if (!g_gameContextFeatures[g_videoGameState].test(GAME_VERSION_AB))
        _m_abRadio.EnableWindow(FALSE);
    if (_m_version > GAME_VERSION_AB)
        _m_abRadio.EnableWindow(FALSE);
    _m_nameEdit.LimitText(s_kMaxNameLen);
    _m_descriptionEdit.LimitText(s_kMaxDescLen);
    _m_heroLevelEdit.LimitText(TDigits<s_kMaxHeroLevel>::getDigits());
    _m_heroLevelSpin.SetRange(1, s_kMaxHeroLevel);
    if (_m_newMap.getMaxHeroLevel() == 0) {
        _m_limitHeroLevelCheck.SetCheck(0);
        _m_heroLevelEdit.EnableWindow(FALSE);
        _m_heroLevelSpin.EnableWindow(FALSE);
        _m_heroLevelEdit.SetWindowText("");
    } else {
        _m_limitHeroLevelCheck.SetCheck(1);
        _m_heroLevelEdit.SetWindowText(TFormattedString("%d", _m_newMap.getMaxHeroLevel()));
    }
    if (_m_newMap.getVersion() < GAME_VERSION_AB)
        _m_limitHeroLevelCheck.EnableWindow(FALSE);
    return TRUE;
}

VA(0x0047185a, 0x44)
BOOL TMapSpecsGeneralPage::OnApply()
{
    if (_m_newMap.isTwoLayer() && !_m_bTwoLevel
        && MessageBox(kContinuingWillDeleteUndergroundLayerStr, kWarningStr, MB_YESNO | MB_ICONQUESTION) == IDNO)
        return FALSE;
    return CPropertyPage::OnApply();
}

VA(0x0047189e, 0xa6)
void TMapSpecsGeneralPage::OnKillFocusHeroLevelEdit()
{
    int level = 0;
    CString text;
    _m_heroLevelEdit.GetWindowText(text);
    sscanf(text, "%d", &level);
    if (level < 1 || level > s_kMaxHeroLevel) {
        level = clamp(1, level, int(s_kMaxHeroLevel));
        text.Format("%d", level);
        _m_heroLevelEdit.SetWindowText(text);
    }
}

VA(0x00471944, 0x6c)
void TMapSpecsGeneralPage::OnLimitHeroLevelCheck()
{
    if (_m_limitHeroLevelCheck.GetCheck()) {
        _m_heroLevelEdit.EnableWindow(TRUE);
        _m_heroLevelSpin.EnableWindow(TRUE);
        _m_heroLevelEdit.SetWindowText("1");
    } else {
        _m_heroLevelEdit.SetWindowText("");
        _m_heroLevelEdit.EnableWindow(FALSE);
        _m_heroLevelSpin.EnableWindow(FALSE);
    }
}
