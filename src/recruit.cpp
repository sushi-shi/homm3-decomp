#include "text.h"
#include "va.h"
#include "includes.h"

#include <stdio.h>
#if defined(_MSC_VER)
#include <xutility>
#else
#include <algorithm>
#endif

#include "recruit.h"

#include "advmgr.h"
#include "armygrp.h"
#include "artifact.h"
#include "border.h"
#include "button.h"
#include "creaturetype.h"
#include "exec.h"
#include "game.h"
#include "hero.h"
#include "iconwdgt.h"
#include "kb.h"
#include "kbwin.h"
#include "message.h"
#include "misc.h"
#include "mousemgr.h"
#include "resourcedisplay.h"
#include "terrain.h"
#include "textntry.h"
#include "textwdgt.h"
#include "townmgr.h"
#include "viewarmywindow.h"
#include "widget.h"
#include "winmgr.h"

// The recruit dialog's window, .bss 0x69d5e8. Name provisional (the
// gp<Type> house convention); recruitUnit::Open builds it,
// recruitUnit::Close RemoveWindow()s and deletes it, and Update
// broadcasts every widget refresh through it. Mac's direct TOC storage
// confirms this pointer belongs to the TU rather than a public interface.
DATA(0x0069d5e8) static TRecruitWindow* g_recruitWindow;
DATA(0x0069d5f4) HMENU__* g_recruitSavedMenu;


// recruit.cpp-owned rollover text pointers. Each has exactly one retail
// reader, the SetRolloverText expansion in recruitUnit::Main; the adjacent
// TRecruitWindow constructor initializes the dialog family that owns them.

VA(0x0054e750, 0x64)  // dc 0x118adc
void getUpgradeCost(TCreatureType creature, TCreatureType upgrade, long amount, long* cost)
{
    const int* toCost = g_creatureTypeTraits[upgrade].m_cost;
    const int* fromCost = g_creatureTypeTraits[creature].m_cost;

    for (int i = 0; i < 7; i++) {
        if (toCost[i] > fromCost[i])
            cost[i] = (toCost[i] - fromCost[i]) * amount;
        else
            cost[i] = 0;
    }
}

VA(0x0054e7c0, 0x31)  // dc 0x118b38
void getMonsterCost(int monId, int* resCost)
{
    int resource;
    MEMCPY(resCost, g_creatureTypeTraits[monId].m_cost,
           7 * sizeof(resCost[0]), resource);
}

// ---------------------------------------------------------------------

// ---------------------------------------------------------------------

VA(0x0054e800, 0x4F)  // dc 0x118b68
void recruitSliderCallback(int state, heroWindow* parentWindow)
{
    g_recruitWindow->m_recruitInfo->m_numberToBuy = state;
    g_recruitWindow->m_acceptButton->enable(state != 0);
    g_recruitWindow->m_recruitInfo->update(0, -1);
    g_recruitWindow->drawWindow(1, WINDOW_ALL_WIDGETS_LOW,
                                WINDOW_ALL_WIDGETS_HIGH);
}

