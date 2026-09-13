// armygrp.cpp - E:\gamedcs\armygrp.cpp (compiland armygrp.obj)
#include "homm3_limit.h"
#include <va.h>
#include <algorithm>
#include <stdlib.h>
#include <string.h>
// VC6's own shipped Dinkumware <bitset> - retail links Dinkumware, NOT
// STLport (P2.3, byte-proven by hero.obj's COMDAT tail and again here:
// GetMorale's four helper calls are Dinkumware bitset members to the
// instruction).
#include <bitset>
// get_luck_description's Rampart/Fountain-of-Fortune gate calls
// town::HasBuilding (dc 0x4fab4 line 1499, `mov #21,r5 / mov #1,r6`);
// see town.h for why the inline's visibility is scoped.
#include "armygrp.h"
#include "game.h"
#include "hero.h"
#include "town.h"
#include "advmgr.h"
#include "castle.h"
#include "border.h"
#include "button.h"
#include "iconwdgt.h"
#include "widget.h"
#include "textntry.h"
#include "textwdgt.h"
#include "message.h"
#include "kb.h"
#include "winmgr.h"
#include "exec.h"
#include "misc.h"
#include "armygrp_split.h"

// DC includes.h:134 names limit at the split-window and army-rating
// sites below. homm3_limit.h owns its shared reference-selector chain.

// The four-way base-elemental compare the magic-terrain gates share -
// eight of them in this compiland, in both polarities. Byte-identical to
// the longhand chain when expanded (viewarmywindow's own copy is the
// byte-proven precedent, and the full-tree diff over all eight sites here
// has ONE mover and no regressions: TSplitWindow 98.4605 -> 99.9895) and
// free at cb <= 0x28, so it costs the /Ob2 allowance one candidate site
// per use - which is the point, since three of this file's plateaued rows
// are measured to be short of exactly that.
inline bool isBaseElemental(int type)
{
    return type == CREATURE_AIR_ELEMENTAL || type == CREATURE_EARTH_ELEMENTAL
        || type == CREATURE_FIRE_ELEMENTAL || type == CREATURE_WATER_ELEMENTAL;
}

inline int creatureBackgroundAlignment(TCreatureType type)
{
    if (!g_game->m_f1f698 && isBaseElemental(type))
        return -1;
    return g_creatureTypeTraits[type].m_townType;
}

inline const char* armygrpCreaturePluralName(TCreatureType creature)
{
    if (creature >= 0 && creature <= 150)
        return g_creatureTypeTraits[creature].m_pluralName;
    return "";
}

DATA(0x00693878)
static TSplitWindow* g_splitWindow;

// Runtime-loaded combat-stat description lines. Their storage addresses and
// uses are retail-proven here; the text-resource loader owns the definitions.
DATA(0x006a5384) extern const char* g_cursedGroundLuckText;
DATA(0x006a5388) extern const char* g_hourglassLuckFormat;
DATA(0x006a538c) extern const char* g_cloverFieldLuckText;
DATA(0x006a5828) extern const char* g_cursedGroundMoraleText;
DATA(0x006a582c) extern const char* g_noMoraleCreatureText;
DATA(0x006a5830) extern const char* g_alignmentMoraleFormat;
DATA(0x006a5834) extern const char* g_sameAlignmentMoraleText;
DATA(0x006a5838) extern const char* g_undeadMoraleText;
DATA(0x006a583c) extern const char* g_angelMoraleFormat;
DATA(0x006a5840) extern const char* g_enemyCreatureStatFormat;
DATA(0x006a5844) extern const char* g_spiritOppressionMoraleFormat;
DATA(0x006a5848) extern const char* g_alwaysPositiveMoraleFormat;
DATA(0x006a584c) extern const char* g_otherStatModifiersFormat;
DATA(0x006a5854) extern const char* g_holyGroundEvilMoraleText;
DATA(0x006a5858) extern const char* g_holyGroundGoodMoraleText;
DATA(0x006a585c) extern const char* g_evilFogGoodMoraleText;
DATA(0x006a5860) extern const char* g_evilFogEvilMoraleText;

inline void TSplitWindow::updateSplitArmy(unsigned char update)
{
    message msg;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;

    sprintf(g_text, "%d", m_sourceTroops);
    msg.m_codeY = 4;
    msg.m_extraText = g_text;
    broadcastMessage(&msg);

    sprintf(g_text, "%d", m_destinationTroops);
    msg.m_codeY = 5;
    msg.m_extraText = g_text;
    broadcastMessage(&msg);

    if (update)
        drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

// Unimplemented carcass stubs stay lexically present (labels and the
// va-claims gate scan text) but outside compilation until each body
// is reconstructed.
#if 0  // @carcass

#endif  // @carcass

VA(0x004496a0, 0x16)  // dc 0x4dae4
unsigned char armyGroup::hasCreatures() const
{
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] != CREATURE_NONE)
            return 1;
    }
    return 0;
}

// ===================================================================
// TSplitWindow bracket RE-MAPPED 2026-08-07.  The carcass generator
// paired seven DC rows with the seven retail slots 1:1 in order; the
// bytes say the pairing slips by one from 0x449790 on, because TWO DC
// rows (UpdateSplitArmy, SetRolloverText) have NO retail slot and one
// retail slot (0x44a460) has no DC row.  Every correction below is
// proven from the target bytes, not from the order:

