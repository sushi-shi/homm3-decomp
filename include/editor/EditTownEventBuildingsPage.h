// EditTownEventBuildingsPage.h - the buildings page of the town event
// sheet (EditTownEventBuildingsPage.cpp; Loki h3maped object 98): a tree
// of the town type's buildings with a "build" check for the selected one.
// Each tree item's data packs the building (low word) and its build flag
// (high word); the buildings to build are bold. Layout from the image: the
// check at 0x8c, the tree at 0xc8, then the event, town type, modified
// flag and build mask (0x118 bytes, the sheet's new).
#ifndef HOMM3_EDITOR_EDITTOWNEVENTBUILDINGSPAGE_H
#define HOMM3_EDITOR_EDITTOWNEVENTBUILDINGSPAGE_H

#include <bitset>

#include "editor/resource.h"
#include "editor/Town.h"

class TEditTownEventBuildingsPage : public CPropertyPage {
public:
    TEditTownEventBuildingsPage(const TTown::TTimedEvent& event, TTownType townType);
    virtual ~TEditTownEventBuildingsPage();

    const std::bitset<kNumBuildings>& getBuildMask() const { return _m_buildMask; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_EDIT_TOWN_EVENT_BUILDINGS };
    CButton _m_buildCheck;
    CTreeCtrl _m_buildingTree;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnBuildCheck();
    afx_msg void OnSelChangedBuildingTree(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()

private:
    void _createTree(TBuilding parentBuilding = eBuildingNone, HTREEITEM hParent = NULL);
    void _retrieveBuildMask(HTREEITEM hParent = NULL);
    void _buildItem(HTREEITEM hItem);
    void _demolishItem(HTREEITEM hItem);

    const TTown::TTimedEvent& _m_event;
    TTownType _m_townType;
    bool _m_bModified;
    std::bitset<kNumBuildings> _m_buildMask;
};

#endif  /* HOMM3_EDITOR_EDITTOWNEVENTBUILDINGSPAGE_H */
