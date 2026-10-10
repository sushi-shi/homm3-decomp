// MonsterPropsSheet.cpp - the monster property sheet (h3maped
// 0x4898d1..0x489e5d; GOG only).
#include "editor/stdafx.h"

#include "objnames.h"
#include "va.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"
#include "editor/Monster.h"
#include "editor/MonsterPropsSheet.h"

VA(0x00489aa5, 0x1a3)
TMonsterPropsSheet::TMonsterPropsSheet(CWnd* pParentWnd, TMonster* pMonster, EGameVersion mapVersion)
    : CPropertySheet(TFormattedString(kObjectPropertiesCaptionFmtStr, akAdvObjectTypeTraits[MONSTER].m_name),
                     pParentWnd),
      _m_pMonster(pMonster)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    _m_pGeneralPage =
        std::auto_ptr<TMonsterPropsGeneralPage>(new TMonsterPropsGeneralPage(_m_pMonster, mapVersion));
    if (!_m_pGeneralPage.get())
        throw TAllocationFailure();
    _m_pTreasurePage =
        std::auto_ptr<TMonsterPropsTreasurePage>(new TMonsterPropsTreasurePage(_m_pMonster, mapVersion));
    if (!_m_pTreasurePage.get())
        throw TAllocationFailure();
    AddPage(_m_pGeneralPage.get());
    AddPage(_m_pTreasurePage.get());
}

VA_COMPGEN(0x00489c48, 0x1c, SCALAR_DELETING_DTOR, TMonsterPropsSheet)

VA(0x00489c64, 0x6c)
TMonsterPropsSheet::~TMonsterPropsSheet()
{
    RemovePage(_m_pTreasurePage.get());
    RemovePage(_m_pGeneralPage.get());
}

VA(0x00489cd0, 0x22)
bool TMonsterPropsSheet::wasModified() const
{
    return _m_pGeneralPage->wasModified() || _m_pTreasurePage->wasModified();
}

VA(0x00489cf2, 0x6)
BEGIN_MESSAGE_MAP(TMonsterPropsSheet, CPropertySheet)
END_MESSAGE_MAP()

VA(0x00489cf8, 0x12c)
int TMonsterPropsSheet::DoModal()
{
    int result = CPropertySheet::DoModal();
    if (_m_pGeneralPage->wasModified()) {
        _m_pMonster->setQuantity(_m_pGeneralPage->getQuantity());
        _m_pMonster->setDisposition(TMonster::TDisposition(_m_pGeneralPage->getDisposition()));
        _m_pMonster->setBNeverFlees(_m_pGeneralPage->getBNeverFlees());
        _m_pMonster->setBNeverGrows(_m_pGeneralPage->getBNeverGrows());
        _m_pMonster->setMessage(_m_pGeneralPage->getMessage());
    }
    if (_m_pTreasurePage->wasModified()) {
        for (unsigned int type = 0; type < kNumGameResourceTypes; type++)
            _m_pMonster->setResourceQuantity(TGameResourceType(type),
                                             _m_pTreasurePage->getResourceQuantity(TGameResourceType(type)));
        _m_pMonster->setArtifact(_m_pTreasurePage->getArtifact());
    }
    return result;
}
