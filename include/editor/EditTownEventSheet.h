// EditTownEventSheet.h - the town event property sheet
// (EditTownEventSheet.cpp; Loki h3maped object 97): the timed event
// sheet's general and resources pages plus the buildings and creatures
// pages, over a copy of the event that DoModal updates from the pages that
// changed. Layout from the image: the parent interface at 0x88, the event
// at 0x8c and the four pages' auto_ptrs from 0xfc.
#ifndef HOMM3_EDITOR_EDITTOWNEVENTSHEET_H
#define HOMM3_EDITOR_EDITTOWNEVENTSHEET_H

#include <memory>

#include "gameversion.h"
#include "editor/EditTimedEventGeneralPage.h"
#include "editor/EditTimedEventResourcesPage.h"
#include "editor/EditTownEventBuildingsPage.h"
#include "editor/EditTownEventCreaturesPage.h"
#include "editor/Player.h"
#include "editor/Town.h"

class TEditTownEventSheet : public CPropertySheet, public TEditTimedEventGeneralPageParentSheet {
public:
    TEditTownEventSheet(CWnd* pParentWnd, const TPlayerMask& playersPresent, TTownType townType,
                        EGameVersion mapVersion, const TTown::TTimedEvent& event);
    virtual ~TEditTownEventSheet();

    virtual int DoModal();

    TTown::TTimedEvent getEvent() const { return _m_event; }

    virtual void onEnableOK() { GetDlgItem(IDOK)->EnableWindow(TRUE); }
    virtual void onDisableOK() { GetDlgItem(IDOK)->EnableWindow(FALSE); }

protected:
    DECLARE_MESSAGE_MAP()

private:
    TTown::TTimedEvent _m_event;
    std::auto_ptr<TEditTimedEventGeneralPage> _m_pGeneralPage;
    std::auto_ptr<TEditTimedEventResourcesPage> _m_pResourcesPage;
    std::auto_ptr<TEditTownEventBuildingsPage> _m_pBuildingsPage;
    std::auto_ptr<TEditTownEventCreaturesPage> _m_pCreaturesPage;
};

#endif  /* HOMM3_EDITOR_EDITTOWNEVENTSHEET_H */
