// singleselectionwindow_priv.h - private widget/dialog classes defined by
// singleselectionwindow.cpp. NOT part of singleselectionwindow.h: townmgr.cpp
// and advmgr.cpp include the public header for TSingleSelectionWindow and the
// four cross-TU globals only, and pulling slider/dialog/netmsg bases into their
// include closures is exactly the include-set perturbation the residual class
// warns about. singleselectionwindow.cpp is the only consumer.
#ifndef HOMM3_SINGLESELECTIONWINDOW_PRIV_H
#define HOMM3_SINGLESELECTIONWINDOW_PRIV_H

#include <bitset>
#include <vector>

#include "inputmgr.h"
#include "slider.h"
#include "textresource.h"
#include "remotedlg.h"
#include "textntry.h"
#include "netmsg.h"
#include "winmgr.h"
// TurnChatOn/TurnChatOff relabel chatToggle through textButton's
// inherited header-inline SetText (retail expands the std::string
// assign in place, calling only _Grow/_Eos - the button.h shape).
#include "button.h"

// The namespace-level text-resource loader (retail body 0x55bdd0), fastcall
// under /Gr. Declared file-locally rather than pulling resourcemanager.h into
// singleselectionwindow.cpp's include closure.
namespace ResourceManager {
    TTextResource* GetText(const char* name);
}

// misc.cpp's free-space probe (retail 0x50c7a0), declared file-locally
// for SaveValid's disk gate rather than pulling misc.h into this
// closure - the ResourceManager::GetText precedent above.
unsigned long get_available_disk_space();
std::string format_string(const char* format, ...);

// The game-context feature bits and their index cell (game.cpp/
// resourcemanager.cpp own the claims); OnSetAsHostMsg gates the
// game-type widget (0x82) on bit one - the same test(1) game.cpp's
// player-slot reader spells. Declared file-locally, the
// get_available_disk_space precedent.
extern std::bitset<4> gGameContextFeatures[4];
extern int* gpVideoGameState;

// The CRT entries SaveValid/OnMapFileNameMsg touch, declared
// file-locally instead of including <io.h>/<direct.h>: those two
// system headers cost HandleNetMsg 90.16 -> 86.33 through the
// include-set wall (measured 2026-08-27); a few declarators do not.
// _access = retail CRT 0x61a26e (OnMapFileNameMsg's existence gate).
extern "C" {
int __cdecl _chdir(const char* path);
int __cdecl _open(const char* filename, int oflag, ...);
int __cdecl _close(int handle);
int __cdecl _access(const char* path, int mode);
}

// The five difficulty names at .bss 0x6a77ec, indexed by the header's
// difficulty byte in DrawBasicMapInfo's bottom row. Owner TU unlocated -
// extern only, no DATA claim; house unnamed-cell spelling.
extern const char* gUnnamed6a77ec[];

// The starting-bonus name table at .bss 0x6a5e14 (rows 0..2 =
// Artifact/Gold/Resource; the random rung draws general-text 523
// instead), read by DrawHeroAdvancedOption's bonus column in both
// mode arms. No attested name survives - house unnamed-cell spelling.
// Owner TU unlocated - extern only, no DATA claim.
extern const char* gUnnamed6a5e14[];

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


