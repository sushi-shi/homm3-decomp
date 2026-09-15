// swapmgr.h - prototypes of swapmgr.cpp (compiland swapmgr.obj)
#ifndef HOMM3_SWAPMGR_H
#define HOMM3_SWAPMGR_H

#include "basemgr.h"
#include "hero.h"
#include "netmsg.h"
#include "remote.h"
#include "window.h"

class Bitmap816;
class BitmapBorder;
class Button;
class Message;

class CSwapManagerChatEdit : public CGameChatEdit {
public:
    CSwapManagerChatEdit(int x, int y, int w, int h, int textSize,
                         char* text, char* fontName, Font::Color color,
                         Font::Justify justification, char* backgroundIcon,
                         int backgroundFrame, int id, int style,
                         int readType, int insetX, int insetY);
    virtual void sendChat(const char* text, int toWho) OVERRIDE;
};

// Dreamcast proves the direct heroWindow base and contributes no additional
// virtuals. Retail's destructor walks the inherited Widgets vector verbatim.
// Before normalization (type): TSwapWindow.
class SwapWindow : public HeroWindow {
public:
    // Role-derived names: ctor 0x5aaa80 creates the transcript/edit controls,
    // trarrowl/trarrowr bitmaps, and kSwapReceiveFromAlly button. updateArrows
    // 0x5ae430 switches the arrows; the manager feeds chatText to CChatManager.
    TextWidget* m_chatText;  // +0x4c  chat transcript consumed by CChatManager
    CSwapManagerChatEdit* m_chatEdit;  // +0x50, rollover suppressed while focused
    BitmapBorder* m_leftArrow;  // +0x54  left-army count arrow
    BitmapBorder* m_rightArrow;  // +0x58  right-army count arrow
    // 0x5ae500 sets this for a network trade between distinct human owners;
    // sendHeroUpdate and canModHero use it for the two-player handshake.
    Button* m_receiveButton;        // +0x5c  transfer control
    int m_field60;       // +0x60  Complete-only tail (allocation extent proof)

    SwapWindow(Hero** heroes);
    virtual ~SwapWindow();
    void updateArrows();
};
SIZE(SwapWindow, 0x64);

// Canonical partial retail layout. IsLeftHero and its sole retail caller
// prove the two hero pointers at +0x40/+0x44; the swapManager ctor (0x5ae500)
// proves the rest of the ctor-touched prefix store-for-store.
// Before normalization (type): ESwapSelectSide.
enum SwapSelectSide {
    kSwapSelectLeft = 0,
    kSwapSelectRight = 1,
};

// Complete moved the four backpack controls two widget ids above the
// Dreamcast build and added a second full-refresh id.  The two network ids
// retain their protocol values across both revisions.
// Before normalization (type): ESwapWidgetId.
enum SwapWidgetId {
    kSwapLeftQuestLog = 0x55,
    kSwapRightQuestLog = 0x56,
    kSwapLeftBackpackLeft = 0x63,
    kSwapRightBackpackLeft = 0x64,
    kSwapLeftBackpackRight = 0x65,
    kSwapRightBackpackRight = 0x66,
    kSwapRefreshLeft = 0x67,
    kSwapRefreshRight = 0x68,
    kSwapReceiveFromAlly = 0x12e,
    kSwapTradeRequestDone = 0x7800,
};

