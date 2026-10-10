// HeroPlaceholderPropsDlg.h - the hero placeholder's properties dialog
// (HeroPlaceholderPropsDlg.cpp; GOG only, Loki's port has no counterpart):
// the owner, and either any hero of a power rating or a specific hero
// picked by class. The dialog edits a copy of the map, which DoModal
// assigns back when OK changed something. Layout from the image: the
// power, hero and class statics, owner combo, power spin and edit, hero
// and class combos from 0x5c, the DDX radio index at 0x23c, the map at
// 0x240, the placeholder's layer and id at 0x244, the old placeholder at
// 0x24c, the map copy at 0x250, its placeholder at 0x258, the modified
// flag at 0x25c, the specific hero radio at 0x260 and the heroes each
// class offers at 0x29c.
#ifndef HOMM3_EDITOR_HEROPLACEHOLDERPROPSDLG_H
#define HOMM3_EDITOR_HEROPLACEHOLDERPROPSDLG_H

#include <memory>
#include <set>

#include "editor/GameMap.h"
#include "editor/Hero.h"
#include "editor/MapObjectRef.h"
#include "editor/resource.h"

class THeroPlaceholderPropsDlg : public CDialog {
public:
    THeroPlaceholderPropsDlg(CWnd* pParent, TGameMap* pMap, TMapObjectRef placeholderRef);

    virtual int DoModal();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_HERO_PLACEHOLDER_PROPS };
    CStatic _m_powerRatingStatic;
    CStatic _m_heroStatic;
    CStatic _m_classStatic;
    CComboBox _m_ownerCombo;
    CSpinButtonCtrl _m_powerRatingSpin;
    CEdit _m_powerRatingEdit;
    CComboBox _m_heroCombo;
    CComboBox _m_classCombo;
    int _m_heroChoice;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnHeroRadio();
    afx_msg void OnKillFocusPowerRatingEdit();
    afx_msg void OnSelChangeClassCombo();
    DECLARE_MESSAGE_MAP()

private:
    const THeroPlaceholder* _getOldPlaceholder() const;

    TGameMap* _m_pMap;
    bool _m_bSecondLayer;
    TMapLayerObjectID _m_objectID;
    const THeroPlaceholder* _m_pOldPlaceholder;
    std::auto_ptr<TGameMap> _m_pNewMap;
    THeroPlaceholder* _m_pNewPlaceholder;
    bool _m_bModified;
    CButton _m_specificHeroRadio;
    std::set<THeroID> _m_aHeroesInClass[kNumHeroClasses];
};

#endif  /* HOMM3_EDITOR_HEROPLACEHOLDERPROPSDLG_H */
