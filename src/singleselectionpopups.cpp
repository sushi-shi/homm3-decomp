// Popup-dialog family. Three widget subclasses of widget (CHotspotWidget,
// CSpriteWidget, CBitmapWidget) and four TDialogBox-derived single-selection
// dialogs (CBonusDlg, CHeroDlg, CTownDlg, CTeamAlignmentDlg) whose common
// base CSingleSelPopup's header-defined ctor/Add/ExitDialog expand into the
// derived ctors. The base is never directly instantiated and has no own vtable.
// The whole order-map is vtable-proven: every class ctor stores its own vtable.
// The overridden slots name the retail address directly (address-take
// proof). Vtables read straight from the hash-verified image:
//   CHotspotWidget  0x6419a4   (widget subclass, 13 slots)
//   CSpriteWidget   0x641a00   (widget subclass, 13 slots)
//   CBitmapWidget   0x641a34   (widget subclass, 13 slots)
//   CBonusDlg       0x6419d8   (TDialogBox subclass, 10 slots)
//   CHeroDlg        0x641a68   (TDialogBox subclass, 10 slots)
//   CTownDlg        0x641a90   (TDialogBox subclass, 10 slots)
//   CTeamAlignmentDlg 0x641ab8 (TDialogBox subclass, 10 slots)
// ICF folds several bodies: CBitmapWidget::Main and its scalar deleting
// destructor onto CSpriteWidget's (0x575a10 / 0x5757b0), and the CTownDlg /
// CTeamAlignmentDlg scalar deleting destructors onto CHeroDlg's (0x575e30);
// only one source body claims each shared retail address.
// Widget/CSingleSelPopup ctors and the trivial zBufferDraw/Draw stubs are
// inlined or ICF-folded out of this TU (0x404140 / 0x404df0 shared empties).
// All three widget non-deleting dtors fold to 0x575a60 (jmp ~widget), whose
// exact source-order bracket selects CBitmapWidget as the canonical claim.
// All four dialog non-deleting dtors fold to 0x576530 (jmp ~TDialogBox),
// similarly bracketed by CTeamAlignmentDlg's ctor and CreateWin.

// CreateWin family status: the two CBonusDlg::CreateWin overloads are
// reconstructed (98.00 / 95.51) and cap on the register-homing family - the
// schedule is aligned (why-reg flow-distance 0) but retail binds `this` to edi
// and the per-widget temp to esi where our CL binds them the other way, a swap
// the vc6 catalog reports as not source-addressable. CHeroDlg and CTownDlg
// remain @stub; CTeamAlignmentDlg is reconstructed below.
#include "va.h"
#include "includes.h"

#include "singleselectionpopups.h"

#include "bitmap16.h"
#include "bitmap816.h"
#include "csprite.h"
#include "font.h"
#include "game.h"
#include "iconwdgt.h"
#include "kb.h"
#include "kbwin.h"
#include "remote.h"
#include "resourcemanager.h"
#include "textresource.h"
#include "textwdgt.h"
#include "winmgr.h"

// ============================================================================
// CHotspotWidget - a bare rectangular click target.
// ============================================================================

VA(0x00575220, 0x40)  // dc 0x12de28
CHotspotWidget::CHotspotWidget(int xPos, int yPos, int w, int h, int widgetId)
{
    m_x = xPos;
    m_y = yPos;
    m_width = w;
    m_height = h;
    m_id = widgetId;
}

VA_COMPGEN(0x00575260, 0x21, SCALAR_DELETING_DTOR, CHotspotWidget)  // vtbl 0x6419a4 slot0, dc 0x12f2b8

// E:\gamedcs\singleselectionpopups.cpp:124 - slot 2 of vtable 0x6419a4, the
// same hotspot mouse handler border::Main runs one file over: a field_2C
// asleep guard, a not-active bail to widget::Main, and a jump-table switch on
// msg->id with the disabled DOWN/UP arms falling through to their right-button
// twins. mouseX/mouseY are int (heroWindow::x/y are int, +8.9 over short).

