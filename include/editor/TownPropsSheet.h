// TownPropsSheet.h - the town property sheet (TownPropsSheet.cpp; Loki
// h3maped object 90): the general, garrison, buildings, spells and timed
// events pages edit the town in a copy of the map, which OK stores back
// when a page changed it. The general page holds OK off while the custom
// name is blank. Layout from the image: the property sheet, the general
// page's sheet interface at 0x88, the map, its copy and the five pages
// (0xc0 bytes).
#ifndef HOMM3_EDITOR_TOWNPROPSSHEET_H
#define HOMM3_EDITOR_TOWNPROPSSHEET_H

#include <memory>

#include "editor/GameMap.h"
#include "editor/MapObjectRef.h"
#include "editor/TownPropsGeneralPage.h"

class TTownPropsGarrisonPage;
class TTownPropsBuildingsPage;
class TTownPropsSpellsPage;
class TTownPropsTimedEventsPage;

class TTownPropsSheet : public CPropertySheet, public TTownPropsGeneralPageParentSheet {
public:
    TTownPropsSheet(CWnd* pParentWnd, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual ~TTownPropsSheet();

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
    std::auto_ptr<TTownPropsGeneralPage> _m_pGeneralPage;
    std::auto_ptr<TTownPropsGarrisonPage> _m_pGarrisonPage;
    std::auto_ptr<TTownPropsBuildingsPage> _m_pBuildingsPage;
    std::auto_ptr<TTownPropsSpellsPage> _m_pSpellsPage;
    std::auto_ptr<TTownPropsTimedEventsPage> _m_pTimedEventsPage;
};

#endif  /* HOMM3_EDITOR_TOWNPROPSSHEET_H */
