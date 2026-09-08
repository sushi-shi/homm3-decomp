// netmsg.h - narrow retail-proven network message layouts
#ifndef HOMM3_NETMSG_H
#define HOMM3_NETMSG_H

#include "struct.h"
#include "hero.h"
#include "victorylossconditions.h"

// The Dreamcast enumerates this ladder from RS_GAME_TRANSMIT_INIT = 1000
// with no gaps, and every value retail has independently produced lands on
// the DC's own name at the same number: 1009 = 0x3f1 RS_COMBAT_TYPE, 1054 =
// 0x41e RS_CLAIM_GENERATOR, 1055 = 0x41f RS_CLAIM_GARRISON. The numbering
// transfers whole, so a retail subtype constant can be named from it.
enum eRS_Messages {
    // CEndPlacementPhaseMsg's retail inline constructor stores 0x3f0;
    // the gapless DC message ladder names that value.
    RS_END_PLACEMENT_PHASE = 0x3f0,
    RS_COMBAT_MAIN = 1006,
    RS_COMBAT_END_PLACEMENT = 1008,
    RS_COMBAT_TYPE = 0x3f1,
    // GATED for exactly the reason RS_ERASE_OBJECT below is - see that
    // note. DC eRS_Messages has RS_SET_VISIBILITY = 1021, and retail's two
    // monolith handlers stamp 0x3fd into the message they transmit.
    RS_SET_VISIBILITY = 0x3fd,
    // The next rung, and the DC ladder is gapless: 1022. Retail's
    // DoEventCoverOfDarkness (0x4a14b0) stamps 0x3fe into a message whose
    // layout is CSetVisibilityMsg's member for member, which is exactly the
    // pairing the two names describe. Gated for the same reason its
    // neighbours are.
    RS_RESET_VISIBILITY = 0x3fe,
    // The complete map-change ladder is corroborated at once by
    // ProcessMapChangeNew's 13-entry jump table. The Dreamcast enum supplies
    // the names; the Windows dispatch and record constructors independently
    // prove every value and the Complete-width payloads.
    RS_MOVE_HERO = 0x41a,
    RS_TELEPORT_HERO = 0x41b,
    RS_CLAIM_MINE = 0x41c,
    RS_CLAIM_TOWN = 0x41d,
    RS_CLAIM_GENERATOR = 0x41e,
    RS_CLAIM_GARRISON = 0x41f,
    RS_CLAIM_SHIPYARD = 0x420,
    RS_BUILD_BOAT = 0x421,
    RS_ERASE_OBJECT = 0x422,
    RS_DEAD_HERO = 0x423,
    RS_RECRUIT_HERO = 0x424,
    RS_DEAD_PLAYER = 0x425,
    RS_HIDE_HERO = 0x426,
    // The adventure dispatcher's case roster, named from the gapless DC
    // eRS_Messages ladder (values 1000..1078 transfer whole; see the note
    // at the top of this enum). Gated to advmgr's view - an ungated
    // enumerator here is a measured include-set trigger (RS_ERASE_OBJECT).
    RS_GAME_TRANSMIT_INIT = 1000,
    RS_GAME_TRANSMIT_MAIN = 1001,
    RS_GAME_TRANSMIT_REQ = 1002,
    RS_GAME_TRANSMIT_END = 1003,
    RS_DESTROY_PLAYER = 1079,
    RS_GAME_TRANSMIT_ACK = 1080,
    RS_GAME_XFER_CONFIRM_END = 1081,
    RS_CHAT_MSG = 1004,
    RS_COMBAT_INIT = 1005,
    RS_PLAYER_DROPPED = 1014,
    RS_TURN_UPDATE = 1016,
    RS_PLAYER_DROP_UPDATE = 1017,
    RS_PLAYER_DEAD = 1018,
    RS_PLAYER_WON = 1019,
    RS_PLAYER_LOST = 1020,
    // The Dreamcast eRS_Messages ladder consumed by the setup/lobby TU.
    // Complete adds the final three transfer-control rungs after DC's
    // 1081 endpoint.
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
    RS_GAME_TRANSMIT_PENDING = 1082,
    RS_GAME_HEADER_INFO_INIT_EX = 1083,
    RS_HEADERS_REQUEST = 1084,
    RS_MAP_CHANGE_START = 1049,
    RS_MAP_CHANGE_END = 1063,
    RS_TRADE_REQUEST = 1064,
    RS_TRADE_REQUEST_DONE = 1065,
    RS_HERO_UPDATE = 1066,
    RS_GIVE_ME_STUFF = 1070,
    RS_PLAYER_ACTIVE = 1071,
    // SendChat's ping command constructs the next two rungs directly.
    RS_PING = 1072,
    // The answer CDPlayHeroes::HandleLowLevelMsg (0x552db0) sends straight
    // back at the DirectPlay layer: retail stores 0x431 into the 0x18-byte
    // CPingResponseMsg it builds on its own frame, one rung above RS_PING.
    RS_PING_REPLY = 1073,
    // Both transfer receiver and ready-player dispatcher consume this rung.
    // Dreamcast names it in the shared message ladder; retail ReceiveSaveGame
    // dispatches value 1015 to the host-status chat notification.
    RS_SET_AS_HOST = 1015,
    // The two consecutive ready-handshake records constructed by
    // CWaitForReadyPlayersDlg.  DC's gapless roster names the rungs;
    // retail stores 0x3f4/0x3f5 in their 20-byte base-only messages.
    RS_HERO_LEVEL_UPDATE = 1011,
    RS_READY_TO_PLAY = 1012,
    RS_ALL_READY_TO_PLAY = 1013,
    // The lobby keepalive pair (DC rungs verbatim); singleselectionwindow's
    // OnPingMsg builds the response as a CPingMsg, whose ctor takes this
    // enum.
    RS_SETUP_PING = 1047,
    RS_SETUP_PING_RESPONSE = 1048,
    RS_GIFT = 1074,
    RS_GIFT_REQUEST = 1075,
    RS_SESSION_LOST = 1076,
    RS_NORMAL_WIN = 1078
};

