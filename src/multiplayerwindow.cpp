#include "prefs.h"
#include "va.h"

#include <direct.h>

#include "multiplayerwindow.h"

#include "border.h"
#include "button.h"
#include "csprite.h"
#include "dplaycaps.h"
#include "gametypewindow.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "multiplayerwindow_globals.h"
#include "netgame.h"
#include "netplayer.h"
#include "remote.h"
#include "resourcemanager.h"
#include "slider.h"
#include "soundmgr.h"
#include "textresource.h"
#include "winfile.h"
#include "winmgr.h"

// Retail scalar state; startup initial values come from the pinned image.
DATA(0x0069ca28) TMultiPlayerWindow* g_multiPlayerWindow;

unsigned char initRemote(eNetGameType netGameType, const char* userName);
unsigned char initConnection(char* address, _DPCOMPORTADDRESS* comportInfo);
void remoteCleanup();

// Original: AddHelp; multiplayerwindow.cpp:94, dc 0xffaac.
void addHelp(THelpText* helpText, const char* rollover, const char* rightClick)
{
    helpText->m_text = rollover;
    helpText->m_rclick = rightClick ? rightClick : rollover;
}

// CMultiPlayerWindowEdit - the text-entry widget the session-host name field
// uses. Derives textEntryWidget, forwarding all sixteen constructor arguments;
// its only addition is the slot-15 key-handler override that gives it a
// distinct vtable (retail 0x640054, stored by the TMultiPlayerWindow
// constructor).
class CMultiPlayerWindowEdit : public textEntryWidget {
public:
    // E:\gamedcs\multiplayerwindow.cpp:141, dc 0x101e00
    CMultiPlayerWindowEdit(int x, int y, int w, int h, int textSize,
                           const char* text, const char* fontName,
                           font::TColor color, unsigned justification,
                           const char* backgroundIcon, int backgroundFrame,
                           int id, int style, int readType, int insetX,
                           int insetY)
        : textEntryWidget(x, y, w, h, textSize, text, fontName, color,
                          justification, backgroundIcon, backgroundFrame, id,
                          style, readType, insetX, insetY)
    {
    }
    virtual int onKeyPress(message* msg);  // slot 15, retail 0x50ed60
};

// E:\gamedcs\multiplayerwindow.cpp:174, dc 0x101f34
inline bool CHeroSessions::getSessionInfo(unsigned long index, char* sessName,
                                          char* userName, int& numPlayers,
                                          eSessionStatus& status)
{
    CDPlaySession* session = get(index);
    if (!session)
        return false;

    char separator[2];
    separator[0] = static_cast<char>(0xfa);
    separator[1] = 0;
    char* split = strstr(session->m_sessionName, separator);
    int nameLength = strlen(session->m_sessionName);
    if (split)
        nameLength = split - session->m_sessionName;
    strncpy(sessName, session->m_sessionName, nameLength);
    sessName[nameLength] = 0;
    if (split)
        strcpy(userName, &session->m_sessionName[nameLength + 1]);
    else
        userName[0] = 0;

    numPlayers = session->m_playerCount;
    status = open;
    if (session->isJoinDisabled())
        status = closed;
    else if (session->isPasswordProtected())
        status = password;
    return true;
}

// --- Retail-located TMultiPlayerWindow / CMPInputDlg / CHotSeatDlg cores ---
// Located by class vtable slots (0x6400a0 TMultiPlayerWindow, 0x6400f4
// CMPInputDlg, 0x6401d8 CHotSeatDlg) read from the retail image, plus the
// scalar-deleting-dtor->~dtor call edges. Retail lays these out in an order
// that does NOT follow the Dreamcast emission order (the reference block above
// keeps DC order), so they claim their retail RVAs in a dedicated ascending
// block. Bodies left @stub - the classes are not yet modelled in the header.
// File-scope storage the constructor reaches. gMultiPlayerHelp is the
// rollover/right-click help table indexed by (widget id - 101); the two char
// buffers hold the local player name shown in the entry field and the name of
// the most recently loaded game (checked for the remote-temp prefix).
DATA(0x006a6578) THelpText g_multiPlayerHelp[30];

// Armed by all three retail host paths before they create a DirectPlay
// session. DC DoNewGame/DoLoadGame identifies iMPExtendedType.
// Original DC name: iMPExtendedType; DoNewGame / DoLoadGame.
DATA(0x0069927c) int g_mpExtendedType;

// Armed beside the two known multiplayer start flags by Complete's generic
// join path. Its only other retail writes are in the adjacent host flows.
// Original DC name: gbWaitForRemoteReceive; DoNewGame / DoLoadGame.
DATA(0x00699288) int g_waitForRemoteReceive;

const long g_dplayErrorUserCancel = 0x88770118;

// The sole retail read at 0x50fade promotes the DirectPlay session from the
// mandatory migrate-host flag to migrate-host|keep-alive. No public symbol
// survives for the byte; its descriptive name follows that flag operation.
DATA(0x00681628) static unsigned char g_sessionKeepAlive = 1;

// The rollover/right-click help pointers the CMPInputDlg and CHotSeatDlg
// constructors hand to widget::set_help_text. The OK/Back pair (0x6a7760/
// 0x6a7768) is shared by both dialogs; the CHotSeatDlg edit ring uses its own
// pair (0x6a7758/0x6a775c). No DC name; provisional house names.
DATA(0x006a7758) char* g_hotSeatEditRollover;
DATA(0x006a775c) char* g_hotSeatEditRightClick;
DATA(0x006a7760) char* g_dialogOkHelp;
DATA(0x006a7768) char* g_dialogBackHelp;

// The generic network host dialog supplies separate help strings for its two
// edit controls and a label for the session-name field. Retail proves their
// cells and uses; descriptive names follow those controls.
DATA(0x006a7770) char* g_sessionNameHelp;
DATA(0x006a7778) char* g_sessionPasswordHelp;
// The TCP search dialog's address field uses a distinct rollover string. The
// second field reuses g_sessionPasswordHelp, while the generic OK/Back buttons keep
// the shared pair above.
DATA(0x006a7780) char* g_sessionNameLabel;
DATA(0x006a7788) char* g_searchAddressHelp;

// CMPEdit owns the focus-ring links and navigation slots shared by the
// multiplayer and hot-seat edits. Retail tables 0x640184/0x640130/0x640210
// share SetFocus (0x510890), OnNextEdit (0x510850), and OnPrevEdit (0x510870).
// DC CHotSeatEdit::OnKeyPress calls CMPEdit::OnKeyPress directly at line 861.
// Different constructor expansion in CMPInputDlg and CHotSeatDlg does not
// establish different base classes; keep the common base and its real calls.
class CMPEdit : public textEntryWidget {
public:
    CMPEdit* m_nextEdit;   // +0x70
    CMPEdit* m_prevEdit;   // +0x74

    CMPEdit(int x, int y, int w, int h, int textSize, const char* text,
            const char* fontName, font::TColor color, unsigned justification,
            const char* backgroundIcon, int backgroundFrame, int id,
            int style, int readType, int insetX, int insetY);
    // DC 0x1020b8 stores the argument at this+0x70.
    // E:\gamedcs\multiplayerwindow.cpp:269, dc 0x1020b4
    void setNextEdit(CMPEdit* nextEdit) { m_nextEdit = nextEdit; }
    // DC 0x1020c0 stores the argument at this+0x74.
    // E:\gamedcs\multiplayerwindow.cpp:274, dc 0x1020bc
    void setPrevEdit(CMPEdit* prevEdit) { m_prevEdit = prevEdit; }
    virtual void setFocus(bool state);
    virtual int onKeyPress(message* msg);
    virtual void onNextEdit();
    virtual void onPrevEdit();                   // slot 20, retail 0x510870
};

class CMPInputEdit : public CMPEdit {
public:
    // E:\gamedcs\multiplayerwindow.cpp:383, dc 0x102210
    VA(0x00511cd0, 0x62)  // exact body + selected-COMDAT ownership, dc 0x102210
    CMPInputEdit(int x, int y, int w, int h, int textSize, const char* text,
                 const char* fontName, font::TColor color,
                 unsigned justification, const char* backgroundIcon,
                 int backgroundFrame, int id, int style, int readType,
                 int insetX, int insetY)
        : CMPEdit(x, y, w, h, textSize, text, fontName, color, justification,
                  backgroundIcon, backgroundFrame, id, style, readType, insetX,
                  insetY)
    {
    }
    virtual int onKeyPress(message* msg);         // slot 15, retail 0x50de50
};

