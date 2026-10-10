// TownPropsSheet.cpp - the town property sheet (h3maped 0x4c64a5..0x4c6c3c;
// Loki h3maped object 90).
#include "editor/stdafx.h"

#include "adventureobjecttype.h"
#include "exceptions.h"
#include "objnames.h"
#include "va.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"
#include "editor/TownPropsBuildingsPage.h"
#include "editor/TownPropsGarrisonPage.h"
#include "editor/TownPropsSheet.h"
#include "editor/TownPropsSpellsPage.h"
#include "editor/TownPropsTimedEventsPage.h"

VA(0x004c6679, 0x41d)
TTownPropsSheet::TTownPropsSheet(CWnd* pParentWnd, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
    : CPropertySheet(TFormattedString(kObjectPropertiesCaptionFmtStr, akAdvObjectTypeTraits[TOWN].m_name),
                     pParentWnd),
      _m_pMap(pMap)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    _m_pNewMap = std::auto_ptr<TGameMap>(new TGameMap(*_m_pMap));
    if (!_m_pNewMap.get())
        throw TAllocationFailure();
    _m_pGeneralPage = std::auto_ptr<TTownPropsGeneralPage>(
        new TTownPropsGeneralPage(this, *_m_pMap, *_m_pNewMap, bSecondLayer, objID));
    if (!_m_pGeneralPage.get())
        throw TAllocationFailure();
    _m_pGarrisonPage =
        std::auto_ptr<TTownPropsGarrisonPage>(new TTownPropsGarrisonPage(*_m_pMap, *_m_pNewMap, bSecondLayer, objID));
    if (!_m_pGarrisonPage.get())
        throw TAllocationFailure();
    _m_pBuildingsPage =
        std::auto_ptr<TTownPropsBuildingsPage>(new TTownPropsBuildingsPage(*_m_pMap, *_m_pNewMap, bSecondLayer, objID));
    if (!_m_pBuildingsPage.get())
        throw TAllocationFailure();
    _m_pSpellsPage = std::auto_ptr<TTownPropsSpellsPage>(new TTownPropsSpellsPage(*_m_pMap, *_m_pNewMap, bSecondLayer, objID));
    if (!_m_pSpellsPage.get())
        throw TAllocationFailure();
    _m_pTimedEventsPage =
        std::auto_ptr<TTownPropsTimedEventsPage>(new TTownPropsTimedEventsPage(*_m_pMap, *_m_pNewMap, bSecondLayer, objID));
    if (!_m_pTimedEventsPage.get())
        throw TAllocationFailure();
    AddPage(_m_pGeneralPage.get());
    AddPage(_m_pGarrisonPage.get());
    AddPage(_m_pBuildingsPage.get());
    AddPage(_m_pSpellsPage.get());
    AddPage(_m_pTimedEventsPage.get());
}

VA_COMPGEN(0x004c6a96, 0x1c, SCALAR_DELETING_DTOR, TTownPropsSheet)

VA(0x004c6ab2, 0xd9)
TTownPropsSheet::~TTownPropsSheet()
{
    RemovePage(_m_pTimedEventsPage.get());
    RemovePage(_m_pSpellsPage.get());
    RemovePage(_m_pBuildingsPage.get());
    RemovePage(_m_pGarrisonPage.get());
    RemovePage(_m_pGeneralPage.get());
}

VA(0x004c6b8b, 0x4c)
bool TTownPropsSheet::wasModified() const
{
    return _m_pGeneralPage->wasModified() || _m_pGarrisonPage->wasModified() || _m_pBuildingsPage->wasModified()
           || _m_pSpellsPage->wasModified() || _m_pTimedEventsPage->wasModified();
}

VA(0x004c6bd7, 0x17)
void TTownPropsSheet::onEnableOK()
{
    CWnd* pOKButton = GetDlgItem(IDOK);
    pOKButton->EnableWindow(TRUE);
}

VA(0x004c6bee, 0x17)
void TTownPropsSheet::onDisableOK()
{
    CWnd* pOKButton = GetDlgItem(IDOK);
    pOKButton->EnableWindow(FALSE);
}

VA(0x004c6c05, 0x6)
BEGIN_MESSAGE_MAP(TTownPropsSheet, CPropertySheet)
END_MESSAGE_MAP()

VA(0x004c6c0b, 0x31)
int TTownPropsSheet::DoModal()
{
    int result = CPropertySheet::DoModal();
    if (result == IDOK && wasModified())
        *_m_pMap = *_m_pNewMap;
    return result;
}
