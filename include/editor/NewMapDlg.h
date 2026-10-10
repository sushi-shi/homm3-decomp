// NewMapDlg.h - the new map dialog (NewMapDlg.cpp, h3maped
// 0x48a5ac..0x48b8ef; not in Loki): the new map's version, size and
// levels, and the random map generator's settings. Declared as far as
// the map document uses it; the layout is its constructor's (0x48a7aa)
// and destructor's (0x48a92d). It runs from an in-memory copy of its
// dialog template whose size it sets before InitModalIndirect.
#ifndef HOMM3_EDITOR_NEWMAPDLG_H
#define HOMM3_EDITOR_NEWMAPDLG_H

#include "editor/MapDoc.h"

class TNewMapDlg : public CDialog {
public:
    TNewMapDlg(CWnd* pParent, const TMapDoc::TNewMapParams& params);
    virtual ~TNewMapDlg();

    const TMapDoc::TNewMapParams& getParams() const { return _m_params; }

private:
    // DoDataExchange (0x48a9d0) binds the check box 1822 and the combo
    // boxes 1855..1852 in this order, then the radio groups and the
    // two-level check box. The combo boxes are the random map's player and
    // team counts; which count each one holds is not proven.
    CButton _m_randomMapCheck;
    CComboBox _m_humanPlayerCountCombo;
    CComboBox _m_humanTeamCountCombo;
    CComboBox _m_computerPlayerCountCombo;
    CComboBox _m_computerTeamCountCombo;
    int _m_size;
    int _m_bTwoLevel;
    int _m_version;
    int _m_waterContent;
    int _m_monsterStrength;
    TMapDoc::TNewMapParams _m_params;
    DLGTEMPLATE* _m_pTemplate;
    // The window's sizes without and with the random map settings (from
    // OnInitDialog's window rectangles, 0x48ad17 and 0x48ad77).
    CSize _m_collapsedSize;
    CSize _m_expandedSize;
};

#endif  /* HOMM3_EDITOR_NEWMAPDLG_H */
