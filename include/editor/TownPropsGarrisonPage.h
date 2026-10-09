// TownPropsGarrisonPage.h - the garrison page of the town property sheet
// (TownPropsGarrisonPage.cpp; Loki h3maped object 92): a customize check
// over a TArmyDlg child and the garrison's formation. The page edits the
// town in the sheet's copy of the map and compares it with the original.
// Layout from the image: the check at 0x8c, the formation radio index at
// 0xc8, then the maps, the town's reference, the original town, the
// modified and custom flags, the army dialog and the grouped flag (0xec
// bytes, the sheet's new).
#ifndef HOMM3_EDITOR_TOWNPROPSGARRISONPAGE_H
#define HOMM3_EDITOR_TOWNPROPSGARRISONPAGE_H

#include "editor/MapObjectRef.h"
#include "editor/resource.h"

class TArmyDlg;
class TGameMap;
class TTown;

class TTownPropsGarrisonPage : public CPropertyPage {
public:
    TTownPropsGarrisonPage(const TGameMap& oldMap, TGameMap& newMap, TMapObjectRef townRef);
    virtual ~TTownPropsGarrisonPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_TOWN_PROPS_GARRISON };
    CButton _m_customizeCheck;
    int _m_formation;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnCustomizeCheck();
    DECLARE_MESSAGE_MAP()

private:
    const TTown* _getOldTown() const;
    TTown* _getNewTown();

    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    TMapObjectRef _m_townRef;
    const TTown* _m_pOldTown;
    bool _m_bModified;
    bool _m_bCustomGarrison;
    TArmyDlg* _m_pArmyDlg;
    bool _m_bGroupedFormation;
};

#endif  /* HOMM3_EDITOR_TOWNPROPSGARRISONPAGE_H */
