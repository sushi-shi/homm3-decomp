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

// File-scope DirectPlay enumeration trampolines (defined at the tail of this TU),
// forward-declared so the Enum* wrappers above them can take their addresses.
int __stdcall EnumAddressCallback(const GUID&, unsigned long, const void*, void*);
int __stdcall enumSession(const DPSESSIONDESC2*, unsigned long*, unsigned long, void*);
int __stdcall enumConnectionsCallback(const GUID*, void*, unsigned long, const DPNAME*, unsigned long, void*);
int __stdcall enumGroupsCallback(unsigned long, unsigned long, const DPNAME*, unsigned long, void*);
int __stdcall enumPlayersCallback(unsigned long, unsigned long, const DPNAME*, unsigned long, void*);

// One-shot COM apartment guard: the CDPlay base constructor CoInitializes the
// process the first time any DirectPlay object is built. Retail's inlined copy
// in the CDPlayLobby constructor tests and sets this byte around CoInitialize
// (0x4988ca/0x4988d9).
DATA(0x006969d8) static unsigned char g_coInitialized = 0;

// E:\gamedcs\dxplay.cpp:66 - inlined into the CDPlayLobby (and CDPlayHeroes)
// constructors. The process is CoInitialized once, guarded by a file-scope
// flag.
DC_ADDRESS(0x08a074, 0x70)
CDPlay::CDPlay()
{
    m_lpDP = 0;
    m_connected = 0;
    m_inSession = 0;
    m_isHost = 0;
    m_hRes = 0;
    m_guid = GUID_NULL;
    m_pSessionArray = 0;
    m_pConnectionArray = 0;
    m_pGroupArray = 0;
    m_pPlayerArray = 0;
    memset(&m_caps, 0, sizeof(m_caps));
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
    if (m_lpDP)
        m_lpDP->Release();
}

VA(0x00496d30, 0x3A)
DC_ADDRESS(0x08a11c, 0x4)
bool CDPlay::Init()
{
    if (m_lpDP) {
        m_lpDP->Release();
        m_lpDP = 0;
    }
    m_hRes = CoCreateInstance(CLSID_DirectPlay, 0, CLSCTX_INPROC_SERVER,
        IID_IDirectPlay4A, reinterpret_cast<void**>(&m_lpDP));
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00496d70, 0x33)
DC_ADDRESS(0x08a120, 0x34)
bool CDPlay::InitConnection(CDPlayConnection* connection)
{
    if (!m_lpDP)
        return 0;
    m_hRes = m_lpDP->InitializeConnection(connection->m_connection, 0);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00496db0, 0xB0)
DC_ADDRESS(0x08a154, 0x4)
bool CDPlay::HostSession(char* sessionName, unsigned long flags,
                                  unsigned long maxPlayers, char* password)
{
    if (!m_lpDP)
        return 0;
    DPSESSIONDESC2 desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = flags;
    desc.dwMaxPlayers = maxPlayers;
    desc.lpszSessionNameA = sessionName;
    if (password)
        desc.lpszPasswordA = password;
    if (memcmp(&m_guid, &GUID_NULL, sizeof(GUID)) != 0)
        desc.guidApplication = m_guid;
    m_hRes = m_lpDP->Open(&desc, 2);
    if (m_hRes < 0)
        return 0;
    m_isHost = 1;
    GetCaps(&m_caps, 1);
    return 1;
}

VA(0x00496e60, 0xB8)
DC_ADDRESS(0x08a158, 0x4)
bool CDPlay::JoinSession(GUID* sessionGuid, char* password)
{
    if (!m_lpDP)
        return 0;
    DPSESSIONDESC2 desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.lpszPasswordA = password;
    if (sessionGuid)
        desc.guidInstance = *sessionGuid;
    if (memcmp(&m_guid, &GUID_NULL, sizeof(GUID)) != 0)
        desc.guidApplication = m_guid;
    m_hRes = m_lpDP->Open(&desc, 1);
    if (m_hRes < 0)
        return 0;
    m_isHost = 0;
    GetCaps(&m_caps, 1);
    return 1;
}

VA(0x00496f20, 0x6D)
DC_ADDRESS(0x08a15c, 0x72)
DPSESSIONDESC2* CDPlay::getCurrSession()
{
    if (!m_lpDP)
        return 0;
    unsigned long size = 0;
    m_hRes = m_lpDP->GetSessionDesc(0, &size);
    if (m_hRes != DPERR_BUFFERTOOSMALL || size == 0)
        return 0;
    DPSESSIONDESC2* buf = static_cast<DPSESSIONDESC2*>(::operator new(size));
    m_hRes = m_lpDP->GetSessionDesc(buf, &size);
    if (m_hRes < 0) {
        ::operator delete(buf);
        buf = 0;
    }
    return buf;
}

VA(0x00496f90, 0x22)
DC_ADDRESS(0x08a1d0, 0x28)
bool CDPlay::UpdateSessionDesc(DPSESSIONDESC2* sessionDesc)
{
    m_hRes = m_lpDP->SetSessionDesc(sessionDesc, 0);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00496fc0, 0x72)
DC_ADDRESS(0x08a1f8, 0x94)
unsigned long CDPlay::createPlayer(char* playerName, void* data, unsigned long size, void* event)
{
    if (!m_lpDP)
        return 0;
    unsigned long flags = 0;
    DPNAME dpName;
    memset(&dpName, 0, sizeof(dpName));
    unsigned long idPlayer;
    dpName.dwSize = sizeof(DPNAME);
    dpName.lpszShortNameA = playerName;
    if (m_isHost)
        flags = 0x100;
    m_hRes = m_lpDP->CreatePlayer(
        &idPlayer, &dpName, event, data, size, flags);
    return m_hRes >= 0 ? idPlayer : 0;
}

VA(0x00497040, 0x2B)
DC_ADDRESS(0x08a28c, 0x2e)
bool CDPlay::DestroyPlayer(unsigned long playerId)
{
    if (!m_lpDP)
        return 0;
    m_hRes = m_lpDP->DestroyPlayer(playerId);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00497070, 0x66)
