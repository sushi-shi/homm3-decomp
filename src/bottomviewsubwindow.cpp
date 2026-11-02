#include "va.h"

#include <crt_stdio.h>
#include <strstream>

#include "bottomviewsubwindow.h"

#include "border.h"
#include "game.h"
#include "iconwdgt.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "resourcemanager.h"
#include "soundmgr.h"
#include "terrain.h"
#include "textresource.h"
#include "textwdgt.h"
#include "widget.h"
#include "window.h"

// Dreamcast bottomviewsubwindow.cpp:123/662 names GameTime::ElapsedSince;
// :676-681 names widget::set_visible; the NewTurn, Kingdom, and EnemyTurn
// constructors name TTextResource::operator[].

// THE LOCAL NAMES BELOW ARE DREAMCAST CODEVIEW, NOT INVENTION. The DC
// build's S_REGREL32 records for this compiland identify examples and
// give their types: TBottomViewKingdom's `town_count` (LF_ARRAY of
// T_INT4, length 16 - so `int[4]`, which also confirms the array's
// width) and `text` (std::string); TBottomViewTown's `town_size_name`
// (std::string); TBottomViewResourceMessage's parameter `res` and its
// `char str[20]`; TBottomViewHero's record also names the std::string
// `quantity_text` in the army loop. Other optimized locals can be absent.

// THE INCLUDE SET IS NOT THE WALL IN THIS TU: file-scope declaration probes
// (extern int, tags, structs, declarators; counts up to 256, before or after
// the includes) and include-list mutations all reach the front end
// (`homm3 vc6 il-diff`) but leave the constructors' object code unchanged.
// See TBottomViewKingdom for what the wall actually is.
VA_COMPGEN(0x00450d20, 0x21, SCALAR_DELETING_DTOR, type_bottom_view_window)

VA(0x00450d50, 0x78)
DC_ADDRESS(0x055114, 0x74)
MAC_ADDRESS(0x05f1f0, 0xcc)
type_bottom_view_window::~type_bottom_view_window()
{
    for (std::vector<widget*>::iterator it = m_widgets.begin();
         it != m_widgets.end(); ++it) {
        widget* item = *it;
        if (item) {
            m_parentWindow->removeWidget(item);
            delete item;
        }
    }
}

// The seven derived destructors have EMPTY bodies - no derived class owns
// storage that needs releasing - so all seven are the base destructor
// inlined (its widget-removal loop and its ??1TSubWindow call).

// Each pair follows its class's constructor in source order (the Dreamcast
// line order 77, 146, 197, 222, 352, 515, 599); each derived table's only
// reference is in its constructor, whose caller is the matching
// advManager::UpdBottomViewXxx.

// Slot 1 corroborates the two classes DC gives an animate override to:
// 0x63bb0c and 0x63bb44 point at their own bodies (0x4511a0, 0x4536f0)
// while the other five share the base's 0x5bc690.

// 0x4521f0 is TBottomViewTown's constructor, not type_bottom_view_window's.

// The base constructor, inlined in all seven derived constructors and
// otherwise unreferenced: TSubWindow's default constructor, then this
// class's vptr, then the one body statement.
DC_ADDRESS(0x0550b8, 0x5c)
MAC_ADDRESS(0x05f188, 0x68)
type_bottom_view_window::type_bottom_view_window(heroWindow* parentWindow)
{
    initialize(614, 400, 176, 166, parentWindow);
}

// The new-turn banner, and the first of the seven big derived
// constructors. Every literal is read off the body:

//   * the base is the same type_bottom_view_window(parent) inline as
//     TBottomViewMessage's - TSubWindow's default ctor, the 0x63bb04 vptr,
//     initialize(614, 400, 176, 166, parent) - followed here by this
//     class's own 0x63bb0c store;
//   * the backdrop is 'AdStatOt.pcx' at the full 176x166 with style 0x800,
//     exactly as in TBottomViewMessage, and there is NO reserve() ahead of
//     the three push_backs;
//   * the calendar branch reads gpGame's three date words. THEN is taken
//     when the day is 1 and the (week, month) pair is not (1, 1) - i.e.
//     a week rolled over and it is not the game's opening week - and it
//     announces the week: NewWeekN.def from the five-slot table and
//     general text 64. Otherwise it announces the day with general text 65
//     and plays newday.wav for 30 s on channel 3;
//   * the icon is 175x166 (one pixel narrower than the backdrop), the text
//     is 'medfont.fnt' in font::WHITE at 10,10,148,146, frameDelay is 100
//     and lastStepTime is stamped at the very end.

// Original: type_bottom_view_window::animate; bottomviewsubwindow.cpp:64.
// The empty base hook is shared (folded) with five presentation-only
// subclasses.
DC_ADDRESS(0x055188, 0x4)
void type_bottom_view_window::animate()
{
}

VA(0x00450dd0, 0x319)
DC_ADDRESS(0x05518c, 0x2bc)
MAC_ADDRESS(0x05f2c0, 0x400)
TBottomViewNewTurn::TBottomViewNewTurn(heroWindow* parent)
    : type_bottom_view_window(parent)
{
    int id = BOTTOM_VIEW_BACKGROUND_ID;

    m_widgets.push_back(new bitmapBorder(
        0, 0, 176, 166, id++, "AdStatOt.pcx", 0x800));

    m_frame = 0;

    std::string text;
    // DC names: local static char*[4] (type 0x581a, 16 bytes).
    DATA(0x00660ba0)
    static char* names[4] = {
        "NewWeek1.def", "NewWeek2.def", "NewWeek3.def", "NewWeek4.def"
    };
    const char* iconName;
    if (g_game->m_day == 1
        && !(g_game->m_week == 1 && g_game->m_month == 1)) {
        iconName = names[static_cast<unsigned short>(g_game->m_week) - 1];
        text = formatString("%s %d", (*g_generalText)[GENERAL_TEXT_CALENDAR_WEEK],
            static_cast<unsigned short>(g_game->m_week));
    } else {
        iconName = "NewDay.def";
        text = formatString("%s %d", (*g_generalText)[GENERAL_TEXT_CALENDAR_DAY],
            static_cast<unsigned short>(g_game->m_day));
        launchSample("newday.wav", 30000, 3);
    }

    m_icon = new iconWidget(0, 0, 175, 166, id++, iconName, 0, 0, 0, 0, 0x10);
    m_icon->setIconFrame(0);
    m_widgets.push_back(m_icon);

    m_frameDelay = 100;
    m_backdrop = new textWidget(10, 10, 148, 146, text.c_str(), "medfont.fnt",
        font::WHITE, id++, 1, 0, 8);
    m_widgets.push_back(m_backdrop);

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }

    m_lastStepTime = GameTime::get();
}

VA_COMPGEN(0x004510f0, 0x21, SCALAR_DELETING_DTOR, TBottomViewNewTurn)

