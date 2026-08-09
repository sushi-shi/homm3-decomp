// netmsg.h - narrow retail-proven network message layouts
#ifndef HOMM3_NETMSG_H
#define HOMM3_NETMSG_H

#include "struct.h"

enum eRS_Messages {
    RS_CLAIM_GENERATOR = 0x41e,
    RS_CLAIM_GARRISON = 0x41f,
    RS_CLAIM_SHIPYARD = 0x420
};

class CNetMsg {
public:
    int field_00;
    int field_04;
    int subType;
    unsigned long size;
    int field_10;

    CNetMsg(int new_sub_type, unsigned long new_size)
        : field_00(-1), field_04(0), subType(new_sub_type),
          size(new_size), field_10(0) {}
};

class CMapChange : public CNetMsg {
public:
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

#endif  // HOMM3_NETMSG_H
