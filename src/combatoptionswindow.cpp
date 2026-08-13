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
#include "soundmgr.h"
#include "textresource.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// Source-private in the Dreamcast compiland. Retail's constructor stores the
// active dialog here and its destructor clears it before widget teardown.
DATA(0x00694f90) static TCombatOptionsWindow* gpCombatOptionsWindow;

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

    int slot = 0;
    for (int musicX = 29; musicX < 219; musicX += 19) {
        Widgets.push_back(new iconWidget(
            musicX, 303, 18, 36, slot + MUSIC_VOLUME_0_ID, "syslb.def",
            0, 0, 0, 0, 0x10));
        slot++;
    }

    slot = 0;
    for (int effectsX = 29; effectsX < 219; effectsX += 19) {
        Widgets.push_back(new iconWidget(
            effectsX, 369, 18, 36, slot + EFFECTS_VOLUME_0_ID, "syslb.def",
            0, 0, 0, 0, 0x10));
        slot++;
    }

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

    for (int speed = COMBAT_SPEED_0_ID; speed <= COMBAT_SPEED_2_ID; ++speed)
        GetWidget(speed)->send_message(widget::WIDGET_CLEAR_STATUS,
            widget::WIDGET_DIMMED_NODRAW);
    GetWidget(gUnnamed698758.combatSpeed + COMBAT_SPEED_0_ID)->send_message(
        widget::WIDGET_SET_STATUS, widget::WIDGET_DIMMED_NODRAW);

    GetWidget(SHOW_GRID_ID)->send_message(widget::WIDGET_SET_ICON_FRAME,
        gUnnamed698758.showCombatGrid);
    GetWidget(MOVEMENT_SHADOW_ID)->send_message(widget::WIDGET_SET_ICON_FRAME,
        gUnnamed698758.combatShadeLevel);
    GetWidget(MOUSE_SHADOW_ID)->send_message(widget::WIDGET_SET_ICON_FRAME,
        gUnnamed698758.showCombatMouseHex);
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
// Retail inlines this switch into CombatOptionsWindowHandler.
#if 0  // @carcass
DC_ONLY(0x67a14, 0x2C)
int TCombatOptionsWindow::convertID2HelpID(int id) const
{
    // @stub
}
#endif

// E:\gamedcs\combatoptionswindow.cpp:214
VA(0x0046f780, 0x28)  // handler address-take + WritePrefs tail, dc 0x67a40
void TCombatOptionsWindow::DoModal()
{
    bPrefsChanged = 0;
    gpWindowManager->DoDialog(this, CombatOptionsWindowHandler, 0);
    if (bPrefsChanged)
        WritePrefs();
}

// The four highlight helpers are present out of line on Dreamcast, but every
// retail use is inlined into the constructor/handler; no retail entries occur
// between DoModal and the handler.
#if 0  // @carcass
DC_ONLY(0x67a78, 0x52)
void TCombatOptionsWindow::HighlightCombatSpeed() { /* @stub */ }
DC_ONLY(0x67acc, 0x22)
void TCombatOptionsWindow::HighlightGrid() { /* @stub */ }
DC_ONLY(0x67af0, 0x22)
void TCombatOptionsWindow::HighlightMovementShadow() { /* @stub */ }
DC_ONLY(0x67b14, 0x68)
void TCombatOptionsWindow::HighlightMouseShadow() { /* @stub */ }
#endif

// E:\gamedcs\combatoptionswindow.cpp:278
#if 0  // @carcass
VA(0x0046f7b0, 0x72A)  // DoModal address-take + complete message CFG, dc 0x67b7c
int CombatOptionsWindowHandler(message& msg)
{
    // @stub
}
#endif

// Dreamcast's UpdateCombatOptions follows the handler, but retail inlines it
// at both call sites. The next retail entry (0x46fee0) is an unrelated cinit.
#if 0  // @carcass
DC_ONLY(0x68098, 0x38)
void UpdateCombatOptions(unsigned char bFirstUpdate)
{
    // @stub
}
#endif

// E:\gamedcs\combatoptionswindow.cpp:171
#if 0  // @carcass -- represented by VA_COMPGEN above
DC_ONLY(0x680d0, 0x34)
void* TCombatOptionsWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}
#endif
