// tradpost.h - prototypes of tradpost.cpp (compiland tradpost.obj)
#ifndef HOMM3_TRADPOST_H
#define HOMM3_TRADPOST_H

#include "town.h"
#include "artifact.h"
#include "advmgr_popup.h"

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

// --- the five marketplace dialogs -------------------------------------
// Retail lays the compiland out in source order as five (constructor,
// scalar-deleting-destructor, destructor) triples, and each constructor
// names its own background PCX, which is what fixes the identities:

//   TPMRKRES.PCX  ctor 0x5df5f0  ??_G 0x5e1620  ~dtor 0x5e1650  vtbl 0x6439f8
//   TPMRKPTS.PCX  ctor 0x5e16c0  ??_G 0x5e3690  ~dtor 0x5e36c0  vtbl 0x643a34
//   TPMRKABS.PCX  ctor 0x5e3730  ??_G 0x5e5690  ~dtor 0x5e56c0  vtbl 0x643a70
//   TPMRKASS.PCX  ctor 0x5e5730  ??_G 0x5e7be0  ~dtor 0x5e7c10  vtbl 0x643aac
//   TPMRKCRS.PCX  ctor 0x5e7c80  ??_G 0x5e9c80  ~dtor 0x5e9cb0  vtbl 0x643ae8

// The order is corroborated twice over. Only constructors 1, 2 and 5 build
// a `slider`, which is exactly the set the Dreamcast roster gives slider
// callbacks for (TradeResourceSlider, GiveResourceSlider,
// SellCreatureSlider); and only constructor 4 touches vector<int>, the
// backpack list only TSellArtifactWindow has (UpdateMarketBackpack /
// increment_backpack_start / decrement_backpack_start). Each vtable is
// referenced exactly twice image-wide - by its own constructor and its own
// destructor - so none of these rows is an /OPT:ICF fold.

// All five constructors are reconstructed byte-exact (2026-08-27): each is
// CAdvPopup(x2, y2, 601, 593, 0x12), Widgets.reserve(144) (Give: 151), a
// fixed widget list, the slider member store where one exists, and the
// AddWidget/MemError sweep. The slider callbacks live in the three
// after-the-ctor gaps (0x5e1600/0x5e3670/0x5e9c60).
// The three dialogs that build a `slider` (TradeResourceSlider,
// GiveResourceSlider, SellCreatureSlider) hold its widget pointer as a member;
// forward-declared here for the pointer, defined where the ctor `new`s it.
class slider;

// Object sizes are byte-proven by DoMarket's `new` immediates
// (0x68/0x8c/0x64/0x64/0x68) and each ctor's `push <size>`. Members past the
// CAdvPopup base (0x60) are named where a reconstructed body attests the store
// and left as field_NN placeholders where only the size is proven so far.
// Before normalization (type): TTradeResourceWindow.
class TradeResourceWindow : public CAdvPopup {
    slider* m_resourceSlider;   // +0x60, set by the ctor (TradeResourceSlider)
    int m_lastHoverId;          // +0x64, last widget the hover handler rolled over
    // Dreamcast tradpost.cpp:2181, original ComputeTradeRatios (private).
    void computeTradeRatios(int inLeftResource, int inRightResource,
                            int* inTradeRatio, int* inLeftDenominated,
                            int* inMaxUnitsToTrade);

public:
    TradeResourceWindow(int x2, int y2);
    void update(unsigned char update);
    void setRolloverText(int codeY);
    virtual int windowHandler(message& msg);   // slot 9
    virtual ~TradeResourceWindow();
};
SIZE(TradeResourceWindow, 0x68);

// Before normalization (type): TGiveResourceWindow.
class GiveResourceWindow : public CAdvPopup {
public:
    // +0x60. The 0x46..0x4c recipient buttons index gPlayerColorNames by this
    // per-slot player-colour array; SetRolloverText's byte-proven
    // `[this + 4*id - 0xb4]` read fixes the array at +0x64, hence the leading
    // dword. DoMarket fills field_60 (recipient count) and slotPlayerColor;
    // the ctor stores resourceSlider at +0x84 and WindowHandler's hover uses
    // lastHoverId at +0x88. field_80 is proven only by the 0x8c object size.
    // DoMarket counts eligible other players into slotPlayerColor;
    // the handler bounds recipient-button selection with this count.
    int m_recipientCount;
    int m_slotPlayerColor[7];   // +0x64
    int m_field80;             // +0x80
    slider* m_resourceSlider;   // +0x84, set by the ctor (GiveResourceSlider)
    int m_lastHoverId;          // +0x88, last widget the hover handler rolled over

    GiveResourceWindow(int x2, int y2);
    void update(bool update);
    void setRolloverText(int codeY);
    virtual int windowHandler(message& msg);   // slot 9
    virtual ~GiveResourceWindow();
};
SIZE(GiveResourceWindow, 0x8c);

