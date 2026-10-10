// TownPropsGeneralPage.cpp - the general page of the town property sheet
// (h3maped 0x4c5068..0x4c64a5; Loki h3maped object 93). The visiting hero
// is added to, edited in and removed from the map's copy directly; a town
// whose owner cannot keep a hero loses its visitor once the user agrees.
// A random town takes its alignment from the Shadow of Death map format on.
#include "editor/stdafx.h"

#include <memory>
#include <string>

#include "va.h"
#include "editor/FormattedString.h"
#include "editor/GameMap.h"
#include "editor/Hero.h"
#include "editor/HeroPropsSheet.h"
#include "editor/MapEditorText.h"
#include "editor/Player.h"
#include "editor/SelectHeroClassDlg.h"
#include "editor/StringUtil.h"
#include "editor/Town.h"
#include "editor/TownPropsGeneralPage.h"

VA(0x004c523c, 0x200)
TTownPropsGeneralPage::TTownPropsGeneralPage(TTownPropsGeneralPageParentSheet* pParentSheet, const TGameMap& oldMap,
                                             TGameMap& newMap, bool bSecondLayer, unsigned int objID)
    : CPropertyPage(TTownPropsGeneralPage::IDD),
      _m_pParentSheet(pParentSheet),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_townRef(bSecondLayer, objID),
      _m_pOldTown(_getOldTown()),
      _m_bIsMainTown(false),
      _m_bModified(false),
      _m_bVisitingHeroModified(false)
{
    _m_townType = _T("");
    _m_visitingHeroClass = _T("");
    _m_visitingHeroName = _T("");
    _m_bCustomName = FALSE;
    _m_name = _T("");
    const TPlayerInfo& ownerInfo = _m_oldMap.getPlayers()[_m_pOldTown->getOwner()];
    _m_bIsMainTown = _m_pOldTown->getOwner() != ePlayerNone && ownerInfo.getMainTownRef() == _m_townRef
                     && ownerInfo.getBHasMainTown();
    m_psp.pszTitle = m_strCaption = kGeneralPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x004c543c, 0x1c, SCALAR_DELETING_DTOR, TTownPropsGeneralPage)

VA(0x004c5458, 0xe6)
TTownPropsGeneralPage::~TTownPropsGeneralPage()
{
}

VA(0x004c553e, 0x166)
THero* TTownPropsGeneralPage::_createHero()
{
    TTown* pNewTown = _getNewTown();
    THeroClassMask availableClasses(0);
    for (unsigned int heroClass = 0; heroClass < kNumHeroClasses + 1; heroClass++)
        availableClasses[heroClass] =
            _m_newMap.canCreate(THero::s_akClassTraits[heroClass].m_objType, pNewTown->getOwner());
    if (availableClasses.none()) {
        MessageBeep((UINT)-1);
        MessageBox(kNoMoreHeroesStr);
        return NULL;
    }
    TSelectHeroClassDlg selectHeroClassDlg(this, availableClasses, _m_newMap.getVersion());
    if (selectHeroClassDlg.DoModal() != IDOK)
        return NULL;
    std::auto_ptr<TGameObject> pObject = _m_newMap.createObject(
        THero::s_akClassTraits[selectHeroClassDlg.getHeroClass()].m_objType, pNewTown->getOwner());
    if (!pObject.get())
        throw TAllocationFailure();
    return dynamic_cast<THero*>(pObject.release());
}

VA(0x004c56a4, 0x42)
const TTown* TTownPropsGeneralPage::_getOldTown() const
{
    const TGameMap::TLayer& layer = _m_oldMap.getLayer(_m_townRef.getBSecondLayer());
    return dynamic_cast<const TTown*>(layer.getPObject(_m_townRef.getObjectID()));
}

VA(0x004c56e6, 0x42)
TTown* TTownPropsGeneralPage::_getNewTown()
{
    TGameMap::TLayer& layer = _m_newMap.getLayer(_m_townRef.getBSecondLayer());
    return dynamic_cast<TTown*>(layer.getPObject(_m_townRef.getObjectID()));
}

VA(0x004c5728, 0xe4)
void TTownPropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_ALIGNMENT_STATIC, _m_alignmentStatic);
    DDX_Control(pDX, IDC_ALIGNMENT_COMBO, _m_alignmentCombo);
    DDX_Control(pDX, IDC_PLAYER_COMBO, _m_playerCombo);
    DDX_Control(pDX, IDC_NAME_EDIT, _m_nameEdit);
    DDX_Control(pDX, IDC_REMOVE_HERO_BUTTON, _m_removeHeroButton);
    DDX_Control(pDX, IDC_EDIT_HERO_BUTTON, _m_editHeroButton);
    DDX_Control(pDX, IDC_ADD_HERO_BUTTON, _m_addHeroButton);
    DDX_Text(pDX, IDC_TYPE_STATIC, _m_townType);
    DDX_Text(pDX, IDC_VISITING_HERO_CLASS_STATIC, _m_visitingHeroClass);
    DDX_Text(pDX, IDC_VISITING_HERO_NAME_STATIC, _m_visitingHeroName);
    DDX_Check(pDX, IDC_CUSTOMIZE_CHECK, _m_bCustomName);
    DDX_Text(pDX, IDC_NAME_EDIT, _m_name);
}

