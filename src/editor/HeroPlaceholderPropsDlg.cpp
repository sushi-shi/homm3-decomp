// HeroPlaceholderPropsDlg.cpp - the hero placeholder's properties dialog
// (h3maped 0x44d108..0x44e2cf; GOG only). A specific hero comes from the
// heroes the map still offers in its class (the placeholder's own hero
// included); the power rating runs 1..8.
#include "editor/stdafx.h"

#include <stdio.h>

#include "exceptions.h"
#include "va.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/FormattedString.h"
#include "editor/HeroPlaceholderPropsDlg.h"
#include "editor/MapEditorText.h"
#include "editor/Player.h"

VA(0x0044d2dc, 0x218)
THeroPlaceholderPropsDlg::THeroPlaceholderPropsDlg(CWnd* pParent, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
    : CDialog(THeroPlaceholderPropsDlg::IDD, pParent),
      _m_pMap(pMap),
      _m_bSecondLayer(bSecondLayer),
      _m_objectID(objID),
      _m_pOldPlaceholder(_getOldPlaceholder()),
      _m_pNewPlaceholder(NULL),
      _m_bModified(false)
{
    _m_heroChoice = -1;
    _m_pNewMap = std::auto_ptr<TGameMap>(new TGameMap(*_m_pMap));
    if (!_m_pNewMap.get())
        throw TAllocationFailure();
    TGameMap::TLayer& layer = _m_pNewMap->getLayer(_m_bSecondLayer);
    _m_pNewPlaceholder = dynamic_cast<THeroPlaceholder*>(layer.getPObject(_m_objectID));
}

VA_COMPGEN(0x0044d4f4, 0x1c, SCALAR_DELETING_DTOR, THeroPlaceholderPropsDlg)
VA_COMPGEN(0x0044d523, 0xd8, IMPLICIT_DTOR, THeroPlaceholderPropsDlg)

VA(0x0044d5fb, 0x42)
const THeroPlaceholder* THeroPlaceholderPropsDlg::_getOldPlaceholder() const
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    const TGameObject* pObject = layer.getPObject(_m_objectID);
    return dynamic_cast<const THeroPlaceholder*>(pObject);
}

VA(0x0044d63d, 0xbe)
void THeroPlaceholderPropsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_POWER_RATING_STATIC, _m_powerRatingStatic);
    DDX_Control(pDX, IDC_HERO_IDENTITY_STATIC, _m_heroStatic);
    DDX_Control(pDX, IDC_PLACEHOLDER_CLASS_STATIC, _m_classStatic);
    DDX_Control(pDX, IDC_OWNER_COMBO, _m_ownerCombo);
    DDX_Control(pDX, IDC_POWER_RATING_SPIN, _m_powerRatingSpin);
    DDX_Control(pDX, IDC_POWER_RATING_EDIT, _m_powerRatingEdit);
    DDX_Control(pDX, IDC_IDENTITY_COMBO, _m_heroCombo);
    DDX_Control(pDX, IDC_CLASS_COMBO, _m_classCombo);
    DDX_Radio(pDX, IDC_ANY_HERO_RADIO, _m_heroChoice);
    DDX_Control(pDX, IDC_SPECIFIC_HERO_RADIO, _m_specificHeroRadio);
}

VA(0x0044d6fb, 0x6)
BEGIN_MESSAGE_MAP(THeroPlaceholderPropsDlg, CDialog)
    ON_WM_CREATE()
    ON_BN_CLICKED(IDC_ANY_HERO_RADIO, OnHeroRadio)
    ON_EN_KILLFOCUS(IDC_POWER_RATING_EDIT, OnKillFocusPowerRatingEdit)
    ON_BN_CLICKED(IDC_SPECIFIC_HERO_RADIO, OnHeroRadio)
    ON_CBN_SELCHANGE(IDC_CLASS_COMBO, OnSelChangeClassCombo)
END_MESSAGE_MAP()

VA(0x0044d701, 0x2f)
int THeroPlaceholderPropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (result == IDOK && _m_bModified)
        *_m_pMap = *_m_pNewMap;
    return result;
}

VA(0x0044d730, 0x9c)
int THeroPlaceholderPropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    CString caption;
    caption.Format(kObjectPropertiesCaptionFmtStr, _m_pNewPlaceholder->getTypeName().c_str());
    SetWindowText(caption);
    return 0;
}

