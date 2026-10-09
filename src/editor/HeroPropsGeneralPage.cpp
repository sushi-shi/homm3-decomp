// HeroPropsGeneralPage.cpp - the general pages of the hero property sheets
// (h3maped 0x44f569..0x452b40; Loki h3maped object 84). The specific
// hero's page renders the 163 portraits once and stretches the shown one
// into a bitmap sized to the portrait control; a Restoration of Erathia
// map shows two other portraits in place of 128 and 129.
#include "editor/stdafx.h"

#include <stdio.h>

#include <set>
#include <string>

#include "bitmap816.h"
#include "exceptions.h"
#include "herotraits.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "va.h"
#include "editor/Digits.h"
#include "editor/FormattedString.h"
#include "editor/GameMap.h"
#include "editor/GDIObjectSelector.h"
#include "editor/HeroPropsGeneralPage.h"
#include "editor/MapEditorText.h"
#include "editor/Player.h"
#include "editor/StringUtil.h"
#include "editor/Town.h"

// The experience of each of the first twelve levels, the combo's choices.
DATA(0x005372d8)
static const int g_akLevelExperience[] = { 0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500 };

// The patrol choices: no patrol, standing still and radii of one to ten
// squares.
struct TPatrolRadiusTraits {
    const char* m_name;
    int m_radius;
};

static const unsigned int g_kNumPortraits = sizeof(akHeroTraits) / sizeof(akHeroTraits[0]);

VA(0x0044f71c, 0x6)
BEGIN_MESSAGE_MAP(THeroPropsGeneralPage, CPropertyPage)
    ON_BN_CLICKED(IDC_CUSTOMIZE_EXPERIENCE_CHECK, OnCustomizeExperienceCheck)
END_MESSAGE_MAP()

VA(0x0044f722, 0xdb)
THeroPropsGeneralPage::THeroPropsGeneralPage(UINT nIDTemplate, const TGameMap& oldMap, TGameMap& newMap,
                                             TMapObjectRef heroRef)
    : CPropertyPage(nIDTemplate),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_heroRef(heroRef),
      _m_pOldHero(_getOldHero()),
      _m_pNewHero(_getNewHero()),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kGeneralPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x0044f7fd, 0x1c, SCALAR_DELETING_DTOR, THeroPropsGeneralPage)

VA_COMPGEN(0x0044f819, 0x59, IMPLICIT_DTOR, THeroPropsGeneralPage)

VA(0x0044f872, 0x66)
void THeroPropsGeneralPage::_setDefaultExperience(THeroID heroID)
{
    if (!_m_bCustomExperience)
        _m_experienceCombo.SetWindowText(TFormattedString("%d", _m_newMap.getHeroPrototype(heroID).getExperience()));
}

VA(0x0044f8d8, 0x62)
const THero* THeroPropsGeneralPage::_getOldHero() const
{
    const TGameMap::TLayer& layer = _m_oldMap.getLayer(_m_heroRef.getBSecondLayer());
    const TGameObject* pObject = layer.getPObject(_m_heroRef.getObjectID());
    const THero* pHero = dynamic_cast<const THero*>(pObject);
    if (pHero == NULL)
        pHero = dynamic_cast<const TTown*>(pObject)->getPVisitingHero();
    return pHero;
}

VA(0x0044f93a, 0x62)
THero* THeroPropsGeneralPage::_getNewHero()
{
    TGameMap::TLayer& layer = _m_newMap.getLayer(_m_heroRef.getBSecondLayer());
    TGameObject* pObject = layer.getPObject(_m_heroRef.getObjectID());
    THero* pHero = dynamic_cast<THero*>(pObject);
    if (pHero == NULL)
        pHero = dynamic_cast<TTown*>(pObject)->getPVisitingHero();
    return pHero;
}

VA(0x0044f99c, 0x43)
void THeroPropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_EXPERIENCE_COMBO, _m_experienceCombo);
    DDX_Control(pDX, IDC_PATROL_COMBO, _m_patrolCombo);
    DDX_Control(pDX, IDC_CUSTOMIZE_EXPERIENCE_CHECK, _m_customizeExperienceCheck);
}

