// HeroPropsGeneralPage.cpp - Loki h3maped object 84: the hero sheet's
// general page and its random hero, specific hero and prison variants. The
// constructor renders every hero portrait into a pixmap once (through a
// 24-bit image on 24-bit visuals); the portrait scroll bar's handler shows
// the selected one. The assert lines come from the retail immediates.
#include "editor/stdafx.h"

#include <stdio.h>
#include <string.h>
#include <string>

namespace {
#include <gtk/gtk.h>
}

#include "bitmap816.h"
#include "exceptions.h"
#include "herodefs.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "editor/cppbridge.h"
#include "editor/Digits.h"
#include "editor/HeroPropsGeneralPage.h"
#include "editor/MapEditorText.h"

// The page whose portrait scroll bar is connected.
static THeroPropsGeneralPage* pGeneralPage = NULL;

static void onPortraitScrollValueChanged(GtkAdjustment* adj, gpointer data)
{
    if (pGeneralPage == NULL)
        return;
    int portrait = (int)adj->value;
    if (portrait != pGeneralPage->_m_portrait) {
        pGeneralPage->_setNewPortrait(portrait);
        pGeneralPage->_m_portrait = portrait;
    }
}

THeroPropsGeneralPage::THeroPropsGeneralPage(unsigned int idTemplate, THeroPropsGeneralPageParentSheet* pParentSheet,
                                             const THero& hero)
    : _m_pParentSheet(pParentSheet),
      _m_hero(hero),
      _m_pPrototype(NULL),
      _m_patrol(-1),
      _m_bCustomName(false),
      _m_bCustomPortrait(false),
      _m_bNameIsSpace(false),
      _m_bModified(false)
{
#line 123
    assert(_m_pParentSheet != NULL);
    pGeneralPage = this;
    GdkWindow* window = _widget("MainWindow")->window;
    GdkGC* gc = gdk_gc_new(window);
#line 129
    assert(gc != NULL);
    _m_apPortraitPixmap = new GdkPixmap*[kNumHeroBios];
    GdkVisual* visual = gdk_visual_get_system();
    for (unsigned int portrait = 0; portrait < kNumHeroBios; portrait++) {
        const char* name = akHeroTraits[portrait].m_large_portrait_name;
        TResourcePtr<Bitmap816> pBitmap(ResourceManager::GetBitmap816(name));
        if (pBitmap.get() == NULL)
#line 150
            throw TRuntimeError(__FILE__, __LINE__, string("Failed to load portrait bitmap:  \"") + name + "\".");
        GdkImage* image = gdk_image_new(GDK_IMAGE_FASTEST, visual, pBitmap->GetWidth(), pBitmap->GetHeight());
        pBitmap->Draw(0, 0, pBitmap->GetWidth(), pBitmap->GetHeight(), (uword*)image->mem, 0, 0, image->width,
                      image->height, getImgPitch(image), false);
        GdkPixmap* pixmap = gdk_pixmap_new(NULL, image->width, image->height, visual->depth);
        GdkVisual* systemVisual = gdk_visual_get_system();
        if (systemVisual->depth == 24) {
            GdkImage* image24 = gdk_image_new(GDK_IMAGE_FASTEST, gdk_visual_get_system(), image->width, image->height);
            if (image != NULL) {
                int x;
                int y;
                int width = image->width;
                int height = image->height;
                uword* src = (uword*)image->mem;
                unsigned int* dst = (unsigned int*)image24->mem;
                for (y = 0; y < height; y++) {
                    for (x = 0; x < width; x++) {
                        int pixel = src[y * width + x];
                        dst[y * width + x] = ((pixel & 0xf800) << 8) + ((pixel & 0x7e0) << 5) + ((pixel & 0x1f) << 3);
                    }
                }
                gdk_draw_image(pixmap, gc, image24, 0, 0, 0, 0, image->width, image->height);
                gdk_image_destroy(image24);
            }
        } else
            gdk_draw_image(pixmap, gc, image, 0, 0, 0, 0, image->width, image->height);
        gdk_image_destroy(image);
        _m_apPortraitPixmap[portrait] = pixmap;
    }
    gdk_gc_unref(gc);
}

