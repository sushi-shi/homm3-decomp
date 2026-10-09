// MapSpecsPlayerSpecsPage.cpp - the player page of the map specifications
// sheet (h3maped 0x472f6a..0x47471d; Loki h3maped object 72). The player
// combo lists the present players; switching players stores the controls
// into the edited player and loads the next. A map keeps at least one
// human-playable player: unchecking the last one re-checks it, or makes the
// first present player human-playable. Before Armageddon's Blade a hero is
// always generated at the main town, and a town with a visiting hero
// cannot be the main town.
#include "editor/stdafx.h"

#include <algorithm>
#include <iterator>

#include "gamecontext.h"
#include "va.h"
#include "videogamestate.h"
#include "editor/GameObject.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsPlayerSpecsPage.h"
#include "editor/Town.h"

VA(0x00472f6a, 0x8)
void TMapSpecsPlayerSpecsPage::OnCastleCheck()
{
    _onTownTypeCheck(TOWN_CASTLE);
}

VA(0x00472f72, 0x8)
void TMapSpecsPlayerSpecsPage::OnRampartCheck()
{
    _onTownTypeCheck(TOWN_RAMPART);
}

VA(0x00472f7a, 0x8)
void TMapSpecsPlayerSpecsPage::OnTowerCheck()
{
    _onTownTypeCheck(TOWN_TOWER);
}

VA(0x00472f82, 0x8)
void TMapSpecsPlayerSpecsPage::OnInfernoCheck()
{
    _onTownTypeCheck(TOWN_INFERNO);
}

VA(0x00472f8a, 0x8)
void TMapSpecsPlayerSpecsPage::OnNecropolisCheck()
{
    _onTownTypeCheck(TOWN_NECROPOLIS);
}

VA(0x00472f92, 0x8)
void TMapSpecsPlayerSpecsPage::OnDungeonCheck()
{
    _onTownTypeCheck(TOWN_DUNGEON);
}

VA(0x00472f9a, 0x8)
void TMapSpecsPlayerSpecsPage::OnStrongholdCheck()
{
    _onTownTypeCheck(TOWN_STRONGHOLD);
}

VA(0x00472fa2, 0x8)
void TMapSpecsPlayerSpecsPage::OnFortressCheck()
{
    _onTownTypeCheck(TOWN_FORTRESS);
}

VA(0x00472faa, 0x8)
void TMapSpecsPlayerSpecsPage::OnConfluxCheck()
{
    _onTownTypeCheck(TOWN_CONFLUX);
}

DATA(0x0053ac08)
static const int g_akTownTypeCheckIDs[kNumTownTypes] = { IDC_CASTLE_CHECK, IDC_RAMPART_CHECK, IDC_TOWER_CHECK, IDC_INFERNO_CHECK, IDC_NECROPOLIS_CHECK, IDC_DUNGEON_CHECK, IDC_STRONGHOLD_CHECK, IDC_FORTRESS_CHECK, IDC_CONFLUX_CHECK };

VA(0x00472fb2, 0x4c)
static const char* getBehaviorTypeName(TPlayerInfo::TBehaviorType behaviorType)
{
    static const char* const akBehaviorTypeName[TPlayerInfo::s_kNumBehaviorTypes] = {
        kRandomStr, kWarriorStr, kBuilderStr, kExplorerStr
    };
    return akBehaviorTypeName[behaviorType];
}

VA(0x00472ffe, 0x1cb)
TMapSpecsPlayerSpecsPage::TMapSpecsPlayerSpecsPage(TParentSheet* pParentSheet, const TGameMap& oldMap,
                                                   TGameMap& newMap)
    : CPropertyPage(TMapSpecsPlayerSpecsPage::IDD),
      _m_pParentSheet(pParentSheet),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kPlayerSpecsPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x004731c9, 0x1c, SCALAR_DELETING_DTOR, TMapSpecsPlayerSpecsPage)

VA(0x004731e5, 0xfa)
TMapSpecsPlayerSpecsPage::~TMapSpecsPlayerSpecsPage()
{
}

