// remote.h - prototypes of remote.cpp (compiland remote.obj)
#ifndef HOMM3_REMOTE_H
#define HOMM3_REMOTE_H

#include "dxplay.h"
#include <deque>
#include "textntry.h"
#include "window.h"
#include "inputmgr.h"

class CNetMsg;
class CNetMsgHandler;
class textWidget;
class sample;
class ds_memsample;

// DC's nested char[21][8] type gives this class its complete 0xac-byte
// layout. Retail OnOK independently proves the same 21-byte stride and the
// eight-player bound while inlining the constructor and AddPlayer.
class CHotSeatMan {
public:
    enum {
        MAX_PLAYERS = 8,
        PLAYER_NAME_SIZE = 21
    };
    int m_playerCount;
    char m_names[MAX_PLAYERS][PLAYER_NAME_SIZE];
    CHotSeatMan() : m_playerCount(0) {}
    void clear() { m_playerCount = 0; }
    void addPlayer(const char* name)
    {
        if (m_playerCount < MAX_PLAYERS) {
            strcpy(m_names[m_playerCount], name);
            ++m_playerCount;
        }
    }
    // E:\gamedcs\remote.h:207. The DC build retains this tiny accessor call;
    // VC6 expands its bounds guard and 21-byte name stride in the hot-seat loop.
    char* getName(int player)
    {
        if (player >= m_playerCount)
            return 0;
        return m_names[player];
    }
};
SIZE(CHotSeatMan, 0xac);

DATA(0x0069ca50) extern CHotSeatMan* g_hotSeatMan;

// DC publishes this exact 351-byte record and the cdecl varargs Log
// signature.  Retail's global at 0x69d648 and its pushed-this call sites
// prove the PC identity independently.
class CLogFile {
public:
    // E:\gamedcs\remote.h:224
    CLogFile(char* logFileName)
    {
        strcpy(m_logFileName, logFileName);
    }
    // Original: CLogFile::InitLogFile; remote.h:235, dc 0xe709c.
    // Both release builds keep the logging hooks empty. Complete's calls
    // share the no-argument ret representative at 0x5bc690.
    void initLogFile() {}
    // Original: CLogFile::Log; remote.h:249, dc 0x70ac4.
    void log(char* format, ...) {}

protected:
    char m_logFileName[351];
};
SIZE(CLogFile, 351);

extern CLogFile g_logFile;

// DC proves the CDPlayLobby base and names m_pNetMsgHandler. Retail proves the
// pointer at +0xf0: its Dinkumware deque widens the derived state by eight
// bytes relative to DC's +0xe8 layout.
// Replaces SUnnamed69d808 at the same global 0x69d808: its pad_00
// covered this canonical base/state, and field_f0 was m_pNetMsgHandler.
class CDPlayHeroes : public CDPlayLobby {
public:
    CDPlayHeroes();
    virtual ~CDPlayHeroes();
    void destroyMsgQueue();
    virtual unsigned char sysMsgHost(DPMSG_GENERIC* message,
                                     unsigned long toId);
    virtual unsigned char sysMsgSessionLost(DPMSG_GENERIC* message,
                                            unsigned long toId);
    virtual unsigned char sysMsgDestroyPlayerOrGroup(
        DPMSG_DESTROYPLAYERORGROUP* message, unsigned long toId);
    virtual unsigned char sysMsgCreatePlayerOrGroup(
        DPMSG_CREATEPLAYERORGROUP* message, unsigned long toId);
    bool pollRemote();
    // Out of line at 0x553040 (`ret 8`), and reached from two directions in
    // remote.obj alone: the free GetRemoteData wrapper at 0x554400 passes
    // ecx through and a literal 0, and CNetMsgHandler::CheckHandleNet
    // (0x557860) calls it with the literal pair (1, 0). Declaration only,
    // for the same reason as the pair above.
    CNetMsg* getRemoteData(unsigned char removeFromQueue,
                           unsigned char* wasCompressed);
    bool transmitRemoteData(CNetMsg* msg, int toWho,
                            bool compressMsg, bool guaranteed);
    bool transmitRemoteDataDPID(CNetMsg* msg, unsigned long dpidTo,
                                bool compressMsg, bool guaranteed);
    // Retail 0x5533d0, DC remote.cpp:578. The free transmit wrappers stamp
    // their sender fields and optional compression, then call this retrying
    // DirectPlay sender with the final message buffer.
    bool sendIt(CNetMsg* msg, unsigned long dpidTo, bool guaranteed);
    void setNetMsgHandler(CNetMsgHandler* netMsgHandler);
    CNetMsgHandler* getNetMsgHandler();
    void handlePlayerDrop(unsigned long dpid);
    void handleHostXFer();
    void handleNewPlayer(unsigned long dpid, char* name, void* data, unsigned long size);

protected:
    void queueMsg(CNetMsg* netMsg);
    CNetMsg* compressMsg(CNetMsg* netMsg);
    CNetMsg* uncompressMsg(CNetMsg* netMsg);
    unsigned char handleLowLevelMsg(CNetMsg* netMsg);

public:
    friend int transmitRemoteDataDPID(CNetMsg*, unsigned long,
                                      bool, bool);
    friend int transmitRemoteData(CNetMsg*, int,
                                  bool, bool);
    CDPlayMsg m_dpMsg;  // +0x60
    std::deque<CNetMsg*> m_msgQueue;  // +0x68..+0x97
    char m_localIpAddress[80];  // +0x98..+0xe7
    unsigned long m_confirmId;  // +0xe8
    unsigned long m_currMessageId;  // +0xec

protected:
    CNetMsgHandler* m_netMsgHandler;  // +0xf0
};
SIZE(CDPlayHeroes, 0xf4);

extern CDPlayHeroes* g_dPlay;
extern unsigned char g_dPlayReady;
extern bool g_mPlayer;
extern bool g_mPlayerHost;
extern char g_tcpAddress[21];

extern "C" const GUID guidHeroes3;

extern unsigned char g_unk69774c;

