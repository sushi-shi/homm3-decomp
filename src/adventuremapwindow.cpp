#include "va.h"

#include <stdio.h>

#include "adventuremapwindow.h"

#include "advmgr.h"
#include "border.h"
#include "bottomviewsubwindow.h"
#include "button.h"
#include "game.h"
#include "hero.h"
#include "iconwdgt.h"
#include "imm_mouse.h"
#include "inputmgr.h"
#include "kb.h"
#include "message.h"
#include "mousemgr.h"
#include "prefs.h"
#include "quest.h"
#include "remote.h"
#include "resourcedisplay.h"
#include "resourcemanager.h"
#include "textntry.h"
#include "textresource.h"
#include "textwdgt.h"
#include "town.h"
#include "townmgr_globals.h"
#include "widget.h"
#include "winmgr.h"

// 0x6a56e4 is g_adventureWindowHelp[0].m_rclick, not another array.
// Dreamcast's gQuickViewText belongs to the distinct object-name table.

// Dreamcast Game.h names both substitution alphabets. Retail's encoder reads
// the second pointer; retaining both definitions preserves the original
// adjacent data pair.
DATA(0x0065f220)
const char* TCheatCode::s_a = "abcdefghijklmnopqrstuvwxyz";
DATA(0x0065f224)
const char* TCheatCode::s_b = "nopqrstuvwxyzabcdefghijklm";

// Retail's gosolo handler at 0x4022e0 calls Dinkumware's string assignment
// at 0x404150. The shared <string> body expands here and remains emitted
// in advmgr, whose retail callers also use it; its enrollment lives there.

#if 0  // @carcass

// E:\gamedcs\adventuremapwindow.cpp:52. DC's ordinary derived member
// directly walks Widgets and calls widget::sleep at dc 0x39a. Complete
// generalizes that operation into heroWindow: its retained 0x5ff5b0 owns
// a new nesting counter and dispatches a virtual edge hook, whose base
// implementation 0x5ff5f0 owns the walk. The adventure override 0x4040b0
// additionally controls its mouse effect. This is a changed class boundary,
// not evidence that an otherwise identical TADW wrapper was inlined away.

// Three RETAIL-ONLY heroWindow virtual overrides the Dreamcast TADW class
// never carried (its field list marks every method VANILLA bar the dtor).
// All three are proven here by address-take: the retail TADW vtable
// ??_7TAdventureMapWindow@@6B@ (0x63a5e4) stores these entries where they
// diverge from heroWindow's own vtable (0x643cc4):
//   slot 1  0x401400 (167 B) (heroWindow::Open    0x5feae0) -> Open
//   slot 2  0x4014d0 ( 60 B) (heroWindow::Close   0x5fec60) -> Close
//   slot 8  0x4040b0 ( 56 B) (heroWindow::_vslot8 0x5ff5f0) -> unnamed
// They sit in the pre-band /Gy COMDAT region, not the linear TU body.
// Reconstruction is BLOCKED on advmgr.h: TAdventureMapWindow needs the three
// virtual declarations added to its class before a body can compile, and the
// header is owned outside this file - so these carry RETAIL_LOCATED markers
// (like the constructor below), NOT VA claims, because an uncompilable @stub
// would only bank a dead 0.0000 row against the unit.

// slot 1: int Open(int, unsigned char) - ret 8, thiscall, returns int
//   (heroWindow::Open is ?Open@heroWindow@@UAEHHE@Z). Creates the owned
//   +0x9c member.
//   RETAIL_LOCATED(0x00401400, 0xA7)  // anchor-vtable slot 1, retail-only
// slot 2: void Close(unsigned char) - ret 4. Deletes the +0x9c member
//   (call sub_b6e40 then operator delete), nulls it, then tail-calls
//   heroWindow::Close (0x5fec60, ?Close@heroWindow@@UAEXE@Z) - proving both
//   the override and that +0x9c is an owned pointer, not the pad_09c[4] the
//   header currently models.
//   RETAIL_LOCATED(0x004014d0, 0x3C)  // anchor-vtable slot 2, retail-only
// slot 8: void f(unsigned char) - ret 4. Overrides heroWindow's retail-era
//   slot-8 virtual (0x5ff5f0, the model's placeholder _vslot8, beyond the six
//   named heroWindow virtuals the DC dump carries), calls that base first,
//   then toggles the +0x9c member via TImmMouseEffect::stop/start on the bool arg.
//   Location, class and signature are proven; the METHOD NAME is not (no DC or
//   sibling attestation), so it stays unnamed.
//   RETAIL_LOCATED(0x004040b0, 0x38)  // anchor-vtable slot 8, retail-only

// E:\gamedcs\adventuremapwindow.cpp:505
// RETAIL_LOCATED(0x00402b90, 0x24)  // anchor-global, dc 0xbf0
void TAdventureMapWindow::animateBottomView(unsigned char in_background)
{
    // @stub
}

// E:\gamedcs\adventuremapwindow.cpp:609
// DECODED 2026-08-14, NOT reconstructed - the two blockers below are
// both in advmgr.h, which the external worker owns.

// The switch value is msg->codeY (message +8), not msg->id, matching
// the rest of the handler family. Retail dispatches ids 15..43 through
// a byte-index table at 0x402fe8 into three arms at 0x402fdc:

//   15..19, 39..43 -> hero quick view   (0x402ea7)
//   32..36         -> town quick view   (0x402f15)
//   20..31, 37, 38 -> default           (0x402f69)

// IDS 39..43 ARE THE SECOND HERO ROW. The hero arm normalises the id to
// a slot with `id - 15` below 39 and `id - 39` at or above it, then
// indexes topHero + slot into playerData's hero list exactly as the
// 15..19 arm does - i.e. the portrait buttons (HERO_0_ID..HERO_4_ID)
// and the five-entry highlight row this header already calls
// HeroLocators (+0x84) answer the same right-click. The Dreamcast
// EWidgetIDs enum stops at CHAT_EDIT_ID = 38, so 39..43 are Complete-era
// additions with no attested spelling; five HERO_LOCATOR_n_ID
// enumerators would satisfy the magic-case-label floor.

// The default arm calls convertID2HelpID, indexes an eight-byte record
// row at 0x6a56e4 by the returned help id, sizes the resulting text
// through kb's 0x4f62a0 and centres it with NormalDialog (0x4f6570,
// iMBType 4) at ((600 - h)/2 - 10, (592 - w)/2).

// BLOCKERS: advManager::HeroQuickView (0x416590) and
// advManager::TownQuickView (0x4167a0) are unclaimed and need
// declarations in advmgr.h, and the five ids above need enumerators in
// the same header's EWidgetIDs.
// RETAIL_LOCATED(0x00402e70, 0x195)  // anchor-global, dc 0xd88
unsigned char TAdventureMapWindow::processRightSelect(const message* msg)
{
    // @stub
}

// E:\gamedcs\adventuremapwindow.cpp:678
// DECODED 2026-08-14, NOT reconstructed - same advmgr.h blockers.

// findWidget(hx, hy) against the cached last-hovered id at .data
// 0x65f228 (an early-out when unchanged, and a mouse-pointer reset when
// it goes to -1), then a jump table at 0x4031f8 over ids 15..36 with
// its byte index at 0x403204:

//   15..29 -> the hero rollover chain (0x4030f6)
//   32..36 -> the town rollover       (0x40309a)
//   30, 31 -> default                 (0x403188)

// The hero arm is a range chain rather than more cases: 15..19 index
// playerData at -0x34, 20..24 at -0x48, 25..29 at -0x5c and the
// fall-through at -0x94, all off `topHero + id`. Ids 39..43 and
// 1001..1015 reach the tail with the rollover text left at its default,
// and everything else goes through convertID2HelpID into the SECOND
// dword of the same 0x6a56e4 record row (0x6a56e0).
// RETAIL_LOCATED(0x00403010, 0x20A)  // anchor-global, dc 0xed8
bool TAdventureMapWindow::processHover(int hx, int hy)
{
    // @stub
}

// E:\gamedcs\adventuremapwindow.cpp:1119
// RETAIL_LOCATED(0x004039b0, 0x1EA)  // anchor-global, dc 0x1134
void TAdventureMapWindow::updateQuestLogButton(bool update)
{
    // @stub
}

