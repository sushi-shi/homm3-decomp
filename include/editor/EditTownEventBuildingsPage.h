// EditTownEventBuildingsPage.h - the buildings page of the town event sheet
// (Loki EditTownEventBuildingsPage.cpp): a tree of the town type's
// buildings with a "build" toggle for the selected one. Each tree row's
// data packs the building (low word) and its build flag (high word).
// Layout from the image: the toggle and tree widgets, the event, the town
// type, the modified flag, the build mask, then the vtable pointer (OnOK,
// OnInitDialog). The member names are not proven.
#ifndef HOMM3_EDITOR_EDITTOWNEVENTBUILDINGSPAGE_H
#define HOMM3_EDITOR_EDITTOWNEVENTBUILDINGSPAGE_H

#include "editor/stdafx.h"

#include <bitset>

#include "editor/Town.h"

namespace {
#include <gtk/gtk.h>
}

class TEditTownEventBuildingsPage {
public:
    TEditTownEventBuildingsPage(const TTown::TTimedEvent& event, TTownType townType);
    ~TEditTownEventBuildingsPage();

    virtual void OnOK();
    virtual BOOL OnInitDialog();
    void OnBuildCheck();
    void OnSelChangedBuildingTree();

    const bitset<kNumBuildings>& getBuildMask() const { return _m_buildMask; }
    bool wasModified() const { return _m_bModified; }

private:
    void _createTree(TBuilding parentBuilding = eBuildingNone, GtkCTreeNode* parentNode = 0);
    void _retrieveBuildMask(GtkCTreeNode* parentNode = 0);
    void _buildItem(GtkCTreeNode* node);
    void _demolishItem(GtkCTreeNode* node);

    GtkToggleButton* _m_buildCheck;
    GtkCTree* _m_buildingTree;
    const TTown::TTimedEvent& _m_event;
    TTownType _m_townType;
    bool _m_bModified;
    bitset<kNumBuildings> _m_buildMask;
};

#endif  /* HOMM3_EDITOR_EDITTOWNEVENTBUILDINGSPAGE_H */
