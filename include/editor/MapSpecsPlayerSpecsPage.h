// MapSpecsPlayerSpecsPage.h - the player page of the map specifications
// sheet (MapSpecsPlayerSpecsPage.cpp; Loki h3maped object 72): per present
// player, who may play it, the computer's behaviour, the main town and
// whether a hero is generated there, and (Shadow of Death maps) the town
// types the player may start with. Customized town types need at least one
// type, so the page has the sheet disable its OK button while none is
// checked. Layout from the image: the controls from 0x8c, the sheet, the
// map before and during the sheet, the modified flag, the edited players at
// 0x2f4, the current player, the number of human-playable players, the
// current player's towns, which players may generate a hero, then the town
// types (customized, the map's default and the shown ones), the number of
// checked types and the nine type checks at 0x3f0 (0x60c bytes, the
// sheet's new).
#ifndef HOMM3_EDITOR_MAPSPECSPLAYERSPECSPAGE_H
#define HOMM3_EDITOR_MAPSPECSPLAYERSPECSPAGE_H

#include <bitset>
#include <vector>

#include "editor/GameMap.h"
#include "editor/MapObjectRef.h"
#include "editor/Player.h"
#include "editor/resource.h"

class TMapSpecsPlayerSpecsPage : public CPropertyPage {
public:
    // The sheet, told when its OK button may be pressed.
    class TParentSheet {
    public:
        virtual void onEnableOK() = 0;
        virtual void onDisableOK() = 0;
    };

    TMapSpecsPlayerSpecsPage(TParentSheet* pParentSheet, const TGameMap& oldMap, TGameMap& newMap);
    virtual ~TMapSpecsPlayerSpecsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MAP_SPECS_PLAYER_SPECS };
    CButton _m_customizeCheck;
    CButton _m_randomTownCheck;
    CButton _m_hasMainTownCheck;
    CComboBox _m_behaviorCombo;
    CStatic _m_behaviorStatic;
    CButton _m_computerPlayableCheck;
    CButton _m_humanPlayableCheck;
    CComboBox _m_mainTownCombo;
    CButton _m_generateHeroCheck;
    CComboBox _m_playerCombo;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnSelChangePlayerCombo();
    afx_msg void OnGenerateHeroCheck();
    afx_msg void OnHumanPlayableCheck();
    afx_msg void OnComputerPlayableCheck();
    afx_msg void OnHasMainTownCheck();
    afx_msg void OnSelChangeMainTownCombo();
    afx_msg void OnCustomizeCheck();
    afx_msg void OnRandomTownCheck();
    afx_msg void OnCastleCheck();
    afx_msg void OnRampartCheck();
    afx_msg void OnTowerCheck();
    afx_msg void OnInfernoCheck();
    afx_msg void OnNecropolisCheck();
    afx_msg void OnDungeonCheck();
    afx_msg void OnStrongholdCheck();
    afx_msg void OnFortressCheck();
    afx_msg void OnConfluxCheck();
    DECLARE_MESSAGE_MAP()

private:
    void _setPlayerInfo();
    void _retrievePlayerInfo();
    void _onTownTypeCheck(unsigned int townType);
    void _enableCustomTownTypes(bool bCustom);

    TParentSheet* _m_pParentSheet;
    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    bool _m_bModified;
    TArray<TPlayerInfo, kNumPlayers> _m_players;
    TPlayer _m_curPlayer;
    unsigned int _m_numHumanPlayable;
    std::vector<TMapObjectRef> _m_aTownRef;
    TPlayerMask _m_abCanGenerateHero;
    bool _m_bCustomTownTypes;
    TPlayerInfo::TTownTypes _m_customTownTypes;
    TPlayerInfo::TTownTypes _m_defaultTownTypes;
    TPlayerInfo::TTownTypes _m_townTypes;
    unsigned int _m_numTownTypes;
    CButton _m_aTownTypeChecks[kNumTownTypes];
};

#endif  /* HOMM3_EDITOR_MAPSPECSPLAYERSPECSPAGE_H */
