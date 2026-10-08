#include "text.h"
#include "va.h"
#include "includes.h"

#include <stdio.h>
#include <string>
#include <strstream>

#include "quicktownwindow.h"

#include "armygrp.h"
#include "border.h"
#include "game.h"
#include "iconwdgt.h"
#include "kb.h"
#include "terrain.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// The shared includes.h limit wrapper passes copies to tLimit before
// center selects an operand address and loads the coordinate.

// Seven compact army slots, consumed only by initialize_army_display.
// Retail's sole reference is 0x5309df and the 56-byte extent closes at the
// next initialized datum (0x6823f0).
DATA(0x006823b8) static int g_quickTownArmyPositions[7][2] = {
    {45, 84}, {81, 84}, {117, 84}, {27, 132},
    {63, 132}, {99, 132}, {135, 132}
};

// E:\gamedcs\quicktownwindow.cpp:39
// DC 0x117fda loads HasBuilding into r11 for all seven hall, silo and
// fort tests. The capitol test is represented by isCapitol's nested call.
// Earlier inliner probes replaced late push_back calls with insert/end
// through a vector pointer (96.3088 -> 98.4193); those spellings were
// superseded by the canonical helper calls below. An enum loop variable
// and inline_depth(1) were score-flat. Named bonus objects retain the
// allocation result's retail stack home; nesting every allocation in
// push_back or naming unrelated early widgets was worse.
VA(0x00530120, 0x67D)
DC_ADDRESS(0x117e48, 0x570)
MAC_ADDRESS(0x14be1c, 0xa90)  // townqvbk/itpt literals + town helpers
TQuickTownWindow::TQuickTownWindow(const town* thisTown, TQuickTownWindow::TViewLevel viewLevel)
    : heroWindow(200, 200, 194, 186, 0x12)
{
    m_widgets.reserve(NWIDGETS);

    bitmapBorder* background = new bitmapBorder(
        0, 0, 194, 186, BACKGROUND_ID, "townqvbk.pcx", 0x800);
    background->setPlayerPaletteColors(
        thisTown->m_owner != -1
            ? thisTown->m_owner
            : g_game->getLocalPlayerGamePos());
    m_widgets.push_back(background);

    m_widgets.push_back(new iconWidget(
        12, 13, 58, 64, PORTRAIT_ID, "itpt.def",
        thisTown->getPortraitFrame(false), 0, 0, 0, 0x10));

    m_widgets.push_back(new textWidget(
        75, 12, 107, 16, thisTown->m_name.c_str(), "smalfont.fnt",
        font::WHITE, NAME_ID, 0, 0, 8));

    std::string townSizeName;
    int hallLevel;
    if (thisTown->hasBuilding(HALL_TOWN_ID, false))
        hallLevel = 1;
    else if (thisTown->hasBuilding(HALL_CITY_ID, false))
        hallLevel = 2;
    else if (thisTown->isCapitol())
        hallLevel = 3;
    else
        hallLevel = 0;
    townSizeName = g_townSizeNames[hallLevel];

    if (viewLevel >= ViewAll) {
        m_widgets.push_back(new iconWidget(
            76, 42, 34, 34, HALL_LEVEL_ID, "itmtls.def", hallLevel,
            0, 0, 0, 0x10));

        if (thisTown->m_garrisonHeroId != -1) {
            m_widgets.push_back(new bitmapBorder(
                158, 86, 22, 30, GARRISON_HERO_ID, "townqkgh.pcx",
                0x800));
        }

        if (thisTown->hasBuilding(MARKETPLACE_SILO_ID, true)) {
            int* siloIncome = thisTown->getSiloIncome();
            // DC resource is sp+0x5c; after the prologue's 68-byte SP
            // decrement this is r14+24, precisely the loop counter slot.
            // DC85 increments count; DC86 stores through r14+24+4*count;
            // DC88 reloads resource[0]. Icon arguments read r14+28/+32.
            // Retail does the same at ebp-0x28/-0x24/-0x20. The loop counter
            // is part of the declared array, not a separate local below it.
            // Its enum ordinal advance needs the explicit standard-C++ cast;
            // the resulting increment and all array accesses match retail.
            EGameResource resource[3];
            int resourceCount = 0;
            for (resource[0] = WOOD; resource[0] <= GOLD;
                 resource[0] = static_cast<EGameResource>(resource[0] + 1) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */) {
                if (siloIncome[resource[0]]) {
                    ++resourceCount;
                    resource[resourceCount] = resource[0];
                }
            }

            // DC94/95,97/98 and104/105 separate each allocation from its
            // push_back. Retail also homes these raw new results in the dead
            // thisTown parameter slot used by the converted widget pointer.
            // Named bonus objects recover that home; moving every allocation
            // into push_back or naming unrelated early widgets was worse.
            if (resourceCount == DOUBLE_RESOURCE_BONUS) {
                iconWidget* firstBonus = new iconWidget(
                    15, 86, 20, 18, RESOURCE_BONUS_ID, "smalres.def",
                    resource[1], 0, 0, 0, 0x10);
                m_widgets.push_back(firstBonus);
                iconWidget* secondBonus = new iconWidget(
                    15, 98, 20, 18, RESOURCE_BONUS_ID, "smalres.def",
                    resource[2], 0, 0, 0, 0x10);
                m_widgets.push_back(secondBonus);
            } else if (resourceCount == SINGLE_RESOURCE_BONUS) {
                iconWidget* singleBonus = new iconWidget(
                    15, 92, 22, 18, RESOURCE_BONUS_ID, "smalres.def",
                    resource[1], 0, 0, 0, 0x10);
                m_widgets.push_back(singleBonus);
            }
        }

        sprintf(g_text, "%d", thisTown->getGoldIncome(1));
        m_widgets.push_back(new textWidget(
            153, 65, 27, 11, g_text, "tiny.fnt", font::WHITE,
            GOLD_PER_DAY_ID, 1, 0, 8));
    }

    // DC116/118/120 reuses HasBuilding through r11 with ids7/8/9 and
    // checkIncluded=0. Retail reads the built mask through these expansions.
    int castleLevel;
    if (thisTown->hasBuilding(CASTLE_FORT_ID, false))
        castleLevel = 0;
    else if (thisTown->hasBuilding(CASTLE_CITADEL_ID, false))
        castleLevel = 1;
    else if (thisTown->hasBuilding(CASTLE_CASTLE_ID, false))
        castleLevel = 2;
    else
        castleLevel = 3;
    m_widgets.push_back(new iconWidget(
        114, 42, 34, 34, CASTLE_LEVEL_ID, "itmcls.def", castleLevel,
        0, 0, 0, 0x10));

    initializeArmyDisplay(thisTown->getArmy(), viewLevel);
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }
}

