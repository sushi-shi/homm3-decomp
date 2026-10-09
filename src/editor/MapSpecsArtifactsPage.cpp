// MapSpecsArtifactsPage.cpp - the artifacts page of the map
// specifications sheet (h3maped 0x46fd3c..0x470353; GOG only). Only the
// artifacts the map's version has are listed, without the special and the
// disabled ones; before Armageddon's Blade the list is read-only. Clearing
// the last allowed artifact allows the next one in the list.
#include "editor/stdafx.h"

#include "va.h"
#include "artifact.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsArtifactsPage.h"

VA(0x0046ff10, 0x9d)
TMapSpecsArtifactsPage::TMapSpecsArtifactsPage(const TGameMap& oldMap, TGameMap& newMap)
    : CPropertyPage(TMapSpecsArtifactsPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kArtifactsPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x0046ffad, 0x1c, SCALAR_DELETING_DTOR, TMapSpecsArtifactsPage)

VA(0x0046ffc9, 0x3e)
TMapSpecsArtifactsPage::~TMapSpecsArtifactsPage()
{
}

VA(0x00470007, 0x18)
void TMapSpecsArtifactsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_ENABLED_ARTIFACTS_LIST, _m_artifactsList);
}

VA(0x0047001f, 0x6)
BEGIN_MESSAGE_MAP(TMapSpecsArtifactsPage, CPropertyPage)
    ON_CLBN_CHKCHANGE(IDC_ENABLED_ARTIFACTS_LIST, OnCheckChangeArtifactsList)
END_MESSAGE_MAP()

VA(0x00470025, 0x159)
BOOL TMapSpecsArtifactsPage::OnInitDialog()
{
    GetDlgItem(IDC_ENABLED_ARTIFACTS_STATIC)->SetWindowText(SMapSpecsArtifactsPageText::kEnabledArtifactsStaticStr);
    _m_bModified = false;
    _m_disabledArtifacts = _m_newMap.getDisabledArtifacts();
    CPropertyPage::OnInitDialog();
    CRect rect;
    _m_artifactsList.GetClientRect(&rect);
    _m_artifactsList.SetColumnWidth(rect.Width() / 2);
    EGameVersion version = _m_newMap.getVersion();
    int numArtifacts;
    if (version >= GAME_VERSION_SOD)
        numArtifacts = kNumArtifacts;
    else
        numArtifacts = version < GAME_VERSION_AB ? s_kNumRoEArtifacts : s_kNumABArtifacts;
    for (int artifact = 0; artifact < numArtifacts; artifact++) {
        if (!akArtifactTraits[artifact].m_disabled && !(akArtifactTraits[artifact].m_class & s_kSpecialClass)) {
            int index = _m_artifactsList.AddString(akArtifactTraits[artifact].m_name);
            _m_artifactsList.SetItemData(index, artifact);
            _m_artifactsList.SetCheck(index, !_m_disabledArtifacts[artifact]);
        }
    }
    if (version < GAME_VERSION_AB) {
        int count = _m_artifactsList.GetCount();
        for (int index = 0; index < count; index++)
            _m_artifactsList.Enable(index, FALSE);
    }
    return TRUE;
}

VA(0x0047019f, 0x54)
void TMapSpecsArtifactsPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_newMap.setDisabledArtifacts(_m_disabledArtifacts);
    _m_bModified = _m_bModified || _m_newMap.getDisabledArtifacts() != _m_oldMap.getDisabledArtifacts();
}

VA(0x004701f3, 0x102)
void TMapSpecsArtifactsPage::OnCheckChangeArtifactsList()
{
    int index = _m_artifactsList.GetCaretIndex();
    _m_disabledArtifacts[_m_artifactsList.GetItemData(index)] = !_m_artifactsList.GetCheck(index);
    for (int artifact = 0; artifact < kNumArtifacts; artifact++) {
        if (!akArtifactTraits[artifact].m_disabled && !(akArtifactTraits[artifact].m_class & s_kSpecialClass)
            && !_m_disabledArtifacts[artifact])
            return;
    }
    int next = ++index < _m_artifactsList.GetCount() ? index : 0;
    _m_disabledArtifacts.set(_m_artifactsList.GetItemData(next), false);
    _m_artifactsList.SetCheck(next, 1);
}
