// HeroPropsSheet.cpp - the hero property sheets (h3maped 0x453840..0x454c95;
// Loki h3maped object 87). The prison sheet's members other than its
// constructor and destructor fold with the specific hero sheet's.
#include "editor/stdafx.h"

#include "adventureobjecttype.h"
#include "exceptions.h"
#include "objnames.h"
#include "va.h"
#include "editor/FormattedString.h"
#include "editor/HeroPropsArtifactsPage.h"
#include "editor/HeroPropsBiographyPage.h"
#include "editor/HeroPropsPriSkillsPage.h"
#include "editor/HeroPropsSecSkillsPage.h"
#include "editor/HeroPropsSheet.h"
#include "editor/HeroPropsSpellsPage.h"
#include "editor/MapEditorText.h"
#include "editor/Town.h"

VA(0x004539f3, 0xca)
THeroPropsSheet::THeroPropsSheet(LPCTSTR pszCaption, CWnd* pParentWnd, const TGameMap& oldMap, TMapObjectRef heroRef)
    : CPropertySheet(pszCaption, pParentWnd),
      _m_map(oldMap),
      _m_heroRef(heroRef),
      _m_pOldHero(_getOldHero()),
      _m_pNewMap(NULL),
      _m_pNewHero(NULL),
      _m_bPagesAdded(false),
      _m_numOKDisablers(0)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
}

VA_COMPGEN(0x00453abd, 0x1c, SCALAR_DELETING_DTOR, THeroPropsSheet)

VA(0x00453ad9, 0xca)
THeroPropsSheet::~THeroPropsSheet()
{
    RemovePage(_m_pSpellsPage.get());
    RemovePage(_m_pArtifactsPage.get());
    RemovePage(_m_pSecSkillsPage.get());
    RemovePage(_m_pPriSkillsPage.get());
    RemovePage(_m_pCreaturesPage.get());
}

VA(0x00453ba3, 0x62)
const THero* THeroPropsSheet::_getOldHero() const
{
    const TGameMap::TLayer& layer = _m_map.getLayer(_m_heroRef.getBSecondLayer());
    const TGameObject* pObject = layer.getPObject(_m_heroRef.getObjectID());
    const THero* pHero = dynamic_cast<const THero*>(pObject);
    if (pHero == NULL)
        pHero = dynamic_cast<const TTown*>(pObject)->getPVisitingHero();
    return pHero;
}

VA(0x00453c05, 0x76)
void THeroPropsSheet::_setNewMap(TGameMap* pNewMap)
{
    _m_pNewMap = pNewMap;
    TGameMap::TLayer& layer = pNewMap->getLayer(_m_heroRef.getBSecondLayer());
    TGameObject* pObject = layer.getPObject(_m_heroRef.getObjectID());
    _m_pNewHero = dynamic_cast<THero*>(pObject);
    if (_m_pNewHero == NULL)
        _m_pNewHero = dynamic_cast<TTown*>(pObject)->getPVisitingHero();
}

VA(0x00453c7b, 0x300)
void THeroPropsSheet::_addPages(bool bRandomHero)
{
    EGameVersion mapVersion = _m_pNewMap->getVersion();
    _m_pCreaturesPage = std::auto_ptr<THeroPropsCreaturesPage>(
        new THeroPropsCreaturesPage(this, _m_pOldHero, _m_pNewHero, mapVersion, bRandomHero));
    if (!_m_pCreaturesPage.get())
        throw TAllocationFailure();
    AddPage(_m_pCreaturesPage.get());
    _m_pPriSkillsPage = std::auto_ptr<THeroPropsPriSkillsPage>(
        new THeroPropsPriSkillsPage(_m_pOldHero, _m_pNewHero, mapVersion));
    if (!_m_pPriSkillsPage.get())
        throw TAllocationFailure();
    AddPage(_m_pPriSkillsPage.get());
    _m_pSecSkillsPage =
        std::auto_ptr<THeroPropsSecSkillsPage>(new THeroPropsSecSkillsPage(_m_pOldHero, _m_pNewHero));
    if (!_m_pSecSkillsPage.get())
        throw TAllocationFailure();
    AddPage(_m_pSecSkillsPage.get());
    _m_pArtifactsPage = std::auto_ptr<THeroPropsArtifactsPage>(
        new THeroPropsArtifactsPage(_m_pOldHero, _m_pNewHero, mapVersion));
    if (!_m_pArtifactsPage.get())
        throw TAllocationFailure();
    AddPage(_m_pArtifactsPage.get());
    _m_pSpellsPage =
        std::auto_ptr<THeroPropsSpellsPage>(new THeroPropsSpellsPage(_m_pOldHero, _m_pNewHero, mapVersion));
    if (!_m_pSpellsPage.get())
        throw TAllocationFailure();
    AddPage(_m_pSpellsPage.get());
    _m_bPagesAdded = true;
}

