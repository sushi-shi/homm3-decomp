// TownPropsBuildingsPage.cpp - the buildings page of the town property
// sheet (h3maped 0x4c38e3..0x4c49a7; Loki h3maped object 91). Building an
// item builds what it hangs under and demolishing one demolishes what
// hangs under it; disabling one grays its subtree. Built buildings are
// bold.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/TownPropsBuildingsPage.h"

// The state flags in the high word of a tree item's data.
enum {
    kBuiltFlag = 1,
    kDisabledFlag = 2,
    kGrayedFlag = 4
};

namespace {

VA(0x004c38e3, 0x3d)
void demolishDependentBuildings(const TTown::TBuildingTraits (&akBuildingTraits)[kNumBuildings],
                                TBuilding baseBuilding,
                                TArray<TTown::TBuildingState, kNumBuildings>* pBuildingStates)
{
    for (unsigned int building = 0; building < kNumBuildings; building++) {
        if (akBuildingTraits[building].m_building == baseBuilding && (*pBuildingStates)[building].getBBuilt()) {
            demolishDependentBuildings(akBuildingTraits, TBuilding(building), pBuildingStates);
            (*pBuildingStates)[building].setBBuilt(false);
        }
    }
}

}

VA(0x004c3920, 0x15b)
TTownPropsBuildingsPage::TTownPropsBuildingsPage(const TGameMap& oldMap, TGameMap& newMap, TMapObjectRef townRef)
    : CPropertyPage(TTownPropsBuildingsPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_townRef(townRef),
      _m_pOldTown(_getOldTown()),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kBuildingsPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x004c3a7b, 0x1c, SCALAR_DELETING_DTOR, TTownPropsBuildingsPage)

VA(0x004c3a97, 0xc3)
TTownPropsBuildingsPage::~TTownPropsBuildingsPage()
{
}

VA(0x004c3b5a, 0x9b)
void TTownPropsBuildingsPage::_createTree(TBuilding parentBuilding, HTREEITEM hParent)
{
    const TTown::TBuildingTraits (&akBuildingTraits)[kNumBuildings] =
        _getNewTown()->getTownTypeTraits().m_akBuildingTraits;
    for (unsigned int building = 0; building < kNumBuildings; building++) {
        if (!akBuildingTraits[building].isDisallowed() && akBuildingTraits[building].m_building == parentBuilding) {
            HTREEITEM hItem = _m_buildingTree.InsertItem(TVIF_TEXT, akBuildingTraits[building].m_pName, 0, 0, 0, 0, 0,
                                                         hParent, TVI_LAST);
            if (hItem != NULL) {
                _createTree(TBuilding(building), hItem);
                _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0, MAKELONG(building, 0));
            }
        }
    }
}

VA(0x004c3bf5, 0x35)
void TTownPropsBuildingsPage::_resetTree()
{
    _resetTree(NULL);
    _m_buildingTree.EnsureVisible(_m_buildingTree.GetRootItem());
}

VA(0x004c3c2a, 0x92)
void TTownPropsBuildingsPage::_resetTree(HTREEITEM hParent)
{
    for (HTREEITEM hItem = _m_buildingTree.GetChildItem(hParent); hItem != NULL;
         hItem = _m_buildingTree.GetNextSiblingItem(hItem)) {
        _resetTree(hItem);
        _m_buildingTree.Expand(hItem, TVE_COLLAPSE);
        _m_buildingTree.SetItem(hItem, TVIF_STATE, NULL, 0, 0, 0, TVIS_BOLD, 0);
        _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0, LOWORD(_m_buildingTree.GetItemData(hItem)));
    }
}

VA(0x004c3cbc, 0xb7)
void TTownPropsBuildingsPage::_showBuildingStates(HTREEITEM hParent)
{
    for (HTREEITEM hItem = _m_buildingTree.GetChildItem(hParent); hItem != NULL;
         hItem = _m_buildingTree.GetNextSiblingItem(hItem)) {
        _showBuildingStates(hItem);
        WORD building = LOWORD(_m_buildingTree.GetItemData(hItem));
        WORD stateFlags = 0;
        if (_m_buildingStates[building].getBBuilt()) {
            _m_buildingTree.SetItem(hItem, TVIF_STATE, NULL, 0, 0, TVIS_BOLD, TVIS_BOLD, 0);
            stateFlags |= kBuiltFlag;
        } else if (_m_buildingStates[building].getBDisabled()) {
            _disableTree(hItem);
            stateFlags |= kDisabledFlag | kGrayedFlag;
        }
        _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0, MAKELONG(building, stateFlags));
    }
}

