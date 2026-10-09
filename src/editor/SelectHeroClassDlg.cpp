// SelectHeroClassDlg.cpp - the hero class picker (h3maped 0x4b821b..0x4b854e;
// Loki h3maped object 90). Before Armageddon's Blade the Conflux classes
// are not offered; OK needs a selection.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/SelectHeroClassDlg.h"

VA(0x004b8237, 0x80)
TSelectHeroClassDlg::TSelectHeroClassDlg(CWnd* pParent, const THeroClassMask& mask, EGameVersion mapVersion)
    : CDialog(TSelectHeroClassDlg::IDD, pParent),
      _m_mask(mask),
      _m_mapVersion(mapVersion),
      _m_heroClass(THeroClass(kNumHeroClasses))
{
}

VA_COMPGEN(0x004b82b7, 0x1c, SCALAR_DELETING_DTOR, TSelectHeroClassDlg)
VA_COMPGEN(0x004b82d3, 0x47, IMPLICIT_DTOR, TSelectHeroClassDlg)

VA(0x004b831a, 0x2b)
void TSelectHeroClassDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDOK, _m_okButton);
    DDX_Control(pDX, IDC_HERO_CLASS_LIST, _m_heroClassList);
}

VA(0x004b8345, 0x6)
BEGIN_MESSAGE_MAP(TSelectHeroClassDlg, CDialog)
    ON_LBN_SELCHANGE(IDC_HERO_CLASS_LIST, OnSelChangeHeroClassList)
    ON_LBN_SELCANCEL(IDC_HERO_CLASS_LIST, OnSelCancelHeroClassList)
    ON_LBN_DBLCLK(IDC_HERO_CLASS_LIST, OnDblClkHeroClassList)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x004b834b, 0x25)
int TSelectHeroClassDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kSelectHeroClassCaptionStr);
    return 0;
}

VA(0x004b8370, 0x146)
BOOL TSelectHeroClassDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    CDialog::OnInitDialog();
    unsigned int numHeroClasses = _m_mapVersion >= GAME_VERSION_AB ? kNumHeroClasses : classPlanesWalker;
    for (unsigned int heroClass = 0; heroClass < numHeroClasses; heroClass++) {
        if (_m_mask[heroClass]) {
            int index = _m_heroClassList.AddString(THero::s_akClassTraits[heroClass].m_name);
            _m_heroClassList.SetItemData(index, heroClass);
        }
    }
    if (_m_mask[kNumHeroClasses]) {
        int index = _m_heroClassList.AddString(THero::s_akClassTraits[kNumHeroClasses].m_name);
        _m_heroClassList.SetItemData(index, kNumHeroClasses);
    }
    _m_heroClassList.SetCurSel(-1);
    _m_okButton.EnableWindow(FALSE);
    return TRUE;
}

VA(0x004b84b6, 0x39)
void TSelectHeroClassDlg::OnOK()
{
    CDialog::OnOK();
    _m_heroClass = THeroClass(_m_heroClassList.GetItemData(_m_heroClassList.GetCurSel()));
}

VA(0x004b84ef, 0x2b)
void TSelectHeroClassDlg::OnSelChangeHeroClassList()
{
    _m_okButton.EnableWindow(_m_heroClassList.GetCurSel() != -1);
}

VA(0x004b851a, 0xb)
void TSelectHeroClassDlg::OnSelCancelHeroClassList()
{
    _m_okButton.EnableWindow(FALSE);
}

VA(0x004b8525, 0x29)
void TSelectHeroClassDlg::OnDblClkHeroClassList()
{
    if (_m_heroClassList.GetCurSel() != -1)
        OnOK();
}