// Before normalization (type): TBuyArtifactWindow.
class BuyArtifactWindow : public CAdvPopup {
    int m_lastHoverId;          // +0x60, last widget the hover handler rolled over

public:
    BuyArtifactWindow(int x2, int y2);
    void update(unsigned char update);
    void setRolloverText(int codeY);
    virtual int windowHandler(message& msg);   // slot 9
    virtual ~BuyArtifactWindow();
};
SIZE(BuyArtifactWindow, 0x64);

// Before normalization (type): TSellArtifactWindow.
class SellArtifactWindow : public CAdvPopup {
    int m_lastHoverId;          // +0x60, last widget the hover handler rolled over
    void setupNewTrade();
    void updateMarketBackpack();
    void incrementBackpackStart();
    void decrementBackpackStart();

public:
    SellArtifactWindow(int x2, int y2);
    void updateSellArtifactWidget(message* msg, long i);
    void setWidgetOn(short id);
    void setWidgetOff(short id);
    void setWidgetDisabled(short id);
    void update(unsigned char update);
    void computeTradeRatios(int inLeftResource, int inRightResource,
                            int* inTradeRatio, int* inLeftDenominated,
                            int* inMaxUnitsToTrade);
    void setRolloverText(int codeY);
    virtual int windowHandler(message& msg);   // slot 9
    virtual ~SellArtifactWindow();
};
SIZE(SellArtifactWindow, 0x64);

// Before normalization (type): TSellCreatureWindow.
class SellCreatureWindow : public CAdvPopup {
    slider* m_creatureSlider;   // +0x60, set by the ctor (SellCreatureSlider)
    int m_lastHoverId;          // +0x64, last widget the hover handler rolled over

public:
    SellCreatureWindow(int x2, int y2);
    void setWidgetOn(short id);
    void setWidgetOff(short id);
    void setWidgetDisabled(short id);
    void update(bool update);
    void computeTradeRatios(int inLeftResource, int inRightResource,
                            int* inTradeRatio, int* inLeftDenominated,
                            int* inMaxUnitsToTrade);
    void setRolloverText(int codeY);
    virtual int windowHandler(message& msg);   // slot 9
    virtual ~SellCreatureWindow();
};
SIZE(SellCreatureWindow, 0x68);

long getMarketValue(EGameResource resource);

// The shared body all six market entry points tail into once they have
// seeded the static market state. The Dreamcast roster has it `static`;
// retail keeps it out of line because six call sites reach it, so the
// linkage difference cannot change its callers' code.
void doMarket();
void doTradingPost();
void doMarketplace();
void doArtifactMerchants();
void doFreelancersGuild(Hero* inHero);
void doFreelancersGuild(Town* currentTown);
void doBlackMarket(Hero* inHero, Artifact* blackArtifacts);

// Retail .data 0x678344. The public retail name carries this spelling;
// calculate_demand indexes entries 1..10 after clamping the number of
// owned legal Marketplaces. tradpost.cpp owns the admitted definition.
extern float g_tradingPostEfficency[];

// The two eleven-float efficiency rows immediately after fTradingPostEfficency
// (0x678370, 0x67839c), byte-verified from the retail image. The artifact-sale
// window divides an artifact's gold cost by the first; the creature-sale window
// divides a creature's gold cost by the second - both indexed by the owned
// Marketplace count. fArtifactPurchaseEfficency carries philai.h's spelling
// (get_artifact_purchase_price shares it); the creature row's name is
// provisional. tradpost.cpp owns both admitted definitions.
extern float g_artifactPurchaseEfficency[];
extern float g_creatureSaleEfficency[];

// --- globals ---
// CODEVIEW(E:\gamedcs\tradpost.cpp:618, dc 0x1883f0) void CountMarkets();
// CODEVIEW(E:\gamedcs\tradpost.cpp:704, dc 0x188708) void DoMarket();
// CODEVIEW(E:\gamedcs\tradpost.cpp:2160, dc 0x18ab8c) long get_market_value(EGameResource resource);

// --- TBuyArtifactWindow ---
// CODEVIEW(E:\gamedcs\tradpost.cpp:941, dc 0x188dd8) void TBuyArtifactWindow::SetWidgetOn(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:947, dc 0x188e10) void TBuyArtifactWindow::SetWidgetOff(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:954, dc 0x188e78) void TBuyArtifactWindow::SetWidgetDisabled(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:1466, dc 0x189aac) void TBuyArtifactWindow::Update(unsigned char bUpdate);
// CODEVIEW(E:\gamedcs\tradpost.cpp:2209, dc 0x18aef4) void TBuyArtifactWindow::ComputeTradeRatios(int inLeftResource, int inRightResource, int* iInTradeRatio, int* bInLeftDenominated, int* iInMaxUnitsToTrade);
// CODEVIEW(E:\gamedcs\tradpost.cpp:2302, dc 0x18b368) void TBuyArtifactWindow::SetupNewTrade();
// CODEVIEW(E:\gamedcs\tradpost.cpp:380, dc 0x18c94c) void* TBuyArtifactWindow::`scalar deleting destructor'(unsigned __flags);

