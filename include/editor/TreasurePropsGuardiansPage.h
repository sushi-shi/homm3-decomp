// TreasurePropsGuardiansPage.h - the guardians page of the treasure
// property sheets (TreasurePropsGuardiansPage.cpp; Loki h3maped object
// 102): a customize check over a TArmyDlg child. Layout from the image:
// the DDX customize flag at 0x8c, then the treasure, the army dialog and
// the custom and modified flags (0x9c bytes, the sheets' new).
#ifndef HOMM3_EDITOR_TREASUREPROPSGUARDIANSPAGE_H
#define HOMM3_EDITOR_TREASUREPROPSGUARDIANSPAGE_H

#include "gameversion.h"
#include "editor/resource.h"

class TArmy;
class TArmyDlg;
class TTreasure;

class TTreasurePropsGuardiansPage : public CPropertyPage {
public:
    TTreasurePropsGuardiansPage(TTreasure* pTreasure, EGameVersion mapVersion, UINT nIDHelp);
    virtual ~TTreasurePropsGuardiansPage();

    bool wasModified() const { return _m_bModified; }
    bool getBCustomGuardians() const { return _m_bCustomGuardians; }
    const TArmy& getGuardians() const;

    enum { IDD = IDD_TREASURE_PROPS_GUARDIANS };
    BOOL _m_bCustomize;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnCustomizeCheck();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

private:
    TTreasure* _m_pTreasure;
    TArmyDlg* _m_pArmyDlg;
    bool _m_bCustomGuardians;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_TREASUREPROPSGUARDIANSPAGE_H */