// Retail's chat methods independently prove every offset used here; the
// Dreamcast CodeView field list supplies the source names and the 0x88-byte
// nested record extent.
class CChatManager {
public:
    CChatManager(int maxChatLines);
    ~CChatManager();
    class CChatStr {
    public:
        char m_text[128];
        unsigned long m_killTime;
        unsigned char m_isSystem;
    // Dreamcast ends the 0x88-byte chat record with isSystem at +0x84.
    // The final three bytes align the record; retail uses the same stride.
        char m_paddingAfterIsSystem[3];

        CChatStr()
        {
            m_text[0] = 0;
            m_killTime = 0;
            m_isSystem = 0;
        }
    };
    void init();
    void shutDown();
    // CodeView defines these as variadic members (remote.cpp:857..990).
    // On x86 cdecl passes this before format on the stack; retail's caller
    // cleanup and [ebp+8]/[ebp+0xc]/[ebp+0x10] layout preserve that member ABI.
    void __cdecl addChat(const char* format, ...);
    void __cdecl playerDropMsg(const char* format, ...);
    void __cdecl turnDurationMsg(const char* format, ...);
    void __cdecl systemMsg(const char* format, ...);
    void __cdecl playerEnterMsg(const char* format, ...);

protected:
    CChatStr* m_msgArray;  // +0x00
    int m_currMsg;  // +0x04
    int m_msgCount;  // +0x08
    char* m_widgetText;  // +0x0c
    unsigned long m_pauseTime;  // +0x10
    unsigned char m_changed;  // +0x14

public:
    // Dreamcast places changed at +0x14 and lastWidget at +0x18,
    // matching retail. These three bytes align the pointer.
    char m_paddingBeforeLastWidget[3];

protected:
    textWidget* m_lastWidget;  // +0x18
    int m_maxLines;  // +0x1c
    int m_position;  // +0x20
    unsigned char m_chatKilled;  // +0x24

public:
    // Retail retains chatKilled at +0x24 and adds the sample handle
    // at +0x28. Three bytes align that pointer; DC has no such handle.
    char m_paddingBeforeChatMemSample[3];
    // Retail PC adds the live Miles handle that AddChat/TurnDurationMsg
    // reuse. The DC record lacks it: isSysMsg moves from +0x25 to
    // +0x2c, and the five resource pointers move by eight bytes.
    ds_memsample* m_chatMemSample;  // +0x28

protected:
    unsigned char m_isSysMsg;  // +0x2c

public:
    // The PC isSysMsg byte moves to +0x2c after the new handle.
    // The sample pointer at +0x30 requires these three alignment bytes.
    char m_paddingBeforeChatSample[3];

    void updateWidget(textWidget* widget, unsigned char killOld, int numLines);
    void pauseTimeOuts();
    void resumeTimeOuts();
    void clearChat();
    // DC remote.h:320-321, dc 0x1474a0/0x1474ac. The lobby slider
    // expands these count and position reads at +0x08/+0x20 in retail.
    int getCount() { return m_msgCount; }
    int getPosition() { return m_position; }
    void setPosition(int newPos);
    void setMaxLines(int maxChatLines);
    bool chatChanged() { return m_changed || m_chatKilled; }
    unsigned char hasOldChat();
    unsigned char hasChat();

protected:
    sample* m_chatSample;  // +0x30
    sample* m_playerDropSample;  // +0x34
    sample* m_sysMsgSample;  // +0x38
    sample* m_turnDurSample;  // +0x3c
    sample* m_playerEnterSample;  // +0x40
    // remote.cpp:1060/1065, DC 0x11c71c/0x11c738; the publics prove
    // protected access. AddChat calls the first canonical helper, while
    // KillOldChat calls the second. Retail expands these source calls.
    int getNextFreeMsgNbr();
    int getNextMsgNbr(int msgNbr);
    void killOldChat();
    void updateNewChat();
    void updateWidgetText(int numLines, textWidget* widget);
};
SIZE(CChatManager::CChatStr, 0x88);
SIZE(CChatManager, 0x44);

extern CChatManager g_chatMan;

enum ENetMessageRecipient {
    NET_MESSAGE_RECIPIENT_ALL = 0x7f
};

// Retail vtable 0x640e30. Slots 0..18 are textEntryWidget's exact prefix;
// Dreamcast supplies the seven introduced method names at slots 19..24 and
// proves that this class adds no data (its 0x70-byte extent equals retail's
// textEntryWidget extent). The retail bodies independently confirm the base
// tail offsets: IsOpen reads cursorIndex at +0x58 and the edit actions use
// Text at +0x30.
class CChatEdit : public textEntryWidget {
public:
    CChatEdit(int x, int y, int w, int h, int textSize, char* text,
              char* fontName, font::TColor color,
              font::EJustify justification,
              char* backgroundIcon, int backgroundFrame, int id, int style,
              int readType, int insetX, int insetY);
    virtual ~CChatEdit();
    virtual int onKeyPress(message* msg);  // slot 15
    virtual unsigned char ignoreKey(message* msg);  // slot 16
    virtual void updateScreen();  // slot 19
    virtual int onEnter(message msg);  // slot 20
    virtual int onEscape(message msg);  // slot 21
    virtual int onFunctionKey(message msg, int toWho);  // slot 22
    virtual bool isOpen();  // slot 23
    virtual void sendChat(const char* text, int toWho) = 0;  // slot 24
};

// Dreamcast remote.h proves this intermediate class. Retail constructors for
// both surviving derived editors expand its forwarding ctor into a direct
// CChatEdit call followed by the +0x70 clear.
class CGameChatEdit : public CChatEdit {
public:
    CGameChatEdit(int x, int y, int w, int h, int textSize, char* text,
                  char* fontName, font::TColor color,
                  font::EJustify justification, char* backgroundIcon,
                  int backgroundFrame, int id, int style, int readType,
                  int insetX, int insetY);
    virtual int onKeyPress(message* msg);
    virtual int onEscape(message msg);
    virtual void sendChatCleanup();
    virtual void activate();
    unsigned char m_activated;
    char m_paddingAfterActivated[3];
};

