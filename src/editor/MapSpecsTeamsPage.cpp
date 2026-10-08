// MapSpecsTeamsPage.cpp - Loki h3maped object 74: the teams page of the map
// specifications sheet. Enabling teams or changing their number deals the
// present players round-robin into the teams (a team of n players for each
// of the remaining slots, rounded up); moving the last player out of a team
// moves a player of the largest team into it. Two of the loops pass the
// number-of-teams loop's index instead of their own team index to
// playerTeamRadio, as Loki did. The assert lines come from the retail
// immediates.
#include "editor/stdafx.h"

#include <stdio.h>
#include <algorithm>

namespace {
#include <gtk/gtk.h>
}

#include "editor/MapSpecsTeamsPage.h"

int TMapSpecsTeamsPage::* const TMapSpecsTeamsPage::_s_apPlayerTeam[kNumPlayers] = {
    &TMapSpecsTeamsPage::_m_player1Team,
    &TMapSpecsTeamsPage::_m_player2Team,
    &TMapSpecsTeamsPage::_m_player3Team,
    &TMapSpecsTeamsPage::_m_player4Team,
    &TMapSpecsTeamsPage::_m_player5Team,
    &TMapSpecsTeamsPage::_m_player6Team,
    &TMapSpecsTeamsPage::_m_player7Team,
    &TMapSpecsTeamsPage::_m_player8Team,
};

static GtkWidget* numTeamsRadio(int index)
{
    char name[100];
    snprintf(name, 100, "numteams_radio_%d", index);
    name[99] = '\0';
    GtkWidget* pWidget = _widget(name);
    return pWidget;
}

static GtkWidget* playerTeamRadio(int player, int team)
{
    char name[100];
    snprintf(name, 100, "player%d_radio_%d", player + 1, team);
    name[99] = '\0';
    GtkWidget* pWidget = _widget(name);
    return pWidget;
}

static int getActiveNumTeamsButton()
{
    GtkToggleButton* pButton;
    for (int i = 0; i < 6; i++) {
        pButton = GTK_TOGGLE_BUTTON(numTeamsRadio(i));
        if (gtk_toggle_button_get_active(pButton) == TRUE)
            return i;
    }
#line 116
    assert(0);
    return -1;
}

static int getActivePlayerTeamButton(int player)
{
    GtkToggleButton* pButton;
    for (int i = 0; i < 7; i++) {
        pButton = GTK_TOGGLE_BUTTON(playerTeamRadio(player, i));
        if (gtk_toggle_button_get_active(pButton) == TRUE)
            return i;
    }
#line 131
    assert(0);
    return -1;
}

void TMapSpecsTeamsPage::UpdateData(bool bSaveAndValidate)
{
    _m_numTeams = getActiveNumTeamsButton();
    _m_player1Team = getActivePlayerTeamButton(0);
    _m_player2Team = getActivePlayerTeamButton(1);
    _m_player3Team = getActivePlayerTeamButton(2);
    _m_player4Team = getActivePlayerTeamButton(3);
    _m_player5Team = getActivePlayerTeamButton(4);
    _m_player6Team = getActivePlayerTeamButton(5);
    _m_player7Team = getActivePlayerTeamButton(6);
    _m_player8Team = getActivePlayerTeamButton(7);
}

TMapSpecsTeamsPage::TMapSpecsTeamsPage(const TGameMap& map)
    : _m_map(map)
{
    _m_bModified = false;
    _m_player1Team = -1;
    _m_player2Team = -1;
    _m_player3Team = -1;
    _m_player4Team = -1;
    _m_player5Team = -1;
    _m_player6Team = -1;
    _m_player7Team = -1;
    _m_player8Team = -1;
    _m_numTeams = -1;
}

TMapSpecsTeamsPage::~TMapSpecsTeamsPage()
{
}

