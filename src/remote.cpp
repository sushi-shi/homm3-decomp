#include "va.h"

#include <string.h>
#include <zlib.h>

#include "remote.h"

#include "advmgr.h"
#include "armygrp.h"
#include "crt_stdio.h"
#include "csprite.h"
#include "cspriteframe.h"
#include "game.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "message.h"
#include "misc.h"
#include "mousemgr.h"
#include "netgame.h"
#include "netplayer.h"
#include "prefs.h"
#include "remotedlg.h"
#include "resourcemanager.h"
#include "sample.h"
#include "smackmgr.h"
#include "soundmgr.h"
#include "textresource.h"
#include "textwdgt.h"
#include "winmgr.h"

DATA(0x00697774) int g_saveGameRequested;
DATA(0x006976dc) int g_tcpHostStatus;


// Retail scalar state; startup initial values come from the pinned image.
// Original DC name: giNumHumanPlayers; DoNewGame / DoLoadGame.
DATA(0x00699274) int g_numHumanPlayers;
// Original DC name: iMPBaseType; DoNewGame / DoLoadGame.
DATA(0x006994e4) int g_mpBaseType;
DATA(0x0069ca50) CHotSeatMan* g_hotSeatMan;

// remote.cpp's CHourGlass wrapper expands these two singleselectionwindow
// helpers at each use. They stay file-local declarations because this is the
// only remote consumer and widening the shared header would perturb its TUs.
void startMouseThread();
void stopMouseThread();

DATA(0x0069d7b0) CChatManager g_chatMan(20);
DATA(0x0069d630) CTurnDuration g_turnDuration;

DATA(0x0069d648) CLogFile g_logFile(
    DATA_COMPGEN(0x00682a3c, remoteGameLogName, "game.log"));
VA_COMPGEN(0x00552260, 0x2A, STATIC_CTOR, g_logFile)

DATA(0x0063dc18) const GUID guidHeroes3 = {
    0x8b743aa0, 0x53b2, 0x11d2,
    { 0x80, 0x8a, 0x00, 0x60, 0x08, 0x95, 0xfb, 0x43 }
};

// DPSD's recursion guard. Dreamcast publishes this compiland-local byte as
// `__inside__`; retail's two inlined error paths fix it at 0x69d814.
DATA(0x0069d814) static unsigned char g_inside;

// E:\gamedcs\remote.cpp:102 - Dreamcast retains this as an out-of-line
// helper; VC6 /Ob2 expands both retail call sites into InitConnection. The
// three beeps, 200-byte local error buffer and recursion guard are visible in
// both byte-identical expansions.
void dpsd(int dpErr, char* file, int line)
{
    if (g_inside)
        return;

    g_inside = 1;
    char errorText[200];
    if (!g_dPlay)
        strcpy(errorText,
               DATA_COMPGEN(0x0067f5fc, remoteInitializationFailed,
                            "Initialization failed!"));
    else
        g_dPlay->getErrorDesc(g_dPlay->getLastError(), errorText);

    MessageBeep(0);
    MessageBeep(0);
    MessageBeep(0);
    sprintf(g_text,
            DATA_COMPGEN(0x00682a48, remoteDirectPlayError,
                         "DirectPlay Error:\n\n'%s'\n\n  File:'%s'\n Line# %d"),
            errorText, file, line);
    shutDown(g_text);
    g_inside = 0;
}

VA(0x005522d0, 0x1E)  // dc order-map (DPSD, calc_crc_long, CDPlayHeroes::CDPlayHeroes) + anchor-callee @crc32@12 twice, dc 0x11b940
unsigned long calcCrcLong(const unsigned char* buf, unsigned len)
{
    unsigned long seed = crc32(0, 0, 0);
    return crc32(seed, buf, len);
}

VA_COMPGEN(0x005522f0, 0x21, SCALAR_DELETING_DTOR, CDPlayHeroes)

// E:\gamedcs\remote.cpp:146,1841 - the retail constructor is fully
// expanded at CreateDPlayObject's sole construction site.  The two named
// records and offsets come from the DC class field list; retail widens the
// deque from 0x28 to the VC6/Dinkumware 0x30 bytes and thereby fixes the PC
// tail at +0x98/+0xe8/+0xec/+0xf0.  currMessageId is deliberately not
// initialized: retail likewise leaves +0xec untouched.
CDPlayHeroes::CDPlayHeroes()
{
    m_localIpAddress[0] = 0;
    m_confirmId = 0;
    m_netMsgHandler = 0;
}

// E:\gamedcs\remote.cpp:155,160 - retail folds DestroyMsgQueue into the
// destructor. The first deque walk deletes every queued message; VC6 then
// emits the member deque destructor, CDPlayMsg cleanup, and the lobby-base
// destructor in declaration order. Declaring CDPlayLobby's real virtual
// destructor is the decisive inline-budget input: with that complete base
// contract, the original named helper call expands to the exact 0x205-byte
// retail body (both deque walks included).
void CDPlayHeroes::destroyMsgQueue()
{
    while (!m_msgQueue.empty()) {
        CNetMsg* netMsg = m_msgQueue.front();
        delete netMsg;
        m_msgQueue.pop_front();
    }
}

VA(0x00552320, 0x205)  // dc 0x11ba38
CDPlayHeroes::~CDPlayHeroes()
{
    destroyMsgQueue();
}

VA(0x00552530, 0x20E)  // dc 0x11bad8
unsigned char CDPlayHeroes::sysMsgHost(DPMSG_GENERIC* message,
                                       unsigned long toId)
{
    unsigned char wasHost = isHost();
    if (!CDPlay::sysMsgHost(message, toId))
        return 0;
    if (!wasHost)
        handleHostXFer();
    return 1;
}

VA(0x00552740, 0x1DE)  // dc 0x11bb20
unsigned char CDPlayHeroes::sysMsgSessionLost(DPMSG_GENERIC* message,
                                              unsigned long toId)
{
    CSessionLostMsg msg;
    queueMsg(&msg);
    return 1;
}

// E:\gamedcs\remote.cpp:204. Only a PLAYER leaving matters - a group being
// destroyed is ignored - and the station that left is logged through the
// same format string HandlePlayerDrop uses before the drop notification is
// queued for the higher-level dispatchers.
VA(0x00552920, 0x216)  // anchor-string(playerDroppedLog) + dc-order-map, dc 0x11bb40
unsigned char CDPlayHeroes::sysMsgDestroyPlayerOrGroup(
    DPMSG_DESTROYPLAYERORGROUP* message, unsigned long toId)
{
    if (message->m_playerType == DPPLAYERTYPE_PLAYER) {
        unsigned long dpid = message->m_dpId;
        g_logFile.log(DATA_COMPGEN(0x00682a78, playerDroppedLog,
                                "********Player dropped---->[%d]"),
                    dpid);
        CPlayerDropMsg msg(dpid);
        queueMsg(&msg);
        return 1;
    }
    return CDPlay::sysMsgDestroyPlayerOrGroup(message, toId);
}

VA(0x00552b40, 0x14)
unsigned char CDPlayHeroes::sysMsgCreatePlayerOrGroup(
    DPMSG_CREATEPLAYERORGROUP* message, unsigned long toId)
{
    return CDPlay::sysMsgCreatePlayerOrGroup(message, toId);
}

VA(0x00552b60, 0x24B)  // dc 0x11bb9c
bool CDPlayHeroes::pollRemote()
{
    unsigned long fromId;
    unsigned long toId;

    while (1) {
        if (!receive(&fromId, &toId, &m_dpMsg, 1))
            break;
        if (fromId == g_thisNetPlayerInfo.m_dpid)
            continue;
        if (!fromId)
            continue;
        CNetMsg* netMsg =
            static_cast<CNetMsg*>(static_cast<void*>(m_dpMsg.m_data));
        if (handleLowLevelMsg(netMsg))
            continue;
        queueMsg(netMsg);
    }

    if (m_res != DPLAY_RECEIVE_ERROR_NO_MESSAGES) {
        char description[256];
        getErrorDesc(m_res, description);
        g_logFile.log(DATA_COMPGEN(0x00682a98, dplayReceiveErrorLog,
                                "DPlay Receive error [%s]"),
                    description);
        return false;
    }
    return true;
}

// DC names the network singleton pDPlay; retail's remote/front-end call
// graph locates the pointer at 0x69d808.
DATA(0x0069d808) CDPlayHeroes* g_dPlay;
// Original public ?g_lobbyLaunched@@3_NA. Retail oldmain stores the native
// TestIfLobbyLaunched result directly here; main-menu host handling agrees.
DATA(0x0069d80c) bool g_lobbyLaunched;
// These adjacent PC bytes are the packed counterparts of Dreamcast's bool
// gbMPlayer/gbMPlayerHost pair. TestIfLobbyLaunched and HandleMPlayerLaunch
// independently distinguish their roles.
DATA(0x00699550) bool g_mPlayer;
DATA(0x00699551) bool g_mPlayerHost;
// Dreamcast publishes `bDefeatedAllPlayers` as a bool in remote.obj. Retail's
// win/loss handlers independently locate the PC cell and store full dwords,
// so the PC representation is int even though the role and owner transfer.
DATA(0x00699510) int g_defeatedAllPlayers;
// Dreamcast publishes gcTCPAddress as char[21]; retail's client launch arm
// passes this exact cell both to the log formatter and InitConnection.

// The PC layout is crossed from the HD build through whole-function operand
// correspondence; the names and types are the Dreamcast CodeView globals.
DATA(0x00697758) char g_tcpAddress[21];
// Retail-only byte armed when player-drop recovery resumes through
// game::NextPlayer. No surviving symbol attests a semantic name.
DATA(0x0069d804) unsigned char g_gameMode;
DATA(0x0069d80d) unsigned char g_playerDrop;
DATA(0x0069d80e) unsigned char g_weMoved;
DATA(0x0069d608) CNetPlayerInfo g_thisNetPlayerInfo;
DATA(0x006989f0) eNetGameType g_mpNetProtocol;
// Dreamcast publishes gMapName as char[260]. LobbyLaunchConnect copies the
// selected setup filename here before refreshing the scenario header; the
// next retail cell at 0x6994e4 independently proves the 0x104-byte extent.
DATA(0x00682a38) unsigned char g_followPlayerMode;
// Dreamcast's remote.obj static-global roster names this timestamp;
// retail's PollRemote fixes its address and unsigned-long type.
DATA(0x006993e0) char g_mapName[260];
DATA(0x0069d818) static unsigned long g_lastActiveUpdate;

static const long g_playerActiveUpdateInterval = 600000;

// Unimplemented carcass rows remain available to the claim/label scanners but
// stay outside compilation as this large TU is admitted incrementally.