// Residual (94.28%): the SAME merged-return generation class border::Main
// carries (its residual note quotes the identical DUP-EXIT). Retail merges
// the RIGHT_BUTTON_UP not-selected exit into the shared return-0 block the
// field_2C guard opens (backward `je`); this C2 duplicates it as a fifth
// `ret`. Plain `return 0;` at every exit is the closest (94.28); a
// `goto returnZero` from any later exit re-sinks the guard to `jg` and drops
// it to 90.06 - the `--branches` DUP-EXIT with the guard block moved. Not
// source-reachable, same as border::Main.
VA(0x00575290, 0x179)  // anchor-vtable CHotspotWidget vtbl 0x6419a4 slot2 (Main override), ret 4, dc 0x12dea8
int CHotspotWidget::main(message& msg)
{
    if (m_sleepCount > 0)
        return 0;

    if (!(m_status & WIDGET_ACTIVE))
        return widget::main(msg);

    unsigned char isDisabled = 0;
    if (m_status & WIDGET_DISABLED)
        isDisabled = 1;

    switch (msg.m_id) {
    case MESSAGE_LEFT_BUTTON_DOWN:
        if (isDisabled)
            break;
        // fall through
    case MESSAGE_RIGHT_BUTTON_DOWN: {
        int mouseX = msg.m_codeX - m_parentWindow->m_x;
        int mouseY = msg.m_codeY - m_parentWindow->m_y;
        if (mouseX < m_x || mouseY < m_y || mouseX >= m_x + m_width
            || mouseY >= m_y + m_height)
            return 0;
        if (msg.m_id == MESSAGE_RIGHT_BUTTON_DOWN) {
            msg.m_qualifier = MESSAGE_MODIFIER_RIGHT;
            msg.m_codeX = WIDGET_RIGHT_SELECT;
        } else {
            m_status |= WIDGET_SELECTED;
            msg.m_codeX = WIDGET_SELECT;
        }
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeY = m_id;
        return 2;
    }

    case MESSAGE_LEFT_BUTTON_UP:
        if (isDisabled)
            break;
        // fall through
    case MESSAGE_RIGHT_BUTTON_UP:
        if (!(m_status & WIDGET_SELECTED))
            return 0;
        m_status &= ~WIDGET_SELECTED;
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = WIDGET_DESELECT;
        msg.m_codeY = m_id;
        return 2;
    }
    return widget::main(msg);
}

// ============================================================================
// CBonusDlg - the two-CreateWin bonus dialog.
// ============================================================================

VA(0x00575410, 0x20)  // dc 0x12dfa8
CBonusDlg::CBonusDlg(unsigned char newGameMode)
    : CSingleSelPopup(0x12, newGameMode)
{
}

VA_COMPGEN(0x005754c0, 0x21, SCALAR_DELETING_DTOR, CBonusDlg)  // dc 0x12f304

VA(0x005754f0, 0x254)  // dc 0x12dff0
unsigned char CBonusDlg::createWin(const char* title, CSprite* sprite, int frame, const char* botTitle, const char* description)
{
    if (!setup(300, 225, 200, 150))
        return 0;
    add(new textWidget(10, 26, m_width - 20, 36, title, "medfont.fnt",
        font::PRIMARY, -1, 1, 0, 8));
    add(new CSpriteWidget((m_width - sprite->getWidth()) / 2, 60, sprite, frame));
    add(new textWidget(10, 95, m_width - 20, 18, botTitle, "smalfont.fnt",
        font::PRIMARY, -1, 1, 0, 8));
    add(new textWidget(15, 120, m_width - 30, m_height - 120, description,
        "smalfont.fnt", font::PRIMARY, -1, 1, 0, 8));
    return 1;
}

// ============================================================================
// CSpriteWidget - a widget wrapping a CSprite.
// ============================================================================

VA(0x00575750, 0x54)  // dc 0x12f0c8
void CSpriteWidget::draw() const
{
    m_sprite->draw(0, m_frame, 0, 0, m_width, m_height,
        g_windowManager->m_screenBitmap, m_x + m_parentWindow->m_x,
        m_y + m_parentWindow->m_y, 0, 1);
}

// E:\gamedcs\singleselectionpopups.cpp:47 - inlined into every CreateWin that
// builds a sprite icon (dc 0x12f018, no standalone retail body). Stores the
// sprite/frame past the widget base and normalises frame against sequence 0's
// count; GetNumFrames(0)'s else arm folds to the literal-0 divisor.
CSpriteWidget::CSpriteWidget(int xPos, int yPos, CSprite* sprite, int frameArg)
{
    m_sprite = sprite;
    m_frame = frameArg;
    m_x = xPos;
    m_y = yPos;
    m_width = sprite->getWidth();
    m_height = sprite->getHeight();
    m_frame %= sprite->getNumFrames(0);
}