void TMapSpecsTeamsPage::_onPlayerTeamRadio(TPlayer player)
{
#line 199
    assert(player >= 0 && player < kNumPlayers);
#line 201
    assert(_m_map.getPlayers()[ player ].getBPresent());
    GtkToggleButton* gtb = GTK_TOGGLE_BUTTON(_widget("enable_teams_check"));
#line 204
    assert(gtk_toggle_button_get_active(gtb) != FALSE);
    unsigned int oldTeam = this->*_s_apPlayerTeam[player];
#line 207
    assert(oldTeam < _m_numTeams + TTeamInfo::s_kMinTeams);
    UpdateData(false);
    unsigned int newTeam = this->*_s_apPlayerTeam[player];
#line 212
    assert(newTeam < _m_numTeams + TTeamInfo::s_kMinTeams);
    if (newTeam != oldTeam) {
        const TArray<TPlayerInfo, kNumPlayers>& players = _m_map.getPlayers();
        bool bOldTeamUsed = false;
        unsigned int playerNum;
        for (playerNum = 0; playerNum < kNumPlayers; playerNum++) {
            if (players[playerNum].getBPresent() && this->*_s_apPlayerTeam[playerNum] == oldTeam) {
                bOldTeamUsed = true;
                break;
            }
        }
        if (!bOldTeamUsed) {
            unsigned int aTeamSize[TTeamInfo::s_kMaxTeams];
            fill_n(aTeamSize, _m_numTeams + TTeamInfo::s_kMinTeams, 0u);
            for (playerNum = 0; playerNum < kNumPlayers; playerNum++) {
                if (players[playerNum].getBPresent()) {
                    unsigned int playerTeam = this->*_s_apPlayerTeam[playerNum];
#line 238
                    assert(playerTeam < _m_numTeams + TTeamInfo::s_kMinTeams);
                    aTeamSize[playerTeam]++;
                }
            }
            unsigned int largestTeamSize = aTeamSize[0];
            for (unsigned int team = 1; team < _m_numTeams + TTeamInfo::s_kMinTeams; team++)
                if (aTeamSize[team] > largestTeamSize)
                    largestTeamSize = aTeamSize[team];
            playerNum = player + 1;
            if (playerNum == kNumPlayers)
                playerNum = 0;
            while (!(players[playerNum].getBPresent()
                     && aTeamSize[this->*_s_apPlayerTeam[playerNum]] == largestTeamSize)) {
                playerNum++;
                if (playerNum == kNumPlayers)
                    playerNum = 0;
#line 259
                assert(playerNum != player);
            }
            this->*_s_apPlayerTeam[playerNum] = oldTeam;
            UpdateData(false);
        }
    }
}

void TMapSpecsTeamsPage::_onSetFocusPlayerTeamRadio(TPlayer player)
{
}

void TMapSpecsTeamsPage::_onKillFocusPlayerTeamRadio(TPlayer player)
{
}

void TMapSpecsTeamsPage::_onPlayerStatic(TPlayer player)
{
}

