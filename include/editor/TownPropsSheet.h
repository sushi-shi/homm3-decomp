// TownPropsSheet.h - the town property sheet (Loki TownPropsSheet.cpp):
// the town, the map, the town's layer and object id, its general,
// garrison, buildings, spells and timed events pages and the modal result,
// then the vtable pointer (the destructor, DoModal). The member names are
// not proven.
#ifndef HOMM3_EDITOR_TOWNPROPSSHEET_H
#define HOMM3_EDITOR_TOWNPROPSSHEET_H

#include "editor/stdafx.h"

class TGameMap;
class TTown;
class TTownPropsGeneralPage;
class TTownPropsGarrisonPage;
class TTownPropsBuildingsPage;
class TTownPropsSpellsPage;
class TTownPropsTimedEventsPage;

class TTownPropsSheet {
public:
    TTownPropsSheet(GtkWidget* thisWidget, TTown* pTown, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual ~TTownPropsSheet();

    bool wasModified() const;

    void OnOK();
    void OnCancel();
    virtual int DoModal();

private:
    void _deleteAllPages();

    TTown* _m_pTown;
    TGameMap* _m_pMap;
    bool _m_bSecondLayer;
    unsigned int _m_objID;

public:
    TTownPropsGeneralPage* _m_pGeneralPage;
    TTownPropsGarrisonPage* _m_pGarrisonPage;
    TTownPropsBuildingsPage* _m_pBuildingsPage;
    TTownPropsSpellsPage* _m_pSpellsPage;
    TTownPropsTimedEventsPage* _m_pTimedEventsPage;

private:
    int _m_result;
};

// The open sheet, for cppbridge.cpp's town_props_dlg signal handlers.
extern TTownPropsSheet* townPropsSheetModal;

#endif  /* HOMM3_EDITOR_TOWNPROPSSHEET_H */