VA(0x004732df, 0x4f5)
void TMapSpecsPlayerSpecsPage::_setPlayerInfo()
{
    _m_mainTownCombo.ResetContent();
    _m_aTownRef.clear();
    const set<TMapObjectRef>& townRefs = _m_newMap.getPlayerTownRefs(_m_curPlayer);
    _m_aTownRef.reserve(townRefs.size());
    copy(townRefs.begin(), townRefs.end(), back_inserter(_m_aTownRef));
    EGameVersion version = _m_newMap.getVersion();
    for (unsigned int i = 0; i < _m_aTownRef.size(); i++) {
        const TMapObjectRef& ref = _m_aTownRef[i];
        const TTown* pTown = dynamic_cast<const TTown*>(&_m_newMap.getLayer(ref.getBSecondLayer()).getObject(ref.getObjectID()));
        if (pTown->getPVisitingHero() == NULL || version >= GAME_VERSION_AB) {
            TTilePoint loc = _m_newMap.getLayer(ref.getBSecondLayer()).getObjectLoc(ref.getObjectID())
                             - pTown->getTriggerLoc();
            CString name;
            name.Format(kObjectAtLocationFmtStr, pTown->getTownTypeTraits().m_pName, loc.x(), loc.y(),
                        ref.getBSecondLayer());
            int index = _m_mainTownCombo.AddString(name);
            _m_mainTownCombo.SetItemData(index, i);
        }
    }

    const TMapObjectRef& mainTownRef = _m_players[_m_curPlayer].getMainTownRef();
    if (_m_players[_m_curPlayer].getBHasMainTown()) {
        _m_hasMainTownCheck.EnableWindow(TRUE);
        _m_hasMainTownCheck.SetCheck(1);
        _m_mainTownCombo.EnableWindow(TRUE);
        int index;
        for (index = 0; _m_aTownRef[_m_mainTownCombo.GetItemData(index)] != mainTownRef; index++)
            ;
        _m_mainTownCombo.SetCurSel(index);
        const TTown* pMainTown = dynamic_cast<const TTown*>(
            &_m_newMap.getLayer(mainTownRef.getBSecondLayer()).getObject(mainTownRef.getObjectID()));
        if (_m_abCanGenerateHero[_m_curPlayer] && pMainTown->getPVisitingHero() == NULL) {
            _m_generateHeroCheck.EnableWindow(TRUE);
            _m_generateHeroCheck.SetCheck(_m_players[_m_curPlayer].getBGenerateHero());
            if (version < GAME_VERSION_AB)
                _m_generateHeroCheck.EnableWindow(FALSE);
        } else {
            _m_generateHeroCheck.EnableWindow(FALSE);
            _m_generateHeroCheck.SetCheck(0);
        }
    } else {
        _m_hasMainTownCheck.EnableWindow(_m_mainTownCombo.GetCount() > 0);
        _m_hasMainTownCheck.SetCheck(0);
        _m_mainTownCombo.SetCurSel(-1);
        _m_mainTownCombo.EnableWindow(FALSE);
        _m_generateHeroCheck.EnableWindow(FALSE);
        _m_generateHeroCheck.SetCheck(0);
    }

    _m_humanPlayableCheck.SetCheck(_m_players[_m_curPlayer].getBHumanPlayable());
    _m_computerPlayableCheck.SetCheck(BST_INDETERMINATE);
    _m_behaviorStatic.EnableWindow(TRUE);
    _m_behaviorCombo.EnableWindow(TRUE);
    int index;
    for (index = 0; _m_behaviorCombo.GetItemData(index) != _m_players[_m_curPlayer].getBehaviorType(); index++)
        ;
    _m_behaviorCombo.SetCurSel(index);

    _m_customTownTypes = _m_players[_m_curPlayer].getTownTypes();
    _m_defaultTownTypes = _m_newMap.getDefaultTownTypes(_m_curPlayer);
    _m_customizeCheck.SetCheck(_m_players[_m_curPlayer].getBCustomTownTypes());
    _enableCustomTownTypes(_m_players[_m_curPlayer].getBCustomTownTypes());
    if (version < GAME_VERSION_SOD)
        _m_customizeCheck.EnableWindow(FALSE);
}