// E:\gamedcs\recruit.cpp:192
// Retail's 49-slot reserve and the 29 fixed widgets explain the long body:
// each push_back carries VC6's checked vector-growth path, followed by the
// variable one-to-four creature cards and a final AddWidget registration
// pass. The resource columns shift right 24 pixels when no alternate
// resource exists; retail spells that branchlessly in edi.
// Residual (99.01%): all 185 semantic blocks and all 89 branches, including
// every symbolic branch target, agree. Retail keeps the incremented
// altResource value in EDI through its 0/24 selection; this compile creates
// the same value through EAX and moves it to EDI, after which only C1/C2
// register/EH-slot scheduling differs. Measured source forms: direct ternary
// 97.19%, default-then-if 98.71% with an extra branch, boolean multiply
// 98.83%, zero-test ternary 97.45%, and reusing or pre-initializing the
// parameter/local 97.38/97.45%. `homm3 vc6 why-reg --model` sees 407
// register-visible slots; its only legal first-created parameter alias is
// copy-propagated and flat, classifying the rest as front-end handle state.
// [polish 16] Two more measured and rejected, and they bound the shape from
// both sides: the declaration form is inert - hoisting the initialiser to
// first use (`int resource_shift = altResource ? 0 : 0x18;` with no separate
// declaration) is byte-flat at 99.0102 to the digit - and the interleave is
// NOT source-movable: retail's `mov eax,[esi+4]` sits above the neg/sbb
// chain, but writing the selection AFTER `Widgets.reserve(49)` to produce
// that order costs 4.82 (94.1908, four size-only blocks). The `and al,-0x18`
// is a CONSEQUENCE of landing in EAX, not a cause - VC6 has no 8-bit form
// for EDI - so nothing at this site can move the allocation. WALL.
// DC lines 207/215/218/224 name the four TTextResource::operator[] calls
// below; restoring that canonical wrapper is VC6 byte-flat at 99.01016%.
VA(0x0054e850, 0x1295)  // unique x86/DC structure + constructor call, dc 0x118bb4
TRecruitWindow::TRecruitWindow(int x2, int y2, int altResource,
                               recruitUnit* recruitInfo)
    : heroWindow(x2, y2, 0x1e5, 0x18b, 0x12)
{
    int resourceShift;
    ++altResource;
    resourceShift = altResource ? 0 : 0x18;

    m_widgets.reserve(49);
    m_widgets.push_back(new bitmapBorder(0, 0, 0x1e5, 0x18b, 0,
        DATA_COMPGEN(0x00682a0c, recruitBackground, "TPrcrt.pcx"),
        0x800));

    m_widgets.push_back(new coloredBorderFrame(0x40, 0xde, 0x63, 0x4c,
        0x227, g_systemPalette->m_data[31], 0x400));
    m_widgets.push_back(new coloredBorderFrame(0x142, 0xde, 0x63, 0x4c,
        0x228, g_systemPalette->m_data[31], 0x400));
    m_widgets.push_back(new coloredBorderFrame(0xac, 0xde, 0x43, 0x2a,
        0x229, g_systemPalette->m_data[31], 0x400));
    m_widgets.push_back(new coloredBorderFrame(0xf6, 0xde, 0x43, 0x2a,
        0x22a, g_systemPalette->m_data[31], 0x400));

    m_widgets.push_back(new textWidget(0xf, 0x14, 0x1c8, 0x1a,
        DATA_COMPGEN(0x00691210, recruitEmptyText, ""),
        DATA_COMPGEN(0x00660b24, recruitBigFont, "bigfont.fnt"),
        font::HEADING, 0x226, font::CENTER_JUSTIFIED, 0, 8));
    m_widgets.push_back(new textWidget(0x42, 0xe0, 0x5f, 0x11,
        (*g_generalText)[GENERAL_TEXT_COST_PER_TROOP],
        DATA_COMPGEN(0x0065f2f8, recruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x1f4,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));

    m_widgets.push_back(new iconWidget(resourceShift + 0x4a, 0xf3,
        0x20, 0x20, 0x1f8,
        DATA_COMPGEN(0x00660224, recruitResourceSprite, "resource.def"),
        6, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
    m_widgets.push_back(new iconWidget(0x7a, 0xf3, 0x20, 0x20, 0x1fc,
        DATA_COMPGEN(0x00660224, recruitResourceSprite, "resource.def"),
        6, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
    m_widgets.push_back(new textWidget(resourceShift + 0x42, 0x117,
        0x30, 0x11,
        DATA_COMPGEN(0x00691210, recruitEmptyText, ""),
        DATA_COMPGEN(0x0065f2f8, recruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x200,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    m_widgets.push_back(new textWidget(0x72, 0x117, 0x30, 0x11,
        DATA_COMPGEN(0x00691210, recruitEmptyText, ""),
        DATA_COMPGEN(0x0065f2f8, recruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x204,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));

    m_widgets.push_back(new textWidget(0xad, 0xdf, 0x41, 0x15,
        (*g_generalText)[GENERAL_TEXT_RECRUIT_AVAILABLE_LABEL],
        DATA_COMPGEN(0x0065f2f8, recruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x208,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    m_widgets.push_back(new textWidget(0xae, 0xf5, 0x3f, 0x11,
        DATA_COMPGEN(0x00691210, recruitEmptyText, ""),
        DATA_COMPGEN(0x0065f2f8, recruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x209,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    m_widgets.push_back(new textWidget(0xf7, 0xdf, 0x41, 0x15,
        (*g_generalText)[GENERAL_TEXT_RECRUIT_TITLE],
        DATA_COMPGEN(0x0065f2f8, recruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x20d,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    m_widgets.push_back(new textEntryWidget(0xf8, 0xf5, 0x3f, 0x11,
        0xa, DATA_COMPGEN(0x00682a08, recruitZeroText, "0"),
        DATA_COMPGEN(0x0065f2f8, recruitSmallFont, "smalfont.fnt"),
        font::WHITE,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED,
        0, 0, RECRUIT_QUANTITY_ID, 0,
        textEntryWidget::READ_TYPE_INSET, 0, 0));

    m_quantitySlider = new slider(0xb0, 0x117, 0x87, 0x10, 0x22f, 0xa,
        recruitSliderCallback, slider::BROWN, 0, 0);
    m_widgets.push_back(m_quantitySlider);

    m_widgets.push_back(new textWidget(0x144, 0xe0, 0x5f, 0x11,
        (*g_generalText)[GENERAL_TEXT_TOTAL_COST],
        DATA_COMPGEN(0x0065f2f8, recruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x20f,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    m_widgets.push_back(new iconWidget(resourceShift + 0x14c, 0xf3,
        0x20, 0x20, 0x210,
        DATA_COMPGEN(0x00660224, recruitResourceSprite, "resource.def"),
        6, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
    m_widgets.push_back(new iconWidget(0x17c, 0xf3, 0x20, 0x20, 0x211,
        DATA_COMPGEN(0x00660224, recruitResourceSprite, "resource.def"),
        6, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN));
    m_widgets.push_back(new textWidget(resourceShift + 0x144, 0x117,
        0x30, 0x11,
        DATA_COMPGEN(0x00691210, recruitEmptyText, ""),
        DATA_COMPGEN(0x0065f2f8, recruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x212,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));
    m_widgets.push_back(new textWidget(0x174, 0x117, 0x30, 0x11,
        DATA_COMPGEN(0x00691210, recruitEmptyText, ""),
        DATA_COMPGEN(0x0065f2f8, recruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x213,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8));

    m_widgets.push_back(new bitmapBorder(8, 0x172, 0x1d4, 0x12, 0x230,
        DATA_COMPGEN(0x00660b10, recruitStatusBar, "StatBar.pcx"),
        0x800));
    m_widgets.push_back(new textWidget(8, 0x172, 0x1d4, 0x12, 0,
        DATA_COMPGEN(0x0065f2f8, recruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x231, font::CENTER_JUSTIFIED, 0, 8));

    m_widgets.push_back(new bitmapBorder(0x85, 0x138, 0x42, 0x22, -1,
        DATA_COMPGEN(0x006829f8, recruitButtonBorder, "Box64x32.pcx"),
        0x800));
    m_maximumButton = new button(0x86, 0x139, 0x40, 0x20, RECRUIT_MAXIMUM_ID,
        DATA_COMPGEN(0x006829ec, recruitMaximumButton, "ircbtns.def"),
        0, 1, 0, 0x32, 2);
    m_widgets.push_back(m_maximumButton);

    m_widgets.push_back(new bitmapBorder(0xd3, 0x138, 0x42,
        0x22, -1,
        DATA_COMPGEN(0x006829f8, recruitButtonBorder, "Box64x32.pcx"),
        0x800));
    m_acceptButton = new button(0xd4, 0x139, 0x40, 0x20, RECRUIT_ACCEPT_ID,
        DATA_COMPGEN(0x006829e0, recruitAcceptButton, "iBY6432.def"),
        0, 1, 0, 0x1c, 2);
    m_widgets.push_back(m_acceptButton);

    m_widgets.push_back(new bitmapBorder(0x121, 0x138, 0x42,
        0x22, -1,
        DATA_COMPGEN(0x006829f8, recruitButtonBorder, "Box64x32.pcx"),
        0x800));
    m_widgets.push_back(new button(0x122, 0x139, 0x40, 0x20,
        RECRUIT_CANCEL_ID,
        DATA_COMPGEN(0x006829d4, recruitCancelButton, "iCN6432.def"),
        0, 1, 0, 1, 2));

    m_creatureWidgets[0] = 0;
    m_creatureWidgets[1] = 0;
    m_creatureWidgets[2] = 0;
    m_creatureWidgets[3] = 0;

    if (recruitInfo->m_monType4 != CREATURE_NONE) {
        addCreatureWidgets(0x1c, 0x41, 0xb4,
                             recruitInfo->m_monType1, 0);
        addCreatureWidgets(0x8a, 0x41, 0xb4,
                             recruitInfo->m_monType2, 1);
        addCreatureWidgets(0xf8, 0x41, 0xb4,
                             recruitInfo->m_monType3, 2);
        addCreatureWidgets(0x166, 0x41, 0xb4,
                             recruitInfo->m_monType4, 3);
    } else if (recruitInfo->m_monType3 != CREATURE_NONE) {
        addCreatureWidgets(0x1e, 0x41, 0xb4,
                             recruitInfo->m_monType1, 0);
        addCreatureWidgets(0xc1, 0x41, 0xb4,
                             recruitInfo->m_monType2, 1);
        addCreatureWidgets(0x164, 0x41, 0xb4,
                             recruitInfo->m_monType3, 2);
    } else if (recruitInfo->m_monType2 != CREATURE_NONE) {
        addCreatureWidgets(0x85, 0x41, 0xb4,
                             recruitInfo->m_monType1, 0);
        addCreatureWidgets(0xfd, 0x41, 0xb4,
                             recruitInfo->m_monType2, 1);
    } else {
        addCreatureWidgets(0xc1, 0x41, 0xb4,
                             recruitInfo->m_monType1, 0);
    }

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }
}

VA_COMPGEN(0x0054faf0, 0x21, SCALAR_DELETING_DTOR, TRecruitWindow)

VA(0x0054fb20, 0x6B)  // dc 0x1197bc
TRecruitWindow::~TRecruitWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// E:\gamedcs\recruit.cpp:302
// Retail's Complete-only elemental-card rule is the four-compare chain at
// 0x54fbd5: when the campaign/version flag is zero, the four base elementals
// use background index -1 (the deliberately biased akCreatureBackgrounds
// base). `name_y` is genuinely unused here; the constructor uses it for the
// adjacent name widgets after calling this helper.
// Residual (93.6325%): blocks B0..B24, including all three allocations and
// constructor calls, are instruction-for-instruction exact. The only delta is
// the slow-growth tail of the FINAL Widgets.push_back: retail leaves the last
// vector<widget*>::size() at 0x423110 out of line and therefore merges its
// empty/non-empty return, while this VC6 compile inlines that 19-byte helper
// and emits one duplicate exit. predict-inline measures 17 candidate calls
// versus retail's 18. A statement-scoped inline_depth(1) is byte-flat, while
// depth(0) de-inlines the whole push_back edge and falls to 51.8481%; it cannot
// isolate the nested size() call. Other source-form controls: named border
// temporary 93.59%; direct count-insert 79.10%; direct single-insert 87.60%.
// All preserve the values but perturb more of Dinkumware's insertion lowering,
// so the natural push_back form below is the banked maximum. Inline-budget /
// generation residual, not missing game logic.
VA(0x0054fb90, 0x30D)  // anchor-callee + anchor-global, dc 0x119820
void TRecruitWindow::addCreatureWidgets(long startX, long startY, long nameY, TCreatureType creature, long slot)
{
    m_widgets.push_back(new bitmapBorder(startX, startY, 100, 130,
        slot + 0x21e,
        g_creatureBackgrounds[
            g_game->m_gameVersion == 0
                && isBaseElemental(creature)
            ? -1 : g_creatureTypeTraits[creature].m_townType],
        0x800));

    m_creatureWidgets[slot] = new iconWidget(startX, startY, 100, 130,
        slot + 0x216, g_creatureTypeTraits[creature].m_spriteName,
        0, 2, 0, 0, iconWidget::ICON_STYLE_CREATURE);
    m_widgets.push_back(m_creatureWidgets[slot]);

    m_widgets.push_back(new coloredBorderFrame(startX - 1, startY - 1,
        102, 132, slot + 0x21a, g_systemPalette->m_data[31], 0x400));
}

VA(0x0054fea0, 0x42E)  // dc 0x11994c
int recruitUnit::open(int newPriority)
{
    message msg;
    int resCost[7];

    g_recruitWindow = new TRecruitWindow(143, 16, m_altResource, this);
    if (!g_recruitWindow)
        memError();

    g_recruitWindow->m_recruitInfo = this;
    m_numberToBuy = 0;
    m_totalGold = 0;
    m_totalResources = 0;

    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_PLAYER_PALETTE_COLORS;
    msg.m_codeY = 0;
    msg.m_extra = g_game->getLocalPlayerGamePos();
    g_recruitWindow->broadcastMessage(msg);

    const char* creatureName;
    if (m_monsterType >= 0 && m_monsterType <= 150)
        creatureName = g_creatureTypeTraits[m_monsterType].m_pluralName;
    else
        creatureName = "";
    sprintf(g_text, "%s %s",
        (*g_generalText)[GENERAL_TEXT_RECRUIT_TITLE], creatureName);
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = 0x226;
    msg.m_extraText = g_text;
    g_recruitWindow->broadcastMessage(msg);

    sprintf(g_text, "%d", m_goldPerTroop);
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = 0x200;
    msg.m_extraText = g_text;
    g_recruitWindow->broadcastMessage(msg);

    if (m_altResource != -1) {
        sprintf(g_text, "%d", m_resourcesPerTroop);
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_SET_TEXT;
        msg.m_codeY = 0x204;
        msg.m_extraText = g_text;
        g_recruitWindow->broadcastMessage(msg);

        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
        msg.m_codeY = 0x1fc;
        msg.m_extra = m_altResource;
        g_recruitWindow->broadcastMessage(msg);

        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
        msg.m_codeY = 0x211;
        msg.m_extra = m_altResource;
        g_recruitWindow->broadcastMessage(msg);
    } else {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_codeY = 0x1fc;
        msg.m_extra = widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;
        g_recruitWindow->broadcastMessage(msg);

        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_codeY = 0x211;
        msg.m_extra = widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;
        g_recruitWindow->broadcastMessage(msg);

        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_codeY = 0x204;
        msg.m_extra = widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;
        g_recruitWindow->broadcastMessage(msg);

        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_codeY = 0x213;
        msg.m_extra = widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;
        g_recruitWindow->broadcastMessage(msg);
    }

    g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);

    for (int slot = 3; slot >= 0; slot--) {
        if (g_recruitWindow->m_creatureWidgets[slot])
            update(1, slot);
    }

    g_windowManager->broadcastMessage(MESSAGE_WIDGET,
        widget::WIDGET_SET_STATUS, 0x7800,
        widget::WIDGET_DIMMED | widget::WIDGET_UPDATE);
    g_windowManager->addWindow(g_recruitWindow, -1, 1);

    g_recruitWindow->m_quantitySlider->setResolution(m_maxAvail + 1);
    m_updateNeeded = 0;
    m_errorExit = 0;
    g_recruitWindow->m_acceptButton->enable(0);

    getMonsterCost(m_monsterType, resCost);
    if (*m_numAvail == 0 || g_currentPlayer->m_resources[6] < resCost[6]) {
        g_recruitWindow->m_acceptButton->enable(0);
        g_recruitWindow->m_maximumButton->enable(0);
    }

    g_recruitSavedMenu = g_currMenu;
    kbChangeMenu(g_dfltMenu);

    m_priority = newPriority;
    m_id = 0x4000;
    m_status = STATUS_ACTIVE;
    strcpy(m_mgrName,
        DATA_COMPGEN(0x00682a18, recruitManagerName, "recruitManager"));

    if (g_remoteOn && !g_currentPlayer->isLocalHuman()) {
        g_recruitWindow->m_acceptButton->enable(0);
        g_recruitWindow->m_maximumButton->enable(0);
    }
    return 0;
}

// This body is what TYPES townManager's +0x13c: it calls
// TResourceDisplay::update through the member, so townmgr.h's
// old heroWindow* declaration is retyped here rather than cast around.
VA(0x005502d0, 0x8C)  // dc 0x119ce4
void recruitUnit::close()
{
    g_windowManager->removeWindow(g_recruitWindow);
    delete g_recruitWindow;
    // 0x7800 is the dialog-exit widget id the whole window family uses
    // (townmgr.h spells it EXIT_BUTTON_ID on every CAdvPopup here).
    g_windowManager->broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_CLEAR_STATUS,
        0x7800, widget::WIDGET_DIMMED | widget::WIDGET_UPDATE);
    if (m_type == RECRUIT_SOURCE_TOWN && m_updateNeeded && m_inTownMainScreen) {
        g_townManager->resetStrips();
        g_townManager->m_resourceDisplay->update(1, 0);
    }
    m_status = 0;
    kbChangeMenu(g_recruitSavedMenu);
}

// E:\gamedcs\recruit.cpp:492
// The last two arms are NOT a typo and NOT the inverse of
// SiegeMonsterToSiegeArtifact: retail's jump table sends the Ammo Cart
// artifact (5) to the First Aid Tent creature (0x93) and the First Aid
// Tent artifact (6) to the Ammo Cart creature (0x94), while the
// creature->artifact switch inlined into recruitUnit::Update pairs
// them the other way round (0x93->6, 0x94->5). Both directions are
// transcribed exactly as the bytes read them.
// E:\gamedcs\recruit.cpp:473 - no out-of-line retail body (dc
// 0x119d64): recruitUnit::Update is its only call site, so /Ob2
// inlined it there as the jump table at 0x5504d4 and the
// single-call-site STATIC rule dropped the standalone copy. `static`
// reproduces that absence.
static TArtifact siegeMonsterToSiegeArtifact(TCreatureType siegeMon)
{
    switch (siegeMon) {
    case CREATURE_CATAPULT:
        return ARTIFACT_CATAPULT;
    case CREATURE_BALLISTA:
        return ARTIFACT_BALLISTA;
    case CREATURE_FIRST_AID_TENT:
        return ARTIFACT_FIRST_AID_TENT;
    case CREATURE_AMMO_CART:
        return ARTIFACT_AMMO_CART;
    }
    return ARTIFACT_NONE;
}

VA(0x00550360, 0x3C)  // dc 0x119d98
TCreatureType siegeArtifactToCreature(TArtifact engine)
{
    switch (engine) {
    case ARTIFACT_CATAPULT:
        return CREATURE_CATAPULT;
    case ARTIFACT_BALLISTA:
        return CREATURE_BALLISTA;
    case ARTIFACT_FIRST_AID_TENT:
        return CREATURE_FIRST_AID_TENT;
    case ARTIFACT_AMMO_CART:
        return CREATURE_AMMO_CART;
    }
    return CREATURE_NONE;
}

// E:\gamedcs\recruit.cpp:511
// Windows Update matches all 55 CFG blocks and 29 calls. The remaining
// arithmetic pair at +0x390 loads goldPerTroop before numberToBuy, whereas
// retail loads numberToBuy first; reversing the source multiplication or
// splitting it into assignment and multiplication is byte-flat under VC6.
// Mac code0+0x14f85c loads goldPerTroop before numberToBuy too; this does
// not decide VC6's load order. Reviewed retail ABI aliases for gpCurrentPlayer
// and gSystemPalette make the named relocations agree but do not change the
// 99.989845% score, so this source keeps the canonical global names.
// The reviewed Mac Update and native-header candidate are both 1484 B;
// 1367/1484 bytes and all 33 ordered calls agree. Mac's 20 uses of the
// recruit-window pointer resolve directly to the TU's zero-filled TOC cell.
// Remaining Mac differences are register and stack-home choices; this does
// not decide VC6's load order at the Windows multiplication sites.
// DC line 521 calls TTextResource::operator[] for the recruit title. Restoring
// that canonical source call is VC6 byte-flat and clears its audit finding.
VA(0x005503a0, 0x594)  // anchor-global, dc 0x119dcc
void recruitUnit::update(unsigned char newMonster, long slot)
{
    message msg;

    if (slot == -1)
        slot = m_selectedPosition;

    updateCost();

    sprintf(g_text, "%s %s",
        (*g_generalText)[GENERAL_TEXT_RECRUIT_TITLE],
        getArmyName(m_monsterType, 2));
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = 0x226;
    msg.m_extraText = g_text;
    g_recruitWindow->broadcastMessage(msg);

    m_numAvail = m_available[slot];
    if (g_creatureTypeTraits[m_monsterType].m_attributes & g_ctaSiegeWeapon) {
        *m_numAvail = 1 - m_thisHero->hasArtifact(
            siegeMonsterToSiegeArtifact(m_monsterType));
        if (*m_numAvail < 0)
            *m_numAvail = 0;
        sprintf(g_text, "%d", *m_numAvail - m_numberToBuy);
    } else {
        sprintf(g_text, "%d", *m_numAvail - m_numberToBuy);
    }
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = 0x209;
    msg.m_extraText = g_text;
    g_recruitWindow->broadcastMessage(msg);

    long maxGold = g_currentPlayer->m_resources[6] / m_goldPerTroop;
    if (m_altResource != -1) {
        int byResource = g_currentPlayer->m_resources[m_altResource] / m_resourcesPerTroop;
        m_maxAvail = maxGold < byResource ? maxGold : byResource;
    } else {
        m_maxAvail = maxGold;
    }
    if (m_maxAvail > *m_numAvail)
        m_maxAvail = *m_numAvail;
    m_numberToBuy = min(m_numberToBuy, m_maxAvail);

    // NAME CONTRADICTED, storage correct: 0x69954c is declared
    // `bVideoPaused` in kbwin.h, which flags all of its .bss names as
    // provisional. The gate here - disable the buy controls when the
    // acting player is not the local one - is a multiplayer test, and
    // the global's 275 image-wide references cluster on remote.obj's
    // CChatEdit (16), type_AI_player::make_gift,
    // combatManager::is_computer_action, SaveGame and the advManager
    // turn machinery. kbwin's own two uses (don't pause video, don't
    // block in GetMessage while iconic) read the same way for a
    // network-game flag. Renaming a 275-reference global is the
    // owning lane's call, so the call site keeps the declared name.
    if (g_remoteOn && !g_currentPlayer->isLocalHuman()) {
        g_recruitWindow->m_acceptButton->enable(0);
        g_recruitWindow->m_maximumButton->enable(0);
        g_recruitWindow->m_quantitySlider->enable(0);
    } else {
        g_recruitWindow->m_acceptButton->enable(m_numberToBuy != 0 && m_maxAvail > 0 && !m_viewOnly);
        g_recruitWindow->m_maximumButton->enable(m_maxAvail > 0 && !m_viewOnly);
        g_recruitWindow->m_quantitySlider->enable(m_maxAvail > 0 && !m_viewOnly);
    }

    if (newMonster) {
        g_recruitWindow->m_quantitySlider->setResolution(m_maxAvail + 1);
        g_recruitWindow->m_quantitySlider->setState(m_numberToBuy);
    }

    sprintf(g_text, "%d", m_numberToBuy);
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = RECRUIT_QUANTITY_ID;
    msg.m_extraText = g_text;
    g_recruitWindow->broadcastMessage(msg);

    m_totalGold = m_numberToBuy * m_goldPerTroop;
    sprintf(g_text, "%d", m_totalGold);
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = 0x212;
    msg.m_extraText = g_text;
    g_recruitWindow->broadcastMessage(msg);

    sprintf(g_text, "%d", m_goldPerTroop);
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = 0x200;
    msg.m_extraText = g_text;
    g_recruitWindow->broadcastMessage(msg);

    if (m_altResource == -1)
        m_resourcesPerTroop = 0;
    m_totalResources = m_numberToBuy * m_resourcesPerTroop;
    sprintf(g_text, "%d", m_totalResources);
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = 0x213;
    msg.m_extraText = g_text;
    g_recruitWindow->broadcastMessage(msg);

    sprintf(g_text, "%d", m_resourcesPerTroop);
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = 0x204;
    msg.m_extraText = g_text;
    g_recruitWindow->broadcastMessage(msg);

    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_COLOR;
    msg.m_codeY = RECRUIT_CREATURE_0_ID;
    msg.m_extra = g_systemPalette->m_data[31];
    g_recruitWindow->broadcastMessage(msg);

    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_COLOR;
    msg.m_codeY = RECRUIT_CREATURE_1_ID;
    msg.m_extra = g_systemPalette->m_data[31];
    g_recruitWindow->broadcastMessage(msg);

    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_COLOR;
    msg.m_codeY = RECRUIT_CREATURE_2_ID;
    msg.m_extra = g_systemPalette->m_data[31];
    g_recruitWindow->broadcastMessage(msg);

    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_COLOR;
    msg.m_codeY = RECRUIT_CREATURE_3_ID;
    msg.m_extra = g_systemPalette->m_data[31];
    g_recruitWindow->broadcastMessage(msg);

    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_COLOR;
    msg.m_codeY = m_selectedPosition + RECRUIT_CREATURE_0_ID;
    msg.m_extra = g_systemPalette->m_data[36];
    g_recruitWindow->broadcastMessage(msg);
}

// E:\gamedcs\recruit.cpp:666 / :693. Dreamcast and Mac retain both helpers
// as functions. VC6 expands them into Main and /OPT:REF leaves no standalone
// recruit-band row. Keeping setRolloverText inline is a working hypothesis:
// with its body ordinary and visible here, VC6 retains the call and Main drops
// from 99.95448% to 87.196785%. The small exit helper needs no inline keyword:
// VC6 auto-inlines it while Mac retains its call.
inline void recruitUnit::setRolloverText(int codeY)
{
    switch (codeY) {
    case RECRUIT_MAXIMUM_ID:
        strcpy(g_text, g_recruitHelp[0].m_text);
        break;
    case RECRUIT_CANCEL_ID:
        strcpy(g_text, g_recruitHelp[2].m_text);
        break;
    case RECRUIT_ACCEPT_ID:
        strcpy(g_text, g_recruitHelp[1].m_text);
        break;
    default:
        strcpy(g_text, "");
        break;
    }

    message update;
    update.m_extraText = g_text;
    g_recruitWindow->broadcastMessage(MESSAGE_WIDGET,
        widget::WIDGET_SET_TEXT, 0x231, update.m_extra);
    g_recruitWindow->drawWindow(0, 0x230, 0x231);
    g_windowManager->updateScreen(g_recruitWindow->m_x + 8,
        g_recruitWindow->m_y + 0x172, 0x1d4, 0x12);
}

int exitRecruitUnit(message& msg)
{
    msg.m_id = MESSAGE_EXECUTIVE;
    msg.m_codeX = EXECUTIVE_COMMAND_RETURN_RESULT;
    return MESSAGE_DISPATCH_FORWARD;
}

// E:\gamedcs\recruit.cpp:704
// The message command map is fixed by the retail switch tables: 0x20e is the
// typed quantity, 0x214 the maximum button, 0x21a..0x21d the four creature
// borders, and 0x7801/0x7802 the cancel/accept controls. Right-select is
// carried independently in qualifier bit 9 and opens a quick army view.
// Residual (99.11%): retail and this reconstruction have the same 119
// executable blocks through the common epilogue; 117 are instruction-count
// exact. The head delta is C1/C2 scheduling: retail reloads glTimers[0]
// after selecting max(100, elapsed), while our CL retains the subtraction
// load in ecx and consequently rotates the four mon_type loads. Compound,
// explicit-assignment, reversed-addend, and split elapsed spellings compile
// identically. `homm3 vc6 why-reg --model` classifies the remaining
// caller-saved divergence as front-end handle state after its only proposed
// naming edit is copy-propagated. The other byte delta is exception-state
// numbering (retail's four view windows are states 0..3 and the temporary
// format_string is 4; ours numbers the temporary first), plus the resulting
// switch-table tail bytes. Reordering the cases to force those state numbers
// regresses the body to 87.17%, so the semantic source order stays intact.
// A target-local 240-trial follow-up on 2026-09-07 crossed the twelve
// non-include Gruntz state families; every candidate remained at 99.0924%.
// Those sampled states did not recover the 99.1111% HIST island; no
// synthetic declaration or unsupported local is kept.
// Timeout and remote-popup rejection share one abortDialog result and the
// existing dialog-routing tail. An unsigned-char result preserves 99.0924%
// and removes the jump; bool/int forms score 98.6908/98.7979%. Duplicating
// the tail, including a copy using exitRecruitUnit, scores 95.0161%.
// DC lines 707/723 call ExitRecruitUnit, but its older four-store body also
// sets the dialog result and codeY: retail's other three helper expansions
// write only id/codeX, so those PC routing differences remain explicit.
// Current 99.95% build has 119/119 exact CFG blocks. Retail reuses one
// TViewArmyWindow stack slot for the first three arms and gives the fourth a
// second slot; VC6 currently reuses one for all four. DC and the bounded Mac
// counterpart each retain four distinct shadowed locals, so this does not
// justify a synthetic Windows-only object or scope.
// The admitted Mac Main pair is currently unavailable: CodeWarrior emits an
// anonymous 16-byte zero template for DC's const monType[4] array that Mac
// retail never loads; Main also expands setRolloverText in the candidate while
// Mac retail calls its retained helper.
VA(0x00550940, 0xA08)  // anchor-callee + switch-table bracket, dc 0x11a30c
int recruitUnit::main(message& msg)
{
    unsigned char abortDialog = g_turnDuration.isExpired();

    if (!abortDialog && g_remoteOn) {
        unsigned char msgReceived = 0;
        CNetMsgHandler* handler = g_dPlay->getNetMsgHandler();
        if (handler) {
            handler->checkHandleNet(1, &msgReceived);
            if (msgReceived && handler->getAbortPopupMsg())
                abortDialog = 1;
        }
    }

    if (abortDialog) {
        g_windowManager->m_dialogReturn = 0x7800;
        msg.m_id = MESSAGE_EXECUTIVE;
        msg.m_codeX = EXECUTIVE_COMMAND_RETURN_RESULT;
        msg.m_codeY = widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }

    long elapsed = GameTime::get()
                   - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
    if (elapsed >= 0) {
        g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] +=
            max(100, elapsed);

        const TCreatureType monType[4] = {
            m_monType1, m_monType2, m_monType3, m_monType4
        };
        for (int slot = 0; slot < 4; slot++) {
            if (g_recruitWindow->m_creatureWidgets[slot]) {
                if (isSiegeWeapon(monType[slot]))
                    g_recruitWindow->m_creatureWidgets[slot]
                        ->nextRandomSiegeEngineFrame();
                else
                    g_recruitWindow->m_creatureWidgets[slot]
                        ->nextRandomFrame();
            }
        }
        g_recruitWindow->drawWindow(1, WINDOW_ALL_WIDGETS_LOW,
                                    WINDOW_ALL_WIDGETS_HIGH);
    }

    int exitFlag = (static_cast<unsigned int>(msg.m_qualifier) >> 9) & 1;
    switch (msg.m_id) {
    case MESSAGE_WIDGET:
        switch (msg.m_codeX) {
        case widget::WIDGET_SELECT:
        case widget::WIDGET_RIGHT_SELECT:
            switch (msg.m_codeY) {
            case RECRUIT_QUANTITY_ID:
                if (exitFlag)
                    break;
                msg.m_codeX = widget::WIDGET_GET_TEXT;
                g_recruitWindow->broadcastMessage(msg);
                m_numberToBuy = atoi(msg.m_extraText);
                if (m_numberToBuy < 0)
                    m_numberToBuy = 0;
                if (m_numberToBuy > m_maxAvail)
                    m_numberToBuy = m_maxAvail;
                g_recruitWindow->m_quantitySlider->setState(m_numberToBuy);
                g_recruitWindow->m_acceptButton->enable(m_numberToBuy != 0);
                update(0, -1);
                break;

            case RECRUIT_CREATURE_0_ID:
                if (m_selectedPosition != RECRUIT_SLOT_0 && !exitFlag) {
                    m_selectedPosition = RECRUIT_SLOT_0;
                    m_monsterType = m_monType1;
                    m_numberToBuy = 0;
                    g_recruitWindow->m_quantitySlider->setState(0);
                    g_recruitWindow->m_acceptButton->enable(m_numberToBuy != 0);
                    update(1, 0);
                } else {
                    TViewArmyWindow viewArmyWindow(
                        m_monType1, 0x77, 0x20, !exitFlag);
                    if (exitFlag)
                        viewArmyWindow.quickView();
                    else
                        viewArmyWindow.doModal();
                }
                break;

            case RECRUIT_CREATURE_1_ID:
                if (m_selectedPosition != RECRUIT_SLOT_1 && !exitFlag) {
                    m_selectedPosition = RECRUIT_SLOT_1;
                    m_monsterType = m_monType2;
                    m_numberToBuy = 0;
                    g_recruitWindow->m_quantitySlider->setState(0);
                    g_recruitWindow->m_acceptButton->enable(m_numberToBuy != 0);
                    update(1, 1);
                } else {
                    TViewArmyWindow viewArmyWindow(
                        m_monType2, 0x77, 0x20, !exitFlag);
                    if (exitFlag)
                        viewArmyWindow.quickView();
                    else
                        viewArmyWindow.doModal();
                }
                break;

            case RECRUIT_CREATURE_2_ID:
                if (m_selectedPosition != RECRUIT_SLOT_2 && !exitFlag) {
                    m_selectedPosition = RECRUIT_SLOT_2;
                    m_monsterType = m_monType3;
                    m_numberToBuy = 0;
                    g_recruitWindow->m_quantitySlider->setState(0);
                    g_recruitWindow->m_acceptButton->enable(m_numberToBuy != 0);
                    update(1, 2);
                } else {
                    TViewArmyWindow viewArmyWindow(
                        m_monType3, 0x77, 0x20, !exitFlag);
                    if (exitFlag)
                        viewArmyWindow.quickView();
                    else
                        viewArmyWindow.doModal();
                }
                break;

            case RECRUIT_CREATURE_3_ID:
                if (m_selectedPosition != RECRUIT_SLOT_3 && !exitFlag) {
                    m_selectedPosition = RECRUIT_SLOT_3;
                    m_monsterType = m_monType4;
                    m_numberToBuy = 0;
                    g_recruitWindow->m_quantitySlider->setState(0);
                    g_recruitWindow->m_acceptButton->enable(m_numberToBuy != 0);
                    update(1, 3);
                } else {
                    TViewArmyWindow viewArmyWindow(
                        m_monType4, 0x77, 0x20, !exitFlag);
                    if (exitFlag)
                        viewArmyWindow.quickView();
                    else
                        viewArmyWindow.doModal();
                }
                break;
            }
            g_recruitWindow->drawWindow(1, WINDOW_ALL_WIDGETS_LOW,
                                        WINDOW_ALL_WIDGETS_HIGH);
            break;

        case widget::WIDGET_DESELECT:
            switch (msg.m_codeY) {
            case RECRUIT_MAXIMUM_ID:
                if (exitFlag)
                    break;
                m_numberToBuy = m_maxAvail;
                g_recruitWindow->m_quantitySlider->setState(m_numberToBuy);
                g_recruitWindow->m_acceptButton->enable(m_numberToBuy != 0);
                update(0, -1);
                g_recruitWindow->drawWindow(1, WINDOW_ALL_WIDGETS_LOW,
                                            WINDOW_ALL_WIDGETS_HIGH);
                break;

            case RECRUIT_ACCEPT_ID:
                if (exitFlag)
                    break;
                if (m_numberToBuy == 0 && m_monType2 == CREATURE_NONE)
                    return exitRecruitUnit(msg);

                if (g_creatureTypeTraits[m_monsterType].m_attributes
                    & g_ctaSiegeWeapon) {
                    if (m_thisHero->getNumberInBackpack(1) + m_numberToBuy
                        > 64) {
                        normalDialog((*g_generalText)[GENERAL_TEXT_RECRUIT_BACKPACK_FULL],
                            1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                        break;
                    }

                    for (int i = 0; i < m_numberToBuy; i++) {
                        if (m_monsterType == CREATURE_BALLISTA) {
                            type_artifact artifact(ARTIFACT_BALLISTA);
                            m_thisHero->giveArtifact(&artifact, 1, 1);
                        } else if (m_monsterType == CREATURE_FIRST_AID_TENT) {
                            type_artifact artifact(ARTIFACT_FIRST_AID_TENT);
                            m_thisHero->giveArtifact(&artifact, 1, 1);
                        } else if (m_monsterType == CREATURE_AMMO_CART) {
                            type_artifact artifact(ARTIFACT_AMMO_CART);
                            m_thisHero->giveArtifact(&artifact, 1, 1);
                        }
                    }
                } else if (m_currArmyGroup->canJoin(m_monsterType)) {
                    m_currArmyGroup->add(m_monsterType, m_numberToBuy, -1);
                } else {
                    if (m_currArmyGroupIsTownGarrison) {
                        normalDialog((*g_generalText)[GENERAL_TEXT_RECRUIT_GARRISON_FULL],
                            1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                    } else {
                        const char* creatureName =
                            getArmyName(m_monsterType, m_numberToBuy);
                        normalDialog(formatString(
                            (*g_generalText)[GENERAL_TEXT_RECRUIT_INSUFFICIENT_PROVISIONS_FORMAT], creatureName).c_str(),
                            1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                    }
                    break;
                }

                g_currentPlayer->m_resources[6] -=
                    m_goldPerTroop * m_numberToBuy;
                if (m_altResource != -1)
                    g_currentPlayer->m_resources[m_altResource] -=
                        m_resourcesPerTroop * m_numberToBuy;
                *m_numAvail -= static_cast<short>(m_numberToBuy);
                m_numberToBuy = 0;
                g_recruitWindow->m_quantitySlider->setState(0);
                g_recruitWindow->m_acceptButton->enable(m_numberToBuy != 0);

                if (m_monType2 == CREATURE_NONE
                    || m_type == RECRUIT_SOURCE_TOWN)
                    return exitRecruitUnit(msg);

                update(1, m_selectedPosition);
                g_advManager->updBottomView(1, 1, 1);
                g_recruitWindow->drawWindow(1, WINDOW_ALL_WIDGETS_LOW,
                                            WINDOW_ALL_WIDGETS_HIGH);
                break;

            case RECRUIT_CANCEL_ID:
                if (exitFlag)
                    break;
                m_numberToBuy = 0;
                g_recruitWindow->m_acceptButton->enable(0);
                return exitRecruitUnit(msg);
            }
            break;

        }
        break;

    case MESSAGE_MOUSE_MOVE:
        g_windowManager->convertToHover(msg);
        if (msg.m_codeY != g_windowManager->m_lastHover) {
            g_windowManager->m_lastHover = msg.m_codeY;
            setRolloverText(msg.m_codeY);
        }
        break;
    }

    return MESSAGE_DISPATCH_CONSUME;
}

// E:\gamedcs\recruit.cpp:1082
// Dreamcast and Mac keep this source-visible helper out of line. Without an
// explicit inline specifier, VC6 still expands it in all three constructors;
// both admitted Mac constructors then match exactly with their updateCost
// calls intact. Raw NB11 names the sole surviving local `resCost`, while the
// body calls the exact GetMonsterCost helper before deriving the two costs.
void recruitUnit::updateCost()
{
    int resCost[7];
    getMonsterCost(m_monsterType, resCost);
    m_goldPerTroop = resCost[6];

    int i;
    for (i = 0; i < 6; i++) {
        if (resCost[i] != 0)
            break;
    }
    if (i < 6) {
        m_altResource = i;
        m_resourcesPerTroop = resCost[i];
    } else {
        m_altResource = -1;
        m_resourcesPerTroop = 0;
    }
}

// E:\gamedcs\recruit.cpp:1120
// `ret 0x28` = 40 argument bytes = the ten Dreamcast parameters, and
// each one lands on the field the DC roster names: [ebp+8] -> +0x98,
// the byte at [ebp+0xc] -> +0x9c, then the four (type, count-pointer)
// pairs into +0x5c/+0x6c .. +0x68/+0x78, with the first pair also
// seeding monsterType/numAvail.
// DC lines 1121..1141 and Mac 0:0x150804..0x150848 preserve the full
// field-store order below. The earlier isolated head-store probes left
// type/viewOnly and selectedPosition outside that sequence. VC6 now matches
// the 0x101-byte retail constructor exactly, including both calls.
VA(0x00551350, 0x101)  // anchor-callee(baseManager ctor) + anchor-vtable 0x640c70, dc 0x11ad04
recruitUnit::recruitUnit(armyGroup* newGroup, unsigned char groupIsTownGarrison,
    TCreatureType monType1, short* numMon1,
    TCreatureType monType2, short* numMon2,
    TCreatureType monType3, short* numMon3,
    TCreatureType monType4, short* numMon4)
{
    m_type = -1;
    m_viewOnly = 0;
    m_inTownMainScreen = 0;
    m_thisHero = 0;
    m_currArmyGroup = newGroup;
    m_currArmyGroupIsTownGarrison = groupIsTownGarrison;
    m_monsterType = monType1;
    m_numAvail = numMon1;
    m_selectedPosition = 0;
    m_monType1 = monType1;
    m_monType2 = monType2;
    m_monType3 = monType3;
    m_monType4 = monType4;
    m_available[0] = numMon1;
    m_available[1] = numMon2;
    m_available[2] = numMon3;
    m_available[3] = numMon4;
    g_timers[0] = GameTime::get() + 100;
    updateCost();
}

// E:\gamedcs\recruit.cpp:1158
// `ret 0x24` = the nine hero-flavoured parameters; identical body with
// thisHero taking the armyGroup pair's place.
// DC lines 1159..1179 and Mac 0:0x1508c4..0x15090c establish the complete
// field-store order below. VC6 matches the 0xFE-byte retail body exactly;
// the earlier thisHero-first-only probe omitted the surrounding store order.
VA(0x00551460, 0xFE)  // anchor-callee(baseManager ctor) + anchor-vtable 0x640c70, dc 0x11adb4
recruitUnit::recruitUnit(hero* thisHero,
    TCreatureType monType1, short* numMon1,
    TCreatureType monType2, short* numMon2,
    TCreatureType monType3, short* numMon3,
    TCreatureType monType4, short* numMon4)
{
    // DC lines 1159..1179 and Mac 0:0x1508c4..0x15090c put the source
    // fields in this order after baseManager construction.
    m_type = -1;
    m_viewOnly = 0;
    m_inTownMainScreen = 0;
    m_thisHero = thisHero;
    m_currArmyGroup = 0;
    m_currArmyGroupIsTownGarrison = 0;
    m_monsterType = monType1;
    m_numAvail = numMon1;
    m_selectedPosition = 0;
    m_monType1 = monType1;
    m_monType2 = monType2;
    m_monType3 = monType3;
    m_monType4 = monType4;
    m_available[0] = numMon1;
    m_available[1] = numMon2;
    m_available[2] = numMon3;
    m_available[3] = numMon4;
    g_timers[0] = GameTime::get() + 100;
    updateCost();
}

VA(0x00551560, 0x14B)  // dc 0x11ae58
recruitUnit::recruitUnit(town* newTown, int newDwellingIndex, int inInTownMainScreen)
{
    m_inTownMainScreen = inInTownMainScreen;
    m_type = RECRUIT_SOURCE_TOWN;
    m_thisHero = 0;
    m_monsterType = g_townDwellingCreatures[newTown->m_type * TOWN_DWELLING_SLOTS
                                         + newDwellingIndex];
    m_numAvail = &newTown->m_population[newDwellingIndex];
    m_currArmyGroup = const_cast<armyGroup*>(&newTown->getArmy());
    m_currArmyGroupIsTownGarrison = 1;
    m_viewOnly = newTown->m_owner != g_netLocalGamePos;
    m_monType2 = (TCreatureType)-1;
    m_monType3 = (TCreatureType)-1;
    m_monType4 = (TCreatureType)-1;
    m_selectedPosition = 0;
    m_monType1 = m_monsterType;
    m_available[0] = m_numAvail;
    m_available[1] = 0;
    m_available[2] = 0;
    m_available[3] = 0;
    if (newDwellingIndex >= TOWN_DWELLING_COUNT) {
        m_monType2 = g_townDwellingCreatures[newTown->m_type * TOWN_DWELLING_SLOTS
                                          + newDwellingIndex - TOWN_DWELLING_COUNT];
        m_available[1] = m_numAvail;
    }
    g_timers[0] = GameTime::get() + 100;
    updateCost();
}

// E:\gamedcs\recruit.cpp:1219. Dreamcast and Mac retain this constructor;
// Mac quickViewRecruit calls it at 0:0x150cc8. VC6 expands this ordinary
// definition into the sole Windows caller at 0x551780 (45/45 calls agree).
TRecruitQuickWindow::TRecruitQuickWindow(int x2, int y2)
    : heroWindow(x2, y2, 160, 320, 0x12)
{
    m_widgets.reserve(49);
}

VA_COMPGEN(0x005516b0, 0x21, SCALAR_DELETING_DTOR, TRecruitQuickWindow)

VA(0x005516e0, 0x6B)  // dc 0x11af98
TRecruitQuickWindow::~TRecruitQuickWindow()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

VA(0x00551750, 0x24)  // dc 0x11affc
void quickViewRecruit(town* newTown, int newDwellingIndex)
{
    quickViewRecruit(
        g_townDwellingCreatures[newTown->m_type * TOWN_DWELLING_SLOTS
                               + newDwellingIndex],
        &newTown->m_population[newDwellingIndex]);
}

VA(0x00551780, 0x641)  // dc 0x11b028
void quickViewRecruit(TCreatureType monType, short* numMon)
{
    message msg;
    int cost[7];
    getMonsterCost(monType, cost);

    int i;
    for (i = 0; i < 6; i++) {
        if (cost[i] != 0)
            break;
    }
    int altResource;
    int resourcesPerTroop;
    int resourceX;
    if (i < 6) {
        altResource = i;
        resourcesPerTroop = cost[i];
        resourceX = 0;
    } else {
        altResource = -1;
        resourcesPerTroop = 0;
        resourceX = 24;
    }

    TRecruitQuickWindow* recruitWindow =
        new TRecruitQuickWindow(356, 16);
    if (!recruitWindow)
        memError();

    recruitWindow->addWidget(
        new bitmapBorder(0, 0, 161, 324, 0,
            DATA_COMPGEN(0x00682a28, quickRecruitBackground,
                "crtoinfo.pcx"),
            0x800), -1);

    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_PLAYER_PALETTE_COLORS;
    msg.m_codeY = 0;
    msg.m_extra = g_game->getLocalPlayerGamePos();
    recruitWindow->broadcastMessage(msg);

    recruitWindow->addWidget(new textWidget(0, 20, 161, 20,
        monType >= 0 && monType <= 150
            ? g_creatureTypeTraits[monType].m_pluralName
            : DATA_COMPGEN(0x00691210, quickRecruitEmptyText, ""),
        DATA_COMPGEN(0x0065f2f8, quickRecruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x222,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8), -1);

    recruitWindow->addWidget(new bitmapBorder(30, 44, 100, 130, 0x21e,
        g_creatureBackgrounds[
            g_game->m_gameVersion == 0
                && isBaseElemental(monType)
            ? -1 : g_creatureTypeTraits[monType].m_townType],
        0x800), -1);
    recruitWindow->addWidget(new iconWidget(30, 44, 100, 130, 0x216,
        g_creatureTypeTraits[monType].m_spriteName,
        0, 2, 0, 0, iconWidget::ICON_STYLE_CREATURE), -1);

    sprintf(g_text,
        DATA_COMPGEN(0x00660c98, quickRecruitAvailabilityFormat, "%s %d"),
        (*g_generalText)[GENERAL_TEXT_CREATURES_AVAILABLE_LABEL], *numMon);
    recruitWindow->addWidget(new textWidget(30, 182, 100, 17, g_text,
        DATA_COMPGEN(0x0065f2f8, quickRecruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x209,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8), -1);
    recruitWindow->addWidget(new textWidget(32, 218, 96, 19,
        (*g_generalText)[GENERAL_TEXT_COST_PER_TROOP],
        DATA_COMPGEN(0x0065f2f8, quickRecruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x1f4,
        font::CENTER_JUSTIFIED | font::VERT_CENTER_JUSTIFIED, 0, 8), -1);

    recruitWindow->addWidget(new iconWidget(resourceX + 40, 238, 32, 32,
        0x1f8,
        DATA_COMPGEN(0x00660224, quickRecruitResourceSprite,
            "resource.def"),
        6, 0, 0, 0,
        iconWidget::ICON_STYLE_PLAIN), -1);
    recruitWindow->addWidget(new iconWidget(90, 238, 32, 32, 0x1fc,
        DATA_COMPGEN(0x00660224, quickRecruitResourceSprite,
            "resource.def"),
        6, 0, 0, 0, iconWidget::ICON_STYLE_PLAIN), -1);
    recruitWindow->addWidget(new textWidget(resourceX + 40, 273, 32, 17,
        DATA_COMPGEN(0x00691210, quickRecruitEmptyText, ""),
        DATA_COMPGEN(0x0065f2f8, quickRecruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x200,
        font::CENTER_JUSTIFIED, 0, 8), -1);
    recruitWindow->addWidget(new textWidget(90, 273, 32, 17,
        DATA_COMPGEN(0x00691210, quickRecruitEmptyText, ""),
        DATA_COMPGEN(0x0065f2f8, quickRecruitSmallFont, "smalfont.fnt"),
        font::PRIMARY, 0x204,
        font::CENTER_JUSTIFIED, 0, 8), -1);

    sprintf(g_text,
        DATA_COMPGEN(0x00660a1c, quickRecruitDecimalFormat, "%d"),
        cost[6]);
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = 0x200;
    msg.m_extraText = g_text;
    recruitWindow->broadcastMessage(msg);

    if (altResource != -1) {
        sprintf(g_text,
            DATA_COMPGEN(0x00660a1c, quickRecruitDecimalFormat, "%d"),
            resourcesPerTroop);
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_SET_TEXT;
        msg.m_codeY = 0x204;
        msg.m_extraText = g_text;
        recruitWindow->broadcastMessage(msg);

        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
        msg.m_codeY = 0x1fc;
        msg.m_extra = altResource;
        recruitWindow->broadcastMessage(msg);
    } else {
        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_codeY = 0x1fc;
        msg.m_extra = widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;
        recruitWindow->broadcastMessage(msg);

        msg.m_id = MESSAGE_WIDGET;
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_codeY = 0x204;
        msg.m_extra = widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN;
        recruitWindow->broadcastMessage(msg);
    }

    g_windowManager->doQuickView(recruitWindow);
}

// COMDAT pairing: vector<widget*>::insert, agreement 0.985. Three addresses
// resembled this COMDAT and the CALLER SET settles it: 0x14d120 is reached
// from TAdventureMapWindow's and TAdventureOptionsWindow's constructors
// among others - window ctors pushing widgets, in units all over the tree,
// which is what a single shared COMDAT looks like. combatwindow 0x73600 has
// exactly ONE caller, TCombatWindow::combat_message, so it is that unit's
// own vector and not this COMDAT; towngatewindow 0x5c2400 turned out to be
// AddTown with the insert expanded into it.
VA_COMPGEN(0x0054d120, 0x209, VECTOR_INSERT, widget)