// Update (0x584550) remaps a campaign scenario's version icon from the
// row's campaign ordinal: rows 0..6 are Restoration of Erathia's seven
// campaigns, 7..12 Armageddon's Blade's six, 13..19 Shadow of Death's
// seven - the same shipping-order split campaignwindow.h's
// ECampaignSets pages by (byte table at 0x584bd4: 7x0, 6x1, 7x2).
// Title identities are deliberately not imported (the
// EGameCampaignOrdinal precedent).
enum ECampaignOrdinal {
    CAMPAIGN_ROE_0 = 0,
    CAMPAIGN_ROE_1 = 1,
    CAMPAIGN_ROE_2 = 2,
    CAMPAIGN_ROE_3 = 3,
    CAMPAIGN_ROE_4 = 4,
    CAMPAIGN_ROE_5 = 5,
    CAMPAIGN_ROE_6 = 6,
    CAMPAIGN_AB_0 = 7,
    CAMPAIGN_AB_1 = 8,
    CAMPAIGN_AB_2 = 9,
    CAMPAIGN_AB_3 = 10,
    CAMPAIGN_AB_4 = 11,
    CAMPAIGN_AB_5 = 12,
    CAMPAIGN_SOD_0 = 13,
    CAMPAIGN_SOD_1 = 14,
    CAMPAIGN_SOD_2 = 15,
    CAMPAIGN_SOD_3 = 16,
    CAMPAIGN_SOD_4 = 17,
    CAMPAIGN_SOD_5 = 18,
    CAMPAIGN_SOD_6 = 19
};

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

    // SetCurrentMap's host-broadcast site expands it (the
    // CMapHeaderRequestMsg pattern); the higher-offset store lands
    // first there, as in that ctor.
    CScrollMsg(int map, int index)
        : CNetMsg(RS_SCROLL, sizeof(CScrollMsg))
    {
        m_map = map;
        m_index = index;
    }
};

class CSortMapsMsg : public CNetMsg {
public:
    int m_how;        // +0x14
    int m_direction;  // +0x18

    // SortMaps expands this ctor at its host-broadcast site (the
    // CRequestHeroFaceReplyMsg pattern).
    CSortMapsMsg(int how, int direction)
        : CNetMsg(RS_SORT_MAPS, sizeof(CSortMapsMsg))
    {
        m_how = how;
        m_direction = direction;
    }
};

// The sort columns SortMaps' jump table dispatches (`how`), in the file
// list's column order. Values are the RS_SORT_MAPS payload rungs.
enum ESortMapsColumn {
    SORT_MAPS_BY_NAME = 0,
    SORT_MAPS_BY_PLAYERS = 1,
    SORT_MAPS_BY_VERSION = 2,
    SORT_MAPS_BY_SIZE = 3,
    SORT_MAPS_BY_VICTORY = 4,
    SORT_MAPS_BY_LOSS = 5
};

// field_18A0[6] selects one of the first three filter buttons; the fourth
// button is the aggregate choice. The underlying category names are not yet
// attested, so only the byte-proven aggregate member is named.
enum EScenarioFilterCategory {
    SCENARIO_FILTER_CATEGORY_ANY = 3
};

// The six std::sort predicates SortMaps instantiates - retail's band at
// 0x590070..0x591cf0 is six Dinkumware _Sort instantiations (the
// one-line sort() wrapper inlines to the observed 4-arg
// _Sort(_F,_L,_P,(_Ty*)0) calls) and the out-of-line comparator bodies
// beside them are these functors' operator()s (0x5903b0 tests the
// "autosave" prefix and picks the +0x33d filename over the +0x58c title
// on its +1 byte; 0x590e00 ranks numPlayers*10+maxNumHumanPlayers).
// operator() stays DECLARED-ONLY so every instantiation calls it out of
// line exactly as retail does; the ctors assign in the body - the
// isNet-first store order is the byte-proven one.
struct TSortMapsByName {
    unsigned char direction;  // +0
    unsigned char isNet;      // +1
    TSortMapsByName(unsigned char dir, unsigned char net)
    {
        isNet = net;
        direction = dir;
    }
    bool operator()(const GameSelectionHeadersStruct& a,
                    const GameSelectionHeadersStruct& b) const;
};

struct TSortMapsByPlayers {
    unsigned char direction;  // +0
    TSortMapsByPlayers(unsigned char dir) { direction = dir; }
    bool operator()(const GameSelectionHeadersStruct& a,
                    const GameSelectionHeadersStruct& b) const;
};

struct TSortMapsByVersion {
    unsigned char direction;  // +0
    unsigned char isNet;      // +1
    TSortMapsByVersion(unsigned char dir, unsigned char net)
    {
        isNet = net;
        direction = dir;
    }
    bool operator()(const GameSelectionHeadersStruct& a,
                    const GameSelectionHeadersStruct& b) const;
};

struct TSortMapsBySize {
    unsigned char direction;  // +0
    TSortMapsBySize(unsigned char dir) { direction = dir; }
    bool operator()(const GameSelectionHeadersStruct& a,
                    const GameSelectionHeadersStruct& b) const;
};