// E:\gamedcs\adventuremapwindow.cpp:1171
// RETAIL_LOCATED(0x00403bf0, 0x4C)  // anchor-global, dc 0x113c
void TAdventureMapWindow::updateSpellButton(const hero* this_hero)
{
    // @stub
}

// E:\gamedcs\adventuremapwindow.cpp:1216
// RETAIL_LOCATED(0x00403cc0, 0x215)  // anchor-global, dc 0x118c
void TAdventureMapWindow::setSleepImage(int image)
{
    // @stub
}

#endif  // @carcass

// Dreamcast adventuremapwindow.cpp proves this final derived editor. Retail's
// adventure-window constructor expands its forwarding constructor through
// CGameChatEdit, then writes this class's vtable after the shared +0x70 clear.
class CAdventurMapChatEdit : public CGameChatEdit {
public:
    CAdventurMapChatEdit(
        int textWidgetX, int textWidgetY, int textWidgetWidth,
        int textWidgetHeight, int textStringSize, char* textString,
        char* textFontName, int colorIndex,
        font::EJustify justification,
        char* backgroundIconName, int backgroundFrame, int textWidgetId,
        int textWidgetStyle, int readType, int textInsetX, int textInsetY);
    virtual void sendChat(const char* text, int toWho);
};

DC_ADDRESS(0x003274, 0x98)
CAdventurMapChatEdit::CAdventurMapChatEdit(
    int textWidgetX, int textWidgetY, int textWidgetWidth,
    int textWidgetHeight, int textStringSize, char* textString,
    char* textFontName, int colorIndex, font::EJustify justification,
    char* backgroundIconName, int backgroundFrame, int textWidgetId,
    int textWidgetStyle, int readType, int textInsetX, int textInsetY)
    : CGameChatEdit(textWidgetX, textWidgetY, textWidgetWidth,
                    textWidgetHeight, textStringSize, textString,
                    textFontName, colorIndex, justification,
                    backgroundIconName, backgroundFrame, textWidgetId,
                    textWidgetStyle, readType, textInsetX, textInsetY)
{
}

VA(0x00401400, 0xC5)
MAC_ADDRESS(0x00026c, 0x20)
int TAdventureMapWindow::open(int zOrder, bool update)
{
    // Mac's override at 0:0x26c only forwards to heroWindow::open;
    // the immersion mouse-effect lifetime belongs to Windows.
    int result = heroWindow::open(zOrder, update);
    if (result == 0) {
        RECT area;
        area.left = 0;
        area.top = 0;
        area.right = 800;
        area.bottom = 600;
        try {
            TImmMouseEffect* effect =
                new TImmMouseEffect(&area, 10000, 16, 10000, 1, 0);
            m_immersion = effect;
            effect->start();
        }
        catch (...) {
        }
    }
    return result;
}

VA(0x004014d0, 0x3C)
MAC_ADDRESS(0x00028c, 0x20)
void TAdventureMapWindow::close(unsigned char update)
{
    // Mac 0:0x28c has only the base close call, without immersion teardown.
    delete static_cast<TImmMouseEffect*>(m_immersion);
    m_immersion = 0;
    heroWindow::close(update);
}

