// ArtifactsDlg.h - the artifacts list of the hero and hero prototype
// artifacts pages (ArtifactsDlg.cpp; GOG only, Loki's pages keep their own
// list): a modeless child dialog over the page's frame listing the worn
// and carried artifacts by name and location, with the spellbook check and
// the add, edit, remove and remove-all buttons. It keeps the slots no
// artifact takes and, per slot class, the slots the worn combination
// artifacts' other components need, which the artifact editor checks.
//
// Layout from the image: the controls from 0x5c, the map version at
// 0x1c4, the artifacts at 0x1c8, the unused slots at 0x224 and the slot
// classes' counts at 0x228 (0x264 bytes).
#ifndef HOMM3_EDITOR_ARTIFACTSDLG_H
#define HOMM3_EDITOR_ARTIFACTSDLG_H

#include <bitset>

#include "artifact.h"
#include "gameversion.h"
#include "editor/Array.h"
#include "editor/Hero.h"
#include "editor/resource.h"

class TArtifactsDlg : public CDialog {
public:
    // The location a backpack artifact lists.
    enum { s_kBackpackSlot = H3_IDX(kNumArtifactSlots) + 1, s_kMaxBackpackSize = 64, s_kNumSlotClasses = 15 };

    typedef std::bitset<H3_IDX(kNumArtifactSlots) + 1> TSlots;
    typedef TArray<int, s_kNumSlotClasses> TSlotClassCounts;

    TArtifactsDlg(CWnd* pParent, EGameVersion mapVersion);

    void setArtifacts(const THeroPrototype::TArtifactContainer& artifacts);
    const THeroPrototype::TArtifactContainer& getArtifacts() const { return _m_artifacts; }

    enum { IDD = IDD_ARTIFACTS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnAddButton();
    afx_msg void OnEditButton();
    afx_msg void OnRemoveButton();
    afx_msg void OnRemoveAllButton();
    afx_msg void OnItemChangedList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnColumnClickList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKeydownList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnHasSpellbookCheck();
    afx_msg void OnEnable(BOOL bEnable);
    DECLARE_MESSAGE_MAP()

private:
    void _enableControls();
    void _disableControls();
    int _insertItem(int index, TArtifact artifact, TArtifactSlot slot);
    void _removeItem(int index);
    void _updateUnusedSlots();

    CButton _m_hasSpellbookCheck;
    CListCtrl _m_list;
    CButton _m_removeAllButton;
    CButton _m_removeButton;
    CButton _m_editButton;
    CButton _m_addButton;
    EGameVersion _m_mapVersion;
    THeroPrototype::TArtifactContainer _m_artifacts;
    TSlots _m_unusedSlots;
    TSlotClassCounts _m_slotClassCounts;
};

#endif  /* HOMM3_EDITOR_ARTIFACTSDLG_H */