VA(0x0044f9df, 0x375)
BOOL THeroPropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_EXPERIENCE_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kExperienceStaticStr);
    GetDlgItem(IDC_PATROL_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kPatrolStaticStr);
    GetDlgItem(IDC_CUSTOMIZE_EXPERIENCE_CHECK)->SetWindowText(SHeroPropsGeneralPageText::kCustomizeGenderCheckStr);
    _m_bModified = false;
    _m_bCustomExperience = _m_pNewHero->getBCustomExperience();
    _m_experience = _m_bCustomExperience ? _m_pNewHero->getExperience() : 0;
    CPropertyPage::OnInitDialog();
    for (const int* pExperience = g_akLevelExperience;
         pExperience < g_akLevelExperience + sizeof(g_akLevelExperience) / sizeof(g_akLevelExperience[0]);
         pExperience++)
        _m_experienceCombo.AddString(TFormattedString("%d", *pExperience));
    _m_experienceCombo.GetWindow(GW_CHILD)->ModifyStyle(0, ES_NUMBER);
    _m_experienceCombo.LimitText(TDigits<s_kMaxExperience>::getDigits());
    if (_m_bCustomExperience) {
        _m_experienceCombo.SetWindowText(TFormattedString("%d", _m_experience));
        _m_customizeExperienceCheck.SetCheck(1);
    } else {
        _m_experienceCombo.SetWindowText(TFormattedString(
            "%d", _m_pNewHero->hasHeroID() ? _m_newMap.getHeroPrototype(_m_pNewHero->getHeroID()).getExperience()
                                           : 0));
        _m_experienceCombo.EnableWindow(FALSE);
        _m_customizeExperienceCheck.SetCheck(0);
    }
    DATA_COMPGEN_GUARD(0x0059f9d8, patrolRadiiGuard, akPatrolRadii)
    DATA(0x0059fa08) static const TPatrolRadiusTraits akPatrolRadii[] = {
        { kSelNoneStr, -1 },
        { kStandStillStr, 0 },
        { kRadiusOneSquareStr, 1 },
        { kRadiusTwoSquaresStr, 2 },
        { kRadiusThreeSquaresStr, 3 },
        { kRadiusFourSquaresStr, 4 },
        { kRadiusFiveSquaresStr, 5 },
        { kRadiusSixSquaresStr, 6 },
        { kRadiusSevenSquaresStr, 7 },
        { kRadiusEightSquaresStr, 8 },
        { kRadiusNineSquaresStr, 9 },
        { kRadiusTenSquaresStr, 10 }
    };
    for (const TPatrolRadiusTraits* pTraits = akPatrolRadii;
         pTraits < akPatrolRadii + sizeof(akPatrolRadii) / sizeof(akPatrolRadii[0]); pTraits++)
        _m_patrolCombo.SetItemData(_m_patrolCombo.AddString(pTraits->m_name), pTraits->m_radius);
    int index;
    for (index = 0; int(_m_patrolCombo.GetItemData(index)) != _m_pNewHero->getPatrolRadius(); index++)
        ;
    _m_patrolCombo.SetCurSel(index);
    return TRUE;
}

VA(0x0044fd54, 0x110)
void THeroPropsGeneralPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_experience = 0;
    if (_m_bCustomExperience) {
        CString text;
        _m_experienceCombo.GetWindowText(text);
        sscanf(text, "%d", &_m_experience);
    }
    int patrolRadius = _m_patrolCombo.GetItemData(_m_patrolCombo.GetCurSel());
    _m_pNewHero->setBCustomExperience(_m_bCustomExperience);
    _m_pNewHero->setExperience(_m_experience);
    _m_pNewHero->setPatrolRadius(patrolRadius);
    _m_bModified = _m_bModified || _m_pOldHero->getBCustomExperience() != _m_pNewHero->getBCustomExperience()
                   || _m_pNewHero->getExperience() != _m_pOldHero->getExperience()
                   || _m_pNewHero->getPatrolRadius() != _m_pOldHero->getPatrolRadius();
}

VA(0x0044fe64, 0x11e)
void THeroPropsGeneralPage::OnCustomizeExperienceCheck()
{
    if (_m_customizeExperienceCheck.GetCheck()) {
        if (!_m_bCustomExperience) {
            _m_experienceCombo.EnableWindow(TRUE);
            _m_experienceCombo.SetWindowText(TFormattedString("%d", _m_experience));
            _m_bCustomExperience = true;
        }
    } else if (_m_bCustomExperience) {
        _m_experienceCombo.SetWindowText(TFormattedString(
            "%d", _m_pNewHero->hasHeroID() ? _m_newMap.getHeroPrototype(_m_pNewHero->getHeroID()).getExperience()
                                           : 0));
        _m_experienceCombo.EnableWindow(FALSE);
        _m_experience = 0;
        _m_bCustomExperience = false;
    }
}

VA(0x0044ff82, 0xaa)
TRandomHeroPropsGeneralPage::TRandomHeroPropsGeneralPage(const TGameMap& oldMap, TGameMap& newMap,
                                                         TMapObjectRef heroRef, bool bAnyAvailableOwner)
    : THeroPropsGeneralPage(TRandomHeroPropsGeneralPage::IDD, oldMap, newMap, heroRef),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_heroRef(heroRef),
      _m_bAnyAvailableOwner(bAnyAvailableOwner),
      _m_pOldHero(_getOldHero()),
      _m_pNewHero(_getNewHero()),
      _m_bModified(false)
{
    m_nIDHelp = TNonRandomHeroPropsGeneralPage::IDD;
}

VA_COMPGEN(0x0045002c, 0x1c, SCALAR_DELETING_DTOR, TRandomHeroPropsGeneralPage)

