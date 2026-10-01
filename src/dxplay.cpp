// Retail emits this TU in the drawing->event_record gap (0x96c50..0x9a1e0,
// retail address bracket); ds_engine, the other alphabetical
// candidate for that gap, is NOT resident here - GetErrorDesc's DPERR switch,
// the CDPlay/CDPlayLobby/CAutoArray vtables and the cinit tail all place the
// whole gap in dxplay. Header-inline virtuals retain their canonical bodies
// and VA annotations in dxplay.h; ordinary definitions here follow retail RVA.

// The CDPlay (0x63dc28, 62 slots), CDPlayLobby (0x63dd20, 73 slots) and
// CAutoArray<CDPlayAddressElement> (0x63de44, 7 slots) vtables read out of the
// image are the primary identity oracle: each slot is a direct address-take of
// a virtual, and the slot order matches dxplay.h's declaration order exactly
// (SetGuid slot30 +0x78, IsHost slot36 +0x90, both already proven by the three
// compiled bodies).  Non-virtual Create*Connection are dispatched from one
// remote-side caller (0x1556e0) in source order; the two CDPlayLobby ctor/dtor
// rows are proven by their dual base+derived vtable stores.
// dxplay.cpp is the DirectPlay TU: it sees the complete DP6 value structures and
// the lobby non-virtual member set, exactly as retail did.
#include "va.h"

#include "dxplay.h"

#include "dxplay_com.h"
#include "exceptions.h"

// Project-inferred name operations shared by group creation and the player/
// group name APIs. These borrow names and preserve the SDK record layout.
static void initializeDirectPlayName(DPNAME& name, char* shortName, char* longName)
{
    name.m_size = sizeof(DPNAME);
    name.m_flags = 0;
    name.m_shortNameA = shortName;
    name.m_longNameA = longName;
}

static void copyDirectPlayName(const DPNAME& name, char* shortName, int maxShort,
                               char* longName, int maxLong)
{
    if (shortName) {
        if (name.m_shortNameA)
            strncpy(shortName, name.m_shortNameA, maxShort);
        else
            shortName[0] = 0;
    }
    if (longName) {
        if (name.m_longNameA)
            strncpy(longName, name.m_longNameA, maxLong);
        else
            longName[0] = 0;
    }
}

// File-scope DirectPlay enumeration trampolines (defined at the tail of this TU),
// forward-declared so the Enum* wrappers above them can take their addresses.
int __stdcall enumAddressCallback(const GUID*, unsigned long, const void*, void*);
int __stdcall enumSession(const DPSESSIONDESC2*, unsigned long*, unsigned long, void*);
int __stdcall enumConnectionsCallback(const GUID*, void*, unsigned long, const DPNAME*, unsigned long, void*);
int __stdcall enumGroupsCallback(unsigned long, unsigned long, const DPNAME*, unsigned long, void*);
int __stdcall enumPlayersCallback(unsigned long, unsigned long, const DPNAME*, unsigned long, void*);

// One-shot COM apartment guard: the CDPlay base constructor CoInitializes the
// process the first time any DirectPlay object is built. Retail's inlined copy
// in the CDPlayLobby constructor tests and sets this byte around CoInitialize
// (0x4988ca/0x4988d9).
DATA(0x006969d8) static unsigned char g_coInitialized = 0;

// E:\gamedcs\dxplay.cpp:66 - the CDPlay base ctor has no standalone retail body;
// it is emitted only inlined into the CDPlayLobby (and CDPlayHeroes) ctors. The
// process is CoInitialized once, guarded by a file-scope flag.
DC_ADDRESS(0x08a074, 0x70)
CDPlay::CDPlay()
{
    m_dp = 0;
    m_connected = 0;
    m_inSession = 0;
    m_isHost = 0;
    m_res = 0;
    m_guid = GUID_NULL;
    m_sessionArray = 0;
    m_connectionArray = 0;
    m_groupArray = 0;
    m_playerArray = 0;
    memset(m_caps, 0, sizeof(m_caps));
    if (!g_coInitialized) {
        CoInitialize(0);
        g_coInitialized = 1;
    }
}

VA_COMPGEN(0x00496ce0, 0x2F, SCALAR_DELETING_DTOR, CDPlay)

VA(0x00496d10, 0x14)
DC_ADDRESS(0x08a0e4, 0x36)
CDPlay::~CDPlay()
{
    if (m_dp)
        static_cast<IDirectPlay4A*>(m_dp)->Release();
}

// Project-inferred operation shared by initialization and lobby connection.
// Keep Release before clearing the pointer, and leave connection flags alone.
void CDPlay::releaseDirectPlay()
{
    if (m_dp) {
        static_cast<IDirectPlay4A*>(m_dp)->Release();
        m_dp = 0;
    }
}

VA(0x00496d30, 0x3A)
DC_ADDRESS(0x08a11c, 0x4)
unsigned char CDPlay::init()
{
    releaseDirectPlay();
    m_res = CoCreateInstance(CLSID_DirectPlay, 0, CLSCTX_INPROC_SERVER,
        IID_IDirectPlay4A, &m_dp);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00496d70, 0x33)
DC_ADDRESS(0x08a120, 0x34)
unsigned char CDPlay::initConnection(CDPlayConnection* connection)
{
    if (!m_dp)
        return 0;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->InitializeConnection(connection->m_connection, 0);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00496db0, 0xB0)
DC_ADDRESS(0x08a154, 0x4)
unsigned char CDPlay::hostSession(char* sessionName, unsigned long flags,
                                  unsigned long maxPlayers, char* password)
{
    if (!m_dp)
        return 0;
    DPSESSIONDESC2 desc;
    memset(&desc, 0, sizeof(desc));
    desc.m_size = sizeof(desc);
    desc.m_flags = flags;
    desc.m_maxPlayers = maxPlayers;
    desc.m_sessionNameA = sessionName;
    if (password)
        desc.m_passwordA = password;
    if (memcmp(&m_guid, &GUID_NULL, sizeof(GUID)) != 0)
        desc.m_guidApplication = m_guid;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->Open(&desc, 2);
    if (m_res < 0)
        return 0;
    m_isHost = 1;
    getCaps(static_cast<DPCAPS*>(static_cast<void*>(m_caps)), 1);
    return 1;
}

VA(0x00496e60, 0xB8)
DC_ADDRESS(0x08a158, 0x4)
unsigned char CDPlay::joinSession(GUID* sessionGuid, char* password)
{
    if (!m_dp)
        return 0;
    DPSESSIONDESC2 desc;
    memset(&desc, 0, sizeof(desc));
    desc.m_size = sizeof(desc);
    desc.m_passwordA = password;
    if (sessionGuid)
        desc.m_guidInstance = *sessionGuid;
    if (memcmp(&m_guid, &GUID_NULL, sizeof(GUID)) != 0)
        desc.m_guidApplication = m_guid;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->Open(&desc, 1);
    if (m_res < 0)
        return 0;
    m_isHost = 0;
    getCaps(static_cast<DPCAPS*>(static_cast<void*>(m_caps)), 1);
    return 1;
}

VA(0x00496f20, 0x6D)
DC_ADDRESS(0x08a15c, 0x72)
DPSESSIONDESC2* CDPlay::getCurrSession()
{
    if (!m_dp)
        return 0;
    unsigned long size = 0;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetSessionDesc(0, &size);
    if (m_res != DPERR_BUFFERTOOSMALL || size == 0)
        return 0;
    DPSESSIONDESC2* buf = static_cast<DPSESSIONDESC2*>(::operator new(size));
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetSessionDesc(buf, &size);
    if (m_res < 0) {
        ::operator delete(buf);
        buf = 0;
    }
    return buf;
}