DC_ADDRESS(0x08a2bc, 0x5e)
unsigned long CDPlay::CreateGroup(char* groupName, void* groupData, unsigned long groupDataSize, bool stagingArea)
{
    if (!m_lpDP)
        return 0;
    unsigned long flags = 0;
    DPNAME dpName;
    dpName.dwSize = sizeof(DPNAME);
    dpName.dwFlags = 0;
    dpName.lpszShortNameA = groupName;
    dpName.lpszLongNameA = groupName;
    if (stagingArea)
        flags = 0x800;
    unsigned long idGroup;
    m_hRes = m_lpDP->CreateGroup(&idGroup, &dpName, groupData, groupDataSize, flags);
    return m_hRes >= 0 ? idGroup : 0;
}

VA(0x004970e0, 0x62)
DC_ADDRESS(0x08a31c, 0x5c)
unsigned long CDPlay::CreateGroupInGroup(unsigned long dpidParent, char* groupName, void* groupData, unsigned long dataSize, bool stagingArea)
{
    unsigned long flags = 0;
    DPNAME dpName;
    dpName.dwSize = sizeof(DPNAME);
    dpName.dwFlags = 0;
    dpName.lpszShortNameA = groupName;
    dpName.lpszLongNameA = groupName;
    if (stagingArea)
        flags = 0x800;
    unsigned long idGroup;
    m_hRes = m_lpDP->CreateGroupInGroup(dpidParent, &idGroup, &dpName, groupData, dataSize, flags);
    return m_hRes >= 0 ? idGroup : 0;
}

VA(0x00497150, 0x2F)
DC_ADDRESS(0x08a378, 0x2a)
bool CDPlay::DestroyGroup(unsigned long groupId)
{
    if (!m_lpDP)
        return 0;
    m_hRes = m_lpDP->DestroyGroup(groupId);
    return m_hRes >= 0;
}

VA(0x00497180, 0x27)
DC_ADDRESS(0x08a3a4, 0x28)
bool CDPlay::DeleteGroupFromGroup(unsigned long dpidParent, unsigned long dpidGroup)
{
    m_hRes = m_lpDP->DeleteGroupFromGroup(dpidParent, dpidGroup);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x004971b0, 0x94)
DC_ADDRESS(0x08a3cc, 0x46)
bool CDPlay::EnumConnections(CAutoArray<CDPlayConnection>* connectionArray)
{
    if (!m_lpDP)
        return 0;
    m_pConnectionArray = connectionArray;
    connectionArray->destroy(1);
    m_hRes = m_lpDP->EnumConnections(0, enumConnectionsCallback, this, 1);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00497250, 0x34)
DC_ADDRESS(0x08a414, 0x42)
bool CDPlay::StartSession(unsigned long groupId)
{
    if (!m_lpDP)
        return 0;
    m_hRes = m_lpDP->StartSession(0, groupId);
    return m_hRes >= 0;
}

VA(0x00497290, 0x22)
DC_ADDRESS(0x08a458, 0x2a)
bool CDPlay::CloseSession()
{
    if (!m_lpDP)
        return 0;
    m_hRes = m_lpDP->Close();
    return m_hRes >= 0;
}

VA(0x004972c0, 0x71)
DC_ADDRESS(0x08a484, 0x44)
bool CDPlay::EnumGroups(CAutoArray<CDPlayGroup>* groupArray, _GUID* guidInstance, unsigned long flags)
{
    m_pGroupArray = groupArray;
    groupArray->destroy(1);
    m_hRes = m_lpDP->EnumGroups(guidInstance, enumGroupsCallback, this, flags);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00497340, 0x71)
DC_ADDRESS(0x08a4c8, 0x44)
bool CDPlay::EnumPlayers(CAutoArray<CDPlayPlayer>* playerArray, _GUID* guidInstance, unsigned long flags)
{
    m_pPlayerArray = playerArray;
    playerArray->destroy(1);
    m_hRes = m_lpDP->EnumPlayers(guidInstance, enumPlayersCallback, this, flags);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x004973c0, 0x75)
DC_ADDRESS(0x08a50c, 0x4c)
bool CDPlay::EnumGroupPlayers(CAutoArray<CDPlayPlayer>* playerArray, unsigned long dpidGroup, _GUID* guidInstance, unsigned long flags)
{
    m_pPlayerArray = playerArray;
    playerArray->destroy(1);
    m_hRes = m_lpDP->EnumGroupPlayers(dpidGroup, guidInstance, enumPlayersCallback, this, flags);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00497440, 0xB5)
DC_ADDRESS(0x08a558, 0x84)
bool CDPlay::EnumSessions(CAutoArray<CDPlaySession>* sessionArray, unsigned long timeOut, unsigned long flags)
{
    if (!m_lpDP)
        return 0;
    m_pSessionArray = sessionArray;
    m_pSessionArray->destroy(1);
    DPSESSIONDESC2 desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.guidApplication = m_guid;
    m_hRes = m_lpDP->EnumSessions(&desc, timeOut, enumSession, this, flags);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00497500, 0x53)
DC_ADDRESS(0x08a5dc, 0x46)
bool CDPlay::SendChat(char* msg, unsigned long idFrom, unsigned long idTo)
{
    if (!m_lpDP)
        return 0;
    DPCHAT chat;
    chat.dwSize = sizeof(DPCHAT);
    chat.dwFlags = 0;
    chat.lpszMessageA = msg;
    m_hRes = m_lpDP->SendChatMessage(idFrom, idTo, 0, &chat);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00497560, 0x48)
DC_ADDRESS(0x08a624, 0x54)
bool CDPlay::Send(void* data, unsigned long size, unsigned long idFrom, unsigned long idTo, bool guaranteed)
{
    if (!m_lpDP)
        return 0;
    unsigned long flags = 1;
    if (!guaranteed)
        flags = 0;
    m_hRes = m_lpDP->Send(idFrom, idTo, flags, data, size);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x004975b0, 0xF3)
