// TownPropsBuildingsPage.cpp - Loki h3maped object 91: the buildings page
// of the town property sheet. While the buildings are customized, the tree
// rows carry each building's built, disabled and grayed (under a disabled
// building) flags; building an item builds what it hangs under, demolishing
// one demolishes what hangs under it. The assert lines come from the retail
// immediates.
#include "editor/stdafx.h"

#include "editor/cppbridge.h"
#include "editor/TownPropsBuildingsPage.h"

// The state flags in the high word of a tree row's data (16-bit constants
// at the end of the object's .rodata).
const unsigned short kBuiltFlag = 1;
const unsigned short kDisabledFlag = 2;
const unsigned short kGrayedFlag = 4;

namespace {

void demolishDependentBuildings(const TTown::TBuildingTraits (&akBuildingTraits)[kNumBuildings],
                                TBuilding baseBuilding, TArray<TTown::TBuildingState, kNumBuildings>* pBuildingStates)
{
#line 73
    assert(baseBuilding >= 0 && baseBuilding < kNumBuildings);
    assert(pBuildingStates != NULL);
    TArray<TTown::TBuildingState, kNumBuildings>& buildingStates = *pBuildingStates;
    for (unsigned int building = 0; building < kNumBuildings; building++) {
        if (akBuildingTraits[building].m_building == baseBuilding && buildingStates[building].getBBuilt()) {
            demolishDependentBuildings(akBuildingTraits, TBuilding(building), pBuildingStates);
            buildingStates[building].setBBuilt(false);
        }
    }
}

}

TTownPropsBuildingsPage::TTownPropsBuildingsPage(const TTown& town)
    : _m_demolishAllButton(GTK_BUTTON(_widget("town_props_buildings_demolish_all"))),
      _m_buildAllButton(GTK_BUTTON(_widget("town_props_buildings_build_all"))),
      _m_descLabel(GTK_LABEL(_widget("town_props_buildings_desc"))),
      _m_treeLabel(GTK_LABEL(_widget("town_props_buildings_tree_label"))),
      _m_hasFortCheck(GTK_TOGGLE_BUTTON(_widget("town_props_buildings_has_fort"))),
      _m_customizeCheck(GTK_TOGGLE_BUTTON(_widget("town_props_buildings_customize"))),
      _m_enabledCheck(GTK_TOGGLE_BUTTON(_widget("town_props_buildings_enabled"))),
      _m_builtCheck(GTK_TOGGLE_BUTTON(_widget("town_props_buildings_built"))),
      _m_buildingTree(GTK_CTREE(_widget("town_props_buildings_tree"))),
      _m_town(town),
      _m_bModified(false)
{
    OnInitDialog();
}

TTownPropsBuildingsPage::~TTownPropsBuildingsPage()
{
}

void TTownPropsBuildingsPage::_createTree(TBuilding parentBuilding, GtkCTreeNode* parentNode)
{
#line 128
    assert(parentBuilding >= eBuildingNone && parentBuilding < kNumBuildings);
    const TTown::TBuildingTraits (&akBuildingTraits)[kNumBuildings] = _m_town.getTownTypeTraits().m_akBuildingTraits;
    for (unsigned int building = 0; building < kNumBuildings; building++) {
        if (!akBuildingTraits[building].isDisallowed() && akBuildingTraits[building].m_building == parentBuilding) {
#line 137
            assert(parentBuilding != building);
            gchar* text[2] = { (gchar*)akBuildingTraits[building].m_pName, NULL };
            GtkCTreeNode* node = gtk_ctree_insert_node(_m_buildingTree, parentNode, NULL, text, 0,
                                                       NULL, NULL, NULL, NULL, FALSE, TRUE);
            if (node) {
                _createTree(TBuilding(building), node);
                gtk_ctree_node_set_row_data(_m_buildingTree, node, (gpointer)building);
            }
        }
    }
}