VA(0x00496f90, 0x22)
DC_ADDRESS(0x08a1d0, 0x28)
unsigned char CDPlay::updateSessionDesc(DPSESSIONDESC2* sessionDesc)
{
    m_res = static_cast<IDirectPlay4A*>(m_dp)->SetSessionDesc(sessionDesc, 0);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00496fc0, 0x72)
DC_ADDRESS(0x08a1f8, 0x94)
unsigned long CDPlay::createPlayer(char* playerName, void* data, unsigned long size, void* event)
{
    if (!m_dp)
        return 0;
    unsigned long flags = 0;
    DPNAME dpName;
    memset(&dpName, 0, sizeof(dpName));
    unsigned long idPlayer;
    dpName.m_size = sizeof(DPNAME);
    dpName.m_shortNameA = playerName;
    if (m_isHost)
        flags = 0x100;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->CreatePlayer(
        &idPlayer, &dpName, event, data, size, flags);
    return m_res >= 0 ? idPlayer : 0;
}

VA(0x00497040, 0x2B)
DC_ADDRESS(0x08a28c, 0x2e)
unsigned char CDPlay::destroyPlayer(unsigned long playerId)
{
    if (!m_dp)
        return 0;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->DestroyPlayer(playerId);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00497070, 0x66)
DC_ADDRESS(0x08a2bc, 0x5e)
unsigned long CDPlay::createGroup(char* groupName, void* groupData, unsigned long groupDataSize, unsigned char stagingArea)
{
    if (!m_dp)
        return 0;
    unsigned long flags = 0;
    DPNAME dpName;
    initializeDirectPlayName(dpName, groupName, groupName);
    if (stagingArea)
        flags = 0x800;
    unsigned long idGroup;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->CreateGroup(&idGroup, &dpName, groupData, groupDataSize, flags);
    return m_res >= 0 ? idGroup : 0;
}

VA(0x004970e0, 0x62)
DC_ADDRESS(0x08a31c, 0x5c)
unsigned long CDPlay::createGroupInGroup(unsigned long dpidParent, char* groupName, void* groupData, unsigned long dataSize, unsigned char stagingArea)
{
    unsigned long flags = 0;
    DPNAME dpName;
    initializeDirectPlayName(dpName, groupName, groupName);
    if (stagingArea)
        flags = 0x800;
    unsigned long idGroup;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->CreateGroupInGroup(dpidParent, &idGroup, &dpName, groupData, dataSize, flags);
    return m_res >= 0 ? idGroup : 0;
}

VA(0x00497150, 0x2F)
DC_ADDRESS(0x08a378, 0x2a)
unsigned char CDPlay::destroyGroup(unsigned long groupId)
{
    if (!m_dp)
        return 0;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->DestroyGroup(groupId);
    return m_res >= 0;
}

VA(0x00497180, 0x27)
DC_ADDRESS(0x08a3a4, 0x28)
unsigned char CDPlay::deleteGroupFromGroup(unsigned long dpidParent, unsigned long dpidGroup)
{
    m_res = static_cast<IDirectPlay4A*>(m_dp)->DeleteGroupFromGroup(dpidParent, dpidGroup);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x004971b0, 0x94)
DC_ADDRESS(0x08a3cc, 0x46)
unsigned char CDPlay::enumConnections(CAutoArray<CDPlayConnection>* connectionArray)
{
    if (!m_dp)
        return 0;
    m_connectionArray = connectionArray;
    connectionArray->destroy(1);
    m_res = static_cast<IDirectPlay4A*>(m_dp)->EnumConnections(0, enumConnectionsCallback, this, 1);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00497250, 0x34)
DC_ADDRESS(0x08a414, 0x42)
unsigned char CDPlay::startSession(unsigned long groupId)
{
    if (!m_dp)
        return 0;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->StartSession(0, groupId);
    return m_res >= 0;
}

VA(0x00497290, 0x22)
DC_ADDRESS(0x08a458, 0x2a)
unsigned char CDPlay::closeSession()
{
    if (!m_dp)
        return 0;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->Close();
    return m_res >= 0;
}

VA(0x004972c0, 0x71)
DC_ADDRESS(0x08a484, 0x44)
unsigned char CDPlay::enumGroups(CAutoArray<CDPlayGroup>* groupArray, _GUID* guidInstance, unsigned long flags)
{
    m_groupArray = groupArray;
    groupArray->destroy(1);
    m_res = static_cast<IDirectPlay4A*>(m_dp)->EnumGroups(guidInstance, enumGroupsCallback, this, flags);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00497340, 0x71)
DC_ADDRESS(0x08a4c8, 0x44)
unsigned char CDPlay::enumPlayers(CAutoArray<CDPlayPlayer>* playerArray, _GUID* guidInstance, unsigned long flags)
{
    m_playerArray = playerArray;
    playerArray->destroy(1);
    m_res = static_cast<IDirectPlay4A*>(m_dp)->EnumPlayers(guidInstance, enumPlayersCallback, this, flags);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x004973c0, 0x75)
DC_ADDRESS(0x08a50c, 0x4c)
unsigned char CDPlay::enumGroupPlayers(CAutoArray<CDPlayPlayer>* playerArray, unsigned long dpidGroup, _GUID* guidInstance, unsigned long flags)
{
    m_playerArray = playerArray;
    playerArray->destroy(1);
    m_res = static_cast<IDirectPlay4A*>(m_dp)->EnumGroupPlayers(dpidGroup, guidInstance, enumPlayersCallback, this, flags);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00497440, 0xB5)
DC_ADDRESS(0x08a558, 0x84)
unsigned char CDPlay::enumSessions(CAutoArray<CDPlaySession>* sessionArray, unsigned long timeOut, unsigned long flags)
{
    if (!m_dp)
        return 0;
    m_sessionArray = sessionArray;
    m_sessionArray->destroy(1);
    DPSESSIONDESC2 desc;
    memset(&desc, 0, sizeof(desc));
    desc.m_size = sizeof(desc);
    desc.m_guidApplication = m_guid;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->EnumSessions(&desc, timeOut, enumSession, this, flags);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00497500, 0x53)
DC_ADDRESS(0x08a5dc, 0x46)
unsigned char CDPlay::sendChat(char* msg, unsigned long idFrom, unsigned long idTo)
{
    if (!m_dp)
        return 0;
    DPCHAT chat;
    chat.m_size = sizeof(DPCHAT);
    chat.m_flags = 0;
    chat.m_messageA = msg;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->SendChatMessage(idFrom, idTo, 0, &chat);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00497560, 0x48)
DC_ADDRESS(0x08a624, 0x54)
unsigned char CDPlay::send(void* data, unsigned long size, unsigned long idFrom, unsigned long idTo, unsigned char guaranteed)
{
    if (!m_dp)
        return 0;
    unsigned long flags = 1;
    if (!guaranteed)
        flags = 0;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->Send(idFrom, idTo, flags, data, size);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x004975b0, 0xF3)
DC_ADDRESS(0x08a678, 0xcc)
unsigned char CDPlay::receive(unsigned long* fromID, unsigned long* toID, CDPlayMsg* msg, unsigned long flags)
{
    if (!m_dp)
        return 0;
    unsigned long buffSize = msg->m_dataSize;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->Receive(fromID, toID, flags, msg->m_data, &buffSize);
    for (;;) {
        if (m_res == DPERR_NOMESSAGES)
            return 0;
        if (m_res != DPERR_BUFFERTOOSMALL)
            break;
        msg->allocSize(buffSize);
        if (m_res != DPERR_BUFFERTOOSMALL)
            break;
        m_res = static_cast<IDirectPlay4A*>(m_dp)->Receive(fromID, toID, flags, msg->m_data, &buffSize);
    }
    if (m_res < 0)
        return 0;
    if (!*fromID)
        return receiveSystemMsg(*toID, msg);
    return receiveMsg(*fromID, *toID, msg);
}