// Original: CSpriteWidget::zBufferDraw; singleselectionpopups.cpp:67, dc 0x12f0c4.
// Vtable0x641a00 slot3 shares the empty ret8 body at0x404140.
void CSpriteWidget::zBufferDraw(unsigned short* zBuffer, int id) const
{
}

CSpriteWidget::~CSpriteWidget()
{
}

VA_COMPGEN(0x005757b0, 0x21, SCALAR_DELETING_DTOR, CSpriteWidget)  // dc 0x12f11c

VA(0x005757e0, 0x226)  // dc 0x12e1cc
unsigned char CBonusDlg::createWin(const char* title, Bitmap816* image, const char* botTitle, const char* description)
{
    if (!setup(300, 225, 200, 150))
        return 0;
    add(new textWidget(10, 26, m_width - 20, 36, title, "medfont.fnt",
        font::PRIMARY, -1, 1, 0, 8));
    add(new CBitmapWidget((m_width - image->getWidth()) / 2, 60, image));
    add(new textWidget(10, 95, m_width - 20, 18, botTitle, "smalfont.fnt",
        font::PRIMARY, -1, 1, 0, 8));
    add(new textWidget(15, 120, m_width - 30, m_height - 120, description,
        "smalfont.fnt", font::PRIMARY, -1, 1, 0, 8));
    return 1;
}

VA(0x00575a10, 0x10)  // dc 0x12f0ac
int CSpriteWidget::main(message& msg)
{
    return widget::main(msg);
}

// ============================================================================
// CBitmapWidget - a widget wrapping a Bitmap816.
// ============================================================================

VA(0x00575a20, 0x3e)  // dc 0x12f1fc
void CBitmapWidget::draw() const
{
    m_image->draw(0, 0, m_image->getWidth(), m_image->getHeight(),
        g_windowManager->m_screenBitmap, m_x + m_parentWindow->m_x,
        m_y + m_parentWindow->m_y, 1);
}

// E:\gamedcs\singleselectionpopups.cpp:82 - inlined into the bitmap CreateWin
// callers (dc 0x12f168, no standalone retail body). Stores the image past the
// widget base with the bitmap's own extent.
CBitmapWidget::CBitmapWidget(int xPos, int yPos, Bitmap816* image)
{
    m_image = image;
    m_x = xPos;
    m_y = yPos;
    m_width = image->getWidth();
    m_height = image->getHeight();
}

// E:\gamedcs\singleselectionpopups.cpp:121, dc 0x12f2ec.
// Original: CBitmapWidget::Main; singleselectionpopups.cpp:93, dc 0x12f1e0.
// Vtable0x641a34 slot2 folds to CSpriteWidget::main0x575a10.
int CBitmapWidget::main(message& msg)
{
    return widget::main(msg);
}

// Original: CBitmapWidget::zBufferDraw; singleselectionpopups.cpp:99, dc 0x12f1f8.
void CBitmapWidget::zBufferDraw(unsigned short* zBuffer, int id) const
{
}

CHotspotWidget::~CHotspotWidget()
{
}

VA_COMPGEN(0x00575a60, 0x5, IMPLICIT_DTOR, CBitmapWidget)  // dc 0x12f2a0

// ============================================================================
// CHeroDlg
// ============================================================================

VA(0x00575a70, 0x20)  // dc 0x12e3a0
CHeroDlg::CHeroDlg(unsigned char newGameMode)
    : CSingleSelPopup(0x12, newGameMode)
{
}

VA(0x00575a90, 0x380)  // dc 0x12e3f0
unsigned char CHeroDlg::createWin(Bitmap816* heroPick, const char* heroName, CSprite* specialtyIcon, int frame, const char* specialtyName, const char* desc)
{
    char tempText[256];

    if (!setup(250, 172, 300, 256))
        return 0;

    add(new textWidget(30, 26, m_width - 60, 36,
        g_generalText->getText(78), "medfont.fnt", font::PRIMARY,
        -1, 1, 0, 8));
    add(new CBitmapWidget((m_width - heroPick->getWidth()) / 2, 56, heroPick));

    sprintf(tempText, DATA_COMPGEN(
        0x0066033c, rolloverOwnedObjectFormat, "%s - %s"), heroName, desc);
    add(new textWidget(30, 91, m_width - 60, 18, tempText,
        "smalfont.fnt", font::PRIMARY, -1, 1, 0, 8));

    add(new textWidget(30, 122, m_width - 60, 36,
        g_generalText->getText(79), "medfont.fnt", font::PRIMARY,
        -1, 1, 0, 8));
    add(new CSpriteWidget((m_width - specialtyIcon->getWidth()) / 2, 149,
        specialtyIcon, frame));
    add(new textWidget(30, specialtyIcon->getHeight() + 151, m_width - 60, 36,
        specialtyName, "smalfont.fnt", font::PRIMARY, -1, 1, 0, 8));
    return 1;
}