VA(0x004c3d73, 0x86)
void TTownPropsBuildingsPage::_retrieveBuildingStates(HTREEITEM hParent)
{
    for (HTREEITEM hItem = _m_buildingTree.GetChildItem(hParent); hItem != NULL;
         hItem = _m_buildingTree.GetNextSiblingItem(hItem)) {
        DWORD itemData = _m_buildingTree.GetItemData(hItem);
        WORD building = LOWORD(itemData);
        WORD stateFlags = HIWORD(itemData);
        _m_buildingStates[building].setBBuilt((stateFlags & kBuiltFlag) != 0);
        _m_buildingStates[building].setBDisabled((stateFlags & kDisabledFlag) != 0);
        _retrieveBuildingStates(hItem);
    }
}

VA(0x004c3df9, 0x7a)
void TTownPropsBuildingsPage::_disableTree(HTREEITEM hItem)
{
    DWORD itemData = _m_buildingTree.GetItemData(hItem);
    WORD stateFlags = HIWORD(itemData);
    if (stateFlags & kDisabledFlag)
        return;
    _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0, MAKELONG(LOWORD(itemData), stateFlags | kGrayedFlag));
    for (HTREEITEM hChild = _m_buildingTree.GetChildItem(hItem); hChild != NULL;
         hChild = _m_buildingTree.GetNextSiblingItem(hChild))
        _disableTree(hChild);
}

VA(0x004c3e73, 0x7a)
void TTownPropsBuildingsPage::_enableTree(HTREEITEM hItem)
{
    DWORD itemData = _m_buildingTree.GetItemData(hItem);
    WORD stateFlags = HIWORD(itemData);
    if (stateFlags & kDisabledFlag)
        return;
    _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0, MAKELONG(LOWORD(itemData), stateFlags & ~kGrayedFlag));
    for (HTREEITEM hChild = _m_buildingTree.GetChildItem(hItem); hChild != NULL;
         hChild = _m_buildingTree.GetNextSiblingItem(hChild))
        _enableTree(hChild);
}

VA(0x004c3eed, 0x88)
void TTownPropsBuildingsPage::_buildItem(HTREEITEM hItem)
{
    HTREEITEM hParent = _m_buildingTree.GetParentItem(hItem);
    if (hParent != NULL && !(HIWORD(_m_buildingTree.GetItemData(hParent)) & kBuiltFlag))
        _buildItem(hParent);
    DWORD itemData = _m_buildingTree.GetItemData(hItem);
    _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0,
                            MAKELONG(LOWORD(itemData), HIWORD(itemData) | kBuiltFlag));
    _m_buildingTree.SetItem(hItem, TVIF_STATE, NULL, 0, 0, TVIS_BOLD, TVIS_BOLD, 0);
}

VA(0x004c3f75, 0x99)
void TTownPropsBuildingsPage::_buildAll(HTREEITEM hParent)
{
    for (HTREEITEM hItem = _m_buildingTree.GetChildItem(hParent); hItem != NULL;
         hItem = _m_buildingTree.GetNextSiblingItem(hItem)) {
        DWORD itemData = _m_buildingTree.GetItemData(hItem);
        WORD stateFlags = HIWORD(itemData);
        if (stateFlags & kDisabledFlag) {
            _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0,
                                    MAKELONG(LOWORD(itemData), stateFlags & ~kDisabledFlag));
            _enableTree(hItem);
        }
        if (!(stateFlags & kBuiltFlag))
            _buildItem(hItem);
        _buildAll(hItem);
    }
}