VA(0x004976b0, 0xDC)
DC_ADDRESS(0x08a744, 0x9c)
unsigned char CDPlay::flushReceiveQueue()
{
    CDPlayMsg msg;
    unsigned long from;
    unsigned long to;
    unsigned long buffSize;
    buffSize = 0;
    while (1) {
        m_res = static_cast<IDirectPlay4A*>(m_dp)->Receive(
            &from, &to, 1, msg.m_data, &buffSize);
        if (m_res == DPERR_NOMESSAGES)
            return 1;
        if (m_res == DPERR_BUFFERTOOSMALL)
            msg.allocSize(buffSize);
        if (m_res != DPERR_BUFFERTOOSMALL && m_res != 0)
            break;
    }
    if (m_res < 0)
        return 0;
    return 1;
}

VA(0x004977c0, 0x144)
DC_ADDRESS(0x08a7e0, 0x48)
unsigned char CDPlay::addSessionEnum(const DPSESSIONDESC2* dpSessionDesc, unsigned long flags)
{
    if (flags & 1)
        return 0;
    CDPlaySession* session = new CDPlaySession(dpSessionDesc);
    m_sessionArray->add(session);
    return 1;
}

VA(0x00497910, 0x180)
DC_ADDRESS(0x08a828, 0x1a2)
unsigned char CDPlay::receiveSystemMsg(unsigned long toID, CDPlayMsg* msg)
{
    DPMSG_GENERIC* generic = static_cast<DPMSG_GENERIC*>(static_cast<void*>(msg->m_data));
    unsigned long messageType = msg->getId();
    switch (messageType) {
    case DPSYS_ADDGROUPTOGROUP:
        return sysMsgAddGroupToGroup(static_cast<DPMSG_ADDGROUPTOGROUP*>(static_cast<void*>(generic)), toID);
    case DPSYS_CHAT:
        return sysMsgChat(static_cast<DPMSG_CHAT*>(static_cast<void*>(generic)), toID);
    case DPSYS_DELETEGROUPFROMGROUP:
        return sysMsgDeleteGroupFromGroup(static_cast<DPMSG_ADDGROUPTOGROUP*>(static_cast<void*>(generic)), toID);
    case DPSYS_SECUREMESSAGE:
        return sysMsgSecureMessage(static_cast<DPMSG_SECUREMESSAGE*>(static_cast<void*>(generic)), toID);
    case DPSYS_SETSESSIONDESC:
        return sysMsgSetSessionDesc(static_cast<DPMSG_SETSESSIONDESC*>(static_cast<void*>(generic)), toID);
    case DPSYS_STARTSESSION:
        return sysMsgStartSession(static_cast<DPMSG_STARTSESSION*>(static_cast<void*>(generic)), toID);
    case DPSYS_CREATEPLAYERORGROUP:
        return sysMsgCreatePlayerOrGroup(static_cast<DPMSG_CREATEPLAYERORGROUP*>(static_cast<void*>(generic)), toID);
    case DPSYS_DESTROYPLAYERORGROUP:
        return sysMsgDestroyPlayerOrGroup(static_cast<DPMSG_DESTROYPLAYERORGROUP*>(static_cast<void*>(generic)), toID);
    case DPSYS_ADDPLAYERTOGROUP:
        return sysMsgAddPlayerToGroup(static_cast<DPMSG_ADDPLAYERTOGROUP*>(static_cast<void*>(generic)), toID);
    case DPSYS_DELETEPLAYERFROMGROUP:
        return sysMsgDeletePlayerFromGroup(static_cast<DPMSG_ADDPLAYERTOGROUP*>(static_cast<void*>(generic)), toID);
    case DPSYS_SESSIONLOST:
        return sysMsgSessionLost(generic, toID);
    case DPSYS_HOST:
        return sysMsgHost(generic, toID);
    case DPSYS_SETPLAYERORGROUPDATA:
        return sysMsgSetPlayerOrGroupData(static_cast<DPMSG_SETPLAYERORGROUPDATA*>(static_cast<void*>(generic)), toID);
    case DPSYS_SETPLAYERORGROUPNAME:
        return sysMsgSetPlayerOrGroupName(static_cast<DPMSG_SETPLAYERORGROUPNAME*>(static_cast<void*>(generic)), toID);
    }
    return 1;
}

VA(0x00497a90, 0x6B)
DC_ADDRESS(0x08a9cc, 0x46)
unsigned char CDPlay::addGroupEnum(unsigned long dpid, const DPNAME* name, unsigned long flags)
{
    CDPlayGroup* group = new CDPlayGroup(name->m_shortNameA, dpid);
    m_groupArray->add(group);
    return 1;
}

VA(0x00497b00, 0x6B)
DC_ADDRESS(0x08aa14, 0x46)
unsigned char CDPlay::addPlayerEnum(unsigned long dpid, const DPNAME* name, unsigned long flags)
{
    CDPlayPlayer* player = new CDPlayPlayer(name->m_shortNameA, dpid);
    m_playerArray->add(player);
    return 1;
}

VA(0x00497b70, 0xDF)
DC_ADDRESS(0x08aa5c, 0x78)
unsigned char CDPlay::addConnectionEnum(const GUID* lpguidSP, void* connection, unsigned long connectionSize, const DPNAME* name, unsigned long flags)
{
    CDPlayConnection* conn = new CDPlayConnection(lpguidSP, connectionSize,
        connection, name->m_shortNameA);
    m_connectionArray->add(conn);
    return 1;
}