VA(0x00451120, 0x78)
DC_ADDRESS(0x056ea4, 0x18)
MAC_ADDRESS(0x062898, 0x60)
TBottomViewNewTurn::~TBottomViewNewTurn()
{
}

VA(0x004511a0, 0x79)
DC_ADDRESS(0x055448, 0x64)
MAC_ADDRESS(0x05f6c0, 0xe4)
void TBottomViewNewTurn::animate()
{
    if (m_frame == m_icon->m_sprite->GetNumFrames(0) - 1)
        return;

    unsigned long lastStep = m_lastStepTime;
    if (GameTime::elapsedSince(lastStep) < m_frameDelay)
        return;

    ++m_frame;
    m_icon->setIconFrame(m_frame);
    m_icon->draw();
    m_backdrop->draw();
    m_icon->forceUpdate();
    m_lastStepTime = GameTime::get();
}

// E:\gamedcs\bottomviewsubwindow.cpp:146
// The resource-award banner: TBottomViewMessage's text panel plus, when
// a resource is named, its icon and the awarded quantity centred under
// it. Read off the body:

//   * the same 'AdStatOt.pcx' backdrop and the same 'smalfont.fnt'
//     message widget as TBottomViewMessage, at 156x146 rather than
//     148x146, behind a reserve(5);
//   * a NEGATIVE resource means "no award": the whole icon-and-quantity
//     block, ostrstream included, sits inside that one guard;
//   * 'resour82.def' is fetched through ResourceManager::GetSprite and
//     the sprite's own Width/Height size the widget - the icon is
//     centred with `(width - Width) / 2` (retail's cdq/sar signed
//     halving) at y=50, and the quantity line sits at Height + 55;
//   * the quantity is measured before it is drawn: LineWidth of the
//     streamed text gives the widget's width and the font's `height`
//     byte its height, both read through the same global font pointer
//     in two separate loads. str() is therefore called TWICE, and both
//     expansions carry their own freeze(true);
//   * the sprite is released through ResourceManager::Dispose; its
//     inline wrapper owns the virtual CSprite::Dispose call.
// DC also records `char str[20]` inside the resource guard; it has no
// corresponding live storage in this optimized Complete body.
VA(0x00451220, 0x393)
DC_ADDRESS(0x0554ac, 0x2bc)
MAC_ADDRESS(0x05f7a4, 0x41c)  // anchor-vtable 0x63bb1c + advManager::UpdBottomViewResMsg
TBottomViewResourceMessage::TBottomViewResourceMessage(
    heroWindow* parent, int res, int quantity,
    const std::string* message)
    : type_bottom_view_window(parent)
{
    m_widgets.reserve(5);

    m_widgets.push_back(new bitmapBorder(0, 0, 176, 166,
        BOTTOM_VIEW_BACKGROUND_ID, "AdStatOt.pcx", 0x800));
    m_widgets.push_back(new textWidget(10, 10, 156, 146,
        message->c_str(), "smalfont.fnt", font::WHITE, BOTTOM_VIEW_TEXT_ID,
        1, 0, 8));

    if (res >= 0) {
        CSprite* sprite = ResourceManager::GetSprite("resour82.def");

        m_widgets.push_back(new iconWidget((m_width - sprite->GetWidth()) / 2, 50,
            sprite->GetWidth(), sprite->GetHeight(), 0x837, "resour82.def", res,
            0, 0, 0, 0x10));

        std::ostrstream quantityText;
        quantityText << quantity << std::ends;

        int textWidth = g_smallFont->LineWidth(quantityText.str());
        int fontHeight = g_smallFont->fs.height;

        m_widgets.push_back(new textWidget((m_width - textWidth) / 2,
            sprite->GetHeight() + 55, textWidth, fontHeight,
            quantityText.str(), "smalfont.fnt", font::PRIMARY, 0x836,
            1, 0, 8));

        quantityText.freeze(false);
        // DC bottomviewsubwindow.cpp:176 names the canonical Dispose wrapper.
        ResourceManager::Dispose(sprite);
    }

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }
}

VA_COMPGEN(0x00453DD0, 0x16B, OSTREAM_PUT, char)

VA_COMPGEN(0x00454450, 0x284, OSTREAM_INSERT_CSTR, char)

VA_COMPGEN(0x00454150, 0xA4, STREAMBUF_XSPUTN, char)

// The default ostrstream used above causes VC6 to retain this consecutive
// Dinkumware support band in bottomviewsubwindow.obj. The identities come
// from the base object's own public symbols; the retail bodies and vtable
// edges independently prove the same roles. They have no user-authored
// definitions on which a live annotation can sit, so ordinary members use
// claim-only carcass declarators and deleting wrappers use VA_COMPGEN.
#if 0  // @carcass -- compiler/library COMDATs emitted by the constructor

VA(0x004515c0, 0x10)
void ios_base::ios_base();

VA_COMPGEN(0x004515d0, 0x21, SCALAR_DELETING_DTOR, ios_base)

VA(0x00451600, 0xE5)
void strstreambuf::strstreambuf(int allocationSize);

VA_COMPGEN(0x004516f0, 0x21, SCALAR_DELETING_DTOR, strstreambuf)

VA_COMPGEN(0x00451720, 0x30, SCALAR_DELETING_DTOR, ostrstream)

