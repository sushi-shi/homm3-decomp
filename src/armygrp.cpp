#include "text.h"
#include "va.h"
#include "includes.h"

#include <algorithm>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "armygrp.h"

#include "advmgr.h"
#include "armygrp_split.h"
#include "border.h"
#include "button.h"
#include "castle.h"
#include "creaturetype.h"
#include "exec.h"
#include "game.h"
#include "hero.h"
#include "iconwdgt.h"
#include "kb.h"
#include "message.h"
#include "misc.h"
#include "spelldefs.h"
#include "textntry.h"
#include "textwdgt.h"
#include "town.h"
#include "townmgr.h"
#include "widget.h"
#include "winmgr.h"

// Retail table initializers, in the layouts used by their named consumers.
DATA(0x00682910) const char* g_creatureBackgrounds[9] = { "CrBkgCas.pcx", "CrBkgRam.pcx", "CrBkgTow.pcx", "CrBkgInf.pcx", "CrBkgNec.pcx", "CrBkgDun.pcx", "CrBkgStr.pcx", "CrBkgFor.pcx", "CrBkgEle.pcx" };

DATA(0x00693878)
static TSplitWindow* g_splitWindow;

// Runtime-loaded combat-stat description lines. Their storage addresses and
// uses are retail-proven here; the text-resource loader owns the definitions.

VA(0x004496a0, 0x16) MAC_ADDRESS(0x056454, 0x94)  // dc 0x4dae4
unsigned char armyGroup::hasCreatures() const
{
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] != CREATURE_NONE)
            return 1;
    }
    return 0;
}

// Dreamcast armygrp.cpp:62 and Mac code0+0x564e8 place this standalone
// helper between hasCreatures and splitSliderCallback. The Mac callback,
// splitArmy and windowHandler retain calls; VC6 expands the same-TU calls.
MAC_ADDRESS(0x0564e8, 0xf4)
void TSplitWindow::updateSplitArmy(unsigned char update)
{
    message msg;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;

    sprintf(g_text, "%d", m_sourceTroops);
    msg.m_codeY = 4;
    msg.m_extraText = g_text;
    broadcastMessage(msg);

    sprintf(g_text, "%d", m_destinationTroops);
    msg.m_codeY = 5;
    msg.m_extraText = g_text;
    broadcastMessage(msg);

    if (update)
        drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

VA(0x004496c0, 0xC3) MAC_ADDRESS(0x0565dc, 0x50)  // dc 0x4db88
void splitSliderCallback(int state, heroWindow*)
{
    g_splitWindow->m_destinationTroops =
        g_splitWindow->m_minimumTransfer + state;
    g_splitWindow->m_sourceTroops =
        g_splitWindow->m_totalTroops - g_splitWindow->m_destinationTroops;
    g_splitWindow->updateSplitArmy(1);
}

VA(0x00449790, 0x65B) MAC_ADDRESS(0x05662c, 0xb2c)  // dc 0x4dbb8
TSplitWindow::TSplitWindow(int x2, int y2, TCreatureType thisArmy)
    : CAdvPopup(x2, y2, 0x12a, 0x151, 0x12)
{
    m_creature = thisArmy;
    m_widgets.reserve(13);

    m_widgets.push_back(new bitmapBorder(
        0, 0, m_width, m_height, 0, "GPuCrDiv.pcx", 0x800));

    sprintf(g_text,
            (*g_generalText)[GENERAL_TEXT_SPLIT_CREATURE_ROLLOVER_FORMAT],
            g_creatureTypeTraits[m_creature].m_pluralName);
    m_widgets.push_back(new textWidget(
        0, 20, m_width, 30, g_text, "bigfont.fnt", font::HEADING,
        1, 1, 0, 8));

    strcpy(g_text, g_creatureBackgrounds[
        g_game->getAlignment(m_creature)]);

    m_widgets.push_back(new bitmapBorder(
        20, 54, 100, 130, -1, g_text, 0x800));
    m_widgets.push_back(new bitmapBorder(
        177, 54, 100, 130, -1, g_text, 0x800));

    strcpy(g_text, g_creatureTypeTraits[m_creature].m_spriteName);
    m_widgets.push_back(new iconWidget(
        20, 54, 100, 130, 2, g_text, 0, 2, 0, 0, 0x12));
    m_widgets.push_back(new iconWidget(
        177, 54, 100, 130, 3, g_text, 0, 2, 0, 0, 0x12));

    m_sourceEntry = new textEntryWidget(
        20, 218, 101, 37, 10, "99999", "bigfont.fnt", font::WHITE, 5,
        0, 0, 4, 0, 4, 0, 0);
    m_widgets.push_back(m_sourceEntry);
    m_destinationEntry = new textEntryWidget(
        177, 218, 101, 37, 10, "99999", "bigfont.fnt", font::WHITE, 5,
        0, 0, 5, 0, 4, 0, 0);
    m_widgets.push_back(m_destinationEntry);

    m_splitSlider = new slider(
        21, 194, 257, 16, 6, 10, splitSliderCallback,
        slider::BROWN, 0, 0);
    m_widgets.push_back(m_splitSlider);

    m_widgets.push_back(new bitmapBorder(
        8, 312, 282, 17, 7, "StatBar.pcx", 0x800));
    m_widgets.push_back(new textWidget(
        8, 312, 282, 17, 0, "smalfont.fnt", font::PRIMARY,
        8, 1, 0, 8));

    m_widgets.push_back(new button(
        20, 263, 64, 32, DIALOG_RETURN_SPLIT_ACCEPT,
        "iOk6432.def", 0, 1, 1, 0x1c, 2));
    m_widgets.push_back(new button(
        214, 263, 64, 30, 0x7801,
        "iCancel.def", 0, 1, 1, 1, 2));

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }
}

VA_COMPGEN(0x00449df0, 0x21, SCALAR_DELETING_DTOR, TSplitWindow)

VA_COMPGEN(0x0044c680, 0x60, BITSET_SET, Bitset9)

VA(0x00449e20, 0x6B) MAC_ADDRESS(0x057158, 0xac)  // dc 0x4e11c
TSplitWindow::~TSplitWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

