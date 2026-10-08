// TownPropsGeneralPage.cpp - Loki h3maped object 93: the town property
// sheet's general page. The owner combo lists "none" and the eight
// players; the visiting hero is edited as a clone the page owns until OnOK
// copies it to the result the sheet reads. The assert and throw lines come
// from the retail immediates.
#include "editor/stdafx.h"

#include <stdio.h>
#include <string>

#include "editor/cppbridge.h"
#include "editor/TownPropsGeneralPage.h"

#include "exceptions.h"
#include "editor/Hero.h"
#include "editor/GameMap.h"
#include "editor/HeroPropsSheet.h"
#include "editor/MapEditorText.h"
#include "editor/SelectHeroClassDlg.h"
#include "editor/Town.h"

TTownPropsGeneralPage::TTownPropsGeneralPage(TTown* pTown, const TGameMap& map, bool bIsMainTown)
    : _m_playerCombo(GTK_COMBO(_widget("town_props_general_player_combo"))),
      _m_nameEdit(GTK_ENTRY(_widget("town_props_general_player_name"))),
      _m_removeButton(GTK_BUTTON(_widget("town_props_general_remove"))),
      _m_editButton(GTK_BUTTON(_widget("town_props_general_edit"))),
      _m_addButton(GTK_BUTTON(_widget("town_props_general_add"))),
      _m_pTown(pTown),
      _m_pMap(NULL),
      _m_bIsMainTown(bIsMainTown),
      _m_bInitialized(false),
      _m_pVisitingHero(NULL),
      _m_pIntVisitingHero(NULL),
      _m_bModified(false),
      _m_bVisitingHeroModified(false)
{
#line 119
    assert(_m_pTown != NULL);
    assert(!bIsMainTown || pTown->getPVisitingHero() == NULL);
    _m_townTypeName = "";
    _m_visitingHeroClass = "";
    _m_visitingHeroName = "";
    _m_bCustomName = false;
    _m_nameText = "";
    _m_pMap = new TGameMap(map);
    if (!_m_pMap)
#line 139
        throw TAllocationFailure(__FILE__, __LINE__);
    OnInitDialog();
}

TTownPropsGeneralPage::~TTownPropsGeneralPage()
{
    delete _m_pIntVisitingHero;
    delete _m_pMap;
}

THero* TTownPropsGeneralPage::_createHero()
{
    THeroClassMask availableClasses;
    unsigned int heroClass;
    for (heroClass = 0; heroClass <= kNumHeroClasses; heroClass++)
        availableClasses[heroClass] = _m_pMap->canCreate(THero::s_akClassTraits[heroClass].m_objType, _m_owner);
    if (availableClasses.none()) {
        MessageBeep((unsigned long)-1);
        MessageBox(kNoMoreHeroesStr);
        return NULL;
    }

    gtk_window_set_modal(GTK_WINDOW(_widget("town_props_dlg")), FALSE);
    TSelectHeroClassDlg selectHeroClassDlg(_widget("select_hero_dlg"), availableClasses);
    int result = selectHeroClassDlg.DoModal();
    gtk_window_set_modal(GTK_WINDOW(_widget("town_props_dlg")), TRUE);
    if (result != 1)
        return NULL;

    heroClass = selectHeroClassDlg.getHeroClass();
#line 177
    assert(heroClass >= 0 && heroClass < kNumHeroClasses + 1);
    assert(_m_pMap->canCreate( THero::s_akClassTraits[ heroClass ].m_objType, _m_owner ));
    TGameObject* pObj = _m_pMap->createObject(THero::s_akClassTraits[heroClass].m_objType, _m_owner,
                                              ::operator new);
    if (!pObj)
#line 182
        throw TAllocationFailure(__FILE__, __LINE__);
#line 184
    assert(dynamic_cast< TNonRandomHero * >( pObj ) != NULL || dynamic_cast< TRandomHero * >( pObj ) != NULL);
    return dynamic_cast<THero*>(pObj);
}

