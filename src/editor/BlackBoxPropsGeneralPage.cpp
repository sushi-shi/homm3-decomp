// BlackBoxPropsGeneralPage.cpp - the general page of the black box property
// sheet (h3maped 0x40bcf8..0x40c1d6; GOG only). A Restoration of Erathia
// map keeps 300 characters of the message.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/BlackBox.h"
#include "editor/BlackBoxPropsGeneralPage.h"
#include "editor/MapEditorText.h"

VA(0x0040bf73, 0x9c)
TBlackBoxPropsGeneralPage::TBlackBoxPropsGeneralPage(TBlackBox* pBlackBox, EGameVersion mapVersion)
    : CPropertyPage(TBlackBoxPropsGeneralPage::IDD),
      _m_pBlackBox(pBlackBox),
      _m_mapVersion(mapVersion),
      _m_bModified(false)
{
    _m_message = _T("");
    m_psp.pszTitle = m_strCaption = kGeneralPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x0040c00f, 0x1c, SCALAR_DELETING_DTOR, TBlackBoxPropsGeneralPage)

VA(0x0040c02b, 0x50)
TBlackBoxPropsGeneralPage::~TBlackBoxPropsGeneralPage()
{
}

VA(0x0040c07b, 0x30)
void TBlackBoxPropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_MESSAGE_EDIT, _m_messageEdit);
    DDX_Text(pDX, IDC_MESSAGE_EDIT, _m_message);
}

VA(0x0040c0ab, 0x6)
BEGIN_MESSAGE_MAP(TBlackBoxPropsGeneralPage, CPropertyPage)
END_MESSAGE_MAP()

VA(0x0040c0b1, 0x85)
BOOL TBlackBoxPropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_MESSAGE_STATIC)->SetWindowText(SBlackBoxPropsGeneralPageText::kMessageStaticStr);
    _m_bModified = false;
    _m_message = _m_pBlackBox->getMessage().c_str();
    _m_message.Replace("\n", "\r\n");
    CPropertyPage::OnInitDialog();
    if (_m_mapVersion == GAME_VERSION_ROE)
        _m_messageEdit.LimitText(TTreasure::s_kMaxMessageLen);
    return TRUE;
}

VA(0x0040c136, 0xa0)
void TBlackBoxPropsGeneralPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_message.Replace("\r\n", "\n");
    _m_bModified = _m_bModified || std::string(_m_message) != _m_pBlackBox->getMessage();
}
