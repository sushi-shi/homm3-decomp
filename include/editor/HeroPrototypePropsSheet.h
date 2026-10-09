// HeroPrototypePropsSheet.h - the properties sheet of one hero's map
// definition (HeroPrototypePropsSheet.cpp; GOG only): the general,
// biography, primary skills, secondary skills, artifacts and spells pages
// edit a copy of the map's THeroPrototype, which OK stores back when a page
// changed it. The general page has the sheet disable OK while its name is
// blank. Layout from the image: the property sheet, the general page's
// sheet interface at 0x88, the map, the hero, the edited prototype, the
// count of pages holding OK off and the six pages (0xcc bytes).
//
// Ported so far: the declarations the map specifications' heroes page
// uses.
#ifndef HOMM3_EDITOR_HEROPROTOTYPEPROPSSHEET_H
#define HOMM3_EDITOR_HEROPROTOTYPEPROPSSHEET_H

#include <memory>

#include "editor/GameMap.h"
#include "editor/Hero.h"

class THeroPrototypePropsBiographyPage;
class THeroPrototypePropsPriSkillsPage;
class THeroPrototypePropsSecSkillsPage;
class THeroPrototypePropsArtifactsPage;
class THeroPrototypePropsSpellsPage;

// Ported so far: the sheet interface.
class THeroPrototypePropsGeneralPage {
public:
    // The sheet, told when its OK button may be pressed.
    class TParentSheet {
    public:
        virtual void onEnableOK() = 0;
        virtual void onDisableOK() = 0;
    };
};

class THeroPrototypePropsSheet : public CPropertySheet, public THeroPrototypePropsGeneralPage::TParentSheet {
public:
    THeroPrototypePropsSheet(CWnd* pParentWnd, TGameMap& map, THeroID heroID);
    virtual ~THeroPrototypePropsSheet();

    // Stores the edited prototype in the map when OK is pressed on a
    // changed one.
    virtual int DoModal();
    bool wasModified() const;

    virtual void onEnableOK();
    virtual void onDisableOK();

protected:
    DECLARE_MESSAGE_MAP()

private:
    TGameMap& _m_map;
    THeroID _m_heroID;
    THeroPrototype _m_prototype;
    int _m_numOKDisablers;
    std::auto_ptr<THeroPrototypePropsGeneralPage> _m_pGeneralPage;
    std::auto_ptr<THeroPrototypePropsBiographyPage> _m_pBiographyPage;
    std::auto_ptr<THeroPrototypePropsPriSkillsPage> _m_pPriSkillsPage;
    std::auto_ptr<THeroPrototypePropsSecSkillsPage> _m_pSecSkillsPage;
    std::auto_ptr<THeroPrototypePropsArtifactsPage> _m_pArtifactsPage;
    std::auto_ptr<THeroPrototypePropsSpellsPage> _m_pSpellsPage;
};

#endif  /* HOMM3_EDITOR_HEROPROTOTYPEPROPSSHEET_H */