VA(0x004c580c, 0x6)
BEGIN_MESSAGE_MAP(TTownPropsGeneralPage, CPropertyPage)
    ON_BN_CLICKED(IDC_ADD_HERO_BUTTON, OnAddHeroButton)
    ON_BN_CLICKED(IDC_EDIT_HERO_BUTTON, OnEditHeroButton)
    ON_BN_CLICKED(IDC_REMOVE_HERO_BUTTON, OnRemoveHeroButton)
    ON_EN_CHANGE(IDC_NAME_EDIT, OnChangeNameEdit)
    ON_BN_CLICKED(IDC_CUSTOMIZE_CHECK, OnCustomizeCheck)
    ON_CBN_SELCHANGE(IDC_PLAYER_COMBO, OnSelChangePlayerCombo)
END_MESSAGE_MAP()

VA(0x004c5812, 0x547)
BOOL TTownPropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_TOWN_TYPE_STATIC)->SetWindowText(STownPropsGeneralPageText::kTownTypeStaticStr);
    GetDlgItem(IDC_PLAYER_STATIC)->SetWindowText(STownPropsGeneralPageText::kPlayerStaticStr);
    GetDlgItem(IDC_TOWN_NAME_STATIC)->SetWindowText(STownPropsGeneralPageText::kTownNameStaticStr);
    GetDlgItem(IDC_CUSTOMIZE_CHECK)->SetWindowText(STownPropsGeneralPageText::kCustomizeCheckStr);
    GetDlgItem(IDC_VISITING_HERO_STATIC)->SetWindowText(STownPropsGeneralPageText::kVisitingHeroStaticStr);
    GetDlgItem(IDC_HERO_NAME_STATIC)->SetWindowText(STownPropsGeneralPageText::kHeroNameStaticStr);
    GetDlgItem(IDC_HERO_CLASS_STATIC)->SetWindowText(STownPropsGeneralPageText::kHeroClassStaticStr);
    GetDlgItem(IDC_ADD_HERO_BUTTON)->SetWindowText(STownPropsGeneralPageText::kAddButtonStr);
    GetDlgItem(IDC_EDIT_HERO_BUTTON)->SetWindowText(STownPropsGeneralPageText::kEditButtonStr);
    GetDlgItem(IDC_REMOVE_HERO_BUTTON)->SetWindowText(STownPropsGeneralPageText::kRemoveButtonStr);
    GetDlgItem(IDC_ALIGNMENT_STATIC)->SetWindowText(STownPropsGeneralPageText::kAlignmentStaticStr);
    _m_bModified = false;
    _m_bVisitingHeroModified = false;
    TTown* pNewTown = _getNewTown();
    _m_townType = TTown::s_akTypeTraits[pNewTown->getTownType()].m_pName;
    _m_bCustomName = pNewTown->getBCustomName();
    _m_customName = pNewTown->getName().c_str();
    _m_name = _m_bCustomName ? _m_customName : CString(kUnknownStr);
    const THero* pVisitingHero = pNewTown->getPVisitingHero();
    if (pVisitingHero) {
        const TNonRandomHero* pHero = dynamic_cast<const TNonRandomHero*>(pVisitingHero);
        if (pHero) {
            _m_visitingHeroClass = THero::s_akClassTraits[pHero->getHeroClass()].m_name;
            _m_visitingHeroName = pHero->getBCustomName()
                                      ? pHero->getName().c_str()
                                      : _m_newMap.getHeroPrototype(pHero->getHeroID()).getName().c_str();
        } else {
            _m_visitingHeroClass = kRandomStr;
            _m_visitingHeroName = pVisitingHero->getBCustomName() ? pVisitingHero->getName().c_str() : kUnknownStr;
        }
    } else {
        _m_visitingHeroClass = _T("");
        _m_visitingHeroName = _T("");
    }
    CPropertyPage::OnInitDialog();
    _m_playerCombo.SetItemData(_m_playerCombo.AddString(kSelNoneStr), DWORD(ePlayerNone));
    for (unsigned int player = 0; player < kNumPlayers; player++)
        _m_playerCombo.SetItemData(_m_playerCombo.AddString(akPlayerTraits[player].m_pName), player);
    TPlayer owner = pNewTown->getOwner();
    int index;
    for (index = 0; TPlayer(_m_playerCombo.GetItemData(index)) != owner; index++)
        ;
    _m_playerCombo.SetCurSel(index);
    _m_nameEdit.EnableWindow(_m_bCustomName);
    _m_nameEdit.LimitText(_m_newMap.getVersion() >= GAME_VERSION_AB ? 14 : 12);
    _m_bBlankName = _isspace(_m_customName);
    if (_m_bCustomName && _m_bBlankName)
        _m_pParentSheet->onDisableOK();
    _m_addHeroButton.EnableWindow(!_m_bIsMainTown && pVisitingHero == NULL && owner != ePlayerNone
                                  && _m_newMap.getAvailableHeroOwnersMask()[owner]);
    _m_editHeroButton.EnableWindow(pVisitingHero != NULL);
    _m_removeHeroButton.EnableWindow(pVisitingHero != NULL);
    if (pNewTown->getTownType() >= kNumTownTypes) {
        _m_alignmentCombo.SetItemData(_m_alignmentCombo.AddString(STownPropsGeneralPageText::kSameAsOwnerOrRandomStr),
                                      DWORD(ePlayerNone));
        for (int player = 0; player < kNumPlayers; player++) {
            int index = _m_alignmentCombo.AddString(
                TFormattedString(STownPropsGeneralPageText::kSameAsTownFmtStr, akPlayerTraits[player].m_pName));
            _m_alignmentCombo.SetItemData(index, player);
        }
        TPlayer alignment = pNewTown->getAlignment();
        int alignmentIndex;
        for (alignmentIndex = 0; TPlayer(_m_alignmentCombo.GetItemData(alignmentIndex)) != alignment;
             alignmentIndex++)
            ;
        _m_alignmentCombo.SetCurSel(alignmentIndex);
        if (_m_newMap.getVersion() < GAME_VERSION_SOD) {
            _m_alignmentStatic.EnableWindow(FALSE);
            _m_alignmentCombo.EnableWindow(FALSE);
        }
    } else {
        _m_alignmentStatic.ShowWindow(SW_HIDE);
        _m_alignmentCombo.ShowWindow(SW_HIDE);
    }
    return TRUE;
}