VA(0x00497c50, 0x57B)
DC_ADDRESS(0x08aad4, 0x468)
void CDPlay::getErrorDesc(long error, char* descriptionOut)
{
    const char* description =
        DATA_COMPGEN(0x006776a0, dplayUnknownErrorText, "Unknown error?");

    switch (error) {
    case DPERR_ALREADYINITIALIZED:
        description = DATA_COMPGEN(0x00677660, dplayAlreadyInitializedText, "Already initialized");
        break;
    case DPERR_ACCESSDENIED:
        description = DATA_COMPGEN(0x00677628, dplayAccessDeniedText, "Access Denied");
        break;
    case DPERR_ACTIVEPLAYERS:
        description = DATA_COMPGEN(0x00677618, dplayActivePlayersText, "Active Players");
        break;
    case DPERR_BUFFERTOOSMALL:
        description = DATA_COMPGEN(0x00677604, dplayBufferTooSmallText, "Buffer too small");
        break;
    case DPERR_CANTADDPLAYER:
        description = DATA_COMPGEN(0x006775f4, dplayCantAddPlayerText, "Cant add player");
        break;
    case DPERR_CANTCREATEGROUP:
        description = DATA_COMPGEN(0x006775e0, dplayCantCreateGroupText, "Cant create group");
        break;
    case DPERR_CANTCREATEPLAYER:
        description = DATA_COMPGEN(0x006775cc, dplayCantCreatePlayerText, "Cant create player");
        break;
    case DPERR_CANTCREATESESSION:
        description = DATA_COMPGEN(0x006775b8, dplayCantCreateSessionText, "Cant create session");
        break;
    case DPERR_CAPSNOTAVAILABLEYET:
        description = DATA_COMPGEN(0x006775a0, dplayCapsNotAvailableYetText, "Caps not available yet");
        break;
    case DPERR_EXCEPTION:
        description = DATA_COMPGEN(0x00677594, dplayExceptionText, "Exception");
        break;
    case DPERR_GENERIC:
        description = DATA_COMPGEN(0x00677674, dplayGenericText, "Generic");
        break;
    case DPERR_INVALIDFLAGS:
        description = DATA_COMPGEN(0x00677584, dplayInvalidFlagsText, "Invalid flags");
        break;
    case DPERR_INVALIDOBJECT:
        description = DATA_COMPGEN(0x00677574, dplayInvalidObjectText, "Invalid object");
        break;
    case DPERR_INVALIDPARAM:
        description = DATA_COMPGEN(0x00677648, dplayInvalidParametersText, "Invalid parameter(s)");
        break;
    case DPERR_INVALIDPLAYER:
        description = DATA_COMPGEN(0x00677564, dplayInvalidPlayerText, "Invalid player");
        break;
    case DPERR_INVALIDGROUP:
        description = DATA_COMPGEN(0x00677554, dplayInvalidGroupText, "Invalid group");
        break;
    case DPERR_NOCAPS:
        description = DATA_COMPGEN(0x0067754c, dplayNoCapsText, "No caps");
        break;
    case DPERR_NOCONNECTION:
        description = DATA_COMPGEN(0x0067753c, dplayNoConnectionText, "No connection");
        break;
    case DPERR_NOMEMORY:
        description = DATA_COMPGEN(0x00677638, dplayOutOfMemoryText, "Out of memory");
        break;
    case DPERR_NOMESSAGES:
        description = DATA_COMPGEN(0x00677530, dplayNoMessagesText, "No messages");
        break;
    case DPERR_NONAMESERVERFOUND:
        description = DATA_COMPGEN(0x00677518, dplayNoNameServerFoundText, "No name server found");
        break;
    case DPERR_NOPLAYERS:
        description = DATA_COMPGEN(0x0067750c, dplayNoPlayersText, "No players");
        break;
    case DPERR_NOSESSIONS:
        description = DATA_COMPGEN(0x00677500, dplayNoSessionsText, "No sessions");
        break;
    case DPERR_PENDING:
        description = DATA_COMPGEN(0x0067767c, dplayPendingText, "Pending");
        break;
    case DPERR_SENDTOOBIG:
        description = DATA_COMPGEN(0x006774f4, dplaySendToBigText, "Send to big");
        break;
    case DPERR_TIMEOUT:
        description = DATA_COMPGEN(0x006774ec, dplayTimeoutText, "Timeout");
        break;
    case DPERR_UNAVAILABLE:
        description = DATA_COMPGEN(0x006774e0, dplayUnavailableText, "Unavailable");
        break;
    case DPERR_UNSUPPORTED:
        description = DATA_COMPGEN(0x00677684, dplayUnsupportedText, "Unsupported");
        break;
    case DPERR_BUSY:
        description = DATA_COMPGEN(0x006774d8, dplayBusyText, "Busy");
        break;
    case DPERR_USERCANCEL:
        description = DATA_COMPGEN(0x006774cc, dplayUserCancelText, "User cancel");
        break;
    case DPERR_NOINTERFACE:
        description = DATA_COMPGEN(0x00677690, dplayNoInterfaceText, "No interface");
        break;
    case DPERR_CANNOTCREATESERVER:
        description = DATA_COMPGEN(0x006774b4, dplayCannotCreateServerText, "Cannot create server");
        break;
    case DPERR_PLAYERLOST:
        description = DATA_COMPGEN(0x006774a8, dplayPlayerLostText, "Player lost");
        break;
    case DPERR_SESSIONLOST:
        description = DATA_COMPGEN(0x00677498, dplaySessionLostText, "Session lost");
        break;
    case DPERR_UNINITIALIZED:
        description = DATA_COMPGEN(0x00677488, dplayUninitializedText, "Uninitialized");
        break;
    case DPERR_NONEWPLAYERS:
        description = DATA_COMPGEN(0x00677478, dplayNoNewPlayersText, "No new players");
        break;
    case DPERR_INVALIDPASSWORD:
        description = DATA_COMPGEN(0x00677464, dplayInvalidPasswordText, "Invalid password");
        break;
    case DPERR_CONNECTING:
        description = DATA_COMPGEN(0x00677430, dplayConnectingText, "Connecting");
        break;
    case DPERR_BUFFERTOOLARGE:
        description = DATA_COMPGEN(0x0067743c, dplayBufferTooLargeText, "Buffer too large");
        break;
    case DPERR_CANTCREATEPROCESS:
        description = DATA_COMPGEN(0x00677450, dplayCantCreateProcessText, "Cant create process");
        break;
    case DPERR_APPNOTSTARTED:
        description = DATA_COMPGEN(0x00677420, dplayAppNotStartedText, "App not started");
        break;
    case DPERR_INVALIDINTERFACE:
        description = DATA_COMPGEN(0x006773f8, dplayInvalidInterfaceText, "Invalid interface");
        break;
    case DPERR_NOSERVICEPROVIDER:
        description = DATA_COMPGEN(0x0067740c, dplayNoServiceProviderText, "No service provider");
        break;
    case DPERR_UNKNOWNAPPLICATION:
        description = DATA_COMPGEN(0x006773e4, dplayUnknownApplicationText, "Unknown application");
        break;
    case DPERR_NOTLOBBIED:
        description = DATA_COMPGEN(0x006773ac, dplayNotLobbiedText, "Not lobbied");
        break;
    case DPERR_SERVICEPROVIDERLOADED:
        description = DATA_COMPGEN(0x006773b8, dplayServiceProviderLoadedText, "Service provider loaded");
        break;
    case DPERR_ALREADYREGISTERED:
        description = DATA_COMPGEN(0x006773d0, dplayAlreadyRegisteredText, "Already registered");
        break;
    case DPERR_NOTREGISTERED:
        description = DATA_COMPGEN(0x0067739c, dplayNotRegisteredText, "Not registered");
        break;
    case DPERR_AUTHENTICATIONFAILED:
        description = DATA_COMPGEN(0x00677374, dplayAuthenticationFailedText, "Authentication failed");
        break;
    case DPERR_CANTLOADSSPI:
        description = DATA_COMPGEN(0x0067738c, dplayCantLoadSspiText, "Cant load sspi");
        break;
    case DPERR_ENCRYPTIONFAILED:
        description = DATA_COMPGEN(0x00677360, dplayEncryptionFailedText, "Encryption failed");
        break;
    case DPERR_SIGNFAILED:
        description = DATA_COMPGEN(0x00677354, dplaySignFailedText, "Sign failed");
        break;
    case DPERR_CANTLOADSECURITYPACKAGE:
        description = DATA_COMPGEN(0x00677338, dplayCantLoadSecurityPackageText, "Cant load security package");
        break;
    case DPERR_ENCRYPTIONNOTSUPPORTED:
        description = DATA_COMPGEN(0x0067731c, dplayEncryptionNotSupportedText, "Encryption not supported");
        break;
    case DPERR_CANTLOADCAPI:
        description = DATA_COMPGEN(0x0067730c, dplayCantLoadCapiText, "Cant load capi");
        break;
    case DPERR_NOTLOGGEDIN:
        description = DATA_COMPGEN(0x006772fc, dplayNotLoggedInText, "Not logged in");
        break;
    case DPERR_LOGONDENIED:
        description = DATA_COMPGEN(0x006772ec, dplayLogonDeniedText, "Logon denied");
        break;
    }

    strcpy(descriptionOut, description);
}

VA(0x004981d0, 0x8)
DC_ADDRESS(0x08af3c, 0xa)
unsigned char CDPlay::sysMsgHost(DPMSG_GENERIC* sysMsg, unsigned long toID)
{
    m_isHost = 1;
    return 1;
}

VA(0x004981e0, 0x5)
DC_ADDRESS(0x08af48, 0x4)
unsigned char CDPlay::sysMsgCreatePlayerOrGroup(DPMSG_CREATEPLAYERORGROUP* sysMsg, unsigned long toID)
{
    return 1;
}

// Original: CDPlay::SysMsgDestroyPlayerOrGroup; dxplay.cpp:890
// Both base/lobby vtables slot57 fold this ordinary return-true body to
// the neighboring CreatePlayerOrGroup implementation0x4981e0.
DC_ADDRESS(0x08af4c, 0x4)
unsigned char CDPlay::sysMsgDestroyPlayerOrGroup(
    DPMSG_DESTROYPLAYERORGROUP* sysMsg, unsigned long toID)
{
    return 1;
}

