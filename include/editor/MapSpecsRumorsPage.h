// MapSpecsRumorsPage.h - the rumors page of the map specifications sheet
// (MapSpecsRumorsPage.cpp; Loki h3maped object 73): the rumors by name,
// each list item keeping its rumor's index, edited through TEditRumorDlg.
// Layout from the image: the add, edit, remove-all and remove buttons and
// the list from 0x8c, the map before and during the sheet at 0x1b8 and
// 0x1bc, the modified flag and the rumors at 0x1c4.
#ifndef HOMM3_EDITOR_MAPSPECSRUMORSPAGE_H
#define HOMM3_EDITOR_MAPSPECSRUMORSPAGE_H

#include <vector>

#include "editor/GameMap.h"
#include "editor/resource.h"

class TMapSpecsRumorsPage : public CPropertyPage {
public:
    enum { s_kMaxNumRumors = 30 };

    TMapSpecsRumorsPage(const TGameMap& oldMap, TGameMap& newMap);
    virtual ~TMapSpecsRumorsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MAP_SPECS_RUMORS };
    CButton _m_addButton;
    CButton _m_editButton;
    CButton _m_removeAllButton;
    CButton _m_removeButton;
    CListBox _m_rumorsList;

    virtual void OnOK();
    virtual BOOL OnInitDialog();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnAddRumorButton();
    afx_msg void OnEditRumorButton();
    afx_msg void OnRemoveRumorButton();
    afx_msg void OnRemoveAllRumorButton();
    afx_msg void OnSelChangeRumorsList();
    afx_msg void OnSelCancelRumorsList();
    afx_msg void OnDblClkRumorsList();
    afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
    DECLARE_MESSAGE_MAP()

private:
    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    bool _m_bModified;
    std::vector<TRumor> _m_rumors;
};

#endif  /* HOMM3_EDITOR_MAPSPECSRUMORSPAGE_H */
