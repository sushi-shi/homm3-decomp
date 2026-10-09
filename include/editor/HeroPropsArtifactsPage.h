// HeroPropsArtifactsPage.h - the artifacts page of the hero property sheets
// (HeroPropsArtifactsPage.cpp; Loki h3maped object 15): a customize check
// over a TArtifactsDlg child holding the hero's worn and carried
// artifacts. Layout from the image: the check's state at 0x8c, the
// original and edited heroes, the map's version, the modified flag, the
// default and the edited artifacts and the artifacts dialog (0x160 bytes,
// the sheets' new).
//
// Ported so far: the declarations the hero sheets use.
#ifndef HOMM3_EDITOR_HEROPROPSARTIFACTSPAGE_H
#define HOMM3_EDITOR_HEROPROPSARTIFACTSPAGE_H

#include <memory>

#include "gameversion.h"
#include "editor/Hero.h"
#include "editor/resource.h"

class TArtifactsDlg;

class THeroPropsArtifactsPage : public CPropertyPage {
public:
    THeroPropsArtifactsPage(const THero* pOldHero, THero* pNewHero, EGameVersion mapVersion);
    virtual ~THeroPropsArtifactsPage();

    // The artifacts the hero carries unless the page customizes them.
    void setDefaultArtifacts(const THeroPrototype::TArtifactContainer& artifacts);

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_HERO_PROPS_ARTIFACTS };
    BOOL _m_bCustomize;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnCustomizeCheck();
    DECLARE_MESSAGE_MAP()

private:
    const THero* _m_pOldHero;
    THero* _m_pNewHero;
    EGameVersion _m_mapVersion;
    bool _m_bModified;
    THeroPrototype::TArtifactContainer _m_defaultArtifacts;
    THeroPrototype::TArtifactContainer _m_artifacts;
    std::auto_ptr<TArtifactsDlg> _m_pArtifactsDlg;
};

#endif  /* HOMM3_EDITOR_HEROPROPSARTIFACTSPAGE_H */
