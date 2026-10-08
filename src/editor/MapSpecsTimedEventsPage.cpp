// MapSpecsTimedEventsPage.cpp - Loki h3maped object 75: the timed events
// page of the map specifications sheet. The timed_events_list glade list
// shows the events by first occurrence (then by index); a map holds at
// most 50 events, and only neighbours that occur on the same day can be
// reordered. The assert lines come from the retail immediates.
#include "editor/stdafx.h"

#include <stdio.h>
#include <bitset>
#include <vector>

#include "editor/cppbridge.h"
#include "editor/EditTimedEventSheet.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsTimedEventsPage.h"

namespace {

bitset<kNumPlayers> getPlayerPresentMask(const TGameMap& map)
{
    bitset<kNumPlayers> mask;
    for (unsigned int player = 0; player < kNumPlayers; player++)
        mask[player] = map.isPlayerPresent(TPlayer(player));
    return mask;
}

}

TMapSpecsTimedEventsPage::TMapSpecsTimedEventsPage(const TGameMap& map)
    : _m_map(map)
{
    _m_bModified = false;
}

TMapSpecsTimedEventsPage::~TMapSpecsTimedEventsPage()
{
}

int TMapSpecsTimedEventsPage::_addToEventListBox(unsigned int eventNum)
{
    const TTimedEvent& event = _m_events[eventNum];
    unsigned int firstOccurence = event.getFirstOccurence();
    GtkList* l = GTK_LIST(_widget("timed_events_list"));
    int pos;
    for (pos = g_list_length(l->children); pos > 0; pos--) {
        unsigned int prevEventNum = pos - 1;
        unsigned int prevFirstOccurence = _m_events[prevEventNum].getFirstOccurence();
        if (firstOccurence > prevFirstOccurence
            || (firstOccurence == prevFirstOccurence && eventNum > prevEventNum))
            break;
    }
    char name[512];
    snprintf(name, 512, kTimedEventNameFmtStr, firstOccurence + 1, event.getName().c_str());
    GtkListItem* li = GTK_LIST_ITEM(gtk_list_item_new_with_label(name));
#line 110
    assert(li != NULL);
    gtk_widget_show(GTK_WIDGET(li));
    GList* items = g_list_append(NULL, li);
    gtk_list_insert_items(l, items, pos);
    return pos;
}

BOOL TMapSpecsTimedEventsPage::OnInitDialog()
{
    _m_bModified = false;
    _m_newEvents = _m_map.getTimedEvents();
    _m_events = _m_newEvents;
    for (unsigned int i = 0; i < _m_events.size(); i++)
        _addToEventListBox(i);

    GtkList* l = GTK_LIST(_widget("timed_events_list"));
    int count = g_list_length(l->children);
    enableWidget("timed_events_add", count < 50);
    enableWidget("timed_events_edit", FALSE);
    enableWidget("timed_events_remove", FALSE);
    enableWidget("timed_events_remall", count > 0);
    enableWidget("timed_events_moveup", FALSE);
    enableWidget("timed_events_movedown", FALSE);
    return true;
}

void TMapSpecsTimedEventsPage::OnOK()
{
    _m_newEvents = _m_events;
    _m_bModified = _m_bModified || _m_newEvents != _m_map.getTimedEvents();
}

void TMapSpecsTimedEventsPage::OnAddEventButton()
{
    _m_pEventSheet = new TEditTimedEventSheet(getPlayerPresentMask(_m_map), TTimedEvent());
    if (_m_pEventSheet->DoModal() == IDOK) {
        GtkList* l = GTK_LIST(_widget("timed_events_list"));
        unsigned int newEventNum = _m_events.size();
        _m_events.push_back(_m_pEventSheet->getEvent());
        int pos = _addToEventListBox(newEventNum);
        gtk_list_select_item(l, pos);
        OnSelChangeEventsListbox();
        if (g_list_length(l->children) >= 50)
            enableWidget("timed_events_add", FALSE);
        enableWidget("timed_events_remall", TRUE);
    }
    delete _m_pEventSheet;
}

void TMapSpecsTimedEventsPage::OnEditEventButton()
{
    GtkList* l = GTK_LIST(_widget("timed_events_list"));
    int curSel = getCurrentSelection(l);
#line 262
    assert(curSel != -1);
    unsigned int eventNum = curSel;
#line 264
    assert(eventNum < _m_events.size());
    _m_pEventSheet = new TEditTimedEventSheet(getPlayerPresentMask(_m_map), _m_events[eventNum]);
    if (_m_pEventSheet->DoModal() == IDOK) {
        _m_events[eventNum] = _m_pEventSheet->getEvent();
        int pos = getCurrentSelection(l);
        gtk_list_clear_items(l, pos, pos + 1);
        pos = _addToEventListBox(eventNum);
        setCurrentSelection(l, pos);
        OnSelChangeEventsListbox();
    }
    delete _m_pEventSheet;
}