VA(0x004c400e, 0xa0)
void TTownPropsBuildingsPage::_demolishItem(HTREEITEM hItem)
{
    for (HTREEITEM hChild = _m_buildingTree.GetChildItem(hItem); hChild != NULL;
         hChild = _m_buildingTree.GetNextSiblingItem(hChild)) {
        if (HIWORD(_m_buildingTree.GetItemData(hChild)) & kBuiltFlag)
            _demolishItem(hChild);
    }
    DWORD itemData = _m_buildingTree.GetItemData(hItem);
    _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0,
                            MAKELONG(LOWORD(itemData), HIWORD(itemData) & ~kBuiltFlag));
    _m_buildingTree.SetItem(hItem, TVIF_STATE, NULL, 0, 0, 0, TVIS_BOLD, 0);
}

VA(0x004c40ae, 0x54)
void TTownPropsBuildingsPage::_demolishAll(HTREEITEM hParent)
{
    for (HTREEITEM hItem = _m_buildingTree.GetChildItem(hParent); hItem != NULL;
         hItem = _m_buildingTree.GetNextSiblingItem(hItem)) {
        if (HIWORD(_m_buildingTree.GetItemData(hItem)) & kBuiltFlag)
            _demolishItem(hItem);
    }
}

VA(0x004c4102, 0x69)
void TTownPropsBuildingsPage::_setCheckStates(BYTE stateFlags)
{
    bool bBuilt = (stateFlags & kBuiltFlag) != 0;
    bool bDisabled = (stateFlags & kDisabledFlag) != 0;
    bool bGrayed = (stateFlags & kGrayedFlag) != 0;
    _m_builtCheck.SetCheck(bBuilt);
    _m_builtCheck.EnableWindow(!bDisabled && !bGrayed);
    _m_enabledCheck.SetCheck(!bDisabled);
}

VA(0x004c416b, 0x42)
const TTown* TTownPropsBuildingsPage::_getOldTown() const
{
    const TGameMap::TLayer& layer = _m_oldMap.getLayer(_m_townRef.getBSecondLayer());
    return dynamic_cast<const TTown*>(layer.getPObject(_m_townRef.getObjectID()));
}

VA(0x004c41ad, 0x42)
TTown* TTownPropsBuildingsPage::_getNewTown()
{
    TGameMap::TLayer& layer = _m_newMap.getLayer(_m_townRef.getBSecondLayer());
    return dynamic_cast<TTown*>(layer.getPObject(_m_townRef.getObjectID()));
}

VA(0x004c41ef, 0xaf)
void TTownPropsBuildingsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_DEMOLISH_ALL_BUTTON, _m_demolishAllButton);
    DDX_Control(pDX, IDC_BUILD_ALL_BUTTON, _m_buildAllButton);
    DDX_Control(pDX, IDC_BUILDING_DESCRIPTION_STATIC, _m_descriptionStatic);
    DDX_Control(pDX, IDC_BUILDING_TREE_STATIC, _m_treeStatic);
    DDX_Control(pDX, IDC_HAS_FORT_CHECK, _m_hasFortCheck);
    DDX_Control(pDX, IDC_CUSTOMIZE_CHECK, _m_customizeCheck);
    DDX_Control(pDX, IDC_ENABLED_CHECK, _m_enabledCheck);
    DDX_Control(pDX, IDC_BUILT_CHECK, _m_builtCheck);
    DDX_Control(pDX, IDC_BUILDING_TREE, _m_buildingTree);
}

VA(0x004c429e, 0x6)
BEGIN_MESSAGE_MAP(TTownPropsBuildingsPage, CPropertyPage)
    ON_BN_CLICKED(IDC_BUILT_CHECK, OnBuiltCheck)
    ON_BN_CLICKED(IDC_ENABLED_CHECK, OnEnabledCheck)
    ON_NOTIFY(TVN_SELCHANGED, IDC_BUILDING_TREE, OnSelChangedBuildingTree)
    ON_BN_CLICKED(IDC_CUSTOMIZE_CHECK, OnCustomizeCheck)
    ON_BN_CLICKED(IDC_HAS_FORT_CHECK, OnHasFortCheck)
    ON_BN_CLICKED(IDC_BUILD_ALL_BUTTON, OnBuildAllButton)
    ON_BN_CLICKED(IDC_DEMOLISH_ALL_BUTTON, OnDemolishAllButton)