struct TSortMapsByVictory {
    unsigned char direction;  // +0
    TSortMapsByVictory(unsigned char dir) { direction = dir; }
    bool operator()(const GameSelectionHeadersStruct& a,
                    const GameSelectionHeadersStruct& b) const;
};

struct TSortMapsByLoss {
    unsigned char direction;  // +0
    TSortMapsByLoss(unsigned char dir) { direction = dir; }
    bool operator()(const GameSelectionHeadersStruct& a,
                    const GameSelectionHeadersStruct& b) const;
};

class CSetFilterMsg : public CNetMsg {
public:
    int m_size;  // +0x14

    // SetFilter expands this ctor at its host-broadcast site.
    CSetFilterMsg(int size)
        : CNetMsg(RS_SET_FILTER, sizeof(CSetFilterMsg))
    {
        m_size = size;
    }
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
    // Extent 80 byte-proven by OnGameHeaderInfoInitMsg's reply: the
    // strncpy bound 0x50 AND the inlined ctor's 0x78 size dword agree.
    char m_errText[80];   // +0x28, format string

    CBadVersionMsg()
        : CNetMsg(RS_BAD_VERSION, sizeof(CBadVersionMsg))
    {
    }
};

// The 1024-path long form of the header-transfer opener: count, the
// net-mode byte and the sender's 20-char version. The receiver
// distinguishes the two forms by the size dword (0x30 long vs the
// version-less EX short form) and falls back to "1.0".
class CGameHeaderInfoInitLongMsg : public CNetMsg {
public:
    unsigned long m_numMaps;   // +0x14
    unsigned char m_netGame;   // +0x18
    char pad_19[3];
    char m_version[20];        // +0x1c
};

// The join announcement: the joining player's full CNetPlayerInfo
// record plus a version string tail. OnNewPlayerMsg reads the record
// at +0x14, the name at +0x18 and gates on the version at +0x34; the
// sender's dpid rides the CNetMsg field_04 slot.
class CNewPlayerMsg : public CNetMsg {
public:
    CNetPlayerInfo m_playerInfo;  // +0x14 (dpid/sName/version int)
    char m_version[20];           // +0x34
};

// The per-row header-transfer payload: a t_complex_net_message wrapping
// one full GameSelectionHeadersStruct plus the transfer flag and row
// number. vtable 0x641b20; OnGameHeaderInfoMsg's inline construction
// calls the base default ctor 0x512c20 and the record ctor COMDAT
// 0x578e00, and 0x578760 is the compiler-generated teardown.
class CNewMapHeaderInfoMsg : public t_complex_net_message {
public:
    unsigned char m_flag;                 // +0x18, 1 = a TransferHeaders row
    char pad_19[3];
    int m_number;                         // +0x1c
    GameSelectionHeadersStruct m_header;  // +0x20

    CNewMapHeaderInfoMsg() {}
    ~CNewMapHeaderInfoMsg() {}
    virtual unsigned char read(TAbstractFile* infile);
    virtual unsigned char write(TAbstractFile* outfile) const;
};

// The re-requested-row reply (subtype RS_GAME_HEADER_INFO through the
// t_complex_net_message subtype ctor 0x512c50): the same
// flag/number/header payload as the transfer stream. Retail widened
// DC's (headerNbr, pHeader) ctor with the list-select flag and
// expands it in HandleRequests - base ctor and the header's member
// assign (the 0x578440 COMDAT) stay calls, the member default
// construction and the flag/number stores inline.
class CGameHeaderInfoMsg : public t_complex_net_message {
public:
    unsigned char m_flag;                 // +0x18
    char pad_19[3];
    int m_number;                         // +0x1c
    GameSelectionHeadersStruct m_header;  // +0x20

    CGameHeaderInfoMsg(unsigned char flag, int number,
                       GameSelectionHeadersStruct* pHeader)
        : t_complex_net_message(RS_GAME_HEADER_INFO)
    {
        m_flag = flag;
        m_number = number;
        m_header = *pHeader;
    }
};

