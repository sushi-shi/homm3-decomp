// netmsg.h - narrow retail-proven network message layouts
#ifndef HOMM3_NETMSG_H
#define HOMM3_NETMSG_H

#include "struct.h"

// The Dreamcast enumerates this ladder from RS_GAME_TRANSMIT_INIT = 1000
// with no gaps, and every value retail has independently produced lands on
// the DC's own name at the same number: 1009 = 0x3f1 RS_COMBAT_TYPE, 1054 =
// 0x41e RS_CLAIM_GENERATOR, 1055 = 0x41f RS_CLAIM_GARRISON. The numbering
// transfers whole, so a retail subtype constant can be named from it.
enum eRS_Messages {
#ifdef HOMM3_COMMAND_GRID_VIEW
    // CEndPlacementPhaseMsg's retail inline constructor stores 0x3f0;
    // the gapless DC message ladder names that value.
    RS_END_PLACEMENT_PHASE = 0x3f0,
    RS_COMBAT_MAIN = 1006,
    RS_COMBAT_END_PLACEMENT = 1008,
#endif
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
#ifdef HOMM3_EVENT_RECORD_NETMSG_DECLS
    // 1052 and 1053, the two rungs directly below RS_CLAIM_GENERATOR on the
    // gapless DC ladder, and the Dreamcast enumerates both by name. Retail
    // agrees: game::record_claim_mine (0x49bf90) and record_claim_town
    // (0x49c190) stamp 0x41c and 0x41d into 0x1c-byte stack records whose
    // layout is CMCClaimMine's / CMCClaimTown's member for member. GATED
    // for exactly the reason RS_ERASE_OBJECT below is.
    RS_CLAIM_MINE = 0x41c,
    RS_CLAIM_TOWN = 0x41d,
#endif
    RS_CLAIM_GENERATOR = 0x41e,
    RS_CLAIM_GARRISON = 0x41f,
    RS_CLAIM_SHIPYARD = 0x420,
    RS_BUILD_BOAT = 0x421,
    // GATED, and it has to be. An ENUMERATOR on an existing enum is
    // usually free - two of them went onto winmgr.h's EDialogReturnType
    // across 41 consumers without moving a byte - but this one is not:
    // ungated it takes recruit.obj's recruitUnit::Update 90.84 -> 88.24,
    // the include-set sensitivity class reaching a TU that never mentions
    // the value. Measured both ways 2026-08-14.
    RS_ERASE_OBJECT = 0x422,
    // GATED for exactly the reason RS_ERASE_OBJECT is, and measured the
    // same way. The VALUE is fixed by hero::Deallocate (0x4d9ec0), whose
    // inlined CMCDeadHero constructor stores 0x423 as the record subtype -
    // the rung directly below RS_TELEPORT_HERO.
    RS_DEAD_HERO = 0x423,
    RS_TELEPORT_HERO = 0x424,
    // Dreamcast's gapless ladder names 1050. Complete's MoveHero stamps
    // the same 0x41a subtype into CMCMoveHero before SendMapChange.
    RS_MOVE_HERO = 0x41a,
    RS_HIDE_HERO = 0x426,
    // The adventure dispatcher's case roster, named from the gapless DC
    // eRS_Messages ladder (values 1000..1078 transfer whole; see the note
    // at the top of this enum). Gated to advmgr's view - an ungated
    // enumerator here is a measured include-set trigger (RS_ERASE_OBJECT).
    RS_GAME_TRANSMIT_INIT = 1000,
#ifdef HOMM3_GAME_TRANSMIT_DECLS
    RS_GAME_TRANSMIT_MAIN = 1001,
    RS_GAME_TRANSMIT_REQ = 1002,
    RS_GAME_TRANSMIT_END = 1003,
    RS_DESTROY_PLAYER = 1079,
    RS_GAME_TRANSMIT_ACK = 1080,
    RS_GAME_XFER_CONFIRM_END = 1081,
#endif
    RS_CHAT_MSG = 1004,
    RS_COMBAT_INIT = 1005,
    RS_PLAYER_DROPPED = 1014,
    RS_TURN_UPDATE = 1016,
    RS_PLAYER_DROP_UPDATE = 1017,
    RS_PLAYER_DEAD = 1018,
    RS_PLAYER_WON = 1019,
    RS_PLAYER_LOST = 1020,
#ifdef HOMM3_SINGLESELECTION_LOBBY_MESSAGES
    // The Dreamcast eRS_Messages ladder consumed by the setup/lobby TU.
    // Keep these on that TU's measured include view; Complete adds the final
    // three transfer-control rungs after DC's 1081 endpoint.
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
#endif
    RS_MAP_CHANGE_START = 1049,
    RS_MAP_CHANGE_END = 1063,
    RS_TRADE_REQUEST = 1064,
    RS_TRADE_REQUEST_DONE = 1065,
    RS_HERO_UPDATE = 1066,
    RS_GIVE_ME_STUFF = 1070,
    RS_PLAYER_ACTIVE = 1071,
#ifdef HOMM3_REMOTE_SEND_CHAT_DECLS
    // SendChat's ping command constructs the next two rungs directly.
    // Kept in remote.cpp's netmsg view because this enum is a measured
    // include-set/codegen trigger for unrelated consumers.
    RS_PING = 1072,
#endif
#ifdef HOMM3_REMOTE_WAIT_READY_DECLS
    // The two consecutive ready-handshake records constructed by
    // CWaitForReadyPlayersDlg.  DC's gapless roster names the rungs;
    // retail stores 0x3f4/0x3f5 in their 20-byte base-only messages.
    RS_HERO_LEVEL_UPDATE = 1011,
    RS_READY_TO_PLAY = 1012,
    RS_ALL_READY_TO_PLAY = 1013,
    RS_SET_AS_HOST = 1015,
    // The lobby keepalive pair (DC rungs verbatim); singleselectionwindow's
    // OnPingMsg builds the response as a CPingMsg, whose ctor takes this
    // enum - same scoped view, same reason.
    RS_SETUP_PING = 1047,
    RS_SETUP_PING_RESPONSE = 1048,
#endif
    RS_GIFT = 1074,
    RS_GIFT_REQUEST = 1075,
    RS_SESSION_LOST = 1076,
    RS_NORMAL_WIN = 1078
};

