// MapSpecsGeneralPage.cpp - the general page of the map specifications
// sheet (h3maped 0x470bbd..0x4719b0; Loki h3maped object 70). Applying
// the sheet without the underground level asks before deleting it. A
// map's version can only rise, and only to versions the installed game
// supports. A new underground level is painted rock.
#include "editor/stdafx.h"

#include <stdio.h>
#include <string>

#include "va.h"
#include "gamecontext.h"
#include "rmg_terrain_tile.h"
#include "terrainplacement.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/FormattedString.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsGeneralPage.h"

namespace {

// One layer of the map as the terrain placement's map. getSize and
// getTile fold onto the map document's repaint adapter (/OPT:ICF).
class TTerrainPlacementMapAdapter : public TTerrainPlacementOp::TAbstractMap {
public:
    TTerrainPlacementMapAdapter(TGameMap* pMap, bool bSecondLayer) : _m_pMap(pMap), _m_bSecondLayer(bSecondLayer) {}

    virtual void setTile(const TTilePoint& loc, const TRmgTerrainTile& tile);
    virtual void setFrame(const TTilePoint& loc, int frame);
    virtual TTilePoint& getSize(TTilePoint& size);
    virtual TRmgTerrainTile getTile(const TTilePoint& loc);
    virtual H3_ENUM_RETURN(TTerrainType, int) getTerrain(const TTilePoint& loc);
    virtual int getFrame(const TTilePoint& loc);

private:
    TGameMap* _m_pMap;
    bool _m_bSecondLayer;
};

VA(0x00470d91, 0x74)
void TTerrainPlacementMapAdapter::setTile(const TTilePoint& loc, const TRmgTerrainTile& tile)
{
    TGameMap* pMap = _m_pMap;
    TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    TGameMap::TLayer::TCell* pCell = layer.getPCell(loc.x(), loc.y());
    pCell->setTerrainType(TTerrainType(tile.m_terrain));
    pCell->setTileNum(tile.m_frame);
    pCell->setBHFlipped(tile.m_flipX);
    pCell->setBVFlipped(tile.m_flipY);
}

VA(0x00470e05, 0x42)
void TTerrainPlacementMapAdapter::setFrame(const TTilePoint& loc, int frame)
{
    TGameMap* pMap = _m_pMap;
    TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    layer.getPCell(loc.x(), loc.y())->setTileNum(frame);
}

TTilePoint& TTerrainPlacementMapAdapter::getSize(TTilePoint& size)
{
    return size = TTilePoint(_m_pMap->getWidth(), _m_pMap->getHeight());
}

TRmgTerrainTile TTerrainPlacementMapAdapter::getTile(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    const TGameMap::TLayer::TCell& cell = layer.getCell(loc.x(), loc.y());
    TRmgTerrainTile tile;
    tile.m_terrain = H3_IDX(cell.getTerrainType());
    tile.m_frame = cell.getTileNum();
    tile.m_flipX = cell.getBHFlipped();
    tile.m_flipY = cell.getBVFlipped();
    return tile;
}

VA(0x00470e47, 0x31)
H3_ENUM_RETURN(TTerrainType, int) TTerrainPlacementMapAdapter::getTerrain(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    return layer.getCell(loc.x(), loc.y()).getTerrainType();
}

VA(0x00470e78, 0x34)
int TTerrainPlacementMapAdapter::getFrame(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    return layer.getCell(loc.x(), loc.y()).getTileNum();
}

}

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

VA(0x00471532, 0x30c)
void TMapSpecsGeneralPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_description.Replace("\r\n", "\n");
    if (_m_version > _m_newMap.getVersion())
        _m_newMap.setVersion(EGameVersion(_m_version));
    _m_newMap.setDifficulty(TGameMap::TDifficulty(_m_difficulty));
    _m_newMap.setName(string(_m_name));
    _m_newMap.setDesc(string(_m_description));
    if (_m_limitHeroLevelCheck.GetCheck()) {
        int level = 0;
        CString text;
        _m_heroLevelEdit.GetWindowText(text);
        sscanf(text, "%d", &level);
        _m_newMap.setMaxHeroLevel(clamp(1, level, int(s_kMaxHeroLevel)));
    } else {
        _m_newMap.setMaxHeroLevel(0);
    }
    if (_m_newMap.isTwoLayer()) {
        if (!_m_bTwoLevel)
            _m_newMap.removeSecondLayer();
    } else if (_m_bTwoLevel) {
        _m_newMap.addSecondLayer();
        TTerrainPlacementMapAdapter map(&_m_newMap, true);
        TTerrainPlacementOp op(&map, eTerrainRock, _m_terrainStrength);
        op.paintRectangle(0, 0, _m_newMap.getWidth(), _m_newMap.getHeight());
    }
    _m_bModified = _m_bModified || _m_newMap.getVersion() != _m_oldMap.getVersion()
                   || _m_newMap.getName() != _m_oldMap.getName()
                   || _m_newMap.getDesc() != _m_oldMap.getDesc()
                   || _m_newMap.getDifficulty() != _m_oldMap.getDifficulty()
                   || _m_newMap.getMaxHeroLevel() != _m_oldMap.getMaxHeroLevel()
                   || _m_newMap.isTwoLayer() != _m_oldMap.isTwoLayer();
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