void TTownPropsBuildingsPage::_resetTree()
{
    _resetTree(NULL);
    GtkCTreeNode* hItem = gtk_ctree_node_nth(_m_buildingTree, 0);
#line 160
    assert(hItem != NULL);
    gtk_ctree_node_moveto(_m_buildingTree, hItem, 0, 0, 0);
}

void TTownPropsBuildingsPage::_resetTree(GtkCTreeNode* parentNode)
{
    GtkCTreeNode* node = NULL;
    if (!parentNode)
        node = gtk_ctree_node_nth(_m_buildingTree, 0);
    else
        node = GTK_CTREE_ROW(parentNode)->children;
    while (node) {
        _resetTree(node);
        gtk_ctree_collapse(_m_buildingTree, node);
        uword building = LOWORD((gulong)gtk_ctree_node_get_row_data(_m_buildingTree, node));
#line 187
        assert(building < kNumBuildings);
        gtk_ctree_node_set_row_data(_m_buildingTree, node, (gpointer)building);
        node = GTK_CTREE_ROW(node)->sibling;
    }
}

void TTownPropsBuildingsPage::_showBuildingStates(GtkCTreeNode* parentNode)
{
    GtkCTreeNode* node = NULL;
    if (!parentNode)
        node = gtk_ctree_node_nth(_m_buildingTree, 0);
    else
        node = GTK_CTREE_ROW(parentNode)->children;
#line 205
    assert(gtk_toggle_button_get_active(_m_customizeCheck) != 0);
    while (node) {
        _showBuildingStates(node);
        uword building = LOWORD((gulong)gtk_ctree_node_get_row_data(_m_buildingTree, node));
#line 214
        assert(building < kNumBuildings);
        uword stateFlags = 0;
        if (_m_buildingStates[building].getBBuilt())
            stateFlags |= kBuiltFlag;
        else if (_m_buildingStates[building].getBDisabled()) {
            _disableTree(node);
            stateFlags |= kDisabledFlag | kGrayedFlag;
        }
        gtk_ctree_node_set_row_data(_m_buildingTree, node, (gpointer)MAKELONG(building, stateFlags));
        node = GTK_CTREE_ROW(node)->sibling;
    }
}

void TTownPropsBuildingsPage::_retrieveBuildingStates(GtkCTreeNode* parentNode)
{
    GtkCTreeNode* node = NULL;
    if (!parentNode)
        node = gtk_ctree_node_nth(_m_buildingTree, 0);
    else
        node = GTK_CTREE_ROW(parentNode)->children;
    while (node) {
        gulong itemData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, node);
        uword building = LOWORD(itemData);
#line 250
        assert(building < kNumBuildings);
        uword stateFlags = HIWORD(itemData);
        _m_buildingStates[building].setBBuilt((stateFlags & kBuiltFlag) != 0);
        _m_buildingStates[building].setBDisabled((stateFlags & kDisabledFlag) != 0);
        _retrieveBuildingStates(node);
        node = GTK_CTREE_ROW(node)->sibling;
    }
}

void TTownPropsBuildingsPage::_disableTree(GtkCTreeNode* node)
{
    GtkCTreeNode* child = NULL;
    if (!node)
        node = child = gtk_ctree_node_nth(_m_buildingTree, 0);
    else
        child = GTK_CTREE_ROW(node)->children;
#line 273
    assert(gtk_toggle_button_get_active(_m_customizeCheck) != 0);
    gulong itemData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, node);
    uword stateFlags = HIWORD(itemData);
    if (stateFlags & kDisabledFlag)
        return;
    gtk_ctree_node_set_row_data(_m_buildingTree, node, (gpointer)MAKELONG(LOWORD(itemData), stateFlags | kGrayedFlag));
    while (child) {
        _disableTree(child);
        child = GTK_CTREE_ROW(child)->sibling;
    }
}

