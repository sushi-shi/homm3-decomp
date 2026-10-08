// MapSpecsTeamsPage.h - the map specifications' teams page (Loki
// MapSpecsTeamsPage.cpp): the enable_teams_check toggle, six "number of
// teams" radios (two to seven teams) and a row of seven team radios per
// player. Not a CWnd: the eight players' team radio indices, the
// number-of-teams radio index (_m_numTeams, the count less
// TTeamInfo::s_kMinTeams), the map, the edited team info and the modified
// flag, then the vtable pointer (OnOK, OnInitDialog). _s_apPlayerTeam maps a
// player to its radio index member (`this->*_s_apPlayerTeam[ player ]` in
// the asserts). The other member names are not proven.
#ifndef HOMM3_EDITOR_MAPSPECSTEAMSPAGE_H
#define HOMM3_EDITOR_MAPSPECSTEAMSPAGE_H

#include "editor/stdafx.h"

#include "editor/GameMap.h"

class TMapSpecsTeamsPage {
public:
    TMapSpecsTeamsPage(const TGameMap& map);
    ~TMapSpecsTeamsPage();

    void UpdateData(bool bSaveAndValidate);

    virtual void OnOK();
    virtual BOOL OnInitDialog();

    void OnEnableTeamsCheck();
    void OnNumTeamsRadio();
    void OnPlayer1TeamRadio();
    void OnPlayer2TeamRadio();
    void OnPlayer3TeamRadio();
    void OnPlayer4TeamRadio();
    void OnPlayer5TeamRadio();
    void OnPlayer6TeamRadio();
    void OnPlayer7TeamRadio();
    void OnPlayer8TeamRadio();
    void OnSetFocusPlayer1TeamRadio();
    void OnSetFocusPlayer2TeamRadio();
    void OnSetFocusPlayer3TeamRadio();
    void OnSetFocusPlayer4TeamRadio();
    void OnSetFocusPlayer5TeamRadio();
    void OnSetFocusPlayer6TeamRadio();
    void OnSetFocusPlayer7TeamRadio();
    void OnSetFocusPlayer8TeamRadio();
    void OnKillFocusPlayer1TeamRadio();
    void OnKillFocusPlayer2TeamRadio();
    void OnKillFocusPlayer3TeamRadio();
    void OnKillFocusPlayer4TeamRadio();
    void OnKillFocusPlayer5TeamRadio();
    void OnKillFocusPlayer6TeamRadio();
    void OnKillFocusPlayer7TeamRadio();
    void OnKillFocusPlayer8TeamRadio();
    void OnPlayer1Static();
    void OnPlayer2Static();
    void OnPlayer3Static();
    void OnPlayer4Static();
    void OnPlayer5Static();
    void OnPlayer6Static();
    void OnPlayer7Static();
    void OnPlayer8Static();

    const TTeamInfo& getTeamInfo() const { return _m_teamInfo; }
    bool wasModified() const { return _m_bModified; }

private:
    void _onPlayerTeamRadio(TPlayer player);
    void _onSetFocusPlayerTeamRadio(TPlayer player);
    void _onKillFocusPlayerTeamRadio(TPlayer player);
    void _onPlayerStatic(TPlayer player);

    static int TMapSpecsTeamsPage::* const _s_apPlayerTeam[kNumPlayers];

    int _m_player1Team;
    int _m_player2Team;
    int _m_player3Team;
    int _m_player4Team;
    int _m_player5Team;
    int _m_player6Team;
    int _m_player7Team;
    int _m_player8Team;
    int _m_numTeams;
    const TGameMap& _m_map;
    TTeamInfo _m_teamInfo;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_MAPSPECSTEAMSPAGE_H */