VA_COMPGEN(0x00450048, 0x38, IMPLICIT_DTOR, TRandomHeroPropsGeneralPage)

VA(0x00450080, 0x75)
const TRandomHero* TRandomHeroPropsGeneralPage::_getOldHero() const
{
    const TGameMap::TLayer& layer = _m_oldMap.getLayer(_m_heroRef.getBSecondLayer());
    const TGameObject* pObject = layer.getPObject(_m_heroRef.getObjectID());
    const TRandomHero* pHero = dynamic_cast<const TRandomHero*>(pObject);
    if (pHero == NULL)
        pHero = dynamic_cast<const TRandomHero*>(dynamic_cast<const TTown*>(pObject)->getPVisitingHero());
    return pHero;
}

VA(0x004500f5, 0x75)
TRandomHero* TRandomHeroPropsGeneralPage::_getNewHero()
{
    TGameMap::TLayer& layer = _m_newMap.getLayer(_m_heroRef.getBSecondLayer());
    TGameObject* pObject = layer.getPObject(_m_heroRef.getObjectID());
    TRandomHero* pHero = dynamic_cast<TRandomHero*>(pObject);
    if (pHero == NULL)
        pHero = dynamic_cast<TRandomHero*>(dynamic_cast<TTown*>(pObject)->getPVisitingHero());
    return pHero;
}

VA(0x0045016a, 0x25)
void TRandomHeroPropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    THeroPropsGeneralPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PLAYER_COMBO, _m_ownerCombo);
}

VA(0x0045018f, 0x103)
BOOL TRandomHeroPropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_PLAYER_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kPlayerStaticStr);
    _m_bModified = false;
    THeroPropsGeneralPage::OnInitDialog();
    TPlayerMask availableOwners;
    if (_m_bAnyAvailableOwner)
        availableOwners = _m_newMap.getAvailableHeroOwnersMask();
    availableOwners.set(_m_pNewHero->getOwner());
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        if (availableOwners[player])
            _m_ownerCombo.SetItemData(_m_ownerCombo.AddString(akPlayerTraits[player].m_pName), player);
    }
    int index;
    for (index = 0; TPlayer(_m_ownerCombo.GetItemData(index)) != _m_pNewHero->getOwner(); index++)
        ;
    _m_ownerCombo.SetCurSel(index);
    return TRUE;
}

VA(0x00450292, 0x85)
void TRandomHeroPropsGeneralPage::OnOK()
{
    THeroPropsGeneralPage::OnOK();
    TPlayer owner = TPlayer(_m_ownerCombo.GetItemData(_m_ownerCombo.GetCurSel()));
    if (owner != _m_pNewHero->getOwner())
        _m_newMap.setHeroOwner(_m_heroRef.getBSecondLayer(), _m_heroRef.getObjectID(), owner);
    _m_bModified = THeroPropsGeneralPage::wasModified() || _m_bModified
                   || _m_pNewHero->getOwner() != _m_pOldHero->getOwner();
}

VA(0x00450317, 0x6)
BEGIN_MESSAGE_MAP(TIdentifiedHeroPropsGeneralPage, THeroPropsGeneralPage)
    ON_CBN_SELCHANGE(IDC_IDENTITY_COMBO, OnSelChangeIdentityCombo)
    ON_NOTIFY(UDN_DELTAPOS, IDC_PORTRAIT_SPIN, OnDeltaposPortraitSpin)
    ON_EN_CHANGE(IDC_NAME_EDIT, OnChangeNameEdit)
    ON_BN_CLICKED(IDC_CUSTOMIZE_NAME_CHECK, OnCustomizeNameCheck)
    ON_BN_CLICKED(IDC_CUSTOMIZE_PORTRAIT_CHECK, OnCustomizePortraitCheck)
END_MESSAGE_MAP()

VA(0x0045031d, 0x356)
TIdentifiedHeroPropsGeneralPage::TIdentifiedHeroPropsGeneralPage(
    UINT nIDTemplate, TIdentifiedHeroPropsGeneralPageParentSheet* pParentSheet, const TGameMap& oldMap,
    TGameMap& newMap, TMapObjectRef heroRef)
    : THeroPropsGeneralPage(nIDTemplate, oldMap, newMap, heroRef),
      _m_pParentSheet(pParentSheet),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_heroRef(heroRef),
      _m_pOldHero(_getOldHero()),
      _m_pNewHero(_getNewHero()),
      _m_bModified(false)
{
    _m_apPortrait = TAutoArrayPtr<std::auto_ptr<T16bppDIBSection> >(
        new std::auto_ptr<T16bppDIBSection>[g_kNumPortraits]);
    if (!_m_apPortrait.get())
        throw TAllocationFailure();
    for (unsigned int portrait = 0; portrait < g_kNumPortraits; portrait++) {
        TResourcePtr<Bitmap816> pBitmap(ResourceManager::GetBitmap816(akHeroTraits[portrait].m_large_portrait_name));
        if (!pBitmap.get())
            throw TRuntimeError();
        std::auto_ptr<T16bppDIBSection> pPortrait(new T16bppDIBSection(pBitmap->GetWidth(), pBitmap->GetHeight()));
        if (!pPortrait.get())
            throw TAllocationFailure();
        pBitmap->Draw(0, 0, pBitmap->GetWidth(), pBitmap->GetHeight(), pPortrait->getPixels(), 0, 0,
                      pPortrait->getWidth(), pPortrait->getHeight(), pPortrait->getPitch(), false);
        _m_apPortrait.get()[portrait] = pPortrait;
    }
}

