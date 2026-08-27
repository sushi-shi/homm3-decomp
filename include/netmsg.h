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
    RS_HIDE_HERO = 0x426,
    // The adventure dispatcher's case roster, named from the gapless DC
    // eRS_Messages ladder (values 1000..1078 transfer whole; see the note
    // at the top of this enum). Gated to advmgr's view - an ungated
    // enumerator here is a measured include-set trigger (RS_ERASE_OBJECT).
    RS_GAME_TRANSMIT_INIT = 1000,
    RS_CHAT_MSG = 1004,
    RS_COMBAT_INIT = 1005,
    RS_PLAYER_DROPPED = 1014,
    RS_TURN_UPDATE = 1016,
    RS_PLAYER_DROP_UPDATE = 1017,
    RS_PLAYER_DEAD = 1018,
    RS_PLAYER_WON = 1019,
    RS_PLAYER_LOST = 1020,
    RS_MAP_CHANGE_START = 1049,
    RS_MAP_CHANGE_END = 1063,
    RS_TRADE_REQUEST = 1064,
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
#endif
    RS_GIFT = 1074,
    RS_GIFT_REQUEST = 1075,
    RS_SESSION_LOST = 1076,
    RS_NORMAL_WIN = 1078
};

class CNetMsg {
public:
    int field_00;
    int field_04;
    int subType;
    unsigned long size;
    int field_10;

    CNetMsg() {}
    CNetMsg(int new_sub_type, unsigned long new_size)
    {
        subType = new_sub_type;
        field_00 = -1;
        size = new_size;
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

// remote.h:537 in DC. This four-byte owner exists solely to release a
// dequeued message on every return arm of the ready-dialog dispatcher.
// Its ctor and dtor are header inline in the original and retail expands
// both into CWaitForReadyPlayersDlg::handle_message.
class CMessageKill {
public:
    CMessageKill(CNetMsg* pNetMsg) : m_pNetMsg(pNetMsg) {}
    ~CMessageKill()
    {
        if (m_pNetMsg)
            delete m_pNetMsg;
    }

    void SetMessage(CNetMsg* pNetMsg) { m_pNetMsg = pNetMsg; }

private:
    CNetMsg* m_pNetMsg;
};
SIZE(CMessageKill, 0x4);
#endif

#ifdef HOMM3_REMOTE_WAIT_READY_DECLS
// DC netmsg.h:488 supplies the class and all four payload names. Retail's
// CLevelPickWaitDlg dispatcher independently proves the 0x3c-byte wire
// extent and every PC offset while copying the two skill bands into a hero.
class CHeroLevelUpdateMsg : public CNetMsg {
public:
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
};

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

#ifdef HOMM3_REMOTE_CDPLAYHEROES_LAYOUT
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
    CMapChange(eRS_Messages id, unsigned long messageSize)
        : CNetMsg(id, messageSize) {}
};

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

    // MEMBER FIRST, and the two call sites CONTRADICT each other on it -
    // A/B'd both ways 2026-08-20 with both bodies compiled:
    //   this spelling      -> SwapHeroes 100.00, add_garrison_hero 98.99
    //   base-first, i.e.
    //   `: CMapChange(RS_HIDE_HERO, sizeof(CMCHideHero)) { heroId = id; }`
    //   (and the two equivalent init-list / body-last forms, all three
    //   measured identical) -> add_garrison_hero 100.00, SwapHeroes 97.77
    // In retail the two differ only in WHICH REGISTER holds the id:
    // SwapHeroes has it in ECX, so the store is forced ahead of the
    // `lea ecx,[ebp-0x20]` that sets up the call, while
    // add_garrison_hero has it in EAX and sinks the store past all five
    // base stores. One emission is scheduling luck either way; no source
    // spelling reproduces both. Kept here because SwapHeroes is EXACT
    // with it and the alternative only trades that exact row for this
    // one (and regresses town.obj below its recorded max).
    CMCHideHero(int id)
        : CMapChange()
    {
        heroId = id;
        subType = RS_HIDE_HERO;
        field_00 = -1;
        size = sizeof(CMCHideHero);
        field_04 = 0;
        field_10 = 0;
    }
};

#endif  // HOMM3_NETMSG_H
