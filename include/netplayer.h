// netplayer.h - CNetPlayerInfo, the {dpid, name} pair the network layer
// hands to playerData.
//
// The Dreamcast roster declares this class in E:\gamedcs\struct.h (both
// of its constructors are attributed to struct.h:340 and :346), so a
// faithful port would put it in include/struct.h. It does not live
// there for one measured reason: struct.h rides in initialize.cpp's
// include closure (initialize.cpp -> town.h -> armygrp.h -> struct.h),
// and initialize_game_data's score is sensitive to the COUNT of
// user-defined type definitions visible in that TU. Its own domain
// header is the standing remedy (artifact.h / prefs.h / herospec.h
// precedent). Moving it back into struct.h is a supervised call.
//
// Layout is the DC class record (classes.csv: 28 B, 2 members) and it
// transfers to retail unshifted - playerData::AssignNetInfo (0x4ba130)
// reads the dpid as a dword at +0 and strncpy's twenty bytes from +4.
#ifndef HOMM3_NETPLAYER_H
#define HOMM3_NETPLAYER_H

#include <va.h>

class CNetPlayerInfo {
public:
    unsigned long dpid;   // +0x00
    char sName[24];       // +0x04
};
SIZE(CNetPlayerInfo, 28);

#endif /* HOMM3_NETPLAYER_H */