VA(0x0044d7cc, 0x4a6)
BOOL THeroPlaceholderPropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_OWNER_COMBO_STATIC)->SetWindowText(SHeroPlaceholderPropsDlgText::kOwnerStaticStr);
    GetDlgItem(IDC_PLACEHOLDER_IDENTITY_STATIC)->SetWindowText(SHeroPlaceholderPropsDlgText::kIdentityStaticStr);
    GetDlgItem(IDC_ANY_HERO_RADIO)->SetWindowText(SHeroPlaceholderPropsDlgText::kAnyHeroRadioStr);
    GetDlgItem(IDC_SPECIFIC_HERO_RADIO)->SetWindowText(SHeroPlaceholderPropsDlgText::kSpecificHeroRadioStr);
    GetDlgItem(IDC_POWER_RATING_STATIC)->SetWindowText(SHeroPlaceholderPropsDlgText::kPowerRatingStaticStr);
    GetDlgItem(IDC_PLACEHOLDER_CLASS_STATIC)->SetWindowText(SHeroPlaceholderPropsDlgText::kClassStaticStr);
    GetDlgItem(IDC_HERO_IDENTITY_STATIC)->SetWindowText(SHeroPlaceholderPropsDlgText::kHeroIdentityStaticStr);
    _m_bModified = false;
    _m_heroChoice = _m_pNewPlaceholder->hasHeroID();
    unsigned int heroClass;
    for (heroClass = 0; heroClass < kNumHeroClasses; heroClass++)
        _m_aHeroesInClass[heroClass] = _m_pNewMap->getAvailableHeroesInClass(THeroClass(heroClass));
    if (_m_pNewPlaceholder->hasHeroID()) {
        THeroID heroID = _m_pNewPlaceholder->getHeroID();
        _m_aHeroesInClass[THero::s_akTraits[heroID].m_class].insert(heroID);
    }
    CDialog::OnInitDialog();
    TPlayerMask owners = _m_pNewMap->getAvailableHeroOwnersMask();
    owners.set(_m_pNewPlaceholder->getOwner(), true);
    int index;
    unsigned int player;
    for (player = 0; player < kNumPlayers; player++) {
        if (owners[player])
            _m_ownerCombo.SetItemData(_m_ownerCombo.AddString(akPlayerTraits[player].m_pColorName), player);
    }
    index = 0;
    while (_m_ownerCombo.GetItemData(index) != _m_pNewPlaceholder->getOwner())
        index++;
    _m_ownerCombo.SetCurSel(index);
    _m_powerRatingEdit.LimitText(TDigits<THeroPlaceholder::s_kMaxPowerRank>::getDigits());
    _m_powerRatingSpin.SetRange(THeroPlaceholder::s_kMinPowerRank, THeroPlaceholder::s_kMaxPowerRank);
    for (heroClass = 0; heroClass < kNumHeroClasses; heroClass++) {
        if (!_m_aHeroesInClass[heroClass].empty())
            _m_classCombo.SetItemData(_m_classCombo.AddString(THero::s_akClassTraits[heroClass].m_name), heroClass);
    }
    if (_m_heroChoice) {
        _m_powerRatingStatic.EnableWindow(FALSE);
        _m_powerRatingEdit.EnableWindow(FALSE);
        _m_powerRatingSpin.EnableWindow(FALSE);
        THeroID heroID = _m_pNewPlaceholder->getHeroID();
        THeroClass heroIDClass = THero::s_akTraits[heroID].m_class;
        index = 0;
        while (_m_classCombo.GetItemData(index) != heroIDClass)
            index++;
        _m_classCombo.SetCurSel(index);
        for (std::set<THeroID>::const_iterator it = _m_aHeroesInClass[heroIDClass].begin();
             it != _m_aHeroesInClass[heroIDClass].end(); ++it)
            _m_heroCombo.SetItemData(_m_heroCombo.AddString(_m_pNewMap->getHeroPrototype(*it).getName().c_str()), *it);
        index = 0;
        while (_m_heroCombo.GetItemData(index) != heroID)
            index++;
        _m_heroCombo.SetCurSel(index);
    } else {
        _m_classStatic.EnableWindow(FALSE);
        _m_classCombo.EnableWindow(FALSE);
        _m_heroStatic.EnableWindow(FALSE);
        _m_heroCombo.EnableWindow(FALSE);
        if (_m_classCombo.GetCount() == 0)
            _m_specificHeroRadio.EnableWindow(FALSE);
        _m_powerRatingEdit.SetWindowText(TFormattedString("%d", _m_pNewPlaceholder->getPowerRank()));
    }
    return TRUE;
}