// ============================================================================
// CTownDlg
// ============================================================================

VA(0x00575e10, 0x20)  // dc 0x12e690
CTownDlg::CTownDlg(unsigned char newGameMode)
    : CSingleSelPopup(0x12, newGameMode)
{
}

VA_COMPGEN(0x00575e30, 0x21, SCALAR_DELETING_DTOR, CHeroDlg)  // vtbl 0x641a68/0x641a90/0x641ab8 slot0; ICF folds CTownDlg (dc 0x12f36c) + CTeamAlignmentDlg (dc 0x12f3a0) dtors, dc 0x12f338

// E:\gamedcs\singleselectionpopups.cpp:302
VA(0x00575e60, 0x670)  // anchor-vtable CTownDlg::CreateWin inlines CSpriteWidget ctor (stores vtbl 0x641a00), ret 0xc (3 args), dc 0x12e708
unsigned char CTownDlg::createWin(CSprite* town, int frame, TTownType townType)
{
    if (!setup(272, 140, 256, 320))
        return 0;

    add(new textWidget(10, 26, m_width - 20, 36,
        g_generalText->getText(81), "medfont.fnt", font::PRIMARY,
        -1, 1, 0, 8));
    add(new CSpriteWidget((m_width - town->getWidth()) / 2, 60, town, frame));
    add(new textWidget(10, 95, m_width - 20, 18,
        g_townTypeNames[townType + 1], "smalfont.fnt", font::PRIMARY,
        -1, 1, 0, 8));
    add(new textWidget(10, 127, m_width - 20, 36,
        g_generalText->getText(80), "medfont.fnt", font::PRIMARY,
        -1, 1, 0, 8));

    int centerX = m_width / 2;
    int iconX = centerX - 68;
    int textX = iconX - 10;
    int creatureBase = townType * (2 * TOWN_DWELLING_COUNT);
    int slot;
    for (slot = 0; slot < 3; ++slot) {
        int creature = g_townDwellingCreatures[creatureBase + slot];
        iconWidget* portrait = new iconWidget(
            iconX, 159, 32, 32, slot, "cprsmall.def",
            0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN);
        add(portrait);
        portrait->setIconFrame(creature + 2);
        add(new textWidget(textX, 193, 52, 32,
            g_creatureTypeTraits[creature].m_name, "tiny.fnt",
            font::PRIMARY, -1, 1, 0, 8));
        iconX += 52;
        textX += 52;
    }

    iconX = centerX - 88;
    textX = iconX - 10;
    for (slot = 3; slot < TOWN_DWELLING_COUNT; ++slot) {
        int creature = g_townDwellingCreatures[creatureBase + slot];
        iconWidget* portrait = new iconWidget(
            iconX, 235, 32, 32, slot, "cprsmall.def",
            0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN);
        add(portrait);
        portrait->setIconFrame(creature + 2);
        add(new textWidget(textX, 267, 52, 32,
            g_creatureTypeTraits[creature].m_name, "tiny.fnt",
            font::PRIMARY, -1, 1, 0, 8));
        iconX += 52;
        textX += 52;
    }
    return 1;
}

// ============================================================================
// CTeamAlignmentDlg
// ============================================================================

VA(0x005764d0, 0x53)  // dc 0x12eac8
CTeamAlignmentDlg::CTeamAlignmentDlg(unsigned char newGameMode)
    : CSingleSelPopup(0x12, newGameMode)
{
    getTeams();
}

VA_COMPGEN(0x00576530, 0x5, IMPLICIT_DTOR, CTeamAlignmentDlg)  // dc 0x12b038