// This dialog-specific edit stays with the private CMPEdit hierarchy.
// Its retained CodeView procedures are OnKillFocus and OnKeyPress below;
// constructor source-line evidence is unavailable. DC CHotSeatDlg instead
// constructs textWidget labels at dc 0x1029d8/0x102a10 (line 648). Complete
// constructs 0x78-byte edit controls at 0x511fbb, installs vtable 0x640210
// at 0x51201f and initializes their focus links at +0x70/+0x74. The added
// constructor forwards the canonical CMPEdit interface in this local class.
class CHotSeatEdit : public CMPEdit {
public:
    CHotSeatEdit(int x, int y, int w, int h, int textSize, const char* text,
                 const char* fontName, font::TColor color,
                 unsigned justification, const char* backgroundIcon,
                 int backgroundFrame, int id, int style, int readType,
                 int insetX, int insetY)
        : CMPEdit(x, y, w, h, textSize, text, fontName, color,
                  justification, backgroundIcon, backgroundFrame, id,
                  style, readType, insetX, insetY)
    {
    }
    virtual void onKillFocus();
    virtual int onKeyPress(message* msg);
};

// CMPInputDlg - a CHeroWindowEx text-entry dialog (host name / password).
// DC field list 0x4493 (base CHeroWindowEx @0, DC size 0x60) lays out
// field1@0x4c, field2@0x50 (CMPInputEdit*), header1@0x54, header2@0x58,
// rollover@0x5c (textWidget*). Retail's CHeroWindowEx is four bytes wider,
// so every member shifts +4: the getter at 0x510970 reads rollover@0x60 and
// OnWidgetDeselect reads field1@0x50 (status@0x16 & WIDGET_ACTIVE, Text@0x30).
// The vtable 0x6400f4 is FIFTEEN slots, not fourteen: it runs 0x2400f4 to
// 0x24012f and CMPInputEdit's own table starts at 0x240130, so slot 14 is
// real and holds 0x510980 - UpdateOK. That is the one place this dialog
// diverges from CHotSeatDlg's roster (whose table stops at slot 13), and
// CMPInputEdit::OnKeyPress 0x50de50 calls it through `[edx+0x38]` rather
// than inlining it, which is the other half of the same proof.
// DisableOK/OnOK stay non-virtual. field1/field2 are DC CMPInputEdit* but
// reached only as textWidget here.
class CMPInputDlg : public CHeroWindowEx {
public:
    // CodeView CMPInputDlg field list 0x4493 owns this nested enum:
    // type 0x4483 / enumerators 0x4482 preserve these widget IDs and values.
    enum {
        BACKGROUND_ID = 500,
        FIELD1_ID = 501,
        FIELD2_ID = 502,
        HEADER1_ID = 503,
        HEADER2_ID = 504,
        OKAY_ID = 505,
        BACK_ID = 506,
        ROLLOVER_ID = 507
    };

    CMPInputEdit* m_field1;  // +0x50
    CMPInputEdit* m_field2;  // +0x54
    textWidget* m_header1;   // +0x58
    textWidget* m_header2;   // +0x5c
    textWidget* m_rollover;  // +0x60

    // DC OnHost/OnJoin retain calls to this source constructor, while
    // Complete expands it only in OnSearch. Standard inline gives VC6 those
    // three natural decisions; the prior forced-inline reconstruction required
    // artificial caller pins.
    inline CMPInputDlg(int maxChars1, int maxChars2);
    virtual ~CMPInputDlg();
    virtual int onWidgetDeselect(int id, bool& exitFlag);
    virtual textWidget* getRolloverWidget();
    unsigned char onOK();
    virtual void updateOK();  // slot 14, retail 0x510980
    inline void disableOK();
};
SIZE(CMPInputDlg, 0x64);

VA(0x00510060, 0x6F7)  // dc 0x1022f4
inline CMPInputDlg::CMPInputDlg(int maxChars1, int maxChars2)
    : CHeroWindowEx(284, 194, 232, 212, 18)
{
    m_widgets.reserve(6);
    m_widgets.push_back(new bitmapBorder(0, 0, m_width, m_height, BACKGROUND_ID,
                                       "MuDialog.pcx", 0x800));

    m_field1 = new CMPInputEdit(17, 66, 198, 23, maxChars1, "", "smalfont.fnt",
                              font::WHITE, 0, 0, 0, FIELD1_ID, 0x100, 0, 7, 5);
    m_field2 = new CMPInputEdit(17, 115, 198, 23, maxChars2, "", "smalfont.fnt",
                              font::WHITE, 0, 0, 0, FIELD2_ID, 0x100, 0, 7, 5);
    m_field1->setNextEdit(m_field2);
    m_field2->setNextEdit(m_field1);
    m_field1->setPrevEdit(m_field2);
    m_field2->setPrevEdit(m_field1);

    m_header1 = new textWidget(17, 43, 198, 18, "", "smalfont.fnt", font::WHITE,
                             -1, 1, 0, 8);
    m_header2 = new textWidget(17, 92, 198, 18, "", "smalfont.fnt", font::WHITE,
                             -1, 1, 0, 8);

    m_widgets.push_back(m_field1);
    m_widgets.push_back(m_field2);
    m_widgets.push_back(m_header1);
    m_widgets.push_back(m_header2);
    m_widgets.push_back(new button(26, 143, 64, 32, OKAY_ID, "mubchck.def", 0, 1,
                                 0, 28, 2));
    m_widgets.push_back(new button(142, 143, 64, 32, BACK_ID, "mubcanc.def", 0, 1,
                                 0, 1, 2));

    m_rollover = new textWidget(8, 186, 216, 18, 0, "smalfont.fnt", font::PRIMARY,
                              ROLLOVER_ID, 1, 32, 8);
    m_widgets.push_back(m_rollover);

    addWidgetsToMessageStream();
    setFocus(m_field1->m_id);
    m_field1->setAutoDraw(1);
    m_field2->setAutoDraw(1);
    getWidget(OKAY_ID)->setHelpText(g_dialogOkHelp, 0, 0);
    getWidget(BACK_ID)->setHelpText(g_dialogBackHelp, 0, 0);
}

// DC keeps this source helper out of line and OnWidgetDeselect calls it.
// Complete emits no standalone body, but the retail caller contains exactly
// its active-field/empty-text guard, proving that VC6 inlined the boundary.
inline unsigned char CMPInputDlg::onOK()
{
    if (m_field1->m_status & widget::WIDGET_ACTIVE) {
        if (!strlen(m_field1->m_text.c_str()))
            return 0;
    }
    return 1;
}

// E:\gamedcs\multiplayerwindow.cpp:521, dc 0x10286c
inline void CMPInputDlg::disableOK()
{
    getWidget(OKAY_ID)->enable(0);
}

// The CHotSeatDlg helpers DC keeps out of line (GetPlayerCount dc 0x102cf8,
// UpdateOK dc 0x102d4c, OnKillFocus dc 0x102cc8). Retail emits none: they
// expand into CHotSeatEdit's two overrides below. DC proves that UpdateOK only
// updates widget 519; OnKillFocus performs the following full-window redraw.
// Marked `inline` so the TU emits no COMDAT for bodies the image does not have.
// E:\gamedcs\multiplayerwindow.cpp:729, dc 0x102cc8
inline void CHotSeatDlg::onKillFocus(int id)
{
    updateOK();
    drawWindow(1, 0xffff0001, 0xffff);
}

inline int CHotSeatDlg::getPlayerCount()
{
    int players = 0;
    for (int i = 0; i < 8; ++i) {
        if (strlen(m_edit[i]->m_text.c_str()))
            ++players;
    }
    return players;
}

inline void CHotSeatDlg::updateOK()
{
    getWidget(OKAY_ID)->enable(getPlayerCount() > 1);
}