VA(0x00451750, 0x14)
void ostrstream::`vbase destructor'();
#endif

VA_COMPGEN(0x00451770, 0x21, SCALAR_DELETING_DTOR, TBottomViewResourceMessage)

VA(0x004517a0, 0x78)
DC_ADDRESS(0x056ef0, 0x18)
MAC_ADDRESS(0x062838, 0x60)
TBottomViewResourceMessage::~TBottomViewResourceMessage()
{
}

VA(0x00451820, 0x1DC)
DC_ADDRESS(0x055768, 0x140)
MAC_ADDRESS(0x05fbc0, 0x214)
TBottomViewMessage::TBottomViewMessage(heroWindow* parent,
                                       const std::string* message)
    : type_bottom_view_window(parent)
{
    m_widgets.reserve(2);

    m_widgets.push_back(new bitmapBorder(
        0, 0, 176, 166, BOTTOM_VIEW_BACKGROUND_ID, "AdStatOt.pcx", 0x800));
    m_widgets.push_back(new textWidget(
        10, 10, 148, 146, message->c_str(), "smalfont.fnt", font::WHITE,
        BOTTOM_VIEW_TEXT_ID, 1, 0, 8));

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }
}

VA_COMPGEN(0x00451a00, 0x21, SCALAR_DELETING_DTOR, TBottomViewMessage)

VA(0x00451a30, 0x78)
DC_ADDRESS(0x056f3c, 0x18)
MAC_ADDRESS(0x0627d8, 0x60)
TBottomViewMessage::~TBottomViewMessage()
{
}

// The four primary-skill number boxes, one row across the portrait's
// right; the loop walks this table with an 8-byte step and tests its
// own cursor against &[2] for the "power and knowledge floor at 1"
// arm, which is what proves the row is a table and not four literals.
DATA(0x00660bb0)
static int g_heroStatCoords[kNumPrimarySkills][2] = {
    { 65, 51 }, { 92, 51 }, { 120, 51 }, { 148, 51 }
};

// The seven army-slot positions, three over four. Contiguous with the
// table above in retail .data, but a separate object: the stat loop
// stops at this table's first byte and the army loop starts there.
DATA(0x00660bd0)
static int g_heroArmyCoords[armyGroup::ARMY_GROUP_SLOT_COUNT][2] = {
    { 36, 73 }, { 72, 73 }, { 108, 73 },
    { 18, 121 }, { 54, 121 }, { 90, 121 }, { 126, 121 }
};

// E:\gamedcs\bottomviewsubwindow.cpp:222
// The hero banner. Everything below is read off the body:

//   * 'AdStatHr.pcx' at the full 176x166 with style 0x800, then the
//     acting player's current hero through game::GetHero - whose -1 arm
//     is spelled, not open-coded, so the null falls straight through
//     into the portrait load exactly as retail does;
//   * the portrait is akHeroTraits[hero->portrait].largePortraitName -
//     retail dereferences 0x67dce8 and indexes it 23 dwords per row,
//     which is the reference-to-array spelling this header already
//     carries - drawn at (3,2,58,64), and the hero's own name band at
//     +0x23 goes to a 'smalfont.fnt' widget at (66,2,107,17);
//   * the four primary skills are clamped in THREE arms, and the third
//     one is the interesting one: a skill at or below zero displays as
//     `i >= 2`, i.e. attack and defense floor at 0 while spell power and
//     knowledge floor at 1. Retail strength-reduces that comparison onto
//     the coordinate cursor (`cmp esi, &gHeroStatCoords[2]` / `setge`),
//     which is why the table and the arm are one loop;
//   * mana is a SHORT (`movsx eax, word [hero+0x18]`), morale and luck
//     are icon frames at GetMorale/GetLuck + 3, and both accessors take
//     the DC-attested three arguments;
//   * the army block is gated on a seven-slot count that this body
//     computes itself: DC 273..278 counts the slots with no GetNumArmies
//     jsr, while the same module's TBottomViewTown does call
//     armyGroup::GetNumArmies. Windows retail expands only this count
//     (mov ecx,7 / dec ecx); Mac 0x60448..0x604c8 retains no call either.

// THE WIDGET IDS ARE LITERALS HERE, NOT AN INCREMENTING LOCAL - the
// opposite of TBottomViewNewTurn. 0x7d8/0x7d9/0x7da are pushed as
// immediates and they are OUT OF CODE ORDER (mana 0x7da is built before
// morale 0x7d8), which no single incrementing local can produce; 0x7d7
// is never used at all. Only the army loop numbers its widgets from a
// running local, and there the `id++` sits inside each `new`'s non-null
// arm in the usual way.

// Residual (96.52%): the stats loop keeps `i` alive. Retail expresses
// every use of it through an induction variable - the widget id off the
// stats cursor (`0x7d3 - &stats[0]` plus the cursor), and BOTH the
// `i >= 2` arm and the loop exit off the coordinate cursor
// (`cmp esi, &gHeroStatCoords[2]` / `setge`, `cmp esi,
// &gHeroStatCoords[4]` / `jl`) - so no `i` is ever materialized. Our CL
// reduces the id the same way but reconstructs `i` as a second bias off
// the stats cursor for the other two. The `i >= 2` compare is SIGNED
// (`setge`), which rules out the obvious pointer-walk spelling: a real
// pointer comparison would be unsigned. A second, smaller residual is
// shared with TBottomViewTown - see the note there on game::GetHero /
// GetTown arm order.

// THE TWO STRING TEMPORARIES GET SEPARATE SLOTS. The >= 10000 arm and
// the plain arm each build their own format_string temporary at its own
// frame offset (-0x40 and -0x50) and tear it down inside the arm, so
// they are two statements in two arms, not one hoisted call.

// THE ACTING HERO COMES FROM game::GetCurrHero (96.52 -> 97.77,
// 2026-08-14). dc line 229 calls it by name; see the note on
// TBottomViewTown below for why that accessor is not GetHero applied to
// gpCurrentPlayer->currHeroId.
// 2026-09-05, easy lane 3 - first divergence localised. The stat sweep's
// `>= 2` test is an INDUCTION-VARIABLE selection: retail keeps the
// gHeroStatCoords row pointer live and tests it directly
// (`cmp esi, offset gHeroStatCoords+0x10 / setge al`), while this compile
// keeps the hero stat-block pointer and rebuilds the index as
// `(K - slot) + ptr` compared against 2, which costs the extra `mov eax,esi`
// copy at the bitset shift (retail clobbers its index register because the
// index is dead), a spilled difference in `[ebp-0x10]`, and the
// one-instruction loop header the preheader `jmp`s past. Two lockstep IVs,
// and VC6 picks the survivor itself; not a guard or return shape.
// The direct DC count restores 95.88 -> 97.76; the earlier getNumArmies
// call kept the helper's retained Windows body out of this expansion.
// The earlier in-class trace also left the quantity string's default
// constructor -> _Tidy(false) call (cost 152, child budget 141); those figures
// describe that prior context rather than the ordinary-body model.
// earlier differences include GetCurrHero arm order and skill-loop IV choice.
VA(0x00451ab0, 0x68A)
DC_ADDRESS(0x0558a8, 0x54c)
MAC_ADDRESS(0x05fdd4, 0x9b4)  // anchor-vtable 0x63bb2c + advManager::UpdBottomViewHero
TBottomViewHero::TBottomViewHero(heroWindow* parent)
    : type_bottom_view_window(parent)
{
    m_widgets.reserve(25);

    m_widgets.push_back(new bitmapBorder(0, 0, 176, 166,
        BOTTOM_VIEW_BACKGROUND_ID, "AdStatHr.pcx", 0x800));

    hero* who = g_game->getCurrHero();

    m_widgets.push_back(new bitmapBorder(3, 2, 58, 64, 0x7d1,
        akHeroTraits[who->m_portrait].m_large_portrait_name, 0x800));
    m_widgets.push_back(new textWidget(66, 2, 107, 17, who->m_name,
        "smalfont.fnt", font::WHITE, 0x7d2, 0, 0, 8));

    for (int i = 0; i < kNumPrimarySkills; i++) {
        int value = who->getPrimarySkill(i);
        sprintf(g_text, "%d", value);
        m_widgets.push_back(new textWidget(g_heroStatCoords[i][0],
            g_heroStatCoords[i][1], 23, 16, g_text, "smalfont.fnt",
            font::WHITE, 0x7d3 + i, 1, 0, 8));
    }

    sprintf(g_text, "%d", who->m_mana);
    m_widgets.push_back(new textWidget(145, 93, 27, 14, g_text, "tiny.fnt",
        font::WHITE, 0x7da, 1, 0, 8));

    m_widgets.push_back(new iconWidget(5, 74, 22, 12, 0x7d8, "imrl22.def",
        who->getMorale(0, 0, 1) + 3, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(5, 91, 22, 12, 0x7d9, "ilck22.def",
        who->getLuck(0, 0, 1) + 3, 0, 0, 0, 0x10));

    int numStacks = 0;
    for (int n = 0; n < armyGroup::ARMY_GROUP_SLOT_COUNT; n++) {
        if (who->m_army.m_armyTypes[n] != CREATURE_NONE)
            numStacks++;
    }

    if (numStacks > 0) {
        int id = 0x7db;
        for (int j = 0; j < armyGroup::ARMY_GROUP_SLOT_COUNT; j++) {
            TCreatureType type = who->m_army.m_armyTypes[j];
            if (type != CREATURE_NONE) {
                m_widgets.push_back(new iconWidget(g_heroArmyCoords[j][0],
                    g_heroArmyCoords[j][1], 32, 32, id++, "cprsmall.def",
                    type + 2, 0, 0, 0, 0x10));

                std::string text;
                if (who->m_army.m_numTroops[j] < 10000)
                    text = formatString("%d", who->m_army.m_numTroops[j]);
                else
                    text = formatString("%dk", who->m_army.m_numTroops[j] / 1000);

                m_widgets.push_back(new textWidget(g_heroArmyCoords[j][0],
                    g_heroArmyCoords[j][1] + 34, 32, 13, text.c_str(),
                    "tiny.fnt", font::WHITE, id++, 1, 0, 8));
            }
        }
    }

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }
}

