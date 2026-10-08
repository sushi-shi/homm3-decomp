// EditTimedEventSheet.h - the timed event property sheet (Loki
// EditTimedEventSheet.cpp): a copy of the event, its general and resources
// pages, and the modal result, then the vtable pointer (the destructor,
// DoModal, onEnableOK, onDisableOK). The member names are not proven.
#ifndef HOMM3_EDITOR_EDITTIMEDEVENTSHEET_H
#define HOMM3_EDITOR_EDITTIMEDEVENTSHEET_H

#include "editor/stdafx.h"
#include "editor/Player.h"
#include "editor/TimedEvent.h"

class TEditTimedEventGeneralPage;
class TEditTimedEventResourcesPage;

class TEditTimedEventSheet {
public:
    TEditTimedEventSheet(const TPlayerMask& playersPresent, const TTimedEvent& event);
    virtual ~TEditTimedEventSheet();

    void OnOK();
    void OnCancel();
    virtual int DoModal();

    TTimedEvent getEvent() const { return _m_event; }

    virtual void onEnableOK();
    virtual void onDisableOK();

private:
    void _deleteAllPages();

    TTimedEvent _m_event;
    TEditTimedEventGeneralPage* _m_pGeneralPage;
    TEditTimedEventResourcesPage* _m_pResourcesPage;
    int _m_result;
};

// The open sheet, for cppbridge.cpp's timed_event_dlg signal handlers.
extern TEditTimedEventSheet* timedEventSheetModal;

#endif  /* HOMM3_EDITOR_EDITTIMEDEVENTSHEET_H */
