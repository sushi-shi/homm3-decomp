#include "va.h"

#include "combatoptionswindow.h"

#include "border.h"
#include "button.h"
#include "cmbtmgr.h"
#include "iconwdgt.h"
#include "kb.h"
#include "misc.h"
#include "mousemgr.h"
#include "prefs.h"
#include "sample.h"
#include "soundmgr.h"
#include "terrain.h"
#include "textresource.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// Source-private in the Dreamcast compiland. Retail's constructor stores the
// active dialog here and its destructor clears it before widget teardown.
DATA(0x00694f90) static TCombatOptionsWindow* g_combatOptionsWindow;

// The shared Help.txt table is owned and filled by text.cpp; column 1
// (right-click) is read.
VA(0x0046e3b0, 0x1320)
DC_ADDRESS(0x066c48, 0xd64)
MAC_ADDRESS(0x07b3b0, 0x2068)
TCombatOptionsWindow::TCombatOptionsWindow()
    : heroWindow(159, 84, 481, 431, 0x12)
{
    m_prefsChanged = 0;
    g_combatOptionsWindow = this;
    m_widgets.reserve(46);

    bitmapBorder* background = new bitmapBorder(
        0, 0, 481, 431, BACKGROUND_ID, "comopbck.pcx", 0x800);
    background->setPlayerPaletteColors(
        g_combatManager->m_playerIds[g_combatManager->m_currentSide]);
    m_widgets.push_back(background);

    m_widgets.push_back(new button(
        246, 359, 100, 48, DEFAULT_ID, "codefaul.def", 1, 0, 0, 0, 2));

    button* accept = new button(
        357, 359, 100, 48, DIALOG_RETURN_SPLIT_ACCEPT, "soretrn.def",
        1, 0, 0, 28, 2);
    accept->setHotkey(1);
    m_widgets.push_back(accept);

    for (int musicSlot = 0; musicSlot < 10; ++musicSlot)
        m_widgets.push_back(new iconWidget(
            29 + musicSlot * 19, 303, 18, 36,
            musicSlot + MUSIC_VOLUME_0_ID, "syslb.def", 0, 0, 0, 0, 0x10));

    for (int effectsSlot = 0; effectsSlot < 10; ++effectsSlot)
        m_widgets.push_back(new iconWidget(
            29 + effectsSlot * 19, 369, 18, 36,
            effectsSlot + EFFECTS_VOLUME_0_ID, "syslb.def",
            0, 0, 0, 0, 0x10));

    m_widgets.push_back(new button(
        28, 225, 62, 32, COMBAT_SPEED_0_ID, "sysopb9.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(
        92, 225, 62, 32, COMBAT_SPEED_0_ID + 1, "sysob10.def",
        0, 1, 0, 0, 2));
    m_widgets.push_back(new button(
        156, 225, 62, 32, COMBAT_SPEED_2_ID, "sysob11.def", 0, 1, 0, 0, 2));

    m_widgets.push_back(new iconWidget(
        246, 84, 32, 24, AUTO_CREATURES_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        246, 114, 32, 24, AUTO_SPELLS_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        246, 144, 32, 24, AUTO_CATAPULT_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        246, 174, 32, 24, AUTO_BALLISTA_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        246, 204, 32, 24, AUTO_FIRST_AID_TENT_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        246, 283, 32, 24, CREATURE_INFO_VERBOSE_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        246, 313, 32, 24, CREATURE_INFO_COMPACT_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        24, 55, 32, 24, SHOW_GRID_ID, "sysopchk.def", 0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        24, 88, 32, 24, MOVEMENT_SHADOW_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        24, 122, 32, 24, MOUSE_SHADOW_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        24, 154, 32, 24, ANIMATE_SPELLBOOK_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));

    m_widgets.push_back(new textWidget(
        26, 19, 432, 28, g_generalText->GetText(GENERAL_TEXT_COMBAT_OPTIONS), "bigfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        26, 204, 193, 20, g_generalText->GetText(GENERAL_TEXT_ANIMATION_SPEED), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        26, 283, 193, 20, g_generalText->GetText(GENERAL_TEXT_MUSIC_VOLUME), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        26, 349, 193, 20, g_generalText->GetText(GENERAL_TEXT_EFFECTS_VOLUME), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        248, 56, 211, 20, g_generalText->GetText(GENERAL_TEXT_AUTO_COMBAT_OPTIONS), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        248, 255, 211, 20, g_generalText->GetText(GENERAL_TEXT_CREATURE_INFO), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));

    m_widgets.push_back(new textWidget(
        283, 84, 182, 24, g_generalText->GetText(GENERAL_TEXT_CREATURES), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        283, 114, 182, 24, g_generalText->GetText(GENERAL_TEXT_SPELLS), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        283, 144, 182, 24, g_generalText->GetText(GENERAL_TEXT_CATAPULT), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        283, 174, 182, 24, g_generalText->GetText(GENERAL_TEXT_COMBAT_BALLISTA_OPTION), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        283, 204, 182, 24, g_generalText->GetText(GENERAL_TEXT_FIRST_AID_TENT), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        283, 283, 182, 24, g_generalText->GetText(GENERAL_TEXT_ALL_STATISTICS), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        283, 313, 182, 24, g_generalText->GetText(GENERAL_TEXT_SPELLS_ONLY), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));

    m_widgets.push_back(new textWidget(
        61, 55, 168, 24, g_generalText->GetText(GENERAL_TEXT_VIEW_HEX_GRID), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        61, 88, 168, 24, g_generalText->GetText(GENERAL_TEXT_MOVEMENT_SHADOW), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        61, 122, 168, 24, g_generalText->GetText(GENERAL_TEXT_CURSOR_SHADOW), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        61, 154, 168, 24, g_generalText->GetText(GENERAL_TEXT_SPELL_BOOK_ANIMATION), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    // As in systemoptionswindow.cpp, DC calls send_message directly for the
    // slider highlight (set_visible is always an out-of-line call there).
    for (int music = MUSIC_VOLUME_0_ID; music <= MUSIC_VOLUME_9_ID; ++music)
        getWidget(music)->sendMessage(
            widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
    getWidget(g_config.m_musicVolume + MUSIC_VOLUME_0_ID)->sendMessage(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
    getWidget(g_config.m_musicVolume + MUSIC_VOLUME_0_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME, g_config.m_musicVolume);

    for (int effects = EFFECTS_VOLUME_0_ID; effects <= EFFECTS_VOLUME_9_ID;
         ++effects)
        getWidget(effects)->sendMessage(
            widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
    getWidget(g_config.m_soundVolume + EFFECTS_VOLUME_0_ID)->sendMessage(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
    getWidget(g_config.m_soundVolume + EFFECTS_VOLUME_0_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME, g_config.m_soundVolume);

    highlightCombatSpeed();
    highlightGrid();
    highlightMovementShadow();
    highlightMouseShadow();
    getWidget(AUTO_CREATURES_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_config.m_combatAutoCreatures);
    getWidget(AUTO_SPELLS_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_config.m_combatAutoSpells);
    getWidget(AUTO_CATAPULT_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_config.m_combatCatapult);
    getWidget(AUTO_BALLISTA_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_config.m_combatBallista);
    getWidget(AUTO_FIRST_AID_TENT_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME, g_config.m_combatFirstAidTent);
    getWidget(CREATURE_INFO_VERBOSE_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME,
        g_config.m_combatArmyInfoLevel == CREATURE_INFO_LEVEL_VERBOSE);
    getWidget(CREATURE_INFO_COMPACT_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME,
        g_config.m_combatArmyInfoLevel == CREATURE_INFO_LEVEL_COMPACT);
    getWidget(ANIMATE_SPELLBOOK_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME, g_config.m_animateSpellBook);

    g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
}

VA_COMPGEN(0x0046f6d0, 0x21, SCALAR_DELETING_DTOR, TCombatOptionsWindow)

VA(0x0046f700, 0x75)
DC_ADDRESS(0x0679ac, 0x68)
MAC_ADDRESS(0x07d418, 0xb4)
TCombatOptionsWindow::~TCombatOptionsWindow()
{
    g_combatOptionsWindow = 0;
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// E:\gamedcs\combatoptionswindow.cpp:194
// Inlined into CombatOptionsWindowHandler.
DC_ADDRESS(0x067a14, 0x2c)
MAC_ADDRESS(0x07d4cc, 0x40)
int TCombatOptionsWindow::convertID2HelpID(int id) const
{
    if (id == DIALOG_RETURN_SPLIT_ACCEPT)
        return 0;
    if (id >= DEFAULT_ID && id <= ANIMATE_SPELLBOOK_ID)
        return id - BACKGROUND_ID;
    return -1;
}

static void updateCombatOptions(int firstUpdate);

VA(0x0046f780, 0x28)
DC_ADDRESS(0x067a40, 0x38)
MAC_ADDRESS(0x07d50c, 0x60)
void TCombatOptionsWindow::doModal()
{
    m_prefsChanged = 0;
    updateCombatOptions(1);
    g_windowManager->doDialog(this, combatOptionsWindowHandler, 0);
    if (m_prefsChanged)
        writePrefs();
}

// The four highlight helpers are present out of line on Dreamcast and
// inlined at every Complete use; they are real source, not open-coded
// loops.
// E:\gamedcs\combatoptionswindow.cpp:230
DC_ADDRESS(0x067a78, 0x52)
MAC_ADDRESS(0x07d56c, 0x78)
void TCombatOptionsWindow::highlightCombatSpeed()
{
    for (int speed = COMBAT_SPEED_0_ID; speed <= COMBAT_SPEED_2_ID; ++speed)
        getWidget(speed)->sendMessage(widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_DIMMED_NODRAW);
    getWidget(g_config.m_combatSpeed + COMBAT_SPEED_0_ID)->sendMessage(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DIMMED_NODRAW);
}

// E:\gamedcs\combatoptionswindow.cpp:243
DC_ADDRESS(0x067acc, 0x22)
MAC_ADDRESS(0x07d5e4, 0x34)
void TCombatOptionsWindow::highlightGrid()
{
    getWidget(SHOW_GRID_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_config.m_showCombatGrid);
}

// E:\gamedcs\combatoptionswindow.cpp:254
DC_ADDRESS(0x067af0, 0x22)
MAC_ADDRESS(0x07d618, 0x34)
void TCombatOptionsWindow::highlightMovementShadow()
{
    getWidget(MOVEMENT_SHADOW_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_config.m_combatShadeLevel);
}

// E:\gamedcs\combatoptionswindow.cpp:265
DC_ADDRESS(0x067b14, 0x68)
MAC_ADDRESS(0x07d64c, 0x34)
void TCombatOptionsWindow::highlightMouseShadow()
{
    getWidget(MOUSE_SHADOW_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_config.m_showCombatMouseHex);
}

// Original: UpdateCombatOptions(int bFirstUpdate), DC 0x68098,
// combatoptionswindow.cpp:651..654. The helper only redraws when firstUpdate
// is zero; preference-member writes belong to the individual handler cases.
// Its original source position is after the handler.
// E:\gamedcs\combatoptionswindow.cpp:278

// DC initializes an exit flag, records preference changes within the
// selected case, and tests both before the redraw/message tails (lines
// 627-639); the per-arm audio-error exits are retained.
VA(0x0046f7b0, 0x72A)
DC_ADDRESS(0x067b7c, 0x51c)
MAC_ADDRESS(0x07d680, 0x6f4)  // DoModal address-take + complete message CFG
int combatOptionsWindowHandler(message& msg)
{
    unsigned char exitFlag = 0;
    pollSound();

    if (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT) {
        if (msg.m_codeX == widget::WIDGET_SELECT
                || msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
            int id = msg.m_codeY;
            if (id >= 0) {
                int helpID = g_combatOptionsWindow->convertID2HelpID(id);
                if (helpID >= 0)
                    normalDialog(g_combatOptionsHelp[helpID].m_rclick,
                        4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            }
        }
    } else if (msg.m_id != MESSAGE_KEY_DOWN && msg.m_id == MESSAGE_WIDGET) {
        switch (msg.m_codeX) {
        case widget::WIDGET_SELECT: {
            int id = g_combatOptionsWindow->findWidget(msg.m_mouseX, msg.m_mouseY);
            if (id >= TCombatOptionsWindow::AUTO_CREATURES_ID
                    && id <= TCombatOptionsWindow::ANIMATE_SPELLBOOK_ID
                    && button::s_clickSample) {
                button::s_clickSample->memSample.memVolume = 0x40;
                button::s_clickSample->memSample.memLooping = 1;
                button::s_clickSample->memSample.memCindex = 3;
                g_soundManager->memorySample(button::s_clickSample);
            }
            return MESSAGE_DISPATCH_CONSUME;
        }
        case widget::WIDGET_DESELECT: {
            int id = msg.m_codeY;
            if (id == DIALOG_RETURN_SPLIT_ACCEPT) {
                exitFlag = 1;
            } else {
                unsigned char prefsChanged = 0;
                switch (id) {
                case TCombatOptionsWindow::DEFAULT_ID: {
                    setDefaultCombatOptions();
                    g_combatOptionsWindow->highlightCombatSpeed();
                    g_combatOptionsWindow->highlightGrid();
                    g_combatOptionsWindow->highlightMovementShadow();
                    g_combatOptionsWindow->highlightMouseShadow();
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_CREATURES_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatAutoCreatures);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_SPELLS_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatAutoSpells);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_CATAPULT_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatCatapult);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_BALLISTA_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatBallista);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_FIRST_AID_TENT_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatFirstAidTent);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::CREATURE_INFO_VERBOSE_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatArmyInfoLevel
                            == TCombatOptionsWindow::CREATURE_INFO_LEVEL_VERBOSE);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::CREATURE_INFO_COMPACT_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatArmyInfoLevel
                            == TCombatOptionsWindow::CREATURE_INFO_LEVEL_COMPACT);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::ANIMATE_SPELLBOOK_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_animateSpellBook);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;
                }

                case TCombatOptionsWindow::MUSIC_VOLUME_0_ID:
                case TCombatOptionsWindow::MUSIC_VOLUME_1_ID:
                case TCombatOptionsWindow::MUSIC_VOLUME_2_ID:
                case TCombatOptionsWindow::MUSIC_VOLUME_3_ID:
                case TCombatOptionsWindow::MUSIC_VOLUME_4_ID:
                case TCombatOptionsWindow::MUSIC_VOLUME_5_ID:
                case TCombatOptionsWindow::MUSIC_VOLUME_6_ID:
                case TCombatOptionsWindow::MUSIC_VOLUME_7_ID:
                case TCombatOptionsWindow::MUSIC_VOLUME_8_ID:
                case TCombatOptionsWindow::MUSIC_VOLUME_9_ID: {
                    if (!g_config.m_musicVolume && !g_soundManager->m_ds) {
                        normalDialog(
                            g_generalText->GetText(
                                GENERAL_TEXT_SYSTEM_OPTIONS_AUDIO_UNAVAILABLE),
                            1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                        break;
                    }
                    g_config.m_musicVolume = id - TCombatOptionsWindow::MUSIC_VOLUME_0_ID;
                    for (int music = TCombatOptionsWindow::MUSIC_VOLUME_0_ID;
                         music <= TCombatOptionsWindow::MUSIC_VOLUME_9_ID; ++music)
                        g_combatOptionsWindow->getWidget(music)->sendMessage(
                            widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
                    g_combatOptionsWindow->getWidget(g_config.m_musicVolume
                            + TCombatOptionsWindow::MUSIC_VOLUME_0_ID)->sendMessage(
                        widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
                    g_combatOptionsWindow->getWidget(g_config.m_musicVolume
                            + TCombatOptionsWindow::MUSIC_VOLUME_0_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME, g_config.m_musicVolume);
                    g_soundManager->adjustMusicVolumes();
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;
                }

                case TCombatOptionsWindow::EFFECTS_VOLUME_0_ID:
                case TCombatOptionsWindow::EFFECTS_VOLUME_1_ID:
                case TCombatOptionsWindow::EFFECTS_VOLUME_2_ID:
                case TCombatOptionsWindow::EFFECTS_VOLUME_3_ID:
                case TCombatOptionsWindow::EFFECTS_VOLUME_4_ID:
                case TCombatOptionsWindow::EFFECTS_VOLUME_5_ID:
                case TCombatOptionsWindow::EFFECTS_VOLUME_6_ID:
                case TCombatOptionsWindow::EFFECTS_VOLUME_7_ID:
                case TCombatOptionsWindow::EFFECTS_VOLUME_8_ID:
                case TCombatOptionsWindow::EFFECTS_VOLUME_9_ID: {
                    if (!g_config.m_soundVolume && !g_soundManager->m_ds) {
                        normalDialog(
                            g_generalText->GetText(
                                GENERAL_TEXT_SYSTEM_OPTIONS_AUDIO_UNAVAILABLE),
                            1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                        break;
                    }
                    g_config.m_soundVolume = id - TCombatOptionsWindow::EFFECTS_VOLUME_0_ID;
                    g_config.m_lastSoundVolume = g_config.m_soundVolume;
                    for (int effects = TCombatOptionsWindow::EFFECTS_VOLUME_0_ID;
                         effects <= TCombatOptionsWindow::EFFECTS_VOLUME_9_ID;
                         ++effects)
                        g_combatOptionsWindow->getWidget(effects)->sendMessage(
                            widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
                    g_combatOptionsWindow->getWidget(g_config.m_soundVolume
                            + TCombatOptionsWindow::EFFECTS_VOLUME_0_ID)->sendMessage(
                        widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
                    g_combatOptionsWindow->getWidget(g_config.m_soundVolume
                            + TCombatOptionsWindow::EFFECTS_VOLUME_0_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME, g_config.m_soundVolume);
                    g_soundManager->adjustSoundVolumes();
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;
                }

                case TCombatOptionsWindow::ANIMATE_SPELLBOOK_ID:
                    g_config.m_animateSpellBook ^= 1;
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::ANIMATE_SPELLBOOK_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_animateSpellBook);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::AUTO_CREATURES_ID:
                    g_config.m_combatAutoCreatures ^= 1;
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_CREATURES_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatAutoCreatures);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::AUTO_SPELLS_ID:
                    g_config.m_combatAutoSpells ^= 1;
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_SPELLS_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatAutoSpells);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::AUTO_CATAPULT_ID:
                    g_config.m_combatCatapult ^= 1;
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_CATAPULT_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatCatapult);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::AUTO_BALLISTA_ID:
                    g_config.m_combatBallista ^= 1;
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_BALLISTA_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatBallista);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::AUTO_FIRST_AID_TENT_ID:
                    g_config.m_combatFirstAidTent ^= 1;
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_FIRST_AID_TENT_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatFirstAidTent);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::COMBAT_SPEED_0_ID:
                case TCombatOptionsWindow::COMBAT_SPEED_1_ID:
                case TCombatOptionsWindow::COMBAT_SPEED_2_ID: {
                    g_config.m_combatSpeed =
                        id - TCombatOptionsWindow::COMBAT_SPEED_0_ID;
                    g_combatOptionsWindow->highlightCombatSpeed();
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;
                }

                case TCombatOptionsWindow::CREATURE_INFO_VERBOSE_ID:
                    if (g_config.m_combatArmyInfoLevel
                            != TCombatOptionsWindow::CREATURE_INFO_LEVEL_VERBOSE) {
                        g_config.m_combatArmyInfoLevel =
                            TCombatOptionsWindow::CREATURE_INFO_LEVEL_VERBOSE;
                        g_combatOptionsWindow->getWidget(
                            TCombatOptionsWindow::CREATURE_INFO_COMPACT_ID)
                            ->sendMessage(widget::WIDGET_SET_ICON_FRAME, 0);
                    } else {
                        g_config.m_combatArmyInfoLevel = 0;
                    }
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::CREATURE_INFO_VERBOSE_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatArmyInfoLevel
                            == TCombatOptionsWindow::CREATURE_INFO_LEVEL_VERBOSE);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::CREATURE_INFO_COMPACT_ID:
                    if (g_config.m_combatArmyInfoLevel
                            != TCombatOptionsWindow::CREATURE_INFO_LEVEL_COMPACT) {
                        g_config.m_combatArmyInfoLevel =
                            TCombatOptionsWindow::CREATURE_INFO_LEVEL_COMPACT;
                        g_combatOptionsWindow->getWidget(
                            TCombatOptionsWindow::CREATURE_INFO_VERBOSE_ID)
                            ->sendMessage(widget::WIDGET_SET_ICON_FRAME, 0);
                    } else {
                        g_config.m_combatArmyInfoLevel = 0;
                    }
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::CREATURE_INFO_COMPACT_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_config.m_combatArmyInfoLevel
                            == TCombatOptionsWindow::CREATURE_INFO_LEVEL_COMPACT);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::SHOW_GRID_ID:
                    g_config.m_showCombatGrid ^= 1;
                    g_combatOptionsWindow->highlightGrid();
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::MOVEMENT_SHADOW_ID:
                    g_config.m_combatShadeLevel ^= 1;
                    g_combatOptionsWindow->highlightMovementShadow();
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::MOUSE_SHADOW_ID:
                    g_config.m_showCombatMouseHex ^= 1;
                    g_combatOptionsWindow->highlightMouseShadow();
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                default:
                    break;
                }
                if (prefsChanged)
                    updateCombatOptions(0);
            }

        }
        break;
        }
    }
    if (exitFlag) {
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = DIALOG_RETURN_SPLIT_ACCEPT;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// DC combatoptionswindow.cpp:651..654, source-private ordinary helper.
DC_ADDRESS(0x068098, 0x38)
MAC_ADDRESS(0x07dd74, 0x4c)
static void updateCombatOptions(int firstUpdate)
{
    if (!firstUpdate) {
        g_combatOptionsWindow->drawWindow(1, 0xffff0001, 0xffff);
    }
}

// E:\gamedcs\combatoptionswindow.cpp:171