VA(0x00449e90, 0x2EF) MAC_ADDRESS(0x057204, 0x318)  // dc 0x4e180
void armyGroup::splitArmy(int srcIndex, armyGroup* ag, int destIndex, unsigned char inSrcRestricted, unsigned char inDestRestricted)
{
    g_splitWindow = new TSplitWindow(0xb1, 0x14, m_armyTypes[srcIndex]);
    if (!g_splitWindow)
        memError();

    g_splitWindow->m_sourceTroops = m_numTroops[srcIndex];
    g_splitWindow->m_destinationTroops = ag->m_numTroops[destIndex];
    g_splitWindow->m_totalTroops =
        g_splitWindow->m_sourceTroops + g_splitWindow->m_destinationTroops;

    message msg;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_PLAYER_PALETTE_COLORS;
    msg.m_codeY = 0;
    msg.m_extra = g_game->getLocalPlayerGamePos();
    g_splitWindow->broadcastMessage(msg);

    msg.m_codeX = widget::WIDGET_SET_SLIDER_RESOLUTION;
    msg.m_codeY = 6;
    if ((inSrcRestricted && getNumArmies() == 1)
        || (inDestRestricted && ag->getNumArmies() == 1)) {
        g_splitWindow->m_sourceMustKeep = 1;
        msg.m_extra = g_splitWindow->m_totalTroops;
    } else {
        g_splitWindow->m_sourceMustKeep = 0;
        msg.m_extra = g_splitWindow->m_totalTroops + 1;
    }
    g_splitWindow->broadcastMessage(msg);

    if (inDestRestricted && ag->getNumArmies() == 1)
        g_splitWindow->m_minimumTransfer = 1;
    else
        g_splitWindow->m_minimumTransfer = 0;

    msg.m_codeX = widget::WIDGET_SET_SLIDER_STATE;
    msg.m_extra = g_splitWindow->m_destinationTroops
        - g_splitWindow->m_minimumTransfer;
    g_splitWindow->broadcastMessage(msg);

    g_splitWindow->m_destinationEntry->setFocus(1);
    g_splitWindow->updateSplitArmy(0);
    g_splitWindow->doModal(0);

    if (g_windowManager->m_dialogReturn == DIALOG_RETURN_SPLIT_ACCEPT) {
        int maximumTransfer = g_splitWindow->m_totalTroops
            - g_splitWindow->m_sourceMustKeep;
        g_splitWindow->m_destinationTroops = limit(
            g_splitWindow->m_minimumTransfer,
            g_splitWindow->m_destinationTroops,
            maximumTransfer);

        ag->m_armies[destIndex] = m_armies[srcIndex];
        ag->m_numTroops[destIndex] = g_splitWindow->m_destinationTroops;
        if (!ag->m_numTroops[destIndex])
            ag->m_armies[destIndex] = CREATURE_NONE;

        m_numTroops[srcIndex] = g_splitWindow->m_totalTroops
            - g_splitWindow->m_destinationTroops;
        if (!m_numTroops[srcIndex])
            m_armies[srcIndex] = CREATURE_NONE;
    }

    delete g_splitWindow;
}

// E:\gamedcs\armygrp.cpp:208. Retail /Ob2 expands the sole call below and
// /OPT:REF removes the out-of-line copy. Mac retains it at code0+0x5751c.
MAC_ADDRESS(0x05751c, 0x118)
void TSplitWindow::setRolloverText(int codeY)
{
    switch (codeY) {
    case DIALOG_RETURN_SPLIT_CANCEL:
        sprintf(g_text,
                g_generalText->getText(GENERAL_TEXT_SPLIT_OTHER_ROLLOVER));
        break;
    case DIALOG_RETURN_SPLIT_ACCEPT:
        sprintf(g_text,
                g_generalText->getText(GENERAL_TEXT_SPLIT_CREATURE_ROLLOVER_FORMAT),
                g_creatureTypeTraits[m_creature].m_pluralName);
        break;
    default:
        strcpy(g_text, "");
        break;
    }

    broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_TEXT, 8, int(g_text));
    drawWindow(0, 7, 8);
    g_windowManager->updateScreen(m_x + 8, m_y + 0x138, 0x11a, 0x11);
}

