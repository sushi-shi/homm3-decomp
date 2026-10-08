// ArtifactPropsGeneralPage.h - the general page of the artifact property
// sheet (Loki object 101, whose file name is inferred): the artifact's type
// name and its pickup message, over the artifact_props_general glade
// widgets. Layout from the image: the message text and type label widgets,
// the DDX strings, the artifact, the modified flag, then the vtable pointer
// (OnOK, OnInitDialog). The member names are not proven.
#ifndef HOMM3_EDITOR_ARTIFACTPROPSGENERALPAGE_H
#define HOMM3_EDITOR_ARTIFACTPROPSGENERALPAGE_H

#include "editor/stdafx.h"

#include <string>

#include "editor/ObjectSpecializations.h"

class TArtifactPropsGeneralPage {
public:
    TArtifactPropsGeneralPage(const TGameArtifact& artifact);
    ~TArtifactPropsGeneralPage();

    virtual BOOL OnInitDialog();
    virtual void OnOK();

    string getMessage() const { return _m_message; }
    bool wasModified() const { return _m_bModified; }

private:
    GtkText* _m_messageText;
    GtkLabel* _m_typeLabel;
    string _m_typeName;
    string _m_message;
    const TGameArtifact& _m_artifact;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_ARTIFACTPROPSGENERALPAGE_H */
