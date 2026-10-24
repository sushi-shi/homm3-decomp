// HeroPrototypePropsGeneralPage.cpp - the general page of the map
// specifications' hero customization sheet (h3maped 0x455aee..0x456b9c;
// GOG only). It renders the 163 portraits once and stretches the shown one
// into a bitmap sized to the portrait control; a player may hire the hero
// only when on the map.
#include "editor/stdafx.h"

#include <ctype.h>
#include <stdio.h>

#include <string>

#include "bitmap816.h"
#include "exceptions.h"
#include "herotraits.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "va.h"
#include "editor/Digits.h"
#include "editor/FormattedString.h"
#include "editor/GDIObjectSelector.h"
#include "editor/HeroPrototypePropsGeneralPage.h"
#include "editor/MapEditorText.h"
#include "editor/StringUtil.h"

namespace {

// The experience of each of the first twelve levels, the combo's choices.
DATA(0x00538370)
const int akLevelExperience[] = { 0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500 };

DATA(0x00538240)
const int akPlayerCheckIDs[kNumPlayers] = {
    IDC_EVENT_PLAYER1_CHECK, IDC_EVENT_PLAYER2_CHECK, IDC_EVENT_PLAYER3_CHECK, IDC_EVENT_PLAYER4_CHECK,
    IDC_EVENT_PLAYER5_CHECK, IDC_EVENT_PLAYER6_CHECK, IDC_EVENT_PLAYER7_CHECK, IDC_EVENT_PLAYER8_CHECK
};

}

static const unsigned int g_kNumPortraits = sizeof(akHeroTraits) / sizeof(akHeroTraits[0]);

VA(0x00455b0a, 0x8)
void THeroPrototypePropsGeneralPage::OnPlayer1Check()
{
    _onPlayerCheck(TPlayer(0));
}

VA(0x00455b12, 0x8)
void THeroPrototypePropsGeneralPage::OnPlayer2Check()
{
    _onPlayerCheck(TPlayer(1));
}

VA(0x00455b1a, 0x8)
void THeroPrototypePropsGeneralPage::OnPlayer3Check()
{
    _onPlayerCheck(TPlayer(2));
}

VA(0x00455b22, 0x8)
void THeroPrototypePropsGeneralPage::OnPlayer4Check()
{
    _onPlayerCheck(TPlayer(3));
}

VA(0x00455b2a, 0x8)
void THeroPrototypePropsGeneralPage::OnPlayer5Check()
{
    _onPlayerCheck(TPlayer(4));
}

VA(0x00455b32, 0x8)
void THeroPrototypePropsGeneralPage::OnPlayer6Check()
{
    _onPlayerCheck(TPlayer(5));
}

VA(0x00455b3a, 0x8)
void THeroPrototypePropsGeneralPage::OnPlayer7Check()
{
    _onPlayerCheck(TPlayer(6));
}

VA(0x00455b42, 0x8)
void THeroPrototypePropsGeneralPage::OnPlayer8Check()
{
    _onPlayerCheck(TPlayer(7));
}

VA(0x00455d02, 0x2a)
bool _isspace(const CString& text)
{
    unsigned int i = text.GetLength();
    while (i > 0) {
        if (!isspace(text.GetAt(--i)))
            return false;
    }
    return true;
}

