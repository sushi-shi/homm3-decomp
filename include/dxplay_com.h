#ifndef HOMM3_DXPLAY_COM_H
#define HOMM3_DXPLAY_COM_H
// Private to dxplay.cpp - NOT included by any other TU. Models the DirectPlay
// COM interface so retail's __stdcall virtual dispatch reproduces byte-for-byte.
#include "dxplay.h"

// The enum trampolines' backing records: a 0x100-byte name buffer followed by
// the DPID at +0x100 (0x104 total). AddGroupEnum/AddPlayerEnum new one, strcpy
// the enumerated short name in, and store the id. CDPlayGroup/CDPlayPlayer are
// only forward-declared in the shared header; completed here for this TU.
class CDPlayGroup {
public:
    CDPlayGroup(char* sName, unsigned long dpid)
    {
        strcpy(m_sName, sName);
        m_dpid = dpid;
    }

    char m_sName[0x100];      // +0x00
    unsigned long m_dpid;     // +0x100
};

class CDPlayPlayer {
public:
    CDPlayPlayer(char* sName, unsigned long dpid)
    {
        strcpy(m_sName, sName);
        m_dpid = dpid;
    }

    char m_sName[0x100];      // +0x00
    unsigned long m_dpid;     // +0x100
};

// The address-element records one DirectPlay SP address chunk EnumAddress splits
// out: a 16-byte data-type GUID, an owned copy of the chunk bytes at +0x10 and
// its size at +0x14. AddAddressEnum news one per enumerated chunk; the array's
// inlined teardown frees the buffer, then the element. Completed here for this
// TU (only forward-declared in the shared header).
class CDPlayAddressElement {
public:
    CDPlayAddressElement(const GUID* lpGuid, const void* pData,
        unsigned long dataSize)
    {
        m_guid = *lpGuid;
        m_dataSize = dataSize;
        m_pData = new char[dataSize];
        memcpy(m_pData, pData, m_dataSize);
    }

    ~CDPlayAddressElement()
    {
        delete [] m_pData;
    }

    GUID m_guid;              // +0x00
    char* m_pData;            // +0x10
    unsigned long m_dataSize; // +0x14
};

// DirectPlay HRESULTs the wrapper bodies branch on. The two-call Get* pattern
// probes with a null buffer and expects DPERR_BUFFERTOOSMALL before allocating.
enum EDPlayResult {
    DPERR_BUFFERTOOSMALL = 0x8877001e,
    DPERR_NOMESSAGES = 0x887700be
};

// DirectPlay system-message discriminants. ReceiveSystemMsg reads dwType off the
// leading DPMSG_GENERIC and dispatches to the matching SysMsg* handler.
enum EDPlaySysMsgType {
    DPSYS_CREATEPLAYERORGROUP = 0x03,
    DPSYS_DESTROYPLAYERORGROUP = 0x05,
    DPSYS_ADDPLAYERTOGROUP = 0x07,
    DPSYS_DELETEPLAYERFROMGROUP = 0x21,
    DPSYS_SESSIONLOST = 0x31,
    DPSYS_HOST = 0x101,
    DPSYS_SETPLAYERORGROUPDATA = 0x102,
    DPSYS_SETPLAYERORGROUPNAME = 0x103,
    DPSYS_SETSESSIONDESC = 0x104,
    DPSYS_ADDGROUPTOGROUP = 0x105,
    DPSYS_DELETEGROUPFROMGROUP = 0x106,
    DPSYS_SECUREMESSAGE = 0x107,
    DPSYS_STARTSESSION = 0x108,
    DPSYS_CHAT = 0x109
};

// The common message prefix: every DPMSG_* system message leads with dwType.
struct DPMSG_GENERIC {
    unsigned long dwType;
};