VA(0x005307d0, 0x145)
DC_ADDRESS(0x1183b8, 0x10c)
MAC_ADDRESS(0x14c8ac, 0x234)
TQuickTownWindow::TQuickTownWindow(const garrison* thisGarrison,
                                   TQuickTownWindow::TViewLevel viewLevel)
    : heroWindow(200, 200, 188, 182, 0x12)
{
    m_widgets.push_back(new bitmapBorder(
        0, 0, 194, 186, BACKGROUND_ID, "townqvbk.pcx", 0x800));
    m_widgets.push_back(new textWidget(
        77, 13, 110, 24, g_quickViewText[33], "smalfont.fnt",
        font::WHITE, NAME_ID, 0, 0, 8));

    initializeArmyDisplay(thisGarrison->m_garrisonArmy, viewLevel);
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }
}

VA_COMPGEN(0x005307a0, 0x21, SCALAR_DELETING_DTOR, TQuickTownWindow)

VA(0x00530920, 0x6B)
DC_ADDRESS(0x1184c4, 0xa0)
MAC_ADDRESS(0x14cae0, 0xac)
TQuickTownWindow::~TQuickTownWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// Retail +0x1de and +0x272 push "tiny.fnt" (0x660cb4) for the quantities.
// Mac 0x14cccc/0x14ccec formats decimal quantities into a stack buffer;
// 0x14ccf8 appends "k" and 0x14cdd8 copies getArmySizeName's result.
// Complete instead retains ostrstream integer/string insertion at
// 0x530af9/0x530b1d/0x530b23/0x530bb2, including its owning lifetime.
VA(0x00530990, 0x303)
DC_ADDRESS(0x118564, 0x22e)
MAC_ADDRESS(0x14cb8c, 0x340)
void TQuickTownWindow::initializeArmyDisplay(
    const armyGroup& currentArmyGroup, TQuickTownWindow::TViewLevel viewLevel)
{
    int numArmies = currentArmyGroup.getNumArmies();
    if (numArmies <= 0 || viewLevel < ViewArmyTypes)
        return;

    int widgetId = ARMY_1_SPRITE_ID;
    int displaySlot = 0;
    for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
        int creature = currentArmyGroup.m_armies[slot];
        if (creature == CREATURE_NONE)
            continue;

        m_widgets.push_back(new iconWidget(
            g_quickTownArmyPositions[displaySlot][0],
            g_quickTownArmyPositions[displaySlot][1], 32, 32, widgetId++,
            "cprsmall.def", creature + 2, 0, 0, 0, 0x10));

        if (viewLevel >= ViewArmySizes) {
            int count = currentArmyGroup.m_numTroops[slot];
            std::ostrstream quantityText;
            if (viewLevel >= ViewAll) {
                if (count < 10000)
                    quantityText << count << std::ends;
                else
                    quantityText << count / 1000 << "k" << std::ends;

                m_widgets.push_back(new textWidget(
                    g_quickTownArmyPositions[displaySlot][0],
                    g_quickTownArmyPositions[displaySlot][1] + 34, 32, 13,
                    quantityText.str(), "tiny.fnt", font::WHITE, -1,
                    1, 0, 8));
            } else {
                quantityText << armyGroup::getArmySizeName(count, 0)
                              << std::ends;
                m_widgets.push_back(new textWidget(
                    g_quickTownArmyPositions[displaySlot][0],
                    g_quickTownArmyPositions[displaySlot][1] + 34, 32, 13,
                    quantityText.str(), "tiny.fnt", font::WHITE,
                    widgetId++, 1, 0, 8));
            }
            quantityText.freeze(false);
        }
        ++displaySlot;
    }
}

VA(0x00530ca0, 0x84)
DC_ADDRESS(0x118794, 0x64)
MAC_ADDRESS(0x14cecc, 0xe4)
void TQuickTownWindow::center(long newX, long newY)
{
    m_x = limit(m_width / 2, newX,
              WINDOW_SCREEN_WIDTH - m_width / 2 - 1) - m_width / 2;
    m_y = limit(m_height / 2, newY,
              WINDOW_SCREEN_HEIGHT - m_height / 2 - 1) - m_height / 2;
}

VA(0x00530d30, 0xD)
DC_ADDRESS(0x1187f8, 0x50)
MAC_ADDRESS(0x14cfb0, 0x2c)
void TQuickTownWindow::quickWindowWait()
{
    g_windowManager->doQuickView(this);
}

// E:\gamedcs\quicktownwindow.cpp:139