VA(0x004737d4, 0x131)
void TMapSpecsPlayerSpecsPage::_retrievePlayerInfo()
{
    TPlayerInfo& playerInfo = _m_players[_m_curPlayer];
    playerInfo.setBHumanPlayable(_m_humanPlayableCheck.GetCheck() != 0);
    if (playerInfo.getBComputerPlayable())
        playerInfo.setBehaviorType(TPlayerInfo::TBehaviorType(_m_behaviorCombo.GetItemData(_m_behaviorCombo.GetCurSel())));
    else
        playerInfo.setBehaviorType(TPlayerInfo::TBehaviorType(0));
    if (_m_hasMainTownCheck.GetCheck()) {
        playerInfo.setMainTown(_m_aTownRef[_m_mainTownCombo.GetItemData(_m_mainTownCombo.GetCurSel())]);
        playerInfo.setBGenerateHero(_m_generateHeroCheck.GetCheck() != 0);
    } else if (playerInfo.getBHasMainTown()) {
        playerInfo.clearMainTown();
    }
    playerInfo.setBCustomTownTypes(_m_bCustomTownTypes);
    if (_m_bCustomTownTypes)
        _m_customTownTypes = _m_townTypes;
    playerInfo.setTownTypes(_m_customTownTypes);
}

VA(0x00473905, 0x9d)
void TMapSpecsPlayerSpecsPage::_onTownTypeCheck(unsigned int townType)
{
    if (_m_aTownTypeChecks[townType].GetCheck()) {
        if (!_m_townTypes.m_mask[townType]) {
            if (_m_numTownTypes++ == 0)
                _m_pParentSheet->onEnableOK();
            _m_townTypes.m_mask.set(townType, true);
        }
    } else {
        if (_m_townTypes.m_mask[townType]) {
            if (--_m_numTownTypes == 0)
                _m_pParentSheet->onDisableOK();
            _m_townTypes.m_mask.set(townType, false);
        }
    }
}

VA(0x004739a2, 0x12f)
void TMapSpecsPlayerSpecsPage::_enableCustomTownTypes(bool bCustom)
{
    _m_bCustomTownTypes = bCustom;
    _m_townTypes = bCustom ? _m_customTownTypes : _m_defaultTownTypes;
    _m_randomTownCheck.EnableWindow(bCustom);
    int townType;
    for (townType = 0; townType < kNumTownTypes; townType++)
        _m_aTownTypeChecks[townType].EnableWindow(_m_bCustomTownTypes);
    if (_m_townTypes.m_bRandom) {
        _m_randomTownCheck.SetCheck(1);
        for (townType = 0; townType < kNumTownTypes; townType++) {
            _m_townTypes.m_mask.set(townType, true);
            _m_aTownTypeChecks[townType].SetCheck(1);
            if (_m_bCustomTownTypes)
                _m_aTownTypeChecks[townType].EnableWindow(FALSE);
        }
    } else {
        _m_randomTownCheck.SetCheck(0);
        for (townType = 0; townType < kNumTownTypes; townType++)
            _m_aTownTypeChecks[townType].SetCheck(_m_townTypes.m_mask[townType]);
    }
    _m_numTownTypes = _m_townTypes.m_mask.count();
}

VA(0x00473ad1, 0xe5)
void TMapSpecsPlayerSpecsPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CUSTOMIZE_CHECK, _m_customizeCheck);
    DDX_Control(pDX, IDC_RANDOM_TOWN_CHECK, _m_randomTownCheck);
    DDX_Control(pDX, IDC_HAS_MAIN_TOWN_CHECK, _m_hasMainTownCheck);
    DDX_Control(pDX, IDC_BEHAVIOR_COMBO, _m_behaviorCombo);
    DDX_Control(pDX, IDC_BEHAVIOR_STATIC, _m_behaviorStatic);
    DDX_Control(pDX, IDC_COMPUTER_PLAYABLE_CHECK, _m_computerPlayableCheck);
    DDX_Control(pDX, IDC_HUMAN_PLAYABLE_CHECK, _m_humanPlayableCheck);
    DDX_Control(pDX, IDC_MAIN_TOWN_COMBO, _m_mainTownCombo);
    DDX_Control(pDX, IDC_GENERATE_HERO_CHECK, _m_generateHeroCheck);
    DDX_Control(pDX, IDC_PLAYER_COMBO, _m_playerCombo);
    for (int townType = 0; townType < kNumTownTypes; townType++)
        DDX_Control(pDX, g_akTownTypeCheckIDs[townType], _m_aTownTypeChecks[townType]);
}

