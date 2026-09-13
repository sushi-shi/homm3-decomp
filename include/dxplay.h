// dxplay.h - prototypes of dxplay.cpp (compiland dxplay.obj)
#ifndef HOMM3_DXPLAY_H
#define HOMM3_DXPLAY_H

#include <string.h>  // strcpy, for CDPlayPlayer's in-class constructor
#include <windows.h>
#include <va.h>
#include "array.h"  // CAutoArray, every Enum* out-parameter

class CDPlayConnection;
class CDPlayAddressElement;
class CDPlayGroup;
class CDPlayMsg;

class CDPlaySession;
template<class T> class CAutoArray;
struct DPCAPS;
struct DPLCONNECTION;
struct DPNAME;
struct DPSESSIONDESC2;
struct DPMSG_ADDGROUPTOGROUP;
struct DPMSG_ADDPLAYERTOGROUP;
struct DPMSG_CHAT;
struct DPMSG_CREATEPLAYERORGROUP;
struct DPMSG_DESTROYPLAYERORGROUP;
struct DPMSG_GENERIC;
struct DPMSG_SECUREMESSAGE;
struct DPMSG_SETPLAYERORGROUPDATA;
struct DPMSG_SETPLAYERORGROUPNAME;
struct DPMSG_SETSESSIONDESC;
struct DPMSG_STARTSESSION;

// DirectPlay 6 structures used by remote's retail lobby-connect path. The
// layouts are published in the Dreamcast CodeView stream and independently
// fixed on PC by the field loads in LobbyLaunchConnect.
struct DPNAME {
public:
    unsigned long m_size;
    unsigned long m_flags;
    union {
        unsigned short* m_shortName;
        char* m_shortNameA;
    };
    union {
        unsigned short* m_longName;
        char* m_longNameA;
    };
};
SIZE(DPNAME, 0x10);

struct DPSESSIONDESC2 {
public:
    unsigned long m_size;
    unsigned long m_flags;
    GUID m_guidInstance;
    GUID m_guidApplication;
    unsigned long m_maxPlayers;
    unsigned long m_currentPlayers;
    union {
        unsigned short* m_sessionName;
        char* m_sessionNameA;
    };
    union {
        unsigned short* m_password;
        char* m_passwordA;
    };
    unsigned long m_reserved1;
    unsigned long m_reserved2;
    unsigned long m_user1;
    unsigned long m_user2;
    unsigned long m_user3;
    unsigned long m_user4;
};
SIZE(DPSESSIONDESC2, 0x50);

struct DPLCONNECTION {
public:
    unsigned long m_size;
    unsigned long m_flags;
    DPSESSIONDESC2* m_sessionDesc;
    DPNAME* m_playerName;
    GUID m_guidSp;
    void* m_address;
    unsigned long m_addressSize;
};
SIZE(DPLCONNECTION, 0x28);

// DirectPlay's player/group destruction notification. remote.obj's override
// of the system-message slot reads only the leading discriminator and the
// id of the station that left, which is what fixes them at +4 and +8; the
// tail follows dplay.h's published record so the struct is not a fake.
struct DPMSG_DESTROYPLAYERORGROUP {
public:
    unsigned long m_type;
    unsigned long m_playerType;
    unsigned long m_dpId;
    void* m_localData;
    unsigned long m_localDataSize;
    void* m_remoteData;
    unsigned long m_remoteDataSize;
    DPNAME m_dpnName;
    unsigned long m_dpIdParent;
    unsigned long m_flags;
};

// dplay.h's DPPLAYERTYPE_ pair, the domain of the field above.
enum EDPlayerType {
    DPPLAYERTYPE_GROUP = 0,
    DPPLAYERTYPE_PLAYER = 1
};

enum EDPlayConnectionFlags {
    DPLAY_CONNECTION_CREATE_SESSION = 0x2
};

enum EDPlaySessionFlags {
    DPLAY_SESSION_MIGRATE_HOST = 0x4,
    DPLAY_SESSION_KEEP_ALIVE = 0x40
};

