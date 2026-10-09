// TownPropsTimedEventsPage.cpp - the timed events page of the town
// property sheet (h3maped 0x4c7653..0x4c85b2; Loki h3maped object 96). It
// works as the map specifications' events page does, with the town event
// sheet; its selection-cancel handler folds with that page's.
#include "editor/stdafx.h"

#include <algorithm>

#include "va.h"
#include "editor/EditTownEventSheet.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/TownPropsTimedEventsPage.h"

VA(0x004c7827, 0x15f)
TTownPropsTimedEventsPage::TTownPropsTimedEventsPage(const TGameMap& oldMap, TGameMap& newMap, TMapObjectRef townRef)
    : CPropertyPage(TTownPropsTimedEventsPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_townRef(townRef),
      _m_pOldTown(_getOldTown()),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kTimedEventsPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
    for (unsigned int player = 0; player < kNumPlayers; player++)
        _m_playersPresent[player] = _m_newMap.isPlayerPresent(TPlayer(player));
}

VA_COMPGEN(0x004c7986, 0x1c, SCALAR_DELETING_DTOR, TTownPropsTimedEventsPage)

VA(0x004c79a2, 0xaa)
TTownPropsTimedEventsPage::~TTownPropsTimedEventsPage()
{
}

VA(0x004c7a4c, 0xf2)
int TTownPropsTimedEventsPage::_addToEventList(unsigned int eventNum)
{
    const TTown::TTimedEvent& event = _m_events[eventNum];
    unsigned int firstOccurence = event.getFirstOccurence();
    int pos;
    for (pos = _m_eventsList.GetCount(); pos > 0; pos--) {
        unsigned int prevEventNum = _m_eventsList.GetItemData(pos - 1);
        unsigned int prevFirstOccurence = _m_events[prevEventNum].getFirstOccurence();
        if (firstOccurence > prevFirstOccurence
            || firstOccurence == prevFirstOccurence && eventNum > prevEventNum)
            break;
    }
    CString name;
    name.Format(kTimedEventNameFmtStr, firstOccurence + 1, event.getName().c_str());
    int index = _m_eventsList.InsertString(pos, name);
    _m_eventsList.SetItemData(index, eventNum);
    return index;
}

VA(0x004c7b3e, 0x42)
const TTown* TTownPropsTimedEventsPage::_getOldTown() const
{
    const TGameMap::TLayer& layer = _m_oldMap.getLayer(_m_townRef.getBSecondLayer());
    return dynamic_cast<const TTown*>(layer.getPObject(_m_townRef.getObjectID()));
}

VA(0x004c7b80, 0x42)
TTown* TTownPropsTimedEventsPage::_getNewTown()
{
    TGameMap::TLayer& layer = _m_newMap.getLayer(_m_townRef.getBSecondLayer());
    return dynamic_cast<TTown*>(layer.getPObject(_m_townRef.getObjectID()));
}

VA(0x004c7bc2, 0x8b)
void TTownPropsTimedEventsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_EVENTS_MOVE_DOWN_BUTTON, _m_moveDownButton);
    DDX_Control(pDX, IDC_EVENTS_MOVE_UP_BUTTON, _m_moveUpButton);
    DDX_Control(pDX, IDC_EVENTS_REMOVE_ALL_BUTTON, _m_removeAllButton);
    DDX_Control(pDX, IDC_EVENTS_REMOVE_BUTTON, _m_removeButton);
    DDX_Control(pDX, IDC_EVENTS_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_EVENTS_ADD_BUTTON, _m_addButton);
    DDX_Control(pDX, IDC_TOWN_EVENTS_LIST, _m_eventsList);
}

VA(0x004c7c4d, 0x6)
BEGIN_MESSAGE_MAP(TTownPropsTimedEventsPage, CPropertyPage)
    ON_BN_CLICKED(IDC_EVENTS_ADD_BUTTON, OnAddEventButton)
    ON_BN_CLICKED(IDC_EVENTS_EDIT_BUTTON, OnEditEventButton)
    ON_BN_CLICKED(IDC_EVENTS_REMOVE_BUTTON, OnRemoveEventButton)
    ON_BN_CLICKED(IDC_EVENTS_REMOVE_ALL_BUTTON, OnRemoveAllEventButton)
    ON_LBN_SELCHANGE(IDC_TOWN_EVENTS_LIST, OnSelChangeEventsList)
    ON_LBN_SELCANCEL(IDC_TOWN_EVENTS_LIST, OnSelCancelEventsList)
    ON_LBN_DBLCLK(IDC_TOWN_EVENTS_LIST, OnDblClkEventsList)
    ON_WM_VKEYTOITEM()
    ON_BN_CLICKED(IDC_EVENTS_MOVE_UP_BUTTON, OnMoveUpButton)
    ON_BN_CLICKED(IDC_EVENTS_MOVE_DOWN_BUTTON, OnMoveDownButton)