void TMapSpecsTimedEventsPage::OnRemoveEventButton()
{
    GtkList* l = GTK_LIST(_widget("timed_events_list"));
    int curSel = getCurrentSelection(l);
#line 299
    assert(curSel != -1);
    unsigned int eventNum = curSel;
#line 302
    assert(eventNum < _m_events.size());
    _m_events.erase(_m_events.begin() + eventNum);
    gtk_list_clear_items(l, curSel, curSel + 1);
    int count = g_list_length(l->children);
    if (count > 0) {
        if (count < 50)
            enableWidget("timed_events_add", TRUE);
        setCurrentSelection(l, curSel >= count ? count - 1 : curSel);
        OnSelChangeEventsListbox();
    } else {
        enableWidget("timed_events_add", TRUE);
        enableWidget("timed_events_edit", FALSE);
        enableWidget("timed_events_remove", FALSE);
        enableWidget("timed_events_remall", FALSE);
        enableWidget("timed_events_moveup", FALSE);
        enableWidget("timed_events_movedown", FALSE);
    }
}

void TMapSpecsTimedEventsPage::OnRemoveAllEventButton()
{
    GtkList* l = GTK_LIST(_widget("timed_events_list"));
#line 370
    assert(g_list_length(l->children) > 0);
    for (int count = g_list_length(l->children); count > 0; count--) {
        setCurrentSelection(l, 0);
        OnRemoveEventButton();
    }
}

void TMapSpecsTimedEventsPage::OnSelChangeEventsListbox()
{
    enableWidget("timed_events_edit", TRUE);
    enableWidget("timed_events_remove", TRUE);
    GtkList* l = GTK_LIST(_widget("timed_events_list"));
    int count = g_list_length(l->children);
#line 398
    assert(count > 0);
    int curSel = getCurrentSelection(l);
#line 401
    assert(curSel != -1);
    unsigned int eventNum = curSel;
#line 403
    assert(eventNum < _m_events.size());
    if (curSel > 0) {
        unsigned int prevEventNum = curSel - 1;
#line 408
        assert(prevEventNum < _m_events.size());
        enableWidget("timed_events_moveup", _m_events[prevEventNum].getFirstOccurence()
                                                == _m_events[eventNum].getFirstOccurence());
    } else
        enableWidget("timed_events_moveup", FALSE);
    if (curSel < count - 1) {
        unsigned int nextEventNum = curSel + 1;
#line 422
        assert(nextEventNum < _m_events.size());
        enableWidget("timed_events_movedown", _m_events[nextEventNum].getFirstOccurence()
                                                  == _m_events[eventNum].getFirstOccurence());
    } else
        enableWidget("timed_events_movedown", FALSE);
}

void TMapSpecsTimedEventsPage::OnSelCancelEventsListbox()
{
    enableWidget("timed_events_edit", FALSE);
    enableWidget("timed_events_remove", FALSE);
    enableWidget("timed_events_moveup", FALSE);
    enableWidget("timed_events_movedown", FALSE);
}

void TMapSpecsTimedEventsPage::OnDblClkEventsListbox()
{
    OnEditEventButton();
}

void TMapSpecsTimedEventsPage::OnMoveUpButton()
{
    GtkList* l = GTK_LIST(_widget("timed_events_list"));
    int curSel = getCurrentSelection(l);
#line 486
    assert(curSel > 0);
    unsigned int eventNum = curSel;
#line 490
    assert(eventNum < _m_events.size());
    unsigned int prevEventNum = curSel - 1;
#line 492
    assert(prevEventNum < _m_events.size());
#line 495
    assert(_m_events[ eventNum ].getFirstOccurence() == _m_events[ prevEventNum ].getFirstOccurence());
    swap(_m_events[eventNum], _m_events[prevEventNum]);
    gtk_list_clear_items(l, curSel, curSel + 1);
    curSel = _addToEventListBox(prevEventNum);
    setCurrentSelection(l, curSel);
    OnSelChangeEventsListbox();
}

void TMapSpecsTimedEventsPage::OnMoveDownButton()
{
    GtkList* l = GTK_LIST(_widget("timed_events_list"));
    int count = g_list_length(l->children);
#line 518
    assert(count > 0);
    int curSel = getCurrentSelection(l);
#line 523
    assert(curSel != -1 && curSel < count - 1);
    unsigned int eventNum = curSel;
#line 527
    assert(eventNum < _m_events.size());
    unsigned int nextEventNum = curSel + 1;
#line 529
    assert(nextEventNum < _m_events.size());
#line 532
    assert(_m_events[ eventNum ].getFirstOccurence() == _m_events[ nextEventNum ].getFirstOccurence());
    swap(_m_events[eventNum], _m_events[nextEventNum]);
    gtk_list_clear_items(l, curSel, curSel + 1);
    curSel = _addToEventListBox(nextEventNum);
    setCurrentSelection(l, curSel);
    OnSelChangeEventsListbox();
}