THeroPropsGeneralPage::~THeroPropsGeneralPage()
{
    for (unsigned int portrait = 0; portrait < kNumHeroBios; portrait++) {
        gdk_pixmap_unref(_m_apPortraitPixmap[portrait]);
        _m_apPortraitPixmap[portrait] = NULL;
    }
    delete[] _m_apPortraitPixmap;
    GtkRange* scroll = GTK_RANGE(_widget("hero_portrait_scroll"));
    GtkAdjustment* adj = gtk_range_get_adjustment(scroll);
    gtk_signal_disconnect_by_func(GTK_OBJECT(adj), GTK_SIGNAL_FUNC(onPortraitScrollValueChanged), NULL);
    pGeneralPage = NULL;
}

void THeroPropsGeneralPage::UpdateData(bool bSaveAndValidate)
{
    GtkRange* scroll = GTK_RANGE(_widget("hero_portrait_scroll"));
    GtkAdjustment* adj = gtk_range_get_adjustment(scroll);
    GtkCombo* patrolCombo = GTK_COMBO(_widget("hero_patrol_combo"));
    _m_name = gtk_entry_get_text(GTK_ENTRY(_widget("hero_name")));
    _m_experience = gtk_entry_get_text(GTK_ENTRY(_widget("hero_exp_combo_entry")));
    _m_patrol = getCurrentSelection(GTK_LIST(patrolCombo->list));
    _m_portrait = (int)adj->value;
    _m_bCustomName = isChecked("hero_name_customize");
    _m_bCustomPortrait = isChecked("hero_portrait_customize");
}

void THeroPropsGeneralPage::_setNewPortrait(int portrait)
{
    GtkPixmap* pPixmap = GTK_PIXMAP(_widget("hero_portrait"));
    gtk_pixmap_set(pPixmap, _m_apPortraitPixmap[portrait], NULL);
}

