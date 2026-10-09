// CampaignPropsDlg.h - the campaign properties dialog (CampaignPropsDlg.cpp):
// the campaign's name, description, difficulty choice, theme music and
// version. DoModal writes the edits back to the document when OK changed
// anything. Layout from the image: the controls at 0x5c..0x14c, the version
// radio index at 0x188, the document at 0x18c, the edited values from 0x190,
// the Armageddon's Blade radio button at 0x1c0.
#ifndef HOMM3_CAMPAIGN_EDITOR_CAMPAIGNPROPSDLG_H
#define HOMM3_CAMPAIGN_EDITOR_CAMPAIGNPROPSDLG_H

#include <string>

#include "campaign_editor/resource.h"

class TCampaignDoc;

class TCampaignPropsDlg : public CDialog {
public:
    TCampaignPropsDlg(CWnd* pParent, TCampaignDoc* pDoc);

    virtual int DoModal();

    enum { IDD = IDD_CAMPAIGN_PROPS };
    CComboBox _m_musicCombo;
    CButton _m_difficultyChoiceCheck;
    CEdit _m_nameEdit;
    CStatic _m_mapStatic;
    CEdit _m_descriptionEdit;
    int _m_versionRadio;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

private:
    TCampaignDoc* _m_pDoc;
    bool _m_bChanged;
    int _m_version;
    std::string _m_name;
    std::string _m_description;
    bool _m_bDifficultyChoice;
    int _m_music;
    CButton _m_armageddonsBladeRadio;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_CAMPAIGNPROPSDLG_H */
