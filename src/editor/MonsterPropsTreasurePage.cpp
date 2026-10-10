// MonsterPropsTreasurePage.cpp - the treasure page of the monster property
// sheet (h3maped 0x489e5d..0x48a5ac; GOG only). The artifact combo offers
// none and the non-special artifacts of the map's version.
#include "editor/stdafx.h"

#include <stdio.h>

#include "artifact.h"
#include "va.h"
#include "editor/Digits.h"
#include "editor/MapEditorText.h"
#include "editor/Monster.h"
#include "editor/MonsterPropsTreasurePage.h"

namespace {

enum {
    kNumRoEArtifacts = 127,
    kNumABArtifacts = 129
};

}

DATA(0x0053f3e8)
static const int g_aakResourceControlIDs[kNumGameResourceTypes][2] = {
    { IDC_RESOURCE1_EDIT, IDC_RESOURCE1_SPIN },
    { IDC_RESOURCE2_EDIT, IDC_RESOURCE2_SPIN },
    { IDC_RESOURCE3_EDIT, IDC_RESOURCE3_SPIN },
    { IDC_RESOURCE4_EDIT, IDC_RESOURCE4_SPIN },
    { IDC_RESOURCE5_EDIT, IDC_RESOURCE5_SPIN },
    { IDC_RESOURCE6_EDIT, IDC_RESOURCE6_SPIN },
    { IDC_RESOURCE7_EDIT, IDC_RESOURCE7_SPIN }
};

VA(0x0048a031, 0xa3)
TMonsterPropsTreasurePage::TMonsterPropsTreasurePage(TMonster* pMonster, EGameVersion mapVersion)
    : CPropertyPage(TMonsterPropsTreasurePage::IDD),
      _m_pMonster(pMonster),
      _m_mapVersion(mapVersion),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kTreasurePageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x0048a0d4, 0x1c, SCALAR_DELETING_DTOR, TMonsterPropsTreasurePage)
VA_COMPGEN(0x0048a0f0, 0x46, CLASS_CTOR, _TResourceControls)
VA_COMPGEN(0x0048a136, 0x36, IMPLICIT_DTOR, _TResourceControls)

VA(0x0048a16c, 0x5a)
TMonsterPropsTreasurePage::~TMonsterPropsTreasurePage()
{
}

VA(0x0048a1c6, 0x50)
void TMonsterPropsTreasurePage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_ARTIFACT_COMBO, _m_artifactCombo);
    for (unsigned int type = 0; type < kNumGameResourceTypes; type++) {
        DDX_Control(pDX, g_aakResourceControlIDs[type][0], _m_aResourceControls[type].m_quantityEdit);
        DDX_Control(pDX, g_aakResourceControlIDs[type][1], _m_aResourceControls[type].m_quantitySpin);
    }
}

VA(0x0048a216, 0x30)
bool TMonsterPropsTreasurePage::_areResourceQuantitiesModified() const
{
    for (unsigned int type = 0; type < kNumGameResourceTypes; type++) {
        if (_m_aResourceControls[type].m_quantity != _m_pMonster->getResourceQuantity(TGameResourceType(type)))
            return true;
    }
    return false;
}

VA(0x0048a246, 0x6)
BEGIN_MESSAGE_MAP(TMonsterPropsTreasurePage, CPropertyPage)
END_MESSAGE_MAP()

VA(0x0048a24c, 0x28d)
BOOL TMonsterPropsTreasurePage::OnInitDialog()
{
    GetDlgItem(IDC_RESOURCES_STATIC)->SetWindowText(SMonsterPropsTreasurePageText::kResourcesStaticStr);
    GetDlgItem(IDC_RESOURCE1_STATIC)->SetWindowText(SMonsterPropsTreasurePageText::kRes1StaticStr);
    GetDlgItem(IDC_RESOURCE2_STATIC)->SetWindowText(SMonsterPropsTreasurePageText::kRes2StaticStr);
    GetDlgItem(IDC_RESOURCE3_STATIC)->SetWindowText(SMonsterPropsTreasurePageText::kRes3StaticStr);
    GetDlgItem(IDC_RESOURCE4_STATIC)->SetWindowText(SMonsterPropsTreasurePageText::kRes4StaticStr);
    GetDlgItem(IDC_RESOURCE5_STATIC)->SetWindowText(SMonsterPropsTreasurePageText::kRes5StaticStr);
    GetDlgItem(IDC_RESOURCE6_STATIC)->SetWindowText(SMonsterPropsTreasurePageText::kRes6StaticStr);
    GetDlgItem(IDC_RESOURCE7_STATIC)->SetWindowText(SMonsterPropsTreasurePageText::kRes7StaticStr);
    GetDlgItem(IDC_ARTIFACT_STATIC)->SetWindowText(SMonsterPropsTreasurePageText::kArtifactStaticStr);
    _m_bModified = false;
    unsigned int type;
    for (type = 0; type < kNumGameResourceTypes; type++)
        _m_aResourceControls[type].m_quantity = _m_pMonster->getResourceQuantity(TGameResourceType(type));
    _m_artifact = _m_pMonster->getArtifact();
    CPropertyPage::OnInitDialog();
    for (type = 0; type < kNumGameResourceTypes; type++) {
        _TResourceControls& controls = _m_aResourceControls[type];
        controls.m_quantityEdit.LimitText(TDigits<TMonster::s_kMaxResourceQuantity>::getDigits());
        CString text;
        text.Format("%d", controls.m_quantity);
        controls.m_quantityEdit.SetWindowText(text);
        controls.m_quantitySpin.SetRange(0, UD_MAXVAL);
    }
    _m_artifactCombo.SetItemData(_m_artifactCombo.AddString(kSelNoneStr), ARTIFACT_NONE);
    int numArtifacts = _m_mapVersion >= GAME_VERSION_SOD ? ARTIFACT_COUNT
                       : (_m_mapVersion >= GAME_VERSION_AB ? kNumABArtifacts : kNumRoEArtifacts);
    for (int artifact = 0; artifact < numArtifacts; artifact++) {
        if (!(akArtifactTraits[artifact].m_class & ArtifactClassSpecial))
            _m_artifactCombo.SetItemData(_m_artifactCombo.AddString(akArtifactTraits[artifact].m_name), artifact);
    }
    int index = 0;
    while (_m_artifactCombo.GetItemData(index) != _m_artifact)
        index++;
    _m_artifactCombo.SetCurSel(index);
    return TRUE;
}

VA(0x0048a4d9, 0xd3)
void TMonsterPropsTreasurePage::OnOK()
{
    CPropertyPage::OnOK();
    for (unsigned int type = 0; type < kNumGameResourceTypes; type++) {
        int value = 0;
        CString text;
        _m_aResourceControls[type].m_quantityEdit.GetWindowText(text);
        sscanf(text, "%d", &value);
        _m_aResourceControls[type].m_quantity = value;
    }
    _m_artifact = TArtifact(_m_artifactCombo.GetItemData(_m_artifactCombo.GetCurSel()));
    _m_bModified = _m_bModified || _areResourceQuantitiesModified() || _m_artifact != _m_pMonster->getArtifact();
}