//   0x4496c0  ret with NO stack cleanup and ecx used as an INTEGER
//     (`movsx edx,[gSplitWindow+0x78]; add edx,ecx`) - a /Gr FREE
//     function of one int, so it cannot be the member
//     UpdateSplitArmy(uchar) the old claim named (a member would use
//     ecx as `this`, and it reads the window from the file-scope
//     0x693878 instead).  It is SplitSliderCallback with
//     UpdateSplitArmy inlined into it: the body sets
//     w->f74 = w->b78 + state, w->f70 = w->f6c - w->f74, sprintf()s
//     both counts and BroadcastMessage()s them.  DC 48 B + 128 B
//     inlined = 176 vs retail 195 (1.11x); the standalone 48 B row
//     alone would have been 4.06x, outside the band.
//   0x449790  the CONSTRUCTOR, proven by its call site: SplitArmy at
//     0x449eb1 does `new(0x80)` and then a THISCALL here with the
//     three stack args (0xb1, 0x14, armies[srcIndex]) - exactly
//     TSplitWindow(int x2, int y2, TCreatureType).  DC 1380 vs 1627
//     (1.18x).
//   0x449df0  `ret 4`, and the body is call-dtor / `test byte
//     [ebp+8],1` / conditional operator delete / return this - the
//     textbook scalar deleting destructor, which is the DC's own
//     0x4fd54 row.  The old claim called it the three-argument ctor;
//     a ctor with three args would be `ret 0xc`.
//   0x44a180  `ret 4` with a message* arg, calls the base popup's
//     slot-9 handler first and then dispatches on msg->type /
//     msg->field_4 / control ids 0x7800..0x7802 - the window handler,
//     with SetRolloverText inlined (DC 540 + 160 = 700 vs 735, 1.05x).
//   0x44a460  NOT TSplitWindow::WindowHandler: no arguments, no
//     `this`, `ret` with no cleanup, and it returns the address of the
//     static bitset<9> at 0x693884 that GetMorale reads.  It is
//     armygrp.cpp's static-set accessor, promoted out of this block
//     below.  It has no DC row because the DC build linked STLport.

// UpdateSplitArmy and SetRolloverText are therefore inlined-and-
// eliminated (the HasSomeUndead pattern in this same file): single
// call site, /Ob2 inlines, /OPT:REF drops the now-unreferenced body.
// ===================================================================
#if 0  // @carcass

// E:\gamedcs\armygrp.cpp:82
#endif  // @carcass

VA(0x004496c0, 0xC3)  // dc 0x4db88
void splitSliderCallback(int state, heroWindow*)
{
    g_splitWindow->m_destinationTroops =
        g_splitWindow->m_minimumTransfer + state;
    g_splitWindow->m_sourceTroops =
        g_splitWindow->m_totalTroops - g_splitWindow->m_destinationTroops;
    g_splitWindow->updateSplitArmy(1);
}

