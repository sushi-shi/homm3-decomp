// ArtifactPropsSheet.h - the artifact property sheet (ArtifactPropsSheet.cpp;
// Loki h3maped object 100): the general and guardians pages over the
// artifact itself, which DoModal updates from the pages that changed.
// Layout from the image: the artifact at 0x88 and the two pages' auto_ptrs
// (0xa0 bytes).
#ifndef HOMM3_EDITOR_ARTIFACTPROPSSHEET_H
#define HOMM3_EDITOR_ARTIFACTPROPSSHEET_H

#include <memory>

#include "gameversion.h"
#include "editor/ArtifactPropsGeneralPage.h"
#include "editor/TreasurePropsGuardiansPage.h"

class TGameArtifact;

class TArtifactPropsSheet : public CPropertySheet {
public:
    TArtifactPropsSheet(CWnd* pParentWnd, TGameArtifact* pArtifact, EGameVersion mapVersion);
    virtual ~TArtifactPropsSheet();

    virtual int DoModal();
    bool wasModified() const;

protected:
    DECLARE_MESSAGE_MAP()

private:
    TGameArtifact* _m_pArtifact;
    std::auto_ptr<TArtifactPropsGeneralPage> _m_pGeneralPage;
    std::auto_ptr<TTreasurePropsGuardiansPage> _m_pGuardiansPage;
};

#endif  /* HOMM3_EDITOR_ARTIFACTPROPSSHEET_H */
