// remotedlg.h - the window/dialog classes retail's remote.cpp defines
// (compiland remote.obj).
// HAND-OWNED. Class layouts are NOT fabricated from method symbols;
// prototypes stay comments until a retail layout is proven.
//
// This is deliberately NOT in remote.h. The Dreamcast field list homes
// these classes in E:\gamedcs\remote.h, but seven other compiled TUs include
// our remote.h (advmgr, ai_player, cmbtmgr, command, levelupwindow, mainmenu,
// systemoptionswindow) and none of them needs a dialog or bitmap base; pulling
// dialogbox.h and bitmap16.h into their include closures is exactly the
// perturbation the include-set residual class warns about. remote.cpp is the
// only consumer.
#ifndef HOMM3_REMOTEDLG_H
#define HOMM3_REMOTEDLG_H

#include "bitmap16.h"
#include "dialogbox.h"
#include "remote.h"
#include "armygrp.h"
#include "hero.h"
#include "netmsg.h"
#include "town.h"

class CSprite;

// Retail-only wrappers in the video TU immediately after smackmgr. The first
// forwards its fastcall frame argument to SmackGoto on the current handle;
// the second decodes that handle's current frame when playback is active.
// Their wider ownership and original names remain unattested.
// Before normalization (function): SetCurrentSmackFrame.
void __fastcall setCurrentSmackFrame(int frame);
// Before normalization (function): DrawCurrentSmackFrame.
void drawCurrentSmackFrame();

// CNetMsgHandlerPause is defined in remote.h beside its base class.

// Layout is DC's field list shifted by the CTextDialog widening (DC's
// CTextDialog is 0x50, retail's is 0x58, so every DC offset moves +8), and
// every shifted offset is corroborated by retail bytes:
//   m_lastTick     DC 80  -> 0x58   zeroed by the constructor 0x554a50
//   m_spriteX      DC 84  -> 0x5c
//   m_spriteY      DC 88  -> 0x60
//   m_spriteFrame  DC 92  -> 0x64   zeroed by the constructor
//   m_seq          DC 96  -> 0x68   written by Setup 0x554b10 (4th argument)
//   m_sSprite      DC 100 -> 0x6c   written by Setup 0x554b10 (3rd argument)
//   m_palUpdated   DC 104 -> 0x70   zeroed (byte) by the constructor
//   m_pSprite      DC 108 -> 0x74   zeroed by the constructor; the destructor
//                                   0x554ab0 Disposes it through slot 1
// DC's total 112 becomes 0x78.
//
// The vtable 0x640e94 is 14 slots against CTextDialog's 13, and DC names the
// extra one exactly: Setup is INTRODUCING VIRTUAL at "vfptr offset = 52",
// i.e. slot 13 - which is where retail's 0x554b10 sits, and that body writes
// m_sSprite/m_seq and then chains to CTextDialog::Setup (0x490820, slot 10).
// The three remaining entries are overrides at their inherited slots: slot 3
// handle_message (0x554d90), slot 5 DrawWindow (0x554e90), slot 12
// CalcDimensions (0x554c30). Declared only, no local definition - the
// widget::Close idiom.
class CAnimatedDlg : public CTextDialog {
public:
    CAnimatedDlg();
    virtual ~CAnimatedDlg();
    // Before normalization (function): CAnimatedDlg::Setup.
    // Before normalization (locals): cText, pFont, sSprite.
    virtual unsigned char setup(const char* text, font* currentFont,
                                const char* spriteName, int seq);  // slot 13
    // Before normalization (function): CAnimatedDlg::CalcDimensions.
    // Before normalization (locals): cText, pFont.
    virtual void calcDimensions(const char* text, font* currentFont,
                                int& winX, int& winY,
                                int& winWidth, int& winHeight);  // slot 12
    // Before normalization (function): CAnimatedDlg::handle_message.
    virtual int handleMessage(message& msg);                    // slot 3
    // Before normalization (function): CAnimatedDlg::DrawWindow.
    // Before normalization (locals): iLowID, iHighID.
    virtual void drawWindow(unsigned char update, int lowID,
                            int highID);                        // slot 5

protected:
    // Before normalization (function): CAnimatedDlg::CalcSpriteDimensions.
    void calcSpriteDimensions(CSprite* sprite, int& maxWidth,
                              int& maxHeight, int& minY);
    // Before normalization (function): CAnimatedDlg::DrawSprite.
    void drawSprite();
    // Before normalization (function): CAnimatedDlg::TickAnimation.
    void tickAnimation();
    unsigned long m_lastTick;    // +0x58
    int m_spriteX;               // +0x5c
    int m_spriteY;               // +0x60
    int m_spriteFrame;           // +0x64
    int m_seq;                   // +0x68
    const char* m_spriteName;       // +0x6c
    unsigned char m_palUpdated;  // +0x70
    // Before normalization: m_pSprite.
    CSprite* m_sprite;          // +0x74
};
SIZE(CAnimatedDlg, 0x78);

