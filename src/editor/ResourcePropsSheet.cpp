// ResourcePropsSheet.cpp - the resource property sheet (h3maped
// 0x4b2ec0..0x4b33d4; GOG only, TArtifactPropsSheet's twin).
#include "editor/stdafx.h"

#include "objnames.h"
#include "va.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/ResourcePropsSheet.h"

VA(0x004b3094, 0x1a8)
TResourcePropsSheet::TResourcePropsSheet(CWnd* pParentWnd, TGameResource* pResource, EGameVersion mapVersion)
    : CPropertySheet(TFormattedString(kObjectPropertiesCaptionFmtStr, akAdvObjectTypeTraits[RESOURCE].m_name),
                     pParentWnd),
      _m_pResource(pResource)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    _m_pGeneralPage =
        std::auto_ptr<TResourcePropsGeneralPage>(new TResourcePropsGeneralPage(_m_pResource, mapVersion));
    if (!_m_pGeneralPage.get())
        throw TAllocationFailure();
    _m_pGuardiansPage = std::auto_ptr<TTreasurePropsGuardiansPage>(
        new TTreasurePropsGuardiansPage(_m_pResource, mapVersion, IDH_RESOURCE_GUARDIANS));
    if (!_m_pGuardiansPage.get())
        throw TAllocationFailure();
    AddPage(_m_pGeneralPage.get());
    AddPage(_m_pGuardiansPage.get());
}

VA_COMPGEN(0x004b323c, 0x1c, SCALAR_DELETING_DTOR, TResourcePropsSheet)

VA(0x004b3258, 0x6c)
TResourcePropsSheet::~TResourcePropsSheet()
{
    RemovePage(_m_pGuardiansPage.get());
    RemovePage(_m_pGeneralPage.get());
}

VA(0x004b32c4, 0x22)
bool TResourcePropsSheet::wasModified() const
{
    return _m_pGeneralPage->wasModified() || _m_pGuardiansPage->wasModified();
}

VA(0x004b32e6, 0x6)
BEGIN_MESSAGE_MAP(TResourcePropsSheet, CPropertySheet)
END_MESSAGE_MAP()

VA(0x004b32ec, 0xaf)
int TResourcePropsSheet::DoModal()
{
    int result = CPropertySheet::DoModal();
    if (_m_pGeneralPage->wasModified()) {
        _m_pResource->setQuantity(_m_pGeneralPage->getQuantity());
        _m_pResource->setMessage(_m_pGeneralPage->getMessage());
    }
    if (_m_pGuardiansPage->wasModified()) {
        _m_pResource->setBCustomGuardians(_m_pGuardiansPage->getBCustomGuardians());
        _m_pResource->setGuardians(_m_pGuardiansPage->getGuardians());
    }
    return result;
}