// E:\gamedcs\remote.h:441
inline CGameChatEdit::CGameChatEdit(
    int x, int y, int w, int h, int textSize, char* text, char* fontName,
    font::TColor color, font::EJustify justification, char* backgroundIcon,
    int backgroundFrame, int id, int style, int readType, int insetX,
    int insetY)
    : CChatEdit(x, y, w, h, textSize, text, fontName, color, justification,
                backgroundIcon, backgroundFrame, id, style, readType,
                insetX, insetY)
{
    m_activated = 0;
}

// E:\gamedcs\remote.h:446
VA(0x004021f0, 0x42)  // dc 0x30c8
inline int CGameChatEdit::onKeyPress(message* msg)
{
    if (m_activated)
        return CChatEdit::onKeyPress(msg);

    if (getCharPressed(msg) == KEYCODE_TAB) {
        activate();
        return 1;
    }
    return 0;
}

// E:\gamedcs\remote.h:460
VA(0x00402240, 0x3C)  // dc 0x3110
inline int CGameChatEdit::onEscape(message msg)
{
    m_activated = 0;
    m_parentWindow->setFocus(-1);
    setFocus(0);
    return CChatEdit::onEscape(msg);
}

// E:\gamedcs\remote.h:471
VA(0x00402280, 0x23)  // dc 0x3178
inline void CGameChatEdit::sendChatCleanup()
{
    m_parentWindow->setFocus(-1);
    setFocus(0);
    m_activated = 0;
    draw();
}

// E:\gamedcs\remote.h:479
VA(0x004022b0, 0x2B)  // dc 0x31ac
inline void CGameChatEdit::activate()
{
    m_activated = 1;
    setFocus(1);
    m_parentWindow->setFocus(m_id);
    draw();
    updateScreen();
}

// Retail's complete method family proves five unsigned-long lanes at
// +0/+4/+8/+c/+10; Dreamcast CodeView supplies their source names.
class CTurnDuration {
public:
    CTurnDuration();
    void addTime(unsigned long howMuch);
    unsigned char isOn();
    unsigned char isExpired();
    unsigned char isClose(unsigned long howClose);
    void start();
    void clear();
    void setDuration(unsigned long ms);
    void checkForWarning();
    void pause();
    void resume();
    friend void __cdecl CChatManager::turnDurationMsg(const char* format, ...);

protected:
    unsigned long m_lastWarned;
    unsigned long m_turnStartTime;
    unsigned long m_currDuration;
    unsigned long m_nextWarning;
    unsigned long m_pauseTime;
};
SIZE(CTurnDuration, 0x14);

extern CTurnDuration g_turnDuration69d630;

// Retail .bss pair right behind gUnnamed69d808's pointer cell, written
// together by advManager::StartLocalPlayerTurn (the acting player's game
// position and an armed byte) and read back by CAdvMgrNetMsgHandler::
// HandleNetMsg. The band 0x552e00..0x556900 that owns their siblings is
// unclaimed, so the names stay ordinal and the DATA claims wait for it.
extern int g_unnamed69d810;
extern unsigned char g_weMoved;

// Retail's constructor/destructor pair stores and tests only this byte;
// Dreamcast supplies the class and member names.
class CHourGlass {
public:
    CHourGlass(unsigned char thread);
    ~CHourGlass();
    void start();
    void stop();

protected:
    unsigned char m_thread;
};
SIZE(CHourGlass, 1);

void destroyMsg(CNetMsg* netMsg);

// remote.h:537 in DC. This four-byte owner exists solely to release a
// dequeued message on every return arm of the owning dispatcher.
// Its ctor and dtor are header inline in the original and retail expands
// both into their command/remote callers.
class CMessageKill {
public:
    CMessageKill(CNetMsg* netMsg) : m_netMsg(netMsg) {}
    VA(0x00474680, 0xC)  // exact selected header COMDAT, dc 0x70ad0
    ~CMessageKill()
    {
        if (m_netMsg)
            destroyMsg(m_netMsg);
    }
    void setMessage(CNetMsg* netMsg) { m_netMsg = netMsg; }

protected:
    CNetMsg* m_netMsg;
};
SIZE(CMessageKill, 0x4);

// Retail inlines these accessors in the adventure-popup constructor and proves
// m_inPopup at +4. Dreamcast supplies the names, the abort pointer at +8, and
// the polymorphic class identity.
class CNetMsgHandler {
public:
    CNetMsgHandler();
    virtual ~CNetMsgHandler();  // slot 0
    virtual CNetMsg* checkHandleNet(unsigned char inPopup,
                                    unsigned char* msgReceived);  // slot 1
    unsigned char isInPopup() { return m_inPopup; }
    // E:\gamedcs\remote.h:629
    VA(0x00557900, 0x4)  // dc 0x201f8
    virtual CNetMsg* getAbortPopupMsg()
    {
        return m_abortPopupMsg;
    }

    VA(0x00555150, 0x1C)  // anchor-vtable (slot 2 call of 0x640f14), dc 0x11f7e0
    void copy(CNetMsgHandler* other)
    {
        m_inPopup = other->isInPopup();
        m_abortPopupMsg = other->getAbortPopupMsg();
    }
    void setAbortPopupMsg(CNetMsg* netMsg);
    void setInPopup(unsigned char b) { m_inPopup = b; }

protected:
    // A pure virtual may still have an out-of-line definition. Retail's
    // vtable keeps _purecall in slot 3, while two direct base-qualified
    // dispatcher calls land on that definition at 0x557920.

    // PROTECTED, not private, 2026-08-20: CAdvMgrNetMsgHandler::
    // HandleNetMsg's defer arms store the abort message DIRECTLY
    // (`mov [this+8], msg` inline at nine sites) where SetAbortPopupMsg
    // is an out-of-line body - the derived dispatcher touches the raw
    // members, so retail's access let it.
    unsigned char m_inPopup;  // +0x04
    virtual CNetMsg* handleNetMsg(CNetMsg* netMsg) = 0;  // slot 3
    char m_paddingBeforeAbortPopupMsg[3];
    CNetMsg* m_abortPopupMsg;  // +0x08
};
SIZE(CNetMsgHandler, 0x0c);

