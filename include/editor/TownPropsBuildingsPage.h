// TownPropsBuildingsPage.h - the buildings page of the town property sheet
// (Loki TownPropsBuildingsPage.cpp): a tree of the town type's buildings
// with built and enabled toggles, build-all and demolish-all buttons, and
// a has-fort toggle used while the buildings are not customized. Each tree
// row's data packs the building (low word) and its state flags (high
// word). Layout from the image: the widgets (_m_customizeCheck from the
// asserts), the town, the modified and custom flags, the published and
// the working building states, then the vtable pointer (OnOK,
// OnInitDialog). The other member names are not proven.
#ifndef HOMM3_EDITOR_TOWNPROPSBUILDINGSPAGE_H
#define HOMM3_EDITOR_TOWNPROPSBUILDINGSPAGE_H

#include "editor/stdafx.h"
#include "editor/Array.h"
#include "editor/Town.h"

namespace {
#include <gtk/gtk.h>
}

class TTownPropsBuildingsPage {
public:
    TTownPropsBuildingsPage(const TTown& town);
    ~TTownPropsBuildingsPage();

    void block_signals();
    void unblock_signals();

    virtual void OnOK();
    virtual BOOL OnInitDialog();
    void OnBuiltCheck();
    void OnEnabledCheck();
    void OnSelChangedBuildingTree();
    void OnCustomizeCheck();
    void OnHasFortCheck();
    void OnBuildAllButton();
    void OnDemolishAllButton();

    bool getBCustomBuildings() const { return _m_bCustomBuildings; }
    const TArray<TTown::TBuildingState, kNumBuildings>& getBuildingStates() const { return _m_newBuildingStates; }
    bool wasModified() const { return _m_bModified; }

private:
    void _createTree(TBuilding parentBuilding = eBuildingNone, GtkCTreeNode* parentNode = 0);
    void _resetTree();
    void _resetTree(GtkCTreeNode* parentNode);
    void _showBuildingStates(GtkCTreeNode* parentNode = 0);
    void _retrieveBuildingStates(GtkCTreeNode* parentNode = 0);
    void _disableTree(GtkCTreeNode* node);
    void _enableTree(GtkCTreeNode* node);
    void _buildItem(GtkCTreeNode* node);
    void _buildAll(GtkCTreeNode* parentNode = 0);
    void _demolishItem(GtkCTreeNode* node);
    void _demolishAll(GtkCTreeNode* parentNode = 0);
    void _setCheckStates(uword stateFlags);

    GtkButton* _m_demolishAllButton;
    GtkButton* _m_buildAllButton;
    GtkLabel* _m_descLabel;
    GtkLabel* _m_treeLabel;
    GtkToggleButton* _m_hasFortCheck;
    GtkToggleButton* _m_customizeCheck;
    GtkToggleButton* _m_enabledCheck;
    GtkToggleButton* _m_builtCheck;
    GtkCTree* _m_buildingTree;
    const TTown& _m_town;
    bool _m_bModified;
    bool _m_bCustomBuildings;
    TArray<TTown::TBuildingState, kNumBuildings> _m_newBuildingStates;
    TArray<TTown::TBuildingState, kNumBuildings> _m_buildingStates;
};

#endif  /* HOMM3_EDITOR_TOWNPROPSBUILDINGSPAGE_H */
