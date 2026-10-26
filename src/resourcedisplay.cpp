#include "va.h"

#include <stdio.h>

#include "resourcedisplay.h"

#include "border.h"
#include "exec.h"
#include "game.h"
#include "kb.h"
#include "terrain.h"
#include "textwdgt.h"
#include "town.h"
#include "window.h"

// Retail .rdata 0x641008, immediately before this class's vtable.
// The seven dwords are read as 0..6 and Update uses them to map
// display slots onto playerData::resources.
DATA(0x00641008)
static const int g_resourceDisplayOrder[NUM_RESOURCES] = {
    WOOD, MERCURY, ORE, SULFUR, CRYSTAL, GEMS, GOLD
};

// E:\gamedcs\resourcedisplay.cpp:40
// `isSmall(is_small)` is a real member initializer.
VA(0x00558ba0, 0x2A1)
DC_ADDRESS(0x120c54, 0x294)
MAC_ADDRESS(0x151684, 0x328)  // anchor-global
TResourceDisplay::TResourceDisplay(heroWindow* parent, bool isSmall)
    : m_isSmall(isSmall)
{
    if (m_isSmall) {
        initialize(7, 0x23f, 0x2e2, 0x16, parent);
    } else {
        initialize(3, 0x23f, 0x31a, 0x16, parent);
    }
    if (isSmall) {
        m_backgroundWidget = new bitmapBorder(
            0, 0, 0x2e2, 0x16, 1000, "kresbar.pcx", 0x800);
    } else {
        m_backgroundWidget = new bitmapBorder(
            0, 0, 0x31a, 0x16, 1000, "aresbar.pcx", 0x800);
    }

    m_backgroundWidget->setPlayerPaletteColors(
        g_game->getLocalPlayerGamePos());
    addWidget(m_backgroundWidget, -1);

    int textX;
    int spacing;
    int borderWidth;
    if (isSmall) {
        textX = 0x1d;
        spacing = 0x4c;
        borderWidth = 0x1a;
    } else {
        textX = 0x25;
        spacing = 0x54;
        borderWidth = 0x1e;
    }

    int textId = 0x3e9;
    int borderId = 0x3f1;
    for (int i = 0; i < NUM_RESOURCES; ++i) {
        m_resourceWidgets[i] = new textWidget(
            textX, 3, 0x32, 0x12, 0, "smalfont.fnt", font::WHITE,
            textId, 0, 0, 8);
        addWidget(m_resourceWidgets[i], -1);
        m_resourceIconWidgets[i] = new border(
            textX - borderWidth, 3, borderWidth, 0x12,
            borderId, 1);
        addWidget(m_resourceIconWidgets[i], -1);
        textX += spacing;
        ++textId;
        ++borderId;
    }

    if (isSmall) {
        m_dayWidget = new textWidget(
            0x22b, 3, 0xb2, 0x12, 0, "smalfont.fnt", font::WHITE,
            0x3f0, 1, 0, 8);
    } else {
        m_dayWidget = new textWidget(
            0x25f, 3, 0xb4, 0x12, 0, "smalfont.fnt", font::WHITE,
            0x3f0, 1, 0, 8);
    }
    addWidget(m_dayWidget, -1);
}

VA_COMPGEN(0x00558e50, 0x21, SCALAR_DELETING_DTOR, TResourceDisplay)

VA(0x00558e80, 0x95)
DC_ADDRESS(0x120ee8, 0xb8)
MAC_ADDRESS(0x1519ac, 0x120)
TResourceDisplay::~TResourceDisplay()
{
    if (m_backgroundWidget)
        delete m_backgroundWidget;
    for (int i = 0; i < NUM_RESOURCES; ++i) {
        if (m_resourceWidgets[i])
            delete m_resourceWidgets[i];
        if (m_resourceIconWidgets[i])
            delete m_resourceIconWidgets[i];
    }
    if (m_dayWidget) {
        delete m_dayWidget;
        m_dayWidget = 0;
    }
}

// E:\gamedcs\resourcedisplay.cpp:150
// Dreamcast has three parameters (draw, update, inMap); Complete takes two,
// as recruitUnit::Close's two-argument call shows. DC's third parameter
// guards `hide()` calls for each resource widget and the status widget;
// Complete deletes both guard and calls. DC public
// ?Update@TResourceDisplay@@QAAX_N00@Z proves native bool flags despite the
// lowered T_UCHAR formal records. DC156 proves playerData&; DC168/201 prove
// operator[] for the three date labels in Update/Clear.
VA(0x00558f20, 0xF3)
DC_ADDRESS(0x120fa0, 0x112)
MAC_ADDRESS(0x151acc, 0x158)  // anchor-global
void TResourceDisplay::update(bool drawRequested, bool update)
{
    int playerPos = g_curWatchPlayer;
    playerData& player = g_game->m_players[playerPos];
    m_backgroundWidget->setPlayerPaletteColors(playerPos);
    for (int i = 0; i < NUM_RESOURCES; ++i) {
        sprintf(g_text, "%d", player.m_resources[g_resourceDisplayOrder[i]]);
        m_resourceWidgets[i]->setText(g_text);
    }

    const TTextResource* labels = g_generalText;
    sprintf(g_text, "%s: %d, %s: %d, %s: %d",
        (*labels)[GENERAL_TEXT_CALENDAR_MONTH],
        static_cast<unsigned short>(g_game->m_month),
        (*labels)[GENERAL_TEXT_CALENDAR_WEEK],
        static_cast<unsigned short>(g_game->m_week),
        (*labels)[GENERAL_TEXT_CALENDAR_DAY],
        static_cast<unsigned short>(g_game->m_day));
    m_dayWidget->setText(g_text);
    if (drawRequested)
        draw(update, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

VA(0x00559020, 0xA4)
DC_ADDRESS(0x1210b4, 0x138)
MAC_ADDRESS(0x151c24, 0x110)
void TResourceDisplay::clear()
{
    m_backgroundWidget->setPlayerPaletteColors(g_curWatchPlayer);
    for (int i = 0; i < NUM_RESOURCES; ++i)
        m_resourceWidgets[i]->setText("");

    const TTextResource* labels = g_generalText;
    sprintf(g_text, "%s: %d, %s: %d, %s: %d",
        (*labels)[GENERAL_TEXT_CALENDAR_MONTH],
        static_cast<unsigned short>(g_game->m_month),
        (*labels)[GENERAL_TEXT_CALENDAR_WEEK],
        static_cast<unsigned short>(g_game->m_week),
        (*labels)[GENERAL_TEXT_CALENDAR_DAY],
        static_cast<unsigned short>(g_game->m_day));
    m_dayWidget->setText(g_text);
    draw(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}
