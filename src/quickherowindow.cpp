#include "va.h"
#include "includes.h"

#include <stdio.h>
#include <string>
#include <strstream>
#include "platform.h"

#include "quickherowindow.h"

#include "armygrp.h"
#include "border.h"
#include "game.h"
#include "hero.h"
#include "iconwdgt.h"
#include "kb.h"
#include "misc.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// DC includes.h:134 supplies the shared limit wrapper called at both
// morale/luck sites; homm3_limit.h owns the integer reference selector.

// DC static skill_loc (type 0x1ae5): const POINT[4].
DATA(0x00640688) static const POINT g_skillLoc[4] = {
    {74, 62}, {101, 62}, {129, 62}, {157, 62}
};

// DC static army_pos (type 0x3fa2): int[7][2].
DATA(0x00682378) static int g_armyPos[7][2] = {
    {45, 84}, {81, 84}, {117, 84}, {27, 132},
    {63, 132}, {99, 132}, {135, 132}
};

// Dreamcast 0x1170bc locates the disguise scans in this constructor at
// lines 94..129 and proves the separate primary-skill widget id, limit/tLimit
// calls, and widget push_back operations. Complete replaces quantity_text's
// 100-byte sprintf buffer with an owning ostrstream; the per-arm textWidget
// constructions and freeze(false) follow retail's EH lifetimes.

// Residual (94.1662%): the first source difference is reserve's temporary
// stack home (-0x18 versus -0x14); primary-stat addressing and register roles
// also differ. The mana string's _Tidy expands where retail calls it,
// contributing four extra CFG blocks and three branches. Keep its meaningful
// temporary lifetime rather than adding an inliner gate. The init helper and
// both window destructors are independently exact.