// E:\gamedcs\singleselectionpopups.cpp:365
// Residual (99.11%): all 41 CFG blocks and their edges agree; only B14/B17/B36
// differ in size because C2 promotes xStart through EDI here while retail
// stores it in one stack slot and reloads it through EAX/EDX. Dreamcast names
// xStart as T_INT4 and sTemp as char[256], in this declaration order. The
// bounded generator measured 1,965 depth-1..3 AST shapes, 124 lifetime/helper/
// alias shapes, and 336 plausible names for the unrecorded loop locals; all
// clean variants were flat or worse. The guided allocator sweep finds only
// `volatile xStart` (distance 15 -> 8), which is neither source evidence nor
// exact and is deliberately rejected. Restoring the two preceding real
// CreateWin bodies (CHeroDlg and CTownDlg) also leaves this score unchanged.
VA(0x00576540, 0x3e8)  // anchor-vtable, dc 0x12eb24
unsigned char CTeamAlignmentDlg::createWin()
{
    int xStart;
    char tempText[256];
    int dialogHeight = ((m_numTeams * 50 + 56) / 64 + 1) * 64;
    if (!setup(272, (600 - dialogHeight) / 2, 256, dialogHeight))
        return 0;

    add(new textWidget(10, 20, m_width - 20, 36,
        g_generalText->getText(658), "medfont.fnt", font::PRIMARY,
        -1, 1, 0, 8));

    for (int team = 0; team < m_numTeams; ++team) {
        int y = team * 50 + 56;
        sprintf(tempText, g_generalText->getText(657), team + 1);
        add(new textWidget(10, y, m_width - 20, 18, tempText,
            "smalfont.fnt", font::PRIMARY, -1, 1, 0, 8));

        int rowWidth = countNumPlayers(team) * 18 - 3;
        xStart = (m_width - rowWidth) / 2;
        for (int player = 0; player < 8; ++player) {
            if (m_teamMasks[team] & (1 << player)) {
                iconWidget* flag = new iconWidget(
                    xStart, y + 20, 15, 20, -1, "itgflags.def",
                    0, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN);
                flag->sendMessage(widget::WIDGET_SET_ICON_FRAME, player);
                add(flag);
                xStart += 18;
            }
        }
    }
    return 1;
}

int CTeamAlignmentDlg::countNumPlayers(int teamNbr)
{
    int count = 0;
    for (int player = 0; player < 8; ++player) {
        if (m_teamMasks[teamNbr] & (1 << player))
            ++count;
    }
    return count;
}

VA(0x00576930, 0xd1)  // dc 0x12edd4
void CTeamAlignmentDlg::getTeams()
{
    unsigned char assigned[8] = { 0 };
    int player;

    memset(m_teamMasks, 0, sizeof(m_teamMasks));
    m_numTeams = 0;
    for (player = 0; player < 8; ++player) {
        if (g_game->m_setup.m_playerPos[player] < 0)
            continue;
        if (assigned[player])
            continue;
        assigned[player] = 1;
        m_teamMasks[m_numTeams] = 1 << player;
        for (int other = player + 1; other < 8; ++other) {
            if (g_game->m_setup.m_playerPos[other] < 0)
                continue;
            if (g_game->onSameTeam(player, other)) {
                assigned[other] = 1;
                m_teamMasks[m_numTeams] |= 1 << other;
            }
        }
        ++m_numTeams;
    }
}

// ============================================================================
// The scenario-setup "Resource" starting bonus, in two halves.
// ============================================================================

// The two differ only in their text bank: 693..696 with default 90 for the
// short bottom title, 689..692 with default 94 for the description block.

VA(0x00576e00, 0x80)
const char* getStartingResourceName(int town)
{
    switch (town) {
    case TOWN_RAMPART:
        return g_generalText->getText(693);
    case TOWN_TOWER:
        return g_generalText->getText(694);
    case TOWN_INFERNO:
    case TOWN_CONFLUX:
        return g_generalText->getText(695);
    case TOWN_DUNGEON:
        return g_generalText->getText(696);
    default:
        return g_generalText->getText(90);
    }
}

VA(0x00576e80, 0x80)
const char* getStartingResourceDescription(int town)
{
    switch (town) {
    case TOWN_RAMPART:
        return g_generalText->getText(689);
    case TOWN_TOWER:
        return g_generalText->getText(690);
    case TOWN_INFERNO:
    case TOWN_CONFLUX:
        return g_generalText->getText(691);
    case TOWN_DUNGEON:
        return g_generalText->getText(692);
    default:
        return g_generalText->getText(94);
    }
}

// ============================================================================
// TRandomMapProgress - the modal progress bar around the generator run.
// ============================================================================

// Complete adds this random-map generation window. The Dreamcast popup
// procedure inventory ends with the team-alignment dialog, and its full
// CodeView class field lists contain no RMG/progress class. The five exact
// Windows-only method identities are reviewed in config/source/win_only.tsv.
// The whole family is vtable-proven: 0x641b14 slot 0 is the scalar deleting
// destructor 0x577090, slot 1 the SetTotal override 0x577300 and slot 2 the
// Advance override 0x577320, and 0x576f00 is the only body that stores that
// vtable.  The base's own constructor 0x530e20 sits in the
// quicktownwindow..recruit span and is left unclaimed.