VA(0x0044dc72, 0x232)
void THeroPlaceholderPropsDlg::OnOK()
{
    CDialog::OnOK();
    TPlayer owner = TPlayer(_m_ownerCombo.GetItemData(_m_ownerCombo.GetCurSel()));
    if (_m_pNewPlaceholder->getOwner() != owner)
        _m_pNewMap->setPlaceholderOwner(_m_bSecondLayer, _m_objectID, owner);
    if (_m_heroChoice) {
        THeroClass heroClass = THeroClass(_m_classCombo.GetItemData(_m_classCombo.GetCurSel()));
        THeroID heroID = THeroID(_m_heroCombo.GetItemData(_m_heroCombo.GetCurSel()));
        if (!_m_pNewPlaceholder->hasHeroID() || _m_pNewPlaceholder->getHeroID() != heroID)
            _m_pNewMap->setPlaceholderHeroID(_m_bSecondLayer, _m_objectID,
                                             heroID);
        _m_pNewPlaceholder->setPowerRank(THeroPlaceholder::s_kMinPowerRank);
    } else {
        if (_m_pNewPlaceholder->hasHeroID())
            _m_pNewMap->clearPlaceholderHeroID(_m_bSecondLayer, _m_objectID);
        int powerRank = 0;
        CString text;
        _m_powerRatingEdit.GetWindowText(text);
        sscanf(text, "%d", &powerRank);
        powerRank = clamp(int(THeroPlaceholder::s_kMinPowerRank), powerRank, int(THeroPlaceholder::s_kMaxPowerRank));
        _m_pNewPlaceholder->setPowerRank(powerRank);
    }
    _m_bModified = _m_bModified || _m_pNewPlaceholder->getOwner() != _m_pOldPlaceholder->getOwner()
                   || _m_pNewPlaceholder->hasHeroID() != _m_pOldPlaceholder->hasHeroID();
    if (!_m_bModified) {
        if (_m_pNewPlaceholder->hasHeroID())
            _m_bModified = _m_pNewPlaceholder->getHeroID() != _m_pOldPlaceholder->getHeroID();
        else
            _m_bModified = _m_pNewPlaceholder->getPowerRank() != _m_pOldPlaceholder->getPowerRank();
    }
}

VA(0x0044dea4, 0x205)
void THeroPlaceholderPropsDlg::OnHeroRadio()
{
    bool bHadHero = _m_heroChoice != 0;
    UpdateData(TRUE);
    if (_m_heroChoice != 0) {
        if (!bHadHero) {
            _m_powerRatingEdit.SetWindowText("");
            _m_powerRatingStatic.EnableWindow(FALSE);
            _m_powerRatingEdit.EnableWindow(FALSE);
            _m_powerRatingSpin.EnableWindow(FALSE);
            _m_classStatic.EnableWindow(TRUE);
            _m_classCombo.EnableWindow(TRUE);
            _m_heroStatic.EnableWindow(TRUE);
            _m_heroCombo.EnableWindow(TRUE);
            _m_classCombo.SetCurSel(0);
            THeroClass heroClass = THeroClass(_m_classCombo.GetItemData(0));
            for (std::set<THeroID>::const_iterator it = _m_aHeroesInClass[heroClass].begin();
                 it != _m_aHeroesInClass[heroClass].end(); ++it)
                _m_heroCombo.SetItemData(_m_heroCombo.AddString(_m_pNewMap->getHeroPrototype(*it).getName().c_str()), *it);
            _m_heroCombo.SetCurSel(0);
        }
    } else if (bHadHero) {
        _m_heroCombo.ResetContent();
        _m_heroStatic.EnableWindow(FALSE);
        _m_heroCombo.EnableWindow(FALSE);
        _m_classCombo.SetCurSel(-1);
        _m_classStatic.EnableWindow(FALSE);
        _m_classCombo.EnableWindow(FALSE);
        _m_powerRatingStatic.EnableWindow(TRUE);
        _m_powerRatingEdit.EnableWindow(TRUE);
        _m_powerRatingSpin.EnableWindow(TRUE);
        _m_powerRatingEdit.SetWindowText(TFormattedString("%d", THeroPlaceholder::s_kMinPowerRank));
    }
}

VA(0x0044e0a9, 0xa6)
void THeroPlaceholderPropsDlg::OnKillFocusPowerRatingEdit()
{
    int powerRank = 0;
    CString text;
    _m_powerRatingEdit.GetWindowText(text);
    sscanf(text, "%d", &powerRank);
    if (powerRank < THeroPlaceholder::s_kMinPowerRank || powerRank > THeroPlaceholder::s_kMaxPowerRank) {
        powerRank = clamp(int(THeroPlaceholder::s_kMinPowerRank), powerRank, int(THeroPlaceholder::s_kMaxPowerRank));
        text.Format("%d", powerRank);
        _m_powerRatingEdit.SetWindowText(text);
    }
}

VA(0x0044e14f, 0xbf)
void THeroPlaceholderPropsDlg::OnSelChangeClassCombo()
{
    _m_heroCombo.ResetContent();
    THeroClass heroClass = THeroClass(_m_classCombo.GetItemData(_m_classCombo.GetCurSel()));
    for (std::set<THeroID>::const_iterator it = _m_aHeroesInClass[heroClass].begin();
         it != _m_aHeroesInClass[heroClass].end(); ++it)
        _m_heroCombo.SetItemData(_m_heroCombo.AddString(_m_pNewMap->getHeroPrototype(*it).getName().c_str()), *it);
    _m_heroCombo.SetCurSel(0);
}