// DirectPlay value structures consumed only by this TU's wrapper bodies. The
// DPCAPS extent (0x28) is fixed by GetCaps's memset; DPCHAT (0xc) by SendChat.
struct DPCAPS {
    unsigned long dwSize;              // +0x00
    unsigned long dwFlags;            // +0x04
    unsigned long dwMaxBufferSize;    // +0x08
    unsigned long dwMaxQueueSize;     // +0x0c
    unsigned long dwMaxPlayers;       // +0x10
    unsigned long dwHundredBaud;      // +0x14
    unsigned long dwLatency;          // +0x18
    unsigned long dwMaxLocalPlayers;  // +0x1c
    unsigned long dwHeaderLength;     // +0x20
    unsigned long dwTimeout;          // +0x24
};
SIZE(DPCAPS, 0x28);

struct DPCHAT {
    unsigned long dwSize;             // +0x00
    unsigned long dwFlags;            // +0x04
    union {
        unsigned short* lpszMessage;  // +0x08
        char* lpszMessageA;
    };
};
SIZE(DPCHAT, 0x0c);

// The lobby application descriptor RegisterApp fills in. dwSize (0x38) is fixed
// by RegisterApp's own store; the executable path at +0x34 is the game's own
// trailing field beyond the stock lobby descriptor.
struct DPAPPLICATIONDESC {
    unsigned long dwSize;              // +0x00
    unsigned long dwFlags;            // +0x04
    char* lpszApplicationNameA;       // +0x08
    GUID guidApplication;             // +0x0c
    char* lpszFilenameA;              // +0x1c
    char* lpszCommandLineA;           // +0x20
    char* lpszPathA;                  // +0x24
    char* lpszCurrentDirectoryA;      // +0x28
    char* lpszDescriptionA;           // +0x2c
    unsigned short* lpszDescriptionW; // +0x30
    char* lpszExecutableA;            // +0x34
};
SIZE(DPAPPLICATIONDESC, 0x38);