END_MESSAGE_MAP()

VA(0x004c7c53, 0x193)
BOOL TTownPropsTimedEventsPage::OnInitDialog()
{
    GetDlgItem(IDC_EVENTS_STATIC)->SetWindowText(STownPropsTimedEventsPageText::kEventsStaticStr);
    GetDlgItem(IDC_EVENTS_ADD_BUTTON)->SetWindowText(STownPropsTimedEventsPageText::kAddButtonStr);
    GetDlgItem(IDC_EVENTS_EDIT_BUTTON)->SetWindowText(STownPropsTimedEventsPageText::kEditButtonStr);
    GetDlgItem(IDC_EVENTS_REMOVE_BUTTON)->SetWindowText(STownPropsTimedEventsPageText::kRemoveButtonStr);
    GetDlgItem(IDC_EVENTS_REMOVE_ALL_BUTTON)->SetWindowText(STownPropsTimedEventsPageText::kRemoveAllButtonStr);
    GetDlgItem(IDC_EVENTS_MOVE_UP_BUTTON)->SetWindowText(STownPropsTimedEventsPageText::kMoveUpButtonStr);
    GetDlgItem(IDC_EVENTS_MOVE_DOWN_BUTTON)->SetWindowText(STownPropsTimedEventsPageText::kMoveDownButtonStr);
    _m_bModified = false;
    const TTown* pNewTown = _getNewTown();
    _m_events = pNewTown->getTimedEvents();
    CPropertyPage::OnInitDialog();
    for (unsigned int i = 0; i < _m_events.size(); i++)
        _addToEventList(i);
    _m_addButton.EnableWindow(_m_events.size() < s_kMaxNumEvents);
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_removeAllButton.EnableWindow(_m_events.size() > 0);
    _m_moveUpButton.EnableWindow(FALSE);
    _m_moveDownButton.EnableWindow(FALSE);
    return TRUE;
}

VA(0x004c7de6, 0x54)
void TTownPropsTimedEventsPage::OnOK()
{
    CPropertyPage::OnOK();
    TTown* pNewTown = _getNewTown();
    pNewTown->setTimedEvents(_m_events);
    _m_bModified = _m_bModified || pNewTown->getTimedEvents() != _m_pOldTown->getTimedEvents();
}

VA(0x004c7e3a, 0x140)
void TTownPropsTimedEventsPage::OnAddEventButton()
{
    TEditTownEventSheet sheet(this, _m_playersPresent, _getNewTown()->getTownType(), _m_newMap.getVersion(),
                              TTown::TTimedEvent());
    if (sheet.DoModal() == IDOK) {
        unsigned int newEventNum = _m_events.size();
        _m_events.push_back(sheet.getEvent());
        _m_eventsList.SetCurSel(_addToEventList(newEventNum));
        OnSelChangeEventsList();
        if (_m_eventsList.GetCount() >= s_kMaxNumEvents)
            _m_addButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(TRUE);
    }
}

VA(0x004c7f98, 0x115)
void TTownPropsTimedEventsPage::OnEditEventButton()
{
    int curSel = _m_eventsList.GetCurSel();
    unsigned int eventNum = _m_eventsList.GetItemData(curSel);
    TEditTownEventSheet sheet(this, _m_playersPresent, _getNewTown()->getTownType(), _m_newMap.getVersion(),
                              _m_events[eventNum]);
    if (sheet.DoModal() == IDOK) {
        _m_events[eventNum] = sheet.getEvent();
        _m_eventsList.DeleteString(curSel);
        _m_eventsList.SetCurSel(_addToEventList(eventNum));
        OnSelChangeEventsList();
    }
}