VA_COMPGEN(0x00452140, 0x21, SCALAR_DELETING_DTOR, TBottomViewHero)

VA(0x00452170, 0x78)
DC_ADDRESS(0x056f88, 0x18)
MAC_ADDRESS(0x062778, 0x60)
TBottomViewHero::~TBottomViewHero()
{
}

// The town window's own seven army-slot positions. Byte-identical in
// CONTENT to gHeroArmyCoords above and immediately after it in retail
// .data, but a separate object: the hero loop's cursor stops at this
// table's first byte and this loop's cursor starts there.
DATA(0x00660c08)
static int g_townArmyCoords[armyGroup::ARMY_GROUP_SLOT_COUNT][2] = {
    { 36, 73 }, { 72, 73 }, { 108, 73 },
    { 18, 121 }, { 54, 121 }, { 90, 121 }, { 126, 121 }
};

// E:\gamedcs\bottomviewsubwindow.cpp:352
// The town banner, and the biggest of the seven. Read off the body:

//   * 'AdStatCs.pcx' at the full extent, then the acting player's
//     current town through game::GetTown - and retail compares the BYTE
//     `currTownId` against 0xff before widening it, which is the
//     accessor's own -1 arm applied to a char field;
//   * 'itpt.def' at (3,2,58,64) framed by town::GetPortraitFrame(false),
//     and the town's own cName beside it;
//   * hall and fort levels are two three-arm ladders over the SAME
//     `built` mask - hall against bitNumber[11..13] counting up from 0,
//     fort against bitNumber[7..9] counting up from 0 with 3 as the
//     no-fortification default. Each feeds an icon frame.

// THE TOWN-SIZE NAME IS A DEAD LOCAL. Retail default-constructs a
// std::string, assigns gTownSizeNames[hallLevel] into it through
// Dinkumware's inlined _Grow + `rep movsd` assign, and then never reads
// it - the only other reference is its destructor at the very end of
// the body. It is transcribed because it is there: the assign is ~40
// bytes of this function and the teardown is the last thing before the
// epilogue.

// The silo row's loop counter IS resource[0]. DC records only
// EGameResource resource[3]: line 406 stores the counter at r14+44, the
// array's own address, line 412 increments the count and 413 stores the
// counter through r14+44+4*count; the icons read resource[1]/[2]. Retail
// keeps the same frame shape - the counter homed at [ebp-0x2c] and
// reloaded after each store, the cursor starting at &resource[0] and
// pre-incrementing - exactly as TQuickTownWindow's sibling sweep. The
// separate `int i` + `slots[found++]` spelling kept the counter in a
// register and left the frame and reserve()'s nested _Ucopy inline
// decision wrong (94.77; this form 98.72). Dreamcast line 373 also calls
// HasBuilding(13, 0) directly, not IsCapitol; both emit the same bytes.
// Remaining: the hall ladder's three mask tests and the fort ladder's
// second pick the other commutative register form (retail copies the
// cached mask and loads the bit into ecx/esi, the exact TQuickTownWindow
// shape). Byte-flat probes: DC braces on both ladders, unbraced
// hasBuilding, swapped mask operands, `hallLevel = 0` split from its
// declaration, a const town receiver (labels only, 98.76); the sibling's
// `else hallLevel = 0` ladder is worse (98.23).

// PART OF IT IS THE SITE COUNT - see TBottomViewKingdom's note for the
// mechanism. Padding this body with free inline candidates gives 94.43
// at one site, 96.13 at three AND at five, and 91.66 at eight, so the
// deficit is around three. At three sites the silo sweep and the
// ostrstream str() region both come right - the block count goes 81 ->
// 82 exact - while the three residuals named above (the zero CSE, the
// GetTown arm order and the 8-byte frame) survive untouched, which is
// what separates them into two independent classes. Nothing is landed:
// the padding is a measurement, not a spelling.

// THE EH CLEANUP TRANSCRIPT LOCATES THAT DEFICIT EXACTLY (2026-08-14,
// docs/vc6/eh-cleanup.md). Retail's `[ebp-4]` states run
// ...,0xd,0xe,0xf,0x10,0x11,... and ours ...,0xd,0xe,0x10,0x11,...: retail
// opens ONE cleanup region we never open, and the funclet chain says where.
// Between the state-0xe call and the state-0x10 call retail does
//     lea ecx,[ebp-0xcc] / mov [ebp-4],0xf / mov [ebp-0x38],ecx
//     lea ecx,[ebp-0xcc] / call <no stack args>
// i.e. one more sub-object CONSTRUCTED on a frame local, with its address
// parked in a second slot - which is also why retail's frame is 4 bytes
// wider here (-0xd0/-0xcc against our -0xc8/-0xc4).

