// netmsg.h - narrow retail-proven network message layouts
#ifndef HOMM3_NETMSG_H
#define HOMM3_NETMSG_H

#include "struct.h"

enum eRS_Messages {
    RS_COMBAT_TYPE = 0x3f1,
    RS_CLAIM_GENERATOR = 0x41e,
    RS_CLAIM_GARRISON = 0x41f,
    RS_CLAIM_SHIPYARD = 0x420,
    RS_BUILD_BOAT = 0x421,
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
        || defined(HOMM3_HERO_OBJ_DECLS)
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
#if defined(HOMM3_TOWN_OBJ_DECLS) || defined(HOMM3_HERO_OBJ_DECLS)
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

#ifdef HOMM3_HERO_OBJ_DECLS
class CMCTeleportHero : public CMapChange {
public:
    int heroId;
    type_point point;
    int playerPos;

    CMCTeleportHero(int id, type_point location);
};
SIZE(CMCTeleportHero, 0x20);
#endif

#ifdef HOMM3_TOWN_OBJ_DECLS
class CMCHideHero : public CMapChange {
public:
    int heroId;

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
