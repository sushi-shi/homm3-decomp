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
DATA(0x00694f90) static TCombatOptionsWindow* gpCombatOptionsWindow;

DATA(0x006a55ac) THelpText gCombatOptionsHelp[39];

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
    bPrefsChanged = 0;
    gpCombatOptionsWindow = this;
    Widgets.reserve(46);

    bitmapBorder* background = new bitmapBorder(
        0, 0, 481, 431, BACKGROUND_ID, "comopbck.pcx", 0x800);
    background->SetPlayerPaletteColors(
        gpCombatManager->playerIds[gpCombatManager->currentSide]);
    Widgets.push_back(background);

    Widgets.push_back(new button(
        246, 359, 100, 48, DEFAULT_ID, "codefaul.def", 1, 0, 0, 0, 2));

    button* accept = new button(
        357, 359, 100, 48, DIALOG_RETURN_SPLIT_ACCEPT, "soretrn.def",
        1, 0, 0, 28, 2);
    accept->set_hotkey(1);
    Widgets.push_back(accept);

    // The loop variable is the SLOT, not the x: retail keeps the raw 0..9 in
    // its frame slot and adds the id base at the use (`add edx,0xca`), while
    // the x lives in the linear-function-test-replaced derived induction
    // variable (`add edi,0x13`, `cmp edi,0xdb`). Walking x directly and
    // counting the slot by hand makes the slot a SECONDARY induction
    // variable, which VC6 folds the id base into (`mov [ebp-0x10],0xca`).
    for (int musicSlot = 0; musicSlot < 10; ++musicSlot)
        Widgets.push_back(new iconWidget(
            29 + musicSlot * 19, 303, 18, 36,
            musicSlot + MUSIC_VOLUME_0_ID, "syslb.def", 0, 0, 0, 0, 0x10));

    for (int effectsSlot = 0; effectsSlot < 10; ++effectsSlot)
        Widgets.push_back(new iconWidget(
            29 + effectsSlot * 19, 369, 18, 36,
            effectsSlot + EFFECTS_VOLUME_0_ID, "syslb.def",
            0, 0, 0, 0, 0x10));

    Widgets.push_back(new button(
        28, 225, 62, 32, COMBAT_SPEED_0_ID, "sysopb9.def", 0, 1, 0, 0, 2));
    Widgets.push_back(new button(
        92, 225, 62, 32, COMBAT_SPEED_0_ID + 1, "sysob10.def",
        0, 1, 0, 0, 2));
    Widgets.push_back(new button(
        156, 225, 62, 32, COMBAT_SPEED_2_ID, "sysob11.def", 0, 1, 0, 0, 2));

    Widgets.push_back(new iconWidget(
        246, 84, 32, 24, AUTO_CREATURES_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    Widgets.push_back(new iconWidget(
        246, 114, 32, 24, AUTO_SPELLS_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    Widgets.push_back(new iconWidget(
        246, 144, 32, 24, AUTO_CATAPULT_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    Widgets.push_back(new iconWidget(
        246, 174, 32, 24, AUTO_BALLISTA_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    Widgets.push_back(new iconWidget(
        246, 204, 32, 24, AUTO_FIRST_AID_TENT_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    Widgets.push_back(new iconWidget(
        246, 283, 32, 24, CREATURE_INFO_VERBOSE_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    Widgets.push_back(new iconWidget(
        246, 313, 32, 24, CREATURE_INFO_COMPACT_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    Widgets.push_back(new iconWidget(
        24, 55, 32, 24, SHOW_GRID_ID, "sysopchk.def", 0, 0, 0, 0, 0x10));
    Widgets.push_back(new iconWidget(
        24, 88, 32, 24, MOVEMENT_SHADOW_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    Widgets.push_back(new iconWidget(
        24, 122, 32, 24, MOUSE_SHADOW_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));
    Widgets.push_back(new iconWidget(
        24, 154, 32, 24, ANIMATE_SPELLBOOK_ID, "sysopchk.def",
        0, 0, 0, 0, 0x10));

    Widgets.push_back(new textWidget(
        26, 19, 432, 28, gpGeneralText->GetText(393), "bigfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    Widgets.push_back(new textWidget(
        26, 204, 193, 20, gpGeneralText->GetText(394), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    Widgets.push_back(new textWidget(
        26, 283, 193, 20, gpGeneralText->GetText(395), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    Widgets.push_back(new textWidget(
        26, 349, 193, 20, gpGeneralText->GetText(396), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    Widgets.push_back(new textWidget(
        248, 56, 211, 20, gpGeneralText->GetText(397), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));
    Widgets.push_back(new textWidget(
        248, 255, 211, 20, gpGeneralText->GetText(398), "medfont.fnt",
        font::HEADING, -1, 5, 0, 8));

    Widgets.push_back(new textWidget(
        283, 84, 182, 24, gpGeneralText->GetText(399), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    Widgets.push_back(new textWidget(
        283, 114, 182, 24, gpGeneralText->GetText(400), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    Widgets.push_back(new textWidget(
        283, 144, 182, 24, gpGeneralText->GetText(401), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    Widgets.push_back(new textWidget(
        283, 174, 182, 24, gpGeneralText->GetText(152), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    Widgets.push_back(new textWidget(
        283, 204, 182, 24, gpGeneralText->GetText(402), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    Widgets.push_back(new textWidget(
        283, 283, 182, 24, gpGeneralText->GetText(403), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    Widgets.push_back(new textWidget(
        283, 313, 182, 24, gpGeneralText->GetText(404), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));

    Widgets.push_back(new textWidget(
        61, 55, 168, 24, gpGeneralText->GetText(405), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    Widgets.push_back(new textWidget(
        61, 88, 168, 24, gpGeneralText->GetText(406), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    Widgets.push_back(new textWidget(
        61, 122, 168, 24, gpGeneralText->GetText(407), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));
    Widgets.push_back(new textWidget(
        61, 154, 168, 24, gpGeneralText->GetText(578), "medfont.fnt",
        font::PRIMARY, -1, 4, 0, 8));

    for (widget** it = Widgets.begin(); it != Widgets.end(); ++it) {
        if (*it)
            AddWidget(*it, -1);
        else
            MemError();
    }

    for (int music = MUSIC_VOLUME_0_ID; music <= MUSIC_VOLUME_9_ID; ++music)
        GetWidget(music)->send_message(widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_DRAWN);
    GetWidget(gUnk698760 + MUSIC_VOLUME_0_ID)->send_message(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
    GetWidget(gUnk698760 + MUSIC_VOLUME_0_ID)->send_message(
        widget::WIDGET_SET_ICON_FRAME, gUnk698760);

    for (int effects = EFFECTS_VOLUME_0_ID; effects <= EFFECTS_VOLUME_9_ID;
         ++effects)
        GetWidget(effects)->send_message(widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_DRAWN);
    GetWidget(gUnk698764 + EFFECTS_VOLUME_0_ID)->send_message(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
    GetWidget(gUnk698764 + EFFECTS_VOLUME_0_ID)->send_message(
        widget::WIDGET_SET_ICON_FRAME, gUnk698764);

    HighlightCombatSpeed();
    HighlightGrid();
    HighlightMovementShadow();
    HighlightMouseShadow();
    GetWidget(AUTO_CREATURES_ID)->send_message(widget::WIDGET_SET_ICON_FRAME,
        gUnnamed698758.combatAutoCreatures);
    GetWidget(AUTO_SPELLS_ID)->send_message(widget::WIDGET_SET_ICON_FRAME,
        gUnnamed698758.combatAutoSpells);
    GetWidget(AUTO_CATAPULT_ID)->send_message(widget::WIDGET_SET_ICON_FRAME,
        gUnnamed698758.combatCatapult);
    GetWidget(AUTO_BALLISTA_ID)->send_message(widget::WIDGET_SET_ICON_FRAME,
        gUnnamed698758.combatBallista);
    GetWidget(AUTO_FIRST_AID_TENT_ID)->send_message(
        widget::WIDGET_SET_ICON_FRAME, gUnnamed698758.combatFirstAidTent);
    GetWidget(CREATURE_INFO_VERBOSE_ID)->send_message(
        widget::WIDGET_SET_ICON_FRAME,
        gUnnamed698758.combatArmyInfoLevel == CREATURE_INFO_LEVEL_VERBOSE);
    GetWidget(CREATURE_INFO_COMPACT_ID)->send_message(
        widget::WIDGET_SET_ICON_FRAME,
        gUnnamed698758.combatArmyInfoLevel == CREATURE_INFO_LEVEL_COMPACT);
    GetWidget(ANIMATE_SPELLBOOK_ID)->send_message(
        widget::WIDGET_SET_ICON_FRAME, gUnnamed698758.animateSpellBook);

    gpMouseManager->SetPointer(0, mouseManager::DEFAULT_SET);
}

// Retail emits the generated wrapper immediately after the constructor;
// Dreamcast appends it to the compiland.
VA_COMPGEN(0x0046f6d0, 0x21, SCALAR_DELETING_DTOR, TCombatOptionsWindow)

// E:\gamedcs\combatoptionswindow.cpp:180
VA(0x0046f700, 0x75)  // vtable/global/widget teardown, dc 0x679ac
TCombatOptionsWindow::~TCombatOptionsWindow()
{
    gpCombatOptionsWindow = 0;
    for (widget** it = Widgets.begin(); it != Widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// E:\gamedcs\combatoptionswindow.cpp:194
// Retail inlines this mapping into CombatOptionsWindowHandler; no separate
// entry exists between the destructor and DoModal.
inline int TCombatOptionsWindow::convertID2HelpID(int id) const
{
    if (id == DIALOG_RETURN_SPLIT_ACCEPT)
        return 0;
    if (id >= DEFAULT_ID && id <= ANIMATE_SPELLBOOK_ID)
        return id - BACKGROUND_ID;
    return -1;
}

// E:\gamedcs\combatoptionswindow.cpp:214
VA(0x0046f780, 0x28)  // handler address-take + WritePrefs tail, dc 0x67a40
void TCombatOptionsWindow::DoModal()
{
    bPrefsChanged = 0;
    gpWindowManager->DoDialog(this, CombatOptionsWindowHandler, 0);
    if (bPrefsChanged)
        WritePrefs();
}

// The four highlight helpers are present out of line on Dreamcast (dc
// 0x67a78/0x67acc/0x67af0/0x67b14) but every retail use is expanded by /Ob2;
// no retail entry occurs between DoModal and the handler. They are real
// source, not open-coded loops: HighlightCombatSpeed's expansion holds its
// inlined `this` in EDI across the whole three-widget sweep, which is what
// retail's Default and speed cases do and what a direct
// `gpCombatOptionsWindow->GetWidget(...)` per iteration cannot produce.
// Spelled `inline` so no out-of-line copy lands in this obj either.
// E:\gamedcs\combatoptionswindow.cpp:230
inline void TCombatOptionsWindow::HighlightCombatSpeed()
{
    for (int speed = COMBAT_SPEED_0_ID; speed <= COMBAT_SPEED_2_ID; ++speed)
        GetWidget(speed)->send_message(widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_DIMMED_NODRAW);
    GetWidget(gUnnamed698758.combatSpeed + COMBAT_SPEED_0_ID)->send_message(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DIMMED_NODRAW);
}

// E:\gamedcs\combatoptionswindow.cpp:243
inline void TCombatOptionsWindow::HighlightGrid()
{
    GetWidget(SHOW_GRID_ID)->send_message(widget::WIDGET_SET_ICON_FRAME,
        gUnnamed698758.showCombatGrid);
}

// E:\gamedcs\combatoptionswindow.cpp:254
inline void TCombatOptionsWindow::HighlightMovementShadow()
{
    GetWidget(MOVEMENT_SHADOW_ID)->send_message(widget::WIDGET_SET_ICON_FRAME,
        gUnnamed698758.combatShadeLevel);
}

// E:\gamedcs\combatoptionswindow.cpp:265
inline void TCombatOptionsWindow::HighlightMouseShadow()
{
    GetWidget(MOUSE_SHADOW_ID)->send_message(widget::WIDGET_SET_ICON_FRAME,
        gUnnamed698758.showCombatMouseHex);
}

// Dreamcast homes this helper after the handler, but retail inlines it at
// both call sites (the next retail entry after the handler, 0x46fee0, is an
// unrelated cinit). Both sites pass 0, so the gate folds away.
__forceinline void UpdateCombatOptions(unsigned char bFirstUpdate)
{
    if (!bFirstUpdate)
        gpCombatOptionsWindow->bPrefsChanged = 1;
    gpCombatOptionsWindow->DrawWindow(1, 0xffff0001, 0xffff);
}

// E:\gamedcs\combatoptionswindow.cpp:278
//
// Residual (83.4%): the tail-block class, in both directions. Retail parks
// the translate-command and audio-unavailable arms at the tail and reaches
// them with `je`, where our CL hoists each to its single goto site; and
// retail expands every case's own `GetWidget(id)->send_message(...)` plus
// the `bPrefsChanged = 1` and shares only the DrawWindow tail, where our CL
// tail-merges the case bodies through a shared push sequence. The same
// class, mirrored, is CampaignWindowHandler's residual - two independent
// functions in this window family now show our SP3 CL taking the opposite
// tail-block placement from retail's, which is the standing
// compiler-generation suspect (match skill, "merged-return blocks"). The
// last register row is the right-click arm's `return`: retail spends the
// hoisted `mov eax,ebx` where ours re-materialises the immediate.
// The constant 1 is NOT a source variable - it is VC6's own B8 hoist into
// EBX. An earlier `register int one = MESSAGE_DISPATCH_CONSUME;` stood in
// for it; with the highlight helpers landed the hoist happens by itself and
// the plain literals score identically, so the crutch is gone.
VA(0x0046f7b0, 0x72A)  // DoModal address-take + complete message CFG, dc 0x67b7c
int CombatOptionsWindowHandler(message& msg)
{
    PollSound();

    if (msg.qualifier & MESSAGE_MODIFIER_RIGHT) {
        if (msg.codeX == widget::WIDGET_SELECT
                || msg.codeX == widget::WIDGET_RIGHT_SELECT) {
            int id = msg.codeY;
            if (id < 0)
                goto consume;
            int helpID = gpCombatOptionsWindow->convertID2HelpID(id);
            if (helpID >= 0)
                NormalDialog(gCombatOptionsHelp[helpID].text,
                    4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.id == MESSAGE_KEY_DOWN)
        goto consume;

    if (msg.id != MESSAGE_WIDGET)
        goto consume;

    switch (msg.codeX) {
    case widget::WIDGET_SELECT:
        goto handle_select;
    case widget::WIDGET_DESELECT:
        break;
    default:
        goto consume;
    }

    {
        int id = msg.codeY;
        if (id == DIALOG_RETURN_SPLIT_ACCEPT)
            goto translate_command;

        switch (id) {
        case TCombatOptionsWindow::DEFAULT_ID: {
            SetDefaultCombatOptions();
            gpCombatOptionsWindow->HighlightCombatSpeed();
            gpCombatOptionsWindow->HighlightGrid();
            gpCombatOptionsWindow->HighlightMovementShadow();
            gpCombatOptionsWindow->HighlightMouseShadow();
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::AUTO_CREATURES_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatAutoCreatures);
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::AUTO_SPELLS_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatAutoSpells);
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::AUTO_CATAPULT_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatCatapult);
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::AUTO_BALLISTA_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatBallista);
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::AUTO_FIRST_AID_TENT_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatFirstAidTent);
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::CREATURE_INFO_VERBOSE_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatArmyInfoLevel
                    == TCombatOptionsWindow::CREATURE_INFO_LEVEL_VERBOSE);
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::CREATURE_INFO_COMPACT_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatArmyInfoLevel
                    == TCombatOptionsWindow::CREATURE_INFO_LEVEL_COMPACT);
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::ANIMATE_SPELLBOOK_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.animateSpellBook);
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
            if (!gUnk698760 && !gpSoundManager->ds)
                goto audio_unavailable;
            gUnk698760 = id - TCombatOptionsWindow::MUSIC_VOLUME_0_ID;
            for (int music = TCombatOptionsWindow::MUSIC_VOLUME_0_ID;
                 music <= TCombatOptionsWindow::MUSIC_VOLUME_9_ID; ++music)
                gpCombatOptionsWindow->GetWidget(music)->send_message(
                    widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
            gpCombatOptionsWindow->GetWidget(gUnk698760
                    + TCombatOptionsWindow::MUSIC_VOLUME_0_ID)->send_message(
                widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
            gpCombatOptionsWindow->GetWidget(gUnk698760
                    + TCombatOptionsWindow::MUSIC_VOLUME_0_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME, gUnk698760);
            gpSoundManager->AdjustMusicVolumes();
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
            if (!gUnk698764 && !gpSoundManager->ds)
                goto audio_unavailable;
            gUnk698764 = id - TCombatOptionsWindow::EFFECTS_VOLUME_0_ID;
            gUnnamed698758.lastSoundVolume = gUnk698764;
            for (int effects = TCombatOptionsWindow::EFFECTS_VOLUME_0_ID;
                 effects <= TCombatOptionsWindow::EFFECTS_VOLUME_9_ID;
                 ++effects)
                gpCombatOptionsWindow->GetWidget(effects)->send_message(
                    widget::WIDGET_CLEAR_STATUS, widget::WIDGET_DRAWN);
            gpCombatOptionsWindow->GetWidget(gUnk698764
                    + TCombatOptionsWindow::EFFECTS_VOLUME_0_ID)->send_message(
                widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
            gpCombatOptionsWindow->GetWidget(gUnk698764
                    + TCombatOptionsWindow::EFFECTS_VOLUME_0_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME, gUnk698764);
            gpSoundManager->AdjustSoundVolumes();
            break;
        }

        case TCombatOptionsWindow::ANIMATE_SPELLBOOK_ID:
            gUnnamed698758.animateSpellBook ^= 1;
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::ANIMATE_SPELLBOOK_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.animateSpellBook);
            break;

        case TCombatOptionsWindow::AUTO_CREATURES_ID:
            gUnnamed698758.combatAutoCreatures ^= 1;
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::AUTO_CREATURES_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatAutoCreatures);
            break;

        case TCombatOptionsWindow::AUTO_SPELLS_ID:
            gUnnamed698758.combatAutoSpells ^= 1;
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::AUTO_SPELLS_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatAutoSpells);
            break;

        case TCombatOptionsWindow::AUTO_CATAPULT_ID:
            gUnnamed698758.combatCatapult ^= 1;
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::AUTO_CATAPULT_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatCatapult);
            break;

        case TCombatOptionsWindow::AUTO_BALLISTA_ID:
            gUnnamed698758.combatBallista ^= 1;
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::AUTO_BALLISTA_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatBallista);
            break;

        case TCombatOptionsWindow::AUTO_FIRST_AID_TENT_ID:
            gUnnamed698758.combatFirstAidTent ^= 1;
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::AUTO_FIRST_AID_TENT_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatFirstAidTent);
            break;

        case TCombatOptionsWindow::COMBAT_SPEED_0_ID:
        case TCombatOptionsWindow::COMBAT_SPEED_1_ID:
        case TCombatOptionsWindow::COMBAT_SPEED_2_ID: {
            gUnnamed698758.combatSpeed =
                id - TCombatOptionsWindow::COMBAT_SPEED_0_ID;
            gpCombatOptionsWindow->HighlightCombatSpeed();
            break;
        }

        case TCombatOptionsWindow::CREATURE_INFO_VERBOSE_ID:
            if (gUnnamed698758.combatArmyInfoLevel
                    != TCombatOptionsWindow::CREATURE_INFO_LEVEL_VERBOSE) {
                gUnnamed698758.combatArmyInfoLevel =
                    TCombatOptionsWindow::CREATURE_INFO_LEVEL_VERBOSE;
                gpCombatOptionsWindow->GetWidget(
                    TCombatOptionsWindow::CREATURE_INFO_COMPACT_ID)
                    ->send_message(widget::WIDGET_SET_ICON_FRAME, 0);
            } else {
                gUnnamed698758.combatArmyInfoLevel = 0;
            }
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::CREATURE_INFO_VERBOSE_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatArmyInfoLevel
                    == TCombatOptionsWindow::CREATURE_INFO_LEVEL_VERBOSE);
            break;

        case TCombatOptionsWindow::CREATURE_INFO_COMPACT_ID:
            if (gUnnamed698758.combatArmyInfoLevel
                    != TCombatOptionsWindow::CREATURE_INFO_LEVEL_COMPACT) {
                gUnnamed698758.combatArmyInfoLevel =
                    TCombatOptionsWindow::CREATURE_INFO_LEVEL_COMPACT;
                gpCombatOptionsWindow->GetWidget(
                    TCombatOptionsWindow::CREATURE_INFO_VERBOSE_ID)
                    ->send_message(widget::WIDGET_SET_ICON_FRAME, 0);
            } else {
                gUnnamed698758.combatArmyInfoLevel = 0;
            }
            gpCombatOptionsWindow->GetWidget(
                TCombatOptionsWindow::CREATURE_INFO_COMPACT_ID)->send_message(
                widget::WIDGET_SET_ICON_FRAME,
                gUnnamed698758.combatArmyInfoLevel
                    == TCombatOptionsWindow::CREATURE_INFO_LEVEL_COMPACT);
            break;

        case TCombatOptionsWindow::SHOW_GRID_ID:
            gUnnamed698758.showCombatGrid ^= 1;
            gpCombatOptionsWindow->HighlightGrid();
            break;

        case TCombatOptionsWindow::MOVEMENT_SHADOW_ID:
            gUnnamed698758.combatShadeLevel ^= 1;
            gpCombatOptionsWindow->HighlightMovementShadow();
            break;

        case TCombatOptionsWindow::MOUSE_SHADOW_ID:
            gUnnamed698758.showCombatMouseHex ^= 1;
            gpCombatOptionsWindow->HighlightMouseShadow();
            break;

        default:
            goto consume;
        }

        UpdateCombatOptions(0);
        return MESSAGE_DISPATCH_CONSUME;
    }

handle_select:
    {
        int id = gpCombatOptionsWindow->findWidget(msg.mouseX, msg.mouseY);
        if (id >= TCombatOptionsWindow::AUTO_CREATURES_ID
                && id <= TCombatOptionsWindow::ANIMATE_SPELLBOOK_ID
                && button::click_sample) {
            button::click_sample->field_2c = 0x40;
            button::click_sample->field_30 = 1;
            button::click_sample->field_28 = 3;
            gpSoundManager->MemorySample(button::click_sample);
        }
        return MESSAGE_DISPATCH_CONSUME;
    }

translate_command:
    msg.id = MESSAGE_WIDGET;
    gpWindowManager->dialogReturn = DIALOG_RETURN_SPLIT_ACCEPT;
    msg.codeY = widget::WIDGET_END_DIALOG;
    msg.codeX = widget::WIDGET_END_DIALOG;
    return MESSAGE_DISPATCH_FORWARD;

audio_unavailable:
    NormalDialog(
        gpGeneralText->GetText(
            GENERAL_TEXT_SYSTEM_OPTIONS_AUDIO_UNAVAILABLE),
        1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);

consume:
    return MESSAGE_DISPATCH_CONSUME;
}

// E:\gamedcs\combatoptionswindow.cpp:171
#if 0  // @carcass -- represented by VA_COMPGEN above
DC_ONLY(0x680d0, 0x34)
void* TCombatOptionsWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}
#endif