VA_COMPGEN(0x00450680, 0x1c, SCALAR_DELETING_DTOR, TIdentifiedHeroPropsGeneralPage)

VA(0x004506e7, 0x10e)
TIdentifiedHeroPropsGeneralPage::~TIdentifiedHeroPropsGeneralPage()
{
}

VA(0x004507f5, 0x15b)
void TIdentifiedHeroPropsGeneralPage::setHeroID(THeroID heroID)
{
    THeroClass heroClass = THero::s_akTraits[heroID].m_class;
    THeroClass oldHeroClass = THero::s_akTraits[_m_heroID].m_class;
    _m_heroID = heroID;
    if (heroClass != oldHeroClass) {
        _m_identityCombo.ResetContent();
        std::set<THeroID> heroes = _m_newMap.getAvailableHeroesInClass(heroClass);
        if (heroClass == _m_pNewHero->getHeroClass())
            heroes.insert(_m_pNewHero->getHeroID());
        for (std::set<THeroID>::const_iterator it = heroes.begin(); it != heroes.end(); ++it)
            _m_identityCombo.SetItemData(
                _m_identityCombo.AddString(_m_newMap.getHeroPrototype(*it).getName().c_str()), *it);
    }
    int index;
    for (index = 0; THeroID(_m_identityCombo.GetItemData(index)) != _m_heroID; index++)
        ;
    _m_identityCombo.SetCurSel(index);
    _onSetHeroID();
}

VA(0x00450950, 0x75)
const TIdentifiedHero* TIdentifiedHeroPropsGeneralPage::_getOldHero() const
{
    const TGameMap::TLayer& layer = _m_oldMap.getLayer(_m_heroRef.getBSecondLayer());
    const TGameObject* pObject = layer.getPObject(_m_heroRef.getObjectID());
    const TIdentifiedHero* pHero = dynamic_cast<const TIdentifiedHero*>(pObject);
    if (pHero == NULL)
        pHero = dynamic_cast<const TIdentifiedHero*>(dynamic_cast<const TTown*>(pObject)->getPVisitingHero());
    return pHero;
}

VA(0x004509c5, 0x75)
TIdentifiedHero* TIdentifiedHeroPropsGeneralPage::_getNewHero()
{
    TGameMap::TLayer& layer = _m_newMap.getLayer(_m_heroRef.getBSecondLayer());
    TGameObject* pObject = layer.getPObject(_m_heroRef.getObjectID());
    TIdentifiedHero* pHero = dynamic_cast<TIdentifiedHero*>(pObject);
    if (pHero == NULL)
        pHero = dynamic_cast<TIdentifiedHero*>(dynamic_cast<TTown*>(pObject)->getPVisitingHero());
    return pHero;
}

VA(0x00450a3a, 0xf3)
void TIdentifiedHeroPropsGeneralPage::_onSetHeroID()
{
    _setDefaultExperience(_m_heroID);
    const THeroPrototype& prototype = _m_newMap.getHeroPrototype(_m_heroID);
    if (!_m_customizeNameCheck.GetCheck())
        _m_nameEdit.SetWindowText(prototype.getName().c_str());
    if (!_m_customizePortraitCheck.GetCheck())
        _setPortrait(prototype.getPortrait());
    _m_defaultGenderRadio.SetWindowText(
        TFormattedString(SHeroPropsGeneralPageText::kDefaultGenderRadioFmtStr, prototype.getSex() ? SHeroPropsGeneralPageText::kFemaleStr : SHeroPropsGeneralPageText::kMaleStr));
    _m_pParentSheet->onSetHeroID(_m_heroID);
}

inline void TIdentifiedHeroPropsGeneralPage::_adjustPortrait(int& portrait) const
{
    if (_m_newMap.getVersion() == GAME_VERSION_ROE) {
        if (portrait == 128)
            portrait = 146;
        else if (portrait == 129)
            portrait = 156;
    }
}

