// FindDlg.h - the Edit > Find dialog (FindDlg.cpp): a list of what to
// find, each entry a pattern matching objects by type (and subtype). The
// map view searches with the chosen entry's index (Find Next and Find
// Previous) and seeds the dialog from the selected object. Loki's port
// keeps a simpler table of names, types and subtypes. Layout from the
// image: the Find Previous and Find Next buttons and the list from 0x5c,
// the entry at 0x110 and the direction at 0x114.
#ifndef HOMM3_EDITOR_FINDDLG_H
#define HOMM3_EDITOR_FINDDLG_H

#include "editor/resource.h"

class TGameObject;

class TFindDlg : public CDialog {
public:
    TFindDlg(CWnd* pParent, int findType);

    // The first entry that finds the object, or -1.
    static int findType(const TGameObject& object);
    static bool matches(const TGameObject& object, int findType);

    int getFindType() const { return _m_findType; }
    bool getBSearchBackwards() const { return _m_bSearchBackwards; }

    enum { IDD = IDD_FIND };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnFindNextButton();
    afx_msg void OnFindPrevButton();
    afx_msg void OnDblClkFindWhatList();
    afx_msg void OnSelChangeFindWhatList();
    afx_msg void OnSelCancelFindWhatList();
    DECLARE_MESSAGE_MAP()

private:
    CButton _m_findPrevButton;
    CButton _m_findNextButton;
    CListBox _m_findWhatList;
    int _m_findType;
    bool _m_bSearchBackwards;
};

#endif  /* HOMM3_EDITOR_FINDDLG_H */
