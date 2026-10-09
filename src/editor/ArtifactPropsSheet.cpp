// ArtifactPropsSheet.cpp - the artifact property sheet (h3maped
// 0x4041ba..0x404739; Loki h3maped object 100).
#include "editor/stdafx.h"

#include "objnames.h"
#include "va.h"
#include "editor/ArtifactPropsSheet.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"

VA(0x00404413, 0x1a5)
TArtifactPropsSheet::TArtifactPropsSheet(CWnd* pParentWnd, TGameArtifact* pArtifact, EGameVersion mapVersion)
    : CPropertySheet(TFormattedString(kObjectPropertiesCaptionFmtStr, akAdvObjectTypeTraits[ARTIFACT].m_name),
                     pParentWnd),
      _m_pArtifact(pArtifact)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    _m_pGeneralPage =
        std::auto_ptr<TArtifactPropsGeneralPage>(new TArtifactPropsGeneralPage(_m_pArtifact, mapVersion));
    if (!_m_pGeneralPage.get())
        throw TAllocationFailure();
    _m_pGuardiansPage = std::auto_ptr<TTreasurePropsGuardiansPage>(
        new TTreasurePropsGuardiansPage(_m_pArtifact, mapVersion, IDH_ARTIFACT_GUARDIANS));
    if (!_m_pGuardiansPage.get())
        throw TAllocationFailure();
    AddPage(_m_pGeneralPage.get());
    AddPage(_m_pGuardiansPage.get());
}

VA_COMPGEN(0x004045b8, 0x1c, SCALAR_DELETING_DTOR, TArtifactPropsSheet)

VA(0x004045d4, 0x6c)
TArtifactPropsSheet::~TArtifactPropsSheet()
{
    RemovePage(_m_pGuardiansPage.get());
    RemovePage(_m_pGeneralPage.get());
}

VA(0x00404640, 0x22)
bool TArtifactPropsSheet::wasModified() const
{
    return _m_pGeneralPage->wasModified() || _m_pGuardiansPage->wasModified();
}

VA(0x00404662, 0x6)
BEGIN_MESSAGE_MAP(TArtifactPropsSheet, CPropertySheet)
END_MESSAGE_MAP()

VA(0x00404668, 0x98)
int TArtifactPropsSheet::DoModal()
{
    int result = CPropertySheet::DoModal();
    if (_m_pGeneralPage->wasModified())
        _m_pArtifact->setMessage(_m_pGeneralPage->getMessage());
    if (_m_pGuardiansPage->wasModified()) {
        _m_pArtifact->setBCustomGuardians(_m_pGuardiansPage->getBCustomGuardians());
        _m_pArtifact->setGuardians(_m_pGuardiansPage->getGuardians());
    }
    return result;
}