// Network transfer wire limits shared by the sender and receiver.  Retail's
// sender allocates 0x400 bytes per main message; the fixed 0x1c-byte header
// leaves 996 payload bytes, and its idle watchdog compares against 30000 ms.
enum EGameTransmitLimits {
    GAME_TRANSMIT_MESSAGE_SIZE = 1024,
    GAME_TRANSMIT_PAYLOAD_SIZE = 996,
    GAME_TRANSMIT_TIMEOUT = 30000
};

class CNetMsg {
public:
    // Before normalization: field_00; reference member CNetMsg::m_from.
    int m_from;
    // Before normalization: field_04; reference member CNetMsg::m_dpidFrom.
    int m_dpidFrom;
    // Before normalization: subType.
    int m_subType;
    // Before normalization: size.
    unsigned long m_size;
    // Before normalization: field_10; reference member CNetMsg::m_UncompressedSize.
    int m_uncompressedSize;

    CNetMsg() {}
    // Raw Dreamcast CodeView names these parameters `subType` and `size` and
    // types the first as eRS_Messages (0x2CCD), the same type rendered on the
    // CMapChange `id` parameter. Keep the five body statements in lines
    // 169/172-175 order; member-shadow spelling is source material here.
    VA(0x004f2930, 0x23)  // anchor-callee + exact body, retail-only slot
    CNetMsg(eRS_Messages subType, unsigned long size)
    {
        this->m_subType = subType;
        m_from = -1;
        this->m_size = size;
        m_dpidFrom = 0;
        m_uncompressedSize = 0;
    }
};
SIZE(CNetMsg, 20);

// The header-inline owner delegates to the process-wide recycler. remote.cpp
// can see and inline that wrapper down to delete; command.cpp retains the
// retail out-of-line DestroyMsg call.
// Before normalization (function): DestroyMsg.
// Before normalization (locals): pNetMsg.
void destroyMsg(CNetMsg* netMsg);

class CReadyToPlayMsg : public CNetMsg {
public:
    CReadyToPlayMsg()
        : CNetMsg(RS_READY_TO_PLAY, sizeof(CReadyToPlayMsg)) {}
};
SIZE(CReadyToPlayMsg, 0x14);

class CAllReadyToPlayMsg : public CNetMsg {
public:
    CAllReadyToPlayMsg()
    {
        m_subType = RS_ALL_READY_TO_PLAY;
        m_from = -1;
        m_size = sizeof(CAllReadyToPlayMsg);
        m_dpidFrom = 0;
        m_uncompressedSize = 0;
    }
};
SIZE(CAllReadyToPlayMsg, 0x14);

// Dreamcast publishes this exact five-dword payload and its names. Retail
// Main copies the first four words into the pending action tuple, logs them,
// and seeds the combat RNG from the fifth.
class CCombatMainMsg : public CNetMsg {
public:
    int m_nextAction;
    int m_nextActionExtra;
    int m_nextActionGridIndex;
    int m_nextActionGridIndex2;
    int m_seed;

