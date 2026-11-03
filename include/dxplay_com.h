#ifndef HOMM3_DXPLAY_COM_H
#define HOMM3_DXPLAY_COM_H

#include "dxplay.h"

// Game-facing system-message domain. SDK owns the wire records and constants.
enum EDPlaySysMsgType {
    DPLAY_SYSMSG_CREATEPLAYERORGROUP = DPSYS_CREATEPLAYERORGROUP,
    DPLAY_SYSMSG_DESTROYPLAYERORGROUP = DPSYS_DESTROYPLAYERORGROUP,
    DPLAY_SYSMSG_ADDPLAYERTOGROUP = DPSYS_ADDPLAYERTOGROUP,
    DPLAY_SYSMSG_DELETEPLAYERFROMGROUP = DPSYS_DELETEPLAYERFROMGROUP,
    DPLAY_SYSMSG_SESSIONLOST = DPSYS_SESSIONLOST,
    DPLAY_SYSMSG_HOST = DPSYS_HOST,
    DPLAY_SYSMSG_SETPLAYERORGROUPDATA = DPSYS_SETPLAYERORGROUPDATA,
    DPLAY_SYSMSG_SETPLAYERORGROUPNAME = DPSYS_SETPLAYERORGROUPNAME,
    DPLAY_SYSMSG_SETSESSIONDESC = DPSYS_SETSESSIONDESC,
    DPLAY_SYSMSG_ADDGROUPTOGROUP = DPSYS_ADDGROUPTOGROUP,
    DPLAY_SYSMSG_DELETEGROUPFROMGROUP = DPSYS_DELETEGROUPFROMGROUP,
    DPLAY_SYSMSG_SECUREMESSAGE = DPSYS_SECUREMESSAGE,
    DPLAY_SYSMSG_STARTSESSION = DPSYS_STARTSESSION,
    DPLAY_SYSMSG_CHAT = DPSYS_CHAT
};

// The DirectPlay COM identifiers the object factory, lobby-connect and
// compound-address paths reference by address. They are Platform SDK data,
// not this TU's: retail links GUID_NULL from UUID.LIB (cguid_i_guid0.obj) and
// the rest from DXGUID.LIB's dxguid.obj COMDATs, in that member's section
// order (`homm3 verify library-code` compares every byte). The spellings are
// the SDK's C ABI names; Dreamcast's dxplay.obj references DPAID_INet and
// DPSPGUID_TCPIP under the same names. The null GUID doubles as the
// unset-application-guid sentinel HostSession and the base ctor compare
// against.
extern "C" {
DATA(0x00643d58) extern const GUID GUID_NULL;
DATA(0x00643d68) extern const GUID DPAID_ComPort;
DATA(0x00643d78) extern const GUID DPAID_INet;
DATA(0x00643d88) extern const GUID DPAID_Modem;
DATA(0x00643d98) extern const GUID DPAID_Phone;
DATA(0x00643da8) extern const GUID DPAID_ServiceProvider;
DATA(0x00643db8) extern const GUID CLSID_DirectPlayLobby;
DATA(0x00643dc8) extern const GUID IID_IDirectPlayLobby3A;
DATA(0x00643dd8) extern const GUID DPSPGUID_MODEM;
DATA(0x00643de8) extern const GUID DPSPGUID_SERIAL;
DATA(0x00643df8) extern const GUID DPSPGUID_TCPIP;
DATA(0x00643e08) extern const GUID DPSPGUID_IPX;
DATA(0x00643e18) extern const GUID CLSID_DirectPlay;
DATA(0x00643e28) extern const GUID IID_IDirectPlay4A;
}

#endif  /* HOMM3_DXPLAY_COM_H */