// 2026-10-07 trace: the only residual is the m_chatEdit push_back's nested
// insert, whose first size() sees depth-4 budget 38 against cost 42 (retail
// expands it). The counterfactual simulator flips it if the chat-edit
// constructor chain costs about 38 less (CAdventurMapChatEdit <= 80). Natural
// for-loops for the three icon rows fall to 96.36%; scoping unusedText before
// the chat edit overshoots (91.66%).
VA(0x00401510, 0xCB5)
DC_ADDRESS(0x00089c, 0x300)
MAC_ADDRESS(0x000a34, 0x15e0)
TAdventureMapWindow::TAdventureMapWindow()
    : heroWindow(0, 0, 800, 600, 1),
      m_topHero(0),
      m_topTown(0),
      m_immersion(0)
{
    m_animateInBackground = 0;
    m_chatTextWidget = 0;

    m_widgets.reserve(39);

    m_widgets.push_back(new border(8, 8, 592, 544, MAP_ID, 1));
    m_mapWidget = m_widgets.back();
    m_widgets.push_back(new border(630, 26, 144, 144, RADAR_ID, 1));
    m_radarWidget = m_widgets.back();
    m_widgets.push_back(new border(605, 389, 188, 182,
                                 SELECTION_WINDOW_ID, 1));

    button* newButton = new button(
        679, 196, 32, 32, KINGDOM_OVERVIEW_ID,
        DATA_COMPGEN(0x0065F3C0, adventureIam002, "iam002.def"),
        0, 1, 0, 0, 2);
    newButton->setHotkey(0x25);
    m_widgets.push_back(newButton);

    newButton = new button(
        711, 196, 32, 32, ELEVATION_TOGGLE_ID,
        DATA_COMPGEN(0x0065F268, adventureIam010, "iam010.def"),
        0, 1, 0, 0, 2);
    newButton->setHotkey(0x16);
    m_widgets.push_back(newButton);

    newButton = new button(
        679, 228, 32, 32, QUEST_LOG_ID,
        DATA_COMPGEN(0x0065F3B4, adventureIam004, "iam004.def"),
        0, 1, 0, 0, 2);
    newButton->setHotkey(0x10);
    m_widgets.push_back(newButton);

    newButton = new button(
        711, 228, 32, 32, SLEEP_ID,
        DATA_COMPGEN(0x0065F250, adventureIam005, "iam005.def"),
        0, 1, 0, 0, 2);
    newButton->setHotkey(0x2c);
    m_widgets.push_back(newButton);

    newButton = new button(
        679, 260, 32, 32, MOVE_ID,
        DATA_COMPGEN(0x0065F3A8, adventureIam006, "iam006.def"),
        0, 1, 0, 0, 2);
    newButton->setHotkey(0x32);
    m_widgets.push_back(newButton);

    newButton = new button(
        711, 260, 32, 32, CAST_SPELL_ID,
        DATA_COMPGEN(0x0065F39C, adventureIam007, "iam007.def"),
        0, 1, 0, 0, 2);
    newButton->setHotkey(0x2e);
    m_widgets.push_back(newButton);

    newButton = new button(
        679, 292, 32, 32, ADVENTURE_OPTIONS_ID,
        DATA_COMPGEN(0x0065F390, adventureIam008, "iam008.def"),
        0, 1, 0, 0, 2);
    newButton->setHotkey(0x1e);
    m_widgets.push_back(newButton);

    newButton = new button(
        711, 292, 32, 32, SYSTEM_OPTIONS_ID,
        DATA_COMPGEN(0x0065F384, adventureIam009, "iam009.def"),
        0, 1, 0, 0, 2);
    newButton->setHotkey(0x18);
    m_widgets.push_back(newButton);

    newButton = new button(
        679, 324, 64, 32, NEXT_HERO_ID,
        DATA_COMPGEN(0x0065F378, adventureIam000, "iam000.def"),
        0, 1, 0, 0, 2);
    newButton->setHotkey(0x23);
    m_widgets.push_back(newButton);

    newButton = new button(
        679, 356, 64, 32, END_TURN_ID,
        DATA_COMPGEN(0x0065F36C, adventureIam001, "iam001.def"),
        0, 1, 0, 0, 2);
    newButton->setHotkey(0x12);
    m_widgets.push_back(newButton);

    m_widgets.push_back(new button(
        609, 196, 64, 16, HERO_UP_ID,
        DATA_COMPGEN(0x0065F360, adventureIam012, "iam012.def"),
        0, 1, 0, 0, 2));
    m_widgets.push_back(new button(
        609, 372, 64, 16, HERO_DOWN_ID,
        DATA_COMPGEN(0x0065F354, adventureIam013, "iam013.def"),
        0, 1, 0, 0, 2));

    std::string unusedText;

    int i;
    int y = 212;
    for (i = 0; i < NUM_HERO_BUTTONS; ++i) {
        m_heroPortraits[i] = new bitmapBorder(
            617, y, 48, 32, HERO_0_ID + i, 0, 0x800);
        m_widgets.push_back(m_heroPortraits[i]);
        y += 32;
    }
    y = 212;
    for (i = 0; i < NUM_HERO_BUTTONS; ++i) {
        m_heroLocators[i] = new bitmapBorder(
            617, y, 48, 32, HERO_LOCATOR_0_ID + i, 0, 0x800);
        m_widgets.push_back(m_heroLocators[i]);
        y += 32;
    }
    {
        int y = 213;
        int widgetId = HERO_MOVEMENT_0_ID;
        int remaining = NUM_HERO_BUTTONS;
        do {
            m_widgets.push_back(new iconWidget(
                610, y, 8, 32, widgetId,
                DATA_COMPGEN(0x0065F348, adventureImobil, "imobil.def"),
                0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
            ++widgetId;
            y += 32;
        } while (--remaining);
    }
    {
        int y = 213;
        int widgetId = HERO_MANA_0_ID;
        int remaining = NUM_HERO_BUTTONS;
        do {
            m_widgets.push_back(new iconWidget(
                666, y, 8, 32, widgetId,
                DATA_COMPGEN(0x0065F33C, adventureImana, "imana.def"),
                0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
            ++widgetId;
            y += 32;
        } while (--remaining);
    }

    m_widgets.push_back(new button(
        747, 196, 48, 16, TOWN_UP_ID,
        DATA_COMPGEN(0x0065F330, adventureIam014, "iam014.def"),
        0, 1, 0, 0, 2));
    m_widgets.push_back(new button(
        747, 372, 48, 16, TOWN_DOWN_ID,
        DATA_COMPGEN(0x0065F324, adventureIam015, "iam015.def"),
        0, 1, 0, 0, 2));

    {
        int y = 212;
        int widgetId = TOWN_0_ID;
        int remaining = NUM_TOWN_BUTTONS;
        do {
            m_widgets.push_back(new iconWidget(
                747, y, 48, 32, widgetId,
                DATA_COMPGEN(0x0065F318, adventureItpa, "itpa.def"),
                0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
            ++widgetId;
            y += 32;
        } while (--remaining);
    }

    m_rolloverTextWidget = new bitmapBackedTextWidget(
        8, 556, 592, 18, 0,
        DATA_COMPGEN(0x0065F2F8, adventureSmallFont, "smalfont.fnt"),
        DATA_COMPGEN(0x0065F308, adventureRolloverBack, "AdRollvr.pcx"),
        font::PRIMARY, ROLLOVER_TEXT_ID, font::CENTER_JUSTIFIED, 8);
    m_widgets.push_back(m_rolloverTextWidget);

    m_chatTextWidget = new textWidget(
        54, 100, 520, 440, 0,
        DATA_COMPGEN(0x0065F2EC, adventureMediumFont, "medfont.fnt"),
        font::CHAT, CHAT_TEXT_ID, font::BOTTOM_JUSTIFIED, 0, 8);
    // Dreamcast line 457 proves the canonical widget-vector append.
    m_widgets.push_back(m_chatTextWidget);

    m_chatEdit = new CAdventurMapChatEdit(
        8, 556, 592, 18, 127,
        DATA_COMPGEN(0x00691210, adventureRolloverEmptyText, ""),
        DATA_COMPGEN(0x0065F2F8, adventureChatSmallFont, "smalfont.fnt"),
        font::WHITE, font::LEFT_JUSTIFIED,
        DATA_COMPGEN(0x0065F308, adventureChatBackground, "AdRollvr.pcx"),
        0, CHAT_EDIT_ID, 0x100, 0, 7, 5);
    m_widgets.push_back(m_chatEdit);

    for (std::vector<widget*>::iterator it = m_widgets.begin();
         it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    m_bottomView = 0;
    m_resourceDisplay = new TResourceDisplay(this, 0);
}

// Defined at its retail address below; SendChat is its only caller and
// precedes it in the retail link order.
void checkAdvCheatCode(std::string& chatString);

// E:\gamedcs\adventuremapwindow.cpp:261. The adventure editor's chat sink,
// In a single-player game the line first goes through the cheat
// scanner; the one cheat handled HERE rather than in CheckAdvCheatCode is
// "gosolo", which turns the whole preferences block over to unattended play -
// auto creatures, auto spells and all three war machines on, combat speed 2,
// both walk speeds 4 - and latches the local player position so the adventure
// manager knows whose turn is being watched. Outside a network game it also
// lifts the map visibility mask.

// The preference fields are the misc.obj block at 0x698758 whose registry
// names prefs.h records; the five combat toggles are its +0x3c..+0x4c run.

// Residual (77.49%): ONE over-inline, and everything else is downstream of
// it. Retail CALLS basic_string::assign(const char*, unsigned) to build
// `chatString`; our compile expands it into _Grow + _Eos + an inline
// rep movsd, which is 36 bytes of surplus body and the register pressure that
// then costs the shared `1` constant retail keeps in EBX across the
// GetLocalPlayerGamePos call (retail stores `bl` and five `ebx`, we
// re-materialise). The call multiset is otherwise identical, 6 of 7 calls
// pair and the statement order matches. Measured and byte-flat: the explicit
// `std::string chatString(sChat)` ctor (77.49); measured and 0.07 higher but
// less faithful to retail's _Tidy-then-assign shape: default construction
// followed by `chatString = sChat` or by `assign(sChat, strlen(sChat))`
// (77.56 both).
// Passive trace (2026-09-07, identical 400-byte candidate): caller cb=184,
// initial budget=1000 and four top-level candidates. The string constructor
// receives 250; nested assign(const char*, size) receives 82 for cost 69 and
// expands. Restoring DC's CheckAdvCheatCode-before-SendChat definition order
// is byte-identical at 77.4872%, so source order does not explain this site.
// DC 262 proves the retained const-char-pointer constructor; default-string
// assignment controls do not recover that source boundary.
// 2026-10-07 trace: the constructor is depth-1 site 1 of 4 (op==, c_str,
// ~string follow); its nested assign keeps cost 69 only once five sites
// remain ((1000 - 61) / 5 - 152 < 69). Mac 0x3e74 and DC 262..311 show the
// same calls; DC's unrecorded 288..308 band does not identify a fifth.
VA(0x004022e0, 0x167)
DC_ADDRESS(0x00330c, 0x108)
MAC_ADDRESS(0x003e74, 0x118)  // anchor-string("gosolo") + anchor-callee(CheckAdvCheatCode)
void CAdventurMapChatEdit::sendChat(const char* chat, int toWho)
{
    std::string chatString = chat;

    if (!g_game->isMultiplayer())
        checkAdvCheatCode(chatString);

    if (chatString == DATA_COMPGEN(0x0065f3cc, advChatGoSolo, "gosolo")) {
        if (!g_remoteOn)
            g_mapVisibilityBit = 0xff;
        g_goSolo = 1;
        g_soloPos = g_game->getLocalPlayerGamePos();
        g_config.m_combatBallista = 1;
        g_config.m_combatCatapult = 1;
        g_config.m_combatAutoCreatures = 1;
        g_config.m_combatFirstAidTent = 1;
        g_config.m_combatAutoSpells = 1;
        g_config.m_combatSpeed = 2;
        g_config.m_walkSpeed[0] = 4;
        g_config.m_walkSpeed[1] = 4;
    }

    ::sendChat(chatString.c_str(), toWho);
    sendChatCleanup();
}

// E:\gamedcs\adventuremapwindow.cpp:63
VA(0x00402450, 0x5D3)
DC_ADDRESS(0x0003b0, 0x4ec)
MAC_ADDRESS(0x0002ac, 0x69c)  // anchor-global
void checkAdvCheatCode(std::string& chatString)
{
    // Retail keeps every cheat code as its own .rdata array in source
    // order (the combat copies of shared codes are separate objects),
    // not as a pooled .data literal.
    DATA(0x0063a480) static const char trinityCode[] = "ajpgevavgl";
    DATA(0x0063a48c) static const char agentsCode[] = "ajpntragf";
    DATA(0x0063a498) static const char lotsOfGunsCode[] = "ajpybgfbsthaf";
    DATA(0x0063a4a8) static const char neoCode[] = "ajparb";
    DATA(0x0063a4b0) static const char followTheWhiteRabbitCode[] = "ajpsbyybjgurjuvgrenoovg";
    DATA(0x0063a4c8) static const char nebuchadnezzarCode[] = "ajparohpunqarmmne";
    DATA(0x0063a4dc) static const char morpheusCode[] = "ajpzbecurhf";
    DATA(0x0063a4e8) static const char oracleCode[] = "ajpbenpyr";
    DATA(0x0063a4f4) static const char whatIsTheMatrixCode[] = "ajpjungvfgurzngevk";
    DATA(0x0063a508) static const char ignoranceIsBlissCode[] = "ajpvtabenaprvfoyvff";
    DATA(0x0063a51c) static const char theConstructCode[] = "ajpgurpbafgehpg";
    DATA(0x0063a52c) static const char bluePillCode[] = "ajpoyhrcvyy";
    DATA(0x0063a538) static const char redPillCode[] = "ajperqcvyy";
    DATA(0x0063a544) static const char thereIsNoSpoonCode[] = "ajpgurervfabfcbba";
    DATA(0x0063a558) static const char zionCode[] = "ajpmvba";
    DATA(0x0063a560) static const char phisherPriceCode[] = "ajpcuvfurecevpr";
    hero* currentHero = g_game->getCurrHero();
    TCheatCode code(chatString.c_str());
    bool cheatUsed = false;

    if (code.compare(trinityCode)
        && currentHero) {
        cheatUsed = true;
        for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; slot++) {
            if (currentHero->m_army.m_armies[slot] == -1)
                currentHero->m_army.add(CREATURE_ARCHANGEL, 5, slot);
        }
        g_advManager->updBottomView(1, 1, 1);
    } else if (code.compare(agentsCode)
               && currentHero) {
        cheatUsed = true;
        for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; slot++) {
            if (currentHero->m_army.m_armies[slot] == -1)
                currentHero->m_army.add(CREATURE_BLACK_KNIGHT, 10, slot);
        }
        g_advManager->updBottomView(1, 1, 1);
    } else if (code.compare(lotsOfGunsCode)
               && currentHero) {
        cheatUsed = true;
        if (!currentHero->hasArtifact(ARTIFACT_AMMO_CART)) {
            type_artifact artifact(ARTIFACT_AMMO_CART);
            currentHero->giveArtifact(artifact, 0, 0);
        }
        if (!currentHero->hasArtifact(ARTIFACT_BALLISTA)) {
            type_artifact artifact(ARTIFACT_BALLISTA);
            currentHero->giveArtifact(artifact, 0, 0);
        }
        if (!currentHero->hasArtifact(ARTIFACT_FIRST_AID_TENT)) {
            type_artifact artifact(ARTIFACT_FIRST_AID_TENT);
            currentHero->giveArtifact(artifact, 0, 0);
        }
    } else if (code.compare(neoCode)
               && currentHero) {
        cheatUsed = true;
        int increment = currentHero->getExperienceIncrement();
        currentHero->giveExperience(increment, 1, 1);
    } else if (code.compare(followTheWhiteRabbitCode)
               && currentHero) {
        cheatUsed = true;
        currentHero->m_flags |= 0x00400000;
        g_advManager->updBottomView(1, 1, 1);
    } else if (code.compare(nebuchadnezzarCode)
               && currentHero) {
        cheatUsed = true;
        currentHero->m_flags |= 0x01000000;
        int mobility = currentHero->getMobility();
        currentHero->m_movePoints = mobility;
        currentHero->m_maxMovePoints = mobility;
    } else if (code.compare(morpheusCode)
               && currentHero) {
        cheatUsed = true;
        currentHero->m_flags |= 0x00800000;
        g_advManager->updBottomView(1, 1, 1);
    } else if (code.compare(oracleCode)) {
        cheatUsed = true;
        g_currentPlayer->m_extraPuzzlePieces = 0x30;
        g_advManager->viewPuzzle();
    } else if (code.compare(whatIsTheMatrixCode)) {
        cheatUsed = true;
        for (int level = 0; level < g_game->getNumMapLevels(); level++)
            g_game->setVisibility(0, 0, level, g_netLocalGamePos, 200, 0);
        if (currentHero)
            g_advManager->reseed(0, 0);
        g_advManager->redrawAdvScreen(1, 0);
    } else if (code.compare(ignoranceIsBlissCode)) {
        cheatUsed = true;
        for (int level = 0; level < g_game->getNumMapLevels(); level++)
            g_game->resetVisibility(0, 0, level, -1, 200);
        g_game->resetAllPlayerVisibility();
        if (currentHero)
            g_advManager->reseed(0, 0);
        g_advManager->redrawAdvScreen(1, 0);
    } else if (code.compare(theConstructCode)) {
        cheatUsed = true;
        for (int resource = 0; resource < 7; resource++)
            g_currentPlayer->m_resources[resource] +=
                resource == GOLD ? 100000 : 100;
        g_advManager->m_advWindow->updateResourceDisplay(1, 1);
    } else if (code.compare(bluePillCode)) {
        cheatUsed = true;
        checkEndGame(2);
    } else if (code.compare(redPillCode)) {
        cheatUsed = true;
        checkEndGame(1);
    } else if (code.compare(thereIsNoSpoonCode)
               && currentHero) {
        cheatUsed = true;
        currentHero->m_mana = 999;
        if (!currentHero->isWieldingArtifact(ARTIFACT_SPELLBOOK)) {
            type_artifact spellbook(ARTIFACT_SPELLBOOK);
            currentHero->giveArtifact(spellbook, 1, 1);
        }
        for (int spell = 0; spell < hero::NUM_SPELLS; spell++)
            currentHero->addSpell(spell);
        g_advManager->updBottomView(1, 1, 1);
    } else if (code.compare(zionCode)) {
        cheatUsed = true;
        g_buildAllBuildings = !g_buildAllBuildings;
    } else if (code.compare(phisherPriceCode)) {
        g_graphicsSaturated = !g_graphicsSaturated;
        if (!g_graphicsSaturated)
            ResourceManager::remapGraphics();
        else
            ResourceManager::saturateGraphics();
        g_advManager->redrawAdvScreen(1, 0);
    }

    if (cheatUsed) {
        // DC line211 calls TTextResource::operator[] here. Keep its canonical
        // getText path together with the shared cheat marker: that boundary
        // restores the retail string-assignment expansion in both handlers.
        chatString = (*g_generalText)[GENERAL_TEXT_CHEATER];
        markGameAsCheated();
    }
}

// slot 0 of ??_7TAdventureMapWindow@@6B@ (0x63a5e4) points at it.
VA_COMPGEN(0x00402ae0, 0x21, SCALAR_DELETING_DTOR, TAdventureMapWindow)

VA(0x00402b10, 0x77)
DC_ADDRESS(0x000b9c, 0x54)
MAC_ADDRESS(0x002160, 0x94)
TAdventureMapWindow::~TAdventureMapWindow()
{
    if (m_resourceDisplay)
        delete m_resourceDisplay;
    clearBottomView();
    deleteWidgets();
}

VA(0x00402b90, 0x24)
DC_ADDRESS(0x000bf0, 0x2a)
MAC_ADDRESS(0x0021f4, 0x4c)
void TAdventureMapWindow::animateBottomView(unsigned char inBackground)
{
    if ((!inBackground || m_animateInBackground) && m_bottomView)
        m_bottomView->animate();
}

VA(0x00402bc0, 0x4E)
DC_ADDRESS(0x000c1c, 0x40)
MAC_ADDRESS(0x002240, 0x80)
void TAdventureMapWindow::drawBottomView(unsigned char update)
{
    if (m_bottomView) {
        m_bottomView->draw(0, 0xffff0001, 0xffff);
        if (update)
            g_windowManager->updateScreen(m_bottomView->m_x, m_bottomView->m_y,
                m_bottomView->m_width, m_bottomView->m_height);
    }
}

VA(0x00402c10, 0x3C)
DC_ADDRESS(0x000c5c, 0x1e)
MAC_ADDRESS(0x0022c0, 0x3c)
void TAdventureMapWindow::setBottomView(type_bottom_view_window* newView)
{
    clearBottomView();
    m_bottomView = newView;
}

VA(0x00402c50, 0x218)
DC_ADDRESS(0x000c7c, 0x10c)
MAC_ADDRESS(0x0022fc, 0x220)
int TAdventureMapWindow::convertID2HelpID(int id) const
{
    if (id < 0)
        return -1;

    switch (id) {
    case RADAR_ID:             return 0;
    case SELECTION_WINDOW_ID:  return 1;
    case KINGDOM_OVERVIEW_ID:  return 2;
    case ELEVATION_TOGGLE_ID:  return 3;
    case QUEST_LOG_ID:         return 4;
    case SLEEP_ID:             return 5;
    case MOVE_ID:              return 6;
    case CAST_SPELL_ID:        return 7;
    case ADVENTURE_OPTIONS_ID: return 8;
    case SYSTEM_OPTIONS_ID:    return 9;
    case NEXT_HERO_ID:         return 10;
    case END_TURN_ID:          return 11;
    case HERO_UP_ID:           return 12;
    case HERO_DOWN_ID:         return 13;

    case HERO_0_ID:
    case HERO_1_ID:
    case HERO_2_ID:
    case HERO_3_ID:
    case HERO_4_ID:
    case HERO_MOVEMENT_0_ID:
    case HERO_MOVEMENT_1_ID:
    case HERO_MOVEMENT_2_ID:
    case HERO_MOVEMENT_3_ID:
    case HERO_MOVEMENT_4_ID:
    case HERO_MANA_0_ID:
    case HERO_MANA_1_ID:
    case HERO_MANA_2_ID:
    case HERO_MANA_3_ID:
    case HERO_MANA_4_ID:
    case HERO_LOCATOR_0_ID:
    case HERO_LOCATOR_1_ID:
    case HERO_LOCATOR_2_ID:
    case HERO_LOCATOR_3_ID:
    case HERO_LOCATOR_4_ID:
        return 14;

    case TOWN_UP_ID: return 15;
    case TOWN_DOWN_ID: return 16;
    case TOWN_0_ID:
    case TOWN_1_ID:
    case TOWN_2_ID:
    case TOWN_3_ID:
    case TOWN_4_ID:
        return 17;

    case HELP_WIDGET_1007_ID:
    case HELP_WIDGET_1015_ID:
        return 18;
    case HELP_WIDGET_1001_ID:
    case HELP_WIDGET_1009_ID:
        return 19;
    case HELP_WIDGET_1002_ID:
    case HELP_WIDGET_1010_ID:
        return 20;
    case HELP_WIDGET_1003_ID:
    case HELP_WIDGET_1011_ID:
        return 21;
    case HELP_WIDGET_1004_ID:
    case HELP_WIDGET_1012_ID:
        return 22;
    case HELP_WIDGET_1005_ID:
    case HELP_WIDGET_1013_ID:
        return 23;
    case HELP_WIDGET_1006_ID:
    case HELP_WIDGET_1014_ID:
        return 24;
    case HELP_WIDGET_1008_ID:
        return 25;
    case ROLLOVER_TEXT_ID:
        return 26;
    }
    return -1;
}

// Last adventure-window widget whose rollover was drawn. Initial -1 is what
// places this cache in .data immediately before the icon-name tables below.
DATA(0x0065f228)
static int g_lastAdventureHover = -1;

// The two input handlers are identity/arity claims while their decoded
// bodies remain on the dependency frontier documented in the carcass above.
VA(0x00402e70, 0x195)
DC_ADDRESS(0x000d88, 0x14e)
MAC_ADDRESS(0x00251c, 0x204)
unsigned char TAdventureMapWindow::processRightSelect(const message* msg)
{
    playerData* player = g_game->getLocalPlayer();

    switch (msg->m_codeY) {
    case HERO_0_ID:
    case HERO_1_ID:
    case HERO_2_ID:
    case HERO_3_ID:
    case HERO_4_ID:
    case HERO_LOCATOR_0_ID:
    case HERO_LOCATOR_1_ID:
    case HERO_LOCATOR_2_ID:
    case HERO_LOCATOR_3_ID:
    case HERO_LOCATOR_4_ID: {
        int slot = msg->m_codeY - HERO_0_ID;
        if (msg->m_codeY >= HERO_LOCATOR_0_ID)
            slot = msg->m_codeY - HERO_LOCATOR_0_ID;

        widget* portrait = getWidget(msg->m_codeY);
        if (!portrait)
            return 0;

        if (m_topHero + slot >= player->m_numHeroes)
            return 0;

        g_advManager->heroQuickView(player->m_heroes[m_topHero + slot], 508,
            portrait->m_y + portrait->m_height / 2, 0);
        return 1;
    }

    case TOWN_0_ID:
    case TOWN_1_ID:
    case TOWN_2_ID:
    case TOWN_3_ID:
    case TOWN_4_ID: {
        widget* portrait = getWidget(msg->m_codeY);
        if (!portrait)
            return 0;

        g_advManager->townQuickView(
            player->m_townIds[m_topTown + msg->m_codeY - TOWN_0_ID], 508,
            portrait->m_y + portrait->m_height / 2, 0);
        return 1;
    }

    default: {
        int helpID = convertID2HelpID(msg->m_codeY);
        if (helpID < 0)
            return 0;

        const char* text = g_adventureWindowHelp[helpID].m_rclick;
        int width;
        int height;
        getQuickviewSize(text, &width, &height);
        normalDialog(text, 4, 592 - width, (600 - height) / 2 - 10,
            -1, 0, -1, 0, -1, 0, -1, 0);
        return 1;
    }
    }
}

// Original ProcessHover@TAdventureMapWindow@@QAA_NHH@Z proves bool.
VA(0x00403010, 0x20A)
DC_ADDRESS(0x000ed8, 0x204)
MAC_ADDRESS(0x002720, 0x2ec)
bool TAdventureMapWindow::processHover(int hx, int hy)
{
    playerData* player = g_game->getLocalPlayer();
    if (m_chatEdit->m_hasFocus)
        return true;

    int hoverID = findWidget(hx, hy);
    if (hoverID != g_lastAdventureHover) {
        const char* rolloverText = DATA_COMPGEN(
            0x00691210, adventureRolloverEmptyText, "");
        g_lastAdventureHover = hoverID;

        if (hoverID == -1) {
            g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
        } else if (hoverID <= HERO_LOCATOR_4_ID) {
            if (hoverID >= HERO_LOCATOR_0_ID)
                goto hero_rollover;

            switch (hoverID) {
            case TOWN_0_ID:
            case TOWN_1_ID:
            case TOWN_2_ID:
            case TOWN_3_ID:
            case TOWN_4_ID:
                goto town_rollover;

            case HERO_0_ID:
            case HERO_1_ID:
            case HERO_2_ID:
            case HERO_3_ID:
            case HERO_4_ID:
            case HERO_MOVEMENT_0_ID:
            case HERO_MOVEMENT_1_ID:
            case HERO_MOVEMENT_2_ID:
            case HERO_MOVEMENT_3_ID:
            case HERO_MOVEMENT_4_ID:
            case HERO_MANA_0_ID:
            case HERO_MANA_1_ID:
            case HERO_MANA_2_ID:
            case HERO_MANA_3_ID:
            case HERO_MANA_4_ID:
hero_rollover: {
                int heroID;
                if (hoverID >= HERO_0_ID && hoverID <= HERO_4_ID)
                    heroID = player->m_heroes[
                        m_topHero + hoverID - HERO_0_ID];
                else if (hoverID >= HERO_MOVEMENT_0_ID
                         && hoverID <= HERO_MOVEMENT_4_ID)
                    heroID = player->m_heroes[
                        m_topHero + hoverID - HERO_MOVEMENT_0_ID];
                else if (hoverID >= HERO_MANA_0_ID
                         && hoverID <= HERO_MANA_4_ID)
                    heroID = player->m_heroes[
                        m_topHero + hoverID - HERO_MANA_0_ID];
                else
                    heroID = player->m_heroes[
                        m_topHero + hoverID - HERO_LOCATOR_0_ID];

                if (heroID == -1)
                    break;

                hero* mapHero = g_game->getHero(heroID);
                // Dreamcast adventuremapwindow.cpp:733 names operator[].
                sprintf(g_text,
                    (*g_generalText)[GENERAL_TEXT_HERO_ROLLOVER_FORMAT],
                    mapHero->m_name, mapHero->getClassName());
                rolloverText = g_text;
                break;
            }

town_rollover: {
                int townID = player->m_townIds[
                    m_topTown + hoverID - TOWN_0_ID];
                if (townID == -1)
                    break;

                const town* mapTown = g_game->getTown(townID);
                // 0x63a608 is basic_string::_Nullstr's byte: c_str().
                const char* townName = mapTown->m_name.c_str();
                sprintf(g_text, DATA_COMPGEN(
                    0x0065f3d4, adventureTownRolloverFormat, "%s, %s"),
                    townName, mapTown->getTypeName());
                rolloverText = g_text;
                break;
            }

            default:
                goto generic_help;
            }
        } else if (hoverID < HELP_WIDGET_1001_ID
                   || hoverID > HELP_WIDGET_1015_ID) {
generic_help:
            int helpID = convertID2HelpID(hoverID);
            if (helpID >= 0)
                rolloverText = g_adventureWindowHelp[helpID].m_text;
        }

        // Mac 0x298c..0x29a0 forwards the text pointer as the integer payload;
        // broadcastMessage owns the actual message object.
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_TEXT,
            ROLLOVER_TEXT_ID, reinterpret_cast<int>(rolloverText));
        drawWindow(0, ROLLOVER_TEXT_ID, ROLLOVER_TEXT_ID);
        g_windowManager->updateScreen(m_x + m_rolloverTextWidget->m_x,
            m_y + m_rolloverTextWidget->m_y, m_rolloverTextWidget->m_width,
            m_rolloverTextWidget->m_height);
    }
    return true;
}

VA(0x00403220, 0x59)
DC_ADDRESS(0x0010dc, 0x4)
MAC_ADDRESS(0x002a1c, 0x94)
void TAdventureMapWindow::doHeroKnob(bool up)
{
    playerData* player = g_game->getLocalPlayer();
    if (up) {
        if (m_topHero > 0)
            m_topHero--;
    } else {
        if (m_topHero < player->m_numHeroes - NUM_HERO_BUTTONS)
            m_topHero++;
    }
    updateHeroLocators(-1, 1, 1);
}

VA(0x00403280, 0x59)
DC_ADDRESS(0x0010e0, 0x4)
MAC_ADDRESS(0x002ab0, 0x94)
void TAdventureMapWindow::doTownKnob(bool up)
{
    playerData* player = g_game->getLocalPlayer();
    if (up) {
        if (m_topTown > 0)
            m_topTown--;
    } else {
        if (m_topTown < player->m_numTowns - NUM_TOWN_BUTTONS)
            m_topTown++;
    }
    updateTownLocators(-1, 1, 1);
}

void TAdventureMapWindow::drawTownLocatorHighlight(int which, unsigned char update)
{
    broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_ICON_FRAME,
                     TOWN_0_ID + which, 1);
    drawWindow(update, TOWN_0_ID + which, TOWN_0_ID + which);
}

VA(0x004032e0, 0x134)
DC_ADDRESS(0x0010e4, 0x4)
MAC_ADDRESS(0x002b44, 0x204)
void TAdventureMapWindow::updateHeroLocators(int top, bool drawWin,
                                             bool update)
{
    playerData* player = g_game->getLocalPlayer();
    if (!player->isHuman())
        return;

    if (top >= 0 && (top < m_topHero || top >= m_topHero + NUM_HERO_BUTTONS)) {
        if (top > player->m_numHeroes - NUM_HERO_BUTTONS)
            top = player->m_numHeroes - NUM_HERO_BUTTONS;
        if (top < 0)
            top = 0;
        m_topHero = top;
    }

    int i;
    for (i = 0; i < NUM_HERO_BUTTONS; i++)
        updateHeroLocator(i, 0, 0);

    if (!m_topHero)
        widgetSetStatus(HERO_UP_ID, widget::WIDGET_DIMMED);
    else
        widgetClearStatus(HERO_UP_ID, widget::WIDGET_DIMMED);

    if (player->m_numHeroes <= m_topHero + NUM_HERO_BUTTONS)
        widgetSetStatus(HERO_DOWN_ID, widget::WIDGET_DIMMED);
    else
        widgetClearStatus(HERO_DOWN_ID, widget::WIDGET_DIMMED);

    if (drawWin) {
        drawWindow(0, 0xffff0001, 0xffff);

        for (i = 0; i < NUM_HERO_BUTTONS; i++) {
            int heroId = player->m_heroes[m_topHero + i];
            if (heroId != -1 && !g_completeDrawAllCells
                && heroId == player->m_currHeroId) {
                m_heroLocators[i]->setVisible(1);
                m_heroLocators[i]->setImage("hpsyyy.pcx");
                m_heroLocators[i]->draw();
                break;
            }
        }
    }

    if (update)
        g_windowManager->updateScreen();
}

VA(0x00403420, 0x131)
DC_ADDRESS(0x0010e8, 0x4)
MAC_ADDRESS(0x002d48, 0x208)
void TAdventureMapWindow::updateTownLocators(int top, bool drawWin,
                                             bool update)
{
    playerData* player = g_game->getLocalPlayer();
    if (!player->isHuman())
        return;

    if (top >= 0 && (top < m_topTown || top >= m_topTown + NUM_TOWN_BUTTONS)) {
        if (top > player->m_numTowns - NUM_TOWN_BUTTONS)
            top = player->m_numTowns - NUM_TOWN_BUTTONS;
        if (top < 0)
            top = 0;
        m_topTown = top;
    }

    int i;
    for (i = 0; i < NUM_TOWN_BUTTONS; i++)
        updateTownLocator(i, 0, 0);

    if (!m_topTown)
        widgetSetStatus(TOWN_UP_ID, widget::WIDGET_DIMMED);
    else
        widgetClearStatus(TOWN_UP_ID, widget::WIDGET_DIMMED);

    if (player->m_numTowns <= m_topTown + NUM_TOWN_BUTTONS)
        widgetSetStatus(TOWN_DOWN_ID, widget::WIDGET_DIMMED);
    else
        widgetClearStatus(TOWN_DOWN_ID, widget::WIDGET_DIMMED);

    if (drawWin) {
        drawWindow(0, 0xffff0001, 0xffff);

        for (i = 0; i < NUM_TOWN_BUTTONS; i++) {
            int townId = player->m_townIds[m_topTown + i];
            if (townId != -1 && !g_completeDrawAllCells
                && townId == player->m_currTownId) {
                broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_ICON_FRAME,
                                 TOWN_0_ID + i, 1);
                drawWindow(0, TOWN_0_ID + i, TOWN_0_ID + i);
                break;
            }
        }
    }

    if (update)
        g_windowManager->updateScreen();
}

// Pass the visibility predicate directly to setVisible. An inferred int
// local widens retail's byte test even though the predicate is Boolean.
VA(0x00403560, 0x23E)
DC_ADDRESS(0x0010ec, 0x4)
MAC_ADDRESS(0x002f50, 0x3a4)
void TAdventureMapWindow::updateHeroLocator(int which, bool drawWinSect,
                                            bool update)
{
    playerData* player = g_game->getLocalPlayer();

    if (which < 0) {
        if (player->m_currHeroId != -1) {
            for (int slot = 0; slot < NUM_HERO_BUTTONS; slot++) {
                if (player->m_currHeroId == player->m_heroes[m_topHero + slot]) {
                    which = slot;
                    break;
                }
            }
        }
        if (which < 0)
            return;
    }

    int heroId = player->m_heroes[m_topHero + which];
    if (heroId != -1 && !g_completeDrawAllCells) {
        hero* thisHero = g_game->getHero(heroId);
        widgetSetStatus(HERO_0_ID + which, widget::WIDGET_ACTIVE);
        m_heroPortraits[which]->setImage(
            g_heroTraits[thisHero->m_portrait].m_smallPortraitName);
        widgetSetStatus(HERO_MOVEMENT_0_ID + which, widget::WIDGET_ACTIVE);
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_ICON_FRAME,
            HERO_MOVEMENT_0_ID + which, thisHero->getMobilityFrame());
        widgetSetStatus(HERO_MANA_0_ID + which, widget::WIDGET_ACTIVE);
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_ICON_FRAME,
            HERO_MANA_0_ID + which, thisHero->getManaFrame());
    } else {
        m_heroPortraits[which]->setImage("hpsxxx.pcx");
        widgetClearStatus(HERO_0_ID + which, widget::WIDGET_ACTIVE);
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_ICON_FRAME,
            HERO_MOVEMENT_0_ID + which, 0);
        widgetClearStatus(HERO_MOVEMENT_0_ID + which, widget::WIDGET_ACTIVE);
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_ICON_FRAME,
            HERO_MANA_0_ID + which, 0);
        widgetClearStatus(HERO_MANA_0_ID + which, widget::WIDGET_ACTIVE);
    }

    m_heroLocators[which]->setVisible(
        heroId != -1 && !g_completeDrawAllCells
        && heroId == player->m_currHeroId);

    if (drawWinSect) {
        drawWindow(0, HERO_0_ID + which, HERO_0_ID + which);
        drawWindow(0, HERO_MOVEMENT_0_ID + which, HERO_MOVEMENT_0_ID + which);
        drawWindow(0, HERO_MANA_0_ID + which, HERO_MANA_0_ID + which);
        if (heroId != -1 && !g_completeDrawAllCells
            && heroId == player->m_currHeroId) {
            m_heroLocators[which]->setImage("hpsyyy.pcx");
            m_heroLocators[which]->setVisible(1);
            m_heroLocators[which]->draw();
        }
        if (update)
            g_windowManager->updateScreen(0x261, 32 * which + 0xd4, 0x30, 0x20);
    }
}