VA(0x004c80ad, 0x156)
void TTownPropsTimedEventsPage::OnRemoveEventButton()
{
    int curSel = _m_eventsList.GetCurSel();
    unsigned int eventNum = _m_eventsList.GetItemData(curSel);
    _m_events.erase(_m_events.begin() + eventNum);
    _m_eventsList.DeleteString(curSel);
    int count = _m_eventsList.GetCount();
    if (count > 0) {
        if (count < s_kMaxNumEvents)
            _m_addButton.EnableWindow(TRUE);
        for (int i = 0; i < count; i++) {
            unsigned int itemEventNum = _m_eventsList.GetItemData(i);
            if (itemEventNum > eventNum)
                _m_eventsList.SetItemData(i, itemEventNum - 1);
        }
        _m_eventsList.SetCurSel(curSel < count ? curSel : count - 1);
        OnSelChangeEventsList();
    } else {
        _m_addButton.EnableWindow(TRUE);
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(FALSE);
        _m_moveUpButton.EnableWindow(FALSE);
        _m_moveDownButton.EnableWindow(FALSE);
        GotoDlgCtrl(&_m_addButton);
    }
}

VA(0x004c8203, 0x55)
void TTownPropsTimedEventsPage::OnRemoveAllEventButton()
{
    int count = _m_eventsList.GetCount();
    if (_m_eventsList.GetCurSel() == -1)
        _m_eventsList.SetCurSel(0);
    for (; count > 0; count--)
        OnRemoveEventButton();
}

VA(0x004c8258, 0xe1)
void TTownPropsTimedEventsPage::OnSelChangeEventsList()
{
    _m_editButton.EnableWindow(TRUE);
    _m_removeButton.EnableWindow(TRUE);
    int count = _m_eventsList.GetCount();
    int curSel = _m_eventsList.GetCurSel();
    unsigned int eventNum = _m_eventsList.GetItemData(curSel);
    if (curSel > 0)
        _m_moveUpButton.EnableWindow(_m_events[_m_eventsList.GetItemData(curSel - 1)].getFirstOccurence()
                                     == _m_events[eventNum].getFirstOccurence());
    else
        _m_moveUpButton.EnableWindow(FALSE);
    if (curSel < count - 1)
        _m_moveDownButton.EnableWindow(_m_events[_m_eventsList.GetItemData(curSel + 1)].getFirstOccurence()
                                       == _m_events[eventNum].getFirstOccurence());
    else
        _m_moveDownButton.EnableWindow(FALSE);
}

void TTownPropsTimedEventsPage::OnSelCancelEventsList()
{
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_moveUpButton.EnableWindow(FALSE);
    _m_moveDownButton.EnableWindow(FALSE);
}

VA(0x004c8339, 0x5)
void TTownPropsTimedEventsPage::OnDblClkEventsList()
{
    OnEditEventButton();
}

VA(0x004c833e, 0x47)
int TTownPropsTimedEventsPage::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    if (nKey == VK_DELETE && _m_eventsList.GetCurSel() != -1) {
        OnRemoveEventButton();
        return -2;
    }
    return CPropertyPage::OnVKeyToItem(nKey, pListBox, nIndex);
}

VA(0x004c8385, 0xb7)
void TTownPropsTimedEventsPage::OnMoveUpButton()
{
    int curSel = _m_eventsList.GetCurSel();
    unsigned int eventNum = _m_eventsList.GetItemData(curSel);
    unsigned int prevEventNum = _m_eventsList.GetItemData(curSel - 1);
    swap(_m_events[eventNum], _m_events[prevEventNum]);
    _m_eventsList.SetItemData(curSel - 1, eventNum);
    _m_eventsList.DeleteString(curSel);
    _m_eventsList.SetCurSel(_addToEventList(prevEventNum));
    OnSelChangeEventsList();
}

VA(0x004c843c, 0xc6)
void TTownPropsTimedEventsPage::OnMoveDownButton()
{
    int count = _m_eventsList.GetCount();
    int curSel = _m_eventsList.GetCurSel();
    unsigned int eventNum = _m_eventsList.GetItemData(curSel);
    unsigned int nextEventNum = _m_eventsList.GetItemData(curSel + 1);
    swap(_m_events[eventNum], _m_events[nextEventNum]);
    _m_eventsList.SetItemData(curSel + 1, eventNum);
    _m_eventsList.DeleteString(curSel);
    _m_eventsList.SetCurSel(_addToEventList(nextEventNum));
    OnSelChangeEventsList();
}
