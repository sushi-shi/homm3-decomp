// HeroPropsSheet.h - the hero property sheets (HeroPropsSheet.cpp; Loki
// h3maped object 87): the creatures, primary skills, secondary skills,
// artifacts and spells pages edit a hero in a copy of the map, found by its
// reference: a hero object or the visiting hero of a town. The random hero,
// specific hero and prison sheets add their general page, own the copy
// and store it in the map when OK is pressed on a changed hero; a specific
// hero's sheet adds the biography page and gives the pages a newly picked
// hero's defaults. Pages that find their entries invalid hold OK off
// until each lets it go.
//
// Layouts from the image: the property sheet, the creatures page's sheet
// interface at 0x88, the map, the reference, the heroes and the map copy,
// the five pages' auto_ptrs from 0xa4, the pages-added flag and the count
// of pages holding OK off (0xd4 bytes); the random hero sheet adds the
// map, reference, copy and general page (0xf0 bytes); the specific hero
// and prison sheets add the general page's sheet interface at 0xd4, the
// map, reference and copy, the heroes, the general and biography pages
// (0x104 bytes).
#ifndef HOMM3_EDITOR_HEROPROPSSHEET_H
#define HOMM3_EDITOR_HEROPROPSSHEET_H

#include <memory>

#include "editor/GameMap.h"
#include "editor/HeroPropsCreaturesPage.h"
#include "editor/HeroPropsGeneralPage.h"
#include "editor/MapObjectRef.h"

class THeroPropsArtifactsPage;
class THeroPropsBiographyPage;
class THeroPropsPriSkillsPage;
class THeroPropsSecSkillsPage;
class THeroPropsSpellsPage;

class THeroPropsSheet : public CPropertySheet, public THeroPropsCreaturesPageParentSheet {
public:
    THeroPropsSheet(LPCTSTR pszCaption, CWnd* pParentWnd, const TGameMap& oldMap, TMapObjectRef heroRef);
    virtual ~THeroPropsSheet();

    bool wasModified() const;

    virtual void onEnableOK();
    virtual void onDisableOK();

    virtual BOOL OnInitDialog();

protected:
    // Points the pages at the hero in the map's copy.
    void _setNewMap(TGameMap* pNewMap);
    // Adds the common pages after the general ones.
    void _addPages(bool bRandomHero);

    THeroPropsPriSkillsPage* _getPPriSkillsPage() { return _m_pPriSkillsPage.get(); }
    THeroPropsSecSkillsPage* _getPSecSkillsPage() { return _m_pSecSkillsPage.get(); }
    THeroPropsArtifactsPage* _getPArtifactsPage() { return _m_pArtifactsPage.get(); }
    THeroPropsSpellsPage* _getPSpellsPage() { return _m_pSpellsPage.get(); }

    DECLARE_MESSAGE_MAP()

private:
    const THero* _getOldHero() const;

    const TGameMap& _m_map;
    TMapObjectRef _m_heroRef;
    const THero* _m_pOldHero;
    TGameMap* _m_pNewMap;
    THero* _m_pNewHero;
    std::auto_ptr<THeroPropsCreaturesPage> _m_pCreaturesPage;
    std::auto_ptr<THeroPropsPriSkillsPage> _m_pPriSkillsPage;
    std::auto_ptr<THeroPropsSecSkillsPage> _m_pSecSkillsPage;
    std::auto_ptr<THeroPropsArtifactsPage> _m_pArtifactsPage;
    std::auto_ptr<THeroPropsSpellsPage> _m_pSpellsPage;
    bool _m_bPagesAdded;
    int _m_numOKDisablers;
};

class TRandomHeroPropsSheet : public THeroPropsSheet {
public:
    TRandomHeroPropsSheet(CWnd* pParentWnd, TGameMap* pMap, TMapObjectRef heroRef, bool bAnyAvailableOwner);
    virtual ~TRandomHeroPropsSheet();

    virtual int DoModal();
    bool wasModified() const;

    virtual BOOL OnInitDialog();

private:
    TGameMap* _m_pMap;
    TMapObjectRef _m_heroRef;
    std::auto_ptr<TGameMap> _m_pNewMap;
    std::auto_ptr<TRandomHeroPropsGeneralPage> _m_pGeneralPage;
};

class TNonRandomHeroPropsSheet : public THeroPropsSheet, public TIdentifiedHeroPropsGeneralPageParentSheet {
public:
    TNonRandomHeroPropsSheet(CWnd* pParentWnd, TGameMap* pMap, TMapObjectRef heroRef, bool bAnyAvailableOwner);
    virtual ~TNonRandomHeroPropsSheet();

    virtual int DoModal();
    bool wasModified() const;

    virtual void onEnableOK();
    virtual void onDisableOK();
    virtual void onSetHeroID(THeroID heroID);

    virtual BOOL OnInitDialog();

private:
    TGameMap* _m_pMap;
    TMapObjectRef _m_heroRef;
    std::auto_ptr<TGameMap> _m_pNewMap;
    const THero* _m_pOldHero;
    THero* _m_pNewHero;
    std::auto_ptr<TNonRandomHeroPropsGeneralPage> _m_pGeneralPage;
    std::auto_ptr<THeroPropsBiographyPage> _m_pBiographyPage;
};

class TPrisonPropsSheet : public THeroPropsSheet, public TIdentifiedHeroPropsGeneralPageParentSheet {
public:
    TPrisonPropsSheet(CWnd* pParentWnd, TGameMap* pMap, TMapObjectRef prisonRef);
    virtual ~TPrisonPropsSheet();

    virtual int DoModal();
    bool wasModified() const;

    virtual void onEnableOK();
    virtual void onDisableOK();
    virtual void onSetHeroID(THeroID heroID);

    virtual BOOL OnInitDialog();

private:
    TGameMap* _m_pMap;
    TMapObjectRef _m_prisonRef;
    std::auto_ptr<TGameMap> _m_pNewMap;
    const TPrison* _m_pOldPrison;
    TPrison* _m_pNewPrison;
    std::auto_ptr<TPrisonPropsGeneralPage> _m_pGeneralPage;
    std::auto_ptr<THeroPropsBiographyPage> _m_pBiographyPage;
};

#endif  /* HOMM3_EDITOR_HEROPROPSSHEET_H */