BOOL THeroPropsGeneralPage::OnInitDialog()
{
    _m_bModified = false;
    const THero::TClassTraits& classTraits = THero::s_akClassTraits[_m_hero.getClass()];
    _m_bNewCustomName = _m_hero.getBCustomName();
    _m_bNewCustomPortrait = _m_hero.getBCustomPortrait();
    _m_newName = _m_hero.getCustomName();
    _m_newPortrait = _m_hero.getCustomPortrait();
    _m_newExperience = _m_hero.getExperience();
    _m_newPatrol = _m_hero.getPatrol();
    _m_pPrototype = &classTraits.m_aPrototype[_m_hero.getProtoNum()];
    _m_bCustomName = _m_bNewCustomName;
    _m_bCustomPortrait = _m_bNewCustomPortrait;
    _m_customName = _m_newName;
    _m_name = _m_bCustomName ? _m_customName : _m_pPrototype->getName();
#line 376
    assert(!isAllSpace( (char *) _m_name.c_str() ));
    _m_bNameIsSpace = _isspace(_m_customName);
    _m_portrait = _m_newPortrait;
    _m_patrol = _m_newPatrol + 1;

    GtkToggleButton* pButton = GTK_TOGGLE_BUTTON(_widget("hero_name_customize"));
    gtk_signal_handler_block_by_func(GTK_OBJECT(pButton), GTK_SIGNAL_FUNC(on_hero_name_customize_toggled), NULL);
    gtk_toggle_button_set_active(pButton, _m_bCustomName ? TRUE : FALSE);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(pButton), GTK_SIGNAL_FUNC(on_hero_name_customize_toggled), NULL);
    pButton = GTK_TOGGLE_BUTTON(_widget("hero_portrait_customize"));
    gtk_signal_handler_block_by_func(GTK_OBJECT(pButton), GTK_SIGNAL_FUNC(on_hero_portrait_customize_toggled), NULL);
    gtk_toggle_button_set_active(pButton, _m_bCustomPortrait ? TRUE : FALSE);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(pButton), GTK_SIGNAL_FUNC(on_hero_portrait_customize_toggled), NULL);

    GtkEntry* pNameEntry = GTK_ENTRY(_widget("hero_name"));
    gtk_entry_set_max_length(pNameEntry, 12);
    gtk_entry_set_text(pNameEntry, _m_name.c_str());
    gtk_widget_set_sensitive(GTK_WIDGET(pNameEntry), _m_bCustomName ? TRUE : FALSE);

    static const int akLevelExperience[] = {
        0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500,
    };
    GList* items = NULL;
    for (unsigned int level = 0; level < sizeof(akLevelExperience) / sizeof(akLevelExperience[0]); level++) {
        char text[50];
        snprintf(text, 50, "%d", akLevelExperience[level]);
        gchar* str = (gchar*)g_malloc(strlen(text) + 1);
#line 430
        assert(str != NULL);
        strcpy(str, text);
        items = g_list_append(items, str);
    }
    GtkCombo* pCombo = GTK_COMBO(_widget("hero_exp_combo"));
    emptyList(GTK_LIST(pCombo->list));
    gtk_combo_set_popdown_strings(pCombo, items);
    items = NULL;
    GtkEntry* pExpEntry = GTK_ENTRY(_widget("hero_exp_combo_entry"));
    gtk_entry_set_max_length(pExpEntry, TDigits<99999999>::getDigits());
    char text[50];
    snprintf(text, 50, "%d", _m_newExperience);
    gtk_entry_set_text(pExpEntry, text);

    _setNewPortrait(_m_bCustomPortrait ? _m_portrait : _m_pPrototype->getPortrait());
    GtkRange* scroll = GTK_RANGE(_widget("hero_portrait_scroll"));
    GtkAdjustment* adj = gtk_range_get_adjustment(scroll);
    adj->lower = 0;
    adj->upper = kNumHeroBios - 1;
    gtk_signal_connect(GTK_OBJECT(adj), "value-changed", GTK_SIGNAL_FUNC(onPortraitScrollValueChanged), NULL);
    gtk_widget_set_sensitive(GTK_WIDGET(scroll), _m_bCustomPortrait ? TRUE : FALSE);
    if (_m_bCustomPortrait) {
        adj->value = _m_portrait;
        gtk_adjustment_changed(adj);
        gtk_adjustment_value_changed(adj);
    }

    static const char* akPatrolStr[] = {
        kSelNoneStr, kStandStillStr, kRadiusOneSquareStr, kRadiusTwoSquaresStr, kRadiusThreeSquaresStr,
        kRadiusFourSquaresStr, kRadiusFiveSquaresStr, kRadiusSixSquaresStr, kRadiusSevenSquaresStr,
        kRadiusEightSquaresStr, kRadiusNineSquaresStr, kRadiusTenSquaresStr,
    };
    pCombo = GTK_COMBO(_widget("hero_patrol_combo"));
    emptyList(GTK_LIST(pCombo->list));
    items = NULL;
    for (unsigned int patrol = 0; patrol < sizeof(akPatrolStr) / sizeof(akPatrolStr[0]); patrol++)
        items = g_list_append(items, (gpointer)akPatrolStr[patrol]);
    gtk_combo_set_popdown_strings(pCombo, items);
    setCurrentSelection(GTK_LIST(pCombo->list), _m_newPatrol + 1);
    return true;
}

void THeroPropsGeneralPage::OnOK()
{
#line 555
    assert(!_m_bCustomName || !_m_bNameIsSpace);
    UpdateData(false);
    _m_bNewCustomName = _m_bCustomName != FALSE;
    _m_bNewCustomPortrait = _m_bCustomPortrait != FALSE;
    int experience = 0;
    sscanf(_m_experience.c_str(), "%d", &experience);
    _m_newExperience = experience;
    _m_newName = _m_customName.c_str();
    _m_newPortrait = _m_portrait;
    _m_newPatrol = _m_patrol - 1;
    _m_bModified = _m_bModified || _m_bNewCustomName != _m_hero.getBCustomName()
                   || _m_bNewCustomPortrait != _m_hero.getBCustomPortrait() || _m_newName != _m_hero.getCustomName()
                   || _m_newPortrait != _m_hero.getCustomPortrait() || _m_newExperience != _m_hero.getExperience()
                   || _m_newPatrol != _m_hero.getPatrol();
}

