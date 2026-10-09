// EditTimedEventResourcesPage.h - the resources page of the timed event
// sheet (EditTimedEventResourcesPage.cpp; Loki h3maped object 81): it
// creates a TResourceQuantitiesDlg as a child over its frame and forwards
// to it. Layout from the image: the dialog at 0x8c.
#ifndef HOMM3_EDITOR_EDITTIMEDEVENTRESOURCESPAGE_H
#define HOMM3_EDITOR_EDITTIMEDEVENTRESOURCESPAGE_H

#include "editor/resource.h"

class TResourceQuantities;
class TResourceQuantitiesDlg;
class TTimedEvent;

class TEditTimedEventResourcesPage : public CPropertyPage {
public:
    TEditTimedEventResourcesPage(const TTimedEvent& event);
    virtual ~TEditTimedEventResourcesPage();

    const TResourceQuantities& getResourceQuantities() const;
    bool wasModified() const;

    enum { IDD = IDD_EDIT_TIMED_EVENT_RESOURCES };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

private:
    TResourceQuantitiesDlg* _m_pResourceQuantitiesDlg;
};

#endif  /* HOMM3_EDITOR_EDITTIMEDEVENTRESOURCESPAGE_H */