// The full-roster broadcast (DC ctor takes both player arrays); the
// receiver reads the human records at +0x14 and the computer block at
// +0x3f4.
class CUpdatePlayerPosMsg : public CNetMsg {
public:
    CNetPlayerHandlerPlayer m_players[8];      // +0x014
    CNetPlayerHandlerPlayer m_compPlayers[8];  // +0x3f4

    // OnNewPlayerMsg's broadcast site proves the pair: sizeof is the
    // 0x7d4 size dword. Retail also runs the CNetPlayerHandlerPlayer
    // ctor (0x57c790) over both arrays - one inline loop, one ??_L
    // vector-iterator call - which our POD record model cannot emit;
    // that delta is OnNewPlayerMsg's documented residual.
    CUpdatePlayerPosMsg()
        : CNetMsg(RS_UPDATE_PLAYER_POS, sizeof(CUpdatePlayerPosMsg))
    {
    }
};

// The per-handicap label pointers the seat rows are retitled from
// (cell 0x6a7800, owner unclaimed). Declared as int cells: the only
// consumer (SetCurrentMap's seat loop) feeds them verbatim into
// send_message's int payload, and the int view spells that without a
// pointer cast.
extern int gUnnamed6a7800[];

// The three advanced-options seat-kind labels (human-or-computer,
// human-only, computer-only) at .bss 0x6a7e18. No published source name
// survives, so the address-based spelling remains provisional.
extern const char* gUnnamed6a7e18[];

// The scenario-description scroller class of the +0x196c widget
// (retail band 0x5ba600..0x5ba920, between text.obj and textntry.obj;
// no DC roster counterpart). SetText (retail 0x5ba6e0, thiscall ret 4)
// refills the lines vector from the font; only that entry is modeled,
// and the class stays abstract - it exists to type the call.
class CScrollTextWidget : public widget {
public:
    void SetText(const char* text);
};

// The selected row's difficulty mirror at .data 0x683454 (initial 1).
// Owner is this TU; unclaimed pending the data phase - the
// gUnnamed6a77ec precedent.
extern int gUnnamed683454;

// The per-row header broadcast Tick streams (subtype 0x406, 0x84 B);
// retail's inline expansion fixes every field offset. DC's ctor takes
// (nbr, fileName, townType, fileTime); retail reads them all from the
// header row plus the list-select flag.
class CMapFileNameMsg : public CNetMsg {
public:
    unsigned char m_flag;         // +0x14
    char pad_15[3];
    int m_number;                 // +0x18
    char m_fileName[0x40];        // +0x1c
    int m_townTypes[8];           // +0x5c
    unsigned int m_fileTimeLow;   // +0x7c
    unsigned int m_fileTimeHigh;  // +0x80

    CMapFileNameMsg(unsigned char flag, int number,
                    GameSelectionHeadersStruct* hdr)
        : CNetMsg(RS_MAP_FILE_NAME, sizeof(CMapFileNameMsg))
    {
        m_flag = flag;
        m_number = number;
        strncpy(m_fileName, hdr->setup.filename, 0x3c);
        m_fileTimeLow = hdr->fileTime.dwLowDateTime;
        m_fileTimeHigh = hdr->fileTime.dwHighDateTime;
        for (int i = 0; i < 8; ++i)
            m_townTypes[i] = hdr->setup.alignment[i];
    }
};

class CReqHeaderConfirmMsg : public CNetMsg {
public:
    CReqHeaderConfirmMsg()
        : CNetMsg(RS_REQ_HEADER_CONFIRM, 0x14)
    {
    }
};

class CNewHostMsg : public CNetMsg {
public:
    unsigned long m_dpidNewHost;  // +0x14

    CNewHostMsg(unsigned long dpidNewHost)
        : CNetMsg(RS_NEW_HOST, sizeof(CNewHostMsg))
    {
        m_dpidNewHost = dpidNewHost;
    }
};

class CMapHeaderRequestMsg : public CNetMsg {
public:
    unsigned char m_flag;  // +0x14
    char pad_15[3];
    int m_number;          // +0x18