VA(0x00450b2d, 0x232)
void TIdentifiedHeroPropsGeneralPage::_setPortrait(int portrait)
{
    _adjustPortrait(portrait);
    _m_portraitStatic.SetBitmap(NULL);
    {
        CClientDC dc(this);
        CDC memDC;
        memDC.CreateCompatibleDC(&dc);
        BITMAP bitmap;
        _m_portraitBitmap.GetBitmap(&bitmap);
        TGDIObjectSelector<CBitmap> bitmapSelector(&memDC, &_m_portraitBitmap);
        if (portrait >= 0 && portrait < g_kNumPortraits) {
            CDC portraitDC;
            portraitDC.CreateCompatibleDC(&dc);
            CBitmap* pPortrait = _m_apPortrait.get()[portrait].get();
            BITMAP portraitBitmap;
            pPortrait->GetBitmap(&portraitBitmap);
            TGDIObjectSelector<CBitmap> portraitSelector(&portraitDC, pPortrait);
            int oldMode = memDC.SetStretchBltMode(COLORONCOLOR);
            memDC.StretchBlt(0, 0, bitmap.bmWidth, bitmap.bmHeight, &portraitDC, 0, 0, portraitBitmap.bmWidth,
                             portraitBitmap.bmHeight, SRCCOPY);
            memDC.SetStretchBltMode(oldMode);
        } else {
            CBrush brush(::GetSysColor(COLOR_3DFACE));
            TGDIObjectSelector<CBrush> brushSelector(&memDC, &brush);
            memDC.PatBlt(0, 0, bitmap.bmWidth, bitmap.bmHeight, PATCOPY);
        }
    }
    _m_portraitStatic.SetBitmap(_m_portraitBitmap);
}

VA(0x00450d88, 0xc6)
void TIdentifiedHeroPropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    THeroPropsGeneralPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_IDENTITY_COMBO, _m_identityCombo);
    DDX_Control(pDX, IDC_NAME_EDIT, _m_nameEdit);
    DDX_Control(pDX, IDC_CUSTOMIZE_NAME_CHECK, _m_customizeNameCheck);
    DDX_Control(pDX, IDC_PORTRAIT, _m_portraitStatic);
    DDX_Control(pDX, IDC_PORTRAIT_SPIN, _m_portraitSpin);
    DDX_Control(pDX, IDC_CUSTOMIZE_PORTRAIT_CHECK, _m_customizePortraitCheck);
    DDX_Control(pDX, IDC_DEFAULT_GENDER_RADIO, _m_defaultGenderRadio);
    DDX_Control(pDX, IDC_MALE_RADIO, _m_maleRadio);
    DDX_Control(pDX, IDC_FEMALE_RADIO, _m_femaleRadio);
    DDX_Radio(pDX, IDC_DEFAULT_GENDER_RADIO, _m_gender);
}

VA(0x00450e4e, 0x4c7)
BOOL TIdentifiedHeroPropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_IDENTITY_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kIdentityStaticStr);
    GetDlgItem(IDC_NAME_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kNameStaticStr);
    GetDlgItem(IDC_PORTRAIT_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kPortraitStaticStr);
    GetDlgItem(IDC_CUSTOMIZE_NAME_CHECK)->SetWindowText(SHeroPropsGeneralPageText::kCustomizeNameCheckStr);
    GetDlgItem(IDC_CUSTOMIZE_PORTRAIT_CHECK)->SetWindowText(SHeroPropsGeneralPageText::kCustomizePortraitCheckStr);
    GetDlgItem(IDC_GENDER_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kGenderStaticStr);
    GetDlgItem(IDC_MALE_RADIO)->SetWindowText(SHeroPropsGeneralPageText::kMaleRadioStr);
    GetDlgItem(IDC_FEMALE_RADIO)->SetWindowText(SHeroPropsGeneralPageText::kFemaleRadioStr);
    _m_bModified = false;
    _m_heroID = _m_pNewHero->getHeroID();
    _m_name = _m_pNewHero->getName().c_str();
    _m_portrait = _m_pNewHero->getPortrait();
    _m_gender = _m_pNewHero->getSex() + 1;
    EGameVersion version = _m_newMap.getVersion();
    THeroPropsGeneralPage::OnInitDialog();
    const THeroPrototype& prototype = _m_newMap.getHeroPrototype(_m_heroID);
    std::set<THeroID> heroes = _m_newMap.getAvailableHeroesInClass(THero::s_akTraits[_m_heroID].m_class);
    heroes.insert(_m_heroID);
    for (std::set<THeroID>::const_iterator it = heroes.begin(); it != heroes.end(); ++it)
        _m_identityCombo.SetItemData(_m_identityCombo.AddString(_m_newMap.getHeroPrototype(*it).getName().c_str()),
                                     *it);
    int index;
    for (index = 0; THeroID(_m_identityCombo.GetItemData(index)) != _m_heroID; index++)
        ;
    _m_identityCombo.SetCurSel(index);
    bool bCustomName = _m_pNewHero->getBCustomName();
    _m_bBlankName = _isspace(_m_name);
    _m_customizeNameCheck.SetCheck(bCustomName);
    _m_nameEdit.LimitText(THeroPrototype::s_kMaxNameLen);
    _m_nameEdit.EnableWindow(bCustomName);
    _m_nameEdit.SetWindowText(bCustomName ? _m_name : CString(prototype.getName().c_str()));
    CRect portraitRect;
    _m_portraitStatic.GetWindowRect(&portraitRect);
    ScreenToClient(&portraitRect);
    CRect spinRect;
    _m_portraitSpin.GetWindowRect(&spinRect);
    ScreenToClient(&spinRect);
    portraitRect.right = spinRect.left;
    portraitRect.bottom = spinRect.bottom;
    _m_portraitStatic.MoveWindow(&portraitRect, FALSE);
    _m_portraitStatic.GetClientRect(&portraitRect);
    {
        CClientDC dc(this);
        _m_portraitBitmap.CreateCompatibleBitmap(&dc, portraitRect.Width(), portraitRect.Height());
    }
    _setPortrait(_m_pNewHero->getBCustomPortrait() ? _m_portrait : prototype.getPortrait());
    _m_customizePortraitCheck.SetCheck(_m_pNewHero->getBCustomPortrait());
    _m_portraitSpin.SetRange(0, _getNumPortraits(version) - 1);
    UDACCEL accel;
    accel.nSec = 0;
    accel.nInc = 1;
    _m_portraitSpin.SetAccel(1, &accel);
    _m_portraitSpin.EnableWindow(_m_pNewHero->getBCustomPortrait());
    if (_m_pNewHero->getBCustomPortrait())
        _m_portraitSpin.SetPos(_m_portrait);
    _m_defaultGenderRadio.SetWindowText(
        TFormattedString(SHeroPropsGeneralPageText::kDefaultGenderRadioFmtStr, prototype.getSex() ? SHeroPropsGeneralPageText::kFemaleStr : SHeroPropsGeneralPageText::kMaleStr));
    if (version < GAME_VERSION_AB) {
        _m_maleRadio.EnableWindow(FALSE);
        _m_femaleRadio.EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x00451315, 0x247)
