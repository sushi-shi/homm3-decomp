// MapSpecsTimedEventsPage.cpp - the timed events page of the map
// specifications sheet (h3maped 0x477e53..0x478d43; Loki h3maped object
// 75). Only neighbours that occur on the same day can be reordered; the
// Delete key removes the selected event.
#include "editor/stdafx.h"

#include <algorithm>

#include "va.h"
#include "editor/EditTimedEventSheet.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsTimedEventsPage.h"

namespace {

VA(0x00478027, 0x39)
TPlayerMask getPlayerPresentMask(const TGameMap& gameMap)
{
    TPlayerMask mask;
    for (unsigned int player = 0; player < kNumPlayers; player++)
        mask[player] = gameMap.isPlayerPresent(TPlayer(player));
    return mask;
}

}

VA(0x00478060, 0x114)
TMapSpecsTimedEventsPage::TMapSpecsTimedEventsPage(const TGameMap& oldMap, TGameMap& newMap)
    : CPropertyPage(TMapSpecsTimedEventsPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kTimedEventsPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00478174, 0x1c, SCALAR_DELETING_DTOR, TMapSpecsTimedEventsPage)

VA(0x00478190, 0xaa)
TMapSpecsTimedEventsPage::~TMapSpecsTimedEventsPage()
{
}

VA(0x0047823a, 0xf2)
int TMapSpecsTimedEventsPage::_addToEventList(unsigned int eventNum)
{
    const TTimedEvent& event = _m_events[eventNum];
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

VA(0x0047832c, 0x8b)
void TMapSpecsTimedEventsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_EVENTS_MOVE_DOWN_BUTTON, _m_moveDownButton);
    DDX_Control(pDX, IDC_EVENTS_MOVE_UP_BUTTON, _m_moveUpButton);
    DDX_Control(pDX, IDC_EVENTS_REMOVE_ALL_BUTTON, _m_removeAllButton);
    DDX_Control(pDX, IDC_EVENTS_REMOVE_BUTTON, _m_removeButton);
    DDX_Control(pDX, IDC_EVENTS_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_EVENTS_ADD_BUTTON, _m_addButton);
    DDX_Control(pDX, IDC_EVENTS_LIST, _m_eventsList);
}

VA(0x004783b7, 0x6)
BEGIN_MESSAGE_MAP(TMapSpecsTimedEventsPage, CPropertyPage)
    ON_BN_CLICKED(IDC_EVENTS_ADD_BUTTON, OnAddEventButton)
    ON_BN_CLICKED(IDC_EVENTS_EDIT_BUTTON, OnEditEventButton)
    ON_BN_CLICKED(IDC_EVENTS_REMOVE_BUTTON, OnRemoveEventButton)
    ON_BN_CLICKED(IDC_EVENTS_REMOVE_ALL_BUTTON, OnRemoveAllEventButton)
    ON_LBN_SELCHANGE(IDC_EVENTS_LIST, OnSelChangeEventsList)
    ON_LBN_SELCANCEL(IDC_EVENTS_LIST, OnSelCancelEventsList)
    ON_LBN_DBLCLK(IDC_EVENTS_LIST, OnDblClkEventsList)
    ON_WM_VKEYTOITEM()
    ON_BN_CLICKED(IDC_EVENTS_MOVE_UP_BUTTON, OnMoveUpButton)
    ON_BN_CLICKED(IDC_EVENTS_MOVE_DOWN_BUTTON, OnMoveDownButton)
END_MESSAGE_MAP()