// DC's CWaitForReadyPlayersDlg is 0x90 over its 0x70 CAnimatedDlg.  Retail
// widens only that base by eight bytes, giving this 0x98-byte translation:
// startTime/lastMsg move to +0x78/+0x7c, the already proven 0x10-byte pause
// handler occupies +0x80, and the eight player-ready bytes begin at +0x90.
// Retail's vtable 0x640ecc is CAnimatedDlg's fourteen slots with slot 0 and
// slot 3 replaced by this class's deleting destructor and message handler.
class CWaitForReadyPlayersDlg : public CAnimatedDlg {
public:
    CWaitForReadyPlayersDlg();
    // Before normalization (function): CWaitForReadyPlayersDlg::Wait.
    void wait();
    // Before normalization (function): CWaitForReadyPlayersDlg::AllPlayersReady.
    bool allPlayersReady();
    // Before normalization (function): CWaitForReadyPlayersDlg::handle_message.
    virtual int handleMessage(message& msg);  // slot 3

protected:
    // Before normalization (function): CWaitForReadyPlayersDlg::OnPlayerDrop.
    // Before normalization (locals): pNetMsg.
    int onPlayerDrop(CNetMsg* netMsg, message& msg);

    // Before normalization: startTime.
    unsigned long m_startTime;             // +0x78
    // Before normalization: lastMsg.
    unsigned long m_lastMsg;               // +0x7c
    CNetMsgHandlerPause m_netMsgHandler;  // +0x80
    // Before normalization: playerReady.
    unsigned char m_playerReady[8];         // +0x90
};
SIZE(CWaitForReadyPlayersDlg, 0x98);

// --- CAnimatedDlg ---
// CODEVIEW(E:\gamedcs\remote.cpp:1539, dc 0x11d1ec) void CAnimatedDlg::CAnimatedDlg();
// CODEVIEW(E:\gamedcs\remote.cpp:1547, dc 0x11d250) void CAnimatedDlg::~CAnimatedDlg();
// CODEVIEW(E:\gamedcs\remote.cpp:1553, dc 0x11d290) bool CAnimatedDlg::Setup(const char* cText, font* pFont, const char* sSprite, int seq);
// CODEVIEW(E:\gamedcs\remote.cpp:1563, dc 0x11d2b0) void CAnimatedDlg::CalcSpriteDimensions(CSprite* pSprite, int& a, int& b, int& c);
// CODEVIEW(E:\gamedcs\remote.cpp:1606, dc 0x11d394) void CAnimatedDlg::CalcDimensions(const char* cText, font* pFont, int& winX, int& winY, int& winWidth, int& winHeight);
// CODEVIEW(E:\gamedcs\remote.cpp:1634, dc 0x11d490) void CAnimatedDlg::DrawSprite();
// CODEVIEW(E:\gamedcs\remote.cpp:1651, dc 0x11d558) int CAnimatedDlg::handle_message(message& msg);
// CODEVIEW(E:\gamedcs\remote.cpp:1658, dc 0x11d56c) void CAnimatedDlg::TickAnimation();
// CODEVIEW(E:\gamedcs\remote.cpp:1673, dc 0x11d5dc) void CAnimatedDlg::DrawWindow(bool update, int iLowID, int iHighID);