void TTownPropsBuildingsPage::_enableTree(GtkCTreeNode* node)
{
    GtkCTreeNode* child = NULL;
    if (!node)
        node = child = gtk_ctree_node_nth(_m_buildingTree, 0);
    else
        child = GTK_CTREE_ROW(node)->children;
#line 307
    assert(gtk_toggle_button_get_active(_m_customizeCheck) != 0);
    gulong itemData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, node);
    uword stateFlags = HIWORD(itemData);
    if (stateFlags & kDisabledFlag)
        return;
    gtk_ctree_node_set_row_data(_m_buildingTree, node, (gpointer)MAKELONG(LOWORD(itemData), stateFlags & ~kGrayedFlag));
    while (child) {
        _enableTree(child);
        child = GTK_CTREE_ROW(child)->sibling;
    }
}

void TTownPropsBuildingsPage::_buildItem(GtkCTreeNode* node)
{
    GtkCTreeNode* parent = NULL;
    if (node)
        parent = GTK_CTREE_ROW(node)->parent;
    else {
        parent = NULL;
        node = gtk_ctree_node_nth(_m_buildingTree, 0);
    }
#line 344
    assert(gtk_toggle_button_get_active(_m_customizeCheck) != 0);
    if (parent) {
        uword stateFlags = HIWORD((gulong)gtk_ctree_node_get_row_data(_m_buildingTree, parent));
        if ((stateFlags & kBuiltFlag) == 0)
            _buildItem(parent);
    }
    gulong itemData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, node);
    uword stateFlags = HIWORD(itemData);
#line 358
    assert(( stateFlags & kBuiltFlag ) == 0);
    assert(( stateFlags & kDisabledFlag ) == 0);
    assert(( stateFlags & kGrayedFlag ) == 0);
    gtk_ctree_node_set_row_data(_m_buildingTree, node, (gpointer)MAKELONG(LOWORD(itemData), stateFlags | kBuiltFlag));
}

void TTownPropsBuildingsPage::_buildAll(GtkCTreeNode* parentNode)
{
    GtkCTreeNode* node = NULL;
    if (!parentNode)
        node = gtk_ctree_node_nth(_m_buildingTree, 0);
    else
        node = GTK_CTREE_ROW(parentNode)->children;
    while (node) {
        gulong itemData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, node);
        uword stateFlags = HIWORD(itemData);
        if (stateFlags & kDisabledFlag) {
            gtk_ctree_node_set_row_data(_m_buildingTree, node,
                                        (gpointer)MAKELONG(LOWORD(itemData), stateFlags & ~kDisabledFlag));
            _enableTree(node);
        }
        if ((stateFlags & kBuiltFlag) == 0)
            _buildItem(node);
        _buildAll(node);
        node = GTK_CTREE_ROW(node)->sibling;
    }
}

void TTownPropsBuildingsPage::_demolishItem(GtkCTreeNode* node)
{
    GtkCTreeNode* child = NULL;
    if (!node)
        node = child = gtk_ctree_node_nth(_m_buildingTree, 0);
    else
        child = GTK_CTREE_ROW(node)->children;
#line 409
    assert(gtk_toggle_button_get_active(_m_customizeCheck) != 0);
    while (child) {
        uword stateFlags = HIWORD((gulong)gtk_ctree_node_get_row_data(_m_buildingTree, child));
        if (stateFlags & kBuiltFlag)
            _demolishItem(child);
        child = GTK_CTREE_ROW(child)->sibling;
    }
    gulong itemData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, node);
    uword stateFlags = HIWORD(itemData);
#line 426
    assert(( stateFlags & kBuiltFlag ) != 0);
    assert(( stateFlags & kDisabledFlag ) == 0);
    assert(( stateFlags & kGrayedFlag ) == 0);
    gtk_ctree_node_set_row_data(_m_buildingTree, node, (gpointer)MAKELONG(LOWORD(itemData), stateFlags & ~kBuiltFlag));
}

