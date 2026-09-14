// bottomviewsubwindow.cpp - E:\gamedcs\bottomviewsubwindow.cpp (compiland bottomviewsubwindow.obj)
#include <va.h>
#include <crt_stdio.h>
#include <strstream>
// TBottomViewKingdom's hall census calls town::HasBuilding three times
// (dc 0x563b8 lines 531/533/535), so this compiland gets the Town.h
// inline; see town.h for why the visibility is scoped.
#include "bottomviewsubwindow.h"
#include "border.h"
#include "game.h"
#include "iconwdgt.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "resourcemanager.h"
#include "soundmgr.h"
#include "textresource.h"
#include "textwdgt.h"
#include "widget.h"
#include "window.h"

__declspec(nothrow) void __cdecl operator delete(void* p);

// THE LOCAL NAMES BELOW ARE DREAMCAST CODEVIEW, NOT INVENTION. The DC
// build's S_REGREL32 records for this compiland name four of them and
// give their types: TBottomViewKingdom's `town_count` (LF_ARRAY of
// T_INT4, length 16 - so `int[4]`, which also confirms the array's
// width) and `text` (std::string); TBottomViewTown's `town_size_name`
// (std::string); TBottomViewResourceMessage's parameter `res` and its
// `char str[20]`. The DC list is INCOMPLETE - TBottomViewHero's record
// names no stack local at all although its source plainly has a
// std::string in the army loop - so its silence is not evidence of
// absence, only its entries are evidence of presence.

// THE INCLUDE SET IS NOT THE WALL IN THIS TU - measured, not assumed.
// The four constructors below plateau on inline-depth divergence, and
// the standing hypothesis was that C1XX front-end state (the symbol
// handle numbering of docs/vc6/handle-order.md, delivered through the
// IL) carried it, so that matching retail's include closure would move
// all four together. It does not. Forty-five controlled probes were run
// against all four rows at once:

//   * six declaration kinds at file scope - `extern int`, forward tag,
//     `struct { int a; }`, `extern <UDT>*`, a member-function
//     declarator, a first-virtual declarator - at counts 1..12, then
//     32, 64, 128 and 256, at two localities (before every #include,
//     and after all of them);
//   * eight mutations of the include list itself - reversing the quoted
//     block, reversing the whole block, hoisting game.h, moving the <>
//     headers last, and adding hero.h / town.h / castle.h.

// Every one is byte-flat: TBottomViewHero 96.52, Town 94.31, Kingdom
// 94.06, ResourceMessage 91.30, and the whole-tree fuzzy figure
// unchanged to four decimals. The probes are NOT null - `homm3 vc6
// il-diff` confirms each reaches the front end (nine `extern int` move
// the gl high-water 0xf63c -> 0xf645 and perturb the ex stream across
// 392 function spans; adding hero.h moves 118700 ex bytes) and produce
// identical object code anyway. The C1 handle-order lever does not
// reach this TU. See TBottomViewKingdom for what the wall actually is.

#if 0  // @carcass

// E:\gamedcs\bottomviewsubwindow.cpp:39
DC_ONLY(0x550b8, 0x5C)
void type_bottom_view_window::type_bottom_view_window(heroWindow* parent_window)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:47
DC_ONLY(0x55114, 0x74)
void type_bottom_view_window::~type_bottom_view_window()
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:64
DC_ONLY(0x55188, 0x4)
void type_bottom_view_window::animate()
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:77
// RETAIL_LOCATED(0x00450dd0, 0x319)  // anchor-vtable + anchor-caller
DC_ONLY(0x5518c, 0x2BC)
void TBottomViewNewTurn::TBottomViewNewTurn(heroWindow* parent)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:121
// RETAIL_LOCATED(0x004511a0, 0x79)  // anchor-vtable + anchor-caller
DC_ONLY(0x55448, 0x64)
void TBottomViewNewTurn::animate()
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:146
// RETAIL_LOCATED(0x00451220, 0x393)  // anchor-vtable + anchor-caller
DC_ONLY(0x554ac, 0x2BC)
void TBottomViewResourceMessage::TBottomViewResourceMessage(heroWindow* parent, int res, int quantity, const std::basic_string<char,std::char_traits<char>,std::allocator<char>* message)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:197
// RETAIL_LOCATED(0x00451820, 0x1DC)  // anchor-vtable + anchor-caller
DC_ONLY(0x55768, 0x140)
void TBottomViewMessage::TBottomViewMessage(heroWindow* parent, const std::basic_string<char,std::char_traits<char>,std::allocator<char>* message)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:222
// RETAIL_LOCATED(0x00451ab0, 0x68A)  // anchor-vtable + anchor-caller
DC_ONLY(0x558a8, 0x54C)
void TBottomViewHero::TBottomViewHero(heroWindow* parent)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:352
// RETAIL_LOCATED(0x004521f0, 0x8D4)  // anchor-vtable + anchor-caller
DC_ONLY(0x55df4, 0x5C4)
void TBottomViewTown::TBottomViewTown(heroWindow* parent)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:515
// RETAIL_LOCATED(0x00452b80, 0x620)  // anchor-vtable + anchor-caller
DC_ONLY(0x563b8, 0x4C8)
void TBottomViewKingdom::TBottomViewKingdom(heroWindow* parent)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:599
// RETAIL_LOCATED(0x00453250, 0x3EE)  // anchor-vtable + anchor-caller
DC_ONLY(0x56880, 0x33C)
void TBottomViewEnemyTurn::TBottomViewEnemyTurn(heroWindow* parent)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:646
DC_ONLY(0x56bbc, 0x58)
long TBottomViewEnemyTurn::sumMobility(long player_id)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:661
// RETAIL_LOCATED(0x004536f0, 0x271)  // anchor-vtable + anchor-caller
DC_ONLY(0x56c14, 0x1E4)
void TBottomViewEnemyTurn::animate()
{
    // @stub
}