VA(0x004037a0, 0x117)
DC_ADDRESS(0x0010f0, 0x4)
MAC_ADDRESS(0x0032f4, 0x1ac)
void TAdventureMapWindow::updateTownLocator(int which, bool drawWinSect,
                                            bool update)
{
    playerData* player = g_game->getLocalPlayer();
    int townId = player->m_townIds[m_topTown + which];

    if (which < player->m_numTowns && !g_completeDrawAllCells) {
        widgetSetStatus(TOWN_0_ID + which, widget::WIDGET_ACTIVE);
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_ICON_FRAME,
            TOWN_0_ID + which, g_game->getTown(townId)->getPortraitFrame(1));
    } else {
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_ICON_FRAME,
            TOWN_0_ID + which, 0);
        widgetClearStatus(TOWN_0_ID + which, widget::WIDGET_ACTIVE);
    }

    if (drawWinSect) {
        drawWindow(0, TOWN_0_ID + which, TOWN_0_ID + which);
        if (which < player->m_numTowns && !g_completeDrawAllCells
            && townId == player->m_currTownId) {
            drawTownLocatorHighlight(which, 0);
        }
        if (update)
            g_windowManager->updateScreen(0x2eb, 32 * which + 0xd4, 0x30, 0x20);
    }
}

VA(0x004038c0, 0xEC)
DC_ADDRESS(0x0010f4, 0x40)
MAC_ADDRESS(0x0034a0, 0x174)
void TAdventureMapWindow::highlightLocators(bool update)
{
    playerData* player = g_game->getLocalPlayer();

    int i;
    for (i = 0; i < NUM_TOWN_BUTTONS; i++) {
        int townId = player->m_townIds[m_topTown + i];
        if (townId != -1 && !g_completeDrawAllCells
            && townId == player->m_currTownId) {
            broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_ICON_FRAME,
                             TOWN_0_ID + i, 1);
            drawWindow(update, TOWN_0_ID + i, TOWN_0_ID + i);
            break;
        }
    }

    for (i = 0; i < NUM_HERO_BUTTONS; i++)
        m_heroLocators[i]->setVisible(0);

    for (i = 0; i < NUM_HERO_BUTTONS; i++) {
        int heroId = player->m_heroes[m_topHero + i];
        if (heroId != -1 && !g_completeDrawAllCells
            && heroId == player->m_currHeroId) {
            m_heroLocators[i]->setVisible(1);
            m_heroLocators[i]->setImage("hpsyyy.pcx");
            m_heroLocators[i]->draw();
            break;
        }
    }
}

