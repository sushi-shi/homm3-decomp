// tradpost_widgets.h - marketplace-dialog widget ids.
// PRIVATE to tradpost.cpp (not part of tradpost.h's shared closure): the
// SetRolloverText handlers switch on these ids, so they must be named
// enumerators rather than magic case labels. Values are byte-proven by the
// SetRolloverText dispatch tables; the resource-button ranges map one-to-one
// onto gResourceNames (WOOD..GOLD), which fixes those names. The five fixed
// panel ids (and the shared command id) keep provisional names until a
// producer attests them.
#ifndef HOMM3_TRADPOST_WIDGETS_H
#define HOMM3_TRADPOST_WIDGETS_H

#include "netmsg.h"

// The give-resource execute path builds a gift network message on the stack
// (subtype 0x432, the CGiftMsg immediate ai_player.h attests) and hands it to
// TransmitRemoteData. CNetMsg carries no vtable, so this tradpost-private
// mirror of that record inlines byte-for-byte with retail's construction; the
// shared spelling lives in ai_player.h (out of this lane's file scope).
class TGiveNetMsg : public CNetMsg {
public:
    int m_giver;
    int m_resource;
    int m_qty;

    TGiveNetMsg(int giver, int resource, int qty)
        : CNetMsg(0x432, sizeof(TGiveNetMsg)),
          m_giver(giver), m_resource(resource), m_qty(qty) {}
};

// The free-function overload the give path calls (remote.h:438); declared here
// to avoid pulling remote.h's DirectPlay closure into this TU.
int TransmitRemoteData(CNetMsg* pMsg, int toWho, bool compressMsg,
                       bool guaranteed);

// The gMarketWindow selector DoMarket dispatches on: the five dialog panes in
// the order the classes are declared. Byte-proven by DoMarket's jump table and
// the per-case `new <size>` immediates (0x68/0x8c/0x64/0x64/0x68).
enum EMarketWindow {
    MARKET_WINDOW_TRADE = 0,
    MARKET_WINDOW_GIVE = 1,
    MARKET_WINDOW_BUY = 2,
    MARKET_WINDOW_SELL_ARTIFACT = 3,
    MARKET_WINDOW_SELL_CREATURE = 4
};

// The gMarketSource pricing/mode selector the entry points seed (marketplace,
// trading post, black market, freelancer's guild). The Update methods branch
// label visibility and the title text on it.
enum EMarketSource {
    MARKET_SOURCE_MARKETPLACE = 0,
    MARKET_SOURCE_TRADING_POST = 1,
    MARKET_SOURCE_BLACK_MARKET = 2,
    MARKET_SOURCE_FREELANCER = 3
};

// The seven resource names, indexed by resource id. Shared table; seerhut.cpp
// carries the retail address claim (0x6a5e64). Declared here (consumer-side
// plain extern, the ai_player.h / advmgr.h pattern) for the resource-trade
// widgets, whose sell/buy id ranges both fold onto this array.
extern const char* gResourceNames[7];

enum EMarketWidgetId {
    MARKET_LEFT_PANEL_ID = 5,
    MARKET_RIGHT_PANEL_ID = 7,
    MARKET_LEFT_COUNT_ID = 16,
    MARKET_LEFT_LABEL_ID = 17,
    MARKET_RIGHT_LABEL_ID = 18,
    MARKET_TITLE_ID = 20,

    // Sell-side resource buttons, one per resource (WOOD..GOLD).
    MARKET_SELL_WOOD_ID = 0x1c,
    MARKET_SELL_MERCURY_ID = 0x1d,
    MARKET_SELL_ORE_ID = 0x1e,
    MARKET_SELL_SULFUR_ID = 0x1f,
    MARKET_SELL_CRYSTAL_ID = 0x20,
    MARKET_SELL_GEMS_ID = 0x21,
    MARKET_SELL_GOLD_ID = 0x22,

    // Buy-side resource buttons, same resource order.
    MARKET_BUY_WOOD_ID = 0x3f,
    MARKET_BUY_MERCURY_ID = 0x40,
    MARKET_BUY_ORE_ID = 0x41,
    MARKET_BUY_SULFUR_ID = 0x42,
    MARKET_BUY_CRYSTAL_ID = 0x43,
    MARKET_BUY_GEMS_ID = 0x44,
    MARKET_BUY_GOLD_ID = 0x45,

    // The buy-artifact window's right-column label (its SetRolloverText reads
    // gBuyArtHelpText[3] for this id). Provisional name.
    MARKET_BUY_RIGHT_LABEL_ID = 0x13,

