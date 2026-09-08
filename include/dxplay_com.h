#ifndef HOMM3_DXPLAY_COM_H
#define HOMM3_DXPLAY_COM_H
// Private to dxplay.cpp - NOT included by any other TU. Models the DirectPlay
// COM interface so retail's __stdcall virtual dispatch reproduces byte-for-byte.
#include "dxplay.h"
#include "dplaycaps.h"


// DirectPlay HRESULT macros used by the wrappers and CDPlay::GetErrorDesc.
// Keep these as preprocessor constants, as they are in the VC6 DPLAY.H:
// importing that header would collide with the hand-modelled DP6 structs
// above, while an enum would add compiler-visible declarators to this TU.
#define HOMM3_MAKE_DPLAY_ERROR(code) (static_cast<long>(0x88770000UL + (code)))
#define DPERR_BUFFERTOOSMALL HOMM3_MAKE_DPLAY_ERROR(30)
#define DPERR_NOMESSAGES HOMM3_MAKE_DPLAY_ERROR(190)
#define DPERR_ALREADYINITIALIZED HOMM3_MAKE_DPLAY_ERROR(5)
#define DPERR_ACCESSDENIED HOMM3_MAKE_DPLAY_ERROR(10)
#define DPERR_ACTIVEPLAYERS HOMM3_MAKE_DPLAY_ERROR(20)
#define DPERR_CANTADDPLAYER HOMM3_MAKE_DPLAY_ERROR(40)
#define DPERR_CANTCREATEGROUP HOMM3_MAKE_DPLAY_ERROR(50)
#define DPERR_CANTCREATEPLAYER HOMM3_MAKE_DPLAY_ERROR(60)
#define DPERR_CANTCREATESESSION HOMM3_MAKE_DPLAY_ERROR(70)
#define DPERR_CAPSNOTAVAILABLEYET HOMM3_MAKE_DPLAY_ERROR(80)
#define DPERR_EXCEPTION HOMM3_MAKE_DPLAY_ERROR(90)
#define DPERR_GENERIC E_FAIL
#define DPERR_INVALIDFLAGS HOMM3_MAKE_DPLAY_ERROR(120)
#define DPERR_INVALIDOBJECT HOMM3_MAKE_DPLAY_ERROR(130)
#define DPERR_INVALIDPARAM E_INVALIDARG
#define DPERR_INVALIDPLAYER HOMM3_MAKE_DPLAY_ERROR(150)
#define DPERR_INVALIDGROUP HOMM3_MAKE_DPLAY_ERROR(155)
#define DPERR_NOCAPS HOMM3_MAKE_DPLAY_ERROR(160)
#define DPERR_NOCONNECTION HOMM3_MAKE_DPLAY_ERROR(170)
#define DPERR_NOMEMORY E_OUTOFMEMORY
#define DPERR_NONAMESERVERFOUND HOMM3_MAKE_DPLAY_ERROR(200)
#define DPERR_NOPLAYERS HOMM3_MAKE_DPLAY_ERROR(210)
#define DPERR_NOSESSIONS HOMM3_MAKE_DPLAY_ERROR(220)
#define DPERR_PENDING E_PENDING
#define DPERR_SENDTOOBIG HOMM3_MAKE_DPLAY_ERROR(230)
#define DPERR_TIMEOUT HOMM3_MAKE_DPLAY_ERROR(240)
#define DPERR_UNAVAILABLE HOMM3_MAKE_DPLAY_ERROR(250)
#define DPERR_UNSUPPORTED E_NOTIMPL
#define DPERR_BUSY HOMM3_MAKE_DPLAY_ERROR(270)
#define DPERR_USERCANCEL HOMM3_MAKE_DPLAY_ERROR(280)
#define DPERR_NOINTERFACE E_NOINTERFACE
#define DPERR_CANNOTCREATESERVER HOMM3_MAKE_DPLAY_ERROR(290)
#define DPERR_PLAYERLOST HOMM3_MAKE_DPLAY_ERROR(300)
#define DPERR_SESSIONLOST HOMM3_MAKE_DPLAY_ERROR(310)
#define DPERR_UNINITIALIZED HOMM3_MAKE_DPLAY_ERROR(320)
#define DPERR_NONEWPLAYERS HOMM3_MAKE_DPLAY_ERROR(330)
#define DPERR_INVALIDPASSWORD HOMM3_MAKE_DPLAY_ERROR(340)
#define DPERR_CONNECTING HOMM3_MAKE_DPLAY_ERROR(350)
#define DPERR_BUFFERTOOLARGE HOMM3_MAKE_DPLAY_ERROR(1000)
#define DPERR_CANTCREATEPROCESS HOMM3_MAKE_DPLAY_ERROR(1010)
#define DPERR_APPNOTSTARTED HOMM3_MAKE_DPLAY_ERROR(1020)
#define DPERR_INVALIDINTERFACE HOMM3_MAKE_DPLAY_ERROR(1030)
#define DPERR_NOSERVICEPROVIDER HOMM3_MAKE_DPLAY_ERROR(1040)
#define DPERR_UNKNOWNAPPLICATION HOMM3_MAKE_DPLAY_ERROR(1050)
#define DPERR_NOTLOBBIED HOMM3_MAKE_DPLAY_ERROR(1070)
#define DPERR_SERVICEPROVIDERLOADED HOMM3_MAKE_DPLAY_ERROR(1080)
#define DPERR_ALREADYREGISTERED HOMM3_MAKE_DPLAY_ERROR(1090)
#define DPERR_NOTREGISTERED HOMM3_MAKE_DPLAY_ERROR(1100)
#define DPERR_AUTHENTICATIONFAILED HOMM3_MAKE_DPLAY_ERROR(2000)
#define DPERR_CANTLOADSSPI HOMM3_MAKE_DPLAY_ERROR(2010)
#define DPERR_ENCRYPTIONFAILED HOMM3_MAKE_DPLAY_ERROR(2020)
#define DPERR_SIGNFAILED HOMM3_MAKE_DPLAY_ERROR(2030)
#define DPERR_CANTLOADSECURITYPACKAGE HOMM3_MAKE_DPLAY_ERROR(2040)
#define DPERR_ENCRYPTIONNOTSUPPORTED HOMM3_MAKE_DPLAY_ERROR(2050)
#define DPERR_CANTLOADCAPI HOMM3_MAKE_DPLAY_ERROR(2060)
#define DPERR_NOTLOGGEDIN HOMM3_MAKE_DPLAY_ERROR(2070)
#define DPERR_LOGONDENIED HOMM3_MAKE_DPLAY_ERROR(2080)

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
    // Before normalization: dwType.
    unsigned long m_type;
};

