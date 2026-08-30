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

// The setup/lobby record ids live on Dreamcast's one eRS_Messages ladder.
// singleselectionwindow.cpp opens their scoped netmsg.h view before its shared
// includes. Complete's three post-1081 transfer-control rungs are byte-proven
// at their build sites in this TU.

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
    CScrollMsg(int map, int index);
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
    CSetFilterMsg(int size);
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

class CGameHeaderInfoEndMsg : public CNetMsg {
public:
    CGameHeaderInfoEndMsg();
};

class CClickMsg : public CNetMsg {
public:
    int m_widgetId;  // +0x14
    CClickMsg(int widgetId);
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

    CNewSetupInfoMsg(SGameSetupOptions* setup);
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

// DC's original 1024 header-transfer opener. CodeView proves the base
// CGameHeaderInfoInitMsg(numMaps, loadGameMode, msgSize) boundary and the
// derived CGameHeaderInfoInitMsgEx(version, numMaps, loadGameMode) boundary;
// Complete retail expands both into CNewPlayerUpdateProc::Go. The receiver
// independently proves the resulting count/mode/version layout and 0x30
// extent. Keep these source boundaries even though VC6 /Ob2 erases them.
class CGameHeaderInfoInitMsg : public CNetMsg {
public:
    unsigned long m_numMaps;   // +0x14
    unsigned char m_netGame;   // +0x18
    char pad_19[3];

    CGameHeaderInfoInitMsg(unsigned long numMaps,
                           unsigned char loadGameMode,
                           unsigned long msgSize)
        : CNetMsg(RS_GAME_HEADER_INFO_INIT, msgSize)
    {
        m_numMaps = numMaps;
        m_netGame = loadGameMode;
    }
};

class CGameHeaderInfoInitMsgEx : public CGameHeaderInfoInitMsg {
public:
    char m_version[20];        // +0x1c

    CGameHeaderInfoInitMsgEx(const char* version, unsigned long numMaps,
                             unsigned char loadGameMode)
        : CGameHeaderInfoInitMsg(numMaps, loadGameMode,
                                 sizeof(CGameHeaderInfoInitMsgEx))
    {
        memset(m_version, 0, sizeof(m_version));
        strncpy(m_version, version, sizeof(m_version) - 1);
    }
};

// The join announcement: the joining player's full CNetPlayerInfo
// record plus a version string tail. OnNewPlayerMsg reads the record
// at +0x14, the name at +0x18 and gates on the version at +0x34; the
// sender's dpid rides the CNetMsg field_04 slot.
class CNewPlayerMsg : public CNetMsg {
public:
    CNetPlayerInfo m_playerInfo;  // +0x14 (dpid/sName/version int)
    char m_version[20];           // +0x34

    CNewPlayerMsg(CNetPlayerInfo* pPlayerInfo, char* version);
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
    CNetPlayerHandlerPlayer m_netPlayer[8];   // +0x014
    CNetPlayerHandlerPlayer m_compPlayer[8];  // +0x3f4

    // OnNewPlayerMsg's broadcast site proves the pair: sizeof is the
    // 0x7d4 size dword. Retail also runs the CNetPlayerHandlerPlayer
    // ctor (0x57c790) over both arrays - one inline loop, one ??_L
    // vector-iterator call - which our POD record model cannot emit;
    // that delta is OnNewPlayerMsg's documented residual.
    CUpdatePlayerPosMsg()
        : CNetMsg(RS_UPDATE_PLAYER_POS, sizeof(CUpdatePlayerPosMsg))
    {
    }

    // E:\gamedcs\singleselectionwindow.cpp:717
    CUpdatePlayerPosMsg(CNetPlayerHandlerPlayer* pNetPlayers,
                        CNetPlayerHandlerPlayer* pCompPlayers);
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

// Dreamcast names the selected row's difficulty mirror `lastDiff`;
// Complete retains it at .data 0x683454 (initial 1).
extern int lastDiff;
// Shared game snapshot owned by campaignbrief.cpp; Dreamcast publishes this
// exact `saveHeader` identity at UpdateGameVars' BackupGameHeaders call.
extern game* saveHeader;

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
    FILETIME m_fileTime;          // +0x7c