// CLevelPickWaitDlg - the modal that waits on the other players' level-up
// picks. DC gives it as 136 bytes over a 112-byte CAnimatedDlg with three
// members, and the retail constructor 0x556ab0 writes all three at exactly
// the offsets the +8 CTextDialog widening predicts:
//   m_fromWho        DC 112 -> 0x78   set to -1 in the body
//   m_netMsgHandler  DC 116 -> 0x7c   the sixteen bytes 0x556af7..0x556b3d fill
//   m_playerDropped  DC 132 -> 0x8c   zeroed (byte) in the body
// DC's 136 becomes 0x90. Vtable 0x640f40 is fourteen slots, CAnimatedDlg's
// own width, so the class introduces no virtual: slot 3 is its
// handle_message override at 0x556c20 and every other entry is inherited,
// down to CAnimatedDlg's own CalcDimensions (0x554c30), Setup (0x554b10) and
// DrawWindow (0x554e90).
class CLevelPickWaitDlg : public CAnimatedDlg {
public:
    CLevelPickWaitDlg();
    // Before normalization (function): CLevelPickWaitDlg::WaitForLevels.
    void waitForLevels(int fromWho);
    // Before normalization (function): CLevelPickWaitDlg::handle_message.
    virtual int handleMessage(message& msg);  // slot 3

protected:
    // Before normalization (function): CLevelPickWaitDlg::OnPlayerDrop.
    // Before normalization (locals): pNetMsg.
    int onPlayerDrop(CNetMsg* netMsg, message& msg);
    // Before normalization (function): CLevelPickWaitDlg::OnHeroLevelUpdate.
    // Before normalization (locals): pNetMsg.
    void onHeroLevelUpdate(CNetMsg* netMsg);

    int m_fromWho;                        // +0x78
    CNetMsgHandlerPause m_netMsgHandler;  // +0x7c
    // Public: advManager::DoCombat re-runs the local CheckLevel when the
    // remote player dropped mid-pick. Access-only change.
public:
    unsigned char m_playerDropped;        // +0x8c
};
SIZE(CLevelPickWaitDlg, 0x90);

class TAbstractFile;

// Retail's complex wire-message base is a vptr followed by an ordinary
// 20-byte CNetMsg image. The subtype constructor at 0x512c50 writes exactly
// that layout, and 0x512e00 copies a received header into netmsg before
// dispatching the remaining payload through virtual read(). The ordinal name
// is retained because neither retail nor DC names that PC-only bridge.
class t_complex_net_message {
public:
    // The no-subtype form at 0x512c20 (stores the base vtable and
    // zeroes the netmsg image); singleselectionwindow's received-row
    // message constructs through it. ADDITIVE 2026-08-27 - one
    // declarator; re-measure the include-set-sensitive rows of the
    // five includers on merge.
    t_complex_net_message();
    // eRS_Messages, not int: the constructor reaches the message image
    // through CNetMsg's own two-argument constructor (the vptr store lands
    // AFTER the five member stores, which only a member-initialiser list
    // produces), and CNetMsg's first parameter is the DC-attested enum.
    t_complex_net_message(eRS_Messages subType);
    virtual unsigned char read(TAbstractFile* infile);
    virtual unsigned char write(TAbstractFile* outfile) const;
    // Before normalization (function): t_complex_net_message::RemoteFn_00512E00.
    // Before normalization (locals): pNetMsg.
    unsigned char remoteFn00512E00(CNetMsg* netMsg);
    // 0x512d40, the send half of the 0x512e00 bridge: serialize this
    // message and hand it to the transport (toWho / compress /
    // guaranteed mirror TransmitRemoteData's tail). Ordinal name for
    // the same reason as its receive twin. Not claimed from here.
    // The two flags are BOOL, not byte: retail pushes both parameter slots
    // straight through to the transport, which takes `_N` in its own
    // mangled name, and a byte parameter would have to be normalised with a
    // `test`/`setne` pair at each site first.
    // Before normalization (function): t_complex_net_message::RemoteFn_00512D40.
    unsigned char remoteFn00512D40(int toWho, bool compressMsg,
                                    bool guaranteed);
    // 0x512c80, the DPID-addressed send twin (its args mirror
    // TransmitRemoteDataDPID's tail); CNewPlayerUpdateProc's
    // HandleRequests hands each re-requested header row through it.
    // ADDITIVE 2026-08-27 (round 3) - one declarator; re-measure the
    // include-set-sensitive rows of the five includers on merge.
    // Before normalization (function): t_complex_net_message::RemoteFn_00512C80.
    unsigned char remoteFn00512C80(unsigned long dpid, bool compressMsg,
                                    bool guaranteed);

