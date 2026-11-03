#ifndef HOMM3_DXPLAY_H
#define HOMM3_DXPLAY_H

#include "va.h"

#include <string.h>
#include "platform.h"

#include "array.h"

// Complete uses the SDK ANSI aliases and their inherited COM interfaces.
#include <dplay.h>
#include <dplobby.h>

SIZE(DPNAME, 0x10);
SIZE(DPSESSIONDESC2, 0x50);
SIZE(DPLCONNECTION, 0x28);
SIZE(DPMSG_CREATEPLAYERORGROUP, 0x30);
SIZE(DPAPPLICATIONDESC2, 0x38);

class CDPlayConnection;
class CDPlayAddressElement;
class CDPlayGroup;
class CDPlayMsg;

class CDPlaySession;
template<class T> class CAutoArray;
// Game-facing player-kind domain, using the SDK discriminants.
enum EDPlayerType {
    DPLAY_PLAYER_TYPE_GROUP = DPPLAYERTYPE_GROUP,
    DPLAY_PLAYER_TYPE_PLAYER = DPPLAYERTYPE_PLAYER
};

enum EDPlayConnectionFlags {
    DPLAY_CONNECTION_CREATE_SESSION = 0x2
};

enum EDPlaySessionFlags {
    DPLAY_SESSION_MIGRATE_HOST = 0x4,
    DPLAY_SESSION_KEEP_ALIVE = 0x40
};

// Game-facing send errors retain their SDK HRESULT values.
enum EDPlaySendError {
    DPLAY_SEND_ERROR_INVALID_PARAMETER = 0x80070057,
    DPLAY_SEND_ERROR_INVALID_PLAYER = 0x88770096
};

// The receive path's own SDK immediate, the one HRESULT its drain loop
// treats as success: MAKE_DPHRESULT(190). remote.obj's PollRemote compares
// m_hRes against it before deciding the session is broken.
enum EDPlayReceiveError {
    DPLAY_RECEIVE_ERROR_NO_MESSAGES = 0x887700be
};

// The Windows structure consumed here preserves the complete Dreamcast
// value layout (E:\gamedcs\dxplay.h:57 for the constructor, :96 for
// IsPasswordProtected).  HandleMPlayerLaunch's inlined Get(0) reaches
// guidInstance at +4, which is the retail proof needed by that TU, and the
// three predicates are the multiplayer window's - IsJoinDisabled has its own
// retail row at 0x5112c0.  This class was carried a SECOND time in
// multiplayerwindow.h behind a per-TU macro; the two copies had the same
// layout and disjoint member sets, and this is the union.
class CDPlaySession {
public:
    unsigned long m_flags;  // +0x00
    GUID m_guidInstance;  // +0x04
    GUID m_guidApp;  // +0x14
    unsigned long m_maxPlayers;  // +0x24
    unsigned long m_playerCount;  // +0x28
    char m_sessionName[128];  // +0x2c
    char m_password[80];  // +0xac
    unsigned long m_user1;  // +0xfc
    unsigned long m_user2;  // +0x100
    unsigned long m_user3;  // +0x104
    unsigned long m_user4;  // +0x108

    // E:\gamedcs\dxplay.h:57
    DC_ADDRESS(0x08bca0, 0x8c)
    CDPlaySession(const DPSESSIONDESC2* session)
    {
        if (session) {
            m_flags = session->dwFlags;
            m_guidInstance = session->guidInstance;
            m_guidApp = session->guidApplication;
            m_maxPlayers = session->dwMaxPlayers;
            m_playerCount = session->dwCurrentPlayers;
            m_user1 = session->dwUser1;
            m_user2 = session->dwUser2;
            m_user3 = session->dwUser3;
            m_user4 = session->dwUser4;
            strcpy(m_sessionName, session->lpszSessionNameA);
            if (session->lpszPasswordA)
                strcpy(m_password, session->lpszPasswordA);
            else
                m_password[0] = 0;
        }
    }

    // DC dxplay.h:90 retains the third early success arm.
    VA(0x005112c0, 0x1C)  // exact selected COMDAT
    DC_ADDRESS(0x101d58, 0x2a)
    bool IsJoinDisabled()
    {
        if (m_flags & 0x20)
            return 1;
        if (m_flags & 1)
            return 1;
        if (m_playerCount == m_maxPlayers)
            return 1;
        return 0;
    }

