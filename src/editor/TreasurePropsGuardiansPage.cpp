// TreasurePropsGuardiansPage.cpp - Loki h3maped object 102: the guardians
// page of the artifact property sheet. A customize toggle enables a
// TArmyDlg over the gd_ prefixed guardian widgets. The allocation check's
// line comes from the retail immediate.
#include "editor/stdafx.h"

#include "exceptions.h"
#include "editor/cppbridge.h"
#include "editor/TreasurePropsGuardiansPage.h"
#include "editor/ArmyDlg.h"

TTreasurePropsGuardiansPage::TTreasurePropsGuardiansPage(const TTreasure& treasure, unsigned int pageNum)
    : _m_treasure(treasure),
      _m_pArmyDlg(NULL),
      _m_bModified(false),
      _m_customizeCheck(GTK_TOGGLE_BUTTON(_widget("artifact_props_guardians_customize")))
{
    _m_bCustomizeCheck = FALSE;
    _m_pArmyDlg = new TArmyDlg(_m_treasure.getGuardians(), "gd_");
#line 54
    if (!_m_pArmyDlg)
        throw TAllocationFailure(__FILE__, __LINE__);
    OnInitDialog();
}

TTreasurePropsGuardiansPage::~TTreasurePropsGuardiansPage()
{
    delete _m_pArmyDlg;
}

const TArmy& TTreasurePropsGuardiansPage::getGuardians() const
{
    return _m_pArmyDlg->getArmy();
}

void TTreasurePropsGuardiansPage::OnDestroy()
{
}

BOOL TTreasurePropsGuardiansPage::OnInitDialog()
{
    _m_bModified = false;
    _m_bCustomGuardians = _m_treasure.getBCustomGuardians();
    _m_bCustomizeCheck = _m_bCustomGuardians;
    gtk_signal_handler_block_by_func(GTK_OBJECT(_m_customizeCheck),
                                     GTK_SIGNAL_FUNC(on_artifact_props_guardians_customize_toggled), NULL);
    gtk_toggle_button_set_active(_m_customizeCheck, _m_bCustomizeCheck);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(_m_customizeCheck),
                                       GTK_SIGNAL_FUNC(on_artifact_props_guardians_customize_toggled), NULL);
    _m_pArmyDlg->EnableWindow(_m_bCustomizeCheck);
    return true;
}

void TTreasurePropsGuardiansPage::OnOK()
{
    _m_pArmyDlg->OnOK();
    _m_bCustomGuardians = _m_bCustomizeCheck != FALSE;
    _m_bModified = _m_bModified || _m_bCustomGuardians != _m_treasure.getBCustomGuardians()
                   || _m_pArmyDlg->wasModified();
}

void TTreasurePropsGuardiansPage::OnCustomizeCheck()
{
    _m_bCustomizeCheck = gtk_toggle_button_get_active(_m_customizeCheck);
    _m_pArmyDlg->EnableWindow(_m_bCustomizeCheck);
}