    CCombatMainMsg(int nextAction, int nextActionExtra,
                   int nextActionGridIndex, int nextActionGridIndex2,
                   int seed)
    {
        m_subType = RS_COMBAT_MAIN;
        m_from = -1;
        m_dpidFrom = 0;
        m_uncompressedSize = 0;
        m_size = sizeof(CCombatMainMsg);
        m_nextAction = nextAction;
        m_nextActionExtra = nextActionExtra;
        m_nextActionGridIndex = nextActionGridIndex;
        m_nextActionGridIndex2 = nextActionGridIndex2;
        m_seed = seed;
    }
};
SIZE(CCombatMainMsg, 0x28);

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
    unsigned char remoteFn00512E00(CNetMsg* netMsg);
    // 0x512d40, the send half of the 0x512e00 bridge: serialize this
    // message and hand it to the transport (toWho / compress /
    // guaranteed mirror TransmitRemoteData's tail). Ordinal name for
    // the same reason as its receive twin. Not claimed from here.
    // The two flags are BOOL, not byte: retail pushes both parameter slots
    // straight through to the transport, which takes `_N` in its own
    // mangled name, and a byte parameter would have to be normalised with a
    // `test`/`setne` pair at each site first.
    unsigned char remoteFn00512D40(int toWho, bool compressMsg,
                                    bool guaranteed);
    // 0x512c80, the DPID-addressed send twin (its args mirror
    // TransmitRemoteDataDPID's tail); CNewPlayerUpdateProc's
    // HandleRequests hands each re-requested header row through it.
    // ADDITIVE 2026-08-27 (round 3) - one declarator; re-measure the
    // include-set-sensitive rows of the five includers on merge.
    unsigned char remoteFn00512C80(unsigned long dpid, bool compressMsg,
                                    bool guaranteed);

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
    // Original: CCombatInitMsg::CCombatInitMsg; netmsg.h:264, dc 0x9caa0.
    // Retail expands this member sequence in DoNetCombat and the wait dialog.
    CCombatInitMsg()
        : t_complex_net_message(RS_COMBAT_INIT)
    {
        m_point = type_point(0, 0, 0);
        m_leftHero = 0;
        m_rightTown = 0;
        m_rightHero = 0;
        m_seed = 0;
        m_winner = 0;
        m_retreatWin = 0;
        m_combatSurrender = 0;
        m_leftOwner = 0;
        m_leftGold = 0;
        m_rightOwner = 0;
        m_rightGold = 0;
    }
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

// Complete retail's resource-trade notification is a compact CNetMsg with
// three dwords at +0x14. HandleTradeRequestMsg proves their player/resource/
// amount roles. The Dreamcast port's same-named class instead embeds two hero
// snapshots, a protocol-level platform divergence.
// The dispatcher-facing message views advmgr's HandleNetMsg reads. Field
// offsets are the handler's own byte-proven reads; the class names come
// from the DC ctor publics (CGameTransmitInitMsg KKK_N_N, CChatMsg PBD,
// CCombatTypeMsg H). Payload member names are ordinal where the DC member
// roster has not been consulted. Gated to advmgr's view.
class CGameTransmitInitMsg : public CNetMsg {
public:
    unsigned long m_fileSize;
    // Before normalization: m_fullGameCRC.
    unsigned long m_fullGameCrc;
    unsigned long m_thisPlayerDead;
    unsigned char m_isDiff;
    unsigned char m_makeOrig;

    CGameTransmitInitMsg(unsigned long fileSize,
                         unsigned long fullGameCRC,
                         unsigned long thisPlayerDead,
                         unsigned char isDiff,
                         unsigned char makeOrig)
        : CNetMsg(RS_GAME_TRANSMIT_INIT, sizeof(CGameTransmitInitMsg)),
          m_fileSize(fileSize),
          m_fullGameCrc(fullGameCRC),
          m_thisPlayerDead(thisPlayerDead),
          m_isDiff(isDiff),
          m_makeOrig(makeOrig)
    {
    }
};
SIZE(CGameTransmitInitMsg, 0x24);

// DC netmsg.h:312..395 supplies every boundary, member and access class.
// Retail TransmitSaveGame independently proves the x86 extents, subtype
// immediates and inline store order at 0x4cafd0.
class CGameTransmitReqMsg : public CNetMsg {
public:
    int m_blockNbr;

    CGameTransmitReqMsg(int blockNbr)
        : CNetMsg(RS_GAME_TRANSMIT_REQ, sizeof(CGameTransmitReqMsg)),
          m_blockNbr(blockNbr)
    {
    }
};
SIZE(CGameTransmitReqMsg, 0x18);

class CGameTransmitMainMsg : public CNetMsg {
public:
    unsigned long m_blockNbr;
    unsigned long m_blockSize;

    // Before normalization (function): CGameTransmitMainMsg::CreateMsg.
    static CGameTransmitMainMsg* createMsg(unsigned long maxSize)
    {
        // DC netmsg.h:325..333 records pTemp first, then pMsg; neither the
        // removed `size` local nor a direct typed allocation exists there.
        // Before normalization (locals): pTemp, pMsg.
        unsigned char* temp = static_cast<unsigned char*>(
            ::operator new(sizeof(CGameTransmitMainMsg) + maxSize));
        memset(temp, 0, sizeof(CGameTransmitMainMsg) + maxSize);
        CGameTransmitMainMsg* msg =
            static_cast<CGameTransmitMainMsg*>(static_cast<void*>(temp));
        msg->m_subType = RS_GAME_TRANSMIT_MAIN;
        return msg;
    }