void TTownPropsBuildingsPage::_demolishAll(GtkCTreeNode* parentNode)
{
    GtkCTreeNode* node = NULL;
    if (!parentNode)
        parentNode = node = gtk_ctree_node_nth(_m_buildingTree, 0);
    else
        node = GTK_CTREE_ROW(parentNode)->children;
#line 444
    assert(gtk_toggle_button_get_active(_m_customizeCheck) != 0);
    while (node) {
        uword stateFlags = HIWORD((gulong)gtk_ctree_node_get_row_data(_m_buildingTree, node));
        if (stateFlags & kBuiltFlag)
            _demolishItem(node);
        node = GTK_CTREE_ROW(node)->sibling;
    }
}

void TTownPropsBuildingsPage::_setCheckStates(uword stateFlags)
{
    bool bBuilt = (stateFlags & kBuiltFlag) != 0;
    bool bDisabled = (stateFlags & kDisabledFlag) != 0;
    bool bGrayed = (stateFlags & kGrayedFlag) != 0;
#line 466
    assert(!bBuilt || ( !bDisabled && !bGrayed ));
    block_signals();
    gtk_toggle_button_set_active(_m_builtCheck, bBuilt);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_builtCheck), !bDisabled && !bGrayed);
    gtk_toggle_button_set_active(_m_enabledCheck, !bDisabled);
    unblock_signals();
}

void TTownPropsBuildingsPage::block_signals()
{
    gtk_signal_handler_block_by_func(GTK_OBJECT(_m_hasFortCheck),
                                     GTK_SIGNAL_FUNC(on_town_props_buildings_has_fort_toggled), NULL);
    gtk_signal_handler_block_by_func(GTK_OBJECT(_m_customizeCheck),
                                     GTK_SIGNAL_FUNC(on_town_props_buildings_customize_toggled), NULL);
    gtk_signal_handler_block_by_func(GTK_OBJECT(_m_enabledCheck),
                                     GTK_SIGNAL_FUNC(on_town_props_buildings_enabled_toggled), NULL);
    gtk_signal_handler_block_by_func(GTK_OBJECT(_m_builtCheck),
                                     GTK_SIGNAL_FUNC(on_town_props_buildings_built_toggled), NULL);
}

void TTownPropsBuildingsPage::unblock_signals()
{
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(_m_hasFortCheck),
                                       GTK_SIGNAL_FUNC(on_town_props_buildings_has_fort_toggled), NULL);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(_m_customizeCheck),
                                       GTK_SIGNAL_FUNC(on_town_props_buildings_customize_toggled), NULL);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(_m_enabledCheck),
                                       GTK_SIGNAL_FUNC(on_town_props_buildings_enabled_toggled), NULL);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(_m_builtCheck),
                                       GTK_SIGNAL_FUNC(on_town_props_buildings_built_toggled), NULL);
}

BOOL TTownPropsBuildingsPage::OnInitDialog()
{
    _m_bModified = false;
    _m_bCustomBuildings = _m_town.getBCustomBuildings();
    _m_newBuildingStates = _m_town.getBuildingStates();
    _m_buildingStates = _m_newBuildingStates;
    GtkCTreeNode* node = gtk_ctree_node_nth(_m_buildingTree, 0);
    while (node) {
        gtk_ctree_remove_node(_m_buildingTree, node);
        node = gtk_ctree_node_nth(_m_buildingTree, 0);
    }
    _createTree();
    block_signals();
    if (_m_bCustomBuildings) {
        gtk_toggle_button_set_active(_m_customizeCheck, TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_hasFortCheck), FALSE);
        _showBuildingStates();
        GtkCTreeNode* hItem = gtk_ctree_node_nth(_m_buildingTree, 0);
        gtk_ctree_select(_m_buildingTree, hItem);
    } else {
        gtk_toggle_button_set_active(_m_customizeCheck, FALSE);
        gtk_toggle_button_set_active(_m_hasFortCheck, _m_buildingStates[eBuildingFort].getBBuilt());
        gtk_widget_set_sensitive(GTK_WIDGET(_m_treeLabel), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_buildingTree), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_builtCheck), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_enabledCheck), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_buildAllButton), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_demolishAllButton), FALSE);
    }
    unblock_signals();
    return true;
}