// Original: DeleteTempSaveGame; multiplayerwindow.cpp:872, dc 0xffb40.
// DC builds the same RMT path but elides deletion; Complete calls DeleteFileA.
void deleteTempSaveGame(const char* filename)
{
    char buffer[450];
    if (!strnicmp(filename, "RMT", 3)) {
        sprintf(buffer, "%s%s", ".\\DATA\\", filename);
        DeleteFileA(buffer);
    }
}

// Original: CHeroSessions::CHeroSessions; multiplayerwindow.cpp:1000,
// dc 0x102fcc. Construction initializes its CAutoArray base; there are no
// additional members. Complete expands that initialization at new CHeroSessions.
CHeroSessions::CHeroSessions()
{
}

// E:\gamedcs\multiplayerwindow.cpp:1005
// Slider callback for the session list; scrolls the displayed window of games.
void sliderGames(int state, heroWindow* parentWindow)
{
    static_cast<TMultiPlayerWindow*>(parentWindow)->m_currentIndex = state;
}

VA(0x0050de50, 0x8F)  // dc 0xffac0
int CMPInputEdit::onKeyPress(message* msg)
{
    int handled;

    if (!m_hasFocus) {
        handled = 0;
    } else if ((HIWORD(GetKeyState(VK_SHIFT)) && msg->m_codeX == KEYCODE_TAB)
               || msg->m_codeX == KEYCODE_KP_8) {
        onPrevEdit();
        handled = 1;
    } else if (msg->m_codeX == KEYCODE_TAB || msg->m_codeX == KEYCODE_ENTER
               || msg->m_codeX == KEYCODE_KP_2) {
        onNextEdit();
        handled = 1;
    } else {
        handled = textEntryWidget::onKeyPress(msg);
    }

    static_cast<CMPInputDlg*>(m_parentWindow)->updateOK();
    return handled;
}

VA(0x0050dee0, 0x7F)  // dc 0xffaec
void CHotSeatEdit::onKillFocus()
{
    textEntryWidget::onKillFocus();
    static_cast<CHotSeatDlg*>(m_parentWindow)->onKillFocus(m_id);
}

// share CMPEdit's ring layout and inherited navigation slots. Preserve the
// qualified base call instead of casting between unrelated class copies.
VA(0x0050df60, 0xEE)  // dc 0xffb0c
int CHotSeatEdit::onKeyPress(message* msg)
{
    int handled = CMPEdit::onKeyPress(msg);

    static_cast<CHotSeatDlg*>(m_parentWindow)->updateOK();
    static_cast<CHotSeatDlg*>(m_parentWindow)->drawWindow(1, 0xffff0001,
                                                          0xffff);
    return handled;
}

// Residual (80.46%): every widget, its screen coordinates, def/pcx name,
// widget id and the whole add order (push_back for the members, single-element
// insert for the slider / session rows / map border) are byte-exact, and the
// frame matches retail's 0x1e0. Two pervasive CL-generation deltas remain, both
// register/inliner rather than source: (1) retail hoists 0 into ebx at entry
// (`xor ebx,ebx`) and reuses it for the base-ctor zero args, hostJoinScreen,
// the EH-state clears and every `new` null test (`cmp eax,ebx`); our SP3 CL
// materialises those as immediates and establishes ebx=0 later - and it swaps
// the EH-state-byte store past the null test at each `new` site. (2) the STL
// single-element `insert` over-inlines `_Ucopy` (7 sites vs retail's 12); the
// caller is already maximal so no shrink lever applies. Storing the slider/map
// widgets through a local before the insert (rather than re-reading the member)
// was worth +1.5.
// Current residual (83.43%): DeleteTempSaveGame and CHeroSessions expand,
// including the proven DeleteFileA path. Nested widget-vector insert/copy
// operations still over-expand; retail retains fourteen additional call sites.
VA(0x0050e050, 0xCFC)  // anchor-vtable 0x6400a0 + CHeroWindowEx base + DeleteFileA + 800x600 dims, dc 0xffb70
TMultiPlayerWindow::TMultiPlayerWindow()
    : CHeroWindowEx(0, 0, 800, 600, 0)
{
    m_x = 173;
    m_y = 55;
    m_width = 454;
    m_height = 490;
    m_type = 16;
    g_multiPlayerWindow = this;
    m_hostJoinScreen = 0;

    m_widgets.reserve(77);

    m_widgets.push_back(new bitmapBorder(0, 0, 454, 490, 100, "mupopup.pcx", 0x800));

    m_hotSeat = 0;
    if (!g_noCdRom)
        m_hotSeat = new button(373, 78, 64, 48, 102, "muBhot.def", 0, 1, 0, 0, 2);

    m_ipx = new button(373, 135, 64, 48, 103, "muBipx.def", 0, 1, 0, 0, 2);
    m_tcp = new button(373, 192, 64, 48, 104, "muBtcp.def", 0, 1, 0, 0, 2);
    m_modem = new button(373, 249, 64, 48, 105, "muBmodm.def", 0, 1, 0, 0, 2);
    m_direct = new button(373, 306, 64, 48, 106, "muBdrct.def", 0, 1, 0, 0, 2);
    m_online = new button(373, 363, 64, 48, 101, "mubonl.def", 0, 1, 0, 0, 2);

    m_host = 0;
    if (!g_noCdRom)
        m_host = new button(373, 78, 64, 48, 107, "muBhost.def", 0, 1, 0, 0, 2);

    m_join = new button(373, 135, 64, 48, 108, "muBjoin.def", 0, 1, 0, 0, 2);
    m_search = new button(373, 192, 64, 48, 109, "muBsrch.def", 0, 1, 0, 0, 2);
    m_cancel = new button(373, 424, 64, 48, 124, "muBcanc.def", 0, 1, 0, 1, 2);

    m_sessNameHeader = new textWidget(216 - m_x, 146 - m_y, 127, 18,
                                    g_generalText->getText(GENERAL_TEXT_SESSION_NAME), "smalfont.fnt",
                                    font::PRIMARY, 127, 1, 0, 8);
    m_userNameHeader = new textWidget(346 - m_x, 146 - m_y, 127, 18,
                                    g_generalText->getText(GENERAL_TEXT_USER_NAME), "smalfont.fnt",
                                    font::PRIMARY, 128, 1, 0, 8);
    m_playerName = new CMultiPlayerWindowEdit(19, 436, 334, 18, 21,
                                            g_config.m_networkDefaultName, "smalfont.fnt",
                                            font::WHITE, 0, 0, 0, 125, 0x100, 0,
                                            7, 5);

    if (m_hotSeat)
        m_widgets.push_back(m_hotSeat);
    m_widgets.push_back(m_ipx);
    m_widgets.push_back(m_tcp);
    m_widgets.push_back(m_modem);
    m_widgets.push_back(m_direct);
    m_widgets.push_back(m_online);
    m_widgets.push_back(m_host);
    m_widgets.push_back(m_join);
    m_widgets.push_back(m_search);
    m_widgets.push_back(m_cancel);
    m_widgets.push_back(m_sessNameHeader);
    m_widgets.push_back(m_userNameHeader);
    m_widgets.push_back(m_playerName);

    m_rolloverWidget = new textWidget(8, 465, 438, 18, 0, "smalfont.fnt",
                                    font::PRIMARY, 123, 1, 32, 8);
    m_widgets.push_back(m_rolloverWidget);

    widget* gs = new slider(337, 81, 16, 330, 122, 10, sliderGames,
                            slider::BLUE, 0, 0);
    m_gameSlider = gs;
    m_widgets.push_back(gs);

    int sessionRowY = 112;
    for (int i = 0; sessionRowY < 412; sessionRowY += 25, i++)
        m_widgets.push_back(new textWidget(18, sessionRowY, 317, 22, 0,
                                      "smalfont.fnt", font::PRIMARY, 110 + i, 1,
                                      0, 8));

    widget* mapBorder = new bitmapBorder(16, 77, 338, 335, 129, "mumap.pcx", 0x800);
    m_splash = mapBorder;
    m_widgets.push_back(mapBorder);

    addWidgetsToMessageStream();
    setFocus(m_playerName->m_id);
    static_cast<slider*>(m_gameSlider)->setResolution(0);

    m_sessions = new CHeroSessions;
    m_sessTimer = 0;
    m_localIpAddress[0] = 0;
    m_sessionRefreshTimeout = 0;
    m_gameState = ResourceManager::getSprite("muGstat.def");
    m_inSessionList = 0;
    m_currentIndex = 0;
    m_currentGame = 0;

    setHelpText(g_multiPlayerHelp, 101, 110, 0);
    m_gameSlider->setHelpText(g_multiPlayerHelp[21].m_text,
                              g_multiPlayerHelp[21].m_rclick, 0);
    m_cancel->setHelpText(g_multiPlayerHelp[23].m_text,
                          g_multiPlayerHelp[23].m_rclick, 0);

    deleteTempSaveGame(g_config.m_scFile);

    goMainMenu();
}