    // Before normalization (function): CGameTransmitMainMsg::Update.
    // Before normalization (locals): pData, pTemp.
    void update(unsigned char* data, unsigned long blockSize)
    {
        m_blockSize = blockSize;
        // DC netmsg.h:342 exposes this source local and its lifetime. The
        // later ReceiveSaveGame call still uses the separately proven
        // GetData helper boundary.
        unsigned char* temp =
            static_cast<unsigned char*>(static_cast<void*>(this))
            + sizeof(CGameTransmitMainMsg);
        memcpy(temp, data, blockSize);
        m_size = getSize();
    }

    // Before normalization (function): CGameTransmitMainMsg::GetSize.
    unsigned long getSize() { return m_blockSize + sizeof(*this); }
    // Before normalization (function): CGameTransmitMainMsg::GetData.
    unsigned char* getData()
    {
        return static_cast<unsigned char*>(static_cast<void*>(this))
            + sizeof(*this);
    }

protected:
    CGameTransmitMainMsg();
};
SIZE(CGameTransmitMainMsg, 0x1c);

class CGameTransmitConfirmEndMsg : public CNetMsg {
public:
    CGameTransmitConfirmEndMsg()
        : CNetMsg(RS_GAME_XFER_CONFIRM_END,
                  sizeof(CGameTransmitConfirmEndMsg))
    {
    }
};
SIZE(CGameTransmitConfirmEndMsg, 0x14);

class CGameTransmitEndMsg : public CNetMsg {
public:
    // Before normalization: m_iMonthType.
    int m_monthType;
    // Before normalization: m_iMonthTypeExtra.
    int m_monthTypeExtra;
    // Before normalization: m_iWeekType.
    int m_weekType;
    // Before normalization: m_iWeekTypeExtra.
    int m_weekTypeExtra;
    unsigned long m_diffSize;

    // Before normalization (locals): iMonthType, iMonthTypeExtra, iWeekType, iWeekTypeExtra.
    CGameTransmitEndMsg(int monthType, int monthTypeExtra,
                        int weekType, int weekTypeExtra,
                        unsigned long diffSize)
        : CNetMsg(RS_GAME_TRANSMIT_END, sizeof(CGameTransmitEndMsg)),
          m_monthType(monthType),
          m_monthTypeExtra(monthTypeExtra),
          m_weekType(weekType),
          m_weekTypeExtra(weekTypeExtra),
          m_diffSize(diffSize)
    {
    }
};
SIZE(CGameTransmitEndMsg, 0x28);

class CChatMsg : public CNetMsg {
public:
    char m_text[128];

    CChatMsg(const char* text)
        : CNetMsg(RS_CHAT_MSG, 0)
    {
        strncpy(m_text, text, 127);
        m_size = getSize();
    }

    // The wire extent; readers consume only the address of m_text.
    // Before normalization (function): CChatMsg::GetSize.
    unsigned long getSize()
    {
        return strlen(m_text) + sizeof(CNetMsg) + 1;
    }
};

// netmsg.h:423 in the DC roster. Retail's CDPlayHeroes drop paths prove the
// duplicated DPID: DirectPlay's sender cell at +4 and the message payload at
// +0x14 both receive the dropped id.
class CPlayerDropMsg : public CNetMsg {
public:
    unsigned long m_dpid;

    CPlayerDropMsg(unsigned long dpid)
        : CNetMsg(RS_PLAYER_DROPPED, sizeof(CPlayerDropMsg)),
          m_dpid(dpid)
    {
        m_dpidFrom = dpid;
    }
};
SIZE(CPlayerDropMsg, 0x18);

class CPlayerDroppedMsg : public CNetMsg {
public:
    int m_gamePos;
};

// Dreamcast supplies the class/member names and the 24-byte size. Retail's
// constructor stores subtype 0x3f8 (RS_TURN_UPDATE) and the game position at
// +20 in both NextPlayer and advManager::StartLocalPlayerTurn.
class CTurnUpdateMsg : public CNetMsg {
public:
    int m_gamePos;

    CTurnUpdateMsg(int gamePos)
        : CNetMsg(RS_TURN_UPDATE, sizeof(CTurnUpdateMsg)),
          m_gamePos(gamePos) {}
};
SIZE(CTurnUpdateMsg, 24);

class CPlayerDropUpdateMsg : public CNetMsg {
public:
    unsigned long m_dpidDropped;

    // DC netmsg.h:461 supplies the constructor and payload name. Retail's
    // two inlined HandleNewHost copies independently prove the 0x18-byte
    // extent, RS_PLAYER_DROP_UPDATE subtype, and final payload store.
    CPlayerDropUpdateMsg(unsigned long dpidDropped)
        : CNetMsg(RS_PLAYER_DROP_UPDATE, sizeof(CPlayerDropUpdateMsg)),
          m_dpidDropped(dpidDropped) {}
};
SIZE(CPlayerDropUpdateMsg, 0x18);

class CPlayerDeadMsg : public CNetMsg {
public:
    int m_gamePos;

