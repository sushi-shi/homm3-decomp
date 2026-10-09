// CampaignDoc.h - the campaign document (CampaignDoc.cpp): the campaign
// being edited, the campaign file version it was read with or will be
// written with, and what File New asked for. RTTI and CRuntimeClass name the
// document and its load failures.
#ifndef HOMM3_CAMPAIGN_EDITOR_CAMPAIGNDOC_H
#define HOMM3_CAMPAIGN_EDITOR_CAMPAIGNDOC_H

#include <memory>

#include "campaign_editor/Campaign.h"

// A campaign file that is not one (CRuntimeClass 0x487ee0).
class TCampaignDocLoadFailure : public CException {
    DECLARE_DYNAMIC(TCampaignDocLoadFailure)

public:
    explicit TCampaignDocLoadFailure(BOOL bAutoDelete = TRUE) : CException(bAutoDelete) {}
};

// A campaign file of a version the editor does not read (0x487ef8).
class TCampaignDocInvalidFileVersion : public TCampaignDocLoadFailure {
    DECLARE_DYNAMIC(TCampaignDocInvalidFileVersion)

public:
    TCampaignDocInvalidFileVersion(int fileVersion, int currentVersion)
        : m_fileVersion(fileVersion), m_currentVersion(currentVersion) {}

    int m_fileVersion;
    int m_currentVersion;
};

class TCampaignDoc : public CDocument {
protected:
    TCampaignDoc();
    DECLARE_DYNCREATE(TCampaignDoc)

public:
    virtual ~TCampaignDoc();

    TCampaign* getCampaign() const { return _m_pCampaign.get(); }
    int getVersion() const { return _m_version; }
    void setVersion(int newVersion);

    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    virtual void DeleteContents();
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual void ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault);
    virtual BOOL SaveModified();

protected:
    afx_msg void OnRefreshScenarioMaps();
    afx_msg void OnExportScenarioMaps();
    afx_msg void OnExportText();
    afx_msg void OnImportText();
    DECLARE_MESSAGE_MAP()

private:
    std::auto_ptr<TCampaign> _m_pCampaign;
    int _m_version;
    bool _m_bNewCampaign;
    int _m_newVersion;
    int _m_newType;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_CAMPAIGNDOC_H */
