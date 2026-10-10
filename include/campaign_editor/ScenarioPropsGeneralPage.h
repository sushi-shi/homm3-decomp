// ScenarioPropsGeneralPage.h - the general page of the scenario properties
// sheet (ScenarioPropsGeneralPage.cpp): the region, its color and text,
// the scenario's map file (imported, refreshed, exported or removed through
// the sheet), its difficulty and its prerequisites. Layout from the image:
// the controls from 0x8c, the sheet at 0x2e4, the document at 0x2e8, the
// campaign being edited at 0x2ec, the scenario at 0x2f0, the modified and
// map-changed flags at 0x2f4 and 0x2f5, and the prerequisite check list at
// 0x2f8.
#ifndef HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSGENERALPAGE_H
#define HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSGENERALPAGE_H

#include <memory>

#include "campaign_editor/Campaign.h"
#include "campaign_editor/resource.h"

class TCampaignDoc;

class TScenarioPropsGeneralPage : public CPropertyPage {
public:
    // What the page asks of its sheet when the scenario's map changes.
    class TParentSheet {
    public:
        virtual void setMap(std::auto_ptr<TCampaignScenarioMap> pMap) = 0;
        virtual void removeMap() = 0;
    };

    TScenarioPropsGeneralPage(TParentSheet* pSheet, TCampaignDoc* pDoc, TCampaign* pCampaign, int scenario);
    virtual ~TScenarioPropsGeneralPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_SCENARIO_PROPS_GENERAL };
    CComboBox _m_regionColorCombo;
    CComboBox _m_difficultyCombo;
    CEdit _m_regionTextEdit;
    CButton _m_exportButton;
    CButton _m_refreshButton;
    CButton _m_importButton;
    CButton _m_removeButton;
    CStatic _m_mapFileStatic;
    CStatic _m_scenarioNameStatic;
    CStatic _m_regionNameStatic;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnImportButton();
    afx_msg void OnExportButton();
    afx_msg void OnRemoveButton();
    afx_msg void OnRefreshButton();
    afx_msg void OnCheckChangePrerequisiteList();
    DECLARE_MESSAGE_MAP()

private:
    void _fillPrerequisiteList();
    void _checkPrerequisites();

    TParentSheet* _m_pSheet;
    TCampaignDoc* _m_pDoc;
    TCampaign* _m_pCampaign;
    int _m_scenario;
    bool _m_bModified;
    bool _m_bMapChanged;
    CCheckListBox _m_prerequisiteList;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSGENERALPAGE_H */