    // kb.obj's HandleRemoteDeadPlayerExit (0x4f4c00) builds this message at
    // both of its transmit sites and proves the whole record: the base
    // constructor's five stores in their declared order, the RS_PLAYER_DEAD
    // subtype, a 0x18 extent, and the seat number landing at +0x14.
    CPlayerDeadMsg(int gamePos)
        : CNetMsg(RS_PLAYER_DEAD, sizeof(CPlayerDeadMsg)),
          m_gamePos(gamePos) {}
};
SIZE(CPlayerDeadMsg, 0x18);

// DC netmsg.h:488 supplies the class and all four payload names. Retail's
// CLevelPickWaitDlg dispatcher independently proves the 0x3c-byte wire
// extent and every PC offset while copying the two skill bands into a hero.
class CHeroLevelUpdateMsg : public CNetMsg {
public:
    // DC netmsg.h:488 (dc 0x9cb78): DoCombat expands this header body.
    CHeroLevelUpdateMsg(int hero, int numSSs,
                        signed char* ssLevel,
                        signed char* stats)
        : CNetMsg(RS_HERO_LEVEL_UPDATE, sizeof(CHeroLevelUpdateMsg))
    {
        m_hero = hero;
        memcpy(m_ssLevel, ssLevel, sizeof(m_ssLevel));
        memcpy(m_stats, stats, sizeof(m_stats));
        m_numSSs = numSSs;
    }

    int m_hero;                    // +0x14
    signed char m_ssLevel[28];     // +0x18
    signed char m_stats[4];        // +0x34
    int m_numSSs;                  // +0x38
};
SIZE(CHeroLevelUpdateMsg, 0x3c);

// The two remote win/loss messages have one sender/loser dword between the
// common 0x14-byte CNetMsg base and their respective condition records.
// Retail's handlers read both payloads at +0x18 and copy 0x4c bytes for the
// victory form; Dreamcast supplies the class and member roles.
class CPlayerWonMsg : public CNetMsg {
public:
    int m_gamePos;
    VictoryConditionStruct m_victoryCondition;

    // Dreamcast netmsg.h:505 fixes the reference parameter and statement
    // order. Complete expands this constructor into DisplayVCWinLoss while
    // retaining or expanding the CNetMsg base constructor per call site.
    CPlayerWonMsg(int gamePos,
                  VictoryConditionStruct& victoryConditionStruct)
      : CNetMsg(RS_PLAYER_WON, sizeof(CPlayerWonMsg))
    {
        this->m_gamePos = gamePos;
        m_victoryCondition = victoryConditionStruct;
    }
};
SIZE(CPlayerWonMsg, 0x64);

class CPlayerLostMsg : public CNetMsg {
public:
    int m_loser;
    LossConditionStruct m_lossCondition;

    // kb.obj's SendPlayerLost builds this at all three DisplayLCWinLoss
    // arms. The member's own default constructor runs before the body's
    // assignment - retail stores Type/-1, GameLost/0 and playerLoser/-1
    // into the frame copy and then overwrites all 36 bytes with the
    // rep movsd, which is what proves the two-statement body rather than
    // a member-initialiser.
    CPlayerLostMsg(int loser, LossConditionStruct& lossConditionStruct)
      : CNetMsg(RS_PLAYER_LOST, sizeof(CPlayerLostMsg))
    {
        this->m_loser = loser;
        m_lossCondition = lossConditionStruct;
    }
};
SIZE(CPlayerLostMsg, 0x3c);

class CMapChange : public CNetMsg {
public:
    CMapChange() {}
    // Dreamcast netmsg.h:532 names the parameters `id` and `size` and keeps
    // this CNetMsg construction as a distinct source boundary.
    CMapChange(eRS_Messages id, unsigned long size)
        : CNetMsg(id, size) {}
};

// Dreamcast CodeView names CMCMoveHero and its m_heroId/m_dir/m_standEnd/
// m_point sequence. Complete keeps the same source object but packs the
// point at +0x17 rather than Dreamcast's +0x18: OnMoveHero (0x481ed0) reads
// the three byte fields at +0x14..+0x16 and compares the packed coordinate
// through word loads at +0x17/+0x19. The unsigned/signed split is likewise
// retail-proven by zero-extending heroId, sign-extending dir, and testing
// standEnd as a byte before the MoveHero call.
#pragma pack(push, 1)
class CMCMoveHero : public CMapChange {
public:
    unsigned char m_heroId;
    signed char m_dir;
    unsigned char m_standEnd;
    type_point m_point;

    // netmsg.h:547-551 in Dreamcast. The wire-size field is rounded to the
    // retail record's dword boundary although Complete packs the point at
    // +0x17 and therefore gives the C++ object a 0x1b extent.
    CMCMoveHero(unsigned char heroId, signed char direction,
                unsigned char standEnd, type_point point)
        : CMapChange(RS_MOVE_HERO, 0x1c),
          m_heroId(heroId), m_dir(direction), m_standEnd(standEnd),
          m_point(point) {}
};
#pragma pack(pop)
SIZE(CMCMoveHero, 0x1b);