void THeroPropsGeneralPage::OnChangeNameEdit()
{
#line 609
    assert(_m_bCustomName);
    UpdateData(false);
    _m_customName = _m_name;
    if (!isAllSpace(_m_customName.c_str())) {
        if (_m_bNameIsSpace) {
            _m_pParentSheet->onEnableOK();
            _m_bNameIsSpace = false;
        }
    } else if (!_m_bNameIsSpace) {
        _m_pParentSheet->onDisableOK();
        _m_bNameIsSpace = true;
    }
}

void THeroPropsGeneralPage::OnCustomizeNameCheck()
{
    UpdateData(false);
    if (_m_bCustomName) {
        _m_name = _m_customName;
        gtk_widget_set_sensitive(_widget("hero_name"), TRUE);
        if (_m_bNameIsSpace)
            _m_pParentSheet->onDisableOK();
    } else {
        _m_name = _m_pPrototype->getName().c_str();
        gtk_widget_set_sensitive(_widget("hero_name"), FALSE);
        if (_m_bNameIsSpace)
            _m_pParentSheet->onEnableOK();
    }
    UpdateData(false);
}

void THeroPropsGeneralPage::OnCustomizePortraitCheck()
{
    UpdateData(false);
    GtkWidget* scroll = _widget("hero_portrait_scroll");
    GtkAdjustment* adj = gtk_range_get_adjustment(GTK_RANGE(scroll));
#line 663
    assert(adj != NULL);
    if (_m_bCustomPortrait) {
        gtk_widget_set_sensitive(scroll, TRUE);
        adj->value = _m_portrait;
        gtk_adjustment_value_changed(adj);
    } else {
        adj->value = _m_pPrototype->getPortrait();
        gtk_adjustment_value_changed(adj);
        gtk_widget_set_sensitive(scroll, FALSE);
    }
}

TRandomHeroPropsGeneralPage::TRandomHeroPropsGeneralPage(THeroPropsGeneralPageParentSheet* pParentSheet,
                                                         const TRandomHero& hero,
                                                         const bitset<kNumPlayers>& availableOwners)
    : THeroPropsGeneralPage(0, pParentSheet, hero),
      _m_hero(hero),
      _m_availableOwnersMask(availableOwners)
{
    _m_availableOwnersMask[hero.getOwner()] = true;
}

BOOL TRandomHeroPropsGeneralPage::OnInitDialog()
{
    _m_owner = _m_hero.getOwner();
    BOOL result = THeroPropsGeneralPage::OnInitDialog();
    unsigned int player;
    unsigned int cbIndex = 0;
    GList* glist = NULL;
    for (player = 0; player < kNumPlayers; player++) {
        if (_m_availableOwnersMask[player]) {
            glist = g_list_append(glist, (gpointer)akPlayerTraits[player].m_pName);
            _m_playerItemData[cbIndex] = player;
            cbIndex++;
        }
    }
    for (cbIndex = 0; _m_playerItemData[cbIndex] != _m_owner; cbIndex++) {
#line 737
        assert(cbIndex < g_list_length(glist));
    }
    GtkCombo* pCombo = GTK_COMBO(_widget("hero_player_combo"));
    emptyList(GTK_LIST(pCombo->list));
    gtk_combo_set_popdown_strings(pCombo, glist);
    setCurrentSelection(GTK_LIST(pCombo->list), cbIndex);
    return result;
}

void TRandomHeroPropsGeneralPage::OnOK()
{
    THeroPropsGeneralPage::OnOK();
    GtkCombo* pCombo = GTK_COMBO(_widget("hero_player_combo"));
    int curSel = getCurrentSelection(GTK_LIST(pCombo->list));
#line 755
    assert(curSel != -1);
    _m_owner = TPlayer(_m_playerItemData[curSel]);
#line 757
    assert(_m_owner >= 0 && _m_owner < kNumPlayers);
    assert(_m_availableOwnersMask[ _m_owner ]);
    _m_bModified = _m_bModified || _m_owner != _m_hero.getOwner();
}