#ifdef HOMM3_GAME_TRANSMIT_DECLS
// Network transfer wire limits shared by the sender and receiver.  Retail's
// sender allocates 0x400 bytes per main message; the fixed 0x1c-byte header
// leaves 996 payload bytes, and its idle watchdog compares against 30000 ms.
enum EGameTransmitLimits {
    GAME_TRANSMIT_MESSAGE_SIZE = 1024,
    GAME_TRANSMIT_PAYLOAD_SIZE = 996,
    GAME_TRANSMIT_TIMEOUT = 30000
};
#endif

class CNetMsg {
public:
    int field_00;
    int field_04;
    int subType;
    unsigned long size;
    int field_10;

    CNetMsg() {}
    // Raw Dreamcast CodeView names these parameters `subType` and `size` and
    // types the first as eRS_Messages (0x2CCD), the same type rendered on the
    // CMapChange `id` parameter. Keep the five body statements in lines
    // 169/172-175 order; member-shadow spelling is source material here.
    CNetMsg(eRS_Messages subType, unsigned long size)
    {
        this->subType = subType;
        field_00 = -1;
        this->size = size;
        field_04 = 0;
        field_10 = 0;
    }
};

#ifdef HOMM3_COMMAND_GRID_VIEW
// DC netmsg.h:758 names the base-only message; retail ResetRound expands
// this constructor in place and proves both the store order and 0x14 extent.
class CEndPlacementPhaseMsg : public CNetMsg {
public:
    CEndPlacementPhaseMsg()
        : CNetMsg(RS_END_PLACEMENT_PHASE,
                  sizeof(CEndPlacementPhaseMsg)) {}
};
SIZE(CEndPlacementPhaseMsg, 0x14);
#endif