// E:\gamedcs\armygrp.cpp:229, dc 0x4e428
VA(0x0044a180, 0x2DF) MAC_ADDRESS(0x057634, 0x280)  // dc 0x4e428 (+ 0x4e388 inlined)
int TSplitWindow::windowHandler(message& msg)
{
    unsigned char closeDialog = false, updateArmy = false;
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;

    switch (msg.m_id) {
    case MESSAGE_WIDGET:
        switch (msg.m_codeX) {
        case widget::WIDGET_SELECT:
            msg.m_codeX = widget::WIDGET_GET_TEXT;
            broadcastMessage(msg);

            switch (msg.m_codeY) {
            case SPLIT_WIDGET_SOURCE_ENTRY:
                m_sourceTroops = atoi(msg.m_extraText);
                m_sourceTroops = limit(0, m_sourceTroops, m_totalTroops);
                m_destinationTroops = m_totalTroops - m_sourceTroops;
                m_destinationEntry->setFocus(0);
                break;

            case SPLIT_WIDGET_DESTINATION_ENTRY:
                m_destinationTroops = atoi(msg.m_extraText);
                m_destinationTroops = limit(
                    0, m_destinationTroops, m_totalTroops);
                m_sourceTroops = m_totalTroops - m_destinationTroops;
                m_sourceEntry->setFocus(0);
                break;

            }

            m_splitSlider->setState(m_destinationTroops);
            updateArmy = true;
            break;

        case widget::WIDGET_DESELECT:
            switch (msg.m_codeY) {
            case DIALOG_RETURN_SPLIT_CLOSE:
            case DIALOG_RETURN_SPLIT_CANCEL:
                g_windowManager->m_dialogReturn = msg.m_codeY;
                break;
            case DIALOG_RETURN_SPLIT_ACCEPT:
                g_windowManager->m_dialogReturn = DIALOG_RETURN_SPLIT_ACCEPT;
                break;
            default:
                return MESSAGE_DISPATCH_CONSUME;
            }
            closeDialog = true;
        }
        break;

    case MESSAGE_MOUSE_MOVE:
        g_windowManager->convertToHover(msg);
        if (msg.m_codeY != g_windowManager->m_lastHover) {
            g_windowManager->m_lastHover = msg.m_codeY;
            setRolloverText(msg.m_codeY);
        }
        return MESSAGE_DISPATCH_CONSUME;

    }

    if (closeDialog == true) {
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    if (updateArmy)
        updateSplitArmy(1);
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x0044a460, 0x55) MAC_ADDRESS(0x0578b4, 0x108)
const std::bitset<9>& armyGrpFn0044A460()
{
    static std::bitset<9> groupedAlignments;
    static unsigned char groupedAlignmentsBuilt = 0;
    if (!groupedAlignmentsBuilt) {
        groupedAlignments[TOWN_CASTLE] = true;
        groupedAlignments[TOWN_RAMPART] = true;
        groupedAlignments[TOWN_TOWER] = true;
        groupedAlignments[TOWN_FORTRESS] = true;
        groupedAlignments[TOWN_STRONGHOLD] = true;
        groupedAlignmentsBuilt = 1;
    }
    return groupedAlignments;
}

// E:\gamedcs\armygrp.cpp:341
// Retail's two compressed switch tables are at +0x418/+0x45c (spells)
// and +0x494/+0x4b8 (creatures). Their shared branch destinations matter:
// Resurrection only tests undead; Bless adds the damage-high test used by
// Fortune/Misfortune/Slayer; Precision shares Forgetfulness's shooter test.
// Stone tests the Troglodyte pair; Poison tests living and the Gargoyle pair.
// The old transcription grouped these entries by apparent similarity and
// also treated Diamond Golems as universally immune. The retail table puts
// them on the normal path, Green/Red/Azure Dragons on level <= 3, Gold on
// level <= 4, and Black/Magic Elemental on the unconditional zero path.

// DC lines 505..507 and retail +0x2bd..+0x2d5 make mind immunity a creature
// trait OR a hero's Badge of Courage. DC 564..565 and retail +0x325 apply
// GetMagicResistanceFactor after the entire creature switch, including its
// default arm; it is not confined to Dwarves. The arrow-tower rejection at
// retail +0xa7 is independent of the siege-weapon trait. These are behavior
// corrections, verified separately from the byte similarity score.

VA(0x0044a4d0, 0x52E) MAC_ADDRESS(0x0579bc, 0x574)  // linkorder, dc 0x4e644
float getSpellWorkChance(SpellID spell, TCreatureType targetArmyType, const hero* const castingHero, const hero* const targetHero)
{
    float chance;
    const TCreatureTypeTraits* creatureRec = &g_creatureTypeTraits[targetArmyType];
    unsigned int attrs = creatureRec->m_attributes;
    const SSpellTraits* spellRec = &g_spellTraits[spell];
    if (targetHero && spellRec->m_level <= 4
        && targetHero->isWieldingArtifact(ARTIFACT_POWER_OF_THE_DRAGON_FATHER))
        return 0.0f;  // Power of the Dragon Father
    if (spell == SPELL_DISPEL) {
        if (targetHero
            && targetHero->isWieldingArtifact(ARTIFACT_SPHERE_OF_PERMANENCE))
            return 0.0f;
        return 1.0f;
    }
    if (attrs & g_ctaSiegeWeapon) {
        if (spellRec->m_flags & 0x1000)
            return 0.0f;
    }
    if (targetArmyType == CREATURE_ARROW_TOWER)
        return 0.0f;
    switch (spell) {
    case SPELL_BLIND:
        if ((targetHero
                && targetHero->isWieldingArtifact(ARTIFACT_PENDANT_OF_SECOND_SIGHT))
            || targetArmyType == CREATURE_TROGLODYTE
            || targetArmyType == CREATURE_INFERNAL_TROGLODYTE
            || (attrs & g_ctaUndead))
            return 0.0f;
        break;
    case SPELL_BERSERK:
        if (targetHero
            && targetHero->isWieldingArtifact(ARTIFACT_PENDANT_OF_DISPASSION))
            return 0.0f;
        break;
    case SPELL_LIGHTNING_BOLT:
    case SPELL_CHAIN_LIGHTNING:
        if (targetHero
            && targetHero->isWieldingArtifact(ARTIFACT_PENDANT_OF_NEGATIVITY))
            return 0.0f;
        break;
    case SPELL_HYPNOTIZE:
        if (targetHero
            && targetHero->isWieldingArtifact(ARTIFACT_PENDANT_OF_FREE_WILL))
            return 0.0f;
        break;
    case SPELL_FORGETFULNESS:
        if (targetHero
            && targetHero->isWieldingArtifact(ARTIFACT_PENDANT_OF_TOTAL_RECALL))
            return 0.0f;
    case SPELL_PRECISION:
        if (!(attrs & 0x4))
            return 0.0f;
        break;
    case SPELL_CURSE:
        if (attrs & g_ctaUndead)
            return 0.0f;
        if (creatureRec->m_damageHighBound == 0)
            return 0.0f;
        if (targetHero
            && targetHero->isWieldingArtifact(ARTIFACT_PENDANT_OF_HOLINESS))
            return 0.0f;
        break;
    case SPELL_RESURRECTION:
        if (attrs & g_ctaUndead)
            return 0.0f;
        break;
    case SPELL_BLESS:
        if (attrs & g_ctaUndead)
            return 0.0f;
    case SPELL_FORTUNE:
    case SPELL_MISFORTUNE:
    case SPELL_SLAYER:
        if (creatureRec->m_damageHighBound == 0)
            return 0.0f;
        break;
    case SPELL_ANIMATE_DEAD:
        if (!(attrs & g_ctaUndead))
            return 0.0f;
        break;
    case SPELL_DEATH_RIPPLE:
        if ((attrs & g_ctaUndead)
            || (targetHero
                && targetHero->isWieldingArtifact(ARTIFACT_PENDANT_OF_LIFE)))
            return 0.0f;
        break;
    case SPELL_DESTROY_UNDEAD:
        if ((!(attrs & g_ctaUndead))
            || (targetHero
                && targetHero->isWieldingArtifact(ARTIFACT_PENDANT_OF_DEATH)))
            return 0.0f;
        break;
    case SPELL_MIRTH:
    case SPELL_SORROW:
        if (attrs & g_ctaNoMorale)
            return 0.0f;
        break;
    case SPELL_STONE:
        if (targetArmyType == CREATURE_TROGLODYTE || targetArmyType == CREATURE_INFERNAL_TROGLODYTE)
            return 0.0f;
        break;
    case SPELL_POISON:
        if (!(attrs & 0x10))
            return 0.0f;
        if (targetArmyType == CREATURE_STONE_GARGOYLE || targetArmyType == CREATURE_OBSIDIAN_GARGOYLE)
            return 0.0f;
        break;
    }
    {
        chance = 1.0f;
        if (!(castingHero
                && castingHero->isWieldingArtifact(ARTIFACT_ORB_OF_VULNERABILITY))
            && !(targetHero
                && targetHero->isWieldingArtifact(ARTIFACT_ORB_OF_VULNERABILITY))) {
            if (isMindSpell(spell)) {
                if ((attrs & 0x400)
                    || (targetHero
                        && targetHero->isWieldingArtifact(ARTIFACT_BADGE_OF_COURAGE)))
                    return 0.0f;
            }
            if ((attrs & 0x4000) && (spellRec->m_school & 0x2))
                return 0.0f;
            switch (targetArmyType) {
            case CREATURE_DWARF:
            case CREATURE_CRYSTAL_DRAGON:
                chance = 0.8f;
                break;
            case CREATURE_BATTLE_DWARF:
                chance = 0.6f;
                break;
            case CREATURE_AIR_ELEMENTAL:
            case CREATURE_STORM_ELEMENTAL:
                if (spell == SPELL_METEOR_SHOWER || spell == SPELL_BLIND)
                    return 0.0f;
                break;
            case CREATURE_EARTH_ELEMENTAL:
            case CREATURE_MAGMA_ELEMENTAL:
                if (spell == SPELL_ARMAGEDDON || spell == SPELL_LIGHTNING_BOLT
                    || spell == SPELL_CHAIN_LIGHTNING
                    || spell == SPELL_TITANS_LIGHTNING_BOLT)
                    return 0.0f;
                break;
            case CREATURE_WATER_ELEMENTAL:
            case CREATURE_ICE_ELEMENTAL:
                if (spell == SPELL_ICE_BOLT || spell == SPELL_FROST_RING)
                    return 0.0f;
                break;
            case CREATURE_GREEN_DRAGON:
            case CREATURE_RED_DRAGON:
            case CREATURE_AZURE_DRAGON:
                if (spellRec->m_level <= 3)
                    return 0.0f;
                break;
            case CREATURE_GOLD_DRAGON:
                if (spellRec->m_level <= 4)
                    return 0.0f;
                break;
            case CREATURE_BLACK_DRAGON:
            case CREATURE_MAGIC_ELEMENTAL:
                return 0.0f;
            }
            if (targetHero)
                chance -= 1.0f - targetHero->getMagicResistanceFactor();
        }
        if (spellRec->m_karma > 0) {
            return 1.0f;
        }
        if (chance < 0.0f)
            chance = 0.0f;
        return chance;
    }
}

VA(0x0044aa00, 0x3A) MAC_ADDRESS(0x057f30, 0x90)  // dc 0x4ea38
int armyGroup::save(TAbstractFile* outfile)
{
    if (outfile->write(m_armies, sizeof(m_armies)) < sizeof(m_armies))
        return -1;
    if (outfile->write(m_numTroops, sizeof(m_numTroops)) < sizeof(m_numTroops))
        return -1;
    return 0;
}

VA(0x0044aa40, 0x3A) MAC_ADDRESS(0x057fc0, 0x90)  // dc 0x4ea78
int armyGroup::load(TAbstractFile* infile)
{
    if (infile->read(m_armies, sizeof(m_armies)) < sizeof(m_armies))
        return -1;
    if (infile->read(m_numTroops, sizeof(m_numTroops)) < sizeof(m_numTroops))
        return -1;
    return 0;
}

VA(0x0044aa80, 0x1F) MAC_ADDRESS(0x058050, 0x30)  // dc 0x4eab8
armyGroup::armyGroup()
{
    initialize();
}

VA(0x0044aaa0, 0x5A) MAC_ADDRESS(0x058080, 0x8c)  // dc 0x4ead0
armyGroup::armyGroup(TCreatureType type, int amount)
{
    initialize();
    for (short i = 0;
            i < ARMY_GROUP_SLOT_COUNT && amount > 0; ++i) {
        m_armies[i] = type;
        int share = amount / (ARMY_GROUP_SLOT_COUNT - i);
        m_numTroops[i] = share;
        amount -= share;
    }
}

VA(0x0044ab00, 0x1D) MAC_ADDRESS(0x05810c, 0x40)  // dc 0x4eb2c
void armyGroup::initialize()
{
    memset(m_armies, 0xFF, sizeof(m_armies));
    memset(m_numTroops, 0, sizeof(m_numTroops));
}

VA(0x0044ab20, 0x3A) MAC_ADDRESS(0x05814c, 0x48)  // dc 0x4eb50
unsigned char armyGroup::hasAllUndead() const
{
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] == CREATURE_NONE)
            continue;
        if (!(g_creatureTypeTraits[m_armies[i]].m_attributes & g_ctaUndead))
            return 0;
    }
    return 1;
}