VA(0x004039b0, 0x1EA)
DC_ADDRESS(0x001134, 0x4)
MAC_ADDRESS(0x003614, 0x2b0)  // ; MAC_ABSTRACTION_FROM(tokens1:283a696f3572,30.7692): unchanged body; CodeWarrior collateral of the DC EGameResource return restored on ExtraInfoUnion::getCampfireResource (mapcell.h)
void TAdventureMapWindow::updateQuestLogButton(bool update)
{
    unsigned char enabled = 0;
    int player = g_game->getLocalPlayerGamePos();

    if (g_game->m_players[player].isLocalHuman()) {
        unsigned i;
        for (i = 0; i < g_game->m_worldMap.m_seerHutList.size(); i++) {
            TSeerHut& hut = g_game->m_worldMap.m_seerHutList[i];
            if (hut.questActiveforPlayer(player)) {
                enabled = 1;
                break;
            }
        }

        for (i = 0; i < g_game->m_worldMap.m_questGuardList.size(); i++) {
            TQuestGuard& guard = g_game->m_worldMap.m_questGuardList[i];
            if (guard.questActiveforPlayer(player)) {
                enabled = 1;
                break;
            }
        }
    }

    widget* questButton = getWidget(QUEST_LOG_ID);
    if (questButton) {
        questButton->enable(enabled);
        questButton->draw();
        if (update)
            g_windowManager->updateScreen(questButton->m_x, questButton->m_y,
                questButton->m_width, questButton->m_height);
    }
}