#if defined(HOMM3_REMOTE_WAIT_READY_DECLS) \
        || defined(HOMM3_COMMAND_GRID_VIEW) \
        || defined(HOMM3_GAME_TRANSMIT_DECLS)
// The header-inline owner delegates to the process-wide recycler. remote.cpp
// can see and inline that wrapper down to delete; command.cpp retains the
// retail out-of-line DestroyMsg call.
void DestroyMsg(CNetMsg* pNetMsg);

#ifdef HOMM3_REMOTE_WAIT_READY_DECLS
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
        subType = RS_ALL_READY_TO_PLAY;
        field_00 = -1;
        size = sizeof(CAllReadyToPlayMsg);
        field_04 = 0;
        field_10 = 0;
    }
};
SIZE(CAllReadyToPlayMsg, 0x14);
#endif

// remote.h:537 in DC. This four-byte owner exists solely to release a
// dequeued message on every return arm of the owning dispatcher.
// Its ctor and dtor are header inline in the original and retail expands
// both into their command/remote callers.
class CMessageKill {
public:
    CMessageKill(CNetMsg* pNetMsg) : m_pNetMsg(pNetMsg) {}
    ~CMessageKill()
    {
        if (m_pNetMsg)
            DestroyMsg(m_pNetMsg);
    }

    void SetMessage(CNetMsg* pNetMsg) { m_pNetMsg = pNetMsg; }

private:
    CNetMsg* m_pNetMsg;
};
SIZE(CMessageKill, 0x4);
#endif

#ifdef HOMM3_COMMAND_GRID_VIEW
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
        subType = RS_COMBAT_MAIN;
        field_00 = -1;
        field_04 = 0;
        field_10 = 0;
        size = sizeof(CCombatMainMsg);
        m_nextAction = nextAction;
        m_nextActionExtra = nextActionExtra;
        m_nextActionGridIndex = nextActionGridIndex;
        m_nextActionGridIndex2 = nextActionGridIndex2;
        m_seed = seed;
    }
};
SIZE(CCombatMainMsg, 0x28);
#endif

#ifdef HOMM3_REMOTE_WAIT_READY_DECLS
// DC netmsg.h:488 supplies the class and all four payload names. Retail's
// CLevelPickWaitDlg dispatcher independently proves the 0x3c-byte wire
// extent and every PC offset while copying the two skill bands into a hero.
class CHeroLevelUpdateMsg : public CNetMsg {
public:
    // DC netmsg.h:488 header inline (dc 0x9cb78, attributed to
    // events.obj); DoCombat expands it in place. Body in events.cpp.
    CHeroLevelUpdateMsg(int hero, int numSSs, signed char* ssLevel,
                        signed char* stats);

    int m_hero;                    // +0x14
    signed char m_ssLevel[28];     // +0x18
    signed char m_stats[4];        // +0x34
    int m_numSSs;                  // +0x38
};
SIZE(CHeroLevelUpdateMsg, 0x3c);
#endif

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
    unsigned long m_fullGameCRC;
    unsigned long m_thisPlayerDead;
    unsigned char m_isDiff;
    unsigned char m_makeOrig;

#ifdef HOMM3_GAME_TRANSMIT_DECLS
    CGameTransmitInitMsg(unsigned long fileSize,
                         unsigned long fullGameCRC,
                         unsigned long thisPlayerDead,
                         unsigned char isDiff,
                         unsigned char makeOrig)
        : CNetMsg(RS_GAME_TRANSMIT_INIT, sizeof(CGameTransmitInitMsg)),
          m_fileSize(fileSize),
          m_fullGameCRC(fullGameCRC),
          m_thisPlayerDead(thisPlayerDead),
          m_isDiff(isDiff),
          m_makeOrig(makeOrig)
    {
    }
#endif
};
SIZE(CGameTransmitInitMsg, 0x24);

