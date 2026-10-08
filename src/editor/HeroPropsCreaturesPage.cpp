// HeroPropsCreaturesPage.cpp - Loki h3maped object 85: the hero sheet's
// creatures page. A customized army needs at least one stack, so the page
// asks its sheet to disable OK while the customized army is empty. The
// assert lines come from the retail immediates.
#include "editor/stdafx.h"

namespace {
#include <gtk/gtk.h>
}

#include "exceptions.h"
#include "editor/cppbridge.h"
#include "editor/HeroPropsCreaturesPage.h"

THeroPropsCreaturesPage::THeroPropsCreaturesPage(THeroPropsCreaturesPageParentSheet* pParentSheet,
                                                 const THero& hero)
    : _m_pParentSheet(pParentSheet),
      _m_hero(hero),
      _m_bModified(false),
      _m_pArmyDlg(NULL)
{
#line 53
    assert(_m_pParentSheet != NULL);
    _m_formation = -1;
    _m_pArmyDlg = new TArmyDlg(this, _m_hero.getCustomArmy(), "hp_");
    if (_m_pArmyDlg == NULL)
#line 67
        throw TAllocationFailure(__FILE__, __LINE__);
}

THeroPropsCreaturesPage::~THeroPropsCreaturesPage()
{
    delete _m_pArmyDlg;
}

void THeroPropsCreaturesPage::setIdentity(THeroClass newHeroClass, unsigned int newProtoNum)
{
#line 79
    assert(newHeroClass >= 0 && newHeroClass < kNumHeroClasses);
    assert(newProtoNum < THero::s_akClassTraits[ newHeroClass ].m_numPrototypes);
}

const TArmy& THeroPropsCreaturesPage::getArmy() const
{
    return _m_pArmyDlg->getArmy();
}

void THeroPropsCreaturesPage::onNumOccupiedStacksChanged(unsigned int newNum, unsigned int oldNum)
{
    if (isChecked("creatures_customize")) {
        if (newNum == 0) {
            if (oldNum != 0)
                _m_pParentSheet->onDisableOK();
        } else if (oldNum == 0)
            _m_pParentSheet->onEnableOK();
    }
}

BOOL THeroPropsCreaturesPage::OnInitDialog()
{
    _m_bModified = false;
    _m_bCustomArmy = _m_hero.getBCustomArmy();
    _m_formation = _m_hero.getBGroupedFormation();
    GtkToggleButton* pButton = GTK_TOGGLE_BUTTON(
        _widget(_m_formation ? "hp_creature_formation_grouped" : "hp_creature_formation_spread"));
    gtk_toggle_button_set_active(pButton, TRUE);
    if (!_m_bCustomArmy)
        _m_pArmyDlg->EnableWindow(false);
    pButton = GTK_TOGGLE_BUTTON(_widget("creatures_customize"));
    gtk_signal_handler_block_by_func(GTK_OBJECT(pButton), GTK_SIGNAL_FUNC(on_creatures_customize_toggled), NULL);
    gtk_toggle_button_set_active(pButton, _m_bCustomArmy);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(pButton), GTK_SIGNAL_FUNC(on_creatures_customize_toggled), NULL);
    return true;
}

void THeroPropsCreaturesPage::OnOK()
{
    _m_pArmyDlg->OnOK();
    if (isChecked("creatures_customize") && _m_pArmyDlg->getNumOccupiedStacks() == 0)
        return;
    _m_bCustomArmy = isChecked("creatures_customize");
    _m_formation = isChecked("hp_creature_formation_grouped");
    _m_bModified = _m_bModified || _m_pArmyDlg->wasModified() || _m_bCustomArmy != _m_hero.getBCustomArmy()
                   || (_m_formation != 0) != _m_hero.getBGroupedFormation();
}

void THeroPropsCreaturesPage::OnCustomizeCheck()
{
    if (!isChecked("creatures_customize")) {
        if (_m_pArmyDlg->IsWindowEnabled()) {
            if (_m_pArmyDlg->getNumOccupiedStacks() == 0)
                _m_pParentSheet->onEnableOK();
            _m_pArmyDlg->EnableWindow(false);
        }
    } else if (!_m_pArmyDlg->IsWindowEnabled()) {
        _m_pArmyDlg->EnableWindow(true);
        if (_m_pArmyDlg->getNumOccupiedStacks() == 0)
            _m_pParentSheet->onDisableOK();
    }
}
