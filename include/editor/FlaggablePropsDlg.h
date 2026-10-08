// FlaggablePropsDlg.h - the owner dialog of flaggable objects
// (FlaggablePropsDlg.cpp; GOG only, Loki's port has no counterpart): a
// radio group of "none" and the eight players. Layout from the image: the
// DDX radio index (the owner plus one) at 0x5c, the object at 0x60 and the
// modified flag at 0x64.
#ifndef HOMM3_EDITOR_FLAGGABLEPROPSDLG_H
#define HOMM3_EDITOR_FLAGGABLEPROPSDLG_H

#include "editor/resource.h"

class TFlaggableObject;

class TFlaggablePropsDlg : public CDialog {
public:
    TFlaggablePropsDlg(CWnd* pParent, TFlaggableObject* pFlaggable);

    virtual int DoModal();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_FLAGGABLE_PROPS };
    int _m_owner;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDoubleClickedOwnerRadio();
    DECLARE_MESSAGE_MAP()

private:
    TFlaggableObject* _m_pFlaggable;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_FLAGGABLEPROPSDLG_H */
