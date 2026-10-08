// TownPropsTimedEventsPage.cpp - Loki h3maped object 96: the timed events
// page of the town property sheet. The town_props_timed_list glade list
// shows the events by first occurrence (then by index) and keeps the event
// shown at each position; a town holds at most TTown::s_kMaxTimedEvents
// events, and only neighbours that occur on the same day can be reordered.
// The assert lines come from the retail immediates.
#include "editor/stdafx.h"

#include <stdio.h>
#include <vector>

#include "editor/cppbridge.h"
#include "editor/EditTownEventSheet.h"
#include "editor/MapEditorText.h"
#include "editor/TownPropsTimedEventsPage.h"

TTownPropsTimedEventsPage::TTownPropsTimedEventsPage(const TTown& town, const TPlayerMask& playersPresent)
    : _m_moveDownButton(GTK_BUTTON(_widget("town_props_timed_movedown"))),
      _m_moveUpButton(GTK_BUTTON(_widget("town_props_timed_moveup"))),
      _m_removeAllButton(GTK_BUTTON(_widget("town_props_timed_remall"))),
      _m_removeButton(GTK_BUTTON(_widget("town_props_timed_remove"))),
      _m_editButton(GTK_BUTTON(_widget("town_props_timed_edit"))),
      _m_addButton(GTK_BUTTON(_widget("town_props_timed_add"))),
      _m_eventList(GTK_LIST(_widget("town_props_timed_list"))),
      _m_town(town),
      _m_playersPresent(playersPresent),
      _m_bModified(false)
{
    OnInitDialog();
}

TTownPropsTimedEventsPage::~TTownPropsTimedEventsPage()
{
}

int TTownPropsTimedEventsPage::_addToEventListBox(unsigned int eventNum)
{
    const TTown::TTimedEvent& event = _m_events[eventNum];
    unsigned int firstOccurence = event.getFirstOccurence();
    int pos;
    for (pos = g_list_length(_m_eventList->children); pos > 0; pos--) {
        unsigned int prevEventNum = _m_aEventNum[pos - 1];
        unsigned int prevFirstOccurence = _m_events[prevEventNum].getFirstOccurence();
        if (firstOccurence > prevFirstOccurence
            || (firstOccurence == prevFirstOccurence && eventNum > prevEventNum))
            break;
    }
    char name[1024];
    snprintf(name, 1024, kTimedEventNameFmtStr, firstOccurence + 1, event.getName().c_str());
    GtkListItem* li = GTK_LIST_ITEM(gtk_list_item_new_with_label(name));
#line 98
    assert(li != NULL);
    gtk_widget_show(GTK_WIDGET(li));
    GList* items = g_list_append(NULL, li);
    gtk_list_insert_items(_m_eventList, items, pos);
    _m_aEventNum[pos] = eventNum;
    return pos;
}

BOOL TTownPropsTimedEventsPage::OnInitDialog()
{
    _m_bModified = false;
    _m_newEvents = _m_town.getTimedEvents();
    _m_events = _m_newEvents;
    for (unsigned int i = 0; i < _m_events.size(); i++)
        _addToEventListBox(i);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_addButton), _m_events.size() < TTown::s_kMaxTimedEvents);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_editButton), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_removeButton), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_removeAllButton), _m_events.size() != 0);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_moveUpButton), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_moveDownButton), FALSE);
    return true;
}

void TTownPropsTimedEventsPage::OnOK()
{
    _m_newEvents = _m_events;
    _m_bModified = _m_bModified || _m_newEvents != _m_town.getTimedEvents();
}

void TTownPropsTimedEventsPage::OnAddEventButton()
{
#line 206
    assert(_m_events.size() < TTown::s_kMaxTimedEvents);
#line 208
    assert(g_list_length(_m_eventList->children) == _m_events.size());
    TEditTownEventSheet sheet(NULL, _m_playersPresent, _m_town.getTownType(), TTown::TTimedEvent());
    if (sheet.DoModal() == IDOK) {
        unsigned int newEventNum = _m_events.size();
        _m_events.push_back(sheet.getEvent());
        int pos = _addToEventListBox(newEventNum);
        setCurrentSelection(_m_eventList, pos);
        if (g_list_length(_m_eventList->children) >= TTown::s_kMaxTimedEvents)
            gtk_widget_set_sensitive(GTK_WIDGET(_m_addButton), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_removeAllButton), TRUE);
    }
}

void TTownPropsTimedEventsPage::OnEditEventButton()
{
#line 244
    assert(g_list_length(_m_eventList->children) == _m_events.size());
    int curSel = getCurrentSelection(_m_eventList);
#line 249
    assert(curSel != -1);
    unsigned int eventNum = _m_aEventNum[curSel];
#line 252
    assert(eventNum < _m_events.size());
#line 255
    assert(curSel < _m_events.size());
    TEditTownEventSheet sheet(NULL, _m_playersPresent, _m_town.getTownType(), _m_events[eventNum]);
    if (sheet.DoModal() == IDOK) {
        _m_events[eventNum] = sheet.getEvent();
        gtk_list_clear_items(_m_eventList, curSel, curSel + 1);
        curSel = _addToEventListBox(eventNum);
        setCurrentSelection(_m_eventList, curSel);
    }
}