VA(0x0050ed60, 0x43)  // dc 0x101e98
int CMultiPlayerWindowEdit::onKeyPress(message* msg)
{
    if (getCharPressed(msg) == KEYCODE_ENTER)
        return 0;

    int result = textEntryWidget::onKeyPress(msg);
    if (!result)
        return 0;
    g_multiPlayerWindow->update();
    return result;
}

VA_COMPGEN(0x0050ede0, 0x21, SCALAR_DELETING_DTOR, CHeroSessions)
VA_COMPGEN(0x00558350, 0x54, IMPLICIT_DTOR, CHeroSessions)

VA_COMPGEN(0x0050edb0, 0x21, SCALAR_DELETING_DTOR, TMultiPlayerWindow)

VA(0x0050ee40, 0xAB)  // dc 0x100430
TMultiPlayerWindow::~TMultiPlayerWindow()
{
    g_multiPlayerWindow = 0;
    m_gameState->dispose();
    deleteWidgets();
    m_sessions->destroy();
    delete m_sessions;
}

VA(0x0050eef0, 0xC9)  // dc 0x100498
void TMultiPlayerWindow::goSessionList()
{
    m_inSessionList = 1;
    m_showSplash = 0;
    m_splash->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    if (m_hotSeat)
        m_hotSeat->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_ipx->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_tcp->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_modem->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_direct->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    if (m_host)
        m_host->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_join->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_search->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_online->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_userNameHeader->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_sessNameHeader->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
}

