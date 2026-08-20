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
#ifdef HOMM3_EVENTS_VIEW
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
#ifdef HOMM3_HERO_OBJ_DECLS
    RS_DEAD_HERO = 0x423,
#endif
    RS_TELEPORT_HERO = 0x424,
    RS_HIDE_HERO = 0x426
};

class CNetMsg {
public:
    int field_00;
    int field_04;
    int subType;
    unsigned long size;
    int field_10;

#if defined(HOMM3_TOWN_OBJ_DECLS) \
        || defined(HOMM3_SYSTEMOPTIONSWINDOW_OBJ_DECLS) \
        || defined(HOMM3_HERO_OBJ_DECLS) \
        || defined(HOMM3_GAME_GARRISON_HERO_DECLS)
    CNetMsg() {}
#endif
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
class CTradeRequestMsg : public CNetMsg {
public:
    int playerPos;
    int resource;
    int amount;
};
SIZE(CTradeRequestMsg, 0x20);

#ifdef HOMM3_SYSTEMOPTIONSWINDOW_OBJ_DECLS
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
#endif

class CMapChange : public CNetMsg {
public:
#if defined(HOMM3_TOWN_OBJ_DECLS) || defined(HOMM3_HERO_OBJ_DECLS) \
        || defined(HOMM3_GAME_GARRISON_HERO_DECLS)
    CMapChange() {}
#endif
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
#endif

#ifdef HOMM3_HERO_OBJ_DECLS
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
#endif

// game.obj opens this on its own narrow gate: playerData::add_garrison_hero
// (0x4b9fc0) broadcasts the same record town::SwapHeroes does.
#if defined(HOMM3_TOWN_OBJ_DECLS) || defined(HOMM3_GAME_GARRISON_HERO_DECLS)
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
#endif

#endif  // HOMM3_NETMSG_H