// DirectPlay value structures consumed only by this TU's wrapper bodies.
// DPCAPS lives in dplaycaps.h because the multiplayer browser consumes it too;
// DPCHAT stays private and its 0xc extent is fixed by SendChat.
struct DPCHAT {
    // Before normalization: dwSize.
    unsigned long m_size;             // +0x00
    // Before normalization: dwFlags.
    unsigned long m_flags;            // +0x04
    union {
        unsigned short* m_message;  // +0x08
        char* m_messageA;
    };
};
SIZE(DPCHAT, 0x0c);

// The lobby application descriptor RegisterApp fills in. dwSize (0x38) is fixed
// by RegisterApp's own store; the executable path at +0x34 is the game's own
// trailing field beyond the stock lobby descriptor.
struct DPAPPLICATIONDESC {
    // Before normalization: dwSize.
    unsigned long m_size;              // +0x00
    // Before normalization: dwFlags.
    unsigned long m_flags;            // +0x04
    // Before normalization: lpszApplicationNameA.
    char* m_applicationNameA;       // +0x08
    // Before normalization: guidApplication.
    GUID m_guidApplication;             // +0x0c
    // Before normalization: lpszFilenameA.
    char* m_filenameA;              // +0x1c
    // Before normalization: lpszCommandLineA.
    char* m_commandLineA;           // +0x20
    // Before normalization: lpszPathA.
    char* m_pathA;                  // +0x24
    // Before normalization: lpszCurrentDirectoryA.
    char* m_currentDirectoryA;      // +0x28
    // Before normalization: lpszDescriptionA.
    char* m_descriptionA;           // +0x2c
    // Before normalization: lpszDescriptionW.
    unsigned short* m_descriptionW; // +0x30
    // Before normalization: lpszExecutableA.
    char* m_executableA;            // +0x34
};
SIZE(DPAPPLICATIONDESC, 0x38);