TNonRandomHeroPropsGeneralPage::TNonRandomHeroPropsGeneralPage(TNonRandomHeroPropsGeneralPageParentSheet* pParentSheet,
                                                               const TNonRandomHero& hero,
                                                               const bitset<kNumPlayers>& availableOwners,
                                                               const set<unsigned int>& availableProtoNums)
    : THeroPropsGeneralPage(0, pParentSheet, hero),
      _m_pParentSheet(pParentSheet),
      _m_hero(hero),
      _m_availableOwnersMask(availableOwners),
      _m_availableProtoNums(availableProtoNums)
{
#line 778
    assert(_m_pParentSheet != NULL);
    assert(_m_availableProtoNums.find( _m_hero.getProtoNum() ) == _m_availableProtoNums.end());
    _m_availableOwnersMask[hero.getOwner()] = true;
    _m_availableProtoNums.insert(_m_hero.getProtoNum());
}

void TNonRandomHeroPropsGeneralPage::setProtoNum(unsigned int newProtoNum)
{
    GtkCombo* pCombo = GTK_COMBO(_widget("hero_identity_combo"));
    int curSel = getCurrentSelection(GTK_LIST(pCombo->list));
#line 793
    assert(curSel != -1);
    assert(_m_identityItemData[curSel] == newProtoNum);
    _m_protoNum = newProtoNum;
    _m_pPrototype = &_m_hero.getClassTraits().m_aPrototype[_m_protoNum];
    if (!_m_bCustomName) {
        UpdateData(false);
        _m_name = _m_pPrototype->getName().c_str();
        UpdateData(false);
    }
    if (!_m_bCustomPortrait) {
        GtkRange* scroll = GTK_RANGE(_widget("hero_portrait_scroll"));
        GtkAdjustment* adj = gtk_range_get_adjustment(scroll);
        adj->value = _m_pPrototype->getPortrait();
        gtk_adjustment_value_changed(adj);
    }
}

BOOL TNonRandomHeroPropsGeneralPage::OnInitDialog()
{
    const THero::TClassTraits& classTraits = _m_hero.getClassTraits();
    _m_className = classTraits.m_name;
    _m_owner = _m_hero.getOwner();
    _m_protoNum = _m_hero.getProtoNum();
    BOOL result = THeroPropsGeneralPage::OnInitDialog();
    GList* glist = NULL;
    unsigned int cbIndex = 0;
    for (unsigned int player = 0; player < kNumPlayers; player++) {
        if (_m_availableOwnersMask[player]) {
            glist = g_list_append(glist, (gpointer)akPlayerTraits[player].m_pName);
            _m_playerItemData[cbIndex] = player;
            cbIndex++;
        }
    }
    GtkCombo* combo = GTK_COMBO(_widget("hero_player_combo"));
    emptyList(GTK_LIST(combo->list));
    gtk_combo_set_popdown_strings(combo, glist);
    for (cbIndex = 0; _m_playerItemData[cbIndex] != _m_owner; cbIndex++) {
#line 868
        assert(cbIndex < g_list_length(GTK_LIST(combo->list)->children));
    }
    setCurrentSelection(GTK_LIST(combo->list), cbIndex);
    glist = NULL;
    cbIndex = 0;
    combo = GTK_COMBO(_widget("hero_identity_combo"));
    int curSel = -1;
    for (set<unsigned int>::const_iterator iter = _m_availableProtoNums.begin(); iter != _m_availableProtoNums.end();
         ++iter) {
        glist = g_list_append(glist, (gpointer)classTraits.m_aPrototype[*iter].getName().c_str());
        _m_identityItemData[cbIndex] = *iter;
        cbIndex++;
        if (*iter == _m_protoNum)
            curSel = cbIndex;
    }
    gtk_combo_set_popdown_strings(combo, glist);
    if (curSel != -1)
        setCurrentSelection(GTK_LIST(combo->list), curSel - 1);
#line 900
    assert(getCurrentSelection(GTK_LIST(combo->list)) != -1);
    return result;
}