VA(0x00403ba0, 0x47)
DC_ADDRESS(0x001138, 0x4)
MAC_ADDRESS(0x0038c4, 0x80)
void TAdventureMapWindow::updateSleepButton(const hero* thisHero)
{
    unsigned char enabled = 0;
    if (thisHero)
        enabled = 1;
    if (!g_currentPlayer->isLocalHuman())
        enabled = 0;

    broadcastMessage(MESSAGE_WIDGET,
        enabled ? widget::WIDGET_CLEAR_STATUS : widget::WIDGET_SET_STATUS,
        SLEEP_ID, widget::WIDGET_UPDATE | widget::WIDGET_DIMMED);
}

VA(0x00403bf0, 0x4C)
DC_ADDRESS(0x00113c, 0x4c)
MAC_ADDRESS(0x003944, 0x94)
void TAdventureMapWindow::updateSpellButton(const hero* thisHero)
{
    unsigned char enabled = 0;
    if (g_currentPlayer->isLocalHuman() && thisHero) {
        if (thisHero->m_owner < 0)
            return;
        enabled = 1;
    }

    broadcastMessage(MESSAGE_WIDGET,
        enabled ? widget::WIDGET_CLEAR_STATUS : widget::WIDGET_SET_STATUS,
        CAST_SPELL_ID, widget::WIDGET_UPDATE | widget::WIDGET_DIMMED);
}