VA(0x00449790, 0x65B)  // dc 0x4dbb8
TSplitWindow::TSplitWindow(int x2, int y2, TCreatureType thisArmy)
    : CAdvPopup(x2, y2, 0x12a, 0x151, 0x12)
{
    m_creature = thisArmy;
    m_widgets.reserve(13);

    m_widgets.push_back(new bitmapBorder(
        0, 0, m_width, m_height, 0, "GPuCrDiv.pcx", 0x800));

    sprintf(g_text,
            (*g_generalText)[GENERAL_TEXT_SPLIT_CREATURE_ROLLOVER],
            g_creatureTypeTraits[m_creature].m_pluralName);
    m_widgets.push_back(new textWidget(
        0, 20, m_width, 30, g_text, "bigfont.fnt", font::HEADING,
        1, 1, 0, 8));

    strcpy(g_text, g_creatureBackgrounds[
        creatureBackgroundAlignment(m_creature)]);

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

VA(0x00449e20, 0x6B)  // dc 0x4e11c
TSplitWindow::~TSplitWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

#if 0  // @carcass

// E:\gamedcs\armygrp.cpp:143
#endif  // @carcass

VA(0x00449e90, 0x2EF)  // dc 0x4e180
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
    g_splitWindow->broadcastMessage(&msg);

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
    g_splitWindow->broadcastMessage(&msg);

    if (inDestRestricted && ag->getNumArmies() == 1)
        g_splitWindow->m_minimumTransfer = 1;
    else
        g_splitWindow->m_minimumTransfer = 0;

    msg.m_codeX = widget::WIDGET_SET_SLIDER_STATE;
    msg.m_extra = g_splitWindow->m_destinationTroops
        - g_splitWindow->m_minimumTransfer;
    g_splitWindow->broadcastMessage(&msg);

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
// /OPT:REF removes the out-of-line copy.
inline void TSplitWindow::setRolloverText(int codeY)
{
    switch (codeY) {
    case DIALOG_RETURN_SPLIT_CANCEL:
        sprintf(g_text,
                (*g_generalText)[GENERAL_TEXT_SPLIT_OTHER_ROLLOVER]);
        break;
    case DIALOG_RETURN_SPLIT_ACCEPT:
        sprintf(g_text,
                (*g_generalText)[GENERAL_TEXT_SPLIT_CREATURE_ROLLOVER],
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

VA(0x0044a180, 0x2DF)  // dc 0x4e428
int TSplitWindow::windowHandler(message* msg)
{
    unsigned char closeDialog = false, updateArmy = false;
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;

    switch (msg->m_id) {
    case MESSAGE_WIDGET:
        switch (msg->m_codeX) {
        case widget::WIDGET_SELECT:
            msg->m_codeX = widget::WIDGET_GET_TEXT;
            broadcastMessage(msg);

            switch (msg->m_codeY) {
            case SPLIT_WIDGET_SOURCE_ENTRY:
                m_sourceTroops = atoi(msg->m_extraText);
                m_sourceTroops = limit(0, m_sourceTroops, m_totalTroops);
                m_destinationTroops = m_totalTroops - m_sourceTroops;
                m_destinationEntry->setFocus(0);
                break;

            case SPLIT_WIDGET_DESTINATION_ENTRY:
                m_destinationTroops = atoi(msg->m_extraText);
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
            switch (msg->m_codeY) {
            case DIALOG_RETURN_SPLIT_CLOSE:
            case DIALOG_RETURN_SPLIT_CANCEL:
                g_windowManager->m_dialogReturn = msg->m_codeY;
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
        g_windowManager->convertToHover(*msg);
        if (msg->m_codeY != g_windowManager->m_lastHover) {
            g_windowManager->m_lastHover = msg->m_codeY;
            setRolloverText(msg->m_codeY);
        }
        return MESSAGE_DISPATCH_CONSUME;

    }

    if (closeDialog == true) {
        msg->m_codeY = widget::WIDGET_END_DIALOG;
        msg->m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    if (updateArmy)
        updateSplitArmy(1);
    return MESSAGE_DISPATCH_CONSUME;
}

#if 0  // @carcass

// LINKER-ELIMINATED in retail (inlined at their single call sites,
// then dropped by /OPT:REF - the HasSomeUndead pattern):
//   E:\gamedcs\armygrp.cpp:62   TSplitWindow::UpdateSplitArmy(uchar)
//     -> inlined into SplitSliderCallback (0x4496c0)
DC_ONLY(0x4db08, 0x80)
void TSplitWindow::updateSplitArmy(unsigned char bUpdate)
{
    // @stub
}

//   E:\gamedcs\armygrp.cpp:208  TSplitWindow::SetRolloverText(int)
//     -> reconstructed above and inlined into WindowHandler (0x44a180)
DC_ONLY(0x4e388, 0xA0)
void TSplitWindow::setRolloverText(int codeY)
{
    // @stub
}

#endif  // @carcass

VA(0x0044a460, 0x55)
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

// Restore the separate IsWieldingArtifact calls attested by DC 379..486;
// VC6 merges the shared pendant tail itself. Together with the corrected
// scopes this removes all seven gotos and improves 88.5071% to 95.8839%.
// Restoring the header IsMindSpell accessor called at DC 505 raises this
// further to 96.6018%. Mask and shift accessor expressions emit the same
// code, and all 95 header consumers were measured without collateral loss.
// The same corrected behavior with the old pendant join scores 76.8554%;
// a post-switch artifact selector scores 75.4607%. Initializing chance at
// function entry gives 95.0429%; keep its assignment after the spell gates.
VA(0x0044a4d0, 0x52E)  // linkorder, dc 0x4e644
float getSpellWorkChance(SpellID spell, TCreatureType targetArmyType, const hero* castingHero, const hero* targetHero)
{
    float chance;
    const TCreatureTypeTraits* creatureRec = &g_creatureTypeTraits[targetArmyType];
    const SSpellTraits* spellRec = &g_spellTraits[spell];
    unsigned int attrs = creatureRec->m_attributes;
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
        if (targetHero
            && targetHero->isWieldingArtifact(ARTIFACT_PENDANT_OF_SECOND_SIGHT))
            return 0.0f;
        if (targetArmyType == CREATURE_TROGLODYTE || targetArmyType == CREATURE_INFERNAL_TROGLODYTE)
            return 0.0f;
        if (attrs & g_ctaUndead)
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
        if (attrs & g_ctaUndead)
            return 0.0f;
        if (targetHero
            && targetHero->isWieldingArtifact(ARTIFACT_PENDANT_OF_LIFE))
            return 0.0f;
        break;
    case SPELL_DESTROY_UNDEAD:
        if (!(attrs & g_ctaUndead))
            return 0.0f;
        if (targetHero
            && targetHero->isWieldingArtifact(ARTIFACT_PENDANT_OF_DEATH))
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
                chance -= 1.0f - const_cast<hero*>(targetHero)
                                         ->getMagicResistanceFactor();
        }
        if (spellRec->m_karma > 0) {
            return 1.0f;
        }
        if (chance < 0.0f)
            chance = 0.0f;
        return chance;
    }
}

VA(0x0044aa00, 0x3A)  // dc 0x4ea38
int armyGroup::save(TAbstractFile* outfile)
{
    if (outfile->write(m_armies, sizeof(m_armies)) < sizeof(m_armies))
        return -1;
    if (outfile->write(m_numTroops, sizeof(m_numTroops)) < sizeof(m_numTroops))
        return -1;
    return 0;
}

#if 0  // @carcass

#endif  // @carcass

VA(0x0044aa40, 0x3A)  // dc 0x4ea78
int armyGroup::load(TAbstractFile* infile)
{
    if (infile->read(m_armies, sizeof(m_armies)) < sizeof(m_armies))
        return -1;
    if (infile->read(m_numTroops, sizeof(m_numTroops)) < sizeof(m_numTroops))
        return -1;
    return 0;
}

#if 0  // @carcass

#endif  // @carcass

VA(0x0044aa80, 0x1F)  // dc 0x4eab8
armyGroup::armyGroup()
{
    memset(m_armies, 0xFF, sizeof(m_armies));
    memset(m_numTroops, 0, sizeof(m_numTroops));
}

#if 0  // @carcass

#endif  // @carcass

VA(0x0044aaa0, 0x5A)  // dc 0x4ead0
armyGroup::armyGroup(TCreatureType type, int amount)
{
    memset(m_armies, 0xFF, sizeof(m_armies));
    memset(m_numTroops, 0, sizeof(m_numTroops));
    for (short i = 0;
            i < ARMY_GROUP_SLOT_COUNT && amount > 0; ++i) {
        m_armies[i] = type;
        int share = amount / (ARMY_GROUP_SLOT_COUNT - i);
        m_numTroops[i] = share;
        amount -= share;
    }
}

#if 0  // @carcass

#endif  // @carcass

VA(0x0044ab00, 0x1D)  // dc 0x4eb2c
void armyGroup::initialize()
{
    memset(m_armies, 0xFF, sizeof(m_armies));
    memset(m_numTroops, 0, sizeof(m_numTroops));
}

#if 0  // @carcass

#endif  // @carcass

VA(0x0044ab20, 0x3A)  // dc 0x4eb50
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

VA(0x0044ab60, 0x19)  // dc 0x4ebc0
void armyGroup::dismiss(int whichIndex)
{
    m_armies[whichIndex] = CREATURE_NONE;
    m_numTroops[whichIndex] = 0;
}

#if 0  // @carcass

#endif  // @carcass

VA(0x0044ab80, 0x21)  // dc 0x4ebd0
unsigned char armyGroup::isMember(TCreatureType monType) const
{
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] == monType)
            return 1;
    }
    return 0;
}

#if 0  // @carcass

#endif  // @carcass

VA(0x0044abb0, 0x97)  // dc 0x4ebf0
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
        int alignment;
        if (g_game->m_f1f698 == 0 && isBaseElemental(m_armies[i]))
            alignment = -1;
        else
            alignment = traits.m_townType;
        alignments[alignment + 1]++;
    }
    int count = 0;
    for (int j = 0; j < 10; ++j) {
        if (alignments[j] > 0)
            ++count;
    }
    return count;
}

#if 0  // @carcass

// E:\gamedcs\armygrp.cpp:748
DC_ONLY(0x4ec98, 0x16)
int armyGroup::GetHomogeneityMoraleAdjust() const
{
    // @stub
}

#endif  // @carcass

VA(0x0044ac50, 0x2E)  // dc 0x4ecb0
int armyGroup::canJoin(int monType) const
{
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] == monType || m_armies[i] == CREATURE_NONE)
            return 1;
    }
    return 0;
}