void TTownPropsBuildingsPage::OnOK()
{
    _m_bCustomBuildings = gtk_toggle_button_get_active(_m_customizeCheck);
    if (_m_bCustomBuildings) {
        _m_buildingStates = TArray<TTown::TBuildingState, kNumBuildings>();
        _retrieveBuildingStates();
    }
    _m_newBuildingStates = _m_buildingStates;
    _m_bModified = _m_bModified || _m_bCustomBuildings != _m_town.getBCustomBuildings()
                   || _m_newBuildingStates != _m_town.getBuildingStates();
}

void TTownPropsBuildingsPage::OnBuiltCheck()
{
#line 629
    assert(gtk_toggle_button_get_active(_m_customizeCheck) != 0);
    GtkCTreeNode* hCurTreeItem = NULL;
    if (_m_buildingTree->clist.selection && _m_buildingTree->clist.selection->data)
        hCurTreeItem = GTK_CTREE_NODE(_m_buildingTree->clist.selection->data);
#line 639
    assert(hCurTreeItem != NULL);
    bool bBuilt = gtk_toggle_button_get_active(_m_builtCheck);
    if (bBuilt)
        _buildItem(hCurTreeItem);
    else
        _demolishItem(hCurTreeItem);
}

void TTownPropsBuildingsPage::OnEnabledCheck()
{
#line 653
    assert(gtk_toggle_button_get_active(_m_customizeCheck) != 0);
    GtkCTreeNode* hCurTreeItem = NULL;
    if (_m_buildingTree->clist.selection && _m_buildingTree->clist.selection->data)
        hCurTreeItem = GTK_CTREE_NODE(_m_buildingTree->clist.selection->data);
#line 663
    assert(hCurTreeItem != NULL);
    bool bDisabled = !gtk_toggle_button_get_active(_m_enabledCheck);
    gulong itemData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, hCurTreeItem);
    if (bDisabled) {
        uword stateFlags = HIWORD(itemData);
#line 673
        assert(( stateFlags & kDisabledFlag ) == 0);
        if (stateFlags & kBuiltFlag) {
            _demolishItem(hCurTreeItem);
            block_signals();
            gtk_toggle_button_set_active(_m_builtCheck, FALSE);
            unblock_signals();
        }
        gtk_widget_set_sensitive(GTK_WIDGET(_m_builtCheck), FALSE);
        _disableTree(hCurTreeItem);
        stateFlags = HIWORD((gulong)gtk_ctree_node_get_row_data(_m_buildingTree, hCurTreeItem));
        gtk_ctree_node_set_row_data(_m_buildingTree, hCurTreeItem,
                                    (gpointer)MAKELONG(LOWORD(itemData), stateFlags | kDisabledFlag));
    } else {
        uword stateFlags = HIWORD(itemData);
#line 696
        assert(( stateFlags & kBuiltFlag ) == 0);
        assert(( stateFlags & kDisabledFlag ) != 0);
        assert(( stateFlags & kGrayedFlag ) != 0);
        gtk_ctree_node_set_row_data(_m_buildingTree, hCurTreeItem,
                                    (gpointer)MAKELONG(LOWORD(itemData), stateFlags & ~kDisabledFlag));
        GtkCTreeNode* parent = GTK_CTREE_ROW(hCurTreeItem)->parent;
        gulong parentData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, parent);
        if (!parent || (HIWORD(parentData) & kGrayedFlag) == 0) {
            _enableTree(hCurTreeItem);
            gtk_widget_set_sensitive(GTK_WIDGET(_m_builtCheck), TRUE);
        }
    }
}