// --- TGiveResourceWindow ---
// CODEVIEW(E:\gamedcs\tradpost.cpp:923, dc 0x188d50) void TGiveResourceWindow::SetWidgetOn(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:929, dc 0x188d88) void TGiveResourceWindow::SetWidgetOff(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:936, dc 0x188dbc) void TGiveResourceWindow::SetWidgetDisabled(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:2202, dc 0x18ae88) void TGiveResourceWindow::ComputeTradeRatios(int inLeftResource, int inRightResource, int* iInTradeRatio, int* bInLeftDenominated, int* iInMaxUnitsToTrade);
// CODEVIEW(E:\gamedcs\tradpost.cpp:2294, dc 0x18b328) void TGiveResourceWindow::SetupNewTrade();
// CODEVIEW(E:\gamedcs\tradpost.cpp:286, dc 0x18c918) void* TGiveResourceWindow::`scalar deleting destructor'(unsigned __flags);

// --- TSellArtifactWindow ---
// CODEVIEW(E:\gamedcs\tradpost.cpp:959, dc 0x188e94) void TSellArtifactWindow::SetWidgetOn(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:965, dc 0x188ecc) void TSellArtifactWindow::SetWidgetOff(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:972, dc 0x188f00) void TSellArtifactWindow::SetWidgetDisabled(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:2312, dc 0x18b3a8) void TSellArtifactWindow::SetupNewTrade();
// CODEVIEW(E:\gamedcs\tradpost.cpp:2327, dc 0x18b414) void TSellArtifactWindow::UpdateMarketBackpack();
// CODEVIEW(E:\gamedcs\tradpost.cpp:2344, dc 0x18b48c) void TSellArtifactWindow::increment_backpack_start();
// CODEVIEW(E:\gamedcs\tradpost.cpp:2356, dc 0x18b4c8) void TSellArtifactWindow::decrement_backpack_start();
// CODEVIEW(E:\gamedcs\tradpost.cpp:2932, dc 0x18c00c) int TSellArtifactWindow::WindowHandler(message* msg);
// CODEVIEW(E:\gamedcs\tradpost.cpp:509, dc 0x18c980) void* TSellArtifactWindow::`scalar deleting destructor'(unsigned __flags);

// --- TSellCreatureWindow ---
// CODEVIEW(E:\gamedcs\tradpost.cpp:977, dc 0x188f1c) void TSellCreatureWindow::SetWidgetOn(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:983, dc 0x188f54) void TSellCreatureWindow::SetWidgetOff(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:990, dc 0x188f88) void TSellCreatureWindow::SetWidgetDisabled(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:1920, dc 0x18a550) void TSellCreatureWindow::Update(unsigned char bUpdate);
// CODEVIEW(E:\gamedcs\tradpost.cpp:2250, dc 0x18b114) void TSellCreatureWindow::ComputeTradeRatios(int inLeftResource, int inRightResource, int* iInTradeRatio, int* bInLeftDenominated, int* iInMaxUnitsToTrade);
// CODEVIEW(E:\gamedcs\tradpost.cpp:2319, dc 0x18b3d4) void TSellCreatureWindow::SetupNewTrade();
// CODEVIEW(E:\gamedcs\tradpost.cpp:606, dc 0x18c9b4) void* TSellCreatureWindow::`scalar deleting destructor'(unsigned __flags);

// --- TTradeResourceWindow ---
// CODEVIEW(E:\gamedcs\tradpost.cpp:905, dc 0x188cc8) void TTradeResourceWindow::SetWidgetOn(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:911, dc 0x188d00) void TTradeResourceWindow::SetWidgetOff(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:918, dc 0x188d34) void TTradeResourceWindow::SetWidgetDisabled(short id);
// CODEVIEW(E:\gamedcs\tradpost.cpp:995, dc 0x188fa4) void TTradeResourceWindow::Update(unsigned char bUpdate);
// CODEVIEW(E:\gamedcs\tradpost.cpp:2181, dc 0x18ad48) void TTradeResourceWindow::ComputeTradeRatios(int inLeftResource, int inRightResource, int* iInTradeRatio, int* bInLeftDenominated, int* iInMaxUnitsToTrade);
// CODEVIEW(E:\gamedcs\tradpost.cpp:2286, dc 0x18b2e8) void TTradeResourceWindow::SetupNewTrade();
// CODEVIEW(E:\gamedcs\tradpost.cpp:183, dc 0x18c8e4) void* TTradeResourceWindow::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_TRADPOST_H */
