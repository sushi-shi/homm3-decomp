// EditTimedEventResourcesPage.h - the resources page of the timed event
// sheet (Loki EditTimedEventResourcesPage.cpp): it owns a
// TResourceQuantitiesDlg over the timed_event glade widgets and forwards to
// it. Layout from the image: the dialog pointer, then the vtable pointer
// (OnOK, OnInitDialog). The member name is not proven.
#ifndef HOMM3_EDITOR_EDITTIMEDEVENTRESOURCESPAGE_H
#define HOMM3_EDITOR_EDITTIMEDEVENTRESOURCESPAGE_H

#include "editor/stdafx.h"
#include "editor/ResourceQuantities.h"

class TResourceQuantitiesDlg;
class TTimedEvent;

class TEditTimedEventResourcesPage {
public:
    TEditTimedEventResourcesPage(const TTimedEvent& event);
    ~TEditTimedEventResourcesPage();

    const TResourceQuantities& getResourceQuantities() const;
    bool wasModified() const;

    void OnDestroy();
    virtual void OnOK();
    virtual BOOL OnInitDialog();

private:
    TResourceQuantitiesDlg* _m_pResourceQuantitiesDlg;
};

#endif  /* HOMM3_EDITOR_EDITTIMEDEVENTRESOURCESPAGE_H */