// The DirectPlay COM identifiers the object factory and lobby-connect paths
// reference by address. Values read from the retail .rdata GUID pool; only the
// address matters to the emitted code (the reloc immediate is masked). The
// null GUID doubles as the unset-application-guid sentinel HostSession and the
// base ctor compare against.
// Before normalization: s_guidNull.
DATA(0x00643d58) static const GUID g_guidNull =
    { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
// Before normalization: s_dpaidINet.
DATA(0x00643d78) static const GUID g_dpaidINet =
    { 0xC4A54DA0, 0xE0AF, 0x11CF, { 0x9C, 0x4E, 0x00, 0xA0, 0xC9, 0x05, 0x42, 0x5E } };
// Before normalization: s_clsidDirectPlayLobby.
DATA(0x00643db8) static const GUID g_clsidDirectPlayLobby =
    { 0x2FE8F810, 0xB2A5, 0x11D0, { 0xA7, 0x87, 0x00, 0x00, 0xF8, 0x03, 0xAB, 0xFC } };
// Before normalization: s_iidDirectPlayLobby3A.
DATA(0x00643dc8) static const GUID g_iidDirectPlayLobby3A =
    { 0x2DB72491, 0x652C, 0x11D1, { 0xA7, 0xA8, 0x00, 0x00, 0xF8, 0x03, 0xAB, 0xFC } };
// Before normalization: s_clsidDirectPlay.
DATA(0x00643e18) static const GUID g_clsidDirectPlay =
    { 0xD1EB6D20, 0x8923, 0x11D0, { 0x9D, 0x97, 0x00, 0xA0, 0xC9, 0x0A, 0x43, 0xCB } };
// Before normalization: s_iidDirectPlay4A.
DATA(0x00643e28) static const GUID g_iidDirectPlay4A =
    { 0x0AB1C531, 0x4745, 0x11D1, { 0xA7, 0xA1, 0x00, 0x00, 0xF8, 0x03, 0xAB, 0xFC } };

// The DPAID address-element data-type tags and the four service-provider GUIDs
// the Create*Connection compound-address builders reference. Values read from the
// same .rdata pool; DATA-claimed so the reloc names pair.
// Before normalization: s_dpaidComPort.
DATA(0x00643d68) static const GUID g_dpaidComPort =
    { 0xF2F0CE00, 0xE0AF, 0x11CF, { 0x9C, 0x4E, 0x00, 0xA0, 0xC9, 0x05, 0x42, 0x5E } };
// Before normalization: s_dpaidModem.
DATA(0x00643d88) static const GUID g_dpaidModem =
    { 0xF6DCC200, 0xA2FE, 0x11D0, { 0x9C, 0x4F, 0x00, 0xA0, 0xC9, 0x05, 0x42, 0x5E } };
// Before normalization: s_dpaidPhone.
DATA(0x00643d98) static const GUID g_dpaidPhone =
    { 0x78EC89A0, 0xE0AF, 0x11CF, { 0x9C, 0x4E, 0x00, 0xA0, 0xC9, 0x05, 0x42, 0x5E } };
// Before normalization: s_dpaidServiceProvider.
DATA(0x00643da8) static const GUID g_dpaidServiceProvider =
    { 0x07D916C0, 0xE0AF, 0x11CF, { 0x9C, 0x4E, 0x00, 0xA0, 0xC9, 0x05, 0x42, 0x5E } };
// Before normalization: s_spModem.
DATA(0x00643dd8) static const GUID g_spModem =
    { 0x44EAA760, 0xCB68, 0x11CF, { 0x9C, 0x4E, 0x00, 0xA0, 0xC9, 0x05, 0x42, 0x5E } };
// Before normalization: s_spSerial.
DATA(0x00643de8) static const GUID g_spSerial =
    { 0x0F1D6860, 0x88D9, 0x11CF, { 0x9C, 0x4E, 0x00, 0xA0, 0xC9, 0x05, 0x42, 0x5E } };
// Before normalization: s_spTCPIP.
DATA(0x00643df8) static const GUID g_spTcpip =
    { 0x36E95EE0, 0x8577, 0x11CF, { 0x96, 0x0C, 0x00, 0x80, 0xC7, 0x53, 0x4E, 0x82 } };
// Before normalization: s_spIPX.
DATA(0x00643e08) static const GUID g_spIpx =
    { 0x685BC400, 0x9D2C, 0x11CF, { 0xA9, 0xCD, 0x00, 0xAA, 0x00, 0x68, 0x86, 0xE3 } };

// The lobby compound-address builder consumes an array of these tag/size/value
// triples; CreateCompoundAddress packs them into an SP address blob.
struct _DPCOMPORTADDRESS;
struct DPCOMPOUNDADDRESSELEMENT {
    // Before normalization: guidDataType.
    GUID m_guidDataType;         // +0x00
    // Before normalization: dwDataSize.
    unsigned long m_dataSize;  // +0x10
    // Before normalization: lpData.
    const void* m_data;        // +0x14
};
SIZE(DPCOMPOUNDADDRESSELEMENT, 0x18);

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
    // Before normalization (locals): lpGroupName, lpData, dwDataSize, dwFlags.
    virtual long __stdcall CreateGroup(unsigned long* lpidGroup, DPNAME* groupName, void* data, unsigned long dataSize, unsigned long flags) = 0; // 0x14
    // Before normalization (locals): lpPlayerName, hEvent, lpData, dwDataSize, dwFlags.
    virtual long __stdcall CreatePlayer(unsigned long* lpidPlayer, DPNAME* playerName, void* event, void* data, unsigned long dataSize, unsigned long flags) = 0; // 0x18
    virtual long __stdcall DeletePlayerFromGroup(unsigned long idGroup, unsigned long idPlayer) = 0; // 0x1c
    virtual long __stdcall DestroyGroup(unsigned long idGroup) = 0;                          // 0x20
    virtual long __stdcall DestroyPlayer(unsigned long idPlayer) = 0;                        // 0x24
    // Before normalization (locals): lpEnumCallback, lpContext, dwFlags.
    virtual long __stdcall EnumGroupPlayers(unsigned long idGroup, GUID* lpguidInstance, DPENUMPLAYERSCB2 enumCallback, void* context, unsigned long flags) = 0; // 0x28
    // Before normalization (locals): lpEnumCallback, lpContext, dwFlags.
    virtual long __stdcall EnumGroups(GUID* lpguidInstance, DPENUMPLAYERSCB2 enumCallback, void* context, unsigned long flags) = 0; // 0x2c
    // Before normalization (locals): lpEnumCallback, lpContext, dwFlags.
    virtual long __stdcall EnumPlayers(GUID* lpguidInstance, DPENUMPLAYERSCB2 enumCallback, void* context, unsigned long flags) = 0; // 0x30
    // Before normalization (locals): dwTimeout, lpEnumCallback, lpContext, dwFlags.
    virtual long __stdcall EnumSessions(DPSESSIONDESC2* lpsd, unsigned long timeout, DPENUMSESSIONSCB2 enumCallback, void* context, unsigned long flags) = 0; // 0x34
    // Before normalization (locals): lpDPCaps, dwFlags.
    virtual long __stdcall GetCaps(DPCAPS* dpCaps, unsigned long flags) = 0;             // 0x38
    // Before normalization (locals): lpData, dwFlags.
    virtual long __stdcall GetGroupData(unsigned long idGroup, void* data, unsigned long* lpdwDataSize, unsigned long flags) = 0; // 0x3c
    // Before normalization (locals): lpData.
    virtual long __stdcall GetGroupName(unsigned long idGroup, void* data, unsigned long* lpdwDataSize) = 0; // 0x40
    virtual long __stdcall GetMessageCount(unsigned long idPlayer, unsigned long* lpdwCount) = 0; // 0x44
    // Before normalization (locals): lpData.
    virtual long __stdcall GetPlayerAddress(unsigned long idPlayer, void* data, unsigned long* lpdwDataSize) = 0; // 0x48
    // Before normalization (locals): lpPlayerCaps, dwFlags.
    virtual long __stdcall GetPlayerCaps(unsigned long idPlayer, DPCAPS* playerCaps, unsigned long flags) = 0; // 0x4c
    // Before normalization (locals): lpData, dwFlags.
    virtual long __stdcall GetPlayerData(unsigned long idPlayer, void* data, unsigned long* lpdwDataSize, unsigned long flags) = 0; // 0x50
    // Before normalization (locals): lpData.
    virtual long __stdcall GetPlayerName(unsigned long idPlayer, void* data, unsigned long* lpdwDataSize) = 0; // 0x54
    // Before normalization (locals): lpData.
    virtual long __stdcall GetSessionDesc(void* data, unsigned long* lpdwDataSize) = 0;    // 0x58
    // Before normalization (locals): lpGUID.
    virtual long __stdcall Initialize(GUID* guid) = 0;                                     // 0x5c
    // Before normalization (locals): dwFlags.
    virtual long __stdcall Open(DPSESSIONDESC2* lpsd, unsigned long flags) = 0;            // 0x60
    // Before normalization (locals): dwFlags, lpData.
    virtual long __stdcall Receive(unsigned long* lpidFrom, unsigned long* lpidTo, unsigned long flags, void* data, unsigned long* lpdwDataSize) = 0; // 0x64
    // Before normalization (locals): dwFlags, lpData, dwDataSize.
    virtual long __stdcall Send(unsigned long idFrom, unsigned long idTo, unsigned long flags, void* data, unsigned long dataSize) = 0; // 0x68
    // Before normalization (locals): lpData, dwDataSize, dwFlags.
    virtual long __stdcall SetGroupData(unsigned long idGroup, void* data, unsigned long dataSize, unsigned long flags) = 0; // 0x6c
    // Before normalization (locals): lpGroupName, dwFlags.
    virtual long __stdcall SetGroupName(unsigned long idGroup, DPNAME* groupName, unsigned long flags) = 0; // 0x70
    // Before normalization (locals): lpData, dwDataSize, dwFlags.
    virtual long __stdcall SetPlayerData(unsigned long idPlayer, void* data, unsigned long dataSize, unsigned long flags) = 0; // 0x74
    // Before normalization (locals): lpPlayerName, dwFlags.
    virtual long __stdcall SetPlayerName(unsigned long idPlayer, DPNAME* playerName, unsigned long flags) = 0; // 0x78
    // Before normalization (locals): dwFlags.
    virtual long __stdcall SetSessionDesc(DPSESSIONDESC2* lpsd, unsigned long flags) = 0;  // 0x7c
    virtual long __stdcall AddGroupToGroup(unsigned long idParentGroup, unsigned long idGroup) = 0; // 0x80
    // Before normalization (locals): lpGroupName, lpData, dwDataSize, dwFlags.
    virtual long __stdcall CreateGroupInGroup(unsigned long idParentGroup, unsigned long* lpidGroup, DPNAME* groupName, void* data, unsigned long dataSize, unsigned long flags) = 0; // 0x84
    virtual long __stdcall DeleteGroupFromGroup(unsigned long idParentGroup, unsigned long idGroup) = 0; // 0x88
    // Before normalization (locals): lpEnumCallback, lpContext, dwFlags.
    virtual long __stdcall EnumConnections(const GUID* lpguidApplication, DPENUMCONNECTIONSCB enumCallback, void* context, unsigned long flags) = 0; // 0x8c
    // Before normalization (locals): lpEnumCallback, lpContext, dwFlags.
    virtual long __stdcall EnumGroupsInGroup(unsigned long idGroup, GUID* lpguidInstance, DPENUMPLAYERSCB2 enumCallback, void* context, unsigned long flags) = 0; // 0x90
    // Before normalization (locals): dwFlags, lpData.
    virtual long __stdcall GetGroupConnectionSettings(unsigned long flags, unsigned long idGroup, void* data, unsigned long* lpdwDataSize) = 0; // 0x94
    // Before normalization (locals): lpConnection, dwFlags.
    virtual long __stdcall InitializeConnection(void* connection, unsigned long flags) = 0; // 0x98
    // Before normalization (locals): dwFlags, lpSecurity, lpCredentials.
    virtual long __stdcall SecureOpen(const DPSESSIONDESC2* lpsd, unsigned long flags, const void* security, const void* credentials) = 0; // 0x9c
    // Before normalization (locals): dwFlags, lpChatMessage.
    virtual long __stdcall SendChatMessage(unsigned long idFrom, unsigned long idTo, unsigned long flags, void* chatMessage) = 0; // 0xa0
    // Before normalization (locals): dwFlags, lpConnection.
    virtual long __stdcall SetGroupConnectionSettings(unsigned long flags, unsigned long idGroup, void* connection) = 0; // 0xa4
    // Before normalization (locals): dwFlags.
    virtual long __stdcall StartSession(unsigned long flags, unsigned long idGroup) = 0;   // 0xa8
    virtual long __stdcall GetGroupFlags(unsigned long idGroup, unsigned long* lpdwFlags) = 0; // 0xac
    virtual long __stdcall GetGroupParent(unsigned long idGroup, unsigned long* lpidParent) = 0; // 0xb0
    // Before normalization (locals): dwFlags, lpData.
    virtual long __stdcall GetPlayerAccount(unsigned long idPlayer, unsigned long flags, void* data, unsigned long* lpdwDataSize) = 0; // 0xb4
    virtual long __stdcall GetPlayerFlags(unsigned long idPlayer, unsigned long* lpdwFlags) = 0; // 0xb8
    virtual long __stdcall GetGroupOwner(unsigned long idGroup, unsigned long* lpidOwner) = 0; // 0xbc
    virtual long __stdcall SetGroupOwner(unsigned long idGroup, unsigned long idOwner) = 0;  // 0xc0
    // Before normalization (locals): dwFlags, lpData, dwDataSize, dwPriority, dwTimeout,
    // lpContext.
    virtual long __stdcall SendEx(unsigned long idFrom, unsigned long idTo, unsigned long flags, void* data, unsigned long dataSize, unsigned long priority, unsigned long timeout, void* context, unsigned long* lpdwMsgID) = 0; // 0xc4
    // Before normalization (locals): dwFlags.
    virtual long __stdcall GetMessageQueue(unsigned long idFrom, unsigned long idTo, unsigned long flags, unsigned long* lpdwNumMsgs, unsigned long* lpdwNumBytes) = 0; // 0xc8
};