VA(0x004c5d59, 0x122)
void TTownPropsGeneralPage::OnOK()
{
    CPropertyPage::OnOK();
    TTown* pNewTown = _getNewTown();
    pNewTown->setBCustomName(_m_bCustomName != FALSE);
    pNewTown->setName(std::string(_m_customName));
    if (pNewTown->getTownType() >= kNumTownTypes)
        pNewTown->setAlignment(TPlayer(_m_alignmentCombo.GetItemData(_m_alignmentCombo.GetCurSel())));
    _m_bModified = _m_bModified || _m_bVisitingHeroModified || pNewTown->getOwner() != _m_pOldTown->getOwner()
                   || _m_pOldTown->getBCustomName() != pNewTown->getBCustomName()
                   || pNewTown->getName() != _m_pOldTown->getName()
                   || pNewTown->getAlignment() != _m_pOldTown->getAlignment();
}

VA(0x004c5e7b, 0x164)
void TTownPropsGeneralPage::OnAddHeroButton()
{
    TTown* pNewTown = _getNewTown();
    std::auto_ptr<THero> pHero(_createHero());
    if (pHero.get()) {
        _m_bVisitingHeroModified = true;
        _m_newMap.setVisitingHero(pHero.get(), _m_townRef.getBSecondLayer(), _m_townRef.getObjectID());
        const THero* pVisitingHero = pNewTown->getPVisitingHero();
        const TNonRandomHero* pNonRandomHero = dynamic_cast<const TNonRandomHero*>(pVisitingHero);
        if (pNonRandomHero) {
            _m_visitingHeroClass = THero::s_akClassTraits[pNonRandomHero->getHeroClass()].m_name;
            _m_visitingHeroName = pNonRandomHero->getBCustomName()
                                      ? pNonRandomHero->getName().c_str()
                                      : _m_newMap.getHeroPrototype(pNonRandomHero->getHeroID()).getName().c_str();
        } else {
            _m_visitingHeroClass = kRandomStr;
            _m_visitingHeroName = pVisitingHero->getBCustomName() ? pVisitingHero->getName().c_str() : kUnknownStr;
        }
        UpdateData(FALSE);
        _m_addHeroButton.EnableWindow(FALSE);
        _m_editHeroButton.EnableWindow(TRUE);
        _m_removeHeroButton.EnableWindow(TRUE);
    }
}

