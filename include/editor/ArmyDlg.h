// ArmyDlg.h - the seven creature stack editor (ArmyDlg.cpp; Loki h3maped
// object 88), a child dialog of the props pages that edit an army. Each
// slot is a creature type combo (none, then the version's creatures and,
// where allowed, the random ones) and a quantity edit with its spin. The
// editor works on a copy of the caller's army and reports changes in the
// number of occupied stacks to its client. Layout from the image: the
// dialog, the client interface at 0x5c, the client, the caller's army, the
// map's version, whether random creatures are offered, the modified flag,
// the number of occupied stacks, the seven slots at 0x74 (0xbc bytes each)
// and the edited army at 0x598 (0x5d0 bytes).
#ifndef HOMM3_EDITOR_ARMYDLG_H
#define HOMM3_EDITOR_ARMYDLG_H

#include "gameversion.h"
#include "editor/Army.h"
#include "editor/resource.h"

class TArmyDlgClient {
public:
    virtual void onNumOccupiedStacksChanged(unsigned int newNum, unsigned int oldNum) = 0;
};

class TArmyDlg : public CDialog, private TArmyDlgClient {
public:
    TArmyDlg(const TArmy& army, EGameVersion mapVersion, bool bRandomCreatures);
    TArmyDlg(TArmyDlgClient* pClient, const TArmy& army, EGameVersion mapVersion, bool bRandomCreatures);

    unsigned int getNumOccupiedStacks() const { return _m_numOccupiedStacks; }
    const TArmy& getArmy() const { return _m_army; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_ARMY };

    virtual void OnOK();
    virtual BOOL OnInitDialog();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnSelChangeTypeCombo1();
    afx_msg void OnSelChangeTypeCombo2();
    afx_msg void OnSelChangeTypeCombo3();
    afx_msg void OnSelChangeTypeCombo4();
    afx_msg void OnSelChangeTypeCombo5();
    afx_msg void OnSelChangeTypeCombo6();
    afx_msg void OnSelChangeTypeCombo7();
    afx_msg void OnKillFocusQtyEdit1();
    afx_msg void OnKillFocusQtyEdit2();
    afx_msg void OnKillFocusQtyEdit3();
    afx_msg void OnKillFocusQtyEdit4();
    afx_msg void OnKillFocusQtyEdit5();
    afx_msg void OnKillFocusQtyEdit6();
    afx_msg void OnKillFocusQtyEdit7();
    afx_msg void OnEnable(BOOL bEnable);
    DECLARE_MESSAGE_MAP()

private:
    enum { _s_kNumCreatureStacks = 7 };

    // One slot: the stack shown and its controls.
    struct _TStackData {
        TCreatureType m_creatureType;
        unsigned int m_quantity;
        CComboBox m_typeCombo;
        CEdit m_quantityEdit;
        CSpinButtonCtrl m_quantitySpin;
    };

    virtual void onNumOccupiedStacksChanged(unsigned int newNum, unsigned int oldNum) {}

    void _retrieveStackQuantities();
    void _onSelChangeTypeCombo(unsigned int stackNum);
    void _onKillFocusQtyEdit(unsigned int stackNum);

    TArmyDlgClient* _m_pClient;
    const TArmy& _m_originalArmy;
    EGameVersion _m_mapVersion;
    bool _m_bRandomCreatures;
    bool _m_bModified;
    unsigned int _m_numOccupiedStacks;
    _TStackData _m_aStackData[_s_kNumCreatureStacks];
    TArmy _m_army;
};

#endif  /* HOMM3_EDITOR_ARMYDLG_H */