BOOL TTownPropsGeneralPage::OnInitDialog()
{
    delete _m_pIntVisitingHero;
    _m_pIntVisitingHero = NULL;
    _m_bModified = false;
    _m_bVisitingHeroModified = false;
    _m_intOwner = _m_pTown->getOwner();
    _m_bIntCustomName = _m_pTown->getBCustomName();
    _m_intName = _m_pTown->getName();
    if (_m_pTown->getPVisitingHero()) {
#line 249
        assert(!_m_bIsMainTown);
        TGameObject* pObj = _m_pTown->getPVisitingHero()->clone(::operator new);
        if (!pObj)
#line 253
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pIntVisitingHero = dynamic_cast<THero*>(pObj);
#line 255
        assert(_m_pIntVisitingHero != NULL);
    } else
        _m_pIntVisitingHero = NULL;
    _m_bIntVisitingHeroRemoved = false;

    _m_townTypeName = TTown::s_akTypeTraits[_m_pTown->getTownType()].m_pName;
    _m_owner = _m_intOwner;
    _m_bCustomName = _m_bIntCustomName;
    _m_name = _m_intName.c_str();
    _m_nameText = _m_bCustomName ? _m_name : string(kUnknownStr);
    if (_m_pIntVisitingHero) {
        TGameObject* pObj = _m_pIntVisitingHero->clone(::operator new);
        if (!pObj)
#line 270
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pVisitingHero = dynamic_cast<THero*>(pObj);
#line 272
        assert(_m_pVisitingHero != NULL);
    } else
        _m_pVisitingHero = NULL;
    _m_bVisitingHeroRemoved = false;
    if (_m_pVisitingHero) {
        _m_visitingHeroClass = _m_pVisitingHero->getClassTraits().m_name;
        _m_visitingHeroName = _m_pVisitingHero->getName().c_str();
    } else {
        _m_visitingHeroClass = "";
        _m_visitingHeroName = "";
    }

    GList* glist = g_list_append(NULL, (gpointer)kSelNoneStr);
    unsigned int cbIndex = 1;
    _m_aComboIndexPlayer[0] = ePlayerNone;
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        glist = g_list_append(glist, (gpointer)akPlayerTraits[player].m_pName);
        _m_aComboIndexPlayer[cbIndex] = player;
        cbIndex++;
    }
    gtk_combo_set_popdown_strings(_m_playerCombo, glist);
    for (cbIndex = 0; _m_aComboIndexPlayer[cbIndex] != _m_owner; cbIndex++)
#line 311
        assert(cbIndex < g_list_length(glist));
    g_list_free(glist);
    setCurrentSelection(GTK_LIST(_m_playerCombo->list), cbIndex);

    gtk_widget_set_sensitive(GTK_WIDGET(_m_nameEdit), _m_bCustomName);
    gtk_entry_set_max_length(_m_nameEdit, 12);
    _m_bNameIsSpace = _isspace(_m_name);
    if (_m_bCustomName && _m_bNameIsSpace)
        gtk_widget_set_sensitive(_widget("town_props_ok"), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_addButton),
                             !_m_bIsMainTown && _m_pVisitingHero == NULL && _m_owner != ePlayerNone
                                 && _m_pMap->getAvailableHeroOwnersMask()[_m_owner]);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_editButton), _m_pVisitingHero != NULL);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_removeButton), _m_pVisitingHero != NULL);
    _m_bInitialized = true;
    UpdateData(false);
    return true;
}

void TTownPropsGeneralPage::OnOK()
{
#line 348
    assert(!_m_bCustomName || !_m_bNameIsSpace);
    _m_intOwner = _m_owner;
    _m_bIntCustomName = _m_bCustomName != false;
    _m_intName = _m_name;
    delete _m_pIntVisitingHero;
    _m_pIntVisitingHero = NULL;
    if (_m_pVisitingHero) {
        TGameObject* pObj = _m_pVisitingHero->clone(::operator new);
        if (!pObj)
#line 363
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pIntVisitingHero = dynamic_cast<THero*>(pObj);
#line 365
        assert(_m_pIntVisitingHero != NULL);
    }
    _m_bIntVisitingHeroRemoved = _m_bVisitingHeroRemoved;
    _m_bModified = _m_bModified || _m_bVisitingHeroModified || _m_intOwner != _m_pTown->getOwner()
                   || _m_bIntCustomName != _m_pTown->getBCustomName() || _m_intName != _m_pTown->getName();
}