// CNetMsgHandlerPause - the scoped handler that parks whatever handler the
// network singleton is carrying, installs itself for the life of a modal
// dialog (remotedlg.h's three dialogs embed one) or a combat
// (combatManager::field_38 owns one), and puts the old one back. Sixteen
// bytes: CNetMsgHandler's twelve plus one pointer, and 0x557e30's
// `mov [esi+0xc], eax` is that pointer.

// Vtable 0x640f04 is four slots wide, CNetMsgHandler's own, so the class
// introduces nothing: slot 0 is the ??_G at 0x557eb0, slot 1 the
// CheckHandleNet override at 0x555170, slot 2 the INHERITED
// CNetMsgHandler::GetAbortPopupMsg at 0x557900 (already claimed), and slot 3
// the HandleNetMsg override at 0x555180. Both overrides are five bytes of
// `xor eax,eax` and a sized return - the pause semantics are to swallow
// everything - and both are header-origin COMDATs in retail too, which is
// why they sit at 0x555170/0x555180 beside CNetMsgHandler::Copy rather than
// in the 0x557exx run with the rest of the class.
class CNetMsgHandlerPause : public CNetMsgHandler {
public:
    CNetMsgHandler* m_netMsgHandlerSave;  // +0x0c
    CNetMsgHandlerPause();
    virtual ~CNetMsgHandlerPause();
    // at all. Retail retains their header COMDATs beside Copy, separately
    // from the class's ordinary remote.cpp definitions.
    // E:\gamedcs\remote.h:658
    VA(0x00555170, 0x5)  // dc 0x11f80c
    virtual CNetMsg* checkHandleNet(unsigned char inPopup,
                                                 unsigned char* msgReceived)
    {
        return 0;
    }
    // E:\gamedcs\remote.h:659
    VA(0x00555180, 0x5)  // dc 0x11f810
    virtual CNetMsg* handleNetMsg(CNetMsg* netMsg)
    {
        return 0;
    }
};
SIZE(CNetMsgHandlerPause, 0x10);

// Adventure-map network dispatch. Retail's gift handler reads the inherited
// m_inPopup byte through IsInPopup; the DC roster supplies the class and
// method names but no additional data members.
class CAdvMgrNetMsgHandler : public CNetMsgHandler {
protected:
    virtual CNetMsg* handleNetMsg(CNetMsg* netMsg);
    void handleGiftRequestMsg(CNetMsg* netMsg);
    virtual void handleGiftMsg(CNetMsg* netMsg);
    void handleTradeRequestMsg(CNetMsg* netMsg);
};
SIZE(CAdvMgrNetMsgHandler, 0x0c);

void handlePlayerDrop(unsigned long dpid);
void onPlayerDropUpdateMsg(unsigned long dpid);
void handlePlayerDead(int deadGuy, unsigned char showMsg);
void handlePlayerWon(CNetMsg* netMsg);
void handlePlayerLost(CNetMsg* netMsg);
void handleNormalWinMsg(CNetMsg* netMsg);

unsigned char getQueueSize(int toWho, unsigned long& numMsgs, unsigned long& queueSize);
void receiveChat(char* chat, int fromWho);
void handlePlayerDrop(unsigned long dpid);

int transmitRemoteData(CNetMsg* msg, int toWho,
                       bool compressMsg, bool guaranteed);
int transmitRemoteDataDPID(CNetMsg* msg, unsigned long dpidTo,
                           bool compressMsg, bool guaranteed);
CNetMsg* getRemoteData(unsigned char removeFromQueue,
                       unsigned char* wasCompressed);
unsigned long calcCrcLong(const unsigned char* buf, unsigned len);
// DC remote.cpp:1411, dc 0x11ce68; retail ReceiveSaveGame keeps this
// cleanup boundary out of line on both fatal in-game receive paths.
// Retail .data 0x699510, remote.cpp's DATA claim. The kb.obj
// adjudicator sets it beside gbGameOver on every terminal path, so kb
// is a second reader and needs the declaration here rather than a
// line-initial extern in the .cpp.
extern int g_defeatedAllPlayers;
void remoteCleanup();
void pollRemote();
void sendChat(const char* chat, int toWho);
unsigned char lobbyLaunchConnect();
// Dreamcast names this network-launch state directly; retail oldmain tests
// it only while handling the missing-CD startup result.
extern int g_tcpHostStatus;
unsigned char testIfLobbyLaunched();
// Dreamcast publishes the owning remote.obj buffer and Complete's tutorial
// setup copies its selected filename here before loading the map header.
extern char g_mapName[260];

// Retail .data 0x69954c. make_gift only uses it as the gate for sending
// a gift/request message to a non-local human; wider role unattested.
extern int g_networkActive69954c;
extern int g_unnamed6994e4;

// CODEVIEW(E:\gamedcs\remote.cpp:102, dc 0x11b8c4) void DPSD(int iDPErr, char* cFile, int iLine);
// CODEVIEW(E:\gamedcs\remote.cpp:1390, dc 0x11ce14) unsigned char InitRemote(eNetGameType iMPType, const char* sUserName);
// CODEVIEW(E:\gamedcs\remote.cpp:1411, dc 0x11ce68) void RemoteCleanup();
// CODEVIEW(E:\gamedcs\remote.cpp:1438, dc 0x11cf34) int TransmitRemoteDataDPID(CNetMsg* pMsg, unsigned long dpidTo, unsigned char compressMsg, unsigned char guaranteed);
// CODEVIEW(E:\gamedcs\remote.cpp:1446, dc 0x11cf64)
void destroyMsg(CNetMsg* netMsg);
void handlePlayerDrop(unsigned long dpid);
void onPlayerDropUpdateMsg(unsigned long dpid);
void handlePlayerDead(int deadGuy, unsigned char showMsg);
void handlePlayerWon(CNetMsg* netMsg);
void handlePlayerLost(CNetMsg* netMsg);
void handleNormalWinMsg(CNetMsg* netMsg);

