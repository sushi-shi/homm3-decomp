// combatcontrolsubwindow.cpp - E:\gamedcs\combatcontrolsubwindow.cpp (compiland combatcontrolsubwindow.obj)
#include <va.h>
#include "combatcontrolsubwindow.h"
#include "border.h"
#include "button.h"
#include "combatwindow.h"
#include "cmbtmgr.h"
#include "game.h"
#include "iconwdgt.h"
#include "inputmgr.h"
#include "homm3_minmax.h"
#include "kb.h"
#include "textresource.h"
#include "textwdgt.h"
#include "widget.h"
#include "window.h"
#include "winmgr.h"

// The combat sub-window family's help-text table. Eleven rows, and every
// one of them is referenced from exactly one of this compiland's three
// reconstructed-or-not constructors: the base 0x46b610 takes rows 0..3 and
// 6..8, TCombatControlSubWindow rows 4 and 5, TCombatPlacementSubWindow
// rows 9 and 10. The START is the lowest row any of them names; nothing in
// the image reads the two dwords below it, so the table could in principle
// begin earlier, and the eleven-row extent is a floor rather than a
// measured end.
DATA(0x006a6968) extern HelpText g_combatSubWindowHelp[11];

#if 0  // @carcass

// E:\gamedcs\combatcontrolsubwindow.cpp:44
DC_ONLY(0x64a84, 0x468)
void type_combat_sub_window::type_combat_sub_window(heroWindow* parent, const char* background_sprite_name)
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:122
DC_ONLY(0x64eec, 0x74)
void type_combat_sub_window::~type_combat_sub_window()
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:148
DC_ONLY(0x64f68, 0x64)
void type_combat_sub_window::disableAllButtons()
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:177
DC_ONLY(0x64fcc, 0x278)
void CombatControlSubWindow::CombatControlSubWindow(heroWindow* parent)
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:222
DC_ONLY(0x65244, 0x2C)
void CombatControlSubWindow::~CombatControlSubWindow()
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:249
DC_ONLY(0x65274, 0x24)
void CombatControlSubWindow::setRollover(const char* new_text)
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:282
DC_ONLY(0x652a8, 0x1A4)
void CombatPlacementSubWindow::CombatPlacementSubWindow(heroWindow* parent)
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:317
DC_ONLY(0x6544c, 0x2C)
void CombatPlacementSubWindow::~CombatPlacementSubWindow()
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:327
DC_ONLY(0x65478, 0x28)
void CombatPlacementSubWindow::disableAllButtons()
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:343
DC_ONLY(0x654a0, 0x638)
void CombatHeroSubWindow::CombatHeroSubWindow(int x, int y, int w, int h, heroWindow* parent)
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:407
DC_ONLY(0x65ad8, 0x68)
void CombatHeroSubWindow::~CombatHeroSubWindow()
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:428
DC_ONLY(0x65b40, 0x144)
void CombatHeroSubWindow::update(const Hero* info, const Hero* otherHero, unsigned char on_cursed_ground)
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:472
#endif  // @carcass

// It is an 800x44 strip at (0, 556) with a ten-slot reserve, a
// bitmapBorder over the caller's sprite and SEVEN buttons -
// 'icm001.def'..'icm007.def', all 48x36 on row 5, ids 0x7d1..0x7d4 then
// 0x7d8..0x7da. The four-id gap is real: 0x7d5/0x7d6/0x7d7 belong to
// TCombatControlSubWindow's rollover strip and its two log arrows, which
// is why these are seven literal ids and not one incrementing local.
// The help rows are 0..3 and 6..8 of the same eleven-row table, leaving
// 4/5 to the control bar and 9/10 to the placement bar - the table's
// three consumers partition it exactly, which is the second witness for
// the table's extent.

// rolloverWidget IS NULLED HERE, IN THE BODY - the 0x63d410 vptr store
// precedes it, so it is not a mem-init - and that is what makes the
// member the base's rather than TCombatControlSubWindow's.

// The tail dims 0x7d4 through the PARENT window rather than through this
// sub-window, and only when both sides are AI.

