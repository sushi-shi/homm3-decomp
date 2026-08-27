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
// misc.cpp's PRNG pair and kb's fatal exit, declared here so the
// CHostWaitDlg::Wait inline below can reach them (the cpp-local rule
// would hide them from a header inline).
int Random(int min, int max);
void SRand(int iSeed);

// Devil / Arch Devil, ids fixed by army.h's Inferno-run arithmetic
// (Demon 0x30 opens it, 0x35..0x37 close it); the wait dialog rerolls
// its random flavor creature past both. TU-private for the same
// include-set reason army.h scopes its own creature ids.
enum EWaitDialogCreatures {
    WAIT_CREATURE_DEVIL = 0x36,
    WAIT_CREATURE_ARCH_DEVIL = 0x37
};

class CHostWaitDlg : public CAnimatedDlg {
public:
    CHostWaitDlg()
    {
        m_pMsg = 0;
        m_forWho = 0;
    }
    virtual ~CHostWaitDlg();
    virtual int handle_message(message& msg);  // slot 3

    // DC Wait takes the dpid alone; retail's two expansions differ only
    // in the general-text row, so the text rides as a parameter here
    // (provisional widening). Inline - both HandleNetMsg arms expand it,
    // and the virtual Setup/DoModal calls stay virtual because they go
    // through the inlined body's `this`.
    void Wait(unsigned long forWho, const char* cText)
    {
        m_forWho = forWho;
        SRand(GameTime::Get());
        int creature;
        do {
            creature = Random(0, 111);
        } while (creature == WAIT_CREATURE_ARCH_DEVIL
                 || creature == WAIT_CREATURE_DEVIL);
        Setup(cText, gpMediumFont,
              akCreatureTypeTraits[creature].m_sprite_name, 0);
        DoModal(0);
    }

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
    char pad_334[0x4a5 - 0x334];
    // CheckMissingHeaders requests every row whose byte here is still
    // clear - the received flag of the transfer.
    unsigned char received;  // +0x4a5
    char pad_4a6[0xCA4 - 0x4a6];
};
SIZE(GameSelectionHeadersStruct, 0xCA4);

// The lobby-only message subtypes past the DC eRS_Messages ladder's
// 1081 end (SoD renumbered/extended the header-transfer family). Values
// byte-proven at their build sites in this TU; TU-private so the shared
// netmsg.h ladder - a measured include-set trigger - is untouched.
// The DC eRS_Messages rungs this TU dispatches that the shared
// netmsg.h ladder does not carry (values are the DC enum verbatim),
// plus the three retail-only rungs past the DC ladder's 1081 end.
// TU-private so netmsg.h - a measured include-set trigger - is
// untouched.
enum eRS_LobbyMessages {
    RS_GAME_HEADER_INFO = 1023,
    RS_GAME_HEADER_INFO_INIT = 1024,
    RS_GAME_HEADER_INFO_END = 1025,
    RS_NEW_SETUP_INFO = 1026,
    RS_SCROLL = 1027,
    RS_NEW_MAP_HEADER_INFO = 1028,
    RS_MAP_HEADER_REQUEST = 1029,
    RS_MAP_FILE_NAME = 1030,
    RS_SORT_MAPS = 1031,
    RS_SET_FILTER = 1032,
    RS_REQUEST_HERO_FACE = 1035,
    RS_REQUEST_HERO_FACE_REPLY = 1036,
    RS_SETAGR = 1037,
    RS_NEW_HOST = 1038,
    RS_UPDATE_PLAYER_POS = 1039,
    RS_NEW_PLAYER = 1040,
    RS_REQ_HEADER_CONFIRM = 1041,
    RS_HEADER_CONFIRM = 1042,
    RS_CLICK = 1043,
    RS_TOWN_UPDATE = 1044,
    RS_LAUNCHING_GAME = 1045,
    RS_BAD_VERSION = 1046,
    RS_GAME_TRANSMIT_PENDING = 1082,     // retail-only hold msg
    RS_GAME_HEADER_INFO_INIT_EX = 1083,  // retail-only, CGameHeaderInfoInitMsg
    RS_HEADERS_REQUEST = 1084            // retail-only, starts a transfer job
};

// The lobby message shapes HandleNetMsg reads and builds. All are
// DC-attested class names (SingleSelectionWindow.cpp 473..773); only the
// fields the retail arms touch are modeled, at the offsets the arms fix
// (first derived field at +0x14 over the 0x14-byte CNetMsg base).
class CScrollMsg : public CNetMsg {
public:
    int m_map;    // +0x14
    int m_index;  // +0x18
};

class CSortMapsMsg : public CNetMsg {
public:
    int m_how;        // +0x14
    int m_direction;  // +0x18
};

class CSetFilterMsg : public CNetMsg {
public:
    int m_size;  // +0x14
};

