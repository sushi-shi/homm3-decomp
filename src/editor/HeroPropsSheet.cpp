// HeroPropsSheet.cpp - Loki h3maped object 83: the hero property sheet.
// DoModal runs the hero_props_dlg notebook; afterwards every page that
// reports a change is written back to the hero. The sheet keeps a count of
// pages that disable its OK button. The assert lines come from the retail
// immediates.
#include "editor/stdafx.h"

namespace {
#include <gtk/gtk.h>
}

#include "exceptions.h"
#include "editor/HeroPropsSheet.h"

THeroPropsSheet* heroPropsSheetModal = NULL;

THeroPropsSheet::THeroPropsSheet(char* pszCaption, CWnd* pParentWnd, THero* pHero)
    : _m_pHero(pHero),
      _m_pCreaturesPage(NULL),
      _m_pSecSkillsPage(NULL),
      _m_pArtifactsPage(NULL)
{
#line 43
    assert(_m_pHero != NULL);
    try {
        _m_pCreaturesPage = new THeroPropsCreaturesPage(this, *_m_pHero);
        if (_m_pCreaturesPage == NULL)
#line 56
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pSecSkillsPage = new THeroPropsSecSkillsPage(*_m_pHero);
        if (_m_pSecSkillsPage == NULL)
#line 63
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pArtifactsPage = new THeroPropsArtifactsPage(*_m_pHero);
        if (_m_pArtifactsPage == NULL)
#line 68
            throw TAllocationFailure(__FILE__, __LINE__);
    } catch (...) {
        _deleteAllPages();
        throw;
    }
}

THeroPropsSheet::~THeroPropsSheet()
{
    _deleteAllPages();
}

bool THeroPropsSheet::wasModified() const
{
    return _getPGeneralPage()->wasModified() || _m_pCreaturesPage->wasModified() || _m_pSecSkillsPage->wasModified()
           || _m_pArtifactsPage->wasModified();
}

void THeroPropsSheet::_deleteAllPages()
{
    delete _m_pArtifactsPage;
    delete _m_pSecSkillsPage;
    delete _m_pCreaturesPage;
}

void THeroPropsSheet::retrieveGeneralPageData()
{
    const THeroPropsGeneralPage* pGeneralPage = _getPGeneralPage();
#line 104
    assert(pGeneralPage != NULL);
    _m_pHero->setBCustomName(pGeneralPage->getBCustomName());
    _m_pHero->setBCustomPortrait(pGeneralPage->getBCustomPortrait());
    _m_pHero->setName(pGeneralPage->getName());
    _m_pHero->setPortrait(pGeneralPage->getPortrait());
    _m_pHero->setExperience(pGeneralPage->getExperience());
    _m_pHero->setPatrol(pGeneralPage->getPatrol());
}

void THeroPropsSheet::onEnableOK()
{
#line 117
    assert(_m_disableOKCtr > 0);
    _m_disableOKCtr--;
    if (_m_disableOKCtr == 0) {
        GtkWidget* pOKBtn = _widget("hero_props_ok");
#line 122
        assert(pOKBtn != NULL);
        gtk_widget_set_sensitive(pOKBtn, TRUE);
    }
}

void THeroPropsSheet::onDisableOK()
{
    _m_disableOKCtr++;
    if (_m_disableOKCtr == 1) {
        GtkWidget* pOKBtn = _widget("hero_props_ok");
#line 135
        assert(pOKBtn != NULL);
        gtk_widget_set_sensitive(pOKBtn, FALSE);
    }
}

int THeroPropsSheet::DoModal()
{
    _m_disableOKCtr = 0;
    GtkNotebook* pNotebook = GTK_NOTEBOOK(_widget("hero_props_notebook"));
    gtk_notebook_set_page(pNotebook, 0);
    _m_result = 0;
    heroPropsSheetModal = this;
    GtkWidget* pDialog = _widget("hero_props_dlg");
    gtk_widget_show(pDialog);
    while (_m_result == 0)
        gtk_main_iteration();
    gtk_widget_hide(pDialog);
    heroPropsSheetModal = NULL;
    if (_getPGeneralPage()->wasModified())
        retrieveGeneralPageData();
    if (_m_pCreaturesPage->wasModified()) {
        _m_pHero->setBCustomArmy(_m_pCreaturesPage->getBCustomArmy());
        _m_pHero->setArmy(_m_pCreaturesPage->getArmy());
        _m_pHero->setBGroupedFormation(_m_pCreaturesPage->getBGroupedFormation());
    }
    if (_m_pSecSkillsPage->wasModified()) {
        _m_pHero->setBCustomSecondarySkills(_m_pSecSkillsPage->getBCustomSecondarySkills());
        _m_pHero->setSecondarySkills(_m_pSecSkillsPage->getSecondarySkills());
    }
    if (_m_pArtifactsPage->wasModified()) {
        _m_pHero->setBCustomArtifacts(_m_pArtifactsPage->getBCustomArtifacts());
        _m_pHero->setArtifacts(_m_pArtifactsPage->getArtifacts());
    }
    return _m_result;
}