VA(0x0046b610, 0x570)  // dc 0x64a84
type_combat_sub_window::type_combat_sub_window(
    heroWindow* parent, const char* backgroundSpriteName)
    : SubWindow(0, 556, 800, 44, parent)
{
    m_rolloverWidget = 0;
    m_widgets.reserve(10);

    bitmapBorder* background = new bitmapBorder(0, 0, 800, 44, 0x7d0,
        backgroundSpriteName, 0x800);
    int gamePos = g_combatManager->m_playerIds[g_combatManager->m_currentSide];
    if (gamePos < 0)
        gamePos = g_game->getLocalPlayerGamePos();
    background->setPlayerPaletteColors(gamePos);
    m_widgets.push_back(background);

    // The hotkeys are scancodes: S, R, O, A, C, W, then D and SPACE.
    button* b = new button(54, 5, 48, 36, 0x7d1, "icm001.def",
        0, 1, 0, 0, 2);
    b->setHelpText(g_combatSubWindowHelp[0].m_text,
        g_combatSubWindowHelp[0].m_rclick, 1);
    b->setHotkey(0x1f);
    m_widgets.push_back(b);

    b = new button(105, 5, 48, 36, 0x7d2, "icm002.def", 0, 1, 0, 0, 2);
    b->setHelpText(g_combatSubWindowHelp[1].m_text,
        g_combatSubWindowHelp[1].m_rclick, 1);
    b->setHotkey(0x13);
    m_widgets.push_back(b);

    b = new button(3, 5, 48, 36, 0x7d3, "icm003.def", 0, 1, 0, 0, 2);
    b->setHelpText(g_combatSubWindowHelp[2].m_text,
        g_combatSubWindowHelp[2].m_rclick, 1);
    b->setHotkey(0x18);
    m_widgets.push_back(b);

    b = new button(156, 5, 48, 36, 0x7d4, "icm004.def", 0, 1, 0, 0, 2);
    b->setHelpText(g_combatSubWindowHelp[3].m_text,
        g_combatSubWindowHelp[3].m_rclick, 1);
    b->setHotkey(0x1e);
    m_widgets.push_back(b);

    b = new button(645, 5, 48, 36, 0x7d8, "icm005.def", 0, 1, 0, 0, 2);
    b->setHelpText(g_combatSubWindowHelp[6].m_text,
        g_combatSubWindowHelp[6].m_rclick, 1);
    b->setHotkey(0x2e);
    m_widgets.push_back(b);

    b = new button(696, 5, 48, 36, 0x7d9, "icm006.def", 0, 1, 0, 0, 2);
    b->setHelpText(g_combatSubWindowHelp[7].m_text,
        g_combatSubWindowHelp[7].m_rclick, 1);
    b->setHotkey(0x11);
    m_widgets.push_back(b);

    b = new button(747, 5, 48, 36, 0x7da, "icm007.def", 0, 1, 0, 0, 2);
    b->setHelpText(g_combatSubWindowHelp[8].m_text,
        g_combatSubWindowHelp[8].m_rclick, 1);
    b->setHotkey(0x20);
    b->setHotkey(0x39);
    m_widgets.push_back(b);

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }

    if (g_combatManager->m_sideIsAi[0] && g_combatManager->m_sideIsAi[1])
        parent->widgetSetStatus(0x7d4, widget::WIDGET_DIMMED_NODRAW);
}

VA_COMPGEN(0x0046bb80, 0x21, SCALAR_DELETING_DTOR, type_combat_sub_window)

VA(0x0046bbb0, 0x78)  // dc 0x64eec
type_combat_sub_window::~type_combat_sub_window()
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

// The combat control bar: the rollover strip and the two message-log
// arrows. Same family shape as the placement bar below - an out-of-line
// base call, a LOCAL vector<widget*> that the tail loop drains into the
// inherited Widgets, and a bare operator delete for the local's storage -
// with three differences that the bytes fix.
//
// THE TAIL LOOP PUSHES UNCONDITIONALLY AND ONLY GUARDS AddWidget. Retail
// hands push_back the iterator itself (`push edi`, no copy through a
// local) and only then tests the element, where the placement bar's loop
// wraps both calls in one `if`. Two loops, two shapes, both transcribed.
//
// rolloverWidget IS THE BASE'S MEMBER AT +0x34 and its declared type is a
// bitmapBackedTextWidget*: retail reloads it out of +0x34 into a stack
// temporary for push_back, which only the derived-to-widget* conversion
// can explain. Same test as TBottomViewNewTurn::backdrop.
//
// The two arrows' handlers are ADDRESS-TAKEN ONLY. They were absent from the
// original carve but are now promoted at 0x472df0/0x472e40 in combatwindow.obj
// under their Dreamcast-attested static TCombatWindow names.
//
// Exact after restoring the canonical setDisabledFrame calls. DC191
// names the setter, and DC199 calls the same retained r10 address. The
// 24-state setter/max family emits sixteen objects; all ten retained elites
// reproduce. The up-arrow setter alone closes 96.3990 to 100; restoring only
// the down-arrow setter is byte-flat. Both proven source calls are retained.
// The old direct stores had changed scheduling inside the adjacent setHotkey
// expansions. Moving those stores before/after the hotkeys was a failed
// hypothesis (90.91/92.72), not proof that the setter boundary was irrelevant.
// Original vector local new_widgets; its scope and lifetime are unchanged.

// DC136/140 and their retained publics prove these empty virtual methods.
// Retail base-table slots 1/2 fold to 0x485d80 (ret 4) and 0x5bc7e0
// (ret 8). Keep the bodies and original long arguments (JJ), despite
// the generated carcass prototypes having lost both parameters.
DC_ONLY(0x64f60, 0x4)
void type_combat_sub_window::setRollover(const char*)
{
}

DC_ONLY(0x64f64, 0x4)
void type_combat_sub_window::setRolloverButtons(long, long)
{
}

