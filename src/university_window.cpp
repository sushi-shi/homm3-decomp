#include "text.h"
#include "va.h"

#include <stdio.h>
#include <windows.h>

#include "university_window.h"

#include "border.h"
#include "button.h"
#include "game.h"
#include "hero.h"
#include "iconwdgt.h"
#include "kb.h"
#include "message.h"
#include "misc.h"
#include "sskilltraits.h"
#include "textresource.h"
#include "widget.h"
#include "winmgr.h"

// DC's two source-private POINT[4] globals retain their names in CodeView.
// Retail preserves the same interleaved x/y tables immediately before this
// TU's vtables; both arrays are read by the constructor's four-skill loop.
DATA(0x00643b60) static const POINT g_topBarPositions[4] = {
    {26, 212}, {130, 212}, {234, 212}, {338, 212}
};
DATA(0x00643b80) static const POINT g_buttonPositions[4] = {
    {54, 234}, {158, 234}, {261, 234}, {365, 234}
};

// DC public gUniversityWindowHelp is the two-row cancel/accept help table.
// Retail reads its four pointers consecutively at 0x6a7dd8..0x6a7de4.
DATA(0x006a7dd8) THelpText g_universityWindowHelp[2];

// Source-owned format selected by the constructor for the university's
// right-click description. The pointer is zero-fill storage until that setup.
DATA(0x006a7dec) static const char* g_universitySkillHelpFormat;

// E:\gamedcs\university_window.cpp:50. The two-store latch setter, and it
// belongs in this TU rather than the carcass: retail EXPANDS it at its one
// call site in skill_click (the call census reads `set_skill base x1 vs
// retail x0`), which /Ob2 can only do from a visible body.

void type_university_skill_button::setSkill(TSecondarySkill newSkill,
                                             unsigned char newClick)
{
    m_skill = newSkill;
    m_click = newClick;
}

// E:\gamedcs\university_window.cpp:65. Complete emits no surviving
// out-of-line body, but both allocations in the window constructor preserve
// this source helper in full: iconWidget base construction, derived vtable,
// then skill/click stores.

type_university_skill_button::type_university_skill_button(
    long x, long y, long width, long height, long newId,
    const char* image, TSecondarySkill newSkill)
    : iconWidget(x, y, width, height, newId, image,
                 newSkill * 3, 0, 0, 0, ICON_STYLE_PLAIN)
{
    m_skill = newSkill;
    m_click = 0;
}

