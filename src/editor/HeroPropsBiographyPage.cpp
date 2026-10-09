// HeroPropsBiographyPage.cpp - the biography page of the hero property
// sheets (h3maped 0x44ea53..0x44eeb5; GOG only).
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Hero.h"
#include "editor/HeroPropsBiographyPage.h"
#include "editor/MapEditorText.h"

VA(0x0044ea6f, 0xbb)
THeroPropsBiographyPage::THeroPropsBiographyPage(const THero* pOldHero, THero* pNewHero, EGameVersion mapVersion)
    : CPropertyPage(THeroPropsBiographyPage::IDD),
      _m_pOldHero(pOldHero),
      _m_pNewHero(pNewHero),
      _m_mapVersion(mapVersion),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = SHeroPropsBiographyPageText::kCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x0044eb2a, 0x1c, SCALAR_DELETING_DTOR, THeroPropsBiographyPage)

VA(0x0044eb46, 0x6e)
THeroPropsBiographyPage::~THeroPropsBiographyPage()
{
}

VA(0x0044ebb4, 0x4e)
void THeroPropsBiographyPage::setDefaultBiography(const std::string& biography)
{
    _m_defaultBiography = biography.c_str();
    _m_defaultBiography.Replace("\n", "\r\n");
    if (!_m_bCustomBiography)
        _m_biographyEdit.SetWindowText(_m_defaultBiography);
}

VA(0x0044ec02, 0x31)
void THeroPropsBiographyPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_CUSTOM_BIOGRAPHY_CHECK, _m_customizeCheck);
    DDX_Control(pDX, IDC_BIOGRAPHY_EDIT, _m_biographyEdit);
}

VA(0x0044ec33, 0x6)
BEGIN_MESSAGE_MAP(THeroPropsBiographyPage, CPropertyPage)
    ON_BN_CLICKED(IDC_CUSTOM_BIOGRAPHY_CHECK, OnCustomizeCheck)
END_MESSAGE_MAP()

VA(0x0044ec39, 0xdb)
BOOL THeroPropsBiographyPage::OnInitDialog()
{
    GetDlgItem(IDC_CUSTOM_BIOGRAPHY_CHECK)->SetWindowText(SHeroPropsBiographyPageText::kCustomizeCheckStr);
    _m_bModified = false;
    _m_customBiography = _m_pNewHero->getBiography().c_str();
    _m_customBiography.Replace("\n", "\r\n");
    _m_bCustomBiography = _m_pNewHero->getBCustomBiography();
    CPropertyPage::OnInitDialog();
    _m_customizeCheck.SetCheck(_m_bCustomBiography);
    if (_m_mapVersion < GAME_VERSION_AB)
        _m_customizeCheck.EnableWindow(FALSE);
    _m_biographyEdit.SetWindowText(_m_bCustomBiography ? _m_customBiography : _m_defaultBiography);
    if (!_m_bCustomBiography)
        _m_biographyEdit.EnableWindow(FALSE);
    return TRUE;
}

VA(0x0044ed14, 0x11b)
void THeroPropsBiographyPage::OnOK()
{
    CPropertyPage::OnOK();
    if (_m_bCustomBiography) {
        _m_pNewHero->setBCustomBiography(true);
        _m_biographyEdit.GetWindowText(_m_customBiography);
        _m_customBiography.Replace("\r\n", "\n");
        _m_pNewHero->setBiography(std::string(_m_customBiography));
    } else {
        _m_pNewHero->setBCustomBiography(false);
        _m_pNewHero->setBiography(std::string());
    }
    _m_bModified = _m_bModified || _m_pOldHero->getBCustomBiography() != _m_pNewHero->getBCustomBiography()
                   || _m_pOldHero->getBiography() != _m_pNewHero->getBiography();
}

VA(0x0044ee2f, 0x86)
void THeroPropsBiographyPage::OnCustomizeCheck()
{
    if (_m_customizeCheck.GetCheck()) {
        if (!_m_bCustomBiography) {
            _m_biographyEdit.SetWindowText(_m_customBiography);
            _m_biographyEdit.EnableWindow(TRUE);
            _m_bCustomBiography = true;
        }
    } else if (_m_bCustomBiography) {
        _m_biographyEdit.GetWindowText(_m_customBiography);
        _m_biographyEdit.EnableWindow(FALSE);
        _m_biographyEdit.SetWindowText(_m_defaultBiography);
        _m_bCustomBiography = false;
    }
}