// The DirectPlay COM identifiers the object factory and lobby-connect paths
// reference by address. Values read from the retail .rdata GUID pool; only the
// address matters to the emitted code (the reloc immediate is masked). The
// null GUID doubles as the unset-application-guid sentinel HostSession and the
// base ctor compare against.
DATA(0x00643d58) static const GUID s_guidNull =
    { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
DATA(0x00643d78) static const GUID s_dpaidINet =
    { 0xC4A54DA0, 0xE0AF, 0x11CF, { 0x9C, 0x4E, 0x00, 0xA0, 0xC9, 0x05, 0x42, 0x5E } };
DATA(0x00643db8) static const GUID s_clsidDirectPlayLobby =
    { 0x2FE8F810, 0xB2A5, 0x11D0, { 0xA7, 0x87, 0x00, 0x00, 0xF8, 0x03, 0xAB, 0xFC } };
DATA(0x00643dc8) static const GUID s_iidDirectPlayLobby3A =
    { 0x2DB72491, 0x652C, 0x11D1, { 0xA7, 0xA8, 0x00, 0x00, 0xF8, 0x03, 0xAB, 0xFC } };
DATA(0x00643e18) static const GUID s_clsidDirectPlay =
    { 0xD1EB6D20, 0x8923, 0x11D0, { 0x9D, 0x97, 0x00, 0xA0, 0xC9, 0x0A, 0x43, 0xCB } };
DATA(0x00643e28) static const GUID s_iidDirectPlay4A =
    { 0x0AB1C531, 0x4745, 0x11D1, { 0xA7, 0xA1, 0x00, 0x00, 0xF8, 0x03, 0xAB, 0xFC } };

// DirectPlay COM interface, modeled privately for this TU only (m_lpDP is a
// void* in the shared header, static_cast here). Retail dispatches every
// DirectPlay call as a __stdcall virtual with the interface pointer pushed as
// the first stack argument, so this reproduces `mov ecx,[obj] / call [ecx+slot]`
// at the exact vtable byte offsets read from the retail bodies. No object of
// this type is ever constructed here, so no vtable is emitted for it.
// DirectPlay enumeration callback pointer types (FAR PASCAL = __stdcall). The
// wrappers pass their file-scope trampolines here; typing the vtable parameter
// lets the function name convert without a cast.
typedef int (__stdcall* DPENUMPLAYERSCB2)(unsigned long, unsigned long, const DPNAME*, unsigned long, void*);
typedef int (__stdcall* DPENUMSESSIONSCB2)(const DPSESSIONDESC2*, unsigned long*, unsigned long, void*);
typedef int (__stdcall* DPENUMCONNECTIONSCB)(const GUID*, void*, unsigned long, const DPNAME*, unsigned long, void*);
typedef int (__stdcall* DPENUMADDRESSCB)(const GUID*, unsigned long, const void*, void*);

struct IDirectPlay4A {
    virtual long __stdcall QueryInterface(const GUID& riid, void** ppv) = 0;                 // 0x00
    virtual unsigned long __stdcall AddRef() = 0;                                            // 0x04
    virtual unsigned long __stdcall Release() = 0;                                           // 0x08
    virtual long __stdcall AddPlayerToGroup(unsigned long idGroup, unsigned long idPlayer) = 0; // 0x0c
    virtual long __stdcall Close() = 0;                                                      // 0x10
    virtual long __stdcall CreateGroup(unsigned long* lpidGroup, DPNAME* lpGroupName, void* lpData, unsigned long dwDataSize, unsigned long dwFlags) = 0; // 0x14
    virtual long __stdcall CreatePlayer(unsigned long* lpidPlayer, DPNAME* lpPlayerName, void* hEvent, void* lpData, unsigned long dwDataSize, unsigned long dwFlags) = 0; // 0x18
    virtual long __stdcall DeletePlayerFromGroup(unsigned long idGroup, unsigned long idPlayer) = 0; // 0x1c
    virtual long __stdcall DestroyGroup(unsigned long idGroup) = 0;                          // 0x20
    virtual long __stdcall DestroyPlayer(unsigned long idPlayer) = 0;                        // 0x24
    virtual long __stdcall EnumGroupPlayers(unsigned long idGroup, GUID* lpguidInstance, DPENUMPLAYERSCB2 lpEnumCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x28
    virtual long __stdcall EnumGroups(GUID* lpguidInstance, DPENUMPLAYERSCB2 lpEnumCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x2c
    virtual long __stdcall EnumPlayers(GUID* lpguidInstance, DPENUMPLAYERSCB2 lpEnumCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x30
    virtual long __stdcall EnumSessions(DPSESSIONDESC2* lpsd, unsigned long dwTimeout, DPENUMSESSIONSCB2 lpEnumCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x34
    virtual long __stdcall GetCaps(DPCAPS* lpDPCaps, unsigned long dwFlags) = 0;             // 0x38
    virtual long __stdcall GetGroupData(unsigned long idGroup, void* lpData, unsigned long* lpdwDataSize, unsigned long dwFlags) = 0; // 0x3c
    virtual long __stdcall GetGroupName(unsigned long idGroup, void* lpData, unsigned long* lpdwDataSize) = 0; // 0x40
    virtual long __stdcall GetMessageCount(unsigned long idPlayer, unsigned long* lpdwCount) = 0; // 0x44
    virtual long __stdcall GetPlayerAddress(unsigned long idPlayer, void* lpData, unsigned long* lpdwDataSize) = 0; // 0x48
    virtual long __stdcall GetPlayerCaps(unsigned long idPlayer, DPCAPS* lpPlayerCaps, unsigned long dwFlags) = 0; // 0x4c
    virtual long __stdcall GetPlayerData(unsigned long idPlayer, void* lpData, unsigned long* lpdwDataSize, unsigned long dwFlags) = 0; // 0x50
    virtual long __stdcall GetPlayerName(unsigned long idPlayer, void* lpData, unsigned long* lpdwDataSize) = 0; // 0x54
    virtual long __stdcall GetSessionDesc(void* lpData, unsigned long* lpdwDataSize) = 0;    // 0x58
    virtual long __stdcall Initialize(GUID* lpGUID) = 0;                                     // 0x5c
    virtual long __stdcall Open(DPSESSIONDESC2* lpsd, unsigned long dwFlags) = 0;            // 0x60
    virtual long __stdcall Receive(unsigned long* lpidFrom, unsigned long* lpidTo, unsigned long dwFlags, void* lpData, unsigned long* lpdwDataSize) = 0; // 0x64
    virtual long __stdcall Send(unsigned long idFrom, unsigned long idTo, unsigned long dwFlags, void* lpData, unsigned long dwDataSize) = 0; // 0x68
    virtual long __stdcall SetGroupData(unsigned long idGroup, void* lpData, unsigned long dwDataSize, unsigned long dwFlags) = 0; // 0x6c
    virtual long __stdcall SetGroupName(unsigned long idGroup, DPNAME* lpGroupName, unsigned long dwFlags) = 0; // 0x70
    virtual long __stdcall SetPlayerData(unsigned long idPlayer, void* lpData, unsigned long dwDataSize, unsigned long dwFlags) = 0; // 0x74
    virtual long __stdcall SetPlayerName(unsigned long idPlayer, DPNAME* lpPlayerName, unsigned long dwFlags) = 0; // 0x78
    virtual long __stdcall SetSessionDesc(DPSESSIONDESC2* lpsd, unsigned long dwFlags) = 0;  // 0x7c
    virtual long __stdcall AddGroupToGroup(unsigned long idParentGroup, unsigned long idGroup) = 0; // 0x80
    virtual long __stdcall CreateGroupInGroup(unsigned long idParentGroup, unsigned long* lpidGroup, DPNAME* lpGroupName, void* lpData, unsigned long dwDataSize, unsigned long dwFlags) = 0; // 0x84
    virtual long __stdcall DeleteGroupFromGroup(unsigned long idParentGroup, unsigned long idGroup) = 0; // 0x88
    virtual long __stdcall EnumConnections(const GUID* lpguidApplication, DPENUMCONNECTIONSCB lpEnumCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x8c
    virtual long __stdcall EnumGroupsInGroup(unsigned long idGroup, GUID* lpguidInstance, DPENUMPLAYERSCB2 lpEnumCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x90
    virtual long __stdcall GetGroupConnectionSettings(unsigned long dwFlags, unsigned long idGroup, void* lpData, unsigned long* lpdwDataSize) = 0; // 0x94
    virtual long __stdcall InitializeConnection(void* lpConnection, unsigned long dwFlags) = 0; // 0x98
    virtual long __stdcall SecureOpen(const DPSESSIONDESC2* lpsd, unsigned long dwFlags, const void* lpSecurity, const void* lpCredentials) = 0; // 0x9c
    virtual long __stdcall SendChatMessage(unsigned long idFrom, unsigned long idTo, unsigned long dwFlags, void* lpChatMessage) = 0; // 0xa0
    virtual long __stdcall SetGroupConnectionSettings(unsigned long dwFlags, unsigned long idGroup, void* lpConnection) = 0; // 0xa4
    virtual long __stdcall StartSession(unsigned long dwFlags, unsigned long idGroup) = 0;   // 0xa8
    virtual long __stdcall GetGroupFlags(unsigned long idGroup, unsigned long* lpdwFlags) = 0; // 0xac
    virtual long __stdcall GetGroupParent(unsigned long idGroup, unsigned long* lpidParent) = 0; // 0xb0
    virtual long __stdcall GetPlayerAccount(unsigned long idPlayer, unsigned long dwFlags, void* lpData, unsigned long* lpdwDataSize) = 0; // 0xb4
    virtual long __stdcall GetPlayerFlags(unsigned long idPlayer, unsigned long* lpdwFlags) = 0; // 0xb8
    virtual long __stdcall GetGroupOwner(unsigned long idGroup, unsigned long* lpidOwner) = 0; // 0xbc
    virtual long __stdcall SetGroupOwner(unsigned long idGroup, unsigned long idOwner) = 0;  // 0xc0
    virtual long __stdcall SendEx(unsigned long idFrom, unsigned long idTo, unsigned long dwFlags, void* lpData, unsigned long dwDataSize, unsigned long dwPriority, unsigned long dwTimeout, void* lpContext, unsigned long* lpdwMsgID) = 0; // 0xc4
    virtual long __stdcall GetMessageQueue(unsigned long idFrom, unsigned long idTo, unsigned long dwFlags, unsigned long* lpdwNumMsgs, unsigned long* lpdwNumBytes) = 0; // 0xc8
};

// IDirectPlayLobby3A - the lobby object at CDPlayLobby::m_lpLobby (+0x58).
struct IDirectPlayLobby3A {
    virtual long __stdcall QueryInterface(const GUID& riid, void** ppv) = 0;                 // 0x00
    virtual unsigned long __stdcall AddRef() = 0;                                            // 0x04
    virtual unsigned long __stdcall Release() = 0;                                           // 0x08
    virtual long __stdcall Connect(unsigned long dwFlags, void** lplpDP, void* pUnk) = 0;    // 0x0c
    virtual long __stdcall CreateAddress(const GUID& guidSP, const GUID& guidDataType, const void* lpData, unsigned long dwDataSize, void* lpAddress, unsigned long* lpdwAddressSize) = 0; // 0x10
    virtual long __stdcall EnumAddress(DPENUMADDRESSCB lpEnumAddressCallback, const void* lpAddress, unsigned long dwAddressSize, void* lpContext) = 0; // 0x14
    virtual long __stdcall EnumAddressTypes(void* lpCallback, const GUID& guidSP, void* lpContext, unsigned long dwFlags) = 0; // 0x18
    virtual long __stdcall EnumLocalApplications(void* lpCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x1c
    virtual long __stdcall GetConnectionSettings(unsigned long dwAppID, void* lpData, unsigned long* lpdwDataSize) = 0; // 0x20
    virtual long __stdcall ReceiveLobbyMessage(unsigned long dwFlags, unsigned long dwAppID, unsigned long* lpdwMessageFlags, void* lpData, unsigned long* lpdwDataSize) = 0; // 0x24
    virtual long __stdcall RunApplication(unsigned long dwFlags, unsigned long* lpdwAppID, void* lpConn, void* hReceiveEvent) = 0; // 0x28
    virtual long __stdcall SendLobbyMessage(unsigned long dwFlags, unsigned long dwAppID, void* lpData, unsigned long dwDataSize) = 0; // 0x2c
    virtual long __stdcall SetConnectionSettings(unsigned long dwFlags, unsigned long dwAppID, void* lpConn) = 0; // 0x30
    virtual long __stdcall SetLobbyMessageEvent(unsigned long dwFlags, unsigned long dwAppID, void* hReceiveEvent) = 0; // 0x34
    virtual long __stdcall CreateCompoundAddress(const void* lpElements, unsigned long dwElementCount, void* lpAddress, unsigned long* lpdwAddressSize) = 0; // 0x38
    virtual long __stdcall ConnectEx(unsigned long dwFlags, const GUID& riid, void** lplpDP, void* pUnk) = 0; // 0x3c
    virtual long __stdcall RegisterApplication(unsigned long dwFlags, void* lpAppDesc) = 0;  // 0x40
    virtual long __stdcall UnregisterApplication(unsigned long dwFlags, const GUID& guidApplication) = 0; // 0x44
    virtual long __stdcall WaitForConnectionSettings(unsigned long dwFlags) = 0;             // 0x48
};

#endif  /* HOMM3_DXPLAY_COM_H */
