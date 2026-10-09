// ArtifactPropsGeneralPage.cpp - the general page of the artifact property
// sheet (h3maped 0x403e77..0x4041ba; Loki h3maped object 101). A
// Restoration of Erathia map keeps 300 characters of the message.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/ArtifactPropsGeneralPage.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"

VA(0x00403e93, 0xb4)
TArtifactPropsGeneralPage::TArtifactPropsGeneralPage(TGameArtifact* pArtifact, EGameVersion mapVersion)
    : CPropertyPage(TArtifactPropsGeneralPage::IDD),
      _m_pArtifact(pArtifact),
      _m_mapVersion(mapVersion),
      _m_bModified(false)
{
    _m_typeName = _T("");
    _m_message = _T("");
    m_psp.pszTitle = m_strCaption = kGeneralPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00403f47, 0x1c, SCALAR_DELETING_DTOR, TArtifactPropsGeneralPage)

VA(0x00403f63, 0x5f)
TArtifactPropsGeneralPage::~TArtifactPropsGeneralPage()
{
}

VA(0x00403fc2, 0x42)
void TArtifactPropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_MESSAGE_EDIT, _m_messageEdit);
    DDX_Text(pDX, IDC_ARTIFACT_TYPE_STATIC, _m_typeName);
    DDX_Text(pDX, IDC_MESSAGE_EDIT, _m_message);
}

VA(0x00404004, 0x6)
BEGIN_MESSAGE_MAP(TArtifactPropsGeneralPage, CPropertyPage)
END_MESSAGE_MAP()

VA(0x0040400a, 0x110)
BOOL TArtifactPropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_TYPE_STATIC)->SetWindowText(SArtifactPropsGeneralPageText::kTypeStaticStr);
    GetDlgItem(IDC_MESSAGE_STATIC)->SetWindowText(SArtifactPropsGeneralPageText::kMessageStaticStr);
    GetDlgItem(IDC_NOTE_STATIC)->SetWindowText(SArtifactPropsGeneralPageText::kNoteStaticStr);
    _m_bModified = false;
    _m_typeName = _m_pArtifact->getTypeName().c_str();
    _m_message = _m_pArtifact->getMessage().c_str();
    _m_message.Replace("\n", "\r\n");
    CPropertyPage::OnInitDialog();
    if (_m_mapVersion == GAME_VERSION_ROE)
        _m_messageEdit.LimitText(TTreasure::s_kMaxMessageLen);
    return TRUE;
}

VA(0x0040411a, 0xa0)
void TArtifactPropsGeneralPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_message.Replace("\r\n", "\n");
    _m_bModified = _m_bModified || std::string(_m_message) != _m_pArtifact->getMessage();
}
