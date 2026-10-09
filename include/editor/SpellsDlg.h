// SpellsDlg.h - the spell check list (GOG SpellsDlg.cpp, not in Loki), a
// child dialog of the hero spells page. Before Shadow of Death a hero
// keeps a single spell: checking one clears the other. Layout from the
// image: the dialog, the map's version, whether the items are enabled,
// the checked spells and the list (0xb4 bytes).
#ifndef HOMM3_EDITOR_SPELLSDLG_H
#define HOMM3_EDITOR_SPELLSDLG_H

#include <bitset>

#include "armygrp.h"
#include "gameversion.h"
#include "editor/resource.h"

class TSpellsDlg : public CDialog {
public:
    // Creates the dialog as a child of the parent.
    TSpellsDlg(CWnd* pParent, EGameVersion mapVersion);

    const std::bitset<kNumSpells>& getSpells() const { return _m_spells; }
    void setSpells(const std::bitset<kNumSpells>& spells);
    // Enables or disables the list's items.
    void enableSpells(bool bEnable);

    enum { IDD = IDD_SPELLS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnCheckChangeSpellsList();
    DECLARE_MESSAGE_MAP()

private:
    EGameVersion _m_mapVersion;
    bool _m_bEnabled;
    std::bitset<kNumSpells> _m_spells;
    CCheckListBox _m_spellsList;
};

#endif  /* HOMM3_EDITOR_SPELLSDLG_H */