VA(0x00552db0, 0x28F)  // dc 0x11bc88
unsigned char CDPlayHeroes::handleLowLevelMsg(CNetMsg* netMsg)
{
    switch (netMsg->m_subType) {
    case RS_PING:
        {
            transmitRemoteDataDPID(
                &CPingResponseMsg(
                    static_cast<CPingMsg*>(netMsg)->m_pingTime, RS_PING_REPLY),
                netMsg->m_dpidFrom, false, false);
        }
        break;

    case RS_PING_REPLY:
        {
            char tempText[256];
            sprintf(tempText,
                    g_generalText->getText(
                        GENERAL_TEXT_CHAT_PING_RESULT_FORMAT),
                    GameTime::elapsedSince(
                        static_cast<CPingMsg*>(netMsg)->m_pingTime));
            receiveChat(tempText, netMsg->m_from);
        }
        break;

    case RS_DESTROY_PLAYER:
        {
            unsigned long dpid =
                static_cast<CDestroyPlayerMsg*>(netMsg)->m_dpid;
            if (dpid == g_thisNetPlayerInfo.m_dpid) {
                remoteCleanup();
                normalDialog(
                    g_generalText->getText(
                        GENERAL_TEXT_REMOTE_SESSION_DESTROYED),
                    1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                shutDown(0);
            }
            else {
                handlePlayerDrop(dpid);
            }
        }
        break;

    default:
        return 0;
    }

    return 1;
}

// Original: CDPlayHeroes::GetRemoteData; remote.cpp:350, dc 0x11bd5c
// Retail expands IsCompressed and ordinary UncompressMsg. Restoring these
// boundaries and DestroyMsg calls recovers the retained deque helper and
// register homes: 86.26 -> 98.62%, with all nine named calls in order.
// Assigning the result before setting wasCompressed recovers the remaining
// four instruction differences and reaches 100%; the reversed ordering is
// the 98.62% negative control. Both helper and caller boundaries stay natural.
VA(0x00553040, 0x1D1)  // anchor-caller(the free GetRemoteData wrapper, CheckHandleNet) + dc-order-map, dc 0x11bd5c
CNetMsg* CDPlayHeroes::getRemoteData(unsigned char removeFromQueue,
                                     unsigned char* wasCompressed)
{
    if (wasCompressed)
        *wasCompressed = 0;
    if (!g_remoteOn)
        return 0;
    if (!m_msgQueue.size())
        return 0;

    CNetMsg* netMsg = m_msgQueue.front();
    if (netMsg) {
        if (removeFromQueue)
            m_msgQueue.pop_front();

        if (netMsg->isCompressed()) {
            CNetMsg* uncompressedMsg = uncompressMsg(netMsg);
            if (!uncompressedMsg) {
                destroyMsg(netMsg);
                return 0;
            }
            if (removeFromQueue)
                destroyMsg(netMsg);
            netMsg = uncompressedMsg;
            if (wasCompressed)
                *wasCompressed = 1;
        }
    }
    return netMsg;
}

VA(0x00553220, 0x89)
bool CDPlayHeroes::transmitRemoteData(CNetMsg* msg, int toWho,
                                      bool compressMsg, bool guaranteed)
{
    unsigned long dpidTo = 0;
    if (toWho != NET_MESSAGE_RECIPIENT_ALL) {
        dpidTo = g_game->m_players[toWho].m_dpid;
        if (!dpidTo)
            return true;
    }

    return transmitRemoteDataDPID(
        msg, dpidTo, compressMsg, guaranteed);
}

VA(0x005532b0, 0xB9)
CNetMsg* CDPlayHeroes::compressMsg(CNetMsg* netMsg)
{
    HOMM3_RELEASE_VERIFY(netMsg != 0 && netMsg->m_size >= sizeof(CNetMsg));
    unsigned long compressedSize =
        static_cast<unsigned long>(netMsg->m_size * 1.2) + 12;
    void* storage = ::operator new(compressedSize);
    CNetMsg* compressedMsg = static_cast<CNetMsg*>(storage);
    memcpy(compressedMsg, netMsg, sizeof(CNetMsg));

    compressedSize -= sizeof(CNetMsg);
    if (compress2(
            static_cast<unsigned char*>(storage) + sizeof(CNetMsg),
            &compressedSize,
            static_cast<const unsigned char*>(
                static_cast<const void*>(netMsg)) + sizeof(CNetMsg),
            netMsg->m_size - sizeof(CNetMsg), 6)) {
        ::operator delete(storage);
        return 0;
    }

    compressedMsg->m_size = compressedSize + sizeof(CNetMsg);
    unsigned long originalSize = netMsg->m_size;
    compressedMsg->m_uncompressedSize = originalSize;
    if (compressedMsg->m_size >= originalSize) {
        ::operator delete(storage);
        return 0;
    }
    return compressedMsg;
}

// Original: CDPlayHeroes::UncompressMsg; remote.cpp:463, dc 0x11bf40
// GetRemoteData (0x553040) expands this header copy, zlib call and cleanup.
CNetMsg* CDPlayHeroes::uncompressMsg(CNetMsg* netMsg)
{
    unsigned long destSize = netMsg->m_uncompressedSize + sizeof(CNetMsg);
    CNetMsg* result = static_cast<CNetMsg*>(::operator new(destSize));
    *result = *netMsg;
    destSize -= sizeof(CNetMsg);
    if (uncompress(static_cast<unsigned char*>(static_cast<void*>(result)) + sizeof(CNetMsg),
                   &destSize,
                   static_cast<const unsigned char*>(static_cast<const void*>(netMsg)) + sizeof(CNetMsg),
                   netMsg->m_size - sizeof(CNetMsg))) {
        destroyMsg(result);
        return 0;
    }
    result->m_size = destSize + sizeof(CNetMsg);
    return result;
}

VA(0x00553370, 0x5C)
bool CDPlayHeroes::transmitRemoteDataDPID(CNetMsg* msg,
                                          unsigned long dpidTo,
                                          bool compressMsg, bool guaranteed)
{
    msg->m_from = g_localGamePos;
    CNetMsg* compressedMsg = 0;
    msg->m_dpidFrom = g_thisNetPlayerInfo.m_dpid;
    if (compressMsg) {
        compressedMsg = this->compressMsg(msg);
        if (compressedMsg)
            msg = compressedMsg;
    }

    bool result = sendIt(msg, dpidTo, guaranteed);
    if (compressedMsg)
        delete compressedMsg;
    return result;
}

// E:\gamedcs\remote.cpp:578. Dreamcast supplies the public member boundary
// and its HandlePlayerDrop edge. Retail fixes the DirectPlay error cases,
// six-attempt retry loop, localized retry dialog and queued drop message.
// Residual wall (86.98%): the invalid-player tail is exact block-for-block,
// including the caller-context decision to leave deque::push_back out of
// line. C2 rotates this source-honest for-loop into a bottom test and
// normalizes Send's AL result through CL; retail keeps the retry-limit test
// at the header and AL live through the HRESULT compares. `while`, explicit
// header-break and call-site inline_depth(1/2) forms do not recover that
// schedule. Caching GetLastError in a named local regresses it to 85.00%.
// 2026-09-05, easy lane 3. Two deltas, one cause. Retail's retry loop is
// UNROTATED - `cmp retries,5 / jg <exit>` at the head and a bare `jmp <head>`
// at the tail - and its ShutDown failure arm falls into the SAME 7-instruction
// epilogue as the loop exit, so retail has 2 returns to our 3. Measured and
// rejected: the goto-loop spelling (`retries = 0; retry: if (retries > 5) goto
// failed; ... ++retries; goto retry; failed: return false;`), which is
// BYTE-IDENTICAL to the `for` - VC6 constant-folds the head test away because
// `retries = 0` immediately dominates it, then rotates, so no source form
// reachable from a zero initialiser reproduces the top test. The doctrine that
// VC6 does not rotate goto flow does not survive a foldable guard.
// 2026-09-06, polish lane 35. Of the two deltas the SECOND one was not a
// schedule at all: `Send` returns `unsigned char` (dxplay.h:214) and landing
// it in a `bool` is a NARROWING CONVERSION, so VC6 normalized AL through
// `setne cl` and then tested CL at both use sites. Retail tests AL itself
// twice, which only a byte-typed receiver produces. `unsigned char sent`
// 86.9847 -> 88.8489. The rotation survives and is measured again here: a
// `for (;;)` with an explicit `if (retries > 5)` head test is byte-flat at
// 88.8489 (VC6 recognizes the induction variable and rotates it back into a
// loop guard exactly as it does the `for`), and routing BOTH failure exits
// through one `goto failed;` label does merge the ShutDown arm into the
// shared epilogue but leaves the rotated loop's own fall-through `xor al,al`
// tail behind - still 3 returns, still 88.8489. The surplus return is a
// CONSEQUENCE of the rotation, not an independent merge to spell.
// Polish 49 adds the third loop form to that list: `int retries = 0;
// while (retries <= 5) { ...; ++retries; }` is byte-identical at 88.8550.
VA(0x005533d0, 0x1AB)  // anchor-strings + virtual-slots + dc-order-map
bool CDPlayHeroes::sendIt(CNetMsg* msg, unsigned long dpidTo,
                          bool guaranteed)
{
    char errorDescription[256];
    int retries;
    for (retries = 0; retries <= 5; ++retries) {
        unsigned char sent = send(msg, msg->m_size, g_thisNetPlayerInfo.m_dpid,
                                  dpidTo, guaranteed);
        if ((!sent
             && getLastError() == DPLAY_SEND_ERROR_INVALID_PLAYER)
            || getLastError() == DPLAY_SEND_ERROR_INVALID_PARAMETER) {
            getErrorDesc(getLastError(), errorDescription);
            g_logFile.log(DATA_COMPGEN(0x00682adc, dplaySendErrorLog,
                                    "DPlay Send error [%s]"),
                        errorDescription);
            g_logFile.log(DATA_COMPGEN(0x00682abc, invalidSendPlayerLog,
                                    "Sending to invalid player? [%d]"),
                        dpidTo);
            if (getLastError() == DPLAY_SEND_ERROR_INVALID_PLAYER) {
                destroyPlayer(dpidTo);
                handlePlayerDrop(dpidTo);
            }
            return true;
        }

        if (!sent) {
            getErrorDesc(getLastError(), errorDescription);
            g_logFile.log(DATA_COMPGEN(0x00682adc, dplaySendErrorLog,
                                    "DPlay Send error [%s]"),
                        errorDescription);
            GameTime::delay(200);

            if (retries >= 5) {
                normalDialogTimeOut(
                    g_generalText->getText(GENERAL_TEXT_DPLAY_SEND_RETRY),
                    2, 15000, -1, -1, -1, 0, -1, 0, -1, -1, 0);
                if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT) {
                    shutDown(0);
                    return false;
                }
                retries = -1;
            }
        }
        else {
            return true;
        }
    }
    return false;
}

// Original: CDPlayHeroes::HandleHostXFer; remote.cpp:685, dc 0x11c1d8
void CDPlayHeroes::handleHostXFer()
{
    CSetAsHostMsg msg;
    queueMsg(&msg);
}

// Original: CDPlayHeroes::HandleNewPlayer; remote.cpp:697, dc 0x11c228
void CDPlayHeroes::handleNewPlayer(unsigned long, char*, void*, unsigned long)
{
}

VA(0x00553580, 0x1F0)
void CDPlayHeroes::handlePlayerDrop(unsigned long dpid)
{
    g_logFile.log(DATA_COMPGEN(0x00682a78, playerDroppedLog,
                            "********Player dropped---->[%d]"),
                dpid);
    CPlayerDropMsg msg(dpid);
    queueMsg(&msg);
}

// E:\gamedcs\remote.cpp:702. Retail keeps no standalone copy. /Ob2 expands
// allocation/copy into both member drop paths; the standalone handler also
// expands Dinkumware's push_back internals, while SendIt's nested occurrence
// stops at that template boundary.
void CDPlayHeroes::queueMsg(CNetMsg* netMsg)
{
    void* storage = ::operator new(netMsg->m_size);
    memcpy(storage, netMsg, netMsg->m_size);
    m_msgQueue.push_back(static_cast<CNetMsg*>(storage));
}

// DC788 records both guard tests together, followed by Copy at 789. The
// combined condition keeps this body exact but costs 62 VC6 inline units;
// nested ifs cost 71. WaitForReadyToPlayMsg's ready-path cleanup has budget 64:
// this source form expands SetNetMsgHandler and retains Copy exactly as retail.
VA(0x00553770, 0x30)  // dc 0x11c268
void CDPlayHeroes::setNetMsgHandler(CNetMsgHandler* netMsgHandler)
{
    CNetMsgHandler* old = m_netMsgHandler;
    m_netMsgHandler = netMsgHandler;
    if (old && m_netMsgHandler)
        m_netMsgHandler->copy(old);
}

VA(0x005537a0, 0x7)  // dc 0x11c290
CNetMsgHandler* CDPlayHeroes::getNetMsgHandler()
{
    return m_netMsgHandler;
}

// Original: CChatManager::CChatManager; remote.cpp:804, dc 0x11c298
// Retail initializer 0x5521c0 constructs g_chatMan with 20 lines. It adds
// the Miles handle at +0x28 between isSysMsg and the five sample stores.
CChatManager::CChatManager(int maxChatLines)
{
    m_currMsg = 0;
    m_pauseTime = 0;
    m_changed = 1;
    m_lastWidget = 0;
    m_maxLines = maxChatLines;
    m_widgetText = 0;
    m_msgArray = 0;
    m_position = -1;
    m_chatKilled = 0;
    m_isSysMsg = 0;
    m_chatMemSample = 0;
    m_chatSample = 0;
    m_playerDropSample = 0;
    m_sysMsgSample = 0;
    m_turnDurSample = 0;
    m_playerEnterSample = 0;
    setMaxLines(maxChatLines);
}

// Original: CChatManager::~CChatManager; remote.cpp:829, dc 0x11c314
// Retail's registered cleanup 0x552240 deletes these two arrays in order.
CChatManager::~CChatManager()
{
    delete[] m_widgetText;
    delete[] m_msgArray;
}

VA(0x005537b0, 0x46)  // dc 0x11c330
void CChatManager::init()
{
    m_chatSample = ResourceManager::getSample(
        DATA_COMPGEN(0x00682b30, chatSampleName, "chat.wav"));
    m_playerDropSample = ResourceManager::getSample(
        DATA_COMPGEN(0x00682b20, playerDropSampleName, "playexit.wav"));
    m_sysMsgSample = ResourceManager::getSample(
        DATA_COMPGEN(0x00682b14, systemMessageSampleName, "sysmsg.wav"));
    m_turnDurSample = ResourceManager::getSample(
        DATA_COMPGEN(0x00682b04, turnDurationSampleName, "timeover.wav"));
    m_playerEnterSample = ResourceManager::getSample(
        DATA_COMPGEN(0x00682af4, playerEnterSampleName, "playcome.wav"));
}

VA(0x00553800, 0x31)  // dc 0x11c374
void CChatManager::shutDown()
{
    if (m_chatSample) {
        m_chatSample->dispose();
        m_playerDropSample->dispose();
        m_sysMsgSample->dispose();
        m_turnDurSample->dispose();
        m_playerEnterSample->dispose();
    }
}

VA(0x00553840, 0x11B)  // dc 0x11c3a8
void CChatManager::addChat(const char* format, ...)
{
    char chatText[1024];
    va_list args;
    va_start(args, format);
    vsprintf(chatText, format, args);

    bool atNewestMessage = false;
    if (m_position == m_msgCount - 1)
        atNewestMessage = true;

    if (m_msgCount >= 20) {
        m_msgArray[m_currMsg].m_killTime = 0;
        m_currMsg = (m_currMsg + 1) % m_maxLines;
        --m_msgCount;
    }

    int msgNbr = getNextFreeMsgNbr();
    strncpy(m_msgArray[msgNbr].m_text, chatText, 127);
    m_msgArray[msgNbr].m_killTime = 0;
    m_msgArray[msgNbr].m_isSystem = m_isSysMsg;
    ++m_msgCount;
    if (atNewestMessage)
        m_position = m_msgCount - 1;
    m_changed = 1;

    if (!m_isSysMsg) {
        if (m_chatMemSample
            && g_soundManager->getSampleInfo(
                m_chatMemSample, AIL_SAMPLE_PLAYING))
            return;
        sample* chatSample = m_chatSample;
        if (chatSample) {
            int soundWasEnabled = g_soundManager->m_playSounds;
            g_soundManager->m_playSounds = 1;
            m_chatMemSample =
                g_soundManager->memorySample(chatSample);
            g_soundManager->m_playSounds = soundWasEnabled;
        }
    }
}