void TIdentifiedHeroPropsGeneralPage::OnOK()
{
    THeroPropsGeneralPage::OnOK();
    if (_m_heroID != _m_pNewHero->getHeroID())
        _m_newMap.setHeroID(_m_heroRef.getBSecondLayer(), _m_heroRef.getObjectID(), _m_heroID);
    bool bCustomName = _m_customizeNameCheck.GetCheck() != 0;
    _m_pNewHero->setBCustomName(bCustomName);
    if (bCustomName) {
        _m_nameEdit.GetWindowText(_m_name);
        _m_pNewHero->setName(std::string(_m_name));
    } else
        _m_pNewHero->setName(std::string());
    bool bCustomPortrait = _m_customizePortraitCheck.GetCheck() != 0;
    _m_pNewHero->setBCustomPortrait(bCustomPortrait);
    if (bCustomPortrait) {
        _adjustPortrait(_m_portrait);
        _m_pNewHero->setPortrait(_m_portrait);
    }
    _m_pNewHero->setSex(_m_gender - 1);
    _m_bModified = THeroPropsGeneralPage::wasModified() || _m_bModified
                   || _m_pOldHero->getHeroID() != _m_pNewHero->getHeroID()
                   || _m_pOldHero->getBCustomName() != _m_pNewHero->getBCustomName()
                   || _m_pNewHero->getName() != _m_pOldHero->getName()
                   || _m_pOldHero->getBCustomPortrait() != _m_pNewHero->getBCustomPortrait()
                   || _m_pNewHero->getPortrait() != _m_pOldHero->getPortrait()
                   || _m_pNewHero->getSex() != _m_pOldHero->getSex();
}

VA(0x0045155c, 0x45)
void TIdentifiedHeroPropsGeneralPage::OnSelChangeIdentityCombo()
{
    THeroID heroID = _m_identityCombo.GetItemData(_m_identityCombo.GetCurSel());
    if (heroID != _m_heroID) {
        _m_heroID = heroID;
        _onSetHeroID();
    }
}

VA(0x004515a1, 0x65)
void TIdentifiedHeroPropsGeneralPage::OnDeltaposPortraitSpin(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
    int portrait = pNMUpDown->iPos + pNMUpDown->iDelta;
    int numPortraits = _getNumPortraits(_m_newMap.getVersion());
    if (portrait >= 0 && portrait < numPortraits) {
        _setPortrait(portrait);
        _m_portrait = portrait;
        *pResult = 0;
    } else
        *pResult = 1;
}

VA(0x00451606, 0x60)
void TIdentifiedHeroPropsGeneralPage::OnChangeNameEdit()
{
    if (_m_nameEdit.IsWindowEnabled()) {
        _m_nameEdit.GetWindowText(_m_name);
        if (!_isspace(_m_name)) {
            if (_m_bBlankName) {
                _m_pParentSheet->onEnableOK();
                _m_bBlankName = false;
            }
        } else if (!_m_bBlankName) {
            _m_pParentSheet->onDisableOK();
            _m_bBlankName = true;
        }
    }
}