// NAMED, 2026-08-14: it is NOT a missing statement. Reading the calls
// either side of it, retail's state-0xe call is `basic_ostream(streambuf*,
// bool, bool)` taken OUT OF LINE on the whole object (this=[ebp-0xd0],
// args `&_Sb,0,1,0` - the trailing 0 is the virtual-base flag), state 0xf
// is `basic_streambuf::basic_streambuf()` and state 0x10 is
// `strstreambuf::_Init(0,0,0,0)`. That is retail INLINING
// `strstreambuf::strstreambuf(streamsize)` - a two-line header inline whose
// base-class ctor opens the extra region. We do the exact opposite: we
// inline `basic_ostream`'s ctor (vftable store + an out-of-line
// `basic_ios::init`) and CALL `??0strstreambuf@std@@QAE@H@Z`. Two swapped
// depth-2 decisions in one statement, so the statement is right and the
// nested budget is not: per docs/vc6/inliner.md the depth-2 budget is
// `budget / sites-remaining`, and we need it in [cb(strstreambuf ctor),
// cb(basic_ostream ctor)) where ours is above both. Two more free
// candidate sites at or after the declaration put it there (95.63 -> 97.45,
// flat at +3 and +4), and the proven-exact sibling
// TQuickTownWindow::initialize_army_display shows retail's OTHER side of
// the same knife-edge: there BOTH ctors are out of line, exactly as ours
// is. So `str()`/`freeze(false)` are not the lever - the caller's statement
// mass is. Open: which two candidate sites retail's body has after the
// declaration that ours does not.

// ALL SEVEN SITES ARE SOURCE-REAL: town::HasBuilding (restored
// 2026-08-30). dc spells the hall ladder (lines
// 369/371/373, `mov #11/#12/#13,r5 / mov #0,r6`), the fort ladder
// (382/384/386, r5 = 7/8/9, r6 = 0) and the silo gate (line 402, r5 =
// 15, r6 = 1) as calls where this body tested `built`/`active &
// bitNumber[...]` by hand; retail's 0x4305a0 proves the arms
// (check_included == 0 reads [ecx+0x150] = built, != 0 reads
// [ecx+0x158] = active) and the inlined expansion is byte-identical to
// the ladders, so once again only the site count moves.

// The old source refused the silo site on measurement alone. The three
// ladder subsets measured 97.3638 base / 96.8134 hall-only / 97.3638
// fort-only / 97.1557 silo-only / 98.7476 hall+fort / 96.0953
// hall+silo / 97.1960 fort+silo / 94.0054 all seven, so the DC's own
// seventh call costs 4.74 points on top of the two ladders. Six sites
// land on the +2 the probe wanted (98.7476 against the probe's 98.86);
// seven overshoot it, exactly as the free-probe curve said (flat at +3).
// That is compiler-state evidence, not a source contradiction: the retail
// expansion and direct mask are identical at this site, while Dreamcast
// positively proves the helper. Preserve 98.7476 as score history and keep
// the seventh call while the surrounding post-Dreamcast quantity-text shape
// is reconstructed. Keep the Dreamcast-attested helper despite this local
// maximum; retail remains the final semantic/codegen check.

// get_army() IS CALLED FRESH EVERY TIME, three times per army slot;
// retail never caches the reference. The quantity text is the same
// ostrstream idiom TQuickTownWindow::initialize_army_display already
// matches exactly, including the freeze(false) after the widget, and
// the coordinate cursor advances only for a NON-EMPTY slot.

// town_size_name IS COPY-INITIALIZED, not default-constructed and then
// assigned (94.31 -> 95.63, 2026-08-14). The EH transcript is what says
// so: retail's `mov [ebp-4],5` sits AFTER the whole strlen/_Grow/copy
// block, ours sat between _Tidy and it. One region that opens only when
// the ctor has fully run is one constructor call, not a ctor plus an
// operator=. The DC xref graph corroborates - this compiland reaches
// basic_string's CONSTRUCTOR (plus an allocator<char> temporary) and no
// assignment operator.
// DC get_army reads select mutable dc 0x168bd0; Mac 0x61174/0x611a0/
// 0x6127c/0x61294/0x612b4 all call mutable 0x1b6fdc, not const 0x1b7020.
// The retail twins fold at 0x5c1460, so its const label cannot establish a
// const receiver. Keep the native mutable GetCurrTown/getArmy path.
// VC6 mutable path: 94.7651% versus 94.7987% for the artificial const view;
// the five folded overload labels change, while CFG and instructions hold.
// The DC three-resource array restores four bytes of the frame and raises
// 94.8000 to 94.8201%; changing only its enum element type is byte-flat.
// Address-arithmetic review (2026-09-10): indexing army_pos by the packed
// display slot replaces its flattened int* walk and raises 94.0054 to 94.80%.
VA(0x004521f0, 0x8D4)
DC_ADDRESS(0x055df4, 0x5c4)
MAC_ADDRESS(0x060788, 0xcac)  // anchor-vtable 0x63bb34 + advManager::UpdBottomViewTown
TBottomViewTown::TBottomViewTown(heroWindow* parent)
    : type_bottom_view_window(parent)
{
    m_widgets.reserve(25);

    m_widgets.push_back(new bitmapBorder(0, 0, 176, 166,
        BOTTOM_VIEW_BACKGROUND_ID, "AdStatCs.pcx", 0x800));

    town* which = g_game->getCurrTown();

    m_widgets.push_back(new iconWidget(3, 2, 58, 64, 0x7d1, "itpt.def",
        which->getPortraitFrame(false), 0, 0, 0, 0x10));
    m_widgets.push_back(new textWidget(66, 1, 107, 16, which->m_name.c_str(),
        "smalfont.fnt", font::WHITE, 0x7d2, 0, 0, 8));

    int hallLevel = 0;
    if (which->hasBuilding(HALL_TOWN_ID, false)) {
        hallLevel = 1;
    } else if (which->hasBuilding(HALL_CITY_ID, false)) {
        hallLevel = 2;
    } else if (which->hasBuilding(HALL_CAPITOL_ID, false)) {
        hallLevel = 3;
    }

    std::string townSizeName = g_townSizeNames[hallLevel];

    m_widgets.push_back(new iconWidget(67, 31, 34, 34, 0x7d3, "itmtls.def",
        hallLevel, 0, 0, 0, 0x10));

    int fortLevel;
    if (which->hasBuilding(CASTLE_FORT_ID, false)) {
        fortLevel = 0;
    } else if (which->hasBuilding(CASTLE_CITADEL_ID, false)) {
        fortLevel = 1;
    } else if (which->hasBuilding(CASTLE_CASTLE_ID, false)) {
        fortLevel = 2;
    } else {
        fortLevel = 3;
    }

    m_widgets.push_back(new iconWidget(105, 31, 34, 34, 0x7d4, "itmcls.def",
        fortLevel, 0, 0, 0, 0x10));

    if (which->m_garrisonHeroId != -1)
        m_widgets.push_back(new bitmapBorder(149, 76, 22, 30, 0x7d8,
            "townqkgh.pcx", 0x800));

    if (which->hasBuilding(MARKETPLACE_SILO_ID, true)) {
        int* income = which->getSiloIncome();
        EGameResource resource[3];
        int found = 0;
        for (resource[0] = WOOD; resource[0] <= GOLD;
             resource[0] = static_cast<EGameResource>(resource[0] + 1) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */) {
            if (income[resource[0]] != 0) {
                ++found;
                resource[found] = resource[0];
            }
        }
        if (found == BOTTOM_VIEW_SILO_TWO_RESOURCES) {
            m_widgets.push_back(new iconWidget(6, 75, 20, 18, 0x7d7,
                "smalres.def", resource[1], 0, 0, 0, 0x10));
            m_widgets.push_back(new iconWidget(6, 87, 20, 18, 0x7d7,
                "smalres.def", resource[2], 0, 0, 0, 0x10));
        } else if (found == BOTTOM_VIEW_SILO_ONE_RESOURCE) {
            m_widgets.push_back(new iconWidget(6, 81, 20, 18, 0x7d7,
                "smalres.def", resource[1], 0, 0, 0, 0x10));
        }
    }

    sprintf(g_text, "%d", which->getGoldIncome(1));
    m_widgets.push_back(new textWidget(144, 54, 28, 12, g_text, "tiny.fnt",
        font::WHITE, 0x7d6, 1, 0, 8));

    if (which->getArmy().getNumArmies() > 0) {
        int id = 0x7d9;
        // DC lines 452/457/476 distinguish army slots from packed display
        // positions. Keep the two-dimensional army_pos table's row boundary.
        int displaySlot = 0;
        for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; i++) {
            int creature = which->getArmy().m_armies[i];
            if (creature == -1)
                continue;

            m_widgets.push_back(new iconWidget(g_townArmyCoords[displaySlot][0],
                g_townArmyCoords[displaySlot][1],
                32, 32, id++, "cprsmall.def", creature + 2, 0, 0, 0, 0x10));

            std::ostrstream quantityText;
            if (which->getArmy().m_numTroops[i] < 10000)
                quantityText << which->getArmy().m_numTroops[i] << std::ends;
            else
                quantityText << which->getArmy().m_numTroops[i] / 1000 << "k"
                              << std::ends;

            m_widgets.push_back(new textWidget(g_townArmyCoords[displaySlot][0],
                g_townArmyCoords[displaySlot][1] + 34, 32, 13,
                quantityText.str(), "tiny.fnt",
                font::WHITE, id++, 1, 0, 8));

            quantityText.freeze(false);
            ++displaySlot;
        }
    }

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }
}