// E:\gamedcs\remote.cpp:904
// Variadic member ownership follows DC; the stack receiver is VC6 ABI.
// The display guard is IsClose(59000) with the
// adventure-suspended and popup checks; the sound tail prefers timeover.wav
// and falls back to chat.wav. A positive display scope removes the
// skip-chat goto with identical VC6 scores throughout this TU, retaining
// the common sound tail and the order of the short-circuit time checks.
VA(0x00553960, 0x136)  // anchor-callees + arity/order-map, dc 0x11c4ac
void __cdecl CChatManager::turnDurationMsg(const char* format, ...)
{
    char chatText[1024];
    char finalText[1024];
    va_list args;
    va_start(args, format);
    vsprintf(chatText, format, args);

    unsigned char canDisplay = 1;
    if (g_advManager
        && g_advManager->m_status == baseManager::STATUS_SUSPENDED)
        canDisplay = 0;

    if (g_dPlay) {
        CNetMsgHandler* handler = g_dPlay->getNetMsgHandler();
        if (handler && handler->isInPopup())
            canDisplay = 0;
    }

    // DC line 918 calls CTurnDuration::IsClose; preserve the helper
    // instead of accessing its protected timer fields from this formatter.
    if (!g_turnDuration.isClose(59000) || canDisplay) {

        sprintf(
            finalText,
            DATA_COMPGEN(0x00660358, turnDurationLineFormat, "%s%s"),
            g_generalText->getText(GENERAL_TEXT_TURN_DURATION_PREFIX),
            chatText);
        m_isSysMsg = 1;
        addChat(finalText);
        m_isSysMsg = 0;
    }

    sample* sampleToPlay = m_turnDurSample;
    if (m_chatMemSample
        && g_soundManager->getSampleInfo(
            m_chatMemSample, AIL_SAMPLE_PLAYING))
        return;
    if (!sampleToPlay)
        sampleToPlay = m_chatSample;
    if (sampleToPlay) {
        int soundWasEnabled = g_soundManager->m_playSounds;
        g_soundManager->m_playSounds = 1;
        m_chatMemSample = g_soundManager->memorySample(sampleToPlay);
        g_soundManager->m_playSounds = soundWasEnabled;
    }
}

VA(0x00553aa0, 0xC0)  // dc 0x11c558
void __cdecl CChatManager::systemMsg(const char* format, ...)
{
    char chatText[1024];
    char finalText[1024];
    va_list args;
    va_start(args, format);
    vsprintf(chatText, format, args);
    sprintf(
        finalText,
        DATA_COMPGEN(0x00660358, turnDurationLineFormat, "%s%s"),
        g_generalText->getText(GENERAL_TEXT_TURN_DURATION_PREFIX),
        chatText);

    m_isSysMsg = 1;
    addChat(finalText);
    sample* sampleToPlay = m_sysMsgSample;
    m_isSysMsg = 0;

    if (m_chatMemSample
        && g_soundManager->getSampleInfo(
            m_chatMemSample, AIL_SAMPLE_PLAYING))
        return;
    if (!sampleToPlay)
        sampleToPlay = m_chatSample;
    if (sampleToPlay) {
        int soundWasEnabled = g_soundManager->m_playSounds;
        g_soundManager->m_playSounds = 1;
        m_chatMemSample = g_soundManager->memorySample(sampleToPlay);
        g_soundManager->m_playSounds = soundWasEnabled;
    }
}

VA(0x00553b60, 0xCA)  // dc 0x11c5bc
void CChatManager::playerDropMsg(const char* format, ...)
{
    char chatText[1024];
    char finalText[1024];
    va_list args;
    va_start(args, format);
    vsprintf(chatText, format, args);
    sprintf(
        finalText,
        DATA_COMPGEN(0x00660358, turnDurationLineFormat, "%s%s"),
        g_generalText->getText(GENERAL_TEXT_TURN_DURATION_PREFIX),
        chatText);

    m_isSysMsg = 1;
    addChat(finalText);
    sample* sampleToPlay = m_playerDropSample;

    if (!(m_chatMemSample
          && g_soundManager->getSampleInfo(
              m_chatMemSample, AIL_SAMPLE_PLAYING))) {
        if (!sampleToPlay)
            sampleToPlay = m_chatSample;
        if (sampleToPlay) {
            int soundWasEnabled = g_soundManager->m_playSounds;
            g_soundManager->m_playSounds = 1;
            m_chatMemSample =
                g_soundManager->memorySample(sampleToPlay);
            g_soundManager->m_playSounds = soundWasEnabled;
        }
    }
    m_isSysMsg = 0;
}

VA(0x00553c30, 0xCA)  // dc 0x11c658
void __cdecl CChatManager::playerEnterMsg(const char* format, ...)
{
    char chatText[1024];
    char finalText[1024];
    va_list args;
    va_start(args, format);
    vsprintf(chatText, format, args);
    sprintf(
        finalText,
        DATA_COMPGEN(0x00660358, turnDurationLineFormat, "%s%s"),
        g_generalText->getText(GENERAL_TEXT_TURN_DURATION_PREFIX),
        chatText);

    m_isSysMsg = 1;
    addChat(finalText);
    sample* sampleToPlay = m_playerEnterSample;

    if (!(m_chatMemSample
          && g_soundManager->getSampleInfo(
              m_chatMemSample, AIL_SAMPLE_PLAYING))) {
        if (!sampleToPlay)
            sampleToPlay = m_chatSample;
        if (sampleToPlay) {
            int soundWasEnabled = g_soundManager->m_playSounds;
            g_soundManager->m_playSounds = 1;
            m_chatMemSample =
                g_soundManager->memorySample(sampleToPlay);
            g_soundManager->m_playSounds = soundWasEnabled;
        }
    }
    m_isSysMsg = 0;
}

// DC's UpdateWidget public encodes native bool for killOld; the retained
// PC body tests that byte, and all authored callers supply 0 or 1.
VA(0x00553d00, 0xA1)  // dc 0x11c6bc
void CChatManager::updateWidget(textWidget* widget, bool killOld, int numLines)
{
    if (m_pauseTime == 0) {
        updateNewChat();
        if (killOld)
            killOldChat();
    }
    if (m_changed || widget != m_lastWidget) {
        m_lastWidget = widget;
        updateWidgetText(numLines, widget);
        widget->setText(m_widgetText);
        m_changed = 0;
    }
}

// E:\gamedcs\remote.cpp:1060
// E:\gamedcs\remote.cpp:1060/1065. DC records these named source helpers.
// AddChat uses the first canonical helper; retail /Ob2 also expands the
// second at both surviving KillOldChat call sites.
int CChatManager::getNextFreeMsgNbr()
{
    return (m_currMsg + m_msgCount) % m_maxLines;
}

// E:\gamedcs\remote.cpp:1065.
// Retail KillOldChat expands this helper at both surviving call sites.
int CChatManager::getNextMsgNbr(int msgNbr)
{
    return (msgNbr + 1) % m_maxLines;
}

VA(0x00553db0, 0x33)  // dc 0x11c754
unsigned char CChatManager::hasOldChat()
{
    if (m_msgCount == 0)
        return 0;
    unsigned long killTime = m_msgArray[m_currMsg].m_killTime;
    return static_cast<long>(GameTime::get() - killTime) > 20000;
}

VA(0x00553df0, 0xE4)  // dc 0x11c7b0
void CChatManager::killOldChat()
{
    m_chatKilled = 0;
    if (m_msgCount != 0) {
        GameTime::get();
        unsigned long killTime = m_msgArray[m_currMsg].m_killTime;
        if (static_cast<unsigned long>(GameTime::elapsedSince(killTime))
                > 20000) {
            m_msgArray[m_currMsg].m_killTime = 0;
            m_currMsg = getNextMsgNbr(m_currMsg);
            --m_msgCount;
            m_changed = 1;
            m_chatKilled = 1;

            int msgNbr = m_currMsg;
            int i = 0;
            while (i < m_msgCount - 1) {
                unsigned long nextKillTime = m_msgArray[msgNbr].m_killTime;
                long elapsed = GameTime::elapsedSince(nextKillTime);
                if (elapsed <= 20000)
                    break;
                if (!m_msgArray[msgNbr].m_isSystem)
                    m_msgArray[msgNbr].m_killTime += 10000;
                msgNbr = getNextMsgNbr(msgNbr);
                i++;
            }
        }
        m_position = m_msgCount - 1;
    }
}

// Original: CChatManager::UpdateNewChat; remote.cpp:1115, dc 0x11c87c
void CChatManager::updateNewChat()
{
    int msgNbr = m_currMsg;
    for (int i = 0; i < m_msgCount; ++i) {
        if (m_msgArray[msgNbr].m_killTime == 0)
            m_msgArray[msgNbr].m_killTime = GameTime::get();
        msgNbr = getNextMsgNbr(msgNbr);
    }
}

VA(0x00553ee0, 0x163)  // dc 0x11c8d0
void CChatManager::updateWidgetText(int numLines, textWidget* widget)
{
    int lineCounts[20];

    m_widgetText[0] = 0;
    if (m_msgCount == 0)
        return;
    if (m_position == -1)
        m_position = m_msgCount - 1;

    int lastMsg = m_position;
    int firstMsg = m_position - numLines + 1;
    if (firstMsg < 0)
        firstMsg = 0;

    int msgNbr = (firstMsg + m_currMsg) % m_maxLines;
    int totalLines = 0;
    int lineNbr = 0;
    int i;
    for (i = firstMsg; i <= lastMsg; i++) {
        lineCounts[lineNbr] =
            widget->m_font->lineLength(m_msgArray[msgNbr].m_text, widget->m_width);
        msgNbr = (msgNbr + 1) % m_maxLines;
        totalLines += lineCounts[lineNbr];
        lineNbr++;
    }

    lineNbr = 0;
    while (totalLines > numLines) {
        totalLines -= lineCounts[lineNbr];
        firstMsg++;
        lineNbr++;
    }

    msgNbr = (firstMsg + m_currMsg) % m_maxLines;
    for (i = firstMsg; i <= lastMsg;) {
        strcat(m_widgetText, m_msgArray[msgNbr].m_text);
        if (i < lastMsg)
            strcat(m_widgetText, DATA_COMPGEN(0x006603bc, chatLineBreak, "\n"));
        i++;
        msgNbr = (msgNbr + 1) % m_maxLines;
        if (msgNbr == m_currMsg)
            break;
    }
}

VA(0x00554050, 0xD)  // dc 0x11c9fc
void CChatManager::pauseTimeOuts()
{
    m_pauseTime = GameTime::get();
}

VA(0x00554060, 0x4B)  // dc 0x11ca14
void CChatManager::resumeTimeOuts()
{
    for (int i = 0; i < 20; i++) {
        if (m_msgArray[i].m_killTime > 0) {
            unsigned long pausedAt = m_pauseTime;
            unsigned long elapsed = GameTime::get() - pausedAt;
            m_msgArray[i].m_killTime += elapsed;
        }
    }
    m_pauseTime = 0;
}

// Original: CChatManager::HasChat; remote.cpp:1213, dc 0x11ca60
unsigned char CChatManager::hasChat()
{
    return m_msgCount > 0;
}

VA(0x005540b0, 0x20)  // dc 0x11ca70
void CChatManager::clearChat()
{
    m_msgCount = 0;
    m_changed = 1;
    for (int i = 0; i < 20; i++)
        m_msgArray[i].m_killTime = 0;
}

VA(0x005540d0, 0x9C)  // dc 0x11cabc
void CChatManager::setMaxLines(int maxChatLines)
{
    if (m_widgetText)
        delete[] m_widgetText;
    if (m_msgArray)
        delete[] m_msgArray;
    m_widgetText = new char[m_maxLines * 127];
    m_msgArray = new CChatStr[m_maxLines];
    clearChat();
}

VA(0x00554170, 0x23)  // dc 0x11cb24
void CChatManager::setPosition(int newPos)
{
    if (newPos < 0)
        newPos = m_msgCount - 1;
    if (newPos >= m_msgCount)
        newPos = m_msgCount - 1;
    m_position = newPos;
    m_changed = 1;
}

VA(0x005541a0, 0x5A)  // dc 0x11cb48
CChatEdit::CChatEdit(int x, int y, int w, int h, int textSize, char* text,
    char* fontName, font::TColor color, font::EJustify justification,
    char* backgroundIcon, int backgroundFrame, int id, int style,
    int readType, int insetX, int insetY)
    : textEntryWidget(x, y, w, h, textSize, text, fontName, color,
          justification, backgroundIcon, backgroundFrame, id, style,
          readType, insetX, insetY)
{
}

