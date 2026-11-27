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
    // loadCampaign calls this retained body: CException(autoDelete), then
    // the TCampaignDocLoadFailure vtable at 0x488078; returns with ret 4.
    VA(0x00413fe0, 0x19)
    explicit TCampaignDocLoadFailure(BOOL bAutoDelete = TRUE) : CException(bAutoDelete) {}
};

// A campaign file of a version the editor does not read (0x487ef8).
class TCampaignDocInvalidFileVersion : public TCampaignDocLoadFailure {
    DECLARE_DYNAMIC(TCampaignDocInvalidFileVersion)

public:
    TCampaignDocInvalidFileVersion(int fileVersion, int currentVersion)
        : _m_fileVersion(fileVersion), _m_currentVersion(currentVersion) {}

    int getFileVersion() const { return _m_fileVersion; }
    int getCurrentVersion() const { return _m_currentVersion; }

private:
    int _m_fileVersion;
    int _m_currentVersion;
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
    // File New on an open document: SaveModified asks for the new
    // campaign's map (the document template sets it).
    void setBNewCampaign(bool bNewCampaign) { _m_bNewCampaign = bNewCampaign; }

    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    virtual void DeleteContents();
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual void ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault);
    virtual BOOL SaveModified();

    std::auto_ptr<TCampaignScenarioMap> loadScenarioMap(const CString& pathName);
    void exportScenarioMap(const TCampaignScenarioMap* pMap, const CString& pathName);

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
    // What File New asked for: the campaign version and type the next new
    // document gets. OnNewDocument resets it by assigning a temporary.
    struct TNewCampaignOptions {
        TNewCampaignOptions(int version, int type) : m_version(version), m_type(type) {}

        int m_version;
        int m_type;
    };
    TNewCampaignOptions _m_newOptions;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_CAMPAIGNDOC_H */