VA_COMPGEN(0x00452ad0, 0x21, SCALAR_DELETING_DTOR, TBottomViewTown)

VA(0x00452b00, 0x78)
DC_ADDRESS(0x056fd4, 0x18)
MAC_ADDRESS(0x062718, 0x60)
TBottomViewTown::~TBottomViewTown()
{
}

// E:\gamedcs\bottomviewsubwindow.cpp:515
// The kingdom-overview banner: a hall-level census of the acting player's
// towns above a two-row flag strip splitting the other players into
// allies and enemies.
// The hall census calls town::HasBuilding three times (dc lines
// 531/533/535), the check_included arm reading `active`.

// DC line 524 calls _memset on the int[4] town_count local.
VA(0x00452b80, 0x620)
DC_ADDRESS(0x0563b8, 0x4c8)
MAC_ADDRESS(0x061434, 0x8a4)  // anchor-vtable 0x63bb3c + advManager::UpdBottomViewKingdom
TBottomViewKingdom::TBottomViewKingdom(heroWindow* parent)
    : type_bottom_view_window(parent)
{
    int i;

    m_widgets.reserve(8);

    int id = BOTTOM_VIEW_BACKGROUND_ID;
    m_widgets.push_back(new bitmapBorder(
        0, 0, 176, 166, id++, "AdStatin.pcx", 0x800));

    int townCount[4];
    memset(townCount, 0, sizeof(townCount));

    for (i = 0; i < g_currentPlayer->m_numTowns; i++) {
        town* which = g_game->getTown(g_currentPlayer->m_townIds[i]);
        if (which->hasBuilding(HALL_CAPITOL_ID, true))
            townCount[3]++;
        else if (which->hasBuilding(HALL_CITY_ID, true))
            townCount[2]++;
        else if (which->hasBuilding(HALL_TOWN_ID, true))
            townCount[1]++;
        else
            townCount[0]++;
    }

    std::string text;

    for (i = 0; i < 4; i++) {
        m_widgets.push_back(new iconWidget(42 * i + 6, 11, 38, 38, id++,
            "itmtl.def", i, 0, 0, 0, 0x10));
        if (townCount[i] > 0)
            m_widgets.push_back(new textWidget(42 * i + 7, 56, 37, 17,
                formatString("%d", townCount[i]).c_str(), "smalfont.fnt",
                font::WHITE, -1, 1, 0, 8));
    }

    text = formatString("%s:", (*g_generalText)[GENERAL_TEXT_ALLIES]);
    m_widgets.push_back(new textWidget(10, 103, 57, 20, text.c_str(),
        "smalfont.fnt", font::WHITE, -1, 0, 0, 8));
    text = formatString("%s:", (*g_generalText)[GENERAL_TEXT_ENEMIES]);
    m_widgets.push_back(new textWidget(10, 134, 57, 20, text.c_str(),
        "smalfont.fnt", font::WHITE, -1, 0, 0, 8));

    int allyX, enemyX;
    allyX = enemyX = 67;
    for (i = 0; i < NUM_PLAYERS; i++) {
        if (!g_game->m_playerDisabled[i]) {
            if (g_game->onSameTeam(i, g_curWatchPlayer)) {
                m_widgets.push_back(new iconWidget(allyX, 102, 15, 20, id++,
                    "itgflags.def", i, 0, 0, 0, 0x10));
                allyX += 15;
            } else {
                m_widgets.push_back(new iconWidget(enemyX, 133, 15, 20, id++,
                    "itgflags.def", i, 0, 0, 0, 0x10));
                enemyX += 15;
            }
        }
    }

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }
}

VA_COMPGEN(0x004531a0, 0x21, SCALAR_DELETING_DTOR, TBottomViewKingdom)

VA(0x004531d0, 0x78)
DC_ADDRESS(0x057020, 0x18)
MAC_ADDRESS(0x0626b8, 0x60)
TBottomViewKingdom::~TBottomViewKingdom()
{
}