VA(0x00554200, 0x36)  // dc 0x11cbf4
void CChatEdit::updateScreen()
{
    draw();
    g_windowManager->updateScreen(
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
}

VA(0x00554240, 0xEA)  // dc 0x11cc2c
int CChatEdit::onKeyPress(message* msg)
{
    int key = getCharPressed(msg);
    switch (key) {
        case KEYCODE_ENTER:
            return onEnter(*msg);
        case KEYCODE_ESCAPE:
            return onEscape(*msg);
        case KEYCODE_F1:
        case KEYCODE_F2:
        case KEYCODE_F3:
        case KEYCODE_F4:
        case KEYCODE_F5:
        case KEYCODE_F6:
        case KEYCODE_F7:
        case KEYCODE_F8:
            return onFunctionKey(*msg, key - KEYCODE_F1);
    }

    int result = textEntryWidget::onKeyPress(msg);
    updateScreen();
    return result;
}

VA(0x00554330, 0x44)  // dc 0x11cd14
int CChatEdit::onFunctionKey(message msg, int toWho)
{
    if (m_text.size() > 0)
        sendChat(m_text.c_str(), toWho);
    setupDisplayString(
        DATA_COMPGEN(0x00691210, chatEditEmptyText, ""), 0);
    updateScreen();
    return 1;
}

VA(0x00554380, 0x3E)  // dc 0x11cd64
int CChatEdit::onEnter(message msg)
{
    if (m_text.size() > 0)
        sendChat(m_text.c_str(), NET_MESSAGE_RECIPIENT_ALL);
    setupDisplayString(
        DATA_COMPGEN(0x00691210, chatEditEmptyText, ""), 0);
    updateScreen();
    return 1;
}

VA(0x005543c0, 0x1F)  // dc 0x11cdb0
int CChatEdit::onEscape(message msg)
{
    setupDisplayString(
        DATA_COMPGEN(0x00691210, chatEditEmptyText, ""), 0);
    updateScreen();
    return 1;
}

VA(0x005543e0, 0x9)  // dc 0x11cddc
bool CChatEdit::isOpen()
{
    if (m_text.size() > 0)
        return true;
    return false;
}

VA(0x005543f0, 0x5)  // dc 0x11cdf8
unsigned char CChatEdit::ignoreKey(message* msg)
{
    return 0;
}

VA(0x00554400, 0xF)  // dc 0x11cdfc
CNetMsg* getRemoteData(unsigned char removeFromQueue,
                       unsigned char* wasCompressed)
{
    return g_dPlay->getRemoteData(removeFromQueue, 0);
}

VA(0x00554410, 0x93)
unsigned char initRemote(eNetGameType mpType, const char* userName)
{
    CNetPlayerInfo playerInfo;

    g_gameMode = static_cast<unsigned char>(mpType);
    g_followPlayerMode = 0;
    g_weMoved = 0;

    playerInfo.m_dpid = 0;
    playerInfo.m_name[0] = 0;
    playerInfo.m_version = *g_videoGameState;
    g_thisNetPlayerInfo = playerInfo;

    strcpy(g_config.m_networkDefaultName, userName);
    strcpy(g_thisNetPlayerInfo.m_name,
           g_config.m_networkDefaultName);
    writePrefs();
    g_remoteOn = 1;
    return 1;
}

VA(0x005544b0, 0xAA)
void remoteCleanup()
{
    g_gameMode = 0;
    g_mpNetProtocol = MP_SINGLE;
    g_chatMan.clearChat();

    if (g_remoteOn) {
        if (g_dPlay) {
            if (g_thisNetPlayerInfo.m_dpid)
                g_dPlay->destroyPlayer(g_thisNetPlayerInfo.m_dpid);
            g_dPlay->closeSession();
            delete g_dPlay;
            g_dPlay = 0;
        }

        g_remoteOn = 0;
        {
            CNetPlayerInfo playerInfo;
            playerInfo.m_dpid = 0;
            playerInfo.m_name[0] = 0;
            playerInfo.m_version = *g_videoGameState;
            g_thisNetPlayerInfo = playerInfo;
        }
    }
}

VA(0x00554560, 0x82)
int transmitRemoteDataDPID(CNetMsg* msg, unsigned long dpidTo,
                           bool compressMsg, bool guaranteed)
{
    if (g_remoteOn && g_dPlay)
        return g_dPlay->transmitRemoteDataDPID(
            msg, dpidTo, compressMsg, guaranteed);
    return 0;
}

VA(0x005545f0, 0xBA)
int transmitRemoteData(CNetMsg* msg, int toWho,
                       bool compressMsg, bool guaranteed)
{
    if (g_remoteOn && g_dPlay)
        return g_dPlay->transmitRemoteData(
            msg, toWho, compressMsg, guaranteed);
    return 0;
}

VA(0x005546b0, 0x103)
void pollRemote()
{
    if (g_gameOver || !g_remoteOn || !g_dPlay)
        return;

    if (g_currentPlayer && g_currentPlayer->isLocalHuman()) {
        if (g_lastActiveUpdate == 0) {
            g_lastActiveUpdate = GameTime::get();
        } else if (GameTime::elapsedSince(g_lastActiveUpdate)
                   > g_playerActiveUpdateInterval) {
            g_lastActiveUpdate = GameTime::get();
            CPlayerActiveMsg msg;
            transmitRemoteDataDPID(&msg, 0, false, false);
        }
    } else {
        g_lastActiveUpdate = 0;
    }

    g_dPlay->pollRemote();
}

VA(0x005547c0, 0x25A)  // dc 0x11d020
void sendChat(const char* chatString, int toWho)
{
    if (_strcmpi(chatString,
                 g_generalText->getText(GENERAL_TEXT_CHAT_PING_COMMAND)) == 0) {
        if (toWho == NET_MESSAGE_RECIPIENT_ALL) {
            g_chatMan.systemMsg(g_generalText->getText(GENERAL_TEXT_CHAT_PING_ALL));
        } else {
            g_chatMan.systemMsg(g_generalText->getText(GENERAL_TEXT_CHAT_PING_PLAYER_FORMAT),
                g_game->getPlayerName(toWho));
        }

        CPingMsg msg(GameTime::get(), RS_PING);
        transmitRemoteData(&msg, toWho, false, false);
        return;
    }

    char transformedChat[256];
    const char* outgoingChat = chatString;

    if (toWho != NET_MESSAGE_RECIPIENT_ALL) {
        playerData* recipient = &g_game->m_players[toWho];
        char* recipientName = recipient->m_name;
        if (!recipient->isHuman())
            recipientName = DATA_COMPGEN(
                0x00682b54, chatUnknownRecipient, "???");

        g_chatMan.addChat(
            DATA_COMPGEN(0x00682b44, chatNonHumanLineFormat,
                         "%s: (%s:%s) %s"),
            g_game->getPlayerName(g_game->getLocalPlayerGamePos()),
            g_generalText->getText(GENERAL_TEXT_CHAT_NONHUMAN_LINE_TAG),
            recipientName,
            chatString);
        sprintf(
            transformedChat,
            DATA_COMPGEN(0x00682b3c, chatNonHumanWireFormat, "(%s) %s"),
            g_generalText->getText(GENERAL_TEXT_CHAT_NONHUMAN_WIRE_TAG),
            chatString);
        outgoingChat = transformedChat;
    } else {
        g_chatMan.addChat(
            DATA_COMPGEN(0x00682ab4, chatPlayerLineFormat, "%s: %s"),
            g_game->getPlayerName(g_game->getLocalPlayerGamePos()),
            chatString);
    }

    CChatMsg msg(outgoingChat);
    transmitRemoteData(&msg, toWho, false, false);
}

VA(0x00554a20, 0x21)  // dc 0x11d1c8
void receiveChat(char* chatString, int fromPlayer)
{
    g_chatMan.addChat(DATA_COMPGEN(0x00682ab4, chatPlayerLineFormat, "%s: %s"),
        g_game->getPlayerName(fromPlayer), chatString);
}

VA(0x00554a50, 0x22)  // dc 0x11d1ec
CAnimatedDlg::CAnimatedDlg()
    : CTextDialog(0x12)
{
    m_sprite = 0;
    m_spriteFrame = 0;
    m_lastTick = 0;
    m_palUpdated = 0;
}

VA_COMPGEN(0x00554a80, 0x21, SCALAR_DELETING_DTOR, CAnimatedDlg)

// E:\gamedcs\remote.cpp:1547, dc 0x11d250
// WaitForReadyToPlayMsg calls this destructor out of line.
VA(0x00554ab0, 0x55)  // dc 0x11d250
CAnimatedDlg::~CAnimatedDlg()
{
    if (m_sprite)
        m_sprite->dispose();
}

VA(0x00554b10, 0x20)  // dc 0x11d290
unsigned char CAnimatedDlg::setup(
    const char* text, font* textFont, const char* spriteName, int sequence)
{
    m_spriteName = spriteName;
    m_seq = sequence;
    return CTextDialog::setup(text, textFont);
}

VA(0x00554b30, 0xF5)  // dc 0x11d2b0
void CAnimatedDlg::calcSpriteDimensions(
    CSprite* sprite, int& maxWidth, int& maxHeight, int& minY)
{
    minY = 999;
    int minX = 999;
    int width = 0;
    int height = 0;

    int numFrames = sprite->getNumFrames(m_seq);
    CSpriteFrame* firstFrame = sprite->getFrame(m_seq, 0);
    int baseX = firstFrame->getCroppedX();
    int baseY = firstFrame->getCroppedY();

    for (int i = 0; i < numFrames; ++i) {
        CSpriteFrame* frame = sprite->getFrame(m_seq, i);
        int frameHeight = frame->getCroppedHeight();
        int frameWidth = frame->getCroppedWidth();
        int frameX = frame->getCroppedX() - baseX;
        int frameY = frame->getCroppedY() - baseY;
        if (frameX < minX)
            minX = frameX;
        if (frameY < minY)
            minY = frameY;
        if (frameX + frameWidth > width)
            width = frameX + frameWidth;
        if (frameY + frameHeight > height)
            height = frameY + frameHeight;
    }

    maxWidth = width - minX;
    maxHeight = height - minY;
}

VA(0x00554c30, 0xB8)  // dc 0x11d394
void CAnimatedDlg::calcDimensions(
    const char* text, font* textFont, int& winX, int& winY,
    int& winWidth, int& winHeight)
{
    CTextDialog::calcDimensions(
        text, textFont, winX, winY, winWidth, winHeight);

    m_sprite = ResourceManager::getSprite(m_spriteName);
    int spriteWidth;
    int spriteHeight;
    int minY;
    calcSpriteDimensions(m_sprite, spriteWidth, spriteHeight, minY);

    if (spriteWidth > winWidth) {
        winWidth = spriteWidth + 40;
        winX = (800 - winWidth) / 2;
    }
    winHeight += 20;
    m_spriteX = (winWidth - spriteWidth) / 2;
    m_spriteY = winHeight - minY;
    winHeight = m_spriteY + spriteHeight + 20;
    winX = (800 - winWidth) / 2;
    winY = (600 - winHeight) / 2;
}

VA(0x00554cf0, 0x94)  // dc 0x11d490
void CAnimatedDlg::drawSprite()
{
    int s0x = m_sprite->getCroppedX(m_seq, m_spriteFrame);
    int s0y = m_sprite->getCroppedY(m_seq, m_spriteFrame);
    int sw = m_sprite->getCroppedWidth(m_seq, m_spriteFrame);
    int sh = m_sprite->getCroppedHeight(m_seq, m_spriteFrame);
    int dx = m_x + m_spriteX + s0x - m_sprite->getCroppedX(m_seq, 0);
    int dy = m_y + m_spriteY + s0y - m_sprite->getCroppedY(m_seq, 0);
    m_sprite->drawCreature(
        m_seq, m_spriteFrame, s0x, s0y, sw, sh,
        g_windowManager->m_screenBitmap, dx, dy, 0, 0);
}

VA(0x00554d90, 0x80)  // dc 0x11d558
int CAnimatedDlg::handleMessage(message& msg)
{
    tickAnimation();
    return 0;
}

VA(0x00554e10, 0x7C)  // dc 0x11d56c
void CAnimatedDlg::tickAnimation()
{
    unsigned long currentTime = GameTime::get();
    unsigned long lastTick = m_lastTick;
    if (static_cast<long>(GameTime::get() - lastTick) >= 200) {
        m_spriteFrame = (m_spriteFrame + 1)
                      % m_sprite->getNumFrames(m_seq);
        m_lastTick = currentTime;
        drawWindow(0, 0xffff0001, 0xffff);
        g_windowManager->updateScreen(m_x, m_y, m_width, m_height);
    }
}

VA(0x00554e90, 0x7D)  // dc 0x11d5dc
void CAnimatedDlg::drawWindow(unsigned char update, int lowID, int highID)
{
    if (!m_palUpdated) {
        if (g_game->getLocalPlayer()) {
            for (int id = m_beginId; id <= m_endId; ++id)
                broadcastMessage(
                    0x200, widget::WIDGET_SET_PLAYER_PALETTE_COLORS,
                    id, g_game->getLocalPlayerGamePos());
        }
        m_palUpdated = 1;
    }

    heroWindow::drawWindow(update, lowID, highID);
    heroWindow::drawWindow(update, lowID, highID);
    drawSprite();
}

// E:\gamedcs\remote.cpp:1708. The order is byte-proven by the constructor's
// expansion in WaitForReadyToPlayMsg: clear/mark playerReady first, then clear
// the timestamps. Dreamcast independently gives the member identities.
CWaitForReadyPlayersDlg::CWaitForReadyPlayersDlg()
{
    memset(m_playerReady, 0, sizeof(m_playerReady));
    m_playerReady[g_localGamePos] = 1;
    m_startTime = 0;
    m_lastMsg = 0;
}

// E:\gamedcs\remote.cpp:1718
// DC1729/1734 preserve the text subscript and TransmitRemoteData wrappers;
// do not flatten the latter's network-active/DirectPlay guard into this helper.
void CWaitForReadyPlayersDlg::wait()
{
    m_startTime = GameTime::get();
    sRand(m_startTime);

    int creature;
    do {
        // Complete calls Random (retail 0x554f10+0x13b); DC1723 calls SRandom.
        creature = random(0, 111);
    } while (creature == CREATURE_ARCH_DEVIL
             || creature == CREATURE_DEVIL);

    setup(g_generalText->getText(GENERAL_TEXT_WAIT_FOR_READY_PLAYERS), g_mediumFont,
          g_creatureTypeTraits[creature].m_spriteName, 0);
    doModal(0);

    if (g_dPlay->isHost()) {
        CAllReadyToPlayMsg msg;
        transmitRemoteData(&msg, 127, false, true);
    }
}

// E:\gamedcs\remote.cpp:1798. Dreamcast's `_N` return mangling proves bool;
// retail independently proves the eight-entry human/ready scan when inlined.
bool CWaitForReadyPlayersDlg::allPlayersReady()
{
    for (int i = 0; i < 8; ++i) {
        if (g_game->isHuman(i) && !m_playerReady[i])
            return 0;
    }
    return 1;
}

// E:\gamedcs\remote.cpp:1813 - forget the dropped player's DirectPlay
// identity and close only once every remaining human has checked in.
int CWaitForReadyPlayersDlg::onPlayerDrop(CNetMsg* netMsg, message& msg)
{
    int gamePos = g_game->getGamePosFromDPID(netMsg->m_dpidFrom);
    if (gamePos != -1)
        g_game->m_players[gamePos].clearNetInfo();
    if (allPlayersReady())
        return exitDialog(msg);
    return 0;
}

// E:\gamedcs\remote.cpp:1829 - the readiness fast path avoids opening the
// modal at all. Constructor, AllPlayersReady, Wait and the complete
// destructor chain are all expanded into this retail body by /Ob2.

// Residual 92.5776%: SetNetMsgHandler's DC-shaped combined guard restores all
// sixteen retail call sites and Copy's exact retained body without an inline
// fence. The implicit derived destructor is also exact (keep it implicit:
// DC compgenx). Remaining differences concern the shared zero and EH-state
// registers: candidate uses EBX for zero, retail EDI and EBX for EH states.
// Eight palette/ready/return boolean-literal combinations emit one identical
// object; why-reg finds no named caller-local value to reorder. DC's palette
// flag is T_UCHAR, so changing its type to bool would discard source evidence.
// Earlier controls: inverted Wait/return contradicts DC's condition/return
// scope; a depth-zero return calls the entire derived destructor (70.4658%).
VA(0x00554f10, 0x23A)  // anchor-vtable + dc-order-map, dc 0x11d6c8
void waitForReadyToPlayMsg()
{
    CWaitForReadyPlayersDlg dlg;
    if (dlg.allPlayersReady()) {
        return;
    }
    dlg.wait();
}

VA(0x00555190, 0x319)  // dc 0x11f9f0
int CWaitForReadyPlayersDlg::handleMessage(message& msg)
{
    CAnimatedDlg::handleMessage(msg);
    pollSound();

    if (GameTime::elapsedSince(m_lastMsg) > 1000) {
        if (!g_dPlay->isHost()) {
            CReadyToPlayMsg readyMsg;
            transmitRemoteData(&readyMsg, 127, false, true);
            m_lastMsg = GameTime::get();
        }
    }

    if (GameTime::elapsedSince(m_startTime) >= 3000) {
        CNetMsg* netMsg = getRemoteData(1, 0);
        if (netMsg) {
            CMessageKill killMsg(netMsg);
            switch (netMsg->m_subType) {
                case RS_PLAYER_DROPPED:
                    return onPlayerDrop(netMsg, msg);

                case RS_SET_AS_HOST:
                    g_chatMan.systemMsg(g_generalText->getText(GENERAL_TEXT_LOCAL_PLAYER_IS_HOST));
                    break;

                case RS_SESSION_LOST:
                    normalDialog(g_generalText->getText(GENERAL_TEXT_REMOTE_SESSION_DESTROYED),
                                 1, -1, -1, -1, 0, -1, 0,
                                 -1, 0, -1, 0);
                    shutDown(0);
                    break;

                case RS_READY_TO_PLAY:
                    m_playerReady[netMsg->m_from] = 1;
                    if (allPlayersReady() && g_dPlay->isHost())
                        return exitDialog(msg);
                    break;

                case RS_ALL_READY_TO_PLAY:
                    return exitDialog(msg);

                case RS_CHAT_MSG:
                    receiveChat(static_cast<CChatMsg*>(netMsg)->m_text,
                                netMsg->m_from);
                    break;
            }
        }
    }
    return 0;
}

// E:\gamedcs\remote.cpp:1820 - CWaitForReadyPlayersDlg's compiler-generated
// deleting destructor, slot 0 of vtable 0x640ecc. DC marks the following
// implicit destructor compgenx: it must not acquire an authored body.
VA_COMPGEN(0x005554b0, 0x21, SCALAR_DELETING_DTOR,
           CWaitForReadyPlayersDlg)

// E:\gamedcs\remote.cpp:1820 - implicit destructor called by ??_G above.
// The full nested teardown is expanded in retail. Removing CAnimatedDlg's
// auto-inline fence restores this generated body to 100%; the readiness
// caller's distinct expansion decisions remain debt beside that caller.
VA_COMPGEN(0x005554e0, 0xC9, IMPLICIT_DTOR, CWaitForReadyPlayersDlg)

VA(0x005555b0, 0x126)  // dc 0x11d708
unsigned char createDPlayObject()
{
    if (g_dPlay)
        return 1;

    g_dPlay = new CDPlayHeroes;
    if (!g_dPlay->init())
        return 0;

    g_dPlay->setGuid(guidHeroes3);
    g_logFile.log(DATA_COMPGEN(0x00682b58, remoteDPlayInitialized,
                             "DPlay initialized"));
    return 1;
}

VA(0x005556e0, 0x224)  // dc 0x11d770
unsigned char initConnection(char* ipAddressOrPhoneNbr,
                             _DPCOMPORTADDRESS* comportInfo)
{
    if (!createDPlayObject()) {
        dpsd(0,
             DATA_COMPGEN(0x00682bb8, remoteSourceFile,
                          "C:\\Dev\\Heroes 3 Exp 2\\Game\\Remote.cpp"),
             1867);
        return 0;
    }

    CDPlayConnection* connection = 0;
    switch (g_mpNetProtocol) {
        case MP_TCP:
            if (ipAddressOrPhoneNbr)
                connection = g_dPlay->createTCPIPConnection(
                    ipAddressOrPhoneNbr,
                    DATA_COMPGEN(0x00682ba4, remoteTCPIPConnection,
                                 "TCP/IP Connection"),
                    0);
            else
                connection = g_dPlay->createTCPIPConnection(
                    "",
                    DATA_COMPGEN(0x00682ba4, remoteTCPIPConnection,
                                 "TCP/IP Connection"),
                    0);
            break;

        case MP_IPX:
            connection = g_dPlay->createIPXConnection(
                DATA_COMPGEN(0x00682b94, remoteIPXConnection,
                             "IPX Connection"),
                0);
            break;

        case MP_MODEM:
            connection = g_dPlay->createModemConnection(
                DATA_COMPGEN(0x00682b80, remoteModemConnection,
                             "Modem Connection"),
                ipAddressOrPhoneNbr,
                0);
            break;

        case MP_SERIAL:
            connection = g_dPlay->createSerialConnection(
                DATA_COMPGEN(0x00682b6c, remoteSerialConnection,
                             "Serial Connection"),
                comportInfo);
            break;
    }

    if (!connection) {
        dpsd(0,
             DATA_COMPGEN(0x00682bb8, remoteSourceFile,
                          "C:\\Dev\\Heroes 3 Exp 2\\Game\\Remote.cpp"),
             1898);
        return 0;
    }

    if (!g_dPlay->initConnection(connection)) {
        delete connection;
        return 0;
    }

    delete connection;
    return 1;
}

VA(0x00555910, 0x08)  // dc 0x11d8ec
void destroyMsg(CNetMsg* netMsg)
{
    delete netMsg;
}

// DC ?TestIfLobbyLaunched@@YA_NXZ proves the native bool return; retail
// forwards the already-boolean TestLobbied result or returns 0/1.
VA(0x00555920, 0x171)  // dc 0x11d900
bool testIfLobbyLaunched()
{
    HKEY key;
    char appName[256];
    char fileName[256];
    char commandLine[256];
    char executableName[256];

    if (g_mPlayer)
        return 1;
    if (!createDPlayObject())
        return 0;
    if (!g_dPlay)
        return 0;

    char* registryKey = DATA_COMPGEN(
        0x00640ca0, remoteDPlayRegistryKey,
        "SOFTWARE\\Microsoft\\DirectPlay\\Applications\\Heroes of Might and Magic III");
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, registryKey, 0, KEY_READ, &key)
        == ERROR_SUCCESS) {
        RegCloseKey(key);
        return g_dPlay->testLobbied();
    }

    char* appNameStart = strrchr(registryKey, '\\');
    ++appNameStart;
    strcpy(appName, appNameStart);
    strcpy(fileName,
           DATA_COMPGEN(0x00640cec, remoteDPlayIcdName, "Heroes3.icd"));
    commandLine[0] = 0;
    strcpy(executableName,
           DATA_COMPGEN(0x00640cf8, remoteDPlayExecutableName,
                        "Heroes3.exe"));

    g_dPlay->registerApp(
        appName, fileName, commandLine, guidHeroes3,
        executableName[0] ? executableName : 0);
    return g_dPlay->testLobbied();
}

