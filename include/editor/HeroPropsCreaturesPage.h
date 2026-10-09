// HeroPropsCreaturesPage.h - the creatures page of the hero property
// sheets (HeroPropsCreaturesPage.cpp; Loki h3maped object 17): a customize
// check over a TArmyDlg child, whose client the page is, and the army's
// formation. A customized army needs a creature: the page holds the
// sheet's OK off while it has none. Layout from the image: the client
// interface at 0x8c, the check at 0x90, the formation radio index at
// 0xcc, then the sheet, the original and edited heroes, the modified flag
// and the army dialog (0xe4 bytes, the sheets' new).
#ifndef HOMM3_EDITOR_HEROPROPSCREATURESPAGE_H
#define HOMM3_EDITOR_HEROPROPSCREATURESPAGE_H

#include "gameversion.h"
#include "editor/ArmyDlg.h"
#include "editor/resource.h"

class THero;

class THeroPropsCreaturesPage : public CPropertyPage, private TArmyDlgClient {
public:
    // The sheet, told when its OK button may be pressed.
    class TParentSheet {
    public:
        virtual void onEnableOK() = 0;
        virtual void onDisableOK() = 0;
    };

    THeroPropsCreaturesPage(TParentSheet* pParentSheet, const THero* pOldHero, THero* pNewHero,
                            EGameVersion mapVersion, bool bRandomCreatures);
    virtual ~THeroPropsCreaturesPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_HERO_PROPS_CREATURES };
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
    virtual void onNumOccupiedStacksChanged(unsigned int newNum, unsigned int oldNum);

    TParentSheet* _m_pParentSheet;
    const THero* _m_pOldHero;
    THero* _m_pNewHero;
    bool _m_bModified;
    TArmyDlg* _m_pArmyDlg;
};

#endif  /* HOMM3_EDITOR_HEROPROPSCREATURESPAGE_H */
