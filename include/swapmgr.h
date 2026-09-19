// swapmgr.h - swapmgr.cpp (compiland swapmgr.obj)
#ifndef HOMM3_SWAPMGR_H
#define HOMM3_SWAPMGR_H

#include "basemgr.h"
#include "hero.h"
#include "netmsg.h"
#include "remote.h"
#include "window.h"

class Bitmap816;
class bitmapBorder;
class button;
class message;

class CSwapManagerChatEdit : public CGameChatEdit {
public:
    CSwapManagerChatEdit(int x, int y, int w, int h, int textSize,
                         char* text, char* fontName, font::TColor color,
                         font::EJustify justification, char* backgroundIcon,
                         int backgroundFrame, int id, int style,
                         int readType, int insetX, int insetY);
    virtual void sendChat(const char* text, int toWho) OVERRIDE;
};

// Dreamcast proves the direct heroWindow base and contributes no additional
// virtuals. Retail's destructor walks the inherited Widgets vector verbatim.
class TSwapWindow : public heroWindow {
public:
    // Role-derived names: ctor 0x5aaa80 creates the transcript/edit controls,
    // trarrowl/trarrowr bitmaps, and kSwapReceiveFromAlly button. updateArrows
    // 0x5ae430 switches the arrows; the manager feeds chatText to CChatManager.
    textWidget* m_chatText;  // +0x4c  chat transcript consumed by CChatManager
    CSwapManagerChatEdit* m_chatEdit;  // +0x50, rollover suppressed while focused
    bitmapBorder* m_leftArrow;  // +0x54  left-army count arrow
    bitmapBorder* m_rightArrow;  // +0x58  right-army count arrow
    // 0x5ae500 sets this for a network trade between distinct human owners;
    // sendHeroUpdate and canModHero use it for the two-player handshake.
    button* m_receiveButton;        // +0x5c  transfer control
    int m_field60;       // +0x60  Complete-only tail (allocation extent proof)

    TSwapWindow(hero** heroes);
    virtual ~TSwapWindow();
    void updateArrows();
};
SIZE(TSwapWindow, 0x64);

// Canonical partial retail layout. IsLeftHero and its sole retail caller
// prove the two hero pointers at +0x40/+0x44; the swapManager ctor (0x5ae500)
// proves the rest of the ctor-touched prefix store-for-store.
enum ESwapSelectSide {
    kSwapSelectLeft = 0,
    kSwapSelectRight = 1,
};

// Complete moved the four backpack controls two widget ids above the
// Dreamcast build and added a second full-refresh id.  The two network ids
// retain their protocol values across both revisions.
enum ESwapWidgetId {
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
enum ESwapRolloverWidgetId {
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

enum ESwapRolloverCreatureDomain {
    kSwapRolloverCreatureLast = 0x96,
};

// Complete's campaign-only guard in handle_artifact_click. Retail fixes the
// scenario ordinals and the one exempt hero id directly.
enum EArmageddonsBladeCampaignGuard {
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
    hero m_leftHero;
    hero m_rightHero;

    CHeroUpdateMsg(hero* left, hero* right);
};

SIZE(CTradeRequestDoneMsg, 0x14);
SIZE(CGiveMeStuffMsg, 0x14);
SIZE(CHeroUpdateMsg, 0x938);

class swapManager : public baseManager {
public:
    TSwapWindow* m_parent;     // +0x38
    Bitmap816* m_border;       // +0x3c
    hero* m_heroes[2];         // +0x40 / +0x44
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

    swapManager(hero* leftHero, hero* rightHero);
    void reset();
    virtual int open(int newPriority);  // baseManager vtable slot 0
    virtual void close();               // slot 1
    virtual int main(message& msg);     // slot 2
    int drawSwapWin();
    inline bool isLeftHero();
    inline unsigned char isRightHero();
    inline hero* getOtherHero();
    void drawSelector();
    void sendHeroUpdate();
    int exitSwapManager(message& msg);
    void swapSide();
    void onChatUpdate();
    void updateArtifactWidget(long id, TArtifact artifact);
    void updateSlot(int hero, TArtifactSlot slot);
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
    void onWidgetDeselect(message& msg, int& exitFlag);
    void onReceiveFromAlly();
    void onGiveMeStuffMsg();
    bool canModHero(int hero);
};

#endif  /* HOMM3_SWAPMGR_H */