    DC_ADDRESS(0x101d84, 0x14)
    bool IsPasswordProtected()
    {
        if (m_flags & 0x400)
            return 1;
        return 0;
    }
};
SIZE(CDPlaySession, 0x10c);

// Dreamcast publishes the complete 0x98-byte value layout. Retail's two
// inlined delete paths in remote::InitConnection independently prove the
// owned connection buffer at +0x10 and the trivial non-virtual destructor.
class CDPlayConnection {
public:
    GUID m_guidSp;  // +0x00
    unsigned char* m_connection;  // +0x10
    char m_name[128];  // +0x14
    unsigned long m_size;  // +0x94

    DC_ADDRESS(0x08bd2c, 0x64)
    CDPlayConnection(const GUID* guid, unsigned long connSize, void* conn,
        char* name)
    {
        m_guidSp = *guid;
        m_size = connSize;
        m_connection = new unsigned char[connSize];
        memcpy(m_connection, conn, m_size);
        strcpy(m_name, name);
    }

    DC_ADDRESS(0x08bd90, 0x18)
    ~CDPlayConnection()
    {
        delete [] m_connection;
    }
};
SIZE(CDPlayConnection, 0x98);

// Dreamcast fixes both fields and the eight-byte extent. Retail's
// CDPlayHeroes constructor/destructor independently show these header-inline
// members: construction clears both dwords, while destruction delegates to
// Destroy(), which frees pData and clears the pair.
class CDPlayMsg {
public:
    unsigned char* m_data;
    unsigned long m_dataSize;

    // separate line rows prove body assignments rather than an initializer
    // list; inlined in its callers.
    DC_ADDRESS(0x08bda8, 0xa)
    CDPlayMsg()
    {
        m_data = 0;
        m_dataSize = 0;
    }

    // Mac's CDPlayHeroes deleting destructor reads its data pointer at +0x0c
    // where this Windows wrapper stores it at +0, so its class identity is
    // unresolved.
    VA(0x00497790, 0x21)  // annotation-only anchor for the active header-inline COMDAT
    DC_ADDRESS(0x08bdb4, 0x18)
    ~CDPlayMsg()
    {
        Destroy();
    }

    // early size guard, conditional delete, allocation, and size store.
    DC_ADDRESS(0x08bdcc, 0x40)
    bool AllocSize(unsigned long dSize)
    {
        if (dSize < m_dataSize)
            return 1;
        if (m_data)
            delete m_data;
        m_data = new unsigned char[dSize];
        m_dataSize = dSize;
        return 1;
    }

    DC_ADDRESS(0x08be0c, 0x2c)
    bool Destroy()
    {
        if (!m_data)
            return 0;
        delete m_data;
        m_data = 0;
        m_dataSize = 0;
        return 1;
    }

    // Original: CDPlayMsg::GetId; dxplay.h:177
    // Inlined in ReceiveSystemMsg: the null-data sentinel and the payload's
    // first dword. The bytes are a DirectPlay message.
    DC_ADDRESS(0x08be38, 0x10)
    unsigned long getId()
    {
        if (!m_data)
            return 0xFFFFFFFF;
        return *static_cast<const unsigned long*>(static_cast<const void*>(m_data));
    }
};
SIZE(CDPlayMsg, 0x08);

// DC's complete 0x104-byte class: a 0x100-byte name (members.csv puts
// m_dpid at 256) followed by the DPID. Retail independently proves that
// tail: UpdateCurrentPlayers compares the result of CAutoArray::Get at
// +0x100. The constructor (DC dxplay.h:203) is in-class: AddPlayerEnum
// news one per enumerated player and expands it there.
class CDPlayPlayer {
public:

    DC_ADDRESS(0x08be48, 0x28)
    CDPlayPlayer(char* name, unsigned long dpid)
    {
        strcpy(m_name, name);
        m_dpid = dpid;
    }

    DC_ADDRESS(0x14746c, 0xa)
    char* getName() { return m_name; }

         // DC dxplay.h:210
    DC_ADDRESS(0x11f7ac, 0x8)
    unsigned long getId() { return m_dpid; }

protected:
    // DC dxplay.h:211

    char m_name[0x100];  // +0x00
    unsigned long m_dpid;  // +0x100
};
SIZE(CDPlayPlayer, 0x104);