#if 0  // @carcass

#endif  // @carcass

VA(0x0044ac80, 0x39)  // dc 0x4ecdc
long armyGroup::getAIValue() const
{
    long value = 0;
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] != CREATURE_NONE)
            value += g_creatureTypeTraits[m_armies[i]].m_aiValue * m_numTroops[i];
    }
    return value;
}

#if 0  // @carcass

#endif  // @carcass

VA(0x0044acc0, 0x14)  // dc 0x4ed28
int armyGroup::getNumArmies() const
{
    int numArmies = 0;
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] != CREATURE_NONE)
            ++numArmies;
    }
    return numArmies;
}

#if 0  // @carcass

#endif  // @carcass

VA(0x0044ace0, 0x76)  // dc 0x4ed4c
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

#if 0  // @carcass

#endif  // @carcass

VA(0x0044ad60, 0x36)  // dc 0x4edcc
void armyGroup::swap(int srcIndex, armyGroup* destGroup, int destIndex)
{
    int army = m_armies[srcIndex];
    m_armies[srcIndex] = destGroup->m_armies[destIndex];
    destGroup->m_armies[destIndex] = army;
    short troops = m_numTroops[srcIndex];
    m_numTroops[srcIndex] = destGroup->m_numTroops[destIndex];
    destGroup->m_numTroops[destIndex] = troops;
}

#if 0  // @carcass

// E:\gamedcs\armygrp.cpp:885
DC_ONLY(0x4ee08, 0x180)
void armyGroup::DamageGroup(float casualtyRate)
{
    // @stub
}

#endif  // @carcass

VA(0x0044ada0, 0x16)  // dc 0x4ef88
int armyGroup::getCreatureTotal() const
{
    int total = 0;
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armies[i] != -1)
            total += m_numTroops[i];
    }
    return total;
}

VA(0x0044adc0, 0x20)  // dc 0x4efb8
int armyGroup::getCreatureTotal(TCreatureType monType) const
{
    int total = 0;
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_armies[i] == monType)
            total += m_numTroops[i];
    }
    return total;
}

VA(0x0044ade0, 0x79)  // dc 0x4efec
const char* armyGroup::getArmySizeName(int howMany, int nameSet)
{
    if (howMany < 5)
        return g_apszArmySizeNames[0][nameSet];
    if (howMany < 10)
        return g_apszArmySizeNames[1][nameSet];
    if (howMany < 20)
        return g_apszArmySizeNames[2][nameSet];
    if (howMany < 50)
        return g_apszArmySizeNames[3][nameSet];
    if (howMany < 100)
        return g_apszArmySizeNames[4][nameSet];
    if (howMany < 250)
        return g_apszArmySizeNames[5][nameSet];
    if (howMany < 500)
        return g_apszArmySizeNames[6][nameSet];
    if (howMany < 1000)
        return g_apszArmySizeNames[7][nameSet];
    return g_apszArmySizeNames[8][nameSet];
}

VA(0x0044ae60, 0x29A)  // dc 0x4f078
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
        if (ownerTown->hasBuilding(TAVERN_ID, 0))
            morale++;
        if (ownerTown->m_type == TOWN_CASTLE
            && ownerTown->hasBuilding(EXTRA_1_ID, 1))
            morale += 2;
    }
    return applyLimits ? limit(-3, morale, 3) : morale;
}