VA(0x004981f0, 0x24)
DC_ADDRESS(0x08af50, 0x26)
unsigned char CDPlay::addPlayerToGroup(unsigned long groupId, unsigned long playerId)
{
    m_res = static_cast<IDirectPlay4A*>(m_dp)->AddPlayerToGroup(groupId, playerId);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00498220, 0x24)
DC_ADDRESS(0x08af78, 0x120)
unsigned char CDPlay::deletePlayerFromGroup(unsigned long groupId, unsigned long playerId)
{
    m_res = static_cast<IDirectPlay4A*>(m_dp)->DeletePlayerFromGroup(groupId, playerId);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00498250, 0x4D)
DC_ADDRESS(0x08b098, 0x40)
unsigned char CDPlay::setPlayerName(unsigned long playerId, char* shortName, char* longName, unsigned long flags)
{
    char* longValue = longName;
    if (!longValue)
        longValue = shortName;
    DPNAME dpName;
    initializeDirectPlayName(dpName, shortName, longValue);
    m_res = static_cast<IDirectPlay4A*>(m_dp)->SetPlayerName(playerId, &dpName, flags);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x004982a0, 0x115)
DC_ADDRESS(0x08b0d8, 0xd8)
unsigned char CDPlay::getPlayerName(unsigned long playerId, char* shortName, int maxShort, char* longName, int maxLong)
{
    CDPlayMsg name;
    unsigned long size = 0;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetPlayerName(playerId, 0, &size);
    if (m_res != DPERR_BUFFERTOOSMALL)
        return 0;
    unsigned long allocSize = size + 1;
    name.m_data = new unsigned char[allocSize];
    name.m_dataSize = allocSize;
    DPNAME* dpName = static_cast<DPNAME*>(static_cast<void*>(name.m_data));
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetPlayerName(playerId, dpName, &size);
    if (m_res < 0)
        return 0;
    copyDirectPlayName(*dpName, shortName, maxShort, longName, maxLong);
    return 1;
}

VA(0x004983c0, 0x2C)
DC_ADDRESS(0x08b1b0, 0x30)
unsigned char CDPlay::setGroupData(unsigned long groupId, void* data, unsigned long dataSize, unsigned long flags)
{
    m_res = static_cast<IDirectPlay4A*>(m_dp)->SetGroupData(groupId, data, dataSize, flags);
    unsigned char ok = m_res >= 0;
    return ok;
}

// Project-inferred shared two-query protocol. Re-read the COM interface for
// the second call and publish size only on the original successful exits.
void* CDPlay::getPlayerOrGroupData(EDPlayerType type, unsigned long id,
                                  unsigned long* pdwSize, unsigned long flags)
{
    void* buf = 0;
    unsigned long dataSize = 0;
    if (pdwSize)
        dataSize = *pdwSize;
    if (type == DPPLAYERTYPE_GROUP)
        m_res = static_cast<IDirectPlay4A*>(m_dp)->GetGroupData(id, 0, &dataSize, flags);
    else
        m_res = static_cast<IDirectPlay4A*>(m_dp)->GetPlayerData(id, 0, &dataSize, flags);
    if (m_res < 0) {
        if (m_res != DPERR_BUFFERTOOSMALL)
            return 0;
        m_res = 0;
        if (dataSize == 0)
            return 0;
        buf = ::operator new(dataSize);
        if (type == DPPLAYERTYPE_GROUP)
            m_res = static_cast<IDirectPlay4A*>(m_dp)->GetGroupData(id, buf, &dataSize, flags);
        else
            m_res = static_cast<IDirectPlay4A*>(m_dp)->GetPlayerData(id, buf, &dataSize, flags);
        if (m_res < 0) {
            ::operator delete(buf);
            return 0;
        }
    }
    if (pdwSize)
        *pdwSize = dataSize;
    return buf;
}

VA(0x004983f0, 0xB1)
DC_ADDRESS(0x08b1e0, 0xa4)
void* CDPlay::getGroupData(unsigned long groupId, unsigned long* pdwSize, unsigned long flags)
{
    return getPlayerOrGroupData(DPPLAYERTYPE_GROUP, groupId, pdwSize, flags);
}

VA(0x004984b0, 0x4D)
DC_ADDRESS(0x08b284, 0x40)
unsigned char CDPlay::setGroupName(unsigned long groupId, char* shortName, char* longName, unsigned long flags)
{
    char* longValue = longName;
    if (!longValue)
        longValue = shortName;
    DPNAME dpName;
    initializeDirectPlayName(dpName, shortName, longValue);
    m_res = static_cast<IDirectPlay4A*>(m_dp)->SetGroupName(groupId, &dpName, flags);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00498500, 0x115)
DC_ADDRESS(0x08b2c4, 0xf6)
unsigned char CDPlay::getGroupName(unsigned long groupId, char* shortName, int maxShort, char* longName, int maxLong)
{
    CDPlayMsg name;
    unsigned long size = 0;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetGroupName(groupId, 0, &size);
    if (m_res != DPERR_BUFFERTOOSMALL)
        return 0;
    unsigned long allocSize = size + 1;
    name.m_data = new unsigned char[allocSize];
    name.m_dataSize = allocSize;
    DPNAME* dpName = static_cast<DPNAME*>(static_cast<void*>(name.m_data));
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetGroupName(groupId, dpName, &size);
    if (m_res < 0)
        return 0;
    copyDirectPlayName(*dpName, shortName, maxShort, longName, maxLong);
    return 1;
}

VA(0x00498620, 0x2C)
DC_ADDRESS(0x08b3bc, 0x30)
unsigned char CDPlay::setPlayerData(unsigned long playerId, void* data, unsigned long dataSize, unsigned long flags)
{
    m_res = static_cast<IDirectPlay4A*>(m_dp)->SetPlayerData(playerId, data, dataSize, flags);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00498650, 0xB1)
DC_ADDRESS(0x08b3ec, 0xa8)
void* CDPlay::getPlayerData(unsigned long playerId, unsigned long* pdwSize, unsigned long flags)
{
    return getPlayerOrGroupData(DPPLAYERTYPE_PLAYER, playerId, pdwSize, flags);
}

VA(0x00498710, 0x8C)
DC_ADDRESS(0x08b494, 0x88)
unsigned char* CDPlay::getPlayerAddress(unsigned long dpid, unsigned long* sizeOut)
{
    unsigned long size = 0;
    if (!m_dp)
        return 0;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetPlayerAddress(dpid, 0, &size);
    if (size == 0)
        return 0;
    if (sizeOut)
        *sizeOut = size;
    unsigned char* buf = static_cast<unsigned char*>(::operator new(size));
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetPlayerAddress(dpid, buf, &size);
    if (m_res < 0) {
        ::operator delete(buf);
        return 0;
    }
    return buf;
}

VA(0x004987a0, 0x42)
DC_ADDRESS(0x08b51c, 0x48)
unsigned char CDPlay::getCaps(DPCAPS* dpCaps, unsigned char guaranteed)
{
    memset(dpCaps, 0, sizeof(DPCAPS));
    dpCaps->m_size = sizeof(DPCAPS);
    unsigned long flags = 0;
    if (guaranteed)
        flags = 1;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetCaps(dpCaps, flags);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x004987f0, 0x31)
DC_ADDRESS(0x08b564, 0x4)
unsigned char CDPlay::getSendQueueSize(unsigned long from, unsigned long to, unsigned long* numMsgs, unsigned long* numBytes)
{
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetMessageQueue(from, to, 1, numMsgs, numBytes);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00498830, 0x31)
DC_ADDRESS(0x08b568, 0x4)
unsigned char CDPlay::getReceiveQueueSize(unsigned long from, unsigned long to, unsigned long* numMsgs, unsigned long* numBytes)
{
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetMessageQueue(from, to, 2, numMsgs, numBytes);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00498870, 0x82)
DC_ADDRESS(0x08b56c, 0x5c)
CDPlayLobby::CDPlayLobby()
{
    m_lobby = 0;
    m_addressArray = 0;
}