// Dreamcast's teleport payload is char+padding+point; Windows widens the id
// to the dword ProcessMapChangeNew reads at +0x14, leaving the packed point at
// +0x18 and the same 0x1c wire extent.
class CMCTeleportHero : public CMapChange {
public:
    int m_heroId;
    type_point m_point;

    // Retail has NO out-of-line body: advManager::TeleportTo (0x41d930) is
    // the only constructor site in the image and expands it, sharing the
    // CNetMsg base's zero register with the `gCompleteDrawEnabled = 0` store
    // above it. Same member-initialiser shape as CMCMoveHero's next door.
    CMCTeleportHero(int id, type_point location)
        : CMapChange(RS_TELEPORT_HERO, 0x1c), m_heroId(id), m_point(location) {}
};
SIZE(CMCTeleportHero, 0x1c);

// Dreamcast CodeView names both classes and both constructors
// (netmsg.h:577 / netmsg.h:591, dc 0x8f2c8 / 0x8f2fc), and the constructors
// are the only bodies retail keeps - expanded into the two recorders. The
// 0x1c extent and the +0x14 / +0x18 member offsets are what those
// expansions store, in the CNetMsg base's own field order.
class CMCClaimMine : public CMapChange {
public:
    // Before normalization: mineId.
    signed char m_mineId;
    // Before normalization: playerPos.
    int m_playerPos;

    CMCClaimMine(signed char id, int player)
        : CMapChange(RS_CLAIM_MINE, sizeof(CMCClaimMine))
    {
        m_mineId = id;
        m_playerPos = player;
    }
};
SIZE(CMCClaimMine, 0x1c);

class CMCClaimTown : public CMapChange {
public:
    // Before normalization: townId.
    signed char m_townId;
    // Before normalization: playerPos.
    int m_playerPos;

    CMCClaimTown(signed char id, int player)
        : CMapChange(RS_CLAIM_TOWN, sizeof(CMCClaimTown))
    {
        m_townId = id;
        m_playerPos = player;
    }
};
SIZE(CMCClaimTown, 0x1c);

class CMCClaimGenerator : public CMapChange {
public:
    // Before normalization: generatorId.
    int m_generatorId;
    // Before normalization: playerPos.
    int m_playerPos;

    CMCClaimGenerator(int id, int player)
        : CMapChange(RS_CLAIM_GENERATOR, sizeof(CMCClaimGenerator)),
          m_generatorId(id), m_playerPos(player) {}
};

class CMCClaimGarrison : public CMapChange {
public:
    int m_garrisonId;
    int m_playerPos;

    // Dreamcast netmsg.h:619 owns the two-argument constructor. Complete
    // retains its out-of-line copy in game.obj after RandomizeEvents.
    VA(0x004c23e0, 0x31)  // retained game.obj copy, dc 0xbd290
    CMCClaimGarrison(int id, int player)
        : CMapChange(RS_CLAIM_GARRISON, sizeof(CMCClaimGarrison)),
          m_garrisonId(id), m_playerPos(player)
    {
    }
};

class CMCClaimShipYard : public CMapChange {
public:
    // Before normalization: point.
    type_point m_point;
    // Before normalization: playerPos.
    int m_playerPos;

    CMCClaimShipYard(type_point location, int player)
        : CMapChange(RS_CLAIM_SHIPYARD, sizeof(CMCClaimShipYard)),
          m_point(location), m_playerPos(player) {}
};

class CMCBuildBoat : public CMapChange {
public:
    // Before normalization: point.
    type_point m_point;
    // Before normalization: playerPos.
    int m_playerPos;

    CMCBuildBoat(type_point location, int player)
        : CMapChange(RS_BUILD_BOAT, sizeof(CMCBuildBoat)),
          m_point(location), m_playerPos(player) {}
};

// Dreamcast CodeView names the class, its single `m_point` member at +0x14
// and the netmsg.h:662 constructor that takes the point BY VALUE.
// advManager::EraseObj (0x4aabb0) builds it on the stack and hands it
// straight to SendMapChange, which fixes the 0x18 extent and the 0x422
// subtype - the value that sits between RS_BUILD_BOAT and RS_TELEPORT_HERO
// in the same ladder.
class CMCEraseObject : public CMapChange {
public:
    type_point m_point;

    CMCEraseObject(type_point location)
        : CMapChange(RS_ERASE_OBJECT, sizeof(CMCEraseObject)),
          m_point(location) {}
};
SIZE(CMCEraseObject, 0x18);

// DC netmsg.h:675 (dc 0xd5964, a hero.obj COMDAT); retail /Ob2-expands it
// inside hero::Deallocate, whose 0x1c-byte frame record and 0x423 subtype
// fix both the extent and the rung. `heroId` is an INT, not the DC row's
// signed char: retail copies the whole dword out of hero::id.
class CMCDeadHero : public CMapChange {
public:
    int m_heroId;
    type_point m_point;