// IDirectPlayLobby3A - the lobby object at CDPlayLobby::m_lpLobby (+0x58).
struct IDirectPlayLobby3A {
    virtual long __stdcall QueryInterface(const GUID& riid, void** ppv) = 0;                 // 0x00
    virtual unsigned long __stdcall AddRef() = 0;                                            // 0x04
    virtual unsigned long __stdcall Release() = 0;                                           // 0x08
    // Before normalization (locals): dwFlags, pUnk.
    virtual long __stdcall Connect(unsigned long flags, void** lplpDP, void* unk) = 0;    // 0x0c
    // Before normalization (locals): lpData, dwDataSize, lpAddress.
    virtual long __stdcall CreateAddress(const GUID& guidSP, const GUID& guidDataType, const void* data, unsigned long dataSize, void* address, unsigned long* lpdwAddressSize) = 0; // 0x10
    // Before normalization (locals): lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext.
    virtual long __stdcall EnumAddress(DPENUMADDRESSCB enumAddressCallback, const void* address, unsigned long addressSize, void* context) = 0; // 0x14
    // Before normalization (locals): lpCallback, lpContext, dwFlags.
    virtual long __stdcall EnumAddressTypes(void* callback, const GUID& guidSP, void* context, unsigned long flags) = 0; // 0x18
    // Before normalization (locals): lpCallback, lpContext, dwFlags.
    virtual long __stdcall EnumLocalApplications(void* callback, void* context, unsigned long flags) = 0; // 0x1c
    // Before normalization (locals): dwAppID, lpData.
    virtual long __stdcall GetConnectionSettings(unsigned long appID, void* data, unsigned long* lpdwDataSize) = 0; // 0x20
    // Before normalization (locals): dwFlags, dwAppID, lpData.
    virtual long __stdcall ReceiveLobbyMessage(unsigned long flags, unsigned long appID, unsigned long* lpdwMessageFlags, void* data, unsigned long* lpdwDataSize) = 0; // 0x24
    // Before normalization (locals): dwFlags, lpConn, hReceiveEvent.
    virtual long __stdcall RunApplication(unsigned long flags, unsigned long* lpdwAppID, void* conn, void* receiveEvent) = 0; // 0x28
    // Before normalization (locals): dwFlags, dwAppID, lpData, dwDataSize.
    virtual long __stdcall SendLobbyMessage(unsigned long flags, unsigned long appID, void* data, unsigned long dataSize) = 0; // 0x2c
    // Before normalization (locals): dwFlags, dwAppID, lpConn.
    virtual long __stdcall SetConnectionSettings(unsigned long flags, unsigned long appID, void* conn) = 0; // 0x30
    // Before normalization (locals): dwFlags, dwAppID, hReceiveEvent.
    virtual long __stdcall SetLobbyMessageEvent(unsigned long flags, unsigned long appID, void* receiveEvent) = 0; // 0x34
    // Before normalization (locals): lpElements, dwElementCount, lpAddress.
    virtual long __stdcall CreateCompoundAddress(const void* elements, unsigned long elementCount, void* address, unsigned long* lpdwAddressSize) = 0; // 0x38
    // Before normalization (locals): dwFlags, pUnk.
    virtual long __stdcall ConnectEx(unsigned long flags, const GUID& riid, void** lplpDP, void* unk) = 0; // 0x3c
    // Before normalization (locals): dwFlags, lpAppDesc.
    virtual long __stdcall RegisterApplication(unsigned long flags, void* appDesc) = 0;  // 0x40
    // Before normalization (locals): dwFlags.
    virtual long __stdcall UnregisterApplication(unsigned long flags, const GUID& guidApplication) = 0; // 0x44
    // Before normalization (locals): dwFlags.
    virtual long __stdcall WaitForConnectionSettings(unsigned long flags) = 0;             // 0x48
};

#endif  /* HOMM3_DXPLAY_COM_H */
