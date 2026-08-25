#ifndef HOMM3_DXPLAY_COM_H
#define HOMM3_DXPLAY_COM_H
// Private to dxplay.cpp - NOT included by any other TU. Models the DirectPlay
// COM interface so retail's __stdcall virtual dispatch reproduces byte-for-byte.
#include "dxplay.h"

// DirectPlay HRESULTs the wrapper bodies branch on. The two-call Get* pattern
// probes with a null buffer and expects DPERR_BUFFERTOOSMALL before allocating.
enum EDPlayResult {
    DPERR_BUFFERTOOSMALL = 0x8877001e
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

// DirectPlay COM interface, modeled privately for this TU only (m_lpDP is a
// void* in the shared header, static_cast here). Retail dispatches every
// DirectPlay call as a __stdcall virtual with the interface pointer pushed as
// the first stack argument, so this reproduces `mov ecx,[obj] / call [ecx+slot]`
// at the exact vtable byte offsets read from the retail bodies. No object of
// this type is ever constructed here, so no vtable is emitted for it.
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
    virtual long __stdcall EnumGroupPlayers(unsigned long idGroup, GUID* lpguidInstance, void* lpEnumCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x28
    virtual long __stdcall EnumGroups(GUID* lpguidInstance, void* lpEnumCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x2c
    virtual long __stdcall EnumPlayers(GUID* lpguidInstance, void* lpEnumCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x30
    virtual long __stdcall EnumSessions(DPSESSIONDESC2* lpsd, unsigned long dwTimeout, void* lpEnumCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x34
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
    virtual long __stdcall EnumConnections(const GUID* lpguidApplication, void* lpEnumCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x8c
    virtual long __stdcall EnumGroupsInGroup(unsigned long idGroup, GUID* lpguidInstance, void* lpEnumCallback, void* lpContext, unsigned long dwFlags) = 0; // 0x90
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
    virtual long __stdcall EnumAddress(void* lpEnumAddressCallback, const void* lpAddress, unsigned long dwAddressSize, void* lpContext) = 0; // 0x14
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