// The elevation toggle's two .data objects, in retail's own order: the
// icon names indexed by map level, and the last level the toggle was set
// to (initialised to -1, which is why it lands in .data rather than
// .bss). Bounded exactly - 0x65f22c + 2*4 lands on the level cell.
DATA(0x0065f22c)
static const char* g_aszElevationIcons[2] = { "iam010.def", "iam003.def" };

// The sleep button swaps both its DEF and its active keyboard command.
// The two scancodes are the only contents of this retail .rdata row.
DATA(0x0063a570)
static const int g_aiSleepHotkeys[2] = { 44, 17 };

DATA(0x0065f238)
static const char* g_aszSleepIcons[2] = { "iam005.def", "iam011.def" };

// Project-inferred shared button refresh. Resolve the local player after the
// image message, as both callers did; keep all four widget dispatch stages.
void TAdventureMapWindow::showButtonImage(int id, const char* image)
{
    broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_ICON_NAME,
                     id, reinterpret_cast<int>(image));
    broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_PLAYER_PALETTE_COLORS,
                     id, g_game->getLocalPlayerGamePos());
    broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_DRAW, id, 0);
    widgetSetStatus(id, widget::WIDGET_UPDATE);
}

VA(0x00403c40, 0x78)
DC_ADDRESS(0x001188, 0x4)
MAC_ADDRESS(0x0039d8, 0xcc)
bool TAdventureMapWindow::setElevationToggleImage(int level)
{
    // Mac 0x39ec..0x3a04 initializes a function-local cache; 0x3a2c passes
    // the icon pointer directly to the four-int broadcast overload.
    DATA(0x0065f234) static int previousLevel = -1;
    if (level != previousLevel) {
        previousLevel = level;
        showButtonImage(ELEVATION_TOGGLE_ID, g_aszElevationIcons[level]);
        return true;
    }
    return false;
}

