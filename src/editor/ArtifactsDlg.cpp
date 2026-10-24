// ArtifactsDlg.cpp - the artifacts list of the hero and hero prototype
// artifacts pages (h3maped 0x404739..0x406562; GOG only). Each item's data
// packs the artifact into the low word and its location into the high
// word: a slot, or the backpack. Placing a combination artifact counts,
// per slot class, the slots its other components need; the artifact
// editor keeps them free.
#include "editor/stdafx.h"

#include <algorithm>
#include <functional>

#include "artifact.h"
#include "const_bitset_iterator.h"
#include "exceptions.h"
#include "va.h"
#include "editor/ArtifactsDlg.h"
#include "editor/EditArtifactDlg.h"
#include "editor/MapEditorText.h"

namespace {

// The slots the list shows: the worn ones but the catapult and the
// spellbook, which the check stands for.
DATA(0x005306a0)
const TArtifactSlot akListedSlots[] = {
    eArtifactSlotHead,      eArtifactSlotShoulders,  eArtifactSlotNeck,        eArtifactSlotRightHand,
    eArtifactSlotLeftHand,  eArtifactSlotTorso,      eArtifactSlotRightRing,   eArtifactSlotLeftRing,
    eArtifactSlotFeet,      eArtifactSlotMisc1,      eArtifactSlotMisc2,       eArtifactSlotMisc3,
    eArtifactSlotMisc4,     TArtifactSlot(kNumArtifactSlots), eArtifactSlotWarMachine1, eArtifactSlotWarMachine2,
    eArtifactSlotWarMachine3
};

class TListedSlots : public TArtifactsDlg::TSlots {
public:
    TListedSlots();
    ~TListedSlots() {}
};

VA(0x0040490d, 0x29)
TListedSlots::TListedSlots()
{
    for (const TArtifactSlot* pSlot = akListedSlots; pSlot < akListedSlots + sizeof(akListedSlots) / sizeof(akListedSlots[0]); ++pSlot)
        set(*pSlot);
}

// Releases a string's buffer, which a list item's text holds, on leaving
// the scope.
class TBufferRelease {
public:
    explicit TBufferRelease(CString& string) : _m_string(string) {}
    ~TBufferRelease() { _m_string.ReleaseBuffer(); }

private:
    CString& _m_string;
};

const char* getLocationName(TArtifactSlot slot)
{
    return slot < TArtifactsDlg::s_kBackpackSlot ? akArtifactSlotTraits[slot].m_name : kBackpackStr;
}

// Sorts by the clicked column, then by the other one.
VA(0x00404936, 0x8d)
int CALLBACK compareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    TArtifact artifact1 = TArtifact(LOWORD(lParam1));
    TArtifactSlot slot1 = TArtifactSlot(HIWORD(lParam1));
    TArtifact artifact2 = TArtifact(LOWORD(lParam2));
    TArtifactSlot slot2 = TArtifactSlot(HIWORD(lParam2));
    const char* name1 = akArtifactTraits[artifact1].m_name;
    const char* name2 = akArtifactTraits[artifact2].m_name;
    const char* location1 = getLocationName(slot1);
    const char* location2 = getLocationName(slot2);
    int result;
    if (lParamSort == 0) {
        result = lstrcmpi(name1, name2);
        if (result == 0)
            result = lstrcmpi(location1, location2);
    } else {
        result = lstrcmpi(location1, location2);
        if (result == 0)
            result = lstrcmpi(name1, name2);
    }
    return result;
}

const TArtifactsDlg::TSlots& getListedSlots();

}

VA(0x004049c3, 0x11d)
TArtifactsDlg::TArtifactsDlg(CWnd* pParent, EGameVersion mapVersion)
    : CDialog(TArtifactsDlg::IDD, pParent),
      _m_mapVersion(mapVersion),
      _m_slotClassCounts(0)
{
    if (!Create(TArtifactsDlg::IDD, pParent))
        throw TRuntimeError();
}

VA_COMPGEN(0x00404b32, 0x1c, SCALAR_DELETING_DTOR, TArtifactsDlg)
VA_COMPGEN(0x00404b7b, 0x9c, IMPLICIT_DTOR, TArtifactsDlg)