VA_COMPGEN(0x00498900, 0x21, SCALAR_DELETING_DTOR, CDPlayLobby)

VA(0x00498930, 0x62)
DC_ADDRESS(0x08b5c8, 0x42)
CDPlayLobby::~CDPlayLobby()
{
    if (m_lobby)
        static_cast<IDirectPlayLobby3A*>(m_lobby)->Release();
}

VA(0x004989a0, 0xB7)
DC_ADDRESS(0x08b60c, 0x4)
unsigned char CDPlayLobby::registerApp(char* appName, char* fileName, char* commandLine, GUID appGuid, char* executableName)
{
    if (!m_lobby)
        return 0;
    char curDir[0x105];
    DPAPPLICATIONDESC desc;
    desc.m_size = sizeof(desc);
    desc.m_flags = 0;
    desc.m_applicationNameA = appName;
    desc.m_descriptionA = 0;
    desc.m_descriptionW = 0;
    if (!GetCurrentDirectoryA(0x105, curDir))
        return 0;
    desc.m_pathA = curDir;
    desc.m_currentDirectoryA = curDir;
    desc.m_filenameA = fileName;
    desc.m_guidApplication = appGuid;
    desc.m_commandLineA = commandLine;
    desc.m_executableA = executableName;
    m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->RegisterApplication(0, &desc);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00498a60, 0x72)
DC_ADDRESS(0x08b610, 0x4)
unsigned char CDPlayLobby::init()
{
    if (!CDPlay::init())
        return 0;
    if (m_lobby) {
        static_cast<IDirectPlayLobby3A*>(m_lobby)->Release();
        m_lobby = 0;
    }
    m_res = CoCreateInstance(CLSID_DirectPlayLobby, 0, CLSCTX_INPROC_SERVER,
        IID_IDirectPlayLobby3A, &m_lobby);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00498ae0, 0x8C)
DC_ADDRESS(0x08b614, 0x88)
DPLCONNECTION* CDPlayLobby::getConnectionSettings(unsigned long appId, unsigned long* sizeOut)
{
    unsigned long size = 0;
    if (!m_lobby)
        return 0;
    m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->GetConnectionSettings(appId, 0, &size);
    if (sizeOut)
        *sizeOut = size;
    if (size == 0)
        return 0;
    DPLCONNECTION* buf = static_cast<DPLCONNECTION*>(::operator new(size));
    m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->GetConnectionSettings(appId, buf, &size);
    if (m_res < 0) {
        ::operator delete(buf);
        return 0;
    }
    return buf;
}

// Residual (88.4210%): the two-call probe is byte-right; retail's final
// `m_hRes >= 0` is a jge to separate return-1/return-0 epilogues, while our
// CL folds either polarity to `setge al`. `why-branch` measures 57/3/4
// instructions/branches/returns in retail versus 51/2/3 here, classifies the
// residual as D6 plus D8/D13 branchless folding, and finds no catalog lever.
// A 24-state batch of early/nested/single-pass exits, byte-result forms,
// and separate GlobalAlloc/GlobalLock lifetimes also leaves 88.4211% best.
// A named result initialized to zero and assigned in the success arm is a
// negative control: it falls to 85.33% without recovering the extra exit.
// A 24-source return-width/assignment/label batch stays at 88.4211%; a
// further 17 nested-guard and scoped-return forms also fail to improve it.
// The unchanged baseline is retained; scope/goto variants score at most
// 77.5439% and do not reproduce retail's fourth return path.
// Restoring Complete's public bool return type is byte-flat at 88.4211%,
// including both remote.cpp callers; it preserves the branch-folding residual.
// Sixteen connection-validity result controls (two reproduced objects) are
// also flat at 88.4210%: returning the already validated buffer, with implicit
// or explicit bool conversion, does not restore the branch. Both final test
// polarities and void*/DPLCONNECTION* buffer types preserve all 86 exact
// siblings. The missing separate exits remain unexplained.
// Eight native-bool literal/polarity controls under the restored signature
// emit one reproduced object: true/false versus integer return constants
// leaves the same branchless result and all sibling scores unchanged.
// DC compiles 1352..1379 out; GHND 0x42 plus GlobalLock is windowsx.h's
// GlobalAllocPtr. The DirectPlay-sample goto FAILURE spelling with a
// FAILED(hr) test falls to 54.30% (2026-09-29).
// E:\gamedcs\dxplay.cpp:1351
VA(0x00498b70, 0x6E)  // anchor-callee IDirectPlayLobby::GetConnectionSettings probe + GlobalAlloc/GlobalLock; ret 0, src-order
DC_ADDRESS(0x08b69c, 0x4)
bool CDPlayLobby::testLobbied()
{
    unsigned long size;
    m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->GetConnectionSettings(0, 0, &size);
    if (m_res != DPERR_BUFFERTOOSMALL)
        return 0;
    void* buf = GlobalLock(GlobalAlloc(0x42, size));
    if (!buf)
        return 0;
    m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->GetConnectionSettings(0, buf, &size);
    if (m_res >= 0)
        return 1;
    return 0;
}

VA(0x00498be0, 0x31)
DC_ADDRESS(0x08b6a0, 0x3a)
unsigned char CDPlayLobby::setConnectionSettings(unsigned long appId, DPLCONNECTION* connection)
{
    if (!m_lobby)
        return 0;
    m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->SetConnectionSettings(0, appId, connection);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00498c20, 0x29)
DC_ADDRESS(0x08b6dc, 0x2c)
unsigned char CDPlayLobby::setGroupConnectionSettings(unsigned long dpidGroup, DPLCONNECTION* connection)
{
    m_res = static_cast<IDirectPlay4A*>(m_dp)->SetGroupConnectionSettings(0, dpidGroup, connection);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00498c50, 0x80)
DC_ADDRESS(0x08b708, 0x76)
DPLCONNECTION* CDPlayLobby::getGroupConnectionSettings(unsigned long dpidGroup)
{
    unsigned long size = 0;
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetGroupConnectionSettings(0, dpidGroup, 0, &size);
    if (size == 0)
        return 0;
    DPLCONNECTION* buf = static_cast<DPLCONNECTION*>(::operator new(size));
    m_res = static_cast<IDirectPlay4A*>(m_dp)->GetGroupConnectionSettings(0, dpidGroup, buf, &size);
    if (m_res < 0) {
        ::operator delete(buf);
        return 0;
    }
    return buf;
}

VA(0x00498cd0, 0xAB)
DC_ADDRESS(0x08b780, 0x88)
unsigned char CDPlayLobby::connect()
{
    DPLCONNECTION* conn = getConnectionSettings(0, 0);
    if (conn->m_flags & DPLAY_CONNECTION_CREATE_SESSION)
        m_isHost = 1;
    else
        m_isHost = 0;
    ::operator delete(conn);
    releaseDirectPlay();
    m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->ConnectEx(0, IID_IDirectPlay4A, &m_dp, 0);
    unsigned char ok = m_res >= 0;
    return ok;
}

// Original: CDPlayLobby::SendStandardLobbyMsg; dxplay.cpp:1476
// The ordinary lobby-message APIs use the same interface and HRESULT state
// as Complete's retained connection-setting wrappers. DC1478/1479 proves
// the null-lobby guard and SendLobbyMessage flags2. They have no separately
// claimed retail entries or invented callers.
DC_ADDRESS(0x08b808, 0x5c)
unsigned char CDPlayLobby::sendStandardLobbyMsg(
    unsigned long appId, void* data, unsigned long size)
{
    if (!m_lobby)
        return 0;
    m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->SendLobbyMessage(
        DPLMSG_STANDARD, appId, data, size);
    if (m_res < 0)
        return 0;
    return 1;
}

