// EditTownEventBuildingsPage.cpp - Loki h3maped object 98: the buildings
// page of the town event sheet. Building a building builds the buildings
// it hangs under; demolishing one demolishes those under it. The assert
// lines come from the retail immediates.
#include "editor/stdafx.h"

#include "editor/cppbridge.h"
#include "editor/EditTownEventBuildingsPage.h"

TEditTownEventBuildingsPage::TEditTownEventBuildingsPage(const TTown::TTimedEvent& event, TTownType townType)
    : _m_buildCheck(GTK_TOGGLE_BUTTON(_widget("edit_town_event_buildings_build"))),
      _m_buildingTree(GTK_CTREE(_widget("edit_town_event_buildings_tree"))),
      _m_event(event),
      _m_townType(townType),
      _m_bModified(false)
{
#line 45
    assert(townType >= 0 && townType <= kNumTownTypes);
    OnInitDialog();
}

TEditTownEventBuildingsPage::~TEditTownEventBuildingsPage()
{
}

void TEditTownEventBuildingsPage::_createTree(TBuilding parentBuilding, GtkCTreeNode* parentNode)
{
#line 66
    assert(parentBuilding >= eBuildingNone && parentBuilding < kNumBuildings);
    const TTown::TBuildingTraits (&akBuildingTraits)[kNumBuildings] =
        TTown::s_akTypeTraits[_m_townType].m_akBuildingTraits;
    for (unsigned int building = 0; building < kNumBuildings; building++) {
        if (!akBuildingTraits[building].isDisallowed() && akBuildingTraits[building].m_building == parentBuilding) {
#line 75
            assert(parentBuilding != building);
            gchar* text[2] = { (gchar*)akBuildingTraits[building].m_pName, NULL };
            GtkCTreeNode* node = gtk_ctree_insert_node(_m_buildingTree, parentNode, NULL, text, 0,
                                                       NULL, NULL, NULL, NULL, FALSE, TRUE);
            if (node) {
                _createTree(TBuilding(building), node);
                gtk_ctree_node_set_row_data(_m_buildingTree, node,
                                            (gpointer)MAKELONG(building, _m_buildMask[building]));
            }
        }
    }
}

void TEditTownEventBuildingsPage::_retrieveBuildMask(GtkCTreeNode* parentNode)
{
    GtkCTreeNode* node = NULL;
    if (!parentNode)
        node = gtk_ctree_node_nth(_m_buildingTree, 0);
    else
        node = GTK_CTREE_ROW(parentNode)->children;
    while (node) {
        gulong itemData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, node);
        uword building = LOWORD(itemData);
#line 110
        assert(building < kNumBuildings);
        _m_buildMask[building] = HIWORD(itemData);
        _retrieveBuildMask(node);
        node = GTK_CTREE_ROW(node)->sibling;
    }
}

void TEditTownEventBuildingsPage::_buildItem(GtkCTreeNode* node)
{
    GtkCTreeNode* parent = NULL;
    if (node)
        parent = GTK_CTREE_ROW(node)->parent;
    else
        node = gtk_ctree_node_nth(_m_buildingTree, 0);
    if (parent && HIWORD((gulong)gtk_ctree_node_get_row_data(_m_buildingTree, parent)) == 0)
        _buildItem(parent);
    gulong itemData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, node);
#line 141
    assert(HIWORD( itemData ) == 0);
    gtk_ctree_node_set_row_data(_m_buildingTree, node, (gpointer)MAKELONG(LOWORD(itemData), 1));
}

void TEditTownEventBuildingsPage::_demolishItem(GtkCTreeNode* node)
{
    GtkCTreeNode* child = NULL;
    if (!node)
        child = gtk_ctree_node_nth(_m_buildingTree, 0);
    else
        child = GTK_CTREE_ROW(node)->children;
    while (child) {
        if (HIWORD((gulong)gtk_ctree_node_get_row_data(_m_buildingTree, child)) == 0)
            _demolishItem(child);
        child = GTK_CTREE_ROW(child)->sibling;
    }
    gulong itemData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, node);
#line 172
    assert(HIWORD( itemData ) != 0);
    gtk_ctree_node_set_row_data(_m_buildingTree, node, (gpointer)MAKELONG(LOWORD(itemData), 0));
}

BOOL TEditTownEventBuildingsPage::OnInitDialog()
{
    _m_bModified = false;
    _m_buildMask = _m_event.getBuildMask();
    _createTree();
    GtkCTreeNode* hItem = gtk_ctree_node_nth(_m_buildingTree, 0);
#line 218
    assert(hItem != NULL);
    gtk_ctree_select(_m_buildingTree, hItem);
    return true;
}

void TEditTownEventBuildingsPage::OnOK()
{
    _retrieveBuildMask();
    _m_bModified = _m_bModified || _m_buildMask != _m_event.getBuildMask();
}

void TEditTownEventBuildingsPage::OnBuildCheck()
{
    GtkCTreeNode* hCurTreeItem = NULL;
    if (_m_buildingTree->clist.selection && _m_buildingTree->clist.selection->data)
        hCurTreeItem = GTK_CTREE_NODE(_m_buildingTree->clist.selection->data);
#line 246
    assert(hCurTreeItem != NULL);
    bool bBuild = gtk_toggle_button_get_active(_m_buildCheck);
    if (bBuild)
        _buildItem(hCurTreeItem);
    else
        _demolishItem(hCurTreeItem);
}

void TEditTownEventBuildingsPage::OnSelChangedBuildingTree()
{
    gboolean bBuild = FALSE;
    if (_m_buildingTree->clist.selection && _m_buildingTree->clist.selection->data) {
        GtkCTreeNode* hCurTreeItem = GTK_CTREE_NODE(_m_buildingTree->clist.selection->data);
        gulong itemData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, hCurTreeItem);
        bBuild = HIWORD(itemData) != 0;
    }
    gtk_signal_handler_block_by_func(GTK_OBJECT(_m_buildCheck),
                                     GTK_SIGNAL_FUNC(on_edit_town_event_buildings_build_toggled), NULL);
    gtk_toggle_button_set_active(_m_buildCheck, bBuild);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(_m_buildCheck),
                                       GTK_SIGNAL_FUNC(on_edit_town_event_buildings_build_toggled), NULL);
}