// E:\gamedcs\armygrp.cpp:668
// LINKER-ELIMINATED in retail: /Ob2 expands this member at its morale
// consumers and /OPT:REF drops the remaining copy, so the 0x4ab20..0x4ab80
// image gap contains Dismiss rather than this body.
MAC_ADDRESS(0x058194, 0x48)
unsigned char armyGroup::hasSomeUndead() const
{
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] == CREATURE_NONE)
            continue;
        if (g_creatureTypeTraits[m_armies[i]].m_attributes & g_ctaUndead)
            return 1;
    }
    return 0;
}

VA(0x0044ab60, 0x19) MAC_ADDRESS(0x0581dc, 0x1c)  // dc 0x4ebc0
void armyGroup::dismiss(int whichIndex)
{
    m_armies[whichIndex] = CREATURE_NONE;
    m_numTroops[whichIndex] = 0;
}

VA(0x0044ab80, 0x21) MAC_ADDRESS(0x0581f8, 0x94)  // dc 0x4ebd0
unsigned char armyGroup::isMember(TCreatureType monType) const
{
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] == monType)
            return 1;
    }
    return 0;
}

VA(0x0044abb0, 0x97) MAC_ADDRESS(0x05828c, 0x17c)  // dc 0x4ebf0
int armyGroup::getAlignments(unsigned char* alignments) const
{
    unsigned char local[10];
    if (!alignments)
        alignments = local;
    memset(alignments, 0, sizeof(local));
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] == CREATURE_NONE)
            continue;
        const TCreatureTypeTraits& traits = g_creatureTypeTraits[m_armies[i]];
        if (traits.m_attributes & g_ctaSiegeWeapon)
            continue;
        int alignment = g_game->getAlignment(m_armies[i]);
        alignments[alignment + 1]++;
    }
    int count = 0;
    for (int j = 0; j < 10; ++j) {
        if (alignments[j] > 0)
            ++count;
    }
    return count;
}

// Original: armyGroup::GetHomogeneityMoraleAdjust; armygrp.cpp:748, dc 0x4ec98
// Complete getMorale additionally groups allied alignments before applying
// this adjustment, so that path keeps its explicit alignment census.
int armyGroup::getHomogeneityMoraleAdjust() const
{
    return 2 - getAlignments(0);
}

VA(0x0044ac50, 0x2E) MAC_ADDRESS(0x058408, 0xcc)  // dc 0x4ecb0
int armyGroup::canJoin(int monType) const
{
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] == monType || m_armies[i] == CREATURE_NONE)
            return 1;
    }
    return 0;
}

VA(0x0044ac80, 0x39) MAC_ADDRESS(0x0584d4, 0x54)  // dc 0x4ecdc
long armyGroup::getAIValue() const
{
    long value = 0;
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] != CREATURE_NONE)
            value += g_creatureTypeTraits[m_armies[i]].m_aiValue * m_numTroops[i];
    }
    return value;
}

VA(0x0044acc0, 0x14) MAC_ADDRESS(0x058528, 0x7c)  // dc 0x4ed28
int armyGroup::getNumArmies() const
{
    int numArmies = 0;
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] != CREATURE_NONE)
            ++numArmies;
    }
    return numArmies;
}

VA(0x0044ace0, 0x76) MAC_ADDRESS(0x0585a4, 0x16c)  // dc 0x4ed4c
int armyGroup::add(int armyType, int newNumTroops, int newIndex)
{
    if (newIndex == -1) {
        for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
            if (m_armies[i] == armyType) {
                newIndex = i;
                break;
            }
        }
    }
    if (newIndex == -1) {
        for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
            if (m_armies[i] == CREATURE_NONE) {
                newIndex = i;
                break;
            }
        }
    }
    if (newIndex == -1)
        return 0;
    m_armies[newIndex] = armyType;
    if (m_numTroops[newIndex] < 0)
        m_numTroops[newIndex] = 0;
    m_numTroops[newIndex] += newNumTroops;
    return 1;
}

VA(0x0044ad60, 0x36) MAC_ADDRESS(0x058710, 0x3c)  // dc 0x4edcc
void armyGroup::swap(int srcIndex, armyGroup* destGroup, int destIndex)
{
    int army = m_armies[srcIndex];
    m_armies[srcIndex] = destGroup->m_armies[destIndex];
    destGroup->m_armies[destIndex] = army;
    short troops = m_numTroops[srcIndex];
    m_numTroops[srcIndex] = destGroup->m_numTroops[destIndex];
    destGroup->m_numTroops[destIndex] = troops;
}

// Original: armyGroup::DamageGroup; armygrp.cpp:885, dc 0x4ee08
void armyGroup::damageGroup(float casualtyRate)
{
    int limit = static_cast<int>(casualtyRate * 100.0);
    unsigned char first = 1;
    for (int slot = 0; slot < ARMY_GROUP_SLOT_COUNT; ++slot) {
        if (m_armies[slot] != CREATURE_NONE) {
            int casualties = 0;
            for (int troop = 0; troop < m_numTroops[slot]; ++troop) {
                if (sRandom(0, 100) < limit)
                    ++casualties;
            }
            if (first && casualties == m_numTroops[slot]
                && casualtyRate < 0.999)
                --casualties;
            m_numTroops[slot] -= casualties;
            if (m_numTroops[slot] <= 0 || casualtyRate >= 1.0) {
                m_numTroops[slot] = 0;
                m_armies[slot] = CREATURE_NONE;
            }
            first = 0;
        } else {
            m_numTroops[slot] = 0;
        }
    }
}