class CRequestHeroFaceMsg : public CNetMsg {
public:
    int m_which;  // +0x14
};

class CRequestHeroFaceReplyMsg : public CNetMsg {
public:
    int m_pos;   // +0x14
    int m_face;  // +0x18

    CRequestHeroFaceReplyMsg(int pos, int face)
        : CNetMsg(RS_REQUEST_HERO_FACE_REPLY,
                  sizeof(CRequestHeroFaceReplyMsg))
    {
        m_pos = pos;
        m_face = face;
    }
};

class CSetAGRMsg : public CNetMsg {
public:
    int m_gamePos;  // +0x14
    int m_agr;      // +0x18
};

class CHeaderConfirmMsg : public CNetMsg {
public:
    CHeaderConfirmMsg()
        : CNetMsg(RS_HEADER_CONFIRM, 0x14)
    {
    }
};

class CClickMsg : public CNetMsg {
public:
    int m_widgetId;  // +0x14
};

class CTownUpdateMsg : public CNetMsg {
public:
    int m_gamePos;  // +0x14
    int m_town;     // +0x18
};

class CNewSetupInfoMsg : public CNetMsg {
public:
    SGameSetupOptions m_setup;  // +0x14
    unsigned char m_flag;       // +0x1e0, the window's +0x37f byte
    char pad_1e1[3];
    int m_extras[8];            // +0x1e4, the window's +0x18a0 run
};

class CBadVersionMsg : public CNetMsg {
public:
    char m_version[20];   // +0x14
    char m_errText[1];    // +0x28, format string (extent unmodeled)
};

// The full-roster broadcast (DC ctor takes both player arrays); the
// receiver reads the human records at +0x14 and the computer block at
// +0x3f4.
class CUpdatePlayerPosMsg : public CNetMsg {
public:
    CNetPlayerHandlerPlayer m_players[8];      // +0x014
    CNetPlayerHandlerPlayer m_compPlayers[8];  // +0x3f4
};

// The per-handicap label pointers the seat rows are retitled from
// (cell 0x6a7800, owner unclaimed).
extern char* gUnnamed6a7800[];

class CNewHostMsg : public CNetMsg {
public:
    unsigned long m_dpidNewHost;  // +0x14
};

class CMapHeaderRequestMsg : public CNetMsg {
public:
    unsigned char m_flag;  // +0x14
    char pad_15[3];
    int m_number;          // +0x18

    // Retail widened the DC (nbr) ctor with the list-select flag; both
    // CheckMissingHeaders expansions fix the field order.
    CMapHeaderRequestMsg(unsigned char flag, int number)
        : CNetMsg(RS_MAP_HEADER_REQUEST, 0x1c)
    {
        m_flag = flag;
        m_number = number;
    }
};

// DC SingleSelectionWindow.h's header-transfer opener (dc 0x1478a4 takes
// numMaps/loadGameMode/msgSize; retail's inline expansion at Go varies
// only the count - the mode rides the zeroed base field).
class CGameHeaderInfoInitMsg : public CNetMsg {
public:
    unsigned long m_numMaps;  // +0x14

    CGameHeaderInfoInitMsg(unsigned long numMaps)
        : CNetMsg(RS_GAME_HEADER_INFO_INIT_EX,
                  sizeof(CGameHeaderInfoInitMsg))
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

    // DC GetFirstAvailable; HandleNetMsg's transfer-start arm expands it.
    int GetFirstAvailable()
    {
        for (int i = 0; i < 8; ++i)
            if (m_procs[i] == 0)
                return i;
        return -1;
    }

    // DC GetProc (protected there); the HeaderConfirmed body expands it.
    CNewPlayerUpdateProc* GetProc(unsigned long dpid)
    {
        for (int i = 0; i < 8; ++i)
            if (m_procs[i] && m_procs[i]->m_dpid == dpid)
                return m_procs[i];
        return 0;
    }

    void Tick();
    void PlayerDropped(unsigned long dpid);  // retail 0x589480
    void HeaderConfirmed(unsigned long dpid);  // retail 0x589270
    // Retail widened the DC (dpid, number) pair with a middle byte; the
    // 1029 arm forwards the request-msg fields verbatim.
    void HeaderRequested(unsigned long dpid, unsigned char flag,
                         int number);  // retail 0x5892b0
};

// The header-cache loader shared by the constructor and the
// header-transfer-end arm (retail 0x58eab0, past the stale span end):
// fastcall on the address of the window's SelectionHeaders vector
// pointers, returning the loaded count. Name provisional - no attested
// spelling survives.
unsigned int LoadHeadersList(void* headerVector);

// Layout-identical message subtype carrying the zeroing default ctor the
// RS_CLICK arm calls out of line (retail 0x589190) - message itself must
// stay ctor-less for every POD-style site in the shared closure (the
// townmgr mage_message precedent).
class lobby_message : public message {
public:
    lobby_message();
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
