// TownPropsTimedEventsPage.h - the timed events page of the town property
// sheet (Loki TownPropsTimedEventsPage.cpp): a glade list of the town's
// events ordered by first occurrence, with add, edit, remove, remove-all,
// move-up and move-down buttons over TEditTownEventSheet. Layout from the
// image: the six buttons and the list (_m_eventList, the asserts' name),
// the town, the players present, the modified flag, the edited events
// (_m_events) and the events OnOK publishes, the event shown at each list
// position, then the vtable pointer (OnOK, OnInitDialog). The other member
// names and the 1024-entry size of the position array (0x40..0x1040) are
// not proven.
#ifndef HOMM3_EDITOR_TOWNPROPSTIMEDEVENTSPAGE_H
#define HOMM3_EDITOR_TOWNPROPSTIMEDEVENTSPAGE_H

#include "editor/stdafx.h"

#include <vector>

#include "editor/Player.h"
#include "editor/Town.h"

class TTownPropsTimedEventsPage {
public:
    TTownPropsTimedEventsPage(const TTown& town, const TPlayerMask& playersPresent);
    ~TTownPropsTimedEventsPage();

    virtual void OnOK();
    virtual BOOL OnInitDialog();

    void OnAddEventButton();
    void OnEditEventButton();
    void OnRemoveEventButton();
    void OnRemoveAllEventButton();
    void OnSelChangeEventList();
    void OnSelCancelEventList();
    void OnDblClkEventList();
    void OnMoveUpButton();
    void OnMoveDownButton();

    const vector<TTown::TTimedEvent>& getTimedEvents() const { return _m_newEvents; }
    bool wasModified() const { return _m_bModified; }

private:
    int _addToEventListBox(unsigned int eventNum);

    GtkButton* _m_moveDownButton;
    GtkButton* _m_moveUpButton;
    GtkButton* _m_removeAllButton;
    GtkButton* _m_removeButton;
    GtkButton* _m_editButton;
    GtkButton* _m_addButton;
    GtkList* _m_eventList;
    const TTown& _m_town;
    TPlayerMask _m_playersPresent;
    bool _m_bModified;
    vector<TTown::TTimedEvent> _m_events;
    vector<TTown::TTimedEvent> _m_newEvents;
    unsigned int _m_aEventNum[1024];
};

#endif  /* HOMM3_EDITOR_TOWNPROPSTIMEDEVENTSPAGE_H */