VA(0x0044ada0, 0x16) MAC_ADDRESS(0x05874c, 0x94)  // dc 0x4ef88
int armyGroup::getCreatureTotal() const
{
    int total = 0;
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armies[i] != -1)
            total += m_numTroops[i];
    }
    return total;
}

VA(0x0044adc0, 0x20) MAC_ADDRESS(0x0587e0, 0x94)  // dc 0x4efb8
int armyGroup::getCreatureTotal(TCreatureType monType) const
{
    int total = 0;
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armies[i] == monType)
            total += m_numTroops[i];
    }
    return total;
}

VA(0x0044ade0, 0x79) MAC_ADDRESS(0x058874, 0xd0)  // dc 0x4efec
const char* armyGroup::getArmySizeName(int howMany, int nameSet)
{
    if (howMany < 5)
        return g_armySizeNames[0][nameSet];
    if (howMany < 10)
        return g_armySizeNames[1][nameSet];
    if (howMany < 20)
        return g_armySizeNames[2][nameSet];
    if (howMany < 50)
        return g_armySizeNames[3][nameSet];
    if (howMany < 100)
        return g_armySizeNames[4][nameSet];
    if (howMany < 250)
        return g_armySizeNames[5][nameSet];
    if (howMany < 500)
        return g_armySizeNames[6][nameSet];
    if (howMany < 1000)
        return g_armySizeNames[7][nameSet];
    return g_armySizeNames[8][nameSet];
}

VA(0x0044ae60, 0x29A) MAC_ADDRESS(0x058944, 0x21c)  // dc 0x4f078
int armyGroup::getMorale(const hero* ownerHero, const town* ownerTown,
                         const hero* otherHero, const armyGroup* otherGroup,
                         unsigned char onCursedGround,
                         unsigned char groupAlignments,
                         unsigned char applyLimits) const
{
    if (onCursedGround)
        return 0;
    int morale = 0;
    if (ownerHero)
        morale = ownerHero->getMorale(otherHero, 0, 0);
    unsigned char alignments[10];
    int numAlignments = getAlignments(alignments);
    if (groupAlignments) {
        int grouped = 0;
        for (int a = -1; a < 9; ++a) {
            if (alignments[a + 1] > 0 && a != -1) {
                if (armyGrpFn0044A460().test(a))
                    ++grouped;
            }
        }
        if (grouped > 1)
            numAlignments += 1 - grouped;
    }
    morale += 2 - numAlignments;
    if (hasSomeUndead())
        morale--;
    if (isMember(CREATURE_ANGEL) || isMember(CREATURE_ARCHANGEL))
        morale++;
    if (otherGroup
        && (otherGroup->isMember(CREATURE_BONE_DRAGON)
            || otherGroup->isMember(CREATURE_GHOST_DRAGON)))
        morale--;
    if (ownerTown) {
        if (ownerTown->hasBuilding(TAVERN_ID, false))
            morale++;
        if (ownerTown->m_type == TOWN_CASTLE
            && ownerTown->hasBuilding(EXTRA_1_ID, true))
            morale += 2;
    }
    return applyLimits ? limit(-3, morale, 3) : morale;
}

// FULLY TRANSCRIBED 2026-08-06. Complete has SIX params (ret 0x18),
// extending the older DC five-argument API: (index, ownerHero, ownerTown, MODE
// 0, arg5, 0) - SEVEN pushes; Complete also adds the grouping argument
// to DC's six-argument GetMorale. mode==3 -> (elementals/f_1f698 gate) townType
VA(0x0044b100, 0x1C9) MAC_ADDRESS(0x058b60, 0x270)  // dc 0x4f160
int armyGroup::getArmyMorale(int index, const hero* ownerHero, const town* ownerTown, int mode, unsigned char arg5, unsigned char applyLimits) const
{
    if (mode == MAGIC_TERRAIN_CURSED_GROUND)
        return 0;
    if (g_creatureTypeTraits[m_armies[index]].m_attributes & g_ctaNoMorale)
        return 0;
    int morale = getMorale(ownerHero, ownerTown, 0, 0, 0, arg5, 0);
    if (mode == MAGIC_TERRAIN_HOLY_GROUND) {
        int type = m_armies[index];
        do {
            switch (g_game->getAlignment(type)) {
            case TOWN_CASTLE:
            case TOWN_RAMPART:
            case TOWN_TOWER:
                morale++;
                break;
            case TOWN_INFERNO:
            case TOWN_NECROPOLIS:
            case TOWN_DUNGEON:
                morale--;
                break;
            case TOWN_STRONGHOLD:
            case TOWN_FORTRESS:
            case TOWN_CONFLUX:
                continue;
            }
        } while (0);
    }
    do {
        if (mode == MAGIC_TERRAIN_EVIL_FOG) {
            int type = m_armies[index];
            switch (g_game->getAlignment(type)) {
            case TOWN_CASTLE:
            case TOWN_RAMPART:
            case TOWN_TOWER:
                morale--;
                break;
            case TOWN_INFERNO:
            case TOWN_NECROPOLIS:
            case TOWN_DUNGEON:
                morale++;
                break;
            case TOWN_STRONGHOLD:
            case TOWN_FORTRESS:
            case TOWN_CONFLUX:
                continue;
            }
        }
    } while (0);

    int type = m_armies[index];
    if ((type == CREATURE_MINOTAUR || type == CREATURE_MINOTAUR_KING)
        && morale < 1)
        morale = 1;
    if (ownerHero
        && ownerHero->isWieldingArtifact(ARTIFACT_SPIRIT_OF_OPPRESSION)
        && morale > 0)
        morale = 0;
    return applyLimits ? limit(-3, morale, 3) : morale;
}

VA(0x0044b2d0, 0xEB) MAC_ADDRESS(0x058dd0, 0x178)  // dc 0x4f20c
int armyGroup::getLuck(const hero* ownerHero, const town* ownerTown, const hero* otherHero, const armyGroup* otherGroup, unsigned char onCursedGround, unsigned char applyLimits) const
{
    if (onCursedGround)
        return 0;
    if ((ownerHero && const_cast<hero*>(ownerHero)
                          ->isWieldingArtifact(ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR))
        || (otherHero && const_cast<hero*>(otherHero)
                             ->isWieldingArtifact(ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR)))
        return 0;
    int luck = 0;
    if (ownerHero)
        luck = ownerHero->getLuck(otherHero, 0, 0);
    if (otherGroup
        && (otherGroup->isMember(CREATURE_DEVIL)
            || otherGroup->isMember(CREATURE_ARCH_DEVIL)))
        luck--;
    if (ownerTown && ownerTown->m_type == TOWN_RAMPART
        && ownerTown->hasBuilding(EXTRA_0_ID, true))
        luck += 2;
    if (applyLimits)
        return limit(-3, luck, 3);
    return luck;
}