VA(0x0046bc30, 0x26D)  // dc 0x64fcc
CombatControlSubWindow::CombatControlSubWindow(heroWindow* parent)
    : type_combat_sub_window(parent, "cbar.pcx")
{
    std::vector<widget*> newWidgets;

    m_rolloverWidget = new bitmapBackedTextWidget(214, 7, 400, 32, "",
        "smalfont.fnt", "cRollovr.pcx", font::PRIMARY, 0x7d5, 1, 8);
    m_rolloverWidget->setHelpText(g_combatSubWindowHelp[4].m_text,
        g_combatSubWindowHelp[4].m_rclick, 1);
    newWidgets.push_back(m_rolloverWidget);

    m_logScrollUpButton = new type_func_button(624, 5, 18, 17, 0x7d6,
        "ComSlide.def", CombatWindow::scrollUp, 0, 1);
    m_logScrollUpButton->setHelpText(g_combatSubWindowHelp[5].m_text,
        g_combatSubWindowHelp[5].m_rclick, 1);
    m_logScrollUpButton->setDisabledFrame(1);
    m_logScrollUpButton->setHotkey(KEYCODE_KP_8);
    newWidgets.push_back(m_logScrollUpButton);

    m_logScrollDownButton = new type_func_button(624, 24, 18, 17, 0x7d7,
        "ComSlide.def", CombatWindow::scrollDown, 2, 3);
    m_logScrollDownButton->setHelpText(g_combatSubWindowHelp[5].m_text,
        g_combatSubWindowHelp[5].m_rclick, 1);
    m_logScrollDownButton->setHotkey(KEYCODE_KP_2);
    m_logScrollDownButton->setDisabledFrame(3);
    newWidgets.push_back(m_logScrollDownButton);

    for (widget** it = newWidgets.begin(); it != newWidgets.end(); ++it) {
        m_widgets.push_back(*it);
        if (*it)
            addWidget(*it, -1);
    }

    m_logScrollUpButton->sendMessage(widget::WIDGET_SET_STATUS,
        widget::WIDGET_DIMMED);
    m_logScrollDownButton->sendMessage(widget::WIDGET_SET_STATUS,
        widget::WIDGET_DIMMED);
}

// UNBLOCKED by the constructor above: its 0x63d420 store is the only
// image-wide reference to this class's table.
VA_COMPGEN(0x0046bea0, 0x21, SCALAR_DELETING_DTOR, TCombatControlSubWindow)

VA(0x0046bed0, 0x78)  // dc 0x65244
CombatControlSubWindow::~CombatControlSubWindow()
{
}

// DC227..246 retains an empty derived override with the same JJ ABI.
// Retail control-table slot 2 shares the base method's ret-8 fold.
DC_ONLY(0x65270, 0x4)
void CombatControlSubWindow::setRolloverButtons(long, long)
{
}

// E:\gamedcs\combatcontrolsubwindow.cpp:249
VA(0x0046bf50, 0x32)  // vtable 0x63d420 slot 1 + rollover widget at +0x34, dc 0x65274
void CombatControlSubWindow::setRollover(const char* newText)
{
    m_rolloverWidget->setText(newText);
    m_rolloverWidget->sendMessage(widget::WIDGET_DRAW, 0);
    m_rolloverWidget->sendMessage(widget::WIDGET_SET_STATUS,
                                 widget::WIDGET_UPDATE);
}

VA(0x0046bf90, 0xB2)  // dc 0x64f68
void type_combat_sub_window::disableAllButtons()
{
    m_parentWindow->widgetSetStatus(0x7d1, widget::WIDGET_DIMMED_NODRAW);
    m_parentWindow->widgetSetStatus(0x7d2, widget::WIDGET_DIMMED_NODRAW);
    m_parentWindow->widgetSetStatus(0x7d3, widget::WIDGET_DIMMED_NODRAW);
    m_parentWindow->widgetSetStatus(0x7d4, widget::WIDGET_DIMMED_NODRAW);
    m_parentWindow->widgetSetStatus(0x7d8, widget::WIDGET_DIMMED_NODRAW);
    m_parentWindow->widgetSetStatus(0x7d9, widget::WIDGET_DIMMED_NODRAW);
    m_parentWindow->widgetSetStatus(0x7da, widget::WIDGET_DIMMED_NODRAW);
    m_parentWindow->drawWindow(0, WINDOW_ALL_WIDGETS_LOW,
                             WINDOW_ALL_WIDGETS_HIGH);
    g_windowManager->updateScreen(m_x, m_y, m_width, m_height);
}

// DC271 calls the ordinary base implementation. Retail control-table
// slot 3 shares its 0x46bf90 body after the call expands and ICF folds it.
// Retain the override and canonical source call instead of only inheriting
// the base slot. The base body is visible here, as in the original TU.
DC_ONLY(0x65298, 0x10)
void CombatControlSubWindow::disableAllButtons()
{
    type_combat_sub_window::disableAllButtons();
}