VA(0x00473bb6, 0x6)
BEGIN_MESSAGE_MAP(TMapSpecsPlayerSpecsPage, CPropertyPage)
    ON_CBN_SELCHANGE(IDC_PLAYER_COMBO, OnSelChangePlayerCombo)
    ON_BN_CLICKED(IDC_GENERATE_HERO_CHECK, OnGenerateHeroCheck)
    ON_BN_CLICKED(IDC_HUMAN_PLAYABLE_CHECK, OnHumanPlayableCheck)
    ON_BN_CLICKED(IDC_COMPUTER_PLAYABLE_CHECK, OnComputerPlayableCheck)
    ON_BN_CLICKED(IDC_HAS_MAIN_TOWN_CHECK, OnHasMainTownCheck)
    ON_CBN_SELCHANGE(IDC_MAIN_TOWN_COMBO, OnSelChangeMainTownCombo)
    ON_BN_CLICKED(IDC_CUSTOMIZE_CHECK, OnCustomizeCheck)
    ON_BN_CLICKED(IDC_RANDOM_TOWN_CHECK, OnRandomTownCheck)
    ON_BN_CLICKED(IDC_CASTLE_CHECK, OnCastleCheck)
    ON_BN_CLICKED(IDC_RAMPART_CHECK, OnRampartCheck)
    ON_BN_CLICKED(IDC_TOWER_CHECK, OnTowerCheck)
    ON_BN_CLICKED(IDC_INFERNO_CHECK, OnInfernoCheck)
    ON_BN_CLICKED(IDC_NECROPOLIS_CHECK, OnNecropolisCheck)
    ON_BN_CLICKED(IDC_DUNGEON_CHECK, OnDungeonCheck)
    ON_BN_CLICKED(IDC_STRONGHOLD_CHECK, OnStrongholdCheck)
    ON_BN_CLICKED(IDC_FORTRESS_CHECK, OnFortressCheck)
    ON_BN_CLICKED(IDC_CONFLUX_CHECK, OnConfluxCheck)
END_MESSAGE_MAP()