VA(0x00453f7b, 0x4c)
bool THeroPropsSheet::wasModified() const
{
    return _m_pCreaturesPage->wasModified() || _m_pPriSkillsPage->wasModified() || _m_pSecSkillsPage->wasModified()
           || _m_pArtifactsPage->wasModified() || _m_pSpellsPage->wasModified();
}

VA(0x00453fc7, 0x1c)
void THeroPropsSheet::onEnableOK()
{
    if (--_m_numOKDisablers == 0) {
        CWnd* pOKButton = GetDlgItem(IDOK);
        pOKButton->EnableWindow(TRUE);
    }
}

VA(0x00453fe3, 0x24)
void THeroPropsSheet::onDisableOK()
{
    if (_m_numOKDisablers++ == 0) {
        CWnd* pOKButton = GetDlgItem(IDOK);
        pOKButton->EnableWindow(FALSE);
    }
}

VA(0x00454007, 0x6)
BEGIN_MESSAGE_MAP(THeroPropsSheet, CPropertySheet)
END_MESSAGE_MAP()

VA(0x0045400d, 0x51)
BOOL THeroPropsSheet::OnInitDialog()
{
    BOOL bResult = CPropertySheet::OnInitDialog();
    SetActivePage(_m_pSpellsPage.get());
    SetActivePage(_m_pArtifactsPage.get());
    SetActivePage(_m_pSecSkillsPage.get());
    SetActivePage(_m_pPriSkillsPage.get());
    SetActivePage(_m_pCreaturesPage.get());
    return bResult;
}

VA(0x0045405e, 0x1cc)
TRandomHeroPropsSheet::TRandomHeroPropsSheet(CWnd* pParentWnd, TGameMap* pMap, TMapObjectRef heroRef,
                                             bool bAnyAvailableOwner)
    : THeroPropsSheet(TFormattedString(kObjectPropertiesCaptionFmtStr, akAdvObjectTypeTraits[RANDOM_HERO].m_name),
                      pParentWnd, *pMap, heroRef),
      _m_pMap(pMap),
      _m_heroRef(heroRef)
{
    _m_pNewMap = std::auto_ptr<TGameMap>(new TGameMap(*_m_pMap));
    if (!_m_pNewMap.get())
        throw TAllocationFailure();
    _setNewMap(_m_pNewMap.get());
    _m_pGeneralPage = std::auto_ptr<TRandomHeroPropsGeneralPage>(
        new TRandomHeroPropsGeneralPage(*_m_pMap, *_m_pNewMap, _m_heroRef, bAnyAvailableOwner));
    if (!_m_pGeneralPage.get())
        throw TAllocationFailure();
    AddPage(_m_pGeneralPage.get());
    _addPages(true);
}

VA_COMPGEN(0x0045422a, 0x1c, SCALAR_DELETING_DTOR, TRandomHeroPropsSheet)

VA(0x00454246, 0x69)
TRandomHeroPropsSheet::~TRandomHeroPropsSheet()
{
    RemovePage(_m_pGeneralPage.get());
}

VA(0x004542af, 0x21)
bool TRandomHeroPropsSheet::wasModified() const
{
    return THeroPropsSheet::wasModified() || _m_pGeneralPage->wasModified();
}

VA(0x004542d0, 0x31)
int TRandomHeroPropsSheet::DoModal()
{
    int result = CPropertySheet::DoModal();
    if (result == IDOK && wasModified())
        *_m_pMap = *_m_pNewMap;
    return result;
}

VA(0x00454301, 0x1d)
BOOL TRandomHeroPropsSheet::OnInitDialog()
{
    BOOL bResult = THeroPropsSheet::OnInitDialog();
    SetActivePage(_m_pGeneralPage.get());
    return bResult;
}

