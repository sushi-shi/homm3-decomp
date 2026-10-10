// TownPropsBuildingsPage.h - the buildings page of the town property sheet
// (TownPropsBuildingsPage.cpp; Loki h3maped object 91). An uncustomized
// town only chooses whether it has a fort; a customized one shows the town
// type's building tree, whose items carry each building's built, disabled
// and grayed (under a disabled building) flags in the high word of their
// data. Layout from the image: the controls from 0x8c, the maps, the
// town's reference, the original town and the modified and custom flags
// from 0x2a8, then the building states at 0x2be (0x2e8 bytes, the sheet's
// new).
#ifndef HOMM3_EDITOR_TOWNPROPSBUILDINGSPAGE_H
#define HOMM3_EDITOR_TOWNPROPSBUILDINGSPAGE_H

#include "editor/Array.h"
#include "editor/MapObjectRef.h"
#include "editor/resource.h"
#include "editor/Town.h"

class TGameMap;

class TTownPropsBuildingsPage : public CPropertyPage {
public:
    TTownPropsBuildingsPage(const TGameMap& oldMap, TGameMap& newMap, bool bSecondLayer, unsigned int objID);
    virtual ~TTownPropsBuildingsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_TOWN_PROPS_BUILDINGS };
    CButton _m_demolishAllButton;
    CButton _m_buildAllButton;
    CStatic _m_descriptionStatic;
    CStatic _m_treeStatic;
    CButton _m_hasFortCheck;
    CButton _m_customizeCheck;
    CButton _m_enabledCheck;
    CButton _m_builtCheck;
    CTreeCtrl _m_buildingTree;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnBuiltCheck();
    afx_msg void OnEnabledCheck();
    afx_msg void OnSelChangedBuildingTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCustomizeCheck();
    afx_msg void OnHasFortCheck();
    afx_msg void OnBuildAllButton();
    afx_msg void OnDemolishAllButton();
    DECLARE_MESSAGE_MAP()

private:
    void _createTree(TBuilding parentBuilding = eBuildingNone, HTREEITEM hParent = NULL);
    void _resetTree();
    void _resetTree(HTREEITEM hParent);
    void _showBuildingStates(HTREEITEM hParent);
    void _retrieveBuildingStates(HTREEITEM hParent);
    void _disableTree(HTREEITEM hItem);
    void _enableTree(HTREEITEM hItem);
    void _buildItem(HTREEITEM hItem);
    void _buildAll(HTREEITEM hParent);
    void _demolishItem(HTREEITEM hItem);
    void _demolishAll(HTREEITEM hParent);
    void _setCheckStates(BYTE stateFlags);
    const TTown* _getOldTown() const;
    TTown* _getNewTown();

    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    bool _m_bSecondLayer;
    unsigned int _m_objectID;
    const TTown* _m_pOldTown;
    bool _m_bModified;
    bool _m_bCustomBuildings;
    TArray<TTown::TBuildingState, kNumBuildings> _m_buildingStates;
};

#endif  /* HOMM3_EDITOR_TOWNPROPSBUILDINGSPAGE_H */
