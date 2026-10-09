// NewCampaignDlg.h - the new campaign dialog (NewCampaignDlg.cpp): the
// campaign's version and the campaign map it is built on. The version radios
// choose which maps the list offers; the list keeps each map's index in the
// campaign map table as its item data. Layout from the image: the list at
// 0x5c, the radio index at 0x98, the version and map at 0x9c and 0xa0.
#ifndef HOMM3_CAMPAIGN_EDITOR_NEWCAMPAIGNDLG_H
#define HOMM3_CAMPAIGN_EDITOR_NEWCAMPAIGNDLG_H

#include "campaign_editor/resource.h"

class TNewCampaignDlg : public CDialog {
public:
    TNewCampaignDlg(CWnd* pParent, int version, int type);

    int getVersion() const { return _m_version; }
    int getType() const { return _m_type; }

    enum { IDD = IDD_NEW_CAMPAIGN };
    CListBox _m_mapList;
    int _m_versionRadio;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnDblclkMapList();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnVersionRadio();
    DECLARE_MESSAGE_MAP()

private:
    int _m_version;
    int _m_type;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_NEWCAMPAIGNDLG_H */