DC_ADDRESS(0x08a678, 0xcc)
bool CDPlay::Receive(unsigned long* fromID, unsigned long* toID, CDPlayMsg* msg, unsigned long flags)
{
    if (!m_lpDP)
        return 0;
    unsigned long buffSize = msg->m_dataSize;
    m_hRes = m_lpDP->Receive(fromID, toID, flags, msg->m_data, &buffSize);
    for (;;) {
        if (m_hRes == DPERR_NOMESSAGES)
            return 0;
        if (m_hRes != DPERR_BUFFERTOOSMALL)
            break;
        msg->AllocSize(buffSize);
        if (m_hRes != DPERR_BUFFERTOOSMALL)
            break;
        m_hRes = m_lpDP->Receive(fromID, toID, flags, msg->m_data, &buffSize);
    }
    if (m_hRes < 0)
        return 0;
    if (!*fromID)
        return ReceiveSystemMsg(*toID, msg);
    return ReceiveMsg(*fromID, *toID, msg);
}

VA(0x004976b0, 0xDC)
DC_ADDRESS(0x08a744, 0x9c)
bool CDPlay::FlushReceiveQueue()
{
    CDPlayMsg msg;
    unsigned long from;
    unsigned long to;
    unsigned long buffSize;
    buffSize = 0;
    while (1) {
        m_hRes = m_lpDP->Receive(
            &from, &to, 1, msg.m_data, &buffSize);
        if (m_hRes == DPERR_NOMESSAGES)
            return 1;
        if (m_hRes == DPERR_BUFFERTOOSMALL)
            msg.AllocSize(buffSize);
        if (m_hRes != DPERR_BUFFERTOOSMALL && m_hRes != 0)
            break;
    }
    if (m_hRes < 0)
        return 0;
    return 1;
}

VA(0x004977c0, 0x144)
DC_ADDRESS(0x08a7e0, 0x48)
bool CDPlay::AddSessionEnum(const DPSESSIONDESC2* dpSessionDesc, unsigned long flags)
{
    if (flags & 1)
        return 0;
    CDPlaySession* session = new CDPlaySession(dpSessionDesc);
    m_pSessionArray->add(session);
    return 1;
}

VA(0x00497910, 0x180)
DC_ADDRESS(0x08a828, 0x1a2)
bool CDPlay::ReceiveSystemMsg(unsigned long toID, CDPlayMsg* msg)
{
    DPMSG_GENERIC* generic = static_cast<DPMSG_GENERIC*>(static_cast<void*>(msg->m_data));
    unsigned long messageType = msg->getId();
    switch (messageType) {
    case DPLAY_SYSMSG_ADDGROUPTOGROUP:
        return SysMsgAddGroupToGroup(static_cast<DPMSG_ADDGROUPTOGROUP*>(static_cast<void*>(generic)), toID);
    case DPLAY_SYSMSG_CHAT:
        return SysMsgChat(static_cast<DPMSG_CHAT*>(static_cast<void*>(generic)), toID);
    case DPLAY_SYSMSG_DELETEGROUPFROMGROUP:
        return SysMsgDeleteGroupFromGroup(static_cast<DPMSG_ADDGROUPTOGROUP*>(static_cast<void*>(generic)), toID);
    case DPLAY_SYSMSG_SECUREMESSAGE:
        return SysMsgSecureMessage(static_cast<DPMSG_SECUREMESSAGE*>(static_cast<void*>(generic)), toID);
    case DPLAY_SYSMSG_SETSESSIONDESC:
        return SysMsgSetSessionDesc(static_cast<DPMSG_SETSESSIONDESC*>(static_cast<void*>(generic)), toID);
    case DPLAY_SYSMSG_STARTSESSION:
        return SysMsgStartSession(static_cast<DPMSG_STARTSESSION*>(static_cast<void*>(generic)), toID);
    case DPLAY_SYSMSG_CREATEPLAYERORGROUP:
        return SysMsgCreatePlayerOrGroup(static_cast<DPMSG_CREATEPLAYERORGROUP*>(static_cast<void*>(generic)), toID);
    case DPLAY_SYSMSG_DESTROYPLAYERORGROUP:
        return SysMsgDestroyPlayerOrGroup(static_cast<DPMSG_DESTROYPLAYERORGROUP*>(static_cast<void*>(generic)), toID);
    case DPLAY_SYSMSG_ADDPLAYERTOGROUP:
        return SysMsgAddPlayerToGroup(static_cast<DPMSG_ADDPLAYERTOGROUP*>(static_cast<void*>(generic)), toID);
    case DPLAY_SYSMSG_DELETEPLAYERFROMGROUP:
        return SysMsgDeletePlayerFromGroup(static_cast<DPMSG_ADDPLAYERTOGROUP*>(static_cast<void*>(generic)), toID);
    case DPLAY_SYSMSG_SESSIONLOST:
        return SysMsgSessionLost(generic, toID);
    case DPLAY_SYSMSG_HOST:
        return SysMsgHost(generic, toID);
    case DPLAY_SYSMSG_SETPLAYERORGROUPDATA:
        return SysMsgSetPlayerOrGroupData(static_cast<DPMSG_SETPLAYERORGROUPDATA*>(static_cast<void*>(generic)), toID);
    case DPLAY_SYSMSG_SETPLAYERORGROUPNAME:
        return SysMsgSetPlayerOrGroupName(static_cast<DPMSG_SETPLAYERORGROUPNAME*>(static_cast<void*>(generic)), toID);
    }
    return 1;
}

VA(0x00497a90, 0x6B)
DC_ADDRESS(0x08a9cc, 0x46)
bool CDPlay::AddGroupEnum(unsigned long dpid, const DPNAME* name, unsigned long flags)
{
    CDPlayGroup* group = new CDPlayGroup(name->lpszShortNameA, dpid);
    m_pGroupArray->add(group);
    return 1;
}