#ifdef HOMM3_GAME_TRANSMIT_DECLS
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

    static CGameTransmitMainMsg* CreateMsg(unsigned long maxSize)
    {
        unsigned long size = sizeof(CGameTransmitMainMsg) + maxSize;
        CGameTransmitMainMsg* msg = static_cast<CGameTransmitMainMsg*>(
            ::operator new(size));
        memset(msg, 0, size);
        msg->subType = RS_GAME_TRANSMIT_MAIN;
        return msg;
    }

    void Update(unsigned char* data, unsigned long blockSize)
    {
        m_blockSize = blockSize;
        memcpy(GetData(), data, blockSize);
        size = GetSize();
    }

    unsigned long GetSize() { return m_blockSize + sizeof(*this); }
    unsigned char* GetData()
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
    int m_iMonthType;
    int m_iMonthTypeExtra;
    int m_iWeekType;
    int m_iWeekTypeExtra;
    unsigned long m_diffSize;

    CGameTransmitEndMsg(int iMonthType, int iMonthTypeExtra,
                        int iWeekType, int iWeekTypeExtra,
                        unsigned long diffSize)
        : CNetMsg(RS_GAME_TRANSMIT_END, sizeof(CGameTransmitEndMsg)),
          m_iMonthType(iMonthType),
          m_iMonthTypeExtra(iMonthTypeExtra),
          m_iWeekType(iWeekType),
          m_iWeekTypeExtra(iWeekTypeExtra),
          m_diffSize(diffSize)
    {
    }
};
SIZE(CGameTransmitEndMsg, 0x28);

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
#endif

class CChatMsg : public CNetMsg {
public:
#ifdef HOMM3_REMOTE_SEND_CHAT_DECLS
    char m_text[128];

    CChatMsg(const char* text)
        : CNetMsg(RS_CHAT_MSG, 0)
    {
        strncpy(m_text, text, 127);
        size = GetSize();
    }

    unsigned long GetSize()
    {
        return strlen(m_text) + sizeof(CNetMsg) + 1;
    }
#else
    char m_text[1];  // flexible tail; the wire extent is GetSize's, only
                     // the address is consumed here
#endif
};

#ifdef HOMM3_REMOTE_SEND_CHAT_DECLS
// netmsg.h:804 in the Dreamcast roster. Retail SendChat independently
// proves the one-dword payload, 0x18-byte extent and constructor store order.
class CPingMsg : public CNetMsg {
public:
    unsigned long m_pingTime;

    CPingMsg(unsigned long pingTime, eRS_Messages id)
        : CNetMsg(id, sizeof(CPingMsg)), m_pingTime(pingTime) {}
};
SIZE(CPingMsg, 0x18);
#endif

class CPlayerDroppedMsg : public CNetMsg {
public:
    int m_gamePos;
};

#if defined(HOMM3_REMOTE_CDPLAYHEROES_LAYOUT) \
        || defined(HOMM3_COMMAND_GRID_VIEW)
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
        field_04 = dpid;
    }
};
SIZE(CPlayerDropMsg, 0x18);
#endif

class CPlayerDropUpdateMsg : public CNetMsg {
public:
    unsigned long m_dpidDropped;

#ifdef HOMM3_REMOTE_WAIT_READY_DECLS
    // DC netmsg.h:461 supplies the constructor and payload name. Retail's
    // two inlined HandleNewHost copies independently prove the 0x18-byte
    // extent, RS_PLAYER_DROP_UPDATE subtype, and final payload store.
    CPlayerDropUpdateMsg(unsigned long dpidDropped)
        : CNetMsg(RS_PLAYER_DROP_UPDATE, sizeof(CPlayerDropUpdateMsg)),
          m_dpidDropped(dpidDropped) {}
#endif
};
SIZE(CPlayerDropUpdateMsg, 0x18);

class CPlayerDeadMsg : public CNetMsg {
public:
    int m_gamePos;
};


class CTradeRequestMsg : public CNetMsg {
public:
    int playerPos;
    int resource;
    int amount;
};
SIZE(CTradeRequestMsg, 0x20);

// Dreamcast CodeView names this one-dword CNetMsg derivative and its
// `m_quick` member. Retail DoModal independently proves the 0x18-byte
// extent, RS_COMBAT_TYPE subtype and member at +0x14.
class CCombatTypeMsg : public CNetMsg {
public:
    int m_quick;