void TTownPropsTimedEventsPage::OnRemoveEventButton()
{
#line 280
    assert(g_list_length(_m_eventList->children) == _m_events.size());
    int curSel = getCurrentSelection(_m_eventList);
#line 284
    assert(curSel != -1);
    unsigned int eventNum = _m_aEventNum[curSel];
#line 287
    assert(eventNum < _m_events.size());
    _m_events.erase(_m_events.begin() + eventNum);
    gtk_list_clear_items(_m_eventList, curSel, curSel + 1);
    int count = g_list_length(_m_eventList->children);
    if (count > 0) {
        if (count < TTown::s_kMaxTimedEvents)
            gtk_widget_set_sensitive(GTK_WIDGET(_m_addButton), TRUE);
        for (int i = 0; i < count; i++) {
            unsigned int num = _m_aEventNum[i];
            if (num > eventNum)
                _m_aEventNum[i] = num - 1;
        }
        setCurrentSelection(_m_eventList, curSel < count ? curSel : count - 1);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(_m_addButton), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_editButton), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_removeButton), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_removeAllButton), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_moveUpButton), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_moveDownButton), FALSE);
        gtk_widget_grab_focus(GTK_WIDGET(_m_addButton));
    }
}

void TTownPropsTimedEventsPage::OnRemoveAllEventButton()
{
#line 356
    assert(_m_events.size() > 0);
    int count = g_list_length(_m_eventList->children);
#line 360
    assert(count == _m_events.size());
    if (getCurrentSelection(_m_eventList) == -1)
        setCurrentSelection(_m_eventList, 0);
    while (count > 0) {
        OnRemoveEventButton();
        count--;
    }
}

void TTownPropsTimedEventsPage::OnSelChangeEventList()
{
#line 381
    assert(getCurrentSelection(_m_eventList) != -1);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_editButton), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_removeButton), TRUE);
    int count = g_list_length(_m_eventList->children);
#line 392
    assert(count > 0);
    int curSel = getCurrentSelection(_m_eventList);
#line 395
    assert(curSel != -1);
    unsigned int eventNum = _m_aEventNum[curSel];
#line 398
    assert(eventNum < _m_events.size());
    if (curSel > 0) {
        unsigned int prevEventNum = _m_aEventNum[curSel - 1];
#line 404
        assert(prevEventNum < _m_events.size());
        gtk_widget_set_sensitive(GTK_WIDGET(_m_moveUpButton), _m_events[prevEventNum].getFirstOccurence()
                                                                  == _m_events[eventNum].getFirstOccurence());
    } else
        gtk_widget_set_sensitive(GTK_WIDGET(_m_moveUpButton), FALSE);
    if (curSel < count - 1) {
        unsigned int nextEventNum = _m_aEventNum[curSel + 1];
#line 418
        assert(nextEventNum < _m_events.size());
        gtk_widget_set_sensitive(GTK_WIDGET(_m_moveDownButton), _m_events[nextEventNum].getFirstOccurence()
                                                                    == _m_events[eventNum].getFirstOccurence());
    } else
        gtk_widget_set_sensitive(GTK_WIDGET(_m_moveDownButton), FALSE);
}

void TTownPropsTimedEventsPage::OnSelCancelEventList()
{
#line 433
    assert(getCurrentSelection(_m_eventList) == -1);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_editButton), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_removeButton), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_moveUpButton), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_moveDownButton), FALSE);
}

void TTownPropsTimedEventsPage::OnDblClkEventList()
{
    OnEditEventButton();
}

void TTownPropsTimedEventsPage::OnMoveUpButton()
{
    int curSel = getCurrentSelection(_m_eventList);
#line 482
    assert(curSel > 0);
    unsigned int eventNum = _m_aEventNum[curSel];
#line 487
    assert(eventNum < _m_events.size());
    unsigned int prevEventNum = _m_aEventNum[curSel - 1];
#line 490
    assert(prevEventNum < _m_events.size());
#line 493
    assert(_m_events[ eventNum ].getFirstOccurence() == _m_events[ prevEventNum ].getFirstOccurence());
    swap(_m_events[eventNum], _m_events[prevEventNum]);
    _m_aEventNum[curSel - 1] = eventNum;
    gtk_list_clear_items(_m_eventList, curSel, curSel + 1);
    curSel = _addToEventListBox(prevEventNum);
    setCurrentSelection(_m_eventList, curSel);
}

void TTownPropsTimedEventsPage::OnMoveDownButton()
{
    int count = g_list_length(_m_eventList->children);
#line 517
    assert(count > 0);
    int curSel = getCurrentSelection(_m_eventList);
#line 523
    assert(curSel != -1 && curSel < count - 1);
    unsigned int eventNum = _m_aEventNum[curSel];
#line 528
    assert(eventNum < _m_events.size());
    unsigned int nextEventNum = _m_aEventNum[curSel + 1];
#line 531
    assert(nextEventNum < _m_events.size());
#line 534
    assert(_m_events[ eventNum ].getFirstOccurence() == _m_events[ nextEventNum ].getFirstOccurence());
    swap(_m_events[eventNum], _m_events[nextEventNum]);
    _m_aEventNum[curSel + 1] = eventNum;
    gtk_list_clear_items(_m_eventList, curSel, curSel + 1);
    curSel = _addToEventListBox(nextEventNum);
    setCurrentSelection(_m_eventList, curSel);
}
