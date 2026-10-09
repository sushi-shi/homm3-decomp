// EditTownEventBuildingsPage.cpp - the buildings page of the town event
// sheet (h3maped 0x41735d..0x417b32; Loki h3maped object 98). Building a
// building builds the buildings it hangs under; demolishing one demolishes
// those under it.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/EditTownEventBuildingsPage.h"
#include "editor/MapEditorText.h"

VA(0x00417531, 0xad)
TEditTownEventBuildingsPage::TEditTownEventBuildingsPage(const TTown::TTimedEvent& event, TTownType townType)
    : CPropertyPage(TEditTownEventBuildingsPage::IDD),
      _m_event(event),
      _m_townType(townType),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kBuildingsPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x004175de, 0x1c, SCALAR_DELETING_DTOR, TEditTownEventBuildingsPage)

VA(0x004175fa, 0x50)
TEditTownEventBuildingsPage::~TEditTownEventBuildingsPage()
{
}

VA(0x0041764a, 0xec)
void TEditTownEventBuildingsPage::_createTree(TBuilding parentBuilding, HTREEITEM hParent)
{
    const TTown::TBuildingTraits (&akBuildingTraits)[kNumBuildings] =
        TTown::s_akTypeTraits[_m_townType].m_akBuildingTraits;
    for (unsigned int building = 0; building < kNumBuildings; building++) {
        if (!akBuildingTraits[building].isDisallowed() && akBuildingTraits[building].m_building == parentBuilding) {
            HTREEITEM hItem = _m_buildingTree.InsertItem(TVIF_TEXT, akBuildingTraits[building].m_pName, 0, 0, 0, 0, 0,
                                                         hParent, TVI_LAST);
            if (hItem != NULL) {
                _createTree(TBuilding(building), hItem);
                if (_m_buildMask[building])
                    _m_buildingTree.SetItem(hItem, TVIF_STATE, NULL, 0, 0, TVIS_BOLD, TVIS_BOLD, 0);
                _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0, MAKELONG(building, _m_buildMask[building]));
            }
        }
    }
}

VA(0x00417736, 0x66)
void TEditTownEventBuildingsPage::_retrieveBuildMask(HTREEITEM hParent)
{
    for (HTREEITEM hItem = _m_buildingTree.GetChildItem(hParent); hItem != NULL;
         hItem = _m_buildingTree.GetNextSiblingItem(hItem)) {
        DWORD itemData = _m_buildingTree.GetItemData(hItem);
        WORD building = LOWORD(itemData);
        _m_buildMask[building] = HIWORD(itemData) != 0;
        _retrieveBuildMask(hItem);
    }
}

VA(0x0041779c, 0x81)
void TEditTownEventBuildingsPage::_buildItem(HTREEITEM hItem)
{
    HTREEITEM hParent = _m_buildingTree.GetParentItem(hItem);
    if (hParent != NULL && HIWORD(_m_buildingTree.GetItemData(hParent)) == 0)
        _buildItem(hParent);
    DWORD itemData = _m_buildingTree.GetItemData(hItem);
    _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0, MAKELONG(LOWORD(itemData), 1));
    _m_buildingTree.SetItem(hItem, TVIF_STATE, NULL, 0, 0, TVIS_BOLD, TVIS_BOLD, 0);
}

// h3maped 0x41781d..0x4178b1: the census cuts this body at weak seeds (0x417845, 0x41786f).
void TEditTownEventBuildingsPage::_demolishItem(HTREEITEM hItem)
{
    for (HTREEITEM hChild = _m_buildingTree.GetChildItem(hItem); hChild != NULL;
         hChild = _m_buildingTree.GetNextSiblingItem(hChild)) {
        if (HIWORD(_m_buildingTree.GetItemData(hChild)) != 0)
            _demolishItem(hChild);
    }
    DWORD itemData = _m_buildingTree.GetItemData(hItem);
    _m_buildingTree.SetItem(hItem, TVIF_PARAM, NULL, 0, 0, 0, 0, MAKELONG(LOWORD(itemData), 0));
    _m_buildingTree.SetItem(hItem, TVIF_STATE, NULL, 0, 0, 0, TVIS_BOLD, 0);
}

VA(0x004178b1, 0x31)
void TEditTownEventBuildingsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_BUILD_CHECK, _m_buildCheck);
    DDX_Control(pDX, IDC_BUILDING_TREE, _m_buildingTree);
}

VA(0x004178e2, 0x6)
BEGIN_MESSAGE_MAP(TEditTownEventBuildingsPage, CPropertyPage)
    ON_BN_CLICKED(IDC_BUILD_CHECK, OnBuildCheck)
    ON_NOTIFY(TVN_SELCHANGED, IDC_BUILDING_TREE, OnSelChangedBuildingTree)
END_MESSAGE_MAP()

VA(0x004178e8, 0x94)
BOOL TEditTownEventBuildingsPage::OnInitDialog()
{
    GetDlgItem(IDC_BUILDING_TREE_STATIC)->SetWindowText(SEditTownEventBuildingsPageText::kBuildingTreeStaticStr);
    GetDlgItem(IDC_BUILD_CHECK)->SetWindowText(SEditTownEventBuildingsPageText::kBuildCheckStr);
    _m_bModified = false;
    _m_buildMask = _m_event.getBuildMask();
    CPropertyPage::OnInitDialog();
    _createTree();
    HTREEITEM hItem = _m_buildingTree.GetRootItem();
    _m_buildingTree.SelectItem(hItem);
    return TRUE;
}

VA(0x0041797c, 0x42)
void TEditTownEventBuildingsPage::OnOK()
{
    CPropertyPage::OnOK();
    _retrieveBuildMask();
    _m_bModified = _m_bModified || _m_buildMask != _m_event.getBuildMask();
}

VA(0x004179be, 0x46)
void TEditTownEventBuildingsPage::OnBuildCheck()
{
    HTREEITEM hItem = _m_buildingTree.GetSelectedItem();
    if (_m_buildCheck.GetCheck())
        _buildItem(hItem);
    else
        _demolishItem(hItem);
}

VA(0x00417a04, 0x2f)
void TEditTownEventBuildingsPage::OnSelChangedBuildingTree(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    _m_buildCheck.SetCheck(HIWORD(pNMTreeView->itemNew.lParam) != 0);
    *pResult = 0;
}