VA(0x00576F00, 0x190)
TRandomMapProgress::TRandomMapProgress(int totalSteps)
    : TProgressSink(totalSteps)
{
    m_barSprite = ResourceManager::getSprite(
        DATA_COMPGEN(0x0067F5AC, progressBarSpriteName, "loadprog.def"));
    m_drawnPosition = 0;
    m_window = new TDialogBox(240, 236, 320, 128, 0x12);

    const char* caption = g_generalText->getText(761);
    int captionWidth = g_mediumFont->getStringWidth(caption);
    int captionX = (m_window->m_width - captionWidth) / 2;
    textWidget* captionWidget = new textWidget(
        captionX, 30, captionWidth, 20, caption,
        DATA_COMPGEN(0x0065F2EC, progressBarFontName, "medfont.fnt"),
        font::PRIMARY, -1, 1, 0, 8);
    m_widgets.push_back(captionWidget);

    for (unsigned int i = 0; i < m_widgets.size(); i++)
        m_window->addWidget(m_widgets[i], -1);
    g_windowManager->addWindow(m_window, -1, 1);
    updateProgressBar();
    g_windowManager->updateScreen(0, 0, 800, 600);
}

// Slot 0 of vtable 0x641b14.
VA_COMPGEN(0x00577090, 0x21, SCALAR_DELETING_DTOR, TRandomMapProgress)

VA(0x005770C0, 0xBE)
TRandomMapProgress::~TRandomMapProgress()
{
    g_windowManager->removeWindow(m_window);
    delete m_window;
    if (m_barSprite)
        m_barSprite->dispose();
    for (unsigned int i = 0; i < m_widgets.size(); i++)
        delete m_widgets[i];
}

VA(0x00577180, 0x17F)
void TRandomMapProgress::updateProgressBar()
{
    if (!m_barSprite)
        return;
    if (m_steps <= 0)
        return;

    int position = m_done * 256 / m_steps;
    int fullRow = position / 16;
    if (position == m_drawnPosition)
        return;
    m_drawnPosition = position;
    int partialRow = position % 16;
    m_window->drawWindow(0, 0xffff0001, 0xffff);

    for (int i = 0; i < fullRow; i++) {
        m_barSprite->draw(0, i, 0, 0, m_barSprite->getWidth(), m_barSprite->getHeight(),
                        g_windowManager->m_screenBitmap->getMap(0, 0),
                        m_window->m_x + i * 18 + 16, m_window->m_y + 0x3c,
                        g_windowManager->m_screenBitmap->getWidth(),
                        g_windowManager->m_screenBitmap->getHeight(),
                        g_windowManager->m_screenBitmap->getPitch(), 0, 0);
    }
    for (int j = 0; j < partialRow; j++) {
        m_barSprite->draw(0, j, 0, 0, m_barSprite->getWidth(), m_barSprite->getHeight(),
                        g_windowManager->m_screenBitmap->getMap(0, 0),
                        m_window->m_x + j * 18 + 16, m_window->m_y + 0x50,
                        g_windowManager->m_screenBitmap->getWidth(),
                        g_windowManager->m_screenBitmap->getHeight(),
                        g_windowManager->m_screenBitmap->getPitch(), 0, 0);
    }

    g_windowManager->updateScreen(m_window->m_x + 16, m_window->m_y + 0x3c, 0x120, 16);
    g_windowManager->updateScreen(m_window->m_x + 16, m_window->m_y + 0x50, 0x120, 16);
}

// Slot 1 of vtable 0x641b14 - the base's SetTotal override.
VA(0x00577300, 0x12)
void TRandomMapProgress::setTotal(int totalSteps)
{
    m_steps = totalSteps;
    updateProgressBar();
}

// COMDAT pairing: vector<widget*>::_Ucopy, agreement 0.978. Same caller-set
// argument as the insert above: 0x174ce0 is reached from TAdventureMapWindow's
// constructor and SetSleepImage, and from TTownGateWindow::AddTown's expanded
// insert - cross-unit reach that only a shared COMDAT has. dialogbox 0x8dba0
// (49 B, identical similarity) is reached only from unresolved labels inside
// its own two segments.
VA_COMPGEN(0x00574ce0, 0x2F, VECTOR_UCOPY, widget)
