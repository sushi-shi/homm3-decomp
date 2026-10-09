// EditTimedEventSheet.h - the timed event property sheet
// (EditTimedEventSheet.cpp; Loki h3maped object 79): the general and
// resources pages over a copy of the event, which DoModal updates from
// the pages that changed. The sheet is the general page's parent: a blank
// name disables OK. Layout from the image: the parent interface at 0x88,
// the event at 0x8c and the two pages' auto_ptrs at 0xd8 and 0xe0.
#ifndef HOMM3_EDITOR_EDITTIMEDEVENTSHEET_H
#define HOMM3_EDITOR_EDITTIMEDEVENTSHEET_H

#include <memory>

#include "gameversion.h"
#include "editor/EditTimedEventGeneralPage.h"
#include "editor/EditTimedEventResourcesPage.h"
#include "editor/Player.h"
#include "editor/TimedEvent.h"

class TEditTimedEventSheet : public CPropertySheet, public TEditTimedEventGeneralPageParentSheet {
public:
    TEditTimedEventSheet(CWnd* pParentWnd, const TPlayerMask& playersPresent, EGameVersion mapVersion,
                         const TTimedEvent& event);
    virtual ~TEditTimedEventSheet();

    virtual int DoModal();

    const TTimedEvent& getEvent() const { return _m_event; }

    virtual void onEnableOK() { GetDlgItem(IDOK)->EnableWindow(TRUE); }
    virtual void onDisableOK() { GetDlgItem(IDOK)->EnableWindow(FALSE); }

protected:
    DECLARE_MESSAGE_MAP()

private:
    TTimedEvent _m_event;
    std::auto_ptr<TEditTimedEventGeneralPage> _m_pGeneralPage;
    std::auto_ptr<TEditTimedEventResourcesPage> _m_pResourcesPage;
};

#endif  /* HOMM3_EDITOR_EDITTIMEDEVENTSHEET_H */