void receiveChat(char* chat, int fromWho);

int transmitRemoteData(CNetMsg* msg, int toWho, bool compressMsg, bool guaranteed);
// CODEVIEW(E:\gamedcs\remote.cpp:1454, dc 0x11cf94) void PollRemote();
// CODEVIEW(E:\gamedcs\remote.cpp:1829, dc 0x11d6c8) void WaitForReadyToPlayMsg();
// CODEVIEW(E:\gamedcs\remote.cpp:1960, dc 0x11d9ac) unsigned char HandleMPlayerLaunch();
// CODEVIEW(E:\gamedcs\remote.cpp:2045, dc 0x11dbbc) unsigned char LobbyLaunchConnect();
// CODEVIEW(E:\gamedcs\remote.cpp:2150, dc 0x11dde8) int GetPlayerPos(unsigned long dpid);
// CODEVIEW(E:\gamedcs\remote.cpp:2161, dc 0x11de9c) int GetPriorPlayer(int gamePos);
// CODEVIEW(E:\gamedcs\remote.cpp:2174, dc 0x11dec8) unsigned char IsValidHuman(CAutoArray<CDPlayPlayer>* playerArray, unsigned long dpid);
// CODEVIEW(E:\gamedcs\remote.cpp:2289, dc 0x11e0f4) void HandleNewHost();
// CODEVIEW(E:\gamedcs\remote.cpp:2317, dc 0x11e1cc) void OnPlayerDropUpdateMsg(unsigned long dpid);
// CODEVIEW(E:\gamedcs\remote.cpp:3142, dc 0x11f550) unsigned char GetQueueSize(int toWho, unsigned long* numMsgs, unsigned long* queueSize);

// --- CAllReadyToPlayMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:199, dc 0x11f62c) void CAllReadyToPlayMsg::CAllReadyToPlayMsg();

// --- CAnimatedDlg ---
// CODEVIEW(E:\gamedcs\remote.cpp:1544, dc 0x11f87c) void* CAnimatedDlg::`scalar deleting destructor'(unsigned __flags);

// --- CAutoArray<CDPlayPlayer> ---
// CODEVIEW(E:\gamedcs\array.h:37, dc 0x11ff34) void CAutoArray<CDPlayPlayer>::CAutoArray<CDPlayPlayer>();
// CODEVIEW(E:\gamedcs\array.h:46, dc 0x11ff64) void CAutoArray<CDPlayPlayer>::~CAutoArray<CDPlayPlayer>();
// CODEVIEW(E:\gamedcs\array.h:73, dc 0x11ff9c) unsigned char CAutoArray<CDPlayPlayer>::Add(CDPlayPlayer* element);
// CODEVIEW(E:\gamedcs\array.h:95, dc 0x120004) CDPlayPlayer* CAutoArray<CDPlayPlayer>::Get(unsigned long elementNbr);
// CODEVIEW(E:\gamedcs\array.h:103, dc 0x120018) unsigned char CAutoArray<CDPlayPlayer>::Put(unsigned long elementNbr, CDPlayPlayer* element);
// CODEVIEW(E:\gamedcs\array.h:113, dc 0x120030) unsigned char CAutoArray<CDPlayPlayer>::Delete(unsigned long elementNbr);
// CODEVIEW(E:\gamedcs\array.h:127, dc 0x12006c) unsigned char CAutoArray<CDPlayPlayer>::Insert(unsigned long nextElementNbr, CDPlayPlayer* element);
// CODEVIEW(E:\gamedcs\array.h:144, dc 0x1200dc) unsigned long CAutoArray<CDPlayPlayer>::GetCount();
// CODEVIEW(..\stlport\stl_string.h:144, dc 0x1200e0) void* CAutoArray<CDPlayPlayer>::`scalar deleting destructor'(unsigned __flags);

// --- CChatEdit ---
// CODEVIEW(E:\gamedcs\remote.cpp:1294, dc 0x11f848) void* CChatEdit::`scalar deleting destructor'(unsigned __flags);

// --- CChatManager ---
// CODEVIEW(E:\gamedcs\remote.cpp:804, dc 0x11c298) void CChatManager::CChatManager(int maxChatLines);
// CODEVIEW(E:\gamedcs\remote.cpp:829, dc 0x11c314) void CChatManager::~CChatManager();
// CODEVIEW(E:\gamedcs\remote.cpp:904, dc 0x11c4ac) void CChatManager::TurnDurationMsg(const char* cChatMsg);
// CODEVIEW(E:\gamedcs\remote.cpp:1060, dc 0x11c71c) int CChatManager::GetNextFreeMsgNbr();
// CODEVIEW(E:\gamedcs\remote.cpp:1065, dc 0x11c738) int CChatManager::GetNextMsgNbr(int msgNbr);
// CODEVIEW(E:\gamedcs\remote.cpp:1115, dc 0x11c87c) void CChatManager::UpdateNewChat();
// CODEVIEW(E:\gamedcs\remote.cpp:1213, dc 0x11ca60) unsigned char CChatManager::HasChat();
// CODEVIEW(E:\gamedcs\remote.h:291, dc 0x11f7d0) void CChatManager::CChatStr::CChatStr();

// --- CChatMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:405, dc 0x11f64c) void CChatMsg::CChatMsg(const char* sMsg);
// CODEVIEW(E:\gamedcs\netmsg.h:411, dc 0x11f690) unsigned long CChatMsg::GetSize();

