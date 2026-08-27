// singleselectionwindow_priv.h - private widget/dialog classes defined by
// singleselectionwindow.cpp. NOT part of singleselectionwindow.h: townmgr.cpp
// and advmgr.cpp include the public header for TSingleSelectionWindow and the
// four cross-TU globals only, and pulling slider/dialog/netmsg bases into their
// include closures is exactly the include-set perturbation the residual class
// warns about. singleselectionwindow.cpp is the only consumer.
#ifndef HOMM3_SINGLESELECTIONWINDOW_PRIV_H
#define HOMM3_SINGLESELECTIONWINDOW_PRIV_H

#include "inputmgr.h"
#include "slider.h"
#include "textresource.h"
#include "remotedlg.h"
#include "textntry.h"
#include "netmsg.h"
#include "winmgr.h"

// The namespace-level text-resource loader (retail body 0x55bdd0), fastcall
// under /Gr. Declared file-locally rather than pulling resourcemanager.h into
// singleselectionwindow.cpp's include closure.
namespace ResourceManager {
    TTextResource* GetText(const char* name);
}

// The chat/duration/file-menu slider. DC gives it a `slider` base and a
// SetResolution/SetState override pair (slots 13/14 of the 0x241b8c vtable).
// Both bodies read the slider base fields retail's slider.obj proves
// (numStates +0x48, currentState +0x3c, oldState +0x38, knobPos +0x40,
// knobRange +0x44). CChatSlider introduces no field either body reaches.
class CChatSlider : public slider {
public:
    virtual void SetResolution(int num);  // slot 13
    virtual void SetState(int state);     // slot 14
};

// The free remote.obj poll wrapper (0x554400), fastcall under /Gr; one arg.
CNetMsg* GetRemoteData(unsigned char removeFromQueue, unsigned char* wasCompressed);

// The host-wait animated dialog. CAnimatedDlg base is 0x78; handle_message
// proves the two tail fields (the polled message pointer at +0x78, the awaited
// dpid at +0x7c). Its vtable 0x241cf8 replaces CAnimatedDlg slot 0 (the ??_G)
// and slot 3 (handle_message).
class CHostWaitDlg : public CAnimatedDlg {
public:
    virtual ~CHostWaitDlg();
    virtual int handle_message(message& msg);  // slot 3

    CNetMsg* m_pMsg;         // +0x78
    unsigned long m_forWho;  // +0x7c
};

// The selection window's net-message handler. Its scalar deleting destructor
// at 0x58e2e0 calls ~CAdvMgrNetMsgHandler, proving the base; CheckHandleNet
// polls through GetRemoteData into the compression flag at +0xc. vtable
// 0x241ce8 overrides slot 1 (CheckHandleNet) and slot 3 (HandleNetMsg).
class CSingleSelectionNetMsgHandler : public CAdvMgrNetMsgHandler {
public:
    virtual CNetMsg* CheckHandleNet(unsigned char inPopup,
                                    unsigned char* msgReceived);  // slot 1
    virtual CNetMsg* HandleNetMsg(CNetMsg* pNetMsg);              // slot 3

    unsigned char m_wasCompressed;  // +0x0c
};

// The chat text widget. It snapshots the screen region under itself into a
// CChatSave (Bitmap16Bit + a saved flag at +0x38, the CTextEntrySave shape)
// so Draw restores the background before repainting. m_save at +0x50; vtable
// 0x241bdc overrides slot 4 (Draw vs the textWidget base).
class CChatWidget : public textWidget {
public:
    class CChatSave : public Bitmap16Bit {
    public:
        unsigned char bSaved;  // +0x38
        unsigned char IsSaved() const { return bSaved; }
    };

    virtual void Draw();  // slot 4

    CChatSave* m_save;  // +0x50
};

// One cached map/save header row of the file list. Only the stride is
// modeled: 0xCA4 is fixed by the size() magic-multiply in every
// vector<GameSelectionHeadersStruct>::size() expansion (WindowHandler's
// scroll arms): 0x5102371 * 2^-38 is 1/3236 exactly (the /3235 constant
// would take the add-fix form retail lacks). The DC record
// (SingleSelectionWindow.h:73) awaits field reconstruction.
struct GameSelectionHeadersStruct {
    char pad_0[0x314];
    // UpdatePlayerPositions copies the slot's town alignment out of the
    // selected header through this row (int, 8 slots at +0x314).
    int slotAlignments[8];
    char pad_334[0xCA4 - 0x334];
};
SIZE(GameSelectionHeadersStruct, 0xCA4);

// The lobby-only message subtypes past the DC eRS_Messages ladder's
// 1081 end (SoD renumbered/extended the header-transfer family). Values
// byte-proven at their build sites in this TU; TU-private so the shared
// netmsg.h ladder - a measured include-set trigger - is untouched.
enum eRS_LobbyMessages {
    RS_NEW_SETUP_INFO_EX = 0x402,       // 1026 DC RS_NEW_SETUP_INFO
    RS_SETUP_PING_EX = 0x417,           // 1047 DC RS_SETUP_PING
    RS_SETUP_PING_RESPONSE_EX = 0x418,  // 1048 DC RS_SETUP_PING_RESPONSE
    RS_GAME_HEADER_INFO_INIT = 0x43b    // 1083, CGameHeaderInfoInitMsg
};