// E:\gamedcs\remote.cpp:1960 - the retail PC path adds the MPlayer host/client
// bootstrap around the same DC function.  The client explicitly clears its
// enumerated session pointers before leaving the loop; the automatic
// CAutoArray destructor is consequently redundant on the success path but
// remains visible on the two failure exits.

// Residual wall (98.3167%, 2026-08-22): all 24 branches and seven returns
// agree.  Our VC6 retains the success-edge destructor's dead vptr store plus
// three already-zero member stores (16 bytes); retail drops all four and
// keeps only the EH-state close.  Constructor order, bool-vs-byte, chained
// zeroing, inner-vs-outer deleteData guards, and the seven guided why-reg
// mutations all plateau here.  The other two destructor exits, including
// the redundant Destroy(1) call after failed JoinSession, match retail and
// forbid changing the class lifetime merely to erase this one compiler
// artifact.
VA(0x00555aa0, 0x443)  // anchor-IAT/vtable/data + dc-xref/order-map, dc 0x11d9ac
unsigned char handleMPlayerLaunch()
{
    g_logFile.log(DATA_COMPGEN(0x00682c50, remoteMPlayerDetected,
                            "Detected MPlayer launch."));

    g_numHumanPlayers = 1;
    g_mpBaseType = 1;
    g_mpNetProtocol = MP_TCP;

    if (g_mPlayerHost) {
        g_logFile.log(DATA_COMPGEN(0x00682c34, remoteMPlayerHost,
                                "We are the MPlayer host."));
        if (!initConnection(
                DATA_COMPGEN(0x00691210, remoteMPlayerEmptyAddress, ""), 0))
            return 0;
        if (!g_dPlay->hostSession(
                DATA_COMPGEN(0x00682c24, remoteMPlayerSession,
                             "MPlayer Session"),
                4, 8, 0))
            return 0;
    } else {
        g_logFile.log(DATA_COMPGEN(0x00682c00, remoteMPlayerClient,
                                "We are a MPlayer client. Host IP=%s"),
                    g_tcpAddress);
        if (!initConnection(g_tcpAddress, 0))
            return 0;

        bool connected = false;
        CAutoArray<CDPlaySession> sessions;
        for (int attempt = 0; attempt < 3; ++attempt) {
            if (g_dPlay->enumSessions(&sessions, 0, 2)
                && sessions.getCount() > 0) {
                if (!g_dPlay->joinSession(
                        &sessions.get(0)->m_guidInstance, 0)) {
                    sessions.destroy(1);
                    return 0;
                }
                connected = true;
                break;
            }
            Sleep(1000);
        }
        sessions.destroy(1);

        if (!connected) {
            normalDialog(g_generalText->getText(GENERAL_TEXT_RECONNECT_FAILED), 1,
                         -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            remoteCleanup();
            return 0;
        }
    }

    initRemote(MP_TCP, g_config.m_networkDefaultName);

    int version = *g_videoGameState;
    g_thisNetPlayerInfo.m_dpid = g_dPlay->createPlayer(
        g_config.m_networkDefaultName, &version, sizeof(version), 0);
    if (!g_thisNetPlayerInfo.m_dpid)
        return 0;

    g_thisNetPlayerInfo.m_version = version;
    g_logFile.log(DATA_COMPGEN(0x00682be0, remoteMPlayerConnected,
                            "Successful MPlayer connection."));
    strcpy(g_thisNetPlayerInfo.m_name,
           g_config.m_networkDefaultName);
    return 1;
}

VA(0x00555ef0, 0x3E4)
unsigned char lobbyLaunchConnect()
{
    strcpy(g_mapName, g_game->m_setup.m_filename);
    g_game->m_mapHeader.get(
        g_game->m_setup.m_path, g_game->m_setup.m_filename, 0);

    if (g_mPlayer) {
        if (handleMPlayerLaunch())
            return 1;
        remoteCleanup();
        return 0;
    }

    g_logFile.log(DATA_COMPGEN(0x00682d7c, lobbyGettingSettings,
                            "Getting connection settings..."));
    DPLCONNECTION* connection =
        g_dPlay->getConnectionSettings(0, 0);
    if (!connection)
        return 0;

    if (connection->m_flags & DPLAY_CONNECTION_CREATE_SESSION) {
        connection->m_sessionDesc->m_flags |=
            DPLAY_SESSION_MIGRATE_HOST | DPLAY_SESSION_KEEP_ALIVE;
        g_logFile.log(DATA_COMPGEN(0x00682d68, lobbyIsHost,
                                "We are the host...."));
    } else {
        g_logFile.log(DATA_COMPGEN(0x00682d54, lobbyIsGuest,
                                "We are a guest...."));
    }
    g_logFile.log(DATA_COMPGEN(0x00682d2c, lobbyMaxPlayers,
                            "CDPlayLobby::Connect - MaxPlayers: %d"),
                connection->m_sessionDesc->m_maxPlayers);
    g_logFile.log(DATA_COMPGEN(0x00682d00, lobbyCurrentPlayers,
                            "CDPlayLobby::Connect - CurrentPlayers: %d"),
                connection->m_sessionDesc->m_currentPlayers);

    if (!g_dPlay->setConnectionSettings(0, connection)) {
        g_logFile.log(DATA_COMPGEN(0x00682cdc, lobbySetSettingsError,
                                "Error setting connection settings!"));
        delete connection;
        return 0;
    }

    CHourGlass hourGlass(1);
    g_logFile.log(DATA_COMPGEN(0x00682cc4, lobbyAttemptingConnect,
                            "Attempting connect..."));
    if (!g_dPlay->connect()) {
        g_logFile.log(DATA_COMPGEN(0x00682ca0, lobbyConnectError,
                                "Error connecting to lobby session!"));
        char description[256];
        g_dPlay->getErrorDesc(g_dPlay->getLastError(), description);
        g_logFile.log(DATA_COMPGEN(0x00682c90, lobbyLastError,
                                "Last error=[%s]"), description);
        delete connection;
        return 0;
    }

    g_logFile.log(DATA_COMPGEN(0x00682c7c, lobbyConnected,
                            "Successful connect!"));
    strncpy(g_config.m_networkDefaultName,
            connection->m_playerName->m_shortNameA, 21);
    delete connection;
    g_config.m_networkDefaultName[21] = 0;
    g_logFile.log(DATA_COMPGEN(0x00682c6c, lobbyUserName,
                            "Username=[%s]"),
                g_config.m_networkDefaultName);

    g_mpNetProtocol = MP_TCP;
    g_numHumanPlayers = 1;
    g_mpBaseType = 1;
    initRemote(MP_TCP, g_config.m_networkDefaultName);
    int version = *g_videoGameState;
    g_thisNetPlayerInfo.m_dpid = g_dPlay->createPlayer(
        g_config.m_networkDefaultName, &version, sizeof(version), 0);
    if (!g_thisNetPlayerInfo.m_dpid)
        return 0;
    g_thisNetPlayerInfo.m_version = version;
    return 1;
}

// E:\gamedcs\remote.cpp:2150. Retail has no surviving out-of-line copy:
// HandlePlayerDrop expands the eight-player DPID search and consumes -1 as
// its not-found sentinel.
int getPlayerPos(unsigned long dpid)
{
    for (int i = 0; i < 8; ++i) {
        if (g_game->m_players[i].m_dpid == dpid)
            return i;
    }
    return -1;
}

// E:\gamedcs\remote.cpp:2161. Retail has no surviving out-of-line copy:
// both HandleNewHost expansions keep the candidate in a register, wrap at
// zero and ask game::IsHuman until they find the prior human seat.
int getPriorPlayer(int gamePos)
{
    do {
        --gamePos;
        if (gamePos < 0)
            gamePos = 7;
    } while (!g_game->isHuman(gamePos));
    return gamePos;
}

// E:\gamedcs\remote.cpp:2174. DC supplies this source boundary and the
// CDPlayPlayer member names. Retail expands it into UpdateCurrentPlayers:
// virtual GetCount/Get calls remain, while GetId becomes the +0x100 load.
static inline unsigned char isValidHuman(
    CAutoArray<CDPlayPlayer>& playerArray, unsigned long dpid)
{
    for (unsigned long i = 0; i < playerArray.getCount(); ++i) {
        if (playerArray.get(i)->getId() == dpid)
            return 1;
    }
    return 0;
}

VA(0x005562e0, 0x14E)  // dc 0x11df10
void updateCurrentPlayers()
{
    CAutoArray<CDPlayPlayer> playerArray;
    g_dPlay->enumPlayers(&playerArray, 0, 0);

    for (int i = 0; i < 8; ++i) {
        if (g_game->m_players[i].m_dpid
            && isValidHuman(playerArray, g_game->m_players[i].m_dpid))
            continue;

        g_game->m_players[i].m_dpid = 0;
        g_game->m_players[i].m_isHuman = 0;
        g_game->m_players[i].m_isLocal = 0;
        strcpy(g_game->m_players[i].m_name,
               g_generalText->getText(GENERAL_TEXT_DEFAULT_PLAYER_NAME));
    }

    g_numHumanPlayers = playerArray.getCount();
    playerArray.destroy(1);
}

VA(0x00556430, 0x1A1)  // dc 0x11e01c
void handlePlayerDrop(unsigned long dpid)
{
    int playerPos = getPlayerPos(dpid);
    if (playerPos == -1)
        return;

    g_logFile.log(DATA_COMPGEN(0x00682df8, handlingPlayerDropLog,
                            "Handling player drop [%d]"),
                dpid);
    g_chatMan.playerDropMsg(
                  g_generalText->getText(GENERAL_TEXT_PLAYER_DROPPED),
                  g_game->m_players[playerPos].m_name);
    updateCurrentPlayers();

    if (g_playerTurn == playerPos
        && !g_game->m_playerDisabled[playerPos]) {
        int priorPlayer = getPriorPlayer(g_playerTurn);
        g_netLocalGamePos = priorPlayer;
        g_playerTurn = priorPlayer;

        if (g_dPlay->isHost()) {
            if (g_netLocalGamePos == g_game->getLocalPlayerGamePos()) {
                g_logFile.log(DATA_COMPGEN(
                                0x00682dc8, hostWasLastPlayerLog,
                                "Host [%d] was last player... time to recover..."),
                            g_thisNetPlayerInfo.m_dpid);
                onPlayerDropUpdateMsg(dpid);
            } else {
                g_logFile.log(DATA_COMPGEN(
                                0x00682d9c, playerWasLastPlayerLog,
                                "%d was last player... time to recover..."),
                            dpid);
                CPlayerDropUpdateMsg msg(dpid);
                transmitRemoteData(
                    &msg, g_playerTurn, false, true);
            }
        }
    }
}

// E:\gamedcs\remote.cpp:2289. DC publishes this free-function boundary and
// both call edges; retail /Ob2 expands it into CLevelPickWaitDlg's dispatcher
// and CNetMsgHandler::HandleNetMsg, leaving no standalone body. The two PC
// copies agree on every global, message field, and call.
void handleNewHost()
{
    g_logFile.log(DATA_COMPGEN(0x00682e14, handleNewHostLog,
                            "HandleNewHost"));
    if (!g_game->isHuman(g_playerTurn)) {
        g_netLocalGamePos = getPriorPlayer(g_netLocalGamePos);
        if (g_netLocalGamePos == g_game->getLocalPlayerGamePos()) {
            onPlayerDropUpdateMsg(-1);
        } else {
            CPlayerDropUpdateMsg msg(-1);
            transmitRemoteData(
                &msg, g_netLocalGamePos, false, true);
        }
    }
    g_chatMan.systemMsg(g_generalText->getText(GENERAL_TEXT_LOCAL_PLAYER_IS_HOST));
}

// E:\gamedcs\remote.cpp:2317. Dreamcast supplies the public boundary and
// local CTextDialog/CHourGlass types. Retail independently fixes the recovery
// filenames, general-text row, player-record updates and the two resume arms.
// Residual (98.70642%): a 72-variant ordinary-source tree found this highest
// pointer-lifetime form. Retail acquires its cached ESI after the first local
// seat query; moving the declaration there reproduces the mask core but keeps
// the pointer alive through NextPlayer and scores 98.11926%. Retail also folds
// the empty CTextDialog destructor to its TDialogBox base; exposing that body
// in the header regresses both this function and an exact caller, so it is
// rejected. The refreshed structure pass has all 8 edges aligned (only B4 is
// one instruction smaller); predict-inline independently isolates that same
// TDialogBox cleanup as the sole real over-inline call boundary.
VA(0x005565e0, 0x19E)  // anchor-string + callgraph + dc-order-map, dc 0x11e1cc
void onPlayerDropUpdateMsg(unsigned long dpid)
{
    g_logFile.log(DATA_COMPGEN(0x00682e24, playerDropUpdateLog,
                            "OnPlayerDropUpdateMsg (%d)"),
                dpid);

    g_mouseManager->setPointer(1, mouseManager::ADVENTURE_SET);
    CTextDialog dlg(0x12);
    dlg.setup(g_generalText->getText(GENERAL_TEXT_PLAYER_DROP_RELOAD),
              g_mediumFont);
    dlg.open(0, 1);
    g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);

    CHourGlass hourGlass(1);
    if (!g_game->loadGame(g_config.m_scFile, 0, 0))
        g_game->loadGame(g_config.m_rcFile, 0, 0);

    dlg.close(1);
    hourGlass.stop();
    updateCurrentPlayers();

    int playerPos = g_game->getGamePosFromDPID(dpid);
    if (playerPos != -1)
        g_game->m_players[playerPos].clearNetInfo();

    // Retail keeps this gpGame read live across the two local-seat queries.
    game* currentGame = g_game;
    int localPlayer = currentGame->getLocalPlayerGamePos();
    g_netLocalGamePos = localPlayer;
    g_playerTurn = localPlayer;
    g_currentPlayer = &g_game->m_players[localPlayer];
    g_curPlayerBit = 1 << localPlayer;

    int visiblePlayer = currentGame->getLocalPlayerGamePos();
    g_curWatchPlayer = visiblePlayer;
    g_mapVisibilityBit = 1 << visiblePlayer;

    if (g_weMoved) {
        g_playerDrop = 1;
        g_game->nextPlayer();
    } else {
        g_advManager->startLocalPlayerTurn();
    }
}