VA(0x00404c17, 0x169)
void TArtifactsDlg::setArtifacts(const THeroPrototype::TArtifactContainer& artifacts)
{
    OnRemoveAllButton();
    int numSlots = _m_mapVersion >= GAME_VERSION_SOD ? kNumArtifactSlots + 1 : kNumArtifactSlots;
    for (int slot = 0; slot < numSlots; slot++) {
        if (getListedSlots()[slot]) {
            TArtifact artifact = artifacts.getSlot(TArtifactSlot(slot));
            if (artifact != ARTIFACT_NONE)
                _insertItem(_m_list.GetItemCount(), artifact, TArtifactSlot(slot));
        }
    }
    for (std::multiset<TArtifact>::const_iterator pArtifact = artifacts.getBackpack().begin();
         pArtifact != artifacts.getBackpack().end(); ++pArtifact) {
        TArtifact artifact = *pArtifact;
        _insertItem(_m_list.GetItemCount(), artifact, TArtifactSlot(s_kBackpackSlot));
    }
    if (IsWindowEnabled() && _m_unusedSlots.none() && _m_artifacts.getBackpack().size() >= s_kMaxBackpackSize)
        _m_addButton.EnableWindow(FALSE);
    if (_m_list.GetItemCount() > 0) {
        _m_list.SortItems(compareItems, 0);
        if (IsWindowEnabled())
            _m_removeAllButton.EnableWindow(TRUE);
    }
    _m_artifacts.setSlot(eArtifactSlotSpellbook, artifacts.getSlot(eArtifactSlotSpellbook));
    _m_hasSpellbookCheck.SetCheck(_m_artifacts.getSlot(eArtifactSlotSpellbook) == ARTIFACT_SPELLBOOK);
}

namespace {

VA(0x00404d80, 0x2c)
const TArtifactsDlg::TSlots& getListedSlots()
{
    DATA_COMPGEN_GUARD(0x005aa701, listedSlotsGuard, slots)
    // The atexit argument at 0x404d9d is the shared bare-ret body at
    // 0x45f6da; this empty local-static cleanup is folded with destructors.
    VA_COMPGEN(0x0045f6da, 0x1, STATIC_DTOR, slots)
    DATA(0x005aa704) static const TListedSlots slots;
    return slots;
}

}

VA(0x00404dac, 0x6d)
void TArtifactsDlg::_enableControls()
{
    _m_hasSpellbookCheck.EnableWindow(TRUE);
    if (_m_unusedSlots.any() || _m_artifacts.getBackpack().size() < s_kMaxBackpackSize)
        _m_addButton.EnableWindow(TRUE);
    if (_m_list.GetItemCount() > 0)
        _m_removeAllButton.EnableWindow(TRUE);
    _m_list.EnableWindow(TRUE);
}

VA(0x00404e19, 0x77)
void TArtifactsDlg::_disableControls()
{
    _m_list.EnableWindow(FALSE);
    _m_hasSpellbookCheck.EnableWindow(FALSE);
    _m_addButton.EnableWindow(FALSE);
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_removeAllButton.EnableWindow(FALSE);
    int index = _m_list.GetNextItem(-1, LVNI_SELECTED);
    if (index != -1)
        _m_list.SetItemState(index, 0, LVIS_SELECTED);
}

