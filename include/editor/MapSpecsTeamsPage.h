// MapSpecsTeamsPage.h - the teams page of the map specifications sheet
// (MapSpecsTeamsPage.cpp; Loki h3maped object 74): whether the map has
// teams, how many, and a row of team radios per player. The focused row's
// player label draws a focus rectangle. Layout from the image: the team
// number static, the two group boxes and the enable check from 0x8c, the
// DDX team of each player at 0x17c and the number of teams (less two) at
// 0x19c, the eight player rows at 0x1a0 (0x1e4 bytes each), the number of
// teams radios at 0x10c0, then the map before and during the sheet and the
// modified flag.
#ifndef HOMM3_EDITOR_MAPSPECSTEAMSPAGE_H
#define HOMM3_EDITOR_MAPSPECSTEAMSPAGE_H

#include "editor/GameMap.h"
#include "editor/resource.h"

// A static that draws a focus rectangle around itself while shown so
// (RTTI TFocusRectStatic, 0x40 bytes).
class TFocusRectStatic : public CStatic {
public:
    TFocusRectStatic() : _m_bShowFocusRect(false) {}

    void showFocusRect(bool bShow)
    {
        if (_m_bShowFocusRect != bShow) {
            _m_bShowFocusRect = bShow;
            Invalidate();
        }
    }

protected:
    afx_msg void OnPaint();
    DECLARE_MESSAGE_MAP()

private:
    bool _m_bShowFocusRect;
};

class TMapSpecsTeamsPage : public CPropertyPage {
public:
    enum { s_kNumNumTeamsRadios = TTeamInfo::s_kMaxTeams - TTeamInfo::s_kMinTeams + 1 };

    TMapSpecsTeamsPage(const TGameMap& oldMap, TGameMap& newMap);
    virtual ~TMapSpecsTeamsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MAP_SPECS_TEAMS };
    CStatic _m_teamNumberStatic;
    CButton _m_teamAssignmentsGroup;
    CButton _m_numTeamsGroup;
    CButton _m_enableTeamsCheck;
    int _m_player1Team;
    int _m_player2Team;
    int _m_player3Team;
    int _m_player4Team;
    int _m_player5Team;
    int _m_player6Team;
    int _m_player7Team;
    int _m_player8Team;
    int _m_numTeams;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnEnableTeamsCheck();
    afx_msg void OnPlayer1TeamRadio();
    afx_msg void OnPlayer2TeamRadio();
    afx_msg void OnPlayer3TeamRadio();
    afx_msg void OnPlayer4TeamRadio();
    afx_msg void OnPlayer5TeamRadio();
    afx_msg void OnPlayer6TeamRadio();
    afx_msg void OnPlayer7TeamRadio();
    afx_msg void OnPlayer8TeamRadio();
    afx_msg void OnNumTeamsRadio();
    afx_msg void OnSetFocusPlayer1TeamRadio();
    afx_msg void OnSetFocusPlayer2TeamRadio();
    afx_msg void OnSetFocusPlayer3TeamRadio();
    afx_msg void OnSetFocusPlayer4TeamRadio();
    afx_msg void OnSetFocusPlayer5TeamRadio();
    afx_msg void OnSetFocusPlayer6TeamRadio();
    afx_msg void OnSetFocusPlayer7TeamRadio();
    afx_msg void OnSetFocusPlayer8TeamRadio();
    afx_msg void OnKillFocusPlayer1TeamRadio();
    afx_msg void OnKillFocusPlayer2TeamRadio();
    afx_msg void OnKillFocusPlayer3TeamRadio();
    afx_msg void OnKillFocusPlayer4TeamRadio();
    afx_msg void OnKillFocusPlayer5TeamRadio();
    afx_msg void OnKillFocusPlayer6TeamRadio();
    afx_msg void OnKillFocusPlayer7TeamRadio();
    afx_msg void OnKillFocusPlayer8TeamRadio();
    afx_msg void OnPlayer1Static();
    afx_msg void OnPlayer2Static();
    afx_msg void OnPlayer3Static();
    afx_msg void OnPlayer4Static();
    afx_msg void OnPlayer5Static();
    afx_msg void OnPlayer6Static();
    afx_msg void OnPlayer7Static();
    afx_msg void OnPlayer8Static();
    DECLARE_MESSAGE_MAP()

private:
    // One player's row: the label and the radio of each team.
    struct _TPlayerControls {
        TFocusRectStatic m_playerStatic;
        CButton m_aTeamRadios[TTeamInfo::s_kMaxTeams];
    };

    // Each player's DDX team member.
    static int TMapSpecsTeamsPage::* const _s_apPlayerTeam[kNumPlayers];

    void _onPlayerTeamRadio(unsigned int player);
    void _onSetFocusPlayerTeamRadio(unsigned int player);
    void _onKillFocusPlayerTeamRadio(unsigned int player);
    void _onPlayerStatic(unsigned int player);

    _TPlayerControls _m_aPlayerControls[kNumPlayers];
    CButton _m_aNumTeamsRadios[s_kNumNumTeamsRadios];
    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_MAPSPECSTEAMSPAGE_H */