VA(0x0050efc0, 0x12B)  // dc 0x10051c
void TMultiPlayerWindow::goMainMenu()
{
    m_inSessionList = 0;
    m_showSplash = 1;
    m_sessTimer = 0;
    m_hostJoinScreen = 0;
    m_splash->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    if (m_hotSeat)
        m_hotSeat->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_ipx->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_tcp->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_modem->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_direct->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    if (m_host)
        m_host->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_join->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_search->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_online->sendMessage(widget::WIDGET_SET_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_userNameHeader->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_sessNameHeader->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    widget* w = getWidget(126);
    if (w)
        w->sendMessage(widget::WIDGET_CLEAR_STATUS, widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_sessions->destroy();
}

VA(0x0050f0f0, 0x3E6)  // anchor-callee: sole big drawing method (font::DrawBoundedString x3, CSprite::Draw, session-name strncpy/sprintf), size 0.99x DC, dc 0x1005fc
void TMultiPlayerWindow::update()
{
    char userBuf[256];
    char nameBuf[256];
    char countBuf[100];

    int wx = m_x;
    int wy = m_y;
    int numPlayers;
    CHeroSessions::eSessionStatus status;
    unsigned long count = m_sessions->getCount();
    int shown = 0;
    unsigned char haveName = 0;

    const char* pn = m_playerName->m_text.c_str();
    unsigned char anySelected = 0;
    if (pn && strlen(pn))
        haveName = 1;

    if (!m_showSplash) {
        if (count > 0) {
            drawWindow(0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
            if (count >= 12)
                count = 12;
            int row = 0;
            if (count > 0) {
                wy = wy + 0x70;
                do {
                    if (!m_sessions->getSessionInfo(
                            row + m_currentIndex, nameBuf, userBuf, numPlayers,
                            status))
                        return;

                    int isSelected = m_currentGame == row + m_currentIndex;
                    if (status != CHeroSessions::closed) {
                        if (isSelected)
                            anySelected = 1;
                        shown++;
                    }

                    m_gameState->draw(0, status, 0, 0,
                                    g_multiPlayerWindow->m_gameState->getWidth(),
                                    g_multiPlayerWindow->m_gameState->getHeight(),
                                    g_windowManager->m_screenBitmap, wx + 0x12,
                                    wy, 0, 1);
                    int fontColor = isSelected ? 5 : 1;
                    g_smallFont->drawBoundedString(
                        nameBuf, g_windowManager->m_screenBitmap, wx + 0x2b, wy,
                        0x80, 0x16, font::TColor(fontColor), 5, -1);
                    g_smallFont->drawBoundedString(
                        userBuf, g_windowManager->m_screenBitmap, wx + 0xad, wy,
                        0x80, 0x16, font::TColor(fontColor), 5, -1);
                    sprintf(countBuf, "%d", numPlayers);
                    g_smallFont->drawBoundedString(
                        countBuf, g_windowManager->m_screenBitmap, wx + 0x130,
                        wy, 0x1e, 0x16, font::TColor(fontColor), 5, -1);
                    ++row;
                    wy += 0x19;
                } while (row < count);
            }

            if (shown > 0 && haveName && anySelected)
                m_join->enable(1);
            else
                m_join->enable(0);
            if (m_host) {
                m_host->enable(haveName);
                m_host->draw();
            }
            m_join->draw();
        } else {
            if (m_hostJoinScreen)
                m_join->enable(haveName);
            else
                m_join->enable(0);
            if (m_host)
                m_host->enable(haveName);
            drawWindow(0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        }
    } else {
        if (m_hostJoinScreen) {
            if (m_host)
                m_host->enable(haveName);
            m_join->enable(haveName);
        } else {
            if (m_hotSeat)
                m_hotSeat->enable(haveName);
            m_ipx->enable(haveName);
            m_tcp->enable(haveName);
            m_modem->enable(haveName);
            m_direct->enable(haveName);
        }
        drawWindow(0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    }

    g_windowManager->updateScreen(m_x, m_y, m_width, m_height);
}

// Original: TMultiPlayerWindow::RefreshSessions; multiplayerwindow.cpp:1344,
// dc 0x100c44. Expanded in onTCP and the timed window handler.
void TMultiPlayerWindow::refreshSessions()
{
    m_sessions->destroy();
    g_dPlay->enumSessions(m_sessions, m_sessionRefreshTimeout, 0x52);
    m_sessTimer = GameTime::get();
    static_cast<slider*>(m_gameSlider)->updateResolution(
        m_sessions->getCount() - 12);
    update();
}

// Original: TMultiPlayerWindow::CheckSessions; multiplayerwindow.cpp:1449,
// dc 0x100e94. The second timer read follows the complete refresh operation.
void TMultiPlayerWindow::checkSessions()
{
    if (m_sessTimer
        && GameTime::elapsedSince(m_sessTimer) > m_sessionRefreshTimeout) {
        refreshSessions();
        m_sessTimer = GameTime::get();
    }
}

// E:\gamedcs\multiplayerwindow.cpp:1461
// Complete inlines this body into OnHost. Keeping the source boundary is
// material: VC6 leaves the nested member InitRemote call out of line, exactly
// as retail does.
unsigned char TMultiPlayerWindow::onModemHost()
{
    if (!initRemote(g_mpNetProtocol, 0, 0)) {
        normalDialog(g_generalText->getText(GENERAL_TEXT_MODEM_CONNECTION_INITIALIZATION_ERROR), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return 0;
    }

    g_mpExtendedType = 1;
    g_mpBaseType = 1;
    ShowCursor(1);
    if (!hostSession(g_generalText->getText(GENERAL_TEXT_MODEM_SESSION), 0)) {
        ShowCursor(0);
        g_windowManager->m_dialogReturn = DIALOG_RETURN_CANCEL;
        remoteCleanup();
        return 0;
    }
    ShowCursor(0);
    return 1;
}

// DC retains these three member helpers and OnWidgetDeselect calls them.
// Complete emits no standalone bodies; the corresponding retail case arms
// contain the complete inlined bodies, including Complete's expanded IPX
// session setup and the later widget-status API.
inline unsigned char TMultiPlayerWindow::onIPX()
{
    g_mpNetProtocol = MP_IPX;
    if (::initRemote(MP_IPX, m_playerName->m_text.c_str()) &&
        initConnection(0, 0)) {
        DPCAPS caps;
        g_dPlay->getCaps(&caps, 1);
        m_sessionRefreshTimeout = caps.m_timeout + 100;
        if (g_mpNetProtocol == MP_TCP)
            m_sessionRefreshTimeout = 1000;

        if (m_host)
            m_host->sendMessage(
                widget::WIDGET_SET_STATUS,
                widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        m_join->sendMessage(widget::WIDGET_SET_STATUS,
                           widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        m_join->enable(0);
        refreshSessions();
        return 1;
    }

    normalDialog(g_generalText->getText(GENERAL_TEXT_IPX_CONNECTION_INITIALIZATION_ERROR), 1, -1, -1,
                 -1, 0, -1, 0, -1, 0, -1, 0);
    return 0;
}

inline unsigned char TMultiPlayerWindow::onModem()
{
    g_mpNetProtocol = MP_MODEM;
    m_hostJoinScreen = 1;
    m_splash->sendMessage(widget::WIDGET_SET_STATUS,
                         widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    if (m_host)
        m_host->sendMessage(widget::WIDGET_SET_STATUS,
                           widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_join->sendMessage(widget::WIDGET_SET_STATUS,
                       widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_join->enable(1);
    update();
    return 1;
}

VA(0x0050f4e0, 0x458)  // anchor-vtable 0x6400a0 slot 12 (OnWidgetDeselect), dc 0x1009a4
int TMultiPlayerWindow::onWidgetDeselect(int id, bool& exitFlag)
{
    bool connectionFailed = 0;
    switch (id) {
    case CANCEL_ID:
        if (!m_inSessionList) {
            connectionFailed = 1;
            break;
        }
        goto return_to_main_menu;

    case IPX_ID: {
        goSessionList();
        if (!onIPX()) {
            connectionFailed = 1;
            break;
        }
        return 1;
    }

    case TCP_ID:
        goSessionList();
        if (!onTCP()) {
            remoteCleanup();
            exitFlag = 1;
            g_windowManager->m_dialogReturn = DIALOG_RETURN_CANCEL;
            return 1;
        }
        break;

    case MODEM_ID:
        goSessionList();
        if (!onModem()) {
            remoteCleanup();
            exitFlag = 1;
            g_windowManager->m_dialogReturn = DIALOG_RETURN_CANCEL;
        }
        return 1;

    case DIRECT_ID:
        goSessionList();
        if (!onDirect()) {
            remoteCleanup();
            exitFlag = 1;
            g_windowManager->m_dialogReturn = DIALOG_RETURN_CANCEL;
        }
        return 1;

    case ONLINE_ID:
        _chdir("online");
        ShellExecuteA(g_hwndApp, "open", "autorun.exe", 0, 0, SW_SHOWNORMAL);
        shutDown(0);
        return 1;

    case HOST_ID:
        if (onHost())
            goto exit_dialog;
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_CANCEL) {
            g_windowManager->m_dialogReturn = DIALOG_RETURN_CANCEL;
            remoteCleanup();
            exitFlag = 1;
            return 1;
        }
        goto check_host_join_screen;

    case JOIN_ID:
        if (onJoin()) {
            exitFlag = 1;
            return 1;
        }

check_host_join_screen:
        if (m_hostJoinScreen)
            goto return_to_main_menu;
        break;

    case SEARCH_ID:
        if (onSearch())
            goto exit_dialog;
        break;

    exit_dialog:
        exitFlag = 1;
        return 1;

    case HOT_SEAT_ID:
        if (onHotSeat()) {
            exitFlag = 1;
            return 1;
        }
        break;

    case FIRST_SESSION_ID:
    case FIRST_SESSION_ID + 1:
    case FIRST_SESSION_ID + 2:
    case FIRST_SESSION_ID + 3:
    case FIRST_SESSION_ID + 4:
    case FIRST_SESSION_ID + 5:
    case FIRST_SESSION_ID + 6:
    case FIRST_SESSION_ID + 7:
    case FIRST_SESSION_ID + 8:
    case FIRST_SESSION_ID + 9:
    case FIRST_SESSION_ID + 10:
    case LAST_SESSION_ID: {
        int game = m_currentIndex + id - FIRST_SESSION_ID;
        if (game < m_sessions->getCount()) {
            m_currentGame = game;
            update();
        }
        break;
    }

    case GAME_SLIDER_ID:
    case ROLLOVER_ID:
        break;
    }

    if (connectionFailed) {
        exitFlag = 1;
        g_windowManager->m_dialogReturn = DIALOG_RETURN_CANCEL;
        remoteCleanup();
        return 1;
    }

    return 1;

return_to_main_menu:
    remoteCleanup();
    goMainMenu();
    drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    update();
    return 1;
}

VA(0x0050f940, 0xC5)  // anchor-vtable 0x6400a0 slot 9 (WindowHandler); ret 4 = (this,message*)->int.
                      // 197 B vs DC 40: retail inlines the timer-gated session refresh (PollSound +
                      // GameTime::Get) that DC keeps in RefreshSessions/CheckSessions. dc 0x100c1c
int TMultiPlayerWindow::windowHandler(message& msg)
{
    pollSound();
    checkSessions();

    return CHeroWindowEx::windowHandler(msg);
}

VA(0x0050fa10, 0x9F)  // dc 0x100ca0
unsigned char TMultiPlayerWindow::joinSession(CDPlaySession* session, const char* password)
{
    if (!g_dPlay->joinSession(&session->m_guidInstance,
                             const_cast<char*>(password)))
        return 0;

    int version = *g_videoGameState;
    g_thisNetPlayerInfo.m_dpid = g_dPlay->createPlayer(
        g_config.m_networkDefaultName, &version, sizeof(version), 0);
    strcpy(g_thisNetPlayerInfo.m_name, g_config.m_networkDefaultName);
    g_thisNetPlayerInfo.m_version = version;

    if (g_dPlay->getLastError())
        return 0;

    m_sessTimer = 0;
    return 1;
}

VA(0x0050fab0, 0x106)  // dc 0x100d0c
unsigned char TMultiPlayerWindow::hostSession(const char* sessName, const char* password)
{
    char fullName[256];
    sprintf(fullName,
            DATA_COMPGEN(0x006816e4, multiplayerSessionNameFormat, "%s%c%s"),
            sessName, 0xfa, g_config.m_networkDefaultName);

    unsigned long flags = 4;
    if (g_sessionKeepAlive)
        flags |= 0x40;
    if (g_mpNetProtocol != MP_TCP)
        flags |= 0x2000;

    if (!g_dPlay->hostSession(fullName, flags, 8,
                             const_cast<char*>(password)))
        return 0;

    int version = *g_videoGameState;
    g_thisNetPlayerInfo.m_dpid = g_dPlay->createPlayer(
        g_config.m_networkDefaultName, &version, sizeof(version), 0);
    if (!g_thisNetPlayerInfo.m_dpid)
        return 0;

    strcpy(g_thisNetPlayerInfo.m_name, g_config.m_networkDefaultName);
    g_thisNetPlayerInfo.m_version = version;

    if (g_mpNetProtocol == MP_TCP)
        g_dPlay->getIPAddress(g_thisNetPlayerInfo.m_dpid, m_localIpAddress);

    return 1;
}

VA(0x0050fbc0, 0x86)  // dc 0x100e18
unsigned char TMultiPlayerWindow::initRemote(eNetGameType netGameType, const char* extra, _DPCOMPORTADDRESS* comportInfo)
{
    DPCAPS dpCaps;

    g_mpNetProtocol = netGameType;
    if (!::initRemote(netGameType, m_playerName->m_text.c_str()))
        return 0;
    if (!initConnection(const_cast<char*>(extra), comportInfo))
        return 0;

    g_dPlay->getCaps(&dpCaps, 1);
    m_sessionRefreshTimeout = dpCaps.m_timeout + 100;
    if (g_mpNetProtocol == MP_TCP)
        m_sessionRefreshTimeout = 1000;
    return 1;
}

// E:\gamedcs\multiplayerwindow.cpp:2043, dc 0x101ca4
inline unsigned char TMultiPlayerWindow::onDirect()
{
    g_mpNetProtocol = MP_SERIAL;
    m_hostJoinScreen = 1;
    m_splash->sendMessage(widget::WIDGET_SET_STATUS,
                         widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    if (m_host)
        m_host->sendMessage(widget::WIDGET_SET_STATUS,
                           widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_join->sendMessage(widget::WIDGET_SET_STATUS,
                       widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    m_join->enable(1);
    update();
    return 1;
}

VA(0x0050fc50, 0x14F)  // dc 0x100f94
unsigned char TMultiPlayerWindow::onDirectHost()
{
    if (!initRemote(MP_SERIAL, 0, 0)) {
        normalDialog(g_generalText->getText(GENERAL_TEXT_SERIAL_CONNECTION_INITIALIZATION_ERROR), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return 0;
    }

    g_mpExtendedType = 1;
    g_mpBaseType = 1;
    ShowCursor(1);

    if (!hostSession(g_generalText->getText(GENERAL_TEXT_SERIAL_SESSION), 0)) {
        ShowCursor(0);
        if (g_dPlay->getLastError() != g_dplayErrorUserCancel)
            normalDialog(g_generalText->getText(GENERAL_TEXT_SERIAL_CONNECTION_HOST_ERROR), 1, -1, -1,
                         -1, 0, -1, 0, -1, 0, -1, 0);
        g_windowManager->m_dialogReturn = DIALOG_RETURN_CANCEL;
        remoteCleanup();
        return 0;
    }

    ShowCursor(0);
    return 1;
}

VA(0x0050fda0, 0x2B7)  // dc 0x101058
unsigned char TMultiPlayerWindow::onHost()
{
    if (g_mpNetProtocol == MP_MODEM)
        return onModemHost();

    if (g_mpNetProtocol == MP_SERIAL)
        return onDirectHost();

    g_mpExtendedType = 1;
    g_mpBaseType = 1;
    strcpy(g_config.m_networkDefaultName, m_playerName->getText());

    CMPInputDlg sessDlg(20, 20);
    sessDlg.m_field1->setText(g_generalText->getText(GENERAL_TEXT_MY_GAME));
    sessDlg.m_header1->setText(g_sessionNameLabel);
    sessDlg.m_header2->setText(g_generalText->getText(GENERAL_TEXT_PASSWORD));
    sessDlg.m_field1->setHelpText(g_sessionNameHelp, 0, 0);
    sessDlg.m_field2->setHelpText(g_sessionPasswordHelp, 0, 0);
    sessDlg.doModal(0);

    if (g_windowManager->m_dialogReturn == DIALOG_RETURN_CANCEL)
        return 0;

    const char* password = sessDlg.m_field2->m_text.c_str();
    if (!strlen(password))
        password = 0;
    if (!hostSession(sessDlg.m_field1->m_text.c_str(), password))
        return 0;
    return 1;
}

VA(0x00510760, 0x62)  // dc 0x102014
CMPEdit::CMPEdit(int x, int y, int w, int h, int textSize, const char* text,
                 const char* fontName, font::TColor color,
                 unsigned justification, const char* backgroundIcon,
                 int backgroundFrame, int id, int style, int readType,
                 int insetX, int insetY)
    : textEntryWidget(x, y, w, h, textSize, text, fontName, color, justification,
                      backgroundIcon, backgroundFrame, id, style, readType,
                      insetX, insetY)
{
    m_nextEdit = 0;
    m_prevEdit = 0;
}

VA(0x005107d0, 0x73)  // dc 0x1020c4
int CMPEdit::onKeyPress(message* msg)
{
    if (!m_hasFocus)
        return 0;

    if ((HIWORD(GetKeyState(VK_SHIFT)) && msg->m_codeX == KEYCODE_TAB)
        || msg->m_codeX == KEYCODE_KP_8) {
        onPrevEdit();
        return 1;
    }

    if (msg->m_codeX == KEYCODE_TAB || msg->m_codeX == KEYCODE_ENTER
        || msg->m_codeX == KEYCODE_KP_2) {
        onNextEdit();
        return 1;
    }

    return textEntryWidget::onKeyPress(msg);
}

VA(0x00510850, 0x1B)  // dc 0x10215c; m_nextEdit/status/id + parent SetFocus
void CMPEdit::onNextEdit()
{
    if (m_nextEdit && (m_nextEdit->m_status & widget::WIDGET_ACTIVE))
        m_parentWindow->setFocus(m_nextEdit->m_id);
}

VA(0x00510870, 0x1B)  // dc 0x102184; m_prevEdit/status/id + parent SetFocus
void CMPEdit::onPrevEdit()
{
    if (m_prevEdit && (m_prevEdit->m_status & widget::WIDGET_ACTIVE))
        m_parentWindow->setFocus(m_prevEdit->m_id);
}

VA(0x00510890, 0x10)  // dc 0x1021ac
void CMPEdit::setFocus(bool state)
{
    textEntryWidget::setFocus(state);
}

VA(0x005108a0, 0x4E)  // dc 0x102718
CMPInputDlg::~CMPInputDlg()
{
    deleteWidgets();
}

VA(0x005108f0, 0x72)  // dc 0x10275c
int CMPInputDlg::onWidgetDeselect(int id, bool& exitFlag)
{
    switch (id) {
    case OKAY_ID:
        if (onOK()) {
            exitFlag = 1;
            g_windowManager->m_dialogReturn = DIALOG_RETURN_OK;
            return 1;
        }
        break;

    case BACK_ID:
        exitFlag = 1;
        g_windowManager->m_dialogReturn = DIALOG_RETURN_CANCEL;
        return 1;
    }

    return 0;
}

VA(0x00510970, 0x4)  // dc 0x1027ec
textWidget* CMPInputDlg::getRolloverWidget()
{
    return m_rollover;
}

VA(0x00510980, 0x5D)  // dc 0x1027f4
void CMPInputDlg::updateOK()
{
    if (m_field1->m_status & widget::WIDGET_ACTIVE) {
        if (!strlen(m_field1->m_text.c_str()))
            getWidget(OKAY_ID)->enable(0);
        else
            getWidget(OKAY_ID)->enable(1);
        drawWindow(1, 0xffff0001, 0xffff);
    }
}

VA_COMPGEN(0x005109e0, 0x21, SCALAR_DELETING_DTOR, CMPInputDlg)

VA(0x00510a10, 0x298)  // dc 0x1011b8
unsigned char TMultiPlayerWindow::onModemJoin()
{
    if (!initRemote(MP_MODEM, 0, 0)) {
        if (g_dPlay->getLastError() != g_dplayErrorUserCancel)
            normalDialog(g_generalText->getText(GENERAL_TEXT_MODEM_CONNECTION_INITIALIZATION_ERROR), 1, -1, -1,
                         -1, 0, -1, 0, -1, 0, -1, 0);
        return 0;
    }

    ShowCursor(1);
    m_sessions->destroy();
    for (int retry = 0; retry < 3; ++retry) {
        g_dPlay->enumSessions(m_sessions, 0, 0x42);
        if (g_dPlay->getLastError() == g_dplayErrorUserCancel)
            break;
        if (!retry)
            g_windowManager->updateScreen(0, 0, 800, 600);
        if (m_sessions->getCount() > 0)
            break;
    }

    if (!m_sessions->getCount()) {
        ShowCursor(0);
        if (g_dPlay->getLastError() != g_dplayErrorUserCancel)
            normalDialog(g_generalText->getText(GENERAL_TEXT_NETWORK_NO_SESSIONS_FOUND), 1, -1, -1,
                         -1, 0, -1, 0, -1, 0, -1, 0);
        return 0;
    }

    ShowCursor(0);
    CDPlaySession* session = m_sessions->get(0);
    Sleep(1000);
    if (!joinSession(session, 0)) {
        if (g_dPlay->getLastError() != g_dplayErrorUserCancel)
            normalDialog(g_generalText->getText(GENERAL_TEXT_SESSION_CONNECTION_ERROR), 1, -1, -1,
                         -1, 0, -1, 0, -1, 0, -1, 0);
        return 0;
    }
    return 1;
}

VA(0x00510cb0, 0x282)  // dc 0x101374
unsigned char TMultiPlayerWindow::onDirectJoin()
{
    if (!initRemote(MP_SERIAL, 0, 0)) {
        normalDialog(g_generalText->getText(GENERAL_TEXT_SERIAL_CONNECTION_INITIALIZATION_ERROR), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return 0;
    }

    m_sessions->destroy();
    ShowCursor(1);
    for (int retry = 0; retry < 2; ++retry) {
        unsigned char enumFailed = !g_dPlay->enumSessions(
            m_sessions, m_sessionRefreshTimeout, 0x42);
        if (g_dPlay->getLastError() == g_dplayErrorUserCancel)
            break;
        if (enumFailed)
            break;
        if (!retry)
            g_windowManager->updateScreen(0, 0, 800, 600);
        if (m_sessions->getCount() > 0)
            break;
        if (!retry)
            Sleep(500);
    }

    if (!m_sessions->getCount()) {
        ShowCursor(0);
        normalDialog(g_generalText->getText(GENERAL_TEXT_NETWORK_GAME_NOT_FOUND), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        remoteCleanup();
        g_mpNetProtocol = MP_SERIAL;
        return 0;
    }

    ShowCursor(0);
    CDPlaySession* session = m_sessions->get(0);
    if (!joinSession(session, 0)) {
        normalDialog(g_generalText->getText(GENERAL_TEXT_SESSION_CONNECTION_ERROR), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return 0;
    }
    return 1;
}

VA(0x00510f40, 0x380)  // dc 0x101510
unsigned char TMultiPlayerWindow::onJoin()
{
    if (g_mpNetProtocol == MP_MODEM)
        return onModemJoin();
    if (g_mpNetProtocol == MP_SERIAL)
        return onDirectJoin();

    g_windowManager->m_dialogReturn = DIALOG_RETURN_OK;
    g_mpExtendedType = 2;
    g_mpBaseType = 1;
    g_waitForRemoteReceive = 1;
    strcpy(g_config.m_networkDefaultName, m_playerName->getText());

    char userName[256];
    char sessName[256];
    int numPlayers;
    CHeroSessions::eSessionStatus status;
    if (!m_sessions->getSessionInfo(static_cast<unsigned long>(m_currentGame),
                                   sessName, userName,
                                   numPlayers, status))
        return 0;

    CDPlaySession* session = m_sessions->get(m_currentGame);
    if (!session)
        return 0;

    const char* password = 0;
    CMPInputDlg dlg(20, 20);
    if (session->isPasswordProtected()) {
        dlg.m_header1->setText(g_sessionNameLabel);
        dlg.m_header2->setText(g_generalText->getText(GENERAL_TEXT_PASSWORD));
        dlg.m_field1->enable(0);
        dlg.m_field1->setText(sessName);
        dlg.drawWindow(1, 0xffff0001, 0xffff);
        dlg.setFocus(dlg.m_field2->m_id);
        dlg.m_field1->setHelpText(g_sessionNameLabel, 0, 0);
        dlg.m_field2->setHelpText(g_sessionPasswordHelp, 0, 0);
        dlg.doModal(0);
        if (g_windowManager->m_dialogReturn == DIALOG_RETURN_CANCEL)
            return 0;
        password = dlg.m_field2->getText();
    }

    if (joinSession(session, password))
        return 1;

    const char* errorText = g_generalText->getText(GENERAL_TEXT_SESSION_CONNECTION_ERROR);
    if (g_dPlay->getLastError() == static_cast<long>(0x88770154))
        errorText = g_generalText->getText(GENERAL_TEXT_INVALID_PASSWORD);
    normalDialog(errorText, 1, -1, -1,
                 -1, 0, -1, 0, -1, 0, -1, 0);
    return 0;
}

VA(0x005112e0, 0x101)  // dc 0x101780
unsigned char getIPAddress(char* ipAddress)
{
    WSADATA wsaData;
    char hostName[256];
    TIPv4SocketAddress address;

    if (WSAStartup(MAKEWORD(1, 1), &wsaData))
        return 0;

    SOCKET socketHandle = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketHandle == INVALID_SOCKET)
        return 0;

    address.m_internet.sin_family = AF_INET;
    address.m_internet.sin_port = htons(2000);
    address.m_internet.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(socketHandle, &address.m_generic, sizeof(address.m_internet))
        == SOCKET_ERROR)
        return 0;
    u_long nonBlocking = 1;
    if (ioctlsocket(socketHandle, FIONBIO, &nonBlocking) == SOCKET_ERROR)
        return 0;
    if (gethostname(hostName, 255) == SOCKET_ERROR)
        return 0;

    hostent* host = gethostbyname(hostName);
    in_addr internetAddress;
    memcpy(&internetAddress, host->h_addr_list[0], sizeof(internetAddress));
    strcpy(ipAddress, inet_ntoa(internetAddress));
    closesocket(socketHandle);
    return 1;
}

VA(0x005113f0, 0x263)  // dc 0x101784
unsigned char TMultiPlayerWindow::onTCP()
{
    char ipAddress[80];

    if (!initRemote(MP_TCP, 0, 0)) {
        normalDialog(g_generalText->getText(GENERAL_TEXT_TCP_IP_CONNECTION_INITIALIZATION_ERROR), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return 0;
    }

    if (m_host)
        m_host->show();
    m_join->show();
    m_search->show();
    m_search->enable(1);
    m_join->enable(0);

    if (getIPAddress(ipAddress)) {
        if (!getWidget(IP_ADDRESS_ID)) {
            char addressText[256];
            textWidget* ipWidget = new textWidget(
                0, 16, m_width, 50, 0, "bigfont.fnt", font::PRIMARY,
                IP_ADDRESS_ID, 1, 0, 8);
            m_widgets.push_back(ipWidget);
            addWidget(ipWidget, -1);
            sprintf(addressText, g_generalText->getText(GENERAL_TEXT_IP_ADDRESS_FORMAT), ipAddress);
            ipWidget->setText(addressText);
        }
    }

    m_sessions->destroy();
    g_dPlay->enumSessions(m_sessions, m_sessionRefreshTimeout, 0x52);
    m_sessTimer = GameTime::get();
    static_cast<slider*>(m_gameSlider)->updateResolution(
        m_sessions->getCount() - 12);
    update();
    return 1;
}

// E:\gamedcs\multiplayerwindow.cpp:1944
// Complete expands CMPInputDlg's constructor at this site, asks for a TCP
// address, tears down the browser's current DirectPlay connection, and runs
// one bounded session enumeration. The four failure dialogs are pinned by
// their folded general-text indices (459, 463, 456, then GetErrorDesc), and
// the Dreamcast xref graph independently records exactly four NormalDialog
// calls plus JoinSession, InitRemote, CAutoArray::Destroy and CHourGlass.
VA(0x00511660, 0x666)  // caller slot + complete TCP search flow, dc 0x10196c
unsigned char TMultiPlayerWindow::onSearch()
{
    // Current 89.766%, banked MAX 91.324%: all 13 retail branch tests are
    // present, but C1 feeds
    // the allocator the first two call-crossing pseudos in the opposite order:
    // candidate ESI=this/EDI=-1 versus retail EDI=this/ESI=-1. Reusing ESI for
    // the saved DirectPlay error therefore spills this, grows the frame by four
    // bytes, and lets VC6 merge the two late false cleanup tails (3 emitted
    // returns versus retail's 4). DC proves the saved error precedes dialog 456
    // and scopes sErr inside the join-failure block. DC and the retail branch
    // direction both prove the retained negative JoinSession guard with the
    // failure body preceding the final success return; the higher positive-
    // guard spelling omitted that source fact. Naming/parameterizing the -1
    // sentinel is copy-propagated byte-flat, as the register model predicts;
    // the remaining role swap is not a statement-level lever.
    CMPInputDlg searchDlg(20, 20);
    searchDlg.m_header1->setText(g_generalText->getText(GENERAL_TEXT_NETWORK_HOST_ADDRESS_PROMPT));
    searchDlg.m_header2->setText(g_generalText->getText(GENERAL_TEXT_PASSWORD_OPTIONAL));
    searchDlg.m_field1->setHelpText(g_searchAddressHelp, 0, 0);
    searchDlg.m_field2->setHelpText(g_sessionPasswordHelp, 0, 0);
    searchDlg.disableOK();
    searchDlg.doModal(0);

    if (g_windowManager->m_dialogReturn == DIALOG_RETURN_CANCEL)
        return 0;

    remoteCleanup();
    if (!initRemote(MP_TCP, searchDlg.m_field1->getText(), 0)) {
        normalDialog(g_generalText->getText(GENERAL_TEXT_TCP_IP_CONNECTION_INITIALIZATION_ERROR), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        return 0;
    }

    CHourGlass hourGlass(1);
    m_sessions->destroy();
    g_dPlay->enumSessions(m_sessions, 5000, 0x42);

    if (!m_sessions->getCount()) {
        normalDialog(g_generalText->getText(GENERAL_TEXT_IP_ADDRESS_WAS_NOT_FOUND), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        remoteCleanup();
        initRemote(MP_TCP, 0, 0);
        return 0;
    }

    if (!joinSession(m_sessions->get(0), 0)) {
        // Original DC local name: sErr.
        char errorText[256];
        long lastError = g_dPlay->getLastError();
        normalDialog(g_generalText->getText(GENERAL_TEXT_SESSION_CONNECTION_ERROR), 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        g_dPlay->getErrorDesc(lastError, errorText);
        normalDialog(errorText, 1, -1, -1,
                     -1, 0, -1, 0, -1, 0, -1, 0);
        remoteCleanup();
        initRemote(MP_TCP, 0, 0);
        return 0;
    }
    return 1;
}

VA(0x00511d40, 0xD1)  // dc 0x101c00
unsigned char TMultiPlayerWindow::onHotSeat()
{
    CHotSeatDlg dlg;
    dlg.doModal(0);
    if (g_windowManager->m_dialogReturn == DIALOG_RETURN_CANCEL)
        return 0;
    g_mpNetProtocol = MP_HOTSEAT;
    return 1;
}

// Original: TMultiPlayerWindow::IsNT; multiplayerwindow.cpp:2061, dc 0x101cfc.
// The ordinary platform query uses the PC ANSI OSVERSIONINFO representation.
unsigned char TMultiPlayerWindow::isNT()
{
    OSVERSIONINFOA versionInfo;
    versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
    if (GetVersionExA(&versionInfo)) {
        if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
            return 1;
    }
    return 0;
}

// DC CHotSeatDlg::WindowHandler (786, dc 0x102e24) invokes VRKeyboard
// for player-name edits. Complete vtable 0x6401d8 slot 9 uses the inherited
// CHeroWindowEx::windowHandler (0x5ff820); CHotSeatEdit handles native keys.

VA(0x00511e20, 0x5A1)  // dc 0x1028c4
CHotSeatDlg::CHotSeatDlg()
    : CHeroWindowEx(218, 96, 363, 407, 18)
{
    m_widgets.reserve(19);
    m_widgets.push_back(new bitmapBorder(0, 0, m_width, m_height, BACKGROUND_ID,
                                       "muhotsea.pcx", 0x800));
    m_widgets.push_back(new textWidget(0, 30, m_width, 150,
                                     g_generalText->getText(GENERAL_TEXT_HOTSEAT_NAME_PROMPT), "bigfont.fnt",
                                     font::WHITE, HEADER_ID, 1, 0, 8));

    int sy = 178;
    for (int i = 0; i < 8; i++) {
        m_edit[i] = new CHotSeatEdit(277 - m_x, sy - m_y, 281, 18, 21, "",
                                   "smalfont.fnt", font::WHITE, 0, 0, 0,
                                   FIRST_EDIT_ID + i, 0x100, 0, 7, 5);
        m_edit[i]->setHelpText(g_hotSeatEditRollover, g_hotSeatEditRightClick, 0);
        m_widgets.push_back(m_edit[i]);
        sy += 30;
    }

    int j;
    for (j = 0; j < 7; j++)
        m_edit[j]->setNextEdit(m_edit[j + 1]);
    for (j = 1; j < 8; j++)
        m_edit[j]->setPrevEdit(m_edit[j - 1]);
    m_edit[0]->setPrevEdit(m_edit[7]);
    m_edit[7]->setNextEdit(m_edit[0]);

    m_widgets.push_back(new button(95, 338, 64, 32, OKAY_ID, "mubchck.def", 0, 1,
                                 0, 28, 2));
    m_widgets.push_back(new button(205, 338, 64, 32, BACK_ID, "mubcanc.def", 0, 1,
                                 0, 1, 2));

    m_rollover = new textWidget(10, 382, m_width - 20, 18, 0, "smalfont.fnt",
                                font::PRIMARY, ROLLOVER_ID, 1, 32, 8);
    m_widgets.push_back(m_rollover);

    addWidgetsToMessageStream();
    m_edit[0]->setText(g_multiPlayerWindow->m_playerName->m_text.c_str());
    setFocus(m_edit[0]->m_id);
    for (j = 0; j < 8; j++)
        m_edit[j]->setAutoDraw(1);
    widget* w = getWidget(OKAY_ID);
    w->setHelpText(g_dialogOkHelp, 0, 0);
    w->enable(0);
    w = getWidget(BACK_ID);
    w->setHelpText(g_dialogBackHelp, 0, 0);
}

VA(0x005123d0, 0x4E)  // dc 0x102c28
CHotSeatDlg::~CHotSeatDlg()
{
    deleteWidgets();
}

VA(0x00512420, 0x4D)  // dc 0x102c6c
int CHotSeatDlg::onWidgetDeselect(int id, bool& exitFlag)
{
    switch (id) {
    case OKAY_ID:
        if (onOK()) {
            exitFlag = 1;
            g_windowManager->m_dialogReturn = DIALOG_RETURN_OK;
        }
        break;

    case BACK_ID:
        exitFlag = 1;
        g_windowManager->m_dialogReturn = DIALOG_RETURN_CANCEL;
        return 1;
    }

    return 0;
}

VA(0x00512470, 0xB2)  // dc 0x102d88
unsigned char CHotSeatDlg::onOK()
{
    g_hotSeatMan = new CHotSeatMan;

    for (int i = 0; i < 8; ++i) {
        if (strlen(m_edit[i]->m_text.c_str()))
            g_hotSeatMan->addPlayer(m_edit[i]->m_text.c_str());
    }

    return 1;
}

VA(0x00512530, 0x4)  // dc 0x102e1c
textWidget* CHotSeatDlg::getRolloverWidget()
{
    return m_rollover;
}

VA_COMPGEN(0x00512540, 0x21, SCALAR_DELETING_DTOR, CHotSeatDlg)

// E:\gamedcs\array.h:51. DC emits this CDPlaySession specialization from
// dxplay.obj, while Complete's selected COMDAT physically occupies the
// multiplayerwindow.obj band and is called by OnSearch. The active template
// definition remains in multiplayerwindow.h.
#if 0  // @carcass: claim-only - definition lives in multiplayerwindow.h
VA(0x00512570, 0x53)  // exact selected COMDAT, dc 0x8c0c0
void CAutoArray<CDPlaySession>::destroy(unsigned char deleteData)
{
    // @stub
}
#endif  // @carcass

// E:\gamedcs\array.h:113,127. These selected CAutoArray<CDPlaySession>
// COMDATs physically occupy multiplayerwindow.obj and DC records the same
// specialization in this module. Their active definitions remain in
// multiplayerwindow.h so the surrounding template instantiation keeps its
// source shape; these disabled declarators only assign the retail homes.
#if 0  // @carcass: claim-only - definitions live in multiplayerwindow.h
VA(0x005125d0, 0x3A)  // exact selected COMDAT, dc 0x103150
unsigned char CAutoArray<CDPlaySession>::deleteElement(unsigned long elementNbr)
{
    // @stub
}

VA(0x00512610, 0x5B)  // exact selected COMDAT, dc 0x10318c
unsigned char CAutoArray<CDPlaySession>::insert(
    unsigned long nextElementNbr, CDPlaySession* element)
{
    // @stub
}
#endif  // @carcass

VA_COMPGEN(0x00512670, 0x6C, SCALAR_DELETING_DTOR, CAutoArray)
