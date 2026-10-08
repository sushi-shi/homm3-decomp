#include "va.h"

#include "systemoptionswindow.h"

#include "border.h"
#include "button.h"
#include "cmbtmgr.h"
#include "exec.h"
#include "game.h"
#include "iconwdgt.h"
#include "kb.h"
#include "mainmenu.h"
#include "message.h"
#include "misc.h"
#include "prefs.h"
#include "remote.h"
#include "sample.h"
#include "soundmgr.h"
#include "terrain.h"
#include "textresource.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// The shared Help.txt table is owned and filled by text.cpp.
// Retail reads column 1 (right-click), four bytes after each row base.

// genrltxt.txt rows this dialog labels itself with. They have no other
// consumer in the image, so no EGeneralTextIndex name is coined for them;
// the retail index is the evidence and the comment is the role:
//   569 window title
//   570/571/572/21 hero-speed, computer-speed, scroll-speed and video
//                  quality group headings
//   395/396 music- and effects-volume headings (shared with the combat
//           options dialog, which labels its own sliders with them)
//   573..578 the six checkbox labels (show path, move reminder, quick
//            combat, video subtitles, town outlines, spell book
//            animation; 578 is shared with the combat options dialog)

// Text subscripts keep the public operator[] -> getText -> vector[] chain.
// Native Mac 0x1ab964 onward retains the vector indexer; DC names the
// outer operator on the corresponding label and confirmation statements.
// E:\gamedcs\systemoptionswindow.cpp:43
VA(0x005b1790, 0x187C)
DC_ADDRESS(0x15f588, 0x10ac)
MAC_ADDRESS(0x1aa2ec, 0x2534)  // sole sysopbck.pcx reference + vtable block
TSystemOptionsWindow::TSystemOptionsWindow()
    : CAdvPopup(159, 56, 481, 487, 0x12), m_prefsChanged(0)
{
    m_quickCombatSave = g_config.m_quickCombat;
    m_widgets.reserve(NWIDGETS);

    bitmapBorder* background = new bitmapBorder(
        0, 0, 481, 487, BACKGROUND_ID, "SysOpBck.pcx", 0x800);
    background->setPlayerPaletteColors(g_game->getLocalPlayerGamePos());
    m_widgets.push_back(background);

    m_widgets.push_back(new button(246, 298, 100, 48,
        TMainMenu::LOAD_GAME_ID, "soload.def", 1, 0, 0, 38, 2));
    m_widgets.push_back(new button(357, 298, 100, 48,
        TMainMenu::SAVE_GAME_ID, "sosave.def", 1, 0, 0, 31, 2));
    m_widgets.push_back(new button(246, 357, 100, 48,
        TMainMenu::RESTART_ID, "sorstrt.def", 1, 0, 0, 19, 2));
    m_widgets.push_back(new button(357, 357, 100, 48,
        TMainMenu::MAIN_MENU_ID, "somain.def", 1, 0, 0, 50, 2));
    m_widgets.push_back(new button(246, 415, 100, 48,
        TMainMenu::QUIT_ID, "soquit.def", 1, 0, 0, 16, 2));
    m_widgets.push_back(new button(357, 415, 100, 48,
        DIALOG_RETURN_SPLIT_ACCEPT, "soretrn.def", 1, 0, 0, 1, 2));

    // Retail indexes both slider rows by slot and derives the x coordinate.
    // Keep each real coordinate as a local before allocation: with the
    // canonical widget helpers this recovers 76.1093 -> 76.5135; folding
    // either coordinate into its constructor argument loses that gain.
    // The remaining differences are nested vector expansions. Historical
    // inline-depth and dead-code diagnostics did not recover those sites;
    // no artificial caller work belongs here.
    for (int musicSlot = 0; musicSlot < 10; musicSlot++) {
        int musicX = 29 + musicSlot * 19;
        m_widgets.push_back(new iconWidget(
            musicX, 359, 18, 36, musicSlot + MUSIC_VOLUME_0_ID, "syslb.def",
            0, 0, 0, 0, 0x10));
    }

    for (int effectsSlot = 0; effectsSlot < 10; effectsSlot++) {
        int effectsX = 29 + effectsSlot * 19;
        m_widgets.push_back(new iconWidget(
            effectsX, 425, 18, 36, effectsSlot + EFFECTS_VOLUME_0_ID,
            "syslb.def", 0, 0, 0, 0, 0x10));
    }

    m_widgets.push_back(new button(28, 77, 46, 32,
        HERO_SPEED_WALK_ID, "sysopb1.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(76, 77, 46, 32,
        HERO_SPEED_CANTER_ID, "sysopb2.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(124, 77, 46, 32,
        HERO_SPEED_GALLOP_ID, "sysopb3.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(172, 77, 46, 32,
        HERO_SPEED_JUMP_ID, "sysopb4.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(28, 144, 46, 32,
        AI_SPEED_CANTER_ID, "sysopb5.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(76, 144, 46, 32,
        AI_SPEED_GALLOP_ID, "sysopb6.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(124, 144, 46, 32,
        AI_SPEED_JUMP_ID, "sysopb7.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(172, 144, 46, 32,
        AI_SPEED_NONE_ID, "sysopb8.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(28, 210, 62, 32,
        WINDOW_SCROLL_SLOW, "sysopb9.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(92, 210, 62, 32,
        WINDOW_SCROLL_MEDIUM, "sysob10.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(156, 210, 62, 32,
        WINDOW_SCROLL_FAST, "sysob11.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(28, 276, 94, 32,
        VIDEO_QUALITY_HIGH, "sysob12.def", 0, 1, 0, 0, 2));
    m_widgets.push_back(new button(124, 276, 94, 32,
        VIDEO_QUALITY_LOW, "sysob13.def", 0, 1, 0, 0, 2));

    m_widgets.push_back(new iconWidget(246, 55, 32, 24,
        SHOW_PATH_ID, "sysopchk.def", 0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(246, 87, 32, 24,
        MOVE_REMINDER_ID, "sysopchk.def", 0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(246, 119, 32, 24,
        QUICK_COMBAT_ID, "sysopchk.def", 0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(246, 151, 32, 24,
        VIDEO_SUBTITLES_ID, "sysopchk.def", 0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(246, 183, 32, 24,
        TOWN_OUTLINES_ID, "sysopchk.def", 0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(246, 215, 32, 24,
        ANIMATE_SPELLBOOK_ID, "sysopchk.def", 0, 0, 0, 0, 0x10));

    m_widgets.push_back(new textWidget(
        26, 19, 432, 28, (*g_generalText)[GENERAL_TEXT_SYSTEM_OPTIONS], "bigfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        26, 56, 193, 20, (*g_generalText)[GENERAL_TEXT_HERO_SPEED], "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        26, 122, 193, 20, (*g_generalText)[GENERAL_TEXT_ENEMY_SPEED], "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        26, 188, 193, 20, (*g_generalText)[GENERAL_TEXT_MAP_SCROLL_SPEED], "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        26, 254, 193, 20, (*g_generalText)[GENERAL_TEXT_VIDEO_QUALITY], "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        26, 339, 193, 20, (*g_generalText)[GENERAL_TEXT_MUSIC_VOLUME], "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    m_widgets.push_back(new textWidget(
        26, 406, 193, 20, (*g_generalText)[GENERAL_TEXT_EFFECTS_VOLUME], "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));

    m_widgets.push_back(new textWidget(
        282, 55, 182, 24, (*g_generalText)[GENERAL_TEXT_SHOW_MOVE_PATH], "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        282, 87, 182, 24, (*g_generalText)[GENERAL_TEXT_SHOW_HERO_REMINDER], "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        282, 119, 182, 24, (*g_generalText)[GENERAL_TEXT_QUICK_COMBAT], "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        282, 151, 182, 24, (*g_generalText)[GENERAL_TEXT_VIDEO_SUBTITLES], "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        282, 183, 182, 24, (*g_generalText)[GENERAL_TEXT_TOWN_BUILDING_OUTLINES], "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    m_widgets.push_back(new textWidget(
        282, 215, 182, 24, (*g_generalText)[GENERAL_TEXT_SPELL_BOOK_ANIMATION], "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));

    // DC121 initializes the pointer iterator from begin, checks end at
    // each iteration, then DC123..126 register the widget or call MemError.
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    // DC 130..136 send the status bits directly: the Dreamcast compiler
    // never expands widget::set_visible (all 51 DC uses call dc 0x56df8) and
    // none comes from this compiland. Retail pushes both constants ahead of
    // each getWidget call, as a direct send does; the setVisible spelling
    // pushed after it (76.55 -> 98.31).
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

    for (int walk = HERO_SPEED_WALK_ID; walk <= HERO_SPEED_JUMP_ID; ++walk)
        getWidget(walk)->sendMessage(widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_DIMMED_NODRAW);
    getWidget(g_config.m_walkSpeed[1] + MUSIC_TYPE_MIDI_ID)->sendMessage(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DIMMED_NODRAW);

    for (int ai = AI_SPEED_CANTER_ID; ai <= AI_SPEED_NONE_ID; ++ai)
        getWidget(ai)->sendMessage(widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_DIMMED_NODRAW);
    getWidget(g_config.m_walkSpeed[0]
            + HERO_SPEED_GALLOP_ID)->sendMessage(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DIMMED_NODRAW);

    getWidget(SHOW_PATH_ID)->sendMessage(widget::WIDGET_SET_ICON_FRAME,
        g_config.m_showRoute);

    if (!g_game->m_isTutorial) {
        getWidget(MOVE_REMINDER_ID)->sendMessage(
            widget::WIDGET_SET_ICON_FRAME, g_config.m_moveReminder);
        getWidget(QUICK_COMBAT_ID)->sendMessage(
            widget::WIDGET_SET_ICON_FRAME, g_config.m_quickCombat);
    } else {
        getWidget(MOVE_REMINDER_ID)->sendMessage(
            widget::WIDGET_SET_ICON_FRAME, 0);
        getWidget(MOVE_REMINDER_ID)->sendMessage(
            widget::WIDGET_SET_STATUS, widget::WIDGET_DIMMED_NODRAW);
        getWidget(MOVE_REMINDER_ID)->sendMessage(
            widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE);
        getWidget(QUICK_COMBAT_ID)->sendMessage(
            widget::WIDGET_SET_ICON_FRAME, 0);
        getWidget(QUICK_COMBAT_ID)->sendMessage(
            widget::WIDGET_SET_STATUS, widget::WIDGET_DIMMED_NODRAW);
        getWidget(QUICK_COMBAT_ID)->sendMessage(
            widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE);
    }

    getWidget(VIDEO_SUBTITLES_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME, g_config.m_videoSubtitles);
    getWidget(TOWN_OUTLINES_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME, g_config.m_townOutlines);
    getWidget(ANIMATE_SPELLBOOK_ID)->sendMessage(
        widget::WIDGET_SET_ICON_FRAME, g_config.m_animateSpellBook);

    for (int scroll = WINDOW_SCROLL_SLOW; scroll <= WINDOW_SCROLL_FAST;
         ++scroll)
        getWidget(scroll)->sendMessage(widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_DIMMED_NODRAW);
    getWidget(g_config.m_windowScrollSpeed
            + WINDOW_SCROLL_SLOW)->sendMessage(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DIMMED_NODRAW);

    for (int video = VIDEO_QUALITY_LOW; video <= VIDEO_QUALITY_HIGH; ++video)
        getWidget(video)->sendMessage(widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_DIMMED_NODRAW);
    getWidget(g_config.m_binkVideo + VIDEO_QUALITY_LOW)->sendMessage(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DIMMED_NODRAW);

    updateSystemOptions(true);
}

VA_COMPGEN(0x005b3010, 0x21, SCALAR_DELETING_DTOR, TSystemOptionsWindow)

VA(0x005b3040, 0x6B)
DC_ADDRESS(0x160634, 0x62)
MAC_ADDRESS(0x1ac820, 0xac)
TSystemOptionsWindow::~TSystemOptionsWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// E:\gamedcs\systemoptionswindow.cpp:205
// Retail inlines this switch into WindowHandler; no separate entry exists
// between the destructor and DoModal.
DC_ADDRESS(0x160698, 0x68)
MAC_ADDRESS(0x1ac8cc, 0x98)
int TSystemOptionsWindow::convertID2HelpID(int id) const
{
    if (id < 0)
        return -1;
    if (id >= MUSIC_VOLUME_0_ID && id <= ANIMATE_SPELLBOOK_ID)
        return id - 195;

    int helpID;
    switch (id) {
    case TMainMenu::MAIN_MENU_ID: helpID = 0; break;
    case TMainMenu::LOAD_GAME_ID: helpID = 1; break;
    case TMainMenu::SAVE_GAME_ID: helpID = 2; break;
    case TMainMenu::RESTART_ID: helpID = 3; break;
    case TMainMenu::QUIT_ID: helpID = 4; break;
    case DIALOG_RETURN_SPLIT_ACCEPT: helpID = 5; break;
    default: helpID = -1; break;
    }
    return helpID;
}

VA(0x005b30b0, 0x8E)
DC_ADDRESS(0x160700, 0x6e)
MAC_ADDRESS(0x1ac964, 0xcc)
void TSystemOptionsWindow::doModal()
{
    m_prefsChanged = 0;
    heroWindow::doModal(0);

    if (m_prefsChanged) {
        if (g_remoteOn
                && g_config.m_quickCombat != m_quickCombatSave) {
            g_game->getLocalPlayer()->m_quickCombat =
                g_config.m_quickCombat;
            CCombatTypeMsg msg(g_config.m_quickCombat);
            transmitRemoteData(&msg, 0x7f, 0, 1);
        }
        writePrefs();
    }
}

// E:\gamedcs\systemoptionswindow.cpp:260
// DC-SOURCE-SHAPE RESTORED 2026-09-01 (92.5000 -> 95.3661).
// The dc 0x160698 helper owns, in order, the negative-id return, the
// 201..242 range return, and the six-command switch assigning a local for one
// final return. Retail independently agrees at fn+0x46..+0xae: negative
// guard, range first, command switch second, then one joined helpID test. The
// old 92.5000 spelling moved the negative guard into this caller and put the
// switch first, contradicting both sources. Restoring the exact mixed-return
// helper shape reached 93.3701. DC line 288 ends the right-click group with a
// branch to the common return; retaining that join as `goto consume` restores
// retail's EBX-held dispatch value and reaches 95.3661.

// Negative controls: all-immediate and fully-assigned range-first helper
// families are byte-identical at 85.5532; plain, inline and forced-inline
// declarations are byte-identical; moving the dispatch-value declaration or
// reusing the base-handler result is byte-identical in that family and
// score-flat at 95.3661. Moving the consume label physically ahead of the
// main dispatch is likewise score-flat, so no synthetic layout spelling is
// retained.

// 91.9193 -> 92.5000, 2026-08-21: two statement-order facts from the
// unmasked retail stream. UpdateSystemOptions sets bPrefsChanged BEFORE it
// initializes the otherwise-unused message (91.9193 -> 92.0354), and the
// command tail saves codeY, writes msg->id, then latches the saved command in
// dialogReturn (-> 92.5000). The latter now agrees in instruction order; only
// its EAX/ECX versus retail EDX/EAX allocation remains.

// Residual (95.3661%): all 32 out-of-line calls and all 76 CFG blocks agree in
// count. The help helper's range/switch blocks now agree instruction for
// instruction. The first remaining CFG difference is return-tail placement:
// retail splits the post-NormalDialog consume epilogue before main dispatch,
// while this compiler keeps it attached to the call despite the common join.
// In the redraw tail retail also hoists the DrawWindow vtable load and three
// argument pushes ahead of the message-constructor stores; this compiler
// leaves them after. The old volatile surrogate was a 92.5000 local maximum;
// the recovered constructor is the positive source fact. The final findWidget
// and click-sample deltas are register rotations over the same loads/stores
// and calls.
// Goto audit: Replacing consume jumps with direct returns scores
// 92.3189% versus 94.3957%; retain the shared dispatch epilogue.
// DC's nested right-click/ordinary-widget groups remove three consume gotos
// without changing 94.3957%. A separate prefsChanged result and positive
// accepted-command scope remove two more joins at the same score. The
// translated command label still admits commands that need no confirmation.
// Combining this with an outer widget-code if dispatch falls to 85.1339%;
// both byte and bool preference flags preserve the retained switch form.
// The outer widget-code default can return one directly: all 1526 compiled
// bytes and 96 relocation names/addends remain unchanged at 94.3957%.
// Separate direct confirmation/translation bodies and their combinations
// still lose code agreement (92.9902%); keep each action shared as below.
// A single do/while(0) around widget dispatch uses confirmation's continue
// and unconfirmed commands' break to reach common translation. Confirmation
// stays before translation; both joins are removed with all 1526 compiled
// bytes and 96 references/addends unchanged at 94.3957%.
// The slider arms send WIDGET_CLEAR/SET_STATUS directly, as DC's rows name
// send_message here (DC never expands widget::set_visible; all 51 of its
// uses call dc 0x56df8). Retail pushes both constants before getWidget, as
// a direct send does: 96.70 -> 100 (2026-09-29).
VA(0x005b3140, 0x61E)
DC_ADDRESS(0x160770, 0x578)
MAC_ADDRESS(0x1aca30, 0x6a4)  // vtable slot 9 + inlined help switch
int TSystemOptionsWindow::windowHandler(message& msg)
{
    // DC records one procedure-local save; Mac reuses r24 in both slider arms.
    int save;
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;

    pollSound();

    bool exitFlag = 0;
    if (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)
    {
        if (msg.m_codeX == widget::WIDGET_SELECT ||
            msg.m_codeX == widget::WIDGET_RIGHT_SELECT)
        {
            int id = msg.m_codeY;
            int helpID = convertID2HelpID(id);
            if (helpID >= 0)
                normalDialog(g_systemOptionsHelp[helpID].m_rclick, 4, -1, -1, -1, 0, -1,
                             0, -1, 0, -1, 0);
        }
    }
    else if (msg.m_id != MESSAGE_DISPATCH_CONSUME)
    {
        if (msg.m_id == MESSAGE_WIDGET)
        {
            switch (msg.m_codeX)
            {
            case widget::WIDGET_SELECT:
            {
                int id = findWidget(msg.m_mouseX, msg.m_mouseY);
                if (id >= SHOW_PATH_ID && id <= ANIMATE_SPELLBOOK_ID &&
                    button::s_clickSample)
                {
                    button::s_clickSample->m_memSample.m_memVolume = 0x40;
                    button::s_clickSample->m_memSample.m_memLooping =
                        MESSAGE_DISPATCH_CONSUME;
                    button::s_clickSample->m_memSample.m_memCindex = 3;
                    g_soundManager->memorySample(button::s_clickSample);
                }
                break;
            }
            case widget::WIDGET_DESELECT:
            {
                int id = msg.m_codeY;
                if (id == TMainMenu::LOAD_GAME_ID || id == TMainMenu::MAIN_MENU_ID ||
                    id == TMainMenu::QUIT_ID)
                {
                    normalDialog(
                        (*g_generalText)[GENERAL_TEXT_UNSAVED_GAME_COMMAND_CONFIRM],
                        2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                    if (g_windowManager->m_dialogReturn == DIALOG_RETURN_ACCEPT)
                        exitFlag = 1;
                }
                else if (id == TMainMenu::SAVE_GAME_ID || id == TMainMenu::RESTART_ID ||
                         id == DIALOG_RETURN_SPLIT_ACCEPT)
                    exitFlag = 1;

                else
                {
                    bool prefsChanged = 0;
                    switch (id)
                    {
                    case VIDEO_QUALITY_LOW:
                    case VIDEO_QUALITY_HIGH:
                    {
                        g_config.m_binkVideo = id - VIDEO_QUALITY_LOW;
                        for (int i = VIDEO_QUALITY_LOW; i <= VIDEO_QUALITY_HIGH; ++i)
                            getWidget(i)->sendMessage(widget::WIDGET_CLEAR_STATUS,
                                                      widget::WIDGET_DIMMED);
                        getWidget(g_config.m_binkVideo + VIDEO_QUALITY_LOW)
                            ->sendMessage(widget::WIDGET_SET_STATUS,
                                          widget::WIDGET_DIMMED);
                        prefsChanged = 1;
                        m_prefsChanged = 1;
                        break;
                    }

                    case WINDOW_SCROLL_SLOW:
                    case WINDOW_SCROLL_MEDIUM:
                    case WINDOW_SCROLL_FAST:
                    {
                        g_config.m_windowScrollSpeed = id - WINDOW_SCROLL_SLOW;
                        for (int i = WINDOW_SCROLL_SLOW; i <= WINDOW_SCROLL_FAST; ++i)
                            getWidget(i)->sendMessage(widget::WIDGET_CLEAR_STATUS,
                                                      widget::WIDGET_DIMMED);
                        getWidget(g_config.m_windowScrollSpeed +
                                  WINDOW_SCROLL_SLOW)
                            ->sendMessage(widget::WIDGET_SET_STATUS,
                                          widget::WIDGET_DIMMED);
                        prefsChanged = 1;
                        m_prefsChanged = 1;
                        break;
                    }

                    case HERO_SPEED_WALK_ID:
                    case HERO_SPEED_CANTER_ID:
                    case HERO_SPEED_GALLOP_ID:
                    case HERO_SPEED_JUMP_ID:
                    {
                        g_config.m_walkSpeed[1] = id - MUSIC_TYPE_MIDI_ID;
                        for (int i = HERO_SPEED_WALK_ID; i <= HERO_SPEED_JUMP_ID; ++i)
                            getWidget(i)->sendMessage(widget::WIDGET_CLEAR_STATUS,
                                                      widget::WIDGET_DIMMED);
                        getWidget(g_config.m_walkSpeed[1] + MUSIC_TYPE_MIDI_ID)
                            ->sendMessage(widget::WIDGET_SET_STATUS,
                                          widget::WIDGET_DIMMED);
                        prefsChanged = 1;
                        m_prefsChanged = 1;
                        break;
                    }

                    case AI_SPEED_CANTER_ID:
                    case AI_SPEED_GALLOP_ID:
                    case AI_SPEED_JUMP_ID:
                    case AI_SPEED_NONE_ID:
                    {
                        g_config.m_walkSpeed[0] = id - HERO_SPEED_GALLOP_ID;
                        g_config.m_blackoutComputer =
                            g_config.m_walkSpeed[0] ==
                            AI_SPEED_BLACKOUT_VALUE;
                        for (int i = AI_SPEED_CANTER_ID; i <= AI_SPEED_NONE_ID; ++i)
                            getWidget(i)->sendMessage(widget::WIDGET_CLEAR_STATUS,
                                                      widget::WIDGET_DIMMED);
                        getWidget(g_config.m_walkSpeed[0] +
                                  HERO_SPEED_GALLOP_ID)
                            ->sendMessage(widget::WIDGET_SET_STATUS,
                                          widget::WIDGET_DIMMED);
                        prefsChanged = 1;
                        m_prefsChanged = 1;
                        break;
                    }

                    case MUSIC_VOLUME_0_ID:
                    case MUSIC_VOLUME_1_ID:
                    case MUSIC_VOLUME_2_ID:
                    case MUSIC_VOLUME_3_ID:
                    case MUSIC_VOLUME_4_ID:
                    case MUSIC_VOLUME_5_ID:
                    case MUSIC_VOLUME_6_ID:
                    case MUSIC_VOLUME_7_ID:
                    case MUSIC_VOLUME_8_ID:
                    case MUSIC_VOLUME_9_ID:
                    {
                        if (!g_config.m_musicVolume && !g_soundManager->m_ds)
                        {
                            normalDialog(
                                (*g_generalText)[GENERAL_TEXT_SYSTEM_OPTIONS_AUDIO_UNAVAILABLE],
                                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                            return MESSAGE_DISPATCH_CONSUME;
                        }
                        g_config.m_musicVolume = id - MUSIC_VOLUME_0_ID;
                        for (int i = MUSIC_VOLUME_0_ID; i <= MUSIC_VOLUME_9_ID; ++i)
                            getWidget(i)->sendMessage(
                                widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
                        getWidget(g_config.m_musicVolume + MUSIC_VOLUME_0_ID)
                            ->sendMessage(
                                widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
                        getWidget(g_config.m_musicVolume + MUSIC_VOLUME_0_ID)
                            ->sendMessage(widget::WIDGET_SET_ICON_FRAME, g_config.m_musicVolume);
                        save = g_soundManager->m_playSounds;
                        g_soundManager->m_playSounds = MESSAGE_DISPATCH_CONSUME;
                        g_soundManager->adjustMusicVolumes();
                        g_soundManager->m_playSounds = save;
                        prefsChanged = 1;
                        m_prefsChanged = 1;
                        break;
                    }

                    case EFFECTS_VOLUME_0_ID:
                    case EFFECTS_VOLUME_1_ID:
                    case EFFECTS_VOLUME_2_ID:
                    case EFFECTS_VOLUME_3_ID:
                    case EFFECTS_VOLUME_4_ID:
                    case EFFECTS_VOLUME_5_ID:
                    case EFFECTS_VOLUME_6_ID:
                    case EFFECTS_VOLUME_7_ID:
                    case EFFECTS_VOLUME_8_ID:
                    case EFFECTS_VOLUME_9_ID:
                    {
                        if (!g_config.m_soundVolume && !g_soundManager->m_ds)
                        {
                            normalDialog(
                                (*g_generalText)[GENERAL_TEXT_SYSTEM_OPTIONS_AUDIO_UNAVAILABLE],
                                MESSAGE_DISPATCH_CONSUME, -1, -1, -1, 0, -1, 0, -1, 0,
                                -1, 0);
                            return MESSAGE_DISPATCH_CONSUME;
                        }
                        g_config.m_soundVolume = id - EFFECTS_VOLUME_0_ID;
                        g_config.m_lastSoundVolume = g_config.m_soundVolume;
                        for (int i = EFFECTS_VOLUME_0_ID; i <= EFFECTS_VOLUME_9_ID; ++i)
                            getWidget(i)->sendMessage(
                                widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
                        getWidget(g_config.m_soundVolume + EFFECTS_VOLUME_0_ID)
                            ->sendMessage(
                                widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
                        getWidget(g_config.m_soundVolume + EFFECTS_VOLUME_0_ID)
                            ->sendMessage(widget::WIDGET_SET_ICON_FRAME, g_config.m_soundVolume);
                        save = g_soundManager->m_playSounds;
                        g_soundManager->m_playSounds = MESSAGE_DISPATCH_CONSUME;
                        g_soundManager->adjustSoundVolumes();
                        g_soundManager->m_playSounds = save;
                        prefsChanged = 1;
                        m_prefsChanged = 1;
                        break;
                    }

                    case SHOW_PATH_ID:
                        g_config.m_showRoute ^= MESSAGE_DISPATCH_CONSUME;
                        getWidget(SHOW_PATH_ID)
                            ->sendMessage(widget::WIDGET_SET_ICON_FRAME,
                                          g_config.m_showRoute);
                        prefsChanged = 1;
                        m_prefsChanged = 1;
                        break;
                    case MOVE_REMINDER_ID:
                        g_config.m_moveReminder ^= MESSAGE_DISPATCH_CONSUME;
                        getWidget(MOVE_REMINDER_ID)
                            ->sendMessage(widget::WIDGET_SET_ICON_FRAME,
                                          g_config.m_moveReminder);
                        prefsChanged = 1;
                        m_prefsChanged = 1;
                        break;
                    case QUICK_COMBAT_ID:
                        g_config.m_quickCombat ^= MESSAGE_DISPATCH_CONSUME;
                        getWidget(QUICK_COMBAT_ID)
                            ->sendMessage(widget::WIDGET_SET_ICON_FRAME,
                                          g_config.m_quickCombat);
                        prefsChanged = 1;
                        m_prefsChanged = 1;
                        break;
                    case TOWN_OUTLINES_ID:
                        g_config.m_townOutlines ^= MESSAGE_DISPATCH_CONSUME;
                        getWidget(TOWN_OUTLINES_ID)
                            ->sendMessage(widget::WIDGET_SET_ICON_FRAME,
                                          g_config.m_townOutlines);
                        prefsChanged = 1;
                        m_prefsChanged = 1;
                        break;
                    case VIDEO_SUBTITLES_ID:
                        g_config.m_videoSubtitles ^= MESSAGE_DISPATCH_CONSUME;
                        getWidget(VIDEO_SUBTITLES_ID)
                            ->sendMessage(widget::WIDGET_SET_ICON_FRAME,
                                          g_config.m_videoSubtitles);
                        prefsChanged = 1;
                        m_prefsChanged = 1;
                        break;
                    case ANIMATE_SPELLBOOK_ID:
                        g_config.m_animateSpellBook ^= MESSAGE_DISPATCH_CONSUME;
                        getWidget(ANIMATE_SPELLBOOK_ID)
                            ->sendMessage(widget::WIDGET_SET_ICON_FRAME,
                                          g_config.m_animateSpellBook);
                        prefsChanged = 1;
                        m_prefsChanged = 1;
                        break;

                    default:
                        break;
                    }

                    if (prefsChanged)
                    {
                        updateSystemOptions(false);
                    }
                }
                break;
            }
            }
        }
    }
    if (exitFlag)
    {
        int command = msg.m_codeY;
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = command;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        msg.m_codeX = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// E:\gamedcs\systemoptionswindow.cpp:667
// Original public ?UpdateSystemOptions@TSystemOptionsWindow@@QAAX_N@Z
// proves bool firstUpdate; the lowered CodeView byte is not a byte contract.
DC_ADDRESS(0x160ce8, 0xa8)
MAC_ADDRESS(0x1ad0d4, 0x12c)
void TSystemOptionsWindow::updateSystemOptions(bool firstUpdate)
{
    message msg;
    msg.m_id = MESSAGE_WIDGET;
    if (firstUpdate && g_remoteOn) {
        getWidget(TMainMenu::RESTART_ID)->enable(0);
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_extra = widget::WIDGET_ACTIVE;
        broadcastMessage(msg);
        msg.m_codeY = TMainMenu::LOAD_GAME_ID;
        msg.m_codeX = widget::WIDGET_SET_STATUS;
        msg.m_extra = widget::WIDGET_DIMMED_NODRAW;
        broadcastMessage(msg);
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_extra = widget::WIDGET_ACTIVE;
        broadcastMessage(msg);
    }
    if (!firstUpdate)
        drawWindow(1, 0xffff0001, 0xffff);
}

// E:\gamedcs\systemoptionswindow.cpp:194