// The SDK macro values are also the exact HRESULT immediates used by the
// DirectPlay send path. The domain lives here instead of importing DPLAY.H's
// anonymous typedef structs over these hand-owned forward declarations.
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
    CDPlaySession(const DPSESSIONDESC2* session)
    {
        if (session) {
            m_flags = session->m_flags;
            m_guidInstance = session->m_guidInstance;
            m_guidApp = session->m_guidApplication;
            m_maxPlayers = session->m_maxPlayers;
            m_playerCount = session->m_currentPlayers;
            m_user1 = session->m_user1;
            m_user2 = session->m_user2;
            m_user3 = session->m_user3;
            m_user4 = session->m_user4;
            strcpy(m_sessionName, session->m_sessionNameA);
            if (session->m_passwordA)
                strcpy(m_password, session->m_passwordA);
            else
                m_password[0] = 0;
        }
    }

    VA(0x005112c0, 0x1C)  // exact selected COMDAT, dc 0x101d58
    unsigned char isJoinDisabled()
    {
        if (m_flags & 0x20)
            return 1;
        if (m_flags & 1)
            return 1;
        unsigned char disabled = m_playerCount == m_maxPlayers;
        return disabled;
    }
    unsigned char isPasswordProtected()
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
    CDPlayConnection(const GUID* guid, unsigned long connSize, void* conn,
        char* name)
    {
        m_guidSp = *guid;
        m_size = connSize;
        m_connection = new unsigned char[connSize];
        memcpy(m_connection, conn, m_size);
        strcpy(m_name, name);
    }
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
    // CODEVIEW(E:\gamedcs\dxplay.h:137, dc 0x8bda8).  The constructor's
    // separate line rows prove body assignments rather than an initializer list;
    // Complete folds the helper into its callers while preserving both stores.
    CDPlayMsg()
    {
        m_data = 0;
        m_dataSize = 0;
    }
    // CODEVIEW(E:\gamedcs\dxplay.h:145, dc 0x8bdb4)
    VA(0x00497790, 0x21)  // annotation-only anchor for the active header-inline COMDAT
    ~CDPlayMsg()
    {
        destroy();
    }
    // CODEVIEW(E:\gamedcs\dxplay.h:150, dc 0x8bdcc).  Dreamcast proves the
    // early size guard, conditional delete, allocation, and size store; retail's
    // inlined cmp/jb fixes this equivalent operand order.
    unsigned char allocSize(unsigned long dSize)
    {
        if (dSize < m_dataSize)
            return 1;
        if (m_data)
            delete m_data;
        m_data = new unsigned char[dSize];
        m_dataSize = dSize;
        return 1;
    }
    // CODEVIEW(E:\gamedcs\dxplay.h:164, dc 0x8be0c)
    unsigned char destroy()
    {
        if (!m_data)
            return 0;
        delete m_data;
        m_data = 0;
        m_dataSize = 0;
        return 1;
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
    CDPlayPlayer(char* name, unsigned long dpid)
    {
        strcpy(m_name, name);
        m_dpid = dpid;
    }
    char* getName() { return m_name; }
         // DC dxplay.h:210
    unsigned long getId() { return m_dpid; }

protected:
    // DC dxplay.h:211

    char m_name[0x100];  // +0x00
    unsigned long m_dpid;  // +0x100
};
SIZE(CDPlayPlayer, 0x104);

// The group enum trampoline's backing record: a 0x100-byte name buffer
// followed by the DPID at +0x100 (0x104 total). AddGroupEnum news one,
// strcpys the enumerated short name in, and stores the id. CDPlayGroup is
// defined in dxplay.h:231, following its player-record twin.
class CDPlayGroup {
public:
    CDPlayGroup(char* name, unsigned long dpid)
    {
        strcpy(m_name, name);
        m_dpid = dpid;
    }
    char m_name[0x100];  // +0x00
    unsigned long m_dpid;  // +0x100
};

