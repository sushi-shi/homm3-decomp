// MapSpecsPlayerSpecsPage.cpp - Loki h3maped object 72: the player page of
// the map specifications sheet over the playerspecs_* glade widgets. The
// player combo lists the present players; switching players stores the
// widgets into the edited player info and loads the next. A map keeps at
// least one human-playable player: unchecking the last one re-checks it, or
// makes the first present player human-playable. The g_warning traces are
// Loki's. The assert lines come from the retail immediates.
#include "editor/stdafx.h"

#include <stdio.h>
#include <algorithm>
#include <iterator>
#include <set>
#include <vector>

#include "editor/cppbridge.h"
#include "editor/GameObject.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsPlayerSpecsPage.h"
#include "editor/Town.h"

namespace {

const char* getBehaviorTypeName(TPlayerInfo::TBehaviorType behaviorType)
{
#line 55
    assert(behaviorType >= 0 && behaviorType < TPlayerInfo::s_kNumBehaviorTypes);
    static const char* const akBehaviorTypeName[TPlayerInfo::s_kNumBehaviorTypes] = {
        kRandomStr, kWarriorStr, kBuilderStr, kExplorerStr
    };
    return akBehaviorTypeName[behaviorType];
}

}

TMapSpecsPlayerSpecsPage::TMapSpecsPlayerSpecsPage(const TGameMap& map)
    : _m_map(map),
      _m_bModified(false),
      _m_bSettingHumanPlayable(false)
{
}

TMapSpecsPlayerSpecsPage::~TMapSpecsPlayerSpecsPage()
{
}

void TMapSpecsPlayerSpecsPage::_setPlayerInfo()
{
#line 99
    assert(_m_curPlayer != ePlayerNone);
#line 102
    assert(_m_intPlayers[ _m_curPlayer ].getBPresent());
    GtkCombo* c = GTK_COMBO(_widget("playerspecs_player_combo"));
#line 105
    assert(getCurrentSelection(GTK_LIST(c->list)) != -1);
    assert(_m_playerItemData[getCurrentSelection(GTK_LIST(c->list))] == _m_curPlayer);

    c = GTK_COMBO(_widget("playerspecs_maintown_combo"));
    GList* items = NULL;
    emptyList(GTK_LIST(c->list));
    _m_aTownRef.clear();
    const set<TMapObjectRef>& townRefs = _m_map.getPlayerTownRefs(_m_curPlayer);
    _m_aTownRef.reserve(townRefs.size());
    copy(townRefs.begin(), townRefs.end(), back_inserter(_m_aTownRef));
    int cbIndex;
    for (unsigned int i = 0; i < _m_aTownRef.size(); i++) {
        const TMapObjectRef& ref = _m_aTownRef[i];
        const TTown* pTown = dynamic_cast<const TTown*>(&_m_map.getLayer(ref.getBSecondLayer()).getObject(ref.getObjectID()));
#line 125
        assert(pTown != NULL);
        assert(pTown->getOwner() == _m_curPlayer);
        if (pTown->getPVisitingHero() == NULL) {
            TPoint<unsigned int> loc = _m_map.getLayer(ref.getBSecondLayer()).getObjectLoc(ref.getObjectID())
                                       - pTown->getTriggerLoc();
            char* name = (char*)g_malloc(100);
            snprintf(name, 100, kObjectAtLocationFmtStr, pTown->getTownTypeTraits().m_pName, loc.x(), loc.y(),
                     ref.getBSecondLayer());
            items = g_list_append(items, name);
            _m_townItemData[cbIndex] = i;
        }
    }
    if (items) {
        gtk_combo_set_popdown_strings(c, items);
        g_list_free(items);
        items = NULL;
    }

    GtkToggleButton* pGenerateHeroCheck = GTK_TOGGLE_BUTTON(_widget("generate_hero_check"));
    if (_m_intPlayers[_m_curPlayer].getBGenerateHero()) {
#line 161
        assert(_m_abCanGenerateHero[ _m_curPlayer ]);
#line 163
        assert(g_list_length(GTK_LIST(c->list)->children) > 0);
        gtk_widget_set_sensitive(GTK_WIDGET(pGenerateHeroCheck), TRUE);
        gtk_toggle_button_set_active(pGenerateHeroCheck, TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(c), TRUE);
        for (cbIndex = 0; _m_aTownRef[_m_townItemData[cbIndex]] != _m_intPlayers[_m_curPlayer].getMainTownRef();
             cbIndex++) {
#line 175
            assert(cbIndex < g_list_length(GTK_LIST(c->list)->children));
        }
        setCurrentSelection(GTK_LIST(c->list), cbIndex);
    } else {
        gtk_toggle_button_set_active(pGenerateHeroCheck, FALSE);
        if (_m_abCanGenerateHero[_m_curPlayer] && g_list_length(GTK_LIST(c->list)->children))
            gtk_widget_set_sensitive(GTK_WIDGET(pGenerateHeroCheck), TRUE);
        else
            gtk_widget_set_sensitive(GTK_WIDGET(pGenerateHeroCheck), FALSE);
        setCurrentSelection(GTK_LIST(c->list), -1);
        gtk_widget_set_sensitive(GTK_WIDGET(c), FALSE);
    }

    GtkToggleButton* pHumanPlayableCheck = GTK_TOGGLE_BUTTON(_widget("human_playable_check"));
    g_warning("_setPlayerInfo(): (%d) is human playable == (%d).\n", _m_curPlayer,
              _m_intPlayers[_m_curPlayer].getBHumanPlayable());
    _m_bSettingHumanPlayable = true;
    gtk_toggle_button_set_active(pHumanPlayableCheck, _m_intPlayers[_m_curPlayer].getBHumanPlayable());

    c = GTK_COMBO(_widget("playerspecs_behaviour_combo"));
    GtkCheckButton* pComputerPlayableCheck = GTK_CHECK_BUTTON(_widget("computer_playable_check"));
    if (_m_intPlayers[_m_curPlayer].getBComputerPlayable()) {
        gtk_widget_set_sensitive(GTK_WIDGET(c), TRUE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pComputerPlayableCheck), TRUE);
        int len = g_list_length(GTK_LIST(c->list)->children);
        for (cbIndex = 0; _m_behaviorItemData[cbIndex] != _m_intPlayers[_m_curPlayer].getBehaviorType(); cbIndex++) {
#line 221
            assert(cbIndex < len);
        }
        setCurrentSelection(GTK_LIST(c->list), cbIndex);
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pComputerPlayableCheck), FALSE);
        setCurrentSelection(GTK_LIST(c->list), -1);
        gtk_widget_set_sensitive(GTK_WIDGET(c), FALSE);
    }
}