// The older TAdvMenu::SetSleepImage at dc 0x2a74 calls clear_hotkeys and
// set_hotkey on consecutive button.h source lines. Preserve both wrappers.
// Mac 0x3abc..0x3ad4 lazily initializes the function-local image cache to -1;
// VC6 constant-initializes the same retail data cell without a runtime guard.
// Mac 0x3aec..0x3b04 passes the icon pointer directly to the four-int broadcast
// overload, whose body owns message construction. A redundant caller message
// consumed VC6 inline budget and prevented the third vector::size expansion.
VA(0x00403cc0, 0x215)
DC_ADDRESS(0x00118c, 0x4)
MAC_ADDRESS(0x003aa4, 0xf4)  // anchor-global
void TAdventureMapWindow::setSleepImage(int image)
{
    DATA(0x0065f240) static int previousImage = -1;
    if (image != previousImage) {
        previousImage = image;
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_ICON_NAME,
            SLEEP_ID, reinterpret_cast<int>(g_aszSleepIcons[image]));
        broadcastMessage(MESSAGE_WIDGET,
            widget::WIDGET_SET_PLAYER_PALETTE_COLORS, SLEEP_ID,
            g_game->getLocalPlayerGamePos());
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_DRAW, SLEEP_ID, 0);
        widgetSetStatus(SLEEP_ID, widget::WIDGET_UPDATE);

        button* sleepButton = static_cast<button*>(getWidget(SLEEP_ID));
        sleepButton->clearHotkeys();
        sleepButton->setHotkey(g_aiSleepHotkeys[image]);
    }
}

VA(0x00403ee0, 0x1F)
DC_ADDRESS(0x001190, 0x2c)
MAC_ADDRESS(0x003b98, 0x54)
void TAdventureMapWindow::clearBottomView()
{
    if (m_bottomView) {
        delete m_bottomView;
        m_bottomView = 0;
    }
}

// E:\gamedcs\adventuremapwindow.cpp:1253
// DC public ?UpdateResourceDisplay@TAdventureMapWindow@@QAAX_N0@Z
// proves native bools despite the lowered T_UCHAR formal records. Both this
// forwarding interface and TResourceDisplay::update must preserve that ABI:
// a byte-to-bool boundary inserts two setne conversions absent at 0x403f00.
// Retail has the !draw reset and unconditional two-argument Update only.
// DC1257's update guard and DC1261's UpdateScreen call are port-specific;
// the 30-byte retail body has neither extra branch nor screen update.
VA(0x00403f00, 0x1E)
DC_ADDRESS(0x0011bc, 0x38)
MAC_ADDRESS(0x003bec, 0x30)  // anchor-global
void TAdventureMapWindow::updateResourceDisplay(bool draw, bool update)
{
    if (!draw)
        update = 0;
    m_resourceDisplay->update(draw, update);
}

VA(0x00403f20, 0x3F)
DC_ADDRESS(0x0011f4, 0x44)
MAC_ADDRESS(0x003c1c, 0x78)
void TAdventureMapWindow::drawChatText(bool update)
{
    drawWindow(0, CHAT_TEXT_ID, CHAT_TEXT_ID);
    if (update)
        g_windowManager->updateScreen(m_chatTextWidget->m_x, m_chatTextWidget->m_y,
            m_chatTextWidget->m_width, m_chatTextWidget->m_height);
}

// Original: TAdvMenu::SetAdvWinButtonPalette; adventuremapwindow.cpp:1273
// Complete owns these menu buttons directly in TAdventureMapWindow; its
// updateButtons body at 0x403f60 expands GetWidget and the button palette call.
// Mac 0:0x3c94 receives id in r3 and player in r4, then loads the window
// through g_advManager; no instance pointer is passed.
DC_ADDRESS(0x001238, 0x22)
MAC_ADDRESS(0x003c94, 0x4c)
void TAdventureMapWindow::setAdvWinButtonPalette(int id, int player)
{
    widget* w = g_advManager->m_advWindow->getWidget(id);
    if (w)
        static_cast<button*>(w)->setPlayerPaletteColors(player);
}

VA(0x00403f60, 0x144)
DC_ADDRESS(0x00125c, 0x28)
MAC_ADDRESS(0x003ce0, 0xfc)
void TAdventureMapWindow::updateButtons(bool draw, bool update)
{
    int player = g_game->getLocalPlayerGamePos();

    setAdvWinButtonPalette(KINGDOM_OVERVIEW_ID, player);
    setAdvWinButtonPalette(ELEVATION_TOGGLE_ID, player);
    setAdvWinButtonPalette(QUEST_LOG_ID, player);
    setAdvWinButtonPalette(SLEEP_ID, player);
    setAdvWinButtonPalette(MOVE_ID, player);
    setAdvWinButtonPalette(CAST_SPELL_ID, player);
    setAdvWinButtonPalette(ADVENTURE_OPTIONS_ID, player);
    setAdvWinButtonPalette(SYSTEM_OPTIONS_ID, player);
    setAdvWinButtonPalette(NEXT_HERO_ID, player);
    setAdvWinButtonPalette(END_TURN_ID, player);

    if (draw)
        drawWindow(update, KINGDOM_OVERVIEW_ID, END_TURN_ID);
}

#if 0  // @carcass

// Platform difference: DC puts a second copy of the adventure controls in
// TAdvMenu, a modal CAdvPopup (ctor0x1284, handler0x1ab8, dtor0x1a40).
// Complete constructs the buttons directly in TAdventureMapWindow0x401510
// and updates them through0x403220..0x403f60; advManager owns dispatch.
// These additional modal methods are accounted individually in dc_only.tsv.
// Their operations survive in the persistent window; they are not missing
// standalone retail claims. SetAdvWinButtonPalette above has its own proven
// source-identity bridge and is deliberately retained as a canonical helper.

// E:\gamedcs\button.h:104
void button::setHotkey(int code)
{
    // @stub
}

#endif  // @carcass

VA(0x004040b0, 0x38)
MAC_ADDRESS(0x003ddc, 0x20)
void TAdventureMapWindow::onSleepChange(unsigned char on)
{
    // Mac 0:0x3ddc forwards only; the mouse-effect edge is Windows-specific.
    heroWindow::onSleepChange(on);

    if (on) {
        if (m_immersion)
            static_cast<TImmMouseEffect*>(m_immersion)->stop();
    } else {
        if (m_immersion)
            static_cast<TImmMouseEffect*>(m_immersion)->start();
    }
}

// UNCLAIMED IN SPAN, RESOLVED 2026-09-06 (claim lane 31): 0x404690 is NOT a
// basic_string destructor - it stores logic_error's vtable 0x6455bc and
// tail-calls ~exception, so it is `??1logic_error`, and it is claimed in
// objecttype.cpp with the rest of that COMDAT group (0x404640 / 0x404660 /
// 0x4046e0). events' 0x4ad0e0 is still open.

// COMDAT pairing: basic_string<char>::_Eos, 20 B against this compiland's
// single 20-byte COMDAT.
#if 0  // @carcass: Dinkumware instantiations emitted by this compiland

VA(0x00404a70, 0x14)  // COMDAT pairing (unique 20 B in this obj)
void std::basic_string<char>::_Eos(unsigned n)
{
    // @stub
}

#endif  // @carcass

// COMDAT pairing: _Grow on the char instantiation, mnemonic agreement 0.971.
VA_COMPGEN(0x00404a90, 0x122, BASIC_STRING_GROW, char)

VA_COMPGEN(0x00404BD0, 0x108, BASIC_STRING_COPY, char)

// COMDAT pairing: std::_Construct<widget*>, 9 B against this compiland's
// single 9-byte COMDAT. Declarator form: the authority keys the free
// template flat (std__construct) with no owner arm.
#if 0  // @carcass: Dinkumware instantiation emitted by this compiland

VA(0x00404dc0, 0x9)  // COMDAT pairing (unique 9 B in this obj)
void std::_Construct(widget** slot, widget* const& value)
{
    // @stub
}

#endif  // @carcass
