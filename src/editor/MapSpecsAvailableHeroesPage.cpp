// MapSpecsAvailableHeroesPage.cpp - the heroes page of the map
// specifications sheet (h3maped 0x470527..0x470bbd; GOG only). A hero
// placed on the map stays available: unchecking it is undone.
#include "editor/stdafx.h"

#include <string>

#include "va.h"
#include "editor/HeroPrototypePropsSheet.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsAvailableHeroesPage.h"

VA(0x00470527, 0xb4)
TMapSpecsAvailableHeroesPage::TMapSpecsAvailableHeroesPage(const TGameMap& oldMap, TGameMap& newMap)
    : CPropertyPage(TMapSpecsAvailableHeroesPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kHeroesStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x004705db, 0x1c, SCALAR_DELETING_DTOR, TMapSpecsAvailableHeroesPage)

VA(0x004705f7, 0x50)
TMapSpecsAvailableHeroesPage::~TMapSpecsAvailableHeroesPage()
{
}

VA(0x00470647, 0x31)
void TMapSpecsAvailableHeroesPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PROPERTIES_BUTTON, _m_propertiesButton);
    DDX_Control(pDX, IDC_HERO_LIST, _m_heroList);
}

VA(0x00470678, 0x6)
BEGIN_MESSAGE_MAP(TMapSpecsAvailableHeroesPage, CPropertyPage)
    ON_LBN_SELCHANGE(IDC_HERO_LIST, OnSelChangeHeroList)
    ON_LBN_SELCANCEL(IDC_HERO_LIST, OnSelCancelHeroList)
    ON_BN_CLICKED(IDC_PROPERTIES_BUTTON, OnPropertiesButton)
    ON_LBN_DBLCLK(IDC_HERO_LIST, OnDblClkHeroList)
    ON_CLBN_CHKCHANGE(IDC_HERO_LIST, OnCheckChangeHeroList)
END_MESSAGE_MAP()

VA(0x0047067e, 0x1f5)
BOOL TMapSpecsAvailableHeroesPage::OnInitDialog()
{
    GetDlgItem(IDC_AVAILABLE_HEROES_STATIC)->SetWindowText(SMapSpecsAvailableHeroesPageText::kAvailableHeroesStaticStr);
    GetDlgItem(IDC_PROPERTIES_BUTTON)->SetWindowText(SMapSpecsAvailableHeroesPageText::kPropertiesButtonStr);
    _m_bModified = false;
    const std::bitset<kNumHeroes>& disabledHeroes = _m_newMap.getDisabledHeroes();
    CPropertyPage::OnInitDialog();
    CRect rect;
    _m_heroList.GetClientRect(&rect);
    _m_heroList.SetColumnWidth(rect.Width() / 4);
    for (int heroClass = 0; heroClass < kNumHeroClasses; heroClass++) {
        const THero::TClassTraits& classTraits = THero::s_akClassTraits[heroClass];
        for (std::set<int>::const_iterator it = classTraits.m_heroes.begin(); it != classTraits.m_heroes.end(); ++it) {
            const std::bitset<3>& gameVersions = THero::s_akTraits[*it].m_gameVersions;
            if (gameVersions.test(_m_newMap.getVersion())) {
                int index = _m_heroList.AddString(_m_newMap.getHeroPrototype(*it).getName().c_str());
                _m_heroList.SetItemData(index, *it);
                _m_heroList.SetCheck(index, !disabledHeroes.test(*it));
            }
        }
    }
    if (_m_newMap.getVersion() < GAME_VERSION_AB) {
        int count = _m_heroList.GetCount();
        for (int i = 0; i < count; i++)
            _m_heroList.Enable(i, FALSE);
    }
    _m_propertiesButton.EnableWindow(FALSE);
    return TRUE;
}

VA(0x00470873, 0xbe)
void TMapSpecsAvailableHeroesPage::OnOK()
{
    CPropertyPage::OnOK();
    std::bitset<kNumHeroes> disabledHeroes;
    int count = _m_heroList.GetCount();
    for (int i = 0; i < count; i++) {
        THeroID heroID = _m_heroList.GetItemData(i);
        disabledHeroes[heroID] = !_m_heroList.GetCheck(i);
    }
    _m_newMap.setDisabledHeroes(disabledHeroes);
    _m_bModified = _m_bModified || _m_newMap.getDisabledHeroes() != _m_oldMap.getDisabledHeroes();
}

VA(0x00470931, 0x22)
void TMapSpecsAvailableHeroesPage::OnSelChangeHeroList()
{
    if (_m_newMap.getVersion() >= GAME_VERSION_SOD)
        _m_propertiesButton.EnableWindow(TRUE);
}

VA(0x00470953, 0x22)
void TMapSpecsAvailableHeroesPage::OnSelCancelHeroList()
{
    if (_m_newMap.getVersion() >= GAME_VERSION_SOD)
        _m_propertiesButton.EnableWindow(FALSE);
}

VA(0x00470975, 0x187)
void TMapSpecsAvailableHeroesPage::OnPropertiesButton()
{
    int index = _m_heroList.GetCurSel();
    THeroID heroID = _m_heroList.GetItemData(index);
    THeroPrototypePropsSheet sheet(this, _m_newMap, heroID);
    if (sheet.DoModal() && sheet.wasModified()) {
        _m_bModified = true;
        const THeroPrototype& prototype = _m_newMap.getHeroPrototype(heroID);
        CString name;
        _m_heroList.GetText(index, name);
        if ((LPCTSTR)name != prototype.getName()) {
            int check = _m_heroList.GetCheck(index);
            BOOL bEnabled = _m_heroList.IsEnabled(index);
            _m_heroList.DeleteString(index);
            _m_heroList.InsertString(index, prototype.getName().c_str());
            _m_heroList.SetItemData(index, heroID);
            _m_heroList.Enable(index, bEnabled);
            _m_heroList.SetCheck(index, check);
            _m_heroList.SetCurSel(index);
        }
    }
}

VA(0x00470afc, 0x36)
void TMapSpecsAvailableHeroesPage::OnDblClkHeroList()
{
    if (_m_newMap.getVersion() >= GAME_VERSION_SOD && _m_heroList.GetCurSel() != LB_ERR)
        OnPropertiesButton();
}

VA(0x00470b32, 0x79)
void TMapSpecsAvailableHeroesPage::OnCheckChangeHeroList()
{
    int index = _m_heroList.GetCurSel();
    if (!_m_heroList.GetCheck(index)) {
        THeroID heroID = _m_heroList.GetItemData(index);
        if (_m_newMap.getHeroesOnMap()[heroID])
            _m_heroList.SetCheck(index, 1);
    }
}