VA(0x0052ead0, 0x8C8)  // heroqvbk.pcx + vtable/allocation block, dc 0x1170bc
TQuickHeroWindow::TQuickHeroWindow(hero* thisHero, TViewLevel viewLevel)
    : heroWindow(200, 200, 194, 186, 0x12)
{
    std::vector<widget*>& widgets = m_widgets;
    widgets.reserve(NWIDGETS);

    bitmapBorder* background = new bitmapBorder(
        0, 0, 194, 186, BACKGROUND_ID, "heroqvbk.pcx", 0x800);
    // Mac retains a call in each branch at 0:0x14a8d4 and 0:0x14a8ec.
    if (thisHero->m_owner >= 0)
        background->setPlayerPaletteColors(thisHero->m_owner);
    else
        background->setPlayerPaletteColors(g_game->getLocalPlayerGamePos());
    widgets.push_back(background);

    widgets.push_back(new bitmapBorder(
        12, 13, 58, 64, PORTRAIT_ID,
        g_heroTraits[thisHero->m_portrait].m_largePortraitName, 0x800));

    widgets.push_back(new textWidget(
        75, 13, 107, 17, thisHero->m_name, "smalfont.fnt", font::WHITE,
        NAME_ID, 0, 0, 8));

    if (viewLevel >= ViewAll) {
        int widgetId = PRIMARY_SKILL_1_ID;
        for (int stat = 0; stat < 4; ++stat) {
            sprintf(g_text, "%d", thisHero->getPrimarySkill(stat));
            widgets.push_back(new textWidget(
                g_skillLoc[stat].x,
                g_skillLoc[stat].y,
                23, 16, g_text, "smalfont.fnt", font::WHITE,
                widgetId, 1, 0, 8));
            ++widgetId;
        }

        widgets.push_back(new textWidget(
            154, 104, 27, 13,
            formatString("%d", thisHero->m_mana).c_str(), "tiny.fnt",
            font::WHITE, MANA_ID, 1, 0, 8));

        int morale = limit(
            -3, thisHero->getMorale(0, 0, 1), 3);
        widgets.push_back(new iconWidget(
            14, 86, 22, 12, MORALE_ID, "imrl22.def", morale + 3,
            0, 0, 0, 0x10));

        int luck = limit(
            -3, thisHero->getLuck(0, 0, 1), 3);
        widgets.push_back(new iconWidget(
            14, 103, 22, 12, LUCK_ID, "ilck22.def", luck + 3,
            0, 0, 0, 0x10));
    }

    if (viewLevel >= ViewSome && thisHero->m_army.getNumArmies() > 0) {
        int disguiseCreature = CREATURE_NONE;
        if (thisHero->m_disguiseLevel != TQuickHeroWindow::DisguiseInvalid &&
            thisHero->m_disguiseLevel <= TQuickHeroWindow::DisguiseAdvanced) {
            const int* currentArmy = thisHero->m_army.m_armies;
            for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT;
                 ++slot, ++currentArmy) {
                int creature = *currentArmy;
                // Retail compares the slot ordinal, not the creature loaded just
                // above.  Preserve that byte-proven source-level wart.
                if (slot != CREATURE_NONE &&
                    (disguiseCreature == CREATURE_NONE ||
                     g_creatureTypeTraits[creature].m_aiValue >
                         g_creatureTypeTraits[disguiseCreature].m_aiValue))
                    disguiseCreature = creature;
            }
        } else if (thisHero->m_disguiseLevel == TQuickHeroWindow::DisguiseExpert) {
            int creature = g_game->m_gameVersion ? 145 : 118;
            int owner = thisHero->m_owner;
            while (creature--) {
                int townType = g_game->getAlignment(creature);

                int alignment = owner >= 0
                    ? g_game->m_setup.m_alignment[owner]
                    : -1;
                if (townType == alignment &&
                    (disguiseCreature == CREATURE_NONE ||
                     g_creatureTypeTraits[creature].m_aiValue >
                         g_creatureTypeTraits[disguiseCreature].m_aiValue))
                    disguiseCreature = creature;
            }
        }

        int widgetId = ARMY_1_SPRITE_ID;
        // DC lines 144/159/163 index the army and its packed army_pos
        // display separately. Do not recover armies by indexing backwards
        // out of the neighboring numTroops member.
        int displaySlot = 0;
        for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
            int creature = thisHero->m_army.m_armies[slot];
            if (creature == CREATURE_NONE)
                continue;
            if (disguiseCreature != CREATURE_NONE)
                creature = disguiseCreature;

            widgets.push_back(new iconWidget(
                g_armyPos[displaySlot][0],
                g_armyPos[displaySlot][1], 32, 32, widgetId++,
                "cprsmall.def", creature + 2, 0, 0, 0, 0x10));

            int count;
            if (thisHero->m_disguiseLevel >= DisguiseAdvanced)
                count = 0;
            else
                count = thisHero->m_army.m_numTroops[slot];
            std::ostrstream quantityText;
            if (viewLevel >= ViewAll) {
                if (count < 10000)
                    quantityText << count << std::ends;
                else
                    quantityText << count / 1000 << "k" << std::ends;

                widgets.push_back(new textWidget(
                    g_armyPos[displaySlot][0],
                    g_armyPos[displaySlot][1] + 34, 32, 11,
                    quantityText.str(), "tiny.fnt", font::WHITE,
                    widgetId++, 1, 0, 8));
            } else {
                quantityText << armyGroup::getArmySizeName(count, 0)
                              << std::ends;
                widgets.push_back(new textWidget(
                    g_armyPos[displaySlot][0],
                    g_armyPos[displaySlot][1] + 34, 32, 11,
                    quantityText.str(), "tiny.fnt", font::WHITE,
                    widgetId++, 1, 0, 8));
            }
            quantityText.freeze(false);
            ++displaySlot;
        }
    }

    for (widget** it = widgets.begin(); it != widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }
}

VA_COMPGEN(0x0052f3a0, 0x21, SCALAR_DELETING_DTOR, TQuickHeroWindow)

VA(0x0052f3d0, 0x6B)  // dc 0x1177b4
TQuickHeroWindow::~TQuickHeroWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

VA_COMPGEN(0x0052f440, 0x47, BASIC_IOS_INIT, char)

// Original: TQuickHeroWindow::QuickWindowWait; quickherowindow.cpp:221, dc 0x117818.
// Identical quick-window wrappers fold onto the retail 0x530d30 body.
// Mac retains this ordinary wrapper at 0:0x14b4d0; hero and townmgr call it.
void TQuickHeroWindow::quickWindowWait()
{
    g_windowManager->doQuickView(this);
}

// E:\gamedcs\quickherowindow.cpp:209