END_MESSAGE_MAP()

VA(0x004c42a4, 0x1d6)
BOOL TTownPropsBuildingsPage::OnInitDialog()
{
    GetDlgItem(IDC_CUSTOMIZE_CHECK)->SetWindowText(kCustomizeCheckStr);
    GetDlgItem(IDC_HAS_FORT_CHECK)->SetWindowText(STownPropsBuildingsPageText::kHasFortCheckStr);
    GetDlgItem(IDC_BUILDING_TREE_STATIC)->SetWindowText(STownPropsBuildingsPageText::kBuildingTreeStaticStr);
    GetDlgItem(IDC_ENABLED_CHECK)->SetWindowText(STownPropsBuildingsPageText::kEnabledCheckStr);
    GetDlgItem(IDC_BUILT_CHECK)->SetWindowText(STownPropsBuildingsPageText::kBuiltCheckStr);
    GetDlgItem(IDC_BUILD_ALL_BUTTON)->SetWindowText(STownPropsBuildingsPageText::kBuildAllButtonStr);
    GetDlgItem(IDC_DEMOLISH_ALL_BUTTON)->SetWindowText(STownPropsBuildingsPageText::kDemolishAllButtonStr);
    _m_bModified = false;
    const TTown* pNewTown = _getNewTown();
    _m_bCustomBuildings = pNewTown->getBCustomBuildings();
    _m_buildingStates = pNewTown->getBuildingStates();
    CPropertyPage::OnInitDialog();
    _createTree();
    if (_m_bCustomBuildings) {
        _m_customizeCheck.SetCheck(1);
        _m_hasFortCheck.EnableWindow(FALSE);
        _showBuildingStates(NULL);
        _m_buildingTree.SelectItem(_m_buildingTree.GetRootItem());
    } else {
        _m_customizeCheck.SetCheck(0);
        _m_hasFortCheck.SetCheck(_m_buildingStates[eBuildingFort].getBBuilt());
        _m_treeStatic.EnableWindow(FALSE);
        _m_buildingTree.EnableWindow(FALSE);
        _m_builtCheck.EnableWindow(FALSE);
        _m_enabledCheck.EnableWindow(FALSE);
        _m_buildAllButton.EnableWindow(FALSE);
        _m_demolishAllButton.EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x004c447a, 0xc7)
void TTownPropsBuildingsPage::OnOK()
{
    CPropertyPage::OnOK();
    TTown* pNewTown = _getNewTown();
    pNewTown->setBCustomBuildings(_m_customizeCheck.GetCheck() != 0);
    if (pNewTown->getBCustomBuildings()) {
        _m_buildingStates = TArray<TTown::TBuildingState, kNumBuildings>();
        _retrieveBuildingStates(NULL);
    }
    pNewTown->setBuildingStates(_m_buildingStates);
    _m_bModified = _m_bModified || _m_pOldTown->getBCustomBuildings() != pNewTown->getBCustomBuildings()
                   || !(pNewTown->getBuildingStates() == _m_pOldTown->getBuildingStates());
}

VA(0x004c4541, 0x46)
void TTownPropsBuildingsPage::OnBuiltCheck()
{
    HTREEITEM hItem = _m_buildingTree.GetSelectedItem();
    if (_m_builtCheck.GetCheck())
        _buildItem(hItem);
    else
        _demolishItem(hItem);
}

VA(0x004c4587, 0x116)
void TTownPropsBuildingsPage::OnEnabledCheck()
{
    HTREEITEM hItem = _m_buildingTree.GetSelectedItem();
    BOOL bEnabled = _m_enabledCheck.GetCheck();
    DWORD itemData = _m_buildingTree.GetItemData(hItem);
    if (!bEnabled) {
        WORD stateFlags = HIWORD(itemData);
        if (stateFlags & kBuiltFlag) {
            _demolishItem(hItem);
            _m_builtCheck.SetCheck(0);
        }
        _m_builtCheck.EnableWindow(FALSE);
        _disableTree(hItem);
        stateFlags = HIWORD(_m_buildingTree.GetItemData(hItem));
        _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0,
                                MAKELONG(LOWORD(itemData), stateFlags | kDisabledFlag));
    } else {
        WORD stateFlags = HIWORD(itemData);
        _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0,
                                MAKELONG(LOWORD(itemData), stateFlags & ~kDisabledFlag));
        HTREEITEM hParent = _m_buildingTree.GetParentItem(hItem);
        if (hParent == NULL || !(HIWORD(_m_buildingTree.GetItemData(hParent)) & kGrayedFlag)) {
            _enableTree(hItem);
            _m_builtCheck.EnableWindow(TRUE);
        }
    }
}

