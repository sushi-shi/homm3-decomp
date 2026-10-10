// TownPropsTimedEventsPage.h - the timed events page of the town property
// sheet (TownPropsTimedEventsPage.cpp; Loki h3maped object 96): the
// town's events by first day, edited with the town event sheet. Layout
// from the image: the buttons and list from 0x8c, the maps, the town's
// reference, the original town and the modified flag from 0x230, the
// players present at 0x248 and the edited events at 0x24c (0x25c bytes,
// the sheet's new).
#ifndef HOMM3_EDITOR_TOWNPROPSTIMEDEVENTSPAGE_H
#define HOMM3_EDITOR_TOWNPROPSTIMEDEVENTSPAGE_H

#include <vector>

#include "editor/MapObjectRef.h"
#include "editor/Player.h"
#include "editor/resource.h"
#include "editor/Town.h"

class TGameMap;

class TTownPropsTimedEventsPage : public CPropertyPage {
public:
    enum { s_kMaxNumEvents = 500 };

    TTownPropsTimedEventsPage(const TGameMap& oldMap, TGameMap& newMap, bool bSecondLayer, unsigned int objID);
    virtual ~TTownPropsTimedEventsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_TOWN_PROPS_TIMED_EVENTS };
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
    const TTown* _getOldTown() const;
    TTown* _getNewTown();

    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    bool _m_bSecondLayer;
    unsigned int _m_objectID;
    const TTown* _m_pOldTown;
    bool _m_bModified;
    TPlayerMask _m_playersPresent;
    std::vector<TTown::TTimedEvent> _m_events;
};

#endif  /* HOMM3_EDITOR_TOWNPROPSTIMEDEVENTSPAGE_H */