VA(0x00473bbc, 0x3e4)
BOOL TMapSpecsPlayerSpecsPage::OnInitDialog()
{
    GetDlgItem(IDC_PLAYER_STATIC)->SetWindowText(SMapSpecsPlayerSpecsPageText::kPlayerStaticStr);
    GetDlgItem(IDC_HAS_MAIN_TOWN_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kHasMainTownCheckStr);
    GetDlgItem(IDC_GENERATE_HERO_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kGenerateHeroCheckStr);
    GetDlgItem(IDC_PLAYABILITY_STATIC)->SetWindowText(SMapSpecsPlayerSpecsPageText::kPlayabilityStaticStr);
    GetDlgItem(IDC_HUMAN_PLAYABLE_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kHumanPlayableCheckStr);
    GetDlgItem(IDC_COMPUTER_PLAYABLE_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kComputerPlayableCheckStr);
    GetDlgItem(IDC_BEHAVIOR_STATIC)->SetWindowText(SMapSpecsPlayerSpecsPageText::kBehaviorStaticStr);
    GetDlgItem(IDC_ALLOWED_ALIGNMENTS_STATIC)->SetWindowText(SMapSpecsPlayerSpecsPageText::kAllowedAlignmentsStaticStr);
    GetDlgItem(IDC_CUSTOMIZE_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kCustomizeCheckStr);
    GetDlgItem(IDC_RANDOM_TOWN_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kRandomTownCheckStr);
    GetDlgItem(IDC_CASTLE_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kCastleCheckStr);
    GetDlgItem(IDC_RAMPART_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kRampartCheckStr);
    GetDlgItem(IDC_TOWER_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kTowerCheckStr);
    GetDlgItem(IDC_INFERNO_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kInfernoCheckStr);
    GetDlgItem(IDC_NECROPOLIS_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kNecropolisCheckStr);
    GetDlgItem(IDC_DUNGEON_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kDungeonCheckStr);
    GetDlgItem(IDC_STRONGHOLD_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kStrongholdCheckStr);
    GetDlgItem(IDC_FORTRESS_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kFortressCheckStr);
    GetDlgItem(IDC_CONFLUX_CHECK)->SetWindowText(SMapSpecsPlayerSpecsPageText::kConfluxCheckStr);
    if (g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH)
        GetDlgItem(IDC_CONFLUX_CHECK)->ShowWindow(SW_HIDE);

    _m_bModified = false;
    _m_players = _m_newMap.getPlayers();
    unsigned int player = 0;
    while (player < kNumPlayers && !_m_players[player].getBPresent())
        player++;
    if (player < kNumPlayers) {
        _m_curPlayer = TPlayer(player);
        _m_numHumanPlayable = 0;
        _m_abCanGenerateHero = _m_newMap.getAvailableHeroOwnersMask();
        for (player = 0; player < kNumPlayers; player++) {
            if (_m_players[player].getBHumanPlayable())
                _m_numHumanPlayable++;
            if (_m_players[player].getBGenerateHero())
                _m_abCanGenerateHero.set(player, true);
        }
    } else
        _m_curPlayer = ePlayerNone;

    CPropertyPage::OnInitDialog();

    if (_m_curPlayer != ePlayerNone) {
        for (player = 0; player < kNumPlayers; player++) {
            if (_m_players[player].getBPresent()) {
                int index = _m_playerCombo.AddString(akPlayerTraits[player].m_pName);
                _m_playerCombo.SetItemData(index, player);
            }
        }
        int index;
        for (index = 0; _m_playerCombo.GetItemData(index) != _m_curPlayer; index++)
            ;
        _m_playerCombo.SetCurSel(index);
        for (unsigned int behaviorType = 0; behaviorType < TPlayerInfo::s_kNumBehaviorTypes; behaviorType++) {
            index = _m_behaviorCombo.AddString(getBehaviorTypeName(TPlayerInfo::TBehaviorType(behaviorType)));
            _m_behaviorCombo.SetItemData(index, behaviorType);
        }
        _setPlayerInfo();
    } else {
        _m_playerCombo.SetCurSel(_m_playerCombo.AddString(kNoPlayersOnMapStr));
        for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd != NULL; pWnd = pWnd->GetWindow(GW_HWNDNEXT))
            pWnd->EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x00473fa0, 0x71)
void TMapSpecsPlayerSpecsPage::OnOK()
{
    CPropertyPage::OnOK();
    if (_m_curPlayer != ePlayerNone) {
        _retrievePlayerInfo();
        _m_newMap.setPlayers(_m_players);
    }
    _m_bModified = _m_bModified || !(_m_newMap.getPlayers() == _m_oldMap.getPlayers());
}

VA(0x00474011, 0x40)
void TMapSpecsPlayerSpecsPage::OnSelChangePlayerCombo()
{
    _retrievePlayerInfo();
    _m_curPlayer = TPlayer(_m_playerCombo.GetItemData(_m_playerCombo.GetCurSel()));
    _setPlayerInfo();
}

void TMapSpecsPlayerSpecsPage::OnGenerateHeroCheck()
{
}

VA(0x00474051, 0xc4)
void TMapSpecsPlayerSpecsPage::OnHasMainTownCheck()
{
    if (_m_hasMainTownCheck.GetCheck()) {
        _m_mainTownCombo.EnableWindow(TRUE);
        _m_mainTownCombo.SetCurSel(0);
        OnSelChangeMainTownCombo();
        if (_m_newMap.getVersion() < GAME_VERSION_AB)
            _m_generateHeroCheck.SetCheck(1);
    } else {
        if (_m_abCanGenerateHero[_m_curPlayer]) {
            _m_generateHeroCheck.SetCheck(0);
            _m_generateHeroCheck.EnableWindow(FALSE);
        }
        _m_mainTownCombo.SetCurSel(-1);
        _m_mainTownCombo.EnableWindow(FALSE);
    }
}

