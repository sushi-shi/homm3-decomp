// ShrinePropsDlg.h - the shrine's spell dialog (ShrinePropsDlg.cpp; GOG
// only, Loki's port has no counterpart): a random spell or one of the
// spells of the shrine's level. Layout from the image: the spell list at
// 0x5c, the spell group box at 0x98, the DDX radio index at 0xd4, the
// shrine at 0xd8, the modified flag at 0xdc and the spell at 0xe0.
#ifndef HOMM3_EDITOR_SHRINEPROPSDLG_H
#define HOMM3_EDITOR_SHRINEPROPSDLG_H

#include "armygrp.h"
#include "editor/resource.h"

class TShrine;

class TShrinePropsDlg : public CDialog {
public:
    TShrinePropsDlg(CWnd* pParent, TShrine* pShrine);

    virtual int DoModal();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_SHRINE_PROPS };
    CListBox _m_spellList;
    CButton _m_spellGroup;
    int _m_spellChoice;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnCustomSpellRadio();
    afx_msg void OnRandomSpellRadio();
    afx_msg void OnDblclkSpellList();
    DECLARE_MESSAGE_MAP()

private:
    TShrine* _m_pShrine;
    bool _m_bModified;
    ESpellId _m_spell;
};

#endif  /* HOMM3_EDITOR_SHRINEPROPSDLG_H */