void TTownPropsGeneralPage::OnVisitingAddButton()
{
#line 379
    assert(!_m_bIsMainTown);
    assert(_m_pVisitingHero == NULL);
    assert(_m_owner != ePlayerNone && _m_pMap->getAvailableHeroOwnersMask()[ _m_owner ]);
    _m_pVisitingHero = _createHero();
    if (_m_pVisitingHero) {
        _m_bVisitingHeroModified = true;
        _m_pMap->onHeroAdded(*_m_pVisitingHero);
        _m_visitingHeroClass = _m_pVisitingHero->getClassTraits().m_name;
        _m_visitingHeroName = _m_pVisitingHero->getName().c_str();
        UpdateData(false);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_addButton), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_editButton), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_removeButton), TRUE);
    }
}

void TTownPropsGeneralPage::OnVisitingEditButton()
{
#line 406
    assert(_m_pVisitingHero != NULL);
    TNonRandomHero* pNonRandomHero = dynamic_cast<TNonRandomHero*>(_m_pVisitingHero);
    if (pNonRandomHero) {
        unsigned int oldProtoNum = pNonRandomHero->getProtoNum();
        TNonRandomHeroPropsSheet sheet(this, pNonRandomHero, bitset<kNumPlayers>(),
                                       _m_pMap->getAvailableHeroesInClass(pNonRandomHero->getClass()));
        sheet.DoModal();
        _m_bVisitingHeroModified = _m_bVisitingHeroModified || sheet.wasModified();
        if (pNonRandomHero->getProtoNum() != oldProtoNum)
            _m_pMap->onHeroProtoChanged(pNonRandomHero->getClass(), oldProtoNum, pNonRandomHero->getProtoNum());
    } else {
#line 425
        assert(dynamic_cast< TRandomHero * >( _m_pVisitingHero ) != NULL);
        TRandomHero* pRandomHero = static_cast<TRandomHero*>(_m_pVisitingHero);
        TRandomHeroPropsSheet sheet(this, pRandomHero, bitset<kNumPlayers>());
        sheet.DoModal();
        _m_bVisitingHeroModified = _m_bVisitingHeroModified || sheet.wasModified();
    }
    _m_visitingHeroName = _m_pVisitingHero->getName().c_str();
    UpdateData(false);
}

void TTownPropsGeneralPage::OnVisitingRemoveButton()
{
#line 440
    assert(_m_pVisitingHero != NULL);
    _m_bVisitingHeroModified = true;
    _m_bVisitingHeroRemoved = true;
    _m_pMap->onRemovingHero(*_m_pVisitingHero);
    delete _m_pVisitingHero;
    _m_pVisitingHero = NULL;
    _m_visitingHeroClass = "";
    _m_visitingHeroName = "";
    UpdateData(false);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_addButton), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_editButton), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(_m_removeButton), FALSE);
}

void TTownPropsGeneralPage::OnChangeNameEdit()
{
#line 466
    assert(_m_bCustomName);
    UpdateData(false);
    _m_name = _m_nameText;
    if (_isspace(_m_name)) {
        if (!_m_bNameIsSpace) {
            _m_bNameIsSpace = true;
            gtk_widget_set_sensitive(_widget("town_props_ok"), FALSE);
        }
    } else if (_m_bNameIsSpace) {
        _m_bNameIsSpace = false;
        gtk_widget_set_sensitive(_widget("town_props_ok"), TRUE);
    }
}

void TTownPropsGeneralPage::OnCustomizeCheck()
{
    UpdateData(false);
    if (_m_bCustomName) {
        gtk_widget_set_sensitive(GTK_WIDGET(_m_nameEdit), TRUE);
        _m_nameText = _m_name;
        UpdateData(false);
        if (_m_bNameIsSpace)
            gtk_widget_set_sensitive(_widget("town_props_ok"), FALSE);
        gtk_widget_grab_focus(GTK_WIDGET(_m_nameEdit));
    } else {
        if (_m_bNameIsSpace)
            gtk_widget_set_sensitive(_widget("town_props_ok"), FALSE);
        _m_nameText = kUnknownStr;
        UpdateData(false);
        gtk_widget_set_sensitive(GTK_WIDGET(_m_nameEdit), FALSE);
    }
}

