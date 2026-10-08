// HeroPropsSheet.h - the hero property sheet (Loki HeroPropsSheet.cpp): the
// hero_props_dlg glade notebook with the general, creatures, secondary
// skills and artifacts pages; the random hero, specific hero and prison
// sheets supply the general page. The sheet implements both pages' parent
// interfaces: the creatures page's as its primary base and the general
// page's as a virtual base, which the specific hero and prison sheets'
// own interfaces share. Layout from the image: the vtable and virtual base
// pointers, the modal result, the hero (_m_pHero, assert), the three
// common pages and the OK-disable count (_m_disableOKCtr, assert). The
// page member names are not proven.
#ifndef HOMM3_EDITOR_HEROPROPSSHEET_H
#define HOMM3_EDITOR_HEROPROPSSHEET_H

#include "editor/stdafx.h"

#include <bitset>
#include <set>

#include "editor/HeroPropsArtifactsPage.h"
#include "editor/HeroPropsGeneralPage.h"
#include "editor/HeroPropsCreaturesPage.h"
#include "editor/HeroPropsSecSkillsPage.h"

class THeroPropsSheet : protected virtual THeroPropsGeneralPageParentSheet, THeroPropsCreaturesPageParentSheet {
public:
    THeroPropsSheet(char* pszCaption, CWnd* pParentWnd, THero* pHero);

    virtual void onEnableOK();
    virtual void onDisableOK();
    virtual ~THeroPropsSheet();
    virtual void retrieveGeneralPageData();
    virtual int DoModal();
    virtual void OnOK();
    virtual void OnCancel();

public:
    // cppbridge.cpp's hero general page handlers call it.
    virtual THeroPropsGeneralPage* _getPGeneralPage() = 0;
    virtual const THeroPropsGeneralPage* _getPGeneralPage() const = 0;

public:
    virtual BOOL OnInitDialog();

    bool wasModified() const;

    THeroPropsCreaturesPage* getPCreaturesPage() { return _m_pCreaturesPage; }
    THeroPropsSecSkillsPage* getPSecSkillsPage() { return _m_pSecSkillsPage; }
    THeroPropsArtifactsPage* getPArtifactsPage() { return _m_pArtifactsPage; }

private:
    void _deleteAllPages();

    volatile int _m_result;
    THero* _m_pHero;

public:
    // cppbridge.cpp's creatures page handlers read it directly (+0x10), where
    // they call getPSecSkillsPage and getPArtifactsPage.
    THeroPropsCreaturesPage* _m_pCreaturesPage;

private:
    THeroPropsSecSkillsPage* _m_pSecSkillsPage;
    THeroPropsArtifactsPage* _m_pArtifactsPage;
    unsigned int _m_disableOKCtr;
};

class TRandomHeroPropsSheet : public THeroPropsSheet {
public:
    TRandomHeroPropsSheet(CWnd* pParentWnd, TRandomHero* pHero, const bitset<kNumPlayers>& availableOwners);
    virtual ~TRandomHeroPropsSheet() { delete _m_pGeneralPage; }

    virtual void retrieveGeneralPageData();

protected:
    virtual THeroPropsGeneralPage* _getPGeneralPage() { return _m_pGeneralPage; }
    virtual const THeroPropsGeneralPage* _getPGeneralPage() const { return _m_pGeneralPage; }

private:
    TRandomHero* _m_pHero;
    TRandomHeroPropsGeneralPage* _m_pGeneralPage;
};

class TNonRandomHeroPropsSheet : public THeroPropsSheet, TNonRandomHeroPropsGeneralPageParentSheet {
public:
    TNonRandomHeroPropsSheet(CWnd* pParentWnd, TNonRandomHero* pHero, const bitset<kNumPlayers>& availableOwners,
                             const set<unsigned int>& availableProtoNums);
    virtual ~TNonRandomHeroPropsSheet() { delete _m_pGeneralPage; }

    virtual void retrieveGeneralPageData();
    virtual void onSetNewProtoNum(unsigned int newProtoNum);

protected:
    virtual THeroPropsGeneralPage* _getPGeneralPage() { return _m_pGeneralPage; }
    virtual const THeroPropsGeneralPage* _getPGeneralPage() const { return _m_pGeneralPage; }

private:
    TNonRandomHero* _m_pHero;
    TNonRandomHeroPropsGeneralPage* _m_pGeneralPage;
};

class TPrisonPropsSheet : public THeroPropsSheet, TPrisonPropsGeneralPageParentSheet {
public:
    TPrisonPropsSheet(CWnd* pParentWnd, TPrison* pPrison,
                      const TArray<set<unsigned int>, kNumHeroClasses>& aAvailableProtoNums);
    virtual ~TPrisonPropsSheet() { delete _m_pGeneralPage; }

    virtual void retrieveGeneralPageData();
    virtual void onSetNewIdentity(THeroClass newHeroClass, unsigned int newProtoNum);

protected:
    virtual THeroPropsGeneralPage* _getPGeneralPage() { return _m_pGeneralPage; }
    virtual const THeroPropsGeneralPage* _getPGeneralPage() const { return _m_pGeneralPage; }

private:
    TPrison* _m_pPrison;
    TPrisonPropsGeneralPage* _m_pGeneralPage;
};

// The open sheet, for cppbridge.cpp's hero_props_dlg signal handlers.
extern THeroPropsSheet* heroPropsSheetModal;

#endif  /* HOMM3_EDITOR_HEROPROPSSHEET_H */
