// EditArtifactDlg.h - the artifact editor of the hero artifacts page (Loki
// EditArtifactDlg.cpp): the edit_artifact_dlg glade dialog picks a slot
// ("where": the backpack or an unused slot) and an artifact allowed there.
// Layout from the image: the artifact combo's row-to-artifact table, the
// where combo's row-to-slot table (1000 rows each), the modal result, the
// artifact and slot (_m_artifact, _m_slot), the unused slots
// (_m_unusedSlots) and the backpack-full flag (_m_bBackpackFull), then the
// vtable pointer (OnInitDialog, OnOK, OnCancel). The table and result names
// and the tables' bound are not proven.
#ifndef HOMM3_EDITOR_EDITARTIFACTDLG_H
#define HOMM3_EDITOR_EDITARTIFACTDLG_H

#include "editor/stdafx.h"

#include <set>

#include "artifact.h"

// "TEditArtifactDlg::TEditArtifactDlg(CWnd *, const TArtifactSlotSet &, bool)"
typedef set<TArtifactSlot, less<TArtifactSlot> > TArtifactSlotSet;

class TEditArtifactDlg {
public:
    TEditArtifactDlg(CWnd* pParent, const TArtifactSlotSet& unusedSlots, bool bBackpackFull);
    TEditArtifactDlg(CWnd* pParent, const TArtifactSlotSet& unusedSlots, bool bBackpackFull,
                     TArtifact artifact, TArtifactSlot slot);

    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    int DoModal();
    void OnSelChangeWhereCombo();
    void OnSelChangeArtifactCombo();

    TArtifact getArtifact() const { return _m_artifact; }
    TArtifactSlot getSlot() const { return _m_slot; }

private:
    enum { _s_kMaxListItems = 1000 };

    void _setArtifacts();

    int _m_listArtifacts[_s_kMaxListItems];
    int _m_listSlots[_s_kMaxListItems];
    volatile int _m_result;
    TArtifact _m_artifact;
    TArtifactSlot _m_slot;
    TArtifactSlotSet _m_unusedSlots;
    bool _m_bBackpackFull;
};

// The open dialog, for cppbridge.cpp's edit_artifact_dlg signal handlers.
extern TEditArtifactDlg* editArtifactDlg;

#endif  /* HOMM3_EDITOR_EDITARTIFACTDLG_H */