// DC types 0x3d83/0x5447 prove the public CDPlayPlayer base at offset zero
// and no added fields. The constructor at 0x8be70 calls that base at
// 0x8be7a. Retail AddGroupEnum expands the same name copy and DPID store.
class CDPlayGroup : public CDPlayPlayer {
public:

    DC_ADDRESS(0x08be70, 0x1c)
    CDPlayGroup(char* name, unsigned long dpid)
        : CDPlayPlayer(name, dpid)
    {
    }
};
SIZE(CDPlayGroup, 0x104);

// The address-element records one DirectPlay SP address chunk EnumAddress splits
// out: a 16-byte data-type GUID, an owned copy of the chunk bytes at +0x10 and
// its size at +0x14. AddAddressEnum news one per enumerated chunk; the array's
// inlined teardown frees the buffer, then the element. CodeView places the
// constructor/destructor in dxplay.h:244/257.
class CDPlayAddressElement {
public:
    GUID m_guid;  // +0x00
    char* m_data;  // +0x10

    DC_ADDRESS(0x08be8c, 0x44)
    CDPlayAddressElement(const GUID* guid, const void* data,
        unsigned long dataSize)
    {
        m_guid = *guid;
        m_dataSize = dataSize;
        m_data = new char[dataSize];
        memcpy(m_data, data, m_dataSize);
    }

    DC_ADDRESS(0x08bed0, 0x18)
    ~CDPlayAddressElement()
    {
        delete [] m_data;
    }
    unsigned long m_dataSize;  // +0x14
};

// Dreamcast CodeView proves this complete virtual order. Retail's
// CDPlay/CDPlayLobby/CDPlayHeroes vtables preserve it: in particular,
// IsHost is slot 36 (+0x90), exactly the indirect call emitted by the main
// menu and oldmain. Retail bodies also prove the complete base data layout.
// Native DC public names encode these boolean interfaces with _N (for example
// ?Send@CDPlay@@UAA_NPAXKKK_N@Z), not unsigned-char E. Preserve their original
// method spellings too; CodeView's lowered primitive 0x20 alone is ambiguous.
class CDPlay {
public:
    DPCAPS m_caps;  // +0x04; first public field in DC field list 0x2a35.

    CDPlay();
    virtual ~CDPlay();
    virtual bool Init();
    virtual bool InitConnection(CDPlayConnection* connection);
    virtual bool HostSession(char* sessionName,
        unsigned long flags, unsigned long maxPlayers, char* password);
    virtual bool JoinSession(GUID* sessionGuid, char* password);
    virtual bool StartSession(unsigned long groupId);
    virtual bool CloseSession();
    virtual unsigned long createPlayer(char* playerName, void* data,
        unsigned long size, void* event);
    virtual bool DestroyPlayer(unsigned long playerId);
    virtual unsigned long CreateGroup(char* groupName, void* groupData,
        unsigned long groupDataSize, bool stagingArea);
    virtual bool DestroyGroup(unsigned long groupId);
    virtual bool DeleteGroupFromGroup(unsigned long dpidParent,
        unsigned long dpidGroup);
    virtual bool SetGroupName(unsigned long groupId, char* shortName,
        char* longName, unsigned long flags);
    virtual bool SetGroupData(unsigned long groupId, void* data,
        unsigned long dataSize, unsigned long flags);
    virtual bool SetPlayerName(unsigned long playerId,
        char* shortName, char* longName, unsigned long flags);
    virtual bool SetPlayerData(unsigned long playerId, void* data,
        unsigned long dataSize, unsigned long flags);
    virtual void* getGroupData(unsigned long groupId, unsigned long* pdwSize,
        unsigned long flags);
    virtual bool GetGroupName(unsigned long groupId, char* shortName,
        int maxShort, char* longName, int maxLong);
    virtual void* getPlayerData(unsigned long playerId, unsigned long* pdwSize,
        unsigned long flags);
    virtual bool GetPlayerName(unsigned long playerId,
        char* shortName, int maxShort, char* longName, int maxLong);
    virtual unsigned long CreateGroupInGroup(unsigned long dpidParent,
        char* groupName, void* groupData, unsigned long dataSize,
        bool stagingArea);
    virtual bool AddPlayerToGroup(unsigned long groupId,
        unsigned long playerId);
    virtual bool DeletePlayerFromGroup(unsigned long groupId,
        unsigned long playerId);
    virtual bool UpdateSessionDesc(DPSESSIONDESC2* sessionDesc);
    virtual DPSESSIONDESC2* getCurrSession();
    virtual bool EnumConnections(
        CAutoArray<CDPlayConnection>* connectionArray);
    virtual bool EnumSessions(CAutoArray<CDPlaySession>* sessionArray,
        unsigned long timeOut, unsigned long flags);
    virtual bool EnumGroups(CAutoArray<CDPlayGroup>* groups,
        GUID* instance, unsigned long flags);
    virtual bool EnumPlayers(CAutoArray<CDPlayPlayer>* players,
        GUID* instance, unsigned long flags);
    virtual bool EnumGroupPlayers(CAutoArray<CDPlayPlayer>* players,
        unsigned long groupId, GUID* instance, unsigned long flags);

