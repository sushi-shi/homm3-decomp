// MapSpecsSheet.h - the map specifications sheet (Loki MapSpecsSheet.cpp):
// the map_specs glade dialog's notebook of the general, player, teams,
// rumors, timed events and loss condition pages. A CWnd over the dialog
// widget: after the CWnd part come the map (_m_pMap, the constructor's
// assert), the six page pointers (with an unused slot before the loss
// condition page, cleared and never read), and the towns, heroes and
// monsters on the map that OnInitDialog collects for the loss condition
// page; then the vtable's destructor, OnInitDialog and OnOK. cppbridge.cpp
// creates the sheet with new, runs OnInitDialog and OnOK through the
// vtable, reads the page pointers and deletes the sheet. The names other
// than _m_pMap are not proven.
#ifndef HOMM3_EDITOR_MAPSPECSSHEET_H
#define HOMM3_EDITOR_MAPSPECSSHEET_H

#include "editor/stdafx.h"

#include <vector>

#include "editor/MapObjectRef.h"

class TGameMap;
class TMapSpecsGeneralPage;
class TMapSpecsPlayerSpecsPage;
class TMapSpecsTeamsPage;
class TMapSpecsRumorsPage;
class TMapSpecsTimedEventsPage;
class TMapSpecsVictoryCondPage;
class TMapSpecsLossCondPage;

class TMapSpecsSheet : public CWnd {
public:
    TMapSpecsSheet(GtkWidget* thisWidget, TGameMap* pMap);
    virtual ~TMapSpecsSheet();

    virtual void OnInitDialog();
    virtual int OnOK();

    bool wasModified() const;

    TMapSpecsLossCondPage* getLossCondPage() { return _m_pLossCondPage; }

    TGameMap* _m_pMap;
    TMapSpecsGeneralPage* _m_pGeneralPage;
    TMapSpecsPlayerSpecsPage* _m_pPlayerSpecsPage;
    TMapSpecsTeamsPage* _m_pTeamsPage;
    TMapSpecsRumorsPage* _m_pRumorsPage;
    TMapSpecsTimedEventsPage* _m_pTimedEventsPage;
    TMapSpecsVictoryCondPage* _m_pVictoryCondPage;
    TMapSpecsLossCondPage* _m_pLossCondPage;

private:
    void _deleteAllPages();

    vector<TMapObjectRef> _m_aTownsOnMap;
    vector<TMapObjectRef> _m_aHeroesOnMap;
    vector<TMapObjectRef> _m_aMonstersOnMap;
};

#endif  /* HOMM3_EDITOR_MAPSPECSSHEET_H */
