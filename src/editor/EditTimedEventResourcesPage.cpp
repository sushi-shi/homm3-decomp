// EditTimedEventResourcesPage.cpp - the resources page of the timed event
// sheet (h3maped 0x416c06..0x416e02; Loki h3maped object 81).
#include "editor/stdafx.h"

#include "va.h"
#include "editor/EditTimedEventResourcesPage.h"
#include "editor/MapEditorText.h"
#include "editor/ResourceQuantitiesDlg.h"
#include "editor/TimedEvent.h"

VA(0x00416c22, 0xb4)
TEditTimedEventResourcesPage::TEditTimedEventResourcesPage(const TTimedEvent& event)
    : CPropertyPage(TEditTimedEventResourcesPage::IDD),
      _m_pResourceQuantitiesDlg(NULL)
{
    _m_pResourceQuantitiesDlg = new TResourceQuantitiesDlg(event.getResourceQuantities());
    if (!_m_pResourceQuantitiesDlg)
        throw TAllocationFailure();
    m_psp.pszTitle = m_strCaption = kResourcesPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00416cd6, 0x1c, SCALAR_DELETING_DTOR, TEditTimedEventResourcesPage)

VA(0x00416cf2, 0x44)
TEditTimedEventResourcesPage::~TEditTimedEventResourcesPage()
{
    delete _m_pResourceQuantitiesDlg;
}

VA(0x00416d36, 0xc)
const TResourceQuantities& TEditTimedEventResourcesPage::getResourceQuantities() const
{
    return _m_pResourceQuantitiesDlg->getResourceQuantities();
}

VA(0x00416d42, 0xa)
bool TEditTimedEventResourcesPage::wasModified() const
{
    return _m_pResourceQuantitiesDlg->wasModified();
}

VA(0x00416d4c, 0x6)
BEGIN_MESSAGE_MAP(TEditTimedEventResourcesPage, CPropertyPage)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

VA(0x00416d52, 0x2d)
int TEditTimedEventResourcesPage::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CPropertyPage::OnCreate(lpCreateStruct) == -1
        || !_m_pResourceQuantitiesDlg->Create(TResourceQuantitiesDlg::IDD, this))
        return -1;
    return 0;
}

VA(0x00416d7f, 0x17)
void TEditTimedEventResourcesPage::OnDestroy()
{
    _m_pResourceQuantitiesDlg->DestroyWindow();
    CPropertyPage::OnDestroy();
}

VA(0x00416d96, 0x54)
BOOL TEditTimedEventResourcesPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    CWnd* pFrame = GetDlgItem(IDC_RESOURCES_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pResourceQuantitiesDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    return TRUE;
}

VA(0x00416dea, 0x18)
void TEditTimedEventResourcesPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_pResourceQuantitiesDlg->OnOK();
}