    VA(0x00496c70, 0x21)
    DC_ADDRESS(0x08bee8, 0x1c)
    virtual void setGuid(GUID guid)
    {
        m_guid = guid;
    }

    VA(0x00496ca0, 0x4)
    DC_ADDRESS(0x08bf04, 0x6)
    virtual GUID* getGuid()
    {
        return &m_guid;
    }

    // E:\gamedcs\dxplay.h:375
    DC_ADDRESS(0x101d98, 0x6)
    long getLastError() { return m_hRes; }
    virtual bool Send(void* data, unsigned long size,
        unsigned long idFrom, unsigned long idTo, bool guaranteed);
    virtual bool SendChat(char* msg, unsigned long idFrom,
        unsigned long idTo);
    virtual bool Receive(unsigned long* fromID, unsigned long* toID,
        CDPlayMsg* msg, unsigned long flags);
    virtual void getErrorDesc(long error, char* descriptionOut);

    VA(0x00496cb0, 0x4)
    DC_ADDRESS(0x08bf0c, 0x8)
    virtual bool isHost()
    {
        return m_isHost;
    }
    virtual bool FlushReceiveQueue();
    virtual unsigned char* getPlayerAddress(
        unsigned long dpid, unsigned long* sizeOut);
    virtual bool GetCaps(DPCAPS* dpCaps, bool guaranteed);
    // Native publics use AAK0: two unsigned-long output references.
    virtual bool GetSendQueueSize(
        unsigned long from, unsigned long to,
        unsigned long& numMsgs, unsigned long& numBytes);
    virtual bool GetReceiveQueueSize(
        unsigned long from, unsigned long to,
        unsigned long& numMsgs, unsigned long& numBytes);

protected:
    // Native DC field list 0x2a35 places the data before protected methods.
    IDirectPlay4A* m_lpDP;  // +0x2c: DC m_lpDP; Complete creates IDirectPlay4A.
    GUID m_guid;  // +0x30
    long m_hRes;  // +0x40: DC field list 0x2a35 proves this name and long type.
    CAutoArray<CDPlaySession>* m_pSessionArray;  // +0x44
    CAutoArray<CDPlayConnection>* m_pConnectionArray;  // +0x48
    CAutoArray<CDPlayGroup>* m_pGroupArray;  // +0x4c
    CAutoArray<CDPlayPlayer>* m_pPlayerArray;  // +0x50
    unsigned char m_connected;  // +0x54
    unsigned char m_inSession;  // +0x55
    bool m_isHost;  // +0x56 (original m_isHost)

    VA(0x00496cc0, 0x5)
    DC_ADDRESS(0x08bf14, 0x4)
    virtual bool ReceiveMsg(unsigned long from, unsigned long to, CDPlayMsg* msg)
    {
        return 1;
    }
    virtual bool ReceiveSystemMsg(
        unsigned long toID, CDPlayMsg* msg);

    VA(0x00496cd0, 0x5)
    DC_ADDRESS(0x08bf18, 0x4)
    virtual bool SysMsgAddGroupToGroup(DPMSG_ADDGROUPTOGROUP* sysMsg, unsigned long toID)
    {
        return 1;
    }

    // These Complete base/lobby vtable slots share one return-true body;
    // DC's distinct in-class virtual defaults remain distinct source methods.
    // Original: CDPlay::SysMsgAddPlayerToGroup; dxplay.h:441
    DC_ADDRESS(0x08bf1c, 0x4)
    virtual bool SysMsgAddPlayerToGroup(
        DPMSG_ADDPLAYERTOGROUP* message, unsigned long toId)
    {
        return 1;
    }

