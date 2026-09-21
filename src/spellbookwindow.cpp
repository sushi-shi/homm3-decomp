#include "va.h"

#include <algorithm>
#include <string.h>
#include <vector>

#include "spellbookwindow.h"

#include "armygrp.h"
#include "border.h"
#include "game.h"
#include "hero.h"
#include "iconwdgt.h"
#include "inputmgr.h"
#include "kb.h"
#include "mousemgr.h"
#include "prefs.h"
#include "resourcemanager.h"
#include "smackmgr.h"
#include "soundmgr.h"
#include "textresource.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// Source-private active-window slot. Retail's constructor stores `this` at
// 0x59be77 and the destructor clears the same address before widget teardown.
DATA(0x006a34ec) static TSpellbookWindow* g_spellbookWindow;

// Complete retains the DC static member and initializes it to the same
// sentinel used for every empty SpellMap slot.
DATA(0x00684b40) int TSpellbookWindow::s_lastPage = -1;
DATA(0x00684b44) TSpellbookWindow::TSpellContext
    TSpellbookWindow::s_lastContext = TSpellbookWindow::eContextInvalid;

// Complete places the current school in zero-fill storage immediately
// before get_level_string's five-pointer table.  The handler and constructor
// both retain it across spellbook instances.
DATA(0x006a34f4) TSpellSchool TSpellbookWindow::s_lastSchool;

// Dreamcast names this source-private rollover cache `lastIMHoverID`.
// Complete's sole two references are both in WindowHandler.
DATA(0x00684b4c) static int g_lastImHoverId = -1;

// Id of the hero used for the last spellbook instance.  The constructor
// resets the remembered view when the spellbook changes heroes.
DATA(0x00684b48) static int g_lastSpellbookHeroId = -1;

// Rollover/right-click pairs for the five school tabs, the two context tabs,
// the mana label, the page arrows, and the close button.  The constructor
// fills the zero-initialized rows from the spellbook text resource.
DATA(0x006a52d0) static THelpText g_spellbookHelpText[11];

// Dreamcast names this table `level_sprites`; the retail .rdata relocation
// run at 0x641d94 fixes both its order and its four literals.
DATA(0x00641d94) static const char* const g_levelSprites[] = {
    DATA_COMPGEN(0x00684b74, spellLevelAirSprite, "SPLEVA.def"),
    DATA_COMPGEN(0x00684b68, spellLevelFireSprite, "SPLEVF.def"),
    DATA_COMPGEN(0x00684b5c, spellLevelWaterSprite, "SPLEVW.def"),
    DATA_COMPGEN(0x00684b50, spellLevelEarthSprite, "SPLEVE.def")
};

// Complete indexes this four-pointer table directly, unlike the pointer
// form attested for Dreamcast's gSecondarySkillLevels.
DATA(0x006a5d48) const char* g_secondarySkillLevels[4];

// E:\gamedcs\spellbookwindow.cpp:82
int TSpellbookWindow::getPositionFromSchool(unsigned schoolMask)
{
    if (schoolMask == eSchoolAll)
        return 4;
    for (int position = 0; position < 4; ++position) {
        if (schoolMask & (1 << position))
            return position;
    }
    return 0;
}

// E:\gamedcs\spellbookwindow.cpp:103, dc 0x14d3cc.
inline TSpellSchool TSpellbookWindow::getSchoolFromPosition(int position)
{
    return position < 4 ? (TSpellSchool)(1 << position)
                        : eSchoolAll;
}

// Dreamcast retains this source-private helper out of line at dc 0x14bc80;
// Complete /Ob2 folds it into get_spell_description.  The five adjacent
// TextResource rows and the guarded pointer array are byte-proven by retail.
static const char* getLevelString(SpellID spell)
{
    static const char* levelStrings[] = {
        (*g_generalText)[173],
        (*g_generalText)[174],
        (*g_generalText)[175],
        (*g_generalText)[176],
        (*g_generalText)[177]
    };
    // DC117 initializes the five labels; DC119 reads the level, subtracts
    // one and indexes this table. Capture that actual index before lookup.
    // Across 24 caller/helper states (nine emitted objects), this binding
    // keeps all other scores and makes getSpellDescription's return copy
    // exact. Flattening it back into the subscript leaves that caller at
    // 96.8000% by expanding its final basic_string::_Tidy. A level value,
    // trait pointer or trait reference also reproduces the exact caller.
    int index = g_spellTraits[spell].m_level - 1;
    return levelStrings[index];
}

