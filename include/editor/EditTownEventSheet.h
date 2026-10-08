// EditTownEventSheet.h - the town event property sheet (Loki
// EditTownEventSheet.cpp): a copy of the event and its general, resources,
// buildings and creatures pages over the timed_event_dlg glade dialog,
// then the modal result and the vtable pointer (the destructor, DoModal).
// cppbridge.cpp reaches the buildings page through the sheet directly, so
// the pages are public. The member names are not proven.
#ifndef HOMM3_EDITOR_EDITTOWNEVENTSHEET_H
#define HOMM3_EDITOR_EDITTOWNEVENTSHEET_H

#include "editor/stdafx.h"
#include "editor/Player.h"
#include "editor/Town.h"

class TEditTimedEventGeneralPage;
class TEditTimedEventResourcesPage;
class TEditTownEventBuildingsPage;
class TEditTownEventCreaturesPage;

class TEditTownEventSheet {
public:
    TEditTownEventSheet(GtkWidget* pParent, const TPlayerMask& playersPresent, TTownType townType,
                        const TTown::TTimedEvent& event = TTown::TTimedEvent());
    virtual ~TEditTownEventSheet();

    virtual int DoModal();

    TTown::TTimedEvent getEvent() const { return _m_event; }

private:
    void _deleteAllPages();

    TTown::TTimedEvent _m_event;

public:
    TEditTimedEventGeneralPage* _m_pGeneralPage;
    TEditTimedEventResourcesPage* _m_pResourcesPage;
    TEditTownEventBuildingsPage* _m_pBuildingsPage;
    TEditTownEventCreaturesPage* _m_pCreaturesPage;

private:
    int _m_result;
};

// The open sheet, for cppbridge.cpp's town event signal handlers.
extern TEditTownEventSheet* editTownEventSheetModal;

#endif  /* HOMM3_EDITOR_EDITTOWNEVENTSHEET_H */