// The battlefield-placement bar: two buttons over the family base, and
// the smallest of the family's constructors. Three things in it are read
// off the bytes rather than assumed.

// THE BASE IS AN OUT-OF-LINE CALL, not an inlined body - `call 0x46b610`
// with (parent, "CoPlacbr.pcx") on the stack - so this constructor needs
// nothing from the base's 1392 bytes and lands without them.

VA(0x0046c050, 0x18C)  // dc 0x65310
CombatPlacementSubWindow::CombatPlacementSubWindow(heroWindow* parent)
    : type_combat_sub_window(parent, "CoPlacbr.pcx")
{
    std::vector<widget*> buttons;

    // 0x8fc has no attested name; 0x7802 is winmgr.h's DIALOG_RETURN_OK
    // value, but nothing here proves this id is that domain, so both stay
    // literal. The hotkeys are the SPACE and ENTER scancodes.
    widget* b = new button(213, 4, 198, 36, 0x8fc, "ICM011.def",
        0, 1, 0, 0x39, 2);
    b->setHelpText(g_combatSubWindowHelp[9].m_text,
        g_combatSubWindowHelp[9].m_rclick, 1);
    buttons.push_back(b);

    b = new button(419, 4, 198, 36, 0x7802, "ICM012.def",
        0, 1, 0, 0x1c, 2);
    b->setHelpText(g_combatSubWindowHelp[10].m_text,
        g_combatSubWindowHelp[10].m_rclick, 1);
    buttons.push_back(b);

    for (widget** it = buttons.begin(); it != buttons.end(); ++it) {
        widget* w = *it;
        if (w) {
            m_widgets.push_back(w);
            addWidget(w, -1);
        }
    }
}

VA_COMPGEN(0x0046c1e0, 0x21, SCALAR_DELETING_DTOR, TCombatPlacementSubWindow)

VA(0x0046c210, 0x78)  // dc 0x6544c
CombatPlacementSubWindow::~CombatPlacementSubWindow()
{
}

VA(0x0046c290, 0xD6)  // dc 0x65478
void CombatPlacementSubWindow::disableAllButtons()
{
    m_parentWindow->widgetSetStatus(0x8fc, widget::WIDGET_DIMMED_NODRAW);
    m_parentWindow->widgetSetStatus(0x7802, widget::WIDGET_DIMMED_NODRAW);
    type_combat_sub_window::disableAllButtons();
}