    // Before normalization: netmsg.
    CNetMsg m_netmsg;  // +0x04
};
SIZE(t_complex_net_message, 0x18);

// DC supplies all seventeen payload names and their order. Retail shifts the
// scalar prefix by four bytes for t_complex_net_message's vptr, retains both
// 0x38-byte army groups, aligns town to +0xb0, and widens each hero to 0x492.
// The last hero ends at +0xb3c; town's natural eight-byte alignment rounds the
// complete PC class to 0xb40, exactly the stack extent in DoNetCombat and the
// member extent in the wait-dialog constructor.
class CCombatInitMsg : public t_complex_net_message {
public:
    CCombatInitMsg();
    virtual unsigned char read(TAbstractFile* infile);
    virtual unsigned char write(TAbstractFile* outfile) const;

    type_point m_point;             // +0x018
    unsigned char m_leftHero;       // +0x01c
    unsigned char m_rightTown;      // +0x01d
    unsigned char m_rightHero;      // +0x01e
    int m_seed;                     // +0x020
    int m_winner;                   // +0x024
    unsigned char m_retreatWin;     // +0x028
    unsigned char m_combatSurrender;// +0x029
    int m_leftOwner;                // +0x02c
    int m_leftGold;                 // +0x030
    int m_rightOwner;               // +0x034
    int m_rightGold;                // +0x038
    armyGroup m_leftArmyGroup;      // +0x03c
    armyGroup m_rightArmyGroup;     // +0x074
    town m_town;                    // +0x0b0
    hero m_leftHeroData;            // +0x218
    hero m_rightHeroData;           // +0x6aa
};
SIZE(CCombatInitMsg, 0xb40);

// The remote-combat wait dialog shares CAnimatedDlg's 0x78-byte prefix.
// Dreamcast proves the method names and m_playerPos at the first derived
// dword; retail 0x557090 independently reads/writes it at +0x78. The PC
// constructor then builds a large, platform-specific combat-init payload at
// +0x80, a pause handler at +0xbc0 and a received byte at +0xbd0. The
// intervening +0x7c pointer is DC-attested; retail leaves it untouched in the
// constructor/Wait/handler trio. Every PC offset below is independently fixed
// by those three bodies.
class CWaitForRemoteBattleDlg : public CAnimatedDlg {
public:
    CWaitForRemoteBattleDlg();
    // COMPILER-GENERATED, corrected 2026-09-06 (claim lane 31). DC lists
    // a `~CWaitForRemoteBattleDlg` at E:\gamedcs\events.cpp:6709
    // (dc 0x9cf24), but the x86 COMDAT retail selected for it - 0x4aea00,
    // in events.obj's band - has no vtable store at entry and expands all
    // three member string teardowns, which is the synthesized shape and
    // not the user-defined one. Declaring it here scored that row 28.9125;
    // leaving it implicit scores 100.0000 (see the note in events.cpp).
    // The destructor stays virtual through CAnimatedDlg's.
    // Before normalization (function): CWaitForRemoteBattleDlg::Wait.
    void wait(int playerPos);
    // Before normalization (function): CWaitForRemoteBattleDlg::handle_message.
    virtual int handleMessage(message& msg);  // slot 3

protected:
    // Before normalization (function): CWaitForRemoteBattleDlg::OnPlayerDrop.
    // Before normalization (locals): pNetMsg.
    int onPlayerDrop(CNetMsg* netMsg, message& msg);

    int m_playerPos;                         // +0x78
    // Before normalization: m_pCombatInitMsg.
    CCombatInitMsg* m_combatInitMsgPointer;        // +0x7c (DC name)