VA(0x0044b3c0, 0xED) MAC_ADDRESS(0x058f48, 0x178)  // dc 0x4f2e8
int armyGroup::getArmyLuck(int index, const hero* ownerHero, const town* ownerTown, int mode, unsigned char applyLimits) const
{
    if (mode == MAGIC_TERRAIN_CURSED_GROUND)
        return 0;
    int luck = getLuck(ownerHero, ownerTown, 0, 0, 0, 0);
    if (mode == MAGIC_TERRAIN_CLOVER_FIELD) {
        int creature = m_armies[index];
        do {
            switch (g_game->getAlignment(creature)) {
            case TOWN_CASTLE:
            case TOWN_RAMPART:
            case TOWN_TOWER:
            case TOWN_INFERNO:
            case TOWN_NECROPOLIS:
            case TOWN_DUNGEON:
                continue;
            case TOWN_STRONGHOLD:
            case TOWN_FORTRESS:
            case TOWN_CONFLUX:
                luck += 2;
                break;
            default:
                break;
            }
        } while (0);
    }
    if (m_armies[index] == CREATURE_HALFLING && luck < 1)
        luck = 1;
    if (applyLimits)
        return limit(-3, luck, 3);
    return luck;
}

VA(0x0044b4b0, 0x162) MAC_ADDRESS(0x0590c0, 0x140)  // dc 0x4f328
long modifySpellDamage(long damage, SpellID spell, TCreatureType creature)
{
    switch (creature) {
    case CREATURE_AIR_ELEMENTAL:
    case CREATURE_STORM_ELEMENTAL:
        if (spell == SPELL_ARMAGEDDON || spell == SPELL_LIGHTNING_BOLT
            || spell == SPELL_CHAIN_LIGHTNING
            || spell == SPELL_TITANS_LIGHTNING_BOLT) {
            damage *= 2;
            break;
        }
        break;
    case CREATURE_FIRE_ELEMENTAL:
    case CREATURE_ENERGY_ELEMENTAL:
        if (spell == SPELL_ICE_BOLT || spell == SPELL_FROST_RING) {
            damage *= 2;
            break;
        }
        break;
    case CREATURE_EARTH_ELEMENTAL:
    case CREATURE_MAGMA_ELEMENTAL:
        if (spell == SPELL_METEOR_SHOWER) {
            damage *= 2;
            break;
        }
        break;
    case CREATURE_WATER_ELEMENTAL:
    case CREATURE_ICE_ELEMENTAL:
        if (spell == SPELL_FIREBALL || spell == SPELL_INFERNO
            || spell == SPELL_ARMAGEDDON) {
            damage *= 2;
            break;
        }
        break;
    case CREATURE_IRON_GOLEM:
        damage /= 4;
        break;
    case CREATURE_STONE_GOLEM:
        damage /= 2;
        break;
    case CREATURE_GOLD_GOLEM:
        damage = damage * 15 / 100;
        break;
    case CREATURE_DIAMOND_GOLEM:
        damage /= 20;
        break;
    }
    return damage;
}

VA(0x0044b620, 0x1FE) MAC_ADDRESS(0x059200, 0x3c4)  // dc 0x4f3cc
unsigned char armyGroup::merge(armyGroup* ag)
{
    armyGroup ag1;
    armyGroup ag2;
    int i;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        ag1.m_armies[i] = m_armies[i];
        ag1.m_numTroops[i] = m_numTroops[i];
        ag2.m_armies[i] = ag->m_armies[i];
        ag2.m_numTroops[i] = ag->m_numTroops[i];
    }
    i = 0;
    while (i < ARMY_GROUP_SLOT_COUNT) {
        if (ag2.m_armies[i] != CREATURE_NONE && ag2.m_numTroops[i] > 0) {
            int j = 0;
            while (j < ARMY_GROUP_SLOT_COUNT && ag2.m_armies[i] != ag1.m_armies[j])
                ++j;
            if (j < ARMY_GROUP_SLOT_COUNT) {
                ag1.m_numTroops[j] += ag2.m_numTroops[i];
                ag2.m_numTroops[i] = 0;
                ag2.m_armies[i] = CREATURE_NONE;
                ++i;
            } else {
                j = 0;
                while (j < ARMY_GROUP_SLOT_COUNT && ag1.m_numTroops[j] != 0)
                    ++j;
                if (j < ARMY_GROUP_SLOT_COUNT) {
                    ag1.m_numTroops[j] = ag2.m_numTroops[i];
                    ag1.m_armies[j] = ag2.m_armies[i];
                    ag2.m_numTroops[i] = 0;
                    ag2.m_armies[i] = CREATURE_NONE;
                    ++i;
                } else {
                    int a = 0;
                    unsigned char progress = 0;
                    while (a < ARMY_GROUP_SLOT_COUNT - 1 && !progress) {
                        int b = a;
                        while (++b < ARMY_GROUP_SLOT_COUNT && ag1.m_armies[a] != ag1.m_armies[b]) {
                        }
                        if (b < ARMY_GROUP_SLOT_COUNT) {
                            ag1.m_numTroops[a] += ag1.m_numTroops[b];
                            ag1.m_numTroops[b] = 0;
                            ag1.m_armies[b] = CREATURE_NONE;
                            progress = 1;
                        } else {
                            ++a;
                        }
                    }
                    if (!progress)
                        return 0;

                }
            }
        } else {
            ++i;
        }
    }
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        m_armies[i] = ag1.m_armies[i];
        m_numTroops[i] = ag1.m_numTroops[i];
        ag->m_armies[i] = ag2.m_armies[i];
        ag->m_numTroops[i] = ag2.m_numTroops[i];
    }
    return 1;
}

VA(0x0044b820, 0x140) MAC_ADDRESS(0x0595c4, 0x170)  // dc 0x4f5ec
void armyGroup::mergeArmies(armyGroup& source)
{
    for (;;) {
        int bestIndex = -1;
        int weakestIndex = -1;
        long bestGain = 0;
        int weakestValue = 0;
        for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
            if (m_armies[i] == CREATURE_NONE)
                break;
            long value = g_creatureTypeTraits[m_armies[i]].m_aiValue
                         * m_numTroops[i];
            if (weakestIndex < 0 || weakestValue >= value) {
                weakestValue = value;
                weakestIndex = i;
            }
        }
        for (int j = 0; j < ARMY_GROUP_SLOT_COUNT; ++j) {
            if (source.m_armies[j] == CREATURE_NONE)
                continue;
            long gain = g_creatureTypeTraits[source.m_armies[j]].m_aiValue
                        * source.m_numTroops[j];
            if (!canJoin(source.m_armies[j]))
                gain -= weakestValue;
            if (gain > bestGain) {
                bestGain = gain;
                bestIndex = j;
            }
        }
        if (bestIndex < 0)
            return;
        if (canJoin(source.m_armies[bestIndex])) {
            add(source.m_armies[bestIndex], source.m_numTroops[bestIndex], -1);
            source.dismiss(bestIndex);
        } else {
            std::swap(source.m_armies[bestIndex], m_armies[weakestIndex]);
            std::swap(source.m_numTroops[bestIndex], m_numTroops[weakestIndex]);
        }
    }
}