VA(0x0059ba80, 0x1D)  // dc 0x14bc58
void TSpellbookWindow::reset()
{
    s_lastSchool = const_invalid_school;
    s_lastPage = -1;
    s_lastContext = eContextInvalid;
    g_lastSpellbookHeroId = -1;
}

// E:\gamedcs\spellbookwindow.cpp:128
// Byte-exact with the ordinary getLevelString helper and its bound lookup
// index. Both helpers' work and the returned string remain canonical; no
// inline pin or extra substring is used. The previous flattened lookup
// made the return copy expand _Tidy while retail calls it. Its verified C2
// replay had root cb=388, budget=1000 and return-copy nested budget=152,
// exactly _Tidy's cost. Meaningful helper value bindings recover the
// caller boundary without changing its source or the other unit scores.
VA(0x0059baa0, 0x341)  // retail widens DC's magic-plains byte to the Complete magic-terrain field at +0x6c; dc 0x14bcf4
std::string TSpellbookWindow::getSpellDescription(
    SpellID spell, const hero* currentHero, unsigned char rollover)
{
    const SSpellTraits* traits = &g_spellTraits[spell];
    std::string result;
    int mastery = 0;
    if (currentHero)
        mastery = currentHero->getSpellLevel(
            spell, m_onMagicPlains);
    result = traits->m_levelDescriptions[mastery];

    if (rollover) {
        for (unsigned int i = 0; i < result.size(); ++i) {
            if (result[i] == '\n') {
                result = result.substr(0, i);
                break;
            }
        }
        result += " (";
        result += getLevelString(spell);
        result += ")";
    } else if (currentHero && (traits->m_flags & 0x200)) {
        int power = currentHero->getPrimarySkill(2);
        int damage = const_cast<hero*>(currentHero)->modifySpellDamage(
            spell,
            traits->m_powerFactor * power + traits->m_masteryBonus[mastery],
            0);
        sprintf(g_text, (*g_generalText)[344], damage);
        result += g_text;
    }
    return result;
}

