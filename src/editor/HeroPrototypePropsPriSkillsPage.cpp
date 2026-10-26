// HeroPrototypePropsPriSkillsPage.cpp - the primary skills page of the hero prototype sheet
// (h3maped 0x456b9c..0x456fc5; GOG only). OK stores the dialog's primary skills
// when they changed.
#include "editor/stdafx.h"

#include "exceptions.h"
#include "va.h"
#include "editor/PrimarySkillsDlg.h"
#include "editor/Hero.h"
#include "editor/HeroPrototypePropsPriSkillsPage.h"
#include "editor/MapEditorText.h"

VA(0x00456d70, 0x7c)
THeroPrototypePropsPriSkillsPage::THeroPrototypePropsPriSkillsPage(const THeroPrototype& oldPrototype, THeroPrototype* pNewPrototype)
    : CPropertyPage(THeroPrototypePropsPriSkillsPage::IDD),
      _m_oldPrototype(oldPrototype),
      _m_pNewPrototype(pNewPrototype),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = SHeroPrototypePropsPriSkillsPageText::kCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00456dec, 0x1c, SCALAR_DELETING_DTOR, THeroPrototypePropsPriSkillsPage)

VA(0x00456e08, 0x3e)
THeroPrototypePropsPriSkillsPage::~THeroPrototypePropsPriSkillsPage()
{
}

VA(0x00456e46, 0x6)
BEGIN_MESSAGE_MAP(THeroPrototypePropsPriSkillsPage, CPropertyPage)
END_MESSAGE_MAP()

VA(0x00456e4c, 0x107)
BOOL THeroPrototypePropsPriSkillsPage::OnInitDialog()
{
    _m_bModified = false;
    CPropertyPage::OnInitDialog();
    _m_pPrimarySkillsDlg = std::auto_ptr<TPrimarySkillsDlg>(new TPrimarySkillsDlg(this));
    if (!_m_pPrimarySkillsDlg.get())
        throw TAllocationFailure();
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pPrimarySkillsDlg->SetWindowPos(pFrame, rect.left, rect.top, rect.Width(), rect.Height(), 0);
    pFrame->DestroyWindow();
    _m_pPrimarySkillsDlg->setPrimarySkills(_m_pNewPrototype->getPrimarySkills());
    return TRUE;
}

VA(0x00456f53, 0x72)
void THeroPrototypePropsPriSkillsPage::OnOK()
{
    CPropertyPage::OnOK();
    TArray<int, H3_IDX(kNumPrimarySkills)> skills = _m_pPrimarySkillsDlg->getPrimarySkills();
    if (!(skills == _m_pNewPrototype->getPrimarySkills())) {
        _m_bModified = true;
        _m_pNewPrototype->setPrimarySkills(skills);
    }
}