// DC SingleSelectionWindow.h's header-transfer opener (dc 0x1478a4 takes
// numMaps/loadGameMode/msgSize; retail's inline expansion at Go varies
// only the count - the mode rides the zeroed base field).
class CGameHeaderInfoInitMsg : public CNetMsg {
public:
    unsigned long m_numMaps;  // +0x14

    CGameHeaderInfoInitMsg(unsigned long numMaps)
        : CNetMsg(RS_GAME_HEADER_INFO_INIT, sizeof(CGameHeaderInfoInitMsg))
    {
        m_numMaps = numMaps;
    }
};

// One per-joining-player header-transfer job. Retail vtable 0x641d38
// (stored by the ctor at 0x589240): slot 0 Go (0x577d70), slot 1 Tick
// (0x577de0) - the slot WindowHandler's inlined CNewPlayerUpdateMan::Tick
// dispatches - then four more virtuals (0x578930/0x5789f0/0x578a90/
// 0x5795a0) not yet order-mapped onto the DC HeaderRequested/
// HeaderConfirmed/RequestConfirmation/HandleRequests/Finish roster;
// placeholders keep the slot arithmetic honest. The destructor is
// NON-virtual: retail deletes through a direct call to 0x583ef0.
// Member offsets are the ctor's stores (dpid +4, the +0x10..+0x18
// buffer triple the dtor frees and zeroes, m_finished +0x20).
class __declspec(novtable) CNewPlayerUpdateProc {
public:
    CNewPlayerUpdateProc(unsigned long dpid);
    virtual void Go();          // slot 0, 0x577d70
    virtual void Tick();        // slot 1, 0x577de0
    virtual void _vslot02();    // slot 2, 0x578930
    virtual void _vslot03();    // slot 3, 0x5789f0
    virtual void _vslot04();    // slot 4, 0x578a90
    virtual void _vslot05();    // slot 5, 0x5795a0
    ~CNewPlayerUpdateProc();

    unsigned long m_dpid;        // +0x04
    int field_08;                // +0x08
    unsigned char field_0C;      // +0x0c
    GameSelectionHeadersStruct* m_buffer;  // +0x10, freed by the dtor
    int field_14;                // +0x14
    int field_18;                // +0x18
    int field_1C;                // +0x1c
    unsigned char m_finished;    // +0x20, Tick-loop delete gate

    unsigned char IsFinished() const { return m_finished; }
};

// The per-lobby set of header-transfer jobs: eight slots, ticked from
// WindowHandler every pump. Tick is defined out of class in the TU
// (retail keeps an out-of-line copy and expands it into WindowHandler).
class CNewPlayerUpdateMan {
public:
    CNewPlayerUpdateProc* m_procs[8];

    void Tick();
    void PlayerDropped(unsigned long dpid);  // retail 0x589480
};

// The lobby player-name editor (one per name row, widget ids 353..360).
// vtable 0x241c14 overrides slot 11 (OnKillFocus) and slot 15 (OnKeyPress);
// both bodies expand the shared commit helper OnEnter, which DC keeps out
// of line (dc 0x149238) and retail fully inlines - no retail row exists
// for it, so its definition must be `inline` (cpp-local, this TU only).
class CEnterNameEdit : public textEntryWidget {
public:
    virtual void OnKillFocus();            // slot 11
    virtual int OnKeyPress(message* msg);  // slot 15
    int OnEnter();
};

// The save-filename editor. vtable 0x241c60 overrides slot 15 (OnKeyPress)
// and slot 16 (IgnoreKey).
class CSaveGameEdit : public textEntryWidget {
public:
    virtual int OnKeyPress(message* msg);           // slot 15
    virtual unsigned char IgnoreKey(message* msg);  // slot 16
};

// The persisted multiplayer nickname (prefs "Network Name").
// multiplayerwindow.cpp owns the DATA claim at 0x698817; the name editors
// commit into it before calling WritePrefs.
extern char gLocalPlayerName[21];

// A cross-module dword at 0x6989f0 the game-selection window branches on
// during teardown; DoModal and ExitDialog each take a distinct path when it
// equals 3, the only value recoverable here. House ordinal placeholder,
// exactly the textntry.h EField68 rule - names the domain member so the
// branch is not a magic compare, without claiming an attested identity.
enum EWindowMode6989f0 {
    WINDOW_MODE_6989F0_3 = 3
};
extern int gUnnamed6989f0;

// 0x69954c, the paused-video gate DoModal/ExitDialog test. DECLARATION ONLY
// (kbwin.cpp owns the DATA claim); declared here rather than by pulling
// kbwin.h into this closure, the same reason hero.h states for its own copy.
extern int bVideoPaused;

// The local network identity. remote.cpp owns the address claim; the
// selection window reads its dpid when choosing the current lobby player.
extern CNetPlayerInfo gsThisNetPlayerInfo;

// The free game-selection message pump (retail dialogDrawFunction, dc
// 0x145128), passed by address to DoDialogDraw alongside HeroWindowHandler.
// message& (not message*) so it binds the int(*)(message&) TDialogHandler.
int Update(message& msg);

#endif  /* HOMM3_SINGLESELECTIONWINDOW_PRIV_H */
