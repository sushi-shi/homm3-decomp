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
void __fastcall SetCurrentSmackFrame(int frame);
void DrawCurrentSmackFrame();

// CNetMsgHandlerPause - the scoped handler that parks whatever handler the
// network singleton is carrying, installs itself for the life of a modal
// dialog, and puts the old one back. Sixteen bytes: CNetMsgHandler's twelve
// plus one pointer, and 0x557e30's `mov [esi+0xc], eax` is that pointer.
//
// Vtable 0x640f04 is four slots wide, CNetMsgHandler's own, so the class
// introduces nothing: slot 0 is the ??_G at 0x557eb0, slot 1 the
// CheckHandleNet override at 0x555170, slot 2 the INHERITED
// CNetMsgHandler::GetAbortPopupMsg at 0x557900 (already claimed), and slot 3
// the HandleNetMsg override at 0x555180. Both overrides are five bytes of
// `xor eax,eax` and a sized return - the pause semantics are to swallow
// everything - and both are header-origin COMDATs in retail too, which is
// why they sit at 0x555170/0x555180 beside CNetMsgHandler::Copy rather than
// in the 0x557exx run with the rest of the class.
//
// This class does NOT go in remote.h. Seven other compiled TUs include that
// header and none of them needs it; a new user-defined type in their include
// closures is exactly the perturbation the include-set residual class warns
// about.
class CNetMsgHandlerPause : public CNetMsgHandler {
public:
    CNetMsgHandlerPause();
    virtual ~CNetMsgHandlerPause();
    virtual CNetMsg* CheckHandleNet(unsigned char inPopup,
                                    unsigned char* msgReceived);  // slot 1
    virtual CNetMsg* HandleNetMsg(CNetMsg* pNetMsg);              // slot 3

protected:
    CNetMsgHandler* m_pNetMsgHandlerSave;  // +0x0c
};
SIZE(CNetMsgHandlerPause, 0x10);

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
    virtual unsigned char Setup(const char* cText, font* pFont,
                                const char* sSprite, int seq);  // slot 13
    virtual void CalcDimensions(const char* cText, font* pFont,
                                int& winX, int& winY,
                                int& winWidth, int& winHeight);  // slot 12
    virtual int handle_message(message& msg);                    // slot 3
    virtual void DrawWindow(unsigned char update, int iLowID,
                            int iHighID);                        // slot 5

protected:
    void CalcSpriteDimensions(CSprite* sprite, int& maxWidth,
                              int& maxHeight, int& minY);
    void DrawSprite();
    void TickAnimation();
    unsigned long m_lastTick;    // +0x58
    int m_spriteX;               // +0x5c
    int m_spriteY;               // +0x60
    int m_spriteFrame;           // +0x64
    int m_seq;                   // +0x68
    const char* m_sSprite;       // +0x6c
    unsigned char m_palUpdated;  // +0x70
    CSprite* m_pSprite;          // +0x74
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
    void Wait();
    bool AllPlayersReady();
    virtual int handle_message(message& msg);  // slot 3

protected:
    int OnPlayerDrop(CNetMsg* pNetMsg, message& msg);

    unsigned long startTime;             // +0x78
    unsigned long lastMsg;               // +0x7c
    CNetMsgHandlerPause m_netMsgHandler;  // +0x80
    unsigned char playerReady[8];         // +0x90
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
    void WaitForLevels(int fromWho);
    virtual int handle_message(message& msg);  // slot 3

protected:
    int OnPlayerDrop(CNetMsg* pNetMsg, message& msg);
    void OnHeroLevelUpdate(CNetMsg* pNetMsg);

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
    t_complex_net_message(int subType);
    virtual unsigned char read(TAbstractFile* infile);
    virtual unsigned char write(TAbstractFile* outfile) const;
    unsigned char RemoteFn_00512E00(CNetMsg* pNetMsg);
    // 0x512d40, the send half of the 0x512e00 bridge: serialize this
    // message and hand it to the transport (toWho / compress /
    // guaranteed mirror TransmitRemoteData's tail). Ordinal name for
    // the same reason as its receive twin. Not claimed from here.
    unsigned char RemoteFn_00512D40(int toWho, unsigned char compressMsg,
                                    unsigned char guaranteed);

    CNetMsg netmsg;  // +0x04
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

#ifdef HOMM3_REMOTE_BATTLE_DLG_DECLS
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
    // USER-DEFINED, and the DC proves it: E:\gamedcs\events.cpp:6709
    // (dc 0x9cf24) is ~CWaitForRemoteBattleDlg, an events.cpp body.
    // Defined there with a pinned interior so the member teardown CALLS
    // ~CNetMsgHandlerPause / ~CCombatInitMsg (retail 0x4ad130) /
    // ~CAnimatedDlg, retail's exact expansion at DoCombat's one
    // invocation site.
    virtual ~CWaitForRemoteBattleDlg();
    void Wait(int playerPos);
    virtual int handle_message(message& msg);  // slot 3

protected:
    int OnPlayerDrop(CNetMsg* pNetMsg, message& msg);

    int m_playerPos;                         // +0x78
    CCombatInitMsg* m_pCombatInitMsg;        // +0x7c (DC name)

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
#endif

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
    void Save(int x, int y);
    void Restore(unsigned char update);
    unsigned char IsSaved();

protected:
    unsigned char screenSaved;  // +0x38
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
    void Setup(int x, int y, unsigned char sending, unsigned char drawText);
    void Start();
    void SetPercentage(float pct);
    void DrawCurrentFrame() { DrawCurrentSmackFrame(); }
    void Stop();
    void SaveScreen();
    void RestoreScreen();

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
    virtual void CalcDimensions(const char* cText, font* pFont,
                                int& winX, int& winY,
                                int& winWidth, int& winHeight);  // slot 12

protected:
    CGameTransferSmack smack;    // +0x58
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