void TMapSpecsPlayerSpecsPage::_retrievePlayerInfo()
{
#line 242
    assert(_m_curPlayer != ePlayerNone);
    assert(_m_intPlayers[ _m_curPlayer ].getBPresent());
    GtkToggleButton* cPlayCheck = GTK_TOGGLE_BUTTON(_widget("computer_playable_check"));
    GtkToggleButton* hPlayCheck = GTK_TOGGLE_BUTTON(_widget("human_playable_check"));
    GtkToggleButton* genHeroCheck = GTK_TOGGLE_BUTTON(_widget("generate_hero_check"));
#line 251
    assert(gtk_toggle_button_get_active(hPlayCheck) || gtk_toggle_button_get_active(cPlayCheck));
    GtkCombo* c = GTK_COMBO(_widget("playerspecs_behaviour_combo"));
    TPlayerInfo& playerInfo = _m_intPlayers[_m_curPlayer];
    playerInfo.setBHumanPlayable(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hPlayCheck)));
    if (playerInfo.getBComputerPlayable()) {
        int curSel = getCurrentSelection(GTK_LIST(c->list));
        TPlayerInfo::TBehaviorType behaviorType = TPlayerInfo::TBehaviorType(_m_behaviorItemData[curSel]);
#line 261
        assert(behaviorType >= 0 && behaviorType < TPlayerInfo::s_kNumBehaviorTypes);
        playerInfo.setBehaviorType(behaviorType);
    } else
        playerInfo.setBehaviorType(TPlayerInfo::TBehaviorType(0));

    c = GTK_COMBO(_widget("playerspecs_maintown_combo"));
    int curSel = getCurrentSelection(GTK_LIST(c->list));
    playerInfo.setBGenerateHero(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(genHeroCheck)));
    if (playerInfo.getBGenerateHero()) {
#line 272
        assert(_m_abCanGenerateHero[ _m_curPlayer ]);
        assert(curSel != -1);
        unsigned int itemData = _m_townItemData[curSel];
#line 276
        assert(itemData >= 0 && itemData < _m_aTownRef.size());
        playerInfo.setMainTownRef(_m_aTownRef[itemData]);
    } else {
#line 281
        assert(curSel == -1);
        playerInfo.setMainTownRef(TMapObjectRef());
    }
}