// Original: CDPlayLobby::SendLobbyMsg; dxplay.cpp:1490
DC_ADDRESS(0x08b864, 0x42)
unsigned char CDPlayLobby::sendLobbyMsg(
    unsigned long appId, void* data, unsigned long size)
{
    if (!m_lobby)
        return 0;
    m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->SendLobbyMessage(
        0, appId, data, size);
    if (m_res < 0)
        return 0;
    return 1;
}

// Original: CDPlayLobby::ReceiveLobbyMsg; dxplay.cpp:1503
// DC1511..1521 retries after growing CDPlayMsg's buffer. DC1525/1526
// dispatches SYSTEM/STANDARD messages and negates the handler result.
DC_ADDRESS(0x08b8a8, 0xa6)
unsigned char CDPlayLobby::receiveLobbyMsg(unsigned long appId, CDPlayMsg* msg)
{
    unsigned long flags;
    if (!m_lobby)
        return 0;
    do {
        m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->ReceiveLobbyMessage(
            0, appId, &flags, msg->m_data, &msg->m_dataSize);
        if (m_res == DPERR_BUFFERTOOSMALL)
            msg->allocSize(msg->m_dataSize);
        else if (m_res < 0)
            return 0;
    } while (m_res == DPERR_BUFFERTOOSMALL);
    if (flags == DPLMSG_SYSTEM || flags == DPLMSG_STANDARD)
        return !handleSystemLobbyMsg(appId, msg);
    return 1;
}

// Project-inferred borrowed address-record setup. The SDK record stays a
// plain value; the factory or enumeration container owns the pointed-to data.
static void initializeDirectPlayAddressElement(DPCOMPOUNDADDRESSELEMENT& element,
                                              const GUID& type,
                                              unsigned long size,
                                              const void* data)
{
    element.m_guidDataType = type;
    element.m_dataSize = size;
    element.m_data = data;
}

static void copyDirectPlayAddressElements(DPCOMPOUNDADDRESSELEMENT* elements,
                                         CAutoArray<CDPlayAddressElement>& addresses,
                                         unsigned long& count)
{
    while (count < addresses.getCount()) {
        CDPlayAddressElement* element = addresses.get(count);
        initializeDirectPlayAddressElement(elements[count], element->m_guid,
                                           element->m_dataSize, element->m_data);
        ++count;
    }
}

// Project-inferred common tail of the four transport factories. Borrowed
// element data stays alive in each caller until the connection copy is made.
CDPlayConnection* CDPlayLobby::createConnectionFromElements(
    const DPCOMPOUNDADDRESSELEMENT* elements, unsigned long count,
    const GUID* provider, char* name)
{
    unsigned long addressSize = 0;
    m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->CreateCompoundAddress(
        elements, count, 0, &addressSize);
    if (m_res != DPERR_BUFFERTOOSMALL)
        return 0;
    void* address = ::operator new(addressSize);
    m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->CreateCompoundAddress(
        elements, count, address, &addressSize);
    if (m_res < 0) {
        ::operator delete(address);
        return 0;
    }
    CDPlayConnection* connection = new CDPlayConnection(
        provider, addressSize, address, name);
    ::operator delete(address);
    return connection;
}

VA(0x00498d80, 0x3C9)
DC_ADDRESS(0x08b950, 0x4)
CDPlayConnection* CDPlayLobby::createTCPIPConnection(
    char* ipAddress, char* name, CDPlayConnection* append)
{
    DPCOMPOUNDADDRESSELEMENT elements[10];
    CAutoArray<CDPlayAddressElement> addresses;
    unsigned long count = 0;
    if (append) {
        if (!enumAddress(append->m_connection, append->m_size, &addresses))
            return 0;
        copyDirectPlayAddressElements(elements, addresses, count);
    }
    initializeDirectPlayAddressElement(elements[count], DPAID_ServiceProvider,
                                       sizeof(GUID), &DPSPGUID_TCPIP);
    ++count;
    if (ipAddress) {
        initializeDirectPlayAddressElement(elements[count], DPAID_INet,
                                           strlen(ipAddress) + 1, ipAddress);
        ++count;
    }
    return createConnectionFromElements(elements, count, &DPSPGUID_TCPIP, name);
}

VA(0x00499150, 0x356)
DC_ADDRESS(0x08b954, 0x4)
CDPlayConnection* CDPlayLobby::createIPXConnection(char* name, CDPlayConnection* connAppend)
{
    DPCOMPOUNDADDRESSELEMENT elements[10];
    unsigned long count = 0;
    CAutoArray<CDPlayAddressElement> addresses;
    if (connAppend) {
        if (!enumAddress(connAppend->m_connection, connAppend->m_size, &addresses))
            return 0;
        copyDirectPlayAddressElements(elements, addresses, count);
    }
    initializeDirectPlayAddressElement(elements[count], DPAID_ServiceProvider,
                                       sizeof(GUID), &DPSPGUID_IPX);
    ++count;
    return createConnectionFromElements(elements, count, &DPSPGUID_IPX, name);
}

VA(0x004994b0, 0x24E)
DC_ADDRESS(0x08b958, 0x4)
CDPlayConnection* CDPlayLobby::createModemConnection(char* name, char* phoneNbr, char* modemString)
{
    DPCOMPOUNDADDRESSELEMENT elements[10];
    initializeDirectPlayAddressElement(elements[0], DPAID_ServiceProvider,
                                       sizeof(GUID), &DPSPGUID_MODEM);
    unsigned long count = 1;
    if (modemString) {
        initializeDirectPlayAddressElement(elements[1], DPAID_Modem,
                                           strlen(modemString) + 1, modemString);
        count = 2;
    }
    if (phoneNbr) {
        initializeDirectPlayAddressElement(elements[count], DPAID_Phone,
                                           strlen(phoneNbr) + 1, phoneNbr);
        ++count;
    }
    return createConnectionFromElements(elements, count, &DPSPGUID_MODEM, name);
}

VA(0x00499700, 0x1F4)
DC_ADDRESS(0x08b95c, 0x4)
CDPlayConnection* CDPlayLobby::createSerialConnection(char* name, _DPCOMPORTADDRESS* comPortInfo)
{
    DPCOMPOUNDADDRESSELEMENT elements[10];
    initializeDirectPlayAddressElement(elements[0], DPAID_ServiceProvider,
                                       sizeof(GUID), &DPSPGUID_SERIAL);
    unsigned long count = 1;
    if (comPortInfo) {
        initializeDirectPlayAddressElement(elements[1], DPAID_ComPort,
                                           0x14, comPortInfo);
        count = 2;
    }
    return createConnectionFromElements(elements, count, &DPSPGUID_SERIAL, name);
}

// Original: CDPlayLobby::HandleSystemLobbyMsg; dxplay.cpp:1802
// Complete's lobby vtable0x63dd20 slot71 folds this return-true default
// into the identical ordinary body0x4981e0 (mov al,1; ret8).
DC_ADDRESS(0x08b960, 0x4)
unsigned char CDPlayLobby::handleSystemLobbyMsg(unsigned long appId, CDPlayMsg* msg)
{
    return 1;
}

VA(0x00499900, 0x97)
DC_ADDRESS(0x08b964, 0x4)
unsigned char CDPlayLobby::enumLobbyConnections(CAutoArray<CDPlayConnection>* connectionArray)
{
    if (!m_dp)
        return 0;
    m_connectionArray = connectionArray;
    connectionArray->destroy(1);
    m_res = static_cast<IDirectPlay4A*>(m_dp)->EnumConnections(&m_guid, enumConnectionsCallback, this, 2);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x004999a0, 0x75)