//   * the backdrop is 'AdStatNx.pcx' - the Nx twin of the Ot bitmap the
//     other six use - at the full 176x166 with style 0x800;
//   * 'crest58.def' at (20,51,58,64) with the acting player's position
//     as its FRAME, then the two hourglass halves at the same
//     (98,51,58,64): 'HourSand.def' (frame reset to 0) and
//     'HourGlas.def';
//   * the two captions only exist when the acting player is human -
//     general text 631 at y=20 and the player's own name at y=123, both
//     'medfont.fnt' in font::PRIMARY, centred, with id -1.
VA(0x00453250, 0x3EE)
DC_ADDRESS(0x056880, 0x33c)
MAC_ADDRESS(0x061cd8, 0x5fc)
TBottomViewEnemyTurn::TBottomViewEnemyTurn(heroWindow* parent)
    : type_bottom_view_window(parent)
{
    int id = BOTTOM_VIEW_ENEMY_TURN_ID;

    m_widgets.push_back(new bitmapBorder(
        0, 0, 176, 166, id++, "AdStatNx.pcx", 0x800));

    m_crest = new iconWidget(20, 51, 58, 64, id++, "crest58.def",
        g_netLocalGamePos, 0, 0, 0, 0x10);
    m_widgets.push_back(m_crest);

    m_sand = new iconWidget(98, 51, 58, 64, id++, "HourSand.def",
        0, 0, 0, 0, 0x10);
    m_sand->setIconFrame(0);
    m_widgets.push_back(m_sand);

    m_frame = 0;
    m_frameDelay = 50;

    m_hourGlass = new iconWidget(98, 51, 58, 64, id++, "HourGlas.def",
        0, 0, 0, 0, 0x10);
    m_widgets.push_back(m_hourGlass);

    m_step = 0;
    m_lastPlayerPos = g_netLocalGamePos;

    for (int i = 0; i < NUM_PLAYERS; i++) {
        if (g_game->m_playerDisabled[i] || g_game->m_players[i].isHuman())
            m_mobility[i] = 0;
        else
            m_mobility[i] = sumMobility(i);
    }

    if (g_currentPlayer->isHuman()) {
        m_widgets.push_back(new textWidget(0, 20, 176, 31,
            (*g_generalText)[GENERAL_TEXT_CURRENT_PLAYER_IS], "medfont.fnt", font::PRIMARY,
            -1, 1, 0, 8));
        m_widgets.push_back(new textWidget(0, 123, 176, 31,
            g_currentPlayer->m_name, "medfont.fnt", font::PRIMARY,
            -1, 1, 0, 8));
    }

    for (unsigned int w = 0; w < m_widgets.size(); w++)
        addWidget(m_widgets[w], -1);

    m_lastStepTime = GameTime::get();
}

VA_COMPGEN(0x00453640, 0x21, SCALAR_DELETING_DTOR, TBottomViewEnemyTurn)

VA_COMPGEN(0x00453970, 0xAE, CLASS_CTOR, basic_ostream)

VA_COMPGEN(0x00455820, 0x10B, CLASS_CTOR, numpunct)

VA(0x00453670, 0x78)
DC_ADDRESS(0x05706c, 0x18)
MAC_ADDRESS(0x062658, 0x60)
TBottomViewEnemyTurn::~TBottomViewEnemyTurn()
{
}

// E:\gamedcs\bottomviewsubwindow.cpp:646
// `player` is a local, not a repeated `gpGame->players[player_id]`.
DC_ADDRESS(0x056bbc, 0x58)
MAC_ADDRESS(0x0622d4, 0x98)
long TBottomViewEnemyTurn::sumMobility(long playerId)
{
    playerData* player = &g_game->m_players[playerId];
    long total = 1000;
    for (int i = 0; i < player->m_numHeroes; i++)
        total += g_game->getHero(player->m_heroes[i])->getMobility();
    return total;
}

// lastStepTime IS READ INTO A LOCAL BEFORE THE FIRST CALL, exactly as
// in TBottomViewNewTurn::animate.
VA(0x004536f0, 0x271)
DC_ADDRESS(0x056c14, 0x1e4)
MAC_ADDRESS(0x06236c, 0x2ec)
void TBottomViewEnemyTurn::animate()
{
    unsigned long lastStep = m_lastStepTime;
    if (GameTime::elapsedSince(lastStep) < m_frameDelay)
        return;

    int numFrames = m_hourGlass->m_sprite->GetNumFrames(0);

    m_lastStepTime = GameTime::get();

    if (m_lastPlayerPos != g_netLocalGamePos) {
        m_lastPlayerPos = g_netLocalGamePos;
        m_crest->setIconFrame(g_netLocalGamePos);
        m_crest->draw();
        m_crest->forceUpdate();
        if (g_currentPlayer->isHuman()) {
            m_hourGlass->setVisible(0);
            m_sand->setVisible(0);
            return;
        }
        m_hourGlass->setVisible(1);
        m_sand->setVisible(1);
        m_mobility[g_netLocalGamePos] = sumMobility(g_netLocalGamePos);
        m_step = 0;
    }

    if (g_currentPlayer->isHuman())
        return;

    int movePointsLeft = 0;
    for (int i = 0; i < g_currentPlayer->m_numHeroes; i++)
        movePointsLeft +=
            g_game->getHero(g_currentPlayer->m_heroes[i])->m_movePoints;

    int total = m_mobility[g_netLocalGamePos];
    if (movePointsLeft > total)
        movePointsLeft = total;

    int target;
    if (total == 0)
        target = numFrames - 1;
    else
        target = (total - movePointsLeft) * (numFrames - 1) / total;

    if (m_step < target)
        m_step++;

    m_frame++;
    if (m_frame >= m_sand->m_sprite->GetNumFrames(0))
        m_frame = 0;

    m_hourGlass->setIconFrame(m_step);
    m_sand->setIconFrame(m_frame);
    m_sand->draw();
    m_hourGlass->draw();
    m_hourGlass->forceUpdate();
    m_crest->setIconFrame(g_netLocalGamePos);
    m_crest->draw();
    m_crest->forceUpdate();
}

// COMDAT pairing: basic_streambuf::1?$basic_streambuf.
VA_COMPGEN(0x00454000, 0x50, IMPLICIT_DTOR, basic_streambuf)

// COMDAT pairing: numpunct::1?$numpunct.
VA_COMPGEN(0x00455bf0, 0x2F, IMPLICIT_DTOR, numpunct)

// COMDAT pairing: basic_streambuf::0?$basic_streambuf.
VA_COMPGEN(0x00453f50, 0xA9, CLASS_CTOR, basic_streambuf)

// COMDAT pairing: num_put::0?$num_put.
VA_COMPGEN(0x004546e0, 0x5C, CLASS_CTOR, num_put)

// COMDAT pairing: sentry::1sentry, mnemonic agreement 0.911.
VA_COMPGEN(0x00454320, 0x63, IMPLICIT_DTOR, sentry)

// COMDAT pairing: fpos::0?$fpos.
VA_COMPGEN(0x00454390, 0x3C, CLASS_CTOR, fpos)

// COMDAT pairing: _Iput on the char instantiation.
VA_COMPGEN(0x004552d0, 0x3FE, NUM_PUT_IPUT, char)

// COMDAT pairing: _Fput on the char instantiation.
VA_COMPGEN(0x00454f20, 0x3A1, NUM_PUT_FPUT, char)

// COMDAT pairing: xsgetn on the char instantiation.
VA_COMPGEN(0x004540b0, 0x98, STREAMBUF_XSGETN, char)

