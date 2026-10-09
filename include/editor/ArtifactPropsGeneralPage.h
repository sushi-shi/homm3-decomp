// ArtifactPropsGeneralPage.h - the general page of the artifact property
// sheet (ArtifactPropsGeneralPage.cpp; Loki h3maped object 101): the
// artifact's type name and its pickup message. Layout from the image: the
// message edit at 0x8c, the DDX type name and message at 0xc8, then the
// artifact, the map's version and the modified flag (0xdc bytes, the
// sheet's new).
#ifndef HOMM3_EDITOR_ARTIFACTPROPSGENERALPAGE_H
#define HOMM3_EDITOR_ARTIFACTPROPSGENERALPAGE_H

#include <string>

#include "gameversion.h"
#include "editor/resource.h"

class TGameArtifact;

class TArtifactPropsGeneralPage : public CPropertyPage {
public:
    TArtifactPropsGeneralPage(TGameArtifact* pArtifact, EGameVersion mapVersion);
    virtual ~TArtifactPropsGeneralPage();

    bool wasModified() const { return _m_bModified; }
    std::string getMessage() const { return std::string(_m_message); }

    enum { IDD = IDD_ARTIFACT_PROPS_GENERAL };
    CEdit _m_messageEdit;
    CString _m_typeName;
    CString _m_message;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()

private:
    TGameArtifact* _m_pArtifact;
    EGameVersion _m_mapVersion;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_ARTIFACTPROPSGENERALPAGE_H */
