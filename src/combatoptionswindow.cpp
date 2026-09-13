// combatoptionswindow.cpp - E:\gamedcs\combatoptionswindow.cpp (compiland combatoptionswindow.obj)
// HAND-OWNED after admission - retail-byte claims with Dreamcast CodeView prototypes.
#include <va.h>
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
#include "textresource.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// Source-private in the Dreamcast compiland. Retail's constructor stores the
// active dialog here and its destructor clears it before widget teardown.
// Before normalization: gpCombatOptionsWindow.
DATA(0x00694f90) static TCombatOptionsWindow* g_combatOptionsWindow;

DATA(0x006a55ac) THelpText g_combatOptionsHelp[39];

// genrltxt.txt rows this dialog labels itself with. They are consumed
// nowhere else in the image, so no EGeneralTextIndex name is coined for
// them; the retail index is the evidence and the comment is the role:
//   393 window title            394/395/396 left-column group headings
//   397/398 right-column group headings
//   399..401,152,402 the five auto-combat labels (creatures, spells,
//                    catapult, ballista, first aid tent)
//   403/404 creature-info verbose/compact
//   405/406/407 grid, movement shadow, mouse shadow
//   578 spell book animation

// E:\gamedcs\combatoptionswindow.cpp:60
VA(0x0046e3b0, 0x1320)  // combatManager caller + cmpopbck.pcx + vtable/global stores, dc 0x66c48
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

    // The loop variable is the SLOT, not the x: retail keeps the raw 0..9 in
    // its frame slot and adds the id base at the use (`add edx,0xca`), while
    // the x lives in the linear-function-test-replaced derived induction
    // variable (`add edi,0x13`, `cmp edi,0xdb`). Walking x directly and
    // counting the slot by hand makes the slot a SECONDARY induction
    // variable, which VC6 folds the id base into (`mov [ebp-0x10],0xca`).
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
        26, 19, 432, 28, g_generalText->getText(393), "bigfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        26, 204, 193, 20, g_generalText->getText(394), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        26, 283, 193, 20, g_generalText->getText(395), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        26, 349, 193, 20, g_generalText->getText(396), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        248, 56, 211, 20, g_generalText->getText(397), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        248, 255, 211, 20, g_generalText->getText(398), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));

    m_widgets.push_back(new textWidget(
        283, 84, 182, 24, g_generalText->getText(399), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        283, 114, 182, 24, g_generalText->getText(400), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        283, 144, 182, 24, g_generalText->getText(401), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        283, 174, 182, 24, g_generalText->getText(152), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        283, 204, 182, 24, g_generalText->getText(402), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        283, 283, 182, 24, g_generalText->getText(403), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        283, 313, 182, 24, g_generalText->getText(404), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));

    m_widgets.push_back(new textWidget(
        61, 55, 168, 24, g_generalText->getText(405), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        61, 88, 168, 24, g_generalText->getText(406), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        61, 122, 168, 24, g_generalText->getText(407), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        61, 154, 168, 24, g_generalText->getText(578), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    for (int music = MUSIC_VOLUME_0_ID; music <= MUSIC_VOLUME_9_ID; ++music)
        getWidget(music)->sendMessage(widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_DRAWN);
    getWidget(g_unk698760 + MUSIC_VOLUME_0_ID)->sendMessage(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
    getWidget(g_unk698760 + MUSIC_VOLUME_0_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME, g_unk698760);

    for (int effects = EFFECTS_VOLUME_0_ID; effects <= EFFECTS_VOLUME_9_ID;
         ++effects)
        getWidget(effects)->sendMessage(widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_DRAWN);
    getWidget(g_unk698764 + EFFECTS_VOLUME_0_ID)->sendMessage(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
    getWidget(g_unk698764 + EFFECTS_VOLUME_0_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME, g_unk698764);

    highlightCombatSpeed();
    highlightGrid();
    highlightMovementShadow();
    highlightMouseShadow();
    getWidget(AUTO_CREATURES_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_unnamed698758.m_combatAutoCreatures);
    getWidget(AUTO_SPELLS_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_unnamed698758.m_combatAutoSpells);
    getWidget(AUTO_CATAPULT_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_unnamed698758.m_combatCatapult);
    getWidget(AUTO_BALLISTA_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_unnamed698758.m_combatBallista);
    getWidget(AUTO_FIRST_AID_TENT_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME, g_unnamed698758.m_combatFirstAidTent);
    getWidget(CREATURE_INFO_VERBOSE_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME,
        g_unnamed698758.m_combatArmyInfoLevel == CREATURE_INFO_LEVEL_VERBOSE);
    getWidget(CREATURE_INFO_COMPACT_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME,
        g_unnamed698758.m_combatArmyInfoLevel == CREATURE_INFO_LEVEL_COMPACT);
    getWidget(ANIMATE_SPELLBOOK_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME, g_unnamed698758.m_animateSpellBook);

    g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
}

// Retail emits the generated wrapper immediately after the constructor;
// Dreamcast appends it to the compiland.
VA_COMPGEN(0x0046f6d0, 0x21, SCALAR_DELETING_DTOR, TCombatOptionsWindow)

// E:\gamedcs\combatoptionswindow.cpp:180
VA(0x0046f700, 0x75)  // vtable/global/widget teardown, dc 0x679ac
TCombatOptionsWindow::~TCombatOptionsWindow()
{
    g_combatOptionsWindow = 0;
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// E:\gamedcs\combatoptionswindow.cpp:194
// Retail inlines this mapping into CombatOptionsWindowHandler; no separate
// entry exists between the destructor and DoModal.
int TCombatOptionsWindow::convertID2HelpID(int id) const
{
    if (id == DIALOG_RETURN_SPLIT_ACCEPT)
        return 0;
    if (id >= DEFAULT_ID && id <= ANIMATE_SPELLBOOK_ID)
        return id - BACKGROUND_ID;
    return -1;
}

// E:\gamedcs\combatoptionswindow.cpp:214
VA(0x0046f780, 0x28)  // handler address-take + WritePrefs tail, dc 0x67a40
void TCombatOptionsWindow::doModal()
{
    m_prefsChanged = 0;
    g_windowManager->doDialog(this, combatOptionsWindowHandler, 0);
    if (m_prefsChanged)
        writePrefs();
}

// The four highlight helpers are present out of line on Dreamcast (dc
// 0x67a78/0x67acc/0x67af0/0x67b14) but every retail use is expanded by /Ob2;
// no retail entry occurs between DoModal and the handler. They are real
// source, not open-coded loops: HighlightCombatSpeed's expansion holds its
// inlined `this` in EDI across the whole three-widget sweep, which is what
// retail's Default and speed cases do and what a direct
// `gpCombatOptionsWindow->GetWidget(...)` per iteration cannot produce.
// Keep ordinary definitions in their recorded source order; retail makes
// the automatic expansion decision without an authored inline qualifier.
// E:\gamedcs\combatoptionswindow.cpp:230
void TCombatOptionsWindow::highlightCombatSpeed()
{
    for (int speed = COMBAT_SPEED_0_ID; speed <= COMBAT_SPEED_2_ID; ++speed)
        getWidget(speed)->sendMessage(widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_DIMMED_NODRAW);
    getWidget(g_unnamed698758.m_combatSpeed + COMBAT_SPEED_0_ID)->sendMessage(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DIMMED_NODRAW);
}

// E:\gamedcs\combatoptionswindow.cpp:243
void TCombatOptionsWindow::highlightGrid()
{
    getWidget(SHOW_GRID_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_unnamed698758.m_showCombatGrid);
}

// E:\gamedcs\combatoptionswindow.cpp:254
void TCombatOptionsWindow::highlightMovementShadow()
{
    getWidget(MOVEMENT_SHADOW_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_unnamed698758.m_combatShadeLevel);
}

// E:\gamedcs\combatoptionswindow.cpp:265
void TCombatOptionsWindow::highlightMouseShadow()
{
    getWidget(MOUSE_SHADOW_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_unnamed698758.m_showCombatMouseHex);
}

// Original: UpdateCombatOptions(int bFirstUpdate), DC 0x68098,
// combatoptionswindow.cpp:651..654. The helper only redraws when firstUpdate
// is zero; preference-member writes belong to the individual handler cases.
// Its original source position is after the handler.
static void updateCombatOptions(int firstUpdate);

// E:\gamedcs\combatoptionswindow.cpp:278, DC 0x67b7c.
//
// The select/deselect command switch follows DC line 314's source group and
// retail's sub eax,12 / dec eax dispatch. Twelve dispatch/flag states yield
// four reproduced objects: that inner switch raises 85.9186% to 96.4205%;
// case order and byte/bool flags are neutral. An outer message switch changes
// the layout (85.8977%), so retain the existing retail key/widget guard and
// DC's select-before-deselect source order. All four exact siblings are flat.
//
// Preference writes belong to each option case, not to UpdateCombatOptions.
// DC lines 605/606, 613/614 and 621/622 separately set the local redraw flag
// and the window member at +0x44; the other cases join those tails. Retail
// corroborates the same member at +0x4c, with separate ECX/EDX/EAX reloads
// after the grid/movement/mouse messages. Restoring all fifteen member stores
// closes the remaining mismatch. UpdateCombatOptions takes int and only
// conditionally redraws (DC 651..654); it has no preference-member store.
// Keep its ordinary definition after this handler, where DC records it.
//
// The former shared-store reconstruction caused VC6 to merge those case
// tails, which falsely looked like an unreachable register-allocation limit.
// Splitting only three update calls, flattening highlight helpers, hoisting
// a constant-one local, entry accessor/VERIFY controls, and moving the accept
// id into the dense switch did not fix that false source model. Earlier goto
// and audio-label controls are also superseded: keep the local redraw/exit
// scopes and each audio-error exit, as DC records at 627..639. No pragma,
// inline override, copied helper body or validation padding is required.
VA(0x0046f7b0, 0x72A)  // DoModal address-take + complete message CFG, dc 0x67b7c
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
                    normalDialog(g_combatOptionsHelp[helpID].m_text,
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
                button::s_clickSample->m_memSample.m_memVolume = 0x40;
                button::s_clickSample->m_memSample.m_memLooping = 1;
                button::s_clickSample->m_memSample.m_memCindex = 3;
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
                        g_unnamed698758.m_combatAutoCreatures);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_SPELLS_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatAutoSpells);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_CATAPULT_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatCatapult);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_BALLISTA_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatBallista);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_FIRST_AID_TENT_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatFirstAidTent);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::CREATURE_INFO_VERBOSE_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatArmyInfoLevel
                            == TCombatOptionsWindow::CREATURE_INFO_LEVEL_VERBOSE);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::CREATURE_INFO_COMPACT_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatArmyInfoLevel
                            == TCombatOptionsWindow::CREATURE_INFO_LEVEL_COMPACT);
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::ANIMATE_SPELLBOOK_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_animateSpellBook);
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
                    if (!g_unk698760 && !g_soundManager->m_ds) {
                        normalDialog(
                            g_generalText->getText(
                                GENERAL_TEXT_SYSTEM_OPTIONS_AUDIO_UNAVAILABLE),
                            1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                        break;
                    }
                    g_unk698760 = id - TCombatOptionsWindow::MUSIC_VOLUME_0_ID;
                    for (int music = TCombatOptionsWindow::MUSIC_VOLUME_0_ID;
                         music <= TCombatOptionsWindow::MUSIC_VOLUME_9_ID; ++music)
                        g_combatOptionsWindow->getWidget(music)->sendMessage(
                            widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
                    g_combatOptionsWindow->getWidget(g_unk698760
                            + TCombatOptionsWindow::MUSIC_VOLUME_0_ID)->sendMessage(
                        widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
                    g_combatOptionsWindow->getWidget(g_unk698760
                            + TCombatOptionsWindow::MUSIC_VOLUME_0_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME, g_unk698760);
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
                    if (!g_unk698764 && !g_soundManager->m_ds) {
                        normalDialog(
                            g_generalText->getText(
                                GENERAL_TEXT_SYSTEM_OPTIONS_AUDIO_UNAVAILABLE),
                            1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                        break;
                    }
                    g_unk698764 = id - TCombatOptionsWindow::EFFECTS_VOLUME_0_ID;
                    g_unnamed698758.m_lastSoundVolume = g_unk698764;
                    for (int effects = TCombatOptionsWindow::EFFECTS_VOLUME_0_ID;
                         effects <= TCombatOptionsWindow::EFFECTS_VOLUME_9_ID;
                         ++effects)
                        g_combatOptionsWindow->getWidget(effects)->sendMessage(
                            widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
                    g_combatOptionsWindow->getWidget(g_unk698764
                            + TCombatOptionsWindow::EFFECTS_VOLUME_0_ID)->sendMessage(
                        widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
                    g_combatOptionsWindow->getWidget(g_unk698764
                            + TCombatOptionsWindow::EFFECTS_VOLUME_0_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME, g_unk698764);
                    g_soundManager->adjustSoundVolumes();
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;
                }

                case TCombatOptionsWindow::ANIMATE_SPELLBOOK_ID:
                    g_unnamed698758.m_animateSpellBook ^= 1;
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::ANIMATE_SPELLBOOK_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_animateSpellBook);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::AUTO_CREATURES_ID:
                    g_unnamed698758.m_combatAutoCreatures ^= 1;
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_CREATURES_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatAutoCreatures);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::AUTO_SPELLS_ID:
                    g_unnamed698758.m_combatAutoSpells ^= 1;
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_SPELLS_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatAutoSpells);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::AUTO_CATAPULT_ID:
                    g_unnamed698758.m_combatCatapult ^= 1;
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_CATAPULT_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatCatapult);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::AUTO_BALLISTA_ID:
                    g_unnamed698758.m_combatBallista ^= 1;
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_BALLISTA_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatBallista);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::AUTO_FIRST_AID_TENT_ID:
                    g_unnamed698758.m_combatFirstAidTent ^= 1;
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::AUTO_FIRST_AID_TENT_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatFirstAidTent);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::COMBAT_SPEED_0_ID:
                case TCombatOptionsWindow::COMBAT_SPEED_1_ID:
                case TCombatOptionsWindow::COMBAT_SPEED_2_ID: {
                    g_unnamed698758.m_combatSpeed =
                        id - TCombatOptionsWindow::COMBAT_SPEED_0_ID;
                    g_combatOptionsWindow->highlightCombatSpeed();
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;
                }

                case TCombatOptionsWindow::CREATURE_INFO_VERBOSE_ID:
                    if (g_unnamed698758.m_combatArmyInfoLevel
                            != TCombatOptionsWindow::CREATURE_INFO_LEVEL_VERBOSE) {
                        g_unnamed698758.m_combatArmyInfoLevel =
                            TCombatOptionsWindow::CREATURE_INFO_LEVEL_VERBOSE;
                        g_combatOptionsWindow->getWidget(
                            TCombatOptionsWindow::CREATURE_INFO_COMPACT_ID)
                            ->sendMessage(widget::WIDGET_SET_ICON_FRAME, 0);
                    } else {
                        g_unnamed698758.m_combatArmyInfoLevel = 0;
                    }
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::CREATURE_INFO_VERBOSE_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatArmyInfoLevel
                            == TCombatOptionsWindow::CREATURE_INFO_LEVEL_VERBOSE);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::CREATURE_INFO_COMPACT_ID:
                    if (g_unnamed698758.m_combatArmyInfoLevel
                            != TCombatOptionsWindow::CREATURE_INFO_LEVEL_COMPACT) {
                        g_unnamed698758.m_combatArmyInfoLevel =
                            TCombatOptionsWindow::CREATURE_INFO_LEVEL_COMPACT;
                        g_combatOptionsWindow->getWidget(
                            TCombatOptionsWindow::CREATURE_INFO_VERBOSE_ID)
                            ->sendMessage(widget::WIDGET_SET_ICON_FRAME, 0);
                    } else {
                        g_unnamed698758.m_combatArmyInfoLevel = 0;
                    }
                    g_combatOptionsWindow->getWidget(
                        TCombatOptionsWindow::CREATURE_INFO_COMPACT_ID)->sendMessage(
                        widget::WIDGET_SET_ICON_FRAME,
                        g_unnamed698758.m_combatArmyInfoLevel
                            == TCombatOptionsWindow::CREATURE_INFO_LEVEL_COMPACT);
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::SHOW_GRID_ID:
                    g_unnamed698758.m_showCombatGrid ^= 1;
                    g_combatOptionsWindow->highlightGrid();
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::MOVEMENT_SHADOW_ID:
                    g_unnamed698758.m_combatShadeLevel ^= 1;
                    g_combatOptionsWindow->highlightMovementShadow();
                    prefsChanged = 1;
                    g_combatOptionsWindow->m_prefsChanged = 1;
                    break;

                case TCombatOptionsWindow::MOUSE_SHADOW_ID:
                    g_unnamed698758.m_showCombatMouseHex ^= 1;
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
static void updateCombatOptions(int firstUpdate)
{
    if (!firstUpdate) {
        g_combatOptionsWindow->drawWindow(1, 0xffff0001, 0xffff);
    }
}


// E:\gamedcs\combatoptionswindow.cpp:171
#if 0  // @carcass -- represented by VA_COMPGEN above
DC_ONLY(0x680d0, 0x34)
void* TCombatOptionsWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}
#endif