// SetRolloverText's complete dispatch domain. The 215-byte retail selector
// table proves each band; ordinal names are retained where the corresponding
// TSwapWindow constructor body has not yet supplied a stronger widget role.
// Before normalization (type): ESwapRolloverWidgetId.
enum SwapRolloverWidgetId {
    kSwapRolloverHeroLeft = 1,
    kSwapRolloverHeroRight,
    kSwapRolloverLeftPrimary0,
    kSwapRolloverLeftPrimary1,
    kSwapRolloverLeftPrimary2,
    kSwapRolloverLeftPrimary3,
    kSwapRolloverRightPrimary0 = 8,
    kSwapRolloverRightPrimary1,
    kSwapRolloverRightPrimary2,
    kSwapRolloverRightPrimary3,
    kSwapRolloverLeftArmy0 = 13,
    kSwapRolloverLeftArmy1,
    kSwapRolloverLeftArmy2,
    kSwapRolloverLeftArmy3,
    kSwapRolloverLeftArmy4,
    kSwapRolloverLeftArmy5,
    kSwapRolloverLeftArmy6,
    kSwapRolloverRightArmy0,
    kSwapRolloverRightArmy1,
    kSwapRolloverRightArmy2,
    kSwapRolloverRightArmy3,
    kSwapRolloverRightArmy4,
    kSwapRolloverRightArmy5,
    kSwapRolloverRightArmy6,
    kSwapRolloverLeftArtifact0,
    kSwapRolloverLeftArtifact1,
    kSwapRolloverLeftArtifact2,
    kSwapRolloverLeftArtifact3,
    kSwapRolloverLeftArtifact4,
    kSwapRolloverLeftArtifact5,
    kSwapRolloverLeftArtifact6,
    kSwapRolloverLeftArtifact7,
    kSwapRolloverLeftArtifact8,
    kSwapRolloverLeftArtifact9,
    kSwapRolloverLeftArtifact10,
    kSwapRolloverLeftArtifact11,
    kSwapRolloverLeftArtifact12,
    kSwapRolloverLeftArtifact13,
    kSwapRolloverLeftArtifact14,
    kSwapRolloverLeftArtifact15,
    kSwapRolloverLeftArtifact16,
    kSwapRolloverLeftArtifact17,
    kSwapRolloverLeftArtifact18,
    kSwapRolloverRightArtifact0,
    kSwapRolloverRightArtifact1,
    kSwapRolloverRightArtifact2,
    kSwapRolloverRightArtifact3,
    kSwapRolloverRightArtifact4,
    kSwapRolloverRightArtifact5,
    kSwapRolloverRightArtifact6,
    kSwapRolloverRightArtifact7,
    kSwapRolloverRightArtifact8,
    kSwapRolloverRightArtifact9,
    kSwapRolloverRightArtifact10,
    kSwapRolloverRightArtifact11,
    kSwapRolloverRightArtifact12,
    kSwapRolloverRightArtifact13,
    kSwapRolloverRightArtifact14,
    kSwapRolloverRightArtifact15,
    kSwapRolloverRightArtifact16,
    kSwapRolloverRightArtifact17,
    kSwapRolloverRightArtifact18,
    kSwapRolloverLeftArmyCount0 = 65,
    kSwapRolloverLeftArmyCount1,
    kSwapRolloverLeftArmyCount2,
    kSwapRolloverLeftArmyCount3,
    kSwapRolloverLeftArmyCount4,
    kSwapRolloverLeftArmyCount5,
    kSwapRolloverLeftArmyCount6,
    kSwapRolloverRightArmyCount0,
    kSwapRolloverRightArmyCount1,
    kSwapRolloverRightArmyCount2,
    kSwapRolloverRightArmyCount3,
    kSwapRolloverRightArmyCount4,
    kSwapRolloverRightArmyCount5,
    kSwapRolloverRightArmyCount6,
    kSwapRolloverText0Left = 85,
    kSwapRolloverText0Right,
    kSwapRolloverLeftBackpack0 = 89,
    kSwapRolloverLeftBackpack1,
    kSwapRolloverLeftBackpack2,
    kSwapRolloverLeftBackpack3,
    kSwapRolloverLeftBackpack4,
    kSwapRolloverRightBackpack0,
    kSwapRolloverRightBackpack1,
    kSwapRolloverRightBackpack2,
    kSwapRolloverRightBackpack3,
    kSwapRolloverRightBackpack4,
    kSwapRolloverArmyMoveLeft = 103,
    kSwapRolloverArmyMoveRight,
    kSwapRolloverText27Left,
    kSwapRolloverText27Right,
    kSwapRolloverMoraleLeft,
    kSwapRolloverMoraleRight,
    kSwapRolloverLuckLeft,
    kSwapRolloverLuckRight,
    kSwapRolloverText9Left,
    kSwapRolloverText9Right,
    kSwapRolloverText22Left,
    kSwapRolloverText22Right,
    kSwapRolloverStat115,
    kSwapRolloverStat116,
    kSwapRolloverStat117,
    kSwapRolloverStat118,
    kSwapRolloverStat145 = 145,
    kSwapRolloverLeftSkill0 = 200,
    kSwapRolloverLeftSkill1,
    kSwapRolloverLeftSkill2,
    kSwapRolloverLeftSkill3,
    kSwapRolloverLeftSkill4,
    kSwapRolloverLeftSkill5,
    kSwapRolloverLeftSkill6,
    kSwapRolloverLeftSkill7,
    kSwapRolloverRightSkill0,
    kSwapRolloverRightSkill1,
    kSwapRolloverRightSkill2,
    kSwapRolloverRightSkill3,
    kSwapRolloverRightSkill4,
    kSwapRolloverRightSkill5,
    kSwapRolloverRightSkill6,
    kSwapRolloverRightSkill7,
    kSwapNoRefreshWidget = 300,
};

// Before normalization (type): ESwapRolloverCreatureDomain.
enum SwapRolloverCreatureDomain {
    kSwapRolloverCreatureLast = 0x96,
};