// Keep the requested list position separate from InsertItem's result.
// Temporary end iterators preserve the component search's value lifetimes.
VA(0x00404e90, 0x271)
int TArtifactsDlg::_insertItem(int index, TArtifact artifact, TArtifactSlot slot)
{
    CString text;
    LVITEM item;
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = index;
    item.iSubItem = 0;
    item.lParam = MAKELONG(artifact, slot);
    text = akArtifactTraits[artifact].m_name;
    item.pszText = text.GetBuffer(0);
    int insertedIndex;
    {
        TBufferRelease release(text);
        insertedIndex = _m_list.InsertItem(&item);
        if (insertedIndex == -1)
            return -1;
    }
    try {
        item.mask = LVIF_TEXT;
        item.iItem = insertedIndex;
        item.iSubItem = 1;
        text = getLocationName(slot);
        item.pszText = text.GetBuffer(0);
        {
            TBufferRelease release(text);
            if (!_m_list.SetItem(&item)) {
                _m_list.DeleteItem(insertedIndex);
                return -1;
            }
        }
        if (slot >= s_kBackpackSlot)
            _m_artifacts.getBackpack().insert(artifact);
        else {
            _m_artifacts.setSlot(slot, artifact);
            try {
                int combination = akArtifactTraits[artifact].m_comboType;
                if (combination != -1) {
                    const std::bitset<ARTIFACT_COUNT>& components = g_combinationArtifacts[combination].m_components;
                    TConstBitsetIterator<ARTIFACT_COUNT> current(components, 0);
                    for (;;) {
                        current = std::find_if(
                            current, TConstBitsetIterator<ARTIFACT_COUNT>(components, ARTIFACT_COUNT),
                            std::bind2nd(std::not_equal_to<bool>(), false));
                        if (!(current != TConstBitsetIterator<ARTIFACT_COUNT>(components, ARTIFACT_COUNT)))
                            break;
                        ++_m_slotClassCounts[akArtifactTraits[current.position()].m_allowableSlotMask];
                        ++current;
                    }
                    --_m_slotClassCounts[akArtifactTraits[artifact].m_allowableSlotMask];
                }
                _updateUnusedSlots();
            } catch (...) {
                _m_artifacts.setSlot(slot, ARTIFACT_NONE);
                throw;
            }
        }
    } catch (...) {
        _m_list.DeleteItem(insertedIndex);
        throw;
    }
    return insertedIndex;
}

VA(0x0040510b, 0x113)
void TArtifactsDlg::_removeItem(int index)
{
    DWORD data = _m_list.GetItemData(index);
    TArtifact artifact = TArtifact(LOWORD(data));
    TArtifactSlot slot = TArtifactSlot(HIWORD(data));
    _m_list.DeleteItem(index);
    if (slot >= s_kBackpackSlot)
        _m_artifacts.getBackpack().erase(_m_artifacts.getBackpack().find(artifact));
    else {
        int combination = akArtifactTraits[artifact].m_comboType;
        if (combination != -1) {
            ++_m_slotClassCounts[akArtifactTraits[artifact].m_allowableSlotMask];
            const std::bitset<ARTIFACT_COUNT>& components = g_combinationArtifacts[combination].m_components;
            TConstBitsetIterator<ARTIFACT_COUNT> current(components, 0);
            TConstBitsetIterator<ARTIFACT_COUNT> end(components, ARTIFACT_COUNT);
            for (; (current = std::find_if(current, end, std::bind2nd(std::not_equal_to<bool>(), false))) != end;
                 ++current)
                --_m_slotClassCounts[akArtifactTraits[current.position()].m_allowableSlotMask];
        }
        _m_artifacts.setSlot(slot, ARTIFACT_NONE);
        _updateUnusedSlots();
    }
}

// A slot is unused when no artifact takes it and the worn combination
// artifacts' other components do not need every remaining slot of its
// class.
VA(0x0040521e, 0xcd)
void TArtifactsDlg::_updateUnusedSlots()
{
    _m_unusedSlots = getListedSlots();
    int i;
    for (i = 0; i < kNumArtifactSlots + 1; i++) {
        if (_m_artifacts.getSlot(TArtifactSlot(i)) != ARTIFACT_NONE)
            _m_unusedSlots.set(i, false);
    }
    for (i = 0; i < s_kNumSlotClasses; i++) {
        if (_m_slotClassCounts[i] != 0) {
            TSlots classSlots = g_artifactSlotMasks[i];
            unsigned int numClassSlots = classSlots.count();
            unsigned int numNeeded = _m_slotClassCounts[i];
            TSlots usedSlots = ~_m_unusedSlots & classSlots;
            if (usedSlots.count() + numNeeded == numClassSlots)
                _m_unusedSlots &= ~classSlots;
        }
    }
}

VA(0x004052eb, 0x76)
void TArtifactsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_HAS_SPELLBOOK_CHECK, _m_hasSpellbookCheck);
    DDX_Control(pDX, IDC_ARTIFACTS_LIST, _m_list);
    DDX_Control(pDX, IDC_ARTIFACTS_REMOVE_ALL_BUTTON, _m_removeAllButton);
    DDX_Control(pDX, IDC_ARTIFACTS_REMOVE_BUTTON, _m_removeButton);
    DDX_Control(pDX, IDC_ARTIFACTS_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_ARTIFACTS_ADD_BUTTON, _m_addButton);
}