VA(0x0045431e, 0x369)
TNonRandomHeroPropsSheet::TNonRandomHeroPropsSheet(CWnd* pParentWnd, TGameMap* pMap, TMapObjectRef heroRef,
                                                   bool bAnyAvailableOwner)
    : THeroPropsSheet(TFormattedString(kObjectPropertiesCaptionFmtStr, akAdvObjectTypeTraits[HERO].m_name),
                      pParentWnd, *pMap, heroRef),
      _m_pMap(pMap),
      _m_heroRef(heroRef),
      _m_pOldHero(NULL),
      _m_pNewHero(NULL)
{
    _m_pNewMap = std::auto_ptr<TGameMap>(new TGameMap(*_m_pMap));
    if (!_m_pNewMap.get())
        throw TAllocationFailure();
    const TGameMap& oldMap = *_m_pMap;
    const TGameMap::TLayer& oldLayer = oldMap.getLayer(_m_heroRef.getBSecondLayer());
    const TGameObject* pOldObject = oldLayer.getPObject(_m_heroRef.getObjectID());
    _m_pOldHero = dynamic_cast<const TNonRandomHero*>(pOldObject);
    if (_m_pOldHero == NULL)
        _m_pOldHero = dynamic_cast<const TTown*>(pOldObject)->getPVisitingHero();
    TGameMap::TLayer& newLayer = _m_pNewMap->getLayer(_m_heroRef.getBSecondLayer());
    TGameObject* pNewObject = newLayer.getPObject(_m_heroRef.getObjectID());
    _m_pNewHero = dynamic_cast<TNonRandomHero*>(pNewObject);
    if (_m_pNewHero == NULL)
        _m_pNewHero = dynamic_cast<TTown*>(pNewObject)->getPVisitingHero();
    _setNewMap(_m_pNewMap.get());
    _m_pGeneralPage = std::auto_ptr<TNonRandomHeroPropsGeneralPage>(
        new TNonRandomHeroPropsGeneralPage(this, *_m_pMap, *_m_pNewMap, _m_heroRef, bAnyAvailableOwner));
    if (!_m_pGeneralPage.get())
        throw TAllocationFailure();
    _m_pBiographyPage = std::auto_ptr<THeroPropsBiographyPage>(
        new THeroPropsBiographyPage(_m_pOldHero, _m_pNewHero, _m_pNewMap->getVersion()));
    if (!_m_pBiographyPage.get())
        throw TAllocationFailure();
    AddPage(_m_pGeneralPage.get());
    AddPage(_m_pBiographyPage.get());
    _addPages(false);
}

VA(0x00454687, 0x5)
void TNonRandomHeroPropsSheet::onEnableOK()
{
    THeroPropsSheet::onEnableOK();
}

VA(0x0045468c, 0x5)
void TNonRandomHeroPropsSheet::onDisableOK()
{
    THeroPropsSheet::onDisableOK();
}

VA_COMPGEN(0x00454691, 0x1c, SCALAR_DELETING_DTOR, TNonRandomHeroPropsSheet)

VA(0x004546ad, 0x8f)
TNonRandomHeroPropsSheet::~TNonRandomHeroPropsSheet()
{
    RemovePage(_m_pBiographyPage.get());
    RemovePage(_m_pGeneralPage.get());
}

VA(0x0045473c, 0x2f)
bool TNonRandomHeroPropsSheet::wasModified() const
{
    return THeroPropsSheet::wasModified() || _m_pGeneralPage->wasModified() || _m_pBiographyPage->wasModified();
}

VA(0x0045476b, 0x31)
int TNonRandomHeroPropsSheet::DoModal()
{
    int result = CPropertySheet::DoModal();
    if (result == IDOK && wasModified())
        *_m_pMap = *_m_pNewMap;
    return result;
}

VA(0x0045479c, 0x31a)
TPrisonPropsSheet::TPrisonPropsSheet(CWnd* pParentWnd, TGameMap* pMap, TMapObjectRef prisonRef)
    : THeroPropsSheet(TFormattedString(kObjectPropertiesCaptionFmtStr, akAdvObjectTypeTraits[PRISON].m_name),
                      pParentWnd, *pMap, prisonRef),
      _m_pMap(pMap),
      _m_prisonRef(prisonRef),
      _m_pOldPrison(NULL),
      _m_pNewPrison(NULL)
{
    _m_pNewMap = std::auto_ptr<TGameMap>(new TGameMap(*_m_pMap));
    if (!_m_pNewMap.get())
        throw TAllocationFailure();
    const TGameMap& oldMap = *_m_pMap;
    const TGameMap::TLayer& oldLayer = oldMap.getLayer(_m_prisonRef.getBSecondLayer());
    _m_pOldPrison = dynamic_cast<const TPrison*>(oldLayer.getPObject(_m_prisonRef.getObjectID()));
    TGameMap::TLayer& newLayer = _m_pNewMap->getLayer(_m_prisonRef.getBSecondLayer());
    _m_pNewPrison = dynamic_cast<TPrison*>(newLayer.getPObject(_m_prisonRef.getObjectID()));
    _setNewMap(_m_pNewMap.get());
    _m_pGeneralPage = std::auto_ptr<TPrisonPropsGeneralPage>(
        new TPrisonPropsGeneralPage(this, *_m_pMap, *_m_pNewMap, _m_prisonRef));
    if (!_m_pGeneralPage.get())
        throw TAllocationFailure();
    _m_pBiographyPage = std::auto_ptr<THeroPropsBiographyPage>(
        new THeroPropsBiographyPage(_m_pOldPrison, _m_pNewPrison, _m_pNewMap->getVersion()));
    if (!_m_pBiographyPage.get())
        throw TAllocationFailure();
    AddPage(_m_pGeneralPage.get());
    AddPage(_m_pBiographyPage.get());
    _addPages(false);
}

