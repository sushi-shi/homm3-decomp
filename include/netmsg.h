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
    RS_COMBAT_TYPE = 0x3f1,
    // GATED for exactly the reason RS_ERASE_OBJECT below is - see that
    // note. DC eRS_Messages has RS_SET_VISIBILITY = 1021, and retail's two
    // monolith handlers stamp 0x3fd into the message they transmit.
#if defined(HOMM3_EVENTS_VIEW) || defined(HOMM3_ADVMGR_TURN_DECLS)
    RS_SET_VISIBILITY = 0x3fd,
    // The next rung, and the DC ladder is gapless: 1022. Retail's
    // DoEventCoverOfDarkness (0x4a14b0) stamps 0x3fe into a message whose
    // layout is CSetVisibilityMsg's member for member, which is exactly the
    // pairing the two names describe. Gated for the same reason its
    // neighbours are.
    RS_RESET_VISIBILITY = 0x3fe,
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
#ifdef HOMM3_EVENTS_VIEW
    RS_ERASE_OBJECT = 0x422,
#endif
    // GATED for exactly the reason RS_ERASE_OBJECT is, and measured the
    // same way. The VALUE is fixed by hero::Deallocate (0x4d9ec0), whose
    // inlined CMCDeadHero constructor stores 0x423 as the record subtype -
    // the rung directly below RS_TELEPORT_HERO.
    RS_DEAD_HERO = 0x423,
    RS_TELEPORT_HERO = 0x424,
    RS_HIDE_HERO = 0x426,
#ifdef HOMM3_ADVMGR_TURN_DECLS
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
    RS_GIFT = 1074,
    RS_GIFT_REQUEST = 1075,
    RS_SESSION_LOST = 1076,
    RS_NORMAL_WIN = 1078
#endif
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

// Complete retail's resource-trade notification is a compact CNetMsg with
// three dwords at +0x14. HandleTradeRequestMsg proves their player/resource/
// amount roles. The Dreamcast port's same-named class instead embeds two hero
// snapshots, a protocol-level platform divergence.
#ifdef HOMM3_ADVMGR_TURN_DECLS
// The dispatcher-facing message views advmgr's HandleNetMsg reads. Field
// offsets are the handler's own byte-proven reads; the class names come
// from the DC ctor publics (CGameTransmitInitMsg KKK_N_N, CChatMsg PBD,
// CCombatTypeMsg H). Payload member names are ordinal where the DC member
// roster has not been consulted. Gated to advmgr's view.
class CGameTransmitInitMsg : public CNetMsg {
public:
    unsigned long m_field_14;
    unsigned long m_field_18;
    unsigned long m_field_1c;
    unsigned char m_field_20;
    unsigned char m_field_21;
};

class CChatMsg : public CNetMsg {
public:
    char m_text[1];  // flexible tail; the wire extent is GetSize's, only
                     // the address is consumed here
};

class CPlayerDroppedMsg : public CNetMsg {
public:
    int m_gamePos;
};

class CPlayerDropUpdateMsg : public CNetMsg {
public:
    int m_gamePos;
};

class CPlayerDeadMsg : public CNetMsg {
public:
    int m_gamePos;
};

#endif

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

    CMCClaimGarrison(int id, int player)
        : CMapChange(RS_CLAIM_GARRISON, sizeof(CMCClaimGarrison)),
          garrisonId(id), playerPos(player) {}
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

#ifdef HOMM3_EVENTS_VIEW
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

#endif
// advmgr.obj joins the gate for CSetVisibilityMsg alone (its two
// visibility dispatch arms read it); split guard, CMCEraseObject and the
// reset twin stay events-view-only.
#if defined(HOMM3_EVENTS_VIEW) || defined(HOMM3_ADVMGR_TURN_DECLS)
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
#endif
#ifdef HOMM3_EVENTS_VIEW

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
#endif

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
