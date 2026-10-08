// ArtifactPropsSheet.h - the artifact property sheet (Loki
// ArtifactPropsSheet.cpp): the artifact, its general and guardians pages
// and the modal result, then the vtable pointer (the destructor, DoModal,
// OnOK, OnCancel). The member names are not proven.
#ifndef HOMM3_EDITOR_ARTIFACTPROPSSHEET_H
#define HOMM3_EDITOR_ARTIFACTPROPSSHEET_H

#include "editor/stdafx.h"

class TGameArtifact;
class TArtifactPropsGeneralPage;
class TTreasurePropsGuardiansPage;

class TArtifactPropsSheet {
public:
    TArtifactPropsSheet(void* pParent, TGameArtifact* pArtifact);
    virtual ~TArtifactPropsSheet();

    bool wasModified() const;

    virtual int DoModal();
    virtual void OnOK();
    virtual void OnCancel();

private:
    void _deleteAllPages();

    TGameArtifact* _m_pArtifact;
    TArtifactPropsGeneralPage* _m_pGeneralPage;
    TTreasurePropsGuardiansPage* _m_pGuardiansPage;
    int _m_result;
};

// The open sheet, for cppbridge.cpp's artifact_props_dlg signal handlers.
extern TArtifactPropsSheet* artifactPropsSheetModal;

#endif  /* HOMM3_EDITOR_ARTIFACTPROPSSHEET_H */
