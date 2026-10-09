// MapSpecsSheet.h - the map specifications sheet (MapSpecsSheet.cpp; Loki
// h3maped object 69): the general, players, teams, rumors, timed events,
// victory and loss condition, heroes, artifacts, spells and secondary
// skills pages edit a copy of the map, which OK stores back when a page
// changed it. The sheet collects the map's towns, heroes and monsters for
// the condition pages, and lets the players page hold OK off. Layout from
// the image: the property sheet, the players page's sheet interface at
// 0x88, the map, its copy, the count of pages holding OK off, the eleven
// pages and the towns, heroes and monsters (0x124 bytes).
#ifndef HOMM3_EDITOR_MAPSPECSSHEET_H
#define HOMM3_EDITOR_MAPSPECSSHEET_H

#include <memory>
#include <vector>

#include "editor/GameMap.h"
#include "editor/MapObjectRef.h"
#include "editor/MapSpecsPlayerSpecsPage.h"

class TMapSpecsGeneralPage;
class TMapSpecsTeamsPage;
class TMapSpecsRumorsPage;
class TMapSpecsTimedEventsPage;
class TMapSpecsVictoryCondPage;
class TMapSpecsLossCondPage;
class TMapSpecsAvailableHeroesPage;
class TMapSpecsArtifactsPage;
class TMapSpecsSpellsPage;
class TMapSpecsSecSkillsPage;

class TMapSpecsSheet : public CPropertySheet, public TMapSpecsPlayerSpecsPage::TParentSheet {
public:
    TMapSpecsSheet(CWnd* pParentWnd, TGameMap* pMap, int terrainStrength);
    virtual ~TMapSpecsSheet();

    // Stores the edited copy in the map when OK is pressed on a changed
    // one.
    virtual int DoModal();
    bool wasModified() const;

    virtual void onEnableOK();
    virtual void onDisableOK();

protected:
    DECLARE_MESSAGE_MAP()

private:
    TGameMap* _m_pMap;
    std::auto_ptr<TGameMap> _m_pNewMap;
    int _m_numOKDisablers;
    std::auto_ptr<TMapSpecsGeneralPage> _m_pGeneralPage;
    std::auto_ptr<TMapSpecsPlayerSpecsPage> _m_pPlayerSpecsPage;
    std::auto_ptr<TMapSpecsTeamsPage> _m_pTeamsPage;
    std::auto_ptr<TMapSpecsRumorsPage> _m_pRumorsPage;
    std::auto_ptr<TMapSpecsTimedEventsPage> _m_pTimedEventsPage;
    std::auto_ptr<TMapSpecsVictoryCondPage> _m_pVictoryCondPage;
    std::auto_ptr<TMapSpecsLossCondPage> _m_pLossCondPage;
    std::auto_ptr<TMapSpecsAvailableHeroesPage> _m_pAvailableHeroesPage;
    std::auto_ptr<TMapSpecsArtifactsPage> _m_pArtifactsPage;
    std::auto_ptr<TMapSpecsSpellsPage> _m_pSpellsPage;
    std::auto_ptr<TMapSpecsSecSkillsPage> _m_pSecSkillsPage;
    std::vector<TMapObjectRef> _m_aTownsOnMap;
    std::vector<TMapObjectRef> _m_aHeroesOnMap;
    std::vector<TMapObjectRef> _m_aMonstersOnMap;
};

#endif  /* HOMM3_EDITOR_MAPSPECSSHEET_H */
