// EditCreatureStackDlg.h - the creature stack editor (EditCreatureStackDlg.cpp;
// GOG only, Loki's port has no counterpart), a child of the black box
// contents page's creature list: a creature type among the allowed ones
// and a quantity of 1..TCreatureStack::s_kMaxQuantity. Restoration of
// Erathia maps offer only its creatures. Layout from the image: the
// quantity spin at 0x5c, the quantity edit at 0x98, the type combo at
// 0xd4, the stack at 0x110, the map's version at 0x118 and the allowed
// creature types at 0x11c.
#ifndef HOMM3_EDITOR_EDITCREATURESTACKDLG_H
#define HOMM3_EDITOR_EDITCREATURESTACKDLG_H

#include <bitset>

#include "armygrp.h"
#include "gameversion.h"
#include "editor/Army.h"
#include "editor/resource.h"

class TEditCreatureStackDlg : public CDialog {
public:
    // A new stack: the first allowed creature, one of it.
    TEditCreatureStackDlg(CWnd* pParent, EGameVersion mapVersion,
                          const std::bitset<kNumCreatureTypes>& abAllowedCreature);
    TEditCreatureStackDlg(CWnd* pParent, const TCreatureStack& stack, EGameVersion mapVersion,
                          const std::bitset<kNumCreatureTypes>& abAllowedCreature);

    const TCreatureStack& getStack() const { return _m_stack; }

    enum { IDD = IDD_EDIT_CREATURE_STACK };
    CSpinButtonCtrl _m_quantitySpin;
    CEdit _m_quantityEdit;
    CComboBox _m_typeCombo;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnKillFocusQuantityEdit();
    DECLARE_MESSAGE_MAP()

private:
    TCreatureStack _m_stack;
    EGameVersion _m_mapVersion;
    std::bitset<kNumCreatureTypes> _m_abAllowedCreature;
};

#endif  /* HOMM3_EDITOR_EDITCREATURESTACKDLG_H */