VA(0x0044b100, 0x1C9)  // dc 0x4f160
int armyGroup::getArmyMorale(int index, const hero* ownerHero, const town* ownerTown, int mode, unsigned char arg5, unsigned char applyLimits) const
{
    if (mode == MAGIC_TERRAIN_CURSED_GROUND)
        return 0;
    if (g_creatureTypeTraits[m_armies[index]].m_attributes & g_ctaNoMorale)
        return 0;
    int morale = getMorale(ownerHero, ownerTown, 0, 0, 0, arg5, 0);
    if (mode == MAGIC_TERRAIN_HOLY_GROUND) {
        int type = m_armies[index];
        if (g_game->m_f1f698 != 0 || !isBaseElemental(type)) {
            do {
                switch (g_creatureTypeTraits[type].m_townType) {
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
    }
    do {
        if (mode == MAGIC_TERRAIN_EVIL_FOG) {
            int type = m_armies[index];
            if (g_game->m_f1f698 != 0 || !isBaseElemental(type)) {
                switch (g_creatureTypeTraits[type].m_townType) {
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

VA(0x0044b2d0, 0xEB)  // dc 0x4f20c
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
        && ownerTown->hasBuilding(EXTRA_0_ID, 1))
        luck += 2;
    if (applyLimits)
        return limit(-3, luck, 3);
    return luck;
}

VA(0x0044b3c0, 0xED)  // dc 0x4f2e8
int armyGroup::getArmyLuck(int index, const hero* ownerHero, const town* ownerTown, int mode, unsigned char applyLimits) const
{
    if (mode == MAGIC_TERRAIN_CURSED_GROUND)
        return 0;
    int luck = getLuck(ownerHero, ownerTown, 0, 0, 0, 0);
    if (mode == MAGIC_TERRAIN_CLOVER_FIELD) {
        int creature = m_armies[index];
        if (g_game->m_f1f698 != 0 || !isBaseElemental(creature)) {
            do {
                switch (g_creatureTypeTraits[creature].m_townType) {
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
    }
    if (m_armies[index] == CREATURE_HALFLING && luck < 1)
        luck = 1;
    if (applyLimits)
        return limit(-3, luck, 3);
    return luck;
}

VA(0x0044b4b0, 0x162)  // dc 0x4f328
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

VA(0x0044b620, 0x1FE)  // dc 0x4f3cc
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

VA(0x0044b820, 0x140)  // dc 0x4f5ec
void armyGroup::mergeArmies(armyGroup* source)
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
            if (source->m_armies[j] == CREATURE_NONE)
                continue;
            long gain = g_creatureTypeTraits[source->m_armies[j]].m_aiValue
                        * source->m_numTroops[j];
            if (!canJoin(source->m_armies[j]))
                gain -= weakestValue;
            if (gain > bestGain) {
                bestGain = gain;
                bestIndex = j;
            }
        }
        if (bestIndex < 0)
            return;
        if (canJoin(source->m_armies[bestIndex])) {
            add(source->m_armies[bestIndex], source->m_numTroops[bestIndex], -1);
            source->dismiss(bestIndex);
        } else {
            std::swap(source->m_armies[bestIndex], m_armies[weakestIndex]);
            std::swap(source->m_numTroops[bestIndex], m_numTroops[weakestIndex]);
        }
    }
}

// E:\gamedcs\armygrp.cpp:1347
// Retail Complete adds a full-width magic-terrain mode and the alignment-
// grouping byte to the older Dreamcast signature. `ret 24h` proves the hidden
// result plus eight explicit arguments. The required DC pass records lines
// 1347..1451, `result`, `alignments[9]`, and `angel_type`; retail's extra
// terrain modes require the ten-byte alignment array used below.

// Retail 0x44b960 proves the accumulator shape that closes the frame and
// register allocation. GetMorale's result is `currentMorale` in the dead
// groupAlignments home [ebp+0x28]. The four terrain arms instead mutate the
// incoming `morale` home [ebp+0x10] with the inverse sign, then the tail does
// `morale -= currentMorale` in place. This gives retail's 0x50-byte frame and
// keeps the creature-row offset in the dead magicTerrain home [ebp+0x24].
// Updating currentMorale was behaviorally close for ordinary values but left a
// 0x54-byte frame and scored 93.97%; the retail accumulator reaches 96.07%.

// Both terrain selectors load `g_creatureTypeTraits[creature].m_townType`
// inside their own arm. Passing a precomputed town type loads it too early.
// Values 0..2 and 3..5 select the good/evil actions; 6..8 and out-of-range
// values join the exit. The goto form reproduces retail B6..B26, including the
// four-way shared strlen/append tail. Flattening both selectors or splitting
// one back into the caller changes the inline phase; the best split reached
// 96.53% numerically but duplicated a terrain strlen, introduced B27, and
// shifted every later CFG block. The combined ordinary helper is retained.

// Other retail/DC facts retained here: both short string arms use operator+=;
// the penalty is `numAlignments >= 5 ? -3 : 2 - numAlignments`; the angel arm
// performs `IsMember(ANGEL) || IsMember(ARCHANGEL)` and then tests ARCHANGEL
// again. DC attributes GetArmyName at that last name lookup, but in the newer
// retail body its candidate site changes the final append frontier and drops
// the combined-helper match to 89.41%; the direct constant-bounded traits
// lookup restores retail's exact CFG through B103.

// Residual at 96.07%: 102/126 candidate blocks are exact against 131 retail
// blocks, and every branch through the Spirit-of-Oppression condition agrees.
// Candidate keeps the Spirit temporary's nested basic_string::_Tidy call and
// the final operator+= path's nested _Eos call; retail expands both, adding the
// five remaining blocks and one delete call. With the nested DC scopes and
// operator spellings below, the traced caller is cb=1040, budget=2080: the
// Spirit destructor receives 145 for _Tidy's cb=152, while the tail append
// leaves 10 for _Eos's cb=46. Direct/split terrain and explicit-temporary
// controls overshoot other library boundaries. Exhaustive crosses of the
// grouping, Minotaur/Spirit scopes, string operators, terrain gate, and the
// canonical plural-name predicate produced many byte-distinct objects but no
// score above 96.07%. This is a compiler-state wall rather than permission to
// flatten helpers or retain a budget probe.
static void applyMoraleMagicTerrain(int magicTerrain, TCreatureType creature,
                                    int& morale, std::string& result)
{
    if (magicTerrain == MAGIC_TERRAIN_HOLY_GROUND
        && (g_game->m_f1f698 != 0 || !isBaseElemental(creature))) {
        switch (g_creatureTypeTraits[creature].m_townType) {
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
            goto done;
        }
        goto done;

    holyGroundGood:
        --morale;
        result += g_holyGroundGoodMoraleText;
        goto done;

    holyGroundEvil:
        ++morale;
        result += g_holyGroundEvilMoraleText;
        goto done;
    }
    if (magicTerrain == MAGIC_TERRAIN_EVIL_FOG
        && (g_game->m_f1f698 != 0 || !isBaseElemental(creature))) {
        switch (g_creatureTypeTraits[creature].m_townType) {
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
            goto done;
        }
        goto done;

    evilFogGood:
        ++morale;
        result += g_evilFogGoodMoraleText;
        goto done;

    evilFogEvil:
        --morale;
        result += g_evilFogEvilMoraleText;
        goto done;
    }

done:
    ;
}

// The Spirit-of-Oppression arm assigns the formatted temporary to `result`.
// Retail calls basic_string::assign at that site; append is the wrong source
// operation even though both are equivalent while `result` is empty.
VA(0x0044b960, 0x859)  // retail-body signature, dc 0x4f708
std::string armyGroup::getMoraleDescription(
    TCreatureType creature, int morale, const hero* ownerHero,
    const town* ownerTown, const hero* otherHero,
    const armyGroup* otherGroup, int magicTerrain,
    unsigned char groupAlignments) const
{
    if (magicTerrain == MAGIC_TERRAIN_CURSED_GROUND)
        return g_cursedGroundMoraleText;

    // NOT a named `const TCreatureTypeTraits&`: retail's CSE keeps the
    // 116-byte OFFSET (it stores the `shl eax,2` result, not an address)
    // and re-adds the table base at each use through base+index
    // addressing (`test [eax+edx+0x10], mask`). Naming the row as a
    // reference makes VC6 materialise the ADDRESS instead - one extra
    // `add` per use, a stack slot of its own, and the table base loaded
    // BEFORE the index chain rather than after it.
    if (g_creatureTypeTraits[creature].m_attributes & g_ctaNoMorale)
        return g_noMoraleCreatureText;

    int currentMorale = getMorale(
        ownerHero, ownerTown, otherHero, otherGroup, 0,
        groupAlignments, 0);
    std::string result;

    if (ownerHero)
        result = ownerHero->getMoraleDescription();

    applyMoraleMagicTerrain(magicTerrain, creature, morale, result);

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
        result += formatString(g_alignmentMoraleFormat,
                                numAlignments, penalty);
    } else if (numAlignments == 1) {
        result += g_sameAlignmentMoraleText;
    }

    if (hasSomeUndead())
        result += g_undeadMoraleText;

    TCreatureType angelType;
    if (isMember(CREATURE_ANGEL) || isMember(CREATURE_ARCHANGEL)) {
        angelType = CREATURE_ANGEL;
        if (isMember(CREATURE_ARCHANGEL))
            angelType = CREATURE_ARCHANGEL;
        result += formatString(g_angelMoraleFormat,
                                g_creatureTypeTraits[angelType].m_pluralName);
    }

    if (otherGroup) {
        TCreatureType dragonType = CREATURE_NONE;
        if (otherGroup->isMember(CREATURE_BONE_DRAGON))
            dragonType = CREATURE_BONE_DRAGON;
        if (otherGroup->isMember(CREATURE_GHOST_DRAGON))
            dragonType = CREATURE_GHOST_DRAGON;
        if (dragonType != CREATURE_NONE)
            result += formatString(
                g_enemyCreatureStatFormat,
                armygrpCreaturePluralName(dragonType));
    }

    if (ownerTown) {
        if (ownerTown->hasBuilding(TAVERN_ID, 0))
            result += formatString(
                "\n%s +1", getBuildingName(ownerTown->m_type, TAVERN_ID));
        if (ownerTown->m_type == TOWN_CASTLE
            && ownerTown->hasBuilding(EXTRA_1_ID, 1))
            result += formatString(
                "\n%s +2", getBuildingName(TOWN_CASTLE, EXTRA_1_ID));
    }

    if (creature == CREATURE_MINOTAUR
        || creature == CREATURE_MINOTAUR_KING) {
        if (currentMorale < 1) {
            result += formatString(g_alwaysPositiveMoraleFormat,
                                    armygrpCreaturePluralName(creature));
            currentMorale = 1;
        }
    }

    if ((ownerHero && ownerHero->isWieldingArtifact(
                         ARTIFACT_SPIRIT_OF_OPPRESSION))
        || (otherHero && otherHero->isWieldingArtifact(
                            ARTIFACT_SPIRIT_OF_OPPRESSION))) {
        if (currentMorale > 0) {
            result = formatString(
                g_spiritOppressionMoraleFormat,
                g_artifactTraits[ARTIFACT_SPIRIT_OF_OPPRESSION].m_name);
            currentMorale = 0;
        }
    }

    morale -= currentMorale;
    if (morale)
        result += formatString(g_otherStatModifiersFormat, morale);

    return result;
}

// E:\gamedcs\armygrp.cpp:1464. Retail Complete's body proves the added
// creature argument and full-width magic-terrain mode; the older Dreamcast
// prototype omits the former and calls the latter a boolean.
// Semantic transcription complete; residual 82.5689%. The bounded
// variable-creature name lookup raised the body to 74.7874%; a function-wide
// shared result regresses.

// THE CURSED-GROUND ARM RETURNS THE LITERAL, no local at all (74.79 ->
// 82.57, 2026-08-14). The EH cleanup transcript is what named it
// (docs/vc6/eh-cleanup.md): retail's states run [0,1,0,2,0,3,0,4,0,5,0] -
// state 0 is `result`, each temporary opens N and closes back to 0 - while
// ours ran [reg,-1,1,2,1,3,1,4,1,5,1,6,1], one whole extra lifetime ahead of
// everything else and every close landing on 1 instead of 0. That leading
// region was the branch-local `std::string result` this arm used to build
// and then COPY into the return object. Retail builds the return object
// itself: `mov [esi],al / call _Tidy / <strlen> / call assign` with
// esi = [ebp+8], which is `basic_string(const char*)` expanded straight onto
// the NRV - i.e. `return gCursedGroundLuckText;`. The hourglass arm below
// is the same shape from the other side: retail passes [ebp+8] as
// format_string's hidden return slot, so `return format_string(...)` elides
// too. The eh signal line is now absent from `diagnose` on this row.

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

// WHAT THE LINE TABLE CAN AND CANNOT SAY HERE. The DC compiland is an
// older revision with no `creature` parameter, and it has NO line at all
// for two blocks retail has: the clover-field arm (between DC 1482 and
// 1485) and the halfling arm (between DC 1502 and 1505) - retail's four
// `format_string`/append groups against DC's three corroborate the second
// exactly. So the +4 candidate sites this row is measured to be short of
// (docs/vc6/inliner.md §5.12) must live in those two blocks; the table
// bounds them negatively and cannot name them. The clover arm's own
// bytes - including its longhand four-way elemental compare, which is the
// `is_base_elemental` shape landed in viewarmywindow - already match
// retail exactly, so it is a site count and not a spelling.

// A DC-census lead that does NOT transfer (2026-08-14): the xref graph
// records `GetArmyName` (dc 0x1ef94, E:\gamedcs\CreatureType.h:296) from
// both this body (x1) and get_morale_description (x3), and that helper is
// what closed ??0TQuickCreatureWindow. Respelling
// `armygrp_creature_plural_name(x)` as `GetArmyName(x, 0)` - the same
// lookup, with the count test folding away - costs this row 1.9 points and
// is byte-flat on get_morale_description (67.5649) and
// get_spell_work_chance (88.5071). The plain plural lookup with no count
// parameter is retail's x86 spelling in this compiland; the census counts
// (x1/x3 against our x2/x2) already said the port's bodies differ.
// Two further census leads are real but not reachable from this file:
// `town::HasBuilding` x1 (E:\gamedcs\Town.h:324) where we read
// `ourTown->active & bitNumber[EXTRA_0_ID]` as a field - town.h carries
// the declaration - and `std::string::operator+=`
// x3 against our mixed `+=`/`append`.
// THE SITE COUNT IS RE-MEASURED AND THE SEARCH IS NARROWED TO ONE BLOCK
// (2026-08-15). The deficit is still exactly FOUR free candidate sites,
// but the probe has to be a USER-DEFINED inline to register at all -
// `armygrp_clamp(0, luck, 3);` steps 82.5689 flat through +3 and jumps to
// 95.1557 at +4, while `result.size()` / `result.capacity()` (Dinkumware
// members) are inert until they start doing harm. And POSITION decides it:
//   clover arm  x4  -> 95.1557      halfling arm x4  -> 80.5000
//   devil block x4  -> 95.1557      after the tail   x4  -> 80.5000
//   any 1/3, 2/2 or 3/1 split across clover+halfling -> 80.5000
// So the four sites are at or BEFORE the Rampart gate, which EXCLUDES the
// halfling arm - half of what the line table's negative bound allowed -
// and leaves the clover arm as the only post-Dreamcast block they can live
// in. Two real sites are now landed inside that window (`is_base_elemental`
// in the clover gate, `town::HasBuilding` in the Rampart gate); both are
// byte-flat, which is expected on a threshold this sharp. Open: which four
// statements the clover arm carries. Nothing is padded - the probe is an
// instrument, and the baseline row is deliberately left at 82.5689.

// THE RAMPART GATE IS A town::HasBuilding CALL (byte-flat, 2026-08-15):
// dc 0x4fab4 line 1499 is `mov #21,r5 / mov #1,r6 / jsr` on
// `?HasBuilding@town@@QBA_NH_N@Z` where this body tested `active &
// bitNumber[EXTRA_0_ID]`. It buys ONE candidate site, and this row is
// short FOUR, so the score does not move - recorded because it is the
// statement retail wrote and because it narrows the outstanding deficit
// to +3 sites in the two post-Dreamcast blocks above. GetLuck's twin
// gate (dc 0x4f20c line 1101) is byte-flat too and stays exact.

// THE CALLER-SHRINK MOVES IT WITHOUT A PROBE (82.5689 -> 84.5060,
// 2026-08-20).  The +4-site instrument above measures the /Ob2 DIVISOR;
// `budget = clamp(2 * caller_cb, 1000, 35000)` has a numerator as well, and
// lifting the clover arm into `apply_luck_magic_terrain` below pushes it the
// same direction with real code instead of padding.  Same lever, same round,
// +12.20 on get_morale_description.  Two further doses measured on top of
// this one and BOTH lose - the whole devil block -7.6 (-> 76.8563) and the
// devil member pick alone as the thinner slice -6.4 (-> 78.1018) - so this
// body, like its twin, wants exactly one lift and it is the magic-terrain
// arm.  The old +4-site probe has now been re-measured against this baseline
// (2026-08-21): four `limit` candidates in the lifted clover helper regress
// 84.5060 -> 74.7246.  The helper changed the budget phase; a hidden four-call
// VERIFY family is not the remaining lever at the retained source shape.

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
static void applyLuckMagicTerrain(int magicTerrain, TCreatureType creature,
                                     int& currentLuck, std::string& result)
{
    if (magicTerrain == MAGIC_TERRAIN_CLOVER_FIELD
        && (g_game->m_f1f698 != 0 || !isBaseElemental(creature))) {
        // Retail uses a compressed nine-town selector. After restoring the
        // bonus/default arms, an ordinary neutral return preserves all 957
        // caller bytes and 53 relocation names/addends at 92.3623%. The older
        // comment rejecting return no longer applies to this helper body.
        // A neutral break still collapses the selector and scores 85.3503%;
        // all 48 combinations with the other terrain joins were checked.
        switch (g_creatureTypeTraits[creature].m_townType) {
        case TOWN_CASTLE:
        case TOWN_RAMPART:
        case TOWN_TOWER:
        case TOWN_INFERNO:
        case TOWN_NECROPOLIS:
        case TOWN_DUNGEON:
            return;
        case TOWN_STRONGHOLD:
        case TOWN_FORTRESS:
        case TOWN_CONFLUX:
            currentLuck -= 2;
            result.append(g_cloverFieldLuckText);
            break;
        default:
            break;
        }
    }
}

VA(0x0044c1c0, 0x3C5)  // retail-body signature, dc 0x4fab4
std::string armyGroup::getLuckDescription(
    TCreatureType creature, int luck, const hero* ourHero,
    const town* ourTown, const hero* enemyHero,
    const armyGroup* enemyGroup, int magicTerrain) const
{
    if (magicTerrain == MAGIC_TERRAIN_CURSED_GROUND)
        return g_cursedGroundLuckText;

    if ((ourHero && ourHero->isWieldingArtifact(
                        ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR))
        || (enemyHero && enemyHero->isWieldingArtifact(
                           ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR))) {
        return formatString(g_hourglassLuckFormat,
                             g_artifactTraits[
                                 ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR].m_name);
    }

    int currentLuck = getLuck(
        ourHero, ourTown, enemyHero, enemyGroup, 0, 0);
    std::string result;
    if (ourHero)
        result = ourHero->getLuckDescription();

    applyLuckMagicTerrain(magicTerrain, creature, currentLuck, result);

    if (enemyGroup) {
        TCreatureType devilType = CREATURE_NONE;
        if (enemyGroup->isMember(CREATURE_DEVIL))
            devilType = CREATURE_DEVIL;
        if (enemyGroup->isMember(CREATURE_ARCH_DEVIL))
            devilType = CREATURE_ARCH_DEVIL;
        if (devilType != CREATURE_NONE)
            result += formatString(g_enemyCreatureStatFormat,
                                    armygrpCreaturePluralName(devilType));
    }

    if (ourTown) {
        char ourTownType = ourTown->m_type;
        if (ourTownType == TOWN_RAMPART
            && ourTown->hasBuilding(EXTRA_0_ID, 1)) {
            result += formatString(
                "\n%s +2", getBuildingName(TOWN_RAMPART, EXTRA_0_ID));
        }
    }

    if (creature == CREATURE_HALFLING && currentLuck < 1) {
        result += formatString("%s are always lucky",
                                armygrpCreaturePluralName(creature));
        currentLuck = 1;
    }

    int otherModifier = luck - currentLuck;
    if (otherModifier)
        result += formatString(g_otherStatModifiersFormat, otherModifier);

    return result;
}

VA(0x0044c590, 0x76)  // dc 0x4fc98
TTerrainType armyGroup::getNativeTerrain() const
{
    TTerrainType native = TERRAIN_NONE;
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (m_armies[i] == CREATURE_NONE)
            continue;
        int alignment;
        if (g_game->m_f1f698 == 0 && isBaseElemental(m_armies[i]))
            alignment = -1;
        else
            alignment = g_creatureTypeTraits[m_armies[i]].m_townType;
        TTerrainType terrain = g_nativeTerrains[alignment];
        if (native != TERRAIN_NONE) {
            if (terrain != native)
                return TERRAIN_NONE;
        } else
            native = terrain;
    }
    return native;
}

#if 0  // @carcass

// E:\gamedcs\armygrp.cpp:131
DC_ONLY(0x4fd54, 0x34)
void* TSplitWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// ..\stlport\stl_string.h:296
DC_ONLY(0x4fd88, 0x4C)
void std::basic_string<char,std::char_traits<char>,std::allocator<char> >::basic_string<char,std::char_traits<char>,std::allocator<char> >(const std::basic_string<char,std::char_traits<char>,std::allocator<char>* __s)
{
    // @stub
}

// ..\stlport\stl_string.h:272
DC_ONLY(0x4fdd4, 0x8)
std::allocator<char> std::basic_string<char,std::char_traits<char>,std::allocator<char> >::get_allocator(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_algobase.h:79
DC_ONLY(0x4fddc, 0xA)
void std::swap(TCreatureType* __a, TCreatureType* __b)
{
    // @stub
}

// ..\stlport\stl_algobase.h:79
DC_ONLY(0x4fde8, 0xA)
void std::swap(int* __a, int* __b)
{
    // @stub
}

// ..\stlport\stl_string.h:464
DC_ONLY(0x4fdf4, 0x3C)
std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_M_range_initialize(char* __f, char* __l)
{
    // @stub
}

// ..\stlport\stl_string.h:454
DC_ONLY(0x4fe30, 0x5C)
std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_M_range_initialize(char* __f, char* __l, std::forward_iterator_tag __formal)
{
    // @stub
}

#endif  // @carcass

// COMDAT pairing: bitset<9>::reference::operator=, agreement 0.922; the
// neighbouring 0x4c680 row scores 0.600 against the same COMDAT.
VA_COMPGEN(0x0044c610, 0x67, BITSET_REFERENCE_ASSIGN, bitset9)

// COMDAT pairing: the same bitset<9> instantiation's _Tidy, 23 B against
// this compiland's single 23-byte COMDAT.
VA_COMPGEN(0x0044c6e0, 0x17, BITSET_TIDY, bitset9)