VA(0x00405361, 0x6)
BEGIN_MESSAGE_MAP(TArtifactsDlg, CDialog)
    ON_BN_CLICKED(IDC_ARTIFACTS_ADD_BUTTON, OnAddButton)
    ON_BN_CLICKED(IDC_ARTIFACTS_EDIT_BUTTON, OnEditButton)
    ON_BN_CLICKED(IDC_ARTIFACTS_REMOVE_BUTTON, OnRemoveButton)
    ON_BN_CLICKED(IDC_ARTIFACTS_REMOVE_ALL_BUTTON, OnRemoveAllButton)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_ARTIFACTS_LIST, OnItemChangedList)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_ARTIFACTS_LIST, OnColumnClickList)
    ON_NOTIFY(NM_DBLCLK, IDC_ARTIFACTS_LIST, OnDblclkList)
    ON_NOTIFY(LVN_KEYDOWN, IDC_ARTIFACTS_LIST, OnKeydownList)
    ON_BN_CLICKED(IDC_HAS_SPELLBOOK_CHECK, OnHasSpellbookCheck)
    ON_WM_ENABLE()
END_MESSAGE_MAP()

VA(0x00405367, 0x19e)
BOOL TArtifactsDlg::OnInitDialog()
{
    GetDlgItem(IDC_HAS_SPELLBOOK_CHECK)->SetWindowText(SHeroPropsArtifactsPageText::kHasSpellbookCheckStr);
    GetDlgItem(IDC_ARTIFACTS_STATIC)->SetWindowText(SHeroPropsArtifactsPageText::kArtifactsStaticStr);
    GetDlgItem(IDC_ARTIFACTS_ADD_BUTTON)->SetWindowText(SHeroPropsArtifactsPageText::kAddButtonStr);
    GetDlgItem(IDC_ARTIFACTS_EDIT_BUTTON)->SetWindowText(SHeroPropsArtifactsPageText::kEditButtonStr);
    GetDlgItem(IDC_ARTIFACTS_REMOVE_BUTTON)->SetWindowText(SHeroPropsArtifactsPageText::kRemoveButtonStr);
    GetDlgItem(IDC_ARTIFACTS_REMOVE_ALL_BUTTON)->SetWindowText(SHeroPropsArtifactsPageText::kRemoveAllButtonStr);
    _m_artifacts = THeroPrototype::TArtifactContainer();
    _m_unusedSlots = getListedSlots();
    std::fill(_m_slotClassCounts.begin(), _m_slotClassCounts.end(), 0);
    CDialog::OnInitDialog();
    _m_list.ModifyStyle(0, LVS_SINGLESEL);
    CRect rect;
    _m_list.GetClientRect(&rect);
    int width = rect.Width() - GetSystemMetrics(SM_CXVSCROLL);
    int nameWidth = 2 * width / 3;
    _m_list.InsertColumn(0, SHeroPropsArtifactsPageText::kNameColumnStr, LVCFMT_LEFT, nameWidth, 0);
    _m_list.InsertColumn(1, SHeroPropsArtifactsPageText::kLocationColumnStr, LVCFMT_LEFT, width - nameWidth, 1);
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_removeAllButton.EnableWindow(FALSE);
    return TRUE;
}

VA(0x0040553f, 0x16)
void TArtifactsDlg::OnEnable(BOOL bEnable)
{
    if (bEnable)
        _enableControls();
    else
        _disableControls();
}

VA(0x00405555, 0xd6)
void TArtifactsDlg::OnAddButton()
{
    TEditArtifactDlg dlg(this, _m_unusedSlots, _m_slotClassCounts,
                         _m_artifacts.getBackpack().size() >= s_kMaxBackpackSize, _m_mapVersion);
    if (dlg.DoModal() == IDOK) {
        TArtifact artifact = dlg.getArtifact();
        TArtifactSlot slot = dlg.getSlot();
        if (_insertItem(_m_list.GetItemCount(), artifact, slot) != -1) {
            if (_m_unusedSlots.none() && _m_artifacts.getBackpack().size() >= s_kMaxBackpackSize)
                _m_addButton.EnableWindow(FALSE);
            _m_removeAllButton.EnableWindow(TRUE);
        }
    }
}

