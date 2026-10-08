// MapSpecsPlayerSpecsPage.h - the map specifications' player page (Loki
// MapSpecsPlayerSpecsPage.cpp): per present player, who may play it
// (human/computer checks), the computer's behaviour and whether a hero is
// generated at a chosen main town. Not a CWnd: three combo item-data tables
// (players, towns, behaviours), the map, the modified flag, the published
// and the edited player infos (_m_intPlayers), the current player, the
// number of human-playable players, the current player's towns
// (_m_aTownRef), which players may generate a hero (_m_abCanGenerateHero)
// and a flag that swallows the toggle signal _setPlayerInfo raises, then the
// vtable pointer (OnOK, OnInitDialog). The names the asserts give are
// _m_playerItemData, _m_intPlayers, _m_curPlayer, _m_numHumanPlayable,
// _m_aTownRef and _m_abCanGenerateHero; the others and the 256-entry size of
// the item-data tables are not proven.
#ifndef HOMM3_EDITOR_MAPSPECSPLAYERSPECSPAGE_H
#define HOMM3_EDITOR_MAPSPECSPLAYERSPECSPAGE_H

#include "editor/stdafx.h"

#include <vector>

#include "editor/GameMap.h"
#include "editor/MapObjectRef.h"
#include "editor/Player.h"

class TMapSpecsPlayerSpecsPage {
public:
    TMapSpecsPlayerSpecsPage(const TGameMap& map);
    ~TMapSpecsPlayerSpecsPage();

    void enableWidgets(bool bEnable);

    virtual void OnOK();
    virtual BOOL OnInitDialog();

    void OnSelChangePlayerCombo();
    void OnGenerateHeroCheck();
    void OnHumanPlayableCheck();
    void OnComputerPlayableCheck();

    const TArray<TPlayerInfo, kNumPlayers>& getPlayers() const { return _m_players; }
    bool wasModified() const { return _m_bModified; }

private:
    void _setPlayerInfo();
    void _retrievePlayerInfo();

    int _m_playerItemData[256];
    int _m_townItemData[256];
    int _m_behaviorItemData[256];
    const TGameMap& _m_map;
    bool _m_bModified;
    TArray<TPlayerInfo, kNumPlayers> _m_players;
    TArray<TPlayerInfo, kNumPlayers> _m_intPlayers;
    TPlayer _m_curPlayer;
    unsigned int _m_numHumanPlayable;
    vector<TMapObjectRef> _m_aTownRef;
    TPlayerMask _m_abCanGenerateHero;
    bool _m_bSettingHumanPlayable;
};

#endif  /* HOMM3_EDITOR_MAPSPECSPLAYERSPECSPAGE_H */