VA(0x00497b00, 0x6B)
DC_ADDRESS(0x08aa14, 0x46)
bool CDPlay::AddPlayerEnum(unsigned long dpid, const DPNAME* name, unsigned long flags)
{
    CDPlayPlayer* player = new CDPlayPlayer(name->lpszShortNameA, dpid);
    m_pPlayerArray->add(player);
    return 1;
}

VA(0x00497b70, 0xDF)
DC_ADDRESS(0x08aa5c, 0x78)
bool CDPlay::AddConnectionEnum(const GUID* lpguidSP, void* connection, unsigned long connectionSize, const DPNAME* name, unsigned long flags)
{
    CDPlayConnection* conn = new CDPlayConnection(lpguidSP, connectionSize,
        connection, name->lpszShortNameA);
    m_pConnectionArray->add(conn);
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
bool CDPlay::SysMsgHost(DPMSG_GENERIC* sysMsg, unsigned long toID)
{
    m_isHost = 1;
    return 1;
}

VA(0x004981e0, 0x5)
DC_ADDRESS(0x08af48, 0x4)
bool CDPlay::SysMsgCreatePlayerOrGroup(DPMSG_CREATEPLAYERORGROUP* sysMsg, unsigned long toID)
{
    return 1;
}

// Original: CDPlay::SysMsgDestroyPlayerOrGroup; dxplay.cpp:890
// Shares the return-true CreatePlayerOrGroup body.
DC_ADDRESS(0x08af4c, 0x4)
bool CDPlay::SysMsgDestroyPlayerOrGroup(
    DPMSG_DESTROYPLAYERORGROUP* sysMsg, unsigned long toID)
{
    return 1;
}

VA(0x004981f0, 0x24)
DC_ADDRESS(0x08af50, 0x26)
bool CDPlay::AddPlayerToGroup(unsigned long groupId, unsigned long playerId)
{
    m_hRes = m_lpDP->AddPlayerToGroup(groupId, playerId);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00498220, 0x24)
DC_ADDRESS(0x08af78, 0x120)
bool CDPlay::DeletePlayerFromGroup(unsigned long groupId, unsigned long playerId)
{
    m_hRes = m_lpDP->DeletePlayerFromGroup(groupId, playerId);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00498250, 0x4D)
DC_ADDRESS(0x08b098, 0x40)
bool CDPlay::SetPlayerName(unsigned long playerId, char* shortName, char* longName, unsigned long flags)
{
    char* longValue = longName;
    if (!longValue)
        longValue = shortName;
    DPNAME dpName;
    dpName.dwSize = sizeof(DPNAME);
    dpName.dwFlags = 0;
    dpName.lpszShortNameA = shortName;
    dpName.lpszLongNameA = longValue;
    m_hRes = m_lpDP->SetPlayerName(playerId, &dpName, flags);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x004982a0, 0x115)
DC_ADDRESS(0x08b0d8, 0xd8)
bool CDPlay::GetPlayerName(unsigned long playerId, char* shortName, int maxShort, char* longName, int maxLong)
{
    CDPlayMsg name;
    unsigned long size = 0;
    m_hRes = m_lpDP->GetPlayerName(playerId, 0, &size);
    if (m_hRes != DPERR_BUFFERTOOSMALL)
        return 0;
    name.AllocSize(size + 1);
    DPNAME* dpName = static_cast<DPNAME*>(static_cast<void*>(name.m_data));
    m_hRes = m_lpDP->GetPlayerName(playerId, dpName, &size);
    if (m_hRes < 0)
        return 0;
    if (shortName) {
        if (dpName->lpszShortNameA)
            strncpy(shortName, dpName->lpszShortNameA, maxShort);
        else
            shortName[0] = 0;
    }
    if (longName) {
        if (dpName->lpszLongNameA)
            strncpy(longName, dpName->lpszLongNameA, maxLong);
        else
            longName[0] = 0;
    }
    return 1;
}

VA(0x004983c0, 0x2C)
DC_ADDRESS(0x08b1b0, 0x30)
bool CDPlay::SetGroupData(unsigned long groupId, void* data, unsigned long dataSize, unsigned long flags)
{
    m_hRes = m_lpDP->SetGroupData(groupId, data, dataSize, flags);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x004983f0, 0xB1)
DC_ADDRESS(0x08b1e0, 0xa4)
void* CDPlay::getGroupData(unsigned long groupId, unsigned long* pdwSize, unsigned long flags)
{
    void* buf = 0;
    unsigned long dataSize = 0;
    if (pdwSize)
        dataSize = *pdwSize;
    m_hRes = m_lpDP->GetGroupData(groupId, 0, &dataSize, flags);
    if (m_hRes < 0) {
        if (m_hRes != DPERR_BUFFERTOOSMALL)
            return 0;
        m_hRes = 0;
        if (dataSize == 0)
            return 0;
        buf = ::operator new(dataSize);
        m_hRes = m_lpDP->GetGroupData(groupId, buf, &dataSize, flags);
        if (m_hRes < 0) {
            ::operator delete(buf);
            return 0;
        }
    }
    if (pdwSize)
        *pdwSize = dataSize;
    return buf;
}

VA(0x004984b0, 0x4D)
DC_ADDRESS(0x08b284, 0x40)
bool CDPlay::SetGroupName(unsigned long groupId, char* shortName, char* longName, unsigned long flags)
{
    char* longValue = longName;
    if (!longValue)
        longValue = shortName;
    DPNAME dpName;
    dpName.dwSize = sizeof(DPNAME);
    dpName.dwFlags = 0;
    dpName.lpszShortNameA = shortName;
    dpName.lpszLongNameA = longValue;
    m_hRes = m_lpDP->SetGroupName(groupId, &dpName, flags);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00498500, 0x115)
DC_ADDRESS(0x08b2c4, 0xf6)
bool CDPlay::GetGroupName(unsigned long groupId, char* shortName, int maxShort, char* longName, int maxLong)
{
    CDPlayMsg name;
    unsigned long size = 0;
    m_hRes = m_lpDP->GetGroupName(groupId, 0, &size);
    if (m_hRes != DPERR_BUFFERTOOSMALL)
        return 0;
    name.AllocSize(size + 1);
    DPNAME* dpName = static_cast<DPNAME*>(static_cast<void*>(name.m_data));
    m_hRes = m_lpDP->GetGroupName(groupId, dpName, &size);
    if (m_hRes < 0)
        return 0;
    if (shortName) {
        if (dpName->lpszShortNameA)
            strncpy(shortName, dpName->lpszShortNameA, maxShort);
        else
            shortName[0] = 0;
    }
    if (longName) {
        if (dpName->lpszLongNameA)
            strncpy(longName, dpName->lpszLongNameA, maxLong);
        else
            longName[0] = 0;
    }
    return 1;
}

VA(0x00498620, 0x2C)
DC_ADDRESS(0x08b3bc, 0x30)
bool CDPlay::SetPlayerData(unsigned long playerId, void* data, unsigned long dataSize, unsigned long flags)
{
    m_hRes = m_lpDP->SetPlayerData(playerId, data, dataSize, flags);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00498650, 0xB1)
DC_ADDRESS(0x08b3ec, 0xa8)
void* CDPlay::getPlayerData(unsigned long playerId, unsigned long* pdwSize, unsigned long flags)
{
    void* buf = 0;
    unsigned long dataSize = 0;
    if (pdwSize)
        dataSize = *pdwSize;
    m_hRes = m_lpDP->GetPlayerData(playerId, 0, &dataSize, flags);
    if (m_hRes < 0) {
        if (m_hRes != DPERR_BUFFERTOOSMALL)
            return 0;
        m_hRes = 0;
        if (dataSize == 0)
            return 0;
        buf = ::operator new(dataSize);
        m_hRes = m_lpDP->GetPlayerData(playerId, buf, &dataSize, flags);
        if (m_hRes < 0) {
            ::operator delete(buf);
            return 0;
        }
    }
    if (pdwSize)
        *pdwSize = dataSize;
    return buf;
}

VA(0x00498710, 0x8C)
DC_ADDRESS(0x08b494, 0x88)
unsigned char* CDPlay::getPlayerAddress(unsigned long dpid, unsigned long* sizeOut)
{
    unsigned long size = 0;
    if (!m_lpDP)
        return 0;
    m_hRes = m_lpDP->GetPlayerAddress(dpid, 0, &size);
    if (size == 0)
        return 0;
    if (sizeOut)
        *sizeOut = size;
    unsigned char* buf = static_cast<unsigned char*>(::operator new(size));
    m_hRes = m_lpDP->GetPlayerAddress(dpid, buf, &size);
    if (m_hRes < 0) {
        ::operator delete(buf);
        return 0;
    }
    return buf;
}

VA(0x004987a0, 0x42)
DC_ADDRESS(0x08b51c, 0x48)
bool CDPlay::GetCaps(DPCAPS* dpCaps, bool guaranteed)
{
    memset(dpCaps, 0, sizeof(DPCAPS));
    dpCaps->dwSize = sizeof(DPCAPS);
    unsigned long flags = 0;
    if (guaranteed)
        flags = 1;
    m_hRes = m_lpDP->GetCaps(dpCaps, flags);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x004987f0, 0x31)
DC_ADDRESS(0x08b564, 0x4)
bool CDPlay::GetSendQueueSize(unsigned long from, unsigned long to, unsigned long& numMsgs, unsigned long& numBytes)
{
    m_hRes = m_lpDP->GetMessageQueue(from, to, 1, &numMsgs, &numBytes);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00498830, 0x31)
DC_ADDRESS(0x08b568, 0x4)
bool CDPlay::GetReceiveQueueSize(unsigned long from, unsigned long to, unsigned long& numMsgs, unsigned long& numBytes)
{
    m_hRes = m_lpDP->GetMessageQueue(from, to, 2, &numMsgs, &numBytes);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00498870, 0x82)
DC_ADDRESS(0x08b56c, 0x5c)
CDPlayLobby::CDPlayLobby()
{
    m_lpLobby = 0;
    m_pAddressArray = 0;
}

VA_COMPGEN(0x00498900, 0x21, SCALAR_DELETING_DTOR, CDPlayLobby)

VA(0x00498930, 0x62)
DC_ADDRESS(0x08b5c8, 0x42)
CDPlayLobby::~CDPlayLobby()
{
    if (m_lpLobby)
        m_lpLobby->Release();
}

// DC's older RegisterApp takes four arguments. Complete's caller supplies
// executableName as a fifth argument for DPAPPLICATIONDESC2's app launcher.
VA(0x004989a0, 0xB7)
DC_ADDRESS(0x08b60c, 0x4)
bool CDPlayLobby::RegisterApp(char* appName, char* fileName, char* commandLine, GUID appGuid, char* executableName)
{
    if (!m_lpLobby)
        return 0;
    char curDir[0x105];
    DPAPPLICATIONDESC2 desc;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0;
    desc.lpszApplicationNameA = appName;
    desc.lpszDescriptionA = 0;
    desc.lpszDescriptionW = 0;
    if (!GetCurrentDirectoryA(0x105, curDir))
        return 0;
    desc.lpszPathA = curDir;
    desc.lpszCurrentDirectoryA = curDir;
    desc.lpszFilenameA = fileName;
    desc.guidApplication = appGuid;
    desc.lpszCommandLineA = commandLine;
    desc.lpszAppLauncherNameA = executableName;
    m_hRes = m_lpLobby->RegisterApplication(0, &desc);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00498a60, 0x72)
DC_ADDRESS(0x08b610, 0x4)
bool CDPlayLobby::Init()
{
    if (!CDPlay::Init())
        return 0;
    if (m_lpLobby) {
        m_lpLobby->Release();
        m_lpLobby = 0;
    }
    m_hRes = CoCreateInstance(CLSID_DirectPlayLobby, 0, CLSCTX_INPROC_SERVER,
        IID_IDirectPlayLobby3A, reinterpret_cast<void**>(&m_lpLobby));
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00498ae0, 0x8C)
DC_ADDRESS(0x08b614, 0x88)
DPLCONNECTION* CDPlayLobby::getConnectionSettings(unsigned long appId, unsigned long* sizeOut)
{
    unsigned long size = 0;
    if (!m_lpLobby)
        return 0;
    m_hRes = m_lpLobby->GetConnectionSettings(appId, 0, &size);
    if (sizeOut)
        *sizeOut = size;
    if (size == 0)
        return 0;
    DPLCONNECTION* buf = static_cast<DPLCONNECTION*>(::operator new(size));
    m_hRes = m_lpLobby->GetConnectionSettings(appId, buf, &size);
    if (m_hRes < 0) {
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
// Native SDK interfaces, field order and m_hRes spelling are also byte-flat.
// HRESULT severity/success switches retain separate exits (best 90.8772%)
// but leave extra shift/setcc/dispatch instructions absent in retail; keep
// the direct signed-status guard until a supported source model closes it.
// Identity-checked SP3 C2 trace locates the fold in conditional-select pass
// 0x27954, before tail merging: cost 2 passes threshold 10. A shared failure
// label avoids it but leaves two returns (three if only one early failure
// shares it). See docs/vc6/behavior-catalog.md D13.
// Clearing buf on failure retains jge but adds pointer tracking and a final
// test/setne (75.0%); a VC5 control still folds to setge and worsens siblings.
// E:\gamedcs\dxplay.cpp:1351
VA(0x00498b70, 0x6E)  // anchor-callee IDirectPlayLobby::GetConnectionSettings probe + GlobalAlloc/GlobalLock; ret 0, src-order
DC_ADDRESS(0x08b69c, 0x4)
bool CDPlayLobby::TestLobbied()
{
    unsigned long size;
    m_hRes = m_lpLobby->GetConnectionSettings(0, 0, &size);
    if (m_hRes != DPERR_BUFFERTOOSMALL)
        return 0;
    void* buf = GlobalLock(GlobalAlloc(0x42, size));
    if (!buf)
        return 0;
    m_hRes = m_lpLobby->GetConnectionSettings(0, buf, &size);
    if (m_hRes >= 0)
        return 1;
    return 0;
}

VA(0x00498be0, 0x31)
DC_ADDRESS(0x08b6a0, 0x3a)
bool CDPlayLobby::SetConnectionSettings(unsigned long appId, DPLCONNECTION* connection)
{
    if (!m_lpLobby)
        return 0;
    m_hRes = m_lpLobby->SetConnectionSettings(0, appId, connection);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00498c20, 0x29)
DC_ADDRESS(0x08b6dc, 0x2c)
bool CDPlayLobby::SetGroupConnectionSettings(unsigned long dpidGroup, DPLCONNECTION* connection)
{
    m_hRes = m_lpDP->SetGroupConnectionSettings(0, dpidGroup, connection);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00498c50, 0x80)
DC_ADDRESS(0x08b708, 0x76)
DPLCONNECTION* CDPlayLobby::getGroupConnectionSettings(unsigned long dpidGroup)
{
    unsigned long size = 0;
    m_hRes = m_lpDP->GetGroupConnectionSettings(0, dpidGroup, 0, &size);
    if (size == 0)
        return 0;
    DPLCONNECTION* buf = static_cast<DPLCONNECTION*>(::operator new(size));
    m_hRes = m_lpDP->GetGroupConnectionSettings(0, dpidGroup, buf, &size);
    if (m_hRes < 0) {
        ::operator delete(buf);
        return 0;
    }
    return buf;
}

VA(0x00498cd0, 0xAB)
DC_ADDRESS(0x08b780, 0x88)
bool CDPlayLobby::Connect()
{
    DPLCONNECTION* conn = getConnectionSettings(0, 0);
    if (conn->dwFlags & DPLAY_CONNECTION_CREATE_SESSION)
        m_isHost = 1;
    else
        m_isHost = 0;
    ::operator delete(conn);
    if (m_lpDP) {
        m_lpDP->Release();
        m_lpDP = 0;
    }
    m_hRes = m_lpLobby->ConnectEx(0, IID_IDirectPlay4A, reinterpret_cast<void**>(&m_lpDP), 0);
    bool ok = m_hRes >= 0;
    return ok;
}

// Original: CDPlayLobby::SendStandardLobbyMsg; dxplay.cpp:1476
// The ordinary lobby-message APIs use the same interface and HRESULT state
// as Complete's connection-setting wrappers. DC1478/1479 proves the
// null-lobby guard and SendLobbyMessage flags2; no separate retail entries
// are claimed.
DC_ADDRESS(0x08b808, 0x5c)
bool CDPlayLobby::SendStandardLobbyMsg(
    unsigned long appId, void* data, unsigned long size)
{
    if (!m_lpLobby)
        return 0;
    m_hRes = m_lpLobby->SendLobbyMessage(
        DPLMSG_STANDARD, appId, data, size);
    if (m_hRes < 0)
        return 0;
    return 1;
}

// Original: CDPlayLobby::SendLobbyMsg; dxplay.cpp:1490
DC_ADDRESS(0x08b864, 0x42)
bool CDPlayLobby::SendLobbyMsg(
    unsigned long appId, void* data, unsigned long size)
{
    if (!m_lpLobby)
        return 0;
    m_hRes = m_lpLobby->SendLobbyMessage(
        0, appId, data, size);
    if (m_hRes < 0)
        return 0;
    return 1;
}

// Original: CDPlayLobby::ReceiveLobbyMsg; dxplay.cpp:1503
// DC1511..1521 retries after growing CDPlayMsg's buffer. DC1525/1526
// dispatches SYSTEM/STANDARD messages and negates the handler result.
DC_ADDRESS(0x08b8a8, 0xa6)
bool CDPlayLobby::ReceiveLobbyMsg(unsigned long appId, CDPlayMsg* msg)
{
    unsigned long flags;
    if (!m_lpLobby)
        return 0;
    do {
        m_hRes = m_lpLobby->ReceiveLobbyMessage(
            0, appId, &flags, msg->m_data, &msg->m_dataSize);
        if (m_hRes == DPERR_BUFFERTOOSMALL)
            msg->AllocSize(msg->m_dataSize);
        else if (m_hRes < 0)
            return 0;
    } while (m_hRes == DPERR_BUFFERTOOSMALL);
    if (flags == DPLMSG_SYSTEM || flags == DPLMSG_STANDARD)
        return !HandleSystemLobbyMsg(appId, msg);
    return 1;
}

VA(0x00498d80, 0x3C9)
DC_ADDRESS(0x08b950, 0x4)
CDPlayConnection* CDPlayLobby::createTCPIPConnection(
    char* ipAddress, char* name, CDPlayConnection* append)
{
    DPCOMPOUNDADDRESSELEMENT elements[10];
    unsigned long addressSize = 0;
    CAutoArray<CDPlayAddressElement> addresses;
    unsigned long count = 0;
    if (append) {
        if (!EnumAddress(append->m_connection, append->m_size, &addresses))
            return 0;
        while (count < addresses.getCount()) {
            CDPlayAddressElement* element = addresses.get(count);
            elements[count].guidDataType = element->m_guid;
            elements[count].dwDataSize = element->m_dataSize;
            elements[count].lpData = element->m_data;
            ++count;
        }
    }
    elements[count].guidDataType = DPAID_ServiceProvider;
    elements[count].dwDataSize = sizeof(GUID);
    elements[count].lpData = const_cast<GUID*>(&DPSPGUID_TCPIP);
    ++count;
    if (ipAddress) {
        elements[count].guidDataType = DPAID_INet;
        elements[count].dwDataSize = strlen(ipAddress) + 1;
        elements[count].lpData = ipAddress;
        ++count;
    }
    m_hRes = m_lpLobby->CreateCompoundAddress(
        elements, count, 0, &addressSize);
    if (m_hRes != DPERR_BUFFERTOOSMALL)
        return 0;
    void* address = ::operator new(addressSize);
    m_hRes = m_lpLobby->CreateCompoundAddress(
        elements, count, address, &addressSize);
    if (m_hRes < 0) {
        ::operator delete(address);
        return 0;
    }
    CDPlayConnection* connection = new CDPlayConnection(
        &DPSPGUID_TCPIP, addressSize, address, name);
    ::operator delete(address);
    return connection;
}

VA(0x00499150, 0x356)
DC_ADDRESS(0x08b954, 0x4)
CDPlayConnection* CDPlayLobby::createIPXConnection(char* name, CDPlayConnection* connAppend)
{
    DPCOMPOUNDADDRESSELEMENT elements[10];
    unsigned long addressSize = 0;
    unsigned long count = 0;
    CAutoArray<CDPlayAddressElement> addresses;
    if (connAppend) {
        if (!EnumAddress(connAppend->m_connection, connAppend->m_size, &addresses))
            return 0;
        while (count < addresses.getCount()) {
            CDPlayAddressElement* elem = addresses.get(count);
            elements[count].guidDataType = elem->m_guid;
            elements[count].dwDataSize = elem->m_dataSize;
            elements[count].lpData = elem->m_data;
            ++count;
        }
    }
    elements[count].guidDataType = DPAID_ServiceProvider;
    elements[count].dwDataSize = sizeof(GUID);
    elements[count].lpData = const_cast<GUID*>(&DPSPGUID_IPX);
    ++count;
    m_hRes = m_lpLobby->CreateCompoundAddress(elements, count, 0, &addressSize);
    if (m_hRes != DPERR_BUFFERTOOSMALL)
        return 0;
    void* address = ::operator new(addressSize);
    m_hRes = m_lpLobby->CreateCompoundAddress(elements, count, address, &addressSize);
    if (m_hRes < 0) {
        ::operator delete(address);
        return 0;
    }
    CDPlayConnection* conn = new CDPlayConnection(&DPSPGUID_IPX, addressSize, address, name);
    ::operator delete(address);
    return conn;
}

VA(0x004994b0, 0x24E)
DC_ADDRESS(0x08b958, 0x4)
CDPlayConnection* CDPlayLobby::createModemConnection(char* name, char* phoneNbr, char* modemString)
{
    DPCOMPOUNDADDRESSELEMENT elements[10];
    unsigned long addressSize = 0;
    elements[0].guidDataType = DPAID_ServiceProvider;
    elements[0].dwDataSize = sizeof(GUID);
    elements[0].lpData = const_cast<GUID*>(&DPSPGUID_MODEM);
    unsigned long count = 1;
    if (modemString) {
        elements[1].guidDataType = DPAID_Modem;
        elements[1].dwDataSize = strlen(modemString) + 1;
        elements[1].lpData = modemString;
        count = 2;
    }
    if (phoneNbr) {
        elements[count].guidDataType = DPAID_Phone;
        elements[count].dwDataSize = strlen(phoneNbr) + 1;
        elements[count].lpData = phoneNbr;
        ++count;
    }
    m_hRes = m_lpLobby->CreateCompoundAddress(elements, count, 0, &addressSize);
    if (m_hRes != DPERR_BUFFERTOOSMALL)
        return 0;
    void* address = ::operator new(addressSize);
    m_hRes = m_lpLobby->CreateCompoundAddress(elements, count, address, &addressSize);
    if (m_hRes < 0) {
        ::operator delete(address);
        return 0;
    }
    CDPlayConnection* conn = new CDPlayConnection(&DPSPGUID_MODEM, addressSize, address, name);
    ::operator delete(address);
    return conn;
}

VA(0x00499700, 0x1F4)
DC_ADDRESS(0x08b95c, 0x4)
CDPlayConnection* CDPlayLobby::createSerialConnection(char* name, _DPCOMPORTADDRESS* comPortInfo)
{
    DPCOMPOUNDADDRESSELEMENT elements[10];
    unsigned long addressSize = 0;
    elements[0].guidDataType = DPAID_ServiceProvider;
    elements[0].dwDataSize = sizeof(GUID);
    elements[0].lpData = const_cast<GUID*>(&DPSPGUID_SERIAL);
    unsigned long count = 1;
    if (comPortInfo) {
        elements[1].guidDataType = DPAID_ComPort;
        elements[1].dwDataSize = 0x14;
        elements[1].lpData = comPortInfo;
        count = 2;
    }
    m_hRes = m_lpLobby->CreateCompoundAddress(elements, count, 0, &addressSize);
    if (m_hRes != DPERR_BUFFERTOOSMALL)
        return 0;
    void* address = ::operator new(addressSize);
    m_hRes = m_lpLobby->CreateCompoundAddress(elements, count, address, &addressSize);
    if (m_hRes < 0) {
        ::operator delete(address);
        return 0;
    }
    CDPlayConnection* conn = new CDPlayConnection(&DPSPGUID_SERIAL, addressSize, address, name);
    ::operator delete(address);
    return conn;
}

// Original: CDPlayLobby::HandleSystemLobbyMsg; dxplay.cpp:1802
// Shares the return-true body.
DC_ADDRESS(0x08b960, 0x4)
bool CDPlayLobby::HandleSystemLobbyMsg(unsigned long appId, CDPlayMsg* msg)
{
    return 1;
}

VA(0x00499900, 0x97)
DC_ADDRESS(0x08b964, 0x4)
bool CDPlayLobby::EnumLobbyConnections(CAutoArray<CDPlayConnection>* connectionArray)
{
    if (!m_lpDP)
        return 0;
    m_pConnectionArray = connectionArray;
    connectionArray->destroy(1);
    m_hRes = m_lpDP->EnumConnections(&m_guid, enumConnectionsCallback, this, 2);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x004999a0, 0x75)
DC_ADDRESS(0x08b968, 0x48)
bool CDPlayLobby::EnumGroupsInGroup(CAutoArray<CDPlayGroup>* groupArray, unsigned long dpidParent, unsigned long flags)
{
    m_pGroupArray = groupArray;
    groupArray->destroy(1);
    m_hRes = m_lpDP->EnumGroupsInGroup(dpidParent, 0, enumGroupsCallback, this, flags);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00499a20, 0x72)
DC_ADDRESS(0x08b9b0, 0x46)
bool CDPlayLobby::EnumGroupPlayers(CAutoArray<CDPlayPlayer>* playerArray, unsigned long dpidGroup, unsigned long flags)
{
    m_pPlayerArray = playerArray;
    playerArray->destroy(1);
    m_hRes = m_lpDP->EnumGroupPlayers(dpidGroup, 0, enumPlayersCallback, this, flags);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00499aa0, 0x78)
DC_ADDRESS(0x08b9f8, 0x70)
bool CDPlayLobby::EnumGroupPlayersRemote(CAutoArray<CDPlayPlayer>* playerArray, unsigned long dpidGroup, _GUID* guidInstance, unsigned long flags)
{
    m_pPlayerArray = playerArray;
    playerArray->destroy(1);
    m_hRes = m_lpDP->EnumGroupPlayers(dpidGroup, guidInstance, enumPlayersCallback, this, flags | 0x80);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00499b20, 0x89)
DC_ADDRESS(0x08ba68, 0x46)
bool CDPlayLobby::EnumAddress(void* conn, unsigned long size, CAutoArray<CDPlayAddressElement>* array)
{
    m_pAddressArray = array;
    array->destroy(1);
    m_hRes = m_lpLobby->EnumAddress(EnumAddressCallback, conn, size, this);
    bool ok = m_hRes >= 0;
    return ok;
}

VA(0x00499bb0, 0xAA)
DC_ADDRESS(0x08bab0, 0x4c)
bool CDPlayLobby::AddAddressEnum(const GUID* guid, unsigned long dataSize, const void* data)
{
    CDPlayAddressElement* element = new CDPlayAddressElement(guid, data, dataSize);
    m_pAddressArray->add(element);
    return 1;
}

VA(0x00499c60, 0x1B8)
DC_ADDRESS(0x08bafc, 0xa6)
bool CDPlayLobby::GetIPAddress(unsigned long dpid, char* ipAddress)
{
    ipAddress[0] = 0;
    unsigned long size;
    unsigned char* buf = getPlayerAddress(dpid, &size);
    if (!buf)
        return 0;
    CAutoArray<CDPlayAddressElement> addresses;
    EnumAddress(buf, size, &addresses);
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
int __stdcall EnumAddressCallback(const GUID& guidDataType, unsigned long dwDataSize, const void* lpData, void* lpContext)
{
    return static_cast<CDPlayLobby*>(lpContext)->AddAddressEnum(&guidDataType, dwDataSize, lpData);
}

VA(0x00499e50, 0x1F)
DC_ADDRESS(0x08bbc4, 0x18)
int __stdcall enumSession(const DPSESSIONDESC2* dpSessionDesc, unsigned long* lpdwTimeOut, unsigned long flags, void* context)
{
    return static_cast<CDPlay*>(context)->AddSessionEnum(dpSessionDesc, flags);
}

VA(0x00499e70, 0x2B)
DC_ADDRESS(0x08bbdc, 0x30)
int __stdcall enumConnectionsCallback(const GUID* lpguidSP, void* connection, unsigned long connectionSize, const DPNAME* name, unsigned long flags, void* context)
{
    return static_cast<CDPlay*>(context)->AddConnectionEnum(lpguidSP, connection, connectionSize, name, flags);
}

VA(0x00499ea0, 0x23)
DC_ADDRESS(0x08bc0c, 0x1c)
int __stdcall enumGroupsCallback(unsigned long dpid, unsigned long playerType, const DPNAME* name, unsigned long flags, void* context)
{
    return static_cast<CDPlay*>(context)->AddGroupEnum(dpid, name, flags);
}

VA(0x00499ed0, 0x23)
DC_ADDRESS(0x08bc28, 0x5c)
int __stdcall enumPlayersCallback(unsigned long dpid, unsigned long playerType, const DPNAME* name, unsigned long flags, void* context)
{
    return static_cast<CDPlay*>(context)->AddPlayerEnum(dpid, name, flags);
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
