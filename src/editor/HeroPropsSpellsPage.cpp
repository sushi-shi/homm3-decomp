// HeroPropsSpellsPage.cpp - the spells page of the hero property sheets
// (h3maped 0x454c69..0x45527a; GOG only). The spells dialog takes the
// frame control's place.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Hero.h"
#include "editor/HeroPropsSpellsPage.h"
#include "editor/MapEditorText.h"

VA(0x00454e4d, 0xbc)
THeroPropsSpellsPage::THeroPropsSpellsPage(const THero* pOldHero, THero* pNewHero, EGameVersion mapVersion)
    : CPropertyPage(THeroPropsSpellsPage::IDD),
      _m_pOldHero(pOldHero),
      _m_pNewHero(pNewHero),
      _m_mapVersion(mapVersion),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = SHeroPropsSpellsPageText::kCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00454f09, 0x1c, SCALAR_DELETING_DTOR, THeroPropsSpellsPage)

VA(0x00454f25, 0x50)
THeroPropsSpellsPage::~THeroPropsSpellsPage()
{
}

VA(0x00454f75, 0x2b)
void THeroPropsSpellsPage::setDefaultSpells(const std::bitset<kNumSpells>& spells)
{
    _m_defaultSpells = spells;
    if (!_m_bCustomSpells)
        _m_pSpellsDlg->setSpells(_m_defaultSpells);
}

VA(0x00454fa0, 0x18)
void THeroPropsSpellsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_CUSTOM_SPELLS_CHECK, _m_customizeCheck);
}

VA(0x00454fb8, 0x6)
BEGIN_MESSAGE_MAP(THeroPropsSpellsPage, CPropertyPage)
    ON_BN_CLICKED(IDC_CUSTOM_SPELLS_CHECK, OnCustomizeCheck)
END_MESSAGE_MAP()

VA(0x00454fbe, 0x19a)
BOOL THeroPropsSpellsPage::OnInitDialog()
{
    GetDlgItem(IDC_CUSTOM_SPELLS_CHECK)->SetWindowText(SHeroPropsSpellsPageText::kCustomizeCheckStr);
    _m_bModified = false;
    _m_bCustomSpells = _m_pNewHero->getBCustomSpells();
    _m_customSpells = _m_pNewHero->getSpells();
    CPropertyPage::OnInitDialog();
    _m_pSpellsDlg = std::auto_ptr<TSpellsDlg>(new TSpellsDlg(this, _m_mapVersion));
    if (!_m_pSpellsDlg.get())
        throw TAllocationFailure();
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pSpellsDlg->SetWindowPos(pFrame, rect.left, rect.top, rect.Width(), rect.Height(), 0);
    pFrame->DestroyWindow();
    _m_customizeCheck.SetCheck(_m_bCustomSpells);
    if (_m_mapVersion < GAME_VERSION_AB)
        _m_customizeCheck.EnableWindow(FALSE);
    _m_pSpellsDlg->setSpells(_m_bCustomSpells ? _m_customSpells : _m_defaultSpells);
    if (!_m_bCustomSpells)
        _m_pSpellsDlg->enableSpells(false);
    return TRUE;
}

VA(0x00455158, 0xb2)
void THeroPropsSpellsPage::OnOK()
{
    CPropertyPage::OnOK();
    if (_m_bCustomSpells)
        _m_customSpells = _m_pSpellsDlg->getSpells();
    _m_pNewHero->setBCustomSpells(_m_bCustomSpells);
    _m_pNewHero->setSpells(_m_bCustomSpells ? _m_customSpells : std::bitset<kNumSpells>());
    _m_bModified = _m_bModified || _m_pOldHero->getBCustomSpells() != _m_pNewHero->getBCustomSpells()
                   || _m_pNewHero->getSpells() != _m_pOldHero->getSpells();
}

VA(0x0045520a, 0x70)
void THeroPropsSpellsPage::OnCustomizeCheck()
{
    if (_m_customizeCheck.GetCheck()) {
        if (!_m_bCustomSpells) {
            _m_bCustomSpells = true;
            _m_pSpellsDlg->enableSpells(true);
            _m_pSpellsDlg->setSpells(_m_customSpells);
        }
    } else if (_m_bCustomSpells) {
        _m_bCustomSpells = false;
        _m_pSpellsDlg->setSpells(_m_defaultSpells);
        _m_pSpellsDlg->enableSpells(false);
    }
}