void TMapSpecsPlayerSpecsPage::enableWidgets(bool bEnable)
{
    gtk_widget_set_sensitive(_widget("generate_hero_check"), bEnable);
    gtk_widget_set_sensitive(_widget("playerspecs_maintown_combo"), bEnable);
    gtk_widget_set_sensitive(_widget("human_playable_check"), bEnable);
    gtk_widget_set_sensitive(_widget("computer_playable_check"), bEnable);
    gtk_widget_set_sensitive(_widget("playerspecs_behaviour_combo"), bEnable);
    gtk_widget_set_sensitive(_widget("behaviour_label"), bEnable);
}

BOOL TMapSpecsPlayerSpecsPage::OnInitDialog()
{
    GList* items = NULL;
    GtkCombo* behaviorC = GTK_COMBO(_widget("playerspecs_behaviour_combo"));
    GtkCombo* playerC = GTK_COMBO(_widget("playerspecs_player_combo"));
    _m_bModified = false;
    _m_players = _m_map.getPlayers();
    _m_intPlayers = _m_players;

    unsigned int player = 0;
    while (player < kNumPlayers && !_m_intPlayers[player].getBPresent())
        player++;
    if (player < kNumPlayers) {
        _m_curPlayer = TPlayer(player);
        _m_numHumanPlayable = 0;
        _m_abCanGenerateHero = _m_map.getAvailableHeroOwnersMask();
        for (player = 0; player < kNumPlayers; player++) {
            if (_m_intPlayers[player].getBHumanPlayable()) {
                g_warning("(%d) IS human playable.", player);
                _m_numHumanPlayable++;
            }
            if (_m_intPlayers[player].getBGenerateHero())
                _m_abCanGenerateHero[player] = true;
        }
        g_warning("_m_numHumanPlayable == (%d).\n", _m_numHumanPlayable);
#line 369
        assert(_m_numHumanPlayable > 0);
    } else
        _m_curPlayer = ePlayerNone;

    if (_m_curPlayer != ePlayerNone) {
        int cbIndex = 0;
        for (unsigned int behaviorType = 0; behaviorType < TPlayerInfo::s_kNumBehaviorTypes; behaviorType++) {
            items = g_list_append(items, (gpointer)getBehaviorTypeName(TPlayerInfo::TBehaviorType(behaviorType)));
            _m_behaviorItemData[cbIndex] = behaviorType;
            cbIndex++;
        }
        if (items) {
            gtk_combo_set_popdown_strings(behaviorC, items);
            g_list_free(items);
            items = NULL;
        }
        cbIndex = 0;
        for (player = 0; player < kNumPlayers; player++) {
            if (_m_intPlayers[player].getBPresent()) {
                items = g_list_append(items, (gpointer)akPlayerTraits[player].m_pName);
                _m_playerItemData[cbIndex] = player;
                cbIndex++;
            }
        }
        if (items) {
            gtk_combo_set_popdown_strings(playerC, items);
            g_list_free(items);
            items = NULL;
        }
        for (cbIndex = 0; _m_playerItemData[cbIndex] != _m_curPlayer; cbIndex++) {
#line 419
            assert(cbIndex < g_list_length(GTK_LIST(playerC->list)->children));
        }
        setCurrentSelection(GTK_LIST(playerC->list), cbIndex);
        _setPlayerInfo();
        enableWidgets(true);
    } else {
        items = NULL;
        items = g_list_append(items, (gpointer)kNoPlayersOnMapStr);
        gtk_combo_set_popdown_strings(playerC, items);
        g_list_free(items);
        setCurrentSelection(GTK_LIST(playerC->list), 0);
        gtk_widget_set_sensitive(GTK_WIDGET(playerC), FALSE);
        enableWidgets(false);
    }
    return true;
}

void TMapSpecsPlayerSpecsPage::OnOK()
{
    if (_m_curPlayer != ePlayerNone) {
        _retrievePlayerInfo();
        _m_players = _m_intPlayers;
    }
    _m_bModified = _m_bModified || _m_players != _m_map.getPlayers();
}

