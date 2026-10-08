// MapSpecsTimedEventsPage.h - the map specifications' timed events page
// (Loki MapSpecsTimedEventsPage.cpp): a glade list of the events ordered by
// first occurrence, with add, edit, remove, remove-all, move-up and
// move-down buttons over TEditTimedEventSheet. Not a CWnd: the map, the
// edited events (_m_events, the asserts' name), the events OnOK publishes,
// the modified flag and the open sheet, then the vtable pointer (OnOK,
// OnInitDialog). The other member names are not proven.
#ifndef HOMM3_EDITOR_MAPSPECSTIMEDEVENTSPAGE_H
#define HOMM3_EDITOR_MAPSPECSTIMEDEVENTSPAGE_H

#include "editor/stdafx.h"

#include <vector>

#include "editor/GameMap.h"
#include "editor/TimedEvent.h"

class TEditTimedEventSheet;

class TMapSpecsTimedEventsPage {
public:
    TMapSpecsTimedEventsPage(const TGameMap& map);
    ~TMapSpecsTimedEventsPage();

    virtual void OnOK();
    virtual BOOL OnInitDialog();

    void OnAddEventButton();
    void OnEditEventButton();
    void OnRemoveEventButton();
    void OnRemoveAllEventButton();
    void OnSelChangeEventsListbox();
    void OnSelCancelEventsListbox();
    void OnDblClkEventsListbox();
    void OnMoveUpButton();
    void OnMoveDownButton();

    const vector<TTimedEvent>& getTimedEvents() const { return _m_newEvents; }
    bool wasModified() const { return _m_bModified; }

private:
    int _addToEventListBox(unsigned int eventNum);

    const TGameMap& _m_map;
    vector<TTimedEvent> _m_events;
    vector<TTimedEvent> _m_newEvents;
    bool _m_bModified;
    TEditTimedEventSheet* _m_pEventSheet;
};

#endif  /* HOMM3_EDITOR_MAPSPECSTIMEDEVENTSPAGE_H */