    CMCDeadHero(int id, type_point location)
        : CMapChange(RS_DEAD_HERO, sizeof(CMCDeadHero)),
          m_heroId(id),
          m_point(location)
    {
    }
};
SIZE(CMCDeadHero, 0x1c);

// The old model called this 0x20-byte 0x424 record CMCTeleportHero. The
// Windows dispatcher proves it is the next ladder entry, CMCRecruitHero:
// hero id at +0x14, point at +0x18 and player position at +0x1c. Dreamcast
// independently publishes the same three-member class and constructor.
class CMCRecruitHero : public CMapChange {
public:
    int m_heroId;
    type_point m_point;
    int m_playerPos;

    CMCRecruitHero(int id, type_point location, int player)
        : CMapChange(RS_RECRUIT_HERO, sizeof(CMCRecruitHero)),
          m_heroId(id), m_point(location), m_playerPos(player)
    {
    }
};
SIZE(CMCRecruitHero, 0x20);

// kb.obj's PlayerDead expands this constructor at its one broadcast site
// and proves the whole record: CNetMsg's five stores in their declared
// order, the RS_DEAD_PLAYER subtype, a 0x18 extent, and the seat number
// landing at +0x14.
class CMCDeadPlayer : public CMapChange {
public:
    int m_playerPos;

    CMCDeadPlayer(int player)
        : CMapChange(RS_DEAD_PLAYER, sizeof(CMCDeadPlayer)),
          m_playerPos(player) {}
};
SIZE(CMCDeadPlayer, 0x18);

// game.obj opens this on its own narrow gate: playerData::add_garrison_hero
// (0x4b9fc0) broadcasts the same record town::SwapHeroes does.
class CMCHideHero : public CMapChange {
public:
    int m_heroId;

    // Dreamcast netmsg.h:717-718 proves the CMapChange construction boundary
    // is followed by a distinct heroId assignment statement. Retail lowers
    // this coherently in add_garrison_hero. Retail SwapHeroes schedules the
    // same store early with the id in ECX; the present coherent caller instead
    // assigns it EAX and zeros through ECX. That compiler-state residual cannot
    // justify reversing the attested source order.
    // Raw CodeView names the T_INT4 parameter `heroId`; the member-shadowing
    // body assignment is the distinct netmsg.h:718 statement.
    CMCHideHero(int heroId)
        : CMapChange(RS_HIDE_HERO, sizeof(CMCHideHero))
    {
        this->m_heroId = heroId;
    }
};

// advmgr.obj joins the gate for CSetVisibilityMsg alone (its two
// visibility dispatch arms read it); split guard, CMCEraseObject and the
// reset twin stay events-view-only.
// NOT a CMapChange - the Dreamcast classes list gives CSetVisibilityMsg a
// 32-byte extent, three members and the base CNetMsg directly, and retail
// agrees on both counts: the monolith handlers hand it to
// TransmitRemoteData rather than to SendMapChange. The three members are
// the DC's own m_point / m_playerPos / m_range (members list, DC offsets
// 20/24/28), and retail's inlined constructor writes them at +0x14 / +0x18
// / +0x1c of a 0x20-byte frame record whose subtype is 0x3fd.
class CSetVisibilityMsg : public CNetMsg {
public:
    type_point m_point;
    int m_playerPos;
    int m_range;

    CSetVisibilityMsg(type_point point, int playerPos, int range)
        : CNetMsg(RS_SET_VISIBILITY, sizeof(CSetVisibilityMsg)),
          m_point(point), m_playerPos(playerPos), m_range(range) {}
};
SIZE(CSetVisibilityMsg, 0x20);

// CResetVisibilityMsg (netmsg.h:747 in the DC roster, dc 0x9ccd4) is
// CSetVisibilityMsg with the opposite subtype and nothing else changed:
// advManager::DoEventCoverOfDarkness builds a 0x20-byte frame record with
// -1/0/0x3fe/0x20/0 in the CNetMsg base and the point, the player and the
// range at +0x14/+0x18/+0x1c, then hands it to TransmitRemoteData exactly
// as the monolith pair hands over the set message.
class CResetVisibilityMsg : public CNetMsg {
public:
    type_point m_point;
    int m_playerPos;
    int m_range;

    CResetVisibilityMsg(type_point point, int playerPos, int range)
        : CNetMsg(RS_RESET_VISIBILITY, sizeof(CResetVisibilityMsg)),
          m_point(point), m_playerPos(playerPos), m_range(range) {}
};
SIZE(CResetVisibilityMsg, 0x20);

// DC netmsg.h:758 names the base-only message; retail ResetRound expands
// this constructor in place and proves both the store order and 0x14 extent.
class CEndPlacementPhaseMsg : public CNetMsg {
public:
    CEndPlacementPhaseMsg()
        : CNetMsg(RS_END_PLACEMENT_PHASE,
                  sizeof(CEndPlacementPhaseMsg)) {}
};
SIZE(CEndPlacementPhaseMsg, 0x14);

