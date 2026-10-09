// CampaignPropsDlg.cpp - the campaign properties dialog (h3ccmped
// 0x41ba40..0x41c3ec).
#include "campaign_editor/stdafx.h"

#include "va.h"
#include "campaignmap.h"
#include "campaignmusic.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/CampaignDoc.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/CampaignPropsDlg.h"

VA(0x0041ba40, 0x12a)
TCampaignPropsDlg::TCampaignPropsDlg(CWnd* pParent, TCampaignDoc* pDoc)
    : CDialog(TCampaignPropsDlg::IDD, pParent), _m_pDoc(pDoc), _m_bChanged(false)
{
    _m_versionRadio = -1;
}

VA(0x0041bcb0, 0x88)
void TCampaignPropsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_MUSIC_COMBO, _m_musicCombo);
    DDX_Control(pDX, IDC_DIFFICULTY_CHOICE_CHECK, _m_difficultyChoiceCheck);
    DDX_Control(pDX, IDC_CAMPAIGN_NAME_EDIT, _m_nameEdit);
    DDX_Control(pDX, IDC_CAMPAIGN_MAP_STATIC, _m_mapStatic);
    DDX_Control(pDX, IDC_CAMPAIGN_DESCRIPTION_EDIT, _m_descriptionEdit);
    DDX_Radio(pDX, IDC_ARMAGEDDONS_BLADE_RADIO, _m_versionRadio);
    DDX_Control(pDX, IDC_ARMAGEDDONS_BLADE_RADIO, _m_armageddonsBladeRadio);
}

VA(0x0041bd40, 0x6)
BEGIN_MESSAGE_MAP(TCampaignPropsDlg, CDialog)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x0041bd50, 0x28)
int TCampaignPropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(SCampaignPropsDlgText::kCaptionStr);
    return 0;
}

VA(0x0041bd80, 0xa0)
int TCampaignPropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (result == IDOK && _m_bChanged) {
        if (_m_version > _m_pDoc->getVersion())
            _m_pDoc->setVersion(_m_version);
        TCampaign* pCampaign = _m_pDoc->getCampaign();
        pCampaign->setName(_m_name);
        pCampaign->setDescription(_m_description);
        pCampaign->setBDifficultyChoice(_m_bDifficultyChoice);
        pCampaign->setMusic(_m_music);
        _m_pDoc->SetModifiedFlag(TRUE);
        _m_pDoc->UpdateAllViews(NULL);
    }
    return result;
}

VA(0x0041be20, 0x2e5)
BOOL TCampaignPropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_CAMPAIGN_MAP_LABEL_STATIC)->SetWindowText(SCampaignPropsDlgText::kMapStaticStr);
    GetDlgItem(IDC_CAMPAIGN_NAME_STATIC)->SetWindowText(SCampaignPropsDlgText::kNameStaticStr);
    GetDlgItem(IDC_CAMPAIGN_DESCRIPTION_STATIC)->SetWindowText(SCampaignPropsDlgText::kDescriptionStaticStr);
    GetDlgItem(IDC_MUSIC_STATIC)->SetWindowText(SCampaignPropsDlgText::kMusicStaticStr);
    GetDlgItem(IDC_DIFFICULTY_CHOICE_CHECK)->SetWindowText(SCampaignPropsDlgText::kDifficultyChoiceCheckStr);
    GetDlgItem(IDC_VERSION_GROUP)->SetWindowText(SCampaignPropsDlgText::kVersionGroupStr);
    GetDlgItem(IDC_ARMAGEDDONS_BLADE_RADIO)->SetWindowText(SCampaignPropsDlgText::kArmageddonsBladeRadioStr);
    GetDlgItem(IDC_SHADOW_OF_DEATH_RADIO)->SetWindowText(SCampaignPropsDlgText::kShadowOfDeathRadioStr);
    _m_bChanged = false;
    _m_version = _m_pDoc->getVersion();
    _m_versionRadio = _m_version - 1;
    const TCampaign* pCampaign = _m_pDoc->getCampaign();
    _m_name = pCampaign->getName();
    _m_description = pCampaign->getDescription();
    _m_bDifficultyChoice = pCampaign->getBDifficultyChoice();
    _m_music = pCampaign->getMusic();
    CDialog::OnInitDialog();
    if (_m_version > eCampaignVersionArmageddonsBlade)
        _m_armageddonsBladeRadio.EnableWindow(FALSE);
    _m_mapStatic.SetWindowText(pCampaign->getMapTraits().m_name);
    _m_nameEdit.LimitText(TCampaign::s_kMaxNameLen);
    _m_nameEdit.SetWindowText(_m_name.c_str());
    _m_descriptionEdit.LimitText(TCampaign::s_kMaxDescriptionLen);
    _m_descriptionEdit.SetWindowText(_m_description.c_str());
    unsigned int numThemes = _m_pDoc->getVersion() > eCampaignVersionArmageddonsBlade ? CAMPAIGN_MUSIC_CUE_COUNT
                                                                                       : CAMPAIGN_MUSIC_CUE_COUNT - 1;
    for (unsigned int theme = 0; theme < numThemes; theme++)
        _m_musicCombo.SetItemData(_m_musicCombo.AddString(g_campaignMusicTraits[theme].m_track), theme);
    int item;
    for (item = 0; _m_musicCombo.GetItemData(item) != _m_music; item++)
        ;
    _m_musicCombo.SetCurSel(item);
    _m_difficultyChoiceCheck.SetCheck(_m_bDifficultyChoice);
    return TRUE;
}

VA(0x0041c110, 0x254)
void TCampaignPropsDlg::OnOK()
{
    CDialog::OnOK();
    _m_version = _m_versionRadio + 1;
    CString text;
    _m_nameEdit.GetWindowText(text);
    _m_name = text;
    _m_descriptionEdit.GetWindowText(text);
    _m_description = text;
    _m_bDifficultyChoice = _m_difficultyChoiceCheck.GetCheck() != 0;
    _m_music = _m_musicCombo.GetItemData(_m_musicCombo.GetCurSel());
    const TCampaign* pCampaign = _m_pDoc->getCampaign();
    _m_bChanged = _m_bChanged || _m_version > _m_pDoc->getVersion() || _m_name != pCampaign->getName()
                  || _m_description != pCampaign->getDescription()
                  || _m_bDifficultyChoice != pCampaign->getBDifficultyChoice() || _m_music != pCampaign->getMusic();
}