// --- CDPlayHeroes ---
// CODEVIEW(E:\gamedcs\remote.cpp:146, dc 0x11b96c) void CDPlayHeroes::CDPlayHeroes();
// CODEVIEW(E:\gamedcs\remote.cpp:160, dc 0x11ba80) void CDPlayHeroes::DestroyMsgQueue();
// CODEVIEW(E:\gamedcs\remote.cpp:204, dc 0x11bb40) unsigned char CDPlayHeroes::SysMsgDestroyPlayerOrGroup(DPMSG_DESTROYPLAYERORGROUP* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\remote.cpp:214, dc 0x11bb60) unsigned char CDPlayHeroes::SysMsgCreatePlayerOrGroup(DPMSG_CREATEPLAYERORGROUP* pSysMsg, unsigned long toID);
// CODEVIEW(E:\gamedcs\remote.cpp:350, dc 0x11bd5c) CNetMsg* CDPlayHeroes::GetRemoteData(unsigned char removeFromQueue, unsigned char* wasCompressed);
// CODEVIEW(E:\gamedcs\remote.cpp:407, dc 0x11be50) unsigned char CDPlayHeroes::TransmitRemoteData(CNetMsg* pMsg, int toWho, unsigned char compressMsg, unsigned char guaranteed);
// CODEVIEW(E:\gamedcs\remote.cpp:425, dc 0x11be94) CNetMsg* CDPlayHeroes::CompressMsg(CNetMsg* pNetMsg);
// CODEVIEW(E:\gamedcs\remote.cpp:463, dc 0x11bf40) CNetMsg* CDPlayHeroes::UncompressMsg(CNetMsg* pNetMsg);
// CODEVIEW(E:\gamedcs\remote.cpp:496, dc 0x11bfec) unsigned char CDPlayHeroes::TransmitRemoteDataDPID(CNetMsg* pMsg, unsigned long dpidTo, unsigned char compressMsg, unsigned char guaranteed);
// CODEVIEW(E:\gamedcs\remote.cpp:578, dc 0x11c04c) unsigned char CDPlayHeroes::SendIt(CNetMsg* pMsg, unsigned long dpidTo, unsigned char guaranteed);
// CODEVIEW(E:\gamedcs\remote.cpp:685, dc 0x11c1d8) void CDPlayHeroes::HandleHostXFer();
// CODEVIEW(E:\gamedcs\remote.cpp:690, dc 0x11c1f8) void CDPlayHeroes::HandlePlayerDrop(unsigned long dpid);
// CODEVIEW(E:\gamedcs\remote.cpp:697, dc 0x11c228) void CDPlayHeroes::HandleNewPlayer();
// CODEVIEW(E:\gamedcs\remote.cpp:702, dc 0x11c22c) void CDPlayHeroes::QueueMsg(CNetMsg* pNetMsg);
// CODEVIEW(E:\gamedcs\remote.cpp:152, dc 0x11f814) void* CDPlayHeroes::`scalar deleting destructor'(unsigned __flags);

// --- CDPlayPlayer ---
// CODEVIEW(E:\gamedcs\dxplay.h:211, dc 0x11f7ac) unsigned long CDPlayPlayer::GetId();

// --- CGameTransferDlg ---
// CODEVIEW(E:\gamedcs\remote.cpp:2818, dc 0x11fdbc) void* CGameTransferDlg::`scalar deleting destructor'(unsigned __flags);

// --- CGameTransferSmack ---
// CODEVIEW(E:\gamedcs\remote.cpp:2784, dc 0x11ede8) void CGameTransferSmack::DrawCurrentFrame();

// --- CHourGlass ---
// CODEVIEW(E:\gamedcs\remote.cpp:3125, dc 0x11f4f8) void CHourGlass::Stop();
// CODEVIEW(E:\gamedcs\remote.cpp:3134, dc 0x11f524) void CHourGlass::Start();

// --- CLevelPickWaitDlg ---
// CODEVIEW(E:\gamedcs\remote.cpp:2546, dc 0x11e848) int CLevelPickWaitDlg::OnPlayerDrop(CNetMsg* pNetMsg, message* msg);
// CODEVIEW(E:\gamedcs\remote.cpp:2567, dc 0x11e894) void CLevelPickWaitDlg::OnHeroLevelUpdate(CNetMsg* pNetMsg);
// CODEVIEW(E:\gamedcs\remote.cpp:2482, dc 0x11fd08) void* CLevelPickWaitDlg::`scalar deleting destructor'(unsigned __flags);

// --- CLogFile ---
// CODEVIEW(E:\gamedcs\remote.h:224, dc 0x11f7b4) void CLogFile::CLogFile(char* sLogFileName);

// --- CNetMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:179, dc 0x11f5f4) unsigned char CNetMsg::IsCompressed();

// --- CPingMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:804, dc 0x11f73c) void CPingMsg::CPingMsg(unsigned long pingTime, eRS_Messages id);

// --- CPingResponseMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:815, dc 0x11f764) void CPingResponseMsg::CPingResponseMsg(unsigned long pingTime, eRS_Messages id);

// --- CPlayerActiveMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:793, dc 0x11f71c) void CPlayerActiveMsg::CPlayerActiveMsg();

// --- CPlayerDropMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:423, dc 0x11f6a8) void CPlayerDropMsg::CPlayerDropMsg(unsigned long dpid);

// --- CPlayerDropUpdateMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:461, dc 0x11f6f4) void CPlayerDropUpdateMsg::CPlayerDropUpdateMsg(unsigned long dpidDropped);

// --- CReadyToPlayMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:191, dc 0x11f60c) void CReadyToPlayMsg::CReadyToPlayMsg();

// --- CSaveScreen ---
// CODEVIEW(E:\gamedcs\remote.cpp:2677, dc 0x11fd70) void* CSaveScreen::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\remote.cpp:2677, dc 0x11fda4) void CSaveScreen::~CSaveScreen();

// --- CSessionLostMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:852, dc 0x11f78c) void CSessionLostMsg::CSessionLostMsg();

// --- CSetAsHostMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:433, dc 0x11f6d4) void CSetAsHostMsg::CSetAsHostMsg();

// --- CTurnDuration ---
// CODEVIEW(E:\gamedcs\remote.cpp:2912, dc 0x11f060) void CTurnDuration::CTurnDuration();
// CODEVIEW(E:\gamedcs\remote.cpp:2950, dc 0x11f108) void CTurnDuration::CheckForWarning();
// CODEVIEW(E:\gamedcs\remote.cpp:3070, dc 0x11f3ec) void CTurnDuration::AddTime(unsigned long howMuch);