VA(0x00455d2c, 0x2f3)
THeroPrototypePropsGeneralPage::THeroPrototypePropsGeneralPage(TParentSheet* pParentSheet, THeroID heroID,
                                                               const THeroPrototype& oldPrototype,
                                                               THeroPrototype* pNewPrototype,
                                                               const TPlayerMask& playersPresent)
    : CPropertyPage(THeroPrototypePropsGeneralPage::IDD),
      _m_pParentSheet(pParentSheet),
      _m_heroID(heroID),
      _m_oldPrototype(oldPrototype),
      _m_pNewPrototype(pNewPrototype),
      _m_playersPresent(playersPresent),
      _m_bModified(false)
{
    _m_className = _T("");
    _m_gender = -1;
    m_psp.pszTitle = m_strCaption = SHeroPrototypePropsGeneralPageText::kCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
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

VA_COMPGEN(0x0045601f, 0x1c, SCALAR_DELETING_DTOR, THeroPrototypePropsGeneralPage)

VA(0x0045603b, 0xbe)
THeroPrototypePropsGeneralPage::~THeroPrototypePropsGeneralPage()
{
}

VA(0x004560f9, 0x18b)
void THeroPrototypePropsGeneralPage::_setPortrait()
{
    _m_portraitStatic.SetBitmap(NULL);
    {
        CClientDC dc(this);
        CDC memDC;
        memDC.CreateCompatibleDC(&dc);
        BITMAP bitmap;
        _m_portraitBitmap.GetBitmap(&bitmap);
        TGDIObjectSelector<CBitmap> bitmapSelector(&memDC, &_m_portraitBitmap);
        CDC portraitDC;
        portraitDC.CreateCompatibleDC(&dc);
        CBitmap* pPortrait = _m_apPortrait.get()[_m_portrait].get();
        BITMAP portraitBitmap;
        pPortrait->GetBitmap(&portraitBitmap);
        TGDIObjectSelector<CBitmap> portraitSelector(&portraitDC, pPortrait);
        int oldMode = memDC.SetStretchBltMode(COLORONCOLOR);
        memDC.StretchBlt(0, 0, bitmap.bmWidth, bitmap.bmHeight, &portraitDC, 0, 0, portraitBitmap.bmWidth,
                         portraitBitmap.bmHeight, SRCCOPY);
        memDC.SetStretchBltMode(oldMode);
    }
    _m_portraitStatic.SetBitmap(_m_portraitBitmap);
}

VA(0x00456284, 0x9d)
void THeroPrototypePropsGeneralPage::_onPlayerCheck(TPlayer player)
{
    if (_m_aPlayerCheck[player].GetCheck()) {
        if (!_m_availability[player]) {
            if (_m_numAvailable++ == 0)
                _m_pParentSheet->onEnableOK();
            _m_availability.set(player, true);
        }
    } else if (_m_availability[player]) {
        if (--_m_numAvailable == 0)
            _m_pParentSheet->onDisableOK();
        _m_availability.set(player, false);
    }
}

VA(0x00456321, 0x9d)
void THeroPrototypePropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_PORTRAIT_SPIN, _m_portraitSpin);
    DDX_Control(pDX, IDC_PORTRAIT, _m_portraitStatic);
    DDX_Control(pDX, IDC_NAME_EDIT, _m_nameEdit);
    DDX_Control(pDX, IDC_EXPERIENCE_COMBO, _m_experienceCombo);
    DDX_Text(pDX, IDC_CLASS_NAME_STATIC, _m_className);
    DDX_Radio(pDX, IDC_MALE_RADIO, _m_gender);
    for (int player = 0; player < kNumPlayers; player++)
        DDX_Control(pDX, akPlayerCheckIDs[player], _m_aPlayerCheck[player]);
}

VA(0x004563be, 0x6)
BEGIN_MESSAGE_MAP(THeroPrototypePropsGeneralPage, CPropertyPage)
    ON_EN_CHANGE(IDC_NAME_EDIT, OnChangeNameEdit)
    ON_NOTIFY(UDN_DELTAPOS, IDC_PORTRAIT_SPIN, OnDeltaposPortraitSpin)
    ON_BN_CLICKED(IDC_EVENT_PLAYER1_CHECK, OnPlayer1Check)
    ON_BN_CLICKED(IDC_EVENT_PLAYER2_CHECK, OnPlayer2Check)
    ON_BN_CLICKED(IDC_EVENT_PLAYER3_CHECK, OnPlayer3Check)
    ON_BN_CLICKED(IDC_EVENT_PLAYER4_CHECK, OnPlayer4Check)
    ON_BN_CLICKED(IDC_EVENT_PLAYER5_CHECK, OnPlayer5Check)
    ON_BN_CLICKED(IDC_EVENT_PLAYER6_CHECK, OnPlayer6Check)
    ON_BN_CLICKED(IDC_EVENT_PLAYER7_CHECK, OnPlayer7Check)
    ON_BN_CLICKED(IDC_EVENT_PLAYER8_CHECK, OnPlayer8Check)