// COMDAT pairing: _Rep on the char instantiation.
VA_COMPGEN(0x004556d0, 0x82, NUM_PUT_REP, char)

// COMDAT pairing: opfx on the char instantiation.
VA_COMPGEN(0x004543d0, 0x74, OSTREAM_OPFX, char)

// COMDAT pairing: sputc on the char instantiation.
VA_COMPGEN(0x00454a30, 0x4B, STREAMBUF_SPUTC, char)

// COMDAT pairing: _Decref on the char instantiation.
VA_COMPGEN(0x00453d30, 0x3B, LOCALE_FACET_DECREF, char)

// COMDAT pairing: num_put::do_put(bool); six do_put overloads share one key.
VA_COMPGEN(0x00454770, 0x292, NUM_PUT_DO_PUT, char)

// COMDAT pairing: num_put::do_put(long).
VA_COMPGEN(0x00454a80, 0x9F, NUM_PUT_DO_PUT, char)

// COMDAT pairing: num_put::do_put(unsigned long).
VA_COMPGEN(0x00454b20, 0x9F, NUM_PUT_DO_PUT, char)

// COMDAT pairing: num_put::do_put(double).
VA_COMPGEN(0x00454bc0, 0xCA, NUM_PUT_DO_PUT, char)

// COMDAT pairing: num_put::do_put(long double).
VA_COMPGEN(0x00454c90, 0xCE, NUM_PUT_DO_PUT, char)

// COMDAT pairing: num_put::do_put(const void*).
VA_COMPGEN(0x00454d60, 0x1B2, NUM_PUT_DO_PUT, char)

// COMDAT pairing: bad_cast::_Doraise.
VA_COMPGEN(0x00453c80, 0x1D, EXCEPTION_DORAISE, bad_cast)

// The three destructor pairs this compiland's <iostream> surface leaves,
// each identified by the vtable its body stores: bad_cast's (tail-jumping
// ??1exception), basic_ios<char>'s (tail-jumping ??1ios_base), and the
// wrappers over ??1basic_streambuf and ??1numpunct<char>.
VA_COMPGEN(0x00453c70, 0xB, IMPLICIT_DTOR, bad_cast)

VA_COMPGEN(0x00453ca0, 0x21, SCALAR_DELETING_DTOR, bad_cast)

VA_COMPGEN(0x00453f40, 0xB, IMPLICIT_DTOR, basic_ios)

VA_COMPGEN(0x004542c0, 0x21, SCALAR_DELETING_DTOR, basic_ios)

VA_COMPGEN(0x004542f0, 0x21, SCALAR_DELETING_DTOR, basic_streambuf)

VA_COMPGEN(0x00455bc0, 0x21, SCALAR_DELETING_DTOR, numpunct)

// COMDAT pairing: bad_cast::bad_cast(const&).
VA_COMPGEN(0x00453cd0, 0x1C, CLASS_CTOR, bad_cast)

// COMDAT pairing: ios_base::getloc.
VA_COMPGEN(0x00453d90, 0x3B, IOS_BASE_GETLOC, char)

// COMDAT pairing: bad_cast::bad_cast(const char*), the second half of the
// ctor overload group.
VA_COMPGEN(0x00454a10, 0x1C, CLASS_CTOR, bad_cast)

VA_COMPGEN(0x00455930, 0x4, NUMPUNCT_DO_DECIMAL_POINT, char)

VA_COMPGEN(0x00455940, 0x4, NUMPUNCT_DO_THOUSANDS_SEP, char)

VA_COMPGEN(0x00455950, 0xCF, NUMPUNCT_DO_GROUPING, char)

VA_COMPGEN(0x00455a20, 0xCF, NUMPUNCT_DO_FALSENAME, char)

VA_COMPGEN(0x00455af0, 0xCF, NUMPUNCT_DO_TRUENAME, char)

VA_COMPGEN(0x00455760, 0x1E, NUMPUNCT_FALSENAME, char)

VA_COMPGEN(0x00455780, 0x1E, NUMPUNCT_TRUENAME, char)

VA_COMPGEN(0x00455800, 0x1E, NUMPUNCT_GROUPING, char)

// COMDAT pairing: _Tidyfac's two instantiations (num_put<char> and
// numpunct<char>), each pair sharing one static `_Ptr`.
VA_COMPGEN(0x00455c20, 0x7B, TIDYFAC_NUM_PUT_SAVE, char)

VA_COMPGEN(0x00455ca0, 0x7B, TIDYFAC_NUMPUNCT_SAVE, char)

VA_COMPGEN(0x00455d20, 0x92, TIDYFAC_NUM_PUT_TIDY, char)

VA_COMPGEN(0x00455dc0, 0x92, TIDYFAC_NUMPUNCT_TIDY, char)

// COMDAT pairing: basic_streambuf<char>::uflow, a defaulted virtual of the
// stream base (installed in four vtables, among them TGzInflateBuf's).
VA_COMPGEN(0x00454080, 0x2B, STREAMBUF_UFLOW, char)

// COMDAT pairing: ostreambuf_iterator<char>::operator=(char).
VA_COMPGEN(0x004557a0, 0x5C, OSTREAMBUF_ITERATOR_ASSIGN, char)

VA_COMPGEN(0x00453a20, 0xF, IMPLICIT_DTOR, basic_ostream)

// COMDAT pairing: basic_ostream<char>::operator<<(int) - the MEMBER
// overload.
VA_COMPGEN(0x00453a30, 0x238, OSTREAM_INSERT_INT, char)

// COMDAT pairing: locale::id::operator size_t and locale::~locale.
VA_COMPGEN(0x00453cf0, 0x32, LOCALE_ID_CAST, char)

VA_COMPGEN(0x00453d70, 0x18, IMPLICIT_DTOR, locale)

// COMDAT pairing: basic_streambuf<char>::seekoff and ::seekpos - the BASE
// class defaults.
VA_COMPGEN(0x00454200, 0x28, STREAMBUF_SEEKOFF, char)

VA_COMPGEN(0x00454230, 0x28, STREAMBUF_SEEKPOS, char)

VA_COMPGEN(0x00454740, 0x23, SCALAR_DELETING_DTOR, facet)

VA_COMPGEN(0x00454050, 0x6, STREAMBUF_OVERFLOW, char)

VA_COMPGEN(0x00454060, 0x3, STREAMBUF_SHOWMANYC, char)

VA_COMPGEN(0x00454070, 0x4, STREAMBUF_UNDERFLOW, char)

VA_COMPGEN(0x00454260, 0x5, STREAMBUF_SETBUF, char)

VA_COMPGEN(0x00454270, 0x3, STREAMBUF_IMBUE, char)

// COMDAT pairing: basic_ostream<char>'s scalar deleting destructor.
VA_COMPGEN(0x00454280, 0x32, SCALAR_DELETING_DTOR, basic_ostream)