void THeroPropsSheet::OnOK()
{
    _m_pCreaturesPage->OnOK();
    _m_pSecSkillsPage->OnOK();
    _m_pArtifactsPage->OnOK();
    _getPGeneralPage()->OnOK();
    _m_result = 1;
}

void THeroPropsSheet::OnCancel()
{
    _m_result = 2;
}

BOOL THeroPropsSheet::OnInitDialog()
{
    _m_pCreaturesPage->OnInitDialog();
    _m_pSecSkillsPage->OnInitDialog();
    _m_pArtifactsPage->OnInitDialog();
    _getPGeneralPage()->OnInitDialog();
    return true;
}

TRandomHeroPropsSheet::TRandomHeroPropsSheet(CWnd* pParentWnd, TRandomHero* pHero,
                                             const bitset<kNumPlayers>& availableOwners)
    : THeroPropsSheet(NULL, pParentWnd, pHero),
      _m_pHero(pHero),
      _m_pGeneralPage(NULL)
{
#line 251
    assert(_m_pHero != NULL);
    _m_pGeneralPage = new TRandomHeroPropsGeneralPage(this, *pHero, availableOwners);
    if (_m_pGeneralPage == NULL)
#line 256
        throw TAllocationFailure(__FILE__, __LINE__);
}

void TRandomHeroPropsSheet::retrieveGeneralPageData()
{
    _m_pHero->setOwner(_m_pGeneralPage->getOwner());
    THeroPropsSheet::retrieveGeneralPageData();
}

TNonRandomHeroPropsSheet::TNonRandomHeroPropsSheet(CWnd* pParentWnd, TNonRandomHero* pHero,
                                                   const bitset<kNumPlayers>& availableOwners,
                                                   const set<unsigned int>& availableProtoNums)
    : THeroPropsSheet(NULL, pParentWnd, pHero),
      _m_pHero(pHero),
      _m_pGeneralPage(NULL)
{
#line 279
    assert(_m_pHero != NULL);
    _m_pGeneralPage = new TNonRandomHeroPropsGeneralPage(this, *pHero, availableOwners, availableProtoNums);
    if (_m_pGeneralPage == NULL)
#line 284
        throw TAllocationFailure(__FILE__, __LINE__);
}

void TNonRandomHeroPropsSheet::retrieveGeneralPageData()
{
    _m_pHero->setOwner(_m_pGeneralPage->getOwner());
    _m_pHero->setProtoNum(_m_pGeneralPage->getProtoNum());
    THeroPropsSheet::retrieveGeneralPageData();
}

void TNonRandomHeroPropsSheet::onSetNewProtoNum(unsigned int newProtoNum)
{
    _m_pGeneralPage->setProtoNum(newProtoNum);
    getPCreaturesPage()->setIdentity(_m_pHero->getClass(), newProtoNum);
    getPSecSkillsPage()->setIdentity(_m_pHero->getClass(), newProtoNum);
    getPArtifactsPage()->setIdentity(_m_pHero->getClass(), newProtoNum);
}

TPrisonPropsSheet::TPrisonPropsSheet(CWnd* pParentWnd, TPrison* pPrison,
                                     const TArray<set<unsigned int>, kNumHeroClasses>& aAvailableProtoNums)
    : THeroPropsSheet(NULL, pParentWnd, pPrison),
      _m_pPrison(pPrison),
      _m_pGeneralPage(NULL)
{
#line 313
    assert(_m_pPrison != NULL);
    _m_pGeneralPage = new TPrisonPropsGeneralPage(this, *pPrison, aAvailableProtoNums);
    if (_m_pGeneralPage == NULL)
#line 318
        throw TAllocationFailure(__FILE__, __LINE__);
}

void TPrisonPropsSheet::retrieveGeneralPageData()
{
    _m_pPrison->setClass(_m_pGeneralPage->getHeroClass());
    _m_pPrison->setProtoNum(_m_pGeneralPage->getProtoNum());
    THeroPropsSheet::retrieveGeneralPageData();
}

void TPrisonPropsSheet::onSetNewIdentity(THeroClass newHeroClass, unsigned int newProtoNum)
{
    _m_pGeneralPage->setIdentity(newHeroClass, newProtoNum);
    getPCreaturesPage()->setIdentity(newHeroClass, newProtoNum);
    getPSecSkillsPage()->setIdentity(newHeroClass, newProtoNum);
    getPArtifactsPage()->setIdentity(newHeroClass, newProtoNum);
}
