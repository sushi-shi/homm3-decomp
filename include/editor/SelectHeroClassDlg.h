// SelectHeroClassDlg.h - the hero class picker (SelectHeroClassDlg.cpp;
// Loki h3maped object 90): lists the classes a mask allows, the random
// class last. Layout from the image: the OK button at 0x5c, the class list
// at 0x98, then the mask, the map's version and the chosen class (0xe0
// bytes).
#ifndef HOMM3_EDITOR_SELECTHEROCLASSDLG_H
#define HOMM3_EDITOR_SELECTHEROCLASSDLG_H

#include <bitset>

#include "gameversion.h"
#include "heroclass.h"
#include "editor/resource.h"

// The hero classes, and the random class after them.
typedef std::bitset<kNumHeroClasses + 1> THeroClassMask;

class TSelectHeroClassDlg : public CDialog {
public:
    TSelectHeroClassDlg(CWnd* pParent, const THeroClassMask& mask, EGameVersion mapVersion);

    THeroClass getHeroClass() const { return _m_heroClass; }

    enum { IDD = IDD_SELECT_HERO_CLASS };
    CButton _m_okButton;
    CListBox _m_heroClassList;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnSelChangeHeroClassList();
    afx_msg void OnSelCancelHeroClassList();
    afx_msg void OnDblClkHeroClassList();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

private:
    THeroClassMask _m_mask;
    EGameVersion _m_mapVersion;
    THeroClass _m_heroClass;
};

#endif  /* HOMM3_EDITOR_SELECTHEROCLASSDLG_H */
