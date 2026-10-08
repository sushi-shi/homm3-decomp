// MapSpecsGeneralPage.cpp - Loki h3maped object 70: the general page of the
// map specifications sheet over the map_specs glade tree. OnInitDialog
// fills the name entry (30 characters), the description text, the
// difficulty radios and the two-level check from the map; OnOK reads them
// back (at most 300 characters of description) and records whether
// anything differs from the map. The assert lines come from the retail
// immediates.
#include "editor/stdafx.h"

#include <string>

namespace {
#include <gtk/gtk.h>
}

#include "editor/MapSpecsGeneralPage.h"

TMapSpecsGeneralPage::TMapSpecsGeneralPage(const TGameMap& map)
    : _m_map(map),
      _m_intDifficulty(map.getDifficulty()),
      _m_bTwoLevelMap(map.isTwoLayer()),
      _m_name(map.getName()),
      _m_desc(map.getDesc())
{
    _m_bModified = false;
    _m_difficultyRadio = -1;
    _m_descEdit = "";
    _m_nameEdit = "";
    _m_bTwoLevelMapCheck = false;
}

TMapSpecsGeneralPage::~TMapSpecsGeneralPage()
{
}

static bool isRadioActive(char* name)
{
    GtkToggleButton* pButton = GTK_TOGGLE_BUTTON(_widget(name));
    return gtk_toggle_button_get_active(pButton);
}

static void activateRadio(char* name)
{
    GtkToggleButton* pButton = GTK_TOGGLE_BUTTON(_widget(name));
    gtk_toggle_button_set_active(pButton, TRUE);
}

BOOL TMapSpecsGeneralPage::OnInitDialog()
{
    _m_bModified = false;
    _m_difficultyRadio = _m_intDifficulty;
    _m_bTwoLevelMapCheck = _m_bTwoLevelMap;
    _m_nameEdit = _m_name.c_str();
    _m_descEdit = _m_desc.c_str();

    if (_m_intDifficulty == 0)
        activateRadio("easy_diff");
    else if (_m_intDifficulty == 1)
        activateRadio("normal_diff");
    else if (_m_intDifficulty == 2)
        activateRadio("hard_diff");
    else if (_m_intDifficulty == 3)
        activateRadio("expert_diff");
    else if (_m_intDifficulty == 4)
        activateRadio("impossible_diff");
    else {
#line 149
        assert(0);
    }

    GtkEntry* pNameEntry = GTK_ENTRY(_widget("mapspecs_gen_name"));
    pNameEntry->text_max_length = 30;
    gtk_entry_set_text(pNameEntry, _m_nameEdit.c_str());

    GtkToggleButton* pTwoLevelCheck = GTK_TOGGLE_BUTTON(_widget("twolevelmap"));
    gtk_toggle_button_set_active(pTwoLevelCheck, _m_bTwoLevelMapCheck ? TRUE : FALSE);

    GtkText* pDescText = GTK_TEXT(_widget("mapspecs_gen_desc"));
    gtk_text_set_point(pDescText, 0);
    int length = gtk_text_get_length(pDescText);
    if (length > 0)
        gtk_editable_delete_text(GTK_EDITABLE(pDescText), 0, -1);
    gtk_text_insert(pDescText, NULL, NULL, NULL, _m_descEdit.c_str(), -1);
    gtk_text_set_point(pDescText, 0);
    return true;
}

void TMapSpecsGeneralPage::OnOK()
{
    GtkEntry* pNameEntry;
    GtkText* pDescText;
    GtkToggleButton* pTwoLevelCheck;

    if (isRadioActive("easy_diff"))
        _m_difficultyRadio = 0;
    else if (isRadioActive("normal_diff"))
        _m_difficultyRadio = 1;
    else if (isRadioActive("hard_diff"))
        _m_difficultyRadio = 2;
    else if (isRadioActive("expert_diff"))
        _m_difficultyRadio = 3;
    else if (isRadioActive("impossible_diff"))
        _m_difficultyRadio = 4;
    else {
#line 196
        assert(0);
    }

    _m_intDifficulty = TGameMap::TDifficulty(_m_difficultyRadio);
#line 200
    assert(_m_intDifficulty >= 0 && _m_intDifficulty < TGameMap::s_kNumDifficulties);

    pTwoLevelCheck = GTK_TOGGLE_BUTTON(_widget("twolevelmap"));
    _m_bTwoLevelMap = gtk_toggle_button_get_active(pTwoLevelCheck);

    pNameEntry = GTK_ENTRY(_widget("mapspecs_gen_name"));
    _m_name = gtk_entry_get_text(pNameEntry);

    pDescText = GTK_TEXT(_widget("mapspecs_gen_desc"));
    int length = gtk_text_get_length(pDescText);
    gchar* desc = gtk_editable_get_chars(GTK_EDITABLE(pDescText), 0, length > 300 ? 300 : length);
    _m_desc = desc;
    g_free(desc);

    _m_bModified = _m_bModified || _m_name != _m_map.getName() || _m_desc != _m_map.getDesc()
                   || _m_intDifficulty != _m_map.getDifficulty() || (_m_bTwoLevelMap != FALSE) != _m_map.isTwoLayer();
}

void TMapSpecsGeneralPage::OnApply()
{
    OnOK();
}
