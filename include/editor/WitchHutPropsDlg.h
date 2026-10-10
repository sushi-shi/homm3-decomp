// WitchHutPropsDlg.h - the witch hut's skills dialog (WitchHutPropsDlg.cpp;
// GOG only, Loki's port has no counterpart): the secondary skills the hut
// may teach, at least one of them. Maps before Armageddon's Blade show the
// list disabled. Layout from the image: the hut at 0x5c, the map version at
// 0x60, the modified flag at 0x64, the potential skills at 0x68 and the
// check list at 0x6c.
#ifndef HOMM3_EDITOR_WITCHHUTPROPSDLG_H
#define HOMM3_EDITOR_WITCHHUTPROPSDLG_H

#include <bitset>

#include "gameversion.h"
#include "secondaryskill.h"
#include "editor/resource.h"

class TWitchHut;

class TWitchHutPropsDlg : public CDialog {
public:
    TWitchHutPropsDlg(CWnd* pParent, TWitchHut* pWitchHut, EGameVersion version);

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_WITCH_HUT_PROPS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnCheckChangeSkillsList();
    DECLARE_MESSAGE_MAP()

private:
    TWitchHut* _m_pWitchHut;
    EGameVersion _m_version;
    bool _m_bModified;
    std::bitset<kNumSecSkills> _m_abPotentialSkill;
    CCheckListBox _m_skillsList;
};

#endif  /* HOMM3_EDITOR_WITCHHUTPROPSDLG_H */