VA(0x00451666, 0x9e)
void TIdentifiedHeroPropsGeneralPage::OnCustomizeNameCheck()
{
    if (_m_customizeNameCheck.GetCheck()) {
        _m_nameEdit.SetWindowText(_m_name);
        _m_nameEdit.EnableWindow(TRUE);
        if (_m_bBlankName)
            _m_pParentSheet->onDisableOK();
        _m_nameEdit.SetFocus();
    } else {
        _m_nameEdit.EnableWindow(FALSE);
        _m_nameEdit.SetWindowText(_m_newMap.getHeroPrototype(_m_heroID).getName().c_str());
        if (_m_bBlankName)
            _m_pParentSheet->onEnableOK();
    }
}

VA(0x00451704, 0x7f)
void TIdentifiedHeroPropsGeneralPage::OnCustomizePortraitCheck()
{
    if (_m_customizePortraitCheck.GetCheck()) {
        _setPortrait(_m_portrait);
        _m_portraitSpin.EnableWindow(TRUE);
        _m_portraitSpin.SetPos(_m_portrait);
    } else {
        _setPortrait(_m_newMap.getHeroPrototype(_m_heroID).getPortrait());
        _m_portraitSpin.EnableWindow(FALSE);
    }
}

VA(0x00451783, 0xb1)
TNonRandomHeroPropsGeneralPage::TNonRandomHeroPropsGeneralPage(TIdentifiedHeroPropsGeneralPageParentSheet* pParentSheet,
                                                               const TGameMap& oldMap, TGameMap& newMap,
                                                               TMapObjectRef heroRef, bool bAnyAvailableOwner)
    : TIdentifiedHeroPropsGeneralPage(TNonRandomHeroPropsGeneralPage::IDD, pParentSheet, oldMap, newMap, heroRef),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_heroRef(heroRef),
      _m_bAnyAvailableOwner(bAnyAvailableOwner),
      _m_pOldHero(_getOldHero()),
      _m_pNewHero(_getNewHero()),
      _m_bModified(false)
{
}

VA_COMPGEN(0x00451834, 0x1c, SCALAR_DELETING_DTOR, TNonRandomHeroPropsGeneralPage)

VA_COMPGEN(0x00451850, 0x4a, IMPLICIT_DTOR, TNonRandomHeroPropsGeneralPage)

VA(0x0045189a, 0x75)
const TNonRandomHero* TNonRandomHeroPropsGeneralPage::_getOldHero() const
{
    const TGameMap::TLayer& layer = _m_oldMap.getLayer(_m_heroRef.getBSecondLayer());
    const TGameObject* pObject = layer.getPObject(_m_heroRef.getObjectID());
    const TNonRandomHero* pHero = dynamic_cast<const TNonRandomHero*>(pObject);
    if (pHero == NULL)
        pHero = dynamic_cast<const TNonRandomHero*>(dynamic_cast<const TTown*>(pObject)->getPVisitingHero());
    return pHero;
}

VA(0x0045190f, 0x75)
TNonRandomHero* TNonRandomHeroPropsGeneralPage::_getNewHero()
{
    TGameMap::TLayer& layer = _m_newMap.getLayer(_m_heroRef.getBSecondLayer());
    TGameObject* pObject = layer.getPObject(_m_heroRef.getObjectID());
    TNonRandomHero* pHero = dynamic_cast<TNonRandomHero*>(pObject);
    if (pHero == NULL)
        pHero = dynamic_cast<TNonRandomHero*>(dynamic_cast<TTown*>(pObject)->getPVisitingHero());
    return pHero;
}

VA(0x00451984, 0x37)
void TNonRandomHeroPropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    TIdentifiedHeroPropsGeneralPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PLAYER_COMBO, _m_ownerCombo);
    DDX_Text(pDX, IDC_CLASS_NAME_STATIC, _m_className);
}

VA(0x004519bb, 0x14e)
BOOL TNonRandomHeroPropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_CLASS_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kClassStaticStr);
    GetDlgItem(IDC_PLAYER_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kPlayerStaticStr);
    _m_bModified = false;
    _m_className = THero::s_akClassTraits[_m_pNewHero->getHeroClass()].m_name;
    TIdentifiedHeroPropsGeneralPage::OnInitDialog();
    TPlayerMask availableOwners;
    if (_m_bAnyAvailableOwner)
        availableOwners = _m_newMap.getAvailableHeroOwnersMask();
    availableOwners.set(_m_pNewHero->getOwner());
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        if (availableOwners[player])
            _m_ownerCombo.SetItemData(_m_ownerCombo.AddString(akPlayerTraits[player].m_pName), player);
    }
    int index;
    for (index = 0; TPlayer(_m_ownerCombo.GetItemData(index)) != _m_pNewHero->getOwner(); index++)
        ;
    _m_ownerCombo.SetCurSel(index);
    return TRUE;
}