void TMapSpecsPlayerSpecsPage::OnSelChangePlayerCombo()
{
    if (_m_curPlayer == ePlayerNone)
        return;
    GtkCombo* c = GTK_COMBO(_widget("playerspecs_player_combo"));
    int curSel = getCurrentSelection(GTK_LIST(c->list));
    if (curSel == -1)
        return;
    _retrievePlayerInfo();
    _m_curPlayer = TPlayer(_m_playerItemData[curSel]);
#line 483
    assert(_m_curPlayer >= 0 && _m_curPlayer < kNumPlayers);
    _setPlayerInfo();
}

void TMapSpecsPlayerSpecsPage::OnGenerateHeroCheck()
{
#line 491
    assert(_m_curPlayer != ePlayerNone);
    assert(_m_abCanGenerateHero[ _m_curPlayer ]);
    GtkCombo* c = GTK_COMBO(_widget("playerspecs_maintown_combo"));
    GtkToggleButton* b = GTK_TOGGLE_BUTTON(_widget("generate_hero_check"));
    if (!gtk_toggle_button_get_active(b)) {
        setCurrentSelection(GTK_LIST(c->list), -1);
        gtk_widget_set_sensitive(GTK_WIDGET(c), FALSE);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(c), TRUE);
        setCurrentSelection(GTK_LIST(c->list), 0);
    }
}

void TMapSpecsPlayerSpecsPage::OnHumanPlayableCheck()
{
    if (_m_bSettingHumanPlayable) {
        _m_bSettingHumanPlayable = false;
        return;
    }
#line 524
    assert(_m_curPlayer != ePlayerNone);
    g_warning("OnHumanPlayableCheck()...number of human playable == (%d)", _m_numHumanPlayable);
    GtkToggleButton* hPlayCheck = GTK_TOGGLE_BUTTON(_widget("human_playable_check"));
    GtkToggleButton* cPlayCheck = GTK_TOGGLE_BUTTON(_widget("computer_playable_check"));
    if (!gtk_toggle_button_get_active(hPlayCheck)) {
        g_warning("decrementing human playable...");
        _m_numHumanPlayable--;
        if (_m_numHumanPlayable == 0) {
            _m_numHumanPlayable = 0;
            g_warning("NO HUMAN PLAYERS!");
            unsigned int player = 0;
            while (!_m_intPlayers[player].getBPresent()) {
                player++;
#line 552
                assert(player < kNumPlayers);
            }
            if (player == _m_curPlayer) {
                g_warning("hmm. it's us.");
                _m_bSettingHumanPlayable = true;
                gtk_toggle_button_set_active(hPlayCheck, TRUE);
            } else {
                g_warning("Another player is to be enabled.");
#line 567
                assert(!_m_intPlayers[ player ].getBHumanPlayable());
                _m_intPlayers[player].setBHumanPlayable(true);
            }
            g_warning("incrementing human playable to recover...");
            _m_numHumanPlayable++;
        }
        if (!gtk_toggle_button_get_active(hPlayCheck) && !gtk_toggle_button_get_active(cPlayCheck)) {
            g_warning("defaulting to computer playable...");
            gtk_toggle_button_set_active(cPlayCheck, TRUE);
            OnComputerPlayableCheck();
        }
    } else {
        g_warning("incrementing human playable...");
        _m_numHumanPlayable++;
    }
}

void TMapSpecsPlayerSpecsPage::OnComputerPlayableCheck()
{
    if (_m_bSettingHumanPlayable == true) {
        _m_bSettingHumanPlayable = false;
        return;
    }
#line 601
    assert(_m_curPlayer != ePlayerNone);
    GtkToggleButton* hPlayCheck = GTK_TOGGLE_BUTTON(_widget("human_playable_check"));
    GtkToggleButton* cPlayCheck = GTK_TOGGLE_BUTTON(_widget("computer_playable_check"));
    GtkCombo* c = GTK_COMBO(_widget("playerspecs_behaviour_combo"));
    if (!gtk_toggle_button_get_active(cPlayCheck)) {
        if (!gtk_toggle_button_get_active(hPlayCheck))
            gtk_toggle_button_set_active(cPlayCheck, TRUE);
        setCurrentSelection(GTK_LIST(c->list), -1);
        gtk_widget_set_sensitive(GTK_WIDGET(c), FALSE);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(c), TRUE);
        int cbIndex;
        int listlen = g_list_length(GTK_LIST(c->list)->children);
        for (cbIndex = 0; _m_behaviorItemData[cbIndex] != 0; cbIndex++) {
#line 637
            assert(cbIndex < listlen);
        }
        setCurrentSelection(GTK_LIST(c->list), cbIndex);
    }
}