VA(0x004c5fdf, 0x194)
void TTownPropsGeneralPage::OnEditHeroButton()
{
    const THero* pVisitingHero = _getNewTown()->getPVisitingHero();
    const THero* pHero = dynamic_cast<const TNonRandomHero*>(pVisitingHero);
    if (pHero) {
        TNonRandomHeroPropsSheet sheet(this, &_m_newMap, _m_townRef.getBSecondLayer(), _m_townRef.getObjectID(), false);
        sheet.DoModal();
        if (sheet.wasModified()) {
            _m_bVisitingHeroModified = true;
            pHero = _getNewTown()->getPVisitingHero();
        }
        _m_visitingHeroName = pHero->getBCustomName()
                                  ? pHero->getName().c_str()
                                  : _m_newMap.getHeroPrototype(pHero->getHeroID()).getName().c_str();
    } else {
        pHero = pVisitingHero;
        TRandomHeroPropsSheet sheet(this, &_m_newMap, _m_townRef.getBSecondLayer(), _m_townRef.getObjectID(), false);
        sheet.DoModal();
        if (sheet.wasModified()) {
            _m_bVisitingHeroModified = true;
            pHero = _getNewTown()->getPVisitingHero();
        }
        _m_visitingHeroName = pHero->getBCustomName() ? pHero->getName().c_str() : kUnknownStr;
    }
    UpdateData(FALSE);
}

VA(0x004c6173, 0x72)
void TTownPropsGeneralPage::OnRemoveHeroButton()
{
    _m_newMap.removeVisitingHero(_m_townRef.getBSecondLayer(), _m_townRef.getObjectID());
    _m_bVisitingHeroModified = true;
    _m_visitingHeroClass = _T("");
    _m_visitingHeroName = _T("");
    UpdateData(FALSE);
    _m_addHeroButton.EnableWindow(TRUE);
    _m_editHeroButton.EnableWindow(FALSE);
    _m_removeHeroButton.EnableWindow(FALSE);
}

VA(0x004c61e5, 0x5a)
void TTownPropsGeneralPage::OnChangeNameEdit()
{
    UpdateData(TRUE);
    _m_customName = _m_name;
    if (_isspace(_m_customName)) {
        if (!_m_bBlankName) {
            _m_bBlankName = true;
            _m_pParentSheet->onDisableOK();
        }
    } else if (_m_bBlankName) {
        _m_bBlankName = false;
        _m_pParentSheet->onEnableOK();
    }
}

VA(0x004c623f, 0x97)
void TTownPropsGeneralPage::OnCustomizeCheck()
{
    UpdateData(TRUE);
    if (_m_bCustomName) {
        _m_nameEdit.EnableWindow(TRUE);
        _m_name = _m_customName;
        UpdateData(FALSE);
        if (_m_bBlankName)
            _m_pParentSheet->onDisableOK();
        _m_nameEdit.SetFocus();
    } else {
        if (_m_bBlankName)
            _m_pParentSheet->onEnableOK();
        _m_name = kUnknownStr;
        UpdateData(FALSE);
        _m_nameEdit.EnableWindow(FALSE);
    }
}

VA(0x004c62d6, 0x1b2)
void TTownPropsGeneralPage::OnSelChangePlayerCombo()
{
    TPlayer newOwner = TPlayer(_m_playerCombo.GetItemData(_m_playerCombo.GetCurSel()));
    TTown* pNewTown = _getNewTown();
    if (newOwner == pNewTown->getOwner())
        return;
    if (pNewTown->getPVisitingHero()
        && (newOwner == ePlayerNone || !_m_newMap.getAvailableHeroOwnersMask()[newOwner])) {
        const char* const reason = newOwner == ePlayerNone ? kHeroesNeedOwnerStr : kPlayerHasMaxHeroesStr;
        CString message;
        message.Format(kContinuingWillDeleteHeroFmtStr, reason);
        if (MessageBox(message, kWarningStr, MB_ICONQUESTION | MB_YESNO) == IDNO) {
            int index;
            for (index = 0; TPlayer(_m_playerCombo.GetItemData(index)) != pNewTown->getOwner(); index++)
                ;
            _m_playerCombo.SetCurSel(index);
            return;
        }
        OnRemoveHeroButton();
    }
    _m_newMap.setTownOwner(newOwner, _m_townRef.getBSecondLayer(), _m_townRef.getObjectID());
    if (newOwner != ePlayerNone && _m_newMap.getAvailableHeroOwnersMask()[newOwner]) {
        if (pNewTown->getPVisitingHero())
            _m_bVisitingHeroModified = true;
        else if (!_m_bIsMainTown)
            _m_addHeroButton.EnableWindow(TRUE);
    } else
        _m_addHeroButton.EnableWindow(FALSE);
}