END_MESSAGE_MAP()

// Keep the portrait geometry inside bitmap creation, as in the sibling
// identified-hero page; retail reuses these homes for the later player scan.
VA(0x004563c4, 0x591)
BOOL THeroPrototypePropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_CLASS_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kClassStaticStr);
    GetDlgItem(IDC_NAME_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kNameStaticStr);
    GetDlgItem(IDC_EXPERIENCE_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kExperienceStaticStr);
    GetDlgItem(IDC_PORTRAIT_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kPortraitStaticStr);
    GetDlgItem(IDC_GENDER_STATIC)->SetWindowText(SHeroPropsGeneralPageText::kGenderStaticStr);
    GetDlgItem(IDC_MALE_RADIO)->SetWindowText(SHeroPropsGeneralPageText::kMaleRadioStr);
    GetDlgItem(IDC_FEMALE_RADIO)->SetWindowText(SHeroPropsGeneralPageText::kFemaleRadioStr);
    GetDlgItem(IDC_MAY_BE_HIRED_BY_STATIC)->SetWindowText(SHeroPrototypePropsGeneralPageText::kMayBeHiredByStaticStr);
    GetDlgItem(IDC_EVENT_PLAYER1_CHECK)->SetWindowText(TFormattedString(SHeroPrototypePropsGeneralPageText::kPlayerCheckFmtStr, 1));
    GetDlgItem(IDC_EVENT_PLAYER2_CHECK)->SetWindowText(TFormattedString(SHeroPrototypePropsGeneralPageText::kPlayerCheckFmtStr, 2));
    GetDlgItem(IDC_EVENT_PLAYER3_CHECK)->SetWindowText(TFormattedString(SHeroPrototypePropsGeneralPageText::kPlayerCheckFmtStr, 3));
    GetDlgItem(IDC_EVENT_PLAYER4_CHECK)->SetWindowText(TFormattedString(SHeroPrototypePropsGeneralPageText::kPlayerCheckFmtStr, 4));
    GetDlgItem(IDC_EVENT_PLAYER5_CHECK)->SetWindowText(TFormattedString(SHeroPrototypePropsGeneralPageText::kPlayerCheckFmtStr, 5));
    GetDlgItem(IDC_EVENT_PLAYER6_CHECK)->SetWindowText(TFormattedString(SHeroPrototypePropsGeneralPageText::kPlayerCheckFmtStr, 6));
    GetDlgItem(IDC_EVENT_PLAYER7_CHECK)->SetWindowText(TFormattedString(SHeroPrototypePropsGeneralPageText::kPlayerCheckFmtStr, 7));
    GetDlgItem(IDC_EVENT_PLAYER8_CHECK)->SetWindowText(TFormattedString(SHeroPrototypePropsGeneralPageText::kPlayerCheckFmtStr, 8));
    _m_bModified = false;
    _m_className = THero::s_akClassTraits[THero::s_akTraits[_m_heroID].m_class].m_name;
    _m_portrait = _m_pNewPrototype->getPortrait();
    _m_gender = _m_pNewPrototype->getSex();
    _m_availability = _m_pNewPrototype->getAvailability();
    CPropertyPage::OnInitDialog();
    _m_bNameBlank = false;
    _m_nameEdit.LimitText(THeroPrototype::s_kMaxNameLen);
    _m_nameEdit.SetWindowText(_m_pNewPrototype->getName().c_str());
    for (const int* pExperience = akLevelExperience;
         pExperience < akLevelExperience + sizeof(akLevelExperience) / sizeof(akLevelExperience[0]);
         pExperience++)
        _m_experienceCombo.AddString(TFormattedString("%d", *pExperience));
    _m_experienceCombo.GetWindow(GW_CHILD)->ModifyStyle(0, ES_NUMBER);
    _m_experienceCombo.LimitText(TDigits<s_kMaxExperience>::getDigits());
    _m_experienceCombo.SetWindowText(TFormattedString("%d", _m_pNewPrototype->getExperience()));
    {
        CRect rect;
        _m_portraitStatic.GetWindowRect(&rect);
        ScreenToClient(&rect);
        CRect spinRect;
        _m_portraitSpin.GetWindowRect(&spinRect);
        ScreenToClient(&spinRect);
        rect.right = spinRect.left;
        rect.bottom = spinRect.bottom;
        _m_portraitStatic.MoveWindow(&rect, FALSE);
        _m_portraitStatic.GetClientRect(&rect);
        CClientDC dc(this);
        _m_portraitBitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    }
    _setPortrait();
    _m_portraitSpin.SetRange(0, g_kNumPortraits - 1);
    UDACCEL accel = { 0, 1 };
    _m_portraitSpin.SetAccel(1, &accel);
    _m_portraitSpin.SetPos(_m_portrait);
    _m_numAvailable = 0;
    for (int player = 0; player < kNumPlayers; player++) {
        if (_m_playersPresent[player]) {
            if (_m_availability[player]) {
                _m_numAvailable++;
                _m_aPlayerCheck[player].SetCheck(1);
            }
        } else
            _m_aPlayerCheck[player].EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x00456955, 0x194)