VA(0x00451b09, 0x85)
void TNonRandomHeroPropsGeneralPage::OnOK()
{
    TIdentifiedHeroPropsGeneralPage::OnOK();
    TPlayer owner = TPlayer(_m_ownerCombo.GetItemData(_m_ownerCombo.GetCurSel()));
    if (owner != _m_pNewHero->getOwner())
        _m_newMap.setHeroOwner(_m_heroRef.getBSecondLayer(), _m_heroRef.getObjectID(), owner);
    _m_bModified = TIdentifiedHeroPropsGeneralPage::wasModified() || _m_bModified
                   || _m_pNewHero->getOwner() != _m_pOldHero->getOwner();
}

VA(0x00451b8e, 0x6)
BEGIN_MESSAGE_MAP(TPrisonPropsGeneralPage, TIdentifiedHeroPropsGeneralPage)
    ON_CBN_SELCHANGE(IDC_CLASS_COMBO, OnSelChangeClassCombo)
END_MESSAGE_MAP()

VA(0x00451b94, 0xa6)
TPrisonPropsGeneralPage::TPrisonPropsGeneralPage(TIdentifiedHeroPropsGeneralPageParentSheet* pParentSheet,
                                                 const TGameMap& oldMap, TGameMap& newMap, TMapObjectRef prisonRef)
    : TIdentifiedHeroPropsGeneralPage(TPrisonPropsGeneralPage::IDD, pParentSheet, oldMap, newMap, prisonRef),
      _m_pParentSheet(pParentSheet),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_prisonRef(prisonRef),
      _m_pOldPrison(_getOldPrison()),
      _m_pNewPrison(_getNewPrison()),
      _m_bModified(false)
{
}

VA_COMPGEN(0x00451c3a, 0x1c, SCALAR_DELETING_DTOR, TPrisonPropsGeneralPage)

VA_COMPGEN(0x00451c56, 0x38, IMPLICIT_DTOR, TPrisonPropsGeneralPage)

VA(0x00451c8e, 0x42)
const TPrison* TPrisonPropsGeneralPage::_getOldPrison() const
{
    const TGameMap::TLayer& layer = _m_oldMap.getLayer(_m_prisonRef.getBSecondLayer());
    return dynamic_cast<const TPrison*>(layer.getPObject(_m_prisonRef.getObjectID()));
}

VA(0x00451cd0, 0x42)
TPrison* TPrisonPropsGeneralPage::_getNewPrison()
{
    TGameMap::TLayer& layer = _m_newMap.getLayer(_m_prisonRef.getBSecondLayer());
    return dynamic_cast<TPrison*>(layer.getPObject(_m_prisonRef.getObjectID()));
}

VA(0x00451d12, 0x25)
void TPrisonPropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    TIdentifiedHeroPropsGeneralPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CLASS_COMBO, _m_classCombo);
}

VA(0x00451d37, 0x167)
BOOL TPrisonPropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_CLASS_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kClassStaticStr);
    _m_bModified = false;
    _m_heroClass = THero::s_akTraits[_m_pNewPrison->getHeroID()].m_class;
    TIdentifiedHeroPropsGeneralPage::OnInitDialog();
    std::set<THeroClass> heroClasses;
    for (THeroClass heroClass = THeroClass(0); heroClass < kNumHeroClasses; heroClass = THeroClass(heroClass + 1)) {
        if (_m_newMap.getAvailableHeroesInClass(heroClass).size() > 0)
            heroClasses.insert(heroClass);
    }
    heroClasses.insert(_m_heroClass);
    for (std::set<THeroClass>::const_iterator it = heroClasses.begin(); it != heroClasses.end(); ++it)
        _m_classCombo.SetItemData(_m_classCombo.AddString(THero::s_akClassTraits[*it].m_name), *it);
    int index;
    for (index = 0; THeroClass(_m_classCombo.GetItemData(index)) != _m_heroClass; index++)
        ;
    _m_classCombo.SetCurSel(index);
    return TRUE;
}

VA(0x00451ea3, 0x25)
void TPrisonPropsGeneralPage::OnOK()
{
    TIdentifiedHeroPropsGeneralPage::OnOK();
    _m_bModified = TIdentifiedHeroPropsGeneralPage::wasModified() || _m_bModified;
}

VA(0x00451ec8, 0xbf)
void TPrisonPropsGeneralPage::OnSelChangeClassCombo()
{
    THeroClass heroClass = THeroClass(_m_classCombo.GetItemData(_m_classCombo.GetCurSel()));
    if (heroClass != _m_heroClass) {
        _m_heroClass = heroClass;
        std::set<THeroID> heroes = _m_newMap.getAvailableHeroesInClass(heroClass);
        if (_m_heroClass == _m_pNewPrison->getHeroClass())
            heroes.insert(_m_pNewPrison->getHeroID());
        setHeroID(*heroes.begin());
    }
}