// E:\gamedcs\Widget.h:263
DC_ONLY(0x56df8, 0x28)
void widget::setVisible(unsigned char arg)
{
    // @stub
}

// E:\gamedcs\Widget.h:271
DC_ONLY(0x56e20, 0x1C)
void widget::force_update()
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:41
DC_ONLY(0x56e3c, 0x34)
void* type_bottom_view_window::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:118
DC_ONLY(0x56e70, 0x34)
void* TBottomViewNewTurn::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:118
DC_ONLY(0x56ea4, 0x18)
void TBottomViewNewTurn::~TBottomViewNewTurn()
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:183
DC_ONLY(0x56ebc, 0x34)
void* TBottomViewResourceMessage::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:183
DC_ONLY(0x56ef0, 0x18)
void TBottomViewResourceMessage::~TBottomViewResourceMessage()
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:209
DC_ONLY(0x56f08, 0x34)
void* TBottomViewMessage::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:209
DC_ONLY(0x56f3c, 0x18)
void TBottomViewMessage::~TBottomViewMessage()
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:339
DC_ONLY(0x56f54, 0x34)
void* TBottomViewHero::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:339
DC_ONLY(0x56f88, 0x18)
void TBottomViewHero::~TBottomViewHero()
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:503
DC_ONLY(0x56fa0, 0x34)
void* TBottomViewTown::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:503
DC_ONLY(0x56fd4, 0x18)
void TBottomViewTown::~TBottomViewTown()
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:586
DC_ONLY(0x56fec, 0x34)
void* TBottomViewKingdom::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:586
DC_ONLY(0x57020, 0x18)
void TBottomViewKingdom::~TBottomViewKingdom()
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:643
DC_ONLY(0x57038, 0x34)
void* TBottomViewEnemyTurn::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\bottomviewsubwindow.cpp:643
DC_ONLY(0x5706c, 0x18)
void TBottomViewEnemyTurn::~TBottomViewEnemyTurn()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x57084, 0x20)
widget** std::vector<widget *,std::allocator<widget *> >::operator[](unsigned __n)
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x00450d20, 0x21, SCALAR_DELETING_DTOR, type_bottom_view_window)

VA(0x00450d50, 0x78)  // dc 0x55114
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

// The seven derived destructors. Each has an EMPTY body in the source -
// no derived class owns storage that needs releasing - so all seven are
// the same 120 bytes: the base destructor inlined whole, its 0x63bb04
// vptr store, its widget-removal loop and its ??1TSubWindow call. The
// derived class's OWN vptr store is dead (the inlined base overwrites it
// immediately) and VC6 eliminates it, which is why not one of these
// bodies mentions its own vftable and why the only image-wide reference
// to each derived table is inside that class's constructor.

// The identity of every pair is settled by three independent facts that
// agree, and the sizes leave no freedom:

//   ??_G     ~dtor    vftable     ctor (identified by its advManager
//   -------  -------  ----------  caller) that precedes the pair
//   0x4510f0 0x451120 0x63bb0c    0x450dd0  UpdBottomViewNewTurn
//   0x451770 0x4517a0 0x63bb1c    0x451220  UpdBottomViewResMsg
//   0x451a00 0x451a30 0x63bb24    0x451820  UpdBottomViewMessage
//   0x452140 0x452170 0x63bb2c    0x451ab0  UpdBottomViewHero
//   0x452ad0 0x452b00 0x63bb34    0x4521f0  UpdBottomViewTown
//   0x4531a0 0x4531d0 0x63bb3c    0x452b80  UpdBottomViewKingdom
//   0x453640 0x453670 0x63bb44    0x453250  UpdBottomViewEnemyTurn

// First, the eight two-slot tables 0x63bb04..0x63bb44 are consecutive and
// each one's slot 0 is exactly one of these 33-byte wrappers, so the
// tables enumerate the family in emission order. Second, each derived
// table has EXACTLY ONE image-wide reference and it sits inside the
// constructor listed above, whose own caller is the matching
// advManager::UpdBottomViewXxx - the class name is read off the caller,
// not guessed. Third, retail's layout is plain source order with the
// pair falling immediately after its class's constructor, and that order
// reproduces the Dreamcast line order (77, 146, 197, 222, 352, 515, 599)
// exactly.

// Slot 1 corroborates the two classes DC gives an animate override to:
// 0x63bb0c and 0x63bb44 point at their own bodies (0x4511a0, 0x4536f0)
// while the other five share the base's 0x5bc690.

// This also corrects a misattribution left in the carcass above:
// 0x4521f0 is TBottomViewTown's constructor, not
// type_bottom_view_window's.

// The base constructor. No retail body exists for it: every one of the
// seven derived constructors inlines it, nothing else calls it, and
// retail's /Gy + /OPT:REF link then drops the unreferenced copy - the
// same mechanism that puts textWidget::SetText's COMDAT far outside
// textwdgt.obj's band. It is defined here purely to be inlined, and
// carries no claim. Its shape is read straight off every derived
// constructor's first eighteen instructions: TSubWindow's default
// constructor, then this class's vptr, then the one body statement.
type_bottom_view_window::type_bottom_view_window(heroWindow* parentWindow)
{
    initialize(614, 400, 176, 166, parentWindow);
}