VA_COMPGEN(0x00454ab6, 0x1c, SCALAR_DELETING_DTOR, TPrisonPropsSheet)

VA(0x00454ad2, 0x8f)
TPrisonPropsSheet::~TPrisonPropsSheet()
{
    RemovePage(_m_pBiographyPage.get());
    RemovePage(_m_pGeneralPage.get());
}

VA(0x00454b61, 0xa5)
BOOL TNonRandomHeroPropsSheet::OnInitDialog()
{
    BOOL bResult = THeroPropsSheet::OnInitDialog();
    SetActivePage(_m_pBiographyPage.get());
    SetActivePage(_m_pGeneralPage.get());
    const THeroPrototype& prototype = _m_pNewMap->getHeroPrototype(_m_pNewHero->getHeroID());
    _m_pBiographyPage->setDefaultBiography(prototype.getBiography());
    _getPPriSkillsPage()->setDefaultPrimarySkills(prototype.getPrimarySkills());
    _getPSecSkillsPage()->setDefaultSecondarySkills(prototype.getSecondarySkills());
    _getPArtifactsPage()->setDefaultArtifacts(prototype.getArtifacts());
    _getPSpellsPage()->setDefaultSpells(prototype.getSpells());
    return bResult;
}

VA(0x00454c06, 0x63)
void TNonRandomHeroPropsSheet::onSetHeroID(THeroID heroID)
{
    const THeroPrototype& prototype = _m_pNewMap->getHeroPrototype(heroID);
    _m_pBiographyPage->setDefaultBiography(prototype.getBiography());
    _getPPriSkillsPage()->setDefaultPrimarySkills(prototype.getPrimarySkills());
    _getPSecSkillsPage()->setDefaultSecondarySkills(prototype.getSecondarySkills());
    _getPArtifactsPage()->setDefaultArtifacts(prototype.getArtifacts());
    _getPSpellsPage()->setDefaultSpells(prototype.getSpells());
}

void TPrisonPropsSheet::onEnableOK()
{
    THeroPropsSheet::onEnableOK();
}

void TPrisonPropsSheet::onDisableOK()
{
    THeroPropsSheet::onDisableOK();
}

bool TPrisonPropsSheet::wasModified() const
{
    return THeroPropsSheet::wasModified() || _m_pGeneralPage->wasModified() || _m_pBiographyPage->wasModified();
}

int TPrisonPropsSheet::DoModal()
{
    int result = CPropertySheet::DoModal();
    if (result == IDOK && wasModified())
        *_m_pMap = *_m_pNewMap;
    return result;
}

BOOL TPrisonPropsSheet::OnInitDialog()
{
    BOOL bResult = THeroPropsSheet::OnInitDialog();
    SetActivePage(_m_pBiographyPage.get());
    SetActivePage(_m_pGeneralPage.get());
    const THeroPrototype& prototype = _m_pNewMap->getHeroPrototype(_m_pNewPrison->getHeroID());
    _m_pBiographyPage->setDefaultBiography(prototype.getBiography());
    _getPPriSkillsPage()->setDefaultPrimarySkills(prototype.getPrimarySkills());
    _getPSecSkillsPage()->setDefaultSecondarySkills(prototype.getSecondarySkills());
    _getPArtifactsPage()->setDefaultArtifacts(prototype.getArtifacts());
    _getPSpellsPage()->setDefaultSpells(prototype.getSpells());
    return bResult;
}

void TPrisonPropsSheet::onSetHeroID(THeroID heroID)
{
    const THeroPrototype& prototype = _m_pNewMap->getHeroPrototype(heroID);
    _m_pBiographyPage->setDefaultBiography(prototype.getBiography());
    _getPPriSkillsPage()->setDefaultPrimarySkills(prototype.getPrimarySkills());
    _getPSecSkillsPage()->setDefaultSecondarySkills(prototype.getSecondarySkills());
    _getPArtifactsPage()->setDefaultArtifacts(prototype.getArtifacts());
    _getPSpellsPage()->setDefaultSpells(prototype.getSpells());
}