DC_ADDRESS(0x08b968, 0x48)
unsigned char CDPlayLobby::enumGroupsInGroup(CAutoArray<CDPlayGroup>* groupArray, unsigned long dpidParent, unsigned long flags)
{
    m_groupArray = groupArray;
    groupArray->destroy(1);
    m_res = static_cast<IDirectPlay4A*>(m_dp)->EnumGroupsInGroup(dpidParent, 0, enumGroupsCallback, this, flags);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00499a20, 0x72)
DC_ADDRESS(0x08b9b0, 0x46)
unsigned char CDPlayLobby::enumGroupPlayers(CAutoArray<CDPlayPlayer>* playerArray, unsigned long dpidGroup, unsigned long flags)
{
    m_playerArray = playerArray;
    playerArray->destroy(1);
    m_res = static_cast<IDirectPlay4A*>(m_dp)->EnumGroupPlayers(dpidGroup, 0, enumPlayersCallback, this, flags);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00499aa0, 0x78)
DC_ADDRESS(0x08b9f8, 0x70)
unsigned char CDPlayLobby::enumGroupPlayersRemote(CAutoArray<CDPlayPlayer>* playerArray, unsigned long dpidGroup, _GUID* guidInstance, unsigned long flags)
{
    m_playerArray = playerArray;
    playerArray->destroy(1);
    m_res = static_cast<IDirectPlay4A*>(m_dp)->EnumGroupPlayers(dpidGroup, guidInstance, enumPlayersCallback, this, flags | 0x80);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00499b20, 0x89)
DC_ADDRESS(0x08ba68, 0x46)
unsigned char CDPlayLobby::enumAddress(void* conn, unsigned long size, CAutoArray<CDPlayAddressElement>* array)
{
    m_addressArray = array;
    array->destroy(1);
    m_res = static_cast<IDirectPlayLobby3A*>(m_lobby)->EnumAddress(enumAddressCallback, conn, size, this);
    unsigned char ok = m_res >= 0;
    return ok;
}

VA(0x00499bb0, 0xAA)
DC_ADDRESS(0x08bab0, 0x4c)
unsigned char CDPlayLobby::addAddressEnum(const GUID* guid, unsigned long dataSize, const void* data)
{
    CDPlayAddressElement* element = new CDPlayAddressElement(guid, data, dataSize);
    m_addressArray->add(element);
    return 1;
}

VA(0x00499c60, 0x1B8)
DC_ADDRESS(0x08bafc, 0xa6)
unsigned char CDPlayLobby::getIPAddress(unsigned long dpid, char* ipAddress)
{
    ipAddress[0] = 0;
    unsigned long size;
    unsigned char* buf = getPlayerAddress(dpid, &size);
    if (!buf)
        return 0;
    CAutoArray<CDPlayAddressElement> addresses;
    enumAddress(buf, size, &addresses);
    for (unsigned long i = 0; i < addresses.getCount(); ++i) {
        CDPlayAddressElement* elem = addresses.get(i);
        if (memcmp(&elem->m_guid, &DPAID_INet, sizeof(GUID)) == 0) {
            strcpy(ipAddress, elem->m_data);
            break;
        }
    }
    ::operator delete(buf);
    if (!ipAddress[0])
        return 0;
    return 1;
}

VA(0x00499e20, 0x23)
DC_ADDRESS(0x08bba4, 0x20)
int __stdcall enumAddressCallback(const GUID* guidDataType, unsigned long dataSize, const void* data, void* context)
{
    return static_cast<CDPlayLobby*>(context)->addAddressEnum(guidDataType, dataSize, data);
}

VA(0x00499e50, 0x1F)
DC_ADDRESS(0x08bbc4, 0x18)
int __stdcall enumSession(const DPSESSIONDESC2* dpSessionDesc, unsigned long* lpdwTimeOut, unsigned long flags, void* context)
{
    return static_cast<CDPlay*>(context)->addSessionEnum(dpSessionDesc, flags);
}

VA(0x00499e70, 0x2B)
DC_ADDRESS(0x08bbdc, 0x30)
int __stdcall enumConnectionsCallback(const GUID* lpguidSP, void* connection, unsigned long connectionSize, const DPNAME* name, unsigned long flags, void* context)
{
    return static_cast<CDPlay*>(context)->addConnectionEnum(lpguidSP, connection, connectionSize, name, flags);
}

VA(0x00499ea0, 0x23)
DC_ADDRESS(0x08bc0c, 0x1c)
int __stdcall enumGroupsCallback(unsigned long dpid, unsigned long playerType, const DPNAME* name, unsigned long flags, void* context)
{
    return static_cast<CDPlay*>(context)->addGroupEnum(dpid, name, flags);
}

VA(0x00499ed0, 0x23)
DC_ADDRESS(0x08bc28, 0x5c)
int __stdcall enumPlayersCallback(unsigned long dpid, unsigned long playerType, const DPNAME* name, unsigned long flags, void* context)
{
    return static_cast<CDPlay*>(context)->addPlayerEnum(dpid, name, flags);
}
#if 0  // @carcass -- located @stub bodies, PROVEN, in retail RVA order

VA(0x00499f00, 0x5B)  // anchor-body: stores CAutoArray vtable (0x63de44) + inlined Destroy loop, no flags param (plain dtor)
void CAutoArray<CDPlayAddressElement>::~CAutoArray<CDPlayAddressElement>()
{
    // @stub
}

VA(0x00499f60, 0x60)  // anchor-body: ret 4 (deleteData param at [ebp+8]) + Destroy loop, no vtable store; called from CreateTCPIPConnection
void CAutoArray<CDPlayAddressElement>::destroy(unsigned char deleteData)
{
    // @stub
}

VA(0x00499fc0, 0x1D)  // anchor-vtable CAutoArray<CDPlayAddressElement> slot2 (Get)
CDPlayAddressElement* CAutoArray<CDPlayAddressElement>::get(unsigned long elementNbr)
{
    // @stub
}

VA(0x00499fe0, 0x22)  // anchor-vtable CAutoArray<CDPlayAddressElement> slot3 (Put)
unsigned char CAutoArray<CDPlayAddressElement>::put(unsigned long elementNbr, CDPlayAddressElement* element)
{
    // @stub
}

VA(0x0049a010, 0x4)  // anchor-vtable CAutoArray<CDPlayAddressElement> slot6 (GetCount)
unsigned long CAutoArray<CDPlayAddressElement>::getCount()
{
    // @stub
}

// ..\stlport\stl_bvector.h:144
// Retail claim promoted to the compiler-generated body below; DC identity retained.
void* CAutoArray<CDPlayAddressElement>::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass

// CAutoArray<CDPlayAddressElement> scalar deleting destructor (vtable slot 0).
VA_COMPGEN(0x0049a020, 0x73, SCALAR_DELETING_DTOR, CAutoArray)

// TRuntimeError's retained message constructor sits after the DxPlay family.
// The physical dxplay.cpp allocation follows that retail band; RTTI proves
// the class name, while the original Windows source filename is unknown.
// Objnames' 0x41b500 expands the derived allocation-error initialization
// around a call here; gzinflatebuf retains and calls 0x4d6b80.

DATA(0x0063de60) extern const char g_allocationFailureText[] =
    "Allocation failure.";

// The body is the base list. RTTI proves the empty TDebugBreak base;
// its default constructor is declared in exceptions.h. The
VA(0x0049a0c0, 0xF9)
MAC_ADDRESS(0x2207bc, 0x94)
TRuntimeError::TRuntimeError(const char* text)
    : std::runtime_error(text)
{
}

// The retained empty-base calls at 0x41b62a and 0x514dbd reach a body
// folded with philAI::philAI at 0x524360. Its original TU is unknown.
MAC_ADDRESS(0x2207b8, 0x4)
TDebugBreak::TDebugBreak()
{
}
