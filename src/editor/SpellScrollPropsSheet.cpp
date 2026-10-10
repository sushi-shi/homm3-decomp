// SpellScrollPropsSheet.cpp - the spell scroll property sheet (h3maped
// 0x4b9bb2..0x4ba0c7; GOG only). Its caption names the artifact object
// type, and the guardians page keeps the artifact's help.
#include "editor/stdafx.h"

#include "objnames.h"
#include "va.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/SpellScrollPropsSheet.h"

VA(0x004b9d86, 0x1a5)
TSpellScrollPropsSheet::TSpellScrollPropsSheet(CWnd* pParentWnd, TSpellScroll* pSpellScroll, EGameVersion mapVersion)
    : CPropertySheet(TFormattedString(kObjectPropertiesCaptionFmtStr, akAdvObjectTypeTraits[ARTIFACT].m_name),
                     pParentWnd),
      _m_pSpellScroll(pSpellScroll)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    _m_pGeneralPage =
        std::auto_ptr<TSpellScrollPropsGeneralPage>(new TSpellScrollPropsGeneralPage(_m_pSpellScroll, mapVersion));
    if (!_m_pGeneralPage.get())
        throw TAllocationFailure();
    _m_pGuardiansPage = std::auto_ptr<TTreasurePropsGuardiansPage>(
        new TTreasurePropsGuardiansPage(_m_pSpellScroll, mapVersion, IDH_ARTIFACT_GUARDIANS));
    if (!_m_pGuardiansPage.get())
        throw TAllocationFailure();
    AddPage(_m_pGeneralPage.get());
    AddPage(_m_pGuardiansPage.get());
}

VA_COMPGEN(0x004b9f2b, 0x1c, SCALAR_DELETING_DTOR, TSpellScrollPropsSheet)

VA(0x004b9f47, 0x6c)
TSpellScrollPropsSheet::~TSpellScrollPropsSheet()
{
    RemovePage(_m_pGuardiansPage.get());
    RemovePage(_m_pGeneralPage.get());
}

VA(0x004b9fb3, 0x22)
bool TSpellScrollPropsSheet::wasModified() const
{
    return _m_pGeneralPage->wasModified() || _m_pGuardiansPage->wasModified();
}

VA(0x004b9fd5, 0x6)
BEGIN_MESSAGE_MAP(TSpellScrollPropsSheet, CPropertySheet)
END_MESSAGE_MAP()

VA(0x004b9fdb, 0xb3)
int TSpellScrollPropsSheet::DoModal()
{
    int result = CPropertySheet::DoModal();
    if (_m_pGeneralPage->wasModified()) {
        _m_pSpellScroll->setMessage(_m_pGeneralPage->getMessage());
        _m_pSpellScroll->setSpell(_m_pGeneralPage->getSpell());
    }
    if (_m_pGuardiansPage->wasModified()) {
        _m_pSpellScroll->setBCustomGuardians(_m_pGuardiansPage->getBCustomGuardians());
        _m_pSpellScroll->setGuardians(_m_pGuardiansPage->getGuardians());
    }
    return result;
}