// Dreamcast CodeView names this one-dword CNetMsg derivative and its
// `m_quick` member. Retail DoModal independently proves the 0x18-byte
// extent, RS_COMBAT_TYPE subtype and member at +0x14.
class CCombatTypeMsg : public CNetMsg {
public:
    int m_quick;

    CCombatTypeMsg(int quick)
    {
        m_quick = quick;
        m_subType = RS_COMBAT_TYPE;
        m_from = -1;
        m_size = sizeof(CCombatTypeMsg);
        m_dpidFrom = 0;
        m_uncompressedSize = 0;
    }
};
SIZE(CCombatTypeMsg, 0x18);

class CTradeRequestMsg : public CNetMsg {
public:
    int m_playerPos;
    int m_resource;
    int m_amount;
};
SIZE(CTradeRequestMsg, 0x20);

// netmsg.h:804 in the Dreamcast roster. Retail SendChat independently
// proves the one-dword payload, 0x18-byte extent and constructor store order.
class CPingMsg : public CNetMsg {
public:
    unsigned long m_pingTime;

    CPingMsg(unsigned long pingTime, eRS_Messages id)
        : CNetMsg(id, sizeof(CPingMsg)), m_pingTime(pingTime) {}
};
SIZE(CPingMsg, 0x18);

// netmsg.h:815 in the Dreamcast roster - a distinct class from CPingMsg
// above with the same one-dword payload and the same two-argument
// constructor (DC ??0CPingResponseMsg@@QAA@KW4eRS_Messages@@@Z stores only
// the base and the echoed time). Retail's HandleLowLevelMsg proves the same
// 0x18-byte extent and store order on its own frame.
class CPingResponseMsg : public CNetMsg {
public:
    unsigned long m_pingTime;

    CPingResponseMsg(unsigned long pingTime, eRS_Messages id)
        : CNetMsg(id, sizeof(CPingResponseMsg)), m_pingTime(pingTime) {}
};
SIZE(CPingResponseMsg, 0x18);

// The two gift messages extend the shared 20-byte network-message head.
// Their subtype and total-size constants are the immediates retail stores at
// +8/+0xc; the derived payload offsets agree with the DC member roster.
class CGiftMsg : public CNetMsg {
public:
    int m_niceGuy;
    int m_resource;
    int m_qty;

    CGiftMsg(int niceGuy, int resource, int qty)
        : CNetMsg(RS_GIFT, sizeof(CGiftMsg)), m_niceGuy(niceGuy),
          m_resource(resource), m_qty(qty) {}
};
SIZE(CGiftMsg, 32);

class CGiftRequestMsg : public CNetMsg {
public:
    int m_greedyGuy;
    int m_resource;

    CGiftRequestMsg(int greedyGuy, int resource)
        : CNetMsg(RS_GIFT_REQUEST, sizeof(CGiftRequestMsg)),
          m_greedyGuy(greedyGuy),
          m_resource(resource) {}
};

SIZE(CGiftRequestMsg, 28);

// The normal-win notification carries only the winning network game slot.
// Retail's handler reads the dword immediately after CNetMsg at +0x14;
// Dreamcast CodeView supplies the class/member identity.
class CNormalWinMsg : public CNetMsg {
public:
    int m_gamePos;

    // kb.obj's CheckEndGame (0x4f2ce0) builds this message on the
    // last-team-standing path and proves the whole record: the base
    // constructor's five stores in their declared order, RS_NORMAL_WIN as
    // the subtype, a 0x18 extent, and the winning seat landing at +0x14.
    CNormalWinMsg(int gamePos) : CNetMsg(RS_NORMAL_WIN, sizeof(CNormalWinMsg))
    {
        this->m_gamePos = gamePos;
    }
};
SIZE(CNormalWinMsg, 0x18);

class CDestroyPlayerMsg : public CNetMsg {
public:
    unsigned long m_dpid;

    CDestroyPlayerMsg(unsigned long dpid)
        : CNetMsg(RS_DESTROY_PLAYER, sizeof(CDestroyPlayerMsg)),
          m_dpid(dpid)
    {
    }
};
SIZE(CDestroyPlayerMsg, 0x18);

// DC netmsg.h:882 proves this header-defined boundary. Complete retains no
// standalone copy: ReceiveSaveGame expands the five CNetMsg stores at the
// every-thirtieth-block acknowledgement site (subtype 1080, size 0x14).
class CGameXferAckMsg : public CNetMsg {
public:
    CGameXferAckMsg()
        : CNetMsg(RS_GAME_TRANSMIT_ACK, sizeof(CGameXferAckMsg))
    {
    }
};
SIZE(CGameXferAckMsg, 0x14);

#endif  // HOMM3_NETMSG_H