// E:\gamedcs\spellbookwindow.cpp:180, dc 0x14be88
VA(0x0059bdf0, 0xAC9)  // anchor-bracket: immediately precedes scalar-del-dtor 0x59c8c0; EH, ret 0x10 = 4 args; spelback.pcx setup; dc 0x14be88
TSpellbookWindow::TSpellbookWindow(const hero& h, const armyGroup* g, TSpellbookWindow::TSpellContext context, int magicTerrain)
    : CAdvPopup(90, 2, 620, 595, 0x12),
      m_allowedContext(context),
      m_hero(&h),
      m_enemyGroup(g),
      m_onMagicPlains(magicTerrain)
{
    // DC181 calls del_Spr_from_Cache, a sprite-cache sweep with LOD
    // file-map and cached-frame operations (resourcemanager.cpp:2280..2350).
    // Complete has neither that work nor a call here: after the derived
    // vptr store at +0x4d it reads h's id directly. The canonical ordinary
    // helper belongs to resourcemanager.cpp, so retaining its external
    // call cannot reproduce this PC prologue by cross-TU auto-inlining.

    if (h.m_id != g_lastSpellbookHeroId) {
        s_lastPage = -1;
        s_lastSchool = (TSpellSchool)0;
        s_lastContext = eContextInvalid;
        g_lastSpellbookHeroId = m_hero->m_id;
    }
    g_spellbookWindow = this;

    m_widgets.reserve(51);

    {
        bitmapBorder* const background = new bitmapBorder(
            0, 0, 620, 595, BACKGROUND_ID,
            DATA_COMPGEN(0x00684bcc, spellbookBackground, "Spelback.pcx"),
            0x800);
        background->setPlayerPaletteColors(
            h.m_owner >= 0 ? h.m_owner : g_game->getLocalPlayerGamePos());
        m_widgets.push_back(background);
    }

    int spellLevelWidgetsIndex = m_widgets.size();
    int id = SPELL_LEVEL_0_ID;
    int row;
    int column;
    for (row = 89; row < 377; row += 96) {
        for (column = 117; column < 289; column += 86) {
            m_widgets.push_back(new iconWidget(
                column, row, 78, 65, id++, 0, 0, 0, 0, 0,
                iconWidget::ICON_STYLE_PLAIN));
        }
    }
    id = SPELL_LEVEL_6_ID;
    for (row = 89; row < 377; row += 96) {
        for (column = 333; column < 505; column += 86) {
            m_widgets.push_back(new iconWidget(
                column, row, 78, 65, id++, 0, 0, 0, 0, 0,
                iconWidget::ICON_STYLE_PLAIN));
        }
    }

    int spellIconWidgetsIndex = m_widgets.size();
    id = SPELL_0_ID;
    for (row = 89; row < 377; row += 96) {
        for (column = 117; column < 289; column += 86) {
            m_widgets.push_back(new iconWidget(
                column, row, 78, 65, id++,
                DATA_COMPGEN(0x00660208, m_spellIcons, "spells.def"),
                0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
        }
    }
    id = SPELL_6_ID;
    for (row = 89; row < 377; row += 96) {
        for (column = 333; column < 505; column += 86) {
            m_widgets.push_back(new iconWidget(
                column, row, 78, 65, id++,
                DATA_COMPGEN(0x00660208, m_spellIcons, "spells.def"),
                0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
        }
    }

    int spellNameWidgetsIndex = m_widgets.size();
    id = SPELL_0_NAME_ID;
    for (row = 154; row < 442; row += 96) {
        for (column = 113; column < 285; column += 86) {
            m_widgets.push_back(new textWidget(
                column, row, 86, 36, 0,
                DATA_COMPGEN(0x00660cb4, spellbookTinyFont, "tiny.fnt"),
                font::PRIMARY, id++,
                1, 0, 8));
        }
    }
    id = SPELL_6_NAME_ID;
    for (row = 154; row < 442; row += 96) {
        for (column = 329; column < 501; column += 86) {
            m_widgets.push_back(new textWidget(
                column, row, 86, 36, 0,
                DATA_COMPGEN(0x00660cb4, spellbookTinyFont, "tiny.fnt"),
                font::PRIMARY, id++,
                1, 0, 8));
        }
    }

    m_headingWidget = new iconWidget(
        117, 74, 160, 96, SCHOOL_HEADING_ID,
        DATA_COMPGEN(0x00684bc0, spellbookSchools, "schools.def"),
        0, 0, 0, 0,
        iconWidget::ICON_STYLE_PLAIN);
    m_widgets.push_back(m_headingWidget);

    m_schoolTabsWidget = new iconWidget(
        524, 88, 83, 294, SCHOOL_TABS_ID,
        DATA_COMPGEN(0x00684bb4, spellbookTabs, "SpelTab.def"),
        0, 0, 0, 0,
        iconWidget::ICON_STYLE_PLAIN);
    m_widgets.push_back(m_schoolTabsWidget);

    m_widgets.push_back(new border(523, 87, 83, 43, AIR_SCHOOL_ID, 1));
    m_widgets.push_back(new border(523, 205, 83, 43, FIRE_SCHOOL_ID, 1));
    m_widgets.push_back(new border(523, 268, 83, 43, WATER_SCHOOL_ID, 1));
    m_widgets.push_back(new border(523, 145, 83, 43, EARTH_SCHOOL_ID, 1));
    m_widgets.push_back(new border(523, 329, 83, 43, ALL_SCHOOL_ID, 1));

    m_previousPageWidget = new bitmapBorder(
        97, 77, 33, 39, PREVIOUS_PAGE_ID,
        DATA_COMPGEN(0x00684ba4, spellbookPreviousPage, "SpelTrnL.pcx"),
        0x800);
    m_widgets.push_back(m_previousPageWidget);

    m_nextPageWidget = new bitmapBorder(
        487, 74, 29, 32, NEXT_PAGE_ID,
        DATA_COMPGEN(0x00684b94, spellbookNextPage, "SpelTrnR.pcx"),
        0x800);
    m_widgets.push_back(m_nextPageWidget);

    m_widgets.push_back(new border(
        219, 404, 37, 47, COMBAT_SPELLS_ID, 1));
    m_widgets.push_back(new border(
        353, 405, 35, 41, ADVENTURE_SPELLS_ID, 1));

    sprintf(g_text,
        DATA_COMPGEN(0x00660a1c, spellbookDecimalFormat, "%d"),
        m_hero->m_mana);
    m_widgets.push_back(new textWidget(
        417, 405, 36, 45, g_text,
        DATA_COMPGEN(0x0065f2f8, spellbookSmallFont, "smalfont.fnt"),
        font::HEADING,
        SPELL_POINTS_ID, 5, 0, 8));

    m_widgets.push_back(new border(
        478, 407, 35, 42, DIALOG_RETURN_CANCEL, 1));

    m_rolloverWidget = new bitmapBackedTextWidget(
        8, 569, 605, 19,
        0,
        DATA_COMPGEN(0x0065f2f8, spellbookSmallFont, "smalfont.fnt"),
        DATA_COMPGEN(0x00684b84, spellbookRollover, "spelroll.pcx"),
        font::PRIMARY, ROLLOVER_ID, 1, 8);
    m_widgets.push_back(m_rolloverWidget);

    m_spellLevelWidgets = static_cast<iconWidget**>(static_cast<void*>(
        &m_widgets[spellLevelWidgetsIndex]));
    m_spellIconWidgets = static_cast<iconWidget**>(static_cast<void*>(
        &m_widgets[spellIconWidgetsIndex]));
    m_spellNameWidgets = static_cast<textWidget**>(static_cast<void*>(
        &m_widgets[spellNameWidgetsIndex]));

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    if (context == eContextNeither) {
        if (s_lastContext != eContextInvalid) {
            setContext(s_lastContext);
            TSpellSchool selectedSchool = s_lastSchool;
            setSchool(selectedSchool);
            m_schoolTabsWidget->setIconFrame(
                getPositionFromSchool(selectedSchool));
            gotoPage(s_lastPage);
        } else {
            setContext(eContextAdventure);
            setSchool(eSchoolAll);
            m_schoolTabsWidget->setIconFrame(
                getPositionFromSchool(eSchoolAll));
            gotoPage(0);
        }
    } else if (s_lastContext == context) {
        setContext(context);
        TSpellSchool selectedSchool = s_lastSchool;
        setSchool(selectedSchool);
        m_schoolTabsWidget->setIconFrame(
            getPositionFromSchool(selectedSchool));
        gotoPage(s_lastPage);
    } else {
        setContext(context);
        setSchool(eSchoolAll);
        m_schoolTabsWidget->setIconFrame(
            getPositionFromSchool(eSchoolAll));
        gotoPage(0);
    }
}

VA_COMPGEN(0x0059c8c0, 0x21, SCALAR_DELETING_DTOR, TSpellbookWindow)

// Dinkumware's _Insertion_sort_1 over TSpellbookEntry, the tail of the sort
// that orders the open page's entries. The two mnemonic streams agree
// EXACTLY over all 368 bytes against the COMDAT this object already emits.
VA_COMPGEN(0x0059def0, 0x170, INSERTION_SORT_1, TSpellbookEntry)

VA(0x0059c8f0, 0x75)  // dc 0x14c864
TSpellbookWindow::~TSpellbookWindow()
{
    g_spellbookWindow = 0;
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

VA(0x0059c970, 0x1B)  // dc 0x14c8d4
int TSpellbookWindow::open(int newPriority, unsigned char update)
{
    return heroWindow::open(newPriority, update) ? 3 : 0;
}

VA(0x0059c990, 0x10)  // dc 0x14c8f0
void TSpellbookWindow::close(unsigned char update)
{
    heroWindow::close(update);
}

// DC uses push_back for the available-spell entry. Restoring that wrapper
// leaves sort's _Unguarded_insert retained where retail expands it (88.8745%,
// formerly exact with direct insert). Conditional or single-value school
// selection does not recover it. Retail retains getSpellLevel here; DC's
// older caller uses GetSpellSchoolLevel, so keep Complete's spell-id contract.
VA(0x0059c9a0, 0x691)  // dc 0x14c904
void TSpellbookWindow::gotoPage(int page)
{
    if (page < 0)
        return;

    // DC locals: available_spells and FIRST_SLOT_AFTER_HEADING.
    const int firstSlotAfterHeading = 2;
    std::vector<TSpellbookEntry> availableSpells;
    availableSpells.reserve(hero::NUM_SPELLS);

    for (int spell = 0; spell < hero::NUM_SPELLS; ++spell) {
        if (m_hero->spellIsAvailable(spell)
            && (m_school & g_spellTraits[spell].m_school)
            && (m_contextMask & g_spellTraits[spell].m_flags)) {
            TSpellSchool highestSchool =
                m_hero->getHighestSchool(g_spellTraits[spell].m_school);
            TSpellSchool school = m_school;
            if (school == eSchoolAll)
                school = highestSchool;
            TSkillMastery mastery = m_hero->getSpellLevel(
                spell, m_onMagicPlains);
            availableSpells.push_back(TSpellbookEntry(spell, school, mastery));
        }
    }

    std::sort(availableSpells.begin(), availableSpells.end());

    unsigned spellIndex = page * SPELLS_PER_PAGE;
    if (page > 0 && m_school != eSchoolAll)
        spellIndex -= firstSlotAfterHeading;

    int widgetIndex;
    if (page == 0 && m_school != eSchoolAll) {
        m_headingWidget->m_status |= widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;
        m_headingWidget->setIconFrame(getPositionFromSchool(m_school));

        m_spellLevelWidgets[0]->m_status &=
            ~(widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        m_spellIconWidgets[0]->m_status &=
            ~(widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        m_spellNameWidgets[0]->setText(
            DATA_COMPGEN(0x00691210, adventureRolloverEmptyText, ""));
        m_spellMap[0] = -1;

        m_spellLevelWidgets[1]->m_status &=
            ~(widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        m_spellIconWidgets[1]->m_status &=
            ~(widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        m_spellNameWidgets[1]->setText(
            DATA_COMPGEN(0x00691210, adventureRolloverEmptyText, ""));
        m_spellMap[1] = -1;
        widgetIndex = firstSlotAfterHeading;
    } else {
        m_headingWidget->m_status &=
            ~(widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        widgetIndex = 0;
    }

    for (; widgetIndex < SPELLS_PER_PAGE && spellIndex < availableSpells.size();
         ++widgetIndex, ++spellIndex) {
        const TSpellbookEntry& entry = availableSpells[spellIndex];
        SpellID displaySpell = entry.m_id;
        m_spellLevelWidgets[widgetIndex]->m_status |=
            widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;
        m_spellLevelWidgets[widgetIndex]->setSprite(
            g_levelSprites[getPositionFromSchool(entry.m_school)]);
        m_spellLevelWidgets[widgetIndex]->setIconFrame(entry.m_mastery);

        m_spellIconWidgets[widgetIndex]->m_status |=
            widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;
        m_spellIconWidgets[widgetIndex]->setIconFrame(displaySpell);

        if (entry.m_mastery > 0) {
            sprintf(g_text,
                    DATA_COMPGEN(0x00684bec, spellInfoWithMastery,
                                 "{%s}\n%s/%s\n%s: %d"),
                    g_spellTraits[displaySpell].m_name,
                    getLevelString(displaySpell),
                    g_secondarySkillLevels[entry.m_mastery],
                    (*g_generalText)[388],
                    const_cast<hero*>(m_hero)->getManaCost(
                        displaySpell, m_enemyGroup, m_onMagicPlains));
        } else {
            sprintf(g_text,
                    DATA_COMPGEN(0x00684bdc, spellInfoWithoutMastery,
                                 "{%s}\n%s\n%s: %d"),
                    g_spellTraits[displaySpell].m_name,
                    getLevelString(displaySpell), (*g_generalText)[388],
                    const_cast<hero*>(m_hero)->getManaCost(
                        displaySpell, m_enemyGroup, m_onMagicPlains));
        }
        m_spellNameWidgets[widgetIndex]->setText(g_text);

        if (const_cast<hero*>(m_hero)->getManaCost(
                displaySpell, m_enemyGroup, m_onMagicPlains) <= m_hero->m_mana)
            m_spellNameWidgets[widgetIndex]->m_status &= ~widget::WIDGET_DIMMED;
        else
            m_spellNameWidgets[widgetIndex]->m_status |= widget::WIDGET_DIMMED;

        m_spellMap[widgetIndex] = displaySpell;
    }

    for (; widgetIndex < SPELLS_PER_PAGE; ++widgetIndex) {
        m_spellLevelWidgets[widgetIndex]->m_status &=
            ~(widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        m_spellIconWidgets[widgetIndex]->m_status &=
            ~(widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        m_spellNameWidgets[widgetIndex]->setText(
            DATA_COMPGEN(0x00691210, adventureRolloverEmptyText, ""));
        m_spellMap[widgetIndex] = -1;
    }

    m_page = page;
    s_lastPage = page;

    if (page > 0)
        m_previousPageWidget->m_status |=
            widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;
    else
        m_previousPageWidget->m_status &=
            ~(widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);

    if (spellIndex < availableSpells.size())
        m_nextPageWidget->m_status |=
            widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;
    else
        m_nextPageWidget->m_status &=
            ~(widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
}

void TSpellbookWindow::displayNewSchool(int position)
{
    if (getSchool() == getSchoolFromPosition(position))
        return;

    if (g_unnamed698758.m_animateSpellBook)
        videoPlay(0x25, m_x + 13, m_y + 14, -1, -1);
    setSchool(getSchoolFromPosition(position));
    gotoPage(0);
    m_schoolTabsWidget->setIconFrame(position);
    drawWindow(1, -65535, 65535);
}

// E:\gamedcs\spellbookwindow.cpp:702, dc 0x14ce68.
int TSpellbookWindow::convertID2HelpID(int id) const
{
    if (id < 0)
        return -1;
    switch (id) {
    case PREVIOUS_PAGE_ID: return 0;
    case NEXT_PAGE_ID: return 1;
    case ADVENTURE_SPELLS_ID: return 2;
    case COMBAT_SPELLS_ID: return 3;
    case AIR_SCHOOL_ID: return 4;
    case FIRE_SCHOOL_ID: return 5;
    case WATER_SCHOOL_ID: return 6;
    case EARTH_SCHOOL_ID: return 7;
    case ALL_SCHOOL_ID: return 8;
    case SPELL_POINTS_ID: return 9;
    case DIALOG_RETURN_CANCEL: return 10;
    }
    return -1;
}

DATA(0x00641da4) static const int g_schoolToTab[] = {0, 2, 3, 1, 4};
DATA(0x00641db8) static const int g_tabToSchool[] = {0, 3, 1, 2, 4};

// E:\gamedcs\spellbookwindow.cpp:755
// Byte-exact after repairing convertID2HelpID's two inlined lookup tables.
// Preserve the widget dispatch (spells, schools, combat, adventure,
// previous, next, cancel), real exit flag, mana-ok SpellMap re-subscript,
// and rollover cache guard. Named spell and rolloverWidget locals plus
// canonical previousPage/nextPage calls reproduce the string expansions.
// The physical epilogue precedes the rollover code; the full carve and both
// lookup pools are required because their addends affect the function bytes.
// Failed controls: dialogReturn-before-id changed the shared exit tail;
// early return on a rollover cache hit changed the lifetime/return paths.
VA(0x0059d040, 0xBA0)  // anchor-callee: calls GotoPage/get_spell_description/GetManaCost/SetIconFrame, msg jump-table, ret 4; absorbs inlined DisplayNewSchool+convertID2HelpID; dc 0x14cecc
int TSpellbookWindow::windowHandler(message& msg)
{
    int exitFlag = 0;
    int handled = CAdvPopup::windowHandler(msg);
    if (handled)
        return handled;

    pollSound();

    if (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT) {
        if (msg.m_codeX == widget::WIDGET_SELECT
            || msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
            int id = msg.m_codeY;
            if (id >= SPELL_0_ID && id <= SPELL_11_ID) {
                SpellID spell = m_spellMap[id - SPELL_0_ID];
                normalDialog(getSpellDescription(spell, m_hero, 0).c_str(),
                             4, -1, -1,
                             9, spell, -1, 0, -1, 0, -1, 0);
            } else {
                int helpId = convertID2HelpID(id);
                if (helpId >= 0)
                    normalDialog(g_spellbookHelpText[helpId].m_rclick,
                                 4, -1, -1, -1, 0, -1, 0,
                                 -1, 0, -1, 0);
            }
        }
    } else if (msg.m_id == MESSAGE_KEY_DOWN) {
        switch (msg.m_codeX) {
        case KEYCODE_KP_4: // left
            if (m_previousPageWidget->m_status & widget::WIDGET_ACTIVE) {
                if (g_unnamed698758.m_animateSpellBook)
                    videoPlay(0x24, m_x + 13, m_y + 14, -1, -1);
                previousPage();
                drawWindow(1, -65535, 65535);
            }
            break;

        case KEYCODE_KP_6: // right
            if (m_nextPageWidget->m_status & widget::WIDGET_ACTIVE) {
                if (g_unnamed698758.m_animateSpellBook)
                    videoPlay(0x25, m_x + 13, m_y + 14, -1, -1);
                nextPage();
                drawWindow(1, -65535, 65535);
            }
            break;

        case KEYCODE_KP_8: { // up
            int schoolPosition = getPositionFromSchool(m_school);
            int tab = g_schoolToTab[schoolPosition] - 1;
            if (tab < 0)
                tab = 4;
            displayNewSchool(g_tabToSchool[tab]);
            break;
        }

        case KEYCODE_KP_2: { // down
            int schoolPosition = getPositionFromSchool(m_school);
            int tab = g_schoolToTab[schoolPosition] + 1;
            if (tab >= 5)
                tab = 0;
            displayNewSchool(g_tabToSchool[tab]);
            break;
        }

        case KEYCODE_A: // adventure spells
            if (m_contextMask != eAdventureContextMask) {
                if (g_unnamed698758.m_animateSpellBook)
                    videoPlay(0x25, m_x + 13, m_y + 14, -1, -1);
                setContext(eContextAdventure);
                gotoPage(0);
                drawWindow(1, -65535, 65535);
            }
            break;

        case KEYCODE_C: // combat spells
            if (m_contextMask != eCombatContextMask) {
                if (g_unnamed698758.m_animateSpellBook)
                    videoPlay(0x24, m_x + 13, m_y + 14, -1, -1);
                setContext(eContextCombat);
                gotoPage(0);
                drawWindow(1, -65535, 65535);
            }
            break;

        case KEYCODE_ESCAPE:
        case KEYCODE_ENTER:
            msg.m_codeY = DIALOG_RETURN_CANCEL;
            exitFlag = 1;
            break;
        }
    } else if (msg.m_id == MESSAGE_WIDGET) {
        if (msg.m_codeX == widget::WIDGET_SELECT) {
            switch (msg.m_codeY) {
            case SPELL_0_ID:
            case SPELL_1_ID:
            case SPELL_2_ID:
            case SPELL_3_ID:
            case SPELL_4_ID:
            case SPELL_5_ID:
            case SPELL_6_ID:
            case SPELL_7_ID:
            case SPELL_8_ID:
            case SPELL_9_ID:
            case SPELL_10_ID:
            case SPELL_11_ID: {
                SpellID spell = m_spellMap[msg.m_codeY - SPELL_0_ID];
                if ((m_allowedContext == eContextCombat
                     && getContextMask() == eCombatContextMask)
                    || (m_allowedContext == eContextAdventure
                        && getContextMask() == eAdventureContextMask)) {
                    int manaCost = const_cast<hero*>(m_hero)->getManaCost(
                        spell, m_enemyGroup, m_onMagicPlains);
                    if (manaCost <= m_hero->m_mana) {
                        exitFlag = 1;
                        msg.m_codeY = m_spellMap[msg.m_codeY - SPELL_0_ID];
                    } else {
                        sprintf(g_text, (*g_generalText)[207],
                                manaCost, m_hero->m_mana);
                        normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0,
                                     -1, 0, -1, 0);
                    }
                } else {
                    normalDialog(
                        getSpellDescription(spell, m_hero, 0).c_str(),
                        1, -1, -1, 9, spell, -1, 0,
                        -1, 0, -1, 0);
                }
                break;
            }

            case AIR_SCHOOL_ID:
            case FIRE_SCHOOL_ID:
            case WATER_SCHOOL_ID:
            case EARTH_SCHOOL_ID:
            case ALL_SCHOOL_ID:
                displayNewSchool(msg.m_codeY - AIR_SCHOOL_ID);
                break;

            case COMBAT_SPELLS_ID:
                if (getContextMask() != eCombatContextMask) {
                    if (g_unnamed698758.m_animateSpellBook)
                        videoPlay(0x24, m_x + 13, m_y + 14, -1, -1);
                    setContext(eContextCombat);
                    gotoPage(0);
                    drawWindow(1, -65535, 65535);
                }
                break;

            case ADVENTURE_SPELLS_ID:
                if (getContextMask() != eAdventureContextMask) {
                    if (g_unnamed698758.m_animateSpellBook)
                        videoPlay(0x25, m_x + 13, m_y + 14, -1, -1);
                    setContext(eContextAdventure);
                    gotoPage(0);
                    drawWindow(1, -65535, 65535);
                }
                break;

            case PREVIOUS_PAGE_ID:
                if (g_unnamed698758.m_animateSpellBook)
                    videoPlay(0x24, m_x + 13, m_y + 14, -1, -1);
                previousPage();
                drawWindow(1, -65535, 65535);
                break;

            case NEXT_PAGE_ID:
                if (g_unnamed698758.m_animateSpellBook)
                    videoPlay(0x25, m_x + 13, m_y + 14, -1, -1);
                nextPage();
                drawWindow(1, -65535, 65535);
                break;

            case DIALOG_RETURN_CANCEL:
                exitFlag = 1;
                break;
            }
        }
    } else if (msg.m_id == MESSAGE_MOUSE_MOVE) {
        int id = findWidget(msg.m_mouseX, msg.m_mouseY);
        if (id != g_lastImHoverId) {
            std::string rollover;
            g_lastImHoverId = id;
            if (id != -1) {
                g_mouseManager->setPointer(1, mouseManager::DEFAULT_SET);
                if (id >= SPELL_0_ID && id <= SPELL_11_ID) {
                    SpellID spell = m_spellMap[id - SPELL_0_ID];
                    rollover = getSpellDescription(spell, m_hero, 1);
                } else {
                    int helpId = convertID2HelpID(id);
                    if (helpId >= 0)
                        rollover = g_spellbookHelpText[helpId].m_text;
                    else
                        rollover = "";
                }
            } else {
                g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
                rollover = "";
            }

            textWidget* rolloverWidget = m_rolloverWidget;
            rolloverWidget->setText(rollover.c_str());
            drawWindow(0, ROLLOVER_ID, ROLLOVER_ID);
            g_windowManager->updateScreen(
                m_x + rolloverWidget->m_x, m_y + rolloverWidget->m_y,
                rolloverWidget->m_width, rolloverWidget->m_height);
        }
    }

    if (exitFlag) {
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = msg.m_codeY;
        msg.m_codeY = 10;
        msg.m_codeX = 10;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x0059dbe0, 0x84)  // dc 0x14d290
bool TSpellbookWindow::TSpellbookEntry::operator<(const TSpellbookEntry& y) const
{
    const SSpellTraits* traits = &g_spellTraits[m_id];
    const SSpellTraits* yTraits = &g_spellTraits[y.m_id];
    if (traits->m_level < yTraits->m_level)
        return true;
    if (traits->m_level > yTraits->m_level)
        return false;
    if (m_school < y.m_school)
        return true;
    if (m_school > y.m_school)
        return false;
    return _strcmpi(traits->m_name, yTraits->m_name) < 0;
}

// COMDAT pairing: std::_Sort<TSpellbookEntry>, agreement 1.000 over all 235
// instructions. Sits beside the unit's _Insertion_sort_1 over the same element.
VA_COMPGEN(0x0059dc90, 0x260, STD_SORT, tspellbookentry)

// COMDAT pairing: basic_string<char>::erase(size_t, size_t), agreement 0.984.
VA_COMPGEN(0x0041bdb0, 0x11C, BASIC_STRING_ERASE, char)