void THeroPrototypePropsGeneralPage::OnOK()
{
    CPropertyPage::OnOK();
    {
        CString name;
        _m_nameEdit.GetWindowText(name);
        if ((LPCTSTR)name != _m_pNewPrototype->getName()) {
            _m_bModified = true;
            _m_pNewPrototype->setName(std::string(name));
        }
    }
    {
        int experience = 0;
        CString text;
        _m_experienceCombo.GetWindowText(text);
        sscanf(text, "%d", &experience);
        if (experience != _m_pNewPrototype->getExperience()) {
            _m_bModified = true;
            _m_pNewPrototype->setExperience(experience);
        }
    }
    if (_m_portrait != _m_pNewPrototype->getPortrait()) {
        _m_bModified = true;
        _m_pNewPrototype->setPortrait(_m_portrait);
    }
    if (_m_gender != _m_pNewPrototype->getSex()) {
        _m_bModified = true;
        _m_pNewPrototype->setSex(_m_gender);
    }
    if (_m_availability != _m_pNewPrototype->getAvailability()) {
        _m_bModified = true;
        _m_pNewPrototype->setAvailability(TPlayerMask(_m_availability));
    }
}

VA(0x00456ae9, 0x7f)
void THeroPrototypePropsGeneralPage::OnChangeNameEdit()
{
    CString name;
    _m_nameEdit.GetWindowText(name);
    if (_isspace(name)) {
        if (!_m_bNameBlank) {
            _m_pParentSheet->onDisableOK();
            _m_bNameBlank = true;
        }
    } else if (_m_bNameBlank) {
        _m_pParentSheet->onEnableOK();
        _m_bNameBlank = false;
    }
}

VA(0x00456b68, 0x34)
void THeroPrototypePropsGeneralPage::OnDeltaposPortraitSpin(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
    int portrait = pNMUpDown->iPos + pNMUpDown->iDelta;
    if (portrait >= 0 && portrait < g_kNumPortraits) {
        _m_portrait = portrait;
        _setPortrait();
        *pResult = 0;
    } else
        *pResult = 1;
}