    // Original: CDPlay::SysMsgChat; dxplay.h:442
    DC_ADDRESS(0x08bf20, 0x4)
    virtual bool SysMsgChat(
        DPMSG_CHAT* message, unsigned long toId)
    {
        return 1;
    }

    // Original: CDPlay::SysMsgDeleteGroupFromGroup; dxplay.h:443
    DC_ADDRESS(0x08bf24, 0x4)
    virtual bool SysMsgDeleteGroupFromGroup(
        DPMSG_ADDGROUPTOGROUP* message, unsigned long toId)
    {
        return 1;
    }

    // Original: CDPlay::SysMsgDeletePlayerFromGroup; dxplay.h:444
    DC_ADDRESS(0x08bf28, 0x4)
    virtual bool SysMsgDeletePlayerFromGroup(
        DPMSG_ADDPLAYERTOGROUP* message, unsigned long toId)
    {
        return 1;
    }

    // Original: CDPlay::SysMsgSecureMessage; dxplay.h:445
    DC_ADDRESS(0x08bf2c, 0x4)
    virtual bool SysMsgSecureMessage(
        DPMSG_SECUREMESSAGE* message, unsigned long toId)
    {
        return 1;
    }

    // Original: CDPlay::SysMsgSessionLost; dxplay.h:446
    DC_ADDRESS(0x08bf30, 0x4)
    virtual bool SysMsgSessionLost(
        DPMSG_GENERIC* message, unsigned long toId)
    {
        return 1;
    }

    // Original: CDPlay::SysMsgSetPlayerOrGroupData; dxplay.h:447
    DC_ADDRESS(0x08bf34, 0x4)
    virtual bool SysMsgSetPlayerOrGroupData(
        DPMSG_SETPLAYERORGROUPDATA* message, unsigned long toId)
    {
        return 1;
    }

    // Original: CDPlay::SysMsgSetPlayerOrGroupName; dxplay.h:448
    DC_ADDRESS(0x08bf38, 0x4)
    virtual bool SysMsgSetPlayerOrGroupName(
        DPMSG_SETPLAYERORGROUPNAME* message, unsigned long toId)
    {
        return 1;
    }

    // Original: CDPlay::SysMsgSetSessionDesc; dxplay.h:449
    DC_ADDRESS(0x08bf3c, 0x4)
    virtual bool SysMsgSetSessionDesc(
        DPMSG_SETSESSIONDESC* message, unsigned long toId)
    {
        return 1;
    }

    // Original: CDPlay::SysMsgStartSession; dxplay.h:450
    DC_ADDRESS(0x08bf40, 0x4)
    virtual bool SysMsgStartSession(
        DPMSG_STARTSESSION* message, unsigned long toId)
    {
        return 1;
    }
    virtual bool SysMsgHost(
        DPMSG_GENERIC* sysMsg, unsigned long toID);
    virtual bool SysMsgCreatePlayerOrGroup(
        DPMSG_CREATEPLAYERORGROUP* sysMsg, unsigned long toID);
    virtual bool SysMsgDestroyPlayerOrGroup(
        DPMSG_DESTROYPLAYERORGROUP* sysMsg, unsigned long toID);

public:
    // The DirectPlay enum trampolines are file-scope callbacks that forward to
    // these virtuals through the lpContext object; keep them reachable without
    // reordering (vtable slots 58-61 are unchanged).
    // DC free enum callbacks 0x8bbc4..0x8bc28 call the protected virtuals;
    // retail 0x499e50..0x499ed0 preserves their address-taken stdcall ABI.
    friend int __stdcall enumSession(const DPSESSIONDESC2* dpSessionDesc, unsigned long* lpdwTimeOut, unsigned long flags, void* context);
    friend int __stdcall enumConnectionsCallback(const GUID* lpguidSP, void* connection, unsigned long connectionSize, const DPNAME* name, unsigned long flags, void* context);
    friend int __stdcall enumGroupsCallback(unsigned long dpid, unsigned long playerType, const DPNAME* name, unsigned long flags, void* context);
    friend int __stdcall enumPlayersCallback(unsigned long dpid, unsigned long playerType, const DPNAME* name, unsigned long flags, void* context);

protected:
    // The DirectPlay enum trampolines are file-scope callbacks that forward to
    // these virtuals through the lpContext object; keep them reachable without
    // reordering (vtable slots 58-61 are unchanged).
    virtual bool AddGroupEnum(
        unsigned long dpid, const DPNAME* name, unsigned long flags);
    virtual bool AddPlayerEnum(
        unsigned long dpid, const DPNAME* name, unsigned long flags);
    virtual bool AddSessionEnum(
        const DPSESSIONDESC2* dpSessionDesc, unsigned long flags);
    virtual bool AddConnectionEnum(
        const GUID* lpguidSP, void* connection,
        unsigned long connectionSize, const DPNAME* name,
        unsigned long flags);

};
SIZE(CDPlay, 0x58);