    CCombatTypeMsg(int quick)
    {
        m_quick = quick;
        subType = RS_COMBAT_TYPE;
        field_00 = -1;
        size = sizeof(CCombatTypeMsg);
        field_04 = 0;
        field_10 = 0;
    }
};
SIZE(CCombatTypeMsg, 0x18);

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

#ifdef HOMM3_EVENT_RECORD_NETMSG_DECLS
// Dreamcast CodeView names both classes and both constructors
// (netmsg.h:577 / netmsg.h:591, dc 0x8f2c8 / 0x8f2fc), and the constructors
// are the only bodies retail keeps - expanded into the two recorders. The
// 0x1c extent and the +0x14 / +0x18 member offsets are what those
// expansions store, in the CNetMsg base's own field order.
class CMCClaimMine : public CMapChange {
public:
    signed char mineId;
    int playerPos;

    CMCClaimMine(signed char id, int player)
        : CMapChange(RS_CLAIM_MINE, sizeof(CMCClaimMine))
    {
        mineId = id;
        playerPos = player;
    }
};
SIZE(CMCClaimMine, 0x1c);

class CMCClaimTown : public CMapChange {
public:
    signed char townId;
    int playerPos;

    CMCClaimTown(signed char id, int player)
        : CMapChange(RS_CLAIM_TOWN, sizeof(CMCClaimTown))
    {
        townId = id;
        playerPos = player;
    }
};
SIZE(CMCClaimTown, 0x1c);
#endif

class CMCClaimGarrison : public CMapChange {
public:
    int garrisonId;
    int playerPos;

#ifdef HOMM3_GAME_GARRISON_HERO_DECLS
    CMCClaimGarrison() {}
    CMCClaimGarrison(int id, int player);
#else
    CMCClaimGarrison(int id, int player)
        : CMapChange(RS_CLAIM_GARRISON, sizeof(CMCClaimGarrison)),
          garrisonId(id), playerPos(player) {}
#endif
};

class CMCClaimGenerator : public CMapChange {
public:
    int generatorId;
    int playerPos;

    CMCClaimGenerator(int id, int player)
        : CMapChange(RS_CLAIM_GENERATOR, sizeof(CMCClaimGenerator)),
          generatorId(id), playerPos(player) {}
};

class CMCClaimShipYard : public CMapChange {
public:
    type_point point;
    int playerPos;

    CMCClaimShipYard(type_point location, int player)
        : CMapChange(RS_CLAIM_SHIPYARD, sizeof(CMCClaimShipYard)),
          point(location), playerPos(player) {}
};

class CMCBuildBoat : public CMapChange {
public:
    type_point point;
    int playerPos;

    CMCBuildBoat(type_point location, int player)
        : CMapChange(RS_BUILD_BOAT, sizeof(CMCBuildBoat)),
          point(location), playerPos(player) {}
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

class CMCTeleportHero : public CMapChange {
public:
    int heroId;
    type_point point;
    int playerPos;

    CMCTeleportHero(int id, type_point location);
};
SIZE(CMCTeleportHero, 0x20);

// DC netmsg.h:675 (dc 0xd5964, a hero.obj COMDAT); retail /Ob2-expands it
// inside hero::Deallocate, whose 0x1c-byte frame record and 0x423 subtype
// fix both the extent and the rung. `heroId` is an INT, not the DC row's
// signed char: retail copies the whole dword out of hero::id.
class CMCDeadHero : public CMapChange {
public:
    int heroId;
    type_point point;

    CMCDeadHero(int id, type_point location);
};
SIZE(CMCDeadHero, 0x1c);

// game.obj opens this on its own narrow gate: playerData::add_garrison_hero
// (0x4b9fc0) broadcasts the same record town::SwapHeroes does.
class CMCHideHero : public CMapChange {
public:
    int heroId;

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
        this->heroId = heroId;
    }
};

#endif  // HOMM3_NETMSG_H