    // The shared exit/command widget (the same 0x7802 combatwindow names
    // COMBAT_PLACEMENT_COMMAND_1_ID).
    MARKET_COMMAND_ID = 0x7802
};

// The three command subtypes shared by all five market-window handlers.
enum EMarketWidgetCommand {
    MARKET_WIDGET_SELECT = 12,
    MARKET_WIDGET_ACTIVATE = 13,
    MARKET_WIDGET_QUICK_VIEW = 14
};

// The buy-artifact left column: one widget id per artifact-for-sale slot
// (0x3f..0x45). The same numeric range other windows use for the buy-side
// resource buttons here selects an artifact out of gpMarketArtifacts, so it
// needs its own named cases.
enum EMarketBuyArtifactSlotId {
    BUY_ARTIFACT_SLOT_0_ID = 0x3f,
    BUY_ARTIFACT_SLOT_1_ID, BUY_ARTIFACT_SLOT_2_ID,
    BUY_ARTIFACT_SLOT_3_ID, BUY_ARTIFACT_SLOT_4_ID,
    BUY_ARTIFACT_SLOT_5_ID, BUY_ARTIFACT_SLOT_6_ID   // 0x45
};

// The sell-artifact left column: one widget id per selectable artifact slot,
// starting at 0x6b (update_sell_artifact_widget stamps id = slot + 0x6b).
// Equipped slots 0..17 then the visible backpack rows; the SetRolloverText
// switch needs each as a named case rather than a magic label.
enum EMarketArtifactSlotId {
    MARKET_ARTIFACT_SLOT_00_ID = 0x6b,
    MARKET_ARTIFACT_SLOT_01_ID, MARKET_ARTIFACT_SLOT_02_ID,
    MARKET_ARTIFACT_SLOT_03_ID, MARKET_ARTIFACT_SLOT_04_ID,
    MARKET_ARTIFACT_SLOT_05_ID, MARKET_ARTIFACT_SLOT_06_ID,
    MARKET_ARTIFACT_SLOT_07_ID, MARKET_ARTIFACT_SLOT_08_ID,
    MARKET_ARTIFACT_SLOT_09_ID, MARKET_ARTIFACT_SLOT_10_ID,
    MARKET_ARTIFACT_SLOT_11_ID, MARKET_ARTIFACT_SLOT_12_ID,
    MARKET_ARTIFACT_SLOT_13_ID, MARKET_ARTIFACT_SLOT_14_ID,
    MARKET_ARTIFACT_SLOT_15_ID, MARKET_ARTIFACT_SLOT_16_ID,
    MARKET_ARTIFACT_SLOT_17_ID, MARKET_ARTIFACT_SLOT_18_ID,
    MARKET_ARTIFACT_SLOT_19_ID, MARKET_ARTIFACT_SLOT_20_ID,
    MARKET_ARTIFACT_SLOT_21_ID, MARKET_ARTIFACT_SLOT_22_ID   // 0x81
};

// The two backpack scroll arrows on the sell-artifact panel; its WindowHandler
// pages gBackpackStart on these and re-blits the five visible backpack-icon
// widgets (ids 0x66..0x6a). Provisional names.
enum EMarketArtifactArrowId {
    MARKET_ARTIFACT_LEFT_ARROW_ID = 0x82,
    MARKET_ARTIFACT_RIGHT_ARROW_ID = 0x83
};

// The sell-creature left column: one widget id per army slot (0x8b..0x91).
enum EMarketCreatureSlotId {
    MARKET_CREATURE_SLOT_0_ID = 0x8b,
    MARKET_CREATURE_SLOT_1_ID, MARKET_CREATURE_SLOT_2_ID,
    MARKET_CREATURE_SLOT_3_ID, MARKET_CREATURE_SLOT_4_ID,
    MARKET_CREATURE_SLOT_5_ID, MARKET_CREATURE_SLOT_6_ID   // 0x91
};

// The give-resource recipient column: one widget id per selectable player
// slot (0x46..0x4c). TGiveResourceWindow::SetRolloverText formats each with
// the player-colour name of the slot's stored colour id, so the seven ids
// need named cases rather than a magic-label range. Provisional names.
enum EGiveRecipientId {
    GIVE_RECIPIENT_SLOT_0_ID = 0x46,
    GIVE_RECIPIENT_SLOT_1_ID, GIVE_RECIPIENT_SLOT_2_ID,
    GIVE_RECIPIENT_SLOT_3_ID, GIVE_RECIPIENT_SLOT_4_ID,
    GIVE_RECIPIENT_SLOT_5_ID, GIVE_RECIPIENT_SLOT_6_ID   // 0x4c
};

#endif  /* HOMM3_TRADPOST_WIDGETS_H */