VA(0x00474115, 0xd5)
void TMapSpecsPlayerSpecsPage::OnSelChangeMainTownCombo()
{
    if (_m_newMap.getVersion() >= GAME_VERSION_AB && _m_abCanGenerateHero[_m_curPlayer]) {
        const TMapObjectRef& ref = _m_aTownRef[_m_mainTownCombo.GetItemData(_m_mainTownCombo.GetCurSel())];
        const TTown* pTown = dynamic_cast<const TTown*>(&_m_newMap.getLayer(ref.getBSecondLayer()).getObject(ref.getObjectID()));
        if (pTown->getPVisitingHero() != NULL) {
            _m_generateHeroCheck.SetCheck(0);
            _m_generateHeroCheck.EnableWindow(FALSE);
        } else
            _m_generateHeroCheck.EnableWindow(TRUE);
    }
}

VA(0x004741ea, 0xb3)
void TMapSpecsPlayerSpecsPage::OnHumanPlayableCheck()
{
    if (!_m_humanPlayableCheck.GetCheck()) {
        if (--_m_numHumanPlayable == 0) {
            unsigned int player = 0;
            while (!_m_players[player].getBPresent())
                player++;
            if (player == _m_curPlayer)
                _m_humanPlayableCheck.SetCheck(1);
            else
                _m_players[player].setBHumanPlayable(true);
            _m_numHumanPlayable++;
        }
        if (!_m_humanPlayableCheck.GetCheck() && !_m_computerPlayableCheck.GetCheck()) {
            _m_computerPlayableCheck.SetCheck(1);
            OnComputerPlayableCheck();
        }
    } else
        _m_numHumanPlayable++;
}

VA(0x0047429d, 0xc8)
void TMapSpecsPlayerSpecsPage::OnComputerPlayableCheck()
{
    if (!_m_computerPlayableCheck.GetCheck()) {
        if (!_m_humanPlayableCheck.GetCheck()) {
            _m_humanPlayableCheck.SetCheck(1);
            OnHumanPlayableCheck();
        }
        _m_behaviorStatic.EnableWindow(FALSE);
        _m_behaviorCombo.SetCurSel(-1);
        _m_behaviorCombo.EnableWindow(FALSE);
    } else {
        _m_behaviorStatic.EnableWindow(TRUE);
        _m_behaviorCombo.EnableWindow(TRUE);
        int index;
        for (index = 0; _m_behaviorCombo.GetItemData(index) != 0; index++)
            ;
        _m_behaviorCombo.SetCurSel(index);
    }
}

VA(0x00474365, 0x68)
void TMapSpecsPlayerSpecsPage::OnCustomizeCheck()
{
    if (_m_customizeCheck.GetCheck()) {
        if (!_m_bCustomTownTypes) {
            _enableCustomTownTypes(true);
            if (_m_numTownTypes == 0)
                _m_pParentSheet->onDisableOK();
        }
    } else if (_m_bCustomTownTypes) {
        if (_m_numTownTypes == 0)
            _m_pParentSheet->onEnableOK();
        _enableCustomTownTypes(false);
    }
}

VA(0x004743cd, 0xb8)
void TMapSpecsPlayerSpecsPage::OnRandomTownCheck()
{
    int townType;
    if (_m_randomTownCheck.GetCheck()) {
        if (!_m_townTypes.m_bRandom) {
            if (_m_numTownTypes == 0)
                _m_pParentSheet->onEnableOK();
            for (townType = 0; townType < kNumTownTypes; townType++) {
                _m_townTypes.m_mask.set(townType, true);
                _m_aTownTypeChecks[townType].SetCheck(1);
                _m_aTownTypeChecks[townType].EnableWindow(FALSE);
            }
            _m_numTownTypes = kNumTownTypes;
            _m_townTypes.m_bRandom = true;
        }
    } else if (_m_townTypes.m_bRandom) {
        for (townType = 0; townType < kNumTownTypes; townType++)
            _m_aTownTypeChecks[townType].EnableWindow(TRUE);
        _m_townTypes.m_bRandom = false;
    }
}