// E:\gamedcs\university_window.cpp:75
// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x005ef490, 0x6A)  // vtable slot 13 + skill_click call, dc 0x18e728
bool type_university_skill_button::handleClick(
    bool downClick, bool rightClick)
{
    if (downClick) {
        if (rightClick) {
            normalDialog(g_sSkillTraits[m_skill].m_levelNames[0], 4,
                         -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            return 1;
        }

        if (m_click)
            return 1;

        static_cast<type_university_window*>(m_parentWindow)->skillClick(m_skill);
        return 1;
    }

    return 0;
}

// DC 124/133/236 use TTextResource::operator[] for all four text lookups.
// E:\gamedcs\university_window.cpp:102. Exact with the original helper
// calls and recovered pointer lifetimes. DC records widget* new_widget;
// the typed background/exit/cancel locals below are inferred from retail's
// derived-to-base argument temporaries, not recovered local names. Direct
// skill-array access restores the cancel hotkey's vector<int>::insert
// expansion without changing the canonical setHotkey/push_back boundary.
// DC 147 and retail both reread university->skills for the skill-button
// constructor. DC 199..201 keeps the second background in new_widget and
// calls push_back twice; unlike 187/188, 247/248 and 255/256, there is no
// back() call at this site. Complete adds the townUniversity image arm;
// its third parameter is proven by both retail callers and the branch.
//
// Negative controls: the 54-state access/lifetime family leaves cached
// record references/pointers at 88.1141..88.1533%, versus 95.1581..95.2402%
// for direct indexing (four objects, four reproduced). The next 54-state
// typed-local family reaches 98.9215% (18 objects, ten reproduced). In the
// final 33-state family, reading the stored skill stays at 99.5480% and
// using back() for the second background at 99.3736%; restoring both is
// exact for all eight tested scope/declaration combinations (five objects,
// five reproduced). All other TU functions stay unchanged in these tests.
// The apparent +4 coordinate-load differences are the same addresses:
// g_topBarPositions+4 and retail const_243b64. All 154 calls agree after
// verifying the six folded vector-helper aliases against their retail bodies.
VA(0x005ef500, 0x1252)  // Univers1.pcx + two call-site modes, dc 0x18e790
type_university_window::type_university_window(
    hero* newHero, const type_university* university,
    unsigned char townUniversity)
    : CAdvPopup(0, 0, 800, 600, 0)
{
    long widgetId = 100;

    m_currentHero = newHero;
    m_x = 135;
    m_y = 206;
    m_width = 465;
    m_height = 388;
    m_type = 18;

    bitmapBorder* background = new bitmapBorder(
        0, 0, 465, 388, widgetId++, "univers1.pcx", 0x800);
    background->setPlayerPaletteColors(
        g_game->getLocalPlayerGamePos());
    m_widgets.push_back(background);
    m_selectionWidgets.push_back(background);

    widget* newWidget = new textWidget(
        21, 16, 422, 20, (*g_generalText)[603], "medfont.fnt",
        font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(newWidget);
    m_selectionWidgets.push_back(newWidget);

    if (townUniversity) {
        newWidget = new iconWidget(
            157, 41, 150, 70, widgetId++, "hallelem.def", 21,
            0, 0, 0, iconWidget::ICON_STYLE_PLAIN);
    } else {
        newWidget = new bitmapBorder(
            175, 36, 114, 80, widgetId++, "univbldg.pcx", 0x800);
    }
    m_widgets.push_back(newWidget);
    m_selectionWidgets.push_back(newWidget);

    newWidget = new textWidget(
        27, 129, 410, 70, (*g_generalText)[604], "smalfont.fnt",
        font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(newWidget);
    m_selectionWidgets.push_back(newWidget);

    for (int i = 0; i < 4; ++i) {
        m_skills[i].m_skill = university->m_skills[i];

        m_skills[i].m_button = new type_university_skill_button(
            g_buttonPositions[i].x, g_buttonPositions[i].y, 44, 44,
            widgetId++, "SecSkill.def", university->m_skills[i]);
        m_widgets.push_back(m_skills[i].m_button);
        m_selectionWidgets.push_back(m_skills[i].m_button);

        m_skills[i].m_topBar = new iconWidget(
            g_topBarPositions[i].x, g_topBarPositions[i].y, 100, 18,
            widgetId++, "Univcolr.def", 0, 0, 0, 0,
            iconWidget::ICON_STYLE_PLAIN);
        m_widgets.push_back(m_skills[i].m_topBar);
        m_selectionWidgets.push_back(m_skills[i].m_topBar);

        m_skills[i].m_bottomBar = new iconWidget(
            g_topBarPositions[i].x, g_topBarPositions[i].y + 70, 100, 18,
            widgetId++, "Univcolr.def", 0, 0, 0, 0,
            iconWidget::ICON_STYLE_PLAIN);
        m_widgets.push_back(m_skills[i].m_bottomBar);
        m_selectionWidgets.push_back(m_skills[i].m_bottomBar);

        m_skills[i].m_textWidget = new textWidget(
            m_skills[i].m_topBar->m_x, m_skills[i].m_topBar->m_y,
            m_skills[i].m_topBar->m_width, m_skills[i].m_topBar->m_height,
            g_sSkillTraits[m_skills[i].m_skill].m_name, "smalfont.fnt",
            font::PRIMARY, -1, 1, 0, 8);
        m_widgets.push_back(m_skills[i].m_textWidget);
        m_selectionWidgets.push_back(m_skills[i].m_textWidget);

        newWidget = new textWidget(
            m_skills[i].m_bottomBar->m_x, m_skills[i].m_bottomBar->m_y,
            m_skills[i].m_bottomBar->m_width, m_skills[i].m_bottomBar->m_height,
            g_secondarySkillLevels[0], "smalfont.fnt",
            font::PRIMARY, -1, 1, 0, 8);
        m_widgets.push_back(newWidget);
        m_selectionWidgets.push_back(newWidget);
    }

    m_widgets.push_back(new bitmapBorder(
        199, 312, 66, 32, -1, "box64x30.pcx", 0x800));
    m_selectionWidgets.push_back(m_widgets.back());

    type_func_button* exitButton = new type_func_button(
        200, 313, 64, 30, widgetId++, "iOkay.def",
        exitClick, 0, 1);
    exitButton->setHotkey(1);
    exitButton->setHotkey(28);
    exitButton->setHelpText(g_universityWindowHelp[1].m_text, 0, 1);
    m_widgets.push_back(exitButton);
    m_selectionWidgets.push_back(exitButton);

    newWidget = new bitmapBorder(
        0, 0, 465, 388, widgetId++, "univers2.pcx", 0x800);
    m_widgets.push_back(newWidget);
    m_purchaseWidgets.push_back(newWidget);

    m_selectedSkill.m_topBar = 0;
    m_selectedSkill.m_textWidget = new textWidget(
        179, 27, 100, 16, "", "smalfont.fnt",
        font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_selectedSkill.m_textWidget);
    m_purchaseWidgets.push_back(m_selectedSkill.m_textWidget);

    m_selectedSkill.m_button = new type_university_skill_button(
        210, 50, 44, 44, widgetId++, "SecSkill.def", kNumSecSkills);
    m_widgets.push_back(m_selectedSkill.m_button);
    m_purchaseWidgets.push_back(m_selectedSkill.m_button);
    m_selectedSkill.m_bottomBar = 0;

    newWidget = new textWidget(
        179, 97, 100, 16, g_secondarySkillLevels[0], "smalfont.fnt",
        font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(newWidget);
    m_purchaseWidgets.push_back(newWidget);

    m_purchaseTextWidget = new textWidget(
        28, 133, 409, 67, "", "smalfont.fnt",
        font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_purchaseTextWidget);
    m_purchaseWidgets.push_back(m_purchaseTextWidget);

    iconWidget* resourceIcon = new iconWidget(
        216, 208, 32, 32, widgetId++, "resource.def", 0,
        0, 0, 0, iconWidget::ICON_STYLE_PLAIN);
    resourceIcon->setIconFrame(6);
    resourceIcon->setHelpText(
        (*g_generalText)[626], (*g_generalText)[243], 1);
    m_widgets.push_back(resourceIcon);
    m_purchaseWidgets.push_back(resourceIcon);

    sprintf(g_text, "%d", TUITION);
    newWidget = new textWidget(
        196, 259, 69, 17, g_text, "smalfont.fnt", font::PRIMARY,
        widgetId++, 1, 0, 8);
    m_widgets.push_back(newWidget);
    m_purchaseWidgets.push_back(newWidget);

    m_widgets.push_back(new bitmapBorder(
        147, 299, 66, 34, -1, "box64x32.pcx", 0x800));
    m_purchaseWidgets.push_back(m_widgets.back());

    m_purchaseButton = new type_func_button(
        148, 300, 64, 32, widgetId++, "iBuy30.def",
        purchaseClick, 0, 1);
    m_widgets.push_back(m_purchaseButton);
    m_purchaseWidgets.push_back(m_purchaseButton);

    m_widgets.push_back(new bitmapBorder(
        251, 299, 66, 34, -1, "box64x32.pcx", 0x800));
    m_purchaseWidgets.push_back(m_widgets.back());

    type_func_button* cancelButton = new type_func_button(
        252, 300, 64, 32, widgetId++, "iCancel.def",
        cancelClick, 0, 1);
    cancelButton->setHelpText(g_universityWindowHelp[0].m_text, 0, 1);
    cancelButton->setHotkey(1);
    m_widgets.push_back(cancelButton);
    m_purchaseWidgets.push_back(cancelButton);

    m_rolloverWidget = new textWidget(
        8, 362, 448, 18, "", "smalfont.fnt",
        font::PRIMARY, widgetId, 1, 0, 8);
    m_widgets.push_back(m_rolloverWidget);

    for (std::vector<widget*>::iterator it = m_widgets.begin();
         it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }
}

// Dreamcast preserves this helper as a separate source function. Complete
// inlines it into DoModal and cancel_click; retaining the boundary is required
// even though no standalone x86 body survives.

void type_university_window::setSelectionMode()
{
    int i;
    for (i = 0; i < m_purchaseWidgets.size(); ++i)
        m_purchaseWidgets[i]->sendMessage(
            widget::WIDGET_CLEAR_STATUS, widget::WIDGET_CLEAR_STATUS);
    for (i = 0; i < m_selectionWidgets.size(); ++i)
        m_selectionWidgets[i]->sendMessage(
            widget::WIDGET_SET_STATUS, widget::WIDGET_CLEAR_STATUS);

    for (i = 0; i < 4; ++i)
        updateSkillButton(m_skills[i]);
    m_selectedSkill.m_skill = eSecSkillNone;
}

VA_COMPGEN(0x005f0760, 0x21, SCALAR_DELETING_DTOR, type_university_window)

VA_COMPGEN(0x005ef460, 0x21, SCALAR_DELETING_DTOR, type_university_skill_button)

VA(0x005f0790, 0x284)  // dc 0x18f2f8
void type_university_window::updateSkillButton(type_university_skill& skill)
{
    std::string text;

    if (m_currentHero->m_skillLevel[skill.m_skill] > 0) {
        text = (*g_generalText)[605];
        skill.m_topBar->setIconFrame(0);
        skill.m_bottomBar->setIconFrame(0);
    } else if (g_heroClasses[m_currentHero->m_heroClass]
                   .m_gainSecondarySkillChance[skill.m_skill] == 0) {
        text = (*g_generalText)[606];
        skill.m_topBar->setIconFrame(2);
        skill.m_bottomBar->setIconFrame(2);
    } else if (m_currentHero->m_skillCount == MAX_SECONDARY_SKILLS) {
        text = (*g_generalText)[607];
        skill.m_topBar->setIconFrame(2);
        skill.m_bottomBar->setIconFrame(2);
    } else {
        text = formatString((*g_generalText)[608], g_secondarySkillLevels[0],
                             g_sSkillTraits[skill.m_skill].m_name, TUITION);
        if (skill.m_topBar)
            skill.m_topBar->setIconFrame(1);
        if (skill.m_bottomBar)
            skill.m_bottomBar->setIconFrame(1);
    }
    skill.m_button->setIconFrame((skill.m_skill + 1) * 3);
    skill.m_button->setHelpText(text.c_str(), 0, 1);
}

VA(0x005f0a20, 0x92)  // dc 0x18f508

void type_university_window::doModal(bool fade)
{
    setSelectionMode();
    heroWindow::doModal(fade);
}

VA(0x005f0ac0, 0x2F2)  // dc 0x18f52c
void type_university_window::skillClick(TSecondarySkill skill)
{
    std::string helpText;
    unsigned int i;

    if (m_currentHero->m_skillLevel[skill] > 0) {
        normalDialog((*g_generalText)[605], NORMAL_DIALOG_DEFAULT, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }
    if (g_heroClasses[m_currentHero->m_heroClass]
            .m_gainSecondarySkillChance[skill] == 0) {
        normalDialog((*g_generalText)[606], NORMAL_DIALOG_DEFAULT, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }
    if (m_currentHero->m_skillCount == MAX_SECONDARY_SKILLS) {
        normalDialog((*g_generalText)[607], NORMAL_DIALOG_DEFAULT, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    for (i = 0; i < m_selectionWidgets.size(); i++)
        m_selectionWidgets[i]->hide();
    for (i = 0; i < m_purchaseWidgets.size(); i++)
        m_purchaseWidgets[i]->show();

    std::string newText;
    const char* skillName = g_sSkillTraits[skill].m_name;

    newText = formatString((*g_generalText)[609], g_secondarySkillLevels[0],
                             skillName, TUITION);
    m_selectedSkill.m_skill = skill;
    m_selectedSkill.m_button->setSkill(skill, 1);
    m_selectedSkill.m_textWidget->setText(skillName);
    m_purchaseTextWidget->setText(newText.c_str());
    newText = formatString((*g_generalText)[610], skillName);
    m_purchaseButton->setHelpText(newText.c_str(), 0, 1);
    updateSkillButton(m_selectedSkill);
    m_purchaseButton->enable(
        m_currentHero->getPlayer()->m_resources[GOLD] >= TUITION);
    drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}
// Earlier skillClick controls at 98.3269%: naming a button pointer was
// byte-flat and moving the selected-skill store below setSkill fell to
// 98.2560%. Retaining the named skill string and canonical setSkill/show/
// hide calls is exact in the current TU; the old register-only diagnosis
// no longer describes this object.

// E:\gamedcs\university_window.cpp:404
// DC 405/407/408/411 proves the base-handler result guard followed by
// ConvertToHover for mouse movement. Retail's university vtable 0x643bd8
// slot 9 points to 0x5666f0, the identical body owned by
// type_skeleton_window::windowHandler; inheriting CAdvPopup's 0x41b1c0
// handler loses this derived hover step. Retain the separate source
// override without claiming the folded retail address twice.

int type_university_window::windowHandler(message& msg)
{
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;
    if (msg.m_id == MESSAGE_MOUSE_MOVE)
        return g_windowManager->convertToHover(msg);
    return 0;
}

// Identified by shape: heroWindow slot 4 (one widget* argument, `ret 4`),
// reading widget::RollOver at +0x20 and dispatching slot 13
// (textWidget::SetText) through the +0x70 rollover pointer before a
// whole-window redraw.
VA(0x005f0dc0, 0x38)  // dc 0x18f868
void type_university_window::handleWidgetHover(widget* currentWidget)
{
    // DC 420 obtains the help pointer once before the 422..425 arms.
    const char* helpText = currentWidget->getHelpText();
    if (!helpText)
        m_rolloverWidget->setText("");
    else
        m_rolloverWidget->setText(helpText);
    drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

VA(0x005f0e00, 0xE4)  // dc 0x18f8b0
int type_university_window::cancelClick(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(g_universityWindowHelp[0].m_rclick, 4,
                     -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return 1;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_university_window* window =
            static_cast<type_university_window*>(msg.m_window);
        window->setSelectionMode();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return 1;
    }
    return 0;
}

VA(0x005f0ef0, 0x65)  // dc 0x18f91c
int type_university_window::exitClick(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        normalDialog(g_universityWindowHelp[1].m_rclick, 4,
                     -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return 1;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = 0;
        msg.m_codeX = msg.m_codeY = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

// The address-taken callback immediately following handle_widget_hover is the
// purchase button's handler: right-select (14) formats the selected skill's
// Basic description and opens the canonical skill dialog; a plain deselect
// (13) pays 2,000 gold, teaches one level, restores the two widget groups,
// refreshes all four offer rows and clears the selection.

// Dreamcast CodeView records the branch-local `result` string and the shared
// set_selection_mode helper. Restoring that helper gives Complete's exact
// 28-block/15-branch CFG while keeping the positive source boundary intact.
// E:\gamedcs\university_window.cpp:479
VA(0x005f0f60, 0x21D)  // constructor callback xref + selected-skill tail, dc 0x18f97c
int type_university_window::purchaseClick(message& msg)
{
    if (msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
        // DC 482 constructs result before the window/skill reads at 484/486;
        // retail likewise initializes the string before loading msg.window.
        std::string result;
        type_university_window* window =
            static_cast<type_university_window*>(msg.m_window);
        int skill = window->m_selectedSkill.m_skill;
        const char* skillName = g_sSkillTraits[skill].m_name;
        result = formatString(
            g_universitySkillHelpFormat, g_secondarySkillLevels[0],
            skillName, 2000);
        normalDialog(result.c_str(), 4, -1, -1,
                     20, skill * 3 + 3, -1, 0, -1, 0, -1, 0);
        return 1;
    }

    if (msg.m_codeX == widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        type_university_window* window =
            static_cast<type_university_window*>(msg.m_window);
        playerData* player = window->m_currentHero->getPlayer();
        player->m_resources[6] -= 2000;
        window->m_currentHero->giveSS(window->m_selectedSkill.m_skill, 1);
        window->setSelectionMode();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return 1;
    }
    return 0;
}

// E:\gamedcs\university_window.cpp:515
VA(0x005f1180, 0x2D)  // link order + vtable slot 14, dc 0x18faa4
int type_university_window::exitDialog(message& msg)
{
    msg.m_id = MESSAGE_WIDGET;
    g_windowManager->m_dialogReturn = 0;
    msg.m_codeX = msg.m_codeY = 10;
    return MESSAGE_DISPATCH_FORWARD;
}