BOOL TMapSpecsTeamsPage::OnInitDialog()
{
    _m_bModified = false;
    _m_teamInfo = _m_map.getTeamInfo();
    if (_m_teamInfo.getBHasTeams()) {
        _m_numTeams = _m_teamInfo.getNumTeams() - TTeamInfo::s_kMinTeams;
        for (unsigned int player = 0; player < kNumPlayers; player++) {
            const TArray<TPlayerInfo, kNumPlayers>& players = _m_map.getPlayers();
            if (players[player].getBPresent()) {
                this->*_s_apPlayerTeam[player] = _m_teamInfo.getPlayerTeam(TPlayer(player));
#line 663
                assert(this->*_s_apPlayerTeam[ player ] < _m_numTeams + TTeamInfo::s_kMinTeams);
            } else
                this->*_s_apPlayerTeam[player] = -1;
        }
    } else {
        _m_numTeams = -1;
        for (unsigned int player = 0; player < kNumPlayers; player++)
            this->*_s_apPlayerTeam[player] = -1;
    }

    GtkToggleButton* gtb = GTK_TOGGLE_BUTTON(_widget("enable_teams_check"));
    gtk_toggle_button_set_active(gtb, _m_teamInfo.getBHasTeams());
    gtk_widget_set_sensitive(GTK_WIDGET(gtb), TRUE);
    if (!_m_teamInfo.getBHasTeams()) {
        for (int i = 0; i < 6; i++)
            gtk_widget_set_sensitive(numTeamsRadio(i), FALSE);
        for (int player = 0; player < kNumPlayers; player++)
            for (int team = 0; team < 7; team++)
                gtk_widget_set_sensitive(playerTeamRadio(player, team), FALSE);
        if (_m_map.getNumPlayableSlots() <= 2)
            gtk_widget_set_sensitive(GTK_WIDGET(gtb), FALSE);
    } else {
#line 711
        assert(_m_numTeams >= 0 && _m_numTeams < 7);
        assert(_m_numTeams < _m_map.getNumPlayableSlots() - TTeamInfo::s_kMinTeams);
        for (unsigned int i = _m_map.getNumPlayableSlots() - TTeamInfo::s_kMinTeams; i < 6; i++)
            gtk_widget_set_sensitive(numTeamsRadio(i), FALSE);
        for (unsigned int player = 0; player < kNumPlayers; player++) {
            gtk_widget_set_sensitive(playerTeamRadio(player, i), FALSE);
            const TArray<TPlayerInfo, kNumPlayers>& players = _m_map.getPlayers();
            if (players[player].getBPresent()) {
#line 729
                assert(this->*_s_apPlayerTeam[ player ] < _m_numTeams + TTeamInfo::s_kMinTeams);
                for (unsigned int team = _m_numTeams + TTeamInfo::s_kMinTeams; team < 7; team++)
                    gtk_widget_set_sensitive(playerTeamRadio(player, i), FALSE);
            } else {
                for (unsigned int team = 0; team < 7; team++)
                    gtk_widget_set_sensitive(playerTeamRadio(player, i), FALSE);
            }
        }
    }
    return true;
}

void TMapSpecsTeamsPage::OnOK()
{
    const TArray<TPlayerInfo, kNumPlayers>& players = _m_map.getPlayers();
    GtkToggleButton* gtb = GTK_TOGGLE_BUTTON(_widget("enable_teams_check"));
    _m_teamInfo.setBHasTeams(gtk_toggle_button_get_active(gtb));
    if (_m_teamInfo.getBHasTeams()) {
        _m_teamInfo.setNumTeams(_m_numTeams + TTeamInfo::s_kMinTeams);
        for (unsigned int player = 0; player < kNumPlayers; player++) {
            if (players[player].getBPresent())
                _m_teamInfo.setPlayerTeam(TPlayer(player), this->*_s_apPlayerTeam[player]);
            else
                _m_teamInfo.setPlayerTeam(TPlayer(player), 0);
        }
    }
    _m_bModified = _m_bModified || _m_teamInfo != _m_map.getTeamInfo();
}