// --- CWaitForReadyPlayersDlg ---
// CODEVIEW(E:\gamedcs\remote.cpp:1708, dc 0x11f8b0) void CWaitForReadyPlayersDlg::CWaitForReadyPlayersDlg();
// CODEVIEW(E:\gamedcs\remote.cpp:1718, dc 0x11f928) void CWaitForReadyPlayersDlg::Wait();
// CODEVIEW(E:\gamedcs\remote.cpp:1798, dc 0x11fbf0) unsigned char CWaitForReadyPlayersDlg::AllPlayersReady();
// CODEVIEW(E:\gamedcs\remote.cpp:1813, dc 0x11fc3c) int CWaitForReadyPlayersDlg::OnPlayerDrop(CNetMsg* pNetMsg, message* msg);
// CODEVIEW(E:\gamedcs\remote.cpp:1820, dc 0x11fcac) void* CWaitForReadyPlayersDlg::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\remote.cpp:1820, dc 0x11fce0) void CWaitForReadyPlayersDlg::~CWaitForReadyPlayersDlg();

// --- CWaitForRemoteBattleDlg ---
// CODEVIEW(E:\gamedcs\remote.cpp:2660, dc 0x11eaf8) int CWaitForRemoteBattleDlg::OnPlayerDrop(CNetMsg* pNetMsg, message* msg);
// CODEVIEW(E:\gamedcs\remote.cpp:2595, dc 0x11fd3c) void* CWaitForRemoteBattleDlg::`scalar deleting destructor'(unsigned __flags);

