#ifndef HOMM3_MULTIPLAYERWINDOW_H
#define HOMM3_MULTIPLAYERWINDOW_H

#include "va.h"

#include <string.h>
#include "platform.h"

#include "dxplay.h"
#include "hotseat.h"
#include "netgame.h"
#include "textntry.h"
#include "textwdgt.h"
#include "window.h"

struct _DPCOMPORTADDRESS;

// multiplayerwindow.cpp owns this 21-byte preference-backed player name.
// DoNewGame and the campaign launch copy it into a player cName slot.
extern int g_mpExtendedType;
extern int g_waitForRemoteReceive;

// Cast-free storage for the Winsock bind call in GetIPAddress. Both views are
// the same 16-byte IPv4 socket-address record; keeping the union in the domain
// header avoids a TU-local layout view.
union TIPv4SocketAddress {
public:
    sockaddr_in m_internet;
    sockaddr m_generic;
};
SIZE(TIPv4SocketAddress, 0x10);

// The private edit hierarchy is defined in multiplayerwindow.cpp.
class CHotSeatEdit;

// DC derives CHotSeatDlg from CHeroWindowEx and places its `edit` run at
// +0x4c, followed by m_rollover at +0x6c. Retail's independently proven
// CHeroWindowEx is four bytes wider, and the retail getter at 0x512530 reads
// m_rollover at +0x70, proving the corresponding repack. DC identifies the
// edit run as textWidget*[8]; retail OnOK corroborates its +0x50 start, four-
// byte stride, and textWidget::Text access.
class CHotSeatDlg : public CHeroWindowEx {
public:
    enum {
        BACKGROUND_ID = 500,
        FIRST_EDIT_ID = 509,
        HEADER_ID = 517,
        ROLLOVER_ID = 518,
        OKAY_ID = 519,
        BACK_ID = 520
    };
    CHotSeatEdit* m_edit[8];  // +0x50
    textWidget* m_rollover;  // +0x70
    THelpText m_hotSeatHelp[20];
    CHotSeatDlg();
    virtual ~CHotSeatDlg();
    virtual int onWidgetDeselect(int id, bool& exitFlag);
    void onKillFocus(int id);
    // Non-virtual, and the vtable proves it: 0x6401d8 stops after slot 13
    // (0x240210, CHotSeatEdit's table, starts at +0x38). Retail emits no
    // body for these helpers - they expand into CHotSeatEdit's two overrides.
    // DC separates UpdateOK (dc 0x102d4c), which only updates widget 519,
    // from OnKillFocus (dc 0x102cc8), which then redraws the dialog.
    int getPlayerCount();
    void updateOK();
    unsigned char onOK();
    virtual textWidget* getRolloverWidget();
};
SIZE(CHotSeatDlg, 0x114);

class CSprite;

class CHeroSessions : public CAutoArray<CDPlaySession> {
public:
    CHeroSessions();
    enum eSessionStatus {
        closed,
        open,
        password
    };
    bool getSessionInfo(unsigned long index, char* sessName, char* userName,
                        int& numPlayers, eSessionStatus& status);
};
SIZE(CHeroSessions, 0x14);

// TMultiPlayerWindow - CHeroWindowEx multiplayer session browser / host UI.
// DC field list 0x472e (base CHeroWindowEx @0, DC size 252); retail's four-
// byte-wider base shifts every trailing member +4 (all are pointers, a char
// array, or small scalars, so the shift is uniform). Retail proofs:
// GetRolloverWidget reads RolloverWidget@0xfc; InitRemote reads playerName
// @0xbc and sessTimer@0x68; ~dtor reads GameState@0x50 and pSessions@0x60;
// GoSessionList/GoMainMenu toggle the button run @0xc4..0xf8. Total 0x100.
// The 14-slot vtable 0x6400a0 overrides slot 0 (sdd/dtor), 9 (WindowHandler),
// 12 (OnWidgetDeselect), 13 (GetRolloverWidget).