    // Retail widened the DC (nbr) ctor with the list-select flag; both
    // CheckMissingHeaders expansions fix the field order - and the
    // STORE order: number lands before flag on every expansion (the
    // CheckMissingHeaders pair and OnMapFileNameMsg's mismatch arm).
    CMapHeaderRequestMsg(unsigned char flag, int number)
        : CNetMsg(RS_MAP_HEADER_REQUEST, 0x1c)
    {
        m_number = number;
        m_flag = flag;
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

// One queued header re-request (CMapHeaderRequestMsg's payload pair);
// CNewPlayerUpdateProc::HandleRequests drains a vector of these. Field
// order is byte-proven by both ends: HeaderRequested's push_back fills
// the byte at +0 and the dword at +4, and HandleRequests reads
// [elem+8*i] as the transfer flag and [elem+8*i+4] as the row number.
struct SHeaderRequest {
    unsigned char m_flag;
    char pad_1[3];
    int m_number;
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
    // Defined inline: Man::NewPlayer expands it (retail 0x58a280's
    // guts), where the novtable model costs only the 0x641d44 vtbl
    // store the real derived ctor 0x589240 performs.
    CNewPlayerUpdateProc(unsigned long dpid)
    {
        m_dpid = dpid;
        m_nextHeader = 0;
        m_finished = 0;
        m_lastSendTime = 0;
    }
    virtual void Go();          // slot 0, 0x577d70
    virtual void Tick();        // slot 1, 0x577de0
    virtual void _vslot02();    // slot 2, 0x578930
    virtual void _vslot03();    // slot 3, 0x5789f0
    virtual void _vslot04();    // slot 4, 0x578a90
    virtual void _vslot05();    // slot 5, 0x5795a0
    void HandleRequests();      // retail 0x578010
    ~CNewPlayerUpdateProc();

    unsigned long m_dpid;           // +0x04
    int m_nextHeader;               // +0x08, next row to send
    // +0x0c..+0x1c: allocator byte + _First/_Last/_End - Tick's
    // (_Last-_First)>>3 emptiness test fixes the 8-byte element, and the
    // dtor's deallocate+zero triple is the inlined vector teardown.
    std::vector<SHeaderRequest> m_requests;
    unsigned long m_lastSendTime;   // +0x1c, Tick's 75-tick throttle
    unsigned char m_finished;       // +0x20, Tick-loop delete gate

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
    // DC NewPlayer (dc 0x14870c, LOCATED round 2 at retail 0x58a280):
    // take the first free slot and start a transfer job for the
    // joining dpid.
    void NewPlayer(unsigned long dpid);  // retail 0x58a280
};

// RESOLVED (round 2): the round-1 "LoadHeadersList" at 0x58eab0 is the
// out-of-line vector<GameSelectionHeadersStruct>::size() COMDAT
// (thiscall on the vector at this+0x1050; seventeen callers). The
// vector view (HOMM3_SSWINDOW_HEADER_VECTORS) spells those sites
// .size() and /Ob2 reproduces the call-vs-expand split per caller.

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

// DC keeps both helpers out of line; retail VC6 expands them at the advanced-
// options call sites.  Keep the source boundaries visible while allowing the
// retail TU to reproduce that lowering.
inline unsigned char TSingleSelectionWindow::IsMultiPlayer()
{
    if (bVideoPaused)
        return 1;
    if (gUnnamed6989f0 == WINDOW_MODE_6989F0_3)
        return 1;
    return 0;
}

inline unsigned char TSingleSelectionWindow::SendPlayerPositions(
    unsigned long dpidTo)
{
    CUpdatePlayerPosMsg msg;
    memcpy(msg.m_players, m_players.humanPlayers, sizeof(msg.m_players));
    memcpy(msg.m_compPlayers, m_players.computerPlayers,
           sizeof(msg.m_compPlayers));
    TransmitRemoteDataDPID(&msg, dpidTo, true, true);
    return 1;
}

// The local network identity. remote.cpp owns the address claim; the
// selection window reads its dpid when choosing the current lobby player.
extern CNetPlayerInfo gsThisNetPlayerInfo;

// The free game-selection message pump (retail dialogDrawFunction, dc
// 0x145128), passed by address to DoDialogDraw alongside HeroWindowHandler.
// message& (not message*) so it binds the int(*)(message&) TDialogHandler.
int Update(message& msg);

#endif  /* HOMM3_SINGLESELECTIONWINDOW_PRIV_H */