// --- std ---
// CODEVIEW(..\stlport\stl_deque.h:568, dc 0x11fdf0) std::_Deque_iterator<CNetMsg std::deque<CNetMsg *,std::allocator<CNetMsg *>,0>::begin(__$ReturnUdt);
// CODEVIEW(..\stlport\stl_deque.h:612, dc 0x11fe10) unsigned char std::deque<CNetMsg *,std::allocator<CNetMsg *>,0>::empty();
// CODEVIEW(..\stlport\stl_deque.h:615, dc 0x11fe28) void std::deque<CNetMsg *,std::allocator<CNetMsg *>,0>::deque<CNetMsg *,std::allocator<CNetMsg *>,0>(const std::allocator<CNetMsg* __a);
// CODEVIEW(..\stlport\stl_deque.h:680, dc 0x11fe44) void std::deque<CNetMsg *,std::allocator<CNetMsg *>,0>::~deque<CNetMsg *,std::allocator<CNetMsg *>,0>();
// CODEVIEW(..\stlport\stl_deque.h:765, dc 0x11fe98) void std::deque<CNetMsg *,std::allocator<CNetMsg *>,0>::push_back(CNetMsg** __t);
// CODEVIEW(..\stlport\stl_deque.h:816, dc 0x11fed4) void std::deque<CNetMsg *,std::allocator<CNetMsg *>,0>::pop_front();
// CODEVIEW(..\stlport\stl_deque.h:272, dc 0x11ff0c) void std::_Deque_iterator<CNetMsg *,std::_Nonconst_traits<CNetMsg *>,std::_Buf_size_traits<CNetMsg *,0> >::_Deque_iterator<CNetMsg *,std::_Nonconst_traits<CNetMsg *>,std::_Buf_size_traits<CNetMsg *,0> >();
// CODEVIEW(..\stlport\stl_deque.h:276, dc 0x11ff28) CNetMsg** std::_Deque_iterator<CNetMsg *,std::_Nonconst_traits<CNetMsg *>,std::_Buf_size_traits<CNetMsg *,0> >::operator*();
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0x11ff2c) void std::allocator<CNetMsg *>::allocator<CNetMsg *>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0x11ff30) void std::allocator<CNetMsg *>::~allocator<CNetMsg *>();
// CODEVIEW(..\stlport\stl_deque.h:452, dc 0x120114) void std::_Deque_base<CNetMsg *,std::allocator<CNetMsg *>,0>::_Deque_base<CNetMsg *,std::allocator<CNetMsg *>,0>(const std::allocator<CNetMsg* __a, unsigned __num_elements);
// CODEVIEW(..\stlport\stl_deque.h:274, dc 0x12017c) void std::_Deque_iterator<CNetMsg *,std::_Nonconst_traits<CNetMsg *>,std::_Buf_size_traits<CNetMsg *,0> >::_Deque_iterator<CNetMsg *,std::_Nonconst_traits<CNetMsg *>,std::_Buf_size_traits<CNetMsg *,0> >(const std::_Deque_iterator<CNetMsg* __x);
// CODEVIEW(..\stlport\stl_deque.h:190, dc 0x120190) void std::_Deque_iterator_base<CNetMsg *,std::_Buf_size_traits<CNetMsg *,0> >::_Deque_iterator_base<CNetMsg *,std::_Buf_size_traits<CNetMsg *,0> >();
// CODEVIEW(..\stlport\stl_string.h:190, dc 0x1201a0) void std::_STL_alloc_proxy<CNetMsg * * *,CNetMsg * *,std::allocator<CNetMsg *> >::~_STL_alloc_proxy<CNetMsg * * *,CNetMsg * *,std::allocator<CNetMsg *> >();
// CODEVIEW(..\stlport\stl_string.h:190, dc 0x1201b8) void std::_STL_alloc_proxy<unsigned int,CNetMsg *,std::allocator<CNetMsg *> >::~_STL_alloc_proxy<unsigned int,CNetMsg *,std::allocator<CNetMsg *> >();
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0x1201d0) void std::_STL_alloc_proxy<CNetMsg * * *,CNetMsg * *,std::allocator<CNetMsg *> >::_STL_alloc_proxy<CNetMsg * * *,CNetMsg * *,std::allocator<CNetMsg *> >(const std::allocator<CNetMsg* __a, CNetMsg**** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0x1201dc) void std::_STL_alloc_proxy<unsigned int,CNetMsg *,std::allocator<CNetMsg *> >::_STL_alloc_proxy<unsigned int,CNetMsg *,std::allocator<CNetMsg *> >(const std::allocator<CNetMsg* __a, const unsigned* __p);
// CODEVIEW(..\stlport\stl_deque.c:104, dc 0x1201e8) void std::_Deque_base<CNetMsg *,std::allocator<CNetMsg *>,0>::~_Deque_base<CNetMsg *,std::allocator<CNetMsg *>,0>();
// CODEVIEW(..\stlport\stl_deque.c:118, dc 0x120238) void std::_Deque_base<CNetMsg *,std::allocator<CNetMsg *>,0>::_M_initialize_map(unsigned __num_elements);
// CODEVIEW(..\stlport\stl_deque.c:364, dc 0x1202fc) void std::deque<CNetMsg *,std::allocator<CNetMsg *>,0>::_M_push_back_aux(CNetMsg** __t);
// CODEVIEW(..\stlport\stl_deque.c:444, dc 0x120350) void std::deque<CNetMsg *,std::allocator<CNetMsg *>,0>::_M_pop_front_aux();
// CODEVIEW(..\stlport\stl_deque.h:328, dc 0x120390) unsigned char std::operator==(const std::_Deque_iterator_base<CNetMsg* __x, const std::_Deque_iterator_base<CNetMsg* __y);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0x12039c) void std::destroy(std::_Deque_iterator<CNetMsg __first, std::_Deque_iterator<CNetMsg __last);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0x1203fc) void std::construct(CNetMsg** __p, CNetMsg** __value);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0x120424) void std::destroy(CNetMsg** __pointer);
// CODEVIEW(..\stlport\stl_deque.h:1145, dc 0x120440) void std::deque<CNetMsg *,std::allocator<CNetMsg *>,0>::_M_reserve_map_at_back(unsigned __nodes_to_add);
// CODEVIEW(..\stlport\stl_deque.h:233, dc 0x120470) void std::_Deque_iterator_base<CNetMsg *,std::_Buf_size_traits<CNetMsg *,0> >::_M_set_node(CNetMsg*** __new_node);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0x120480) CNetMsg*** std::_STL_alloc_proxy<CNetMsg * * *,CNetMsg * *,std::allocator<CNetMsg *> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0x1204a8) void std::_STL_alloc_proxy<CNetMsg * * *,CNetMsg * *,std::allocator<CNetMsg *> >::deallocate(CNetMsg*** __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0x1204d4) CNetMsg** std::_STL_alloc_proxy<unsigned int,CNetMsg *,std::allocator<CNetMsg *> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0x1204fc) void std::_STL_alloc_proxy<unsigned int,CNetMsg *,std::allocator<CNetMsg *> >::deallocate(CNetMsg** __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0x120528) CNetMsg** std::allocator<CNetMsg *>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0x12054c) void std::allocator<CNetMsg *>::deallocate(CNetMsg** __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0x120568) CNetMsg*** std::allocator<CNetMsg * *>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0x12058c) void std::allocator<CNetMsg * *>::deallocate(CNetMsg*** __p, unsigned __n);
// CODEVIEW(..\stlport\stl_deque.c:144, dc 0x1205a8) void std::_Deque_base<CNetMsg *,std::allocator<CNetMsg *>,0>::_M_create_nodes(CNetMsg*** __nstart, CNetMsg*** __nfinish);
// CODEVIEW(..\stlport\stl_deque.c:157, dc 0x1205e4) void std::_Deque_base<CNetMsg *,std::allocator<CNetMsg *>,0>::_M_destroy_nodes(CNetMsg*** __nstart, CNetMsg*** __nfinish);
// CODEVIEW(..\stlport\stl_deque.c:731, dc 0x120620) void std::deque<CNetMsg *,std::allocator<CNetMsg *>,0>::_M_reallocate_map(unsigned __nodes_to_add, unsigned char __add_at_front);
// CODEVIEW(..\stlport\stl_deque.h:413, dc 0x120730) CNetMsg** std::value_type(const std::_Deque_iterator<CNetMsg* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0x120734) void std::__destroy(std::_Deque_iterator<CNetMsg __first, std::_Deque_iterator<CNetMsg __last, CNetMsg** __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0x120788) void std::__destroy_aux();
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0x12078c) std::allocator<CNetMsg* std::__stl_alloc_rebind(std::allocator<CNetMsg* __a, CNetMsg*** __formal);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0x120790) std::allocator<CNetMsg* std::__stl_alloc_rebind(std::allocator<CNetMsg* __a, CNetMsg** __formal);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0x120794) CNetMsg*** std::copy(CNetMsg*** __first, CNetMsg*** __last, CNetMsg*** __result);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0x1207e4) CNetMsg*** std::copy_backward(CNetMsg*** __first, CNetMsg*** __last, CNetMsg*** __result);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0x120834) void std::__destroy_aux(std::_Deque_iterator<CNetMsg __first, std::_Deque_iterator<CNetMsg __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_deque.h:285, dc 0x120888) std::_Deque_iterator<CNetMsg* std::_Deque_iterator<CNetMsg *,std::_Nonconst_traits<CNetMsg *>,std::_Buf_size_traits<CNetMsg *,0> >::operator++();
// CODEVIEW(..\stlport\stl_deque.h:199, dc 0x1208a4) void std::_Deque_iterator_base<CNetMsg *,std::_Buf_size_traits<CNetMsg *,0> >::_M_increment();
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0x1208d4) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, CNetMsg*** __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0x1208e0) int* std::distance_type(CNetMsg*** __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0x1208e4) CNetMsg*** std::__copy(CNetMsg*** __first, CNetMsg*** __last, CNetMsg*** __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0x120904) CNetMsg*** std::__copy_backward(CNetMsg*** __first, CNetMsg*** __last, CNetMsg*** __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_deque.h:345, dc 0x120928) unsigned char std::operator!=(const std::_Deque_iterator_base<CNetMsg* __x, const std::_Deque_iterator_base<CNetMsg* __y);

#endif  /* HOMM3_REMOTE_H */