VA(0x00556780, 0x1C0)  // dc 0x11e39c
void handlePlayerDead(int deadGuy, unsigned char showMsg)
{
    g_game->m_playerDisabled[deadGuy] = 1;

    if (deadGuy == g_localGamePos) {
        remoteCleanup();

        if (showMsg) {
            strcpy(g_text, g_generalText->getText(GENERAL_TEXT_LOCAL_PLAYER_DEFEATED));
            normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0,
                         -1, 0, -1, 0);
        }

        g_defeatedAllPlayers = 0;
        g_gameOver = 1;
    } else {
        if (!g_goSolo && showMsg) {
            sprintf(g_text, g_generalText->getText(GENERAL_TEXT_PLAYER_DEFEATED_FORMAT),
                    g_game->getPlayerName(deadGuy));
            normalDialog(g_text, 1, -1, -1, 10, deadGuy, -1, -1,
                         -1, 5000, -1, 0);
        }

        if (deadGuy == g_playerTurn) {
            int nextPlayer = getNextHumanPlayer(deadGuy);
            g_currentPlayer = &g_game->m_players[nextPlayer];
            g_netLocalGamePos = nextPlayer;
            g_playerTurn = nextPlayer;
        }
    }
}

VA(0x00556940, 0x5E)  // dc 0x11e494
void handlePlayerWon(CNetMsg* netMsg)
{
    CPlayerWonMsg* message = static_cast<CPlayerWonMsg*>(netMsg);
    g_game->m_mapHeader.m_victoryCondition = message->m_victoryCondition;

    int gameLost;
    int gameWon;
    gameWon = 0;
    gameLost = 0;
    g_gameOver = 1;
    displayVCWinLoss(message->m_victoryCondition,
                     gameWon, gameLost, true);
    if (gameLost)
        g_defeatedAllPlayers = 0;
    if (gameWon)
        g_defeatedAllPlayers = 1;
}

VA(0x005569a0, 0x4B)  // dc 0x11e500
void handlePlayerLost(CNetMsg* netMsg)
{
    CPlayerLostMsg* message = static_cast<CPlayerLostMsg*>(netMsg);
    int gameLost;
    int gameWon;
    gameWon = 0;
    gameLost = 0;
    displayLCWinLoss(message->m_lossCondition,
                     gameWon, gameLost, 1);
    if (gameLost)
        g_defeatedAllPlayers = 0;
    if (gameWon)
        g_defeatedAllPlayers = 1;
}