// E:\gamedcs\armygrp.cpp:1347, dc 0x4f708. Complete adds the full-width
// magic-terrain mode and alignment-grouping byte; ret 24h proves the hidden
// result and eight explicit parameters. DC lines 1347..1451 retain result,
// alignments[9], angel_type, the GetArmyName calls and nested modifier scopes.
// Complete's neutral alignment requires the ten-byte array below.

VA(0x0044b960, 0x859) MAC_ADDRESS(0x059734, 0x714)  // retail-body signature, dc 0x4f708
std::string armyGroup::getMoraleDescription(
    TCreatureType creature, int morale, const hero* ownerHero,
    const town* ownerTown, const hero* otherHero,
    const armyGroup* otherGroup, int magicTerrain,
    unsigned char groupAlignments) const
{
    if (magicTerrain == MAGIC_TERRAIN_CURSED_GROUND)
        return g_moraleInfo[27];

    // NOT a named `const TCreatureTypeTraits&`: retail's CSE keeps the
    // 116-byte OFFSET (it stores the `shl eax,2` result, not an address)
    // and re-adds the table base at each use through base+index
    // addressing (`test [eax+edx+0x10], mask`). Naming the row as a
    // reference makes VC6 materialise the ADDRESS instead - one extra
    // `add` per use, a stack slot of its own, and the table base loaded
    // BEFORE the index chain rather than after it.
    if (g_creatureTypeTraits[creature].m_attributes & g_ctaNoMorale)
        return g_moraleInfo[28];

    int currentMorale = getMorale(
        ownerHero, ownerTown, otherHero, otherGroup, 0,
        groupAlignments, 0);
    std::string result;

    if (ownerHero)
        result = ownerHero->getMoraleDescription();

    // Complete terrain arms: mutate the incoming morale home, then subtract
    // currentMorale at the tail, as proved by retail 0x44b960.
    // Mac 0x5983c and 0x59910 expand getAlignment separately in each arm.
    {
        if (magicTerrain == MAGIC_TERRAIN_HOLY_GROUND) {
            switch (g_game->getAlignment(creature)) {
            case TOWN_CASTLE:
            case TOWN_RAMPART:
            case TOWN_TOWER:
                goto holyGroundGood;
            case TOWN_INFERNO:
            case TOWN_NECROPOLIS:
            case TOWN_DUNGEON:
                goto holyGroundEvil;
            case TOWN_STRONGHOLD:
            case TOWN_FORTRESS:
            case TOWN_CONFLUX:
                goto moraleTerrainDone;
            }
            goto moraleTerrainDone;

        holyGroundGood:
            --morale;
            result += g_moraleInfo[39];
            goto moraleTerrainDone;

        holyGroundEvil:
            ++morale;
            result += g_moraleInfo[38];
            goto moraleTerrainDone;
        }
        if (magicTerrain == MAGIC_TERRAIN_EVIL_FOG) {
            switch (g_game->getAlignment(creature)) {
            case TOWN_CASTLE:
            case TOWN_RAMPART:
            case TOWN_TOWER:
                goto evilFogGood;
            case TOWN_INFERNO:
            case TOWN_NECROPOLIS:
            case TOWN_DUNGEON:
                goto evilFogEvil;
            case TOWN_STRONGHOLD:
            case TOWN_FORTRESS:
            case TOWN_CONFLUX:
                goto moraleTerrainDone;
            }
            goto moraleTerrainDone;

        evilFogGood:
            ++morale;
            result += g_moraleInfo[40];
            goto moraleTerrainDone;

        evilFogEvil:
            --morale;
            result += g_moraleInfo[41];
            goto moraleTerrainDone;
        }

    moraleTerrainDone:
        ;
    }

    unsigned char alignments[10];
    int numAlignments = getAlignments(alignments);
    if (groupAlignments) {
        int grouped = 0;
        for (int alignment = -1; alignment < 9; ++alignment) {
            if (alignments[alignment + 1] > 0 && alignment != -1) {
                if (armyGrpFn0044A460().test(alignment))
                    ++grouped;
            }
        }
        if (grouped > 1)
            numAlignments += 1 - grouped;
    }

    if (numAlignments >= 3) {
        int penalty = numAlignments >= 5 ? -3 : 2 - numAlignments;
        result += formatString(g_moraleInfo[29],
                                numAlignments, penalty);
    } else if (numAlignments == 1) {
        result += g_moraleInfo[30];
    }

    if (hasSomeUndead())
        result += g_moraleInfo[31];

    TCreatureType angelType;
    if (isMember(CREATURE_ANGEL) || isMember(CREATURE_ARCHANGEL)) {
        angelType = CREATURE_ANGEL;
        if (isMember(CREATURE_ARCHANGEL))
            angelType = CREATURE_ARCHANGEL;
        result += formatString(g_moraleInfo[32],
                                getArmyName(angelType, 2));
    }

    if (otherGroup) {
        TCreatureType dragonType = CREATURE_NONE;
        if (otherGroup->isMember(CREATURE_BONE_DRAGON))
            dragonType = CREATURE_BONE_DRAGON;
        if (otherGroup->isMember(CREATURE_GHOST_DRAGON))
            dragonType = CREATURE_GHOST_DRAGON;
        if (dragonType != CREATURE_NONE)
            result += formatString(
                g_moraleInfo[33],
                getArmyName(dragonType, 2));
    }

    if (ownerTown) {
        if (ownerTown->hasBuilding(TAVERN_ID, false))
            result += formatString(
                "\n%s +1", getBuildingName(ownerTown->m_type, TAVERN_ID));
        if (ownerTown->m_type == TOWN_CASTLE
            && ownerTown->hasBuilding(EXTRA_1_ID, true))
            result += formatString(
                "\n%s +2", getBuildingName(TOWN_CASTLE, EXTRA_1_ID));
    }

    if (creature == CREATURE_MINOTAUR
        || creature == CREATURE_MINOTAUR_KING) {
        if (currentMorale < 1) {
            result += formatString(g_moraleInfo[35],
                                    getArmyName(creature, 2));
            currentMorale = 1;
        }
    }

    if ((ownerHero && ownerHero->isWieldingArtifact(
                         ARTIFACT_SPIRIT_OF_OPPRESSION))
        || (otherHero && otherHero->isWieldingArtifact(
                            ARTIFACT_SPIRIT_OF_OPPRESSION))) {
        if (currentMorale > 0) {
            result = formatString(
                g_moraleInfo[34],
                g_artifactTraits[ARTIFACT_SPIRIT_OF_OPPRESSION].m_name);
            currentMorale = 0;
        }
    }

    morale -= currentMorale;
    if (morale)
        result += formatString(g_moraleInfo[36], morale);

    return result;
}

// E:\gamedcs\armygrp.cpp:1464. Retail Complete's body proves the added
// creature argument and full-width magic-terrain mode; the older Dreamcast
// prototype omits the creature argument and calls the terrain mode a boolean.
// Semantic transcription complete; residual 82.5689%. The bounded
// variable-creature name lookup raised the body to 74.7874%; a function-wide
// shared result regresses.

// What is left is a pure inline-depth divergence, 41 conditional branches
// against retail's 33: retail CALLS basic_string::assign(const char*,
// size_t) where we expand it into _Grow + rep movs + _Eos, and the same one
// level too deep repeats at the other string sites.

