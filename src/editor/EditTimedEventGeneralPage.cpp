// EditTimedEventGeneralPage.cpp - Loki h3maped object 80: the general page
// of the timed event sheet. OnInitDialog loads the event into the page and
// the timed_event glade widgets (players absent from the map are
// insensitive); OnOK reads the widgets back and records whether anything
// differs from the event. The assert lines come from the retail
// immediates.
#include "editor/stdafx.h"

#include <stdio.h>
#include <string>

#include "editor/cppbridge.h"
#include "editor/Clamp.h"
#include "editor/EditTimedEventGeneralPage.h"
#include "editor/MapEditorText.h"

namespace {

// One entry of the event_subsequent combo: its text and repeat interval.
struct TSubsequentIntervalEntryInfo {
    TSubsequentIntervalEntryInfo(const char* pText, unsigned int interval)
        : m_pText(pText), m_interval(interval) {}

    const char* m_pText;
    unsigned int m_interval;
};

}

TEditTimedEventGeneralPage::TEditTimedEventGeneralPage(void* pParentSheet, const TTimedEvent& event,
                                                       const TPlayerMask& playersPresent)
    : _m_event(event),
      _m_playersPresent(playersPresent)
{
#line 88
    assert(pParentSheet != NULL);
    _m_name = "";
    _m_message = "";
    _m_bApplyToComputer = FALSE;
    OnInitDialog();
}

TEditTimedEventGeneralPage::~TEditTimedEventGeneralPage()
{
}

static GtkToggleButton* getPlayerCheck(unsigned int player)
{
    char name[50];
    snprintf(name, 50, "event_player%d", player);
    GtkToggleButton* pCheck = GTK_TOGGLE_BUTTON(_widget(name));
    return pCheck;
}

void TEditTimedEventGeneralPage::UpdateData(bool bSaveAndValidate)
{
    gchar* text;
    GtkEditable* pEdit = GTK_EDITABLE(_widget("timed_event_name"));
    text = gtk_editable_get_chars(pEdit, 0, -1);
    _m_name = text;
    g_free(text);
    pEdit = GTK_EDITABLE(_widget("timed_event_message"));
    text = gtk_editable_get_chars(pEdit, 0, -1);
    _m_message = text;
    g_free(text);
    GtkToggleButton* pCheck = GTK_TOGGLE_BUTTON(_widget("timed_event_comp_opponents"));
    _m_bApplyToComputer = gtk_toggle_button_get_active(pCheck);
}

bool TEditTimedEventGeneralPage::getBApplyToPlayer(TPlayer player) const
{
#line 141
    assert(player >= 0 && player < kNumPlayers);
    return _m_bApplyToPlayer[player];
}

BOOL TEditTimedEventGeneralPage::OnInitDialog()
{
    _m_bModified = false;
    _m_name = _m_event.getName().c_str();
    _m_bBlankName = _isspace(_m_name);
    _m_message = _m_event.getMessage().c_str();

    unsigned int player;
    for (player = 0; player < kNumPlayers; player++)
        _m_bApplyToPlayer[player] = _m_event.getBApplyToPlayer(TPlayer(player));
    _m_bApplyToComputer = _m_event.getBApplyToComputer();
    _m_firstOccurence = _m_event.getFirstOccurence();
    _m_subsequentInterval = _m_event.getSubsequentInterval();
    if (_m_bBlankName) {
    }

    for (player = 0; player < kNumPlayers; player++) {
        if (_m_playersPresent[player])
            gtk_toggle_button_set_active(getPlayerCheck(player), _m_bApplyToPlayer[player]);
        else
            gtk_widget_set_sensitive(GTK_WIDGET(getPlayerCheck(player)), FALSE);
    }

    GtkSpinButton* pDaySpin = GTK_SPIN_BUTTON(_widget("event_day_spin"));
    GtkAdjustment* adj = gtk_spin_button_get_adjustment(pDaySpin);
#line 262
    assert(adj != NULL);
    adj->lower = 1;
    adj->upper = 672.0f;

    static const TSubsequentIntervalEntryInfo akSubsequentIntervals[] = {
        TSubsequentIntervalEntryInfo(kNeverStr, 0),
        TSubsequentIntervalEntryInfo(kEveryDayStr, 1),
        TSubsequentIntervalEntryInfo(kEvery2DaysStr, 2),
        TSubsequentIntervalEntryInfo(kEvery3DaysStr, 3),
        TSubsequentIntervalEntryInfo(kEvery4DaysStr, 4),
        TSubsequentIntervalEntryInfo(kEvery5DaysStr, 5),
        TSubsequentIntervalEntryInfo(kEvery6DaysStr, 6),
        TSubsequentIntervalEntryInfo(kEvery7DaysStr, 7),
        TSubsequentIntervalEntryInfo(kEvery14DaysStr, 14),
        TSubsequentIntervalEntryInfo(kEvery21DaysStr, 21),
        TSubsequentIntervalEntryInfo(kEvery28DaysStr, 28),
    };
    GtkCombo* c = GTK_COMBO(_widget("event_subsequent"));
    unsigned int cbIndex = 0;
    for (unsigned int i = 0; i < sizeof(akSubsequentIntervals) / sizeof(akSubsequentIntervals[0]); i++) {
        _m_aComboIntervals[cbIndex] = akSubsequentIntervals[i].m_interval;
        cbIndex++;
    }
    for (cbIndex = 0; _m_aComboIntervals[cbIndex] != _m_subsequentInterval; cbIndex++) {
#line 309
        assert(cbIndex < g_list_length(GTK_LIST(c->list)->children));
    }
    setCurrentSelection(GTK_LIST(c->list), cbIndex);
    return true;
}

void TEditTimedEventGeneralPage::OnOK()
{
    unsigned int player;
    for (player = 0; player < kNumPlayers; player++) {
        if (_m_playersPresent[player])
            _m_bApplyToPlayer[player] = gtk_toggle_button_get_active(getPlayerCheck(player));
    }

    GtkSpinButton* pDaySpin = GTK_SPIN_BUTTON(_widget("event_day_spin"));
    GtkCombo* c = GTK_COMBO(_widget("event_subsequent"));
    int day = gtk_spin_button_get_value_as_int(pDaySpin);
    day = clamp(1, day, 672);
    _m_firstOccurence = day - 1;
#line 340
    assert(getCurrentSelection(GTK_LIST(c->list)) != -1);
    _m_subsequentInterval = _m_aComboIntervals[getCurrentSelection(GTK_LIST(c->list))];

    _m_bModified = _m_bModified
                   || string(_m_name) != _m_event.getName()
                   || string(_m_message) != _m_event.getMessage()
                   || (_m_bApplyToComputer != FALSE) != _m_event.getBApplyToComputer()
                   || _m_firstOccurence != _m_event.getFirstOccurence()
                   || _m_subsequentInterval != _m_event.getSubsequentInterval();
    for (player = 0; !_m_bModified && player < kNumPlayers; player++)
        _m_bModified = _m_bApplyToPlayer[player] != _m_event.getBApplyToPlayer(TPlayer(player));
}

void TEditTimedEventGeneralPage::OnChangeEventNameEdit()
{
    UpdateData(false);
    if (!isAllSpace(_m_name.c_str())) {
        if (_m_bBlankName)
            _m_bBlankName = false;
    } else {
        if (!_m_bBlankName)
            _m_bBlankName = true;
    }
}

void TEditTimedEventGeneralPage::OnKillFocusFirstOccurenceEdit()
{
}