void TMapSpecsTeamsPage::OnEnableTeamsCheck()
{
    if (_m_map.getNumPlayableSlots() <= 2)
        return;
    GtkToggleButton* gtb = GTK_TOGGLE_BUTTON(_widget("enable_teams_check"));
    if (gtk_toggle_button_get_active(gtb)) {
        _m_numTeams = 0;
        const TArray<TPlayerInfo, kNumPlayers>& players = _m_map.getPlayers();
        unsigned int playerNum = 0;
        unsigned int team = 0;
        unsigned int numTeamsLeft = TTeamInfo::s_kMinTeams;
        unsigned int numPlayersLeft = _m_map.getNumPlayableSlots();
        while (numTeamsLeft != 0) {
            unsigned int teamSize = (numPlayersLeft + numTeamsLeft - 1) / numTeamsLeft;
            numPlayersLeft -= teamSize;
            numTeamsLeft--;
            while (teamSize != 0) {
#line 810
                assert(playerNum < kNumPlayers);
                if (players[playerNum].getBPresent()) {
                    this->*_s_apPlayerTeam[playerNum] = team;
                    teamSize--;
                }
                playerNum++;
            }
            team++;
        }
        for (unsigned int i = 0; i < _m_map.getNumPlayableSlots() - TTeamInfo::s_kMinTeams; i++)
            gtk_widget_set_sensitive(numTeamsRadio(i), TRUE);
        for (unsigned int player = 0; player < kNumPlayers; player++) {
            const TArray<TPlayerInfo, kNumPlayers>& players = _m_map.getPlayers();
            if (players[player].getBPresent()) {
#line 842
                assert(this->*_s_apPlayerTeam[ player ] < TTeamInfo::s_kMinTeams);
                for (unsigned int team = 0; team < TTeamInfo::s_kMinTeams; team++)
                    gtk_widget_set_sensitive(playerTeamRadio(player, i), TRUE);
            }
        }
        UpdateData(false);
    } else {
        _m_numTeams = -1;
        for (unsigned int player = 0; player < kNumPlayers; player++)
            this->*_s_apPlayerTeam[player] = -1;
        UpdateData(false);
        for (int i = 0; i < 6; i++)
            gtk_widget_set_sensitive(numTeamsRadio(i), FALSE);
        for (int player = 0; player < kNumPlayers; player++)
            for (int team = 0; team < 7; team++)
                gtk_widget_set_sensitive(playerTeamRadio(player, team), FALSE);
    }
}

void TMapSpecsTeamsPage::OnNumTeamsRadio()
{
    GtkToggleButton* gtb = GTK_TOGGLE_BUTTON(_widget("enable_teams_check"));
#line 889
    assert(gtk_toggle_button_get_active(gtb));
    int oldNumTeams = _m_numTeams;
    UpdateData(false);
    if (_m_numTeams != oldNumTeams) {
#line 895
        assert(_m_numTeams >= 0 && _m_numTeams < _m_map.getNumPlayableSlots() - TTeamInfo::s_kMinTeams);
        const TArray<TPlayerInfo, kNumPlayers>& players = _m_map.getPlayers();
        unsigned int playerNum = 0;
        unsigned int team = 0;
        unsigned int numTeamsLeft = _m_numTeams + TTeamInfo::s_kMinTeams;
        unsigned int numPlayersLeft = _m_map.getNumPlayableSlots();
        while (numTeamsLeft != 0) {
            unsigned int teamSize = (numPlayersLeft + numTeamsLeft - 1) / numTeamsLeft;
            numPlayersLeft -= teamSize;
            numTeamsLeft--;
            while (teamSize != 0) {
#line 910
                assert(playerNum < kNumPlayers);
                if (players[playerNum].getBPresent()) {
                    this->*_s_apPlayerTeam[playerNum] = team;
                    unsigned int radio;
                    for (radio = TTeamInfo::s_kMinTeams; radio < _m_numTeams + TTeamInfo::s_kMinTeams; radio++)
                        gtk_widget_set_sensitive(playerTeamRadio(playerNum, radio), TRUE);
                    for (; radio < 7; radio++)
                        gtk_widget_set_sensitive(playerTeamRadio(playerNum, radio), FALSE);
                    teamSize--;
                }
                playerNum++;
            }
            team++;
        }
        UpdateData(false);
    }
}