void TTownPropsGeneralPage::UpdateData(bool bSaveAndValidate)
{
    gtk_label_set_text(GTK_LABEL(_widget("town_props_general_type")), _m_townTypeName.c_str());
    gtk_label_set_text(GTK_LABEL(_widget("town_props_general_visiting_class")), _m_visitingHeroClass.c_str());
    gtk_label_set_text(GTK_LABEL(_widget("town_props_general_visiting_name")), _m_visitingHeroName.c_str());
    _m_bCustomName = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_widget("town_props_general_customize")));
    if (!_m_bCustomName) {
        gtk_signal_handler_block_by_func(GTK_OBJECT(_m_nameEdit),
                                         GTK_SIGNAL_FUNC(on_town_props_general_player_name_changed), NULL);
        gtk_entry_set_text(_m_nameEdit, _m_visitingHeroName.c_str());
        gtk_signal_handler_unblock_by_func(GTK_OBJECT(_m_nameEdit),
                                           GTK_SIGNAL_FUNC(on_town_props_general_player_name_changed), NULL);
    }
    _m_nameText = gtk_entry_get_text(_m_nameEdit);
    _m_bNameIsSpace = _isspace(_m_name);
    if (_m_bCustomName && _m_bNameIsSpace)
        gtk_widget_set_sensitive(_widget("town_props_ok"), FALSE);
    else
        gtk_widget_set_sensitive(_widget("town_props_ok"), TRUE);
}

void TTownPropsGeneralPage::OnSelChangePlayerCombo()
{
#line 565
    assert(getCurrentSelection(GTK_LIST(_m_playerCombo->list)) != -1);
    TPlayer newOwner = TPlayer(_m_aComboIndexPlayer[getCurrentSelection(GTK_LIST(_m_playerCombo->list))]);
#line 568
    assert(newOwner >= ePlayerNone && newOwner < kNumPlayers);
    if (newOwner != _m_owner) {
        if (_m_pVisitingHero
            && (newOwner == ePlayerNone || !_m_pMap->getAvailableHeroOwnersMask()[newOwner])) {
            const char* const reason = newOwner == ePlayerNone ? kHeroesNeedOwnerStr : kPlayerHasMaxHeroesStr;
            char message[1024];
            snprintf(message, 1024, kContinuingWillDeleteHeroFmtStr, reason);
            if (!askYesNoQuestion(message)) {
                unsigned int cbIndex;
                for (cbIndex = 0; _m_aComboIndexPlayer[cbIndex] != _m_owner; cbIndex++)
#line 590
                    assert(cbIndex < g_list_length(GTK_LIST(_m_playerCombo->list)->children));
                setCurrentSelection(GTK_LIST(_m_playerCombo->list), cbIndex);
                return;
            } else
                OnVisitingRemoveButton();
        }
        if (newOwner == ePlayerNone || !_m_pMap->getAvailableHeroOwnersMask()[newOwner]) {
#line 604
            assert(_m_pVisitingHero == NULL);
            gtk_widget_set_sensitive(GTK_WIDGET(_m_addButton), FALSE);
        } else if (_m_pVisitingHero) {
#line 612
            assert(!_m_bIsMainTown);
            _m_pVisitingHero->setOwner(newOwner);
            _m_bVisitingHeroModified = true;
            _m_pMap->onHeroOwnerChanged(*_m_pVisitingHero, _m_owner);
        } else if (!_m_bIsMainTown)
            gtk_widget_set_sensitive(GTK_WIDGET(_m_addButton), TRUE);
        _m_owner = newOwner;
    }
}

void TTownPropsGeneralPage::OnDestroy()
{
    if (_m_bInitialized) {
        if (_m_pVisitingHero)
            _m_pMap->onRemovingHero(*_m_pVisitingHero);
        if (_m_pTown->getPVisitingHero())
            _m_pMap->onHeroAdded(*_m_pTown->getPVisitingHero());
        delete _m_pVisitingHero;
        _m_pVisitingHero = NULL;
        _m_bInitialized = false;
    }
}
