// EditArtifactDlg.h - the artifact editor of the hero artifacts page
// (EditArtifactDlg.cpp; Loki h3maped object 82): picks where the artifact
// goes, the backpack (unless full) or an unused slot of the map version,
// and an artifact allowed there. A combination artifact needs a free slot
// of each of its components' slot classes beyond those its other worn
// components already take. Windows passes the unused slots as a slot mask
// and the slot classes' counts of the hero's worn combination parts.
//
// Layout from the image: the OK button at 0x5c, the artifact and where
// combos at 0x98 and 0xd4, the artifact and slot at 0x110, the unused
// slots and the slot-class counts at 0x118, the backpack-full flag at
// 0x120 and the map version at 0x124.
#ifndef HOMM3_EDITOR_EDITARTIFACTDLG_H
#define HOMM3_EDITOR_EDITARTIFACTDLG_H

#include <bitset>

#include "artifact.h"
#include "gameversion.h"
#include "editor/Array.h"
#include "editor/resource.h"

class TEditArtifactDlg : public CDialog {
public:
    // The slot "where" names the backpack with.
    enum { s_kBackpackSlot = H3_IDX(kNumArtifactSlots) + 1, s_kNumSlotClasses = 15 };

    typedef std::bitset<H3_IDX(kNumArtifactSlots) + 1> TSlots;
    typedef TArray<int, s_kNumSlotClasses> TSlotClassCounts;

    TEditArtifactDlg(CWnd* pParent, const TSlots& unusedSlots, const TSlotClassCounts& slotClassCounts,
                     bool bBackpackFull, EGameVersion mapVersion);
    TEditArtifactDlg(CWnd* pParent, const TSlots& unusedSlots, const TSlotClassCounts& slotClassCounts,
                     bool bBackpackFull, EGameVersion mapVersion, TArtifact artifact, TArtifactSlot slot);

    TArtifact getArtifact() const { return _m_artifact; }
    TArtifactSlot getSlot() const { return _m_slot; }

    enum { IDD = IDD_EDIT_ARTIFACT };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSelChangeWhereCombo();
    afx_msg void OnSelChangeArtifactCombo();
    DECLARE_MESSAGE_MAP()

private:
    void _setArtifacts();
    bool _isAllowed(TArtifact artifact, TArtifactSlot slot) const;

    CButton _m_okButton;
    CComboBox _m_artifactCombo;
    CComboBox _m_whereCombo;
    TArtifact _m_artifact;
    TArtifactSlot _m_slot;
    const TSlots& _m_unusedSlots;
    const TSlotClassCounts& _m_slotClassCounts;
    bool _m_bBackpackFull;
    EGameVersion _m_mapVersion;
};

#endif  /* HOMM3_EDITOR_EDITARTIFACTDLG_H */
