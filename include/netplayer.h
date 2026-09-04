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
// precedent). Moving it back into struct.h is a deliberate tree-wide call.
//
// DC's class record ends after the name at 28 bytes.  Retail keeps those
// offsets (playerData::AssignNetInfo at 0x4ba130 reads dpid at +0 and the
// name at +4) but the independently identified CNetPlayerHandlerPlayer
// constructor at 0x57c790 initializes an additional version dword at +0x1c
// before the derived fields begin at +0x20.  This is the complete Windows
// base layout, not a view local to the selection window.
#ifndef HOMM3_NETPLAYER_H
#define HOMM3_NETPLAYER_H

#include <va.h>

class CNetPlayerInfo {
public:
    unsigned long dpid;   // +0x00
    char sName[24];       // +0x04
    int version;          // +0x1c (retail-only extension)

    // E:\gamedcs\struct.h:340. The Dreamcast body initializes the two
    // shared fields; Complete's added version member belongs to the same
    // base boundary in the retail selection-window TU.
    CNetPlayerInfo();
    CNetPlayerInfo(char* _sName, unsigned long _dpid);
};
SIZE(CNetPlayerInfo, 32);

// Network-layer singleton defined by remote.cpp. Retail's multiplayer host
// path writes the DPID, name, and version fields through this complete view.
extern CNetPlayerInfo gsThisNetPlayerInfo;

#endif /* HOMM3_NETPLAYER_H */
