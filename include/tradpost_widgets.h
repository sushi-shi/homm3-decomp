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

    // The shared exit/command widget (the same 0x7802 combatwindow names
    // COMBAT_PLACEMENT_COMMAND_1_ID).
    MARKET_COMMAND_ID = 0x7802
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

// The sell-creature left column: one widget id per army slot (0x8b..0x91).
enum EMarketCreatureSlotId {
    MARKET_CREATURE_SLOT_0_ID = 0x8b,
    MARKET_CREATURE_SLOT_1_ID, MARKET_CREATURE_SLOT_2_ID,
    MARKET_CREATURE_SLOT_3_ID, MARKET_CREATURE_SLOT_4_ID,
    MARKET_CREATURE_SLOT_5_ID, MARKET_CREATURE_SLOT_6_ID   // 0x91
};

#endif  /* HOMM3_TRADPOST_WIDGETS_H */