void TMapSpecsTeamsPage::OnPlayer1TeamRadio() { _onPlayerTeamRadio(TPlayer(0)); }
void TMapSpecsTeamsPage::OnPlayer2TeamRadio() { _onPlayerTeamRadio(TPlayer(1)); }
void TMapSpecsTeamsPage::OnPlayer3TeamRadio() { _onPlayerTeamRadio(TPlayer(2)); }
void TMapSpecsTeamsPage::OnPlayer4TeamRadio() { _onPlayerTeamRadio(TPlayer(3)); }
void TMapSpecsTeamsPage::OnPlayer5TeamRadio() { _onPlayerTeamRadio(TPlayer(4)); }
void TMapSpecsTeamsPage::OnPlayer6TeamRadio() { _onPlayerTeamRadio(TPlayer(5)); }
void TMapSpecsTeamsPage::OnPlayer7TeamRadio() { _onPlayerTeamRadio(TPlayer(6)); }
void TMapSpecsTeamsPage::OnPlayer8TeamRadio() { _onPlayerTeamRadio(TPlayer(7)); }
void TMapSpecsTeamsPage::OnSetFocusPlayer1TeamRadio() { _onSetFocusPlayerTeamRadio(TPlayer(0)); }
void TMapSpecsTeamsPage::OnSetFocusPlayer2TeamRadio() { _onSetFocusPlayerTeamRadio(TPlayer(1)); }
void TMapSpecsTeamsPage::OnSetFocusPlayer3TeamRadio() { _onSetFocusPlayerTeamRadio(TPlayer(2)); }
void TMapSpecsTeamsPage::OnSetFocusPlayer4TeamRadio() { _onSetFocusPlayerTeamRadio(TPlayer(3)); }
void TMapSpecsTeamsPage::OnSetFocusPlayer5TeamRadio() { _onSetFocusPlayerTeamRadio(TPlayer(4)); }
void TMapSpecsTeamsPage::OnSetFocusPlayer6TeamRadio() { _onSetFocusPlayerTeamRadio(TPlayer(5)); }
void TMapSpecsTeamsPage::OnSetFocusPlayer7TeamRadio() { _onSetFocusPlayerTeamRadio(TPlayer(6)); }
void TMapSpecsTeamsPage::OnSetFocusPlayer8TeamRadio() { _onSetFocusPlayerTeamRadio(TPlayer(7)); }
void TMapSpecsTeamsPage::OnKillFocusPlayer1TeamRadio() { _onKillFocusPlayerTeamRadio(TPlayer(0)); }
void TMapSpecsTeamsPage::OnKillFocusPlayer2TeamRadio() { _onKillFocusPlayerTeamRadio(TPlayer(1)); }
void TMapSpecsTeamsPage::OnKillFocusPlayer3TeamRadio() { _onKillFocusPlayerTeamRadio(TPlayer(2)); }
void TMapSpecsTeamsPage::OnKillFocusPlayer4TeamRadio() { _onKillFocusPlayerTeamRadio(TPlayer(3)); }
void TMapSpecsTeamsPage::OnKillFocusPlayer5TeamRadio() { _onKillFocusPlayerTeamRadio(TPlayer(4)); }
void TMapSpecsTeamsPage::OnKillFocusPlayer6TeamRadio() { _onKillFocusPlayerTeamRadio(TPlayer(5)); }
void TMapSpecsTeamsPage::OnKillFocusPlayer7TeamRadio() { _onKillFocusPlayerTeamRadio(TPlayer(6)); }
void TMapSpecsTeamsPage::OnKillFocusPlayer8TeamRadio() { _onKillFocusPlayerTeamRadio(TPlayer(7)); }
void TMapSpecsTeamsPage::OnPlayer1Static() { _onPlayerStatic(TPlayer(0)); }
void TMapSpecsTeamsPage::OnPlayer2Static() { _onPlayerStatic(TPlayer(1)); }
void TMapSpecsTeamsPage::OnPlayer3Static() { _onPlayerStatic(TPlayer(2)); }
void TMapSpecsTeamsPage::OnPlayer4Static() { _onPlayerStatic(TPlayer(3)); }
void TMapSpecsTeamsPage::OnPlayer5Static() { _onPlayerStatic(TPlayer(4)); }
void TMapSpecsTeamsPage::OnPlayer6Static() { _onPlayerStatic(TPlayer(5)); }
void TMapSpecsTeamsPage::OnPlayer7Static() { _onPlayerStatic(TPlayer(6)); }
void TMapSpecsTeamsPage::OnPlayer8Static() { _onPlayerStatic(TPlayer(7)); }