void TTownPropsBuildingsPage::OnSelChangedBuildingTree()
{
#line 722
    assert(gtk_toggle_button_get_active(_m_customizeCheck) != FALSE);
    if (_m_buildingTree->clist.selection && _m_buildingTree->clist.selection->data) {
        GtkCTreeNode* hCurTreeItem = GTK_CTREE_NODE(_m_buildingTree->clist.selection->data);
        gulong itemData = (gulong)gtk_ctree_node_get_row_data(_m_buildingTree, hCurTreeItem);
        _setCheckStates(HIWORD(itemData));
        unsigned int building = LOWORD(itemData);
#line 737
        assert(building >= 0 && building < kNumBuildings);
        const TTown::TBuildingTraits& traits = _m_town.getTownTypeTraits().m_akBuildingTraits[building];
        gtk_label_set_text(_m_descLabel, traits.m_pDescription);
    } else {
        _setCheckStates(0);
        gtk_label_set_text(_m_descLabel, "");
    }
}

void TTownPropsBuildingsPage::OnCustomizeCheck()
{
    block_signals();
    if (gtk_toggle_button_get_active(_m_customizeCheck)) {
        gtk_toggle_button_set_active(_m_hasFortCheck, FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_hasFortCheck), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_treeLabel), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_buildingTree), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_builtCheck), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_enabledCheck), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_buildAllButton), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_demolishAllButton), TRUE);
        _showBuildingStates();
        GtkCTreeNode* hItem = gtk_ctree_node_nth(_m_buildingTree, 0);
#line 784
        assert(hItem != NULL);
        gtk_ctree_select(_m_buildingTree, hItem);
    } else {
        _m_buildingStates = TArray<TTown::TBuildingState, kNumBuildings>();
        _retrieveBuildingStates();
        demolishDependentBuildings(_m_town.getTownTypeTraits().m_akBuildingTraits, eBuildingFort, &_m_buildingStates);
        if (_m_buildingTree->clist.selection && _m_buildingTree->clist.selection->data)
            gtk_ctree_unselect(_m_buildingTree, GTK_CTREE_NODE(_m_buildingTree->clist.selection->data));
        _resetTree();
        gtk_widget_set_sensitive(GTK_WIDGET(_m_treeLabel), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_buildingTree), FALSE);
        gtk_toggle_button_set_active(_m_builtCheck, FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_builtCheck), FALSE);
        gtk_toggle_button_set_active(_m_enabledCheck, FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_enabledCheck), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_buildAllButton), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_demolishAllButton), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_hasFortCheck), TRUE);
        gtk_toggle_button_set_active(_m_hasFortCheck, _m_buildingStates[eBuildingFort].getBBuilt());
        gtk_label_set_text(_m_descLabel, "");
    }
    unblock_signals();
}

void TTownPropsBuildingsPage::OnHasFortCheck()
{
#line 835
    assert(gtk_toggle_button_get_active(_m_customizeCheck) == 0);
    _m_buildingStates[eBuildingFort].setBBuilt(gtk_toggle_button_get_active(_m_hasFortCheck));
    if (_m_buildingStates[eBuildingFort].getBBuilt())
        _m_buildingStates[eBuildingFort].setBDisabled(false);
}

void TTownPropsBuildingsPage::OnBuildAllButton()
{
#line 847
    assert(gtk_toggle_button_get_active(_m_customizeCheck) != 0);
    _buildAll();
    GtkCTreeNode* hCurTreeItem = NULL;
    if (_m_buildingTree->clist.selection && _m_buildingTree->clist.selection->data) {
        hCurTreeItem = GTK_CTREE_NODE(_m_buildingTree->clist.selection->data);
        gtk_ctree_unselect(_m_buildingTree, hCurTreeItem);
        gtk_ctree_select(_m_buildingTree, hCurTreeItem);
    }
}

void TTownPropsBuildingsPage::OnDemolishAllButton()
{
#line 869
    assert(gtk_toggle_button_get_active(_m_customizeCheck) != 0);
    _demolishAll();
    GtkCTreeNode* hCurTreeItem = NULL;
    if (_m_buildingTree->clist.selection && _m_buildingTree->clist.selection->data) {
        hCurTreeItem = GTK_CTREE_NODE(_m_buildingTree->clist.selection->data);
        gtk_ctree_unselect(_m_buildingTree, hCurTreeItem);
        gtk_ctree_select(_m_buildingTree, hCurTreeItem);
    }
}
