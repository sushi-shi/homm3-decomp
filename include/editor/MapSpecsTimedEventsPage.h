// MapSpecsTimedEventsPage.h - the timed events page of the map
// specifications sheet (MapSpecsTimedEventsPage.cpp; Loki h3maped object
// 75). The list shows the events by first day, then by index; each item
// keeps its event's index. Layout from the image: the move, remove, edit
// and add buttons and the list from 0x8c, the map before and during the
// sheet at 0x230 and 0x234, the modified flag and the events at 0x23c.
#ifndef HOMM3_EDITOR_MAPSPECSTIMEDEVENTSPAGE_H
#define HOMM3_EDITOR_MAPSPECSTIMEDEVENTSPAGE_H

#include <vector>

#include "editor/resource.h"
#include "editor/TimedEvent.h"

class TGameMap;

class TMapSpecsTimedEventsPage : public CPropertyPage {
public:
    // At most this many events (Loki's port allowed 50).
    enum { s_kMaxNumEvents = 500 };

    TMapSpecsTimedEventsPage(const TGameMap& oldMap, TGameMap& newMap);
    virtual ~TMapSpecsTimedEventsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MAP_SPECS_TIMED_EVENTS };
    CButton _m_moveDownButton;
    CButton _m_moveUpButton;
    CButton _m_removeAllButton;
    CButton _m_removeButton;
    CButton _m_editButton;
    CButton _m_addButton;
    CListBox _m_eventsList;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnAddEventButton();
    afx_msg void OnEditEventButton();
    afx_msg void OnRemoveEventButton();
    afx_msg void OnRemoveAllEventButton();
    afx_msg void OnSelChangeEventsList();
    afx_msg void OnSelCancelEventsList();
    afx_msg void OnDblClkEventsList();
    afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
    afx_msg void OnMoveUpButton();
    afx_msg void OnMoveDownButton();
    DECLARE_MESSAGE_MAP()

private:
    int _addToEventList(unsigned int eventNum);

    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    bool _m_bModified;
    std::vector<TTimedEvent> _m_events;
};

#endif  /* HOMM3_EDITOR_MAPSPECSTIMEDEVENTSPAGE_H */