// The four new-week announcement icons, indexed by the game's week number
// 1..4; slot 0 is never read, which is why retail's dword there is null.
DATA(0x00660b9c)
static const char* g_newWeekIcons[5] = {
    0, "NewWeek1.def", "NewWeek2.def", "NewWeek3.def", "NewWeek4.def"
};

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

// TWO SPELLINGS ARE LOAD-BEARING.

// The two calendar arms tear their format_string temporary down
// DIFFERENTLY - THEN calls ?_Tidy@ out of line, ELSE has it inlined - and
// so does `text` itself at the end. That asymmetry is the /Ob2 budget
// spending itself down across the body, not a source difference; both arms
// are the same `text = format_string(...)` statement.

VA(0x00450dd0, 0x319)  // dc 0x5518c
TBottomViewNewTurn::TBottomViewNewTurn(heroWindow* parent)
    : type_bottom_view_window(parent)
{
    int id = BOTTOM_VIEW_BACKGROUND_ID;

    m_widgets.push_back(new bitmapBorder(
        0, 0, 176, 166, id++, "AdStatOt.pcx", 0x800));

    m_frame = 0;

    std::string text;
    const char* iconName;
    if (g_game->m_day == 1
        && !(g_game->m_week == 1 && g_game->m_month == 1)) {
        iconName = g_newWeekIcons[
            static_cast<unsigned short>(g_game->m_week)];
        text = formatString("%s %d", g_generalText->getText(64),
            static_cast<unsigned short>(g_game->m_week));
    } else {
        iconName = "NewDay.def";
        text = formatString("%s %d", g_generalText->getText(65),
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

VA(0x00451120, 0x78)  // dc 0x56ea4
TBottomViewNewTurn::~TBottomViewNewTurn()
{
}

VA(0x004511a0, 0x79)  // dc 0x55448
void TBottomViewNewTurn::animate()
{
    if (m_frame == m_icon->m_sprite->getNumFrames(0) - 1)
        return;

    unsigned long lastStep = m_lastStepTime;
    if (static_cast<long>(GameTime::get() - lastStep) < m_frameDelay)
        return;

    ++m_frame;
    m_icon->setIconFrame(m_frame);
    m_icon->draw();
    m_backdrop->draw();
    m_icon->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_UPDATE);
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
//   * the sprite is released through vtable slot 1 - CSprite::Dispose -
//     at the end of the guarded block.

// Residual (91.30%): THE FIRST TWO push_backs STOP ONE INLINE LEVEL
// SHORT IN RETAIL. Retail reaches the backdrop and the message through
// 0x422f50 - the TWO-argument `insert(iterator, const widget*&)`, kept
// out of line - while the icon and the quantity reach 0x54d120, the
// three-argument `insert(iterator, size_type, const widget*&)` that
// every other constructor in this file produces. Our CL expands the
// two-argument insert at all four sites and so always emits the
// three-argument call. This is NOT a spelling: `push_back(w)` and
// `Widgets.insert(Widgets.end(), w)` were measured byte-identical here,
// both giving the three-argument form, so the divergence is inline
// DEPTH and nothing else - the same class as TBottomViewKingdom's
// out-of-line vector::size(). The rest of the delta is downstream
// scheduling: retail lets the sprite's register die across the icon's
// argument list because it reads both sprite fields first.

// SAME LEVER AS TBottomViewKingdom, DIFFERENT THRESHOLD. Read that
// function's note first for the mechanism. Padding this body with free
// (cb <= 0x28) inline candidates traces a single-peaked curve - 1 and 2
// sites 91.30, 3 sites 91.30, FIVE sites 93.21, 8 sites 92.80, 12 sites
// 91.26, 20 sites 87.19 - so retail's own body carries roughly five
// more inline candidates than this reconstruction and the two-argument
// insert falls out of the nested budget once it does. That is a large
// enough deficit to be a real reconstruction gap rather than one
// unnoticed accessor, so the probe is not landed and the gap is left
// named for the next lane. The include set is NOT the cause - see the
// TU-level note at the top of this file.

// A LEAD FOR THAT GAP, FROM THE DREAMCAST CODEVIEW. The DC build of
// this constructor (dc 0x554ac) names exactly one stack local, and it
// sits inside the nested blocks that are the `res >= 0` guard: `str`,
// whose type index resolves to LF_ARRAY of T_RCHAR length 20 - a plain
// `char str[20]`, not a stream object. This reconstruction has no such
// buffer. The obvious reading, `std::ostrstream(char*, streamsize)`
// over a caller-supplied buffer, was BUILT AND REJECTED: that ctor is
// _CRTIMP (out of line, where the default ctor is inline) and both
// `sizeof(str)` and a literal 20 score 86.62, nearly five points BELOW
// the default-ctor spelling. So the buffer is real evidence and the
// buffered stream is not its explanation; what `str` is remains open,
// and it is the first place to look for the five missing candidates.
VA(0x00451220, 0x393)  // anchor-vtable 0x63bb1c + advManager::UpdBottomViewResMsg, dc 0x554ac
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
        CSprite* sprite = ResourceManager::getSprite("resour82.def");

        m_widgets.push_back(new iconWidget((m_width - sprite->getWidth()) / 2, 50,
            sprite->getWidth(), sprite->getHeight(), 0x837, "resour82.def", res,
            0, 0, 0, 0x10));

        std::ostrstream quantityText;
        quantityText << quantity << std::ends;

        int textWidth = g_unnamed698a08->lineWidth(quantityText.str());
        int fontHeight = g_unnamed698a08->m_fs.m_height;

        m_widgets.push_back(new textWidget((m_width - textWidth) / 2,
            sprite->getHeight() + 55, textWidth, fontHeight,
            quantityText.str(), "smalfont.fnt", font::PRIMARY, 0x836,
            1, 0, 8));

        quantityText.freeze(false);
        sprite->dispose();
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

// UNBLOCKED by the constructor above - its 0x63bb1c store is the one
// image-wide reference to this class's table.
VA_COMPGEN(0x00451770, 0x21, SCALAR_DELETING_DTOR, TBottomViewResourceMessage)

VA(0x004517a0, 0x78)  // dc 0x56ef0
TBottomViewResourceMessage::~TBottomViewResourceMessage()
{
}

VA(0x00451820, 0x1DC)  // dc 0x55768
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

VA(0x00451a30, 0x78)  // dc 0x56f3c
TBottomViewMessage::~TBottomViewMessage()
{
}

// The four primary-skill number boxes, one row across the portrait's
// right; the loop walks this table with an 8-byte step and tests its
// own cursor against &[2] for the "power and knowledge floor at 1"
// arm, which is what proves the row is a table and not four literals.
DATA(0x00660bb0)
static const int g_heroStatCoords[4][2] = {
    { 65, 51 }, { 92, 51 }, { 120, 51 }, { 148, 51 }
};

// The seven army-slot positions, three over four. Contiguous with the
// table above in retail .data, but a separate object: the stat loop
// stops at this table's first byte and the army loop starts there.
DATA(0x00660bd0)
static const int g_heroArmyCoords[7][2] = {
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
//   * the army block is gated on a separate counting sweep over the
//     seven type slots. That sweep has no call in it, so VC6 turns it
//     into the `mov ecx,7` / `dec ecx` countdown retail shows, while the
//     display loop below it stays a pointer walk.

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
// 2026-08-14). dc 0x558a8 line 229 calls it by name; see the note on
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
VA(0x00451ab0, 0x68A)  // anchor-vtable 0x63bb2c + advManager::UpdBottomViewHero, dc 0x558a8
TBottomViewHero::TBottomViewHero(heroWindow* parent)
    : type_bottom_view_window(parent)
{
    m_widgets.reserve(25);

    m_widgets.push_back(new bitmapBorder(0, 0, 176, 166,
        BOTTOM_VIEW_BACKGROUND_ID, "AdStatHr.pcx", 0x800));

    hero* who = g_game->getCurrHero();

    m_widgets.push_back(new bitmapBorder(3, 2, 58, 64, 0x7d1,
        g_heroTraits[who->m_portrait].m_largePortraitName, 0x800));
    m_widgets.push_back(new textWidget(66, 2, 107, 17, who->m_name,
        "smalfont.fnt", font::WHITE, 0x7d2, 0, 0, 8));

    for (int i = 0; i < 4; i++) {
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
    for (int n = 0; n < 7; n++) {
        if (who->m_army.m_armies[n] != -1)
            numStacks++;
    }

    if (numStacks > 0) {
        int id = 0x7db;
        for (int j = 0; j < 7; j++) {
            int type = who->m_army.m_armies[j];
            if (type != -1) {
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

// UNBLOCKED by the constructor above - its 0x63bb2c store is the one
// image-wide reference to this class's table.
VA_COMPGEN(0x00452140, 0x21, SCALAR_DELETING_DTOR, TBottomViewHero)

VA(0x00452170, 0x78)  // dc 0x56f88
TBottomViewHero::~TBottomViewHero()
{
}

// The town window's own seven army-slot positions. Byte-identical in
// CONTENT to gHeroArmyCoords above and immediately after it in retail
// .data, but a separate object: the hero loop's cursor stops at this
// table's first byte and this loop's cursor starts there.
DATA(0x00660c08)
static const int g_townArmyCoords[7][2] = {
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

// THE SILO ROW COLLECTS INDICES INTO A TWO-SLOT ARRAY. get_silo_income
// hands back a seven-entry row; the sweep records the position of each
// non-zero entry and the display then has exactly two shapes - two
// icons at y=75/87 when two resources are produced, one at y=81 when
// one is. Retail walks the array with a cursor that LAGS by one slot
// (`lea edx,[ebp-0x2c]` is &slots[-1] and it pre-increments) and
// reloads `i` after every store because the store through that cursor
// might alias it. Both are what `slots[found++] = i` produces.

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
// 2026-08-30). dc 0x55df4 spells the hall ladder (lines
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
// Address-arithmetic review (2026-09-10): indexing army_pos by the packed
// display slot replaces its flattened int* walk and raises 94.0054 to 94.80%.
VA(0x004521f0, 0x8D4)  // anchor-vtable 0x63bb34 + advManager::UpdBottomViewTown, dc 0x55df4
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
    if (which->hasBuilding(HALL_TOWN_ID, 0))
        hallLevel = 1;
    else if (which->hasBuilding(HALL_CITY_ID, 0))
        hallLevel = 2;
    else if (which->hasBuilding(HALL_CAPITOL_ID, 0))
        hallLevel = 3;

    std::string townSizeName = g_townSizeNames[hallLevel];

    m_widgets.push_back(new iconWidget(67, 31, 34, 34, 0x7d3, "itmtls.def",
        hallLevel, 0, 0, 0, 0x10));

    int fortLevel;
    if (which->hasBuilding(CASTLE_FORT_ID, 0))
        fortLevel = 0;
    else if (which->hasBuilding(CASTLE_CITADEL_ID, 0))
        fortLevel = 1;
    else if (which->hasBuilding(CASTLE_CASTLE_ID, 0))
        fortLevel = 2;
    else
        fortLevel = 3;

    m_widgets.push_back(new iconWidget(105, 31, 34, 34, 0x7d4, "itmcls.def",
        fortLevel, 0, 0, 0, 0x10));

    if (which->m_garrisonHeroId != -1)
        m_widgets.push_back(new bitmapBorder(149, 76, 22, 30, 0x7d8,
            "townqkgh.pcx", 0x800));

    if (which->hasBuilding(MARKETPLACE_SILO_ID, 1)) {
        int* resource = which->getSiloIncome();
        int slots[2];
        int found = 0;
        for (int i = 0; i <= 6; i++) {
            if (resource[i] != 0)
                slots[found++] = i;
        }
        if (found == BOTTOM_VIEW_SILO_TWO_RESOURCES) {
            m_widgets.push_back(new iconWidget(6, 75, 20, 18, 0x7d7,
                "smalres.def", slots[0], 0, 0, 0, 0x10));
            m_widgets.push_back(new iconWidget(6, 87, 20, 18, 0x7d7,
                "smalres.def", slots[1], 0, 0, 0, 0x10));
        } else if (found == BOTTOM_VIEW_SILO_ONE_RESOURCE) {
            m_widgets.push_back(new iconWidget(6, 81, 20, 18, 0x7d7,
                "smalres.def", slots[0], 0, 0, 0, 0x10));
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

// UNBLOCKED by the constructor above - its 0x63bb34 store is the one
// image-wide reference to this class's table.
VA_COMPGEN(0x00452ad0, 0x21, SCALAR_DELETING_DTOR, TBottomViewTown)

VA(0x00452b00, 0x78)  // dc 0x56fd4
TBottomViewTown::~TBottomViewTown()
{
}

// E:\gamedcs\bottomviewsubwindow.cpp:515
// The kingdom-overview banner: a hall-level census of the acting player's
// towns above a two-row flag strip splitting the other players into
// allies and enemies.
// THE MISSING INLINE CANDIDATE WAS town::HasBuilding, AND IT IS LANDED
// (94.0575 -> 98.5213, 2026-08-15). The hall census below calls it three
// times where this body tested `active & bitNumber[HALL_*_ID]` by hand;
// dc 0x563b8 lines 531/533/535 are `mov #13/#12/#11,r5 / mov #1,r6 /
// jsr @r11` on `?HasBuilding@town@@QBA_NH_N@Z`, and retail's own
// out-of-line copy at 0x4305a0 proves the arm: `check_included != 0`
// reads [ecx+0x158] = active. The expansion is byte-identical to the
// ladder that was here, so the bytes never arbitrated - only the /Ob2
// candidate-site count did, and three real sites land exactly on the +1
// number the probe had measured (98.5213 vs the probe's 98.52).

// What the probe had established, kept because it is what made the
// landing legible: the wall was the INLINE-CANDIDATE SITE COUNT of this
// body. Retail called vector<widget*>::size() out of line (0x423110)
// inside reserve() where our CL expanded its 19 bytes; capacity() was
// inlined on BOTH sides, so retail's nested budget ran out between two
// 19-byte functions. The RE'd /Ob2 rule (docs/vc6/inliner.md section 2)
// hands a nested expansion `budget / sites-remaining`, so that decision
// is controlled by the NUMBER of inline-candidate call sites, not by
// size: one more free (cb <= 0x28) site was worth 94.06 -> 98.52, the
// callee did not matter (size/capacity/empty all gave 98.52), the site
// was inert everywhere BEFORE Widgets.reserve(8) and worth the full
// 4.46 everywhere at or after it, and three free probe sites overshot
// (95.88). A bare `Widgets.size();` was deliberately never landed
// because a probe is a score, not a reconstruction - the DC line table
// then named the statement that carries the sites honestly.

// The remaining 1.48 is a SECOND, independent divergence: our CL CSEs
// the constant zero into ESI across the whole prologue (`xor esi,esi`,
// then `cmp eax,esi` for the new-null test, `push esi` twice for the
// backdrop's x/y, four `mov [ebp-N],esi` for counts[]) where retail
// rematerialises it after the bitmapBorder call (`test eax,eax`,
// `push 0x0` twice, `xor eax,eax` / `xor ecx,ecx`). TBottomViewTown has
// the SAME divergence with the sides reversed - retail CSEs there and
// we do not - so it is not a spelling of this body. The counts[] store
// order is downstream of it and byte-invariant under every init form
// tried: four statements, a chained assignment, an aggregate
// initializer and a hand-written 3/0/1/2 order all give the same bytes.

VA(0x00452b80, 0x620)  // anchor-vtable 0x63bb3c + advManager::UpdBottomViewKingdom, dc 0x563b8
TBottomViewKingdom::TBottomViewKingdom(heroWindow* parent)
    : type_bottom_view_window(parent)
{
    int i;

    m_widgets.reserve(8);

    int id = BOTTOM_VIEW_BACKGROUND_ID;
    m_widgets.push_back(new bitmapBorder(
        0, 0, 176, 166, id++, "AdStatin.pcx", 0x800));

    int townCount[4];
    townCount[0] = 0;
    townCount[1] = 0;
    townCount[2] = 0;
    townCount[3] = 0;

    for (i = 0; i < g_currentPlayer->m_numTowns; i++) {
        town* which = g_game->getTown(g_currentPlayer->m_townIds[i]);
        if (which->hasBuilding(HALL_CAPITOL_ID, 1))
            townCount[3]++;
        else if (which->hasBuilding(HALL_CITY_ID, 1))
            townCount[2]++;
        else if (which->hasBuilding(HALL_TOWN_ID, 1))
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

    text = formatString("%s:", g_generalText->getText(391));
    m_widgets.push_back(new textWidget(10, 103, 57, 20, text.c_str(),
        "smalfont.fnt", font::WHITE, -1, 0, 0, 8));
    text = formatString("%s:", g_generalText->getText(392));
    m_widgets.push_back(new textWidget(10, 134, 57, 20, text.c_str(),
        "smalfont.fnt", font::WHITE, -1, 0, 0, 8));

    int allyX, enemyX;
    allyX = enemyX = 67;
    for (i = 0; i < 8; i++) {
        if (!g_game->m_playerDisabled[i]) {
            if (g_game->onSameTeam(i, g_unnamed69778c)) {
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

// UNBLOCKED by the constructor above - its 0x63bb3c store is the one
// image-wide reference to this class's table.
VA_COMPGEN(0x004531a0, 0x21, SCALAR_DELETING_DTOR, TBottomViewKingdom)

VA(0x004531d0, 0x78)  // dc 0x57020
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

VA(0x00453250, 0x3EE)  // dc 0x56880
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

    for (int i = 0; i < 8; i++) {
        if (g_game->m_playerDisabled[i] || g_game->m_players[i].isHuman())
            m_mobility[i] = 0;
        else
            m_mobility[i] = sumMobility(i);
    }

    if (g_currentPlayer->isHuman()) {
        m_widgets.push_back(new textWidget(0, 20, 176, 31,
            g_generalText->getText(631), "medfont.fnt", font::PRIMARY,
            -1, 1, 0, 8));
        m_widgets.push_back(new textWidget(0, 123, 176, 31,
            g_currentPlayer->m_name, "medfont.fnt", font::PRIMARY,
            -1, 1, 0, 8));
    }

    for (unsigned int w = 0; w < m_widgets.size(); w++)
        addWidget(m_widgets[w], -1);

    m_lastStepTime = GameTime::get();
}

// UNBLOCKED by the constructor above - its 0x63bb44 store is the one
// image-wide reference to this class's table.
VA_COMPGEN(0x00453640, 0x21, SCALAR_DELETING_DTOR, TBottomViewEnemyTurn)

VA_COMPGEN(0x00453970, 0xAE, CLASS_CTOR, basic_ostream)
VA_COMPGEN(0x00455820, 0x10B, CLASS_CTOR, numpunct)

VA(0x00453670, 0x78)  // dc 0x5706c
TBottomViewEnemyTurn::~TBottomViewEnemyTurn()
{
}

// E:\gamedcs\bottomviewsubwindow.cpp:646
// `player` IS A LOCAL, and it is worth 8.2 points. Retail pins
// &players[player_id].numHeroes in a register across the GetMobility
// call and walks heroes[] through a separate stack pointer; spelled
// `gpGame->players[player_id]` twice instead, our CL re-derives both
// addresses from gpGame every iteration. Same lever as the mouseX/mouseY
// and glTimers hoists - the value has to be a statement before the call.
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

VA(0x004536f0, 0x271)  // dc 0x56c14
void TBottomViewEnemyTurn::animate()
{
    unsigned long lastStep = m_lastStepTime;
    if (static_cast<long>(GameTime::get() - lastStep) < m_frameDelay)
        return;

    int numFrames = m_hourGlass->m_sprite->getNumFrames(0);

    m_lastStepTime = GameTime::get();

    if (m_lastPlayerPos != g_netLocalGamePos) {
        m_lastPlayerPos = g_netLocalGamePos;
        m_crest->setIconFrame(g_netLocalGamePos);
        m_crest->draw();
        m_crest->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_UPDATE);
        if (g_currentPlayer->isHuman()) {
            m_hourGlass->sendMessage(widget::WIDGET_CLEAR_STATUS,
                                    widget::WIDGET_DRAWN);
            m_sand->sendMessage(widget::WIDGET_CLEAR_STATUS,
                               widget::WIDGET_DRAWN);
            return;
        }
        m_hourGlass->sendMessage(widget::WIDGET_SET_STATUS,
                                widget::WIDGET_DRAWN);
        m_sand->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_DRAWN);
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
    if (m_frame >= m_sand->m_sprite->getNumFrames(0))
        m_frame = 0;

    m_hourGlass->setIconFrame(m_step);
    m_sand->setIconFrame(m_frame);
    m_sand->draw();
    m_hourGlass->draw();
    m_hourGlass->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_UPDATE);
    m_crest->setIconFrame(g_netLocalGamePos);
    m_crest->draw();
    m_crest->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_UPDATE);
}

// COMDAT pairing: basic_streambuf::1?$basic_streambuf, mnemonic agreement 1.000.
VA_COMPGEN(0x00454000, 0x50, IMPLICIT_DTOR, basic_streambuf)

// COMDAT pairing: numpunct::1?$numpunct, mnemonic agreement 0.970.
VA_COMPGEN(0x00455bf0, 0x2F, IMPLICIT_DTOR, numpunct)

// COMDAT pairing: basic_streambuf::0?$basic_streambuf, mnemonic agreement 0.937.
VA_COMPGEN(0x00453f50, 0xA9, CLASS_CTOR, basic_streambuf)

// COMDAT pairing: num_put::0?$num_put, mnemonic agreement 0.931.
VA_COMPGEN(0x004546e0, 0x5C, CLASS_CTOR, num_put)

// COMDAT pairing: sentry::1sentry, mnemonic agreement 0.911.
VA_COMPGEN(0x00454320, 0x63, IMPLICIT_DTOR, sentry)

// COMDAT pairing: fpos::0?$fpos, mnemonic agreement 0.909.
VA_COMPGEN(0x00454390, 0x3C, CLASS_CTOR, fpos)

// COMDAT pairing: _Iput on the char instantiation, mnemonic agreement 1.000.
VA_COMPGEN(0x004552d0, 0x3FE, NUM_PUT_IPUT, char)

// COMDAT pairing: _Fput on the char instantiation, mnemonic agreement 1.000.
VA_COMPGEN(0x00454f20, 0x3A1, NUM_PUT_FPUT, char)

// COMDAT pairing: xsgetn on the char instantiation, mnemonic agreement 0.946.
VA_COMPGEN(0x004540b0, 0x98, STREAMBUF_XSGETN, char)

// COMDAT pairing: _Rep on the char instantiation, mnemonic agreement 0.935.
VA_COMPGEN(0x004556d0, 0x82, NUM_PUT_REP, char)

// COMDAT pairing: opfx on the char instantiation, mnemonic agreement 0.926.
VA_COMPGEN(0x004543d0, 0x74, OSTREAM_OPFX, char)

// COMDAT pairing: sputc on the char instantiation, mnemonic agreement 0.932.
VA_COMPGEN(0x00454a30, 0x4B, STREAMBUF_SPUTC, char)

// COMDAT pairing: _Decref on the char instantiation, mnemonic agreement 0.918.
VA_COMPGEN(0x00453d30, 0x3B, LOCALE_FACET_DECREF, char)

// COMDAT pairing: num_put::do_put(bool), agreement 1.000. Six do_put
// overloads share one key and zip as an overload group; this arm and the
// const void* arm below have unique sizes and anchor both ends of the zip.
VA_COMPGEN(0x00454770, 0x292, NUM_PUT_DO_PUT, char)

// COMDAT pairing: num_put::do_put(long) - second in both COFF and RVA order.
VA_COMPGEN(0x00454a80, 0x9F, NUM_PUT_DO_PUT, char)

// COMDAT pairing: num_put::do_put(unsigned long).
VA_COMPGEN(0x00454b20, 0x9F, NUM_PUT_DO_PUT, char)

// COMDAT pairing: num_put::do_put(double).
VA_COMPGEN(0x00454bc0, 0xCA, NUM_PUT_DO_PUT, char)

// COMDAT pairing: num_put::do_put(long double).
VA_COMPGEN(0x00454c90, 0xCE, NUM_PUT_DO_PUT, char)

// COMDAT pairing: num_put::do_put(const void*), agreement 0.977.
VA_COMPGEN(0x00454d60, 0x1B2, NUM_PUT_DO_PUT, char)

// COMDAT pairing: bad_cast::_Doraise, agreement 1.000.
VA_COMPGEN(0x00453c80, 0x1D, EXCEPTION_DORAISE, bad_cast)

// The three destructor pairs this compiland's <iostream> surface leaves in
// the 0x453c70..0x455bc0 run, each one identified by the VTABLE its body
// stores rather than by similarity - every scalar deleting destructor in the
// image is the same fourteen-instruction shape.
//   0x453c70 stores 0x6457a8 and tail-jumps ??1exception. 0x6457a8 is
//     bad_cast's table: this unit's two bad_cast constructors (0x453cd0 and
//     0x454a10, both claimed above) are the only other writers of it.
//   0x453f40 stores 0x6456e0 and tail-jumps ??1ios_base, i.e. it is
//     basic_ios<char>'s. 0x6456e0 is the table basic_ostream's constructor
//     (0x453970) writes at `this+4`, which is where its basic_ios base sits.
//   0x4542f0 calls ??1basic_streambuf (0x454000) outright.
//   0x455bc0 calls ??1numpunct<char> (0x455bf0) outright.
// Each ??_G is the wrapper over the ??1 immediately named, in the
// flags&1 / operator delete form.
VA_COMPGEN(0x00453c70, 0xB, IMPLICIT_DTOR, bad_cast)
VA_COMPGEN(0x00453ca0, 0x21, SCALAR_DELETING_DTOR, bad_cast)
VA_COMPGEN(0x00453f40, 0xB, IMPLICIT_DTOR, basic_ios)
VA_COMPGEN(0x004542c0, 0x21, SCALAR_DELETING_DTOR, basic_ios)
VA_COMPGEN(0x004542f0, 0x21, SCALAR_DELETING_DTOR, basic_streambuf)
VA_COMPGEN(0x00455bc0, 0x21, SCALAR_DELETING_DTOR, numpunct)

// COMDAT pairing: bad_cast::bad_cast(const&), agreement 0.857 against a 32 B
// object; resourcemanager's two 19 B candidates score 0.762.
VA_COMPGEN(0x00453cd0, 0x1C, CLASS_CTOR, bad_cast)

// COMDAT pairing: ios_base::getloc, agreement 0.906 against a 64 B object;
// quicktownwindow's 28 B candidate scores 0.750.
VA_COMPGEN(0x00453d90, 0x3B, IOS_BASE_GETLOC, char)

// COMDAT pairing: bad_cast::bad_cast(const char*), the second half of the
// ctor overload group. COFF order is (const&, const char*) and RVA order is
// 0x53cd0, 0x54a10; the similarity agrees with that zip in both directions
// (0.857 each way, 0.786 crossed).
VA_COMPGEN(0x00454a10, 0x1C, CLASS_CTOR, bad_cast)

VA_COMPGEN(0x00455930, 0x4, NUMPUNCT_DO_DECIMAL_POINT, char)
VA_COMPGEN(0x00455940, 0x4, NUMPUNCT_DO_THOUSANDS_SEP, char)

VA_COMPGEN(0x00455950, 0xCF, NUMPUNCT_DO_GROUPING, char)
VA_COMPGEN(0x00455a20, 0xCF, NUMPUNCT_DO_FALSENAME, char)
VA_COMPGEN(0x00455af0, 0xCF, NUMPUNCT_DO_TRUENAME, char)

VA_COMPGEN(0x00455760, 0x1E, NUMPUNCT_FALSENAME, char)
VA_COMPGEN(0x00455780, 0x1E, NUMPUNCT_TRUENAME, char)
VA_COMPGEN(0x00455800, 0x1E, NUMPUNCT_GROUPING, char)

// COMDAT pairing: _Tidyfac's two instantiations. The object emits exactly
// four such COMDATs and the image has exactly four candidate bodies; the
// static `_Ptr` each pair shares is the discriminator. 0x55c20 stores into
// bss_294d9c and 0x55d20 reads and clears it; 0x55ca0/0x55dc0 do the same
// with bss_294da0. Which static belongs to which facet is settled by the
// constructors: 0x53a30 calls the claimed num_put<char> ctor at 0x546e0 and
// then 0x55c20, while 0x11ab90 installs numpunct<char>'s vtbl_245728 and
// then stores into bss_294da0.
VA_COMPGEN(0x00455c20, 0x7B, TIDYFAC_NUM_PUT_SAVE, char)
VA_COMPGEN(0x00455ca0, 0x7B, TIDYFAC_NUMPUNCT_SAVE, char)
VA_COMPGEN(0x00455d20, 0x92, TIDYFAC_NUM_PUT_TIDY, char)
VA_COMPGEN(0x00455dc0, 0x92, TIDYFAC_NUMPUNCT_TIDY, char)

// COMDAT pairing: basic_streambuf<char>::uflow. Not reached by any rel32 call
// at all - it is installed as slot 5 of FOUR vtables, one of them the named
// ??_7TGzInflateBuf@@6B@, which is exactly what a defaulted virtual of the
// stream base looks like.
VA_COMPGEN(0x00454080, 0x2B, STREAMBUF_UFLOW, char)

// COMDAT pairing: ostreambuf_iterator<char>::operator=(char), reached from
// two CRT sites and three of this unit's own - cross-image reach.
VA_COMPGEN(0x004557a0, 0x5C, OSTREAMBUF_ITERATOR_ASSIGN, char)

VA_COMPGEN(0x00453a20, 0xF, IMPLICIT_DTOR, basic_ostream)

// COMDAT pairing: basic_ostream<char>::operator<<(int), agreement 0.960 at
// an exactly equal 568-byte extent - the MEMBER overload, distinct from the
// free operator<<(const char*) already claimed elsewhere.
VA_COMPGEN(0x00453a30, 0x238, OSTREAM_INSERT_INT, char)

// COMDAT pairing: locale::id::operator size_t, agreement 0.905, and
// locale::~locale, agreement 1.000. Both are 1:1 in this object.
VA_COMPGEN(0x00453cf0, 0x32, LOCALE_ID_CAST, char)
VA_COMPGEN(0x00453d70, 0x18, IMPLICIT_DTOR, locale)

// COMDAT pairing: basic_streambuf<char>::seekoff and ::seekpos - the BASE
// class defaults, both 40 bytes, distinct from basic_stringbuf's overrides
// already claimed in resourcemanager. Agreements 1.000 and 1.000.
VA_COMPGEN(0x00454200, 0x28, STREAMBUF_SEEKOFF, char)
VA_COMPGEN(0x00454230, 0x28, STREAMBUF_SEEKPOS, char)

VA_COMPGEN(0x00454740, 0x23, SCALAR_DELETING_DTOR, facet)

VA_COMPGEN(0x00454050, 0x6, STREAMBUF_OVERFLOW, char)
VA_COMPGEN(0x00454060, 0x3, STREAMBUF_SHOWMANYC, char)
VA_COMPGEN(0x00454070, 0x4, STREAMBUF_UNDERFLOW, char)
VA_COMPGEN(0x00454260, 0x5, STREAMBUF_SETBUF, char)
VA_COMPGEN(0x00454270, 0x3, STREAMBUF_IMBUE, char)

// COMDAT pairing: basic_ostream<char>'s scalar deleting destructor,
// agreement 0.944 - its 15-byte ??1 twin is already claimed at 0x453a20.
VA_COMPGEN(0x00454280, 0x32, SCALAR_DELETING_DTOR, basic_ostream)