VA(0x00405681, 0xe5)
void TArtifactsDlg::OnEditButton()
{
    int index = _m_list.GetNextItem(-1, LVNI_SELECTED);
    DWORD data = _m_list.GetItemData(index);
    TArtifact artifact = TArtifact(LOWORD(data));
    TArtifactSlot slot = TArtifactSlot(HIWORD(data));
    TArtifact newArtifact = artifact;
    TArtifactSlot newSlot = slot;
    _removeItem(index);
    TEditArtifactDlg dlg(this, _m_unusedSlots, _m_slotClassCounts,
                         _m_artifacts.getBackpack().size() >= s_kMaxBackpackSize, _m_mapVersion, artifact, slot);
    if (dlg.DoModal() == IDOK) {
        newArtifact = dlg.getArtifact();
        newSlot = dlg.getSlot();
    }
    if (_insertItem(index, newArtifact, newSlot) != -1)
        _m_list.SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
}

VA(0x00405766, 0x90)
void TArtifactsDlg::OnRemoveButton()
{
    int index = _m_list.GetNextItem(-1, LVNI_SELECTED);
    _removeItem(index);
    if (_m_unusedSlots.any() || _m_artifacts.getBackpack().size() < s_kMaxBackpackSize)
        _m_addButton.EnableWindow(TRUE);
    int count = _m_list.GetItemCount();
    if (count > 0) {
        if (index >= count - 1)
            index = count - 1;
        _m_list.SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
    } else
        _m_removeAllButton.EnableWindow(FALSE);
}

VA(0x004057f6, 0x53)
void TArtifactsDlg::OnRemoveAllButton()
{
    for (int count = _m_list.GetItemCount(); count > 0; count--)
        _removeItem(0);
    if (IsWindowEnabled()) {
        _m_addButton.EnableWindow(TRUE);
        _m_removeAllButton.EnableWindow(FALSE);
    }
}

VA(0x00405849, 0x8e)
void TArtifactsDlg::OnItemChangedList(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    if (pNMListView->iItem != -1 && pNMListView->iSubItem == 0 && (pNMListView->uChanged & LVIF_STATE)) {
        bool bWasSelected = (pNMListView->uOldState & LVIS_SELECTED) != 0;
        bool bSelected = (pNMListView->uNewState & LVIS_SELECTED) != 0;
        if (bSelected && !bWasSelected) {
            if (IsWindowEnabled()) {
                _m_editButton.EnableWindow(TRUE);
                _m_removeButton.EnableWindow(TRUE);
            } else
                _m_list.SetItemState(pNMListView->iItem, 0, LVIS_SELECTED);
        } else if (!bSelected && bWasSelected) {
            _m_editButton.EnableWindow(FALSE);
            _m_removeButton.EnableWindow(FALSE);
        }
    }
    *pResult = 0;
}

VA(0x004058d7, 0x27)
void TArtifactsDlg::OnColumnClickList(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    _m_list.SortItems(compareItems, pNMListView->iSubItem);
    *pResult = 0;
}

VA(0x004058fe, 0x82)
void TArtifactsDlg::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult)
{
    if (IsWindowEnabled()) {
        int index = _m_list.GetNextItem(-1, LVNI_SELECTED);
        if (index != -1) {
            CRect rect;
            if (_m_list.GetItemRect(index, &rect, LVIR_BOUNDS)) {
                CPoint point;
                GetCursorPos(&point);
                _m_list.ScreenToClient(&point);
                if (rect.PtInRect(point))
                    OnEditButton();
            }
        }
    }
    *pResult = 0;
}

VA(0x00405980, 0x43)
void TArtifactsDlg::OnKeydownList(NMHDR* pNMHDR, LRESULT* pResult)
{
    LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;
    if (IsWindowEnabled() && pLVKeyDow->wVKey == VK_DELETE && _m_list.GetNextItem(-1, LVNI_SELECTED) != -1)
        OnRemoveButton();
    *pResult = 0;
}

VA(0x004059c3, 0x2d)
void TArtifactsDlg::OnHasSpellbookCheck()
{
    _m_artifacts.setSlot(eArtifactSlotSpellbook, _m_hasSpellbookCheck.GetCheck() ? ARTIFACT_SPELLBOOK : ARTIFACT_NONE);
}