VA(0x004783bd, 0x197)
BOOL TMapSpecsTimedEventsPage::OnInitDialog()
{
    GetDlgItem(IDC_EVENTS_STATIC)->SetWindowText(SMapSpecsTimedEventsPageText::kEventsStaticStr);
    GetDlgItem(IDC_EVENTS_ADD_BUTTON)->SetWindowText(SMapSpecsTimedEventsPageText::kAddButtonStr);
    GetDlgItem(IDC_EVENTS_EDIT_BUTTON)->SetWindowText(SMapSpecsTimedEventsPageText::kEditButtonStr);
    GetDlgItem(IDC_EVENTS_REMOVE_BUTTON)->SetWindowText(SMapSpecsTimedEventsPageText::kRemoveButtonStr);
    GetDlgItem(IDC_EVENTS_REMOVE_ALL_BUTTON)->SetWindowText(SMapSpecsTimedEventsPageText::kRemoveAllButtonStr);
    GetDlgItem(IDC_EVENTS_MOVE_UP_BUTTON)->SetWindowText(SMapSpecsTimedEventsPageText::kMoveUpButtonStr);
    GetDlgItem(IDC_EVENTS_MOVE_DOWN_BUTTON)->SetWindowText(SMapSpecsTimedEventsPageText::kMoveDownButtonStr);
    _m_bModified = false;
    _m_events = _m_newMap.getTimedEvents();
    CPropertyPage::OnInitDialog();
    for (unsigned int i = 0; i < _m_events.size(); i++)
        _addToEventList(i);
    _m_eventsList.SetCurSel(-1);
    _m_addButton.EnableWindow(_m_eventsList.GetCount() < s_kMaxNumEvents);
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_removeAllButton.EnableWindow(_m_eventsList.GetCount() > 0);
    _m_moveUpButton.EnableWindow(FALSE);
    _m_moveDownButton.EnableWindow(FALSE);
    return TRUE;
}

VA(0x00478554, 0x58)
void TMapSpecsTimedEventsPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_newMap.setTimedEvents(_m_events);
    _m_bModified = _m_bModified || _m_newMap.getTimedEvents() != _m_oldMap.getTimedEvents();
}

VA(0x004785ac, 0x132)
void TMapSpecsTimedEventsPage::OnAddEventButton()
{
    TEditTimedEventSheet sheet(this, getPlayerPresentMask(_m_newMap), _m_newMap.getVersion(), TTimedEvent());
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

VA(0x004786fc, 0x10b)
void TMapSpecsTimedEventsPage::OnEditEventButton()
{
    int curSel = _m_eventsList.GetCurSel();
    unsigned int eventNum = _m_eventsList.GetItemData(curSel);
    TEditTimedEventSheet sheet(this, getPlayerPresentMask(_m_newMap), _m_newMap.getVersion(), _m_events[eventNum]);
    if (sheet.DoModal() == IDOK) {
        _m_events[eventNum] = sheet.getEvent();
        _m_eventsList.DeleteString(curSel);
        _m_eventsList.SetCurSel(_addToEventList(eventNum));
        OnSelChangeEventsList();
    }
}

VA(0x00478807, 0x156)
void TMapSpecsTimedEventsPage::OnRemoveEventButton()
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

VA(0x0047895d, 0x53)
void TMapSpecsTimedEventsPage::OnRemoveAllEventButton()
{
    if (_m_eventsList.GetCurSel() == -1)
        _m_eventsList.SetCurSel(0);
    for (int count = _m_eventsList.GetCount(); count > 0; count--)
        OnRemoveEventButton();
}

VA(0x004789b0, 0xe1)
void TMapSpecsTimedEventsPage::OnSelChangeEventsList()
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

VA(0x00478a91, 0x39)
void TMapSpecsTimedEventsPage::OnSelCancelEventsList()
{
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_moveUpButton.EnableWindow(FALSE);
    _m_moveDownButton.EnableWindow(FALSE);
}

VA(0x00478aca, 0x5)
void TMapSpecsTimedEventsPage::OnDblClkEventsList()
{
    OnEditEventButton();
}

VA(0x00478acf, 0x47)
int TMapSpecsTimedEventsPage::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    if (nKey == VK_DELETE && _m_eventsList.GetCurSel() != -1) {
        OnRemoveEventButton();
        return -2;
    }
    return CPropertyPage::OnVKeyToItem(nKey, pListBox, nIndex);
}

VA(0x00478b16, 0xb7)
void TMapSpecsTimedEventsPage::OnMoveUpButton()
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

VA(0x00478bcd, 0xc6)
void TMapSpecsTimedEventsPage::OnMoveDownButton()
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
