// BlackBoxPropsSheet.cpp - the black box property sheet (h3maped
// 0x40c1d6..0x40c9d0; GOG only).
#include "editor/stdafx.h"

#include "objnames.h"
#include "va.h"
#include "editor/BlackBox.h"
#include "editor/BlackBoxPropsSheet.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"

VA(0x0040c3aa, 0x245)
TBlackBoxPropsSheet::TBlackBoxPropsSheet(CWnd* pParentWnd, TBlackBox* pBlackBox, EGameVersion mapVersion)
    : CPropertySheet(TFormattedString(kObjectPropertiesCaptionFmtStr, akAdvObjectTypeTraits[BLACK_BOX].m_name),
                     pParentWnd),
      _m_pBlackBox(pBlackBox)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    _m_pGeneralPage =
        std::auto_ptr<TBlackBoxPropsGeneralPage>(new TBlackBoxPropsGeneralPage(_m_pBlackBox, mapVersion));
    if (!_m_pGeneralPage.get())
        throw TAllocationFailure();
    _m_pGuardiansPage = std::auto_ptr<TTreasurePropsGuardiansPage>(
        new TTreasurePropsGuardiansPage(_m_pBlackBox, mapVersion, IDH_BLACK_BOX_GUARDIANS));
    if (!_m_pGuardiansPage.get())
        throw TAllocationFailure();
    _m_pContentsPage = std::auto_ptr<TBlackBoxPropsContentsPage>(
        new TBlackBoxPropsContentsPage(_m_pBlackBox, mapVersion, IDH_BLACK_BOX_CONTENTS));
    if (!_m_pContentsPage.get())
        throw TAllocationFailure();
    AddPage(_m_pGeneralPage.get());
    AddPage(_m_pGuardiansPage.get());
    AddPage(_m_pContentsPage.get());
}

VA_COMPGEN(0x0040c5ef, 0x1c, SCALAR_DELETING_DTOR, TBlackBoxPropsSheet)

VA(0x0040c60b, 0x88)
TBlackBoxPropsSheet::~TBlackBoxPropsSheet()
{
    RemovePage(_m_pContentsPage.get());
    RemovePage(_m_pGuardiansPage.get());
    RemovePage(_m_pGeneralPage.get());
}

VA(0x0040c693, 0x34)
bool TBlackBoxPropsSheet::wasModified() const
{
    return _m_pGeneralPage->wasModified() || _m_pGuardiansPage->wasModified() || _m_pContentsPage->wasModified();
}

VA(0x0040c6c7, 0x6)
BEGIN_MESSAGE_MAP(TBlackBoxPropsSheet, CPropertySheet)
END_MESSAGE_MAP()

VA(0x0040c6cd, 0x2aa)
int TBlackBoxPropsSheet::DoModal()
{
    int result = CPropertySheet::DoModal();
    if (_m_pGeneralPage->wasModified())
        _m_pBlackBox->setMessage(_m_pGeneralPage->getMessage());
    if (_m_pGuardiansPage->wasModified()) {
        _m_pBlackBox->setBCustomGuardians(_m_pGuardiansPage->getBCustomGuardians());
        _m_pBlackBox->setGuardians(_m_pGuardiansPage->getGuardians());
    }
    if (_m_pContentsPage->wasModified()) {
        _m_pBlackBox->getPContents()->setExperienceBonus(_m_pContentsPage->getExperienceBonus());
        _m_pBlackBox->getPContents()->setManaBonus(_m_pContentsPage->getManaBonus());
        _m_pBlackBox->getPContents()->setMoraleBonus(_m_pContentsPage->getMoraleBonus());
        _m_pBlackBox->getPContents()->setLuckBonus(_m_pContentsPage->getLuckBonus());
        _m_pBlackBox->getPContents()->setResourceQuantities(_m_pContentsPage->getResourceQuantities());
        _m_pBlackBox->getPContents()->setPrimarySkillBonuses(_m_pContentsPage->getPrimarySkillBonuses());
        _m_pBlackBox->getPContents()->setSecondarySkills(_m_pContentsPage->getSecondarySkills());
        _m_pBlackBox->getPContents()->setArtifacts(_m_pContentsPage->getArtifacts());
        _m_pBlackBox->getPContents()->setSpells(_m_pContentsPage->getSpells());
        _m_pBlackBox->getPContents()->setCreatureStacks(_m_pContentsPage->getCreatureStacks());
    }
    return result;
}