// Complete's campaign-only guard in handle_artifact_click. Retail fixes the
// scenario ordinals and the one exempt hero id directly.
// Before normalization (type): EArmageddonsBladeCampaignGuard.
enum ArmageddonsBladeCampaignGuard {
    ARMAGEDDONS_BLADE_CAMPAIGN = 7,
    ARMAGEDDONS_BLADE_MAP = 7,
    ARMAGEDDONS_BLADE_EXEMPT_HERO = 148
};

class CTradeRequestDoneMsg : public CNetMsg {
public:
    CTradeRequestDoneMsg();
};

class CGiveMeStuffMsg : public CNetMsg {
public:
    CGiveMeStuffMsg();
};

class CHeroUpdateMsg : public CNetMsg {
public:
    Hero m_leftHero;
    Hero m_rightHero;

    CHeroUpdateMsg(Hero* left, Hero* right);
};

SIZE(CTradeRequestDoneMsg, 0x14);
SIZE(CGiveMeStuffMsg, 0x14);
SIZE(CHeroUpdateMsg, 0x938);

// Before normalization (type): swapManager.
class SwapManager : public BaseManager {
public:
    SwapWindow* m_parent;     // +0x38
    Bitmap816* m_border;       // +0x3c
    Hero* m_heroes[2];         // +0x40 / +0x44
    // Two-stage army selection. swapMons 0x5b0da0 indexes the source and
    // destination heroes and their respective army slots with these four
    // words, then combines or swaps the stacks. Role-derived names.
    int m_sourceHeroIndex;            // +0x48  selection state (all -1 at construction)
    int m_destinationHeroIndex;            // +0x4c
    int m_sourceArmySlot;            // +0x50
    int m_destinationArmySlot;            // +0x54
    // Ctor 0x5ae530 and reset 0x5ae5c6 store -1 (awaiting source stack).
    // handleMonster tests it at 0x5af2fc, clears it at 0x5af3af when the
    // source is selected, and then accepts the destination at 0x5af40b.
    // drawSelector paints the selected stack only when this word is zero.
    int m_armySelectionPending;            // +0x58
    // 0x5ae500 sets this for a network trade between distinct human owners;
    // sendHeroUpdate and canModHero use it for the two-player handshake.
    unsigned char m_humanPlayerTrade;  // +0x5c  two-human cross-owner network trade
    // clears this, GiveMeStuff sets it. updateArrows points away from our
    // hero while set; canModHero prevents editing during the receiving phase.
    unsigned char m_givingToAlly;  // +0x5d  default 1; giver/receiver phase in a human trade
    // +0x5e, +0x5f pad
    // Open saves the installed handler before allocating CSwapMgrNetMsgHandler;
    // Close restores it and deletes the owned handler below.
    CNetMsgHandler* m_previousNetMsgHandler;  // +0x60
    CNetMsgHandler* m_netMsgHandler;

    SwapManager(Hero* leftHero, Hero* rightHero);
    void reset();
    virtual int open(int newPriority);  // baseManager vtable slot 0
    virtual void close();               // slot 1
    virtual int main(Message& msg);     // slot 2
    int drawSwapWin();
    inline bool isLeftHero();
    inline unsigned char isRightHero();
    inline Hero* getOtherHero();
    void drawSelector();
    void sendHeroUpdate();
    int exitSwapManager(Message& msg);
    void swapSide();
    void onChatUpdate();
    void updateArtifactWidget(long id, Artifact artifact);
    void updateSlot(int hero, ArtifactSlot slot);
    void updateAllSlots();
    void updateBackpackItem(int hero, int i);
    void updateBackpack(int hero);
    void handleMonster(int hero, int monster, int rightMouse,
                       unsigned char shift);
    void handleArtifactClick(long side, long id,
                               unsigned char rightClick);
    void handleBackpackClick(long side, long id,
                               unsigned char rightClick);
    void swapMons();
    void viewMon();
    void setRolloverText(int codeY);
    void update();
    void handleHeroUpdateMsg(CNetMsg* netMsg);
    void onWidgetDeselect(Message& msg, int& exitFlag);
    void onReceiveFromAlly();
    void onGiveMeStuffMsg();
    bool canModHero(int hero);
};

// --- CGiveMeStuffMsg ---
// CODEVIEW(E:\gamedcs\swapmgr.cpp:151, dc 0x15f084) void CGiveMeStuffMsg::CGiveMeStuffMsg();

// --- CHeroUpdateMsg ---
// CODEVIEW(E:\gamedcs\swapmgr.cpp:120, dc 0x15efd4) void CHeroUpdateMsg::CHeroUpdateMsg(hero* left, hero* right);

