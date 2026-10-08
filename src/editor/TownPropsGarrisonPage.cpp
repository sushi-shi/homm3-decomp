// TownPropsGarrisonPage.cpp - Loki h3maped object 92: the garrison page of
// the town property sheet. The town_props_garrison_customize toggle
// enables a TArmyDlg over the tp_ prefixed garrison widgets. The
// allocation check's line comes from the retail immediate.
#include "editor/stdafx.h"

#include "exceptions.h"
#include "editor/cppbridge.h"
#include "editor/ArmyDlg.h"
#include "editor/TownPropsGarrisonPage.h"

TTownPropsGarrisonPage::TTownPropsGarrisonPage(const TTown& town)
    : _m_customizeCheck(GTK_TOGGLE_BUTTON(_widget("town_props_garrison_customize"))),
      _m_town(town),
      _m_bModified(false),
      _m_pArmyDlg(NULL)
{
    _m_formation = -1;
    _m_pArmyDlg = new TArmyDlg(_m_town.getGarrison(), "tp_");
#line 55
    if (!_m_pArmyDlg)
        throw TAllocationFailure(__FILE__, __LINE__);
    OnInitDialog();
}

TTownPropsGarrisonPage::~TTownPropsGarrisonPage()
{
    delete _m_pArmyDlg;
}

const TArmy& TTownPropsGarrisonPage::getGarrison() const
{
    return _m_pArmyDlg->getArmy();
}

BOOL TTownPropsGarrisonPage::OnInitDialog()
{
    _m_bModified = false;
    _m_bCustomGarrison = _m_town.getBCustomGarrison();
    _m_bGroupedFormation = _m_town.getBGroupedFormation();
    _m_formation = _m_bGroupedFormation ? 1 : 0;
    if (_m_bCustomGarrison)
        gtk_toggle_button_set_active(_m_customizeCheck, TRUE);
    else
        _m_pArmyDlg->EnableWindow(false);
    return true;
}

void TTownPropsGarrisonPage::OnOK()
{
    _m_pArmyDlg->OnOK();
    _m_bCustomGarrison = gtk_toggle_button_get_active(_m_customizeCheck);
    _m_bGroupedFormation = _m_formation != 0;
    _m_bModified = _m_bModified || _m_bCustomGarrison != _m_town.getBCustomGarrison()
                   || _m_bGroupedFormation != _m_town.getBGroupedFormation() || _m_pArmyDlg->wasModified();
}

void TTownPropsGarrisonPage::OnCustomizeCheck()
{
    if (gtk_toggle_button_get_active(_m_customizeCheck)) {
        if (!_m_pArmyDlg->IsWindowEnabled())
            _m_pArmyDlg->EnableWindow(true);
    } else {
        if (_m_pArmyDlg->IsWindowEnabled())
            _m_pArmyDlg->EnableWindow(false);
    }
}