    CMapFileNameMsg(unsigned char flag, int number, const char* fileName,
                    int* townTypes, FILETIME fileTime)
        : CNetMsg(RS_MAP_FILE_NAME, sizeof(CMapFileNameMsg))
    {
        m_flag = flag;
        m_number = number;
        strncpy(m_fileName, fileName, 0x3c);
        m_fileTime = fileTime;
        memcpy(m_townTypes, townTypes, sizeof(m_townTypes));
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

// Complete's retail-only 1083 opener for TransferHeaders. It is the compact
// count-only sibling of DC's original 1024 message above.
class CTransferHeaderInfoInitMsg : public CNetMsg {
public:
    unsigned long m_numMaps;  // +0x14

    CTransferHeaderInfoInitMsg(unsigned long numMaps)
        : CNetMsg(RS_GAME_HEADER_INFO_INIT_EX,
                  sizeof(CTransferHeaderInfoInitMsg))
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

// Shared state/interface for the two retail header-transfer jobs. Complete
// has two three-slot vtables over this exact 0x24-byte layout:
//
//   0x641d38: 0x577d70 / 0x577de0 / 0x578930 (t_map_list_update)
//   0x641d44: 0x5789f0 / 0x578a90 / 0x5795a0 (CNewPlayerUpdateProc)
//
// The names of the concrete tables come from secondary IDA evidence; their
// separation, slot contents and construction sites are retail-byte facts.
// Dreamcast had only CNewPlayerUpdateProc. Its Go source shape survives in
// Complete's 0x5789f0 base implementation; Complete adds the
// t_map_list_update overrides for the separate 1083 TransferHeaders path.
// Constructor scheduling proves an inheritance chain rather than two siblings: the
// 0x641d44 vptr is written after shared member construction but before
// CNewPlayerUpdateProc's field-initializing body, while t_map_list_update's
// 0x641d38 override is written after that body. novtable belongs on the shared
// interface: it suppresses an invented base vptr reset in the directly-called
// non-virtual teardown at 0x583ef0 while both concrete levels emit their own
// retail vptr at the source-correct phase.
class __declspec(novtable) CNewPlayerUpdateTask {
public:
    CNewPlayerUpdateTask() {}
    virtual void Go() = 0;
    virtual void Tick() = 0;
    virtual void Finish() = 0;
    ~CNewPlayerUpdateTask();

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

// Complete's CNewPlayerUpdateProc implementation. NewPlayer constructs it
// inline, proving both the 0x641d44 vptr and its placement before this body.
// Go retains the Dreamcast constructor/send shape and is exact at 0x5789f0;
// Tick and Finish retain their complete Dreamcast CFGs at 86.6723% and
// 87.0155% respectively (Finish's rejected wrong-boundary peak is 89.5855%).
class CNewPlayerUpdateProc : public CNewPlayerUpdateTask {
public:
    CNewPlayerUpdateProc(unsigned long dpid)
    {
        m_dpid = dpid;
        m_nextHeader = 0;
        m_finished = 0;
        m_lastSendTime = 0;
    }
    virtual void Go();       // slot 0, 0x5789f0
    virtual void Tick();     // slot 1, 0x578a90
    virtual void Finish();   // slot 2, 0x5795a0
    void RequestConfirmation();  // DC source helper, inlined in retail Tick
    void HandleRequests();       // retail 0x578010
};

// Complete's derived map-list implementation for the added 1083
// TransferHeaders protocol. Its separate vtable and constructor are retail-
// only; Go is exact, while Tick/Finish retain their documented residuals.
class t_map_list_update : public CNewPlayerUpdateProc {
public:
    t_map_list_update(unsigned long dpid);
    virtual void Go();       // slot 0, 0x577d70
    virtual void Tick();     // slot 1, 0x577de0
    virtual void Finish();   // slot 2, 0x578930
};

// The per-lobby set of header-transfer jobs: eight slots, ticked from
// WindowHandler every pump. Tick is defined out of class in the TU
// (retail keeps an out-of-line copy and expands it into WindowHandler).
class CNewPlayerUpdateMan {
public:
    CNewPlayerUpdateTask* m_procs[8];

    // DC GetFirstAvailable; HandleNetMsg's transfer-start arm expands it.
    int GetFirstAvailable()
    {
        for (int i = 0; i < 8; ++i)
            if (m_procs[i] == 0)
                return i;
        return -1;
    }

    // DC GetProc (protected there); the HeaderConfirmed body expands it.
    CNewPlayerUpdateTask* GetProc(unsigned long dpid)
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

// The local network identity. remote.cpp owns the address claim; the
// selection window reads its dpid when choosing the current lobby player.
extern CNetPlayerInfo gsThisNetPlayerInfo;

// The free game-selection message pump (retail dialogDrawFunction, dc
// 0x145128), passed by address to DoDialogDraw alongside HeroWindowHandler.
// message& (not message*) so it binds the int(*)(message&) TDialogHandler.
int Update(message& msg);

#endif  /* HOMM3_SINGLESELECTIONWINDOW_PRIV_H */