VA(0x005569f0, 0xB4)  // dc 0x11e598
void handleNormalWinMsg(CNetMsg* netMsg)
{
    CNormalWinMsg* message = static_cast<CNormalWinMsg*>(netMsg);
    g_gameOver = 1;
    int localPlayer = g_game->getLocalPlayerGamePos();

    if (g_game->onSameTeam(message->m_gamePos, localPlayer)) {
        normalDialog(g_generalText->getText(GENERAL_TEXT_TEAM_VICTORY), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        g_defeatedAllPlayers = 1;
        g_normalVictory = 1;
    } else {
        normalDialog(g_generalText->getText(GENERAL_TEXT_TEAM_DEFEAT), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        g_defeatedAllPlayers = 0;
    }
}

VA(0x00556ab0, 0xB6)  // dc 0x11e630
CLevelPickWaitDlg::CLevelPickWaitDlg()
{
    m_fromWho = -1;
    m_playerDropped = 0;
}

VA(0x00556ba0, 0x72)  // dc 0x11e6a4
void CLevelPickWaitDlg::waitForLevels(int fromWho)
{
    m_fromWho = fromWho;
    sRand(GameTime::get());

    int creature;
    do {
        creature = random(0, 111);
    } while (creature == CREATURE_ARCH_DEVIL
             || creature == CREATURE_DEVIL);

    setup(g_generalText->getText(GENERAL_TEXT_WAIT_FOR_LEVEL_SELECTION), g_mediumFont,
          g_creatureTypeTraits[creature].m_spriteName, 0);
    doModal(0);
}

VA_COMPGEN(0x00556b70, 0x21, SCALAR_DELETING_DTOR, CLevelPickWaitDlg)

VA(0x00556c20, 0x2F5)  // dc 0x11e718
int CLevelPickWaitDlg::handleMessage(message& msg)
{
    CAnimatedDlg::handleMessage(msg);
    pollSound();

    CNetMsg* netMsg = getRemoteData(1, 0);
    if (netMsg) {
        CMessageKill msgKill(netMsg);
        switch (netMsg->m_subType) {
            case RS_HERO_LEVEL_UPDATE:
                onHeroLevelUpdate(netMsg);
                return exitDialog(msg);

            case RS_PLAYER_DROPPED:
                return onPlayerDrop(netMsg, msg);

            case RS_SET_AS_HOST:
                handleNewHost();
                break;

            case RS_SESSION_LOST:
                normalDialog(g_generalText->getText(GENERAL_TEXT_REMOTE_SESSION_DESTROYED),
                             1, -1, -1, -1, 0, -1, 0,
                             -1, 0, -1, 0);
                shutDown(0);
                break;

            case RS_CHAT_MSG:
                receiveChat(static_cast<CChatMsg*>(netMsg)->m_text,
                            netMsg->m_from);
                break;
        }
    }
    return 0;
}

// E:\gamedcs\remote.cpp:2546. Retail expands this source boundary into the
// dispatcher. A drop from the player whose level choice is pending marks the
// modal and closes it; every other drop is still handed to the global handler.
int CLevelPickWaitDlg::onPlayerDrop(CNetMsg* netMsg, message& msg)
{
    int gamePos = g_game->getGamePosFromDPID(netMsg->m_dpidFrom);
    if (gamePos == m_fromWho) {
        m_playerDropped = 1;
        handlePlayerDrop(netMsg->m_dpidFrom);
        return exitDialog(msg);
    }
    handlePlayerDrop(netMsg->m_dpidFrom);
    return 0;
}

// The incoming level-update packet restores the raw four-byte skill band,
// rather than the clamped gameplay accessor. Name provisional; retain an
// ordinary body before its caller so VC6 can expand the copy.
void hero::setPrimarySkills(const signed char* stats)
{
    memcpy(m_stats, stats, sizeof(m_stats));
}

// E:\gamedcs\remote.cpp:2567. DC names the message fields; retail proves
// their offsets by copying the 28 secondary-skill levels and four primary
// stats into the selected hero before replacing the secondary-skill count.
void CLevelPickWaitDlg::onHeroLevelUpdate(CNetMsg* netMsg)
{
    CHeroLevelUpdateMsg* levelMsg =
        static_cast<CHeroLevelUpdateMsg*>(netMsg);
    hero* targetHero = g_game->getHero(levelMsg->m_hero);
    if (targetHero) {
        memcpy(targetHero->m_skillLevel, levelMsg->m_ssLevel,
               sizeof(levelMsg->m_ssLevel));
        targetHero->setPrimarySkills(levelMsg->m_stats);
        targetHero->m_skillCount = levelMsg->m_numSSs;
    }
}

VA(0x00556f20, 0x13C)  // dc 0x11e8e0
CWaitForRemoteBattleDlg::CWaitForRemoteBattleDlg()
{
    m_playerPos = 0;
    m_combatInitMsgReceived = 0;
}

// E:\gamedcs\remote.cpp:2595 - slot 0 of vtable 0x640f78. Its ordinary
// destructor is the events.obj body at 0x4aea00; this is the generated
// delete-flag wrapper emitted beside the vtable-owning constructor.
VA_COMPGEN(0x00557060, 0x21, SCALAR_DELETING_DTOR,
           CWaitForRemoteBattleDlg)

VA(0x00557090, 0x5C)  // dc 0x11e948
void CWaitForRemoteBattleDlg::wait(int playerPos)
{
    m_playerPos = playerPos;
    int creature = random(0, 111);
    setup(g_generalText->getText(GENERAL_TEXT_WAIT_FOR_REMOTE_BATTLE), g_mediumFont,
          g_creatureTypeTraits[creature].m_spriteName, 12);
    doModal(0);
}

VA(0x005570f0, 0x1E9)  // dc 0x11e9a0
int CWaitForRemoteBattleDlg::handleMessage(message& msg)
{
    CAnimatedDlg::handleMessage(msg);
    pollSound();

    CNetMsg* netMsg = getRemoteData(1, 0);
    if (netMsg) {
        CMessageKill killMsg(0);
        if (netMsg->m_subType != RS_COMBAT_INIT)
            killMsg.setMessage(netMsg);

        switch (netMsg->m_subType) {
            case RS_PLAYER_DROPPED:
                return onPlayerDrop(netMsg, msg);

            case RS_SET_AS_HOST:
                g_chatMan.systemMsg(g_generalText->getText(GENERAL_TEXT_LOCAL_PLAYER_IS_HOST));
                break;

            case RS_SESSION_LOST:
                normalDialog(g_generalText->getText(GENERAL_TEXT_REMOTE_SESSION_DESTROYED),
                             1, -1, -1, -1, 0, -1, 0,
                             -1, 0, -1, 0);
                shutDown(0);
                break;

            case RS_COMBAT_INIT:
                m_combatInitMsg.remoteFn00512E00(netMsg);
                m_combatInitMsgReceived = 1;
                return exitDialog(msg);

            case RS_CHAT_MSG:
                receiveChat(static_cast<CChatMsg*>(netMsg)->m_text,
                            netMsg->m_from);
                break;
        }
    }
    return 0;
}

// E:\gamedcs\remote.cpp:2660. Retail expands the helper into the dispatcher:
// it resolves and processes every dropped DPID, but closes this modal only
// when the dropped player is the combat peer it is waiting for.
int CWaitForRemoteBattleDlg::onPlayerDrop(CNetMsg* netMsg, message& msg)
{
    int gamePos = g_game->getGamePosFromDPID(netMsg->m_dpidFrom);
    handlePlayerDrop(netMsg->m_dpidFrom);
    if (gamePos == m_playerPos)
        return exitDialog(msg);
    return 0;
}

VA(0x005572e0, 0x2D)  // dc 0x11eb40
CSaveScreen::CSaveScreen(int w, int h)
    : Bitmap16Bit(w, h)
{
    m_screenSaved = 0;
    m_x = 0;
    m_y = 0;
}

VA_COMPGEN(0x00557310, 0x21, SCALAR_DELETING_DTOR, CSaveScreen)
VA_COMPGEN(0x00557340, 0x05, IMPLICIT_DTOR, CSaveScreen)

VA(0x00557350, 0x3A)  // dc 0x11eb9c
void CSaveScreen::save(int x, int y)
{
    m_x = x;
    m_y = y;
    m_screenSaved = 1;
    grab(g_windowManager->m_screenBitmap, x, y);
}

VA(0x00557390, 0x69)  // dc 0x11ebc8
void CSaveScreen::restore(unsigned char update)
{
    if (m_screenSaved) {
        draw(0, 0, getWidth(), getHeight(), g_windowManager->m_screenBitmap,
             m_x, m_y, 0);
        if (update)
            g_windowManager->updateScreen(m_x, m_y, getWidth(), getHeight());
    }
}

VA(0x00557400, 0x4)  // dc 0x11ec5c
unsigned char CSaveScreen::isSaved()
{
    return m_screenSaved;
}

void showVideo(int id, int x, int y, int w, int h, int a6, bool a7, bool a8);

VA(0x00557410, 0x1E)  // dc 0x11ec64
CGameTransferSmack::CGameTransferSmack()
{
    m_x = 0;
    m_y = 0;
    m_lastFrame = -1;
    m_started = 0;
    m_saveScreen = 0;
    m_sending = 0;
    m_drawText = 1;
}

VA(0x00557430, 0x22)  // dc 0x11ec88
CGameTransferSmack::~CGameTransferSmack()
{
    stop();
    delete m_saveScreen;
}

VA(0x00557460, 0x1E)  // dc 0x11ecbc
void CGameTransferSmack::setup(int x, int y, unsigned char sending,
                               unsigned char drawText)
{
    m_x = x;
    m_y = y;
    m_sending = sending;
    m_drawText = drawText;
}

VA(0x00557480, 0x25)  // dc 0x11ecd8
void CGameTransferSmack::start()
{
    m_started = 1;
    showVideo(0x3f, m_x, m_y, 160, 160, 0, 0, 0);
}

// DrawCurrentFrame is defined in remote.cpp:2784 in DC; the Windows
// helper below calls the current-handle video wrapper at 0x598e80.
VA(0x005574b0, 0x12D)  // dc 0x11ece4
void CGameTransferSmack::setPercentage(float pct)
{
    m_lastFrame = static_cast<int>(pct * 20.0f);
    SmackManager::gotoSmackerFrame(m_lastFrame);
    drawCurrentFrame();

    char text[256];
    char percentageText[256];
    if (m_sending)
        strcpy(text, g_generalText->getText(GENERAL_TEXT_SENDING_GAME));
    else
        strcpy(text, g_generalText->getText(GENERAL_TEXT_RECEIVING_GAME));
    sprintf(percentageText,
            DATA_COMPGEN(0x00682e40, transferPercentageFormat, "\n%0.0f%%"),
            pct * 100.0f);
    strcat(text, percentageText);

    if (m_drawText) {
        g_mediumFont->drawBoundedString(
            text, g_windowManager->m_screenBitmap, m_x, m_y, 160, 160,
            font::PRIMARY, 5, -1);
    }
    g_windowManager->updateScreen(m_x, m_y, 160, 160);
}

// DC retains an empty body on the console; retail SetPercentage calls the
// Windows video draw wrapper through this source helper.
// E:\gamedcs\remote.cpp:2784, dc 0x11ede8
inline void CGameTransferSmack::drawCurrentFrame()
{
    SmackManager::drawSmackerFrame();
}

// E:\gamedcs\remote.cpp:2789
VA(0x005575e0, 0x15)  // dc 0x11edec
void CGameTransferSmack::stop()
{
    if (m_started) {
        SmackManager::closeSmacker();
        m_started = 0;
    }
}

VA(0x00557600, 0xAB)  // dc 0x11ee04
void CGameTransferSmack::saveScreen()
{
    if (!m_saveScreen)
        m_saveScreen = new CSaveScreen(160, 160);
    m_saveScreen->save(m_x, m_y);
}

VA(0x005576b0, 0x61)  // dc 0x11ee3c
void CGameTransferSmack::restoreScreen()
{
    if (m_saveScreen)
        m_saveScreen->restore(1);
}

VA(0x00557720, 0x3C)  // dc 0x11ee54
CGameTransferDlg::CGameTransferDlg(unsigned char sending)
    : CTextDialog(0x12)
{
    m_sending = sending;
}

VA_COMPGEN(0x00557760, 0x21, SCALAR_DELETING_DTOR, CGameTransferDlg)

VA(0x00557790, 0x51)  // dc 0x11eed8
void CGameTransferDlg::calcDimensions(const char* text, font* currentFont,
                                      int& winX, int& winY,
                                      int& winWidth, int& winHeight)
{
    winHeight = 160;
    winWidth = 160;
    winX = 320;
    winY = (600 - winHeight) / 2;
    m_smack.setup(winX + 15, winY + 15, m_sending, 1);
}

// E:\gamedcs\remote.cpp:1293
VA(0x005577f0, 0x11)  // dc 0x11ef34
CNetMsgHandler::CNetMsgHandler()
{
    m_inPopup = 0;
    m_abortPopupMsg = 0;
}

// Complete's CNetMsgHandler::`scalar deleting destructor', slot 0 of
// vtable 0x640f14. It is 0x45 rather than the usual 0x21 because
// the destructor below is small and NOT EH-bearing, so /Ob2 inlines it here
// while still emitting it out of line.
VA_COMPGEN(0x00557810, 0x45, SCALAR_DELETING_DTOR, CNetMsgHandler)

VA(0x00557860, 0x70)  // dc 0x11ef60
CNetMsg* CNetMsgHandler::checkHandleNet(unsigned char inPopup,
                                        unsigned char* msgReceived)
{
    if (msgReceived)
        *msgReceived = 0;
    m_inPopup = inPopup;
    if (m_abortPopupMsg) {
        if (inPopup) {
            *msgReceived = 1;
            return 0;
        }
        CNetMsg* abortMsg = m_abortPopupMsg;
        m_abortPopupMsg = 0;
        return handleNetMsg(abortMsg);
    }
    CNetMsg* netMsg = g_dPlay->getRemoteData(1, 0);
    if (netMsg == 0)
        return 0;
    if (msgReceived)
        *msgReceived = 1;
    return handleNetMsg(netMsg);
}

VA(0x005578d0, 0x30)  // anchor-vtable: slot 0 chain of 0x640f14; Complete-only
CNetMsgHandler::~CNetMsgHandler()
{
    if (g_dPlay && g_dPlay->getNetMsgHandler() == this)
        g_dPlay->setNetMsgHandler(0);
}

// E:\gamedcs\remote.cpp:2875
VA(0x00557910, 0xD)  // dc 0x11efcc
void CNetMsgHandler::setAbortPopupMsg(CNetMsg* netMsg)
{
    m_abortPopupMsg = netMsg;
}

VA(0x00557920, 0x157)  // dc 0x11efd0
CNetMsg* CNetMsgHandler::handleNetMsg(CNetMsg* netMsg)
{
    switch (netMsg->m_subType) {
    case RS_SET_AS_HOST:
        if (m_inPopup) {
            m_abortPopupMsg = netMsg;
            return 0;
        }
        handleNewHost();
        break;

    case RS_SESSION_LOST:
        normalDialog(g_generalText->getText(GENERAL_TEXT_REMOTE_SESSION_DESTROYED), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        shutDown(0);
        break;
    }

    if (netMsg)
        destroyMsg(netMsg);
    return 0;
}

// Original: CTurnDuration::CTurnDuration; remote.cpp:2912, dc 0x11f060
// Retail initializer 0x5522b0 leaves nextWarning untouched, as does DC.
CTurnDuration::CTurnDuration()
{
    m_lastWarned = 0;
    m_currDuration = 0;
    m_turnStartTime = 0;
    m_pauseTime = 0;
}

VA(0x00557a80, 0x15)  // dc 0x11f070
unsigned char CTurnDuration::isOn()
{
    // DC remote.cpp:2921/2922 and 2925 retain two separate early-outs.
    // This canonical body matches both the retained function and its
    // expansion in isClose; a compound boolean return changes VC6 lowering.
    if (m_currDuration == 0)
        return 0;
    if (g_inCampaign)
        return 0;
    return 1;
}

VA(0x00557aa0, 0x4D)  // dc 0x11f090
unsigned char CTurnDuration::isExpired()
{
    if ((!g_currentPlayer || g_currentPlayer->isLocalHuman())
            && m_currDuration != 0
            && !g_inCampaign
            && m_pauseTime <= 0) {
        unsigned long startTime = m_turnStartTime;
        if (startTime > 0
                && GameTime::get() - startTime > m_currDuration)
            return 1;
    }
    return 0;
}

// E:\gamedcs\remote.cpp:2950
// Residual (97.09%): flow-distance 0, register-distance 38 - one
// caller-saved permutation at the head of the re-arm block. Retail parks
// m_currDuration in ECX and the half in EAX; this compile parks them the
// other way round, and that single swap is what makes our two `timeLeft`
// arms reassociate to `m_currDuration - currTime + m_turnStartTime` where
// retail keeps `m_turnStartTime - currTime + m_currDuration`. `why-reg
// --model` reports the creation-order lever copy-propagated (C1 handle
// state, capped); naming the half as a local does not move it.

// Three spellings ARE byte-load-bearing and were found the hard way:
//   * `((m_currDuration >> 1) << 1) > 120000` and NOT the semantically
//     identical `(m_currDuration / 2) * 2`. C1XX folds a shift pair into
//     `and x,-2` in the front end, before C2 ever sees it; the divide form
//     stays a DIVIDE/MULTIPLY pair that C2 lowers only AFTER CSE-ing the
//     `/2` it shares with the two neighbouring uses, so it comes out as
//     `lea [ecx+ecx]` off the shared half (96.16 against 97.09).
//   * `m_lastWarned` must be READ INTO A LOCAL between the two
//     GameTime::Get() calls. Retail's `mov edi,[esi]` sits between them,
//     and no compiler may hoist a member load across an opaque call - so
//     the load is in source order there, not in the subtraction (95.26
//     against 93.95).
//   * the last arm assigns 1000 in the THEN arm and 0 in the ELSE. VC6
//     lowers `c ? K : 0` as `setcc(!c); dec; and K`, which is retail's
//     three instructions; written the other way round it takes the general
//     two-constant form `setcc; dec; and -K; add K` (96.16 against 95.26).
// The two Get() calls really are two calls - retail saves the first in EBX
// for the m_lastWarned store and prices the gap with the second.
// E:\gamedcs\remote.cpp:2950
VA(0x00557af0, 0x208)  // anchor-global, dc 0x11f108
void CTurnDuration::checkForWarning()
{
    if (m_currDuration == 0)
        return;
    if (g_inCampaign)
        return;
    if (m_nextWarning == 0)
        return;
    if (m_turnStartTime == 0)
        return;
    if (m_lastWarned == 0)
        return;
    if (g_currentPlayer == 0)
        return;
    if (!g_currentPlayer->isLocalHuman())
        return;
    if (m_pauseTime > 0)
        return;

    unsigned long currTime = GameTime::get();
    unsigned long lastWarned = m_lastWarned;
    if (GameTime::get() - lastWarned < m_nextWarning)
        return;

    long timeLeft = m_turnStartTime - currTime + m_currDuration;
    if (timeLeft < 0) {
        m_nextWarning = 0;
        return;
    }

    if (timeLeft > 60000) {
        float minutes = timeLeft / 60000.0f;
        if (minutes >= 0.8 && minutes <= 1.2)
            g_chatMan.turnDurationMsg(g_generalText->getText(GENERAL_TEXT_TURN_ONE_MINUTE_REMAINING));
        else
            g_chatMan.turnDurationMsg(g_generalText->getText(GENERAL_TEXT_TURN_MINUTES_REMAINING_FORMAT), minutes);
    } else {
        // A 29-second remainder is announced as the 30-second mark. The
        // bound is spelled as a named local rather than an enumerator on
        // purpose: remote.h is included by eight TUs (cmbtmgr, ai_player,
        // advmgr among them) and a new type definition there moves their
        // front-end handle numbering - the documented include-set class.
        const int roundUpSeconds = 29;
        int seconds = timeLeft / 1000;
        if (seconds == roundUpSeconds)
            seconds = 30;
        if (seconds == 1)
            g_chatMan.turnDurationMsg(g_generalText->getText(GENERAL_TEXT_TURN_ONE_SECOND_REMAINING));
        else
            g_chatMan.turnDurationMsg(g_generalText->getText(GENERAL_TEXT_TURN_SECONDS_REMAINING_FORMAT), seconds);
    }

    m_lastWarned = currTime;
    if (timeLeft > m_currDuration / 2 && ((m_currDuration >> 1) << 1) > 120000)
        m_nextWarning = m_currDuration / 2;
    else if (timeLeft > 60000)
        m_nextWarning = m_turnStartTime - currTime + m_currDuration - 60000;
    else if (timeLeft > 10000)
        m_nextWarning = m_turnStartTime - currTime + m_currDuration - 10000;
    else if (timeLeft >= 2000)
        m_nextWarning = 1000;
    else
        m_nextWarning = 0;
}

VA(0x00557d00, 0x55)  // dc 0x11f2fc
unsigned char CTurnDuration::isClose(unsigned long howClose)
{
    if (!isOn())
        return 0;
    if (m_turnStartTime == 0)
        return 0;
    if (m_pauseTime != 0)
        return 0;
    unsigned char close = GameTime::get() + howClose
                          > m_turnStartTime + m_currDuration;
    return close;
}

VA(0x00557d60, 0xB)  // dc 0x11f39c
void CTurnDuration::clear()
{
    m_nextWarning = m_lastWarned = m_turnStartTime = 0;
}

VA(0x00557d70, 0x14)  // dc 0x11f3a8
void CTurnDuration::setDuration(unsigned long ms)
{
    m_turnStartTime = 0;
    m_currDuration = ms;
}

VA(0x00557d90, 0x3D)  // dc 0x11f3b0
void CTurnDuration::start()
{
    if (m_currDuration != 0 && !g_inCampaign) {
        m_lastWarned = m_turnStartTime = GameTime::get();
        m_nextWarning = 0;
        if (m_currDuration > 60000)
            m_nextWarning = m_currDuration / 4;
        else
            m_nextWarning = m_currDuration / 2;
    }
}

// Original: CTurnDuration::AddTime; remote.cpp:3070, dc 0x11f3ec
void CTurnDuration::addTime(unsigned long howMuch)
{
    m_turnStartTime += howMuch;
    m_lastWarned += howMuch;
}

VA(0x00557dd0, 0x14)  // dc 0x11f3fc
void CTurnDuration::pause()
{
    if (m_turnStartTime != 0)
        m_pauseTime = GameTime::get();
}

VA(0x00557df0, 0x31)  // dc 0x11f41c
void CTurnDuration::resume()
{
    unsigned long pausedAt = m_pauseTime;
    if (pausedAt != 0 && m_turnStartTime != 0) {
        unsigned long pausedFor = GameTime::get() - pausedAt;
        m_pauseTime = 0;
        addTime(pausedFor);
    }
}

VA(0x00557e30, 0x7A)  // dc 0x11f448
CNetMsgHandlerPause::CNetMsgHandlerPause()
{
    if (g_dPlay) {
        m_netMsgHandlerSave = g_dPlay->getNetMsgHandler();
        g_dPlay->setNetMsgHandler(this);
    }
}

VA_COMPGEN(0x00557eb0, 0x21, SCALAR_DELETING_DTOR, CNetMsgHandlerPause)

// CAutoArray<CDPlayPlayer>'s two out-of-line destructors. remote.obj emits
// BOTH the CDPlaySession and the CDPlayPlayer instantiation of each, and the
// two share one join key, so the claims name the INSTANTIATION the way
// `hero_vector` and `CImmEnclosure_auto_ptr` do.

// The element is settled by the vftable both bodies store, 0x640f24, and by
// the ONE slot that separates it from 0x6400d8:
//   0x6400d8  ??_G 0x512670 | Add 0x558410 | Get 0x499fc0 | Put 0x499fe0
//   0x640f24  ??_G 0x558490 | Add 0x558410 | Get 0x499fc0 | Put 0x499fe0
// - seven slots each, CAutoArray's virtual count exactly (against four for
// CNetMsgHandler's table at 0x640f14 just below), identical in six of them.
// Only the scalar deleting destructor differs, because only it stores the
// table's own address, and that single self-reference is what stopped
// /OPT:ICF folding either the bodies or the tables. 0x6400d8 is proven
// CDPlaySession's in multiplayerwindow.cpp, so 0x640f24 is the other
// instantiation this object carries; `CAutoArray<CDPlayPlayer> playerArray`
// at remote.cpp:2568 is the local whose teardown reaches 0x5583b0.

// The sibling 84-byte row at 0x558350 stores a different vftable again and
// is already claimed as ~CHeroSessions in multiplayerwindow.cpp.
VA_COMPGEN(0x005583b0, 0x54, IMPLICIT_DTOR, CDPlayPlayer_CAutoArray)
VA_COMPGEN(0x00558490, 0x6C, SCALAR_DELETING_DTOR, CDPlayPlayer_CAutoArray)

// deque<CNetMsg*>'s back-block allocator, the half of the pair army.obj's
// int deque needed the freeing side of. Byte-verified against the emitted
// COMDAT at 0.967 over 322 bytes.
VA_COMPGEN(0x00558500, 0x142, DEQUE_BUYBACK, CNetMsg)

VA(0x00557ee0, 0x91)  // dc 0x11f498
CNetMsgHandlerPause::~CNetMsgHandlerPause()
{
    if (g_dPlay)
        g_dPlay->setNetMsgHandler(m_netMsgHandlerSave);
}

VA(0x00557f80, 0x31)  // dc 0x11f4d0
CHourGlass::CHourGlass(unsigned char thread)
    : m_thread(thread)
{
    start();
}

VA(0x00557fc0, 0x1A)  // dc 0x11f4e8
CHourGlass::~CHourGlass()
{
    stop();
}

// E:\gamedcs\remote.cpp:3125..3134. Start and Stop have no standalone
// retail bodies - /Ob2 expands each into the one caller it has, the
// constructor and the destructor claimed further down - but their SHAPE is
// readable there: each arm ends in a mouse-thread call when the guard was
// built with a worker thread, and in a direct pointer store when it was
// not. Stop deliberately leaves m_thread armed, so an explicit Stop and the
// later destructor both stop the thread, exactly as retail does.

void CHourGlass::stop()
{
    if (m_thread)
        stopMouseThread();
    else
        g_mouseManager->setPointer(0, mouseManager::ADVENTURE_SET);
}

void CHourGlass::start()
{
    if (m_thread)
        startMouseThread();
    else
        g_mouseManager->setPointer(1, mouseManager::ADVENTURE_SET);
}

// Original: GetQueueSize; remote.cpp:3142, dc 0x11f550
unsigned char getQueueSize(int toWho, unsigned long& numMsgs,
                           unsigned long& queueSize)
{
    if (!g_dPlay)
        return 0;
    unsigned long dpidTo = 0;
    if (toWho != NET_MESSAGE_RECIPIENT_ALL)
        dpidTo = g_game->m_players[toWho].m_dpid;
    if (!g_dPlay->getSendQueueSize(g_thisNetPlayerInfo.m_dpid, dpidTo,
                                 &numMsgs, &queueSize))
        return 0;
    return 1;
}

// COMDAT pairing: deque<CNetMsg*>'s own destructor, 160 B against
// remote.obj's single 160-byte COMDAT, and the owner of the DEQUE_BUYBACK
// row claimed just below. Declarator form: _demangle_key keys a deque
// destructor `deque_deque@dtor`, which no compgen kind builds.
#if 0  // @carcass: Dinkumware instantiations emitted by this compiland

VA(0x00557fe0, 0xA0)  // COMDAT pairing (unique 160 B in this obj)
std::deque<CNetMsg*>::~deque()
{
    // @stub
}

#endif  // @carcass

// COMDAT pairing: `CAutoArray<T>::Add`, the ONE row the whole link carries
// for this member - both vftables reach it (0x6400d8 and 0x640f24 agree in
// slot 1), and remote.obj's CDPlaySession and CDPlayPlayer instantiations
// are byte-identical to each other AND relocation-identical, so /OPT:ICF
// folded them exactly as it folded ~TResourceHandle. It is claim-only: the
// definition lives in array.h, and `_demangle_key` keys the member
// `cautoarray_add`, which no compgen kind builds.
#if 0  // @carcass: claim-only - the definition lives in array.h

VA(0x00558410, 0x7C)  // COMDAT pairing (ICF-folded CAutoArray<T>::Add)
unsigned char CAutoArray<CDPlayPlayer>::add(CDPlayPlayer* element)
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\remote.cpp:3125
// COMDAT pairing: deque<CNetMsg*>::push_back and its map-growth helper
// _Growmap, agreements 1.000 and 1.000 at exactly equal extents. This object
// is the only one that instantiates the message queue.
VA_COMPGEN(0x00558080, 0x2CF, DEQUE_PUSH_BACK, CNetMsg_ptr)
VA_COMPGEN(0x00558650, 0x10, DEQUE_CONST_ITERATOR_CTOR, CNetMsg_ptr)
VA_COMPGEN(0x00558660, 0x6D, DEQUE_GROWMAP, CNetMsg_ptr)
VA_COMPGEN(0x005586d0, 0x24, DEQUE_CONST_ITERATOR_CTOR_NODE, CNetMsg_ptr)