// Dreamcast CodeView and retail's inherited IsHost call both prove the
// zero-offset CDPlay base.  The two trailing pointers retain their DC offsets
// in retail, so this is also the complete PC layout needed by CDPlayHeroes.
class CDPlayLobby : public CDPlay {
public:
    CDPlayLobby();
    virtual ~CDPlayLobby();
    virtual bool Init();
    virtual bool RegisterApp(
        char* appName, char* fileName, char* commandLine,
        GUID appGuid, char* executableName);
    CDPlayConnection* createTCPIPConnection(
        char* ipAddress, char* name, CDPlayConnection* append);
    CDPlayConnection* createIPXConnection(
        char* name, CDPlayConnection* connAppend);
    CDPlayConnection* createModemConnection(
        char* name, char* phoneNbr, char* modemString);
    CDPlayConnection* createSerialConnection(
        char* name, struct _DPCOMPORTADDRESS* comportInfo);
    DPLCONNECTION* getConnectionSettings(
        unsigned long appId, unsigned long* sizeOut);
    bool SetConnectionSettings(
        unsigned long appId, DPLCONNECTION* connection);
    bool Connect();
    bool SendStandardLobbyMsg(unsigned long appId, void* data,
                                       unsigned long size);
    bool SendLobbyMsg(unsigned long appId, void* data, unsigned long size);
    bool ReceiveLobbyMsg(unsigned long appId, CDPlayMsg* msg);
    bool TestLobbied();
    virtual bool EnumLobbyConnections(
        CAutoArray<CDPlayConnection>* connectionArray);
    virtual bool SetGroupConnectionSettings(
        unsigned long dpidGroup, DPLCONNECTION* connection);
    virtual DPLCONNECTION* getGroupConnectionSettings(
        unsigned long dpidGroup);
    virtual bool EnumGroupsInGroup(
        CAutoArray<CDPlayGroup>* groupArray,
        unsigned long dpidParent, unsigned long flags);
    virtual bool EnumGroupPlayers(
        CAutoArray<CDPlayPlayer>* playerArray,
        unsigned long dpidGroup, unsigned long flags);
    virtual bool EnumGroupPlayersRemote(
        CAutoArray<CDPlayPlayer>* players, unsigned long groupId,
        GUID* instance, unsigned long flags);
    virtual bool EnumAddress(
        void* conn, unsigned long size,
        CAutoArray<CDPlayAddressElement>* array);
    virtual bool GetIPAddress(
        unsigned long dpid, char* ipAddress);

public:
    // Reachable by the EnumAddress file-scope callback (vtable slot unchanged).
    // DC 0x8bba4 and retail 0x499e20 directly call this protected virtual.
    friend int __stdcall EnumAddressCallback(const GUID& guidDataType, unsigned long dwDataSize, const void* lpData, void* lpContext);

protected:
    // DC: IDirectPlayLobby2* m_lpLobby; Complete creates IID_IDirectPlayLobby3A.
    IDirectPlayLobby3A* m_lpLobby;  // +0x58
    CAutoArray<CDPlayAddressElement>* m_pAddressArray;  // +0x5c
    virtual bool HandleSystemLobbyMsg(
        unsigned long appId, CDPlayMsg* msg);
    // Reachable by the EnumAddress file-scope callback (vtable slot unchanged).
    virtual bool AddAddressEnum(
        const GUID* guid, unsigned long dataSize, const void* data);

};
SIZE(CDPlayLobby, 0x60);

#endif  /* HOMM3_DXPLAY_H */
