// NewCampaignDlg.cpp - the new campaign dialog (h3ccmped 0x429470..0x42988c).
#include "campaign_editor/stdafx.h"

#include "va.h"
#include "campaignmap.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/NewCampaignDlg.h"

VA(0x00429470, 0x7e)
TNewCampaignDlg::TNewCampaignDlg(CWnd* pParent, int version, int type)
    : CDialog(TNewCampaignDlg::IDD, pParent), _m_version(version), _m_type(type)
{
    _m_versionRadio = -1;
}

VA(0x00429510, 0x2e)
void TNewCampaignDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_MAP_LIST, _m_mapList);
    DDX_Radio(pDX, IDC_ARMAGEDDONS_BLADE_RADIO, _m_versionRadio);
}

VA(0x00429540, 0x6)
BEGIN_MESSAGE_MAP(TNewCampaignDlg, CDialog)
    ON_LBN_DBLCLK(IDC_MAP_LIST, OnDblclkMapList)
    ON_WM_CREATE()
    ON_BN_CLICKED(IDC_ARMAGEDDONS_BLADE_RADIO, OnVersionRadio)
    ON_BN_CLICKED(IDC_SHADOW_OF_DEATH_RADIO, OnVersionRadio)
END_MESSAGE_MAP()

VA(0x00429550, 0x28)
int TNewCampaignDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(SNewCampaignDlgText::kCaptionStr);
    return 0;
}

VA(0x00429580, 0x184)
BOOL TNewCampaignDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_NEW_CAMPAIGN_MAP_STATIC)->SetWindowText(SNewCampaignDlgText::kMapStaticStr);
    GetDlgItem(IDC_VERSION_GROUP)->SetWindowText(SNewCampaignDlgText::kVersionGroupStr);
    GetDlgItem(IDC_ARMAGEDDONS_BLADE_RADIO)->SetWindowText(SNewCampaignDlgText::kArmageddonsBladeRadioStr);
    GetDlgItem(IDC_SHADOW_OF_DEATH_RADIO)->SetWindowText(SNewCampaignDlgText::kShadowOfDeathRadioStr);
    _m_versionRadio = _m_version - 1;
    CDialog::OnInitDialog();
    int numCampaigns = _m_version == eCampaignVersionShadowOfDeath ? kNumShadowOfDeathCampaigns
                                                                    : kNumArmageddonsBladeCampaigns;
    for (int campaign = 0; campaign < numCampaigns; campaign++) {
        if (g_campaignMapTraits[campaign].m_numRegions > 0)
            _m_mapList.SetItemData(_m_mapList.AddString(g_campaignMapTraits[campaign].m_name), campaign);
    }
    int item;
    for (item = 0; _m_mapList.GetItemData(item) != _m_type; item++)
        ;
    _m_mapList.SetCurSel(item);
    return TRUE;
}

VA(0x00429710, 0x42)
void TNewCampaignDlg::OnOK()
{
    CDialog::OnOK();
    _m_version = _m_versionRadio + 1;
    _m_type = _m_mapList.GetItemData(_m_mapList.GetCurSel());
}

VA(0x00429760, 0x8)
void TNewCampaignDlg::OnDblclkMapList()
{
    OnOK();
}

VA(0x00429770, 0xf9)
void TNewCampaignDlg::OnVersionRadio()
{
    UpdateData(TRUE);
    if (_m_versionRadio + 1 == eCampaignVersionArmageddonsBlade) {
        if (_m_version == eCampaignVersionShadowOfDeath) {
            for (int item = _m_mapList.GetCount(); item > 0;) {
                --item;
                int campaign = _m_mapList.GetItemData(item);
                if (campaign >= kNumArmageddonsBladeCampaigns)
                    _m_mapList.DeleteString(item);
            }
            if (_m_mapList.GetCurSel() == LB_ERR)
                _m_mapList.SetCurSel(0);
            _m_version = eCampaignVersionArmageddonsBlade;
        }
    } else if (_m_version == eCampaignVersionArmageddonsBlade) {
        for (int campaign = kNumArmageddonsBladeCampaigns; campaign < kNumShadowOfDeathCampaigns; campaign++) {
            if (g_campaignMapTraits[campaign].m_numRegions > 0)
                _m_mapList.SetItemData(_m_mapList.AddString(g_campaignMapTraits[campaign].m_name), campaign);
        }
        _m_version = eCampaignVersionShadowOfDeath;
    }
}