// The DC types of the widget members are bitmapBorder* (splash), button*
// (the ten screen buttons), slider* (gameSlider) and textWidget* (the three
// headers); they are modelled as their widget/textWidget base here because
// every reconstructed body reaches them only through widget::send_message /
// textWidget::Text, and narrowing the include set avoids the declarator wall.
class TMultiPlayerWindow : public CHeroWindowEx {
public:
    enum EWidgetId {
        ONLINE_ID = 101,
        HOT_SEAT_ID = 102,
        IPX_ID = 103,
        TCP_ID = 104,
        MODEM_ID = 105,
        DIRECT_ID = 106,
        HOST_ID = 107,
        JOIN_ID = 108,
        SEARCH_ID = 109,
        FIRST_SESSION_ID = 110,
        LAST_SESSION_ID = 121,
        GAME_SLIDER_ID = 122,
        ROLLOVER_ID = 123,
        CANCEL_ID = 124,
        PLAYER_NAME_ID = 125,
        IP_ADDRESS_ID = 126
    };
    CSprite* m_gameState;  // +0x50
    unsigned char m_inSessionList;  // +0x54
    unsigned char m_showSplash;  // +0x55
    int m_currentGame;  // +0x58
    int m_currentIndex;  // +0x5c
    CHeroSessions* m_sessions;  // +0x60
    unsigned long m_sessTimer;  // +0x64
    unsigned long m_sessionRefreshTimeout;  // +0x68
    char m_localIpAddress[80];  // +0x6c
    textWidget* m_playerName;  // +0xbc (DC textEntryWidget*)
    unsigned char m_hostJoinScreen;  // +0xc0
    widget* m_splash;  // +0xc4 (DC bitmapBorder*)

    TMultiPlayerWindow();
    virtual ~TMultiPlayerWindow();
    unsigned char initRemote(eNetGameType netGameType, const char* extra,
                             _DPCOMPORTADDRESS* comportInfo);
    unsigned char joinSession(CDPlaySession* session, const char* password);
    unsigned char hostSession(const char* sessName, const char* password);
    unsigned char onJoin();
    unsigned char onHost();
    unsigned char onIPX();
    unsigned char onTCP();
    unsigned char onSearch();
    unsigned char onHotSeat();
    unsigned char onModem();
    unsigned char onDirect();
    unsigned char onModemJoin();
    unsigned char onModemHost();
    unsigned char onDirectHost();
    unsigned char onDirectJoin();
    void update();
    void goSessionList();
    void goMainMenu();
    void refreshSessions();
    void checkSessions();
    unsigned char isNT();
    virtual int windowHandler(message& msg);
    virtual int onWidgetDeselect(int id, bool& exitFlag);
    // E:\gamedcs\MultiPlayerWindow.h:91, dc 0x101da0
    VA(0x0050ed50, 0x7) MAC_ADDRESS(0x21cfdc, 0x8)  // dc 0x101da0
    virtual textWidget* getRolloverWidget()
    {
        return m_rolloverWidget;
    }

private:
    widget* m_hotSeat;  // +0xc8 (DC button*)
    widget* m_ipx;  // +0xcc
    widget* m_tcp;  // +0xd0
    widget* m_modem;  // +0xd4
    widget* m_direct;  // +0xd8
    widget* m_online;  // +0xdc
    widget* m_host;  // +0xe0
    widget* m_join;  // +0xe4
    widget* m_search;  // +0xe8
    widget* m_cancel;  // +0xec
    widget* m_gameSlider;  // +0xf0 (DC slider*)
    textWidget* m_sessNameHeader;  // +0xf4
    textWidget* m_userNameHeader;  // +0xf8
    textWidget* m_rolloverWidget;  // +0xfc
};
SIZE(TMultiPlayerWindow, 0x100);

// The singleton the ctor latches to `this` (0x50e050+0x66) and the dtor
// nulls (0x50ee40+0x51). No DC public names it - provisional house name.
extern TMultiPlayerWindow* g_multiPlayerWindow;

#endif  /* HOMM3_MULTIPLAYERWINDOW_H */