// The address-element records one DirectPlay SP address chunk EnumAddress splits
// out: a 16-byte data-type GUID, an owned copy of the chunk bytes at +0x10 and
// its size at +0x14. AddAddressEnum news one per enumerated chunk; the array's
// inlined teardown frees the buffer, then the element. CodeView places the
// constructor/destructor in dxplay.h:244/257.
class CDPlayAddressElement {
public:
    GUID m_guid;  // +0x00
    char* m_data;  // +0x10
    CDPlayAddressElement(const GUID* guid, const void* data,
        unsigned long dataSize)
    {
        m_guid = *guid;
        m_dataSize = dataSize;
        m_data = new char[dataSize];
        memcpy(m_data, data, m_dataSize);
    }
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
class CDPlay {
public:
    CDPlay();
    virtual ~CDPlay();
    virtual unsigned char init();
    virtual unsigned char initConnection(CDPlayConnection* connection);
    virtual unsigned char hostSession(char* sessionName,
        unsigned long flags, unsigned long maxPlayers, char* password);
    virtual unsigned char joinSession(GUID* sessionGuid, char* password);
    virtual unsigned char startSession(unsigned long groupId);
    virtual unsigned char closeSession();
    virtual unsigned long createPlayer(char* playerName, void* data,
        unsigned long size, void* eventHandle);
    virtual unsigned char destroyPlayer(unsigned long playerId);
    virtual unsigned long createGroup(char* groupName, void* data,
        unsigned long size, unsigned char stagingArea);
    virtual unsigned char destroyGroup(unsigned long groupId);
    virtual unsigned char deleteGroupFromGroup(unsigned long parentId,
        unsigned long groupId);
    virtual unsigned char setGroupName(unsigned long groupId, char* shortName,
        char* longName, unsigned long flags);
    virtual unsigned char setGroupData(unsigned long groupId, void* data,
        unsigned long size, unsigned long flags);
    virtual unsigned char setPlayerName(unsigned long playerId,
        char* shortName, char* longName, unsigned long flags);
    virtual unsigned char setPlayerData(unsigned long playerId, void* data,
        unsigned long size, unsigned long flags);
    virtual void* getGroupData(unsigned long groupId, unsigned long* size,
        unsigned long flags);
    virtual unsigned char getGroupName(unsigned long groupId, char* shortName,
        int maxShort, char* longName, int maxLong);
    virtual void* getPlayerData(unsigned long playerId, unsigned long* size,
        unsigned long flags);
    virtual unsigned char getPlayerName(unsigned long playerId,
        char* shortName, int maxShort, char* longName, int maxLong);
    virtual unsigned long createGroupInGroup(unsigned long parentId,
        char* groupName, void* data, unsigned long size,
        unsigned char stagingArea);
    virtual unsigned char addPlayerToGroup(unsigned long groupId,
        unsigned long playerId);
    virtual unsigned char deletePlayerFromGroup(unsigned long groupId,
        unsigned long playerId);
    virtual unsigned char updateSessionDesc(DPSESSIONDESC2* session);
    virtual DPSESSIONDESC2* getCurrSession();
    virtual unsigned char enumConnections(
        CAutoArray<CDPlayConnection>* connections);
    virtual unsigned char enumSessions(CAutoArray<CDPlaySession>* sessions,
        unsigned long timeout, unsigned long flags);
    virtual unsigned char enumGroups(CAutoArray<CDPlayGroup>* groups,
        GUID* instance, unsigned long flags);
    virtual unsigned char enumPlayers(CAutoArray<CDPlayPlayer>* players,
        GUID* instance, unsigned long flags);
    virtual unsigned char enumGroupPlayers(CAutoArray<CDPlayPlayer>* players,
        unsigned long groupId, GUID* instance, unsigned long flags);
    VA(0x00496c70, 0x21)  // dc 0x8bee8
    virtual void setGuid(GUID guid)
    {
        m_guid = guid;
    }
    VA(0x00496ca0, 0x4)  // dc 0x8bf04
    virtual GUID* getGuid()
    {
        return &m_guid;
    }
    // E:\gamedcs\dxplay.h:375
    long getLastError() { return m_res; }
    virtual unsigned char send(void* data, unsigned long size,
        unsigned long fromId, unsigned long toId, unsigned char guaranteed);
    virtual unsigned char sendChat(char* message, unsigned long fromId,
        unsigned long toId);
    virtual unsigned char receive(unsigned long* fromId, unsigned long* toId,
        CDPlayMsg* message, unsigned long flags);
    virtual void getErrorDesc(long error, char* description);
    VA(0x00496cb0, 0x4)  // dc 0x8bf0c
    virtual bool isHost()
    {
        return m_isHost;
    }
    virtual unsigned char flushReceiveQueue();
    virtual unsigned char* getPlayerAddress(
        unsigned long playerId, unsigned long* size);
    virtual unsigned char getCaps(DPCAPS* caps, unsigned char guaranteed);
    virtual unsigned char getSendQueueSize(
        unsigned long fromId, unsigned long toId,
        unsigned long* numMessages, unsigned long* numBytes);
    virtual unsigned char getReceiveQueueSize(
        unsigned long fromId, unsigned long toId,
        unsigned long* numMessages, unsigned long* numBytes);

protected:
    VA(0x00496cc0, 0x5)  // dc 0x8bf14
    virtual unsigned char receiveMsg(unsigned long from, unsigned long to, CDPlayMsg* msg)
    {
        return 1;
    }
    virtual unsigned char receiveSystemMsg(
        unsigned long toId, CDPlayMsg* message);
    VA(0x00496cd0, 0x5)  // dc 0x8bf18
    virtual unsigned char sysMsgAddGroupToGroup(DPMSG_ADDGROUPTOGROUP* sysMsg, unsigned long toID)
    {
        return 1;
    }
    virtual unsigned char sysMsgAddPlayerToGroup(
        DPMSG_ADDPLAYERTOGROUP* message, unsigned long toId);
    virtual unsigned char sysMsgChat(
        DPMSG_CHAT* message, unsigned long toId);
    virtual unsigned char sysMsgDeleteGroupFromGroup(
        DPMSG_ADDGROUPTOGROUP* message, unsigned long toId);
    virtual unsigned char sysMsgDeletePlayerFromGroup(
        DPMSG_ADDPLAYERTOGROUP* message, unsigned long toId);
    virtual unsigned char sysMsgSecureMessage(
        DPMSG_SECUREMESSAGE* message, unsigned long toId);
    virtual unsigned char sysMsgSessionLost(
        DPMSG_GENERIC* message, unsigned long toId);
    virtual unsigned char sysMsgSetPlayerOrGroupData(
        DPMSG_SETPLAYERORGROUPDATA* message, unsigned long toId);
    virtual unsigned char sysMsgSetPlayerOrGroupName(
        DPMSG_SETPLAYERORGROUPNAME* message, unsigned long toId);
    virtual unsigned char sysMsgSetSessionDesc(
        DPMSG_SETSESSIONDESC* message, unsigned long toId);
    virtual unsigned char sysMsgStartSession(
        DPMSG_STARTSESSION* message, unsigned long toId);
    virtual unsigned char sysMsgHost(
        DPMSG_GENERIC* message, unsigned long toId);
    virtual unsigned char sysMsgCreatePlayerOrGroup(
        DPMSG_CREATEPLAYERORGROUP* message, unsigned long toId);
    virtual unsigned char sysMsgDestroyPlayerOrGroup(
        DPMSG_DESTROYPLAYERORGROUP* message, unsigned long toId);

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
    // Protected (not private): CDPlayLobby's own methods write m_lpDP, m_hRes,
    // m_isHost and the array pointers directly, exactly as retail does.
    // Retail's vtable slots 30, 31, and 36 prove the GUID and IsHost
    // offsets. The intervening names are Dreamcast CodeView's and agree
    // with the PC methods; DPCAPS stays opaque until a retail body needs it.
    char m_caps[0x28];  // +0x04

protected:
    void* m_dp;  // +0x2c
    GUID m_guid;  // +0x30
    // The DirectPlay enum trampolines are file-scope callbacks that forward to
    // these virtuals through the lpContext object; keep them reachable without
    // reordering (vtable slots 58-61 are unchanged).
    virtual unsigned char addGroupEnum(
        unsigned long groupId, const DPNAME* name, unsigned long flags);
    virtual unsigned char addPlayerEnum(
        unsigned long playerId, const DPNAME* name, unsigned long flags);
    virtual unsigned char addSessionEnum(
        const DPSESSIONDESC2* session, unsigned long flags);
    virtual unsigned char addConnectionEnum(
        const GUID* serviceProvider, void* connection,
        unsigned long connectionSize, const DPNAME* name,
        unsigned long flags);
    long m_res;  // +0x40, DC long / SDK HRESULT
    CAutoArray<CDPlaySession>* m_sessionArray;  // +0x44
    CAutoArray<CDPlayConnection>* m_connectionArray;  // +0x48
    CAutoArray<CDPlayGroup>* m_groupArray;  // +0x4c
    CAutoArray<CDPlayPlayer>* m_playerArray;  // +0x50
    unsigned char m_connected;  // +0x54
    unsigned char m_inSession;  // +0x55
    bool m_isHost;  // +0x56 (original m_isHost)
    // Dreamcast CDPlay ends with connected/inSession/isHost at
    // 84/85/86. Retail retains the same bytes and 0x58-byte size;
    // this last byte aligns the complete object.
    char m_tailPadding;
};
SIZE(CDPlay, 0x58);

// Dreamcast CodeView and retail's inherited IsHost call both prove the
// zero-offset CDPlay base.  The two trailing pointers retain their DC offsets
// in retail, so this is also the complete PC layout needed by CDPlayHeroes.
class CDPlayLobby : public CDPlay {
public:
    CDPlayLobby();
    virtual ~CDPlayLobby();
    virtual unsigned char init();
    virtual unsigned char registerApp(
        char* appName, char* fileName, char* commandLine,
        GUID appGuid, char* executableName);
    CDPlayConnection* createTCPIPConnection(
        char* ipAddress, char* name, CDPlayConnection* append);
    CDPlayConnection* createIPXConnection(
        char* name, CDPlayConnection* append);
    CDPlayConnection* createModemConnection(
        char* name, char* phoneNumber, char* modemString);
    CDPlayConnection* createSerialConnection(
        char* name, struct _DPCOMPORTADDRESS* comportInfo);
    DPLCONNECTION* getConnectionSettings(
        unsigned long appId, unsigned long* size);
    unsigned char setConnectionSettings(
        unsigned long appId, DPLCONNECTION* connection);
    unsigned char connect();
    // DC public ?TestLobbied@CDPlayLobby@@QAA_NXZ proves bool; the
    bool testLobbied();
    virtual unsigned char enumLobbyConnections(
        CAutoArray<CDPlayConnection>* connections);
    virtual unsigned char setGroupConnectionSettings(
        unsigned long groupId, DPLCONNECTION* connection);
    virtual DPLCONNECTION* getGroupConnectionSettings(
        unsigned long groupId);
    virtual unsigned char enumGroupsInGroup(
        CAutoArray<CDPlayGroup>* groups,
        unsigned long parentId, unsigned long flags);
    virtual unsigned char enumGroupPlayers(
        CAutoArray<CDPlayPlayer>* players,
        unsigned long groupId, unsigned long flags);
    virtual unsigned char enumGroupPlayersRemote(
        CAutoArray<CDPlayPlayer>* players, unsigned long groupId,
        GUID* instance, unsigned long flags);
    virtual unsigned char enumAddress(
        void* connection, unsigned long size,
        CAutoArray<CDPlayAddressElement>* addresses);
    virtual unsigned char getIPAddress(
        unsigned long playerId, char* ipAddress);

protected:
    virtual unsigned char handleSystemLobbyMsg(
        unsigned long appId, CDPlayMsg* message);

public:
    // Reachable by the EnumAddress file-scope callback (vtable slot unchanged).
    // DC 0x8bba4 and retail 0x499e20 directly call this protected virtual.
    friend int __stdcall enumAddressCallback(const GUID* guidDataType, unsigned long dataSize, const void* data, void* context);

protected:
    void* m_lobby;  // +0x58
    CAutoArray<CDPlayAddressElement>* m_addressArray;  // +0x5c
    // Reachable by the EnumAddress file-scope callback (vtable slot unchanged).
    virtual unsigned char addAddressEnum(
        const GUID* type, unsigned long size, const void* data);
};
SIZE(CDPlayLobby, 0x60);

// --- globals ---
// CODEVIEW(C:\WCEDreamcast\inc\objbase.h:519, dc 0x8bc84) int operator==(const _GUID* guidOne, const _GUID* guidOther);

// --- CAutoArray<CDPlayAddressElement> ---
// CODEVIEW(E:\gamedcs\array.h:37, dc 0x8c118) void CAutoArray<CDPlayAddressElement>::CAutoArray<CDPlayAddressElement>();
// CODEVIEW(E:\gamedcs\array.h:73, dc 0x8c1e4) unsigned char CAutoArray<CDPlayAddressElement>::Add(CDPlayAddressElement* element);
// CODEVIEW(E:\gamedcs\array.h:113, dc 0x8c278) unsigned char CAutoArray<CDPlayAddressElement>::Delete(unsigned long elementNbr);
// CODEVIEW(E:\gamedcs\array.h:127, dc 0x8c2b4) unsigned char CAutoArray<CDPlayAddressElement>::Insert(unsigned long nextElementNbr, CDPlayAddressElement* element);
// CODEVIEW(..\stlport\stl_bvector.h:144, dc 0x8c35c) void* CAutoArray<CDPlayAddressElement>::`scalar deleting destructor'(unsigned __flags);

// --- CAutoArray<CDPlayConnection> ---
// CODEVIEW(E:\gamedcs\array.h:51, dc 0x8bfac) void CAutoArray<CDPlayConnection>::Destroy(unsigned char deleteData);

// --- CAutoArray<CDPlayGroup> ---
// CODEVIEW(E:\gamedcs\array.h:51, dc 0x8c010) void CAutoArray<CDPlayGroup>::Destroy(unsigned char deleteData);

// --- CAutoArray<CDPlayPlayer> ---
// CODEVIEW(E:\gamedcs\array.h:51, dc 0x8c068) void CAutoArray<CDPlayPlayer>::Destroy(unsigned char deleteData);

// --- CAutoArray<CDPlaySession> ---

// --- CDPlay ---
// CODEVIEW(E:\gamedcs\dxplay.cpp:66, dc 0x8a074) void CDPlay::CDPlay();
// CODEVIEW(E:\gamedcs\dxplay.cpp:890, dc 0x8af4c) unsigned char CDPlay::SysMsgDestroyPlayerOrGroup(DPMSG_DESTROYPLAYERORGROUP* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\dxplay.h:441, dc 0x8bf1c) unsigned char CDPlay::SysMsgAddPlayerToGroup(DPMSG_ADDPLAYERTOGROUP* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\dxplay.h:442, dc 0x8bf20) unsigned char CDPlay::SysMsgChat(DPMSG_CHAT* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\dxplay.h:443, dc 0x8bf24) unsigned char CDPlay::SysMsgDeleteGroupFromGroup(DPMSG_ADDGROUPTOGROUP* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\dxplay.h:444, dc 0x8bf28) unsigned char CDPlay::SysMsgDeletePlayerFromGroup(DPMSG_ADDPLAYERTOGROUP* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\dxplay.h:445, dc 0x8bf2c) unsigned char CDPlay::SysMsgSecureMessage(DPMSG_SECUREMESSAGE* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\dxplay.h:446, dc 0x8bf30) unsigned char CDPlay::SysMsgSessionLost(DPMSG_GENERIC* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\dxplay.h:447, dc 0x8bf34) unsigned char CDPlay::SysMsgSetPlayerOrGroupData(DPMSG_SETPLAYERORGROUPDATA* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\dxplay.h:448, dc 0x8bf38) unsigned char CDPlay::SysMsgSetPlayerOrGroupName(DPMSG_SETPLAYERORGROUPNAME* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\dxplay.h:449, dc 0x8bf3c) unsigned char CDPlay::SysMsgSetSessionDesc(DPMSG_SETSESSIONDESC* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\dxplay.h:450, dc 0x8bf40) unsigned char CDPlay::SysMsgStartSession(DPMSG_STARTSESSION* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\dxplay.cpp:89, dc 0x8bf44) void* CDPlay::`scalar deleting destructor'(unsigned __flags);

// --- CDPlayAddressElement ---
// CODEVIEW(E:\gamedcs\dxplay.h:244, dc 0x8be8c) void CDPlayAddressElement::CDPlayAddressElement(const _GUID* lpGuid, const void* pData, unsigned long dataSize);
// CODEVIEW(E:\gamedcs\dxplay.h:257, dc 0x8bed0) void CDPlayAddressElement::~CDPlayAddressElement();
// CODEVIEW(..\stlport\stl_bvector.h:144, dc 0x8c390) void* CDPlayAddressElement::`scalar deleting destructor'(unsigned __flags);

// --- CDPlayConnection ---
// CODEVIEW(E:\gamedcs\dxplay.h:113, dc 0x8bd2c) void CDPlayConnection::CDPlayConnection(const _GUID* lpGuid, unsigned long connSize, void* lpConn, char* name);
// CODEVIEW(E:\gamedcs\dxplay.h:125, dc 0x8bd90) void CDPlayConnection::~CDPlayConnection();
// CODEVIEW(..\stlport\stl_bvector.h:144, dc 0x8c328) void* CDPlayConnection::`scalar deleting destructor'(unsigned __flags);

// --- CDPlayGroup ---
// CODEVIEW(E:\gamedcs\dxplay.h:231, dc 0x8be70) void CDPlayGroup::CDPlayGroup(char* sName, unsigned long dpid);

// --- CDPlayLobby ---
// CODEVIEW(E:\gamedcs\dxplay.cpp:1351, dc 0x8b69c) unsigned char CDPlayLobby::TestLobbied();
// CODEVIEW(E:\gamedcs\dxplay.cpp:1476, dc 0x8b808) unsigned char CDPlayLobby::SendStandardLobbyMsg(unsigned long dwAppId, void* pData, unsigned long dwSize);
// CODEVIEW(E:\gamedcs\dxplay.cpp:1490, dc 0x8b864) unsigned char CDPlayLobby::SendLobbyMsg(unsigned long dwAppId, void* pData, unsigned long dwSize);
// CODEVIEW(E:\gamedcs\dxplay.cpp:1503, dc 0x8b8a8) unsigned char CDPlayLobby::ReceiveLobbyMsg(unsigned long dwAppId, CDPlayMsg* pMsg);
// CODEVIEW(E:\gamedcs\dxplay.cpp:1802, dc 0x8b960) unsigned char CDPlayLobby::HandleSystemLobbyMsg(unsigned long dwAppId, CDPlayMsg* pMsg);
// CODEVIEW(E:\gamedcs\dxplay.cpp:1215, dc 0x8bf78) void* CDPlayLobby::`scalar deleting destructor'(unsigned __flags);

// --- CDPlayMsg ---
// CODEVIEW(E:\gamedcs\dxplay.h:137, dc 0x8bda8) void CDPlayMsg::CDPlayMsg();
// CODEVIEW(E:\gamedcs\dxplay.h:150, dc 0x8bdcc) unsigned char CDPlayMsg::AllocSize(unsigned long dSize);
// CODEVIEW(E:\gamedcs\dxplay.h:177, dc 0x8be38) unsigned long CDPlayMsg::GetId();

// --- CDPlayPlayer ---
// CODEVIEW(E:\gamedcs\dxplay.h:203, dc 0x8be48) void CDPlayPlayer::CDPlayPlayer(char* sName, unsigned long dpid);

// --- CDPlaySession ---
// CODEVIEW(E:\gamedcs\dxplay.h:57, dc 0x8bca0) void CDPlaySession::CDPlaySession(const DPSESSIONDESC2* lpSession);

#endif  /* HOMM3_DXPLAY_H */