// THE HERO ARM ASSIGNS, IT DOES NOT APPEND (byte-flat, 2026-08-14). The
// Dreamcast line table says so directly - dc 0x4fab4 line 1482 reaches
// `basic_string::operator=` and no operator+= - and retail's own call at
// that slot is `assign(const basic_string&, uint, uint)` (0x404860) where
// ours was `append(...)` (0x41b250); every other call in the body already
// lined up 1:1. `result` is empty there, so the two are behaviourally the
// same and the argument sequence is identical - only the relocation target
// differs, which objdiff does not score. Recorded because it is what retail
// wrote and because the call multiset is what `predict-inline` reads.

// Complete adds the clover-field and halfling arms to the older DC body.
// Their missing DC lines do not recover absent statements. Keep the shared
// isBaseElemental, GetArmyName and HasBuilding helpers already used below.
// DC line 1499 positively calls HasBuilding(EXTRA_0_ID, true).
// Earlier flattened lookup controls and artificial inline-budget probes
// changed string expansion decisions, but do not establish alternate helper
// declarations or justify adding candidate sites to this source.

// [2026-08-21] +5.69 (84.5060 -> 90.1916) from the nine-town switch routing
// below, and the two sides' instruction counts now agree exactly (332 = 332,
// one `ret` each). What is left is a parameter-home family: retail spills
// `currentLuck` into the dead `ourHero` parameter slot ([ebp+0x14]) the
// moment GetLuck returns while this compile keeps it in EBX for the whole
// body, and retail's empty-allocator scratch byte sits in the [ebp+0x10]
// slot's high byte ([ebp+0x13]) where ours sits in the [ebp+0xc] slot's
// ([ebp+0xf]). By the recorded scratch-slot rule that second fact reads as
// retail's SECOND parameter being narrower than the `int luck` modelled
// here - worth testing, but it is a non-additive change to a declarator
// viewarmywindow.cpp also calls, so it needs an owner who can re-measure
// that unit's rows in the same build.
// [2026-08-26] Scoping a signed-byte snapshot of `ourTown->type` under the
// nonnull gate raises 90.1916 -> 92.3623. The cache preserves retail's 33
// conditional branches and symbolic branch targets. A generated one-line
// town-type accessor reaches the same bytes, but no such accessor is attested
// in the Dreamcast class record; the ordinary local is retained instead.
// Dreamcast 0x4fab4:1499 calls town::HasBuilding for the Rampart
// Fountain of Fortune check (building 21, built-only flag 1).
VA(0x0044c1c0, 0x3C5) MAC_ADDRESS(0x059e48, 0x3a8)  // retail-body signature, dc 0x4fab4
std::string armyGroup::getLuckDescription(
    TCreatureType creature, int luck, const hero* ourHero,
    const town* ourTown, const hero* enemyHero,
    const armyGroup* enemyGroup, int magicTerrain) const
{
    if (magicTerrain == MAGIC_TERRAIN_CURSED_GROUND)
        return g_luckInfo[22];

    if ((ourHero && ourHero->isWieldingArtifact(
                        ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR))
        || (enemyHero && enemyHero->isWieldingArtifact(
                           ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR))) {
        return formatString(g_luckInfo[23],
                             g_artifactTraits[
                                 ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR].m_name);
    }

    int currentLuck = getLuck(
        ourHero, ourTown, enemyHero, enemyGroup, 0, 0);
    std::string result;
    if (ourHero)
        result = ourHero->getLuckDescription();

    // Complete adds the clover-field luck bonus before applying enemy-group
    // modifiers. Dreamcast has only the cursed-ground terrain parameter.
    // Mac 0x59f68 expands getAlignment before the town switch.
    if (magicTerrain == MAGIC_TERRAIN_CLOVER_FIELD) {
        // Nine town values routed to NAMED exits, the recipe GetArmyMorale
        // (0x44b100) already carries: retail lowers this arm through a
        // compressed byte selector - `cmp eax,8 / ja <default> / xor ecx,ecx
        // / mov cl,[bytetable] / jmp [4*ecx + jumptable]` - and that only
        // survives with the neutral cases naming their exit. The bonus can
        // live in its own arm and default can break, removing two gotos
        // without changing any score in the 36-state family. Spelled with `return`
        // in the no-op arms VC6 sees two outcomes, collapses the whole
        // switch, and emits the range test `cmp 6 / jl` + `cmp 8 / jg`
        // instead of the tables.
        switch (g_game->getAlignment(creature)) {
        case TOWN_CASTLE:
        case TOWN_RAMPART:
        case TOWN_TOWER:
        case TOWN_INFERNO:
        case TOWN_NECROPOLIS:
        case TOWN_DUNGEON:
            goto clover_done;
        case TOWN_STRONGHOLD:
        case TOWN_FORTRESS:
        case TOWN_CONFLUX:
            luck -= 2;
            result += g_luckInfo[24];
            break;
        default:
            break;
        }
    clover_done:
        ;
    }

    if (enemyGroup) {
        TCreatureType devilType = CREATURE_NONE;
        if (enemyGroup->isMember(CREATURE_DEVIL))
            devilType = CREATURE_DEVIL;
        if (enemyGroup->isMember(CREATURE_ARCH_DEVIL))
            devilType = CREATURE_ARCH_DEVIL;
        if (devilType != CREATURE_NONE)
            result += formatString(g_moraleInfo[33],
                                    getArmyName(devilType, 2));
    }

    if (ourTown) {
        char ourTownType = ourTown->m_type;
        if (ourTownType == TOWN_RAMPART
            && ourTown->hasBuilding(EXTRA_0_ID, true)) {
            result += formatString(
                "\n%s +2", getBuildingName(TOWN_RAMPART, EXTRA_0_ID));
        }
    }

    if (creature == CREATURE_HALFLING && currentLuck < 1) {
        result += formatString("%s are always lucky",
                                getArmyName(creature, 2));
        currentLuck = 1;
    }

    luck -= currentLuck;
    if (luck)
        result += formatString(g_moraleInfo[36], luck);

    return result;
}

VA(0x0044c590, 0x76) MAC_ADDRESS(0x05a1f0, 0x90)  // dc 0x4fc98
TTerrainType armyGroup::getNativeTerrain() const
{
    TTerrainType native = TERRAIN_NONE;
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] == CREATURE_NONE)
            continue;
        int alignment = g_game->getAlignment(m_armies[i]);
        TTerrainType terrain = townManager::getNativeTerrain(alignment);
        if (native != TERRAIN_NONE) {
            if (terrain != native)
                return TERRAIN_NONE;
        } else
            native = terrain;
    }
    return native;
}

// COMDAT pairing: bitset<9>::reference::operator=, agreement 0.922; the
// neighbouring 0x4c680 row scores 0.600 against the same COMDAT.
VA_COMPGEN(0x0044c610, 0x67, BITSET_REFERENCE_ASSIGN, bitset9)

// COMDAT pairing: the same bitset<9> instantiation's _Tidy, 23 B against
// this compiland's single 23-byte COMDAT.
VA_COMPGEN(0x0044c6e0, 0x17, BITSET_TIDY, bitset9)