VA(0x0046c370, 0x7FC)  // dc 0x654a0
CombatHeroSubWindow::CombatHeroSubWindow(
    int x, int y, int w, int h, heroWindow* parent)
    : SubWindow(x, y, w, h, parent)
{
    m_widgets.reserve(12);

    m_backgroundWidget = new bitmapBorder(
        0, 0, 78, 202, 0x834, "CHrPop.pcx", 0x800);
    m_widgets.push_back(m_backgroundWidget);
    m_portrait = new bitmapBorder(10, 6, 58, 64, 0x835, 0, 0x800);
    m_widgets.push_back(m_portrait);

    sprintf(g_text, "%s:", (*g_generalText)[381]);
    m_widgets.push_back(new textWidget(
        9, 75, 60, 12, g_text, "tiny.fnt", font::WHITE,
        0x836, 0, 0, 8));
    m_attackText = new textWidget(
        9, 75, 60, 12, 0, "tiny.fnt", font::WHITE,
        0x837, 2, 0, 8);
    m_widgets.push_back(m_attackText);

    sprintf(g_text, "%s:", (*g_generalText)[382]);
    m_widgets.push_back(new textWidget(
        9, 87, 60, 12, g_text, "tiny.fnt", font::WHITE,
        0x838, 0, 0, 8));
    m_defenseText = new textWidget(
        9, 87, 60, 12, 0, "tiny.fnt", font::WHITE,
        0x839, 2, 0, 8);
    m_widgets.push_back(m_defenseText);

    sprintf(g_text, "%s:", (*g_generalText)[383]);
    m_widgets.push_back(new textWidget(
        9, 99, 60, 12, g_text, "tiny.fnt", font::WHITE,
        0x83a, 0, 0, 8));
    m_powerText = new textWidget(
        9, 99, 60, 12, 0, "tiny.fnt", font::WHITE,
        0x83b, 2, 0, 8);
    m_widgets.push_back(m_powerText);

    sprintf(g_text, "%s:", (*g_generalText)[384]);
    m_widgets.push_back(new textWidget(
        9, 111, 60, 12, g_text, "tiny.fnt", font::WHITE,
        0x83c, 0, 0, 8));
    m_knowledgeText = new textWidget(
        9, 111, 60, 12, 0, "tiny.fnt", font::WHITE,
        0x83d, 2, 0, 8);
    m_widgets.push_back(m_knowledgeText);

    sprintf(g_text, "%s:", (*g_generalText)[385]);
    m_widgets.push_back(new textWidget(
        9, 131, 60, 12, g_text, "tiny.fnt", font::PRIMARY,
        0x83e, 0, 0, 8));
    m_moraleIcon = new iconWidget(
        47, 131, 22, 12, 0x83f, "imrls.def", 0, 0, 0, 0,
        iconWidget::ICON_STYLE_PLAIN);
    m_widgets.push_back(m_moraleIcon);

    sprintf(g_text, "%s:", (*g_generalText)[386]);
    m_widgets.push_back(new textWidget(
        9, 143, 60, 12, g_text, "tiny.fnt", font::PRIMARY,
        0x840, 0, 0, 8));
    m_luckIcon = new iconWidget(
        47, 143, 22, 12, 0x841, "ilcks.def", 0, 0, 0, 0,
        iconWidget::ICON_STYLE_PLAIN);
    m_widgets.push_back(m_luckIcon);

    m_manaText = new textWidget(
        7, 165, 64, 30, 0, "tiny.fnt", font::WHITE,
        0x842, 5, 0, 8);
    m_widgets.push_back(m_manaText);

    for (std::vector<widget*>::iterator current = m_widgets.begin();
         current != m_widgets.end(); ++current) {
        widget* w = *current;
        if (w)
            w->m_status &= ~(widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        addWidget(w, -1);
    }

    m_shown = false;
}

// TCombatHeroSubWindow's own destructor is 107 bytes, not 120, and the
// 13-byte difference is one missing call: its loop deletes each widget
// but does NOT hand it to parentWindow->RemoveWidget first. That also
// explains why this body DOES name its own table where the three above
// do not - it derives straight from TSubWindow, whose destructor is an
// out-of-line call rather than an inlined body, so the 0x63d440 store
// survives. The surviving reference is what lets the wrapper be claimed
// with it, exactly as for the family base.

VA_COMPGEN(0x0046cb70, 0x21, SCALAR_DELETING_DTOR, TCombatHeroSubWindow)

VA(0x0046cba0, 0x6B)  // dc 0x65ad8
CombatHeroSubWindow::~CombatHeroSubWindow()
{
    for (std::vector<widget*>::iterator it = m_widgets.begin();
         it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

VA(0x0046cc10, 0x1D7)  // dc 0x65b40
void CombatHeroSubWindow::update(const Hero& info, const Hero* otherHero,
                                  bool onCursedGround)
{
    char buffer[64];
    Hero& mutableInfo = const_cast<Hero&>(info);

    m_backgroundWidget->setPlayerPaletteColors(info.m_owner);
    m_portrait->setImage(g_heroTraits[info.m_portrait].m_largePortraitName);

    sprintf(buffer, "%d", info.getPrimarySkill(0));
    m_attackText->setText(buffer);
    sprintf(buffer, "%d", info.getPrimarySkill(1));
    m_defenseText->setText(buffer);
    sprintf(buffer, "%d", info.getPrimarySkill(2));
    m_powerText->setText(buffer);
    sprintf(buffer, "%d", info.getPrimarySkill(3));
    m_knowledgeText->setText(buffer);

    m_moraleIcon->setIconFrame(
        mutableInfo.getMorale(otherHero, onCursedGround, 1) + 3);
    m_luckIcon->setIconFrame(
        mutableInfo.getLuck(otherHero, onCursedGround, 1) + 3);

    sprintf(buffer, "%s\n%d/%d", (*g_generalText)[388], info.m_mana,
            mutableInfo.getMaxMana());
    m_manaText->setText(buffer);
}

VA(0x0046cdf0, 0x74)  // dc 0x65c84
void CombatHeroSubWindow::show()
{
    if (!m_shown) {
        saveBackground();
        for (std::vector<widget*>::iterator current = m_widgets.begin();
             current != m_widgets.end(); ++current) {
            if (*current)
                (*current)->m_status |= widget::WIDGET_ACTIVE |
                                      widget::WIDGET_DRAWN;
        }
        draw(0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        g_windowManager->updateScreen(
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width + 1, m_height);
        m_shown = true;
    }
}

VA(0x0046ce70, 0x3A)  // dc 0x65cf8
void CombatHeroSubWindow::unShow()
{
    if (m_shown) {
        for (std::vector<widget*>::iterator current = m_widgets.begin();
             current != m_widgets.end(); ++current) {
            if (*current)
                (*current)->m_status &= ~(widget::WIDGET_ACTIVE |
                                        widget::WIDGET_DRAWN);
        }
        restoreBackground();
        m_shown = false;
    }
}

// The compiland's last class repeats the pattern one more time, and the
// carve continues to agree with the roster row for row:

//   0x46ceb0  0xcd1  TCombatCreatureSubWindow::ctor              562
//   0x46db90  0x21   TCombatCreatureSubWindow::`scalar del dtor' 655
//   0x46dbc0  0x6b   TCombatCreatureSubWindow::~                 666
//   0x46dc30  0x2c2  TCombatCreatureSubWindow::Update            688
//   0x46df00  0x73   TCombatCreatureSubWindow::Show              773
//   0x46df80  0x3a   TCombatCreatureSubWindow::UnShow            820

// and 0x46df80 is the entry combatManager::DoCommand already proved to
// be this class's UnShow, so the run is pinned at both ends. The
// destructor is TCombatHeroSubWindow's 107 bytes exactly - the same
// delete-only loop over Widgets - differing only in the table it stores:
// 0x63d444, four bytes past 0x63d440, so both classes carry a one-slot
// table holding nothing but the destructor.

// E:\gamedcs\combatcontrolsubwindow.cpp:562
// The DC lexical map proves the two view-level arms and their common
// spellText append. Retail fixes the Complete-only compact-stat labels,
// every widget argument, the three-iteration standing-spell loops, and the
// full derived layout at +0x34..+0x6c.
// Exact with named text-resource arguments at DC573/578/583/588/593/598.
// The local names are inferred; DC records the operator[] then sprintf
// order, but no local names. Capturing each caption before formatting
// preserves that order and all allocation/member/push_back boundaries.
// All 130 retail blocks and 105 positional calls agree (STL empty-body,
// pointer-size and pointer/int _Ufill names are existing ICF aliases).
//
// Negative controls: the original direct resource arguments retain one
// extra vector::size call in the compact spell-icon append (99.1290%).
// A 64-state argument-capture family emitted seven objects, all reproduced:
// five or six named arguments recover the exact expansion, fewer do not.
// The consistent six-caption form is retained. A separate 64-state typed
// caption-result family and 47-state scope follow-up reach only 99.9957%:
// they restore that expansion but put allocation temporaries at EBP+1c
// instead of retail EBP+18. Their 64 and 17 objects are not adopted.
// Earlier 54-state compact conversion/loop-birth controls also leave the
// size call unresolved. Folding DC-separated member assignments into
// push_back loses the exact reserve/full-arm lowering (97.27%); keep the
// canonical vector interface and both proven loop bodies.
VA(0x0046ceb0, 0xCD1)  // roster order + vtable 0x63d444 + CCrPop/SpellInf, dc 0x65dbc
CombatCreatureSubWindow::CombatCreatureSubWindow(
    int x, int y, int w, int h, heroWindow* parent, int viewLevel)
    : SubWindow(x, y, w, h, parent), m_viewLevel(viewLevel)
{
    m_widgets.reserve(11);

    if (viewLevel == 1) {
        m_backgroundWidget = new bitmapBorder(
            0, 0, 78, 288, 0x898, "CCrPop.pcx", 0x800);
        m_widgets.push_back(m_backgroundWidget);
        m_creatureIcon = new iconWidget(
            10, 6, 58, 64, 0x899, "TwCrPort.def", 0, 0, 0, 0,
            iconWidget::ICON_STYLE_PLAIN);
        m_widgets.push_back(m_creatureIcon);

        const char* attackName = (*g_generalText)[381];
        sprintf(g_text, "%s:", attackName);
        m_widgets.push_back(new textWidget(
            9, 75, 60, 12, g_text, "tiny.fnt", font::WHITE,
            0x89a, 0, 0, 8));
        m_attackText = new textWidget(
            9, 75, 60, 12, 0, "tiny.fnt", font::WHITE,
            0x89b, 2, 0, 8);
        m_widgets.push_back(m_attackText);

        const char* defenseName = (*g_generalText)[382];
        sprintf(g_text, "%s:", defenseName);
        m_widgets.push_back(new textWidget(
            9, 87, 60, 12, g_text, "tiny.fnt", font::WHITE,
            0x89c, 0, 0, 8));
        m_defenseText = new textWidget(
            9, 87, 60, 12, 0, "tiny.fnt", font::WHITE,
            0x89d, 2, 0, 8);
        m_widgets.push_back(m_defenseText);

        const char* damageName = (*g_generalText)[387];
        sprintf(g_text, "%s:", damageName);
        m_widgets.push_back(new textWidget(
            9, 99, 60, 12, g_text, "tiny.fnt", font::WHITE,
            0x89e, 0, 0, 8));
        m_damageText = new textWidget(
            9, 99, 60, 12, 0, "tiny.fnt", font::WHITE,
            0x89f, 2, 0, 8);
        m_widgets.push_back(m_damageText);

        const char* speedName = (*g_generalText)[390];
        sprintf(g_text, "%s:", speedName);
        m_widgets.push_back(new textWidget(
            9, 111, 60, 12, g_text, "tiny.fnt", font::WHITE,
            0x8a0, 0, 0, 8));
        m_speedText = new textWidget(
            9, 111, 60, 12, 0, "tiny.fnt", font::WHITE,
            0x8a1, 2, 0, 8);
        m_widgets.push_back(m_speedText);

        const char* moraleName = (*g_generalText)[385];
        sprintf(g_text, "%s:", moraleName);
        m_widgets.push_back(new textWidget(
            9, 131, 60, 12, g_text, "tiny.fnt", font::WHITE,
            0x8a2, 0, 0, 8));
        m_moraleIcon = new iconWidget(
            47, 131, 22, 12, 0x8a3, "imrls.def", 0, 0, 0, 0,
            iconWidget::ICON_STYLE_PLAIN);
        m_widgets.push_back(m_moraleIcon);

        const char* luckName = (*g_generalText)[386];
        sprintf(g_text, "%s:", luckName);
        m_widgets.push_back(new textWidget(
            9, 143, 60, 12, g_text, "tiny.fnt", font::WHITE,
            0x8a4, 0, 0, 8));
        m_luckIcon = new iconWidget(
            47, 143, 22, 12, 0x8a5, "ilcks.def", 0, 0, 0, 0,
            iconWidget::ICON_STYLE_PLAIN);
        m_widgets.push_back(m_luckIcon);

        m_countText = new textWidget(
            10, 8, 58, 64, 0, "Verd10B.fnt", font::WHITE,
            0x8a6, 10, 0, 8);
        m_widgets.push_back(m_countText);

        int spellY = 169;
        for (int i = 0; i < 3; ++i) {
            m_spellIcons[i] = new iconWidget(
                15, spellY, 48, 36, 0x8a7 + i, "spellint.def",
                0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN);
            m_widgets.push_back(m_spellIcons[i]);
            spellY += 38;
        }
        m_spellText = new textWidget(
            15, 169, 48, 36, g_emptyRolloverText, "tiny.fnt", font::PRIMARY,
            0x8aa, 1, 0, 8);
    } else {
        m_backgroundWidget = new bitmapBorder(
            0, 0, 78, 126, 0x898, "SpellInf.pcx", 0x800);
        m_widgets.push_back(m_backgroundWidget);

        int spellY = 7;
        for (int i = 0; i < 3; ++i) {
            m_spellIcons[i] = new iconWidget(
                15, spellY, 48, 36, 0x8a7 + i, "spellint.def",
                0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN);
            m_widgets.push_back(m_spellIcons[i]);
            spellY += 38;
        }
        m_spellText = new textWidget(
            15, 7, 48, 36, g_emptyRolloverText, "tiny.fnt", font::PRIMARY,
            0x8aa, 1, 0, 8);
    }

    m_widgets.push_back(m_spellText);
    for (std::vector<widget*>::iterator current = m_widgets.begin();
         current != m_widgets.end(); ++current) {
        widget* w = *current;
        if (w)
            w->m_status &= ~(widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        addWidget(w, -1);
    }
    m_shown = false;
}

VA_COMPGEN(0x0046db90, 0x21, SCALAR_DELETING_DTOR, TCombatCreatureSubWindow)

VA(0x0046dbc0, 0x6B)  // dc 0x665e0
CombatCreatureSubWindow::~CombatCreatureSubWindow()
{
    for (std::vector<widget*>::iterator it = m_widgets.begin();
         it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// E:\gamedcs\combatcontrolsubwindow.cpp:688

VA(0x0046dc30, 0x2C2)  // roster order + "%d(%d)" pair + the three spell icons, dc 0x66648
void CombatCreatureSubWindow::update(const army& info, const Hero* owner)
{
    char buffer[64];

    m_backgroundWidget->setPlayerPaletteColors(
        owner != 0 ? owner->m_owner : g_game->getLocalPlayerGamePos());

    if (m_viewLevel == 1) {
        m_creatureIcon->setIconFrame(info.m_creatureType + 2);
        const CreatureTypeTraits& normalTraits =
            g_creatureTypeTraits[info.m_creatureType];

        unsigned char canShoot = info.canShoot(0);
        long attack = info.getAdjustedAttack(0, canShoot);
        int defense = info.getAdjustedDefense(0, 1);
        if (canShoot)
            attack = ::max(attack, info.getAdjustedAttack(0, 0));

        sprintf(buffer, "%d(%d)", normalTraits.m_attackSkill, attack);
        m_attackText->setText(buffer);
        sprintf(buffer, "%d(%d)", normalTraits.m_defenseSkill, defense);
        m_defenseText->setText(buffer);

        if (info.m_monInfo.m_damageLowBound != info.m_monInfo.m_damageHighBound) {
            sprintf(buffer, "%d-%d", info.m_monInfo.m_damageLowBound,
                    info.m_monInfo.m_damageHighBound);
        } else {
            sprintf(buffer, "%d", info.m_monInfo.m_damageLowBound);
        }
        m_damageText->setText(buffer);

        sprintf(buffer, "%d", info.m_monInfo.m_hitPoints);
        m_speedText->setText(buffer);

        m_moraleIcon->setIconFrame(info.getMorale(1) + 3);
        m_luckIcon->setIconFrame(info.getLuck(1) + 3);

        int count = info.m_numTroopsToShowOverride;
        if (count == -1)
            count = info.m_numTroops;
        sprintf(buffer, "%d", count);
        m_countText->setText(buffer);
    }

    unsigned int spell = ::max(
        0, static_cast<int>(info.m_spellInfluenceQueue.size()) - 3);
    for (int icon = 0; icon < 3; ++icon) {
        if (spell < info.m_spellInfluenceQueue.size()) {
            m_spellIcons[icon]->setIconFrame(info.m_spellInfluenceQueue[spell] + 1);
        } else {
            m_spellIcons[icon]->setIconFrame(0);
        }
        ++spell;
    }

    if (info.m_spellInfluenceQueue.size() == 0)
        m_spellText->setText((*g_generalText)[675]);
    else
        m_spellText->setText("");
}

VA(0x0046df00, 0x73)  // dc 0x668e8
void CombatCreatureSubWindow::show()
{
    if (!m_shown) {
        saveBackground();
        for (std::vector<widget*>::iterator current = m_widgets.begin();
             current != m_widgets.end(); ++current) {
            if (*current)
                (*current)->m_status |= widget::WIDGET_ACTIVE |
                                      widget::WIDGET_DRAWN;
        }
        draw(0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        g_windowManager->updateScreen(
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
        m_shown = true;
    }
}

VA(0x0046df80, 0x3A)  // dc 0x66970
void CombatCreatureSubWindow::unShow()
{
    if (m_shown) {
        for (std::vector<widget*>::iterator current = m_widgets.begin();
             current != m_widgets.end(); ++current) {
            if (*current)
                (*current)->m_status &= ~(widget::WIDGET_ACTIVE |
                                        widget::WIDGET_DRAWN);
        }
        restoreBackground();
        m_shown = false;
    }
}

#if 0  // @carcass

// E:\gamedcs\combatcontrolsubwindow.cpp:562
DC_ONLY(0x65dbc, 0x824)
void CombatCreatureSubWindow::CombatCreatureSubWindow(int x, int y, int w, int h, heroWindow* parent, int view_level)
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:666
DC_ONLY(0x665e0, 0x68)
void CombatCreatureSubWindow::~CombatCreatureSubWindow()
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:688
DC_ONLY(0x66648, 0x2A0)
void CombatCreatureSubWindow::update(const army* info, const Hero* owner)
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:773
DC_ONLY(0x668e8, 0x88)
void CombatCreatureSubWindow::show()
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:820
DC_ONLY(0x66970, 0x84)
void CombatCreatureSubWindow::unShow()
{
    // @stub
}

// E:\gamedcs\button.h:99
DC_ONLY(0x669f4, 0x6)
void button::setDisabledFrame(long frame)
{
    // @stub
}

// E:\gamedcs\Hero.h:634
DC_ONLY(0x669fc, 0x3C)
int Hero::getMaxMana()
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:116
DC_ONLY(0x66a38, 0x34)
void* type_combat_sub_window::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:214
DC_ONLY(0x66a6c, 0x34)
void* CombatControlSubWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:308
DC_ONLY(0x66aa0, 0x34)
void* CombatPlacementSubWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:396
DC_ONLY(0x66ad4, 0x34)
void* CombatHeroSubWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\combatcontrolsubwindow.cpp:655
DC_ONLY(0x66b08, 0x34)
void* CombatCreatureSubWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x66b3c, 0x1C)
void std::vector<widget *,std::allocator<widget *> >::vector<widget *,std::allocator<widget *> >(const std::allocator<widget* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x66b58, 0x28)
void std::vector<widget *,std::allocator<widget *> >::~vector<widget *,std::allocator<widget *> >()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x66b80, 0x4)
void std::allocator<widget *>::allocator<widget *>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x66b84, 0x4)
void std::allocator<widget *>::~allocator<widget *>()
{
    // @stub
}

// ..\stlport\stl_deque.h:583
DC_ONLY(0x66b88, 0x18)
const SpellID* std::deque<enum SpellID,std::allocator<enum SpellID>,0>::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x66ba0, 0x2C)
void std::_Vector_base<widget *,std::allocator<widget *> >::_Vector_base<widget *,std::allocator<widget *> >(const std::allocator<widget* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x66bcc, 0x30)
void std::_Vector_base<widget *,std::allocator<widget *> >::~_Vector_base<widget *,std::allocator<widget *> >()
{
    // @stub
}

// ..\stlport\stl_deque.h:312
DC_ONLY(0x66bfc, 0x28)
SpellID* std::_Deque_iterator<enum SpellID,std::_Nonconst_traits<enum SpellID>,std::_Buf_size_traits<enum SpellID,0> >::operator[](int __n)
{
    // @stub
}

// ..\stlport\stl_string.h:469
DC_ONLY(0x66c24, 0x18)
void std::_STL_alloc_proxy<widget * *,widget *,std::allocator<widget *> >::~_STL_alloc_proxy<widget * *,widget *,std::allocator<widget *> >()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x66c3c, 0xC)
void std::_STL_alloc_proxy<widget * *,widget *,std::allocator<widget *> >::_STL_alloc_proxy<widget * *,widget *,std::allocator<widget *> >(const std::allocator<widget* __a, widget*** __p)
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x0046a650, 0x26, VECTOR_DTOR, widget)

VA_COMPGEN(0x004491c0, 0x69, DEQUE_CONST_ITERATOR_ADD, int)