VA(0x004c469d, 0x69)
void TTownPropsBuildingsPage::OnSelChangedBuildingTree(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    if (pNMTreeView->itemNew.hItem != NULL) {
        _setCheckStates(HIWORD(pNMTreeView->itemNew.lParam));
        unsigned int building = LOWORD(pNMTreeView->itemNew.lParam);
        _m_descriptionStatic.SetWindowText(
            _getNewTown()->getTownTypeTraits().m_akBuildingTraits[building].m_pDescription);
    } else {
        _setCheckStates(0);
        _m_descriptionStatic.SetWindowText("");
    }
    *pResult = 0;
}

VA(0x004c4706, 0x1b9)
void TTownPropsBuildingsPage::OnCustomizeCheck()
{
    if (_m_customizeCheck.GetCheck()) {
        _m_hasFortCheck.SetCheck(0);
        _m_hasFortCheck.EnableWindow(FALSE);
        _m_treeStatic.EnableWindow(TRUE);
        _m_buildingTree.EnableWindow(TRUE);
        _m_builtCheck.EnableWindow(TRUE);
        _m_enabledCheck.EnableWindow(TRUE);
        _m_buildAllButton.EnableWindow(TRUE);
        _m_demolishAllButton.EnableWindow(TRUE);
        _showBuildingStates(NULL);
        _m_buildingTree.SelectItem(_m_buildingTree.GetRootItem());
    } else {
        _m_buildingStates = TArray<TTown::TBuildingState, kNumBuildings>();
        _retrieveBuildingStates(NULL);
        demolishDependentBuildings(_getNewTown()->getTownTypeTraits().m_akBuildingTraits, eBuildingFort,
                                   &_m_buildingStates);
        _m_buildingTree.SelectItem(NULL);
        _resetTree();
        _m_treeStatic.EnableWindow(FALSE);
        _m_buildingTree.EnableWindow(FALSE);
        _m_builtCheck.SetCheck(0);
        _m_builtCheck.EnableWindow(FALSE);
        _m_enabledCheck.SetCheck(0);
        _m_enabledCheck.EnableWindow(FALSE);
        _m_buildAllButton.EnableWindow(FALSE);
        _m_demolishAllButton.EnableWindow(FALSE);
        _m_hasFortCheck.EnableWindow(TRUE);
        _m_hasFortCheck.SetCheck(_m_buildingStates[eBuildingFort].getBBuilt());
    }
}

VA(0x004c48bf, 0x3c)
void TTownPropsBuildingsPage::OnHasFortCheck()
{
    _m_buildingStates[eBuildingFort].setBBuilt(_m_hasFortCheck.GetCheck() != 0);
    if (_m_buildingStates[eBuildingFort].getBBuilt())
        _m_buildingStates[eBuildingFort].setBDisabled(false);
}

VA(0x004c48fb, 0x48)
void TTownPropsBuildingsPage::OnBuildAllButton()
{
    _buildAll(NULL);
    HTREEITEM hItem = _m_buildingTree.GetSelectedItem();
    _m_buildingTree.SelectItem(NULL);
    _m_buildingTree.SelectItem(hItem);
}

VA(0x004c4943, 0x48)
void TTownPropsBuildingsPage::OnDemolishAllButton()
{
    _demolishAll(NULL);
    HTREEITEM hItem = _m_buildingTree.GetSelectedItem();
    _m_buildingTree.SelectItem(NULL);
    _m_buildingTree.SelectItem(hItem);
}