    // Public tail: advManager::DoCombat reads the received flag and
    // hands the payload message straight to ReceiveHeroTownData.
    // Access-only change - no member moved, no declarator added.
public:
    CCombatInitMsg m_combatInitMsg;          // +0x80 (retail by-value copy)
protected:
    CNetMsgHandlerPause m_netMsgHandler;     // +0xbc0
public:
    unsigned char m_combatInitMsgReceived;   // +0xbd0
};
// CCombatInitMsg gives the containing dialog eight-byte alignment, so the
// received byte's +0xbd1 end rounds to 0xbd8. DoCombat's local begins at
// ebp-0xd58 and the next aligned local band begins at ebp-0x180, confirming
// that full stack extent independently.
SIZE(CWaitForRemoteBattleDlg, 0xbd8);

// CSaveScreen - the off-screen backing store the transfer dialog parks the
// framebuffer in. DC's field list sits on a 184-byte Bitmap16Bit; retail's
// Bitmap16Bit is 0x38, and the three trailing members land on exactly the
// three offsets the constructor 0x5572e0 writes after its base call:
//   screenSaved  DC 184 -> 0x38 (byte)
//   m_x          DC 188 -> 0x3c
//   m_y          DC 192 -> 0x40
// The vtable 0x640fb0 is three slots wide - Bitmap16Bit's own - so this class
// introduces no virtual of its own, and DC marks ~CSaveScreen (compgenx):
// there is no user-written destructor to declare, which is why 0x557340 is a
// bare five-byte tail jump into ??1Bitmap16Bit.
class CSaveScreen : public Bitmap16Bit {
public:
    CSaveScreen(int w, int h);
    // Before normalization (function): CSaveScreen::Save.
    void save(int x, int y);
    // Before normalization (function): CSaveScreen::Restore.
    void restore(unsigned char update);
    // Before normalization (function): CSaveScreen::IsSaved.
    unsigned char isSaved();

protected:
    // Before normalization: screenSaved.
    unsigned char m_screenSaved;  // +0x38
    int m_x;                    // +0x3c
    int m_y;                    // +0x40
};
SIZE(CSaveScreen, 0x44);

// CGameTransferSmack - the Smacker-backed progress animation the transfer
// dialog owns by value. DC's record is 20 bytes with no vftable, and every
// member lands on a store in the constructor 0x557410:
//   m_x           0   m_started     12 (byte)   m_saveScreen  16
//   m_y           4   m_sending     13 (byte)
//   m_lastFrame   8   m_drawText    14 (byte)
// The destructor closes the Smacker handle through the 0x198xxx video
// free function at 0x599050 and then deletes m_saveScreen through
// CSaveScreen's slot 0. Its guard-and-clear half is CGameTransferSmack::Stop
// byte for byte (0x5575e0 is those nine instructions and nothing else),
// which is what proves the destructor's first statement is a Stop() call
// rather than an open-coded repeat.
class CGameTransferSmack {
public:
    CGameTransferSmack();
    ~CGameTransferSmack();
    // Before normalization (function): CGameTransferSmack::Setup.
    void setup(int x, int y, unsigned char sending, unsigned char drawText);
    // Before normalization (function): CGameTransferSmack::Start.
    void start();
    // Before normalization (function): CGameTransferSmack::SetPercentage.
    void setPercentage(float pct);
    // Before normalization (function): CGameTransferSmack::DrawCurrentFrame.
    protected:
    void drawCurrentFrame() { drawCurrentSmackFrame(); }
    public:
    // Before normalization (function): CGameTransferSmack::Stop.
    void stop();
    // Before normalization (function): CGameTransferSmack::SaveScreen.
    void saveScreen();
    // Before normalization (function): CGameTransferSmack::RestoreScreen.
    void restoreScreen();

protected:
    int m_x;                     // +0x00
    int m_y;                     // +0x04
    int m_lastFrame;             // +0x08
    unsigned char m_started;     // +0x0c
    unsigned char m_sending;     // +0x0d
    unsigned char m_drawText;    // +0x0e
    CSaveScreen* m_saveScreen;   // +0x10
};
SIZE(CGameTransferSmack, 0x14);