void TNonRandomHeroPropsGeneralPage::OnOK()
{
    THeroPropsGeneralPage::OnOK();
    GtkCombo* pCombo = GTK_COMBO(_widget("hero_player_combo"));
    int curSel = getCurrentSelection(GTK_LIST(pCombo->list));
#line 911
    assert(curSel != -1);
    _m_owner = TPlayer(_m_playerItemData[curSel]);
#line 913
    assert(_m_owner >= 0 && _m_owner < kNumPlayers);
    assert(_m_availableOwnersMask[ _m_owner ]);
    _m_bModified = _m_bModified || _m_owner != _m_hero.getOwner() || _m_protoNum != _m_hero.getProtoNum();
}

void TNonRandomHeroPropsGeneralPage::OnSelChangeIdentityCombo()
{
    GtkCombo* pCombo = GTK_COMBO(_widget("hero_player_combo"));
    int curSel = getCurrentSelection(GTK_LIST(pCombo->list));
#line 926
    assert(curSel != -1);
    unsigned int newProtoNum = _m_identityItemData[curSel];
#line 928
    assert(newProtoNum < _m_hero.getClassTraits().m_numPrototypes);
    if (newProtoNum != _m_protoNum)
        _m_pParentSheet->onSetNewProtoNum(newProtoNum);
#line 931
    assert(_m_protoNum == newProtoNum);
}

TPrisonPropsGeneralPage::TPrisonPropsGeneralPage(TPrisonPropsGeneralPageParentSheet* pParentSheet,
                                                 const TPrison& prison,
                                                 const TArray<set<unsigned int>, kNumHeroClasses>& aAvailableProtoNums)
    : THeroPropsGeneralPage(0, pParentSheet, prison),
      _m_pParentSheet(pParentSheet),
      _m_prison(prison),
      _m_aAvailableProtoNums(aAvailableProtoNums)
{
#line 947
    assert(_m_pParentSheet != NULL);
#line 949
    assert(_m_aAvailableProtoNums[ _m_prison.getClass() ].find( _m_prison.getProtoNum() ) == _m_aAvailableProtoNums[ _m_prison.getClass() ].end());
    _m_aAvailableProtoNums[_m_prison.getClass()].insert(_m_prison.getProtoNum());
}

void TPrisonPropsGeneralPage::setIdentity(THeroClass newHeroClass, unsigned int newProtoNum)
{
    int curSel;
    GtkCombo* pCombo = GTK_COMBO(_widget("hero_player_combo"));
    curSel = getCurrentSelection(GTK_LIST(pCombo->list));
#line 965
    assert(curSel != -1);
    assert(_m_classItemData[curSel] == newHeroClass);
    pCombo = GTK_COMBO(_widget("hero_identity_combo"));
    curSel = getCurrentSelection(GTK_LIST(pCombo->list));
#line 970
    assert(curSel != -1);
    assert(_m_identityItemData[curSel] == newProtoNum);
    _m_heroClass = newHeroClass;
    _m_protoNum = newProtoNum;
    _m_pPrototype = &THero::s_akClassTraits[_m_heroClass].m_aPrototype[_m_protoNum];
    if (!_m_bCustomName) {
        UpdateData(false);
        _m_name = _m_pPrototype->getName().c_str();
        UpdateData(false);
    }
    if (!_m_bCustomPortrait) {
        GtkWidget* scroll = _widget("hero_portrait_scroll");
        GtkAdjustment* adj = gtk_range_get_adjustment(GTK_RANGE(scroll));
#line 989
        assert(adj != NULL);
        adj->value = _m_pPrototype->getPortrait();
        gtk_adjustment_value_changed(adj);
    }
}

BOOL TPrisonPropsGeneralPage::OnInitDialog()
{
    _m_heroClass = _m_prison.getClass();
    _m_protoNum = _m_prison.getProtoNum();
    BOOL result = THeroPropsGeneralPage::OnInitDialog();
#line 1022
    assert(0);
    return result;
}

void TPrisonPropsGeneralPage::OnOK()
{
}

void TPrisonPropsGeneralPage::OnSelChangeClassCombo()
{
}

void TPrisonPropsGeneralPage::OnSelChangeIdentityCombo()
{
}