// --- CSwapManagerChatEdit ---
// CODEVIEW(E:\gamedcs\swapmgr.cpp:192, dc 0x15f0a4) void CSwapManagerChatEdit::CSwapManagerChatEdit(int textWidgetX, int textWidgetY, int textWidgetWidth, int textWidgetHeight, int textStringSize, char* textString, char* textFontName, int colorIndex, font::EJustify justification, char* backgroundIconName, int backgroundFrame, int textWidgetId, int textWidgetStyle, int iReadType, int textInsetX, int textInsetY);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:201, dc 0x15f164) void* CSwapManagerChatEdit::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:201, dc 0x15f198) void CSwapManagerChatEdit::~CSwapManagerChatEdit();

// --- CSwapMgrNetMsgHandler ---
// CODEVIEW(E:\gamedcs\swapmgr.cpp:512, dc 0x15f1e4) void CSwapMgrNetMsgHandler::CSwapMgrNetMsgHandler();

// --- CTradeRequestDoneMsg ---
// CODEVIEW(E:\gamedcs\swapmgr.cpp:130, dc 0x15f064) void CTradeRequestDoneMsg::CTradeRequestDoneMsg();

// --- TSwapWindow ---
// CODEVIEW(E:\gamedcs\swapmgr.cpp:210, dc 0x159328) void TSwapWindow::TSwapWindow(hero** heroes);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:454, dc 0x15f1b0) void* TSwapWindow::`scalar deleting destructor'(unsigned __flags);

// --- baseManager ---
// CODEVIEW(E:\gamedcs\basemgr.h:41, dc 0x15efd0) void baseManager::SetStatus(short newStatus);

// --- swapManager ---
// CODEVIEW(E:\gamedcs\swapmgr.cpp:655, dc 0x15c648) int swapManager::DrawSwapWin();
// CODEVIEW(E:\gamedcs\swapmgr.cpp:665, dc 0x15c66c) int swapManager::Open(int newPriority);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:816, dc 0x15cadc) void swapManager::DrawSelector();
// CODEVIEW(E:\gamedcs\swapmgr.cpp:914, dc 0x15cccc) void swapManager::update_artifact_widget(long id, TArtifact artifact);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:962, dc 0x15cdbc) void swapManager::update_all_slots();
// CODEVIEW(E:\gamedcs\swapmgr.cpp:977, dc 0x15ce20) void swapManager::UpdateBackpackItem(int iHero, int i);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:1106, dc 0x15d150) void swapManager::handle_artifact_click(long side, long id, unsigned char right_click);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:1168, dc 0x15d2e0) void swapManager::handle_backpack_click(long side, long id, unsigned char right_click);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:1222, dc 0x15d4ac) int swapManager::Main(message* msg);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:1660, dc 0x15de34) int swapManager::ExitSwapManager(message* msg);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:1667, dc 0x15de40) void swapManager::swap_side();
// CODEVIEW(E:\gamedcs\swapmgr.cpp:1793, dc 0x15e308) void swapManager::SetRolloverText(int codeY);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:2024, dc 0x15e85c) void swapManager::ViewMon();
// CODEVIEW(E:\gamedcs\swapmgr.cpp:2072, dc 0x15ea00) void swapManager::Update();
// CODEVIEW(E:\gamedcs\swapmgr.cpp:2140, dc 0x15eb90) void swapManager::OnChatUpdate();
// CODEVIEW(E:\gamedcs\swapmgr.cpp:2147, dc 0x15ebac) void swapManager::HandleHeroUpdateMsg(CNetMsg* pNetMsg);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:2165, dc 0x15ec58) void swapManager::OnWidgetDeselect(message* msg, int* exitFlag);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:2231, dc 0x15edf8) bool swapManager::IsLeftHero();
// CODEVIEW(E:\gamedcs\swapmgr.cpp:2241, dc 0x15ee24) unsigned char swapManager::IsRightHero();
// CODEVIEW(E:\gamedcs\swapmgr.cpp:2251, dc 0x15ee50) hero* swapManager::GetOtherHero();
// CODEVIEW(E:\gamedcs\swapmgr.cpp:2259, dc 0x15ee74) hero* swapManager::GetOurHero();
// CODEVIEW(E:\gamedcs\swapmgr.cpp:2267, dc 0x15ee98) bool swapManager::CanModHero(int hero);
// CODEVIEW(E:\gamedcs\swapmgr.cpp:2287, dc 0x15ef1c) void swapManager::OnReceiveFromAlly();
// CODEVIEW(E:\gamedcs\swapmgr.cpp:2298, dc 0x15ef60) void swapManager::OnGiveMeStuffMsg();

#endif  /* HOMM3_SWAPMGR_H */