// CGameTransferDlg - the save-game transfer progress dialog. DC's field list
// is a CTextDialog carrying `smack` at 80 and `m_sending` at 100 in a
// 104-byte record; retail's CTextDialog is 0x58, so smack occupies
// 0x58..0x6b - exactly the twenty bytes the constructor 0x557720 fills with
// CGameTransferSmack's inlined constructor - and m_sending lands on 0x6c,
// which is where that constructor stores its one byte argument.
//
// The vtable 0x640fbc is thirteen slots, CTextDialog's own width, so this
// class introduces no virtual; slot 12 holds its CalcDimensions override at
// 0x557790 and every other slot is inherited. DC marks ~CGameTransferDlg
// (compgenx), and retail's copy of it sits at 0x4cbcf0 - outside remote's
// band, i.e. a COMDAT the linker took from another object - so there is
// nothing here to declare or claim for it.
class CGameTransferDlg : public CTextDialog {
public:
    CGameTransferDlg(unsigned char sending);
    // Before normalization (function): CGameTransferDlg::CalcDimensions.
    // Before normalization (locals): cText, pFont.
    virtual void calcDimensions(const char* text, font* currentFont,
                                int& winX, int& winY,
                                int& winWidth, int& winHeight);  // slot 12
    // Public in DC field list 0x4e47; TransmitSaveGame selects this member
    // when the progress window, rather than the adventure view, owns it.
    // Before normalization: smack.
    CGameTransferSmack m_smack;    // +0x58

protected:
    unsigned char m_sending;     // +0x6c
};
SIZE(CGameTransferDlg, 0x70);

// --- CSaveScreen ---
// CODEVIEW(E:\gamedcs\remote.cpp:2673, dc 0x11eb40) void CSaveScreen::CSaveScreen(int w, int h);
// CODEVIEW(E:\gamedcs\remote.cpp:2680, dc 0x11eb9c) void CSaveScreen::Save(int x, int y);
// CODEVIEW(E:\gamedcs\remote.cpp:2689, dc 0x11ebc8) void CSaveScreen::Restore();
// CODEVIEW(E:\gamedcs\remote.cpp:2701, dc 0x11ec5c) bool CSaveScreen::IsSaved();

// --- CGameTransferSmack ---
// CODEVIEW(E:\gamedcs\remote.cpp:2713, dc 0x11ec64) void CGameTransferSmack::CGameTransferSmack();
// CODEVIEW(E:\gamedcs\remote.cpp:2724, dc 0x11ec88) void CGameTransferSmack::~CGameTransferSmack();
// CODEVIEW(E:\gamedcs\remote.cpp:2733, dc 0x11ecbc) void CGameTransferSmack::Setup(int x, int y, bool sending, int a);
// CODEVIEW(E:\gamedcs\remote.cpp:2741, dc 0x11ecd8) void CGameTransferSmack::Start();
// CODEVIEW(E:\gamedcs\remote.cpp:2747, dc 0x11ece4) void CGameTransferSmack::SetPercentage(float pct);
// CODEVIEW(E:\gamedcs\remote.cpp:2784, dc 0x11ede8) void CGameTransferSmack::DrawCurrentFrame();
// CODEVIEW(E:\gamedcs\remote.cpp:2789, dc 0x11edec) void CGameTransferSmack::Stop();
// CODEVIEW(E:\gamedcs\remote.cpp:2799, dc 0x11ee04) void CGameTransferSmack::SaveScreen();
// CODEVIEW(E:\gamedcs\remote.cpp:2807, dc 0x11ee3c) void CGameTransferSmack::RestoreScreen();

// --- CGameTransferDlg ---
// CODEVIEW(E:\gamedcs\remote.cpp:2816, dc 0x11ee54) void CGameTransferDlg::CGameTransferDlg(bool sending);
// CODEVIEW(E:\gamedcs\remote.cpp:2821, dc 0x11eed8) void CGameTransferDlg::CalcDimensions(const char* cText, font* pFont, int& winX, int& winY, int& winWidth, int& winHeight);

#endif  /* HOMM3_REMOTEDLG_H */
